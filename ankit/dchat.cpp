#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <math.h>

int defaultPORT=8672;
int socketFD,n;
int chatSocketFD, heartBeatSocketFD, electionSocketFD, sequencerSocketFD;
struct sockaddr_in joinClientAddress, clientAddress, selfAddress;
char msg[1000];
char response[1000];

void identify()
{
	switch(response[0])
	{
		case 'N':
			printf("Notification\n");
			strcpy(msg,"N0A");
			sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
		break;
	}
}


void *userThread(void *data)
{
	printf("Starting User Thread\n");
	while(fgets(msg,1000,stdin)!=NULL)
	{
		sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
	}
}

void *networkThread(void *data)
{
	printf("Starting Network Thread\n");
	while(1)
	{
		socklen_t len=sizeof(clientAddress);
		n=recvfrom(chatSocketFD,response,1000,0,(struct sockaddr *)&clientAddress,&len);
		if(n<0)
		{
			printf("Error in receiving message\n");
		}
		else
		{
			response[n]=0;
			printf("Received : %s\n",response);
			identify();
		}
	}
}

int main(int argc, char **argv)
{
	if((argc !=2)&&(argc!=3))
	{
		printf("To start a chat : dchat <user_name>\n");
		printf("To join a chat : dchat <user_name> <IP_ADDRESS:PORT>\n");
		exit(1);
	}
	
	if(argc==2)		//start a chat
	{
		chatSocketFD=socket(AF_INET, SOCK_DGRAM,0);
		if(chatSocketFD<0)
		{
			printf("Error in getting self socket\n");
			exit(1);
		}
		bzero(&selfAddress,sizeof(selfAddress));
		selfAddress.sin_family=AF_INET;
		selfAddress.sin_addr.s_addr=htonl(INADDR_ANY);
		selfAddress.sin_port=htons(defaultPORT);
		if(bind(chatSocketFD,(struct sockaddr *)&selfAddress,sizeof(selfAddress))<0)
		{
			printf("Error in self bind\n");
			exit(1);
		}		
		FILE *fp;
		char returnData[64];
		char ipString[17];
		fp = popen("/sbin/ifconfig em1", "r");

		while (fgets(returnData, 64, fp) != NULL)
		{
			char *start=strstr(returnData,"inet addr");
			if(start!=NULL)
			{
				char *end=strstr((start+10)," ");
				strncpy(ipString,(start+10),abs(end-(start+10)));
				ipString[end-(start+10)]='\0';
				printf("%s\n",ipString);
			}
		}

		pclose(fp);
		printf("%s started a new chat, listening on %s:%d\nSuccedded, current users : \nTODO - print user list\n",argv[1],ipString,defaultPORT);
	}
	else			//join a chat
	{
		char ip[17],port[6];
		char *tok;
		tok=strstr(argv[2],":");
		if(tok==NULL)
		{
			printf("Error in IP:PORT\nUsage : dchat <user_name> <IP_ADDRESS:PORT>\n");
			exit(1);
		}
		else
		{
			strncpy(ip,argv[2],(tok-argv[2]));
			ip[tok-argv[2]]='\0';
			strncpy(port,tok+1,strlen(argv[2])-(tok-argv[2]));
			port[strlen(argv[2])-(tok-argv[2])]='\0';
		}
		printf("%s\n%s\n",ip,port);
		chatSocketFD=socket(AF_INET, SOCK_DGRAM,0);
		if(chatSocketFD<0)
		{
			printf("Error in getting join client socket\n");
			exit(1);
		}
		bzero(&joinClientAddress,sizeof(joinClientAddress));
		joinClientAddress.sin_family=AF_INET;
		joinClientAddress.sin_addr.s_addr=inet_addr(ip);
		joinClientAddress.sin_port=htons(atoi(port));
		
		bzero(&selfAddress,sizeof(selfAddress));
		selfAddress.sin_family=AF_INET;
		selfAddress.sin_addr.s_addr=htonl(INADDR_ANY);
		selfAddress.sin_port=htons(defaultPORT);
		
		do
		{
			printf("Error in join client bind\nRetrying...\n");
			selfAddress.sin_port=htons(++defaultPORT);
		}while((bind(chatSocketFD,(struct sockaddr *)&selfAddress,sizeof(selfAddress))<0));
		
		strcpy(msg,"N0_");
		if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&joinClientAddress,sizeof(joinClientAddress))<0)
		{
			printf("error in sending join request\n");
			exit(1);
		}
		n=recvfrom(chatSocketFD,response,1000,0,NULL,NULL);
		if(n<0)
		{
			printf("Error in receiving Join confirmation");
			exit(1);
		}
		if(response[0]=='N' && response[1]=='0' && response[2]=='N')
		{
			printf("Join request Denied\n");
			exit(1);
		}
		else if(response[0]=='N' && response[1]=='0' && response[2]=='A')
		{
			printf("Join request Accepted\n");
		}
		else
		{
			printf("Join request Error\n");
			exit(1);
		}
		FILE *fp;
		char returnData[64];
		char ipString[16];
		fp = popen("/sbin/ifconfig em1", "r");

		while (fgets(returnData, 64, fp) != NULL)
		{
			char *start=strstr(returnData,"inet addr");
			if(start!=NULL)
			{
				char *end=strstr((start+10)," ");
				strncpy(ipString,(start+10),abs(end-(start+10)));
				ipString[end-(start+10)]='\0';
				printf("%s\n",ipString);
			}
		}

		pclose(fp);
		printf("%s joined a new chat on %s, listening on %s:%d\nSuccedded, current users : \nTODO - print user list\n",argv[2],argv[1],ipString,defaultPORT);
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