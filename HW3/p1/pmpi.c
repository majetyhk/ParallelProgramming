#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int *sendCount; // Declaring sendCount array global to access it within various functions to update the sendCounts
int numproc; //Total number of processes
int myRank; // Rank of the current process


int MPI_Init( int *argc, char ***argv );
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Finalize( void );

int MPI_Init(int *argc, char ***argv){
	int retVal = PMPI_Init(argc,argv);
	PMPI_Comm_size(MPI_COMM_WORLD, &numproc);
	PMPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	// Initialize the counting array
	sendCount = (int *)calloc(numproc*numproc,sizeof(int));
	return retVal;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
	//Incrementing the count
	sendCount[myRank*numproc+dest]++;
	return PMPI_Send(buf,count,datatype,dest,tag,comm);
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request){
	//Incrementing the count
	sendCount[myRank*numproc+dest]++;
	return PMPI_Isend(buf,count,datatype,dest,tag, comm,request);
}

int MPI_Finalize( void ){
	//Initialize the receiving buff to gather all the counts into the root process(0)
	int *recvBuf = (int *)calloc(numproc*numproc,sizeof(int));
	//Gather the counts
	PMPI_Gather(sendCount+(myRank*numproc) , numproc, MPI_INT, recvBuf, numproc, MPI_INT, 0, MPI_COMM_WORLD);
	if(myRank == 0){
		//Writing the Counts to a file
		FILE *fp = fopen("matrix.data", "w");
		for (int i = 0; i < numproc; ++i)
		{
			for (int j = 0; j < numproc; ++j)
			{
				fprintf(fp, "%d ", recvBuf[i*numproc+j]);
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
	return PMPI_Finalize();
}