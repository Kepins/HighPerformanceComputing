#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <sys/time.h>
#include "numgen.c"


#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

__host__
void errorexit(const char *s) {
    printf("\n%s",s);	
    exit(EXIT_FAILURE);	 	
}

//generate elements of sequence – parallel part
__global__ 
void calculate(long inputArgument, unsigned long int* input, int *result) {
    int my_index=blockIdx.x*blockDim.x+threadIdx.x;
    
    if (my_index >= inputArgument){
      return;
    }

    unsigned long int number = input[my_index];
    if (number == 0 || number == 1){
      result[my_index] = 0;
      return;
    }
    for (long j = 2; j*j <= number; j ++ ){
      if(number % j == 0){
        result[my_index] = 0;
        return;
      }
    }
    result[my_index]=1;
}


int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);
  
  //program input argument
  long inputArgument = ins__args.arg; 
  unsigned long int *numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  numgen(inputArgument, numbers);

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);
  

  int threadsinblock=1024;
  int blocksingrid = inputArgument / threadsinblock + 1;	

  //memory allocation on host
  int *hresults=(int*)malloc(inputArgument*sizeof(int));
  if (!hresults) errorexit("Error allocating memory on the host");	
  

  //memory allocation on device (GPU)
  unsigned long int *dinputs=NULL;
  if (cudaSuccess!=cudaMalloc((void **)&dinputs, inputArgument*sizeof(unsigned long int)))
    errorexit("Error allocating memory on the GPU");
  int *dresults=NULL;
  if (cudaSuccess!=cudaMalloc((void **)&dresults, inputArgument*sizeof(int)))
    errorexit("Error allocating memory on the GPU");

  long long result = 0;


  //copy all elements from host to device
  if (cudaSuccess!=cudaMemcpy(dinputs, numbers, inputArgument*sizeof(unsigned long int), cudaMemcpyHostToDevice))
      errorexit("Error copying results");

  // run your CUDA kernel(s) here
  //call kernel on GPU – calculation are executed on GPU
  calculate<<<blocksingrid,threadsinblock>>>(inputArgument, dinputs, dresults);
  if (cudaSuccess!=cudaGetLastError())
    errorexit("Error during kernel launch");

  //copy all elements from device to host
  if (cudaSuccess!=cudaMemcpy(hresults, dresults, inputArgument*sizeof(int), cudaMemcpyDeviceToHost))
      errorexit("Error copying results");
  
  // synchronize/finalize your CUDA computations
  for(int i=0;i<inputArgument;i++) {
    result = result + hresults[i];
  }

  printf("\nThe final result is %lld\n",result);

  //free resources
  free(hresults);
  if (cudaSuccess!=cudaFree(dresults))
    errorexit("Error when deallocating space on the GPU");

  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
}
