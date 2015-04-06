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
using namespace std;

int defaultPORT=8672;
int socketFD,n;
int chatSocketFD, heartBeatSocketFD, electionSocketFD, sequencerSocketFD;
struct sockaddr_in joinClientAddress, clientAddress, selfAddress;
char msg[1000];
char response[1000];

struct participant
{
	struct sockaddr_in address;
	int seqNumber;
	string username;
};

std::map <string, struct participant * > participantList;


bool compareParticipants()
{
	bool result=false;
	
	return result;
}

struct participant * createParticipant(struct sockaddr_in address, int seqNumber, string name)
{
	struct participant * participant = new struct participant;
	participant->address=address;
	participant->seqNumber=seqNumber;
	participant->username=name;
	return participant;
}


in_port_t getPort(struct sockaddr *address)
{
	if (address->sa_family == AF_INET) 
	{
        return (((struct sockaddr_in*)address)->sin_port);
    }
    return (((struct sockaddr_in6*)address)->sin6_port);
}

int getPort(struct participant *participant)
{
	int port=ntohs(getPort((struct sockaddr*)&participant->address));
	return port;
}

char * getIP(struct participant *participant)
{
	char *ip=new char[INET_ADDRSTRLEN];
	
	if(inet_ntop(AF_INET,&(participant->address.sin_addr),ip, INET_ADDRSTRLEN)==NULL)
	{
		cout<<"Error in inet_ntop\n";
	}
	return ip;
}
void printParticipant(struct participant *participant)
{
	
	//cout<<"-------------------------------\n";
	cout<<participant->username<<endl<<getIP(participant)<<":"<<getPort(participant)<<endl;
	//cout<<"-------------------------------\n";
}
void printParticipantList()
{
	std::map <string, struct participant * >::iterator participantListIterator;
	cout<<"-------------------------------\n";
	for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
	{
		cout<<participantListIterator->first<<endl;
		printParticipant(participantListIterator->second);
		cout<<endl;
	}
	cout<<"-------------------------------\n";
}


#endif