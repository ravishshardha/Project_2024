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
        sizeOfArray = 64; 
    }
    else
    {
        printf("\n Please provide the size of the array");
        return 0;
    }

    MPI_Init(&argc, &argv);

    int taskid, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid); // Get the rank of the processor
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // Get the total number of processors

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
    CALI_MARK_BEGIN(whole_computation); 

    

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
    
    // // Step 1: Sort local data
    // std::sort(local_data.begin(), local_data.end());

    // MPI_Gather(local_array.data(), local_size, MPI_INT, sorted_array.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // // Step 2: Choose p-1 samples from local data
    // std::vector<int> local_samples = select_samples(local_data, num_procs);


    CALI_MARK_END(whole_computation); 

    //printf("Whole Computation Time: %f \n", whole_computation_time);

    //print arrays for debugging
    //printArray(global_data,taskid);

    // Flush Caliper output before finalizing MPI
   mgr.stop();
   mgr.flush();

   MPI_Finalize();
}