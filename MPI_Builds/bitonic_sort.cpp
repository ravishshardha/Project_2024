// Name: Lydia Harding
// BITONIC SORT

// References: https://github.com/adrianlee/mpi-bitonic-sort/tree/master
// https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf
// https://github.com/francesco-biscaccia-carrara/BitonicSort/blob/main/parallel_test.c
// Help from Ravish Shardha (groupmate) for input generation & sort verification.
// Include necessary packages for MPI, adiak, caliper, etc.
#include <iostream>      // Printf
#include <math.h>       // Logarithm
#include <stdlib.h>     // Malloc
#include "mpi.h"        // MPI Library
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <random>
#include <string.h>

void Comp_exchange_Low(int bit);
void Comp_exchange_High(int bit);
int ComparisonFunc(const void * a, const void * b);
int is_sorted(int *, int);
void generate_array(int*, int, const char*);
int comparator(const void* a, const void* b);

// DEFS
#define MASTER 0;

/********** Define Caliper ***************/
const char* data_init_runtime = "data_init_runtime";
const char* correctness_check = "correctness_check";
const char* comm  = "comm";
const char* comm_small = "comm_small";
const char* comm_large = "comm_large";
const char* comp  = "comp";
const char* comp_small = "comp_small";
const char* comp_large = "comp_large";

// GLOBAL VARS
int rank;
int num_proc;
int * array;
int array_size;

int main(int argc, char * argv[]){
  CALI_CXX_MARK_FUNCTION;
  cali::ConfigManager mgr;
  mgr.start();

  // Check if command was run with proper vars
  if (argc < 3) {
    printf("Not enough arguments, stopping...");
    return 0;
  }

  // MPI Initialization & vars
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Generate the input array on each processor (rand, in-order, reverse-order, or perturbed)
  int n = atoi(argv[1]);
  const char* input_type = argv[2];
  
  /********** Create and process the array based on input type **********/
  // From Ravish's Mergesort Code
    int *original_array = NULL;

    // Only the root process creates and initializes the array
    if (rank == 0) {
        original_array = (int *)malloc(n * sizeof(int));
        
        CALI_MARK_BEGIN(data_init_runtime);
        generate_array(original_array, n, input_type);
        CALI_MARK_END(data_init_runtime);
        
        // Print unsorted array (first 10 elements)
        // printf("Unsorted array for %s: ", input_type);
        // for (int j = 0; j < n; j++) {
        //     printf("%d ", original_array[j]);
        // }
        // printf("\n");
    }

  /********** Set Adiak Values (moved inside main) **********/
    adiak::value("algorithm", "bitonic");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", n);
    adiak::value("input_type", input_type);
    adiak::value("num_procs", num_proc);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 13);
    adiak::value("implementation_source", "online and handwritten");

  /********** Send each subarray to each process **********/
  array_size = n / num_proc;
  array = (int *)malloc(array_size * sizeof(int));
  CALI_MARK_BEGIN(comm);
  CALI_MARK_BEGIN(comm_large);
  MPI_Scatter(original_array, array_size, MPI_INT, array, array_size, MPI_INT, 0, MPI_COMM_WORLD);
  CALI_MARK_END(comm_large);
  MPI_Barrier(MPI_COMM_WORLD);
  CALI_MARK_END(comm);


  // sort the local array before starting
  qsort(array, array_size, sizeof(int), comparator);

  // BITONIC SORT
  int i, j;
  int dimensions = (int)(log2(num_proc));

  for (i = 0; i < dimensions; i++){
      for (j = i; j >= 0; j--){
          if (((rank >> (i + 1)) % 2 == 0 && (rank >> j) % 2 == 0) || 
              ((rank >> (i + 1)) % 2 != 0 && (rank >> j) % 2 != 0)) {
            Comp_exchange_Low(j);
          }
          else {
            Comp_exchange_High(j);
          }
      CALI_MARK_BEGIN(comm);
      MPI_Barrier(MPI_COMM_WORLD);
      CALI_MARK_END(comm);
      }
  }

  CALI_MARK_BEGIN(comm);
  MPI_Barrier(MPI_COMM_WORLD);
  CALI_MARK_END(comm);

  // VERIFY SORT

  /********** Gather the sorted subarrays into one **********/
  int *sorted = NULL;
  if (rank == 0) {
      sorted = (int *)malloc(n * sizeof(int));
  }

  CALI_MARK_BEGIN(comm);
  CALI_MARK_BEGIN(comm_large);
  MPI_Gather(array, array_size, MPI_INT, sorted, array_size, MPI_INT, 0, MPI_COMM_WORLD);
  CALI_MARK_END(comm_large);
  CALI_MARK_END(comm);

/********** Check if sorted **********/
    if (rank == 0) {
      CALI_MARK_BEGIN(correctness_check);
      if (is_sorted(sorted, n)) {
          printf("Array sorted correctly for %s.\n", input_type);
      } else {
          printf("Array NOT sorted correctly for %s.\n", input_type);
      }
      CALI_MARK_END(correctness_check);

      // Print sorted array (first 10 elements)
      // printf("Sorted array for %s: ", input_type);
      // for (int j = 0; j < n; j++) {
      //     printf("%d ", sorted[j]);
      // }
      // printf("\n");

      /********** Clean up root **********/
      free(sorted);
      free(original_array);
    }

  // Cleanup & finalize mpi
  free(array);
  mgr.stop();
  mgr.flush();
  MPI_Finalize();
  return 0;
}

////////////////////
// HELPER FUNCTIONS

///////////////////
// GENERATE ARRAY
// From Ravish's Mergesort code
void generate_array(int *array, int size, const char* input_type) {
    srand(time(NULL));

    // if input type is NULL, default to random
    if (input_type == NULL){
      input_type = "Random";
    }

    if (strcmp(input_type, "Random") == 0) {
        // Random array
        for (int i = 0; i < size; i++) {
            array[i] = rand() % size;
        }
    } else if (strcmp(input_type, "ReverseSorted") == 0) {
        // Reverse sorted array
        int max_val = size * 2;  // Start with a larger value than size
        for (int i = 0; i < size; i++) {
            int decrement = rand() % (size / 5 + 1);  // Sometimes allows 0 decrement, creating repeated values
            max_val -= decrement;
            array[i] = max_val;
        }
    } else if (strcmp(input_type, "Sorted") == 0) {
        // Sorted array with random values, allowing for repeats
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);  // Sometimes allows 0 increment, creating repeated values
            min_val += increment;
            array[i] = min_val;
        }
    } else if (strcmp(input_type, "1_perc_perturbed") == 0) {
        // 1% Permuted array
        int min_val = 0;
        for (int i = 0; i < size; i++) {
            int increment = rand() % (size / 5 + 1);  // Sometimes allows 0 increment
            min_val += increment;
            array[i] = min_val;
        }

        // Permute 1% of the array
        int permuted_count = size / 100;
        for (int i = 0; i < permuted_count; i++) {
            int index1 = rand() % size;
            int index2 = rand() % size;
            int temp = array[index1];
            array[index1] = array[index2];
            array[index2] = temp;
        }
    }
}

////////////////////
// IS SORTED
// From Ravish's mergesort code
int is_sorted(int *a, int s) {
    for (int i = 0; i < s - 1; i++) {
        if (a[i] > a[i + 1]) {
            return 0; // Array is not sorted
        }
    }
    return 1; // Array is sorted
}

int comparator(const void* a, const void* b){
  return ( * (int * )a - * (int *)b);
}

////////////////////
// COMP EXCHANGE MIN (j)
void Comp_exchange_Low(int j){
  int partner = rank ^ (1 << j);
  int i;
  int status = 0;
  int * buffer_recieve = (int*)malloc((array_size) * sizeof(int));
  
  CALI_MARK_BEGIN(comm);
  CALI_MARK_BEGIN(comm_small);
  status = MPI_Sendrecv(
        array, 
        array_size, 
        MPI_INT, 
        partner, 
        0,
        buffer_recieve, 
        array_size, 
        MPI_INT, 
        partner, 
        0,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
  CALI_MARK_END(comm_small);
  MPI_Barrier(MPI_COMM_WORLD);
  CALI_MARK_END(comm);

  if (status != 0){
    printf("MIN Failed to sendrecv. [%d, %d]", rank, partner);
    status = 0;
  }

  // Concatenate lists into temp
  CALI_MARK_BEGIN(comp);
  CALI_MARK_BEGIN(comp_small);
  int * temp = (int*)malloc((array_size) * sizeof(int) * 2);
  for (i = 0; i < array_size; i++){
    temp[i] = array[i];
  }
  for (i = 0; i < array_size; i++){
    temp[array_size + i] = buffer_recieve[i];
  }
  CALI_MARK_END(comp_small);

  // Sort locally
  CALI_MARK_BEGIN(comp_large);
  qsort(temp, array_size * 2, sizeof(int), comparator);
  CALI_MARK_END(comp_large);

  // Assign array to be the lower half of the given values
  CALI_MARK_BEGIN(comp_small);
  for (i = 0; i < array_size; i++){
    array[i] = temp[i];
  }
  CALI_MARK_END(comp_small);
  CALI_MARK_END(comp);

  // free buffers
  free(buffer_recieve);
  free(temp);

  return;
}
// END COMP EXCHANGE MIN
////////////////////

////////////////////
// COMP EXCHANGE MAX (j)
void Comp_exchange_High(int j){
  int partner = rank ^ (1 << j);
  int i;
  int status = 0;
  int * buffer_recieve = (int*)malloc((array_size) * sizeof(int));
  
  CALI_MARK_BEGIN(comm);
  CALI_MARK_BEGIN(comm_small);
  status = MPI_Sendrecv(
        array, 
        array_size, 
        MPI_INT, 
        partner, 
        0,
        buffer_recieve, 
        array_size, 
        MPI_INT, 
        partner, 
        0,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
  CALI_MARK_END(comm_small);
  MPI_Barrier(MPI_COMM_WORLD);
  CALI_MARK_END(comm);

  if (status != 0){
    printf("MAX Failed to sendrecv. [%d, %d]", rank, partner);
    status = 0;
  }

  // Concatenate lists into temp
  CALI_MARK_BEGIN(comp);
  CALI_MARK_BEGIN(comp_small);
  int * temp = (int*)malloc((array_size) * sizeof(int) * 2);
  for (i = 0; i < array_size; i++){
    temp[i] = array[i];
  }
  for (i = 0; i < array_size; i++){
    temp[array_size + i] = buffer_recieve[i];
  }
  CALI_MARK_END(comp_small);

  // Sort locally
  CALI_MARK_BEGIN(comp_large);
  qsort(temp, array_size * 2, sizeof(int), comparator);
  CALI_MARK_END(comp_large);

  // Assign array to be the higher half of the given values
  CALI_MARK_BEGIN(comp_small);
  for (i = 0; i < array_size; i++){
    array[i] = temp[i + array_size];
  }
  CALI_MARK_END(comp_small);
  CALI_MARK_END(comp);
  
  // free buffers
  free(buffer_recieve);
  free(temp);

  return;
}
// END COMP EXCHANGE MAX
////////////////////

