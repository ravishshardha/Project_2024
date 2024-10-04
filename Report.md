# CSCE 435 Group project

## 0. Group number: 

## 1. Group members:
1. First
2. Second
3. Third
4. Fourth

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort: Brack Harmon
- Merge Sort:
- Radix Sort:

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes


Sample Sort:
    // Starting MPI commands 
    MPI_Init
    MPI_Comm_rank
    MPI_Comm_world

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
   
    // Step 3: Gather all the samples at rank 0 
	call MPI_Gather with sample data to fill gathered_samples
	

    // Step 4: Rank 0 sorts the samples and selects pivots
	if(rank == 0)
		sort gathered_samples
		for each sample
			pivots[i] = sample from gathered_samples
	

    // Step 5: Broadcast the pivots to all processes 
	call MPI_Bcast with pivots and num_samples

    // Step 6: Partition the local data based on the pivots
	
	create send_counts, send_offsets, and partitioned_data arrays
	populate partitioned_data with info from data based on offsets and send counts
	
    // Step 7: Send partitioned data to respective processors 
	use MPI_Alltoall with send_counts and recv_counts
	create recv_data array and populate with received information using MPI_Alltoallv
    
    // Step 8: Sort the received data locally
	sort recv_data

    // Here is where we end timing the Samplesort algorithm
	
    // print sorted data (optional)

    // Ending MPI commands (MPI_Finalize)




### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
