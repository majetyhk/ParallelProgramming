#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>
#include <math.h>
#include <sys/time.h>


/* first grid point */
#define   XI              1.0
/* last grid point */
#define   XF              100.0

/* function declarations */
double     fn(double);
void        print_function_data(int, double*, double*, double*);
int         main(int, char**);

int main (int argc, char *argv[])
{
        int   numproc, rank, len, source, dest;
        char *message;
        int tag;
        MPI_Status status;
        struct timeval t1, t2;
        double *elapsedTime;
        //int *endValues;
        int startValue;
        int endValue;
        int NGRID;
        MPI_Datatype stype; 

        int *displs,*rcounts; 
        double *finalDerivBuff;
        double *tyc;
        if(argc > 1)
            NGRID = atoi(argv[1]);
        else 
        {
                printf("Please specify the number of grid points.\n");
                exit(0);
        }
        //loop index
        int         i;

        /* initialize MPI */
        MPI_Init(&argc, &argv);

        /* get the number of procs in the comm */
        MPI_Comm_size(MPI_COMM_WORLD, &numproc);

        /* get my rank in the comm */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        //domain array and step size
        double       *xc = (double *)malloc(sizeof(double)* (NGRID + 2));
        double      dx;

        //function array and derivative
        //the size will be dependent on the
        //number of processors used
        //to the program
        double     *yc, *dyc;

        //Calculate the number of points per process
        int pointsPerProcess;
        pointsPerProcess = NGRID/numproc;
        int remPoints = NGRID%numproc;

        //endValues = (int *)malloc(sizeof(int)*numproc);

        //"real" grid indices
        int         imin, imax;  

        imin = 1;
        imax = NGRID;

        //construct grid
        for (i = 1; i <= NGRID ; i++)
        {
                xc[i] = XI + (XF - XI) * (double)(i - 1)/(double)(NGRID - 1);
                //printf("XC[%d]: %lf\n ",i,xc[i]);
        }
        //step size and boundary points
        dx = xc[2] - xc[1];
        xc[0] = xc[1] - dx;
        xc[NGRID + 1] = xc[NGRID] + dx;

        

        //define the function
        startValue = rank*pointsPerProcess+1;
        if(rank+1<numproc){
            endValue = startValue + pointsPerProcess-1;
        }
        else{
            endValue = NGRID;
        }
        int totalPoints = endValue-startValue+1;
       // printf("Rank: %d, Start: %d , End: %d\n",rank,startValue,endValue);
        //endValue = pointsPerProcess+(remPoints>rank)?1:0;

        //allocate function arrays
        yc  =   (double*) malloc((NGRID + 2) * sizeof(double));
        dyc =   (double*) malloc((NGRID + 2) * sizeof(double));

        //allocate function arrays
        //tyc  =   (double*) malloc((totalPoints + 2) * sizeof(double));
        //tdyc =   (double*) malloc((totalPoints + 2) * sizeof(double));

        for( i = startValue; i <= endValue ; i++ )
        {
                yc[i] = fn(xc[i]);
                //tyc[i-startValue+1] = yc[i];
        }

        //set boundary values
        yc[imin - 1] = 0.0;
        yc[imax + 1] = 0.0;

        //NB: boundary values of the whole domain
        //should be set
        yc[0] = fn(xc[0]);
        yc[imax + 1] = fn(xc[NGRID + 1]);

        //compute the derivative using first-order finite differencing
        //
        //  d           f(x + h) - f(x - h)
        // ---- f(x) ~ --------------------
        //  dx                 2 * dx
        //
        //printf("Yc Value  %lf",yc[51] );
        if(rank%2==0){
            tag = rank;
            if(rank+1<numproc){
                MPI_Recv(yc + (endValue+1),1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD,&status);
                //printf("Recv_Rank:%d, Ind:%d, %lf\n",rank,endValue+1,yc[endValue+1] );
            }
            if(rank!=0){
                //yc[51] = 2601;
                //printf("YC Value Recv %lf %lf %lf %lf %lf\n",yc[49] ,yc[50] ,yc[51] ,yc[52] ,yc[53]);
                MPI_Recv(yc + (startValue-1),1, MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&status);
                //printf("Recv_Rank:%d, Ind:%d, %lf\n",rank,startValue-1,yc[startValue-1] );
                //printf("YC Value Recv %lf %lf %lf %lf %lf\n",yc[49] ,yc[50] ,yc[51] ,yc[52] ,yc[53]);
            }
            if(rank+1<numproc){
                MPI_Send(yc + endValue,1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD);
                //printf("Send_Rank:%d, Ind:%d, %lf\n",rank,endValue,yc[endValue] );
            }
            if(rank!=0){
                MPI_Send(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD);
                //printf("Yc Value send %lf",yc[51] );
                //printf("Send_Rank:%d, Ind:%d, %lf\n",rank,startValue,yc[startValue] );
            }
            // for( i = startValue; i <= endValue ; i++ )
            // {
            //     printf(" y value %lf %lf \n", yc[i], xc[i]);
            // }
        }
        else{
            tag = rank-1;
            
            MPI_Send(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD);
                //printf("Send_Rank:%d, Ind:%d, %lf\n",rank,startValue,yc[startValue] );

            if(rank+1<numproc){
                MPI_Send(yc + endValue,1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD);
                //printf("Send_Rank:%d, Ind:%d, %lf\n",rank,endValue,yc[endValue] );
            }
            if(rank!=0){
                MPI_Recv(yc +(startValue-1),1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&status);
                //printf("Recv_Rank:%d, Ind:%d, %lf\n",rank,startValue-1,yc[startValue-1] );
            }
            if(rank+1<numproc){
                MPI_Recv(yc + (endValue + 1),1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD,&status);
                //printf("Recv_Rank:%d, Ind:%d, %lf\n",rank,endValue+1,yc[endValue+1] );
            }  
            
        }

        
        for (i = startValue; i <= endValue; i++)
        {
                dyc[i] = (yc[i + 1] - yc[i - 1])/(2.0 * dx);
                //printf("i =%d, dyc = %lf\n",i,dyc[i] );
        }
        if(rank==0){
            finalDerivBuff = (double*) malloc((NGRID + 2) * sizeof(double));
            tyc = (double*) malloc((NGRID + 2) * sizeof(double));
            displs = (int *)malloc(numproc*sizeof(int)); 
            rcounts = (int *)malloc(numproc*sizeof(int));
            for (i=0; i<numproc; ++i) { 
                displs[i] = i*pointsPerProcess;
                if(i==numproc-1){
                    rcounts[i] = pointsPerProcess+remPoints;
                }
                else{
                    rcounts[i] = pointsPerProcess;
                }
                //printf("i %d, Disp %d, RCount %d\n",i,displs[i],rcounts[i] );  
            }
        }
        //finalDerivBuff = (double*) malloc((NGRID + 2) * numproc * sizeof(double));

        //MPI_Type_vector( rcounts[rank], rcounts[rank], , MPI_DOUBLE, &stype); 
        //MPI_Type_commit( &stype ); 
        double *k;
        k=dyc+startValue;
        //printf("Rank: %d At Start Value: %lf\n",rank,k[0]);
        MPI_Gatherv(dyc+startValue,totalPoints,MPI_DOUBLE,finalDerivBuff,rcounts,displs,MPI_DOUBLE, 0,MPI_COMM_WORLD);
        MPI_Gatherv(yc+startValue,totalPoints,MPI_DOUBLE,tyc,rcounts,displs,MPI_DOUBLE,0,MPI_COMM_WORLD);
        
        /*if(rank==0){
            for (int i = 0; i < (NGRID+1); ++i)
            {
                printf("%d, %lf\n",i,finalDerivBuff[i] );
            }
        }*/
        /*int k = 0;
        int l =0;
        for( k = 0; k< numproc-1 ; k++){
            int start = k*pointsPerProcess +1;
            int end;
            if(k==numproc-1){
                end = NGRID;
            }
            else{
                end = start + pointsPerProcess;
            }
            for( l=start; l<end; l++){
                dyc[l] = finalDerivBuff[(NGRID+2)*k+start+l];
            }
        }*/
        if(rank==0){
            print_function_data(NGRID, &xc[1], &tyc[0], &finalDerivBuff[0]);
        }
        


        //free allocated memory 
        free(yc);
        free(dyc);


        /* graceful exit */
        MPI_Finalize();

        return 0;
}

//prints out the function and its derivative to a file
void print_function_data(int np, double *x, double *y, double *dydx)
{
        int   i;

        char filename[1024];
        sprintf(filename, "fn-%d.dat", np);

        FILE *fp = fopen(filename, "w");

        for(i = 0; i < np; i++)
        {
                printf("%f %f %f\n", x[i], y[i], dydx[i]);
                fprintf(fp, "%f %f %f\n", x[i], y[i], dydx[i]);
        }

        fclose(fp);
}
