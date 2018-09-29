

#ifndef MY_MPI_H_
#define MY_MPI_H_
#include <stdio.h>
#include <stdlib.h>

#define MPI_Datatype int	//MPI_Datatype
#define MPI_DOUBLE sizeof(double)	//corresponding to double
#define MPI_CHAR sizeof(char)	//corresponding to char
#define MPI_Status int	//status of a send or recv call
#define MAXNAMELEN 128

typedef struct mpi_Comm{	//Structure for MPI_Comm
	int rank;	//rank of the process
	int size;	//number of processes
	char myHostName[MAXNAMELEN];
	char **hostList;	//all hostnames
}MPI_Comm;


MPI_Comm MPI_COMM_WORLD;	//for all processes
int MPI_Comm_size ( MPI_Comm comm, int *size );	//returns world size
int MPI_Comm_rank ( MPI_Comm comm, int *rank );	//returns process rank
int MPI_Init( int *argc, char **argv[] );	//initialises MPI_Comm and creates server sockets
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm); //sends data to a dest
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status); //receives fata from a source
int MPI_Gather(void* buf, int count, MPI_Datatype datatype,void *gather_buf, int gather_count,MPI_Datatype gather_datatype, int root, MPI_Comm comm); //gathers data for all processes in a root process sequentially
int MPI_Barrier(MPI_Comm comm);	//checks if all processes have completed their task
int MPI_Finalize(MPI_Comm comm);	//cleansup the work directory only when all processes have completed their tasks


#endif /* MY_MPI_H_ */
