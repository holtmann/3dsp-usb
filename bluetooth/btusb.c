/***********************************************************************
 * FILENAME:         btusb.c
 * CURRENT VERSION:
 * CREATE DATE:      2006
 * PURPOSE:  G
 *
 * AUTHORS:  Peter han
 *
 * NOTES:    //
 ***********************************************************************/
#include "bt_sw.h"        
#include "bt_hci.h"
#include "bt_pr.h"
#include "bt_dbg.h"
#include "bt_usbregdef.h"
#include "bt_task.h"
#include "bt_hal.h"
#include "bt_frag.h"


NTSTATUS UsbResetDevice(PBT_DEVICE_EXT devExt);

NTSTATUS UsbSendIrp(PBT_DEVICE_EXT devExt, IN PURB Urb);
NTSTATUS UsbProcessInt(PBT_DEVICE_EXT devExt, PVOID pBuffer);
UINT8 UsbGetFreeRecvAreaIndex(IN PBT_DEVICE_EXT devExt);

VOID Usb_ReadCompletion(PURB pUrb);
VOID Usb_InquiryResultCompletion(PURB pUrb);
VOID Usb_WriteCompletion(PURB pUrb);
VOID Usb_IntCompletion(PBT_DEVICE_EXT devExt, UINT8 * pBuf);



/* URB complete proxy */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)

VOID Usb_IntCompletion_Imp(PURB pUrb);
VOID Usb_ReadCompletion_Imp(PURB pUrb);
VOID Usb_WriteCompletion_Imp(PURB pUrb);
VOID Usb_InquiryResultCompletion_Imp(PURB pUrb);

#else

struct pt_regs;
VOID Usb_IntCompletion_Imp(PURB pUrb, struct pt_regs *pt);
VOID Usb_ReadCompletion_Imp(PURB pUrb, struct pt_regs *pt);
VOID Usb_WriteCompletion_Imp(PURB pUrb, struct pt_regs *pt);
VOID Usb_InquiryResultCompletion_Imp(PURB pUrb, struct pt_regs *pt);

#endif





/* queue operations */
void _urb_queue_init(struct _urb_queue *q)
{
	INIT_LIST_HEAD(&q->head);
	spin_lock_init(&q->lock);
}

void _urb_queue_head(struct _urb_entry *ue, struct _urb_queue *q)
{
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	list_add(&ue->list, &q->head);
	spin_unlock_irqrestore(&q->lock, flags);
}

void _urb_queue_tail(struct _urb_entry *ue, struct _urb_queue *q)
{
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	list_add_tail(&ue->list, &q->head);
	spin_unlock_irqrestore(&q->lock, flags);
}


struct _urb_entry *_urb_dequeue(struct _urb_queue *q)
{
	struct _urb_entry *ue = NULL;
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	{
		struct list_head *head = &q->head;
		struct list_head *next = head->next;
		if (next != head) {
			ue = list_entry(next, struct _urb_entry, list);
			list_del(next);
		}
	}
	spin_unlock_irqrestore(&q->lock, flags);
	
	return ue;
}

/* Acquire one queue entry */
struct _urb_entry *_get_free_urb_entry(struct _urb_queue *q)
{
	struct _urb_entry *ue = NULL;
        
	ue = _urb_dequeue(q); 
	if(!ue){
		BT_DBGEXT(ZONE_USB | LEVEL3, "Alloc new urb resource\n");
        ue = kzalloc(sizeof(struct _urb_entry), GFP_ATOMIC);
	}
	return ue;
}

/* Free urb queue entry */
void _free_urb_entry(struct _urb_queue *q)
{
	struct _urb_entry *ue = NULL;
	
	while(ue = _urb_dequeue(q)){
		BT_DBGEXT(ZONE_USB | LEVEL3, "Free urb queue entry\n");
		ExFreePool(ue);
	}
}


/* First kill, then free the urb */
static void kill_urb(struct urb *purb)
{
    usb_kill_urb(purb);
	usb_free_urb(purb);
}

struct urb *alloc_urb(int iso_packets, gfp_t mem_flags)
{    
	return usb_alloc_urb(iso_packets, mem_flags);
}


//////////////////////////////////////////////////////////////////////////
// Routine Description:
// This routine active usb device
// Arguments:
// DeviceObject - pointer to device object
// Return Value:
// NTSTATUS
//////////////////////////////////////////////////////////////////////////
NTSTATUS UsbActiveDevice(PBT_DEVICE_EXT devExt)
{
	return STATUS_SUCCESS;
}



NTSTATUS UsbAsynConInit(IN PBT_DEVICE_EXT devExt)
{
	UINT32	i;
	BOOLEAN	FunctionOK = FALSE;
	PURB	pUrb = NULL;

	if(devExt == NULL)
		return STATUS_UNSUCCESSFUL;
	__try
	{
		QueueInitList(&devExt->UsbContext.AsyUrb_FreeList);
		QueueInitList(&devExt->UsbContext.AsyUrb_UsedList);
		QueueInitList(&devExt->UsbContext.AsyContext_FreeList);
		QueueInitList(&devExt->UsbContext.AsyContext_UsedList);
		QueueInitList(&devExt->UsbContext.AsyVcmdPara_FreeList);
		QueueInitList(&devExt->UsbContext.AsyVcmdPara_UsedList);

		for(i =0; i < MAX_ASYN_REQUEST_NUM; i++)
		{
			devExt->UsbContext.VAsynUrb[i].ptotaldata = NULL;
		}

		
		for(i =0; i < MAX_ASYN_REQUEST_NUM; i++)
		{
			pUrb = alloc_urb(0, GFP_KERNEL);
			if(pUrb == NULL)
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "UsbAsynConInit---allocate urb failed");
				__leave;
			}
			devExt->UsbContext.VAsynUrb[i].ptotaldata = (PUINT8)pUrb;
		}
		//link all the nodes
		for(i =0; i < MAX_ASYN_REQUEST_NUM; i++)
		{
			QueuePutTail(&devExt->UsbContext.AsyUrb_FreeList, &devExt->UsbContext.VAsynUrb[i].Link);
			QueuePutTail(&devExt->UsbContext.AsyContext_FreeList, &devExt->UsbContext.VAsynCmdCon[i].Link);
			QueuePutTail(&devExt->UsbContext.AsyVcmdPara_FreeList, &devExt->UsbContext.VAsynCmdPara[i].Link);
		}
		
		FunctionOK = TRUE;
	}
	__finally
	{
		if(FunctionOK == TRUE)
		{
			
		}
		else
		{
			UsbAsynConRelease(devExt);
		}
	}
	if(FunctionOK == TRUE)
		return STATUS_SUCCESS;
	else
	{
		return STATUS_UNSUCCESSFUL;
	}

}


VOID UsbAsynConRelease(IN PBT_DEVICE_EXT devExt)
{
	UINT32	i;
	PURB	pUrb;

	if(devExt == NULL)
		return;

	for(i =0; i < MAX_ASYN_REQUEST_NUM; i++)
	{
		pUrb = (PURB)devExt->UsbContext.VAsynUrb[i].ptotaldata;
		if(pUrb)
		{
			/* Unlink and free the URB */
			kill_urb(pUrb);
			devExt->UsbContext.VAsynUrb[i].ptotaldata = NULL;	
		}
	}
	UsbAsynConReset(devExt);
}

VOID UsbAsynConReset(IN PBT_DEVICE_EXT devExt)
{
	if(devExt == NULL)
		return;
	
	QueueInitList(&devExt->UsbContext.AsyUrb_FreeList);
	QueueInitList(&devExt->UsbContext.AsyUrb_UsedList);
	QueueInitList(&devExt->UsbContext.AsyContext_FreeList);
	QueueInitList(&devExt->UsbContext.AsyContext_UsedList);
	QueueInitList(&devExt->UsbContext.AsyVcmdPara_FreeList);
	QueueInitList(&devExt->UsbContext.AsyVcmdPara_UsedList);
}



/*********************************************************************************
Routine Description:
	This routine initialize usb device's sturctrue
Arguments:
	DeviceObject - pointer to device object
Return Value:
	NTSTATUS
ChangLog:
	Jakio rewrite this function using SEH tech to avoid mem peaking, 20071203
*********************************************************************************/
NTSTATUS UsbInitialize(IN PBT_DEVICE_EXT devExt)
{
	UINT8 i;
	PUINT8 pbuf;
	PURB	pUrb = NULL;
	UINT8 FunctionOK = FALSE;

	__try
	{

		RtlZeroMemory(&devExt->UsbContext.UsbBulkInContext, sizeof(USB_BULKIN_CONTEXT));
		
		pUrb = alloc_urb(0, GFP_KERNEL);
		if(pUrb == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "allocate urb for bulkin failure1");
			__leave;
		}
		devExt->UsbContext.UsbBulkInContext.pUrb = (PINT8)pUrb;
		
		for (i = 0; i < MAX_USB_BUKIN_AREA; i++)
		{
			pbuf = (PUINT8)ExAllocatePool(sizeof(USB_BUKLIN_RECV_AREA), GFP_KERNEL);
			if(pbuf == NULL)
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "allocate buffer for bulkin failure\n");
				__leave;
			}
			devExt->UsbContext.UsbBulkInContext.pRecvArea[i] = (PUSB_BUKLIN_RECV_AREA)(pbuf);
			devExt->UsbContext.UsbBulkInContext.pRecvArea[i]->InUseFlag = 0;	
			devExt->UsbContext.UsbBulkInContext.pRecvArea[i]->length = 0;
		}

		devExt->UsbContext.UsbBulkOutContext.pBulkOutPool = (PUINT8)ExAllocatePool(1024, GFP_KERNEL);
		if (devExt->UsbContext.UsbBulkOutContext.pBulkOutPool == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Allocate bulk OUT memory failed!\n");
			__leave;
		}
		
		devExt->UsbContext.UsbBulkOutContext.pBulkOutScoPool = (PUINT8)ExAllocatePool(1024, GFP_KERNEL);
		if (devExt->UsbContext.UsbBulkOutContext.pBulkOutScoPool == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Allocate bulk OUT memory failed!\n");
			__leave;
		}
		
		pUrb = alloc_urb(0, GFP_KERNEL);
		if(pUrb == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "allocate urb for bulkin failure2\n");
			__leave;
		}
		devExt->UsbContext.UsbBulkOutContext.pUrb = (PUINT8)pUrb;

		pUrb = alloc_urb(0, GFP_KERNEL);
		if(pUrb == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "allocate urb for bulkin failure3\n");
			__leave;
		}
		devExt->UsbContext.UsbBulkOutContext.pScoUrb = (PUINT8)pUrb;	


		devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr = (PUINT8)ExAllocatePool(8 *MAX_USB_INTERRUPT_AREA, GFP_KERNEL);
		if (devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Allocate bulk in memory failed!\n");
			__leave;
		}
		for (i = 0; i < MAX_INTERRUPT_IRPNUM; i++)
		{
			devExt->UsbContext.UsbIntContext.pInterruptPool[i] = devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr + i * MAX_USB_INTERRUPT_AREA;
		}
		
		pUrb = alloc_urb(0, GFP_KERNEL);
		if(pUrb == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "allocate urb for bulkin failure4\n");
			__leave;
		}
		devExt->UsbContext.UsbIntContext.pUrb = (PUINT8)pUrb;

		devExt->UsbContext.UsbBulkIn2.BulkIn2Buf = ExAllocatePool(MAX_USB_BULKIN_FRAME, GFP_KERNEL);
		if(devExt->UsbContext.UsbBulkIn2.BulkIn2Buf == NULL)
		{
			__leave;
		}
		
		pUrb = alloc_urb(0, GFP_KERNEL);
		if(pUrb == NULL)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "allocate urb for bulkin failure4\n");
			__leave;
		}
		devExt->UsbContext.UsbBulkIn2.pUrb = (PUINT8)pUrb;
		
		if(STATUS_SUCCESS != UsbAsynConInit(devExt))
		{
			__leave;
		}

		init_waitqueue_head(&devExt->BulkOutEvent);
		atomic_set(&(devExt->BulkOutFlag), EVENT_UNSIGNED);

		init_waitqueue_head(&(devExt->UsbContext.UsbIntContext.IntEvent));
		atomic_set(&(devExt->UsbContext.UsbIntContext.IntEventFlag), EVENT_UNSIGNED);
		

		FunctionOK = TRUE;
	
	}
	__finally
	{
		if(FunctionOK == TRUE)
		{
			
		}
		else
		{
			UsbUnInitialize(devExt);
		}
	}
	if(FunctionOK == TRUE)
		return STATUS_SUCCESS;
	else
	{
		return FALSE;
	}


}



VOID UsbUnInitialize(IN PBT_DEVICE_EXT devExt)
{
	UINT32 i;
	PUINT8 pbuf;
	PURB	pUrb;

	UsbAsynConRelease(devExt);
	
	devExt->UsbContext.UsbBulkInContext.Length = 0x0;

	pUrb = (PURB)devExt->UsbContext.UsbBulkInContext.pUrb;
	if(pUrb){
		kill_urb(pUrb);
	}
	devExt->UsbContext.UsbBulkInContext.pUrb = NULL;

	for(i = 0; i < MAX_USB_BUKIN_AREA; i++)
	{
		pbuf = (PUINT8)devExt->UsbContext.UsbBulkInContext.pRecvArea[i];
		if(pbuf)
			ExFreePool(pbuf);
		devExt->UsbContext.UsbBulkInContext.pRecvArea[i] = NULL;
	}
	
	
	pUrb = (PURB)devExt->UsbContext.UsbIntContext.pUrb;
	if(pUrb){
		kill_urb(pUrb);
	}
	devExt->UsbContext.UsbIntContext.pUrb = NULL;
	for (i = 0; i < MAX_INTERRUPT_IRPNUM; i++)
	{
		devExt->UsbContext.UsbIntContext.IsUsed[i] = NOT_USED;
		devExt->UsbContext.UsbIntContext.Length[i] = 0x0;
	}
	if (devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr)
	{
		ExFreePool((PVOID)devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr);
		devExt->UsbContext.UsbIntContext.pInterruptPoolStartAddr = NULL;
	}
	for (i = 0; i < MAX_INTERRUPT_IRPNUM; i++)
	{
		devExt->UsbContext.UsbIntContext.pInterruptPool[i] = NULL;
	}

	pUrb = (PURB)devExt->UsbContext.UsbBulkOutContext.pUrb;
	if(pUrb){
		kill_urb(pUrb);
	}
	devExt->UsbContext.UsbBulkOutContext.pUrb = NULL;
	
	pUrb = (PURB)devExt->UsbContext.UsbBulkOutContext.pScoUrb;
	if(pUrb){
		kill_urb(pUrb);
	}
	devExt->UsbContext.UsbBulkOutContext.pScoUrb = NULL;

	pbuf = devExt->UsbContext.UsbBulkOutContext.pBulkOutPool;
	if(pbuf)
		ExFreePool(pbuf);
	devExt->UsbContext.UsbBulkOutContext.pBulkOutPool = NULL;

	pbuf = devExt->UsbContext.UsbBulkOutContext.pBulkOutScoPool;
	if(pbuf)
		ExFreePool(pbuf);
	devExt->UsbContext.UsbBulkOutContext.pBulkOutScoPool = NULL;

	pUrb = (PURB)devExt->UsbContext.UsbBulkIn2.pUrb;
	if(pUrb){
		kill_urb(pUrb);
	}
	devExt->UsbContext.UsbBulkIn2.pUrb = NULL;
	pbuf = devExt->UsbContext.UsbBulkIn2.BulkIn2Buf;
	if(pbuf)
		ExFreePool(pbuf);
	devExt->UsbContext.UsbBulkIn2.BulkIn2Buf = NULL;

	_free_urb_entry(&devExt->complete_q);
	_free_urb_entry(&devExt->urb_pool);		
}


VOID Usb_ResetResource(PBT_DEVICE_EXT devExt)
{
	UINT8 i;
	PBT_FRAG_T	pFrag;
	ASSERT(devExt);

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	
	BT_DBGEXT(ZONE_USB | LEVEL2, "Usb_ResetResource-- entered\n");
	
	for (i = 0; i < MAX_USB_BUKIN_AREA; i++)
	{
		devExt->UsbContext.UsbBulkInContext.pRecvArea[i]->InUseFlag = 0;
	}


	for (i = 0; i < MAX_INTERRUPT_IRPNUM; i++)
	{
		devExt->UsbContext.UsbIntContext.IsUsed[i] = NOT_USED;
		devExt->UsbContext.UsbIntContext.Length[i] = 0x0;
	}
	
}



NTSTATUS BtUsbSubmitReadIrp(PBT_DEVICE_EXT devExt)
{
	PURB urb = NULL;
	UINT32 stageLength = MAX_USB_BULKIN_FRAME;
	NTSTATUS ntStatus;
	PBULKUSB_READ_CONTEXT rwContext = NULL;
	PUINT8 BulkInBuffer;
	UINT8 index;
	UINT32	pipe;
	INT32	ret;
	

	if(devExt == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if(devExt->IsRxIrpSubmit == TRUE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Fatal error, an rx irp has already been dropped\n");
		goto BulkUsbReadWrite_Exit;
	}
	devExt->IsRxIrpSubmit = TRUE;

	index = UsbGetFreeRecvAreaIndex(devExt);
	if (index == 0xff)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbSubmitReadIrp()---failed to get recv area\n");
		devExt->IsRxIrpSubmit = FALSE;
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		devExt->UsbContext.UsbBulkInContext.Index_InUse = index;
		devExt->UsbContext.UsbBulkInContext.pRecvArea[index]->InUseFlag = 1;
		BulkInBuffer = devExt->UsbContext.UsbBulkInContext.pRecvArea[index]->buffer;
	}

	urb = (PURB)devExt->UsbContext.UsbBulkInContext.pUrb;
	
	rwContext = &devExt->UsbContext.BulkRwContext.BulkInReadContext0;
	RtlZeroMemory(rwContext, sizeof(BULKUSB_READ_CONTEXT));
	rwContext->Urb = urb;
	rwContext->Buffer = BulkInBuffer;
	rwContext->Length = stageLength;
	rwContext->DeviceExtension = devExt;
	RtlZeroMemory(rwContext->Buffer, MAX_USB_BULKIN_FRAME);

	pipe = usb_rcvbulkpipe(devExt->usb_device, INDEX_EP_BULK_IN_DATA);
	usb_fill_bulk_urb(urb,
				devExt->usb_device,
				pipe,
				BulkInBuffer,
				stageLength,
				Usb_ReadCompletion_Imp,
				rwContext);
	urb->transfer_flags &= ~URB_SHORT_NOT_OK;
	//urb->transfer_flags |=  URB_ASYNC_UNLINK;
	
	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if(ret == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL4, "submit rx urb ok\n");
		atomic_set(&devExt->RxIdleFlag, EVENT_UNSIGNED);
		ntStatus = STATUS_SUCCESS;
	}
	else
	{
		ntStatus = STATUS_UNSUCCESSFUL;
		devExt->IsRxIrpSubmit = FALSE;
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbSubmitReadIrp---Fatal error, IoCalldriver failed\n");
	}
	
	return ntStatus;
	
BulkUsbReadWrite_Exit: 
	BT_DBGEXT(ZONE_USB | LEVEL2, "BulkUsb_DispatchReadWrite - ends\n");
	return STATUS_UNSUCCESSFUL;
}




NTSTATUS BtUsbSubmitIquiryIrp(PBT_DEVICE_EXT devExt)
{

	PURB urb = NULL;
	UINT32 stageLength = MAX_USB_BULKIN_FRAME;
	NTSTATUS ntStatus;
	PBULKUSB_READ_CONTEXT rwContext = NULL;
	PUINT8 BulkInBuffer;
	UINT32	pipe;
	INT32	ret;
	
	BT_DBGEXT(ZONE_USB | LEVEL2, "Submit Inq IRP\n");

	if(devExt == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	
	if(devExt->IsInquiryIrpSubmit == TRUE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Fatal error, an rx irp has already been dropped\n");
		goto BulkUsbInquiry_Exit;
	}
	devExt->IsInquiryIrpSubmit = TRUE;

	BulkInBuffer = devExt->UsbContext.UsbBulkIn2.BulkIn2Buf;
	urb = (PURB)devExt->UsbContext.UsbBulkIn2.pUrb;
	
	rwContext = &devExt->UsbContext.BulkRwContext.BulkInReadContext2;
	RtlZeroMemory(rwContext, sizeof(BULKUSB_READ_CONTEXT));
	rwContext->Urb = urb;
	rwContext->Buffer = BulkInBuffer;
	rwContext->Length = stageLength;
	rwContext->DeviceExtension = devExt;
	RtlZeroMemory(rwContext->Buffer, MAX_USB_BULKIN_FRAME);

	pipe = usb_rcvbulkpipe(devExt->usb_device, INDEX_EP_BULK_IN_INQ);
	usb_fill_bulk_urb(urb,
				devExt->usb_device,
				pipe,
				BulkInBuffer,
				stageLength,
				Usb_InquiryResultCompletion_Imp,
				rwContext);
	urb->transfer_flags &= ~URB_SHORT_NOT_OK;
	//urb->transfer_flags |=  URB_ASYNC_UNLINK;
	
	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if(ret == 0)
	{
		ntStatus = STATUS_SUCCESS;
		atomic_set(&devExt->InqIdleFlag, EVENT_UNSIGNED);
	}
	else
	{
		ntStatus = STATUS_UNSUCCESSFUL;
		devExt->IsInquiryIrpSubmit = FALSE;
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbSubmitReadIrp---Fatal error, IoCalldriver failed\n");
	}
	
	return ntStatus;
	
BulkUsbInquiry_Exit: 
	BT_DBGEXT(ZONE_USB | LEVEL2, "BulkUsb_DispatchReadWrite - ends\n");
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS BtUsbWrite(PBT_DEVICE_EXT devExt, IN PVOID Buffer, IN UINT32 Length, IN UINT32 DataType)
{
	PURB urb;
	NTSTATUS ntStatus;
	
	PBULKUSB_WRITE_CONTEXT rwContext;
	KIRQL oldIrql;
	PBT_FRAG_T pFrag;
	UINT32	pipe;
	INT32	ret;
	
	urb = NULL;
	rwContext = NULL;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return STATUS_UNSUCCESSFUL;

	if(FALSE == BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbWrite--Hardware unnormal\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	if (DataType == MAILBOX_DATA_TYPE_SCO)
	{
		pipe = usb_sndbulkpipe(devExt->usb_device, INDEX_EP_BULK_OUT_SCO);
		rwContext = &devExt->UsbContext.BulkRwContext.BulkOutWriteScoContext;
		
		urb = (PURB)devExt->UsbContext.UsbBulkOutContext.pScoUrb;
		//Jakio20080307: change state
		KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
		devExt->SubmitBulkoutSco = TRUE;
		devExt->BulkoutSco_TimerCount = 3;	//we make it 3 seconds now
		KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);
		
	}
	else
	{
		pipe = usb_sndbulkpipe(devExt->usb_device, INDEX_EP_BULK_OUT_ACL);
		rwContext = &devExt->UsbContext.BulkRwContext.BulkOutWriteContext;

		urb = (PURB)devExt->UsbContext.UsbBulkOutContext.pUrb;
		KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
		devExt->SubmitBulkoutAcl = TRUE;
		devExt->BulkoutAcl_TimerCount = 7;	//we make it 3 seconds now
		KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);
	}
	RtlZeroMemory(rwContext, sizeof(BULKUSB_WRITE_CONTEXT));
	if (Length > BULKUSB_TRANSFER_BUFFER_SIZE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "Transfer length > circular buffer\n");
		ntStatus = STATUS_INVALID_PARAMETER;
		rwContext = NULL;
		goto BulkUsbReadWrite_Exit;
	}
	if (Length == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "Transfer data length = 0\n");
		ntStatus = STATUS_SUCCESS;
		rwContext = NULL;
		goto BulkUsbReadWrite_Exit;
	}

	rwContext->Urb = urb;
	rwContext->Buffer = Buffer;
	rwContext->Length = Length;
	rwContext->Numxfer = Length; /* jakio changed here
*/
	rwContext->DeviceExtension = devExt;
	rwContext->FragQueueType = (UINT8)DataType;
	


	//2 Fill urb
	usb_fill_bulk_urb(urb, 
				devExt->usb_device,
				pipe,
				Buffer,
				Length,
//      		Usb_WriteCompletion,
				Usb_WriteCompletion_Imp,
				rwContext);
	//urb->transfer_flags |= URB_ASYNC_UNLINK;

	//2 submit urb
	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if(ret == 0)
	{
		ntStatus = STATUS_SUCCESS;	
	}
	else
	{
		ntStatus = STATUS_UNSUCCESSFUL;
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbWrite----submit urb failure, errorNo:%ld\n", ret);
		if (DataType == MAILBOX_DATA_TYPE_SCO)
		{
			//Jakio20080307: reset state
			KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
			devExt->SubmitBulkoutSco = FALSE;
			devExt->BulkoutSco_TimerCount = 0;
			KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);	
		}
		else
		{
			KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
			devExt->SubmitBulkoutAcl = FALSE;
			devExt->BulkoutAcl_TimerCount = 0;
			KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);	
			
		}
	}

	return ntStatus;

BulkUsbReadWrite_Exit:
	BT_DBGEXT(ZONE_USB | LEVEL2, "BulkUsb_DispatchReadWrite - ends\n");
	return ntStatus;
}

VOID ReleaseBulkOut1Buf(PVOID pBuffer, PBT_DEVICE_EXT devExt)
{
	UINT32 i;
	KIRQL OldIrql;
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxPool[i].Buffer == pBuffer)
		{
			RtlZeroMemory(devExt->TxPool[i].Buffer, BT_MAX_FRAME_SIZE);
			devExt->TxPool[i].IsUsed = NOT_USED;
			devExt->TxAclPendingFlag = FALSE;
			devExt->AclPendingCounter = 0;
			break;
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
}

VOID ReleaseBulkOut2Buf(PVOID pBuffer, PBT_DEVICE_EXT devExt)
{
	UINT32 i;
	KIRQL OldIrql;
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, OldIrql);
	for (i = 0; i < MAX_TX_POOL_COUNT; i++)
	{
		if (devExt->TxScoPool[i].Buffer == pBuffer)
		{
			RtlZeroMemory(devExt->TxScoPool[i].Buffer, BT_MAX_SCO_FRAME_SIZE);
			devExt->TxScoPool[i].IsUsed = NOT_USED;
			devExt->TxScoPendingFlag = FALSE;
			devExt->ScoPendingCounter = 0;
			break;
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, OldIrql);
}

/*++
Routine Description:
This is the completion routine for reads/writes
If the irp completes with success, we check if we
need to recirculate this irp for another stage of
transfer. In this case return STATUS_MORE_PROCESSING_REQUIRED.
if the irp completes in error, free all memory allocs and
return the status.
Arguments:
DeviceObject - pointer to device object
Irp - I/O request packet
Context - context passed to the completion routine.
Return Value:
NT status value
--*/
VOID Usb_WriteCompletion(PURB pUrb)
{
	INT32	status;
	PBULKUSB_WRITE_CONTEXT rwContext;
	PBT_DEVICE_EXT devExt;
	PBT_FRAG_T	pFrag = NULL;
	KIRQL oldIrql;

	
	rwContext = (PBULKUSB_WRITE_CONTEXT)pUrb->context;
	if(rwContext == NULL)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "NULL pointer\n");
		return;
	}
    
	devExt = rwContext->DeviceExtension;
	status = pUrb->status;
	
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if (status == 0)
	{
		if (rwContext && rwContext->Numxfer != pUrb->actual_length){	
			BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_WriteCompletion()--tx real length error\n");
		}
	}
	else
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_WriteCompletion() - failed with status = %ld\n", status);
		if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_WriteCompletion---Device is suprised removed\n");
			devExt->DriverHaltFlag = 1;
		}
	}

	if (rwContext)
	{
		//set bulkout event
		atomic_set(&devExt->BulkOutFlag, EVENT_SIGNED);
		wake_up_interruptible(&devExt->BulkOutEvent);
		if(rwContext->FragQueueType != MAILBOX_DATA_TYPE_FIRMWARE)
		{
			if(rwContext->FragQueueType == MAILBOX_DATA_TYPE_SCO)
			{	
				ReleaseBulkOut2Buf(rwContext->Buffer, devExt);

				KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
				devExt->SubmitBulkoutSco = FALSE;
				devExt->BulkoutSco_TimerCount = 0;
				KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);	
			}
			else
			{
				KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
				devExt->SubmitBulkoutAcl = FALSE;
				devExt->BulkoutAcl_TimerCount = 0;
				KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);	
				ReleaseBulkOut1Buf(rwContext->Buffer, devExt);
			}

			UsbQueryDMASpace(devExt);	
			
		}
		else	//reset flag when type is MAILBOX_DATA_TYPE_FIRMWARE
		{
			KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
			devExt->SubmitBulkoutAcl = FALSE;
			devExt->BulkoutAcl_TimerCount = 0;
			KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);
		}
	}

	return;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
void Usb_WriteCompletion_Imp(PURB pUrb)
#else
void Usb_WriteCompletion_Imp(PURB pUrb, struct pt_regs  *pt)
#endif
{
	PBT_DEVICE_EXT devExt;
	PBULKUSB_WRITE_CONTEXT rwContext;
	struct _urb_entry * ue_free;

	rwContext = (PBULKUSB_WRITE_CONTEXT)pUrb->context;
	devExt = rwContext->DeviceExtension;

	/* Queue the URB */
	ue_free = _get_free_urb_entry(&devExt->urb_pool);
	ue_free->priv = pUrb;
	ue_free->utype = WRITE_URB;
	_urb_queue_tail(ue_free, &devExt->complete_q);
    
	/* Delay the job to tasklet */        
	BT_DBGEXT(ZONE_USB | LEVEL4, "$$Schedule the Tasklet$$\n");
    tasklet_schedule(&devExt->usb_complete_task);
}



NTSTATUS BtUsbWriteAll(PBT_DEVICE_EXT devExt , IN PVOID Buffer, IN UINT32 Length, IN UINT8 dataType, UINT8 UpdateSpace)
{
	PBT_FRAG_T	pFrag = NULL;
	NTSTATUS nStatus;
	UINT32 LoopNum = 10;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return STATUS_UNSUCCESSFUL;
	

	while(--LoopNum)
	{
		nStatus = VendorCmdWriteDataLength(devExt, (UINT16)Length, dataType);
		if (!NT_SUCCESS(nStatus))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbWriteAclAndOthers()--vendor cmd write failure\n");
		}
		else
			break;
	}
	if(LoopNum == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbWriteAll---Write cmd length failure\n");
		
		return STATUS_UNSUCCESSFUL;
	}

	nStatus = BtUsbWrite(devExt, Buffer, Length, dataType);
	if (!NT_SUCCESS(nStatus))
	{
		return nStatus;
	}

	return nStatus;

	
}



/***************************************************************************
Routine Description:
This is the completion routine for reads/writes
If the irp completes with success, we check if we
need to recirculate this irp for another stage of
transfer. In this case return STATUS_MORE_PROCESSING_REQUIRED.
if the irp completes in error, free all memory allocs and
return the status.
Arguments:
DeviceObject - pointer to device object
Irp - I/O request packet
Context - context passed to the completion routine.
Return Value:
NT status value
****************************************************************************/
void Usb_ReadCompletion(PURB pUrb)
{
	INT32	status;
	PBULKUSB_READ_CONTEXT rwContext;
	PBT_DEVICE_EXT devExt;
	KIRQL oldIrql;
	UINT8 RecvArea_Index;
	BT_RX_STATE tmpState;
	UINT32	PacketType;
	PUINT8 buffer;
	UINT32   length;


	rwContext = (PBULKUSB_READ_CONTEXT)pUrb->context;
	devExt = rwContext->DeviceExtension;
	status = pUrb->status;

	RecvArea_Index = devExt->UsbContext.UsbBulkInContext.Index_InUse;
	devExt->IsRxIrpSubmit = FALSE;
	if(devExt->InD3Flag == 1)
	{
		
		devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->InUseFlag = 0;
		if (status == 0)
		{
			atomic_set(&devExt->RxFwFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->RxFwEvent);
		}
		else if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN)
                || (status == -EPROTO) || ( status == -ECONNRESET))
		{
			atomic_set(&devExt->RxIdleFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->RxIdleEvent);
		}

		BT_DBGEXT(ZONE_USB | LEVEL4, "In D3, return %d\n", status);
		return;
	}

	if (status == 0)
	{
		devExt->RxFailTimerValid = FALSE;
		devExt->RxFailureCount = 0;
		if (rwContext)
		{
			if (pUrb->actual_length == 0)
			{
				devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->InUseFlag = 0;
				BtUsbSubmitReadIrp(devExt);				
				return;
			}
			else
			{
				devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->length = pUrb->actual_length;

				KeAcquireSpinLock(&devExt->RxStateLock, &oldIrql);
				tmpState = devExt->RxState;
				KeReleaseSpinLock(&devExt->RxStateLock, oldIrql);

				if (tmpState == RX_STATE_CONNECTED)
				{
					BtProcessRx_New(devExt, RecvArea_Index);
					devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->InUseFlag = 0;
				}
				else if (tmpState == RX_STATE_CONNECTING)
				{
					buffer = rwContext->Buffer;
					length = pUrb->actual_length;
					PacketType = BtSetPacketFilter(devExt);
					BtPreProcessPacket(devExt, PacketType, buffer, length);
					devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->InUseFlag = 0;
				}
				else
				{
					ASSERT(0);
				}	
				BtUsbSubmitReadIrp(devExt);
			}
		}
	}
	else
	{
		if(devExt->RxFailureCount == 0)
		{
			devExt->RxFailTimerCount = USB_READ_IRP_FAIL_TIME_COUNT;
			devExt->RxFailTimerValid = TRUE;
		}
		devExt->RxFailureCount++;
		BT_DBGEXT(ZONE_USB | LEVEL0, "ReadCompletion - failed with status = %d\n", status);
		if (rwContext)
		{
			if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN) ||
				(status == (-EPROTO)))
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_ReadCompletion---Device is suprised removed\n");
				devExt->DriverHaltFlag = 1;
				atomic_set(&devExt->RxIdleFlag, EVENT_SIGNED);
				wake_up_interruptible(&devExt->RxIdleEvent);
			}
            else if( status == -ECONNRESET)
			{
				atomic_set(&devExt->RxIdleFlag, EVENT_SIGNED);
				wake_up_interruptible(&devExt->RxIdleEvent);
			}
			else if(devExt->RxFailureCount < 50 && -ENOENT != status)
			{	
				BT_DBGEXT(ZONE_USB | LEVEL1, "Usb_ReadCompletion---submit read irp\n");
				devExt->UsbContext.UsbBulkInContext.pRecvArea[RecvArea_Index]->InUseFlag = 0;
			}
		}
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
void Usb_ReadCompletion_Imp(PURB pUrb)
#else
void Usb_ReadCompletion_Imp(PURB pUrb, struct pt_regs  *pt)
#endif
{
	PBT_DEVICE_EXT devExt;
	PBULKUSB_READ_CONTEXT rwContext;
	struct _urb_entry *ue_free;

	rwContext = (PBULKUSB_READ_CONTEXT)pUrb->context;

	devExt = rwContext->DeviceExtension;

	/* Queue the URB */
	ue_free = _get_free_urb_entry(&devExt->urb_pool);
	ue_free->priv = pUrb;
	ue_free->utype = READ_URB;
	_urb_queue_tail(ue_free, &devExt->complete_q);

	/* Delay the job to tasklet */        
	BT_DBGEXT(ZONE_USB | LEVEL4, "$$Schedule the Tasklet$$\n");
    tasklet_schedule(&devExt->usb_complete_task);
}


VOID Usb_InquiryResultCompletion(PURB pUrb)
{
	PBULKUSB_READ_CONTEXT rwContext;
	PBT_DEVICE_EXT devExt;
	INT32	status;


	rwContext = (PBULKUSB_READ_CONTEXT)pUrb->context;
	devExt = rwContext->DeviceExtension;

	devExt->IsInquiryIrpSubmit = FALSE;
	status = pUrb->status;
	if (status == 0)
	{
		devExt->InqFailTimerValid = FALSE;
		devExt->InquiryFailCount = 0;
		if (pUrb->actual_length == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL1, "Usb_InquiryResultCompletion--result length is zero\n");
		}
		else
		{
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_INQUIRY_RESULT_PROCESS), 
				BT_TASK_PRI_NORMAL, devExt->UsbContext.UsbBulkIn2.BulkIn2Buf, sizeof(FHS_PACKET_T));
		}

		BtUsbSubmitIquiryIrp(devExt);
	}
	else
	{
		if(devExt->InquiryFailCount == 0)
		{
			devExt->InqFailTimerCount = USB_READ_IRP_FAIL_TIME_COUNT;
			devExt->InqFailTimerValid = TRUE;
		}
		devExt->InquiryFailCount++;
		BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_InquiryResultCompletion - failed with status = %d\n", status);
		if (rwContext)
		{
			//Jakio20080612: judge device state
			if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN) || 
					(status == (-71)))
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_InquiryResultCompletion---Device is suprised removed\n");
				devExt->DriverHaltFlag = 1;
				atomic_set(&devExt->InqIdleFlag, EVENT_SIGNED);
				wake_up_interruptible(&devExt->InqIdleEvent);
			}
            else if(status == -ECONNRESET)
			{
				atomic_set(&devExt->InqIdleFlag, EVENT_SIGNED);
				wake_up_interruptible(&devExt->InqIdleEvent);
			}
			else
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_InquiryResultCompletion---submit inquiry irp\n");
			}
			
		}
	}

 }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
VOID Usb_InquiryResultCompletion_Imp(PURB pUrb)
#else
VOID Usb_InquiryResultCompletion_Imp(PURB pUrb, struct pt_regs  *pt)
#endif
{
	PBULKUSB_READ_CONTEXT rwContext;
	PBT_DEVICE_EXT devExt;
	struct _urb_entry * ue_free;

	rwContext = (PBULKUSB_READ_CONTEXT)pUrb->context;
	devExt = rwContext->DeviceExtension;
	BT_DBGEXT(ZONE_USB | LEVEL3, "@@Schedule Tasklet@@\n");

	/* Queue the URB */
	ue_free = _get_free_urb_entry(&devExt->urb_pool);
	ue_free->priv = pUrb;
	ue_free->utype = INQUIRY_URB;
	_urb_queue_tail(ue_free, &devExt->complete_q);

	/* Delay the job to tasklet */
    tasklet_schedule(&devExt->usb_complete_task);
}


/*++
Routine Description:
This routine synchronously submits a URB_FUNCTION_RESET_PIPE
request down the stack.
Arguments:
DeviceObject - pointer to device object
PipeInfo - pointer to PipeInformation structure
to retrieve the pipe handle
Return Value:
NT status value
--*/
NTSTATUS UsbResetPipe(PBT_DEVICE_EXT devExt, UINT8 PipeNum)
{
	
	return STATUS_SUCCESS;
}

VOID ResetDevice(PBT_DEVICE_EXT devExt)
{
	return;
}

NTSTATUS BtUsbInt(PBT_DEVICE_EXT devExt)
{
	PURB urb = NULL;
	UINT32 Length = MAX_USB_INTERRUPT_AREA;
	NTSTATUS ntStatus;
	PBULKUSB_INTERRUPT_CONTEXT IntContext = NULL;
	UINT32	pipe;
	INT32	ret;
	PUINT8  pIntrBuf;
	
	if(devExt == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if(devExt->UsbContext.UsbIntContext.InUsedFlag == USED)
	{
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		goto BulkUsbInt_Exit;
	}

    // Dynamically alloc buffer
	pIntrBuf = kzalloc(MAX_USB_INTERRUPT_AREA, GFP_ATOMIC);
	if(!pIntrBuf){
        BT_DBGEXT(ZONE_USB | LEVEL0, "Alloc interrupt buffer failure\n");
        goto BulkUsbInt_Exit;
	}


	devExt->UsbContext.UsbIntContext.InUsedFlag = USED;
	urb = (PURB)devExt->UsbContext.UsbIntContext.pUrb;
	IntContext = &devExt->UsbContext.BulkRwContext.BulkIntContext;

	IntContext->Urb = urb;
	IntContext->Buffer = pIntrBuf;
	IntContext->Length = Length;
	IntContext->Numxfer = Length;
	IntContext->DeviceExtension = (PVOID)devExt;
	RtlZeroMemory(IntContext->Buffer, MAX_USB_INTERRUPT_AREA);

	/*The '7' is a dummy, modify interval in tdsp_usb_rcvint in tdsp_usb.c*/
	
	pipe = usb_rcvintpipe(devExt->usb_device, INDEX_EP_INTR);

	usb_fill_int_urb(urb,
				devExt->usb_device,
				pipe,
				IntContext->Buffer,
				Length,
				Usb_IntCompletion_Imp,
				IntContext,
				7);	//Set interval to 16 microframes, equeals to 2ms in high speed device
	//urb->transfer_flags |=  URB_ASYNC_UNLINK;

	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if(ret == 0)
	{
		ntStatus = STATUS_SUCCESS;
		atomic_set(&devExt->IntIdleFlag, EVENT_UNSIGNED);
	}
	else
	{
		ntStatus = STATUS_UNSUCCESSFUL;
		devExt->UsbContext.UsbIntContext.InUsedFlag = NOT_USED;
		BT_DBGEXT(ZONE_USB | LEVEL0, "BtUsbSubmitIntIrp---failed submit urb:0x%x\n", ret);
	}

	return ntStatus;
	
BulkUsbInt_Exit: 
	BT_DBGEXT(ZONE_USB | LEVEL2, "BtUsbInt Error\n");
	return STATUS_UNSUCCESSFUL;

}

VOID Usb_VendorAsynCompletion(PURB pUrb)
{
	PVENDOR_ASYN_CONTEXT pContext;
	INT32	status;
	PBT_DEVICE_EXT devExt;

	KIRQL	oldIrql;
	PBT_FRAG_T	pFrag = NULL;

	pContext = (PVENDOR_ASYN_CONTEXT)pUrb->context;
	if(pContext == NULL)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "NULL pointer\n");
		return;
	}
	devExt = (PBT_DEVICE_EXT)pContext->DeviceExtension;
	

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	status = pUrb->status;
	if (status != 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_VendorAsynCompletion failure: 0x%lx, cmd:%lu\n", status, pContext->cmdSrc);
		if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_InquiryResultCompletion---Device is suprised removed\n");
			devExt->DriverHaltFlag = 1;
		}
		else
		{
		}
	}
	else
	{
		devExt->VcmdAsynFailureCount = 0;
	}
	/* jakio20080307: put resources into free list
*/
	if(pContext)
	{
		KeAcquireSpinLock(&devExt->AsynCmdLock, &oldIrql);
		if(pContext->pUrbBlock)
		{
			QueuePutTail(&devExt->UsbContext.AsyUrb_FreeList, &pContext->pUrbBlock->Link);
			pContext->pUrbBlock = NULL;
		}
		if(pContext->pVendPara)
		{
			QueuePutTail(&devExt->UsbContext.AsyVcmdPara_FreeList, &(((PVENDOR_CMD_PARAMETERS)pContext->pVendPara)->Link));
			pContext->pVendPara = NULL;
		}
		QueuePutTail(&devExt->UsbContext.AsyContext_FreeList, &pContext->Link);
		KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);
	}

}



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
VOID Usb_VendorAsynCompletion_Imp(PURB pUrb)
#else
void Usb_VendorAsynCompletion_Imp(PURB pUrb, struct pt_regs *pt)
#endif
{
	PVENDOR_ASYN_CONTEXT pContext;
	PBT_DEVICE_EXT devExt;
	struct _urb_entry * ue_free;
        
	pContext = (PVENDOR_ASYN_CONTEXT)pUrb->context;
	devExt = (PBT_DEVICE_EXT)pContext->DeviceExtension;

	/* Queue the URB */
	ue_free = _get_free_urb_entry(&devExt->urb_pool);
	ue_free->priv = pUrb;
	ue_free->utype = CTRL_URB;
	_urb_queue_tail(ue_free, &devExt->complete_q);

	/* Delay the job to tasklet */
    tasklet_schedule(&devExt->usb_complete_task);
}


VOID Usb_IntCompletion(PBT_DEVICE_EXT devExt, UINT8 *pBuf)
{
	INT32	status;
    
	/* Do real int function */
	status = UsbProcessInt(devExt, pBuf);
	if (!NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Interrupt processing error\n");
	}
    kfree(pBuf);
}

/* Move all implementation into tasklet */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
VOID Usb_IntCompletion_Imp(PURB pUrb)
#else
VOID Usb_IntCompletion_Imp(PURB pUrb, struct pt_regs  *pt)
#endif
{
	PBULKUSB_INTERRUPT_CONTEXT pContext;
	INT32	status;
	PBT_DEVICE_EXT devExt;
	struct _urb_entry * ue_free;

	pContext = (PBULKUSB_INTERRUPT_CONTEXT)pUrb->context;
	if(pContext == NULL)
		return;

	devExt = pContext->DeviceExtension;
	devExt->UsbContext.UsbIntContext.InUsedFlag = NOT_USED;

	status = pUrb->status;
	if (status != 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_IntCompletion---failed with status:%ld\n", status);
		if((status == -ENODEV) || (status == -EINVAL) || (status == -ESHUTDOWN) || 
			(status == (-71)))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Usb_IntCompletion---Device is suprised removed\n");
			devExt->DriverHaltFlag = 1;

			atomic_set(&devExt->IntIdleFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->IntIdleEvent);
			if(STATE_STARTED != devExt->State || !devExt->isOpen)
			{
				devExt->devRdyOk = 0;
            	atomic_set(&devExt->devRdyFlag, EVENT_SIGNED);
            	wake_up_interruptible(&devExt->DevRdyEvt);
			}
		}
        else if(status == -ECONNRESET)
		{
			atomic_set(&devExt->IntIdleFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->IntIdleEvent);
		}
		BT_DBGEXT(ZONE_USB | LEVEL0, "USB Interrupt URB failure\n");
		goto resubmit;
	}

	/* Queue the URB */
	ue_free = _get_free_urb_entry(&devExt->urb_pool);
	ue_free->priv = pContext->Buffer;
	ue_free->utype = INT_URB;
	_urb_queue_tail(ue_free, &devExt->complete_q);

	/* Delay the job to tasklet */
    tasklet_schedule(&devExt->usb_complete_task);

resubmit:
	BT_DBGEXT(ZONE_USB | LEVEL3, "Resubmit interrupt hanlder\n");
	BtUsbInt(devExt);	
}


void usb_complete_task(ULONG_PTR arg)
{
	PBT_DEVICE_EXT devExt;
	struct _urb_entry * ue_inq = NULL;
	devExt = (PBT_DEVICE_EXT)arg;  

	while((ue_inq = _urb_dequeue(&devExt->complete_q))){   
		if(ue_inq->priv){
			switch(ue_inq->utype){
			case READ_URB:
                Usb_ReadCompletion((PURB)ue_inq->priv);
				break;
			case WRITE_URB:
                Usb_WriteCompletion((PURB)ue_inq->priv);
				break;
			case INQUIRY_URB:
                Usb_InquiryResultCompletion((PURB)ue_inq->priv);
				break;
			case INT_URB:
				Usb_IntCompletion(devExt, ue_inq->priv);
				break;
			case CTRL_URB:
                Usb_VendorAsynCompletion((PURB)ue_inq->priv);
				break;
			default:
				BT_DBGEXT(ZONE_USB | LEVEL0, "Wrong URB scheduled\n");
			}
		}
		else{
			BT_DBGEXT(ZONE_USB | LEVEL0, "!!!! ERROR in urb entry !!!!\n");
		}
		/* return the urb entry to pool */
		ue_inq->priv = NULL;
		ue_inq->utype = NULL_URB;
		_urb_queue_tail(ue_inq, &devExt->urb_pool);
	}
}


BOOLEAN UsbCancelInterruptReq(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;

	BT_DBGEXT(ZONE_USB | LEVEL2, "UsbCancelInterruptReq entered\n");

	if (devExt->UsbContext.UsbIntContext.InUsedFlag == USED)
	{
		pUrb = (PURB)devExt->UsbContext.UsbIntContext.pUrb;
		usb_unlink_urb(pUrb);
		return TRUE;
	}
	else
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "no int irp needs to be canceled\n");
		return FALSE;
	}
		
	
}

BOOLEAN UsbCancelRxReq(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;
	INT32 i;
    
	i = devExt->UsbContext.UsbBulkInContext.Index_InUse;
	if(devExt->UsbContext.UsbBulkInContext.pRecvArea[i]->InUseFlag == 1)
	{
		pUrb = (PURB)devExt->UsbContext.UsbBulkInContext.pUrb;
		usb_unlink_urb(pUrb);
		return TRUE;		
	}
	else
		return FALSE;
}

BOOLEAN UsbCancelInquiryIrp(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;

	if(devExt->IsInquiryIrpSubmit == TRUE)
	{
		pUrb = (PURB)devExt->UsbContext.UsbBulkIn2.pUrb;
		usb_unlink_urb(pUrb);
		return TRUE;	
	}
	else
		return FALSE;
}



BOOLEAN UsbCancelAclIrp(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;
	
	if(devExt->SubmitBulkoutAcl == TRUE)
	{
		pUrb = (PURB)devExt->UsbContext.UsbBulkOutContext.pUrb;
		usb_unlink_urb(pUrb);
		return TRUE;		
	}
	else
		return FALSE;
}


NTSTATUS UsbCancelAclWriteIrp(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;

	pUrb = (PURB)devExt->UsbContext.UsbBulkOutContext.pUrb;
	usb_unlink_urb(pUrb);

	return STATUS_SUCCESS;		
}

NTSTATUS UsbCancelScoWriteIrp(PBT_DEVICE_EXT devExt)
{
	PURB	pUrb;

	pUrb = (PURB)devExt->UsbContext.UsbBulkOutContext.pScoUrb;	
	usb_unlink_urb(pUrb);

	return STATUS_SUCCESS;	
}


VOID BtUsbWriteIrpTimeOut(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;

	
	KeAcquireIRQSpinLock(&devExt->TxSpinLock, oldIrql);
	if(devExt->SubmitBulkoutAcl == TRUE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL3, "Check acl irp timeout\n");
		devExt->BulkoutAcl_TimerCount --;
		if(devExt->BulkoutAcl_TimerCount == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL1, "BtUsbWriteIrpTimeOut--acl irp timeout, Cancel it\n");
			UsbCancelAclWriteIrp(devExt);
		}
	}

	if(devExt->SubmitBulkoutSco == TRUE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL3, "Check sco irp timeout\n");	
		devExt->BulkoutSco_TimerCount --;
		if(devExt->BulkoutSco_TimerCount == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL1, "BtUsbWriteIrpTimeOut--sco irp timeout, Cancel it\n");
			UsbCancelScoWriteIrp(devExt);
		}
	}
	KeReleaseIRQSpinLock(&devExt->TxSpinLock, oldIrql);	
}


VOID BtUsbRxFailTimeOut(PBT_DEVICE_EXT devExt)
{
	if(devExt == NULL)
		return;

	if(devExt->RxFailTimerValid == TRUE)
	{
		if(--devExt->RxFailTimerCount  == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Rx fail timeout, clear fail count:%lu\n", devExt->RxFailureCount);
			devExt->RxFailTimerValid = FALSE;
			devExt->RxFailureCount = 0;
		}
	}

	if(devExt->InqFailTimerValid == TRUE)
	{
		if(--devExt->InqFailTimerCount  == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Inquiry fail timeout, clear fail count:%lu\n", devExt->InquiryFailCount);
			devExt->InqFailTimerValid = FALSE;
			devExt->InquiryFailCount = 0;
		}
	}
}







NTSTATUS UsbProcessInt(PBT_DEVICE_EXT devExt, PVOID pBuffer)
{
	UINT32 i = 0;
	NTSTATUS Status = STATUS_SUCCESS;
	UINT32 IntStatus = 0;
	UINT8 IntType = 0;
	UINT8 tmpVar;
	UINT8 buf[4];
	PUSB_INT_STACK pStack = NULL;
	PBTUSB_FLUSH_INT_BODY	pFlushBody = NULL;
	PBT_FRAG_T				pFrag = NULL;


	IntType = *(PUINT8)pBuffer;
	switch (IntType)
	{
		case USB_INTERRUPT_TYPE_NULL:
			BT_DBGEXT(ZONE_USB | LEVEL1, "UsbProcessInt()-- USB_INTERRUPT_TYPE_NULL\n");
			break;
		case USB_INTERRUPT_TYPE_3DSPHW_EVENT_MAP:
			BT_DBGEXT(ZONE_USB | LEVEL1, "UsbProcessInt()-- USB_INTERRUPT_TYPE_3DSPHW_EVENT_MAP\n");
			IntStatus = *((PUINT32)pBuffer + 1);
			if (IntStatus & BT_DSP_INT_DSP_ACK)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_DSP_ACK\n");
				BtProcessDspAckInt(devExt);
			}
			if (IntStatus &BT_DSP_INT_INQUIRY_RESULT_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_INQUIRY_RESULT_EVENT\n");
			}
			if (IntStatus &BT_DSP_INT_INQUIRY_COMPLETE_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_INQUIRY_COMPLETE_EVENT\n");
				BtProcessInquiryCompleteInt(devExt);
			}
			if (IntStatus &BT_DSP_INT_HARDWARE_ERROR_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_HARDWARE_ERROR_EVENT\n");
				BtProcessHardErrorInt(devExt);
			}
			if (IntStatus &BT_DSP_INT_FLUSH_OCCURED_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_FLUSH_OCCURED_EVENT\n");
				BtProcessFlushOccuredInt(devExt);
			}
			if (IntStatus &BT_DSP_INT_ROLE_CHANGE_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_ROLE_CHANGE_EVENT\n");
				Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_ROLE_CHANGE_INT),BT_TASK_PRI_NORMAL,NULL, 0);
			}
			if (IntStatus &BT_DSP_INT_DSP_MODE_CHANGE_INTO_HOLD)
			{
				/* jakiotodo: just mark here, need to think about it.
*/
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_DSP_MODE_CHANGE_INTO_HOLD\n");
			}
			if (IntStatus &BT_DSP_INT_DSP_CLOCK_READY_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_DSP_CLOCK_READY_EVENT\n");
			}
			if (IntStatus &BT_DSP_INT_DSP_MODE_CHANGE_INTO_ACTIVE)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_DSP_MODE_CHANGE_INTO_ACTIVE\n");
				tmpVar = BT_MODE_CURRENT_MODE_ACTIVE;
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_DSP_INT_ENTER_SNIFF), 
					BT_TASK_PRI_NORMAL, &tmpVar, sizeof(UINT8));
			}
			if (IntStatus &BT_DSP_INT_DSP_MODE_CHANGE_INTO_SNIFF)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_DSP_MODE_CHANGE_INTO_SNIFF\n");
				tmpVar = BT_MODE_CURRENT_MODE_SNIFF;
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_DSP_INT_ENTER_SNIFF), 
					BT_TASK_PRI_NORMAL, &tmpVar, sizeof(UINT8));
			}
			if (IntStatus &BT_DSP_INT_ROLE_CHANGE_FAIL_EVENT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_ROLE_CHANGE_FAIL_EVENT\n");
				Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_ROLE_CHANGE_FAIL_INT),BT_TASK_PRI_NORMAL,NULL, 0);
			}
			if (IntStatus &BT_DSP_INT_REPORT_RX_POWER)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_REPORT_RX_POWER\n");
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_DSP2HC_EVENT(BT_DSP_INT_REPORT_RX_POWER), BT_TASK_PRI_NORMAL, NULL, 0);
				break;
			}
			if (IntStatus &BT_DSP_INT_SLAVE_TX_COMP_BIT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_SLAVE_TX_COMP_BIT\n");
				break;
			}
			if (IntStatus &BT_DSP_INT_MASTER_SNIFF_TX_COMP_BIT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "BT_DSP_INT_MASTER_SNIFF_TX_COMP_BIT\n");
				break;
			}
			if(IntStatus >= BT_DSP_INT_MASTER_SNIFF_TX_COMP_BIT)
			{
				BT_DBGEXT(ZONE_USB | LEVEL1, "Unknow INT type: 0x%lx\n", IntStatus);
			}
			break;
		case USB_INTERRUPT_TYPE_3DSP_READ_REGISTER:
			BT_DBGEXT(ZONE_USB | LEVEL1, "UsbProcessInt()-- USB_INTERRUPT_TYPE_3DSP_READ_REGISTER\n");
			i = 0;
			pStack = &devExt->UsbContext.UsbIntContext.UsbIntStack[0];
			pStack->DataContent = *((PUINT32)pBuffer + 1);
			pStack->DataType = 0x02;
			pStack->DataAddress = *((PUINT16)pBuffer + 1);
			atomic_set(&devExt->UsbContext.UsbIntContext.IntEventFlag, EVENT_SIGNED);	
			wake_up_interruptible(&devExt->UsbContext.UsbIntContext.IntEvent);
			break;
		case USB_INTERRUPT_TYPE_READ_PROGRAM_REGISTER:
			BT_DBGEXT(ZONE_USB | LEVEL1, "UsbProcessInt()-- USB_INTERRUPT_TYPE_READ_PROGRAM_REGISTER\n");
			i = 0;
			pStack = &devExt->UsbContext.UsbIntContext.UsbIntStack[1];
			pStack->DataContent = *((PUINT32)pBuffer + 1);
			pStack->DataType = 0x02;
			pStack->DataAddress = *((PUINT16)pBuffer + 1);
			atomic_set(&devExt->UsbContext.UsbIntContext.IntEventFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->UsbContext.UsbIntContext.IntEvent);
			break;

		case USB_INTERRUPT_TYPE_DMA_SPACE_STATUS:
			if (*((PUINT8)pBuffer + 2) == MAILBOX_DATA_TYPE_MASTER)
			{
				devExt->AclWriteFlag = *((PUINT8)pBuffer + 1);
			}
			else if (*((PUINT8)pBuffer + 2) == MAILBOX_DATA_TYPE_SLAVE)
			{
				devExt->SlaveWriteFlag = *((PUINT8)pBuffer + 1);
			}
			else if (*((PUINT8)pBuffer + 2) == MAILBOX_DATA_TYPE_SNIFF)
			{
				devExt->SniffWriteFlag = *((PUINT8)pBuffer + 1);
			}
			else if (*((PUINT8)pBuffer + 2) == MAILBOX_DATA_TYPE_SCO)
			{
				devExt->ScoWriteFlag = *((PUINT8)pBuffer + 1);
			}
			else
			{
				devExt->FlagStatus = STATUS_INVALID_PARAMETER;
			}
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt()-- USB_INTERRUPT_TYPE_DMA_SPACE_STATUS: 0x%lx\n", *(PUINT32)pBuffer);
			break;
		case DSP_FHS_BDADDR_EXIST:
			

			if(*((PUINT8)pBuffer+1) == 0x01)
			{
				UINT8 bd_addr[BT_BD_ADDR_LENGTH];
				PBT_HCI_T pHci= NULL;
				pHci = (PBT_HCI_T)devExt->pHci;
				if (pHci == NULL)
				{
					ASSERT(0);
					break ;
				}
				RtlCopyMemory(bd_addr,(PUINT8)pBuffer+2,sizeof(BT_BD_ADDR_LENGTH));
				if (Hci_Find_Connect_Device_By_BDAddr(pHci, bd_addr) || Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, bd_addr))
				{
					BT_DBGEXT(ZONE_USB | LEVEL1, "ACL connection already exists when in savle page prcess\n");
					 break;
				}
			}
			else if(*((PUINT8)pBuffer+1) == 0x02)
			{	PBT_HCI_T pHci= NULL;
				pHci = (PBT_HCI_T)devExt->pHci;
				if (pHci == NULL)
				{
					ASSERT(0);
					break ;
				}
				RtlZeroMemory((PFHS_TEMP_PACKET)&pHci->fhs_temppacket,sizeof(FHS_TEMP_PACKET));
				RtlCopyMemory(pHci->fhs_temppacket.bd_addr,(PUINT8)pBuffer+2,sizeof(BT_BD_ADDR_LENGTH));
				break;

			}
			else if(*((PUINT8)pBuffer+1) == 0x03)
			{
				PBT_HCI_T pHci= NULL;
				pHci = (PBT_HCI_T)devExt->pHci;
				if (pHci == NULL)
				{
					ASSERT(0);
					break ;
				}
				pHci->fhs_temppacket.am_addr = *((PUINT8)pBuffer + 2);
				RtlCopyMemory(pHci->fhs_temppacket.class_of_device,(PUINT8)pBuffer + 3,BT_CLASS_OF_DEVICE_LENGTH);
				break;
			}
			else if(*((PUINT8)pBuffer+1) == 0x04)
			{

				PBT_HCI_T pHci= NULL;
				pHci = (PBT_HCI_T)devExt->pHci;
				if (pHci == NULL)
				{
					ASSERT(0);
					break ;
				}
				pHci->fhs_temppacket.sr = *(PUINT8)pBuffer + 2;
				pHci->fhs_temppacket.page_scan_mode = *(PUINT8)pBuffer +3;
				pHci->fhs_temppacket.clk27_2 = *(PUINT8)pBuffer +4;
				Hci_Add_Slave_Connect_Device(
					pHci, 
					pHci->fhs_temppacket.am_addr, 
					pHci->fhs_temppacket.bd_addr, 
					pHci->fhs_temppacket.class_of_device, 
					BT_PACKET_TYPE_DM1, 
					pHci->fhs_temppacket.sr, 
					pHci->fhs_temppacket.page_scan_mode, 
					pHci->fhs_temppacket.clk27_2, 
					1, 
					pHci->fhs_temppacket.tmpslaveid);
				if (pHci->sco_link_count > 0)
				{
					pHci->pcurrent_connect_device->max_slot = BT_MAX_SLOT_1_SLOT;
				}
				Hci_StartTimer(pHci->pcurrent_connect_device, BT_TIMER_TYPE_CONN_ACCEPT, (UINT16)(((UINT32)pHci->conn_accept_timeout *625) / (1000 *1000) + 1));

			}
			else
			{
				BT_DBGEXT(ZONE_USB | LEVEL1, "bdaddr query -- unknown int type..................\n");
				break; ;
			}

			break;
		case USB_INTERRUPT_TYPE_INIT_DSP_PARA:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt()--USB_INTERRUPT_TYPE_INIT_DSP_PARA:0x%8lx\n", *(PUINT32)((PUINT8)pBuffer+1));

			break;

		case USB_INTERRUPT_TYPE_JUMP_MIN_PROCESS:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt()--USB_INTERRUPT_TYPE_JUMP_MIN_PROCESS\n");
			atomic_set(&devExt->minLoopFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->MinLoopEvent);            
            
			break;
		case USB_INTERRUPT_TYPE_JUMP_MAIN_PROCESS:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt()--USB_INTERRUPT_TYPE_JUMP_MAIN_PROCESS\n");
			if(devExt->State < STATE_ALL_BELOW_FAIL)
			{
				BT_DBGEXT(ZONE_USB | LEVEL0, "the device status is error????\n");
			}
			if(devExt->AllowWlanJoin == FALSE)
				devExt->AllowWlanJoin = TRUE;
			atomic_set(&devExt->mainLoopFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->MainLoopEvent);            

			break;
		case DSP_SCRATCH_SPACE_INFO:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt--DSP_SCRATCH_SPACE_INFO\n");
			Frag_ProcessSpaceInt(devExt, pBuffer);
			break;
		case USB_INTERRUPT_TYPE_WAIT:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt--USB_INTERRUPT_TYPE_WAIT\n");
#if 0
			BtPrintBuffer(pBuffer, 8);
#endif
			
			atomic_set(&devExt->joinFlag, EVENT_SIGNED);
			wake_up_interruptible(&devExt->JoinEvent);

			if(devExt->PsComboReqFlag || devExt->InD3Flag)
			{
			    BT_DBGEXT(ZONE_USB | LEVEL1, "Do nothing, In D3 or Combo Request\n");
			}
			else
			{
				if(devExt->StartStage == START_STAGE_INIT)
				{

					if(*((PUINT8)pBuffer+1) == 0x02)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan exist, so just wait...\n");
					}
					else if(*((PUINT8)pBuffer+1) == 0x00)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan not exist, go my way..\n");
						tmpVar = 1;
						Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_START_STAGE_MAINLOOP),BT_TASK_PRI_NORMAL,&tmpVar, 1);
					}
					
				}
				else if(devExt->StartStage == START_STAGE_MINLOOP)
				{
					BT_DBGEXT(ZONE_USB | LEVEL3, "minloop stage....\n");
					if(*((PUINT8)pBuffer+1) == 0x02)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan exist...\n");

					}
					else if(*((PUINT8)pBuffer+1) == 0x00)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan not exist..\n");

						buf[0] = 1;	//download dsp firmare
						buf[1]= 1;		//request 8051 go to main loop
						buf[2] = 0;	//combo switch, not power management
						Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POWER_STATE_D0),BT_TASK_PRI_NORMAL,buf, 3);
						devExt->StartStage = START_STAGE_MAINLOOP;
					}
					
				}
				else if(devExt->StartStage == START_STAGE_MAINLOOP)
				{
					if(*((PUINT8)pBuffer+1) == 0x02)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan exist...\n");
						if(devExt->AllowWlanJoin == TRUE)
						{
							tmpVar = 0; //Jakio20080107: don't cancel int irp, we're waiting interrupt
							buf[0] = 0; //don't cancel int irp
							buf[1] = 0; //don't request 8051 go to mini loop
							buf[2] = 0; //Jakio20080519:not power management
							Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POWER_STATE_D3),BT_TASK_PRI_NORMAL,buf, 3);
							devExt->StartStage = START_STAGE_MINLOOP;
						}
						else
						{
							BT_DBGEXT(ZONE_USB | LEVEL1, "don't allow wlan join, ignore it\n");
						}
						
					}
					else if(*((PUINT8)pBuffer+1) == 0x00)
					{
						BT_DBGEXT(ZONE_USB | LEVEL1, "wlan not exist.., DON'T allow wlan join until we make work done\n");
						devExt->AllowWlanJoin = FALSE;
						/*
						buf[0] = 0;	//don't cancel int irp
						buf[1]= 1;		//request 8051 go to mini loop
						Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POWER_STATE_D3),BT_TASK_PRI_NORMAL,buf, 2);
						devExt->StartStage = START_STAGE_MINLOOP;
						buf[0] = 1;	//download dsp firmare
						buf[1]= 1;		//request 8051 go to main loop
						Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POWER_STATE_D0),BT_TASK_PRI_NORMAL,buf, 2);
						
						devExt->StartStage = START_STAGE_MAINLOOP;
						*/
						Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_STOP_AND_GO),BT_TASK_PRI_NORMAL,NULL, 0);
					}
					
				}
				
			}
			if(*((PUINT8)pBuffer+1) == 0x02)
			{
				devExt->ComboState = FW_WORK_MODE_COMBO;
				BT_DBGEXT(ZONE_USB | LEVEL3, "Combo state is combo\n");
			}
			else if(*((PUINT8)pBuffer+1) == 0x00)
			{
				devExt->ComboState = FW_WORK_MODE_SINGLE;
				BT_DBGEXT(ZONE_USB | LEVEL3, "Combo state is single\n");
			}
			
			break;
			
		case USB_INTERRUPT_TYPE_BEGIN_WORK:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt--USB_INTERRUPT_TYPE_BEGIN_WORK\n");
			if(devExt->PsComboReqFlag)
			{
				BT_DBGEXT(ZONE_USB | LEVEL3, "Set combo ready event\n");
				atomic_set(&devExt->ComboReadyFlag, EVENT_SIGNED);
				wake_up_interruptible(&devExt->ComboReadyEvent);
				devExt->PsComboReqFlag = FALSE;
			}
			else
			{
				if(devExt->StartStage == START_STAGE_INIT)
				{
					tmpVar = 0;
					// Finish Device init with phase 2
					devExt->phase2 = 1;
					Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_START_STAGE_MAINLOOP),BT_TASK_PRI_NORMAL,&tmpVar, 1);
				}
				else if(devExt->StartStage == START_STAGE_MINLOOP)
				{
					tmpVar = 0;
					buf[0] = 0; //don't download firmware
					buf[1] = 1;  //request 8051 go to main loop
					buf[2] = 0;	//combo switch, not power management
					Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POWER_STATE_D0),BT_TASK_PRI_NORMAL, buf, 3);
					devExt->StartStage = START_STAGE_MAINLOOP;
				}
				else if(devExt->StartStage == START_STAGE_MAINLOOP)
				{
					BT_DBGEXT(ZONE_USB | LEVEL3, "Already run, do nothing\n");
					
				}
			}
			
			
			if(*((PUINT8)pBuffer+1) == 0x02)
			{
				devExt->ComboState = FW_WORK_MODE_COMBO;
				BT_DBGEXT(ZONE_USB | LEVEL3, "Combo state is combo\n");
			}
			else if(*((PUINT8)pBuffer+1) == 0x00)
			{
				devExt->ComboState = FW_WORK_MODE_SINGLE;
				BT_DBGEXT(ZONE_USB | LEVEL3, "Combo state is single\n");
			}
			
			break;
		case USB_8051_VERSION_INFO:
			BT_DBGEXT(ZONE_USB | LEVEL3, "8051Version int!\n");
			RtlZeroMemory(devExt->Version8051,4);
			RtlCopyMemory(devExt->Version8051,(PUINT8)pBuffer+4,4);
			BT_DBGEXT(ZONE_USB | LEVEL3, "8051Version 0x%x\n", *(UINT32 *)((PUINT8)pBuffer + 4));
			break;
		case USB_DSP_VERSION_INFO:
			BT_DBGEXT(ZONE_USB | LEVEL3, "dspVersion int!\n");
			RtlZeroMemory(devExt->VersionDSP,4);
			RtlCopyMemory(devExt->VersionDSP,(PUINT8)pBuffer+4,4);
			BT_DBGEXT(ZONE_USB | LEVEL3, "DspVersion 0x%x\n", *(UINT32 *)((PUINT8)pBuffer + 4));
			break;
		case USB_INTERRUPT_CLOCK_INFO:
			BT_DBGEXT(ZONE_USB | LEVEL3, "UsbProcessInt--USB_INTERRUPT_CLOCK_INFO:%lu\n", *(PUINT32)((PUINT8)pBuffer+4));
			BtProcessClockReadyInt(devExt, *(PUINT32)((PUINT8)pBuffer+4));
			break;

		case USB_INTERRUPT_FLUSH:
            BT_INFOEXT("UsbProcessInt--USB_INTERRUPT_FLUSH\n");
			#ifdef BT_3DSP_FLUSH_SUPPORT
				pFlushBody = (PBTUSB_FLUSH_INT_BODY)pBuffer;
				pFrag = (PBT_FRAG_T)devExt->pFrag;
				
				BT_DBGEXT(ZONE_USB | LEVEL3, "FlushInt, TxQueue:%d, size: %d\n", pFlushBody->TxFifo, pFlushBody->Size);
				BT_DBGEXT(ZONE_USB | LEVEL3, "FlushInt, header: %x\n", pFlushBody->HcbbHeader);
				if(pFrag== NULL)
				{
					BT_DBGEXT(ZONE_USB | LEVEL3, "Error,Frag module pointer is null\n");
					break;
				}

				pFrag->TxDisable_Flag = 1;
				Task_CreateTask((PBT_TASK_T)devExt->pTask,BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_PROCESS_FLUSH),
									BT_TASK_PRI_NORMAL, pBuffer, sizeof(BTUSB_FLUSH_INT_BODY));
			#endif
			break;
			
		default:
			BT_DBGEXT(ZONE_USB | LEVEL1, "UsbProcessInt()-- unknown int type:%x, print int body:\n", IntType);
			BtPrintBuffer(pBuffer, 8);
			Status = STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}



UINT8 UsbGetFreeRecvAreaIndex(IN PBT_DEVICE_EXT devExt)
{
	UINT8 index;
	if (devExt == NULL)
	{
		ASSERT(0);
		return 0xff;
	}
	for (index = 0; index < MAX_USB_BUKIN_AREA; index++)
	{
		if (devExt->UsbContext.UsbBulkInContext.pRecvArea[index]->InUseFlag == 0)
		{
			break;
		}
	}
	if (index == MAX_USB_BUKIN_AREA)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "UsbGetFreeRecvAreaIndex()--no free recv area for bulkin\n");
		return 0xff;
	}
	else
	{
		return index;
	}
}

NTSTATUS UsbVendorRequest(PBT_DEVICE_EXT devExt, PVENDOR_CMD_PARAMETERS VenderCmdParameters)
{
	unsigned int pipe;
	int	ret;
	

	if(FALSE == BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "UsbVendorRequest--Error, driver not started\n");
		return STATUS_UNSUCCESSFUL;
	}
	if(VenderCmdParameters->bIn == VENDOR_CMD_IN)
		pipe = usb_rcvctrlpipe(devExt->usb_device, INDEX_EP_CTRL);
	else
		pipe = usb_sndctrlpipe(devExt->usb_device, INDEX_EP_CTRL);

	ret = usb_control_msg(devExt->usb_device, 
						pipe,
						VenderCmdParameters->Request,  					//request value
						VenderCmdParameters->RequestType, 					//request type
						VenderCmdParameters->Value,					//valuse
						VenderCmdParameters->Index,					//index
						VenderCmdParameters->TransferBuffer,	//data buffer
						VenderCmdParameters->TransferBufferLength,
						0				//timeout is 10s
						);
	if(ret < 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "UsbVendorRequest---usb_control_msg return failure:%d\n", ret);
		return STATUS_UNSUCCESSFUL;
		

	}
	else
	{
		return STATUS_SUCCESS;
	}

}

NTSTATUS VendorCmdRegRead(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PUINT8 pValue)
{
	VENDOR_CMD_PARAMETERS VenderCmdParameters;
	VenderCmdParameters.bIn = VENDOR_CMD_IN;
	VenderCmdParameters.bShortOk = 1;
	VenderCmdParameters.Index = address;
	VenderCmdParameters.ReservedBits = 0;
	VenderCmdParameters.TransferBuffer = pValue;
	VenderCmdParameters.TransferBufferLength = 1;
	VenderCmdParameters.Value = 0;
	VenderCmdParameters.Request = VENDOR_CMD_READ_REG;
	VenderCmdParameters.RequestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	return (UsbVendorRequest(devExt, &VenderCmdParameters));
} 

NTSTATUS VendorCmdRegWrite(PBT_DEVICE_EXT devExt, IN UINT16 address, IN UINT8 Value)
{
	VENDOR_CMD_PARAMETERS VenderCmdParameters;
	VenderCmdParameters.bIn = VENDOR_CMD_OUT;
	VenderCmdParameters.bShortOk = 1;
	VenderCmdParameters.Index = address;
	VenderCmdParameters.ReservedBits = 0;
	VenderCmdParameters.TransferBuffer = NULL;
	VenderCmdParameters.TransferBufferLength = 0;
	VenderCmdParameters.Value = Value;
	VenderCmdParameters.Request = VENDOR_CMD_WRITE_REG;
	VenderCmdParameters.RequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	UsbVendorRequest(devExt, &VenderCmdParameters);
	return STATUS_SUCCESS;
}

/*
NTSTATUS VendorCmdBurstRegRead(IN PBT_DEVICE_EXT devExt,, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length)
{
	NTSTATUS status;
	VENDOR_CMD_PARAMETERS VenderCmdParameters;
	VenderCmdParameters.bIn = VENDOR_CMD_IN;
	VenderCmdParameters.bShortOk = 1;
	VenderCmdParameters.Index = address;
	VenderCmdParameters.ReservedBits = 0;
	VenderCmdParameters.TransferBuffer = pValue;
	VenderCmdParameters.TransferBufferLength = Length;
	VenderCmdParameters.Value = 0;
	VenderCmdParameters.Request = VENDOR_CMD_BURST_READ_REG;
	VenderCmdParameters.RequestTypeReservedBits = USB_TYPE_VENDOR | USB_DIR_IN | USB_RECIP_DEVICE;
	status = UsbVendorRequest(devExt, &VenderCmdParameters);
	return status;
}
*/
NTSTATUS VendorCmdBurstRegRead2(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length)
{
	INT8 temp = 0;
	NTSTATUS status;
	VENDOR_CMD_PARAMETERS VenderCmdParameters;
	while (Length > 0)
	{
		RtlZeroMemory(&VenderCmdParameters, sizeof(VENDOR_CMD_PARAMETERS));
		VenderCmdParameters.bIn = VENDOR_CMD_IN;
		VenderCmdParameters.bShortOk = 1;
		VenderCmdParameters.Index = address + temp;
		VenderCmdParameters.ReservedBits = 0;
		VenderCmdParameters.TransferBuffer = (PUINT8)pValue + temp;
		VenderCmdParameters.TransferBufferLength = 1;
		VenderCmdParameters.Value = 0;
		VenderCmdParameters.Request = VENDOR_CMD_READ_REG;
		VenderCmdParameters.RequestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
		status = UsbVendorRequest(devExt, &VenderCmdParameters);
		if (!NT_SUCCESS(status))
		{
			return status;
		}
		Length--;
		temp++;
	}
	return STATUS_SUCCESS;
}

NTSTATUS VendorCmdBurstRegWrite(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length)
{
	VENDOR_CMD_PARAMETERS VenderCmdParameters;

	RtlZeroMemory(&VenderCmdParameters, sizeof(struct _VENDOR_CMD_PARAMETERS));
	VenderCmdParameters.bIn = VENDOR_CMD_OUT;
	VenderCmdParameters.bShortOk = 0;
	VenderCmdParameters.Index = address;
	VenderCmdParameters.ReservedBits = 0;
	VenderCmdParameters.TransferBuffer = pValue;
	VenderCmdParameters.TransferBufferLength = Length;
	VenderCmdParameters.Value = 0;
	VenderCmdParameters.Request = VENDOR_CMD_BURST_WRITE_REG;
	VenderCmdParameters.RequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;//USB_TYPE_VENDOR | USB_DIR_OUT | USB_RECIP_DEVICE;
	UsbVendorRequest(devExt, &VenderCmdParameters);
	return STATUS_SUCCESS;
}

NTSTATUS VendorCmdBurstRegWrite2(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length)
{
	UINT8 temp = 0;
	VENDOR_CMD_PARAMETERS VenderCmdParameters;
	NTSTATUS status;
	while (Length > 0)
	{
		RtlZeroMemory(&VenderCmdParameters, sizeof(VENDOR_CMD_PARAMETERS));
		VenderCmdParameters.bIn = VENDOR_CMD_OUT;
		VenderCmdParameters.bShortOk = 1;
		VenderCmdParameters.Index = address + temp;
		VenderCmdParameters.ReservedBits = 0;
		VenderCmdParameters.TransferBuffer = NULL;
		VenderCmdParameters.TransferBufferLength = 0;
		VenderCmdParameters.Value = *((PUINT8)pValue + temp);
		VenderCmdParameters.Request = VENDOR_CMD_WRITE_REG;
		VenderCmdParameters.RequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;//USB_TYPE_VENDOR | USB_DIR_OUT | USB_RECIP_DEVICE;
		status = UsbVendorRequest(devExt, &VenderCmdParameters);
		if (!NT_SUCCESS(status))
		{
			return status;
		} Length--;
		temp++;
	}
	return STATUS_SUCCESS;
}

UINT8 VendorCmdReadMailBoxFlag(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT8 Value = 0;
	status = VendorCmdRegRead(devExt, MAILBOX_CMD_VENDORCMD_ADDRESS, &Value);
	return Value;
}

UINT8 VendorCmdReadActivateBits(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT8 Value = 0;
	status = VendorCmdRegRead(devExt, MAILBOX_CMD_VENDORCMD_ADDRESS, &Value);
	if (!NT_SUCCESS(status))
	{
		Value = 0x55;
	}
	return Value;
}

BOOLEAN VendorCmdMailBoxIsBusy(PBT_DEVICE_EXT devExt)
{
	UINT8 tmp;
	tmp = VendorCmdReadMailBoxFlag(devExt);
	tmp &= (UINT8)MAILBOX_CMD_BUSY_BITS;
	return (tmp == 0) ? FALSE : TRUE;
}

UINT8 VendorCmdActivateByte(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status = 0;
	UINT8 state =  0xfe;//Jakio20080613: init var, not equal to "MAILBOX_CMD_INVALID"
	status = VendorCmdBurstRegRead2(devExt, 0x128, (PVOID) &state, 1);
	if (!NT_SUCCESS(status))
	{
		state = MAILBOX_CMD_INVALID;
	}
	if(state == 0xfe)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdActivateByte--Error, vcmd lost\n");
	}
	return state;
	/*
	if (state == 0)
	{
		DebugPrint("ok\n");
	}
	DebugPrint("the busy byte = %d\n", state);
	return (state == 0) ? FALSE : TRUE;
	*/
}

UINT8 VendorCmdRegAPIByte(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status = 0;
	UINT8 state =  0xfe; //Jakio20080613: init var, not equal to "MAILBOX_CMD_INVALID"
	status = VendorCmdBurstRegRead2(devExt, 0x148, (PVOID) &state, 1);
	if (!NT_SUCCESS(status))
	{
		state = MAILBOX_CMD_INVALID;
	}
	return state;

}

NTSTATUS VendorCmdWriteLengthToMailBox(PBT_DEVICE_EXT devExt, IN PVOID value, IN UINT8 CmdType)
{
	UINT16 offset = 0;
	UINT32 valueLength;
	NTSTATUS Status = STATUS_SUCCESS;

	PVCMD_LENGTH pCmdLength = NULL;
	
	if(FALSE == BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteLengthToMailBox--Error,driver not started\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	switch (CmdType)
	{
		case MAILBOX_DATA_TYPE_SCO:
			offset = MAILBOX_LENGTH_SCO_ADDRESS;
			break;
		case MAILBOX_DATA_TYPE_MASTER:
		case MAILBOX_DATA_TYPE_SLAVE:
		case MAILBOX_DATA_TYPE_SNIFF:
			offset = MAILBOX_LENGTH_ACL_OTHERS_ADDRESS;
			pCmdLength = (PVCMD_LENGTH)value;
			break;
		default:
			return STATUS_INVALID_PARAMETER;
	}
	valueLength = sizeof(VCMD_LENGTH);
	
	Status = VendorCmdBurstRegWriteAnyn(devExt, offset, value, valueLength, VENDOR_CMD_SET_DMA_REG);
	if(!NT_SUCCESS(Status))
	{
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		return STATUS_SUCCESS;
	}

}




NTSTATUS VendorCmdWriteDataLength(PBT_DEVICE_EXT devExt, IN UINT16 Length, IN UINT8 CmdType)
{
	VCMD_LENGTH VCmdLength;
	NTSTATUS Status = STATUS_SUCCESS;

	VCmdLength.Length = (UINT16)Length;
	VCmdLength.Type = CmdType;
	

	Status = VendorCmdWriteLengthToMailBox(devExt, &VCmdLength, CmdType);
	if (!NT_SUCCESS(Status))
	{
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		return STATUS_SUCCESS;
	}

	
	return Status;
}



NTSTATUS VendorCmdWriteCmdToMailBox(PBT_DEVICE_EXT devExt, IN PVOID value, IN UINT8 CmdType)
{
	UINT32 loop = VENDOR_LOOP_MIDDLE;
	UINT16 offset = MAILBOX_CMD_VENDORCMD_ADDRESS;
	UINT32 valueLength = 0;
	PVENDOR_CUSTOM_CMD CmdContext;
	UINT32 DspAddress = 0;
	UINT8 Type = 0;
	UINT8 state;
	UINT32 WriteData = 0;
	UINT32 CriticalLoop;

	CriticalLoop = loop/4;

	if(FALSE == BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteCmdToMailBox--Error,driver not started\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	while(--loop)
	{
		if(FALSE == BtIsPluggedIn(devExt))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteCmdToMailBox--Error,driver not started2\n");
			return STATUS_UNSUCCESSFUL;
		}
		if(loop%CriticalLoop == 0)
		{	
			BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteCmdToMailBox---Too much retries, delay a while,cmd type:0x%2x, loop:%lu\n",CmdType,loop);
			BtDelay(2000);
		}
		state = VendorCmdActivateByte(devExt);
		if(state == MAILBOX_CMD_INVALID)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteCmdToMailBox---can't not get valid status of cmd reg\n");
			devExt->DriverHaltFlag = 1;
			return STATUS_UNSUCCESSFUL;
		}
		else if(state == 0)
			break;
	}
	if (loop == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "VendorCmdWriteCmdToMailBox--mailbox cmd busy\n");
		return STATUS_UNSUCCESSFUL;
	}
	switch (CmdType)
	{
		case MAILBOX_CMD_TYPE_NULL:
			valueLength = 1;
			break;
		case MAILBOX_CMD_TYPE_READ_3DSP_REGISTER:
			CmdContext = (PVENDOR_CUSTOM_CMD)value;
			DspAddress = CmdContext->CmdAddress;
			Type = MAILBOX_CMD_TYPE_READ_3DSP_REGISTER;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 1, (PVOID)(&DspAddress), 2))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			break;
		case MAILBOX_CMD_TYPE_WRITE_3DSP_REGISTER:
			CmdContext = (PVENDOR_CUSTOM_CMD)value;
			DspAddress = CmdContext->CmdAddress;
			Type = MAILBOX_CMD_TYPE_WRITE_3DSP_REGISTER;
			WriteData = CmdContext->CmdData;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 1, (PVOID)(&DspAddress), 2))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 3, (PVOID)(&WriteData), 4))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			valueLength = 7;
			return STATUS_SUCCESS;
			break;
		case MAILBOX_CMD_TYPE_READ_PROGRAM_REGISTER:
			CmdContext = (PVENDOR_CUSTOM_CMD)value;
			DspAddress = CmdContext->CmdAddress;
			Type = MAILBOX_CMD_TYPE_READ_PROGRAM_REGISTER;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 1, (PVOID)(&DspAddress), 2))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			break;
		case MAILBOX_CMD_BTINITDSPPARAMETERS:
			Type = MAILBOX_CMD_BTINITDSPPARAMETERS;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 1;
			break;
		case MAILBOX_CMD_QUERY_SPACE_INFO:
			Type = MAILBOX_CMD_QUERY_SPACE_INFO;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 1;
			break;	
		case MAILBOX_CMD_REQUEST_TO_JOIN:
			Type = MAILBOX_CMD_REQUEST_TO_JOIN;
			BT_DBGEXT(ZONE_USB | LEVEL1, "Mailbox cmd join req....,combo state is %d\n",*(PUINT8)value);
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 1, value, 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 2;
			break;
		case VCMD_API_8051_JUMP_MIN_PROCESS:
			Type = VCMD_API_8051_JUMP_MIN_PROCESS;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset + 1, value, 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 2;
			break;
			
		case VCMD_API_8051_JUMP_MAIN_PROCESS:
			Type = VCMD_API_8051_JUMP_MAIN_PROCESS;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 1;
			break;
		case MAILBOX_CMD_FLUSH_TX_FIFO_MASTER:
		case MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE:
		case MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF:
		case MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE1:
		case MAILBOX_CMD_FLUSH_TX_FIFO_SCO:
		case MAILBOX_CMD_READY_BEGIN_WORK:
		case MAILBOX_CMD_GET_8051_VERSION:
		case MAILBOX_CMD_GET_DSP_VERSION:
		case MAILBOX_CMD_BT_PARAMETER_READY:
			Type = CmdType;
			if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, (PVOID)(&Type), 1))
			{
				return STATUS_UNSUCCESSFUL;
			}
			return STATUS_SUCCESS;
			valueLength = 1;
			break;
		default:
	            BT_DBGEXT(ZONE_USB | LEVEL1, "INVALID CMD %02x fails\n", CmdType);
			return STATUS_INVALID_PARAMETER;
	}
	if (STATUS_SUCCESS != VendorCmdBurstRegWrite2(devExt, offset, value, valueLength))
	{
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}

/*
write 3dsp register with dword size
single mode is used for the write
 */
NTSTATUS VendorCmdWrite3DSPDword(PBT_DEVICE_EXT devExt, IN UINT32 value, IN UINT16 offset)
{
	NTSTATUS status;
	status = UsbWriteTo3DspReg(devExt, offset, value);
	return status;
}

UINT32 VendorCmdRead3DSPDword(PBT_DEVICE_EXT devExt, IN UINT16 offset)
{
	NTSTATUS status;
	UINT32 RetData = 0;
	status = UsbReadFrom3DspReg(devExt, offset, (PVOID) &RetData);
	if (!NT_SUCCESS(status))
	{
		RetData = 0xFFFFFFFF;
	}
	return RetData;
}

NTSTATUS UsbResetDevice(PBT_DEVICE_EXT devExt)
{
	return STATUS_SUCCESS;
}





NTSTATUS Vcmd_Transmit_DSPFw_Head(PBT_DEVICE_EXT devExt, UINT16 offset, UINT32 len, UINT8 file_id)
{
	NTSTATUS status;
	UINT8 SrcPortNo, DstPortNo; /*Sourc/Destination Port */
	UINT32 secret_steering_bit; /*Secret Bit*/
	UINT32 dma_src = 0; /*DMA Source address*/
	UINT32 dma_dst = 0; /*DMA destination address*/
	UINT32 destoffset = 0; /*Destination offset*/
	UINT32 dma_control_word = 0; /*DMA control word*/
	UINT32 ulCounter, state;
	SrcPortNo = SHUTTLEBUS__PORT_CARDBUS;
	DstPortNo = SHUTTLEBUS__PORT_UNIPHY;
	ASSERT(len <= 1024);
	switch (file_id)
	{
		case SPX_P_MEM:
			secret_steering_bit = 0;
			destoffset = (UINT32)(offset / 4); //should be consumed/4
			dma_control_word = REG_CSR_DMA0_CONTROL(SHUTTLE_PRIORITY_4, SHUTTLE_FIFO_OFF, SHUTTLE_HIP_OFF, 0, 0, SHUTTLE_BURST_SIZE_2, len, SHUTTLE_R_STOP_OFF, SHUTTLE_W_STOP_OFF, SHUTTLE_R_CTRL_OFF, SHUTTLE_MODE_TX_NORMAL, SHUTTLE_W_LINK_OFF, SHUTTLE_W_CTRL_ON);
			dma_dst = REG_CSR_DMA0_DST_ADDR(DstPortNo, secret_steering_bit | destoffset);
			break;
		case SPX_A_MEM:
			secret_steering_bit = 0;
			destoffset = (offset);
			dma_control_word = REG_CSR_DMA0_CONTROL(SHUTTLE_PRIORITY_4,  /* _priority */
			SHUTTLE_FIFO_OFF,  /* _fifo */
			SHUTTLE_HIP_OFF,  /* _hiperf */
			0,  /* _shuttle2host */
			0,  /* _host2shuttle */
			SHUTTLE_BURST_SIZE_2,  /* _burstsize */
			len,  /* _numofbytes */
			SHUTTLE_R_STOP_OFF,  /* _readstop */
			SHUTTLE_W_STOP_OFF,  /* _writestop */
			SHUTTLE_R_CTRL_OFF,  /* _readctl */
			SHUTTLE_MODE_TX_NORMAL,  /* _mode */
			SHUTTLE_W_LINK_OFF,  /* _writelink */
			SHUTTLE_W_CTRL_OFF /* _writectl */
			);
			dma_dst = REG_CSR_DMA0_DST_ADDR(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		case SPX_B_MEM:
			secret_steering_bit = 0x08000000;
			destoffset = (offset);
			dma_control_word = REG_CSR_DMA0_CONTROL(SHUTTLE_PRIORITY_4,  /* _priority */
			SHUTTLE_FIFO_OFF,  /* _fifo */
			SHUTTLE_HIP_OFF,  /* _hiperf */
			0,  /* _shuttle2host */
			0,  /* _host2shuttle */
			SHUTTLE_BURST_SIZE_2,  /* _burstsize */
			len,  /* _numofbytes */
			SHUTTLE_R_STOP_OFF,  /* _readstop */
			SHUTTLE_W_STOP_OFF,  /* _writestop */
			SHUTTLE_R_CTRL_OFF,  /* _readctl */
			SHUTTLE_MODE_TX_NORMAL,  /* _mode */
			SHUTTLE_W_LINK_OFF,  /* _writelink */
			SHUTTLE_W_CTRL_OFF /* _writectl */
			);
			dma_dst = REG_CSR_DMA0_DST_ADDR(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		case SPX_T_MEM:
			secret_steering_bit = 0x04000000;
			destoffset = (offset);
			dma_control_word = REG_CSR_DMA0_CONTROL(SHUTTLE_PRIORITY_4,  /* _priority */
			SHUTTLE_FIFO_OFF,  /* _fifo */
			SHUTTLE_HIP_OFF,  /* _hiperf */
			0,  /* _shuttle2host */
			0,  /* _host2shuttle */
			SHUTTLE_BURST_SIZE_2,  /* _burstsize */
			len,  /* _numofbytes */
			SHUTTLE_R_STOP_OFF,  /* _readstop */
			SHUTTLE_W_STOP_OFF,  /* _writestop */
			SHUTTLE_R_CTRL_OFF,  /* _readctl */
			SHUTTLE_MODE_TX_NORMAL,  /* _mode */
			SHUTTLE_W_LINK_OFF,  /* _writelink */
			SHUTTLE_W_CTRL_OFF /* _writectl */
			);
			dma_dst = REG_CSR_DMA0_DST_ADDR(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		default:
			break;
	}
	dma_src = REG_CSR_DMA0_SRC_ADDR(SrcPortNo, PCI_STEERING_BIT |  \
		((UINT32)(DOWNLOAD_DSP_FW_FIELD_OFFSET)) >> 2);
	status = VendorCmdWrite3DSPDword(devExt, dma_src, WLS_CSR__DMA0_SRC_ADDR);
	if (STATUS_SUCCESS != status)
	{
		return status;
	}
	status = VendorCmdWrite3DSPDword(devExt, dma_dst, WLS_CSR__DMA0_DST_ADDR);
	if (STATUS_SUCCESS != status)
	{
		return status;
	}
	status = VendorCmdWrite3DSPDword(devExt, dma_control_word, WLS_CSR__DMA0_CONTROL);
	if (STATUS_SUCCESS != status)
	{
		return status;
	}
	ulCounter = 1000;
	while(--ulCounter)
	{
		state = VendorCmdRead3DSPDword(devExt, WLS_CSR__STATUS);
		if(state == 0xFFFFFFFF)//failed to read, should retrun unsuccessful
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Vcmd_Transmit_DSPFw_Head---failed to read WLS_CSR__STATUS\n");
			return STATUS_UNSUCCESSFUL;
		}
		else if(state&BITS_STATUS__DMA0_BUSY)
		{
			continue;
		}
		else
			break;
	}
	if(ulCounter == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "Dma busy!\n");
	}
	
	return STATUS_SUCCESS;
}

/*
Soft reset and leave NIC in a sleep mode.  Leave UniPHY core in reset.
 */
NTSTATUS Vcmd_CardBusNICReset(PBT_DEVICE_EXT devExt)
{
	UINT32 state = 0;
	
	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_RESET_WLAN_SUBSYSTEM_BIT |
			PCI_RESET_WLAN_SYSTEM_BIT;


	if(STATUS_SUCCESS != VendorCmdWrite3DSPDword(devExt,state,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "@@@@@@@@#Vcmd_CardBusNICReset 2 \n");
		return STATUS_UNSUCCESSFUL;
	}

	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_SUBSYSTEM_BIT |
			PCI_SLEEP_WLAN_SYSTEM_BIT |
			PCI_SLEEP_MAC_GATED_BIT |
			PCI_SLEEP_MAC_BIT;

	return (VendorCmdWrite3DSPDword(devExt,state,WLS_CSR__PCI_CONTROL_REGISTER));
	

}

NTSTATUS Vcmd_PowerSave_CardBusNICReset(PBT_DEVICE_EXT devExt)
{
	UINT32 state = 0;

	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_RESET_WLAN_SUBSYSTEM_BIT |
			PCI_RESET_WLAN_SYSTEM_BIT;


	if(STATUS_SUCCESS != VendorCmdWrite3DSPDword(devExt,state,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "@@@@@@@@#Vcmd_CardBusNICReset 3 \n");
		return STATUS_UNSUCCESSFUL;
	}

	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_SUBSYSTEM_BIT |
			PCI_SLEEP_WLAN_SYSTEM_BIT |
			PCI_SLEEP_MAC_GATED_BIT |
			PCI_SLEEP_MAC_BIT;

	return (VendorCmdWrite3DSPDword(devExt,state,WLS_CSR__PCI_CONTROL_REGISTER));
	
}



/*
get vendor command byte from command space(wlan - 0x108, bt - 0x128)
command byte is written by host and cleaned by 8051 after command finished.
 */
UINT8 Vcmd_ReadVenderCmdFlag(PBT_DEVICE_EXT devExt)
{
	return (UINT8)VendorCmdRead3DSPDword(devExt, REG_MAILBOX_CMD);
}

/*
check if previous vendor command has been finished
return value:
true:  indicate previous command has been done.
false: indicate previous command has not been done.
 */
BOOLEAN VCMD_VenderCmd_BUSY(PBT_DEVICE_EXT devExt)
{
	UINT8 tmp;
	tmp = Vcmd_ReadVenderCmdFlag(devExt);
	tmp &= (UINT8)VENDERCMD_BUSY_BITS;
	return (tmp == 0) ? FALSE : TRUE;
}

/*
description:
send vendor command api to 8051.
the command is sent to vendor space of mailbox
 */
NTSTATUS Vcmd_Send_API(PBT_DEVICE_EXT devExt, PUINT8 buff, UINT16 len)
{
	UINT32 loop = VENDOR_LOOP_MIDDLE;
	NTSTATUS status = STATUS_SUCCESS;
	while (VCMD_VenderCmd_BUSY(devExt) && --loop)
		;
	if (loop == 0)
	{
		return STATUS_UNSUCCESSFUL;
	}
	if (len > VENDOR_CMD_HEAD_BYTES && len <= VENDOR_CMD_LENGTH_BYTES)
	{
		status = VendorCmdBurstRegWrite(devExt, (UINT16)(REG_MAILBOX_DATA + VENDOR_CMD_HEAD_BYTES), buff + VENDOR_CMD_HEAD_BYTES, len - VENDOR_CMD_HEAD_BYTES);
	}
	if (STATUS_SUCCESS != status)
	{
		return status;
	}
	status = VendorCmdBurstRegWrite(devExt, (UINT16)REG_MAILBOX_CMD, buff, VENDOR_CMD_HEAD_BYTES);
	return status;
}


/*
Name					: mmacSp20ResetRemoved()
return value			: void
global variables 		: void
description			: It takes sp-20 out of reset.
the functoin should be called if Vcmd_mmacSp20ResetApplied has called in advanced.
special considerations 	:
see also				:
 */
NTSTATUS Vcmd_mmacSp20ResetRemoved(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT32 state;
	state = VendorCmdRead3DSPDword(devExt, WLS_CSR__PCI_CONTROL_REGISTER);
	
	state &= (~(PCI_RESET_WLAN_CORE_BIT)); /* relese reset */
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);
	BtDelay(100);
	BT_DBGEXT(ZONE_USB | LEVEL3, "Resetting SP20 in Vcmd_mmacSp20ResetRemoved\n");
	return status;

}

NTSTATUS Vcmd_CardBusNICStart(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT32 val = 0;
	UINT32 loop = 1000;
	UINT32	state;
	state = DMA0_ENABLE_BIT;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);
	val = 0;
	while (--loop && (MMAC_CORE_RDY != VendorCmdRead3DSPDword(devExt, WLS_STRACTH__SP20_READY)))
	{
		BtDelay(1); /* wait */
	}
	if (loop == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "start card bus nic fail \n");
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_USB | LEVEL2, "start card bus nic ok: loop = %lu \n", loop);
	return STATUS_SUCCESS;

}

NTSTATUS Vcmd_NIC_INTERRUPT_DISABLE(PBT_DEVICE_EXT devExt)
{
	UINT32 state;
	NTSTATUS status;
	state = 0;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__FUNCTION_EVENT_MASK);
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__INTERRUPT_ENABLE);
	return status;
}

NTSTATUS Vcmd_NIC_RESET_ALL_BUT_HARDPCI(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT32 state;
	state = PCI_RESET_WLAN_CORE_BIT |  \
		PCI_RESET_WLAN_SUBSYSTEM_BIT | PCI_RESET_WLAN_SYSTEM_BIT | PCI_SOFT_RESET_PCI_BIT |  \
		PCI_SLEEP_MAC_BIT;
	state = state | VendorCmdRead3DSPDword(devExt, WLS_CSR__PCI_CONTROL_REGISTER);
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);
	return status;
}

/*
Name					: mmacRestart_initial()
return value			: MMACSTATUS
global variables 		: void
description				: It initializes MiniMAC.
special considerations 	: It needs to be preceeded by mac_init().
see also				:
 */
NTSTATUS Adap_mmacRestart_initial(PBT_DEVICE_EXT devExt, UINT32 channel)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT32 val;

	val = 2; // 2
	if (STATUS_SUCCESS != VendorCmdWrite3DSPDword(devExt, val, WLS_STRACTH__SP20_READY))
	{
		goto error_exit;
	}
	if (STATUS_SUCCESS != Vcmd_mmacSp20ResetRemoved(devExt))
	{
		goto error_exit;
	}
	if (STATUS_SUCCESS != Vcmd_CardBusNICStart(devExt))
	{
		goto error_exit;
	}



	val = VendorCmdRead3DSPDword(devExt, WLS_CSR__PCI_CONTROL_REGISTER);
	val &= ~(BIT20|BIT21|BIT22|BIT23|BIT24);
	if(STATUS_SUCCESS !=VendorCmdWrite3DSPDword(devExt, val,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		goto error_exit;
	}

#if 0
	{
		UINT32 state;
		UINT32 loop = 1000;
		while(loop-- && (MMAC_CORE_RDY != VendorCmdRead3DSPDword(DeviceObject, (UINT16)(SCRATCH_MEM_BASE + BASEBAND_TEST_SCRATCH_OFFSET + STRACTH_SP20_READY)))) {
			BtDelay(1); /* wait */
		}

		if(loop == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "start card bus nic fail \n");
			return STATUS_UNSUCCESSFUL;
		}

	}
#endif	
	return status;
	error_exit:
	return STATUS_UNSUCCESSFUL;

}

/*	description				: It puts sp-20 in reset.
for v4 chip, maybe is unuseful. please refer to mmacV4Init() of pci driver.
Vcmd_mmacSp20ResetRemoved should be called later if the function called
 */
NTSTATUS Vcmd_mmacSp20ResetApplied(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	UINT32 state;
	/*
	Do not change the following order.
	 */
	state = VendorCmdRead3DSPDword(devExt, WLS_CSR__PCI_CONTROL_REGISTER);
	state |= (UINT32)PCI_SLEEP_WLAN_CORE_BIT;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);
	state |= (UINT32)PCI_SLEEP_WLAN_SUBSYSTEM_BIT;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);
	state |= (UINT32)PCI_RESET_WLAN_CORE_BIT;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER);

	return status;
}




NTSTATUS Vcmd_NIC_ENABLE_RETRY(PBT_DEVICE_EXT devExt, UINT32 value)
{
	return VendorCmdWrite3DSPDword(devExt, value, WLS_CSR__NUMBER_OF_RETRY);
}

NTSTATUS Vcmd_NIC_INTERRUPT_ENABLE(PBT_DEVICE_EXT devExt)
{
	UINT32 state;
	NTSTATUS status;
	state = (UINT32)WLS_INT_MASK;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__CLEAR_STATUS);
	/* WriteToReg(pAdapter,WLS_CSR_CLEARSTATUS_WD,WLS_INT_MASK); */
	state = (UINT32)BIT15;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__FUNCTION_EVENT);
	/* WriteToReg(pAdapter,WLS_CSR_FUNCTION_EVENT_CLR_WD,0x8000); */
	state = (UINT32)BIT15;
	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__FUNCTION_EVENT_MASK);
	/* WriteToReg(pAdapter,WLS_CSR_FUNCTION_EVENT_MASK_WD,0x8000); */
	state = (UINT32)(WLS_INT_MASK);
	state |= MAILBOX_IEN;

	status = VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__INTERRUPT_ENABLE);
	/* WriteToReg(pAdapter,WLS_CSR_IE_WD,WLS_INT_MASK); */
	return status;
}

/*******************************************************************
 *   Vcmd_Set_Firmware_Download_Ok
 *
 *   Descriptions:
 *      The routine of Firmware_Download_Ok contorl command.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
NTSTATUS Vcmd_Set_Firmware_Download_Ok(PBT_DEVICE_EXT devExt)
{
	DSP_FW_DOWN_OK_T val;
	val.request_cmd = VCMD_API_RELEASE_CODE_DSPCODE;
	return (Vcmd_Send_API(devExt, (PUINT8) &val, sizeof(DSP_FW_DOWN_OK_T)));
}

NTSTATUS Vcmd_CardBusNICEnable(PBT_DEVICE_EXT devExt)
{
	UINT32	state;
	state  = PCI_RESET_WLAN_CORE_BIT |DMA0_ENABLE_BIT;
	return (VendorCmdWrite3DSPDword(devExt, state, WLS_CSR__PCI_CONTROL_REGISTER));

}

NTSTATUS UsbReadFrom3DspReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, OUT PUINT32 Datas)
{
	PUSB_INT_STACK pStack = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	VENDOR_CUSTOM_CMD CmdContent;
	LARGE_INTEGER TimeOut;
	UINT32 Loop = 3;
	CmdContent.CmdAddress = (UINT16)offset;
	CmdContent.CmdType = MAILBOX_CMD_TYPE_READ_3DSP_REGISTER;
	CmdContent.CmdData = 0;

    atomic_set(&devExt->UsbContext.UsbIntContext.IntEventFlag, EVENT_UNSIGNED);
	do
	{
		Status = VendorCmdWriteCmdToMailBox(devExt, (PVOID) &(CmdContent), MAILBOX_CMD_TYPE_READ_3DSP_REGISTER);
		if (!NT_SUCCESS(Status))
		{
			continue;
		}

		TimeOut.QuadPart =  2 * HZ;
		Status = wait_event_interruptible_timeout(devExt->UsbContext.UsbIntContext.IntEvent, atomic_read(&(devExt->UsbContext.UsbIntContext.IntEventFlag)) == EVENT_SIGNED,TimeOut.QuadPart);
		atomic_set(&devExt->UsbContext.UsbIntContext.IntEventFlag, EVENT_UNSIGNED);

        	if (Status == 0)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "Read DSP reg timeout, exception!!");
			continue;
		}
		pStack = &devExt->UsbContext.UsbIntContext.UsbIntStack[0];
		if (pStack->DataType == 0x02)
		{

			*Datas = pStack->DataContent;
			pStack->DataAddress = 0;
			pStack->DataContent = 0;
			pStack->DataType = 0;
			return STATUS_SUCCESS;

		}
	}
	while (Loop-- > 0);
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS UsbReadFrom3DspRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT8 DataNum, OUT PUINT8 Datas)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT8 i = 0;
	for (i = 0; i < DataNum; i++)
	{
		Status = UsbReadFrom3DspReg(devExt, offset + 4 * i, (PUINT32)((PUINT8)Datas + 4 * i));
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
	}
	return Status;
}

NTSTATUS UsbWriteTo3DspReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT32 Datas)
{
	NTSTATUS Status = STATUS_SUCCESS;
	VENDOR_CUSTOM_CMD CmdContent;
	CmdContent.CmdAddress = (UINT16)offset;
	CmdContent.CmdData = Datas;
	CmdContent.CmdType = MAILBOX_CMD_TYPE_WRITE_3DSP_REGISTER;
	Status = VendorCmdWriteCmdToMailBox(devExt, (PVOID) &CmdContent, MAILBOX_CMD_TYPE_WRITE_3DSP_REGISTER);
	return Status;
}

NTSTATUS UsbWriteTo3DspRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT8 DataNum, IN PVOID Datas)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT8 i;
	UINT32 Value = 0;
	for (i = 0; i < DataNum; i++)
	{
		Value = *((PUINT32)Datas + i);
		Status = UsbWriteTo3DspReg(devExt, offset + 4 * i, Value);
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
	}
	return Status;
}




UINT16 Usb_Read_2Bytes_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 reg) 
{	
	UINT32 div, mod;
	UINT8 regvalue[8] = {0};
	div = reg / 4;
	mod = reg % 4;
	if (mod < 3)
	{ 
		UsbReadFrom3DspReg(devExt, div * 4, (PUINT32)regvalue); 
	}
	else
	{ 
		UsbReadFrom3DspReg(devExt, div * 4, (PUINT32)regvalue); 
		UsbReadFrom3DspReg(devExt, div * 4 + 4, (PUINT32)(regvalue + 4)); 
	}
	return *((PUINT16)(regvalue + mod)); 
}
UINT32 Usb_Read_4Bytes_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 reg) 
{ 
	UINT32 div, mod;
	UINT8 regvalue[8] = {0};
	div = reg / 4;
	mod = reg % 4;
	if (mod == 0)
	{ 
		UsbReadFrom3DspReg(devExt, div * 4, (PUINT32)regvalue); 
	}
	else
	{ 
		UsbReadFrom3DspReg(devExt, div * 4, (PUINT32)regvalue); 
		UsbReadFrom3DspReg(devExt, div * 4 + 4, (PUINT32)(regvalue + 4)); 
	}
	return *((PUINT32)(regvalue + mod)); 
} 




NTSTATUS UsbReadFromProgramReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, OUT PUINT32 Datas)
{
	PUSB_INT_STACK pStack = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	VENDOR_CUSTOM_CMD CmdContent;
	LARGE_INTEGER TimeOut;
	UINT32 Loop = 5;

	CmdContent.CmdAddress = (UINT16)offset;
	CmdContent.CmdType = MAILBOX_CMD_TYPE_READ_PROGRAM_REGISTER;
	CmdContent.CmdData = 0;
	do
	{
		Status = VendorCmdWriteCmdToMailBox(devExt, (PVOID) &(CmdContent), MAILBOX_CMD_TYPE_READ_PROGRAM_REGISTER);
		if (!NT_SUCCESS(Status))
		{
			continue;
		}

		TimeOut.QuadPart =  5 * HZ;
		atomic_set(&devExt->UsbContext.UsbIntContext.IntEventFlag, EVENT_UNSIGNED);
		Status = wait_event_interruptible_timeout(devExt->UsbContext.UsbIntContext.IntEvent, 
							atomic_read(&(devExt->UsbContext.UsbIntContext.IntEventFlag)) == EVENT_SIGNED,TimeOut.QuadPart);
		if (Status == 0)
		{
			continue;
		}

		pStack = &devExt->UsbContext.UsbIntContext.UsbIntStack[1];
		if (pStack->DataType == 0x02)
		{
			*Datas = pStack->DataContent;
			pStack->DataAddress = 0;
			pStack->DataContent = 0;
			pStack->DataType = 0;
			return STATUS_SUCCESS;
		}
	}
	while (Loop-- > 0);
	return STATUS_UNSUCCESSFUL;
}


NTSTATUS UsbReadFromProgramRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT16 DataNum, OUT PUINT8 Datas)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT16 i = 0;
	for (i = 0; i < DataNum; i++)
	{
		Status = UsbReadFromProgramReg(devExt, offset + 4 * i, (PUINT32)((PUINT8)Datas + 4 * i));
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
	}
	return Status;
}



NTSTATUS UsbInitDspPara(PBT_DEVICE_EXT devExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_BTINITDSPPARAMETERS);
	return Status;
}

/*****************************************************************
*Changelog:
*Jakio20080801:
*	Change cmd length from one byte to four bytes. Because one byte can cause control
*	pipe stall frequently, but we don't know why now.  
*****************************************************************/
NTSTATUS UsbQueryDMASpace(PBT_DEVICE_EXT devExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT16	offset;
	UINT8	tmpchar[4];
	UINT32	LoopNum = 10;
	PBT_FRAG_T	pFrag = NULL;

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
		return Status;


	offset = 0x130;
	tmpchar[0] = 0x01;

	if(devExt->SpaceQueryFlag == TRUE)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "UsbQueryDMASpace--Another operation is in the way, just return\n");
		return STATUS_UNSUCCESSFUL;
	}
	devExt->SpaceQueryFlag = TRUE;

	while(--LoopNum)
	{
		Status = VendorCmdBurstRegWriteAnyn(devExt, offset, tmpchar, 4, VENDOR_CMD_QUERY_DMA_SPACE);
		if(!NT_SUCCESS(Status))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "UsbQueryDMASpace fail\n");
		}
		else
			break;
	}
	if(LoopNum == 0)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "UsbQueryDMASpace---fatal error, query retry limit occurs\n");
		return STATUS_UNSUCCESSFUL;
	}
	return Status;
}	


NTSTATUS VendorCmdBurstRegWriteAnyn(PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, 
								IN UINT32 Length, UINT32 cmdSrc)
{
	NTSTATUS status;
	KIRQL	oldIrql;
	PVENDOR_CMD_PARAMETERS pVenderCmdParameters;
	if(devExt == NULL)
		return STATUS_UNSUCCESSFUL;
	KeAcquireSpinLock(&devExt->AsynCmdLock, &oldIrql);
	pVenderCmdParameters = (PVENDOR_CMD_PARAMETERS)QueuePopHead(&devExt->UsbContext.AsyVcmdPara_FreeList);
	KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);
	if(pVenderCmdParameters == NULL)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Failed to  allocate vendor cmd para resource\n");
		return STATUS_UNSUCCESSFUL;
	}

	if(Length > MAX_BURST_WRITE_LENGTH)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Error: length beyond the limit, value:%lu\n", Length);
		return STATUS_UNSUCCESSFUL;
	}
	
	RtlZeroMemory(pVenderCmdParameters, sizeof(struct _VENDOR_CMD_PARAMETERS));
	pVenderCmdParameters->bIn = VENDOR_CMD_OUT;
	pVenderCmdParameters->RequestType = 0;
	pVenderCmdParameters->bShortOk = 0;
	pVenderCmdParameters->Index = address;
	pVenderCmdParameters->ReservedBits = 0;
	RtlCopyMemory(pVenderCmdParameters->AsynTranBuffer, pValue, Length);
	pVenderCmdParameters->TransferBufferLength = Length;
	pVenderCmdParameters->Value = 0;
	pVenderCmdParameters->Request = VENDOR_CMD_BURST_WRITE_REG;
	status = UsbVendorRequestAsyn(devExt, pVenderCmdParameters, cmdSrc);
	return STATUS_SUCCESS;
}


NTSTATUS UsbVendorRequestAsyn(PBT_DEVICE_EXT devExt, PVENDOR_CMD_PARAMETERS VenderCmdParameters, UINT32 cmdSrc)
{
	PURB urb;

	PVENDOR_ASYN_CONTEXT AsynContext;
	PBTUSB_URB_BLOCK_T	pUrbBlock;
	KIRQL	oldIrql;
	unsigned int pipe;
	INT32		ret;

	KeAcquireSpinLock(&devExt->AsynCmdLock, &oldIrql);
	
	pUrbBlock = (PBTUSB_URB_BLOCK_T)QueuePopHead(&devExt->UsbContext.AsyUrb_FreeList);
	if(pUrbBlock == NULL)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Failed to get urb resource\n");
		QueuePutTail(&devExt->UsbContext.AsyVcmdPara_FreeList, &VenderCmdParameters->Link);
		KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	urb = (PURB)pUrbBlock->ptotaldata;
	
	AsynContext = (PVENDOR_ASYN_CONTEXT)QueuePopHead(&devExt->UsbContext.AsyContext_FreeList);
	if(AsynContext == NULL)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Failed to get asyn context resource\n");
		QueuePutTail(&devExt->UsbContext.AsyVcmdPara_FreeList, &VenderCmdParameters->Link);
		QueuePutTail(&devExt->UsbContext.AsyUrb_FreeList, &pUrbBlock->Link);
		KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);

	AsynContext->DeviceExtension = devExt;
	AsynContext->pUrbBlock = pUrbBlock;
	AsynContext->cmdSrc = cmdSrc;
	AsynContext->pVendPara = (PUINT8)VenderCmdParameters;

	//fill setup packet
	AsynContext->setup.bRequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	AsynContext->setup.bRequest = VENDOR_CMD_BURST_WRITE_REG;
	AsynContext->setup.wValue = cpu_to_le16p(&VenderCmdParameters->Value);
	AsynContext->setup.wIndex = cpu_to_le16p(&VenderCmdParameters->Index);
	AsynContext->setup.wLength = cpu_to_le16p((PUINT16)&VenderCmdParameters->TransferBufferLength);
	/*
	if (VenderCmdParameters->bIn)
	{
		pipe = usb_rcvctrlpipe(devExt->usb_device, INDEX_EP_CTRL);
		if (FALSE == VenderCmdParameters->bShortOk)
		{
			transferflag |= URB_SHORT_NOT_OK;
		}
	}
	else
	{
		pipe = usb_sndctrlpipe(devExt->usb_device, INDEX_EP_CTRL);
	}
	*/
	pipe = usb_sndctrlpipe(devExt->usb_device, INDEX_EP_CTRL);
	usb_fill_control_urb(urb,
					devExt->usb_device,
					pipe,
					(PUINT8)&AsynContext->setup,	//Need to be confirmed for this var
					VenderCmdParameters->AsynTranBuffer,
					VenderCmdParameters->TransferBufferLength,
					Usb_VendorAsynCompletion_Imp,
					(void *)AsynContext
					);
	
	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if(ret == 0)
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "UsbVendorRequestAsyn---Error, submit urb failed\n");
		KeAcquireSpinLock(&devExt->AsynCmdLock, &oldIrql);
		QueuePutTail(&devExt->UsbContext.AsyVcmdPara_FreeList, &VenderCmdParameters->Link);
		QueuePutTail(&devExt->UsbContext.AsyUrb_FreeList, &pUrbBlock->Link);
		QueuePutTail(&devExt->UsbContext.AsyContext_FreeList, &AsynContext->Link);
		KeReleaseSpinLock(&devExt->AsynCmdLock, oldIrql);

		return STATUS_UNSUCCESSFUL;
	}
}



/*******************************************************************
 *RegApi_Fill_Cmd_Body
 *	fill an element field, refer doc for  the format of element 
 *return:
 *	this routine returns addr of next element
********************************************************************/
PUINT8 RegApi_Fill_Cmd_Body(PBT_DEVICE_EXT devExt, UINT8 subCmd, UINT16 addr, PUINT8 destBuf, UINT16 len, PUINT8 srcBuf)
{
	PBTUSB_REG_API_ELE		pEle;

	ASSERT(len <= 48);

	pEle = (PBTUSB_REG_API_ELE)destBuf;

	pEle->regAddr = addr;
	pEle->operand.subCmd = subCmd;
	pEle->operand.eleLen = (UINT8)len;
	RtlCopyMemory(pEle->data, srcBuf, len);

	BTUSB_REGAPI_NEXT_ELE(pEle);
	
	return (PUINT8)pEle;
	
}


PUINT8 RegApi_Fill_Cmd_Body_Constant(PBT_DEVICE_EXT devExt, UINT8 subCmd, UINT16 addr, PUINT8 destBuf, UINT16 len, UINT32 value)
{
	PBTUSB_REG_API_ELE		pEle;

	ASSERT(len <= 48);

	pEle = (PBTUSB_REG_API_ELE)destBuf;

	pEle->regAddr = addr;
	pEle->operand.subCmd = subCmd;
	pEle->operand.eleLen = (UINT8)len;
	RtlCopyMemory(pEle->data, &value, len);

	BTUSB_REGAPI_NEXT_ELE(pEle);
	
	return (PUINT8)pEle;
	
}


/*******************************************************************
 *RegAPI_SendTo_MailBox
 *	send the packet to 8051, para 'buf' is the start_addr of  the first element, 'len'  is the total
 *	length of all elements and cmd header
 *	when sending to 8051, we should write 'cmd type' in the end so that all data are there when
 *	8051 sees the cmd
*******************************************************************/
NTSTATUS RegAPI_SendTo_MailBox(IN PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	UINT32 loop = VENDOR_LOOP_MIDDLE;
	UINT16 offset = MAILBOX_CMD_REGAPI_ADDRESS;
	PBTUSB_REG_API_HEADER	pHead;
	PBTUSB_REG_API_ELE	pEle;
	NTSTATUS				status;
	UINT8 state;
	
	pHead = (PBTUSB_REG_API_HEADER)buf;
	pEle = (PBTUSB_REG_API_ELE)BTUSB_REGAPI_FIRST_ELE(buf);

	pHead->cmdType = 0x04;
	pHead->sumLen = (UINT8)len - sizeof(BTUSB_REG_API_HEADER);


	if(len <= 2)
	{
		BT_DBGEXT(ZONE_USB | LEVEL1, "nothing to write\n");
		return STATUS_SUCCESS;
	}
	if(len > (REGAPI_MAX_BUFFER_LENGTH-2))
	{
		ASSERT(0);
		return STATUS_UNSUCCESSFUL;
	}
#ifdef DBG
#endif	

	if(FALSE == BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "RegAPI_SendTo_MailBox--Error,driver not started\n");
		return STATUS_UNSUCCESSFUL;
	}

	while(--loop)
	{	
		if(FALSE == BtIsPluggedIn(devExt))
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "RegAPI_SendTo_MailBox--Error,driver not started2\n");
			return STATUS_UNSUCCESSFUL;
		}
	
		if(loop == VENDOR_LOOP_MIDDLE/2)
		{	//delay 2 ms
			BT_DBGEXT(ZONE_USB | LEVEL1, "RegAPI_SendTo_MailBox---Too much retries, delay a while\n");
			BtDelay(2000);
		}
		state = VendorCmdRegAPIByte(devExt);
		if(state == MAILBOX_CMD_INVALID)
		{
			BT_DBGEXT(ZONE_USB | LEVEL0, "RegAPI_SendTo_MailBox---can't not get valid status of cmd reg\n");
			/* jakio20080528: mark hardware disconnect
*/
			devExt->DriverHaltFlag = 1;
			return STATUS_UNSUCCESSFUL;
		}
		else if(state == 0)
			break;
	}
	if (loop == 0)
	{	
		BT_DBGEXT(ZONE_USB | LEVEL0, "RegAPI_SendTo_MailBox---mail box cmd busy\n");
		return STATUS_UNSUCCESSFUL;
	}	

	status = VendorCmdBurstRegWrite(devExt, offset+1, buf+1, len-1);
	if(status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "reg api write error1\n");
		return status;
	}


	status = VendorCmdBurstRegWrite2(devExt, offset, &pHead->cmdType, 1);
	if(status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "reg api write error2\n");
		return status;
	}

	return STATUS_SUCCESS;
}

/*******************************************************************
 *RegApi_Write_Cmd_Indicator
 *	this routine is capsulated for writing cmd indicator.
 *return value:
 *	this routine return the next element's addr
*******************************************************************/
PUINT8 RegApi_Write_Cmd_Indicator(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 value)
{
	PBTUSB_REG_API_ELE	pEle;
	UINT32	tempLong;
	UINT8	tempChar;
	pEle = (PBTUSB_REG_API_ELE)buf;


	tempChar = 0x04;
	pEle = (PBTUSB_REG_API_ELE)RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, (PUINT8)pEle, 1, &tempChar);

	tempLong = value;
	pEle = (PBTUSB_REG_API_ELE)RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_OR_OPERATION,
			BT_REG_HCI_COMMAND_INDICATOR, (PUINT8)pEle, 4, (PUINT8)&tempLong);

	tempChar = 0x00;
	pEle = (PBTUSB_REG_API_ELE)RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, (PUINT8)pEle, 1, &tempChar);
	
	return (PUINT8)pEle;
}



/*********************************************
 *this routine is used to print regapi buffer body
**********************************************/
VOID RegAPI_Print_Buffer(PUINT8 buffer)
{
	PBTUSB_REG_API_HEADER	pHeader;
	PBTUSB_REG_API_ELE		pEle;
	UINT16	TotalLen;
	UINT16	EleLen;
	UINT16	i;
	PUINT8	pBody;

	pHeader = (PBTUSB_REG_API_HEADER)buffer;
	pEle = (PBTUSB_REG_API_ELE)BTUSB_REGAPI_FIRST_ELE(buffer);
	TotalLen = pHeader->sumLen;
	EleLen = pEle->operand.eleLen + 3;
	


	BT_DBGEXT(ZONE_USB | LEVEL3, "-----------------------------------------------\n");
	BT_DBGEXT(ZONE_USB | LEVEL3, "Bulk writing registers structure:\n");
	BT_DBGEXT(ZONE_USB | LEVEL3, "Cmd Type: 0x%2x\n", pHeader->cmdType);
	BT_DBGEXT(ZONE_USB | LEVEL3, "Element total length: 0x%2x\n", pHeader->sumLen);
	while(EleLen <= TotalLen)
	{
		BT_DBGEXT(ZONE_USB | LEVEL3, "*---*---*---*---*---*---*---*\n");
		BT_DBGEXT(ZONE_USB | LEVEL3, "Element subcmd: 0x%2x\n", pEle->operand.subCmd);
		BT_DBGEXT(ZONE_USB | LEVEL3, "Element length: 0x%2x\n", pEle->operand.eleLen);
		BT_DBGEXT(ZONE_USB | LEVEL3, "Element addr: 0x%4x\n", pEle->regAddr);
		BT_DBGEXT(ZONE_USB | LEVEL3, "Element body:  ");
		pBody = pEle->data;
		for(i=0; i < pEle->operand.eleLen;i++)
		{
			BT_DBGEXT(ZONE_USB | LEVEL3, "%2x  ",pBody[i]);	
		}
		BT_DBGEXT(ZONE_USB | LEVEL3, "\n");
		BT_DBGEXT(ZONE_USB | LEVEL3, "*---*---*---*---*---*---*---*\n");

		BTUSB_REGAPI_NEXT_ELE(pEle);
		EleLen += pEle->operand.eleLen + 3;
	}
	BT_DBGEXT(ZONE_USB | LEVEL3,"-----------------------------------------------\n");
}






BOOLEAN Vcmd_8051fw_isWork(PBT_DEVICE_EXT devExt)
{
	UINT8 		tmp;

	VendorCmdBurstRegRead2(devExt, REG_UIR, &tmp, sizeof(UINT8));
	BT_DBGEXT(ZONE_USB | LEVEL3, "Vcmd_8051fw_isWork--Reg value:%x\n", tmp);
 	return ((tmp & BIT6) == 0x00);
}


BOOLEAN BtUsb_Device_Removed(PBT_DEVICE_EXT devExt)
{
	UINT8 val;


	if(devExt == NULL)
	{
		return TRUE;
	}
	
	if(STATUS_UNSUCCESSFUL == VendorCmdBurstRegRead2(devExt, REG_USBCTL1, &val, sizeof(UINT8)))
	{
		BT_DBGEXT(ZONE_USB | LEVEL0, "Read REG_UIR failure\n");
		return TRUE;
	}
	if(((val & BIT_PCIRDY) != BIT_PCIRDY) || (val ==0xff) || (val ==0x00))
	{
		BT_DBGEXT(ZONE_USB | LEVEL3, "REG_UIR value is %2x\n", val);
		return TRUE;
	}

	return FALSE;
}


