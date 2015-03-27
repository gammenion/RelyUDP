#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "conn.h"

int main(int argc, char **argv)
{
   char buffer[32];
   int i = 1;
   pconnState state;

   if(argc!=3)
   {
      printf("Usage: %s <this host> <that host>\n",argv[0]);
      exit(1);
   }

   //state = connInit(ANY_HOST, ANY_PORT);
   state = connInit(argv[1], 6001);

   if(connConnect(state, argv[2], 6000))
   {
      printf("conectou beleza\n");
   }

   for(i=1; i<=100; i++)
   {
      memset(buffer,0,32);
      sprintf(buffer,"msg %d", i);
      connSend(state, buffer, 32);
   }
   sleep(15);

   printf("Saindo fora.\n");
   
   connSend(state, "close", 5);
   connClose(state);

   return 1;
}
