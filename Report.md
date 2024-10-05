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

// BITONIC SORT
// dimensions = log2(num_proc)
// For i = 0 to dimensions - 1:
    // For j = i down to 0:
        if (i + 1)st bit of rank == jth bit of rank then
            COMP EXCHANGE MAX (j)
        else
            COMP EXCHANGE MIN (j)


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
// MPI_Send MAX of array to partner (A)
// MPI_Recv MIN from partner (B)

// copy all values in array larger than MIN to send_buffer

// MPI_Send send_buffer array to partner (C)
// MPI_Recv array from partner, store in receive_buffer (D)

// store smallest value from receive_buffer at end of array
// sort array

// free buffers
// RETURN
// END COMP EXCHANGE MIN
////////////////////

////////////////////
// COMP EXCHANGE MAX (j)
// partner process = rank XOR (1 << j)
// MPI_Recv MAX from partner (A)
// MPI_Send MIN of array to partner (B)

// copy all values in array smaller than MAX to send_buffer

// MPI_Recv array from partner, store in receive_buffer (C)
// MPI_Send send_buffer array to partner (D)

// store largest value from receive_buffer at start of array
// sort array

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
