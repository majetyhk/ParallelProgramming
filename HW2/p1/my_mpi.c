#include<stdio.h>
#include<math.h>

int MPI_Init( int *argc, char ***argv ){
	
}


int MPI_Comm_rank( MPI_Comm comm, int *rank ) {

}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){

}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){

}

int MPI_Barrier( MPI_Comm comm ){

}

int MPI_Finalize( void ){
	
}

