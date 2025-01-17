#ifndef _SERVER_INC
#define _SERVER_INC
#include "main.h"
#include "structs.h"
#include <sstream>
void deleteClient(Client *client);
void reallyDeleteClient(Client *client);
void deleteChannel(Channel *client);
void sendToAll(char *str, ...);
void sendWallops(Client *sender,char *msg);
void sendToAllOpers(char *str,...);
void sendToAllOpers(uint32_t rights,char *str,...);
bool nameInUse(char *name);
bool nameValid(char *name); 
bool validChan(char *name);
Channel *find_chan(char *name);
Client *find_user(char *name);
bool onChanTogether(Client *s, Client *target);
bool canSeeIp(Client *s,Client *target);
void sendOnceAllChan(Client *source, bool sendtosource,char *str,...);
int numChans(Client *user);
void removeFromAllChans(Client *user);
EModeFlags getModeStr(char *str);
void addUserMode(Client *setter, char *target, char *modestr, bool addSQL = true);
void applyUserMode(userMode *usermode);
bool usermodeMatches(Client *user, userMode *usermode);
void sendUserMode(Client *user,userMode *um);
void applyUserMode(userMode *usermode, Client *user);
void getModeStr(userMode *um, std::string& dst);
void applyUserModes(Client *user);
void applyUserMode(userMode *usermode, Client *user);
bool isGlobalExcempt(Client *user);
void serversetChanModes(Channel *chan, chanClient *client, userMode *usermode,bool set);
void applyChanUserModes(Channel *chan,userMode *usermode);
int getUserChannelModes(Client *user, char *channame);
bool isGlobalBanned(Client *user);
userMode *getUserMode(int usermodeid);
void removeUsermode(userMode *usermode, bool nosql = false);
void removeChanUserModes(Channel *chan,userMode *usermode);
userMode *findRemoveableUsermode(char *name, char *hostmask, int modeflags, bool wildcard = false);
void addChanProp(Client *setter, char *target, char *modestr);
void applyChanProps(chanProps *props, bool kickexisting);
void deleteChanProp(chanProps *chanprop, bool resetchan = true);
void addWhowas(Client *user);
void checkExpiry();
chanProps *findChanProp(char *mask);
chanProps *getClosestChanProp(char *channame, chanProps *skip = NULL);
void sendChanProps(Client *who, chanProps *prop);
bool getKey(std::list<customKey *> userKeys, char *name, char *dst,int len);
bool getKey(std::list<customKey *> *userKeys, char *name, char *dst,int len);
gameInfo *findGame(char *name);
operInfo *getOperInfo(int profileid);
void sendAllChanNames(Client *target);
void sendToAllWithMode(uint32_t mode, char *str,...);
void addChannelInvite(Client *user,Channel *chan);
bool isUserInvited(Client *user, Channel *chan);
void removeChannelInvite(Client *user, Channel *chan);
void resetByProfileID(int profileid, char *reason);
void delOper(int profileid);
void addOper(int profileid, uint32_t rightsmask);
void clearResults(MYSQL *sql);
void deleteChanProp(char *chanmask, bool resetchan = true);
bool isClientAllowed(Client *user, char *channame);
chanGameClient *findGameClient(char *chanmask, int gameid);
bool deleteGameClient(char *chanmask, int gameid);
Client *findClientBySocket(int sd);
int numUsersByIP(uint32_t ip);
#endif
