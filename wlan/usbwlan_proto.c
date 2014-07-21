 /***********************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  
  * FILENAME		:usbwlan_proto.c         VERSION:1.2
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
static char* TDSP_FILE_INDICATOR="PROTO";
/*
#include <ndis.h>
#include "usbwlan_wlan.h"
#include "usbwlan_Sw.h"
#include "usbwlan_proto.h"
#include "usbwlan_dbg.h"
#include "usbwlan_rx.h"
*/
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
// Rotation functions on 32 bit values
#define ROL32( A, n ) ( ((A) << (n)) | ( ((A)>>(32-(n)))  & ( (1UL << (n)) - 1 ) ) )
#define ROR32( A, n ) ROL32( (A), 32-(n) )
/*--file local variables-----------------------------------------------*/
/*--file local function prototypes-------------------------------------*/
/*--functions ---------------------------------------------------------*/
	
/***********************************************************************************
 *  Function Name:	mic_getUInt32,mic_putUInt32,mic_clear,mic_setKey,mic_Michael,
 *		mic_appendByte,mic_append,mic_getMIC.
 *  Description:
 *		these functions used to calculate michael values.
 *  Arguments:
 *		pmic, pointer to MICHAEL_T structure. 
 *	Return  Value:
 *********************************************************************************/
UINT32 mic_getUInt32(UINT8 * p)
// Convert from Byte[] to UInt32 in a portable way
{
	UINT32 res = 0;
	UINT8 i;
	
	for (i=0; i<4; i++)
	{
//		res |= (*p++) << (8*i);
		res |= p[i] << (8*i);
	}
	return res;
}

VOID mic_putUInt32(UINT8 * p, UINT32 val )
// Convert from UInt32 to Byte[] in a portable way
{
	UINT8 i;
	
	for (i=0; i<4; i++)
	{
//		*p++ = (UINT8) (val & 0xff);
		p[i] = (UINT8) (val & 0xff);
	val >>= 8;
	}
}

VOID mic_clear(PMICHAEL_T pmic)
{
	// Reset the state to the empty message.
	pmic->L = pmic->K0;
	pmic->R = pmic->K1;
	pmic->nBytesInM = 0;
	pmic->M = 0;
}

VOID mic_setKey(PMICHAEL_T pmic, UINT8 * key )
{
	// Set the key
	pmic->K0 = mic_getUInt32(key);
	pmic->K1 = mic_getUInt32(key + 4);
	// and reset the message
	mic_clear(pmic);
}


VOID mic_appendByte(PMICHAEL_T pmic,UINT8 b )
{
	// Append the byte to our word-sized buffer
	pmic->M |= b << (8*pmic->nBytesInM);
	pmic->nBytesInM++;
	// Process the word if it is full.
	if( pmic->nBytesInM >= 4 )
	{
		pmic->L ^= pmic->M;
		pmic->R ^= ROL32( pmic->L, 17 );
		pmic->L += pmic->R;
		pmic->R ^= ((pmic->L & 0xff00ff00) >> 8) | ((pmic->L & 0x00ff00ff) << 8);
		pmic->L += pmic->R;
		pmic->R ^= ROL32( pmic->L, 3 );
		pmic->L += pmic->R;
		pmic->R ^= ROR32( pmic->L, 2 );
		pmic->L += pmic->R;
		// Clear the buffer
		pmic->M = 0;
		pmic->nBytesInM = 0;
	}
}

VOID mic_append( PMICHAEL_T pmic, UINT8 * src, INT32 nBytes )
{
	INT32 i;
	
	// This is simple
	for (i = 0; i < nBytes; i++)
	{
		mic_appendByte(pmic, src[i]);
	}
}

VOID mic_getMIC(PMICHAEL_T pmic, UINT8 * dst )
{
	// Append the minimum padding
	mic_appendByte(pmic, 0x5a);
	mic_appendByte(pmic, 0);
	mic_appendByte(pmic, 0);
	mic_appendByte(pmic, 0);
	mic_appendByte(pmic, 0);
	// and then zeroes until the length is a multiple of 4
	while (pmic->nBytesInM != 0)
	{
		mic_appendByte(pmic, 0);
	}
	// The appendByte function has already computed the result.
	mic_putUInt32(dst, pmic->L);
	mic_putUInt32(dst + 4, pmic->R);
	// Reset to the empty message.
	mic_clear(pmic);
}

/***********************************************************************************
 *  Function Name:	p80211_stt_findproto
 *  Description:
 *		Searches the 802.1h Selective Translation Table for a given protocol.
 *  Arguments:
 *		proto, protocl number (in host order) to search for.
 *		pfrag, pointer to structure FRAG_STRUCT_T
 *	Return  Value:
 *		1, true.
 *		0, false.
 *********************************************************************************/
INT32 p80211_stt_findproto(PFRAG_STRUCT_T pfrag, UINT16 proto)
{
	PSTT_TABLE_T pstt_table = &pfrag->F_SttTable;
	UINT8 i;
	
	/* Always return found for now.  This is the behavior used by the */
	/*  Zoom Win95 driver when 802.1h mode is selected */
	/* TODO: If necessary, add an actual search we'll probably
		 need this to match the CMAC's way of doing things.
		 Need to do some testing to confirm.	*/
		 
	for (i = 0; i < pstt_table->len; i++)
	{
		if (pstt_table->proto[i] == proto)
		{
			return 1;
		}
	}
	
	return 0;
}

/***********************************************************************************
 *  Function Name:	p80211_stt_addproto
 *  Description:
 *		add protocol number to protocol structure.
 *  Arguments:
 *		proto, protocl number (in host order) to search for.
 *		pstt_table, pointer to structure STT_TABLE_T
 *	Return  Value: VOID
 *********************************************************************************/
VOID p80211_stt_addproto(PFRAG_STRUCT_T pfrag, UINT16 proto)
{
	PSTT_TABLE_T pstt_table = &pfrag->F_SttTable;
	
	/* Always return found for now.  This is the behavior used by the */
	/*  Zoom Win95 driver when 802.1h mode is selected */
	/* TODO: If necessary, add an actual search we'll probably
		 need this to match the CMAC's way of doing things.
		 Need to do some testing to confirm.
	*/
	if (pstt_table->len >= MAXSTTLEN)
		return;
	pstt_table->proto[pstt_table->len] = proto;
	pstt_table->len++;
}

/***********************************************************************************
 *  Function Name:	Proto_get_addr
 *  Description:
 *		this function get saddr and daddr from 80211_HEADER_A3_T information.
 *  Arguments:
 *		p_a3: IN, pointer to head information structure 80211_HEADER_A3_T.
 *		saddr: OUT, return addr pointer 
 *		daddr: OUT, return addr pointer  
 *	Return  Value: VOID
 * *********************************************************************************/
VOID Proto_get_addr(P80211_HEADER_A3_T p_a3, PUINT8 *saddr, PUINT8 *daddr)
{
	if ((!p_a3->ToDS) && (!p_a3->FromDS))
	{
		*saddr = p_a3->a2;
		*daddr = p_a3->a1;
	}
	else if ((!p_a3->ToDS) && (p_a3->FromDS))
	{
		*saddr = p_a3->a3;
		*daddr = p_a3->a1;
	}
	else if ((p_a3->ToDS) && (!p_a3->FromDS))
	{
		*saddr = p_a3->a2;
		*daddr = p_a3->a3;
	}
	else
	{
		*saddr = p_a3->a2;
		*daddr = p_a3->a1;
	}
}

/***********************************************************************************
 *  Function Name:	Proto_802_11_to_ether
 *  Description:
 *		this function build up 802.3 header according to 802.11 header and
 *		copy data to destination buffer.
 *  Arguments:
 *		adaptor: IN,pointer to driver information structure.
 *		p_header_trans: IN, pointer to Rx temporary variables structure. HEADER_TRANSLATE_T 
 *	Return  Value:
 *		STATUS_SUCCESS, executed successfully.
 *		STATUS_FAILURE,fail.
* *********************************************************************************/
TDSP_STATUS Proto_802_11_to_ether(PDSP_ADAPTER_T adaptor, 
						PHEADER_TRANSLATE_T p_header_trans)
{
	UINT8			*daddr = NULL;
	UINT8			*saddr = NULL;
	UINT16			dixlen;		/* DIX data length */
	UINT16			buflen;		/* full frame length, including PAD */
	
	PFRAG_STRUCT_T 			pfrag 		= adaptor->pfrag;
	
	PETHER_HEADER_T 		p_src_ether_header;	
	PETHER_HEADER_T		p_dest_ether_header;
	PWLAN_LLC_T				pwlan_llc;
	PWLAN_SNAP_T			pwlan_snap;

	//get source and destination address.
	Proto_get_addr(&p_header_trans->p80211_header->a3, &saddr, &daddr);
	
	//set variable pointer.
	p_src_ether_header = (PETHER_HEADER_T)(p_header_trans->psource);

	//check encapsulation type.
	if (IS_MATCH_IN_ADDRESS(daddr, p_src_ether_header->daddr)  && IS_MATCH_IN_ADDRESS(saddr, p_src_ether_header->saddr)) 
	{
		// Test for an overlength frame 
		if (p_header_trans->p80211frmlen > (WLAN_HDR_A3_LEN+WLAN_CRC_LEN+WLAN_MAX_ETHFRM_LEN)) 
		{
			DBG_WLAN__RX(LEVEL_TRACE, "%s():%d fail\n", __FUNCTION__, __LINE__);
			return (STATUS_FAILURE);
		}
		
		/* now copy the data from the 80211 frame */
		if(p_header_trans->src_len > p_header_trans->max_des_len)
		{
			DBG_WLAN__RX(LEVEL_TRACE, "%s():%d fail\n", __FUNCTION__, __LINE__);
			return STATUS_FAILURE;
		}
		sc_memory_copy(p_header_trans->pdes, p_header_trans->psource, p_header_trans->src_len);
		*(p_header_trans->pdes_len) = p_header_trans->src_len;
		return STATUS_SUCCESS;
	}
	
	pwlan_llc = (PWLAN_LLC_T)(p_header_trans->psource);
	pwlan_snap = (PWLAN_SNAP_T)(p_header_trans->psource + sizeof(WLAN_LLC_T));
	
	p_dest_ether_header = (PETHER_HEADER_T)p_header_trans->pdes;
	p_header_trans->pdes += sizeof(ETHER_HEADER_T);
	
	 if ((pwlan_llc->dsap == 0xaa) && (pwlan_llc->ssap == 0xaa) && (pwlan_llc->ctl == 0x03))
	{
		WLAN_COPY_ADDRESS(p_dest_ether_header->daddr, daddr);
		WLAN_COPY_ADDRESS(p_dest_ether_header->saddr, saddr);

#if 1
		if ((0 == sc_memory_cmp(pwlan_snap->oui, pfrag->oui_rfc1042, WLAN_IEEE_OUI_LEN)) && 
			p80211_stt_findproto(pfrag, pwlan_snap->type)) 
		{
			/* it's a SNAP + RFC1042 frame && protocol is in STT */
			/* build 802.3 + RFC1042 */
		
			/* Test for an overlength frame */
			if ( p_header_trans->src_len > WLAN_MAX_ETHFRM_LEN - WLAN_ETHHDR_LEN ) 
			{
				/* A bogus length ethfrm has been sent. */
				/* Is someone trying an oflow attack? */
				DBG_WLAN__RX(LEVEL_TRACE, "%s():%d fail\n", __FUNCTION__, __LINE__);
				return STATUS_FAILURE;
			}

			buflen = wlan_max( p_header_trans->src_len + sizeof(ETHER_HEADER_T), WLAN_MIN_ETHFRM_LEN);
			/* set up the 802.3 header */	
			p_dest_ether_header->type = UshortByteSwap(p_header_trans->src_len);
			/* now copy the data from the 80211 frame */
			sc_memory_copy(p_header_trans->pdes, p_header_trans->psource, p_header_trans->src_len);
			*(p_header_trans->pdes_len) = (UINT16)buflen;
			return STATUS_SUCCESS;
		}
#endif	
		
		if ((pwlan_snap->type == pfrag->type_8021x) 
			||(0 == sc_memory_cmp(pwlan_snap->oui, pfrag->oui_rfc1042, WLAN_IEEE_OUI_LEN))
			||(((0 == sc_memory_cmp(pwlan_snap->oui, pfrag->oui_8021h, WLAN_IEEE_OUI_LEN)) && 
				p80211_stt_findproto(pfrag, pwlan_snap->type))))
		{
			/* it's an 802.1h frame || (an RFC1042 && protocol is not in STT) */
			/* build a DIXII + RFC894 */

			dixlen = p_header_trans->src_len - sizeof(WLAN_LLC_T) - sizeof(WLAN_SNAP_T);

			/* Test for an overlength frame */
			if ( dixlen + WLAN_ETHHDR_LEN > WLAN_MAX_ETHFRM_LEN) 
			{
				/* A bogus length ethfrm has been sent. */
				/* Is someone trying an oflow attack? */
				DBG_WLAN__RX(LEVEL_TRACE, "%s():%d fail\n", __FUNCTION__, __LINE__);
				return STATUS_FAILURE;
			}

			buflen = wlan_max( dixlen + sizeof(ETHER_HEADER_T), WLAN_MIN_ETHFRM_LEN);
			/* set up the DIXII header */	
			p_dest_ether_header->type = pwlan_snap->type;
			/* now copy the data from the 80211 frame */
			sc_memory_copy(p_header_trans->pdes, 
					p_header_trans->psource + sizeof(WLAN_LLC_T) + sizeof(WLAN_SNAP_T), 
					dixlen);
			*(p_header_trans->pdes_len) = (UINT16)buflen;

			return STATUS_SUCCESS;
		}
	 }
	 

	/* any NON-ENCAP */
	/* it's a generic 80211+LLC or IPX 'Raw 802.3' */
	/*  build an 802.3 frame */
	/* allocate space and setup hostbuf */

	/* Test for an overlength frame */
	if ((p_header_trans->src_len + WLAN_ETHHDR_LEN) > WLAN_MAX_ETHFRM_LEN) 
	{
		/* A bogus length ethfrm has been sent. */
		/* Is someone trying an oflow attack? */
		DBG_WLAN__RX(LEVEL_TRACE, "%s():%d fail\n", __FUNCTION__, __LINE__);
		return STATUS_FAILURE;
	}

	buflen = wlan_max( p_header_trans->src_len + sizeof(ETHER_HEADER_T), WLAN_MIN_ETHFRM_LEN);

	/* set up the 802.3 header */	
	p_dest_ether_header->type = UshortByteSwap((UINT16)p_header_trans->src_len);
	WLAN_COPY_ADDRESS(p_dest_ether_header->daddr, daddr);
	WLAN_COPY_ADDRESS(p_dest_ether_header->saddr, saddr);

	/* now copy the data from the 80211 frame */
	sc_memory_copy( p_header_trans->pdes, p_header_trans->psource, p_header_trans->src_len);
	*(p_header_trans->pdes_len) = (UINT16)buflen;
	
#if 0	//justin: 0711, check if our body(80211) is integrated
	if(*p_header_trans->pdes_len >= 230)//justin: 0720.    2c_wlan_wep
	{
//		MOD_WLAN__RX(LEVEL_INFO, "*******proto processed source********\n");
//		Adap_PrintBuffer(p_header_trans->psource,p_header_trans->src_len);
		MOD_WLAN__RX(LEVEL_INFO, "*******proto processed********\n");
		Adap_PrintBuffer(p_header_trans->pdes,*p_header_trans->pdes_len);
		MOD_WLAN__RX(LEVEL_INFO, "***********************\n");
	}	
#endif
	return STATUS_SUCCESS;
}

