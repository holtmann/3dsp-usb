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
$RCSfile: bt_sendrecv.c,v $
$Revision: 1.13 $
$Date: 2010/09/06 05:17:28 $
 **************************************************************************/
#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include hci interface
#include "bt_task.h"
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
#include "bt_frag.h"			
#include "bt_hal.h"
#include "afhclassify.h"
#ifdef BT_PCM_COMPRESS
#include "g711.h"
#include "cvsd.h"
#endif
#ifdef BT_TESTDRIVER
#endif
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
#include "afhclassify.h"
#endif
#ifdef BT_SCHEDULER_SUPPORT
#include "sched.h"
#endif
#ifdef BT_LOOPBACK_SUPPORT
#include "bt_loopback.h"
#endif
#ifdef BT_TESTMODE_SUPPORT
#include "bt_testmode.h"
#endif
#ifdef BT_SERIALIZE_DRIVER
#include "bt_serialize.h"
#endif

UINT8 tempvalue;
///////////////////////////////////////////////////////////////////////////////
//
//  BtSetupShareMemory
//
//    This function can allocate the share memory which is used for BD. Rx cached
//    memory is also allocated in this function
//
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
//
//    STATUS_SUCCESS.   memory is allocated with succesfully
//    STATUS_UNSUCCESSFUL.   memory is allocated with fail
//
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//ChangLog:
//	Jakio20071204: rewrite this function to avoid mem peak;	
///////////////////////////////////////////////////////////////////////////////
NTSTATUS BtSetupShareMemory(PBT_DEVICE_EXT devExt)
{
	UINT32	i;
	UINT8	FunctionOK = FALSE;
	
	devExt->TxStartAddress = NULL;
	devExt->RxCachPid = devExt->RxCachCid = 0;
	devExt->RxCachStartAddr = NULL;
	devExt->RxSeqNo = 0;
	devExt->IsFirstRxFrame = TRUE;
	KeInitializeSpinLock(&devExt->TxSpinLock);
	__try
	{
		devExt->TxStartAddress = (PUINT8)ExAllocatePool(MAX_TX_POOL_COUNT *(BT_MAX_FRAME_SIZE + BT_MAX_SCO_FRAME_SIZE), GFP_KERNEL);
		if (devExt->TxStartAddress == NULL)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Allocate Tx memory failed!\n");
			__leave;
		}
		// Set the allocated share memory to each tx pool
		for (i = 0; i < MAX_TX_POOL_COUNT; i++)
		{
			devExt->TxPool[i].Buffer = devExt->TxStartAddress + i * BT_MAX_FRAME_SIZE;
			devExt->TxPool[i].IsUsed = NOT_USED;
		}

		devExt->TxAclPendingFlag = FALSE;
		devExt->TxScoPendingFlag = FALSE;
		devExt->AclPendingCounter = 0;
		devExt->ScoPendingCounter = 0;
		
		for (i = 0; i < MAX_TX_POOL_COUNT; i++)
		{
			devExt->TxScoPool[i].Buffer = devExt->TxStartAddress + MAX_TX_POOL_COUNT * BT_MAX_FRAME_SIZE + i * BT_MAX_SCO_FRAME_SIZE;
			devExt->TxScoPool[i].IsUsed = NOT_USED;
		}
		
		devExt->RxCachStartAddr = (PUINT8)ExAllocatePool(MAX_RX_CACH_COUNT *BT_MAX_FRAME_SIZE, GFP_KERNEL);
		if (devExt->RxCachStartAddr == NULL)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Allocate rx cache memory failed!\n");
			__leave;
		}
		// Set the allocated non-page memory to each rx-cach
		for (i = 0; i < MAX_RX_CACH_COUNT; i++)
		{
			devExt->RxCach[i] = devExt->RxCachStartAddr + i * BT_MAX_FRAME_SIZE;
		}

		for(i = 0; i < MAX_LMP_CACH_COUNT; i++)
		{
			devExt->RxLMPPDUCach[i].Flag_InUse = FALSE;
		}

		FunctionOK = TRUE;
	}
	__finally
	{
		if(FunctionOK == TRUE)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtSetupShareMemory ok\n");
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtSetupShareMemory failure\n");
			if(devExt->TxStartAddress)
				ExFreePool(devExt->TxStartAddress);
			if(devExt->RxCachStartAddr)
				ExFreePool(devExt->RxCachStartAddr);
			//return STATUS_UNSUCCESSFUL;
		}
	}
	if(FunctionOK == TRUE)
	{
		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_UNSUCCESSFUL;
	}


}

///////////////////////////////////////////////////////////////////////////////
//
//  BtDeleteShareMemory
//
//    This function delete the share memory which is allocated by functin
//    BtSetupShareMemory
//
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
//		
//		None
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
//ChangeLog:
//	Jakio20071204: rewrite this function dued to the change of BtSetupSharememory();
///////////////////////////////////////////////////////////////////////////////
VOID BtDeleteShareMemory(PBT_DEVICE_EXT devExt)
{
	if (devExt->TxStartAddress)
	{
		ExFreePool((PVOID)devExt->TxStartAddress);
		devExt->TxStartAddress = NULL;
	}

	devExt->RxCachPid = devExt->RxCachCid = 0;
	if (devExt->RxCachStartAddr)
	{
		ExFreePool((PVOID)devExt->RxCachStartAddr);
		devExt->RxCachStartAddr = NULL;
	}	
	// Initialize each rx cach
	RtlZeroMemory((PVOID)devExt->RxCach, MAX_RX_CACH_COUNT);

	if (devExt->pBTRegMems != NULL)
	{
		ExFreePool(devExt->pBTRegMems);
		devExt->pBTRegMems = NULL;
	}
}



UINT8 BtGetFreeLMPCach(PBT_DEVICE_EXT devExt)
{
	UINT8	index;

	ASSERT(devExt);

	for(index = 0; index < MAX_LMP_CACH_COUNT; index++)
	{
		if(devExt->RxLMPPDUCach[index].Flag_InUse == FALSE)
			break;
	}
	if(index == MAX_LMP_CACH_COUNT)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "LMP cach exhausted\n");
		return 0xff;
	}
	else
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "LMp cach index is %d\n", index);
		return index;
	}
	
}



///////////////////////////////////////////////////////////////////////////////
//
//  BtInitSomeCSR
//
//    This function write some CSR registers such as retry-limit and so on.
//
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
//		
//		None
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID BtInitSomeCSR(PBT_DEVICE_EXT devExt)
{
	// Set retry limit as 15
	UsbWriteTo3DspReg(devExt, BT_CSR_NUM_RETRY_REG, 0xf);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtInitSomeVars
//
//    This function initialize some vars.
//
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
//		
//		None
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID BtInitSomeVars(PBT_DEVICE_EXT devExt)
{
	devExt->ResetFlag = FALSE;

	devExt->pBTRegMems = NULL;
#ifdef BT_TESTDRIVER
	devExt->RecvDataCount = 0;
	devExt->TxDataCount = 0;
	devExt->RealRecvSuccCount = 0;
	devExt->RealRecvNotSuccCount = 0;
#endif
	devExt->TxRetryCount = 0;
	devExt->TxAclSendCount = 0;
	devExt->StartRetryCount = 0;
	devExt->EndRetryCount = 0;

	devExt->AdapterUnplugged = FALSE;
}




///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessTxCommand
//
//    This function process the tx command frame that saved in the buffer.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    buf - Address of the frame.
//    len - Length of the frame.
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
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS BtProcessTxCommand(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	PHCI_COMMAND_HEADER_T pCommandHead;
	UINT8 invalidflag = 0;
	PUINT8 pParam;
	PBT_HCI_T pHci;
	KIRQL oldIrql;

	pHci = (PBT_HCI_T)devExt->pHci;
    /* Actually, the Higher-Level protocol has avoid this exception */
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "!!!!!Exception!!!!! Multi HCI commands are disallowed\n");

        /* Keep going */
	}
    KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    
	pCommandHead = (PHCI_COMMAND_HEADER_T)buf;
	pParam = &pCommandHead->para;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "HCI command ogf 0x%x, ocf 0x%x\n", pCommandHead->opcode.ogf, pCommandHead->opcode.ocf);
    
	switch (pCommandHead->opcode.ogf)
	{
		case BT_HCI_COMMAND_OGF_LINK_CONTROL:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): link control command\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_INQUIRY:
				Hci_Command_Inquiry(devExt, pParam);
				break;
			case BT_HCI_COMMAND_INQUIRY_CANCEL:
				Hci_Command_Inquiry_Cancel(devExt);
				break;
            case BT_HCI_COMMAND_PERIODIC_INQUIRY_MODE:
                Hci_Command_Periodic_Inquiry_Mode(devExt, pParam);
                break;
            case BT_HCI_COMMAND_EXIT_PERIODIC_INQUIRY_MODE:
                Hci_Command_Exit_Periodic_Inquiry_Mode(devExt);
                break;
			case BT_HCI_COMMAND_CREATE_CONNECTION:
				Hci_Command_Create_Connection(devExt, pParam);
				break;
			case BT_HCI_COMMAND_READ_CLOCK_OFFSET:
				Hci_Command_Read_Clock_Offset(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_REMOTE_NAME_REQUEST:
				Hci_Command_Remote_Name_Request(devExt, pParam);
				break;
			case BT_HCI_COMMAND_REMOTE_NAME_REQUEST_CANCEL:
				Hci_Command_Remote_Name_Request_Cancel(devExt, pParam);
				break;
			case BT_HCI_COMMAND_DISCONNECT:
				Hci_Command_Disconnect(devExt, *(PUINT16)pParam, *(pParam + 2));
				break;
			case BT_HCI_COMMAND_ACCEPT_CONNECTION_REQUEST:
				Hci_Command_Accept_Connection_Request(devExt, pParam);
				break;
			case BT_HCI_COMMAND_REMOTE_SUPPORTED_FEATURES:
				Hci_Command_Read_Remote_Supported_Features(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_REJECT_CONNECTION_REQUEST:
				Hci_Command_Reject_Connection_Request(devExt, pParam);
				break;
			case BT_HCI_COMMAND_READ_REMOTE_VERSION_INFORMATION:
				Hci_Command_Read_Remote_Version_Info(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_LINK_KEY_REQUEST_REPLY:
				Hci_Command_Link_Keq_Request_Reply(devExt, pParam);
				break;
			case BT_HCI_COMMAND_LINK_KEY_REQUEST_NEGATIVE_REPLY:
				Hci_Command_Link_Keq_Request_Negative_Reply(devExt, pParam);
				break;
			case BT_HCI_COMMAND_PIN_CODE_REQUEST_NEGATIVE_REPLY:
				Hci_Command_Pin_Code_Request_Negative_Reply(devExt, pParam);
				break;
			case BT_HCI_COMMAND_PIN_CODE_REQUEST_REPLY:
				Hci_Command_Pin_Code_Request_Reply(devExt, pParam);
				break;
			case BT_HCI_COMMAND_SET_CONNECTION_ENCRYPTION:
				Hci_Command_Set_Connection_Encryption(devExt, *(PUINT16)pParam, *(pParam + 2));
				break;
			case BT_HCI_COMMAND_AUTHENTICATION_REQUESTED:
				Hci_Command_Authentication_Requested(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_CHANGE_CONNECTION_PACKET_TYPE:
				Hci_Command_ChangeConnectionPacketType(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_ADD_SCO_CONNECTION:
				Hci_Command_Add_Sco_Connection(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_SETUP_SYNC_CONNECTION:
				Hci_Command_Setup_Sync_Connection(devExt, pParam);
				break;
			case BT_HCI_COMMAND_ACCEPT_SYNC_CONNECTION_REQ:
				Hci_Command_Accept_Sync_Connection_Request(devExt, pParam);
				break;
			case BT_HCI_COMMAND_REJECT_SYNC_CONNECTION_REQ:
				Hci_Command_Reject_Sync_Connection_Request(devExt, pParam);
				break;
			default:
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_LINK_POLICY:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): link policy command\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_WRITE_LINK_POLICY_SETTINGS:
				Hci_Command_Write_Link_Policy_Settings(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_ROLE_DISCOVERY:
				Hci_Command_Role_Discovery(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_READ_LINK_POLICY_SETTINGS:
				Hci_Command_Read_Link_Policy_Settings(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_HOLD_MODE:
				Hci_Command_Hold_Mode(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2), *(PUINT16)(pParam + 4));
				break;
			case BT_HCI_COMMAND_SNIFF_MODE:
				Hci_Command_Sniff_Mode(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2), *(PUINT16)(pParam + 4), *(PUINT16)(pParam + 6), *(PUINT16)(pParam + 8));
				break;
			case BT_HCI_COMMAND_EXIT_SNIFF_MODE:
				Hci_Command_Exit_Sniff_Mode(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_SWITCH_ROLE:
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Command does not support ocf = 0x%x\n", pCommandHead->opcode.ocf);
				Hci_Command_Switch_Role(devExt, pParam);
				break;
			case BT_HCI_COMMAND_QOS_SETUP:
				Hci_Command_Qos_Setup(devExt, pParam);
				break;
			case BT_HCI_COMMAND_FLOW_SPECIFICATION:
				Hci_Command_Flow_Specification(devExt, pParam);
				break;
			default:
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): host controller and baseband command\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_RESET:
				Hci_Command_Reset(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_SCAN_ENABLE:
				Hci_Command_Write_Scan_Enable(devExt, pCommandHead->para);
				break;
			case BT_HCI_COMMAND_WRITE_CONNECTION_ACCEPT_TIMEOUT:
				Hci_Command_Write_Conn_Accept_Timeout(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_WRITE_PAGE_TIMEOUT:
				Hci_Command_Write_Page_Timeout(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_WRITE_PIN_TYPE:
				Hci_Command_Write_Pin_Type(devExt, pCommandHead->para);
				break;
			case BT_HCI_COMMAND_CHANGE_LOCAL_NAME:
				Hci_Command_Change_Local_Name(devExt, pParam);
				break;
			case BT_HCI_COMMAND_READ_LOCAL_NAME:
				Hci_Command_Read_Local_Name(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_CLASS_OF_DEVICE:
				Hci_Command_Write_Class_Of_Device(devExt, pParam);
				break;
			case BT_HCI_COMMAND_WRITE_AUTHENTICATION_ENABLE:
				Hci_Command_Write_Authentication_Enable(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_WRITE_ENCRYPTION_MODE:
				Hci_Command_Write_Encryption_Mode(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_WRITE_LINK_SUPERVISION_TIMEOUT:
				Hci_Command_Write_Link_Supervision_Timeout(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_READ_LINK_SUPERVISION_TIMEOUT:
				Hci_Command_Read_Link_Supervision_Timeout(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_WRITE_PAGE_SCAN_ACTIVITY:
				Hci_Command_Write_Page_Scan_Activity(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_ACTIVITY:
				Hci_Command_Write_Inquiry_Scan_Activity(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_READ_ENCRYPTION_MODE:
				Hci_Command_Read_Encryption_Mode(devExt);
				break;
			case BT_HCI_COMMAND_READ_PIN_TYPE:
				Hci_Command_Read_Pin_Type(devExt);
				break;
			case BT_HCI_COMMAND_READ_CONNECTION_ACCEPT_TIMEOUT:
				Hci_Command_Read_Conn_Accept_Timeout(devExt);
				break;
			case BT_HCI_COMMAND_READ_PAGE_TIMEOUT:
				Hci_Command_Read_Page_Timeout(devExt);
				break;
			case BT_HCI_COMMAND_READ_SCAN_ENABLE:
				Hci_Command_Read_Scan_Enable(devExt);
				break;
			case BT_HCI_COMMAND_READ_CLASS_OF_DEVICE:
				Hci_Command_Read_Class_Of_Device(devExt);
				break;
			case BT_HCI_COMMAND_READ_AUTHENTICATION_ENABLE:
				Hci_Command_Read_Authentication_Enable(devExt);
				break;
			case BT_HCI_COMMAND_READ_PAGE_SCAN_ACTIVITY:
				Hci_Command_Read_Page_Scan_Activity(devExt);
				break;
			case BT_HCI_COMMAND_READ_INQUIRY_SCAN_ACTIVITY:
				Hci_Command_Read_Inquiry_Scan_Activity(devExt);
				break;
			case BT_HCI_COMMAND_READ_SCO_FLOW_CONTROL_ENABLE:
				Hci_Command_Read_Sco_Flow_Control_Enable(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_SCO_FLOW_CONTROL_ENABLE:
				Hci_Command_Write_Sco_Flow_Control_Enable(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_FLUSH:
				Hci_Command_Flush(devExt, *(PUINT16)(buf + 4));
				break;
			case BT_HCI_COMMAND_WRITE_AUTOMATIC_FLUSH_TIMEOUT:
				Hci_Command_Write_Automatic_Flush_Timeout(devExt, *(PUINT16)pParam, *(PUINT16)(pParam + 2));
				break;
			case BT_HCI_COMMAND_READ_AUTOMATIC_FLUSH_TIMEOUT:
				Hci_Command_Read_Automatic_Flush_Timeout(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_READ_STORED_LINK_KEY:
				Hci_Command_Read_Stored_Link_Key(devExt, pParam);
				break;
			case BT_HCI_COMMAND_WRITE_STORED_LINK_KEY:
				Hci_Command_Write_Stored_Link_Key(devExt, pParam);
				break;
			case BT_HCI_COMMAND_DELETE_STORED_LINK_KEY:
				Hci_Command_Delete_Stored_Link_Key(devExt, pParam);
				break;
			case BT_HCI_COMMAND_WRITE_INQUIRY_MODE:
				Hci_Command_Write_Inquiry_Mode(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_READ_INQUIRY_MODE:
				Hci_Command_Read_Inquiry_Mode(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_TYPE:
				Hci_Command_Write_Inquiry_Scan_Type(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_READ_INQUIRY_SCAN_TYPE:
				Hci_Command_Read_Inquiry_Scan_Type(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_PAGE_SCAN_TYPE:
				Hci_Command_Write_Page_Scan_Type(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_READ_PAGE_SCAN_TYPE:
				Hci_Command_Read_Page_Scan_Type(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_AFH_CHANNEL_ASSESSMENT_MODE:
				Hci_Command_Write_AFH_Channel_Assessment_Mode(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_READ_AFH_CHANNEL_ASSESSMENT_MODE:
				Hci_Command_Read_AFH_Channel_Assessment_Mode(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_HOLD_MODE_ACTIVITY:
				Hci_Command_Write_Hold_Mode_Activity(devExt, *pParam);
				break;
			case BT_HCI_COMMAND_READ_HOLD_MODE_ACTIVITY:
				Hci_Command_Read_Hold_Mode_Activity(devExt);
				break;
			case BT_HCI_COMMAND_SET_AFH_HOST_CHANNEL_CLASSIFICATION:
				Hci_Command_Set_AFH_Host_Channel_Classification(devExt, pParam);
				break;
			case BT_HCI_COMMAND_WRITE_CURRENT_IAC_LAP:
				Hci_Command_Write_Current_Iac_Lap(devExt, pParam);
				break;
			case BT_HCI_COMMAND_READ_PAGE_SCAN_MODE:
				Hci_Command_Read_Page_Scan_Mode(devExt);
				break;
            case BT_HCI_COMMAND_SET_EVENT_FILTER:
                Hci_Command_Set_Event_Filter(devExt, pParam);
                break;
            case BT_HCI_COMMAND_READ_VOICE_SETTING:
                Hci_Command_Read_Voice_Setting(devExt);
                break;
            case BT_HCI_COMMAND_WRITE_VOICE_SETTING:
                Hci_Command_Write_Voice_Setting(devExt, *(PUINT16)pParam);
                break;
			default:
				if (pCommandHead->opcode.ocf == BT_HCI_COMMAND_FLUSH)
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL0, "HCI_Flush command but we now don't process it yet!\n");
				}
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): informational parameters\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_READ_BUFFER_SIZE:
				Hci_Command_Read_Buffer_Size(devExt);
				break;
			case BT_HCI_COMMAND_READ_LOCAL_VERSION_INFORMATION:
				Hci_Command_Read_Local_Version_Info(devExt);
				break;
			case BT_HCI_COMMAND_READ_BD_ADDR:
				Hci_Command_Read_BD_Addr(devExt);
				break;
			case BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_FEATURES:
				Hci_Command_Read_Local_Supported_Features(devExt);
				break;
			case BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_COMMANDS:
				Hci_Command_Read_Local_Supported_Commands(devExt);
				break;
    		case BT_HCI_COMMAND_READ_LOCAL_EXTENDED_FEATURES:
    			Hci_Command_Read_Local_Extended_Features(devExt, *(pParam));
    			break;
			default:
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_STATUS_PARAMETERS:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): status parameters\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_READ_RSSI:
				Hci_Command_Read_Rssi(devExt, *(PUINT16)pParam);
				break;
			case BT_HCI_COMMAND_READ_AFH_CHANNEL_MAP:
				Hci_Command_Read_AFH_Channel_Map(devExt, *(PUINT16)pParam);
				break;
			default:
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_TESTING:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): testing commands\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_READ_LOOPBACK_MODE:
				Hci_Command_Read_Loopback_Mode(devExt);
				break;
			case BT_HCI_COMMAND_WRITE_LOOPBACK_MODE:
				Hci_Command_Write_Loopback_Mode(devExt, pCommandHead->para);
				break;
			case BT_HCI_COMMAND_ENABLE_DEVICE_UNDER_TEST_MODE:
				Hci_Command_Enable_Device_Under_Test_Mode(devExt);
				break;
			default:
				invalidflag = 1;
			}
			break;
		case BT_HCI_COMMAND_OGF_VENDOR_SPECIFIC:
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessTxCommand(OGF): vendor specific commands\n");
			switch (pCommandHead->opcode.ocf)
			{
			case BT_HCI_COMMAND_READ_ENCRYPTION_KEY_FROM_IVT:
				Hci_Command_Read_IVT_Encryption_Key(devExt);
				break;
			default:
				invalidflag = 1;
			}
			break;
		default:
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "BtProcessTxCommand: OGF can't be matched!\n");
			invalidflag = 1;
	}
	if (invalidflag == 1)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Command does not support ogf=0x%x, ocf=0x%x\n", pCommandHead->opcode.ogf, pCommandHead->opcode.ocf);
		Hci_Command_Unknown_Command(devExt, pCommandHead->opcode.opcode);
	}
	else
	{
		if (Hci_Command_Is_Discard_Command(devExt, pCommandHead->opcode.opcode))
		{
		}
	}
	return STATUS_TRANSLATION_COMPLETE;
}


///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessTxLMPDU
//
//    This function process LMPPDU frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    pConnectDevice - the pointer to connect device.
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
NTSTATUS BtProcessTxLMPDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	PBT_FRAG_T	pFrag;
	UINT32 reallen;
	UINT8 dataType;
	UINT8 ListIndex;
	NTSTATUS status;
	UINT32	IdleSpace;
	LARGE_INTEGER	timevalue;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Return because pFrag = NULL\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	/*
	if (BtIsTxPoolFull(devExt))
	{
		return STATUS_PENDING;
	}
	*/
	if (pConnectDevice != NULL)
	{
		if (pConnectDevice->valid_flag == 0)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Sending LMP PDU..., but connect device does not exis\n");
			//Jakio20080605:we should set a tx timer to triger next transmission
			timevalue.QuadPart = kOneMillisec;
			KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
			BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Return because pConnectDevice->valid_flag == 0 \n");
			return STATUS_SUCCESS;
		}
	}

	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if(pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_ACTIVE)
		{
			dataType = MAILBOX_DATA_TYPE_MASTER;
			ListIndex = FRAG_MASTER_LIST;	
		}
		else
		{
			dataType = MAILBOX_DATA_TYPE_SNIFF;
			ListIndex = FRAG_SNIFF_LIST;		
		}
		
		
	}
	else
	{
		if(pConnectDevice->slave_index == 0)
		{
			dataType = MAILBOX_DATA_TYPE_SLAVE;
			ListIndex = FRAG_SLAVE_LIST;	
		}
		else
		{
			dataType = MAILBOX_DATA_TYPE_SLAVE1;
			ListIndex = FRAG_SLAVE1_LIST;
		}
		
	}
	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = pFrag->IdleSpace[ListIndex];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	
	
	dest = BtGetValidTxPool(devExt);
	if (dest == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Return because dest == NULL\n");
		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(dest, BT_MAX_FRAME_SIZE);
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = pConnectDevice->am_addr;
	pHCBBHead->type = BT_ACL_PACKET_DM1;

	#ifdef BT_TESTMODE_SUPPORT
	pHCBBHead->tx_retry_count = TestMode_GetWhitening(devExt, pLmpPdu);
	#else
	pHCBBHead->tx_retry_count = 0;
	#endif
	pHCBBHead->slave_index = pConnectDevice->slave_index;
	pHCBBHead->master_slave_flag = (pConnectDevice->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;

	pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(dest + sizeof(HCBB_PAYLOAD_HEADER_T));
	pSingHead->l_ch = BT_LCH_TYPE_LMP;
	pSingHead->flow = 1;
	pSingHead->length = PduLength;
	pHCBBHead->length = pSingHead->length + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
	RtlCopyMemory(dest + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), pLmpPdu, PduLength);
	reallen = pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T);

	if(IdleSpace <= ALIGNLONGDATALEN(reallen))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Sending LMP--Not enough space, idle 0x%x, require 0x%x\n", IdleSpace, ALIGNLONGDATALEN(reallen));
		ReleaseBulkOut1Buf(dest, devExt);

		/* jakio20080219: query scratch space info
*/
		timevalue.QuadPart = kOneMillisec;
		KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);

		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Return because Not enough space\n");

		return STATUS_UNSUCCESSFUL;
	}
	
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending LMP PDU..., length 0x%x, type %d\n", ALIGNLONGDATALEN(reallen), dataType);
	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pFrag->IdleSpace[ListIndex] -= ALIGNLONGDATALEN(reallen);
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	status = BtUsbWriteAll(devExt, dest, ALIGNLONGDATALEN(reallen), dataType, 0);
	if(NT_SUCCESS(status))
	{
		#ifdef BT_3DSP_FLUSH_SUPPORT
		Flush_BakupData(devExt, dest, ALIGNLONGDATALEN(reallen), ListIndex);
		#endif
	}
	else
	{
		ReleaseBulkOut1Buf(dest, devExt);	
	}
	
	
	BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Return from bottom\n");
	return status;
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessTxPollFrame
//
//    This function send poll frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    pConnectDevice - the pointer to connect device.
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
NTSTATUS BtProcessTxPollFrame(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	UINT32 reallen, IdleSpace;
	UINT8 ListIndex = FRAG_MASTER_LIST;
	UINT8 dataType = MAILBOX_DATA_TYPE_MASTER;
	PBT_FRAG_T	pFrag = NULL;
	NTSTATUS	status;
	LARGE_INTEGER	timevalue;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "sending poll, but frag pointer is null\n");
		return STATUS_UNSUCCESSFUL;
	}

	/* jakiotodo: return. add flow control codes later
*/

	
	if (BtIsTxPoolFull(devExt))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending Poll pending!\n");
		return STATUS_UNSUCCESSFUL;
	}
	if (pConnectDevice != NULL)
	{
		if (pConnectDevice->valid_flag == 0)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Sending Poll..., but connect device does not exis\n");
			return STATUS_SUCCESS;
		}
	}
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending Poll...\n");
	dest = BtGetValidTxPool(devExt);
	if(dest == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "resource error\n");
		return STATUS_UNSUCCESSFUL;
	}

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = (UINT16)pFrag->IdleSpace[ListIndex];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

	
	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = pConnectDevice->am_addr;
	pHCBBHead->type = BT_ACL_PACKET_POLL;
	pHCBBHead->tx_retry_count = 0;
	pHCBBHead->length = 0;
	pHCBBHead->slave_index = pConnectDevice->slave_index;
	pHCBBHead->master_slave_flag = (pConnectDevice->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;
	reallen = ALIGNLONGDATALEN(pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T));

	if(IdleSpace <= reallen)
	{
	
//		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "sending poll, but space not enough\n");
		ReleaseBulkOut1Buf(dest, devExt);
		/* jakio20080219: query scratch space info
*/
		//UsbQueryDMASpace(devExt);
		timevalue.QuadPart = kOneMillisec;
		KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
		return STATUS_UNSUCCESSFUL;
	}

	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pFrag->IdleSpace[ListIndex] -= reallen;
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	

	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Poll frame length:0x%lx\n", reallen);
	status = BtUsbWriteAll(devExt, dest, reallen, dataType, 0);
	if(NT_SUCCESS(status))
	{
		#ifdef BT_3DSP_FLUSH_SUPPORT
		Flush_BakupData(devExt, dest, reallen, ListIndex);
		#endif
	
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "poll frame sending ok\n");
		return STATUS_SUCCESS;
	}
	else
	{
		ReleaseBulkOut1Buf(dest, devExt);
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Fatal error, poll frame sending failure\n");
		return STATUS_UNSUCCESSFUL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessTxAclNullFrame
//
//    This function send null acl frame.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    pConnectDevice - the pointer to connect device.
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
NTSTATUS BtProcessTxAclNullFrame(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 flow)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	UINT32 reallen, IdleSpace;
	UINT8 ListIndex = FRAG_MASTER_LIST;
	UINT8 dataType;
	PBT_FRAG_T	pFrag = NULL;
	LARGE_INTEGER	timevalue;
	NTSTATUS	status;


	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "sending flow control packet, but frag pointer is null\n");
		return STATUS_UNSUCCESSFUL;
	}


	if (BtIsTxPoolFull(devExt))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending flow control packet pending!\n");
		return STATUS_UNSUCCESSFUL;
	}
	if (pConnectDevice != NULL)
	{
		if (pConnectDevice->valid_flag == 0)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending flow control packet...  flow = %d, but connect device does not exis\n", flow);
			return STATUS_SUCCESS;
		}
	}

	dest = BtGetValidTxPool(devExt);
	if (dest == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "BtProcessTxAclNullFrame--resource error\n");
		return STATUS_UNSUCCESSFUL;
	}


	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)dest;
	pHCBBHead->am_addr = pConnectDevice->am_addr;
	pHCBBHead->type = BT_ACL_PACKET_DM1;
	pHCBBHead->tx_retry_count = 0;
	pHCBBHead->slave_index = pConnectDevice->slave_index;
	pHCBBHead->master_slave_flag = (pConnectDevice->raw_role == BT_ROLE_MASTER) ? BT_HCBB_MASTER_FLAG : BT_HCBB_SLAVE_FLAG;
	pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(dest + sizeof(HCBB_PAYLOAD_HEADER_T));
	pSingHead->l_ch = BT_LCH_TYPE_START_L2CAP;
	pSingHead->flow = flow;
	pSingHead->length = 0;
	pHCBBHead->length = pSingHead->length + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T);
	reallen = ALIGNLONGDATALEN(pHCBBHead->length + sizeof(HCBB_PAYLOAD_HEADER_T));

	ListIndex = (pConnectDevice->raw_role == BT_ROLE_MASTER) ? FRAG_MASTER_LIST : FRAG_SLAVE_LIST;
	dataType = (pConnectDevice->raw_role == BT_ROLE_MASTER) ? MAILBOX_DATA_TYPE_MASTER : MAILBOX_DATA_TYPE_SLAVE;
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	IdleSpace = (UINT16)pFrag->IdleSpace[ListIndex];
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	if(IdleSpace <= reallen)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "sending flow control packet, but space not enough.... flow = %d\n", flow);
		ReleaseBulkOut1Buf(dest, devExt);

		timevalue.QuadPart = kOneMillisec;
		KeSetTimer(&pFrag->TxTimer, timevalue, &pFrag->TxDPC);
		return STATUS_UNSUCCESSFUL;
	}

	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	pFrag->IdleSpace[ListIndex] -= reallen;
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);	

	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "flow control packet length:0x%lx\n", reallen);
	status = BtUsbWriteAll(devExt, dest, reallen, dataType, 0);
	if(NT_SUCCESS(status))
	{
		#ifdef BT_3DSP_FLUSH_SUPPORT
		Flush_BakupData(devExt, dest, reallen, ListIndex);
		#endif
	
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Sending flow control packet...  flow = %d\n", flow);
		return STATUS_SUCCESS;
	}
	else
	{
		ReleaseBulkOut1Buf(dest, devExt);
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Fatal error, flow control packet sending failure\n");
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}


/*
 * For HCI Packet debug 
 * 
*/
#if 0
void dump_hci_packet(struct sk_buff *skb)
{
	struct hci_event_hdr *evt_hdr = (struct hci_event_hdr *)skb->data;
	struct hci_ev_cmd_complete * ec = (struct hci_ev_cmd_complete *)(skb->data + sizeof(*evt_hdr));
	int i = 0;
	char *temp;

	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "++++++++++ DUMP ++++++++++\n");
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Packet type %d\n", bt_cb(skb)->pkt_type);

	switch(bt_cb(skb)->pkt_type){
	case BT_HCI_PACKET_EVENT:
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Event packet ::::\n");
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Event code 0x%x, len 0x%x\n", evt_hdr->evt, evt_hdr->plen);
		if(0x0e == *(skb->data)){
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "ogf 0x%x, ocf 0x%x\n", hci_opcode_ogf(ec->opcode), hci_opcode_ocf(ec->opcode));
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "\t Params:\n");
			i = evt_hdr->plen - sizeof(*ec);
			temp = skb->data + sizeof(*evt_hdr) + sizeof(*ec);
			while(i--){
				BT_DBGSUB(ZONE_SDRCV | LEVEL3, "0x%x ", *temp++);
			}
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "\n");
		}
		break;
	case BT_HCI_PACKET_SCO_DATA:
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO packet ::::\n");
		{
			PHCI_SCO_HEADER_T phead= (PHCI_SCO_HEADER_T)skb->data;
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Connection Handle 0x%x, length 0x%x, skb len 0x%x\n", phead->connection_handle, phead->total_len, skb->len);
		}
		break;
	case BT_HCI_PACKET_ACL_DATA:
		{
			PHCI_DATA_HEADER_T phead = (PHCI_DATA_HEADER_T)skb->data;
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "ACL packet ::::\n");
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Connection Handle 0x%x, length 0x%x\n", phead->connection_handle, phead->total_len);
		}
		break;
	default:
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Error packet type ::::\n");
	}
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "--------------------------\n");
}
#endif

/* Send the SCO packet up to the HCI level */
void notify_up_sco_packet(PHCI_SCO_HEADER_T head, struct hci_dev *hdev,PINT8 buf, UINT32 len)
{
    struct sk_buff *skb = NULL;
    UINT32 head_len = sizeof(HCI_SCO_HEADER_T);
    
	if(0 == len){
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "0 SCO packet\n");
		return;
	}

    skb = bt_skb_alloc((len + head_len), GFP_ATOMIC);
    if(!skb){
        BT_ERR("fail alloc bt skb\n");
		return;
	}

	/* The dev of skb must be configured, or the 
	* HCI event will not be recognized by upper layer
	*/
    skb->dev = (void *)hdev;        
	/* One packet is done */
	head->total_len = len;
    memcpy(skb_put(skb, head_len), head, head_len);
    memcpy(skb_put(skb, len), buf, len);
    bt_cb(skb)->pkt_type = BT_HCI_PACKET_SCO_DATA;

    //dump_hci_packet(skb);
    hci_notify_frame(skb);

}


/* Process the received data packet */
NTSTATUS BtProcessRx_skb(IN PBT_DEVICE_EXT devExt)
{
	NTSTATUS code = STATUS_SUCCESS;
	KIRQL oldIrql;
	UINT32 OutLen;
	PBT_FRAG_T pFrag;
	UINT8 pktType, id = 0;
	struct sk_buff *skb = NULL;
	PINT8 pTmpBuf = NULL;
	PBT_HCI_T	pHci = NULL;

	/* Process event and ACL data */
	while(!BtIsRxCachEmpty(devExt)){
		skb = bt_skb_alloc(BT_MAX_FRAME_SIZE, GFP_ATOMIC);
        if(!skb){
            BT_ERR("fail alloc bt skb\n");
			return -1;
		}

		/* The dev of skb must be configured, or the 
		* HCI event will not be recognized by upper layer
		*/
        skb->dev = (void *)devExt->hdev;

        pTmpBuf = skb->data;
    	
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Processing RX frame\n");
   	 	code = BtProcessRxData(devExt, pTmpBuf, BT_MAX_FRAME_SIZE, devExt->RxCach[devExt->RxCachCid], &OutLen, &pktType);
   	 	if(code == STATUS_RESOURCE_DATA_NOT_FOUND)	//Jakio20090218: check if role switch is in process
   	 	{
			//DebugPrint("BtProcessRx---resource NOT found\n");
			pHci = (PBT_HCI_T)devExt->pHci;
			if((pHci != NULL) && (pHci->role_switching_flag == 1))
			{
				//don't get next rx cach in this case
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRx---role switching flag is set, give a chance for this packet\n");
    			code = STATUS_SUCCESS;
    			kfree_skb(skb);

	    		//trigger it again
	    		// Just raise the IRQL and process the task immudiatly in another DPC process.
    			tasklet_schedule(&devExt->taskletRcv);
    			break;
    		}
    		else
    		{
    		    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
    			/* This rx cach has been processed, so we move to next rx cach */
    			devExt->RxCachCid = BtGetNextRxCachId(devExt->RxCachCid);
                KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
    		}
		}
		else if (code != STATUS_BUFFER_TOO_SMALL)
    	{
			/* One packet is done */
            skb_put(skb, OutLen);
            bt_cb(skb)->pkt_type = pktType;

			/* For debug using */
            //dump_hci_packet(skb);
            hci_notify_frame(skb);

			if (code == STATUS_MORE_PROCESSING_REQUIRED){
				/* The RX Cache has more packet, so do not move rx cach pointer */
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Require more processing\n");
			}
			else
			{
    		    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
				/* This rx cach has been processed, so we move to next rx cach */
				devExt->RxCachCid = BtGetNextRxCachId(devExt->RxCachCid);	
                KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			}
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL1, "buffer too small!\n");
            kfree_skb(skb);
		}
	}


	/* Process SCO data */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	while(!Frag_IsAllScoRxCachEmptyAndRetId(pFrag, &id)){

		pTmpBuf = devExt->pBufBridge;
		code = Frag_ProcessRxScoData(devExt, pTmpBuf, BT_MAX_FRAME_SIZE + 1, pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachCid].RxCach, &OutLen);
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "$$$$ RX SCO, len: %u $$$$\n", OutLen);
		if (code == STATUS_SUCCESS){
			/* Break up to 48 bytes HCI SCO packet */
			PHCI_SCO_HEADER_T sco_head = (PHCI_SCO_HEADER_T)pTmpBuf; 
			PUINT8 tbuf = pTmpBuf;
			UINT32 rcv_len;

			/* Filter out the SCO header */
			tbuf += sizeof(HCI_SCO_HEADER_T);
			rcv_len = OutLen - sizeof(HCI_SCO_HEADER_T);

			while(rcv_len >= PROTO_SCO_PACKET_LEN){
				notify_up_sco_packet(sco_head, devExt->hdev, tbuf, PROTO_SCO_PACKET_LEN);
				rcv_len -= PROTO_SCO_PACKET_LEN;
				tbuf += PROTO_SCO_PACKET_LEN;
			}
			/* Send the tail bytes */
			notify_up_sco_packet(sco_head, devExt->hdev, tbuf, rcv_len);

		    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
			/* This rx cach has been processed, so we move to next rx cach */
			pFrag->RxScoElement[id].RxCachCid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachCid);
            KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		}
		else{
			if(code == STATUS_RESOURCE_DATA_NOT_FOUND){
				BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Noise in SCO receive\n");
			}
			else if(code == STATUS_BUFFER_TOO_SMALL){
				BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Buffer too small in SCO receive\n");
			}
		}       
	}    
	return code;
}


/************************************************************************
*BtReadData:
*Description:
*	This function process the reading IRP.  And in order to replace the routine 'BtProcessRx'
*Changelog:
*	Jakio20071229 write this function to support the function that IVT can read multi buffer from
*	bt driver, this can save times of BtRead()
*************************************************************************/
NTSTATUS BtReadData(IN PBT_DEVICE_EXT devExt, IN OUT PUINT32 pOutLen)
{
	NTSTATUS code = STATUS_SUCCESS;
#if 0
	UINT32	tmpReadLen = 0;  //temp length when handling a rx cach
	UINT32	ConsumedLen = 0; //the total length have hanled

	ioStack = IoGetCurrentIrpStackLocation(Irp);
	ReadLen = ioStack->Parameters.Read.Length;
	pReadBuf = (PUINT8)Irp->AssociatedIrp.SystemBuffer;


	IoAcquireCancelSpinLock(&CancelIrql);
	IoSetCancelRoutine(Irp, NULL);
	IoReleaseCancelSpinLock(CancelIrql);

	*pOutLen = 0;
	tmpReadLen = 0;
	ConsumedLen = 0;

	if(!BtIsRxCachEmpty(devExt))
	{
		code = BtProcessRxData(devExt, pReadBuf, ReadLen, devExt->RxCach[devExt->RxCachCid], pOutLen);
		if (code != STATUS_BUFFER_TOO_SMALL)
		{
			if (code == STATUS_MORE_PROCESSING_REQUIRED)
			{
				code = STATUS_SUCCESS;
				return code;
			}
			else
			{
				devExt->RxCachCid = BtGetNextRxCachId(devExt->RxCachCid);
			}
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "buffer too small!\n");
			return code;
		}
	}

	ConsumedLen = *pOutLen;
	if(ReadLen <= ConsumedLen)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "buffer is not enough for read sco data\n");
		return code;
	}

	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "frag pointer is null, return\n");
		return code;
	}

	if(Frag_IsAllScoRxCachEmptyAndRetId(pFrag, &id))
	{
			return code;
	}

	code = Frag_ProcessRxScoData(devExt, pReadBuf+ConsumedLen, ReadLen-ConsumedLen, 
			pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachCid].RxCach, &tmpReadLen);
	if(code == STATUS_BUFFER_TOO_SMALL)
	{
		if(ConsumedLen != 0)
			code = STATUS_SUCCESS;
		else
			BT_DBGEXT(ZONE_SDRCV | LEVEL1, "buffer too small\n");
		return code;
	}
	else if(code == STATUS_MORE_PROCESSING_REQUIRED)
	{
		*pOutLen += tmpReadLen;
		code = STATUS_SUCCESS;
	}
	else
	{
		*pOutLen += tmpReadLen;
		pFrag->RxScoElement[id].RxCachCid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachCid);
		code = STATUS_SUCCESS;
	}
#endif
	return code;


}











///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxData
//
//    This function process the rx cach and copy the converted frame to irp's
//    system buffer.
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//    source - Address of the frame.
//    inlen - Length of the frame.
//    dest - Address of the converted frame.
//    poutlen - Length of the converted frame.
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
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS BtProcessRxData(PBT_DEVICE_EXT devExt, PUINT8 source, UINT32 inlen, PUINT8 dest, PUINT32 poutlen, PUINT8 pPktType)
{
	UINT16 DataLen;
	UINT16 FrameType;
	PHCI_DATA_HEADER_T phead;
	PHCI_SCO_HEADER_T pscohead;
	PCONNECT_DEVICE_T pConnectDevice;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PPAYLOAD_HEADER_MULTI_SLOT_T pMultiHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	PBT_HCI_T pHci;
	UINT8 i;
	UINT8 air_mode;
	DataLen = *(PUINT16)dest;
	FrameType = *(PUINT16)(dest + sizeof(UINT16));
	if (FrameType == (UINT16)RX_FRAME_TYPE_DATA)
	{
		*pPktType = BT_HCI_PACKET_ACL_DATA; // initial value
		phead = (PHCI_DATA_HEADER_T)(source);
		pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
		pHci = (PBT_HCI_T)devExt->pHci;
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
			if ((pHCBBHead->type == BT_ACL_PACKET_DM1) || (pHCBBHead->type == BT_ACL_PACKET_DH1) || (pHCBBHead->type == BT_ACL_PACKET_AUX1))
			{
				*poutlen = sizeof(HCI_DATA_HEADER_T) + ((*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T)) &0xf8) >> 3);
			}
			else
			{
				*poutlen = sizeof(HCI_DATA_HEADER_T) + ((*(PUINT16)(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T)) &0x0ff8) >> 3);
			}
#ifdef BT_TESTDRIVER
			devExt->RealRecvNotSuccCount++;
#endif
			return STATUS_RESOURCE_DATA_NOT_FOUND;
		}
		if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
		{
			Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
		}
		#ifdef BT_SCHEDULER_SUPPORT
			Sched_SendPollFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
			if (pConnectDevice->l2cap_rx_flow_control_flag == 1)
				Sched_SendAclNullContinueFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
	 	#endif
		if ((pConnectDevice->edr_mode == 0) && pHCBBHead->type == BT_SCO_PACKET_DV)
		{
			pscohead = (PHCI_SCO_HEADER_T)(source);
			*pPktType = BT_HCI_PACKET_SCO_DATA; // SCO data
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				pscohead->connection_handle = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle;
			}
			else
			{
				pscohead->connection_handle = 0;
			}
			pscohead->reserved = 0;
			pscohead->total_len = (UINT8)PACKET_MAX_BYTES[BT_SCO_PACKET_HV1];
			if (inlen < sizeof(HCI_SCO_HEADER_T) + pscohead->total_len)
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
			for (i = 0; i < pscohead->total_len / 2; i++)
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
			RtlCopyMemory(source + sizeof(HCI_SCO_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), pscohead->total_len);
#endif
			*poutlen = sizeof(HCI_SCO_HEADER_T) + pscohead->total_len;
#ifdef DBG
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Read file data:\n");
#endif
#ifdef BT_TESTDRIVER
#endif
			pHCBBHead->type = BT_ACL_PACKET_DM1;
			RtlMoveMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2, ((*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2) &0xf8) >> 3) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T));
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
		else if ((pHCBBHead->type == BT_SCO_PACKET_HV1) || (pHCBBHead->type == BT_SCO_PACKET_HV2) || (pHCBBHead->type == BT_SCO_PACKET_HV3))
		{
			pscohead = (PHCI_SCO_HEADER_T)(source);
			*pPktType = BT_HCI_PACKET_SCO_DATA; // SCO data
			if (pConnectDevice->pScoConnectDevice != NULL)
			{
				pscohead->connection_handle = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle;
			}
			else
			{
				pscohead->connection_handle = 0;
			}
			pscohead->reserved = 0;
			pscohead->total_len = (UINT8)PACKET_MAX_BYTES[pHCBBHead->type];
			if (inlen < sizeof(HCI_SCO_HEADER_T) + pscohead->total_len)
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
			for (i = 0; i < pscohead->total_len / 2; i++)
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
			RtlCopyMemory(source + sizeof(HCI_SCO_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), pscohead->total_len);
#endif
			*poutlen = sizeof(HCI_SCO_HEADER_T) + pscohead->total_len;
		}
		else
		{
			*pPktType = BT_HCI_PACKET_ACL_DATA; // ACL data
			if ((pHCBBHead->type == BT_ACL_PACKET_DM1) || (pHCBBHead->type == BT_ACL_PACKET_AUX1) || (pHCBBHead->type == BT_ACL_PACKET_DH1 && pConnectDevice->edr_mode == 0))
			{
				pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T));
				phead->connection_handle = pConnectDevice->connection_handle;
				phead->pb_flag = pSingHead->l_ch;
				phead->bc_flag = 0; // ??
				phead->total_len = pSingHead->length;
				if (inlen < (sizeof(HCI_DATA_HEADER_T) + phead->total_len))
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "singleslot. buffer too small. inlen = %lu, real len = %d\n", inlen, (sizeof(HCI_DATA_HEADER_T) + phead->total_len));
#ifdef BT_TESTDRIVER
					devExt->RealRecvNotSuccCount++;
#endif
					return STATUS_BUFFER_TOO_SMALL;
				}
				RtlCopyMemory(source + sizeof(HCI_DATA_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), phead->total_len);
			}
			else
			{
			#ifdef BT_SUPPROT_RX_FRAG
				pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T));
				phead->connection_handle = pConnectDevice->connection_handle;
				phead->pb_flag = pMultiHead->l_ch;
				phead->bc_flag = 0; // ??
				phead->total_len = pMultiHead->length;
				if (phead->total_len >= BT_ACL_RX_MAX_LIMIT) // should frag
				{
					if (inlen < sizeof(HCI_DATA_HEADER_T) + BT_ACL_RX_FRAGMENT_THRESHOLD)
					{
					#ifdef BT_TESTDRIVER
						devExt->RealRecvNotSuccCount++;
					#endif
						return STATUS_BUFFER_TOO_SMALL;
					}
					RtlCopyMemory(source + sizeof(HCI_DATA_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), BT_ACL_RX_FRAGMENT_THRESHOLD);
					phead->total_len = BT_ACL_RX_FRAGMENT_THRESHOLD;
					pMultiHead->l_ch = BT_LCH_TYPE_CON_L2CAP;
					pMultiHead->length -= BT_ACL_RX_FRAGMENT_THRESHOLD;
					RtlMoveMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T) + BT_ACL_RX_FRAGMENT_THRESHOLD, pMultiHead->length);
					*poutlen = sizeof(HCI_DATA_HEADER_T) + phead->total_len;
				#ifdef DBG
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Read fragment file data:\n");
				#endif
					return STATUS_MORE_PROCESSING_REQUIRED;
				}
				else
				{
					if (inlen < sizeof(HCI_DATA_HEADER_T) + phead->total_len)
					{
					#ifdef BT_TESTDRIVER
						devExt->RealRecvNotSuccCount++;
					#endif
						return STATUS_BUFFER_TOO_SMALL;
					}
					RtlCopyMemory(source + sizeof(HCI_DATA_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), phead->total_len);
				}
			#else
				pMultiHead = (PPAYLOAD_HEADER_MULTI_SLOT_T)(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T));
				phead->connection_handle = pConnectDevice->connection_handle;
				phead->pb_flag = pMultiHead->l_ch;
				phead->bc_flag = 0; // ??
				phead->total_len = pMultiHead->length;
				if (inlen < (sizeof(HCI_DATA_HEADER_T) + phead->total_len))
				{

					BT_DBGEXT(ZONE_SDRCV | LEVEL1, "multislot. buffer too small. inlen = %d, real len = %d\n", inlen, (sizeof(HCI_DATA_HEADER_T) + phead->total_len));
					#ifdef BT_TESTDRIVER
					devExt->RealRecvNotSuccCount++;
					#endif
					return STATUS_BUFFER_TOO_SMALL;
				}
				RtlCopyMemory(source + sizeof(HCI_DATA_HEADER_T), dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_MULTI_SLOT_T), phead->total_len);
			#endif	
			}
			*poutlen = sizeof(HCI_DATA_HEADER_T) + phead->total_len;
		}
#ifdef DBG
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Read file data:\n");
#endif
#ifdef BT_TESTDRIVER
#endif
	}
	else
	{
#ifdef BT_LOOPBACK_SUPPORT
		if (FrameType == (UINT16)RX_FRAME_TYPE_EVENT)
		{
			*pPktType = BT_HCI_PACKET_EVENT;
			if (inlen < (UINT32)(DataLen))
			{
				return STATUS_BUFFER_TOO_SMALL;
			}
			RtlCopyMemory(source, dest + sizeof(UINT32), DataLen);
			*poutlen = DataLen;
		}
		else if (FrameType == (UINT16)RX_FRAME_TYPE_LOOPBACK_DATA)
		{
			*pPktType = BT_HCI_PACKET_ACL_DATA; // initial value
			RtlCopyMemory(source, dest + sizeof(UINT32), sizeof(UINT32));
			RtlCopyMemory(source+sizeof(UINT32), dest + sizeof(UINT32) + sizeof(UINT32), *(PUINT16)(dest + sizeof(UINT32) + sizeof(UINT16)));
			*poutlen = sizeof(HCI_DATA_HEADER_T) + *(PUINT16)(dest + sizeof(UINT32) + sizeof(UINT16));
		}
		else
		{
			*pPktType = BT_HCI_PACKET_SCO_DATA; // initial value
			RtlCopyMemory(source, dest + sizeof(UINT32), 3);
			RtlCopyMemory(source+3, dest + sizeof(UINT32) + 3, *(dest + sizeof(UINT32) + sizeof(UINT16)));
			*poutlen = sizeof(HCI_SCO_HEADER_T) + *(dest + sizeof(UINT32) + sizeof(UINT16));
		}
#else
		*pPktType = BT_HCI_PACKET_EVENT;
		if (inlen < (UINT32)(DataLen))
		{
			return STATUS_BUFFER_TOO_SMALL;
		}
		RtlCopyMemory(source, dest + sizeof(UINT32), DataLen);
		*poutlen = DataLen;
#endif
	}
	return STATUS_SUCCESS;
}

VOID BtProcessRx_New(PBT_DEVICE_EXT devExt, UINT8 RecvAreaIndex)
{
	KIRQL oldIrql;
	PBT_RX_BUF_HEAD pRxBufHead;
	PBT_RX_BUF_TAIL pRxBufTail;
	PUINT8 pHeadDesc, pTailDesc;
	UINT32 realDataLength;
	PUINT8 pBuffer;
	UINT32 tmpstartaddr, tmptailaddr;
	UINT32 chanNum;
	BT_RX_FRAME_TYPE tmprxtype;
	UINT8 tmptype;
	UINT32 consumeLength;
	UINT32 DestLen;
	LARGE_INTEGER timevalue;
	PUINT8 dest;
	UINT8 am_addr;
	PHCBB_PAYLOAD_HEADER_T ptmphcbb;
	UINT8 master_slave_flag;
	UINT8 slave_index;
	UINT8 CachIndex;
	PUINT8 pLMPbuf;
	PCONNECT_DEVICE_T pConnectDevice;
    PSCO_CONNECT_DEVICE_T pScoConDevice;
	UINT8 needsubmitscoflag = 0;
	UINT32 tmpretrycount;
	UINT8 tmpflow;
	PBT_FRAG_T pFrag;
	UINT8 id;
	UINT8	PktCount = 0;

	ASSERT(RecvAreaIndex < MAX_USB_BUKIN_AREA);
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	pBuffer = devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvAreaIndex]->buffer;
	if (pBuffer == NULL)
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		return ;
	}
	DestLen = realDataLength =devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvAreaIndex]->length;
	consumeLength = 0;
	pHeadDesc = pBuffer;
	pTailDesc = pBuffer;
	pRxBufHead = NULL;
	pRxBufTail = NULL;
	tmpstartaddr = tmptailaddr = 0;
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	while ((consumeLength < DestLen) && (pHeadDesc))
	{
		PktCount++;
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
		pRxBufHead = (PBT_RX_BUF_HEAD)pHeadDesc;
		if ((pRxBufHead->Len == pRxBufHead->Len1) && (pRxBufHead->Len1 == 0))
		{
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			return ;
		}
		if ((pRxBufHead->Len == pRxBufHead->Len1))// && (pRxBufHead->Len1 == pRxBufHead->Len2))
		{
			tmpstartaddr = sizeof(UINT32);
			tmptailaddr = sizeof(UINT32) + ALIGNLONGDATALEN(pRxBufHead->Len);
			pTailDesc = pHeadDesc + tmptailaddr;
			pRxBufTail = (PBT_RX_BUF_TAIL)pTailDesc;
			if (devExt->IsFirstRxFrame)
			{
				devExt->IsFirstRxFrame = FALSE;
				devExt->RxSeqNo = (UINT8)pRxBufTail->SeqNo;
			}
			else
			{
				devExt->RxSeqNo++;
			}
			if ((UINT8)pRxBufTail->SeqNo == devExt->RxSeqNo)
			{
				tmprxtype = BtGetRxFrameTypeAndRetryCount(devExt, tmpstartaddr, &tmpretrycount, &am_addr, &tmptype, &tmpflow, &master_slave_flag, &slave_index, pHeadDesc);
				devExt->TxRetryCount = tmpretrycount;
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "the type = %x\n", tmprxtype);
				if (tmprxtype == RX_FRAME_TYPE_DATA)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->l2cap_flow_control_flag = tmpflow;
						if ((pConnectDevice->l2cap_flow_control_flag == 1) && (pConnectDevice->timer_l2cap_flow_valid == 0))
						{
							Hci_StartL2capFlowTimer(devExt,pConnectDevice);
						}
						else if ((pConnectDevice->l2cap_flow_control_flag == 0) && (pConnectDevice->timer_l2cap_flow_valid == 1))
						{
							Hci_StopL2capFlowTimer(devExt,pConnectDevice);
						}
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					if (BtIsRxCachFull(devExt))
					{
						needsubmitscoflag = 1;

						BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx cach is full!\n");
						pHeadDesc += (tmptailaddr + sizeof(UINT32));
						consumeLength += (tmptailaddr + sizeof(UINT32));
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						continue;
					}
					if(BtIsRxCachToBeFull(devExt))
					{
					#ifdef BT_SCHEDULER_SUPPORT
						if (pConnectDevice != NULL)
						{
							if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
							{
								Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
							}
						}
					#endif
					}
					
					dest = devExt->RxCach[devExt->RxCachPid];
					BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
					devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);

					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					tmptype = (*(dest + sizeof(UINT32)) &0x78) >> 3;
					if ((tmptype == BT_SCO_PACKET_HV1) || (tmptype == BT_SCO_PACKET_HV2) || (tmptype == BT_SCO_PACKET_HV3))
					{
					}
					else
					{
						if ((tmptype == BT_ACL_PACKET_DM1) || (tmptype == BT_ACL_PACKET_AUX1) || (tmptype == BT_ACL_PACKET_DH1 && pConnectDevice != NULL && pConnectDevice->edr_mode == 0))
						{
						}
						else
						{
						}
					}

					needsubmitscoflag = 1;
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
				}
				else if (tmprxtype == RX_FRAME_TYPE_NULL)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					BtProcessRxNULLFrame_New(devExt, tmpstartaddr, pHeadDesc);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_POLL)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);

					BtProcessRxPoolFrame(devExt, pHeadDesc);
					/*
					Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_POLL_FRAME_PROCESS), BT_TASK_PRI_NORMAL, pHeadDesc, 64);
					sc_spin_lock_bh(&devExt->RxStateLock);
					devExt->RxState = RX_STATE_CONNECTING;
					devExt->RxPacketNum = 1;
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "RxPacketNum is %d\n", devExt->RxPacketNum);
					sc_spin_unlock_bh(&devExt->RxStateLock);
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRx_New()--Rx state changed to RX_STATE_CONNECTING\n");
					*/
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_CRC_ERROR)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					BtProcessRxCRCErrorFrame(devExt, master_slave_flag, am_addr, slave_index);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_DV)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

					if (pConnectDevice != NULL)
					{
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					
					if (BtIsRxCachFull(devExt))
					{
						needsubmitscoflag = 1;

						BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx cach is full!\n");
						pHeadDesc += (tmptailaddr + sizeof(UINT32));
						consumeLength += (tmptailaddr + sizeof(UINT32));
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						continue;
					}	
					if(BtIsRxCachToBeFull(devExt))
					{
					#ifdef BT_SCHEDULER_SUPPORT
						if (pConnectDevice != NULL)
						{
							if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
							{
								Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
							}
						}
					#endif	
					}
					
					dest = devExt->RxCach[devExt->RxCachPid];
					BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
					devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
					needsubmitscoflag = 1;
					if ((*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2) &0x3) == (UINT8)BT_LCH_TYPE_LMP)
					{
						ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
						ptmphcbb->type = BT_SCO_PACKET_HV1;

						CachIndex = BtGetFreeLMPCach(devExt);
						if(CachIndex < MAX_LMP_CACH_COUNT)
						{
							devExt->RxLMPPDUCach[CachIndex].Flag_InUse = TRUE;
							pLMPbuf = devExt->RxLMPPDUCach[CachIndex].buffer;

							*(PUINT32)pLMPbuf = *(PUINT16)dest - PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2;
							*(PUINT32)(pLMPbuf + sizeof(UINT32)) = *(PUINT32)(dest + sizeof(UINT32));
							RtlCopyMemory(pLMPbuf + sizeof(UINT32) + sizeof(INT32), dest + sizeof(UINT32) + sizeof(UINT32) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2, 18);

							KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
							BtProcessRxLMPPDU(devExt, CachIndex);
						}
						else
						{
							BT_DBGEXT(ZONE_SDRCV | LEVEL0, "No lmp cach, error\n");
						}

						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					}
					else
					{
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					}

					needsubmitscoflag = 1;
				}
				else if (tmprxtype == RX_FRAME_TYPE_SCO)
				{
					pFrag = (PBT_FRAG_T)devExt->pFrag;
					needsubmitscoflag = 0;

					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
					    BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Reset sco null count %d, connect device addr 0x%x\n", pConnectDevice->rx_sco_con_null_count, pConnectDevice);
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
						    BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO Frame, Reset BT_TIMER_TYPE_LINK_SUPERVISION\n");
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
                        pScoConDevice = pConnectDevice->pScoConnectDevice;
						if (pScoConDevice != NULL)
						{
							id = pScoConDevice->index;
							ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
							BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO Frame SCO index = %d\n", id);

							if(pScoConDevice->NeedSendScoNull)
							{
								pScoConDevice->NeedSendScoNull = FALSE;
							}

							if (Frag_IsScoRxCachFull(pFrag, id))
							{
								needsubmitscoflag = 1;
								BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx sco cach is full!\n");
								
								pHeadDesc += (tmptailaddr + sizeof(UINT32));
								consumeLength += (tmptailaddr + sizeof(UINT32));
								KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
								continue;
							}
							
							dest = pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + pFrag->RxScoElement[id].currentpos;
							if (pFrag->RxScoElement[id].currentpos == 0)
							{
								BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
								pFrag->RxScoElement[id].currentpos += (pRxBufHead->Len + sizeof(UINT32));
								pFrag->RxScoElement[id].currentlen += (1+sizeof(HCI_SCO_HEADER_T) + (pRxBufHead->Len - 4) *2);
							}
							else
							{
								BtTransferRxFrameBodyToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
								pFrag->RxScoElement[id].currentpos += (pRxBufHead->Len - 4);
								pFrag->RxScoElement[id].currentlen += ((pRxBufHead->Len - 4) *2);
							}
							pFrag->RxScoElement[id].totalcount += (pRxBufHead->Len - 4);
							if (pFrag->RxScoElement[id].totalcount >= BT_SCO_DEFRAG_LIMIT)
							{
								BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRx_New()--sco data full, inform IVT to get data\n");
								ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + sizeof(UINT32));
								ptmphcbb->length = pFrag->RxScoElement[id].totalcount;
								pFrag->RxScoElement[id].RxCachPid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachPid);
								KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
								KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
								pFrag->RxScoElement[id].currentpos = 0;
								pFrag->RxScoElement[id].currentlen = 0;
								pFrag->RxScoElement[id].totalcount = 0;
								needsubmitscoflag = 1;
                                if(pScoConDevice->txRxBalance)
                                {
                                    pScoConDevice->txRxBalance--;
                                }
							}
						}
					}
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_SCO_NULL)
				{
					pFrag = (PBT_FRAG_T)devExt->pFrag;
					needsubmitscoflag = 0;
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						ASSERT((tmptype >= (UINT8)BT_SCO_PACKET_HV1) && (tmptype <= (UINT8)BT_SCO_PACKET_HV3));
						pConnectDevice->rx_sco_con_null_count += (PACKET_MAX_BYTES[tmptype] / 2);
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "rx_sco_con_null_count = %lu, connect device addr 0x%x\n", pConnectDevice->rx_sco_con_null_count, pConnectDevice);
						if (pConnectDevice->rx_sco_con_null_count >= BT_RX_SCO_CON_NULL_COUNT_LIMIT)
						{
							if (pConnectDevice->rx_sco_con_null_count <= (BT_RX_SCO_CON_NULL_COUNT_LIMIT + 120))
							{
								BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Start Timer for SCO NULL frame!\n");
								Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, 1);
							}
						}
						else
						{
							if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
							{
								Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
							}
                            pScoConDevice = pConnectDevice->pScoConnectDevice;
							if (pScoConDevice != NULL)
							{
								id = pScoConDevice->index;
								ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
								BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO NULL SCO index = %d\n", id);

								if(pScoConDevice->NeedSendScoNull)
								{
									pScoConDevice->NeedSendScoNull = FALSE;
								}
                                // Ignore the NULL SCO Rx, if the TX/RX is almost balanced
                                if(pScoConDevice->txRxBalance <= 2)
                                {
									pHeadDesc += (tmptailaddr + sizeof(UINT32));
									consumeLength += (tmptailaddr + sizeof(UINT32));
                                    KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
									continue;
                                }

								if (Frag_IsScoRxCachFull(pFrag, id))
								{
								    BT_DBGEXT(ZONE_SDRCV | LEVEL1, "SCO NULL SCO RX cache full\n");
									needsubmitscoflag = 1;
									pHeadDesc += (tmptailaddr + sizeof(UINT32));
									consumeLength += (tmptailaddr + sizeof(UINT32));
									KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
									continue;
								}
								
								dest = pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + pFrag->RxScoElement[id].currentpos;
								if (pFrag->RxScoElement[id].currentpos == 0)
								{
								    BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO NULL fake data\n");
									*(PUINT16)dest = (UINT16)(PACKET_MAX_BYTES[tmptype] / 2+sizeof(HCBB_PAYLOAD_HEADER_T));
									*(PUINT16)(dest + sizeof(UINT16)) = (UINT16)RX_FRAME_TYPE_DATA;
									ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
									ptmphcbb->am_addr = am_addr;
									ptmphcbb->tx_retry_count = tmpretrycount;
									ptmphcbb->type = tmptype;
									ptmphcbb->length = PACKET_MAX_BYTES[tmptype] / 2;

									ptmphcbb->master_slave_flag = master_slave_flag;
									if (pScoConDevice->air_mode == BT_AIRMODE_A_LAW)
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x1);
									}
									else if (pScoConDevice->air_mode == BT_AIRMODE_MU_LAW)
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x2);
									}
									else
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x55);
									}
									pFrag->RxScoElement[id].currentpos += (PACKET_MAX_BYTES[tmptype] / 2+sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(UINT32));
									pFrag->RxScoElement[id].currentlen += (1+sizeof(HCI_SCO_HEADER_T) + PACKET_MAX_BYTES[tmptype]);
								}
								else
								{
									if (pScoConDevice->air_mode == BT_AIRMODE_A_LAW)
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x1);
									}
									else if (pScoConDevice->air_mode == BT_AIRMODE_MU_LAW)
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x2);
									}
									else
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x55);
									}
									pFrag->RxScoElement[id].currentpos += (PACKET_MAX_BYTES[tmptype] / 2);
									pFrag->RxScoElement[id].currentlen += PACKET_MAX_BYTES[tmptype];
#ifdef DBG
									BT_DBGEXT(ZONE_SDRCV | LEVEL3, "receive sco null data:\n");
#endif
								}
								pFrag->RxScoElement[id].totalcount += (PACKET_MAX_BYTES[tmptype] / 2);
								if (pFrag->RxScoElement[id].totalcount >= BT_SCO_DEFRAG_LIMIT)
								{
									ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + sizeof(UINT32));
									ptmphcbb->length = pFrag->RxScoElement[id].totalcount;
									pFrag->RxScoElement[id].RxCachPid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachPid);
									KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
									KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
									pFrag->RxScoElement[id].currentpos = 0;
									pFrag->RxScoElement[id].currentlen = 0;
									pFrag->RxScoElement[id].totalcount = 0;
									needsubmitscoflag = 1;
                                    if(pScoConDevice->txRxBalance)
                                    {
                                        pScoConDevice->txRxBalance--;
                                    }
								}
							}
						}
					}

					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_AFH_CH_BAD)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					chanNum = *(PUINT32)(pBuffer + tmpstartaddr + sizeof(HCBB_PAYLOAD_HEADER_T));
					BtProcessRxAFHChBadFrame(devExt, chanNum);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_DATA_NULL)
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL1, "receive RX_FRAME_TYPE_DATA_NULL frame, just discard in HCI and not transmitted to L2CAP\n");
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->l2cap_flow_control_flag = tmpflow;
						if ((pConnectDevice->l2cap_flow_control_flag == 1) && (pConnectDevice->timer_l2cap_flow_valid == 0))
						{
							Hci_StartL2capFlowTimer(devExt,pConnectDevice);
						}
						else if ((pConnectDevice->l2cap_flow_control_flag == 0) && (pConnectDevice->timer_l2cap_flow_valid == 1))
						{
							Hci_StopL2capFlowTimer(devExt,pConnectDevice);
						}
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));

					needsubmitscoflag = 1;
				}
                /* LMP packet */
				else
				{
					PHCBB_PAYLOAD_HEADER_T pPacketHeader;
					PUINT8 pBuf;

					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "LMP PDU recept\n");
					CachIndex = BtGetFreeLMPCach(devExt);
					if(CachIndex < MAX_LMP_CACH_COUNT)
					{
						devExt->RxLMPPDUCach[CachIndex].Flag_InUse = TRUE;
						pLMPbuf = devExt->RxLMPPDUCach[CachIndex].buffer;

						*(PUINT32)pLMPbuf = pRxBufHead->Len;
						pLMPbuf += sizeof(UINT32);
						pBuf = pHeadDesc + sizeof(UINT32);
						pPacketHeader = (PHCBB_PAYLOAD_HEADER_T)pBuf;
						RtlCopyMemory(pLMPbuf, pBuf, pRxBufHead->Len);
						
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						
						Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_LMP_PDU_PROCESS), BT_TASK_PRI_NORMAL, &CachIndex, 1);
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

					}
					else
					{
						BT_DBGEXT(ZONE_SDRCV | LEVEL0, "No lmp cach, error\n");
					}
					
					
					
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Tail error!\n");
				
#ifdef DBG
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRx_New()---print buffer data in this case:\n");
				BtPrintBuffer(pBuffer, DestLen);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "packet sequence No----%d\n", pRxBufTail->SeqNo);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Current sequence Num: %x\n", devExt->RxSeqNo);
#endif
				KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);		
				break;
			}
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Head error!\n");
			BtPrintBuffer(pBuffer, DestLen);
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			break;
		}
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	}


	if (needsubmitscoflag == 1)
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

	
}





///////////////////////////////////////////////////////////////////////////////
//BtTaskProcessRx
//	This function is copied from BtProcessRxInt, the main difference is not to trigger
//	intsendingtimer in this function.
//	this funciton is called by task thread
//Changelog:
//	Jakio20070823 add here
///////////////////////////////////////////////////////////////////////////////
VOID BtTaskProcessRx(PBT_DEVICE_EXT devExt, PUINT8 buffer, UINT32 BufLength)
{
	KIRQL oldIrql;
	PBT_RX_BUF_HEAD pRxBufHead;
	PBT_RX_BUF_TAIL pRxBufTail;
	PUINT8 pHeadDesc, pTailDesc;
	PUINT8 pBuffer;
	UINT32 tmpstartaddr, tmptailaddr;
	UINT32 chanNum;
	BT_RX_FRAME_TYPE tmprxtype;
	UINT8 tmptype;
	UINT32 consumeLength;
	UINT32 DestLen;
	LARGE_INTEGER timevalue;
	PUINT8 dest;
	UINT8 am_addr;
	UINT8 CachIndex;
	PUINT8	pLMPbuf;
	PHCBB_PAYLOAD_HEADER_T ptmphcbb;
	UINT8 master_slave_flag;
	UINT8 slave_index;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 needsubmitscoflag = 0;
	UINT32 tmpretrycount;
	UINT8 tmpflow;
	PBT_FRAG_T pFrag;
	UINT8 id;

	/*
	ASSERT(RecvAreaIndex < MAX_USB_BUKIN_AREA);
	sc_spin_lock_bh(&devExt->ReadQueueLock);
	pBuffer = devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvAreaIndex]->buffer;
	if (pBuffer == NULL)
	{
		sc_spin_unlock_bh(&devExt->ReadQueueLock);
		return ;
	}
	DestLen = realDataLength =devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvAreaIndex]->length;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "destlen:%d\n",DestLen);
	consumeLength = 0;
	pHeadDesc = pBuffer;
	pTailDesc = pBuffer;
	pRxBufHead = NULL;
	pRxBufTail = NULL;
	tmpstartaddr = tmptailaddr = 0;
	sc_spin_unlock_bh(&devExt->ReadQueueLock);
	*/

	pBuffer = buffer;
	DestLen = BufLength;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_Process----data length:%lu\n", DestLen);
	consumeLength = 0;
	pHeadDesc = pBuffer;
	pTailDesc = pBuffer;
	pRxBufHead = NULL;
	pRxBufTail = NULL;
	tmpstartaddr = tmptailaddr = 0;
	
	while ((consumeLength < DestLen) && (pHeadDesc))
	{
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
		pRxBufHead = (PBT_RX_BUF_HEAD)pHeadDesc;
		if ((pRxBufHead->Len == pRxBufHead->Len1) && (pRxBufHead->Len1 == 0))
		{
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "error exit: %d, %d\n", pRxBufHead->Len, pRxBufHead->Len1);
			return ;
		}
		if ((pRxBufHead->Len == pRxBufHead->Len1) )//&& (pRxBufHead->Len1 == pRxBufHead->Len2))
		{
			tmpstartaddr = sizeof(UINT32);
			tmptailaddr = sizeof(UINT32) + ALIGNLONGDATALEN(pRxBufHead->Len);
			pTailDesc = pHeadDesc + tmptailaddr;
			pRxBufTail = (PBT_RX_BUF_TAIL)pTailDesc;


			if((pRxBufHead->Res == MAGIC_NUM_HEAD) && (pRxBufTail->Res == MAGIC_NUM_TAIL))
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre-processing function---this frame has been processed, get next frame\n");\
				pHeadDesc += (tmptailaddr + sizeof(UINT32));
				consumeLength += (tmptailaddr + sizeof(UINT32));
				KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				continue;
			}
			

			/*
			if (devExt->IsFirstRxFrame)
			{
				devExt->IsFirstRxFrame = FALSE;
				devExt->RxSeqNo = (UINT8)pRxBufTail->SeqNo;
			}
			else
			{
				devExt->RxSeqNo++;
			}
			*/

			if(1)
			{
				tmprxtype = BtGetRxFrameTypeAndRetryCount(devExt, tmpstartaddr, &tmpretrycount, &am_addr, &tmptype, &tmpflow, &master_slave_flag, &slave_index, pHeadDesc);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "the type = %x\n", tmprxtype);
				if (tmprxtype == RX_FRAME_TYPE_DATA)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->l2cap_flow_control_flag = tmpflow;
						if ((pConnectDevice->l2cap_flow_control_flag == 1) && (pConnectDevice->timer_l2cap_flow_valid == 0))
						{
							Hci_StartL2capFlowTimer(devExt,pConnectDevice);
						}
						else if ((pConnectDevice->l2cap_flow_control_flag == 0) && (pConnectDevice->timer_l2cap_flow_valid == 1))
						{
							Hci_StopL2capFlowTimer(devExt,pConnectDevice);
						}
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					if (BtIsRxCachFull(devExt))
					{
						needsubmitscoflag = 1;
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Rx int but rx cach is full!\n");
						pHeadDesc += (tmptailaddr + sizeof(UINT32));
						consumeLength += (tmptailaddr + sizeof(UINT32));
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						continue;
					}
					if(BtIsRxCachToBeFull(devExt))
					{
					#ifdef BT_SCHEDULER_SUPPORT
						if (pConnectDevice != NULL)
						{
							if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
							{
								Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
							}
						}
					#endif
						needsubmitscoflag = 1;
					}
					
					dest = devExt->RxCach[devExt->RxCachPid];
					BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
					devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);

					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					tmptype = (*(dest + sizeof(UINT32)) &0x78) >> 3;
					if ((tmptype == BT_SCO_PACKET_HV1) || (tmptype == BT_SCO_PACKET_HV2) || (tmptype == BT_SCO_PACKET_HV3))
					{
					}
					else
					{
						if ((tmptype == BT_ACL_PACKET_DM1) || (tmptype == BT_ACL_PACKET_AUX1) || (tmptype == BT_ACL_PACKET_DH1 && pConnectDevice != NULL && pConnectDevice->edr_mode == 0))
						{
						}
						else
						{
						}
					}

					needsubmitscoflag = 1;
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
				}
				else if (tmprxtype == RX_FRAME_TYPE_NULL)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					BtProcessRxNULLFrame(devExt, tmpstartaddr, pHeadDesc);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_POLL)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					BtProcessRxPoolFrame(devExt, pHeadDesc);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_CRC_ERROR)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					BtProcessRxCRCErrorFrame(devExt, master_slave_flag, am_addr, slave_index);
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_DV)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					if (BtIsRxCachFull(devExt))
					{
						needsubmitscoflag = 1;

						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Rx int but rx cach is full!\n");
						pHeadDesc += (tmptailaddr + sizeof(UINT32));
						consumeLength += (tmptailaddr + sizeof(UINT32));
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						continue;
					}
					if(BtIsRxCachToBeFull(devExt))
					{
					#ifdef BT_SCHEDULER_SUPPORT
						if (pConnectDevice != NULL)
						{
							if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
							{
								Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
							}
						}
					#endif	
					}
					
					dest = devExt->RxCach[devExt->RxCachPid];
					BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
					devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
					if ((*(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2) &0x3) == (UINT8)BT_LCH_TYPE_LMP)
					{
						ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
						ptmphcbb->type = BT_SCO_PACKET_HV1;


						CachIndex = BtGetFreeLMPCach(devExt);
						if(CachIndex < MAX_LMP_CACH_COUNT)
						{
							devExt->RxLMPPDUCach[CachIndex].Flag_InUse = TRUE;
							pLMPbuf = devExt->RxLMPPDUCach[CachIndex].buffer;

							*(PUINT32)pLMPbuf = *(PUINT16)dest - PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2;
							*(PUINT32)(pLMPbuf + sizeof(UINT32)) = *(PUINT32)(dest + sizeof(UINT32));
							RtlCopyMemory(pLMPbuf + sizeof(UINT32) + sizeof(INT32), dest + sizeof(UINT32) + sizeof(UINT32) + PACKET_MAX_BYTES[BT_SCO_PACKET_HV1] / 2, 18);

							KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
							BtProcessRxLMPPDU(devExt, CachIndex);
						}
						else
						{
							BT_DBGEXT(ZONE_SDRCV | LEVEL1, "No lmp cach, error\n");
						}
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					}
					else
					{
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					}

					needsubmitscoflag = 1;
				}
				else if (tmprxtype == RX_FRAME_TYPE_SCO)
				{
					pFrag = (PBT_FRAG_T)devExt->pFrag;
					needsubmitscoflag = 0;
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
						if (pConnectDevice->pScoConnectDevice != NULL)
						{
							id = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->index;
							ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
							BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO index = %d\n", id);

							if(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->NeedSendScoNull)
							{
								((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->NeedSendScoNull = FALSE;
							}

							if (Frag_IsScoRxCachFull(pFrag, id))
							{
								BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx sco cach is full!\n");
								needsubmitscoflag = 1;
								pHeadDesc += (tmptailaddr + sizeof(UINT32));
								consumeLength += (tmptailaddr + sizeof(UINT32));
								KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
								continue;
								/*
								BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx sco cach is full!\n");
								sc_spin_unlock_bh(&devExt->ReadQueueLock);
								
								break;
								*/
							}
							
							dest = pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + pFrag->RxScoElement[id].currentpos;
							if (pFrag->RxScoElement[id].currentpos == 0)
							{
								BtTransferRxFrameToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
								pFrag->RxScoElement[id].currentpos += (pRxBufHead->Len + sizeof(UINT32));
								pFrag->RxScoElement[id].currentlen += (1+sizeof(HCI_SCO_HEADER_T) + (pRxBufHead->Len - 4) *2);
							}
							else
							{
								BtTransferRxFrameBodyToRxCach(devExt, tmpstartaddr, tmptailaddr, pRxBufHead->Len, pHeadDesc, dest);
								pFrag->RxScoElement[id].currentpos += (pRxBufHead->Len - 4);
								pFrag->RxScoElement[id].currentlen += ((pRxBufHead->Len - 4) *2);
							}
							pFrag->RxScoElement[id].totalcount += (pRxBufHead->Len - 4);
							if (pFrag->RxScoElement[id].totalcount >= BT_SCO_DEFRAG_LIMIT)
							{
								BT_DBGEXT(ZONE_SDRCV | LEVEL1, "BtProcessRx_New()--sco data full, inform IVT to get data\n");
								ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + sizeof(UINT32));
								ptmphcbb->length = pFrag->RxScoElement[id].totalcount;
								pFrag->RxScoElement[id].RxCachPid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachPid);
								KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
								KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
								pFrag->RxScoElement[id].currentpos = 0;
								pFrag->RxScoElement[id].currentlen = 0;
								pFrag->RxScoElement[id].totalcount = 0;
								needsubmitscoflag = 1;
							}
						}
					}
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
					if (needsubmitscoflag)
					{
						
					}
				}
				else if (tmprxtype == RX_FRAME_TYPE_SCO_NULL)
				{
					pFrag = (PBT_FRAG_T)devExt->pFrag;
					needsubmitscoflag = 0;
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						ASSERT((tmptype >= (UINT8)BT_SCO_PACKET_HV1) && (tmptype <= (UINT8)BT_SCO_PACKET_HV3));
						pConnectDevice->rx_sco_con_null_count += (PACKET_MAX_BYTES[tmptype] / 2);
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "rx_sco_con_null_count = %lu\n", pConnectDevice->rx_sco_con_null_count);
						if (pConnectDevice->rx_sco_con_null_count >= BT_RX_SCO_CON_NULL_COUNT_LIMIT)
						{
							if (pConnectDevice->rx_sco_con_null_count <= (BT_RX_SCO_CON_NULL_COUNT_LIMIT + 120))
							{
								BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Start Timer for SCO NULL frame!\n");
								Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, 1);
							}
						}
						else
						{
							if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
							{
								Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
							}
							if (pConnectDevice->pScoConnectDevice != NULL)
							{
								id = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->index;
								ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
								BT_DBGEXT(ZONE_SDRCV | LEVEL3, "SCO index = %d\n", id);


								if(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->NeedSendScoNull)
								{
									((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->NeedSendScoNull = FALSE;
								}

								if (Frag_IsScoRxCachFull(pFrag, id))
								{
									needsubmitscoflag = 1;
									BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Rx int but rx sco cach is full!\n");
									needsubmitscoflag = 1;
									pHeadDesc += (tmptailaddr + sizeof(UINT32));
									consumeLength += (tmptailaddr + sizeof(UINT32));
									KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
									continue;
									/*
									sc_spin_unlock_bh(&devExt->ReadQueueLock);
									break;
									*/
								}
								
								dest = pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + pFrag->RxScoElement[id].currentpos;
								if (pFrag->RxScoElement[id].currentpos == 0)
								{
									*(PUINT16)dest = (UINT16)(PACKET_MAX_BYTES[tmptype] / 2+sizeof(HCBB_PAYLOAD_HEADER_T));
									*(PUINT16)(dest + sizeof(UINT16)) = (UINT16)RX_FRAME_TYPE_DATA;
									ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(dest + sizeof(UINT32));
									ptmphcbb->am_addr = am_addr;
									ptmphcbb->tx_retry_count = tmpretrycount;
									ptmphcbb->type = tmptype;
									ptmphcbb->length = PACKET_MAX_BYTES[tmptype] / 2;
									if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode == BT_AIRMODE_A_LAW)
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x1);
									}
									else if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode == BT_AIRMODE_MU_LAW)
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x2);
									}
									else
									{
										RtlFillMemory(dest + sizeof(UINT32) + sizeof(HCBB_PAYLOAD_HEADER_T), PACKET_MAX_BYTES[tmptype] / 2, 0x55);
									}
									pFrag->RxScoElement[id].currentpos += (PACKET_MAX_BYTES[tmptype] / 2+sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(UINT32));
									pFrag->RxScoElement[id].currentlen += (1+sizeof(HCI_SCO_HEADER_T) + PACKET_MAX_BYTES[tmptype]);
								}
								else
								{
									if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode == BT_AIRMODE_A_LAW)
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x1);
									}
									else if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode == BT_AIRMODE_MU_LAW)
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x2);
									}
									else
									{
										RtlFillMemory(dest, PACKET_MAX_BYTES[tmptype] / 2, 0x55);
									}
									pFrag->RxScoElement[id].currentpos += (PACKET_MAX_BYTES[tmptype] / 2);
									pFrag->RxScoElement[id].currentlen += PACKET_MAX_BYTES[tmptype];
								}
								pFrag->RxScoElement[id].totalcount += (PACKET_MAX_BYTES[tmptype] / 2);
								if (pFrag->RxScoElement[id].totalcount >= BT_SCO_DEFRAG_LIMIT)
								{
									ptmphcbb = (PHCBB_PAYLOAD_HEADER_T)(pFrag->RxScoElement[id].RxCachBlock[pFrag->RxScoElement[id].RxCachPid].RxCach + sizeof(UINT32));
									ptmphcbb->length = pFrag->RxScoElement[id].totalcount;
									pFrag->RxScoElement[id].RxCachPid = Frag_GetNextScoRxCachId(pFrag->RxScoElement[id].RxCachPid);
									KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
									KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
									pFrag->RxScoElement[id].currentpos = 0;
									pFrag->RxScoElement[id].currentlen = 0;
									pFrag->RxScoElement[id].totalcount = 0;
									needsubmitscoflag = 1;
								}
							}
						}
					}
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_AFH_CH_BAD)
				{
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
					chanNum = *(PUINT32)(pBuffer + tmpstartaddr + sizeof(HCBB_PAYLOAD_HEADER_T));
					BtProcessRxAFHChBadFrame(devExt, chanNum);
					
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
				else if (tmprxtype == RX_FRAME_TYPE_DATA_NULL)
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "receive RX_FRAME_TYPE_DATA_NULL frame, just discard in HCI and not transmitted to L2CAP\n");
					KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				#ifdef BT_USE_NEW_ARCHITECTURE
					if (master_slave_flag == BT_HCBB_MASTER_FLAG)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
					}
				#else
					if (((PBT_HCI_T)devExt->pHci)->role == BT_ROLE_MASTER)
					{
						pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
					else
					{
						pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						if (pConnectDevice == NULL)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
					}
				#endif
					KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
					if (pConnectDevice != NULL)
					{
						pConnectDevice->l2cap_flow_control_flag = tmpflow;
						if ((pConnectDevice->l2cap_flow_control_flag == 1) && (pConnectDevice->timer_l2cap_flow_valid == 0))
						{
							Hci_StartL2capFlowTimer(devExt,pConnectDevice);
						}
						else if ((pConnectDevice->l2cap_flow_control_flag == 0) && (pConnectDevice->timer_l2cap_flow_valid == 1))
						{
							Hci_StopL2capFlowTimer(devExt,pConnectDevice);
						}
						pConnectDevice->rx_sco_con_null_count = 0;
						if ((pConnectDevice->timer_valid == 1) && (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
						{
							Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
						}
					}
					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));

					needsubmitscoflag = 1;
				}
				else
				{
					PHCBB_PAYLOAD_HEADER_T pPacketHeader;
					PUINT8 pBuf;


					CachIndex = BtGetFreeLMPCach(devExt);
					if(CachIndex < MAX_LMP_CACH_COUNT)
					{
						devExt->RxLMPPDUCach[CachIndex].Flag_InUse = TRUE;
						pLMPbuf = devExt->RxLMPPDUCach[CachIndex].buffer;

						*(PUINT32)pLMPbuf = pRxBufHead->Len;
						pLMPbuf += sizeof(UINT32);
						pBuf = pHeadDesc + sizeof(UINT32);
						pPacketHeader = (PHCBB_PAYLOAD_HEADER_T)pBuf;
						RtlCopyMemory(pLMPbuf, pBuf, pRxBufHead->Len);
						
						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						
						BtProcessRxLMPPDU(devExt, CachIndex);
						
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

					}
					else
					{
						BT_DBGEXT(ZONE_SDRCV | LEVEL0, "No lmp cach, error\n");
					}

					pHeadDesc += (tmptailaddr + sizeof(UINT32));
					consumeLength += (tmptailaddr + sizeof(UINT32));
				}
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Tail error!\n");
			#ifdef DBG
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtTaskProcessRx()---print buffer data in this case:\n");
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "packet sequence No----%d\n", pRxBufTail->SeqNo);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Current sequence Num: %x\n", devExt->RxSeqNo);
			#endif
				KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				break;
			}
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Head error!\n");
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			break;
		}
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	}


	if (needsubmitscoflag == 1)
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

	
}





/******************************************************************************
*Description:
*	Check rx cach status and make decisions which packets type should be pre-processed
*Return:
*	the packets types
******************************************************************************/
UINT32 BtSetPacketFilter(PBT_DEVICE_EXT devExt)
{
	UINT32	Results = 0;
	PBT_FRAG_T	pFrag;
	PBT_TASK_T	pTask;
	KIRQL		oldIrql;
	UINT32	RxPendingNum;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return 0xffffffff;
	pTask = (PBT_TASK_T)devExt->pTask;
	if(pTask == NULL)
		return 0xffffffff;
	
	if(TRUE == BtIsRxCachToBeFull(devExt))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Pre_Processed--Rx cach to be full, should pre-process acl data\n");
		Results |= BT_PRE_PROCESS_ACL_DATA;
	}
	KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
	RxPendingNum = devExt->RxPacketNum;
	KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_Processed--Rx pending packets number:%lu\n",RxPendingNum);
	if(RxPendingNum >= 1)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_Processed--add bad channel packet\n");
		Results |= BT_PRE_PROCESS_BAD_CHANNEL;
	}
	if(RxPendingNum >= MAXTASKN/4)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_Processed--add sco,acl packet\n");
		Results |= BT_PRE_PROCESS_SCO;
		Results |= BT_PRE_PROCESS_SCO_NULL;
		Results |= BT_PRE_PROCESS_ACL_DATA;
	}

	return Results;
	
}


VOID BtPreProcessPacket(PBT_DEVICE_EXT devExt, UINT32 PacketType, PUINT8 buffer, UINT32 length)
{
	PBT_RX_BUF_HEAD pRxBufHead;
	PBT_RX_BUF_TAIL pRxBufTail;
	PUINT8 pHeadDesc, pTailDesc;
	UINT32 tmpstartaddr, tmptailaddr;
	UINT32 chanNum;
	UINT32 consumeLength;
	KIRQL   oldIrql;
	UINT8	NeedPutIntoTask = 0;
	BT_RX_FRAME_TYPE tmprxtype;
	UINT8 tmptype;
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 master_slave_flag;
	UINT8 slave_index;
	UINT32 tmpretrycount;
	UINT8 am_addr;
	UINT8 tmpflow;
	PCONNECT_DEVICE_T pConnectDevice;
	PBT_HCI_T	pHci = NULL;
	
	consumeLength = 0;
	pHeadDesc = buffer;
	pTailDesc = buffer;
	pRxBufHead = NULL;
	pRxBufTail = NULL;
	tmpstartaddr = tmptailaddr = 0;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_Process----data length:%lu\n", length);

	while ((consumeLength < length) && (pHeadDesc))
	{
		KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
		pRxBufHead = (PBT_RX_BUF_HEAD)pHeadDesc;
		if ((pRxBufHead->Len == pRxBufHead->Len1) && (pRxBufHead->Len1 == 0))
		{
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Pre_process---error exit: %d, %d\n", pRxBufHead->Len, pRxBufHead->Len1);
			return ;
		}

		if ((pRxBufHead->Len == pRxBufHead->Len1) )
		{
			tmpstartaddr = sizeof(UINT32);
			tmptailaddr = sizeof(UINT32) + ALIGNLONGDATALEN(pRxBufHead->Len);
			pTailDesc = pHeadDesc + tmptailaddr;
			pRxBufTail = (PBT_RX_BUF_TAIL)pTailDesc;

			pRxBufHead->Res = 0;
			pRxBufTail->Res = 0;


			if (devExt->IsFirstRxFrame)
			{
				devExt->IsFirstRxFrame = FALSE;
				devExt->RxSeqNo = (UINT8)pRxBufTail->SeqNo;
			}
			else
			{
				devExt->RxSeqNo++;
			}
			if ((UINT8)pRxBufTail->SeqNo == devExt->RxSeqNo)
			{
				tmprxtype = BtGetRxFrameTypeAndRetryCount(devExt, tmpstartaddr, &tmpretrycount, &am_addr, &tmptype, &tmpflow, &master_slave_flag, &slave_index, pHeadDesc);
		                BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Packet Type:%d\n", tmprxtype);

				devExt->TxRetryCount = tmpretrycount;

				switch(tmprxtype)
				{
					case RX_FRAME_TYPE_DATA:

						KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
						if (master_slave_flag == BT_HCBB_MASTER_FLAG)
						{
							pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
						}
						else
						{
							pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
						}

						if(pConnectDevice != NULL)
						{
							if (((pConnectDevice->class_of_device[1] & 0x1f) == 0x5) && (pConnectDevice->connection_state == 0)) 
							{
								if(pConnectDevice->MsKb_PacketCount > 12)
								{
									BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_process----drop mouse or keyboard data, device:%2x\n",pConnectDevice->bd_addr[0]);
									pRxBufHead->Res = MAGIC_NUM_HEAD;
									pRxBufTail->Res = MAGIC_NUM_TAIL;	
								}
								else
								{
									BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_process--Allow %lu packets for device:0x%x\n", pConnectDevice->MsKb_PacketCount,
											pConnectDevice->bd_addr[0]);
									pConnectDevice->MsKb_PacketCount++;
									NeedPutIntoTask = 1;
								}
								
							}
							else
							{
								if(PacketType&BT_PRE_PROCESS_ACL_DATA)
								{
									#ifdef BT_SCHEDULER_SUPPORT
										if (pConnectDevice != NULL)
										{
											if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
											{
												BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_process--send null stop frame\n");
												Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
											}
										}
									#endif
								}
								
								NeedPutIntoTask = 1;
							}
						}
						else
						{

							BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Pre_process--Can't find device\n");
							pHci = (PBT_HCI_T)devExt->pHci;
							if((pHci != NULL) && (pHci->role_switching_flag == 1))
							{
									
								{
									BT_DBGEXT(ZONE_SDRCV | LEVEL1, "Pre_process--role switching flag is set, give a chance for this packet\n");
									NeedPutIntoTask = TRUE;
								}
									
							}
						
							
						}
						KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
						/*
						if(PacketType&BT_PRE_PROCESS_ACL_DATA)
						{
							sc_spin_unlock_bh(&devExt->ReadQueueLock);
							if (master_slave_flag == BT_HCBB_MASTER_FLAG)
							{
								pConnectDevice = Hci_Find_Connect_Device_By_AMAddr((PBT_HCI_T)devExt->pHci, am_addr);
							}
							else
							{
								pConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index((PBT_HCI_T)devExt->pHci, am_addr, slave_index);
							}
						#ifdef BT_SCHEDULER_SUPPORT
							if (pConnectDevice != NULL)
							{
								if (pConnectDevice->l2cap_rx_flow_control_flag == 0)
								{
									Sched_SendAclNullStopFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
								}
							}
						#endif
							sc_spin_lock_bh(&devExt->ReadQueueLock);
						}
						NeedPutIntoTask = 1;
						*/
						break;
					case RX_FRAME_TYPE_SCO:
					case RX_FRAME_TYPE_SCO_NULL:
						if((PacketType&BT_PRE_PROCESS_SCO) || (PacketType&BT_PRE_PROCESS_SCO_NULL))
						{
							pRxBufHead->Res = MAGIC_NUM_HEAD;
							pRxBufTail->Res = MAGIC_NUM_TAIL;
						}
						else
						{
							NeedPutIntoTask = 1;
						}
						break;
					case RX_FRAME_TYPE_AFH_CH_BAD:
						if(PacketType&BT_PRE_PROCESS_BAD_CHANNEL)
						{
							KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
							chanNum = *(PUINT32)(pHeadDesc + tmpstartaddr + sizeof(HCBB_PAYLOAD_HEADER_T));
							BtProcessRxAFHChBadFrame(devExt, chanNum);
							KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

							pRxBufHead->Res = MAGIC_NUM_HEAD;
							pRxBufTail->Res = MAGIC_NUM_TAIL;
						}
						else
						{
							NeedPutIntoTask = 1;
						}
						break;
					default:
						NeedPutIntoTask = 1;
						
				}

				if(NeedPutIntoTask)
				{
					status = Task_CreateTaskForRxData((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_INT_PROCESS), 
							BT_TASK_PRI_NORMAL, (PUINT8)pHeadDesc, tmptailaddr + sizeof(UINT32));
					if(status == STATUS_SUCCESS)
					{
						KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
						devExt->RxPacketNum++;
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "RxPacketNum is %d\n", devExt->RxPacketNum);
						KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);	
					}

					NeedPutIntoTask = FALSE;
				}
				
				pHeadDesc += (tmptailaddr + sizeof(UINT32));
				consumeLength += (tmptailaddr + sizeof(UINT32));
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Pre_process---Tail error\n");
				KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
				break;
			}
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL0, "Pre_process---Head error\n");
			KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
			break;
		}

		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	}

	/*
	if(NeedPutIntoTask)
	{
		status = Task_CreateTaskForRxData((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_INT_PROCESS), 
				BT_TASK_PRI_NORMAL, buffer, length);
		if(status == STATUS_SUCCESS)
		{
			sc_spin_lock_bh(&devExt->RxStateLock);
			devExt->RxPacketNum++;
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "RxPacketNum is %d\n", devExt->RxPacketNum);
			sc_spin_unlock_bh(&devExt->RxStateLock);	
		}
		
	}
	*/
}




VOID BtTaskProcessInquiryResult(PBT_DEVICE_EXT devExt, PUINT8 pbuffer, UINT32 length)
{
	PBT_HCI_T pHci;
	UINT8 inquiryresultevent;
	PFHS_PACKET_T pFhsPacket;
	UINT8 bd_addr[BT_BD_ADDR_LENGTH];

	if(pbuffer == NULL)
	{
		return;
	}
	if(length < sizeof(FHS_PACKET_T))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "BtTaskProcessInquiryResult, length error:%lu\n",length);
		return;
	}
	pFhsPacket = (PFHS_PACKET_T)pbuffer;
	bd_addr[0] = (UINT8)pFhsPacket->lap;
	bd_addr[1] = (UINT8)(pFhsPacket->lap >> 8);
	bd_addr[2] = (UINT8)(pFhsPacket->lap >> 16);
	bd_addr[3] = pFhsPacket->uap;
	bd_addr[4] = (UINT8)pFhsPacket->nap;
	bd_addr[5] = (UINT8)(pFhsPacket->nap >> 8);
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Scan BD ADDR : %02x %02x %02x %02x %02x %02x\n", bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
	pHci = (PBT_HCI_T)devExt->pHci;
	if (Hci_Find_Inquiry_Result_By_BDAddr(pHci, bd_addr) == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Clock offset: %x\n", (UINT16)pFhsPacket->clk27_2);
		Hci_Add_Inquiry_Result(pHci, bd_addr, (UINT8)pFhsPacket->sr, (UINT8)pFhsPacket->sp, 
					(UINT8)pFhsPacket->page_scan_mode, pFhsPacket->class_of_device, (UINT16)pFhsPacket->clk27_2);
		inquiryresultevent = pHci->inquiry_mode == BT_INQUIRYMODE_RESULT_WITH_RSSI ? BT_HCI_EVENT_INQUIRY_RESULT_WITH_RSSI : BT_HCI_EVENT_INQUIRY_RESULT;
		Task_Normal_Send_Event(devExt, inquiryresultevent, 1);
		
		if (pHci->current_num_responses != 0)
		{
			if (++pHci->current_inquiry_result_num >= pHci->current_num_responses)
			{
				pHci->current_opcode = 0;
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
				BtDelay(5);
				pHci->current_inquiry_result_num = 0;
				pHci->is_in_inquiry_flag = 0;
				Hci_StopProtectInquiryTimer(pHci);
				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_INQUIRY_COMPLETE, 1);
			}
		}
	}
}







///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessInquiryCompleteInt
//
//    This function process the inquiry complete interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessInquiryCompleteInt(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->is_in_inquiry_flag = 0;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Hci_StopProtectInquiryTimer(pHci);
	
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_INQUIRY_COMPLETE, 1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessHardErrorInt
//
//    This function process the hardware error interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessHardErrorInt(PBT_DEVICE_EXT devExt){}
///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessFlushOccuredInt
//
//    This function process the flush occured error interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessFlushOccuredInt(PBT_DEVICE_EXT devExt){}
///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRoleChangeInt
//
//    This function process the role change interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessRoleChangeInt(PBT_DEVICE_EXT devExt)
{
	UINT8				bd_addr[BT_BD_ADDR_LENGTH];
	PBT_HCI_T			pHci;
	PCONNECT_DEVICE_T	pTempConnectDevice;
	KIRQL				oldIrql;
	LARGE_INTEGER		timevalue;
	FHS_PACKET_T		fhs_packet;
	UINT8				am_addr, move_frag = 0, slaveindex = 0;
	UINT16				edr_mode;/* jakio20071025: add here for EDR
*/
	UINT8				Datastr[8];
	UINT8				ListIndex;

	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change\n");

	#ifdef BT_SCHEDULER_SUPPORT
	Sched_DeleteAllPoll(devExt);
	#endif

	pHci = (PBT_HCI_T)devExt->pHci;
	UsbReadFrom3DspRegs(devExt, BT_REG_OP_BD_ADDR_DSP, 2, bd_addr);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, bd_addr);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change 2\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, bd_addr);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice != NULL)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "start queue\n");
			Frag_GetQueueHead(devExt, pTempConnectDevice, &ListIndex);
			Frag_StartQueue(devExt, ListIndex);
		
			if (pTempConnectDevice->role_switching_flag == 0)
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "More role change interrupt occur, just discard it\n");
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				return;
			}
			pHci->role_switch_fail_count = 0;

			Hci_StopProtectHciCommandTimer(pHci);
			pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
			if (pTempConnectDevice->role_switching_role == BT_ROLE_MASTER)
			{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change 4\n");
				if (pTempConnectDevice->am_addr_for_fhs == 0)
				{
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change 5\n");
					pTempConnectDevice->status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					if (pTempConnectDevice->state == BT_DEVICE_STATE_WAIT_ROLE_SWITCH)
					{
							BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change 6\n");
						pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
						Hci_StopTimer(pTempConnectDevice);
						Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
						pHci->pcurrent_connect_device = pTempConnectDevice;
						pHci->current_connection_handle = pTempConnectDevice->connection_handle;
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					tasklet_schedule(&devExt->taskletRcv);
				}
				else
				{
					am_addr = pTempConnectDevice->am_addr_for_fhs;
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "am_addr = %d\n",am_addr);
					pTempConnectDevice->raw_role = pTempConnectDevice->role_switching_role;
					pTempConnectDevice->current_role = pTempConnectDevice->role_switching_role;
					slaveindex = pTempConnectDevice->slave_index;
					pTempConnectDevice->slave_index = 0;
					move_frag = 1;
					pTempConnectDevice->slave_index = 0;
					pTempConnectDevice->connection_state = 1;

					KeReleaseSpinLock(&pHci->HciLock, oldIrql);

					/*
					if (pTempConnectDevice->current_role == BT_ROLE_MASTER)
						reg_offset = pTempConnectDevice->am_addr;
					else
						reg_offset = pTempConnectDevice->slave_index * 8;
					
					pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
					pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_EDR_MODE, pBuf, 2, ~(1 << reg_offset) );
					pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_EDR_MODE, pBuf, 2, (pTempConnectDevice->edr_mode << reg_offset) );
					RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
					*/
					edr_mode = Hci_Read_Word_From_3DspReg(devExt, BT_REG_EDR_MODE, 0);
					edr_mode = edr_mode & (~(1 << (slaveindex * 8))) & (~(1 << am_addr)) | (pTempConnectDevice->edr_mode << am_addr);
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "master: edr_mode value:%d\n", edr_mode);
					Hci_Write_Word_To_3DspReg( devExt, BT_REG_EDR_MODE, 0, edr_mode);
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_PACKET_TYPE_EDR_MODE);
					
					
					pHci->role = BT_ROLE_MASTER;
					QueueRemoveEle(&pHci->device_slave_list, &pTempConnectDevice->Link);
					pHci->num_device_slave--;
					Hci_Write_One_Byte(devExt,BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);

					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
					QueuePutTail(&pHci->device_am_list, &pTempConnectDevice->Link);
					if (pTempConnectDevice->state == BT_DEVICE_STATE_SLAVE_CONNECTED)
					{
						pTempConnectDevice->state = BT_DEVICE_STATE_CONNECTED;
					}
					pHci->num_device_am++;
					pHci->pcurrent_connect_device = pTempConnectDevice;
					pHci->current_connection_handle = pTempConnectDevice->connection_handle;
					Hci_Write_Data_To_EightBytes(devExt, BT_REG_PAGED_BD_ADDR, pTempConnectDevice->bd_addr, 6);
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Clock offset: %x\n", pHci->pcurrent_connect_device->clock_offset);
					Hci_Write_AM_Addr(devExt, am_addr, am_addr);
					Hci_Write_Byte_In_FourByte(devExt, BT_REG_CURRENT_PROCESS_AM_ADDR, 0, pHci->pcurrent_connect_device->am_addr);
					RtlZeroMemory(Datastr, 8);
					RtlCopyMemory(Datastr, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
					RtlCopyMemory(Datastr + BT_BD_ADDR_LENGTH, (PUINT8) &pHci->pcurrent_connect_device->clock_offset, 2);
					UsbWriteTo3DspRegs(devExt, BT_REG_AM_BD_ADDR_MASTER, 2, Datastr);
					pHci->pcurrent_connect_device->am_addr = am_addr;
					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_ADD, pHci->pcurrent_connect_device->am_addr);
					
					Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					if (pTempConnectDevice->state == BT_DEVICE_STATE_WAIT_ROLE_SWITCH)
					{
							BT_DBGEXT(ZONE_SDRCV | LEVEL3, "role change 8\n");
						pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
						Hci_StopTimer(pTempConnectDevice);
						Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					else
					{
					#ifdef BT_AFH_SUPPORT // Set AFH for this connection
						BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Set Afh for this devic[%02x]\n", pTempConnectDevice->bd_addr[0]);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						RtlFillMemory(pHci->afh_channel_map, BT_MAX_CHANNEL_MAP_NUM, 0xff);
						pHci->afh_timer_count = 0; // Reset check afh timer count
						pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_SET_AFH_FOR_ONE_CONNECTION;
						pHci->pcurrent_connect_device = pTempConnectDevice;
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					#endif

					}
					tasklet_schedule(&devExt->taskletRcv);
				}
			}
			pTempConnectDevice->role_switching_flag = 0;
			pHci->role_switching_flag = 0;
		}
		else
		{
			if(pHci != NULL)
			{
				pHci->role_switching_flag = 0;
			}
			
		}

	}
	else
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "start queue2\n");
		Frag_GetQueueHead(devExt, pTempConnectDevice, &ListIndex);
		Frag_StartQueue(devExt, ListIndex);
	
		if (pTempConnectDevice->role_switching_flag == 0)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "More role change interrupt occur, just discard it\n");
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return;
		}
		pHci->role_switch_fail_count = 0;

		Hci_StopProtectHciCommandTimer(pHci);
		pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
		if (pTempConnectDevice->role_switching_role == BT_ROLE_SLAVE)
		{
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_Free_Am_address(pHci, pTempConnectDevice->am_addr);
		
			pTempConnectDevice->raw_role = pTempConnectDevice->role_switching_role;
			pTempConnectDevice->current_role = pTempConnectDevice->role_switching_role;
			pTempConnectDevice->slave_index = Hci_Read_Byte_From_3DspReg(devExt, BT_REG_ROLE_SWITCH_AMADDR, 0);
			slaveindex = pTempConnectDevice->slave_index;
			move_frag = 1;
			

			edr_mode = Hci_Read_Word_From_3DspReg(devExt, BT_REG_EDR_MODE, 0);
			edr_mode = edr_mode & (~(1 << pTempConnectDevice->am_addr)) & (~(1 << (slaveindex * 8))) | (pTempConnectDevice->edr_mode << (slaveindex * 8));
			Hci_Write_Word_To_3DspReg( devExt, BT_REG_EDR_MODE, 0, edr_mode);

			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_PACKET_TYPE_EDR_MODE);
			
			pHci->role = BT_ROLE_SLAVE;
			QueueRemoveEle(&pHci->device_am_list, &pTempConnectDevice->Link);
			
			pHci->num_device_am--;

			Hci_Clear_AM_Addr(devExt, pTempConnectDevice->am_addr);
			Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pTempConnectDevice->am_addr);

			QueuePutTail(&pHci->device_slave_list, &pTempConnectDevice->Link);
			if (pTempConnectDevice->state == BT_DEVICE_STATE_CONNECTED)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_SLAVE_CONNECTED;
			}
			pHci->num_device_slave++;
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
			UsbReadFrom3DspRegs(devExt, BT_REG_FHS_FOR_PAGE_SCAN, sizeof(FHS_PACKET_T) / 4, (PUINT8) &fhs_packet);
			bd_addr[0] = (UINT8)fhs_packet.lap;
			bd_addr[1] = (UINT8)(fhs_packet.lap >> 8);
			bd_addr[2] = (UINT8)(fhs_packet.lap >> 16);
			bd_addr[3] = fhs_packet.uap;
			bd_addr[4] = (UINT8)fhs_packet.nap;
			bd_addr[5] = (UINT8)(fhs_packet.nap >> 8);
			am_addr = (UINT8)fhs_packet.am_addr;
			pTempConnectDevice->am_addr = am_addr;
			RtlCopyMemory(pTempConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH);
			pTempConnectDevice->clock_offset = (UINT16)fhs_packet.clk27_2;
			Hci_Write_AM_Addr(devExt, 0, pTempConnectDevice->am_addr);
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Create slave connection am address = %d\n", pTempConnectDevice->am_addr);
			Hci_Write_Byte_In_FourByte(devExt, BT_REG_CURRENT_PROCESS_AM_ADDR, 0, 0);
			
			RtlZeroMemory(Datastr, 8);
			UsbReadFrom3DspRegs(devExt, BT_REG_AM_BD_ADDR0, 2, Datastr);
			RtlCopyMemory(Datastr, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
			UsbWriteTo3DspRegs(devExt, BT_REG_AM_BD_ADDR0, 2, Datastr);

			RtlZeroMemory(Datastr,8);
			UsbReadFrom3DspRegs(devExt, BT_REG_AM_CLK_OFFSET_SLAVE0, 1, Datastr);
			RtlCopyMemory(Datastr, (PUINT8) &pTempConnectDevice->clock_offset,2);			
			UsbWriteTo3DspRegs(devExt, BT_REG_AM_CLK_OFFSET_SLAVE0, 1, Datastr);

			Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_ADD, 0);
			
			Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
		pTempConnectDevice->role_switching_flag = 0;
		pHci->role_switching_flag = 0;
		tasklet_schedule(&devExt->taskletRcv);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	if(move_frag == 1)
	{
		if(pTempConnectDevice->current_role == BT_ROLE_MASTER)
		{
			Frag_MoveFrag(devExt, pTempConnectDevice, &((PBT_FRAG_T)devExt->pFrag)->Master_FragList, &((PBT_FRAG_T)devExt->pFrag)->Slave_FragList);
		}
		else
		{
			Frag_MoveFrag(devExt, pTempConnectDevice, &((PBT_FRAG_T)devExt->pFrag)->Slave_FragList, &((PBT_FRAG_T)devExt->pFrag)->Master_FragList);
		}
		
		timevalue.QuadPart = 0;
		KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRoleChangeFailInt
//
//    This function process the role change fail interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessRoleChangeFailInt(PBT_DEVICE_EXT devExt)
{
	UINT8 bd_addr[BT_BD_ADDR_LENGTH];
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	KIRQL oldIrql;
	LARGE_INTEGER timevalue;
	UINT8		ListIndex;
	pHci = (PBT_HCI_T)devExt->pHci;
	UsbReadFrom3DspRegs(devExt, BT_REG_OP_BD_ADDR_DSP, 2, bd_addr);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, bd_addr);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Can't find any device\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, bd_addr);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice != NULL)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "start queue\n");
			Frag_GetQueueHead(devExt, pTempConnectDevice, &ListIndex);
			Frag_StartQueue(devExt, ListIndex);
			
			if (pTempConnectDevice->role_switching_flag == 0)
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "More role change interrupt occur, just discard it\n");
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
				return;
			}

			if((++pHci->role_switch_fail_count<3)&&(pTempConnectDevice->state==BT_DEVICE_STATE_SLAVE_CONNECTED))
			{
				if (pTempConnectDevice->role_switching_role == BT_ROLE_MASTER)
				{
					if (pTempConnectDevice->am_addr_for_fhs != 0)
					{
						/* jakio20080226: release spinlock first, otherwise this will lead a dead lock
*/
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_Free_Am_address(pHci, pTempConnectDevice->am_addr_for_fhs);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						pTempConnectDevice->am_addr_for_fhs = 0;
					}
				}

				pHci->role_switching_flag = 0;
				pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
				pHci->pcurrent_connect_device = pTempConnectDevice;
				/* jakio20080118: should release spinlock before write registers
*/
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
				UsbWriteTo3DspRegs(devExt, BT_REG_OP_BD_ADDR_DSP, 2, bd_addr);
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "retry role change, just retry it!\n");
				return;
			}
			else
			{
				pHci->role_switch_fail_count = 0;
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "retry role change not match, just do it!\n");
			}
			

			Hci_StopProtectHciCommandTimer(pHci);
			pTempConnectDevice->status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
			pTempConnectDevice->role_switching_flag = 0;
			pHci->role_switching_flag = 0;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			if (pTempConnectDevice->role_switching_role == BT_ROLE_MASTER)
			{
				if (pTempConnectDevice->am_addr_for_fhs != 0)
				{
					Hci_Free_Am_address(pHci, pTempConnectDevice->am_addr_for_fhs);
					pTempConnectDevice->am_addr_for_fhs = 0;

				}
			}

			Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if (pTempConnectDevice->state == BT_DEVICE_STATE_WAIT_ROLE_SWITCH)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
				Hci_StopTimer(pTempConnectDevice);
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				pHci->pcurrent_connect_device = pTempConnectDevice;
				pHci->current_connection_handle = pTempConnectDevice->connection_handle;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
			tasklet_schedule(&devExt->taskletRcv);
		}
		else
		{
			if(pHci != NULL)
			{
				pHci->role_switching_flag = 0;
			}
		}

	}
	else
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "start queue2\n");
		Frag_GetQueueHead(devExt, pTempConnectDevice, &ListIndex);
		Frag_StartQueue(devExt, ListIndex);
		
		if (pTempConnectDevice->role_switching_flag == 0)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "More role change interrupt occur, just discard it\n");
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			return;
		}
		Hci_StopProtectHciCommandTimer(pHci);
		pTempConnectDevice->status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
		pTempConnectDevice->role_switching_flag = 0;
		pHci->role_switching_flag = 0;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		if (pTempConnectDevice->role_switching_role == BT_ROLE_MASTER)
		{
			if (pTempConnectDevice->am_addr_for_fhs != 0)
			{
				Hci_Free_Am_address(pHci, pTempConnectDevice->am_addr_for_fhs);
			}
		}
		Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice->state == BT_DEVICE_STATE_WAIT_ROLE_SWITCH)
		{
			pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
			Hci_StopTimer(pTempConnectDevice);
			Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
		tasklet_schedule(&devExt->taskletRcv);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessDspAckInt
//
//    This function process the DSP ack interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessDspAckInt(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	KIRQL oldIrql;

    ENTER_FUNC();
    
	pHci = (PBT_HCI_T)devExt->pHci;
	switch (BT_GET_OGF(pHci->current_opcode))
	{
		case (BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND): 
    		switch (BT_GET_OCF(pHci->current_opcode))
    		{
    			case (BT_HCI_COMMAND_RESET):
    			// case (BT_HCI_COMMAND_WRITE_SCAN_ENABLE):
    			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "DSP ACK for BT_HCI_COMMAND_RESET\n");
                // Leave the command hander to notify
    			//Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
    			Hci_StopProtectHciCommandTimer(pHci);
    			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    			break;
    		}
		break;
		case (BT_HCI_COMMAND_OGF_LINK_CONTROL): 
            switch (BT_GET_OCF(pHci->current_opcode))
    		{
			case (BT_HCI_COMMAND_INQUIRY):
                BT_DBGEXT(ZONE_SDRCV | LEVEL3, "DSP ACK for Inquiry command\n");
                Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
       			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
    			Hci_StopProtectHciCommandTimer(pHci);
    			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
                break;
            case (BT_HCI_COMMAND_PERIODIC_INQUIRY_MODE):
                BT_DBGEXT(ZONE_SDRCV | LEVEL3, "DSP ACK for Periodic Inquiry command\n");
       			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
    			Hci_StopProtectHciCommandTimer(pHci);
    			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
                break;
			case (BT_HCI_COMMAND_CREATE_CONNECTION):
    			// case (BT_HCI_COMMAND_ADD_SCO_CONNECTION):
    			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "DSP ACK for Connection command\n");

                /* Delay the status event to moment of send connection complete
                 * to serialize the BlueZ command processing.
                */
                //Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
    			Hci_StopProtectHciCommandTimer(pHci);
    			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    			break;
			case (BT_HCI_COMMAND_INQUIRY_CANCEL): 
    			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
    			pHci->is_in_inquiry_flag = 0;
    			Hci_StopProtectHciCommandTimer(pHci);
    			//Jakio20080724: stop protect inquiry timer
    			Hci_StopProtectInquiryTimer(pHci);
    			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    			break;
    		}
		break;
	}

    EXIT_FUNC();
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessModeChangeInt
//
//    This function process the mode change interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessModeChangeInt(PBT_DEVICE_EXT devExt, UINT8 mode)
{
	PBT_HCI_T pHci;
	UINT8 bd_addr[BT_BD_ADDR_LENGTH];
	PCONNECT_DEVICE_T pTempConnectDevice;
	KIRQL oldIrql;
	UINT32		tmpLong;
	pHci = (PBT_HCI_T)devExt->pHci;
	tmpLong = Hci_Read_DWord_From_3DspReg(devExt, BT_REG_OP_BD_ADDR_DSP);
	RtlCopyMemory(bd_addr, &tmpLong, sizeof(UINT32));
	tmpLong = Hci_Read_DWord_From_3DspReg(devExt, BT_REG_OP_BD_ADDR_DSP+4);
	RtlCopyMemory(bd_addr+4, &tmpLong, 2);
	
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, bd_addr);
	if (pTempConnectDevice == NULL)
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, bd_addr);
		if (pTempConnectDevice != NULL)
		{
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if (mode != pTempConnectDevice->mode_current_mode)
			{
				pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
				pTempConnectDevice->mode_current_mode = mode;
				Hci_StopProtectHciCommandTimer(pHci);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_HCI2HC_Send_Mode_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "mode is no difference, so does not send mode change event!\n");
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			}
		}
	}
	else
	{
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (mode != pTempConnectDevice->mode_current_mode)
		{
			pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
			pTempConnectDevice->mode_current_mode = mode;
			Hci_StopProtectHciCommandTimer(pHci);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_HCI2HC_Send_Mode_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "mode is no difference, so does not send mode change event!\n");
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessClockReadyInt
//
//    This function process the clock reday interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//	jakio20080227: add a para, and process it in dispatch level excepth writing registers
//
///////////////////////////////////////////////////////////////////////////////
VOID BtProcessClockReadyInt(PBT_DEVICE_EXT devExt, UINT32 AfhInstant)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	{
		if (pHci->clock_ready_flag == BT_CLOCK_READY_FLAG_SET_AFH)
		{
			pHci->need_write_afh_command = 0;
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
			if (Afh_GetMap(devExt, pHci->afh_channel_map))
			{
				Hci_SetAFHForAllDevice(devExt, AfhInstant);
			}
#else
			Hci_SetAFHForAllDevice(devExt, AfhInstant);
#endif
		}
		
		pHci->clock_ready_flag = 0;
	}
	if (pHci->pcurrent_connect_device)
	{
		if (pHci->pcurrent_connect_device->clock_ready_flag == BT_CLOCK_READY_FLAG_HODE_MODE)
		{
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_HODE_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
		}
		else if (pHci->pcurrent_connect_device->clock_ready_flag == BT_CLOCK_READY_FLAG_SNIFF_MODE)
		{
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_SNIFF_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
		}
		else if (pHci->pcurrent_connect_device->clock_ready_flag == BT_CLOCK_READY_FLAG_ROLE_SWITCH)
		{
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ROLE_SWITCH), BT_TASK_PRI_NORMAL, (PUINT8) &pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
		}
		else if (pHci->pcurrent_connect_device->clock_ready_flag == BT_CLOCK_READY_FLAG_SET_AFH_FOR_ONE_CONNECTION)
		{
			pHci->need_write_afh_command = 0;
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_SET_AFH), BT_TASK_PRI_NORMAL, (PUINT8)&pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND), BT_TASK_PRI_NORMAL, (PUINT8)&pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
		}
		pHci->pcurrent_connect_device->clock_ready_flag = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessReportRxPower
//
//    This function process the report rx power interrupt.
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
//     None.
//
//
//  IRQL:
//
//    This routine is called at IRQL_DISPATCH_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////

VOID BtProcessReportRxPowerInt(PBT_DEVICE_EXT devExt)
{
	UINT32  power;
	UINT8  i;
	UINT8  eachpower;
	PCONNECT_DEVICE_T pConnectDevice;
	PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;
	
	if (pHci == NULL)
		return;
	
	power = Hci_Read_DWord_From_3DspReg(devExt, BT_REG_AM_TRANSMITT_POWER);
	Hci_Write_DWord_To_3DspReg(devExt, BT_REG_AM_TRANSMITT_POWER, 0);
	for (i = 0; i < 9; i++)
	{
		eachpower = (UINT8)((power >> (2 * i)) & 0x3);
		if (eachpower)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessReportRxPowerInt am addr[%d] want to adjust power\n", i);
			if (i == 0 || i == 8) // Slave
			{
			#ifdef BT_USE_NEW_ARCHITECTURE
				pConnectDevice = Hci_Find_Slave_Connect_Device_By_Index(pHci, (UINT8)(i / 8));
			#else
				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
				pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			#endif
			}
			else
			{
				pConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, i);
			}
			if (pConnectDevice)
			{
				if (eachpower == 1) // Increace power
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Increase Power\n");
					LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_POWER_UP);
				}
				else if (eachpower == 2) // Decrease power
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Decrease power\n");
					LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_POWER_DOWN);
				}
			}
		}		
	}
	
	return;
}


///////////////////////////////////////////////////////////////////////////////
//
//  BtGetTxFrameType
//
//    This function get the type (command or data) according to the head of the
//    frame.
//
//  INPUTS:
//
//    buf - Address of the frame.
//    len - Length of the frame.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//    TX_FRAME_TYPE_DATA   type is data
//    TX_FRAME_TYPE_COMMAND type is command
//
//
//  IRQL:
//
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
BT_TX_FRAME_TYPE BtGetTxFrameType(PUINT8 buf, UINT32 len)
{
	if ((*buf == BT_HCI_PACKET_ACL_DATA) || (*buf == BT_HCI_PACKET_SCO_DATA))
	{
		return TX_FRAME_TYPE_DATA;
	}
	else
	{
		return TX_FRAME_TYPE_COMMAND;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtGetBDAddrById
//
//    This function is used to get BD's address according to its index
//
//
//  INPUTS:
//
//      id - index.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		DB's address
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT32 BtGetBDAddrById(UINT32 id)
{
	ASSERT(id < BD_MAX_COUNT);
	return (BT_BD_BASE_ADDR + EACH_BD_LENGTH * id);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtGetNextBDId
//
//    This function is used to get next BD's id no according to its current id
//
//
//  INPUTS:
//
//      id - current index.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		next BD's id no
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT32 BtGetNextBDId(UINT32 id)
{
	UINT32 tmpid = id;
	ASSERT(tmpid < BD_MAX_COUNT);
	if (tmpid == BD_MAX_COUNT - 1)
	{
		return (0);
	}
	else
	{
		return (++tmpid);
	}
}


BOOLEAN BtIsBDEmpty(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T	pFrag = NULL;
	KIRQL OldIrql;
	BOOLEAN  ret;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "frag pointer is null\n");
		return FALSE;
	}
	KeAcquireSpinLock(&pFrag->FragTxLock, &OldIrql);
	ret = ((pFrag->IdleSpace[FRAG_MASTER_LIST] == BT_MAX_MASTER_IDLE_SPACE_SIZE) && 
		((pFrag->IdleSpace[FRAG_SLAVE_LIST] == BT_MAX_SLAVE_IDLE_SPACE_SIZE)));
	KeReleaseSpinLock(&pFrag->FragTxLock, OldIrql);
	return ret;
}


BOOLEAN BtIsMasterBDEmpty(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T	pFrag = NULL;
	KIRQL OldIrql;
	BOOLEAN  ret;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "frag pointer is null\n");
		return FALSE;
	}
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtIsMasterBDEmpty: Master space: 0x%lx\n", pFrag->IdleSpace[FRAG_MASTER_LIST]);
	KeAcquireSpinLock(&pFrag->FragTxLock, &OldIrql);
	ret = (pFrag->IdleSpace[FRAG_MASTER_LIST] == BT_MAX_MASTER_IDLE_SPACE_SIZE);
	KeReleaseSpinLock(&pFrag->FragTxLock, OldIrql);
	return ret;
}

BOOLEAN BtIsSlaveBDEmpty(PBT_DEVICE_EXT devExt)
{
	PBT_FRAG_T	pFrag = NULL;
	KIRQL OldIrql;
	BOOLEAN  ret;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL0, "frag pointer is null\n");
		return FALSE;
	}
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtIsSlaveBDEmpty: slave space: %lx\n", pFrag->IdleSpace[FRAG_SLAVE_LIST]);
	KeAcquireSpinLock(&pFrag->FragTxLock, &OldIrql);
	ret = (pFrag->IdleSpace[FRAG_SLAVE_LIST] == BT_MAX_SLAVE_IDLE_SPACE_SIZE);
	KeReleaseSpinLock(&pFrag->FragTxLock, OldIrql);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtIsTxPoolFull
//
//    Check if BD is full or not
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		TRUE if bd is full
//      FALSE if bd is not full
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsTxPoolFull(PBT_DEVICE_EXT devExt)
{
	UINT32 i = 0;
	BOOLEAN IsFull = TRUE;
	KIRQL OldIrql;
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxPool[i].IsUsed == NOT_USED)
		{
			IsFull = FALSE;
			break;
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	return IsFull;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
PUINT8 BtGetValidTxPool(PBT_DEVICE_EXT devExt)
{
	UINT32 i = 0;
	PUINT8 pDest = NULL;
	KIRQL OldIrql;

	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);

	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxPool[i].IsUsed == NOT_USED)
		{
			devExt->TxPool[i].IsUsed = USED;
			pDest = devExt->TxPool[i].Buffer;
			devExt->TxAclPendingFlag = TRUE;
			devExt->AclPendingCounter = 0;
			break;
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	return pDest;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsTxScoPoolFull(PBT_DEVICE_EXT devExt)
{
	UINT32 i = 0;
	BOOLEAN IsFull = TRUE;
	KIRQL OldIrql;
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	
	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxScoPool[i].IsUsed == NOT_USED)
		{
			IsFull = FALSE;
			break;
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	return IsFull;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
PUINT8 BtGetValidTxScoPool(PBT_DEVICE_EXT devExt)
{
	UINT32 i = 0;
	PUINT8 pDest = NULL;
	KIRQL OldIrql;
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxScoPool[i].IsUsed == NOT_USED)
		{
			devExt->TxScoPool[i].IsUsed = USED;
			pDest = devExt->TxScoPool[i].Buffer;
			devExt->TxScoPendingFlag = TRUE;
			devExt->ScoPendingCounter = 0;
			break;
		}
	}
	//BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtGetValidTxScoPool----after acquire txspinlock,  tx pool index is %d\n", i);
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	return pDest;
}


///////////////////////////////////////////////////////////////////////////////
//
//  BtGetNextRxCachId
//
//    This function is used to get next rx cach's id no according to its current id
//
//
//  INPUTS:
//
//      id - current index.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		next rx cach's id no
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT32 BtGetNextRxCachId(UINT32 id)
{
	UINT32 tmpid = id;
	ASSERT(tmpid < MAX_RX_CACH_COUNT);
	if (tmpid == MAX_RX_CACH_COUNT - 1)
	{
		return (0);
	}
	else
	{
		return (++tmpid);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtIsRxCachEmpty
//
//    Check if rx cach is empty or not
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		TRUE if rx cach is empty
//      FALSE if rx cach is not empty
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsRxCachEmpty(PBT_DEVICE_EXT devExt)
{
	ASSERT(devExt->RxCachPid < MAX_RX_CACH_COUNT);
	ASSERT(devExt->RxCachCid < MAX_RX_CACH_COUNT);
	return (devExt->RxCachPid == devExt->RxCachCid);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtIsRxCachFull
//
//    Check if rx cach is full or not
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		TRUE if rx cach is full
//      FALSE if rx cach is not full
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsRxCachFull(PBT_DEVICE_EXT devExt)
{
	ASSERT(devExt->RxCachPid < MAX_RX_CACH_COUNT);
	ASSERT(devExt->RxCachCid < MAX_RX_CACH_COUNT);
	return (BtGetNextRxCachId(devExt->RxCachPid) == devExt->RxCachCid);
}

/**************************************************************
*Description:
*	To see whether there are too many rx packets in cach
**************************************************************/
BOOLEAN BtIsRxCachToBeFull(PBT_DEVICE_EXT devExt)
{
	UINT32	tmpPid, tmpCid, tmpRes, tmpPendingPackets;
	KIRQL	oldIrql;
	
	ASSERT(devExt->RxCachPid < MAX_RX_CACH_COUNT);
	ASSERT(devExt->RxCachCid < MAX_RX_CACH_COUNT);

	/*
	KeAcquireSpinLock(&devExt->ThreadSpinLock, &oldIrql);
	totalBytes = devExt->SerialInfo.SerialStatus.AmountInInQueue;
	KeReleaseSpinLock(&devExt->ThreadSpinLock, oldIrql);
	if(totalBytes > 4000)	//4000 is a temp top threshold
	{
		return TRUE;
	}
	*/
	KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
	tmpPendingPackets = devExt->RxPacketNum;
	KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);

	tmpCid = devExt->RxCachCid;
	tmpPid = devExt->RxCachPid;


	
	tmpRes = (tmpPid + MAX_RX_CACH_COUNT - tmpCid)%MAX_RX_CACH_COUNT;
	tmpRes = MAX_RX_CACH_COUNT - tmpRes - 1;	//free rx cach count

	if(tmpRes <= MAX_RX_CACH_COUNT/2)
		return TRUE;
	else 
		return FALSE;

	/*
	if((tmpRes - tmpPendingPackets) > 0)
		return FALSE;
	else
		return TRUE;
	*/

}

/* jakio20080105: if rx cach's elements is over half of the total, it is not good.
*/
BOOLEAN BtIsRxCachGood(PBT_DEVICE_EXT devExt)
{
	UINT32	tmpPid, tmpCid, tmpRes;
	
	ASSERT(devExt->RxCachPid < MAX_RX_CACH_COUNT);
	ASSERT(devExt->RxCachCid < MAX_RX_CACH_COUNT);

	tmpCid = devExt->RxCachCid;
	tmpPid = devExt->RxCachPid;

	/*
	KeAcquireSpinLock(&devExt->ThreadSpinLock, &oldIrql);
	totalBytes = devExt->SerialInfo.SerialStatus.AmountInInQueue;
	KeReleaseSpinLock(&devExt->ThreadSpinLock, oldIrql);
	if(totalBytes > 500)	//500 is a temp bottom threshold
	{
		return FALSE;
	}
	*/

	tmpRes = (tmpPid + MAX_RX_CACH_COUNT - tmpCid)%MAX_RX_CACH_COUNT;

	tmpRes = MAX_RX_CACH_COUNT - tmpRes - 1;	//free rx cach count

	if(tmpRes > (MAX_RX_CACH_COUNT/2))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtIsRxCachGood---free rx cach count:%lu. Good status\n",tmpRes);
		return TRUE;
	}
	else
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtIsRxCachGood---free rx cach count:%lu, Bad status\n",tmpRes);
		return FALSE;
	}
		
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtGetScoBDAddrById
//
//    This function is used to get SCO BD's address according to its index
//
//
//  INPUTS:
//
//      id - index.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		DB's address
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
UINT32 BtGetScoBDAddrById(UINT32 id)
{
	ASSERT(id < BD_MAX_COUNT);
	return (BT_BD_SCO_BASE_ADDR + EACH_BD_LENGTH * id);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtIsScoBDEmpty
//
//    Check if SCO BD is empty or not
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		TRUE if sco bd is empty
//      FALSE if sco bd is not empty
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsScoBDEmpty(PBT_DEVICE_EXT devExt)
{
	return FALSE; //(devExt->TxScoPid == devExt->TxScoCid);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtIsScoBDFull
//
//    Check if SCO BD is full or not
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		TRUE if sco bd is full
//      FALSE if sco bd is not full
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN BtIsScoBDFull(PBT_DEVICE_EXT devExt)
{
	return FALSE; //(BtGetNextBDId(devExt->TxScoPid) == devExt->TxScoCid);
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtGetRxFrameType
//
//    Get the rx frame's type
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      startaddr - the start address of the frame in rx buffer
//      datalen - the length of the frame
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
BT_RX_FRAME_TYPE BtGetRxFrameType(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 datalen, PUINT8 Buf)
{
	UINT32 tmplong;
	UINT8 type;
	UINT8 l_ch;
	tmplong = (UINT32)(*(PUINT32)(PUINT8)Buf + 4); //Hci_Read_DWord_From_3DspReg(devExt,(startaddr));
	type = (UINT8)((tmplong &0x78) >> 3);
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "datalen = %lu, type = %d\n", datalen, type);
	if (type == (UINT8)BT_ACL_PACKET_NULL)
	{
		return RX_FRAME_TYPE_NULL;
	}
	else if (type == (UINT8)BT_ACL_PACKET_POLL)
	{
		return RX_FRAME_TYPE_POLL;
	}
	if (((tmplong &0xff80) >> 7) == 0)
	{
		return RX_FRAME_TYPE_CRC_ERROR;
	}
	if ((type >= (UINT8)BT_SCO_PACKET_HV1) && (type <= (UINT8)BT_SCO_PACKET_HV3))
	{
		return RX_FRAME_TYPE_SCO;
	}
	if (type == (UINT8)BT_SCO_PACKET_DV)
	{
		return RX_FRAME_TYPE_DV;
	}
	l_ch = *(PUINT8)Buf + 4+sizeof(HCBB_PAYLOAD_HEADER_T);
	l_ch &= 0x3;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "l_ch = %d", l_ch);
	if (l_ch == (UINT8)BT_LCH_TYPE_LMP)
	{
		return RX_FRAME_TYPE_LMPPDU;
	}
	return RX_FRAME_TYPE_DATA;
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtGetRxFrameTypeAndRetryCount
//
//    Get the rx frame's type and retry count.
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      startaddr - the start address of the frame in rx buffer
//      pretrycount - the pointer to the retry count.
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
BT_RX_FRAME_TYPE BtGetRxFrameTypeAndRetryCount(PBT_DEVICE_EXT devExt, UINT32 startaddr, PUINT32 pretrycount, 
		PUINT8 pam_addr, PUINT8 ptype, PUINT8 flow, PUINT8 pmaster_slave_flag, PUINT8 pslave_index, PUINT8 pHeadDesc)
{
	UINT8 type;
	UINT8 l_ch;
	UINT16 length;
	PHCBB_PAYLOAD_HEADER_T pHeader = NULL;
	pHeader = (PHCBB_PAYLOAD_HEADER_T)(pHeadDesc + 4);
	type = (UINT8)pHeader->type;
	*pretrycount = (UINT32)pHeader->tx_retry_count;
	*pam_addr = (UINT8)pHeader->am_addr;
	*pmaster_slave_flag = (UINT8)pHeader->master_slave_flag;
	*pslave_index = (UINT8)pHeader->slave_index;
	*ptype = type;
	length = (UINT16)pHeader->length;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "retry count = %lu, type = %d, am_addr = %d, len = %d, master_slave_flag = %d, slave_index = %d\n",  *pretrycount, type,  *pam_addr, length, *pmaster_slave_flag,  *pslave_index);
	if (type == (UINT8)BT_ACL_PACKET_NULL)
	{
		return RX_FRAME_TYPE_NULL;
	}
	else if (type == (UINT8)BT_ACL_PACKET_POLL)
	{
		return RX_FRAME_TYPE_POLL;
	}
	else if (type == (UINT8)BT_ACL_PACKET_FHS)
	{
		if (length == 1)
		{
			return RX_FRAME_TYPE_AFH_CH_BAD;
		}
	}
	if(length == 0)	//Jakio20090204: length takes 10bits, so above adjugement is uncorrect
	{
		if ((type >= (UINT8)BT_SCO_PACKET_HV1) && (type <= (UINT8)BT_SCO_PACKET_HV3))
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Get Rx Frame type: RX_FRAME_TYPE_SCO_NULL\n");
			return RX_FRAME_TYPE_SCO_NULL;
		}
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Get Rx Frame type: RX_FRAME_TYPE_CRC_ERROR\n");
		return RX_FRAME_TYPE_CRC_ERROR;
	}
	if ((type >= (UINT8)BT_SCO_PACKET_HV1) && (type <= (UINT8)BT_SCO_PACKET_HV3))
	{
		BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Get Rx Frame type: RX_FRAME_TYPE_SCO\n");
		return RX_FRAME_TYPE_SCO;
	}

	/*
	if (type == (UINT8)BT_SCO_PACKET_DV)
	{
		return RX_FRAME_TYPE_DV;
	}
	*/
	
	l_ch = *(PUINT8)(pHeadDesc + 4+sizeof(HCBB_PAYLOAD_HEADER_T));
	if ((l_ch &0x3) == (UINT8)BT_LCH_TYPE_LMP)
	{
		return RX_FRAME_TYPE_LMPPDU;
	}
	if (l_ch &0x4)
	{
		*flow = 0;
	}
	else
	{
		*flow = 1;
	}
	if (type == BT_ACL_PACKET_DM1)
	{
		if ((l_ch &0xf8) == 0)
		{
			return RX_FRAME_TYPE_DATA_NULL;
		}
	}
	return RX_FRAME_TYPE_DATA;
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtTransferRxFrameToRxCach
//
//    Copy the frame in rx buffer into the rx cach
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      startaddr - the start address of the frame in rx buffer.
//      tailaddr - the tail address of the frame in rx buffer.
//      datalen - the length of the frame
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
VOID BtTransferRxFrameToRxCach(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 tailaddr, UINT32 datalen, PUINT8 SrcBuf, PUINT8 destbuf)
{
	PUINT8 tmpbuf = destbuf;
	PUINT8 tmpsrcbuf = SrcBuf + startaddr;
	*(PUINT16)tmpbuf = (UINT16)datalen;
	tmpbuf += sizeof(UINT16);
	*(PUINT16)tmpbuf = (UINT16)RX_FRAME_TYPE_DATA;
	tmpbuf += sizeof(UINT16);
	if (tailaddr > startaddr)
	{
		RtlCopyMemory(tmpbuf, tmpsrcbuf, ALIGNLONGDATALEN(datalen));
	}
	else
	{
		RtlCopyMemory(tmpbuf, tmpsrcbuf, BT_RX_BUFFER_TAIL - startaddr);
		tmpbuf += BT_RX_BUFFER_TAIL - startaddr;
		RtlCopyMemory(tmpbuf, tmpsrcbuf + BT_RX_BUFFER_TAIL - startaddr, (UINT8)(ALIGNLONGDATALEN(datalen) - (BT_RX_BUFFER_TAIL - startaddr)));
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtTransferRxFrameBodyToRxCach
//
//    Copy the frame body in rx buffer into the rx cach
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//      startaddr - the start address of the frame in rx buffer.
//      tailaddr - the tail address of the frame in rx buffer.
//      datalen - the length of the frame
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
VOID BtTransferRxFrameBodyToRxCach(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 tailaddr, UINT32 datalen, PUINT8 srcBuf, PUINT8 destbuf)
{
	PUINT8 tmpbuf = destbuf;
	if (tailaddr > startaddr)
	{
		RtlCopyMemory(tmpbuf, srcBuf + startaddr + sizeof(HCBB_PAYLOAD_HEADER_T), ALIGNLONGDATALEN(datalen) - sizeof(HCBB_PAYLOAD_HEADER_T));
	}
	else
	{
		ASSERT(0);
		/*
		if ((startaddr + 4) < BT_RX_BUFFER_TAIL)
		{
		UsbReadFrom3DspRegs(devExt, (startaddr + 4), (UINT8)(BT_RX_BUFFER_TAIL - startaddr - 4), tmpbuf);
		tmpbuf += BT_RX_BUFFER_TAIL - startaddr - 4;
		UsbReadFrom3DspRegs(devExt, (BT_RX_BUFFER_BASE_ADDR), (UINT8)(ALIGNLONGDATALEN(datalen) - (BT_RX_BUFFER_TAIL - startaddr)), tmpbuf);
		}
		else
		{
		UsbReadFrom3DspRegs(devExt, (BT_RX_BUFFER_BASE_ADDR), (UINT8)(ALIGNLONGDATALEN(datalen) - 4), tmpbuf);
		}
		 */
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxLMPPDU
//
//    Process the LMPPDU cach
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
VOID BtProcessRxLMPPDU(PBT_DEVICE_EXT devExt, UINT8 index)
{
	KIRQL oldIrql;
	UINT32 DataLen;
#ifdef BT_USE_NEW_ARCHITECTURE
	UINT8 tmpamaddr;
	UINT8 tmpmasterslaveflag;
	UINT8 tmpslaveid;
#else
	UINT8 tmpchar;
#endif
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;
	PPAYLOAD_HEADER_SINGLE_SLOT_T pSingHead;
	PHCBB_PAYLOAD_HEADER_T pHCBBHead;
	PUINT8 tmpbuf; //= devExt->RxLMPPDUCach[index].buffer;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxLMPPDU Enter! cach index is:%d\n", index);
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		goto exit;
	}
	ASSERT(index < MAX_LMP_CACH_COUNT);

	tmpbuf = devExt->RxLMPPDUCach[index].buffer;
	
	DataLen = *(PUINT32)tmpbuf;
	tmpbuf += sizeof(UINT32);

	pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)tmpbuf;
#ifdef BT_USE_NEW_ARCHITECTURE

	/*
	tmpamaddr = (*tmpbuf) &0x7;
	tmpmasterslaveflag = *(tmpbuf + 2) &0x1;
	tmpslaveid = (*(tmpbuf + 2) &0x2) >> 1;
	*/
	tmpamaddr = (UINT8)pHCBBHead->am_addr;
	tmpmasterslaveflag = (UINT8)pHCBBHead->master_slave_flag;
	tmpslaveid = (UINT8)pHCBBHead->slave_index;
	
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Am address = %d, master_slave_flag = %d, slave_index = %d\n", tmpamaddr, tmpmasterslaveflag, tmpslaveid);
	if (tmpmasterslaveflag == BT_HCBB_MASTER_FLAG)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpamaddr);
	}
	else
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, tmpamaddr, tmpslaveid);
	}
#else
	tmpchar = (*tmpbuf) &0x7;
	if (pHci->role == BT_ROLE_MASTER)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
		if (pTempConnectDevice == NULL)
		{
			pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
		}
	}
	else
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr(pHci, tmpchar);
		if (pTempConnectDevice == NULL)
		{
			pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpchar);
		}
	}
#endif
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->rx_sco_con_null_count = 0;
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
		{
			Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
		}
		pHCBBHead = (PHCBB_PAYLOAD_HEADER_T)tmpbuf;
		pSingHead = (PPAYLOAD_HEADER_SINGLE_SLOT_T)(tmpbuf + sizeof(HCBB_PAYLOAD_HEADER_T));
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		LMP_PDU_LC2LM_Process(devExt, pTempConnectDevice, tmpbuf + sizeof(HCBB_PAYLOAD_HEADER_T) + sizeof(PAYLOAD_HEADER_SINGLE_SLOT_T), (UINT8)pSingHead->length);
	}
	else if(pHci->role_switching_flag == 1)
	{
		Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_LMP_PDU_PROCESS), BT_TASK_PRI_NORMAL, &index, 1);
		return;
	}
exit:
	devExt->RxLMPPDUCach[index].Flag_InUse =  FALSE;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
VOID BtProcessRxNULLFrame_New(PBT_DEVICE_EXT devExt, UINT32 tmpstartaddr, PUINT8 pBuf)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT32 tmplong;
	UINT8 tmpamaddr;
	UINT8 tmpmasterslaveflag;
	UINT8 tmpslaveid;
	PHCBB_PAYLOAD_HEADER_T pHeader = NULL;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxNULLFrame_New Enter!\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	pHeader = (PHCBB_PAYLOAD_HEADER_T)(pBuf + tmpstartaddr);
	tmplong = (UINT32)(*(PUINT32)((PUINT8)pBuf + tmpstartaddr)); //Hci_Read_DWord_From_3DspReg(devExt, tmpstartaddr,0);
	tmpamaddr = (UINT8)pHeader->am_addr;
	tmpmasterslaveflag = (UINT8)pHeader->master_slave_flag;
	tmpslaveid = (UINT8)pHeader->slave_index;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Am address = %d, master_slave_flag = %d, slave_index = %d\n", tmpamaddr, tmpmasterslaveflag, tmpslaveid);
	if (tmpmasterslaveflag == BT_HCBB_MASTER_FLAG)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpamaddr);
	}
	else
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, tmpamaddr, tmpslaveid);
	}
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->rx_sco_con_null_count = 0;
        BT_DBGEXT(ZONE_SDRCV | LEVEL3, "####Connection state %d####\n", pTempConnectDevice->state);
		switch (pTempConnectDevice->state)
		{
			case BT_DEVICE_STATE_PAGING:
				
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				Hci_StopTimer(pTempConnectDevice);
				if (pTempConnectDevice->tempflag == 1)
				{
					pTempConnectDevice->state = BT_DEVICE_STATE_NAME_REQ;
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get name Enter!\n");
					Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_NAME_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get version Enter1!\n");
					pTempConnectDevice->state = BT_DEVICE_STATE_PAGED;
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get version Enter!\n");
					Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_VERSION_REQ), BT_TASK_PRI_NORMAL, (PUINT8) &pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				break;
			case BT_DEVICE_STATE_DETACH:
				pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT;
				
				break;
			case BT_DEVICE_STATE_CONNECTED:
			case (BT_DEVICE_STATE_SLAVE_CONNECTED):
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
				{
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
				}
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				break;
			case BT_DEVICE_STATE_DISCONNECT:
			pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN;
			break;
			default:
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxNULLFrame()--error state 0x%x, handle 0x%x\n", pTempConnectDevice->state, pTempConnectDevice->connection_handle);
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxNULLFrame
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
VOID BtProcessRxNULLFrame(PBT_DEVICE_EXT devExt, UINT32 tmpstartaddr, PUINT8 pBuf)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT32 tmplong;
	UINT8 tmpamaddr;
	UINT8 tmpmasterslaveflag;
	UINT8 tmpslaveid;
	PHCBB_PAYLOAD_HEADER_T pHeader = NULL;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxNULLFrame Enter!\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	pHeader = (PHCBB_PAYLOAD_HEADER_T)(pBuf + tmpstartaddr);
	tmplong = (UINT32)(*(PUINT32)((PUINT8)pBuf + tmpstartaddr)); //Hci_Read_DWord_From_3DspReg(devExt, tmpstartaddr,0);
	tmpamaddr = (UINT8)pHeader->am_addr;
	tmpmasterslaveflag = (UINT8)pHeader->master_slave_flag;
	tmpslaveid = (UINT8)pHeader->slave_index;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Am address = %d, master_slave_flag = %d, slave_index = %d\n", tmpamaddr, tmpmasterslaveflag, tmpslaveid);
	if (tmpmasterslaveflag == BT_HCBB_MASTER_FLAG)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, tmpamaddr);
	}
	else
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, tmpamaddr, tmpslaveid);
	}
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->rx_sco_con_null_count = 0;
		switch (pTempConnectDevice->state)
		{
			case (BT_DEVICE_STATE_PAGING):
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			Hci_StopTimer(pTempConnectDevice);
			if (pTempConnectDevice->tempflag == 1)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_NAME_REQ;

				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get name Enter!\n");
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_NAME_REQ);
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get version Enter1!\n");
				pTempConnectDevice->state = BT_DEVICE_STATE_PAGED;
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "set task get version Enter!\n");
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_VERSION_REQ);
			}
			break;
			case (BT_DEVICE_STATE_DETACH):
			pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT;
			Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *3 * 625 / 5000) + 1));
			break;
			case (BT_DEVICE_STATE_CONNECTED):
            case (BT_DEVICE_STATE_SLAVE_CONNECTED):
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
			{
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
			}
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
			case BT_DEVICE_STATE_DISCONNECT:
			pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN;
			Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *3 * 625 / 5000) + 1));
			break;
			default:
				BT_DBGEXT(ZONE_SDRCV | LEVEL0, "BtProcessRxNULLFrame()--error state %x\n", pTempConnectDevice->state);
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxPoolFrame
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
VOID BtProcessRxPoolFrame(PBT_DEVICE_EXT devExt, PUINT8 pBuf)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT32 tmplong;
	UINT8 tmpamaddr;
	UINT8 tmpslaveid;
	UINT8 tmpmasterslaveflag;
	PHCBB_PAYLOAD_HEADER_T pHeader = NULL;
	PCONNECT_DEVICE_T pTempConnectDevice;
	NTSTATUS status;
#ifdef DSP_REG_WRITE_API
	UINT8		buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif

	
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxPoolFrame Enter!\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		ASSERT(0);
		return ;
	}
	pHeader = (PHCBB_PAYLOAD_HEADER_T)(pBuf + 4);
	tmplong = (UINT32)(*(PUINT32)((PUINT8)pBuf + 4)); //Hci_Read_DWord_From_3DspReg(devExt, tmpstartaddr,0);
	tmplong = (UINT32)(*(PUINT32)((PUINT8)pBuf + 4)); //Hci_Read_DWord_From_3DspReg(devExt, tmpstartaddr,0);
	tmpamaddr = (UINT8)pHeader->am_addr;
	tmpmasterslaveflag = (UINT8)pHeader->master_slave_flag;
	tmpslaveid = (UINT8)pHeader->slave_index;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Am address = %d, slave index = %d\n", tmpamaddr, tmpslaveid);
	pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, tmpamaddr, tmpslaveid);
	if (pTempConnectDevice != NULL)
	{
	    BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Poll frame for link supervise, state %d\n", pTempConnectDevice->state);
		pTempConnectDevice->rx_sco_con_null_count = 0;
		switch (pTempConnectDevice->state)
		{
			case (BT_DEVICE_STATE_PAGING): 
                break;
			case (BT_DEVICE_STATE_DETACH): 
                break;
			case (BT_DEVICE_STATE_CONNECTED): 
            case (BT_DEVICE_STATE_SLAVE_CONNECTED):
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
			{
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
			}
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
		}
	}
	else
	{
		KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
		if(devExt->RxState == RX_STATE_CONNECTING)
		{
			KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
			BtTaskProcessRxPoolFrame(devExt);
			KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
		}
		else
		{
			/*
			devExt->RxState = RX_STATE_CONNECTING;
			devExt->RxPacketNum = 1;
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "RxPacketNum is %d\n", devExt->RxPacketNum);
			
			sc_spin_unlock_bh(&devExt->RxStateLock);
			Task_CreateTaskForRxData((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_FHS_FRAME_PROCESS),BT_TASK_PRI_NORMAL,NULL, 0);
			sc_spin_lock_bh(&devExt->RxStateLock);
			*/
			KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
			status = Task_CreateTaskForRxData((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_RX_FHS_FRAME_PROCESS),BT_TASK_PRI_NORMAL,NULL, 0);
			if(status == STATUS_SUCCESS)
			{
				KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
				devExt->RxState = RX_STATE_CONNECTING;
				devExt->RxPacketNum = 1;
				BT_DBGEXT(ZONE_SDRCV | LEVEL3, "RxPacketNum is %d\n", devExt->RxPacketNum);
			}
			else
			{
				BT_DBGEXT(ZONE_SDRCV | LEVEL0, "BtProcessRxPollFrame--create task failed\n");
				KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
			}
		}
		KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
		return;

		
	}


}


VOID BtTaskProcessRxPoolFrame(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T	pHci;
	FHS_PACKET_T fhs_packet;
	UINT8 am_addr;
	UINT8 bd_addr[BT_BD_ADDR_LENGTH];
	UINT16 packet_type;
	UINT8 role_switch;
	NTSTATUS status;
	UINT8 tmpslaveid = 0;	//Jakio20080226: fix it to 0 at present
	PCONNECT_DEVICE_T	pConDev = NULL;
	
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtTaskProcessRxPoolFrame Enter!\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}

	#ifdef BT_COMBO_SNIFF_SUPPORT 
	if(pHci->lmp_features.byte0.sniff_mode == 1)
	{
		PCONNECT_DEVICE_T	pConnectDevice = NULL;
		UINT32	i;
		for(i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
		{
			pConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice;
			if(pConnectDevice != NULL)
			{
				((PBT_LMP_T)devExt->pLmp)->sniff_index = i;
				status = Send_UnsniffReq_PDU(devExt, pConnectDevice);	
			}
			
		}	
	}
	#endif

	/* jakio20070731: mark here
*/
	UsbReadFrom3DspRegs(devExt, BT_REG_FHS_FOR_PAGE_SCAN, sizeof(FHS_PACKET_T) / 4, (PUINT8) &fhs_packet);
	bd_addr[0] = (UINT8)fhs_packet.lap;
	bd_addr[1] = (UINT8)(fhs_packet.lap >> 8);
	bd_addr[2] = (UINT8)(fhs_packet.lap >> 16);
	bd_addr[3] = fhs_packet.uap;
	bd_addr[4] = (UINT8)fhs_packet.nap;
	bd_addr[5] = (UINT8)(fhs_packet.nap >> 8);

    BT_DBGEXT(ZONE_SDRCV | LEVEL3, "bdaddr: %2x:%2x:%2x:%2x:%2x:%2x\n", bd_addr[0], bd_addr[1], bd_addr[2],
                                                                        bd_addr[3], bd_addr[4], bd_addr[5]);
	/* jakio20070731: mark here
*/
	if ((pConDev = Hci_Find_Connect_Device_By_BDAddr(pHci, bd_addr)) || 
			(pConDev = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, bd_addr)))
	{
		if(pConDev->role_switching_flag == 1)
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Device %2x is in role switching, just ignore the poll frame\n", pConDev->bd_addr[0]);
		}
		else
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "ACL connection already exists when in savle page prcess\n");
			Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((tmpslaveid+ 1) % 2), 0);
			Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, tmpslaveid* 8);	
		}
		
		
		return ;
	}

	am_addr = (UINT8)fhs_packet.am_addr;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Create slave connection am address = %d\n", am_addr);
	
	packet_type = BT_PACKET_TYPE_DM1 | BT_PACKET_TYPE_DH1 | BT_PACKET_TYPE_DM3 | BT_PACKET_TYPE_DH3 | BT_PACKET_TYPE_DM5 | BT_PACKET_TYPE_DH5;
	role_switch = 1;

	status = Hci_Add_Slave_Connect_Device(pHci, am_addr, bd_addr, fhs_packet.class_of_device, packet_type, (UINT8)fhs_packet.sr, (UINT8)fhs_packet.page_scan_mode, (UINT16)fhs_packet.clk27_2, role_switch, tmpslaveid);

	if (status == STATUS_SUCCESS)
	{
		pHci->pcurrent_connect_device->state = BT_DEVICE_STATE_WAIT_CONN_REQ;
		if (pHci->sco_link_count > 0)
		{
			pHci->pcurrent_connect_device->max_slot = BT_MAX_SLOT_1_SLOT;
		}
		Hci_StartTimer(pHci->pcurrent_connect_device, BT_TIMER_TYPE_CONN_ACCEPT, (UINT16)(((UINT32)pHci->conn_accept_timeout *625) / (1000 *1000) + 1));
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxCRCErrorFrame
//
//    Process the condition that received CRC error frame
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
VOID BtProcessRxCRCErrorFrame(PBT_DEVICE_EXT devExt, UINT8 master_slave_flag, UINT8 amaddr, UINT8 slaveIndex)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxCRCErrorFrame Enter!\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		ASSERT(0);
		return ;
	}


	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Am address = %d, master_slave_flag = %d, slave_index = %d\n", amaddr, master_slave_flag, slaveIndex);
	if (master_slave_flag == BT_HCBB_MASTER_FLAG)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, amaddr);
	}
	else
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(pHci, amaddr, slaveIndex);
	}
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->rx_sco_con_null_count = 0;
		switch (pTempConnectDevice->state)
		{
			case (BT_DEVICE_STATE_PAGING): break;
			case (BT_DEVICE_STATE_DETACH): break;
			case (BT_DEVICE_STATE_CONNECTED): case (BT_DEVICE_STATE_SLAVE_CONNECTED):
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pTempConnectDevice->timer_valid == 1) && (pTempConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION))
			{
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
			}
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			break;
		}
	}


}

///////////////////////////////////////////////////////////////////////////////
//
//  BtProcessRxAFHChBadFrame
//
//    Process the condition that received AFH bad channel frame
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
VOID BtProcessRxAFHChBadFrame(PBT_DEVICE_EXT devExt, UINT32 BadChanNum)
{
	/*
	if ((tmpstartaddr + 4) < BT_RX_BUFFER_TAIL)
	{
		tmpchar = Hci_Read_Byte_From_3DspReg(devExt, tmpstartaddr + sizeof(HCBB_PAYLOAD_HEADER_T), 0);
	}
	else
	{
		tmpchar = Hci_Read_Byte_From_3DspReg(devExt, BT_RX_BUFFER_BASE_ADDR, 0);
	}
	*/
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "BtProcessRxAFHChBadFrame Enter, channel no = %lu\n", BadChanNum);
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
	Afh_Statistic(devExt, (UINT8)BadChanNum);
#endif

}

///////////////////////////////////////////////////////////////////////////////
//
//  BtWriteFHSPacket
//
//    Write FHS packet to register.
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
VOID BtWriteFHSPacket(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pTempConnectDevice)
{
	FHS_PACKET_T fhs_packet;
	UINT8 tempaccesscode[9];
	PUINT64 ptmplonglong;
	UINT8 am_addr;
	NTSTATUS status;
	PBT_HCI_T pHci;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif
	
	pHci = (PBT_HCI_T)devExt->pHci;
	status = Hci_Allocate_Am_address(pHci, &am_addr);
	if (status != STATUS_SUCCESS)
	{
		am_addr = 0;
	}
	AccessCode_Gen(pTempConnectDevice->bd_addr, tempaccesscode);
#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 9;
	offset = BT_REG_PAGE_ACCESS_CODE;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, tempaccesscode);
#else
	Hci_Write_Page_Access_Code(devExt, tempaccesscode);
#endif
	AccessCode_Gen(pHci->local_bd_addr, tempaccesscode);
	BT_DBGEXT(ZONE_SDRCV | LEVEL3, "The length of FHS_PACKET_T = %d\n", sizeof(FHS_PACKET_T));
	ptmplonglong = (PUINT64)tempaccesscode;
	*ptmplonglong =  *ptmplonglong >> 4;
	RtlCopyMemory(&fhs_packet, ptmplonglong, 8);
	fhs_packet.undefined = 0;
	fhs_packet.sr = 0;
	fhs_packet.sp = 0;
	fhs_packet.uap = pHci->local_bd_addr[3];
	fhs_packet.nap = *(PUINT16)(&pHci->local_bd_addr[4]);
	RtlCopyMemory(fhs_packet.class_of_device, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	fhs_packet.am_addr = am_addr;
	fhs_packet.page_scan_mode = 0;
#ifdef DSP_REG_WRITE_API
	length = sizeof(FHS_PACKET_T);
	offset = BT_REG_FHS_FOR_PAGE;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8) &fhs_packet);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	UsbWriteTo3DspRegs(devExt, BT_REG_FHS_FOR_PAGE, sizeof(FHS_PACKET_T) / 4, (PUINT8) &fhs_packet);
#endif
	pTempConnectDevice->am_addr_for_fhs = am_addr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  BtInitializeTimer
//
//    Initialize timer driver used
//
//
//  INPUTS:
//
//      devExt - Pointer to device extension of device to start.
//
//
//  OUTPUTS:
//
//      NONE
//
//  RETURNS:
//		
//		None
//	
//  IRQL:
//
//      This routine is called at IRQL PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
VOID BtInitializeTimer(PBT_DEVICE_EXT devExt)
{
	//Init watchdog timer
	init_timer(&devExt->watchDogTimer);
	devExt->watchDogTimer.function = BtWatchdogTimer;
	devExt->watchDogTimer.data = (ULONG_PTR)devExt;
}

///////////////////////////////////////////////////////////////////////////////
//
// BtTriggerRcv
//
//    Trigger the receive routine of HCI packets
//
//
//    Called in Tasklet level.
///////////////////////////////////////////////////////////////////////////////
VOID BtTriggerRcv(ULONG_PTR SystemContext)
{
	PBT_DEVICE_EXT devExt = (PBT_DEVICE_EXT)SystemContext;
	KIRQL oldIrql;

	ENTER_FUNC();
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (!BtIsRxCachEmpty(devExt) || !Frag_IsAllScoRxCachEmpty((PBT_FRAG_T)devExt->pFrag))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		BtProcessRx_skb(devExt);
	}
	else
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	}
    
    EXIT_FUNC();
}


VOID BtCheckAndChangeRxState(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	KIRQL	oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8	need_change_state = 0;
	if(devExt == NULL)
		return;
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
		return;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if(QueueEmpty(&pHci->device_slave_list))
	{
		need_change_state = 1;
	}
	else
	{
		need_change_state = 1;
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
		while(pConnectDevice != NULL)
		{
			if(pConnectDevice->connection_state != 1)
				need_change_state = 0;
			pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
		}
		
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	if(need_change_state == 1)
	{
		KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
		if((devExt->RxState == RX_STATE_CONNECTING) && (devExt->RxPacketNum == 0))
		{
			BT_DBGEXT(ZONE_SDRCV | LEVEL3, "Pre_process---change connection state to RX_STATE_CONNECTED\n");
			devExt->RxState = RX_STATE_CONNECTED;
		}
		KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
	}
}
