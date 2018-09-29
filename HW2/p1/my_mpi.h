

#ifndef MY_MPI_H_
#define MY_MPI_H_
#include <stdio.h>
#include <stdlib.h>

#define MPI_Datatype int	
#define MPI_DOUBLE sizeof(double)	
#define MPI_CHAR sizeof(char)	
#define MPI_Status int	
#define MAXNAMELEN 128

typedef struct mpi_Comm{	
	int rank;	
	int size;	
	char myHostName[MAXNAMELEN];
	char **hostList;	
}MPI_Comm;


MPI_Comm MPI_COMM_WORLD;	
int MPI_Comm_size ( MPI_Comm comm, int *size );	
int MPI_Comm_rank ( MPI_Comm comm, int *rank );	
int MPI_Init( int *argc, char **argv[] );	
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm); 
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status); 

#endif /* MY_MPI_H_ */
