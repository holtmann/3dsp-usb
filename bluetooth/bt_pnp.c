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
$RCSfile: bt_pnp.c,v $
$Revision: 1.12 $
$Date: 2010/09/06 05:17:28 $
 **************************************************************************/

#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"
#include "bt_task.h"
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
#include "bt_usbregdef.h"
#include "bt_frag.h"	
#include "base_types.h"

#include "btdownload.h"
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
#include "afhclassify.h"
#endif
#ifdef BT_SCHEDULER_SUPPORT
#include "sched.h"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
extern void DiagWorkFunc(struct work_struct *pwork);
#else
extern void DiagWorkFunc(ULONG_PTR pvoid);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
#ifndef init_MUTEX
#define init_MUTEX(sem)		sema_init(sem, 1)
#endif
#endif
///////////////////////////////////////////////////////////////////////////////
//
//  BtAddDevice
//
//      We are called at this entry point by the Plug and Play Manager
//      to add a Functional Device Object for a Physical Device Object.
//      Note that we may NOT access the device in this routine, as the
//      Plug and Play Manager has not yet given us any hardware resoruces.
//      We get these hardware resources via the IRP_MJ_PNP IRP with
//      a minor function IRP_MN_START_DEVICE.
//
//
//  INPUTS:
//
//      DriverObj - Address of our DRIVER_OBJECT.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//    This routine is called at IRQL_PASSIVE_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS BtAddDevice(PBT_DEVICE_EXT devExt)
{
	NTSTATUS code = STATUS_SUCCESS;

	BT_DBGEXT(ZONE_PNP | LEVEL2,  "BtAddDevice: entered sizeof(BT_DEVICE_EXT) %d\n", sizeof(BT_DEVICE_EXT));

	//zero memory
	RtlZeroMemory(devExt, sizeof(BT_DEVICE_EXT));

	// Initialize our Spin Locks
	KeInitializeSpinLock(&devExt->ReadQueueLock);
	//KeInitializeSpinLock(&devExt->WriteQueueLock);
	//KeInitializeSpinLock(&devExt->ThreadSpinLock);
	KeInitializeSpinLock(&devExt->RxStateLock);
	KeInitializeSpinLock(&devExt->AsynCmdLock);
	

	init_waitqueue_head(&devExt->RxIdleEvent);
	init_waitqueue_head(&devExt->InqIdleEvent);
	init_waitqueue_head(&devExt->IntIdleEvent);
	init_waitqueue_head(&devExt->ComboReadyEvent);
	init_waitqueue_head(&devExt->RxFwEvent);
	init_waitqueue_head(&devExt->MainLoopEvent);
	init_waitqueue_head(&devExt->MinLoopEvent);
	init_waitqueue_head(&devExt->JoinEvent);
	init_waitqueue_head(&devExt->DevRdyEvt);
    // Command queue init
    skb_queue_head_init(&devExt->cmd_queue);


    atomic_set(&devExt->RxIdleFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->IntIdleFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->InqIdleFlag, EVENT_UNSIGNED);
    //atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->ComboReadyFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->RxFwFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->mainLoopFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->minLoopFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->joinFlag, EVENT_UNSIGNED);
    atomic_set(&devExt->devRdyFlag, EVENT_UNSIGNED);

    // Lmp Debug Init
    init_waitqueue_head(&devExt->debug.lmpDumpEvt);
    devExt->debug.lmpEvtFlag = 0;
    skb_queue_head_init(&devExt->debug.lmpDumpQ);
    
	devExt->ManualCancelFlag = 0;
	devExt->SpaceQueryFlag = FALSE;
	devExt->PsComboReqFlag = FALSE;
	devExt->AllowWlanJoin = TRUE;
	devExt->IsRxIrpSubmit = FALSE;
	devExt->IntSendingTimerMissed = FALSE;
	devExt->IsInquiryIrpSubmit = FALSE;

	devExt->SubmitBulkoutAcl = FALSE;
	devExt->SubmitBulkoutSco = FALSE;

	devExt->SubmitReadIrp = 0;
	devExt->QueryFailCount = 0;

	devExt->pCmdPendingIrp = NULL;
	//
	// Init the count of in-progress I/O requests to zero.  We use this
	// to keep track of when we can remove the device.
	//
	devExt->OutstandingIO = 0;
	devExt->UsbOutstandingIO = 0;

	//init rx bulkin failure count
	devExt->RxFailureCount = 0;
	devExt->VcmdAsynFailureCount = 0;

	//init afh pdu count
	devExt->AfhPduCount = 0;
	
	//
	// Internal device state flags, used for managing PnP state of device
	//
	devExt->Started = FALSE;
	devExt->HoldNewRequests = TRUE;
	devExt->Removed = FALSE;
	devExt->AllowIntSendingFlag = TRUE;
	//devExt->AllowSubmitReadIrpFlag = TRUE;
	devExt->Standby = 0;

	devExt->RxState = RX_STATE_CONNECTED;
	devExt->AclWriteFlag = 0xff;
	devExt->SlaveWriteFlag = 0xff;
	devExt->SniffWriteFlag = 0xff;
	devExt->FlagStatus = STATUS_SUCCESS;
	//
	// Set initial state
	//
	devExt->State = STATE_NEVER_STARTED;

	devExt->DriverHaltFlag = 0;
	devExt->LoadEepromFlag = 0;
	devExt->FakeD3State = 0;
	devExt->InD3Flag = 0;
	devExt->WorkItemRun = FALSE;

	devExt->RssiWhenSniff = FALSE;
		
	RtlCopyMemory(devExt->MyVersion, BT_STRING_VERSION_ID, sizeof(BT_STRING_VERSION_ID));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
	init_timer(&devExt->BtWorkitem.timer);
	INIT_WORK(&devExt->BtWorkitem.work, BtWatchdogItem);
	init_timer(&devExt->cmd_work.timer);
    INIT_WORK(&devExt->cmd_work.work, thread_processing_cmd);
    
    // Diagnose related init
    INIT_WORK(&devExt->diagWork, DiagWorkFunc);
#else	
	init_timer(&devExt->BtWorkitem.timer);	
	INIT_WORK(&devExt->BtWorkitem, BtWatchdogItem, devExt);	
	init_timer(&devExt->cmd_work.timer);      
	INIT_WORK(&devExt->cmd_work, thread_processing_cmd, devExt);          
	INIT_WORK(&devExt->diagWork, DiagWorkFunc, devExt);
#endif
    init_waitqueue_head(&devExt->diagEvent);
    devExt->diagFlag = 0;

    _urb_queue_init(&devExt->urb_pool);
    _urb_queue_init(&devExt->complete_q);

    // Mutext to sync between open and close
    init_MUTEX(&devExt->openLock);
   	return code;
}

static NTSTATUS PreStart(PBT_DEVICE_EXT devExt)
{
	/* USB completion tasklet */
	tasklet_init(&devExt->taskletRcv, BtTriggerRcv, (ULONG_PTR)devExt);
    tasklet_init(&devExt->usb_complete_task, usb_complete_task, (ULONG_PTR)devExt);    
    devExt->pBufBridge = vmalloc(BT_MAX_FRAME_SIZE + 1);
	if(!devExt->pBufBridge)
	{
		BT_DBGEXT(ZONE_PNP | LEVEL0,  "vmalloc ERROR!!\n");
		return -STATUS_UNSUCCESSFUL;
	}
	devExt->pBtWorkqueue = create_singlethread_workqueue("btusb_cmd_wq");
	if (!devExt->pBtWorkqueue)
	{
		BT_DBGEXT(ZONE_PNP | LEVEL0,  "Work queue create failed!\n");
		return -STATUS_UNSUCCESSFUL;
	}
    
	devExt->ComboState = FW_WORK_MODE_SINGLE;
	devExt->OsVersion = LINUX_FC9;

	return STATUS_SUCCESS;
}

NTSTATUS ReleasePreStart(PBT_DEVICE_EXT devExt)
{
	/* Release the tasklet */
	BT_DBGEXT(ZONE_PNP | LEVEL3,"Release Tasklets and work thread;\n");
    tasklet_kill(&devExt->taskletRcv);
    tasklet_kill(&devExt->usb_complete_task);

	if(devExt->pBtWorkqueue)
	{
		BT_DBGEXT(ZONE_PNP | LEVEL3, "Release command work queue\n");
	    flush_workqueue(devExt->pBtWorkqueue);
		destroy_workqueue(devExt->pBtWorkqueue);
		devExt->pBtWorkqueue = NULL;
	}

	if(devExt->pBufBridge)
	{
        vfree(devExt->pBufBridge);
	}
    return STATUS_SUCCESS;
}


/****************************************************************************
 BtStartDevice
	This function is called from the DispatchPnp Entry Point to
	actually start the hardware.

INPUTS:
	DevExt  - Address of our device extension.
	IoStackLocation -- Pointer to I/O Stack Location containing
	configuration information

OUTPUTS:
	None.

RETURNS:
	STATUS_SUCCESS;

ChangeLog:
	Jakio20071204 rewrite this function to avoid mem peak
	Jakio20071213 changed this function proto type for combo test
****************************************************************************/
NTSTATUS BtStartDevice(PBT_DEVICE_EXT devExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT8 Process = 0;
	UINT8 FunctionOK = FALSE;
	UINT8	Magic[4] = {0x33, 0x44, 0x53, 0x50};//stand for"3DSP"
	UINT8	tmpBuf[4];
	UINT32 Loop;
	BOOLEAN CancelRx,CancelInt,CancelInquiry;
	LARGE_INTEGER timevalue;

	__try
	{
		devExt->StartStage = START_STAGE_INIT;

		devExt->devRdyOk = 0;

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "Pre Start Device\n");
		if(STATUS_SUCCESS != PreStart(devExt))
		{
			__leave;
		}
        
		BT_DBGEXT(ZONE_PNP | LEVEL3,  "SetupShareMemory\n");
		if (BtSetupShareMemory(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 1;

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "Usbinitialize\n");
		if (UsbInitialize(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 2;

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "usb active device\n");
		if (UsbActiveDevice(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 3;

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "frag_init\n");
		devExt->ScrachFreeSize = 0;
		if (Frag_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "flush init\n");
		#ifdef BT_3DSP_FLUSH_SUPPORT
		if (Flush_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		#endif
		Process = 4;	


		BT_DBGEXT(ZONE_PNP | LEVEL3,  "task init\n");
		if (Task_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 5;

		BT_DBGEXT(ZONE_PNP | LEVEL3,  "devExt 0x%x,devExt->usb_device 0x%x\n", devExt,devExt->usb_device);

		Status = BtUsbInt(devExt);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
		Process = 6;


		BT_DBGEXT(ZONE_PNP | LEVEL3,  "ready download 8051\n");

		if(!Vcmd_8051fw_isWork(devExt))
		{
			Status = DownLoadFirmWare(devExt, 0, TRUE);
			if (Status != STATUS_SUCCESS)
			{
				BT_DBGEXT(ZONE_PNP | LEVEL1, "Download Fails\n");
				__leave;
			}
		}
		else
		{
			VendorCmdBurstRegRead2(devExt, REG_InitCompleted, tmpBuf, 4);
			while(!RtlEqualMemory(Magic, tmpBuf, 4))
			{
				BtDelay(1000);	
				VendorCmdBurstRegRead2(devExt, REG_InitCompleted, tmpBuf, 4);
				BT_DBGEXT(ZONE_PNP | LEVEL3,  "%2x %2x %2x %2x\n",tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3]);
				if(FALSE == BtIsPluggedIn(devExt))
				{
					BT_DBGEXT(ZONE_PNP | LEVEL1,  "BtStartDevice---Device is removed\n");
					__leave;
				}
			}
			BT_DBGEXT(ZONE_PNP | LEVEL1,  "BtStartDevice----8051 init completed\n");
			Loop = 5;
			while(--Loop)
			{
				VendorCmdWriteCmdToMailBox(devExt, &devExt->ComboState, MAILBOX_CMD_REQUEST_TO_JOIN);
				if(FALSE == BtIsPluggedIn(devExt))
				{
					BT_DBGEXT(ZONE_PNP | LEVEL1, "BtStartDevice---device is removed\n");
					__leave;
				}

				//wait event for 5 seconds
				timevalue.QuadPart =  5*HZ;
				Status = wait_event_interruptible_timeout(devExt->JoinEvent, atomic_read(&devExt->joinFlag) == EVENT_SIGNED, timevalue.QuadPart);
                		atomic_set(&devExt->joinFlag, EVENT_UNSIGNED);
				if(0 != Status)
				{
					if(FALSE == BtIsPluggedIn(devExt))
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1, "BtStartDevice---Join event set,device is removed\n");
					}
					else
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1, "BtStartDevice---Join cmd sending ok\n");
					}
					break;
				}
				else
				{
					BT_DBGEXT(ZONE_PNP | LEVEL1, "BtStartDevice---Join cmd lost, retry it\n");
					DumpUsbRegs(devExt);
					continue;
				}
			}
			if(Loop == 0)
			{
				BT_DBGEXT(ZONE_PNP | LEVEL1,  "BtStartDevice---retry join cmd limit reached, give up\n");
				__leave;
			}

			FunctionOK = TRUE;
			__leave;
			
		}

		Status = BtUsbSubmitReadIrp(devExt);
		if(!NT_SUCCESS(Status))
		{
			__leave;
		}
		Status = BtUsbSubmitIquiryIrp(devExt);
		if(!NT_SUCCESS(Status))
		{
			__leave;
		}	
		Process = 7;
		
		if (BtInitHw(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 8;
		
		if (Hci_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 9;

		if (LMP_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 10;

		#ifdef BT_AFH_ADJUST_MAP_SUPPORT
		if (Afh_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 11;
		#endif

		#ifdef BT_SCHEDULER_SUPPORT
		if(STATUS_SUCCESS != Sched_Init(devExt))
		{
			__leave;
		}
		Process = 12;
		#endif

		#ifdef BT_TESTMODE_SUPPORT
		if (STATUS_SUCCESS != TestMode_Init(devExt))
		{
			__leave;
		}
		Process = 13;
		#endif
		

		if(STATUS_SUCCESS != BtGetInfoFromEEPROM(devExt)){
			__leave;    
		}

		/* jakio20080125: reset bus enable
*/
		BtUsb_Bus_Reset_Enable(devExt);
		Process = 14;
		devExt->StartStage = START_STAGE_MAINLOOP;
		devExt->devRdyOk = 1;
		if(devExt->ComboState == FW_WORK_MODE_SINGLE)
		{
			Hci_SingleStateInit(devExt);
		}
		else
		{
			Hci_ComboStateInit(devExt);
		}

		atomic_set(&devExt->devRdyFlag, EVENT_SIGNED);
		wake_up_interruptible(&devExt->DevRdyEvt);
		
		FunctionOK = TRUE;
	}
	__finally
	{
		if(FunctionOK == TRUE)
		{
			BT_DBGEXT(ZONE_PNP | LEVEL1,  "Start device ok, work mode = %d\n", devExt->ComboState);
		}
		else
		{
			BT_DBGEXT(ZONE_PNP | LEVEL0,  "start device failure, release resources, STEP=%d\n", Process);
            cancel_delayed_work(&devExt->cmd_work);

			if(Process >= 14)
			{
				
			}
			if(Process >= 13)
			{
				#ifdef BT_TESTMODE_SUPPORT
				TestMode_Release(devExt);
				#endif
			}
			if(Process >= 12)
			{
				#ifdef BT_SCHEDULER_SUPPORT
				Sched_Release(devExt);
				#endif
			}
			if(Process >= 11)
			{
				#ifdef BT_AFH_ADJUST_MAP_SUPPORT
				Afh_Release(devExt);
				#endif
			}
			if(Process >= 10)
			{
				LMP_Release(devExt);
				
			}
			if(Process >= 9)
			{
				Hci_Release(devExt);
			}
			if(Process >= 8)
			{
				;
			}
			if(Process >= 7)
			{
				CancelRx = UsbCancelRxReq(devExt);
				CancelInquiry = UsbCancelInquiryIrp(devExt);
				if(CancelRx)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->RxIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->RxIdleEvent, atomic_read(&devExt->RxIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait rx idle event timeout\n");
					}
				}
				if(CancelInquiry)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->InqIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->InqIdleEvent, atomic_read(&devExt->InqIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait inquiry irp idle event timeout\n");
					}
				}
				
				
			}
			if(Process >= 6)
			{
				CancelInt = UsbCancelInterruptReq(devExt);
				if(CancelInt)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->IntIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->IntIdleEvent, atomic_read(&devExt->IntIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait int idle event timeout\n");
					}
		
				}
			}
			if(Process >= 5)
			{
				Task_Release(devExt);
			}
			if(Process >= 4)
			{
				Frag_Release(devExt);
				
				#ifdef BT_3DSP_FLUSH_SUPPORT
				Flush_Release(devExt);
				#endif
			}
			if(Process >= 3)
			{
			}
			if(Process >= 2)
			{
				UsbUnInitialize(devExt);
			}
			if(Process >= 1)
			{
				BtDeleteShareMemory(devExt);
			}
            ReleasePreStart(devExt);
		}
	}

	if(FunctionOK == TRUE)
	{
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;

}


NTSTATUS BtStartDevice_2Phase(PBT_DEVICE_EXT devExt, UINT8 mode)
{
	NTSTATUS Status;
	UINT8 Process = 0;
	UINT8 FunctionOK = FALSE;
	BOOLEAN CancelRx,CancelInt,CancelInquiry;
	LARGE_INTEGER timevalue;
	UINT8 Loop;

	__try
	{
		if(mode == 1)
		{
			Status = DownLoadFirmWare(devExt, 1, FALSE);
			if (Status != STATUS_SUCCESS)
			{
				__leave;
			}	
		}
		/*
		else
		{
			if(STATUS_SUCCESS != VendorCmdWriteCmdToMailBox(devExt, NULL, VCMD_API_8051_JUMP_MAIN_PROCESS))
			{
				BT_DBGEXT(ZONE_PNP | LEVEL0,  "Inform 8051 goto main loop failure\n");
				__leave;
			}
		}
		*/
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_GET_8051_VERSION);
		BtDelay(1000);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_GET_DSP_VERSION);
		
		Status = BtUsbSubmitReadIrp(devExt);
		if(!NT_SUCCESS(Status))
		{
		}
		Status = BtUsbSubmitIquiryIrp(devExt);
		if(!NT_SUCCESS(Status))
		{
			__leave;
		}
		Process = 7;
		
		if (BtInitHw(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 8;
		
		if (Hci_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 9;

		if (LMP_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 10;

		#ifdef BT_AFH_ADJUST_MAP_SUPPORT
		if (Afh_Init(devExt) != STATUS_SUCCESS)
		{
			__leave;
		}
		Process = 11;
		#endif
		
		#ifdef BT_SCHEDULER_SUPPORT
		if(STATUS_SUCCESS != Sched_Init(devExt))
		{
			__leave;
		}
		Process = 12;
		#endif

		#ifdef BT_TESTMODE_SUPPORT
		if (STATUS_SUCCESS != TestMode_Init(devExt))
		{
			__leave;
		}
		Process = 13;
		#endif
	
		BtGetInfoFromEEPROM(devExt);

		/* jakio20080125: reset bus enable
*/
		BtUsb_Bus_Reset_Enable(devExt);
		
		/* jakio20080523: init for combo state
*/
		if(devExt->ComboState == FW_WORK_MODE_COMBO)
		{
			BT_DBGEXT(ZONE_PNP | LEVEL1,  "init vars for combo mode\n");
			Hci_ComboStateInit(devExt);
		}
		else
		{
			BT_DBGEXT(ZONE_PNP | LEVEL1,  "init vars for bt only mode\n");
			Hci_SingleStateInit(devExt);
		}
		Process = 14;
		
		/* jakio20080104: for combo
*/
		devExt->StartStage = START_STAGE_MAINLOOP;
		
		Loop = 5;
		while(--Loop)
		{
			Status = VendorCmdWriteCmdToMailBox(devExt, NULL, VCMD_API_8051_JUMP_MAIN_PROCESS);
			if(Status != STATUS_SUCCESS)
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad3DspFirmWare---vcmd to jump mainloop failed\n");
				__leave;
			}
			else	
			{
				Status = wait_event_interruptible_timeout(devExt->MainLoopEvent, atomic_read(&devExt->mainLoopFlag) == EVENT_SIGNED, 3*HZ);
                atomic_set(&devExt->mainLoopFlag, EVENT_UNSIGNED);
				if(0 == Status){
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Jump MainLoop TIMEOUT\n");
				}
				else{
					BT_DBGEXT(ZONE_DLD | LEVEL1, "Wait MainLoop event ok, Loop count: %d\n", Loop);
					break;
				}
			}
		}

		FunctionOK = TRUE;
	}
	__finally
	{
		if(FunctionOK == TRUE)
		{
			BT_DBGEXT(ZONE_PNP | LEVEL3,  "Start device ok, work mode = %d\n", devExt->ComboState);
		}
		else
		{
			BT_DBGEXT(ZONE_PNP | LEVEL0,  "start device failure, release resources, STEP=%d\n", Process);
            cancel_delayed_work(&devExt->cmd_work);
            
			if(Process >= 14)
			{
			}
			if(Process >= 13)
			{
				#ifdef BT_TESTMODE_SUPPORT
				TestMode_Release(devExt);
				#endif
			}	
			if(Process >= 12)
			{
				#ifdef BT_SCHEDULER_SUPPORT
				Sched_Release(devExt);
				#endif
			}
			if(Process >= 11)
			{
				#ifdef BT_AFH_ADJUST_MAP_SUPPORT
				Afh_Release(devExt);
				#endif
			}
			if(Process >= 10)
			{
				LMP_Release(devExt);
				
			}
			if(Process >= 9)
			{
				Hci_Release(devExt);
			}
			if(Process >= 8)
			{
				;
			}
			if(Process >= 7)
			{
				CancelRx = UsbCancelRxReq(devExt);
				CancelInquiry = UsbCancelInquiryIrp(devExt);
				if(CancelRx)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->RxIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->RxIdleEvent, atomic_read(&devExt->RxIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait rx idle event timeout\n");
					}
				}
				if(CancelInquiry)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->InqIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->InqIdleEvent, atomic_read(&devExt->InqIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait inquiry irp idle event timeout\n");
					}

				}
				
			}
			if(Process >= 6)
			{
				CancelInt = UsbCancelInterruptReq(devExt);
				if(CancelInt)
				{
					timevalue.QuadPart =  HZ*10;
					atomic_set(&devExt->IntIdleFlag, EVENT_UNSIGNED);
					Status = wait_event_interruptible_timeout(devExt->IntIdleEvent, atomic_read(&devExt->IntIdleFlag) == EVENT_SIGNED, timevalue.QuadPart);
					if (Status == 0)
					{
						BT_DBGEXT(ZONE_PNP | LEVEL1,  "wait int idle event timeout\n");
					}
		
				}
			}
			if(Process >= 5)
			{
				Task_Release(devExt);
			}
			if(Process >= 4)
			{
				Frag_Release(devExt);
				#ifdef BT_3DSP_FLUSH_SUPPORT
				Flush_Release(devExt);
				#endif
			}
			if(Process >= 3)
			{
			}
			if(Process >= 2)
			{
				UsbUnInitialize(devExt);
			}
			if(Process >= 1)
			{
				BtDeleteShareMemory(devExt);
			}
		}
	}
	if(FunctionOK == TRUE)
	{
		devExt->devRdyOk = 1;
		Status = STATUS_SUCCESS;
	}
	else
	{  
		devExt->devRdyOk = 0;
		Status = STATUS_UNSUCCESSFUL;
	}

	atomic_set(&devExt->devRdyFlag, EVENT_SIGNED);
	wake_up_interruptible(&devExt->DevRdyEvt);
	return Status;
}


VOID BtRequestIncrement(PBT_DEVICE_EXT devExt)
{
	
	return ;
}


VOID BtRequestDecrement(PBT_DEVICE_EXT devExt)
{
	return;
}


VOID BtReleaseResources(PBT_DEVICE_EXT devExt)
{
	UINT8 subCmd;
	INT32 remain;
	PBT_FRAG_T	pFrag = NULL;
	PBT_TASK_T pTask = NULL;
	
	ENTER_FUNC();
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	pTask = (PBT_TASK_T)devExt->pTask;

	BT_DBGEXT(ZONE_PNP | LEVEL3,"Cancel work item\n");
    cancel_delayed_work(&devExt->BtWorkitem);
	if(pFrag)
	{   
		BT_DBGEXT(ZONE_PNP | LEVEL3, "Release Frag timer\n");
        del_timer_sync(&pFrag->TxTimer);
	}

	if(BtIsPluggedIn(devExt)){
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "Active Close, Request to enter Min Loop\n");
		subCmd = VCMD_SUB_CMD_DRIVER_UNLOAD;
		if(VendorCmdWriteCmdToMailBox(devExt, &subCmd, VCMD_API_8051_JUMP_MIN_PROCESS))
		{
			BT_DBGEXT(ZONE_MAIN | LEVEL2,  "Write CMD Error\n");
		}
		else
		{
        	remain = wait_event_interruptible_timeout(devExt->MinLoopEvent,
        	                                          atomic_read(&devExt->minLoopFlag) == EVENT_SIGNED,
        	                                          10 * HZ);
            atomic_set(&devExt->minLoopFlag, EVENT_UNSIGNED);
			if(0 == remain){
				BT_DBGEXT(ZONE_MAIN | LEVEL0, "Wait Mini Loop timeout\n");
			}
		}
	}

	Task_Stop(devExt->pTask);
	ReleasePreStart(devExt);
	Task_Release(devExt);
    
	#ifdef BT_TESTMODE_SUPPORT
		TestMode_Release(devExt);
	#endif

	#ifdef BT_SCHEDULER_SUPPORT
		Sched_Release(devExt);
	#endif

	#ifdef BT_AFH_ADJUST_MAP_SUPPORT
		Afh_Release(devExt);
	#endif

	#ifdef BT_TESTDRIVER
		Test_Release(devExt);
	#endif

	UsbUnInitialize(devExt);

	BtClearQueues(devExt);

	Frag_Release(devExt);

	LMP_Release(devExt);

	Hci_Release(devExt);

	#ifdef BT_3DSP_FLUSH_SUPPORT
	Flush_Release(devExt);
	#endif

    BtDeleteShareMemory(devExt);
    EXIT_FUNC();
}



VOID BtPrintBuffer(PUINT8 buffer, UINT32 len)
{
	UINT32 i, j, k;
	i = len / 16;
	j = len % 16;
	k = 0;
	BT_DBGEXT(ZONE_PNP | LEVEL3,  "Print Buffer = 0x%x  len = %lu\n", buffer, len);
	for (k = 0; k < i; k++)
	{
		BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10), *(buffer + k * 16+11),  \
			*(buffer + k * 16+12), *(buffer + k * 16+13), *(buffer + k * 16+14), *(buffer + k * 16+15));
	}
	switch (j)
	{
		case (0): break;
		case (1): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x\n", *(buffer + k * 16));
		break;
		case (2): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1));
		break;
		case (3): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2));
		break;
		case (4): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3));
		break;
		case (5): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4));
		break;
		case (6): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5));
		break;
		case (7): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x \n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6));
		break;
		case (8): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7));
		break;
		case (9): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8));
		break;
		case (10): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9));
		break;
		case (11): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10));
		break;
		case (12): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10), *(buffer + k * 16+11));
		break;
		case (13): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10), *(buffer + k * 16+11),  \
			*(buffer + k * 16+12));
		break;
		case (14): BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
			*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10), *(buffer + k * 16+11),  \
			*(buffer + k * 16+12), *(buffer + k * 16+13));
		break;
		default:
			BT_DBGEXT(ZONE_PNP | LEVEL3,  "%02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x\n", *(buffer + k * 16), *(buffer + k * 16+1), *(buffer + k * 16+2), *(buffer + k * 16+3),  \
				*(buffer + k * 16+4), *(buffer + k * 16+5), *(buffer + k * 16+6), *(buffer + k * 16+7), *(buffer + k * 16+8), *(buffer + k * 16+9), *(buffer + k * 16+10), *(buffer + k * 16+11),  \
				*(buffer + k * 16+12), *(buffer + k * 16+13), *(buffer + k * 16+14));
	}
}
