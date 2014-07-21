 /***********************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  
  * FILENAME		:usbwlan_rate.c         VERSION:1.2
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
#include "precomp.h"	

static char* TDSP_FILE_INDICATOR="ARATE";

VOID Rate_set_original_rate(
	PDSP_ADAPTER_T pAdap,
	UINT8 			set_rate)
{
	//set to auto rate
	if (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G)
	{
		pAdap->wlan_attr.rate = 
			((set_rate < OP_1_MBPS) || (set_rate > OP_54_MBPS)) ? OP_54_MBPS : set_rate;
	}
	else if (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_B)
	{
		pAdap->wlan_attr.rate =
			(set_rate < OP_1_MBPS || set_rate > OP_11_MBPS) ? OP_11_MBPS : set_rate;
	}
	else
	{		
		ASSERT(0);
		pAdap->wlan_attr.rate = 
			(set_rate < OP_1_MBPS || set_rate > OP_54_MBPS) ? OP_54_MBPS : set_rate;
	}
}


	
/*******************************************************************
 *   Rate_up
 *   
 *   Descriptions:
 *      This routine make TX rate up with corresponding rank
 *   Arguments:
 *     rank:
	   0:  rate should be up by one rank each time for 11g.
	   1:  rate should be up by two rank each time for 11g.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Rate_up(PDSP_ADAPTER_T pAdap, UINT32	rank)
{
	UINT16 index;
	UINT32 old_rate;
	PMNG_STRU_T pt_mng = (PMNG_STRU_T)pAdap->pmanagment;		
	PDSP_ATTR_T  pt_mattr = &pAdap->wlan_attr;
//	UINT8   rate;
	
	pAdap->rate_up_to_24mbps_count = 0;	
	index = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.rate);

	if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
		&& (pt_mattr->is_exist_ibss_b_sta))
	{
		if((pAdap->wlan_attr.rate & 0x7f) > OP_11_MBPS)
		{
			DBG_WLAN__MLME(LEVEL_TRACE, "Rate_up too high in ibss b set, just to OP_11_MBPS:   rate = %0x,rate_len = %x \n",pAdap->wlan_attr.rate,pAdap->wlan_attr.support_rate_len);

			pAdap->wlan_attr.rate = OP_11_MBPS;
			pt_mng->rateIndex = 3;
			return;
		}
	}
	
	//has be highest rate
	if ((index +1) >=  pAdap->wlan_attr.support_rate_len)
	{
		return;
	}

	index++;		

	//set new rate
	old_rate = pAdap->wlan_attr.rate;
	//DBG_WLAN__MLME(LEVEL_TRACE, "up old rate = %x,pAdap->wlan_attr.support_rate_len=%d\n",pAdap->wlan_attr.rate,pAdap->wlan_attr.support_rate_len);
	pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap,index);
	//fileter 6mpbs and 9mbps
	if ((pAdap->wlan_attr.rate == OP_6_MBPS) ||
		(pAdap->wlan_attr.rate == OP_9_MBPS))
	{
		pAdap->wlan_attr.rate = OP_12_MBPS;
		pt_mng->rateIndex = 6;
	}
	else
	{
		pt_mng->rateIndex = index;
	}
	
//	Adap_SetRateIndex(pAdap);
#ifndef NEW_RETRY_LIMIT
	Adap_set_basicrate_map_ofdm11g(pAdap);
#endif
	DBG_WLAN__MLME(LEVEL_TRACE, "%x->%x\n",old_rate,pAdap->wlan_attr.rate);
}

VOID Rate_Save_UsingRate(PDSP_ADAPTER_T pAdap)
{
	if (!pAdap->wlan_attr.rate_change_flag)
	{
		PMNG_STRU_T     pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
		
		pAdap->wlan_attr.rate_using = pt_mng->rateIndex;//pAdap->wlan_attr.rate;
		pAdap->wlan_attr.rate_change_flag = 1;
	}	
}

VOID Rate_Retuen_UsingRate(PDSP_ADAPTER_T pAdap)
{
	if (pAdap->wlan_attr.rate_change_flag)
	{
		PMNG_STRU_T     pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
		
		pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap, pAdap->wlan_attr.rate_using);
		pt_mng->rateIndex = pAdap->wlan_attr.rate_using;
//		Adap_SetRateIndex(pAdap);
		pAdap->wlan_attr.rate_change_flag = 0;
	}	
}
/*******************************************************************
 *   Rate_down_By_RSSI
 *   
 *   Descriptions:
 *      This routine make TX rate down with corresponding rank,mean while it should consider the current rssi value.
 *   Arguments:
 *     rank:
	   0:  rate should be down by one rank each time for 11g.
	   1:  rate should be down by two rank each time for 11g.
 *   Return Value:
 *      NONE
 ******************************************************************/
 
VOID Rate_down_By_RSSI(PDSP_ADAPTER_T pAdap, UINT32 rank)
{
	UINT16 index;
	UINT32 old_rate;
	PMNG_STRU_T pt_mng = (PMNG_STRU_T)pAdap->pmanagment;		
#ifdef ANTENNA_DIVERSITY
	DSP_WRITE_MAILBOX	dsp_mailbox;
#endif	
	
//	UINT8   rate;
//	UINT16 index_for_24mbps;
	
#if 0		
//	DBG_WLAN__MLME(LEVEL_TRACE, "Rate_down_By_RSSI :  = %x \n", pAdap->wlan_attr.rssi);
	//if current rate >24mbps,then execute down rate directly
	//if((pAdap->wlan_attr.rate & 0x7f) <= OP_24_MBPS)
	{
		//if current rate <=24mbps,then execute down rate with rssi situation
		//this can make to select a good rate for transmit
		if(GetRssi(pAdap->wlan_attr.rssi)  > AUTO_RATE_ADJUST_MIN_RSSI_THRESHOLD)
		{
			DBGSTR_RETRY(("No down rate due to RSSI is ok,rssi = %x\n",pAdap->wlan_attr.rssi));
			return;
		}
	}
#endif		
	
	pAdap->rate_up_to_24mbps_count = 0;

	if (OP_1_MBPS == pAdap->wlan_attr.rate)	//has be lowest rate
	{
		DBG_WLAN__MLME(LEVEL_TRACE, "Has been in lowerst rate \n");
#ifdef ANTENNA_DIVERSITY
		//retry limit reach lowest rate 1mbps
		//antenna switch 
		if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
		{
			dsp_mailbox.type = TASK_MAILBOX_TYPE_ANTENNA;
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_ACCESS_DSP_MAILBOX))
			{
				DBG_WLAN__MLME(LEVEL_TRACE, "^^^^^^switch antenna,current is %x\n",pAdap->wlan_attr.antenna_num);
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
					DSP_TASK_EVENT_ACCESS_DSP_MAILBOX,DSP_TASK_PRI_NORMAL,(PUINT8)&dsp_mailbox,sizeof(DSP_WRITE_MAILBOX));	

				//Set current rate to 11mbps after switching antenna
				pAdap->wlan_attr.rate = OP_11_MBPS;
				pt_mng->rateIndex = 3;
			}
		}	
#endif	
		return;
	}

	index = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.rate) -1;

	if (rank != 0)	//not lowest rate
		index--;

	//set new rate
	old_rate = pAdap->wlan_attr.rate;
	//DBG_WLAN__MLME(LEVEL_TRACE, "DOWN OLD = %x		rssi=%x\n", pAdap->wlan_attr.rate, pAdap->wlan_attr.rssi);
	pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap,index);
	//fileter 6mpbs and 9mbps
	if ((pAdap->wlan_attr.rate == OP_6_MBPS) ||
		(pAdap->wlan_attr.rate == OP_9_MBPS))
	{
		pAdap->wlan_attr.rate = OP_11_MBPS;
		pt_mng->rateIndex = 3;
	}
	else
	{
		pt_mng->rateIndex = index;
	}

//	Adap_SetRateIndex(pAdap);
#ifndef NEW_RETRY_LIMIT	
	Adap_set_basicrate_map_ofdm11g(pAdap);
#endif
	DBG_WLAN__MLME(LEVEL_TRACE, "%x->%x(%x)\n",old_rate, pAdap->wlan_attr.rate, pAdap->wlan_attr.rssi);
}

VOID Rate_Up_To_Standard(PDSP_ADAPTER_T pAdap)
{
	UINT16 index;
	PMNG_STRU_T     pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
//	UINT8   rate;

	if (GetRssi(pAdap->wlan_attr.rssi) < AUTO_RATE_ADJUST_MIN_RSSI_THRESHOLD)
	{
		DBG_WLAN__MLME(LEVEL_TRACE, "No up rate due to RSSI is not ok,rssi = %x\n",pAdap->wlan_attr.rssi);
		return;
	}

	if ((pAdap->wlan_attr.rate & 0x7f) <OP_24_MBPS)
	{
		//11gmode
		if (pAdap->wlan_attr.support_rate_len > 4)	
		{
			pAdap->wlan_attr.rate = OP_24_MBPS;
			pt_mng->rateIndex = 8;
//			Adap_SetRateIndex(pAdap);
		}
		else
		{
			//for 11b
			pAdap->wlan_attr.rate = OP_11_MBPS;
			pt_mng->rateIndex = 3;
//			Adap_SetRateIndex(pAdap);
		}
	}
	else
	{
		index = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.rate);
		DBG_WLAN__MLME(LEVEL_TRACE, "Rate_up :  = %x index = %0x , rate = %0x,rate_len = %x \n",pAdap->wlan_attr.rssi,index,pAdap->wlan_attr.rate,pAdap->wlan_attr.support_rate_len);
		//has be highest rate
		if((index +1) >=  pAdap->wlan_attr.support_rate_len)
		{
			return;
		}

		index++;		

		//set new rate
		DBG_WLAN__MLME(LEVEL_TRACE, "up old rate = %x\n",pAdap->wlan_attr.rate);
		pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap,index);
		pt_mng->rateIndex = index;
//		Adap_SetRateIndex(pAdap);		
	}
#ifndef NEW_RETRY_LIMIT
	Adap_set_basicrate_map_ofdm11g(pAdap);
#endif
}

/*******************************************************************
 *   Rate_down_Directly
 *   
 *   Descriptions:
 *      This routine make TX rate down with corresponding rank,do not consider the rssi value.
 *   Arguments:
 *     rank:
	   0:  rate should be down by one rank each time for 11g.
	   1:  rate should be down by two rank each time for 11g.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Rate_down_Directly(	PDSP_ADAPTER_T pAdap, UINT32 rank)
{
	UINT16 index;
	UINT32 old_rate;
	PMNG_STRU_T pt_mng = (PMNG_STRU_T)pAdap->pmanagment;	
//	UINT8   rate;
//	UINT16 index_for_24mbps;
	
	//if current rate >24mbps,then execute down rate directly
#if 0 //Consider rssi value.
	if (0)//(pAdap->wlan_attr.rate & 0x7f) <= OP_24_MBPS)
	{
		//if current rate <=24mbps,then execute down rate with rssi situation
		//this can make to select a good rate for transmit
		if(GetRssi(pAdap->wlan_attr.rssi)  > AUTO_RATE_ADJUST_MIN_RSSI_THRESHOLD)
		{
			DBG_WLAN__MLME(LEVEL_TRACE, "No down rate due to RSSI is ok,rssi = %x\n",pAdap->wlan_attr.rssi);
			return;
		}
	}
#endif

	if (OP_1_MBPS == pAdap->wlan_attr.rate)	//has be lowest rate
	{
		//DBG_WLAN__MLME(LEVEL_TRACE, "RATE HAS BEEN LOWEST \n");
		return;
	}

	index = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.rate) -1;

	if (rank!= 0)	//not lowest rate
		index--;

	//set new rate
	pAdap->rate_up_to_24mbps_count = 0;
	old_rate = pAdap->wlan_attr.rate;
	//DBG_WLAN__MLME(LEVEL_TRACE, "down old rate = %x			rssi = %x\n", pAdap->wlan_attr.rate, pAdap->wlan_attr.rssi);
	pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap,index);
	//fileter 6mpbs and 9mbps
	if ((pAdap->wlan_attr.rate == OP_6_MBPS) ||
		(pAdap->wlan_attr.rate == OP_9_MBPS))
	{
		pAdap->wlan_attr.rate = OP_11_MBPS;
		pt_mng->rateIndex = 3;
	}
	else
	{
		pt_mng->rateIndex = index;
	}

//	Adap_SetRateIndex(pAdap);
	//set new rate
	DBG_WLAN__MLME(LEVEL_TRACE, "%x->%x(%x)\n",old_rate, pAdap->wlan_attr.rate, pAdap->wlan_attr.rssi);
#ifndef NEW_RETRY_LIMIT	
	Adap_set_basicrate_map_ofdm11g(pAdap);
#endif
}

UINT8 Get_Lower_Txrate(PDSP_ADAPTER_T pAdap, UINT8  curRate)
{
	UINT16 index;
	UINT8   rate;
	
	index = Adap_get_rateindex_from_rate(pAdap, curRate);

	if(index != 0)
	{
		index--;	
	}
	rate = Adap_get_rate_from_index(pAdap,index);

	if(((rate & 0x7f) ==OP_6_MBPS) ||
		((rate & 0x7f) == OP_9_MBPS))
	{
		rate = OP_11_MBPS;
	}
	
	return rate;
}

UINT8 get_true_rts_rate(PDSP_ADAPTER_T pAdap)
{
	UINT8 rts_rate;
	if ((pAdap->wlan_attr.cts_to_self_config == CTS_SELF_ENABLE ) &&
		(pAdap->wlan_attr.cts_en == CTS_SELF_ENABLE )) 
	{
		rts_rate = OP_11_MBPS;
	}
	else
	//normal setting	
	{
		if (pAdap->wlan_attr.rate >= OP_24_MBPS)
		{
			rts_rate = OP_24_MBPS;
		}
		else if (pAdap->wlan_attr.rate >= OP_12_MBPS)
		{
			rts_rate = OP_12_MBPS;
		}
		else if ((pAdap->wlan_attr.rate == OP_6_MBPS) ||
			(pAdap->wlan_attr.rate == OP_9_MBPS))
		{
			rts_rate = OP_6_MBPS;
		}
		//default
		else
		{
			rts_rate = OP_1_MBPS;
			if (pAdap->wlan_attr.rate == OP_11_MBPS)
			{
				rts_rate = OP_11_MBPS;
			}
			else if ((pAdap->wlan_attr.rate == OP_5_MBPS) ||
				(pAdap->wlan_attr.rate == OP_5_MBPS))
			{
				//have bug for 5.5 rts.
				rts_rate = OP_2_MBPS;
			}
		}
	}//end normal
	return rts_rate;
}
/*
	The routine caculate the highest rate for recent 20 tx pacekt.
	the rate will be used for OS query speed.
	This will make speed show look smooth
*/
VOID Rate_Calc_Smooth(PDSP_ADAPTER_T pAdap,UINT8 txrate)
{
	UINT16 index;
	PDSP_ATTR_T pt_mattr = &pAdap->wlan_attr;

	if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)	//Justin: 081118.	Adjust IBSS data rate
		&&(pt_mattr->is_exist_ibss_b_sta))
	{
		if(pAdap->wlan_attr.last_rateshowed > OP_11_MBPS)
		{
			pAdap->wlan_attr.last_rateshowed = OP_11_MBPS;
			pAdap->wlan_attr.rateshowed = OP_11_MBPS;

			return;
		}
	}
	
	if (pAdap->wlan_attr.rateshowed_count == 0)
	{
		pAdap->wlan_attr.rateshowed = txrate;
		pAdap->wlan_attr.rateshowed_count++;
	}
	else
	{
		if (pAdap->wlan_attr.rateshowed < txrate)
		{
			pAdap->wlan_attr.rateshowed = txrate;
		}
		
		pAdap->wlan_attr.rateshowed_count++;

		//make fast adjust to up rate
		if(pAdap->wlan_attr.rateshowed_count == _3DSP_AVERAGE_RATE_FAST_COUNT)
		{
			if(pAdap->wlan_attr.rateshowed >= pAdap->wlan_attr.last_rateshowed)
			{
				pAdap->wlan_attr.rateshowed_count = 0;
				pAdap->wlan_attr.last_rateshowed = pAdap->wlan_attr.rateshowed;
			}
		}
		else if (pAdap->wlan_attr.rateshowed_count >= _3DSP_AVERAGE_RATE_COUNT)
		{
			pAdap->wlan_attr.rateshowed_count = 0;
			if(pAdap->wlan_attr.rateshowed >= pAdap->wlan_attr.last_rateshowed)
			{
				pAdap->wlan_attr.last_rateshowed = pAdap->wlan_attr.rateshowed;
			}
			else
			{
				index = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.rateshowed );
				if(index != 0)
				{
					index--;
				}
				pAdap->wlan_attr.last_rateshowed = Adap_get_rate_from_index(pAdap,index);
			}
		}
	}
}

static const UINT8 support_rate_list[12] = {
	OP_1_MBPS,
	OP_2_MBPS,
	OP_5_MBPS,
	OP_11_MBPS,
	OP_6_MBPS,
	OP_9_MBPS,
	OP_12_MBPS,
	OP_18_MBPS,
	OP_24_MBPS,
	OP_36_MBPS,
	OP_48_MBPS,
	OP_54_MBPS
};

BOOLEAN		Rate_IsSupportRate(UINT8 rate)
{
	UINT32	i;

	for(i=0;i<12;i++)
	{
		if(rate == support_rate_list[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}

VOID Adap_SetRateIndex(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
	UINT8  rate = pAdap->wlan_attr.rate & 0x7f;
	UINT8 i = 0;
	UINT8 j = 6;
	
	ASSERT(pAdap);

	pt_mng->rateIndex = 0x00;
	
	if (rate >= support_rate_list[j])
		i = j;

	while(j > 0)
	{
		j--;
		if (rate == support_rate_list[i + j])
		{
			pt_mng->rateIndex = i + j;
			return;
		}
	}	
}

UINT8 Adap_get_rate_from_index(PDSP_ADAPTER_T pAdap, UINT16 rateIndex)
{
	if (rateIndex < 12)
		return support_rate_list[rateIndex];

	DBG_WLAN__MAIN(LEVEL_TRACE,"find rate error, index=%d.\n", rateIndex);
	return support_rate_list[0];
}


UINT8  Adap_get_rateindex_from_rate(PDSP_ADAPTER_T pAdap, UINT8 rate)
{
	UINT8 i = 0;
	UINT8 j = 6;
	
	rate &= 0x7f;
	if (rate >= support_rate_list[j])
		i = j;

	while(j > 0)
	{
		j--;
		if (rate == support_rate_list[i + j])
			return i + j;
	}
	
	ASSERT(0);
	DBG_WLAN__MAIN(LEVEL_TRACE,"find error from rate to index\n");
	return (pAdap->wlan_attr.macmode == IEEE802_11_A) ? 4 : 3;
}

VOID Adap_set_support_rate(PDSP_ADAPTER_T pAdap, 
			dot11_type_t dot11_type, 
			PUINT8 P_rate_list, 
			PUINT8 p_rate_len)
{
//	PDSP_ATTR_T  pattr = (PDSP_ATTR_T)&pAdap->wlan_attr;
	
	if (dot11_type == IEEE802_11_G)
	{
		*p_rate_len = 12;
		sc_memory_copy(P_rate_list, (PVOID)&support_rate_list[0], 12);
	}
	else if (dot11_type == IEEE802_11_A)
	{
		*p_rate_len = 8;
		sc_memory_copy(P_rate_list, (PVOID)&support_rate_list[4], 8);
	}
	else		//IEEE802_11_B
	{
		*p_rate_len = 4;
		sc_memory_copy(P_rate_list, (PVOID)&support_rate_list[0], 4);
	}

	//set real tx rate
	Rate_set_original_rate(pAdap,pAdap->wlan_attr.used_basic_rate);		
}


