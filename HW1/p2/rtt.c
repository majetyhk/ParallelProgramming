#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>
#include <math.h>
#include <sys/time.h>

/* This is the root process */
#define  ROOT       0

int main (int argc, char *argv[])
{

        /* process information */
        int   numproc, rank, len, source, dest;
        char *message;
        int tag;
        MPI_Status status;
        struct timeval t1, t2;
        double *elapsedTime;
        double avgelapsedTime[18] = {0};
        double stddev[18] = {0} ;
        /* current process hostname */
        //char  hostname[MPI_MAX_PROCESSOR_NAME];

        /* initialize MPI */
        MPI_Init(&argc, &argv);

        /* get the number of procs in the comm */
        MPI_Comm_size(MPI_COMM_WORLD, &numproc);

        /* get my rank in the comm */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        //tag = rank;
        int counter = 0;
        double size;
        int sizecounter = 5;
        while(sizecounter< 22){
                size = pow(2 , sizecounter);
                //printf("Before %d, rank %d\n",sizecounter,rank);
                counter = 0;
                elapsedTime = (double *)calloc(10,sizeof(double));
                while(counter<10){
                        if(rank % 2  ==0){
                                tag = rank;
                                source = dest = rank+1;

                                message = (char *) malloc(size*sizeof(char));
                                sprintf(message, "Process %d. Size %lf, Time %d!", rank, size, counter );
                                //printf("%s\n",message);
                                gettimeofday(&t1, NULL);
                                MPI_Send(message,size*sizeof(char),MPI_CHAR,dest,tag,MPI_COMM_WORLD);
                                MPI_Recv(message, size*sizeof(char), MPI_CHAR,source,tag,MPI_COMM_WORLD, &status);
                                gettimeofday(&t2, NULL);
                                elapsedTime[counter] = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
                                elapsedTime[counter] += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
                                avgelapsedTime[sizecounter-5] += elapsedTime[counter];
                                free(message);
                        }
                        else{
                                tag = rank-1;
                                source = dest = rank - 1;
                                message = (char *) malloc(size*sizeof(char));
                                MPI_Recv(message, size*sizeof(char), MPI_CHAR,source,tag,MPI_COMM_WORLD, &status);
                                MPI_Send(message,size*sizeof(char),MPI_CHAR,dest,tag,MPI_COMM_WORLD);
                                printf("%s\n",message);
                                free(message);
                        }    
                        counter++;
                }

                /*if(rank%2 == 0){
                        avgelapsedTime[sizecounter-5] = avgelapsedTime[sizecounter-5]/(double) 10;
                        for(int i = 0;i<counter;i++){
                                printf("%d\n",i);
                                stddev[sizecounter-5] += (elapsedTime[i] - avgelapsedTime[sizecounter-5])*(elapsedTime[i]-avgelapsedTime[sizecounter-5]);
                        }
                        stddev[sizecounter-5] = sqrt(stddev[sizecounter-5]/(double)10);
                        printf("Size = %lf, Average RTT = %lf, Stddev = %lf \n", size, avgelapsedTime[sizecounter-5], stddev[sizecounter-5]);
                }*/
                free(elapsedTime);
                sizecounter++;
                //printf("After %d, rank %d\n",sizecounter,rank);
        }

        /* graceful exit */
        MPI_Finalize();

}
