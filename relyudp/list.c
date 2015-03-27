#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define _LIST_OWN
#include "list.h"
#undef _LIST_OWN

/******************************************************************************/
/************************** INICIO DE FUNCOES DE LISTAS ***********************/
/******************************************************************************/

// Inicializa os ponteiros e sequenciadores da lista
int List_init(ptpList pList)
{
   pList->pHead = pList->pTail = NULL;
   pList->seq = 1;

   return 1;
}

// Insere elementos na lista ordenadamente com base no seu numero de sequencia
// OBS: Essa funcao nao eh responsavel pela memoria, apenas com o encadeamento
// da lista
int List_insert(ptpList pList, ptpListElm pElm)
{
   //Variaveis de auxilio (clareza de codigo)
   ptpListElm *pHead, *pTail;

   pHead = & pList->pHead;
   pTail = & pList->pTail;

   // Se ele eh o unico elemento da lista
   if( ! *pTail )
   {
      *pHead = *pTail = pElm;
      pElm->next = NULL;
      pElm->prev = NULL;
   }
   else
   {
      if( (*pTail)->seq <= pElm->seq )
      {
         //Se o seq do ultimo for igual ao do elemento, nao o inclui.
         if((*pTail)->seq == pElm->seq)
            return 1;

         pElm->prev = *pTail;
         (*pTail)->next = pElm;
         *pTail = pElm;
         (*pTail)->next = NULL;
      }
      else  //Varre a lista do fim pro inicio procurando sua posicao correta
      {
         ptpListElm pTmp;

         //Comeca a partir do anterior ao tail. Sabemos que o tail eh menor
         //No fim, pTmp vai conter a posicao de uma sequencia anterior ao elm
         for(pTmp = (*pTail)->prev; pTmp != NULL; pTmp = pTmp->prev)
         {
            //printf("[(%d)%d - %d] ",List_checkFirstSeq(pList),pTmp->seq,pElm->seq);
            if(pTmp->seq <= pElm->seq )
            {
               if(pTmp->seq == pElm->seq )
               {
                  //printf("LIST: SEQ do tmp eh igual ao ELM (%d - %d)\n",pTmp->seq,pElm->seq);
                  //dump da lista
                  //for(pTmp = *pHead; pTmp != NULL; pTmp = pTmp->next)
                     //printf("[%d] ",pTmp->seq);
                  //printf("\n");

                  return 1;
               }
                  //printf("\n");

               break;
            }
         }
         //Se nao encontrou nada, pTmp eh NULL, chegou no inicio da lista
         if(! pTmp )
         {
            //printf("LIST INICIO: %d - INSERTED %d\n",(*pHead)->seq,pElm->seq);
            pElm->next = *pHead;
            (*pHead)->prev = pElm;
            pElm->prev = NULL;
            *pHead = pElm;
         }
         else
         {
            pElm->next = pTmp->next;
            pTmp->next->prev = pElm;
            pElm->prev = pTmp;
            pTmp->next = pElm;
            //printf("INSERIU NO MEIO DA LISTA (%d)\n",pTmp->seq);
         }
      }
      //dump da lista
      //for(pTmp = *pHead; pTmp != NULL; pTmp = pTmp->next)
         //printf("[%d] ",pTmp->seq);
      //printf("\n");
   }

   return 1;
}

// Remove o primeiro elemento da lista
// OBS: Essa funcao nao eh responsavel pela memoria, apenas com o encadeamento
// da lista
ptpListElm List_remove(ptpList pList)
{
   ptpListElm pRet;

   //Variaveis de auxilio (clareza de codigo)
   ptpListElm *pHead, *pTail;
   
   pHead = & pList->pHead;
   pTail = & pList->pTail;

   if( ! *pHead )
      return NULL;
   else
   {
      pRet = *pHead;
      *pHead = (*pHead)->next;
      //Se o head chegou ao fim, atualizar o tail
      if( ! *pHead )
         *pTail = NULL;
      else
         (*pHead)->prev = NULL;

      return pRet;
   }
   
   //nunca chega aqui
   return NULL;
}

// retorna o primeiro item da lista
ptpListElm List_getFirstElm(ptpList pList)
{
   if(! pList->pHead )
      return NULL;

   return pList->pHead;
}

// Retorna o numero de sequencia do primeiro elemento da lista
unsigned int List_checkFirstSeq(ptpList pList)
{
   if(! pList->pHead )
      return -1;

   return pList->pHead->seq;
}

// Retorna o momento em que o primeiro item da lista foi adicionado
struct timeval * List_checkFirstTime(ptpList pList)
{
   if(! pList->pHead )
      return NULL;

   return & pList->pHead->time;
}

//Aloca nova memoria para um elemento. Inicializa os ponteiros corretamente.
//O parametro update_seq eh um boolen que diz se vai ou nao incrementar o 
//contador global
ptpListElm List_createElm(ptpList pList, int update_seq)
{
   ptpListElm pnewElm = (ptpListElm) malloc (sizeof(tpListElm));

   pnewElm->next = pnewElm->prev = NULL;

   pnewElm->seq = update_seq ? pList->seq++ : 0;

   gettimeofday(& pnewElm->time, NULL);

   return pnewElm;
}

//Remove elementos da lista quem tenham sequencia menor ou igual a seq.
//free eh um boolean. Se estiver ligado, desaloca a memoria dos elementos.
//Isso porque a funcao remove nao mexe com memoria.
//Lembrando: Essas funcoes nao mexem no ponteiro da lista em si
int List_clean(ptpList pList, unsigned int seq, int freemem)
{
   ptpListElm ptr;
   int first_seq = 0;

   if(! pList->pHead )
      return 1;
   
   while(1)
   {
      first_seq = List_checkFirstSeq(pList);

      if(first_seq == -1 || first_seq > seq)
         break;

      ptr = List_remove(pList);

      if(freemem)
      {
         free(ptr);
      }
   }

   return 1;
}

/******************************************************************************/
/**************************** FIM DE FUNCOES DE LISTAS ************************/
/******************************************************************************/

// Para motivos de teste
#ifdef _DEBUG_list_

#if 1
/*********************************** TESTE 3 **********************************/
#include <stdio.h>
int main()
{
   struct timeval a;
   tpListElm data1 = {1, "SEQ 1", 5,a, NULL, NULL};
   tpListElm data2 = {2, "SEQ 2", 5,a, NULL, NULL};
   tpListElm data3 = {3, "SEQ 3", 5,a, NULL, NULL};
   tpListElm data4 = {4, "SEQ 4", 5,a, NULL, NULL};
   ptpListElm ptr;
   tpList HoldBackList;

   List_init(&HoldBackList);

   //List_insert(&HoldBackList, &data2);
   List_insert(&HoldBackList, &data4);
   List_insert(&HoldBackList, &data1);
   //List_insert(&HoldBackList, &data3);

   while((ptr = List_remove(&HoldBackList)))
   {
      printf("%d) %s\n", ptr->seq, (char *)ptr->msg);
   }
   
   return 1;
}
#endif

#endif // _DEBUG_conn_
