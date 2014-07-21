#ifndef DSP_SW_H
   #define DSP_SW_H

/***********************************************************************
 * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
 *
 * FILENAME:     DSP_Sw.h     CURRENT VERSION: 1.00.01
 * PURPOSE:      mainly define the structrue that store the total adapter
 *               information (adapter context).
 * 
 *
 * DECLARATION:  This document contains confidential proprietary information that
 *               is solely for authorized personnel. It is not to be disclosed to
 *               any unauthorized person without prior written consent of 3DSP
 *               Corporation.        
 *
 **********************************************************************/
	

//#include <linux/netdevice.h> 
#include "precomp.h"
/*
#include "usbwlan_charact.h"
#include "usbwlan_wlan.h"
#include "usbwlan_usbConfig.h"
#include "usbwlan_MacConfig.h"
//#include "usbwlan_Interrupt.h"
//#include "usbwlan_Task.h"
//#include "usbwlan_PktList.h"
//#include "usbwlan_mngqueue.h"
//#include "usbwlan_rxarea.h"
//#include "usbwlan_UsbDev.h"
//#include "ntddndis.h"
//#include "usbwlan_UsbDev.h"
//
#include "usbwlan_defs.h"
#include "tdsp_event.h"
#include "tdsp_timer.h"
#include "tdsp_mutex.h"
#include "usbwlan_vendor.h"
*/

#define USB_WLAN_VERSION         "WL090120_2"

/*--macros------------------------------------------------------------*/

#if defined(NDIS51_MINIPORT)
	#define DSP_USBWLAN_NDIS_MAJOR_VERSION       0x5
	#define DSP_USBWLAN_NDIS_MINOR_VERSION        0x1
#endif

#if defined(NDIS50_MINIPORT)
	#define DSP_USBWLAN_NDIS_MAJOR_VERSION       0x5
	#define DSP_USBWLAN_NDIS_MINOR_VERSION        0x0
#endif


#define DSP_USBWLAN_DRIVER_VERSION ((DSP_USBWLAN_NDIS_MAJOR_VERSION*0x100) + DSP_USBWLAN_NDIS_MINOR_VERSION)




//Justin: 0623	//////   define card type 
#define BOARD_2230
//Given a MiniportContextHandle return the PDSP_ADAPTER_T it represents.
#define PDSP_ADAPTER_FROM_CONTEXT_HANDLE(Handle) ((PDSP_ADAPTER_T)(Handle))

// Define the max multicast list counts
#define MAXMULTICASTADDS               32

#define TIMER_1MS_DELAY 			1   //MILI SECOND
#define TIMER_10MS_DELAY 			10   //MILI SECOND
#define  TIMER_PERIODIC_DELAY		500   //periodic timer delay		2 seconds.	= 2000 miliseconds
#define  CHIP_FUNCTION_WLAN_ONLY			0
#define  CHIP_FUNCTION_BT_ONLY				1
#define  CHIP_FUNCTION_COMBO				2
#define  TX_HANG_EXPECT_1S_COUNT        (1000/TIMER_PERIODIC_DELAY) 
#define  TX_HANG_TRANSACT_INTERVAL     (3*TX_HANG_EXPECT_1S_COUNT) 
/*--constants and types------------------------------------------------*/


/*Jakio 2006-11-24: 
  *why choosing this size?? Because the hw fifo is 12KB in size, I need a little bigger space
  *than that to ensure the trustness
  */
#define  MAX_MINI_MEMORY_POOL_SIZE    15360

//Jakio20070606: heart beating mechnism
#define	DEFAULT_AP_ALIVE_COUNTER		10//20//40//4000//justin : 0727.   change to 40	0920	yo 20, just like buffalo time

//Jakio 20070302: add for the surprising removed status, should be fixed later
#define  STATUS_SURPRISING_REMOVED    0xcfff
#define  SURPRISING_REMOVED_THRESHOLD		0x12

//#define DSP_FW_MODE_WLAN			(0)  //indicate current fw is wlan single fw.
//#define DSP_FW_MODE_COMBO 		(1) //indicate current fw is combo fw

// fdeng mask this !!!
//
//
//#pragma pack(1)

typedef enum pss_tag
{
	PSS_ACTIVE,
	PSS_FAST_DOZE,
	PSS_CLASSIC_DOZE
}pss_t;


typedef enum
{
	TXHANG_IGNORE,
	TXHANG_RESETHW,
	TXHANG_RESERVE
}TXHANG_FLAG;

#ifdef ROAMING_SUPPORT
typedef enum
{
	NO_RECONNECT,		//can not do reconnect in this status. Always set to NO_RECONNECT if normal running or after reconnect
	NEED_RECONNECT,	//need to do reconnect in this status, but not ready to do
	CAN_RECONNECT,		// ready to do reconnect
	DOING_RECONNECT,
	DOING_DISCONNECT
}RECONNECT_STATUS_E;
#endif
typedef enum
{
	REQUEST_CM_RETURN_GO_ON,		//reture this state to let tx/rx flow continue
	REQUEST_CM_STOP_CURRENT,			//request prevent os command when cm happen
	REQUEST_CM_CALCU,					//request beginning calcu cm counter
	REQUEST_CM_TIMEOUT,				//request when cm timer to timeout
	REQUEST_CM_DROP_RX,
	REQUEST_CM_SET_SSID,
	REQUEST_CM_MONITOR_MIC_COUNTER,
	REQUEST_CM_END_COMMAND
}COUNTER_MEASURE_REQUEST_TYPE;


//woody for combo
typedef enum DSP_DRIVER_STATE_TAG
{
	DSP_DRIVER_HALT,       //driver recycle all resource, and close all flows
	DSP_DRIVER_INIT,        //driver recycle all active resource, and close tx/rx/int flow
	DSP_DRIVER_READY,	    //driver recycle all active resource, and close tx/rx flow,but int flow can work due to w/r 3dsp register
	DSP_DRIVER_WORK,	    //driver is in running, TX/RX/INT flow can work
	DSP_HARDWARE_RESET,
	DSP_SYS_RESET,
	DSP_POWER_MANAGER,
	DSP_STOP_TX,
	DSP_STOP_TX_WHILE_PWRMGR,
	DSP_SUPRISE_REMOVE,
	
}DSP_DRIVER_STATE_T;


typedef enum
{
    FW_TYPE_8051,
    FW_TYPE_DSPCOMBO,
    FW_TYPE_DSPWLAN,
    FW_TYPE_DSPEXT,
    FW_TYPE_NONE,
}FW_TYPE;

typedef struct _bss_tag
{
	BOOLEAN ibss;
	BOOLEAN acting_as_ap;
	BOOLEAN pc_avail;
	BOOLEAN pc_dlvr;
	BOOLEAN pc_poll;
	UINT8	dtim_count;
	UINT8 	dtim_period;
	UINT16	brate_map;
	UINT8	cfp_period;
	UINT8	cfp_count;
	UINT8	channel_num;
#ifdef ANTENNA_DIVERSITY
	UINT8   antenna_num;
#endif
	UINT16	aid;
	UINT16  join_timeout;
	UINT16 	probe_delay;
	UINT16	atim_window;
	UINT16	cfp_max_duration;
	UINT16 	cap_info;
	UINT16	beacon_interval;
	UINT8 	rates[9+6];
	UINT8	ssid[WLAN_SSID_MAXLEN + 1 /*33*/];
	UINT8	bssid[6];
}bss_t;

#if 0 //TODO: Jackie
typedef struct rcv_msdu_tag
{
	struct        rcv_msdu_tag*		next_p;		/* points to next node 		*/
	struct        rcv_msdu_tag*		prev_p;
	UINT16		  seq_num;			/* seq num of the msdu		*/
//	UINT32		  rcv_time_stamp;		/* time stamp of the msdu 	*/
	LARGE_INTEGER rcv_time_stamp;		/* time stamp of the msdu 	*/
	BOOLEAN		  rx_timeout;		/* flag to indicate rx timer has timed out*/
	BOOLEAN		  rx_timeout_handled;/*flag to indicate time out has been handled*/
	UINT16		  data_len;			/* len of the data rcvd 	*/
	UINT16		  buf_len;			/* available size in the current msg buffer */
	BOOLEAN		  frag_burst;			/* frag burst is going on	*/
	UINT8		  frg_num;			/* frag num of the mmpdu	*/
	UINT8		  index;				/* Index of th msdu list	*/
	UINT8		  src_addr[WLAN_ADDR_LEN];		/* src of the msdu			*/
	//kl_list_t	  data_que;			/* hold data in mbuf fmt	*/
}rcv_msdu_t;
#endif //#if 0 //TODO: Jackie

// Association response 
typedef struct assoc_resp_info_t
{
	UINT16		capability_info;
	UINT16 		status_code;
	UINT16 		association_id;
	UINT8		SupportedRatesElementID;
	UINT8		SupportedRatesLength;
	UINT8		SupportedRates[NUM_OF_SUPP_RATES];		// space for 12
	UINT8		ExtSupportedRatesElementID;
	UINT8		ExtSupportedRatesLength;
	UINT8		ExtSupportedRates[NUM_OF_SUPP_RATES];	// space for 12
}assoc_resp_info_t;


typedef struct _DISASSOCIATE_COM
{
	UINT8        addr[WLAN_ETHADDR_LEN];
	UINT16       reason;
	UINT16       ind;
} DISASSOCIATE_COM_T, *PDISASSOCIATE_COM_T;

typedef struct core_info_tag
{
	BOOLEAN					tx_disable;
	BOOLEAN					tx_disable_due_to_bgscan;
	BOOLEAN					tx_disable_due_to_retry;
	BOOLEAN					tx_disable_due_to_security;
	BOOLEAN					port_up; /* if WLAN_TRUE, data tx is allowed */
	BOOLEAN					privacy_option;
 	BOOLEAN 					hdr_node_crunch;
 	BOOLEAN 					data_node_crunch;
// 	BOOLEAN					mlme_tx_disable;
 	BOOLEAN					bcn_rx;
	BOOLEAN					awake_flag;
	BOOLEAN					frame_pend;
	BOOLEAN					atim_flag;
	BOOLEAN					pow_mgt_bit;
	BOOLEAN					cfp_flag;
	BOOLEAN					mlme_req_flag;
	BOOLEAN					idle;
	UINT8					active_bss;
	UINT8					from_to_ds;
	UINT8					tuple_index;
	UINT8					power;
	UINT8					rts_bspeed;
	UINT8					max_brate;
	UINT8					max_orate;
	UINT8					alt_orate;
 	UINT8 					rssi;
//	UINT8					preamble_length;
//	UINT8					plcp_header;
	UINT8 					ps_index;
	UINT8					ps_sta_count;
//	UINT8					wep_key_id;
	UINT8					cf_pollable;
//	UINT8					mac_addr[WLAN_ADDR_LEN];
// 	UINT8 					bss_id[WLAN_ADDR_LEN];
	UINT16					seq_num;
	UINT16					bcn_seq_num;
	UINT16 					listen_interval;
	UINT16					probe_resp_len;
	UINT16					bcnlen_till_bss_param;
	UINT16					bcnlen_till_cfp_param;
 	UINT16					port_id;
 	UINT16 					rssi_threshold;
	UINT16					tx_fifo_size;
	UINT32					chan_switch_flag;
 	UINT32					version;
	void*					mac_rx_que_p;
	void*					macdsm_rx_que_p;
	void*					maccore_mlme_que_p;
   	void*           		ps_timer_p;
	//mac_data_t*				bcn_frm_p;
	//msg_buf_t*				user_buffer;
	//rcv_msdu_t*				cache_tuple_p;
	//rcv_msdu_t*				rx_timer_tuple_p;
	BOOLEAN					rx_frag_timeout;
	//mac_if_t*				mac_if_p;
	//chan_switch_state_t		chan_switch_state;
	bss_t					bss_info[2];
	pss_t					ps_support;		// Justin: indicate what power save mode was supported
 	//mac_data_t  			hdr_node;
 	//frm_info_t 				data_p;
	pss_t					ps_mode;	// Justin:  indicate current power save mode 
	//mac_data_t				null_data;
	UINT8			        cipher_type;
	encryption_keysize_t	encryption_keysize;
	dot11_type_t			dot11_type;
	dot11_type_t			ibss_dot11_type;
	modulation_type_t		modulation_type;
	preamble_type_t			preamble_type;
	//msg_buf_t				mac_buffer;
	//mac_data_t				ps_poll_t;
	assoc_resp_info_t		assoc_resp_info;
	UINT8					sifs;
	UINT8                   ofdm_signal_extension;
	UINT8					slot_time;
//	UINT8					num_of_supp_rates;
	UINT16					fifo_size[FIFOS];
	//info for rate/length switching 
	UINT8					prev_rcvr_addr[6];
	UINT8					prev_channel_num;
	UINT8					acc_icvErrCnt;
	UINT8					acc_privacyExcludeCnt;
	UINT8					acc_ackFailCnt;
	UINT8					ackFailCntList[LISTS_SIZE];
	UINT8					acc_frmDupCnt;
	UINT32					ackCount;
	UINT32					ackCountList[LISTS_SIZE];
	UINT32					pktSizeCnt;
	UINT32					pktSizeList[LISTS_SIZE];
	UINT8					failMult[12][3];
	BOOLEAN					autoFragTh;
	BOOLEAN					autoRate;
	BOOLEAN					ps_poll_tx_allowed;
	BOOLEAN					prev_protect_mechanism;
	//BOOLEAN					antenna_sel;
	//The state is useful to improve throughput
	//when scan, set it to 0, so hw's sensativite can be improved, more Ap can be found
	//when link with ap, set it to 1 is good, otherwise more noise will be received.
	//in addition, according RSSI throughput, we can adjust open/close the bit
	//detail process can reference pci driver
	//BOOLEAN					 auto_corr_off;
	//BOOLEAN                             ap_associated;
	//BOOLEAN                             soft_doze;
	BBREG2023_REG_T              bbreg2023;
	MAC_CTL_REG_T			mac_ctl_reg;
#ifdef DBG
	//for debug
	UINT32					dbg_data_node_cnt;
#endif
}core_info_t;


typedef struct _AUTH_DATA
{

   UINT16       auth_alg;
   UINT16      	wpa_version;
   UINT8        key_mng;
   UINT8		group_cipher;
   UINT8        parwise_cipher;
}AUTH_DATA;

/* This structure defines the most attributes vars assicated with WLAN 
  protocol and others */
typedef struct _DSP_ATTR
{
	core_info_t 	gdevice_info;
	UINT32       	ethconv;
	UINT8		bssid[WLAN_BSSID_LEN];		//Justin:	holds the last success connected AP's addr
	UINT32		macmode;
	UINT8		dev_addr[WLAN_ADDR_LEN];
	UINT8       	hasjoined;
// 	cancel UINT8       ps_state;
	UINT8		   tx_power;	// add by Justin
 //   BOOLEAN        fw_valid;
 //	PUINT8         fw_map_buff;
//	UINT32         fw_map_length;
//	UINT8       	ps_state_config;
//	MAC_CONFIG_INFO_T  mac_config_info;
//	UINT8       	phymode;
	UINT8       	channel_num;
	UINT8       	channel_list[WLAN_MAX_CHANNEL_LIST];
	UINT8       	channel_default;
	UINT16      	channel_len_of_a;		//edit by Justin
	UINT16      	channel_len_of_g;		//edit by Justin
	UINT8		current_channel;
	UINT8       	ssid[WLAN_SSID_MAXLEN];
	UINT8       	ssid_len;
	UINT16      	aid;
	UINT16      	auth_alg;
	UINT16      	auth_mode;
	UINT8       	encryption_level;
//	BOOLEAN    	auto_frag_threshold_enable;	// add by Justin
	UINT8		frag_threshold_mode;//justin: 1105
	UINT16      	frag_threshold;
	UINT8       	fallback_rate_to_use;
	UINT8       	antenna_diversity;
	UINT8           antenna_num;
	UINT8       	rate;
	UINT8       	rateshowed;
	UINT8       	last_rateshowed;
	UINT8       	rateshowed_count;
	//the variable is used for saving using rate before down rate
	//the rate will be re-used after retry queue null
	UINT8		rate_change_flag;
	UINT8       	rate_using;
	BOOLEAN       rate_fresh;
	UINT8       	atim_rate;
	UINT8       	used_basic_rate;
//	UINT8       	rsptime_rate_mode;
//	UINT8       	ack_basic_rate;
	UINT8       	rts_rate;
	UINT16      	rts_threshold;
	BOOLEAN    	rts_threshold_enable;	// add by Justin
	wpakey_info_t key_map[REAL_KEY_ARRAY_SIZE];  //0-3 index used for group of wep mode 
	UINT32          default_key_index;
//	UINT8       	wep_key0[WLAN_WEP_MAXKEYLEN]; 
//	UINT8       	wep_key1[WLAN_WEP_MAXKEYLEN]; 
//	UINT8       	wep_key2[WLAN_WEP_MAXKEYLEN]; 
//	UINT8       	wep_key3[WLAN_WEP_MAXKEYLEN]; 
//	UINT8      	wep_mark_bit;
//	UINT8       	wep_key_id;
//	UINT8       	wep2_tk[WLAN_WEP2_TKLEN];  //close it,woody,no use
//	UINT8       	mic_key[WLAN_MIC_KEY_LEN];
//	UINT8       	wep_to_use;
	UINT8       	wep_mode;
	UINT8		group_cipher;	//Jakio20070702, for wpa2 tkip&aes mixed mode
	UINT8       	scan_type;
	UINT16      	min_channel_time;
	UINT16      	max_channel_time;
	UINT8       	txmode_beacon;
	UINT8       	txmode_others;
	UINT16      	beacon_interval;
	UINT16      	ibss_beacon_interval;
	BOOLEAN		is_exist_ibss_b_sta;	//	set to 1 if there is a b station connected to this ibss set	.by Justin, 20081016
	UINT16      	atim_window;
	UINT16		beacon_window;
	CAPABILITY_T  capability;
	UINT8       	support_rate[WLAN_MAX_RATE_SUPPORT_LEN];
	UINT8       	support_rate_len;
//	UINT8       	ext_support_rate[WLAN_MAX_EXT_RATE_SUPPORT_LEN];
//	UINT8       	ext_support_rate_len;
	UINT16      	support_basic_rate_new;
	UINT16      	auth_timeout;
	UINT16      	asoc_timeout;
	UINT16      	scan_timeout;
	UINT16      	join_timeout;
	UINT16      	rx_max_len;
//	IFS_T       	ifs;
	UINT8       	tsf[WLAN_TSF_LEN];
	RETRY_LIMIT_T  retry_limit;
	UINT16      	listen_interval;
	UINT16      	atim_interrupt_timer;
	UINT16      	pre_tbtt_timer;
	UINT16      	tx_frame_lifetime;
	CW_WIN_T    cw_win;
	UINT16      	tx_beacon_time;
	UINT16      	tx_probe_time;
	UINT8       	tx_beacon_length;
	UINT8       	tbtt_timer[WLAN_TBTT_TIMER_LEN];
	UINT8       	gen_timer[WLAN_GENERAL_TIMER_LEN];
	UINT16      	rx_beacon_capability;
	UINT8       	rx_beacon_ds_channel;
	UINT16      	rx_beacon_ibss_atim_wm;
	UINT8       	rx_average_beacon_threshold;
//	UINT8       	wpa_pairwise_key[WLAN_WEP2_TKLEN];
//	WPA_GROUP_KEY_T    wpa_group_key[WLAN_WPA_MAX_GROUP];
	UINT8		wpa_pairwise_mic_tx[WLAN_MIC_KEY_LEN];
	UINT8		wpa_pairwise_mic_rx[WLAN_MIC_KEY_LEN];
	UINT8		wpa_group_mic_tx[WLAN_MIC_KEY_LEN];
	//update mic rx to 5 array to save 0-4 group key
	UINT8       wpa_group_mic_rx[5][WLAN_MIC_KEY_LEN];

	UINT8		wpa_group_key_index;
	WPA_TSC_T	wpa_pairwise_tsc_tx_counter;
	WPA_TSC_T	wpa_pairwise_tsc_rx_counter;
	WPA_TSC_T	wpa_group_tsc_tx_counter;
	WPA_TSC_T	wpa_group_tsc_rx_counter;
	// support data for wpa
	UINT16		wpa_capability;
	UINT16		wpa_listen_interval;
	
	// key valid flag
	UINT8		wpa_pairwise_key_valid;
	UINT8		wpa_group_key_valid;
	
	UINT8		regoin;
	UINT8		seed_result_8021x[WLAN_8021X_KEY_LEN];
	
	// cts en // self-sending cts flag
	UINT8		cts_en;
	UINT8		cts_to_self_config;
	UINT8		cts_current_state;     //indicate either rts/cts or cts to self be used 
	
	// if this flag is set, the erp bit of tx descriptor must be set. It means hardware
	// will start RTS/CTS or CTS-To-Self mechanism.
	//	UINT8		erp_en;
	
	// Store the ERP element information
	UINT8		erp_element;
	
	// we can choice the following value: WLAN_802_11_MODE_11B if we only support 802.11b
	//                                    WLAN_802_11_MODE_11G if we support 802.11g/b
	//UINT8       wlan_802_11_mode;

	// If driver received 802.1x frame, this flag is set.
	UINT8		start_8021x_flag; 

	BOOLEAN		need_set_pwr_mng_bit_flag;

	INT32		rssi;		//Justin:		071214.	save the connected AP's rssi
	

	// If we want to use WZC to control driver, this flag is set as 1. If we want to use
	//  WCU to control driver, this flag is set as 0.
	UINT8		wzc_open;
	//Jakio:  sequence number field.  20061018
	UINT16		seq_num;
	UINT32		force_B_only;	// add by Justin
	UINT32		chipID;
	UINT8		set_disassoc_flag;
//	struct iw_statistics wstats;
	BOOLEAN 	use_wpa2;
	UINT8		key_mng;		//key managemnt: 0: NONE; 1: PSK; 2: EAP(1X)
//	AUTH_DATA   auth_mode_data;
} DSP_ATTR_T, *PDSP_ATTR_T;

/* This structure defines the vars for TKIP counter-measures(802.11i) */
typedef struct _DSP_TKIP_COUNTER_MEASURE
{
	UINT16				mic_failure_counts;	// MIC failure counts. if this counts is greater than 2 in
	                                        // 60 seconds, driver should start the operation of TKIP counter 
	                                        // measures.
	BOOLEAN             mic_failure_rate_timer_valid;  // 60 seconds timer1
	UINT16              mic_failure_rate_timer_counts; //            timer1
	BOOLEAN             wait_1x_frame_timer_valid; // 200 ms timer2
	UINT16              wait_1x_frame_timer_counts; //       timer2
	BOOLEAN             delay_60_seconds_timer_valid; // 60 seconds timer3
	UINT16              delay_60_seconds_timer_counts; //       timer3
	BOOLEAN             disassociate_frame_sent;
} DSP_TKIP_COUNTER_MEASURE_T, *PDSP_TKIP_COUNTER_MEASURE_T;


/*Jakio 2006-11-24*/
/*Realize a mechanism to avoid frequently memory allocating for retry limit.
 * this struct record the memory queue, when need mem, allocate it from this pool,
 * after used , free it to the pool.
  */
typedef struct  _MINI_POOL_STRUCT_
{
	PVOID   buffer;
	UINT32   head;
	UINT32   tail;
	TDSP_SPINLOCK lock;
}MINI_POOL_STRUCT, *PMINI_POOL_STRUCT;

/*Jakio 2006.12.12*/
//used to record some info when testing tx/rx fifo
typedef struct _FIFO_TEST_STRUCT
{
	//TxIrp pointer, used to record the sending irp, 'cause we want to triger event in rx_completion
	PVOID	pTxIrp;
	//RxIrp pointer, used to cancel irp when forcing stop test, add on 2006.12.25
	PVOID	pRxIrp;
	//TxContext pointer,
	PVOID	pTxContext;
	//fifo address offset
	UINT32	offset;
	UINT32	TxLength;		//data length to send
	UINT32	RxLength;		//data lenght of received data
	
	BOOLEAN	TestInProcess;	//test in processing flag, add on 2006.12.25
	BOOLEAN	TxStopFlag;		//flag to stop tx test, add on 2006.12.25
	BOOLEAN	IsOK;			//flag to identify whether this test is success or not
	PUINT8		Buffer;			//data buffer to be sent
	
}FIFO_TEST_STRUCT, *PFIFO_TEST_STRUCT;

//
// This structure contains all the information about a single
// adapter that this driver is controlling.
//
struct _DSP_ADAPTER 
{

    //net device related
	void *net_dev;
	void* net_stats;

	PVOID pBusInterfaceData;
	USB_INFO       usb_info;
    // Linux net-queue corresponding to this net_device stopped due to congestion in driver/hw.
	UINT8			netq_stopped; 
	//UINT8			netq_disabled;
	//TDSP_SPINLOCK		netq_ctrl_lock;

    
	// Current MAC address in use
	UINT8			current_address[ETH_LENGTH_OF_ADDRESS];
	// Permanent MAC address which is usually stored in EEPROM
	UINT8			permanent_address[ETH_LENGTH_OF_ADDRESS];

	// The packet filter (NDIS_PACKET_TYPE_DIRECTED or NDIS_PACKET_TYPE_MULTICAST or NDIS_PACKET_TYPE_BROADCAST and so on)
	UINT32			packet_filter;

	// Store task module data
	PVOID			ppassive_task;
    

	// the pointer of frag module 
	PVOID			pfrag;

	/* the most attribute vars*/
	DSP_ATTR_T		wlan_attr;


	/* This flag is set when we have to reset hard ware but not need to indicate the upper level */	//Justin: 0615.... add
	BOOLEAN			hardware_reset_flag;
	/* This flag is set when halt function is called */

	//combo
	//BOOLEAN		halt_flag;
	DSP_DRIVER_STATE_T		driver_state;

	BOOLEAN					run_in_halt_flag;//Justin 20080808: for fix bug 3157.	In win2000, wlan-driver can not be uninstalled, because halt is invoked while hardware-reset.
											// but halt will not process untill hardware finished. (we designed this before for some reason)
											//so, I add this flag to say that driver halt function has been incoked, and then hardware-reset will run over
											// as fast as posible.

	UINT8					is8051InMiniLoop;//Justin 20090107. hold the status of 8051. set to 1 if 8051 has been in miniloop. set to 0 if 8051 has been in mainloop.
											// if do not get a status, it should be set to 0xFF
	

	//justin: 1009		true: more data should be recv;		false: no more data
	BOOLEAN			more_data;

	//For dlink 1310 ap
	UINT32			wpa_1x_no_encrypt;

	/* used for download Firmware*/
	TDSP_EVENT		tx_fm_event;

	/*Jakio: these two lists are used for realizing RetryLimit policy*/

	//Jakio 2006.12.12: just for testing rx/tx fifo
	FIFO_TEST_STRUCT TestFifo;

	//Jakio 2007.01.30: add for read dsp reg 
	DSP_REG_READ	DspRegRead;

    TDSP_EVENT		DspReadEvent;

	//Jakio20070712: add for power save
	UINT32			ps_state;
	//The variable set while bulk out will be drop on bus
	//The variable clear while the bulk out return from lower driver
	//When retry limit happen, if the variable is not zero
	//this means a tx packet on road to be sent.
	//woody1224
	//ULONG	       TxData_On_Road;

	UINT32 		Retrylimit_Reach;

	//wmi support, Jakio added here 2006.11.07
	UINT32			OidForCustomAccess;

	//Added by joe 2007-11-14
	UINT32			ulBssidQueryIndex;
	//End
	//Jakio20070411: add for record error reason, such as read_dsp error....
	UINT32			error_record;

	PVOID			usb_context;

	PVOID			tx_context;

	// receive area which is used for the packet and buffer indicated to the up layer
	PVOID			prx_pool;
	// Current rssi value
	//UINT32  rssi;
	INT32			rssi;	//Justin:	071210.	rssi can be a negative value.
	RSSI_UPDATE_T	rssi_update;

	// If scan is in progress, this flag is set TRUE. It is to say, driver is scanning
	BOOLEAN			scanning_flag;

	BOOLEAN			seq_tag;
	// Save the source data whose head type is 802.3 type.
	// PUINT8  ptx_cached;

	// For test driver.
	PVOID			pdut_data;
	
	// For management.
	PVOID			pmanagment;

	// For phy module
	PVOID			pphy;
	
	// For ATPA module
	PVOID			patpa;

	BOOLEAN			OID_not_allow_flag;	// OID is not allowed to ex if flag is TURE
	
	// Timer for system. This timer is a periodic timer in fact.
	WLAN_TIMER		sys_timer;

	//the timer for keeping watching if pending oid has been return
	WLAN_TIMER  pending_watch_timer;

	// Timer for management module. 
	WLAN_TIMER		mng_timer;	

	// PeriodicTimer	per 50 milliseconds;		//add bu Justin
	WLAN_TIMER		periodic_timer;

	WLAN_TIMER		tx_countermeasure_timer;

	//save the current tx packet's context
	PVOID			ptx_context;

	//
	PVOID			int_context;
	// Int mask
	UINT32			int_mask_bits;
	
   // If link is ok, this flag is set 1.
	media_link_state_t   link_ok;
	//wumin 090312 delete, media_link_state_t   link_is_active;
	UINT8			device_removed;	/*0:pluged in, 1:removed(pluged out)*/
	// Current reset type
	UINT8			reset_type;
	UINT32			reset_sub_type;
	UINT32          d3_fail_count;

	TDSP_EVENT		is_8051_ready_event;
	BOOLEAN 	is_set_8051_miniloop_failed;		//Justin: 20081211.	if set 8051 to miniloop failed, driver will do something to recover. (for example, fail in D3 OID process)
	
	//Jakio2006.12.22: flag to identify wheter test mode is active
	BOOLEAN			test_mode_flag;

	// Reset completed flag. If Reset procedure is completed, this flag is set TRUE.
	BOOLEAN			reset_complete_flag;

	// Reset needing flag. If driver wants to reset procedure, this flag is set TRUE.
	BOOLEAN			sys_reset_flag;

	BOOLEAN			reconnect_after_reset_interrupt_flag;

	hw_8051_work_mode_t hw_8051_work_mode;
	
	// Just for HCT test, HCT test needs indicating a disconnect event when initialized.
	// If this flag is set, driver indicated a disconnect event to upper layer and then
	// set this flag to FALSE.
	BOOLEAN first_run_flag;

	// rx error flag
	UINT32 rx_error_count;
	UINT32 tx_error_count;
	UINT32 tx_hang_count;
	UINT32 rx_irp_c01_count;
	UINT32 int_irp_c01_count;
    TXHANG_FLAG	 txHangFlag;

	// Information complete flag. If driver return the STATUS_PENDING status for
	//  OID operation. In passive level task, this flag will be set. And later when
	// system timer is timeout, the timeout function will call NdisMSetInformationComplete
	// if this flag is set.
	BOOLEAN set_information_complete_flag;

	// USB mode (usb1.1 or usb2.0)
	UINT8  usb_mode;

	//Jakio20070606: heart beating mechnism
	UINT16	ap_alive_counter;
	
	// Beacon frame sent flag , just for IBSS
	BOOLEAN beacon_send_ok;
	// Atim frame sent flag, just for IBSS
	BOOLEAN	atim_send_ok;

	// If we use WCU to control driver, this flag is set TRUE.
	BOOLEAN use_wcu_flag;

#ifdef ROAMING_SUPPORT
	// means that do reconnect in this time will not interrupt the other process
	//BOOLEAN 	can_do_reconnect_flag;
	RECONNECT_STATUS_E   reconnect_status;
	UINT8			oldBssid[WLAN_ETHADDR_LEN];		//to save mac address of AP former connected
#endif

	// Save all counters.
	MIB_STATISTIC_T mib_stats;

// TODO: Jackie
	// OS power state. Normally the OS power state is set D0 or D3. The default
	// power state is D0
	//NDIS_DEVICE_POWER_STATE power;

	//Jakio added here
	//UINT8  txpwr_level;

	// This member is used for TKIP counter-measures
	DSP_TKIP_COUNTER_MEASURE_T tkip_counter_measure;

#ifdef PMK_CACHING_SUPPORT // add by jason 2007.8.23
	NDIS_802_11_USED_PMKID		pmk_id_caching;
#endif

#ifdef PMK_CACHING_SUPPORT // add by jason 2007.8.23 begin for PMK Caching
#define MAX_NUM_OF_CAND_LIST  3 // change by jason 2007.9.4
#define PMK_CAND_BUF_LEN (sizeof(NDIS_802_11_STATUS_TYPE)+sizeof(NDIS_802_11_PMKID_CANDIDATE_LIST)*MAX_NUM_OF_CAND_LIST)

	char pmkCandidateBuf[PMK_CAND_BUF_LEN];
#endif // add by jason 2007.8.23 end for PMK Caching
	
	// Spin lock for tx, rx and so on.
	TDSP_SPINLOCK            lock;        /* lock */
	TDSP_SPINLOCK		    fw_lock;        /* lock */

	// For checking scanning. If the scanning is not completed in 3 seconds, 
	//  driver will discard this scanning.
	BOOLEAN			is_oid_scan_begin_flag;  // if oid scanning is beginning, this flag is set 1. And if 
	                           //oid scanning is ending, this flag is set 0.
	UINT16			scan_watch_value;
	UINT32			scan_result_new_flag;
	
	UINT32			firmware_version; // This member saves the firmware version value.

	// For checking if there is any received frame or not after beacon lost interrupt occurs.
	// If this flag is set as 1, driver starts to detect whether there is any received frame 
	// or not.
	// BOOLEAN  detect_rec_frame_flag;  // if beacon lost interrupt occurs, this flag is set as 1.
	// UINT16 detect_rec_frame_value; // timer value
	// UINT32 detect_current_rx_total_packet; // current rx total packet.
	UINT8			check_media_counts; // In this driver, drive will check media state every about four seconds.
					// We use a counter to count the times. Whenever driver calls Adap_CheckForHang 
					// function, the counter is added 1. If the counter reaches 2, driver will 
					// check media state. If Adap_SetLink function is called, the counter is reset
					// as zero.
	// System timer active flag. If this flag is set as TRUE, the system can be restarted. Otherwise the
	// system timer can not be restarted.
	BOOLEAN			systimer_active_flag;

	//if reach 1000, check the state of bulk pipes. 
	UINT32			rate_up_to_24mbps_count;                                                                                //It will be set to 0 if receive irp normal.

	// this flag is just used when set link to up. Driver wants indicates status in dispatch level. So in the 
	// timeout function, driver do the real indicating status.
	BOOLEAN			need_fast_checkmedia_flag;

	//Justin:0623.    In HCT reconnect ap which has connected case, we must indicate to uplevel the link status
	BOOLEAN			must_indicate_uplevel_flag;
	//BOOLEAN		auto_corr_off;
	m2bMailbox1_t	m2bMailbox1;
	BOOLEAN			mac_init_done;
	BOOLEAN 		bStarveMac;

	UINT32 			send_nulll_count;
	UINT32 	        st_time;

	// If driver is doing internal reset process, this flag is set as TRUE.
	// BOOLEAN in_doing_internalreset_flag;

	//for control debug procedure
	UINT32			debug;

	//the variable indicate what 3dsp hw used
	//and it imply work mode is either only or combo.
	UINT32			chip_function;

	//event set for download 8051 or dsp firmware
	TDSP_EVENT          download_fw_event;
	//Justin: 20070927		to holds power table
	UINT32			TxPwrTable[(14+1)*3];
	UINT8           u_eeprom[1024+8];
	//woody for debug variable
	//UINT32			debug_val;
	//combo
	UINT8			dsp_fw_mode;
	UINT32			DSP_FW_version; // This member saves the firmware version value.
	UINT32			DSP8051_FW_version; // This member saves the firmware version value.
	UINT32			dsp_pending_count;
	BOOLEAN			ap_in_list;
	
	TDSP_EVENT		set_key_event;
	UINT32			wpa_timeout;		//	wumin: 090223 wpa auth should complete in limited time after associate.
										//				driver should disassociate if this task not complete.
										
	BOOLEAN			adap_released;
    ULONG  				Tmpdebug;
	UINT8  				TmpSeq;

    BOOLEAN        fw_8051valid;
    BOOLEAN        fw_dspvalid;
    PUINT8         fw_buf;
    UINT32         fw_length;
    UINT8          fw_type;
    UINT16         fw_ext_baseport;

};
#if 0 //TODO: Jackie

/* define an union to save packet or others */
typedef union _DSP_PACKET_TYPE {				
	// the rest is optional
	PNDIS_PACKET	Packet;			// for Tx
	PVOID			Something;		// for Rx
	//... Add more here if needed ...
}DSP_PACKET_TYPE_T, *PDSP_PACKET_TYPE_T;

#endif

//Jakio2006.12.15: struct recording paras after calculating hw_header
typedef struct _HW_HEADER_PARSE_RESULT {
	UINT8	tx_speed;		
	UINT8	short_preamble;
	UINT8	cts_self;	
	UINT16	frag_dur;	//current frag duaration
	UINT16	duration;	//next frag's duaration
	UINT16  air_len;                 //Jakio add 20070406, record the length of air frame 
} HW_HDR_PARSE_RESULT, *PHW_HDR_PARSE_RESULT;

//Jakio2006.12.15: structure used for calculating hw_header elements
typedef struct _HW_HEADER_PARSE_CONTEXT{
	UINT8	more_flag;
	UINT8	privacy;
	UINT16	payload_len;
	UINT16	next_frag_len;
	UINT8	type;
	UINT8	sub_type;	//Jakio20070419: add for ps-poll frame
	
}HW_HDR_PARSE_CONTEXT, *PHW_HDR_PARSE_CONTEXT;

// fdeng mask this !!!
//
//
//#pragma pack()



/*--HEADER BIT-----------------Jakio added here-----------------------*/
#define  USBWLAN_HDRPKT_POS_LEN                     		2
#define  USBWLAN_HDRPKT_POS_FIFO_NUM         		14
#define  USBWLAN_HDRPKT_POS_NOTIFY_VALID  	16
#define  USBWLAN_HDRPKT_POS_NOTIFY_INFO             17
#define  USBWLAN_HDRPKT_POS_FRAG_NUM		 24

#define  USBWLAN_HDRPKT_TYPE_HEADER                      0x00
#define  USBWLAN_HDRPKT_TYPE_NOTIFY			 0x01

#define  USBWLAN_HDRPKT_FIFO_NUM_CP			0x00
#define  USBWLAN_HDRPKT_FIFO_NUM_CFP			0x01
#define  USBWLAN_HDRPKT_FIFO_NUM_PRIOTY		0x10
#define  USBWLAN_HDRPKT_FIFO_NUM_BEACON		0x11

#define  USBWLAN_HDRPKT_HDRLEN                                 8
/*--variables---------------------------------------------------------*/

	
/*--function prototypes-----------------------------------------------*/
		

//Begin  Joe Added for Read Adapter Informaitons. 	2007-08-17


// fdeng mask this for debug wlan in Android! 
//
//#pragma pack(1)
//

typedef struct core_info_tag_TEST
{
	BOOLEAN					privacy_option;                 //woody
	UINT8  			        cipher_type;               //woody

	dot11_type_t			dot11_type;                   //woody

	preamble_type_t			preamble_type;       //woody

}core_info_t_TEST;

typedef struct _DSP_ATTR_TEST
{
	core_info_t_TEST 	gdevice_info;
	UINT8		bssid[WLAN_BSSID_LEN];
	UINT32		macmode;                           //woody
	UINT8		dev_addr[WLAN_ADDR_LEN];           //woody
	UINT8       	hasjoined;                     //woody

	UINT8		current_channel;                   //woody
	UINT8       	ssid[WLAN_SSID_MAXLEN];        //woody   
	UINT8       	ssid_len;                      //woody
	BOOLEAN    	auto_frag_threshold_enable;	  //woody
	UINT16      	frag_threshold;              //woody
	UINT8       	fallback_rate_to_use;    //woody
	UINT8       	rate;              //woody
	UINT8       	atim_rate;
	UINT8       	used_basic_rate;            //woody
	UINT8       	rts_rate;                    //woody
	UINT16      	rts_threshold;			     //woody

	// key valid flag
	UINT8       wpa_pairwise_key_valid;         //woody
	UINT8       wpa_group_key_valid;                 //woody

	UINT8       cts_en;                         //woody
	UINT8       cts_to_self_config;                      //woody
	
	// Store the ERP element information
	UINT8       erp_element;            //woody

} DSP_ATTR_T_TEST, *PDSP_ATTR_T_TEST;

typedef struct _DSP_ADAPTER_TEST
{
	UINT8               current_address[ETH_LENGTH_OF_ADDRESS];
	// Permanent MAC address which is usually stored in EEPROM
	UINT8               permanent_address[ETH_LENGTH_OF_ADDRESS];
	
	// Multicast list counts
	UINT8 multicast_counts;
	// Multicast list
	UINT8        multicast_addr[MAXMULTICASTADDS][ETH_LENGTH_OF_ADDRESS];
	
	// The packet filter (NDIS_PACKET_TYPE_DIRECTED or NDIS_PACKET_TYPE_MULTICAST or NDIS_PACKET_TYPE_BROADCAST and so on)
	UINT32 packet_filter;
	/* This flag is set when we have to reset hard ware but not need to indicate the upper level */	//Justin: 0615.... add
	BOOLEAN				hardware_reset_flag;
	//Justin: 0703
	BOOLEAN				stop_tx_flag;
	/* When a irp for rx is sent out, this count adds one and when
	   a irp for rx is received, this count subs one */
	UINT32       		rx_irps_outstanding;
	/* When a irp for int is sent out, this count adds one and when
	   a irp for int is received, this count subs one */
	UINT32       		int_irps_outstanding;	
    /* When a irp for tx is sent out, this count adds one and when
	   a irp for tx is received, this count subs one */
	UINT32       		tx_irps_outstanding;	
	//Jakio20070712: add for power save
	UINT32 ps_state;

	//wmi support, Jakio added here 2006.11.07
	UINT32  OidForCustomAccess;
	
	//Jakio20070411: add for record error reason, such as read_dsp error....
	UINT32	error_record;
	// Current rssi value
	UINT32  rssi;
	// Int mask
	UINT32    int_mask_bits;
	// Current reset type
	UINT8 reset_type;
	UINT32 reset_sub_type;
	
	//Jakio2006.12.22: flag to identify wheter test mode is active
	BOOLEAN test_mode_flag;
	
	// Reset completed flag. If Reset procedure is completed, this flag is set TRUE.
	BOOLEAN reset_complete_flag;
	
	// Reset needing flag. If driver wants to reset procedure, this flag is set TRUE.
	BOOLEAN sys_reset_flag;
	
	// Just for HCT test, HCT test needs indicating a disconnect event when initialized.
	// If this flag is set, driver indicated a disconnect event to upper layer and then
	// set this flag to FALSE.
	BOOLEAN first_run_flag;
	
	// Information complete flag. If driver return the STATUS_PENDING status for
	//  OID operation. In passive level task, this flag will be set. And later when
	// system timer is timeout, the timeout function will call NdisMSetInformationComplete
	// if this flag is set.
	BOOLEAN set_information_complete_flag;
	
	// USB mode (usb1.1 or usb2.0)
	UINT8  usb_mode;
	
	//Jakio20070606: heart beating mechnism
	UINT16	ap_alive_counter;
	
	// Beacon frame sent flag , just for IBSS
	BOOLEAN beacon_send_ok;
	// Atim frame sent flag, just for IBSS
	BOOLEAN	atim_send_ok;
	
	// If we use WCU to control driver, this flag is set TRUE.
	BOOLEAN use_wcu_flag;
	// For checking scanning. If the scanning is not completed in 3 seconds, 
	//  driver will discard this scanning.
	BOOLEAN  is_oid_scan_begin_flag;  // if scanning is beginning, this flag is set 1. And if 
	// scanning is ending, this flag is set 0.
	UINT16 scan_watch_value;
	UINT32   scan_result_new_flag;
	
	UINT32  firmware_version; // This member saves the firmware version value.
	
	// For checking if there is any received frame or not after beacon lost interrupt occurs.
	// If this flag is set as 1, driver starts to detect whether there is any received frame 
	// or not.
	// BOOLEAN  detect_rec_frame_flag;  // if beacon lost interrupt occurs, this flag is set as 1.
	// UINT16 detect_rec_frame_value; // timer value
	// UINT32 detect_current_rx_total_packet; // current rx total packet.
	UINT8  check_media_counts; // In this driver, drive will check media state every about four seconds.
	// We use a counter to count the times. Whenever driver calls Adap_CheckForHang 
	// function, the counter is added 1. If the counter reaches 2, driver will 
	// check media state. If Adap_SetLink function is called, the counter is reset
	// as zero.
	// System timer active flag. If this flag is set as TRUE, the system can be restarted. Otherwise the
	// system timer can not be restarted.
    BOOLEAN systimer_active_flag;
	
	// this flag is just used when set link to up. Driver wants indicates status in dispatch level. So in the 
	// timeout function, driver do the real indicating status.
    BOOLEAN need_fast_checkmedia_flag;
	
	//Justin:0623.    In HCT reconnect ap which has connected case, we must indicate to uplevel the link status
	BOOLEAN must_indicate_uplevel_flag;
	// If driver is doing internal reset process, this flag is set as TRUE.
	// BOOLEAN in_doing_internalreset_flag;
	
	//for control debug procedure
	UINT32 debug;
	
	//the variable indicate what 3dsp hw used
	//and it imply work mode is either only or combo.
	UINT32 chip_function;
	/* This flag is set when halt function is called */
	BOOLEAN				halt_flag;					//woody
	
	/* This flag is set when adapter is suprised-removed  */
	BOOLEAN				adap_suprise_remove_flag;   //woody

	// If driver wants to scan, this flag is set TRUE.
	BOOLEAN scanning_flag;           //woody
	
   // If link is ok, this flag is set 1.
	BOOLEAN		media_link_state;      //woody		Joe

	// Current link status (active or disable)
	//wumin 090312 delete, media_link_state_t		link_is_active;       //woody		Joe
	m2bMailbox1_t	       m2bMailbox1;      //woody
	BOOLEAN 			mac_init_done;      //woody        
	BOOLEAN 			bStarveMac;         //woody	
	DSP_ATTR_T_TEST		wlan_attr;

} DSP_ADAPTER_T_TEST, *PDSP_ADAPTER_T_TEST;



// fdeng mask this for debug wlan in Android! 
//
//#pragma pack()

//End  Joe Added for Read Adapter Informaitons. 	2007-08-17
#endif
