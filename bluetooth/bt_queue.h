#ifndef BT_QUEUE_H
#define BT_QUEUE_H

/***********************************************************************
 * FILENAME:     BT_Queue.h
 * CURRENT VERSION: 1.0.0 (optional)
 * CREATE DATE:  2005/08/10
 * PURPOSE:      This file contain some queue operator such as removing
 *               a element from the head, insertting a element into the
 *               tail and so on. All macros can be used for all modules.
 *
 * AUTHORS:      jason dong
 *
 * NOTES:        description of constraints when using functions of this file
 *
 **********************************************************************/

/*
 * HISTORY OF CHANGES
 *
 */


 typedef struct _LIST_ENTRY 
{
       struct _LIST_ENTRY *Flink; 
       struct _LIST_ENTRY *Blink; 
} LIST_ENTRY, *PLIST_ENTRY; 



typedef struct _BT_LIST_ENTRY {
	
    LIST_ENTRY  Link;
	
} BT_LIST_ENTRY_T, *PBT_LIST_ENTRY_T;

/*--macros------------------------------------------------------------*/

//-------------------------------------------------------------------------
// QueueInitList -- Macro which will initialize a queue to NULL.
//-------------------------------------------------------------------------
#define QueueInitList(_L) (_L)->Link.Flink = (_L)->Link.Blink = (PLIST_ENTRY)0;


//-------------------------------------------------------------------------
// QueueEmpty -- Macro which checks to see if a queue is empty.
//-------------------------------------------------------------------------
#define QueueEmpty(_L) (QueueGetHead((_L)) == (PBT_LIST_ENTRY_T)0)


//-------------------------------------------------------------------------
// QueueGetHead -- Macro which returns the head of the queue, but does not
// remove the head from the queue.
//-------------------------------------------------------------------------
#define QueueGetHead(_L) ((PBT_LIST_ENTRY_T)((_L)->Link.Flink))

//-------------------------------------------------------------------------
// QueueGetNext -- Macro which returns the next element of the input element
//-------------------------------------------------------------------------
#define QueueGetNext(_E) ((PBT_LIST_ENTRY_T)((_E)->Link.Flink))

//-------------------------------------------------------------------------
// QueueGetPrior -- Macro which returns the prior element of the input element
//-------------------------------------------------------------------------
#define QueueGetPrior(_E) ((PBT_LIST_ENTRY_T)((_E)->Link.Blink))

//-------------------------------------------------------------------------
// QueuePushHead -- Macro which puts an element at the head of the queue.
//-------------------------------------------------------------------------
#define QueuePushHead(_L,_E) \
{                                                     \
PBT_LIST_ENTRY_T ListElem;                        \
ASSERT(_L);											\
ASSERT(_E);											\
ListElem = (PBT_LIST_ENTRY_T)(_L)->Link.Flink;	\
if (!((_E)->Link.Flink = (_L)->Link.Flink)) \
{ \
(_L)->Link.Blink = (PLIST_ENTRY)(_E); \
} \
else \
{ \
ListElem->Link.Blink = (PLIST_ENTRY)(_E); \
} \
(_E)->Link.Blink = (PLIST_ENTRY)0;	\
(_L)->Link.Flink = (PLIST_ENTRY)(_E);	\
}


//-------------------------------------------------------------------------
// QueueRemoveHead -- Macro which removes the head of the head of queue.
//-------------------------------------------------------------------------
#define QueueRemoveHead(_L) \
{                                                     \
PBT_LIST_ENTRY_T ListElem;                        \
PBT_LIST_ENTRY_T ListElem1;                        \
ASSERT((_L));                                     \
if (ListElem = (PBT_LIST_ENTRY_T)(_L)->Link.Flink) /* then fix up our our list to point to next elem */ \
{   \
if(!((_L)->Link.Flink = ListElem->Link.Flink)) /* rechain list pointer to next link */ \
{	/* if the list pointer is null, null out the reverse link */ \
(_L)->Link.Blink = (PLIST_ENTRY) 0; \
}	\
else	\
{	\
ListElem1 = (PBT_LIST_ENTRY_T)ListElem->Link.Flink;		\
ListElem1->Link.Blink = (PLIST_ENTRY) 0; \
}	\
ListElem->Link.Flink = ListElem->Link.Blink = (PLIST_ENTRY) 0; \
}	\
}

//-------------------------------------------------------------------------
// QueueRemoveTail -- Macro which removes the tail of queue.
//-------------------------------------------------------------------------
#define QueueRemoveTail(_L) \
{                                                     \
PBT_LIST_ENTRY_T ListElem;                        \
PBT_LIST_ENTRY_T ListElem1;                        \
ASSERT((_L));                                     \
if (ListElem = (PBT_LIST_ENTRY_T)(_L)->Link.Blink) /* then fix up our our list to point to next elem */ \
{   \
if(!((_L)->Link.Blink = ListElem->Link.Blink)) /* rechain list pointer to next link */ \
{	/* if the list pointer is null, null out the reverse link */ \
(_L)->Link.Flink = (PLIST_ENTRY) 0; \
}	\
else	\
{	\
ListElem1 = (PBT_LIST_ENTRY_T)ListElem->Link.Blink;		\
ListElem1->Link.Flink = (PLIST_ENTRY) 0; \
}	\
ListElem->Link.Flink = ListElem->Link.Blink = (PLIST_ENTRY) 0; \
}	\
}

//-------------------------------------------------------------------------
// QueueRemoveEle -- Macro which removes an element from queue.
//-------------------------------------------------------------------------
#define QueueRemoveEle(_L,_E) \
{                                               \
PBT_LIST_ENTRY_T ListElem;                        \
PBT_LIST_ENTRY_T ListElem1;                        \
ASSERT(_L);                                     \
ASSERT(_E);									\
if ((_L)->Link.Flink == (PLIST_ENTRY)_E)		\
{	\
QueueRemoveHead(_L);	\
} else if ((_L)->Link.Blink == (PLIST_ENTRY)_E)		\
{	\
QueueRemoveTail(_L);		\
}	\
else	\
{	\
ListElem = (PBT_LIST_ENTRY_T)(_E)->Link.Flink;	\
ListElem1 = (PBT_LIST_ENTRY_T)(_E)->Link.Blink;	\
if (ListElem && ListElem1)	\
{	\
ListElem->Link.Blink = (PLIST_ENTRY)ListElem1;		\
ListElem1->Link.Flink = (PLIST_ENTRY)ListElem;		\
}	\
(_E)->Link.Flink = (_E)->Link.Blink = (PLIST_ENTRY) 0;	\
} }

//-------------------------------------------------------------------------
// QueuePutTail -- Macro which puts an element at the tail (end) of the queue.
//-------------------------------------------------------------------------
#define QueuePutTail(_L,_E) \
ASSERT(_L); \
ASSERT(_E); \
if ((_L)->Link.Blink) \
{ \
((PBT_LIST_ENTRY_T)(_L)->Link.Blink)->Link.Flink = (PLIST_ENTRY)(_E); \
(_E)->Link.Blink = (PLIST_ENTRY)(_L)->Link.Blink;		\
(_L)->Link.Blink = (PLIST_ENTRY)(_E); \
} \
else \
{ \
(_L)->Link.Flink = \
(_L)->Link.Blink = (PLIST_ENTRY)(_E); \
(_E)->Link.Blink = (PLIST_ENTRY)0;	\
} \
(_E)->Link.Flink = (PLIST_ENTRY)0;

//-------------------------------------------------------------------------
// QueueGetCount -- Macro which get the count of the queue.
//-------------------------------------------------------------------------
#define QueueGetCount(_L,_C) \
{                                                     \
PBT_LIST_ENTRY_T ListElem;                        \
ASSERT(_L); \
ListElem = (PBT_LIST_ENTRY_T)(_L);		\
_C = 0L;		\
while ((ListElem)->Link.Flink) \
{ \
_C++;		\
ListElem = (PBT_LIST_ENTRY_T)(ListElem)->Link.Flink; \
} \
}




//-------------------------------------------------------------------------
// QueueGetTail -- Macro which returns the tail of the queue, but does not
// remove the tail from the queue.
//-------------------------------------------------------------------------
#define QueueGetTail(_L) ((PBT_LIST_ENTRY_T)((_L)->Link.Blink))

//-------------------------------------------------------------------------
// QueuePopHead -- Macro which  will pop the head off of a queue (list), and
//                 return it (this differs only from queueremovehead only in the 1st line)
//-------------------------------------------------------------------------
#define QueuePopHead(_L) \
(PBT_LIST_ENTRY_T) (_L)->Link.Flink; QueueRemoveHead(_L);

//-------------------------------------------------------------------------
// QueuePrint -- Macro which print all the elements' pointer of queue.
//-------------------------------------------------------------------------
#define QueuePrintAll(_L) \
{                                                     \
PBT_LIST_ENTRY_T ListElem;                        \
ASSERT(_L); \
ListElem = (PBT_LIST_ENTRY_T)(_L);		\
printk(KERN_INFO "All elements' pointer:\n");		\
while ((ListElem)->Link.Flink) \
{ \
printk(KERN_INFO " 0x%x\n",ListElem->Link.Flink);	\
ListElem = (PBT_LIST_ENTRY_T)(ListElem)->Link.Flink; \
} \
}






/*--constants and types------------------------------------------------*/

//-------------------------------------------------------------------------
// BT_LIST_ENTRY_T
//-------------------------------------------------------------------------

/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/


#endif
