#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "numgen.c"

#define DATA 0
#define RESULT 1
#define FINISH 2

#define PACKET_SIZE 100000

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


  unsigned long num_packets = 0;
  if(!myrank){
    gettimeofday(&ins__tstart, NULL);

    num_packets = inputArgument / PACKET_SIZE;
    if(inputArgument % PACKET_SIZE != 0) {
      // make last uneven packet bigger and pad it with non prime later
      num_packets++;
    }
    if(num_packets < 2*(nproc -1)){
      // each slave needs to recieve at least one packet
      num_packets = 2*(nproc -1);
    }

    numbers = (unsigned long int*)malloc(num_packets * PACKET_SIZE * sizeof(unsigned long int));

    for(int i=inputArgument; i<num_packets * PACKET_SIZE;i++){
      // fill number that will not be generated with non prime numbers so they don't mess the result
      numbers[i] = 4;
    }

    numgen(inputArgument, numbers);
  }


  // run your computations here (including MPI communication)
  
  if (myrank == 0)
  {
    //master
    MPI_Status status;
    unsigned long final_result = 0;

    unsigned long packets_sent = 0;

    MPI_Request* requests = (MPI_Request *) malloc (3 * (nproc - 1) * sizeof (MPI_Request));
    for (int i = 0; i < 3 * (nproc - 1); i++)
			requests[i] = MPI_REQUEST_NULL;	// none active at this point

    int requestcompleted;
    unsigned long* results_recieved = (long *) malloc ((nproc - 1) * sizeof (long));

    for(int i = 1; i< nproc;i++){
      //send packets of data
      MPI_Send (numbers + (packets_sent * PACKET_SIZE), PACKET_SIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD);
      packets_sent++;
    }

    for(int i = 1; i< nproc;i++){
      //start receiving
      MPI_Irecv (&(results_recieved[i - 1]), 1, MPI_LONG, i, RESULT, MPI_COMM_WORLD, &(requests[i - 1]));

      //start next send
      MPI_Isend (numbers + (packets_sent * PACKET_SIZE), PACKET_SIZE, MPI_LONG, i, DATA, MPI_COMM_WORLD, &(requests[i - 1 + nproc - 1]));
      packets_sent++;
    }

    while(packets_sent < num_packets) {
      // wait for completion of any of the requests
			MPI_Waitany (2 * (nproc - 1), requests, &requestcompleted, MPI_STATUS_IGNORE);
      if (requestcompleted >= (nproc - 1)) {
        // sent finished and not receive
        continue;
      }
      // we know that receive finished
      final_result += results_recieved[requestcompleted];

      // make sure send has terminated
			MPI_Wait (&(requests[nproc - 1 + requestcompleted]), MPI_STATUS_IGNORE);

      // start next send
      MPI_Isend (numbers + (packets_sent * PACKET_SIZE), PACKET_SIZE, MPI_LONG, requestcompleted + 1, DATA, MPI_COMM_WORLD, &(requests[requestcompleted + nproc - 1]));;
      packets_sent++;

      // start corresponding recv
      MPI_Irecv (&(results_recieved[requestcompleted]), 1, MPI_LONG, requestcompleted + 1, RESULT, MPI_COMM_WORLD, &(requests[requestcompleted]));
    }

    // now send the FINISH to the slaves
		for (int i = 1; i < nproc; i++)
		{
			MPI_Isend (NULL, 0, MPI_LONG, i, FINISH, MPI_COMM_WORLD, &(requests[i - 1 + 2 * (nproc - 1)]));
		}

    // now receive results from the processes - that is finalize the pending requests
		MPI_Waitall (3 * nproc - 3, requests, MPI_STATUSES_IGNORE);
		for (int i = 0; i < (nproc - 1); i++)
		{
      // add the results
			final_result += results_recieved[i];
		}


		for (int i = 1; i < nproc; i++)
		{
      // receive result for the initial sends
			MPI_Recv (&(results_recieved[i-1]), 1, MPI_LONG, i, RESULT, MPI_COMM_WORLD, &status);
			final_result += results_recieved[i-1];
		}

    // display the result
    printf ("\nHi, I am process 0, the result is %ld\n", final_result);
  }
  else {
    // slave
    MPI_Status status;

    unsigned long my_result_working = 0;
    unsigned long my_result_sending = 0;

    int requestcompleted = 0;
    MPI_Request requests[3];
    requests[0] = requests[1] = requests[2] = MPI_REQUEST_NULL;
    long* buffor1 = malloc(sizeof(long) * PACKET_SIZE);
    long* buffor2 = malloc(sizeof(long) * PACKET_SIZE);

    long** current_buffor = &buffor1;
    long** other_buffor = &buffor2;
    
    MPI_Recv (*current_buffor, PACKET_SIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);

    MPI_Irecv (NULL, 0, MPI_LONG, 0, FINISH, MPI_COMM_WORLD, &(requests[1]));

    int finished = 0;
    do
    {
      MPI_Irecv (*other_buffor, PACKET_SIZE, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &(requests[0]));

      my_result_working = PACKET_SIZE;
      
      for (long i=0;i<PACKET_SIZE;i++){
        long work_number = (*current_buffor)[i];
        if (work_number == 0 || work_number == 1){
          my_result_working -= 1;
          break;
        }
        for (long j = 2; j*j <= work_number; j ++ ){
          if(work_number % j == 0){
            my_result_working -= 1;
            break;
          }
        }
      }

      MPI_Waitany (2, requests, &requestcompleted, MPI_STATUS_IGNORE);
      if (requestcompleted == 0) {
        // SWAP Pointers to buffors
        long** temp = other_buffor;
        other_buffor = current_buffor;
        current_buffor = temp;
      }
      else {
        // FINISH Received
        finished = 1;
      }

      // wait till send is finished
      MPI_Wait (&(requests[2]), MPI_STATUS_IGNORE);
      my_result_sending = my_result_working;

      // send the result back
      MPI_Isend (&my_result_sending, 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD, &(requests[2]));
    }while(!finished);
    // now finish sending the last results to the master
		MPI_Wait (&(requests[2]), MPI_STATUS_IGNORE);
  }

  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();
}
