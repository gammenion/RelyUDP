/*******************************************************************************
 * Modulo CONN (connection)
 *
 * Cria toda a interface de alto nivel para o uso geral.
 * Faz a manutencao da fila de mensagens e da thread que a manuseia.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#define _CONN_OWN
#include "conn.h"
#undef _CONN_OWN

#include "net.h"
#include "list.h"

//Faz a diferenca entre dois timevals. Retorna em microsecs
#define TIMEDIFF(a, b) ( (b)->tv_usec - (a)->tv_usec ) \
                        + 1000000 * ((b)->tv_sec - (a)->tv_sec)

#define MAX_MSGSIZE  128   // tamanho maximo da mensagem em bytes
#define TIMER_MSEC   1000  // Tempo do timer em milisegundos

#ifdef _DEBUG
   #define DEBUG_PRINT(args...) printf(args)
#else
   #define DEBUG_PRINT(args...)
#endif
 
/***********************************************
 * Estrutura e tipo Gerais para esse modulo
 * 
 * Campos:
 *    pState = Estado do socket
 *    HoldBackList = Lista de recebimento dos pacotes
 *    AckBufferList = Lista de espera de acknoledgement
 *    recv_thread = Thread de recebimento dos pacotes
 *    flags = Flags de configuracao da conexao
 */
struct _stConnState
{
   // Socket
   netState *pnetState;

   // HoldBackList: Guarda as mensagens recebidas ordenadas
   ptpList HoldBackList;

   // AckBufferList: guarda os pacotes enviados que esperam por um ack
   ptpList AckBufferList;

   // AcksToBeSentList: guarda as sequencias de acks a serem enviados
   ptpList AcksToBeSentList;

   // Thread de recebimento de mensagens
   pthread_t recv_thread;
   pthread_t timeout_thread;
   pthread_mutex_t *HoldBackMutex;
   pthread_mutex_t *AckBufferMutex;
   pthread_cond_t *AckCondWait;
   pthread_cond_t *RecvCondWait;

   // Flags de configuracao para essa conexao
   char flags;
   unsigned int last_read_seq;
   unsigned int last_sent_ack;
};

typedef enum msg_type
{
   USER,
   CTRL
} eMsgType;

// Estrutura do pacote para este protocolo
struct packet
{
   unsigned int seq;
   eMsgType msg_type;

   int msg_size;
   char msg[MAX_MSGSIZE];
};

static void * timeout_thread        (void *state);
static void * do_receive            (void *sock);
static int  send_ack                (pconnState pState);
static void receive_user_handle     (pconnState pState, struct packet *pkt);
static void receive_control_handle  (pconnState pState, struct packet *pkt);

/******************************************************************************/
/************************ INICIO DE FUNCOES DE CONEXAO ************************/
/******************************************************************************/

// Inicializa as listas globais, cria o socket e dispara a thread de recebimento
pconnState connInit(char *host, unsigned short port)
{
   //Memoria para o estado
   pconnState pState = (pconnState) malloc(sizeof(connState));

   //Memoria para as listas
   pState->HoldBackList     = (ptpList) malloc(sizeof(tpList));
   pState->AckBufferList    = (ptpList) malloc(sizeof(tpList));
   pState->AcksToBeSentList = (ptpList) malloc(sizeof(tpList));
   
   //Memoria para os mutexes
   pState->HoldBackMutex =(pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
   pState->AckBufferMutex=(pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));

   //Memoria para as condicoes
   pState->AckCondWait  = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
   pState->RecvCondWait = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));

   // Inicializando as listas
   List_init(pState->HoldBackList);
   List_init(pState->AckBufferList);
   List_init(pState->AcksToBeSentList);

   // Inicializando os mutexes
   pthread_mutex_init (pState->HoldBackMutex, NULL);
   pthread_mutex_init (pState->AckBufferMutex, NULL);
   
   // Inicializando a comunicacao
   pState->pnetState = netCreate(host, port);

   // Resetando os contadores de sequencia
   pState->last_read_seq = 0;
   pState->last_sent_ack = 0;
   
   //Cria a thread de recebimento
   pthread_create (&pState->recv_thread, NULL, do_receive, pState);
   //Cria a thread de timeout
   pthread_create (&pState->timeout_thread, NULL, timeout_thread, pState);

   return pState;
}

static int send_ack(pconnState pState)
{
   struct packet ack_pkt;

   ack_pkt.msg_type = CTRL;
   ack_pkt.msg_size = 0;

   //DEBUG_PRINT("LAST_SENT: %d - FIRST_SEQ: %d\n", pState->last_sent_ack + 1, List_checkFirstSeq(pState->AcksToBeSentList));
   while( (pState->last_sent_ack + 1) == List_checkFirstSeq(pState->AcksToBeSentList) )
   {
      pState->last_sent_ack++;
      List_clean(pState->AcksToBeSentList, pState->last_sent_ack, 1);
   }

   ack_pkt.seq = pState->last_sent_ack;
   DEBUG_PRINT("Enviando ACK com seq %d\n", ack_pkt.seq);
   netSend(pState->pnetState, &ack_pkt, sizeof(struct packet));

   return 1;
}

static void receive_user_handle(pconnState pState, struct packet *pkt)
{
   //Pede para acessar a secao critica
   pthread_mutex_lock(pState->HoldBackMutex);

   // Ve se o seq do pacote eh menor do que o ultimo lido pelo usuario
   //DEBUG_PRINT("PKT SEQ: %d - LAST_READ: %d\n",pkt->seq,pState->last_read_seq);
   if(pkt->seq > pState->last_read_seq)
   {
      ptpListElm elm;
      ptpListElm ack_elm;

      elm = List_createElm(pState->HoldBackList, 0);
      elm->seq = pkt->seq;
      elm->msg = calloc(1, pkt->msg_size);
      elm->size = pkt->msg_size;
      memcpy(elm->msg, pkt->msg, pkt->msg_size);

      //Insere essa sequencia na lista de acks a serem enviados
      ack_elm = List_createElm(pState->AcksToBeSentList, 0);
      ack_elm->seq = pkt->seq;
      ack_elm->size = 0;
      List_insert(pState->AcksToBeSentList, ack_elm);

      // Insere a mensagem na lista de recebimento
      List_insert(pState->HoldBackList, elm);

      // Envia os acks 
      send_ack(pState);

      // Avisa que a condicao de recebimento foi satisfeita
      pthread_cond_signal(pState->RecvCondWait);
   }
   else
      send_ack(pState);
      
   pthread_mutex_unlock(pState->HoldBackMutex);

   return;
}

static void receive_control_handle(pconnState pState, struct packet *pkt)
{
   //Pede para acessar a secao critica
   pthread_mutex_lock(pState->AckBufferMutex);

   DEBUG_PRINT("Recebeu um ack com seq: %d. Removendo da lista.\n", pkt->seq);

   List_clean(pState->AckBufferList, pkt->seq, 1);
   
   pthread_mutex_unlock(pState->AckBufferMutex);

   return;
}

// Thread de recebimento
static void * do_receive(void *state)
{
   pconnState pState = (pconnState) state;
   netState *pnetState = pState->pnetState;
   void *buffer = calloc(1, 1024);
   int ret = 0;

   // Nao sei se a concorrencia por essa chamada vai causar problemas futuros
   while((ret = netReceive(pnetState, buffer, 1024)))
   {
      if( ret != -1 )
      {
         struct packet *pkt = (struct packet *) buffer;

         switch(pkt->msg_type)
         {
            case USER:
               receive_user_handle(pState, pkt);
               break;

            case CTRL:
               receive_control_handle(pState, pkt);
               break;

            default:
               DEBUG_PRINT("Esse tipo de msg nao existe! %d\n",pkt->msg_type);
         }

      }
      memset(buffer, 0, 1024);
   }
   return NULL;
}

// Thread de recebimento de acks
static void * timeout_thread(void *state)
{
   pconnState pState = (pconnState) state;
   struct timeval *tmp_tv;
   struct timeval actual_tv;

   while(1)
   {
      usleep(TIMER_MSEC * 100);
      DEBUG_PRINT("Verificando lista de espera por ACKS...\n");

      //Pede para acessar a secao critica
      pthread_mutex_lock(pState->AckBufferMutex);

      tmp_tv = List_checkFirstTime(pState->AckBufferList);

      // Se nao existir mensagens na lista de acks, espera ateh chegar uma
      while( ! tmp_tv )
      {
         DEBUG_PRINT("Nao existem ACKs a serem esperados... Esperando\n");
         pthread_cond_wait(pState->AckCondWait, pState->AckBufferMutex);

         //Quando retomar o poder do mutex, testar de novo a lista, pq ela pode
         //ter sido alterada durante o wait
         tmp_tv = List_checkFirstTime(pState->AckBufferList);
      }

      gettimeofday(&actual_tv, NULL);

      if(TIMEDIFF(tmp_tv, &actual_tv) > (TIMER_MSEC * 1000))
      {
         struct packet pkt;
         ptpListElm elm = List_getFirstElm(pState->AckBufferList);

         memset(pkt.msg, 0, MAX_MSGSIZE);

         //Retransmite a mensagem
         pkt.msg_type = USER;
         pkt.seq = elm->seq;

         pkt.msg_size = elm->size;
         memcpy(pkt.msg, elm->msg, elm->size);

         // Re-envia a mensagem
         DEBUG_PRINT("Retransmitindo a mensagem com seq: %d\n", elm->seq);
         netSend(pState->pnetState, &pkt, sizeof(struct packet));
      }

      pthread_mutex_unlock(pState->AckBufferMutex);
   }

   return NULL;
}

// Envia mensagens
int connSend(pconnState pState, void *msg, int size)
{
   //criando o pacote do tamando da mensagem
   struct packet pkt;
   ptpListElm elm;

   memset(pkt.msg, 0, MAX_MSGSIZE);

   //Pede para acessar a secao critica
   pthread_mutex_lock(pState->AckBufferMutex);

   elm = List_createElm(pState->AckBufferList, 1);

   //Copia a mensagem para dentro da lista
   elm->msg = calloc(1, size);
   elm->size = size;
   memcpy(elm->msg, msg, size);

   List_insert(pState->AckBufferList, elm);

   //Avisa a condicao de espera de ack que existe um elemento a ser monitorado
   pthread_cond_signal(pState->AckCondWait);

   pthread_mutex_unlock(pState->AckBufferMutex);

   pkt.msg_type = USER;
   pkt.seq = elm->seq;

   pkt.msg_size = size;
   memcpy(pkt.msg, msg, size);

   //Envia a mensagem
   netSend(pState->pnetState, &pkt, sizeof(struct packet));

   return 1;
}

int connReceive(pconnState pState, void *buffer, int size, int block)
{
   int retval = 0;

   //Pede para acessar a secao critica
   pthread_mutex_lock(pState->HoldBackMutex);

   //Deve ler o proximo numero de sequencia
   if( block )
   {
      ptpListElm pElm;

      while(pState->last_read_seq + 1 != List_checkFirstSeq(pState->HoldBackList))
      {
         //Se nao tiver mensagem para ser entregue, checa pelo boolean de block
         pthread_cond_wait(pState->RecvCondWait, pState->HoldBackMutex);
      }
      pState->last_read_seq++;
      pElm = List_remove(pState->HoldBackList);
      memcpy(buffer, pElm->msg, size < pElm->size ? size : pElm->size);

      retval = 1;
   }
   else
   {
      if(pState->last_read_seq + 1 == List_checkFirstSeq(pState->HoldBackList))
      {
         ptpListElm pElm;

         pState->last_read_seq++;
         pElm = List_remove(pState->HoldBackList);
         memcpy(buffer, pElm->msg, size < pElm->size ? size : pElm->size);

         retval = 1;
      }
   }

   pthread_mutex_unlock(pState->HoldBackMutex);

   return retval;
}

int connConnect(pconnState pState, char *host, unsigned short port)
{
   return netConnect(pState->pnetState, host, port);
}

// Executa todas as funcoes de fechamento dos elementos do estado e desaloca
// toda a memoria por ele alocada
int connClose( pconnState pState )
{
   // Cancela a thread de recebimento
   pthread_cancel(pState->recv_thread);
   pthread_cancel(pState->timeout_thread);

   // Limpa os mutexes
   pthread_mutex_destroy (pState->HoldBackMutex);
   pthread_mutex_destroy (pState->AckBufferMutex);

   // Limpa as condicoes
   pthread_cond_destroy (pState->AckCondWait);
   pthread_cond_destroy (pState->RecvCondWait);

   // Limpa as listas
   List_clean(pState->HoldBackList, LIST_ALL_MEMBERS, 1);
   List_clean(pState->AckBufferList, LIST_ALL_MEMBERS, 1);
   List_clean(pState->AcksToBeSentList, LIST_ALL_MEMBERS, 1);

   // Limpa a comunicacao
   netClose(pState->pnetState);
   
   // Desaloca memoria das listas
   free(pState->HoldBackList);
   free(pState->AckBufferList);
   free(pState->AcksToBeSentList);

   // Desaloca memoria dos mutexes
   free(pState->HoldBackMutex);
   free(pState->AckBufferMutex);

   //Desaloca memoria das condicoes
   free(pState->AckCondWait);
   free(pState->RecvCondWait);

   // Desaloca memoria do estado
   free(pState);
   pState = NULL;

   return 1;
}

// Para motivos de teste
#ifdef _DEBUG_conn_

#if 1
/*********************************** TESTE 3 **********************************/
#include <unistd.h>
int main()
{
   char buf[4];

   pconnState state = connInit("localhost", 5000);
   netConnect(state->pnetState,"localhost", 5000);

   connSend(state, "isso", 4);
   connSend(state, "manda isso", 10);
   connSend(state, "fechando agora", 14);
   connSend(state, "teste", 5);

   // Espera os pacotes chegarem
   sleep(1);

   while(connReceive(state, buf, 3, 1))
   {
      buf[3] = 0;
      printf("recebi: %s\n",buf);
   }

   /*
   if(connReceive(state, buf, 3))
   {
      buf[3] = 0;
      printf("recebi: %s\n",buf);
   }
   else
   {
      printf("NAO recebi\n");
   }*/

   connClose(state);

   return 1;
}
#endif

#endif // _DEBUG_conn_

