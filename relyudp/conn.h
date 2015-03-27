/*******************************************************************************
 * Modulo CONN (connection)
 *
 * Cria toda a interface de alto nivel para o uso geral.
 * Faz a manutencao da fila de mensagens e da thread que a manuzeia.
 *
 ******************************************************************************/

#ifndef _CONN_MOD_
#define _CONN_MOD_

#undef EXT

#ifdef _CONN_OWN
   #define EXT
#else
   #define EXT extern
#endif // _CONN_OWN

#define ANY_HOST NULL
#define ANY_PORT 0

typedef struct _stConnState connState, *pconnState;

EXT pconnState connInit(char *host, unsigned short port);
EXT int connSend(pconnState pState, void *msg, int size);
EXT int connReceive(pconnState pState, void *buffer, int size, int block);
EXT int connConnect(pconnState pState, char *host, unsigned short port);
EXT int connClose( pconnState pState );

#endif // _CONN_MOD_
