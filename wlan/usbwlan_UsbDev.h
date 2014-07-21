#ifndef DSP_USBDEV_H
   #define DSP_USBDEV_H

/***********************************************************************
 * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
 *
 * FILENAME:	DSP_UsbDev.h	  CURRENT VERSION: 1.00.01
 * PURPOSE:	  This file defines some sturcture and declares some function 
 *			   about usb device module which contains some operator for 
 *			   usb device and usb interface and usb pipe and so on.
 *
 * 
 *
 * DECLARATION:  This document contains confidential proprietary information that
 *			   is solely for authorized personnel. It is not to be disclosed to
 *			   any unauthorized person without prior written consent of 3DSP
 *			   Corporation.	   
 *
 **********************************************************************/

#include "precomp.h"
/*--macros------------------------------------------------------------*/
// indicate index of configuration descriptor , uausally fixed as 1
#define USB_CONFIGURATION_DES_INDEX	(1)

//interface index for wlan function
#define USB_WLAN_INTERFACE_DES_INDEX	(0)
#define USB_BT_INTERFACE_DES_INDEX	(1)

//alternate setting
#define USB_ALTERNATE_SETTING   (0)


#define USB_TOTAL_PIPE_NUM_IN_USE	  (4)
#define USB_TOTAL_PIPE_NUM			 (5)

//Jakio20080311: 
#define MAX_BURST_WRITE_LENGTH			128
#define MAX_FRAME_IN_SIZE				  1280

#define MAX_BULK_TRANSFER_SIZE						65535
#define MAX_PIPE_BUFFER_SIZE		   1280

#define MAX_URB_REQUEST_NUM	  8 
#define MAX_TX_URB_NUM			1
#define MAX_RX_URB_NUM			1
#define MAX_INT_URB_NUM			1
//jakio20080307: add pre-allocate space for asyn request
#define MAX_CNTRL_URB_NUM			1

#define MAX_URB_ENTRY_NUM		  4
#define MAILBOX_DATA_TYPE_FIRMWARE						0x00
#define MAILBOX_DATA_TYPE_SENDHEAD						0x01 //
#define MAILBOX_DATA_TYPE_SENDBODY						0x02 //

#define USB_EP_CTRL			(0)
#define USB_EP_BULKIN		  (1)   //rx pipe
#define USB_EP_BULKOUT_BODY	(2)   //send pipe
#define USB_EP_BULKIN_2		(3)   //rx pipe
#define USB_EP_BULKOUT_HEAD	(4)  //send pipe
#define USB_EP_BULKINT		 (9)   //int pipe

#define USB_PIPE_RETRY_LIMIT	1

/*--constants and types------------------------------------------------*/


// fdeng mask this !
//
//#pragma pack(1)

// status codes for UsbDev_ActivateConfiguration

typedef void* PURB;

typedef  void(* USB_COMPLETE_FUNC)(PDSP_ADAPTER_T,INT32, PVOID);

typedef enum _USB_URB_TYPE{
	NULL_URB,
	ASYN_CONTROL_URB,
	WRITE_BODY_URB,
	WRITE_HEAD_URB,
	READ_URB,
	INT_URB,
	DUMMY_URB,
}USB_URB_TYPE;

//
// This structure contains the information about usb pipe module 
// 
//
typedef enum _USB_PIPE_TYPE
{
	USB_PIPE_TYPE_CONTOL = 0,  
	USB_PIPE_TYPE_SEND,
	USB_PIPE_TYPE_RECV,
	USB_PIPE_TYPE_INT,
	USB_PIPE_TYPE_DUMMY,	  
}USB_PIPE_TYPE;

typedef struct _USB_CONTEXT USB_CONTEXT, *PUSB_CONTEXT;

typedef struct _USB_URB_CONTEXT
{
	DSP_LIST_ENTRY_T	  link;		  //pointer to the next urb_context
	UINT8				 link_no;	  //each urb has an uinique link_no
	PURB				  urb;		   //pointer to urb  
	USB_URB_TYPE		  urb_type;	  //pipe type of urb
	PUSB_CONTEXT		  usb_context;
	USB_COMPLETE_FUNC	 complete_func; //callback func of this urb
	PVOID				 func_context;  //context of the callback func
}USB_URB_CONTEXT,*PUSB_URB_CONTEXT;


typedef struct _USB_CNTRL_URB_CONTEXT
{
	DSP_LIST_ENTRY_T	  link;		  //pointer to the next urb_context
	UINT8				 link_no;	  //each urb has an uinique link_no
	PURB				  urb;		   //pointer to urb  
	USB_URB_TYPE		  urb_type;	  //pipe type of urb
	PUSB_CONTEXT		  usb_context;
	USB_COMPLETE_FUNC	 complete_func; //callback func of this urb
	PVOID				 func_context;  //context of the callback func
	PVOID	             set_up;
   
}USB_CNTRL_URB_CONTEXT,*PUSB_CNTRL_URB_CONTEXT;

typedef struct _USB_PIPE {
	PURB	urb;				 //urb		   
	DSP_LIST_ENTRY_T   urb_list;		//urb not allocated
	DSP_LIST_ENTRY_T   urb_submit_list; //urb has been submit list
	USB_PIPE_TYPE	  pipe_type;
	BOOLEAN  in_busy;			  //flag in use
	UINT8	ep_num;
	//UINT32   pipe;				 // pipe number
	PVOID	urb_buffer;		  //urb buffer
	UINT8	urb_num;					//urb number the pipe has		
	TDSP_SPINLOCK pipe_lock;
	TDSP_EVENT    pipe_idle_event;	
}USB_PIPE, *PUSB_PIPE; 

#if 0
typedef struct _USB_INFO
{
	UINT8 EP_control;		//ep info
	UINT8 EP_bulkout_HEAD;
	UINT8 EP_bulkout_BODY;	  //pipe pointer
	UINT8 EP_bulkin;
	UINT8 EP_interrupt;
	//struct usb_device	*usb_dev;	 /* this usb_device */
	//struct usb_interface	*usb_intf;	 /* this interface */
	
}USB_INFO,*PUSB_INFO;	
#endif

typedef struct _USB_END_POINT
{
	UINT8 ep_num;
	USB_PIPE_TYPE   pipe_type;
	PUSB_PIPE pipe;
}USB_END_POINT,*PUSB_END_POINT;


/* URB queue used in URB complete function */
typedef struct _USB_URB_QUEUE{
	//struct list_head head;
	//spinlock_t	   lock;
	DSP_LIST_ENTRY_T head;
    TDSP_SPINLOCK    lock;
	
}USB_URB_QUEUE,*PUSB_URB_QUEUE;


/* Timer entry in the timer queue pool */
typedef struct _USB_URB_ENTRY{
	//struct list_head	list;
	DSP_LIST_ENTRY_T    link;
	//void				*priv;
	USB_URB_TYPE	  utype;
    PUSB_URB_CONTEXT  urb_context;
    INT32             status;
    INT32             actual_len;
}USB_URB_ENTRY,*PUSB_URB_ENTRY;

struct _USB_CONTEXT
{

	PVOID tdsp_adap;		 //pointer to adapter
	void	*usb_dev;	 /* this usb_device */
	void	*usb_intf;	 /* this interface */

	PUSB_PIPE usb_send_body_pipe;

	// usb bulk in pipe
	PUSB_PIPE usb_receive_pipe;

	// usb interrupt pipe
	PUSB_PIPE usb_interrupt_pipe;

	//usb asy control pipe
	PUSB_PIPE usb_asyn_control_pipe;

	// for int pipe idle
	//TDSP_EVENT int_idle_event;

	//for usb_send_pipe
	//TDSP_EVENT bulkout_event;
	
	//rx idle 
	//TDSP_EVENT rx_idle_event;
	
	 /* USB USB complete Tasklet */  
	TDSP_TASKLET  usb_complete_task;
	USB_URB_ENTRY urb_entry_buf[MAX_URB_ENTRY_NUM];
	USB_URB_QUEUE urb_pool;
	USB_URB_QUEUE complete_q;   

};


// fdeng mask this !
//
//#pragma pack()

/*--variables---------------------------------------------------------*/
	
/*--function prototypes-----------------------------------------------*/
/* usb device function */
TDSP_STATUS UsbDev_Init(PDSP_ADAPTER_T pAdap);
VOID UsbDev_Release(PDSP_ADAPTER_T pAdap);
void Usb_Completion_Imp(void * pUrb);
INT32 UsbDev_BuildVendorRequest(
								   PUSB_CONTEXT usb_context,
								   UINT8  In,
								   UINT8* TransferBuffer,
								   UINT32 TransferBufferLength,
								   UINT8 Request,
								   UINT16 Value,
								   UINT16 Index
								   );

TDSP_STATUS UsbDev_BuildVendorRequestAsyn(
								   PUSB_CONTEXT usb_context,
								   UINT8* TransferBuffer,
								   UINT32 Length,
								   UINT8 Request,
								   UINT16 Value,
								   UINT8  In,
								   UINT16 Index,
								   USB_COMPLETE_FUNC func,
								   PVOID context);


TDSP_STATUS UsbDev_BuildBulkInTransfer(PUSB_CONTEXT usb_context,
												 PVOID buffer,
												 UINT32 len,
												 USB_COMPLETE_FUNC func,
												 PVOID context);


TDSP_STATUS UsbDev_BuildBulkOutTransfer( PUSB_CONTEXT usb_context,
													PVOID  buffer,
													UINT32 len,
													USB_COMPLETE_FUNC func,
													PVOID context);
													 
TDSP_STATUS UsbDev_BuildInterruptTransfer(PUSB_CONTEXT usb_context,						  
													 PVOID buffer,
													 UINT32 len,
													 USB_COMPLETE_FUNC func,
													 PVOID context);
BOOLEAN UsbDev_CancelBulkInTransfer(PUSB_CONTEXT usb_context);

BOOLEAN UsbDev_CancelBulkOutTransfer(PUSB_CONTEXT usb_context);

BOOLEAN UsbDev_CancelInterruptTransfer(PUSB_CONTEXT usb_context);
BOOLEAN UsbDev_CancelBulkInTransfer(PUSB_CONTEXT usb_context);
BOOLEAN UsbDev_CancelAsyRequest(PUSB_CONTEXT usb_context);
BOOLEAN UsbDev_WaitBulkOutIdle(PUSB_CONTEXT usb_context,UINT32 timeout);
BOOLEAN UsbDev_WaitAsyRequestIdle(PUSB_CONTEXT usb_context,UINT32 timeout);
#endif

