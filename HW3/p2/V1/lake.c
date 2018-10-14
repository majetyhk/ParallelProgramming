/*************************************
* lake.c
*
* Models pebbles on a lake
* Description:
* 
* This program uses centered finite differencing to
* solve the wave equation with sources.
*
* The interface is given as
*
*   lake [grid_size] [# of pebbles] [end time] [# threads]
*
* where
*
*   grid_size -     integer, size of one edge of the square grid;
*               so the true size of the computational grid will
*               be grid_size * grid_size
*
*   # of pebbles -  number of simulated "pebbles" to start with
*
*   end time -  the simulation starts from t=0.0 and goes to
*           t=[end time]
*
*   # threads -     the number of threads the simulation uses
*
**************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "./lake.h"
#include "./lake_util.h"

/* Probably not necessary but doesn't hurt */
#define _USE_MATH_DEFINES

/* Number of OpenMP threads */
int nthreads;

int main(int argc, char *argv[])
{

  if(argc != 5)
  {
    fprintf(stdout, "Usage: %s npoints npebs time_finish nthreads \n",argv[0]);
    return 0;
  }

  /* grab the arguments and setup some vars */
  int     npoints   = atoi(argv[1]);
  int     npebs     = atoi(argv[2]);
  double  end_time  = (double)atof(argv[3]);
          nthreads  = atoi(argv[4]);
  int     narea     = npoints * npoints;

  /* check input params for resitrictions */
  if ( npoints % nthreads != 0 )
  {
    fprintf(stderr, "BONK! npoints must be evenly divisible by nthreads\n Try again!");
    return 0;
  }

  /* get the program directory */
  set_wrkdir(argv[0]);
  /* main simulation arrays */
  double *u_i0, *u_i1;
  double *u_cpu, *pebs;

  /* u_err is used when calculating the 
   * error between one version of the code
   * and another. */
  double *u_err;

  /* h is the size of each grid cell */
  double h;
  /* used for error analysis */
  double avgerr;

  /* used for time analysis */
  double elapsed_cpu, elapsed_init;
  struct timeval cpu_start, cpu_end;
  
  /* allocate arrays */
  u_i0 = (double*)malloc(sizeof(double) * narea);
  u_i1 = (double*)malloc(sizeof(double) * narea);
  pebs = (double*)malloc(sizeof(double) * narea);

  u_cpu = (double*)malloc(sizeof(double) * narea);

  start_lake_log("lake.log");

  lake_log("running %s with (%d x %d) grid, until %f, with %d threads\n", argv[0], npoints, npoints, end_time, nthreads);

  /* initialize the simulation */
  h = (XMAX - XMIN)/npoints;

#ifdef __DEBUG
  lake_log("grid step size is %f\n",h);
  lake_log("initializing pebbles\n");
#endif

  init_pebbles(pebs, npebs, npoints);

#ifdef __DEBUG
  lake_log("initializing u0, u1\n");
#endif
  
  gettimeofday(&cpu_start, NULL);
  init(u_i0, pebs, npoints);
  init(u_i1, pebs, npoints);
  gettimeofday(&cpu_end, NULL);
  elapsed_init = ((cpu_end.tv_sec + cpu_end.tv_usec * 1e-6)-(
                  cpu_start.tv_sec + cpu_start.tv_usec * 1e-6));
  lake_log("Initialization took %f seconds\n", elapsed_init);
  
  /* print the initial configuration */
#ifdef __DEBUG
  lake_log("printing initial configuration file\n");
#endif

  print_heatmap("lake_i.dat", u_i0, npoints, h);

  /* time, run the simulation */
#ifdef __DEBUG
  lake_log("beginning simulation\n");
#endif

  gettimeofday(&cpu_start, NULL);
  run_sim(u_cpu, u_i0, u_i1, pebs, npoints, h, end_time);
  gettimeofday(&cpu_end, NULL);
  elapsed_cpu = ((cpu_end.tv_sec + cpu_end.tv_usec * 1e-6)-(
                  cpu_start.tv_sec + cpu_start.tv_usec * 1e-6));
  lake_log("Simulation took %f seconds\n", elapsed_cpu);
  lake_log("Init+Simulation took %f seconds\n", elapsed_init+elapsed_cpu);

  /* print the final configuration */
#ifdef __DEBUG
  lake_log("printing final configuration file\n");
#endif

  print_heatmap("lake_f.dat", u_cpu, npoints, h);

#ifdef __DEBUG
  lake_log("freeing memory\n");
#endif

  /* free memory */
  free(u_i0);
  free(u_i1);
  free(pebs);
  free(u_cpu);
  
  stop_lake_log();
  return 1;
}

/*****************************
* run_sim
*
* Input
* ----------
*   double *u0 - the inital configuation
*   double *u1 - the intial + 1 configuration
*   double *pebbles - the array of pebbles
*   int n - the grid size
*   double h - the grid step size
*   double end_time - the final time
*
* Output
* ----------
*   double *u - the final configuration
*
* Description
* ----------
*   run_sim is the main driver of the program.  It takes in the inital
* configuration and parameters, and runs them until end_time is reached.
*
*******************************/
void run_sim(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time)
{
  /* arrays used in the calculation */
  double *un, *uc, *uo, *temp;
  /* time vars */
  double t, dt;
  int i, j, idx;

  /* allocate the calculation arrays */
  un = (double*)malloc(sizeof(double) * n * n);
  uc = (double*)malloc(sizeof(double) * n * n);
  uo = (double*)malloc(sizeof(double) * n * n);

  /* put the inital configurations into the calculation arrays */
  memcpy(uo, u0, sizeof(double) * n * n);
  memcpy(uc, u1, sizeof(double) * n * n);

  /* start at t=0.0 */
  t = 0.;
  /* this is probably not ideal.  In principal, we should
   * keep the time-step at the size determined by the 
   * CFL condition
   * 
   * dt = h / vel_max
   *
   * where vel_max is the maximum velocity in the current
   * model.  The condition dt = h/2. should suffice, but 
   * be aware the possibility exists for madness and mayhem */
  dt = h / 2.;

  /* loop until time >= end_time */
  while(1)
  {

    /* run a central finite differencing scheme to solve
     * the wave equation in 2D */
    #pragma omp parallel for
    for( i = 0; i < n; i++)
    {
      //#pragma omp parallel for
      for( j = 0; j < n; j++)
      {
        idx = j + i * n;
        
        /* impose the u|_s = 0 boundary conditions */
        if( i == 0 || i == n - 1 || j == 0 || j == n - 1)
        {
          un[idx] = 0.;
        }

        /* otherwise do the FD scheme */
        else
        {
            un[idx] = 2*uc[idx] - uo[idx] + VSQR *(dt * dt) *((uc[idx-1] + uc[idx+1] +
                      uc[idx+n] + uc[idx-n] + 0.25 * (uc[idx-n-1] + uc[idx+n-1]+ uc[idx-n+1] + 
					  uc[idx+n+1]) - 5 * uc[idx])/(h * h) + f(pebbles[idx],t));

        }
      }
    }

    /* update the calculation arrays for the next time step */    
    //memcpy(uo, uc, sizeof(double) * n * n);
    //memcpy(uc, un, sizeof(double) * n * n);
    temp = uo;
    uo = uc;
    uc = un;

    /* have we reached the end? */
    if(!tpdt(&t,dt,end_time)) break;
    un = temp;
  }
  /* cpy the last updated to the output array */
  memcpy(u, un, sizeof(double) * n * n);
}

/*****************************
* init_pebbles
*
* Input
* ----------
*   int pn - the number of pebbles
*   int n - the grid size
*
* Output
* ----------
*   double *p - an array (dimensioned same as the grid) that
*           gives the inital pebble size.
*
* Description
* ----------
*   init_pebbles creates a random scattering of some pn pebbles,
* along with a random size.  The range of the can be adjusted by changing
* the constant MAX_PSZ.
*
*******************************/

void init_pebbles(double *p, int pn, int n)
{
  int i, j, k, idx;
  int sz;

  srand( 10 );
  /* set to zero */
  memset(p, 0, sizeof(double) * n * n);

  for( k = 0; k < pn ; k++ )
  {
    /* the offset is to ensure that no pebbles
     * are spawned on the very edge of the grid */
    i = rand() % (n - 4) + 2;
    j = rand() % (n - 4) + 2;
    sz = rand() % MAX_PSZ;
    idx = j + i * n;
    p[idx] = (double) sz;
  }
}

/*****************************
* f
*
* Input
* ----------
*   double p -  the inital pebble value
*   double t -  the current time
* Returns
* ----------
*   the value of the "pebble" source term at time t
*
* Description
* ----------
*   Each pebbles influance on the surface will "fade" as
*   time marches forward (they may sink away, for instance).
*   This function models that - at large t ("large" defined
*   relative to the constant TSCALE) the pebble will have
*   little to no effect.
*
*   NB: this function can be updated to model whatever behavior
*   you wish the pebbles to have - they could continually jump
*   up and down on the surface, driving more energic waves, for
*   example.
******************************/
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

/*****************************
* error_u
*
* Input
* ----------
*   double *ua  -   error 1
*   double *ub  -   error 2
*   int n       -   array extent
* 
* Output
* ----------
*   double *uerr - array of errors
*       double *avgerr - pointer to the average error
*
* Description
* ----------
*   Calculates the relative error between ua and ub
*
********************************/
void error_u(double *uerr, double *avgerr, double *ua, double *ub, int n)
{
  int i, j, idx;
  
  (*avgerr) = 0.;

  for (i = 0; i < n; i++ )
  {
    for (j = 0; j < n; j++ )
    {
      idx = j + i * n;
      uerr[idx] = fabs((ua[idx]-ub[idx])/ua[idx]);
      (*avgerr) = (*avgerr) * ((double)idx/(double)(idx + 1)) + uerr[idx] / (double)(idx + 1);      
    } 
  }
}

/*****************************
* print_heatmap
*
* Input
* ----------
*   char *filename  - the output file name
*   double *u       - the array to output
*   int n           - the edge extent of u (ie, u is (n x n)
*   double h        - the step size in u
* Output
* ----------
*   None
*
* Description
* ----------
*   Outputs the array u to the file filename
********************************/
void print_heatmap(char *filename, double *u, int n, double h)
{
  char full_filename[64];
  int i, j, idx;

  dir_string(filename, full_filename);
  FILE *fp = fopen(full_filename, "w");  

  for( i = 0; i < n; i++ )
  {
    for( j = 0; j < n; j++ )
    {
      idx = j + i * n;
      fprintf(fp, "%f %f %f\n", i*h, j*h, u[idx]);
    }
  }
  
  fclose(fp);
} 
