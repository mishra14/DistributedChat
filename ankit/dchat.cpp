#include "base.h"

void identify()
{
	switch(responseTag[0])
	{
		case 'N':
			if(responseTag[0]=='N' && responseTag[1]=='0' && responseTag[2]=='_')			//respond to a join request
			{
				if(!decentralized)
				{
					//forward request to leader
					pthread_mutex_lock(&seqBufferMutex);
					localSeq++;
					snprintf(msg,1000,"S0_:0:%d:%s:%s",localSeq,createKey(clientAddress),response);
					seqBuffer.insert(make_pair(localSeq,createMessage(msg,localSeq,createKey(selfAddress))));
					pthread_mutex_unlock(&seqBufferMutex);
					//cout<<"Sending sequence request for : "<<msg<<endl;
					int n=sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
					while(n<0)
					{
						n=sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
					}
				}
				/* updatingParticipantList=true;
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
				updatingParticipantList=false; */
			}
			else if(response[0]=='N' && response[1]=='0' && response[2]=='A')
			{
				notificationGlobalSeq=(notificationGlobalSeq<atoi(responseGlobalSeq))?atoi(responseGlobalSeq):notificationGlobalSeq;
				cout<<"NOTICE : change in participant list\n";
				receiveParticipantList();
				printParticipantList();
			}
			else if(responseTag[0]=='N' && responseTag[1]=='3' && responseTag[2]=='_')			//ready message------NOT USED
			{
				updatingParticipantList=true;
				pthread_mutex_lock(&participantListMutex);
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
				pthread_mutex_unlock(&participantListMutex);
				updatingParticipantList=false;
			}
			else if(responseTag[0]=='N' && responseTag[1]=='1' && responseTag[2]=='_')			//respond to a join request
			{
				receiveParticipantList();
				printParticipantList();
			}
		break;
		case 'C':
			if(responseTag[0]=='C' && responseTag[1]=='0' && responseTag[2]=='_')				//chat msg
			{
				if(!decentralized)
				{
					//cout<<"Testing centr. : "<<responseMsg;
					char *second, *third;					//pointers to the data within the message
					char *ipString = new char[20];
					char *portString = new char[5];
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
					//cout<<"test third : "<<third+1;
					snprintf(response,1000,"C0_:%d:%d:%s",atoi(responseGlobalSeq),atoi(responseLocalSeq),third+1);
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
				//received and validated a chat message generating an ACK
				snprintf(msg,1000,"C0A:%s:%s:_",responseGlobalSeq,responseLocalSeq);
				if(decentralized)
				{
					//send to original sender
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending chat ACK\n";
					}
				}
				else
				{
					//if(defaultPORT!=8674)
					{
						//send to sequencer
						if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&(leader->address),sizeof((leader->address)))<0)
						{
							cout<<"Error in sending chat ACK\n";
						}
					}
				}
				
				//cout<<"In identify : "<<response<<endl;
				if(globalSeq==0)
				{
					//receiving the first chat message
					pthread_mutex_lock(&holdBackQMutex);
					holdBackQ.insert(make_pair(atoi(responseGlobalSeq),createMessage(response,atoi(responseLocalSeq),atoi(responseGlobalSeq),createKey(clientAddress))));
					pthread_mutex_unlock(&holdBackQMutex);
					globalSeq=atoi(responseGlobalSeq);
					pthread_mutex_lock(&participantListMutex);
					participantListIterator=participantList.find(createKey(clientAddress));
					participantListIterator->second->seqNumber=atoi(responseLocalSeq);
					pthread_mutex_unlock(&participantListMutex);
					cout<<participantListIterator->second->username<<":"<<atoi(responseGlobalSeq)<<":"<<responseMsg;
				}
				else
				{
					//cout<<"In else : "<<response<<endl;
					//not the first message; insert the message into the queue
					pthread_mutex_lock(&holdBackQMutex);
					holdBackQ.insert(make_pair(atoi(responseGlobalSeq),createMessage(response,atoi(responseLocalSeq),atoi(responseGlobalSeq),createKey(clientAddress))));
					pthread_mutex_unlock(&holdBackQMutex);
					//check the queue for deliverable messages
					/*  cout<<"Hold back Queue - "<<endl;
					for(holdBackQIterator=holdBackQ.begin();holdBackQIterator!=holdBackQ.end();holdBackQIterator++)
					{
						cout<<holdBackQIterator->first<<":"<<participantList.find(holdBackQIterator->second->senderKey)->second->username<<":"<<holdBackQIterator->second->content;
					}
					cout<<"Searching for  : "<<globalSeq<<endl;  */
					for(holdBackQIterator=holdBackQ.find(globalSeq);holdBackQIterator!=holdBackQ.end();holdBackQIterator++)
					{
						//cout<<holdBackQIterator->first<<":"<<participantList.find(holdBackQIterator->second->senderKey)->second->username<<":"<<holdBackQIterator->second->content;
						//cout<<"Held msg :"<<holdBackQIterator->first<<":"<<holdBackQIterator->second->content<<endl;
						if(holdBackQIterator->first == globalSeq+1 || (holdBackQIterator->first == notificationGlobalSeq+1 && holdBackQIterator->first > globalSeq+1))
						{
							globalSeq=holdBackQIterator->first;
							participantListIterator=participantList.find(holdBackQIterator->second->senderKey);
							if(participantListIterator!=participantList.end())
							{
								strcpy(response,(holdBackQIterator->second->content).c_str());
								breakDownMsg();
								pthread_mutex_lock(&participantListMutex);
								participantListIterator->second->seqNumber=atoi(responseLocalSeq);
								pthread_mutex_unlock(&participantListMutex);
								cout<<participantListIterator->second->username<<":"<<atoi(responseGlobalSeq)<<":"<<atoi(responseLocalSeq)<<":"<<responseMsg;
								/* pthread_mutex_lock(&holdBackQMutex);
								holdBackQ.erase(holdBackQIterator);
								pthread_mutex_unlock(&holdBackQMutex); */
							}
							else
							{
								cout<<"Unknown host :"<<responseMsg;
							}
							//cout<<createKey(clientAddress)<<":"<<responseMsg;
						} 
						else if(holdBackQIterator->first > globalSeq+1 && holdBackQIterator->first > notificationGlobalSeq+1 )
						{
							//cout<<"globalSeq : "<<globalSeq<<" notificationGlobalSeq : "<<notificationGlobalSeq<<endl;
							//a sequence number is missing; send a request to all for retransmission;
							cout<<"Sending sequence lost request for global seq : "<<(globalSeq+1)<<endl;
							globalSeqLost.insert(make_pair(globalSeq+1,0));
							snprintf(msg,1000,"S3_:%d:%d:_",(globalSeq+1),0);
							if(multicast(SEQUENCELOST)<0)
							{
								cout<<"error in sending sequence lost request\n";
							}
							break;
						}
						/* else
						{
							cout<<"globalSeq : "<<globalSeq<<" notificationGlobalSeq : "<<notificationGlobalSeq<<endl;
							cout<<"Else Case : "<<holdBackQIterator->first<<" "<<holdBackQIterator->second->content;
						} */
						//cout<<holdBackQIterator->first<<" : "<<holdBackQIterator->second->content;
					}
					//cout<<endl;
				}
			}
			if(responseTag[0]=='C' && responseTag[1]=='0' && responseTag[2]=='A')				//chat msg ACK
			{
				txBufferIterator=txBuffer.find(atoi(responseGlobalSeq));
				pthread_mutex_lock(&txBufferMutex);
				if(txBufferIterator!=txBuffer.end())
				{
					ackListIterator=(txBufferIterator->second->ackList).find(createKey(clientAddress));
					if(ackListIterator!=(txBufferIterator->second->ackList).end())
					{
						//valid ACK; remove clientAddress from the ack list set
						(txBufferIterator->second->ackList).erase(ackListIterator);
					}
					if((txBufferIterator->second->ackList).empty())
					{
						//all clients have ACKed the message; remove from txBuffer
						std::map <int, struct message * >::iterator seqBufferIteratorR;
						for(seqBufferIteratorR=seqBuffer.begin();seqBufferIteratorR!=seqBuffer.end();seqBufferIteratorR++)
						{
							if((seqBufferIteratorR->second->globalSeq) == txBufferIterator->first)
							{
								//cout<<"Erasing from SeqBuffer : "<<seqBufferIteratorR->second->content;
								seqBuffer.erase(seqBufferIteratorR);
								break;
							}
						}
						txBuffer.erase(txBufferIterator);
					}
				}
				pthread_mutex_unlock(&txBufferMutex);
			}
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
		case 'S':
		if(responseTag[0]=='S' && responseTag[1]=='0' && responseTag[2]=='_')				//only for centralized sequencer
		{	
			//cout<<"Sequencing  : "<<response<<endl;
			//send ack to the requester indicating the request has been received
			sequencer(createKey(clientAddress), atoi(responseLocalSeq));
			/* snprintf(msg,1000,"S0A:%d:%d:%s", generatedGlobalSeq, atoi(responseLocalSeq), responseMsg);
			int n=sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
			if(n <0)
			{
				cout << "Error in sending Global sequence number"<<endl;
			}

			snprintf(msg,1000,"C0_:%d:%d:%s",generatedGlobalSeq, atoi(responseLocalSeq), responseMsg);
			n = multicast(SEQUENCED);
			if(n<0)
			{
				cout << "Error multicasting chat message to all members"<< endl;
			}
			else
			{
			// maintain holdback queue for all broadcast msg
			} */
			

		}			
		else if(responseTag[0]=='S' && responseTag[1]=='0' && responseTag[2]=='A')				//only for centralized sequencer
		{	
			//cout<<"Sequence response : "<<response;
			seqBufferIterator=seqBuffer.find(atoi(responseLocalSeq));
			if(seqBufferIterator!=seqBuffer.end())
			{
				seqBufferIterator->second->globalSeq=(atoi(responseGlobalSeq));
			}
			//update seqBuffer to reflect the globalSeq number
			//sequencer received the request 
			//remove it from Request queue.
			//requestQueue.erase(responseLocalSeq);
			//TODO add seqBuffer code here

		}
		else if(responseTag[0]=='S' && responseTag[1]=='1' && responseTag[2]=='A')
		{
			//use broadcast list code to update the holdback queue
			//ack_update(createKey(clientAddress),atoi(responseGlobalSeq));

		}
		else if(responseTag[0]=='S' && responseTag[1]=='2' && responseTag[2]=='_')			//sequence request
		{
			if(decentralized)
			{
				//this is a decentralized request for sequence
				//respond with a valid proposed sequence number
				proposedSeq=(globalSeq>proposedSeq)?(globalSeq+1):(proposedSeq+1);
				if(proposedSeq==5)
					proposedSeq++;
				snprintf(msg, 1000, "S2A:%d:%d:%s",proposedSeq,atoi(responseLocalSeq),responseMsg);
				if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
				{
					cout<<"Error in sending sequence proposal\n";
				}
				else
				{
					//add the message to a list for expecting an ACK
				}
			}
		}
		else if(responseTag[0]=='S' && responseTag[1]=='2' && responseTag[2]=='A')			//sequence response
		{
			if(decentralized)
			{
				seqBufferIterator=seqBuffer.find(atoi(responseLocalSeq));
				if(seqBufferIterator!=seqBuffer.end())
				{
					seqBufferIterator->second->globalSeq=(atoi(responseGlobalSeq) > seqBufferIterator->second->globalSeq)?atoi(responseGlobalSeq):seqBufferIterator->second->globalSeq;
					seqBufferIterator->second->ackCount++;
					//cout<<"received sequence response :"<<response<<"for :";
					//printMessage(seqBufferIterator->second);
					if(seqBufferIterator->second->ackCount>=participantList.size())
					{
						//sequence response received from all participants; send the message now;
						snprintf(msg, 1000,"C0_:%d:%d:%s",atoi(responseGlobalSeq),atoi(responseLocalSeq),responseMsg);
						if(multicast(SEQUENCEDDISTRIBUTED)<0)
						{
							cout<<"Error in multicasting sequenced message\n";
						}
					}
				}
				else
				{
					cout<<"Sequence response for unknown message; discarding "<<response<<endl;
				}
			}
		}
		else if(responseTag[0]=='S' && responseTag[1]=='3' && responseTag[2]=='_')			//sequence lost request 
		{
			cout<<"Searching for : "<<responseGlobalSeq<<endl;
			//check in hold back queue
			holdBackQIterator=holdBackQ.find(atoi(responseGlobalSeq));
			if(holdBackQIterator!=holdBackQ.end())
			{
				//found the message; send back to the requester
				strcpy(response,(holdBackQIterator->second->content).c_str());
				if (breakDownMsg()!=0)
				{
					cout<<"error in sequence lost response generation breakdown"<<endl;
				}
				snprintf(msg,1000,"S3A:%d:%d:%s:%d:%d:%s:%s",holdBackQIterator->second->globalSeq,holdBackQIterator->second->localSeq,responseTag,holdBackQIterator->second->globalSeq,holdBackQIterator->second->localSeq,(holdBackQIterator->second->senderKey).c_str(),responseMsg);
				cout<<"Searching for : "<<responseGlobalSeq<<"\nFound in holdBackQ; Responding : "<<holdBackQIterator->second->content;
				if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
				{
					cout<<"Error in sending sequence lost response : found in holdBack\n";
				}
			}
			else
			{
				//check seqBuffer
				seqBufferIterator=seqBuffer.find(atoi(responseGlobalSeq));
				if(seqBufferIterator!=seqBuffer.end())
				{
					strcpy(response,(seqBufferIterator->second->content).c_str());
					if (breakDownMsg()!=0)
					{
						cout<<"error in sequence lost response generation breakdown"<<endl;
					}
					//found the message; send back to the requester
					snprintf(msg,1000,"S3A:%d:%d:%s:%d:%d:%s:%s",seqBufferIterator->second->globalSeq,seqBufferIterator->second->localSeq,responseTag,seqBufferIterator->second->globalSeq,seqBufferIterator->second->localSeq,(seqBufferIterator->second->senderKey).c_str(),responseMsg);
					cout<<"Searching for : "<<responseGlobalSeq<<"\nFound in seqBuffer; Responding : "<<msg;
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending sequence lost response : found in seqBuffer\n";
					}
				}
				else
				{
					cout<<"Searching for : "<<responseGlobalSeq<<"\nNot found; Responding\n";
					snprintf(msg,1000,"S3N:%d:%d:_",atoi(responseGlobalSeq),atoi(responseLocalSeq));
					if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
					{
						cout<<"Error in sending sequence lost response : not found\n";
					}
				}
			}
			
		}
		else if(responseTag[0]=='S' && responseTag[1]=='3' && responseTag[2]=='A')			//sequence lost response 
		{
			if(atoi(responseGlobalSeq)==(globalSeq+1))					//found the sequence which was lost
			{
				cout<<"sequenceLost response : "<<responseMsg<<endl;
				if(breakDownMsg()==0)
				{
					//cout<<"After breakdown : "<<responseTag<<":"<<responseGlobalSeq<<":"<<responseLocalSeq<<":"<<responseMsg<<endl;
					strcpy(response,responseMsg);
					if(breakDownMsg()==0)
					{
						if(decentralized)
						{
							//cout<<"After breakdown : "<<responseTag<<":"<<responseGlobalSeq<<":"<<responseLocalSeq<<":"<<responseMsg<<endl;
							char *second, *third, *fourth;
							char *ipString = new char[20];
							char *portString = new char[5];
							second=strstr(responseMsg,":");
							if(second==NULL)
							{
								cout<<"Error in distributed sequence lost response break down - No PORT\n";
								return ;
							}
							third=strstr(second+1,":");
							if(third==NULL)
							{
								cout<<"Error in distributed sequence lost response break down - No Msg\n";
								return ;
							}
							strncpy(ipString,responseMsg,(second-responseMsg));
							ipString[second-responseMsg]='\0';
							strncpy(portString,second+1,(third-second-1));
							portString[third-second-1]='\0';
							//update clientAddress to reflect the details of the original sender
							bzero(&clientAddress,sizeof(clientAddress));
							clientAddress.sin_family=AF_INET;
							if(inet_pton(AF_INET,ipString, &(clientAddress.sin_addr))<=0)
							{
								cout<<"Error in sequence lost response break down - unable to get source IP\n";
							}
							clientAddress.sin_port=htons(atoi(portString));
							//cout<<"original Msg : "<<responseMsg<<" from : "<<ipString<<":"<<portString<<endl;
							strcpy(responseMsg,third+1);
							snprintf(response,1000,"%s:%s:%s:%s",responseTag,responseGlobalSeq,responseLocalSeq,responseMsg);
						}
						identify();
					}
				}
				else
				{
					//problem with the sequence lost response message;
				}
				
			}
		}
		else if(responseTag[0]=='S' && responseTag[1]=='3' && responseTag[2]=='N')	//sequence not found by someone;
		{
			globalSeqLostIterator=globalSeqLost.find(atoi(responseGlobalSeq));
			if(globalSeqLostIterator!=globalSeqLost.end())
			{
				globalSeqLostIterator->second++;
				if(globalSeqLostIterator->second >= participantList.size())
				{	
					cout<<"**********************************************\n";
					cout<<"Message with globalSeq = "<<globalSeqLostIterator->first<<" lost forever :(\n";
					cout<<"**********************************************\n";
					//NACK's from n or more participants; assuming message to be lost forever;
					globalSeq=globalSeqLostIterator->first+1;
					cout<<globalSeq<<" "<<globalSeqLostIterator->first<<endl;
					globalSeqLost.erase(globalSeqLostIterator);
				}
			}
		}
		else if(responseTag[0]=='S' && responseTag[1]=='4' && responseTag[2]=='_')	//sequence request was lost; sequencer has requested re transmission
		{
			seqBufferIterator=seqBuffer.find(atoi(responseLocalSeq));
			if(seqBufferIterator!=seqBuffer.end())				//message found in seqBuffer; retransmit to sequencer;
			{
				strcpy(msg,(seqBufferIterator->second->content).c_str());
				if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
				{
					cout<<"Error in sending sequence request lost retransmission\n";
				}
			}
			else			//message was not found in seqBuffer; inform the sequencer
			{
				snprintf(msg,1000,"S4A:%s:%s:_",responseGlobalSeq,responseLocalSeq);
				if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
				{
					cout<<"Error in sending sequence request lost retransmission\n";
				}
			}
		}
		else if(responseTag[0]=='S' && responseTag[1]=='4' && responseTag[2]=='A')	//sequence request was lost; sequencer has requested re transmission
		{
			//update the hold_back_queue to forget about that message; it is lost forever. FOREVER !!
			outer_itr = hold_back_queue.find(createKey(clientAddress));
			if(outer_itr!=hold_back_queue.end())
			{
				pthread_mutex_lock(&hold_back_queueMutex);
				(outer_itr->second).last_client_seq=(outer_itr->second).last_client_seq<atoi(responseLocalSeq)?atoi(responseLocalSeq):(outer_itr->second).last_client_seq;
				pthread_mutex_unlock(&hold_back_queueMutex);
			}
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

void *reliabilityThread(void *data)				//thread to check for ACK's and sequence response
{
	while(1)
	{
		threadSleep(0,50000000L);				//sleeping for 10 mSec
		//check tx buffer for non received ACK's\n
		std::map <int, struct txMessage * >::iterator txBufferIteratorR;
		for(txBufferIteratorR=txBuffer.begin();txBufferIteratorR!=txBuffer.end();txBufferIteratorR++)
		{
			txBufferCounterIterator=txBufferCounter.find(txBufferIteratorR->first);
			if(txBufferCounterIterator!=txBufferCounter.end())
			{
				(txBufferCounterIterator->second)++;
				if(txBufferCounterIterator->second >= 3 && txBufferCounterIterator->second <5)			//this message has not been ACKed in 3 cycles
				{
					snprintf(reliabilityMsg,1000,"S3A:%d:%d:%s",txBufferIteratorR->second->globalSeq,txBufferIteratorR->second->localSeq,(txBufferIteratorR->second->content).c_str());
					//cout<<"reliablity Msg : "<<reliabilityMsg<<endl;
					//retransmit to the un ACKed client
					std::set <string>::iterator ackListIteratorR;
					for(ackListIteratorR=(txBufferIteratorR->second->ackList).begin();ackListIteratorR!=(txBufferIteratorR->second->ackList).end();ackListIteratorR++)
					{
						std::map <string, struct participant * >::iterator participantListIteratorR; 	//iterator for the participant list
						participantListIteratorR=participantList.find(*ackListIteratorR);
						if(participantListIteratorR!=participantList.end())
						{
							if(sendto(chatSocketFD,reliabilityMsg,strlen(reliabilityMsg),0,(struct sockaddr *)&(participantListIteratorR->second->address),sizeof((participantListIteratorR->second->address)))<0)
							{
								cout<<"Error in sending retransmission from Reliability thread\n";
							}
							else
							{
								//cout<<"Reliability msg : "<<reliabilityMsg<<" Sent to  : "<<participantListIteratorR->second->username<<endl;
							}
						}
					}
				}
				else if(txBufferCounterIterator->second == 5 )			//this message has not been ACKed in 5 cycles and 2 retransmissions
				{
					//remove the message from txBuffer and txBufferCounter
					if(txBufferIteratorR!=txBuffer.end())
					{
						pthread_mutex_lock(&txBufferMutex);
						if(txBufferCounterIterator!=txBufferCounter.end())
						{
							txBufferCounter.erase(txBufferCounterIterator);
						}
						std::map <int, struct message * >::iterator seqBufferIteratorR;
						for(seqBufferIteratorR=seqBuffer.begin();seqBufferIteratorR!=seqBuffer.end();seqBufferIteratorR++)
						{
							if((seqBufferIteratorR->second->globalSeq) == txBufferIteratorR->first)
							{
								seqBuffer.erase(seqBufferIteratorR);
								break;
							}
						}
						txBuffer.erase(txBufferIteratorR);
						pthread_mutex_unlock(&txBufferMutex);
					}
				}
			}
			else													//detecting an un ACKed message for the first time
			{
				txBufferCounter.insert(make_pair(txBufferIteratorR->first,1));		
			}
		}
		//check seqBuffer for non received sequences
		std::map <int, struct message * >::iterator seqBufferIteratorR;
		for(seqBufferIteratorR=seqBuffer.begin();seqBufferIteratorR!=seqBuffer.end();seqBufferIteratorR++)
		{
			if(seqBufferIteratorR->second->globalSeq <=0)
			{
				//sequence request not answered
				seqBufferCounterIterator=seqBufferCounter.find(seqBufferIteratorR->first);
				if(seqBufferCounterIterator!=seqBufferCounter.end())
				{
					seqBufferCounterIterator->second++;
					if((seqBufferCounterIterator->second) % 5 ==0 )
					{
						strcpy(msg,(seqBufferIteratorR->second->content).c_str());
						//retransmit sequence request
						if(decentralized)
						{
							multicast(SEQUENCE);				//send sequence request to sequencer or all participants
						}
						else
						{
							int n=sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
							while(n<0)
							{
								n=sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
							}
						}
					}
				}
				else
				{
					seqBufferCounter.insert(make_pair(seqBufferIteratorR->first,1));
				}
			}
		}
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
					initializeSequencer();
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
			for(participantListIteratorHB=participantList.begin(); participantListIteratorHB!=participantList.end();participantListIteratorHB++)
			{ 
				if(leader==NULL || participantListIteratorHB==participantList.end() || participantListIteratorHB->second==NULL)
				{
					cout<<"breaking on null leader\n";
					continue;
				}

				if(participantListIteratorHB->second!=leader )//&& participantListIteratorHB->second->isReady)
				{
					//cout<<participantListIteratorHB->second->username<<endl;
					//check if a participant has responded or not
					if(heartBeatMap.find(participantListIteratorHB->first)==heartBeatMap.end())			
					{
						//cout<<participantListIteratorHB->second->username<<endl;
						responseCountIterator=responseCount.find(participantListIteratorHB->first);
						if(responseCountIterator==responseCount.end())
						{
							responseCount.insert(make_pair(participantListIteratorHB->first,1));
						}
						else
						{
							responseCountIterator->second=responseCountIterator->second+1;
							cout<<participantListIteratorHB->second->username<<" count - "<<responseCountIterator->second<<endl;
							if(responseCountIterator->second >=5)
							{
								cout<<participantListIteratorHB->second->username<<" is dead\n";
								responseCountIterator=responseCount.find(participantListIteratorHB->first);
								if(responseCountIterator!=responseCount.end())
								{
									responseCount.erase(responseCountIterator);
								}
								if(hold_back_queue.find(participantListIteratorHB->first)!=hold_back_queue.end())
								{
									cout<<"Erasing "<<(participantListIteratorHB->first)<<endl;
									pthread_mutex_lock(&hold_back_queueMutex);
									hold_back_queue.erase((participantListIteratorHB->first));
									pthread_mutex_unlock(&hold_back_queueMutex);
								}
								else
								{
									cout<<"Error in removing participant from hold_back_queue\n";
								}
								for(txBufferIteratorHB=txBuffer.begin();txBufferIteratorHB!=txBuffer.end();txBufferIteratorHB++)
								{
									pthread_mutex_lock(&txBufferMutex);
									ackListIteratorHB=(txBufferIteratorHB->second->ackList).find(participantListIteratorHB->first);
									if(ackListIteratorHB!=(txBufferIteratorHB->second->ackList).end())
									{
										(txBufferIteratorHB->second->ackList).erase(ackListIteratorHB);
									}
									if((txBufferIteratorHB->second->ackList).empty())
									{
										//all clients have ACKed the message; remove from txBuffer
										txBuffer.erase(txBufferIteratorHB);
									}
									pthread_mutex_unlock(&txBufferMutex);
								}
								pthread_mutex_lock(&participantListMutex);
								participantList.erase(participantListIteratorHB);
								pthread_mutex_unlock(&participantListMutex);
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
					participantListIteratorHB=participantList.find(createKey(leader->address));
					if(participantListIteratorHB!=participantList.end())			
					{
						cout<<participantListIteratorHB->second->username<<" is removed\n";
						pthread_mutex_lock(&participantListMutex);
						participantList.erase(participantListIteratorHB);
						pthread_mutex_unlock(&participantListMutex);
					}
					if(participantList.size()==1)
					{
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
						initializeSequencer();
						multicast(LEADER);
						sendParticipantList(MULTICAST);							//send participant list to all participants
						printParticipantList();
					}
					else
					{
						participantListIteratorHB=participantList.find(createKey(self->address));
						participantListIteratorHB++;
						if(participantListIteratorHB==participantList.end())
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
							initializeSequencer();
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
		if(decentralized)
		{
			multicast(SEQUENCE);				//send sequence request to sequencer or all participants
		}
		else
		{
			pthread_mutex_lock(&seqBufferMutex);
			localSeq++;
			chatMsg[0]='\0';
			snprintf(chatMsg,1000,"S0_:0:%d:%s:C0_:0:%d:%s",localSeq,createKey(selfAddress),localSeq,msg);
			seqBuffer.insert(make_pair(localSeq,createMessage(chatMsg,localSeq,createKey(selfAddress))));
			pthread_mutex_unlock(&seqBufferMutex);
			if(localSeq!=3 || defaultPORT!=8673)
			{
				//cout<<"Sending sequence request for : "<<chatMsg<<endl;
				int n=sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
				while(n<0)
				{
					n=sendto(chatSocketFD,chatMsg,strlen(chatMsg),0,(struct sockaddr *)(&(leader->address)),sizeof(leader->address));
				}
			}
		}
		//sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&clientAddress,sizeof(clientAddress));
	}
	cout<<"**********************************\n";
	cout<<"Exiting chat\nSee you later, alligator ! :)\n"; 
	cout<<"**********************************\n";
	exit(1);
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
			//if(response[0]=='S')
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
		snprintf(msg,1000,"N0_:0:%d:%s",++localSeq,argv[1]);
		if(sendto(chatSocketFD,msg,strlen(msg),0,(struct sockaddr *)&joinClientAddress,sizeof(joinClientAddress))<0)
		{
			printf("error in sending join request\n");
			exit(1);
		}
		socklen_t len=sizeof(clientAddress);
		n=recvfrom(chatSocketFD,response,1000,0,(struct sockaddr *)&clientAddress,&len);
		if(n<0)
		{
			printf("Error in receiving Join confirmation");
			exit(1);
		}
		cout<<"response : "<<response<<"\nFrom : "<<createKey(clientAddress)<<endl;
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
		printf("Error in creating network  thread\n");
		exit(1);
	}	
	if(pthread_create(&heartBeatThreadID,NULL, heartBeatThread,NULL))
	{
		printf("Error in creating heart beat thread\n");
		exit(1);
	}
	if(pthread_create(&electionThreadID,NULL, electionThread,NULL))
	{
		printf("Error in creating election thread\n");
		exit(1);
	}
	//if(pthread_create(&reliabilityThreadID,NULL, reliabilityThread,NULL))
	{
		printf("Error in creating reliability thread\n");
		//exit(1);
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
		printf("Error joining heart beat thread \n");
	}
	if(pthread_join(electionThreadID, NULL))
	{
		printf("Error joining election thread \n");
	}
	//if(pthread_join(reliabilityThreadID, NULL))
	{
		printf("Error joining reliability thread \n");
	}
	return 0;
}