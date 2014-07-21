/*
 * REVISION HISTORY
 *   ...
 *
2006.5.26 Found a bug that driver could not send poll frame over a connection that our device is a slave station.
function Sched_SendPollFrame
{
...
if (pConnectDevice->state == BT_DEVICE_STATE_CONNECTED)
...
}
2006.5.31 Found a bug that driver should not send poll frame over a connection that a SCO connection already exists.
function Sched_SendPollFrame
{
...
if ((pConnectDevice->state == BT_DEVICE_STATE_CONNECTED) && (pConnectDevice->pScoConnectDevice == NULL))
...
}
2006.7.26 Add some codes for rx flow control.
Add three new functions: Sched_TimerRoutine, Sched_SendAclNullStopFrame, Sched_SendAclNullContinueFrame
2006.9.12 Add some codes for searilzing driver's some routines (maily for daul cpus and it has no effec on single cpu)
Pls search "BT_SERIALIZE_DRIVER" for details.
 */
#include "bt_sw.h"
#include "bt_hci.h"
#include "bt_pr.h"
#include "bt_dbg.h"
#include "sched.h"
#include "bt_frag.h"
#ifdef BT_SERIALIZE_DRIVER
#include "bt_serialize.h"
#endif
/*
 */


#ifdef BT_SCHEDULER_SUPPORT

/**************************************************************************
 *   Sched_Init
 *
 *   Descriptions:
 *      Initialize schedule module, including alloc memory for the test module and
 *		so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Test module memory is allocated with succesfully
 *      STATUS_UNSUCCESSFUL. Test module   memory is allocated with fail
 *************************************************************************/
NTSTATUS Sched_Init(PBT_DEVICE_EXT devExt)
{
	PBT_SCHED_T pSched;
	UINT32 i;
	BT_DBGEXT(ZONE_SCHED | LEVEL3,  "Sched_Init\n");
	BT_DBGEXT(ZONE_SCHED | LEVEL3,  "Sched_Init devExt = 0x%x\n", devExt);
	/* Alloc memory for scheduler module */
	pSched = (PBT_SCHED_T)ExAllocatePool(sizeof(BT_SCHED_T), GFP_KERNEL);
	if (pSched == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_SCHED | LEVEL0,  "Allocate sched memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	/* Save scheduler module pointer into device extention context */
	devExt->pSched = (PVOID)pSched;
	BT_DBGEXT(ZONE_SCHED | LEVEL3,  "Sched_Init Sched = 0x%x, size = %d\n", pSched, sizeof(BT_SCHED_T));
	/* Zero out the scheduler module space */
	RtlZeroMemory(pSched, sizeof(BT_SCHED_T));
	/* Init list */
	QueueInitList(&pSched->sched_free_pool);
	QueueInitList(&pSched->sched_list);
	/* Alloc spin lock,which used to protect task link operator */
	KeInitializeSpinLock(&pSched->lock);
	
	/* Insert all task blocks into the task free pool. */
	for (i = 0; i < MAXSCHEDBLOCKNUM; i++)
	{
		QueuePutTail(&pSched->sched_free_pool, &pSched->sched_block[i].Link);
		pSched->sched_block[i].in_list_flag = 1; // This block is in free list. So set this value as 1.
	}
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Sched_Release
 *
 *   Descriptions:
 *      Release scheduler module, including free memory for the test module.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Task module memory is released with succesfully
 *      STATUS_UNSUCCESSFUL. Task module  memory is released with fail
 *************************************************************************/
NTSTATUS Sched_Release(PBT_DEVICE_EXT devExt)
{
	PBT_SCHED_T pSched;
	BT_DBGEXT(ZONE_SCHED | LEVEL3,  "Sched_Release\n");
	/*Get pointer to the test */
	pSched = (PBT_SCHED_T)devExt->pSched;
	if (pSched == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_SCHED | LEVEL3,  "Sched_Release devExt = 0x%x, pSched = 0x%x\n", devExt, pSched);
	// Cancel timer for stopping executing sched queue.
	//KeCancelTimer(&pSched->TxExecuteSchedTimer);
	/*Empty two lists */
	QueueInitList(&pSched->sched_free_pool);
	QueueInitList(&pSched->sched_list);
	/*Free the scheduler module memory */
	ExFreePool(pSched);
	devExt->pSched = NULL;
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Sched_CheckExistSched
 *
 *   Descriptions:
 *      Check if the task queue exists the same task whose event is event.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the task event
 *   Return Value:
 *      1: exist
 *      0: doesn't exist
 *Changelog: jakio20070809
 *	1. add a para 'devExt'	
 *************************************************************************/
BOOLEAN Sched_CheckExistSched(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, UINT32 event, PUINT8 para, UINT32 len)
{
	PBT_SCHED_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	/* Lock */
	KeAcquireSpinLock(&pSched->lock, &oldIrql);
	ptmpdatablock = (PBT_SCHED_BLOCK_T)QueueGetHead(&pSched->sched_list);
	while (ptmpdatablock)
	{
		if (ptmpdatablock->sched_event == event)
		{
			if (RtlEqualMemory(ptmpdatablock->data, para, len))
			{
				/* Unlock */
				KeReleaseSpinLock(&pSched->lock, oldIrql);
				return TRUE;
			}
		}
		ptmpdatablock = (PBT_SCHED_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
	}
	/* Unlock */
	KeReleaseSpinLock(&pSched->lock, oldIrql);
	return FALSE;
}

/**************************************************************************
 *   Sched_DoOneSched
 *
 *   Descriptions:
 *      Do one scheduler.
 *   Arguments:
 *      pSched: IN, pointer to the shced moudle saved in Adapter Context.
 *      event: IN, the event value of task.
 *      pri: IN, the pri value of task.
 *      para: IN, parameter of task.
 *      len : IN, the parameter length  of task.
 *   Return Value:
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_UNSUCCESSFUL if this task is not created
 *Changelog: jakio20070809
 *	1. add a para 'devExt'	
 *************************************************************************/
NTSTATUS Sched_DoOneSched(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len)
{
	PBT_SCHED_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	LARGE_INTEGER timevalue;
	if ((devExt == NULL) || (pSched == NULL))
	{
		return (STATUS_UNSUCCESSFUL);
	}
	/* Lock */
	KeAcquireSpinLock(&pSched->lock, &oldIrql);
	/*Get a free sched data block from the sched free pool */
	ptmpdatablock = (PBT_SCHED_BLOCK_T)QueuePopHead(&pSched->sched_free_pool);
	if (ptmpdatablock == NULL)
	{
		/* Unlock */
		KeReleaseSpinLock(&pSched->lock, oldIrql);
		return (STATUS_UNSUCCESSFUL);
	}
	/*Fill some paras */
	ptmpdatablock->sched_event = event;
	ptmpdatablock->sched_pri = pri;
	ptmpdatablock->sched_len = SCHED_MIN(len, MAXSCHEDDATAN);
	if (para)
	{
		RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->sched_len);
	}
	ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.
	/* Insert the task data block into the task list */
	if (pri == SCHED_PRI_HIGH)
	{
		QueuePushHead(&pSched->sched_list, &ptmpdatablock->Link);
	}
	else
	{
		QueuePutTail(&pSched->sched_list, &ptmpdatablock->Link);
	}
	//Jakio20080124: replace timer with workitem
	UsbQueryDMASpace(devExt);
	//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
	//timevalue.QuadPart = 0;
	// Call settimer function
	//KeSetTimer(&pSched->TxExecuteSchedTimer, timevalue, &pSched->TxExecuteSchedDPC);
	//ExQueueWorkItem(&devExt->ScheduleItem, CriticalWorkQueue);
	//IoQueueWorkItem(devExt->pItem,Sched_WorkItemRoutine, CriticalWorkQueue, (PVOID)devExt);
	/* Unlock */
	KeReleaseSpinLock(&pSched->lock, oldIrql);
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Sched_SendPollFrame
 *
 *   Descriptions:
 *      Send Poll Frame.
 *   Arguments:
 *      pSched: IN, pointer to the shced moudle saved in Adapter Context.
 *      pConnectDevice: IN, the parater.
 *   Return Value:
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_UNSUCCESSFUL if this task is not created
 *Changelog: jakio20070809
 *	1. add a para 'devExt'	
 *************************************************************************/
NTSTATUS Sched_SendPollFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice)
{
	if ((devExt == NULL) || (pSched == NULL))
	{
		return (STATUS_UNSUCCESSFUL);
	}
	if(pConnectDevice->role_switching_flag == 1)
	{
		BT_DBGEXT(ZONE_SCHED | LEVEL3, "Sched_SendPollFrame----role switch is in processing, do not send poll frames\n");
		return (STATUS_UNSUCCESSFUL);
	}

	//Jeff 090120 Debug
	if(pConnectDevice->mode_Sniff_debug1 == 1)
	{
		//For debug temporary code
		return STATUS_UNSUCCESSFUL;
	}

	if ((pConnectDevice->state == BT_DEVICE_STATE_CONNECTED) && (pConnectDevice->pScoConnectDevice == NULL))
	{
		if (!Sched_CheckExistSched(devExt, pSched, SCHED_EVENT_SEND_POLL, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)))
		{
			return (Sched_DoOneSched(devExt, pSched, SCHED_EVENT_SEND_POLL, SCHED_PRI_NORMAL, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)));
		}
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *   Sched_SendAclNullStopFrame
 *
 *   Descriptions:
 *      Send Null Acl Frame.
 *   Arguments:
 *      pSched: IN, pointer to the shced moudle saved in Adapter Context.
 *      pConnectDevice: IN, the parater.
 *   Return Value:
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_UNSUCCESSFUL if this task is not created
 *Changelog: jakio20070809
 *	1. add a para 'devExt'	
 *************************************************************************/
NTSTATUS Sched_SendAclNullStopFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice)
{
	if ((devExt == NULL) || (pSched == NULL))
	{
		return (STATUS_UNSUCCESSFUL);
	}
	if (pConnectDevice->pScoConnectDevice == NULL)
	{
		if (!Sched_CheckExistSched(devExt, pSched, SCHED_EVENT_NULL_ACL_STOP, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)))
		{
			return (Sched_DoOneSched(devExt, pSched, SCHED_EVENT_NULL_ACL_STOP, SCHED_PRI_NORMAL, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)));
		}
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *   Sched_SendAclNullContinueFrame
 *
 *   Descriptions:
 *      Send Null Acl Frame.
 *   Arguments:
 *      pSched: IN, pointer to the shced moudle saved in Adapter Context.
 *      pConnectDevice: IN, the parater.
 *   Return Value:
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_UNSUCCESSFUL if this task is not created
 *Changelog: jakio20070809
 *	1. add a para 'devExt'	
 *************************************************************************/
NTSTATUS Sched_SendAclNullContinueFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice)
{
	if ((devExt == NULL) || (pSched == NULL))
	{
		return (STATUS_UNSUCCESSFUL);
	}
	if (pConnectDevice->pScoConnectDevice == NULL)
	{
		if (!Sched_CheckExistSched(devExt, pSched, SCHED_EVENT_NULL_ACL_CONTINUE, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)))
		{
			return (Sched_DoOneSched(devExt, pSched, SCHED_EVENT_NULL_ACL_CONTINUE, SCHED_PRI_NORMAL, (PUINT8) &pConnectDevice, sizeof(PCONNECT_DEVICE_T)));
		}
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *   Sched_Excute
 *
 *   Descriptions:
 *      The timeout function routine.
 *   Arguments:
 *      Dpc: IN, not Used.
 *      DeferredContext: IN, really points to the device extension.
 *      SystemContext1: IN, not Used.
 *      SystemContext2: IN, not Used.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Sched_Excute(PBT_DEVICE_EXT devExt)
{
	PBT_SCHED_T pSched;
	PBT_SCHED_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	UINT32 event;
	UINT16 taskcount = 0;
	/*Get pointer of the task module  */
	pSched = (PBT_SCHED_T)devExt->pSched;

	if(pSched == NULL)
		return;
	
	/* Lock */
	KeAcquireSpinLock(&pSched->lock, &oldIrql);
	taskcount = 0;
	ptmpdatablock = (PBT_SCHED_BLOCK_T)QueuePopHead(&pSched->sched_list);
	/* Unlock */
	KeReleaseSpinLock(&pSched->lock, oldIrql);
	//while (ptmpdatablock)
	if(ptmpdatablock)
	{
		ptmpdatablock->in_list_flag = 0; // This block is not in any list. So set this value as 0.
		/* Get event */
		event = ptmpdatablock->sched_event;
		if (event == SCHED_EVENT_SEND_POLL)
		{
			
			PCONNECT_DEVICE_T pConnectDevice;
			// Get connect device
			RtlCopyMemory(&pConnectDevice, ptmpdatablock->data, ptmpdatablock->sched_len);
			if (BtProcessTxPollFrame(devExt, pConnectDevice) != STATUS_SUCCESS)
			{
				/* Lock */
				KeAcquireSpinLock(&pSched->lock, &oldIrql);
				ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.
				QueuePushHead(&pSched->sched_list, &ptmpdatablock->Link);
				/* Unlock */
				KeReleaseSpinLock(&pSched->lock, oldIrql);
				//break;
			}
			else
			{
				/* Lock */
				KeAcquireSpinLock(&pSched->lock, &oldIrql);
				ptmpdatablock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
				QueuePutTail(&pSched->sched_free_pool, &ptmpdatablock->Link);
				/* Unlock */
				KeReleaseSpinLock(&pSched->lock, oldIrql);
			}
			
		}
		else if (event == SCHED_EVENT_NULL_ACL_STOP)
		{
			{
				PCONNECT_DEVICE_T pConnectDevice;
				// Get connect device
				RtlCopyMemory(&pConnectDevice, ptmpdatablock->data, ptmpdatablock->sched_len);
				if (BtProcessTxAclNullFrame(devExt, pConnectDevice, 0) != STATUS_SUCCESS)
				{
					/* Lock */
					KeAcquireSpinLock(&pSched->lock, &oldIrql);
					ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.
					QueuePushHead(&pSched->sched_list, &ptmpdatablock->Link);
					/* Unlock */
					KeReleaseSpinLock(&pSched->lock, oldIrql);
					//break;
				}
				else
				{
					pConnectDevice->l2cap_rx_flow_control_flag = 1;
					/* Lock */
					KeAcquireSpinLock(&pSched->lock, &oldIrql);
					ptmpdatablock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
					QueuePutTail(&pSched->sched_free_pool, &ptmpdatablock->Link);
					/* Unlock */
					KeReleaseSpinLock(&pSched->lock, oldIrql);
				}
			}
		}
		else if (event == SCHED_EVENT_NULL_ACL_CONTINUE)
		{
			{
				PCONNECT_DEVICE_T pConnectDevice;
				// Get connect device
				RtlCopyMemory(&pConnectDevice, ptmpdatablock->data, ptmpdatablock->sched_len);
				if (BtProcessTxAclNullFrame(devExt, pConnectDevice, 1) != STATUS_SUCCESS)
				{
					/* Lock */
					KeAcquireSpinLock(&pSched->lock, &oldIrql);
					ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.
					QueuePushHead(&pSched->sched_list, &ptmpdatablock->Link);
					/* Unlock */
					KeReleaseSpinLock(&pSched->lock, oldIrql);
					//break;
				}
				else
				{
					pConnectDevice->l2cap_rx_flow_control_flag = 0;
					/* Lock */
					KeAcquireSpinLock(&pSched->lock, &oldIrql);
					ptmpdatablock->in_list_flag = 1; // This block will be in free list. So set this value as 1.
					QueuePutTail(&pSched->sched_free_pool, &ptmpdatablock->Link);
					/* Unlock */
					KeReleaseSpinLock(&pSched->lock, oldIrql);
				}
			}
		}
	}
}





VOID Sched_DeleteAllPoll(PBT_DEVICE_EXT devExt)
{
	
	PBT_SCHED_T		pSched;
	PBT_SCHED_BLOCK_T ptmpdatablock, ptmpdatablock1;
	KIRQL oldIrql;
	UINT32	PollCount = 0;

	pSched = (PBT_SCHED_T)devExt->pSched;
	if(pSched == NULL)
	{
		return;
	}
	
	/* Lock */
	KeAcquireSpinLock(&pSched->lock, &oldIrql);
	ptmpdatablock = (PBT_SCHED_BLOCK_T)QueueGetHead(&pSched->sched_list);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PBT_SCHED_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		if (ptmpdatablock->sched_event == SCHED_EVENT_SEND_POLL)
		{
			PollCount++;
			//if (RtlEqualMemory(ptmpdatablock->data, para, len))
			{
				// Remove the connect device block from the used am list.
				QueueRemoveEle(&pSched->sched_list, &ptmpdatablock->Link);
				// Insert the connect device block into the tail of free list.
				QueuePutTail(&pSched->sched_free_pool, &ptmpdatablock->Link);
				//break;
			}
		}
		ptmpdatablock = ptmpdatablock1;
	}
	/* Unlock */
	KeReleaseSpinLock(&pSched->lock, oldIrql);
	BT_DBGEXT(ZONE_SCHED | LEVEL3, "Sched_DeleteAllPoll---delete poll frame count: %d\n", PollCount);
}





/**************************************************************************
 *   Sched_ScanAllLink
 *
 *   Descriptions:
 *      The timeout function routine.
 *   Arguments:
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Sched_ScanAllLink(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	PBT_SCHED_T pSched;
	PCONNECT_DEVICE_T pConnectDevice;
	KIRQL oldIrql;
	pHci = (PBT_HCI_T)devExt->pHci;
	/*Get pointer of the task module  */
	pSched = (PBT_SCHED_T)devExt->pSched;
	if ((pHci == NULL) || (pSched == NULL))
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if ((pConnectDevice->tx_count_frame < SCHEDMINTXFRAME) && (pConnectDevice->pScoConnectDevice == NULL))
		{
			Sched_SendPollFrame(devExt, pSched, pConnectDevice);
		}
		pConnectDevice->tx_count_frame = 0;
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if ((pConnectDevice->tx_count_frame < SCHEDMINTXFRAME) && (pConnectDevice->pScoConnectDevice == NULL))
		{
			Sched_SendPollFrame(devExt, pSched, pConnectDevice);
		}
		pConnectDevice->tx_count_frame = 0;
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

/**************************************************************************
 *   Sched_DeleteSched
 *
 *   Descriptions:
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the task event
 *   Return Value:
 *      1: exist
 *      0: doesn't exist
 *************************************************************************/
VOID Sched_DeleteSched(PBT_SCHED_T pSched, UINT32 event, PUINT8 para, UINT32 len)
{
	PBT_SCHED_BLOCK_T ptmpdatablock, ptmpdatablock1;
	KIRQL oldIrql;
	/* Lock */
	KeAcquireSpinLock(&pSched->lock, &oldIrql);
	ptmpdatablock = (PBT_SCHED_BLOCK_T)QueueGetHead(&pSched->sched_list);
	while (ptmpdatablock)
	{
		ptmpdatablock1 = (PBT_SCHED_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
		if (ptmpdatablock->sched_event == event)
		{
			if (RtlEqualMemory(ptmpdatablock->data, para, len))
			{
				// Remove the connect device block from the used am list.
				QueueRemoveEle(&pSched->sched_list, &ptmpdatablock->Link);
				// Insert the connect device block into the tail of free list.
				QueuePutTail(&pSched->sched_free_pool, &ptmpdatablock->Link);
				break;
			}
		}
		ptmpdatablock = ptmpdatablock1;
	}
	/* Unlock */
	KeReleaseSpinLock(&pSched->lock, oldIrql);
}



#endif /*BT_SCHEDULER_SUPPORT*/
