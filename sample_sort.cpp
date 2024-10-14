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
const char* input_type = "random";
const char* MPI_Init = "MPI_Init";
const char* MPI_Gather = "MPI_Gather";
const char* MPI_Comm = "MPI_Comm";
const char* MPI_Bcast = "MPI_Bcast";
// const char* master_initialization = "master_initialization";
// const char* master_send_recieve = "master_send_recieve";
// const char* worker_recieve = "worker_recieve";
// const char* worker_calculation = "worker_calculation";
// const char* worker_send = "worker_send";



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

    if (argc == 2)
    {
        sizeOfArray = atoi(argv[1]); 
    }
    else
    {
        printf("\n Please provide the size of the array");
        return 0;
    }

    CALI_MARK_BEGIN(MPI_Init);
    MPI_Init(&argc, &argv);
    CALI_MARK_END(MPI_Init);

    int taskid, numProcs;
    CALI_MARK_BEGIN(MPI_Comm); 
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid); // Get the rank of the processor
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // Get the total number of processors
    CALI_MARK_END(MPI_Comm); 

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

    
    CALI_MARK_BEGIN(data_init_runtime); 
    if(taskid == 0){
        // Generate Random array (vector) of elements
        global_data = generateRandomUniqueArray(sizeOfArray);
        printf("Size of array: %d ", sizeOfArray);
        printArray(global_data,0);

        // Distribute data evenly among processes
        int chunk_size = sizeOfArray / numProcs;
        for (int i = 0; i < numProcs; ++i) {
            if (i == 0) {
                local_data.assign(global_data.begin(), global_data.begin() + chunk_size);
            } else {
                MPI_Send(global_data.data() + i * chunk_size, chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        // recieve local data
        int chunk_size = sizeOfArray / numProcs;
        local_data.resize(chunk_size);
        MPI_Recv(local_data.data(), chunk_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    }
    CALI_MARK_END(data_init_runtime); 
    
    // Part 1: Sort local data
    CALI_MARK_BEGIN(comp); 
    CALI_MARK_BEGIN(comp_small); 
    std::sort(local_data.begin(), local_data.end());
    CALI_MARK_END(comp_small); 
    CALI_MARK_END(comp); 
   // MPI_Gather(local_data.data(), sizeOfArray, MPI_INT, sorted_array.data(), sizeOfArray, MPI_INT, 0, MPI_COMM_WORLD);

    // debug print sorted inital arrays
    //printArray(local_data,taskid);

    // Part 2: Choose p-1 samples from local data
    std::vector<int> local_samples;
    local_samples = select_samples(local_data, numProcs);

    //printf("local samples... \n");

    //printArray(local_samples,taskid);

    // Part 3: Gather samples at the root process
    std::vector<int> gathered_samples;

    // sample processing will be handled by process 0
    // gathered_samples will be length 0 for other processes
    if (taskid == 0) {
        gathered_samples.resize(numProcs * (numProcs - 1));
    }

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    CALI_MARK_BEGIN(MPI_Gather);
    MPI_Gather(local_samples.data(), numProcs - 1, MPI_INT, 
           taskid == 0 ? gathered_samples.data() : NULL, numProcs - 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(MPI_Gather);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    // Part 4: Sort the gathered samples and choose pivots
    std::vector<int> pivots;
    
    // process/rank 0 handles the pivot selection
    if (taskid == 0) {
        std::sort(gathered_samples.begin(), gathered_samples.end());
        for (int i = 1; i < numProcs; i++) {
            pivots.push_back(gathered_samples[i * (numProcs - 1)]);
        }

        printf("pivots... \n");
        printArray(pivots,0);
    }
    pivots.resize(numProcs - 1);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    CALI_MARK_BEGIN(MPI_Bcast);
    // Part 5: Broadcast pivots to processes
    MPI_Bcast(pivots.data(), numProcs - 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(MPI_Bcast);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Part 6: Partition local data based on pivots
    std::vector<std::vector<int>> partitions(numProcs);
    int last_index = 0;
    for (int i = 0; i < numProcs - 1; i++) {
        auto it = std::upper_bound(local_data.begin() + last_index, local_data.end(), pivots[i]);
        partitions[i] = (std::vector<int>(local_data.begin() + last_index, it));
        last_index = it - local_data.begin();
    }
    partitions[numProcs - 1].assign(local_data.begin() + last_index, local_data.end());

    // Display partition sizes in each process
    // std::cout << "Rank " << taskid << " partitions: ";
    // for (size_t i = 0; i < partitions.size(); ++i)
    //     std::cout << partitions[i].size() << " ";
    // std::cout << std::endl;

    // Part 7: Send partitioned data to corresponding processes
    std::vector<int> send_counts(numProcs), recv_counts(numProcs);
    std::vector<int> send_displs(numProcs), recv_displs(numProcs);
    // for (int i = 0; i < numProcs; ++i) {
    //     send_counts[i] = partitions[i].size();
    // }
    

    // std::cout << "Rank " << taskid << " send counts: " << std::endl;
    // for (int i = 0; i < numProcs; ++i) std::cout << send_counts[i] << " ";
    // std::cout << "Rank " << taskid << " receive counts: ";
    // for (int i = 0; i < numProcs; ++i) std::cout << recv_counts[i] << " ";
    // std::cout << std::endl;

    // prepare send buffer
    // std::vector<int> send_data;
    // for (const auto& part : partitions) {
    //     send_data.insert(send_data.end(), part.begin(), part.end());
    // }

    // // prepare recieve buffer
    
    // int total_recv_size = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
    // recv_data.resize(total_recv_size);
    

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

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);


    int total_recv = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
    recv_displs[0] = 0;
    for (int i = 1; i < numProcs; ++i) {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }
    std::vector<int> recv_data(total_recv);
    //calcuate displacement arrays for Alltoallv
    // std::vector<int> send_displs(numProcs), recv_displs(numProcs);
    // send_displs[0] = 0;
    // recv_displs[0] = 0;
    // for (int i = 1; i < numProcs; ++i) {
    //     send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
    //     recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    // }
    // std::partial_sum(send_counts.begin(), send_counts.end() - 1, send_displs.begin() + 1);
    // std::partial_sum(recv_counts.begin(), recv_counts.end() - 1, recv_displs.begin() + 1);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Alltoallv(send_buffer.data(), send_counts.data(), send_displs.data(), MPI_INT,
                  recv_data.data(), recv_counts.data(), recv_displs.data(), MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    // Part 8: Sort the received data
    //CALI_MARK_BEGIN("final_sort");
    std::sort(recv_data.begin(), recv_data.end());
    //CALI_MARK_END("final_sort");
    
    printf("sorted pivoted buckets...\n");
    printArray(recv_data,taskid);

    

     // Part 9: Gather sorted data at the root process

    std::vector<int> recv_sizes(numProcs);
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    CALI_MARK_BEGIN(MPI_Gather);
    MPI_Gather(&total_recv, 1, MPI_INT, recv_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END(MPI_Gather);
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
    // MPI_Gatherv(local_data.data(), local_data.size(), MPI_INT,
    //         rank == 0 ? final_data.data() : NULL, recv_counts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

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

    //CALI_MARK_END(whole_computation); 

    //printf("Whole Computation Time: %f \n", whole_computation_time);

    //print arrays for debugging
    //printArray(global_data,taskid);

    // Flush Caliper output before finalizing MPI
   mgr.stop();
   mgr.flush();

   MPI_Finalize();
   return 0;
}