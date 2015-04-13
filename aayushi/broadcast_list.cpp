<<<<<<< HEAD
#include <map>
#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;


 // map of ips to which msgs were broadcast
 map<int,map <string, bool> > sent_message_details;
 map <string, bool> all_ip;


// add members in the broadcast list to keep track of acknowledgements received

void add_ip(string ip)
{
	all_ip[ip] = false;
	
}


//method to update broadcast list after receiving ACK from members
void ack_update(string ip, int seq_no)
{
	map <string, bool> :: iterator inner_tree;
    map <int, map <string, bool> >:: iterator outer_tree ;

    outer_tree = sent_message_details.find(seq_no);
    if(outer_tree != sent_message_details.end())
    {
    	inner_tree = outer_tree->second.find(ip);
    	if(inner_tree != outer_tree->second.end())
    	{
    		inner_tree->second = true;
    	}
    	else
    	{
    		cout << "IP not in the broadcast list" << endl;
    		exit(1);
    	}
    }

    else
    {
    	cout << "This message was not found in the broadcast list" << endl;
    	exit(1);
    }
}

int check_broadcast_list()
{
	map <string, bool> :: iterator inner_tree;
    map <int, map <string, bool> >:: iterator outer_tree ;
    int counter;
    for(outer_tree= sent_message_details.begin(); outer_tree != sent_message_details.end(); outer_tree++)
    {
    	for( inner_tree = outer_tree->second.begin() ; inner_tree != outer_tree->second.end() ; inner_tree++)
    	{
    		if(inner_tree->second == false)
    		{
    			return 0; // indicating not all members have sent an ACK 
    		}
            else
            {
                cout << outer_tree->first<< ":"<< inner_tree->first << ":" << inner_tree->second<< endl;
            }
    	}
    }
}


//test script
//Remove everything under this comment
int main()
{
    string ips[] = {"1234", "4567", "34673" ,"24546"};
    int i;

    for(i = 0; i<4; i++)
    {
        add_ip(ips[i]);
    }

    for(i = 0; i<3; i++)
    {
        sent_message_details.insert(make_pair(i,all_ip));
    }

    ack_update("1234", 0);
    ack_update("4567", 0);
    ack_update("34673", 0);
    ack_update("24546", 0);
    ack_update("1234", 1);
    if(check_broadcast_list() == 0)
    {
        cout << "Wait for acks"<< endl;
    }

    else
    {
        cout<< "Remove the message from map" << endl;
    }
    return 1;
}
=======
#include <map>
#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;


 // map of ips to which msgs were broadcast
 map<int,map <string, bool> > sent_message_details;
 map <string, bool> all_ip;


// add members in the broadcast list to keep track of acknowledgements received

void add_ip(string ip)
{
	all_ip[ip] = false;
	
}


//method to update broadcast list after receiving ACK from members
void ack_update(string ip, int seq_no)
{
	map <string, bool> :: iterator inner_tree;
    map <int, map <string, bool> >:: iterator outer_tree ;

    outer_tree = sent_message_details.find(seq_no);
    if(outer_tree != sent_message_details.end())
    {
    	inner_tree = outer_tree->second.find(ip);
    	if(inner_tree != outer_tree->second.end())
    	{
    		inner_tree->second = true;
    	}
    	else
    	{
    		cout << "IP not in the broadcast list" << endl;
    		exit(1);
    	}
    }

    else
    {
    	cout << "This message was not found in the broadcast list" << endl;
    	exit(1);
    }
}

int check_broadcast_list()
{
	map <string, bool> :: iterator inner_tree;
    map <int, map <string, bool> >:: iterator outer_tree ;
    int counter;
    for(outer_tree= sent_message_details.begin(); outer_tree != sent_message_details.end(); outer_tree++)
    {
    	for( inner_tree = outer_tree->second.begin() ; inner_tree != outer_tree->second.end() ; inner_tree++)
    	{
    		if(inner_tree->second == false)
    		{
    			return 0; // indicating not all members have sent an ACK 
    		}
            else
            {
                cout << outer_tree->first<< ":"<< inner_tree->first << ":" << inner_tree->second<< endl;
            }
    	}
    }
}


//test script
//Remove everything under this comment
int main()
{
    string ips[] = {"1234", "4567", "34673" ,"24546"};
    int i;

    for(i = 0; i<4; i++)
    {
        add_ip(ips[i]);
    }

    for(i = 0; i<3; i++)
    {
        sent_message_details.insert(make_pair(i,all_ip));
    }

    ack_update("1234", 0);
    ack_update("4567", 0);
    ack_update("34673", 0);
    ack_update("24546", 0);
    ack_update("1234", 1);
    if(check_broadcast_list() == 0)
    {
        cout << "Wait for acks"<< endl;
    }

    else
    {
        cout<< "Remove the message from map" << endl;
    }
    return 1;
}
>>>>>>> ec2cceddd9b266cc02c8dd3e4c46de19b6c0d5b2
