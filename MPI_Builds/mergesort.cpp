#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <adiak.hpp>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>

void merge(int *, int *, int, int, int);
void mergeSort(int *, int *, int, int);
int is_sorted(int *, int);
void process_sorting_logic(int*, int, int, int, const char*);
void generate_array(int*, int, const char*);
void set_adiak_values(const char*, int, int);

/********** Define Caliper ***************/
const char* data_init_runtime = "data_init_runtime";
const char* correctness_check = "correctness_check";
const char* comm  = "comm";
const char* comm_small = "comm_small";
const char* comm_large = "comm_large";
const char* comp  = "comp";
const char* comp_small = "comp_small";
const char* comp_large = "comp_large";

int main(int argc, char** argv) {
    /********** Initialize MPI **********/
    int world_rank;
    int world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /********** Get the array size and input type from parameters **********/
    if (argc != 3) {
        if (world_rank == 0) {
            fprintf(stderr, "Usage: %s <array_size> <input_type>\n", argv[0]);
            fprintf(stderr, "Input type can be: Random, ReverseSorted, Sorted, 1_perc_perturbed\n");
        }
        MPI_Finalize();
        return -1;
    }

    int n = atoi(argv[1]);
    const char* input_type = argv[2];


    /********** Create and process the array based on input type **********/
    int *original_array = NULL;

    // Only the root process creates and initializes the array
    if (world_rank == 0) {
        original_array = (int *)malloc(n * sizeof(int));
        
        CALI_MARK_BEGIN(data_init_runtime);
        generate_array(original_array, n, input_type);
        CALI_MARK_END(data_init_runtime);
        
        // Print unsorted array (first 10 elements)
        printf("Unsorted array for %s: ", input_type);
        for (int j = 0; j < n; j++) {
            printf("%d ", original_array[j]);
        }
        printf("\n");
    }

    // Set Adiak values based on input type
    set_adiak_values(input_type, n, world_size);

    // Call the main sorting process
    process_sorting_logic(original_array, n, world_rank, world_size, input_type);

    // Clean up root array
    if (world_rank == 0) {
        free(original_array);
    }

    /********** Finalize MPI **********/
    MPI_Finalize();

    return 0;
}

/********** Generate Arrays of Different Input Types **********/
void generate_array(int *array, int size, const char* input_type) {
    srand(time(NULL));

    if (strcmp(input_type, "Random") == 0) {
        // Random array
        for (int i = 0; i < size; i++) {
            array[i] = rand() % size;
        }
    } else if (strcmp(input_type, "ReverseSorted") == 0) {
        // Reverse sorted array
        for (int i = 0; i < size; i++) {
            array[i] = size - i;
        }
    } else if (strcmp(input_type, "Sorted") == 0) {
        // Sorted array
        for (int i = 0; i < size; i++) {
            array[i] = i;
        }
    } else if (strcmp(input_type, "1_perc_perturbed") == 0) {
        // 1% Permuted array
        for (int i = 0; i < size; i++) {
            array[i] = i;
        }
        int permuted_count = size / 100;
        for (int i = 0; i < permuted_count; i++) {
            int index1 = rand() % size;
            int index2 = rand() % size;
            int temp = array[index1];
            array[index1] = array[index2];
            array[index2] = temp;
        }
    }
}

/********** Set Adiak Values for Metadata **********/
void set_adiak_values(const char* input_type, int size, int world_size) {
    adiak::value("algorithm", "merge");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", size);
    adiak::value("input_type", input_type);
    adiak::value("num_procs", world_size);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 13);
    adiak::value("implementation_source", "online and handwritten");
}

/********** Main Sorting Logic **********/
void process_sorting_logic(int* original_array, int n, int world_rank, int world_size, const char* input_type) {

    /********** Divide the array in equal-sized chunks **********/
    int size = n / world_size;

    /********** Send each subarray to each process **********/
    int *sub_array = (int *)malloc(size * sizeof(int));
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Scatter(original_array, size, MPI_INT, sub_array, size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    /********** Perform the mergesort on each process **********/
    int *tmp_array = (int *)malloc(size * sizeof(int));
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    mergeSort(sub_array, tmp_array, 0, size - 1);
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);
    
    /********** Gather the sorted subarrays into one **********/
    int *sorted = NULL;
    if (world_rank == 0) {
        sorted = (int *)malloc(n * sizeof(int));
    }

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Gather(sub_array, size, MPI_INT, sorted, size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    /********** Final mergeSort call on the root **********/
    if (world_rank == 0) {
        int *other_array = (int *)malloc(n * sizeof(int));
        CALI_MARK_BEGIN(comp);
        CALI_MARK_BEGIN(comp_large);
        mergeSort(sorted, other_array, 0, n - 1);
        CALI_MARK_END(comp_large);
        CALI_MARK_END(comp);

        /********** Correctness Check **********/
        CALI_MARK_BEGIN(correctness_check);
        if (is_sorted(sorted, n)) {
            printf("Array sorted correctly for %s.\n", input_type);
        } else {
            printf("Array NOT sorted correctly for %s.\n", input_type);
        }
        CALI_MARK_END(correctness_check);

        // Print sorted array (first 10 elements)
        printf("Sorted array for %s: ", input_type);
        for (int j = 0; j < n; j++) {
            printf("%d ", sorted[j]);
        }
        printf("\n");

        /********** Clean up root **********/
        free(sorted);
        free(other_array);
    }

    /********** Clean up rest **********/
    free(sub_array);
    free(tmp_array);
}


int is_sorted(int *array, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0; // Array is not sorted
        }
    }
    return 1; // Array is sorted
}

/********** Merge Function **********/
void merge(int *a, int *b, int l, int m, int r) {
    int h = l, i = l, j = m + 1;

    while ((h <= m) && (j <= r)) {
        if (a[h] <= a[j]) {
            b[i] = a[h];
            h++;
        } else {
            b[i] = a[j];
            j++;
        }
        i++;
    }

    if (h > m) {
        for (int k = j; k <= r; k++) {
            b[i++] = a[k];
        }
    } else {
        for (int k = h; k <= m; k++) {
            b[i++] = a[k];
        }
    }

    for (int k = l; k <= r; k++) {
        a[k] = b[k];
    }
}

/********** Recursive MergeSort Function **********/
void mergeSort(int *a, int *b, int l, int r) {
    if (l < r) {
        int m = (l + r) / 2;
        mergeSort(a, b, l, m);
        mergeSort(a, b, m + 1, r);
        merge(a, b, l, m, r);
    }
}
