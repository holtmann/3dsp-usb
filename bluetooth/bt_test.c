/****************************************************************
 * FILENAME:     BT_Test.c     CURRENT VERSION: 1.00.01
 * CREATE DATE:  2005/08/15
 * PURPOSE:      All the functions for testing.
 *
 * AUTHORS:       Jason dong
 *
 *
 ****************************************************************/
/*
 * HISTORY OF CHANGES
 *
 *
2005.9.20   Add some functions for testing data from or to socket directly (passby hardware).
Test_RecvFromSockDirectly
Test_GetRxFrameType
Test_TransferRxFrameToRxCach
Test_ProcessRxNULLFrame
Test_ProcessRxPoolFrame
Test_TransferRxFrameToRxLMPPDUCach
Test_CopyTxBufAndNotifyApp
2005.10.21   Add some functions for receiving data from device and sending it to application directily.
Test_GetNextIndex
Test_GetRecvData
Modify function Test_CopyTxBufAndNotifyApp
2005.10.24   Add some functions for displaying received data from driver.
Test_InitShareEventForDisplay
Test_ClrEventForDisplay
function Test_Release
{
...
if (pTest->RxDispEvent)
{
ObDereferenceObject(pTest->RxDispEvent);
pTest->RxDispEvent = NULL;
}
pTest->hRxDispEvent = NULL;
...
}
function Test_SendTestData
{
...
pMultiHead->l_ch = BT_LCH_TYPE_START_L2CAP;
...
}
function Test_CopyTxBufAndNotifyApp
{
...
if (pTest->RxDispEvent != NULL)
{
// Notify the application to load the data
KeSetEvent(pTest->RxDispEvent,0,FALSE);
}
}
2005.10.26  Change some codes for notify application to receive data.
function Test_CopyTxBufAndNotifyApp
{
...
pTest->RxBlock[pTest->curIndex].len = (len < 4) ? 0 : (len - 4);
RtlCopyMemory(pTest->RxBlock[pTest->curIndex].buf, buf + 8, pTest->RxBlock[pTest->curIndex].len);
...
}
2005.11.4 Restore the codes for notify application to receive data.
function Test_CopyTxBufAndNotifyApp
{
...
//pTest->RxBlock[pTest->curIndex].len = (len < 4) ? 0 : (len - 4);
//RtlCopyMemory(pTest->RxBlock[pTest->curIndex].buf, buf + 8, pTest->RxBlock[pTest->curIndex].len);
RtlCopyMemory(pTest->RxBlock[pTest->curIndex].buf, buf + 4, pTest->RxBlock[pTest->curIndex].len);
...
}
2005.12.28 Because some register is adjusted and some is deleted, driver should delete the operation
about the deleted registers
function Test_ProcessRxPoolFrame
{
...
}
2006.3.7  Add some source code for supporting record-vox-2-file and play-vox-from-file function
function Test_Init
{
...
#ifdef BT_RECORD_VOX_TO_FILE
// Initialize record flag
pTest->recordflag = 0;
pTest->mempos = 0;
pTest->mem = ExAllocatePool(NonPagedPool, MAX_VOX_MEM_LEN);
if (pTest->mem == NULL)
{
BT_DBGEXT(ZONE_TEST | LEVEL1,  "Allocate vox mem failed!\n");
}
#endif
...
}
function Test_Release
{
...
#ifdef BT_RECORD_VOX_TO_FILE
// Initialize record flag
pTest->recordflag = 0;
pTest->mempos = 0;
if (pTest->mem != NULL)
ExFreePool((PVOID)pTest->mem);
#endif
...
}
Add new function: Test_StartRecordVoxFile, Test_StoptRecordVoxFile, Test_RecordVox2File, Test_ReadVoxMemCount,
Test_ReadVoxMemBuf, Test_WriteVoxMemBuf, Test_PlayVoxFromFile
2006.3.28  Add some source code for supporting record-vox-2-file
function Test_Init
{
...
pTest->mempos1 = 0;
pTest->mem1 = ExAllocatePool(NonPagedPool, MAX_VOX_MEM_LEN / 2);
if (pTest->mem1 == NULL)
{
BT_DBGEXT(ZONE_TEST | LEVEL1,  "Allocate vox mem1 failed!\n");
}
...
}
function Test_Release
{
...
pTest->mempos1 = 0;
if (pTest->mem1 != NULL)
ExFreePool((PVOID)pTest->mem1);
...
}
function Test_StartRecordVoxFile
{
...
pTest->mempos1 = 0;
...
}
Add three new functions: Test_RecordVox2FileAid, Test_ReadVoxMemCountAid, Test_ReadVoxMemBufAid
2006.6.30 Found a bug. In the old version, driver only search master list to find a connect device by am address. The
new version fixed this bug. Driver should search not only master list but also slave list to find a connect
device by ad address.
function Test_ProcessRxNULLFrame
{
...
if (pHci->role == BT_ROLE_MASTER)
{
// Find connect device by am address
pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
// Find slave connect device by am address
if (pConnectDevice == NULL)
pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
}
else
{
// Find slave connect device by am address
pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
// Find connect device by am address
if (pConnectDevice == NULL)
pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
}
...
}
 */
#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include some hci operation
#include "bt_task.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
/*--file local variables-----------------------------------------------*/
/*--file local function prototypes-------------------------------------*/
/**************************************************************************
 *   Test_Init
 *
 *   Descriptions:
 *      Initialize test module, including alloc memory for the test module and
 *		so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Test module memory is allocated with succesfully
 *      STATUS_UNSUCCESSFUL. Test module   memory is allocated with fail
 *************************************************************************/
NTSTATUS Test_Init(PBT_DEVICE_EXT devExt)
{
	PBT_TEST_T pTest;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_Init\n");
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_Init devExt = 0x%x\n", devExt);
	/* Alloc memory for test module */
	pTest = (PBT_TEST_T)ExAllocatePool(sizeof(BT_TEST_T), GFP_KERNEL);
	if (pTest == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_TEST | LEVEL0,  "Allocate test memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	/* Save task module pointer into device extention context */
	devExt->pTest = (PVOID)pTest;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_Init Test = 0x%x, size = %d\n", pTest, sizeof(BT_TEST_T));
	/* Zero out the test module space */
	RtlZeroMemory(pTest, sizeof(BT_TEST_T));
	// Initialize rx block index
	pTest->curIndex = pTest->lastIndex = 0;
	/* Alloc spin lock,which used to protect test rx operator */
	KeInitializeSpinLock(&pTest->lock);

	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Test_Release
 *
 *   Descriptions:
 *      Release test module, including free memory for the test module.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Task module memory is released with succesfully
 *      STATUS_UNSUCCESSFUL. Task module  memory is released with fail
 *************************************************************************/
NTSTATUS Test_Release(PBT_DEVICE_EXT devExt)
{
	PBT_TEST_T pTest;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_Release\n");
	/*Get pointer to the test */
	pTest = (PBT_TEST_T)devExt->pTest;
	if (pTest == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_Release devExt = 0x%x, pTest = 0x%x\n", devExt, pTest);
	if (pTest->RxEvent)
	{
		ObDereferenceObject(pTest->RxEvent);
		pTest->RxEvent = NULL;
	}
	pTest->hRxEvent = NULL;
	if (pTest->RxDispEvent)
	{
		ObDereferenceObject(pTest->RxDispEvent);
		pTest->RxDispEvent = NULL;
	}
	pTest->hRxDispEvent = NULL;
	/*Free the test module memory */
	if (pTest != NULL)
	{
		ExFreePool((PVOID)pTest);
	}
	devExt->pTest = NULL;
	return (STATUS_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_SendTestData
//
//    This function send testing frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    pLmpPdu - the pointer to source frame.
//    PduLength - the length of source frame.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    STATUS_SUCCESS   process with successful
//    STATUS_xxx       other status with unsuccessful
//
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS Test_SendTestData(PBT_DEVICE_EXT devExt, PUINT8 pLmpPdu, UINT8 PduLength)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PPAYLOAD_HEADER_MULTI_SLOT_T pMultiHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	UINT32 reallen;
	// Take out the Write list lock, since we'll insert this IRP
	// onto the write queue
	//
	KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
	//
	// If there is no BD to store the converted frame, we must return STATUS_PENDING
	// and pend this IRP
	if (BtIsTxPoolFull(devExt))
	{
		KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
		return STATUS_PENDING;
	}
	// Get destnation pointer
	//	dest = (PUINT8)devExt->TxBd[devExt->TxPid].VirtAddr;
	dest = BtGetValidTxPool(devExt);
	if (dest == NULL)
	{
		return STATUS_PENDING;
	}
	//
	// Process LMPDU frame (convert the source frame to the destination frame)
	//
	// Fill hcbb payload header
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = 1;
	pHCBBHead->type = BT_ACL_PACKET_DM1;
	pHCBBHead->tx_retry_count = 0;
	pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)(dest + sizeof(HCBB_PAYLOAD_HEADER_T));
	// pMultiHead->l_ch = BT_LCH_TYPE_LMP;
	pMultiHead->l_ch = BT_LCH_TYPE_START_L2CAP;
	pMultiHead->flow = 1;
	pMultiHead->length = PduLength;
	pHCBBHead->length = pMultiHead->length + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
	RtlCopyMemory(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), pLmpPdu, PduLength);
	reallen = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
	//	// Save the information filed of BD
	//	devExt->TxBd[devExt->TxPid].Info = reallen;
	//
	//	// Valid this BD
	//	devExt->TxBd[devExt->TxPid].Valid = 1;
	//
	//	// Save the Irp field
	//	devExt->TxBd[devExt->TxPid].Irp = 0;
	//
	//	// Save the ConnectionHandle field
	//	devExt->TxBd[devExt->TxPid].ConnectionHandle = 0;
	//
	//	// Transfer the host-used-bd to the real bd (write 8 bytes register). We do DWORDs transfer in order to save the bandwidth
	//	// Move the pid to next id
	//	devExt->TxPid = BtGetNextBDId(devExt->TxPid);
	//	// Update HostPptr
	// 	BtWriteToReg(devExt,BT_HOST_P_PTR_REG,BtGetBDAddrById(devExt->TxPid));
	KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_SendPagePoll
//
//    This function send paging poll frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    STATUS_SUCCESS   process with successful
//    STATUS_xxx       other status with unsuccessful
//
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS Test_SendPagePoll(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	UINT32 reallen;
	// Take out the Write list lock, since we'll insert this IRP
	// onto the write queue
	//
	KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
	//
	// If there is no BD to store the converted frame, we must return STATUS_PENDING
	// and pend this IRP
	if (BtIsTxPoolFull(devExt))
	{
		KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
		return STATUS_PENDING;
	}
	// Get destnation pointer
	//	dest = (PUINT8)devExt->TxBd[devExt->TxPid].VirtAddr;
	dest = BtGetValidTxPool(devExt);
	if (dest == NULL)
	{
		return STATUS_PENDING;
	}
	//
	// Process LMPDU frame (convert the source frame to the destination frame)
	//
	// Fill hcbb payload header
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = 1;
	pHCBBHead->type = BT_ACL_PACKET_POLL;
	pHCBBHead->length = 0;
	pHCBBHead->tx_retry_count = 0;
	reallen = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
	//	// Save the information filed of BD
	//	devExt->TxBd[devExt->TxPid].Info = reallen;
	//
	//	// Valid this BD
	//	devExt->TxBd[devExt->TxPid].Valid = 1;
	//
	//	// Save the Irp field
	//	devExt->TxBd[devExt->TxPid].Irp = 0;
	//
	//	// Save the ConnectionHandle field
	//	devExt->TxBd[devExt->TxPid].ConnectionHandle = 0;
	//
	//	// Transfer the host-used-bd to the real bd (write 8 bytes register). We do DWORDs transfer in order to save the bandwidth
	//	// Move the pid to next id
	//	devExt->TxPid = BtGetNextBDId(devExt->TxPid);
	//	// Update HostPptr
	// 	BtWriteToReg(devExt,BT_HOST_P_PTR_REG,BtGetBDAddrById(devExt->TxPid));
	KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_SendPagePoll
//
//    This function send BB ack frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    STATUS_SUCCESS   process with successful
//    STATUS_xxx       other status with unsuccessful
//
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS Test_SendBBAck(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	UINT32 reallen;
	// Take out the Write list lock, since we'll insert this IRP
	// onto the write queue
	//
	KeAcquireSpinLock(&devExt->WriteQueueLock, &oldIrql);
	//
	// If there is no BD to store the converted frame, we must return STATUS_PENDING
	// and pend this IRP
	if (BtIsTxPoolFull(devExt))
	{
		KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
		return STATUS_PENDING;
	}
	// Get destnation pointer
	//	dest = (PUINT8)devExt->TxBd[devExt->TxPid].VirtAddr;
	dest = BtGetValidTxPool(devExt);
	if (dest == NULL)
	{
		return STATUS_PENDING;
	}
	//
	// Process LMPDU frame (convert the source frame to the destination frame)
	//
	// Fill hcbb payload header
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = 1;
	pHCBBHead->type = BT_ACL_PACKET_NULL;
	pHCBBHead->length = 0;
	pHCBBHead->tx_retry_count = 0;
	reallen = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
	//	// Save the information filed of BD
	//	devExt->TxBd[devExt->TxPid].Info = reallen;
	//
	//	// Valid this BD
	//	devExt->TxBd[devExt->TxPid].Valid = 1;
	//
	//	// Save the Irp field
	//	devExt->TxBd[devExt->TxPid].Irp = 0;
	//
	//	// Save the ConnectionHandle field
	//	devExt->TxBd[devExt->TxPid].ConnectionHandle = 0;
	//
	//	// Transfer the host-used-bd to the real bd (write 8 bytes register). We do DWORDs transfer in order to save the bandwidth
	//	// Move the pid to next id
	//	devExt->TxPid = BtGetNextBDId(devExt->TxPid);
	//	// Update HostPptr
	// 	BtWriteToReg(devExt,BT_HOST_P_PTR_REG,BtGetBDAddrById(devExt->TxPid));
	KeReleaseSpinLock(&devExt->WriteQueueLock, oldIrql);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_RecvFromSockDirectly
//
//    This function receive frame directly from socket.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//      len - the length of the frame
//
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//      None.
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_RecvFromSockDirectly(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	KIRQL oldIrql;
	LARGE_INTEGER timevalue;
	PUINT8 dest;
	BT_RX_FRAME_TYPE tmprxtype;
	UINT8 tmptype;
	//
	// Get the Read Queue lock, so we can insert our IRP
	//
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	//
	// Get frame type. If the type is data type, we must cache the frame into rx cach. Otherwise
	// if the type is LMPPDU type, we must cache the frame into RxLMPPDUCach. Then we should
	// process  the cach
	//
	tmprxtype = Test_GetRxFrameType(devExt, buf, len);
	if (tmprxtype == RX_FRAME_TYPE_DATA)
	{
		// If rx cach full, we must pending rx process and set a timer to check it again,
		if (BtIsRxCachFull(devExt))
		{
			//
			// Setup sending tx frame timer whose perodic is 1ms
			//
			timevalue.QuadPart =  (kOneMillisec);
			tasklet_schedule(&devExt->taskletRcv);
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "Rx int but rx cach is full!\n");
			return ;
		}
	
		// Get destination rx cach address which is used to store the rx frame.
		dest = devExt->RxCach[devExt->RxCachPid];
		// Copy frame from rx buffer to rx cach
		Test_TransferRxFrameToRxCach(devExt, buf, len, dest);
		// Move the rx cach pid to next id.
		devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
#if DBG
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "receive real data:\n");
		//BtPrintBuffer(buf, len);
#endif
		//
		// We're done playing with the read queue now
		//
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		//KeAcquireSpinLock(&devExt->ThreadSpinLock, &oldIrql);
		//tmptype = (*(dest + sizeof(UINT32)) &0x78) >> 3;
		// DM1 or DH1
		//if ((tmptype == BT_ACL_PACKET_DM1) || (tmptype == BT_ACL_PACKET_DH1))
		{
			//devExt->SerialInfo.SerialStatus.AmountInInQueue += (1+sizeof(HCI_DATA_HEADER_T) + ((*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T)) &0xf8) >> 3));
		}
		//else
		{
			//devExt->SerialInfo.SerialStatus.AmountInInQueue += (1+sizeof(HCI_DATA_HEADER_T) + ((*(PUINT16)(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T)) &0x0ff8) >> 3));
		}
		//KeReleaseSpinLock(&devExt->ThreadSpinLock, oldIrql);
		//
		// Get the Read Queue lock, so we can insert our IRP
		//
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
		//
		// We can use this timer to complete IRP if it exists some pending IRP
		//
		timevalue.QuadPart = 0; // Just raise the IRQL and process the task immudiatly in another DPC process.
		tasklet_schedule(&devExt->taskletRcv);
	}
	else if (tmprxtype == RX_FRAME_TYPE_NULL)
	// NULL frame
	{
		//
		// We're done playing with the read queue now
		//
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		//
		// Process BB ACK condition
		// Add codes here...
		Test_ProcessRxNULLFrame(devExt, buf);
		//
		// Get the Read Queue lock, so we can insert our IRP
		//
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	}
	else if (tmprxtype == RX_FRAME_TYPE_POLL)
	// POLL frame
	{
		//
		// We're done playing with the read queue now
		//
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		//
		// Process BB Master Pool Packet
		//
		Test_ProcessRxPoolFrame(devExt, buf);
		//
		// Get the Read Queue lock, so we can insert our IRP
		//
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	}
	else
	{
		// Copy frame from rx buffer to rx cach
		Test_TransferRxFrameToRxLMPPDUCach(devExt, buf, len);
		//
		// We're done playing with the read queue now
		//
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		// Process LMPPDU
		//jakiotodo: temply mark here
		//BtProcessRxLMPPDU(devExt);
		//
		// Get the Read Queue lock, so we can insert our IRP
		//
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	}
	
	//
	// We're done playing with the read queue now
	//
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_GetRxFrameType
//
//    Get the rx frame's type
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//      len - the length of the frame
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		RX_FRAME_TYPE_DATA if the type of frame is data
//      RX_FRAME_TYPE_LMPPDU if the type of frame is LMPPDU
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BT_RX_FRAME_TYPE Test_GetRxFrameType(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	UINT32 tmplong;
	UINT8 type;
	UINT8 l_ch;
	//
	// Add code here...
	//
	// Get L_CH field
	tmplong = *(PUINT32)buf;
	type = (UINT8)((tmplong &0x78) >> 3);
	if (type == (UINT8)BT_ACL_PACKET_NULL)
	{
		return RX_FRAME_TYPE_NULL;
	}
	else if (type == (UINT8)BT_ACL_PACKET_POLL)
	{
		return RX_FRAME_TYPE_POLL;
	}
	l_ch = (UINT8)((tmplong >> 16) &0x3);
	if (l_ch == (UINT8)BT_LCH_TYPE_LMP)
	{
		return RX_FRAME_TYPE_LMPPDU;
	}
	return RX_FRAME_TYPE_DATA;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_TransferRxFrameToRxCach
//
//    Copy the frame in rx buffer into the rx cach
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//      len - the length of the frame
//      destbuf - rx cach buffer address
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_TransferRxFrameToRxCach(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len, PUINT8 destbuf)
{
	PUINT8 tmpbuf = destbuf;
	// We save the data length into the first word of rx cach. So the first step is record the data length
	*(PUINT16)tmpbuf = (UINT16)len;
	tmpbuf += sizeof(UINT16);
	// We save the frame type (data or evnet) into the second word of rx cach.
	*(PUINT16)tmpbuf = (UINT16)RX_FRAME_TYPE_DATA;
	// Move the destination buffer pointer to next dword.
	tmpbuf += sizeof(UINT16);
	// Copy memory
	RtlCopyMemory(tmpbuf, buf, len);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_ProcessRxNULLFrame
//
//    Process the received NULL frame
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_ProcessRxNULLFrame(PBT_DEVICE_EXT devExt, PUINT8 buf)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 tmpchar;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_ProcessRxNULLFrame Enter!\n");
	// Get pointer of hci module
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	// Get the first byte of rx frame
	tmpchar =  *buf;
	// Get am_addr
	tmpchar &= 0x7;
	if (pHci->role == BT_ROLE_MASTER)
	{
		// Find connect device by am_addr
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
		// Find slave connect device by am_addr
		if (pTempConnectDevice == NULL)
		{
			pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
		}
	}
	else
	{
		// Find slave connect device by am_addr
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
		// Find connect device by am_addr
		if (pTempConnectDevice == NULL)
		{
			pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
		}
	}
	if (pTempConnectDevice != NULL)
	{
		switch (pTempConnectDevice->state)
		{
			case (BT_DEVICE_STATE_PAGING):
			// Change the hci command state
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			Hci_StopTimer(pTempConnectDevice);
			if (pTempConnectDevice->tempflag == 1)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_NAME_REQ;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_NAME_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_PAGED;
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				// Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_CREATE_CONNECTION), BT_TASK_PRI_NORMAL, (PUINT8)&pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
				//Changed by jason 2005.9.13 system moudle should notify LMP module to send features_req first.
				// Send event to LMP module
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_FEATURE_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			case (BT_DEVICE_STATE_DETACH):
			// Change the hci command state
			//KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// May be delay several milliseconds
			pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT;
			// Start initial timer for 3 Tpoll
			Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *3 * 625 / 5000) + 1));
			//KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
			case (BT_DEVICE_STATE_CONNECTED): case (BT_DEVICE_STATE_SLAVE_CONNECTED):
			// Change the hci command state
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
			{
				// Restart link supervision timer (timer value is second level: value = pTempConnectDevice->link_supervision_timeout * 0.625 / 1000
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
			}
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			//
			// Add other codes here
			//
			break;
			case (BT_DEVICE_STATE_DISCONNECT):
			// Change the hci command state
			//KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// May be delay several milliseconds
			pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN;
			// Start initial timer for 3 Tpoll
			Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *3 * 625 / 5000) + 1));
			//KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_ProcessRxPoolFrame
//
//    Process the received Pool frame
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_ProcessRxPoolFrame(PBT_DEVICE_EXT devExt, PUINT8 buf)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 tmpchar;
	PCONNECT_DEVICE_T pTempConnectDevice;
	NTSTATUS status;
	UINT8 am_addr;
	UINT8 bd_addr[BT_BD_ADDR_LENGTH];
	UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH]; // add by jason 2005.9.19. We'd better save this vars at the same time.
	UINT16 packet_type;
	UINT8 repetition_mode;
	UINT8 scan_mode;
	UINT16 clock_offset;
	UINT8 role_switch;
	UINT8 Datastr[8];
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_ProcessRxPoolFrame Enter!\n");
	// Get pointer of hci module
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	// Get the first byte of rx frame
	tmpchar =  *buf;
	// Get am_addr
	tmpchar &= 0x7;
	// Find connect device by am_addr
	pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
	if (pTempConnectDevice != NULL)
	{
		switch (pTempConnectDevice->state)
		{
			case (BT_DEVICE_STATE_PAGING): break;
			case (BT_DEVICE_STATE_DETACH): break;
			case (BT_DEVICE_STATE_CONNECTED): case (BT_DEVICE_STATE_SLAVE_CONNECTED):
			// Change the hci command state
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
			{
				// Restart link supervision timer (timer value is second level: value = pTempConnectDevice->link_supervision_timeout * 0.625 / 1000
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
			}
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
		}
	}
	else
	{
		am_addr = Hci_Read_Byte_From_3DspReg(devExt, BT_REG_AM_ADDR, 0);
		RtlZeroMemory(Datastr, 8);
		UsbReadFrom3DspRegs(devExt, BT_REG_AM_BD_ADDR, 2, Datastr);
		RtlCopyMemory(bd_addr, Datastr, BT_BD_ADDR_LENGTH);
		// add by jason 2005.9.19. We'd better save this vars at the same time.
		packet_type = BT_PACKET_TYPE_DM1;
		role_switch = 0;
		// Change by jason 2005.9.19. We'd better save "class_of_device " member at the same time.
		// status = Hci_Add_Slave_Connect_Device(pHci, am_addr, bd_addr, packet_type, repetition_mode, scan_mode, clock_offset, role_switch);
		// status = Hci_Add_Slave_Connect_Device(pHci, am_addr, bd_addr, class_of_device, packet_type, repetition_mode, scan_mode, clock_offset, role_switch);
		status = STATUS_SUCCESS;
		if (status == STATUS_SUCCESS)
		{
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			pHci->pcurrent_connect_device->state = BT_DEVICE_STATE_WAIT_CONN_REQ;
			Hci_StartTimer(pHci->pcurrent_connect_device, BT_TIMER_TYPE_CONN_ACCEPT, (UINT16)(((UINT32)pHci->conn_accept_timeout *625) / (1000 *1000) + 1));
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_TransferRxFrameToRxLMPPDUCach
//
//    Copy the frame in rx buffer into the LMPPDU cach
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//      len - the length of the frame
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_TransferRxFrameToRxLMPPDUCach(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	//jakiotodo: temply mark here
	#if 0
	PUINT8 tmpbuf = devExt->RxLMPPDUCach;
	// We save the data length into the first dword of rx cach. So the first step is record the data length/
	*(PUINT32)tmpbuf = len;
	// Move the destination buffer pointer to next dword.
	tmpbuf += sizeof(UINT32);
	// Copy memory
	RtlCopyMemory(tmpbuf, buf, len);
	#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_CopyTxBufAndNotifyApp
//
//    Copy the frame in into tx buffer between application and driver. Then
//    notify application to get data from driver.
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//      len - the length of the frame
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID Test_CopyTxBufAndNotifyApp(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	PBT_TEST_T pTest;
	KIRQL oldIrql;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_CopyTxBufAndNotifyApp enter\n");
	/*Get pointer to the test */
	pTest = (PBT_TEST_T)devExt->pTest;
	if (pTest == NULL)
	{
		return ;
	}
	/* Lock */
	KeAcquireSpinLock(&pTest->lock, &oldIrql);
	if (Test_GetNextIndex(pTest->curIndex) == pTest->lastIndex)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL1,  "Rx block is full!\n");
		/* Unlock */
		KeReleaseSpinLock(&pTest->lock, oldIrql);
		return ;
	}
	pTest->RxBlock[pTest->curIndex].len = (len < 512) ? len : 512;
	// pTest->RxBlock[pTest->curIndex].len = (len < 4) ? 0 : (len - 4);
	// RtlCopyMemory(pTest->RxBlock[pTest->curIndex].buf, buf + 8, pTest->RxBlock[pTest->curIndex].len);
	RtlCopyMemory(pTest->RxBlock[pTest->curIndex].buf, buf + 4, pTest->RxBlock[pTest->curIndex].len);
	pTest->curIndex = Test_GetNextIndex(pTest->curIndex);
	/* Unlock */
	KeReleaseSpinLock(&pTest->lock, oldIrql);
	if (pTest->RxDispEvent != NULL)
	{
		// Notify the application to load the data
		KeSetEvent(pTest->RxDispEvent, 0, FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_LoadOneFileToDSP
//
//    This function load one file and send the content into DSP.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      memport - Memory port P, A, B or E memory
//      filename - the name of file to be loaded.
//
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    STATUS_SUCCESS   process with successful
//    STATUS_xxx       other status with unsuccessful
//
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS Test_LoadOneFileToDSP(PBT_DEVICE_EXT devExt, UINT8 memport, PCSZ filename)
{
	OBJECT_ATTRIBUTES InitiaObject;
	UNICODE_STRING ObjectName;
	ANSI_STRING AnsiString;
	HANDLE FileHandle;
	SIZE_T ViewSize;
	IO_STATUS_BLOCK IoStatus;
	FILE_STANDARD_INFORMATION fileinfo;
	PUINT8 tmpbuf;
	NTSTATUS code1 = STATUS_SUCCESS;
	// RtlInitAnsiString(&AnsiString, "\\DosDevices\\d:\\temp\\bt2000_0829\\res\\dsp_Pmem.bin");
	RtlInitAnsiString(&AnsiString, filename);
	RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, TRUE);
	InitializeObjectAttributes(&InitiaObject, &ObjectName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	code1 = ZwOpenFile(&FileHandle, FILE_OPEN_IF | SYNCHRONIZE, &InitiaObject, &IoStatus, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(code1))
	{
		BT_DBGEXT(ZONE_TEST | LEVEL1,  "ZwOpenFile %s error code = 0x%x\n", filename, code1);
		RtlFreeUnicodeString(&ObjectName);
		return STATUS_UNSUCCESSFUL;
	}
	code1 = ZwQueryInformationFile(FileHandle, &IoStatus, &fileinfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
	if (!NT_SUCCESS(code1))
	{
		BT_DBGEXT(ZONE_TEST | LEVEL1,  "ZwQueryInformationFile %s error code = 0x%x\n", filename, code1);
		RtlFreeUnicodeString(&ObjectName);
		ZwClose(FileHandle);
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "AllocateSize= 0x%x, EndOfFile = 0x%x, NumberOfLinks = 0x%x, DeletePending = %d, Directory = %d\n", fileinfo.AllocationSize.LowPart, fileinfo.EndOfFile.LowPart, fileinfo.NumberOfLinks, fileinfo.DeletePending, fileinfo.Directory);
	tmpbuf = (PUINT8)ExAllocatePool(fileinfo.EndOfFile.LowPart, GFP_KERNEL);
	if (tmpbuf == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_TEST | LEVEL0,  "Allocate non page memory for file %s fail!\n", filename);
		RtlFreeUnicodeString(&ObjectName);
		ZwClose(FileHandle);
		return STATUS_UNSUCCESSFUL;
	}
	code1 = ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatus, (PVOID)tmpbuf, fileinfo.EndOfFile.LowPart, NULL, NULL);
	if (!NT_SUCCESS(code1))
	{
		BT_DBGEXT(ZONE_TEST | LEVEL0,  "ZwReadFile error for file %s code = 0x%x\n", filename, code1);
		ExFreePool(tmpbuf);
		RtlFreeUnicodeString(&ObjectName);
		ZwClose(FileHandle);
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "input byte = %d, return bytes = %d\n", fileinfo.EndOfFile.LowPart, IoStatus.Information);
	if (fileinfo.EndOfFile.LowPart != IoStatus.Information)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL1,  "input bytes is not equal to return bytes for file %s\n", filename);
		ExFreePool(tmpbuf);
		RtlFreeUnicodeString(&ObjectName);
		ZwClose(FileHandle);
		return STATUS_UNSUCCESSFUL;
	}
#if DBG
	// BtPrintBuffer(tmpbuf, fileinfo.EndOfFile.LowPart);
#endif
	// Load SP20 code - resource files saved in SP20Code buffer
	code1 = BtScratch_2_DSP_MemoryBank(devExt, tmpbuf, fileinfo.EndOfFile.LowPart, memport);
	if (code1 != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL0,  "file %s load failed!\n", filename);
	}
	ExFreePool(tmpbuf);
	RtlFreeUnicodeString(&ObjectName);
	ZwClose(FileHandle);
	return (code1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_LoadFilesToDSP
//
//    This function load the contents of the total four files into DSP.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    STATUS_SUCCESS   process with successful
//    STATUS_xxx       other status with unsuccessful
//
//
//  IRQL:
//
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS Test_LoadFilesToDSP(PBT_DEVICE_EXT devExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	// Enable NIC clocks and DMAs. Leave UniPHY core in reset
	BtCardBusNICEnable(devExt);
	Status = Test_LoadOneFileToDSP(devExt, BT_SPX_P_MEM, "\\DosDevices\\c:\\temp\\dsp_Pmem.bin");
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}
	Status = Test_LoadOneFileToDSP(devExt, BT_SPX_A_MEM, "\\DosDevices\\c:\\temp\\dsp_Amem.bin");
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}
	Status = Test_LoadOneFileToDSP(devExt, BT_SPX_B_MEM, "\\DosDevices\\c:\\temp\\dsp_Bmem.bin");
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}
	Status = Test_LoadOneFileToDSP(devExt, BT_SPX_T_MEM, "\\DosDevices\\c:\\temp\\dsp_Emem.bin");
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}
	return Status;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_GetNextIndex
//
//    Get the next index by the input index.
//
//
//  INPUTS:
//
//      index - the input index
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//	    NONE
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT8 Test_GetNextIndex(UINT8 index)
{
	if (index == 15)
	{
		return (0);
	}
	else
	{
		return (index + 1);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test_GetRecvData
//
//    Copy the frame into rx buffer between application and driver.
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      buf - the pointer to the buffer saved the receive data.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//      datalen - output data length
//	
//  IRQL:
//
//      This routine is called at IRQL DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT32 Test_GetRecvData(PBT_DEVICE_EXT devExt, PUINT8 buf)
{
	PBT_TEST_T pTest;
	KIRQL oldIrql;
	UINT32 tmplen;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test_CopyTxBufAndNotifyApp enter\n");
	/*Get pointer to the test */
	pTest = (PBT_TEST_T)devExt->pTest;
	if (pTest == NULL)
	{
		return 0;
	}
	/* Lock */
	KeAcquireSpinLock(&pTest->lock, &oldIrql);
	if (pTest->curIndex == pTest->lastIndex)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "Rx block is empty! curIndex = %d, lastIndex = %d\n", pTest->curIndex, pTest->lastIndex);
		/* Unlock */
		KeReleaseSpinLock(&pTest->lock, oldIrql);
		return 0;
	}
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Rx block is to be copied... curIndex = %d, lastIndex = %d.\n", pTest->curIndex, pTest->lastIndex);
	tmplen = pTest->RxBlock[pTest->lastIndex].len;
	RtlCopyMemory(buf, pTest->RxBlock[pTest->lastIndex].buf, tmplen);
	pTest->lastIndex = Test_GetNextIndex(pTest->lastIndex);
	/* Unlock */
	KeReleaseSpinLock(&pTest->lock, oldIrql);
	return (tmplen);
}



