#ifndef __DSP_MACCONFIG_H
#define __DSP_MACCONFIG_H
/*******************************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:		DSP_macconfig.h	CURRENT VERSION: 1.00.00
* PURPOSE:		Define some macros, structures and others for mac configuration.
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.		
********************************************************************************/

/*--macros------------------------------------------------------------*/
/*--constants and types------------------------------------------------*/
#include "tdsp_basetypes.h"
#include "precomp.h"

#pragma pack(1)

typedef struct _MAC_CONFIG
{
	UINT32           sw_group_addr   : 1;
	UINT32           bss_type        : 1;
	UINT32           rf_cal_en       : 1;
	UINT32           test_mode_retry : 1;
	UINT32           rx_dtim_en      : 1;
	UINT32           sniffer_en      : 1;
	UINT32           chg_channel     : 1;
	UINT32           encrypt_en       : 1;
	UINT32           wep_exclude     : 1;
	UINT32           gen_timer_en    : 1;
	UINT32           rx_dma_fifo_flush  : 1;
	UINT32           tx_dma_fifo_flush  : 1;
	UINT32           share_key_en    : 1;
	UINT32           mib_cnt_en      : 1;
	UINT32           ack_response_sel  : 1;
	UINT32           tssi_func_en    : 1;
	UINT32           spi_clk_en    : 1;
	UINT32           pa_sel        : 1;
	UINT32           reserved3       : 6;
	UINT32           upload_beacon   :  1;
	UINT32           rf_type_supported  : 3;
	UINT32           join_start      : 1;
	UINT32           join_en         : 1;
	UINT32           mac_en          : 1;
	UINT32           soft_reset      : 1;
} MAC_CONFIG_T, *PMAC_CONFIG_T;

typedef struct _MAC_CONFIG_INFO
{
	UINT32        rf_vendor         : 3;
	UINT32        rf_type           : 5;
	UINT32        g24_pa_reverse    : 1;
	UINT32        g5_pa_reverse     : 1;
	UINT32        rftx_delay        : 3;
	UINT32        rfrx_delay        : 3;
	UINT32        region_domain     : 8;
	UINT32        pa_delay          : 3;
	UINT32        rx_iq_switch      : 1;
	UINT32        tx_iq_switch      : 1;
	UINT32        reserved          : 1;
	UINT32        k32_crystalen     : 1;
	UINT32        ap_enable         : 1;
} MAC_CONFIG_INFO_T, *PMAC_CONFIG_INFO_T;

#if 0
typedef struct _CAPABILITY
{
	UINT16          ess             : 1;
	UINT16         ibss             : 1;
	UINT16         cf_pollable      : 1;
	UINT16          cf_pollreq      : 1;
	UINT16          privacy         : 1;
	UINT16          short_preamble  : 1;
	UINT16            pbcc          : 1;
	UINT16          channel_agility : 1;
	UINT16          reserved1       : 2;
	UINT16          short_slot_time : 1;
    UINT16          reserved2       : 2;
	UINT16          dsss_ofdm	    : 1;
	UINT16          reserved3       : 2;
} CAPABILITY_T, *PCAPABILITY_T;
#endif

typedef struct _IFS
{
	UINT32             dsifs         : 5;
	UINT32             ddifs         : 8;
	UINT32             deifs         : 12;
	UINT32             dslot         : 7;
} IFS_T, *PIFS_T;

typedef struct _RETRY_LIMIT
{
	UINT16      short_retry_limit   : 4;
	UINT16      reserved1           : 4;
	UINT16      long_retry_limit    : 4;
	UINT16      reserved2           : 4;
} RETRY_LIMIT_T, *PRETRY_LIMIT_T;

typedef struct _CW_WIN
{
	UINT16      cw_min              : 5;
	UINT16      reserved1           : 3;
	UINT16      cw_max              : 5;
	UINT16      reserved2           : 3;
} CW_WIN_T, *PCW_WIN_T;

/* This structure defines various counters which is acted as the content of MIB */
typedef struct _MIB_STATISTIC
{
	UINT32    unicast_rx;
	UINT32    unicast_tx;
	UINT32    multicast_rx;
	UINT32    multicast_tx;
	UINT32    broadcast_rx;
	UINT32    broadcast_tx;
	UINT32    total_packet_rx;
	UINT32    total_bytes_rx;
	UINT32    total_packet_tx;
	UINT32    total_bytes_tx;
	UINT32    good_packet_tx;
	UINT32    good_packet_rx;
	UINT32    good_data_packet_rx;
	UINT32    error_packet_tx;
	UINT32    error_packet_rx;
	UINT32    crc_error_rx;
	UINT32    retry_limit_tx;
	UINT32    auth_reject;
	UINT32    auth_timeout;
	UINT32    asoc_reject;
	UINT32    asoc_timeout;
	UINT32    rts_cts_succ;
	UINT32    rts_cts_fail;
	UINT32    ack_fail;
	UINT32    beacon_lost;
	UINT32    wep_error;
	UINT32    atim_tx;
	UINT32    atim_retry_limit_tx;
	UINT32    atim_psq_clr;
} MIB_STATISTIC_T, *PMIB_STATISTIC_T;

typedef struct _MIB_PROPERTY_CNT1
{
	UINT32       ackfailcnt          : 16;
	UINT32       fcsencnt            : 16;
} MIB_PROPERTY_CNT1_T, *PMIB_PROPERTY_CNT1_T;

typedef struct _MIB_PROPERTY_CNT2
{
	UINT32       reserved            : 8;
	UINT32       wepexcnt            : 8;
	UINT32       rtsfailcnt          : 8;
	UINT32       rtssucccnt          : 8;
} MIB_PROPERTY_CNT2_T, *PMIB_PROPERTY_CNT2_T;

typedef struct _WPA_GROUP_KEY
{
	UINT8    key[WLAN_WEP2_TKLEN];
} WPA_GROUP_KEY_T, *PWPA_GROUP_KEY_T;

typedef struct _WPA_TSC_UCHAR
{
	UINT8   tsc[8];
} WPA_TSC_UCHAR_T, *PWPA_TSC_UCHAR_T;

typedef struct _WPA_IV
{
	UINT32       rc4_0            : 8;
	UINT32       rc4_1            : 8;
	UINT32       rc4_2            : 8;
	UINT32       reserved         : 5;
	UINT32       ext_iv           : 1;
	UINT32       key_id           : 2;
} WPA_IV_T, *PWPA_IV_T;

typedef struct _WPA_EIV
{
	UINT32       tsc_2            : 8;
	UINT32       tsc_3            : 8;
	UINT32       tsc_4            : 8;
	UINT32       tsc_5            : 8;
} WPA_EIV_T, *PWPA_EIV_T;

typedef union _WPA_TSC
{
	UINT64 tsc64;
	WPA_TSC_UCHAR_T tsc8;
} WPA_TSC_T, *PWPA_TSC_T;

typedef struct _STA_KEY_SUITE
{
	UINT16   key_valid        : 1;
	UINT16   key_suite_type   : 3;
	UINT16   wep_kmk          : 1;
	UINT16   reserved1        : 3;
	UINT16   key_addr         : 5;
	UINT16   wr_en            : 1;
	UINT16   rd_en            : 1;
	UINT16   reserved2        : 1;
} STA_KEY_SUITE_T, *PSTA_KEY_SUITE_T;

typedef struct _DOT11I_MODE_CONFIG
{
	UINT32 multicast_alg       : 3;
	UINT32 multicast_key_valid : 1;
	UINT32 reserved1           : 3;
	UINT32 dot11i_support      : 1;
	UINT32 tsc_format_l        : 1;
	UINT32 tsc_pn_ini          : 1;
	UINT32 ibss_smode          : 1;
	UINT32 pn_sel              : 1;
	UINT32 support_i31         : 1;
	UINT32 nonce_format        : 1;
	UINT32 tsc_format_set      : 1;
	UINT32 pn_format_set       : 1;
	UINT32 ccm_mask_fc_low     : 8;
	UINT32 ccm_mask_fc_hi      : 8;
} DOT11I_MODE_CONFIG_T, *PDOT11I_MODE_CONFIG_T;

typedef struct _DOT11I_MODE_CONFIG_MULTI
{
	UINT8 multicast_alg       : 3;
	UINT8 multicast_key_valid : 1;
	UINT8 reserved1           : 3;
	UINT8 dot11i_support      : 1;
} DOT11I_MODE_CONFIG_MULTI_T, *PDOT11I_MODE_CONFIG_MULTI_T;

typedef struct _DOT11I_MODE_CONFIG_TSC
{
	UINT8 tsc_format_l        : 1;
	UINT8 tsc_pn_ini          : 1;
	UINT8 ibss_smode          : 1;
	UINT8 pn_sel              : 1;
	UINT8 support_i31         : 1;
	UINT8 nonce_format        : 1;
	UINT8 tsc_format_set      : 1;
	UINT8 pn_format_set       : 1;
} DOT11I_MODE_CONFIG_TSC_T, *PDOT11I_MODE_CONFIG_TSC_T;

typedef struct _BASIC_RATE
{
	UINT16   support_1M       : 1;
	UINT16   support_2M       : 1;
	UINT16   support_55M      : 1;
	UINT16   support_11M      : 1;
	UINT16   support_6M       : 1;
	UINT16   support_9M       : 1;
	UINT16   support_12M      : 1;
	UINT16   support_18M      : 1;
	UINT16   support_24M      : 1;
	UINT16   support_36M      : 1;
	UINT16   support_48M      : 1;
	UINT16   support_54M      : 1;
	UINT16   reserved         : 4;
} BASIC_RATE_T, *PBASIC_RATE_T;

typedef struct _DOT11G_CONFIG
{
	UINT32    dot11g_suppot    : 1;
	UINT32    ofdm_sifs        : 5;
	UINT32    reserved1        : 2;
	UINT32    nonerp_element   : 8;
	UINT32    cts_sifs         : 4;
	UINT32    reserved2        : 12;
} DOT11G_CONFIG_T, *PDOT11G_CONFIG_T;

#pragma pack()

/*--variables---------------------------------------------------------*/
	
/*--function prototypes-----------------------------------------------*/

#endif

