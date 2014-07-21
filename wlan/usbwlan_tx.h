#ifndef __DSP_WLANUSB_TX_H
#define __DSP_WLANUSB_TX_H
/*******************************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:		DSP_frag.h	CURRENT VERSION: 1.00.00
* PURPOSE:		Define macro, structure and functions related to fragment module
* AUTHORS:		
* NOTES:		This is DSP_frag.c header file.
* DECLARATION:  This document contains confidential proprietary information that
*			   is solely for authorized personnel. It is not to be disclosed to
*			   any unauthorized person without prior written consent of 3DSP
*			   Corporation.
********************************************************************************/

/*--constants and types------------------------------------------------*/
#include "precomp.h"
#define DSP_TX_CHECK_COUNT	5


// fdeng mask this for debug wlan in Android! 
//
//#pragma pack(1)
//




//
// This structure contains the information about tx queue
// 
//

#define MSDU_ALLOC_NUM		64
#define MMPDU_ALLOC_NUM		12


#define MSDU_ALLOC_SIZE		1600
// TODO: to be adjusted
#define MMPDU_ALLOC_SIZE	320

#define	 DSP_RETRY_LIST_NUM	 (8)	 
#define  MSDU_RETRY_MAX      (3)
#define  TX_SKB_BUF_NUM      (3)
#if 0
#define   MNG_QUEUE_TYPE_NOTIFY		 (0)
#define   MNG_QUEUE_TYPE_PROBERSP		 (1)
#define   MNG_QUEUE_TYPE_NORMAL		   (2)
#define   MNG_QUEUE_TYPE_BEACON		   (3)
#define   MNG_QUEUE_TYPE_ATIM			 (4)
#define   MNG_QUEUE_TYPE_PSPOLL		(5)	//Jakio2006.11.28: add to support ps-poll frame.
#define	 MNG_QUEUE_TYPE_NULLFRM		(6)	//Jakio2006.11.29: add to support null data frame.
#endif

#define PAD4(num) (((((UINT32) (num))%4)==0)?0:(4-(((UINT32) (num))%4)))


typedef UINT8 TX_MNG_FRM_PRIORITY;


#define   TX_MNG_FRM_PRIORITY_HIGH			(1)
#define   TX_MNG_FRM_PRIORITY_NORMAL		  (2)

typedef enum
{
	TX_MSDU_DATA_POOL,
	TX_MSDU_MNG_POOL
}TX_MSDU_TYPE;

typedef enum
{
	TX_MNG_TYPE_NOTIFY,		 
	TX_MNG_TYPE_PROBERSP,
	TX_MNG_TYPE_NORMAL,
	TX_MNG_TYPE_BEACON,		   
	TX_MNG_TYPE_ATIM,			
	TX_MNG_TYPE_PSPOLL,	
	TX_MNG_TYPE_PROBEREQ,
	TX_MNG_TYPE_NULLFRM,
}TX_MNG_FRM_TYPE;


typedef enum{ 

	TX_MNG_FRM_HIGH,
	TX_MNG_FRM_NORMAL,
	TX_DATA_FRM,
	TX_RETRY_FRM,
	TX_FW_FRM,
	TX_BEACON_FRM,
}TX_FRM_TYPE;


// fdeng
#pragma pack(1)

typedef struct snap_hdr_s {
	UINT8  dsap;
	UINT8  ssap;
	UINT8  control;
	UINT8  oui[DOT11_OUI_LEN];
	UINT16 type;
}snap_hdr_t,*psnap_hdr_t;

// fdeng
#pragma pack()



//total length must be DSP_TX_HEAD_BUF_SIZE.
typedef struct _DATA_HEAD_FORMAT
{
	UINT8		fifo_num;				//in fact, fifo num save packet len/4
	UINT8		fragcount;				//indicate fragcount num,in fact only data packet & size <1024 frag not equal 1
	//UINT16		total_len;				//note: [2]   [3]:ind if this is beacon
	//UINT8			reserved;
	UINT8		pad;
	UINT8		   dword_len;				//save dword length of packet head
	//UINT8		reserved1;
	//UINT8		pad;					//pad
	UINT16		total_len;				
	UINT8		sub_type;			//indicate packet >1024 or not for urb
	//UINT8		pad;
} DATA_HEAD_FORMAT_T, *PDATA_HEAD_FORMAT_T;

typedef struct _TX_MSDU_POOL 
{

	DSP_LIST_ENTRY_T	msdu_free_list;		//link of tx msdu buffer which has not been alloc
	UINT8			    pool_type;   //data frame pool or mng frame pool
	UINT32				node_free;
	UINT32				node_used;
	UINT32				node_num;
	UINT32				head_size;
	UINT32				payload_size;
	UINT8*			    node_buffer;	   //pointer to memory alloced for the node 
	UINT8*				head_buff;		 //pointer to memory alloced for the head buffer
	UINT8*				payload_buff;	  //pointer to memory alloced for the payload buffer 
	TDSP_SPINLOCK       pool_lock;
} TX_MSDU_POOL, *PTX_MSDU_POOL;


typedef struct _TX_MSDU_DATA
{
	DSP_LIST_ENTRY_T link;
	UINT32		    msg_type;	
	UINT8*			head_buff;	// for Tx - head packet or notify packet
	UINT8*			payload_buff;		// for Tx - used as the transmit buffer
	UINT8			priority;				/* priority of the msdu			*/
	UINT8			service_class;			/* order of the msdu			*/
	// TODO: to be renamed... actually is BSSID
	UINT8			src_addr[6];			/* src addr of the msdu			*/
	UINT8			dest_addr[6];			/* dest addr of the msdu		 */
	BOOLEAN		    is_multicast;
	// TODO:Jackie
	//tdsp_timestamp	msdu_time_stamp;
	UINT32		    payload_len;			   /* length of payload			*/ 
	UINT8		    head_len;			   /* length of the head		   */
	TX_FRM_TYPE		frm_type;
	  //Justin: 0425 wait tx dma finished
	UINT8			DspSendOkFlag;		  //flag for the head and body of the frm has been sent

	BOOLEAN		    Tx_head_irp_in_working;		//TRUE if tx head irp no completion routine return
	BOOLEAN		    Tx_data_irp_in_working;		//TRUE if tx data irp no completion routine return
	UINT8 			txRate;
	UINT16          seqnum;
	BOOLEAN         is_tx_1x_packet;
    UINT8           retry_count;               //retry counter
}TX_MSDU_DATA,*PTX_MSDU_DATA;

typedef struct _TX_MSDU_CONTEXT
{
	DSP_LIST_ENTRY_T link;
	TX_FRM_TYPE	  frm_type;
}TX_MSDU_CONTEXT,* PTX_MSDU_CONTEXT;

typedef struct _TX_SKB_DATA
{
    DSP_LIST_ENTRY_T link;
    UINT8 skb_buffer[MAX_ETHER_LEN];
}TX_SKB_DATA,*PTX_SKB_DATA;

typedef struct _TX_CONTEXT
{

	TX_MSDU_POOL		tx_msdu_pool;   //msdu pool for data ,fw msdu
	TX_MSDU_POOL		tx_mmpdu_pool;   //msdu pool for mng msdu
	BOOLEAN			    tx_busy_sending;     //if prev sent frame returned 
	BOOLEAN             tx_stop;
	DSP_LIST_ENTRY_T	data_frm_que;
	DSP_LIST_ENTRY_T	mng_frm_high_que;
	//  normal list
	DSP_LIST_ENTRY_T	mng_frm_normal_que;
#ifdef NEW_RETRY_LIMIT
	DSP_LIST_ENTRY_T	retry_frm_que;
	UINT32              tx_retry_valid;	 //indicate tx retry queue begin work
	UINT32              tx_retry_index;	//indicate get the item retry block with index for retry
#endif
	TDSP_SPINLOCK       tx_list_lock; 

	WLAN_TIMER          tx_watching_timer;		//timer for check send head and body has been complete
	PTX_MSDU_DATA       sending_msdu;		   //current msdu being tranmiting
	// rx error static count
	UINT32 tx_good_cnt;
	UINT32 tx_good_payload_cnt;
	UINT32 tx_error_cnt;

	TDSP_TASKLET		tx_tasklet;

	PVOID			    pAdap;		//pointer to the adapt

	BOOLEAN			    is_tx_1x_packet; //current sending packet is a 1x data    
    BOOLEAN             int_before_urb; //prcess retry quene later 
    BOOLEAN             is_time_out;
#if 1
    TX_MSDU_POOL        tx_buffer_pool;
#endif
    DSP_LIST_ENTRY_T     tx_skb_free_list;
    TX_SKB_DATA          tx_skb_buffer[TX_SKB_BUF_NUM];

}TX_CONTEXT,*PTX_CONTEXT;



// fdeng mask this for debug wlan in Android! 
//
//#pragma pack()


//----------------------------------------------------------------------
#define DOT11_SNAP_HDR_LEN  sizeof(snap_hdr_t) // e.g. { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
#define DOT11_SNAP_HDR_LEN_MINUS_TYPE  (DOT11_SNAP_HDR_LEN - 2)

#define RATE_UNMASK			0x7F
////////////////////////////////////////////////////////////////////
TDSP_STATUS Tx_Init(PDSP_ADAPTER_T pAdap);
void  Tx_Release(PDSP_ADAPTER_T pAdap);

void  Tx_Release(PDSP_ADAPTER_T pAdap);
void  Tx_Reset(PDSP_ADAPTER_T pAdap);

void Tx_Restart(PDSP_ADAPTER_T pAdap);

void Tx_Stop(PDSP_ADAPTER_T pAdap);

TDSP_STATUS
Tx_Transmit_FW_Fragment (
		   PDSP_ADAPTER_T pAdap, 
		   PUINT8		   pbuff,
		   UINT32		   len
			);

TDSP_STATUS
Tx_Get_FW_Fragment (
		   PDSP_ADAPTER_T pAdap, 
		   PUINT8		   pbuff,
		   UINT32		   len
			);

BOOLEAN Tx_Transmit_mng_frame (
		   PDSP_ADAPTER_T pAdap, 
		   UINT8*			pbuff,
		   UINT32		   len,
		   TX_MNG_FRM_PRIORITY priority,
		   TX_MNG_FRM_TYPE	 type);

BOOLEAN Tx_Send (PDSP_ADAPTER_T pAdap, UINT8* data, UINT32 data_len);

VOID Tx_Watch_TimeOut_Routine(PDSP_ADAPTER_T pAdap);


VOID	Tx_get_counts(	PDSP_ADAPTER_T pAdap, 
PUINT32		p_good_cnt,
PUINT32		p_good_payload_cnt,
PUINT32		p_error_cnt);

BOOLEAN tx_is_congested(PDSP_ADAPTER_T pAdap);

void Tx_Process_Retry_Int(PDSP_ADAPTER_T pAdap,UINT32 retry_num);


BOOLEAN Tx_DataFrmlist_IsEmpty(PDSP_ADAPTER_T pAdap);

BOOLEAN Tx_MngFrmlist_IsEmpty(PDSP_ADAPTER_T pAdap);

void  Tx_Cancel_Data_Frm(PDSP_ADAPTER_T pAdap);


VOID Tx_Send_Next_Frm(PDSP_ADAPTER_T pAdap);
VOID Tx_Reset_Retry_Queue(PDSP_ADAPTER_T pAdap);
VOID Tx_Print_Status(PVOID adapt);

#if 1
void Tx_print_buffer_msdu(PDSP_ADAPTER_T pAdap);
#endif
#define	 DURATION_CTSTOSELF(pAdap,_x,_y,_z) \
	(pAdap->wlan_attr.gdevice_info.sifs + \
	_duration_ack(pAdap, pAdap->wlan_attr.gdevice_info.preamble_type,(UINT8)(_y)) +		\
	_durn(pAdap,pAdap->wlan_attr.gdevice_info.preamble_type,(UINT16)(_x), (UINT8)(RATE_UNMASK&(_y))))

UINT16 _durn(PDSP_ADAPTER_T pAdap, UINT8 short_preamble,
	UINT16  frm_len,  UINT8	rate);

void  tx_get_speed_and_duartion(
			PDSP_ADAPTER_T pAdap, 
			UINT8 			speed,
			PHW_HDR_PARSE_CONTEXT pcontext,
			PHW_HDR_PARSE_RESULT	presult,
			BOOLEAN  use_lower_speed);

#define	 DURATION_RTS(pAdap,_x,_y,_z) \
	((2 * pAdap->wlan_attr.gdevice_info.sifs) + \
	_durn(pAdap,pAdap->wlan_attr.gdevice_info.preamble_type,14,	\
		 pAdap->wlan_attr.rate & RATE_UNMASK) + \
	_duration_ack(pAdap, (UINT8)(_y)) +		\
		 _durn(pAdap,(UINT16)(_x), (UINT8)(RATE_UNMASK&(_y))))

#endif //file end
