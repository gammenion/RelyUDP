
#ifndef _LIST_MOD_
#define _LIST_MOD_

#undef EXT

#ifdef _LIST_OWN
   #define EXT
#else
   #define EXT extern
#endif


/*********************************************
 * Estrutura e Tipo Lista Duplamente Encadeada
 *
 * Campos:
 *    seq = numero de sequencia dos pacotes
 *    msg = mensagem generica
 *    next = encadeamento da lista
 */
typedef struct _stListElm 
{
   unsigned int seq;        // Numero de sequencia dos pacotes

   void *msg;               // Mensagem generica
   int size;                // Tamanho da Mensagem generica

   struct timeval time;     // Momento em que foi adicionado

   struct _stListElm *next; // Encadeamento da lista
   struct _stListElm *prev; // Encadeamento da lista

} tpListElm, *ptpListElm;

/*******************************************
 * Estrutura de manutencao de listas
 *
 * Campos:
 *    pHead = cabeca da lista
 *    pTail = fim da lista
 *    seq = indicador de sequencia para essa lista
 */
typedef struct _stList
{
   ptpListElm pHead;    // Cabeca da lista

   ptpListElm pTail;    // Fim da lista

   unsigned int seq;    // Sequenciador da lista

} tpList, *ptpList; 

#define LIST_ALL_MEMBERS -1

// Definindo as funcoes estaticas para esse modulo.
EXT int          List_init(ptpList pList);
EXT int          List_insert(ptpList, ptpListElm);
EXT ptpListElm   List_remove(ptpList);
EXT ptpListElm   List_getFirstElm(ptpList);
EXT unsigned int List_checkFirstSeq(ptpList pList);
EXT struct timeval * List_checkFirstTime(ptpList pList);
EXT ptpListElm   List_createElm(ptpList pList, int update_seq);
EXT int          List_clean(ptpList pList, unsigned int seq, int freemem);

#endif // _LIST_MOD_
