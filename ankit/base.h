#ifndef BASE_H
#define BASE_H

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
#include <map>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#define CHAT 1
#define HEARTBEAT 2
#define ELECTION 3
#define LEADER 4
#define MULTICAST 5

using namespace std;

int defaultPORT=8672;
int chatSocketFD, heartBeatSocketFD, electionSocketFD, sequencerSocketFD;
struct sockaddr_in joinClientAddress, clientAddress, selfAddress;
char msg[1000];
char electionMsg[100];
char heartBeatMsg[1000];
char chatMsg[1000];
char response[1000];
char responseTag[4];
char responseGlobalSeq[10];
char responseLocalSeq[10];
char responseMsg[1000];
bool isLeader = false, updatingParticipantList = false, isLeaderAlive=true, electionOnGoing=false, electionBowOut=false;

pthread_t userThreadID,networkThreadID, heartBeatThreadID, electionThreadID;
pthread_mutex_t electionOnGoingMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t electionBowOutMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t isLeaderAliveMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t electionBlockMutex = PTHREAD_MUTEX_INITIALIZER;	

pthread_mutex_t electionMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_cond_t electionBeginCondition = PTHREAD_COND_INITIALIZER;

struct participant									//holds the data for one participant
{
	struct sockaddr_in address;
	int seqNumber;
	string username;
};

std::map <string, int> responseCount;										//map of key - IP:PORT and value - bool to keep track of which clients are alive
std::map <string, int>::iterator responseCountIterator;						//iterator for heartBeatMap
std::map <string, bool> heartBeatMap;										//map of key - IP:PORT and value - bool to keep track of which clients are alive
std::map <string, bool>::iterator heartBeatMapIterator;						//iterator for heartBeatMap
std::map <string, struct participant * > participantList;					//map of key - IP:PORT value - participant struct
std::map <string, struct participant * >::iterator participantListIterator; 	//iterator for the participant list
std::map <string, struct participant * >::iterator participantListIterator2; 	//second iterator for the participant list
struct participant *leader, *self;	


bool compareParticipants()					//TODO - fill this comparison function for the map
{
	bool result=false;
	
	return result;
}

struct participant * createParticipant(struct sockaddr_in address, int seqNumber, string name)		//create a participant based on raw data
{
	struct participant * participant = new struct participant;
	participant->address=address;
	participant->seqNumber=seqNumber;
	participant->username=name;
	return participant;
}


in_port_t getPort(struct sockaddr *address)					//get port number in raw format
{
	if (address->sa_family == AF_INET) 
	{
        return (((struct sockaddr_in*)address)->sin_port);
    }
    return (((struct sockaddr_in6*)address)->sin6_port);
}

int getPort(struct participant *participant)				//get port number in integer format
{
	int port=ntohs(getPort((struct sockaddr*)&participant->address));
	return port;
}

char * getIP(struct participant *participant)				//get IP from participant
{
	char *ip=new char[INET_ADDRSTRLEN];
	
	if(inet_ntop(AF_INET,&(participant->address.sin_addr),ip, INET_ADDRSTRLEN)==NULL)
	{
		cout<<"Error in inet_ntop\n";
	}
	return ip;
}

char * getIP(struct sockaddr_in address)				//get IP from address data structure
{
	char *ip=new char[INET_ADDRSTRLEN];
	
	if(inet_ntop(AF_INET,&(address.sin_addr),ip, INET_ADDRSTRLEN)==NULL)
	{
		cout<<"Error in inet_ntop\n";
	}
	return ip;
}

char *createKey(struct sockaddr_in address)
{
	char *ip=new char[INET_ADDRSTRLEN];
	if(inet_ntop(AF_INET,&(address.sin_addr),ip, INET_ADDRSTRLEN)==NULL)
	{
		cout<<"Error in inet_ntop\n";
	}
	char *key=new char[INET_ADDRSTRLEN+6];
	snprintf(key,INET_ADDRSTRLEN+6,"%s:%d",ip,ntohs(getPort((struct sockaddr*)&address)));
	return key;
}
void printParticipant(struct participant *participant)
{
	
	//cout<<"-------------------------------\n";
	char *seq=new char[10];
	snprintf(seq,10,"%d",participant->seqNumber);
	cout<<participant->username<<endl<<getIP(participant)<<":"<<getPort(participant)<<endl<<seq;
	//cout<<"-------------------------------\n";
}
void printParticipantList()
{
	cout<<"-------------------------------\n";
	for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
	{
		if(participantListIterator->second==self)
		{
			cout<<"Self - \n";
		}
		cout<<participantListIterator->first<<endl;
		printParticipant(participantListIterator->second);
		cout<<endl;
	}
	cout<<"-------------------------------\n";
}

char * serializeParticipant(struct participant *participant)
{
	char *result=new char[1000];
	char *seq=new char[10];
	snprintf(seq,10,"%d",participant->seqNumber);
	strcat(result,createKey(participant->address));
	strcat(result,":");
	strcat(result, seq);
	strcat(result,":");
	strcat(result,(participant->username).c_str());
	if(participant==leader)
	{
		strcat(result,":");
		strcat(result,"leader");
	}
	//cout<<result<<endl;
	return result;
}

int multicast(int type)
{
	int result=1;
	if(type==CHAT)					//chat type message
	{
		chatMsg[0]='\0';
		strcat(chatMsg,"C0_:0:0:");
		strcat(chatMsg,msg);
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			result*=sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address));
		}
	}
	else if(type==HEARTBEAT)			//heartbeat type message
	{
		heartBeatMsg[0]='\0';
		strcat(heartBeatMsg,"H0_:0:0:-");
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(participantListIterator->second!=self)
			{
				//cout<<"HB to "<<participantListIterator->second->username<<" ";
				result*=sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address));
			}
			else
			{
				//cout<<"HB not to "<<participantListIterator->second->username<<endl;
			}
		}
	}
	else if(type==ELECTION)			//heartbeat type message
	{
		electionMsg[0]='\0';
		strcat(electionMsg,"E1_:0:0:-");
		participantListIterator=participantList.find(createKey(self->address));
		participantListIterator++;
		for(; participantListIterator!=participantList.end();participantListIterator++)
		{
			cout<<"Election Req to "<<participantListIterator->second->username<<" ";
			result*=sendto(chatSocketFD,electionMsg,strlen(electionMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address));
		}
	}
	else if(type==LEADER)					//chat type message
	{
		electionMsg[0]='\0';
		strcat(electionMsg,"E2_:0:0:-");
		cout<<"Sending new leader announcement to ";
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(participantListIterator->second!=self)
			{
				cout<<participantListIterator->second->username<<", ";
				result*=sendto(chatSocketFD,electionMsg,strlen(electionMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address));
			}
		}
		cout<<endl;
	}
	return result;
}

int breakDownMsg()
{
	char *second, *third, *fourth;					//pointers to the data within the message
	second=strstr(response,":");
	if(second==NULL)
	{
		cout<<"Error in msg break down - No Tag\n";
		return 1;
	}
	third=strstr(second+1,":");
	if(third==NULL)
	{
		cout<<"Error in msg break down - No Global seq\n";
		return 1;
	}
	fourth=strstr(third+1,":");
	if(fourth==NULL)
	{
		cout<<"Error in msg break down - No Local Seq\n";
		return 1;
	}
	strncpy(responseTag,response,(second-response));
	responseTag[(second-response)]='\0';
	strncpy(responseGlobalSeq,second+1,(third-second-1));
	responseGlobalSeq[(third-second-1)]='\0';
	strncpy(responseLocalSeq,third+1,(fourth-third-1));
	responseLocalSeq[(fourth-third-1)]='\0';
	strcpy(responseMsg,fourth+1);
	//cout<<"BreakDown - \n"<<responseTag<<"\n"<<responseGlobalSeq<<"\n"<<responseLocalSeq<<"\n"<<responseMsg<<endl;
	return 0;
}

void receiveParticipantList()
{
	breakDownMsg();
	cout<<responseMsg<<endl;
	int participantCount=atoi(&responseMsg[0]);
	if(participantCount<2)				//2 as the list should have host+new joinee at the least
	{
		cout<<"Join Request Error : participant List\n";
		//exit(1);
	}
	else
	{
		participantList.clear();
		for(int i=0;i<participantCount;i++)
		{
			int n=recvfrom(chatSocketFD,response,1000,0,NULL,NULL);
			if(n<0)
			{
				cout<<"Error in receiving participant 1\n";
				break;
			}
			response[n]=0;
			breakDownMsg();
			//cout<<response<<endl;
			cout<<responseMsg<<endl;
			char *second, *third, *fourth, *fifth;					//pointers to the data within the message
			second=strstr(responseMsg,":");
			if(second!=NULL)
			{
				third=strstr(second+1,":");
				if(third==NULL)
				{
					cout<<"Error in receiving participant 2\n";
					break;
				}
				fourth=strstr(third+1,":");
				if(fourth==NULL)
				{
					cout<<"Error in receiving participant 3\n";
					break;
				}
				fifth=strstr(fourth+1,":");
				char *ip=new char[INET_ADDRSTRLEN];
				char *port=new char[6];
				char *seq=new char[10];
				char *username=new char[30];
				strncpy(ip,responseMsg,(second-responseMsg));
				ip[INET_ADDRSTRLEN-1]='\0';
				strncpy(port,second+1,(third-second-1));
				port[(third-second-1)]='\0';
				strncpy(seq,third+1,(fourth-third-1));
				seq[(fourth-third-1)]='\0';
				strncpy(username,fourth+1,((fifth!=NULL)?(fifth-fourth-1):20));
				//cout<<"Participant - \n"<<ip<<"\n"<<port<<"\n"<<seq<<"\n"<<username<<((fifth!=NULL)?"\nleader":"")<<endl;
				if((atoi(port)<1024)||(atoi(port)>999999999))
				{
					cout<<"Error in receiving participant 4\n";
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
				cout<<"Error in receiving participant 5\n";
				break;
			}
		
		}
		string selfKey(createKey(selfAddress));
		//cout<<"Self Key : "<<selfKey<<endl;
		participantListIterator=participantList.find(selfKey);
		if(participantListIterator==participantList.end())
		{
			cout<<"Error is participant List : Self Node not found\n";
		}
		else
		{
			self=participantListIterator->second;
		}
		//cout<<"Self - \n";
		//printParticipant(self);
		isLeader=(self==leader)?true:false;
		//printParticipantList();
	}
}

void sendParticipantList(int type)
{
	if(type==MULTICAST)						//send the whole list to everyone
	{
		cout<<"Sending new list to ";
		for(participantListIterator2=participantList.begin(); participantListIterator2!=participantList.end();participantListIterator2++)
		{
			if(participantListIterator2->second!=self)
			{
				cout<<participantListIterator2->second->username<<", ";
				clientAddress=participantListIterator2->second->address;
				snprintf(msg,1000,"N0A:0:0:%d",participantList.size());
				sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
				for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
				{
					strcpy(msg,"N0A:0:0:");
					strcat(msg,serializeParticipant((participantListIterator->second)));
					//cout<<msg<<endl;
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending\n";
					}
				}
			}
		}
		cout<<endl;
	}
	else									//send the whole list to only one 
	{
		snprintf(msg,1000,"N0A:0:0:%d",participantList.size());
		sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			strcpy(msg,"N0A:0:0:");
			strcat(msg,serializeParticipant((participantListIterator->second)));
			//cout<<msg<<endl;
			if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
			{
				cout<<"Error in sending\n";
			}
		}
	}
}
#endif