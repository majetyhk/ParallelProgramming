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
        
        // Declare pointers to buffers which hold final Average Round Trip Time(AvgRTT) amd Std. Deviations
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
        int counter = 0;  //set counter for repeating the transmissions for 10 times
        int skipFlag = 1; // SkipFlag used to skip the first communication
        double size;
        int sizecounter = 5; 
        while(sizecounter< 22){
                size = pow(2 , sizecounter); //Modify size of the message
                //printf("Before %d, rank %d\n",sizecounter,rank);
                counter = 0;
                elapsedTime = (double *)calloc(10,sizeof(double)); // create an empty sum variables array pre-initialized with 0 using calloc.
                while(counter<10){
                        if(rank % 2  ==0){ //Even Processes send the data first while the odd processes will be in receiving mode. Then odd processes send a message of same size back
                                tag = rank;
                                source = dest = rank+1;

                                message = (char *) malloc(size*sizeof(char)); // create a message of required size. Random in this case. 
                                sprintf(message, "Process %d. Size %lf, Time %d!", rank, size, counter ); // Set the message beginning. Rest will be random.
                                //printf("%s\n",message);
                                gettimeofday(&t1, NULL);                                              //Note the time before beginning transmission
                                MPI_Send(message,size*sizeof(char),MPI_CHAR,dest,tag,MPI_COMM_WORLD); // send message to odd process
                                MPI_Recv(message, size*sizeof(char), MPI_CHAR,source,tag,MPI_COMM_WORLD, &status); // Receive return message from the odd process
                                gettimeofday(&t2, NULL);                                                // Note the time after completing the round trip transmission and calc time taken
                                elapsedTime[counter] = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
                                elapsedTime[counter] += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
                                //printf("Pair %d,MSize %lf, Count %d, ElapsedTime %lf\n",rank/2,size,counter,elapsedTime[counter]);
                                if(skipFlag!=1){
                                    avgelapsedTime[sizecounter-5] += elapsedTime[counter];    // cosnider the elapsed time only if the communication is not the first communication
                                }
                                
                                free(message);                                          // unallocate space allocated for message/delete the message
                        }
                        else{
                                tag = rank-1;
                                source = dest = rank - 1;
                                message = (char *) malloc(size*sizeof(char)); // Creating buffer for receiving the message from the even process
                                MPI_Recv(message, size*sizeof(char), MPI_CHAR,source,tag,MPI_COMM_WORLD, &status); //Receive the message
                                MPI_Send(message,size*sizeof(char),MPI_CHAR,dest,tag,MPI_COMM_WORLD); // Respond to the sender by sending a message of same size/same message itself
                                //printf("%s\n",message);
                                free(message); //unallocate of delete the message
                        }
                        if(skipFlag == 1){ //use skipflag to skip the first communication between two nodes.
                                skipFlag =0; 
                        }else{
                                counter++;
                        }    
                }

                if(rank%2 == 0){
                        
                        avgelapsedTime[sizecounter-5] = avgelapsedTime[sizecounter-5]/(double) 10; //Compute the average time
                        for(int i = 0;i<counter;i++){
                                //printf("%d\n",i);
                                stddev[sizecounter-5] += (elapsedTime[i] - avgelapsedTime[sizecounter-5])*(elapsedTime[i]-avgelapsedTime[sizecounter-5]); 
                        }
                        stddev[sizecounter-5] = sqrt(stddev[sizecounter-5]/(double)10); // compute the standard deviation
                        //printf("Size = %lf, Average RTT = %lf, Stddev = %lf \n", size, avgelapsedTime[sizecounter-5], stddev[sizecounter-5]);
                        
                }
                free(elapsedTime);
                sizecounter++;

                //printf("After %d, rank %d\n",sizecounter,rank);
        }

        
        if(rank == 0){ //Allocating space in the root process for receiving the AvgRTT and StdDev values from other processes
                avgRTTBuff = (double *)malloc((numproc)*17*sizeof(double)); 
                stddevBuff = (double *)malloc((numproc)*17*sizeof(double));
        }
        MPI_Gather(avgelapsedTime,17,MPI_DOUBLE,avgRTTBuff,17,MPI_DOUBLE,0,MPI_COMM_WORLD); // Collect values from all the processes
        MPI_Gather(stddev,17,MPI_DOUBLE,stddevBuff,17,MPI_DOUBLE,0,MPI_COMM_WORLD);

        if(rank==0){ // Due to nature of MPI_Gather, we collect from all processes though even processes is where we are calculating the values. So print only the relevant values. Skip values corresponding to odd processes.


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
