#include <stdlib.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include <time.h>

#define __DEBUG
#define TSCALE 1.0
#define VSQR 0.1

#define CUDA_CALL( err )     __cudaSafeCall( err, __FILE__, __LINE__ )
#define CUDA_CHK_ERR() __cudaCheckError(__FILE__,__LINE__)

extern int tpdt(double *t, double dt, double end_time);

/**************************************
* void __cudaSafeCall(cudaError err, const char *file, const int line)
* void __cudaCheckError(const char *file, const int line)
*
* These routines were taken from the GPU Computing SDK
* (http://developer.nvidia.com/gpu-computing-sdk) include file "cutil.h"
**************************************/
inline void __cudaSafeCall( cudaError err, const char *file, const int line )
{
#ifdef __DEBUG

#pragma warning( push )
#pragma warning( disable: 4127 ) // Prevent warning on do-while(0);
  do
  {
    if ( cudaSuccess != err )
    {
      fprintf( stderr, "cudaSafeCall() failed at %s:%i : %s\n",
              file, line, cudaGetErrorString( err ) );
      exit( -1 );
    }
  } while ( 0 );
#pragma warning( pop )
#endif  // __DEBUG
  return;
}

inline void __cudaCheckError( const char *file, const int line )
{
#ifdef __DEBUG
#pragma warning( push )
#pragma warning( disable: 4127 ) // Prevent warning on do-while(0);
  do
  {
    cudaError_t err = cudaGetLastError();
    if ( cudaSuccess != err )
    {
      fprintf( stderr, "cudaCheckError() failed at %s:%i : %s.\n",
               file, line, cudaGetErrorString( err ) );
      exit( -1 );
    }
    // More careful checking. However, this will affect performance.
    // Comment if not needed.
    /*err = cudaThreadSynchronize();
    if( cudaSuccess != err )
    {
      fprintf( stderr, "cudaCheckError() with sync failed at %s:%i : %s.\n",
               file, line, cudaGetErrorString( err ) );
      exit( -1 );
    }*/
  } while ( 0 );
#pragma warning( pop )
#endif // __DEBUG
  return;
}

__global__ void evolve_GPU(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t, int nThreads){
  int idx = blockIdx.x*(nThreads*nThreads)+threadIdx.x;
  if(idx<n*n){
    un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n]  + 0.25*(uc[idx + n - 1] + uc[idx + n + 1] + uc[idx - n - 1] + uc[idx - n + 1]) - 
                    5 * uc[idx])/(h * h) + (double)(-__expf(-TSCALE * (float)t) * pebbles[idx]));
  }
}

void run_gpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time, int nthreads)
{
	cudaEvent_t kstart, kstop;
	float ktime;
        
	/* HW2: Define your local variables here */
  double *uc, *uo, *nd, *cd, *od;
  double t, dt;

  //un = (double*)malloc(sizeof(double) * n * n);
  uc = (double*)malloc(sizeof(double) * n * n);
  uo = (double*)malloc(sizeof(double) * n * n);

  memcpy(uo, u0, sizeof(double) * n * n);
  memcpy(uc, u1, sizeof(double) * n * n);

  t = 0.;
  dt = h / 2.;

  /* Set up device timers */  
	CUDA_CALL(cudaSetDevice(0));
	CUDA_CALL(cudaEventCreate(&kstart));
	CUDA_CALL(cudaEventCreate(&kstop));

	/* HW2: Add CUDA kernel call preperation code here */

  int threadsPerBlock = nthreads * nthreads;
  int nBlocks = (n/nthreads)*(n/nthreads);
  //dim3 blockdims(nthreads,nthreads,1);
  //dim3 griddims(n/nthreads , n/nthreads , 1 );

	/* Start GPU computation timer */
	CUDA_CALL(cudaEventRecord(kstart, 0));

	/* HW2: Add main lake simulation loop here */
	CUDA_CALL(cudaMalloc((void **)&od, n*n*sizeof(double)));
  CUDA_CALL(cudaMalloc((void **)&cd, n*n*sizeof(double)));
  CUDA_CALL(cudaMalloc((void **)&nd, n*n*sizeof(double)));

  CUDA_CALL(cudaMemcpy(od,uo, n*n*sizeof(double), cudaMemcpyHostToDevice));
  CUDA_CALL(cudaMemcpy(cd,uc, n*n*sizeof(double), cudaMemcpyHostToDevice));
  double *temp;
  int count=0;
  while(1){
    evolve_GPU<<<nBlocks, threadsPerBlock >>>(nd, cd, od, pebbles, n, h, dt, t, nthreads);
    temp = od;
    od = cd;
    cd = nd;
    printf(" %ld\n", t);
    if(!tpdt(&t,dt,end_time)) break;
    nd = temp;
  }
  printf("%ld,%ld",sizeof(u),u[1]);
  CUDA_CALL(cudaMemcpy(u,cd, n*n*sizeof(double), cudaMemcpyDeviceToHost));
        /* Stop GPU computation timer */
	CUDA_CALL(cudaEventRecord(kstop, 0));
	CUDA_CALL(cudaEventSynchronize(kstop));
	CUDA_CALL(cudaEventElapsedTime(&ktime, kstart, kstop));
	printf("GPU computation: %f msec\n", ktime);

	/* HW2: Add post CUDA kernel call processing and cleanup here */

	/* timer cleanup */
	CUDA_CALL(cudaEventDestroy(kstart));
	CUDA_CALL(cudaEventDestroy(kstop));
}
