#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <climits> 

void generate_array(int *array, int size, const char* input_type);
int is_sorted(int *array, int size);
void radix_sort(int *array, int size, int max_digits, int world_rank, int world_size);
void counting_sort_for_radix(int *array, int size, int place);

int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;

    /********** Initialize MPI **********/
    int world_rank, world_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    cali::ConfigManager mgr;
    mgr.start();

    int n = atoi(argv[1]); // Input size
    const char* input_type = argv[2]; // Array type

    /********** Create and process the array based on input type **********/
    int *original_array = NULL;
    if (world_rank == 0) {
        original_array = (int *)malloc(n * sizeof(int));
        
        CALI_MARK_BEGIN("data_init_runtime");
        generate_array(original_array, n, input_type);
        CALI_MARK_END("data_init_runtime");

        printf("Unsorted array for %s: ", input_type);
        for (int j = 0; j < n; j++) {
            printf("%d ", original_array[j]);
        }
        printf("\n");
    }

    /********** Set Adiak Values **********/
    
    adiak::init(NULL);
    adiak::launchdate();   
    adiak::libraries();     
    adiak::cmdline();       
    adiak::clustername();   
    adiak::value("algorithm", "Radix");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", n);
    adiak::value("input_type", input_type);
    adiak::value("num_procs", num_proc);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 13);
    adiak::value("implementation_source", "online and handwritten");

    /********** Main Sorting Logic **********/
    int size = n / world_size;
    int *send_array = (int *)malloc(size * sizeof(int));

    // Calculate max_digits based on maximum possible value in array
    int max_val = n; 
    int max_digits = 0;
    while (max_val != 0) {
        max_val /= 10;
        max_digits++;
    }

    /********** Send each subarray to each process **********/
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(original_array, size, MPI_INT, send_array, size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    /********** Perform radix sort on each process **********/
    CALI_MARK_BEGIN("comp");
    radix_sort(send_array, size, max_digits, world_rank, world_size); 
    CALI_MARK_END("comp");

    /********** Gather the sorted subarrays into one **********/
    int *sorted = NULL;
    if (world_rank == 0) {
        sorted = (int *)malloc(n * sizeof(int));
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Gather(send_array, size, MPI_INT, sorted, size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    // Ensure the gathered array is sorted without running a final sort
    if (world_rank == 0) {
    // Merge the sorted subarrays
    int *final_sorted = (int *)malloc(n * sizeof(int));
    int *indices = (int *)calloc(world_size, sizeof(int)); // Track the current index of each subarray
    int total_size = 0;

    while (total_size < n) {
        int min_index = -1; // Declare min_index here
        int min_value = INT_MAX; // Declare min_value here

        // Find the minimum element across the current heads of each subarray
        for (int i = 0; i < world_size; i++) {
            int current_index = indices[i];
            // Debugging output
            if (current_index < size) {
                printf("Process %d checking sorted[%d]: %d\n", world_rank, i * size + current_index, sorted[i * size + current_index]);
            }
            if (current_index < size && sorted[i * size + current_index] < min_value) {
                min_value = sorted[i * size + current_index];
                min_index = i;
            }
        }

        // If a valid index is found, add the min_value to final_sorted
        if (min_index != -1) {
            final_sorted[total_size++] = min_value;
            indices[min_index]++; // Move to the next element in the subarray
        }
    }

    // Print the final sorted array for verification
    printf("Final sorted array before checking: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", final_sorted[i]);
    }
    printf("\n");

    // Final correctness check
    if (is_sorted(final_sorted, n)) {
        printf("Final array sorted correctly for %s.\n", input_type);
    } else {
        printf("Final array NOT sorted correctly for %s.\n", input_type);
    }

    // Clean up root
    free(final_sorted);
    free(indices);
    free(sorted);
    free(original_array);
}

    /********** Clean up rest **********/
    free(send_array);

    /********** Stop Caliper Manager **********/
    mgr.stop();
    mgr.flush();

    /********** Finalize MPI **********/
    MPI_Finalize();
    return 0;
}



/********** Radix Sort Function **********/
void radix_sort(int *array, int size, int max_digits, int world_rank, int world_size) {
    int place = 1; // Start at the least significant digit
    CALI_MARK_BEGIN("comp");
    for (int i = 0; i < max_digits; i++) {
        CALI_MARK_BEGIN("comp_large");
        counting_sort_for_radix(array, size, place);
        CALI_MARK_END("comp_large");     
        place *= 10; // Move to the next digit
    }
    CALI_MARK_END("comp");
}

/********** Counting Sort for a specific digit in Radix Sort **********/
void counting_sort_for_radix(int *array, int size, int place) {
    int *output = (int *)malloc(size * sizeof(int));
    int count[10] = {0}; // Reset count array for each digit

    // Count the occurrences of digits at 'place'
    for (int i = 0; i < size; i++) {
        int digit = (array[i] / place) % 10;
        count[digit]++;
    }

    // Change count[i] to position the digits correctly in the output array
    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    // Build the output array
    for (int i = size - 1; i >= 0; i--) {
        int digit = (array[i] / place) % 10;
        output[count[digit] - 1] = array[i];
        count[digit]--;
    }

    // Copy the sorted array
    for (int i = 0; i < size; i++) {
        array[i] = output[i];
    }

    free(output);
}



/********** Generate array function **********/
void generate_array(int *array, int size, const char* input_type) {
    srand(time(NULL));

    if (strcmp(input_type, "Random") == 0) {
        for (int i = 0; i < size; i++) {
            array[i] = rand() % size;
        }
    } 
    else if (strcmp(input_type, "ReverseSorted") == 0) {
        int max_val = size; // Set max_val to size
        for (int i = 0; i < size; i++) {
            int decrement = rand() % (size / 5 + 1); // Random decrement
            max_val -= decrement; // Decrement max_val
            if (max_val < 0) {
                max_val = 0; // Prevent negative values
            }
            array[i] = max_val; // Assign max_val to the array
            }
    }

     else if (strcmp(input_type, "Sorted") == 0) {
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);
            min_val += increment;
            array[i] = min_val;
        }
    } else if (strcmp(input_type, "1_perc_perturbed") == 0) {
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);
            min_val += increment;
            array[i] = min_val;
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

/********** Check if array is sorted **********/
int is_sorted(int *array, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0; 
        }
    }
    return 1;
}
