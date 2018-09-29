#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "my_mpi.h"

int sockfd,sockfd_root; 

char *numToString(int num, char *str)
{
	if(str == NULL)
	{
		return NULL;
	}
	sprintf(str, "%d", num);
	return str;
}


int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    *rank = comm.rank;
    return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
    *size = comm.size;
    return 0;
}


int createSocket(char* fname){
    struct sockaddr_in serv_addr, my_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);	
    if(sock==-1)
    {
        printf("Error in creating the socket\n" );
        exit(0);
    }
    int on = 1;
    setsockopt(sock, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));	
    
    fflush(stdin);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    unsigned short portno = 0;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0;
    if (bind(sock, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){	
        printf("Bind Failed!\n" );
        exit(0);
    }
    listen(sock,5);	
    
    int len = sizeof(my_addr);
    if(getsockname(sock, (struct sockaddr *) &my_addr, &len)<0){
        printf("Error getting socket info\n" );
        exit(0);
    }
    int myPort = ntohs(my_addr.sin_port);
    FILE *file = fopen(fname, "w");
    fprintf(file, "%d",myPort);
    fclose(file);
    return sock;
}


int MPI_Init(int *argc, char **argv[] )
{
    int n;
    char **argv_param = *argv;
    
    fflush(stdin);
    struct sockaddr_in serv_addr;
    int numProc = atoi(argv_param[3]);

    MPI_COMM_WORLD.size = numProc;  
    MPI_COMM_WORLD.rank = atoi(argv_param[1]);
    strcpy(MPI_COMM_WORLD.myHostName,argv_param[4]);

    MPI_COMM_WORLD.hostList = (char **)malloc(numProc * sizeof(char*));
    for(int i = 0; i <numProc; i++){
        MPI_COMM_WORLD.hostList[i] = (char *)malloc(numProc * MAXNAMELEN * sizeof(char));
    }
    
    fflush(stdin);
    strcat(argv_param[2],"/nodefile.txt");	
    FILE *fp = fopen(argv_param[2], "r");
    if(fp == NULL){
    	printf("No hosts\n");
    	exit(0);
    }
    for(int i = 0; i< numProc; i++){
        fscanf(fp, "%s", MPI_COMM_WORLD.hostList[i]);	
    }
    char fname[20];
    numToString(MPI_COMM_WORLD.rank,fname);
    strcat(fname,".txt");
    sockfd = createSocket(fname);	
    return 0;
}


int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm)
{
    int receiverPort;
    struct hostent *receiver;
    struct sockaddr_in client_addr;
    
    FILE *file;    
    receiver = gethostbyname(comm.hostList[dest]);	
    if (receiver == NULL) {
        printf("No host %s found\n", comm.hostList[dest]);
        exit(0);
    }

    char fname[30]; 
    numToString(dest,fname);
    strcat(fname,".txt");	
    file = fopen(fname, "r");
    while(file == NULL)
    {
        file = fopen(fname, "r");
    }
    fscanf(file, "%d", &receiverPort);	
    fclose(file);

    bzero((char *) &client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    bcopy( (char *)receiver->h_addr, (char *)&client_addr.sin_addr.s_addr,receiver->h_length);
    client_addr.sin_port = htons(receiverPort);
    fflush(stdin);
    int sendingSocket = socket(AF_INET, SOCK_STREAM, 0);	
    if (sendingSocket < 0) {
        printf("Error in creating the socket\n" );
        exit(0);   
    }
    int flag = connect(sendingSocket,(struct sockaddr *) &client_addr,sizeof(client_addr));
    if ( flag < 0) {	
        printf("Error in connecting to the client\n" );
        exit(0);
    }
    flag = send(sendingSocket,buf,datatype * count,0);	
    if (flag < 0) {
        printf("Unable to send to %s from %s\n", comm.hostList[dest],comm.myHostName );
        exit(0);
    }
    fflush(stdin);
    return 0;
}


int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
     
     socklen_t clilen;
     struct sockaddr_in sender_addr;
     
     clilen = sizeof(sender_addr);
     int newsockfd;
     newsockfd = accept(sockfd, (struct sockaddr *) &sender_addr , &clilen);	
     
     if (newsockfd < 0) {
        printf("Error accepting connection from %s on %s\n", comm.hostList[source],comm.myHostName );
        exit(0);
     }
     int flag= recv(newsockfd,buf,count*datatype,MSG_WAITALL);	
     if (flag < 0){
        printf("Error receiving from %s on %s\n", comm.hostList[source],comm.myHostName );
        exit(0);
     }
     fflush(stdin);     
     close(newsockfd);
     return 0;
}

