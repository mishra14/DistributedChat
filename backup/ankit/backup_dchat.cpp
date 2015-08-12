#include "base.h"

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
					sendParticipantList(MULTICAST);							//send participant list to all participants
					//print updated list
					printParticipantList();
					
				}
				updatingParticipantList=false;
			}
			else if(response[0]=='N' && response[1]=='0' && response[2]=='A')
			{
				receiveParticipantList();
				printParticipantList();
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
				pthread_mutex_lock(&isLeaderAliveMutex);
				isLeaderAlive=true;
				pthread_mutex_unlock(&isLeaderAliveMutex);
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
		case 'E':
		if(responseTag[0]=='E' && responseTag[1]=='1' && responseTag[2]=='_')			//request for election
		{
			cout<<"Election request received from : "<<participantList.find(createKey(clientAddress))->second->username<<endl;
			//respond if self id higher than sender (should be)
			strcpy(electionMsg,"E1A:0:0:-");
			if(sendto(chatSocketFD,electionMsg,strlen(electionMsg),0,(struct sockaddr *)&(clientAddress),sizeof(clientAddress))<0)
			{
				cout<<"Error in responding to election request\n";
			}
			//start an election yourself
			cout<<"Election On going : "<<electionOnGoing<<endl;
			if(!electionOnGoing)
			{
				pthread_mutex_lock(&isLeaderAliveMutex);
				isLeaderAlive=false;
				pthread_mutex_unlock(&isLeaderAliveMutex);
				pthread_mutex_lock(&electionMutex);
				cout<<"Sending a start signal on reception\n";
				pthread_cond_signal(&electionBeginCondition);
				pthread_mutex_unlock(&electionMutex);
			}
		}
		else if(responseTag[0]=='E' && responseTag[1]=='1' && responseTag[2]=='A')			//response to an election request
		{
			//bow out of the election 
			if(electionOnGoing)
			{
				participantListIterator=participantList.find(createKey(clientAddress));
				if(participantListIterator!=participantList.end())
				{
					pthread_mutex_lock(&electionBowOutMutex);
					cout<<"Setting BoutOut to true : networkThread1\n";
					electionBowOut=true;				//this will force the election thread to bow out when it wakes up
					pthread_mutex_unlock(&electionBowOutMutex);
					cout<<"Bow out for "<<participantListIterator->second->username<<endl;
				}
				else
				{
					cout<<"Election response from unknown client : "<<createKey(clientAddress)<<endl;
				}
			}
		}
		else if(responseTag[0]=='E' && responseTag[1]=='2' && responseTag[2]=='_')			//broadcast of new leader
		{
			pthread_mutex_lock(&isLeaderAliveMutex);
			isLeaderAlive=true;
			pthread_mutex_unlock(&isLeaderAliveMutex);
			pthread_mutex_lock(&electionOnGoingMutex);
			electionOnGoing=false;
			pthread_mutex_unlock(&electionOnGoingMutex);
			pthread_mutex_lock(&electionBowOutMutex);
			cout<<"Setting BoutOut to true : networkThread2\n";
			electionBowOut=true;
			pthread_mutex_unlock(&electionBowOutMutex);
			participantListIterator=participantList.find(createKey(clientAddress));
			if(participantListIterator!=participantList.end())
			{
				cout<<"leader : "<<participantListIterator->second->username<<endl;
				leader=participantListIterator->second;
			}
			else
			{
				//unknown participant has sent a leader broadcast
				cout<<"Leader Unknown\n";
			}
		}
		else if(responseTag[0]=='E' && responseTag[1]=='2' && responseTag[2]=='A')			//response to new leader broadcast
		{
			
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
		sleepTime.tv_sec=leftTime.tv_sec;
		sleepTime.tv_nsec=leftTime.tv_nsec;
	} 
}

void *electionThread(void *data)
{
	while(1)
	{
		pthread_mutex_lock(&electionMutex);
		pthread_cond_wait(&electionBeginCondition, &electionMutex);			//wait for the signal that the leader is dead
		cout<<"starting new Election\n";		//start the election
		if(!electionOnGoing)
		{
			pthread_mutex_lock(&electionOnGoingMutex);
			electionOnGoing=true;
			pthread_mutex_unlock(&electionOnGoingMutex);
			while(!isLeaderAlive)
			{
				pthread_mutex_lock(&electionBowOutMutex);
				cout<<"Setting BoutOut to false : electionThread1\n";
				electionBowOut=false;				
				pthread_mutex_unlock(&electionBowOutMutex);
				multicast(ELECTION);				//Send Election request to all higher processes
				threadSleep(0,20000000L);					//wait for them to respond
				cout<<"Election Thread : isLeaderAlive : "<<isLeaderAlive<<" electionBowOut : "<<electionBowOut<<endl;
				if(!electionBowOut && !isLeaderAlive)		//none of the higher processes are alive; Hence make self as the leader and broadcast the same
				{
					//threadSleep(1,0);
					cout<<"Self as leader; No higher process responding\n";
					pthread_mutex_lock(&isLeaderAliveMutex);
					isLeaderAlive=true;
					pthread_mutex_unlock(&isLeaderAliveMutex);
					isLeader=true;
					pthread_mutex_lock(&electionOnGoingMutex);
					electionOnGoing=false;
					pthread_mutex_unlock(&electionOnGoingMutex);
					pthread_mutex_lock(&electionBowOutMutex);
					cout<<"Setting BoutOut to false : electionThread2\n";
					electionBowOut=false;				
					pthread_mutex_unlock(&electionBowOutMutex);
					leader=self;
					multicast(LEADER);
					sendParticipantList(MULTICAST);							//send participant list to all participants
					printParticipantList();	
				}
				else
				{
					threadSleep(1,0);				//If atleast one higher process is alive, then give them time to finish the election
				}
			}
		}
		pthread_mutex_unlock(&electionMutex);
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
		//pthread_mutex_lock(&electionBlockMutex);
		while(updatingParticipantList || participantList.size()<=1);
		if(isLeader)
		{
			//cout<<"leader\n";
			heartBeatMap.clear();	 				//clear the responses from the last heart beat
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
								sendParticipantList(MULTICAST);							//send participant list to all participants
								printParticipantList();	
							}
						}
					}
				}
			}
		}
		else
		{
			//cout<<"ElectionOnGoin : "<<electionOnGoing<<endl;
			if(!electionOnGoing)
			{
				pthread_mutex_lock(&isLeaderAliveMutex);
				isLeaderAlive=false;
				pthread_mutex_unlock(&isLeaderAliveMutex);
				threadSleep(1,0);				//sleeping for 15 mSec
				if(!isLeaderAlive && !electionOnGoing)
				{
					cout<<"Leader dead\n";// isLeaderAlive : "<<isLeaderAlive<<" electionOnGoing : "<<electionOnGoing<<endl;
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
						participantListIterator=participantList.find(createKey(self->address));
						participantListIterator++;
						if(participantListIterator==participantList.end())
						{
							//threadSleep(2,0);
							cout<<"Self as leader; No higher process\n";
							pthread_mutex_lock(&isLeaderAliveMutex);
							isLeaderAlive=true;
							pthread_mutex_unlock(&isLeaderAliveMutex);
							isLeader=true;
							pthread_mutex_lock(&electionOnGoingMutex);
							electionOnGoing=false;
							pthread_mutex_unlock(&electionOnGoingMutex);
							pthread_mutex_lock(&electionBowOutMutex);
							cout<<"Setting BoutOut to false : HB1\n";
							electionBowOut=false;				
							pthread_mutex_unlock(&electionBowOutMutex);
							leader=self;
							multicast(LEADER);
							sendParticipantList(MULTICAST);							//send participant list to all participants
							printParticipantList();	
						}
						else if(!electionOnGoing)
						{
							pthread_mutex_lock(&electionMutex);
							cout<<"Sending a start signal on leader dead\n";
							pthread_cond_signal(&electionBeginCondition);
							pthread_mutex_unlock(&electionMutex);
						}
						//TODO inform all others
					}
				}
			}
		}
		//pthread_mutex_unlock(&electionBlockMutex);
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
			if(response[0]=='E')
				cout<<response<<endl;
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
		//find out self public IP and correct the selfAddress to reflect the same
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
			//cout<<response<<endl;
			receiveParticipantList();
		}
		else
		{
			printf("Join request Error\n");
			exit(1);
		}
		
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
	if(pthread_create(&electionThreadID,NULL, electionThread,NULL))
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
	if(pthread_join(electionThreadID, NULL))
	{
		printf("Error joining network thread \n");
	}
	return 0;
}