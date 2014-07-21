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
$RCSfile: bt_power.c,v $
$Revision: 1.8 $
$Date: 2009/10/14 03:03:18 $
 **************************************************************************/

#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include hci module
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"
#include "bt_frag.h"
#include "bt_task.h"
#include "btdownload.h"
#include "bt_usbregdef.h"







VOID BtPower_SetPowerD3(PBT_DEVICE_EXT devExt, UINT8 CancelIntIrp, UINT8 Req8051MinLoop, UINT8 PowerFlag)
{
	PBT_FRAG_T  pFrag;
	PBT_TASK_T pTask;
	//PCONNECT_DEVICE_T	pConnectDevice;
	//PBT_HCI_T	pHci = NULL;
	UINT8		tmp;
	//UINT8		NeedPollDev = 0;
	LARGE_INTEGER  TimeOut;
	NTSTATUS		Status;
	//KIRQL		oldIrql;
	UINT8		subCmd;
	BOOLEAN		CancelRxOk, CancelInquiryOk, CancelIntOk;
	//UINT32		tmpLong;
	//UINT8		i;
	//PUSBD_PIPE_INFORMATION pipeInformation;

    
	CancelIntOk = FALSE;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);

	pTask = (PBT_TASK_T)devExt->pTask;
	ASSERT(pTask);

	//BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD3---entered,  %d\n", PowerDeviceD3);
	//devExt->StartStage = START_STAGE_MINLOOP;

    BT_DBGEXT(ZONE_POWER | LEVEL3,  "Suspend 0x%x\n", jiffies);
    if(devExt->InD3Flag || (FALSE == Vcmd_8051fw_isWork(devExt)))
    {
        BT_DBGEXT(ZONE_POWER | LEVEL3,  "Bad suspend\n");
        return;
    }

	//Jakio20080703: set d3 flag
	devExt->InD3Flag = 1;
	//jakio20080128: to see if there are pending events 
	//TimeOut.QuadPart = 0;
	//KeSetTimer(&devExt->IntSendingTimer, TimeOut, &devExt->IntSendingDPC);
	//BtDelay(5000); //delay 5ms
	
	//Jakio20080111: add for combo
	//delete all the link before we enter into d3 state
	
	Hci_ReleaseAllConnection(devExt);
	BtDelay(200*1000); //delay 500ms
	Hci_ResetHciCommandStatus(devExt);
	BtDelay(100*1000);
	Task_Reset_BeforeFirmwareReset(devExt);
	BtDelay(100*1000); //delay 20 ms

	//Jakio20080517: back up bluetooth reg when combo switch
	if(PowerFlag == 0)
	{
		BtBackupBlueToothRegs(devExt);
	}

	//set reset enable bit
	//VendorCmdRegRead(devExt, REG_USBCTL1, &tmp);
	BtUsb_Bus_Reset_Enable(devExt);
	VendorCmdRegRead(devExt, REG_USBCTL1, &tmp);
	BT_DBGEXT(ZONE_POWER | LEVEL3,  "value %x\n", tmp);
		
	//IoStopTimer(devExt);
	// Stop timer
	del_timer_sync(&devExt->watchDogTimer);
	tasklet_kill(&devExt->taskletRcv);
	if(pFrag)
    {   
		del_timer_sync(&pFrag->TxTimer);
    }

    // Cancel the thread work
    cancel_delayed_work(&devExt->cmd_work);
    cancel_delayed_work(&devExt->BtWorkitem);

	//Indicate 8051 that we want to do hardware reset
	//tmp  = VCMD_API_8051_JUMP_MIN_PROCESS;
	//VendorCmdRegWrite(devExt, 0x128, tmp); 
	if(Req8051MinLoop)
	{
	    INT32 remain = 0;
		subCmd = VCMD_SUB_CMD_POWER_D3;
		BtDelay(5000);
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
	
	#if 0	//jakio20080103: for combo
	//disable interrupt first
	BtDisableInterrupt(devExt);
	Vcmd_PowerSave_CardBusNICReset(devExt);
	BtDelay(200);
	
	DownStopRun8051(devExt);
	BtDelay(200);
	#endif

	// Cancel waitonmask irp
	//BtCancelWaitOnMaskIrp(devExt);
	//cancel rx,int irp
	CancelRxOk = UsbCancelRxReq(devExt);
	CancelInquiryOk = UsbCancelInquiryIrp(devExt);
	if(CancelIntIrp)
		CancelIntOk = UsbCancelInterruptReq(devExt);
	BtDelay(2000);

	/*wait int and read irp to complete*/
	if(CancelRxOk)
	{
		TimeOut.QuadPart =  HZ*10;
		Status = wait_event_interruptible_timeout(devExt->RxIdleEvent, atomic_read(&devExt->RxIdleFlag) == EVENT_SIGNED, TimeOut.QuadPart);
		if (Status == 0)
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Failed to wait rx irp\n");
		}
		atomic_set(&devExt->RxIdleFlag, EVENT_UNSIGNED);
	}
	
	if(CancelInquiryOk)
	{
		TimeOut.QuadPart =  HZ*10;
		Status = wait_event_interruptible_timeout(devExt->InqIdleEvent, atomic_read(&devExt->InqIdleFlag) == EVENT_SIGNED, TimeOut.QuadPart);
		if (Status == 0)
		{
			//timeout
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Failed to wait inquiry irp\n");
		}
		atomic_set(&devExt->InqIdleFlag, EVENT_UNSIGNED);
	}
	
	if(CancelIntOk)
	{
		TimeOut.QuadPart =  HZ*10;
		Status = wait_event_interruptible_timeout(devExt->IntIdleEvent, atomic_read(&devExt->IntIdleFlag) == EVENT_SIGNED, TimeOut.QuadPart);
		if (Status == 0)
		{
			//timeout
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Failed to wait int irp\n");
		}
		atomic_set(&devExt->IntIdleFlag, EVENT_UNSIGNED);
	}

	
	
	// Release Frag module
	Frag_Reset(devExt);

	//jakio20080108: for combo test
	//Release task module
	//Task_Reset(pTask);

	

	//reset usb resource
	//jakio20080214: should not call this function
	//Usb_ResetResource(devExt);

	//jakio20080104: for combo
	//devExt->StartStage = START_STAGE_MINLOOP;
	//jakio20080227: inform 8051 that we are ready now, used for combo state switch
	if(CancelIntIrp == 0)
	{
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_READY_BEGIN_WORK);
		BT_DBGEXT(ZONE_POWER | LEVEL3,  "MAILBOX_CMD_READY_BEGIN_WORK, BtPower_SetPowerD3 exit\n");
	}
	else{
		BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD3 exit\n");
	}
}



/************************************************************************************************
*Changelog:
*	Jakio20080514: add third para "PowerFlag" to identify the call stack of this routin: combo switch or power management
************************************************************************************************/
VOID BtPower_SetPowerD0(PBT_DEVICE_EXT devExt, UINT8 DownFwFlag, UINT8 Req8051MainLoop, UINT8 PowerFlag)
{
	PBT_FRAG_T  pFrag;
	PBT_TASK_T pTask;
	LARGE_INTEGER timevalue;
	UINT8	tmp; 
	//PUINT8	pbuf;
	BOOLEAN	poweredOff, FunctionOk = FALSE;
	NTSTATUS ntStatus;
	//BOOLEAN FlagRequested = FALSE;
	UINT32	Loop = 100;
	UINT8	Magic[4] = {0x33, 0x44, 0x53, 0x50};//stand for"3DSP"
	UINT8	tmpBuf[8];
	UINT32	RetryCount = 50;
    PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;
    
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	ASSERT(pFrag);

	pTask = (PBT_TASK_T)devExt->pTask;
	ASSERT(pTask);

    BT_DBGEXT(ZONE_POWER | LEVEL3,  "Resume 0x%x\n", jiffies);
	BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD0---entered, downFw:%d, mainLoop:%d\n",DownFwFlag,Req8051MainLoop);

    if(0 == devExt->InD3Flag){
        BT_DBGEXT(ZONE_POWER | LEVEL3,  "Not suspended, no resume\n");
        return;
    }
    
	//devExt->StartStage = START_STAGE_MAINLOOP;
	//Jakio20080821: give a chance to access hw
	devExt->DriverHaltFlag = 0;
	poweredOff = FALSE;
	__try
	{
		//delay 10 ms, for test
		BtDelay(10*1000);
		//Jakio20080612:loop three times, give chance to HW  to init
		while(RetryCount)
		{
			if(FALSE == BtIsPluggedIn(devExt))
			{
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD0---Device is removed\n");
				__leave;
			}
			if(TRUE == BtUsb_Device_Removed(devExt))
			{
				BT_DBGEXT(ZONE_POWER | LEVEL4,  "PCI  Not ready yet, retry it later, LoopNum:%lu\n", RetryCount);
				BtDelay(50*1000);//delay 1ms
			}
			else
			{
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "PCI ready\n");
				break;
			}
			RetryCount--;
			
		}
		if(RetryCount == 0)
		{
			BT_DBGEXT(ZONE_POWER | LEVEL3,  "PCI  Not ready,quit\n");
			__leave;
		}
		
		
		ntStatus = BtUsbInt(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Drop Interrupt irp failure\n");
			//__leave;
 		}
		//if(1)
		if(devExt->ComboState == FW_WORK_MODE_SINGLE)
		{
			if(FALSE == Vcmd_8051fw_isWork(devExt))
			{
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "8051 not run, download firmware now\n");
                poweredOff = TRUE;
				VendorCmdBurstRegRead2(devExt, REG_8051CR, &tmp, sizeof(UINT8));
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "Jakio1,%2x\n",tmp);
				if(STATUS_SUCCESS != DownLoad8051FirmWare(devExt))
				{
					BT_DBGEXT(ZONE_POWER | LEVEL0,  "Download 8051 firmware failure\n");
					__leave;
				}

				//tmp = VCMD_API_8051_JUMP_MIN_PROCESS;
				BtDelay(5000);
				VendorCmdBurstRegRead2(devExt, REG_8051CR, &tmp, sizeof(UINT8));
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "Jakio2,%2x\n",tmp);
				//jakio20080521: check 0XC0 to verify whether 8051 has run
				while(((tmp&0x02) == 0) && Loop)
				{
					Loop--;
					if(STATUS_SUCCESS != DownLoad8051FirmWare(devExt))
					{
						BT_DBGEXT(ZONE_POWER | LEVEL0,  "Download 8051 firmware failure 1\n");
						//__leave;
					}

					BtDelay(5000);
					VendorCmdBurstRegRead2(devExt, REG_8051CR, &tmp, sizeof(UINT8));
					BT_DBGEXT(ZONE_POWER | LEVEL3,  "Jakio3,Loop Num:%lu,%u\n",Loop, tmp);
					//jakio20080624:
					if(FALSE == BtIsPluggedIn(devExt))
					{
						BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD0---device is removed, quit\n");
						__leave;
					}
				}
				if(Loop == 0)
				{
					BT_DBGEXT(ZONE_POWER | LEVEL0,  "Start 8051 failure,quit\n");
					__leave;
				}
			
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "Download 8051 success\n");
				BtDisableInterrupt(devExt);
				if(STATUS_SUCCESS != Vcmd_PowerSave_CardBusNICReset(devExt))
				{
					BT_DBGEXT(ZONE_POWER | LEVEL0,  "Cardbus nic reset failure\n");
					__leave;
				}

				if(STATUS_SUCCESS != DownLoad3DspFirmWare(devExt, FALSE))//Jakio20080421: request goto mainloop later
				{
					BT_DBGEXT(ZONE_POWER | LEVEL0,  "Cardbus nic reset failure\n");
					__leave;
				}

				//set flag, we have requested 8051 goto main loop after download dsp firmware
				//FlagRequested = TRUE;

				devExt->StartStage = START_STAGE_MAINLOOP;
				
				BtEnableInterrupt(devExt);
			}
			else
			{
				if(DownFwFlag)
				{
					BT_DBGEXT(ZONE_POWER | LEVEL1,  "Need DownLoad DSP code\n");
					DownLoadFirmWare(devExt, 1, FALSE);
				}
			}
		}
		else  //if combo state, wait until wlan download ok
		{
			//Jakio20080514: do this only after restoring from D3
			if(PowerFlag)
			{
                Loop = 50;
				BtDelay(100*1000);//delay 100ms
				while(FALSE == Vcmd_8051fw_isWork(devExt))
				{
				    Loop--;
					//delay 10 ms
					BtDelay(10*1000);
					//jakio20080529:judge device state
					if(FALSE == BtIsPluggedIn(devExt) || 0 == Loop)
					{
						BT_DBGEXT(ZONE_POWER | LEVEL1,  "BtPower_SetPowerd0---Device is removed1\n");
						__leave;
					}
				}
				//Jakio20080523:check whether 8051 has completed initialization
				VendorCmdBurstRegRead2(devExt, REG_InitCompleted, tmpBuf, 4);
				//BT_DBGEXT(ZONE_POWER | LEVEL3,  "%2x %2x %2x %2x\n",tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3]);
				//Check for 2 seconds
				Loop = 2000;
				while(!RtlEqualMemory(Magic, tmpBuf, 4))
				{
				    Loop--;
					BtDelay(1000);	
					VendorCmdBurstRegRead2(devExt, REG_InitCompleted, tmpBuf, 4);
					//BT_DBGEXT(ZONE_POWER | LEVEL3,  "%2x %2x %2x %2x\n",tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3]);
					if(FALSE == BtIsPluggedIn(devExt) || 0 == Loop)
					{
						BT_DBGEXT(ZONE_POWER | LEVEL1,  "BtPower_SetPowerd0---Device is removed2\n");
						__leave;
					}
				}
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD0----8051 init completed\n");
				

				//set flag and request join
				devExt->PsComboReqFlag = TRUE;
				if(STATUS_SUCCESS !=VendorCmdWriteCmdToMailBox(devExt, &devExt->ComboState, 
						MAILBOX_CMD_REQUEST_TO_JOIN))
				{	
					BT_DBGEXT(ZONE_POWER | LEVEL0,  "BtPower_SetPowerD0---Error, send join command failed\n");
					__leave;
				}
				//wait combo ready event, wait 10s
				atomic_set(&devExt->ComboReadyFlag, EVENT_UNSIGNED);
				if(0 == wait_event_interruptible_timeout(devExt->ComboReadyEvent, 
                                                         atomic_read(&devExt->ComboReadyFlag) == EVENT_SIGNED, 
                                                         HZ*10))
				{
					BT_DBGEXT(ZONE_POWER | LEVEL1,  "BtPower_SetPowerD0----Error, wait combo ready event timeout\n");
				}	
			}
			else
			{
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerD0---begin work\n");
			}
			
		}
	
		ntStatus = BtUsbSubmitReadIrp(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Drop read irp  failure\n");
			__leave;
		}
		ntStatus = BtUsbSubmitIquiryIrp(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Drop read irp  failure\n");
			__leave;
		}

		//Jakio20080709: when wlan request 8051 goto mainloop, 8051 may busy initializing something, delay a while
		if(devExt->ComboState == FW_WORK_MODE_COMBO)
			BtDelay(50*1000);

		//Jakio20080716: write and then read, to make it is correct
		UsbWriteTo3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, 2, ((PBT_HCI_T)devExt->pHci)->local_bd_addr);
		Loop = 100;
		while(--Loop)
		{
			UsbReadFrom3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, 2, tmpBuf);
			//compare
			if(RtlEqualMemory(((PBT_HCI_T)devExt->pHci)->local_bd_addr, tmpBuf, BT_BD_ADDR_LENGTH))
			{
				BT_DBGEXT(ZONE_POWER | LEVEL3,  "Write local bd addr ok, loop number is %lu\n",Loop);
				break;
			}
			//else, write again
			UsbWriteTo3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, 2, ((PBT_HCI_T)devExt->pHci)->local_bd_addr);
			if(FALSE == BtIsPluggedIn(devExt))
			{
				BT_DBGEXT(ZONE_POWER | LEVEL1,  "BtPower_SetPowerd0---Device is removed3\n");
				__leave;
			}
		}
		if(Loop == 0)
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "Error, verify bd addr failed\n");
			__leave;
		}
		
		if(STATUS_SUCCESS !=BtInitHw(devExt))
		{
			BT_DBGEXT(ZONE_POWER | LEVEL0,  "BtPower_SetPowerD0--Error, init hw failed\n");
			//Jakio20080709:check device state
			if(FALSE == BtIsPluggedIn(devExt))
			{
				__leave;
			}
		}
        
        PostInit(devExt);
        /******Write Page timeout**/
    	Hci_Write_One_Word(devExt, BT_REG_PAGE_TIMEOUT, pHci->page_timeout);
        
        /***** Device Class **/
        NotifyDspDevClass(devExt);

        /** Scan Enable*/
        NotifyDspScanEnable(devExt);

		BtClearIvtData(devExt);
		//Jakio20080517: restore bluetooth registers
		if(STATUS_SUCCESS !=BtRestoreBlueToothRegs(devExt))
		{
			//Jakio20080709:check device state
			if(FALSE == BtIsPluggedIn(devExt))
			{
				__leave;
			}
		}

		//jakio20080523: init for combo state
		if(devExt->ComboState == FW_WORK_MODE_COMBO)
		{
			BT_DBGEXT(ZONE_POWER | LEVEL3,  "init vars for combo mode\n");
			Hci_ComboStateInit(devExt);
		}
		else
		{
			BT_DBGEXT(ZONE_POWER | LEVEL3,  "init vars for bt only mode\n");
			Hci_SingleStateInit(devExt);
		}
		
		//IoStartTimer(devExt);
		//UsbWriteTo3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, 2, ((PBT_HCI_T)devExt->pHci)->local_bd_addr);

		if(Req8051MainLoop)
		{
			BT_DBGEXT(ZONE_POWER | LEVEL3,  "Request 8051 goto MainLoop\n");
			//retry this cmd 2 times at most
			Loop = 2;
			while(--Loop)
			{
				ntStatus = VendorCmdWriteCmdToMailBox(devExt, NULL, VCMD_API_8051_JUMP_MAIN_PROCESS);
				if(ntStatus != STATUS_SUCCESS)
				{
    				BT_DBGEXT(ZONE_POWER | LEVEL0,  "Inform 8051 goto main loop failure\n");
    				//Jakio20080709:check device state
    				if(FALSE == BtIsPluggedIn(devExt))
    				{
    					__leave;
    				}
				}
				else	
				{
					ntStatus = wait_event_interruptible_timeout(devExt->MainLoopEvent, atomic_read(&devExt->mainLoopFlag) == EVENT_SIGNED, 10*HZ);
                    atomic_set(&devExt->mainLoopFlag, EVENT_UNSIGNED);
                    if(0 == ntStatus){
                        BT_DBGEXT(ZONE_DLD | LEVEL0, "Dsp firmware download TIMEOUT\n");
                    }
                    else{
						BT_DBGEXT(ZONE_DLD | LEVEL1, "DownLoad3DspFirmWare---wait MainLoop event ok, Loop count: %d\n", Loop);
						break;
                    }
				}
			}
		}

		//Jakio20080627:start timer at last
		//if someone unplug card when restoring from D3, then we should not start timer
		//and driver will quit without go here
		//IoStartTimer(devExt);
		
		// Start Hci command processing
		queue_delayed_work(devExt->pBtWorkqueue, &devExt->cmd_work, 0);
		devExt->watchDogTimer.expires = (unsigned long)jiffies + HZ/2;
		add_timer(&devExt->watchDogTimer);

		FunctionOk = TRUE;
	}
	__finally
	{
		//if there are something to do here?
	}

	if(FunctionOk)
	{
		//Jakio20080703: reset d3 flag
		devExt->InD3Flag = 0;
		BT_DBGEXT(ZONE_POWER | LEVEL3,  "BtPower_SetPowerd0 success\n");
	}
}

