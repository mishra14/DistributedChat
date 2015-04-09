#include "base.h"

void leaderElection()
{
	cout<<"Election\n";
	participantListIterator=participantList.find(createKey(self->address));
	for(; participantListIterator!=participantList.end();participantListIterator++)
	{
		printParticipant(participantListIterator->second);
	}
}


void identify()
{
	switch(responseTag[0])
	{
		case 'N':
			if(responseTag[0]=='N' && responseTag[1]=='0' && responseTag[2]=='_')			//respond to a join request
			{
				updatingParticipantList=true;
				//add requester to participant list
				string newJoineeKey(createKey(clientAddress));
				struct participant *newJoinee=createParticipant(clientAddress,0, responseMsg);
				participantList.insert(make_pair(newJoineeKey,newJoinee));
				if(participantList.find(newJoineeKey)==participantList.end())
				{
					cout<<"participant not inserted\n";
					strcpy(msg,"N0N");
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending\n";
					}
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
						//cout<<msg<<endl;
						if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
						{
							cout<<"Error in sending\n";
						}
					}
					//print updated list
					printParticipantList();
					
				}
				updatingParticipantList=false;
			}
			else if(responseTag[0]=='N' && responseTag[1]=='3' && responseTag[2]=='_')			//respond to a join request
			{
				updatingParticipantList=true;
				participantListIterator=participantList.find(createKey(clientAddress));
				if(participantListIterator==participantList.end())
				{
					cout<<"Ready process not in list\n";
				}
				else
				{
					//participantListIterator->second->isReady=true;
					cout<<participantListIterator->second->username<<" is Ready\n";
				}
				updatingParticipantList=false;
			}
		break;
		case 'C':
			participantListIterator=participantList.find(createKey(clientAddress));
			cout<<participantListIterator->second->username<<":"<<responseMsg;
		break;
		case 'H':
			if(responseTag[0]=='H' && responseTag[1]=='0' && responseTag[2]=='_')			//respond to a join request
			{
				//cout<<response<<endl;
				strcpy(heartBeatMsg,"H0A:0:0:-");
				isLeaderAlive=true;
				//cout<<"HB\n";
				int n=sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
				while(n<0)
				{
					cout<<"error in responding to HB\n";
					int n=sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
				}
			}
			else if(isLeader)
			{
				updatingParticipantList=true;
				heartBeatMap.insert(make_pair(createKey(clientAddress),true));
				responseCountIterator=responseCount.find(createKey(clientAddress));
				if(responseCountIterator!=responseCount.end())
				{
					responseCount.erase(responseCountIterator);
				}
				updatingParticipantList=false;
			}
		break;
	}
}

void threadSleep(int sec, int nSec)					//a method that allows threads to sleep for the specified duration (in seconds and nano seconds)
{
	struct timespec sleepTime, leftTime;		//sleepTime contains the time to sleep; leftTime contains the sleepTime -actual sleepTime;
	sleepTime.tv_sec=sec;
	sleepTime.tv_nsec=nSec;
	while(nanosleep(&sleepTime, &leftTime)<0)
	{
		sleepTime=leftTime;
	}
}

void *heartBeatThread(void *data)
{
	/* if(!self->isReady)
	{	
		strcpy(heartBeatMsg,"N3_:0:0:-");
		if(sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&(leader->address),sizeof((leader->address)))<0)
		{
			cout<<"error in sending ready message\n";
		}
		else
		{
			cout<<"Ready message is sent\n";
		}
	} */
	while(1)
	{
		while(updatingParticipantList || participantList.size()<=1);
		if(isLeader)
		{
			//cout<<"leader\n";
			heartBeatMap.clear();					//clear the responses from the last heart beat
			strcpy(msg,"H0_:0:0:-");				//set up the heart beat message
			multicast(HEARTBEAT);					//send Heart Beat Request to all participants
			threadSleep(0,20000000L);				//sleeping for 10 mSec
			//check the responses from all the clients
			for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
			{
				if(participantListIterator->second!=leader )//&& participantListIterator->second->isReady)
				{
					//cout<<participantListIterator->second->username<<endl;
					//check if a participant has responded or not
					if(heartBeatMap.find(participantListIterator->first)==heartBeatMap.end())			
					{
						//cout<<participantListIterator->second->username<<endl;
						responseCountIterator=responseCount.find(participantListIterator->first);
						if(responseCountIterator==responseCount.end())
						{
							responseCount.insert(make_pair(participantListIterator->first,1));
						}
						else
						{
							responseCountIterator->second=responseCountIterator->second+1;
							cout<<participantListIterator->second->username<<" count - "<<responseCountIterator->second<<endl;
							if(responseCountIterator->second >=5)
							{
								cout<<participantListIterator->second->username<<" is dead\n";
								participantList.erase(participantListIterator);
								responseCount.erase(responseCountIterator);
								printParticipantList();	
							}
						}
					}
				}
			}
		}
		else
		{
			//cout<<"Not Leader\n";
			isLeaderAlive=false;
			threadSleep(1,0);				//sleeping for 15 mSec
			if(!isLeaderAlive)
			{
				cout<<"Leader dead\n";
				participantListIterator=participantList.find(createKey(leader->address));
				if(participantListIterator!=participantList.end())			
				{
					cout<<participantListIterator->second->username<<" is removed\n";
					participantList.erase(participantListIterator);
					printParticipantList();
				}
				if(participantList.size()==1)
				{
					leader=self;
					isLeader=true;
				}
				else
				{
					//leaderElection();
					//TODO start election
					//TODO inform all others
				}
			}
		}
	}
}


void *userThread(void *data)
{
	//printf("Starting User Thread\n");
	while(fgets(msg,1000,stdin)!=NULL)
	{
		//TODO get sequence number first
		multicast(CHAT);
		//sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
	}
}

void *networkThread(void *data)
{
	//printf("Starting Network Thread\n");
	while(1)
	{
		socklen_t len=sizeof(clientAddress);
		int n=recvfrom(chatSocketFD,response,1000,0,(struct sockaddr *)&clientAddress,&len);
		if(n<0)
		{
			printf("Error in receiving message\n");
		}
		else
		{
			response[n]=0;
			/* if(response[0]=='N')			//respond to a join request
			{
				cout<<response<<endl;
			} */
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
	int n=0;
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
		self=createParticipant(selfAddress,0, string(argv[1]));
		participantList.insert(make_pair(selfKey,self));
		leader=self;
		isLeader=true;
		printf("%s started a new chat, listening on %s:%d\nSuccedded, current users : \nParticipant List - \n",argv[1],ipString,defaultPORT);
		printParticipantList();
	}
	else			//join a chat
	{
		char ip[17],port[6],username[20];
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
			//cout<<"Error in join client bind\nRetrying...\n";
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
			if(participantCount<2)				//2 as the list should have host+new joinee at the least
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
					char *second, *third, *fourth, *fifth;					//pointers to the data within the message
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
						fifth=strstr(fourth+1,":");
						char *ip=new char[INET_ADDRSTRLEN];
						char *port=new char[6];
						char *seq=new char[10];
						strncpy(ip,response,(second-response));
						ip[INET_ADDRSTRLEN-1]='\0';
						strncpy(port,second+1,(third-second-1));
						port[(third-second-1)]='\0';
						strncpy(seq,third+1,(fourth-third-1));
						seq[(fourth-third-1)]='\0';
						strncpy(username,fourth+1,((fifth!=NULL)?(fifth-fourth-1):20));
						//cout<<"Participant - \n"<<ip<<"\n"<<port<<"\n"<<seq<<"\n"<<username<<((fifth!=NULL)?"\nleader":"")<<endl;
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
						struct participant *participant=createParticipant(participantAddress,0, username);
						participantList.insert(make_pair(clientKey,participant));
						if(fifth!=NULL)
						{
							leader=participant;
						}	
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
		if(inet_pton(AF_INET,ipString, &(selfAddress.sin_addr))<=0)
		{
			printf("Error in inet_pton\n");
			exit(1);
		}
		string selfKey(createKey(selfAddress));
		participantListIterator=participantList.find(selfKey);
		if(participantListIterator==participantList.end())
		{
			cout<<"Error is participant List : Self Node not found\n";
		}
		else
		{
			self=participantListIterator->second;
		}
		isLeader=(self==leader)?true:false;
		printf("%s joined a new chat on %s, listening on %s:%d\nSuccedded, current users : \nParticipant List - \n",argv[1],argv[2],ipString,defaultPORT);
		printParticipantList();
		//printParticipant(leader);
	}
	//updatingParticipantList=false;
	
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
	if(pthread_create(&heartBeatThreadID,NULL, heartBeatThread,NULL))
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
	if(pthread_join(heartBeatThreadID, NULL))
	{
		printf("Error joining network thread \n");
	}
	return 0;
}