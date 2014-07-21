#ifndef DSP_INT_H
#define DSP_INT_H

/***********************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:     DSP_Int.h      CURRENT VERSION: 1.00.01
* PURPOSE:      mainly define some macros used for MAC's interrupt.
* 
*
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.        
*
**********************************************************************/

#include "precomp.h"
/*--macros------------------------------------------------------------*/

#define  INT_MASK_ALL_BIT        (0xffffffff)
#define  INT_MAX_ERR_COUNT        100
#define  INT_SET_MASK_PktRXComplete(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_PktRXComplete)
#define  INT_SET_MASK_PktTXComplete(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_PktTXComplete)
#define  INT_SET_MASK_RxDesFull(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_RxDesFull)
#define  INT_SET_MASK_UnrecoverableError(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_UnrecoverableError)
#define  INT_SET_MASK_BeaconSent(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_BeaconSent)
#define  INT_SET_MASK_BeaconChanged(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_BeaconChanged)
#define  INT_SET_MASK_PhyRegRdOK(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_PhyRegRdOK)
#define  INT_SET_MASK_SoftInterrupt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_SoftInterrupt)
#define  INT_SET_MASK_GpiEvent(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_GpiEvent)
#define  INT_SET_MASK_AtimWMStart(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_AtimWMStart)
#define  INT_SET_MASK_TbttUpdate(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_TbttUpdate)
#define  INT_SET_MASK_GenTimerInt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_GenTimerInt)
#define  INT_SET_MASK_AcriveReq(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_AcriveReq)
#define  INT_SET_MASK_BeaconLost(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_BeaconLost)
#define  INT_SET_MASK_AckFailCntInt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_AckFailCntInt)
#define  INT_SET_MASK_FcsEnCntInt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_FcsEnCntInt)
#define  INT_SET_MASK_RtsSucCntInt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_RtsSucCntInt)
#define  INT_SET_MASK_WepProCntInt(n)  (UINT32)((UINT32)n | (UINT32)ISR_Event_WepProCntInt)


#define  INT_CLR_MASK_PktRXComplete(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_PktRXComplete)
#define  INT_CLR_MASK_PktTXComplete(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_PktTXComplete)
#define  INT_CLR_MASK_RxDesFull(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_RxDesFull)
#define  INT_CLR_MASK_UnrecoverableError(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_UnrecoverableError)
#define  INT_CLR_MASK_BeaconSent(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_BeaconSent)
#define  INT_CLR_MASK_BeaconChanged(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_BeaconChanged)
#define  INT_CLR_MASK_PhyRegRdOK(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_PhyRegRdOK)
#define  INT_CLR_MASK_SoftInterrupt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_SoftInterrupt)
#define  INT_CLR_MASK_GpiEvent(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_GpiEvent)
#define  INT_CLR_MASK_AtimWMStart(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_AtimWMStart)
#define  INT_CLR_MASK_TbttUpdate(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_TbttUpdate)
#define  INT_CLR_MASK_GenTimerInt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_GenTimerInt)
#define  INT_CLR_MASK_AcriveReq(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_AcriveReq)
#define  INT_CLR_MASK_BeaconLost(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_BeaconLost)
#define  INT_CLR_MASK_AckFailCntInt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_AckFailCntInt)
#define  INT_CLR_MASK_FcsEnCntInt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_FcsEnCntInt)
#define  INT_CLR_MASK_RtsSucCntInt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_RtsSucCntInt)
#define  INT_CLR_MASK_WepProCntInt(n)  (UINT32)((UINT32)n & ~(UINT32)ISR_Event_WepProCntInt)

#define MAX_USB_INTERRUPT_AREA			0x08				//中断缓冲区暂时定义为8字节，随硬件改变而改变


/*--constants and types------------------------------------------------*/
typedef struct _INT_CONTEXT
{
    UINT8 int_buffer[MAX_USB_INTERRUPT_AREA];
    BOOLEAN int_irp_working;
    UINT8   irp_count;
}INT_CONTEXT, *PINT_CONTEXT;

TDSP_STATUS Int_SubmitInt(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Int_Init(PDSP_ADAPTER_T pAdap);
void Int_Release(PDSP_ADAPTER_T pAdap);
BOOLEAN Int_CancelInt(PDSP_ADAPTER_T pAdap);
/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/

#endif
