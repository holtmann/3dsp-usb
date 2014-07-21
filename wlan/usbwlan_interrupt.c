/************************************************************************
(c) 2003-04, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:

3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
**************************************************************************
$RCSfile: usbwlan_interrupt.c,v $ 
$Revision: 1.6 $ 
$Date: 2010/08/16 01:53:23 $
**************************************************************************/


#include "precomp.h"

static char* TDSP_FILE_INDICATOR="INTRU";
/*
#include <ndis.h>
#include "usbwlan_wlan.h"
#include "usbwlan_Sw.h"
#include "usbwlan_dbg.h"
#include "usbwlan_interrupt.h"
#include "usbwlan_mng.h"
#include "usbwlan_pr.h"
*/
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
/*--file local variables-----------------------------------------------*/
/*--file local function prototypes-------------------------------------*/
/*--functions ---------------------------------------------------------*/
/***********************************************************************************
 *  Function Name:    int_handlers
 *  Description:
 *        this function parse interrupt code and dispatch task to corresponding process.
 *  Arguments:
 *        adaptor: IN,pointer to driver information structure.
 *        pBuf:IN,interrupt code buffer.
 *    Return  Value: 
 *        STATUS_SUCCESS, executed successfully.
 *        STATUS_FAILURE,fail.
* *********************************************************************************/

TDSP_STATUS Int_Init(PDSP_ADAPTER_T pAdap)
{
    ASSERT(pAdap);
    DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
    pAdap->int_context = sc_memory_alloc(sizeof(INT_CONTEXT));
    if(NULL == pAdap->int_context)
    {
        DBG_WLAN__INT(LEVEL_ERR,"[%s] :alloc memory for int context failed\n",__FUNCTION__);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    sc_memory_set(pAdap->int_context,0,sizeof(INT_CONTEXT));

    return STATUS_SUCCESS;
}


void Int_Release(PDSP_ADAPTER_T pAdap)
{
    PINT_CONTEXT int_context;
    ASSERT(pAdap);
    DBG_WLAN__INT(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
    int_context = (PINT_CONTEXT)pAdap->int_context;
    if(NULL == int_context)
        return;
    #if 0
    if((int_context->int_irp_working) && ( !UsbDev_CancelInterruptTransfer(pAdap->usb_context)))
    {
         DBG_WLAN__INT(LEVEL_ERR,"[%s] :cancle int transfer failed\n",__FUNCTION__);
    }
    #endif
    sc_memory_free(pAdap->int_context);
    pAdap->int_context = NULL;
    
}



TDSP_STATUS Int_handlers(PDSP_ADAPTER_T pAdap, PUINT8 pBuf)
{

    PUINT8 tmp;
    PUINT32 pTmpEvent;
    PUINT32 pTmpStatus;


    tmp = pBuf;
    if(*tmp != INT_TYPE_3DSP_HW_EVENT_MAP)
        return STATUS_FAILURE;

    pTmpEvent = (PUINT32)(pBuf +1);
//    DBGSTR_INT(("int_handlers().  Event Register: %8x\n", *pTmpEvent));
    if(*pTmpEvent & BITS_INT_EVENT__RX_FIFO_UNDERRUN)
    {
        DBG_WLAN__INT(LEVEL_ERR,"[%s] :event RX_FIFO_OVERFLOW occurs\n",__FUNCTION__);
    }


    //here handle status 
    pTmpStatus = (PUINT32)(pBuf +4);
//    DBGSTR_INT(("int_handlers().  Status Register: %8x\n", *pTmpStatus));
    if(*pTmpStatus & BITS_STATUS__TX_STOPPED)
    {
        DBG_WLAN__INT(LEVEL_ERR,"[%s]: status TX stopped occurs\n",__FUNCTION__);
        
        Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task, 
                        DSP_TASK_EVENT_HANDLE_TX_STOPPED,
                        DSP_TASK_PRI_NORMAL,
                        NULL,
                        0);
        
    }

    return STATUS_SUCCESS;
}


/**************************************************************************
 *   Adap_ProcessInt
 *
 *   Descriptions:
 *      This function will call the function for processing interrupt of frag
 *      module.    
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *      pBuf: IN, the buffer saving the processing data.
 *   Return Value:
 *      NONE
 *************************************************************************/
TDSP_STATUS Int_Process(PDSP_ADAPTER_T pAdap, PUINT8 pBuf)
{
    PDSP_REG_READ pDspReg;    
    pDspReg = (PDSP_REG_READ)pBuf;
    
    //read dsp register interrupt
    if(pDspReg->type == INT_TYPE_READ_3DSP_REG)
    {
        sc_memory_copy(&pAdap->DspRegRead, pBuf, sizeof(DSP_REG_READ));
        sc_event_set(&pAdap->DspReadEvent);
    }
    else if(pDspReg->type == INT_READ_POWER_TABLE)
    {
        sc_memory_copy(&pAdap->DspRegRead, pBuf, sizeof(DSP_REG_READ));
        sc_event_set(&pAdap->DspReadEvent);
    }
    else if(pDspReg->type == VCMD_API_GET_8051VER_REQUEST)
    {
        sc_memory_copy(&pAdap->DspRegRead, pBuf, sizeof(DSP_REG_READ));
        sc_event_set(&pAdap->DspReadEvent);
		DBG_WLAN__INT(LEVEL_TRACE,"[%s]: GET_8051VER int, type = %x, sub_type = %x,offset = %x,result = %x *****\n",
                __FUNCTION__,
                pDspReg->type,
            	pDspReg->sub_type,
            	pDspReg->addr,
            	pDspReg->result);

    }
    else if(pDspReg->type == INT_TYPE_READY_FOR_WLAN_TX)
    {
//        sc_memory_copy(&pAdap->DspRegRead, pBuf, sizeof(DSP_REG_READ));
//        sc_event_set(&pAdap->DspSendOkEvent);

        if (pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
        {
            //pAdap->DspSendOkFlag = 1;            
        }

    }
    //3 Add more handlers
    
    else if(pDspReg->type == INT_TYPE_3DSP_HW_EVENT_MAP)
    {
        Int_handlers(pAdap, pBuf);    
    }

    else if(pDspReg->type == INT_TYPE_TX_STOP_EVENT)
    {

    
        DBG_WLAN__INT(LEVEL_INFO,"[%s]: int for TX STOP, %x ,auto = %x,join =%x \n",
            __FUNCTION__,
            pDspReg->result,
            pAdap->wlan_attr.fallback_rate_to_use,
            pAdap->wlan_attr.hasjoined);

    }
    else if(pDspReg->type == INT_TYPE_RX_OVERFLOW_5_TIMES)
    {
        if(pDspReg->result != 0 )
        {
            DBG_WLAN__INT(LEVEL_ERR,"[%s]: **** overflow happen ****\n",__FUNCTION__);    
        }
        else
        {

            DBG_WLAN__INT(LEVEL_ERR,"[%s]: **** Overflow reach limited ****\n",__FUNCTION__);
//    pAdap->reconnect_after_reset_interrupt_flag = TRUE;

#if 1

        if ((!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_HARDWARE_RESET))
            && Adap_Driver_isIntWork( pAdap))    //justin: 0822, do not create task while sys reset
        {
            //Stop sending
//            pAdap->bStarveMac = FALSE;
//            pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;
    /*        pAdap->reset_type = RESET_TYPE_SYS_RESET;//RESET_TYPE_NOT_SCAN;
    */        
            //Justin: 0801        // clear all exist tasks
//            Task_Reset((PDSP_TASK_T)pAdap->ppassive_task);

            pAdap->reconnect_after_reset_interrupt_flag = TRUE;
            //pAdap->dsp_fw_mode = pDspReg->sub_type;
            Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_HARDWARE_RESET,DSP_TASK_PRI_HIGH,NULL,0);
        }
//        pAdap->sys_reset_flag = TRUE;
#endif
            }
    }
    //auto rate
    else if(pDspReg->type == INT_TYPE_AUTO_RATE_ADJUST)
    {
        if(    //(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA)&&        //in bss mode
             (pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE)
             &&(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED))
        {
            if(Adap_Driver_isIntWork( pAdap))
            {
                //update rate directly in int function instead of in task.
                if(pDspReg->sub_type == 1 && pDspReg->result < 10)
                {
                    //up 2
                    pDspReg->sub_type = 3;
                }
                
                //Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_AUTORATE,DSP_TASK_PRI_NORMAL,(PUINT8)&pDspReg->sub_type,1);
                //Rate_Retuen_UsingRate(pAdap);
                if(pDspReg->sub_type == INT_SUB_TYPE_AUTO_RATE_UP)
                {
                        //up rate
                        Rate_up(pAdap,0);
                }
                else  if(pDspReg->sub_type == INT_SUB_TYPE_AUTO_RATE_DOWN)
                {	//down rate. 2 rand one time
					//Rate_Retuen_UsingRate(pAdap);
					//Rate_get_downrate_duration
					if(1 == sc_time_downout_duration())
					{
						Rate_down_By_RSSI(pAdap,0);
					}	 
                }    
                else if(pDspReg->sub_type == 3)
                {
                    Rate_up(pAdap,1);
                }
            }
        
            
        }
    }
    else if(pDspReg->type == INT_TYPE_RESET_HARDWARE)
    {
        DBG_WLAN__INT(LEVEL_TRACE,"[%s]:* * * * * Dsp_Reset 8051 int\n",__FUNCTION__);

        if(pDspReg->sub_type !=0)
        {            
            DBG_WLAN__INT(LEVEL_TRACE,"[%s] :* * * * * Dsp_Reset 8051 int,COMBO\n",__FUNCTION__);
            pAdap->dsp_fw_mode = INT_SUB_TYPE_RESET_WITH_COMBO_MODE;
        }    
        else
        {
            DBG_WLAN__INT(LEVEL_TRACE,"[%s] :* * * * * Dsp_Reset 8051 int,SINGLE\n",__FUNCTION__);
            pAdap->dsp_fw_mode = INT_SUB_TYPE_RESET_WITH_SINGLE_MODE;
        }

        //woody debug
        if(pAdap->driver_state == DSP_SYS_RESET)
        {
        
            if (!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SYS_RESET))
            {
            //    pAdap->reconnect_after_reset_interrupt_flag = TRUE;
                
                DBG_WLAN__INT(LEVEL_TRACE,"[%s] :* * * * * Dsp_Reset task again\n",__FUNCTION__);
                Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SYS_RESET,DSP_TASK_PRI_HIGH,NULL,0);
            }
        }
        else if(pAdap->driver_state != DSP_DRIVER_INIT
            && pAdap->driver_state != DSP_DRIVER_READY)
        {
            if (!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_HARDWARE_RESET))
            {
            //    pAdap->reconnect_after_reset_interrupt_flag = TRUE;
                
                DBG_WLAN__INT(LEVEL_TRACE,"[%s] :* * * * * hardware reset task again\n",__FUNCTION__);
                Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_HARDWARE_RESET,DSP_TASK_PRI_HIGH,NULL,0);
            }
        }
        else//initialization ...... OR     Oid_SetPowerD0
        {
            DBG_WLAN__INT(LEVEL_TRACE,"[%s] :****8051 in normal working mode****\n",__FUNCTION__);
            sc_event_set(&pAdap->is_8051_ready_event);
        }
    }
    else if(pDspReg->type == VCMD_API_8051_JUMP_MIN_PROCESS)
    {	if(pAdap->is_set_8051_miniloop_failed)//Do not process any interrupt to recover if Oid_SetPowerD3 has failed to set 8051 to miniloop
		{
			DBG_WLAN__INT(LEVEL_TRACE,"[%s] :Do not process any interrupt to recover if Oid_SetPowerD3 has failed to set 8051 to miniloop\n",__FUNCTION__);
			return (STATUS_SUCCESS);
		}
		else
		{
			DBG_WLAN__INT(LEVEL_TRACE,"****8051 in miniloop mode****\n");
			pAdap->hw_8051_work_mode = INT_8051_IN_MINILOOP_MODE;
		}	
    }
    else if(pDspReg->type == VCMD_API_RELEASE_CODE_DSPCODE)
    {
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :*****8051 in normal working mode****\n",__FUNCTION__);
        pAdap->hw_8051_work_mode = INT_8051_IN_WORKING_MODE;
    }
    else if(pDspReg->type == INT_UNENCRYPT)
    {
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :*****UNENCRYPTION TABLE happen *****\n",__FUNCTION__);
    }
    else if(pDspReg->type == 0x16)//print rx fifo overflow.   for check aes halt problem
    {
        pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
            DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;    

        if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)    //justin: in power save mode, wait more time
            pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
            
        
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :****rx overflow. type= 0x%x, sub type = %x, addr = %x, result = %x *****\n",
                                        __FUNCTION__,pDspReg->type,
                                        pDspReg->sub_type,pDspReg->addr,
                                        pDspReg->result);
    }
    //else if(pDspReg->type == 0x50)
    else if(pDspReg->type == INT_TYPE_RETRY_LIMIT_EVENT)
    {
        Tx_Process_Retry_Int(pAdap, pDspReg->result);
    }
    else if(pDspReg->type == INT_TX_HANG_HAPPEN)
    {
    	DBG_WLAN__INT(LEVEL_TRACE,"[%s] :TX hang happen from 8051 ,drvstate = %d, subtype = %d, hcnt = %d  \n",
            __FUNCTION__,
            pAdap->driver_state,
            pDspReg->sub_type,
            pAdap->tx_hang_count);
        //hang maybe happen
        if(pAdap->driver_state != DSP_HARDWARE_RESET)
        {
        if(pDspReg->sub_type == 0)    
        {
            if(pAdap->tx_hang_count !=0 )
            {
                pAdap->tx_hang_count = 0;
              
    	
      
                DBG_WLAN__INT(LEVEL_TRACE,"[%s] :*TX hang happen from 8051 \n",__FUNCTION__);
                if(Adap_Reset_Routine(pAdap) == STATUS_PENDING)
                {
                    DBG_WLAN__INT(LEVEL_TRACE,"[%s] :*TX hang calling adap_reset_routine\n",__FUNCTION__);
                        //Adap_Set_Driver_State(pAdap,DSP_HARDWARE_RESET);
                    pAdap->bStarveMac = TRUE;
                    pAdap->wlan_attr.gdevice_info.tx_disable = TRUE;
                    pAdap->txHangFlag = TXHANG_RESETHW;

                }
            }
            else
            {
                //set tx hang montor count
                pAdap->tx_hang_count = TX_HANG_TRANSACT_INTERVAL;
            }
        }
        else
        {
            //clear tx hang monitor due to 8051 restore tx
            pAdap->tx_hang_count = 0;            
            }
        }
    }
    else if (pDspReg->type == CMD_GET_8051Status)
	{		
		DBG_WLAN__INT(LEVEL_TRACE,"[%s] :****Undefined int, type = %x, sub_type = %x,offset = %x,result = %x *****\n",
            __FUNCTION__,
            pDspReg->type,
			pDspReg->sub_type,
			pDspReg->addr,
			pDspReg->result);

		pAdap->is8051InMiniLoop = ((pDspReg->sub_type == 0) ? 1:0);
	}
    else
    {
    #if 1
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :*****Undefined int, type = %x, sub_type = %x,offset = %x,result = %x *****\n",
            __FUNCTION__,
            pDspReg->type,
            pDspReg->sub_type,
            pDspReg->addr,
            pDspReg->result);
    #endif
    }


    
    return (STATUS_SUCCESS);
}


/**************************************************************************
 *   Adap_CompletionIntRoutine
 *
 *   Descriptions:
 *      This completion routine process the condition that a int irp is responsed.
 *   Argumens:
 *      DeviceObject: IN, pointer to device object.
 *      pIrp: IN, Irp that just completed.
 *      Context: IN, Context structure for Irp to be completed.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 *************************************************************************/
void  Int_Completion(  PDSP_ADAPTER_T pAdap,
                            INT32            len,
                            PVOID            context)
{
    PINT_CONTEXT int_context;

    DBG_WLAN__USB(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);
    /* If halt flag is set, driver just counts and does not dispatch any irp. */
    //combo
    if(NULL == pAdap)
    {
        DBG_WLAN__USB(LEVEL_ERR,"[%s] adapter has been already released!\n",__FUNCTION__);
        return;
    }
    int_context = (PINT_CONTEXT)pAdap->int_context;

    if(NULL == int_context)
    {
        DBG_WLAN__USB(LEVEL_ERR,"[%s] int_context has been already released!\n",__FUNCTION__);
        return;
    }
    int_context->int_irp_working = FALSE;
    if (!Adap_Driver_isIntWork(pAdap))
    {
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :IRP INT is not work,ps state is %d\n",
                    __FUNCTION__,pAdap->ps_state);
	    if (pAdap->ps_state == 0)
        {
            Int_SubmitInt(pAdap);
	    
        }
		return;
    }

    if (len <= 0)
    {
        
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] :**************IRP INT fail*****************\n",
                        __FUNCTION__);        
        if (!Adap_Driver_isHalt(pAdap))
        {
 
            DBG_WLAN__INT(LEVEL_TRACE,"[%s]**************IRP INT fail, resubmit.... *****************\n",__FUNCTION__);
            
            //TODO: Handle the case were the Irp did not complete successfully
            //        In this example we simply are resending the URB back to the
            //        bus driver.
            if(++int_context->irp_count > INT_MAX_ERR_COUNT)
            {
                DBG_WLAN__INT(LEVEL_ERR,"[%s]IRP INT fail more than MAX ERR COUNT\n",__FUNCTION__);
                Adap_Set_Driver_State(pAdap, DSP_DRIVER_HALT);
            }
            Int_SubmitInt(pAdap);
            return;
        }        
    }
    // run to here, irp succed.

    int_context->irp_count = 0;
    // if not suitable to our format, just pass it
    if (len!= INT_RETURN_URB_BUFF_LEN)
    {
        DBG_WLAN__INT(LEVEL_TRACE,"[%s] + + + + + + + +Int buff len = %d\n",__FUNCTION__,len);
    }
    else/* Do real int function */
    {       
        Int_Process(pAdap, int_context->int_buffer);
    }
    // Update the transmit stats
    // m_GenStats->rcv_ok++;

    //Resending the URB back to the    bus driver.
    Int_SubmitInt(pAdap);
        
}


TDSP_STATUS Int_SubmitInt(PDSP_ADAPTER_T pAdap)
{

    PINT_CONTEXT int_context = NULL;
    TDSP_STATUS status;
    //Justin: do not re submit if halt or removed
    //combo
    DBG_WLAN__INT(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);
    if(pAdap->driver_state == DSP_DRIVER_HALT)
    {
        DBG_WLAN__INT(LEVEL_ERR,"[%s]: adapt is halt,can not submit int\n",__FUNCTION__);
        return STATUS_PENDING;
    }
    
    ASSERT(pAdap);

    int_context = pAdap->int_context;
    // Reuse the existing URB. Notice that the 6th parameter is
    // the pointer to the URB that was reclaimed. This call reinitializes
    // the URB for reuse.

    if(int_context->int_irp_working)
    {
        DBG_WLAN__INT(LEVEL_ERR, "[%s]:  int transfer is working!\n",__FUNCTION__);
        return STATUS_PENDING;
    }
        
    sc_memory_set(int_context->int_buffer, 0, MAX_USB_INTERRUPT_AREA);
    
    status = UsbDev_BuildInterruptTransfer((PUSB_CONTEXT)pAdap->usb_context,
                                    (PVOID)int_context->int_buffer,
                                    MAX_USB_INTERRUPT_AREA,
                                    Int_Completion,
                                    NULL);
    if(STATUS_SUCCESS != status)
    {
        DBG_WLAN__INT(LEVEL_ERR, "[%s]: submit int transfer failed \n",__FUNCTION__);
    }
    else
    {
        int_context->int_irp_working = TRUE;
    }
    return status;

//    tdsp_sleep(1000);
}

BOOLEAN Int_CancelInt(PDSP_ADAPTER_T pAdap)
{
	PINT_CONTEXT int_context;

	if(	pAdap==NULL
	||	pAdap->int_context==NULL)
	{
		return FALSE;
	}
	
    int_context = (PINT_CONTEXT)pAdap->int_context;


	
    DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
    if(!int_context->int_irp_working)
    {
        DBG_WLAN__INT(LEVEL_ERR, "[%s]:  int transfer is not working!\n",__FUNCTION__);
        return FALSE;
    }
    
    if(!UsbDev_CancelInterruptTransfer(pAdap->usb_context))
    {
        DBG_WLAN__INT(LEVEL_ERR, "[%s]: cancel  int transfer failed!\n",__FUNCTION__);
        return FALSE;
    }

    int_context->int_irp_working = FALSE;

    return TRUE;
}

