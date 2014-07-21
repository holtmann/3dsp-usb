 /***********************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  
  * FILENAME		:usbwlan_rx.c         VERSION:1.3
  * CREATE DATE	:2009/01/16
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

static char* TDSP_FILE_INDICATOR="RECEV";
/*
#include <ndis.h>
#include <initguid.h>
#include "usbwlan_Task.h"
#include "usbwlan_Sw.h"
#include "usbwlan_mng.h"
#include "usbwlan_proto.h"
#include "usbwlan_Pr.h"
#include "usbwlan_Oid.h"
#include "usbwlan_dbg.h"
#include "usbwlan_rx.h"
#include "usbwlan_Wlan.h"
*/

/*--file local constants and types-------------------------------------*/
#define NEED_DEST_BUFFER_FLAG		0x8000

/*--functions ---------------------------------------------------------*/
void Rx_completion_routine(PDSP_ADAPTER_T	pAdap, INT32 recvcount, PVOID pSrcBuf);

#ifdef RX_BUFFER_BOUNDARY_PROCESS
/***********************************************************************************
*  Function Name:	Rx_buffer_boundary_append_data
*  Description:
*		this function is called by Rx_completion_routine().
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
UINT32 Rx_buffer_boundary_append_data(PDSP_ADAPTER_T	pAdap, UINT32 FrameLen, PVOID pData, UINT32 len)
{
	PFRAG_STRUCT_T pfrag = (PFRAG_STRUCT_T)pAdap->pfrag;	
	PDSP_BUFFER_BOUNDARY_T pBoundary = &pfrag->buffer_boundary;
	UINT32	copied_len;

	sc_spin_lock(&pAdap->lock);
	
	if (pBoundary->copied_bytes == pBoundary->frame_size)
	{
		pBoundary->copied_bytes = 0;
		sc_memory_set(pBoundary->Buffer, 0, DSP_RX_IND_BUF_SIZE);
		pBoundary->frame_size = (UINT16)FrameLen;
	}

	copied_len = ((pBoundary->copied_bytes + len) <= pBoundary->frame_size)?len:(pBoundary->frame_size - pBoundary->copied_bytes);
	sc_memory_copy(pBoundary->Buffer + pBoundary->copied_bytes, pData, copied_len);
	pBoundary->copied_bytes += copied_len;
	
	pBoundary->use_flag = !(pBoundary->copied_bytes == pBoundary->frame_size);

	sc_spin_unlock(&pAdap->lock);
	
	return copied_len;
}
#endif

TDSP_STATUS Frag_ClearInfo(PDSP_ADAPTER_T adaptor)
{
	PFRAG_STRUCT_T pfrag = NULL;
	pfrag= adaptor->pfrag ;
	if(NULL != pfrag)
	{
		sc_memory_set(pfrag->FilterOption,0,sizeof(FILTER_OPTION_T) * MAXFILTERLEN);
	}
	
	return STATUS_SUCCESS;
}

/***********************************************************************************
*  Function Name:	Rx_BuildBulkInTransfer
*  Description:
*		this function is called by Rx_completion_routine().
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_BuildBulkInTransfer(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS ret;
	PFRAG_STRUCT_T pfrag = (PFRAG_STRUCT_T)pAdap->pfrag;

	if (pfrag->r_stoped)
		return;
	
	ret = UsbDev_BuildBulkInTransfer((PUSB_CONTEXT)pAdap->usb_context, 
			&pfrag->BulkInBuffer[0],
			MAXRECDATALEN, 
			Rx_completion_routine, 
			&pfrag->BulkInBuffer[0]);	
	
	if (ret != STATUS_SUCCESS)
	{
		DBG_WLAN__RX(LEVEL_TRACE, "%s(): bulkin fail, try agin\n",__FUNCTION__);
	
		ret = UsbDev_BuildBulkInTransfer((PUSB_CONTEXT)pAdap->usb_context, 
				&pfrag->BulkInBuffer[0],
				MAXRECDATALEN, 
				Rx_completion_routine, 
				&pfrag->BulkInBuffer[0]);	
	}
}

/***********************************************************************************
*  Function Name:	Rx_init
*  Description:
*		this function is called by main control module to initialize fragment module.
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*  Return  Value:
*		STATUS_SUCCESS, executed successfully.
*		STATUS_FAILURE,fail.
* *********************************************************************************/
TDSP_STATUS Rx_init(PDSP_ADAPTER_T pAdap)
{
	PFRAG_STRUCT_T pfrag = NULL;

	DBG_WLAN__RX(LEVEL_TRACE, "Enter [%s]\n",__FUNCTION__);
		
	pAdap->pfrag = NULL;
	
	//Allocate struct memory for frag module structure...
	pfrag = sc_memory_alloc(sizeof(FRAG_STRUCT_T));
	if (pfrag == NULL) 
		return STATUS_FAILURE;;

	sc_memory_set(pfrag, 0, sizeof(FRAG_STRUCT_T));
	
	// init mic structure
	// mic_clear(&pfrag->mic);
	
	// init static data...
	pfrag->oui_8021h[2] = 0xf8;
	pfrag->type_8021x = 0x8e88;
	pfrag->msdu_pri_cfp[0] = 0x01;
	
	p80211_stt_addproto(pfrag, 0x3781); // IPX
	p80211_stt_addproto(pfrag, 0xf380); // Apple talk
//	p80211_stt_addproto(pfrag->f_SttTable,0x8e88); // 1x
	
	pAdap->pfrag = pfrag;

	return STATUS_SUCCESS;
}

/***********************************************************************************
*  Function Name:	Rx_release
*  Description:
*		this function is called by main control module to release rx module.
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_release(PDSP_ADAPTER_T pAdap)
{

	PFRAG_STRUCT_T pfrag = pAdap->pfrag;

	DBG_WLAN__RX(LEVEL_TRACE, "Enter [%s]\n",__FUNCTION__);

	if (pfrag == NULL)
		return;
	
	sc_spin_lock(&pAdap->lock);
//	UsbDev_CancelBulkInTransfer((PUSB_CONTEXT)pAdap->usb_context);

	sc_memory_free(pfrag);

	pAdap->pfrag = NULL;
	sc_spin_unlock(&pAdap->lock);
}

/***********************************************************************************
*  Function Name:	Rx_stop
*  Description:
*		this function is called by main control module to stop rx module.
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_stop(PDSP_ADAPTER_T pAdap)
{
	PFRAG_STRUCT_T pfrag = pAdap->pfrag;

	if(pfrag && pAdap->usb_context)
	{
		DBG_WLAN__RX(LEVEL_TRACE, "Enter [%s]\n",__FUNCTION__);

		pfrag->r_stoped = 1;
		UsbDev_CancelBulkInTransfer((PUSB_CONTEXT)pAdap->usb_context);
	}
}

/***********************************************************************************
*  Function Name:	Rx_restart
*  Description:
*		this function is called by main control module to restart rx module.
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_restart(PDSP_ADAPTER_T pAdap)
{
	PFRAG_STRUCT_T pfrag = pAdap->pfrag;
	
	DBG_WLAN__RX(LEVEL_TRACE, "Enter [%s]\n",__FUNCTION__);

	pfrag->r_stoped = 0;	
	Rx_BuildBulkInTransfer(pAdap);
}

/***********************************************************************************
*  Function Name:	recfrag_set_timer
*  Description:
*		this function set timer valid and set its value to expire.
*  Arguments:
*		prec_element: IN,pointer to receive fragment element structure.
*	TimerValue:	IN,time out value.
*	Return  Value: VOID
* *********************************************************************************/
static inline VOID recfrag_set_timer(PDEFRAG_ELEMENT_T prec_element,UINT16 TimerValue)
{
	prec_element->r_TimerValid = 1;
	prec_element->r_TimerCounter = TimerValue; //500ms unit
}

/***********************************************************************************
*  Function Name:	recfrag_stop_timer
*  Description:
*		this function set timer invalid and set its value to zero.
*  Arguments:
*		prec_element: IN,pointer to receive fragment element structure.
*	Return  Value: VOID
* *********************************************************************************/
static inline VOID recfrag_stop_timer(PDEFRAG_ELEMENT_T prec_element)
{
	prec_element->r_TimerValid = 0;
	prec_element->r_TimerCounter = 0;
}

/***********************************************************************************
*  Function Name:	recfrag_reset_element
*  Description:
*		this function reset element member variables.
*  Arguments:
*		prec_element: IN,pointer to receive fragment element structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID recfrag_reset_element(PFRAG_STRUCT_T pfrag, UINT8 i)
{
	PDEFRAG_ELEMENT_T prec_element;

	if (i >= MAXFRAGCLASS)
		return ;

	pfrag->r_RecFragMap &= (~(1 << i));
	prec_element = &pfrag->r_RecFrag[i];
	
	prec_element->r_BusyFlag = 0;
	prec_element->r_TimerValid = 0;
	prec_element->r_TimerCounter = 0;
	prec_element->r_SeqNum = 0;
	prec_element->r_CurrentFrag = 0;
	prec_element->r_CurrentPoint = 0;
	
	sc_memory_set(prec_element->r_SaddrCached, 0,  WLAN_ETHADDR_LEN);
}

/***********************************************************************************
*  Function Name:	recfrag_copy_data
*  Description:
*		this function copy received frame data to buffer.
*  Arguments:
*		prec_element: IN,pointer to receive fragment element structure.
*		source:	IN,received frame data excluding frame header.
*		len:	IN,received frame data length.
*	Return  Value: INT
*		1,success.
*		0,fail.
* *********************************************************************************/
TDSP_STATUS recfrag_copy_data(PDEFRAG_ELEMENT_T prec_element, VOID *source, UINT16 len)
{
/*	if (source == NULL)
		return STATUS_FAILURE;
	if (prec_element->r_RecCached == NULL)
		return STATUS_FAILURE;*/

	if ((prec_element->r_CurrentPoint + len) > MAXRECDATALEN)
		return STATUS_FAILURE;
	
	sc_memory_copy(prec_element->r_RecCached + prec_element->r_CurrentPoint, source, len);
	prec_element->r_CurrentPoint += len;
	prec_element->r_CurrentFrag++;
	
	return STATUS_SUCCESS;
}

/***********************************************************************************
*  Function Name:	recfrag_set_saddr
*  Description:
*		this function copy received frame source address to buffer.
*  Arguments:
*		prec_element: IN,pointer to receive fragment element structure.
*		Saddr: IN,pointer to receive fragment source address.
*	Return  Value: INT
*		1,success.
*		0,fail.
* *********************************************************************************/
VOID recfrag_set_saddr(PDEFRAG_ELEMENT_T prec_element, PUINT8 Saddr)
{
	if (Saddr && prec_element->r_SaddrCached)
	{
		WLAN_COPY_ADDRESS(prec_element->r_SaddrCached, Saddr);
	}
}

// DriverRecFrag

/***********************************************************************************
*  Function Name:	recfrag_get_free_element
*  Description:
*		this function find and return a free element for defragment.
*  Arguments:
*		pfrag	: IN, pointer to fragment structure.
*		seq		: IN,number of received frame.
*		pSaddr	: IN,source address of received frame.
*  Return  Value: PDEFRAG_ELEMENT_T, pointer to receive fragment structure.
* *********************************************************************************/
PDEFRAG_ELEMENT_T recfrag_get_free_element(PFRAG_STRUCT_T pfrag, UINT16 seq, PUINT8 pSaddr)
{
	UINT8 i = 0;
	
	while(i < MAXFRAGCLASS)
	{
		if ((pfrag->r_RecFragMap & (0xf << i)) == (0xf << i))
		{
			i+= 4;
			continue;
		}
		if (!pfrag->r_RecFrag[i].r_BusyFlag)
			goto RECFRAG_NEW_FLAG;
		i++;
	};

	for (i = 0; i < MAXFRAGCLASS; i++)
	{	
		if (IS_MATCH_IN_ADDRESS(pSaddr, pfrag->r_RecFrag[i].r_SaddrCached))
			goto RECFRAG_NEW_FLAG;
	}
			
	DBG_WLAN__RX(LEVEL_TRACE, "%s: not find\n", __FUNCTION__);
				return NULL;
	
RECFRAG_NEW_FLAG:
	pfrag->r_RecFragMap |= (1 << i);
	pfrag->r_RecFrag[i].r_BusyFlag = 1;
	pfrag->r_RecFrag[i].r_SeqNum = seq;
	recfrag_set_saddr(pfrag->r_RecFrag + i, pSaddr);
	pfrag->r_RecFrag[i].r_CurrentFrag = 0;
	pfrag->r_RecFrag[i].r_CurrentPoint = 0;

	
	return (pfrag->r_RecFrag + i);
}

/***********************************************************************************
*  Function Name:	recfrag_find_element
*  Description:
*		this function try to find a used element according to given sequence number
*		and source address.
*  Arguments:
*		pfrag: IN,pointer to fragment structure.
*		seqnum: IN,number of received frame.
*		saddr:	IN,source address of received frame.
*	Return  Value: VOID
* *********************************************************************************/
PDEFRAG_ELEMENT_T recfrag_find_element(PFRAG_STRUCT_T pfrag, UINT16 seqnum, PUINT8 saddr)
{
	UINT8 i = 0;

	while( i < MAXFRAGCLASS)
	{
		if ((pfrag->r_RecFragMap & (0xf << i)) == 0)
		{
			i += 4;
			continue;
		}
	
		if (pfrag->r_RecFrag[i].r_BusyFlag)
		{
			if ((pfrag->r_RecFrag[i].r_SeqNum == seqnum) && IS_MATCH_IN_ADDRESS(saddr,pfrag->r_RecFrag[i].r_SaddrCached))
				return (&pfrag->r_RecFrag[i]);
		}
		i++;
	};
	
	return (NULL);
}

#ifdef DEBUG_OPEN__WLAN
static inline void recfrag_print_all(PFRAG_STRUCT_T pfrag)
{
	UINT8 i;

	for (i = 0; i < MAXFRAGCLASS; i++)
	{
		if (!pfrag->r_RecFrag[i].r_BusyFlag)
			continue;
		DBG_WLAN__RX(LEVEL_TRACE,"[%d]: seqno = %d  (%d:%d)%x %x %x %x %x %x\n", i, pfrag->r_RecFrag[i].r_SeqNum, 
			pfrag->r_RecFrag[i].r_TimerValid, pfrag->r_RecFrag[i].r_TimerCounter,
			pfrag->r_RecFrag[i].r_SaddrCached[0],
			pfrag->r_RecFrag[i].r_SaddrCached[1],
			pfrag->r_RecFrag[i].r_SaddrCached[2],
			pfrag->r_RecFrag[i].r_SaddrCached[3],
			pfrag->r_RecFrag[i].r_SaddrCached[4],
			pfrag->r_RecFrag[i].r_SaddrCached[5]);
	}
}
#else
static inline void recfrag_print_all(PFRAG_STRUCT_T pfrag)
{
}
#endif

/***********************************************************************************
*  Function Name:	recfrag_reset_all
*  Description:
*		this function reset all used defragment element member variables.
*  Arguments:
*		pfrag: IN,pointer to fragment structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID recfrag_reset_all(PFRAG_STRUCT_T pfrag)
{
	sc_memory_set(pfrag->r_RecFrag, 0, sizeof(DEFRAG_ELEMENT_T) * MAXFRAGCLASS);
}
/***********************************************************************************
*  Function Name:	recfrag_CheckRepeatFilterOption
*  Description:
*		this function check f_filterOption to find out given item.
*  Arguments:
*		pfilter: IN,pointer to filter option structure.
*		seqno:	IN,given sequence number.
*		saddr:	IN,given source address.
*	Return  Value: 
*		STATUS_SUCCESS, successfully executed.
*		STATUS_FAILURE,fail.
* *********************************************************************************/
TDSP_STATUS recfrag_CheckRepeatFilterOption(PFRAG_STRUCT_T pfrag, UINT16 seqno, PUINT8 saddr)
{
	UINT16 i;
	PFILTER_OPTION_T opt;
	
	for (i = 0; i < MAXFILTERLEN; i++)
	{
		opt = &pfrag->FilterOption[i];
		if (opt->seqnum == seqno)
			if (IS_MATCH_IN_ADDRESS(saddr, opt->saddr))
				return (STATUS_SUCCESS);		
	}
	
	return (STATUS_FAILURE);
}

/***********************************************************************************
*  Function Name:	recfrag_AddFilterOption
*  Description:
*		this function add filter item to structure f_filteroption.
*  Arguments:
*		pfrag: IN,pointer to fragment structure.
*		seqno:	IN,given sequence number.
*		saddr:	IN,given source address.
*	Return  Value: VOID
* *********************************************************************************/
VOID recfrag_AddFilterOption(PFRAG_STRUCT_T pfrag, UINT16 seqno, PUINT8 saddr)
{
	PFILTER_OPTION_T opt;
	
	if (pfrag->FilterPoint >= MAXFILTERLEN)
		pfrag->FilterPoint = 0;
	
	opt = &pfrag->FilterOption[pfrag->FilterPoint];
	opt->seqnum = seqno;
	if (saddr)
	{
		WLAN_COPY_ADDRESS(opt->saddr, saddr);
	}
	pfrag->FilterPoint++;
}

/***********************************************************************************
*  Function Name:	Rx_get_counts
*  Description:
*		this function is called by main control module to get the statistic data
*  Arguments:
*		pAdap: IN, pointer to driver information structure.
*		other: OUT, each statistic data pointer
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_get_counts(PDSP_ADAPTER_T pAdap, 
						PUINT32		p_good_cnt,
						PUINT32		p_good_payload_cnt,
						PUINT32		p_error_cnt,
						PUINT32		p_err_nwid_cnt ,
						PUINT32		p_err_decode_cnt, 
						PUINT32		p_err_defrag_cnt,
						PUINT32		p_dropped_pkt_cnt,
						PUINT32		p_mc_pkt_cnt)
{
    PUINT32			p_Counts;
    PFRAG_STRUCT_T pfrag = pAdap->pfrag;

    if(pfrag == NULL)
    {
        DBG_WLAN__RX(LEVEL_ERR,"[%s]:pfrag is null,maybe has been released!\n",__FUNCTION__);		
        return;
    }
    
	p_Counts = &pfrag->i_Stat_Counts[0];

	if (	p_good_cnt 			!= NULL)
		*p_good_cnt 		= p_Counts[RX_STAT_GOOD_CNT];
	
	if (	p_good_payload_cnt != NULL)
		*p_good_payload_cnt = p_Counts[RX_STAT_GOOD_PAYLOAD_CNT];
	
	if (	p_error_cnt 			!= NULL)
		*p_error_cnt 		= p_Counts[RX_STAT_ERROR_CNT];	
	
	if (	p_err_nwid_cnt		!= NULL)
		*p_err_nwid_cnt 	= p_Counts[RX_STAT_ERR_NWID_CNT];
	
	if (	p_err_decode_cnt	!= NULL)
		*p_err_decode_cnt 	= p_Counts[RX_STAT_ERR_DECODE_CNT];
	
	if (	p_err_defrag_cnt	!= NULL)
		*p_err_defrag_cnt 	= p_Counts[RX_STAT_ERR_DEFRAG_CNT];	
	
	if (	p_dropped_pkt_cnt	!= NULL)
		*p_dropped_pkt_cnt 	= p_Counts[RX_STAT_DROPPED_PKT_CNT];
	
	if (	p_mc_pkt_cnt		!= NULL)
		*p_mc_pkt_cnt 		= p_Counts[RX_STAT_MC_PKT_CNT];
	
}

/***********************************************************************************
*  Function Name:	Rx_set_counts
*  Description:
*		this function is called by rx module to set the statistic data
*  Arguments:
*		pAdap: IN,pointer to driver information structure.
*		static_id:IN, statistic type
*		cnt: IN, statistic number
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_set_counts(PDSP_ADAPTER_T pAdap, UINT32 static_id, UINT32 cnt)
{
	PFRAG_STRUCT_T pfrag = pAdap->pfrag;
	
	if (static_id >= RX_STAT_CNT)
		return;
	
	pfrag->i_Stat_Counts[static_id] += cnt;
	
	if (static_id == RX_STAT_GOOD_PAYLOAD_CNT)
		pfrag->i_Stat_Counts[RX_STAT_GOOD_CNT] += 1;
}

/***********************************************************************************
*  Function Name:	Rx_data_filter
*  Description:
*		this function is called by rx module to judge if the data packet should be discarded
*  Arguments:
*		pAdap: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
TDSP_STATUS Rx_data_filter(PDSP_ADAPTER_T pAdap, PUINT8 pDesBuf)
{
	PETHER_HEADER_T pEther_header = (PETHER_HEADER_T)pDesBuf;
		
	if (pAdap->wlan_attr.hasjoined != JOIN_HASJOINED)
	{
		if (pEther_header->type == 0x8e88)
		{
			DBG_WLAN__RX(LEVEL_TRACE,"receive 802.1x , current QoS is %s.\n",(sc_netq_ifstop(pAdap->net_dev) == 1)? "stopped":"started");			
		}
		else
		{	
			goto error_rx_data_filter;
		}
	}

	if ((pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK) 
		&&	!pAdap->wlan_attr.wpa_group_key_valid)
	{
		if (pEther_header->type != 0x8e88)
		{
			goto error_rx_data_filter;
		}
		else
		{
			DBG_WLAN__RX(LEVEL_TRACE,"receive 802.1x packet, current QoS is %s.\n",(sc_netq_ifstop(pAdap->net_dev) == 1)? "stopped":"started");			
		}
	}

	if (!MAC_ADDR_IS_GROUP(pEther_header->daddr))
	{
		if (0 != sc_memory_cmp(pEther_header->daddr, pAdap->current_address, WLAN_ADDR_LEN))
			goto error_rx_data_filter;
	}
	
	return STATUS_SUCCESS;
	
error_rx_data_filter:
	DBG_WLAN__RX(LEVEL_TRACE, "%s: fail, auth_mode = %d, type = %d\n", __FUNCTION__, pAdap->wlan_attr.auth_mode, pEther_header->type);
	return STATUS_FAILURE;	
}

/***********************************************************************************
*  Function Name:	Rx_defragment_timeout
*  Description:
*		this function is called by main control module to inspire timer used by receive
*		defragment module.
*  Arguments:
*		pAdap: IN,pointer to driver information structure.
*	Return  Value: VOID
* *********************************************************************************/
VOID Rx_defragment_timeout(PDSP_ADAPTER_T pAdap)
{
	PFRAG_STRUCT_T pfrag = pAdap->pfrag;
	UINT8	i = 0;

	if(!pfrag)
		return;

	/* Lock */
	sc_spin_lock(&pAdap->lock);
	
	while (i < MAXFRAGCLASS)
	{
		if ((pfrag->r_RecFragMap & (0xf << i)) == 0)
		{
			i += 4;
			continue;
		}

		if (pfrag->r_RecFrag[i].r_BusyFlag && pfrag->r_RecFrag[i].r_TimerValid)
		{
			pfrag->r_RecFrag[i].r_TimerCounter--;
			if (pfrag->r_RecFrag[i].r_TimerCounter == 0)
			{
				DBG_WLAN__RX(LEVEL_TRACE, "Rec frag timeout[%d]: seqno = %d  (%d:%d)%x %x %x %x %x %x\n", i, pfrag->r_RecFrag[i].r_SeqNum, 
				pfrag->r_RecFrag[i].r_TimerValid, pfrag->r_RecFrag[i].r_TimerCounter,
				pfrag->r_RecFrag[i].r_SaddrCached[0],
				pfrag->r_RecFrag[i].r_SaddrCached[1],
				pfrag->r_RecFrag[i].r_SaddrCached[2],
				pfrag->r_RecFrag[i].r_SaddrCached[3],
				pfrag->r_RecFrag[i].r_SaddrCached[4],
				pfrag->r_RecFrag[i].r_SaddrCached[5]);
				recfrag_reset_element(pfrag, i);
			}
		}
		i++;
	};

	/* Unlock */
	sc_spin_unlock(&pAdap->lock);
}



/***********************************************************************************
*  Function Name:	Rx_recfrag_defragment
*  Description:
*		given a frame from hardware, this function search and manage receiving
*		queue. if got a completed PDU, it will copy completed data to destination
*		buffer, else, save the received fragment waiting for next frame.
*  Arguments:
*		adaptor: IN,pointer to driver information structure.
*		source:	IN,data buffer got from hardware including descriptor head and tail. in 3dsp frame format(from fc to body ending)
*		src_len: IN,source data buffer length. (from fc to body ending)
*		max_des_len: IN, max output buffer length.
*		des_buffer:	OUT,data buffer for indicating up level which was processed 
*			by protocol header,attention: it may be a NULL pointer, if in this case,
*			if need to copy data to destination buffer, drop the packages.
*		pdes_len: OUT,pointer to output buffer length.
*		prx_paring_result: IN, the result have gotten previously 
*	Return  Value: 
*		STATUS_SUCCESS, successfully executed.
*		STATUS_FAILURE,fail.
*		TDSP_STATUS_ALREADY_MAPPED,processed and waiting for further frame.
* *********************************************************************************/
TDSP_STATUS Rx_recfrag_defragment(PDSP_ADAPTER_T adaptor, 
						PUINT8 source,
						UINT16 src_len,
						UINT16 max_des_len,
						PUINT8 des_buffer,
						PUINT16 pdes_len,
						PRX_PARSING_RESULT_T prx_paring_result)
{
	PFRAG_STRUCT_T pfrag = adaptor->pfrag;
	HEADER_TRANSLATE_T header_trans;
	PDSP_ATTR_T pwlan_attr = &adaptor->wlan_attr;
	PMICHAEL_T pmic = &pfrag->mic;
	UINT8 rxind;

	PUINT32 tmpbufhead;
	PUINT8 saddr = NULL;
	PUINT8 daddr = NULL;
	UINT16 seqnum;
	UINT16 fragnum;
	PUINT8 tmpbufaddr;
	UINT16 tmpbuflen;
	PDEFRAG_ELEMENT_T pTmpElement;
	UINT8   micvalue[WLAN_MIC_VALUE_COUNT];
	TDSP_STATUS ret ;

	P80211_HEADER_A3_T p80211_header = (P80211_HEADER_A3_T)source;

	//check if destination buffer is NULL
	if(des_buffer == NULL)
	{
		DBG_WLAN__RX(LEVEL_TRACE, "%s: des_buffer == NULL\n", __FUNCTION__);
		return STATUS_FAILURE;
	}

	tmpbufhead = (PUINT32)(source);

	// get source address and destination address.
	Proto_get_addr(p80211_header, &saddr, &daddr);
	
	//get sequence number and frag number.
	seqnum = (UINT16)WLAN_GET_SEQ_SEQNUM(p80211_header->seq);
	fragnum = (UINT16)WLAN_GET_SEQ_FRGNUM(p80211_header->seq);
	
	if (p80211_header->MoreFrag || (fragnum != 0))
	{
		sc_spin_lock(&adaptor->lock);
		pTmpElement = recfrag_find_element(pfrag, seqnum, saddr);
		if (pTmpElement == NULL)
		{
			if (fragnum == 0)
				pTmpElement = recfrag_get_free_element(pfrag, seqnum, saddr);
			
		}
		sc_spin_unlock(&adaptor->lock);

		if (pTmpElement == NULL)
		{
			if (p80211_header->MoreFrag != 0)
			{
				DBG_WLAN__RX(LEVEL_TRACE, "NULL  %d m=%d s=%d  ,  saddr = %x %x %x %x %x %x\n", 
					fragnum, p80211_header->MoreFrag, seqnum, 
					saddr[0], saddr[1], saddr[2], saddr[3], saddr[4], saddr[5]);
				recfrag_print_all(pfrag);
				sc_spin_lock(&adaptor->lock);
				Rx_set_counts(adaptor, RX_STAT_DROPPED_PKT_CNT, 1);
				sc_spin_unlock(&adaptor->lock);
			}
			return STATUS_DUMMY;
		}
		if (pTmpElement->r_CurrentFrag != fragnum)
		{
			if (pTmpElement->r_CurrentFrag == (1+fragnum))
			{
				sc_spin_lock(&adaptor->lock);
				recfrag_set_timer(pTmpElement, MAXFRAGTIMEOUT);
				sc_spin_unlock(&adaptor->lock);
				return STATUS_PENDING;
			}
			
			DBG_WLAN__RX(LEVEL_TRACE, "%d %d m=%d s=%d\n",  pTmpElement->r_CurrentFrag, fragnum, p80211_header->MoreFrag, seqnum);
			return STATUS_FAILURE;
		}

		sc_spin_lock(&adaptor->lock);
		if (fragnum == 0)
		{
			ret = recfrag_copy_data(pTmpElement, source, src_len);
		}
		else
		{
			ret = recfrag_copy_data(pTmpElement, source + prx_paring_result->body_offset_fc, prx_paring_result->body_len);
		}
		
		if (STATUS_FAILURE == ret)
		{
			DBG_WLAN__RX(LEVEL_TRACE, "%d %d %d\n",  pTmpElement->r_CurrentPoint, src_len, MAXRECDATALEN);
			sc_spin_unlock(&adaptor->lock);
			return ret;
		}

		if (!p80211_header->MoreFrag)
		{
			recfrag_stop_timer(pTmpElement);
			tmpbufaddr = pTmpElement->r_RecCached;
			tmpbuflen = pTmpElement->r_CurrentPoint;
			recfrag_reset_element(pfrag, pTmpElement - &pfrag->r_RecFrag[0]);
			sc_spin_unlock(&adaptor->lock);
		}
		else
		{
			recfrag_set_timer(pTmpElement, MAXFRAGTIMEOUT);
			sc_spin_unlock(&adaptor->lock);
			return STATUS_PENDING;
		}
	}
	else
	{
		tmpbufaddr = source;
		tmpbuflen = src_len;
	}
		
	//discard this packet without valid key for tkip
	//Otherwise sometimes driver parses the bufferred the tkip packet then mic failure will happen. 
	if(prx_paring_result->is_wep)
	{
		if (!MAC_ADDR_IS_GROUP(daddr) && 
			(pwlan_attr->wep_mode == WEP_MODE_TKIP) &&
			(adaptor->wlan_attr.wpa_pairwise_key_valid == 0))
		{
			DBG_WLAN__RX(LEVEL_INFO, "TKIP state mismatch ,pairwise\n");
			return STATUS_FAILURE;
		}

		if (MAC_ADDR_IS_GROUP(daddr) && 
			(pwlan_attr->group_cipher == WEP_MODE_TKIP) &&
			(adaptor->wlan_attr.wpa_group_key_valid == 0))
		{
			DBG_WLAN__RX(LEVEL_INFO, "TKIP state mismatch,group \n");
			return STATUS_FAILURE;
		}		
	}
		
	// tkip mic process			// confirm by Justin
	if (prx_paring_result->is_wep && 
		((!MAC_ADDR_IS_GROUP(daddr) && (pwlan_attr->wep_mode == WEP_MODE_TKIP)) ||//unicast
		 (MAC_ADDR_IS_GROUP(daddr) && (pwlan_attr->wep_mode != WEP_MODE_WEP) && ( pwlan_attr->group_cipher== WEP_MODE_TKIP))//broadcast or multicast
		 ))	//justin:	080901.	counter measure start whatever TKIP used in pairwise or group
	{
		if(REQUEST_CM_RETURN_GO_ON != Adap_StartTKIPCounterMeasure(adaptor,REQUEST_CM_STOP_CURRENT,0))
		{
			DBG_WLAN__RX(LEVEL_TRACE, "%s fail!\n", __FUNCTION__);
			return STATUS_FAILURE;
		}

		sc_spin_lock(&adaptor->lock);
		if (MAC_ADDR_IS_GROUP(daddr))//broadcast or multicast
		{	
		    rxind = GetTkipRxIndex(tmpbufaddr + WLAN_HDR_A3_LEN + WLAN_WEP_IV_LEN -1);
			//DBGSTR(("### counter measure get a group data,index = %x\n",rxind));
			rxind = (rxind > 4) ? 4:rxind;
			mic_setKey(pmic,&pwlan_attr->wpa_group_mic_rx[rxind][0]);
		}
		else//unicast
		{
			//DBG_WLAN__RX(LEVEL_INFO, "### counter measure get a pairwise data\n");
			mic_setKey(pmic, pwlan_attr->wpa_pairwise_mic_rx);
		}
		
		mic_append(pmic, daddr, WLAN_ETHADDR_LEN);
		mic_append(pmic, saddr, WLAN_ETHADDR_LEN);
		mic_append(pmic, pfrag->msdu_pri_cp, 4);
		mic_append(pmic, tmpbufaddr + prx_paring_result->body_offset_fc, tmpbuflen - prx_paring_result->body_offset_fc - WLAN_MIC_VALUE_COUNT);
		mic_getMIC(pmic, micvalue);
		if (0 != (sc_memory_cmp(micvalue, tmpbufaddr + tmpbuflen - WLAN_MIC_VALUE_COUNT, WLAN_MIC_VALUE_COUNT)))
		{
			Rx_set_counts(adaptor, RX_STAT_ERR_DECODE_CNT, 1);
			sc_spin_unlock(&adaptor->lock);
			DBG_WLAN__RX(LEVEL_TRACE, "### counter measure fail \n");

			// Mic is not mismatched! So start TKIP counter measure
			Adap_StartTKIPCounterMeasure(adaptor,REQUEST_CM_CALCU,MAC_ADDR_IS_GROUP(daddr));
			return STATUS_FAILURE;
		}
		sc_spin_unlock(&adaptor->lock);		
		tmpbuflen -= WLAN_MIC_VALUE_COUNT;
	}
		
	//check repeat package !		
	if (recfrag_CheckRepeatFilterOption(pfrag, seqnum, saddr) == STATUS_SUCCESS)
	{
//		DBG_WLAN__RX(LEVEL_TRACE, "%s fail!\n", __FUNCTION__);		
		return STATUS_DUMMY;
	}

	sc_spin_lock(&adaptor->lock);
	recfrag_AddFilterOption(pfrag, seqnum, saddr);		
	sc_spin_unlock(&adaptor->lock);
	
	sc_memory_set((PVOID)&header_trans, 0, sizeof(HEADER_TRANSLATE_T));
	
	// do 802.11 to 802.3 translation and copy data to dest buffer.
	header_trans.p80211_header = (P80211_HEADER_T)tmpbufaddr;//p80211;
	header_trans.src_len = tmpbuflen > DSP_MAX_RECDATA_LEN ? DSP_MAX_RECDATA_LEN : tmpbuflen;
	header_trans.p80211frmlen = header_trans.src_len;
	header_trans.psource = tmpbufaddr + prx_paring_result->body_offset_fc;// point to data body
	header_trans.src_len -= prx_paring_result->body_offset_fc;// get data body len
	header_trans.pdes = des_buffer;
	header_trans.pdes_len = pdes_len;
	header_trans.max_des_len = max_des_len;
	
	//translate protocol head.
	sc_spin_lock(&adaptor->lock);	
	ret = Proto_802_11_to_ether(adaptor, &header_trans);
	sc_spin_unlock(&adaptor->lock);	
	if (ret !=  STATUS_SUCCESS)
		DBG_WLAN__RX(LEVEL_TRACE, "Proto_802_11_to_ether fail\n");
	return ret;
}

/***********************************************************************************
* Function Name:	Rx_process
* Description:
*      First this function will get the count of this MPDU. Then this function
*      will call the function for processing rx of frag module.	
* Arguments:
*	pAdap: IN, the pointer of adapter context.
*	p3DspSrcBuf: IN, the buffer saving the processing data. in 3dsp frame format
*	pDesBuf: OUT, the buffer saving the converted data.
*	poutcount: OUT, the pointer of the length of this MPDU.
*	prealbyte: OUT, the real bytes of the converted frame.
*	rescount: IN, the reserved count 
* Return  Value:
*	STATUS_SUCCESS  data needs to be indicated to up layer.
*	STATUS_xxx  others.
* *********************************************************************************/
UINT32 Rx_process(PDSP_ADAPTER_T pAdap, PUINT8 p3DspSrcBuf, PUINT8 pDesBuf, 
						PUINT32 poutcount, PUINT32 prealbyte, UINT32 rescount)
{
	RX_PARSING_RESULT_T	rx_paring_result;
	PUINT32	pheaddata, ptaildata;
	/*Save the detail information about tail. */
	UINT8	tmpdatarssi;//,tmpdatarate;
	/*A regular 802.11 head format.*/
	P80211_HEADER_A3_T a3;
	/* for packet filter */
	UINT8	packettype;
	UINT16 usAlignBodyLen;			//it maybe add addition padding data to the last frag (when moreflag ==0) for align 4 bytes in 3dsp spec.
	PRX_TO_MNG_PARAMETER pMngParam;
	UINT8   cipher_mode = 0;
	PMNG_STRU_T pt_mng = pAdap->pmanagment;
	BOOLEAN		match = FALSE;
	
	/*Save the status. */
	TDSP_STATUS status = STATUS_SUCCESS;
	
	*poutcount = rescount;

	sc_memory_set(&rx_paring_result, 0, sizeof(RX_PARSING_RESULT_T));
	
	/* Get the head and its detail information. */
	pheaddata = (PUINT32)p3DspSrcBuf;
	// check the first two bytes. a good frame was received if it is 0x0000. otherwise, bad frame and will be discard
	//	if ((pheaddata & 0x00001111) != 0)
	//	{
	//		return DSP_USB_STATUS_BAD_FRM;
	//	}
	
	rx_paring_result.in_air_len = RXBUF_GET_RXDATALEN(*pheaddata); // in bytes, in air len
	rx_paring_result.body_padding_len = ((rx_paring_result.in_air_len % 4) == 0) ? 0: (4 - (rx_paring_result.in_air_len % 4));
	rx_paring_result.speed	= RXBUF_GET_RXDATARATE(*pheaddata);

	rx_paring_result.body_len = rx_paring_result.in_air_len - sizeof(T80211_HEADER_A3_T) - DOT11_FRM_FCS_LEN;//to be calculate
	rx_paring_result.body_offset_fc = sizeof(T80211_HEADER_A3_T);//to be calculate
	
	/* Because there is received data, it doesn't need RSSI value from PHY register. Reset timer */
	//	Phy_ResetCheckRssiTimer(pAdap);			// need to confirm ,,,,Justin
	// DBG_WLAN__RX(LEVEL_INFO, "Rx_process: rssi value = %d\n", pAdap->rssi);
	
	/* Pointer to a type of 802.11 head format.*/
	a3 = (P80211_HEADER_A3_T)(p3DspSrcBuf + sizeof(UINT32));

	if((a3->SubType == WLAN_FSTYPE_PROBERESP)
		&&(MAC_ADDR_IS_GROUP(a3->a1)))
	{
		DBG_WLAN__RX(LEVEL_INFO, "rx group or multicast proberesp \n");
		//Adap_PrintBuffer(p3DspSrcBuf, rescount);
	}
	
	//Heart beating mechnism
	//now that it's a packet from ap, reset counter
	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
	{
		if(IS_MATCH_IN_ADDRESS(a3->a2, pt_mng->usedbssdes.bssid))//pAdap->wlan_attr.bssid))//Justin: 0727. only the ap directed packets
																//justin:071220. for roaming, should clear beacon lost while connecting other ap with same ssid
		{
			pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
			if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
				pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
		}	
	}
	//woody add for ibss beacon lost
	else
	{
	//FOR Ibss , just compare a3 called bssid
		if(IS_MATCH_IN_ADDRESS(a3->a3,pAdap->wlan_attr.bssid))//woody 0824
		{
			pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	
			if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
				pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
		}
	}

	if (a3->ToDS && a3->FromDS) // from ds to ds, address 4 is present
	{
		rx_paring_result.a4_len = DSP_USB_RX_FRM_ADDR4_AND_PADDING_LEN;
		rx_paring_result.body_len -= rx_paring_result.a4_len;
		rx_paring_result.body_offset_fc += rx_paring_result.a4_len;
	}

	rx_paring_result.is_wep = (pAdap->wlan_attr.gdevice_info.privacy_option && a3->ProtectedFrame)?1:0;

	if (rx_paring_result.is_wep)	/*wep = 1*/
	{
		/* Get the tail and its detail information. */	
		//Jakio20070709: in wpa or wpa2 mixed mode, broadcast and unicast may differ in ecryption mode
#if 1
		//if(MAC_ADDR_IS_BCAST(a3->a1) && (pAdap->wlan_attr.wep_mode != WEP_MODE_WEP))
		if(MAC_ADDR_IS_GROUP(a3->a1) && (pAdap->wlan_attr.wep_mode != WEP_MODE_WEP))//justin: 080901.	broadcast and multicast frames use group key
		{
			cipher_mode = pAdap->wlan_attr.group_cipher;
		}
		else
#endif
		{
			cipher_mode = pAdap->wlan_attr.wep_mode;
		}

		switch (cipher_mode)
		{
		case WEP_MODE_WEP:
			rx_paring_result.iv_or_eiv_len =  DOT11_RX_FRM_WEP_IV_LEN; 
			rx_paring_result.body_len -= (DOT11_RX_FRM_WEP_IV_LEN + DOT11_RX_FRM_WEP_ICV_LEN);
			break;
		case WEP_MODE_TKIP:
			rx_paring_result.iv_or_eiv_len =  DOT11_RX_FRM_WEP_IV_LEN + DOT11_RX_FRM_TKIP_EIV_LEN; // skip IV and EIV
			rx_paring_result.body_len -= (DOT11_RX_FRM_WEP_IV_LEN + DOT11_RX_FRM_WEP_ICV_LEN + DOT11_RX_FRM_TKIP_EIV_LEN);
			//			ulAdditionBodyLen = DSP_USB_RX_MSDU_TKIP_MIC_LEN;
			break;
		default:	// CCMP...
			rx_paring_result.iv_or_eiv_len = DOT11_RX_FRM_CCMP_HDR_LEN; // skip CCMP hdr
			rx_paring_result.body_len -= (DOT11_RX_FRM_CCMP_HDR_LEN + DOT11_RX_FRM_CCMP_MIC_LEN);
			break;
		}
		
		rx_paring_result.body_offset_fc += rx_paring_result.iv_or_eiv_len;
	}

	usAlignBodyLen = 0;
	if (!a3->MoreFrag)	// the last frag
	{
		usAlignBodyLen = rx_paring_result.body_len & 0x3;
		usAlignBodyLen = (usAlignBodyLen == 0) ? 0 : (4 - usAlignBodyLen);
	}
	
	/* There would be addition offset(hi & lo) fields if it is probe res of beacon flag*/
	if ((a3->Type == WLAN_FTYPE_MGMT) &&
		((a3->SubType == WLAN_FSTYPE_PROBERESP) || (a3->SubType == WLAN_FSTYPE_BEACON)))
	{
		if((a3->SubType == WLAN_FSTYPE_PROBERESP)
			&&(MAC_ADDR_IS_GROUP(a3->a1)))
		{
		}
		else	
		{
			rx_paring_result.offset_len = DSP_USB_RX_FRM_OFFSET_FIELD_LEN;
		}
	}
	
	// whether the left data in this buffer is a total frame. if not, return..
	*poutcount =  sizeof(UINT32) + rx_paring_result.body_offset_fc + rx_paring_result.body_len + usAlignBodyLen + DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN + rx_paring_result.offset_len;
	if (*poutcount > rescount)
	{
		DBG_WLAN__RX(LEVEL_TRACE, " outcount > rescount %d > %d\n", *poutcount ,rescount);
//	 	DBG_WLAN__RX(LEVEL_TRACE, "type = %d %d, %d\n", a3->Type, a3->SubType, ((pAdap->wlan_attr.hasjoined == JOIN_NOJOINED) || (pAdap->link_ok == LINK_FALSE)));
//		DBG_WLAN__RX(LEVEL_TRACE, "in_air_len = %d, a4_len = %d, is_wep = %d, body_padding_len = %d, iv_or_eiv_len = %d, offset_len = %d\n",
//			rx_paring_result.in_air_len, rx_paring_result.a4_len, rx_paring_result.is_wep, rx_paring_result.body_padding_len,
//			rx_paring_result.iv_or_eiv_len, rx_paring_result.offset_len);
//		DBG_PRINT_BUFF("Rx_process", p3DspSrcBuf, rescount);
		return (STATUS_INSUFFICIENT_RESOURCES);
	}
	else if (*poutcount < rescount)
	{
//		DBG_WLAN__RX(LEVEL_TRACE, " %d %d %d %d %d\n", a3->Type, a3->SubType, cipher_mode, *poutcount , rescount);
//		DBG_PRINT_BUFF("Rx_process", p3DspSrcBuf, rescount);
	}
	//	rx_paring_result.frame_len = *poutcount;
	

	/* Get the tail and its detail information. */
	ptaildata = (PUINT32)(p3DspSrcBuf + sizeof(UINT32) + rx_paring_result.body_offset_fc + rx_paring_result.body_len + usAlignBodyLen);
	tmpdatarssi = RXBUF_GET_RSSI(*ptaildata);
	//v4chip
	 if (pAdap->wlan_attr.chipID)
	 {
	 #if 1
	 	if(!RXBUF_IS_RX_MAGIC(*ptaildata))
		 {
		 	DBG_WLAN__RX(LEVEL_TRACE, "type = %d %d, %d\n", a3->Type, a3->SubType, ((pAdap->wlan_attr.hasjoined == JOIN_NOJOINED) || (pAdap->link_ok == LINK_FALSE)));
			DBG_WLAN__RX(LEVEL_TRACE, "Frame end delimiter not found! tail = 0x%x outcount = %d\n",*ptaildata, *poutcount);
			DBG_WLAN__RX(LEVEL_TRACE, "head = 0x%x, body_offset_fc = %d, body_len = %d, usAlignBodyLen = %d\n",
				*p3DspSrcBuf,rx_paring_result.body_offset_fc , rx_paring_result.body_len , usAlignBodyLen);
			DBG_WLAN__RX(LEVEL_TRACE, "in_air_len = %d, a4_len = %d, is_wep = %d, body_padding_len = %d, iv_or_eiv_len = %d, offset_len = %d\n",
				rx_paring_result.in_air_len, rx_paring_result.a4_len, rx_paring_result.is_wep, rx_paring_result.body_padding_len,
				rx_paring_result.iv_or_eiv_len, rx_paring_result.offset_len);
			Adap_PrintBuffer(p3DspSrcBuf, rescount);

			pAdap->mib_stats.error_packet_rx++;
			*poutcount = rescount;//sizeof(UINT32) + sizeof(UINT32) + ALIGNDATALEN(tmpdatalen);

			return STATUS_FAILURE;//justin:	080901.	this a bad frame, discard it
		 }
	#endif
	 }
	/* Save rssi value.*/
	pAdap->rssi = tmpdatarssi;

	if(pAdap->link_ok == LINK_OK) //when connected AP
	{
	#ifdef ROAMING_SUPPORT
		if(pAdap->reconnect_status != NEED_RECONNECT)
	#endif		//#ifdef ROAMING_SUPPORT
		{	//justin:	080407.  normal run
			if(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
			{
				match = IS_MATCH_IN_ADDRESS(a3->a2,pAdap->wlan_attr.bssid);
			}
			else
			{
				match = IS_MATCH_IN_ADDRESS(a3->a3,pAdap->wlan_attr.bssid);
			}
		}
	
		if (match) //only when the ap directed packets, we update connected AP's rssi
		{
			//DBG_WLAN__RX(LEVEL_INFO, "update rssi = 0x%x\n",tmpdatarssi);
			Mng_UpdateRssi(pAdap,tmpdatarssi);
		}
	}
	

	//copy rssi value to the reserved bytes in the begin of 3dspsrcbuf. for latter use, this is very important
	//sc_memory_copy(p3DspSrcBuf,ptaildata,sizeof(UINT8));
	
	/*Set packetype as unicast */
	packettype = 0;
	
	pAdap->mib_stats.total_packet_rx++;
	pAdap->mib_stats.total_bytes_rx += rescount;
	
#if 0	
	/* Raw frame is a good frame. */ //Justin: 0711. we regard it is good if include Fixed_value(0xDEDBEF)
	//ulTemp = *((PUINT32)(p3DspSrcBuf+rescount-rx_paring_result.offset_len -3));
	ulTemp = 0xDEDBEF;
	if (!tdsp_memequ((p3DspSrcBuf + rescount - rx_paring_result.offset_len - 3), &ulTemp, 3))
	{
		pAdap->mib_stats.error_packet_rx++;
		DBG_WLAN__RX(LEVEL_TRACE, " tdsp_memequ %d > %d\n", *poutcount ,rescount);
		*poutcount = rescount;//sizeof(UINT32) + sizeof(UINT32) + ALIGNDATALEN(tmpdatalen);
		return STATUS_FAILURE;
	}
#endif

	pAdap->mib_stats.good_packet_rx++;
	
	if (MAC_ADDR_IS_GROUP(a3->a1))
	{
		if (MAC_ADDR_IS_BCAST(a3->a1))
		{
			packettype = 1; // broadcast
			pAdap->mib_stats.broadcast_rx++;
		}
		else
		{
			packettype = 2; // multicast
			pAdap->mib_stats.multicast_rx++;
		}
		Rx_set_counts(pAdap, RX_STAT_MC_PKT_CNT, 1);
	}
	else
	{
		packettype = 0; // unicast
		pAdap->mib_stats.unicast_rx++;
	}

#if 	0
	if ((pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
		&& (pAdap->link_ok == LINK_OK) 
		&& (a3->Type == WLAN_FTYPE_DATA))
	{
		DBG_WLAN__RX(LEVEL_INFO, "dsp rx ibss\n");
	}
#endif

	if (a3->Type == WLAN_FTYPE_DATA)
	{
		if ((pAdap->wlan_attr.hasjoined == JOIN_NOJOINED) 
			|| (pAdap->link_ok == LINK_FALSE))
		{
//			DBG_WLAN__RX(LEVEL_TRACE, " hasjoined %d > %d\n", pAdap->wlan_attr.hasjoined ,pAdap->link_is_active);
			return (STATUS_ADAPTER_NOT_READY);
		}

		/* Lock */
//		sc_spin_lock(&pAdap->lock);
		/* First defragment, then convert 802.11 to 802.3 */
		status = Rx_recfrag_defragment(pAdap,
							p3DspSrcBuf + sizeof(UINT32),
							rx_paring_result.body_offset_fc + rx_paring_result.body_len,
							rescount, pDesBuf, (PUINT16)prealbyte,
							&rx_paring_result);
		/* Unlock */
//		sc_spin_unlock(&pAdap->lock);

		//zyy: if more data = 0, sta will enter power save mode.
		/*
		The More Data field is valid in directed data or management type frames transmitted by an AP to an STA in power-save mode.
		A value of 1 indicates that at least one additional buffered MSDU, or MMPDU, is present for the same STA.*/
		if (a3->FromDS && !a3->MoreData)		//from ds to sta directly, and more data == 0
		{
			pAdap->more_data = FALSE;
		}
		else
		{
			pAdap->more_data = TRUE;
		}

		return (status);
	}

	if (a3->Type == WLAN_FTYPE_MGMT) /* This frame is a management frame or a control frame. */
	{
		pMngParam = (PRX_TO_MNG_PARAMETER)p3DspSrcBuf;
		pMngParam->rssi = (UINT8)pAdap->rssi;
		pMngParam->body_padding_len = rx_paring_result.body_padding_len;
#if 0		
		if (((a3->SubType == WLAN_FSTYPE_PROBEREQ) 
			&& (pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA))
			|| (a3->SubType == WLAN_FSTYPE_AUTHEN))
		{
			
			// Create a management task.
			Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
				DSP_TASK_EVENT_CALL_MANAGEMENT, DSP_TASK_PRI_NORMAL, p3DspSrcBuf, *poutcount);
			
		}
		else 
#endif
		if ((a3->SubType == WLAN_FSTYPE_ASSOCRESP) || (a3->SubType == WLAN_FSTYPE_REASSOCRESP))
		{
			//have to transact associate response packet in dispatch level
			//otherwise perhaps first 802.1x packet will be process earlier than association response packet in driver
			//this will cause that driver drops the first 8021x packet due to no link state
			//this patch for aolynk ap
			if(Mng_AssocRetrylimit(pAdap))
			{
				//request running in passive level
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
					DSP_TASK_EVENT_CALL_MANAGEMENT, DSP_TASK_PRI_NORMAL, p3DspSrcBuf, *poutcount);
			}
			else
			{
				//request running in dispatch level
				Mng_Receive(p3DspSrcBuf+sizeof(UINT32), *poutcount -sizeof(UINT32), pMngParam, pAdap);
			}	
		}
		else
		{
			
			Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
				DSP_TASK_EVENT_CALL_MANAGEMENT, DSP_TASK_PRI_NORMAL, p3DspSrcBuf, *poutcount);
		}
		//status = STATUS_WAKE_SYSTEM; // Look for a status which means a management module
		
		Rx_set_counts(pAdap, RX_STAT_GOOD_PAYLOAD_CNT, *poutcount);
		return (STATUS_DUMMY);
	}

	DBG_WLAN__RX(LEVEL_TRACE, "Rx type = %d, subtype = %d!\n", a3->Type, a3->SubType);
	
	return (STATUS_FAILURE);  // return status.
}

UINT32 Rx_check_packet(PDSP_ADAPTER_T pAdap, PUINT8 p3DspSrcBuf, UINT32 rescount)
{
	P80211_HEADER_A3_T a3;
	
	if (rescount <= sizeof(UINT32))
	{
		DBG_WLAN__RX(LEVEL_TRACE, "error data\n");
		return STATUS_FAILURE;
	}

    	if (pAdap->wlan_attr.chipID)
	{
		PUINT32	pheaddata;
		
		pheaddata = (PUINT32)p3DspSrcBuf;
		if ((RXBUF_GET_RXDATALEN(*pheaddata) != RXBUF_GET_RXSWAPLEN(*pheaddata)) 
			|| (RXBUF_GET_RXDATARATE(*pheaddata) != RXBUF_GET_RXSWAPRATE(*pheaddata)))
	    	{
		    DBG_WLAN__RX(LEVEL_TRACE, "***chipID = %d Rate & length check fail (%d:%d  %d:%d) %d\n", 
				pAdap->wlan_attr.chipID, RXBUF_GET_RXDATARATE(*pheaddata),
				RXBUF_GET_RXDATALEN(*pheaddata),
				RXBUF_GET_RXSWAPRATE(*pheaddata),
				RXBUF_GET_RXSWAPLEN(*pheaddata), rescount);
//		    DBG_PRINT_BUFF("Rx_process", p3DspSrcBuf, rescount);
		    return STATUS_FAILURE;	
	    	}
    	}

	a3 = (P80211_HEADER_A3_T)(p3DspSrcBuf + sizeof(UINT32));

	//in normal operation, driver should not have a CONTROL frame passed up from h/w
	if ((a3->Type == WLAN_FTYPE_CTL) || (a3->Type == WLAN_FTYPE_RESERVE))
	{
		DBG_WLAN__RX(LEVEL_TRACE, "Get other type(%d) frame\n", a3->Type);
		return STATUS_DUMMY;
	}

	if ((a3->Type == WLAN_FTYPE_DATA) && (a3->SubType == WLAN_FSTYPE_NULL))
	{
		DBG_WLAN__RX(LEVEL_TRACE, "Get a NULL data frame\n");
		return STATUS_DUMMY;
	}

	if(pAdap->link_ok == LINK_OK) //when connected AP
	{
#ifdef ROAMING_SUPPORT
		if(pAdap->reconnect_status != NEED_RECONNECT)
#endif		//#ifdef ROAMING_SUPPORT
		{
			BOOLEAN		match = FALSE;

			if(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
			{
				match = IS_MATCH_IN_ADDRESS(a3->a2,pAdap->wlan_attr.bssid);
			}
			else
			{
				match = IS_MATCH_IN_ADDRESS(a3->a3,pAdap->wlan_attr.bssid);
			}
		
			if(!match)//not from our AP
			{
//				DBG_WLAN__RX(LEVEL_TRACE, "Get an  frame from AP which is not the one we connected\n");
				return STATUS_DUMMY;
#if 0	
				if(!(a3->a1[0] & 0x01))//not multicast and not broadcast
				{
					DBG_WLAN__RX(LEVEL_TRACE, "Get an unicast frame from AP which is not the one we connected\n");
					Rx_set_counts(pAdap, RX_STAT_ERR_NWID_CNT, 1);
					return STATUS_FAILURE;
				}
				else// multicast or  broadcast
				{
					if(a3->Type == WLAN_FTYPE_MGMT) //a mng packet not from our AP
					{
						//DBG_WLAN__RX(LEVEL_TRACE, "Get an multicast or broadcast frame from AP which is not the one we connected\n");
						//return STATUS_FAILURE;
					}
				}
#endif
			}
			else		//from our AP, 
			{
				if((!IS_MATCH_IN_ADDRESS(a3->a1, pAdap->current_address))	//not for me
					&&((a3->a1[0] & 0x01) != 0x01))							//not multicast and not broadcast
				{
					DBG_WLAN__RX(LEVEL_TRACE, "Get an frame which is not for me from AP \n");
					return STATUS_FAILURE;
				}
			}
		}
	#ifdef ROAMING_SUPPORT
		else	//pAdap->reconnect_status == NEED_RECONNECT	//justin:	080407. this is in beacon lost status, and doing scan just want recv probresp and beacon.
		{
			if (a3->Type == WLAN_FTYPE_MGMT) 
			{
				if ((a3->SubType == WLAN_FSTYPE_PROBERESP) || (a3->SubType == WLAN_FSTYPE_BEACON))
				{
					;
				}
				else 
				{
					DBG_WLAN__RX(LEVEL_INFO, "WLAN_FTYPE_MGMT, subtype = %d\n", a3->SubType);
					return STATUS_FAILURE;
				}
			}
		}
	#endif
	}
	else
	{
		if((0 == sc_memory_cmp(a3->a1,pAdap->current_address,WLAN_ADDR_LEN) )
            &&(a3->Type == WLAN_FTYPE_DATA))
		{
//			DBG_WLAN__RX(LEVEL_TRACE, "WLAN_FTYPE_DATA, a1 = %x:%x:%x:%x:%x:%x\n", 
//				a3->a1[0], a3->a1[1], a3->a1[2], a3->a1[3], a3->a1[4], a3->a1[5]);
			return STATUS_ADAPTER_NOT_READY;
		}
	}	

	if (a3->Type == WLAN_FTYPE_DATA)
	{
		if ((pAdap->wlan_attr.hasjoined == JOIN_NOJOINED) 
			|| (pAdap->link_ok == LINK_FALSE))
		{
//			DBG_WLAN__RX(LEVEL_TRACE, " Rec Data, but not joined %d, link_active %d\n", pAdap->wlan_attr.hasjoined ,pAdap->link_is_active);
			return (STATUS_ADAPTER_NOT_READY);
		}
		
		return STATUS_SUCCESS | NEED_DEST_BUFFER_FLAG;
	}
		
	return STATUS_SUCCESS;		
}

/**************************************************************************
*   Rx_completion_routine
*
*   Descriptions:
*      This completion routine process the condition that a rx irp is responded.
*   Arguments:
*      pAdap		:IN, pointer to device object.
*      recvcount	:IN, rx buffer length.
*      pSrcBuf		:IN, a reserved point
*   Return Value:
*      STATUS_SUCCESS: return success.
*      STATUS_xxx: return unsuccessful.
*************************************************************************/
void Rx_completion_routine(PDSP_ADAPTER_T	pAdap, INT32 recvcount, PVOID pSrcBuf)
{
	UINT32			status;
	UINT32			tmpcount = 0;			// has processed count	(in 3dsp frame/packet)
	UINT32			outcount = 0;			// length of this MPDU in 3dsp format
	UINT32			realbyte = 0;			// the real bytes of the converted frame (802.3).
											// receive bytes in this packet(maybe more than one fragment within this packet)
	PUINT8			pDesBuf;
	PVOID 			pRecAreaBlock = NULL;
	
	if (Adap_Driver_isHalt(pAdap) || (pAdap->pfrag == NULL))
	{
		// This error usually occurs when the device is malfunctioning 
		// or it has been disconnected. Halt() cancels all outstanding Irps
		// Note: The OutStanding Rx Irp count must be decrimented because we got
		// here prior to the m_HaltFlag being set
		// TODO: add code to handle you case

		return ;//STATUS_ADAPTER_NOT_READY;
	}
	
	if (recvcount <= 0)
	{
		Rx_BuildBulkInTransfer(pAdap);
		return ;//STATUS_SUCCESS;
	}

	/* Do real rx function */
	
	/* There are maybe a lot of MPDUs in one transfer. */
	while(tmpcount < recvcount)			/* tempcount is the processed count num*/
	{
		BOOLEAN			buffer_flag;

		status = Rx_check_packet	(pAdap, pSrcBuf + tmpcount, recvcount - tmpcount);
		buffer_flag = (status & NEED_DEST_BUFFER_FLAG) == NEED_DEST_BUFFER_FLAG;
		status = status & (~NEED_DEST_BUFFER_FLAG);

		if (status == STATUS_SUCCESS)
		{
			if (buffer_flag)
			{
				/* Get a idle receive area block from the area free list of the receive area module.  */
				pDesBuf = (PUINT8)sc_skb_alloc(MAXRECDATALEN, &pRecAreaBlock);
				if (pDesBuf == NULL) /* If there is no idle block, the pointer to destination buffer  is set NULL. */
				{
					Rx_set_counts(pAdap, RX_STAT_DROPPED_PKT_CNT, 1);
					break;
				}
			}
			else
			{
				pDesBuf = NULL;
			}
			
			/* Process rx  */
			status = Rx_process(pAdap, pSrcBuf + tmpcount, pDesBuf, &outcount, &realbyte, recvcount - tmpcount);
			//if data frame, STATUS_SUCCESS is ok; if MNG frame, STATUS_DUMMY is OK;
			if (status == STATUS_SUCCESS) /* The 802.11 frame has been converted to 802.3 frame. */
			{
				if (Rx_data_filter(pAdap, pDesBuf) == STATUS_SUCCESS)
				{
					sc_skb_sbmt(pAdap->net_dev, pRecAreaBlock, realbyte);
					Rx_set_counts(pAdap, RX_STAT_GOOD_PAYLOAD_CNT, realbyte);			
					tmpcount += outcount;
					continue;
				}
			}
			
			if (buffer_flag)
				sc_skb_free(pRecAreaBlock);		
		}
		else
		{
			if (status != STATUS_FAILURE)
				break;
			else
				outcount = recvcount - tmpcount;
		}

		if (status == STATUS_FAILURE)
			Rx_set_counts(pAdap, RX_STAT_ERROR_CNT, 1);

		// Process the condition that there is a bad frame. If it happened, driver must not process the next 
		// frame in this buffer and just discard this frame.
		if (outcount > DSP_RX_OUT_MAX_SIZE)
			break;
		
		tmpcount += outcount;
	}
	
	//Resending the URB back to the	bus driver.
	Rx_BuildBulkInTransfer(pAdap);

	return ;
}


