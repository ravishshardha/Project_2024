#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <climits>
#include <queue>
#include <iostream>
#include <vector>
#include <algorithm>
#include <mpi.h>
#include <limits.h>
#include <random>
#include <cstring>

using namespace std;


void generate_array(int *array, int size, const char* input_type);
int is_sorted(int *array, int size);
void count_sort(int* array, int n, int exp);
void radix_sort(int*& LocalArray, size_t& LocalSize, int CommSize, int rank, int world_length);

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
    if(world_rank == 0) {
        original_array = (int *)malloc(n * sizeof(int));

        CALI_MARK_BEGIN("data_init_runtime");
        generate_array(original_array, n, input_type);
        CALI_MARK_END("data_init_runtime");

        // printf("Unsorted array for %s: ", input_type);
        // for (int j = 0; j < n; j++) {
        //     printf("%d ", original_array[j]);
        // }
        // printf("\n");
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
    adiak::value("num_procs", world_size);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 13);
    adiak::value("implementation_source", "online and handwritten");

    /********** Main Sorting Logic **********/
    size_t size = n / world_size;
    int *send_array = (int *)malloc(size * sizeof(int));
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /********** Send each subarray to each process **********/
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(original_array, size, MPI_INT, send_array, size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    /********** Perform radix sort on each process **********/
    // CALI_MARK_BEGIN("comp");
    // CALI_MARK_BEGIN("comp_small");
    radix_sort(send_array, size, world_size, world_rank, n);
    // CALI_MARK_END("comp_small");
    // CALI_MARK_END("comp");

    /********** Gather the sorted subarrays into one **********/
    

    // Clean up root
    if (world_rank == 0) {
        free(original_array);
    }
    
    free(send_array);

    /********** Stop Caliper Manager **********/
    mgr.stop();
    mgr.flush();

    /********** Finalize MPI **********/
    MPI_Finalize();
    return 0;
}



void counting_sort(int* Array, int n, int exp) {
    int* OutputArray = new int[n];
    int count[10] = {0};

    //fill count array
    for(int i = 0; i < n; i++) {
        int digit = (Array[i] / exp) % 10;
        count[digit] += 1;
    }

    //build cumulative count array
    for(int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    //fill in the OutputArray array based on the cumulative array
    for(int i = n - 1; i >= 0; i--) {
        int digit = count[(Array[i] / exp) % 10] - 1;
        OutputArray[digit] = Array[i];
        count[(Array[i] / exp) % 10] -= 1;
    }

    //Assign passed in array with OutputArray array
    for(int i = 0; i < n; i++) {
        Array[i] = OutputArray[i];
    }

    delete[] OutputArray;
}

void radix_sort(int*& LocalArray, size_t& LocalSize, int CommSize, int rank, int n) {
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    int GlobalMax, GlobalMin;
    int LocalMin = *min_element(LocalArray, LocalArray + LocalSize);
    int LocalMax = *max_element(LocalArray, LocalArray + LocalSize);
    

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Allreduce(&LocalMax, &GlobalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&LocalMin, &GlobalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");
    

    int* CountSend = new int[CommSize]();
    int* OffsetSend = new int[CommSize]();
    int* CountRecv = new int[CommSize]();
    int* OffsetRecv = new int[CommSize]();

    int Range = (GlobalMax - GlobalMin + 1) / CommSize;

    vector<vector<int>> buckets(CommSize);
    for(int i = 0; i < LocalSize; i++) {
        int value = LocalArray[i];
        int target_proc = (value - GlobalMin) / Range;
        if (target_proc >= CommSize) {
            target_proc = CommSize - 1;
        }
        buckets[target_proc].push_back(value);
    }

    int SendTotal = 0;
    for(int i = 0; i < CommSize; i++) {
        CountSend[i] = buckets[i].size();
        OffsetSend[i] = SendTotal;
        SendTotal += CountSend[i];
    }

    int* DataSend = new int[SendTotal];
    int index = 0;
    for(int i = 0; i < CommSize; i++) {
        for (size_t j = 0; j < buckets[i].size(); j++) {
            DataSend[index++] = buckets[i][j];
        }
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Alltoall(CountSend, 1, MPI_INT, CountRecv, 1, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    int RecvTotal = 0;
    for(int i = 0; i < CommSize; i++) {
        OffsetRecv[i] = RecvTotal;
        RecvTotal += CountRecv[i];
    }

    int* DataRecv = new int[RecvTotal];
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Alltoallv(DataSend, CountSend, OffsetSend, MPI_INT, DataRecv, CountRecv, OffsetRecv, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");
    
    delete[] LocalArray;
    LocalArray = new int[RecvTotal];
    LocalSize = RecvTotal;
    copy(DataRecv, DataRecv + RecvTotal, LocalArray);

    //run count sort for each digit
    int max_val = *max_element(LocalArray, LocalArray + RecvTotal);
    for(long int exp = 1; max_val / exp > 0; exp *= 10) {
        counting_sort(LocalArray, RecvTotal, exp);
    }


    //Gather all subarrays after each digit is processed.
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    //Gather subarrays
    int* GatherSizes = nullptr;
    int* GatherOffsets = nullptr;

    if (rank == 0) {
        GatherSizes = new int[CommSize];
        GatherOffsets = new int[CommSize];
    }

    MPI_Gather(&LocalSize, 1, MPI_INT, GatherSizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int GlobalSize = 0;
    if (rank == 0) {
        GatherOffsets[0] = 0;
        GlobalSize = GatherSizes[0];
        for (int i = 1; i < CommSize; ++i) {
            GatherOffsets[i] = GatherOffsets[i - 1] + GatherSizes[i - 1];
            GlobalSize += GatherSizes[i];
        }
    }

    //Allocate memeory for final array
    int* FinalArray = nullptr;
    if (rank == 0) {
        FinalArray = new int[GlobalSize];
    }

    //Gather local arrays to final
    MPI_Request request;
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Igatherv(LocalArray, LocalSize, MPI_INT, FinalArray, GatherSizes, GatherOffsets, MPI_INT, 0, MPI_COMM_WORLD, &request);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    //Wait to complete
    MPI_Wait(&request, MPI_STATUS_IGNORE);


    if (rank == 0) {
        CALI_MARK_BEGIN("correctness_check");
        if (is_sorted(FinalArray, GlobalSize)) {
            printf("Final array sorted correctly.\n");
        } else {
            printf("Final array NOT sorted correctly.\n");
        }
        CALI_MARK_END("correctness_check");
        delete[] FinalArray; // Clean up
    }

    //Clean up allocated memory
    delete[] CountRecv;
    delete[] OffsetRecv;
    delete[] CountSend;
    delete[] OffsetSend;
    delete[] DataSend;
    delete[] DataRecv;
    delete[] GatherSizes;
    delete[] GatherOffsets;
}





/********** Generate array function **********/
void generate_array(int *array, int size, const char* input_type) {
    srand(time(NULL));  // Seed the random generator
    
    if(strcmp(input_type, "Random") == 0) {
        //Random array with values from 0 to size-1
        for(int i = 0; i < size; i++) {
            array[i] = rand() % size;  //Ensure values are in the range 0 to size-1
        }
    } 
    else if(strcmp(input_type, "ReverseSorted") == 0) {
        //Reverse sorted array only positive values
        int max_val = size * 2;  
        for(int i = 0; i < size; i++) {
            int decrement = rand() % (size / 5 + 1);  
            if (max_val - decrement >= 0) {
                max_val -= decrement;  
            } else {
                max_val = 0;  
            }
            array[i] = max_val;
        }
    } 
    else if(strcmp(input_type, "Sorted") == 0) {
        //Sorted array with random values only positive values
        int min_val = 0; 
        int max_increment = size / 5 + 1;  
        for(int i = 0; i < size; i++) {
            int increment = rand() % max_increment;
            //Check we don't exceed the maximum positive value
            if (min_val + increment >= 0 && min_val + increment <= INT_MAX) {
                min_val += increment;
            } else {
                min_val = INT_MAX;  
            }
            array[i] = min_val;
        }
    } 
    else if(strcmp(input_type, "1_perc_perturbed") == 0) {
        //1% Perturbed array, ensuring only positive values
        int min_val = 0;  
        int max_increment = size / 5 + 1;  
        for(int i = 0; i < size; i++) {
            int increment = rand() % max_increment;
            //Ensure the value stays non-negative and within bounds
            if (min_val + increment >= 0 && min_val + increment <= INT_MAX) {
                min_val += increment;
            } else {
                min_val = INT_MAX;  
            }
            array[i] = min_val;
        }

        //Permute 1% of the array
        int permuted_count = size / 100;  // 1% of the array
        for(int i = 0; i < permuted_count; i++) {
            int index1 = rand() % size;  
            int index2 = rand() % size;  
            //Swap the elements at the two indices
            int temp = array[index1];
            array[index1] = array[index2];
            array[index2] = temp;
        }
    }
}


/********** Check if the array is sorted **********/
int is_sorted(int *array, int size) {
    for(int i = 1; i < size; i++) {
        if(array[i] < array[i - 1]) {
            return 0;
        }
    }
    return 1; 
}
