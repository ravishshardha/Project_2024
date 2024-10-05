# CSCE 435 Group project

## 0. Group number: 13

## 1. Group members:
1. Ravish Shardha
2. Lydia Harding
3. Brack Harmon
4. Jack Hoppe

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort:
- Radix Sort: 
    - The Radix sort is a non-comparative integer sorting algorithm that iterates through each digit of the given elements starting from the least significant digit and progressing to the most significant digit. As the algorithm iterates the temporary results are placed in a "bucket" using an algorithm like the "count sort". These buckets are used to create the new ordering of the elements until all digits have been processed. 
    - Through the MPI library and utilizing Grace a parallel implementation can be achieved by splitting input data into chunks then distributing them to workers. The workers will sort its chunk of data based on the current digit. After sorting, the bucket data from the count sort is redistributed across the processes. This continues until the most significant digit is reached resulting in sorted data.
    - The Radix sort requires integers or data that can be represented with integers and a fixed range of digits, i.e., (0-9)
    - Larger integers cause more passes and slows the algorithm
    - Uneven distributions of input data can lead to inefficiencies due to some buckets becoming disproportionately large.
    - Runtime: O(d*n) where d = number of digits and n = number of elements


### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

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
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
