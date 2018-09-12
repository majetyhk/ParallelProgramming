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
        MPI_Status status,status2;
  //      struct timeval t1, t2;
        double *elapsedTime;
        int startValue;
        int endValue;
        int NGRID,communication,mpi_gather;
        MPI_Datatype stype;
        MPI_Request req[10]; 
        struct timeval t1,t2;
        int *displs,*rcounts; 
        double *finalDerivBuff;
        double *tyc;
        if(argc > 1)
        {
            NGRID = atoi(argv[1]);
        }
            if(argc>2){
            communication = atoi(argv[2]);
            //printf("NGRID: %d Comm Value %d\n", NGRID, communication);
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
        gettimeofday(&t1,NULL);

        if(communication == 0)
        {
            //printf("Entered into Comm value 0 case\n");
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
        else
        {
            //printf("Entered into Comm value 1 case with Rank %d\n",rank);
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
        if(communication !=0)
        {
            //printf("Stopping Here before proceeding with Rank %d\n",rank);
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
        if(mpi_gather==1 && communication == 0)
        {
            //printf("\n******** manual gather *********\n");
        	if(rank>0){
        	    MPI_Send(yc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD);
        	    MPI_Send(dyc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD);
        	}
            if(rank==0) 
            {
        	    double* all_arrays = (double*)malloc(NGRID*sizeof(double));
        	    double* all_arrays_yc = (double*)malloc(NGRID*sizeof(double));
                for(int i=startValue;i<=endValue;i++)
        	    {
        		    all_arrays[i]=dyc[i];
        		    all_arrays_yc[i]=yc[i];
        	    }

        	    for (int i=1; i<numproc-1; i++)
                {
        		    MPI_Recv(&all_arrays_yc[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&status);
        		    MPI_Recv(&all_arrays[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&status);
        	    }
                MPI_Recv(&all_arrays_yc[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&status);
        		MPI_Recv(&all_arrays[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&status);
        	    print_function_data(NGRID, &xc[1], &all_arrays_yc[1], &all_arrays[1]);
        	}
        }
        else if(mpi_gather==1 && communication == 1)
        {
           //printf("\n******** manual gather with non blocking function calls *********\n");
        	if(rank>0){
        	    MPI_Isend(yc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD,&req[8]);
        	    MPI_Isend(dyc+startValue,endValue-startValue+1,MPI_DOUBLE,0,50,MPI_COMM_WORLD,&req[9]);
        	}
            if(rank==0) 
            {
        	    double* all_arrays = (double*)malloc(NGRID*sizeof(double));
        	    double* all_arrays_yc = (double*)malloc(NGRID*sizeof(double));
                for(int i=startValue;i<=endValue;i++)
        	    {
        		    all_arrays[i]=dyc[i];
        		    all_arrays_yc[i]=yc[i];
        	    }
                MPI_Request req2[numproc], req3[numproc];
        	    for (int i=1; i<numproc-1; i++)
                {
        		    MPI_Irecv(&all_arrays_yc[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&req2[i]);
        		    MPI_Irecv(&all_arrays[endValue+1+((i-1)*(endValue-startValue+1))],endValue-startValue+1,MPI_DOUBLE,i,50,MPI_COMM_WORLD,&req3[i]);
        	    }
                MPI_Irecv(&all_arrays_yc[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&req2[numproc-1]);
        		MPI_Irecv(&all_arrays[((numproc-1)*(endValue-startValue+1))+1],NGRID-((numproc-1)*startValue)+1,MPI_DOUBLE,numproc-1,50,MPI_COMM_WORLD,&req3[numproc-1]);
        	    
                for(int i=1; i<numproc;i++)
                {
                    MPI_Wait(&req2[i],&status);
                    MPI_Wait(&req3[i],&status);
                }
        	    print_function_data(NGRID, &xc[1], &all_arrays_yc[1], &all_arrays[1]);
        	} 
        }
        else
        {
            //printf("\n********* MPI_GATHER *******\n");
            if(rank==0)
            {
                finalDerivBuff = (double*) malloc((NGRID + 2) * sizeof(double));
                tyc = (double*) malloc((NGRID + 2) * sizeof(double));
                displs = (int *)malloc(numproc*sizeof(int)); 
                rcounts = (int *)malloc(numproc*sizeof(int));
                for (i=0; i<numproc; ++i) 
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
//                printf("%f %f %f\n", x[i], y[i], dydx[i]);
            fprintf(fp, "%f, %f, %f\n", x[i], y[i], dydx[i]);
        }

        fclose(fp);
}
