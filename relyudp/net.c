/* Cria um socket UDP e envia pacotes */

#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <string.h>     // memset
#include <signal.h>     // signal
#include <sys/socket.h> // socket, connect ....
#include <sys/types.h>  // pre-requisito de socket.h`
#include <netinet/in.h> // sockaddr_in, in_addr
#include <arpa/inet.h>  // inet_addr
#include <unistd.h>     // sleep, close
#include <netdb.h>      // gethostbyname, hostent
#include <fcntl.h>      // fcntl

#define _NET_OWN
#include "net.h"
#undef _NET_OWN

#ifdef _DEBUG
   #define DEBUG_PRINT(m) printf(m"\n")
#else
   #define DEBUG_PRINT(m)
#endif


struct _Net
{
   int sockfd;
   struct sockaddr_in this_sock;
   struct sockaddr_in that_sock;

   int connected;
};

netState * netCreate(char *host, unsigned short port)
{
   netState *newState = (netState *)malloc(sizeof(netState));
   if(! newState) return NULL;

   newState->this_sock.sin_family = AF_INET;
   newState->this_sock.sin_port = htons(port);
   if(!host)
      newState->this_sock.sin_addr.s_addr = INADDR_ANY;
   else
   {
      struct hostent *he = gethostbyname(host);
      memcpy(&newState->this_sock.sin_addr, 
             (struct in_addr *)he->h_addr, 
             he->h_length);
   }
   memset(newState->this_sock.sin_zero, 0, 8);

   newState->sockfd = socket(PF_INET, SOCK_DGRAM, 0);

   newState->connected = 0;
   
   //nonblocking state
   //fcntl(newState->sockfd, F_SETFL, O_NONBLOCK);

   if( 
    ! bind(
            newState->sockfd, 
            (struct sockaddr *)& newState->this_sock, 
            sizeof(struct sockaddr)
          ) // bind end
      ) // if end
      DEBUG_PRINT("BIND OK");
   else
   {
      DEBUG_PRINT("BIND NOT OK");
      close(newState->sockfd);
      free(newState);
      exit(1);
   }

   return newState;
}

int netConnect(netState *pState, char *host, unsigned short port)
{
   struct hostent *he;
   struct sockaddr_in tmp_sock;
   int ret = 0;

   if(! pState) return 0;

   tmp_sock.sin_family = AF_INET;
   tmp_sock.sin_port = htons(port);

   // Resolvendo o nome
   he = gethostbyname(host);
   memcpy(&tmp_sock.sin_addr, (struct in_addr *)he->h_addr, he->h_length);

   if( ! connect(
            pState->sockfd, 
            (struct sockaddr *)& tmp_sock, 
            sizeof(struct sockaddr)
         ) // connect end
      ) // if end
   {
      DEBUG_PRINT("CONNECT OK");
      pState->connected = 1;
      ret = 1;
   }
   else
   {
      DEBUG_PRINT("CONNECT NOT OK");
      ret = 0;
   }

   return ret;
}

int netReceive(netState *pState, void *buffer, int buffer_size)
{
   int len;

   if(! pState) return 0;

   len = recvfrom(pState->sockfd, buffer, buffer_size, 0, 
                                  (struct sockaddr *)&pState->that_sock, &len);
   if( len < 0 )
   {
      //Socket is idle
      return -1;
   }

   return 1;
}

int netSend(netState *pState, void *msg, int len)
{
   if(! pState) return 0;

   if(pState->connected)
   {
      send(pState->sockfd, msg, len, 0);
   }
   else
   {
      sendto(pState->sockfd, msg, len, 0, (struct sockaddr *)&pState->that_sock, sizeof(struct sockaddr));
   }

   return 1;
}

int netSendTo(netState *pState, char *host, unsigned short port, void *msg, int len)
{
   netSetDest(pState, host, port);

   sendto(pState->sockfd, msg, len, 0, (struct sockaddr *)&pState->that_sock, sizeof(struct sockaddr));

   return 1;
}

int netSetDest(netState *pState, char *host, unsigned short port)
{
   struct hostent *he;

   if(! pState) return 0;

   pState->that_sock.sin_family = AF_INET;
   pState->that_sock.sin_port = htons(port);
   pState->that_sock.sin_addr.s_addr = inet_addr(host);

   he = gethostbyname(host);
   memcpy(&pState->that_sock.sin_addr, (struct in_addr *)he->h_addr, 
                                                                  he->h_length);

   memset(pState->that_sock.sin_zero, 0, 8);

   return 1;
}

void netClose(netState *pState)
{
   if(!pState) return;
   
   close(pState->sockfd);
   free(pState);
   pState = NULL;

   DEBUG_PRINT("CLOSE OK");

   return;
}

#ifdef _DEBUG_net_

int main(int argc, char **argv)
{
   netState *pState;
   char buf[256];
   int i;

   if(argc != 3)
   {
      printf("Usage: %s <host> <port>\n", argv[0]);
      exit(0);
   }
   
   signal(SIGINT, recebe_sinal);

   pState = netCreate(argv[1], atoi(argv[2]));

   for EVER
   {
      i = netReceive(pState, buf, 256);
      if(i>=0)
      {
         if(memcmp(buf, "close", 5))
            break;
         else
            printf("%s\n", buf);
      }
   }

   netClose(pState);
   
   return 0;
}

#endif // _DEBUG_net_
