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

using namespace std;

int defaultPORT=8672;
int chatSocketFD, heartBeatSocketFD, electionSocketFD, sequencerSocketFD;
struct sockaddr_in joinClientAddress, clientAddress, selfAddress;
char msg[1000];
char heartBeatMsg[1000];
char chatMsg[1000];
char response[1000];
char responseTag[4];
char responseGlobalSeq[10];
char responseLocalSeq[10];
char responseMsg[1000];
bool isLeader = false, updatingParticipantList = false, isLeaderAlive=true;
pthread_t userThreadID,networkThreadID, heartBeatThreadID;


struct participant									//holds the data for one participant
{
	struct sockaddr_in address;
	int seqNumber;
	string username;
};

std::map <string, bool> heartBeatMap;										//map of key - IP:PORT and value - bool to keep track of which clients are alive
std::map <string, bool>::iterator heartBeatMapIterator;						//iterator for heartBeatMap
std::map <string, struct participant * > participantList;					//map of key - IP:PORT value - participant struct
std::map <string, struct participant * >::iterator participantListIterator; 	//iterator for the participant list
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
			if(participantListIterator->second!=leader)
			{
				result*=sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address));
			}
		}
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

#endif