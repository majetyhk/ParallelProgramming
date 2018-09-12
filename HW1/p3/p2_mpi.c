/*
Single Author info:
hmajety  Hari Krishna Majety
Group info:
hmajety  Hari Krishna Majety
srout Sweta Rout
mreddy2 Harshavardhan Reddy Muppidi
*/

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
        MPI_Status status;                    // Status for Receive Calls
        int startValue, endValue;             // For Defining Processes End Grid Points
        int NGRID,communication,mpi_gather;   // For storing the Argument Values
        MPI_Datatype stype;
        MPI_Request req[10];                  // Request Handlers for Non-Blocking Calls
        struct timeval t1,t2;                 // Time Variables for Performance Monitoring  
        int *displs,*rcounts;                 // Displacement and Count Buffer used under MPI_Gather
        double *finalDerivBuff;               // Final Derivate Buffer Used under MPI_Gather

        // Assigning the argument Values
        if(argc > 1)
        {
            NGRID = atoi(argv[1]);
        }
            if(argc>2){
            communication = atoi(argv[2]);
        }
         if(argc > 3){
        mpi_gather = atoi(argv[3]);
        }
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
        }
        //step size and boundary points
        dx = xc[2] - xc[1];
        xc[0] = xc[1] - dx;
        xc[NGRID + 1] = xc[NGRID] + dx;

        

        //Allocating the Grid Points(Start & End) based on the rank of the Process
        startValue = rank*pointsPerProcess+1;
        if(rank+1<numproc){
            endValue = startValue + pointsPerProcess-1;
        }
        else{
            endValue = NGRID;
        }
        int totalPoints = endValue-startValue+1;

        //allocate function arrays
        yc  =   (double*) malloc((NGRID + 2) * sizeof(double));
        dyc =   (double*) malloc((NGRID + 2) * sizeof(double));

        //Calculating the fn(x) values and allocating it to yc
        for( i = startValue; i <= endValue ; i++ )
        {
                yc[i] = fn(xc[i]);
        }

        //set boundary values
        yc[imin - 1] = 0.0;
        yc[imax + 1] = 0.0;

        //NB: boundary values of the whole domain
        //should be set
        yc[0] = fn(xc[0]);
        yc[imax + 1] = fn(xc[NGRID + 1]);
       
        gettimeofday(&t1,NULL);  // Getting Time for Performance Analysis



        if(communication == 0)              // If Border Communication is to be done with Blocking Calls(Communication Logic in README)
        {
                if(rank%2==0)               
                {
                    tag = rank;
                    if(rank+1<numproc){
                        MPI_Recv(yc + (endValue+1),1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD,&status);
                    }
                    if(rank!=0){
                        MPI_Recv(yc + (startValue-1),1, MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&status);
                    }
                    if(rank+1<numproc){
                        MPI_Send(yc + endValue,1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD);
                    }
                    if(rank!=0){
                        MPI_Send(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD);
                    }
                }
                else
                {
                    tag = rank-1;
                    MPI_Send(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD);
                    if(rank+1<numproc){
                        MPI_Send(yc + endValue,1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD);
                    }
                    if(rank!=0){
                        MPI_Recv(yc +(startValue-1),1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&status);
                    }
                    if(rank+1<numproc){
                        MPI_Recv(yc + (endValue + 1),1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD,&status);
                    }  
                    
                }
        }
        else                                // If Border Communication is to be done with Non-Blocking Calls(Communication Logic in README)
        {
                if(rank%2==0){
                    tag = rank;
                    if(rank+1<numproc){
                        MPI_Irecv(yc + (endValue+1),1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD,&req[0]);
                    }
                    if(rank!=0){
                        MPI_Irecv(yc + (startValue-1),1, MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&req[1]);
                    }
                    if(rank+1<numproc){
                        MPI_Isend(yc + endValue,1,MPI_DOUBLE,rank+1,tag,MPI_COMM_WORLD,&req[2]);
                    }
                    if(rank!=0){
                        MPI_Isend(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&req[3]);
                    }
                }
                else{           
                    tag = rank-1;
                    MPI_Isend(yc + startValue,1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&req[4]);
                    if(rank+1<numproc){
                        MPI_Isend(yc + endValue,1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD,&req[5]);
                    }
                    if(rank!=0){
                        MPI_Irecv(yc +(startValue-1),1,MPI_DOUBLE,rank-1,tag,MPI_COMM_WORLD,&req[6]);
                    }
                    if(rank+1<numproc){
                        MPI_Irecv(yc + (endValue + 1),1,MPI_DOUBLE,rank+1,rank+1,MPI_COMM_WORLD,&req[7]);
                    }  
                    
                }
        }
        if(communication !=0)           // Incase of Non-Blocking Calls, Waiting for all the processes to complete communication 
        {
            if(rank%2==0)
            {
                tag = rank;
                if(rank+1<numproc){
                    MPI_Wait(&req[0],&status);
                }
                if(rank!=0){
                    MPI_Wait(&req[1],&status);
            }
            }
            else{
                    tag = rank-1;
                    if(rank!=0){
                        MPI_Wait(&req[6],&status);
                    }
                    if(rank+1<numproc){
                        MPI_Wait(&req[7],&status);
                    }  
                    
                }
        }

        //compute the derivative using first-order finite differencing       
        for (i = startValue; i <= endValue; i++)
        {
            dyc[i] = (yc[i + 1] - yc[i - 1])/(2.0 * dx);
        }
        if(communication == 0 && mpi_gather==1)         // Incase of Manual Gather Using Blocking Calls                    
        {
            if(rank>0){                                 // All Processes Sending the values over to Rank 0 Process
                MPI_Send(yc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD);
                MPI_Send(dyc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD);
            }
            if(rank==0)                                  //Allocating Arrays to store all the values to be received from all the processes
            {
                double* all_arrays = (double*)malloc(NGRID*sizeof(double));
                double* all_arrays_yc = (double*)malloc(NGRID*sizeof(double));
                for(int i=startValue;i<=endValue;i++)   // Storing It's Own(Rank 0) computed values into the main array
                {
                    all_arrays[i]=dyc[i];
                    all_arrays_yc[i]=yc[i];
                }

                for (int i=1; i<numproc-1; i++)         // Issuing Recv Calls for all the processes except the last one
                {
                    MPI_Recv(&all_arrays_yc[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&status);
                    MPI_Recv(&all_arrays[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&status);
                }
                                                        // Receive calls for the last Process since it has different grid points than all.
                MPI_Recv(&all_arrays_yc[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&status);
                MPI_Recv(&all_arrays[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&status);
                print_function_data(NGRID, &xc[1], &all_arrays_yc[1], &all_arrays[1]);
            }
        }
        else if(mpi_gather==1 && communication == 1)           // Incase of Manual Gather using Non-Blocking Calls
        {
            if(rank>0)                                        // All Processes Sending the values over to Rank 0 Process
            {
                MPI_Isend(yc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD,&req[8]);
                MPI_Isend(dyc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD,&req[9]);
            }

            if(rank==0)                                        //Allocating Arrays to store all the values to be received from all the processes
            {
                double* all_arrays = (double*)malloc(NGRID*sizeof(double));
                double* all_arrays_yc = (double*)malloc(NGRID*sizeof(double));

                for(int i=startValue;i<=endValue;i++)           // Storing It's Own(Rank 0) computed values into the main array
                {
                    all_arrays[i]=dyc[i];
                    all_arrays_yc[i]=yc[i];
                }
                MPI_Request req2[numproc], req3[numproc];
                for (int i=1; i<numproc-1; i++)                 // Issuing Irecv Calls for all the processes except the last one
                {
                    MPI_Irecv(&all_arrays_yc[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&req2[i]);
                    MPI_Irecv(&all_arrays[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&req3[i]);
                }

                                                                // Receive calls for the last Process since it has different grid points than all. 
                MPI_Irecv(&all_arrays_yc[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&req2[numproc-1]);
                MPI_Irecv(&all_arrays[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&req3[numproc-1]);
                
                for(int i=1; i<numproc;i++)                     // Waiting for all the Processes to complete the communications.
                {
                    MPI_Wait(&req2[i],&status);
                    MPI_Wait(&req3[i],&status);
                }
                print_function_data(NGRID, &xc[1], &all_arrays_yc[1], &all_arrays[1]);
            } 
        }
        else                        // Incase of MPI_Gather
        {
            
            if(rank==0)
            {
                finalDerivBuff = (double*) malloc((NGRID + 2) * sizeof(double));       // Final Buffer storing all the derivative values
                tyc = (double*) malloc((NGRID + 2) * sizeof(double));                  // Buffer to store all the Function(y) values
                displs = (int *)malloc(numproc*sizeof(int));                           // Displacement Array for the buffer, to know where to place the gathered values 
                rcounts = (int *)malloc(numproc*sizeof(int));                          // Count to define how many values to be gathered from respective process
                
                for (i=0; i<numproc; ++i)                           // Defining the Displacement and Count Array Values
                { 
                    displs[i] = i*pointsPerProcess;
                    if(i==numproc-1)
                    {
                        rcounts[i] = pointsPerProcess+remPoints;
                    }
                    else
                    {
                        rcounts[i] = pointsPerProcess;
                    }
                }
            }  
            double *k;                                            
            k=dyc+startValue;
            // Gathering all the values to the Rank 0 Process
            MPI_Gatherv(dyc+startValue,totalPoints,MPI_DOUBLE,finalDerivBuff,rcounts,displs,MPI_DOUBLE, 0,MPI_COMM_WORLD);
            MPI_Gatherv(yc+startValue,totalPoints,MPI_DOUBLE,tyc,rcounts,displs,MPI_DOUBLE,0,MPI_COMM_WORLD);
            if(rank==0)
            {
                print_function_data(NGRID, &xc[1], &tyc[0], &finalDerivBuff[0]);
            }
        }

        gettimeofday(&t2,NULL);
        double timeInterval = ((t2.tv_sec - t1.tv_sec) * 1000.0);
        timeInterval += (t2.tv_usec - t1.tv_usec) / 1000.0;
       
        if(rank==0){
            printf("\nTime taken = %lf\n",timeInterval);
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
            fprintf(fp, "%f, %f, %f\n", x[i], y[i], dydx[i]);
        }

        fclose(fp);
}
