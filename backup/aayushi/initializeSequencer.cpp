

struct participant									//holds the data for one participant
{
	struct sockaddr_in address;
	int seqNumber;
	string username;
};

struct LastSeen
{
 	int last_client_seq;
 	vector<int> client_seq_nos;
};

std::map <string, struct participant * > participantList;	
std::map <string, struct participant * >::iterator participantListIterator; 	//iterator for the participant list

map<string, struct LastSeen > :: iterator itr;

int initializeSequencer()
{
	for( participantListIterator = participantList.begin(); participantListIterator != participantList.end() ; participantListIterator++)
	{
		if(participantListIterator->second->seqNumber > 0)
		{
			//a new sequencer was elected. Update lastSeen localSeq from each IP
			struct Lastseen lastseen;
			lastseen.last_client_seq = participantListIterator->second->seqNumber;
			hold_back_queue.insert(make_pair(participantListIterator->first, lastseen));
		}	
	}
}
