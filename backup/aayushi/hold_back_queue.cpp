#include <map>
#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

 map <int, string> hold_back_queue;

 string chat;
 int seq;


void add_to_queue(int seq_no, string mesg)
{
	hold_back_queue[seq_no] = mesg;
	//return 1; // indicating successfully added to the queue
}

int temp()
{
	int i = 0; 
	string mesg[] ={"Hello!", "How are you?", "I am fine"};

	for(i; i<3; i++)
	{

		add_to_queue(i, mesg[i]);
		//cout << mesg[0];
	}

	map <int, string>:: iterator it = hold_back_queue.begin();
   	for(;it!=hold_back_queue.end();it++)
    	{
    		cout<< "Seq no: "<< it ->first << "	 Mesg :"<<it->second<<endl;
    	}
    	
    return 0;
	
}

void tokenize()
{	
	string myString = "This is so stupid.";
	char_separator<char> sep(" ")
	tokenizer<char_separator<char>> tokens(myString, sep);
	for (const auto& t : tokens) {
        cout << t << "." << endl;
    }

}



int main()
{	

	tokenize();
	return 0;
	// This is a chat message
	//string mesg = "C1_:12:This is a mesg:  ";

	//if(mesg[0].at(0)=='C' && mesg[0].at(1)=='1' && mesg[0].at(2) == '_')
	
	//cout << mesg[0].at(0)<< endl;

}