#ifndef DSP_QUEUE_H
#define DSP_QUEUE_H

/***********************************************************************

 *
 * FILENAME:     usbwlan_queue.h      CURRENT VERSION: 1.00.01
 * PURPOSE:      This file contain some queue operator such as removing
 *               a element from the head, insertting a element into the
 *               tail and so on. All macros can be used for all modules.
 *
 *
 * DECLARATION:  This document contains confidential proprietary information that
 *               is solely for authorized personnel. It is not to be disclosed to
 *               any unauthorized person without prior written consent of 3DSP
 *               Corporation.       
 *
 **********************************************************************/
	
/*--macros------------------------------------------------------------*/

   
/*--constants and types------------------------------------------------*/

#include "precomp.h"

 typedef struct _LIST_ENTRY 
{
       struct _LIST_ENTRY *Flink; 
       struct _LIST_ENTRY *Blink; 
} LIST_ENTRY, *PLIST_ENTRY; 


//-------------------------------------------------------------------------
// DSP_LIST_ENTRY_T
//-------------------------------------------------------------------------
typedef struct _DSP_LIST_ENTRY {
	
    LIST_ENTRY  Link;
	
} DSP_LIST_ENTRY_T, *PDSP_LIST_ENTRY_T;

/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/


//-------------------------------------------------------------------------
// QueueInitList -- Macro which will initialize a queue to NULL.
//-------------------------------------------------------------------------
#define QueueInitList(_L) (_L)->Link.Flink = (_L)->Link.Blink = (PLIST_ENTRY)0;


//-------------------------------------------------------------------------
// QueueEmpty -- Macro which checks to see if a queue is empty.
//-------------------------------------------------------------------------
#define QueueEmpty(_L) (QueueGetHead((_L)) == (PDSP_LIST_ENTRY_T)0)


//-------------------------------------------------------------------------
// QueueGetHead -- Macro which returns the head of the queue, but does not
// remove the head from the queue.
//-------------------------------------------------------------------------
#define QueueGetHead(_L) ((PDSP_LIST_ENTRY_T)((_L)->Link.Flink))

//-------------------------------------------------------------------------
// QueueGetNext -- Macro which returns the next element of the input element
//-------------------------------------------------------------------------
#define QueueGetNext(_E) ((PDSP_LIST_ENTRY_T)((_E)->Link.Flink))

//-------------------------------------------------------------------------
// QueueGetPrior -- Macro which returns the prior element of the input element
//-------------------------------------------------------------------------
#define QueueGetPrior(_E) ((PDSP_LIST_ENTRY_T)((_E)->Link.Blink))

//-------------------------------------------------------------------------
// QueuePushHead -- Macro which puts an element at the head of the queue.
//-------------------------------------------------------------------------
#define QueuePushHead(_L,_E) \
{                                                     \
	PDSP_LIST_ENTRY_T ListElem;                        \
	ASSERT(_L);											\
	ASSERT(_E);											\
	ListElem = (PDSP_LIST_ENTRY_T)(_L)->Link.Flink;	\
	(_E)->Link.Flink = (_L)->Link.Flink;			\
	if (!((_E)->Link.Flink)) \
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
	PDSP_LIST_ENTRY_T ListElem;                        \
	PDSP_LIST_ENTRY_T ListElem1;                        \
	ASSERT((_L));                                     		\
	ListElem = (PDSP_LIST_ENTRY_T)(_L)->Link.Flink;	\
	if (ListElem) /* then fix up our our list to point to next elem */ \
	{   \
		(_L)->Link.Flink = ListElem->Link.Flink;	\
		if(!((_L)->Link.Flink)) /* rechain list pointer to next link */ \
		{	/* if the list pointer is null, null out the reverse link */ \
			(_L)->Link.Blink = (PLIST_ENTRY) 0; \
		}	\
		else	\
		{	\
			ListElem1 = (PDSP_LIST_ENTRY_T)ListElem->Link.Flink;		\
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
	PDSP_LIST_ENTRY_T ListElem;                        \
	PDSP_LIST_ENTRY_T ListElem1;                        \
	ASSERT((_L));                                     \
	ListElem = (PDSP_LIST_ENTRY_T)(_L)->Link.Blink;	\
	if (ListElem) /* then fix up our our list to point to next elem */ \
	{   \
		(_L)->Link.Blink = ListElem->Link.Blink;		\
		if(!((_L)->Link.Blink)) /* rechain list pointer to next link */ \
		{	/* if the list pointer is null, null out the reverse link */ \
			(_L)->Link.Flink = (PLIST_ENTRY) 0; \
		}	\
		else	\
		{	\
			ListElem1 = (PDSP_LIST_ENTRY_T)ListElem->Link.Blink;		\
			ListElem1->Link.Flink = (PLIST_ENTRY) 0; \
		}	\
		ListElem->Link.Flink = ListElem->Link.Blink = (PLIST_ENTRY) 0; \
	}	\
}

//-------------------------------------------------------------------------
// QueueRemoveEle -- Macro which removes an element from queue.
//-------------------------------------------------------------------------
#define QueueRemoveEle(_L, _E) \
{                                               \
	PDSP_LIST_ENTRY_T ListElem;                        \
	PDSP_LIST_ENTRY_T ListElem1;                        \
	ASSERT(_L);                                     \
	ASSERT(_E);									\
	if ((_L)->Link.Flink == (PLIST_ENTRY)_E)		\
	{	\
		QueueRemoveHead(_L);	\
	}	\
	else if ((_L)->Link.Blink == (PLIST_ENTRY)_E)		\
	{	\
		QueueRemoveTail(_L);		\
	}	\
	else	\
	{	\
		ListElem = (PDSP_LIST_ENTRY_T)(_E)->Link.Flink;	\
		ListElem1 = (PDSP_LIST_ENTRY_T)(_E)->Link.Blink;	\
		if (ListElem && ListElem1)	\
		{	\
			ListElem->Link.Blink = (PLIST_ENTRY)ListElem1;		\
			ListElem1->Link.Flink = (PLIST_ENTRY)ListElem;		\
		}	\
		(_E)->Link.Flink = (_E)->Link.Blink = (PLIST_ENTRY) 0;	\
	}\
}
	
//-------------------------------------------------------------------------
// QueuePutTail -- Macro which puts an element at the tail (end) of the queue.
//-------------------------------------------------------------------------
#define QueuePutTail(_L,_E) \
{\
	ASSERT(_L); \
	ASSERT(_E); \
	if ((_L)->Link.Blink) \
	{ \
		((PDSP_LIST_ENTRY_T)(_L)->Link.Blink)->Link.Flink = (PLIST_ENTRY)(_E); \
		(_E)->Link.Blink = (PLIST_ENTRY)(_L)->Link.Blink;		\
		(_L)->Link.Blink = (PLIST_ENTRY)(_E); \
	} \
	else \
	{ \
		(_L)->Link.Flink = \
		(_L)->Link.Blink = (PLIST_ENTRY)(_E); \
		(_E)->Link.Blink = (PLIST_ENTRY)0;	\
	} \
	(_E)->Link.Flink = (PLIST_ENTRY)0;\
}

//-------------------------------------------------------------------------
// QueueGetCount -- Macro which get the count of the queue.
//-------------------------------------------------------------------------
#define QueueGetCount(_L, _C) \
{                                                     \
	PDSP_LIST_ENTRY_T ListElem;                        \
	ASSERT(_L); \
	ASSERT(_C); \
	ListElem = (PDSP_LIST_ENTRY_T)(_L);		\
	*(_C) = 0;		\
	while ((ListElem)->Link.Flink) \
	{ \
		(*(_C))++;		\
		ListElem = (PDSP_LIST_ENTRY_T)(ListElem)->Link.Flink; \
	} \
}


//-------------------------------------------------------------------------
// QueueGetTail -- Macro which returns the tail of the queue, but does not
// remove the tail from the queue.
//-------------------------------------------------------------------------
#define QueueGetTail(_L) ((PDSP_LIST_ENTRY_T)((_L)->Link.Blink))

//-------------------------------------------------------------------------
// QueuePopHead -- Macro which  will pop the head off of a queue (list), and
//                 return it (this differs only from queueremovehead only in the 1st line)
//-------------------------------------------------------------------------
#define QueuePopHead(_L) \
(PDSP_LIST_ENTRY_T) (_L)->Link.Flink; QueueRemoveHead(_L);

//-------------------------------------------------------------------------
// QueuePrint -- Macro which print all the elements' pointer of queue.
//-------------------------------------------------------------------------
#define QueuePrintAll(_L) \
{                                                     \
	PDSP_LIST_ENTRY_T ListElem;                        \
	ASSERT(_L); \
	ListElem = (PDSP_LIST_ENTRY_T)(_L);		\
	DBG_WLAN__UTIL(LEVEL_INFO,"All elements' pointer:\n");		\
	while ((ListElem)->Link.Flink) \
	{ \
		DBG_WLAN__UTIL(LEVEL_INFO," 0x%x\n",ListElem->Link.Flink);	\
		ListElem = (PDSP_LIST_ENTRY_T)(ListElem)->Link.Flink; \
	} \
}

#endif

