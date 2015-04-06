#include "base.h"

void leaderElection()
{
	
}


void identify()
{
	switch(responseTag[0])
	{
		case 'N':
			if(responseTag[0]=='N' && responseTag[1]=='0' && responseTag[2]=='_')			//respond to a join request
			{
				//add requester to participant list
				string newJoineeKey(createKey(clientAddress));
				struct participant *newJoinee=createParticipant(clientAddress,0, responseMsg);
				participantList.insert(make_pair(newJoineeKey,newJoinee));
				participantListIterator = participantList.find(newJoineeKey);
				if(participantListIterator==participantList.end())
				{
					cout<<"participant not inserted\n";
					strcpy(msg,"N0N");
					sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
				}
				else
				{
					//send an ACK with expectant paricipant list size
					snprintf(msg,1000,"N0A:%d",participantList.size());
					sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
					//send back the participant list
					for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
					{
						strcpy(msg,serializeParticipant((participantListIterator->second)));
						msg[strlen(serializeParticipant((participantListIterator->second)))]='\0';
						//cout<<msg<<endl;
						sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
					}
					//print updated list
					printParticipantList();
				}
			}

		break;
		case 'C':
			participantListIterator=participantList.find(createKey(clientAddress));
			cout<<participantListIterator->second->username<<":"<<responseMsg;
		break;
	}
}


void *userThread(void *data)
{
	//printf("Starting User Thread\n");
	while(fgets(msg,1000,stdin)!=NULL)
	{
		//TODO get sequence number first
		multicast();
		//sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
	}
}

void *networkThread(void *data)
{
	//printf("Starting Network Thread\n");
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
			//cout<<response<<endl;
			if(breakDownMsg()==0)
			{
				identify();
			}
			else
			{
				//request retransmission
			}
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
		//create self data structure
		bzero(&selfAddress,sizeof(selfAddress));
		selfAddress.sin_family=AF_INET;
		selfAddress.sin_addr.s_addr=htonl(INADDR_ANY);
		selfAddress.sin_port=htons(defaultPORT);
		while((bind(chatSocketFD,(struct sockaddr *)&selfAddress,sizeof(selfAddress))<0))
		{
			printf("Error in join client bind\nRetrying...\n");
			selfAddress.sin_port=htons(++defaultPORT);
		}		
		FILE *fp;
		char returnData[64];
		char ipString[INET_ADDRSTRLEN];
		fp = popen("/sbin/ifconfig em1", "r");

		while (fgets(returnData, 64, fp) != NULL)
		{
			char *start=strstr(returnData,"inet addr");
			if(start!=NULL)
			{
				char *end=strstr((start+10)," ");
				strncpy(ipString,(start+10),abs(end-(start+10)));
				ipString[end-(start+10)]='\0';
				//printf("%s\n",ipString);
			}
		}
		pclose(fp);
		if(inet_pton(AF_INET,ipString, &(selfAddress.sin_addr))<=0)
		{
			printf("Error in inet_pton\n");
			exit(1);
		}
		string selfKey(createKey(selfAddress));
		struct participant *self=createParticipant(selfAddress,0, string(argv[1]));
		participantList.insert(make_pair(selfKey,self));
		printf("%s started a new chat, listening on %s:%d\nSuccedded, current users : \nParticipant List - \n",argv[1],ipString,defaultPORT);
		printParticipantList();
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
		//printf("%s\n%s\n",ip,port);
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
		
		while((bind(chatSocketFD,(struct sockaddr *)&selfAddress,sizeof(selfAddress))<0))
		{
			printf("Error in join client bind\nRetrying...\n");
			selfAddress.sin_port=htons(++defaultPORT);
		}
		
		snprintf(msg,1000,"N0_:0:0:%s",argv[1]);
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
			//printf("Join request Accepted\n");
			int participantCount=atoi(&response[4]);
			if(participantCount<2)				//2 as the list should have  host+new joinee at the least
			{
				cout<<"Join Request Error : participant List\n";
				exit(1);
			}
			else
			{
				for(int i=0;i<participantCount;i++)
				{
					n=recvfrom(chatSocketFD,response,1000,0,NULL,NULL);
					if(n<0)
					{
						cout<<"Error in receiving participant\n";
						break;
					}
					response[n]=0;
					//cout<<response<<endl;
					char *second, *third, *fourth;					//pointers to the data within the message
					second=strstr(response,":");
					if(second!=NULL)
					{
						third=strstr(second+1,":");
						if(third==NULL)
						{
							cout<<"Error in receiving participant\n";
							break;
						}
						fourth=strstr(third+1,":");
						if(fourth==NULL)
						{
							cout<<"Error in receiving participant\n";
							break;
						}
						char *ip=new char[INET_ADDRSTRLEN];
						char *port=new char[6];
						char *seq=new char[10];
						strncpy(ip,response,(second-response));
						ip[INET_ADDRSTRLEN-1]='\0';
						strncpy(port,second+1,(third-second-1));
						port[(third-second-1)]='\0';
						strncpy(seq,third+1,(fourth-third-1));
						seq[(fourth-third-1)]='\0';
						//cout<<"Participant - \n"<<ip<<"\n"<<port<<"\n"<<seq<<"\n"<<fourth+1<<endl;
						if((atoi(port)<1024)||(atoi(port)>999999999))
						{
							cout<<"Error in receiving participant\n";
							break;
						}
						struct sockaddr_in participantAddress;
						bzero(&participantAddress,sizeof(participantAddress));
						participantAddress.sin_family=AF_INET;
						participantAddress.sin_addr.s_addr=inet_addr(ip);
						participantAddress.sin_port=htons(atoi(port));
						string clientKey(createKey(participantAddress));
						struct participant *participant=createParticipant(participantAddress,0, fourth+1);
						participantList.insert(make_pair(clientKey,participant));
					}
					else
					{
						cout<<"Error in receiving participant\n";
						break;
					}
					
				}
			}
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
				//printf("%s\n",ipString);
			}
		}

		pclose(fp);
		printf("%s joined a new chat on %s, listening on %s:%d\nSuccedded, current users : \nParticipant List - \n",argv[1],argv[2],ipString,defaultPORT);
		printParticipantList();
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