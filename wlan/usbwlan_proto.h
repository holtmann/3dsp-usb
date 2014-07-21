#ifndef __DSP_PROTO_H
#define __DSP_PROTO_H
/*******************************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:		DSP_proto.h	CURRENT VERSION: 1.00.00
* PURPOSE:		Define macro, structure and functions related to protocol module
* AUTHORS:		
* NOTES:		This is DSP_proto.c header file.
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.
********************************************************************************/
/*--variables---------------------------------------------------------*/
/*--macros------------------------------------------------------------*/
#include "precomp.h"

#define ENCAP_TYPE_SIMPLE		(0)
#define ENCAP_TYPE_WITH_LLC	(1)
#define MAXSEQNUMBER     		(4096)
#define MAXSENDDATALEN   		(2200)
#define MAXFRAGMENTCOUNT 		(10)
#define CombineSeq(x, y)		(UINT16)((x << 4) | y)
#define ReplaceFrag(x, y)		(UINT16)((x & 0xfff0) | (y & 0x000f))
#define UshortByteSwap(x)	(UINT16)((((UINT16)(x) & (UINT16)0x00ff) << 8) | (((UINT16)(x) & (UINT16)0xff00) >> 8)) 

#define WLAN_GET_FC_PVER(n)	(((UINT16)(n)) & (BIT0 | BIT1))
#define WLAN_GET_FC_FTYPE(n)	((((UINT16)(n)) & (BIT2 | BIT3)) >> 2)
#define WLAN_GET_FC_FSTYPE(n)	((((UINT16)(n)) & (BIT4|BIT5|BIT6|BIT7)) >> 4)
#define WLAN_GET_FC_TODS(n) 	((((UINT16)(n)) & (BIT8)) >> 8)
#define WLAN_GET_FC_FROMDS(n)	((((UINT16)(n)) & (BIT9)) >> 9)
#define WLAN_GET_FC_MOREFRAG(n) ((((UINT16)(n)) & (BIT10)) >> 10)
#define WLAN_GET_FC_RETRY(n)	((((UINT16)(n)) & (BIT11)) >> 11)
#define WLAN_GET_FC_PWRMGT(n)	((((UINT16)(n)) & (BIT12)) >> 12)
#define WLAN_GET_FC_MOREDATA(n) ((((UINT16)(n)) & (BIT13)) >> 13)
#define WLAN_GET_FC_ISWEP(n)	((((UINT16)(n)) & (BIT14)) >> 14)
#define WLAN_GET_FC_ORDER(n)	((((UINT16)(n)) & (BIT15)) >> 15)

#define WLAN_SET_FC_PVER(n)	((UINT16)(n))
#define WLAN_SET_FC_FTYPE(n)	(((UINT16)(n)) << 2)
#define WLAN_SET_FC_FSTYPE(n)	(((UINT16)(n)) << 4)
#define WLAN_SET_FC_TODS(n) 	(((UINT16)(n)) << 8)
#define WLAN_SET_FC_FROMDS(n)	(((UINT16)(n)) << 9)
#define WLAN_SET_FC_MOREFRAG(n) (((UINT16)(n)) << 10)
#define WLAN_SET_FC_RETRY(n)	(((UINT16)(n)) << 11)
#define WLAN_SET_FC_PWRMGT(n)	(((UINT16)(n)) << 12)
#define WLAN_SET_FC_MOREDATA(n) (((UINT16)(n)) << 13)
#define WLAN_SET_FC_ISWEP(n)	(((UINT16)(n)) << 14)
#define WLAN_SET_FC_ORDER(n)	(((UINT16)(n)) << 15)

/*--constants and types-----------------------------------------------*/
#pragma pack(1)
//-------------------------------------
//protocol header structure
/* local ether header type */

/* local llc header type */
typedef struct _WLAN_LLC
{
	UINT8	dsap;
	UINT8	ssap;
	UINT8	ctl;
}WLAN_LLC_T,*PWLAN_LLC_T;

/* local snap header type */
typedef struct _WLAN_SNAP
{
	UINT8	oui[WLAN_IEEE_OUI_LEN];
	UINT16	type;
}WLAN_SNAP_T,*PWLAN_SNAP_T;

//---------------------------------
//descriptor structure
typedef struct _TX_FORMAT_DW0
{
	UINT32      queue	: 3;
	UINT32      pklen	: 13;
	UINT32      i		: 1;
	UINT32      c		: 1;
	UINT32      l		: 1;
	UINT32      k		: 2;
	UINT32      t		: 4;
	UINT32      f		: 1;
	UINT32      m		: 1;
	UINT32      b		: 1;
	UINT32      e		: 1;
	UINT32      s		: 1;
	UINT32      w		: 1;
	UINT32      erp	: 1;
} TX_FORMAT_DW0_T, *PTX_FORMAT_DW0_T;

typedef struct _TX_FORMAT_DW1
{
	UINT32     frglen		: 16;
	UINT32     reqcount	: 16;
} TX_FORMAT_DW1_T, *PTX_FORMAT_DW1_T;

typedef struct _TX_FORMAT_DW2
{
	UINT32     endfrglen		: 16;
	UINT32     endfrgreqcount	: 16;
} TX_FORMAT_DW2_T, *PTX_FORMAT_DW2_T;

typedef struct _TX_FORMAT_DW3
{
	UINT32     txpower	: 7;
	UINT32     tk			: 5;
	UINT32     rtsrate		: 4;
	UINT32     frgnumber	: 4;
	UINT32     reserved	: 5;
	UINT32     ll			: 1;
	UINT32     ee			: 1;
	UINT32     ctsen		: 1;
	UINT32     beacon		: 1;
	UINT32     atim		: 1;
	UINT32     mr			: 1;
	UINT32     cs			: 1;
} TX_FORMAT_DW3_T, *PTX_FORMAT_DW3_T;

typedef struct _TX_FORMAT_DW4
{
	UINT32	  txpn;
}TX_FORMAT_DW4_T, *PTX_FORMAT_DW4_T;

typedef struct _TX_FORMAT_DW5
{
	UINT32	  txpn		: 16;
	UINT32     cfpen		: 8;
	UINT32	  reserved	: 8;
}TX_FORMAT_DW5_T, *PTX_FORMAT_DW5_T;

typedef struct _TX_FORMAT_DW6
{
	UINT32	  reserved;
}TX_FORMAT_DW6_T,*PTX_FORMAT_DW6_T;


typedef struct _TX_FORMAT
{
	TX_FORMAT_DW0_T dw0;
	TX_FORMAT_DW1_T dw1;
	TX_FORMAT_DW2_T dw2;
	TX_FORMAT_DW3_T dw3;
	TX_FORMAT_DW4_T dw4;
	TX_FORMAT_DW5_T dw5;
	TX_FORMAT_DW6_T dw6;
} TX_FORMAT_T,*PTX_FORMAT_T;
//-----------------------
//use main control structure in the future. drop thes two struct definition
//header and tail was dealt by main control module.
typedef struct _RX_FORMAT_HEAD
{
	UINT32 datalen	: 12;
	UINT32 tbtt_num	: 8;
	UINT32 data_rate	: 4;
	UINT32 rssi		: 7;
	UINT32 first_bit	: 1;
} RX_FORMAT_HEAD_T,*PRX_FORMAT_HEAD_T;

typedef struct _RX_FORMAT_TAIL
{
	UINT32 timestamp	: 16;
	UINT32 xfer_status	: 16;
} RX_FORMAT_TAIL_T, *PRX_FORMAT_TAIL_T;
//-----------------------
//---------------------------------

// add by Justin
//Justin: to pass parameters to mng, we fill this structure value to the first 2 bytes of dsp received data(no use to us).
 typedef struct _RX_TO_MNG_PARAMETER
{
	UINT8 body_padding_len;
	UINT8 rssi;
}RX_TO_MNG_PARAMETER, *PRX_TO_MNG_PARAMETER;



// fdeng
#pragma pack()

//module local structure
typedef struct _HEADER_TRANSLATE {
	// 
	PUINT8	psource;			// Pointer to input data buffer.
	UINT16	src_len;			// source length.
	PUINT8	pdes;			// pointer to output data buffer.
	PUINT16	pdes_len;			// destination buffer length.
	PUINT8	p80211_header_buf;	// buffer to store Tx 802.11 head information 
	UINT8	encap_type;		// 802.11 encapsulate type.
	UINT16	max_des_len;		// max destination length.
	// other pointer temporary variables.
//	PETHER_HEADER_T	pether_header;	// point to pdes
	P80211_HEADER_T p80211_header;
//	PWLAN_LLC_T		pwlan_llc;
//	PWLAN_SNAP_T	pwlan_snap;
	UINT32			p80211frmlen; //used for received frame length.
}HEADER_TRANSLATE_T,*PHEADER_TRANSLATE_T;




// fdeng mask this !!!
//
//#pragma pack()


/*--function prototypes-----------------------------------------------*/
TDSP_STATUS proto_ehter_to_802_11(PDSP_ADAPTER_T adaptor);	//build up 802.11 header.
TDSP_STATUS Proto_802_11_to_ether(PDSP_ADAPTER_T adaptor, PHEADER_TRANSLATE_T p_header_trans);
INT32 p80211_stt_findproto(PFRAG_STRUCT_T pfrag, UINT16 proto);
VOID p80211_stt_addproto(PFRAG_STRUCT_T pfrag, UINT16 proto);
VOID Proto_get_addr(P80211_HEADER_A3_T p_a3, PUINT8 *saddr, PUINT8 *daddr);
//-------------------------------------------
// mic functions...
UINT32 mic_getUInt32( UINT8 * p );
VOID mic_putUInt32( UINT8 * p, UINT32 val );
VOID mic_clear(PMICHAEL_T pmic);
VOID mic_setKey(PMICHAEL_T pmic, UINT8 * key );
VOID mic_appendByte( PMICHAEL_T pmic,UINT8 b );
VOID mic_append( PMICHAEL_T pmic,UINT8 * src, INT32 nBytes );
VOID mic_getMIC( PMICHAEL_T pmic,UINT8 * dst );
//-------------------------------------------

#endif //file end

