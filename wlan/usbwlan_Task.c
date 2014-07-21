/****************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  *
  * FILENAME:     DSP_Task.c     CURRENT VERSION: 1.00.01
  * PURPOSE:      We create a passive kernel thread to control the usb
  *               control pipe . We package all the thread process into 
  *               this file. And in the routine of this kernel thread, we
  *               process our task.
  *
  *
  * DECLARATION:  This document contains confidential proprietary information that
  *               is solely for authorized personnel. It is not to be disclosed to
  *               any unauthorized person without prior written consent of 3DSP
  *               Corporation.       
  *
  ****************************************************************/
//#include <linux/workqueue.h>
#include "precomp.h"

static char* TDSP_FILE_INDICATOR="WTASK";

//#include "usbwlan_Task.h"
/*
#include <ndis.h>
#include "usbwlan_Sw.h"
#include "usbwlan_mng.h"
#include "usbwlan_Oid.h"
#include "usbwlan_Pr.h"
#include "usbwlan_dbg.h"
*/
	
/*--file local constants and types-------------------------------------*/
	 
/*--file local macros--------------------------------------------------*/
	 
/*--file local variables-----------------------------------------------*/
	 
/*--file local function prototypes-------------------------------------
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)		    
VOID Task_ThreadRoutine(struct work_struct *param_data);
#else
VOID Task_ThreadRoutine(void *param_data);
#endif*/
void Task_ThreadRoutine(ULONG param_data);

/**************************************************************************
 *   Task_Init
 *
 *   Descriptions:
 *      Initialize task module, including alloc memory for the task module and 
 *      link task block into the task pool and so on.
 *   Arguments:
 *      MiniportAdapterContext: IN, pointer to the adapter object data area.
 *   Return Value:
 *      STATUS_SUCCESS if successfully 
 *      TDSP_STATUS_xxx if unsucessfully
 *************************************************************************/
TDSP_STATUS Task_Init(PDSP_ADAPTER_T pAdap)
{
	PDSP_TASK_T pTask;
	PTASK_DATA_BLOCK_T pTaskBlock;
	UINT32 i;
	UINT32 listcount;
	PUINT8	pbuffer;

	DBG_WLAN__TASK(LEVEL_TRACE, "Task_Init pAdap = %p\n",pAdap);

	/* Alloc memory for task module */
	pTask = (PDSP_TASK_T)sc_memory_alloc(sizeof(DSP_TASK_T));
	if (NULL == pTask)
	{
		DBG_WLAN__TASK(LEVEL_ERR, "[%s] TASK MODULE Allocate Memory failed\n",__FUNCTION__);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
    
	DBG_WLAN__TASK(LEVEL_TRACE, "Task_Init: pTask memory alloc success!\n");
    
	/* Save task module pointer into Adapter context */
	pAdap->ppassive_task = (PVOID)pTask;

	/* Zero out the task module space */
	sc_memory_set(pTask, 0, sizeof(DSP_TASK_T));

	/* Init list */
	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->task_list_high);
	
	/* Alloc spin lock,which used to protect task link operator */
	sc_spin_lock_init(&pTask->lock);
		
	/* Alloc memory for task block */
	pTaskBlock = (PTASK_DATA_BLOCK_T)sc_memory_alloc(MAXTASKN * sizeof(TASK_DATA_BLOCK_T));
	if (NULL == pTaskBlock)
	{
		DBG_WLAN__TASK(LEVEL_ERR,  "[%s] TASK BLOCK Allocate Memory failed\n", __FUNCTION__);
		pAdap->ppassive_task = NULL;
		sc_memory_free(pTask);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	sc_memory_set((PVOID)pTaskBlock, 0,  MAXTASKN * sizeof(TASK_DATA_BLOCK_T));

	DBG_WLAN__TASK(LEVEL_TRACE, "Task_Init: pTaskBlock memory alloc success!\n");

	/*Save task block pointer into task module */
	pTask->ptask_block = pTaskBlock;
    
	/* Insert all task blocks into the task free pool. */

	pbuffer = sc_memory_alloc(MAXTASKDATAN * MAXTASKN);
	if (pbuffer == NULL)
	{
		DBG_WLAN__TASK(LEVEL_ERR,  "[%s] TASK BLOCK_DATA Allocate Memory failed\n", __FUNCTION__);
		sc_memory_free(pTaskBlock);
		sc_memory_free(pTask);
		pAdap->ppassive_task = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	pTask->ptask_block_buffer = pbuffer;
	
	for(i = 0; i < MAXTASKN; i++)
	{	
		pTaskBlock[i].data = pbuffer;
		pbuffer += MAXTASKDATAN;
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock[i].Link);
		pTaskBlock[i].in_list_flag = 1; // This block is in free list. So set this value as 1.
		pTaskBlock[i].ID = i;
	}

	/* Clear exit flag */
	pTask->exit_thread_flag = 0;

//	tdsp_event_init(&pTask->event);
//	pTask->task_thread = kthread_create(&Task_ThreadRoutine, (PVOID)&pAdap->ppassive_task, "Task_ThreadRoutine");
//	wake_up_process(pTask->task_thread);
	sc_worklet_init(&pTask->worklet, Task_ThreadRoutine, (ULONG)pAdap);

	/*Test if it is the correct counts in the list  */
	QueueGetCount(&pTask->task_free_pool, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE, "task free pool list count = %d\n", listcount);
	QueueGetCount(&pTask->task_list, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE, "task list count = %d\n", listcount);
	QueueGetCount(&pTask->task_list_high, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE, "task list high count = %d\n", listcount);

	
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Task_Release
 *
 *   Descriptions:
 *      Release task module, including free memory for the task module and
 *      for the task block.
 *   Arguments:
 *      MiniportAdapterContext: IN, pointer to the adapter object data area.
 *   Return Value: 
 *      STATUS_SUCCESS if successfully 
 *      TDSP_STATUS_xxx if unsucessfully
 *************************************************************************/
TDSP_STATUS Task_Release(PDSP_ADAPTER_T pAdap)
{
	PDSP_TASK_T pTask = (PDSP_TASK_T)pAdap->ppassive_task;
	UINT32 i;
	UINT32 loop = 1000;

	if (NULL == pTask)
		return (STATUS_SUCCESS);

	DBG_WLAN__TASK(LEVEL_TRACE,"Task_Release pAdap = %p, pTask = %p, pTaskBlock = %p\n",
                    pAdap,
                    pTask,
                    pTask->ptask_block);

	Task_RemoveAllExistTask(pTask);

	/*Notify self-created thread exit */
	pTask->exit_thread_flag = 1;

	while ((1 == sc_worklet_running(&pTask->worklet)) && (--loop))
	{
		DBG_WLAN__TASK(LEVEL_TRACE,"task worklet is running %d\n", loop);
		sc_sleep(10);
	}
	
	sc_worklet_kill(&pTask->worklet);

	/*Empty two lists */
	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->task_list_high);
	
	for(i = 0; i < MAXTASKN; i++)
	{	
		if (pTask->ptask_block[i].data_len > MAXTASKDATAN)
			sc_memory_free(pTask->ptask_block[i].data);
	}
	
	/*Free the task block memory */
	if (pTask->ptask_block_buffer)
		sc_memory_free(pTask->ptask_block_buffer);
	
	if (pTask->ptask_block)
		sc_memory_free(pTask->ptask_block);
	pAdap->ppassive_task = NULL;


	/* Free spin lock */
	sc_spin_lock_kill(&pTask->lock);
    
	/*Free the task module memory */
	if (pTask)
		sc_memory_free(pTask);

	DBG_WLAN__TASK(LEVEL_TRACE,"Task_Release exit! \n");
	return (STATUS_SUCCESS);
}

TDSP_STATUS Task_PrintAllTask(PDSP_TASK_T pTask)
{

	UINT32	listcount = 0;

	PTASK_DATA_BLOCK_T ptmpdatablock;

	if (pTask == NULL)
		return (STATUS_FAILURE);
#if 0	
	sc_spin_lock(&pTask->lock);
	
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);
	while (ptmpdatablock)
	{
		DBG_WLAN__TASK(LEVEL_TRACE,"Task[%02u] event = %2u, len = %u\n",ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);;
	}

	
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list_high);
	while (ptmpdatablock)
	{
		DBG_WLAN__TASK(LEVEL_TRACE,"TaskHigh[%02u] event = %2u, len = %u\n",ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);;
	}	

	sc_spin_unlock(&pTask->lock);
#endif	

	sc_spin_lock(&pTask->lock);
	/*Test if it is the correct counts in the list  */
	QueueGetCount(&pTask->task_free_pool, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE,"%s:task free pool list count = %d\n", __FUNCTION__, listcount);
	QueueGetCount(&pTask->task_list, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE, "%s:task list count = %d\n", __FUNCTION__, listcount);
	QueueGetCount(&pTask->task_list_high, &listcount);
	DBG_WLAN__TASK(LEVEL_TRACE, "%s:task list high count = %d\n", __FUNCTION__, listcount);
	sc_spin_unlock(&pTask->lock);
	
	return (STATUS_SUCCESS);	
}


/**************************************************************************
 *   Task_Reset
 *
 *   Descriptions:
 *      Reset task module in the middle of running.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *   Return Value: 
 *      NONE
 *************************************************************************/
VOID Task_Reset(PDSP_TASK_T pTask)
{
//	UINT32 i;
	PTASK_DATA_BLOCK_T pTaskBlock;

#if 0
//Justin:	0523.	there is problem in this functon, so do nothing, fix it latter.
	DBGSTR(("**before Task_Reset\n"));		
	Task_PrintAllTask(pTask);

	return;
#endif

	pTaskBlock = pTask->ptask_block;

//	DBG_WLAN__TASK(LEVEL_TRACE,"**before Task_Reset\n");		
//	Task_PrintAllTask(pTask);

	/* Lock */
	sc_spin_lock_bh(&pTask->lock);
#if 0
	/* Init list */
	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->task_list_high);
	

	/*Link all task block again */
	for(i = 0; i < MAXTASKN; i++)
	{
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock[i].Link);
		pTaskBlock[i].in_list_flag = 1; // This block is in free list. So set this value as 1.
	}
#else
	pTaskBlock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list_high);
	while(pTaskBlock)
	{
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock->Link);
		pTaskBlock->in_list_flag = 1; // This block is in free list. So set this value as 1.
		
		pTaskBlock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list_high);
	}

	pTaskBlock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
	while(pTaskBlock)
	{
		pTaskBlock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock->Link);
	
		pTaskBlock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
	}

#endif

	/* Unlock */
	sc_spin_unlock_bh(&pTask->lock);

//	DBG_WLAN__TASK(LEVEL_TRACE,"**after Task_Reset\n");		
//	Task_PrintAllTask(pTask);

}

/**************************************************************************
 *   Task_CreateTask
 *
 *   Descriptions:
 *      Create a self-defined task.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the event value of task.
 *      pri: IN, the pri value of task.
 *      para: IN, parameter of task.
 *      len : IN, the parameter length  of task.
 *   Return Value: 
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_FAILURE if this task is not created
 *************************************************************************/
TDSP_STATUS Task_CreateTask(PDSP_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	UINT32				force_flag;
    ULONG               flags;
	if (pTask == NULL)
		return (STATUS_FAILURE);

	force_flag = (pri >> 8) & 0x1;
	pri = pri & 0xff;


//	if(event!=1)
//	{
//		DBG_WLAN__TASK(LEVEL_TRACE, "Add Task: Event=%u, pri=%u, paralen=%u, callby %s(%u).\n",
//			event,pri,len,func,line);
//	}	
	/*Get a free task data block from the task free pool */
	sc_spin_lock_irqsave(&pTask->lock,flags);
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_free_pool);
	sc_spin_unlock_irqrestore(&pTask->lock,flags);

#if 0	
	while (ptmpdatablock == NULL)
	{
		UINT32				cnt = 0;
		cnt++;
		DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: ** Task create fail, event = 0x%x, pri = %d, len = %d **\n", 
						__FUNCTION__, event, pri, len);	

		sc_sleep(10);

		if ((pri == DSP_TASK_PRI_HIGH) || (force_flag))
		{
			sc_spin_lock(&pTask->lock);
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
			sc_spin_unlock(&pTask->lock);
		}
		else
		{
			sc_spin_lock(&pTask->lock);
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_free_pool);
			sc_spin_unlock(&pTask->lock);
		}
		
		if (cnt > 100)
			break;
	}
#endif	
	
	if (ptmpdatablock == NULL)
	{
		if ((pri == DSP_TASK_PRI_HIGH) || (force_flag))
		{
			sc_spin_lock_irqsave(&pTask->lock,flags);
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
			sc_spin_unlock_irqrestore(&pTask->lock,flags);
		}
	}
	
	if (ptmpdatablock == NULL)
	{
		if (DSP_TASK_EVENT_CALL_MANAGEMENT != event)
		{
			DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: ** Task create failed, event = 0x%x, pri = %d, len = %d **\n", 
					__FUNCTION__, event, pri, len);	
			
			Task_PrintAllTask(pTask);
		}
		return (STATUS_FAILURE);
	}


	/*Fill some paras */
	ptmpdatablock->task_event = event;
	ptmpdatablock->task_pri = pri;

	if (ptmpdatablock->data_len > MAXTASKDATAN)
	{
		sc_memory_free(ptmpdatablock->data);
		ptmpdatablock->data = &pTask->ptask_block_buffer[ptmpdatablock->ID * MAXTASKDATAN];			
	}
	
	if (len <= MAXTASKDATAN)
	{
		ptmpdatablock->data_len = len;
		if (para)
			sc_memory_copy(ptmpdatablock->data, para, ptmpdatablock->data_len);
	}
	else
	{
		PVOID pbuffer;

		ptmpdatablock->data_len = len;
		pbuffer = sc_memory_alloc(len);
		if (pbuffer == NULL)
		{
			QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
			DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: ** Task create failed, too big. event = 0x%x, pri = %d, len = %d **\n", 
							__FUNCTION__, event, pri, len);
			return (STATUS_FAILURE);
		}
		if (para)
			sc_memory_copy(pbuffer, para, len);
		ptmpdatablock->data = pbuffer;
		DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: ** Task create too big. event = 0x%x, pri = %d, len = %d **\n", 
						__FUNCTION__, event, pri, len);		
	}

	ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.

	/* Insert the task data block into the task list */
	sc_spin_lock_irqsave(&pTask->lock,flags);
	if (pri != DSP_TASK_PRI_HIGH)
	{
		QueuePutTail(&pTask->task_list,&ptmpdatablock->Link);
	}
	else
	{
		if (force_flag)
		{
			QueuePushHead(&pTask->task_list,&ptmpdatablock->Link);
		}
		else
		{
			QueuePutTail(&pTask->task_list_high,&ptmpdatablock->Link);
		}
	}
	sc_spin_unlock_irqrestore(&pTask->lock,flags);

//	tdsp_event_set(&pTask->event);
	sc_worklet_schedule(&pTask->worklet);
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *  Function Name:	Task_CheckExistTask
 *
 *   Descriptions:
 *      Check if the task queue exists the same task whose event is event.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the task event
 *   Return Value: 
 *      1: exist
 *      0: doesn't exist
 *************************************************************************/
UINT32 Task_CheckExistTask(PDSP_TASK_T pTask, UINT32 event)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	UINT32		i = 0;
	ULONG flags;	
	/* Lock */
	sc_spin_lock_irqsave(&pTask->lock,flags);

	while (i < 2)
	{
		if (i == 0)
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list_high);	// check high level list
		else if ( i == 1)
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);		//check normal level list
		else
			break;
		
		while (ptmpdatablock)
		{
			if (ptmpdatablock->task_event == event)
			{
				/* Unlock */
				sc_spin_unlock_irqrestore(&pTask->lock,flags);
				return (1);
			}
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		}
		i++;
	}
	
	/* Unlock */
	sc_spin_unlock_irqrestore(&pTask->lock,flags);
	
	return (0);
}

/**************************************************************************
 *  Function Name:	Task_RemoveExistTask
 *
 *   Descriptions:
 *      remove the task whose event is event.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the task event
 *   Return Value: 
 *      1: removed
 *      0: doesn't exist
 *************************************************************************/
UINT32 Task_RemoveExistTask(PDSP_TASK_T pTask, UINT32 event)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	UINT32		i = 0;
	
	/* Lock */
	sc_spin_lock(&pTask->lock);

	//DBG_WLAN__TASK(LEVEL_TRACE,"**Enter %s\n", __FUNCTION__);		

	while (i < 2)
	{
		if (i == 0)
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list_high);	// check high level list
		else if ( i == 1)
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);		//check normal level list
		else
			break;
			
		while (ptmpdatablock)
		{
			if (ptmpdatablock->task_event == event)
			{
				//remove task for this event
				QueueRemoveEle(&pTask->task_list_high,&ptmpdatablock->Link);

				//put into free pool
				QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
				ptmpdatablock->in_list_flag = 1; // This block is in free list. So set this value as 1.

				DBG_WLAN__TASK(LEVEL_TRACE,"[%s]Remove task %u in %s list.\n", __FUNCTION__,event,i ? "normal" : "high");	
				/* Unlock */
				sc_spin_unlock(&pTask->lock);
				return (1);
			}
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		}
		i++;
	}
	
	//DBG_WLAN__TASK(LEVEL_TRACE,"**Exit %s\n", __FUNCTION__);	
	
	sc_spin_unlock(&pTask->lock);	
	return (0);
}

/**************************************************************************
 *  Function Name:	Task_RemoveAllExistTask
 *
 *   Descriptions:
 *      remove the task whose event is event.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *   Return Value: 
 *      1: removed
 *      0: doesn't exist
 *************************************************************************/
UINT32 Task_RemoveAllExistTask(PDSP_TASK_T pTask)
{
	PTASK_DATA_BLOCK_T ptmpdatablock, ptmpdatablock1;
	
	/* Lock */
	sc_spin_lock(&pTask->lock);
	
	DBG_WLAN__TASK(LEVEL_TRACE,"**Enter %s\n", __FUNCTION__);		

	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		QueueRemoveEle(&pTask->task_list, &ptmpdatablock->Link);
			
		DBG_WLAN__TASK(LEVEL_TRACE,"Delete task block, ID=%u,Event=%u,datalen=%u\n", 
			ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);	
		//put into free pool
		QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
		ptmpdatablock->in_list_flag = 1; // This block is in free list. So set this value as 1.

		ptmpdatablock = ptmpdatablock1;
	}
	
	//wumin: 090217.	also check high priority task list
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list_high);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		
		QueueRemoveEle(&pTask->task_list_high, &ptmpdatablock->Link);
		
		DBG_WLAN__TASK(LEVEL_TRACE,"Delete task block, ID=%u,Event=%u,datalen=%u\n", 
			ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);	
		//put into free pool
		QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
		ptmpdatablock->in_list_flag = 1; // This block is in free list. So set this value as 1.

		ptmpdatablock = ptmpdatablock1;
	}
	DBG_WLAN__TASK(LEVEL_TRACE,"**Exit %s\n", __FUNCTION__);			
	/* Unlock */
	sc_spin_unlock(&pTask->lock);
	
	return (0);
}





UINT32 Task_RemoveAllMngFrameTask(PDSP_TASK_T pTask)
{
	PTASK_DATA_BLOCK_T ptmpdatablock, ptmpdatablock1;
	
	/* Lock */
	sc_spin_lock(&pTask->lock);
	
	DBG_WLAN__TASK(LEVEL_TRACE,"**Enter %s\n", __FUNCTION__);		

	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		if (	ptmpdatablock->task_event == DSP_TASK_EVENT_CALL_MANAGEMENT)	//Justin:	080104.	remove mng task only
		{
			QueueRemoveEle(&pTask->task_list, &ptmpdatablock->Link);
			
			DBG_WLAN__TASK(LEVEL_TRACE,"Delete task block, ID=%u,Event=%u,datalen=%u\n", 
				ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);	
			//put into free pool
			QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
			ptmpdatablock->in_list_flag = 1; // This block is in free list. So set this value as 1.

		}
		ptmpdatablock = ptmpdatablock1;
	}
	
	//wumin: 090217.	also check high priority task list
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list_high);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		if (	ptmpdatablock->task_event == DSP_TASK_EVENT_CALL_MANAGEMENT)		//Justin:	080104.	remove mng task only
		{
			QueueRemoveEle(&pTask->task_list_high, &ptmpdatablock->Link);
			
			DBG_WLAN__TASK(LEVEL_TRACE,"Delete task block, ID=%u,Event=%u,datalen=%u\n", 
				ptmpdatablock->ID,ptmpdatablock->task_event,ptmpdatablock->data_len);	
			//put into free pool
			QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
			ptmpdatablock->in_list_flag = 1; // This block is in free list. So set this value as 1.
		}
		ptmpdatablock = ptmpdatablock1;
	}
	DBG_WLAN__TASK(LEVEL_TRACE,"**Exit %s\n", __FUNCTION__);			
	/* Unlock */
	sc_spin_unlock(&pTask->lock);
	
	return (0);
}

/**************************************************************************
 *  Function Name:	Task_GetHighLevelTask_DEL
 *
 *   Descriptions:
 *      remove the high task of the task_list.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *   Return Value: 
 *      1: removed
 *      0: doesn't exist
 *************************************************************************/
PTASK_DATA_BLOCK_T Task_GetHighLevelTask_DEL(PDSP_TASK_T pTask)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	
	/* Lock *///Justin:		no lock, we lock out side this function(lock before and unlock after this)
//	sc_spin_lock(&pTask->lock);
	
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);
	while (ptmpdatablock)
	{
		if (ptmpdatablock->task_pri == DSP_TASK_PRI_HIGH)
		{
			QueueRemoveEle(&pTask->task_list,&ptmpdatablock->Link);
			
			/* Unlock */
			//sc_spin_unlock_bh(&pTask->lock);
			return (ptmpdatablock);
		}
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
	}
	
	/* Unlock */
//	sc_spin_unlock(&pTask->lock);
	
	return (NULL);
}

/**************************************************************************
 *  Function Name:	Task_ThreadRoutine
 *
 *   Descriptions:
 *      The thread routine.
 *   Arguments:
 *      StartContext: IN, the thread context(always is the adapter context).
 *   Return Value:
 *      NONE
 *************************************************************************/
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)		    
//VOID Task_ThreadRoutine(struct work_struct *param_data)
//#else
//VOID Task_ThreadRoutine(void *param_data)
//#endif
void Task_ThreadRoutine(ULONG param_data)
{
	PDSP_ADAPTER_T pAdap;
	PDSP_TASK_T pTask;
	PTASK_DATA_BLOCK_T ptmpdatablock;
	UINT16    taskcount = 0;
//	pAdap  = container_of(param_data, DSP_ADAPTER_T, ppassive_task);
	pAdap = (PDSP_ADAPTER_T)param_data;
	pTask = pAdap->ppassive_task;

//	DBG_WLAN__TASK(LEVEL_TRACE,"Task_ThreadRoutine pAdap = %p, pTask = %p\n",pAdap, pTask);

//	while(1) /* Don't stop untill it received a exit event */
	{
		
		if (pTask->exit_thread_flag) /* Received a exit event */
		{
			pTask->exit_thread_flag = 0;
			DBG_WLAN__TASK(LEVEL_TRACE,"TASK return due to exit_thread_flag\n");
			return;
		}	
		if (Adap_Driver_isHalt(pAdap))
		{
			DBG_WLAN__TASK(LEVEL_TRACE, "TASK return due to halt status\n");
			return;
		}

		taskcount = 0;

		sc_spin_lock(&pTask->lock);
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list_high);
		if (ptmpdatablock == NULL)
		{
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
		}
		sc_spin_unlock(&pTask->lock);

		while (ptmpdatablock)
		{
			//if (taskcount >= MAXTASKN || Adap_Driver_isHalt(pAdap) )
			if (Adap_Driver_isHalt(pAdap))
			{
				DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: task count out of range1 \n",__FUNCTION__);
				Task_PrintAllTask(pTask);
				
				/* Lock */
				sc_spin_lock(&pTask->lock);

				if (ptmpdatablock->task_pri == DSP_TASK_PRI_HIGH)
				{
					QueuePushHead(&pTask->task_list_high, &ptmpdatablock->Link);
				}
				else
				{
					QueuePushHead(&pTask->task_list, &ptmpdatablock->Link);
				}

				/* Unlock */
				sc_spin_unlock(&pTask->lock);
				break;
			}

			//Justin: 0801   Do not excute task if we want to hardware reset
			//combo
			//if(pAdap->sys_reset_flag && (DSP_TASK_EVENT_SYS_RESET != ptmpdatablock->task_event))
			if ((pAdap->driver_state == DSP_SYS_RESET) || (pAdap->driver_state == DSP_HARDWARE_RESET))
			{
				if ((DSP_TASK_EVENT_SYS_RESET != ptmpdatablock->task_event) &&
					 (DSP_TASK_EVENT_REQJOIN_8051!= ptmpdatablock->task_event))
				{
					DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: discard task while waiting reqjoin return...event = 0x%x\n",
										__FUNCTION__, ptmpdatablock->task_event);

					/* Lock */
					sc_spin_lock(&pTask->lock);

					// If this block is already in free or task list, driver must not insert it again.
					//if (!ptmpdatablock->in_list_flag)//Justin:	080220.	I have cofused this flag before..... we can't do this 
					{
						ptmpdatablock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
						QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
					}

					ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list_high);
					if (ptmpdatablock == NULL)
					{
						ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
					}
					sc_spin_unlock(&pTask->lock);

					taskcount++;

					continue;
				}	
			}
			
			ptmpdatablock->in_list_flag = 0; // This block is not in any list. So set this value as 0.

				switch (ptmpdatablock->task_event)
				{
				case (DSP_TASK_EVENT_CALL_MANAGEMENT):
					// Mng_Receive(ptmpdatablock->data,ptmpdatablock->data_len,0,pAdap);
					Mng_Receive(ptmpdatablock->data + sizeof(UINT32),
						ptmpdatablock->data_len - sizeof(UINT32),(PRX_TO_MNG_PARAMETER)(ptmpdatablock->data),pAdap);
					/* the 3rd parameter is rssi,
					before this is called, have copy rssi to the begin of data buff*/
					break;
				case (DSP_TASK_EVENT_MNGTIMEOUT):
					Mng_Timeout(pAdap);
					break;
				case (DSP_TASK_EVENT_INTERNALRESET):
					// Call Adap_InternalReset function here.
					Adap_InternalReset(pAdap);
					break;
					
				case (DSP_TASK_EVENT_SYS_RESET)://Justin: 0731
					// Call Adap_InternalReset function here.
					Adap_SysReset(pAdap);
					break;
				case (DSP_TASK_EVENT_SCAN):
					//woody 080630
					//return when there is scan flow is running
					//especially for set ssid flow with scan procedure
					if(pAdap->scanning_flag)
					{
						DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: Task, DSP_TASK_EVENT_SCAN return due to scan flag true\n",__FUNCTION__);
						break;
					}
					
					{
						UINT32 val;
						UINT32 readCount=50;
						//for TX hang
						//Prepare for SCAN
						pAdap->bStarveMac = TRUE;
						pAdap->wlan_attr.gdevice_info.tx_disable = TRUE;
						//sc_sleep(10);
						//
						val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
						val &= 0xff;
						while(val != 0)
						{
							DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: COUNT = %x, maybe TX hang happen!!\n",__FUNCTION__,val);
							//wait 10ms
							if(readCount == 0)
							{
								break;
							}
							readCount--;
							sc_sleep(100);					
							val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
							val &= 0xff;
						}

						if(readCount == 0)
						{
							DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: #####TX hang happen, run reset ####\n",__FUNCTION__);
							Adap_Reset_Routine(pAdap);
							break;
						}
						else
						{
							Mng_InitParas(pAdap);
							Adap_Scan(pAdap);
						}
					}
					break;
				case (DSP_TASK_EVENT_BEACON_CHANGE):
					Adap_BeaconChange(pAdap);
					break;
				case (DSP_TASK_EVENT_TBTT_UPDATE):
					Adap_TbttUpdate(pAdap);
					break;
				case DSP_TASK_EVENT_OID_SET_RTS:	//add for wumin by wanghk
					Oid_Set_RTS(pAdap);
					break;	
				case (DSP_TASK_EVENT_OID_ADD_WEP):
					Oid_SetAddWep(pAdap, ptmpdatablock->data, ptmpdatablock->data_len);
					break;
				case (DSP_TASK_EVENT_OID_REMOVE_WEP):
					Oid_SetRemoveWep(pAdap, ptmpdatablock->data, ptmpdatablock->data_len);
					break;
				case (DSP_TASK_EVENT_OID_ADD_KEY):
					Oid_SetAddKey(pAdap, ptmpdatablock->data, ptmpdatablock->data_len);
					Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
					Tx_Restart(pAdap);
					//Tx_Send_Next_Frm(pAdap);
					break;
				case (DSP_TASK_EVENT_OID_REMOVE_KEY):
					Oid_SetRemoveKey(pAdap, ptmpdatablock->data, ptmpdatablock->data_len);
					break;
				case (DSP_TASK_EVENT_BEACON_LOST):
					if(	pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE
					||	pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
					{
						DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: wlan beacon lost with single or ibss mode,USB0X20 = %x,USB0X30=%x\n",
										__FUNCTION__,
										Basic_ReadRegByte(pAdap,REG_BICR1),
										Basic_ReadRegByte(pAdap,REG_BOCR2));
						Adap_SetResetType(pAdap, RESET_TYPE_DO_JOIN_TILL_BEACON,0);
						Adap_InternalReset(pAdap);
					}	
					else
					{
						//Justin:	080506.	just indicate DISCONNECTED status to up-level in combo mode. 
						//maybe add roaming support in combo mode latter.....
						pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;
						Adap_SetLink(pAdap,LINK_FALSE);
						Adap_UpdateMediaState(pAdap,(UINT8)LINK_FALSE);
						
						DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: wlan beacon lost with combo mode\n", __FUNCTION__);					
					}
					break;
				case (DSP_TASK_EVENT_OID_SET_SSID):
					//thread for running set ssid flow
					Adap_SetResetType(pAdap,ptmpdatablock->data[0],*((PUINT32)&ptmpdatablock->data[1]));
					Adap_InternalReset(pAdap);
					break;
				//combo
				case (DSP_TASK_EVENT_REQJOIN_8051):
					if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
					{
						Adap_Set_Driver_State(pAdap,DSP_SUPRISE_REMOVE);
						DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: Vcmd_Funciton_Req_JOIN failed\n", __FUNCTION__);
					}
					break;
				case (DSP_TASK_EVENT_OID_DISASSOC):
					{
						PDISASSOCIATE_COM_T  pdisass_cmd;
						pdisass_cmd = (PDISASSOCIATE_COM_T)ptmpdatablock->data;
						Mng_BssDisAssoc(pAdap, pdisass_cmd->addr, pdisass_cmd->reason);
						Mng_InitParas(pAdap);
					
						sc_sleep(100);
						
						if(pdisass_cmd->ind !=0)
						{
							Adap_SetResetType(pAdap,RESET_TYPE_DISASSOC,0);
							Adap_InternalReset(pAdap);
						}

						
					}	
					break;
				case (DSP_TASK_EVENT_DIRECTLY_DISCONNECTED):
					Adap_SetResetType(pAdap,RESET_TYPE_DIRECT_DISCONNECT,0);
					Adap_InternalReset(pAdap);
					break;
				case (DSP_TASK_EVENT_OID_INFRASTRUCTURE_MODE):
					Oid_SetInfrastructureMode(pAdap);
					break;
				case  (DSP_TASK_EVENT_HANDLE_TX_STOPPED):
	//				Adap_request_flush_tx(pAdap);	
					break;
				case (DSP_TASK_EVENT_HARDWARE_RESET):
					Adap_HardwareReset(pAdap,FALSE);
					break;
				case (DSP_TASK_EVENT_SOFT_RESET):
					Adap_SoftReset(pAdap);
					//ProtocolResetComplete();
					break;
					
#ifdef ROAMING_SUPPORT
				case (DSP_TASK_EVENT_RECONNECT):
					Mng_reconnect(pAdap);
					break;
				case (DSP_TASK_EVENT_RECOVER_REGS_JOINED):
					Mng_recover_regs_joined(pAdap);
					break;
#endif				


#ifdef NEW_SUPRISING_REMOVED
				case  (DSP_TASK_EVENT_SUPRISING_REMOVED):
					Dsp_Suprise_Removed_Handler(pAdap);
					break;
#endif

				case (DSP_TASK_EVENT_SET_POWER_ACTIVE):
	//				if(ptmpdatablock->data[0] == 1)// 1: active	0: save
					{
						Mng_SetPowerActive(pAdap,TRUE);
						Tx_Send_Next_Frm(pAdap);
					}
					break;
				case (DSP_TASK_EVENT_SET_POWER_SAVE):
					if(pAdap->wlan_attr.gdevice_info.ps_support == PSS_ACTIVE)
						break;
					
					if ((pAdap->link_ok != LINK_OK) ||(pAdap->wlan_attr.need_set_pwr_mng_bit_flag))
						break;

/*					if (!PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->ptx_packet_list)
						|| (!MngQueue_IsEmpty((PDSP_MNG_QUEUE_T)pAdap->pmng_queue_list))//more packet need to send
						|| pAdap->more_data)//more packet need to recv
	*/					break;

					Mng_SetPowerSave(pAdap);
					break;
				case (DSP_TASK_EVENT_SET_POWER_MNG_BIT):
					{
						Vcmd_set_power_mng(pAdap);
					}
					break;

		            case (DSP_TASK_EVENT_CHECK_BULK_STALL):
					Adap_CheckBulkStall(pAdap);
					break;

		            case (DSP_TASK_EVENT_BULKIN_ERROR):
					if(Adap_Device_Removed(pAdap))
						Adap_Set_Driver_State(pAdap,DSP_SUPRISE_REMOVE);
					
					pAdap->rx_error_count = 0;
					break;
			     case (DSP_TASK_EVENT_BULKOUT_ERROR):
					Vcmd_Reset_Bulkout_Request(pAdap);
					break;

#if 0					
				case (DSP_TASK_EVENT_OID_ASSOC_INFO):
//by hank			Oid_SetAssocInfo(pAdap, ptmpdatablock->data, ptmpdatablock->data_len);
					break;
				case (DSP_TASK_EVENT_OID_POWER_D3):
//todo					Oid_SetPowerD3(pAdap);
					break;
				case (DSP_TASK_EVENT_OID_POWER_D0):
//todo					Oid_SetPowerD0(pAdap);
					break;
				case (DSP_TASK_EVENT_ATPA_CHECK_TP):
					//Atpa_Do_ATPA(pAdap);
					break;
				case (DSP_TASK_EVENT_CHECK_RSSI):
					//Phy_GetRssiValueFromReg(pAdap);
					break;
				case (DSP_TASK_EVENT_OID_RELOAD_DEFAULT):
//by hank			Oid_SetReloadDefault(pAdap,ptmpdatablock->data,ptmpdatablock->data_len);
					break;
				/*Jakio 2006.12.25: add for complete fifo test*/
				case (DSP_TASK_EVENT_FIFO_TEST_COMPLETED):
					Adap_SetResetType(pAdap,RESET_TYPE_NOT_SCAN);
					Adap_InternalReset(pAdap);
					break;
				case (DSP_TASK_EVENT_FIFO_TEST_BEGIN):
					//3 get the test parameters, do a reset, then begin test
					// TODO: GET test parameters
					//Dut_DriverReset(pAdap);
					//Dut_TestFifo(PDSP_ADAPTER_T pAdap, PUINT8 buffer, UINT32 length, UINT32 nTimes);

					break;
				case (DSP_TASK_EVENT_TEST):
					// Get descriptor.
					//UsbDev_GetDeviceDescriptor((PDSP_USB_DEVICE_T)pAdap->usb_context,&usbdes);
					break;
				case (DSP_TASK_EVENT_AUTORATE):
					DBGSTR_RETRY(("DSP_TASK_EVENT_AUTORATE, data[0] = %x \n",ptmpdatablock->data[0]));
					//pAdap->tx_packet_num = 0;
					Rate_Retuen_UsingRate(pAdap);
					if(ptmpdatablock->data[0] == INT_SUB_TYPE_AUTO_RATE_UP)
					{
							//up rate
							Rate_up(pAdap,0);
					}
					else  if(ptmpdatablock->data[0] == INT_SUB_TYPE_AUTO_RATE_DOWN)
					{
							//down rate. 2 rand one time
							Rate_down_By_RSSI(pAdap,0);
							//woody 070630
							//for test, don't down rate 
					}	
					else if(ptmpdatablock->data[0] == 3)
					{
						Rate_up(pAdap,1);
					}
					break;
				case (DSP_TASK_WLANCFG_SETPARAMETERS):
//by hank			Set_OID_802_11_SETPARAM(pAdap);
					break;
#endif					
				case DSP_TASK_EVENT_ACCESS_DSP_MAILBOX:
				{
					DSP_WRITE_MAILBOX mailbox;
#ifdef ANTENNA_DIVERSITY
					if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
					{
						sc_memory_copy((PUINT8)&mailbox, ptmpdatablock->data, sizeof(DSP_WRITE_MAILBOX));
						if(mailbox.type == TASK_MAILBOX_TYPE_ANTENNA)
						{
							Adap_set_antenna(pAdap,!pAdap->wlan_attr.antenna_num);
							DBG_WLAN__TASK(LEVEL_TRACE, "Switch antenna to %x\n",pAdap->wlan_attr.antenna_num);
						}
						else if(mailbox.type == TASK_MAILBOX_TYPE_CORR)
						{
							sc_memory_copy((PUINT8)&mailbox, ptmpdatablock->data, sizeof(DSP_WRITE_MAILBOX));
							DBG_WLAN__TASK(LEVEL_TRACE, "WRITE DSP MAILBOX,ADDR = %x, val = %x \n",mailbox.addr,mailbox.val);
							VcmdW_3DSP_Mailbox(pAdap, mailbox.val, mailbox.addr,(PVOID)__FUNCTION__);
							//update variable according new set
							sc_memory_copy((PUINT8)&pAdap->wlan_attr.gdevice_info.bbreg2023, &mailbox.val, sizeof(UINT32));
						}
						else
						{
							DBG_WLAN__TASK(LEVEL_TRACE, "mailbox error case\n");
						}
					}
					else
#endif						
					{
						sc_memory_copy((PUINT8)&mailbox, ptmpdatablock->data, sizeof(DSP_WRITE_MAILBOX));
						DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: WRITE DSP MAILBOX,ADDR = %x, val = %x \n",__FUNCTION__,mailbox.addr,mailbox.val);
						VcmdW_3DSP_Mailbox(pAdap, mailbox.val, mailbox.addr,(PVOID)__FUNCTION__);
						//update variable according new set
						sc_memory_copy((PUINT8)&pAdap->wlan_attr.gdevice_info.bbreg2023, &mailbox.val, sizeof(UINT32));
						break;
					}
                    break;
				}
				case DSP_TASK_EVENT_TX_WATCH_TIMEOUT:
					Tx_Watch_TimeOut_Routine(pAdap);
					break;
				default:
					DBG_WLAN__TASK(LEVEL_TRACE, "[%s]: ptmpdatablock->task_event default\n",__FUNCTION__);
					break; 
				} 
			
			//DBGSTR(("ptmpdatablock->task_event out %x\n",ptmpdatablock->task_event));
			/* Lock */
			sc_spin_lock(&pTask->lock);

			// If this block is already in free or task list, driver must not insert it again.
			//if (!ptmpdatablock->in_list_flag)
			{
				ptmpdatablock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
				QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);
			}
#if 0
			if (ptmpdatablock->task_event == DSP_TASK_EVENT_SYS_RESET)://Justin: 0731
			{
				//Reset task
				Task_Reset((PDSP_TASK_T)pAdap->ppassive_task);
			}
#endif			
			ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list_high);
			if (ptmpdatablock == NULL)
			{
				ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
			}

			/* Unlock */
			sc_spin_unlock(&pTask->lock);
			taskcount++; 
		}
	}
}
	

