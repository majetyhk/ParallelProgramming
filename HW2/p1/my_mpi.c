#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "my_mpi.h"

#define MYSERVERPORT 5000
#define MYCLIENTPORT 5001

int sockfd,sockfd_root;
struct sockaddr_in my_addr,dest_addr;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

char *my_itoa(int num, char *str)
{
    if(str == NULL)
    {
        return NULL;
    }
    sprintf(str, "%d", num);
    return str;
}

int createClient()
{
	sockfd=socket(PF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		printf("Error in creating the socket\n" );
		return -1;
	}
	printf("Socket Creation Successful\n");
	dest_addr.sin_family=AF_INET;
	dest_addr.sin_port=htons(MYCLIENTPORT);
	//dest_addr.sin_addr.s_addr=INADDR_ANY;
	inet_aton("127.0.0.1", &(dest_addr.sin_addr));
	socklen_t size=sizeof(dest_addr);
	int flag=connect(sockfd,(sockaddr*)&dest_addr,size);
	if(flag==-1)
	{
		printf("Connection Failed\n");
		return -1;
	}
	return 1;
}

int createServer()
{
	sockfd=socket(PF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		printf("Error in creating the socket\n");
		return -1;
	}
	printf("Socket Creation Successful\n");
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(MYSERVERPORT);
	my_addr.sin_addr.s_addr=INADDR_ANY;
	int tmp=bind(sockfd,(sockaddr*)&my_addr,sizeof(my_addr));
	if(tmp==-1)
	{
		printf("Bind Failed\n");
		return -1;
	}
	printf("Bind Successful!\n");
	listen(sockfd,10);
	return 1;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    *rank = comm.rank;
    return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *sz)
{
    *sz = comm.size;
    return 0;
}


int MPI_Init( int *argc, char ***argv ){
	
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){

}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){

}

int MPI_Barrier( MPI_Comm comm ){

}

int MPI_Finalize( void ){
	
}

