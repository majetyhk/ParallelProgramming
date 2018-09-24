/* Program to compute Pi using Monte Carlo methods */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define SEED 35791246


int main(int argc, char** argv)

{

   int niter=0;
   double x,y;
   int i,count=0; /* # of points in the 1st quadrant of unit circle */
   double z;
   double pi;


    niter = atoi(argv[1]);


   /* initialize random numbers */

   srand(SEED);
   count=0;

   for ( i=0; i<niter; i++) {

      x = (double)rand()/RAND_MAX;
      y = (double)rand()/RAND_MAX;
      z = x*x+y*y;
      if (z<=1) count++;

   }

   pi=(double)count/niter*4;
   printf("# of trials= %d , estimate of pi is %.16f \n",niter,pi);

}
