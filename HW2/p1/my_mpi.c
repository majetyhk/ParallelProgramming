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
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "my_mpi.h"


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


int createSocket(int rank,int gather){
    struct sockaddr_in serv_addr, my_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);	//Create socket
    if(sock==-1)
    {
        printf("Error in creating the socket\n" );
        exit(0);
    }
    int on = 1;
    setsockopt(sock, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));	// Set socket to reuse same address
    
    fflush(stdin);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    unsigned short portno = 0;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0;
    if (bind(sock, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){	//Configure the socket to a port
        printf("Bind Failed!\n" );
        exit(0);
    }
    listen(sock,5);	// Listening for connections
    
    int len = sizeof(my_addr);
    if(getsockname(sock, (struct sockaddr *) &my_addr, &len)<0){ // Get the port number assigned to the socket
        printf("Error getting socket info\n" );
        exit(0);
    }
    char fname[20];
    numToString(MPI_COMM_WORLD.rank,fname);
    if(gather == 1){
	 strcat(fname,"gather.txt");
   }else if(gather == 2){
	strcat(fname,"gather1.txt");
    }else{
    strcat(fname,".txt");
    }
    int myPort = ntohs(my_addr.sin_port); 
    FILE *file = fopen(fname, "w");
    fprintf(file, "%d",myPort); //Write the port number to a file
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

    MPI_COMM_WORLD.size = numProc;  // Read the num of nodes participating parameter passed as command line argument
    MPI_COMM_WORLD.rank = atoi(argv_param[1]); //Read in the rank of the node
    strcpy(MPI_COMM_WORLD.myHostName,argv_param[4]); // Read the node name passed
    MPI_COMM_WORLD.gather = 0;
    MPI_COMM_WORLD.hostList = (char **)malloc(numProc * sizeof(char*)); //Allocate memory for fetching node names
    char* host = (char *)malloc(numProc * MAXNAMELEN * sizeof(char));
    for(int i = 0; i <numProc; i++){
        MPI_COMM_WORLD.hostList[i] = &host[i * MAXNAMELEN];
    }
    fflush(stdin);
    strcat(argv_param[2],"/nodefile.txt");	//Open file to read the node names
    FILE *fp = fopen(argv_param[2], "r");
    if(fp == NULL){
    	printf("No hosts\n");
    	exit(0);
    }
    for(int i = 0; i< numProc; i++){
        fscanf(fp, "%s", MPI_COMM_WORLD.hostList[i]);	//Read node names into an array
    }
    MPI_COMM_WORLD.sockfd = createSocket(MPI_COMM_WORLD.rank,0);	//Create a socket to listen for connections
    if(MPI_COMM_WORLD.rank == 0){
	MPI_COMM_WORLD.sockfd_gather = createSocket(MPI_COMM_WORLD.rank,1);
        MPI_COMM_WORLD.sockfd_gather1 = createSocket(MPI_COMM_WORLD.rank,2);
    }else{
	MPI_COMM_WORLD.sockfd_gather = -1;
	MPI_COMM_WORLD.sockfd_gather1 = -1;
    }
    return 0;
}


int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm)
{
    
    int receiverPort;
    struct hostent *receiver;
    struct sockaddr_in client_addr;
    
    FILE *file;    
    receiver = gethostbyname(comm.hostList[dest]);	//Get IP address of the host by it's name
    if (receiver == NULL) {
        printf("No host %s found\n", comm.hostList[dest]);
        exit(0);
    }

    char fname[30]; 
    numToString(dest,fname);
    if(comm.gather == 1 && dest == 0){
  //      printf("\n gather send \n");
	strcat(fname,"gather.txt");
    }else if(comm.gather == 2 && dest == 0){
	strcat(fname,"gather1.txt");
}else{
    strcat(fname,".txt");	
}
    file = fopen(fname, "r");
    while(file == NULL)
    {
        file = fopen(fname, "r"); // Get the port number the host is listening to.
    }
    fscanf(file, "%d", &receiverPort);	
    fclose(file);

    bzero((char *) &client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    bcopy( (char *)receiver->h_addr, (char *)&client_addr.sin_addr.s_addr,receiver->h_length);
    client_addr.sin_port = htons(receiverPort);
    fflush(stdin);
    int sendingSocket = socket(AF_INET, SOCK_STREAM, 0);	 // Create a socket to connect to host
    if (sendingSocket < 0) {
        printf("Error in creating the socket\n" );
        exit(0);   
    }
    int flag = connect(sendingSocket,(struct sockaddr *) &client_addr,sizeof(client_addr)); //Connect to the host
    if ( flag < 0) {	
        printf("Error in connecting to the client\n" );
        exit(0);
    }
    flag = send(sendingSocket,buf,datatype * count,0);	//Send the message to the host
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
     struct sockaddr_in cli_addr;
     
     clilen = sizeof(cli_addr);
     int newsockfd;
     int diff;
	if(comm.gather==1 || comm.gather==2){
		struct sockaddr_in serv_addr;
		struct hostent *server;
	   // printf("\n gather recv\n");
	        int sck = (comm.gather == 1)?comm.sockfd_gather:comm.sockfd_gather1;
	        newsockfd = accept(sck, (struct sockaddr *) &cli_addr , &clilen);	//Accept connection on the already created socket during MPI_Init()
		int sender,i=0;
        	while(i<comm.size){
	            server = gethostbyname(comm.hostList[i]);
		    bzero((char *) &serv_addr, sizeof(serv_addr));
	            bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        	    if(serv_addr.sin_addr.s_addr == cli_addr.sin_addr.s_addr ){     //save the client with whom the connection is accepted by the server
	                sender = i;
			break;
        	    }
		    i++;
		 }
		 if(sender!=source)
		     buf = buf+((sender-source)*count*sizeof(double));  
		 
	}
	else{
		 newsockfd = accept(comm.sockfd, (struct sockaddr *) &cli_addr , &clilen);
	}
     
     if (newsockfd < 0) {
        printf("Error accepting connection from %s on %s\n", comm.hostList[source],comm.myHostName );
        exit(0);
     }
     int flag= recv(newsockfd,buf,count*datatype,MSG_WAITALL);	//Receive the message
//     printf("Size recevide from %d at %u is %d\n",source,buf,flag);
     if (flag < 0){
        printf("Error receiving from %s on %s\n", comm.hostList[source],comm.myHostName );
        exit(0);
     }
     fflush(stdin);     
     close(newsockfd); //Close the newly created socket after connection establishment
     return 0;
}
int MPI_Gather(void *buf,int count, MPI_Datatype datatype, void *res, int gCount, MPI_Datatype gdatatype,int root, MPI_Comm comm){
//	printf("\n gathering \n");
	if(comm.rank!=0){
		comm.gather = root;
                MPI_Send((double*)buf, count, MPI_DOUBLE,0,root, comm);
                return 0;
        }
		
        int status = 0;
        for (int i=1; i<comm.size; i++) {
		comm.gather=root;
                MPI_Recv(((double*)res)+(i*count),count,MPI_DOUBLE,i,root,comm,&status);
        }
        return 0;

}
 int MPI_Finalize(MPI_Comm comm){
	close(comm.sockfd);
	if(comm.rank == 0){
		close(comm.sockfd_gather);
	}
}
