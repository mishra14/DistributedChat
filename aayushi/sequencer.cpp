#include <string>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

int global_seq = 0;

struct LastSeen
{
 	int last_client_seq;
 	vector<int> client_seq_nos;
};

//ip of client, client_seq
map<string, struct LastSeen> hold_back_queue;

int sequencer(string ip, int client_seq)
{
	map<string, struct LastSeen > :: iterator itr;
	struct LastSeen lastseen;


	itr = hold_back_queue.find(ip);

	if(itr == hold_back_queue.end())
	{
		// this is the first request from this IP
		lastseen = itr->second;
		lastseen.last_client_seq = client_seq;
		return ++global_seq;
	}

	else 
	{	
		lastseen = itr->second;
		if(lastseen.client_seq_nos.empty() && lastseen.last_client_seq == client_seq-1) 
		{//vector corresponding to this IP is empty but this is not the first request
			lastseen.last_client_seq = client_seq;
			return ++global_seq;
		}

		else
		{	//add the 
			lastseen.client_seq_nos.push_back(client_seq);
			hold_back_queue.insert(make_pair(ip,lastseen));
			sort(lastseen.client_seq_nos.begin(), lastseen.client_seq_nos.end());

			while(lastseen.last_client_seq == lastseen.client_seq_nos[0]-1)

			{
				lastseen.last_client_seq = last_client_seq.client_seq_nos[0];
				
				//sendto(sockfd,response,strlen(response),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr));
				lastseen.client_seq_nos.erase(lastseen.client_seq_nos.begin());

			}
		}
	}
}