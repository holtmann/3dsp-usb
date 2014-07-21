/***********************************************************************
 * FILENAME:         BT_TestModule.c
 * CURRENT VERSION:  1.00.01
 * CREATE DATE:      2006/06/18
 * PURPOSE:  We package all the BT test procedures into this file.
 * 
 * AUTHORS:  Lewis Wang
 *
 * NOTES:    //
***********************************************************************/

/*
 *  REVISION HISTORY
 2006.8.4  Add a member "valid_flag" in struct CONNECT_DEVICE_T. This flag is set as 1 when a connection is connected. And this flag is
           set as 0 when the connection is disconnected. And if this flag is set as 0, LMPDU and some others frames should not be sent.
 2006.8.9  Found some bugs and fixed. This bug would lead to dead lock and cause OS hung.
           function TestMode_TransmitData
 2007.1.23 Add some codes for checking device's state(plug in or out).
 2007.11.7 Add some codes for 64 bit OS
 */

#include "bt_sw.h"         /* include <WDM.H> and PBT_DEVICE_EXT structure */
#include "bt_testmode.h"
#include "bt_lmp.h"
#include "bt_hci.h"
#include "bt_frag.h"
#include "bt_queue.h"
#include "bt_pr.h"
#include "bt_hal.h"        /* include accessing hardware resources function */
#include "bt_dbg.h"        /* include debug function */

#ifdef BT_SERIALIZE_DRIVER
#include "bt_serialize.h"
#endif

/*--file local macros--------------------------------------------------*/

/* send LMP PDU in GUI */
#define  TEST_ACTIVATE_COMMAND  1
#define  TEST_CONTROL_COMMAND   2
#define  TEST_PARA_LEN          12

/*--file local constants and types-------------------------------------*/

/*--local function prototypes------------------------------------------*/

extern void ChangeLmpExtraState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state);

/*************************************************************
* TestMode_Init
* 
* Description:
*    This function intializes the test module.
*
* Arguments:
*    devExt: IN, pointer to device extension.
*       
* Return Value: 
*    STATUS_SUCCESS:      Test module initialized succesfully.
*    STATUS_UNSUCCESSFUL: Test module initialized failed.
**************************************************************/
NTSTATUS TestMode_Init(PBT_DEVICE_EXT devExt)
{
	PBT_TESTMODE_T pTestMode;
	
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_Init\n");
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_Init devExt = 0x%p\n", devExt);
	
	/* Alloc memory for test module */
	pTestMode = (PBT_TESTMODE_T)ExAllocatePool(sizeof(BT_TESTMODE_T), GFP_KERNEL);
	if (pTestMode == NULL)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "Allocate Test Module memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_Init pTestModule = 0x%p, size = %d\n", pTestMode, sizeof(BT_TESTMODE_T));
	
	/* Save test module pointer into device extention context */
	devExt->pTestMode = (PVOID)pTestMode;
	devExt->AllowCmdIndicator = TRUE;
	/* Zero out the test module space */
	RtlZeroMemory(pTestMode, sizeof(BT_TESTMODE_T));
	
	/* Alloc spin lock, which used to protect test module operator */
	KeInitializeSpinLock(&pTestMode->lock);
	
	
	return (STATUS_SUCCESS);
}

/*************************************************************
 *   TestMode_Release
 *
 *   Descriptions:
 *      Releases test module.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *
 *   Return Value:
 *      STATUS_SUCCESS:      Test module released succesfully.
 *      STATUS_UNSUCCESSFUL: Test module released failed.
 *************************************************************/
NTSTATUS TestMode_Release(PBT_DEVICE_EXT devExt)
{
	PBT_TESTMODE_T pTestMode;
	
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_Release\n");
	
	/* Get pointer of the test module */
	pTestMode = (PBT_TESTMODE_T)devExt->pTestMode;
	if (pTestMode == NULL)
		return STATUS_UNSUCCESSFUL;
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_Release devExt = 0x%p, pTestModule = 0x%p\n", devExt, pTestMode);
	
	/* Cancel test module timer */
	KeCancelTimer(&pTestMode->TxTimer);
	
	/*Free the test module memory */
	if (pTestMode != NULL)
		ExFreePool(pTestMode);
	devExt->pTestMode = NULL;
	
	return (STATUS_SUCCESS);
}

void TestMode_ResetMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice->current_role != BT_ROLE_SLAVE || ((PBT_TESTMODE_T)devExt->pTestMode)->pDevice != pConnectDevice)
		return;
	
	((PBT_HCI_T)devExt->pHci)->test_mode_active = 0;
	
	((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = DATA_WHITENING;
	((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_WHITENING;
	((PBT_TESTMODE_T)devExt->pTestMode)->pDevice           = NULL;
	((PBT_TESTMODE_T)devExt->pTestMode)->being_test        = 0;
	((PBT_TESTMODE_T)devExt->pTestMode)->role              = 0;
	RtlZeroMemory(&(((PBT_TESTMODE_T)devExt->pTestMode)->configuration), sizeof(DUT_CONFIG));
	
	((PBT_TESTMODE_T)devExt->pTestMode)->PacketType    = 0;
	((PBT_TESTMODE_T)devExt->pTestMode)->PaylaodHeader = 0;
	((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength  = 0;
	
	return;
}

/*************************************************************
 *   TestMode_Start_Tester_Data
 *
 *   Descriptions:
 *      Process the LMP test command from GUI.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *      para:   IN, pointer to the LMP PDU parameters, para[0, 1]: ConnHandle, para[2]: command-type.
 *      length: IN, the length of parameters.
 *
 *   Return Value:
 *      STATUS_SUCCESS:          Command be executed succesfully.
 *      STATUS_UNSUCCESSFUL:     Command be executed failed.
 *      RPC_NT_NULL_REF_POINTER: if the IN pointer is NULL.
 *************************************************************/
NTSTATUS TestMode_Start_Tester_Data(PBT_DEVICE_EXT devExt, PUINT8 para, UINT32 length)
{
	PCONNECT_DEVICE_T pConnectDevice;
	LMP_PUD_PACKAGE_T pdu;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8  i;
	UINT16 conn_handle;

#ifdef BT_TESTMODE_SUPPORT
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----TestMode_Start_Tester_Data enter, devExt = 0x%p, pPara = 0x%p, length = %d\n", devExt, para, length);
	if (devExt == NULL || para == NULL || length < TEST_PARA_LEN)
		return (RPC_NT_NULL_REF_POINTER);
	
	RtlCopyMemory(&conn_handle, para, sizeof(UINT16));
	pConnectDevice = Hci_Find_Connect_Device_By_ConnHandle((PBT_HCI_T)devExt->pHci, conn_handle);
	if (pConnectDevice == NULL || pConnectDevice->current_role != BT_ROLE_MASTER)
		return (STATUS_UNSUCCESSFUL);
	
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----para(byte-orders)= %d %d %d %d %d %d %d %d %d %d %d %d", para[0], para[1], para[2], para[3], para[4], 
		para[5], para[6], para[7], para[8], para[9], para[10], para[11]);
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = MASTER_TRANSACTION_ID;
	if (para[2] == TEST_ACTIVATE_COMMAND)
	{
		pdu.OpCode = 56;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_TEST_ACTIVATE);
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "LMP----PDU Inserted Tx list (TransID=%d): Test Activate\n", pdu.TransID);
		}
	}
	else if (para[2] == TEST_CONTROL_COMMAND)
	{
		pdu.OpCode = 57;
		RtlCopyMemory(&(((PBT_TESTMODE_T)devExt->pTestMode)->configuration), &para[3], sizeof(DUT_CONFIG));
		RtlCopyMemory(pdu.contents, &para[3], TEST_PARA_LEN - 3);
		for (i = 0; i < TEST_PARA_LEN - 3; i++)
			pdu.contents[i] = (UINT8)(pdu.contents[i] ^ 0x55);

		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 10);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_TEST_CONTROL);
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "LMP----PDU Inserted Tx list (TransID=%d): Test Control\n", pdu.TransID);
		}
	}
	else
	{
		return (STATUS_UNSUCCESSFUL);
	}
	
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----TestMode_Start_Tester_Data exit!\n");
#endif

	return status;
}

/*************************************************************
 *   TestMode_MakeTransmitterPacket
 *
 *   Descriptions:
 *      For the transmitter test mode, only packets without FEC should be used;
 *      i.e. HV3, EV3, EV5, DH1, DH3, DH5 and AUX1 packets.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *
 *   Return Value:
 *      STATUS_SUCCESS
 *************************************************************/
NTSTATUS TestMode_MakeTransmitterPacket(PBT_DEVICE_EXT devExt)
{
	PDUT_CONFIG  pDUTConfig;
	UINT8  Data[MAX_TEST_DATA_BUF], FeedbackBit, PRBSbyte = 0xFF, PRBSbit = 0x01;
	UINT8  packettype = 0, paylaodheader = 0;
	UINT16 packetlen = 0, bitlen = 0;
	
	pDUTConfig = &(((PBT_TESTMODE_T)devExt->pTestMode)->configuration);
	RtlCopyMemory(&packetlen, &pDUTConfig->data_length_L, sizeof(UINT16));
	packetlen  = (packetlen < MAX_TEST_DATA_BUF) ? packetlen : MAX_TEST_DATA_BUF;
	packettype = (UINT8)(pDUTConfig->packet_type & 0x0F);
	switch (packettype)
	{
		case 0: /* NULL */
		case 1: /* POLL */
			packetlen     = 0;
			paylaodheader = 0;
			break;
			
		case 4: /* DH1, 2-DH1 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x00) /* DH1 */
			{
				paylaodheader = 1;
				if (packetlen > 27)
					packetlen = 27;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x20) /* 2-DH1 */
			{
				paylaodheader = 2;
				if (packetlen > 54)
					packetlen = 54;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 6: /* 2-EV3 */
			paylaodheader = 0;
			if ((pDUTConfig->packet_type & 0xF0) == 0x30)
			{
				if (packetlen > 60 || packetlen < 1)
					packetlen = 60;
			}
			else
			{
				packetlen  = 0;
				packettype = 0;
			}
			
			break;
			
		case 7: /* HV3, EV3, 3-EV3 */
			paylaodheader = 0;
			if ((pDUTConfig->packet_type & 0xF0) == 0x00) /* HV3 */
			{
				packetlen = 30;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x10) /* EV3 */
			{
				if (packetlen > 30 || packetlen < 1)
					packetlen = 30;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x30) /* 3-EV3 */
			{
				if (packetlen > 90 || packetlen < 1)
					packetlen = 90;
			}
			else
			{
				packetlen  = 0;
				packettype = 0;
			}
			
			break;
			
		case 8: /* 3-DH1 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x20)
			{
				paylaodheader = 2;
				if (packetlen > 83)
					packetlen = 83;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 9: /* AUX1 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x00 || (pDUTConfig->packet_type & 0xF0) == 0x20)
			{
				paylaodheader = 1;
				if (packetlen > 29)
					packetlen = 29;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 10: /* 2-DH3 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x20)
			{
				paylaodheader = 2;
				if (packetlen > 367)
					packetlen = 367;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 11: /* DH3, 3-DH3 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x00) /* DH3 */
			{
				paylaodheader = 2;
				if (packetlen > 183)
					packetlen = 183;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x20) /* 3-DH3 */
			{
				paylaodheader = 2;
				if (packetlen > 552)
					packetlen = 552;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 12: /* 2-EV5 */
			paylaodheader = 0;
			if ((pDUTConfig->packet_type & 0xF0) == 0x30)
			{
				if (packetlen > 360 || packetlen < 1)
					packetlen = 360;
			}
			else
			{
				packetlen  = 0;
				packettype = 0;
			}
			
			break;
			
		case 13: /* EV5, 3-EV5 */
			paylaodheader = 0;
			if ((pDUTConfig->packet_type & 0xF0) == 0x10) /* EV5 */
			{
				if (packetlen > 180 || packetlen < 1)
					packetlen = 180;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x30) /* 3-EV5 */
			{
				if (packetlen > 540 || packetlen < 1)
					packetlen = 540;
			}
			else
			{
				packetlen  = 0;
				packettype = 0;
			}
			
			break;
			
		case 14: /* 2-DH5 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x20)
			{
				paylaodheader = 2;
				if (packetlen > 679)
					packetlen = 679;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		case 15: /* DH5, 3-DH5 */
			if ((pDUTConfig->packet_type & 0xF0) == 0x00) /* DH5 */
			{
				paylaodheader = 2;
				if (packetlen > 339)
					packetlen = 339;
			}
			else if ((pDUTConfig->packet_type & 0xF0) == 0x20) /* 3-DH5 */
			{
				paylaodheader = 2;
				if (packetlen > 1021)
					packetlen = 1021;
			}
			else
			{
				packetlen     = 0;
				packettype    = 0;
				paylaodheader = 0;
			}
			
			break;
			
		default :
			packetlen     = 0;
			packettype    = 0;
			paylaodheader = 0;
			break;
	}
	
	if (packetlen != 0)
	{
		if (pDUTConfig->test_scenario == 1)
		{
			RtlFillMemory(Data, packetlen, 0x00);
		}
		else if (pDUTConfig->test_scenario == 2)
		{
			RtlFillMemory(Data, packetlen, 0xFF);
		}
		else if (pDUTConfig->test_scenario == 3)
		{
			RtlFillMemory(Data, packetlen, 0x55);
		}
		else if (pDUTConfig->test_scenario == 9)
		{
			RtlFillMemory(Data, packetlen, 0x0F);
		}
		else if (pDUTConfig->test_scenario == 4)
		{
			RtlZeroMemory(Data, MAX_TEST_DATA_BUF);
			for (bitlen = 0; bitlen <= (packetlen * 8 - 1); bitlen++)
			{
				Data[bitlen / 8] = (UINT8)(Data[bitlen / 8] + (PRBSbit << (bitlen % 8)));
				
				FeedbackBit = (UINT8)((PRBSbyte >> 4) & 0x01); /* get 5th bit */
				FeedbackBit = (UINT8)((FeedbackBit ^ PRBSbit) & 0x01);
				PRBSbit     = (UINT8)((PRBSbyte >> 7) & 0x01);
				PRBSbyte    = (UINT8)(((PRBSbyte << 1) & 0xFE) | FeedbackBit);
			}
		}
		else
		{
			packetlen     = 0;
			packettype    = 0;
			paylaodheader = 0;
		}
	}
	
	((PBT_TESTMODE_T)devExt->pTestMode)->PacketType    = packettype;
	((PBT_TESTMODE_T)devExt->pTestMode)->PaylaodHeader = paylaodheader;
	((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength  = packetlen;
	RtlZeroMemory(((PBT_TESTMODE_T)devExt->pTestMode)->PacketData, MAX_TEST_DATA_BUF);
	RtlCopyMemory(((PBT_TESTMODE_T)devExt->pTestMode)->PacketData, Data, packetlen);
	return STATUS_SUCCESS;
}

/*************************************************************
 *   TestMode_TransmitData
 *
 *   Descriptions:
 *      Make the test packets and Tx.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *
 *   Return Value:
 *      STATUS_SUCCESS, STATUS_PENDING, STATUS_CANCELLED
 *************************************************************/
UINT32 TestMode_TransmitData(PBT_DEVICE_EXT devExt)
{
	LARGE_INTEGER	timevalue;
	PBT_FRAG_T	pFrag;
	PHCBB_PAYLOAD_HEADER_T        pHCBBHead;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PPAYLOAD_HEADER_MULTI_SLOT_T  pMultiHead;
	KIRQL oldIrql;
	UINT32 reallen;
	PUINT8 	dest;
	UINT8	dataType = 0;
	UINT32	IdleSpace;
	PCONNECT_DEVICE_T	pConnectDevice;
	NTSTATUS status = STATUS_SUCCESS;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return BT_FRAG_RESOURCE_ERROR;
	
	if (((PBT_HCI_T)devExt->pHci)->test_flag == 0 || ((PBT_HCI_T)devExt->pHci)->test_mode_active == 0 || ((PBT_TESTMODE_T)devExt->pTestMode)->being_test != 1)
		return STATUS_CANCELLED;

	pConnectDevice = ((PBT_TESTMODE_T)devExt->pTestMode)->pDevice;
	if ((pConnectDevice != NULL) && (pConnectDevice->valid_flag == 0))
	{
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Sending Transmitter Test Data, but connect device does not exist\n");
		return STATUS_SUCCESS;
	}

	 if(pConnectDevice->timer_l2cap_flow_valid == 1)
	 {
	 	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----l2cap stop flag is set, return\n");
		return STATUS_SUCCESS;
	 }


	dest = BtGetValidTxPool(devExt);
	if(dest == NULL)
	{
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_TransmitData---Failed to get valid pool\n");
		return BT_FRAG_RESOURCE_ERROR;
	}

	/* Fill hcbb payload header */
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->slave_index       = (((PBT_TESTMODE_T)devExt->pTestMode)->pDevice)->slave_index;
	pHCBBHead->master_slave_flag = ((((PBT_TESTMODE_T)devExt->pTestMode)->pDevice)->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;

	
	BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Sending Transmitter Test Data...\n");
	pHCBBHead->am_addr = (((PBT_TESTMODE_T)devExt->pTestMode)->pDevice)->am_addr;
	pHCBBHead->type    = ((PBT_TESTMODE_T)devExt->pTestMode)->PacketType;
	pHCBBHead->tx_retry_count = 3 << RETRY_COUNT_LEFT_SHIFT_BIT_TESTMODE;
	
	if (((PBT_TESTMODE_T)devExt->pTestMode)->PaylaodHeader == 1)
	{
		pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)((PUINT8)pHCBBHead + sizeof(HCBB_PAYLOAD_HEADER_T));
		pSingHead->l_ch   = 2;
		pSingHead->flow   = 1;
		pSingHead->length = (UINT8)(((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength);
		pHCBBHead->length = pSingHead->length + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
		RtlCopyMemory((PUINT8)pSingHead + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), ((PBT_TESTMODE_T)devExt->pTestMode)->PacketData, pSingHead->length);
	}
	else if (((PBT_TESTMODE_T)devExt->pTestMode)->PaylaodHeader == 2)
	{
		pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)((PUINT8)pHCBBHead + sizeof(HCBB_PAYLOAD_HEADER_T));
		pMultiHead->l_ch      = 2;
		pMultiHead->flow      = 1;
		pMultiHead->undefined = 0;
		pMultiHead->length    = ((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength;
		pHCBBHead->length     = pMultiHead->length + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T);
		RtlCopyMemory((PUINT8)pMultiHead + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), ((PBT_TESTMODE_T)devExt->pTestMode)->PacketData, pMultiHead->length);
	}
	else
	{
		pHCBBHead->length = ((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength;
		RtlCopyMemory((PUINT8)pHCBBHead + sizeof(HCBB_PAYLOAD_HEADER_T), ((PBT_TESTMODE_T)devExt->pTestMode)->PacketData, pHCBBHead->length);
	}
	reallen = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);
	//Jakio20080630: 4bytes aligned
	reallen = ALIGNLONGDATALEN(reallen);

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = pFrag->IdleSpace[FRAG_SLAVE_LIST];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	if(IdleSpace <= reallen)
	{
		ReleaseBulkOut1Buf(dest, devExt);
		Frag_TxTimerInstant(devExt, &timevalue);
		KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
		return BT_FRAG_RESOURCE_ERROR;
	}
	else
	{
		#if DBG
		BtPrintBuffer(dest, 16);	
		#endif
	
		status = BtUsbWriteAll(devExt, dest, reallen, MAILBOX_DATA_TYPE_SLAVE, 0);
		if(NT_SUCCESS(status))
		{
			//write success,update space info
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			pFrag->IdleSpace[FRAG_SLAVE_LIST] -= reallen;
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		}
		else
		{
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_TransmitData---write data failed\n");
			ReleaseBulkOut1Buf(dest, devExt);
			return BT_FRAG_RESOURCE_ERROR;
		}
	}

	return BT_FRAG_SUCCESS;
}

/*************************************************************
 *   TestMode_GetWhitening
 *
 *   Descriptions:
 *      used for sending LMP PDU in test mode.
 *
 *   Arguments:
 *      devExt:  IN, pointer to device extension.
 *      pLmpPdu: IN, pointer to LMP PDU.
 *
 *   Return Value:
 *      bit0: 0(whitening) 1(no whitening).
 *      bit1: 0(re-trans)  1(no re-trans), now retrans-PDU is LMP_accepted test_control.
 *************************************************************/
UINT16 TestMode_GetWhitening(PBT_DEVICE_EXT devExt, PUINT8 pLmpPdu)
{
	UINT16 whitening = 0;
	
	if (devExt == NULL || pLmpPdu == NULL || devExt->pHci == NULL || ((PBT_HCI_T)devExt->pHci)->test_flag == 0)
		return 0;
	
	if (((PBT_HCI_T)devExt->pHci)->test_mode_active == 1)
	{
		if (((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 3 && (((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 57 || ((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 56))
		{
			whitening = (UINT16)(((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening & 0x01);
			if (((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 57)
				BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Accepted Test Control PDU: whitening= %d\n", whitening);
			else
				BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Accepted Test Activate PDU: whitening= %d\n", whitening);
		}
		else
		{
			whitening = (UINT16)(((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening & 0x01) + 2;
		}
	}
	else
	{
		if (((PBT_TESTMODE_T)devExt->pTestMode)->being_test == 2 && ((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 3 && ((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 57)
		{
			((PBT_TESTMODE_T)devExt->pTestMode)->being_test = 0;
			whitening = (UINT16)(((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening & 0x01);
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Accepted Test Control PDU: whitening= %d\n", whitening);
		}
		else
		{
			whitening = 0;
		}
	}
	
	whitening = whitening << RETRY_COUNT_LEFT_SHIFT_BIT_TESTMODE;
	return whitening;
}

NTSTATUS TestMode_WriteCommandIndicator(PBT_DEVICE_EXT devExt)
{
	PDUT_CONFIG     pDUT_Config;

	if (((PBT_TESTMODE_T)devExt->pTestMode)->role == 2) /* DUT */
	{
		pDUT_Config = &(((PBT_TESTMODE_T)devExt->pTestMode)->configuration);
		BtDelay(5*1000); /* unit: ms */
		
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Begin write test mode command indicator\n");
		//Jakio20080701: changed here
		//BT_WRITE_COMMAND_INDICATOR(devExt, BT_HCI_COMMAND_INDICATOR_TEST_MODE);
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_TEST_MODE);
		if (pDUT_Config->test_scenario == 1 || pDUT_Config->test_scenario == 2 || pDUT_Config->test_scenario == 3 
			|| pDUT_Config->test_scenario == 4 || pDUT_Config->test_scenario == 9)
		{
			TestMode_MakeTransmitterPacket(devExt);
			((PBT_TESTMODE_T)(devExt->pTestMode))->being_test = 1;
		}
	}
	else if (((PBT_TESTMODE_T)devExt->pTestMode)->role == 1) /* tester */
	{
		BtDelay(6*1000); /* unit: ms */
		
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "Test----Begin write test mode command indicator\n");
		//Jakio20080701: changed here
		//BT_WRITE_COMMAND_INDICATOR(devExt, BT_HCI_COMMAND_INDICATOR_TEST_MODE);
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_TEST_MODE);
	}
	

	
	return STATUS_SUCCESS;
}

/*************************************************************
 *   TestMode_FlushFIFO
 *
 *   Descriptions:
 *      used for flush FIFO in test mode.
 *
 *   Arguments:
 *      devExt:         IN, pointer to device extension.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *
 *   Return Value:
 *      STATUS_SUCCESS:      be processed successfully.
 *      STATUS_UNSUCCESSFUL: all the remain of unsuccessful situation.
 *************************************************************/
NTSTATUS TestMode_FlushFIFO(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
		KIRQL oldIrql;
		PBT_HCI_T pHci;
		UINT32 tmpConnectDevice;
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
		BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_FlushFIFO: entered\n");
		// Initialize complete packets
		RtlZeroMemory(&CompletePackets, sizeof(COMPLETE_PACKETS_T));
		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		if(pFrag == NULL)
			return STATUS_UNSUCCESSFUL;

		// Release role/switch resource begin
		// Get pointer of hci module
		pHci = (PBT_HCI_T)devExt->pHci;
		if(pHci == NULL)
			return STATUS_UNSUCCESSFUL;

		
		
		//clear all frag element related with the connect device
		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pList = (PBT_LIST_ENTRY_T)Frag_GetQueueHead(devExt, pConnectDevice, &ListIndex);
		pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(pList);
		while(pEle != NULL)
		{
			if(pEle->pConnectDevice == (ULONG_PTR)pConnectDevice)
			{
				//get next element
				pTmpEle = pEle;
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);

				//record num of completed packets
				if(ListIndex != FRAG_SCO_LIST)
				{
					phead = (PHCI_DATA_HEADER_T)pTmpEle->memory;
					CompletePackets.number_of_handles++;
					CompletePackets.connection_handle[CompletePackets.number_of_handles] = (UINT16)phead->connection_handle;
					CompletePackets.num_of_complete_packets[CompletePackets.number_of_handles] = 1;	
				}
				//remove from the used list and put it into freelist
				QueueRemoveEle(pList, &pTmpEle->link);
				QueuePutTail(&pFrag->Free_FragList, &pTmpEle->link);
			}
			else
			{
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);
			}
		}
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

		if(ListIndex == FRAG_SLAVE_LIST)
		{
			BtDelay(5000);
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_FlushFIFO--no slave device, clear scratch buffer\n");
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE);
		}
		else if(ListIndex == FRAG_MASTER_LIST)
		{
			//judge if all connection is released
			/*this is the only connect master device*/
			if(pHci->num_device_am == 1)
			{
				BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_FlushFIFO--No connect device, clear scratch buffer\n");
				BtDelay(5000);
				VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_MASTER);
			}
	
		}
		else
		{
			BT_DBGEXT(ZONE_TEST | LEVEL3,  "TestMode_FlushFIFO--Error happens. Frag list is %d\n", ListIndex);
		}
	return STATUS_SUCCESS;
}

/*--end of file--------------------------------------------------------*/
