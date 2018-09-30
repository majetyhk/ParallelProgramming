/*
Single Author info:
hmajety  Hari Krishna Majety
Group info:
hmajety  Hari Krishna Majety
srout Sweta Rout
mreddy2 Harshavardhan Reddy Muppidi
*/
#include <stdlib.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include <time.h>
#include <unistd.h>
#include "mpi.h"
#define __DEBUG
#define TSCALE 1.0
#define VSQR 0.1


#define disp2darr(src,row,col) for(int kai=0;kai<row;kai++) {for(int l=0;l<col;l++) { int idx = kai*row+l; printf("%lf ",src[idx]);} printf("\n");} printf("\n")

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

__global__ void evolve_GPU(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t, int nThreads, int rank){
  int idx = blockIdx.x*blockDim.x+threadIdx.x;
  int i, j, pebbJ, pebbIdx;
  i = idx % n;
  j = idx /n;
  pebbJ = rank*(n/4)+j;
  pebbIdx = pebbJ*n+i;
  //printf("[ %d, %d ]: %lf \n",i,j, un[idx])
  if(idx < n*((n/4)+2)){
    if(rank==0){
      //Exclude last two rows in the array for computations apart from the top row of the lake
      if( i == 0 || i == n-1|| j == 0 || j >= n/4)
      {
        un[idx] = 0.;
        //printf("[ %d, %d ]: %lf \n",i,j, un[idx]);
      }
      else{
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n]  + 0.25*(uc[idx + n - 1] + uc[idx + n + 1] + uc[idx - n - 1] + uc[idx - n + 1]) - 
                    5 * uc[idx])/(h * h) + (double)(-__expf(-TSCALE * (float)t) * pebbles[pebbIdx]));
        //printf("[%d: %d, %d -(%d)] - %lf \n",rank,i,j, idx,un[idx]);
      }
    }
    else{
      //Exclude first and last row in the array for computations
      if( i  == 0 || i == n-1|| j == 0 || j >= n/4 + 1)
      {
        un[idx] = 0.;
      }
      else{
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n]  + 0.25*(uc[idx + n - 1] + uc[idx + n + 1] + uc[idx - n - 1] + uc[idx - n + 1]) - 
                    5 * uc[idx])/(h * h) + (double)(-__expf(-TSCALE * (float)t) * pebbles[pebbIdx]));
        //printf("[%d: %d, %d -(%d)] - %lf \n",rank,i,j, idx,un[idx]);
      }
    }
    /*else if(rank == 3){
      //Exclude first two rows in the array for computations apart from the last row of the lake
      if( i == 0 || i == n-1|| j == 0  || j >= n/4+1)
      {
        un[idx] = 0.;
      }
      else{
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n]  + 0.25*(uc[idx + n - 1] + uc[idx + n + 1] + uc[idx - n - 1] + uc[idx - n + 1]) - 
                    5 * uc[idx])/(h * h) + (double)(-__expf(-TSCALE * (float)t) * pebbles[idx]));
        printf("[%d: %d, %d -(%d)] - %lf \n",rank,i,j, idx,un[idx]);
      }
    }*/
    
    //printf("[%d: %d, %d -(%d)] - %lf \n",rank,i,j, idx,un[idx]);
    //fflush(stdin);
    
    //fflush(stdin);
  }
 
}

void run_gpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time, int nthreads,int tag, int rank, MPI_Comm world)
{
  cudaEvent_t kstart, kstop;
  float ktime;
  int npoints = n;
  MPI_Status status;
  /* HW2: Define your local variables here */
  double *uc, *uo, *nd, *cd, *od, *pebblesd;
  double t, dt;

  //un = (double*)malloc(sizeof(double) * n * n);
  uc = (double*)calloc( ((n/4)+2) * n,sizeof(double));
  uo = (double*)calloc( ((n/4)+2) * n,sizeof(double));


  memcpy(uo, u0, sizeof(double) * ((n/4)+2) * n);
  memcpy(uc, u1, sizeof(double) * ((n/4)+2) * n);
   

  t = 0.0;
  dt = h / 2.;
  /*for (int i = 0; i < n*n; ++i)
  {
    printf("%d - %lf; ",i, pebbles[i]);
  }*/
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
  CUDA_CALL(cudaMalloc((void **)&od, ((n/4)+2)*n*sizeof(double)));
  CUDA_CALL(cudaMalloc((void **)&cd, ((n/4)+2)*n*sizeof(double)));
  CUDA_CALL(cudaMalloc((void **)&nd, ((n/4)+2)*n*sizeof(double)));
  CUDA_CALL(cudaMemset(od,0,((n/4)+2)*n*sizeof(double)));
  CUDA_CALL(cudaMemset(cd,0,((n/4)+2)*n*sizeof(double)));
  CUDA_CALL(cudaMalloc((void **)&pebblesd, n*n*sizeof(double)));

  //CUDA_CALL(cudaMemcpy(od,uo, ((n/4)+2)*((n/4)+2)*sizeof(double), cudaMemcpyHostToDevice));
  //CUDA_CALL(cudaMemcpy(cd,uc, ((n/4)+2)*((n/4)+2)*sizeof(double), cudaMemcpyHostToDevice));
  CUDA_CALL(cudaMemcpy(pebblesd,pebbles, n*n*sizeof(double), cudaMemcpyHostToDevice));

  printf("%d rank\n",rank);
  //int count=0;
  while(1){
    //Transfer the boundary slices to other processes for computation
    if(rank==0){
      tag = rank;
      //Stage 1
      MPI_Send(uo+(npoints/4 - 1)*npoints,npoints,MPI_DOUBLE,rank+1,tag,world);
      MPI_Send(uc+(npoints/4 - 1)*npoints,npoints,MPI_DOUBLE,rank+1,tag,world);
      //Stage 2
      //-----No actions for rank 0 in this stage------
      //Stage 3
      MPI_Recv(uo+(npoints/4)*npoints,npoints, MPI_DOUBLE, rank+1, tag, world, &status);
      MPI_Recv(uc+(npoints/4)*npoints,npoints, MPI_DOUBLE, rank+1, tag, world, &status);

    }
    else if(rank == 1){
      tag = rank-1;
      //Stage 1
      MPI_Recv(uo,npoints,MPI_DOUBLE,rank-1,tag,world, &status);
      MPI_Recv(uc,npoints,MPI_DOUBLE,rank-1,tag,world, &status);
      //Stage 2
      MPI_Send(uo+(npoints/4)*npoints,npoints,MPI_DOUBLE,rank+1,rank+1,world);
      MPI_Send(uc+(npoints/4)*npoints,npoints,MPI_DOUBLE,rank+1,rank+1,world);
      //Stage 3
      MPI_Send(uo+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);
      MPI_Send(uc+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);
      //Stage 4
      MPI_Recv(uo+(npoints/4 + 1)*npoints,npoints, MPI_DOUBLE, rank+1, rank+1, world, &status);
      MPI_Recv(uc+(npoints/4 + 1)*npoints,npoints, MPI_DOUBLE, rank+1, rank+1, world, &status);

    }
    else if(rank==2){
      tag = rank;
      //Stage 1
      MPI_Send(uo+(npoints/4)*npoints,npoints,MPI_DOUBLE,rank+1,tag,world);
      MPI_Send(uc+(npoints/4)*npoints,npoints,MPI_DOUBLE,rank+1,tag,world);
      //Stage 2
      MPI_Recv(uo,npoints,MPI_DOUBLE,rank-1,tag,world, &status);
      MPI_Recv(uc,npoints,MPI_DOUBLE,rank-1,tag,world, &status);
      //Stage 3
      MPI_Recv(uo+(npoints/4 + 1)*npoints,npoints, MPI_DOUBLE, rank+1, tag,world, &status);
      MPI_Recv(uc+(npoints/4 + 1)*npoints,npoints, MPI_DOUBLE, rank+1, tag, world, &status);
      //Stage 4
      MPI_Send(uo+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);
      MPI_Send(uc+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);
    }
    else{
      tag = rank-1;
      //Stage 1
      MPI_Recv(uo, npoints, MPI_DOUBLE, rank-1,tag, world, &status);
      MPI_Recv(uc, npoints, MPI_DOUBLE, rank-1, tag, world, &status);
      //Stage 2
      //------ No actions for rank 3 in this stage
      //Stage 3
      MPI_Send(uo+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);
      MPI_Send(uc+npoints,npoints,MPI_DOUBLE,rank-1,tag,world);

    }
    //disp2darr(uc,npoints/4+1,npoints);
    /*if(rank==0||rank==3){
      for(int j = 0;j<npoints/4+1;j++){
        for(int i =0;i<npoints;i++){
          printf("[%d:%d,%d]:%lf ",rank,i,j,uc[j*npoints+i]);
        }
        printf("\n");
      }
    }
    else{
      for(int j = 0;j<npoints/4+2;j++){
        for(int i =0;i<npoints;i++){
          printf("[%d:%d,%d]:%lf ",rank,i,j,uc[j*npoints+i]);
        }
        printf("\n");
      }
    }*/
    
    CUDA_CALL(cudaMemcpy(od,uo, ((n/4)+2)*n*sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CALL(cudaMemcpy(cd,uc, ((n/4)+2)*n*sizeof(double), cudaMemcpyHostToDevice));
    //sleep(rank*0.6);
    //Invoke Cuda Kernel with same number of threads in all nodes and handle the additional threads within the Kernel
    evolve_GPU<<<nBlocks,threadsPerBlock>>>(nd,cd,od,pebblesd,n,h,dt,t,nthreads, rank);

    cudaMemcpy(u,nd, ((n/4)+2)*n*sizeof(double), cudaMemcpyDeviceToHost);
    memcpy(uo, uc, sizeof(double) * ((n/4)+2) * n);
    memcpy(uc, u, sizeof(double) * ((n/4)+2) * n);
  	if(!tpdt(&t,dt,end_time)) break;
    //count++;
    //if(count==1) break;

  }
  
  /*if(rank==0||rank==3){
      for(int j = 0;j<npoints/4+1;j++){
        for(int i =0;i<npoints;i++){
          printf("[%d:%d,%d]:%lf ",rank,i,j,u[j*npoints+i]);
        }
        printf("\n");
      }
    }
    else{
      for(int j = 0;j<npoints/4+2;j++){
        for(int i =0;i<npoints;i++){
          printf("[%d:%d,%d]:%lf ",rank,i,j,u[j*npoints+i]);
        }
        printf("\n");
      }
    }*/
  /* Stop GPU computation timer */
  /*for (int i = 0; i < n;  ++i)
  {
    fflush(stdin);
    for (int j = 0; j < n/4; ++j)
    {
      printf("%d:%lf ", u[i*n+j]);
    }
    printf("\n");
  }*/
  CUDA_CALL(cudaEventRecord(kstop, 0));
  CUDA_CALL(cudaEventSynchronize(kstop));
  CUDA_CALL(cudaEventElapsedTime(&ktime, kstart, kstop));
  printf("GPU computation: %f msec\n", ktime);

  /* HW2: Add post CUDA kernel call processing and cleanup here */
  CUDA_CALL(cudaFree(od));
  CUDA_CALL(cudaFree(cd));
  CUDA_CALL(cudaFree(nd));
  CUDA_CALL(cudaFree(pebblesd));
  free(uc);
  free(uo);
  /* timer cleanup */
  CUDA_CALL(cudaEventDestroy(kstart));
  CUDA_CALL(cudaEventDestroy(kstop));
  CUDA_CALL(cudaDeviceReset());
}
