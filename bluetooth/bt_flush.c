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
#include "bt_flush.h"
#include "sched.h"
#ifdef BT_SERIALIZE_DRIVER
	#include "bt_serialize.h"
#endif


/********************************************************
*Desc:
*	Alloc and Initialize data structure used for flush function. In Principle, we
*	should keep our driver normal even if this routine is failed, because flush
*	is taken as a accessorial function.
*Return Value:
*	STATUS_SUCCESS: allocation and initialization success
*	STATUS_UNSUCCESSFUL: failed to allocate buffer for flush
*	
********************************************************/
NTSTATUS Flush_Init(PBT_DEVICE_EXT DevExt)
{

	PBTUSB_FLUSH_T	pFlush;
	UINT32	i;
	PUINT8	pbuf;

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush init entered, size %d\n", sizeof(BTUSB_FLUSH_T));
	pFlush = (PBTUSB_FLUSH_T)vmalloc(sizeof(BTUSB_FLUSH_T));
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_init----allocate mem failed\n");
		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(pFlush, sizeof(BTUSB_FLUSH_T));

	
	pFlush->MasterBakList.DeltaLength = 0;
	pFlush->SlaveBakList.DeltaLength = 0;
	pFlush->MasterBakList.SendingLength = 0;
	pFlush->SlaveBakList.SendingLength = 0;


	//init spin_lock
	KeInitializeSpinLock(&pFlush->FlushLock);

	QueueInitList(&pFlush->MasterBakList.Link);
	QueueInitList(&pFlush->SlaveBakList.Link);
	QueueInitList(&pFlush->FreeNodeList);
	QueueInitList(&pFlush->SwapBakList.Link);

	for(i = 0; i < MAX_NUM_BAK_NODES; i++)
	{
		QueuePutTail(&pFlush->FreeNodeList, &pFlush->FlushNodes[i].Link);
	}

	pFlush->MasterBakList.Ptr_Head = 0; //MAX_LEN_BAKUP_MASTER - 1;
	pFlush->MasterBakList.Ptr_Tail = 0; //MAX_LEN_BAKUP_MASTER - 1;
	pFlush->SlaveBakList.Ptr_Head = 0;//MAX_LEN_BAKUP_SLAVE - 1;
	pFlush->SlaveBakList.Ptr_Tail = 0;//MAX_LEN_BAKUP_SLAVE - 1;

#ifdef BT_SNIFF_SUPPORT
	pFlush->SniffBakList.DeltaLength = 0;
	pFlush->SniffBakList.SendingLength = 0;
	pFlush->SniffBakList.Ptr_Head = 0; 
	pFlush->SniffBakList.Ptr_Tail = 0; 
	QueueInitList(&pFlush->SniffBakList.Link);
#endif

	DevExt->pFlush = pFlush;

	pFlush->MasterBakBuffer = ExAllocatePool(MAX_LEN_BAKUP_MASTER, GFP_KERNEL);
	if(pFlush->MasterBakBuffer == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Init---Failed to alloctte master bakup buffer\n");
		Flush_Release(DevExt);
		return STATUS_UNSUCCESSFUL;
	}
	pFlush->SlaveBakBuffer = ExAllocatePool(MAX_LEN_BAKUP_SLAVE, GFP_KERNEL);
	if(pFlush->SlaveBakBuffer == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Init---Failed to alloctte master bakup buffer\n");
		Flush_Release(DevExt);
		return STATUS_UNSUCCESSFUL;
	}
	pFlush->SwapBuffer = ExAllocatePool(MAX_LEN_BAKUP_SLAVE, GFP_KERNEL);
	if(pFlush->SwapBuffer == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Init---Failed to alloctte master bakup buffer\n");
		Flush_Release(DevExt);
		return STATUS_UNSUCCESSFUL;
	}

#ifdef BT_SNIFF_SUPPORT
	pFlush->SniffBuffer = ExAllocatePool(MAX_LEN_BAKUP_SNIFF, GFP_KERNEL);
	if(pFlush->SniffBuffer == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Init---Failed to alloctte sniff bakup buffer\n");
		Flush_Release(DevExt);
		return STATUS_UNSUCCESSFUL;
	}
#endif

	return STATUS_SUCCESS;
}

VOID Flush_Reset(PBT_DEVICE_EXT DevExt)
{
	PBTUSB_FLUSH_T	pFlush;
	PBTUSB_BAK_LIST_HEAD	pList;
	UINT32			i;
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Reset entered\n");
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_Reset---Null module pointer\n");
		return;
	}

	
	pList = (PBTUSB_BAK_LIST_HEAD)&pFlush->MasterBakList;
	pList->DeltaLength = 0;
	pList->FlushValidFlag = 0;
	pList->Num_Nodes = 0;
	pList->Ptr_Head = pList->Ptr_Tail = 0;
	pList->SendingLength = 0;
	pList->Total_length = 0;

	pList = (PBTUSB_BAK_LIST_HEAD)&pFlush->SlaveBakList;
	pList->DeltaLength = 0;
	pList->FlushValidFlag = 0;
	pList->Num_Nodes = 0;
	pList->Ptr_Head = pList->Ptr_Tail = 0;
	pList->SendingLength = 0;
	pList->Total_length = 0;

#ifdef BT_SNIFF_SUPPORT
	pList = (PBTUSB_BAK_LIST_HEAD)&pFlush->SniffBakList;
	pList->DeltaLength = 0;
	pList->FlushValidFlag = 0;
	pList->Num_Nodes = 0;
	pList->Ptr_Head = pList->Ptr_Tail = 0;
	pList->SendingLength = 0;
	pList->Total_length = 0;
	
	QueueInitList(&pFlush->SniffBakList.Link);
#endif



	QueueInitList(&pFlush->MasterBakList.Link);
	QueueInitList(&pFlush->SlaveBakList.Link);
	QueueInitList(&pFlush->FreeNodeList);
	QueueInitList(&pFlush->SwapBakList.Link);

	for(i = 0; i < MAX_NUM_BAK_NODES; i++)
	{
		QueuePutTail(&pFlush->FreeNodeList, &pFlush->FlushNodes[i].Link);
	}

	pFlush->MasterBakList.Ptr_Head = 0; //MAX_LEN_BAKUP_MASTER - 1;
	pFlush->MasterBakList.Ptr_Tail = 0; //MAX_LEN_BAKUP_MASTER - 1;
	pFlush->SlaveBakList.Ptr_Head = 0;//MAX_LEN_BAKUP_SLAVE - 1;
	pFlush->SlaveBakList.Ptr_Tail = 0;//MAX_LEN_BAKUP_SLAVE - 1;

}


/********************************************************
*Desc:
*	Release the resource of flush
********************************************************/
VOID Flush_Release(PBT_DEVICE_EXT DevExt)
{

	PBTUSB_FLUSH_T	pFlush;

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_release entered\n");

	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush != NULL)
	{
		if(pFlush->MasterBakBuffer)
		{
			ExFreePool((PVOID)pFlush->MasterBakBuffer);
			pFlush->MasterBakBuffer = NULL;
		}
		if(pFlush->SlaveBakBuffer)
		{
			ExFreePool((PVOID)pFlush->SlaveBakBuffer);
			pFlush->SlaveBakBuffer = NULL;
		}
		if(pFlush->SwapBuffer)
		{
			ExFreePool((PVOID)pFlush->SwapBuffer);
			pFlush->SwapBuffer = NULL;
		}

	#ifdef BT_SNIFF_SUPPORT
		if(pFlush->SniffBuffer)
		{
			ExFreePool((PVOID)pFlush->SniffBuffer);
			pFlush->SniffBuffer = NULL;
		}
	#endif
		
		//ExFreePool((PVOID)pFlush);
		vfree(pFlush);
		DevExt->pFlush = NULL;
	}
	return;
}

/********************************************************
*Desc:
*	Entry for process flush event. run in PASSIVE_LEVEL
********************************************************/
NTSTATUS Flush_ProcessFlushInt(PBT_DEVICE_EXT DevExt, PUINT8 pIntBuf)
{
	KIRQL	oldIrql;
	PUINT8	pBuffer;
	UINT8	slaveindex = 0;
	UINT8	ListIndex;
	UINT32	Size;
	UINT32	tmpLength;
	LARGE_INTEGER 			timevalue;
	PBTUSB_BAK_LIST_HEAD		pList;
	PBTUSB_FLUSH_T			pFlush;
	PBTUSB_BAK_LIST_NODE		pNode;			//record the node which caused flush event
	PBTUSB_BAK_LIST_NODE		pTmpNode; 		//used temply
	PCONNECT_DEVICE_T		pConnDev;
	HCBB_PAYLOAD_HEADER_T	HcbbHead;
	PHCBB_PAYLOAD_HEADER_T	ptmpHcbbHead;
	PBT_HCI_T				pHci;
	PBT_FRAG_T				pFrag;
	PBTUSB_FLUSH_INT_BODY	pFlushBody;

	pFlushBody = (PBTUSB_FLUSH_INT_BODY)pIntBuf;
	if(pFlushBody == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt----flush int body is NULL\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt----flush module pointer is null\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	pHci = (PBT_HCI_T)DevExt->pHci;
	if(pHci == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt-----Null pointer of hci module\n");
		return STATUS_UNSUCCESSFUL;
	}

	pFrag = (PBT_FRAG_T)DevExt->pFrag;
	if(pFrag== NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt-----Null pointer of frag module\n");
		return STATUS_UNSUCCESSFUL;
	}

	KeAcquireSpinLock(&pFlush->FlushLock, &oldIrql);

	if(pFlushBody->TxFifo == BT_FLUSH_QUEUE_MASTER)
	{
		ListIndex = FRAG_MASTER_LIST;
		pList = &pFlush->MasterBakList;
		pBuffer = pFlush->MasterBakBuffer;
	}
	else if(pFlushBody->TxFifo == BT_FLUSH_QUEUE_SLAVE)
	{
		ListIndex = FRAG_SLAVE_LIST;
		pList = &pFlush->SlaveBakList;
		pBuffer = pFlush->SlaveBakBuffer;
	}
	else if(pFlushBody->TxFifo == BT_FLUSH_QUEUE_SNIFF)
	{
	#ifdef BT_SNIFF_SUPPORT
		ListIndex = FRAG_SNIFF_LIST;
		pList = &pFlush->SniffBakList;
		pBuffer = pFlush->SniffBuffer;	
	#endif
	}
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt----wrong queue:%d\n", pFlushBody->TxFifo);
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}

	if(pList->FlushValidFlag == 0)
	{
		pList->SendingLength = 0;
	}

	
	RtlCopyMemory(&pList->RetryPacketHead, &pFlushBody->HcbbHeader, sizeof(UINT32));
	Size = pFlushBody->Size;

	RtlCopyMemory(&HcbbHead, &pFlushBody->HcbbHeader, sizeof(HCBB_PAYLOAD_HEADER_T));
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt-----Bakup queue length is %d,  DSP buf length is %d\n", pList->Total_length, Size);
	if(pList->Total_length <= Size)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt---List Delta length is: %d\n", pList->DeltaLength);
		if((pList->Total_length + pList->DeltaLength) <= Size)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt-----Fatal Error,queue length is less than dsp buffer length\n");	
			pFrag->TxDisable_Flag = 0;
			UsbQueryDMASpace(DevExt);
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return STATUS_UNSUCCESSFUL;
		}
	}

	Flush_PrintInfo(DevExt);
	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	while(pNode)
	{
		
		Flush_PrintNodeInfo(pNode);
		if(pList->FlushValidFlag == 0)
		{
			if(((pList->Total_length + pList->DeltaLength) - (pNode->Size +  pNode->FlushInfo.DeltaLength)) <= Size)
			{
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt----get the node. head_ptr is:%d, size is:%d\n", pNode->Ptr_Head, pNode->Size);
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt--payload header is:\n");
				BtPrintBuffer(pBuffer+pNode->Ptr_Head, 4);
				tmpLength = pList->Total_length + pList->DeltaLength - Size;
				if(((pNode->Size + pNode->FlushInfo.DeltaLength) != tmpLength) && (tmpLength > 0))
					Flush_RefreshNode(DevExt, pList, tmpLength, ListIndex);
				break;
			}	
		}
		else
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt--Attention, sending length is: %d\n", pList->SendingLength);
			//if(((pList->SendingLength) - (pNode->Size +  pNode->FlushInfo.DeltaLength)) <= Size)
			if(!pNode->FlushInfo.SendOk)
			{
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt----get the node. head_ptr is:%d, size is:%d\n", pNode->Ptr_Head, pNode->Size);
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt--payload header is:\n");
				BtPrintBuffer(pBuffer+pNode->Ptr_Head, 4);
				break;
			}	
		}
		

		QueueRemoveTail(&pList->Link);
		QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);

		Flush_DelTailNode(DevExt, pNode, ListIndex);
		
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	}
	if(pNode == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt-----we got another unreasonable error here\n");
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
		pFrag->TxDisable_Flag = 0;
		UsbQueryDMASpace(DevExt);
		return STATUS_UNSUCCESSFUL;
	}
	Flush_PrintInfo(DevExt);


	Flush_ResetDeltaLength(DevExt, ListIndex);

#ifdef BT_SNIFF_SUPPORT
	if((ListIndex == FRAG_MASTER_LIST) || ((ListIndex == FRAG_SNIFF_LIST)))
#else
	if(ListIndex == FRAG_MASTER_LIST)
#endif
	{
		pConnDev = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)DevExt->pHci, HcbbHead.am_addr);
	}
	else
	{
		pConnDev = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)DevExt->pHci, HcbbHead.am_addr);
		slaveindex = HcbbHead.slave_index;
	}
	if(pConnDev == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt-----error, can't find connect device\n");
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
		pFrag->TxDisable_Flag = 0;
		UsbQueryDMASpace(DevExt);
		
		return STATUS_UNSUCCESSFUL;
	}


	if((pList->FlushValidFlag == 1) || (HcbbHead.type == BT_ACL_PACKET_DM1))
	{

		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Ready to disconnect device: %2x\n", pConnDev->bd_addr[0]);
		Flush_DelNodes(DevExt, ListIndex, HcbbHead.am_addr);
		if(pList->Num_Nodes != 0)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "FlushProcessFlushInt--have other packets in list, Num: %d\n", pList->Num_Nodes);
			 pList->FlushValidFlag = 1;
		}
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);

		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt---disconnect device:[%2x]\n", pConnDev->bd_addr[0]);
			Hci_StopTimer(pConnDev);
			pConnDev->state = BT_DEVICE_STATE_IDLE;
			pConnDev->status = BT_HCI_ERROR_CONNECTION_TERMINATED_BY_LOCAL_HOST;
            
			Hci_StopAllTimer(pConnDev);
			Hci_InitLMPMembers(DevExt, pConnDev);
		#ifdef BT_SNIFF_SUPPORT
			if((ListIndex == FRAG_MASTER_LIST) || ((ListIndex == FRAG_SNIFF_LIST)))
		#else
			if(ListIndex == FRAG_MASTER_LIST)
		#endif		
			{
				Hci_Clear_AM_Addr(DevExt, pConnDev->am_addr);
				Hci_Write_AM_Connection_Indicator(DevExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnDev->am_addr);	
			}
			else
			{
				
				Hci_Clear_AM_Addr(DevExt, 0);
				Hci_Write_One_Byte(DevExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
				Hci_Write_AM_Connection_Indicator(DevExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
			}
			
			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(DevExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnDev);
			#else
				Hci_Write_Command_Indicator(DevExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif
			Hci_InitTxForConnDev(DevExt, pConnDev);
			Hci_ReleaseScoLink(DevExt, pConnDev);
			Task_HCI2HC_Send_Disconnection_Complete_Event(DevExt, (PUINT8) &pConnDev, 0);

			if(pConnDev->current_role == BT_ROLE_MASTER)
			{
				Hci_Del_Connect_Device_By_AMAddr(pHci, pConnDev->am_addr);	
			}
			else
			{
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ProcessFlushInt---Disconnect slave device: %x\n", pConnDev->bd_addr[0]);
				Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(pHci, pConnDev->am_addr, pConnDev->slave_index);
			}
			

			if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
			{
				DevExt->TxAclSendCount = 0;
				DevExt->StartRetryCount = 0;
				DevExt->EndRetryCount = 0;
				DevExt->TxRetryCount = 0;
				Hci_ClearMainlyCommandIndicator(DevExt);
			}

			//send event 
			timevalue.QuadPart = 0;
			// Just raise the IRQL and process the task sdft in another DPC process.
			tasklet_schedule(&DevExt->taskletRcv);
		}


		#ifdef BT_SCHEDULER_SUPPORT
		Sched_DeleteAllPoll(DevExt);
		Sched_ScanAllLink(DevExt);
		#endif
		
		pFrag->TxDisable_Flag = 0;
		UsbQueryDMASpace(DevExt);
		return STATUS_SUCCESS;
	}


	Flush_SetNewPacketType(DevExt, pConnDev);

	pNode->FlushInfo.PacketType = BT_ACL_PACKET_DM1;
	pNode->FlushInfo.RebuildFlag = 1;
	pNode->FlushInfo.SendOk = FALSE;
	pNode->FlushInfo.pConnDev = (ULONG_PTR)pConnDev;
	Flush_RebuildBakupPacket(DevExt, ListIndex, pNode);
	if(pNode->Ptr_Head != pList->Ptr_Head)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush---Pnode head:%d, plist head:%d\n", pNode->Ptr_Head, pList->Ptr_Head);
		pTmpNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
		while(pTmpNode)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush---Pnode head:%d, plist head:%d\n", pTmpNode->Ptr_Head, pList->Ptr_Head);
			if(pTmpNode->Size == 4)	//poll packet
			{
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush--This is a Poll or NULL frame\n");
				pTmpNode->FlushInfo.SendOk = FALSE;
				pTmpNode->FlushInfo.RebuildFlag = 0;
				pTmpNode->FlushInfo.pConnDev = (ULONG_PTR)pConnDev;
			}
			else
			{
				pTmpNode->FlushInfo.SendOk = FALSE;
				ptmpHcbbHead = (PHCBB_PAYLOAD_HEADER_T)(pBuffer + pTmpNode->Ptr_Head);
				if(ptmpHcbbHead->am_addr == HcbbHead.am_addr)
				{
					pTmpNode->FlushInfo.PacketType = pConnDev->current_packet_type;
					pTmpNode->FlushInfo.RebuildFlag = 1;
					pTmpNode->FlushInfo.pConnDev = (ULONG_PTR)pConnDev;
					Flush_RebuildBakupPacket(DevExt, ListIndex, pTmpNode);
				}	
			}
			
			if(pTmpNode->Ptr_Head == pList->Ptr_Head)
				break;
			pTmpNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pTmpNode->Link);
		}	
	}
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Set flush valid flag\n");
	pList->FlushValidFlag = 1;
	KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Rebuild frag\n");
	Frag_RebuildFrag(DevExt, pConnDev);

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Enable Tx\n");
	pFrag->TxDisable_Flag = 0;
	
	UsbQueryDMASpace(DevExt);
	
	return STATUS_SUCCESS;
}




/**************************************************************************
 *   Flush_SetNewPacketType
 *
 *   Descriptions:
 *      Driver should set new packet type when flush occur.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: the pointer to connect device.
 *   Return Value: 
 *      NONE.
 *************************************************************************/
VOID Flush_SetNewPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice->edr_mode != 0)
	{
	#ifdef BT_3DSP_HI_SPEED
		if (pConnectDevice->hi_speed_mode != 0 && (pConnectDevice->current_packet_type == BT_ACL_PACKET_6DH5 || pConnectDevice->current_packet_type == BT_ACL_PACKET_6DH3))
		{
			UINT8 i;
			
			pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH1;
			if (pConnectDevice->tx_max_slot > BT_MAX_SLOT_5_SLOT)
				pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;
			
			for (i = BT_ACL_PACKET_3DH5; i >= BT_ACL_PACKET_2DH1; i--)
			{
				if (pConnectDevice->EDR_FeatureBit[i] == 1 && pConnectDevice->EDR_PacketBit[i] == 1 && ((PacketSlotFlag[pConnectDevice->tx_max_slot] >> i) & 0x0001) == 1)
				{
					pConnectDevice->current_packet_type = i;
					break;
				}
			}
			
		}
	
	#else	
		if (pConnectDevice->current_packet_type > BT_ACL_PACKET_2DH3)
		{
			if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 1)
				pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH3;
			else if (pConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH1] == 1)
				pConnectDevice->current_packet_type = BT_ACL_PACKET_3DH1;
			else
				pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH1;
		}
		else if (pConnectDevice->current_packet_type > BT_ACL_PACKET_2DH1)
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_2DH1;
		}
		else
		{
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
		}
	#endif
	}
	else
	{
		if (pConnectDevice->current_packet_type > BT_ACL_PACKET_DM3)
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
		else
			pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	}
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SetNewPacketType current_packet_type= 0x%x, edr_mode= %d, packet_type= 0x%x, BD0= 0x%x!\n", pConnectDevice->current_packet_type, pConnectDevice->edr_mode, pConnectDevice->packet_type, pConnectDevice->bd_addr[0]);
	return;
}



BOOLEAN	Flush_HasData_ToSend(PBT_DEVICE_EXT DevExt, PUINT8 pListIndex)
{
	PBTUSB_FLUSH_T			pFlush;


	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----flush module pointer is null\n");
		return FALSE;
	}

	if(pFlush->MasterBakList.FlushValidFlag == 1)
	{
		*pListIndex = FRAG_MASTER_LIST;
		return TRUE;
	}
	else if(pFlush->SlaveBakList.FlushValidFlag == 1)
	{
		*pListIndex = FRAG_SLAVE_LIST;
		return TRUE;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(pFlush->SniffBakList.FlushValidFlag == 1)
	{
		*pListIndex = FRAG_SNIFF_LIST;
		return TRUE;
	}
#endif
	else
	{
		return FALSE;
	}
	
	
}



UINT32 Flush_SendBakup_Packet(PBT_DEVICE_EXT DevExt, UINT8 ListIndex)
{
	KIRQL		oldIrql;
	PUINT8		dest;
	PUINT8		pSrcbuffer;
	PUINT8		pOrgPayloadHead;
	UINT32		IdleSpace;
	UINT32		MaxBufSize;
	UINT8		dataType;
	UINT8		ppid;
	UINT8		PacketType;
	UINT32		length, tmpLen, ConsumedLen = 0;
	NTSTATUS		status;
	LARGE_INTEGER  			timevalue;
	PBTUSB_FLUSH_T			pFlush;
	PBTUSB_BAK_LIST_NODE		pNode;
	PBTUSB_BAK_LIST_HEAD 	pList;
	PBT_FRAG_T				pFrag;
	PHCBB_PAYLOAD_HEADER_T	pHcbbHeader;
	PBTUSB_FLUSH_INFO		pFlushInfo;
	PCONNECT_DEVICE_T		pTmpConnDev = NULL;
	PPAYLOAD_HEADER_SINGLE_SLOT_T	pSingHead;
	PPAYLOAD_HEADER_MULTI_SLOT_T	pMultiHead;
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----flush module pointer is null\n");
		return BT_FRAG_RESOURCE_ERROR;
	}


	pFrag = (PBT_FRAG_T)DevExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----frag module pointer is null\n");
		return BT_FRAG_RESOURCE_ERROR;
	}

	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		pSrcbuffer = pFlush->MasterBakBuffer;
		MaxBufSize = MAX_LEN_BAKUP_MASTER;
		dataType = MAILBOX_DATA_TYPE_MASTER;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		pSrcbuffer = pFlush->SlaveBakBuffer;
		MaxBufSize = MAX_LEN_BAKUP_SLAVE;
		dataType = MAILBOX_DATA_TYPE_SLAVE;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		pSrcbuffer = pFlush->SniffBuffer;
		MaxBufSize = MAX_LEN_BAKUP_SNIFF;
		dataType = MAILBOX_DATA_TYPE_SNIFF;
	}
#endif
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet---Invalid queue index:%d\n", ListIndex);
		return BT_FRAG_RESOURCE_ERROR;
	}

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = pFrag->IdleSpace[ListIndex];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);


	KeAcquireSpinLock(&pFlush->FlushLock, &oldIrql);

	if(pList->FlushValidFlag != 1)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----Flush valid flag not set\n");
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
		return BT_FRAG_RESOURCE_ERROR;
	}

	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	while(pNode)
	{
		if(pNode->Ptr_Head == pList->Ptr_Head)
		{
			if(pNode->FlushInfo.SendOk == TRUE)
			{
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet---No packet need to be sent\n");
				pNode = NULL;
				break;
			}
			else
			{
				break;
			}
		}
		else
		{
			if(pNode->FlushInfo.SendOk == FALSE)
			{
				break;
			}
		}
		
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
	}
	if(pNode == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----empty queue\n");
		pList->FlushValidFlag = 0;
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);

		//triger for next send
		timevalue.QuadPart = -kOneMillisec;
		KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
		return BT_FRAG_LIST_NULL;
	}

	dest = BtGetValidTxPool(DevExt);
	if(dest == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----can't get valid tx pool\n");
		KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
		return BT_FRAG_RESOURCE_ERROR;
	}

	if(pNode->FlushInfo.RebuildFlag == 0)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "This is not rebuilt fragment\n");
		if(IdleSpace <= pNode->Size)
		{
			ReleaseBulkOut1Buf(dest, DevExt);
			
			Frag_TxTimerInstant(DevExt, &timevalue);
			KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);	
			
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return BT_FRAG_RESOURCE_ERROR;	
		}
		if((pNode->Ptr_Head + pNode->Size) >  MaxBufSize)
		{
			tmpLen = MaxBufSize - pNode->Ptr_Head;
			RtlCopyMemory(dest, pSrcbuffer + pNode->Ptr_Head, tmpLen);
			RtlCopyMemory(dest + tmpLen, pSrcbuffer, pNode->Size - tmpLen);
		}
		else
		{
			RtlCopyMemory(dest, pSrcbuffer + pNode->Ptr_Head, pNode->Size);
		}
		
		length = pNode->Size;
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Send bak up packet\n");
		status = BtUsbWriteAll(DevExt, dest, length, dataType, 0);
		if(!NT_SUCCESS(status))
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet-----Error, send data failed\n");
			ReleaseBulkOut1Buf(dest, DevExt);
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return BT_FRAG_RESOURCE_ERROR;
		}
		else
		{
			pNode->FlushInfo.SendOk = TRUE;
			pList->SendingLength += length;
			/*
			QueueRemoveTail(&pList->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
			Flush_DelTailNode(DevExt, pNode->Size, ListIndex);
			*/
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);

			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			pFrag->IdleSpace[ListIndex] -= length;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

			return BT_FRAG_SUCCESS;
		}
		
	}
	else
	{
		pFlushInfo = &pNode->FlushInfo;
		pTmpConnDev = (PCONNECT_DEVICE_T)pFlushInfo->pConnDev;
		ASSERT(pTmpConnDev);
		ConsumedLen = 0;
		pOrgPayloadHead = pSrcbuffer + (pNode->Ptr_Head + sizeof(HCBB_PAYLOAD_HEADER_T))%MaxBufSize;
		if(pFlushInfo->ppid < pFlushInfo->fpid)
		{
			ppid = pFlushInfo->ppid;
			pHcbbHeader = (PHCBB_PAYLOAD_HEADER_T)(dest + ConsumedLen);

			/* Determine packet type by payload length */
			Frag_GetFragPacketType(DevExt, pTmpConnDev, pFlushInfo->TxBlock[ppid].len, &PacketType);
			pHcbbHeader->am_addr = pTmpConnDev->am_addr;
			pHcbbHeader->type = PacketType;
			pHcbbHeader->tx_retry_count = 0;
			pHcbbHeader->slave_index = pTmpConnDev->slave_index;
			pHcbbHeader->master_slave_flag = (pTmpConnDev->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;
			if ((PacketType == BT_ACL_PACKET_DM1) || (PacketType == BT_ACL_PACKET_AUX1) || ((PacketType == BT_ACL_PACKET_DH1) && (pTmpConnDev != NULL) && (pTmpConnDev->edr_mode == 0)))
			{
				pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(dest + ConsumedLen + sizeof(HCBB_PAYLOAD_HEADER_T));
				if (ppid == 0)
				{	
					pSingHead->l_ch = *pOrgPayloadHead & 0x03; /* the first fragment frame */
				}
				else
				{
					pSingHead->l_ch = BT_LCH_TYPE_CON_L2CAP;
				}
				pSingHead->flow = 1;
				pSingHead->length = (UINT8)(pFlushInfo->TxBlock[ppid].len);
				pHcbbHeader->length = pFlushInfo->TxBlock[ppid].len + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);

				//copy to dest buffer
				if((pFlushInfo->TxBlock[ppid].bufpos + pFlushInfo->TxBlock[ppid].len) > MaxBufSize)
				{
					tmpLen = MaxBufSize - pFlushInfo->TxBlock[ppid].bufpos;
					RtlCopyMemory(dest + ConsumedLen+ sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, tmpLen);
					RtlCopyMemory(dest + ConsumedLen+ tmpLen + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), 
						pSrcbuffer, pFlushInfo->TxBlock[ppid].len - tmpLen);
				}
				else
				{
					RtlCopyMemory(dest + ConsumedLen+ sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, pFlushInfo->TxBlock[ppid].len);
				}
			}
			else
			{
				pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)(dest + ConsumedLen + sizeof(HCBB_PAYLOAD_HEADER_T));
				if (ppid == 0)
				{
					pMultiHead->l_ch  = *pOrgPayloadHead & 0x03; /* the first fragment frame */
				}
				else
				{
					pMultiHead->l_ch = BT_LCH_TYPE_CON_L2CAP;
				}
				pMultiHead->flow = 1;
				pMultiHead->undefined = 0;
				pMultiHead->length = (UINT16)(pFlushInfo->TxBlock[ppid].len);
				pHcbbHeader->length = pFlushInfo->TxBlock[ppid].len + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
				
				if((pFlushInfo->TxBlock[ppid].bufpos + pFlushInfo->TxBlock[ppid].len) > MaxBufSize)
				{
					tmpLen = MaxBufSize - pFlushInfo->TxBlock[ppid].bufpos;
					/*
					sc_memory_copy(dest + ConsumedLen+ sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, tmpLen);
					sc_memory_copy(dest + ConsumedLen+ tmpLen + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, tmpLen);
						*/
					RtlCopyMemory(dest + ConsumedLen+ sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PPAYLOAD_HEADER_MULTI_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, tmpLen);
					RtlCopyMemory(dest + ConsumedLen+ tmpLen + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PPAYLOAD_HEADER_MULTI_SLOT_T), 
						pSrcbuffer, pFlushInfo->TxBlock[ppid].len - tmpLen);	
				}
				else
				{
					RtlCopyMemory(dest + ConsumedLen+ sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PPAYLOAD_HEADER_MULTI_SLOT_T), 
						pSrcbuffer + pFlushInfo->TxBlock[ppid].bufpos, pFlushInfo->TxBlock[ppid].len);
				}
			}


			tmpLen = ConsumedLen + pHcbbHeader->length + sizeof(HCBB_PAYLOAD_HEADER_T);
			tmpLen = ALIGNLONGDATALEN(tmpLen);

			/**
			if((IdleSpace <= tmpLen) || (tmpLen > 1028))
			{
				break;
			}
			**/

			pFlushInfo->ppid++;
			ConsumedLen += ALIGNLONGDATALEN(pHcbbHeader->length + sizeof(HCBB_PAYLOAD_HEADER_T));
		}

		if(ConsumedLen == 0)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet----not enough space\n");
			ReleaseBulkOut1Buf(dest, DevExt);
			
			// triger delay timer to send again
			Frag_TxTimerInstant(DevExt, &timevalue);
			KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);	
			
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return BT_FRAG_RESOURCE_ERROR; 
		}

		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Ready send flush data, Lengh: %d\n", ConsumedLen);
		status = BtUsbWriteAll(DevExt, dest, ConsumedLen, dataType, 0);
		if(!NT_SUCCESS(status))
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet-----Error, send data failed\n");
			ReleaseBulkOut1Buf(dest, DevExt);
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return BT_FRAG_RESOURCE_ERROR;
		}
		else
		{
			pNode->FlushInfo.DeltaLength += (UINT16)ConsumedLen;
			pList->SendingLength += ConsumedLen;
			if(pFlushInfo->ppid == pFlushInfo->fpid)
			{
				pNode->FlushInfo.SendOk = TRUE;

				pNode->FlushInfo.DeltaLength -= pNode->Size;
				BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_SendBakup_Packet-----Delta Length is: %d\n", pNode->FlushInfo.DeltaLength);
				pList->DeltaLength += pNode->FlushInfo.DeltaLength;
				/*
				QueueRemoveTail(&pList->Link);
				QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
				Flush_DelTailNode(DevExt, pNode->Size, ListIndex);
				*/
			}
			
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);

			//update idle space
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			pFrag->IdleSpace[ListIndex] -= ConsumedLen;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

			return BT_FRAG_SUCCESS;
		}

		
	}

}


/********************************************************
*Desc:
*	Back up data to buffer. 
*	We should always force this operation success, if space is not enough,
*	drop the oldest data and put in the new. Keep in mind: the newer the data
*	is, the closer to the list head it will be
********************************************************/
NTSTATUS Flush_BakupData(PBT_DEVICE_EXT DevExt, PUINT8 buffer, UINT32 length, UINT8 ListIndex)
{

	PBTUSB_FLUSH_T	pFlush = NULL;
	KIRQL	oldIrql;
	PBTUSB_BAK_LIST_HEAD 	pList;
	PBTUSB_BAK_LIST_NODE		pNode;
	UINT32	MaxBufLen;
	UINT32	OriginHeadPtr;
	PUINT8	pBakBuf;
	UINT32	tmpLong;
	
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_BakupData---error, pflush pointer in null\n");
		return STATUS_UNSUCCESSFUL;
	}
	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
		pBakBuf = pFlush->MasterBakBuffer;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
		pBakBuf = pFlush->SlaveBakBuffer;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "No need to backup sniff data, return\n");
		return STATUS_SUCCESS;
		/*
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
		pBakBuf = pFlush->SniffBuffer;
		*/
	}
#endif
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_BakupData--Error, invalid list index:%d\n",ListIndex);
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pFlush->FlushLock, &oldIrql);
	pNode = (PBTUSB_BAK_LIST_NODE)QueuePopHead(&pFlush->FreeNodeList);
	if(pNode == NULL)
	{
		Flush_TidyUp_UsedList(DevExt);
		pNode = (PBTUSB_BAK_LIST_NODE)QueuePopHead(&pFlush->FreeNodeList);
		if(pNode == NULL)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_BakupData--Fatal error, can't get a bak node\n");
			Flush_PrintInfo(DevExt);
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return STATUS_UNSUCCESSFUL;
		}
	}
	pNode->Ptr_Head = 0;
	pNode->Size = 0;
	pNode->FlushInfo.DeltaLength = 0;
	RtlZeroMemory(&pNode->FlushInfo, sizeof(PBTUSB_FLUSH_INFO));
	
	if((pList->Total_length + length) >= MaxBufLen)
	{
		Flush_TidyUp_UsedList(DevExt);
		if((pList->Total_length + length) >= MaxBufLen)
		{
			BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_BakupData--Fatal error, no free space for bak up\n");
			Flush_PrintInfo(DevExt);
			KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
			return STATUS_UNSUCCESSFUL;
		}
	}
	
	OriginHeadPtr = pList->Ptr_Head;
	Flush_AddHeadNode(DevExt, length, ListIndex);
	if(pList->Ptr_Head > OriginHeadPtr)
	{
		tmpLong = MaxBufLen - pList->Ptr_Head;
		RtlCopyMemory(pBakBuf+pList->Ptr_Head, buffer, tmpLong);
		RtlCopyMemory(pBakBuf, buffer+tmpLong, length-tmpLong);
	}
	else
	{
		RtlCopyMemory(pBakBuf+pList->Ptr_Head, buffer, length);
	}
	pNode->Ptr_Head = pList->Ptr_Head;
	pNode->Size = length;
	QueuePushHead(&pList->Link, &pNode->Link);
	
	KeReleaseSpinLock(&pFlush->FlushLock, oldIrql);
	return STATUS_SUCCESS;
}

/********************************************************
*Desc:
*	update tail ptr when dele a node. For internal call
********************************************************/
VOID Flush_DelTailNode(PBT_DEVICE_EXT DevExt, PBTUSB_BAK_LIST_NODE pNode, UINT8 ListIndex)
{
	PBTUSB_FLUSH_T	pFlush = NULL;
	PBTUSB_BAK_LIST_HEAD	pList = NULL;
	UINT32	MaxBufLen;

	if(pNode == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_DelTailNode---error, pNode is null\n");
		return;
	}
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_DelTailNode---error, pflush pointer is null\n");
		return;
	}



	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
	}	
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
	}
#endif
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_DelTailNode---wrong list index:%d\n", ListIndex);
		return;
	}

	
	pList->Ptr_Tail = (pList->Ptr_Tail + MaxBufLen - pNode->Size) % MaxBufLen;
	pList->Total_length -= pNode->Size;
	pList->Num_Nodes--;
	if(pNode->FlushInfo.DeltaLength != 0)
	{
		pList->DeltaLength -= pNode->FlushInfo.DeltaLength;
	}
	if(pList->Num_Nodes == 0)
	{
		pList->FlushValidFlag = 0;
	}

	
	
}

/********************************************************
*Desc:
*	update head ptr when add a node. For internal call
********************************************************/
VOID Flush_AddHeadNode(PBT_DEVICE_EXT DevExt, UINT32 length, UINT8 ListIndex)
{
	PBTUSB_FLUSH_T	pFlush = NULL;
	PBTUSB_BAK_LIST_HEAD	pList = NULL;

	UINT32	MaxBufLen;

	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_MoveTail_WhenAddNode---error, pflush pointer in null\n");
		return;
	}


	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
	}	
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
	}
#endif	
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_DelTailNode---wrong list index:%d\n", ListIndex);
		return;
	}

	pList->Ptr_Head = (pList->Ptr_Head + MaxBufLen - length) % MaxBufLen;
	pList->Total_length += length;
	pList->Num_Nodes++;

}


VOID Flush_ResetDeltaLength(PBT_DEVICE_EXT DevExt, UINT8 ListIndex)
{
	PBTUSB_BAK_LIST_HEAD		pList, pSwapList;
	PBTUSB_FLUSH_T			pFlush = NULL;
	PBTUSB_BAK_LIST_NODE		pNode;


	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL1, "Flush_DelNodes---error, pflush pointer in null\n");
		return;
	}

	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
	}
#endif
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_ResetDeltaLength---wrong list index:%d\n", ListIndex);
		return;
	}

	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetHead(&pList->Link);
	while(pNode != NULL)
	{
		pNode->FlushInfo.DeltaLength = 0;

		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetNext(&pNode->Link);
	}
	pList->DeltaLength = 0;
	pList->SendingLength = 0;

	
}

VOID Flush_DelNodes(PBT_DEVICE_EXT DevExt, UINT8 ListIndex, UINT8 am_addr)
{

	PUINT8		pBakBuf, pSwapBuf;
	UINT8		NeedBreak = 0;
	UINT32		MaxBufLen, tmpLen;
	PBTUSB_BAK_LIST_HEAD		pList, pSwapList;
	PBTUSB_BAK_LIST_NODE		pNode, pTmpNode;
	PBTUSB_FLUSH_T			pFlush = NULL;
	PHCBB_PAYLOAD_HEADER_T	pHcbbHead;
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_DelNodes---ListIndex:%d, am_addr:%d\n", ListIndex, am_addr);
	
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_DelNodes---error, pflush pointer in null\n");
		return;
	}
	
	
	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
		pBakBuf = pFlush->MasterBakBuffer;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
		pBakBuf = pFlush->SlaveBakBuffer;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
		pBakBuf = pFlush->SniffBuffer;
	}
#endif
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_DelTailNode---wrong list index:%d\n", ListIndex);
		return;
	}

	if(ListIndex == FRAG_MASTER_LIST)
	{
		pSwapBuf = pFlush->SwapBuffer;
		pSwapList = &pFlush->SwapBakList;
		
		pSwapList->FlushValidFlag = pList->FlushValidFlag;
		pSwapList->Num_Nodes = 0;
		pSwapList->Ptr_Head = pSwapList->Ptr_Tail = MaxBufLen - 4;	//Jakio20080926:do not change this value
		pSwapList->Total_length = 0;
		pSwapList->DeltaLength = 0;

		
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		while(pNode)
		{
			if(pNode->Ptr_Head == pList->Ptr_Head)
			{
				NeedBreak = 1;
			}
			pTmpNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
			
			pHcbbHead = (PHCBB_PAYLOAD_HEADER_T)(pBakBuf + pNode->Ptr_Head);
			if(pHcbbHead->am_addr != am_addr)
			{
				pSwapList->Num_Nodes++;
				pSwapList->Ptr_Head = (pSwapList->Ptr_Head + MaxBufLen - pNode->Size)%MaxBufLen;
				pSwapList->Total_length += pNode->Size;
				pSwapList->DeltaLength +=  pNode->FlushInfo.DeltaLength;
				if((pNode->Size + pNode->Ptr_Head) <= MaxBufLen)
				{
					RtlCopyMemory(pSwapBuf + pSwapList->Ptr_Head, pBakBuf + pNode->Ptr_Head, pNode->Size);
				}
				else
				{
					tmpLen = MaxBufLen - pNode->Ptr_Head;
					RtlCopyMemory(pSwapBuf + pSwapList->Ptr_Head, pBakBuf + pNode->Ptr_Head, tmpLen);
					//RtlCopyMemory(pSwapBuf + pSwapList->Ptr_Head + tmpLen, pBakBuf + pNode->Ptr_Head + tmpLen, pNode->Size - tmpLen);
					RtlCopyMemory(pSwapBuf + pSwapList->Ptr_Head + tmpLen, pBakBuf, pNode->Size - tmpLen);

				}
				pNode->Ptr_Head = pSwapList->Ptr_Head;
				pNode->FlushInfo.SendOk = FALSE;
			}
			else
			{
				QueueRemoveEle(&pList->Link,&pNode->Link);
				QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);

			}

			if(NeedBreak)
				break;
			else
				pNode = pTmpNode;
			
		}

		pList->Num_Nodes = pSwapList->Num_Nodes;
		pList->Ptr_Head = pSwapList->Ptr_Head;
		pList->Ptr_Tail = pSwapList->Ptr_Tail;
		pList->Total_length = pSwapList->Total_length;
		pList->DeltaLength = pSwapList->DeltaLength;

		pFlush->MasterBakBuffer = pSwapBuf;
		pFlush->SwapBuffer = pBakBuf;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_DelNodes---Delete all slave nodes in list\n");
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		while(pNode)
		{
			if(pNode->Ptr_Head == pList->Ptr_Head)
			{
				NeedBreak = 1;
			}
			pTmpNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);

			QueueRemoveEle(&pList->Link,&pNode->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);

			if(NeedBreak)
				break;
			else
				pNode = pTmpNode;
		}

		pList->DeltaLength = 0;
		pList->FlushValidFlag = 0;
		pList->Num_Nodes = 0;
		pList->Ptr_Head = pList->Ptr_Tail = MaxBufLen - 4;
		pList->SendingLength = 0;
		pList->Total_length = 0;
		
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_DelNodes---Delete all sniff nodes in list\n");
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		while(pNode)
		{
			if(pNode->Ptr_Head == pList->Ptr_Head)
			{
				NeedBreak = 1;
			}
			pTmpNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);

			QueueRemoveEle(&pList->Link,&pNode->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);

			if(NeedBreak)
				break;
			else
				pNode = pTmpNode;
		}

		pList->DeltaLength = 0;
		pList->FlushValidFlag = 0;
		pList->Num_Nodes = 0;
		pList->Ptr_Head = pList->Ptr_Tail = MaxBufLen - 4;
		pList->SendingLength = 0;
		pList->Total_length = 0;
	}
#endif
	else
	{
	}
	
		
}



VOID Flush_RebuildBakupPacket(PBT_DEVICE_EXT DevExt, UINT8 ListIndex, PBTUSB_BAK_LIST_NODE pNode)
{
	UINT32		curpos;
	UINT32		MaxBufLen;
	PUINT8		pBakBuf;
	UINT16 		reserved_len, current_frag_threshold;
	UINT8  		fpid;
	PBTUSB_FLUSH_T				pFlush = NULL;
	PBTUSB_BAK_LIST_HEAD			pList;
	PHCBB_PAYLOAD_HEADER_T		pHCBBHead;
	PBTUSB_FLUSH_INFO			pFlushInfo;
	PCONNECT_DEVICE_T			pConnDev;

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RebuildBakupPacket entered\n");

	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RebuildBakupPacket---error, pflush pointer in null\n");
		return;
	}
	if(pNode == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RebuildBakupPacket---error, NULL pointer\n");
		return;
	}
	if(pNode->FlushInfo.RebuildFlag != 1)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RebuildBakupPacket---error, rebuild flag not set\n");
		return;
	}


	if(ListIndex == FRAG_MASTER_LIST)
	{
		pList = &pFlush->MasterBakList;
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
		pBakBuf = pFlush->MasterBakBuffer;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		pList = &pFlush->SlaveBakList;
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
		pBakBuf = pFlush->SlaveBakBuffer;
	}	
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
		pBakBuf = pFlush->SniffBuffer;
	}
#endif	
	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RebuildBakupPacket---wrong list index:%d\n", ListIndex);
		return;
	}

	
	pFlushInfo = &pNode->FlushInfo;
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)(pBakBuf + pNode->Ptr_Head);

	//Jakio20090820: check if this packet really need rebuild
	if((pHCBBHead->length == 0) || (pNode->Size == 4))
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush----Error, this packet need not rebuild\n");
		BtPrintBuffer((PUINT8)pHCBBHead, 4);
		pFlushInfo->RebuildFlag = 0;
		return;
	}
	
	if ((pHCBBHead->type == BT_ACL_PACKET_AUX1) || (pHCBBHead->type == BT_ACL_PACKET_DH1 && ((PCONNECT_DEVICE_T)pFlushInfo->pConnDev)->edr_mode == 0))
	{
		curpos = (pNode->Ptr_Head + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T))%MaxBufLen;
		reserved_len = pHCBBHead->length - sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
	}
	else
	{
		curpos = ( pNode->Ptr_Head + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T))%MaxBufLen;
		reserved_len = pHCBBHead->length - sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
	}

	pConnDev = (PCONNECT_DEVICE_T)pFlushInfo->pConnDev;
	if(pConnDev->edr_mode == 0)
		current_frag_threshold = PACKET_MAX_BYTES[pFlushInfo->PacketType];
	else
		current_frag_threshold = EDR_PACKET_MAX_BYTES[pFlushInfo->PacketType];
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Node head: %d, Node Size: %d\n", pNode->Ptr_Head, pNode->Size);
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Current Posision: %d\n", curpos);
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Current Threshold: %d\n", current_frag_threshold);
	pFlushInfo->fpid = 0;
	pFlushInfo->ppid = 0;
	while (1)
	{
		fpid = pFlushInfo->fpid;
		
		if (reserved_len <= current_frag_threshold)
		{
			pFlushInfo->TxBlock[fpid].len    = reserved_len;
			pFlushInfo->TxBlock[fpid].bufpos = curpos;
			
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Element[%d].len = %d, bufpos = %d\n", fpid, pFlushInfo->TxBlock[fpid].len, pFlushInfo->TxBlock[fpid].bufpos);
			
			pFlushInfo->fpid++;
			curpos = (curpos + reserved_len) % MaxBufLen;
			reserved_len = 0;
			break;
		}
		else
		{
			pFlushInfo->TxBlock[fpid].len    = current_frag_threshold;
			pFlushInfo->TxBlock[fpid].bufpos = curpos;
			
			BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Element[%d].len = %d, bufpos = %d\n", fpid, pFlushInfo->TxBlock[fpid].len, pFlushInfo->TxBlock[fpid].bufpos);
			
			pFlushInfo->fpid++;
			curpos = (curpos + current_frag_threshold) % MaxBufLen;
			reserved_len -= current_frag_threshold;
		}
	}
	
	return;
	
}


VOID Flush_RefreshNode(PBT_DEVICE_EXT DevExt, PBTUSB_BAK_LIST_HEAD	pList, UINT32 Length, UINT8 ListIndex)
{	
	PBTUSB_BAK_LIST_NODE	pTailNode;
	PBTUSB_FLUSH_INFO		pFlushInfo;
	PBTUSB_FLUSH_T			pFlush;
	UINT8	i;
	UINT32	tmpLong = 0;
	UINT32	HeaderLen, Useful_len, Res_len, TotalLength;
	UINT32	MaxBufLen;
	PUINT8	pBuf, pStartBuf, pSwapBuf, pTmpBuf;
	UINT32	StartAddr, EndAddr;

	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode entered\n");
	
	if((DevExt == NULL) || (pList == NULL))
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---Null pointer\n");
		return;
	}

	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---Null module pointer\n");
		return;
	}
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---length sending out is: %d\n", Length);
	if(Length <= 0)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---Invalid para\n");
		return;
	}
	
	if(ListIndex == FRAG_MASTER_LIST)
	{
		MaxBufLen = MAX_LEN_BAKUP_MASTER;
		pBuf = pFlush->MasterBakBuffer;
	}
	else if(ListIndex == FRAG_SLAVE_LIST)
	{
		MaxBufLen = MAX_LEN_BAKUP_SLAVE;
		pBuf = pFlush->SlaveBakBuffer;
	}
#ifdef BT_SNIFF_SUPPORT
	else if(ListIndex == FRAG_SNIFF_LIST)
	{
		pList = &pFlush->SniffBakList;
		MaxBufLen = MAX_LEN_BAKUP_SNIFF;
	}
#endif

	else
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Error list\n");
		return;
	}
	pSwapBuf = pFlush->SwapBuffer;

	pTailNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	ASSERT(pTailNode);

	pFlushInfo = &pTailNode->FlushInfo;
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush info of node:\n");
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Packet type = %d, fpid = %d, Delta length = %d\n", pFlushInfo->PacketType, pFlushInfo->fpid, pFlushInfo->DeltaLength);

	if((pTailNode->Size + pFlushInfo->DeltaLength) >= Length)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---Do nothing for this case\n");
		return;
	}


	if((pFlushInfo->PacketType == BT_ACL_PACKET_AUX1) || 
		(pFlushInfo->PacketType == BT_ACL_PACKET_DH1 && ((PCONNECT_DEVICE_T)pFlushInfo->pConnDev)->edr_mode == 0))
	{
		HeaderLen = sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
	}
	else
	{
		HeaderLen = sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
	}

	tmpLong = 0;
	for(i = 0; i < pFlushInfo->fpid; i++)
	{
		tmpLong += ALIGNLONGDATALEN(pFlushInfo->TxBlock[i].len + HeaderLen);
		if(tmpLong > Length)
		{
			i--;
			break;
		}
	}
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---%d packets has been sent out, buf pos is: %d\n", i, pFlushInfo->TxBlock[i].bufpos);

	StartAddr = pFlushInfo->TxBlock[0].bufpos;
	TotalLength = pFlushInfo->TxBlock[pFlushInfo->fpid-1].len + (pFlushInfo->fpid - 1)*(pFlushInfo->TxBlock[0].len);
	Useful_len = TotalLength - (pFlushInfo->TxBlock[i].bufpos + MaxBufLen - StartAddr)%MaxBufLen;
	Res_len = TotalLength - Useful_len;
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_RefreshNode---Total data length is: %d\n", TotalLength);
	if((StartAddr + TotalLength) < MaxBufLen)
	{
		//the easiest case
		RtlCopyMemory(pBuf + StartAddr, pBuf + Res_len, Useful_len);
	}
	else
	{
		if((StartAddr + Res_len) < MaxBufLen)
		{
			tmpLong = MaxBufLen - StartAddr - Res_len;
			RtlCopyMemory(pSwapBuf, pBuf+StartAddr+Res_len, tmpLong);
			RtlCopyMemory(pSwapBuf+tmpLong, pBuf, Useful_len - tmpLong);
		}
		else
		{
			RtlCopyMemory(pSwapBuf, pBuf+((StartAddr+Res_len)%MaxBufLen), Useful_len);
		}

		if((StartAddr + Useful_len) < MaxBufLen)
		{
			RtlCopyMemory(pBuf+StartAddr, pSwapBuf, Useful_len);
		}
		else
		{
			tmpLong = MaxBufLen - StartAddr;
			RtlCopyMemory(pBuf+StartAddr, pSwapBuf, tmpLong);
			RtlCopyMemory(pBuf, pSwapBuf+tmpLong, Useful_len-tmpLong);
		}
	}
	
	

	Flush_PrintInfo(DevExt);
	pTmpBuf = pBuf + (pTailNode->Ptr_Head + sizeof(HCBB_PAYLOAD_HEADER_T))%MaxBufLen;
	*pTmpBuf = (*pTmpBuf &  0xfc) | BT_LCH_TYPE_CON_L2CAP;	//set this l2cap to be a "continued packet"

	pTailNode->Size -= Res_len;

	pList->Total_length -= Res_len;
	pList->Ptr_Tail = (pTailNode->Ptr_Head + pTailNode->Size)%MaxBufLen;
	Flush_PrintInfo(DevExt);
	
}


VOID Flush_TidyUp_UsedList(PBT_DEVICE_EXT DevExt)
{
	PBTUSB_FLUSH_T	pFlush = NULL;
	PBTUSB_BAK_LIST_HEAD	pList = NULL;
	PBTUSB_BAK_LIST_NODE pNode;	
	UINT32	MaxBufLen;
    PBT_HCI_T	pHci = NULL;

	
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
    pHci = (PBT_HCI_T)DevExt->pHci;
	if(pFlush == NULL || (pHci == NULL))
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL3, "Flush_TidyUp_UsedList---error, pflush pointer is null\n");
		return;
	}


	/* jakio20090220: do not delete nodes according to "2 packets limits" policy
*/
	pList = &pFlush->MasterBakList;
	if(pHci->num_device_am == 0)
	{
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		while(pNode != NULL)
		{
			QueueRemoveTail(&pList->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
			Flush_DelTailNode(DevExt, pNode, FRAG_MASTER_LIST);

			/*Get next node*/
			pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		}
	}
	else
	{
		while(pList->Num_Nodes > 2)	
		{		
			pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);		
			QueueRemoveTail(&pList->Link);		
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);		
			Flush_DelTailNode(DevExt, pNode, FRAG_MASTER_LIST);	
		}
		/*
		while(pList->Total_length > BT_MAX_MASTER_IDLE_SPACE_SIZE)
		{
			pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
			if((pList->Total_length - pNode->Size) < BT_MAX_MASTER_IDLE_SPACE_SIZE)
			{
				break;
			}
			QueueRemoveTail(&pList->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
			Flush_DelTailNode(DevExt, pNode, FRAG_MASTER_LIST);
		}
		*/
	}
	pList = &pFlush->SlaveBakList;
	if(pHci->num_device_slave == 0)
	{
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		while(pNode != NULL)
		{
			QueueRemoveTail(&pList->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
			Flush_DelTailNode(DevExt, pNode, FRAG_SLAVE_LIST);

			/*Get next node*/
			pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		}
	}
	else
	{
		while(pList->Total_length > BT_MAX_SLAVE_IDLE_SPACE_SIZE)
		{
			pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
			if((pList->Total_length - pNode->Size) < BT_MAX_SLAVE_IDLE_SPACE_SIZE)
			{
				break;
			}
			QueueRemoveTail(&pList->Link);
			QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
			Flush_DelTailNode(DevExt, pNode, FRAG_SLAVE_LIST);
		}
	}

	
#ifdef BT_SNIFF_SUPPORT
	pList = &pFlush->SniffBakList;
	while(pList->Total_length > BT_MAX_SNIFF_IDLE_SPACE_SIZE)
	{
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
		if((pList->Total_length - pNode->Size) < BT_MAX_SNIFF_IDLE_SPACE_SIZE)
		{
			break;
		}
		QueueRemoveTail(&pList->Link);
		QueuePutTail(&pFlush->FreeNodeList, &pNode->Link);
		Flush_DelTailNode(DevExt, pNode, FRAG_SNIFF_LIST);
	}

#endif


	return;
	
}


VOID Flush_PrintInfo(PBT_DEVICE_EXT DevExt)
{
	UINT32			i;
	PBTUSB_FLUSH_T	pFlush;
	PBTUSB_BAK_LIST_HEAD	pList;
	PBTUSB_BAK_LIST_NODE	pNode;
	pFlush = (PBTUSB_FLUSH_T)DevExt->pFlush;
	if(pFlush == NULL)
		return;

	pList = &pFlush->MasterBakList;
	
	BT_DBGEXT(ZONE_FLUSH | LEVEL4, "MasterBak---node num:%d,   ptr_head:%d,    ptr_tail:%d,   totalLen:%d,   deltaLen:%d\n", 
				pList->Num_Nodes, pList->Ptr_Head, pList->Ptr_Tail, pList->Total_length, pList->DeltaLength);
	i = 0;
	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	while((i < pList->Num_Nodes) && pNode)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL4, "Node %d info:\n", i);
		Flush_PrintNodeInfo(pNode);

		i++;
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
		
	}
	
	pList = &pFlush->SlaveBakList;
	BT_DBGEXT(ZONE_FLUSH | LEVEL4, "SlaveBak---node num:%d,   ptr_head:%d,    ptr_tail:%d,   totalLen:%d\n", pList->Num_Nodes, pList->Ptr_Head, pList->Ptr_Tail, pList->Total_length);
	i = 0;
	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	while((i < pList->Num_Nodes) && pNode)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL4, "Node %d info:\n",i);
		Flush_PrintNodeInfo(pNode);

		i++;
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
		
	}

#ifdef BT_SNIFF_SUPPORT
	pList = &pFlush->SniffBakList;
	BT_DBGEXT(ZONE_FLUSH | LEVEL4, "SniffBak---node num:%d,   ptr_head:%d,    ptr_tail:%d,   totalLen:%d\n", pList->Num_Nodes, pList->Ptr_Head, pList->Ptr_Tail, pList->Total_length);
	i = 0;
	pNode = (PBTUSB_BAK_LIST_NODE)QueueGetTail(&pList->Link);
	while((i < pList->Num_Nodes) && pNode)
	{
		BT_DBGEXT(ZONE_FLUSH | LEVEL4, "Node %d info:\n",i);
		Flush_PrintNodeInfo(pNode);

		i++;
		pNode = (PBTUSB_BAK_LIST_NODE)QueueGetPrior(&pNode->Link);
		
	}
#endif

}

VOID Flush_PrintNodeInfo(PBTUSB_BAK_LIST_NODE pNode)
{
	if(pNode == NULL)
		return;

	BT_DBGEXT(ZONE_FLUSH | LEVEL4, "Pnode size: %d, Pnode header: %d, deltaLen: %d\n", pNode->Size,pNode->Ptr_Head, pNode->FlushInfo.DeltaLength);
	return;
}

