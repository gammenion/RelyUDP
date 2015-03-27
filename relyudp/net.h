
#ifndef _NET_MOD_
#define _NET_MOD_

#undef EXT

#ifdef _NET_OWN
   #define EXT
#else
   #define EXT extern
#endif

typedef struct _Net netState;

EXT netState * netCreate(char *host, unsigned short port);
EXT int netConnect(netState *pState, char *host, unsigned short port);
EXT int netReceive(netState *pState, void *buffer, int buffer_size);
EXT int netSend(netState *pState, void *msg, int len);
EXT int netSendTo(netState *pState, char *host, unsigned short port, void *msg, int len);
EXT int netSetDest(netState *pState, char *host, unsigned short port);
EXT void netClose(netState *pState);

#endif //_NET_MOD_
