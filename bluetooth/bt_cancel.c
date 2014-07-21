/************************************************************************
(c) 2003-05, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
 **************************************************************************
$RCSfile: bt_cancel.c,v $
$Revision: 1.5 $
$Date: 2010/09/06 05:17:26 $
 **************************************************************************/

#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include hci module
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_task.h"
#include "bt_usb_vendorcom.h"
#include "bt_frag.h"

#ifdef BT_SCHEDULER_SUPPORT
	#include "sched.h"
#endif
#ifdef BT_SERIALIZE_DRIVER
	#include "bt_serialize.h"
#endif
VOID CancelInProgressRead(PBT_DEVICE_EXT devExt);







VOID BtWatchdogTimer(ULONG_PTR DeviceObject)
{
	PBT_DEVICE_EXT devExt;
	NTSTATUS Status;
	UINT32	ret;

    ENTER_FUNC();
	devExt = (PBT_DEVICE_EXT)DeviceObject;

    if(FALSE == BtIsPluggedIn(devExt))
	{
	    BT_DBGEXT(ZONE_CANCEL | LEVEL0, "Unplugged\n");
        EXIT_FUNC();
		return;
	}

	//BT_DBGEXT(ZONE_CANCEL | LEVEL2, "Btwatchdog timer entered\n");
	//reset timer
	devExt->watchDogTimer.expires = (unsigned long)jiffies + HZ;
	add_timer(&devExt->watchDogTimer);

	//slave connect timeout function
	Hci_SlaveConnectionTimeOut(devExt);

	//check write irp completion status
	BtUsbWriteIrpTimeOut(devExt);

	BtUsbRxFailTimeOut(devExt);

	Status =UsbQueryDMASpace(devExt);
	if(Status == STATUS_UNSUCCESSFUL)
	{
		devExt->QueryFailCount++;
		if(devExt->QueryFailCount >= 2)
		{
			BT_DBGEXT(ZONE_CANCEL | LEVEL0, "BtWatchdogTimer--Error, vcmd 'query space' lost\n");
			devExt->SpaceQueryFlag = FALSE;
			UsbQueryDMASpace(devExt);	
		}
	}
	else
	{
		devExt->QueryFailCount = 0;
	}

	
	if(devExt->WorkItemRun == FALSE)
	{
		//BT_DBGEXT(ZONE_CANCEL | LEVEL3, "queue work item\n");
		ret = queue_delayed_work(devExt->pBtWorkqueue, &devExt->BtWorkitem, 0);
		if (ret == 0)
		{
			//BT_DBGEXT(ZONE_CANCEL | LEVEL1, "queue_work function failed!\n");
			return;
		}
		devExt->WorkItemRun = TRUE;
	} 

    EXIT_FUNC();
	return ;
}
///////////////////////////////////////////////////////////////////////////////
//
//
// BtWatchdogTimer
//
//  This function is called once per second per device object support.
//  It's role is to check to ensure that the currently active read and write
//  requests don't "hang" on the device.  If either requests take longer
//  than the prescribed time to complete, this function cancels the request.
//
//  INPUTS:
//
//      DeviceObject - Address of our DEVICE_OBJECT.
//
//      Context - Not used.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//    None.
//
//  IRQL:
//
//    This routine is called at IRQL DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
VOID BtWatchdogItem(struct work_struct *pwork)
#else
VOID BtWatchdogItem(ULONG_PTR pdevExt)
#endif
{
	PBT_DEVICE_EXT devExt;
	PBT_HCI_T pHci = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)      

	struct delayed_work *dw;    
	
	/* Parse the structure */      
	dw = container_of(pwork, struct delayed_work, work);
	devExt = (PBT_DEVICE_EXT)container_of(dw, BT_DEVICE_EXT, BtWorkitem);
#else	
	devExt = (PBT_DEVICE_EXT)pdevExt;
#endif
	ENTER_FUNC();	
	/* Parse the structure */

	if (devExt == NULL)
	{
		return;	
	}
	if(devExt->State != STATE_STARTED)
	{
		BT_DBGEXT(ZONE_CANCEL | LEVEL1, "BtWatchdogItem--driver not started\n");
		return;
	}
	
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
		return;
	
	Hci_Timeout(devExt);
	LMP_PDU_Timeout(devExt);
	Hci_ProtectHciCommandTimeout(devExt);
	Hci_L2capFlowTimeout(devExt);
	#ifdef BT_SCHEDULER_SUPPORT
		Sched_ScanAllLink(devExt);
	#endif

	Hci_AFHCheckTimeout(devExt);

	Hci_ProtectInquiryTimeout(devExt);
	Hci_PeriodicInquiryTimeout(devExt);
	Hci_ClassificationCheckTimeout(devExt);

	devExt->WorkItemRun = FALSE;

	EXIT_FUNC();
	return ;
}
///////////////////////////////////////////////////////////////////////////////
//
// CancelInProgressRead
//
//    This is a synchronize function, called with the ISR spinlock, and the
//    read queue spin lock HELD.
//    Returns with these locks HELD.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      TRUE.
//
//  IRQL:
//
//      DIRQL
//
//  NOTES:
//      This function is called inside the protection of spin lock. So it must not
//      be locked again.
//
//
///////////////////////////////////////////////////////////////////////////////
VOID CancelInProgressRead(PBT_DEVICE_EXT devExt)
{
		UINT8 i;
		for (i = 0; i < BT_TOTAL_SCO_LINK_COUNT; i++)
		{
			((PBT_FRAG_T)devExt->pFrag)->RxScoElement[i].RxCachCid = ((PBT_FRAG_T)devExt->pFrag)->RxScoElement[i].RxCachPid;
		}
	devExt->RxCachCid = devExt->RxCachPid;
}
VOID BtClearQueues(PBT_DEVICE_EXT DevExt)
{

	KIRQL oldIrql;
	KIRQL CancelIrql;
	PBT_FRAG_T			pFrag;
	PBT_FRAG_ELEMENT_T	pEle;
	PBT_LIST_ENTRY_T		pList = NULL;
	PPDU_TX_LIST_T		pLmpEle;
	UINT8				ListIndex;
	KeAcquireSpinLock(&DevExt->ReadQueueLock, &oldIrql);
	CancelInProgressRead(DevExt);
	KeReleaseSpinLock(&DevExt->ReadQueueLock, oldIrql);


	pFrag = (PBT_FRAG_T)DevExt->pFrag;
	ASSERT(pFrag);
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	//clear acl queue
	for(ListIndex=FRAG_MASTER_LIST; ListIndex < FRAG_SLAVE1_LIST; ListIndex++)
	{
		switch(ListIndex)
		{
		case FRAG_MASTER_LIST:
			pList = &pFrag->Master_FragList;
			break;
		case FRAG_SLAVE_LIST:
			pList = &pFrag->Slave_FragList;
			break;
		case FRAG_SLAVE1_LIST:
			pList = &pFrag->Slave1_FragList;
			break;
		case FRAG_SNIFF_LIST:
			pList = &pFrag->Sniff_FragList;
			break;
		}
		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(pList);
		while(pEle != NULL)
		{
			
			QueuePutTail(&pFrag->Free_FragList, &pEle->link);
			pFrag->FreeFrag_Count++;
			
			pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(pList);
		}
	}
	

	pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Sco_FragList);
	while(pEle != NULL)
	{
		
		QueuePutTail(&pFrag->Free_FragList, &pEle->link);
		pFrag->FreeFrag_Count++;
		
		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Sco_FragList);
	}

	BT_DBGEXT(ZONE_CANCEL | LEVEL3, "BtClearQueues: free frag count is %d\n", pFrag->FreeFrag_Count);
	

	pLmpEle = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_UsedList);
	while(pLmpEle != NULL)
	{
		pLmpEle->pConnectDevice = NULL;
		QueuePutTail(&pFrag->Pdu_FreeList, &pLmpEle->Link);
		pFrag->FreePdu_Count++;
		
		pLmpEle = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_UsedList);
	}
	BT_DBGEXT(ZONE_CANCEL | LEVEL3, "BtClearQueues: free lmp pdu count is %d\n", pFrag->FreePdu_Count);

	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

	return;

}



VOID BtClearIvtData(PBT_DEVICE_EXT DevExt)
{

	KIRQL oldIrql;
	KIRQL CancelIrql;
	PBT_FRAG_T			pFrag;
	PBT_FRAG_ELEMENT_T	pEle;
	PBT_LIST_ENTRY_T		pList;
	PPDU_TX_LIST_T		pLmpEle;
	UINT8				ListIndex;
	PHCI_DATA_HEADER_T	phead;
	COMPLETE_PACKETS_T CompletePackets;


	pFrag = (PBT_FRAG_T)DevExt->pFrag;
	ASSERT(pFrag);

	// Initialize complete packets
	RtlZeroMemory(&CompletePackets, sizeof(COMPLETE_PACKETS_T));

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	for(ListIndex=FRAG_MASTER_LIST; ListIndex < FRAG_SLAVE1_LIST; ListIndex++)
	{
		switch(ListIndex)
		{
		case FRAG_MASTER_LIST:
			pList = &pFrag->Master_FragList;
			break;
		case FRAG_SLAVE_LIST:
			pList = &pFrag->Slave_FragList;
			break;
		case FRAG_SLAVE1_LIST:
			pList = &pFrag->Slave1_FragList;
			break;
		case FRAG_SNIFF_LIST:
			pList = &pFrag->Sniff_FragList;
			break;
        default:
            pList = &pFrag->Master_FragList;
		}
		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(pList);
		while(pEle != NULL)
		{

			phead = (PHCI_DATA_HEADER_T)pEle->memory;
			CompletePackets.number_of_handles++;
			CompletePackets.connection_handle[CompletePackets.number_of_handles] = (UINT16)phead->connection_handle;
			CompletePackets.num_of_complete_packets[CompletePackets.number_of_handles] = 1;	
			QueuePutTail(&pFrag->Free_FragList, &pEle->link);
			pFrag->FreeFrag_Count++;
			
			pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(pList);
		}
	}
	

	pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Sco_FragList);
	while(pEle != NULL)
	{
		QueuePutTail(&pFrag->Free_FragList, &pEle->link);
		pFrag->FreeFrag_Count++;
		
		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Sco_FragList);
	}

	BT_DBGEXT(ZONE_CANCEL | LEVEL3, "BtClearQueues: free frag count is %d\n", pFrag->FreeFrag_Count);
	

	pLmpEle = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_UsedList);
	while(pLmpEle != NULL)
	{
		pLmpEle->pConnectDevice = NULL;
		QueuePutTail(&pFrag->Pdu_FreeList, &pLmpEle->Link);
		pFrag->FreePdu_Count++;
		
		pLmpEle = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_UsedList);
	}
	BT_DBGEXT(ZONE_CANCEL | LEVEL3, "BtClearQueues: free lmp pdu count is %d\n", pFrag->FreePdu_Count);

	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);


	if (CompletePackets.number_of_handles > 0)
	{
		Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(DevExt, (PUINT8) &CompletePackets, 1);
	}

	return;

}
