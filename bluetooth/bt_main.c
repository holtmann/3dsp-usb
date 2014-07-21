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
$RCSfile: bt_main.c,v $
$Revision: 1.34 $
$Date: 2010/12/10 03:07:01 $
 **************************************************************************/



#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"
#include "bt_task.h"
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_frag.h"			
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
#include "afhclassify.h"
#endif
#ifdef BT_SCHEDULER_SUPPORT
#include "sched.h"
#endif
#include "tdsp_bus.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
#include <linux/usb/quirks.h>
#endif

#define VERSION "LinuxBtUsb--0.01"

/* Globals */
unsigned int dbg_zone = (ZONE_MASK);
unsigned int dbg_level = (LEVEL_MASK);

// module param
unsigned int combodrv = 1;
//unsigned int intr=1;

module_param(combodrv, uint, S_IRUGO);
//module_param(intr, uint, S_IRUGO);

/* Statics */
static struct usb_driver hci_usb_driver; 
static struct usb_device_id bluetooth_ids[] = {
	/* 3dsp usb bluetooth */
	{ USB_DEVICE(0x05e1, 0x0100) },
	{ }	/* Terminating entry */
};


MODULE_DEVICE_TABLE (usb, bluetooth_ids);


/* For code coverage */
struct code_coverage{
    unsigned char hci_event[TOTAL_HCI_EVENTS];
    unsigned char lc_cmd[TOTAL_LINK_CTRL_CMD];
    unsigned char lp_cmd[TOTAL_LINK_POLICY_CMD];
    unsigned char bb_cmd[TOTAL_BASEBAND_CMD];
    unsigned char info_cmd[TOTAL_INFO_CMD];
    unsigned char status_cmd[TOTAL_STATUS_CMD];
};

/* code coverage statistic */
struct code_coverage *cc_sta;

// Diag work handler
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
void DiagWorkFunc(struct work_struct *pwork)
#else
void DiagWorkFunc(PVOID pvoid)
#endif
{
	PBT_DEVICE_EXT devExt;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)		
	devExt = (PBT_DEVICE_EXT)container_of(pwork, BT_DEVICE_EXT, diagWork);
#else
	devExt =(PBT_DEVICE_EXT)pvoid;
#endif
	devExt->diagFlag = 1;
	wake_up_interruptible(&devExt->diagEvent);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
// Diagnose the system
static void DiagSys(PBT_DEVICE_EXT	devExt)
{
    KIRQL oldIrql;
    KSPIN_LOCK *pLock;
    struct work_struct *pDiagWork;
    int ret, to;

    BT_INFOEXT("----------------- DIAGNOSE -----------------\n");
    BT_INFOEXT("Spinlock:\n");

    // Check Hci Lock
    pLock = &((PBT_HCI_T)devExt->pHci)->HciLock;
    BT_INFOEXT("Hci Lock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check Frag Tx Lock
    pLock = &((PBT_FRAG_T)devExt->pFrag)->FragTxLock;
    BT_INFOEXT("Frag Tx Lock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check read queue Lock
    pLock = &devExt->ReadQueueLock;
    BT_INFOEXT("Dev ReadQueueLock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT( "[ OK ]\n");
    
    // Check rx state Lock
    pLock = &devExt->RxStateLock;
    BT_INFOEXT("Dev RxStateLock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check TxSpinLock
    pLock = &devExt->TxSpinLock;
    BT_INFOEXT("Dev TxSpinLock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check AsynCmdLock
    pLock = &devExt->AsynCmdLock;
    BT_INFOEXT("Dev AsynCmdLock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check Task lock
    pLock = &((PBT_TASK_T)devExt->pTask)->lock;
    BT_INFOEXT("Task Lock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    // Check Schedule lock
    pLock = &((PBT_SCHED_T)devExt->pSched)->lock;
    BT_INFOEXT("Schedule Lock: ");
    KeAcquireSpinLock(pLock, &oldIrql);
    KeReleaseSpinLock(pLock, oldIrql);
    BT_INFOEXT("[ OK ]\n");

    BT_INFOEXT("Work Queue:\n");
    BT_INFOEXT("Command Queue: ");
    queue_work(devExt->pBtWorkqueue, &devExt->diagWork);
    // Maximal wait 20 HZ
    ret = wait_event_interruptible_timeout(devExt->diagEvent, devExt->diagFlag == 1, msecs_to_jiffies(20*HZ));
    if(0 == ret){
        BT_INFOEXT("[ FAIL ]\n");
    }
    else{
        BT_INFOEXT("[ OK ]\n");
    }
    devExt->diagFlag = 0;
    BT_INFOEXT("----------------- END -----------------\n");
}
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
/* Device IOCTL */
static int hci_usb_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg)
{
	struct dbg_dsp_reg *d_reg = NULL;
	struct dbg_usb_reg *u_reg = NULL;
	struct code_coverage *cc_tmp;
	int errcode = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	PBT_DEVICE_EXT	devExt = hci_get_drvdata(hdev);
#else
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)hdev->driver_data;
#endif
	
	if (!test_bit(HCI_RUNNING, &hdev->flags))
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "%s NOT running\n", hdev->name);
		return -1;
	}
    switch(cmd){
    case HCISETDBGZONE:
        printk(KERN_INFO "Debug zone 0x%04x-->0x%04x, debug level 0x%04x-->0x%04x\n", 
            dbg_zone, (arg & 0xFFFF0000), dbg_level, arg & 0xFFFF);
        
        dbg_zone = arg & (0xFFFF << 16);    
        dbg_level = arg & 0xFFFF;
        
        break;
    case HCICODECOVERAGE:
        BT_DBGEXT(ZONE_MAIN | LEVEL1, "Dump the code coverage\n");
        cc_tmp = (struct code_coverage *)arg;
        if(!cc_tmp){
            BT_DBGEXT(ZONE_MAIN | LEVEL0, "Illegle cc dump buffer\n");
            return -EFAULT;
        }
        copy_to_user((char __user *)cc_tmp, cc_sta, sizeof(*cc_sta));
        
        break;
    case IOCTL_READ_DSP_REG:
    {
			d_reg = (struct dbg_dsp_reg *)kzalloc(sizeof(struct dbg_dsp_reg), GFP_KERNEL);
			if(d_reg == NULL)
			{
				errcode = -EFAULT;
				break;
			}
			if(copy_from_user(d_reg, (char __user*)arg, sizeof(struct dbg_dsp_reg)))
			{
				errcode = -EFAULT;
				break;
			}
			BT_DBGEXT(ZONE_MAIN | LEVEL3, "Read DSP Register Offset 0x%x, Length %d\n", d_reg->offset, d_reg->len);
			if(STATUS_SUCCESS != UsbReadFrom3DspRegs(devExt, d_reg->offset, d_reg->len, d_reg->out)){
				BT_DBGEXT(ZONE_MAIN | LEVEL0, "Read DSP register failure!!\n");
				errcode = -ENODEV;
				break;
			}
			else
			{
				if(copy_to_user((char __user*)arg, d_reg, sizeof(struct dbg_dsp_reg)))
				{
					errcode = -EFAULT;
				}
			}
			break;            
    }
	case IOCTL_WRITE_DSP_REG:
		{
			d_reg = (struct dbg_dsp_reg *)kzalloc(sizeof(struct dbg_dsp_reg), GFP_KERNEL);
			if(d_reg == NULL)
			{
				errcode = -EFAULT;
				break;
			}
			
			if(copy_from_user(d_reg, (char __user*)arg, sizeof(struct dbg_dsp_reg)))
			{
				errcode = -EFAULT;
				break;
			}
			
			if(STATUS_SUCCESS != UsbWriteTo3DspRegs(devExt, d_reg->offset, d_reg->len, d_reg->out))
			{
				BT_DBGEXT(ZONE_MAIN | LEVEL0, "%Write DSP register failure!!\n");
				errcode = -ENODEV;
				break;
			}
			break;
		}
    case IOCTL_READ_USB_REG:
    {
			u_reg = (struct dbg_usb_reg *)kzalloc(sizeof(struct dbg_usb_reg), GFP_KERNEL);
			if(u_reg == NULL)
			{
				errcode = -EFAULT;
				break;
			}
			if(copy_from_user(u_reg, (char __user*)arg, sizeof(struct dbg_usb_reg)))
			{
				errcode = -EFAULT;
				break;
			}
			BT_DBGEXT(ZONE_MAIN | LEVEL3, "Read USB Register Addr 0x%x, Length %d\n", u_reg->addr, u_reg->len);        
			if(STATUS_SUCCESS != VendorCmdBurstRegRead2(devExt, u_reg->addr, u_reg->out, u_reg->len))
			{
				BT_DBGEXT(ZONE_MAIN | LEVEL0, "Read USB register failure!!\n");
				errcode = -ENODEV;
				break;
			}
			else
			{
				if(copy_to_user((char __user*)arg, u_reg, sizeof(struct dbg_usb_reg)))
				{
					errcode = -EFAULT;
				}
			}
			break;
    }
    case IOCTL_WRITE_USB_REG:
		{
			u_reg = (struct dbg_usb_reg *)kzalloc(sizeof(struct dbg_usb_reg), GFP_KERNEL);
			if(u_reg == NULL)
			{
				errcode = -EFAULT;
				break;
			}
			if(copy_from_user(u_reg, (char __user*)arg, sizeof(struct dbg_usb_reg)))
			{
				errcode = -EFAULT;
				break;
			}
			BT_DBGEXT(ZONE_MAIN | LEVEL3, "Write USB Register Addr 0x%x, Value 0x%x\n", u_reg->addr, u_reg->len);        
			if(STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, u_reg->addr, &u_reg->len, sizeof(UINT8)))
			{
				BT_DBGEXT(ZONE_MAIN | LEVEL0, "Write USB register failure !!\n");
				errcode = -ENODEV;
			}
        break;
    }
    case IOCTL_DUMP_LMP_PKT:
    {
        struct sk_buff *skb;
        struct lmp_dump *pOut;
        
        pOut = (struct lmp_dump *)arg;
        if(skb_queue_empty(&devExt->debug.lmpDumpQ)){
            BT_DBGEXT(ZONE_MAIN | LEVEL4, "Wait Lmp Packet......\n");
        	wait_event_interruptible(devExt->debug.lmpDumpEvt,
                                     devExt->debug.lmpEvtFlag != 0);
            devExt->debug.lmpEvtFlag = 0;
        }

        // Copy to application
        skb = skb_dequeue(&devExt->debug.lmpDumpQ);
        if(skb){
            pOut = (struct lmp_dump *)skb->data;
            if(copy_to_user((char __user*)arg, pOut, sizeof(struct lmp_dump)));

            kfree_skb(skb);
        }
        
        break;
    }

    case IOCTL_SYS_DIAG:
    {
        // Do system diagnose
        DiagSys(devExt);
        break;
    }

    case IOCTL_DBG_CRASH:
    {
        BT_DBGEXT(ZONE_MAIN | LEVEL4, "Crash the system for debug\n");
        BT_CRASH();
        break;
    }
    
    default:
        BT_DBGEXT(ZONE_MAIN | LEVEL0, "Unknown debug command\n");
    }
    
	if(d_reg)
		kfree(d_reg);
	if(u_reg)
		kfree(u_reg);
	return errcode;
}
#endif

static int OpenDev(PBT_DEVICE_EXT devExt)
{
	int err = 0;
    INT32 remain;

    // Start the device
    devExt->State = STATE_STARTED;
	if(STATUS_SUCCESS != BtStartDevice(devExt)){
        BT_DBGEXT(ZONE_MAIN | LEVEL0, "Device start failure\n");    
        return -ENODEV;
    }
    remain = wait_event_interruptible_timeout(devExt->DevRdyEvt,
	                                          atomic_read(&devExt->devRdyFlag) == EVENT_SIGNED,
	                                          300 * HZ);
    atomic_set(&devExt->devRdyFlag, EVENT_UNSIGNED);
    if((0 == remain) || (0 == devExt->devRdyOk)){
        BT_DBGEXT(ZONE_DLD | LEVEL0, "Wait Dev Ready Fails\n");
        if(devExt->phase2){
            // Release the workqueue and related
            ReleasePreStart(devExt);
        }
        return -ENODEV;
    }

	//Init timers
	BtInitializeTimer(devExt);

	//Add watch dog timer. 1 second.
	devExt->watchDogTimer.expires = (unsigned long)jiffies + HZ;
	add_timer(&devExt->watchDogTimer);
    
    return 0;
}

/* Initialize device */
static int hci_usb_open(struct hci_dev *hdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	PBT_DEVICE_EXT	devExt = hci_get_drvdata(hdev);
#else
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)hdev->driver_data;
#endif

	if (test_and_set_bit(HCI_RUNNING, &hdev->flags))
	{   
		return 0;
	}

    BT_DBGEXT(ZONE_MAIN | LEVEL3, "devExt 0x%x,devExt->usb_device 0x%x, hdev 0x%x\n", devExt,devExt->usb_device, hdev);

    // To avoid multiple opens by different threads
    if(devExt->isOpen)
    {
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "Device %s is already OPEN\n", hdev->name);    
        return -EIO;
    }

    devExt->isOpen = 1;
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "Try to be MASTER aggressively\n");
    hdev->link_mode |= HCI_LM_MASTER;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	PostInit(hci_get_drvdata(hdev));
#else
	PostInit(hdev->driver_data);
#endif
	
    
    if(combodrv){
        // push up the firmware version
        Bus_set_firmware_version(devExt->pBusInterface, 
                                *((UINT32 *)devExt->Version8051), 
                                *((UINT32 *)devExt->VersionDSP));
    }

	BT_DBGEXT(ZONE_MAIN | LEVEL3, "open device %s----\n", hdev->name);
	return 0;
}

// After CloseDev, we should reset some status
static void ResetDevCtx(PBT_DEVICE_EXT devExt)
{
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

    // Refer to BtAddDevice to set these flags
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
	devExt->DriverHaltFlag = 0;
	devExt->FakeD3State = 0;
	devExt->InD3Flag = 0;
	devExt->WorkItemRun = FALSE;
}

static int CloseDev(PBT_DEVICE_EXT devExt)
{
	// Stop Watchdog timer
    BT_DBGEXT(ZONE_PNP | LEVEL3,"Release Watchdog\n");
	del_timer_sync(&devExt->watchDogTimer);

    // Release Resource
	BtReleaseResources(devExt);
    ResetDevCtx(devExt);
    devExt->State = STATE_STOPPED;
    return 0;
}

/* Close device */
static int hci_usb_close(struct hci_dev *hdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	PBT_DEVICE_EXT	devExt = hci_get_drvdata(hdev);
#else
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)hdev->driver_data;
#endif
	
    ENTER_FUNC();
	if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags)){
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "%s NOT Running\n", hdev->name);    
        return 0;
    }

    if(!devExt->isOpen)
    {
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "%s NOT Opened\n", hdev->name);    
        goto end;
    }
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "close %s\n", hdev->name);

    // Exit Lmp Dump
	devExt->debug.lmpEvtFlag = 1;
	wake_up_interruptible(&devExt->debug.lmpDumpEvt);

end:
    devExt->isOpen = 0;
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "End close\n", hdev->name);

    EXIT_FUNC();
	return 0;
}


/* Flush command queue */
static int hci_usb_flush(struct hci_dev *hdev)
{
	//int i;
    //PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)hdev->driver_data;

    ENTER_FUNC();
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "hci_usb_flush %s\n", hdev->name);

    EXIT_FUNC();
	return 0;
}


/* Do code coverage statistic */
static void do_cc(struct sk_buff *skb)
{
    PHCI_COMMAND_HEADER_T cmd_head;
    PHCI_EVENT_HEADER_T evt_head;

    ENTER_FUNC();
    
    switch(bt_cb(skb)->pkt_type){
    case HCI_EVENT_PKT:
        evt_head = (PHCI_EVENT_HEADER_T)skb->data;
        cc_sta->hci_event[evt_head->eventcode]++;
        
        break;
    case HCI_COMMAND_PKT:
    {
        cmd_head = (PHCI_COMMAND_HEADER_T)skb->data;
        switch(cmd_head->opcode.ogf){
        case BT_HCI_COMMAND_OGF_LINK_CONTROL:
            cc_sta->lc_cmd[cmd_head->opcode.ocf]++;
            
            break;
        case BT_HCI_COMMAND_OGF_LINK_POLICY:
            cc_sta->lp_cmd[cmd_head->opcode.ocf]++;
            
            break;
        case BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND:
            cc_sta->bb_cmd[cmd_head->opcode.ocf]++;
            
            break;
        case BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS:
            cc_sta->info_cmd[cmd_head->opcode.ocf]++;
            
            break;
        case BT_HCI_COMMAND_OGF_STATUS_PARAMETERS:
            cc_sta->status_cmd[cmd_head->opcode.ocf]++;
            
            break;
        default:
            BT_DBGEXT(ZONE_MAIN | LEVEL0, "Exception OGF!!\n");
        }
        break;
    }
    case HCI_SCODATA_PKT:
    case HCI_ACLDATA_PKT:
        BT_DBGEXT(ZONE_MAIN | LEVEL3, "Data packet\n");
        break;
    default:
        BT_DBGEXT(ZONE_MAIN | LEVEL0, "Exception!!\n");
    }

    EXIT_FUNC();
}


/* 
 * Send frames from HCI layer
 * @skb: the frame to send
 *
 * Return the send status, 0 for success. Actually the return status
 * is never used in the BlueZ stack, so it's not important
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
static int hci_usb_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
#else
static int hci_usb_send_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;
#endif
    int retval = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	PBT_DEVICE_EXT	devExt = hci_get_drvdata(hdev);
#else
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)hdev->driver_data;
#endif
    //BT_DBGEXT(ZONE_MAIN | LEVEL3, "send frame++++");
    
	if (!hdev) {
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "frame for uknown device (hdev=NULL)\n");
		return -ENODEV;
	}

	if (!test_bit(HCI_RUNNING, &hdev->flags)){
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "HCI Not Running\n");
		return -EBUSY;
    }

    if(!devExt->isOpen)
    {
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "Device %s is NOT OPEN\n", hdev->name);    
        return -EIO;
    }

    if(devExt->InD3Flag)
    {
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "Device %s in D3\n", hdev->name);    
        return -EBUSY;
    }
    
	//BT_DBGEXT(ZONE_MAIN | LEVEL3, "send frame to %s, type %d, len %d", hdev->name, bt_cb(skb)->pkt_type, skb->len);

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		hdev->stat.cmd_tx++;
        BT_DBGEXT(ZONE_MAIN | LEVEL3, "Command packet received\n");

        /* Code coverage operation */
        do_cc(skb);
        
        skb_queue_tail(&devExt->cmd_queue, skb);
        queue_delayed_work(devExt->pBtWorkqueue, &devExt->cmd_work, 0);
        /* Notify the command processing thread */
#if 0        
        devExt->cmd_wait_flag = 1;
        wake_up_interruptible(&devExt->cmd_wait);
#endif        
        
		break;
	case HCI_ACLDATA_PKT:
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "ACL packet received\n");
        hdev->stat.acl_tx++;
        Frag_BuildFrag(devExt, skb);
        /* The data has been buffered in Frag memory*/
        kfree_skb(skb);

        break;
	case HCI_SCODATA_PKT:
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "SCO packet received\n");
        hdev->stat.sco_tx++;
        Frag_BuildFrag(devExt, skb);
        /* The data has been buffered in Frag memory*/
        kfree_skb(skb);

		break;
	default:
        BT_DBGEXT(ZONE_MAIN | LEVEL0, "Unknow HCI packet received\n");
		kfree_skb(skb);
        return 0;
	}

    BT_DBGEXT(ZONE_MAIN | LEVEL3, "send frame return %d\n", retval);
    return retval;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 9)
static void hci_usb_destruct(struct hci_dev *hdev)
{
    ENTER_FUNC();
	BT_DBGEXT(ZONE_MAIN | LEVEL2, "hci_usb_destruct %s\n", hdev->name);
   
    EXIT_FUNC();
}
#endif

static void hci_usb_notify(struct hci_dev *hdev, unsigned int evt)
{
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "hci_usb_notify %s evt %d\n", hdev->name, evt);
}

void unregister_HCI_device(void *dev)
{
	struct hci_dev *hdev = NULL;
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)dev;

    down_interruptible(&devExt->openLock);
	if (devExt == NULL)
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "%s, devExt = NULL\n", __FUNCTION__);
		goto end;
	}

	hdev = devExt->hdev;
	if (hdev == NULL)
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "%s, devExt->hdev = NULL\n", __FUNCTION__);
		goto end;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	hci_unregister_dev(hdev);
#else
	if (hci_unregister_dev(hdev) < 0)
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "Can't unregister HCI device %s\n", hdev->name);
#endif

    // Close Dev
    CloseDev(devExt);

    // Free hdev
	hci_free_dev(hdev);	
	devExt->hdev = NULL; 
    
end:
    up(&devExt->openLock);
	BT_DBGEXT(ZONE_MAIN | LEVEL0, "unregister_HCI_device OK\n");	
}

void *register_HCI_device(void *dev)
{
	struct hci_dev *hdev = NULL;
	PBT_DEVICE_EXT	devExt = (PBT_DEVICE_EXT)dev;

    down_interruptible(&devExt->openLock);
    if(0 != OpenDev(devExt)){
        goto end;
    }
    
	/* Initialize and register HCI device */
	hdev = hci_alloc_dev();
	if (!hdev) {
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "Can't allocate HCI device\n");
        goto end;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
	hdev->dev_type = HCI_BREDR;
	hdev->bus  = HCI_USB; //add by lewis.wang
#else
	hdev->type = HCI_USB; //change by lewis.wang
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 9)	
	hci_set_drvdata(hdev, devExt);
#else
	hdev->driver_data = devExt;
#endif

	SET_HCIDEV_DEV(hdev, &devExt->intf->dev);
	hdev->open     = hci_usb_open;
	hdev->close    = hci_usb_close;
	hdev->flush    = hci_usb_flush;
	hdev->send     = hci_usb_send_frame;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 9)		
	hdev->destruct = hci_usb_destruct;
	hdev->owner = THIS_MODULE;
#endif
	hdev->notify   = hci_usb_notify;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
	hdev->ioctl    = hci_usb_ioctl;
#endif
    
   	devExt->hdev = hdev;

	if (hci_register_dev(devExt->hdev) < 0) {
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "Can't register HCI device\n");
	        hci_free_dev(hdev);
        goto end;
	}
	BT_DBGEXT(ZONE_MAIN | LEVEL0, "register_HCI_device End %p\n", devExt->hdev);

end:    
    up(&devExt->openLock);
 	return hdev;
}
	

static int hci_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_endpoint  *ep;
	struct usb_host_interface *uif;
	PBT_DEVICE_EXT	devExt = NULL;
    //INT32 remain;
	struct hci_dev *hdev = NULL;
	int e;

	BT_DBGEXT(ZONE_MAIN | LEVEL3, "udev %p intf %p\n", udev, intf);
   
	if (!id->driver_info) 
	{
		const struct usb_device_id *match;
		match = usb_match_id(intf, bluetooth_ids);
		if (match)
			id = match;
	}

	if (intf->cur_altsetting->desc.bInterfaceNumber != TDSP_BT_INTERFACE_NUM)
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "intf %d is not 3dsp bluetooth interface\n", 
		intf->cur_altsetting->desc.bInterfaceNumber);
		return -ENODEV;
	}

	/* Find endpoints that we need */
	uif = intf->cur_altsetting;
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "3dsp usb bluetooth device found:) intf %d, alt configs %d, endpoints %d\n", 
		uif->desc.bInterfaceNumber, intf->num_altsetting, uif->desc.bNumEndpoints);

	if (uif->desc.bNumEndpoints < EP_COUNT) 
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "endpoints not enough!!\n");
		goto done;
	}

	/* Only for debug, print out the endpoint informations */
	for (e = 0; e < uif->desc.bNumEndpoints; e++) 
	{
		ep = &uif->endpoint[e];
		BT_DBGEXT(ZONE_MAIN | LEVEL3, "ep addr %d, type %d, in/out %s\n", 
				ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK,
				ep->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK,
				ep->desc.bEndpointAddress & USB_DIR_IN ? " IN" : "OUT");
	}

    /* For code coverage statistic */
    cc_sta = kzalloc(sizeof(struct code_coverage), GFP_KERNEL);
    BT_DBGEXT(ZONE_MAIN | LEVEL0, "sizeof(BT_DEVICE_EXT) %d sizeof(SCO_CONNECT_DEVICE_T) 0x%x 0x%x\n", sizeof(BT_DEVICE_EXT), sizeof(SCO_CONNECT_DEVICE_T), &((PSCO_CONNECT_DEVICE_T)0)->NeedSendScoNull);
	devExt = kzalloc(sizeof(BT_DEVICE_EXT), GFP_KERNEL);
	if(devExt == NULL || cc_sta == NULL)
	{
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "Can't allocate: control structure\n");
		goto done;
	}

	BtAddDevice(devExt);
    
	/* Hard code to get the ep configuration according HW setting */
	devExt->usb_device = udev;
	devExt->intf = intf;
	devExt->UsbContext.EP_control = INDEX_EP_CTRL;
	devExt->UsbContext.EP_bulkout_ACL = INDEX_EP_BULK_OUT_ACL;
	devExt->UsbContext.EP_bulkout_SCO = INDEX_EP_BULK_OUT_SCO;
	devExt->UsbContext.EP_bulkin_Inquiry = INDEX_EP_BULK_IN_INQ;
	devExt->UsbContext.EP_bulkin_Rx = INDEX_EP_BULK_IN_DATA;
	devExt->UsbContext.EP_interrupt = INDEX_EP_INTR;

    //udev->quirks |= USB_QUIRK_RESET_RESUME;
	usb_set_intfdata(intf, devExt);

    if(combodrv){
	    devExt->pBusInterface = Bus_new_interface((void *) devExt);
    	if (NULL == devExt->pBusInterface)
    	{
    		BT_DBGEXT(ZONE_MAIN| LEVEL0, "%s: register BT USB driver to BUS Driver failed!! 1 \n", __FUNCTION__ );	
    		 goto probe_error;	
    	}
    	if (0 != Bus_openInterface(devExt->pBusInterface))
    	{
    		devExt->pBusInterface = NULL;
        		BT_DBGEXT(ZONE_MAIN| LEVEL0, "%s: register BT USB driver to BUS Driver failed!! 2\n", __FUNCTION__ );	
        		 goto probe_error;	
    	}
    }
    // If bluetooth only mode, register HCI
    if(0 == combodrv)
    {
    	if (!(hdev = register_HCI_device(devExt)))
    	{
    		BT_DBGEXT(ZONE_MAIN| LEVEL0, "%s: register BT USB driver to BUS Driver failed!!\n", __FUNCTION__ );	
    		 goto probe_error;	
    	}
    }

    BT_DBGEXT(ZONE_MAIN | LEVEL3, "devExt 0x%x, hdev 0x%x\n", devExt, devExt->hdev);
	return 0;

probe_error:
    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Error in probe\n");
	if(devExt)
		kfree(devExt);
	if(devExt->hdev)
		hci_free_dev(devExt->hdev);
    if(cc_sta)
        kfree(cc_sta);
done:
	return -EIO;
}


// TODO: rewrite this routine
static void hci_usb_disconnect(struct usb_interface *intf)
{
	PBT_DEVICE_EXT	devExt = NULL; 
	struct hci_dev *hdev = NULL;

    ENTER_FUNC();
	devExt = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);
	hdev = devExt->hdev;

    // hdev is not registered yet
    if(!hdev){
        BT_DBGEXT(ZONE_MAIN | LEVEL3, "NO hdev registered\n");
    }
    else{
    	BT_DBGEXT(ZONE_MAIN | LEVEL3, "Disconnect %s\n", hdev->name);
        down_interruptible(&devExt->openLock);
        hci_usb_close(hdev);
        up(&devExt->openLock);
    }

    // Notify the bus driver
    if(combodrv)
    {
    	if (0 != Bus_closeInterface(devExt->pBusInterface))
    	{
    		BT_DBGEXT(ZONE_MAIN| LEVEL2, "%s: register BT USB driver to BUS Driver failed!!\n", __FUNCTION__ );	
    	}
	    devExt->pBusInterface = NULL;
    }
    else
    {
    	unregister_HCI_device((void *) devExt);
    }

	if(devExt){
        BT_DBGEXT(ZONE_MAIN | LEVEL3, "Free Device Context\n");
		kfree((PVOID)devExt);
    }
    if(cc_sta){
        BT_DBGEXT(ZONE_MAIN | LEVEL3, "Free Code Coverage statistic\n");
        kfree(cc_sta);
    }
    
    EXIT_FUNC();	
}

static int hci_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
#if 1
	PBT_DEVICE_EXT	devExt = NULL; 

    ENTER_FUNC();
    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Suspend %d\n", message.event);

/**
    if(PM_EVENT_SUSPEND != message.event
        || PM_EVENT_HIBERNATE != message.event){
        return -EBUSY;
    }
**/

	devExt = usb_get_intfdata(intf);
    if(!devExt->hdev){
        return 0;
    }
    
	hci_suspend_dev(devExt->hdev);
    BtPower_SetPowerD3(devExt, 1, 1, 1);

    if(PM_EVENT_SUSPEND != message.event 
        && FW_WORK_MODE_SINGLE == devExt->ComboState){
        devExt->usb_device->state = USB_STATE_SUSPENDED;
    }
    EXIT_FUNC();
	return 0;
#else
    // Not allow suspend
    return -EBUSY;
#endif    

}

static int hci_usb_resume(struct usb_interface *intf)
{
#if 1
	PBT_DEVICE_EXT	devExt = NULL; 

    ENTER_FUNC();
    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Resume\n");
	devExt = usb_get_intfdata(intf);
	
    if(!devExt->hdev){
        return 0;
    }
    BtPower_SetPowerD0(devExt, 1, 1, 1);
	
    hci_resume_dev(devExt->hdev);
	return 0;
#else
    return -EBUSY;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
static int hci_usb_reset_resume(struct usb_interface *intf)
{
    ENTER_FUNC();
    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Reset Resume\n");
    dump_stack();
    return hci_usb_resume(intf);
}

static int hci_usb_prereset(struct usb_interface *intf)
{
    ENTER_FUNC();
	// TODO: cancel timers
	//3 Jakio20080822: this is like " pnp query stop"

	//  1. disable  intsending timers
	//  2. cancel watchdog timers
	//  3. cancel watch dog workqueue
	//  4. cancel tx sending timers

    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Prereset\n");
    EXIT_FUNC();
	return 0;
}

static int hci_usb_postreset(struct usb_interface *intf)
{
    ENTER_FUNC();
	// TODO: cancel timers
	//3 Jakio20080822: this is like " pnp query stop"

	//  1. disable  intsending timers
	//  2. cancel watchdog timers
	//  3. cancel watch dog workqueue
	//  4. cancel tx sending timers

    BT_DBGEXT(ZONE_MAIN | LEVEL0, "Postreset\n");
    EXIT_FUNC();
	return 0;
}
#endif

static struct usb_driver hci_usb_driver = {
	.name		= "3DSP_BT_USB",
	.probe		= hci_usb_probe,
	.disconnect	= hci_usb_disconnect,
	.suspend	= hci_usb_suspend,
	.resume		= hci_usb_resume,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
	.reset_resume = hci_usb_reset_resume,
	.pre_reset  = hci_usb_prereset,
	.post_reset = hci_usb_postreset,
#endif
	.id_table	= bluetooth_ids,
};

/* Wrapper of hc_recv_frame */
int hci_notify_frame(struct sk_buff *skb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
    struct hci_dev *hdev = (struct hci_dev *) skb->dev;
#endif
    BT_DBGEXT(ZONE_MAIN | LEVEL3, "Notify up HCI frames\n");

    /* Filter out the HCI Event */
    if(HCI_EVENT_PKT == bt_cb(skb)->pkt_type){
       do_cc(skb); 
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
    hci_recv_frame(hdev, skb);
#else
    hci_recv_frame(skb);
#endif

    return 0;
}


static int __init hci_usb_init(void)
{
	int err;

    ENTER_FUNC();
	BT_DBGEXT(ZONE_MAIN | LEVEL3, "HCI USB driver ver %s\n", VERSION);

	if ((err = usb_register(&hci_usb_driver)) < 0)
		BT_DBGEXT(ZONE_MAIN | LEVEL0, "Failed to register HCI USB driver\n");

    EXIT_FUNC();
	return err;
}

static void __exit hci_usb_exit(void)
{
    ENTER_FUNC();
	usb_deregister(&hci_usb_driver);
    EXIT_FUNC();
}

module_init(hci_usb_init);
module_exit(hci_usb_exit);


MODULE_AUTHOR("jakio.chen@3dsp.com.cn, yassil.gong@3dsp.com.cn");
MODULE_DESCRIPTION("3DSP Bluetooth HCI USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");

