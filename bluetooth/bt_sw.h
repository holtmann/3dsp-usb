/************************************************************************
 * Copyright (c) 2003, 3DSP Corporation, all rights reserved.  Duplication or
 * reproduction of any part of this software (source code, object code or
 * comments) without the expressed written consent by 3DSP Corporation is
 * forbidden.  For further information please contact:
 *
 * 3DSP Corporation
 * 16271 Laguna Canyon Rd
 * Irvine, CA 92618
 * www.3dsp.com
 *************************************************************************/
#ifndef __BT_SW_H__
#define __BT_SW_H__


#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/pm.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "ntstatus.h"
#include "bt_base.h"
#include "bt_ioctl.h"
#include "bt_version.h"
#include "btusb.h"
#include "bt_defs.h"
#include "bt_queue.h"
//
// These are AMCC's Vendor ID and Device ID
//
// (Note: The VID and DID pre-programmed into the
//  PCI Config ROM on the AMCC 5933 Eval Board changes
//  from time to time.  So this may not be the
//  correct VID and DID for every board).
//
#define BT_PCI_VID 0x3d5b
#define BT_PCI_DID 0x5001

//
// Maxmimum transfer size supported
// 64K sounds right.
//
#define BT_PCI_MAX_TXFER	65535

#define BT_MAX_API_BUF         64


//
// The watchdog interval.  Any request pending
// for this number of seconds is subject to
// being cancelled.  Note, this is a ONE SECOND GRANULARITY TIMER
// so provide a length of time at least one second greater
// than the minimum.
//
#define BT_WATCHDOG_INTERVAL	5


//
// Device State definitions
//
// These values are used to track the state of the device
// with regard to PnP events.
//
// NOTE: The code makes assumptions about the order of these
// states, and the fact that their values are in the high
// word of a longword.
//
//

//
// We use a set of dummy states to delineate the actions we take
// on receipt and completion of an IRP.  These also appear below.
//

//
// In STATE_REMOVED, we immediately fail any received I/O requests
//
#define STATE_REMOVED           0X00000000

//
// In STATE_SURPRISE_REMOVED, we immediately fail all incoming requests
//
#define STATE_SURPRISE_REMOVED   0x00010000

//
// In STATE_NEVER_STARTED, we also immediately fail all incoming requests
//
#define STATE_NEVER_STARTED     0x00020000

//
// Dummy State -- When the state is < this value, no H/W access is
// allowed
//
#define STATE_ALL_BELOW_NO_HW   0x00030000	// ******************

//
// Self state -- Add for the sate when device starts with failure
//
#define STATE_START_FAILED      0x00040000

//
// In STATE_REMOVE_PENDING, we also immediately fail all incoming requests
//
#define STATE_REMOVE_PENDING    0x00100000


//
// Dummy state -- When an IRP arrives at the driver, if the current
// device state is below this value, it is immediately failed
//
#define STATE_ALL_BELOW_FAIL    0x00FF0000	// ******************

//
// In STATE_STARTED, requests are processed and removed from the
// queues normally
//
#define STATE_STARTED           0X01000000

//
// Dummy state -- When an IRP arrives at the driver, if the current
// device state is above this value, it is queued, not initiated on
// the device (even if the device is free)
//
#define STATE_ALL_ABOVE_QUEUE   0x0FFF0000	// ******************

//
// Dummy State -- When an IRP is COMPLETED on the device, if
// the device state is below this value, we'll start another
// IRP in an attempt to drain the queue of pending requests.
//
#define STATE_ALL_BELOW_DRAIN   STATE_ALL_ABOVE_QUEUE	// ******************

//
// In STATE_STOP_PENDING, we queue incoming requests
//
#define STATE_STOP_PENDING      0x10000000

//
// In STATE_STOPPED, we queue incoming requests
//
#define STATE_STOPPED           0x10010000

//
// Initial serial device name and symbolic link device name
//
#define SERIAL_DEVICE_NAME					L"\\Device\\TdspSerial"
#define SERIAL_SYMBOLOC_LINK_DEVICE_NAME	L"\\DosDevices\\COM"
#define SERIAL_SYMBOLOC_LINK_DEVICE_PORT_NAME	L"\\DosDevices\\"
#define SERIAL_DEVICE_NAME_MAX_LEN          128

//
// The count of BD
#define BD_MAX_COUNT                       4
// #define BT_MAX_FRAME_SIZE                  512
//#define BT_MAX_FRAME_SIZE                  1024
#define BT_MAX_FRAME_SIZE                  1280
#define BT_MAX_SCO_FRAME_SIZE              256

//
// Each BD has eight bytes
//
#define EACH_BD_LENGTH                     8

//
// Max count of Rx-cach. We use rx-cach to store the
//
#define MAX_RX_CACH_COUNT                  32

#define MAX_LMP_CACH_COUNT		16


#define USB_PIPETYPE_SCO			0x00
#define USB_PIPETYPE_ACL			0x01

#define USB_READ_IRP_FAIL_TIME_COUNT	120	//jakio20080313: every 2 minutes, we clear rx failure count	


#define MAX_TX_POOL_COUNT                   1
//
// Max count of rx buffer in the second region of scratch memory.
//
#define MAX_RX_BUF_COUNT                  1280

#ifdef BT_SUPPROT_RX_FRAG
// If IRP's length is not enough, driver should frag a frame into several fragments. 
#define BT_ACL_RX_MAX_LIMIT				1008 // If a frame's length is more than this value, driver should frag it.
#define BT_ACL_RX_FRAGMENT_THRESHOLD	508  // The max length that a framgemnt is 
#endif


/*=============================================================*/
/*------ Bit settings -----------------------------------------*/
/*=============================================================*/

#define BIT0	0x00000001
#define BIT1	0x00000002
#define BIT2	0x00000004
#define BIT3	0x00000008
#define BIT4	0x00000010
#define BIT5	0x00000020
#define BIT6	0x00000040
#define BIT7	0x00000080
#define BIT8	0x00000100
#define BIT9	0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000
//  + for absolute / - for relative
#define kOneMillisec (HZ/1000)


// These two macros is used to align data with four bytes
#define ALIGNDATALEN(x)   (UINT16)(((UINT16)x + 3) & 0xfffc)
#define ALIGNLONGDATALEN(x)   (UINT32)(((UINT32)x + 3) & 0xfffffffc)

/**
#define BT_REGISTRY_PARAMETERS_PATH  \
L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\BTCARD"
**/

#define PDSP_ADAPTER_FROM_CONTEXT_HANDLE(Handle) ((PBT_DEVICE_EXT)(Handle))


//define base address to 8051 fw downloaded to
#define DOWNLOAD_FW_FIELD_OFFSET		0x0000    //define it later
#define DOWNLOAD_DSP_FW_FIELD_OFFSET		0x2400    //define it later

//define for mailbox
#define OFFSET_3DSP_CHIP						0x0		//define it later
#define OFFSET_MAILBOX_FLAG_REG				0x100		//define it later
#define OFFSET_8051_RESET_FW_REG				0x100		//define it later

//define 8051 reg
#define OFFSET_8051FW_DMA_SRC_REG					0x100     //define it later
#define OFFSET_8051FW_DMA_DST_REG					0x100
#define OFFSET_8051FW_DMA_CTL_REG					0x100

#define DSP_8051_FIRMWARE_FRAGMENT_LENGTH	    (512)
#define DSP_FIRMWARE_FRAGMENT_LENGTH	    (512)

//Jakio20071009: add for v4
#define DSP_CODE_LENGTH	                       (4*65536+8)
#define BIN_CODE_HEADSZIE                      (0x80)
#define V4_BMEM_ROM_START			(1024*8)
#define V4_BMEM_ROM_LEN			(1024*4)


#define SPX_P_MEM            0
#define SPX_A_MEM            1
#define SPX_B_MEM            2
#define SPX_T_MEM            3


/*
 * Memory-mapped Host Address Space:
 *
 * ----------------------------------------------------------------------
 * |  Region Name			|  Addr[14:0] (Hex)		|  Reserved Size	|			
 * ----------------------------------------------------------------------
 * |  CSR region			|  0x0000 - 0x1FFF		|  8 KB				|
 * ----------------------------------------------------------------------
 * |  Scratchpad region		|  0x2000 - 0x3FFF		|  8 KB				|
 * ----------------------------------------------------------------------
 * |  MAC region			|  0x4000 - 0x7FFF		|  16 KB			|
 * ----------------------------------------------------------------------
 *
 * CSR:  CardBus control and status registers
 *
 */


#define _OFFSET_CSR		0x0000
#define _OFFSET_SPD		0x2000
#define _OFFSET_MAC	0x4000

#define SCRATCH_RAM_OFFSET 1024

#define DSP_TX_BUF_SIZE  2048

/* SB ports */

#define SB_PORT_MAC 0
#define SB_PORT_UNIPHY 1
#define SB_PORT_CARDBUS 3

#define PCI_STEERING_BIT              0x08000000

/************************************************************************
 *																		*
 *					ShuttleBus macros and constants						*
 *																		*
 ************************************************************************/



/* ShuttleBus ports */
#define SHUTTLEBUS__PORT_UNIPHY				1
#define SHUTTLEBUS__PORT_CARDBUS			3






/************************************************************************
 *																		*
 *						DSP-Shuttle Control Word						*
 *																		*
 ************************************************************************/

/*
 * Priority:
 * The Channel number that the request will be put into.  This must be a
 * valid channel number.  Range 0-15 .
 * (0 = highest Priority,14 = lowest Priority,15 = Matrix Definition Channel)
 */
#define SHUTTLE_PRIORITY_0		0
#define SHUTTLE_PRIORITY_1		1
#define SHUTTLE_PRIORITY_2		2
#define SHUTTLE_PRIORITY_3		3
#define SHUTTLE_PRIORITY_4		4
#define SHUTTLE_PRIORITY_5		5
#define SHUTTLE_PRIORITY_6		6
#define SHUTTLE_PRIORITY_7		7
#define SHUTTLE_PRIORITY_8		8
#define SHUTTLE_PRIORITY_9		9
#define SHUTTLE_PRIORITY_10		10
#define SHUTTLE_PRIORITY_11		11
#define SHUTTLE_PRIORITY_12		12
#define SHUTTLE_PRIORITY_13		12
#define SHUTTLE_PRIORITY_14		14
#define SHUTTLE_PRIORITY_15		15


/*
 * FIFO:
 * When this bit is set to 1, the transfer will be part of a FIFO transfer.
 * It is only valid when the corresponding channel is configured as FIFO
 * channel in the hardware.
 */
#define SHUTTLE_FIFO_ON	 		1
#define SHUTTLE_FIFO_OFF	 	0


/*
 * HiP:
 * When this bit is set to 1, the Read Request for a waiting chained command
 * can be sent out as soon as the previous command in the chain has finished
 * using its read buffers.  This increases the performance and ensures that
 * the peripheral that is being read from sees no break in the read requests.
 * If this bit is not set then the waiting chained command must wait for the
 * previous command to finish flushing it write buffers before it can have
 * access to the read buffers.
 * This bit may only be set to '1' when Mode = '10' (chained transfer).
 * In other modes, it has no effect.
 */
#define SHUTTLE_HIP_ON	 		1
#define SHUTTLE_HIP_OFF	 		0



/*
 * Burst Size:
 * 	Encoded version of the burst size to be used;
 *	'000' = Burst size of 1,
 *	'001' = Size of 2,
 *	'010' = Size of 4,
 *	'011' = Size of 8,
 *	'1xx' = reserved for future expansion.
 */
#define SHUTTLE_BURST_SIZE_1		0	// 0000
#define SHUTTLE_BURST_SIZE_2		1	// 0001
#define SHUTTLE_BURST_SIZE_3		2	// 0010
#define SHUTTLE_BURST_SIZE_8		3	// 0011


/*
 * Number of Bytes:
 * Gives the total number of bytes to be moved over the Data Bus by this
 * transfer.  This number is encoded such that 0x0000 means move one byte,
 * 0x0001 means move 2 bytes etc, 0x3fff means move 16384 bytes.
 *
 * Note that if UnPacked data is transferred then only one useful byte is
 * moved for each 32 bits that is transferred over the Data Bus, but because
 * one word (32 bit) is moved over the Data Bus then this is counted as 4
 * bytes when calculating Number of Bytes.
 */



/*
 * RdStp:
 * When this bit is set to '1' the Read Port will not increment its
 * address between each data read from the peripheral.
 */
#define SHUTTLE_R_STOP_ON	 	1
#define SHUTTLE_R_STOP_OFF	 	0


/*
 * WrStp:
 * When this bit is set to '1' the Write Port will not increment its
 * address between each data written to the peripheral.
 */
#define SHUTTLE_W_STOP_ON	 	1
#define SHUTTLE_W_STOP_OFF	 	0


/*
 * RdCtl:
 * This is a multi-purpose bit.  It is passed to the source peripherals as is.
 */
#define SHUTTLE_R_CTRL_ON	 	1
#define SHUTTLE_R_CTRL_OFF	 	0


/*
 * Mode:
 *	'00' = Abort transfer,
 *	'01' = Normal transfer,
 *	'10' = Chained transfer,
 *	'11' = Infinite transfer.
 *
 * Note that if a Abort transfer is written to a channel only the Mode bits
 * are written into the Channel register.  All other bits are left untouched.
 * The Requestor uses these bits to directly control the peripheral.  For
 * example, they can be used as a cache lock bit if the peripheral is a cache.
 */
#define SHUTTLE_MODE_TX_ABORT	 		0	// 00
#define SHUTTLE_MODE_TX_NORMAL	 		1	// 01
#define SHUTTLE_MODE_TX_CHAINED	 		2	// 10
#define SHUTTLE_MODE_TX_INFINITE	 	3	// 11



/*
 * WrLnk:
 * When this bit is set to '1' then the Bus Arbitrator will check to see if
 * the Write Port is vacant before issuing the Read Request to the Read
 * Port for each burst, so that the slow port will not stall the fast ports.
 */
#define SHUTTLE_W_LINK_ON	 	1
#define SHUTTLE_W_LINK_OFF	 	0



/*
 * WrCtl:
 * This is a multi-purpose bit.  It is passed to the destination peripherals as is.
 */
#define SHUTTLE_W_CTRL_ON	 	1
#define SHUTTLE_W_CTRL_OFF	 	0




#pragma pack(1)

//
// The frame got from HCI has two type: one is data and the other is command
//
typedef enum
{
	TX_FRAME_TYPE_DATA = 0x1, 
    TX_FRAME_TYPE_COMMAND
} BT_TX_FRAME_TYPE;


typedef enum
{	
    LINUX_FC7,
    LINUX_FC9,
}OS_VERSION;



//
// The PDU's type
//
typedef enum
{
	RX_FRAME_TYPE_DATA = 0x1, RX_FRAME_TYPE_LMPPDU, RX_FRAME_TYPE_EVENT, RX_FRAME_TYPE_NULL, RX_FRAME_TYPE_POLL, RX_FRAME_TYPE_CRC_ERROR, RX_FRAME_TYPE_DV, RX_FRAME_TYPE_SCO, RX_FRAME_TYPE_SCO_NULL, RX_FRAME_TYPE_AFH_CH_BAD, RX_FRAME_TYPE_DATA_NULL, RX_FRAME_TYPE_LOOPBACK_DATA, RX_FRAME_TYPE_LOOPBACK_SCO, RX_FRAME_TYPE_LOOPBACK_SCO_NULL, RX_FRAME_TYPE_TEST_MODE_DATA
} BT_RX_FRAME_TYPE;



//Jakio20070823: this a simple state machine, used to serialize rx process flow.
//if current state is RX_STATE_CONNECTED, then handle rx data in completion routine,
//else, create a task.
typedef enum
{
	RX_STATE_CONNECTING, RX_STATE_CONNECTED
} BT_RX_STATE;



typedef struct
{
	PUINT8 Buffer;
	UINT8 IsUsed;
	UINT32 frageleno;
} TXPOOL,  *PTXPOOL;






//Jakio20080811: flush int body format
typedef struct _BTUSB_FLUSH_INT_BODY
{
	UINT8			Type;
	UINT8			TxFifo;
	UINT16			Size;
	UINT32			HcbbHeader;
}BTUSB_FLUSH_INT_BODY, *PBTUSB_FLUSH_INT_BODY;




typedef struct
{
	UINT32 Len: 11;
	UINT32 Len1: 11;
	//UINT32 Len2: 9;
	UINT32 Res: 10;
} BT_RX_BUF_HEAD,  *PBT_RX_BUF_HEAD;

//
// Rx Buffer tail
//
// This is for rx buffer tail
//
typedef struct
{
	UINT32 Status: 16;
	UINT32 SeqNo: 8;
	UINT32 Res: 8;
} BT_RX_BUF_TAIL,  *PBT_RX_BUF_TAIL;

typedef struct
{
	UINT16 Reserved : 4;
	UINT16 Gps : 2;
	UINT16 Wifi : 2;
	UINT16 Board : 4;
	UINT16 Chip : 4;
}BT_SUB_SYSTEM_ID, *PBT_SUB_SYSTEM_ID;

#define	ROM_VER_V4B		0x02
#define	ROM_VER_V3		0x01

//Jakio20080311:
#define MAX_BURST_WRITE_LENGTH                  128

typedef struct _VENDOR_CMD_PARAMETERS
{
	//Jakio20080307: add link entry
	BT_LIST_ENTRY_T	Link;
	PUINT8 TransferBuffer; 
	UINT8 AsynTranBuffer[MAX_BURST_WRITE_LENGTH];
	UINT32 TransferBufferLength; //cmd length
	UINT8 RequestType;
	UINT8 ReservedBits;
	UINT8 Request;
	UINT16 Value;
	BOOLEAN bIn;
	BOOLEAN bShortOk; 
	UINT16 Index; 
} VENDOR_CMD_PARAMETERS,  *PVENDOR_CMD_PARAMETERS;

#pragma pack()


//
// Device Extension
//
// This is the per-device component of the device object.
//
#define MAX_USB_BULKIN_FRAME			BT_MAX_FRAME_SIZE	//bulk in 每次接受数据的最大缓冲区大小
#define MAX_USB_INTERRUPT_AREA			0x08				//中断缓冲区暂时定义为8字节，随硬件改变而改变
//Jakio20080225: changed this number from 8 to 32
#define MAX_USB_BUKIN_AREA				1				//Jakio20070802: 接收缓冲区的个数
//jakio20080307: add pre-allocate space for asyn request
#define MAX_ASYN_REQUEST_NUM			8


typedef struct _USB_INT_STACK
{
	UINT32 DataContent;
	UINT16 DataType;
	UINT16 DataAddress;
} USB_INT_STACK,  *PUSB_INT_STACK;




typedef struct _USB_INT_CONTEXT
{
	PUINT8 pInterruptPoolStartAddr;
	PUINT8 pInterruptPool[MAX_INTERRUPT_IRPNUM];
	INT8 IsUsed[MAX_INTERRUPT_IRPNUM];
	UINT32 Length[MAX_INTERRUPT_IRPNUM];
	USB_INT_STACK UsbIntStack[5]; //usbintstack[0] for read dsp reg, usbintstack[1] for read program reg, 
	wait_queue_head_t	IntEvent;
	atomic_t			IntEventFlag;

	PUINT8	pUrb;
	//Jakio20071008: add for cancel int irp
	UINT8	InUsedFlag;
} USB_INT_CONTEXT,  *PUSB_INT_CONTEXT;



typedef struct _USB_BUKLIN_RECV_AREA
{
	UINT8 buffer[BT_MAX_FRAME_SIZE];
	UINT8 InUseFlag;
	UINT32 length;
} USB_BUKLIN_RECV_AREA,  *PUSB_BUKLIN_RECV_AREA;

typedef struct _USB_BULKIN_CONTEXT
{
	PUSB_BUKLIN_RECV_AREA pRecvArea[MAX_USB_BUKIN_AREA];
	UINT8 Index_InUse;

	UINT32 Length;
	PUINT8 pUrb;
} USB_BULKIN_CONTEXT,  *PUSB_BULKIN_CONTEXT;
typedef struct _USB_BULKOUT_CONTEXT
{
	PUINT8 pBulkOutPool;
	PUINT8 pBulkOutScoPool;
	PUINT8 pUrb;
	PUINT8 pScoUrb;
} USB_BULKOUT_CONTEXT,  *PUSB_BULKOUT_CONTEXT;

typedef struct _USB_BULKIN_BDADDR
{
	PUINT8 BulkIn2Buf;
	PUINT8 pUrb;
} USB_BULKIN_BDADDR,  *PUSB_BULKIN_BDADDR;


//Jakio20071011: add here for multi lmp cach
typedef struct _USB_LMP_CACH
{
	UINT8 Flag_InUse;
	UINT8 buffer[128];	//jakio20080301: changed from 256 to 128
}USB_LMP_CACH, *PUSB_LMP_CACH;



//...
typedef struct _BULKUSB_READ_CONTEXT
{
	PURB Urb;
	PUINT8 Buffer;
	UINT32 Length; // remaining to xfer
	PVOID	BulkInStr;
	PVOID	DeviceExtension;
} BULKUSB_READ_CONTEXT,  *PBULKUSB_READ_CONTEXT;

//...
typedef struct _BULKUSB_WRITE_CONTEXT
{
	PURB Urb;
	PUINT8 Buffer;
	UINT32 Length; // remaining to xfer
	UINT32 Numxfer; // cumulate xfer
	UINT8 PipeNum;
	PVOID	DeviceExtension;
	UINT8	FragQueueType;
} BULKUSB_WRITE_CONTEXT,  *PBULKUSB_WRITE_CONTEXT;

//...
typedef struct _BULKUSB_INTERRUPT_CONTEXT
{
	PURB Urb;
	PUINT8 Buffer;
	UINT32 Length; // remaining to xfer
	UINT32 Numxfer; // cumulate xfer
	PVOID	DeviceExtension;
} BULKUSB_INTERRUPT_CONTEXT,  *PBULKUSB_INTERRUPT_CONTEXT;






//Jakio20071204: this struct saves pipe context for int,bulkout,bulkin.
typedef struct _BULKUSB_PIPE_RW_CONTEXT
{
	BULKUSB_READ_CONTEXT		BulkInReadContext0;
	BULKUSB_READ_CONTEXT		BulkInReadContext2;
	BULKUSB_WRITE_CONTEXT		BulkOutWriteContext;
	BULKUSB_WRITE_CONTEXT		BulkOutWriteScoContext;
	BULKUSB_INTERRUPT_CONTEXT	BulkIntContext;
}BULKUSB_PIPE_RW_CONTEXT, *PBULKUSB_PIPE_RW_CONTEXT;



typedef struct _BTUSB_URB_BLOCK
{
	// Link
	BT_LIST_ENTRY_T	Link;	// Pointer to next urb block
	//point to urb block
	PUINT8  ptotaldata;
} BTUSB_URB_BLOCK_T, *PBTUSB_URB_BLOCK_T;  




typedef struct _VENDOR_ASYN_CONTEXT
{
	//Jakio20080307: add link entry
	BT_LIST_ENTRY_T		Link;
	//PURB Urb;
	PBTUSB_URB_BLOCK_T	pUrbBlock;
	PVOID DeviceExtension;
	//PUINT8	pBuffer;
	PUINT8	pVendPara;
	UINT32	cmdSrc;
	struct usb_ctrlrequest	setup;
} VENDOR_ASYN_CONTEXT,  *PVENDOR_ASYN_CONTEXT;




typedef struct _USB_CONTEXT
{
	//record endpoint info
	// TODO: check what endpoint prototype!
	UINT32	EP_control;
	UINT32	EP_bulkout_ACL;
	UINT32	EP_bulkout_SCO;
	UINT32	EP_bulkin_Rx;
	UINT32	EP_bulkin_Inquiry;
	UINT32	EP_interrupt;
	
	USB_BULKIN_CONTEXT UsbBulkInContext;

	USB_INT_CONTEXT UsbIntContext;
	USB_BULKOUT_CONTEXT UsbBulkOutContext;
	USB_BULKIN_BDADDR UsbBulkIn2;


	BULKUSB_PIPE_RW_CONTEXT		BulkRwContext;


	//Jakio20080307: add for asyn request
	BT_LIST_ENTRY_T	AsyUrb_FreeList;
	BT_LIST_ENTRY_T	AsyUrb_UsedList;
	BT_LIST_ENTRY_T	AsyContext_FreeList;
	BT_LIST_ENTRY_T	AsyContext_UsedList;
	BT_LIST_ENTRY_T	AsyVcmdPara_FreeList;
	BT_LIST_ENTRY_T	AsyVcmdPara_UsedList;

	VENDOR_CMD_PARAMETERS	VAsynCmdPara[MAX_ASYN_REQUEST_NUM];
	VENDOR_ASYN_CONTEXT		VAsynCmdCon[MAX_ASYN_REQUEST_NUM];
	BTUSB_URB_BLOCK_T		VAsynUrb[MAX_ASYN_REQUEST_NUM];
} USB_CONTEXT,  *PUSB_CONTEXT;


typedef struct _BULKOUT2_DATA_STRUCT
{
	UINT8 DataType;
	UINT32 DataLength;
	UINT8 IsUsed;
} BULKOUT2_DATA_STRUCT,  *PBULKOUT2_DATA_STRUCT;


typedef struct _VENDOR_CUSTOM_CMD
{
	UINT8 CmdType;
	UINT16 CmdAddress;
	UINT32 CmdData;
} VENDOR_CUSTOM_CMD,  *PVENDOR_CUSTOM_CMD;


/* URB queue used in URB complete function */
struct _urb_queue {
	struct list_head head;
	spinlock_t       lock;
};

enum _urb_type{
    NULL_URB,
    WRITE_URB,
    READ_URB,
    INT_URB,
    INQUIRY_URB,
    CTRL_URB,
};

/* Timer entry in the timer queue pool */
struct _urb_entry {
	struct list_head    list;
	void                *priv;
    enum _urb_type      utype;
};


// Device Extension
//
// This is the per-device component of the device object.
//
typedef struct
{

	struct usb_device *usb_device;
	struct hci_dev *hdev;
    struct usb_interface *intf;

	PVOID	pBusInterface;

	// Data structure elements for Read IRPs
	KSPIN_LOCK ReadQueueLock;
	LIST_ENTRY ReadQueue;

	// Jakio20070904: current num of irp in writequeue
	UINT16 CurNumIrp;

	PVOID    pCmdPendingIrp;


	//Jakio20071226: add this var to statistic bulkin pipe
	UINT32	RxFailureCount;
	UINT32	InquiryFailCount;
	UINT32	VcmdAsynFailureCount;
	//Jakio20080313: fail timer count
	UINT32	RxFailTimerCount;
	BOOLEAN	RxFailTimerValid;
	UINT32	InqFailTimerCount;
	BOOLEAN	InqFailTimerValid;
	

	//Jakio20080227: used to statistic the afh pdu in lmp queue 
	UINT32	AfhPduCount;
	//Jakio20080924: used to statistic the failure times when trying to write afh cmd
	UINT32	AfhWriteCmdFailTimes;

	//
	// Spinlock for threads
	//

	wait_queue_head_t 		BulkOutEvent;
	atomic_t				BulkOutFlag;
	

	//jakio20071116: add for power save
	wait_queue_head_t		RxIdleEvent;
	atomic_t				RxIdleFlag;
	
	wait_queue_head_t		IntIdleEvent;
	atomic_t				IntIdleFlag;
	
	
	wait_queue_head_t		InqIdleEvent;
	atomic_t				InqIdleFlag;

	//Jakio20080407:add for synchronizing combo state, when wlan download ok,we set this event
	wait_queue_head_t		ComboReadyEvent;
	atomic_t				ComboReadyFlag;

	//Jakio20080708: verify download
	wait_queue_head_t		RxFwEvent;
	atomic_t				RxFwFlag;

	//Jakio20080126: add for wait minloop interrupt when remove device
	wait_queue_head_t		MinLoopEvent;
	atomic_t				minLoopFlag;
	//Jakio20081111: wait MainLoop interrupt when start working
	wait_queue_head_t		MainLoopEvent;
	atomic_t				mainLoopFlag;
	//Jakio20081121: add a event to confirm "Join" cmd
	wait_queue_head_t		JoinEvent;
	atomic_t				joinFlag;

    // Device init is finished, even with 2 phases
    struct{
        wait_queue_head_t   DevRdyEvt;
        atomic_t devRdyFlag;
        UINT8   devRdyOk;
        UINT8   phase2;
        // Mutex to sync between open and close
       	struct semaphore openLock;
    };


	UINT8  ManualCancelFlag;
	
	BOOLEAN	PsComboReqFlag;
	//Jakio20080422: add for combo switch debug
	BOOLEAN	AllowWlanJoin;
	
	UINT8  WorkItemRun;
	UINT8  SpaceQueryFlag;

	UINT8 RxFlag;

	UINT16 AclWriteFlag;
	UINT16 SniffWriteFlag;
	UINT16 SlaveWriteFlag;
	UINT16 ScoWriteFlag;
	NTSTATUS FlagStatus;




	//Jakio20070810: add a flag to allow set intsendingtimer, this is a key point to keep flow serialize
	BOOLEAN AllowIntSendingFlag;
	//BOOLEAN AllowSubmitReadIrpFlag;	

	//Jakio20070823: add a state to serialize rx process
	BT_RX_STATE RxState;
	KSPIN_LOCK RxStateLock;
	UINT16 RxPacketNum;


	//Jakio20080103: for combo
	UINT8	FwState;   	//0: bt single mode. 1: combo mode
	UINT8	StartStage;	//init: ;   limited: ;  full function: 
	UINT8	ComboState;   //combo; bt only

	BOOLEAN	IsRxIrpSubmit;	//flag to identify whethe irp is submitted
	UINT8	IntSendingTimerMissed;	//flag to identify whether we missed a chance to inform IVT to read
	BOOLEAN	IsInquiryIrpSubmit;

	BOOLEAN	SubmitBulkoutAcl;
	BOOLEAN	SubmitBulkoutSco;
	BOOLEAN	CompleteBulkoutAcl;
	BOOLEAN	CompleteBulkoutSco;
	UINT32	BulkoutAcl_TimerCount;
	UINT32	BulkoutSco_TimerCount;
	
	//
	// Device State for PnP Purposes
	//
	UINT32 State;
    UINT32 isOpen;

	//Jakio20080526: add for quick remove
	UINT32	DriverHaltFlag;

	//Jakio20080910: add for load EEPROM 
	UINT32	LoadEepromFlag;
	
	//Jakio20071114: temply for test power
	UINT16		Standby;
	UINT16		ScoAmaddr;
	//
	// PNP Flags
	//
	BOOLEAN Removed;
	BOOLEAN Started;
	BOOLEAN HoldNewRequests;


	//
	// Outstanding I/O counters
	//
	UINT32 OutstandingIO;
	UINT32 UsbOutstandingIO;


	//
	// TX produce ptr index and consumer ptr index, Rx cach produce ptr index and consumer ptr index
	//
	UINT32 RxCachPid, RxCachCid;


	//Tx pool start address
	PUINT8 TxStartAddress;

	//Jakio20070907: a set of vars used in tx thread
	UINT8 TxThread_suspend_flag;
	PVOID TxThread_Object;

	//Jakio20080123: buffer used to store EEPROM body
	UINT8	BufferEeprom[1024];
	UINT32	EEPROM_Version;

	//Tx pool
	TXPOOL TxPool[MAX_TX_POOL_COUNT];

	TXPOOL TxScoPool[MAX_TX_POOL_COUNT];

	UINT8	TxAclPendingFlag;
	UINT8	TxScoPendingFlag;

	UINT8	AclPendingCounter;
	UINT8	ScoPendingCounter;
	
	//BULKOUT2_DATA_STRUCT BulkOut2Data;

	UINT16 ScrachFreeSize;

	KSPIN_LOCK TxSpinLock;

	//Jakio20080310: serialize write completion routine

	KSPIN_LOCK AsynCmdLock;

	// Rx-cach start address
	PUINT8 RxCachStartAddr;
	// Rx cach total six
	PUINT8 RxCach[MAX_RX_CACH_COUNT];

	//Jakio20071011: changed here for multi lmp cach
	// Rx-LMPPDU-cach
	//PUINT8 RxLMPPDUCach;
	USB_LMP_CACH RxLMPPDUCach[MAX_LMP_CACH_COUNT];

	//Jakio20071104: marked here, seems it is useless
	// Rx temp buffer
	//PUINT8 RxBuffer;

	// Rx sequence no
	UINT8 RxSeqNo;
	BOOLEAN IsFirstRxFrame;


	// Reset flag
	BOOLEAN ResetFlag;



	// Pointer to store most of blue tooth registers
	PUINT8 pBTRegMems;

	// HCI module pointer
	PVOID pHci;

	// Task module pointer
	PVOID pTask;

	// LMP module pointer
	PVOID pLmp;

	//Flush module pointer, Jakio20080804
	PVOID pFlush;

	//Jakio20080606: test 
	UINT32	SubmitReadIrp;
	//PVOID	pAsynContext;
	//UINT32	QueryDmaTimeout;
	UINT32	QueryFailCount;
	
	USB_CONTEXT UsbContext;


#ifdef BT_TESTDRIVER
	// Test module pointer
	PVOID pTest;

	UINT32 RecvDataCount;
	UINT32 TxDataCount;
	UINT32 RealRecvSuccCount;
	UINT32 RealRecvNotSuccCount;
#endif

	// The following vars is for auto-select-packet-type. That is packet type of ACL data could be down or
	// up according to the performance of channel. For example, if the performance is low, the packet type
	// could be down from DH5 to DM5. Another example, if the performance is high, the packet type could be
	// up from DM1 to DM3.
	volatile UINT32 TxRetryCount;
	volatile UINT32 TxAclSendCount;
	volatile UINT32 StartRetryCount;
	volatile UINT32 EndRetryCount;

	UINT8 MyVersion[MAXVERSIONLENTH];


	UINT32               chipID;

	BOOLEAN             AdapterUnplugged;


	// Frag module pointer
	PVOID pFrag;

#ifdef BT_AFH_ADJUST_MAP_SUPPORT
	PVOID pAfh;
#endif

#ifdef BT_SCHEDULER_SUPPORT
	PVOID pSched;
#endif

#ifdef BT_LOOPBACK_SUPPORT
	PVOID pLoopback;
#endif
#ifdef BT_TESTMODE_SUPPORT
	PVOID pTestMode;
	BOOLEAN	AllowCmdIndicator;
#endif

	UINT8 Version8051[4];
	UINT8 VersionDSP[4];

	//jakio20080617: identify what kind of OS
	UINT32	OsVersion;
	//Jakio20080623: patch for vista's standby bug
	UINT32	FakeD3State;

	//Jakio20080703: set in BtPower_SetPowerD3
	UINT32	InD3Flag;

	BOOLEAN RssiWhenSniff;
	
	UINT16	AntennaNum;
	
	//timers
	//watchdog timer
	struct timer_list  watchDogTimer;
	struct workqueue_struct *pBtWorkqueue;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)
	struct delayed_work BtWorkitem;
#else	
	struct work_struct BtWorkitem;
#endif

	// Timer used for sending frame
	struct tasklet_struct taskletRcv;

    /* Command transmit queue, to cache the command from tasklet to thread */
    struct sk_buff_head cmd_queue;
    struct task_struct *cmd_thread;    /* Command processing thread */
    wait_queue_head_t cmd_wait;  /* Command wait queue head */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)    
	struct delayed_work cmd_work;
#else    
	struct work_struct cmd_work;
#endif

    int cmd_wait_flag;
    int exit_thread;


    // Driver diagnose work
    struct work_struct diagWork;
    wait_queue_head_t diagEvent;
    int diagFlag;
    

    /* USB USB complete Tasklet */  
  	struct tasklet_struct usb_complete_task;

    struct _urb_queue urb_pool;
    struct _urb_queue complete_q;  

    // Bridge buffer used for Rx HCI data
    PUINT8 pBufBridge;
    
    // Debug related
    struct{
        // Lmp Dump Debug queue
        struct sk_buff_head lmpDumpQ;
        /* Lmp Dump event */
        wait_queue_head_t  lmpDumpEvt;
        UINT32  lmpEvtFlag;
        BOOLEAN on;
    }debug;

} BT_DEVICE_EXT,  *PBT_DEVICE_EXT;





#endif /* __OSR_PCI_H__ */
