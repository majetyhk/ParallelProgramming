/*
Single Author info:
hmajety  Hari Krishna Majety
Group info:
hmajety  Hari Krishna Majety
srout Sweta Rout
mreddy2 Harshavardhan Reddy Muppidi
*/

#ifndef MY_MPI_H_
#define MY_MPI_H_
#include <stdio.h>
#include <stdlib.h>

#define MPI_Datatype int	
#define MPI_DOUBLE 8
#define MPI_CHAR 1	
#define MPI_Status int	
#define MAXNAMELEN 128

typedef struct mpi_Comm{	
	int rank;	
	int size;
        int gather;	
 	int sockfd;
	int sockfd_gather;
	int sockfd_gather1;
	char myHostName[MAXNAMELEN];
	char **hostList;	
}MPI_Comm;


MPI_Comm MPI_COMM_WORLD;	
int MPI_Comm_size ( MPI_Comm comm, int *size );	
int MPI_Comm_rank ( MPI_Comm comm, int *rank );	
int MPI_Init( int *argc, char **argv[] );	
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm); 
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status); 
int MPI_Gather(void *buf,int count, MPI_Datatype datatype, void *res, int gCount, MPI_Datatype gdatatype,int root, MPI_Comm comm);
int MPI_Finalize(MPI_Comm comm);
int createSocket(int rank,int gather);
char *numToString(int num, char *str);
#endif /* MY_MPI_H_ */
