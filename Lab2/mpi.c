#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "numgen.c"

#define DATA 0
#define RESULT 1
#define FINISH 2

#define PACKET_SIZE 1000000

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //program input argument
  long inputArgument = ins__args.arg; 
  long lenNumbers = 0;

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  unsigned long int *numbers;

  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if (nproc < 2)
  {
    printf ("Run with at least 2 processes");
    MPI_Finalize ();
    return -1;
  }

  if(!myrank){
    gettimeofday(&ins__tstart, NULL);
    if (inputArgument < PACKET_SIZE * nproc){
      lenNumbers = PACKET_SIZE * nproc;
      numbers = (unsigned long int*)malloc(lenNumbers * sizeof(unsigned long int));
      for (int i=0;i<lenNumbers;i++){
        numbers[i] = 4;
      }
    }
    else {
      lenNumbers = inputArgument;
      numbers = (unsigned long int*)malloc(lenNumbers * sizeof(unsigned long int));
    }
    numgen(inputArgument, numbers);
  }


  // run your computations here (including MPI communication)
  MPI_Status status;
  unsigned long int work_numbers[PACKET_SIZE];
  unsigned long numbers_sent = 0;
  unsigned long int result = 0;
  unsigned long int result_temp = 0;

  if (myrank == 0)
  {
    for(int i = 1; i< nproc;i++){
      //send packets of data
      MPI_Send (numbers + numbers_sent, PACKET_SIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD);
      numbers_sent += PACKET_SIZE;
    }
    while (numbers_sent + PACKET_SIZE < lenNumbers)
		{
			// distribute remaining subranges to the processes which have completed their parts
			MPI_Recv (&result_temp, 1, MPI_LONG, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
			result += result_temp;
			// check the sender and send some more data
			MPI_Send (numbers + numbers_sent, PACKET_SIZE, MPI_LONG, status.MPI_SOURCE, DATA,MPI_COMM_WORLD);
      numbers_sent += PACKET_SIZE;
		}

    // calculate the last numbers that don't form one whole packet
    result_temp = lenNumbers - numbers_sent;
    for (long i=numbers_sent;i<lenNumbers;i++){
      long work_number = numbers[i];
      for (long j = 2; j*j <= work_number; j ++ ){
        if(work_number % j == 0){
          result_temp -= 1;
          break;
        }
      }
    }
    // add result from the last numbers that don't form one whole packet
    result += result_temp;

		for(int i = 1; i< nproc;i++){
      // now receive results from the processes
			MPI_Recv (&result_temp, 1, MPI_LONG, MPI_ANY_SOURCE, RESULT,MPI_COMM_WORLD, &status);
			result += result_temp;
		}
		for (int i = 1; i < nproc; i++){
      // shut down the slaves
			MPI_Send (NULL, 0, MPI_LONG, i, FINISH, MPI_COMM_WORLD);
		}
    // now display the result
    printf ("\nHi, I am process 0, the result is %ld\n", result);
  }
  else {
    // slave
    // this is easy - just receive data and do the work
		do
		{
			MPI_Probe (0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_TAG == DATA)
			{
        MPI_Recv (work_numbers, PACKET_SIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);
        result_temp = PACKET_SIZE;
        for (long i=0;i<PACKET_SIZE;i++){
          long work_number = work_numbers[i];
          for (long j = 2; j*j <= work_number; j ++ ){
            if(work_number % j == 0){
              result_temp -= 1;
              break;
            }
          }
        }
        // send the result back
        MPI_Send (&result_temp, 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD);
			}
		}while (status.MPI_TAG != FINISH);
  }

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}
