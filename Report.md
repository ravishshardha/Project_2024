# CSCE 435 Group project

## 0. Group number: 13
The team will communicate with discord channel
## 1. Group members:
1. Ravish Shardha
2. Lydia Harding
3. Brack Harmon
4. Jack Hoppe

## 2.Parallel sorting algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)


All algorithms are going to be written and tested on grace cluster using MPI

- Bitonic Sort:
    - Parallel sorting algorithm that repeatedly sorts segments of a sequence of numbers into bitonic sequences. A bitonic sequence consists of a list of numbers which is first increasing, then decreasing. In the parallel version, for each i round, sorting is done with the partner that differs in the ith bit, alternating between collecting the lower half of all elements, and collecting the higher half of all elements, then sorting in order. Once all rounds have completed, the full array is sorted.
    - Bitonic sort only works on input arrays of size 2^n.
    - Bitonic sort will make the same number of comparisons for any input array, with a complexity of O(log^2n), where n is the number of elements to be sorted.
   
- Sample Sort:
  - Sample sort is a parallelized version of bucket sort. 
  - Each processor takes a chunk of the data and sorts locally. 
  - Then each processor takes s samples and those samples get combined into a buffer and sorted. 
  - Global splitters or pivots are selected form the sorted samples and define endpoints for buckets. 
  - each processor takes its data and filters it into each respective bucket. 
  - The buckets are sorted and combined. 
  - Finally the endpoints of each bucket are checked to verify the whole dataset is sorted.
  - Uses quicksort to sort partitioned buckets, so works well with random and in-order data.  

- Merge Sort:
  - Divide-and-conquer sorting algorithm that splits the array into halves recursively until each sub-array contains a single element, then merges the sub-arrays to produce a sorted array.
  - Best and Worst case time complexity is O(nlogn) and space complexity is O(n).
  - For reverse and random sorted data, merge is a good choice.
  - For sorted and nearly sorted(1% random), merge sort doesn't consider the existing order so might not be      the best choice.
  
- Radix Sort:
   - The Radix sort is a non-comparative integer sorting algorithm that iterates through each digit of the given elements starting from the least significant digit and progressing to the most significant digit. As the algorithm iterates the temporary results are placed in a "bucket" using an algorithm like the "count sort". These buckets are used to create the new ordering of the elements until all digits have been processed. 
   - Through the MPI library and utilizing Grace a parallel implementation can be achieved by splitting input data into chunks then distributing them to workers. The workers will sort its chunk of data based on the current digit. After sorting, the bucket data from the count sort is redistributed across the processes. This continues until the most significant digit is reached resulting in sorted data.
   - The Radix sort requires integers or data that can be represented with integers and a fixed range of digits, i.e., (0-9)
   - Larger integers cause more passes and slows the algorithm
   - Uneven distributions of input data can lead to inefficiencies due to some buckets becoming disproportionately large.
   - Runtime: O(d*n) where d = number of digits and n = number of elements


### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes
- **Merge Sort:**
```
  INITIALIZE MPI(MPI_Init)
  GET world_rank(MPI_Comm_rank)
  GET world_size(MPI_Comm_size)
  
  DIVIDE n by world_size to get chunk_size
  
  CREATE sub_array of size chunk_size
  
  SCATTER original_array to all processes (MPI_Scatter)
  EACH PROCESS receives sub_array of size chunk_size
  
  // Local Merge Sort
  CALL mergeSort(sub_array, 0, chunk_size - 1)
  
  // Gather sorted sub-arrays at root
  GATHER sub_array at root into original_array (MPI_Gather)
  
  IF world_rank == 0 THEN
      CALL mergeSort(original_array, 0, n - 1)  // Final merge at root
      PRINT sorted original_array
  
  //Clean root and the rest of dynamically allocated arrays
  FREE sub_array and temp_arrays
  
  FINALIZE MPI
```

Bitonic Sort:

```
////////////////////
// MAIN
// MPI_Init, 
// MPI_Comm_size(num_procs)
// MPI_Comm_rank(rank)

// Generate the input array on each processor (rand, in-order, reverse-order, or perturbed)

// MPI_Barrier

// BITONIC SORT
// dimensions = log2(num_proc)
// For i = 0 to dimensions - 1:
    // For j = i down to 0:
        if (i + 1)st bit of rank == jth bit of rank then
            COMP EXCHANGE MIN (j)
        else
            COMP EXCHANGE MAX (j)
        // MPI_Barrier to ensure steps are in sync


// MPI_Barrier to ensure all sorting is complete

// VERIFY SORT
// Check if array is sorted locally
// Check if array end is less than start of neighbor process's array
// If both true for all processors, array is sorted.

// free array 

// MPI Finalize
// RETURN
// END MAIN

////////////////////
// HELPER FUNCTIONS

////////////////////
// COMP EXCHANGE MIN (j)
// partner process = rank XOR (1 << j)
// MPI_Sendrecv array with partner, store in buffer_receive

// Concatenate array and buffer receive, store as temp buffer
// Sort temp 
// Set array to be the lower half of temp

// free buffers
// RETURN
// END COMP EXCHANGE MIN
////////////////////

////////////////////
// COMP EXCHANGE MAX (j)
// partner process = rank XOR (1 << j)
// MPI_Sendrecv array with partner, store in buffer_receive

// Concatenate array and buffer receive, store as temp
// Sort temp 
// Set array to be the higher half of temp

// free buffers
// RETURN
// END COMP EXCHANGE MAX
////////////////////
```


Sample Sort:

```
    // Starting MPI commands 
    MPI_Init
    MPI_Comm_rank
    MPI_Comm_world
    etc.

    // Set Number of elements per processor
    data_size = total_size / num_processors

    // Fill local data with random integers
    for each element in data from rank * data_size to (rank + 1) * data_size:
	data[i] = random int from 1 to 999

    // Here is where we start timing the Samplesort algorithm

    // Step 1: Sort the local data
	sort local_data 
    
    // Step 2: Select local data samples
	set samples_list empty
	samples_list[i] = sample an index from local_data
   
    // Step 3: Gather all the samples at rank 0 (rank 0 handles the samples)
	call MPI_Gather with sample data to fill gathered_samples
	

    // Step 4: Rank 0 sorts the samples and selects splitters
	if(rank == 0)
		sort gathered_samples
		for each sample
			splitters[i] = sample from gathered_samples
	

    // Step 5: Broadcast the splitters to all processes 
	call MPI_Bcast with splitters and num_samples

    // Step 6: Partition the local data based on the splitters
	
	create send_counts, send_offsets, and partitioned_data arrays
	populate partitioned_data with info from data based on offsets and send counts
	
    // Step 7: Send partitioned data to respective processor buckets
	use MPI_Alltoall with send_counts and recv_counts
	create recv_data array and populate with received information using MPI_Alltoallv
    
    // Step 8: Sort the received data locally
	sort recv_data

    // Here is where we end timing the Samplesort algorithm
	
    // print sorted data (optional)

    // Ending MPI commands (MPI_Finalize)
```



Radix Sort:

```
1. Initialize MPI, get rank, and size
INITIALIZE MPI(MPI_Init)
GET world_rank(MPI_Comm_rank)
GET world_size(MPI_Comm_size)

2. Generate different types of inputs listed in 2c.
    Create input arrays of different sizes etc
    Define length of data

3. Distribute data to processes with MPI_Scatter
    rank = 0:
        divide n by world size to calculate chunck size
        create arrays with chunk size and then fill
        send the chunks out to the processes
        MPI_Scatter(chunks) 

4. Radix sort for worker
    -iterate through each digit:
        -each process performs local count sort:

        -initialize count array representing numbers 0-9
         array count[10] = {0}; 

        -Count occurence of each digit from local chunk
         Extract current digit with modulo (%)
         count[current digit]++;

        -Gather count data from each process and combine
         MPI_Allreduce(total_count)

        -Caluclate Cumulative count
        cumulative[i] = cumulative[i-1] + total_count[i] 

        -Redistribute the elements based on the combined count
        Then place into processes depending on that calculation
        MPI_Alltoall
        
5. Use MPI_Gather to collect sorted data
    After processing all digits gather data
    MPI_Gather(sorted data)

6. Finalize MPI
MPI_Finalize()
```

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types:
  - The input will consist of multiple arrays of numerical data, with sizes increasing by powers of 2. These arrays will be tested on varying numbers of processors, which will also increase by powers of 2. Furthermore, there would be four main input types: sorted, nearly sorted, random, and reverse sorted.
  - These would be the inputs:
    - For input_size's:
      - 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28
    - For input_type's:
      - Sorted, Random, Reverse sorted, 1%perturbed
    - MPI: num_procs:
      - 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
- Strong scaling (same problem size, increasing number of processors/nodes)
  - We will keep the problem size fixed (e.g., a 2^18-sized array) and increase the number of processors/nodes. For each configuration, we will graph the computation time. This analysis will be done for all four input types: sorted, nearly sorted, random, and reverse sorted. Overall, there would be a total of 7 plots, one for each input size with 4 lines per plot representing each input type.
- Weak scaling (increase problem size, increase number of processors)
  - We will increase the problem size while also increasing the number of processors (e.g., 2, 4, 8, etc.). The computation time for each array size will be graphed across varying numbers of processors. This analysis will also be conducted for all four input types: sorted, nearly sorted, random, and reverse sorted. There would be four plots for each input type for weak scaling.

### 3a. Caliper instrumentation
Please use the caliper build `/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper` 
(same as lab2 build.sh) to collect caliper files for each experiment you run.

Your Caliper annotations should result in the following calltree
(use `Thicket.tree()` to see the calltree):
```
main
|_ data_init_X      # X = runtime OR io
|_ comm
|    |_ comm_small
|    |_ comm_large
|_ comp
|    |_ comp_small
|    |_ comp_large
|_ correctness_check
```

Required region annotations:
- `main` - top-level main function.
    - `data_init_X` - the function where input data is generated or read in from file. Use *data_init_runtime* if you are generating the data during the program, and *data_init_io* if you are reading the data from a file.
    - `correctness_check` - function for checking the correctness of the algorithm output (e.g., checking if the resulting data is sorted).
    - `comm` - All communication-related functions in your algorithm should be nested under the `comm` region.
      - Inside the `comm` region, you should create regions to indicate how much data you are communicating (i.e., `comm_small` if you are sending or broadcasting a few values, `comm_large` if you are sending all of your local values).
      - Notice that auxillary functions like MPI_init are not under here.
    - `comp` - All computation functions within your algorithm should be nested under the `comp` region.
      - Inside the `comp` region, you should create regions to indicate how much data you are computing on (i.e., `comp_small` if you are sorting a few values like the splitters, `comp_large` if you are sorting values in the array).
      - Notice that auxillary functions like data_init are not under here.
    - `MPI_X` - You will also see MPI regions in the calltree if using the appropriate MPI profiling configuration (see **Builds/**). Examples shown below.

All functions will be called from `main` and most will be grouped under either `comm` or `comp` regions, representing communication and computation, respectively. You should be timing as many significant functions in your code as possible. **Do not** time print statements or other insignificant operations that may skew the performance measurements.

### **Nesting Code Regions Example** - all computation code regions should be nested in the "comp" parent code region as following:
```
CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_small");
sort_pivots(pivot_arr);
CALI_MARK_END("comp_small");
CALI_MARK_END("comp");

# Other non-computation code
...

CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_large");
sort_values(arr);
CALI_MARK_END("comp_large");
CALI_MARK_END("comp");
```

### **Calltree Example**:
```
# MPI Mergesort
4.695 main
├─ 0.001 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 2.599 comm
│  ├─ 2.572 MPI_Barrier
│  └─ 0.027 comm_large
│     ├─ 0.011 MPI_Gather
│     └─ 0.016 MPI_Scatter
├─ 0.910 comp
│  └─ 0.909 comp_large
├─ 0.201 data_init_runtime
└─ 0.440 correctness_check
```
#### I) Calltree For Merge Sort:
![image](https://github.com/user-attachments/assets/5e52a84f-a7ce-4625-9742-d039656d652e)

#### II) Calltree For Sample Sort:
![Screenshot 2024-10-30 212641](https://github.com/user-attachments/assets/428b3b97-f619-4b77-a318-1dd478fec2df)


#### III) Calltree for Bitonic Sort:
![calltree](https://github.com/user-attachments/assets/5147e233-5f8f-483a-a47b-056c44bf9c79)


#### VI) Calltree for Radix Sort:
![image](https://cdn.discordapp.com/attachments/1281268344036524110/1296320598678704128/image.png?ex=6711dc23&is=67108aa3&hm=b06794b26da7f35a3944a7f43b78a78902eca5d821d9150ca02f39fa80fa0ceb&)

### 3b. Collect Metadata

Have the following code in your programs to collect metadata:
```
adiak::init(NULL);
adiak::launchdate();    // launch date of the job
adiak::libraries();     // Libraries used
adiak::cmdline();       // Command line used to launch the job
adiak::clustername();   // Name of the cluster
adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
adiak::value("programming_model", programming_model); // e.g. "mpi"
adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
```

They will show up in the `Thicket.metadata` if the caliper file is read into Thicket.

### **See the `Builds/` directory to find the correct Caliper configurations to get the performance metrics.** They will show up in the `Thicket.dataframe` when the Caliper file is read into Thicket.

We have included the metadata code mentioned in 3b in our algorithms.
The values that we are getting in the metadata are the following:
- Launch date of the job
- Libraries used in our algorithm
- Name of the cluster
- Name of the algorithm you are using
- The programming model used for communication which in our case is MPI
- The datatype of input elements, which in our case is int
- The sizeof(datatype) of input(int) elements in bytes
- The number of elements in the input dataset/n - 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28
- The input type used for sorting("Sorted", "ReverseSorted", "Random", "1_perc_perturbed").
- The number of processors used(MPI ranks) - 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
- The scalability of your algorithm. Either if we are doing strong or weak
- Our group number, which is 13
- The place where we got the source code of your algorithm

## 4. Performance evaluation

Include detailed analysis of computation performance, communication performance. 
Include figures and explanation of your analysis.

### 4a. Vary the following parameters
For input_size's:
- 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28

For input_type's:
- Sorted, Random, Reverse sorted, 1%perturbed

MPI: num_procs:
- 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

This should result in 4x7x10=280 Caliper files for your MPI experiments.

### 4b. Hints for performance analysis

To automate running a set of experiments, parameterize your program.

- input_type: "Sorted" could generate a sorted input to pass into your algorithms
- algorithm: You can have a switch statement that calls the different algorithms and sets the Adiak variables accordingly
- num_procs: How many MPI ranks you are using

When your program works with these parameters, you can write a shell script 
that will run a for loop over the parameters above (e.g., on 64 processors, 
perform runs that invoke algorithm2 for Sorted, ReverseSorted, and Random data).  

### 4c. You should measure the following performance metrics
- `Time`
    - Min time/rank
    - Max time/rank
    - Avg time/rank
    - Total time
    - Variance time/rank

### I) Merge Sort
- #### main:
  - Avg time/Rank:
![Avg timr](https://github.com/user-attachments/assets/ef80d888-7222-484f-bdeb-f10b7d5dd5c1)

  - Min time/Rank:
![Min time](https://github.com/user-attachments/assets/ba72190c-36d0-403d-b976-669263bab88f)

 
  - Max time/Rank:
![Max time](https://github.com/user-attachments/assets/0aac856d-d92c-491f-a2ce-7ad08760c3d4)


Intially time decreased or stabilized as the processors increased for all array sizes and all input types. This shows good scaling because the array is uniformly divided among the processors, which stabilizes the communication and computation time. Sorted, Perturbed and Reverse Sorted patterns show very similar behavior. 
Random data shows the most volatile behavior, especially around 128 processors. It also takes the most time initially because merge sort does more comp and comm due to the nature of data. The reason for this outlier can be caused by the random distribution which creates particularly challenging merge patterns at this scale or load imbalance at this particular processor count.


- #### comm:
  - Avg time/Rank:
![Avg time](https://github.com/user-attachments/assets/56efb437-4e7f-43f9-8045-5adbdcadc3de)

  - Min time/Rank:
![Min time](https://github.com/user-attachments/assets/73c7f7f9-1966-484f-a820-3d41c3d5acef)

  - Max time/Rank:
![Max time](https://github.com/user-attachments/assets/8096e60c-e080-4b24-8d72-b2154d69b8b1)



For all communication patterns, avg increase as the input size increases, but then becomes stable. This is expected as the algorithm has more work to do with larger datasets. So more data to scatter, gather, and wait for due to MPI barrier. This consistency makes sense because the communication costs are dependent on data size, not data arrangement. The code shows the same communication pattern (Scatter/Gather) regardless of input type.
Relatively stable scaling up to 64 processors with a minor increase in communication time as processor count grows
Outlier for random data is likely caused by network congestion or load imbalance when distributing random data.

- #### comp:
  - Avg time/Rank:
 ![Avg time](https://github.com/user-attachments/assets/f90ca4dc-57c0-4078-8e3e-2264f4ac374b)


  - Min time/Rank:
![Min time](https://github.com/user-attachments/assets/f5006209-f59a-4f71-8314-684edde197a1)


  - Max time/Rank:
![Max time](https://github.com/user-attachments/assets/206d3b8e-ee7d-46d6-9cd0-6bc509cf9207)


All four input types show a clear trend of decreasing avg computation time as the number of processors increases
This is because the mergeSort function is being parallelized, with each processor handling a smaller subset (size = n/world_size) of the data. That is why 2^28 with 2 processors shows the highest time because that data is divided on only 2 processors. As the number of processors increases, the data is being uniformly divided into more processors so less time spent on computation for the smaller chunk.
The computation time curves flatten out after around 64-128 processors, this suggests that each processor's local mergeSort operation becomes too small to benefit from further division.

- #### Data generation:
  - Avg time/Rank:
![Avg time](https://github.com/user-attachments/assets/fb752d69-7a37-4ce6-aeaa-c50f7aa8f7c0)


- #### Correctness check:
  - Avg time/Rank:
![Avg time](https://github.com/user-attachments/assets/e6a96aeb-1847-4ba3-b56f-3c71a582917c)


Data initialization and correctness check time remain stable and show consistent performance across processor counts and array sizes for all input types. They show similar patterns
The root process (rank 0) handles all data generation (generate_array function) and traversing and checking if the array is sorted. Both of them are sequential, so O(n) time.
There is an outlier for random and sorted due to the reason described above. This sequential process is the main bottleneck for the calculation time.

### II) Sample Sort
- #### main:
  - Avg time/Rank:

![Screenshot 2024-10-21 at 22-38-40 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/bcbae6b1-99c6-4009-bebe-e93a80b2fb62)

  - Min time/Rank:
 ![Screenshot 2024-10-21 at 22-41-27 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/fa990528-2f00-4884-a27c-4baf26bb3d62)

  - Max time/Rank:
![Screenshot 2024-10-21 at 22-44-12 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/2e1f4a6c-6385-4d61-b8cc-1b4f0ae35832)

  - Variance time/Rank:
![Screenshot 2024-10-21 at 22-45-06 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/2281fa08-dd0f-478a-b681-bd3cc3f0defc)

  - Total time:
 ![Screenshot 2024-10-21 at 22-31-18 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/57ab822f-8467-4b6c-9d8e-150e02f8c8f1)


  Overall, for main as the input size increases, the longer sample sort takes to sort the array across all input types as shown by the higher input size dots are higher than the lower input size dots on average time per processor. 
  before 64 processors, runtime descrases as the number of processors increase likely due to the data getting separated into more buckets and sorted quicker locally. 
  After 64 processors, runtime increases slightly as the number of processors increase likely due to increases in communication overhead (MPI_Alltoall, MPI_Gather).

  - #### comm:
  - Avg time/Rank:

![Screenshot 2024-10-21 at 22-37-50 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/62d6c535-42b2-49fa-8e0b-525bb10ca0a4)

  - Min time/Rank:
![Screenshot 2024-10-21 at 22-40-39 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/88f36ede-8f0e-4a74-aac8-8932a2ed7834)

  - Max time/Rank:
![Screenshot 2024-10-21 at 22-43-22 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/a20f9db7-d90a-4679-9ac1-62ce2ba1affc)

  - Variance time/Rank:
![Screenshot 2024-10-21 at 22-45-06 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/aa023bea-38ca-440c-b621-60088c983eee)

   - Total time:
![Screenshot 2024-10-21 at 22-32-00 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/7603ea2c-c880-496d-a918-353ab298ad4e)

  Similarly with main, comm time decreases as the number processors increases up until 64 processors and communication time increases after 64 processors on average per processor. Total time increaes exponentially after 64 processors, likely affecting total time a lot for main. 
  This is likely due to message calls such as MPI_AlltoAll that send and gather from all other processors, in my case the data split by the partitions into their respective final buckets. 


- #### comp:
  - Avg time/Rank:
 
![Screenshot 2024-10-21 at 22-38-07 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/88828b01-b4b3-4bd0-bb2c-164f111411a8)


  - Min time/Rank:
![Screenshot 2024-10-21 at 22-40-59 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/ef0e3f4b-65dc-4119-b033-56128509ff02)


  - Max time/Rank:
    ![Screenshot 2024-10-21 at 22-43-52 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/fe4ea91d-e215-4567-9d75-e894f61a4591)


  - Variance time/Rank:
    ![Screenshot 2024-10-21 at 22-45-27 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/05986ef3-d636-4739-b9b4-ac22950029d8)


  - Total time:
    
 ![Screenshot 2024-10-21 at 22-32-58 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/86906f4e-91c3-439e-8bdd-0bc31c747fe1)

  On average, computation time decays exponentailly on average as the number of processors increases for all input types. 
  With the communication time taken out, one can see that operations like computing partition sizes can be done quicker in more parallel. 

- #### Data generation:
  - Avg time/Rank:
    ![Screenshot 2024-10-21 at 22-36-38 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/b9fa5065-9760-449b-836e-6e1d2c5eff2b)


  - Min time/Rank:
    
![Screenshot 2024-10-21 at 22-41-57 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/91effd01-15a6-4c60-b901-fcf6b17a4a74)

  - Max time/Rank:
    
![Screenshot 2024-10-21 at 22-42-31 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/21cb09ab-272f-453d-b14b-ced572bb6082)

  - Variance time/Rank:
![Screenshot 2024-10-21 at 22-46-06 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/55e53b5b-f7b9-49cd-8f5e-98d40e562c0c)

  - Total time:
    

![Screenshot 2024-10-21 at 22-35-23 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/9133df89-0745-494e-882f-6b8db9530c85)


Data generation, much similar to computation, decays in time exponenitally as the number of processors increases on average. 

- #### Correctness check:
  - Avg time/Rank:
    ![Screenshot 2024-10-21 at 22-39-18 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/a2b2d7e6-99ff-4247-be35-30da16a16aca)


  - Min time/Rank:
    ![Screenshot 2024-10-21 at 22-40-09 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/dd33550d-4589-44fa-9555-91b8ae71bb13)


  - Max time/Rank:
    
![Screenshot 2024-10-21 at 22-42-58 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/d2b89b8b-1f38-47f7-830b-e73ef2b3fa36)

  - Variance time/Rank:
    
![Screenshot 2024-10-21 at 22-46-38 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/e9636703-f7cd-4fbb-bf64-d4498fd27d56)

  - Total time:
    ![Screenshot 2024-10-21 at 22-34-29 lab2-analysis_single_trial - Jupyter Notebook](https://github.com/user-attachments/assets/c3f30d9b-9fe4-4cdf-87c6-2b8bccb1f60b)


  Similar to data generation and computation, correctness check decays in time exponenitally as the number of processors increases on average. 

### III) Bitonic Sort
- #### main:
  - Avg time/Rank:
    
![mainavg](https://github.com/user-attachments/assets/89f351bc-c7b4-4f93-bd99-0b3b6b805e43)

  - Min time/Rank:
    
![mainmin](https://github.com/user-attachments/assets/f0078656-c569-479f-836a-911c1e8e6ceb)

  - Max time/Rank:
    
![mainmax](https://github.com/user-attachments/assets/7e2bc67b-be6b-4892-a753-de594540e637)

  - Variance time/Rank:
    
![mainvar](https://github.com/user-attachments/assets/e3ba719c-63d6-41fe-bd3c-9d0f081c3b6f)

  - Total time:
    
![maintotal](https://github.com/user-attachments/assets/70e72651-e3be-4108-9e0c-7dbe7958167e)


Across all input types, the runtime of main was shortest for 1% perturbed and sorted input, and random took the longest to complete. These differences were caused by the extra time added by the local quicksort that was performed on each round. Overall, however, the time spent in main decreased as the number of processors increased. This makes sense since the work was evenly distributed between processors as they sorted their local arrays and made swaps with their partners.
  - #### comm:
  - Avg time/Rank:
    
![commavg](https://github.com/user-attachments/assets/5fb623e6-4968-43ff-b7e4-fd8e5be8ddab)

  - Min time/Rank:
    
![commmin](https://github.com/user-attachments/assets/89704a34-4703-478a-ae7f-aa6c1b2a25d2)

  - Max time/Rank:
    
![commmax](https://github.com/user-attachments/assets/f3a8eed4-640a-4355-a7b2-7e329bd575b8)

  - Variance time/Rank:
    
![commvar](https://github.com/user-attachments/assets/18035438-3097-4c49-bc60-1c8a8e1ae3ff)

   - Total time:
     
![commtotal](https://github.com/user-attachments/assets/faf96695-f80e-4e8e-b9b4-f93dbd7ec13b)


Generally, the amount of time spent communicating went up as the number of processors went up, which makes sense, however this increase was rather gradual. I believe this was a combination of how large the arrays being sent were with how few communications were made within the bitonic sorting function. More time was spent on communicating between processes due to the large array size being sent, because of how I implemented this algorithm. However, I did use a minimal amount of communication while sorting was happening. There were several outliers in these graphs, as can be seen in the variance graph. 


- #### comp:
  - Avg time/Rank:
    
![compavg](https://github.com/user-attachments/assets/c3fe2de0-ceb7-4ed1-8e6b-5321708d4076)

  - Min time/Rank:
    
![compmin](https://github.com/user-attachments/assets/412502c6-d84b-4ca0-8ebb-6fef4e145392)

  - Max time/Rank:
    
![compmax](https://github.com/user-attachments/assets/83d23b83-fb0e-4c55-970c-2cb15102a7e6)

  - Variance time/Rank:
    
![compvar](https://github.com/user-attachments/assets/10160a45-a8e7-4631-818e-eac339e8bfec)

  - Total time:
    
![comptotal](https://github.com/user-attachments/assets/74d20b6b-ae9a-4ae3-9915-0ffe59b167da)


  The average computation time decreased as the number of processors increased, which lines up with what we were expecting for bitonic sort. As the number of processors increases, each gets a smaller local array to swap elements with its partner. This makes it more efficient to sort locally and perform the appropriate swaps. The overall time spent on computation was increasing since there were more rounds made, and more arrays were copied, sorted, and swapped between processors.

- #### Data generation:
  - Avg time/Rank:
    
![datagen](https://github.com/user-attachments/assets/d37bca47-82bb-4383-8b7c-f86525459d64)

- #### Correctness check:
  - Avg time/Rank:
    
![correctnesscheck](https://github.com/user-attachments/assets/693a8412-9dbd-43ed-b30a-7bd4d7d04934)

These were our main bottlenecks, since these sections of code were not parallelized. As you can see, all input sizes have a constant time across all processor counts, which is what we were expecting.


### IV) Radix Sort
- #### main:
  - Avg time/Rank:
    ![Radix_Sort_Performance_Avg_time_Sorted_Input_main](https://github.com/user-attachments/assets/7d01688f-28a5-48e2-851a-e148de30ef0c)


  - Min time/Rank:
  ![Radix_Sort_Performance_Min_time_Sorted_Input_main](https://github.com/user-attachments/assets/e959818d-9dbc-4c66-8aa3-de658342cc76)



  - Max time/Rank:
  ![Radix_Sort_Performance_Max_time_Sorted_Input_main](https://github.com/user-attachments/assets/fdbbe2a5-af4a-4117-919f-aaf1af8a17df)



  - Variance time/Rank:
  ![Radix_Sort_Performance_Variance_Sorted_Input_main](https://github.com/user-attachments/assets/e6270fda-4609-4bbd-9a1f-7086d4699051)



  - Total time/Rank:
  ![Radix_Sort_Performance_Total_time_Sorted_Input_main](https://github.com/user-attachments/assets/9c2868f2-5852-4058-bdbf-825d1cffb0c9)


The total times increase as both the input size increases and the proceses increase. This is due to several reasons. The first being that due to more data being present, more time must be spent sending and receiving the data to and from procesess meaning a higher communication overhead. Further, more data means a higher total computation time. In addition, more processes means that data needs to be sent to and from more locations resulting in longer runtimes. These observations are evident in the figures above. 

- #### comm:
  - Avg time/Rank:
  ![Radix_Sort_Performance_Avg_time_Sorted_Input_comm](https://github.com/user-attachments/assets/a81c8b18-7d4a-489b-8315-63012c19e295)



  - Min time/Rank:
  ![Radix_Sort_Performance_Min_time_Sorted_Input_comm](https://github.com/user-attachments/assets/28e0891b-1841-4bf8-a9a6-442fea0c8e2d)



  - Max time/Rank:
  ![Max time for correct](https://cdn.discordapp.com/attachments/1298468749887672370/1298468870654267442/Radix_Sort_Performance_Max_time_Sorted_Input_comm.png?ex=6719acdf&is=67185b5f&hm=901ac14dea8834c1de17932479a58fee7e20dc8b4afe67b0a93ab12de31b7ac6&)



  - Variance time/Rank:
  ![Radix_Sort_Performance_Variance_Sorted_Input_comm](https://github.com/user-attachments/assets/3e329bcb-f6f1-4ed1-b7e7-5d13d21e09f9)



  - Total time/Rank:
  ![Radix_Sort_Performance_Total_time_Sorted_Input_comm](https://github.com/user-attachments/assets/616bd505-e790-46e1-94db-e511e0b36010)


The communication times increase as the input size increases. Further, more processes also cause an increase in time. This is due to several reasons. The first being that due to more data being present, more time must be spent sending and receiving the data to and from procesess. In addition, more processes means that data needs to be sent to and from more locations resulting in longer runtimes as evident in the figures above. 

- #### comp
  - Avg time/Rank:
  ![Radix_Sort_Performance_Avg_time_Sorted_Input_comp](https://github.com/user-attachments/assets/3eba957b-84bd-4bda-80a3-e2ba9b5e8724)



  - Min time/Rank:
  ![Radix_Sort_Performance_Min_time_Sorted_Input_comp](https://github.com/user-attachments/assets/44e5b141-21c4-4dd6-bf12-d31d799f17c2)



  - Max time/Rank:
  ![Radix_Sort_Performance_Max_time_Sorted_Input_comp](https://github.com/user-attachments/assets/049a0709-eb87-4dc8-8748-a82e34a3aefc)



  - Variance time/Rank:
  ![Radix_Sort_Performance_Variance_Sorted_Input_comp](https://github.com/user-attachments/assets/015b2283-ee82-4cf5-81da-f22cc96fc9d6)



  - Total time/Rank:
  ![Radix_Sort_Performance_Total_time_Sorted_Input_comp](https://github.com/user-attachments/assets/0890e4b2-7fff-451f-b867-f7cebd637978)


On average the computation time should be decreasing however upon analyzing the total times for the computation this is not necessarily the case. There could be several reasons. One is the fact that our data generation function when provided with a large number also generates numbers that have a large number of digits. Therefore, since the runtime of the radix sort is dependent on the number of digits could end up increasing the computation time. The implementation might also be flawed. I realized that the algorithm has one small portion that is not parallelized, which needs to be changed before our final presentation. The high variance can be explained by the fact that different generation can lead to much different sets of data leading to differing performance rates. 

- #### Data Generation
  ![Radix_Sort_Performance_Avg_time_Sorted_Input_data](https://github.com/user-attachments/assets/f5406d97-f712-4e0f-85c5-24b7aa05df7d)



  - Min time/Rank:
  ![Radix_Sort_Performance_Min_time_Sorted_Input_data](https://github.com/user-attachments/assets/d2374d76-2271-41a4-b261-94330a899eb7)



  - Max time/Rank:
  ![Radix_Sort_Performance_Max_time_Sorted_Input_data](https://github.com/user-attachments/assets/23ca8090-ca04-4cfe-9966-9babd3070e0d)



  - Variance time/Rank:
  ![Radix_Sort_Performance_Variance_Sorted_Input_data](https://github.com/user-attachments/assets/c82cd1bd-d3ef-4e57-8420-9d7bbb4eba6e)



  - Total time/Rank:
  ![Radix_Sort_Performance_Total_time_Sorted_Input_data](https://github.com/user-attachments/assets/47abab63-c65b-4e76-b6b8-96d660c65acc)


As the total amount of data increases it will take longer to generate the data needed for our sort this increased time can be seen above. There is little variance because the same function to generate our data is used on every iteration.

- #### Correctness Check:
  ![Radix_Sort_Performance_Avg_time_Sorted_Input_correct](https://github.com/user-attachments/assets/94e21e09-f204-4765-ad18-0c7381ff4e8d)



  - Min time/Rank:
  ![Radix_Sort_Performance_Min_time_Sorted_Input_correct](https://github.com/user-attachments/assets/25c09694-80d8-418c-bf72-11903dfad18e)



  - Max time/Rank:
  ![Radix_Sort_Performance_Max_time_Sorted_Input_correct](https://github.com/user-attachments/assets/31a8d861-dfe9-4de1-8a01-ca02747f85da)



  - Variance time/Rank:
  ![Radix_Sort_Performance_Variance_Sorted_Input_correct](https://github.com/user-attachments/assets/3e497431-f7aa-4daf-90f6-e4aa897563a2)



  - Total time/Rank:
  ![Radix_Sort_Performance_Avg_time_Sorted_Input_correct](https://github.com/user-attachments/assets/4f0596d4-28c4-4186-a11b-2d7ba994845d)


As the total amount of data increases it will take longer to check if our output is correctly sorted as supported by the graphs above. There is little variance because the same function to check that our data sorted is used on every iteration. I was not able to obtain every sample size and process count for the data check as Grace's scheudler was over crowded leading to queues so long that no jobs could be executed. I was assured that this would not negatively affect us.


## 5. Presentation
Plots for the presentation should be as follows:
- For each implementation:
    - For each of comp_large, comm, and main:
        - Strong scaling plots for each input_size with lines for input_type (7 plots - 4 lines each)
        - Strong scaling speedup plot for each input_type (4 plots)
        - Weak scaling plots for each input_type (4 plots)

Analyze these plots and choose a subset to present and explain in your presentation.

### I) Strong Scaling

- **2^16**
  - **Main**
    ![2^16 - Main](https://github.com/user-attachments/assets/55151416-480f-4cb6-953e-9f4bf3b5f870)
  - **comp_large**
    ![2^16 - comp_large](https://github.com/user-attachments/assets/8de30d2c-b061-4b8c-b9cc-3601c8a913d4)
  - **comm**
    ![2^16 - comm](https://github.com/user-attachments/assets/efbd420a-1efe-40a7-a74d-3e6b4d290c89)

- **2^18**
  - **Main**
    ![2^18 - Main](https://github.com/user-attachments/assets/443ff9b4-4a91-4ebf-8607-b1fa22e4c087)
  - **comp_large**
    ![2^18 - comp_large](https://github.com/user-attachments/assets/6c64eb15-c81e-4b4b-86ae-bc230d3337e3)
  - **comm**
    ![2^18 - comm](https://github.com/user-attachments/assets/3990cc1e-af7d-4386-9e40-d4e67c46b1df)

- **2^20**
  - **Main**
    ![2^20 - Main](https://github.com/user-attachments/assets/a204255c-5c84-4ca0-a545-feba8bc41825)
  - **comp_large**
    ![2^20 - comp_large](https://github.com/user-attachments/assets/0a2ea27d-142c-4797-8305-f147649c2b3d)
  - **comm**
    ![2^20 - comm](https://github.com/user-attachments/assets/286d155a-8cbf-4273-9656-e96a6776f311)

- **2^22**
  - **Main**
    ![2^22 - Main](https://github.com/user-attachments/assets/84a32e57-0fa1-4e75-a759-0a2ff8c38943)
  - **comp_large**
    ![2^22 - comp_large](https://github.com/user-attachments/assets/d9215deb-648e-46cf-851e-aed109de8e9b)
  - **comm**
    ![2^22 - comm](https://github.com/user-attachments/assets/754d937b-c354-4865-b4f5-53b8d90b3253)

- **2^24**
  - **Main**
    ![2^24 - Main](https://github.com/user-attachments/assets/e0dfb2c1-6d30-4385-8d5b-6d956f41ca0c)
  - **comp_large**
    ![2^24 - comp_large](https://github.com/user-attachments/assets/99f1a348-3c53-45ec-9b63-7e4c96966d91)
  - **comm**
    ![2^24 - comm](https://github.com/user-attachments/assets/517c64b8-ca47-4f00-ac73-7fa98352f9b7)

- **2^26**
  - **Main**
    ![2^26 - Main](https://github.com/user-attachments/assets/65b23bc3-a38b-4310-8a71-8ee0b4fce03d)
  - **comp_large**
    ![2^26 - comp_large](https://github.com/user-attachments/assets/72f64422-c079-4a93-a4b7-0b787c4f7381)
  - **comm**
    ![2^26 - comm](https://github.com/user-attachments/assets/5665bd20-5cb8-450c-b63a-f87db7c5661e)

- **2^28**
  - **Main**
    ![2^28 - Main](https://github.com/user-attachments/assets/15804c71-ecaa-432d-8a7f-e0b1402599dc)
  - **comp_large**
    ![2^28 - comp_large](https://github.com/user-attachments/assets/457510c4-0a49-41fb-a39a-df0d505c8213)
  - **comm**
    ![2^28 - comm](https://github.com/user-attachments/assets/14821acf-009f-4cb1-aa51-45fcc2c3dafd)


**Merge**: For smaller processor counts, there is a reduction in main time due to parallelization benefits. However, as the processor count reaches a higher range (e.g., 256 or 512), the main time stabilizes. We also see a slight increase in communication time across all input types. This increase can be attributed to communication overheads associated with increased processors outweighing the computation benefits, especially in communication-intensive algorithms. For all array sizes across all input types, computation time decreases as the number of processors increases, showing effective parallelization. However, the decrease is much more significant in the smaller array size (2^16) compared to the larger array size (2^28). This decrease can be caused by data being more uniformly divided into multiple processors. For almost all array sizes, the computation time plateaus after a certain point, likely because each processor's workload is reduced to a minimal portion and further decomposition doesn’t impact comp time significantly.

**Bitonic**: At small input sizes, similar to the other algorithms, Bitonic sort does not benefit from parallelization, instead increasing the overall time spent sorting by introducing unnecessary communication overhead. Once reaching input size 2^22, the overall runtime of main stabilizes at ~2 seconds across all processor counts. Past this point, the runtime of main decreases as the number of processors increases. This suggests that the benefits of parallelization only come in after 2^22 array size, where the workload given to each processor is large enough to balance out the cost of communication time. Communication time, when compared to the other algorithms, appears to remain very stable and does not increase too significantly. Comp_large, on the other hand, appears to increase and then decrease, with its maximum point at 4 processors. This is very odd when compared to the other algorithms, which only decrease as processor count increases. This may be due to a variety of factors, the main one being how the local sorting is completed in qsort. This is the only section that comp_large is timing in Bitonic sort, and that means that the runtime is dependent on how long that function takes to run given the array size given. Since it's always sorting an array composed of 2 sorted arrays, its runtime in this section is fairly consistent across input types. When compared to other algorithms, bitonic sort has the lowest main runtime, a slightly higher computation time, and very low communication time. Bitonic sort performs the best out of all algorithms on large input sizes, and roughly the same as the others at low input sizes. 

**Radix**: Strong scaling involves maintaining the size of the input while increasing the number of processes from 2 to 512. In radix sort, as the number of processes increases, the overall time slightly decreases, indicating some benefit to parallelization. However, as the number of processes grows, particularly with larger input sizes, the total time eventually begins to increase due to several factors. The most significant factor is that increased nodes and processes lead to communication overhead, which becomes a bottleneck since radix sort is inherently communication-intensive. Although computation time generally decreases as processes increase, the communication cost tends to outweigh these gains in the current implementation. In fact, radix seems to on average have the largest communication costs. However, radix shows the best performance among all sorting algorithms tested based on computation time; however, the high communication time in my implementation places radix among the highest in terms of total execution time. It is also worth noting that our data generation may have further slowed radix in comparison to other algorithms, as larger input sizes increase the digit count of each element (e.g., 100 versus 100,000), which impacts radix more than other algorithms. This could also explain the differing time graphs for plots with various data types, such as random versus perturbed, which generate data differently. It is also worth noting that my implemenation struggled to balance data evenly among processes potentially leading to longer runtimes, explainig variance among runs of the same type.

**Sample:** Computation average time has the steepest downward slope for each of the input types. this is likely due to more buckets to sort data locally, which makes the overall sorting faster. For some graphs, such as the perturbed data main avg. time, there is a sharp increase in execution time. This is likely due to running those number of processors on 8 nodes with 8 tasks/node which does not scale well to the higher number of processor counts. Communication grows slightly from 2 to 64 processors on random data, this is likely due to more processors to scatter and gather array elements for both the pivot picking step and sending partitioned data to buckets step. I was able to scale my program to 1024 processors and 2^28 input length, but I ended up with poll() runtime errors when I had a high processors to data ratio, specifically when I had 1024 processors and 2^16 input length. This is likely due to either not enough data to partition and send or a network/hardware issue with grace with running a program with that many processors at once. 

### II) Strong Scaling Speedup

#### Random
![Screenshot 2024-10-30 152203](https://github.com/user-attachments/assets/274a1c40-a5de-4b49-95ec-c1022e7053d5)

![Screenshot 2024-10-30 151911](https://github.com/user-attachments/assets/c0423b1f-fe21-4826-926a-037f9c24e8fd)

![Screenshot 2024-10-30 152234](https://github.com/user-attachments/assets/3ecbb083-270d-4a5a-b3bf-02c17cbf61c9)

    
#### Reverse
![Screenshot 2024-10-30 154142](https://github.com/user-attachments/assets/00fe567d-dd03-47f2-a7e1-f50be1daa157)

![Screenshot 2024-10-30 154059](https://github.com/user-attachments/assets/1972236a-0196-4f63-a56e-2e2ed203e81a)
    
![Screenshot 2024-10-30 153916](https://github.com/user-attachments/assets/93547cde-51e0-47e5-ab2e-de45b75464a4)


#### 1% Perturbed
![Screenshot 2024-10-30 155327](https://github.com/user-attachments/assets/43833a61-cf97-4193-97fd-274c0a3beae7)

![Screenshot 2024-10-30 155452](https://github.com/user-attachments/assets/107aca1b-6b62-4a07-9130-53be49e6a623)

![Screenshot 2024-10-30 155408](https://github.com/user-attachments/assets/b00c9269-f47b-4ad9-a1da-53efa95a1126)


#### Sorted
![Screenshot 2024-10-30 160031](https://github.com/user-attachments/assets/09fad681-4fcd-42dc-bc6a-e402e18d2a61)

![Screenshot 2024-10-30 155915](https://github.com/user-attachments/assets/3e54b5c7-e50b-42cf-a8ee-edc321f43fe6)

  
![Screenshot 2024-10-30 155958](https://github.com/user-attachments/assets/b97b215e-a4ec-4778-b4b0-5363abbd2544)

**Merge**: Random data has the best overall scaling, with good computation speedup but is limited by communication. Reverse sorted has medium performance, with predictable patterns helping efficiency. 1% perturbed has limited speedup but has consistent performance. Finally, sorted has the poorest speedup due to minimal parallel processing benefits. Computation shows the best speedup for random input because it benefits from local sorting, however, performance decreases with more ordered data. Communication seems to be the major bottleneck for overall performance. It increases with processor count and is most impactful in random data. It has a lower overhead for sorted and perturbed data.

**Bitonic:** Bitonic sort showed generally low values for speedup, and this is because the T(1) value (calculated as T(2)*2) was relatively low across all input sizes. The initial time being low limits the speedup value, since speedup is calculated with T(1)/T(P). Ideally, speedup will be increasing as processor count goes up, and this is true for all graphs except [main, 1% perturbed] and [comm, 1% perturbed]. An outlier among these graphs for bitonic sort is [comp, reverse], where the speedup is 120,000 at 512 processors. This very, very high number may be due to the usage of very small decimals in calculating speedup, or some error in calculation. 

**Radix**: Radix sort generally demonstrated consistent and positive speedups across all input types, with computation times exhibiting a gradual, steady increase as the problem size grew. This pattern indicates that the computational component of the algorithm is well-optimized for parallel execution. However, communication times showed considerable variability, largely influenced by factors such as randomness in data generation and the specific type of input data. For example, runs using perturbed data often exhibited high variance in communication times; in some cases, data distributed more evenly among processes, reducing communication needs and resulting in faster overall execution. Conversely, less favorable distributions caused increased communication overhead, impacting speedup. Additionally, speedup differed across input types, with certain inputs aligning better with the parallel structure, thus achieving higher efficiency. This variability highlights areas for potential improvement in both the communication strategy and data generation functions, as optimized data splitting and reduced communication demands could further improve overall performance, ensuring more predictable and stable speedups.

**Sample:** The highest speedup on main is on sorted data. this is likely due to partitioning being the same size for each bucket as the data is already sorted. There is either a sharp  drop after 64 processors for main and communication speedup graphs. This is likely due to running those number of processors on 8 nodes with 8 tasks/node which does not scale well to the higher number of processor counts.

### III) Weak Scaling
#### Merge
  - Main
    
![mergemain](https://github.com/user-attachments/assets/d228eaa0-536e-4a2d-8249-fb9344993ecd)

  - comp_large
    
![mergecomplarge](https://github.com/user-attachments/assets/cc32c15e-1df1-40bf-b56f-e010917774ab)

  - comm
    
![mergecomm](https://github.com/user-attachments/assets/724dc38f-faae-489a-b5d6-c1eb1e302d91)

#### Bitonic
  - Main
    
![bitonicmain](https://github.com/user-attachments/assets/58cf473f-d9fc-4a2a-a507-db987b435bf9)

  - comp_large
    
![bitoniccomplarge](https://github.com/user-attachments/assets/083b6b85-4af9-46cf-bfe7-3ecac477a83d)

  - comm
    
![bitoniccomm](https://github.com/user-attachments/assets/424d6633-13bc-4b2b-84b0-de90a35a0efa)


#### Sample
  - Main
    
![samplemain](https://github.com/user-attachments/assets/db0039f3-50a8-4a9a-9d79-d50453e957e9)

  - comp_large
    
![samplecomplarge](https://github.com/user-attachments/assets/01539efa-0ee6-424b-9105-3e6a9bf33fd4)

  - comm
    
![samplecomm](https://github.com/user-attachments/assets/a0944905-efd2-4ee1-984a-2a716949bd1d)

#### Radix
  - Main
![radixmain](https://github.com/user-attachments/assets/730da29f-cd74-438a-a5d3-da0de95cca1e)

  - comp_large
![radixcomp](https://github.com/user-attachments/assets/e52195bb-9bbe-4c6c-91ff-a4e57d6b5bf8)

  - comm
![radixcomm](https://github.com/user-attachments/assets/04471d1c-82ae-4c18-8cec-7e76519d301f)

*Note, final changes in Radix sort were made to fix bottlenecks, and caliper markers were missing for comp_large. Instead, comp is used for these graphs. The calltree was also changed such that there were duplicate sections of Comm, so the graph shows both of these sections. Despite these factors, it can still be seen that Radix sort is scaling well, when compared to Sample and Merge sort.

**Merge:** The plots show weak scaling where the problem size increases proportionally with the number of processors. This pattern is consistent with all input_types. This shows that this implementation of merge sort has not scaled nicely because ideally, weak scaling should stay constant. This could be caused by increased communication overhead with more processors.
The computation time shows better scaling compared to the overall time. It has lower absolute values on the y-axis (0-3.5) than the main section (0-6). Communication shows the most dramatic scaling issues. This can be because communication volume grows with processor count. Therefore, the communication component appears to be the main bottleneck limiting overall scalability.

**Bitonic:** Since the weak scaling graph is ideally flat, it can be said that this implementation of Bitonic sort does not scale well when processor workload is held constant. For every graph, the average time per processor increases as processor number increases, meaning it is not scaling well. However, it should be noted that the y-axis is dealing with very small numbers, on main, the largest increase in time from 2 processors to 512 is 3 seconds. This relatively low time scale does show that, when compared to the other algorithms, bitonic sort scales the best.
When looking at the time taken by comm and comp_large, comm dominates runtime, meaning that the main bottleneck in this implementation is communication time, which makes sense. There are multiple MPI_Barriers preventing processors from running out-of-sync, and these wait times could be adding unnecessary bloat to runtime for partnered processors who are both ready to compare and exchange data. This is made worse by an increasing processor count, driving up communication time significantly.

**Radix:** Weak scaling graphs should ideally maintain a flat curve indicating that the implemented algorithm scales well. However, it is evident that the implementation of Radix sort is not perfect and has a few scaling issues. This is seen when the majority of the graphs feature an  increase in the average time as the number of processes increase, indicating subpar scaling. However, it is important to realize that the increase in the graphs look exagerated due to the small values beinged examined. In reality the increase is typically fairly small. Viewing the graphs it does appear that the portions of the algorithm most affected by the scaling is the communication. This makes sense as more processes and nodes are introduced the radix sort has to make increasingly complex communications with Alltoall, Igather, and similar functions. It is also important to note that different methods of data generation could have caused some of the poor scaliability. For example, with our data generation a smaller size of 2^16 creates a smaller range of values than using 2^28, digit wise. This means that the larger size not only contains more input elements but also each element in the input has more digits than the smaller input runs, which the Radix sort is required to iterate through. If the data generation simply created more inputs with the same range of values the algorithm would have scaled better since the same number of digits would need to be processed. Overall radix sort scaled better than both the sample and merge sort and was similar to bitonic.  

**Sample:** The weak scaling charts shows a general exponential increase with the increase of number of processors and average time for each of the input types. The sharpest increase is with 1% perturbed, likely because the perturbed data on the larger data size had a bigger impact with the higher number of processors compared to smaller data sizes with smaller number of processors. computation large contributes very little to the main time compared to communication. This is likely due to me only setting comp_large markers for the partitioning state of my algorithm which does not account for local sorting. 

## 6. Final Report
Submit a zip named `TeamX.zip` where `X` is your team number. The zip should contain the following files:
- Algorithms: Directory of source code of your algorithms.
- Data: All `.cali` files used to generate the plots seperated by algorithm/implementation.
- Jupyter notebook: The Jupyter notebook(s) used to generate the plots for the report.
- Report.md

