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
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"
#include <unistd.h>
#define _USE_MATH_DEFINES

#define XMIN 0.0
#define XMAX 1.0
#define YMIN 0.0
#define YMAX 1.0

#define MAX_PSZ 10
#define TSCALE 1.0
#define VSQR 0.1

void init(double *u, double *pebbles, int n);
void evolve(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
void evolve9pt(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t);
int tpdt(double *t, double dt, double end_time);
void print_heatmap(const char *filename, double *u, int n, double h);
void init_pebbles(double *p, int pn, int n);

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time);

extern void run_gpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time, int nthreads,int tag,int rank,MPI_Comm world);
char *my_itoa(int num, char *str)
{
  if(str == NULL)
  {
    return NULL;
  }
  sprintf(str, "%d", num);
  return str;
}

int main(int argc, char *argv[])
{

  if(argc != 5)
  {
    printf("Usage: %s npoints npebs time_finish nthreads \n",argv[0]);
    return 0;
  }

  int     npoints   = atoi(argv[1]);
  int     npebs     = atoi(argv[2]);
  double  end_time  = (double)atof(argv[3]);
  int     nthreads  = atoi(argv[4]);
  int 	  narea	    = npoints * npoints;

  //------------------------MPI Setup-----------------------

  int   numproc, rank;
  int tag=50;
  int startValueX, endValueX, startValueY, endValueY;             // For Defining Processes End Grid Points

  /* initialize MPI */
  MPI_Init(&argc, &argv);

  /* get the number of procs in the comm */
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);

  /* get my rank in the comm */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /*startValueX = (rank%2) * (npoints/2);
  endValueX = (npoints/2)*(1+rank%2);

  startValueY = (rank%2)*(npoints/2);
  endValueY = (npoints/2)*(1+rank%2);*/

  startValueX = 0; // assign starting and ending indices on x and y axes in the lake for different processes based on rank
  endValueX = npoints-1;

  startValueY = rank * npoints/4; 
  endValueY = startValueY + npoints/4 -1;

  printf("Rank %d : [%d, %d] - [%d, %d]\n",rank, startValueX,endValueX,startValueY, endValueY );

  //------------------------MPI Setup-----------------------

  double *u_i0, *u_i1;
  double *u_cpu, *u_gpu, *pebs;
  double h;

  double  elapsed_gpu;
  struct timeval gpu_start, gpu_end;
  
  u_i0 = (double*)malloc(sizeof(double) * narea);
  u_i1 = (double*)malloc(sizeof(double) * narea);
  pebs = (double*)malloc(sizeof(double) * narea);

  u_cpu = (double*)malloc(sizeof(double) * narea);
  u_gpu = (double*)malloc(sizeof(double) * ((npoints/4)+2)*npoints);

  printf("Running %s with (%d x %d) grid, until %f, with %d threads\n", argv[0], npoints, npoints, end_time, nthreads);

  h = (XMAX - XMIN)/npoints;
  
  init_pebbles(pebs, npebs, npoints); // Initialize pebbles
  init(u_i0, pebs, npoints); //Initialize lake arrays
  init(u_i1, pebs, npoints);
  char fname[20];
  
  


  //----------------------MPI Manipulations--------------------------------------------

  double *uc, *un;
  int fidx,cidx; // Create indices and required arrays to pass it to kernel
  if(rank==0||rank==4){ // First and last processes will have one extra row for exchange
    uc = (double*)calloc((npoints/4 + 1)*(npoints),sizeof(double));
    un = (double*)calloc((npoints/4 + 1)*(npoints),sizeof(double));
  }
  else{ // Other processes have two extra rows, one above and one below for exchange
    uc = (double*)calloc((npoints/4 + 2)*(npoints),sizeof(double));
    un = (double*)calloc((npoints/4 + 2)*(npoints),sizeof(double));
  }
  if(rank==0){
    cidx = 0;
  }
  else{
    cidx = npoints;
  }

  // Copy the required parts of the lake into the respective arrays
  for (int j = startValueY; j < endValueY; ++j)
  {
    for (int i = startValueX; i < endValueX; ++i)
    {
      fidx = j*npoints+i;
      uc[cidx] = u_i0[fidx];
      un[cidx] = u_i1[fidx];
      cidx++;
    }
  }
  if(rank == 0) {
    //my_itoa(rank,fname);
    //strcat(fname,".dat");

    //Print initial heatmap of the lake
    print_heatmap("lake_i.dat", u_i0, npoints, h);
  }
  //print_heatmap(fname, uc, npoints, h);
  /*my_itoa(rank,fname);
  strcat(fname,".dat");
  print_heatmap(fname, uc, npoints, h);*/
  


//----------------------MPI Manipulations--------------------------------------------


  //gettimeofday(&cpu_start, NULL);
  //run_cpu(u_cpu, u_i0, u_i1, pebs, npoints, h, end_time);
  //gettimeofday(&cpu_end, NULL);

  //elapsed_cpu = ((cpu_end.tv_sec + cpu_end.tv_usec * 1e-6)-(
  //                cpu_start.tv_sec + cpu_start.tv_usec * 1e-6));
  //printf("CPU took %f seconds\n", elapsed_cpu);

  //Run the GPU code for resized arrays
  gettimeofday(&gpu_start, NULL);
  run_gpu(u_gpu, uc, un, pebs, npoints, h, end_time, nthreads,tag,rank,MPI_COMM_WORLD);  
  gettimeofday(&gpu_end, NULL);
  elapsed_gpu = ((gpu_end.tv_sec + gpu_end.tv_usec * 1e-6)-(
                  gpu_start.tv_sec + gpu_start.tv_usec * 1e-6));
  printf("GPU took %f seconds\n", elapsed_gpu);
  
  double *res;
  if(rank == 0){
    res = (double*)(malloc(npoints*npoints*sizeof(double)));
  }

  //Gather GPU Output at the the root node
  if(rank == 0){
    MPI_Gather(u_gpu,(npoints/4)*npoints,MPI_DOUBLE,res,(npoints/4)*npoints,MPI_DOUBLE,0,MPI_COMM_WORLD);
    /*for(int j = 0;j<npoints;j++){
      for(int i =0;i<npoints;i++){
        printf("[%d:%d,%d]:%lf ",rank,i,j,res[j*npoints+i]);
      }
      printf("\n");
    }*/
  }
  else{
    MPI_Gather(u_gpu+npoints,(npoints/4)*npoints,MPI_DOUBLE,res,(npoints/4)*npoints,MPI_DOUBLE,0,MPI_COMM_WORLD);
  }

  
    

  //print_heatmap("lake_f.dat", u_cpu, npoints, h);
  my_itoa(rank,fname);
  strcat(fname,"_gpu.dat");
  if(rank==0){
    //print_heatmap(fname, u_gpu, npoints, h);
    print_heatmap("lake_f_gpu.dat", res, npoints, h);
  }
  /*my_itoa(rank,fname);
  strcat(fname,"_gpu.dat");
  print_heatmap(fname, u_gpu, npoints, h);*/

  free(u_i0);
  free(u_i1);
  free(pebs);
  free(u_cpu);
  free(u_gpu);

  return 0;
}

void run_cpu(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time)
{
  double *un, *uc, *uo;
  double t, dt;

  un = (double*)malloc(sizeof(double) * n * n);
  uc = (double*)malloc(sizeof(double) * n * n);
  uo = (double*)malloc(sizeof(double) * n * n);

  memcpy(uo, u0, sizeof(double) * n * n);
  memcpy(uc, u1, sizeof(double) * n * n);

  t = 0.;
  dt = h / 2.;

  while(1)
  {
    evolve9pt(un, uc, uo, pebbles, n, h, dt, t);

    memcpy(uo, uc, sizeof(double) * n * n);
    memcpy(uc, un, sizeof(double) * n * n);

    if(!tpdt(&t,dt,end_time)) break;
  }
  
  memcpy(u, un, sizeof(double) * n * n);
}

void init_pebbles(double *p, int pn, int n)
{
  int i, j, k, idx;
  int sz;

  srand( 2347 );
  memset(p, 0, sizeof(double) * n * n);

  for( k = 0; k < pn ; k++ )
  {
    i = rand() % (n - 4) + 2;
    j = rand() % (n - 4) + 2;
    sz = rand() % MAX_PSZ;
    idx = j + i * n;
    p[idx] = (double) sz;
  }
}

double f(double p, double t)
{
  return -expf(-TSCALE * t) * p;
}

int tpdt(double *t, double dt, double tf)
{
  if((*t) + dt > tf) return 0;
  (*t) = (*t) + dt;
  return 1;
}

void init(double *u, double *pebbles, int n)
{
  int i, j, idx;

  for(i = 0; i < n ; i++)
  {
    for(j = 0; j < n ; j++)
    {
      idx = j + i * n;
      u[idx] = f(pebbles[idx], 0.0);
    }
  }
}

void evolve(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t)
{
  int i, j, idx;

  for( i = 0; i < n; i++)
  {
    for( j = 0; j < n; j++)
    {
      idx = j + i * n;

      if( i == 0 || i == n - 1 || j == 0 || j == n - 1)
      {
        un[idx] = 0.;
      }
      else
      {
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n] - 4 * uc[idx])/(h * h) + f(pebbles[idx],t));
      }
    }
  }
}

void evolve9pt(double *un, double *uc, double *uo, double *pebbles, int n, double h, double dt, double t){
  int i, j, idx;

  for( i = 0; i < n; i++)
  {
    for( j = 0; j < n; j++)
    {
      idx = j + i * n;

      if( i == 0 || i == n - 1 || j == 0 || j == n - 1)
      {
        un[idx] = 0.;
      }
      else
      {
        un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] + 
                    uc[idx + n] + uc[idx - n]  + 0.25*(uc[idx + n - 1] + uc[idx + n + 1] + uc[idx - n - 1] + uc[idx - n + 1]) - 
                    5 * uc[idx])/(h * h) + f(pebbles[idx],t));
      }
    }
  }
}

void print_heatmap(const char *filename, double *u, int n, double h)
{
  int i, j, idx;

  FILE *fp = fopen(filename, "w");  

  for( i = 0; i < n; i++ )
  {
    for( j = 0; j < n; j++ )
    {
      idx = j + i * n;
      fprintf(fp, "%lf %lf %lf\n", i*h, j*h, u[idx]*10);
    }
  }
  
  fclose(fp);
} 
