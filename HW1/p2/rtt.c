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
        double *avgRTTBuff;
        double *stddevBuff;
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
        int escapeFlag = 1;
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
                                //printf("Pair %d,MSize %lf, Count %d, ElapsedTime %lf\n",rank/2,size,counter,elapsedTime[counter]);
                                if(!(sizecounter==5 && escapeFlag==1)){
                                    avgelapsedTime[sizecounter-5] += elapsedTime[counter];
                                    counter++;    
                                }else{
                                        //printf("Ignored %lf\n",elapsedTime[counter]);
                                        escapeFlag = 0;
                                        free(message);
                                        //continue;
                                }
                                
                                free(message);
                        }
                        else{
                                tag = rank-1;
                                source = dest = rank - 1;
                                message = (char *) malloc(size*sizeof(char));
                                MPI_Recv(message, size*sizeof(char), MPI_CHAR,source,tag,MPI_COMM_WORLD, &status);
                                MPI_Send(message,size*sizeof(char),MPI_CHAR,dest,tag,MPI_COMM_WORLD);
                                //printf("%s\n",message);
                                free(message);
                                if(!(sizecounter==5 && escapeFlag==1)){
                                        counter++;
                                }else{
                                        escapeFlag = 0;
                                }
                                
                        }    
                        
                }

                if(rank%2 == 0){
                        /*if(sizecounter == 5){
                                avgelapsedTime[sizecounter-5] = avgelapsedTime[sizecounter-5]/(double) 9;
                                for(int i = 1;i<counter;i++){
                                        //printf("%d\n",i);
                                        stddev[sizecounter-5] += (elapsedTime[i] - avgelapsedTime[sizecounter-5])*(elapsedTime[i]-avgelapsedTime[sizecounter-5]);
                                }
                                stddev[sizecounter-5] = sqrt(stddev[sizecounter-5]/(double)9);
                        }
                        else{
                                avgelapsedTime[sizecounter-5] = avgelapsedTime[sizecounter-5]/(double) 10;
                                for(int i = 0;i<counter;i++){
                                        //printf("%d\n",i);
                                        stddev[sizecounter-5] += (elapsedTime[i] - avgelapsedTime[sizecounter-5])*(elapsedTime[i]-avgelapsedTime[sizecounter-5]);
                                }
                                stddev[sizecounter-5] = sqrt(stddev[sizecounter-5]/(double)10);
                        }*/
                        avgelapsedTime[sizecounter-5] = avgelapsedTime[sizecounter-5]/(double) 10;
                        for(int i = 0;i<counter;i++){
                                //printf("%d\n",i);
                                stddev[sizecounter-5] += (elapsedTime[i] - avgelapsedTime[sizecounter-5])*(elapsedTime[i]-avgelapsedTime[sizecounter-5]);
                        }
                        stddev[sizecounter-5] = sqrt(stddev[sizecounter-5]/(double)10);
                        
                        //printf("Size = %lf, Average RTT = %lf, Stddev = %lf \n", size, avgelapsedTime[sizecounter-5], stddev[sizecounter-5]);
                        
                }
                free(elapsedTime);
                sizecounter++;

                //printf("After %d, rank %d\n",sizecounter,rank);
        }

        
        if(rank == 0){
                avgRTTBuff = (double *)malloc((numproc)*17*sizeof(double));
                stddevBuff = (double *)malloc((numproc)*17*sizeof(double));
        }
        MPI_Gather(avgelapsedTime,17,MPI_DOUBLE,avgRTTBuff,17,MPI_DOUBLE,0,MPI_COMM_WORLD);
        MPI_Gather(stddev,17,MPI_DOUBLE,stddevBuff,17,MPI_DOUBLE,0,MPI_COMM_WORLD);

        if(rank==0){
                /*printf("'Pair','MessageSize','AvgRTT','Stddev'\n");
                for(int i = 0;i<(numproc)*17;i++){
                        if((i/17)%2==0){
                                printf("%d, %lf, %lf, %lf\n",((i/17)/2),pow(2, (i%17)+5 ),avgRTTBuff[i],stddevBuff[i]);
                        }
                        
                }*/
                printf("'MessageSize','AvgRTT1','Stddev1','AvgRTT2','Stddev2','AvgRTT3','Stddev3','AvgRTT4','Stddev4'\n");
                for (int i = 0; i < 17; ++i)
                {
                        /* code */
                        printf("%lf , %lf, %lf , %lf, %lf, %lf, %lf, %lf, %lf \n",pow(2, (i%17)+5 ) ,avgRTTBuff[i],stddevBuff[i], avgRTTBuff[i+17*2],stddevBuff[i+17*2], avgRTTBuff[i+17*4],stddevBuff[i+17*4], avgRTTBuff[i+17*6],stddevBuff[i+17*6]);
                }
        }
        /* graceful exit */
        MPI_Finalize();

}
