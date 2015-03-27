#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "conn.h"

int main(int argc, char **argv)
{
   char buf[32];
   pconnState state;

   if(argc!=3)
   {
      printf("Usage: %s <this host> <that host>\n",argv[0]);
      exit(1);
   }

   state = connInit(argv[1], 6000);
   memset(buf, 0, 32);

   connConnect(state, argv[2], 6001);

   while( connReceive(state, buf, 32, 1) )
   {
      printf("Recebeu: %s\n", buf);
      if(!strncmp(buf,"close",5))
         break;
      memset(buf, 0, 32);
   }
   
   connClose(state);

   return 1;
}
