// Name: Lydia Harding
// BITONIC SORT

// References: https://github.com/adrianlee/mpi-bitonic-sort/tree/master
// https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf
// Include necessary packages for MPI, adiak, caliper, etc.
#include <iostream>      // Printf
#include <math.h>       // Logarithm
#include <stdlib.h>     // Malloc
#include "mpi.h"        // MPI Library
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

void Comp_exchange_Low(int bit);
void Comp_exchange_High(int bit);
int ComparisonFunc(const void * a, const void * b);

// DEFS
#define MASTER 0;

// GLOBAL VARS
int rank;
int num_proc;
int * array;
int array_size;

int main(int argc, char * argv[]){
  // MPI_Init, 
  MPI_Init(&argc, &argv);

  // MPI_Comm_size(num_procs)
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

  // MPI_Comm_rank(rank)
  MPI_Comm_rank(MPI_COMM_WORLD, & rank);

  // Generate the input array on each processor (rand, in-order, reverse-order, or perturbed)
  // TODO

  MPI_Barrier(MPI_COMM_WORLD);

  // BITONIC SORT
  int i, j;
  // dimensions = log2(num_proc)
  int dimensions = (int)(log2(num_proc));

  // For i = 0 to dimensions - 1:
  for (i = 0; i < dimensions; i++){
      // For j = i down to 0:
      for (j = i; j >= 0; j--){
          //if (i + 1)st bit of rank == jth bit of rank then
          if ((((rank >> (i+1)) % 2 == 0) && (rank >> j) % 2 == 0) || 
              (((rank >> (i+1)) % 2 != 0) && (rank >> j) % 2 != 0)) {
            // COMP EXCHANGE MIN (j)
            Comp_exchange_Low(j);
          }
          else {
            // COMP EXCHANGE MAX (j)
            Comp_exchange_High(j);
          }
      }
  }


  // MPI_Barrier to ensure all sorting is complete
  MPI_Barrier(MPI_COMM_WORLD);

  // VERIFY SORT
  // Check if array is sorted locally
  // Check if array end is less than start of neighbor process's array
  // If both true for all processors, array is sorted.
  // TODO

  // free array 
  free(array);

  // MPI Finalize
  MPI_Finalize();
  // RETURN
  return 0;
}

////////////////////
// HELPER FUNCTIONS

////////////////////
// COMP EXCHANGE MIN (j)
Comp_exchange_Low(){
  // partner process = rank XOR (1 << j)
  int partner = rank ^ (1 << j);

  // MPI_Send MAX of array to partner (A)
  // Max is stored at end of array, assumed to be in order
  MPI_Send(&array[array_size - 1], 1, MPI_INT, partner, 0, MPI_COMM_WORLD);

  // MPI_Recv MIN from partner (B)
  int min;
  MPI_Recv(&min, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  // copy all values in array larger than MIN to send_buffer
  int i;
  int * send_buffer = malloc((array_size + 1) * sizeof(int));
  int send_buf_size = 0;
  for (i = 0; i < array_size; i++){
    if (array[i] > min){
      send_buffer[send_buf_size + 1] = array[i];
      send_buf_size++;
    }
    else{
      break;
    }
  }

  // MPI_Send send_buffer array to partner (C)
  MPI_Send(send_buffer, send_buf_size, MPI_INT, partner, 0, MPI_COMM_WORLD);

  // MPI_Recv array from partner, store in receive_buffer (D)
  int * receive_buffer = malloc((array_size + 1) * sizeof(int));
  MPI_Recv(receive_buffer, array_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  // store smallest value from receive_buffer at end of array
  for (i = 1; i < receive_buffer[0] + 1; i++){
    if (array[array_size - 1] < receive_buffer[i]){
      array[array_size - 1] = receive_buffer[i];
    }
    else {
      break;
    }
  }

  // sort array
  qsort(array, array_size, sizeof(int), comparator);

  // free buffers
  free(send_buffer);
  free(receive_buffer);

  // RETURN
  return;
}
// END COMP EXCHANGE MIN
////////////////////

////////////////////
// COMP EXCHANGE MAX (j)
Comp_exchange_High(){
  // partner process = rank XOR (1 << j)
  int partner = rank ^ (1 << j);
  
  // MPI_Recv MAX from partner (A)
  int max;
  MPI_Recv(&max, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  
  // MPI_Send MIN of array to partner (B)
  // Min is first in array, assuming sorted
  MPI_Send(&array[0], 1, MPI_INT, partner, 0, MPI_COMM_WORLD);

  // copy all values in array smaller than MAX to send_buffer
  int i;
  int * send_buffer = malloc((array_size + 1) * sizeof(int));
  int send_buf_size = 0;
  for (i = 0; i < array_size; i++){
    if (array[i] < max){
      send_buffer[send_buf_size + 1] = array[i];
      send_buf_size++;
    }
    else{
      break;
    }
  }

  // MPI_Recv array from partner, store in receive_buffer (C)
  int * receive_buffer = malloc((array_size + 1) * sizeof(int));
  MPI_Recv(receive_buffer, array_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  
  // MPI_Send send_buffer array to partner (D)
  MPI_Send(send_buffer, send_buf_size, MPI_INT, partner, 0, MPI_COMM_WORLD);

  // store largest value from receive_buffer at start of array
  for (i = 1; i < receive_buffer[0] + 1; i++){
    if (array[0] < receive_buffer[i]){
      array[0] = receive_buffer[i];
    }
    else {
      break;
    }
  }

  // sort array
  qsort(array, array_size, sizeof(int), comparator);
  
  // free buffers
  free(send_buffer);
  free(receive_buffer);

  // RETURN
  return;
}
// END COMP EXCHANGE MAX
////////////////////

int comparator(const void* a, const void* b){
  return ( * (int * )a - * (int *)b);
}