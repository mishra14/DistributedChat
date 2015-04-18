/* Centralized Sequencer to assign sequence numbers to imcoming requests*/

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
	map<string, struct LastSeen > :: iterator itr2;
	struct LastSeen lastseen;


	itr = hold_back_queue.find(ip);
	itr2 = itr;

	if(itr == hold_back_queue.end())
	{
		// this is the first request from this IP
		cout << "First req from this IP"<< ip<<":"<< "seq:"<< client_seq<< endl;
		lastseen = itr->second;
		lastseen.last_client_seq = client_seq;
		//m_timers.insert(std::pair<std::string,timerInfo>(timerName, timerInfo(GetTime(),0)));
		hold_back_queue.insert(pair<string, LastSeen>(ip, lastseen));
		return ++global_seq;
	}

	else 
	{	
		lastseen = (itr->second);
		if((itr->second).client_seq_nos.empty() && (itr->second).last_client_seq == client_seq-1) 
		{//vector corresponding to this IP is empty but this is not the first request
			cout <<"IP exists but vector empty:"<< ip<< " Previous seen from this IP:" << (itr->second).last_client_seq<< endl;
			(itr->second).last_client_seq = client_seq;
			return ++global_seq;
		}

		else
		{	//Out of local sequence number
			lastseen = itr->second;
			cout << "Out of seq msg:" << client_seq<< endl;
			// add last_seq number too?
			(itr->second).client_seq_nos.push_back(client_seq);
			hold_back_queue.insert(make_pair(ip,(itr->second)));
			sort((itr->second).client_seq_nos.begin(), (itr->second).client_seq_nos.end());
			cout << "last seen seq for this ip:"<<(itr->second).last_client_seq<< endl;
			
			while((itr->second).last_client_seq == (itr->second).client_seq_nos[0]-1)
			{	
				(itr->second).last_client_seq = (itr->second).client_seq_nos[0];
				
				cout<<"Giving seq no to :" << (itr->second).last_client_seq <<":"<<++global_seq<< endl;;
				(itr->second).client_seq_nos.erase((itr->second).client_seq_nos.begin());

			}
			return 0;
		}
	}
}


int main()
{
	string x[]= {"12344", "344545","3421","23535","12344","12344","12344","12344","12344"};
	int seq[] = {1, 1,1,1,2,4,5,3};
	int gs;
	for(int i = 0; i<8; i++)
	{	
		gs =sequencer(x[i], seq[i]);
		if(gs >0)
		{
			cout << "Seq No for "<< seq[i] << " :"<< gs<< endl;
		}
	}

 
}
