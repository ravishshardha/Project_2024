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
- Radix Sort: The Radix sort is a non-comparative integer sorting algorithm that iterates through each digit of the given elements starting from the least significant digit and progressing to the most significant digit. As the algorithm iterates the temporary results are placed in a "bucket" based on the current digit. Once entirely iterated the elements are sorted. Through the MPI library a parallel implementaion can be achieved by splitting up master data into chunks then sending them to workers. The workers will sort the data and send it back to the master, resulting in sorted data.

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
