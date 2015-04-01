#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

int socketFD,n;
struct sockaddr_in serverAddress, clientAddress;
char msg[1000];
char response[1000];

void *userThread(void *data)
{
	printf("Starting User Thread\n");
	while(fgets(msg,1000,stdin)!=NULL)
	{
		sendto(socketFD,msg,strlen(msg),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress));
	}
}

void *networkThread(void *data)
{
	printf("Starting Network Thread\n");
	while(1)
	{
		//printf("waiting....\n");
		n=recvfrom(socketFD,response,1000,0,NULL,NULL);
		response[n]=0;
		printf("Received : %s\n",response);
	}
}

int main(int argc, char **argv)
{
	
	if(argc !=3)
	{
		printf("usage : dchat <IP Address> <Port Number>\n");
		exit(1);
	}
	
	if((atoi(argv[2])<1025) || (atoi(argv[2])>65535))
	{
		printf("usage : dchat <IP Address> <Port Number>\n<Port Number> should be from 1025 to 65535\n");
		exit(1);
	}
	
	socketFD=socket(AF_INET, SOCK_DGRAM,0);
	bzero(&serverAddress,sizeof(serverAddress));
	serverAddress.sin_family=AF_INET;
	serverAddress.sin_addr.s_addr=inet_addr(argv[1]);
	serverAddress.sin_port=htons(2000);//atoi(argv[2]));
	if(bind(socketFD,(struct sockaddr *)&serverAddress,sizeof(serverAddress))<0)
	{
		printf("Error in bind\n");
		exit(1);
	}
	
	pthread_t userThreadID,networkThreadID;
	
	if(pthread_create(&userThreadID,NULL, userThread,NULL))
	{
		printf("Error in creating user thread\n");
		exit(1);
	}	
	if(pthread_create(&networkThreadID,NULL, networkThread,NULL))
	{
		printf("Error in creating user thread\n");
		exit(1);
	}	
	if(pthread_join(userThreadID, NULL))
	{
		printf("Error joining user thread \n");
	}
	if(pthread_join(networkThreadID, NULL))
	{
		printf("Error joining network thread \n");
	}
	return 0;
}