/****************************************************************
 * FILENAME:     BT_Task.c     CURRENT VERSION: 1.00.01
 * CREATE DATE:  2005/08/15
 * PURPOSE:      We package all the thread process into this file.
 *               And in the routine of this kernel thread, we process
 *               our task.
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
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
#include "bt_frag.h"			
#ifdef BT_LOOPBACK_SUPPORT
#include "bt_loopback.h"
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
/*--file local function prototypes-------------------------------------*/



static void *CreateTaskBlock(PBT_TASK_T pTask, void *pTaskFun)
{
	PTASK_DATA_BLOCK_T pBlock = NULL;

    // Directly alloc
    pBlock = kzalloc(sizeof(TASK_DATA_BLOCK_T), GFP_ATOMIC);
	if(pBlock){
	pBlock->tmpFlag = 1;
	pBlock->devExt = pTask->pDevExt;
	init_timer(&pBlock->dwork.timer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)		
	INIT_WORK(&pBlock->dwork.work, pTaskFun);
#else		
	INIT_WORK(&pBlock->dwork, pTaskFun, pBlock);
#endif
	}
	return pBlock;
}

static void FreeTaskBlock(PTASK_DATA_BLOCK_T pBlock)
{
    del_timer(&pBlock->dwork.timer);
    kfree(pBlock);
}

/**************************************************************************
 *   Task_ThreadRoutine
 *
 *   Descriptions:
 *      The thread routine.
 *   Arguments:
 *      StartContext: IN, the thread context(always is the adapter context).
 *   Return Value:
 *      NONE
 *************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
VOID Task_ThreadRoutine(struct work_struct * pwork) 
#else
VOID Task_ThreadRoutine(ULONG_PTR para)
#endif
{
	PBT_DEVICE_EXT devExt;
	PBT_TASK_T pTask;
	PTASK_DATA_BLOCK_T ptmpdatablock;
	PBT_HCI_T pHci;
	KIRQL oldIrql;
	UINT32 eventtype;
	UINT32 eventnum;
	UINT32	tmpLong;
	UINT8 Task_RxPool_Flag;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)	
	struct delayed_work *dw;	
	/* Parse the structure */	
	dw = container_of(pwork, struct delayed_work, work);
	ptmpdatablock  = (PTASK_DATA_BLOCK_T)container_of(dw, TASK_DATA_BLOCK_T, dwork);
#else	
	ptmpdatablock  = (PTASK_DATA_BLOCK_T)para;
#endif


	if(ptmpdatablock == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_ThreadRoutine---null data block\n");
		return;
	}
	
	devExt = (PBT_DEVICE_EXT)ptmpdatablock->devExt;
	pTask = (PBT_TASK_T)devExt->pTask;
	pHci = (PBT_HCI_T)devExt->pHci;


	if(1)
	{
		
		/* Unlock */


		if(ptmpdatablock)
		{
			ptmpdatablock->in_list_flag = 0;
			Task_RxPool_Flag = 0;
			/* Get event type */
			eventtype = BT_TASK_GET_EVENT_TYPE(ptmpdatablock->task_event);
			/* Get event number */
			eventnum = BT_TASK_GET_EVENT_NUMBER(ptmpdatablock->task_event);
			/* Process task directly */
			switch (eventtype)
			{
				case BT_TASK_EVENT_TYPE_HCI2LMP:
					switch (eventnum)
					{
					case BT_TASK_EVENT_HCI2LMP_TEST:
						break;
					case BT_TASK_EVENT_HCI2LMP_CREATE_CONNECTION:
					case BT_TASK_EVENT_HCI2LMP_LINK_SUPERVISION_TIMEOUT:
					case BT_TASK_EVENT_HCI2LMP_CLK_OFFSET_REQ:
					case BT_TASK_EVENT_HCI2LMP_NAME_REQ:
					case BT_TASK_EVENT_HCI2LMP_FEATURE_REQ:
					case BT_TASK_EVENT_HCI2LMP_VERSION_REQ:
					case BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN:
					case BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_CONN:
					case BT_TASK_EVENT_HCI2LMP_DETACH:
					case BT_TASK_EVENT_HCI2LMP_LINK_KEY_REPLY:
					case BT_TASK_EVENT_HCI2LMP_PIN_CODE_NEG_REPLY:
					case BT_TASK_EVENT_HCI2LMP_PIN_CODE_REPLY:
					case BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_ON:
					case BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_OFF:
					case BT_TASK_EVENT_HCI2LMP_AUTH_REQUEST:
					case BT_TASK_EVENT_HCI2LMP_ADD_SCO:
					case BT_TASK_EVENT_HCI2LMP_REMOVE_SCO:
					case BT_TASK_EVENT_HCI2LMP_HODE_REQ:
					case BT_TASK_EVENT_HCI2LMP_SNIFF_REQ:
					case BT_TASK_EVENT_HCI2LMP_EXIT_SNIFF_REQ:
					case BT_TASK_EVENT_HCI2LMP_SET_AFH:
					case BT_TASK_EVENT_HCI2LMP_CHANGE_MAX_SLOT:
					case BT_TASK_EVENT_HCI2LMP_ROLE_SWITCH:
					case BT_TASK_EVENT_HCI2LMP_POWER_UP:
					case BT_TASK_EVENT_HCI2LMP_POWER_DOWN:
					case BT_TASK_EVENT_HCI2LMP_ADD_ESCO:
					case BT_TASK_EVENT_HCI2LMP_CHANGE_ESCO:
					case BT_TASK_EVENT_HCI2LMP_ACCEPT_ESCO_CONN:
					case BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_ESCO_CONN:
					case BT_TASK_EVENT_HCI2LMP_REMOVE_ESCO:
					case BT_TASK_EVENT_HCI2LMP_SET_CLASSIFICATION:
					case BT_TASK_EVENT_HCI2LMP_QOS_SETUP:
					case BT_TASK_EVENT_HCI2LMP_FLOW_SPECIFICATION:
					case BT_TASK_EVENT_HCI2LMP_CLOCK_READY:
					case BT_TASK_EVENT_HCI2LMP_CHANGE_SCO_PACKET_TYPE:
					case BT_TASK_EVENT_HCI2LMP_CHANGE_PACKET_TYPE:
					case BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE:
					{
						PCONNECT_DEVICE_T pTempConnectDevice;
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "enter hc2lm command process!\n");
						RtlCopyMemory(&pTempConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len);
						LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, eventnum);
					}
						break;

					case BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND:
					{
						PCONNECT_DEVICE_T pTempConnectDevice;
						if(devExt->AfhPduCount > 0)
						{
							devExt->AfhWriteCmdFailTimes++;
							if(devExt->AfhWriteCmdFailTimes >= 5)
							{
								LMP_Reset_AfhLmpCount(devExt);
								devExt->AfhWriteCmdFailTimes = 0;
							}
							BT_DBGEXT(ZONE_TASK | LEVEL1,  "LMP---some afh pdu are pending, create a delay task to write cmd indicator\n");
							Task_CreateDelayTask(pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND), 
								BT_TASK_PRI_NORMAL, ptmpdatablock->data, ptmpdatablock->data_len);
						}
						else
						{
							BT_DBGEXT(ZONE_TASK | LEVEL3,  "LMP---write afh command indicator\n");
							RtlCopyMemory(&pTempConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len);
							LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND);	
						}
					}
						break;
						
					case BT_TASK_EVENT_HCI2LMP_WRITE_AFH_REGISTERS:
						LMP_WriteReg_AfterSendingLMP(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "HCI2LMP: No event num match cases\n");
					}
					break;
				case BT_TASK_EVENT_TYPE_DSP2LMP:
					switch (eventnum)
					{
					case BT_TASK_EVENT_DSP2LMP_TEST:
						break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "DSP2LMP: No event num match cases\n");
					}
					break;
				case BT_TASK_EVENT_TYPE_HCI2HC:
					switch (eventnum)
					{
					case BT_TASK_EVENT_HCI2HC_SEND_NUM_OF_COMP_PACKET_EVENT:
						Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_CONNECTION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_DISCONNECTION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_CLKOFFSET_COMPLETE_EVENT:
						Task_HCI2HC_Send_ClkOffset_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMEREQ_COMPLETE_EVENT:
						Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMESUPPFEA_COMPLETE_EVENT:
						Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_REMOTE_VERINFO_COMPLETE_EVENT:
						Task_HCI2HC_Send_RemoteVerInfo_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_CONN_REQ_EVENT:
						Task_HCI2HC_Send_Conn_Req_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_REQ_EVENT:
						Task_HCI2HC_Send_Link_Key_Req_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_NOTIFICATION_EVENT:
						Task_HCI2HC_Send_Link_Key_Notification_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_PIN_CODE_REQ_EVENT:
						Task_HCI2HC_Send_Pin_Code_Req_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_ENCRYPTION_CHANGE_EVENT:
						Task_HCI2HC_Send_Encryption_Change_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_AUTHENTICATION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Authentication_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_CONN_PACKET_TYPE_CHANGED_EVENT:
						Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_SCO_CONNECTION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Sco_Connection_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_SCO_DISCONNECTION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_MODE_CHANGE_EVENT:
						Task_HCI2HC_Send_Mode_Change_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_ROLE_CHANGE_EVENT:
						Task_HCI2HC_Send_Role_Change_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Sync_Connection_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_CHANGED_EVENT:
						Task_HCI2HC_Send_Sync_Connection_Changed_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_QOS_SETUP_COMPLETE_EVENT:
						Task_HCI2HC_Send_Qos_Setup_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_FLOW_SPECIFICATION_COMPLETE_EVENT:
						Task_HCI2HC_Send_Flow_Specification_Complete_Event(devExt, ptmpdatablock->data, 1);
						break;

					case BT_TASK_EVENT_HCI2HC_SEND_SCO_CONN_PACKET_TYPE_CHANGED_EVENT:
						Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_MAX_SLOT_CHANGED_EVENT:
						Task_HCI2HC_Send_Max_Slot_Changed_Event(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_HCI2HC_SEND_HARDWARE_ERROR_EVENT:
						Task_HCI2HC_Send_Hardware_Error_Event(devExt, ptmpdatablock->data[0], 1);
						break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "HCI2HC: No event num match cases\n");
					}
					break;
				case BT_TASK_EVENT_TYPE_DSP2HC:
					switch (eventnum)
					{
					case BT_DSP_INT_INQUIRY_RESULT_EVENT:
						break;
					case BT_DSP_INT_REPORT_RX_POWER:
						BtProcessReportRxPowerInt(devExt);
						break;
					case BT_TASK_EVENT_DSP2HC_TEST:
						break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "DSP2HC: No event num match cases\n");
					}
					break;
				case BT_TASK_EVENT_TYPE_HC2HCI:
					switch (eventnum)
					{
					case BT_TASK_EVENT_HC2HCI_RESET:
						Task_HC2HCI_Reset(devExt);
						break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "HC2HCI: No event num match cases\n");
					}
					break;
				case BT_TASK_EVENT_TYPE_NORMAL:
					switch (eventnum)
					{
					case BT_TASK_EVENT_NORMAL_SEND_EVENT:
						Task_Normal_Send_Event(devExt, ptmpdatablock->data[0], 1);
						break;
					case BT_TASK_EVENT_NORMAL_ADD_SCO:
						Task_Normal_Add_Sco(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_NORMAL_SEND_CONN_REQ:
						Task_Normal_Send_Conn_Req(devExt, ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_NORMAL_RES_DISCARD_COMMAND:
						Task_Normal_Res_Discard_Command(devExt, *(PUINT16)ptmpdatablock->data, 1);
						break;
					case BT_TASK_EVENT_NORMAL_REL_RESOURCE_FOR_REMOTE_REQ:
						Task_Normal_Rel_Resouce_For_Remote_Req(devExt, ptmpdatablock->data);
						break;
					case BT_TASK_EVENT_CHECK_HCI_LMP_TIMEOUT:
						Hci_Timeout(devExt);
						LMP_PDU_Timeout(devExt);
						break;
					case BT_TASK_EVENT_RX_INT_PROCESS:
						BtTaskProcessRx(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
						devExt->RxPacketNum--;
						tmpLong = devExt->RxPacketNum;
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task1, RxPacketNum is %d\n", devExt->RxPacketNum);
						KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);						
						if(tmpLong == 0)
							BtCheckAndChangeRxState(devExt);
						Task_RxPool_Flag = 1;
						break;
					case BT_TASK_EVENT_RX_POLL_FRAME_PROCESS:
						BtProcessRxPoolFrame(devExt, ptmpdatablock->data);
						KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
						devExt->RxPacketNum--;
						tmpLong = devExt->RxPacketNum;
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "task2, RxPacketNum is %d\n", devExt->RxPacketNum);
						KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
						if(tmpLong == 0)
							BtCheckAndChangeRxState(devExt);
						break;
					case BT_TASK_EVENT_RX_CRCERROR_FRAME_PROCESS:
						break;
					case BT_TASK_EVENT_RX_AFH_CH_BAD_FRAME_PROCESS:
						BtProcessRxAFHChBadFrame(devExt, *(PUINT32)(ptmpdatablock->data));
						break;
					case BT_TASK_EVENT_RX_LMP_PDU_PROCESS:
						BtProcessRxLMPPDU(devExt, ptmpdatablock->data[0]);
						break;
					case BT_TASK_EVENT_NORMAL_ENTER_SNIFF:
						LMP_Task_EnterSniff(devExt, ptmpdatablock->data);
						break;
					case BT_TASK_EVENT_NORMAL_LEAVE_SNIFF:
						LMP_Task_LeaveSniff(devExt, ptmpdatablock->data);
						break;
					case BT_DSP_INT_ENTER_SNIFF:
						BtProcessModeChangeInt(devExt, ptmpdatablock->data[0]);
						break;
					case BT_TASK_EVENT_NORMAL_EDR_MODE_CHANGE:
						LMP_Task_EdrModeChange(devExt, ptmpdatablock->data);
						break;
					case BT_TASK_EVENT_ROLE_CHANGE_INT:
						BtProcessRoleChangeInt(devExt);
						break;
					case BT_TASK_EVENT_ROLE_CHANGE_FAIL_INT:
						BtProcessRoleChangeFailInt(devExt);
						break;
					case BT_TASK_EVENT_START_STAGE_MAINLOOP:
						if(devExt->StartStage == START_STAGE_INIT)
							BtStartDevice_2Phase(devExt, ptmpdatablock->data[0]);
						break;
					case BT_TASK_EVENT_POWER_STATE_D0:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task set power d0 entered\n");
						BtPower_SetPowerD0(devExt, ptmpdatablock->data[0], ptmpdatablock->data[1], ptmpdatablock->data[2]);
						break;
					case BT_TASK_EVENT_POWER_STATE_D3:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task set power d3 entered\n");
						BtPower_SetPowerD3(devExt, ptmpdatablock->data[0], ptmpdatablock->data[1], ptmpdatablock->data[2]);
						break;
					case BT_TASK_EVENT_REQUEST_8051_MAINLOOP:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task request 8051 enter main loop\n");
						VendorCmdWriteCmdToMailBox(devExt, NULL, VCMD_API_8051_JUMP_MAIN_PROCESS);
						break;
					case BT_TASK_EVENT_RX_FHS_FRAME_PROCESS:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task fhs frame process\n");
						BtTaskProcessRxPoolFrame(devExt);
						KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
						devExt->RxPacketNum--;
						tmpLong = devExt->RxPacketNum;
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "task2, RxPacketNum is %d\n", devExt->RxPacketNum);
						KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);
						if(tmpLong == 0)
							BtCheckAndChangeRxState(devExt);
						Task_RxPool_Flag = 1;
						break;
					case BT_TASK_EVENT_STOP_AND_GO:
						BtPower_SetPowerD3(devExt, 0, 0, 0);
						BtPower_SetPowerD0(devExt, 1, 1, 0);
						break;
					case BT_TASK_EVENT_INQUIRY_RESULT_PROCESS:
						BtTaskProcessInquiryResult(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_RESUBMIT_READ_IRP:
						if(TRUE == BtIsPluggedIn(devExt))
						{
							UsbResetPipe(devExt, PIPE_NUMBER_READ_PIPE);
							BtUsbSubmitReadIrp(devExt);	
						}
						else
						{
							BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_ThreadRoutin---device removed, don't resubmit read irp\n");
						}
						break;

					case BT_TASK_EVENT_RESUBMIT_INQUIRY_IRP:
						if(TRUE == BtIsPluggedIn(devExt))
						{
							UsbResetPipe(devExt, PIPE_NUMBER_INQUIRY_PIPE);
							BtUsbSubmitIquiryIrp(devExt);	
						}
						else
						{
							BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_ThreadRoutin---device removed, don't resubmit inquiry irp\n");
						}
						break;
					case BT_TASK_EVENT_WRITE_TEST_CMD_INDICATOR:
						#ifdef BT_TESTMODE_SUPPORT
						if(devExt->AllowCmdIndicator == TRUE)
						{
							TestMode_WriteCommandIndicator(devExt);
						}
						else
						{
							BT_DBGEXT(ZONE_TASK | LEVEL3,  "Testmode lmp pdu hasn't sent to dsp, delay a while to write cmd indicator\n");
							Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_WRITE_TEST_CMD_INDICATOR), BT_TASK_PRI_NORMAL, NULL, 0);
						}

						#endif
							
						break;

		                    case BT_TASK_EVENT_POLL_TIMER:
		                        BT_DBGEXT(ZONE_TASK | LEVEL1, "Poll timer is checked!!");
		                        /* For Poll timer management */
		                		if (((PBT_HCI_T)devExt->pHci)->start_poll_flag == 1)
		                		{
		                			
		                			Hci_PollDevStatus(devExt);
		                		}
		                        break;

					case BT_TASK_EVENT_PROCESS_FLUSH:
						BT_DBGEXT(ZONE_TASK | LEVEL3, "process flush task\n");
						Flush_ProcessFlushInt(devExt, ptmpdatablock->data);
						break;

					case BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_SEND:
						BT_DBGEXT(ZONE_TASK | LEVEL3, "process role switch sending task\n");
						LMP_Task_RoleChange_Send(devExt, ptmpdatablock->data);
						break;
						
					case BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_RECV:
						BT_DBGEXT(ZONE_TASK | LEVEL3,  "process role switch sending task\n");
						LMP_Task_RoleChange_Recv(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;

					case BT_TASK_EVENT_CHECK_SCO_SNIFF:
						LMP_Unsniff_SCO(devExt);
						break;
					case BT_TASK_EVENT_SEND_SCO_LINK_REQ_PDU:
						{
							PCONNECT_DEVICE_T pConnectDevice;
							LMP_PUD_PACKAGE_T pdu;
							BT_DBGEXT(ZONE_TASK | LEVEL3,  "process BT_TASK_EVENT_SEND_SCO_LINK_REQ_PDU!\n");
							
							RtlCopyMemory(&pConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len);
							RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
							pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;

							if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
								Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_SEND_SCO_LINK_REQ_PDU), BT_TASK_PRI_NORMAL, (PUINT8)(&pConnectDevice), sizeof(PCONNECT_DEVICE_T));
							else
								Send_SCOLinkReq_PDU(devExt, pConnectDevice, pdu.TransID);
						}
						break;		
					}
					break;
				case BT_TASK_EVENT_TYPE_LMP2HCI:
					switch (eventnum)
					{
					case BT_TASK_EVENT_LMP2HCI_SETUP_COMPLETE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SETUP_COMPLETE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_DETACH:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_CLKOFFSET_RES:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CLKOFFSET_RES, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_NAME_RES:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_NAME_RES, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_FEATURE_RES:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_FEATURE_RES, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_VERSION_RES:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_VERSION_RES, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_HOST_CONN_REQ:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_HOST_CONN_REQ, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_LMP_TIMEOUT:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_LINK_SUPERVISION_TIMEOUT:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_SUPERVISION_TIMEOUT, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_LINK_KEY_REQ:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_REQ, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_AUTH_FAILURE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_FAILURE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_LINK_KEY_NOTIFICATION:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_NOTIFICATION, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_PIN_CODE_REQ:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PIN_CODE_REQ, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_PAIR_NOT_ALLOW:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PAIR_NOT_ALLOW, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ENCRYPTION_SUCCESS:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ENCRYPTION_FAILURE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ENCRYPTION_NOT_COMP:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_NOT_COMP, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_AUTH_COMP_SUCCESS:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_SUCCESS, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_AUTH_COMP_FAILURE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_SCO_LINK_REQ:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_LINK_REQ, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_SCO_LMP_TIMEOUT:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_LMP_TIMEOUT, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_SCO_DETACH:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_SCO_CONNECTED:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_CONNECTED, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_SCO_REMOVED:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_MODE_CHANGE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ROLE_SWITCH_FAIL:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ESCO_CONNECTED:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CONNECTED, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ESCO_CHANGED:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGED, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ESCO_CHANGE_FAIL:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ESCO_REMOVED:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_ESCO_LINK_REQ:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_LINK_REQ, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_CHANGE_TX_MAX_SLOT:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_TX_MAX_SLOT, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_QOS_SETUP_COMPLETE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_QOS_SETUP_COMPLETE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_FLOW_SPECIFICATION_COMPLETE:
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_CHANGE_SCO_PACKET_TYPE_COMPLETE:
						Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_LMP2HCI_CHANGE_PACKET_TYPE_COMPLETE:
						Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE, ptmpdatablock->data, (UINT16)ptmpdatablock->data_len);
						break;
					}
					break;
#ifdef BT_LOOPBACK_SUPPORT
				case BT_TASK_EVENT_TYPE_HCI2LOOPBACK:
					switch (eventnum)
					{
					case BT_TASK_EVENT_HCI2LOOPBACK_RESPONCE_CONN_HANDLES_FOR_LOCAL_LOOPBACK:
						Loopback_Responce_Conn_Handles_For_Local_Loopback(devExt);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_SEND_CONN_COMPLETE_EVENT:
						Loopback_Send_Connection_Complete_Event(devExt, *(PUINT16)ptmpdatablock->data, ptmpdatablock->data[2]);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_EXIT_LOCAL_LOOPBACK:
						Loopback_Exit_Local_Loopback(devExt);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_SEND_DISCONN_COMPLETE_EVENT:
						Loopback_Send_Disconnection_Complete_Event(devExt, *(PUINT16)ptmpdatablock->data);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_SEND_COMMAND_TO_HC:
						Loopback_Send_Command_To_HC(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_SEND_ACL_DATA_TO_HC:
						Loopback_Send_Acl_Data_To_HC(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					case BT_TASK_EVENT_HCI2LOOPBACK_SEND_SCO_DATA_TO_HC:
						Loopback_Send_Sco_Data_To_HC(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					}
					break;
#endif
#ifdef BT_TESTMODE_SUPPORT
				case BT_TASK_EVENT_TYPE_HCI2TESTMODE:
					switch (eventnum)
					{
					case BT_TASK_EVENT_HCI2TESTMODE_START_TESTER_DATA:
						TestMode_Start_Tester_Data(devExt, ptmpdatablock->data, ptmpdatablock->data_len);
						break;
					}
					break;
#endif
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "No event type match cases\n");
			}
			/* Lock */
			KeAcquireSpinLock(&pTask->lock, &oldIrql);
			{
                // Dynamic allocated task block, free it here
                if(ptmpdatablock->tmpFlag)
                {
                    FreeTaskBlock(ptmpdatablock);
                }
                else
                {
    				// This block will be in free list. So set this value as 1.
    				ptmpdatablock->in_list_flag = 1;
    				if(Task_RxPool_Flag)
    				{
    					QueuePutTail(&pTask->Task_RxFree_Pool, &ptmpdatablock->Link);						
    				}
    				else
    				{
    					QueuePutTail(&pTask->task_free_pool, &ptmpdatablock->Link);	
    				}
                }
			}
			/* Unlock */
			KeReleaseSpinLock(&pTask->lock, oldIrql);
		}
	}
}



/**************************************************************************
 *   Task_Init
 *
 *   Descriptions:
 *      Initialize task module, including alloc memory for the task module and
 *      link task block into the task pool and so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Task module memory is allocated with succesfully
 *      STATUS_UNSUCCESSFUL. Task module   memory is allocated with fail
 *************************************************************************/
NTSTATUS Task_Init(PBT_DEVICE_EXT devExt)
{
	PBT_TASK_T pTask;
	PTASK_DATA_BLOCK_T pTaskBlock;
	UINT32 i;
	UINT32 listcount;

	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Init devExt = 0x%x\n", devExt);
	/* Alloc memory for task module */
	pTask = (PBT_TASK_T)ExAllocatePool(sizeof(BT_TASK_T), GFP_KERNEL);
	if (pTask == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Allocate task memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}

	/* Save task module pointer into device extention context */
	devExt->pTask = (PVOID)pTask;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Init Task = 0x%x, size = %d\n", pTask, sizeof(BT_TASK_T));
	/* Zero out the task module space */
	RtlZeroMemory(pTask, sizeof(BT_TASK_T));
	
	/* Init list */
	pTask->pDevExt = devExt;
	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->Task_RxFree_Pool);
	QueueInitList(&pTask->delay_task_list);
	/* Alloc spin lock,which used to protect task link operator */
	KeInitializeSpinLock(&pTask->lock);

	/* Alloc memory for task block */
	pTaskBlock = (PTASK_DATA_BLOCK_T)ExAllocatePool(MAXTASKN *sizeof(TASK_DATA_BLOCK_T), GFP_KERNEL);
	if (pTaskBlock == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "TASK BLOCK Allocate Memory failed \n");
		devExt->pTask = NULL;
		ExFreePool((PVOID)pTask);
		return STATUS_UNSUCCESSFUL;
	}

    pTask->queue = devExt->pBtWorkqueue;
	
	/*Save task block pointer into task module */
	pTask->ptask_block = pTaskBlock;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Init TaskBlock = 0x%x, size = %d\n", pTaskBlock, MAXTASKN *sizeof(TASK_DATA_BLOCK_T));
	/* Insert all task blocks into the task free pool. */
	for (i = 0; i < MAXTASKN/2; i++)
	{
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock[i].Link);
		pTaskBlock[i].in_list_flag = 1;
		pTaskBlock[i].tmpFlag = 0;

		pTaskBlock[i].devExt = (PVOID)devExt;
		init_timer(&pTaskBlock[i].dwork.timer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)		
		INIT_WORK(&pTaskBlock[i].dwork.work, Task_ThreadRoutine);
#else		
		INIT_WORK(&pTaskBlock[i].dwork, Task_ThreadRoutine, &pTaskBlock[i]);
#endif
		
	}
	for (i = MAXTASKN/2; i < MAXTASKN; i++)
	{
		QueuePutTail(&pTask->Task_RxFree_Pool, &pTaskBlock[i].Link);
		pTaskBlock[i].in_list_flag = 1;
		pTaskBlock[i].tmpFlag = 0;

		pTaskBlock[i].devExt = (PVOID)devExt;
		init_timer(&pTaskBlock[i].dwork.timer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)		
		INIT_WORK(&pTaskBlock[i].dwork.work, Task_ThreadRoutine);
#else		
		INIT_WORK(&pTaskBlock[i].dwork, Task_ThreadRoutine, &pTaskBlock[i]);
#endif
	}

	/*Test if it is the correct counts in the list  */
	QueueGetCount(&pTask->task_free_pool, listcount);
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "task free pool list count = %lu\n", listcount);
	QueueGetCount(&pTask->task_list, listcount);
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "task list count = %lu\n", listcount);
	QueueGetCount(&pTask->delay_task_list, listcount);
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "delay task list count = %lu\n", listcount);
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Task_Release
 *
 *   Descriptions:
 *      Release task module, including free memory for the task module and
 *      for the task block.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Task module memory is released with succesfully
 *      STATUS_UNSUCCESSFUL. Task module  memory is released with fail
 *************************************************************************/
NTSTATUS Task_Release(PBT_DEVICE_EXT devExt)
{
	PBT_TASK_T pTask;
	PTASK_DATA_BLOCK_T pTaskBlock;

	ENTER_FUNC();
	/*Get pointer of the task module and task block in the task module */
	pTask = (PBT_TASK_T)devExt->pTask;
	if (pTask == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	pTaskBlock = pTask->ptask_block;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Release devExt = 0x%x, pTask = 0x%x, pTaskBlock = 0x%x\n", devExt, pTask, pTaskBlock);


	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->delay_task_list);

    
	/*Free the task block memory */
	if (pTaskBlock != NULL)
	{
		ExFreePool((PVOID)pTaskBlock);
	}
	pTask->ptask_block = NULL;
	/*Free the task module memory */
	if (pTask != NULL)
	{
		ExFreePool((PVOID)pTask);
	}
	devExt->pTask = NULL;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Release End\n");
	EXIT_FUNC();
	return (STATUS_SUCCESS);
}

NTSTATUS Task_Stop(PBT_TASK_T pTask)
{
	UINT32 i;
	PTASK_DATA_BLOCK_T pTaskBlock;

#if 0
	if(pTask->queue)
	{
	    BT_DBGEXT(ZONE_TASK | LEVEL3, "Should never here !!Release the task work queue\n");
	    flush_workqueue(pTask->queue);
		destroy_workqueue(pTask->queue);
		pTask->queue = NULL;
	}
#endif

	ENTER_FUNC();
	/*Get pointer of the task module and task block in the task module */
	pTaskBlock = pTask->ptask_block;

	for (i = 0; i < MAXTASKN; i++)
	{
        cancel_delayed_work(&pTaskBlock[i].dwork);
	}

	EXIT_FUNC();
	return STATUS_SUCCESS;
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
VOID Task_Reset(PBT_TASK_T pTask)
{
	UINT32 i;
	PTASK_DATA_BLOCK_T pTaskBlock;
	KIRQL oldIrql;

	return;

	if(pTask == NULL)
		return;
	
	pTaskBlock = pTask->ptask_block;
	
	
	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);
	/* Init list */
	QueueInitList(&pTask->task_free_pool);
	QueueInitList(&pTask->Task_RxFree_Pool);
	QueueInitList(&pTask->task_list);
	QueueInitList(&pTask->delay_task_list);
	/*Link all task block again */
	for (i = 0; i < MAXTASKN/2; i++)
	{
		QueuePutTail(&pTask->task_free_pool, &pTaskBlock[i].Link);
		pTaskBlock[i].in_list_flag = 1;
	}
	/* Unlock */
	KeReleaseSpinLock(&pTask->lock, oldIrql);

	
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
 *      STATUS_UNSUCCESSFUL if this task is not created
 *************************************************************************/
NTSTATUS Task_CreateTask(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
    
	ENTER_FUNC();
	if (pTask == NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);
	/*Get a free task data block from the task free pool */
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_free_pool);
	if (ptmpdatablock == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateTask()--get task block from pool fails, alloc it directly, eventCode = 0x%lx\n", event);
		ptmpdatablock = CreateTaskBlock(pTask, Task_ThreadRoutine);
		if(NULL == ptmpdatablock){
			BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateTask()--create task fails!!!!, eventCode = 0x%lx\n", event);
			/* Unlock */
    		KeReleaseSpinLock(&pTask->lock, oldIrql);
			return (STATUS_UNSUCCESSFUL);
		}
	}
	/*Fill some paras */
	ptmpdatablock->task_event = event;
	ptmpdatablock->task_pri = pri;
	ptmpdatablock->data_len = TASK_MIN(len, MAXTASKDATAN);
	if (para)
	{
		RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->data_len);
	}
	ptmpdatablock->in_list_flag = 0;
	if (pri == BT_TASK_PRI_HIGH)
	{
	}
	else
	{
	}
	KeReleaseSpinLock(&pTask->lock, oldIrql);
	queue_delayed_work(pTask->queue, &ptmpdatablock->dwork, 0);
	EXIT_FUNC();
	return (STATUS_SUCCESS);
}




/**************************************************************************
*Description
*	Jakio20080301:Create task for rx data
 *************************************************************************/
NTSTATUS Task_CreateTaskForRxData(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	
	if (pTask == NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->Task_RxFree_Pool);
	if (ptmpdatablock == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateTaskForRxData()--get task block from pool fails, alloc it directly, eventCode = 0x%lx\n", event);
		ptmpdatablock = CreateTaskBlock(pTask, Task_ThreadRoutine);
		if(NULL == ptmpdatablock){
    		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateTaskForRxData()--create task fails!!!!, eventCode = 0x%lx\n", event);
    		/* Unlock */
    		KeReleaseSpinLock(&pTask->lock, oldIrql);
    		return (STATUS_UNSUCCESSFUL);
        }
	}
	/*Fill some paras */
	ptmpdatablock->task_event = event;
	ptmpdatablock->task_pri = pri;
	ptmpdatablock->data_len = TASK_MIN(len, MAXTASKDATAN);
	if (para)
	{
		RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->data_len);
	}
	ptmpdatablock->in_list_flag = 1;
	/* Insert the task data block into the task list */
	KeReleaseSpinLock(&pTask->lock, oldIrql);

	queue_delayed_work(pTask->queue, &ptmpdatablock->dwork, 0);
	
	return (STATUS_SUCCESS);
}



/**************************************************************************
 *   Task_CreateDelayTask
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
 *      STATUS_UNSUCCESSFUL if this task is not created
 *************************************************************************/
 NTSTATUS Task_CreateDelayTask(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para,UINT32 len)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	KIRQL   oldIrql;
	LARGE_INTEGER timevalue;

	if (pTask == NULL)
		return (STATUS_UNSUCCESSFUL);

	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);

	/*Get a free task data block from the task free pool */
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_free_pool);
	if (ptmpdatablock == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateDelayTask()--get task block from pool fails, alloc it directly, eventCode = 0x%lx\n", event);
		// Directly alloc
		ptmpdatablock = CreateTaskBlock(pTask, Task_ThreadRoutine);
		if(NULL == ptmpdatablock){
			BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_CreateDelayTask()--create task fails!!!!, eventCode = 0x%lx\n", event);
			/* Unlock */
    		KeReleaseSpinLock(&pTask->lock, oldIrql);
			return (STATUS_UNSUCCESSFUL);
		}
	}

	/*Fill some paras */
	ptmpdatablock->task_event = event;
	ptmpdatablock->task_pri = pri;
	ptmpdatablock->data_len = TASK_MIN(len, MAXTASKDATAN);
	if (para)
		RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->data_len);

	ptmpdatablock->in_list_flag = 1; // This block will be in task list. So set this value as 1.

	/* Unlock */
	KeReleaseSpinLock(&pTask->lock, oldIrql);

	// Delay 1 milliseconds and call timeout function
	timevalue.QuadPart = HZ/200;	//5ms
	//queue delay task
	queue_delayed_work(pTask->queue, &ptmpdatablock->dwork, timevalue.QuadPart);

	return (STATUS_SUCCESS);
	
}

/**************************************************************************
 *   Task_CreateTaskForce
 *
 *   Descriptions:
 *      Create a self-defined task and this task must be created successfully.
 *   Arguments:
 *      pTask: IN, pointer to the task moudle saved in Adapter Context.
 *      event: IN, the event value of task.
 *      pri: IN, the pri value of task.
 *      para: IN, parameter of task.
 *      len : IN, the parameter length  of task.
 *   Return Value:
 *      STATUS_SUCCESS if task is created successfully
 *      STATUS_UNSUCCESSFUL if this task is not created
 *************************************************************************/
NTSTATUS Task_CreateTaskForce(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	if (pTask == NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);
	/*Get a free task data block from the task free pool */
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_free_pool);
	if (ptmpdatablock == NULL)
	/* If there is no block in the free pool, a task block must be get from the task list*/
	{
		/*Get a free task data block from the task list (replace the valid task)*/
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueuePopHead(&pTask->task_list);
		if (ptmpdatablock == NULL)
		{
			/* Unlock */
			KeReleaseSpinLock(&pTask->lock, oldIrql);
			return (STATUS_UNSUCCESSFUL);
		}
		/*Fill some paras */
		ptmpdatablock->task_event = event;
		ptmpdatablock->task_pri = pri;
		ptmpdatablock->data_len = TASK_MIN(len, MAXTASKDATAN);
		/* Insert the task data block into the task list */
		if (para)
		{
			RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->data_len);
		}
		ptmpdatablock->in_list_flag = 1;
		if (pri == BT_TASK_PRI_HIGH)
		{
			QueuePushHead(&pTask->task_list, &ptmpdatablock->Link);
		}
		else
		{
			QueuePutTail(&pTask->task_list, &ptmpdatablock->Link);
		}
		
		KeReleaseSpinLock(&pTask->lock, oldIrql);
		queue_delayed_work(pTask->queue, &ptmpdatablock->dwork, 0);
		return (STATUS_SUCCESS);
	}
	/*Fill some paras */
	ptmpdatablock->task_event = event;
	ptmpdatablock->task_pri = pri;
	ptmpdatablock->data_len = TASK_MIN(len, MAXTASKDATAN);
	if (para)
	{
		RtlCopyMemory(ptmpdatablock->data, para, ptmpdatablock->data_len);
	}
	ptmpdatablock->in_list_flag = 1;
	/* Insert the task data block into the task list */
	if (pri == BT_TASK_PRI_HIGH)
	{
		QueuePushHead(&pTask->task_list, &ptmpdatablock->Link);
	}
	else
	{
		QueuePutTail(&pTask->task_list, &ptmpdatablock->Link);
	}
	KeReleaseSpinLock(&pTask->lock, oldIrql);
	queue_delayed_work(pTask->queue, &ptmpdatablock->dwork, 0);
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Task_CheckExistTask
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
BOOLEAN Task_CheckExistTask(PBT_TASK_T pTask, UINT32 event)
{
	PTASK_DATA_BLOCK_T ptmpdatablock;
	KIRQL oldIrql;
	/* Lock */
	KeAcquireSpinLock(&pTask->lock, &oldIrql);
	ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetHead(&pTask->task_list);
	while (ptmpdatablock)
	{
		if (ptmpdatablock->task_event == event)
		{
			/* Unlock */
			KeReleaseSpinLock(&pTask->lock, oldIrql);
			return TRUE;
		}
		ptmpdatablock = (PTASK_DATA_BLOCK_T)QueueGetNext(&ptmpdatablock->Link);
	}
	/* Unlock */
	KeReleaseSpinLock(&pTask->lock, oldIrql);
	return FALSE;
}




/**************************************************************************
 *   Task_HC2HCI_Reset
 *
 *   Descriptions:
 *      Do the real processing of HCI reset command.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HC2HCI_Reset(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT16 i;
	PCONNECT_DEVICE_T	pConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_RESET);


	Hci_Clear_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT);
	Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT);
	pHci->scan_enable = 2;

	Hci_ClearMainlyCommandIndicator(devExt);

	Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_DSPACK, 2);

	if (pHci->is_in_inquiry_flag)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL2,  "cancel inquiry \n");
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
		pHci->is_in_inquiry_flag = 0;
		BtDelay(10);
	}
	Hci_StopProtectInquiryTimer(pHci);

	BT_DBGEXT(ZONE_TASK | LEVEL3,  "clear inquiry result list \n");
	Hci_Clear_Inquiry_Result_List(pHci);
	BtClearQueues(devExt);
	BtInitHw(devExt);


	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		LMP_ReleaseSCOMembers(devExt,pConnectDevice);
		LMP_ResetMembers(devExt,pConnectDevice);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_am_list);
	}

	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		LMP_ReleaseSCOMembers(devExt,pConnectDevice);
		LMP_ResetMembers(devExt,pConnectDevice);		
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_slave_list);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	devExt->RxCachPid = devExt->RxCachCid = 0;
	devExt->RxSeqNo = 0;
	devExt->IsFirstRxFrame = TRUE;

	{
		PBT_FRAG_T pFrag = devExt->pFrag;
		if(pFrag)
		{
			for(i = 0; i < BT_TOTAL_SCO_LINK_COUNT; i++)
			{
				pFrag->RxScoElement[i].RxCachCid = pFrag->RxScoElement[i].RxCachPid;
				pFrag->RxScoElement[i].currentlen = 0;
				pFrag->RxScoElement[i].currentpos = 0;
				pFrag->RxScoElement[i].totalcount = 0;
			}	
		}		
	}



	QueueInitList(&pHci->device_free_list);
	QueueInitList(&pHci->device_am_list);
	QueueInitList(&pHci->device_pm_list);
	QueueInitList(&pHci->device_slave_list);
	QueueInitList(&pHci->sco_device_free_list);
	QueueInitList(&pHci->inquiry_result_free_list);
	QueueInitList(&pHci->inquiry_result_used_list);
	for (i = 0; i < BT_MAX_INQUIRY_RESULT_NUM; i++)
	{
		QueuePutTail(&pHci->inquiry_result_free_list, &pHci->inquiry_result_all[i].Link);
	}
	for (i = 0; i < BT_MAX_DEVICE_NUM; i++)
	{
		QueuePutTail(&pHci->device_free_list, &pHci->device_all[i].Link);
	}
	for (i = 0; i < BT_MAX_SCO_DEVICE_NUM; i++)
	{
		QueuePutTail(&pHci->sco_device_free_list, &pHci->sco_device_all[i].Link);
	}
	RtlZeroMemory(&pHci->addr_table, sizeof(ADDRESS_TABLE_T));
	pHci->num_inquiry_result_used = 0;
	pHci->num_device_am = 0;
	pHci->num_device_pm = 0;
	pHci->num_device_slave = 0;
	pHci->sco_link_count = 0;
	pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
	pHci->test_flag = 0;
	pHci->test_mode_active = 0;
	pHci->authentication_enable = BT_AUTHENTICATION_DISABLE;
	pHci->encryption_mode = BT_ENCRYPTION_DIABLE;
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_LOOPBACK_SUPPORT
	Loopback_Release_Resource(devExt);
#endif
#ifdef BT_SERIALIZE_DRIVER
#endif
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Write am connection indicator as 0\n");
#ifdef ACCESS_REGISTER_DIRECTLY
	pHci->page_timeout = 0x2000; // default time is 5.12 seconds
	Hci_Write_One_Word(devExt, BT_REG_PAGE_TIMEOUT, pHci->page_timeout);
	pHci->inquiry_mode = BT_INQUIRYMODE_STANDED;
	Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_CLR, 0);
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "disconnect sco and acl\n");
	Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
	BtDelay(10);
	Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT | BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT | BT_HCI_COMMAND_INDICATOR_RESET_BIT);
#else
	BtUsbTaskHc2HciReset(devExt);
#endif
	BtDelay(10);
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_HC2HCI_Reset exit\n");
    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}






/**************************************************************************
 *   Task_Reset_AfterFirmwareReset
 *
 *   Descriptions:
 *      Do some init after firmware reset, used in combo state.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Reset_BeforeFirmwareReset(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT16 i;
	PBT_FRAG_T	pFrag;
	
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Reset_BeforeFirmwareReset enter\n");
	pHci = (PBT_HCI_T)devExt->pHci;
	pFrag = (PBT_FRAG_T)devExt->pFrag;


	if((pHci == NULL) || (pFrag == NULL))
	{
		return;
	}
	/*
	sc_spin_lock_bh(&pHci->HciLock);
	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL3,  "Another command but prior command not completed. Ignore it!\n");
		sc_spin_unlock_bh(&pHci->HciLock);
		return ;
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_RESET);
	}
	sc_spin_unlock_bh(&pHci->HciLock);
	*/

	//pHci->scan_enable = 0;

	Hci_ClearMainlyCommandIndicator(devExt);


	if (pHci->is_in_inquiry_flag)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL3,  "cancel inquiry \n");
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
		pHci->is_in_inquiry_flag = 0;
		BtDelay(10);
	}
	Hci_StopProtectInquiryTimer(pHci);

	BT_DBGEXT(ZONE_TASK | LEVEL3,  "clear inquiry result list \n");
	Hci_Clear_Inquiry_Result_List(pHci);
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	devExt->RxCachPid = devExt->RxCachCid = 0;
	devExt->RxSeqNo = 0;
	devExt->IsFirstRxFrame = TRUE;

	for(i = 0; i < BT_TOTAL_SCO_LINK_COUNT; i++)
	{
		pFrag->RxScoElement[i].RxCachCid = pFrag->RxScoElement[i].RxCachPid;
		pFrag->RxScoElement[i].currentlen = 0;
		pFrag->RxScoElement[i].currentpos = 0;
		pFrag->RxScoElement[i].totalcount = 0;
	}

	
	QueueInitList(&pHci->device_free_list);
	QueueInitList(&pHci->device_am_list);
	QueueInitList(&pHci->device_pm_list);
	QueueInitList(&pHci->device_slave_list);
	QueueInitList(&pHci->sco_device_free_list);
	QueueInitList(&pHci->inquiry_result_free_list);
	QueueInitList(&pHci->inquiry_result_used_list);
	for (i = 0; i < BT_MAX_INQUIRY_RESULT_NUM; i++)
	{
		QueuePutTail(&pHci->inquiry_result_free_list, &pHci->inquiry_result_all[i].Link);
	}
	for (i = 0; i < BT_MAX_DEVICE_NUM; i++)
	{
		QueuePutTail(&pHci->device_free_list, &pHci->device_all[i].Link);
	}
	for (i = 0; i < BT_MAX_SCO_DEVICE_NUM; i++)
	{
		QueuePutTail(&pHci->sco_device_free_list, &pHci->sco_device_all[i].Link);
	}
	RtlZeroMemory(&pHci->addr_table, sizeof(ADDRESS_TABLE_T));
	pHci->num_inquiry_result_used = 0;
	pHci->num_device_am = 0;
	pHci->num_device_pm = 0;
	pHci->num_device_slave = 0;
	pHci->sco_link_count = 0;
	pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
	pHci->test_flag = 0;
	pHci->test_mode_active = 0;
	pHci->authentication_enable = BT_AUTHENTICATION_DISABLE;
	pHci->encryption_mode = BT_ENCRYPTION_DIABLE;

	/* jakio20080126: reset command state
*/
	pHci->command_state = BT_COMMAND_STATE_IDLE;
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
#ifdef BT_LOOPBACK_SUPPORT
	Loopback_Release_Resource(devExt);
#endif
#ifdef BT_SERIALIZE_DRIVER
#endif
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Write am connection indicator as 0\n");
#ifdef ACCESS_REGISTER_DIRECTLY
	/*
	pHci->page_timeout = 0x2000; // default time is 5.12 seconds
	Hci_Write_One_Word(devExt, BT_REG_PAGE_TIMEOUT, pHci->page_timeout);
	pHci->inquiry_mode = BT_INQUIRYMODE_STANDED;
	Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_CLR, 0);
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "disconnect sco and acl\n");
	Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
	BtDelay(10);
	*/
#else
	BtUsbTaskHc2HciReset(devExt);
#endif
	BtDelay(10);
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Reset_BeforeFirmwareReset exit\n");
}










/**************************************************************************
 *   Task_Normal_Send_Event
 *
 *   Descriptions:
 *      Send event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      eventcode: IN, event code
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Normal_Send_Event(PBT_DEVICE_EXT devExt, UINT8 eventcode, UINT8 urgent)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PUINT8 dest;
	PUINT16 pdatalen;
	UINT16 OutLen;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send event to HC, opcode = %x, eventcode = %x\n", pHci->current_opcode, eventcode);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if ((pHci->command_state == BT_COMMAND_STATE_IDLE) && ((eventcode == BT_HCI_EVENT_COMMAND_COMPLETE) || (eventcode == BT_HCI_EVENT_COMMAND_STATUS)))
	{
		BT_DBGEXT(ZONE_TASK | LEVEL1,  "no command send but response event. Ignore it!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return ;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_SEND_EVENT), BT_TASK_PRI_NORMAL,  &eventcode, 1);
		return ;
	}
	OutLen = 0; // temp value
	dest = devExt->RxCach[devExt->RxCachPid];
	pdatalen = (PUINT16)dest;
	*pdatalen = 0; // temp value
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = eventcode;
	pEventHead->total_len = 3; // temp value
	dest += sizeof(UINT16);
	switch (eventcode)
	{
		case (BT_HCI_EVENT_COMMAND_COMPLETE):
		*dest = 1;
		dest += sizeof(UINT8);
		*(PUINT16)dest = pHci->current_opcode;
		dest += sizeof(UINT16);
		switch (BT_GET_OGF(pHci->current_opcode))
		{
			case BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND: 
				switch (BT_GET_OCF(pHci->current_opcode))
				{
					case BT_HCI_COMMAND_RESET:
						Hci_Response_Reset(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_CONNECTION_ACCEPT_TIMEOUT:
						Hci_Response_Write_Conn_Accept_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_PAGE_TIMEOUT:
						Hci_Response_Write_Page_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_PIN_TYPE:
						Hci_Response_Write_Pin_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_CHANGE_LOCAL_NAME:
						Hci_Response_Change_Local_Name(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_LOCAL_NAME:
						Hci_Response_Read_Local_Name(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_CLASS_OF_DEVICE:
						Hci_Response_Write_Class_Of_Device(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_AUTHENTICATION_ENABLE:
						Hci_Response_Write_Authentication_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_CLASS_OF_DEVICE:
						Hci_Response_Read_Class_Of_Device(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_ENCRYPTION_MODE:
						Hci_Response_Write_Encryption_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_LINK_SUPERVISION_TIMEOUT:
						Hci_Response_Write_Link_Supervision_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_SCAN_ENABLE:
						Hci_Response_Write_Scan_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_PAGE_SCAN_ACTIVITY:
						Hci_Response_Write_Page_Scan_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_ACTIVITY:
						Hci_Response_Write_Inquiry_Scan_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_LINK_SUPERVISION_TIMEOUT:
						Hci_Response_Read_Link_Supervision_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_ENCRYPTION_MODE:
						Hci_Response_Read_Encryption_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_PIN_TYPE:
						Hci_Response_Read_Pin_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_CONNECTION_ACCEPT_TIMEOUT:
						Hci_Response_Read_Conn_Accept_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_PAGE_TIMEOUT:
						Hci_Response_Read_Page_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_SCAN_ENABLE:
						Hci_Response_Read_Scan_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_AUTHENTICATION_ENABLE:
						Hci_Response_Read_Authentication_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_PAGE_SCAN_ACTIVITY:
						Hci_Response_Read_Page_Scan_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_INQUIRY_SCAN_ACTIVITY:
						Hci_Response_Read_Inquiry_Scan_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_SCO_FLOW_CONTROL_ENABLE:
						Hci_Response_Write_Sco_Flow_Control_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_SCO_FLOW_CONTROL_ENABLE:
						Hci_Response_Read_Sco_Flow_Control_Enable(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_FLUSH:
						Hci_Response_Flush(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_AUTOMATIC_FLUSH_TIMEOUT:
						Hci_Response_Write_Automatic_Flush_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_AUTOMATIC_FLUSH_TIMEOUT:
						Hci_Response_Read_Automatic_Flush_Timeout(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_STORED_LINK_KEY:
						Hci_Response_Read_Stored_Link_Key(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_STORED_LINK_KEY:
						Hci_Response_Write_Stored_Link_Key(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_DELETE_STORED_LINK_KEY:
						Hci_Response_Delete_Stored_Link_Key(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_INQUIRY_MODE:
						Hci_Response_Write_Inquiry_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_INQUIRY_MODE:
						Hci_Response_Read_Inquiry_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_TYPE:
						Hci_Response_Write_Inquiry_Scan_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_INQUIRY_SCAN_TYPE:
						Hci_Response_Read_Inquiry_Scan_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_PAGE_SCAN_TYPE:
						Hci_Response_Write_Page_Scan_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_PAGE_SCAN_TYPE:
						Hci_Response_Read_Page_Scan_Type(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_AFH_CHANNEL_ASSESSMENT_MODE:
						Hci_Response_Write_AFH_Channel_Assessment_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_AFH_CHANNEL_ASSESSMENT_MODE:
						Hci_Response_Read_AFH_Channel_Assessment_Mode(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_HOLD_MODE_ACTIVITY:
						Hci_Response_Write_Hold_Mode_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_HOLD_MODE_ACTIVITY:
						Hci_Response_Read_Hold_Mode_Activity(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_SET_AFH_HOST_CHANNEL_CLASSIFICATION:
						Hci_Response_Set_AFH_Host_Channel_Classification(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_WRITE_CURRENT_IAC_LAP:
						Hci_Response_Write_Current_Iac_Lap(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_PAGE_SCAN_MODE:
						Hci_Response_Read_Page_Scan_Mode(devExt, dest, &OutLen);
						break;
                    case BT_HCI_COMMAND_READ_VOICE_SETTING:
                        Hci_Response_Read_Voice_Setting(devExt, dest, &OutLen);
                        break;
                    case BT_HCI_COMMAND_WRITE_VOICE_SETTING:
                        Hci_Response_Write_Voice_Setting(devExt, dest, &OutLen);
                        break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF HOST_CONTROLLER_BASEBAND\n");
						Hci_Response_Unknown_Command(devExt, dest, &OutLen);
					}
				break;
				case BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS: 
					switch (BT_GET_OCF(pHci->current_opcode))
					{
					case BT_HCI_COMMAND_READ_BUFFER_SIZE:
						Hci_Response_Read_Buffer_Size(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_LOCAL_VERSION_INFORMATION:
						Hci_Response_Read_Local_Version_Info(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_BD_ADDR:
						Hci_Response_Read_BD_Addr(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_FEATURES:
						Hci_Response_Read_Local_Supported_Features(devExt, dest, &OutLen);
						break;
					case BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_COMMANDS:
						Hci_Response_Read_Local_Supported_Commands(devExt, dest, &OutLen);
						break;
        			case (BT_HCI_COMMAND_READ_LOCAL_EXTENDED_FEATURES):
        				Hci_Response_Read_Local_Extended_Features(devExt, dest, &OutLen);
                        break;
					default:
						BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF INFORMATION_PARAMETERS\n");
						Hci_Response_Unknown_Command(devExt, dest, &OutLen);
					}
				break;
			case BT_HCI_COMMAND_OGF_LINK_POLICY:
				switch (BT_GET_OCF(pHci->current_opcode))
				{
				case BT_HCI_COMMAND_WRITE_LINK_POLICY_SETTINGS:
					Hci_Response_Write_Link_Policy_Settings(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_ROLE_DISCOVERY:
					Hci_Response_Role_Discovery(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_READ_LINK_POLICY_SETTINGS:
					Hci_Response_Read_Link_Policy_Settings(devExt, dest, &OutLen);
					break;
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF LINK_POLICY\n");
					Hci_Response_Unknown_Command(devExt, dest, &OutLen);
				}
				break;
			case BT_HCI_COMMAND_OGF_LINK_CONTROL:
				switch (BT_GET_OCF(pHci->current_opcode))
				{
				case BT_HCI_COMMAND_INQUIRY_CANCEL:
					Hci_Response_Inquiry_Cancel(devExt, dest, &OutLen);
					break;
                case BT_HCI_COMMAND_EXIT_PERIODIC_INQUIRY_MODE:
                    Hci_Response_Exit_Periodic_Inquiry_Mode(devExt, dest, &OutLen);
                    break;
                case BT_HCI_COMMAND_PERIODIC_INQUIRY_MODE:
                    Hci_Response_Periodic_Inquiry_Mode(devExt, dest, &OutLen);
                    break;
				case BT_HCI_COMMAND_LINK_KEY_REQUEST_REPLY:
					Hci_Response_Link_Keq_Request_Reply(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_LINK_KEY_REQUEST_NEGATIVE_REPLY:
					Hci_Response_Link_Keq_Request_Negative_Reply(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_PIN_CODE_REQUEST_NEGATIVE_REPLY:
					Hci_Response_Pin_Code_Request_Negative_Reply(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_PIN_CODE_REQUEST_REPLY:
					Hci_Response_Pin_Code_Request_Reply(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_REMOTE_NAME_REQUEST_CANCEL:
					Hci_Response_Remote_Name_Request_Cancel(devExt, dest, &OutLen);
					break;
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF LINK_CONTROL\n");
					Hci_Response_Unknown_Command(devExt, dest, &OutLen);
				}
				break;
			case BT_HCI_COMMAND_OGF_STATUS_PARAMETERS:
				switch (BT_GET_OCF(pHci->current_opcode))
				{
				case BT_HCI_COMMAND_READ_RSSI:
					Hci_Response_Read_Rssi(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_READ_AFH_CHANNEL_MAP:
					Hci_Response_Read_AFH_Channel_Map(devExt, dest, &OutLen);
					break;
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF STATUS_PARAMETERS\n");
					Hci_Response_Unknown_Command(devExt, dest, &OutLen);
				}
				break;
			case BT_HCI_COMMAND_OGF_VENDOR_SPECIFIC:
				switch (BT_GET_OCF(pHci->current_opcode))
				{
				case BT_HCI_COMMAND_READ_ENCRYPTION_KEY_FROM_IVT:
					Hci_Response_Read_IVT_Encryption_Key(devExt, dest, &OutLen);
					break;
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF VENDOR_SPECIFIC\n");
					Hci_Response_Unknown_Command(devExt, dest, &OutLen);
				}
				break;
			case BT_HCI_COMMAND_OGF_TESTING:
				switch (BT_GET_OCF(pHci->current_opcode))
				{
				case BT_HCI_COMMAND_READ_LOOPBACK_MODE:
					Hci_Response_Read_Loopback_Mode(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_WRITE_LOOPBACK_MODE:
					Hci_Response_Write_Loopback_Mode(devExt, dest, &OutLen);
					break;
				case BT_HCI_COMMAND_ENABLE_DEVICE_UNDER_TEST_MODE:
					Hci_Response_Enable_Device_Under_Test_Mode(devExt, dest, &OutLen);
					break;
				default:
					BT_DBGEXT(ZONE_TASK | LEVEL1,  "OCF does not support for OGF TESTING\n");
					Hci_Response_Unknown_Command(devExt, dest, &OutLen);
				}
				break;
			default:
				BT_DBGEXT(ZONE_TASK | LEVEL1,  "OGF does not support\n");
				Hci_Response_Unknown_Command(devExt, dest, &OutLen);
			}
			pEventHead->total_len = (UINT8)(OutLen + 3);
			*pdatalen = OutLen + 5;
			break;
		case BT_HCI_EVENT_COMMAND_STATUS:
			*dest = pHci->command_status;
			dest += sizeof(UINT8);
			*dest = 1;
			dest += sizeof(UINT8);
			*(PUINT16)dest = pHci->current_opcode;
			dest += sizeof(UINT16);
			pEventHead->total_len = 4;
			*pdatalen = 6;
			break;
		case BT_HCI_EVENT_INQUIRY_COMPLETE:
			Hci_Response_Inquiry_Complete(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_INQUIRY_RESULT:
			Hci_Response_Inquiry_Result(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
			Hci_Response_Inquiry_Result_With_Rssi(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_CONNECTION_COMPLETE:
			Hci_Response_Connection_Complete(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_DISCONNECTION_COMPLETE:
			Hci_Response_Disconnect_Complete(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE:
			Hci_Response_Read_Clock_Offset(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
			Hci_Response_Remote_Name_Request(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		case BT_HCI_EVENT_CONNECTION_REQUEST: 
			Hci_Event_Connection_Request(devExt, dest, &OutLen);
			pEventHead->total_len = (UINT8)OutLen;
			*pdatalen = OutLen + 2;
			break;
		default:
			BT_DBGEXT(ZONE_TASK | LEVEL1,  "this event type can not be supported to send\n");
	}
 	//
  	// Move the rx cach pid to next id.
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);

 	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	if ((pHci->command_state != BT_COMMAND_STATE_IDLE) && ((eventcode == BT_HCI_EVENT_COMMAND_COMPLETE) || (eventcode == BT_HCI_EVENT_COMMAND_STATUS)))
		pHci->command_state = BT_COMMAND_STATE_IDLE;
	BT_DBGEXT(ZONE_TASK | LEVEL3, "Schedule the tasklet\n");
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

/**************************************************************************
 *   Task_HCI2HC_Send_Num_Of_Comp_Packet_Event
 *
 *   Descriptions:
 *      Send Number_Of_Completed_Packets event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCOMPLETE_PACKETS_T pCompletePackets;
	UINT16 i;
	
	pCompletePackets = (PCOMPLETE_PACKETS_T)para;
	if (pCompletePackets->number_of_handles > BT_TOTAL_NUM_DATA_PACKET)
	{
		pCompletePackets->number_of_handles = BT_TOTAL_NUM_DATA_PACKET;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send_Num_Of_Comp_Packet_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		if (!Task_CheckExistTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_NUM_OF_COMP_PACKET_EVENT)))
		{
			Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_NUM_OF_COMP_PACKET_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(COMPLETE_PACKETS_T));
		}
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 1+(pCompletePackets->number_of_handles *4) + 2;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_NUMBER_OF_COMPLETED_PACKET;
	pEventHead->total_len = 1+(pCompletePackets->number_of_handles *4);
	dest += sizeof(UINT16);
	*dest = pCompletePackets->number_of_handles;
	dest += sizeof(UINT8);
	for (i = 0; i < pCompletePackets->number_of_handles; i++)
	{
		*(PUINT16)dest = pCompletePackets->connection_handle[i];
		dest += sizeof(UINT16);

		*(PUINT16)dest = pCompletePackets->num_of_complete_packets[i];
		dest += sizeof(UINT16);
	}
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Connection_Complete_Event
 *
 *   Descriptions:
 *      Send Connection Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Connection_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_CONNECTION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 13;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_CONNECTION_COMPLETE;
	pEventHead->total_len = 11;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	*dest = pTempConnectDevice->link_type;
	dest += sizeof(UINT8);
	*dest = pTempConnectDevice->encryption_mode;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Disconnection_Complete_Event
 *
 *   Descriptions:
 *      Send Disconnection Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Disconnection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    ENTER_FUNC();    
    /* BlueZ ONLY */
    /* BlueZ ONLY, send the command status in the moment of Encryption success */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_DISCONNECT))){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_DISCONNECTION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 6;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_DISCONNECTION_COMPLETE;
	pEventHead->total_len = 4;
	dest += sizeof(UINT16);
	*dest = BT_HCI_STATUS_SUCCESS;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->current_reason_code;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

    EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_ClkOffset_Complete_Event
 *
 *   Descriptions:
 *      Send Clock offset Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_ClkOffset_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

    /* BlueZ ONLY */
    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_ClkOffset_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_CLKOFFSET_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 7;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE;
	pEventHead->total_len = 5;
	dest += sizeof(UINT16);
	*dest = BT_HCI_STATUS_SUCCESS;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pTempConnectDevice->clock_offset;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Remote_NameReq_Complete_Event
 *
 *   Descriptions:
 *      Send Remote name request Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *    Changelog:
 *		(1)Jakio20070723: add third para 'urgent', if set this flag, set
 *			IntSendingTimer in this routine, else not set timer
 *************************************************************************/
VOID Task_HCI2HC_Send_Remote_NameReq_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

    /* BlueZ ONLY */
    //Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Remote_NameReq_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMEREQ_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 257;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE;
	pEventHead->total_len = 255;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	RtlCopyMemory(dest, pTempConnectDevice->remote_name, BT_LOCAL_NAME_LENGTH);
	dest += BT_LOCAL_NAME_LENGTH;
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event
 *wc
 *   Descriptions:
 *      Send Remote name support feature Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
    /* BlueZ ONLY */
    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_RemoteNameSuppFea_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMESUPPFEA_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 13;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_READ_REMOTE_SUPPORTED_REATURES_COMPLETE;
	pEventHead->total_len = 11;
	dest += sizeof(UINT16);
	*dest = BT_HCI_STATUS_SUCCESS;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, &pTempConnectDevice->lmp_features, sizeof(LMP_FEATURES_T));
	dest += sizeof(LMP_FEATURES_T);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

    EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_RemoteVerInfo_Complete_Event
 *
 *   Descriptions:
 *      Send Remote version information Complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_RemoteVerInfo_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
    /* BlueZ ONLY */
    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_REMOTE_VERINFO_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 10;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE;
	pEventHead->total_len = 8;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->current_reason_code;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->lmp_version;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->manufacturer_name;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pTempConnectDevice->lmp_subversion;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Conn_Req_Event
 *
 *   Descriptions:
 *      Send connect request event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Conn_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Conn_Req_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_CONN_REQ_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 12;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_CONNECTION_REQUEST;
	pEventHead->total_len = 10;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	RtlCopyMemory(dest, pTempConnectDevice->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	dest += BT_CLASS_OF_DEVICE_LENGTH;
	if (pTempConnectDevice->acl_or_sco_flag == BT_ACL_IN_USE)
	{
		*dest = BT_LINK_TYPE_ACL;
	}
	else
	{
		if (pTempConnectDevice->pScoConnectDevice)
		{
			if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->using_esco_command_flag)
			{
				*dest = BT_LINK_TYPE_ESCO;
			}
			else
			{
				*dest = BT_LINK_TYPE_SCO;
			}
		}
		else
		{
			*dest = BT_LINK_TYPE_SCO;
		}
	}
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Link_Key_Req_Event
 *
 *   Descriptions:
 *      Send link key request event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Link_Key_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Link_Key_Req_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_REQ_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 8;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_LINK_KEY_REQUEST;
	pEventHead->total_len = 6;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Link_Key_Notification_Event
 *
 *   Descriptions:
 *      Send link key notification event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Link_Key_Notification_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Link_Key_Notification_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_NOTIFICATION_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 25;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_LINK_KEY_NOTIFICATION;
	pEventHead->total_len = 23;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	RtlCopyMemory(dest, pTempConnectDevice->link_key, BT_LINK_KEY_LENGTH);
	dest += BT_LINK_KEY_LENGTH;
	*dest = pTempConnectDevice->key_type_key;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Pin_Code_Req_Event
 *
 *   Descriptions:
 *      Send pin code request event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Pin_Code_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Pin_Code_Req_Even \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_PIN_CODE_REQ_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 8;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_PIN_CODE_REQUEST;
	pEventHead->total_len = 6;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Encryption_Change_Event
 *
 *   Descriptions:
 *      Send encryption change event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Encryption_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;
    
    /* BlueZ ONLY, send the command status in the moment of Encryption success */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_SET_CONNECTION_ENCRYPTION))){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
        
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Encryption_Change_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_ENCRYPTION_CHANGE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 6;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_ENCRYPTION_CHANGE;
	pEventHead->total_len = 4;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = (pTempConnectDevice->encryption_mode == 0) ? 0 : 1;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Authentication_Complete_Event
 *
 *   Descriptions:
 *      Send authentication complete event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Authentication_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Authentication_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_AUTHENTICATION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 5;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_AUTHENTICATION_COMPLETE;
	pEventHead->total_len = 3;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event
 *
 *   Descriptions:
 *      Send connection packet type changed event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_CHANGE_CONNECTION_PACKET_TYPE))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Conn_Packet_Type_Changed_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_CONN_PACKET_TYPE_CHANGED_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 7;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_CONNECTION_PACKET_TYPE_CHANGED;
	pEventHead->total_len = 5;
	dest += sizeof(UINT16);
	*dest = BT_HCI_STATUS_SUCCESS;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pTempConnectDevice->packet_type;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Send_Sco_Connection_Complete_Event
 *
 *   Descriptions:
 *      Send Connection Complete event to host controller(SCO used).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Sco_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

	PBT_HCI_T pHci;

    ENTER_FUNC();
    /* BlueZ ONLY */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_ADD_SCO_CONNECTION))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	if (pTempConnectDevice->pScoConnectDevice == NULL)
	{
		return ;
	}

	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_SCO_CONNECTION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 13;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_CONNECTION_COMPLETE;
	pEventHead->total_len = 11;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->connection_handle;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	*dest = BT_LINK_TYPE_SCO;
	dest += sizeof(UINT8);
	*dest = pTempConnectDevice->encryption_mode;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
	

	Task_CreateTask((PBT_TASK_T)devExt->pTask, 
				BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_CHECK_SCO_SNIFF), 
				BT_TASK_PRI_NORMAL, 
				NULL, 
				0);
	
	EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_Sco_Disconnection_Complete_Event
 *
 *   Descriptions:
 *      Send Disconnection Complete event to host controller for Sco link.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    ENTER_FUNC();

    /* BlueZ ONLY */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_DISCONNECT))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	if (pTempConnectDevice->pScoConnectDevice == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_SCO_DISCONNECTION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 6;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_DISCONNECTION_COMPLETE;
	pEventHead->total_len = 4;
	dest += sizeof(UINT16);
	*dest = BT_HCI_STATUS_SUCCESS;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->connection_handle;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_reason_code;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_HCI2HC_Mode_Change_Event
 *
 *   Descriptions:
 *      Send Mode Change event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Mode_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Mode_Change_Event \n");
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_MODE_CHANGE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 8;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_MODE_CHANGE;
	pEventHead->total_len = 6;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->mode_current_mode;
	dest += sizeof(UINT8);
	if (pTempConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_HOLD)
	{
		*(PUINT16)dest = pTempConnectDevice->hold_mode_interval;
	}
	else if (pTempConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
	{
		*(PUINT16)dest = pTempConnectDevice->sniff_mode_interval;
	}
	else
	{
		*(PUINT16)dest = 0;
	}
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_Normal_Add_Sco
 *
 *   Descriptions:
 *      If BD and frag element is empty, create task "BT_TASK_EVENT_HCI2LMP_ADD_SCO".
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Normal_Add_Sco(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	LARGE_INTEGER timevalue;
	UINT8 tmpamaddr = 0;

	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Normal_Add_Sco \n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_Normal_Add_Sco fail(1)\n");

		return ;
	}
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_TASK | LEVEL0,  "Task_Normal_Add_Sco fail(2)\n");
		return ;
	}
	if (BtIsBDEmpty(devExt))
	{
		BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Add_Sco (01)\n");
		pHci->acl_temp_pending_flag = 0;
		if (pTempConnectDevice->pScoConnectDevice != NULL)
		{
			pHci->sco_need_1_slot_for_acl_flag = 1;
			pHci->hv1_use_dv_instead_dh1_flag = 0;
			BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Add_Sco (02)\n");
			Frag_StartQueueForSco(devExt,1);
			Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 1);
			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT);
			pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
			if (pScoConnectDevice->using_esco_command_flag)
			{
				BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Add_Sco (03)\n");
				if (pScoConnectDevice->esco_packet_type &(BT_PACKET_TYPE_ESCO_EV3 | BT_PACKET_TYPE_ESCO_EV4 | BT_PACKET_TYPE_ESCO_EV5))
				{
					if (pTempConnectDevice->current_role == BT_ROLE_MASTER)
					{
						if (pScoConnectDevice->esco_amaddr == 0)
						{
							Hci_Allocate_Am_address(pHci, &tmpamaddr);
							pScoConnectDevice->esco_amaddr = tmpamaddr;
						}
					}
					Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ADD_ESCO), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Add_Sco (04)\n");
					Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ADD_SCO), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
				}
			}
			else
			{
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_ADD_SCO), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
			}
		}
		else
		{
			BT_DBGEXT(ZONE_TASK | LEVEL1,  "Task_Normal_Add_Sco fail(3)\n");
		}
		if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
		{
			tasklet_schedule(&devExt->taskletRcv);
		}
	}
	else
	{
		BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Add_Sco (05)\n");
		UsbQueryDMASpace(devExt);
		if (pTempConnectDevice->pScoConnectDevice != NULL)
		{
			Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_ADD_SCO), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		}
		else
		{
		}
	}
}

/**************************************************************************
 *   Task_Normal_Send_Conn_Req
 *
 *   Descriptions:
 *      If BD and frag element is empty, just call function Task_HCI2HC_Send_Conn_Req_Event.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Normal_Send_Conn_Req(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	LARGE_INTEGER timevalue;
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Normal_Send_Conn_Req \n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pHci->acl_temp_pending_flag = 0;
		if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
		{
			tasklet_schedule(&devExt->taskletRcv);
		}
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return ;
	}
	if (BtIsBDEmpty(devExt))
	{
		pHci->acl_temp_pending_flag = 0;
		if (pTempConnectDevice->pScoConnectDevice != NULL)
		{
			Task_HCI2HC_Send_Conn_Req_Event(devExt, para, 0);
		}
		else
		{
		}
		if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
		{
			tasklet_schedule(&devExt->taskletRcv);
		}
	}
	else
	{
		if (pTempConnectDevice->pScoConnectDevice != NULL)
		{
			Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_SEND_CONN_REQ), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		}
		else
		{
			pHci->acl_temp_pending_flag = 0;
			if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
			{
				tasklet_schedule(&devExt->taskletRcv);
			}
		}
	}
}

/**************************************************************************
 *   Task_Normal_Res_Discard_Command
 *
 *   Descriptions:
 *      Send status event to notify host controller that a command is discarded.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      opcode: IN, the command whose opcode is opcode.
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Normal_Res_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Task_Normal_Res_Discard_Command, opcode = %x\n", opcode);
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_RES_DISCARD_COMMAND), BT_TASK_PRI_NORMAL, (PUINT8) &opcode, 2);
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 6;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_COMMAND_STATUS;
	pEventHead->total_len = 4;
	dest += sizeof(UINT16);
	*dest = BT_HCI_ERROR_UNSPECIFIED_ERROR;
	dest += sizeof(UINT8);
	*dest = 1;
	dest += sizeof(UINT8);
	*(PUINT16)dest = opcode;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}

/**************************************************************************
 *   Task_Normal_Rel_Resouce_For_Remote_Req
 *
 *   Descriptions:
 *      Release resource for remote request which establishes a temp link.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_Normal_Rel_Resouce_For_Remote_Req(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Task_Normal_Rel_Resouce_For_Remote_Req \n");
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	if (pTempConnectDevice->tempflag == 1)
	{
		
		pTempConnectDevice->state = BT_DEVICE_STATE_IDLE;
		Hci_StopAllTimer(pTempConnectDevice);
		Hci_InitLMPMembers(devExt, pTempConnectDevice);
#ifdef ACCESS_REGISTER_DIRECTLY
		Hci_Clear_AM_Addr(devExt, pTempConnectDevice->am_addr);
		Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pTempConnectDevice->am_addr);
#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
		Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pTempConnectDevice);
#else
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
#endif
#else
		BtUsbTaskReleaseRemoteRequest(devExt);
#endif
		Hci_InitTxForConnDev(devExt, pTempConnectDevice);
		Hci_ReleaseScoLink(devExt, pTempConnectDevice);
		Hci_Del_Connect_Device_By_AMAddr(pHci, pTempConnectDevice->am_addr);
#ifdef BT_AUTO_SEL_PACKET
		if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
		{
			devExt->TxAclSendCount = 0;
			devExt->StartRetryCount = 0;
			devExt->EndRetryCount = 0;
			devExt->TxRetryCount = 0;
			Hci_ClearMainlyCommandIndicator(devExt);
		}
#endif
	}
}

/**************************************************************************
 *   Task_HCI2HC_Role_Change_Event
 *
 *   Descriptions:
 *      Send Role Change event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Role_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    ENTER_FUNC();
    /* BlueZ ONLY, send the command status in the moment of Role Switch success */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE)
        &&(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_SWITCH_ROLE))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    
    
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_ROLE_CHANGE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 10;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_ROLE_CHANGE;
	pEventHead->total_len = 8;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	*dest = pTempConnectDevice->current_role;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

    EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_Sync_Connection_Complete_Event
 *
 *   Descriptions:
 *      Send Synchronous Connection Complete event to host controller(ESCO used).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Sync_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

	PBT_HCI_T pHci;

    ENTER_FUNC();

    /* BlueZ ONLY */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) 
        &&
    	( (pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_SETUP_SYNC_CONNECTION))
    	  || (pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_ACCEPT_SYNC_CONNECTION_REQ))
    	  || (pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REJECT_SYNC_CONNECTION_REQ))
    	)
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	if (pTempConnectDevice->pScoConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send_Sync_Connection_Complete_Event used by SCO link \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 19;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_SYNC_CONNECTION_COMPLETE;
	pEventHead->total_len = 17;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->connection_handle;
	dest += sizeof(UINT16);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->link_type;
	dest += sizeof(UINT8);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->transmission_interval;
	dest += sizeof(UINT8);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->retransmission_window;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->rx_packet_length;
	dest += sizeof(UINT16);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->tx_packet_length;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->air_mode;
	dest += sizeof(UINT8);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

	Task_CreateTask((PBT_TASK_T)devExt->pTask, 
				BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_CHECK_SCO_SNIFF), 
				BT_TASK_PRI_NORMAL, 
				NULL, 
				0);

       EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_Sync_Connection_Changed_Event
 *
 *   Descriptions:
 *      Send Synchronous Connection Changed event to host controller(ESCO used).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Sync_Connection_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;

	PBT_HCI_T pHci;

    ENTER_FUNC();

    /* BlueZ ONLY */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE) &&
    	(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_SETUP_SYNC_CONNECTION))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	if (pTempConnectDevice->pScoConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send_Sync_Connection_Changed_Event used by SCO link \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_CHANGED_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 11;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_SYNC_CONNECTION_CHANGED;
	pEventHead->total_len = 9;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->connection_handle;
	dest += sizeof(UINT16);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->transmission_interval;
	dest += sizeof(UINT8);
	*dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->retransmission_window;
	dest += sizeof(UINT8);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->rx_packet_length;
	dest += sizeof(UINT16);
	*(PUINT16)dest = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->tx_packet_length;
	dest += sizeof(UINT16);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

    EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_Qos_Setup_Complete_Event
 *
 *   Descriptions:
 *      Send Qos Setup Complete event to host controller(ESCO used).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Qos_Setup_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    ENTER_FUNC();
    /* BlueZ ONLY, send the command status in the moment of Role Switch success */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE)
        &&(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_QOS_SETUP))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}

	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_QOS_SETUP_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 23;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_QOS_SETUP_COMPLETE;
	pEventHead->total_len = 21;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->qos_flags;
	dest += sizeof(UINT8);
	*dest = pTempConnectDevice->qos_service_type;
	dest += sizeof(UINT8);
	*(PUINT32)dest = pTempConnectDevice->qos_token_rate;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_peak_bandwidth;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_latency;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_delay_variation;
	dest += sizeof(UINT32);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}

    EXIT_FUNC();
}

/**************************************************************************
 *   Task_HCI2HC_Send_Flow_Specification_Complete_Event
 *
 *   Descriptions:
 *      Send Flow Specification Complete event to host controller(ESCO used).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None
 *************************************************************************/
VOID Task_HCI2HC_Send_Flow_Specification_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_HCI_T pHci;

    ENTER_FUNC();
    /* BlueZ ONLY, send the command status in the moment of Role Switch success */
    pHci = (PBT_HCI_T)devExt->pHci;
	if((pHci->command_state != BT_COMMAND_STATE_IDLE)
        &&(pHci->current_opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_FLOW_SPECIFICATION))
      ){
    	
    	pHci->command_status = BT_HCI_STATUS_SUCCESS;
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Flow_Specification_Complete_Event \n");
	KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);
	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_FLOW_SPECIFICATION_COMPLETE_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return ;
	}
	dest = devExt->RxCach[devExt->RxCachPid];
	*(PUINT16)dest = 24;
	dest += sizeof(UINT16);
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);
	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_FLOW_SPECIFICATION_COMPLETE;
	pEventHead->total_len = 22;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);
	*dest = pTempConnectDevice->qos_flags;
	dest += sizeof(UINT8);
	*dest = pTempConnectDevice->qos_flow_direction;
	dest += sizeof(UINT8);
	*dest = pTempConnectDevice->qos_service_type;
	dest += sizeof(UINT8);
	*(PUINT32)dest = pTempConnectDevice->qos_token_rate;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_token_bucket_size;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_peak_bandwidth;
	dest += sizeof(UINT32);
	*(PUINT32)dest = pTempConnectDevice->qos_latency;
	dest += sizeof(UINT32);
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);
	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
    EXIT_FUNC();
}




/**************************************************************************
 *   Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event
 *
 *   Descriptions:
 *      Send connection packet type changed event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PSCO_CONNECT_DEVICE_T pTempScoConnectDevice;
	
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));

	if (pTempConnectDevice == NULL)
		return;

	if (pTempConnectDevice->pScoConnectDevice == NULL)
		return;

	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send_Sco_Conn_Packet_Type_Changed_Event \n");
	
	pTempScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
	
    // Get the Read Queue lock, so we can insert our IRP
    //
    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);

		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_SCO_CONN_PACKET_TYPE_CHANGED_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return;
	}

	dest = devExt->RxCach[devExt->RxCachPid];

	
	*(PUINT16)dest = 7; 
	dest += sizeof(UINT16);
	
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);


	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_CONNECTION_PACKET_TYPE_CHANGED;
	pEventHead->total_len = 5;

	dest += sizeof(UINT16);

	*dest = pTempScoConnectDevice->current_reason_code;
	dest += sizeof(UINT8);

	*(PUINT16)dest = pTempScoConnectDevice->connection_handle;
	dest += sizeof(UINT16);

	if (pTempScoConnectDevice->current_packet_type == pTempScoConnectDevice->changed_packet_type)
	{
		*(PUINT16)dest = pTempScoConnectDevice->packet_type;
	}
	else
	{
		if (pTempScoConnectDevice->current_packet_type == BT_SCO_PACKET_HV1)
			*(PUINT16)dest = BT_PACKET_TYPE_HV1;
		else if (pTempScoConnectDevice->current_packet_type == BT_SCO_PACKET_HV2)
			*(PUINT16)dest = BT_PACKET_TYPE_HV2;
		else
			*(PUINT16)dest = BT_PACKET_TYPE_HV3;
	}
	dest += sizeof(UINT16);
	
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);




	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}




/**************************************************************************
 *   Task_HCI2HC_Send_Max_Slot_Changed_Event
 *
 *   Descriptions:
 *      Send max slot changed event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Task_HCI2HC_Send_Max_Slot_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	PCONNECT_DEVICE_T pTempConnectDevice;
	
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));

	if (pTempConnectDevice == NULL)
		return;
	
	BT_DBGEXT(ZONE_TASK | LEVEL3,  "Send_Max_Slot_Changed_Event \n");
	
    // Get the Read Queue lock, so we can insert our IRP
    //
    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);

		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_MAX_SLOT_CHANGED_EVENT), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return;
	}

	dest = devExt->RxCach[devExt->RxCachPid];

	
	*(PUINT16)dest = 5; 
	dest += sizeof(UINT16);
	
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);


	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_MAX_SLOTS_CHANGE;
	pEventHead->total_len = 3;

	dest += sizeof(UINT16);

	*(PUINT16)dest = pTempConnectDevice->connection_handle;
	dest += sizeof(UINT16);

	*dest = pTempConnectDevice->tx_max_slot;
	dest += sizeof(UINT8);
	
	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);




	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}



/**************************************************************************
 *   Task_HCI2HC_Send_Hardware_Error_Event
 *
 *   Descriptions:
 *      Send hardware error event to host controller.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      para: IN, parameters of this event
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Task_HCI2HC_Send_Hardware_Error_Event(PBT_DEVICE_EXT devExt, UINT8 Hardware_Code, UINT8 urgent)
{
	KIRQL oldIrql;
	PUINT8 dest;
	PHCI_EVENT_HEADER_T pEventHead;
	LARGE_INTEGER timevalue;
	
	BT_DBGEXT(ZONE_TASK | LEVEL2,  "Send_Hardware_Error_Event \n");
	
    // Get the Read Queue lock, so we can insert our IRP
    //
    KeAcquireSpinLock(&devExt->ReadQueueLock, &oldIrql);

	if (BtIsRxCachFull(devExt))
	{
		KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);

		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_HARDWARE_ERROR_EVENT), BT_TASK_PRI_NORMAL, &Hardware_Code, sizeof(UINT8));
		return;
	}

	dest = devExt->RxCach[devExt->RxCachPid];

	
	*(PUINT16)dest = 3; 
	dest += sizeof(UINT16);
	
	*(PUINT16)dest = (UINT16)RX_FRAME_TYPE_EVENT;
	dest += sizeof(UINT16);


	pEventHead = (PHCI_EVENT_HEADER_T)dest;
	pEventHead->eventcode = BT_HCI_EVENT_HARDWARE_ERROR;
	pEventHead->total_len = 1;

	dest += sizeof(UINT16);

	*dest = Hardware_Code;
	dest += sizeof(UINT8);

	devExt->RxCachPid = BtGetNextRxCachId(devExt->RxCachPid);
	KeReleaseSpinLock(&devExt->ReadQueueLock, oldIrql);




	if ((devExt->AllowIntSendingFlag == TRUE) && (urgent != 0))
	{
		tasklet_schedule(&devExt->taskletRcv);
	}
}
