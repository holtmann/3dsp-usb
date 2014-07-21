/****************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  *
  * FILENAME:     DSP_mng.c     CURRENT VERSION: 1.00.01
  * PURPOSE:      this module implement function of management of 
  *               between 802.11 spec and 802.11i, include scan, join
  *               authention, association, Bss, Ibss. 
  *
  *
  * DECLARATION:  This document contains confidential proprietary information that
  *               is solely for authorized personnel. It is not to be disclosed to
  *               any unauthorized person without prior written consent of 3DSP
  *               Corporation.         
  ***************************************************************/

#include "precomp.h"	
#include "usbwlan_mng.h"


static char* TDSP_FILE_INDICATOR="MNAGE";

/*
#include <ndis.h>
#include "usbwlan_Sw.h"
#include "usbwlan_pr.h"
#include "usbwlan_proto.h"
#include "usbwlan_frag.h"
#include "usbwlan_dbg.h"
#include "usbwlan_mng.h"
#include "usbwlan_tx.h"
#include "usbwlan_rx.h"
#include "usbwlan_vendor.h"
#include "usbwlan_defs.h"
*/
/*--file local constants and types-------------------------------------*/
	 
/*--file local macros--------------------------------------------------*/

#define DSP_TX_HEAD_LENGTH  \
	(sizeof(USBWLAN_HW_FRMHDR_TX))
#define DSP_RX_USED_HEAD_LENGTH  		24
#define DSP_RX_TOTAL_HEAD_LENGTH 		28

#define WLAN_NOFIXED_ELE_HEAD_LENGTH    (sizeof(MNG_NOFIXED_ELE_ALL_T) - 1)

#define WLAN_FRAME_NOFIXED_ELE_LENGTH(pele) \
	(WLAN_NOFIXED_ELE_HEAD_LENGTH + pele->length)
	
#define WLAN_MOVE_TO_NEXT_ELE(pele) \
	(pele = (PMNG_NOFIXED_ELE_ALL_T)(pele->data + pele->length))

#define WLAN_CONVERT_FRAME_NOFIXED_ELE(address) \
	((PMNG_NOFIXED_ELE_ALL_T)(address))
	
#define WLAN_BEACON_FIRST_NOFIXED_ELE(start_address) \
	((PMNG_NOFIXED_ELE_ALL_T)((PUINT8)start_address + DSP_RX_USED_HEAD_LENGTH + sizeof(BEACON_FIXED_FIELD_T)))

#define WLAN_AUTHFRAME_CHALLENGE_ELE(start_address) \
	((PMNG_NOFIXED_ELE_ALL_T)((PUINT8)start_address + DSP_RX_USED_HEAD_LENGTH + sizeof(MNG_AUTHFRAME_DATA_T)))

//WLAN_HDR_A3_DATAP
//DSP_TX_HEAD_LENGTH
#define WLAN_LOCATE_3DSP_TX_PAYLOAD(start_address) \
	((PUINT8)start_address + DSP_TX_HEAD_LENGTH)

#define WLAN_LOCATE_3DSP_RX_PAYLOAD(start_address) \
	((PUINT8)start_address + DSP_RX_USED_HEAD_LENGTH)


/*--file local variables-----------------------------------------------*/
	 
/*--file local function prototypes-------------------------------------*/

VOID Mng_EndScan(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Mng_MakeProbeReq(PDSP_ADAPTER_T pAdap);

TDSP_STATUS Mng_Transact_Probersp(PDSP_ADAPTER_T pAdap);
	
VOID Mng_Rx_WhenScan(PDSP_ADAPTER_T pAdap);
VOID Mng_Rx_WhenJoin(PDSP_ADAPTER_T pAdap,UINT8 rssi);
VOID Mng_Rx_WhenJoinOk(PDSP_ADAPTER_T pAdap);
VOID Mng_Rx_WhenAuthOk(PDSP_ADAPTER_T pAdap);
VOID Mng_Rx_WhenAssocOk(PDSP_ADAPTER_T pAdap);
VOID Mng_Rx_WhenOidScan(PDSP_ADAPTER_T pAdap, UINT8 rssi);

/*******************************************************************
 *   Mng_SendPacket
 *   
 *   Descriptions:
 *      send managment frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      source: , start address of managment frame
 *      len: , length of management frame
 *   Return Value:
 *      none
 ******************************************************************/
static inline TDSP_STATUS Mng_SendPacket(PUINT8 source,UINT16 len,PDSP_ADAPTER_T pAdap)
{
	Tx_Transmit_mng_frame(pAdap,source,len,TX_MNG_FRM_PRIORITY_NORMAL,TX_MNG_TYPE_NORMAL);
	
	return STATUS_SUCCESS;
}

/*******************************************************************
 *   Mng_SendNonMngFrame
 *   
 *   Descriptions:
 *      execute the routine to send null or ps poll packet
 *    
 * 	Jakio2006.11.29: changed func's name, null data frame also uses this entry
 *	Jakio2006.11.28: send ps-poll frame, ps-poll frame is the third class frame, is sent after associated ,
 *	so we should check the status
 *
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      source:
 *      type:
 *      len:
 *
 *   Return Value:
 *      none
 ******************************************************************/
static inline TDSP_STATUS  Mng_SendNonMngFrame(PUINT8 source, TX_MNG_FRM_TYPE  type, UINT16 len,PDSP_ADAPTER_T pAdap)
{
	Tx_Transmit_mng_frame(pAdap,source,len,TX_MNG_FRM_PRIORITY_NORMAL,type);
	
	return STATUS_SUCCESS;
}


static VOID	Mng_DeleteUnsupportedRate(PDSP_ADAPTER_T pAdap, PMNG_DES_T pcurdes)
{
	UINT32		i,j,k;

//	DBG_PRINT_BUFF("Suprate before:",&pcurdes->suprate[1],pcurdes->suprate[0]);
//	DBG_PRINT_BUFF("Extrate before:",&pcurdes->ExtendedSupportRate,pcurdes->ExtendedSupportRateLen);


	for(i=0,j=0;i<pcurdes->suprate[0];i++)
	{
		pcurdes->suprate[j+1] = pcurdes->suprate[i+1];

		if(Rate_IsSupportRate(0x7f & pcurdes->suprate[i+1]))
		{
			j++;
		}
	}

	pcurdes->suprate[0] = j;

	for(i=0,j=0;i<pcurdes->ExtendedSupportRateLen;i++)
	{
		pcurdes->ExtendedSupportRate[j] = pcurdes->ExtendedSupportRate[i];

		for(k=0;k<pcurdes->suprate[0] ;k++)
		{
			if((0x7f & pcurdes->ExtendedSupportRate[i]) == (0x7f &pcurdes->suprate[k+1] ))
			{
				break;
			}
		}

		if(	Rate_IsSupportRate(0x7f & pcurdes->ExtendedSupportRate[i])
		&&	k == pcurdes->suprate[0])
		{
			j++;
		}
	}
	
	pcurdes->ExtendedSupportRateLen = j;
	
//	DBG_PRINT_BUFF("Suprate after:",&pcurdes->suprate[1],pcurdes->suprate[0]);
//	DBG_PRINT_BUFF("Extrate after:",&pcurdes->ExtendedSupportRate,pcurdes->ExtendedSupportRateLen);

	return;
}

/*
 * Name						: mlme_gen_rand_number()
 * arguments				: uint8* out_rand (allocted bytes stream)
 * 							: uint8 size
 * return value				: void
 * global variables 		: none
 * description				: 
 * special considerations 	: None
 * see also					: 
 * TODO						:
 */
static inline VOID
Mng_mlme_gen_rand_number(
	PDSP_ADAPTER_T pAdap,
	UINT32 size, 
	UINT8 *out_rand)
{
	UINT8 i = 0;

	while(i<size)
	{
		out_rand[i++] = (UINT8)(sc_get_random_byte());
	}
}

static inline VOID
Mng_build_ibss_bssid(
	PDSP_ADAPTER_T pAdap,
	PUINT8 bssid)
{
	Mng_mlme_gen_rand_number(pAdap,WLAN_BSSID_LEN,bssid);
	bssid[0] &= ~((BIT_0));
	bssid[0] |= BIT_1;

}
/**************************************************************************
 *   Mng_Init
 *
 *   Descriptions:
 *      Init management module. including allocate Mng_Str structure used 
 *      for management process.
 *   Arguments:
 *      MiniportAdapterContext: , pointer to the adpater context.
 *   Return Value:
 *      STATUS_SUCCESS if successfully 
 *      TDSP_STATUS_xxx if unsucessfully
 *************************************************************************/

TDSP_STATUS
Mng_Init(PDSP_ADAPTER_T 		pAdap)
{	
	PMNG_STRU_T		pmng;
	//TDSP_STATUS		 	Status;

	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Init\n");

	kd_assert(pAdap->pmanagment == NULL);

	/* Alloc memory for Mng structure */
	pmng = sc_memory_alloc(sizeof(MNG_STRU_T));
	if (pmng == NULL)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"MNG MODULE Allocate Memory failed.\n");
        	return STATUS_FAILURE;
	}
	
	sc_memory_set(pmng, 0, sizeof(MNG_STRU_T));

	pmng->statuscontrol.curstatus = IDLE;
	pmng->rateIndex = 0; //Jakio add here, init rateIndex
	pAdap->pmanagment = (PVOID)pmng;
//Justin: 0717. prepare wep keys for reload defaults	  // Maybe a better way is to import these from registry
    sc_memory_copy(&(pmng->default_wep_keys[0][0]), REGISTRY_DEFAULT_WEP_KEY0, SME_MAX_KEY_LEN_IN_HW);
    sc_memory_copy(&(pmng->default_wep_keys[1][0]), REGISTRY_DEFAULT_WEP_KEY1, SME_MAX_KEY_LEN_IN_HW);
    sc_memory_copy(&(pmng->default_wep_keys[2][0]), REGISTRY_DEFAULT_WEP_KEY2, SME_MAX_KEY_LEN_IN_HW);
    sc_memory_copy(&(pmng->default_wep_keys[3][0]), REGISTRY_DEFAULT_WEP_KEY3, SME_MAX_KEY_LEN_IN_HW);
	
	return (STATUS_SUCCESS);
}


/**************************************************************************
 *   Mng_Realease
 *
 *   Descriptions:
 *      Realease management module. Realease space of Mng_Str structure 
 *      allocated when init.
 *   Arguments:
 *      MiniportAdapterContext: , pointer to the adpater context.
 *   Return Value:
 *      STATUS_SUCCESS if successfully 
 *      TDSP_STATUS_xxx if unsucessfully
 *************************************************************************/
TDSP_STATUS Mng_Release(PDSP_ADAPTER_T 		pAdap)
{
//	PMNG_STRU_T	pmng;

	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Release\n");
	/* Free the memory of mng module */
	if(pAdap->pmanagment != NULL)
	{
		sc_memory_free(pAdap->pmanagment);
		pAdap->pmanagment=NULL;
	}
	
	DBG_EXIT();
	return (STATUS_SUCCESS);
}


/**************************************************************************
 *	Jakio200611.28: add "type" parameter to support control frame
 *    wlan_build_mng_frmctrl
 *	Descriptions:
 *    		The routine build frame control field for management frame
 *   	Arguments:
 *   	       sub_type: , sub type of management frame
 *   	       to_do: , indicate to ds set or not
 *   	       privacy: , indicate if privacy is need ( management frame never encrypted)
 *   	       frm_ctl: , frame control field of management frame returned by the function
 *   	Return Value:
 *    		VOID
 *************************************************************************/
static inline VOID  wlan_build_mng_frmctrl(PDSP_ADAPTER_T pAdap, UINT8 type, UINT8 sub_type, 
			UINT8 to_ds, UINT8 privacy, UINT16 *frm_ctrl)
{

	ASSERT(pAdap);

	*frm_ctrl = 0x00;

	//Type&SubType
	*frm_ctrl = (UINT16)( WLAN_SET_FC_FTYPE(type) |  
				WLAN_SET_FC_FSTYPE(sub_type));
	//Wep_use?
	if (privacy)
		*frm_ctrl |= (UINT16)WLAN_SET_FC_ISWEP(1);
	
	//To Ds
	if(to_ds)
		*frm_ctrl |= (UINT16)(WLAN_SET_FC_TODS(1));

	//Power Save
	//if((pAdap->wlan_attr.ps_state == PS_POWERSAVE_HIGH)&&(pAdap->wlan_attr->hasjoined == JOIN_HASJOINED))
	if((pAdap->wlan_attr.gdevice_info.ps_support!= PSS_ACTIVE) && 
		(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED))
	{
		*frm_ctrl |=(UINT16)WLAN_SET_FC_PWRMGT(1);
	}	

	//more frag
	//if(more_flag)
		//*frm_ctrl |= (UINT16)(WLAN_SET_FC_MOREFRAG(1));

	
}




/**************************************************************************
 *    Mng_fill_hw_header
 *	Descriptions:
 *    		The routine fill some common fields of hardware header of mng&control frame
 *   	Arguments:
 *   	       pAdap
 *		header
 *		to_ds
 *		type
 *		sub_type
 *		payload_len
 *		privacy
 *   	Return Value:
 *    		VOID
 *************************************************************************/
static void Mng_fill_hw_header(PDSP_ADAPTER_T pAdap,PUSBWLAN_HW_FRMHDR_TX header, 
				UINT8 to_ds, UINT8 type, UINT8 sub_type, UINT8 payload_len, UINT8 privacy)
{
	UINT16  tx_len;
	UINT16 frm_ctrl;
	UINT8   txpwr_lvl;
	UINT8   data_rate; //Jakio20061205:fix an error when build broadcast frame

	//Jakio2006.12.15: add here, 'cause tx_get_speed_and_duration()'s change
	HW_HDR_PARSE_CONTEXT	context;
	HW_HDR_PARSE_RESULT	result;
	
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = (PDSP_ATTR_T)&pAdap->wlan_attr;
	
	ASSERT(pAdap);
	ASSERT(header);

	sc_memory_set(header, 0, sizeof(USBWLAN_HW_FRMHDR_TX));
	sc_memory_set(&result, 0, sizeof(HW_HDR_PARSE_RESULT));


	
	//txpwr_lvl = pAdap->txpwr_level;
	txpwr_lvl = pAdap->wlan_attr.tx_power;

	//fill parse_context members
	context.more_flag = 0;
	context.next_frag_len = 0;
	context.payload_len = payload_len;
	context.privacy = privacy;
	context.sub_type = sub_type;
	context.type = type;
	//calculate some fields
	//Jakio2006.12.15: changed here
	tx_get_speed_and_duartion(pAdap,0, &context, &result,TRUE);

	//Jakio20070419: tx_length should be this, include encryption if have
	tx_len = result.air_len;

	//this situation is special, packet type is broadcast,
	//just probe request and beacon frame into this case
	if(	type == WLAN_FTYPE_MGMT 
	&&	(	(	sub_type == WLAN_FSTYPE_PROBEREQ
			&&	!(pt_mng->statuscontrol.worktype & SCAN_STATE_SSID_FLAG)) 
		||	sub_type == WLAN_FSTYPE_BEACON))
	{
		result.duration = 0;
		//when sending broadcast mng packet
		//802.11a mode, short preamble + 4 speed
		//802.11b/g mode, long preamble + 1 speed
		result.tx_speed = (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A) ? 4 : 0;
		result.short_preamble = (pt_mattr->gdevice_info.preamble_type == SHORT_PRE_AMBLE) ? 1:0;//(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A) ? 1:0 ;

		data_rate = Adap_get_rate_from_index(pAdap,result.tx_speed);
		result.frag_dur = 2 * pt_mattr->gdevice_info.sifs + 
			_durn(pAdap, result.short_preamble, WLAN_CTS_LEN, data_rate&0x7f) +
			_durn(pAdap, result.short_preamble, result.air_len, data_rate&0x7f);
	}
	wlan_build_mng_frmctrl(pAdap, type, sub_type,to_ds, privacy, &frm_ctrl);//justin: 1025. some mng frm like auth3 should be set privacy
	
	//begin fill the hw frame header
	 header->len_speed_ctrl |=  (result.tx_speed << _3DSP_HWHDR_POS_TX_SPEED) | 
        		(tx_len << _3DSP_HWHDR_POS_TX_LENGTH) |
        		_3DSP_HWHDR_BIT_INT_ON_TX|//_3DSP_HWHDR_BIT_WR_ACK|	// Justin: discard ack
        		(result.short_preamble? _3DSP_HWHDR_BIT_SHORT_PREAMBLE:0) |
        		(privacy?_3DSP_HWHDR_BIT_PRIVACY:0);
	//justin: 1030//some mng frm like auth3 should be set privacy
	if(privacy)
	{
		header->len_speed_ctrl |= ((pAdap->wlan_attr.default_key_index & 0x03) << _3DSP_HWHDR_POS_DEFAULT_KEY_ID);
	}
	
	//Jakio20070406: for broadcast packet, ACK is unnecessary.
	//As a wlan station, only proberequest is broadcast type
	//in ibss, probe response not needed for ack
	if(sub_type == WLAN_FSTYPE_PROBEREQ ||sub_type == WLAN_FSTYPE_PROBERESP)
	{
		header->len_speed_ctrl  &= ~_3DSP_HWHDR_BIT_DONT_EXPECT_ACK;
		header->len_speed_ctrl |=   _3DSP_HWHDR_BIT_DONT_EXPECT_ACK;
	}
	//if(privacy)
	//	header->len_speed_ctrl |= ((pAdap->wlan_attr.wep_mode & 0x03) << _3DSP_HWHDR_POS_DEFAULT_KEY_ID);
	
	//else
	//	header->len_speed_ctrl |= (WEP_MODE_NONE<< _3DSP_HWHDR_POS_DEFAULT_KEY_ID);

	//Jakio2006.11.29: for ps-poll frame,no retry_enable bit to set
	//woody for ibss, probe response not retry
	//if(sub_type != WLAN_FSTYPE_PSPOLL)
	if((sub_type != WLAN_FSTYPE_PSPOLL) && (sub_type != WLAN_FSTYPE_PROBERESP))
	{
		header->len_speed_ctrl |= _3DSP_HWHDR_BIT_RETRY_ENABLE;
	}	

	
	//header->len_speed_ctrl &=(~((UINT32)_3DSP_HWHDR_BIT_TX_CTS_SELF)); 	
   	if(result.cts_self)
  		 header->len_speed_ctrl |=(UINT32)_3DSP_HWHDR_BIT_TX_CTS_SELF; 
	//Tx power level
	header->len_speed_ctrl |= (txpwr_lvl & 0x03) << _3DSP_HWHDR_POS_TX_PWR_LVL;

	//Jakio2006.11.28: ps-poll frame has no frag_dur, filling aid instead
	if(sub_type == WLAN_FSTYPE_PSPOLL)
		header->frmctrl_fragdur = (frm_ctrl << _3DSP_HWHDR_POS_FRM_CTRL) | (pt_mattr->aid) |BIT14 | BIT15;
	else 
	 	header->frmctrl_fragdur |= result.frag_dur | (frm_ctrl << _3DSP_HWHDR_POS_FRM_CTRL);

	//duration
	if(sub_type != WLAN_FSTYPE_PSPOLL)//justin: 1009.	for ps-poll, duration is fill with aid
		header->addr1lo_txdurid |= result.duration;		
	else
		header->addr1lo_txdurid |= pAdap->wlan_attr.aid;
	//Not setting seq_num here, set seq_num when submit irp
	//header->seqctrl_addr3hi |= seq_num << _3DSP_HWHDR_POS_SEQ_CTRL;

	//fill the addr, should distinguish mngt and data frame
}


/*******************************************************************
 *   Mng_DoScan
 *   
 *   Descriptions:
 *      begin scan a channel or end current scan
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
  *   Return Value:
 *      none
 ******************************************************************/
 //#define	Mng_DoScan(pAdap)		_Mng_DoScan(pAdap,__FUNCTION__,__LINE__)
 
 static VOID Mng_DoScan(PDSP_ADAPTER_T pAdap)//,char* func, UINT32 line)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   		pt_mattr = &pAdap->wlan_attr;
	UINT32            		channe_list_index;
	//UINT8					tmp;
	//UINT32 				scan_status =pt_mng->statuscontrol.worktype;
	UINT8 				chan;
	UINT32				delay_timer;
	
	dot11_type_t			dot11_type;
	//UINT8	broadcast[] = {0xff,0xff,0xff,0xff,0xff,0xff};

	//DBG_ENTER();
	
	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"do scan return while halt\n");
		return;
	}

	//wumin: 090226 scan has been cancelled
#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status != NEED_RECONNECT)
#endif
	{
		if(!pAdap->is_oid_scan_begin_flag)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Scan has been cancelled.\n");
			pt_mng->oidscanflag = FALSE;

			if(pt_mng->statuscontrol.curstatus == SCANNING)
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Change mng status from SCANNING to IDLE.\n");
				pt_mng->statuscontrol.curstatus = IDLE;
			}

			if(pAdap->scanning_flag )
			{
				pAdap->scanning_flag = FALSE;
			}
			
			return;
		}
	}
	
	channe_list_index = pt_mng->statuscontrol.workcount;
	dot11_type = (dot11_type_t)pt_mng->statuscontrol.workflag;
	DBG_WLAN__MLME( LEVEL_INFO,"Mng_DoScan, channe_list_index= %X \n",channe_list_index);

	// end scan		//justin
	//11g channel have been scanned.
	//woody fixed the value for debug 
	//Jakio20070621: change  it for full scan
	//pt_mattr->channel_len_of_g = 1;
//	if((channe_list_index >= pt_mattr->channel_len_of_g)
//		&& (dot11_type == IEEE802_11_G))	// 11g end scan
//	{	
//		
//
//		//return due to 11a scan is not requested
//		if(!(scan_status & SCAN_STATE_11A_FLAG))
//		{
//			Mng_EndScan(pAdap);
//			return;
//		}
//		// continue to scan 11a channels
//		pt_mng->statuscontrol.workflag = IEEE802_11_A;
//		pt_mng->statuscontrol.workcount = pt_mattr->channel_len_of_g;      /* init channel num. 11a channel follow 11g */
//		Mng_StartScan_A_OR_G(pt_mng->statuscontrol.worktype,pAdap);	
//		return;
//	}
//	//else if((channe_list_index >= pt_mattr->channel_num)
//	//	&&(dot11_type == IEEE802_11_A))	// 11a end scan
//	else 
	if(channe_list_index >= pt_mattr->channel_len_of_g)
	{
		DBG_WLAN__MLME( LEVEL_INFO,"End Scan. \n");
		Mng_EndScan(pAdap);
		return;
	}


	//here, channel_list_index < channel_len
	//scan should be going on
	pt_mng->curchannel = pt_mattr->channel_list[channe_list_index];
	DBG_WLAN__MLME(LEVEL_TRACE, "Scan %x channel , with antenna = %d \n", pt_mng->curchannel, pAdap->wlan_attr.antenna_num);	

	//1:set channel
	//get current channel
	chan = Adap_get_channel(pAdap);
	if(chan != pt_mng->curchannel)
	{	
	    //Joe added for wait txcount == 0. 2009 - 12 - 28
		if(TRUE)
		{
			//Wait untill tx count = 0;
			ULONG val;
			ULONG readCount=3;
			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
			val &= 0xff;
			while(val != 0)
			{
				DBG_WLAN__MLME(LEVEL_TRACE," Mng do scan wait for txcount = 0; COUNT = %x, times = %d!\n",val, readCount );
				//wait 15 times
				if(readCount == 0)
				{
					break;
				}
				readCount--;
				sc_sleep(15);	//Estimated value.		
				val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
				val &= 0xff;
			}
			
			if(readCount == 0)
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Mng do scan ,maybe tx hang happen,COUNT = %x, times = %d!\n",val, readCount );
			}
			else
			{
				DBG_WLAN__MLME(LEVEL_INFO,"Mng do scan ,continue COUNT = %x, times = %d!\n",val, readCount );
			}
		}
		//end

	   //add by jacqueline for can not set channel success while unplug-combo 
		if( (UINT32)DEV_IDLE != Adap_get_current_state_control(pAdap))
		{
			//DBG_WLAN__MLME(LEVEL_TRACE,"Adap_get_current_state_control_111 != idle\n");
            if(pt_mng->statuscontrol.retrynum >= 3)
            {
                Adap_set_state_control(pAdap,(UINT8)DEV_IDLE, 0, 0);
			
            }
		}
        
		//set new channel
		Adap_set_channel(pAdap,pt_mng->curchannel);
		sc_sleep(30);
		//
		if( (UINT32)DEV_IDLE != Adap_get_current_state_control(pAdap))
		{
			DBG_WLAN__MLME(LEVEL_INFO,"Adap_get_current_state_control != idle\n");
        
		}
		else
		{
			Adap_set_state_control(pAdap,(UINT8)DEV_ACTIVE, 0, 0);
			DBG_WLAN__MLME(LEVEL_TRACE,"Adap_get_current_state_control == idle\n");
		}
		//delay
		sc_sleep(30);
	}

	//verify channel again
	chan = Adap_get_channel(pAdap);

	if(chan != pt_mng->curchannel)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"diff channel\n");
		pt_mng->statuscontrol.worktype |= SCAN_STATE_WAIT_STATUS;
		Mng_SetTimerPro(CONFIRM_CHANNEL_TIMEOUT, pAdap);   /* set maxtimer scan */
		//fail in set channel so wait an interval to continue setting channel.
		pt_mng->statuscontrol.retrynum++;
		return;
	}
	//woody debug
	else if(pt_mng->statuscontrol.worktype & SCAN_STATE_WAIT_STATUS)
	{
		pt_mng->statuscontrol.retrynum=0;
	}
	
#ifdef ANTENNA_DIVERSITY
	if(STATUS_SUCCESS != Adap_set_antenna(pAdap, (UINT8)pAdap->wlan_attr.antenna_num))
	{
        	DBG_WLAN__MLME(LEVEL_ERR,"Set antenna fail\n");
	}
#endif	

	//WOODY add for restore acite state after swtich channel
	Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);

	//2:set scan para
	//set for init scan para
	pt_mng->scan_stage = ACTIVE_SCAN_SECOND_STAGE;
	pt_mng->haveSignal = 0;
	delay_timer = pt_mattr->max_channel_time;
	//Jakio20070521: print mac register
	//DBG_WLAN__MLME(("Print Mac register when doing scan, just before send probereq out\n"));
	//Adap_Print_MacReg(pAdap);
	//set for active scan
	if(pt_mng->statuscontrol.worktype & SCAN_STATE_ACTIVE_FLAG)
	{
		pt_mng->scan_stage = ACTIVE_SCAN_FIRST_STAGE;
		delay_timer = pt_mattr->min_channel_time;
		if(STATUS_SUCCESS != Mng_MakeProbeReq(pAdap))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Send probe success for scan\n");
			//add channel for next scan
			pt_mng->statuscontrol.worktype |= SCAN_STATE_WAIT_STATUS;
			Mng_SetTimerPro(MNG_WAIT_HW_IDLE_TIMEOUT, pAdap);
			//plus 1 to the variable to indicate one retry be done.
			//we will be into failure flow if retry num more than max 
			pt_mng->statuscontrol.retrynum++;
			return;
		}
		//Jakio20070621: when active scan, set min delay timer in first stage
		Mng_SetTimerPro(delay_timer, pAdap);  
#ifdef ANTENNA_DIVERSITY	
		if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
		{
			if (pAdap->wlan_attr.antenna_num == DSP_ANTENNA_TYPE_MAX)
			{
				pt_mng->statuscontrol.workcount++;
			}
		}
		else
		{
		pt_mng->statuscontrol.workcount++;
		}
#else
		pt_mng->statuscontrol.workcount++;
#endif
		return;
	}


	//3: set scan timer
	//set timer for current scan
	Mng_SetTimerPro(delay_timer, pAdap);  
	
#ifdef ANTENNA_DIVERSITY	
	//swtich for next channel
	if (pAdap->wlan_attr.antenna_num == DSP_ANTENNA_TYPE_MAX)
	{
	pt_mng->statuscontrol.workcount++;
	}
#else
	pt_mng->statuscontrol.workcount++;
#endif

	//clear retry count for confirm channel flow			//justin:	080514.	reset retrynum for next channel scan
	pt_mng->statuscontrol.retrynum = 0; 


	return ;
}



/**************************************************************************
 *   Mng_StartScan
 *
 *   Descriptions:
 *      The function is called when driver reset and begin a new link 
 *      process. The function only be called on win2000.
 *   Arguments:
 *      pAdap: , pointer to adapter context
 *      status: ,  current scan status
 *           OID_SCAN -- scan all of channels with broadcast ssid
 *           SET_NORMAL_SCAN -- scan all of channels with ssid
 *           SET_SSID_SCAN -- scan channels with ssid ,stopping
 *                scan action while find corresponding AP.
 *   Return Value:
 *      none
 *************************************************************************/
static VOID Mng_StartScan(UINT32 status,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"DRIVER HALT.\n");
		return;
	}
	
	//DBG_ENTER();
	//woody set the two variable when scan begin running
	pAdap->scanning_flag = TRUE;
	pt_mng->statuscontrol.workcount = 0;     	// init channel index num 
	
	if(status & SCAN_STATE_11G_FLAG)
	{
		pt_mng->statuscontrol.workflag = (UINT32)IEEE802_11_G;      //scan with phy mode
		//status &=(~((UINT32)SCAN_STATE_11G_FLAG));
	}
//	else if(status & SCAN_STATE_11A_FLAG)	
//	{
//		pt_mng->statuscontrol.workflag = (UINT32)IEEE802_11_A;      //scan with phy mode
//		//status &=(~((UINT32)SCAN_STATE_11A_FLAG));
//	}
	else
	{	
		DBG_WLAN__MLME(LEVEL_TRACE,"startscan fail due to no valid phy mode \n");
		return;
	}
	
#ifdef ANTENNA_DIVERSITY
	if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
	{
		pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;
	}	
#endif			
//	Mng_StartScan_A_OR_G(status,pAdap);


	if(status & SCAN_STATE_OID_FLAG)
	{
		pt_mng->oidscanflag = TRUE;
//		DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = TRUE;\n");
	}
	else
	{
		pt_mng->statuscontrol.curstatus = SCANNING;
	}	
	
	//pt_mng->bssdeslist.bssdesnum = 0;
	//pt_mng->statuscontrol.retrynum = 0;
	pt_mng->statuscontrol.worktype = status;   

	//set scan api, setting scan enviourment
	Adap_set_scan_reg(pAdap);
	


	//begin scan a channel
	Mng_DoScan(pAdap);

}



/*******************************************************************
 *   Mng_MakeProbeReq
 *   
 *   Descriptions:
 *      build up probe request frame for active scan
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
  *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeProbeReq(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T       		pt_mattr = &pAdap->wlan_attr;
	UINT8				broadcastaddr[]={0xff,0xff,0xff,0xff,0xff,0xff};
//	UINT16				offset;
	PUINT8                       addr1 = NULL;
	PUINT8				addr3 = NULL;
	UINT8				sup_len;
	UINT8 				sup_rates[12];
	PMNG_NOFIXED_ELE_ALL_T		pele;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;
	UINT8 				i;

	ASSERT(pAdap);
	
	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"DRIVER HALT.\n");
		return STATUS_FAILURE;
	}
	
	
	/*hw header pos*/
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)pt_mng->tranbuf;
	//offset = sizeof(USBWLAN_HW_FRMHDR_TX);

	/*body*/
	/*ssid*/
	pele = WLAN_CONVERT_FRAME_NOFIXED_ELE(
		pt_mng->tranbuf + sizeof(USBWLAN_HW_FRMHDR_TX));
	pele->element = SSID_EID;
	//set ssid element 
	pele->length = 0;
	if(pt_mng->statuscontrol.worktype & SCAN_STATE_SSID_FLAG)
	{
		pele->length = pt_mattr->ssid_len;
		//sc_memory_copy((PUINT8)pele + sizeof(MNG_NOFIXED_ELE_HEAD_T), pt_mattr->ssid, pt_mattr->ssid_len);
		sc_memory_copy(pele->data,pt_mattr->ssid, pt_mattr->ssid_len);
	}
	WLAN_MOVE_TO_NEXT_ELE(pele);
	//offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pele);

	/* supported rates */
	//pele = (PMNG_NOFIXED_ELE_ALL_T)(pt_mng->tranbuf + offset);
	pele->element = SUPRATE_EID;
	Adap_set_support_rate(
		pAdap,
		pt_mng->statuscontrol.workflag,		//alwaye save phy mode for scan or oid scan
		sup_rates,
		&sup_len);

	if(sup_len > 12)
	{
    		DBG_WLAN__MLME(LEVEL_TRACE,"!! Length of rate list too long \n");
	}
	//11a: put all rate into support element, (rate number is 8)
	//11b & 11g: put 11b rate into this element, (number is 4)
	pele->length = (pt_mng->statuscontrol.workflag == IEEE802_11_A) ? sup_len : 4;
	sc_memory_copy(pele->data,sup_rates,pele->length);
	WLAN_MOVE_TO_NEXT_ELE(pele);
	//offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pele);

	// extend supported rates for 11g
	if(pt_mng->statuscontrol.workflag == IEEE802_11_G)
	{
		//pele = (PMNG_NOFIXED_ELE_ALL_T)(pt_mng->tranbuf + offset);
		//WLAN_MOVE_TO_NEXT_ELE(pele);
		pele->element = EXTENDRATE_EID;
		pele->length = sup_len - 4;

		if(pele->length >8 )
		{
	    		DBG_WLAN__MLME(LEVEL_TRACE,"!! Length of rate list too long \n");
		}


		sc_memory_copy(pele->data, sup_rates + 4,pele->length);
		//offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pele);
		WLAN_MOVE_TO_NEXT_ELE(pele);
	}	

	//get tx len
	pt_mng->txlen = (PUINT8)pele - pt_mng->tranbuf;

	//FILL some common info of hw header
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, WLAN_FSTYPE_PROBEREQ, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 0);
	//fill addr1 and addr2
	//if(( pt_mng->statuscontrol.curstatus == SCANNING)&&(pt_mng->statuscontrol.worktype & WITH_SSID_SCAN))
	if(pt_mng->statuscontrol.worktype & SCAN_STATE_BSSID_PRESENT_FLAG)
	{
		addr1 = addr3 = pt_mng->usedbssdes.bssid;
	}	
	else
	{
		addr1 = addr3 = broadcastaddr;
	}	
	
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
        	hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
        	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
        	hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

#if 0
{	
	int i;
	for(i = 0; i < 7; i++)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"probe req head[%x] = %x \n",i, hw_header[i]);
	}
	
}	
#endif	

	pt_mng->txlen = (pt_mng->txlen + 3) & 0xfffc;

	for(i=0;i<SEND_PROBE_TIMES;i++)
	{
		sc_memory_copy(&pt_mng->tranbuf[pt_mng->txlen*(i+1)],&pt_mng->tranbuf[pt_mng->txlen*(i)],pt_mng->txlen);
	}
	//sc_memory_copy(&pt_mng->tranbuf[pt_mng->txlen],pt_mng->tranbuf,pt_mng->txlen);
	pt_mng->txlen = pt_mng->txlen*SEND_PROBE_TIMES;
	pt_mng->is_probe = TRUE;

	/* send frame */
    Tx_Transmit_mng_frame(pAdap,pt_mng->tranbuf,pt_mng->txlen,TX_MNG_FRM_PRIORITY_NORMAL,TX_MNG_TYPE_PROBEREQ);
	return STATUS_SUCCESS;
	//return Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
}

/*******************************************************************
 *   Mng_MakeBeaconOrProbersp
 *   
 *   Descriptions:
 *      build up beacon frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
  *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeBeaconOrProbersp(PDSP_ADAPTER_T pAdap,UINT8 type,PUINT8 probeaddr)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T       		pt_mattr = &pAdap->wlan_attr;
	UINT8				broadcastaddr[]={0xff,0xff,0xff,0xff,0xff,0xff};
//	UINT16				offset;
	PUINT8                       addr1 = NULL;
	PUINT8				addr3 = NULL;
	UINT8				sup_len;
	UINT8 				sup_rates[12];
	PMNG_NOFIXED_ELE_ALL_T		pele;
	PBEACON_FIXED_FIELD_T		pbeacon_fixed;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;

	ASSERT(pAdap);
	
	/*hw header pos*/
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)pt_mng->tranbuf;
	//offset = sizeof(USBWLAN_HW_FRMHDR_TX);

	//fill fix content of beacon 
	//beacon interval	
	pbeacon_fixed = (PBEACON_FIXED_FIELD_T)(pt_mng->tranbuf + sizeof(USBWLAN_HW_FRMHDR_TX));
	pbeacon_fixed->beaconinterval = pt_mng->usedbssdes.beaconinterval;

	//cap
	pbeacon_fixed->cap = pt_mng->usedbssdes.cap;

	//get nofixed offset
	pele = WLAN_CONVERT_FRAME_NOFIXED_ELE(
		pt_mng->tranbuf + sizeof(USBWLAN_HW_FRMHDR_TX) + sizeof(BEACON_FIXED_FIELD_T));
	//set ssid
	pele->element = SSID_EID;
	//set ssid element 
	pele->length = pt_mng->usedbssdes.ssid[0];
	sc_memory_copy(pele->data,&pt_mng->usedbssdes.ssid[1], pt_mng->usedbssdes.ssid[0]);

	//support rate
	WLAN_MOVE_TO_NEXT_ELE(pele);
	pele->element = SUPRATE_EID;
	Adap_set_support_rate(
		pAdap,
		pAdap->wlan_attr.gdevice_info.dot11_type,		//alwaye save phy mode for scan or oid scan
		sup_rates,
		&sup_len);

	if(sup_len >12)
	{
	     	sup_len = 12;
    		DBG_WLAN__MLME(LEVEL_TRACE,"!! Length of rate list too long \n");
	}
	
	//11a: put all rate into support element, (rate number is 8)
	//11b & 11g: put 11b rate into this element, (number is 4)
	pele->length = (pAdap->wlan_attr.gdevice_info.dot11_type  == IEEE802_11_A) ? sup_len : 4;
	sc_memory_copy(pele->data,sup_rates,pele->length);

	//set channel
	WLAN_MOVE_TO_NEXT_ELE(pele);
	pele->element = DSPARA_EID;
	pele->length = 1;
	pele->data[0] = pt_mng->usedbssdes.channel;

	//set atim window
	WLAN_MOVE_TO_NEXT_ELE(pele);
	pele->element = IBSSPARA_EID;
	pele->length = 2;
	pele->data[0] = 0;
	pele->data[1] = 0;
	

	//extent support rate
	WLAN_MOVE_TO_NEXT_ELE(pele);
	if(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G)
	{
		//pele = (PMNG_NOFIXED_ELE_ALL_T)(pt_mng->tranbuf + offset);
		//WLAN_MOVE_TO_NEXT_ELE(pele);
		pele->element = EXTENDRATE_EID;
		pele->length = sup_len - 4;
		
		if(pele->length >8)
		{
			pele->length = 8;
	    		DBG_WLAN__MLME(LEVEL_TRACE,"!! Length of rate list too long \n");			
		}
		
		
		sc_memory_copy(pele->data, sup_rates + 4,pele->length);
		//offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pele);
		WLAN_MOVE_TO_NEXT_ELE(pele);
	}	

	//get tx len
	pt_mng->txlen = (PUINT8)pele - pt_mng->tranbuf;
	//Fill some common info of hw header
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, type, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 0);

	//fill addr1 and addr2
	//if(( pt_mng->statuscontrol.curstatus == SCANNING)&&(pt_mng->statuscontrol.worktype & WITH_SSID_SCAN))
	addr3 = pt_mng->usedbssdes.bssid;

	//A1 is dest IBSS STA for probe frame
	if(type == WLAN_FSTYPE_PROBERESP)
	{
		addr1 = probeaddr;
	}
	//A1 is broadcast frame for beacon frame
	else
	{
	   	addr1 =broadcastaddr;
	}
	
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
        	hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
        	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
        	hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

#if 0
{	
	int i;
	for(i = 0; i < 7; i++)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"probe req head[%x] = %x \n",i, hw_header[i]);
	}
	
}	
#endif	

	/* send frame */

	Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);

#if 0	//justin:	let 8051 do this. 	to fix fail to change beacon in dsp fifo 
	{
			UINT32 value;
			value = VcmdR_3DSP_Dword(pAdap, 0x4050);
			value |= BIT23;
			VcmdW_3DSP_Dword(pAdap,value,0x4050);
			
	}
#endif

	//return Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
	return STATUS_SUCCESS;
}



/*******************************************************************
 *   Mng_SetTimerPro
 *   
 *   Descriptions:
 *      set timer
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      delayv: , delay time of timer
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_SetTimerPro(UINT32 delayv,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     	pt_mng = pAdap->pmanagment;
	
	if(	Adap_Driver_isHalt(pAdap)
	||	pt_mng->mng_timer_running)
	{
		return;
	}
	
	pt_mng->mng_timer_running=TRUE;
		
	DBG_WLAN__MLME(LEVEL_INFO,"Mng_SetTimerPro:%u\n",delayv); 
	
	wlan_timer_start(&pAdap->mng_timer,delayv);
}
/*******************************************************************
 *   Mng_ClearTimerPro
 *   
 *   Descriptions:
 *      clear timer
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_ClearTimerPro(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     	pt_mng = pAdap->pmanagment;

	if(pt_mng->mng_timer_running)
	{
		wlan_timer_stop(&pAdap->mng_timer);	

		pt_mng->mng_timer_running=FALSE;
	}
}

/*******************************************************************
 *   Mng_IbssScanFail
 *   
 *   Descriptions:
 *      execute the routine when scan fail on IBSS
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
static inline VOID Mng_IbssScanFail(PDSP_ADAPTER_T pAdap)
{
	//woody open for ibss	
	PMNG_STRU_T     	pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T		pt_mattr = &pAdap->wlan_attr;
	UINT16			flag;
	UINT8          		save_ssid[WLAN_SSID_MAXLEN+1];      /* first byte is length */

	PCAPABILITY_T pCap =(PCAPABILITY_T)& pt_mng->usedbssdes.cap;

	//DBG_ENTER();
//	union {
//		UINT64		t64;
//		UINT32		t32[2];
//		UINT8		t8[8];
//	} tt;


	sc_memory_copy(save_ssid,pt_mng->usedbssdes.ssid,pt_mng->usedbssdes.ssid[0]+1);
	sc_memory_set(&pt_mng->usedbssdes, 0, sizeof(MNG_DES_T));//justin:	080427.	init used bss des, prevent dirty data saved last time connected

	sc_memory_copy(pt_mng->usedbssdes.ssid,save_ssid,save_ssid[0]+1);
	

	flag = (WRITE_ALL_BEACONINFO_REG) & (~WRITE_AID_REG);
	
	//init ibss parameters
	pt_mng->usedbssdes.channel= pt_mattr->channel_default;
	
	//init some ibss default value
	pt_mng->usedbssdes.beaconinterval= pt_mattr->ibss_beacon_interval;

	
	//generate bssid of ibss
	Mng_build_ibss_bssid(pAdap,pt_mng->usedbssdes.bssid);
	//set cap value
	sc_memory_set(&pt_mng->usedbssdes.cap, 0, sizeof(UINT16));
	pCap->ibss = 1;
	
	if(pAdap->wlan_attr.gdevice_info.privacy_option == TRUE)
	{
		pCap->privacy= 1;
	}

	//write beacon register into hw,in fact only set channel be done.
	//Mng_WriteBeaconInfoReg(WRITE_ALL_BEACONINFO_REG,&pt_mng->usedbssdes, pAdap);
	DBG_WLAN__MLME(LEVEL_TRACE,"IBSS SCAN FAIL,ibss beacon interval = %x\n",pAdap->wlan_attr.ibss_beacon_interval);

	if(pt_mng->usedbssdes.beaconinterval == 0)
	{
		pt_mng->usedbssdes.beaconinterval = 100;
	}
	
	//pt_mattr->beacon_interval = pAdap->wlan_attr.ibss_beacon_interval;
	 pAdap->wlan_attr.ibss_beacon_interval = pt_mng->usedbssdes.beaconinterval;
	pt_mng->usedbssdes.networkType = Ndis802_11OFDM24;
	
	//set parameter from pt_mattr to pt_mng
	//Mng_ExchangeMattrAndMng(flag, &pt_mng->usedbssdes, 1, pAdap);
	
	Adap_set_support_rate(
		pAdap,
		pAdap->wlan_attr.gdevice_info.ibss_dot11_type,		
		&pt_mng->usedbssdes.suprate[1],
		&pt_mng->usedbssdes.suprate[0]);

	pt_mng->usedbssdes.rssi = 0x44;

	//fixe ibss as 11g
	//pt_mng->usedbssdes.phy_mode = IEEE802_11_G;
	pt_mng->usedbssdes.phy_mode = (UINT32)pAdap->wlan_attr.gdevice_info.ibss_dot11_type;//IEEE802_11_B;
	pAdap->wlan_attr.gdevice_info.dot11_type = pAdap->wlan_attr.gdevice_info.ibss_dot11_type;

	Mng_Renew_BssList(pAdap, &pt_mng->usedbssdes);

	//build beacon frame and transmit it	
	DBG_WLAN__MLME(LEVEL_TRACE,"ibss scan fail, phy_mode = %x\n",pt_mng->usedbssdes.phy_mode);

	Mng_StartJoin(pAdap, JOIN_STATE_KEEP_WITH_BEACON | JOIN_SUBTYPE_WAIT_CHAN_CFM);

}






/*******************************************************************
 *   Mng_SetLinkToUp
 *   
 *   Descriptions:
 *      inform up lever that station have connect to ap successfully.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      ch: , channel
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_SetLinkToUp(PDSP_ADAPTER_T pAdap)
{
	pAdap->mib_stats.good_data_packet_rx = 0;
	pAdap->wpa_timeout = 0;
	Mng_GetRateIndex(pAdap);
	Adap_SetConnection(pAdap,JOIN_HASJOINED);
	Adap_SetLink(pAdap,LINK_OK);
		
	if(pAdap->must_indicate_uplevel_flag)
	{
		Adap_UpdateMediaState(pAdap,LINK_OK);
		pAdap->must_indicate_uplevel_flag = FALSE;
	}
	else
	{
		pAdap->need_fast_checkmedia_flag = TRUE;
//wumin		wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);
	}

	Tx_Send_Next_Frm(pAdap);

	
	pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
	if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
		pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;

	pAdap->scanning_flag = FALSE;		//justin:	080703.	scan must have done if run here.
	
	sc_netq_start(pAdap->net_dev);
}



/*******************************************************************
 *   Mng_Receive
 *   
 *   Descriptions:
 *      the function is called by other module when a management
 *      frame received.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      rxbuf: , buffer pointer (Justin comment: this is the frag of 3dsp format without the first UINT32 data)
 *      rxlen: , frame len
 *      rssi: , rssi value
 *   Return Value:
 *      none
 ******************************************************************/

VOID Mng_Receive(PUINT8 rxbuf,UINT32 rxlen,PRX_TO_MNG_PARAMETER pMngPara,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PMNG_HEAD80211_T	phead =(PMNG_HEAD80211_T)pt_mng->recbuf ;
//	PBEACON_FIXED_FIELD_T   pfix;
//	PUINT8				pbssaddr;
	
	if(!Adap_Driver_isWork(pAdap))
	{
		return;
	}
	
	//packet is invalid in length
	if (rxlen == 0 || rxlen >= MNGRXBUF_MAXLENGTH ) 
	{
		pt_mng->rssi = pMngPara->rssi;
		return;
	}
	
	//copy rx data to mng structure
	pt_mng->rxlen = (UINT16)rxlen;
	pt_mng->body_padding_len = pMngPara->body_padding_len;
	sc_memory_copy(pt_mng->recbuf, rxbuf, rxlen);
	pt_mng->rssi = pMngPara->rssi;
	
	//transact for oid scan process
        if(pt_mng->oidscanflag)
    	{
		Mng_Rx_WhenOidScan(pAdap, pMngPara->rssi);
		return;
	}

#if 1	//justin:	071217.		problem in this section. maybe receive proberequest, in this case, will cause driver fault.....
					////so, change to reject deauth and disassoc from other AP
//***************************(Justin: 071213		
/* if it is not beacon or probersp frame, we just accept mng frames from the AP that we connected or want to connect */
	if(pAdap->link_ok == LINK_OK)	//filter enable only when connected!
	{
		phead = (PMNG_HEAD80211_T)pt_mng->recbuf;	

		//if( (DOT11_FRM_IS_DEAUTH(phead->fc))
		//	||(DOT11_FRM_IS_DISASSOC(phead->fc))
		//	)//!DOT11_FRM_IS_PROBE_RESP(phead->fc)  &&  !DOT11_FRM_IS_BEACON(phead->fc))
		if(	(	phead->fc == DEAUTH
			||	phead->fc == DISASSOC)
		&&	(!IS_MATCH_IN_ADDRESS(phead->adr.addr.a2, pt_mng->usedbssdes.bssid)))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"received no beacon and no probersp frame and no our AP's mng frame\n"); 
			return;
		}
	}
//******************************
#endif

	switch (pt_mng->statuscontrol.curstatus)
	{
	case IDLE:
		break;
	case SCANNING:
		Mng_Rx_WhenScan(pAdap);
		/* rProbeRsp(type,scanTimeType,len); */
		break;
	case JOINNING:
		/* if(rBeacon(type)) */
		Mng_Rx_WhenJoin(pAdap,pMngPara->rssi);
		break;
	case JOINOK:
		/* Deal_Joinok(type); */
		Mng_Rx_WhenJoinOk(pAdap);
		break;
	case AUTHOK:
		/* Deal_Authok(type,len); */
		Mng_Rx_WhenAuthOk(pAdap);
		break;
	case ASSOCOK:
		/* Deal_Assocok(type); */
		Mng_Rx_WhenAssocOk(pAdap);
		break;
	default: 	
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Receive into default fail \n");
		break;
	}
	return;

}
/*******************************************************************
 *   Mng_Rx_WhenScan
 *   
 *   Descriptions:
 *      the function is called with scanning status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      len: , frame len
 *   Return Value:
 *      none
 ******************************************************************/
//update it by Justin
//woody, simply the scan procedure
//if a suitable AP reveived, then stop scan instead of adding into bss des list
VOID Mng_Rx_WhenScan(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PMNG_DESLIST_T			pt_deslist = &pt_mng->bssdeslist;
	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PBEACON_FIXED_FIELD_T	pfixed;
	PMNG_HEAD80211_T		phead;
	MNG_DES_T				curdes;
	//UINT32					i;
	PMNG_NOFIXED_ELE_ALL_T	pele;


	//(Justin comment: this is the frag of 3dsp format without the first UINT32 data)
	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;	


	/* is not beacon or probersp frame */
	if(	!DOT11_FRM_IS_PROBE_RESP(phead->fc)  
	&&	!DOT11_FRM_IS_BEACON(phead->fc))
	{
		//DBG_WLAN__MLME(("received no beacon and no probersp frame when scan\n")); 
		return;
	}

	// Justin: beacon and probersp frame have the same format except that beacon has TIM information at the end position

	DBG_WLAN__MLME(LEVEL_INFO,"received beacon or probersp frame when scan\n"); 
	
	//get pointer to beginning of data of beacon frame
	pfixed = (PBEACON_FIXED_FIELD_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);
	//copy cap of AP
	curdes.cap = pfixed->cap;

	//get pointer to beginning of first no fixed element of beacon frame	
	pele = WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);

	//ssid scan, just ssid match, we can join it
	//use for win2000 case
	if(pt_mng->statuscontrol.worktype & SCAN_STATE_SSID_FLAG)
	{
		if( IS_MATCH_IN_SSID(pele->element,pele->length,pele->data,
			SSID_EID,pt_mattr->ssid_len,pt_mattr->ssid) 	&&
			IS_MATCH_IN_ESSMODE(pt_mattr->macmode,curdes.cap))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"find suitable AP by scan\n");
			/*
			STAs in an infrastructure network shall only use other information in received Beacon frames, if the BSSID
			field is equal to the MAC address currently in use by the STA contained in the AP of the BSS.
						
						  STAs in an IBSS shall use other information in any received Beacon frame for which the IBSS subfield of
			the Capability field is set to 1 and the content of the SSID element is equal to the SSID of the IBSS. Use of
			this information is specified in 11.1.4.  

			  11.1.3
			Upon receipt of an MLME-JOIN.request, the STA will join a BSS by adopting the BSSID, TSF timer value,
			PHY parameters, and the beacon period specified in the request.			*/
		}
		else
		{
			//return;
		}
	}
	//bssid scan, an appointed AP should be found
	//use for the case that no ssid include in beacon frame
	else if(pt_mng->statuscontrol.worktype & SCAN_STATE_BSSID_PRESENT_FLAG)
	{
		//there is problem in this case, for ibss , position of bssid is same with infrastruecure
		if(!IS_MATCH_IN_ADDRESS(phead->adr.addr.a3, pt_mattr->bssid)) 
		{
			return;
		}

		if(!IS_MATCH_IN_ESSMODE(pt_mattr->macmode,curdes.cap))
		{
			return;
		}
	}
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"mng_rx_whenscan into error case \n");
	}

	/*  scan begin */
	if(!(Mng_GetBeaconInfo(&curdes, pAdap)))
	{
		return;
	}
	//add phy mode flag into the ap's description
	curdes.phy_mode = (UINT32)pt_mng->statuscontrol.workflag;
	curdes.rssi = (INT32)pt_mng->rssi ;	//Justin: 0626
	
#ifdef ANTENNA_DIVERSITY
	curdes.antenna_num = pAdap->wlan_attr.antenna_num;
#endif


	if(GetRssiMsb(curdes.rssi) != 0)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"###Find dlink preamble head \n");
	}
#ifdef DEBUG_RSSI
	DBG_WLAN__MLME(LEVEL_TRACE,"rx when scan rssi = %X\n",curdes.rssi);
#endif

	//curdes.rssi = (long)Phy_GetRssi((PDSP_PHY_INFO_T)pAdap->pphy,rssi);

	if(curdes.channel == pt_mng->curchannel)
	{
		pt_mng->haveSignal = 1;
	}

	if(pt_mng->statuscontrol.worktype & SCAN_STATE_MID_STOP_FLAG)
	{
	

		Mng_Renew_BssList(pAdap, &curdes);

		/* Mng_ClearScanFlag(pAdap); */
		//only compare ssid content is not enough, length must be match also
		//if(sc_memory_cmp(&pt_mng->usedbssdes.ssid[1], &curdes.ssid[1], pt_mng->usedbssdes.ssid[0] ))	//Justin: match the ssid we want to join
		if(0 == sc_memory_cmp(pt_mng->usedbssdes.ssid, curdes.ssid, pt_mng->usedbssdes.ssid[0]+1 ))	//Justin: match the ssid we want to join
		{
			/* EndScanWithSsid(); */
			Mng_ClearTimerPro(pAdap);
			sc_memory_copy(&pt_mng->usedbssdes, &curdes, sizeof(MNG_DES_T));
            
            //if find the hiding ssid ap,update the bssid 
            sc_memory_copy(&pt_mng->hide_bssid,curdes.bssid,WLAN_ETHADDR_LEN);
            pt_mng->hide_ssid_found = TRUE;
            
			DBG_WLAN__MLME(LEVEL_TRACE,"FOUND AP with specified SSID, ch = %d\n",pt_mng->usedbssdes.channel);

			//justin: 071123.	to get a right privacy
			//if(find_ssid_result != ASSIGN_SSID_NO_FOUND)
			//woody first close it
			//because encryption decided by add key oid
			//rx when scan called by ssid oid
			//driver get add key oid earlier than ssid oid 
			//so this is a risk to change encryption mode
			if((pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA))
			{
				pAdap->wlan_attr.gdevice_info.privacy_option = CAP_EXPRESS_PRIVACY(pt_mng->usedbssdes.cap);

				//wumin 090613
				//End Scan when found AP with hidden SSID
				//Change state to IDLE
				//Do NOT start Join process 
				//Because WPA_supplicant hadn't seting the correct Auth mode at this monment
				//Just wait WPA_supplicant send ioctl_set_auth and the second ioctl_set_essid
				
				DBG_WLAN__MLME(LEVEL_TRACE,"Stop scan with SSID, change to IDLE mode.\n");

				pAdap->is_oid_scan_begin_flag =FALSE;;
				pt_mng->oidscanflag = TRUE;
				pt_mng->statuscontrol.curstatus = IDLE;
				pt_mng->statuscontrol.worktype = 0;   		
				Mng_ClearTimerPro(pAdap);
				pAdap->scanning_flag = FALSE;
				
			}	
			else
			{
				Mng_MidJoinUntilSuccess(pAdap);
			}
		}
	}
	else
	{
		if ( pt_deslist->bssdesnum < WLAN_BSSDES_NUM) 
		{
			sc_memory_copy((PUINT8)&pt_deslist->bssdesset[pt_deslist->bssdesnum], &curdes, sizeof(MNG_DES_T));
			pt_deslist->bssdesnum++;
		}
		/*
		pt_deslist->bssdesnum = 1;
		sc_memory_copy((PUINT8)&pt_deslist->bssdesset[0], &curdes, sizeof(MNG_DES_T));
		DBG_WLAN__MLME(("get an AP by scan and always set bss des list = 1 \n"));
		*/
	}
}


/*******************************************************************
 *   Mng_GetChannelFromBeacon
 *   
 *   Descriptions:
 *      get channel info from beacon frame
 *   Arguments:
 *      recbuf: , start address of beacon frame
 *   Return Value:
 *      channel value included in beacon frame
 ******************************************************************/
 //update it by Justin
static UINT8 Mng_GetChannelFromBeacon(PUINT8 recbuf)
{
	PMNG_NOFIXED_ELE_ALL_T  pele;
	//UINT8   	chan;
	//PUINT8	ptmp;
	UINT8 	num = 5;   // due to dsp para = 3
	UINT32	i;

	pele = WLAN_BEACON_FIRST_NOFIXED_ELE(recbuf);
	
	for(i = 0; i < num; i++)
	{
		if(DSPARA_EID == pele->element)
		{
			return pele->data[0];
		}
		WLAN_MOVE_TO_NEXT_ELE(pele);
	}

	return 0;


	
	/*pele = (PMNG_NOFIXED_ELE_HEAD_T)(recbuf + WLAN_HDR_A3_LEN + sizeof(BEACON_FIXED_FIELD_T));
	
	if(pele->element == SSID_EID)
	{
		pele = (PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pele + sizeof(MNG_NOFIXED_ELE_HEAD_T) + pele->length);
	}
	if(pele->element == SUPRATE_EID)
	{
		pele = (PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pele + sizeof(MNG_NOFIXED_ELE_HEAD_T) + pele->length);
	}
	if(pele->element == FHPARA_EID)
	{
		pele = (PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pele + sizeof(MNG_NOFIXED_ELE_HEAD_T) + pele->length);
	}

	if(pele->element == DSPARA_EID)
	{
		ptmp = (PUINT8)((PUINT8)pele + sizeof(MNG_NOFIXED_ELE_HEAD_T));
		chan = *ptmp;
		if(chan > 0 && chan < (MAXCHANNEL + 1))
		{
			return chan;
		}
	}
	
	return 0;
	*/	
}



/*******************************************************************
 *   Mng_GetChannelFromBeacon
 *   
 *   Descriptions:
 *	  //update it by Justin
 *      get ssid information from beacon frame
 *   Arguments:
 *      recbuf: , start address of beacon frame
 *      pssid:   ,save ssid to the address
 *      len:      ,ssid's length
 *   Return Value:
 *      indicate if ssid get successfully
 ******************************************************************/
BOOLEAN Mng_GetSSIDFromBeacon(PUINT8 recbuf,PUINT8 pssid,UINT8 *len)
{
	PMNG_NOFIXED_ELE_ALL_T  pele;

	pele = WLAN_BEACON_FIRST_NOFIXED_ELE(recbuf);

	//ssid's id is 0, we just do search once		
	if((SSID_EID == pele->element) && (pele->length != 0))
	{
		if(pele->length <= WLAN_SSID_MAXLEN) 
		{
			*len = pele->length;
			sc_memory_copy(pssid,pele->data,pele->length);
			return TRUE;
		}
	}
	return FALSE;
}

/*******************************************************************
 *   Mng_Rx_WhenJoin
 *   
 *   Descriptions:
 *      the function is called with join status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
 //update it by Justin
VOID Mng_Rx_WhenJoin(PDSP_ADAPTER_T pAdap,UINT8 rssi)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T			pt_mattr = &pAdap->wlan_attr;
	PMNG_HEAD80211_T	phead =(PMNG_HEAD80211_T)pt_mng->recbuf ;
	PUINT8				pbssaddr;
	UINT8				chan;
//	MNG_DES_T			curdes;
	UINT8	            		length;
	PBEACON_FIXED_FIELD_T   pfix;

	//get data pointer of beacon
	pfix = (PBEACON_FIXED_FIELD_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);

	//
	if( (pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_WAIT_BEACON) == 0 )
	{
//		DBG_WLAN__MLME(LEVEL_TRACE,"Joining but register not read \n");
		return;
	}
		
	if(DOT11_FRM_IS_PROBE_REQ(phead->fc) && 
		(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA))
	{
		Mng_Transact_Probersp(pAdap);
			//DBG_WLAN__MLME(("received probe when join\n"));
		return;
	}
	
	//only transact beacon frame for join 
	if(!DOT11_FRM_IS_BEACON(phead->fc))
	{
		//DBGMLME(pAdap,("received no beacon when join\n")); 
		DBG_WLAN__MLME(LEVEL_TRACE,"received no beacon when join\n");
		return;
	}


	//woody debug
	if(CAP_EXPRESS_IBSS(pfix->cap))
	{
		UINT8 buf[100]; 
		UINT8 len;
		UINT8 str[]={'N','D','T','E','S','T','_','I','B','S','S'};
		Mng_GetSSIDFromBeacon(pt_mng->recbuf,buf,&len);

		if(len == 0xb &&
			(0 == sc_memory_cmp(buf,str,len)))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"find ndtest_ibss net\n");
		}
		
	}
		

	//get bssid pointer according infra mode
	if(IS_MATCH_IN_ESSMODE(pt_mattr->macmode,pfix->cap))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"received beacon match ess mode when join\n");
		pbssaddr = CAP_EXPRESS_BSS(pfix->cap) ? 
			phead->adr.bssrx.bssid : phead->adr.ibss.bssid;
		
		DBG_WLAN__MLME(LEVEL_TRACE,"BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
			pbssaddr[0],pbssaddr[1],pbssaddr[2],pbssaddr[3],pbssaddr[4],pbssaddr[5]);
	}
	else
	{
		UINT8 buf[100]; 
		UINT8 len;
		Mng_GetSSIDFromBeacon(pt_mng->recbuf,buf,&len);

		if(len == pt_mng->usedbssdes.ssid[0] &&
			(0 == sc_memory_cmp(buf,pt_mng->usedbssdes.ssid+1,len)))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"beacon from same ssid recv but mismatch feature\n");
		}
		
		DBG_WLAN__MLME(LEVEL_TRACE,"return when join due to mismatch feature\n");
		//DBGMLME(pAdap,("return when join due to mismatch feature\n")); 
		return;
	}

	if( (pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA) )
	{
		UINT8 buf[100]; 
		UINT8 len;
		Mng_GetSSIDFromBeacon(pt_mng->recbuf,buf,&len);

		if(len == pt_mng->usedbssdes.ssid[0] &&
			(0 == sc_memory_cmp(buf,pt_mng->usedbssdes.ssid+1,len)))
		{
			//sc_memory_copy(pAdap->wlan_attr.bssid,pbssaddr,6);
			sc_memory_copy(pt_mng->usedbssdes.bssid,pbssaddr,6);
			Vcmd_hal_set_bssid(pAdap,pbssaddr);
			Mng_MakeBeaconOrProbersp(pAdap,WLAN_FSTYPE_BEACON,NULL);
			
			DBG_WLAN__MLME(LEVEL_TRACE,"change bssid = %x %x %x  %x %x %x\n",pbssaddr[0],
				pbssaddr[1],pbssaddr[2],pbssaddr[3],pbssaddr[4],pbssaddr[5]);
		}
	}

	
	//bss case
	DBG_WLAN__MLME(LEVEL_TRACE,"enter rbeacon function when join\n"); 
	//got a beacon with expected bssid
	if(IS_MATCH_IN_ADDRESS(pbssaddr, pt_mng->usedbssdes.bssid))
	{
		// check if channel is match 
		chan = Mng_GetChannelFromBeacon(pt_mng->recbuf);
		
		if(chan != pt_mng->usedbssdes.channel)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Join fail for mismatch channel\n"); 
			return;
		}
		
		
		//add for the case:
		//DSP will not join in the AP without RSN element when auth mode are both wpa and psk-wpa.
		//Jakio2006.12.18: add for wpa2 case
		#ifdef DSP_WPA2
		if((pAdap->wlan_attr.auth_mode != AUTH_MODE_OPEN) &&
			(pAdap->wlan_attr.auth_mode != AUTH_MODE_SHARED_KEY)  &&
			(!Mng_GetOneElementFromBeacon(pAdap,WPA1_EID,NULL,&length)) &&
			(!Mng_GetOneElementFromBeacon(pAdap,WPA2_EID,NULL,&length)))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Join fail for no wpa rsn field in beacon\n"); 
			return;
		}
		#else
		if((pAdap->wlan_attr.auth_mode != AUTH_MODE_OPEN) &&
			(pAdap->wlan_attr.auth_mode != AUTH_MODE_SHARED_KEY)  &&
			(!Mng_GetOneElementFromBeacon(pAdap,WPA1_EID,NULL,&length)))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Join fail for no wpa rsn field in beacon\n"); 
			return;
		}
		#endif
		//Jakio20070515: duplicated code , close it
		/*
		if((pAdap->wlan_attr.auth_mode != AUTH_MODE_OPEN) &&
			(pAdap->wlan_attr.auth_mode != AUTH_MODE_SHARED_KEY)  &&
			(!Mng_GetOneElementFromBeacon(pAdap,WPA1_EID,NULL,&length)))
		{
			DBG_WLAN__MLME(("Join fail for no wpa rsn field in beacon\n")); 
			return;
		}
		*/

		//now close it
		//Mng_GetCountryWhenJoin(pAdap);


		//get support rate
		{
		 // add for NETGEAR ap, netgear's support rate is difference between beacon and probe response
			UINT8   len_sup;

			//get support rate from beacon
			len_sup = SUPPORTRATELEN;
			if(Mng_GetOneElementFromBeacon(pAdap,SUPRATE_EID,&pt_mng->usedbssdes.suprate[1],&len_sup))
			{
				pt_mng->usedbssdes.suprate[0] = len_sup;
			}	

			//get extent support rate from beacon
			len_sup = MAX_ESR_LEN;
			if(Mng_GetOneElementFromBeacon(pAdap,EXTENDRATE_EID,pt_mng->usedbssdes.ExtendedSupportRate,&len_sup))
			{
				pt_mng->usedbssdes.ExtendedSupportRateLen = len_sup;
			}	


			Mng_DeleteUnsupportedRate(pAdap,&pt_mng->usedbssdes);
		}

		//close these sentence, do them in startjoin function
		//set preamble according to CAP of AP
		//pAdap->wlan_attr.preamble_type = 
		//	CAP_EXPRESS_SHORT_PREAMBLE(pt_mng->usedbssdes.cap) ? SHORT_PRE_AMBLE : LONG_PRE_AMBLE;
		//re-cal cap due to preamble type changed
		//Adap_Cal_Capability(pt_mattr);

		//set slot type according to CAP of AP
		//pAdap->wlan_attr.gdevice_info.slot_time = 
		//	CAP_EXPRESS_SHORT_SLOTTIME(pt_mng->usedbssdes.cap) ? SHORT_PRE_AMBLE : LONG_PRE_AMBLE;
		


		//if(!(Mng_GetBeaconInfo(&curdes, pAdap)))
		//{
		//	return;
		//}
		//sc_memory_copy(&pt_mng->usedbssdes, &curdes, sizeof(MNG_DES_T));
		
		

		//sc_memory_copy(pt_mattr->bssid, pt_mng->usedbssdes.bssid, WLAN_ADDR_LEN);

		/* beacon interval*/
		//pt_mattr->beacon_interval = pt_mng->usedbssdes.beaconinterval;
		//pt_mattr->beacon_window = pt_mattr->beacon_interval * 100; 
		//pt_mattr->current_channel = chan;
		
		//Mng_WriteBeaconInfoReg(WRITE_CAP_REG, &pt_mng->usedbssdes, pAdap);
		/* set joinenbit*/
		

		// add for adjust ed thougthhold
		

		pt_mng->statuscontrol.curstatus = JOINOK;

		//woody add for ibss 
		pAdap->ap_alive_counter =(pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
		if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
			pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
		//set for preamble type and slot time type according to info of AP

		//now we reset these flag temporariy here
		pAdap->bStarveMac = FALSE;
		pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;
		

#ifdef  DSP_IBSS_OPEN   //close for ibss
		//ibss 
		//indicate to connect when ibss succeed in join
		if( pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"join ok under ibss\n"); 
			Mng_ExchangeMattrAndMng(WRITE_ALL_BEACONINFO_REG, &pt_mng->usedbssdes, 0, pAdap);
			Adap_SetRateIndex(pAdap);
			pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
			if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
				pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;

			pAdap->bStarveMac = FALSE;
			pAdap->wlan_attr.gdevice_info.tx_disable= FALSE;

			pAdap->must_indicate_uplevel_flag = TRUE;
			Mng_SetLinkToUp(pAdap);

			pt_mng->statuscontrol.curstatus = JOINOK;

			DBG_WLAN__MLME(LEVEL_TRACE,"join ok under ibss\n");
		
			return;
		}
		/* bss */
		else
#endif			
		{
			Mng_ClearTimerPro(pAdap);
			DBG_WLAN__MLME(LEVEL_TRACE,"join ok under bss\n"); 
			Adap_start_bss(pAdap);
			Mng_StartAuth(pAdap);
		}
	}
	//get a beacon without expected bssid
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"bssid is not same as expired bssid\n"); 
		if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA &&
			CAP_EXPRESS_IBSS(pfix->cap))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"join new: %x %x %x %x %x %x \n",
				pbssaddr[0],
				pbssaddr[1],
				pbssaddr[2],
				pbssaddr[3],
				pbssaddr[4],
				pbssaddr[5]
				);

			pbssaddr = pt_mng->usedbssdes.bssid;
			DBG_WLAN__MLME(LEVEL_TRACE,"join used: %x %x %x %x %x %x \n",
				pbssaddr[0],
				pbssaddr[1],
				pbssaddr[2],
				pbssaddr[3],
				pbssaddr[4],
				pbssaddr[5]
				);

			
		}
	}
}


/*******************************************************************
 *   Mng_StartAuth
 *   
 *   Descriptions:
 *      start authencation process after Join is sucess
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_StartAuth(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	/* pt_mng->curstatus = JOINOK; */
	pt_mng->statuscontrol.curstatus = JOINOK;
	pt_mng->statuscontrol.retrynum = 0;
	/* pt_mng->authstatus = AUTH_INIT; */
	/* pt_mng->authnum = 0; */
	pt_mng->statuscontrol.worktype = AUTH_INIT;

	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_StartAuth\n"); 
	
	Mng_DoAuth(pAdap);
}

//justin:	081009.	return 0xFF if not found bss info for this addr
UINT8 Mng_GetMaxIbssSupportRate(PDSP_ADAPTER_T pAdap, PUINT8 paddr)
{
	PMNG_STRU_T	pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;
	UINT8			i,j;
	UINT8 			uReturnVal = 0xFF;

	if(pt_mattr->is_exist_ibss_b_sta)
	{
		return OP_11_MBPS;
	}

	for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)
	{
		if(IS_MATCH_IN_ADDRESS(paddr, pt_mng->oiddeslist.bssdesset[i].addr))
		{	
			//Adap_PrintBuffer(pt_mng->oiddeslist.bssdesset[i].suprate, pt_mng->oiddeslist.bssdesset[i].suprate[0]+1);//first byte is len
			//Adap_PrintBuffer(pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRate, pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRateLen);
		
			//get max support rate
			uReturnVal = pt_mng->oiddeslist.bssdesset[i].suprate[1]&0x7f;
			for(j=2; j<=pt_mng->oiddeslist.bssdesset[i].suprate[0]; j++)
			{
				if(uReturnVal < (pt_mng->oiddeslist.bssdesset[i].suprate[j]&0x7f))
				{
					uReturnVal = pt_mng->oiddeslist.bssdesset[i].suprate[j]&0x7f;
				}
			}
			//DBGSTR(("ibss support rate count = 0x%x,  ex support rate counte = 0x%x\n",
			//	pt_mng->oiddeslist.bssdesset[i].suprate[0],pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRateLen));

			//get max extend support rate
			for(j=0; j<pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRateLen; j++)
			{
				if(uReturnVal < (pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRate[j]&0x7f))
				{
					uReturnVal = pt_mng->oiddeslist.bssdesset[i].ExtendedSupportRate[j]&0x7f;
				}
			}
			//DBGSTR(("ibss max s = 0x%x,  ex max s = 0x%x\n",uMaxSRate,uMaxESRate));

			if(uReturnVal <= OP_11_MBPS)
			{
				pt_mattr->is_exist_ibss_b_sta = TRUE;
			}
		}
	}

	return uReturnVal;
}



/*******************************************************************
 *   Mng_Rx_WhenJoinOk
 *   
 *   Descriptions:
 *      the function is called with joinok status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
// update by Justin
VOID Mng_Rx_WhenJoinOk(PDSP_ADAPTER_T pAdap)
{

	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PUINT8					paddr;
	PMNG_HEAD80211_T		phead;
	PMNG_AUTHFRAME_DATA_T pAuthData;
	UINT32					i;
	PMNG_NOFIXED_ELE_ALL_T	pele;
	PMNG_DISASSOC_DATA_T		pDeAuth;


	PUINT8				pbssaddr;
	PBEACON_FIXED_FIELD_T   pfix;
	MNG_DES_T				des;
	

	//get data pointer of beacon
	pfix = (PBEACON_FIXED_FIELD_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);

	
	//Check it later
	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;
	pele = WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);

	/* ibss */
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{

		if( (pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA) )
		{
			UINT8 buf[100]; 
			UINT8 len;


			//get bssid pointer according infra mode
			if(IS_MATCH_IN_ESSMODE(pt_mattr->macmode,pfix->cap))
			{
				pbssaddr = CAP_EXPRESS_BSS(pfix->cap) ? 
					phead->adr.bssrx.bssid : phead->adr.ibss.bssid;
			}
			else
			{
				//DBG_WLAN__MLME(("return when join due to mismatch feature\n")); 
				return;
			}
			
			Mng_GetSSIDFromBeacon(pt_mng->recbuf,buf,&len);

#if 0	
			if(len == pt_mng->usedbssdes.ssid[0] &&
				sc_memory_cmp(buf,pt_mng->usedbssdes.ssid+1,len))
			{
				sc_memory_copy(pAdap->wlan_attr.bssid,pbssaddr,6);
				Vcmd_hal_set_bssid(pAdap,pbssaddr);
				DBG_WLAN__MLME(LEVEL_TRACE,"change bssid -= %x %x %x  %x %x %x\n",pbssaddr[0],
					pbssaddr[1],pbssaddr[2],pbssaddr[3],pbssaddr[4],pbssaddr[5]);
			}
#endif
		}

		
	/*//merge funtion
		if(Mng_Merge(pAdap))
		{
			return;
		}
		
		//

		DBG_WLAN__MLME(("received a mgt frame with status of joinok under ibss\n")); 

		if(pt_mng->ibssinfo.num >= MAXSTANUM_IBSS)
		{
			DBG_WLAN__MLME(("register ibss sta num > max and return\n")); 
			return;
		}
*/

		//send probe response
		if(DOT11_FRM_IS_PROBE_REQ(phead->fc)) 
		{
			Mng_Transact_Probersp(pAdap);
			return;
		}


		//parse beacon in same channel
		if(DOT11_FRM_IS_BEACON(phead->fc))
		{
			if(pt_mng->usedbssdes.channel != Mng_GetChannelFromBeacon(pt_mng->recbuf))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"channel is not same, return\n"); 
				return ;
			}
		}

		//just transact IBSS beacon for merge
		if(DOT11_FRM_IS_BEACON(phead->fc) && 
			!DOT11_FRM_FROM_DS(phead->fc) &&
			pt_mng->usedbssdes.ssid[0] == pele->length && 
			(0 == sc_memory_cmp(&pt_mng->usedbssdes.ssid[1], pele->data, pt_mng->usedbssdes.ssid[0]))
			)
		{
			//DBG_WLAN__MLME(("received a beacon frame with status of joinok under ibss\n")); 

			if(IS_MATCH_IN_ADDRESS(phead->adr.ibss.bssid,pt_mng->usedbssdes.bssid))//justin:	080506.	treat as alive only when match bssid
			{
				pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
					DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
				if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
					pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;

			}
			
			pAdap->bStarveMac = FALSE;
			pAdap->wlan_attr.gdevice_info.tx_disable= FALSE;

			//merge funtion
			if(!IS_MATCH_IN_ADDRESS(phead->adr.ibss.bssid,pt_mng->usedbssdes.bssid))
			{
				Mng_Merge(pAdap);
				return;
			}
			//merge end
			/* else
			{
				//for beacon change  liu
				MNG_DES_T  des;

				if(!Mng_GetBeaconInfo(&des, pAdap))
					return;
				
				if((des.atimw != pt_mng->usedbssdes.atimw) || (des.cap != pt_mng->usedbssdes.cap))
				{
					sc_memory_copy(&pt_mng->usedbssdes,&des,sizeof(MNG_DES_T));
					Task_CreateTaskForce((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_BEACON_LOST,DSP_TASK_PRI_NORMAL,NULL,0);
				}
				return; 
			} */
			
			//DBG_WLAN__MLME(("received a mgt frame with status of joinok under ibss\n")); 
			
			if(pt_mng->ibssinfo.num >= MAXSTANUM_IBSS)
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"register ibss sta num > max and return\n"); 
				return;
			}

			//justin:	081007
			if(!Mng_GetBeaconInfo(&des, pAdap))
				return;
			Mng_Renew_BssList(pAdap, &des);
			
			/* getsaddr */
			paddr = phead->adr.ibss.sa;
			for(i = 0; i < pt_mng->ibssinfo.num; i++)
			{
				if(IS_MATCH_IN_ADDRESS(paddr, pt_mng->ibssinfo.ibsssta[i].addr))
				{
					pt_mng->ibssinfo.ibsssta[i].ps = DOT11_FRM_PWR_MGMT(phead->fc);
					//DBG_WLAN__MLME(("the sta have been in ibsss sta list\n")); 
					return;
				}
			}

			if(pt_mng->ibssinfo.num == MAXSTANUM_IBSS)
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"ibsss sta list has full!! \n"); 
				return;
			}
			
			i = pt_mng->ibssinfo.num;
			
			WLAN_COPY_ADDRESS(pt_mng->ibssinfo.ibsssta[i].addr, paddr);
			pt_mng->ibssinfo.ibsssta[i].ps = DOT11_FRM_PWR_MGMT(phead->fc);
			pt_mng->ibssinfo.num++;
			DBG_WLAN__MLME(LEVEL_TRACE,"add a sta to ibss sta list when receive beacon of the sta\n"); 
			return;
		}
		/* other frame received with ibss status */
		else
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"is not a beacon frame received with status of joinok under ibss \n"); 
		}

		return;
	}
///////////////////////////////////////////////////////////////////////////////////////
	/* bss */
	if(phead->fc == AUTH)
	{
		/* saddr */
		
		if(!IS_MATCH_IN_ADDRESS(phead->adr.bssrx.sa, pt_mng->usedbssdes.addr))     
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Receive Auth Frame with incorrect BSSID when Auth. Excepted:%02x:%02x:%02x:%02x:%02x:%02x, Received:%02x:%02x:%02x:%02x:%02x:%02x\n",
				pt_mng->usedbssdes.addr[0],pt_mng->usedbssdes.addr[1],pt_mng->usedbssdes.addr[2],
				pt_mng->usedbssdes.addr[3],pt_mng->usedbssdes.addr[4],pt_mng->usedbssdes.addr[5],
				phead->adr.bssrx.sa[0],phead->adr.bssrx.sa[1],phead->adr.bssrx.sa[2],
				phead->adr.bssrx.sa[3],phead->adr.bssrx.sa[4],phead->adr.bssrx.sa[5]); 
			return ;
		}
		
		/* stop timer */
		Mng_ClearTimerPro(pAdap);

		//pointer to data field of auth frame
		pAuthData = (PMNG_AUTHFRAME_DATA_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);

		DBG_WLAN__MLME(LEVEL_TRACE,"Receive Auth Frame with correct BSSID when Auth. Status=%u, Seq=%u, Current  Seq=%u\n",
			pAuthData->status, pAuthData->authseq, pt_mng->statuscontrol.worktype);
		
		/* 	AuthProcess(); */
		if(	pAuthData->authseq == AUTH_SEQ2  
		&& 	pt_mng->statuscontrol.worktype == WAIT_AUTH2)
		{
			if(pAuthData->status == SUCCESS)
			{
				if(pt_mattr->auth_alg == AUTH_ALG_SHARED_KEY)
				{
					pele = WLAN_AUTHFRAME_CHALLENGE_ELE(pt_mng->recbuf);
					sc_memory_copy(pt_mng->rec_challenge_text, 
								pele->data,
								CHALLENGE_TEXT_LEN);
					Mng_MakeAuthFrame3(pAdap);
					
					pt_mng->statuscontrol.worktype = WAIT_AUTH4;
					
					Mng_ClearTimerPro(pAdap);
					Mng_SetTimerPro(pt_mattr->auth_timeout,pAdap);

					DBG_WLAN__MLME(LEVEL_TRACE,"auth2 ok,send auth3\n"); 
				}
				else
				{
					DBG_WLAN__MLME(LEVEL_TRACE,"open auth ok,start asoc\n"); 
					Mng_StartAssoc(pAdap);
				}
			}
			else
			/* status is error */
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Auth2 not success. status=%u\n",pAuthData->status); 
				
				if(pAuthData->status == DEAUTH_ERROR_MODE 
                    && pt_mattr->auth_mode == AUTH_MODE_OPEN)
				{
					DBG_WLAN__MLME(LEVEL_TRACE,"Change Auth mode to Shared Key.\n"); 
					//change auth mode and try again
					pt_mattr->auth_alg = AUTH_ALG_SHARED_KEY;
					pt_mattr->auth_mode = AUTH_MODE_SHARED_KEY;

					Mng_StartAuth(pAdap);
				}
				else
				{
					Mng_DoAuth(pAdap);
				}
			}
		}
		else if(	pAuthData->authseq == AUTH_SEQ4  
		&&		pt_mng->statuscontrol.worktype == WAIT_AUTH4)
		{
			if(pAuthData->status == SUCCESS)
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"auth4 status ok, start asoc\n"); 
				Mng_StartAssoc(pAdap);
			}
			else
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Auth4 not success. status=%u\n",pAuthData->status);
				pt_mng->statuscontrol.worktype = WAIT_AUTH2;
				Mng_DoAuth(pAdap);
			}
		}
		else if(	pAuthData->authseq == AUTH_SEQ2  
		&& 		pt_mng->statuscontrol.worktype == WAIT_AUTH4
		&&		pt_mattr->auth_alg == AUTH_ALG_SHARED_KEY)
		{
			if	(pAuthData->status == SUCCESS)
			{	
				pele = WLAN_AUTHFRAME_CHALLENGE_ELE(pt_mng->recbuf);
				sc_memory_copy(pt_mng->rec_challenge_text, 
							pele->data,
							CHALLENGE_TEXT_LEN);
				Mng_MakeAuthFrame3(pAdap);
					
				pt_mng->statuscontrol.worktype = WAIT_AUTH4;
					
				Mng_ClearTimerPro(pAdap);
				Mng_SetTimerPro(pt_mattr->auth_timeout,pAdap);

				DBG_WLAN__MLME(LEVEL_TRACE,"receive new auth2 with success,resend new auth3\n"); 
			}
			else	
			{
				Mng_MakeAuthFrame1(pAdap);
					
				pt_mng->statuscontrol.worktype = WAIT_AUTH2;
					
				Mng_ClearTimerPro(pAdap);
				Mng_SetTimerPro(pt_mattr->auth_timeout,pAdap);

				DBG_WLAN__MLME(LEVEL_TRACE,"Receive new auth 2 with failure, resend auth 1.\n"); 				}
		}

		else
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"is not a expected atuh\n"); 
			Mng_SetTimerPro(pt_mattr->auth_timeout,pAdap);
		}
	}
	else if(phead->fc == DEAUTH || phead->fc == DISASSOC)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"received a deauth or disassoc\n"); 
		pDeAuth = (PMNG_DISASSOC_DATA_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);
		//if()DEAUTH_ERROR_MODE
		if(pDeAuth->reason == DEAUTH_ERROR_MODE)
		{
			//change auth mode and try again
			pt_mattr->auth_alg = AUTH_ALG_SHARED_KEY;
			pt_mattr->auth_mode = AUTH_MODE_SHARED_KEY;

			Mng_StartAuth(pAdap);
		}
		else
		{	
			Mng_AuthFail(pAdap);
		}

	}
	else
	{
		DBG_WLAN__MLME(LEVEL_INFO,"other frame fc=%04x,do nothing\n",phead->fc); 
	}
}


/*******************************************************************
 *   Mng_Rx_WhenAuthOk
 *   
 *   Descriptions:
 *      the function is called with authok status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
 //update by Justin
VOID Mng_Rx_WhenAuthOk(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;

	PMNG_HEAD80211_T		phead;
	PMNG_ASSOCRSP_DATA_T	pAssocRspData;

	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;
	pAssocRspData = (PMNG_ASSOCRSP_DATA_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);
	
	if(	(UINT8)phead->fc == ASSOCRSP 
	||	(UINT8)phead->fc == REASSOCRSP)
	{
		Mng_ClearTimerPro(pAdap);

		DBG_WLAN__MLME(LEVEL_TRACE,"Receive (Re)Associate Frame when Auth OK. Status=%u, AID=%u, CAP=%04x.\n",
			pAssocRspData->status, pAssocRspData->aid, pAssocRspData->cap);
	
		{
		
			pt_mng->reqRspInfo.ResponseIELength = 0;
			pt_mng->reqRspInfo.AvailableResponseFixedIEs = 0;
			pt_mng->reqRspInfo.rspIe.StatusCode = pAssocRspData->status;
			pt_mng->reqRspInfo.AvailableResponseFixedIEs |= MNDIS_802_11_AI_RESFI_STATUSCODE;
			//CAP
			pt_mng->reqRspInfo.rspIe.Capabilities = pAssocRspData->cap;
			pt_mng->reqRspInfo.AvailableResponseFixedIEs |= MNDIS_802_11_AI_RESFI_CAPABILITIES;
			//AID
			pt_mng->reqRspInfo.rspIe.AssociationId =pAssocRspData->aid;
			pt_mng->reqRspInfo.AvailableResponseFixedIEs |= MNDIS_802_11_AI_RESFI_ASSOCIATIONID;

		//bugs for linksys AP
		//The AP send a association response frame withou AID field after re-connect
		//This will cause driver copy with huge length. then blue screen will happen
			if(pt_mng->rxlen > (WLAN_HDR_A3_LEN +sizeof(MNG_ASSOCRSP_DATA_T)+ DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN))
			{
				//variable //- DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN))	// edit by Justin
				pt_mng->reqRspInfo.ResponseIELength = (UINT32)(pt_mng->rxlen - WLAN_HDR_A3_LEN - sizeof(MNG_ASSOCRSP_DATA_T)
					- DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN);	// edit by Justin again, there is no offset field in ASSOCRSP or REASSOCRSP. 06.11.16
				//reqRspInfo->ResponseIELength =min(reqRspInfo->ResponseIELength,(UINT32)MAX_ASSOC_RSP_NOFIXED_NUM);
				sc_memory_copy(pt_mng->reqRspInfo.RspBuff,
					pt_mng->recbuf + WLAN_HDR_A3_LEN + sizeof(MNG_ASSOCRSP_DATA_T), 
					pt_mng->reqRspInfo.ResponseIELength);
			}
			else 
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"This Association frame is not full \n");
				return;
			}
		}

		
		if( pAssocRspData->status == SUCCESS)    /* ASSOC SUCCESS */
		{
			pt_mng->usedbssdes.aid = pAssocRspData->aid;


			//Mng_WriteBeaconInfoReg(WRITE_SPRATE_REG | WRITE_BEACONINTERVAL_REG | WRITE_CAP_REG | WRITE_SPRATE_REG | WRITE_AID_REG, &pt_mng->usedbssdes, pAdap);

			/* copy current des --> old des for reassociation process */
			sc_memory_copy(&pt_mng->olddes, &pt_mng->middledes, sizeof(MNG_DES_T));
			sc_memory_copy(&pt_mng->middledes, &pt_mng->usedbssdes, sizeof(MNG_DES_T));
			//sc_memory_copy(&pt_mng->olddes, &pt_mng->usedbssdes, sizeof(MNG_DES_T));
			
			//close it for debug ,woody 070424
			//pt_mng->reassocflag = 1;

			pt_mng->statuscontrol.curstatus = ASSOCOK;
			pt_mng->statuscontrol.worktype = 0;

			//Jakio20070528: add for test WPA
			//Adap_Test_Add_WPA(pAdap);
			//Jakio20070606: heart beating mechnism
			pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
			if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
				pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;

			pAdap->bStarveMac = FALSE;
			pAdap->wlan_attr.gdevice_info.tx_disable= FALSE;
			pAdap->must_indicate_uplevel_flag = TRUE;
			
			Mng_SetLinkToUp(pAdap);

			Mng_ExchangeMattrAndMng(WRITE_ALL_BEACONINFO_REG, &pt_mng->usedbssdes, 0, pAdap);
			
			DBG_WLAN__MLME(LEVEL_TRACE,"**********dddddd********\n");
			
			sc_iw_send_bssid_to_up(pAdap->net_dev, pAdap->wlan_attr.bssid);
			
			return;
		}
		else		/* ASSOC Failed */
		{
			Mng_DoAssoc(pAdap);
		}
	}
	else if(phead->fc == DEAUTH)
	{
		//Mng_StartAuth(pAdap);
		Mng_AuthFail(pAdap);
	}
}

/*******************************************************************
 *   tim_process
 *   
 *   Descriptions:
 *      the function is called with assocok status when beacon
 *      frame was receive. It process the tim.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      TRUE:  need to awake the core 
 *		FALSE: do nothing
 ******************************************************************/
BOOLEAN tim_process(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PMNG_NOFIXED_ELE_ALL_T	pele;
	UINT16					offset;
	UINT32					tmplong;
	PUINT8					pdtim_count;
	UINT8					n1,n2;
	BOOLEAN					bFound;

	UINT16 aid = pt_mng->usedbssdes.aid &( ~(BIT14|BIT15));//(UINT8)pt_mattr->gdevice_info.bss_info[pt_mattr->gdevice_info.active_bss].aid;

	bFound = FALSE;
	
	tmplong =  pt_mng->rxlen - WLAN_HDR_A3_LEN - 
			DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN -pt_mng->body_padding_len;
	
	offset = sizeof(BEACON_FIXED_FIELD_T);
	pele = WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);
	
	while( offset < tmplong)
	{
		if(pele->element != TIM_EID)	
		{
			WLAN_MOVE_TO_NEXT_ELE(pele);
			offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pele);
		}
		else   //found tim
		{
			//ptim = (PUINT8)pdata;
			//ptim = (PUINT8)pele;
			bFound = TRUE;
//			len = pdata->length + sizeof(MNG_NOFIXED_ELE_HEAD_T);
			break;
		}
	}  //while end
	
	if (!bFound)	// not found tim
	{
		return FALSE;
	}

	/*
	 * Take the DTIM count and whether it is zero
	 */
	//pdtim_count = ptim + 2;
	pdtim_count = pele->data;

	/*
	* Decode N1 parameter. N1 = (Bitmap Offset * 2).Bitmap Offset is last 7bit of Bitmap Control.
	*/
	n1 = ((UINT8)(*(pdtim_count + 2)) >> 1) * 2;
	
	/*
	* Decode N2 parameter.N2 = (tim_length - 4) + N1. tim_length is (N2 - N1) + 4.
	*/
	//n2 = (((UINT8)pdata->length) - 4) + n1;
	n2 = (((UINT8)pele->length) - 4) + n1;
	
#ifdef MAC_DBG_PS_MODE
    w2lan_printf("======== DTIM count  = %d ========\n", (uint8)(*(tim_p)));
    w2lan_printf("======== DTIM period = %d ========\n", (uint8)(*(tim_p + 1)));
    w2lan_printf("======== our AID     = %d ========\n", aid);
    w2lan_printf("======== N1*8        = %d ========\n", n1*8);
    w2lan_printf("======== (N2+1)*8    = %d ========\n", (n2+1)*8);
    w2lan_printf("======== octet in partial map = 0x%x ========\n", ((uint8)(*(tim_p + 3 + (aid / 8)))));
#endif

	/*
	 * Locate the byte in which,the station's aid present and check the aid bit is set or not.
	 */
	/*
	 * If the staion's aid is not in the range between N1 and N2,the just return.
	 */
	if (!((aid < (n1 * 8)) || (aid >= ((n2 + 1) * 8))))
	{
		if((((UINT8)(*(pdtim_count+3 + (aid/8))))) & Bit(aid%8))
		{
			return TRUE;
		/*
			if(!((pt_mattr->gdevice_info.ps_mode == PSS_ACTIVE) &&
				(pt_mattr->gdevice_info.ps_support == PSS_FAST_DOZE)))
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
			//*/
		}
	}
	
/*	// n1 is the largest, n2 is the smallest
	if ((aid < (n1 * 8)) && (aid >= ((n2 + 1) * 8)))	// aid in range
	{
		UINT8 bit;
		bit = (aid % 8);
		if (((UINT8)(*(pdtim_count + 3 + (aid / 8)))) & (1<<bit))	// there are directed frames buffered for the station
		{
			return TRUE;		
		}
	}
*/
	// at this point we already know our AID is not in the TIM
	if ( (((*(pdtim_count + 2)) & 0x01) == 0) // no broadcast or multicast frames are buffered at the AP
		&& (pt_mattr->gdevice_info.ps_support == PSS_CLASSIC_DOZE) )
		return FALSE;


	if ((UINT16)*pdtim_count == 0)	//this DTIM
	{
		/*
		 * During DTIM zero,if AID0 is present in the Bitmap Control field,then the station
		 * shoud be awake.
		 */
		if (((*(pdtim_count + 2)) & 0x01) == 1)	// broadcast or multicast frames are buffered at the AP
		{
//			pAdapter->gdevice_info.ps_time_stamp = kt_get_time(pAdapter);
//			NdisGetCurrentSystemTime(&pAdapter->gdevice_info.ps_time_stamp);
//			pAdap->gdevice_info.awake_flag = WLAN_TRUE;
			return TRUE;

#ifdef MAC_DBG_PS_MODE
            w2lan_printf("======== AID0_awake ========\n");
#endif
		}
		else if ((((*(pdtim_count + 2)) & 0x01) == 0) 
			&& (pt_mattr->gdevice_info.ps_mode == PSS_CLASSIC_DOZE))
		{
			/*
		 	 * If the tim doesn't contain bcst aid,then just make it doze
		 	 */
//			if ((pAdap->gdevice_info.cfp_flag != TRUE) && 
//				(pAdap->fifo_info[MACHW_CONTENTION_FIFO].tx_que.count == 0) &&
//				(pAdap->fifo_info[MACHW_CONTENTION_FREE_FIFO].tx_que.count ==0))
//			{
//				pAdap->gdevice_info.awake_flag = WLAN_FALSE;
//			}
		}
	}
//	WLAN_TRACE2("%s: %d : decode_tim-return WLAN_FALSE\n",FNAMEGET(__FILE__),__LINE__);
	return FALSE;
}


VOID  Mng_Append_Item_BssList(PDSP_ADAPTER_T pAdap, PMNG_DES_T pdes,PMNG_DESLIST_T plist)
{
	UINT32 i;

	if(plist->bssdesnum < WLAN_BSSDES_NUM)
	{
		for(i = 0; i < plist->bssdesnum; i++)	//search ap in list
		{
			if(IS_MATCH_IN_ADDRESS(plist->bssdesset[i].bssid, pdes->bssid))
			{			
				sc_memory_copy(&plist->bssdesset[i], pdes,sizeof(MNG_DES_T));
				//ap has in list
				return;
			}
		}

		//save old ap 
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng Append AP in nest list \n");
		plist->bssdesnum++; 
		sc_memory_copy(&plist->bssdesset[i], pdes,sizeof(MNG_DES_T));
	}	
}

//justin: 071213		Justin:071213	renew bss list
/*
PMNG_DES_T pdes;		//the bss description that we want add to list
*/
VOID Mng_Renew_BssList(PDSP_ADAPTER_T pAdap, PMNG_DES_T pdes)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
//	UINT32					data = 0;
	UINT32					i;

	BOOLEAN					bFound = FALSE;


		pdes->lifetime = 0;
		/* add current description to list */
		
		for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)	//search ap in list
		{
			if(IS_MATCH_IN_ADDRESS(pt_mng->oiddeslist.bssdesset[i].bssid, pdes->bssid))
			{
				bFound = TRUE;
				break;
			}
		}

		if(bFound)/*  if already exist, renew */
		{
			//DBGSTR_ROAMING(("^^^^^^ROAMING_SUPPORT, renew bss\n"));
			sc_memory_copy(&pt_mng->oiddeslist.bssdesset[i], pdes, sizeof(MNG_DES_T));	
			
			if((pAdap->link_ok == LINK_OK)
				&& (IS_MATCH_IN_ADDRESS(pdes->bssid, pt_mng->usedbssdes.bssid)))	//rssi can not be renew, because it is an average value
			{
				pt_mng->oiddeslist.bssdesset[i].rssi = pt_mng->usedbssdes.rssi;
				
				sc_memory_copy(&pt_mng->usedbssdes, pdes, sizeof(MNG_DES_T));	//update usedbssdes too
				pt_mng->usedbssdes.rssi = pt_mng->oiddeslist.bssdesset[i].rssi;				
			}
		}
		else if(pt_mng->oiddeslist.bssdesnum == WLAN_BSSDES_NUM)	// bss list has full.	
		{//For this case, we discard bbs description which ssid is not equal to the connected AP's ssid
			if(0 == sc_memory_cmp(pdes->ssid, pt_mng->usedbssdes.ssid, pdes->ssid[0]+1))	//same as  the connected AP's ssid, so add to the first place which ssid is not equal to  the connected AP's ssid
			{
				for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)	//search ap in list
				{
					if(!IS_MATCH_IN_ADDRESS(pt_mng->oiddeslist.bssdesset[i].ssid, pt_mng->usedbssdes.ssid))//found first not equal to connected ap's ssid
					{
						DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, replace bss\n");
						sc_memory_copy(&pt_mng->oiddeslist.bssdesset[i], pdes, sizeof(MNG_DES_T));		//replace 
						break;
					}
				}				
			}
		}	
		else//not found in list and the list is not full, so it is to say, we get a newest subscript and need to  add to bss list.
		{
			if(CAP_EXPRESS_IBSS(pdes->cap))
			{
				for (i = 0;i < pt_mng->oiddeslist.bssdesnum;i ++)
				{
					if(0 == sc_memory_cmp(pdes->ssid,pt_mng->oiddeslist.bssdesset[i].ssid,pdes->ssid[0] +1))
					{
						sc_memory_copy(&pt_mng->oiddeslist.bssdesset[i], pdes,sizeof(MNG_DES_T));
						return;			
					}
				}
			}

		
			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, add new bss\n");
			//pt_mng->oiddeslist.bssdesnum = (pt_mng->oiddeslist.bssdesnum < WLAN_BSSDES_NUM) ?(i + 1) : (WLAN_BSSDES_NUM );
			sc_memory_copy(&pt_mng->oiddeslist.bssdesset[pt_mng->oiddeslist.bssdesnum], pdes, sizeof(MNG_DES_T));	
			pt_mng->oiddeslist.bssdesnum ++;
		}


}

/*******************************************************************
 *   Mng_Rx_WhenAssocOk
 *   
 *   Descriptions:
 *      the function is called with assocok status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
// update by Justin
VOID Mng_Rx_WhenAssocOk(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PMNG_HEAD80211_T		phead =(PMNG_HEAD80211_T)pt_mng->recbuf ;
	MNG_DES_T               		des;
//	UINT32					data = 0;
	UINT8 					broadaddr[WLAN_ETHADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};


	//DBG_WLAN__MLME(LEVEL_TRACE,"receive a mgt frame with status of assoc\n"); 
	
	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;
	
	if(phead->fc == DEAUTH || phead->fc == DISASSOC)
	{
		//must make sure the deauth or disassoc sent from the AP we connect with and
		//destination address equal current address or broadcast address
		if(	(0 == sc_memory_cmp(phead->adr.bssrx.bssid, pt_mng->usedbssdes.bssid,WLAN_ETHADDR_LEN)) 
		&&	(	(0 == sc_memory_cmp(phead->adr.bssrx.da,broadaddr,WLAN_ETHADDR_LEN))
			||	(0 == sc_memory_cmp(phead->adr.bssrx.da,pAdap->current_address,WLAN_ETHADDR_LEN))))
		{
				
			DBG_WLAN__MLME(LEVEL_TRACE,"receive a deauth or disassoc mgt frame with status of assoc\n"); 
			DBG_WLAN__MLME(LEVEL_TRACE,"bssid=%02x:%02x:%02x:%02x:%02x:%02x, da=%02x:%02x:%02x:%02x:%02x:%02x, phead->fc=0x%02x.\n",
				phead->adr.bssrx.bssid[0], phead->adr.bssrx.bssid[1], phead->adr.bssrx.bssid[2], 
				phead->adr.bssrx.bssid[3], phead->adr.bssrx.bssid[4], phead->adr.bssrx.bssid[5],
				phead->adr.bssrx.da[0], phead->adr.bssrx.da[1], phead->adr.bssrx.da[2], 
				phead->adr.bssrx.da[3], phead->adr.bssrx.da[4], phead->adr.bssrx.da[5], phead->fc); 		
			Adap_PrintBuffer(pt_mng->recbuf, pt_mng->rxlen);
			Mng_DisConnect(pAdap);
		}
	}
	//beacon change
	else if(DOT11_FRM_IS_BEACON(phead->fc))
	{
		//get new beacon info
		if(!Mng_GetBeaconInfo(&des,pAdap))
		{
			return;
		}
		Mng_BeaconChange(&pt_mng->usedbssdes,&des,pAdap);
		//add phy mode flag into the ap's description
		des.phy_mode = (UINT32)pt_mng->statuscontrol.workflag;
		des.rssi = pt_mng->rssi ;	//Justin: 0626

#ifdef DEBUG_RSSI		
		DBG_WLAN__MLME(LEVEL_TRACE,"rx_when assoc ok rssi = %X\n",des.rssi);
		//curdes.rssi = (long)Phy_GetRssi((PDSP_PHY_INFO_T)pAdap->pphy,rssi);
#endif

//*********begin*****************	Justin:071213	renew bss list
		Mng_Renew_BssList(pAdap, &des);
//**********end*****************	Justin:	renew bss list


		//is not same ssid
		if(0 != sc_memory_cmp(&des.ssid[1], pAdap->wlan_attr.ssid, des.ssid[0]))//pt_mng->usedbssdes.ssid
		{
			return;	
		}	
		if(pAdap->wlan_attr.aid <= 0)//pt_mng->usedbssdes.aid <= 0)	//Justin: 0930.		aid should be a positive number if connected already
		{
			//if(pt_mattr->gdevice_info.ps_mode != PSS_ACTIVE)
			//	Mng_SetPowerActive(pAdap,TRUE);
			
			DBG_WLAN__MLME(LEVEL_TRACE,"receive a beacon frame while aid <=0 with status of assoc\n"); 
			return;
		}

{//justin: 080729.	just disconnect if recv a different akm or cipher beacon 
    NDIS_802_11_AUTHENTICATION_MODE AuthMode;

	if (pAdap->wlan_attr.auth_mode == AUTH_MODE_OPEN)
	{
		AuthMode = Ndis802_11AuthModeOpen;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_SHARED_KEY)
	{
		AuthMode = Ndis802_11AuthModeShared;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA)
	{
		AuthMode = Ndis802_11AuthModeWPA;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK)
	{
		AuthMode = Ndis802_11AuthModeWPAPSK;
	}
	
	#ifdef DSP_WPA2
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2)
	{
		AuthMode = Ndis802_11AuthModeWPA2;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK)
	{
		AuthMode = Ndis802_11AuthModeWPA2PSK;
	}
	#endif
	else
	{
		AuthMode = Ndis802_11AuthModeAutoSwitch;
	}

	// Check the Authmode first
	if (AuthMode >= Ndis802_11AuthModeWPA)//wpa or wpa2
	{
		if((des.offset_wpa == 0xffff)
			&&(des.offset_wpa2 == 0xffff))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"offset_wpa check fail\n");
	             	Mng_DisConnect(pAdap);
			return;
		}
		if(!Mng_CheckAKMAndCipher(AuthMode, &des,pAdap->wlan_attr.wep_mode,pAdap->wlan_attr.group_cipher))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Mng_CheckAKMAndCipher check fail\n");
			Mng_DisConnect(pAdap);
			return;
		}
	}
	else if(  CAP_EXPRESS_PRIVACY(des.cap) != pt_mattr->gdevice_info.privacy_option )	//wep or no encryption
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"wep or no encryption check fail \n");
		Mng_DisConnect(pAdap);
		return;
	}
}

#ifdef ROAMING_SUPPORT11
		//Justin:	1205.	same ssid but not same bssid.....to do roaming or else.....		
		if(!IS_MATCH_IN_ADDRESS(des.bssid, pAdap->wlan_attr.bssid))//pt_mng->usedbssdes.bssid))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, rx_whenassocOK----> same ssid but not same bssid\n");
			if(Mng_roam_threshold_reached(pAdap,&des))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, rx_whenassocOK----> reconnect\n");
				Mng_reconnect(pAdap);
			}
			return;	
		}	
#endif

#if 0	//justin: 20070929		don't renew bssdes , it may clear some usefull info like aid,.....
		des.aid = pt_mng->usedbssdes.aid;
		sc_memory_copy(&pt_mng->usedbssdes,&des,sizeof(MNG_DES_T));
#endif
		//handle erp information
		
		//write para to attr
		//Mng_ExchangeMattrAndMng(WRITE_CAP_REG | WRITE_ERP_ENABLE, &pt_mng->usedbssdes,0,pAdap);
		//Mng_WriteBeaconInfoReg(WRITE_CAP_REG | WRITE_ERP_ENABLE,&pt_mng->usedbssdes, pAdap);
			
		pt_mng->statuscontrol.worktype = 0;


		// zyy : power save mng will be added here
		if ((pt_mattr->gdevice_info.ps_support == PSS_ACTIVE) //not support ps
			||(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA))	//not support ps in ibss mode
		{
			return;
		}

		//update cts to self state
//		Mng_Update_Cts_State(pAdap,(CTS_SELF_ENABLE == des.Erpflag) ? des.Erpinfo : 0);
		/*
		The More Data field is valid in directed data or management type frames transmitted by an AP to an STA in power-save mode.
		A value of 1 indicates that at least one additional buffered MSDU, or MMPDU, is present for the same STA.*/
		if (1)//WLAN_GET_FC_FROMDS(phead->fc))				// from ds to sta directly
		{
			if(1)//WLAN_GET_FC_MOREDATA(phead->fc) == 1)		// more data = 1
			{
				// control.wakeupDTIM: Wakeup DTIM interval.  When set in DOZE state, the core wakes up every DTIM beacons.
				// if we received tim, the core auto  wakes up, what we have to do is tell ap that status through send a ps-poll frame
				if (tim_process(pAdap))	// tim received
				{
					//if (pt_mattr->gdevice_info.ps_mode != PSS_ACTIVE)		// not work in active mode
					{
						if(Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE))
						{
							Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE);
						}

						Mng_SetPowerActive(pAdap,TRUE);
					/*
						if(Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SET_POWER_SAVE))
						{
							Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SET_POWER_SAVE);
						}
						//Mng_SetPowerActive(pAdap);
						else if (!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SET_POWER_ACTIVE))
						{//para = 1	-> active;			para = 0  -> save
							UINT8 para = 1;
							Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_ACTIVE,DSP_TASK_PRI_NORMAL,&para,1);
						}
						//*/
//						Adap_Transmit_Pspoll(pAdap);
//						if (pAdap->gdevice_info.ps_mode != PSS_FAST_DOZE)
//						{
//						//In ps_poll,the device is still in power save mode,so we will always set powmgt bit
//							send_frame(pAdap, PS_POLL);
//						}
//						else
//						{
//						/* During fastdoze,the device temporarily is going to active,so we will inform to ap
//						* device is already in active state,so we no need to program active again.*/
//							Vcmd_reset_power_mng(pAdap);
//							send_frame(pAdap, NULL_FRM);
//							//			pAdapter->gdevice_info.ps_time_stamp = kt_get_time(pAdapter);
//							NdisGetCurrentSystemTime(&pAdap->gdevice_info.ps_time_stamp);
//						}
						
					}
				}
				else//	&&(WLAN_GET_FC_MOREDATA(phead80211->fc) == 0)// more data = 0
				{	
					//DBGSTR_POWER_MNG(("receive not a tim, ps supp = %d, curr ps = %d \n",pt_mattr->gdevice_info.ps_support,pt_mattr->gdevice_info.ps_mode)); 
					
					if(!Tx_DataFrmlist_IsEmpty(pAdap)
					|| !Tx_MngFrmlist_IsEmpty(pAdap)//more packet need to send
					|| pAdap->more_data)//more packet need to recv
					{
					#if 1
						//DBGSTR_POWER_MNG(("receive not a tim, active \n"));
						if(Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE))
						{
							Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE);
						}
						
						Mng_SetPowerActive(pAdap,FALSE);
					#endif
					}
					else
					{
						if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE))
						{
		      					Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE,DSP_TASK_PRI_NORMAL,NULL,0);
						}
						//Mng_SetPowerSave(pAdap);
						//DBGSTR_POWER_MNG(("receive not a tim, save \n"));
					}
	/*		
					//if ((pt_mattr->gdevice_info.ps_mode == PSS_ACTIVE)		// work in active mode
					if(PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->ptx_packet_list)
						&&PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->pmng_queue_list))//no more packet need to send
					{
						//Mng_SetPowerSave(pAdap);
						if (!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SET_POWER_SAVE))
						{//para = 1	-> active;			para = 0  -> save
							UINT8 para = 0;
							Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE,DSP_TASK_PRI_NORMAL,&para,1);
						}
								
					}
	//*/
				}

			}
			#if 0
			else//	&&(WLAN_GET_FC_MOREDATA(phead80211->fc) == 0)// more data = 0
			{	//zyy: if more data = 0, sta will enter power save mode.
				//if ((pt_mattr->gdevice_info.ps_mode == PSS_ACTIVE)		// work in active mode
				if(PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->ptx_packet_list)
					&&PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->pmng_queue_list))//no more packet need to send
				{
					//Mng_SetPowerSave(pAdap);
					if (!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SET_POWER_SAVE))
					{//para = 1	-> active;			para = 0  -> save
						UINT8 para = 0;
						Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_SAVE,DSP_TASK_PRI_NORMAL,&para,1);
					}
							
				}
			}
			#endif
		}
		
	}
	else
	{		
		DBG_WLAN__MLME(LEVEL_INFO,"Receive a mgt frame with status of Associate OK. FC=%04x.\n", phead->fc); 
	}
}

/*******************************************************************
 *   Mng_DoAuth
 *   
 *   Descriptions:
 *      execute authentication operation, include auth1 frame and so on
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_DoAuth(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T			pt_mattr = &pAdap->wlan_attr;
//	TDSP_STATUS		status;
	
	
	//no auth with ibss	
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_DoAuth return with ibss \n");
		return;
	}

	//09.Feb.10th, add by wumin, for dlink 624 sheard key bug
	if(	pt_mng->statuscontrol.retrynum*2 >= MAXAUTHNUM
	&&	pt_mattr->auth_mode == AUTH_ALG_OPEN
	&&	pt_mattr->gdevice_info.privacy_option)
	{
		pt_mattr->auth_alg = AUTH_ALG_SHARED_KEY;
		pt_mattr->auth_mode = AUTH_MODE_SHARED_KEY;

		Mng_StartAuth(pAdap);

		return;
	}
	//end of 09.Feb.10th, add by wumin, for dlink 624 sheard key bug

		
	//auth fail with retry num reach
	if(pt_mng->statuscontrol.retrynum >= MAXAUTHNUM)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"auth num > max ,so Mng_AuthFail.\n"); 
		Mng_AuthFail(pAdap);

		return;
	}

//	if(pt_mng->statuscontrol.worktype == WAIT_AUTH4)
//	{
//		if(STATUS_SUCCESS != Mng_MakeAuthFrame3(pAdap))
//		{
//			DBG_WLAN__MLME(LEVEL_TRACE,"send auth3 fail \n");		
//		}
//		DBG_WLAN__MLME(LEVEL_TRACE,"send a auth3,and wait auth4\n"); 
//	}
//	else
	{		
		pAdap->bStarveMac = TRUE;
		pt_mng->statuscontrol.worktype = WAIT_AUTH2;
		if(STATUS_SUCCESS != Mng_MakeAuthFrame1(pAdap))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"send auth1 fail \n");		
		}
		DBG_WLAN__MLME(LEVEL_TRACE,"send a auth1,and wait auth2\n"); 
	}

	
	Mng_SetTimerPro(pt_mattr->auth_timeout, pAdap);
	//pt_mng->statuscontrol.workcount++;
	pt_mng->statuscontrol.retrynum++;
	
}

/*******************************************************************
 *   Mng_MakeAuthFrame3
 *   
 *   Descriptions:
 *      build up auth3 frame 
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeAuthFrame3(PDSP_ADAPTER_T pAdap)
{
#if 1/*Now I don't know how to do with encrption for the challenge text*/	
	PMNG_STRU_T     				pt_mng = pAdap->pmanagment;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;
	PMNG_AUTHFRAME_DATA_T		pAuthData;
	PMNG_NOFIXED_ELE_ALL_T		pchallenge;
	UINT8 *addr1 = NULL;
	UINT8 *addr3 = NULL;

	addr1 = addr3 = pt_mng->usedbssdes.addr;
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)pt_mng->tranbuf;

	//Auth data body
	pAuthData = (PMNG_AUTHFRAME_DATA_T)WLAN_HDR_A3_DATAP(
		pt_mng->tranbuf );//+ sizeof(USBWLAN_HW_FRMHDR_TX));
	
	pAuthData->authalg = AUTH_ALG_SHARED_KEY;
	pAuthData->authseq = 0x0003;
	pAuthData->status = 0x0000;
	//offset += sizeof(MNG_AUTHFRAME_DATA_T);

	pchallenge = WLAN_CONVERT_FRAME_NOFIXED_ELE(
		(PUINT8)pAuthData + sizeof(MNG_AUTHFRAME_DATA_T));
	pchallenge->element = CHALLENGE_EID;
	pchallenge->length = CHALLENGE_TEXT_LEN;
	//copy the challenge text 
	sc_memory_copy(pchallenge->data, pt_mng->rec_challenge_text, CHALLENGE_TEXT_LEN);
	WLAN_MOVE_TO_NEXT_ELE(pchallenge);

	//get length of 3dsp tx packet
	pt_mng->txlen = (PUINT8)pchallenge - pt_mng->tranbuf;

	//FILL some common info of hw header
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, WLAN_FSTYPE_AUTHEN, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 1);
	//fill addr1 and addr3
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
        	hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
        	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
        	hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);
	
#endif	
	
	DBG_PRINT_BUFF("Send auth3 Frame:",pt_mng->tranbuf,pt_mng->txlen);

	return Mng_SendPacket(pt_mng->tranbuf,pt_mng->txlen,pAdap);
}

/*******************************************************************
 *   Mng_StartAssoc
 *   
 *   Descriptions:
 *      start association process after authencation was success.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_StartAssoc(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	pt_mng->statuscontrol.curstatus = AUTHOK;
	pt_mng->statuscontrol.retrynum = 0;

	Mng_DoAssoc(pAdap);

	DBG_WLAN__MLME(LEVEL_TRACE,"start asoc process\n"); 
}
/*******************************************************************
 *   Mng_MakeAuthFrame1
 *   
 *   Descriptions:
 *      build up auth1 frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeAuthFrame1(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T           			pt_mattr = &pAdap->wlan_attr;
	PMNG_AUTHFRAME_DATA_T   	pAuthData;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;
	UINT8 *addr1 = NULL;
	UINT8 *addr3 = NULL;
	


	//mng frame, dst and receiving addr is same
	//addr1 = addr3 = pt_mattr->bssid;
	/*Jakio: I have no idea about which var saves the current AP address*/
	addr1 = addr3 = pt_mng->usedbssdes.addr;
	
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)pt_mng->tranbuf;
	pAuthData =  (PMNG_AUTHFRAME_DATA_T)WLAN_LOCATE_3DSP_TX_PAYLOAD(pt_mng->tranbuf);

	Mng_fill_hw_header( pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT,WLAN_FSTYPE_AUTHEN,
				sizeof(MNG_AUTHFRAME_DATA_T), 0);
	//FILL addresses
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
        	hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) |
             				(addr1[3] << 8)  | addr1[2]);
        	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) |
             				(addr3[1] << 8)  | addr3[0]);
        	hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

	/*body*/
	pAuthData->authalg = pt_mattr->auth_alg;      /* auth arg */
	pAuthData->authseq = 0x0001;
	pAuthData->status = 0x0000;

	//DBG_WLAN__MLME(LEVEL_TRACE,"pt_mattr->auth_alg = %d\n",pt_mattr->auth_alg);
	
	/*frame length*/
	pt_mng->txlen = DSP_TX_HEAD_LENGTH + sizeof(MNG_AUTHFRAME_DATA_T);
//	pt_mng->txlen = (pt_mng->txlen + 3) & 0xfffc;

	// add by jason 2007.9.7 begin
#ifdef PMK_CACHING_SUPPORT		//justin:	071224	move to add_key_oid, just like rt61_linux codes //justin: 080822. done here is better

	if ((pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2) && (pt_mattr->macmode == WLAN_MACMODE_ESS_STA))
	{
		NDIS_802_11_STATUS_TYPE*  type_p;
		NDIS_802_11_PMKID_CANDIDATE_LIST* req_p;

		type_p = (NDIS_802_11_STATUS_TYPE* ) pAdap->pmkCandidateBuf;
		*type_p = Ndis802_11StatusType_PMKID_CandidateList;
		req_p = (NDIS_802_11_PMKID_CANDIDATE_LIST* ) 
			&(pAdap->pmkCandidateBuf[sizeof(NDIS_802_11_STATUS_TYPE)]);

		req_p->Version = 1;
		req_p->NumCandidates = 1;
		req_p->CandidateList[0].Flags = NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED;// 0; justin:	080822.   in rt61 is 0
		sc_memory_copy(&req_p->CandidateList[0].BSSID, pt_mng->usedbssdes.bssid, sizeof(NDIS_802_11_MAC_ADDRESS));
		DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: Indicate status, total num = %d, bssid = %02x %02x %02x %02x %02x %02x\n",
			req_p->NumCandidates,
			req_p->CandidateList[0].BSSID[0],req_p->CandidateList[0].BSSID[1],req_p->CandidateList[0].BSSID[2],
			req_p->CandidateList[0].BSSID[3],req_p->CandidateList[0].BSSID[4],req_p->CandidateList[0].BSSID[5]);
		
		if((pAdap->reconnect_status != NO_RECONNECT)
			&&(0 != sc_memory_cmp(pt_mng->usedbssdes.bssid,pAdap->oldBssid,WLAN_ETHADDR_LEN))
			)//if (pAdap->g_sme.reAssociate == WLAN_TRUE)//justin:	080822.	do reconnect.
		{
			req_p->NumCandidates++;
			req_p->CandidateList[1].Flags = NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED;
			sc_memory_copy(&req_p->CandidateList[1].BSSID, pAdap->oldBssid, sizeof(NDIS_802_11_MAC_ADDRESS));
		}
		
//wumin					NdisMIndicateStatus(pAdap->dsp_adap_handle,
//wumin												TDSP_STATUS_MEDIA_SPECIFIC_INDICATION,
//wumin												(PVOID) pAdap->pmkCandidateBuf,
//wumin												PMK_CAND_BUF_LEN);
		// NOTE: have to indicate status complete every time you indicate status
//wumin						NdisMIndicateStatusComplete(pAdap->dsp_adap_handle);

	}

#endif 

#if 0
	DBG_WLAN__MLME(LEVEL_TRACE,("***********************\n"));
	DBG_WLAN__MLME(LEVEL_TRACE,("make auth  ssid len = 0x%02X\n",pt_mng->usedbssdes.ssid[0]));
	//Adap_PrintBuffer(pSsidStruct->Ssid,pSsidStruct->SsidLength);
	DBG_WLAN__MLME(LEVEL_TRACE,("ssid = %02x %02x %02x %02x %02x %02x %02x %02x\n",
			pt_mng->usedbssdes.ssid[0], pt_mng->usedbssdes.ssid[1], pt_mng->usedbssdes.ssid[2],pt_mng->usedbssdes.ssid[3],
			pt_mng->usedbssdes.ssid[4], pt_mng->usedbssdes.ssid[5],pt_mng->usedbssdes.ssid[6],pt_mng->usedbssdes.ssid[7]));
	DBG_WLAN__MLME(LEVEL_TRACE,("***********************\n"));
#endif

	DBG_PRINT_BUFF("Send auth1 Frame:",pt_mng->tranbuf,pt_mng->txlen);

	return Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
}


BOOLEAN Mng_AssocRetrylimit(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	return (pt_mng->statuscontrol.retrynum >= MAXASOCNUM)? TRUE:FALSE;
}
/*******************************************************************
 *   Mng_DoAssoc
 *   
 *   Descriptions:
 *      begin association process, include association request frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_DoAssoc(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	TDSP_STATUS  status;
	
	//ibss has not this case 	
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		//note that. should add for this dead state later
		DBG_WLAN__MLME(LEVEL_TRACE,"Do associate flow in IBSS mode, Jost Wait\n");
		return;
	}	
	
	
	if( pt_mng->statuscontrol.retrynum >= MAXASOCNUM)
	{
		/* temp variableNPDEBUG("asoc num reach max,redo scan"); */
		/* temp variableMng_StartScan(SCANNING,pAdap); */
		Mng_AuthFail(pAdap);
		return;
	}

	//for re-assoc
	if(pt_mng->reassocflag == 1)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"May Reassociate.\n");
		
		if(	IS_MATCH_IN_SSID(SSID_EID,pt_mng->usedbssdes.ssid[0],pt_mng->usedbssdes.ssid + 1,SSID_EID,pt_mng->olddes.ssid[0], pt_mng->olddes.ssid+1) 
		&&	!IS_MATCH_IN_ADDRESS(&pt_mng->usedbssdes.bssid, pt_mng->olddes.bssid))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Send Reassociate Frame, Same SSID but Different BSSID.\n");
			status = Mng_MakeReAsocReqFrame(pAdap);
		}
		else
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Can't Reassociate, Send Associate Frame.\n");
			status = Mng_MakeAssocReqFrame(pAdap);
		}
	}
	//for assoc
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Send Associate Frame.\n");
		status = Mng_MakeAssocReqFrame(pAdap);
	}

	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__MLME(LEVEL_ERR,"Send (Re)Associate Frame Failure.\n");
	}

	//set timer
	Mng_SetTimerPro(pt_mattr->asoc_timeout,pAdap);
	
	pt_mng->statuscontrol.retrynum++; 
}

/*******************************************************************
 *   Mng_MakeDisAssocFrame
 *   
 *   Descriptions:
 *      build up association request frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_DisAssocFrame(PDSP_ADAPTER_T pAdap,PUINT8 disassoc_bssid,UINT16 reason)
{	
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T					pt_mattr = &pAdap->wlan_attr;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;
	PMNG_DISASSOC_DATA_T		disassoc_data;
	PUINT8				addr1 = NULL;
	PUINT8				addr3 = NULL;
	UINT8 				invalid_addr[] = {0,0,0,0,0,0};
	TDSP_STATUS  status;


	/*hw header pos*/
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)pt_mng->tranbuf;
	//offset = sizeof(USBWLAN_HW_FRMHDR_TX);

	disassoc_data = (PMNG_DISASSOC_DATA_T)WLAN_LOCATE_3DSP_TX_PAYLOAD(pt_mng->tranbuf);
	disassoc_data->reason = reason;
	pt_mng->txlen = DSP_TX_HEAD_LENGTH + sizeof(MNG_DISASSOC_DATA_T);

	/*fill common info of hw header*/
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, WLAN_FSTYPE_DISASSOC, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 0);
	//fill addr1 and addr3
	if (0 == sc_memory_cmp(disassoc_bssid, invalid_addr, ETH_LENGTH_OF_ADDRESS))
		addr1 = addr3 = pAdap->wlan_attr.bssid;
	else
		addr1 = addr3 = disassoc_bssid;	//justin:1205.	a specific BSSID to be disassociate
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
        	hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
        	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
        	hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);
	status = Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
	DBG_WLAN__MLME(LEVEL_TRACE,"disassoc status = %x \n",status);
}

#ifdef PMK_CACHING_SUPPORT
BOOLEAN Mng_GetRSNCapabilities(
///     NDIS_802_11_AUTHENTICATION_MODE auth, 
     PUINT16		pRSNCap,
     MNG_DES_T *pBss)
{
    PUINT8 pwpaIe = pBss->IEs + pBss->offset_wpa;
    PUINT8 pwpa2Ie = pBss->IEs + pBss->offset_wpa2;
    PUINT8 pRSNStartAddr;	
    PUINT8 pPWkey_count;
    PUINT8 pAKM_count;
//    PAKM_SUITE_STRUCT pAKM;

	//Justin: 071214		for wpa, must skip 4 BYTEs after length.	that is to say, skip the bytes between length and version
    pRSNStartAddr = (pBss->offset_wpa != 0xFFFF) ? (pwpaIe) : ((pBss->offset_wpa2 != 0xFFFF)?pwpa2Ie:0);

	if(pRSNStartAddr == 0)
		return FALSE;
	
    pPWkey_count = pRSNStartAddr+sizeof(RSN_IE_HEADER_STRUCT)+sizeof(CIPHER_SUITE_STRUCT);
    if(pBss->offset_wpa != 0xFFFF)	//wpa mode , do skip
	pPWkey_count += 4;		//pPWkey_count is pushort, so just add 2 for 4 bytes

	pAKM_count = pPWkey_count 		   + sizeof(UINT16)         +(*((PUINT16)pPWkey_count) *sizeof(CIPHER_SUITE_STRUCT)) ;
//    pAKM	= (PAKM_SUITE_STRUCT)(pPWkey_count 		   + sizeof(UINT16)         +(*((PUINT16)pPWkey_count) *sizeof(CIPHER_SUITE_STRUCT)) + sizeof(UINT16));
    //						   pairwisekey count start  + skip pw count            + skip pwkeys                                                              +  Skip akm count	//

	*pRSNCap = *(PUINT16)(pAKM_count + sizeof(UINT16) + (*((PUINT16)pAKM_count) *sizeof(AKM_SUITE_STRUCT)));

	return TRUE;
}
#endif	//#ifdef PMK_CACHING_SUPPORT

/*******************************************************************
 *   Mng_MakeAssocReqFrame
 *   
 *   Descriptions:
 *      build up association request frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeAssocReqFrame(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T					pt_mattr = &pAdap->wlan_attr;
	PMNG_ASSOCREQ_DATA_T		pAssocData;
	PUSBWLAN_HW_FRMHDR_TX  	hw_header;
	PMNG_NOFIXED_ELE_ALL_T 		pele;
	PMNG_NOFIXED_ELE_ALL_T		pWpaIe;
	PMNG_NOFIXED_ELE_ALL_T		pWpa2Ie;
	PMNG_RSNFIXED_T			prsn;   //Jakio20070530: add here for fill wpa element
	PMNG_WPA2_RSNFIXED_T		pwpa2_rsn;	//Jakio20070629: add for fill wpa2 element
	PMNG_RSN_SUITE_T			psuite;
	PMNG_RSN_COUNT_T			pcount;
	PUINT16					pCap;
	PUINT16 					pPMKID_Count;
	UINT8 *addr1 = NULL;
	UINT8 *addr3 = NULL;
	UINT8 offset = 0; //Jakio20070530: add for fill wpa element
	//UINT16						usRSNCap;

	//Jakio20070702: init group cipher mode
	pt_mattr->group_cipher = pt_mattr->wep_mode;


	hw_header = (PUSBWLAN_HW_FRMHDR_TX)(pt_mng->tranbuf);

	if(pt_mng->usedbssdes.offset_wpa < pt_mng->usedbssdes.IELength)
		pWpaIe = (PMNG_NOFIXED_ELE_ALL_T)(pt_mng->usedbssdes.IEs + pt_mng->usedbssdes.offset_wpa);
	else
		pWpaIe = NULL;
	
	if(pt_mng->usedbssdes.offset_wpa2 < pt_mng->usedbssdes.IELength)
		pWpa2Ie = (PMNG_NOFIXED_ELE_ALL_T)(pt_mng->usedbssdes.IEs + pt_mng->usedbssdes.offset_wpa2);
	else
		pWpa2Ie = NULL;

	/*assoc body*/
	pAssocData = (PMNG_ASSOCREQ_DATA_T)WLAN_LOCATE_3DSP_TX_PAYLOAD(pt_mng->tranbuf);
	pAssocData->cap = pt_mng->usedbssdes.cap & (~BIT11);//justin: 1127.	mask rsn enable bit
	DBG_WLAN__MLME(LEVEL_TRACE,"Assoc Rssi = %x!!\n", pt_mng->usedbssdes.rssi);
	if(GetRssiMsb(pt_mng->usedbssdes.rssi))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Dlink DI624 AP !!\n");
		pAdap->wlan_attr.gdevice_info.preamble_type = LONG_PRE_AMBLE;
		pAssocData->cap &=((UINT16) (~WLAN_CAP_BIT_SHORT_PREAMBLE));		
	}
	pAssocData->listenInt = pt_mattr->listen_interval;


	/*write ssid*/
	pele = WLAN_CONVERT_FRAME_NOFIXED_ELE(
		(PUINT8)pAssocData + sizeof(MNG_ASSOCREQ_DATA_T));

	WriteElement(SSID_EID, pele, (PUINT8)&pt_mng->usedbssdes.ssid[1], pt_mng->usedbssdes.ssid[0]);
	WLAN_MOVE_TO_NEXT_ELE(pele);

	/*support rate*/
	WriteElement(SUPRATE_EID, pele, (PUINT8)&pt_mng->usedbssdes.suprate[1], pt_mng->usedbssdes.suprate[0]);
	WLAN_MOVE_TO_NEXT_ELE(pele);

	/*extended eid*/
	//if(pt_mattr->wlan_802_11_mode == WLAN_802_11_MODE_11G)
	if(pt_mattr->gdevice_info.dot11_type == IEEE802_11_G)
	{
		if(pt_mng->usedbssdes.ExtendedSupportRateLen != 0)
		{
			WriteElement(EXTENDRATE_EID, pele, (PUINT8)&pt_mng->usedbssdes.ExtendedSupportRate,pt_mng->usedbssdes.ExtendedSupportRateLen);
			WLAN_MOVE_TO_NEXT_ELE(pele);	
		}
	}

	//Jakio2006.11.28: moved codes below


	if(((pt_mattr->auth_mode == AUTH_MODE_WPA) || 
		(pt_mattr->auth_mode == AUTH_MODE_WPA_PSK) ||
		(pt_mattr->auth_mode == AUTH_MODE_WPA2) ||
		(pt_mattr->auth_mode == AUTH_MODE_WPA2_PSK)) &&
		((pWpaIe != NULL) || (pWpa2Ie != NULL)))
	{
		//Jakio20070626
		//record wpa info
		{
			pt_mng->reqRspInfo.RequestIELength = 0;
			pt_mng->reqRspInfo.AvailableRequestFixedIEs = (UINT16)0;
			pt_mng->reqRspInfo.reqIe.Capabilities = pt_mng->usedbssdes.cap;
			pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_CAPABILITIES;
			pt_mng->reqRspInfo.reqIe.ListenInterval = pt_mattr->listen_interval;
			pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_LISTENINTERVAL;
			//pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_CURRENTAPADDRESS;
		}
		//fill the wpa1 element
		if((pt_mattr->auth_mode == AUTH_MODE_WPA) || 
			(pt_mattr->auth_mode == AUTH_MODE_WPA_PSK))
		{
			if(pWpaIe == NULL)	//Justin:	080221.	maybe change from wpa to wpa2 while connected. we should fail to auto connect ap...
				return STATUS_FAILURE;
			pele->element = WPA1_EID;
		}
		else
		{
			if(pWpa2Ie == NULL)	//Justin:	080221.	maybe change from wpa2 to wpa while connected. we should fail to auto connect ap...
				return STATUS_FAILURE;

			pele->element = WPA2_EID;
		}
		offset = 0;

		pAdap->wpa_1x_no_encrypt = 0;
		if(pele->element == WPA1_EID)
		{
			//get group cipher type
			if(!Mng_WpaGroupCipherMode(pAdap, (PUINT8)pWpaIe, &pt_mattr->group_cipher))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Failure to match group cipher mode\n");
				pt_mattr->group_cipher = pt_mattr->wep_mode;
			}
		
			/*wpa1 element body*/
			//fixed data
			prsn = (PMNG_RSNFIXED_T)(pele->data);
			prsn->oui = RSNOUI;
			prsn->version = RSNVERSION;
			//if(pt_mattr->wep_mode == WEP_MODE_TKIP)
			if(pt_mattr->group_cipher == WEP_MODE_TKIP)
				prsn->groupSuite = KEY_SUITE_TKIP;
			else
				prsn->groupSuite = KEY_SUITE_CCMP;
			offset += sizeof(MNG_RSNFIXED_T);

			//unicast len
			pcount = (PMNG_RSN_COUNT_T)(pele->data + offset);
			pcount->count = 0x0001;
			offset += sizeof(MNG_RSN_COUNT_T);

			//unicast suite
			psuite = (PMNG_RSN_SUITE_T)(pele->data + offset);
			if(pt_mattr->wep_mode == WEP_MODE_TKIP)
				psuite->suite = KEY_SUITE_TKIP;
			else
				psuite->suite = KEY_SUITE_CCMP;
			offset += sizeof(MNG_RSN_SUITE_T);

			//auth len
			pcount = (PMNG_RSN_COUNT_T)(pele->data + offset);
			pcount->count = 0x0001;
			offset += sizeof(MNG_RSN_COUNT_T);

			//auth suite
			psuite = (PMNG_RSN_SUITE_T)(pele->data + offset);
			if(pt_mattr->auth_mode == AUTH_MODE_WPA)
				psuite->suite = AUTH_SUITE_RSN;
			else
			psuite->suite = AUTH_SUITE_RSN_PSK;
			offset += sizeof(MNG_RSN_SUITE_T);

			pele->length = offset;
		}
		else//WPA2_EID
		{

			//sc_memory_copy(pele->data, pWpa2Ie->data, pWpa2Ie->length);
			//pele->length = pWpa2Ie->length;

			//get group cipher type
			if(FALSE == Mng_Wpa2GroupCipherMode(pAdap, (PUINT8)pWpa2Ie, &pt_mattr->group_cipher))
			{
				DBG_WLAN__MLME( LEVEL_TRACE,"Failure to match group cipher mode\n");
				pt_mattr->group_cipher = pt_mattr->wep_mode;
			}
			
			#if 1  //close for test
			/*element body*/
			//fixed data
			pwpa2_rsn = (PMNG_WPA2_RSNFIXED_T)(pele->data);
			pwpa2_rsn->version = RSNVERSION;
			if(pt_mattr->group_cipher == WEP_MODE_TKIP)
				pwpa2_rsn->groupSuite = KEY_SUITE_TKIP_WPA2;
			else
				pwpa2_rsn->groupSuite = KEY_SUITE_CCMP_WPA2;
			offset += sizeof(MNG_WPA2_RSNFIXED_T);

			//unicast len
			pcount = (PMNG_RSN_COUNT_T)(pele->data + offset);
			pcount->count = 0x0001;
			offset += sizeof(MNG_RSN_COUNT_T);

			//unicast suite
			psuite = (PMNG_RSN_SUITE_T)(pele->data + offset);
			if(pt_mattr->wep_mode == WEP_MODE_TKIP)
				psuite->suite = KEY_SUITE_TKIP_WPA2;
			else
				psuite->suite = KEY_SUITE_CCMP_WPA2;
			offset += sizeof(MNG_RSN_SUITE_T);

			//auth len
			pcount = (PMNG_RSN_COUNT_T)(pele->data + offset);
			pcount->count = 0x0001;
			offset += sizeof(MNG_RSN_COUNT_T);

			//auth suite
			psuite = (PMNG_RSN_SUITE_T)(pele->data + offset);
			if(pt_mattr->auth_mode == AUTH_MODE_WPA2)
				psuite->suite = AUTH_SUITE_RSN_WPA2;
			else
				psuite->suite = AUTH_SUITE_RSN_PSK_WPA2;
			offset += sizeof(MNG_RSN_SUITE_T);

			//capability//justin: 1127.	must add this item in wpa2 mode
			//Mng_GetRSNCapabilities(&usRSNCap, &pt_mng->usedbssdes);
			pCap = (PUINT16)(pele->data + offset);
			*pCap = 0x0000; //no capbilities		usRSNCap;
			offset += sizeof(UINT16);		

				
				// add by jason 2007.9.10
				#ifdef PMK_CACHING_SUPPORT
				//	if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2)
					{
						if ((pAdap->pmk_id_caching.Length != 0) && (pAdap->pmk_id_caching.BSSIDInfoCount != 0))
						{//justin:	080901.	check old connected AP's PMKid first
							UINT8 j;

							DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: WPA2 Add PMKID feild, length = %d, bssidinfocount = %d\n",
								pAdap->pmk_id_caching.Length, pAdap->pmk_id_caching.BSSIDInfoCount);
							for (j = 0; j < pAdap->pmk_id_caching.BSSIDInfoCount; j++)
							{
								if (0 == sc_memory_cmp(&pAdap->pmk_id_caching.BSSIDInfo[j].BSSID,pAdap->oldBssid , 6))
								{
									break;
								}
							}
							if (j < pAdap->pmk_id_caching.BSSIDInfoCount)
							{
								DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: Find Bssid index = %d\n", j);

								/*
								//capability//justin: 1127.	must add this item in wpa2 mode
								pCap = (PUINT16)(pele->data + offset - sizeof(UINT16));	//get capability pointer
								*pCap = 0x0000;	//0x0001	 //set bit 1 for preauthentication
								//offset += sizeof(UINT16);	
								//*/
								

								// PMKID Count, only support one pmkid
								//pAdap->newBestAP.alternatepWPA_IEdata[22] = 1;
								//pAdap->newBestAP.alternatepWPA_IEdata[23] = 0;
								
								pPMKID_Count = (PUINT16)(pele->data + offset);
								*pPMKID_Count = 0x0001; 			// count = 1;
								offset += sizeof(UINT16);


								// PMKID
								sc_memory_copy((PUINT8)(pele->data + offset), &pAdap->pmk_id_caching.BSSIDInfo[j].PMKID, sizeof(NDIS_802_11_PMKID_VALUE));
								offset += sizeof(NDIS_802_11_PMKID_VALUE);

								/*if (pAdap->newBestAP.alternatepWPA_IEdata[1] == pAdap->g_sme.newBestAP.wpa2_rsn_len)
								{
									// Change Length Again
									pAdap->newBestAP.alternatepWPA_IEdata[1] += (2 + sizeof(NDIS_802_11_PMKID_VALUE));
								}*/
							}
							else		//can not found pmkid for new roaming bssid
							{
								//justin:	080822.	try old bssid
								UINT8 j;
								DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: WPA2 Add PMKID feild, try old bssid \n");

								for (j = 0; j < pAdap->pmk_id_caching.BSSIDInfoCount; j++)
								{
									if (0 == sc_memory_cmp(&pAdap->pmk_id_caching.BSSIDInfo[j].BSSID,pt_mng->usedbssdes.bssid , 6))
									{
										break;
									}
								}
								if (j < pAdap->pmk_id_caching.BSSIDInfoCount)
								{
									DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: Find old Bssid index = %d\n", j);

									pPMKID_Count = (PUINT16)(pele->data + offset);
									*pPMKID_Count = 0x0001; 			// count = 1;
									offset += sizeof(UINT16);

									// PMKID
									sc_memory_copy((PUINT8)(pele->data + offset), &pAdap->pmk_id_caching.BSSIDInfo[j].PMKID, sizeof(NDIS_802_11_PMKID_VALUE));
									offset += sizeof(NDIS_802_11_PMKID_VALUE);
								}
								else
								{
									DBG_WLAN__MLME(LEVEL_TRACE,"PMK Caching: Can not Find any Bssid\n");
								}
							}
						}
					}
				#endif

			
			pele->length = offset;
			#endif
		}
		

		//adjust element position
		WLAN_MOVE_TO_NEXT_ELE(pele);
		//record for wpa info
		{
			//pt_mng->reqRspInfo.RequestIELength = (UINT32)(offset - sizeof(MNG_ASSOCREQ_DATA_T) - WLAN_HDR_A3_LEN);
			pt_mng->reqRspInfo.RequestIELength = ((PUINT8)pele - pt_mng->tranbuf) - sizeof(MNG_ASSOCREQ_DATA_T) - DSP_TX_HEAD_LENGTH;
			pt_mng->reqRspInfo.RequestIELength = wlan_min(pt_mng->reqRspInfo.RequestIELength, MAX_ASSOC_REQ_NOFIXED_NUM);
			sc_memory_copy(pt_mng->reqRspInfo.ReqBuff, pt_mng->tranbuf + sizeof(MNG_ASSOCREQ_DATA_T) + DSP_TX_HEAD_LENGTH ,pt_mng->reqRspInfo.RequestIELength);
		}
		
	}
	
	//get total length
	pt_mng->txlen = (PUINT8)pele - pt_mng->tranbuf;

	//Jakio2006.11.28: moved here
	/*fill common info of hw header*/
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, WLAN_FSTYPE_ASSOCREQ, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 0);
	//fill addr1 and addr3
	addr1 = addr3 = pt_mng->usedbssdes.addr;
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
       hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
       hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
       hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

	DBG_PRINT_BUFF("Send Associate Frame:",pt_mng->tranbuf,pt_mng->txlen);
	
	return Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
}
/* 
	the function transact timeout event for scan procedure
*/
VOID Mng_Scan_Timeout(PDSP_ADAPTER_T pAdap)
{

	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	//DBG_ENTER();
	
	//confirm channel
	if(pt_mng->statuscontrol.worktype & SCAN_STATE_WAIT_STATUS)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"mng timeout for confirm channel\n");
		pt_mng->statuscontrol.worktype &=(~((UINT32)(SCAN_STATE_WAIT_STATUS)));

		if(pt_mng->statuscontrol.retrynum > MAX_SCAN_WAIT_COUNT)
		{
			//here reset chip job should be done
			DBG_WLAN__MLME(LEVEL_TRACE,"SCAN end with retry count reach\n");
			//woody 080630
			//close mng_scanfail and call endscan
			//otherwise, if scan list will save old list even now no ap scanned
			//Mng_ScanFail(pAdap);
			Mng_EndScan(pAdap);
		}
		else
		{
#ifdef ANTENNA_DIVERSITY
			DBG_WLAN__MLME(LEVEL_INFO,"antenna mode = %x\n",pAdap->wlan_attr.antenna_diversity);
			if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
			{
				if (pAdap->wlan_attr.antenna_num == DSP_ANTENNA_TYPE_MAX)
				{
					pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;
					Mng_DoScan(pAdap);				
				}
				else
				{
					pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MAX;
					Mng_SetTimerPro( pt_mattr->min_channel_time/2,pAdap);
				}
				DBG_WLAN__MLME(LEVEL_INFO,"scan with antenna %d\n", pAdap->wlan_attr.antenna_num);
			}
			else
			{
				Mng_DoScan(pAdap);				
			}
#else			
			Mng_DoScan(pAdap);
#endif

		}	
		return;
	}

	
	if(	pt_mng->scan_stage == ACTIVE_SCAN_FIRST_STAGE 
	&&	pt_mng->haveSignal)  
	{
		DBG_WLAN__MLME(LEVEL_INFO,"haveSignal\n");
		/* Mng_ContinueScan(pAdap); */
		pt_mng->haveSignal = 0;
		/* set to the value that max-min; */
		pt_mng->scan_stage = ACTIVE_SCAN_SECOND_STAGE;
		Mng_SetTimerPro(pt_mattr->max_channel_time - pt_mattr->min_channel_time, pAdap);
	}
	//is second stage of scan, just flag next channel should be scan
	else /* max time reach in active and passive */
	{
#ifdef ANTENNA_DIVERSITY
		DBG_WLAN__MLME(LEVEL_INFO, "antenna mode = %x\n",pAdap->wlan_attr.antenna_diversity);
		if(pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_ENABLE)
		{
			if (pAdap->wlan_attr.antenna_num == DSP_ANTENNA_TYPE_MAX)
			{
				pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;
			}
			else
			{
				pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MAX;
			}
			DBG_WLAN__MLME(LEVEL_INFO,"scan with antenna %d\n", pAdap->wlan_attr.antenna_num);
		}
#endif
		Mng_DoScan(pAdap);				
	}

	return;
}
/*******************************************************************
 *   Mng_Join_Timeout
 *   
 *   Descriptions:
 *      the routine is called after join process fail
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_Join_Timeout(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     	pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;
	//UINT32 			keep_join;


	DBG_WLAN__MLME(LEVEL_TRACE," join timeout, worktype = %x \n",pt_mng->statuscontrol.worktype);

	if(!(pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_EXPRESS_BITS))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Join_Timeout fail without sub state \n");
		//woody
		//the timer is not meaningful. only tell us that current MNG state.
		//otherwise it is diffcult to know current state if driver enters the flow.
		Mng_SetTimerPro(50000,pAdap);
		return;
	}


	if(pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_WAIT_HW_IDLE) 
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Join_Timeout confirm idle\n");
		Mng_DoJoin(pAdap,pt_mng->statuscontrol.worktype);
		return;
	}

	//is cfm both channel or idle state
	if(!(pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_WAIT_BEACON))
	{
		if(pt_mng->statuscontrol.retrynum)
		{
			pt_mng->statuscontrol.retrynum--;
			Mng_DoJoin(pAdap,pt_mng->statuscontrol.worktype);
		}
		else
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Join fail in no waiting dev into idle #######\n");
			Mng_InitParas(pAdap);
			Mng_Fail(pAdap);
		}
		return;	
	}
	//add this else flow for this case
	//if in keep wait beacon flow, nobody know what state current is if no beacon receive
	//so here we can know what we are doing
	else		//Justin:	080725.	we can not wait beacon forever...
	{
		if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)		//wait forever in ibss mode
		{
			     DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Join_Timeout + JOIN_SUBTYPE_WAIT_BEACON\n");
			     Mng_SetTimerPro(5000,pAdap);		//justin: no wait so long
		}
		else													//bss mode
		{
			if(pt_mng->statuscontrol.retrynum)
			{
				pt_mng->statuscontrol.retrynum--;

			     DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Join_Timeout + JOIN_SUBTYPE_WAIT_BEACON\n");
			     Mng_SetTimerPro(500,pAdap);		//justin: no wait so long
			}
			else
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Join fail in no waiting beacon #######\n");
				Mng_InitParas(pAdap);
				Mng_Fail(pAdap);
			}
		}
	     //return;
	}

	
	//for beacon lost on bss mode
	if(pt_mng->statuscontrol.worktype & JOIN_STATE_KEEP_WITHOUT_BEACON) 
	{
		DBG_WLAN__MLME(LEVEL_TRACE," join timeout, worktype1 = %x \n",pt_mng->statuscontrol.worktype);
		Mng_InitParas(pAdap);
		Adap_SetLink(pAdap,LINK_FALSE);
		Adap_UpdateMediaState(pAdap,LINK_FALSE);
		return;
	}

	return; 
}

/*******************************************************************
 *   Mng_AuthTimeout
 *   
 *   Descriptions:
 *      called when authentication process timeout
 *   be finished.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_AuthTimeout(PDSP_ADAPTER_T pAdap)
{
	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_AuthTimeout\n"); 
	Mng_DoAuth(pAdap);
	return;
}

/*******************************************************************
 *   Mng_Timeout
 *   
 *   Descriptions:
 *      the routine is called by other module when timeout happen
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_Timeout(PDSP_ADAPTER_T pAdap)
{
	/* temp variableNPDEBUG("mng timer timeout"); */
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	//DBG_WLAN__MLME(("Management timeout!\n"));
	//Justin: don't do this during hardware reset
	if(!Adap_Driver_isWork(pAdap))
	{
		return;
	}
	
	if(	pt_mng->oidscanflag
	&&	pt_mng->statuscontrol.curstatus == JOINNING)
	{
		DBG_WLAN__MLME(LEVEL_ERR,"@@@@@@WRONG STATE: JOINING with OIDSCANFLAG(%u)@@@@@@@@@@@@@@@@@@@@@\n",(UINT32)(pt_mng->oidscanflag)); 		
	}

	if(	pt_mng->oidscanflag
	&&	pt_mng->statuscontrol.curstatus != JOINNING)	//wumin: 090223 avoid wrong state
	{
//		DBG_TRACE();
		Mng_Scan_Timeout(pAdap);
		return;
	}

	
	switch (pt_mng->statuscontrol.curstatus)
	{
	case IDLE:
		DBG_WLAN__MLME(LEVEL_TRACE,"timeout with idle status\n"); 
		break;
	case SCANNING:
//		DBG_TRACE();
		Mng_Scan_Timeout(pAdap);
		break;
	case JOINNING:  /* joinfailure */
		DBG_WLAN__MLME(LEVEL_TRACE,"timeout with joinning status\n"); 
		Mng_Join_Timeout(pAdap);
		break;
	case JOINOK:  /* authfailure */
		Mng_AuthTimeout(pAdap);
		break;
	case AUTHOK:
		//pt_mng->reassocflag = 0;
		Mng_DoAssoc(pAdap);
		break;
	case ASSOCOK:
		break;
	default:
		DBG_WLAN__MLME(LEVEL_ERR,"Timeout with UNKNOW status\n"); 
		pt_mng->statuscontrol.curstatus = IDLE;

		/* Mng_StartScan(SCANNING,pAdap); */
		break;
	}
}

/*******************************************************************
 *   Mng_GetLinkStatus
 *   
 *   Descriptions:
 *      get current driver work status
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      TRUE: station has connected to an AP successfully.
 *      FALSE: no connect
 ******************************************************************/
BOOLEAN Mng_GetLinkStatus(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	if(pt_mng->statuscontrol.curstatus == ASSOCOK && pt_mattr->macmode == WLAN_MACMODE_ESS_STA)
		return TRUE;
	if(pt_mng->statuscontrol.curstatus >= JOINOK && pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
		return TRUE;
	return FALSE;
}

/*******************************************************************
 *   Mng_Reset
 *   
 *   Descriptions:
 *      reset managment module
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 ******************************************************************/
VOID Mng_Reset(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	//for bss with associated
	if((pt_mng->statuscontrol.curstatus == ASSOCOK) 
		&& (pt_mattr->macmode == WLAN_MACMODE_ESS_STA))
	{
		Mng_SetLinkToUp(pAdap);

		//set wlan_attributes
	}
	//for ibss with joinok
	else if((pt_mng->statuscontrol.curstatus == JOINOK) && ( pt_mattr->macmode == WLAN_MACMODE_IBSS_STA))
	{
		//set atim window
		//Adap_Set_Inform_ATIM_Window(pAdap,FALSE);

		Mng_SetLinkToUp(pAdap);
	}
	//is joining,joinok,authok
	else if(pt_mng->statuscontrol.curstatus != SCANNING)
	{
		Mng_StartJoin(pAdap,pt_mng->statuscontrol.worktype);
	}
	 else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Reset enter wrong flow\n");
	}
	
	
}

/*******************************************************************
 *   Mng_OidScan
 *   
 *   Descriptions:
 *      called by other module to being an Oid scan process
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID  Mng_OidScan(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	Mng_ClearTimerPro(pAdap);
	pt_mng->statuscontrol.retrynum = 0;
	pt_mng->bssdeslist.bssdesnum = 0;		//init bss description

	Adap_hal_set_rx_any_bssid_beacon(pAdap);

	if(CHIP_FUNCTION_WLAN_ONLY == pAdap->chip_function)
	{
		Mng_StartScan(
		SCAN_STATE_OID_FLAG | 
		SCAN_STATE_ACTIVE_FLAG | 
		SCAN_STATE_11G_FLAG, //|        //Justin: for test
//		SCAN_STATE_11A_FLAG,
		pAdap);
	}
	else
	{
		Mng_StartScan(
			SCAN_STATE_OID_FLAG | 
			SCAN_STATE_ACTIVE_FLAG | 
			SCAN_STATE_11B_FLAG,
			pAdap);
	}
}

/*******************************************************************
 *   Mng_IsScanFinished
 *   
 *   Descriptions:
 *      Get the status whether scan is finished
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      TRUE : scan finished
 *	  FALSE: still in scaning
 ******************************************************************/
BOOLEAN Mng_IsScanFinished(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	return (pt_mng->oidscanflag == FALSE)? TRUE : FALSE ;
}
/*******************************************************************
 *   Mng_EndScan
 *   
 *   Descriptions:
 *      end scan process after all channels be scanned
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_EndScan(PDSP_ADAPTER_T pAdap)
{
	//zyy: should we flush the contention period FIFO after active scan??

#if 1
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T	pt_mattr = &pAdap->wlan_attr;
	UINT32			i;
	BOOLEAN              scan_val;
	//stop timer of scan
	Mng_ClearTimerPro(pAdap);

	
	//DBG_WLAN__MLME( ("Mng_EndScan, pt_mng->oidscanflag= %X \n",pt_mng->oidscanflag));
	scan_val = pAdap->is_oid_scan_begin_flag;
	
	/* end scan for oid scan */
	if(pt_mng->oidscanflag)        
	{
		pt_mng->oidscanflag = FALSE;
		pAdap->scan_result_new_flag = 1;
//		DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = FALSE;\n");

	
		//cp list ot oid list
		for(i = 0; i < pt_mng->bssdeslist.bssdesnum; i++)
		{
			pt_mng->bssdeslist.bssdesset[i].lifetime = 0;
		}

		//attached old AP into new scan list result if its lifetime  !=0
		for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)
		{
			pt_mng->oiddeslist.bssdesset[i].lifetime++;
			if(pt_mng->oiddeslist.bssdesset[i].lifetime > AP_LIFE_TIME_IN_SCAN)
			{
				//handle next old ap info
				continue;
			}
			Mng_Append_Item_BssList(pAdap,&pt_mng->oiddeslist.bssdesset[i],&pt_mng->bssdeslist);
		}
		pt_mng->oiddeslist.bssdesnum = pt_mng->bssdeslist.bssdesnum;
		sc_memory_copy(&pt_mng->oiddeslist.bssdesset, &pt_mng->bssdeslist.bssdesset, pt_mng->bssdeslist.bssdesnum*sizeof(MNG_DES_T));
		//dos some setting for ending scan
		if(pAdap->is_oid_scan_begin_flag == TRUE)
		{
			pAdap->is_oid_scan_begin_flag = FALSE;
		}
		//This 2 sentences should be here.

		Mng_ClearScanFlag(pAdap);
		Mng_ResourceAvailable(pAdap);

#ifdef ROAMING_SUPPORT
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, end oid scan----> reconnect_status = %d\n",pAdap->reconnect_status);
		//pAdap->can_do_reconnect_flag = TRUE;
		if(pAdap->reconnect_status == NEED_RECONNECT)
		{
			pAdap->reconnect_status = CAN_RECONNECT;
			Mng_reconnect(pAdap);
		    return;
		}
#endif
		//set softdoze to 0 to improve BT Throughput
		Mng_Fail(pAdap);

		return;
	}


	
	/* end scan for scanning */
	//here, transact normal scan procedure, usually the case happen in window 2000 or specially for hct test.
	if(pt_mng->statuscontrol.curstatus == SCANNING )
	{
		//the case for win2000 or some specially case, just expected ap is scaned, the stop scan procedure.
		if(pt_mng->statuscontrol.worktype & SCAN_STATE_MID_STOP_FLAG)
		{		
			//IBSS CASE			
			if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA) 
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Mng_EndScan IBSS, scan fail \n");

    			pt_mng->statuscontrol.retrynum = 0;
    			Mng_ClearScanFlag(pAdap);
    			Mng_ScanFail(pAdap);
			}
			//BSS CASE
			else   
			{//Justin: 0709..   try once scan only
					DBG_WLAN__MLME(LEVEL_TRACE,"Mng_EndScan BSS, scan fail ,scan = %x\n",scan_val);
					pt_mng->statuscontrol.retrynum = 0;
					Mng_ClearScanFlag(pAdap);

					Mng_ScanFail(pAdap);
					//Restore oid scan procedure
					//the procedure cancel when scanning running 
                    /*
                    if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN) && scan_val)
					{
						DBG_WLAN__MLME(LEVEL_TRACE,"Restore OID SCAN task after scanning\n ");
						Task_CreateTask(
							(PDSP_TASK_T)pAdap->ppassive_task,
							DSP_TASK_EVENT_SCAN,
							DSP_TASK_PRI_HIGH,
							NULL,
							0);							
					}*/
					return;

			#if 0	//Justin: 0709
				if(pt_mng->statuscontrol.retrynum >= MAXSCANNUM)
				{
					DBG_WLAN__MLME(LEVEL_TRACE,"Mng_EndScan BSS, scan fail \n");
					pt_mng->statuscontrol.retrynum = 0;
					Mng_ClearScanFlag(pAdap);
					Mng_ScanFail(pAdap);
					return;
				}
				else
				{
					DBG_WLAN__MLME(LEVEL_TRACE,"Mng_EndScan BSS, try scan times = %d \n",pt_mng->statuscontrol.retrynum);
					pt_mng->statuscontrol.retrynum++;
					Mng_StartScan(pt_mng->statuscontrol.worktype, pAdap);
					return;
				}
			#endif
			}
		}
		/*  this is normal join process of 2000 */
		else
		{
			/* ??? here , join of 2000 should be done */
			if(pt_mng->bssdeslist.bssdesnum == 0)        //scan fail
			{
				Mng_ScanFail(pAdap);			
			}
			else					
			/* select first drscription ,scan end and begin join */
			{
				if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA) 
				{
					Mng_ScanFail(pAdap);	
					return;				
				}
				
				sc_memory_copy(&pt_mng->usedbssdes, &pt_mng->bssdeslist.bssdesset[0], sizeof(MNG_DES_T)); 
				Mng_StartJoin(pAdap,JOIN_STATE_NORMAL_INIT);
			}
		}
	}
	//never happen
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng end scan enter wrong flow\n");
	}
	return;
#endif
}


VOID Mng_BreakIfScanning(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T	pt_mattr = &pAdap->wlan_attr;
	//UINT32			i;

	//wumin : 090226 move this from outside to inside of function
	if(	pt_mng->statuscontrol.curstatus == SCANNING
	||	pt_mng->oidscanflag//Set_OID_802_11_SSID
	||	pAdap->is_oid_scan_begin_flag)
	{
		DBG_WLAN__MLME(LEVEL_TRACE, "Scanning, break it.\n");
		//stop timer of scan
		Mng_ClearTimerPro(pAdap);

		// wumin : 090226 also clear this flag
		pAdap->is_oid_scan_begin_flag=FALSE;
		
		pt_mng->oidscanflag = FALSE;

		pAdap->scanning_flag = FALSE;

		if (pt_mng->statuscontrol.curstatus == SCANNING)
			pt_mng->statuscontrol.curstatus = IDLE;
	}
}

/*******************************************************************
 *   Mng_ClearScanFlag
 *   
 *   Descriptions:
 *      clear scan flag to inform up level continuing send packet
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_ClearScanFlag(PDSP_ADAPTER_T pAdap)
{
	//PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T             pt_mattr = &pAdap->wlan_attr;

	/*  pt_mattr->->m_ScanFlag = 0; */
	/*  pt_mattr->scanflag */
	pAdap->scanning_flag = FALSE;
	pAdap->is_oid_scan_begin_flag = FALSE;
}
		
/*******************************************************************
 *   Mng_RestoreRegValue
 *   
 *   Descriptions:
 *      restore register value after oid scan finished
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_RestoreRegValue(PDSP_ADAPTER_T pAdap)
{	
#if 0
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	UINT32			i;

	pt_mng->oidscanflag = FALSE;
	
#ifdef NDIS50_MINIPORT
	//if(pt_mng->statuscontrol.curstatus == ASSOCOK)
	if(Mng_GetLinkStatus(pAdap))
	{
		Mng_WriteBeaconInfoReg(WRITE_BSSID_REG|WRITE_SSID_REG|WRITE_CHANNEL_REG, &pt_mng->usedbssdes, pAdap);
		// Mng_ResourceAvailable(pAdap);
	}
	else
	{
		if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA) &&
		    (pt_mng->statuscontrol.curstatus == JOINNING) && 
			(pt_mng->statuscontrol.worktype = KEEP_JOIN_WITH_BEACON))
		{
			Mng_WriteBeaconInfoReg(WRITE_BSSID_REG|WRITE_SSID_REG|WRITE_CHANNEL_REG, &pt_mng->usedbssdes, pAdap);
			// Mng_ResourceAvailable(pAdap);
		}
		else
		{
			Mng_StartScan(pt_mng->statuscontrol.worktype, pAdap);
		}
	}
#endif
	
	
	/* only after joinen,restore action is meaningful */

#ifdef NDIS51_MINIPORT
	/*  if(pt_mng->statuscontrol.curstatus == JOINNING) */
	if(Mng_GetLinkStatus(pAdap))
	{
		Mng_WriteBeaconInfoReg(WRITE_BSSID_REG|WRITE_SSID_REG|WRITE_CHANNEL_REG, &pt_mng->usedbssdes, pAdap);
		// Mng_ResourceAvailable(pAdap);
	}
	else
	{
		if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
		{
			if((pt_mng->statuscontrol.curstatus == JOINNING) && (pt_mng->statuscontrol.worktype = KEEP_JOIN_WITH_BEACON))
			{
				Mng_WriteBeaconInfoReg(WRITE_BSSID_REG|WRITE_SSID_REG|WRITE_CHANNEL_REG, &pt_mng->usedbssdes, pAdap);
				// Mng_ResourceAvailable(pAdap);
			}
		}
		//11.02
		// add to restore successful when scan happen with joining,authing,associng 
		//jump to joinning
		else   //bss mode
		{
			Mng_StartJoin(pAdap);
		}
		

		//other condition do nothing
	}
#endif
	
	/*Mng_ClearScanFlag(pAdap);*/
#endif
}

//VOID Mng_RestoreForJumpScan(PDSP_ADAPTER_T pAdap)
//{	
	//PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	//UINT32			i;

	// DBG_WLAN__MLME(("++++++++++++++++++Mng_RestoreForJumpScan++++++++++++++++\n"));

	//??????
	//pt_mng->oidscanflag = FALSE;

	//Mng_WriteBeaconInfoReg(WRITE_BSSID_REG|WRITE_SSID_REG|WRITE_CHANNEL_REG, &pt_mng->usedbssdes, pAdap);

//}



/*******************************************************************
 *   Mng_ScanFail
 *   
 *   Descriptions:
 *      is called after scan fail
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_ScanFail(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T	pt_mattr = &pAdap->wlan_attr;
	UINT32		i;

	//woody for debug
	Mng_InitParas( pAdap);
	DBG_ENTER();

	//set soft doze into idle state
	Mng_Fail(pAdap);

	// IBSS scan fail
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		Mng_IbssScanFail(pAdap);
		return;
	}

#ifdef ROAMING_SUPPORT
	DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, end oid scan----> reconnect_status = %d\n",pAdap->reconnect_status);
	//pAdap->can_do_reconnect_flag = TRUE;
	if(pAdap->reconnect_status == NEED_RECONNECT)
	{
		pt_mng->oidscanflag = FALSE;
//		DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = FALSE;\n");
		
		pAdap->reconnect_status = CAN_RECONNECT;
		Mng_reconnect(pAdap);
		return;
	}
#endif

	//attached old AP into new scan list result if its lifetime  !=0
	for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)
	{
		pt_mng->oiddeslist.bssdesset[i].lifetime++;
		if(pt_mng->oiddeslist.bssdesset[i].lifetime > AP_LIFE_TIME_IN_SCAN)
		{
			//handle next old ap info
			continue;
		}
		Mng_Append_Item_BssList(pAdap,&pt_mng->oiddeslist.bssdesset[i],&pt_mng->bssdeslist);
	}
	pt_mng->oiddeslist.bssdesnum = pt_mng->bssdeslist.bssdesnum;
	sc_memory_copy(&pt_mng->oiddeslist.bssdesset, &pt_mng->bssdeslist.bssdesset, pt_mng->bssdeslist.bssdesnum*sizeof(MNG_DES_T));


	//bss scan fail
	if(pt_mng->oidscanflag)
	{
		pt_mng->oidscanflag = FALSE;
//		DBG_TRACE();
		
		//{//Justin:	080222.	this flag means we have received a set_ssid OID before, so we must indicate complete here while fail to join...
//wumin		NdisMSetInformationComplete(pAdap->dsp_adap_handle,STATUS_SUCCESS);
		//pAdap->set_information_complete_flag = TRUE;
		Adap_SetLink(pAdap,LINK_FALSE);
		Adap_UpdateMediaState(pAdap,LINK_FALSE);
		Mng_ClearScanFlag(pAdap);

		Mng_InitParas(pAdap);
	}

	return;
/////////////////////////////////////////stop here////////////////////////////////////////////
	
	/* BSS scan fail */
	if(!(pt_mng->statuscontrol.worktype & SCAN_STATE_MID_STOP_FLAG))
	{

#if  SCANFAILDEALY > 0
			//keep scanning for scanfaildelay timer
//			DBG_TRACE();
			pt_mng->statuscontrol.workcount = 0; 
			pt_mng->statuscontrol.retrynum = 0;
			Mng_SetTimerPro(SCANFAILDEALY, pAdap);
#else
			//repeat scan forever
//			DBG_TRACE();
			Mng_StartScan(pt_mng->statuscontrol.worktype, pAdap);
#endif
	}
	/* is fail of middle stop scan */
	else
	{
	////	//nothing will be done for this case, wait system send new command
		//if(pt_mng->statuscontrol.worktype & SCAN_STATE_MID_STOP_FLAG)
		if(pt_mng->oidscanflag)        
		{
			pt_mng->oidscanflag = FALSE;
			
//			DBG_TRACE();
			
//			DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = FALSE;\n");
			//This flag means we have received a set_ssid OID before, so we must indicate complete here while fail to join...
			Adap_SetLink(pAdap,LINK_FALSE);
			Adap_UpdateMediaState(pAdap,LINK_FALSE);

			Mng_InitParas(pAdap);
			//Re-set auto correlator parameter
			//pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 0;
			//pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze = 0;
			//Adap_set_auto_corr(pAdap);
		}
	
	}

	return;
}

/*******************************************************************
 *   Mng_Rx_WhenOidScan
 *   
 *   Descriptions:
 *      the function is called with oid scan status when managment
 *      frame was receive.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_Rx_WhenOidScan(PDSP_ADAPTER_T pAdap, UINT8 rssi)
{
	
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T       			pt_mattr = &pAdap->wlan_attr;
	PMNG_DESLIST_T      		pt_deslist = (PMNG_DESLIST_T)&pt_mng->bssdeslist;
	PMNG_HEAD80211_T		phead;
	MNG_DES_T		    		curdes;
	PBEACON_FIXED_FIELD_T	pbeaconfixed;
	UINT16	cap;
	PUINT8  bssid;
	UINT32	i;
	UINT32	j;
	
	//DBG_WLAN__MLME(("in Mng_Rx_WhenOidScan\n"));

#ifdef DEBUG_RSSI	
	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Rx_WhenOidScan rssi = %X\n",rssi);
#endif

	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;
	pbeaconfixed = (PBEACON_FIXED_FIELD_T)WLAN_LOCATE_3DSP_RX_PAYLOAD(pt_mng->recbuf);
	cap = pbeaconfixed->cap;
	

	//Bssdes already full  ??
	if (pt_deslist->bssdesnum >= WLAN_BSSDES_NUM)
	{
		pt_deslist->bssdesnum = WLAN_BSSDES_NUM;
		DBG_WLAN__MLME(LEVEL_TRACE,"OID SCAN but AP list already full \n");
		return;
	}

	// is not beacon or probersp frame 
	if(!DOT11_FRM_IS_PROBE_RESP(phead->fc)  &&  
		!DOT11_FRM_IS_BEACON(phead->fc))
	{
		DBG_WLAN__MLME(LEVEL_INFO,"received no beacon and no probersp frame when oid scan\n");
		return;
	}

		
	//filter ap has been added into list
	for (i = 0; i < pt_deslist->bssdesnum ; i++)
	{
		//get bssid pointer
		bssid = CAP_EXPRESS_BSS(cap) ? phead->adr.bssrx.bssid:   //bss
										phead->adr.ibss.bssid;    //ibss
			                                                
		if(IS_MATCH_IN_ADDRESS(pt_deslist->bssdesset[i].bssid, bssid))				
		{
#ifdef ANTENNA_DIVERSITY
			//signal is better with this antenna
			if(rssi > pt_deslist->bssdesset[i].rssi)
			{
				pt_deslist->bssdesset[i].antenna_num = pAdap->wlan_attr.antenna_num;
			 	pt_deslist->bssdesset[i].rssi = (INT32)rssi;
			}	
#endif
			DBG_WLAN__MLME(LEVEL_INFO,"scanned addr have been include bssdes table when oid scan\n");
				return;
		}		

	}

	//can't get correct beacon info
	if( !Mng_GetBeaconInfo(&curdes, pAdap))
	{
		DBG_WLAN__MLME(LEVEL_INFO,"Oid scan ,get beacon info error\n");
		return;
	}

	//add phy mode flag into the ap's description
	curdes.phy_mode = (UINT32)pt_mng->statuscontrol.workflag;
	curdes.rssi = (INT32)rssi ;	//Justin: 0626
	DBG_WLAN__MLME(LEVEL_INFO,"RSSI BEACON = %x\n",rssi);
	
#ifdef ANTENNA_DIVERSITY
	curdes.antenna_num = pAdap->wlan_attr.antenna_num;
#endif	


	if(GetRssiMsb(curdes.rssi) != 0)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"###Find dlink preamble head \n");
	}

	
	/* set havesignal flag for continue scan */
	if(pt_mng->curchannel == curdes.channel)
	{
		pt_mng->haveSignal = 1;
	}

	//FOR ibss, find if already exist according ssid
	if(CAP_EXPRESS_IBSS(cap))
	{
		for (j = 0; j < pt_deslist->bssdesnum ; j++)
		{
			if(0 == sc_memory_cmp(curdes.ssid,pt_deslist->bssdesset[j].ssid,curdes.ssid[0] +1))
			{
				sc_memory_copy(&pt_deslist->bssdesset[j], &curdes,sizeof(MNG_DES_T));
				return;			
			}
		}
	}

	//put new AP info into AP list
	{
		i = pt_deslist->bssdesnum;
		sc_memory_copy(&pt_deslist->bssdesset[i], &curdes,sizeof(MNG_DES_T));
		pt_deslist->bssdesnum++;
//			if(DBG_FLAG & DBG_MLME_FLOW)
//			{
//				UINT8 buf[40];
//				sc_memory_copy(buf,&curdes.ssid[1],curdes.ssid[0]);
//				buf[curdes.ssid[0]] = '\0';
//				DBG_WLAN__MLME(LEVEL_TRACE,"Scan And Get an AP and save it into positon :%x \n",i);
//				DBG_WLAN__MLME(LEVEL_TRACE," SSID = %s \n",buf);
//			}			
	}

	
}

/*******************************************************************
 *   Mng_GetBeaconInfo
 *   
 *   Descriptions:
 *      get information of ap from beacon frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      pcurdes: , save information got from beacon frame
 *   Return Value:
 *      TRUE: get valid information from beacon
 *      FALSE: did not get valid information from beacon
 *   Jakio2006.12.18: changelog:
 *		1) copy the IE, whatever
 *		2)record wpa or wpa2 ie if have
 ******************************************************************/
BOOLEAN Mng_GetBeaconInfo(PMNG_DES_T pcurdes,PDSP_ADAPTER_T pAdap)
{
	
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	PBEACON_FIXED_FIELD_T   	pfixed;
	PMNG_HEAD80211_T		phead;
	PMNG_NOFIXED_ELE_ALL_T  pnofixed;
//	PMNG_COUNTRY_CHAN_T	pcountrychan;
	PMNG_RSNFIXED_T			prsn; 
	PMNG_WPA2_RSNFIXED_T		pwpa2rsn;
//	PUINT8				    	ptmp;
//	UINT16					offset;
	UINT32					i;
//	UINT8  					id;
	PUINT8   				bssid;
	BOOLEAN 				goOn;
	UINT16					tmpLen;
	
	sc_memory_set(pcurdes, 0, sizeof(MNG_DES_T));

	phead = (PMNG_HEAD80211_T)pt_mng->recbuf;

	//Jakio2006.12.18: buffer all ies
	if(pt_mng->rxlen < WLAN_HDR_A3_LEN)
	{
		DBG_WLAN__MLME( LEVEL_TRACE,"Invalid frame lenth");
		return FALSE;
	}
	tmpLen = pt_mng->rxlen - WLAN_HDR_A3_LEN - 
			DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN -pt_mng->body_padding_len;
	if(tmpLen > 0)
	{
		pcurdes->IELength = (tmpLen > MAX_BEACON_RSP_INFO_LENGTH)? MAX_BEACON_RSP_INFO_LENGTH:tmpLen;
		sc_memory_copy(pcurdes->IEs, WLAN_LOCATE_3DSP_RX_PAYLOAD(pt_mng->recbuf), pcurdes->IELength);
	}
	//endof Jakio2006.12.18
	
	/* 1: get addr  */
	WLAN_COPY_ADDRESS(pcurdes->addr, phead->adr.addr.a2);

	/* 2: get fixed field  */
	pfixed = (PBEACON_FIXED_FIELD_T)WLAN_LOCATE_3DSP_RX_PAYLOAD(pt_mng->recbuf);
	pcurdes->cap = pfixed->cap;
	
	pcurdes->beaconinterval = pfixed->beaconinterval;
	if(pfixed->beaconinterval == 0)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"! beacon interval = 0\n");
		return FALSE;
	}

	/* 3:bssid */
	bssid = CAP_EXPRESS_BSS(pcurdes->cap) ? phead->adr.bssrx.bssid : phead->adr.ibss.bssid;
	WLAN_COPY_ADDRESS(pcurdes->bssid, bssid);

	
	//init some value
	pcurdes->atimw = 0;
	pcurdes->Erpflag  = 0;
	pcurdes->Erpinfo = 0;
	pcurdes->ExtendedSupportRateLen = 0;
	pcurdes->networkType = Ndis802_11DS;
	//Jakio20070629: add for wpa2
	pcurdes->offset_wpa = 0xffff;
	pcurdes->offset_wpa2 = 0xffff;
	
	//get first no fixed element	
	//pnofixed =WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);
	//indicate if check in valid range 
	//goOn = (((PUINT8)pnofixed -pt_mng->recbuf) < 
	//		(pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN
	//		- pt_mng->body_padding_len));
	//Jakio20070703: change for record wpa/wpa2 info
	pnofixed = (PMNG_NOFIXED_ELE_ALL_T)(pcurdes->IEs + sizeof(BEACON_FIXED_FIELD_T));
	//Justin:0806.   body_padding_len must be subtracted, otherwise, it cause cover the existed item's value by a wrong value in the padding bytes.
	// This a very serious case. maybe will cause total driver works abnormal, so much as blue screen.
	// Do not edit this sentence anytime.
	goOn = (pcurdes->IELength > 0) && ((pcurdes->IEs + pcurdes->IELength- pt_mng->body_padding_len) > (PUINT8)pnofixed);
	while(goOn)	// edit by Justin
	{
		//id = pnofixed->element;
		switch (pnofixed->element)
		{
			case SSID_EID:
				//get ssid
				if(SSID_LENGTH_VALID(pnofixed->length))
				{
					/* get value */
					pcurdes->ssid[0] = pnofixed->length;
					sc_memory_copy(&pcurdes->ssid[1], pnofixed->data,pnofixed->length);
					break;
				}
				return FALSE;
			case SUPRATE_EID:			
				if(SUPPORT_RATE_LENGTH_VALID(pnofixed->length))
				{
					/* get value */
					pcurdes->suprate[0] = pnofixed->length;
					sc_memory_copy(&pcurdes->suprate[1], pnofixed->data, pnofixed->length);
					break;
				}
				DBG_WLAN__MLME(LEVEL_TRACE,"AP's support length out of ragne \n");
				return FALSE;
			case DSPARA_EID:  //3
				pcurdes->channel = pnofixed->data[0];
				break;
			case IBSSPARA_EID:  //4
				if(CAP_EXPRESS_IBSS(pcurdes->cap))
				{
					/* get value */
					pcurdes->atimw = pnofixed->data[0];
					break;
				}
				DBG_WLAN__MLME(LEVEL_TRACE,"Find IBSS item at non IBSS beaocn\n");
				return FALSE;
			case ERPINFORMATION_EID:  //5
				pcurdes->Erpinfo  = pnofixed->data[0];
				pcurdes->Erpflag  = 1;
				break;
			case EXTENDRATE_EID:   //6
				pcurdes->ExtendedSupportRateLen = pnofixed->length;
				sc_memory_copy(pcurdes->ExtendedSupportRate,pnofixed->data,MAX_ESR_LEN);
				break;
				

			//Jakio2006.12.18: add for wpa(wpa2)
			case WPA1_EID:
				prsn = (PMNG_RSNFIXED_T)pnofixed->data;
				if(prsn->oui != RSNOUI)
				{
					break;
				}	
				pcurdes->offset_wpa = (UINT16)((PUINT8)pnofixed - pcurdes->IEs);
				break;
			
			#ifdef DSP_WPA2
			case WPA2_EID:
				pwpa2rsn = (PMNG_WPA2_RSNFIXED_T)pnofixed->data;
				if((pwpa2rsn->groupSuite&0x00ffffff) != 0x00ac0f00)
					break;
				pcurdes->offset_wpa2 = (UINT16)((PUINT8)pnofixed - pcurdes->IEs);
				break;
			#endif 

			default:
				//DBG_WLAN__MLME(("err situation in default case\n"));
			;
		}

		/* move point */
		WLAN_MOVE_TO_NEXT_ELE(pnofixed);
		//Justin:0806.   body_padding_len must be subtracted, otherwise, it cause cover the existed item's value by a wrong value in the padding bytes.
		// This a very serious case. maybe will cause total driver works abnormal, so much as blue screen.
		// Do not edit this sentence anytime.
		goOn = ((pcurdes->IEs + pcurdes->IELength) > (PUINT8)pnofixed);//- pt_mng->body_padding_len

		if((((pcurdes->IEs + pcurdes->IELength) - (PUINT8)pnofixed)) < sizeof(MNG_NOFIXED_ELE_ALL_T))
		{
			break;
		}

	}

	
	Mng_DeleteUnsupportedRate(pAdap,pcurdes);


	// to be confirm latter ...... zyy	//jUSTIN: 0625..... check
	if(Adap_CheckOFDMSupport((PUINT8)&pcurdes->suprate[1],pcurdes->suprate[0],pcurdes->ExtendedSupportRate,pcurdes->ExtendedSupportRateLen))
	{
		pcurdes->networkType = (pcurdes->channel > 14) ? Ndis802_11OFDM5: Ndis802_11OFDM24;//IEEE802_11_A : IEEE802_11_G;

		pcurdes->phy_mode = (pcurdes->networkType == Ndis802_11OFDM5) ? IEEE802_11_A:IEEE802_11_G;
	}	
	else
	{
		pcurdes->networkType = Ndis802_11DS;//IEEE802_11_B;
		pcurdes->phy_mode = IEEE802_11_B;
	}	

	//Get offset Hi and Lo
	pcurdes->offsetHi = (UINT32)(*(pt_mng->recbuf + pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_HI_LEN));
	pcurdes->offsetLo = (UINT32)(*(pt_mng->recbuf + pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN));

	/*12 rsn */
	//Jakio2006.12.18: changed here, we do this no matter whether we support wpa(wpa2) or not
	//#ifdef DSP_WPA			// need to update latter,  zyy
	/* 1:first all elements to buffer */

	//Jakio2006.12.18:moved codes from here to the upper place
	/*
	if( pt_mng->rxlen < WLAN_HDR_A3_LEN )  
	{
		return FALSE;
	}

	pcurdes->IELength = pt_mng->rxlen - WLAN_HDR_A3_LEN - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN;
	sc_memory_copy(pcurdes->IEs, WLAN_LOCATE_3DSP_RX_PAYLOAD(pt_mng->recbuf), pcurdes->IELength);	// Justin:  include body, not rssi and offset
	*/
		
	if(DOT11_FRM_IS_BEACON(phead->fc))
	{
		for( i = 0; i < pt_mng->beacondeslist.bssdesnum; i++)
		{
			if(0 == sc_memory_cmp(pt_mng->beacondeslist.bssdesset[i].addr, pcurdes->addr,WLAN_BSSID_LEN))
			{
				sc_memory_copy(&pt_mng->beacondeslist.bssdesset[i], pcurdes, sizeof(MNG_DES_T));
				break;
			}
		}

		if( i == pt_mng->beacondeslist.bssdesnum)
		{
			if(pt_mng->beacondeslist.bssdesnum >= WLAN_BSSDES_NUM)
			{
				pt_mng->beacondeslist.bssdesnum = WLAN_BSSDES_NUM - 1; 
			}
		
			sc_memory_copy(&pt_mng->beacondeslist.bssdesset[pt_mng->beacondeslist.bssdesnum], pcurdes, sizeof(MNG_DES_T));
			pt_mng->beacondeslist.bssdesnum++;
		}
	}
	else  /* is probe rsp */
	{/* get tim */
		UINT32	length;
		PUINT8  p;
		/* p=m_Scan->GetNoExistEle(bssp.tmpBssid,&length); */
		if(Mng_GetNoExistEle(pcurdes->bssid,(UINT32)pcurdes->IELength, &p, &length, pAdap))
		{
			if(pcurdes->IELength + length  <= (UINT32)(MAX_BEACON_RSP_INFO_LENGTH))
			{
				sc_memory_copy(pcurdes->IEs + pcurdes->IELength, p , length);
				pcurdes->IELength += length;
			}
		}
	}
	
	if((pcurdes->channel > (MAXCHANNEL-1)) || (pcurdes->channel < 1))
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"######## invalide channel = 0x%x, rx_len = 0x%x  ############\n",pcurdes->channel,pt_mng->rxlen);
		Adap_PrintBuffer(pt_mng->recbuf, pt_mng->rxlen);
		pcurdes->channel = MAXCHANNEL-1;
		
		Adap_PrintBuffer(pcurdes->IEs, pcurdes->IELength);

		return FALSE;
	}

	return TRUE;
}

/*******************************************************************
 *   Mng_GetCountryWhenJoin
 *   
 *   Descriptions:
 *      get country info when join,because probe response frame didn't
 *      include the info.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      pcurdes: , save information got from beacon frame
 *   Return Value:
 *      TRUE: get valid information from beacon
 *      FALSE: did not get valid information from beacon
 ******************************************************************/
BOOLEAN Mng_GetCountryWhenJoin(PDSP_ADAPTER_T pAdap)
{

//	PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
//	PBEACON_FIXED_FIELD_T   	pfixed;
//	PMNG_HEAD80211_T		phead;
//	PMNG_COUNTRY_CHAN_T	pcountrychan;
//	PUINT8				    	ptmp;
//	UINT16					offset;
//	UINT32					i;
//	UINT16                  		reclen;
//	UINT16	                		position;
//PMNG_DES_T			    	pcurdes;
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PMNG_NOFIXED_ELE_ALL_T  pnofixed;
	BOOLEAN					goOn;

	//pcurdes = (PMNG_DES_T)&pt_mng->usedbssdes;
	//sc_memory_set(&pcurdes->countryinfo,sizeof(MNG_COUNTRY_INFO_T));
	//reclen = pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN;	// edit by Justin
	//position = WLAN_HDR_A3_LEN + sizeof(BEACON_FIXED_FIELD_T);

	pnofixed = WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);
	goOn = (((PUINT8)pnofixed - pt_mng->recbuf) < (pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN));

#ifdef COUNTRY_DOMAIN		// spec in 11d
	while(goOn)
	{
		if(pnofixed->element == COUNTRY_ELD)
		{
			PMNG_COUNTRY_CHAN_T	pcountrychan;
			PMNG_COUNTRY_INFO_T      pcountryinfo;
			UINT32   					left;
			UINT8					power=0;
			UINT8   					count=0;
			UINT8   					i;
//			PUINT8  pcountry;


			pcountryinfo = &pt_mng->usedbssdes.countryinfo;
			//1:get country info	(country string 3 octets)
			//get country code
			pcountryinfo->country[0] = pnofixed->data[0];
			pcountryinfo->country[1] = pnofixed->data[1];
			pcountryinfo->country[2] = pnofixed->data[2];
			//get left length
			left = pnofixed->length -3;
			pcountrychan = (PMNG_COUNTRY_CHAN_T)(&pnofixed->data[3]);

			while(left >= sizeof(MNG_COUNTRY_CHAN_T))
			{
				for(i = 0; i < pcountrychan->numofch; i++)
				{
					//channe number always is bigger than max channel list
					if(count >= WLAN_MAX_CHANNEL_LIST)
					{
						break;
					}	
					//put channel into list
					pcountryinfo->channellist[count++] = pcountrychan->firstch + i;
				}
				if(power < pcountrychan->maxpower)
				{
					power = pcountrychan->maxpower;
				}	

				left -= sizeof(MNG_COUNTRY_CHAN_T);
				//move to next channel info field
				pcountrychan++;
			}
		
			pcountryinfo->channelnum = count;
			pcountryinfo->maxpower = power;
			return TRUE;
		}  //ele end
		else
		{
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
			goOn = (((PUINT8)pnofixed - pt_mng->recbuf) < (pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN));
		}
	} //while end
#endif
	return FALSE;
}
/*******************************************************************
 *   Mng_AddBeaconBssDes
 *   
 *   Descriptions:
 *      add a ap description to beacon description list
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      des: , the description is added to list
 *   Return Value:
 *      none
 ******************************************************************/
 /*
VOID Mng_AddBeaconBssDes(PMNG_DES_T des,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	UINT32			i;


	* if (beaconBssDesNum>=WLAN_BSSDES_NUM) return;    *
	*  if already have ,not add again *
	for( i = 0; i < pt_mng->beacondeslist.bssdesnum; i++)
	{
		* if(sc_memory_cmp(pt[i].tmpBssid,bsslist->tmpBssid,WLAN_BSSID_LEN)) *
		if(sc_memory_cmp(pt_mng->beacondeslist.bssdesset[i].bssid, des->bssid,WLAN_BSSID_LEN))
		{
			sc_memory_copy(&pt_mng->beacondeslist.bssdesset[i],des,sizeof(MNG_DES_T));
			return;		
		}
	}
	
	while(pt_mng->beacondeslist.bssdesnum >= WLAN_BSSDES_NUM)
		pt_mng->beacondeslist.bssdesnum--; 
	
	sc_memory_copy(&pt_mng->beacondeslist.bssdesset[pt_mng->beacondeslist.bssdesnum],des,sizeof(MNG_DES_T));
	pt_mng->beacondeslist.bssdesnum++;
	* DBG_WLAN__MLME(("OTHER scan for ssid: found a assigned ssid!!!\n\n"); *
	return;
}
*/

/*******************************************************************
 *   Mng_AddBeaconBssDes
 *   
 *   Descriptions:
 *      get element is included in beacon instead of probe response 
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      pbssid: , bssid of ap
 *      len: , length of getting information
 *   Return Value:
 *      none
 ******************************************************************/
BOOLEAN Mng_GetNoExistEle(PUINT8 pbssid,UINT32 beaconlen,PUINT8 *pbuf,UINT32 *len,PDSP_ADAPTER_T pAdap)
{
	
	/*PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PMNG_NOFIXED_ELE_HEAD_T pdata;
	PUINT8				ptmp;
	MNG_DES_T			curdes;
	UINT32				i;
	
	UINT32 tmplong;
	UINT32 offset;

	tmplong = beaconlen;

	if(tmplong > 300)
		return FALSE;


	for(i=0; i < pt_mng->beacondeslist.bssdesnum; i++)
	{
		if(sc_memory_cmp(pt_mng->beacondeslist.bssdesset[i].bssid, pbssid,WLAN_BSSID_LEN))
		{
			pdata = (PMNG_NOFIXED_ELE_HEAD_T)(pt_mng->beacondeslist.bssdesset[i].IEs);
			offset = sizeof(BEACON_FIXED_FIELD_T);
			pdata = (PMNG_NOFIXED_ELE_HEAD_T)pdata + offset;

			while( offset < tmplong)
			{
				if(pdata->element != TIM_EID)
				{
					pdata = (PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pdata + sizeof(MNG_NOFIXED_ELE_HEAD_T) + pdata->length);					
					offset = offset + sizeof(MNG_NOFIXED_ELE_HEAD_T) + pdata->length;
				}
				else   //tim
				{
					*pbuf = (PUINT8)pdata;
					*len = pdata->length + sizeof(MNG_NOFIXED_ELE_HEAD_T);
					return TRUE;
				}
			}  //while end

			return FALSE;
		} //bssid no same
	} 
	return FALSE;
	*/


	/* find tim */

	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PMNG_NOFIXED_ELE_ALL_T pdata;
	//PUINT8				ptmp;
	//MNG_DES_T			curdes;
	UINT32				i;
	
	UINT32 tmplong;
	UINT32 offset;

	tmplong = beaconlen;

	if(tmplong > 300)
		return FALSE;


	for(i=0; i < pt_mng->beacondeslist.bssdesnum; i++)
	{
		if(IS_MATCH_IN_ADDRESS(pt_mng->beacondeslist.bssdesset[i].bssid, pbssid))
		{
			//pdata = (PMNG_NOFIXED_ELE_HEAD_T)(pt_mng->beacondeslist.bssdesset[i].IEs);
			offset = sizeof(BEACON_FIXED_FIELD_T);
			pdata = WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->beacondeslist.bssdesset[i].IEs);

			while( offset < tmplong)
			{
				if(pdata->element != TIM_EID)
				{
					WLAN_MOVE_TO_NEXT_ELE(pdata);
					offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pdata);
				}
				else   //tim
				{
					*pbuf = (PUINT8)pdata;
					*len = WLAN_FRAME_NOFIXED_ELE_LENGTH(pdata);
					return TRUE;
				}
			}  //while end

			return FALSE;
		} //bssid no same
	} 
	return FALSE;
}
/*******************************************************************
 *   Mng_AddtoBssDesSetXp
 *   
 *   Descriptions:
 *      add a ap description to normal description list
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      pdes: , the description should be added
 *   Return Value:
 *      none
 ******************************************************************/
/*
VOID Mng_AddtoBssDesSetXp(PMNG_DES_T pdes,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	UINT32			i;
	UINT32			pos;

	* full of *
	if ( pt_mng->bssdeslist.bssdesnum >= WLAN_BSSDES_NUM) 
		return;
	
	pos = pt_mng->bssdeslist.bssdesnum;     

	*  if already have ,not add again *
	for(i = 0; i < pos; i++)
	{
		if(sc_memory_cmp(pt_mng->bssdeslist.bssdesset[i].bssid, pdes->bssid, WLAN_BSSID_LEN))
			return;		
	}

	sc_memory_copy((PUINT8)&pt_mng->bssdeslist.bssdesset[pt_mng->bssdeslist.bssdesnum], pdes, sizeof(MNG_DES_T));
	pt_mng->bssdeslist.bssdesnum++;
}
*/

/*******************************************************************
 *   Mng_WriteBeaconInfoReg
 *   
 *   Descriptions:
 *      write registers related with beacon
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      flag: , map of register should be written.
 *      pdes: , description of saving information
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_WriteBeaconInfoReg(UINT16 flag,PMNG_DES_T pdes,PDSP_ADAPTER_T pAdap)
{
	//PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	//UINT32			i;
	//UINT32		    tmplong;
	
	
	/* bit 1: */
	if(flag & WRITE_BSSID_REG)
	{
		//for (i = 0; i < WLAN_BSSID_LEN; i++)
		//{
		//}
		// woody close , Adap_WriteRegs(pAdap, pdes->bssid, WLAN_BSSID_LEN, (UINT16)BSSID_REG);
	}
	
	/* bit 2 */
	if(flag & WRITE_SSID_REG)
	{
		/* if(pdes->ssidlen < 32 ) */
		if(pdes->ssid[0] <= WLAN_SSID_MAXLEN) 
		{

			//for(i = 0; i < (UINT32)pdes->ssid[0] ; i++)

			if(pdes->ssid[0] != 0)
			{
				//// woody close , Adap_WriteRegs(pAdap, (PUINT8)&pdes->ssid[1], (UINT16)pdes->ssid[0], (UINT16)SSID_REG);
			}
			
		}
	}
	
	/* bit 3 */
	if(flag & WRITE_BEACONINTERVAL_REG)
	{
		UINT16 temp;
		//Mng_WriteReg16(pAdap, pdes->beaconinterval, (UINT16)BEACON_INTERVAL);
		temp = pdes->beaconinterval * 100; 
		//Mng_WriteReg16(pAdap, temp, (UINT16)BEACON_WINDOW);
	}
	
	/* bit 4 */
	if(flag & WRITE_CAP_REG)
	{
		//Mng_WriteReg16(pAdap, pdes->cap, (UINT16)CAPABILITY_INFORMATION);
	}
	
	/* bit 5 */
	if(flag & WRITE_SPRATE_REG)
	{
		if(pdes->suprate[0] <= WLAN_MAX_RATE_SUPPORT_LEN)
		{
			//for(i=0; i < pdes->suprate[0]; i++)
			if(pdes->suprate[0] != 0)
			{
				//// woody close , Adap_WriteRegs(pAdap, (PUINT8)&pdes->suprate[1], (UINT16)pdes->suprate[0], (UINT16)SP_RATE);
			}
			
		}
	}
	
	/* bit 6 */
	if(flag & WRITE_ATIM_REG)
	{
		//Mng_WriteReg16(pAdap, pdes->atimw, (UINT16)IBSS_ATIM);
	}
	
	
	/* bit 7 */
	if(flag & WRITE_CHANNEL_REG)
	{
		//NdisStallExecution(50);
		Adap_set_channel(pAdap,pdes->channel);
	}
	
	/* bit 8 */
	if(flag & WRITE_AID_REG)
	{
		//Mng_WriteReg16(pAdap, pdes->aid, (UINT16)AID_REG);
	}


/*	if(flag & WRITE_ERP_ENABLE)
	{
		if(Adap_Check11GRates((PUINT8)&pdes->suprate[1],pdes->suprate[0],pdes->ExtendedSupportRate,pdes->ExtendedSupportRateLen))
		{
			tmplong = 0x00080006;
			tmplong += (((UINT32)pt_mattr->erp_element) << 8);
			//Adap_WriteRegDword(pAdap,tmplong,DOT11G_CONFIG);
		}
	}
	*/
	
}


/*******************************************************************
 *   Mng_ExchangeMattrAndMng
 *   
 *   Descriptions:
 *      write value  related with beacon to mattr structure
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      flag: , map of register should be written.
 *      pdes: , description of saving information
 *      direction: , 
 *            0   write from mng to mattr
 *            1   write from mattr to mng
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_ExchangeMattrAndMng(UINT16 flag,PMNG_DES_T pdes,UINT32 direction,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	UINT8			tmp;
	UINT32			i;
	
	/*  write mng--->mattr  */
	if(direction == 0)
	{
	
		/* 1: */
		if(flag & WRITE_BSSID_REG)
		{
			WLAN_COPY_ADDRESS(pt_mattr->bssid, pt_mng->usedbssdes.bssid);
		}
	
		/* 2 */
		if(flag & WRITE_SSID_REG)
		{
			if(pdes->ssid[0] <= WLAN_SSID_MAXLEN) 
			{
				pt_mattr->ssid_len = pdes->ssid[0];
				sc_memory_copy(pt_mattr->ssid, &pdes->ssid[1], pt_mattr->ssid_len);
			}
		}
	
		/* 3 */
		if(flag & WRITE_BEACONINTERVAL_REG)
		{
			pt_mattr->beacon_interval = pdes->beaconinterval;
			pt_mattr->beacon_window = pdes->beaconinterval * 100; 
		}
	
		/* 4 */
		if(flag & WRITE_CAP_REG)
		{
			sc_memory_copy(&pt_mattr->capability, &pdes->cap, sizeof(CAPABILITY_T));

			#ifdef DSP_CTS_TO_SELF
			if(CAP_EXPRESS_SHORT_SLOTTIME(pdes->cap) &&
				(pdes->Erpflag) &&
				!(pdes->Erpinfo & ERP_HAVE_NONERP_STA))
			{
			}
			else
			#else
			{
			}
			#endif /*DSP_CTS_TO_SELF*/
		}
	
		/*  */
		if(flag & WRITE_SPRATE_REG)
		{
			if(pdes->suprate[0] <= WLAN_MAX_RATE_SUPPORT_LEN)
			{
				//copy support
				sc_memory_copy(pt_mattr->support_rate, &pdes->suprate[1], pdes->suprate[0]);
				//get extent support len
				tmp = pdes->suprate[0] + pdes->ExtendedSupportRateLen;
				tmp = (tmp > WLAN_MAX_RATE_SUPPORT_LEN) ?
					(WLAN_MAX_RATE_SUPPORT_LEN - pdes->suprate[0]) :
					pdes->ExtendedSupportRateLen;
				//copy extend support rate
				sc_memory_copy(
					pt_mattr->support_rate + pdes->suprate[0],
					pdes->ExtendedSupportRate,
					tmp);
				
				//DBG_WLAN__MAIN(LEVEL_TRACE ,"pattr->support_rate_len = %d, tmp=%d, suprate[0]=%d\n",pt_mattr->support_rate_len, tmp,pdes->suprate[0]);
				pt_mattr->support_rate_len = pdes->suprate[0] + tmp;
				//DBG_WLAN__MAIN(LEVEL_TRACE ,"pattr->support_rate_len = %d\n",pt_mattr->support_rate_len);
				
				//update tx rate accoridng AP's support rates
				//Jakio20070529: select rate: if rate is zero, use the max support rate,
				//if rate in registery less than the max support rate,
				//use the rate in registery, else use the max support rate
				tmp = pt_mattr->support_rate[pt_mattr->support_rate_len-1] & 0x7f;
				if(pt_mattr->rate == 0)
					pt_mattr->rate = tmp;
				else
				{
				//rate decided by registry. no care support rate of ap
					//pt_mattr->rate = (pt_mattr->rate >= tmp) ? tmp:pt_mattr->rate;
				}
				//pt_mattr->rate = pt_mattr->support_rate[pt_mattr->support_rate_len-1] & 0x7f;// Justin: for join test, use the min data rate. update latter
				//pt_mattr->rate = pt_mattr->support_rate[0] & 0x7f;
			}
			
		}
	
		/* 6 */
		if(flag & WRITE_ATIM_REG)
		{
			pt_mattr->atim_window = 0;
			if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA) && (pdes->atimw <= MAX_ATIM_WINDOW))
			{
				pt_mattr->atim_window = pdes->atimw;
			}
		}
	
	
		/* 7 */
		if(flag & WRITE_CHANNEL_REG)
		{
			pt_mattr->current_channel = pdes->channel;
		}
	
		/* 8 */
		if(flag & WRITE_AID_REG)
		{
			pt_mattr->aid = pdes->aid;
		}

		/* 9 */
/*		if(flag & WRITE_ERP_ENABLE)
		{
			if((pdes->Erpflag) && 
			   (pdes->Erpinfo & ERP_USE_PROTECTION) &&
			   (pt_mattr->macmode == WLAN_MACMODE_ESS_STA))
			{
				pt_mattr->erp_en = 1;
				pt_mattr->erp_element = pdes->Erpinfo;
			}
			else
			{
				pt_mattr->erp_en = 0;
				pt_mattr->erp_element = pdes->Erpinfo;
			}
		}
*/
#ifdef COUNTRY_DOMAIN
		/* country */
		if(flag & WRITE_COUNRTR_REG)
		{
			if((pdes->countryinfo.channelnum <= WLAN_MAX_CHANNEL_LIST) &&
				(pdes->countryinfo.channelnum != 0))
			{
				sc_memory_copy(
					pt_mattr->channel_list,
					pdes->countryinfo.channellist,
					pdes->countryinfo.channelnum);
				
				// find out the len of 11g,,,  add by justin
				pt_mattr->channel_len_of_g = 0;
				for (i=0; i <pdes->countryinfo.channelnum; i++) 
				{
					if (pt_mattr->channel_list[i] <= 0x0e) 
					{
						pt_mattr->channel_len_of_g ++;
					}
				}
				pt_mattr->channel_len_of_a = 
					pdes->countryinfo.channelnum - pt_mattr->channel_len_of_g;
				pt_mattr->channel_num = pdes->countryinfo.channelnum;
				pt_mattr->regoin = pdes->countryinfo.country[0];
			}
		}
#endif

/*		if(flag &WRITE_MAX_BASICRATE)
		{
			pt_mattr->ack_basic_rate = Mng_MaxBasicRate(pAdap,pdes->ExtendedSupportRate,pdes->ExtendedSupportRateLen,(PUINT8)&pdes->suprate[1],pdes->suprate[0]);
		}
*/		

		return;
	}


	/*  write mattr--->mng  */
	{
		/* 1: */
		if(flag & WRITE_BSSID_REG)
		{
			WLAN_COPY_ADDRESS(pdes->bssid, pt_mattr->bssid);
		}
		
		/* 2 */
		if(flag & WRITE_SSID_REG)
		{
			//if(pt_mattr->ssid_len < WLAN_SSID_MAXLEN) 
			if(pt_mattr->ssid_len <= WLAN_SSID_MAXLEN) 
			{
				pdes->ssid[0] = pt_mattr->ssid_len;
				sc_memory_copy(&pdes->ssid[1], pt_mattr->ssid, pt_mattr->ssid_len);
			}
		}
		
		/* 3 */
		if(flag & WRITE_BEACONINTERVAL_REG)
		{
			pdes->beaconinterval = pt_mattr->beacon_interval;
		}
		
		/* 4 */
		if(flag & WRITE_CAP_REG)
		{
			sc_memory_copy(&pdes->cap, &pt_mattr->capability, sizeof(CAPABILITY_T));
		}
		
		/*  */
		if(flag & WRITE_SPRATE_REG)
		{
			if(pt_mattr->support_rate_len < WLAN_MAX_RATE_SUPPORT_LEN)
			{
				pdes->suprate[0] = pt_mattr->support_rate_len;
				sc_memory_copy(&pdes->suprate[1], pt_mattr->support_rate, pt_mattr->support_rate_len);
			}
#if 0			
			if(pt_mattr->ext_support_rate_len!=0 && pt_mattr->ext_support_rate_len<MAX_ESR_LEN)
			{
				pdes->ExtendedSupportRateLen = pt_mattr->ext_support_rate_len;
				sc_memory_copy(pdes->ExtendedSupportRate,pt_mattr->ext_support_rate,MAX_ESR_LEN);
			}
#endif			
	
		}
		
		/* 6 */
		if(flag & WRITE_ATIM_REG)
		{
			if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA) &&(pt_mattr->atim_window <= MAX_ATIM_WINDOW))
			{
				pdes->atimw = pt_mattr->atim_window;
			}
			else
			{
				pdes->atimw = 0;
			}
		}
		
		
		/* 7 */
		if(flag & WRITE_CHANNEL_REG)
		{
			pdes->channel = pt_mattr->current_channel;
		}
		
		/* 8 */
		if(flag & WRITE_AID_REG)
		{
			 pdes->aid = pt_mattr->aid;
		}

		/*country*/
#ifdef COUNTRY_DOMAIN
		/* country */
		if(flag & WRITE_COUNRTR_REG)
		{
			if(pdes->countryinfo.channelnum <= WLAN_MAX_CHANNEL_LIST)
			{
				//pdes->countryinfo.country = (UINT8)pdes->countryinfo.country;
				pdes->countryinfo.channelnum = pt_mattr->channel_num;
				sc_memory_copy(pdes->countryinfo.channellist,pt_mattr->channel_list,pdes->countryinfo.channelnum);
				pdes->countryinfo.country[0] = pt_mattr->regoin;
			}
		}
#endif
	
	}

}
/*******************************************************************
 *   Mng_StartJoin
 *   
 *   Descriptions:
 *      do join to connect to appointed AP. called after mng_oidassign()
 *   be finished.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
 VOID Mng_StartJoin(PDSP_ADAPTER_T pAdap,UINT32 state)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   		pt_mattr = &pAdap->wlan_attr;

	//starve mac
	pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;
	pAdap->bStarveMac = TRUE;

	//set to join state
	pt_mng->statuscontrol.curstatus = JOINNING;

	pt_mng->statuscontrol.worktype = state;
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_StartJoin IBSS\n");
		pt_mng->statuscontrol.worktype = 
			JOIN_STATE_KEEP_WITH_BEACON | JOIN_SUBTYPE_WAIT_CHAN_CFM;
	}
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_StartJoin BSS\n");
	}

	//set init state
	pt_mng->statuscontrol.workcount = JOIN_PROCESS_RETRY_NUM;
	pt_mng->statuscontrol.retrynum = JOIN_CFM_RETRY_NUM;

	if(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA)
	{
		pAdap->wlan_attr.gdevice_info.privacy_option = CAP_EXPRESS_PRIVACY(pt_mng->usedbssdes.cap);	
	}	

	Mng_DoJoin(pAdap, state);
	
}
VOID Mng_DoJoin(PDSP_ADAPTER_T pAdap,UINT32 state)
{
	
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   		pt_mattr = &pAdap->wlan_attr;
	UINT8 				chan;
	UINT32 				loop = VENDOR_LOOP_MIDDLE;
	BOOLEAN				check_idle = FALSE;

	//set retry num for channel swtich and state swtich
	//pt_mng->statuscontrol.retrynum = retrynum;

	//set & verify channel
	if(pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_WAIT_CHAN_CFM)
	{
		//it is necessary to result is same by read reg twice
		chan = Adap_get_channel(pAdap);
		while(chan != Adap_get_channel(pAdap) && loop--)
		{
			chan = Adap_get_channel(pAdap);
		}

		if(loop == 0)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"can't get channel correctly when join \n");
		}

		//assume channel got correct
		//compare both hw channel and appointed channel

		DBG_WLAN__MLME(LEVEL_TRACE,"join channel = %u\n",pt_mng->usedbssdes.channel);
		
		if(chan != pt_mng->usedbssdes.channel)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"current channel is %u\n",chan);
			//set channel
			Adap_set_channel(pAdap,pt_mng->usedbssdes.channel);
			//verify current state, because channel can't be changed with idle status
			if( (UINT32)DEV_IDLE == Adap_get_current_state_control(pAdap))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"Adap_get_current_state_control == idle\n");
				Adap_set_state_control(pAdap,(UINT8)DEV_ACTIVE, 0, 2*pt_mattr->join_timeout);
			}
			goto WAIT_CHANNEL_CFM;
		}
		//channel match
		//check if idle state to set para
		else
		{
			//set retry num for next switch state action
			pt_mng->statuscontrol.retrynum = JOIN_PROCESS_RETRY_NUM;
			check_idle  = TRUE;
		}	
	}
	else if(pt_mng->statuscontrol.worktype & JOIN_SUBTYPE_WAIT_HW_IDLE)
	{
		check_idle = TRUE;
	}
	else
	{
		if(JOIN_STATE_KEEP_WITH_BEACON != pt_mng->statuscontrol.worktype)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"! Join flow with unknow type\n");
		}
		check_idle = TRUE;
	}

	
	//DO check idle for setting para
	if(check_idle)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"check_idle process\n");
		//wait hw idle for setting join parameters
		if( (UINT32)DEV_IDLE != Adap_get_current_state_control(pAdap))
		{
			//set to idle status when current != idle
			Adap_set_state_control(pAdap, (UINT8)DEV_IDLE,0,0);
			goto WAIT_IDLE_CFM;
		}
		//set para with idle state
		else
		{
			//set join pare 
			Adap_set_join_para(pAdap,&pt_mng->usedbssdes);
			//pt_mng->statuscontrol.worktype |= JOIN_SUBTYPE_WAIT_TIMEOUT;		
			goto WAIT_BEGIN_JOIN;
		}				
	}		
	else
	{
		DBG_WLAN__MLME(LEVEL_ERR,"Mng dojoin enter wrong flow\n");
	}
	return;
WAIT_CHANNEL_CFM:
		DBG_WLAN__MLME(LEVEL_TRACE,"WAIT_CHANNEL_CFM process\n");
		pt_mng->statuscontrol.worktype &= JOIN_SUBTYPE_CLEAR_BITS;
		pt_mng->statuscontrol.worktype |= JOIN_SUBTYPE_WAIT_CHAN_CFM;
		Mng_SetTimerPro(MNG_WAIT_HW_IDLE_TIMEOUT, pAdap);
		return;
		
WAIT_IDLE_CFM:
		DBG_WLAN__MLME(LEVEL_TRACE,"WAIT_IDLE_CFM process\n");
		pt_mng->statuscontrol.worktype &= JOIN_SUBTYPE_CLEAR_BITS;
		pt_mng->statuscontrol.worktype |= JOIN_SUBTYPE_WAIT_HW_IDLE;		
		Mng_SetTimerPro(MNG_WAIT_HW_IDLE_TIMEOUT, pAdap);
		return;
		
WAIT_BEGIN_JOIN:
		//set retry num
		DBG_WLAN__MLME(LEVEL_TRACE,"WAIT_BEGIN_JOIN process\n");
		pt_mng->statuscontrol.worktype &= JOIN_SUBTYPE_CLEAR_BITS;
		pt_mng->statuscontrol.worktype |= JOIN_SUBTYPE_WAIT_BEACON;
		//Mng_SetTimerPro(3* pt_mattr->join_timeout, pAdap);
		Mng_SetTimerPro( pt_mattr->join_timeout, pAdap);
		return;
}

/*******************************************************************
 *   Mng_MidJoinUntilSuccess
 *   
 *   Descriptions:
 *      called by other module.
 *      keep doing join not considering timeout
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_MidJoinUntilSuccess(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T             pt_mattr = &pAdap->wlan_attr;

	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_MidJoinUntilSuccess\n");
	
#ifdef DEBUG_OPEN
	if (pt_mng->usedbssdes.ssid[0] < 32)
	{
		INT8		ssid_print[32 + 1];
		sc_memory_copy(ssid_print, &pt_mng->usedbssdes.ssid[1],pt_mng->usedbssdes.ssid[0]);
		ssid_print[pt_mng->usedbssdes.ssid[0]]='\0';		
		DBG_WLAN__MLME(LEVEL_TRACE,"ssid=\'%s\'\n",ssid_print);
		
		sc_memory_copy(ssid_print, &pt_mng->usedbssdes.ssid[1],pt_mng->usedbssdes.oldssid[0]);
		ssid_print[pt_mng->usedbssdes.oldssid[0]]='\0';		
		DBG_WLAN__MLME(LEVEL_TRACE,"oldssid=\'%s\'\n",ssid_print);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
			pt_mng->usedbssdes.bssid[0],pt_mng->usedbssdes.bssid[1],pt_mng->usedbssdes.bssid[2],
			pt_mng->usedbssdes.bssid[3],pt_mng->usedbssdes.bssid[4],pt_mng->usedbssdes.bssid[5]);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "oldBssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
			pt_mng->usedbssdes.oldBssid[0],pt_mng->usedbssdes.oldBssid[1],pt_mng->usedbssdes.oldBssid[2],
			pt_mng->usedbssdes.oldBssid[3],pt_mng->usedbssdes.oldBssid[4],pt_mng->usedbssdes.oldBssid[5]);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
			pt_mng->usedbssdes.addr[0],pt_mng->usedbssdes.addr[1],pt_mng->usedbssdes.addr[2],
			pt_mng->usedbssdes.addr[3],pt_mng->usedbssdes.addr[4],pt_mng->usedbssdes.addr[5]);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "channel=%u, beaconinterval=%u, atimw=%u\n",
			pt_mng->usedbssdes.channel,pt_mng->usedbssdes.beaconinterval,pt_mng->usedbssdes.atimw);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "cap=0x%04x, aid=%u, rssi=%d\n",
			pt_mng->usedbssdes.cap,pt_mng->usedbssdes.aid,pt_mng->usedbssdes.rssi);
		
		DBG_WLAN__MLME(LEVEL_TRACE, "suprate[0]=%u, ExtendedSupportRateLen=%u, IELength=%u\n",
			pt_mng->usedbssdes.suprate[0],pt_mng->usedbssdes.ExtendedSupportRateLen,pt_mng->usedbssdes.IELength);		
	}
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE, "%s: usedbssdes.ssid len = %d\n", __FUNCTION__, pt_mng->usedbssdes.ssid[0]);
	}
#endif

//return with no ssid
	if(pt_mng->usedbssdes.ssid[0] == 0)
	{
		DBG_WLAN__MLME(LEVEL_ERR,"SSID Length is 0,return!!\n");
		return;
	}
	
	//ibss beacon lost
	if(pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		DBG_WLAN__MLME(LEVEL_ERR,"JOIN IBSS\n");
		pt_mng->statuscontrol.curstatus = JOINNING;
		pt_mng->statuscontrol.worktype = JOIN_STATE_KEEP_WITH_BEACON |JOIN_SUBTYPE_WAIT_BEACON ;
		//indicate to disconnect
		//debug ibss
		Mng_StartJoin(pAdap,JOIN_STATE_KEEP_WITHOUT_BEACON | JOIN_SUBTYPE_WAIT_CHAN_CFM);

	}
	//bss beaconlost
	else
	{
		DBG_WLAN__MLME(LEVEL_ERR,"JOIN BSS\n");
		Mng_StartJoin(pAdap,JOIN_STATE_KEEP_WITHOUT_BEACON | JOIN_SUBTYPE_WAIT_CHAN_CFM);
	}
	/* Mng_SetTimerPro(pt_mattr->join_timeout, pAdap); */
}

/*******************************************************************
 *   Mng_ResourceAvailable
 *   
 *   Descriptions:
 *      restore
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_ResourceAvailable(PDSP_ADAPTER_T pAdap)
{
	//PNDIS_PACKET Packet;
	//TDSP_STATUS Status;

//	Phy_WriteCMReg(pAdap, 0x48, 0x3c); // If station associates with AP, driver crease received limited value.
	
	DBG_WLAN__MLME(LEVEL_TRACE,"Ending scaning.\n");
	
	pAdap->is_oid_scan_begin_flag = FALSE;
	
	/* Resend the packet saved into the tx packet list. */
	/* Packet = PktList_RemoveHead((PDSP_PKT_LIST_T)pAdap->ptx_packet_list);
	while (Packet)
	{
		DBG_WLAN__MLME(("Sending Queued Tx Packets\n"));
		// Retry to transmit previously queued packets
		// Status = Adap_Transmit(pAdap,Packet);
		Status = Adap_ReSend(pAdap,Packet);
		if (Status == TDSP_STATUS_RESOURCES)
		{
			// The packet can not be transmited because of a lack of resources
			// so requeue it for later
			if (PktList_InsertHead((PDSP_PKT_LIST_T)pAdap->ptx_packet_list,Packet) != STATUS_SUCCESS)
				NdisMSendComplete(pAdap->dsp_adap_handle, Packet, STATUS_FAILURE);
			
			Packet = NULL;
		}
		else if (Status == STATUS_ADAPTER_NOT_READY)
		{
			NdisMSendComplete(pAdap->dsp_adap_handle, Packet, STATUS_SUCCESS);
			Packet = PktList_RemoveHead((PDSP_PKT_LIST_T)pAdap->ptx_packet_list);
		}
		else if (Status == TDSP_STATUS_NOT_ACCEPTED)
		{
			NdisMSendComplete(pAdap->dsp_adap_handle, Packet, STATUS_SUCCESS);
			Packet = PktList_RemoveHead((PDSP_PKT_LIST_T)pAdap->ptx_packet_list);
		}
		else
		{
			// Get the next queued packet
			Packet = PktList_RemoveHead((PDSP_PKT_LIST_T)pAdap->ptx_packet_list);
		}
	} */
	Tx_Send_Next_Frm(pAdap);
//wumin	wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);
//	if (pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
//	{
//		Phy_WriteCMReg(pAdap, 0x48, 0x3c); // If station associates with AP, driver crease received limited value.
//	}

}

/*******************************************************************
 *   Mng_OidAssignSsid
 *   
 *   Descriptions:
 *      called by other module
 *      get a description from list got by scan
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      ssid: , appoint ssid of the description
 *      len: , appoint length of ssid
 *   Return Value:
 *      0:   no right description
 *      1:   get a right description
 *      2:   description got is mismathc with driver in wep algothim
 ******************************************************************/
UINT32 Mng_OidAssignSsid(PUINT8 pssid,UINT8 len,BOOLEAN cmp_bssid,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     	pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;

	PMNG_DES_T		pdes = NULL;
	PMNG_DES_T	ptmpDes;
	UINT32			i;
	BOOLEAN             	infra_match_flag;
	BOOLEAN             	privacy_match_flag = FALSE;
	BOOLEAN             	ssid_match_flag;
	PMNG_NOFIXED_ELE_ALL_T  pWpaIe = NULL;
	PMNG_NOFIXED_ELE_ALL_T  pWpa2Ie = NULL;
	
	if(len <= WLAN_SSID_MAXLEN)
	{
		Mng_InitParas(pAdap);
		
		/* arbitrary select a description */
		pdes = NULL;
		for(i = 0; i < pt_mng->oiddeslist.bssdesnum; i++)
		{
			ptmpDes = &(pt_mng->oiddeslist.bssdesset[i]);
			//check if match in infra mode
			infra_match_flag = IS_MATCH_IN_ESSMODE(pt_mattr->macmode,pt_mng->oiddeslist.bssdesset[i].cap);

			//check if match in privacy mode
			privacy_match_flag = ((CAP_EXPRESS_PRIVACY(pt_mng->oiddeslist.bssdesset[i].cap) && 
									(pt_mattr->gdevice_info.privacy_option == TRUE)) ||
									(!CAP_EXPRESS_PRIVACY(pt_mng->oiddeslist.bssdesset[i].cap) && 
									(pt_mattr->gdevice_info.privacy_option == FALSE)));

			//update privacy match flag after check if match in privacy mode for wpa rsn element
			if(privacy_match_flag && 
				(pt_mattr->wep_mode != WEP_MODE_WEP) && 
				(pt_mattr->gdevice_info.privacy_option == TRUE))
			{

				
				if(ptmpDes->offset_wpa < ptmpDes->IELength)
					pWpaIe = (PMNG_NOFIXED_ELE_ALL_T)(ptmpDes->IEs + ptmpDes->offset_wpa);
				else
					pWpaIe = NULL;
				
				if(ptmpDes->offset_wpa2 < ptmpDes->IELength)
					pWpa2Ie = (PMNG_NOFIXED_ELE_ALL_T)(ptmpDes->IEs + ptmpDes->offset_wpa2);
				else
					pWpa2Ie = NULL;
				
				privacy_match_flag = FALSE;


				//Jakio2006.12.18: add for wpa2
				if((pWpaIe != NULL) && 
					(pWpaIe->element == WPA1_EID) &&
					(Mng_MatchWPA1_FromRsn(pAdap, (PUINT8)pWpaIe, pt_mattr->wep_mode)))
				{
					privacy_match_flag = TRUE;
				}
				#ifdef DSP_WPA2
				else if((pWpa2Ie != NULL) && 
					(pWpa2Ie->element == WPA2_EID) &&
					(Mng_MatchWPA2_FromRsn(pAdap, (PUINT8)pWpa2Ie, pt_mattr->wep_mode)))
				{
					privacy_match_flag = TRUE;
				}
				#endif /*DSP_WPA2*/

			}
				
			//always set ssid is match with len = 0
			ssid_match_flag = (len == 0) ? TRUE: FALSE;
			//update ssid match flag after compare ssid while ssid len !=0
			if(FALSE == ssid_match_flag)
			{
				ssid_match_flag = ((len == pt_mng->oiddeslist.bssdesset[i].ssid[0])  &&	
					(0 == sc_memory_cmp(pssid, &pt_mng->oiddeslist.bssdesset[i].ssid[1], len)));
			}

			//find a ap
			if(infra_match_flag && ssid_match_flag)
			{
				//return the ap's info when 1:no care bssid or   or 2: equal with bssid of driver
				if((len ==0) ||
					(FALSE == cmp_bssid) || 
					(IS_MATCH_IN_ADDRESS(pt_mattr->bssid,pt_mng->oiddeslist.bssdesset[i].bssid)))
				{
					pdes = &pt_mng->oiddeslist.bssdesset[i];           /* return a valid bssdes for join*/
					break;	
				}
					
			}
		}
	}

	//copy found desc to used descriptor
	if( pdes != NULL)
	{
		//
		sc_memory_copy(&pt_mng->usedbssdes, pdes, sizeof(MNG_DES_T));
		
		DBG_WLAN__MLME(LEVEL_TRACE,"copy des in mng_oidassignssid bssid = %x %x %x %x %x %x \n",
			pt_mng->usedbssdes.bssid[0],pt_mng->usedbssdes.bssid[1],pt_mng->usedbssdes.bssid[2],
			pt_mng->usedbssdes.bssid[3],pt_mng->usedbssdes.bssid[4],pt_mng->usedbssdes.bssid[5]);
	}	


	if((pdes != NULL) && !privacy_match_flag)
	{
		return ASSIGN_SSID_MISMACTH_PRIVACY;
	}
	else if(pdes != NULL)
	{
		return ASSIGN_SSID_FOUND;
	}
	
	return ASSIGN_SSID_NO_FOUND;
}
/*******************************************************************
 *   Mng_OidScanWithSsid
 *   
 *   Descriptions:
 *      do scan with an appointed ssid and stop the action once find
 *      corresponding AP.
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_OidScanWithSsid(PDSP_ADAPTER_T pAdap)
{
	
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T      pt_mattr = &pAdap->wlan_attr;
	UINT32 		     state;
	
	//Justin: 0706. advoid that has been doing another mng.
	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_OidScanWithSsid\n");
	
	Mng_ClearTimerPro(pAdap);
	pAdap->scan_watch_value = 0;
	
	if(	pt_mng->usedbssdes.ssid[0] != 0 
	&&	pt_mng->usedbssdes.ssid[0] <= (WLAN_SSID_MAXLEN ))
	{
#ifdef DEBUG_OPEN
		if (pt_mng->usedbssdes.ssid[0] < WLAN_SSID_MAXLEN)
		{
			INT8		ssid_print[32 + 1];
			sc_memory_set(ssid_print, 0, sizeof(ssid_print));
			sc_memory_copy(ssid_print, &pt_mng->usedbssdes.ssid[1],pt_mng->usedbssdes.ssid[0]);
			DBG_WLAN__IOCTL(LEVEL_TRACE, "SCAN WITH SSID. len = %d, SSID=\'%s\'\n",pt_mng->usedbssdes.ssid[0], ssid_print);
		}
		else
		{
			DBG_WLAN__IOCTL(LEVEL_TRACE, "%s:SCAN WITH SSID. SSID len = %d\n", __FUNCTION__, pt_mng->usedbssdes.ssid[0]);
		}
#endif
		//wumin: 090226, need to set this flag before doscan
		pAdap->is_oid_scan_begin_flag =TRUE;

		pAdap->bStarveMac = TRUE;
		//copy ssid info to pt_mattr because mng module get ssid from pt_mattr when building probe request frame
		pt_mattr->ssid_len = pt_mng->usedbssdes.ssid[0];
		sc_memory_copy(pt_mattr->ssid, &pt_mng->usedbssdes.ssid[1], pt_mattr->ssid_len);

		state = SCAN_STATE_SSID_FLAG |//SCAN_STATE_OID_FLAG |		//Justin: 0525 remark
			SCAN_STATE_ACTIVE_FLAG | SCAN_STATE_MID_STOP_FLAG
			| SCAN_STATE_11G_FLAG;	//Justin: 0525    add for test
		Mng_StartScan(state,pAdap);

		return;
	}

	DBG_WLAN__IOCTL(LEVEL_ERR, "SCAN WITH SSID Error. length=%u\n",pt_mng->usedbssdes.ssid[0]);

	return;
}

/*******************************************************************
 *   Mng_GetBssList
 *   
 *   Descriptions:
 *      called by other module
 *      get description list 
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      bsslist: , the pointer to the list
 *      num: , number of description in list
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_GetBssList(PMNG_DES_T *bsslist,UINT32 *num,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	if(pt_mng->oiddeslist.bssdesnum > WLAN_BSSDES_NUM)
		pt_mng->oiddeslist.bssdesnum = WLAN_BSSDES_NUM;

//	BssTableSortByRssi(&pt_mng->oiddeslist);
	
	*num = pt_mng->oiddeslist.bssdesnum;
	*bsslist = pt_mng->oiddeslist.bssdesset;
	
	return;
}
/*******************************************************************
 *   Mng_GetCurCh
 *   
 *   Descriptions:
 *      called by other module
 *      get current channel of work
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
UINT8 Mng_GetCurCh(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	return pt_mng->usedbssdes.channel;
}

/*******************************************************************
 *   Mng_InitMgt
 *   
 *   Descriptions:
 *      init the managment module
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
/*VOID Mng_InitMgt(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	pt_mng->ibssinfo.num = 0;
	pt_mng->statuscontrol.curstatus = IDLE;
	pt_mng->statuscontrol.retrynum = 0;
	pt_mng->bssdeslist.bssdesnum = 0;
	pt_mng->beacondeslist.bssdesnum = 0;
	pt_mng->usedbssdes.ssid[0] = 0;
}
*/
/*******************************************************************
 *   Mng_MakeReAsocReqFrame
 *   
 *   Descriptions:
 *      build up reassociation frame
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
TDSP_STATUS Mng_MakeReAsocReqFrame(PDSP_ADAPTER_T pAdap)
{	
#if 0
	UINT16		offset;
	UINT8 * addr1 = NULL;
	UINT8 * addr3 = NULL;
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   			pt_mattr = &pAdap->wlan_attr;
	PMNG_REASSOCREQ_DATA_T	pReAssocData;
	PUSBWLAN_HW_FRMHDR_TX  hw_header;
	MNG_NOFIXED_ELE_ALL_T		pele;

	/*hw header pos*/
	hw_header = (PUSBWLAN_HW_FRMHDR_TX)(pt_mng->tranbuf);
	offset = sizeof(USBWLAN_HW_FRMHDR_TX);

	/*Reassoc body*/
	pReAssocData = (PMNG_REASSOCREQ_DATA_T)(pt_mng->tranbuf + offset);
	pReAssocData->cap = pt_mng->usedbssdes.cap;
	pReAssocData->listenInt = pt_mattr->listen_interval;
	sc_memory_copy(pReAssocData->apaddr, pt_mng->olddes.addr, WLAN_BSSID_LEN);
	offset += sizeof(MNG_REASSOCREQ_DATA_T);

	/*ssid*/
	WriteElement(SSID_EID, pt_mng->tranbuf + offset, (PUINT8)&pt_mng->usedbssdes.ssid[1], pt_mng->usedbssdes.ssid[0]);
	offset += pt_mng->usedbssdes.ssid[0] + sizeof(MNG_NOFIXED_ELE_HEAD_T);

	/* support rate */
	WriteElement(SUPRATE_EID, pt_mng->tranbuf + offset, (PUINT8)&pt_mng->usedbssdes.suprate[1], pt_mng->usedbssdes.suprate[0]);
	offset += pt_mng->usedbssdes.suprate[0] + sizeof(MNG_NOFIXED_ELE_HEAD_T);

	if(pt_mattr->gdevice_info.dot11_type == IEEE802_11_G)
	{
		if(pt_mng->usedbssdes.ExtendedSupportRateLen != 0)
		{
			WriteElement(EXTENDRATE_EID, pt_mng->tranbuf + offset, (PUINT8)&pt_mng->usedbssdes.ExtendedSupportRate,pt_mng->usedbssdes.ExtendedSupportRateLen);
			offset += pt_mng->usedbssdes.ExtendedSupportRateLen + sizeof(MNG_NOFIXED_ELE_HEAD_T);
		}
	}

	//Jakio2006.11.28: moved codes here below


	/*Jakio: add WPA codes, copied from project ""*/
#ifdef DSP_WPA
	{
	
	PMNG_NOFIXED_ELE_HEAD_T		pnofixed;
    	PMNG_RSNFIXED_T			prsn;
	PMNG_RSN_COUNT_T		pcount;
	PMNG_RSN_SUITE_T		psuite;
	UINT16					rsnLen = 0;
	
	
	pt_mng->reqRspInfo.RequestIELength = 0;
	pt_mng->reqRspInfo.AvailableRequestFixedIEs = (UINT16)0;
	pt_mng->reqRspInfo.reqIe.Capabilities = pt_mng->usedbssdes.cap;
	pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_CAPABILITIES;
	pt_mng->reqRspInfo.reqIe.ListenInterval = pt_mattr->listen_interval;
	pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_LISTENINTERVAL;
	sc_memory_copy(pt_mng->reqRspInfo.reqIe.CurrentAPAddress, pt_mng->usedbssdes.addr, WLAN_BSSID_LEN);
	pt_mng->reqRspInfo.AvailableRequestFixedIEs |= (UINT16)MNDIS_802_11_AI_REQFI_CURRENTAPADDRESS;


	/* aes */
	if(pt_mattr->wep_mode == WEP_MODE_AES)
	{
		pnofixed =(PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pt_mng->tranbuf + offset);	
		offset += sizeof(MNG_NOFIXED_ELE_HEAD_T);
		prsn = (PMNG_RSNFIXED_T)(pt_mng->tranbuf + offset);

		/*no fixed contents*/
		pnofixed->element = RSN_EID;
		prsn->oui = RSNOUI;
		prsn->version = RSNVERSION;
		prsn->groupSuite = KEY_SUITE_CCMP;
		
		rsnLen = sizeof(MNG_RSNFIXED_T);
		pcount = (PMNG_RSN_COUNT_T)(pt_mng->tranbuf + offset + rsnLen);
		/* unicast len */
		pcount->count = 0x0001;   
		
		rsnLen += sizeof(MNG_RSN_COUNT_T);
		psuite = (PMNG_RSN_SUITE_T)(pt_mng->tranbuf + offset + rsnLen);
		/* unicast suite */
		psuite->suite = KEY_SUITE_CCMP;
		
		rsnLen += sizeof(MNG_RSN_SUITE_T);
		pcount = (PMNG_RSN_COUNT_T)(pt_mng->tranbuf + offset + rsnLen);
		/* auth len */
		pcount->count = 0x0001;
		
		rsnLen += sizeof(MNG_RSN_COUNT_T);
		psuite = (PMNG_RSN_SUITE_T)(pt_mng->tranbuf + offset + rsnLen);
		/* auth suite */
		if(pt_mattr->auth_mode == AUTH_MODE_WPA)
		{
			psuite->suite = AES_AUTH_SUITE_RSN;
		}
		/* psk */
		else if(pt_mattr->auth_mode == AUTH_MODE_WPA_PSK)
		{
			psuite->suite = AUTH_SUITE_RSN_PSK;
		}
		else
		{
			psuite->suite = 0;
		}
		
		rsnLen += sizeof(MNG_RSN_SUITE_T);
		/* RSN cap */
		/*

		*(UINT16 *)(pt_mng->tranbuf + offset + rsnLen) = 0x0000;
		rsnLen += sizeof(UINT16);

		*/
		pnofixed->length = (UINT8)rsnLen;
		offset += rsnLen;
	}
	/* tkip */
	else if(pt_mattr->wep_mode == WEP_MODE_TKIP)
	{
		//
		pnofixed =(PMNG_NOFIXED_ELE_HEAD_T)((PUINT8)pt_mng->tranbuf + offset);	
		offset += sizeof(MNG_NOFIXED_ELE_HEAD_T);
		prsn = (PMNG_RSNFIXED_T)(pt_mng->tranbuf + offset);
		//
		pnofixed->element = RSN_EID;
		prsn->oui = RSNOUI;
		prsn->version = RSNVERSION;
		prsn->groupSuite = KEY_SUITE_TKIP;

		rsnLen = sizeof(MNG_RSNFIXED_T);
		pcount = (PMNG_RSN_COUNT_T)(pt_mng->tranbuf + offset + rsnLen);
		/* unicast len */
		pcount->count = 0x0001;   
		
		rsnLen += sizeof(MNG_RSN_COUNT_T);
		psuite = (PMNG_RSN_SUITE_T)(pt_mng->tranbuf + offset + rsnLen);
		/* unicast suite */
		psuite->suite = KEY_SUITE_TKIP;
		
		rsnLen += sizeof(MNG_RSN_SUITE_T);
		pcount = (PMNG_RSN_COUNT_T)(pt_mng->tranbuf + offset + rsnLen);
		/* auth len */
		pcount->count = 0x0001;
		
		rsnLen += sizeof(MNG_RSN_COUNT_T);
		psuite = (PMNG_RSN_SUITE_T)(pt_mng->tranbuf + offset + rsnLen);
		/* auth suite */
		if( pt_mattr->auth_mode == AUTH_MODE_WPA)
		{
			psuite->suite = AUTH_SUITE_RSN;
		}
		/* psk */
		else if(pt_mattr->auth_mode == AUTH_MODE_WPA_PSK)
		{
			psuite->suite = AUTH_SUITE_RSN_PSK;
		}
		else
		{
			psuite->suite = 0;
		}


		rsnLen += sizeof(MNG_RSN_SUITE_T);

		/* RSN cap */
		/*

		*(UINT16 *)(pt_mng->tranbuf + offset + rsnLen) = 0x0000;
		rsnLen += sizeof(UINT16);

		*/

		pnofixed->length = (UINT8)rsnLen;
		offset += rsnLen;

	}
	/* other mode instead of aes, tkip */
	else
	{
		/*  */
	}
	
	//pnofixed->length = (UINT8)rsnLen;
	//offset += rsnLen;

	pt_mng->reqRspInfo.RequestIELength = (UINT32)(offset - sizeof(MNG_ASSOCREQ_DATA_T) - sizeof(USBWLAN_HW_FRMHDR_TX));
	//pt_mng->reqRspInfo.RequestIELength = min(pt_mng->reqRspInfo.RequestIELength, MAX_ASSOC_REQ_NOFIXED_NUM);
	sc_memory_copy(pt_mng->reqRspInfo.ReqBuff, pt_mng->tranbuf + sizeof(MNG_ASSOCREQ_DATA_T) + sizeof(USBWLAN_HW_FRMHDR_TX) ,pt_mng->reqRspInfo.RequestIELength);

	}
#endif /*Endof  DSP_WPA*/

	//*************************************************************
	//Jakio2006.11.28: move codes here, otherwise pt_mng->txlen is uncorrect.
	//*************************************************************
	/*tx length*/
	pt_mng->txlen = offset;
	/*fill common info of hw header*/
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_MGMT, WLAN_FSTYPE_REASSOCREQ, 
			pt_mng->txlen - sizeof(USBWLAN_HW_FRMHDR_TX), 0);
	//fill addr1 and addr3
	addr1 = addr3 = pt_mng->usedbssdes.bssid;
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
       hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
       hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
       hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);
	//************************************************************
	//endof Jakio2006.11.28
	//*************************************************************

	/* send */
	return Mng_SendPacket(pt_mng->tranbuf, pt_mng->txlen, pAdap);
#endif
	return STATUS_SUCCESS;
}


/*********************************************************************
  *Description:
  *	used in ps module, Null frame is data frame with null body, we make use of mng's
  *	tx_queue, and make null frame in the mng module
**********************************************************************/
TDSP_STATUS  Mng_MakeNullFrame(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T	pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;
	PUSBWLAN_HW_FRMHDR_TX  hw_header;
	PUINT8 addr1, addr3;
	UINT8  privacy;
	hw_header  = (PUSBWLAN_HW_FRMHDR_TX)(pt_mng->tranbuf);

	//Jakio2006.12.15: for null frame, we don't encrypt it
	//privacy = (UINT8)(pt_mattr->gdevice_info.privacy_option);
	privacy =  0;
	/*fill common info of hw header*/
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_DATA, WLAN_FSTYPE_NULL, 
			0, privacy);

	//fill addr, 
	addr1 = addr3 = pt_mng->usedbssdes.bssid;
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
       hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
	hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
       hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

	pt_mng->txlen = sizeof(USBWLAN_HW_FRMHDR_TX);
	/* send */
	return Mng_SendNonMngFrame(pt_mng->tranbuf, TX_MNG_TYPE_NULLFRM, pt_mng->txlen, pAdap);
}





/******************************************************************
  *Jakio: make ps-poll frame, this is not the standard way, because ps-poll is control
  *	frame. In order to make use of the mng's tx_queue, I make the ps-poll frame 
  *	and insert it to the mng tx_queue
  *To be fixed later
  *Differencess between ps-poll and other mng frames:
  *	 1). AID field rather than txDuration 
  *	 2). addr3 is current mac address
  *    3).  body length is zero
  *	 4). does not have addr3
  *
  *please check it, Justin. I'm not sure about it.
  ******************************************************************/
TDSP_STATUS Mng_MakePsPollFrame(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T	pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;
	PUSBWLAN_HW_FRMHDR_TX  hw_header;
	PUINT8 addr1;//, addr3;
	hw_header  = (PUSBWLAN_HW_FRMHDR_TX)(pt_mng->tranbuf);
	
	//fill the ps-poll frame as spec "MACHardwareUserManual"

	/*fill common info of hw header*/
	Mng_fill_hw_header(pAdap, hw_header, NO_TODS_BIT, WLAN_FTYPE_CTL, WLAN_FSTYPE_PSPOLL, 
			0, 0);

	//fill addr, 
	addr1 = pt_mng->usedbssdes.bssid;
	//Jakio2006.11.28: which address should be use? pAdap->current_address or pt_mattr->dev_addr ??
	//Jakio2006.11.29: ps-poll frame does not have addr3 and sequence number
	//addr3 = pAdap->current_address;
	hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
	hw_header->addr1lo_txdurid |= pt_mng->usedbssdes.aid;
       hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) | (addr1[3] << 8)  | addr1[2]);
	//hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) | (addr3[1] << 8)  | addr3[0]);
       //hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

	pt_mng->txlen = sizeof(USBWLAN_HW_FRMHDR_TX) - 2*sizeof(UINT32);
	/* send */
	return Mng_SendNonMngFrame(pt_mng->tranbuf, TX_MNG_TYPE_PSPOLL, pt_mng->txlen, pAdap);
	
}




/*******************************************************************
 *   Mng_GetPsStatus
 *   
 *   Descriptions:
 *      called by other module to get ps status of station
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *      addr: mac addr of station
 *   Return Value:
 *      none
 ******************************************************************/
UINT32 Mng_GetPsStatus(PUINT8 paddr,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T       pt_mattr = &pAdap->wlan_attr;
	
	UINT32 i;


	if(pt_mattr->atim_window == 0)
		return WLAN_ACTIVESTATE;

	/* broadcast frame */
	if (MAC_ADDR_IS_GROUP(paddr))
		return WLAN_POWERSAVESTATE;
	
	/* unicast frame */
	for(i = 0; i < pt_mng->ibssinfo.num; i++)
	{
		if(IS_MATCH_IN_ADDRESS(paddr, pt_mng->ibssinfo.ibsssta[i].addr))
		{
			if(pt_mng->ibssinfo.ibsssta[i].ps)
			{
				return WLAN_POWERSAVESTATE;
			}
			else
			{
				return WLAN_ACTIVESTATE;
			}	
		}
	}
	return WLAN_POWERSAVESTATE;
}

/*******************************************************************
 *   Mng_DisConnect
 *   
 *   Descriptions:
 *      the routine called when deauth or disassoc frame were recieved
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_DisConnect(PDSP_ADAPTER_T pAdap)
{
	//Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_DIRECTLY_DISCONNECTED,DSP_TASK_PRI_HIGH,NULL,0);

	pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;
	Adap_SetLink(pAdap, LINK_FALSE);
		
	Mng_InitParas(pAdap);
	Mng_Fail(pAdap);
	sc_sleep(5);
	Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_DIRECTLY_DISCONNECTED,DSP_TASK_PRI_HIGH,NULL,0);
	
}

/*******************************************************************
 *   Mng_BssDisAssoc
 *   
 *   Descriptions:
 *      the routine called when deauth or disassoc frame were recieved
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_BssDisAssoc(PDSP_ADAPTER_T pAdap,PUINT8 disassoc_bssid,UINT16 reason)
{
	//PMNG_STRU_T			pt_mng = pAdap->pmanagment;
		
	Mng_DisAssocFrame(pAdap,disassoc_bssid,reason);
	Mng_AuthFail(pAdap);
//	Adap_reset_conn_para(pAdap);		//Justin: 0723....it may cause privacy confused while we do disassociate in set_ssid procedure
	pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;
	pAdap->link_ok = LINK_FALSE;

#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status == DOING_RECONNECT
		||pAdap->reconnect_status == DOING_DISCONNECT)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Mng_BssDisAssoc,  --> NO RECONNECT\n");
		pAdap->reconnect_status = NO_RECONNECT;
	}
#endif	

#ifdef ROAMING_SUPPORT1
	DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Mng_BssDisAssoc----> zero bssid\n");
	sc_memory_set(pt_mng->usedbssdes.addr, 0 ,WLAN_ETHADDR_LEN);
	sc_memory_set(pAdap->wlan_attr.bssid, 0, WLAN_ETHADDR_LEN);
#endif	
}


/*******************************************************************
 *   Mng_InitParas
 *   
 *   Descriptions:
 *      init the managment module
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      none
 ******************************************************************/
VOID Mng_InitParas(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	pt_mng->ibssinfo.num = 0;
	pt_mng->statuscontrol.curstatus = IDLE;
	pt_mng->statuscontrol.retrynum = 0;
	pt_mng->bssdeslist.bssdesnum = 0;		//justin: 080423.	only init bssdesnum in oid scan, so don't set to 0 here. other wise 
	pt_mng->beacondeslist.bssdesnum = 0;
//	pt_mng->usedbssdes.ssid[0] = 0;		//justin:	071217.	can't clear ssid of usedbssdes, if may invalid ssid after send disassoc frame which made by ourselve follow a set_ssid_oid
	pt_mng->flag11g = 1;

//DBG_WLAN__MLME(("^^^^Mng_InitParas^^^^^\n "));	

#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status != NO_RECONNECT)
	{
		return;
	}
#endif	//#ifdef ROAMING_SUPPORT

	if(pAdap->wlan_attr.wep_mode == WEP_MODE_WEP)
	{
		if(pAdap->wlan_attr.gdevice_info.privacy_option == FALSE)
		{
			pAdap->wlan_attr.wpa_group_key_valid = 0;
			pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
		}	
	}
	else
	{
		//pAdap->wlan_attr.gdevice_info.privacy_option = FALSE;
		pAdap->wlan_attr.wpa_group_key_valid = 0;
		pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
	}
	
}
/*******************************************************************
 *   WriteElement
 *   
 *   Descriptions:
 *      write a element of frame
 *   Arguments:
 *      elementId: id of element
 *      elementAddr: begin address of element
 *      contentAddr: begin address of content field of element
 *      contentLen: length of content field of element   
 *   Return Value:
 *      none
 ******************************************************************/
VOID WriteElement(UINT8 elementId,PMNG_NOFIXED_ELE_ALL_T elementAddr,PUINT8 contentAddr,UINT8 contentLen) 
{
	//PMNG_NOFIXED_ELE_ALL_T pele; 
	//pele = elementAddr; 
	elementAddr->element = elementId; 
	elementAddr->length = contentLen; 
	sc_memory_copy(elementAddr->data, contentAddr, contentLen); 
}

/*******************************************************************
 *   Mng_GetAssocInfoStructure
 *   
 *   Descriptions:
 *      get association information
 *   Arguments:
 *      pAdap: , the pointer of adapter context.
 *   Return Value:
 *      pointer to association information structure
 *      none
 ******************************************************************/

PREQ_RSP_FIXEDIES Mng_GetAssocInfoStructure(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	return &pt_mng->reqRspInfo;
}




BOOLEAN Mng_Merge(PDSP_ADAPTER_T pAdap)
{
	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_Merge\n");
#if 0
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	BOOLEAN status = FALSE;
	MNG_DES_T curdes;
	UINT32     flag;
	

	
	typedef union {
		UINT32      t32[2];
		UINT64    t64;
	}TIMESTAMP;

	TIMESTAMP regtime;
	TIMESTAMP frametime;
	
		
	
	regtime.t32[0] = Mng_ReadReg32(pAdap,(UINT16)(TSF_REG));     //low 4 
	regtime.t32[1] = Mng_ReadReg32(pAdap,(UINT16)(TSF_REG + 4));  //high 4

	frametime.t32[0]=*(UINT32 *)(pt_mng->recbuf+WLAN_HDR_A3_LEN);//low 4 byte
	frametime.t32[1]=*(UINT32 *)(pt_mng->recbuf+WLAN_HDR_A3_LEN+4);//high 4 byte


	//compare high 4 bytes
	if( regtime.t64 > frametime.t64)
	{
		if((regtime.t64 - frametime.t64) < MERGE_TIME)
			flag = 1;
		else
			flag = 0;
	}
	else
	{
		flag = 1;
	}

	flag = 1;


	//begin merge
	if(flag == 1)
	{
		if(!Mng_GetBeaconInfo(&curdes,pAdap))
			return FALSE;

		sc_memory_copy(&pt_mng->usedbssdes,&curdes,sizeof(MNG_DES_T));
		Task_CreateTaskForce((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_BEACON_LOST,DSP_TASK_PRI_NORMAL,NULL,0);
		return TRUE;
	}
#endif	
	return FALSE;
}

#if 0
UINT8 Mng_MaxBasicRate(PDSP_ADAPTER_T pAdap,PUINT8 exrate,UINT8 exlen,PUINT8 suprate,UINT8 suplen)
{

	//update it later for rate,woody
	UINT8 i;
	UINT8 ackrate;
	UINT8 rate1,rate2;
	UINT8 maxbasicrate;
	

	//ex rate
	if(exlen > 8)
		exlen = 8;
	
	rate1 = 0;

	if (exlen != 0)
	{
		for(i=0; i < exlen; i++)
		{
			if( exrate[exlen - i - 1] & 0x80)
			{
				rate1 = exrate[exlen - i - 1];
				break;
			}
		}
	}
	

	//suprate
	if(suplen >= WLAN_MAX_RATE_SUPPORT_LEN)
		suplen = WLAN_MAX_RATE_SUPPORT_LEN;
	
	rate2= 0;

	if (suplen != 0)
	{
		for(i=0; i < suplen; i++)
		{
			if( suprate[suplen - i - 1] & 0x80)
			{
				rate2 = suprate[suplen - i - 1];
				break;
			}
		}
	}

	//get max -> rate1
	if(rate2 > rate1 )
		rate1 = rate2;

	rate1 = rate1 & 0x7f;

	if (pAdap->wlan_attr.rate > RATE_CCK_SHORT_11M ) // ofdm rates
	{
		switch(rate1)
		{
		case 0x0c: //6
			maxbasicrate = RATE_6M;
			break;
		case 0x12: //9
			maxbasicrate = RATE_9M;
			break;
		case 0x18: //12
			maxbasicrate = RATE_12M;
			break;
		case 0x24: //18
			maxbasicrate = RATE_18M;
			break;
		case 0x30: //24
			maxbasicrate = RATE_24M;
			break;
		case 0x48: //36
			maxbasicrate = RATE_36M;
			break;
		case 0x60: //48
			maxbasicrate = RATE_48M;
			break;
		case 0x6c: //54
			maxbasicrate = RATE_54M;
			break;
		default:
			{
				switch (pAdap->wlan_attr.rate)
				{
				case (RATE_24M):
				case (RATE_36M):
				case (RATE_48M):
				case (RATE_54M):
					maxbasicrate = RATE_24M;
					break;
				case (RATE_18M):
				case (RATE_12M):
					maxbasicrate = RATE_12M;
					break;
				default:
					maxbasicrate = RATE_6M;
				}
			
			}
		}
	}
	else  //b
	{
		switch(rate1)
		{
		case 0x4: //2m
			if (pAdap->wlan_attr.preamble_type == PREAMBLE_TYPE_LONG)
				maxbasicrate = RATE_DSSS_LONG_2M;
			else
				maxbasicrate = RATE_DSSS_SHORT_2M;
			break;
		case 0xb: // 55m
			if (pAdap->wlan_attr.preamble_type == PREAMBLE_TYPE_LONG)
				maxbasicrate = RATE_CCK_LONG_55M;
			else
				maxbasicrate = RATE_CCK_SHORT_55M;
			break;
		case 0x16: //11m
			if (pAdap->wlan_attr.preamble_type == PREAMBLE_TYPE_LONG)
				maxbasicrate = RATE_CCK_LONG_11M;
			else
				maxbasicrate = RATE_CCK_SHORT_11M;
			break;
		default:
			maxbasicrate = RATE_DSSS_1M;
		}
	}

	return maxbasicrate;

}


#endif


/*Jakio: should be rewrite later, because pt_mng->rateIndex is not the same meaning as before*/
VOID Mng_GetRateIndex(PDSP_ADAPTER_T pAdap)
{
#if 0 
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T			pt_mattr = &pAdap->wlan_attr;

	if (!pt_mattr->fallback_rate_to_use)
		return;

	pt_mng->flag11g = Adap_Check11GRates(pt_mattr->support_rate,pt_mattr->support_rate_len,pt_mattr->ext_support_rate,pt_mattr->ext_support_rate_len);


	//11g & have 11g rate & auto enable
	if((pt_mattr->gdevice_info.dot11_type == IEEE802_11_G) && (pt_mng->flag11g))
	{
		pt_mng->rateIndex = MAX_11GRATE_INDEX;
		pt_mattr->rate = RATE_TYPE_54M;
	}
	else
	{
		pt_mng->flag11g = 0;
		pt_mng->rateIndex = MAX_11BRATE_INDEX;
		if(pt_mattr->preamble_type == PREAMBLE_TYPE_LONG)
			pt_mattr->rate = RATE_CCK_LONG_11M;
		else
			pt_mattr->rate = RATE_CCK_SHORT_11M;
	}
	#endif
}





VOID Mng_AuthFail(PDSP_ADAPTER_T pAdap)
{
//	pAdap->wlan_attr.gdevice_info.privacy_option = FALSE; 	
	DBG_WLAN__MLME(LEVEL_TRACE,"Mng_AuthFail \n");
	Mng_InitParas(pAdap);
	Mng_Fail(pAdap);
	
	pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;//justin: bug 3292  	081111, stop tkip counter measure

	Adap_SetLink(pAdap, LINK_FALSE);
#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status == DOING_RECONNECT)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Mng_BssDisAssoc,  --> NO RECONNECT\n");
		pAdap->reconnect_status = NO_RECONNECT;
	}
#endif
}

BOOLEAN Mng_GetOneElementFromBeacon(PDSP_ADAPTER_T pAdap,UINT8 ele,PUINT8 pbuf,PUINT8 plen)
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T				pt_mattr = &pAdap->wlan_attr;
	//PBEACON_FIXED_FIELD_T   	pfixed;
	//PMNG_HEAD80211_T		phead;
	PMNG_NOFIXED_ELE_ALL_T  pnofixed;
	//PMNG_COUNTRY_CHAN_T	pcountrychan;
	//PUINT8				    	ptmp;
	UINT16					offset;
	//UINT32					i;
	UINT8					id;
	UINT8					len = *plen;
//	UINT8					tmp;

	offset = WLAN_HDR_A3_LEN + sizeof(BEACON_FIXED_FIELD_T);
	pnofixed =WLAN_BEACON_FIRST_NOFIXED_ELE(pt_mng->recbuf);

	while(offset <
		pt_mng->rxlen - DSP_USB_RX_FRM_OFFSET_FIELD_LEN - DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN - pt_mng->body_padding_len)	// edit by Justin
	{
		id = pnofixed->element;
		if(id != ele)
		{
			offset += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
		}
		else
		{
			if(pnofixed->length <= len)
			{
				len = pnofixed->length;
				*plen = len;
			}
		
			//tmp = sizeof(MNG_NOFIXED_ELE_HEAD_T);
			//tmp = WLAN_NOFIXED_ELE_HEAD_LENGTH;


			if(pbuf != NULL)
			{
				//sc_memory_copy(pbuf,(PUINT8)(pnofixed) + tmp,len);
				sc_memory_copy(pbuf,pnofixed->data,len);
			}

			return TRUE;
		}
	}
	
	return FALSE;
}



BOOLEAN Mng_GetWPA_FROM_RSN(PDSP_ADAPTER_T pAdap,PUINT8 pbuf,PUINT32 pinfo)
{
	//PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T           pt_mattr = &pAdap->wlan_attr;

	//PBEACON_FIXED_FIELD_T   pfixed;
	//PMNG_HEAD80211_T		phead;
	
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	//PMNG_COUNTRY_CHAN_T	    pcountrychan;
	
	//PUINT8				    ptmp;
	//UINT16					offset;
	//UINT32					i;
	//UINT8   id;
	//UINT8   len;
	//UINT8	tmp;
	UINT32   border = 14;
	UINT32   tmp;
	UINT32   wepmode;

	//offset = sizeof(BEACON_FIXED_FIELD_T);
	pnofixed =WLAN_CONVERT_FRAME_NOFIXED_ELE(pbuf);
	if(pnofixed->element == WPA1_EID)
	{
		tmp = *(UINT32 *)((PUINT8)(pnofixed) + border);

		switch(tmp)
		{
			case 0x01f25000:
			case 0x05f25000:
				wepmode = WEP_MODE_WEP;
				break;
			case 0x02f25000:
				wepmode = WEP_MODE_TKIP;
				break;
			case 0x04f25000:
				wepmode = WEP_MODE_TKIP;
				break;
			default:
				wepmode = WEP_MODE_WEP;
		}

		*(UINT32 *)pinfo = wepmode;
		return TRUE;
	}
	return FALSE;
}

/*************************************************************
  *Mng_MatchWPA1_FromRsn()
  *
  *Description:
  *	Find whether the assigned wep_mode exists in the rsn information buffer,
  *	called when assingning ssid
  *Return:
  *	Ture when the given wep_mode is find in the IE buffer, or return False
  *************************************************************/
BOOLEAN Mng_MatchWPA1_FromRsn(PDSP_ADAPTER_T pAdap,PUINT8 pbuf, UINT32 wep_mode)
{
	//PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T          	pt_mattr = &pAdap->wlan_attr;
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	BOOLEAN isFind = FALSE;
	UINT16		count, offset, i;
	UINT8		lo_byte, hi_byte;
	UINT32		tmpLong, tmpMode;
	pnofixed =WLAN_CONVERT_FRAME_NOFIXED_ELE(pbuf);
	offset  = 12; //position of pairwise cipher suite count

	//safe check
	if(pnofixed->element != WPA1_EID)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"not the correct IE buffer\n");
		return FALSE;
	}
	
	lo_byte = *(pbuf + offset);
	hi_byte = *(pbuf + offset +1);
	count = lo_byte | (hi_byte << 8);//get pairwise cipher suite count
	offset += 2;  //Jakio20070530: should skip "count" byte


	/*
	offset += count * sizeof(UINT32); //position of AKM suite count

	lo_byte = *(pbuf + offset);
	hi_byte = *(pbuf + offset + 1);
	count = lo_byte | (hi_byte << 8); // get AKM suite count

	offset += 2; //position of akm suite
	*/

	//Jakio20070619: at begin, wzc maybe set some encryption mode and pass a
	//random ssid down, data in the related IEs buffer in nonsence, so parse it as 
	//the normal way may make code disordered.
	if(count > 2)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_MatchWPA1_FromRsn flow wrong with count >2  ??\n");
		return FALSE;
	}
	
	//find the match wepmode
	for(i = 0; i < count; i++)
	{
		tmpLong = *(PUINT32)(pbuf + offset + i*sizeof(UINT32));
		switch(tmpLong)
		{
			case 0x01f25000:
			case 0x05f25000:
				tmpMode = WEP_MODE_WEP;
				break;
			case 0x02f25000:
				tmpMode = WEP_MODE_TKIP;
				break;
			case 0x04f25000:
				tmpMode = WEP_MODE_AES;
				break;
			default:
				tmpMode = 0xffffffff;
		}
		if(tmpMode == wep_mode)
		{
			isFind = TRUE;
			break;
		}
	}

	return isFind;
}

BOOLEAN Mng_WpaGroupCipherMode(PDSP_ADAPTER_T pAdap, PUINT8 pbuf, PUINT8 mode)
{
	//PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T          	pt_mattr = &pAdap->wlan_attr;
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	UINT16				offset;
	UINT32				tmpLong;
	
	offset  = 8; //position of pairwise cipher suite count
	pnofixed = (PMNG_NOFIXED_ELE_ALL_T)pbuf;

	if(pnofixed->element != WPA1_EID)
		return FALSE;
	

	//find the match wepmode

	tmpLong = *(PUINT32)(pbuf + offset);
	switch(tmpLong)
	{
		case 0x01f25000:
		case 0x05f25000:
			*mode = WEP_MODE_WEP;
			break;
		case 0x02f25000:
			*mode = WEP_MODE_TKIP;
			break;
		case 0x04f25000:
			*mode = WEP_MODE_AES;
			break;
		default:
			*mode = 0xff;
	}

	if(*mode != 0xff)
		return TRUE;
	else
		return FALSE;
}






#ifdef DSP_WPA2
/*************************************************************
  *Mng_MatchWPA2_FromRsn()
  *
  *Description:
  *	Find whether the assigned wep_mode exists in the rsn information buffer,
  *	called when assingning ssid
  *Return:
  *	Ture when the given wep_mode is find in the IE buffer, or return False
  *************************************************************/
BOOLEAN Mng_MatchWPA2_FromRsn(PDSP_ADAPTER_T pAdap,PUINT8 pbuf, UINT32 wep_mode)
{
	//PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T          	pt_mattr = &pAdap->wlan_attr;
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	BOOLEAN isFind = FALSE;
	UINT16		count, offset, i;
	UINT8		lo_byte, hi_byte;
	UINT32		tmpLong, tmpMode;
	pnofixed =WLAN_CONVERT_FRAME_NOFIXED_ELE(pbuf);
	offset  = 8; //position of pairwise cipher suite count

	//safe check
	if(pnofixed->element != WPA2_EID)
	{
		DBG_WLAN__MLME( LEVEL_TRACE, "not the correct IE buffer\n");
		return FALSE;
	}
	
	lo_byte = *(pbuf + offset);
	hi_byte = *(pbuf + offset +1);
	count = lo_byte | (hi_byte << 8);//get pairwise cipher suite count
	offset += 2; //
	/*
	lo_byte = *(pbuf + offset);
	hi_byte = *(pbuf + offset + 1);
	count = lo_byte | (hi_byte << 8); // get AKM suite count

	offset += 2; //position of akm suite
	*/

	if(count > 2)
		return FALSE;
	
	//find the match wepmode
	for(i = 0; i < count; i++)
	{
		tmpLong = *(PUINT32)(pbuf + offset + i*sizeof(UINT32));
		switch(tmpLong)
		{
			case 0x01ac0f00:
			case 0x05ac0f00:
				tmpMode = WEP_MODE_WEP;
				break;
			case 0x02ac0f00:
				tmpMode = WEP_MODE_TKIP;
				break;
			case 0x04ac0f00:
				tmpMode = WEP_MODE_AES;
				break;
			default:
				tmpMode = 0xffffffff;
		}
		if(tmpMode == wep_mode)
		{
			isFind = TRUE;
			break;
		}
	}

	return isFind;
}
BOOLEAN Mng_Wpa2GroupCipherMode(PDSP_ADAPTER_T pAdap, PUINT8 pbuf, PUINT8 mode)
{
	//PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	//PDSP_ATTR_T          	pt_mattr = &pAdap->wlan_attr;
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	UINT16				offset;
	UINT32				tmpLong;
	
	offset  = 4; //position of pairwise cipher suite count
	pnofixed = (PMNG_NOFIXED_ELE_ALL_T)pbuf;

	if(pnofixed->element != WPA2_EID)
		return FALSE;
	

	//find the match wepmode

	tmpLong = *(PUINT32)(pbuf + offset);
	switch(tmpLong)
	{
		case 0x01ac0f00:
		case 0x05ac0f00:
			*mode = WEP_MODE_WEP;
			break;
		case 0x02ac0f00:
			*mode = WEP_MODE_TKIP;
			break;
		case 0x04ac0f00:
			*mode = WEP_MODE_AES;
			break;
		default:
			*mode = 0xff;
	}

	if(*mode != 0xff)
		return TRUE;
	else
		return FALSE;
}

#endif /*DSP_WPA2*/


VOID Mng_BeaconChange(PMNG_DES_T curdes,PMNG_DES_T newdes,PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T    pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T     pt_mattr = &pAdap->wlan_attr;
	//UINT32 reg_value;
#if 1
	if((0 != sc_memory_cmp(curdes->bssid,newdes->bssid,WLAN_ETHADDR_LEN)) ||
		(pt_mattr->macmode != WLAN_MACMODE_ESS_STA))
	{
		return;
	}
#endif	
	//check  erp bit 
	if(pt_mng->usedbssdes.Erpflag)
	{
		//if(Rts_ERP_state_change(pAdap,pt_mng->usedbssdes.Erpinfo,pAdap->wlan_attr.erp_element))
		if((newdes->Erpinfo!=curdes->Erpinfo) &&
			(pAdap->wlan_attr.cts_to_self_config == CTS_SELF_ENABLE))
		{
			DBG_WLAN__MLME(LEVEL_INFO,"AP ERP protect changed, erp = %x \n",newdes->Erpinfo);

			pt_mng->usedbssdes.Erpinfo = newdes->Erpinfo;
			pAdap->wlan_attr.erp_element = newdes->Erpinfo;
			//update cts to self state
			Mng_Update_Cts_State(pAdap,pt_mng->usedbssdes.Erpinfo);
		}
	}	
	//check other item
}
  

VOID Mng_ClearPara(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	pt_mng->oidscanflag = FALSE;
//		DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = FALSE;\n");
		
}


VOID Mng_UpdataIEs(PUINT8 iesBuf,PUINT32 pIesLength)
{
	UINT8    tmpbuf[500];
	PUINT8   ptmp;
	PUINT8   pIestmp;
	PMNG_NOFIXED_ELE_ALL_T     pnofixed;
	UINT32    len;
	UINT32    curlen;

	PUINT8   pdata;


	// DBG_WLAN__MLME(("$$$$$ BEGIN Mng_UpdataIEs\n")); 		

	if(*pIesLength >500)
	{
	//	DBG_WLAN__MLME(("$$$$$ Mng_UpdataIEs return due to too length\n")); 		
		return;

	}

	//copy fix field
	sc_memory_copy(tmpbuf, iesBuf, 12);
	curlen = 12;
	len = 12;
	
	ptmp = tmpbuf + curlen;
	pIestmp = iesBuf + len;

	//pnofixed = (PMNG_NOFIXED_ELE_HEAD_T)pIestmp;
	pnofixed = WLAN_CONVERT_FRAME_NOFIXED_ELE(pIestmp);

	while(len < *pIesLength)
	{
		if((pnofixed->element == RESERVE_1_EID) || (pnofixed->element == 133 ) || (pnofixed->element == 150))  //cancel the element
		{
			len += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
		}
		else if(pnofixed->element == 0xdd)  //cancel the element
		{

			pdata = pnofixed->data;
			if((pdata[0] == 0x00) && (pdata[1] == 0x50) && (pdata[2] == 0xf2))
			{
				//copy element
			sc_memory_copy(tmpbuf + curlen,pnofixed,WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed));
			curlen += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);

			//move point to next point
			len += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
		
			}
			else //cancel
			{

			len += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
			}
		}
		else   //copy the element
		{
			//copy element
		//	DBG_WLAN__MLME(("$$$$$ Mng_UpdataIEs no filter and copy the data,id = %x\n",pnofixed->element)); 		

			sc_memory_copy(tmpbuf + curlen,pnofixed,WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed));
			curlen += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			
			//move point to next point
			len += WLAN_FRAME_NOFIXED_ELE_LENGTH(pnofixed);
			WLAN_MOVE_TO_NEXT_ELE(pnofixed);
		
		}
	}
   
	*pIesLength = curlen;
	sc_memory_copy(iesBuf,tmpbuf,curlen);

}

/*******************************************************************
 *   Mng_SetPowerSave
 *   
 *   Descriptions:
 *      This routine set the power save state to power save. 
 *   Arguments:
 *      pAdap: , the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Mng_SetPowerSave(PDSP_ADAPTER_T pAdap)
{//Justin
	UINT32 ulTxFifoCounter = 0;
	//pAdap->wlan_attr.ps_state = pAdap->wlan_attr.ps_state_config;
	if ((pAdap->wlan_attr.gdevice_info.ps_support == PSS_ACTIVE)			// not support ps
		||(pAdap->wlan_attr.gdevice_info.ps_mode != PSS_ACTIVE))			// not in active mode. need not to change ps mode
	{
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		return;
	}
	//justin: 	080317.	do not set to save if there is packets in tx or rx fifo in dsp(hw)
	ulTxFifoCounter = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);
	ulTxFifoCounter &=0x000000ff;

	
	if(ulTxFifoCounter != 0)
	{
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		return;

	}

#if 0
	Vcmd_set_power_mng(pAdap);
	Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode

	while(!MngQueue_IsEmpty((PDSP_MNG_QUEUE_T)pAdap->pmng_queue_list))//wait null frame send to dsp
	{
		sc_sleep(100);
		NULL;
	}
#endif

	while((VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT) & 0x000000ff) != 0)//wait null frame send from dsp to air
	{
		sc_sleep(100);
		//NULL;
	}


//	DBGSTR_POWER_MNG(("Power save support mode = %d, curr mode = %d",pAdap->wlan_attr.gdevice_info.ps_support,pAdap->wlan_attr.gdevice_info.ps_mode));
	/*
	* During this req,suppose some frame is pending,then just return it
	*/
//	if ((pAdap->gdevice_info.frame_pend == TRUE) || (pAdap->gdevice_info.idle == TRUE))
//	{
//		/*
//		* here i can raise retry limit reached only.
//		*/
////		send_msg_to_mlme(pAdapter, RETRY_LIMIT_REACHED, (uint8)POWER_SAVE_MODE_MSG);
//		return;
//	}
	

	Adap_Set_Power_Management_Mode(pAdap, TRUE,TRUE);
	pAdap->wlan_attr.gdevice_info.ps_mode = pAdap->wlan_attr.gdevice_info.ps_support;
}

/*******************************************************************
 *   Mng_SetPowerActive
 *   
 *   Descriptions:
 *      This routine set the power save state to active  
 *   Arguments:
 *      pAdap: , the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Mng_SetPowerActive(PDSP_ADAPTER_T pAdap, BOOLEAN ps_poll_flag)
{//Justin
	//pAdap->wlan_attr.ps_state = pAdap->wlan_attr.ps_state_config;
	if (pAdap->wlan_attr.gdevice_info.ps_support == PSS_ACTIVE)	// not support ps
	{
		return;
	}
/*	
	if ( pAdap->wlan_attr.gdevice_info.ps_mode == PSS_ACTIVE)	// in active mode. need not to change ps mode
	{
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		return;
	}
//*/

/*
	if ( pAdap->wlan_attr.gdevice_info.ps_mode == PSS_ACTIVE)
		Adap_Set_Power_Management_Mode(pAdap, FALSE,TRUE);
	else
		Adap_Set_Power_Management_Mode(pAdap, FALSE,FALSE);
	//*/
	Adap_Set_Power_Management_Mode(pAdap, FALSE,ps_poll_flag);

	Tx_Send_Next_Frm(pAdap);
//wumin	wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);
}

/*
	the routine updates cts to self state when a beacon frame is received.
	if cts to self should be supported, set cts_en flag & set rts_threshold to max value
	if cts to self should be not supported, clear cts_en flag & set rts_threshold to max value
*/
VOID Mng_Update_Cts_State(PDSP_ADAPTER_T pAdap,UINT8 erp)
{
	UINT8	cts_self;
	UINT16  rts_threshold;
	//UINT32	reg_value;
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	cts_self = (erp & ERP_HAVE_NONERP_STA) ? CTS_SELF_ENABLE: CTS_SELF_DISABLE;

	//cts self should be supported
	pAdap->wlan_attr.cts_en = cts_self;

	do{	
		if(CTS_SELF_ENABLE == pAdap->wlan_attr.cts_en)
		{
			pAdap->wlan_attr.gdevice_info.preamble_type = LONG_PRE_AMBLE;
			// 11g + fixed 11g rate should use cts to self
			// to be simple, we use cts to self for whole 11g case 
			if(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G)
			{
				rts_threshold =2;			
				pAdap->wlan_attr.gdevice_info.preamble_type = LONG_PRE_AMBLE;
				pAdap->wlan_attr.cts_current_state = RTSCTS_STATE_CTSSELF_WORK;
				break;
			}
		}	

		//make rts/cts enable by setting rts threshold to current value
		rts_threshold = pAdap->wlan_attr.rts_threshold;			
		pAdap->wlan_attr.cts_current_state = RTSCTS_STATE_RTS_WORK;
	}while(FALSE);

	//write rts reg
	Vcmd_set_rts_retrylimit(
				pAdap,
				rts_threshold,
				pAdap->wlan_attr.retry_limit.short_retry_limit,
				pAdap->wlan_attr.retry_limit.long_retry_limit);		

	//update slot
	if(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G)
	{
		pAdap->wlan_attr.gdevice_info.slot_time  = SLOT_TIME_SHORT_DOT11G;
					//((pt_mng->usedbssdes.Erpinfo & 0x01) != 0) ? SLOT_TIME_LONG_DOT11G : SLOT_TIME_SHORT_DOT11G;
		Adap_set_basicrate_map_ofdm11g(pAdap);
		Adap_update_slot_time(pAdap,pAdap->wlan_attr.gdevice_info.slot_time);
	}

	//set backoff window
	if(pAdap->wlan_attr.gdevice_info.slot_time == SLOT_TIME_LONG_DOT11G)
	{
		VcmdW_3DSP_Dword(pAdap, EDCAPARAMETER0_FOR_11B, WLS_MAC__EDCAPARAM0_WD);
	}
	else
	{
		VcmdW_3DSP_Dword(pAdap, EDCAPARAMETER0_FOR_11G, WLS_MAC__EDCAPARAM0_WD);
	}

	if((pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G)
		&&((pt_mng->usedbssdes.Erpinfo & 0x01) != 0) )	//mix b/g
	{
		switch(pAdap->wlan_attr.auth_mode)
		{
			case AUTH_MODE_WPA:
			case AUTH_MODE_WPA2:
				pAdap->wlan_attr.gdevice_info.slot_time  = SLOT_TIME_LONG_DOT11G;
				Adap_set_basicrate_map_ofdm11g(pAdap);
				Adap_update_slot_time(pAdap,pAdap->wlan_attr.gdevice_info.slot_time);
				VcmdW_3DSP_Dword(pAdap, EDCAPARAMETER0_FOR_11B, WLS_MAC__EDCAPARAM0_WD);
				break;
				
			case AUTH_MODE_WPA_PSK:
			case AUTH_MODE_WPA2_PSK:
				pAdap->wlan_attr.gdevice_info.slot_time  = SLOT_TIME_SHORT_DOT11G;
				Adap_set_basicrate_map_ofdm11g(pAdap);
				Adap_update_slot_time(pAdap,pAdap->wlan_attr.gdevice_info.slot_time);
				VcmdW_3DSP_Dword(pAdap, EDCAPARAMETER0_FOR_11G, WLS_MAC__EDCAPARAM0_WD);
				break;
				
			default://open or share
				pAdap->wlan_attr.gdevice_info.slot_time  = SLOT_TIME_SHORT_DOT11G;
				Adap_set_basicrate_map_ofdm11g(pAdap);
				Adap_update_slot_time(pAdap,pAdap->wlan_attr.gdevice_info.slot_time);
				VcmdW_3DSP_Dword(pAdap, EDCAPARAMETER0_FOR_11B, WLS_MAC__EDCAPARAM0_WD);
				break;				
		}
	}


}

//wumin
//VOID Mng_Calc_Capbility(PDSP_ADAPTER_T pAdap)
//{
//	PDSP_ATTR_T pAttr = (PDSP_ATTR_T)&pAdap->wlan_attr;
//
//	sc_memory_set(&pAttr->capability,sizeof(DRIVER_BLOCK_T));
//
//	pAttr->capability.ess = (pAttr->macmode == WLAN_MACMODE_ESS_STA) ? 1:0;
//	pAttr->capability.ibss = (pAttr->macmode == WLAN_MACMODE_ESS_STA) ? 0:1; 
//	pAttr->capability.privacy = (pAttr->gdevice_info.privacy_option == TRUE) ? 1:0;
//}

/*
   STA is responsible to send probe respond packet if a probe request frame received while 
   STA is in IBSS joinning or joinok state.
*/
TDSP_STATUS Mng_Transact_Probersp(PDSP_ADAPTER_T pAdap)
{
	PMNG_NOFIXED_ELE_ALL_T  pdata;
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T			pt_mattr = &pAdap->wlan_attr;
	PMNG_HEAD80211_T	phead =(PMNG_HEAD80211_T)pt_mng->recbuf ;
	//PUINT8				pbssaddr;
	//UINT8	            		length;

	pdata = (PMNG_NOFIXED_ELE_ALL_T)WLAN_HDR_A3_DATAP(pt_mng->recbuf);

	//response for both broadcast ssid or match ssid
	if(((pdata->element == SSID_EID) && (pdata->length == 0)) ||
		((pdata->length == pt_mattr->ssid_len) && 
		(0 == sc_memory_cmp(pdata->data,pt_mattr->ssid,pdata->length)))
		)
	{
		return Mng_MakeBeaconOrProbersp(
			pAdap,
			WLAN_FSTYPE_PROBERESP,
			phead->adr.ibss.sa);
	}
	return STATUS_FAILURE;
}

BOOLEAN Mng_IBSS_valid_probe(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	
	return ((pt_mng->statuscontrol.curstatus == JOINNING) || (pt_mng->statuscontrol.curstatus == JOINOK));

}

/*
typedef struct _RSSI_UPDATE_
{
	UINT8	prev_count;	//holds the position saved last rssi value
//	UINT8	curr_count; //holds the position saved last rssi value
	
	UINT8	total_count;	//holds the number of saved rssi value(valid)
	long		rssi_acc;	//holds the sum of  saved rssi value(valid)

	long		rssi_array[RSSI_ARRAY_SIZE];	//holds times of rssi value
}RSSI_UPDATE_T, PRSSI_UPDATE_T;
*/

//Justin:	071210.		fix bug 2875.  
// update rssi value while receive packet from connected AP.	
// This function is used only after connected an AP.
VOID Mng_UpdateRssi(PDSP_ADAPTER_T pAdap, INT32 rssi)
{
	PRSSI_UPDATE_T		pt_rssi_t = &(pAdap->rssi_update);
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	//UINT8	curr_count;
	INT32		avg_rssi;
	UINT32	used_bssid_in_list_id;
	UINT32	i;

	if(pAdap->link_ok != LINK_OK)
		return;

#if 0
	if(pt_rssi_t->total_count != 0)	// has saved some value in array
	{
		if(pt_rssi_t->prev_count >= RSSI_ARRAY_SIZE-1)	// array has full
		{
			curr_count = 0;
			pt_rssi_t->total_count = RSSI_ARRAY_SIZE;
		}
		else
		{
			curr_count = pt_rssi_t->prev_count++;
			pt_rssi_t->total_count ++;
			if(pt_rssi_t->total_count > RSSI_ARRAY_SIZE)
				pt_rssi_t->total_count = RSSI_ARRAY_SIZE;
		}
	}
	else	//has not saved any value
	{
		curr_count = 0;
		pt_rssi_t->total_count = 1;
	}
	
	if(pt_rssi_t->total_count >= RSSI_ARRAY_SIZE)
		pt_rssi_t->rssi_acc -= pt_rssi_t->rssi_array[curr_count];		// sub the oldest value

	pt_rssi_t->rssi_array[curr_count] = rssi;					// save the current value
	pt_rssi_t->rssi_acc += rssi;										// add current value to sum

	avg_rssi = pt_rssi_t->rssi_acc / pt_rssi_t->total_count;				// caculate average value
	pAdap->rssi_update.prev_count = curr_count;
#endif

	pt_rssi_t->rssi_acc = 0;
	for(i=(RSSI_ARRAY_SIZE-1); i>0; i--)
	{
		pt_rssi_t->rssi_array[i] = pt_rssi_t->rssi_array[i-1];
		pt_rssi_t->rssi_acc +=  GetRssi(pt_rssi_t->rssi_array[i]);
	}
	pt_rssi_t->rssi_array[0] = rssi;
	pt_rssi_t->rssi_acc += GetRssi(rssi);

	if(pt_rssi_t->total_count >= RSSI_ARRAY_SIZE)
	{
		pt_rssi_t->total_count = RSSI_ARRAY_SIZE;
	}
	else
	{
		pt_rssi_t->total_count ++;
	}

	avg_rssi = pt_rssi_t->rssi_acc / pt_rssi_t->total_count;
	avg_rssi = GetRssi(avg_rssi) + GetRssiMsb(rssi);

	//save to scan result list and to used bss description
	used_bssid_in_list_id = BssTableSearch(&pt_mng->bssdeslist, pt_mng->usedbssdes.bssid, pt_mng->usedbssdes.channel);//find bssid in the scan list 

	if(used_bssid_in_list_id != (UINT32)BSS_NOT_FOUND)
		pt_mng->bssdeslist.bssdesset[used_bssid_in_list_id].rssi = avg_rssi;
	
	if(pAdap->link_ok == LINK_OK)
	{
		pt_mng->usedbssdes.rssi = avg_rssi;

		sc_spin_lock(&pAdap->lock);
		pAdap->wlan_attr.rssi = avg_rssi;
		sc_spin_unlock(&pAdap->lock);
	}
}




/*! \brief initialize BSS table
 *  \param p_tab pointer to the table
 *  \return none
 *  \pre
 *  \post
 */
VOID BssTableInit(
     MNG_DESLIST_T *Tab) 
{
    UINT8 i;

    Tab->bssdesnum= 0;	
    for (i = 0; i < WLAN_BSSDES_NUM; i++) 
    {
        sc_memory_set(&Tab->bssdesset[i], 0, sizeof(MNG_DES_T));
    }
}

/*! \brief search the BSS table by SSID
 *  \param p_tab pointer to the bss table
 *  \param ssid SSID string 
 *  \return index of the table, BSS_NOT_FOUND if not in the table
 *  \pre
 *  \post
 *  \note search by sequential search 
 */
UINT32 BssTableSearch(
     MNG_DESLIST_T *Tab, 
     PUINT8    pBssid,
     UINT8     Channel) 
{
    UINT32 i;

    i = (Tab->bssdesnum > WLAN_BSSDES_NUM) ? WLAN_BSSDES_NUM : Tab->bssdesnum;
    Tab->bssdesnum = i;
		
    for (i = 0; i < Tab->bssdesnum; i++) 
    {
    	//
    	// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
    	// We should distinguish this case.
    	//    	
        if ((((Tab->bssdesset[i].channel<= 14) && (Channel <= 14)) ||
			 ((Tab->bssdesset[i].channel > 14) && (Channel > 14))) &&
			IS_MATCH_IN_ADDRESS(Tab->bssdesset[i].bssid, pBssid)) 
        { 
            return i;
        }
    }
    return (UINT32)BSS_NOT_FOUND;
}

UINT32 BssSsidTableSearch(
	 MNG_DESLIST_T *Tab, 
	 PUINT8    pBssid,
	 PUINT8    pSsid,
	 UINT8     SsidLen,
	 UINT8     Channel) 
{
	UINT8 i;

    for (i = 0; i < Tab->bssdesnum; i++) 
	{
		//
		// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
		// We should distinguish this case.
		//    	
        if ((((Tab->bssdesset[i].channel<= 14) && (Channel <= 14)) ||
			 ((Tab->bssdesset[i].channel > 14) && (Channel > 14))) &&
			IS_MATCH_IN_ADDRESS(Tab->bssdesset[i].bssid, pBssid)&&
			IS_MATCH_IN_SSID(SSID_EID, SsidLen,pSsid, SSID_EID,Tab->bssdesset[i].ssid[0],&Tab->bssdesset[i].ssid[1])) 
		{ 
			return i;
		}
	}
	return (UINT32)BSS_NOT_FOUND;
}

VOID BssTableDeleteEntry(
	 	MNG_DESLIST_T *Tab, 
			PUINT8    pBssid)
{
	UINT8 i, j;

	for (i = 0; i < Tab->bssdesnum; i++) 
	{
		//printf("comparing %s and %s\n", p_tab->bss[i].ssid, ssid);
		if (IS_MATCH_IN_ADDRESS(Tab->bssdesset[i].bssid, pBssid))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"delete bssid = %x %x %x %x %x %x\n",
				pBssid[0],pBssid[1],pBssid[2],pBssid[3],pBssid[4],pBssid[5]);

			for (j = i; j < Tab->bssdesnum - 1; j++)
			{
				sc_memory_copy(&(Tab->bssdesset[j]), &(Tab->bssdesset[j + 1]), sizeof(MNG_DES_T));
			}
			Tab->bssdesnum -= 1;
			return;
		}
	}
}

INT32 rssi_reg_value_to_dbm(INT32 rssi)
{
    INT32  rssi_db;
	// Convert to a Voltage +2 for round
	rssi_db = (GetRssi(rssi) + 2) * 12 / 10;

	// Convert to dB, y = 32x - 112
	rssi_db = rssi_db - 112;

    return rssi_db;
}



// sort bss des by rssi,   so we get a best signal ap in the first position of the list.
VOID BssTableSortByRssi(
      MNG_DESLIST_T *OutTab) 
{
    UINT32       i, j;
    MNG_DES_T TmpBss;

DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, BssTableSortByRssi bss num = %d\n",OutTab->bssdesnum);

	if(OutTab->bssdesnum <= 0 || OutTab->bssdesnum > WLAN_BSSDES_NUM)
	{
		return;
	}

    for (i = 0; i < OutTab->bssdesnum - 1; i++) 
    {
        for (j = i+1; j < OutTab->bssdesnum; j++) 
        {
            if (OutTab->bssdesset[j].rssi > OutTab->bssdesset[i].rssi) 
            {
                sc_memory_copy(&TmpBss, &OutTab->bssdesset[j], sizeof(MNG_DES_T));
                sc_memory_copy(&OutTab->bssdesset[j], &OutTab->bssdesset[i], sizeof(MNG_DES_T));
                sc_memory_copy(&OutTab->bssdesset[i], &TmpBss, sizeof(MNG_DES_T));
            }
        }
    }
}
#ifdef ROAMING_SUPPORT

BOOLEAN Mng_CheckAKMAndCipher(
     NDIS_802_11_AUTHENTICATION_MODE auth, 
     MNG_DES_T *pBss,
    UINT8 pwCipher,//pairwise cipher
    UINT8 gpCipher)//group cipher
{
    UINT16 uCount = 0;
	
    PUINT8 pwpaIe = pBss->IEs + pBss->offset_wpa;
    PUINT8 pwpa2Ie = pBss->IEs + pBss->offset_wpa2;
    PUINT8 pRSNStartAddr;	
    PUINT8 pPWkey_count;
    PUINT8 pAKM_count;
    PAKM_SUITE_STRUCT pAKM;
    PCIPHER_SUITE_STRUCT pGroupCipherSuite,pPairwiseCipherSuite;
	UINT8 new_pwCipher,new_gpCipher;

	BOOLEAN bMatchAKM = TRUE;		//TRUE: match 		FALSE: not match
	BOOLEAN bMatchCipher = FALSE;//TRUE: match 		FALSE: not match

	//Justin: 071214		for wpa, must skip 4 BYTEs after length.	that is to say, skip the bytes between length and version
    //pRSNStartAddr = (pBss->offset_wpa != 0xFFFF) ? (pwpaIe) : ((pBss->offset_wpa2 != 0xFFFF)?pwpa2Ie:0);
	//justin:	080910.	for fix bug 3250.	connected WPA ap, then change ap to WPA2, BSD occured. 
	//				must check offset_wpa != 0xffff first
	pRSNStartAddr = (auth<Ndis802_11AuthModeWPA2) ? ((pBss->offset_wpa != 0xFFFF)?pwpaIe:0) : ((pBss->offset_wpa2 != 0xFFFF)?pwpa2Ie:0);
	
	if(pRSNStartAddr == 0)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 AKM check fail. no wpa ie found\n");
		return FALSE;
	}
	
    if((pBss->offset_wpa != 0xFFFF) &&(auth<Ndis802_11AuthModeWPA2))	//wpa mode , there is an OUI after LENGTH. 	do skip this OUI
		pGroupCipherSuite = (PCIPHER_SUITE_STRUCT)(pRSNStartAddr+sizeof(RSN_IE_HEADER_STRUCT) + 4);
    else
	    pGroupCipherSuite = (PCIPHER_SUITE_STRUCT)(pRSNStartAddr+sizeof(RSN_IE_HEADER_STRUCT));

	
    pPWkey_count = ((PUINT8)pGroupCipherSuite)+sizeof(CIPHER_SUITE_STRUCT);

   pAKM_count = (PUINT8)(pPWkey_count 		   + sizeof(UINT16)         +(*((PUINT16)pPWkey_count) *sizeof(CIPHER_SUITE_STRUCT)));
    pAKM	= (PAKM_SUITE_STRUCT)(pPWkey_count 		   + sizeof(UINT16)         +(*((PUINT16)pPWkey_count) *sizeof(CIPHER_SUITE_STRUCT)) + sizeof(UINT16));
    //						   pairwisekey count start  + skip pw count            + skip pwkeys                                                              +  Skip akm count	//

    pPairwiseCipherSuite = (PCIPHER_SUITE_STRUCT)(pPWkey_count 		   + sizeof(UINT16));

	//get new group cipher
    switch(pGroupCipherSuite->Type)
    {
	case 1:		
	case 5:
		new_gpCipher = WEP_MODE_WEP;
		break;
	case 2:		
		new_gpCipher = WEP_MODE_TKIP;
		break;
	case 4:		
		new_gpCipher = WEP_MODE_AES;
		break;
	default:		
		new_gpCipher = 9;
		break;
	}
	if(new_gpCipher != gpCipher)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 group cipher check fail\n");
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT,  n_gp = %x, gp = %x\n",new_gpCipher,gpCipher);
		return FALSE;
	}

	///////////////check cipher
	bMatchCipher = FALSE;
	uCount = 0;
	while(uCount < (UINT16)(*(PUINT16)pPWkey_count))
	{
		pPairwiseCipherSuite = (PCIPHER_SUITE_STRUCT)((PUINT8)pPairwiseCipherSuite +sizeof(CIPHER_SUITE_STRUCT)*uCount);//skip ciper 1
		// get new pairwise cipher
	    switch(pPairwiseCipherSuite->Type)
	    {
	    	case 0:
			new_pwCipher = new_gpCipher;
			break;
		case 1:		
		case 5:
			new_pwCipher = WEP_MODE_WEP;
			break;
		case 2:		
			new_pwCipher = WEP_MODE_TKIP;
			break;
		case 4:		
			new_pwCipher = WEP_MODE_AES;
			break;
		default:		
			new_pwCipher = 9;
			break;
		}

		if(new_pwCipher == pwCipher)
		{
			bMatchCipher = TRUE;	
			//DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, n_pw = %x,  pw = %x \n",new_pwCipher,pwCipher);
			break;
		}
		
		uCount ++;
	}

	if(!bMatchCipher)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 cipher check fail,count = %x, 0x%x\n",*pPWkey_count,*(PUINT32)pPairwiseCipherSuite);
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT,  pw = %x \n",pwCipher);
		return FALSE;
	}

////////////check akm
	bMatchAKM= FALSE;
	uCount = 0;
	while(uCount < (UINT16)(*(PUINT16)pAKM_count))
	{
		bMatchAKM= TRUE;
		pAKM = (PAKM_SUITE_STRUCT)((PUINT8)pAKM +sizeof(AKM_SUITE_STRUCT)*uCount);//skip akm
		// get new akm
	   switch (auth)
	    {
	    case Ndis802_11AuthModeWPA:
	    	if((pBss->offset_wpa != 0xFFFF) && ((pAKM->Type) == 1))
		{
			// return TRUE;
		}
		else 
		{
			bMatchAKM = FALSE;
		}

		break;
		
	    case Ndis802_11AuthModeWPAPSK:
	    	if((pBss->offset_wpa != 0xFFFF) && ((pAKM->Type) == 2))
		{
			// return TRUE;
		}
		else 
		{
			bMatchAKM = FALSE;
		}
		break;
	    case Ndis802_11AuthModeWPA2:
	    	if((pBss->offset_wpa2 != 0xFFFF) && ((pAKM->Type) == 1))
		{
			// return TRUE;
		}
		else 
		{
			bMatchAKM = FALSE;
		}
		break;
	    case Ndis802_11AuthModeWPA2PSK:
	    	if((pBss->offset_wpa2 != 0xFFFF) && ((pAKM->Type) == 2))
		{
			// return TRUE;
		}
		else 
		{
			bMatchAKM = FALSE;
		}
		break;
	    default:
			
		    bMatchAKM = FALSE;
		break;
	    }

		if(bMatchAKM == TRUE)
			break;
		
		uCount ++;
	}

	   if(!bMatchAKM)
	   {
		DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 AKM check fail\n");
		return FALSE;
	   }
	   
   return TRUE;
}


// step 2. find all matching BSS in the lastest SCAN result (inBssTab) 
//    & log them into MlmeAux.SsidBssTab for later-on iteration. Sort by RSSI order
VOID BssTableSsidSort(
    	PDSP_ADAPTER_T pAdap, 
     MNG_DESLIST_T *OutTab, 
      INT8 Ssid[], 
      UINT8 SsidLen) 
{
    UINT8 i;
    NDIS_802_11_AUTHENTICATION_MODE AuthMode;
    PMNG_STRU_T     pt_mng = pAdap->pmanagment;
    PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;
    MNG_DES_T *pOutBss;
    MNG_DES_T *pInBss;


	if (pAdap->wlan_attr.auth_mode == AUTH_MODE_OPEN)
	{
		AuthMode = Ndis802_11AuthModeOpen;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_SHARED_KEY)
	{
		AuthMode = Ndis802_11AuthModeShared;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA)
	{
		AuthMode = Ndis802_11AuthModeWPA;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK)
	{
		AuthMode = Ndis802_11AuthModeWPAPSK;
	}
	
	#ifdef DSP_WPA2
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2)
	{
		AuthMode = Ndis802_11AuthModeWPA2;
	}
	else if (pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK)
	{
		AuthMode = Ndis802_11AuthModeWPA2PSK;
	}
	#endif
	else
	{
		AuthMode = Ndis802_11AuthModeAutoSwitch;
	}

DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, BssTableSsidSort bss num = %d\n",pt_mng->bssdeslist.bssdesnum);
    for (i = 0; i < pt_mng->bssdeslist.bssdesnum; i++) 
    {
	if(i >= (WLAN_BSSDES_NUM-1))
		break;
		
        pInBss = &pt_mng->bssdeslist.bssdesset[i];
        if (IS_MATCH_IN_SSID(SSID_EID, SsidLen,Ssid, SSID_EID,pInBss->ssid[0],&pInBss->ssid[1]))// get one ap contain specific ssid....
        {

			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, match ssid found\n");
			
            		pOutBss = &OutTab->bssdesset[OutTab->bssdesnum];

			// New for WPA2
			// Check the Authmode first
			if (AuthMode >= Ndis802_11AuthModeWPA)
			{
				if((pInBss->offset_wpa == 0xffff)
					&&(pInBss->offset_wpa2 == 0xffff))
				{
		              	continue;// None matched
				}
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 offset ok\n");

				if(!Mng_CheckAKMAndCipher(AuthMode, pInBss,pAdap->wlan_attr.wep_mode,pAdap->wlan_attr.group_cipher))
				{
		              	continue;// None matched
				}
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wpa/wpa2 akm and cipher ok\n");
					
 /* 				
		                if (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPANone)
		                {
		                    //if (pAdap->wlan_attr.auth_mode != pInBss->)
		                        continue;// None matched 
		                }
		                else
		                {
		                    // Check AuthMode and AuthBitMode for matching, in case AP support dual-mode
					//if(!RTMPCheckAKM(pAdap->wlan_attr.auth_mode,pInBss))
					if(pInBss->offset_wpa == 0xffff)
		              			continue;// None matched
		                }
				
				// Check cipher suite, AP must have more secured cipher than station setting
				if ((pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA) 
					|| (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPAPSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipherAux))
						continue;						
				}
				else if ((pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA2) 
					|| (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA2PSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA2.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA2.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA2.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipherAux))
						continue;						
				}
//*/				
			}			
			// Bss Type matched, SSID matched. 
			// We will check wepstatus for qualification Bss
			else if(  CAP_EXPRESS_PRIVACY(pInBss->cap) != pt_mattr->gdevice_info.privacy_option )
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, wep check failed\n");
               // DBGPRINT(RT_DEBUG_TRACE,"PortCfg.WepStatus=%d, while pInBss->WepStatus=%d\n", pAd->PortCfg.WepStatus, pInBss->WepStatus);
				continue;
			}

			// Since the AP is using hidden SSID, and we are trying to connect to ANY
			// It definitely will fail. So, skip it.
			// CCX also require not even try to connect it!!
			if (SsidLen == 0)
				continue;
			
            // copy matching BSS from InTab to OutTab
            sc_memory_copy(pOutBss, pInBss, sizeof(MNG_DES_T));
            
            OutTab->bssdesnum++;
        }
/*        else if ((pInBss->BssType == pAd->PortCfg.BssType) && (SsidLen == 0))
        {
            BSS_ENTRY *pOutBss = &OutTab->bssdesset[OutTab->BssNr];

			// New for WPA2
			// Check the Authmode first
			if (pAdap->wlan_attr.auth_mode >= Ndis802_11AuthModeWPA)
			{
				// Check AuthMode and AuthBitMode for matching, in case AP support dual-mode
				if(!RTMPCheckAKM(pAdap->wlan_attr.auth_mode,pInBss))
					continue;   // None matched
				
				// Check cipher suite, AP must have more secured cipher than station setting
				if ((pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA) || (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPAPSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipherAux))
						continue;						
				}
				else if ((pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA2) || (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA2PSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA2.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA2.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA2.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipherAux))
						continue;						
				}
			}
			// Bss Type matched, SSID matched. 
			// We will check wepstatus for qualification Bss
			else if (pAd->PortCfg.WepStatus != pInBss->WepStatus)
					continue;
			
            // copy matching BSS from InTab to OutTab
            sc_memory_copy(pOutBss, pInBss, sizeof(BSS_ENTRY));
            
            OutTab->BssNr++;
        }
        
		if (OutTab->BssNr >= MAX_LEN_OF_BSS_TABLE)
			break;
 */   
		
    }

	
    BssTableSortByRssi(OutTab);

/*
    if (OutTab->bssdesnum> 0)
	{
	    if (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPA2PSK)
			RTMPMakeRSNIE(pAd, OutTab->bssdesset[0].WPA2.GroupCipher);
		else if (pAdap->wlan_attr.auth_mode == Ndis802_11AuthModeWPAPSK)
			RTMPMakeRSNIE(pAd, OutTab->bssdesset[0].WPA.GroupCipher);
	}
*/

}


// Called from sme_reconnect when:
//    sme_find_best_ap returns TRUE for the given SSID
//    and
//    bss_p->aid is non-zero (we were connected and have not done a disassociate yet)
//    
// Returns TRUE if we need to roam to a new AP.
// If bgscan result handler has caused the sme_reconnect attempt then the bestAP.index
// has been set to SME_INVALID_INDEX. In this case this function will return TRUE.
//
// g_sme.switchToNewSsid could also be set to TRUE if newBestAP is different from BestAP.
// This allerts the caller that dissoc is needed for current AP.
BOOLEAN Mng_roam_threshold_reached( PDSP_ADAPTER_T pAdap, PMNG_DES_T pBestBss)
{
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
    INT32 rssiDbCurrentAP;
    INT32 rssiDbBestAP;

/*	
	pAdapter->g_sme.switchToNewSsid = WLAN_FALSE;

	// Most likely bgscan result has removed AP from GUI list and set index
    // to SME_INVALID_INDEX.  We need to set the roaming flag for SME_COMPLETE
    // to give a port up to Ndis.
	if (SME_INVALID_INDEX == pAdapter->g_sme.bestAP.index) {
        pAdapter->g_sme.ConnectingFromRoaming = WLAN_TRUE;
		CLIENT_PRINTF(
			"%s: %d : Thrshld reached.  No current AP or current AP gone\n", 
			FNAMEGET(__FILE__), __LINE__);
		return WLAN_TRUE;
	}
	
*/
	//current connected AP is not in the new scan list result.
	if(BssTableSearch(&pt_mng->bssdeslist, pAdap->wlan_attr.bssid,pAdap->wlan_attr.current_channel) == BSS_NOT_FOUND)
	{
		return TRUE;
	}

	if ((sc_memory_cmp(pAdap->wlan_attr.ssid, &pBestBss->ssid[1],pBestBss->ssid[0]) == 0)
		&& !IS_MATCH_IN_ADDRESS(pAdap->wlan_attr.bssid, pBestBss->bssid))	//best AP is not the same as the last connected AP
	{
		//pAdapter->g_sme.switchToNewSsid = WLAN_TRUE;
		//CLIENT_PRINTF("%s: %d : switchToNewSsid due to a different SSID\n", FNAMEGET(__FILE__), __LINE__);
		return TRUE;
	}

	// bug 2245 - Roaming fails when AP changes channels without a power cycle.
	// User is connected to AP and then changes AP's channel only.
	// This case is when background scan results removes AP from GUI scan
	// list but AP is found on new channel during the next scan of AP's.
	// newBestAP will match bestAP for ssid and bssid.  If the channel
	// has been changed then treat it as a roaming AP so caller of this
	// routine will disassociate and reconnect not just set sme state
	// back to SME_COMPLETE.
	if ( pAdap->wlan_attr.current_channel != pBestBss->channel )//pt_mng->usedbssdes.channel != pBestBss->channel ) 
	{
	//	pAdapter->g_sme.switchToNewSsid = WLAN_TRUE;
		//CLIENT_PRINTF("%s: %d : switchToNewSsid due to a different ChannelNumber, old=%d, new=%d\n", 
		//	FNAMEGET(__FILE__), __LINE__,
		//	pAdapter->g_sme.bestAP.ucChannelNumber,
		//	pAdapter->g_sme.newBestAP.ucChannelNumber);
		return TRUE;
	}

    rssiDbCurrentAP = rssi_reg_value_to_dbm(pt_mng->usedbssdes.rssi);

    rssiDbBestAP = rssi_reg_value_to_dbm(pBestBss->rssi);

/*
	CLIENT_PRINTF(
		"%s: %d :  ************ best AP BSSID =%x rssi=%d rssiDB =%d current rssi=%d rssi DB=%d\n", 
		FNAMEGET(__FILE__), __LINE__, 
		pAdapter->g_sme.newBestAP.scanResult.bssid[5], 
		pAdapter->g_sme.newBestAP.scanResult.rssi, rssiDbBestAP,
		pAdapter->g_sme.bestAP.scanResult.rssi, rssiDbCurrentAP);
*/

	//prl roaming flag can be set here!
    if ((rssiDbCurrentAP < ROAM_THRESHOLD_RSSI_DBM) && 
        ((rssiDbBestAP - rssiDbCurrentAP) > ROAM_THRESHOLD_DELTA_RSSI_DBM))
       {
	//	CLIENT_PRINTF("%s: %d :  ************ Threshold reached \n", 
	//	FNAMEGET(__FILE__), __LINE__);
        return TRUE;
    }

    return FALSE;
}

VOID  Mng_recover_regs_joined(PDSP_ADAPTER_T pAdap)
{
	UINT8 uRetryCount = 0;
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	
	//set channel
	while(Adap_get_channel(pAdap) != pt_mng->usedbssdes.channel)
	{
		Adap_set_channel(pAdap,pt_mng->usedbssdes.channel);
		if(uRetryCount++ > 3)
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"Mng_recover_regs_joined failed to set channel\n");
			break;
		}
	}

	//set join para
	if(Adap_set_join_para( pAdap,  &pt_mng->usedbssdes) != STATUS_SUCCESS)
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_recover_regs_joined failed ----> set disconnect \n");
		Adap_SetLink(pAdap, LINK_FALSE);
		Adap_UpdateMediaState(pAdap, 0);
		Mng_InitParas(pAdap);
	}
	else
	{
		DBG_WLAN__MLME(LEVEL_TRACE,"Mng_recover_regs_joined ok ----> set connect \n");

		//recover datarate
		pAdap->wlan_attr.rate = Adap_get_rate_from_index(pAdap,pt_mng->rateIndex_b4_roaming);
		Adap_SetRateIndex(pAdap);

		
		pAdap->bStarveMac  = FALSE;
		pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;
		Adap_Set_Driver_State( pAdap, DSP_DRIVER_WORK);
		Adap_SetLink(pAdap, LINK_OK);
		
		pt_mng->statuscontrol.curstatus = ASSOCOK;
		pt_mng->statuscontrol.worktype = 0;

		pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
		if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
			pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
		
		Tx_Send_Next_Frm(pAdap);
//wumin		wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);	//begin tx if some packets in queue
	}

	pAdap->reconnect_status = NO_RECONNECT;
}

VOID Mng_reconnect(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T     pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
	PMNG_DESLIST_T  pOutTab = &pt_mng->OutTab;
	RESET_TYPE_T reset_type;
	UINT8 invalidBssid[6] = {0,0,0,0,0,0};

	if(IS_MATCH_IN_ADDRESS(pAdap->wlan_attr.bssid,invalidBssid))	//invalid bssid, do nothing
	{
		//pAdap->can_do_reconnect_flag = FALSE;
		pAdap->reconnect_status = NO_RECONNECT;
		return;
	}

	if(pAdap->reconnect_status == DOING_RECONNECT
	||pAdap->reconnect_status == DOING_DISCONNECT)	//avoid do same action twice
	{
		return;
	}

	//justin:	080822.	save old bssid for PMKID request
	sc_memory_copy(pAdap->oldBssid,pt_mng->usedbssdes.bssid, WLAN_ETHADDR_LEN);


	pAdap->reconnect_status = DOING_RECONNECT;

	sc_memory_set(pOutTab,0x00,sizeof(MNG_DESLIST_T));

	BssTableInit(pOutTab);
	
	BssTableSsidSort( pAdap, pOutTab, pAdap->wlan_attr.ssid, pAdap->wlan_attr.ssid_len);

	DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ---->bss num = %d\n",pOutTab->bssdesnum);

	
	if(pOutTab->bssdesnum == 0)	// find none Ap match the specific ssid.
	{
		Mng_InitParas(pAdap);
		
		//Indicate disconnect status to up-level
		Adap_SetLink(pAdap, LINK_FALSE);
		Adap_UpdateMediaState(pAdap, 0);
#if 0		
		if(0)//!IS_MATCH_IN_ADDRESS(pAdap->wlan_attr.bssid,invalidBssid)  )	//valid bssid
		{
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_DISASSOC))
			{
		       	// Disassociate with the current AP 
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
										DSP_TASK_EVENT_OID_DISASSOC,
										DSP_TASK_PRI_HIGH,
										NULL,	// not have a  BSSID to specific
										0);	//the last parameter (len) must be set to 0 (zero), driver will indicate up-level after disassocite done.
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ---->bss num = 0, disconnect\n");
				pAdap->reconnect_status = DOING_DISCONNECT;
			}
		}
#endif
		//pAdap->can_do_reconnect_flag = FALSE;
		//if(pAdap->reconnect_status != DOING_DISCONNECT)
			pAdap->reconnect_status = NO_RECONNECT;
		
		//DSP_FREE_MEM(pOutTab,sizeof(MNG_DESLIST_T));
		return;
	}
	
	//pOutTab->bssdesset[0] is the best AP
	
        if (!IS_MATCH_IN_ADDRESS(pAdap->wlan_attr.bssid, invalidBssid) && (pAdap->wlan_attr.aid>0))         // currently connected
	{
		if (Mng_roam_threshold_reached(pAdap,&pOutTab->bssdesset[0]) ) 			// need to connect to a different AP
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ----> diff bss\n");

			if((pAdap->wlan_attr.wep_mode == WEP_MODE_TKIP)
				||(pAdap->wlan_attr.wep_mode == WEP_MODE_AES))
			{
				pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
				pAdap->wlan_attr.wpa_group_key_valid = 0;	//must reget group-key after reconnect
			}

			#if 0
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_DISASSOC)
				&&(BssTableSearch(&pt_mng->bssdeslist, pAdap->wlan_attr.bssid,pAdap->wlan_attr.current_channel) != BSS_NOT_FOUND))//current connected Ap is still in the new scan list result
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ----> diff bss,disconnect old AP\n");
				
		       	// Disassociate with the current AP first
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
					DSP_TASK_EVENT_OID_DISASSOC,DSP_TASK_PRI_HIGH,(PUINT8)pAdap->wlan_attr.bssid,WLAN_ETHADDR_LEN);	
			}
			#endif

			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ----> diff bss,connect new AP\n");
	                	// connect to the new best AP
	                	// reconnect with known bssid and exist AP description in scan list.
	                	reset_type.type = (UINT8)RESET_TYPE_KNOWN_BSSID;
				sc_memory_copy(&pt_mng->usedbssdes, &pOutTab->bssdesset[0], sizeof(MNG_DES_T));
	             		Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID,DSP_TASK_PRI_HIGH,
	             			(PUINT8)&reset_type,sizeof(RESET_TYPE_T));
			}

            	}
            	else 	// not reach roam threshold or current connected AP is still the best AP, just recover all regs before beacon lost
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ----> same bss\n");
			
			//Justin: 080403.	create a task to recover all regs changed by roaming scan
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_RECOVER_REGS_JOINED))
			{
				DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect ----> recover all regs\n");
					
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
						DSP_TASK_EVENT_RECOVER_REGS_JOINED,DSP_TASK_PRI_HIGH,NULL,0);	
			}
			//pAdap->reconnect_status = NO_RECONNECT;
           	}
	}
	else		 // currently not associated with any AP, just join the bestAP
	{
			if((pAdap->wlan_attr.wep_mode == WEP_MODE_TKIP)
				||(pAdap->wlan_attr.wep_mode == WEP_MODE_AES))
			{
				pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
				pAdap->wlan_attr.wpa_group_key_valid = 0;	//must reget group-key after reconnect
			}
	
		if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID))
		{
			DBG_WLAN__MLME(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, reconnect --not connect -> connect\n");
			
			// connect with known bssid and exist AP description in scan list.
			reset_type.type = (UINT8)RESET_TYPE_KNOWN_BSSID;
			sc_memory_copy(&pt_mng->usedbssdes, &pOutTab->bssdesset[0], sizeof(MNG_DES_T));
	      		Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID,DSP_TASK_PRI_HIGH,
	             		(PUINT8)&reset_type,sizeof(RESET_TYPE_T));
		}
        }


	//DSP_FREE_MEM(pOutTab,sizeof(MNG_DESLIST_T));
	//	pAdap->can_do_reconnect_flag = FALSE;
	//pAdap->reconnect_status = NO_RECONNECT;		//cauntion!!!!set NO_RECONNECT only when reconnect finished!!!!
	
}

#endif//ROAMING_SUPPORT

VOID Mng_Fail(PDSP_ADAPTER_T pAdap)
{
	pAdap->scanning_flag = FALSE;		//justin:	080703.	scan must have done if run here.

	pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 0;
	pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze = 0;
	Adap_set_auto_corr(pAdap);
}
