 /***********************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  
  * FILENAME		:usbwlan_rx.h         VERSION:1.2
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
#ifndef __DSP_WLANUSB_RX_H
#define __DSP_WLANUSB_RX_H
#include "precomp.h"

//#define  DSP_USB_TX_DATA_MLME_PACKET        0
//#define  DSP_USB_TX_NOTYFY_PACKET           1

//#define DSP_USB_STATUS_BAD_FRM				((NTSTATUS)0xC0139999L)

/* 802.11 frame (in bytes)*/


#define MAXFILTERLEN     			(32)
#define MAXSTTLEN        			(10)
/* 3dsp frame (in bytes)*/
#define DSP_USB_RX_FRM_RSSI_AND_RSVD_LEN		4	//rssi + reserved
#define DSP_USB_RX_FRM_OFFSET_FIELD_HI_LEN		4	// offsetHi
#define DSP_USB_RX_FRM_OFFSET_FIELD_LO_LEN		4	// offsetLo
#define DSP_USB_RX_FRM_OFFSET_FIELD_LEN			8	// offsetHi + offsetLo
#define DSP_USB_RX_FRM_ADDR4_AND_PADDING_LEN	8	// address4 + padding data
#define DSP_USB_RX_FRM_ADDR4_PAD_LEN			2	//padding data

#define DOT11_RX_FRM_WEP_IV_LEN					4	//wep
#define DOT11_RX_FRM_WEP_ICV_LEN				4
#define DOT11_RX_MSDU_TKIP_MIC_LEN				8	//tkip
#define DOT11_RX_FRM_TKIP_EIV_LEN				4
#define DOT11_RX_FRM_CCMP_MIC_LEN				8	//ccmp        
#define DOT11_RX_FRM_CCMP_HDR_LEN				8


#define DSP_RX_BUF_SIZE       0x1000
#define DSP_RX_IND_BUF_SIZE   2048
#define DSP_RX_OUT_MAX_SIZE   1600

#define MAXPLCPLEN								3000
#define DSP_RX_CHECK_COUNT					5

#define MAXFRAGCLASS							(12) 
#define MAXFRAGTIMEOUT							(2)  // 500ms ~ 1000ms 

#define MAXALLRATES								(0x20)

#define IsOfdmRate(rate)						(((rate>>3)&0x01) == 1 ? 1 : 0)

/* Rx statistic count */
#define 		RX_STAT_GOOD_CNT				0		//successed to submited packed number
#define 		RX_STAT_GOOD_PAYLOAD_CNT	1		//successed to submited payload length
#define 		RX_STAT_ERROR_CNT				2		//received, but not submited packed number
#define 		RX_STAT_ERR_NWID_CNT			3		//BSSID is error, not submit
#define 		RX_STAT_ERR_DECODE_CNT		4		//decode failure 
#define 		RX_STAT_ERR_DEFRAG_CNT		5		//defrag failure
#define 		RX_STAT_DROPPED_PKT_CNT		6		//can not alloc skb, so fail
#define 		RX_STAT_MC_PKT_CNT			7		//successed to received muticast packed number
#define 		RX_STAT_CNT					8		// total of statistic type

//get tkip index
#define GetTkipRxIndex(ptkipindex)				((*((PUINT8)ptkipindex) & 0xc0) >> 6)                      

/*--constants and types------------------------------------------------*/

#pragma pack(1)

typedef struct _DEFRAG_ELEMENT
{
	UINT32	r_SeqNum		:12;		// Seq number of this instance.
	UINT32	r_CurrentFrag	:4;		// Appropriate fragment number.
	UINT32	r_CurrentPoint	:12;		// Current point which is the next pointer to copy.
	UINT32	r_BusyFlag		:1;		// If this instance is used,this flag is set 1,otherwise 0;
	UINT32	reserved		:2;		//not used
	UINT32	r_TimerValid	:1;		// If the timer of this instance is used, this flag is set 1,otherwise 0;
	UINT16	r_TimerCounter;			// Timer counter value
	UINT8	r_SaddrCached[WLAN_ETHADDR_LEN];// Store source mac address of msdu
	UINT8	r_RecCached[MAXRECDATALEN];		// Receive buffer address
}DEFRAG_ELEMENT_T,*PDEFRAG_ELEMENT_T;

typedef struct _MICHAEL
{
	UINT32  K0, K1;         // Key 
	UINT32  L, R;           // Current state
	UINT32  M;              // Message accumulator (single word)
	INT32    nBytesInM;      // # bytes in M
}MICHAEL_T,*PMICHAEL_T;

typedef struct _FILTER_OPTION
{
	UINT16 seqnum;
	UINT8  saddr[WLAN_ETHADDR_LEN];
}FILTER_OPTION_T,*PFILTER_OPTION_T;

typedef struct _STT_TABLE
{
	UINT16 len;
	UINT16 proto[MAXSTTLEN];
}STT_TABLE_T,*PSTT_TABLE_T;

/* This structure defines the vars assicated with the condition that
there are one frame across two buffers. Driver needs copy the two frag
memory into one memory */
typedef struct _DSP_BUFFER_BOUNDARY
{
	UINT8				Buffer[DSP_RX_IND_BUF_SIZE];	// Data buffer  
	UINT16              frame_size; // The frame size
	UINT16              copied_bytes; // first copied bytes
	BOOLEAN             use_flag; // if this frame is across two buffers, this flag is set TRUE.
} DSP_BUFFER_BOUNDARY_T, *PDSP_BUFFER_BOUNDARY_T;

// fdeng
#pragma pack()




typedef struct _FRAG_STRUCT
{
	MICHAEL_T			mic;	//mic structure.

	//	static data array...
	UINT8				oui_rfc1042[3];
	UINT8				oui_8021h[3];
	UINT16				type_8021x;
	UINT8				msdu_pri_cp[4];
	UINT8				msdu_pri_cfp[4];

	// member variables related to fragment and defragment.
	FILTER_OPTION_T		FilterOption[MAXFILTERLEN];	// Filter table pointer address
	STT_TABLE_T			F_SttTable;					// STT table
	UINT8				FilterPoint;					// Current position in filter table

	UINT8				r_stoped;				//if rx_stop, set to 1;
		
	//defragment structure and variables...
	UINT16				r_RecFragMap;		//bit0 -> r_RecFrag[0]
											//bit1 -> r_RecFrag[1]		...
											
	DEFRAG_ELEMENT_T	r_RecFrag[MAXFRAGCLASS];//receive fragment element.

	UINT8				BulkInBuffer[MAXRECDATALEN];
	UINT32				i_Stat_Counts[RX_STAT_CNT];
	// Ack duration table
//	UINT16				ack_duration_table[MAXALLRATES];	// remark by Justin

	// Processes the condition that one frame acrossing two buffers.
#ifdef RX_BUFFER_BOUNDARY_PROCESS
	DSP_BUFFER_BOUNDARY_T buffer_boundary;
#endif

#ifdef RX_RESUBMIT_IRP_B4_PROCESSING						//Justin: 0601    
	UINT8	rx_recv_buffer[DSP_RX_BUF_SIZE];
#endif

}FRAG_STRUCT_T,*PFRAG_STRUCT_T;


// fdeng
#pragma pack(1)

typedef struct _80211_HEADER_A3
{
/*************  Frame Control defined below *********************/
	UINT32	Protocol			:2;
	UINT32	Type			:2;
	UINT32	SubType		:4;
	UINT32	ToDS			:1;
	UINT32	FromDS			:1;
	UINT32	MoreFrag		:1;
	UINT32	Retry			:1;
	UINT32	PwrMgmt		:1;
	UINT32	MoreData		:1;
	UINT32	ProtectedFrame	:1;
	UINT32	Order			:1;	

/*************  Duration ID defined below *********************/
	UINT32	DurationID		:16;	

	UINT8	a1[WLAN_ADDR_LEN];
	UINT8	a2[WLAN_ADDR_LEN];
	UINT8	a3[WLAN_ADDR_LEN];
	UINT16	seq;
}T80211_HEADER_A3_T,*P80211_HEADER_A3_T;

typedef struct _80211_HEADER_A4
{
/*************  Frame Control defined below *********************/
	UINT32	Protocol			:2;
	UINT32	Type			:2;
	UINT32	SubType		:4;
	UINT32	ToDS			:1;
	UINT32	FromDS			:1;
	UINT32	MoreFrag		:1;
	UINT32	Retry			:1;
	UINT32	PwrMgmt		:1;
	UINT32	MoreData		:1;
	UINT32	ProtectedFrame	:1;
	UINT32	Order			:1;	

/*************  Duration ID defined below *********************/
	UINT32	DurationID		:16;	
	UINT8	a1[WLAN_ADDR_LEN];
	UINT8	a2[WLAN_ADDR_LEN];
	UINT8	a3[WLAN_ADDR_LEN];
	UINT16	seq;
	UINT8	a4[WLAN_ADDR_LEN];
}T80211_HEADER_A4_T,*P80211_HEADER_A4_T;

typedef union _80211_HEADER
{
	T80211_HEADER_A3_T		a3;
	T80211_HEADER_A4_T		a4;
} T80211_HEADER_T,*P80211_HEADER_T;

// holds all parsing result during process rx
typedef struct _RX_PARSING_RESULT
{
	UINT32 in_air_len		:12;		// len of 802.11 frame, received from air
	UINT32 speed			:4;
	UINT32 body_padding_len	:3;		//dsp pad data to make air_dody DWORD ALIGN
	UINT32 is_wep			:1;		// 0 or 1
	UINT32 a4_len			:4;		// address 4 len if present(3dsp spec.) 
	UINT32 offset_len		:4;		// offsetHi and offsetLo len if present(3dsp spec.);    0 if not present
	UINT32 iv_or_eiv_len		:4;		// IV/KeyID and ExtendedIV len if present(3dsp spec.);  0 if is_wep=0
	
	UINT16 body_offset_fc; 		// offset of (body addr - fc addr)
	UINT16 body_len;	 
}RX_PARSING_RESULT_T, *PRX_PARSING_RESULT_T;


#pragma pack()


TDSP_STATUS Rx_init(PDSP_ADAPTER_T adaptor);
VOID  Rx_release(PDSP_ADAPTER_T adaptor);
VOID Rx_stop(PDSP_ADAPTER_T pAdap);
VOID Rx_restart(PDSP_ADAPTER_T pAdap);
VOID Rx_defragment_timeout(PDSP_ADAPTER_T pAdap);
VOID Rx_get_counts(PDSP_ADAPTER_T pAdap, 
						PUINT32		p_good_cnt,
						PUINT32		p_good_payload_cnt,
						PUINT32		p_error_cnt,
						PUINT32		p_err_nwid_cnt ,
						PUINT32		p_err_decode_cnt, 
						PUINT32		p_err_defrag_cnt,
						PUINT32		p_dropped_pkt_cnt,
						PUINT32		p_mc_pkt_cnt);

VOID recfrag_reset_all(PFRAG_STRUCT_T pfrag);
TDSP_STATUS Frag_ClearInfo(PDSP_ADAPTER_T adaptor);	//initialize fragment module


#endif //file end
