#include "main.h"
#include "server.h"
#include "Client.h"
#include <qr/structs.h>
#include "structs.h"
MYSQL *conn;
modInfo moduleInfo = {"serverbrowsing","GameSpy ServerBrowsing server"};
modLoadOptions servoptions;
serverInfo server;
bool do_db_check();
int getnfds(fd_set *rset) {
	int hsock = 0;
	boost::shared_ptr<Client> c;
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	while(iterator != end) {
		c=*iterator;
		++iterator;
		if(!c) continue;
		int sock = c->getSocket();
		if(sock > hsock) hsock = sock;
		FD_SET(sock,rset);
	}
	return hsock;
}
void processClients(fd_set *rset) {
	boost::shared_ptr<Client> c;
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	time_t timotim = time(NULL)-SB_TIMEOUT_TIME;
	while(iterator != end) {
		c=*iterator;
		if(!c) { ++iterator; continue; }
		if(timotim > c->getLastPing())
			shutdown(c->getSocket(),SHUT_RDWR);
		c->processConnection(rset);
		if(c->deleteMe)
			iterator->reset();
		++iterator;
	}
}
void *openspy_mod_run(modLoadOptions *options)
{
  int sda,sd;
  socklen_t psz;
  fd_set  rset;
  struct sockaddr_in peer;
  struct sockaddr_in user;
  memset(&peer,0,sizeof(peer));
  int on=1;
  memcpy(&servoptions,options,sizeof(modLoadOptions));
  conn = mysql_init(NULL);
  mysql_options(conn,MYSQL_OPT_RECONNECT, (char *)&on);
  /* Connect to database */
  if (!mysql_real_connect(conn, servoptions.mysql_server,
      servoptions.mysql_user, servoptions.mysql_password, servoptions.mysql_database, 0, NULL, CLIENT_MULTI_RESULTS)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return NULL;
  }
  peer.sin_port        = htons(SBPORT);
  peer.sin_family      = AF_INET;
  peer.sin_addr.s_addr = servoptions.bindIP;
	
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))
      < 0) return NULL;
    if(bind(sd, (struct sockaddr *)&peer, sizeof(struct sockaddr_in))
      < 0) return NULL;
    if(listen(sd, SOMAXCONN)
      < 0) return NULL;
    struct timeval timeout;
    memset(&timeout,0,sizeof(struct timeval));
    for(;;) {
	int hsock;
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	hsock = getnfds(&rset);
	if(hsock < sd) hsock = sd;
	timeout.tv_sec = SB_TIMEOUT_TIME;
	if(select(hsock+1, &rset, NULL, NULL, &timeout)
          < 0) continue;
        if(FD_ISSET(sd, &rset)) {
	} else {
		processClients(&rset);
//		checkExpiry();
		continue;
	}
	psz = sizeof(struct sockaddr_in);
        sda = accept(sd, (struct sockaddr *)&user, &psz);
        if(sda <= 0) continue;
	if(!do_db_check()) {
		const char *errmsg = "ERROR :Closing Link: s (DATABASE ERROR - TRY AGAIN LATER)\r\n";
		send(sda,errmsg,strlen(errmsg),MSG_NOSIGNAL);
		close(sd);
		continue;
	}
	boost::shared_ptr<Client> c = boost::make_shared<Client>(sda,user);
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	for(;;) {
		if(iterator == end) { server.client_list.push_back(c); break; }
		else if(!*iterator) { *iterator = c; break; }
		else ++iterator;
	}
	//handleConnection(sda,&peer);
    }
}
void pushServer(sbPushMsg *msg) {
	boost::shared_ptr<Client> c;
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	serverList slist;
	slist.country = msg->country;
	slist.ipaddr = msg->ipaddr;
	slist.port = msg->port;
	slist.serverKeys = msg->keys;
	while(iterator != end) {
		c=*iterator;
		++iterator;
		if(!c) continue;
		if(c->wantsUpdates()) {
			if(msg->game == c->getQueryGame()) {
				c->pushServer(slist);
			}
		}
	}
}
void deleteServer(sbPushMsg *msg) {
	boost::shared_ptr<Client> c;
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator iterator=server.client_list.begin();
	boost::container::stable_vector< boost::shared_ptr<Client> >::iterator end=server.client_list.end();
	serverList slist;
	slist.country = msg->country;
	slist.ipaddr = msg->ipaddr;
	slist.port = msg->port;
	slist.serverKeys = msg->keys;
	while(iterator != end) {
		c=*iterator;
		++iterator;
		if(!c) continue;
		if(c->wantsUpdates()) {
			if(msg->game == c->getQueryGame()) {
				c->delServer(slist);
			}
		}
	}
}
bool openspy_mod_query(char *sendmodule, void *data,int len) {
	sbServerMsg *msg = (sbServerMsg *)data;
	sbPushMsg *pushmsg;
	pushmsg = (sbPushMsg *)msg->data;
	switch(msg->msgID) {
		case ESBMsgID_PushServer:
		pushServer(pushmsg);
		break;
		case ESBMsgID_DeleteServer:
		deleteServer(pushmsg);
		break;
		default:
		printf("unknown query\n");
		break;
	}
	return true;
}
modInfo *openspy_modInfo() {
	return &moduleInfo;
}

bool do_db_check() {
	int mysql_status = mysql_ping(conn);
	switch(mysql_status) {
		case CR_COMMANDS_OUT_OF_SYNC: {
			servoptions.logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server out of sync\n");
			break;
		}
		case CR_SERVER_GONE_ERROR: {
			servoptions.logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server has gone away\n");
			break;
		}
		case CR_UNKNOWN_ERROR: {
			servoptions.logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server has an unknown connection error\n");
			break;
		}
	}
	mysql_status = mysql_ping(conn); //check if reconnect was successful
	if(mysql_status != 0) {
		servoptions.logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server reconnect was unsuccessful\n");
	}
	return mysql_status == 0;
}
