#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include "numgen.c"


int main(int argc,char **argv) {


  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //set number of threads
  omp_set_num_threads(ins__args.n_thr);
  
  //program input argument
  long inputArgument = ins__args.arg; 
  unsigned long int *numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  numgen(inputArgument, numbers);

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);
  

  long i = 0;
  long j = 0;
  long work_number = 0;
  int is_work_number_prime = 0;
  long result = 0;
  // run your computations here (including OpenMP stuff)
  #pragma omp parallel for reduction(+:result) // reduction allow to execute operation on chosen variable in all threads
  for(i=0;i<ins__args.arg;i++) {
      is_work_number_prime = 1;
      work_number = numbers[i];
      
      if (work_number == 0 || work_number == 1){
        is_work_number_prime = 0;
      }
      for (j = 2; j*j <= work_number; j ++ ){
        if(work_number % j == 0){
          is_work_number_prime = 0;
          break;
        }
      }
      result += is_work_number_prime;
  }

  printf("%ld", result);
  
  
  // synchronize/finalize your computations
  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

}
