/****************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  *
  * FILENAME:	 DSP_UsbDev.c	 CURRENT VERSION: 1.00.01
  * PURPOSE:	  This file realizes the function about usb device 
  *			   module which contains some operator for usb device
  *			   and usb interface and usb pipe and so on.
  *
  *
  * DECLARATION:  This document contains confidential proprietary information that
  *			   is solely for authorized personnel. It is not to be disclosed to
  *			   any unauthorized person without prior written consent of 3DSP
  *			   Corporation.		  
  ******************************************************************/
//#include <linux/errno.h>
//#include <linux/slab.h>
//#include <linux/mm.h>

static char* TDSP_FILE_INDICATOR="USBDV";
#include "usbwlan_UsbDev.h"
/*
#include <ndis.h>
#include "usbwlan_Sw.h"
#include "usbwlan_dbg.h"
#include "usbwlan_tx.h"
*/  
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
/*--file local variables-----------------------------------------------*/
/*--file local function prototypes-------------------------------------*/ 
/* usb device function */

/* queue operations */

void _urb_queue_init(PUSB_URB_QUEUE q)
{
	QueueInitList(&q->head);
	sc_spin_lock_init(&q->lock);
}

void _urb_queue_head(PUSB_URB_ENTRY ue, PUSB_URB_QUEUE q)
{

	unsigned long flags;
	sc_spin_lock_irqsave(&q->lock, flags);
	QueuePushHead(&q->head, &(ue->link));
	sc_spin_unlock_irqrestore(&q->lock, flags);

}

void _urb_queue_tail(PUSB_URB_ENTRY ue, PUSB_URB_QUEUE q)
{

	unsigned long flags;
	sc_spin_lock_irqsave(&q->lock, flags);
	QueuePutTail(&q->head, &(ue->link));
	sc_spin_unlock_irqrestore(&q->lock, flags);

}


PUSB_URB_ENTRY _urb_dequeue(PUSB_URB_QUEUE q)
{

	PUSB_URB_ENTRY ue = NULL;

	unsigned long flags;
	sc_spin_lock_irqsave(&q->lock, flags);
	ue = (PUSB_URB_ENTRY)QueuePopHead(&q->head);
    sc_spin_unlock_irqrestore(&q->lock, flags);

    return ue;
}

/* Acquire one queue entry */
PUSB_URB_ENTRY _get_free_urb_entry( PUSB_URB_QUEUE q)
{
	PUSB_URB_ENTRY ue = NULL;
	ue = _urb_dequeue(q); 
	if(!ue)
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] cant't alloc new urb resource\n",__FUNCTION__);
	}
	return ue;
}

void _UsbDev_Pipe_Destory(PUSB_PIPE  pipe)
{
	UINT32 i;
	PURB pUrb = NULL;
    PVOID ctlr_req;
	PUSB_URB_CONTEXT   urb_context;
	PUSB_CNTRL_URB_CONTEXT cntrl_urb_context;
	//ASSERT(pipe);

    DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s],pipe type is %d \n",__FUNCTION__,pipe->pipe_type);
	//empty free urb list
	sc_spin_lock(&pipe->pipe_lock);
	QueueInitList(&pipe->urb_list);
	sc_spin_unlock(&pipe->pipe_lock);

	//wait pipe idle event
	if(pipe->in_busy)
	{
		if(0 == sc_event_wait(&pipe->pipe_idle_event,20))
		{
		    DBG_WLAN__USB(LEVEL_TRACE,"[%s]: urb not all returned,pipe type is %d\n",
				__FUNCTION__,
				pipe->pipe_type);
			//wait all submit urb returned
			do
			{   
				sc_spin_lock(&pipe->pipe_lock);
				urb_context = (PUSB_URB_CONTEXT)QueuePopHead(&pipe->urb_submit_list);
				sc_spin_unlock(&pipe->pipe_lock);
				if(NULL == urb_context)
					break;
				pUrb = urb_context->urb;
				sc_urb_kill(pUrb);
			}while(1);
		}
	}
	//release all urb memorys
	if(USB_PIPE_TYPE_CONTOL == pipe->pipe_type)
	{
		cntrl_urb_context = (PUSB_CNTRL_URB_CONTEXT)pipe->urb_buffer;
		for(i = 0; i < pipe->urb_num; i++)
		{
			pUrb = cntrl_urb_context[i].urb;
            ctlr_req = cntrl_urb_context[i].set_up;
			if(NULL != pUrb)
			{
				sc_urb_free(pUrb);
				pUrb = NULL;
			}
            if(NULL != ctlr_req)
            {
                sc_usb_free_ctrlreq(ctlr_req);
                ctlr_req = NULL;
            }
		}	   
	}
    else
	{
		urb_context = (PUSB_URB_CONTEXT)pipe->urb_buffer;
		for(i = 0; i < pipe->urb_num; i++)
        {
    	    pUrb = urb_context[i].urb;
            if(NULL != pUrb)
            {
            	sc_urb_free(pUrb);
            	pUrb = NULL;
            }	 
    	}
	}

    sc_spin_lock_kill(&pipe->pipe_lock);
	sc_event_kill(&pipe->pipe_idle_event);
	sc_memory_free(pipe->urb_buffer);
	sc_memory_free(pipe);

    
    DBG_WLAN__USB(LEVEL_TRACE,"Leave [%s]:pipe type is %d \n",__FUNCTION__,pipe->pipe_type);

}

PUSB_PIPE _UsbDev_Pipe_Create(PUSB_CONTEXT usb_context,
										USB_PIPE_TYPE type,
										UINT8 end_point,
										UINT8 urb_num)
{

    UINT32 i;
	UINT32 alloc_size;
	PUSB_PIPE pipe = NULL;
	PVOID alloc_buff;
	PUSB_URB_CONTEXT urb_context = NULL;
	PUSB_CNTRL_URB_CONTEXT cntrl_urb_context = NULL;
    PURB pUrb = NULL;
	
	if((NULL == usb_context)
		|| (type >= USB_PIPE_TYPE_DUMMY)
		|| ( 0 == urb_num)
		|| (urb_num > MAX_URB_REQUEST_NUM))
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] entry para error ,type is %d,urb num is %d\n",
										__FUNCTION__,
										type,
										urb_num);
		return NULL;
	}
	if(NULL == (pipe = sc_memory_alloc(sizeof(USB_PIPE))))
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] allocate memory for pipe failure \n",__FUNCTION__);
		return NULL;
	}

	sc_memory_set(pipe,0,sizeof(USB_PIPE));

    #if 0
	//get pipe number  
	switch(type)
	{
	 case USB_PIPE_TYPE_CONTOL:
			 pipe->pipe =  sc_usb_sndctrlpipe(usb_context->usb_dev,end_point);
			break;
	 case USB_PIPE_TYPE_SEND:
			 pipe->pipe =  sc_usb_sndbulkpipe(usb_context->usb_dev,end_point);
			break;
	 case  USB_PIPE_TYPE_RECV:
			 pipe->pipe =  sc_usb_rcvbulkpipe(usb_context->usb_dev, end_point);
			break;
	 case USB_PIPE_TYPE_INT:
			 pipe->pipe =  sc_usb_rcvintpipe(usb_context->usb_dev, end_point);
			break;
	 default:
		   kd_assert(FALSE);
		  break;
		
	}
    #endif
    
	if(USB_PIPE_TYPE_CONTOL != type)
	{
		alloc_size = urb_num * sizeof(USB_URB_CONTEXT);	   
	}
	else
	{
		alloc_size = urb_num * sizeof(USB_CNTRL_URB_CONTEXT);
	}

	alloc_buff = sc_memory_alloc(alloc_size);	 
	if(NULL == alloc_buff)
	{
		sc_memory_free(pipe);	   
		DBG_WLAN__USB(LEVEL_ERR, "[%s] allocate memory for pipe failure \n",__FUNCTION__);
		return NULL;
	}

	sc_memory_set(alloc_buff,0, alloc_size);
	pipe->urb_buffer = alloc_buff;
	QueueInitList(&pipe->urb_list); 
	//allc urb for every context and link all context

	if(USB_PIPE_TYPE_CONTOL != type)
	{
		urb_context = (PUSB_URB_CONTEXT)alloc_buff;
 
        for(i = 0; i < urb_num; i++)
		{
			//allocate urb
			pUrb = sc_urb_alloc();
			if(NULL == pUrb )
			{
				DBG_WLAN__USB(LEVEL_ERR, "[%s] allocate urb failed",__FUNCTION__);
				break;
			}
    		urb_context[i].urb= (PURB)pUrb;
			urb_context[i].link_no = i;
			//link all the nodes
			QueuePutTail(&pipe->urb_list, &urb_context[i].link);
 
        }
   
	}

	else	    
	{
		cntrl_urb_context = (PUSB_CNTRL_URB_CONTEXT)alloc_buff;
   
		for(i = 0; i < urb_num; i++)
		{

			//allocate urb
			pUrb = sc_urb_alloc();
			if(NULL == pUrb )
			{
				DBG_WLAN__USB(LEVEL_ERR, "[%s] allocate urb failed",__FUNCTION__);
				break;
			}
			cntrl_urb_context[i].urb = (PURB)pUrb;
			cntrl_urb_context[i].link_no = i;
            cntrl_urb_context[i].set_up  = sc_usb_alloc_ctrlreq();
            if(NULL == cntrl_urb_context[i].set_up)
            {
                break;
            }
			//link all the nodes
			QueuePutTail(&pipe->urb_list, &cntrl_urb_context[i].link);
		}
 
	}
    
	if (i < urb_num)
	{  
		DBG_WLAN__USB(LEVEL_ERR, "[%s] can not alloc enougth type %d pipe failed!\n",__FUNCTION__,type);
		_UsbDev_Pipe_Destory(pipe);
		return NULL;
	}

	pipe->pipe_type = type;
	pipe->ep_num	= end_point;
	pipe->in_busy   = FALSE;
	pipe->urb_num = urb_num;
	QueueInitList(&pipe->urb_submit_list);
	//inti spin lock
	sc_spin_lock_init(&pipe->pipe_lock);
	sc_event_init(&pipe->pipe_idle_event);
	return pipe;

}

void _UsbDev_Destory_Usb(PUSB_CONTEXT usb_context)
{
	//ASSERT(usb_context);
	//release other pipes
	if(NULL != usb_context->usb_asyn_control_pipe)
	{
		_UsbDev_Pipe_Destory(usb_context->usb_asyn_control_pipe);
		usb_context->usb_asyn_control_pipe = NULL;
	}

	if(NULL != usb_context->usb_interrupt_pipe)
	{
		_UsbDev_Pipe_Destory(usb_context->usb_interrupt_pipe);
		usb_context->usb_interrupt_pipe = NULL;
	}

	if(NULL != usb_context->usb_receive_pipe)
	{
	   _UsbDev_Pipe_Destory(usb_context->usb_receive_pipe);
		usb_context->usb_receive_pipe = NULL;
	}

	if(NULL != usb_context->usb_send_body_pipe)
	{
		_UsbDev_Pipe_Destory(usb_context->usb_send_body_pipe);
		usb_context->usb_send_body_pipe = NULL;
	}
	
	sc_tasklet_kill(&usb_context->usb_complete_task);
	_urb_queue_init(&usb_context->complete_q);
	_urb_queue_init(&usb_context->urb_pool);
	
}

TDSP_STATUS _UsbDev_Create_Usb(PUSB_CONTEXT usb_context,PUSB_INFO usb_info)
{
	DBG_WLAN__USB(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__);  
    
	do
	{
		//allocate resources for bulk in pipe
		usb_context->usb_send_body_pipe = _UsbDev_Pipe_Create( usb_context,
    														   USB_PIPE_TYPE_SEND,
    														   usb_info->EP_bulkout_BODY,
    														   MAX_TX_URB_NUM);
		if(NULL == usb_context->usb_send_body_pipe)
		{
				break;
		}
		
		//create usb revceive ep
		usb_context->usb_receive_pipe = _UsbDev_Pipe_Create( usb_context,
                                    						 USB_PIPE_TYPE_RECV,
                                    						 usb_info->EP_bulkin,
                                    						 MAX_RX_URB_NUM);
		if(NULL == usb_context->usb_receive_pipe)
		{
				break;
		}

        //create usb_inerrup_ep
        usb_context->usb_interrupt_pipe = _UsbDev_Pipe_Create( usb_context,
        				                                       USB_PIPE_TYPE_INT,
        							                           usb_info->EP_interrupt,
        								                       MAX_INT_URB_NUM);		
		if(NULL == usb_context->usb_interrupt_pipe)
		{
				break;
		}

		//create usb asyn control ep
		usb_context->usb_asyn_control_pipe = _UsbDev_Pipe_Create( usb_context,
											                      USB_PIPE_TYPE_CONTOL,
											                      usb_info->EP_control,
											                      MAX_CNTRL_URB_NUM);
		if(NULL == usb_context->usb_asyn_control_pipe)
		{
				break;
		}
											   
		DBG_WLAN__USB(LEVEL_TRACE, "Leave [%s] Success\n",__FUNCTION__);  
		return STATUS_SUCCESS;
	}while(0);	

	_UsbDev_Destory_Usb(usb_context);
		
	DBG_WLAN__USB(LEVEL_TRACE, "Leave [%s] Failure\n",__FUNCTION__);	  
	return STATUS_FAILURE;
}




PVOID _UsbDev_Alloc_Urb(PUSB_PIPE pipe)
{
    ULONG flags;
	PVOID urb_context = NULL;
	ASSERT(pipe);
	sc_spin_lock_irqsave(&pipe->pipe_lock,flags);
	urb_context = QueuePopHead(&pipe->urb_list);
	sc_spin_unlock_irqrestore(&pipe->pipe_lock,flags);
	return urb_context;
	
}

void  _UsbDev_Release_Urb(PUSB_PIPE pipe, PVOID urb_context)
{

	PUSB_URB_CONTEXT release_urb;
	//ASSERT(pipe);
	release_urb = (PUSB_URB_CONTEXT)urb_context;
	sc_spin_lock(&pipe->pipe_lock);
	QueuePushHead(&pipe->urb_list,&release_urb->link);
	sc_spin_unlock(&pipe->pipe_lock);	
}
PUSB_PIPE _UsbDev_Get_Pipe(PUSB_CONTEXT usb_context, USB_URB_TYPE urb_type)
{
	 PUSB_PIPE pipe = NULL;
	 if(usb_context)
	 {
		  switch(urb_type)
		  {
			case ASYN_CONTROL_URB:
				pipe = usb_context->usb_asyn_control_pipe;
				break;
			case READ_URB:
				pipe = usb_context->usb_receive_pipe;
				break;
			case WRITE_BODY_URB:
				 pipe = usb_context->usb_send_body_pipe;
				break;
			case INT_URB:
				pipe = usb_context->usb_interrupt_pipe;
				break;
			default:
				break;
		}
	 }
	  return pipe;
}

void Usb_Completion_Imp(void* pUrb)
{
	PUSB_CONTEXT usb_context;
	PUSB_URB_CONTEXT urb_context;
	PUSB_URB_ENTRY  ue_free;
	PDSP_ADAPTER_T pAdap;
	PUSB_PIPE pipe;
	INT32   actual_len;
	INT32	status;
 	ULONG flags;
	USB_COMPLETE_FUNC	 complete_func; //callback func of this urb
	PVOID				 func_context;  //context of the callback func

    urb_context = (PUSB_URB_CONTEXT)sc_urb_getcntxt(pUrb);
    usb_context = urb_context->usb_context;
#if 0
    if(ASYN_CONTROL_URB == urb_context->urb_type)
    {
    	DBG_WLAN__USB(LEVEL_TRACE, "head urb returned!\n");
    }
    if(WRITE_BODY_URB == urb_context->urb_type)
    {
    	DBG_WLAN__USB(LEVEL_TRACE, "body urb returned!\n");
    }
 #endif   
    if(INT_URB == urb_context->urb_type)
    {
        pAdap = usb_context->tdsp_adap;
        pipe = _UsbDev_Get_Pipe(usb_context, INT_URB);
		
        if(pipe==NULL)
			return;
		
		complete_func = urb_context->complete_func;
		func_context  = urb_context->func_context;

		status = sc_urb_getstatus(pUrb);
		if(0 != status)
		{
			DBG_WLAN__USB(LEVEL_ERR, "[%s] failed with urb status:%d ,urb type is %d\n",
							__FUNCTION__,
							status,
							urb_context->urb_type);
			actual_len = status;
			if(sc_usb_issrmv(status))
			{
				DBG_WLAN__USB(LEVEL_ERR, "[%s] ---Device is suprised removed----with urb status:%d ,urb type is %d\n",
							__FUNCTION__,status,urb_context->urb_type);
				Adap_Set_Driver_State(pAdap,DSP_SUPRISE_REMOVE);
            }
		}
		else
		{
			actual_len = sc_urb_getlen(pUrb); 
		}
		
		//remove the submitted urb from urb_submit_list and return to the urb list
		sc_spin_lock_irqsave(&pipe->pipe_lock,flags);
		QueueRemoveEle(&pipe->urb_submit_list, &urb_context->link);
		QueuePutTail(&pipe->urb_list, &urb_context->link);
		sc_spin_unlock_irqrestore(&pipe->pipe_lock,flags);
        
		//set pipe busy flag to false
		if(QueueEmpty(&pipe->urb_submit_list))
		{
			pipe->in_busy = FALSE;
           //set pipe idle event 
			sc_event_set(&pipe->pipe_idle_event);
		}
        
        //call the callbacke func and set idle event
		if(NULL != complete_func)
		{
		    /* Do real callback function */
            complete_func(pAdap, actual_len, func_context);
		}

			
	    return;
    }
    
    /* Queue the URB */
	ue_free = _get_free_urb_entry(&usb_context->urb_pool);
	if(NULL == ue_free)
	{
		 DBG_WLAN__USB(LEVEL_ERR,"[%s] cant't get urb entry\n",__FUNCTION__);
		 return;
	}
	//ue_free->priv = pUrb;
	ue_free->utype = urb_context->urb_type;
    ue_free->status  = sc_urb_getstatus(pUrb);
    ue_free->actual_len = sc_urb_getlen(pUrb); 
    ue_free->urb_context = urb_context;

	 _urb_queue_tail(ue_free, &usb_context->complete_q);
	
	/* Delay the job to tasklet */		
	DBG_WLAN__USB(LEVEL_INFO,"[%s] $$Schedule the Tasklet$$\n",__FUNCTION__);
	sc_tasklet_schedule(&usb_context->usb_complete_task);
}

///////////////////////////////////////////////////////////////////////////
//																	  ///
///////////////////////////////////////////////////////////////////////////

									
void _usb_complete_task(ULONG arg)
{
    PUSB_CONTEXT usb_context;
	PDSP_ADAPTER_T pAdap;
	PUSB_URB_CONTEXT  urb_context = NULL;
	//INT32	status;
	//PURB   urb;
	PUSB_PIPE pipe;
	INT32   actual_len;
	PUSB_URB_ENTRY ue_inq = NULL;
	USB_COMPLETE_FUNC	 complete_func; //callback func of this urb
	PVOID				 func_context;  //context of the callback func
	USB_URB_TYPE		  urb_type;
	//UINT32 flags;
	usb_context = (PUSB_CONTEXT)arg;  

	while((ue_inq = _urb_dequeue(&usb_context->complete_q)))
	{   
		//urb = (PURB)ue_inq->priv;
		//if(urb == NULL)
		//	return;
		
		urb_context = ue_inq->urb_context;
		
		if(NULL == urb_context)
        {
            DBG_WLAN__USB(LEVEL_ERR,"[%s] cant't get urb context\n",__FUNCTION__);
			return;
        }

        DBG_WLAN__USB(LEVEL_INFO, "[%s] Process completed URB,URB type is %d\n",
			   __FUNCTION__,
			   urb_context->urb_type);
		

		pAdap = usb_context->tdsp_adap;
		pipe = _UsbDev_Get_Pipe(usb_context, urb_context->urb_type);
		
		if(NULL == pipe)
        {
             DBG_WLAN__USB(LEVEL_ERR,"[%s] cant't get urb's pipe,urb_type is %d\n",__FUNCTION__,urb_context->urb_type);
        }      
		
		complete_func = urb_context->complete_func;
		func_context  = urb_context->func_context;
		urb_type	  = urb_context->urb_type;
        
		if(0 != ue_inq->status )
		{
			DBG_WLAN__USB(LEVEL_ERR, "[%s] failed with urb status:%d ,urb type is %d\n",
							__FUNCTION__,
							ue_inq->status,
							urb_type);
			actual_len = ue_inq->status;
			if(sc_usb_issrmv(ue_inq->status))
			{
				DBG_WLAN__USB(LEVEL_ERR, "[%s] ---Device is suprised removed----with urb status:%d ,urb type is %d\n",
							__FUNCTION__,ue_inq->status,urb_type);
				Adap_Set_Driver_State(pAdap,DSP_SUPRISE_REMOVE);
			}
		}
		else
		{
			actual_len = ue_inq->actual_len; 
		}
		
		//remove the submitted urb from urb_submit_list and return to the urb list
		sc_spin_lock(&pipe->pipe_lock);
		QueueRemoveEle(&pipe->urb_submit_list, &urb_context->link);
		QueuePutTail(&pipe->urb_list, &urb_context->link);
		sc_spin_unlock(&pipe->pipe_lock);

         //set pipe busy flag to false
		if(QueueEmpty(&pipe->urb_submit_list))
		{
			pipe->in_busy = FALSE;
            sc_event_set(&pipe->pipe_idle_event);
		}
	
		//call the callbacke func and set idle event
		if(NULL != complete_func)
		{
			/* Do real callback function */
			complete_func(pAdap, actual_len, func_context);
		}

	
	   /* return the urb entry to pool */
		//ue_inq->priv = NULL;
		ue_inq->utype = NULL_URB;
        ue_inq->actual_len = 0;
        ue_inq->status =0;
        ue_inq->urb_context = NULL;
		_urb_queue_tail(ue_inq, &usb_context->urb_pool);        
  
	}

}


VOID UsbDev_Release(PDSP_ADAPTER_T pAdap)
{

	DBG_WLAN__USB(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
	if(NULL == pAdap->usb_context)
		return;
	_UsbDev_Destory_Usb((PUSB_CONTEXT)pAdap->usb_context);
	sc_memory_free(pAdap->usb_context);
	pAdap->usb_context = NULL;
    DBG_WLAN__USB(LEVEL_TRACE, "Leanve [%s] \n",__FUNCTION__); 
}



/**************************************************************************
 *   UsbDev_Init
 *
 *   Descriptions:
 *	  Initialize usb device module, including alloc memory for the usb 
 *	  device module and link interface array and pipe array into the int free
 *	  array and pipe free array and so on.
 *   Arguments:
 *	  MiniportAdapterContext: IN, pointer to the adapter object data area.
 *	  ppusb_contex: OUT, pointer to the pointer of usb device module
 *   Return Value:
 *	  STATUS_SUCCESS if successfully 
 *	  TDSP_STATUS_xxx if unsucessfully
 *************************************************************************/
TDSP_STATUS
UsbDev_Init(PDSP_ADAPTER_T pAdap)
{
	UINT32 i;
	TDSP_STATUS status = STATUS_FAILURE;
	PUSB_CONTEXT usb_context = NULL;

    PUSB_INFO  usb_info = &pAdap->usb_info;

	DBG_WLAN__USB(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__);  
	//alloc memory for usb device struct
	usb_context = sc_memory_alloc(sizeof(USB_CONTEXT));

	if(NULL == usb_context)
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] alloc usb device memory failed!\n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	sc_memory_set(usb_context,0,sizeof(usb_context));
	//init usb info
	usb_context->tdsp_adap = (PVOID)pAdap;
	usb_context->usb_intf = usb_info->usb_intf;
	usb_context->usb_dev  = usb_info->usb_dev;
	
	//create usb pipes
	status = _UsbDev_Create_Usb(usb_context,usb_info);
	
	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] create usb pipe failed!\n",__FUNCTION__);
		UsbDev_Release(pAdap);
		return status;
	}
    
    sc_tasklet_init(&usb_context->usb_complete_task, _usb_complete_task, (ULONG)usb_context);	
    _urb_queue_init(&usb_context->urb_pool);
	_urb_queue_init(&usb_context->complete_q);
	for(i = 0; i < MAX_URB_ENTRY_NUM; i++)
	{
		//link all the nodes
		_urb_queue_tail(&usb_context->urb_entry_buf[i],&usb_context->urb_pool);
	}
	pAdap->usb_context = usb_context;

	DBG_WLAN__USB(LEVEL_TRACE, "Leave [%s] \n",__FUNCTION__);  
	return (STATUS_SUCCESS);
}


/**************************************************************************
 *   UsbDev_BuildVendorRequest
 *
 *   Descriptions:
 *	  Formats an URB to send a vendor or class-specific command to a USB device.
 *   Arguments:
 *	  pusb_contex: IN, the pointer of usb device module.
 *	  TransferBuffer: IN, points to a resident buffer for the transfer.
 *	  TransferBufferLength: IN, Specifies the length, in bytes, of the buffer 
 *					  specified in TransferBuffer.
 *	  RequestTypeReservedBits: IN, Specifies a value, from 4 to 31 inclusive,
 *					  that becomes part of the request type code in the 
 *					  USB-defined setup packet.
 *	  Request: IN, Specifies the USB or vendor-defined request code for the 
 *				  device, interface, endpoint, or other device-defined target.
 *	  Value: IN, Is a value, specific to Request, that becomes part of the 
 *				 USB-defined setup packet for the target.
 *	  bIn: IN, the direction of this transfer.
 *	  bShortOk: IN, directs the HCD not to return an error if a packet is 
 *			   received from the device that is shorter than the maximum
 *			   packet size for the endpoint. Otherwise, a short request is
 *			   returns an error condition.
 *	  Link: IN, Points to an caller-initialized URB. Link becomes the 
 *			subsequent URB in a chain of requests with Urb being its 
 *			predecessor.
 *	  Index: IN, Specifies the device-defined identifier if the request
 *			is for an endpoint, interface, or device-defined target. 
 *			Otherwise, Index must be 0. 
 *	  Function: IN, the function code of this transfer.
 *	  pUrb: IN, the pointer to an urb or NULL.
 *   Return Value:
 *	  the pointer to the urb formatted.
 *************************************************************************/
INT32 UsbDev_BuildVendorRequest(
								   PUSB_CONTEXT usb_context,
								   UINT8  In,
								   UINT8* TransferBuffer,
								   UINT32 TransferBufferLength,
								   UINT8 Request,
								   UINT16 Value,
								   UINT16 Index
								   )
{
	INT32	ret;
    TDSP_USB_CTRLREQ req; 
	//Jakio20080528: judge driver state before write
	/*if(Adap_Driver_isHalt( (PDSP_ADAPTER_T)usb_context->tdsp_adap))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb Hardware unnormal\n",__FUNCTION__);
		return STATUS_FAILURE;
	}*/
    req.request = Request;
    req.value   = Value;
    req.index   = Index;
	ret = sc_usb_ctrlmsg( usb_context->usb_dev, 
						USB_EP_CTRL,
						In,					 //request type
						TransferBuffer,		//data buffer
						TransferBufferLength,
						(void*)&req);
	if(0 != sc_usb_issrmv(ret))
	{
		DBG_WLAN__USB(LEVEL_ERR, "[%s] ---Device is suprised removed----ret:%d, Request:%d\n",
							__FUNCTION__,ret, Request);
		Adap_Set_Driver_State((PDSP_ADAPTER_T)usb_context->tdsp_adap, DSP_SUPRISE_REMOVE);
	}
		
	//success
	if(ret < 0)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb_control_msg return failure:%d\n", __FUNCTION__, ret);
	}
	return ret;		
}


///////////////////////////////////////////////////////////////////////////

TDSP_STATUS UsbDev_BuildVendorRequestAsyn(
								   PUSB_CONTEXT usb_context,
								   UINT8* TransferBuffer,
								   UINT32 Length,
								   UINT8 Request,
								   UINT16 Value,
								   UINT8  In,
								   UINT16 Index,
								   USB_COMPLETE_FUNC func,
								   PVOID context)
{
	PUSB_PIPE pipe;
	PUSB_CNTRL_URB_CONTEXT urb_context;	
	INT32		ret;
    TDSP_USB_CTRLREQ req; 
	//Jakio20080528: judge driver state before write
	if(Adap_Driver_isHalt( (PDSP_ADAPTER_T)usb_context->tdsp_adap))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb Hardware unnormal\n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,ASYN_CONTROL_URB);
	if(NULL == pipe)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb end pipe has not created or in in use end\n",__FUNCTION__);
		 return STATUS_FAILURE;
	}	
	if(pipe->in_busy)
	{
		DBG_WLAN__USB(LEVEL_INFO,"[%s] pipe is busy\n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	urb_context =(PUSB_CNTRL_URB_CONTEXT) _UsbDev_Alloc_Urb(pipe);

	if(NULL == urb_context)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] can't alloc urb,pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}

	//fill urb context
	urb_context->usb_context = usb_context;
	urb_context->urb_type  = ASYN_CONTROL_URB;
	urb_context->func_context = context;
	urb_context->complete_func = func;

    
    //fill asyn reqest struct
    req.value   = Value;
    req.index   = Index;
    req.request = Request;
    req.len     = Length;
    sc_usb_set_ctlrreq(urb_context->set_up,(void *)&req);
    
    //submit usb ctrl request 
    ret = sc_usb_ctrlreq(urb_context->urb, 
                    usb_context->usb_dev,
                    pipe->ep_num,
                    TransferBuffer, 
                    Length,
                    (void *)urb_context,
                    (void *)urb_context->set_up);
	if(ret != 0)
	{			
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb submit urb failed ",__FUNCTION__);
		_UsbDev_Release_Urb(pipe, (PVOID)urb_context);
		return STATUS_FAILURE;
		
	}
    
	//put the submitted urb to urb_submit_list
	sc_spin_lock(&pipe->pipe_lock);
	QueuePutTail(&pipe->urb_submit_list, &urb_context->link);
    pipe->in_busy = TRUE;
    sc_event_reset(&pipe->pipe_idle_event);
	sc_spin_unlock(&pipe->pipe_lock);
	return  STATUS_SUCCESS;


 
}


/**************************************************************************
 *   UsbPipe_BuildBulkTransfer
 *
 *   Descriptions:
 *	  build bulk transfer.
 *   Arguments:
 *	  pusb_pipe: IN, the pointer of usb pipe module.
 *	  Buffer: IN, points to a resident buffer for the transfer.
 *	  Length: IN, Specifies the length, in bytes, of the buffer.
 *	  bIn: IN, the direction of this transfer.
 *	  Link: IN, points to an caller-initialized URB. 
 *	  bShortOk: IN, can be used if USBD_TRANSFER_DIRECTION_IN is set. 
 *				 If set, directs the HCD not to return an error if a
 *				  packet is received from the device that is shorter
 *				  than the maximum packet size for the endpoint. Otherwise,
 *				  a short request is returns an error condition.
 *	  pUrb: IN, the pointer to an URB or NULL.
 *   Return Value:
 *	  a pointer of urb.
 *************************************************************************/
TDSP_STATUS UsbDev_BuildBulkInTransfer(PUSB_CONTEXT usb_context,
												 PVOID buffer,
												 UINT32 len,
												 USB_COMPLETE_FUNC func,
												 PVOID context)
{
	INT32	 ret;
	PUSB_PIPE pipe = NULL;
	PUSB_URB_CONTEXT urb_context = NULL;

	DBG_WLAN__USB(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);
	if((NULL == usb_context)
		|| (NULL == buffer)
		|| (0 == len)
		|| (NULL == func))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s]:Input param error ,usb_contex = %p ,buffer is %p , len is %d ,func is %p!\n",
					   __FUNCTION__,usb_context,buffer,len,func);
		return STATUS_FAILURE;
	}

	pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,READ_URB);
	if(NULL == pipe)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb end pipe has not created or in in use end\n",__FUNCTION__);
		 return STATUS_FAILURE;
	}	
	if(pipe->in_busy)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}

	urb_context =(PUSB_URB_CONTEXT) _UsbDev_Alloc_Urb(pipe);
		
	if(NULL == urb_context)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] can't alloc urb,pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}	
	//fill urb context
	urb_context->usb_context = usb_context;
	urb_context->urb_type  = READ_URB;
	urb_context->complete_func = func;
	urb_context->func_context = context;	

    
	ret = sc_usb_bulkin(urb_context->urb,
				usb_context->usb_dev,
				pipe->ep_num,
				buffer,
				len,
				(void *)urb_context);

	if(ret != 0)
	{			
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb submit urb failed ",__FUNCTION__);
		_UsbDev_Release_Urb(pipe, (PVOID)urb_context);
		return STATUS_FAILURE;	
	}

	//put the submitted urb to urb_submit_list
	sc_spin_lock(&pipe->pipe_lock);
	QueuePutTail(&pipe->urb_submit_list, &urb_context->link);
    pipe->in_busy = TRUE;
    sc_event_reset(&pipe->pipe_idle_event);
	sc_spin_unlock(&pipe->pipe_lock);
	return  STATUS_SUCCESS;

	
}

TDSP_STATUS UsbDev_BuildBulkOutTransfer( PUSB_CONTEXT usb_context,
													PVOID  buffer,
													UINT32 len,
													USB_COMPLETE_FUNC func,
													PVOID context)	
{
	PUSB_PIPE pipe;
	INT32 ret;
	PUSB_URB_CONTEXT urb_context = NULL;

	DBG_WLAN__USB(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);

	if((NULL == usb_context)
		|| (NULL == buffer)
		|| (0 == len)
		|| (NULL == func))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s]:Input param error ,usb_contex = %d ,buffer is %p , len is %p ,func is %p!\n",
					   __FUNCTION__,usb_context,buffer,len,func);
		return STATUS_FAILURE;
	}

	//Jakio20080528: judge driver state before write
	if(Adap_Driver_isHalt( (PDSP_ADAPTER_T)usb_context->tdsp_adap))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb Hardware unnormal\n",__FUNCTION__);
		return STATUS_FAILURE;
	}


	pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,WRITE_BODY_URB);
	if(NULL == pipe)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb end pipe has not created or in in use end\n",__FUNCTION__);
		 return STATUS_FAILURE;
	}	
	if(pipe->in_busy)
	{
		DBG_WLAN__USB(LEVEL_TRACE,"[%s] pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}
	
	urb_context =(PUSB_URB_CONTEXT) _UsbDev_Alloc_Urb(pipe);
	if(NULL == urb_context)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] can't alloc urb,pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}

	//fill urb context
	urb_context->usb_context = usb_context;
	urb_context->urb_type  = WRITE_BODY_URB;
	urb_context->complete_func = func;
	urb_context->func_context = context;
	//2 Fill urb
	ret = sc_usb_bulkout(urb_context->urb, 
				usb_context->usb_dev,
				pipe->ep_num,
				buffer,
				len,
				(void *)urb_context);
	if(ret != 0)
	{			
		pipe->in_busy = FALSE;
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb submit urb failed ",__FUNCTION__);
		_UsbDev_Release_Urb(pipe, (PVOID)urb_context);
		return STATUS_FAILURE;
		
	}

	//put the submitted urb to urb_submit_list
	sc_spin_lock(&pipe->pipe_lock);
	QueuePutTail(&pipe->urb_submit_list, &urb_context->link);
    pipe->in_busy = TRUE;
    sc_event_reset(&pipe->pipe_idle_event);
	sc_spin_unlock(&pipe->pipe_lock);
	return  STATUS_SUCCESS;
}





/**************************************************************************
 *   UsbPipe_BuildInterruptTransfer
 *
 *   Descriptions:
 *	  build interrupt transfer.
 *   Arguments:
 *	  pusb_pipe: IN, the pointer of usb pipe module.
 *	  Buffer: IN, points to a resident buffer for the transfer.
 *	  Length: IN, Specifies the length, in bytes, of the buffer.
 *	  bIn: IN, the direction of this transfer.
 *	  Link: IN, points to an caller-initialized URB. 
 *	  bShortOk: IN, can be used if USBD_TRANSFER_DIRECTION_IN is set. 
 *				 If set, directs the HCD not to return an error if a
 *				  packet is received from the device that is shorter
 *				  than the maximum packet size for the endpoint. Otherwise,
 *				  a short request is returns an error condition.
 *	  pUrb: IN, the pointer to an URB or NULL.
 *   Return Value:
 *	  a pointer of urb.
 *************************************************************************/
TDSP_STATUS UsbDev_BuildInterruptTransfer(PUSB_CONTEXT usb_context,						  
													 PVOID buffer,
													 UINT32 len,
													 USB_COMPLETE_FUNC func,
													 PVOID context)
{
	INT32	ret;
	PUSB_PIPE pipe = NULL;
	PUSB_URB_CONTEXT urb_context = NULL;
	ULONG flags;
	DBG_WLAN__USB(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);

	if((NULL == usb_context)
		|| (NULL == buffer)
		|| (0 == len)
		|| (NULL == func))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s]:Input param error ,usb_contex = %p ,buffer is %p , len is %d ,func is %p!\n",
					   __FUNCTION__,usb_context,buffer,len,func);
		return STATUS_FAILURE;
	}
    
	pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,INT_URB);
	if(NULL == pipe)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] usb_interrupt_pipe has not created\n",__FUNCTION__);
		return STATUS_FAILURE;
	}
	if(pipe->in_busy)
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}

	urb_context =(PUSB_URB_CONTEXT) _UsbDev_Alloc_Urb(pipe);

	if(NULL == urb_context)
	{
		pipe->in_busy = TRUE;
		DBG_WLAN__USB(LEVEL_ERR,"[%s] can't alloc urb,pipe is busy\n",__FUNCTION__);
		return STATUS_PENDING;
	}
	
	//fill urb context
	urb_context->usb_context = usb_context;
	urb_context->urb_type  = INT_URB;
	urb_context->complete_func = func;
	urb_context->func_context = context;
	ret = sc_usb_rcvint(urb_context->urb,
				usb_context->usb_dev,
				pipe->ep_num,
				buffer,
				len,
				(void *)urb_context,
				8);

	if(ret != 0)
	{
		//success
		DBG_WLAN__USB(LEVEL_ERR,"[%s] Fatal error, submit interrupt urb failed\n",__FUNCTION__);
		return STATUS_FAILURE;
	}
	//put the submitted urb to urb_submit_list
	sc_spin_lock_irqsave(&pipe->pipe_lock,flags);
	QueuePutTail(&pipe->urb_submit_list, &urb_context->link);
    pipe->in_busy = TRUE;
    sc_event_reset(&pipe->pipe_idle_event);
	sc_spin_unlock_irqrestore(&pipe->pipe_lock,flags);
	return STATUS_SUCCESS;	
}

//TRUE: cancel irp successfully
//FALSE:not cancel or cancel failure
BOOLEAN _UsbDev_CancelTransfer(PUSB_CONTEXT usb_context, USB_URB_TYPE urb_type, UINT8 link_no)
{
	PURB	pUrb;
	PUSB_PIPE pipe; 
	PUSB_URB_CONTEXT urb_context = NULL;
	if((urb_type < ASYN_CONTROL_URB)|| (urb_type >INT_URB))
	{
		DBG_WLAN__USB(LEVEL_ERR,"[%s] urb type is %d\n",__FUNCTION__,urb_type);
		return FALSE;
	}
	pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,urb_type);
	if(NULL == pipe)
	{

		DBG_WLAN__USB(LEVEL_ERR,"[%s] get usb pipe failed,pipe type is %d!\n",
				__FUNCTION__,
				urb_type);
		return FALSE;
	}


	sc_spin_lock(&pipe->pipe_lock);
	urb_context = (PUSB_URB_CONTEXT)QueueGetHead(&pipe->urb_submit_list);
    sc_spin_unlock(&pipe->pipe_lock);
	
	while(NULL != urb_context)
	{
		if(link_no == urb_context->link_no)
		{
			pUrb = urb_context->urb;	
			sc_urb_unlink(pUrb);
			//remove the urb from submit list and return it to urb list
			sc_spin_lock(&pipe->pipe_lock);
			QueueRemoveEle(&pipe->urb_submit_list, &urb_context->link);
			QueuePutTail(&pipe->urb_list, &urb_context->link);
			sc_spin_unlock(&pipe->pipe_lock);
           //set pipe busy flag to false
		    if(QueueEmpty(&pipe->urb_submit_list))
		    {
			    pipe->in_busy = FALSE;
            }
            return TRUE;
		}
		urb_context = (PUSB_URB_CONTEXT)QueueGetNext(&urb_context->link);	   
	}

	//can't find the urb to be canceld
	DBG_WLAN__USB(LEVEL_ERR,"[%s] can't find the urb to be canceled,link no is %d!\n",
		__FUNCTION__,
		link_no);
	return FALSE;	
}

BOOLEAN UsbDev_CancelBulkInTransfer(PUSB_CONTEXT usb_context)
{
   DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
   return _UsbDev_CancelTransfer(usb_context, READ_URB,0);
}


BOOLEAN UsbDev_CancelBulkOutTransfer(PUSB_CONTEXT usb_context)
{
   DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
   return  _UsbDev_CancelTransfer(usb_context, WRITE_BODY_URB,0);
}


BOOLEAN UsbDev_CancelInterruptTransfer(PUSB_CONTEXT usb_context)
{
   DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
   return  _UsbDev_CancelTransfer(usb_context, INT_URB,0);
}


BOOLEAN UsbDev_CancelAsyRequest(PUSB_CONTEXT usb_context)
{
   DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
   return  _UsbDev_CancelTransfer(usb_context, ASYN_CONTROL_URB,0);
}



BOOLEAN UsbDev_WaitBulkOutIdle(PUSB_CONTEXT usb_context,UINT32 timeout)
{
    PUSB_PIPE pipe; 
    DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
    pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,WRITE_BODY_URB);
	if(NULL == pipe)
	{

		DBG_WLAN__USB(LEVEL_ERR,"[%s] get bulk out pipe failed\n",
				__FUNCTION__);
		return FALSE;
	}
    if(0 == sc_event_wait(&pipe->pipe_idle_event, timeout))
    {
        return FALSE;
    }
    return TRUE;
}

BOOLEAN UsbDev_WaitAsyRequestIdle(PUSB_CONTEXT usb_context,UINT32 timeout)
{
    PUSB_PIPE pipe; 
    DBG_WLAN__USB(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);
    pipe = (PUSB_PIPE)_UsbDev_Get_Pipe(usb_context,ASYN_CONTROL_URB);
	if(NULL == pipe)
	{

		DBG_WLAN__USB(LEVEL_ERR,"[%s] get usb asyn control pipe failed,\n",
				__FUNCTION__);
		return FALSE;
	}
    if(0 == sc_event_wait(&pipe->pipe_idle_event, timeout))
    {
        return FALSE;
    }
    return TRUE;
}