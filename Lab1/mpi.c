#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //program input argument
  long inputArgument = ins__args.arg; 

  int is_prime = 1;
  int is_prime_final = 1;

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  
  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank)
      gettimeofday(&ins__tstart, NULL);


  // run your computations here (including MPI communication)
  for (long i = 2 + myrank; i*i <= inputArgument; i += nproc ){
    if(inputArgument % i == 0){
      is_prime = 0;
      break;
    }
  }

  MPI_Reduce (&is_prime, &is_prime_final, 1, MPI_INT, MPI_LAND, 0, MPI_COMM_WORLD);

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    printf ("is_prime=%d\n", is_prime_final);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }

  MPI_Finalize();

  return 0;
}
