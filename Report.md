# CSCE 435 Group project

## 0. Group number: 13

## 1. Group members:
1. Ravish Shardha
2. Lydia Harding
3. Brack Harmon
4. Jack Hoppe

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort: Lydia Harding
    - Parallel sorting algorithm that repeatedly sorts segments of a sequence of numbers into bitonic sequences. A bitonic sequence consists of a list of numbers which is first increasing, then decreasing. In the parallel version, for each i round, sorting is done with the partner that differs in the ith bit, alternating between collecting the lower half of all elements, and collecting the higher half of all elements, then sorting in order. Once all rounds have completed, the full array is sorted.
    - Bitonic sort only works on input arrays of size 2^n.
    - Bitonic sort will make the same number of comparisons for any input array, with a complexity of O(log^2n), where n is the number of elements to be sorted.
- Sample Sort:
- Merge Sort:
- Radix Sort:

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

Bitonic Sort:

```
////////////////////
// MAIN
// Initialize MPI, get num_proc and rank

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

// Finalize MPI
// END MAIN

////////////////////
// HELPER FUNCTIONS

////////////////////
// COMP EXCHANGE MIN (j)
// partner process = rank XOR (1 << j)
// Send MAX of array to partner (A)
// Receive MIN from partner (B)

// copy all values in array larger than MIN to send_buffer

// Send send_buffer array to partner (C)
// Receive array from partner, store in receive_buffer (D)

// store smallest value from receive_buffer at end of array
// sort array

// free buffers
// END COMP EXCHANGE MIN
////////////////////

////////////////////
// COMP EXCHANGE MAX (j)
// partner process = rank XOR (1 << j)
// Receive MAX from partner (A)
// Send MIN of array to partner (B)

// copy all values in array smaller than MAX to send_buffer

// Receive array from partner, store in receive_buffer (C)
// Send send_buffer array to partner (D)

// store largest value from receive_buffer at start of array
// sort array

// free buffers
// END COMP EXCHANGE MAX
////////////////////
```

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
