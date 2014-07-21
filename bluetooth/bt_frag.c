/****************************************************************
 * FILENAME:     BT_Frag.c     CURRENT VERSION: 1.00.01
 * CREATE DATE:  2005/11/29
 * PURPOSE:      All the functions for frag and defrag.
 *
 * AUTHORS:       Jason dong
 *
 *
 ****************************************************************/

#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include some hci operation
#include "bt_task.h"
#include "bt_lmp.h"
#include "bt_frag.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
#include "bt_queue.h"
#ifdef BT_PCM_COMPRESS
	#include "g711.h"
	#include "cvsd.h"
#endif
#ifdef BT_TESTDRIVER
#endif
#ifdef BT_SCHEDULER_SUPPORT
	#include "sched.h"
#endif
#ifdef BT_TESTMODE_SUPPORT
	#include "bt_testmode.h"
#endif
#ifdef BT_SERIALIZE_DRIVER
	#include "bt_serialize.h"
#endif
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
/*--file local variables-----------------------------------------------*/
const UINT16 PACKET_MAX_BYTES[0x10] =
{
	0, 0, 0, 17, 27, 20, 40, 60, 9, 29, 121, 183, 0, 0, 224, 339
};

/* jakio20071026: add here for EDR
*/
const UINT16 EDR_PACKET_MAX_BYTES[0x10] = 
{
   0,   0,   0,  17,  54,  0,   0,   0,    83,  29,  367,  552,  0,    0,    679,  1021  
};
const UINT16 PacketSlotFlag[6] = 
{
	0x0010, 0x0110, 0x0110, 0x0D10, 0x0D10, 0xCD10
};


/*--file local function prototypes-------------------------------------*/
/**************************************************************************
 *   Frag_Init
 *
 *   Descriptions:
 *      Initialize frag module, including alloc memory for the test module and
 *		so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Test module memory is allocated with succesfully
 *      STATUS_UNSUCCESSFUL. Test module   memory is allocated with fail
 *************************************************************************/
NTSTATUS Frag_Init(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T pFrag;
	UINT32	i;
	
	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_Init\n");
	/* Alloc memory for frag module */
	pFrag = (PBT_FRAG_T)ExAllocatePool(sizeof(BT_FRAG_T), GFP_KERNEL);
	if (pFrag == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Allocate frag memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	/* Save frag module pointer into device extention context */
	devExt->pFrag = (PVOID)pFrag;
	BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_Init Frag = 0x%x, size = %d\n", pFrag, sizeof(BT_FRAG_T));
	/* Zero out the frag module space */
	RtlZeroMemory(pFrag, sizeof(BT_FRAG_T));

	//init spin_lock
	KeInitializeSpinLock(&pFrag->FragTxLock);

	QueueInitList(&pFrag->Master_FragList);
	QueueInitList(&pFrag->Slave_FragList);
	QueueInitList(&pFrag->Slave1_FragList);
	QueueInitList(&pFrag->Sniff_FragList);
	QueueInitList(&pFrag->Sco_FragList);
	QueueInitList(&pFrag->Free_FragList);
	pFrag->FreeFrag_Count = BT_FRAG_MAX_ELEMENTS;
	pFrag->AllowSendingFlag = MASK_BITS_ALLOW_SENDING;
	for(i = 0; i < BT_FRAG_MAX_ELEMENTS; i++)
	{
		QueuePutTail(&pFrag->Free_FragList, &pFrag->TxFragElement[i].link);
	}

	for(i = FRAG_MASTER_LIST; i < FRAG_SCO_LIST; i++)
	{
		pFrag->IdleSpace[i] = 0;
		pFrag->AclNeedProcFlag[i] = 1;
	}

	pFrag->AclProcCounter = 0;
	pFrag->TxDisable_Flag = 0;
	QueueInitList(&pFrag->Pdu_FreeList);
	QueueInitList(&pFrag->Pdu_UsedList);
	pFrag->FreePdu_Count = MAX_PDU_TX_QUEUE;
	for(i = 0; i<MAX_PDU_TX_QUEUE; i++)
	{
		QueuePutTail(&pFrag->Pdu_FreeList, &pFrag->TxPduElement[i].Link);
	}

	//init tx timer for tx data
	//KeInitializeTimer(&pFrag->TxTimer);
	init_timer(&pFrag->TxTimer);
	pFrag->TxTimer.function = Frag_DataSendingTimer;
	pFrag->TxTimer.data = (ULONG_PTR)devExt;
	
	return (STATUS_SUCCESS);
}



VOID Frag_Reset(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T pFrag;
	UINT32	i;

	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_Reset entered\n");

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);


	QueueInitList(&pFrag->Master_FragList);
	QueueInitList(&pFrag->Slave_FragList);
	QueueInitList(&pFrag->Slave1_FragList);
	QueueInitList(&pFrag->Sniff_FragList);
	QueueInitList(&pFrag->Sco_FragList);
	QueueInitList(&pFrag->Free_FragList);
	pFrag->FreeFrag_Count = BT_FRAG_MAX_ELEMENTS;
	pFrag->AllowSendingFlag = MASK_BITS_ALLOW_SENDING;
	for(i = 0; i < BT_FRAG_MAX_ELEMENTS; i++)
	{
		QueuePutTail(&pFrag->Free_FragList, &pFrag->TxFragElement[i].link);
	}

	pFrag->TxDisable_Flag = 0;
	
	QueueInitList(&pFrag->Pdu_FreeList);
	QueueInitList(&pFrag->Pdu_UsedList);
	pFrag->FreePdu_Count = MAX_PDU_TX_QUEUE;
	for(i = 0; i<MAX_PDU_TX_QUEUE; i++)
	{
		QueuePutTail(&pFrag->Pdu_FreeList, &pFrag->TxPduElement[i].Link);
	}
}




/**************************************************************************
 *   Frag_Release
 *
 *   Descriptions:
 *      Release frag module, including free memory for the test module.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Task module memory is released with succesfully
 *      STATUS_UNSUCCESSFUL. Task module  memory is released with fail
 *************************************************************************/

NTSTATUS Frag_Release(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T pFrag;
	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_Release\n");
	/*Get pointer to the test */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if (pFrag == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_Release devExt = 0x%x, pFrag = 0x%x\n", devExt, pFrag);
	/*Free the test module memory */
	if (pFrag != NULL)
	{
		ExFreePool((PVOID)pFrag);
	}
	devExt->pFrag = NULL;
	return (STATUS_SUCCESS);
}





VOID Frag_DataSendingTimer(ULONG_PTR SystemContext)
{
	PBT_DEVICE_EXT devExt = (PBT_DEVICE_EXT)SystemContext;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT32	i;
	UINT32	pduNum;
	UINT8	TimerSrc;
	PBT_FRAG_T	pFrag;
	KIRQL	oldIrql;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return;

	UsbQueryDMASpace(devExt);
}

VOID Frag_CheckTxQueue(PBT_DEVICE_EXT devExt)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8	i;
	UINT8	NeedQuery = 0;
	UINT32	pduNum;
	UINT8	ListIndex;
	UINT8	TimerSrc;
	PBT_FRAG_T	pFrag;
	KIRQL	oldIrql, globalIrql;
	UINT8	AclListEmpty = 0;

	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return;

	if(devExt->StartStage != START_STAGE_MAINLOOP)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckTxQueue--not in mainloop, return\n");
		return;
	}

	
	if(pFrag->TxDisable_Flag == 1)
	{
		goto Check_Sco;
	}
	

	#ifdef BT_3DSP_FLUSH_SUPPORT
	if(TRUE == Flush_HasData_ToSend(devExt, &ListIndex))
	{
		Flush_SendBakup_Packet(devExt, ListIndex);
		return;
	}
	#endif

	if(BtIsTxPoolFull(devExt) == FALSE)
    {   
		LMP_CheckTxQueue(devExt);
    }
    else
    {
        BT_DBGEXT(ZONE_FRG | LEVEL4, "Tx Pool is FULL\n");
    }

	#ifdef BT_TESTMODE_SUPPORT
	if (((PBT_HCI_T)devExt->pHci)->test_flag)
	{
		 if (((PBT_HCI_T)devExt->pHci)->test_mode_active)
		 {
			if(BtIsTxPoolFull(devExt) == FALSE)
			{
				TestMode_TransmitData(devExt);
			}
			return;
		 }
	}
	#endif



	/* jakio20080124: check schedule queue
*/
	#ifdef BT_SCHEDULER_SUPPORT
	if(BtIsTxPoolFull(devExt) == FALSE)
		Sched_Excute(devExt);
	#endif

	if(BtIsTxPoolFull(devExt) == FALSE)
	{
		for(i = 0; i < 4; i++)
		{
			ListIndex = (UINT8)((i+pFrag->AclProcCounter)%4 + 1);
			ntStatus = Frag_ProcessAclFrag(devExt, ListIndex);
			pFrag->AclNeedProcFlag[i] = 0;
			if(ntStatus == BT_FRAG_SUCCESS)
			{	
				break;
			}
			
		}
		pFrag->AclProcCounter++;
	}

Check_Sco:		
	if(BtIsTxScoPoolFull(devExt) == FALSE)
	{
		ntStatus = Frag_ProcessScoFrag(devExt);	
	}
	
}



UINT32 Frag_GetNextEleId(UINT32 id)
{
	UINT32 tmpid = id;
	ASSERT(tmpid < BT_FRAG_MAX_ELEMENTS);
	if (tmpid == BT_FRAG_MAX_ELEMENTS - 1)
	{
		return (0);
	}
	else
	{
		return (++tmpid);
	}
}

/*
	BOOLEAN Frag_IsEleEmpty(PBT_FRAG_T pFrag)
	{
		ASSERT(pFrag->ele_TxPid < BT_FRAG_MAX_ELEMENTS);
		ASSERT(pFrag->ele_TxCid < BT_FRAG_MAX_ELEMENTS);
		return (pFrag->ele_TxPid == pFrag->ele_TxCid);
	}
*/
BOOLEAN Frag_IsEleEmpty(PBT_DEVICE_EXT devExt)
{
	return Frag_IsEleEmpty_New(devExt, FRAG_ALL_LIST);
}

BOOLEAN Frag_IsEleEmpty_New(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{
	PBT_FRAG_T pFrag;

	ASSERT(devExt != NULL);
	ASSERT(devExt->pFrag != NULL);

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if (ListFlag == FRAG_MASTER_LIST)
	{
		return (QueueEmpty(&pFrag->Master_FragList));
	}
	else if (ListFlag == FRAG_SLAVE_LIST)
	{
		return (QueueEmpty(&pFrag->Slave_FragList));
	}
	else if (ListFlag == FRAG_SNIFF_LIST)
	{
		return (QueueEmpty(&pFrag->Sniff_FragList));
	}
	else if (ListFlag == FRAG_SLAVE1_LIST)
	{
		return (QueueEmpty(&pFrag->Slave1_FragList));
	}
	else
	{
		return (QueueEmpty(&pFrag->Master_FragList) && QueueEmpty(&pFrag->Slave_FragList) && 
			QueueEmpty(&pFrag->Sniff_FragList) && QueueEmpty(&pFrag->Slave1_FragList));
	}
}

/*
	BOOLEAN Frag_IsEleFull(PBT_FRAG_T pFrag)
	{
		ASSERT(pFrag->ele_TxPid < BT_FRAG_MAX_ELEMENTS);
		ASSERT(pFrag->ele_TxCid < BT_FRAG_MAX_ELEMENTS);
		return (Frag_GetNextEleId(pFrag->ele_TxPid) == pFrag->ele_TxCid);
	}

	BOOLEAN Frag_IsCmdEleFull(PBT_FRAG_T pFrag)
	{
		ASSERT(pFrag->ele_TxCmdPid < BT_FRAG_MAX_ELEMENTS);
		ASSERT(pFrag->ele_TxCmdCid < BT_FRAG_MAX_ELEMENTS);
		return (Frag_GetNextEleId(pFrag->ele_TxCmdPid) == pFrag->ele_TxCid);
	}

	BOOLEAN Frag_IsCmdEleEmpty(PBT_FRAG_T pFrag)
	{
		ASSERT(pFrag->ele_TxCmdPid < BT_FRAG_MAX_ELEMENTS);
		ASSERT(pFrag->ele_TxCmdCid < BT_FRAG_MAX_ELEMENTS);
		return (pFrag->ele_TxCmdPid == pFrag->ele_TxCmdCid);
	}
*/
/*
BOOLEAN Frag_IsScoEleEmpty(PBT_FRAG_T pFrag)
{
	ASSERT(pFrag->ele_TxScoPid < BT_FRAG_MAX_ELEMENTS);
	ASSERT(pFrag->ele_TxScoCid < BT_FRAG_MAX_ELEMENTS);
	return (pFrag->ele_TxScoPid == pFrag->ele_TxScoCid);
}
*/
BOOLEAN Frag_IsScoEleEmpty(PBT_FRAG_T pFrag)
{
	ASSERT(pFrag);
	return QueueEmpty(&pFrag->Sco_FragList);
}
/*
BOOLEAN Frag_IsScoEleFull(PBT_FRAG_T pFrag)
{
	ASSERT(pFrag->ele_TxScoPid < BT_FRAG_MAX_ELEMENTS);
	ASSERT(pFrag->ele_TxScoCid < BT_FRAG_MAX_ELEMENTS);
	return (Frag_GetNextEleId(pFrag->ele_TxScoPid) == pFrag->ele_TxScoCid);
}
*/


///////////////////////////////////////////////////////////////////////////////
//
//  Frag_BuildFrag
//
//    This function divided a large frame into several small frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    Irp - Address of the IRP representing the IRP_MJ_WRITE call.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//    STATUS_PENDING, since we are putting the IRP on our internal queue.
//    STATUS_SUCCESS, we can completion this irp directly
//
//  IRQL:
//
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
// TODO: rewrite this routine
NTSTATUS Frag_BuildFrag(PBT_DEVICE_EXT devExt, struct sk_buff *skb)
{
	PBT_FRAG_T pFrag;
	PBT_FRAG_ELEMENT_T	pEle;
	PUINT8 pWriteBuf;
	UINT32 WriteLen;
	KIRQL 	oldIrql;
	PHCI_DATA_HEADER_T phead;
	PHCI_SCO_HEADER_T pscohead;
	PCONNECT_DEVICE_T pConnectDevice;
	PBT_LIST_ENTRY_T		pList;
	UINT8				ListFlag;
	UINT8 PacketType;
	PBT_HCI_T pHci;
	UINT16 reserved_len;
	UINT16 curpos;
	UINT8 fpid;
	UINT16 current_frag_threshold;
	UINT16 tmplen;
	LARGE_INTEGER timevalue;
	NTSTATUS status = STATUS_SUCCESS;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	UINT8 link_type, *plink_type;
    
	link_type = BT_LINK_TYPE_ACL;
	plink_type = &link_type;
    
	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_BuildFrag enter!\n");

	// Get the length of the frame
	WriteLen = skb->len;
	// Get the address of the frame
	pWriteBuf = skb->data;
	//
	// Add codes here...
	//
	// We must ignore the UART packet indicator, so we just forwards one bytes
	phead = (PHCI_DATA_HEADER_T)pWriteBuf;
	pHci = (PBT_HCI_T)devExt->pHci;
	pConnectDevice = Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, (UINT16)phead->connection_handle, plink_type);
	if (pConnectDevice == NULL)
	{
		pConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, (UINT16)phead->connection_handle, plink_type);
	}
	if (pConnectDevice == NULL)
	{
		return STATUS_RESOURCE_DATA_NOT_FOUND;
	}


	/*
	#ifdef BT_SNIFF_SUPPORT
		if (pConnectDevice->is_in_encryption_process || (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_HOLD) || pConnectDevice->role_switching_flag)
	#else
			if (pConnectDevice->is_in_encryption_process || (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE) || pConnectDevice->role_switching_flag)
	#endif
	{
		if (pConnectDevice->is_in_encryption_process)
		    {
				BT_DBGEXT(ZONE_FRG | LEVEL3, "in encryption process\n");
		    }
		if (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "mode current mode is not active, mode = %d\n", pConnectDevice->mode_current_mode);
		}
		if (pConnectDevice->role_switching_flag)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "role switching flag is set\n");
		}
		return STATUS_SECTION_PROTECTION;
	}
	*/
	
	/*Get pointer to the test */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if (*plink_type == BT_LINK_TYPE_ACL)
	{
		#ifdef BT_DISCARD_ACL_IN_SCO_CONNECTED
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->state == BT_DEVICE_STATE_CONNECTED)
				{
					BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_BuildFrag: Discard ACL frame in sco connected state!\n");
					return STATUS_RESOURCE_DATA_NOT_FOUND;
				}
			}
		#endif

		if (pConnectDevice->flush_flag)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL2, "Flush flag is set\n");
			if (phead->pb_flag != BT_LCH_TYPE_START_L2CAP)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL1, "This isn't the first pdu, discard it\n");
				return STATUS_RESOURCE_DATA_NOT_FOUND;
			}
			else
			{
				BT_DBGEXT(ZONE_FRG | LEVEL1, "This is the first pdu, so set flush flag as zero\n");
				pConnectDevice->flush_flag = 0;
			}
		}

		pList = (PBT_LIST_ENTRY_T)Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);

		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Free_FragList);
		pFrag->FreeFrag_Count--;
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		if(pEle == NULL)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_BuildFrag: Frag ACL element is exhaused, so return pending!\n");

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			pFrag->FreeFrag_Count++;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	
			
			return STATUS_RESOURCE_DATA_NOT_FOUND;
		}
		RtlCopyMemory(pEle->memory, pWriteBuf, phead->total_len + sizeof(HCI_DATA_HEADER_T));
		pEle->pConnectDevice = (ULONG_PTR)pConnectDevice;
		pEle->total_len = phead->total_len;
		pEle->ppid = 0;
		pEle->fcid = 0;
		pEle->fpid = 0;
		RtlZeroMemory(&pEle->CompletPkt, sizeof(COMPLETE_PACKETS_T));
		pEle->link_type =  *plink_type;
		if (pConnectDevice->is_in_disconnecting)
		{
			pEle->Valid = 0;
		}
		else
		{
			pEle->Valid = 1;
		}
		pEle->failure_times = 0;
		curpos = 0;
		reserved_len = (UINT16)pEle->total_len;

		if (pConnectDevice->edr_mode == 0)
		{
			ASSERT(pConnectDevice->current_packet_type <= BT_ACL_PACKET_DH5);
			ASSERT(PACKET_MAX_BYTES[pConnectDevice->current_packet_type] != 0);
		}
		else
		{
			ASSERT(pConnectDevice->current_packet_type <= BT_ACL_PACKET_3DH5);
			ASSERT(EDR_PACKET_MAX_BYTES[pConnectDevice->current_packet_type] != 0);
		}


		status = Frag_GetFragThreshold(devExt, pConnectDevice, &current_frag_threshold);
		if (status != STATUS_SUCCESS)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "Frag_buildFrag---get frag threshold failure\n");
			return STATUS_RESOURCE_DATA_NOT_FOUND;
		}
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Threshold %d, current frag type:%d\n", current_frag_threshold, pConnectDevice->current_packet_type);
		while (1)
		{
			fpid = pEle->fpid;
			if (reserved_len <= current_frag_threshold)
			{
				pEle->TxBlock[fpid].len = reserved_len;
				pEle->TxBlock[fpid].bufpos = curpos;
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Element[%d].len = %d, bufpos = %d\n", fpid, pEle->TxBlock[fpid].len, pEle->TxBlock[fpid].bufpos);
				pEle->fpid++;
				curpos += reserved_len;
				reserved_len = 0;
				break;
			}
			else
			{
				pEle->TxBlock[fpid].len = current_frag_threshold;
				pEle->TxBlock[fpid].bufpos = curpos;
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Element[%d].len = %d, bufpos = %d\n", fpid, pEle->TxBlock[fpid].len, pEle->TxBlock[fpid].bufpos);
				pEle->fpid++;
				curpos += current_frag_threshold;
				reserved_len -= current_frag_threshold;
			}
		}

		pEle->CompletPkt.number_of_handles = 1;
		pEle->CompletPkt.connection_handle[0] = (UINT16)phead->connection_handle;
		pEle->CompletPkt.num_of_complete_packets[0] = 1;
		
		//put it into the acl used list
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePutTail(pList, &pEle->link);
		/*	//Jakio20070110: mark it, unuseful		
		if (!pConnectDevice->l2cap_flow_control_flag)
		{
			if(pFrag->list_stop_flag[ListFlag]  != LIST_STOP_FLAG)
			{
				Frag_EnableTxQueue(devExt, ListFlag);
			}
			Frag_SetTimerType(devExt, ListFlag);
		}
		*/
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	}
	else
	{
		/* jakio20080612: judge whether there is a null pointer, otherwise, it is dangerous in multi-processor system
*/
		pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice;
		if(pScoConnectDevice ==  NULL)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "Frag_BuildFrag---Error, sco device not exist\n");
            /* Should we return ?? */
            return STATUS_RESOURCE_DATA_NOT_FOUND;
		}
		else
		{
			#ifdef BT_SEND_MULTISCO_ONCE
				current_frag_threshold = BT_FRAG_SCO_THRESHOLD;
                //BT_DBGEXT(ZONE_FRG | LEVEL3, "SCO Threadhold 240\n");
			#else
				current_frag_threshold = PACKET_MAX_BYTES[pScoConnectDevice->current_packet_type];
			#endif
		}

		pscohead = (PHCI_SCO_HEADER_T)pWriteBuf;

        // Find one buffer
        if(0 == pScoConnectDevice->lenTx240 || NULL == pScoConnectDevice->pTx240Buf)
        {
    		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
    		pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Free_FragList);
    		pFrag->FreeFrag_Count--;
    		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

    		if(pEle == NULL)
    		{
    			BT_DBGEXT(ZONE_FRG | LEVEL0, "Frag SCO element is exhaused, return pending!\n");
    			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
    			pFrag->FreeFrag_Count++;
    			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

    			return STATUS_TRANSLATION_COMPLETE;
    		}
            // The Tx240 buffer point to memory of element
            pScoConnectDevice->pTx240Buf = pEle->memory;
            pScoConnectDevice->pEle = pEle;
        }


        //BT_DBGEXT(ZONE_FRG | LEVEL3, "SCO len %d\n", pscohead->total_len);
        /* Assemble the data to 240 bytes */
        if(pScoConnectDevice && 
                pScoConnectDevice->lenTx240 < BT_FRAG_SCO_THRESHOLD){
            UINT16 tmpLen = pscohead->total_len;

            /* Make sure the buffer not overloaded */
            tmpLen = (tmpLen + pScoConnectDevice->lenTx240) > BT_FRAG_SCO_THRESHOLD ?
                (BT_FRAG_SCO_THRESHOLD - pScoConnectDevice->lenTx240) :
                tmpLen;
            
            RtlCopyMemory(pScoConnectDevice->pTx240Buf + pScoConnectDevice->lenTx240, 
                            pWriteBuf + sizeof(HCI_SCO_HEADER_T), tmpLen);

            pScoConnectDevice->lenTx240 += tmpLen;
        }

        if(pScoConnectDevice->lenTx240 < BT_FRAG_SCO_THRESHOLD){
            /* Just wait for more data */
            //BT_DBGEXT(ZONE_FRG | LEVEL3, "@@ Not enough 240 bytes %d @@\n", pScoConnectDevice->len_tx_240);
            return STATUS_TRANSLATION_COMPLETE;
        }
        BT_DBGEXT(ZONE_FRG | LEVEL3, "@@ Enough 240 bytes %d @@, header size %d\n", pScoConnectDevice->lenTx240, sizeof(HCI_SCO_HEADER_T));

        // Restore the element pointer
		pEle = pScoConnectDevice->pEle;
		pEle->pConnectDevice = (ULONG_PTR)pConnectDevice;
		if ((pScoConnectDevice->is_in_disconnecting))
		{
			pEle->Valid = 0;
		}
		else
		{
			pEle->Valid = 1;
		}
        
        //pscohead->total_len = BT_FRAG_SCO_THRESHOLD;
        //Jakio20071217:save sco head
		RtlCopyMemory(pEle->scoHeader, pscohead, sizeof(HCI_SCO_HEADER_T));
        ((PHCI_SCO_HEADER_T)pEle->scoHeader)->total_len = BT_FRAG_SCO_THRESHOLD;
        
        pEle->total_len = BT_FRAG_SCO_THRESHOLD;
        //RtlCopyMemory(pEle->memory, pScoConnectDevice->pTx240Buf, BT_FRAG_SCO_THRESHOLD);
        pScoConnectDevice->lenTx240 = 0;
        pScoConnectDevice->pTx240Buf = NULL;
        // Balance SCO TX and RX
        pScoConnectDevice->txRxBalance++;

        
		BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_BuildFrag for SCO: total length = %d\n", pEle->total_len);
		pEle->ppid = 0;
		pEle->fcid = 0;
		pEle->fpid = 0;
		pEle->link_type = link_type;
		pEle->failure_times = 0;
		RtlZeroMemory(&pEle->CompletPkt, sizeof(COMPLETE_PACKETS_T));
		
		curpos = 0;
		reserved_len = (UINT16)pEle->total_len;

		
		while (1)
		{
			fpid = pEle->fpid;
			if (reserved_len <= current_frag_threshold)
			{
				pEle->TxBlock[fpid].len = reserved_len;
				pEle->TxBlock[fpid].bufpos = curpos;
				BT_DBGEXT(ZONE_FRG | LEVEL3, "(SCO)Element[%d].len = %d, bufpos = %d\n", fpid, pEle->TxBlock[fpid].len, pEle->TxBlock[fpid].bufpos);
				pEle->fpid++;
				curpos += reserved_len;
				reserved_len = 0;
				break;
			}
			else
			{
				pEle->TxBlock[fpid].len = current_frag_threshold;
				pEle->TxBlock[fpid].bufpos = curpos;
				BT_DBGEXT(ZONE_FRG | LEVEL3, "(SCO)Element[%d].len = %d, bufpos = %d\n", fpid, pEle->TxBlock[fpid].len, pEle->TxBlock[fpid].bufpos);
				pEle->fpid++;
				curpos += current_frag_threshold;
				reserved_len -= current_frag_threshold;
			}
		}

		pEle->CompletPkt.number_of_handles = 1;
		pEle->CompletPkt.connection_handle[0] = (UINT16)pscohead->connection_handle;
		pEle->CompletPkt.num_of_complete_packets[0] = 1;
			
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePutTail(&pFrag->Sco_FragList, &pEle->link);
		/*	//Jakio20070110: mark it, unuseful
		if (!pConnectDevice->l2cap_flow_control_flag)
		{
			if(pFrag->list_stop_flag[FRAG_SCO_LIST]  != LIST_STOP_FLAG)
			{
				Frag_EnableTxQueue(devExt, FRAG_SCO_LIST);
			}
			Frag_SetTimerType(devExt, FRAG_SCO_LIST);
		}
		*/
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_BuildFrag for SCO: fpid = %d\n", pEle->fpid);
	}
	//if (!pConnectDevice->l2cap_flow_control_flag)
	{
		timevalue.QuadPart = (kOneMillisec); // Just raise the IRQL and process the task sdft in another DPC process.
		KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
	}

	BT_DBGEXT(ZONE_FRG | LEVEL3, "build frag end\n");
	return STATUS_TRANSLATION_COMPLETE;
}



PBT_LIST_ENTRY_T Frag_GetQueueHead(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pListFlag)
{
	ASSERT(pConnectDevice != NULL);
	ASSERT(devExt != NULL);
	ASSERT(devExt->pFrag != NULL);
	
	if (pConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_GetQueueHead: default master queue, as pConnectDevice = 0x%x\n", pConnectDevice);
		*pListFlag = FRAG_MASTER_LIST;
		return &((PBT_FRAG_T)devExt->pFrag)->Master_FragList;
	}
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_GetQueueHead: slave queue, pConnectDevice = 0x%x\n", pConnectDevice);
			*pListFlag = FRAG_SLAVE_LIST;
			return &((PBT_FRAG_T)devExt->pFrag)->Slave_FragList;
		}
		else
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_GetQueueHead: slave1 queue, pConnectDevice = 0x%x\n", pConnectDevice);
			*pListFlag = FRAG_SLAVE1_LIST;
			return &((PBT_FRAG_T)devExt->pFrag)->Slave1_FragList;
		}
	}
	else if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_GetQueueHead: sniff queue, pConnectDevice = 0x%x\n", pConnectDevice);
		*pListFlag = FRAG_SNIFF_LIST;
		return &((PBT_FRAG_T)devExt->pFrag)->Sniff_FragList;
	}
	else
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_GetQueueHead: master queue, pConnectDevice = 0x%x\n", pConnectDevice);
		*pListFlag = FRAG_MASTER_LIST;
		return &((PBT_FRAG_T)devExt->pFrag)->Master_FragList;
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//  Frag_ProcessThisIRP
//
//    This function process the tx frame that saved in this IRP.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    Irp - Address of the IRP representing the IRP_MJ_WRITE call.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//    STATUS_PENDING, since we are putting the IRP on our internal queue.
//    STATUS_SUCCESS, we can completion this irp directly
//
//  IRQL:
//
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
void thread_processing_cmd(struct work_struct *pwork)
#else
VOID thread_processing_cmd(ULONG_PTR pdevExt)
#endif
{
	PBT_DEVICE_EXT devExt;
    struct sk_buff *skb;
    struct sk_buff_head *q;
    int ret, to;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)      
    struct delayed_work *dw;      
    /* Parse the structure */      
    dw = container_of(pwork, struct delayed_work, work);  
	devExt = (PBT_DEVICE_EXT)container_of(dw, BT_DEVICE_EXT, cmd_work);
#else	
	devExt = (PBT_DEVICE_EXT)pdevExt;
#endif
    ENTER_FUNC();	

	if(devExt->InD3Flag){
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "In D3\n");
		return;
	}

    q = &devExt->cmd_queue;
	/* process command in the queue */
        while(skb = skb_dequeue(q)){
		/* Convert the command buffer from SKB */
            BtProcessTxCommand(devExt, skb->data, skb->len);

		/* 
		* After processing, free the SKB.
             * Attention!!!!!!
             * The command handler should not use the SKB any more after processing
		*/
            kfree_skb(skb);
	}
    return;
}



VOID Frag_EnableTxQueue(PBT_DEVICE_EXT devExt, UINT8 ListIndex)
{
	PBT_FRAG_T pFrag;
	KIRQL	oldIrql;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);
	
	switch(ListIndex)
	{
	case FRAG_MASTER_LIST:
		pFrag->AllowSendingFlag |= MASTER_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SLAVE_LIST:
		pFrag->AllowSendingFlag |= SLAVE_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SLAVE1_LIST:
		pFrag->AllowSendingFlag |= SLAVE1_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SNIFF_LIST:
		pFrag->AllowSendingFlag |= SNIFF_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SCO_LIST:
		pFrag->AllowSendingFlag |= SCO_LIST_ALLOW_SENDING_BIT;
		break;
	default:
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Error: undefined list index:%d\n", ListIndex);
	}
	
}


VOID Frag_DisableTxQueue(PBT_DEVICE_EXT devExt, UINT8 ListIndex)
{
	PBT_FRAG_T pFrag;
	KIRQL	oldIrql;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);
	
	switch(ListIndex)
	{
	case FRAG_MASTER_LIST:
		pFrag->AllowSendingFlag &= ~MASTER_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SLAVE_LIST:
		pFrag->AllowSendingFlag &= ~SLAVE_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SLAVE1_LIST:
		pFrag->AllowSendingFlag &= ~SLAVE1_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SNIFF_LIST:
		pFrag->AllowSendingFlag &= ~SNIFF_LIST_ALLOW_SENDING_BIT;
		break;
	case FRAG_SCO_LIST:
		pFrag->AllowSendingFlag &= ~SCO_LIST_ALLOW_SENDING_BIT;
		break;
	default:
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Error: undefined list index:%d\n", ListIndex);
	}
	
}





/*
void Frag_MoveFrag(PBT_DEVICE_EXT devExt, PBT_LIST_ENTRY_T pDestList, PBT_LIST_ENTRY_T pSourceList)
{
	UINT8 i;
	KIRQL oldIrql;
	PBT_FRAG_ELEMENT_T 	pEle;
	PBT_FRAG_T			pFrag;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);

	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_MoveFrag  entered\n");

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pSourceList);
	while (pEle!= NULL)
	{
		
		QueuePushHead(pDestList, &pEle->link);
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_MoveFrag: move one fragment! \n");
		
		pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pSourceList);
	}
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	return;
}
*/

#if 1
UINT32 Frag_ProcessAclFrag(PBT_DEVICE_EXT devExt, UINT8 ListIndex)
{
	PBT_FRAG_T pFrag;
	PUINT8 pWriteBuf;
	PHCI_DATA_HEADER_T phead;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PPAYLOAD_HEADER_MULTI_SLOT_T pMultiHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	PCOMPLETE_PACKETS_T	pCompletPkt;
	PBT_FRAG_ELEMENT_T	pEle;
	PBT_LIST_ENTRY_T		pList;
	PBT_HCI_T	pHci;
	KIRQL	oldIrql;
	KIRQL	CancelIrql;
	UINT8 PacketType;
	PUINT8 dest,dest1;
	PUINT8 source;
	UINT8 ppid;
	BOOLEAN		Find = FALSE;
	UINT8 i;
	UINT16	IdleSpace;
	UINT32 Length = 0;
	UINT8 dataType = 0;
	LARGE_INTEGER  timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 ppidinc = 0;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
		return BT_FRAG_RESOURCE_ERROR;

	
	switch(ListIndex)
	{
	case FRAG_MASTER_LIST:
		pList = &pFrag->Master_FragList;
		dataType = MAILBOX_DATA_TYPE_MASTER;
		break;
	case FRAG_SLAVE_LIST:
		pList = &pFrag->Slave_FragList;
		dataType = MAILBOX_DATA_TYPE_SLAVE;
		break;
	case FRAG_SLAVE1_LIST:
		pList = &pFrag->Slave1_FragList;
		dataType = MAILBOX_DATA_TYPE_SLAVE1;
		break;
	case FRAG_SNIFF_LIST:
		pList = &pFrag->Sniff_FragList;
		dataType = MAILBOX_DATA_TYPE_SNIFF;
		break;

	default:
		return BT_FRAG_RESOURCE_ERROR;
	}


	if (pFrag->list_stop_flag[ListIndex] == LIST_STOP_FLAG)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessAclFrag---list_stop_flag is set\n");
		return BT_FRAG_LIST_NULL;
	}


	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
	while(pEle)
	{
		pTempConnectDevice = (PCONNECT_DEVICE_T)pEle->pConnectDevice;

		if(pTempConnectDevice->edr_change_flag == 1)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessFrag exit: edr_change_flag is set\n");
		}
		else if(pTempConnectDevice->timer_l2cap_flow_valid == 1)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessFrag exit: l2cap_flow_control_flag is set:%8x\n", pTempConnectDevice);
		}
		else if(pTempConnectDevice->is_in_encryption_process)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "in encryption process\n");
		}
		else if(pTempConnectDevice->role_switching_flag)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "role switching flag is set\n");
		}
	#ifdef BT_SNIFF_SUPPORT
		else if(pTempConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_HOLD)
	#else
		else if(pTempConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
	#endif
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "mode current mode is not active, mode = %d\n", pTempConnectDevice->mode_current_mode);
		}
		else
		{
			Find = TRUE;
		}

		if(Find == TRUE)
		{
			break;
		}
		else if(ListIndex == FRAG_MASTER_LIST)
		{
			pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);
		}
		else
		{
			break;
		}
	}
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);


	if(Find == FALSE)
	{
		if(pEle)
			return BT_FRAG_RESOURCE_ERROR;
		else
			return BT_FRAG_LIST_NULL;
	}
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	QueueRemoveEle(pList, &pEle->link);
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);



	#if 0
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(pList);
	if (pEle == NULL)
	{
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);		
		return BT_FRAG_LIST_NULL;
	}
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	pTempConnectDevice = (PCONNECT_DEVICE_T)pEle->pConnectDevice;
	if (pTempConnectDevice->edr_change_flag == 1)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessFrag exit: edr_change_flag is set\n");
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePushHead(pList, &pEle->link);
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	
		return BT_FRAG_RESOURCE_ERROR;
	}
	if(pTempConnectDevice->timer_l2cap_flow_valid == 1)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessFrag exit: l2cap_flow_control_flag is set\n");
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePushHead(pList, &pEle->link);
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	
		return BT_FRAG_RESOURCE_ERROR;
	}
	/* jakio20080111: more safe check
*/
	#ifdef BT_SNIFF_SUPPORT
		if (pTempConnectDevice->is_in_encryption_process || (pTempConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_HOLD) || pTempConnectDevice->role_switching_flag)
	#else
		if (pTempConnectDevice->is_in_encryption_process || (pTempConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE) || pTempConnectDevice->role_switching_flag)
	#endif
	{
		if (pTempConnectDevice->is_in_encryption_process)
		    {
				BT_DBGEXT(ZONE_FRG | LEVEL3, "in encryption process\n");
		    }
		if (pTempConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "mode current mode is not active, mode = %d\n", pTempConnectDevice->mode_current_mode);
		}
		if (pTempConnectDevice->role_switching_flag)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "role switching flag is set\n");
		}
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePushHead(pList, &pEle->link);
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	
		return BT_FRAG_RESOURCE_ERROR;;
	}
	#endif


	
	pWriteBuf = pEle->memory;
	phead = (PHCI_DATA_HEADER_T)pWriteBuf;
	source = pWriteBuf + sizeof(HCI_DATA_HEADER_T);

	pCompletPkt = &pEle->CompletPkt;
	

	if(pEle->ppid < pEle->fpid)
	{
		
		dest = BtGetValidTxPool(devExt);
		if (dest == NULL)
		{
			KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
			QueuePushHead(pList, &pEle->link);
			KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Not enough tx pool!!!!\n");
			return BT_FRAG_RESOURCE_ERROR;//STATUS_PENDING;
		}
		ppid = pEle->ppid;
		
		dest1 = dest;
		for(i = 0; ppid < pEle->fpid && i < 4; i++, ppid++)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessFrag: current ppid = %d\n", ppid);

			/* Determine packet type by payload length */
			Frag_GetFragPacketType(devExt, pTempConnectDevice, pEle->TxBlock[ppid].len, &PacketType);


			pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest1;
			if (pTempConnectDevice != NULL)
			{
				pHCBBHead->am_addr = pTempConnectDevice->am_addr;
			}
			else
			{
				pHCBBHead->am_addr = 0;
			}
			pHCBBHead->type = PacketType;
			pHCBBHead->tx_retry_count = 0;
			pHCBBHead->slave_index = pTempConnectDevice->slave_index;
			pHCBBHead->master_slave_flag = (pTempConnectDevice->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;
			if (pTempConnectDevice != NULL)
			{
				pTempConnectDevice->tx_count_frame++;
			}

			if ((PacketType == BT_ACL_PACKET_DM1) || (PacketType == BT_ACL_PACKET_AUX1) || (PacketType == BT_ACL_PACKET_DH1 && pTempConnectDevice != NULL && pTempConnectDevice->edr_mode == 0))
			{
				pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(dest1 + sizeof(HCBB_PAYLOAD_HEADER_T));
				if (ppid == 0)
				{
					pSingHead->l_ch = (UINT8)phead->pb_flag;
				}
				else
				{
					pSingHead->l_ch = BT_LCH_TYPE_CON_L2CAP;
				}
				pSingHead->flow = 1;
				pSingHead->length = (UINT8)(pEle->TxBlock[ppid].len);
				pHCBBHead->length = pEle->TxBlock[ppid].len + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
			RtlCopyMemory(dest1 + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), source + pEle->TxBlock[ppid].bufpos, pEle->TxBlock[ppid].len);
			}
			else
			{
				pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)(dest1 + sizeof(HCBB_PAYLOAD_HEADER_T));
				if (ppid == 0)
				{
					pMultiHead->l_ch = (UINT16)phead->pb_flag;
				}
				else
				{
					pMultiHead->l_ch = BT_LCH_TYPE_CON_L2CAP;
				}
				pMultiHead->flow = 1;
				pMultiHead->undefined = 0;
				pMultiHead->length = (UINT16)(pEle->TxBlock[ppid].len);
				pHCBBHead->length = pEle->TxBlock[ppid].len + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
			RtlCopyMemory(dest1 + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), source + pEle->TxBlock[ppid].bufpos, pEle->TxBlock[ppid].len);
			}
			dest1 += ALIGNLONGDATALEN(pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T));
			
			/* jakio20071102: mark here, we only support 2dh1 now
*/
			ppidinc++;	
		
			BT_DBGEXT(ZONE_FRG | LEVEL3, "payload header length: %x\n", pHCBBHead->length);
			Length += pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
			Length = ALIGNLONGDATALEN(Length);
			if((Length + ALIGNLONGDATALEN(pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T))) > MAX_TX_POOL_COUNT *(BT_MAX_FRAME_SIZE + BT_MAX_SCO_FRAME_SIZE))
				break;
		}
		
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		IdleSpace = (UINT16)pFrag->IdleSpace[ListIndex];
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

		if(IdleSpace <= Length)
		{
			pEle->failure_times++;
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Space not enough\n");
			ReleaseBulkOut1Buf(dest, devExt);
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePushHead(pList, &pEle->link);
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	

			Frag_TxTimerInstant(devExt, &timevalue);
			KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);		
				
			return BT_FRAG_RESOURCE_ERROR;
		}
		pEle->failure_times = 0;
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pFrag->IdleSpace[ListIndex] -= Length;	
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	


		/* jakio20070115: for test, 
*/
		status = BtUsbWriteAll(devExt, dest, Length, dataType, 0);
		if(NT_SUCCESS(status))
		{
			#ifdef BT_3DSP_FLUSH_SUPPORT
			Flush_BakupData(devExt, dest, Length, ListIndex);
			#endif
		
			#ifdef BT_AUTO_SEL_PACKET
			{
				if (devExt->TxAclSendCount == 0)
				{
					devExt->StartRetryCount = devExt->TxRetryCount;
				}
				if (++devExt->TxAclSendCount >= 100)
				{
					devExt->EndRetryCount = devExt->TxRetryCount;

					BT_DBGEXT(ZONE_FRG | LEVEL3, "StartRetryCount = %u, EndRetryCount = %u!\n",devExt->StartRetryCount , devExt->EndRetryCount);

					if (pTempConnectDevice != NULL)
					{
						if (devExt->EndRetryCount >= devExt->StartRetryCount)
						{
							Frag_AutoSelAclPacketType(devExt, pTempConnectDevice, devExt->EndRetryCount - devExt->StartRetryCount);
						}
						else 
						#ifdef BT_USE_NEW_ARCHITECTURE
							{
								Frag_AutoSelAclPacketType(devExt, pTempConnectDevice, devExt->EndRetryCount + 16384-devExt->StartRetryCount);
							}
						#else
							Frag_AutoSelAclPacketType(devExt, pTempConnectDevice, devExt->EndRetryCount + 65536-devExt->StartRetryCount);
						#endif
					}
					devExt->TxAclSendCount = 0;
				}
			}
			#endif

			pEle->ppid += ppidinc;	
			
			
		}
		else
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "acl flag sending failure\n");

			ReleaseBulkOut1Buf(dest, devExt);

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePushHead(pList, &pEle->link);
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	

			return BT_FRAG_RESOURCE_ERROR;
		}

		if (pEle->ppid == pEle->fpid)
		{	
			if (pCompletPkt->number_of_handles > 0)
			{
				Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(devExt, (PUINT8)pCompletPkt, 1);
			}
			pEle->CurrentIrp = NULL;

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePutTail(&pFrag->Free_FragList, &pEle->link);
			pFrag->FreeFrag_Count++;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
			
		}
		else	//If not, get next one
		{
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePushHead(pList, &pEle->link);
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		}
		
	}
	
	return BT_FRAG_SUCCESS;
}
#endif

/**************************************************************************
 *   Frag_InitTxForConnDev
 *
 *   Descriptions:
 *      Cancel the sending BD and pending IRP assocaited with the connect device.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *   Changelog:
 *	Jakio20080110: modify this function for usb architecture
 *************************************************************************/
VOID Frag_InitTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
		KIRQL oldIrql;
		PBT_HCI_T pHci;
		PBT_FRAG_T pFrag;
		PBT_LIST_ENTRY_T		pList = NULL;
		PBT_FRAG_ELEMENT_T	pEle = NULL;
		PBT_FRAG_ELEMENT_T	pTmpEle = NULL;
		COMPLETE_PACKETS_T CompletePackets;
		PHCI_DATA_HEADER_T phead;
		UINT32 frageleno;
		UINT32 tmpLong;
		UINT8 i;
		UINT8 ListIndex;
		UINT32 count;
		
		BOOLEAN exitflag = FALSE;
		BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_InitTxForConnDev: entered\n");
		RtlZeroMemory(&CompletePackets, sizeof(COMPLETE_PACKETS_T));
		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		if(pFrag == NULL)
			return;

		
		pHci = (PBT_HCI_T)devExt->pHci;
		if(pHci == NULL)
			return;
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pConnectDevice != NULL)
		{
			pConnectDevice->is_in_disconnecting = 1;
			if (pConnectDevice->role_switching_flag)
			{
				Hci_StopProtectHciCommandTimer(pHci);
				pConnectDevice->status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
				pConnectDevice->role_switching_flag = 0;
				pHci->role_switching_flag = 0;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				if (pConnectDevice->role_switching_role == BT_ROLE_MASTER)
				{
					if (pConnectDevice->am_addr_for_fhs != 0)
					{
						Hci_Free_Am_address(pHci, pConnectDevice->am_addr_for_fhs);
					}
				}
				Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pConnectDevice, 1);
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
			pConnectDevice->l2cap_flow_control_flag = 0;
			pConnectDevice->timer_l2cap_flow_valid = 0;
			pConnectDevice->timer_l2cap_flow_counter = 0;
			pConnectDevice->l2cap_rx_flow_control_flag = 0;
			pConnectDevice->valid_flag = 0;

			#ifdef BT_TESTMODE_SUPPORT
			TestMode_ResetMembers(devExt, pConnectDevice);
			#endif
		}
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		
		LMP_Clear_LmpPdu(devExt, pConnectDevice);
		
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pList = (PBT_LIST_ENTRY_T)Frag_GetQueueHead(devExt, pConnectDevice, &ListIndex);
		pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
		while(pEle != NULL)
		{
			if(pEle->pConnectDevice == (ULONG_PTR)pConnectDevice)
			{
				pTmpEle = pEle;
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);

				if(ListIndex != FRAG_SCO_LIST)
				{
					phead = (PHCI_DATA_HEADER_T)pTmpEle->memory;
					CompletePackets.number_of_handles++;
					CompletePackets.connection_handle[CompletePackets.number_of_handles] = (UINT16)phead->connection_handle;
					CompletePackets.num_of_complete_packets[CompletePackets.number_of_handles] = 1;	
				}
				QueueRemoveEle(pList, &pTmpEle->link);
				QueuePutTail(&pFrag->Free_FragList, &pTmpEle->link);
			}
			else
			{
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);
			}
		}
		//Jakio20090408: check sniff queue
		if(((PBT_LMP_T)devExt->pLmp)->sniff_number == 0)
		{
			pList = &pFrag->Sniff_FragList;

			pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
			while(pEle != NULL)
			{
				//get next element
				pTmpEle = pEle;
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);

				//remove from the used list and put it into freelist
				QueueRemoveEle(pList, &pTmpEle->link);
				QueuePutTail(&pFrag->Free_FragList, &pTmpEle->link);
			}
		}
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

		if(((PBT_LMP_T)devExt->pLmp)->sniff_number == 0)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_InitTxForConnDev---write exit sniff cmd any way\n");
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT);
		}

		#if 1	//Jakio20080129: clear scratch buffer in hci_write_command_indicatro()
		if(ListIndex == FRAG_SLAVE_LIST)
		{
			/*
			UsbReadFrom3DspReg(devExt, 0x2128, &tmpLong);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "slave ppter: %x\n", tmpLong);
			UsbReadFrom3DspReg(devExt, 0x212c, &tmpLong);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "slave cpter: %x\n", tmpLong);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_InitTxForConnDev: ready flush slave buffer\n");
			*/

			BtDelay(5000);
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_InitTxForConnDev--no slave device, clear scratch buffer\n");
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE);

			/*
			UsbReadFrom3DspReg(devExt, 0x2128, &tmpLong);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "slave ppter: %x\n", tmpLong);
			UsbReadFrom3DspReg(devExt, 0x212c, &tmpLong);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "slave cpter: %x\n", tmpLong);
			*/
		}
		else if(ListIndex == FRAG_SNIFF_LIST)
		{
			BtDelay(5000);
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF);
		}
		else if(ListIndex == FRAG_MASTER_LIST)
		{
			/*this is the only connect master device*/
			if(pHci->num_device_am == 1)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_InitTxForConnDev--No connect device, clear scratch buffer\n");
				BtDelay(5000);
				VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_MASTER);
			}
	
		}
		else if(ListIndex == FRAG_SCO_LIST)
		{
			BtDelay(5000);
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SCO);
		}
		#endif


		
		if (CompletePackets.number_of_handles > 0)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_InitTxForConnDev---send num of complete packets event\n");
		}
	#if 0
		KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
		UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x3);
		if (pConnectDevice != NULL)
		{
			pConnectDevice->is_in_disconnecting = 1;
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->is_in_disconnecting = 1;
			}
		}
		while (firstbdid != endbdid)
		{
			tmpConnectDevice = devExt->TxBd[firstbdid].pConnectDevice;
			if (tmpConnectDevice == (UINT32)pConnectDevice)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Find one BD is not processed yet after its connection is closed:(id = %d) \n", firstbdid);
				devExt->TxBd[firstbdid].Valid = 0;
				BT_USB_WRITE_DWORDS(BtGetBDAddrById(firstbdid), (PUINT32) &devExt->TxBd[firstbdid], EACH_BD_LENGTH >> 2);
			}
			firstbdid = BtGetNextBDId(firstbdid);
		}
		firstfragid = pFrag->ele_TxCid;
		endfragid = pFrag->ele_TxPid;
		while (firstfragid != endfragid)
		{
			frageleno = firstfragid;
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Rerange Frag, frageleno = %d\n", firstfragid);
			if (pFrag->TxFragElement[frageleno].pConnectDevice == (UINT32)pConnectDevice)
			{
				pFrag->TxFragElement[frageleno].Valid = 0;
				if (pFrag->TxFragElement[frageleno].send_complete_flag == 0)
				{
					pFrag->TxFragElement[frageleno].send_complete_flag = 1;
					for (i = 0; i < CompletePackets.number_of_handles; i++)
					{
						if (CompletePackets.connection_handle[i] == pConnectDevice->connection_handle)
						{
							CompletePackets.num_of_complete_packets[i]++;
							break;
						}
					}
					if ((i == CompletePackets.number_of_handles) && (i < BT_TOTAL_NUM_DATA_PACKET))
					{
						CompletePackets.number_of_handles++;
						CompletePackets.connection_handle[i] = pConnectDevice->connection_handle;
						CompletePackets.num_of_complete_packets[i] = 1;
					}
				}
			}
			firstfragid = Frag_GetNextEleId(firstfragid);
		}
		while (1)
		{
			firstbdid = devExt->TxCid;
			endbdid = devExt->TxPid;
			while (firstbdid != endbdid)
			{
				if (devExt->TxBd[firstbdid].Valid)
				{
					exitflag = TRUE;
					break;
				}
				BtWriteToReg(devExt, BT_DSP_C_PTR_REG, BtGetBDAddrById(BtGetNextBDId(firstbdid)));
				firstbdid = BtGetNextBDId(firstbdid);
			}
			if (exitflag)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Exist some BD or element whose valid is 1\n");
				break;
			}
			KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
			Frag_ProcessTxInt(devExt);
			KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
			if (BtIsBDEmpty(devExt) && Frag_IsEleEmpty(pFrag))
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "All is processed, just exit\n");
				break;
			}
		}
		firstbdid = devExt->TxScoCid;
		endbdid = devExt->TxScoPid;
		while (firstbdid != endbdid)
		{
			tmpConnectDevice = devExt->TxScoBd[firstbdid].pConnectDevice;
			if (tmpConnectDevice == (UINT32)pConnectDevice)
			{
				devExt->TxScoBd[firstbdid].Valid = 0;
				BT_USB_WRITE_DWORDS(BtGetScoBDAddrById(firstbdid), (PUINT32) &devExt->TxScoBd[firstbdid], EACH_BD_LENGTH >> 2);
			}
			firstbdid = BtGetNextBDId(firstbdid);
		}
		firstfragid = pFrag->ele_TxScoCid;
		endfragid = pFrag->ele_TxScoPid;
		while (firstfragid != endfragid)
		{
			frageleno = firstfragid;
			if (pFrag->TxScoFragElement[frageleno].pConnectDevice == (UINT32)pConnectDevice)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "(SCO)Frag_InitTxForConnDev: the Completed IRP is not this IRP, irp = 0x%x\n", pFrag->TxScoFragElement[frageleno].CurrentIrp);
				pFrag->TxScoFragElement[frageleno].Valid = 0;
				if (((PBT_HCI_T)devExt->pHci)->sco_flow_control_enable)
				{
					if (pFrag->TxScoFragElement[frageleno].send_complete_flag == 0)
					{
						pFrag->TxScoFragElement[frageleno].send_complete_flag = 1;
						for (i = 0; i < CompletePackets.number_of_handles; i++)
						{
							ASSERT(pConnectDevice->pScoConnectDevice != NULL);
							if (CompletePackets.connection_handle[i] == ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle)
							{
								CompletePackets.num_of_complete_packets[i]++;
								break;
							}
						}
						if ((i == CompletePackets.number_of_handles) && (i < BT_TOTAL_NUM_DATA_PACKET))
						{
							CompletePackets.number_of_handles++;
							ASSERT(pConnectDevice->pScoConnectDevice != NULL);
							CompletePackets.connection_handle[i] = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle;
							CompletePackets.num_of_complete_packets[i] = 1;
						}
					}
				}
			}
			firstfragid = Frag_GetNextEleId(firstfragid);
		}
		exitflag = FALSE;
		while (1)
		{
			firstbdid = devExt->TxScoCid;
			endbdid = devExt->TxScoPid;
			while (firstbdid != endbdid)
			{
				if (devExt->TxScoBd[firstbdid].Valid)
				{
					exitflag = TRUE;
					break;
				}
				BtWriteToReg(devExt, BT_DSP_C_SCO_PTR_REG, BtGetScoBDAddrById(BtGetNextBDId(firstbdid)));
				firstbdid = BtGetNextBDId(firstbdid);
			}
			if (exitflag)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Exist some SCO BD or element whose valid is 1\n");
				break;
			}
			KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
			KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
			if (BtIsScoBDEmpty(devExt) && Frag_IsScoEleEmpty(pFrag))
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "All SCO is processed, just exit\n");
				break;
			}
		}
		UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0);
		KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
		if (CompletePackets.number_of_handles > 0)
		{
			Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(devExt, (PUINT8) &CompletePackets);
		}
	#endif
}
/**************************************************************************
 *   Frag_FlushTxForConnDev
 *
 *   Descriptions:
 *      Cancel the sending BD and pending IRP assocaited with the connect device.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Frag_FlushTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
}
///////////////////////////////////////////////////////////////////////////////
//
//  Frag_ProcessScoFrag
//
//    This function sends the frame saved into the fragment element.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    Irp - Address of the IRP representing the IRP_MJ_WRITE call.
//          or NULL means that driver should not determine the IRP.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//    STATUS_PENDING, since we are putting the IRP on our internal queue.
//    STATUS_SUCCESS, we can completion this irp directly
//
//  IRQL:
//
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
UINT32 Frag_ProcessScoFrag(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T pFrag;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PPAYLOAD_HEADER_MULTI_SLOT_T pMultiHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	PCOMPLETE_PACKETS_T	pCompletPkt;
	PHCI_SCO_HEADER_T pscohead;
	PBT_FRAG_ELEMENT_T	pEle;
	KIRQL	oldIrql;
	KIRQL	CancelIrql;
	UINT8 PacketType;
	PUINT8 dest;
	PUINT8 source;
	UINT32 frageleno;
	UINT8 ppid;
	UINT32 Length = 0;
	UINT8 dataType = 0;
	UINT32	IdleSpace; 
	UINT8 i;
	INT16 input;
	UINT8 air_mode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PSCO_CONNECT_DEVICE_T pTempScoConnectDevice;
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER  timevalue;
	
	/*Get pointer to the test */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);

	if (pFrag->list_stop_flag[FRAG_SCO_LIST] == LIST_STOP_FLAG)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessScoFrag---list_stop_flag is set\n");
		return BT_FRAG_RESOURCE_ERROR;
	}


	if(QueueEmpty(&pFrag->Sco_FragList))
	{
		Frag_CheckAndAddScoNullFrame(devExt);
	}
	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Sco_FragList);
	if (pEle == NULL)
	{
		//BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessScoFrag exit for element empty\n");
		//Frag_DisableTxQueue(devExt, FRAG_SCO_LIST);
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		return BT_FRAG_LIST_NULL;
	}
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	pTempConnectDevice = (PCONNECT_DEVICE_T)pEle->pConnectDevice;

	if(pTempConnectDevice->valid_flag == 0)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL1, "connect device is invalid,drop the frag\n");
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePutTail(&pFrag->Free_FragList, &pEle->link);
		pFrag->FreeFrag_Count++;
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
		return BT_FRAG_RESOURCE_ERROR;
	}

	source = pEle->memory;
	pscohead  = (PHCI_SCO_HEADER_T)pEle->scoHeader;
	dataType = MAILBOX_DATA_TYPE_SCO;



	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = pFrag->IdleSpace[FRAG_SCO_LIST];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

	/*
	if(IdleSpace < 240)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Sco buffer not enough\n");

		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		QueuePutTail(&pFrag->Free_FragList, &pEle->link);
		pFrag->FreeFrag_Count++;
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
		return BT_FRAG_RESOURCE_ERROR;
	}
	*/

	if(pEle->ppid < pEle->fpid)
	{
		
		
		
		ppid = pEle->ppid;
		BT_DBGEXT(ZONE_FRG | LEVEL1, "current ppid = %d\n", ppid);

		if (pTempConnectDevice != NULL)
		{
			pTempScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
		}
		else
		{
			pTempScoConnectDevice = NULL;
		}
		if (pTempScoConnectDevice != NULL)
		{
			PacketType = pTempScoConnectDevice->current_packet_type;
		}
		else
		{
			PacketType = BT_SCO_PACKET_HV3;
		}

		/* jakio20071228: test, force using hv3
*/
		PacketType = BT_SCO_PACKET_HV3;
		
		dest = BtGetValidTxScoPool(devExt);
		if(dest == NULL)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "Sco buffer not enough\n");

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePutTail(&pFrag->Free_FragList, &pEle->link);
			pFrag->FreeFrag_Count++;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		
			return BT_FRAG_RESOURCE_ERROR;
			
		}
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Sco dest = 0x%x, pConnectDevice = 0x%x, pScoConnectDevice = 0x%x\n", dest, pTempConnectDevice, pTempScoConnectDevice);
		pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
		if (pTempConnectDevice != NULL)
		{
			pHCBBHead->am_addr = pTempConnectDevice->am_addr;
		}
		else
		{
			pHCBBHead->am_addr = 0;
		}
		pHCBBHead->type = PacketType;
		#ifdef BT_PCM_COMPRESS
			#ifdef BT_SEND_MULTISCO_ONCE
				pHCBBHead->length = pEle->TxBlock[ppid].len / 2;
			#else
				pHCBBHead->length = PACKET_MAX_BYTES[PacketType] / 2;
			#endif
		#else
			pHCBBHead->length = pEle->TxBlock[ppid].len;
		#endif

		if(pHCBBHead->length != 120)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Error: sco length:%d, not equal to 120\n", pHCBBHead->length);
		}
		pHCBBHead->length = ALIGNLONGDATALEN(pHCBBHead->length);
		
		pHCBBHead->tx_retry_count = 0;
		pHCBBHead->slave_index = pTempConnectDevice->slave_index;
		pHCBBHead->master_slave_flag = (pTempConnectDevice->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;
		#ifdef BT_PCM_COMPRESS
			i = 0;
			if (pTempScoConnectDevice != NULL)
			{
				air_mode = pTempScoConnectDevice->air_mode;
			}
			else
			{
				air_mode = BT_AIRMODE_CVSD;
			}
			BT_DBGEXT(ZONE_FRG | LEVEL3, "compress air_mode = %d\n", air_mode);
			while (1)
			{
				input = *(PINT16)(source + pEle->TxBlock[ppid].bufpos + i);
				if (air_mode == BT_AIRMODE_A_LAW)
				{
					*(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + i / 2) = linear2alaw(input);
				}
				else if (air_mode == BT_AIRMODE_MU_LAW)
				{
					*(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + i / 2) = linear2ulaw(input);
				}
				else
				{
					*(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + i / 2) = linear2cvsd(input);
				}
				i += 2;
				if (i >= (UINT8)pEle->TxBlock[ppid].len)
				{
					break;
				}
			}
			#ifndef BT_SEND_MULTISCO_ONCE
				if (pEle->TxBlock[ppid].len < PACKET_MAX_BYTES[PacketType])
				{
					BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ProcessScoFrag: frag lengh is not enough, so padding it to zero!\n");
					RtlZeroMemory(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + pEle->TxBlock[ppid].len / 2, (PACKET_MAX_BYTES[PacketType] - pEle->TxBlock[ppid].len) / 2);
				}
			#endif
		#else
			RtlCopyMemory(dest + sizeof(HCBB_PAYLOAD_HEADER_T), source + pEle->TxBlock[ppid].bufpos, pEle->TxBlock[ppid].len);
		#endif



		Length = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
		Length = ALIGNLONGDATALEN(Length);

		if((Length+120) >= IdleSpace)
		{
			pEle->failure_times++;
			ReleaseBulkOut2Buf(dest, devExt);
			BT_DBGEXT(ZONE_FRG | LEVEL1, "Not enough space for sco\n");

			
			if(pEle->failure_times > 15)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL1, "Drop sco frag\n");
				pEle->failure_times = 0;
				pEle->CurrentIrp = NULL;
				KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
				QueuePutTail(&pFrag->Free_FragList, &pEle->link);
				pFrag->FreeFrag_Count++;
				KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

				timevalue.QuadPart = kOneMillisec;
				KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
				return BT_FRAG_RESOURCE_ERROR;
			}
			else
			{
				KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
				QueuePushHead(&pFrag->Sco_FragList, &pEle->link);
				KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	
				return BT_FRAG_RESOURCE_ERROR;
			}

		}
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pFrag->IdleSpace[FRAG_SCO_LIST] -= Length;
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

		status = BtUsbWriteAll(devExt, dest, Length, dataType, 0);
		if(NT_SUCCESS(status))
		{
			pEle->failure_times = 0;
			pEle->ppid++;	

			
		}
		else
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "sco flag sending failure\n");
			pEle->failure_times++;
			ReleaseBulkOut2Buf(dest, devExt);
			
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePushHead(&pFrag->Sco_FragList, &pEle->link);
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
			
			return BT_FRAG_RESOURCE_ERROR;
		}

		if (pEle->ppid == pEle->fpid)
		{
			pEle->CurrentIrp = NULL;
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePutTail(&pFrag->Free_FragList, &pEle->link);
			pFrag->FreeFrag_Count++;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
           BT_DBGEXT(ZONE_FRG | LEVEL3, "Free SCO frag\n");
		}
		else
		{
			//insert it into sco list again, and disable the queue
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			QueuePushHead(&pFrag->Sco_FragList, &pEle->link);
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Insert SCO frag for another handle\n");
		}

	}
	

	return BT_FRAG_SUCCESS;
}
/**************************************************************************
 *   Frag_InitTxScoForConnDev
 *
 *   Descriptions:
 *      Cancel the sending sco BD and pending IRP assocaited with the connect device.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Frag_InitTxScoForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{

	/* jakio20080219: add code to init sco buffer
*/
	UINT8 id;
	PBT_FRAG_T pFrag;
	if(pConnectDevice->pScoConnectDevice != NULL)
	{
		id = ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->index;

		ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_InitScoRxBuffer: SCO index = %d\n", id);

		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		pFrag->RxScoElement[id].currentpos = 0;
		pFrag->RxScoElement[id].currentlen = 0;
		pFrag->RxScoElement[id].totalcount = 0;
	#ifdef BT_2_1_SPEC_SUPPORT
		pFrag->RxScoElement[id].goodlen = 0;
	#endif
		pFrag->RxScoElement[id].RxCachPid = pFrag->RxScoElement[id].RxCachCid;
	}
	


	#if 0
		KIRQL oldIrql;
		UINT32 tmpConnectDevice;
		PBT_FRAG_T pFrag;
		UINT32 frageleno;
		COMPLETE_PACKETS_T CompletePackets;
		UINT8 i;
		UINT8 id;
		UINT32 firstbdid = 0;
		UINT32 endbdid = 0;
		UINT32 firstfragid = 0;
		UINT32 endfragid = 0;
		BOOLEAN exitflag = FALSE;
		BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_InitTxScoForConnDev: entered\n");
		RtlZeroMemory(&CompletePackets, sizeof(COMPLETE_PACKETS_T));
		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
		UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x3);
		if (pConnectDevice != NULL)
		{
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->is_in_disconnecting = 1;
				id = ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->index;
				ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
				BT_DBGEXT(ZONE_FRG | LEVEL3, "SCO index = %d\n", id);
				pFrag->RxScoElement[id].currentpos = 0;
				pFrag->RxScoElement[id].currentlen = 0;
				pFrag->RxScoElement[id].totalcount = 0;
			}
		}
		firstbdid = devExt->TxScoCid;
		endbdid = devExt->TxScoPid;
		while (firstbdid != endbdid)
		{
			tmpConnectDevice = devExt->TxScoBd[firstbdid].pConnectDevice;
			if (tmpConnectDevice == (UINT32)pConnectDevice)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "(SCO)Find one BD is not processed yet after its connection is closed:(id = %d) \n", firstbdid);
				devExt->TxScoBd[firstbdid].Valid = 0;
				BT_USB_WRITE_DWORDS((BtGetScoBDAddrById(firstbdid)), (PUINT32) &devExt->TxScoBd[firstbdid], EACH_BD_LENGTH >> 2);
			}
			firstbdid = BtGetNextBDId(firstbdid);
		}
		firstfragid = pFrag->ele_TxScoCid;
		endfragid = pFrag->ele_TxScoPid;
		while (firstfragid != endfragid)
		{
			frageleno = firstfragid;
			if (pFrag->TxScoFragElement[frageleno].pConnectDevice == (UINT32)pConnectDevice)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "(SCO)Frag_InitTxForConnDev: the Completed IRP is not this IRP, irp = 0x%x\n", pFrag->TxScoFragElement[frageleno].CurrentIrp);
				pFrag->TxScoFragElement[frageleno].Valid = 0;
				if (((PBT_HCI_T)devExt->pHci)->sco_flow_control_enable)
				{
					if (pFrag->TxScoFragElement[frageleno].send_complete_flag == 0)
					{
						pFrag->TxScoFragElement[frageleno].send_complete_flag = 1;
						for (i = 0; i < CompletePackets.number_of_handles; i++)
						{
							ASSERT(pConnectDevice->pScoConnectDevice != NULL);
							if (CompletePackets.connection_handle[i] == ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle)
							{
								CompletePackets.num_of_complete_packets[i]++;
								break;
							}
						}
						if ((i == CompletePackets.number_of_handles) && (i < BT_TOTAL_NUM_DATA_PACKET))
						{
							CompletePackets.number_of_handles++;
							ASSERT(pConnectDevice->pScoConnectDevice != NULL);
							CompletePackets.connection_handle[i] = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle;
							CompletePackets.num_of_complete_packets[i] = 1;
						}
					}
				}
			}
			firstfragid = Frag_GetNextEleId(firstfragid);
		}
		while (1)
		{
			firstbdid = devExt->TxScoCid;
			endbdid = devExt->TxScoPid;
			while (firstbdid != endbdid)
			{
				if (devExt->TxScoBd[firstbdid].Valid)
				{
					exitflag = TRUE;
					break;
				}
				BtWriteToReg(devExt, BT_DSP_C_SCO_PTR_REG, BtGetScoBDAddrById(BtGetNextBDId(firstbdid)));
				firstbdid = BtGetNextBDId(firstbdid);
			}
			if (exitflag)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Exist some SCO BD or element whose valid is 1\n");
				break;
			}
			KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
			KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
			if (BtIsScoBDEmpty(devExt) && Frag_IsScoEleEmpty(pFrag))
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "All is processed, just exit\n");
				break;
			}
		}
		UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0);
		KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
		if (CompletePackets.number_of_handles > 0)
		{
			Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(devExt, (PUINT8) &CompletePackets);
		}
	#endif
}
/**************************************************************************
 *   Frag_InitTxScoForAllDev
 *
 *   Descriptions:
 *      Cancel the sending sco BD and pending IRP assocaited with the connect device.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Frag_InitTxScoForAllDev(PBT_DEVICE_EXT devExt)
{
	#if 1
		KIRQL oldIrql;
		PBT_FRAG_T pFrag;
		PBT_HCI_T pHci;
		BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_InitTxScoForAllDev: entered\n");
		pHci = (PBT_HCI_T)devExt->pHci;
		if (pHci == NULL)
		{
			return ;
		}
		if (pHci->sco_link_count == 0)
		{
			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_BY_FEATURE);
			/*Get pointer to the test */
		}
		else
		{
		}
	#endif
}
/**************************************************************************
 *   Frag_AutoSelAclPacketType
 *
 *   Descriptions:
 *      Select next packet type according to the current packet type and retry count.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *      retrycount: IN, retry count.
 *   Return Value:
 *      None
 *************************************************************************/
#ifdef BT_ENHANCED_RATE
VOID Frag_AutoSelAclPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 retrycount)
{
	static int BeforeRetryCount = 0;
	int CountTranfer,Weight = 10;
	
	if (pConnectDevice->edr_mode != 0)
	{
		Frag_ExtentAutoSelAclPacketType(devExt,pConnectDevice,retrycount);
		return;
	}
	
	BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_AutoSelAclPacketType: retrycount = %d, old packet type = %d\n", retrycount, pConnectDevice->current_packet_type);
	
	switch (pConnectDevice->current_packet_type)
	{
		case (BT_ACL_PACKET_DH5):
			CountTranfer = retrycount;
			break;
		case (BT_ACL_PACKET_DM5):
			CountTranfer = ((193*retrycount)>>7)+51;
			break;
		case (BT_ACL_PACKET_DM3):
			CountTranfer = ((238*retrycount)>>7)+86;
			break;
		case (BT_ACL_PACKET_DM1):
			CountTranfer = ((850*retrycount)>>7)+564;
			break;
		default:
            CountTranfer = retrycount;
            BT_DBGEXT(ZONE_FRG | LEVEL0, "Error type");
			break;
	}
 
	if (BeforeRetryCount != 0)
	{
		CountTranfer = Weight*BeforeRetryCount+(128-Weight)*CountTranfer;
		retrycount   = CountTranfer>>7;
	}
	BeforeRetryCount = retrycount;

	if(retrycount<=70)
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
		else if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		else
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}
	else if((retrycount>70) && (retrycount<=136))
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM5;
		else if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		else
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}
	else if((retrycount>136) && (retrycount<=724))
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_1_SLOT)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		else
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
	}
	else if(retrycount>724)
	{
		pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}
 
	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_AutoSelAclPacketType: new packet type = %d\n", pConnectDevice->current_packet_type);
}
#else
VOID Frag_AutoSelAclPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 retrycount)
{
	static int BeforeRetryCount = 0;
	int CountTranfer, Weight = 100;
	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_AutoSelAclPacketType: retrycount = %d, old packet type = %d\n", retrycount, pConnectDevice->current_packet_type);
	switch (pConnectDevice->current_packet_type)
	{
		case (BT_ACL_PACKET_DH5): CountTranfer = retrycount;
		break;
		case (BT_ACL_PACKET_DM5): CountTranfer = ((193 *retrycount) >> 7) + 51;
		break;
		case (BT_ACL_PACKET_DM3): CountTranfer = ((238 *retrycount) >> 7) + 86;
		break;
		case (BT_ACL_PACKET_DM1): CountTranfer = ((850 *retrycount) >> 7) + 564;
		break;
		default:
			break;
	}
	if (BeforeRetryCount != 0)
	{
		CountTranfer = Weight * BeforeRetryCount + (128-Weight) *CountTranfer;
		retrycount = CountTranfer >> 7;
	}
	BeforeRetryCount = retrycount;
	if (retrycount <= 70)
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
		}
		else if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		}
		else
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		}
	}
	else if ((retrycount > 70) && (retrycount <= 136))
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM5;
		}
		else if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		}
		else
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		}
	}
	else if ((retrycount > 136) && (retrycount <= 724))
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_1_SLOT)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		}
		else
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		}
	}
	else if (retrycount > 724)
	{
		pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}
	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_AutoSelAclPacketType: new packet type = %d\n", pConnectDevice->current_packet_type);
}
#endif


	UINT32 Frag_GetNextScoRxCachId(UINT32 id)
	{
		UINT32 tmpid = id;
		ASSERT(tmpid < BT_SCO_MAX_RX_CACH_COUNT);
		if (tmpid == BT_SCO_MAX_RX_CACH_COUNT - 1)
		{
			return (0);
		}
		else
		{
			return (++tmpid);
		}
	}


	BOOLEAN Frag_IsScoRxCachEmpty(PBT_FRAG_T pFrag, UINT8 index)
	{
		ASSERT(index < BT_TOTAL_SCO_LINK_COUNT);
		ASSERT(pFrag->RxScoElement[index].RxCachPid < BT_SCO_MAX_RX_CACH_COUNT);
		ASSERT(pFrag->RxScoElement[index].RxCachCid < BT_SCO_MAX_RX_CACH_COUNT);
		return (pFrag->RxScoElement[index].RxCachPid == pFrag->RxScoElement[index].RxCachCid);
	}


	BOOLEAN Frag_IsScoRxCachFull(PBT_FRAG_T pFrag, UINT8 index)
	{
		ASSERT(index < BT_TOTAL_SCO_LINK_COUNT);
		ASSERT(pFrag->RxScoElement[index].RxCachPid < BT_SCO_MAX_RX_CACH_COUNT);
		ASSERT(pFrag->RxScoElement[index].RxCachCid < BT_SCO_MAX_RX_CACH_COUNT);
		return (Frag_GetNextScoRxCachId(pFrag->RxScoElement[index].RxCachPid) == pFrag->RxScoElement[index].RxCachCid);
	}

	NTSTATUS Frag_ProcessRxScoData(PBT_DEVICE_EXT devExt, PUINT8 source, UINT32 inlen, PUINT8 dest, PUINT32 poutlen)
	{
		UINT16 DataLen;
		UINT16 FrameType;
		PHCI_SCO_HEADER_T pscohead;
		PCONNECT_DEVICE_T pConnectDevice;
		PHCBB_PAYLOAD_HEADER_T pHCBBHead;
		PBT_HCI_T pHci;
		UINT8 i;
		UINT8 air_mode;
        
		DataLen = *(PUINT16)dest;
		FrameType = *(PUINT16)(dest + sizeof(UINT16));
		pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
		pHci = (PBT_HCI_T)devExt->pHci;
		BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_ProcessRxScoData: am_addr = %d, type = %d. length = %d\n", pHCBBHead->am_addr, pHCBBHead->type, pHCBBHead->length);
		#ifdef BT_USE_NEW_ARCHITECTURE
			if (pHCBBHead->master_slave_flag == (UINT32)BT_HCBB_MASTER_FLAG)
			{
				pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, (UINT8)pHCBBHead->am_addr);
			}
			else
			{
				pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, (UINT8)pHCBBHead->am_addr, (UINT8)pHCBBHead->slave_index);
			}
		#else
			if (pHci->role == BT_ROLE_MASTER)
			{
				pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, (UINT8)pHCBBHead->am_addr);
				if (pConnectDevice == NULL)
				{
					pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, (UINT8)pHCBBHead->am_addr);
				}
			}
			else
			{
				pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, (UINT8)pHCBBHead->am_addr);
				if (pConnectDevice == NULL)
				{
					pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, (UINT8)pHCBBHead->am_addr);
				}
			}
		#endif
		if (pConnectDevice == NULL)
		{
			return STATUS_RESOURCE_DATA_NOT_FOUND;
		}
		pscohead = (PHCI_SCO_HEADER_T)source;
		if (pConnectDevice->pScoConnectDevice != NULL)
		{
			pscohead->connection_handle = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle;
		}
		else
		{
			pscohead->connection_handle = 0;
		}
		pscohead->reserved = 0;
		pscohead->total_len = (UINT8)(pHCBBHead->length *2);
		if (inlen < pscohead->total_len + 1+sizeof(HCI_SCO_HEADER_T))
		{
			#ifdef BT_TESTDRIVER
				devExt->RealRecvNotSuccCount++;
			#endif
			return STATUS_BUFFER_TOO_SMALL;
		}
		#ifdef BT_PCM_COMPRESS
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				air_mode = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode;
			}
			else
			{
				air_mode = BT_AIRMODE_CVSD;
			}
			BT_DBGEXT(ZONE_FRG | LEVEL3, "decompress air_mode = %d\n", air_mode);
			for (i = 0; i < pHCBBHead->length; i++)
			{
				if (air_mode == BT_AIRMODE_A_LAW)
				{
					*(PINT16)(source + sizeof(HCI_SCO_HEADER_T) + i * 2) = alaw2linear(*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + i));
				}
				else if (air_mode == BT_AIRMODE_MU_LAW)
				{
					*(PINT16)(source + sizeof(HCI_SCO_HEADER_T) + i * 2) = ulaw2linear(*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + i));
				}
				else
				{
					*(PINT16)(source + sizeof(HCI_SCO_HEADER_T) + i * 2) = cvsd2linear(*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + i));
				}
			}
		#else
			RtlCopyMemory(source + sizeof(HCI_SCO_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), pHCBBHead->length);
		#endif
		*poutlen = pscohead->total_len + sizeof(HCI_SCO_HEADER_T);
		#ifdef DBG
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Read file data:\n");
		#endif
		#ifdef BT_TESTDRIVER
			devExt->RealRecvSuccCount += pscohead->total_len;
		#endif
		return STATUS_SUCCESS;
	}


	BOOLEAN Frag_IsAllScoRxCachEmpty(PBT_FRAG_T pFrag)
	{
		UINT8 i;
		for (i = 0; i < BT_TOTAL_SCO_LINK_COUNT; i++)
		{
			if (!Frag_IsScoRxCachEmpty(pFrag, i))
			{
				return FALSE;
			}
		}
		return TRUE;
	}


	BOOLEAN Frag_IsAllScoRxCachEmptyAndRetId(PBT_FRAG_T pFrag, PUINT8 pid)
	{
		UINT8 i;
		for (i = 0; i < BT_TOTAL_SCO_LINK_COUNT; i++)
		{
			if (!Frag_IsScoRxCachEmpty(pFrag, i))
			{
				*pid = i;
				return FALSE;
			}
		}
		return TRUE;
	}







void Frag_RebuildFrag(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	PBT_LIST_ENTRY_T pList;
	PBT_FRAG_ELEMENT_T pTxFragElement;
	PBT_FRAG_T		pFrag;
	NTSTATUS status;
	KIRQL    oldIrql;
	LARGE_INTEGER	timevalue;
	UINT16 reserved_len, curpos, Threshold = BT_FRAG_THRESHOLD;
	UINT8  ListFlag;
	
	if (devExt == NULL || pConnectDevice == NULL)
		return;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);

	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_RebuildFrag entered\n");
	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pList = Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	
	status = Frag_GetFragThreshold(devExt, pConnectDevice, &Threshold);
	if (status != STATUS_SUCCESS)
	{
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		return;
	}
		
	
	pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
	while (pTxFragElement != NULL)
	{
		if ((pTxFragElement->Valid == 1) && (pTxFragElement->pConnectDevice == (ULONG_PTR)pConnectDevice))
		{
			if (pTxFragElement->ppid < pTxFragElement->fpid && pTxFragElement->TxBlock[pTxFragElement->ppid].len > Threshold)
			{
				curpos = pTxFragElement->TxBlock[pTxFragElement->ppid].bufpos;
				reserved_len = (UINT16)(pTxFragElement->total_len - curpos);
				pTxFragElement->fpid = pTxFragElement->ppid;
				while (1)
				{
					if (reserved_len <= Threshold)
					{
						pTxFragElement->TxBlock[pTxFragElement->fpid].len    = reserved_len;
						pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos = curpos;
						
						BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_RebuildFrag: Element[%d].len = %d, bufpos = %d\n", pTxFragElement->fpid, pTxFragElement->TxBlock[pTxFragElement->fpid].len, pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos);
						
						pTxFragElement->fpid++;
						curpos += reserved_len;
						reserved_len = 0;
						break;
					}
					else
					{
						pTxFragElement->TxBlock[pTxFragElement->fpid].len    = Threshold;
						pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos = curpos;
						
						BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_RebuildFrag: Element[%d].len = %d, bufpos = %d\n", pTxFragElement->fpid, pTxFragElement->TxBlock[pTxFragElement->fpid].len, pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos);
						
						pTxFragElement->fpid++;
						curpos += Threshold;
						reserved_len -= Threshold;
					}
				} /* end while (1) */
			}
		}
		
		pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pTxFragElement->link);
	} /* end while (pTxFragElement != NULL) */

	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_RebuildFrag--exit\n");
	return;
}


VOID Frag_ListRebuild(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{
	PBT_LIST_ENTRY_T pList;
	PBT_FRAG_ELEMENT_T pTxFragElement;
	PBT_FRAG_T		pFrag;
	PCONNECT_DEVICE_T	pConnectDevice = NULL;
	NTSTATUS status;
	KIRQL    oldIrql;
	UINT16 reserved_len, curpos, Threshold = BT_FRAG_THRESHOLD;
	
	if (devExt == NULL)
		return;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return;

	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_ListRebuild entered\n");


	switch(ListFlag)
	{
	case FRAG_MASTER_LIST:
		pList = &pFrag->Master_FragList;
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild----master list\n");
		break;
	case FRAG_SLAVE_LIST:
		pList = &pFrag->Slave_FragList;
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild----slave list\n");
		break;
	case FRAG_SLAVE1_LIST:
		pList = &pFrag->Slave1_FragList;
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild----slave1 list\n");
		break;
	case FRAG_SNIFF_LIST:
		pList = &pFrag->Sniff_FragList;
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild----sniff list\n");
		break;
	case FRAG_SCO_LIST:
		pList = &pFrag->Sco_FragList;
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild----sco list\n");
		break;
	default:
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Error list index:%d, list does not exist\n",ListFlag);
		return;
	}


	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
	while (pTxFragElement != NULL)
	{
		pConnectDevice = (PCONNECT_DEVICE_T)pTxFragElement->pConnectDevice;
		if(pConnectDevice == NULL)
		{
			BT_DBGEXT(ZONE_FRG | LEVEL0, "connect device pointer is null, error!\n");
			return;
		}

		Frag_GetFragThreshold(devExt, pConnectDevice, &Threshold);
		
		if (pTxFragElement->ppid < pTxFragElement->fpid && pTxFragElement->TxBlock[pTxFragElement->ppid].len > Threshold)
		{
			curpos = pTxFragElement->TxBlock[pTxFragElement->ppid].bufpos;
			reserved_len = (UINT16)(pTxFragElement->total_len - curpos);
			pTxFragElement->fpid = pTxFragElement->ppid;
			while (1)
			{
				if (reserved_len <= Threshold)
				{
					pTxFragElement->TxBlock[pTxFragElement->fpid].len    = reserved_len;
					pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos = curpos;
					
					BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild: Element[%d].len = %d, bufpos = %d\n", pTxFragElement->fpid, pTxFragElement->TxBlock[pTxFragElement->fpid].len, pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos);
					
					pTxFragElement->fpid++;
					curpos += reserved_len;
					reserved_len = 0;
					break;
				}
				else
				{
					pTxFragElement->TxBlock[pTxFragElement->fpid].len    = Threshold;
					pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos = curpos;
					
					BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ListRebuild: Element[%d].len = %d, bufpos = %d\n", pTxFragElement->fpid, pTxFragElement->TxBlock[pTxFragElement->fpid].len, pTxFragElement->TxBlock[pTxFragElement->fpid].bufpos);
					
					pTxFragElement->fpid++;
					curpos += Threshold;
					reserved_len -= Threshold;
				}
			} /* end while (1) */
		}

		
		pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pTxFragElement->link);
	} /* end while (pTxFragElement != NULL) */
	
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_ListRebuild--exit\n");
	return;
}

VOID Frag_ListRebuildForSco(PBT_DEVICE_EXT devExt)
{
	Frag_ListRebuild(devExt, FRAG_MASTER_LIST);
	Frag_ListRebuild(devExt, FRAG_SLAVE_LIST);
	Frag_ListRebuild(devExt, FRAG_SLAVE1_LIST);
}

VOID Frag_StopQueue(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{
	PBT_FRAG_T	pFrag;


	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Null frag pointer, return\n");
		return;
	}

	if(ListFlag >  MAX_LIST_NUM)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Invalid frag list, return\n");
		return;
	}

	pFrag->list_stop_flag[ListFlag] = LIST_STOP_FLAG;
}


VOID Frag_StartQueue(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{
	PBT_FRAG_T	pFrag;


	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Null frag pointer, return\n");
		return;
	}

	if(ListFlag >  MAX_LIST_NUM)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Invalid frag list, return\n");
		return;
	}

	pFrag->list_stop_flag[ListFlag] = 0;
}

VOID Frag_StopQueueForSco(PBT_DEVICE_EXT devExt)
{
	BT_DBGEXT(ZONE_FRG | LEVEL2, "frag_stopqueue for sco\n");
	Frag_StopQueue(devExt, FRAG_MASTER_LIST);
	Frag_StopQueue(devExt, FRAG_SLAVE_LIST);
	Frag_StopQueue(devExt, FRAG_SLAVE1_LIST);
}

VOID Frag_StartQueueForSco(PBT_DEVICE_EXT devExt, UINT8 NeedRebuildFlag)
{
	PBT_FRAG_T	pFrag;
	LARGE_INTEGER	timevalue;
	KIRQL		oldIrql;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Null frag pointer, return\n");
		return;
	}
	BT_DBGEXT(ZONE_FRG | LEVEL3, "frag_start queue for sco\n");

	if(NeedRebuildFlag)
		Frag_ListRebuildForSco(devExt);

	Frag_StartQueue(devExt, FRAG_MASTER_LIST);
	Frag_StartQueue(devExt, FRAG_SLAVE_LIST);
	Frag_StartQueue(devExt, FRAG_SLAVE1_LIST);

	//set timer to check acl queues
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	timevalue.QuadPart = (10*kOneMillisec);
	KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
	//UsbQueryDMASpace(devExt);
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
}



NTSTATUS Frag_GetFragThreshold(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT16 pFragThreshold)
{
	UINT16  current_frag_threshold;
	PUINT16 pPacketMaxSize;

	
	if ((devExt == NULL) || (pConnectDevice == NULL) || (pFragThreshold == NULL))
	{
		return STATUS_RESOURCE_DATA_NOT_FOUND;
	}
	
	if (pConnectDevice->edr_mode == 0)
	{
		pPacketMaxSize = (PUINT16)PACKET_MAX_BYTES;
		if (((PBT_HCI_T)devExt->pHci)->acl_force_1_slot == 0)
		{
			if (((PBT_HCI_T)devExt->pHci)->auto_packet_select_flag)
			{
			#ifdef BT_AUTO_SEL_PACKET
				current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[pConnectDevice->current_packet_type] ? BT_FRAG_THRESHOLD : pPacketMaxSize[pConnectDevice->current_packet_type];
			#else 
				current_frag_threshold = BT_FRAG_THRESHOLD;
			#endif 
			}
			else //SCO EXIST
			{
				if (((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag == 0)
				{
					if (((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
						current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_ACL_PACKET_DH1] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_ACL_PACKET_DH1];
					else
						current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_ACL_PACKET_DH3] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_ACL_PACKET_DH3];
				}
				else
				{
					if (((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag == 0)
						current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_ACL_PACKET_DH1] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_ACL_PACKET_DH1];
					else
						current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_SCO_PACKET_DV] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_SCO_PACKET_DV];
				}
			}
		}
		else
		{
			if (((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag)
				current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_SCO_PACKET_DV] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_SCO_PACKET_DV];
			else if (((PBT_HCI_T)devExt->pHci)->aux1_instead_dh1_flag)
				current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_ACL_PACKET_AUX1] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_ACL_PACKET_AUX1];
			else
				current_frag_threshold = BT_FRAG_THRESHOLD <= pPacketMaxSize[BT_ACL_PACKET_DH1] ? BT_FRAG_THRESHOLD : pPacketMaxSize[BT_ACL_PACKET_DH1];
		}
		
	#ifdef BT_FORCE_MASTER_SNIFF_ONE_SLOT
		if (pConnectDevice->current_role == BT_ROLE_MASTER && pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
			current_frag_threshold = (current_frag_threshold <= pPacketMaxSize[BT_ACL_PACKET_DH1] ? current_frag_threshold : pPacketMaxSize[BT_ACL_PACKET_DH1]);
	#endif 
	}
	else
	{
		pPacketMaxSize = (PUINT16)EDR_PACKET_MAX_BYTES;
		if (((PBT_HCI_T)devExt->pHci)->acl_force_1_slot == 0) // =0 means maybe send multislot packet
		{
			if (((PBT_HCI_T)devExt->pHci)->auto_packet_select_flag)
			{
			#ifdef BT_AUTO_SEL_PACKET
				/* changed by lewis */
				current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[pConnectDevice->current_packet_type] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[pConnectDevice->current_packet_type];
			#else 
				current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED;
			#endif 
			}
			else // auto_packet_select_flag=0 when sco(eSCO) link exist.at this time,max slot used to send packet can be up to 3slot
			{
				if (((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag == 0)
				{
					/* changed by lewis */
					if (((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
					{
						if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] != 0)
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH3] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH3];
						else
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH1];
					}
					else
					{
						if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] != 0)
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH3] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH3];
						else
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH1];
					}
				}
				else
				{
					if (((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag == 0)
					{
						if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] != 0)
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH3] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH3];
						else
							current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH1];
					}
					else
						current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_SCO_PACKET_DV] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_SCO_PACKET_DV];
				}
			}
		}
		else
		{
			if (((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag)
				current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_SCO_PACKET_DV] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_SCO_PACKET_DV];
			else if (((PBT_HCI_T)devExt->pHci)->aux1_instead_dh1_flag)
				current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_AUX1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_AUX1];
			else
			{
				if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] != 0)
					current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH3] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH3];
				else
					current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH1];
			}
		}
		
	#ifdef BT_FORCE_MASTER_SNIFF_ONE_SLOT
		if (pConnectDevice->current_role == BT_ROLE_MASTER && pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
		{
			/*
			if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] != 0)
				current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH5] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH5];
			else
			*/
			current_frag_threshold = BT_FRAG_THRESHOLD_ENHANCED <= pPacketMaxSize[BT_ACL_PACKET_2DH1] ? BT_FRAG_THRESHOLD_ENHANCED : pPacketMaxSize[BT_ACL_PACKET_2DH1];
		}
	#endif
	}
	
	*pFragThreshold = current_frag_threshold;

	
	return (STATUS_SUCCESS);
}




/* Set feature bit for BT 2.0,called by LMP when enter BT 2.0  mode.added by nescafe*/
NTSTATUS Frag_SetConnectionFeatureBit(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if ((devExt == NULL) || (pConnectDevice == NULL))
	{
		return STATUS_RESOURCE_DATA_NOT_FOUND;
	}
	
	if (pConnectDevice->edr_mode == 0)
	{
		return (STATUS_SUCCESS);
	}

	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_SetConnectionFeatureBit entered\n");
	
	RtlZeroMemory(pConnectDevice->EDR_FeatureBit, sizeof(pConnectDevice->EDR_FeatureBit));
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_3 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_3 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot5_enh_acl == 1) && (pConnectDevice->lmp_features.byte5.slot5_enh_acl == 1))
	{
		pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] = 1;
	}
	
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_2 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_2 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot5_enh_acl == 1) && (pConnectDevice->lmp_features.byte5.slot5_enh_acl == 1))
	{
		pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] = 1;
	}
	
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_3 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_3 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.slot3_enh_acl == 1) && (pConnectDevice->lmp_features.byte4.slot3_enh_acl == 1))
	{
		pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] = 1;
	}
	
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_2 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_2 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.slot3_enh_acl == 1) && (pConnectDevice->lmp_features.byte4.slot3_enh_acl == 1))
	{
		pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] = 1;
	}
	
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_3 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_3 == 1))
	{
		pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH1] = 1;
	}
	
	pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH1] = 1;
	
	/* add by lewis below */
	RtlZeroMemory(pConnectDevice->EDR_PacketBit, sizeof(pConnectDevice->EDR_PacketBit));
	pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_2DH1] = 1;
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_3DH5) == 0)
		pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_3DH5] = 1;
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_2DH5) == 0)
		pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_2DH5] = 1;
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_3DH3) == 0)
		pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_3DH3] = 1;
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_2DH3) == 0)
		pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_2DH3] = 1;
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_3DH1) == 0)
		pConnectDevice->EDR_PacketBit[BT_ACL_PACKET_3DH1] = 1;

	return (STATUS_SUCCESS);
}



void Frag_GetCurrentPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 i;
	
	BT_DBGEXT(ZONE_FRG | LEVEL2, "Frag_GetCurrentPacketType enter!\n");
	
	if (pConnectDevice->edr_mode == 0)
	{
		if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
		{
			if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH5)
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
			else
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DM5;
		}
		else if (pConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
		{
			if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH3)
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DH3;
			else
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		}
		else
		{
			if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH1)
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DH1;
			else
				pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		}

		if(devExt->ComboState == FW_WORK_MODE_COMBO)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DH3;
	}
	else
	{
		pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH1;
		if (pConnectDevice->tx_max_slot > BT_MAX_SLOT_5_SLOT)
			pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;

		/* jakio20071101, I mark these codes for test, when debugging on FPGA board, the space is not enough,so the smaller the packet is,
*/
		#if 1	
		for (i = BT_ACL_PACKET_3DH5; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}

		if(devExt->ComboState == FW_WORK_MODE_COMBO)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH3;
		#endif
		
		
	}
	
	BT_DBGEXT(ZONE_FRG | LEVEL3, "current_packet_type= 0x%x, edr_mode= %d, packet_type= 0x%x, BD0= 0x%x!\n", pConnectDevice->current_packet_type, pConnectDevice->edr_mode, pConnectDevice->packet_type, pConnectDevice->bd_addr[0]);
	return;
}


/**************************************************************************
 *   Frag_ExtentAutoSelAclPacketType
 *
 *   Descriptions:
 *      Select next packet type according to the current packet type and retry count (this function is for EDR).
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *      retrycount: IN, retry count.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Frag_ExtentAutoSelAclPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 retrycount)
{
	UINT8 i;
	static int BeforeRetryCount = 0;
	int CountTranfer,Weight = 10;

	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ExtentAutoSelAclPacketType: retrycount = %d, old packet type = %d\n", retrycount, pConnectDevice->current_packet_type);

	switch (pConnectDevice->current_packet_type)
	{
	case (BT_ACL_PACKET_3DH5):
		CountTranfer = retrycount;
		break;
	case (BT_ACL_PACKET_3DH3):
		CountTranfer = ((170*retrycount)>>7)+33;
		break;
	case (BT_ACL_PACKET_2DH5):
		CountTranfer = ((214*retrycount)>>7)+67;
		break;
	case (BT_ACL_PACKET_2DH3):
		CountTranfer = ((256*retrycount)>>7)+100;
		break;
	case (BT_ACL_PACKET_3DH1):
		CountTranfer = ((554*retrycount)>>7)+333;
		break;
	case (BT_ACL_PACKET_2DH1):
		CountTranfer = ((810*retrycount)>>7)+533;
		break;
	case (BT_ACL_PACKET_DM1):
		CountTranfer = ((2581*retrycount)>>7)+1917;
		break;
	default:
        CountTranfer = retrycount;
		break;
	}

	CountTranfer = Weight*BeforeRetryCount+(128-Weight)*CountTranfer;
	retrycount = CountTranfer>>7;
	BeforeRetryCount = retrycount;
	retrycount = retrycount>>1;    //Added by Feng Wu 08/31/2007 in order to halve the threshold of rate adjusting

	if(retrycount<=58)
	{
		for (i = BT_ACL_PACKET_3DH5; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}
	}
	else if((retrycount>58) && (retrycount<=139))
	{
		for (i = BT_ACL_PACKET_3DH3; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}
	}
	else if((retrycount>139) && (retrycount<=162))
	{
		for (i = BT_ACL_PACKET_2DH5; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}
	}
	else if((retrycount>162)&& (retrycount<=752))
	{
		for (i = BT_ACL_PACKET_2DH3; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}
	}
	else if((retrycount>752)&& (retrycount<=930))
	{
		for (i = BT_ACL_PACKET_3DH1; i >= BT_ACL_PACKET_2DH1; i--)
		{
			if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
			{
				pConnectDevice->current_packet_type = i;
				break;
			}
		}
	}
	else if((retrycount>930)&& (retrycount<=1977))
	{
		pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH1;
	}
	else if(retrycount>1977)
	{
		pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}

	BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_ExtentAutoSelAclPacketType: new packet type = %d\n", pConnectDevice->current_packet_type);
}



VOID Frag_InitScoRxBuffer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 id;
	PBT_FRAG_T pFrag;
	PSCO_CONNECT_DEVICE_T pScoConnectDev = NULL;

	pScoConnectDev = (PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice;
	if(pScoConnectDev != NULL)
	{
		id = pScoConnectDev->index;

		ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_InitScoRxBuffer: SCO index = %d\n", id);

		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		pFrag->RxScoElement[id].currentpos = 0;
		pFrag->RxScoElement[id].currentlen = 0;
		pFrag->RxScoElement[id].totalcount = 0;
	#ifdef BT_2_1_SPEC_SUPPORT
		pFrag->RxScoElement[id].goodlen = 0;
	#endif
		pFrag->RxScoElement[id].RxCachPid = pFrag->RxScoElement[id].RxCachCid;	
	}
	
}



/*Get packet Type .added by nescafe*/
NTSTATUS Frag_GetFragPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 len, PUINT8 pPacketType)
{
	UINT8 PacketType = BT_ACL_PACKET_DH1;
	if ((devExt == NULL) || (pConnectDevice == NULL) || (pPacketType == NULL))
	{
		return STATUS_RESOURCE_DATA_NOT_FOUND;
	}
	
	if (pConnectDevice->edr_mode == 0) //bluetooth ver 1.2
	{
			BT_DBGEXT(ZONE_FRG | LEVEL3, "frag packet type v1.2\n");

		if (((PBT_HCI_T)devExt->pHci)->aux1_instead_dh1_flag)
		{
			if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM1])
				PacketType = BT_ACL_PACKET_DM1;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_AUX1])
				PacketType = BT_ACL_PACKET_AUX1;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM3])
				PacketType = BT_ACL_PACKET_DM3;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DH3])
				PacketType = BT_ACL_PACKET_DH3;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM5])
				PacketType = BT_ACL_PACKET_DM5;
			else
				PacketType = BT_ACL_PACKET_DH5; /* 339 */
		}
		else
		{
			if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM1])
				PacketType = BT_ACL_PACKET_DM1;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DH1])
				PacketType = BT_ACL_PACKET_DH1;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM3])
				PacketType = BT_ACL_PACKET_DM3;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DH3])
				PacketType = BT_ACL_PACKET_DH3;
			else if (len <= PACKET_MAX_BYTES[BT_ACL_PACKET_DM5])
				PacketType = BT_ACL_PACKET_DM5;
			else
				PacketType = BT_ACL_PACKET_DH5; /* 339 */
		}
	}
	else
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "frag packet type v2.0\n");

		if (((PBT_HCI_T)devExt->pHci)->aux1_instead_dh1_flag)
		{
			if (len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_DM1])
				PacketType = BT_ACL_PACKET_DM1;
			else if (len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_AUX1])
				PacketType = BT_ACL_PACKET_AUX1;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_2DH3]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 1))
				PacketType = BT_ACL_PACKET_2DH3;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_3DH3]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] == 1))
				PacketType = BT_ACL_PACKET_3DH3;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_2DH5]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] == 1))
				PacketType = BT_ACL_PACKET_2DH5;
			else if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] == 1)
				PacketType = BT_ACL_PACKET_3DH5;
		}
		else
		{
			if (len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_DM1])
				PacketType = BT_ACL_PACKET_DM1;
			else if (len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_2DH1])
				PacketType = BT_ACL_PACKET_2DH1;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_3DH1]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH1] == 1))
				PacketType = BT_ACL_PACKET_3DH1;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_2DH3]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 1))
				PacketType = BT_ACL_PACKET_2DH3;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_3DH3]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] == 1))
				PacketType = BT_ACL_PACKET_3DH3;
			else if ((len <= EDR_PACKET_MAX_BYTES[BT_ACL_PACKET_2DH5]) && (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] == 1))
				PacketType = BT_ACL_PACKET_2DH5;
			else if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] == 1)
				PacketType = BT_ACL_PACKET_3DH5;
		}
	}
	
	*pPacketType = PacketType;
	return (STATUS_SUCCESS);
}

/*************************************************************
 *   Frag_ListStopSwitch
 *
 *   Descriptions:
 *      This function controls the new fragment timer.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *      pConnectDevice: IN, the pointer to connect device.
 *      timer_value: IN, 0: cancel stop; other: timer value and stop fragment list operation.
 *
 *   Return Value: 
 *      STATUS_SUCCESS; STATUS_PENDING.
 *************************************************************/
void Frag_ListStopSwitch(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_value)
{
	UINT8  ListFlag;

	if (devExt == NULL || pConnectDevice == NULL)
		return;
	
	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if (timer_value != 0)
	{
		((PBT_FRAG_T)devExt->pFrag)->list_timer[ListFlag]     = timer_value;
		((PBT_FRAG_T)devExt->pFrag)->list_stop_flag[ListFlag] = LIST_STOP_FLAG;
	}
	else
	{
		((PBT_FRAG_T)devExt->pFrag)->list_stop_flag[ListFlag] = 0;
		((PBT_FRAG_T)devExt->pFrag)->list_timer[ListFlag]     = 0;
	}
	
	return;
}

void Frag_MoveFrag(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_LIST_ENTRY_T pDestList, PBT_LIST_ENTRY_T pSourceList)
{
	UINT8 i;
	KIRQL oldIrql;
	PBT_FRAG_ELEMENT_T pTxFragElement, pTxFragElement2;
	
	KeAcquireSpinLock(&((PBT_FRAG_T)devExt->pFrag)->FragTxLock, &oldIrql);
	pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetHead(pSourceList);
	while (pTxFragElement != NULL)
	{
		if ((ULONG_PTR)pConnectDevice == pTxFragElement->pConnectDevice)
		{
			if (pTxFragElement->fcid != pTxFragElement->ppid)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL1, "Frag_MoveFrag: fcid(%d) != ppid(%d)! (BD0=0x%x)\n", pTxFragElement->fcid, pTxFragElement->ppid, pConnectDevice->bd_addr[0]);
				for (i = 0; i < 10; i++)
					BT_DBGEXT(ZONE_FRG | LEVEL0, "Frag_MoveFrag: error!!! (BD0=0x%x)\n", pConnectDevice->bd_addr[0]);
				
				pTxFragElement->fcid = pTxFragElement->ppid;
			}
			
			pTxFragElement2 = pTxFragElement;
			pTxFragElement  = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pTxFragElement->link);
			QueueRemoveEle(pSourceList, &pTxFragElement2->link);
			QueuePutTail(pDestList, &pTxFragElement2->link);
			
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_MoveFrag: move one fragment! (BD0=0x%x)\n", pConnectDevice->bd_addr[0]);
		}
		else
		{
			pTxFragElement = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pTxFragElement->link);
		}
	}
	KeReleaseSpinLock(&((PBT_FRAG_T)devExt->pFrag)->FragTxLock, oldIrql);
	
	return;
}

void Frag_FlushFragAndBD(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ListFlag)
{
}

BOOLEAN Frag_IsBDEmpty(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{

	PBT_FRAG_T	pFrag = NULL;
	KIRQL	oldIrql;
	BOOLEAN		ret = FALSE;
	pFrag = (PBT_FRAG_T)devExt->pFrag;

	if(pFrag == NULL)
		return FALSE;

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	switch(ListFlag)
	{
		case FRAG_MASTER_LIST:
			if(pFrag->IdleSpace[FRAG_MASTER_LIST] == BT_MAX_MASTER_IDLE_SPACE_SIZE)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Master queue is empty\n");
				ret = TRUE;
			}
			else
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Master queue not empty\n");
			}
			break;
		case FRAG_SLAVE_LIST:
			if(pFrag->IdleSpace[FRAG_SLAVE_LIST] == BT_MAX_SLAVE_IDLE_SPACE_SIZE)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Slave queue is empty\n");
				ret = TRUE;
			}
			else
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Slave queue not empty\n");
			}
			break;
		case FRAG_SCO_LIST:
			if(pFrag->IdleSpace[FRAG_SCO_LIST] == BT_MAX_SCO_IDLE_SPACE_SIZE)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Sco queue is empty\n");
				ret = TRUE;
			}
			else
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Sco queue not empty\n");
			}
			break;
		case FRAG_SNIFF_LIST:
			if(pFrag->IdleSpace[FRAG_SNIFF_LIST] == BT_MAX_SNIFF_IDLE_SPACE_SIZE)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Sniff queue is empty\n");
				ret = TRUE;
			}
			else
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Sniff queue not empty\n");
			}
			break;
		default:
			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_IsBDEmpty--Invalid tx queue index:%d\n", ListFlag);
			
	}
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	return ret;
}







VOID Frag_SetTimerType(PBT_DEVICE_EXT devExt, UINT8 ListIndex)
{
	PBT_FRAG_T	pFrag;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return;

	switch(ListIndex)
	{
		case FRAG_MASTER_LIST:
			break;
		case FRAG_SLAVE_LIST:
			break;
		case FRAG_SLAVE1_LIST:
			break;
		case FRAG_SNIFF_LIST:
			break;
		case FRAG_SCO_LIST:
			break;
	}
	return;
}

/*0x40  00  BYTE0 BYTE1 BYTE2 BYTE3 BYTE4 BYTE5 
 *bit0~bit9	Master idle space
 *bit10~bit19   Slave0 idle space
 *bit20~bit29   Slacve1 idle space
 *byte4		sniff   idle  space
 *byte5		sco idle space
 *The length has been 4bytes aligned
*/
VOID Frag_ProcessSpaceInt(PBT_DEVICE_EXT devExt, PUINT8 pBuffer)
{
	PBT_FRAG_T	pFrag;
	UINT32	MasterSpace;
	UINT32	SlaveSpace;
	UINT32	Slave1Space;
	UINT32	SniffSpace;
	UINT32	ScoSpace;
	UINT32	tmpLong;
	UINT8	NeedSetTimer = 0;
	LARGE_INTEGER	timevalue;
	KIRQL	oldIrql;
	UINT8	i;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return;
	tmpLong = *(PUINT32)(pBuffer + 2);
	MasterSpace = (tmpLong & 0x3ff) << 2;
	SlaveSpace = ((tmpLong >> 10) & 0x3ff) << 2;


	/* jakio20071229: for test sco, we use slave1 space for sco sending
*/
	ScoSpace = ((tmpLong >> 20) & 0x3ff) << 2;
	SniffSpace = *(pBuffer+6) << 2;
	
    
	BT_DBGEXT(ZONE_FRG | LEVEL4, "Master Space 0x%4x\n", MasterSpace);
	BT_DBGEXT(ZONE_FRG | LEVEL4, "Slave Space  0x%4x\n", SlaveSpace);
	BT_DBGEXT(ZONE_FRG | LEVEL4, "Sco Space:    0x%4x\n", ScoSpace);
	BT_DBGEXT(ZONE_FRG | LEVEL4, "Sniff Space:    0x%4x\n", SniffSpace);
    

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	if(pFrag->IdleSpace[FRAG_MASTER_LIST] < MasterSpace)
	{
		NeedSetTimer = 1;
			
	}
	pFrag->IdleSpace[FRAG_MASTER_LIST] = MasterSpace;
	
	if(pFrag->IdleSpace[FRAG_SLAVE_LIST] < SlaveSpace)
	{
		NeedSetTimer = 1;
			
	}
	pFrag->IdleSpace[FRAG_SLAVE_LIST] = SlaveSpace;

	/*
	if(pFrag->IdleSpace[FRAG_SLAVE_LIST] < Slave1Space)
	{
		NeedSetTimer = 1;
			
		pFrag->TxTimerSource |= TIMER_SOURCE_SLAVE1_BIT;
	}
	pFrag->IdleSpace[FRAG_SLAVE1_LIST] = Slave1Space;
	*/
	
	if(pFrag->IdleSpace[FRAG_SNIFF_LIST] < SniffSpace)
	{
		NeedSetTimer = 1;
		
	}
	pFrag->IdleSpace[FRAG_SNIFF_LIST] = SniffSpace;	

	
	
	if(pFrag->IdleSpace[FRAG_SCO_LIST] < ScoSpace)
	{
		NeedSetTimer = 1;
		
	}
	pFrag->IdleSpace[FRAG_SCO_LIST] = ScoSpace;	

	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	Frag_CheckTxQueue(devExt);


	/* jakio20080606:Reset fail count, Patch for vista
*/
	devExt->QueryFailCount = 0;

	
	devExt->SpaceQueryFlag = FALSE;
}


VOID Frag_TxTimerInstant(PBT_DEVICE_EXT devExt, PLARGE_INTEGER timeValue)
{
	PBT_FRAG_T	pFrag;
	PBT_HCI_T	pHci;
	KIRQL	oldIrql;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	pHci  = (PBT_HCI_T)devExt->pHci;
	if((pFrag == NULL) || (pHci == NULL))
	{
		BT_DBGEXT(ZONE_FRG | LEVEL0, "Frag_TxTimerInstant--Error,NULL Pointer\n");
		return;
	}

	//if a slave is connecting
	KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
	if(devExt->RxState == RX_STATE_CONNECTING)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_TxTimerInstant--RX_STATE_CONNECTING\n");
		timeValue->QuadPart = kOneMillisec;
		KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
		return;
	}
	KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if((pHci->num_device_am + pHci->num_device_slave) > 1)
		timeValue->QuadPart = kOneMillisec;
	else
		timeValue->QuadPart = kOneMillisec;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);	
}




/***************************************************************
*Description:
*	this routine check if we need send sco null frames to some device, which is
*	in order to solve bugs when connecting with some headphones.  if we don't
*	send sco packets to them first, they won't either, so, we will be silence breaker
*	always: sending null sco until receiving a sco from device.
****************************************************************/
VOID Frag_CheckAndAddScoNullFrame(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T	pFrag;
	PBT_HCI_T	pHci;
	PCONNECT_DEVICE_T	pConnDev = NULL;
	PSCO_CONNECT_DEVICE_T	pScoConnDev = NULL;
	PBT_FRAG_ELEMENT_T		pEle = NULL;
	KIRQL	oldIrql;
	KIRQL	oldIrql2;


	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame--Null HCI module pointer\n");
		return;
	}

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame--Null FRAG module pointer\n");
		return;
	}

	
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pConnDev = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while(pConnDev)
	{
		pScoConnDev = pConnDev->pScoConnectDevice;
		if((pScoConnDev != NULL) && (pScoConnDev->NeedSendScoNull))
		{

			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame--Get sco device: %02x\n", pConnDev->bd_addr[0]);
			
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql2);

			pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Free_FragList);
			

			if(pEle == NULL)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame: Frag SCO element is exhaused\n");
				
				KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql2);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				return;
			}

			pFrag->FreeFrag_Count--;
			
			pEle->pConnectDevice = (ULONG_PTR)pConnDev;
			pEle->total_len = BT_FRAG_SCO_THRESHOLD;
			pEle->ppid = 0;
			pEle->fcid = 0;
			pEle->fpid = 1;	//we fill one packet here
			pEle->link_type = pScoConnDev->link_type;
			pEle->failure_times = 0;
			RtlZeroMemory(&pEle->CompletPkt, sizeof(COMPLETE_PACKETS_T));
			RtlZeroMemory(&pEle->scoHeader, sizeof(pEle->scoHeader));
			if(pScoConnDev->is_in_disconnecting != 0)
			{
				pEle->Valid = 0;
			}
			else
			{
				pEle->Valid = 1;
			}
			RtlFillMemory(pEle->memory, pEle->total_len, 0x00);
			pEle->TxBlock[0].len = pEle->total_len;
			pEle->TxBlock[0].bufpos = 0;

			QueuePutTail(&pFrag->Sco_FragList, &pEle->link);

			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql2);

		}

		pConnDev = (PCONNECT_DEVICE_T)QueueGetNext(&pConnDev->Link);
	}



	pConnDev = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while(pConnDev)
	{
		pScoConnDev = pConnDev->pScoConnectDevice;
		if((pScoConnDev != NULL) && (pScoConnDev->NeedSendScoNull))
		{

			BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame--Get sco device: %02x\n", pConnDev->bd_addr[0]);

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql2);

			pEle = (PBT_FRAG_ELEMENT_T)QueuePopHead(&pFrag->Free_FragList);
			

			if(pEle == NULL)
			{
				BT_DBGEXT(ZONE_FRG | LEVEL3, "Frag_CheckAndAddScoNullFrame: Frag SCO element is exhaused\n");

				//unlock all
				KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql2);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				return;
			}

			pFrag->FreeFrag_Count--;
			
			pEle->pConnectDevice = (ULONG_PTR)pConnDev;
			pEle->total_len = BT_FRAG_SCO_THRESHOLD;
			pEle->ppid = 0;
			pEle->fcid = 0;
			pEle->fpid = 1;	//we fill one packet here
			pEle->link_type = pScoConnDev->link_type;
			pEle->failure_times = 0;
			RtlZeroMemory(&pEle->CompletPkt, sizeof(COMPLETE_PACKETS_T));
			RtlZeroMemory(&pEle->scoHeader, sizeof(pEle->scoHeader));
			if(pScoConnDev->is_in_disconnecting != 0)
			{
				pEle->Valid = 0;
			}
			else
			{
				pEle->Valid = 1;
			}
			RtlFillMemory(pEle->memory, pEle->total_len, 0x00);
			pEle->TxBlock[0].len = pEle->total_len;
			pEle->TxBlock[0].bufpos = 0;

			QueuePutTail(&pFrag->Sco_FragList, &pEle->link);

			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql2);

		}

		pConnDev = (PCONNECT_DEVICE_T)QueueGetNext(&pConnDev->Link);
	}
	
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);	


	return;
}
