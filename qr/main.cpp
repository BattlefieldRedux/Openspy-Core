#include "main.h"
#include "server.h"
#include "Client.h"
#include "filter.h"
modInfo moduleInfo = {"qr","Gamespy Query and Reporting server(heartbeat)"};
modLoadOptions servoptions;
serverInfo server;
#ifndef _WIN32
GeoIP *gi;
#endif
bool findMatchingServers(qrServerList *listData, char *sendmodule) {
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	boost::shared_ptr<Client> user;
	serverList slist;
	int i=0;
	while(iterator != end) {
		user=*iterator;
		++iterator;
		if(!(user && user->tryLockKeys())) continue;
		if(user->getGameInfo() == listData->game) {
			if(filterMatches(listData->filter,user) && user->isServerRegistered()) {
				slist.serverKeys = user->copyServerKeys();
				slist.port = user->getServerPort();
				slist.ipaddr = user->getServerAddress();
				slist.country = user->getCountry();
				listData->server_list.push_back(slist);
			}
		}
		user->unlockKeys();
		i++;	
	}
	listData->numServers = i;
	return i != 0;
}
void findMatchingServers_GetRules(qrServerRules *rulesData, char *sendmodule) {
	boost::shared_ptr<Client> user;
	user = find_user(rulesData->ipaddr,rulesData->port);
	if(user) {
		//getRules allocates data which must be freed
		rulesData->server_rules = user->getRules();
		user->unlockKeys();
	}
}
void handleClient(int sd,struct sockaddr_in *si_other,char *buff,int len) {
	char send[256];
	char *p = (char *)&send;
	int blen = 0;
	int slen = sizeof(sockaddr_in);
	boost::shared_ptr<Client> user;
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=find_user(si_other);
	if(iterator == end) { //unregistered user, create
		user = boost::make_shared<Client>(sd,si_other);
		server.client_list.push_back(user);
	} else if(!*iterator) {
		user = boost::make_shared<Client>(sd,si_other);
		*iterator = user;
	} else user = *iterator;
	user->handleIncoming(buff,len);
	if(user->deleteMe)
		iterator->reset();
	
}
void *openspy_mod_run(modLoadOptions *options) {
  int sd;
  int len;
  char buf[MAX_DATA_SIZE + 1];
  #ifndef _WIN32
  gi = GeoIP_new(GEOIP_STANDARD);
  #else
  WSADATA ws;
  WSAStartup(MAKEWORD(1,0),&ws);
  #endif
  struct sockaddr si_other;
  memcpy(&servoptions,options,sizeof(modLoadOptions));
  socklen_t slen = sizeof(si_other);
  struct sockaddr_in si_me;
  if((sd=socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP)) == -1)
   return NULL;
  memset((char *)&si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(QRPORT);
  si_me.sin_addr.s_addr = options->bindIP;
  if(bind(sd,(struct sockaddr *)&si_me,sizeof(si_me)) == -1)
   return NULL;
	struct timeval tv;
  for(;;) {
    memset((char *)&buf,0, sizeof(buf));
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv);
    len = recvfrom(sd,(char *)&buf,sizeof(buf), 0, (struct sockaddr *)&si_other, &slen);
    if(len < 0) { //timeout, do keep alives and delete clients who have expired
	checkTimeouts();
	continue;
    }
    buf[len] = 0;
//  makeStringSafe((char *)&buf, sizeof(buf));
    handleClient(sd,(struct sockaddr_in *)&si_other,(char *)&buf,len);
  }
	return NULL;
}
void sendClientMessage(qrClientMsg *cmsg) {
	boost::shared_ptr<Client> c = find_user(cmsg->toip,cmsg->toport);
	if(c) {
		c->sendMsg(cmsg->data,cmsg->len);
		c->unlockKeys();
	}
}
bool openspy_mod_query(char *sendmodule, void *data,int len) {
	qrServerMsg *msg = (qrServerMsg *)data;
	qrServerList *listData;
	qrServerRules *rulesData;
	qrClientMsg *cmsg;
	switch(msg->msgID) {
		case EQRMsgID_GetServer:
			listData = (qrServerList *)msg->data;
			return findMatchingServers(listData,sendmodule);
		case EQRMsgID_GetServerRules:
			rulesData = (qrServerRules *)msg->data;
			findMatchingServers_GetRules(rulesData,sendmodule);
			return true;
		case EQRMsgID_ClientMessage:
			cmsg = (qrClientMsg *)msg->data;
			sendClientMessage(cmsg);
			return true;
	}
	return false;
}
modInfo *openspy_modInfo() {
	return &moduleInfo;
}
void checkTimeouts() {
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	boost::shared_ptr<Client> user;
	time_t timotim = time(NULL)-QR_PING_TIME;
	while(iterator != end) {
		user=*iterator;
		if(!user) { ++iterator; continue; }
		if(timotim > user->getLastPing())
			iterator->reset();
		++iterator;
	}
}
