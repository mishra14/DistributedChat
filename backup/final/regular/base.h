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
#include <set>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <time.h>


#define CHAT 1
#define HEARTBEAT 2
#define ELECTION 3
#define LEADER 4
#define MULTICAST 5
#define SEQUENCE 6
#define SEQUENCEDDISTRIBUTED 7
#define SEQUENCEDCENTRALIZED 8
#define SEQUENCELOST 9

using namespace std;

int localSeq=0, globalSeq=0, proposedSeq=0, generatedGlobalSeq=0, notificationGlobalSeq=0;;
int defaultPORT=8672;
int chatSocketFD, heartBeatSocketFD, electionSocketFD, sequencerSocketFD;
struct sockaddr_in joinClientAddress, clientAddress, selfAddress;
char msg[1000];
char userMsg[1000];
char electionMsg[100];
char heartBeatMsg[1000];
char chatMsg[1000];
char response[1000];
char responseTag[4];
char responseGlobalSeq[10];
char responseLocalSeq[10];
char responseMsg[1000];
char reliabilityMsg[1000];
bool isLeader = false, updatingParticipantList = false, isLeaderAlive=true, electionOnGoing=false, electionBowOut=false;
bool decentralized=false;


pthread_t userThreadID , networkThreadID, heartBeatThreadID, electionThreadID, reliabilityThreadID;
pthread_mutex_t electionOnGoingMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t electionBowOutMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t isLeaderAliveMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t seqBufferMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t txBufferMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t rxBufferMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t holdBackQMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t hold_back_queueMutex = PTHREAD_MUTEX_INITIALIZER;	

pthread_mutex_t electionBlockMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t sharedDataMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_mutex_t participantListMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t electionMutex = PTHREAD_MUTEX_INITIALIZER;	
pthread_cond_t electionBeginCondition = PTHREAD_COND_INITIALIZER;

struct participant									//holds the data for one participant
{
	struct sockaddr_in address;
	int seqNumber;
	string username;
};

struct message
{
	string content;
	string senderKey;
	int localSeq;
	int globalSeq;
	int ackCount;
};

struct txMessage
{
	string content;
	string senderKey;
	int localSeq;
	int globalSeq;
	int ackCount;
	std::set <string> ackList;
};

struct LastSeen
{
 	int last_client_seq;
 	map<int, string> client_msgs;
};

//ip of client, client_seq
map<string, struct LastSeen> hold_back_queue;

std::map <string, int> responseCount;										//map of key - IP:PORT and value - bool to keep track of which clients are alive
std::map <string, int>::iterator responseCountIterator;						//iterator for heartBeatMap
std::map <string, bool> heartBeatMap;										//map of key - IP:PORT and value - bool to keep track of which clients are alive
std::map <string, bool>::iterator heartBeatMapIterator;						//iterator for heartBeatMap
std::map <string, struct participant * > participantList;					//map of key - IP:PORT value - participant struct
std::map <string, struct participant * > participantListNew;					//map of key - IP:PORT value - participant struct
std::map <string, struct participant * >::iterator participantListIterator; 	//iterator for the participant list
std::map <string, struct participant * >::iterator participantListIterator2; 	//second iterator for the participant list
std::map <string, struct participant * >::iterator participantListIteratorHB; 	//iterator for the participant list
std::map <string, bool> ackList;												//map to hold a list of participants who have not acked a message
std::map <int, struct message * > seqBuffer;								//map to hold messages while deciding sequence number; key - localSeq
std::map <int, struct txMessage * > txBuffer;								//map to hold messages after transmission, for reliability; key - globalSeq
std::map <string, std::map <int, struct message * > > rxBuffer;				//map to hold messages after receiving; key1 - clientIP:PORT; key2 - localSeq;
std::map <int, struct message * > holdBackQ;								//map to hold message before delivery; key - GlobalSeq;
std::map <int, int> globalSeqLost;
std::map <int, int> txBufferCounter;
std::map <int, int> seqBufferCounter;


std::map <int, int>::iterator txBufferCounterIterator;
std::map <int, int>::iterator seqBufferCounterIterator;
std::map <int, int>::iterator globalSeqLostIterator;
std::map <int, struct message * >::iterator seqBufferIterator;
std::map <int, struct txMessage * >::iterator txBufferIterator;
std::map <int, struct txMessage * >::iterator txBufferIteratorHB;
std::set <string>::iterator ackListIterator;
std::set <string>::iterator ackListIteratorHB;
std::map <string, std::map <int, struct message * > > ::iterator rxBufferIterator1;
std::map <int, struct message * >::iterator rxBufferIterator2;
std::map <int, struct message * >::iterator holdBackQIterator;
map<string, struct LastSeen > :: iterator outer_itr;
map<int, string > :: iterator inner_itr;
struct message *message;
struct participant *leader, *self;	

struct message * createMessage(string content, int localSeq, string senderKey)
{
	struct message * message = new struct message;
	message ->content = content;
	message ->localSeq=localSeq;
	message ->globalSeq=0;
	message ->ackCount=0;
	message ->senderKey=senderKey;
	return message;
}
struct message * createMessage(string content, int localSeq, int globalSeq, string senderKey)
{
	struct message * message = new struct message;
	message ->content = content;
	message ->localSeq=localSeq;
	message ->globalSeq=globalSeq;
	message ->ackCount=0;
	message ->senderKey=senderKey;
	return message;
}

struct txMessage * createTXMessage(string content, int localSeq, int globalSeq, string senderKey)
{
	struct txMessage * txMessage = new struct txMessage;
	txMessage ->content = content;
	txMessage ->localSeq=localSeq;
	txMessage ->globalSeq=globalSeq;
	txMessage ->ackCount=0;
	txMessage ->senderKey=senderKey;
	return txMessage;
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

void printMessage (struct message *message)
{
	cout<<message->senderKey<<" : "<<message->content<<"ackCount = "<<message->ackCount<<endl;
}
void printParticipant(struct participant *participant)
{
	
	//cout<<"*******************************\n";
	char *seq=new char[10];
	snprintf(seq,10,"%d",participant->seqNumber);
	cout<<participant->username<<" "<<getIP(participant)<<":"<<getPort(participant);
	if(participant==leader)
	{
		cout<<" (leader)";
	}
	cout<<endl;//<<seq;
	//cout<<"*******************************\n";
}
void printParticipantList()
{
	cout<<"*******************************\n";
	for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
	{
		/* if(participantListIterator->second==self)
		{
			cout<<"Self - \n";
		} */
		//cout<<participantListIterator->first<<endl;
		printParticipant(participantListIterator->second);
		cout<<endl;
	}
	cout<<"*******************************\n";
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
		snprintf(chatMsg,1000,"C0_:%d:%d:",globalSeq,localSeq);
		strcat(chatMsg,msg);
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
			{
				result=-1;
			}
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
				if(sendto(chatSocketFD,heartBeatMsg,strlen(heartBeatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
				{
					result=-1;
				}
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
			//cout<<"Election Req to "<<participantListIterator->second->username<<" ";
			if(sendto(chatSocketFD,electionMsg,strlen(electionMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
			{
				result=-1;
			}
		}
	}
	else if(type==LEADER)					//chat type message
	{
		electionMsg[0]='\0';
		strcat(electionMsg,"E2_:0:0:-");
		//cout<<"Sending new leader announcement to ";
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(participantListIterator->second!=self)
			{
				//cout<<participantListIterator->second->username<<", ";
				if(sendto(chatSocketFD,electionMsg,strlen(electionMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
				{
					result=-1;
				}
			}
		}
		//cout<<endl;
	}
	else if(type==SEQUENCE)					//chat type message
	{
		pthread_mutex_lock(&seqBufferMutex);
		localSeq++;
		chatMsg[0]='\0';
		snprintf(chatMsg,1000,"S2_:0:%d:",localSeq);
		strcat(chatMsg,msg);
		seqBuffer.insert(make_pair(localSeq,createMessage(chatMsg,localSeq,createKey(selfAddress))));
		pthread_mutex_unlock(&seqBufferMutex);
		//cout<<"Sending sequence request for : "<<chatMsg<<endl;
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
			{
				result=-1;
			}
		}
	}
	else if(type==SEQUENCEDCENTRALIZED)					//chat type message
	{
		struct txMessage * txMessage = createTXMessage(msg, atoi(responseLocalSeq), generatedGlobalSeq, createKey(clientAddress));
		std::map <string, struct participant * >::iterator participantListIterator3; 	//second iterator for the participant list
		for(participantListIterator3=participantList.begin(); participantListIterator3!=participantList.end();participantListIterator3++)
		{
			if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&((participantListIterator3->second)->address),sizeof((participantListIterator3->second)->address))<0)
			{
				result=-1;
			}
			else
			{
				//(txMessage->ackList).insert(participantListIterator3->first);
			}
		}
		//insert the msg into txBuffer to check for ACK's; key=globalSeq, value=list of IP's that were sent the message to
		pthread_mutex_lock(&txBufferMutex);
		txBuffer.insert(make_pair(generatedGlobalSeq,txMessage));
		pthread_mutex_unlock(&txBufferMutex);
		/* cout<<"TX buffer - \n";
		for(txBufferIterator=txBuffer.begin();txBufferIterator!=txBuffer.end();txBufferIterator++)
		{
			cout<<txBufferIterator->first<<":";
			for(ackListIterator=(txBufferIterator->second->ackList).begin();ackListIterator!=(txBufferIterator->second->ackList).end();ackListIterator++)
			{
				cout<<*ackListIterator<<" ";
			}
			cout<<endl;
		} */ 
	}
	else if(type==SEQUENCEDDISTRIBUTED)					//chat type message
	{
		struct txMessage * txMessage = createTXMessage(msg, atoi(responseLocalSeq), generatedGlobalSeq, createKey(clientAddress));
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
			{
				result=-1;
			}
			else
			{
				(txMessage->ackList).insert(participantListIterator->first);
			}
		}
		//insert the msg into txBuffer to check for ACK's; key=globalSeq, value=list of IP's that were sent the message too
		pthread_mutex_lock(&txBufferMutex);
		txBuffer.insert(make_pair(generatedGlobalSeq,txMessage));
		pthread_mutex_unlock(&txBufferMutex);
		/* cout<<"TX buffer - \n";
		for(txBufferIterator=txBuffer.begin();txBufferIterator!=txBuffer.end();txBufferIterator++)
		{
			cout<<txBufferIterator->first<<":";
			for(ackListIterator=(txBufferIterator->second->ackList).begin();ackListIterator!=(txBufferIterator->second->ackList).end();ackListIterator++)
			{
				cout<<*ackListIterator<<" ";
			}
			cout<<endl;
		}	 */
	}
	else if(type==SEQUENCELOST)					//chat type message
	{
		for(participantListIterator=participantList.begin(); participantListIterator!=participantList.end();participantListIterator++)
		{
			if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&((participantListIterator->second)->address),sizeof((participantListIterator->second)->address))<0)
			{
				result=-1;
			}
		}
		//insert the msg into txBuffer to check for ACK's; key=globalSeq, value=list of IP's that were sent the message to
	}
	return result;
}

void extractSender(char *ipString, char * portString)
{
	char *second, *third;					//pointers to the data within the message

	second=strstr(responseMsg,":");
	if(second==NULL)
	{
		cout<<"	Error in sequenced chat message break down - No port\n";
		cout<<responseMsg;
		return ;
	}
	third=strstr(second+1,":");
	if(third==NULL)
	{
		cout<<"Error in sequenced chat message break down - No message\n";
		cout<<responseMsg;
		return ;
	}
	strncpy(ipString,responseMsg,(second-responseMsg));
	ipString[second-responseMsg]='\0';
	strncpy(portString,second+1,(third-second-1));
	portString[third-second-1]='\0';
	strcpy(responseMsg,third+1);
	//update clientAddress to reflect the details of the original sender
	bzero(&clientAddress,sizeof(clientAddress));
	clientAddress.sin_family=AF_INET;
	if(inet_pton(AF_INET,ipString, &(clientAddress.sin_addr))<=0)
	{
		cout<<"Error in sequence lost response break down - unable to get source IP\n";
	}
	clientAddress.sin_port=htons(atoi(portString));
	//cout<<"original Msg : "<<response<<" from : "<<ipString<<":"<<portString<<endl;
}

int breakDownMsg()
{
	char *second, *third, *fourth;					//pointers to the data within the message
	second=strstr(response,":");
	if(second==NULL)
	{
		cout<<"Error Source : "<<response;
		cout<<"Error in msg break down - No Tag\n";
		return 1;
	}
	third=strstr(second+1,":");
	if(third==NULL)
	{
		cout<<"Error Source : "<<response;
		cout<<"Error in msg break down - No Global seq\n";
		return 1;
	}
	fourth=strstr(third+1,":");
	if(fourth==NULL)
	{
		cout<<"Error Source : "<<response;
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
	struct message * tempMessage=createMessage(response, atoi(responseLocalSeq), createKey(clientAddress));
	tempMessage->globalSeq=atoi(responseGlobalSeq);
	/* rxBufferIterator1=rxBuffer.find(createKey(clientAddress));
	if(rxBufferIterator1!=rxBuffer.end())
	{
		(rxBufferIterator1->second).insert(make_pair(tempMessage->localSeq,tempMessage));
	}
	else
	{
		std::map<int, struct message *> rxBufferInner;
		rxBufferInner.insert( make_pair(tempMessage->localSeq,tempMessage));
		rxBuffer.insert(make_pair(createKey(clientAddress),rxBufferInner));
	} */
	return 0;
}

void receiveParticipantList()
{
	breakDownMsg();
	//cout<<responseMsg<<endl;
	int participantCount=atoi(&responseMsg[0]);
	bool addition=false;
	if(participantCount<2)				//2 as the list should have host+new joinee at the least
	{
		cout<<"Join Request Error : participant List\n";
		//exit(1);
	}
	else
	{
		addition=(participantCount>participantList.size())?true:false;
		pthread_mutex_lock(&participantListMutex);
		participantListNew=participantList;
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
			//cout<<responseMsg<<endl;
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
				struct participant *participant=createParticipant(participantAddress,atoi(seq), username);
				//check if this participant is known or not
				if(participantList.find(clientKey)==participantList.end())			//not there; add him/her to the list
				{
					participantList.insert(make_pair(clientKey,participant));
					cout<<"NOTICE : "<<participant->username<<" joined the chat on "<<clientKey<<"....\n";
					if(fifth!=NULL)
					{
						leader=participant;
					}	
				}
				else																//if there; remove from the new list (to track deletions)
				{
					participantListNew.erase(clientKey);
				}

			}
			else
			{
				cout<<"Error in receiving participant 5\n";
				break;
			}
		
		}
		if(participantListNew.size()>0)			//some participants need to be deleted
		{
			for(participantListIterator=participantListNew.begin();participantListIterator!=participantListNew.end();participantListIterator++)
			{
					cout<<"NOTICE : "<<participantListIterator->second->username<<" has left the chat or crashed :(\n";
					participantList.erase(participantListIterator->first);
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
		/* cout<<"Self - \n";
		printParticipant(self);
		cout<<"leader - \n";
		printParticipant(leader); */
		isLeader=(self==leader)?true:false;
		//printParticipantList();
		pthread_mutex_unlock(&participantListMutex);
	}
}

void receiveParticipantListFirst()
{
	breakDownMsg();
	//cout<<responseMsg<<endl;
	int participantCount=atoi(&responseMsg[0]);
	bool addition=false;
	if(participantCount<2)				//2 as the list should have host+new joinee at the least
	{
		cout<<"Join Request Error : participant List\n";
		//exit(1);
	}
	else
	{
		addition=(participantCount>participantList.size())?true:false;
		pthread_mutex_lock(&participantListMutex);
		participantListNew=participantList;
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
			//cout<<responseMsg<<endl;
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
				struct participant *participant=createParticipant(participantAddress,atoi(seq), username);
					//check if this participant is known or not
				if(participantList.find(clientKey)==participantList.end())			//not there; add him/her to the list
				{
					participantList.insert(make_pair(clientKey,participant));
				}
				else																//if there; remove from the new list (to track deletions)
				{
					participantListNew.erase(clientKey);
				}
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
		pthread_mutex_unlock(&participantListMutex);
	}
}


void sendParticipantList(int type)
{
	if(type==MULTICAST)						//send the whole list to everyone
	{
		notificationGlobalSeq=++generatedGlobalSeq;
		//cout<<"generatedGlobalSeq : "<<generatedGlobalSeq<<"\tNotification globalSeq : "<<notificationGlobalSeq<<endl;
		//cout<<"Sending new list to ";
		for(participantListIterator2=participantList.begin(); participantListIterator2!=participantList.end();participantListIterator2++)
		{
			if(participantListIterator2->second!=self)
			{
				//cout<<participantListIterator2->second->username<<", ";
				clientAddress=participantListIterator2->second->address;
				snprintf(msg,1000,"N0A:%d:0:%d",generatedGlobalSeq,participantList.size());
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
		//cout<<endl;
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


int initializeSequencer()
{
	//update the last seen queue for every participant with non zero local sequence numbers
	int result=0;
	//clear the last seen queue to ensure no data is present
	if(generatedGlobalSeq==0 && globalSeq>0)
	{
		
		generatedGlobalSeq=globalSeq;
		//cout<<"updating generating Sequence number to  - "<<generatedGlobalSeq<<endl;
	}
	pthread_mutex_lock(&hold_back_queueMutex);
	hold_back_queue.clear();
	for( participantListIterator = participantList.begin(); participantListIterator != participantList.end() ; participantListIterator++)
	{
		if(participantListIterator->second->seqNumber > 0)
		{
			//a new sequencer was elected. Update lastSeen localSeq from each IP
			struct LastSeen lastseen;
			lastseen.last_client_seq = participantListIterator->second->seqNumber;
			hold_back_queue.insert(make_pair(participantListIterator->first, lastseen));
		}	
	}
	pthread_mutex_unlock(&hold_back_queueMutex);
	
	return result;
}

 
void sendSequenced()
{
	if(breakDownMsg()==0)
	{
		snprintf(msg,1000,"S0A:%d:%d:%s", ++generatedGlobalSeq, atoi(responseLocalSeq), responseMsg);
		int n=sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
		if(n<0)
		{
			cout << "Error in sending Global sequence number"<<endl;
		}
		char *ipString = new char[20];
		char *portString = new char[5];
		extractSender(ipString,portString);
		strcpy(response,responseMsg);
		if(breakDownMsg()!=0)
		{
			cout<<"Error in sequence breakdown\n";
		}
		if(strcmp(responseTag,"N0_")==0)
		{
			updatingParticipantList=true;
			pthread_mutex_lock(&participantListMutex);
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
					cout<<"Error in sending join NACK\n";
				}
			}
			else
			{
				//send an ACK with expectant participant list size
				cout<<"NOTICE : "<<newJoinee->username<<" has joined the chat on "<<newJoineeKey<<endl;
				sendParticipantList(MULTICAST);							//send participant list to all participants
				//print updated list
				//printParticipantList();
			}
			pthread_mutex_unlock(&participantListMutex);
			updatingParticipantList=false; 
		}
		else
		{
			snprintf(msg,1000,"%s:%d:%d:%s:%s",responseTag,generatedGlobalSeq,atoi(responseLocalSeq),createKey(clientAddress),responseMsg);
			//cout<<"Multicasting  : "<<msg<<"\nFrom  : "<<createKey(clientAddress)<<endl;	
			n = multicast(SEQUENCEDCENTRALIZED);
			if(n<0)
			{
				cout<<"Error in multicasting sequenced message : "<<msg;
			}
		}
	}
	else
	{
		cout<<"breakdown error in resequencing : "<<response;
	}
}

int sequencer(string key, int client_seq)
{
	struct LastSeen lastseen;
	outer_itr = hold_back_queue.find(key);
	if(outer_itr == hold_back_queue.end())
	{
		// this is the first request from this IP
		//cout << "First req from this IP"<< key<<":"<< "seq:"<< client_seq<< endl;
		lastseen = outer_itr->second;
		lastseen.last_client_seq = client_seq;
		//m_timers.insert(std::pair<std::string,timerInfo>(timerName, timerInfo(GetTime(),0)));
		pthread_mutex_lock(&hold_back_queueMutex);
		hold_back_queue.insert(pair<string, LastSeen>(key, lastseen));
		pthread_mutex_unlock(&hold_back_queueMutex);
		sendSequenced();
	}
	else 
	{	
		if((outer_itr->second).client_msgs.empty() && (outer_itr->second).last_client_seq == client_seq-1) 
		{
			//vector corresponding to this IP is empty but this is not the first request
			//cout <<"IP exists but vector empty:"<< key<< " Previous seen from this IP:" << (outer_itr->second).last_client_seq<< endl;
			pthread_mutex_lock(&hold_back_queueMutex);
			(outer_itr->second).last_client_seq = client_seq;
			pthread_mutex_unlock(&hold_back_queueMutex);
			sendSequenced();
		}
		else			//out of order vector was not empty
		{	//Out of local sequence number
			//cout << "Out of order seq request:" << client_seq<< endl;
			// add last_seq number too?
			
			pthread_mutex_lock(&hold_back_queueMutex);
			if((outer_itr->second).last_client_seq >= client_seq)
			{
				
				snprintf(msg,1000,"S0A:1:%d:%s",atoi(responseLocalSeq), responseMsg);
				int n=sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
				if(n<0)
				{
					cout << "Error in sending Global sequence number"<<endl;
				}
				else
				{
					//cout<<"ACK to old Seq Request : "<<client_seq<<endl;
				}
			}
			else
			{
				(outer_itr->second).client_msgs.insert(make_pair(client_seq,response));
				//(outer_itr->second).client_seq_nos.push_back(client_seq);
				hold_back_queue.insert(make_pair(key,(outer_itr->second)));
				//sort((outer_itr->second).client_seq_nos.begin(), (outer_itr->second).client_seq_nos.end());
				//cout << "last seen seq for this ip:"<<(outer_itr->second).last_client_seq<< endl;
				int missingSeq =((outer_itr->second).last_client_seq)+1;	//missingSeq is the localSeq from that client that you should be receiving
				inner_itr=((outer_itr->second).client_msgs).begin();
				while((missingSeq < inner_itr->first) && (inner_itr!=((outer_itr->second).client_msgs).end()) )	//detected an out of order sequence request
				{
					//request a retransmission for missingSeq
					snprintf(msg,1000,"S4_:0:%d:_",missingSeq);
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending sequence retransmission request\n";
					}
					missingSeq++;
					inner_itr++;			
				}
				
				for(inner_itr=((outer_itr->second).client_msgs).begin(); inner_itr!=((outer_itr->second).client_msgs).end();inner_itr++)
				{
					if((outer_itr->second).last_client_seq == (inner_itr->first)-1)
					{	
						(outer_itr->second).last_client_seq = inner_itr->first;
						
						//cout<<"Giving seq no to :" << (outer_itr->second).last_client_seq <<":"<<(inner_itr->second).c_str()<< endl;
						strcpy(response, (inner_itr->second).c_str());
						sendSequenced();
						((outer_itr->second).client_msgs).erase(inner_itr);
					}
				}
			}
			pthread_mutex_unlock(&hold_back_queueMutex);
		}
	}
	return 0;
}


#endif