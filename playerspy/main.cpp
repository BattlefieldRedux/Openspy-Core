#include "main.h"
#include "server.h"
#include "Client.h"
#define DISCONNECT_TIME 300
playerspyServer server;
modInfo moduleInfo = {"playerspy","GameSpy Presence Server Clone"};
modInfo *openspy_modInfo() {
	return &moduleInfo;
}
bool checkPing(Client *c) {
	time_t now = time(NULL);
	if(c->getLastPacket()+DISCONNECT_TIME < now) {
		shutdown(c->getSocket(),SHUT_RDWR);
	}
	return false;
}
void processClients(fd_set *rset) {
	Client *c;
	boost::unordered_set <Client *> clist = server.client_list;
	boost::unordered_set<Client *>::iterator iterator=clist.begin();
		 while(iterator != clist.end()) {
			c=*iterator;
			if(checkPing(c)) {
				iterator = clist.begin(); 
				continue;
			}
			c->mainLoop(rset);
			if(c->deleteMe)
				reallyDeleteClient(c);
			iterator++;
		 }
}
int getnfds(fd_set *rset) {
	int hsock = 0;
	Client *c;
	boost::unordered_set <Client *> clist = server.client_list;
	boost::unordered_set<Client *>::iterator iterator=clist.begin();
		 while(iterator != clist.end()) {
			c=*iterator;
			int sock = c->getSocket();
			if(sock > hsock) hsock = sock;
			FD_SET(sock,rset);
			iterator++;
		 }
	return hsock;
}
void *openspy_mod_run(modLoadOptions *options) {
#ifdef WIN32
	WSADATA ws;
	WSAStartup(MAKEWORD(1,0),&ws);
#endif
	int on=1,sda;
#ifdef WIN32
	int psz;
#else
	socklen_t psz;
#endif
	u_long on_a=1;
	struct  sockaddr_in peer;
	int sdl;
	fd_set  rset;
	MYSQL_RES *res;
	MYSQL_ROW row;
    peer.sin_addr.s_addr = options->bindIP;
    peer.sin_port        = htons(GPCM_PORT);
    peer.sin_family      = AF_INET;
    server.conn = mysql_init(NULL);
    server.options = options;
	mysql_options(server.conn,MYSQL_OPT_RECONNECT, (char *)&on_a);
   /* Connect to database */
   if (!mysql_real_connect(server.conn, options->mysql_server,
         options->mysql_user, options->mysql_password, options->mysql_database, 0, NULL, CLIENT_MULTI_RESULTS)) {
      fprintf(stderr, "%s\n", mysql_error(server.conn));
      return NULL;
   }
   server.games = options->games;
   server.num_games = options->totalgames;
    sdl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sdl < 0) return NULL;
    if(setsockopt(sdl, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))
      < 0) return NULL;
    if(bind(sdl, (struct sockaddr *)&peer, sizeof(struct sockaddr_in))
      < 0) return NULL;
    if(listen(sdl, SOMAXCONN)
      < 0) return NULL;
	struct timeval timeout;
	memset(&timeout,0,sizeof(struct timeval));
	timeout.tv_usec = 800000;
	int hsock;
    for(;;) {
	FD_ZERO(&rset);
	FD_SET(sdl, &rset);
	hsock = getnfds(&rset);
	if(hsock < sdl) hsock = sdl;
	Client *c=NULL;
	clientParams *param;

	if(select(hsock+1, &rset, NULL, NULL, NULL)
          < 0) continue;

        if(FD_ISSET(sdl, &rset)) {
	} else {
		processClients(&rset);
		continue;
	}
        psz = sizeof(struct sockaddr_in);
        sda = accept(sdl, (struct sockaddr *)&peer, &psz);
        if(sda <= 0) continue;
	if(!do_db_check()) { 
		close(sda);
		continue;
	}
	param = (clientParams *)malloc(sizeof(clientParams)); //its up to the thread to free this
	param->sd=sda;
	memcpy(&param->peer,&peer,sizeof(struct sockaddr));
	c=new Client(param);
	server.client_list.insert(c);
    }
      return NULL;
}
bool openspy_mod_query(char *sendmodule, void *data,int len) {
	return false;
}
bool do_db_check() {
	int mysql_status = mysql_ping(server.conn);
	switch(mysql_status) {
		case CR_COMMANDS_OUT_OF_SYNC: {
			server.options->logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server out of sync\n");
			break;
		}
		case CR_SERVER_GONE_ERROR: {
			server.options->logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server has gone away\n");
			break;
		}
		case CR_UNKNOWN_ERROR: {
			server.options->logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server has an unknown connection error\n");
			break;
		}
	}
	mysql_status = mysql_ping(server.conn); //check if reconnect was successful
	if(mysql_status != 0) {
		server.options->logMessageProc(moduleInfo.name,LOGLEVEL_ERROR,"MySQL server reconnect was unsuccessful\n");
	}
	return mysql_status == 0;
}
