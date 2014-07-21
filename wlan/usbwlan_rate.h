 /***********************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  
  * FILENAME		:usbwlan_rate.h         VERSION:1.2
  * CREATE DATE	:2008/12/17
  * PURPOSE:	This file includes functions related to 802.11, 802.3 protocol 
  *			header transforming when sending data out or receiving data in.
  *
  * AUTHORS:     
  *
  * DECLARATION:  This document contains confidential proprietary information that
  *               is solely for authorized personnel. It is not to be disclosed to
  *               any unauthorized person without prior written consent of 3DSP
  *               Corporation.		
  ***********************************************************************/
#ifndef __DSP_AUTORATE_H
#define __DSP_AUTORATE_H
#include "precomp.h"

VOID Rate_set_original_rate(PDSP_ADAPTER_T pAdap, UINT8 set_rate);

VOID Rate_up(PDSP_ADAPTER_T pAdap, UINT32 rank);

VOID Rate_down_By_RSSI(PDSP_ADAPTER_T pAdap, UINT32 rank);

VOID Rate_down_Directly(	PDSP_ADAPTER_T pAdap, UINT32 rank);

UINT8 get_true_rts_rate(PDSP_ADAPTER_T pAdap);

VOID Rate_Up_To_Standard(PDSP_ADAPTER_T pAdap);

VOID Rate_Save_UsingRate(PDSP_ADAPTER_T pAdap);

VOID Rate_Retuen_UsingRate(PDSP_ADAPTER_T pAdap);

VOID Rate_Calc_Smooth(PDSP_ADAPTER_T pAdap,UINT8 txrate);

VOID Adap_set_support_rate(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type,PUINT8 support_rate_list,PUINT8 support_rate_len);

VOID Adap_SetRateIndex(PDSP_ADAPTER_T pAdap);

//#define Adap_get_rate_from_index(pAdap, rateIndex)	_Adap_get_rate_from_index(pAdap, rateIndex, __FUNCTION__,__LINE__)

UINT8 Adap_get_rate_from_index(PDSP_ADAPTER_T pAdap, UINT16 rateIndex);

UINT8 Adap_get_rateindex_from_rate(PDSP_ADAPTER_T pAdap, UINT8 rate);

UINT8 Get_Lower_Txrate(PDSP_ADAPTER_T pAdap, UINT8  curRate);
	
BOOLEAN		Rate_IsSupportRate(UINT8 rate);
#endif /*file end */
