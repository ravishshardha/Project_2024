#include <mpi.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <random>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>




/* Define Caliper region names */
const char* whole_computation = "whole_computation";
const char* data_init_runtime = "data_init_runtime";
const char* comm = "comm";
const char* comp = "comp";
const char* comm_small = "comm_small";
const char* comm_large = "comm_large";
const char* comp_small = "comp_small";
const char* comp_large = "comp_large";
const char* correctness_check = "correctness_check";
const char* input_type = "Random";
const char* MPI_Comm_Cali = "MPI_Comm";

void generate_array(std::vector<int>* array, int size, const char* input_type) {
    srand(time(NULL));

    if (strcmp(input_type, "Random") == 0) {
        // Random array
        for (int i = 0; i < size; i++) {
            array->at(i) = rand() % size;
        }
    } else if (strcmp(input_type, "ReverseSorted") == 0) {
        // Reverse sorted array
        int max_val = size * 2;  // Start with a larger value than size
        for (int i = 0; i < size; i++) {
            int decrement = rand() % (size / 5 + 1);  // Sometimes allows 0 decrement, creating repeated values
            max_val -= decrement;
            array->at(i) = max_val;
        }
    } else if (strcmp(input_type, "Sorted") == 0) {
        // Sorted array with random values, allowing for repeats
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);  // Sometimes allows 0 increment, creating repeated values
            min_val += increment;
            array->at(i) = min_val;
        }
    } else if (strcmp(input_type, "1_perc_perturbed") == 0) {
        // 1% Permuted array
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);  // Sometimes allows 0 increment
            min_val += increment;
            array->at(i) = min_val;
        }

        // Permute 1% of the array
        int permuted_count = size / 100;
        for (int i = 0; i < permuted_count; i++) {
            int index1 = rand() % size;
            int index2 = rand() % size;
            int temp = array->at(index1);
            array->at(index1) = array->at(index2);
            array->at(index2) = temp;
        }
    }
}

std::vector<int> generateRandomUniqueArray(int numElements) {
    std::vector<int> arr(numElements);
    for (int i = 0; i < numElements; ++i) {
        arr[i] = i; 
    }

    
    // Random number generator (Mersenne Twister)
    std::random_device rd;
    std::mt19937 g(rd());  

    // Shuffle the array
    std::shuffle(arr.begin(), arr.end(), g); 

    return arr;  
}

std::vector<int> select_samples(const std::vector<int>& local_data, int numProcs) {
    std::vector<int> samples;
    int step = local_data.size() / numProcs;
    for (int i = step / 2; i < local_data.size(); i += step) {
        samples.push_back(local_data[i]);
    }
    return samples;
}

int is_sorted(const std::vector<int>& array, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0; // Array is not sorted
        }
    }
    return 1; // Array is sorted
}

void printArray(const std::vector<int>& arr, int rank) {
    printf("Processor %d :",rank);
    for (int x : arr) {
        printf("%d ",x);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    CALI_CXX_MARK_FUNCTION;

    int sizeOfArray;

    if (argc == 3)
    {
        sizeOfArray = atoi(argv[1]); 
        input_type = argv[2];
    }
    else
    {
        printf("\n Please provide the size of the array and input type");
        return 0;
    }

    MPI_Init(&argc, &argv);

    int taskid, numProcs;
    CALI_MARK_BEGIN(MPI_Comm_Cali); 
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid); // Get the rank of the processor
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // Get the total number of processors
    CALI_MARK_END(MPI_Comm_Cali); 

    // check at least 2 processors are running
    int rc;
    if (numProcs < 2 ) {
    printf("Need at least two MPI tasks. Quitting...\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
    exit(1);
    }

    //NOTE: this can change due to how team generates data
    std::vector<int> local_data;
    std::vector<int> global_data;

    // Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();
    //CALI_MARK_BEGIN(whole_computation); 

    int chunk_size = sizeOfArray / numProcs;
    CALI_MARK_BEGIN(data_init_runtime); 
    local_data.resize(chunk_size);
    generate_array(&local_data,chunk_size,input_type );
        
    CALI_MARK_END(data_init_runtime); 

    /********** Set Adiak Values **********/
    adiak::value("algorithm", "sample");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", sizeOfArray);
    adiak::value("input_type", input_type);
    adiak::value("num_procs", numProcs);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 13);
    adiak::value("implementation_source", "AI (ChatGPT) and Online (http://users.atw.hu/parallelcomp/ch09lev1sec5.html)");
    
    // Part 1: Sort local data
    CALI_MARK_BEGIN(comp); 
    CALI_MARK_BEGIN(comp_small); 
    std::sort(local_data.begin(), local_data.end());
    

    // Part 2: Choose p-1 samples from local data
    std::vector<int> local_samples;
    local_samples = select_samples(local_data, numProcs);


    // Part 3: Gather samples at the root process
    std::vector<int> gathered_samples;

    // sample processing will be handled by process 0
    // gathered_samples will be length 0 for other processes
    if (taskid == 0) {
        gathered_samples.resize(numProcs * (numProcs - 1));
    }
    CALI_MARK_END(comp_small); 
    CALI_MARK_END(comp); 

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Gather(local_samples.data(), numProcs - 1, MPI_INT, 
           taskid == 0 ? gathered_samples.data() : NULL, numProcs - 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    // Part 4: Sort the gathered samples and choose pivots
    std::vector<int> pivots;
    
    // process/rank 0 handles the pivot selection
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    if (taskid == 0) {
        std::sort(gathered_samples.begin(), gathered_samples.end());
        for (int i = 1; i < numProcs; i++) {
            pivots.push_back(gathered_samples[i * (numProcs - 1)]);
        }

        //printf("pivots... \n");
        //printArray(pivots,0);
    }
    pivots.resize(numProcs - 1);
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    // Part 5: Broadcast pivots to processes
    MPI_Bcast(pivots.data(), numProcs - 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Part 6: Partition local data based on pivots
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    std::vector<std::vector<int>> partitions(numProcs);
    int last_index = 0;
    for (int i = 0; i < numProcs - 1; i++) {
        auto it = std::upper_bound(local_data.begin() + last_index, local_data.end(), pivots[i]);
        partitions[i] = (std::vector<int>(local_data.begin() + last_index, it));
        last_index = it - local_data.begin();
    }
    partitions[numProcs - 1].assign(local_data.begin() + last_index, local_data.end());
    CALI_MARK_END(comp_large);
    CALI_MARK_END(comp);
    
    // Part 7: Send partitioned data to corresponding processes
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    std::vector<int> send_counts(numProcs), recv_counts(numProcs);
    std::vector<int> send_displs(numProcs), recv_displs(numProcs);
    

    // Calculate send counts and displacements
    int total_send = 0;
    for (int i = 0; i < numProcs; ++i) {
        send_counts[i] = partitions[i].size();
        send_displs[i] = total_send;
        total_send += send_counts[i];
    }

    // Prepare send buffer
    std::vector<int> send_buffer(total_send);
    for (int i = 0, idx = 0; i < numProcs; ++i) {
        std::copy(partitions[i].begin(), partitions[i].end(), send_buffer.begin() + idx);
        idx += partitions[i].size();
    }
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    int total_recv = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
    recv_displs[0] = 0;
    for (int i = 1; i < numProcs; ++i) {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }
    std::vector<int> recv_data(total_recv);
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);
   

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Alltoallv(send_buffer.data(), send_counts.data(), send_displs.data(), MPI_INT,
                  recv_data.data(), recv_counts.data(), recv_displs.data(), MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Part 8: Sort the received data
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    std::sort(recv_data.begin(), recv_data.end());
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);
    
     // Part 9: Gather sorted data at the root process

    std::vector<int> recv_sizes(numProcs);
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Gather(&total_recv, 1, MPI_INT, recv_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Calculate displacements for final gather at Rank 0
    std::vector<int> final_displs(numProcs, 0);
    if (taskid == 0) {
        // Calculate displacement offsets for gathering
        std::partial_sum(recv_sizes.begin(), recv_sizes.end() - 1, final_displs.begin() + 1);
    }


    std::vector<int> final_sorted_data;
    
    if (taskid == 0) {
        final_sorted_data.resize(sizeOfArray);
    }
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Gatherv(recv_data.data(), total_recv, MPI_INT, 
            final_sorted_data.data(), recv_sizes.data(), final_displs.data(), MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    // Part 10: Print final results
    if(taskid == 0){
        printf("Final sorted data");
        printArray(final_sorted_data,0);
    }

    /********** Correctness Check **********/
    CALI_MARK_BEGIN(correctness_check);
    if (is_sorted(final_sorted_data, final_sorted_data.size())) {
        printf("Array sorted correctly for %s.\n", input_type);
    } else {
        printf("Array NOT sorted correctly for %s.\n", input_type);
    }
    CALI_MARK_END(correctness_check);


    //print arrays for debugging
    //printArray(global_data,taskid);

    // Flush Caliper output before finalizing MPI
   mgr.stop();
   mgr.flush();

   MPI_Finalize();
   return 0;
}