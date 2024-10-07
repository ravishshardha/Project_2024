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

int main(int argc, char** argv) {
    
    /********** Define Caliper ***************/
    const char* data_init_runtime = "data_init_runtime";
    const char* correctness_check = "correctness_check";
    const char* comm  = "comm";
    const char* comm_small = "comm_small";
    const char* comm_large = "comm_large";
    const char* comp  = "comp";
    const char* comp_small = "comp_small";
    const char* comp_large = "comp_large";



    /********** Initialize MPI **********/
    int world_rank;
    int world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Adiak metadata collection
    adiak::init(NULL);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "merge");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", atoi(argv[1]));
    adiak::value("input_type", "Random"); // Adjust this based on your test input type
    adiak::value("num_procs", world_size);
    adiak::value("scalability", "strong"); // Adjust this based on your test type
    adiak::value("group_num", 13);          // Set your group number
    adiak::value("implementation_source", "online and handwritten");

    /********** Get the array size from parameters **********/
    if (argc != 2) {
        if (world_rank == 0) {
            fprintf(stderr, "Usage: %s <array_size>\n", argv[0]);
        }
        MPI_Finalize();
        return -1;
    }

    int n = atoi(argv[1]);

    /********** Create and populate the array **********/
    int *original_array = NULL;
    if (world_rank == 0) {
        CALI_MARK_BEGIN(data_init_runtime);
        original_array = (int *)malloc(n * sizeof(int));
        srand(time(NULL));
        printf("This is the unsorted array: ");
        for (int i = 0; i < n; i++) {
            original_array[i] = rand() % n;
            printf("%d ", original_array[i]);
        }
        printf("\n\n");
        CALI_MARK_END(data_init_runtime);
    }

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

    /********** Make the final mergeSort call **********/
    if (world_rank == 0) {
        int *other_array = (int *)malloc(n * sizeof(int));
        CALI_MARK_BEGIN(comp);
        CALI_MARK_BEGIN(comp_large);
        mergeSort(sorted, other_array, 0, n - 1);
        CALI_MARK_END(comp_large);
        CALI_MARK_END(comp);

        /********** Display the sorted array **********/
        printf("This is the sorted array: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", sorted[i]);
        }
        printf("\n\n");
        
        /************ Correctness Check **************/
        CALI_MARK_BEGIN(correctness_check);
        if (is_sorted(sorted, n)) {
          printf("The array is sorted correctly.\n");
        } 
        else {
          printf("The array is NOT sorted correctly.\n");
        }
        CALI_MARK_END(correctness_check);

        /********** Clean up root **********/
        free(sorted);
        free(other_array);
    }

    /********** Clean up rest **********/
    if (world_rank == 0) {
        free(original_array);
    }
    free(sub_array);
    free(tmp_array);

    /********** Finalize MPI **********/
    MPI_Finalize();

    return 0;
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
