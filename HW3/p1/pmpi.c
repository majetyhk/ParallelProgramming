#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int *sendCount;
int numproc;
int myRank;


int MPI_Init( int *argc, char ***argv );
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Finalize( void );

int MPI_Init(int *argc, char ***argv){
	int retVal = PMPI_Init(argc,argv);
	PMPI_Comm_size(MPI_COMM_WORLD, &numproc);
	PMPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	sendCount = (int *)calloc(numproc*numproc,sizeof(int));
	return retVal;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
	sendCount[myRank*numproc+dest]++;
	return PMPI_Send(buf,count,datatype,dest,tag,comm);
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request){
	sendCount[myRank*numproc+dest]++;
	return PMPI_Isend(buf,count,datatype,dest,tag, comm,request);
}

int MPI_Finalize( void ){
	int *recvBuf = (int *)calloc(numproc*numproc,sizeof(int));
	PMPI_Gather(sendCount+(myRank*numproc) , numproc, MPI_INT, recvBuf, numproc, MPI_INT, 0, MPI_COMM_WORLD);
	if(myRank == 0){
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