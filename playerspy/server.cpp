#include "server.h"
extern playerspyServer server;
void deleteClient(Client *client) {
	client->deleteMe = true;
}
void reallyDeleteClient(Client *client) {
/*
	boost::unordered_set<Client *>::iterator iterator;
	iterator=server.client_list.begin();
	while(iterator != server.client_list.end()) {
		if(*iterator==client) {
			iterator = server.client_list.erase(iterator);
*/
	server.client_list.erase(client);
	delete client;
/*
		} else
		iterator++;

	}
*/
}
Client *getProfile(int profileid) {
	Client *client = NULL;
	boost::unordered_set<Client *>::iterator iterator;
	iterator=server.client_list.begin();
	while(iterator != server.client_list.end()) {
		client = *iterator;
		if(client->getProfileID() == profileid) return client;
		iterator++;

	}
	return NULL;
}
void sendStatusUpdateToBuddies(Client *c) {
//	bool Client::hasBuddy(Client *c)
	Client *client = NULL;
	boost::unordered_set<Client *>::iterator iterator;
	iterator=server.client_list.begin();
	while(iterator != server.client_list.end()) {
		client = *iterator;
		if(client->hasBuddy(c)) {
			client->sendBuddyStatus(c);
		}
		iterator++;

	}
}
