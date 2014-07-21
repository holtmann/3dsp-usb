#ifndef __DSP_WLAN_H
#define __DSP_WLAN_H

#include "tdsp_basetypes.h"
#include "precomp.h"
/*******************************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:		DSP_wlan.h	CURRENT VERSION: 1.00.00
* PURPOSE:		Define common macro, structure and functions related to wlan.
* AUTHORS:		
* NOTES:		this file is common file for different modules.
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.
********************************************************************************/

/*--variables---------------------------------------------------------*/
/*--macros------------------------------------------------------------*/
#define  NUMMAPREGS       (32)   /* Number of map registers */
#define  MAXPHYMAPPINGS		(1536)	/* Max physical mappings */


/* Max group address count */
#define MAXGROUPADDRESS    (8)   /* Count of group address */

#define MAXHOSTGROUPADDRESS  (4)   /* Count of group address */

// #define TIMERDELAY          (200) /* delay 200ms to detect cp, cfp and atim descriptor */

#define TIMERSAFEINTRST     (20) /* 20ms for safe internal reset */

/* Number packets */
// #define NUM_OF_PACKETS     (32)
#ifdef DSP_WIN98
#define NUM_OF_PACKETS     (16)
#else
#define NUM_OF_PACKETS     (64)
#endif

/* Max 802.3 frame size */
#define	RX_BUF_SIZE        (1536)

/*802.3 frame type, dixII or snap??*/
#define WLAN_ENCAP_TYPE_BASE    0x600 // this is from IEEE802.1h ?
/* Max 802.11 ---> 802.3 frame size */
#define MAXRECDATALEN    (2048)

/* Max 802.3 frame size */
#define MAX_ETHER_LEN  0x600   //actually, 802.3 and ethernet frames are less than 1518bytes

/* Max 802.3 ---> 802.11 frame size */
#define MAXTXDATALEN     (2048)

/* Max PSQ size */
#define MAXPSQSIZE       (16)

/* MAX ATIM send retry counts */
#define MAXATIMRETRYS    (5)

/* MAX response time rate */
#define MAXRSPTIMERATE   (4)

/* MAX channel */
#define MAXCHANNEL       (15)
/* MAC descriptor point register*/
#define  CP_TX_DES_POINT    (0x0)
#define  CFP_TX_DES_POINT   (0x10)
#define  ATIM_TX_DES_POINT  (0x20)
//#define  ATIM_UNI_TX_DES_POINT  (0x20)
//#define  ATIM_BRO_TX_DES_POINT  (0x30)
#define  RX_DES_POINT       (0x30)

#define MAX_TX_PACKETS_NUM       (0x4)
/* Context control register */
#define CP_TX_CONTEXT_CONTROL_SET  (0x4)
#define CP_TX_CONTEXT_CONTROL_CLR  (0x8)

#define CFP_TX_CONTEXT_CONTROL_SET  (0x14)
#define CFP_TX_CONTEXT_CONTROL_CLR  (0x18)

//#define ATIM_UNI_TX_CONTEXT_CONTROL_SET  (0x24)
//#define ATIM_UNI_TX_CONTEXT_CONTROL_CLR  (0x28)

//#define ATIM_BRO_TX_CONTEXT_CONTROL_SET  (0x34)
//#define ATIM_BRO_TX_CONTEXT_CONTROL_CLR  (0x38)

#define ATIM_TX_CONTEXT_CONTROL_SET  (0x24)
#define ATIM_TX_CONTEXT_CONTROL_CLR  (0x28)

#define RX_CONTEXT_CONTROL_SET  (0x34)
#define RX_CONTEXT_CONTROL_CLR  (0x38)

/* Tx frame control status register */
#define TX_FRAME_CONTROL        (0x50)

/* SPI control register */
#define SPI_HOST_MAC_CONTROLLER     (0x54)
#define SPI_HOST_MAC_DATA           (0x58)

/* Serial eeprom access register */
#define ROM_HOST_MAC_CONTROLLER     (0x5c)

/* Interrupt  register */
#define INT_EVENT_SET               (0x60)
#define INT_EVENT_CLR               (0x64)

#define INT_MASK_SET                (0x68)
#define INT_MASK_CLR                (0x6c)

/* RF_TR_Timer register */
#define RF_TR_TIMER                 (0x7c)

/* Calibration timer */
#define CALIBRATION_TIMER           (0x80)

/* ADDA control */
#define ADDA_CONTROL                (0x82)

/* Rx_Beacon_Thrsh      */
#define RX_BEACON_THRSH             (0x72)

/* MAC configure register */
#define MAC_CONFIG_SET              (0x40)
#define MAC_CONFIG_CLR              (0x44)
#define MAC_CONFIG_INFO             (0x48)


/* TX life time register */
#define TX_MSDU_LIFE_TIME                (0x84)

/* Tx mode */
#define TX_MODE_BEACON              (0x86)
#define TX_MODE_OTHER               (0x87)

/* Pre tbtt time */
#define PRE_TBTT_TIMER              (0x88)
#define TX_BEACON_LENGTH            (0x8a)
#define TX_PROBE_TIME               (0x8c)

/* Sniffer config */
#define SNIFFER_CONFIG              (0x8e)

/* Rx fragment lift time */
#define RX_FRAGMENT_LIFE_TIME       (0x90)

/* Cw win */
#define CW_WIN                      (0x70)

/* MAC address register */
#define MAC_ADDRESS                 (0x94)

/* STA basic rate */
#define STA_BASIC_RATE              (0x9c)

/* IBSS Atim */
#define IBSS_ATIM                   (0x9e)

/* IFS register */
#define IFS_REG                     (0xa0)

/* ID */
#define SSID_LEN                    (0xa4)
#define SSID_REG                    (0xb0)

#define BSSID_REG                   (0xd0)

#define  AID_REG                    (0xd6)

/* Group address register */
#define GROUP_ADDRESS_0             (0x1a0)
#define GROUP_ADDRESS_1             (0x1a6)
#define GROUP_ADDRESS_2             (0x1ac)
#define GROUP_ADDRESS_3             (0x1b2)

/* Beacon interval register */
#define BEACON_INTERVAL             (0xe0)

/* Capability information register */
#define CAPABILITY_INFORMATION      (0xe2)

/* Supported rate register */
#define SP_RATE_LEN                 (0xe4)
#define SP_RATE                     (0xe6)

/* DS channel number register */
#define DS_CHAN_NUMBER              (0xfe)

/* ATIM interrupt timer */
#define ATIM_INT_TIMER              (0xf2)  

/* WEP key register */
#define WEP_KEY_0                   (0x100)
#define WEP_KEY_1                   (0x110)
#define WEP_KEY_2                   (0x120)
#define WEP_KEY_3                   (0x130)
#define WEP2_TK                     (0x300)

/* TSF register */
#define TSF_REG                     (0x160)

/* Retry limit */
#define RETRY_LIMIT                 (0xf4)

/* LisInv register */
#define LISINV_REG                  (0xf6)

/* Mac info */
#define MAC_INFO                    (0xfe)

/* TBTT timer */
#define TBTT_TIMER                  (0x150)
#define GEN_TIMER                   (0x158)

/* Rx Beacon */
#define RX_BEACON_DELAY_TIMER0      (0x168)
#define RX_BEACON_DELAY_TIMER1      (0x16c)
#define RX_AVER_THRESHOLD           (0x72)

/* Mib */
#define MIB_PROPERTY_COUNTER1       (0x170)
#define MIB_PROPERTY_COUNTER2       (0x174)

/* GPIO */
#define GPIO_OUTPUT_CONTROL         (0x178)
#define GPIO_INPUT_CONTROL          (0x179)

/* Led control */
#define LED_CONTOL0                 (0x17a)
#define LED_CONTOL1                 (0x17b)

/* Dot11i */
#define DOT11I_CONFIG_SET           (0x1b8)
#define DOT11I_CONFIG_CLEAR         (0x1ba)
#define DOT11I_MODE_CONFIG          (0x1bc)

/* Rsn */
#define RSN_INFO                    (0x1c0)

/* Dot11g */
#define DOT11G_CONFIG               (0x1f0)

/* Dot11h */
#define DOT11H_CONFIG               (0x200)

#define COUNTRY_LEN                 (0x204)
#define LOCAL_POWER                 (0x205)
#define IBSS_DFS_LEN                (0x206)
#define BEACON_POWER                (0x207)
#define COUNTRY_INFO                (0x208)
#define IBSS_DFS                    (0x214)
#define BEACON_EVENT                (0x244)
#define BEACON_EVENT_MASK           (0x246)
#define QUEIT_COUNT1                (0x248)
#define QUEIT_PERIOD1               (0x249)
#define QUEIT_DURATION1             (0x24c)
#define QUEIT_OFFSET1               (0x24e)
#define QUEIT_COUNT2                (0x250)
#define QUEIT_PERIOD2               (0x251)
#define QUEIT_DURATION2             (0x254)
#define QUEIT_OFFSET2               (0x256)
#define CHANNEL_SWITCH_MODE         (0x258)
#define NEW_CHANNEL_NUM             (0x259)
#define CHANNEL_SWITCH_COUNT        (0x25a)
#define TBTT_NUM                    (0x25b)
#define RX_CHANNEL_SWITCH_MODE      (0x25c)
#define RX_NEW_CHANNEL_NUM          (0x25d)
#define RX_CHANNEL_SWITCH_COUNT     (0x25e)
#define MEASURE_START_POINT        (0x260)
#define MEASURE_OFFSET             (0x261)
#define TX_BEACON_DELAY_TIMER      (0x263)
#define DUMMY_REG                  (0x264)
#define STA_KEY_SUITE              (0x316)
#define STA_KEY                    (0x300)
#define STA_ADDR                   (0x310)

/* RSPTIME register */
#define RSPTIME_CFG                 (0x78)

/* PMUCTRL register */
#define PMUCTRL_REG                 (0x9c)

/* BEACON_WINDOW    */
#define BEACON_WINDOW               (0x7a)             

/* TestParameterThreshold */
#define TEST_PARAMETER_THRES        (0x74)

/* RF 2958 */
#define GLOBAL_RF_WRITE_CONTROL     (0xfc)
#define GLOBAL_RF_CONFIG_CONTENT    (0xf8)
#define GLOBAL_RF_READ_CONTROL      (0xfd)


#define ISR_Event_PktRXComplete   BIT0
#define ISR_Event_PktTXComplete   BIT1
#define ISR_Event_RxDesFull       BIT2
//#define ISR_Event_FrmTxFail       BIT2
#define ISR_Event_UnrecoverableError   BIT3
//#define ISR_Event_RxBuffFail            BIT4
#define ISR_Event_BeaconSent           BIT4
#define ISR_Event_BeaconChanged        BIT5
#define ISR_Event_PhyRegRdOK           BIT6
#define ISR_Event_SoftInterrupt        BIT7
#define ISR_Event_GpiEvent             BIT8
#define ISR_Event_AtimWMStart          BIT9
#define ISR_Event_TbttUpdate           BIT10
#define ISR_Event_GenTimerInt          BIT11
#define ISR_Event_AcriveReq            BIT12
#define ISR_Event_BeaconLost           BIT13
#define ISR_Event_AckFailCntInt        BIT14
#define ISR_Event_FcsEnCntInt          BIT15
#define ISR_Event_RtsFailCntInt        BIT16
#define ISR_Event_RtsSucCntInt         BIT17
#define ISR_Event_WepProCntInt         BIT18


#define ISR_Event_Beacon_Sent_Ok       BIT0
#define ISR_Event_Beacon_Sent_Fail     BIT1
#define ISR_Event_Auto_Rate_Ok_Frame   BIT2
#define ISR_Event_Auto_Rate_Bad_Frame  BIT3
#define ISR_Event_Exception_Handle     BIT6
#define ISR_Event_WatchDog_En		   BIT7


/* ISR extern event, just event is expressed by the fifth bytes of interrupt event status */
//#define ISR_Ext_Event_Beacon_Com	BIT0
//#define ISR_Ext_Event_Atim_Com		BIT1
//#define ISR_Ext_Event_Beacon_Status	BIT4
//#define ISR_Ext_Event_Atim_Status	BIT5

// #define ISR_Ext_Event_WatchDog_En	(0xff)

     
//#define PACKET_Event_Status            0x00
//#define PACKET_Event_RxDesFull         0x01
//#define PACKET_Icv_Mismatch            0x02
//#define PACKET_Event_Overrun          0x03
//#define PACKET_Event_RxpktOK           0x04
//#define PACKET_Event_RxCrcErr          0x05
//#define PACKET_Event_Descriptor_Read   0x08
//#define PACKET_Event_Descriptor_Write  0x09
//#define PACKET_Event_Data_Read         0x0a
//#define PACKET_Event_Data_Write        0x0b
//#define PACKET_Event_RxBuf_Unavailable 0x0c
//#define PACKET_Event_Underrun          0x11
//#define PACKET_Event_Retry_Timeout     0x12
//#define PACKET_Event_TxMsdu_Timeout    0x13
//#define PACKET_Event_TxpktOk           0x14
//#define PACKET_Event_Fragment_Skip     0x15
//#define PACKET_Event_Phy_Interface_Err 0x16
// #define PACKET_Event_Tx_Timeout        0x17

///////////////context control bits////////
//#define CONTEXT_CONTROL_RUN           (UINT16)BIT8
//#define CONTEXT_CONTROL_WAKE          (UINT16)BIT7
//#define CONTEXT_CONTROL_DEAD          (UINT16)BIT6
//#define CONTEXT_CONTROL_ACTIVE        (UINT16)BIT5

///////////////mac config//////////////////
//#define MACCONFIG_SW_GROUP_ADDR_BYHOST 1
//#define MACCONFIG_SW_GROUP_ADDR_BYHARD 0

//#define MACCONFIG_PCF_SUPPORT          1
//#define MACCONFIG_DCF_SUPPORT          0

/* for 802.11b only */
//#define MACCONFIG_RF_VENDOR_RFMD_2948        0
//#define MACCONFIG_RF_VENDOR_PHILIPS_SA2400   1
//#define MACCONFIG_RF_VENDOR_MAXIM            2
//#define MACCONFIG_RF_VENDOR_GCT              3
//#define MACCONFIG_RF_VENDOR_RFMD_2958        4
//#define MACCONFIG_RF_VENDOR_RALINK           5

/* for 802.11 b/g */
//#define MACCONFIG_RF_VENDOR_RFMD_2959        1
//#define MACCONFIG_RF_VENDOR_EN_303           0
//#define MACCONFIG_RF_VENDOR_AIROHA_2230      2

//#define PHYMODE_MAC_AND_PHY                  0
//#define PHYMODE_MAC_ONLY                     1

//#define MACCONFIG_PHY_BSSTYPE_IBSS     0
//#define MACCONFIG_PHY_BSSTYPE_BSS      1

//#define MACCONFIG_WEP_KEYLEN_128BIT    0
//#define MACCONFIG_WEP_KEYLEN_64BIT     1
     
//#define MACCONFIG_WEP_MODE_WEP         0
//#define MACCONFIG_WEP_MODE_WEP2        1
//#define MACCONFIG_WEP_MODE_NOWEP       2

//#define MACCONFIG_TEST_RETRY_DISABLE   0
//#define MACCONFIG_TEST_RETRY_ENABLE    1

//#define MACCONFIG_LED_TWINKED          0
//#define MACCONFIG_LED_ALWAYS_ON        1

//#define MACCONFIG_SNIFFER_DISABLE      0
//#define MACCONFIG_SNIFFER_ENABLE       1

//#define MACCONFIG_LOOPBACK_DISABLE     0
//#define MACCONFIG_LOOPBACK_ENABLE      1

//#define MACCONFIG_RF_TYPE_11B         0x01
//#define MACCONFIG_RF_TYPE_11G         0x02
//#define MACCONFIG_RF_TYPE_11E         0x04
//#define MACCONFIG_RF_TYPE_11I         0x08
//#define MACCONFIG_RF_TYPE_11A         0x10

//#define MACCONFIG_RF_TYPE_SUPPORTED_RFMD      0
//#define MACCONFIG_RF_TYPE_SUPPORTED_PHILIPS   1
//#define MACCONFIG_RF_TYPE_SUPPORTED_MAXIM     2
//#define MACCONFIG_RF_TYPE_SUPPORTED_GCT       3

//#define MACCONFIG_RF_TYPE_11A_AND_11B  (MACCONFIG_RF_TYPE_11A | MACCONFIG_RF_TYPE_11B)
//#define MACCONFIG_RF_TYPE_11A_AND_11B_AND_11G  (MACCONFIG_RF_TYPE_11A | MACCONFIG_RF_TYPE_11B | MACCONFIG_RF_TYPE_11G)
//#define MACCONFIG_RF_TYPE_11A_AND_11B_AND_11E  (MACCONFIG_RF_TYPE_11A | MACCONFIG_RF_TYPE_11B | MACCONFIG_RF_TYPE_11E)

//#define MACCONFIG_MAC_DISABLE          0
//#define MACCONFIG_MAC_ENABLE           1
//#define MACCONFIG_JOIN_DISABLE         0
//#define MACCONFIG_JOIN_ENABLE          1


/////////////encryption_level////////////////////////////////////// 

#define ENCRY_LEVEL_NONE               0
#define ENCRY_LEVEL_64BIT              1
#define ENCRY_LEVEL_128BIT             2
#define ENCRY_LEVEL_WEP2               3
#define ENCRY_LEVEL_MIC                4

/////////////preamble type///////////////////////////////////////

//#define PREAMBLE_TYPE_LONG             0
//#define PREAMBLE_TYPE_SHORT            1

////////////fallback rate to use/////////////////////////////////////////

#define FALLBACK_RATE_NOUSE                      0
#define FALLBACK_RATE_USE                        1
#define FALLBACK_RATE_PACKET_COUNTS      25

////////////antenna diversity/////////////////////////////////////////

#define ANTENNA_DIVERSITY_DISABLE                      0
#define ANTENNA_DIVERSITY_ENABLE                      1

//////////////rate type//////////////////////////////////////////
#define RATE_TYPE_1M                   2
#define RATE_TYPE_2M                   4
#define RATE_TYPE_55M                  11
#define RATE_TYPE_11M                  22
#define RATE_TYPE_AUTO                 0

#define RATE_TYPE_6M                   12
#define RATE_TYPE_9M                   18
#define RATE_TYPE_12M                  24
#define RATE_TYPE_18M                  36
#define RATE_TYPE_24M                  48
#define RATE_TYPE_36M                  72
#define RATE_TYPE_48M                  96
#define RATE_TYPE_54M                  108

/////////////rate////////////////////////////////////////////////

#define RATE_DSSS_1M                   0
#define RATE_DSSS_LONG_2M              2
#define RATE_DSSS_SHORT_2M             3
#define RATE_CCK_LONG_55M              4
#define RATE_CCK_SHORT_55M             5
#define RATE_CCK_LONG_11M              6
#define RATE_CCK_SHORT_11M             7

#define RATE_6M						   0xb
#define RATE_9M						   0xf
#define RATE_12M					   0xa
#define RATE_18M					   0xe
#define RATE_24M					   0x9
#define RATE_36M					   0xd
#define RATE_48M					   0x8
#define RATE_54M					   0xc

////////////response time rate define//////////////////////////
#define RATE_RSPTIME_AUTO              0
#define RATE_RSPTIME_1M                (RATE_DSSS_1M | BIT3)
#define RATE_RSPTIME_2M				   (RATE_DSSS_LONG_2M | BIT3)
#define RATE_RSPTIME_LONG_55M		   (RATE_CCK_LONG_55M | BIT3)
#define RATE_RSPTIME_SHORT_55M		   (RATE_CCK_SHORT_55M | BIT3)
#define RATE_RSPTIME_LONG_11M		   (RATE_CCK_LONG_11M | BIT3)
#define RATE_RSPTIME_SHORT_11M		   (RATE_CCK_SHORT_11M | BIT3)

//////////////response time rate mode define///////////////////
#define RATE_RSPTIME_MODE_1M           0
#define RATE_RSPTIME_MODE_2M		   1
#define RATE_RSPTIME_MODE_55M		   2
#define RATE_RSPTIME_MODE_11M		   3


////////////wep to use/////////////////////////////////////////

//#define WEP_NOUSE                      0
//#define WEP_USE                        1


///////////key suite type////////////////////////////////////
#define KEY_SUITE_TYPE_NONE            0
#define KEY_SUITE_TYPE_WEP40           1
#define KEY_SUITE_TYPE_TKIP            2
#define KEY_SUITE_TYPE_CCMP            4
#define KEY_SUITE_TYPE_WEP104          5

///////////mode config  type////////////////////////////////////
#define MOD_CONFIG_TYPE_NONE            0
#define MOD_CONFIG_TYPE_WEP40           1
#define MOD_CONFIG_TYPE_TKIP            2
#define MOD_CONFIG_TYPE_WRAP            3
#define MOD_CONFIG_TYPE_CCMP            4
#define MOD_CONFIG_TYPE_WEP104          5

/////////////scan type////////////////////////////////////////

#define SCAN_TYPE_ACTIVE               0
#define SCAN_TYPE_PASSIVE              1

/////////////time out value///////////////////////////////////

#define DEFAULT_AUTH_TIMEOUT           1000 // 500  // 200ms
#define DEFAULT_ASOC_TIMEOUT           800 // 500  // 200ms
#define DEFAULT_JOIN_TIMEOUT           500 //500  // 200ms
#define DEFAULT_SCAN_TIMEOUT           200//5000 // 200ms

#define DEFAULT_SCAN_MIN_TIMEOUT           50
#define DEFAULT_SCAN_MIDDLE_TIMEOUT     100
#define DEFAULT_SCAN_MAX_TIMEOUT           150
/////////////retry limit/////////////////////////////////////

#define DEFAULT_LONG_RETRY_LIMIT        4
#define DEFAULT_SHORT_RETRY_LIMIT       7

//////////// threshold///////////////////////////////////////

#define DEFAULT_MAXFRAGLEN            2346
#define DEFAULT_RTSTHRESHOLD          (DEFAULT_MAXFRAGLEN + 1)

/////////////beacon interval////////////////////////////////

#define DEFAULT_BEACON_INTERVAL        200 // 200ms
#define DEFAULT_ATIM_WINDOW            10  // 10ms

/////////////rx max len/////////////////////////////////////

#define DEFAULT_RX_MAX_LEN             2354

//////////////ifs interval/////////////////////////////////

//#define DEFAULT_DSIFS_INTERVAL         10 
// #define DEFAULT_DDIFS_INTERVAL         42  // original value is 50
//#define DEFAULT_DDIFS_INTERVAL         45  // original value is 50
// #define DEFAULT_DEIFS_INTERVAL         364 
//#define DEFAULT_DEIFS_INTERVAL         359 
//#define DEFAULT_DSLOT_INTERVAL         20 

//////////////ifs interval for short slot time////////////

#define DEFAULT_DSIFS_SHORT_INTERVAL   10 
#define DEFAULT_DDIFS_SHORT_INTERVAL   21  // original value is 28
// #define DEFAULT_DEIFS_SHORT_INTERVAL   342 
#define DEFAULT_DEIFS_SHORT_INTERVAL   359 
#define DEFAULT_DSLOT_SHORT_INTERVAL   9 

////////////sifs interval need writen to mac/////////////
//#define DEFAULT_DSIFS_INTERVAL_REG     5 
// #define DEFAULT_DSIFS_INTERVAL_REG     3 

/////////////listen interval/////////////////////////////
#define DEFAULT_LISTEN_INTERVAL        1

/////////////tx msdu life time////////////////////////////
#define DEFAULT_TX_MSDU_LIFE_TIME       5000

/////////////cw window///////////////////////////////////
#define DEFAULT_CW_MIN                  5
#define DEFAULT_CW_MAX                  10

////////////rx average beacon threshold//////////////////
// #define DEFAULT_AVERAGE_THRESHOLD      1 // 5
#define DEFAULT_AVERAGE_THRESHOLD       20  

/////////////join//////////////////////////////////////////
#define JOIN_HASJOINED                 1
#define JOIN_NOJOINED                  0

////////////power save state///////////////////////////////
//#define PS_ACTIVE                      0
//#define PS_POWERSAVE                   1
//#define PS_POWERSAVE_LOW               2
//#define PS_POWERSAVE_HIGH              3

////////////self to CTS flag///////////////////////////////
#define CTS_SELF_DISABLE                      0
#define CTS_SELF_ENABLE                       1

// 1: 11B Mode   ||
// 2: fixed 11b rate   ||
// 3: no cts self config
#define RTSCTS_STATE_RTS_WORK         	0
// 1: cts self config enable  &&
// 2: ap's beacon ERP enable
// 3: 
#define RTSCTS_STATE_CTSSELF_WORK	1
////////////wlan mode option///////////////////////////////
//#define WLAN_802_11_MODE_11B                  0
//#define WLAN_802_11_MODE_11G                  1
//#define WLAN_802_11_MODE_11A                  2   //Jakio added here

///////////frag threshold list////////////////////////////
#define FRAG_THRESHOLD_0               256
#define FRAG_THRESHOLD_1               512
#define FRAG_THRESHOLD_2               1024


//////////preamble length and plcp header time////////////
#define PREAMBLE_LENGTH_LONG           144
#define PREAMBLE_LENGTH_SHORT          72

#define PLCP_HEADER_TIME_LONG          48
#define PLCP_HEADER_TIME_SHORT         24

//////////some macros for 802.11g//////////////////////
#define T_PREAMBLE                     16
#define T_SIGNAL					   4
#define T_SYM						   4
#define SIGNAL_EXTENTION			   6

/////////ack length//////////////////////////////////////
#define ACK_LENGTH                     14
#define FCS_TABLE_LENGTH         148

/////////link status////////////////////////////////////
//#define LINK_OK                        1
//#define LINK_FALSE                     0

/////////reset type/////////////////////////////////////
#define RESET_TYPE_NOT_SCAN            1
#define RESET_TYPE_WITH_SCAN           0
#define RESET_TYPE_RETRIVE_INFO        2
#define RESET_TYPE_KNOWN_INFO          3
#define RESET_TYPE_DIRECT_DISCONNECT   4
#define RESET_TYPE_DISASSOC            5
#define RESET_TYPE_KNOWN_SSID_SCAN     6
#define RESET_TYPE_KNOWN_INFO_DIFF_SSID  7
#define RESET_TYPE_KNOWN_SSID_SCAN_DIFF_SSID 8
#define RESET_TYPE_DO_JOIN_TILL_BEACON   9
#define RESET_TYPE_DISCONNECT   10
//
#define RESET_TYPE_SSID_OID_REQUEST   11

//Justin: 0730.  for DSP_RESET only
#define RESET_TYPE_SYS_RESET          12

//justin: 071210	
#define RESET_TYPE_KNOWN_BSSID     13


#define RESET_SUB_TYPE_CLEAR_LINK			0x01
#define RESET_SUB_TYPE_NOTIFY_UPLEVEL	0x02
#define RESET_SUB_TYPE_BEGIN_JOIN			0x04
#define RESET_SUB_TYPE_BEGIN_SCAN			0x08
#define RESET_SUB_TYPE_NOT_NOTIFY_UPLEVEL			0x10//justin: 0622



#define SCAN_SUB_STATE_STARVE_MAX_NUM	5

#define SME_WPA_COUNTER_MEASURE_TIMEOUT     60000 //ms
#define SME_WPA_COUNTER_MEASURE_FAILURE_REPORT_WAIT 1000 // 1 sec     

////////Rx buffer t bits////////////////////////////////
#define RXBUF_TBITS_NO_ENCRY           0x0
#define RXBUF_TBITS_RC4                0x1
#define RXBUF_TBITS_TKIP               0x2
#define RXBUF_TBITS_AES                0x3

#define DEFAULT_INTERNAL_RESET_VALUE   3  // 400ms ~ 600ms
/*--constants and types-----------------------------------------------*/
#define WLAN_ETHADDR_LEN	6
#define WLAN_IEEE_OUI_LEN	3

#define WLAN_ETHCONV_ENCAP	1
#define WLAN_ETHCONV_RFC1042	2
#define WLAN_ETHCONV_8021h	3

#define WLAN_MIN_ETHFRM_LEN	60
#define WLAN_MAX_ETHFRM_LEN	1518
#define WLAN_ETHHDR_LEN		16

#define WLAN_MIC_VALUE_COUNT   8

/*--- Sizes -----------------------------------------------*/
#define WLAN_ADDR_LEN			6
#define WLAN_CRC_LEN			4
#define WLAN_BSSID_LEN			6
#define WLAN_BSS_TS_LEN			8
#define WLAN_HDR_A3_LEN			24
#define WLAN_HDR_A4_LEN			30
#define WLAN_SSID_MAXLEN		32
#define WLAN_DATA_MAXLEN		2312
#define WLAN_A3FR_MAXLEN		(WLAN_HDR_A3_LEN + WLAN_DATA_MAXLEN + WLAN_CRC_LEN)
#define WLAN_A4FR_MAXLEN		(WLAN_HDR_A4_LEN + WLAN_DATA_MAXLEN + WLAN_CRC_LEN)
#define WLAN_BEACON_FR_MAXLEN		(WLAN_HDR_A3_LEN + 334)
#define WLAN_ATIM_FR_MAXLEN		(WLAN_HDR_A3_LEN + 0)
#define WLAN_DISASSOC_FR_MAXLEN		(WLAN_HDR_A3_LEN + 2)
#define WLAN_ASSOCREQ_FR_MAXLEN		(WLAN_HDR_A3_LEN + 48)
#define WLAN_ASSOCRESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 16)
#define WLAN_REASSOCREQ_FR_MAXLEN	(WLAN_HDR_A3_LEN + 54)
#define WLAN_REASSOCRESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 16)
#define WLAN_PROBEREQ_FR_MAXLEN		(WLAN_HDR_A3_LEN + 44)
#define WLAN_PROBERESP_FR_MAXLEN	(WLAN_HDR_A3_LEN + 78)
#define WLAN_AUTHEN_FR_MAXLEN		(WLAN_HDR_A3_LEN + 261)
#define WLAN_DEAUTHEN_FR_MAXLEN		(WLAN_HDR_A3_LEN + 2)
#define WLAN_WEP_NKEYS			4
#define WLAN_WEP_MAXKEYLEN		13
#define WLAN_CHALLENGE_IE_LEN		130
#define WLAN_CHALLENGE_LEN		128
#define WLAN_WEP_IV_LEN			4
#define WLAN_WEP_ICV_LEN		4

#define MAX_WEP_KEY_BUFFER_LEN     16
#define MAX_WEP_KEY_NUM     	   	4
#define MAX_KEY_ARRAY_SIZE			64
//index 0 -3 , save tx group key, macadr = null
//index 4 , save tx/rx pairwise key, macadr = ap
//index 5-8 ,save rx group key , macadr = ap
#define STA_KEY_ARRAY_SIZE			9 //6   //first 4 for wep group key ,other 2 for both pairwise and rx gourp key

#define REAL_KEY_ARRAY_SIZE		STA_KEY_ARRAY_SIZE
//#define WLAN_MAX_RATE_SUPPORT_LEN     12 //8
#define WLAN_MAX_RATE_SUPPORT_LEN     15 //8			// mark by zyy, it would be 12?? 1,2,5.5,11,6,9,12,18,24,36,48,54
#define WLAN_WEP2_TKLEN               16
#define WLAN_MIC_KEY_LEN              8
#define WLAN_ACK_LEN                      14
#define WLAN_CTS_LEN                      14

#define WLAN_MAX_EXT_RATE_SUPPORT_LEN     255

#define WLAN_WPA_MAX_GROUP            4

#define WLAN_TSF_LEN                  8
//#define WLAN_MAX_CHANNEL_LIST         16		// must update latter, maybe 14(2g) + 34(5g, up to ch216). zyy
#define WLAN_MAX_CHANNEL_LIST         48		// must update latter, maybe 14(2g) + 34(5g, up to ch216). Justin
#define WLAN_TBTT_TIMER_LEN           7
#define WLAN_GENERAL_TIMER_LEN        3
#define WLAN_FRAG_HOLD_MAXLEN         6

/*--- Frame Control Field -------------------------------------*/
/* Frame Types */
#define WLAN_FTYPE_MGMT			0x00
#define WLAN_FTYPE_CTL			0x01
#define WLAN_FTYPE_DATA			0x02
#define WLAN_FTYPE_RESERVE		0x03

/* Frame subtypes */
/* Management */
#define WLAN_FSTYPE_ASSOCREQ		0x00
#define WLAN_FSTYPE_ASSOCRESP		0x01
#define WLAN_FSTYPE_REASSOCREQ		0x02
#define WLAN_FSTYPE_REASSOCRESP		0x03
#define WLAN_FSTYPE_PROBEREQ		0x04 
#define WLAN_FSTYPE_PROBERESP		0x05
#define WLAN_FSTYPE_BEACON		0x08
#define WLAN_FSTYPE_ATIM		0x09
#define WLAN_FSTYPE_DISASSOC		0x0a
#define WLAN_FSTYPE_AUTHEN		0x0b
#define WLAN_FSTYPE_DEAUTHEN		0x0c

/* Control */
#define WLAN_FSTYPE_PSPOLL		0x0a
#define WLAN_FSTYPE_RTS			0x0b
#define WLAN_FSTYPE_CTS			0x0c
#define WLAN_FSTYPE_ACK			0x0d
#define WLAN_FSTYPE_CFEND		0x0e
#define WLAN_FSTYPE_CFENDCFACK		0x0f

/* Data */
#define WLAN_FSTYPE_DATAONLY		0x00
#define WLAN_FSTYPE_DATA_CFACK		0x01
#define WLAN_FSTYPE_DATA_CFPOLL		0x02
#define WLAN_FSTYPE_DATA_CFACK_CFPOLL	0x03
#define WLAN_FSTYPE_NULL		0x04
#define WLAN_FSTYPE_CFACK		0x05
#define WLAN_FSTYPE_CFPOLL		0x06
#define WLAN_FSTYPE_CFACK_CFPOLL	0x07

#define WLAN_FSTYPE_DATA_802_1X		0x08	//Jakio20070706: add for wpa2


#define WLAN_MAXFRAGLEN        2048

#define WLAN_POWERSAVESTATE     1
#define WLAN_ACTIVESTATE        0
#define WLAN_NOLINKSTATE        2


//liu
#define WLAN_8021X_KEY_LEN      80

//Added by Joe mlme support.2007-11-16

/*Following defines can be used by station and ap */
#define ESS_SUPPORT					0x0001
#define	IBSS_SUPPORT				0x0002

/*Following defines can be used by station and ap */
#define	PRIVACY_SUPPORT				0x0010

#define SHORT_PREAMBLE				0x0020
#define PBCC						0x0040
#define CHANNEL_AGILITY				0x0080
#define SHORT_SLOT_TIME				0x0400
#define LONG_SLOT_TIME				0xFBFF
#define DSSS_OFDM					0x2000
//End


/*================================================================*/
/*Hardware bits*/

#define _3DSP_HWHDR_BIT_RETRY_ENABLE        		BIT0
#define _3DSP_HWHDR_BIT_NEW_SEQ_NUM         	BIT1
#define _3DSP_HWHDR_BIT_TX_FCS_ERR          		BIT2
#define _3DSP_HWHDR_BIT_SHORT_PREAMBLE      	BIT3
#define _3DSP_HWHDR_BIT_TX_PWR_LVL         		(BIT4|BIT5)
#define _3DSP_HWHDR_POS_TX_PWR_LVL                     4
#define _3DSP_HWHDR_BIT_TX_CTS_SELF        		BIT6
#define _3DSP_HWHDR_BIT_MODULATION          		BIT7 
#define _3DSP_HWHDR_BIT_IGNORE_BYTES     	  	(BIT8|BIT9)
#define _3DSP_HWHDR_POS_IGNORE_BYTES        		8
#define _3DSP_HWHDR_BIT_INT_ON_TX           		BIT10
#define _3DSP_HWHDR_BIT_WR_ACK              		BIT11
#define _3DSP_HWHDR_BIT_DONT_EXPECT_ACK     	BIT12
#define _3DSP_HWHDR_BIT_PRIVACY             		BIT13
#define _3DSP_HWHDR_BIT_DEFAULT_KEY_ID   	  	(BIT14| BIT15)
#define _3DSP_HWHDR_POS_DEFAULT_KEY_ID  	    	14
#define _3DSP_HWHDR_BIT_TX_SPEED           		(BIT16| BIT17 | BIT18| BIT19)
#define _3DSP_HWHDR_POS_TX_SPEED            		16
#define _3DSP_HWHDR_BIT_TX_LENGTH           		0xFFF00000
#define _3DSP_HWHDR_POS_TX_LENGTH           		20
#define _3DSP_HWHDR_BIT_CURR_FRAG_DUR    		0x0000FFFF
#define _3DSP_HWHDR_POS_FRM_CTRL            		16
#define _3DSP_HWHDR_BIT_FRM_CTRL            		0xFFFF0000
#define _3DSP_HWHDR_POS_SEQ_CTRL            		16
#define _3DSP_HWHDR_BIT_SEQ_CTRL            		0xFFFF0000
#define _3DSP_HWHDR_BIT_RSSI                			0x0000FFFF
/*================================================================*/
/* Macros */

/*--- FC Macros ----------------------------------------------*/
/* Macros to get/set the bitfields of the Frame Control Field */
/*  GET_FC_??? - takes the host byte-order value of an FC     */
/*               and retrieves the value of one of the        */
/*               bitfields and moves that value so its lsb is */
/*               in bit 0.                                    */
/*  SET_FC_??? - takes a host order value for one of the FC   */
/*               bitfields and moves it to the proper bit     */
/*               location for ORing into a host order FC.     */
/*               To send the FC produced from SET_FC_???,     */
/*               one must put the bytes in IEEE order.        */
/*  e.g.                                                      */
/*     printf("the frame subtype is %x",                      */
/*                 GET_FC_FTYPE( ieee2host( rx.fc )))         */
/*                                                            */
/*     tx.fc = host2ieee( SET_FC_FTYPE(WLAN_FTYP_CTL) |       */
/*                        SET_FC_FSTYPE(WLAN_FSTYPE_RTS) );   */
/*------------------------------------------------------------*/

#define WLAN_GET_FC_PVER(n)	 (((UINT16)(n)) & (BIT0 | BIT1))
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

#define WLAN_FC_WEP_BIT               0x4000
#define WLAN_FC_WEP_POS               14
#define WLAN_FC_BIT_VERSION      0x0003
#define WLAN_FC_BIT_TYPE         0x000C
#define WLAN_FC_POS_TYPE         2
#define WLAN_FC_BIT_SUBTYPE      0x00F0
#define WLAN_FC_POS_SUBTYPE      4
#define WLAN_FC_BIT_TO_DS        0x0100
#define WLAN_FC_BIT_FROM_DS      0x0200
#define WLAN_FC_BIT_MORE_FRAG    0x0400
#define WLAN_FC_BIT_RETRY        0x0800
#define WLAN_FC_BIT_PWR_MGMT     0x1000
#define WLAN_FC_BIT_MORE_DATA    0x2000
#define WLAN_FC_BIT_PRIVACY      0x4000
#define WLAN_FC_BIT_ORDER        0x8000

#define WLAN_CAP_BIT_ESS  					0x0001
#define WLAN_CAP_BIT_IBSS 					0x0002
#define WLAN_CAP_BIT_PRIVACY				0x0010
#define WLAN_CAP_BIT_SHORT_PREAMBLE		0x0020
#define WLAN_CAP_BIT_SHORT_SLOTTIME		0x0400



#define DOT11_FRM_ADDR_4_LEN				6
#define DOT11_FRM_FCS_LEN					4

#define DOT11_FRM_TYPE_MGMT                 0
#define DOT11_FRM_SUBTYPE_ASSOC_REQ         0x0
#define DOT11_FRM_SUBTYPE_ASSOC_RESP        0x1
#define DOT11_FRM_SUBTYPE_REASSOC_REQ       0x2
#define DOT11_FRM_SUBTYPE_REASSOC_RESP      0x3
#define DOT11_FRM_SUBTYPE_PROBE_REQ         0x4
#define DOT11_FRM_SUBTYPE_PROBE_RESP        0x5
#define DOT11_FRM_SUBTYPE_BEACON            0x8
#define DOT11_FRM_SUBTYPE_ATIM              0x9
#define DOT11_FRM_SUBTYPE_DISASSOC          0xA
#define DOT11_FRM_SUBTYPE_AUTH              0xB
#define DOT11_FRM_SUBTYPE_DEAUTH            0xC

#define DOT11_FRM_TYPE_CTRL                 1
#define DOT11_FRM_SUBTYPE_PS_POLL           0xA

#define DOT11_FRM_TYPE_DATA                 2
#define DOT11_FRM_SUBTYPE_DATA              0x0
#define DOT11_FRM_SUBTYPE_NULL_FUNC         0x4

#define DOT11_SEQ_CTRL_LEN					2
#define DOT11_SSID_MAX_LEN					32
#define DOT11_TIMESTAMP_LEN					8

#define DOT11_FRM_WEP_BIT					0x4000
#define DOT11_FRM_WEP_POS					14
#define DOT11_FRM_CTRL_BIT_VERSION			0x0003
#define DOT11_FRM_CTRL_BIT_TYPE				0x000C
#define DOT11_FRM_CTRL_POS_TYPE				2
#define DOT11_FRM_CTRL_BIT_SUBTYPE			0x00F0
#define DOT11_FRM_CTRL_POS_SUBTYPE			4
#define DOT11_FRM_CTRL_BIT_TO_DS			0x0100
#define DOT11_FRM_CTRL_BIT_FROM_DS			0x0200
#define DOT11_FRM_CTRL_BIT_MORE_FRAG		0x0400
#define DOT11_FRM_CTRL_BIT_RETRY			0x0800
#define DOT11_FRM_CTRL_BIT_PWR_MGMT			0x1000
#define DOT11_FRM_CTRL_BIT_MORE_DATA		0x2000
#define DOT11_FRM_CTRL_BIT_PRIVACY			0x4000
#define DOT11_FRM_CTRL_BIT_ORDER			0x8000
/*--- Duration Macros ----------------------------------------*/
/* Macros to get/set the bitfields of the Duration Field      */
/*  - the duration value is only valid when bit15 is zero     */
/*  - the firmware handles these values, so I'm not going     */
/*    these macros right now.                                 */
/*------------------------------------------------------------*/

/*--- Sequence Control  Macros -------------------------------*/
/* Macros to get/set the bitfields of the Sequence Control    */
/* Field.                                                     */
/*------------------------------------------------------------*/
#define WLAN_GET_SEQ_FRGNUM(n) (((UINT16)(n)) & (BIT0|BIT1|BIT2|BIT3))
#define WLAN_GET_SEQ_SEQNUM(n) ((((UINT16)(n)) & (~(BIT0|BIT1|BIT2|BIT3))) >> 4) 

/*--- Data ptr macro -----------------------------------------*/
/* Creates a UINT8* to the data portion of a frame            */
/* Assumes you're passing in a ptr to the beginning of the hdr*/
/*------------------------------------------------------------*/
#define WLAN_HDR_A3_DATAP(p) (((UINT8*)(p)) + WLAN_HDR_A3_LEN)
#define WLAN_HDR_A4_DATAP(p) (((UINT8*)(p)) + WLAN_HDR_A4_LEN)


#define wlan_max(a, b) (((a) > (b)) ? (a) : (b))
#define wlan_min(a, b) (((a) < (b)) ? (a) : (b))

#define WLAN_KVERSION(a,b,c)	(((a)<<16)+((b)<<8)+(c))

#define wlan_isprint(c)	(((c) > (0x19)) && ((c) < (0x7f)))

#define wlan_hexchar(x) (((x) < 0x0a) ? ('0' + (x)) : ('a' + ((x) - 0x0a)))

/* for four bytes' alignning. */
#define  ALIGNDATALEN(x)   (UINT16)(((UINT16)x + 3) & 0xfffc)
#define  ALIGNLONGDATALEN(x)   (UINT32)(((UINT32)x + 3) & 0xfffffffc)

/* for getting the details from the head of receiving raw frame. */
#define  RXBUF_GET_RXDATALEN(x)    (UINT16)(((UINT32)x & 0xfff00000) >> 20)
#define  RXBUF_GET_RXDATARATE(x)   (UINT8)((((UINT32)x & 0xf00) >> 8))

//v4chip
#define  RXBUF_GET_RXSWAPLEN(x)    (UINT16)((((UINT32)x & 0xff) << 4) + (((UINT32)x & 0xf000) >> 12))
#define  RXBUF_GET_RXSWAPRATE(x)   (UINT8)(((UINT32)x & 0x000f0000) >> 16)


#define  RXBUF_GET_RSSI(x)         (UINT8)(((UINT32)x & 0x000000ff))// one byte
//v4chip
#define  RXBUF_IS_RX_MAGIC(x)         (((UINT32)x & 0xffffff00) == 0xDEDBEF00)// one byte
//#define  RXBUF_GET_RXTBTTNUM(x)    (UINT8)(((UINT32)x & 0x000ff000) >> 12)
//#define  RXBUF_GET_TIMESTAMP(x)    (UINT16)((UINT32)x & 0x0000ffff)
//#define  RXBUF_GET_XFERSTATUS(x)   (UINT16)(((UINT32)x & 0xffff0000) >> 16)

#define MAC_ADDR_IS_GROUP(addr) (addr[0]&0x1)
#if 0
#define MAC_ADDR_IS_BCAST(addr) (addr[0] == 0xff && addr[1] == 0xff && addr[2] == 0xff && \
		addr[3] == 0xff && addr[4] == 0xff && addr[5] == 0xff) 
#endif
#define CAP_EXPRESS_BSS(cap) 				((cap & WLAN_CAP_BIT_ESS) != 0)
#define CAP_EXPRESS_IBSS(cap) 				((cap & WLAN_CAP_BIT_IBSS) != 0)
#define CAP_EXPRESS_PRIVACY(cap) 			((cap & WLAN_CAP_BIT_PRIVACY) != 0)
#define CAP_EXPRESS_SHORT_PREAMBLE(cap) 	((cap & WLAN_CAP_BIT_SHORT_PREAMBLE) != 0)
#define CAP_EXPRESS_SHORT_SLOTTIME(cap)	((cap & WLAN_CAP_BIT_SHORT_SLOTTIME) != 0)
#define DOT11_FRM_IS_WEP(frm_ctrl)\
	((frm_ctrl) & DOT11_FRM_WEP_BIT != 0)

#define DOT11_FRM_TYPE(frm_ctrl) \
   (((frm_ctrl) & DOT11_FRM_CTRL_BIT_TYPE) \
    >> DOT11_FRM_CTRL_POS_TYPE)   


#define DOT11_FRM_SUBTYPE(frm_ctrl) \
    (((frm_ctrl) & DOT11_FRM_CTRL_BIT_SUBTYPE) \
    >> DOT11_FRM_CTRL_POS_SUBTYPE)

#define DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) \
    (DOT11_FRM_TYPE(frm_ctrl) == DOT11_FRM_TYPE_MGMT)
#define DOT11_FRM_IS_MGMT     DOT11_FRM_TYPE_IS_MGMT

#define DOT11_FRM_IS_PROBE_REQ(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) && \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_PROBE_REQ))

#define DOT11_FRM_IS_PROBE_RESP(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_PROBE_RESP))

#define DOT11_FRM_IS_AUTH(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&   \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_AUTH))

#define DOT11_FRM_IS_DEAUTH(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&    \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_DEAUTH))


#define DOT11_FRM_IS_BEACON(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&     \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_BEACON))

#define DOT11_FRM_IS_ASSOC_REQ(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_ASSOC_REQ))

#define DOT11_FRM_IS_REASSOC_REQ(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_REASSOC_REQ))

#define DOT11_FRM_IS_ASSOC_RESP(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_ASSOC_RESP))

#define DOT11_FRM_IS_REASSOC_RESP(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_REASSOC_RESP))

#define DOT11_FRM_IS_DISASSOC(frm_ctrl) \
    (DOT11_FRM_TYPE_IS_MGMT(frm_ctrl) &&  \
    (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_DISASSOC))



#define DOT11_FRM_TYPE_IS_DATA(frm_ctrl) \
    (DOT11_FRM_TYPE(frm_ctrl) == DOT11_FRM_TYPE_DATA) 

#define DOT11_FRM_TYPE_IS_CTRL(frm_ctrl) \
    (DOT11_FRM_TYPE(frm_ctrl) == DOT11_FRM_TYPE_CTRL) 

#define DOT11_FRM_IS_DATA(frm_ctrl) \
    ((DOT11_FRM_TYPE(frm_ctrl) == DOT11_FRM_TYPE_DATA) &&   \
     (DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_DATA))

// NULL_FUNC frame is null data frame
#define DOT11_FRM_IS_NULL_FUNC(frm_ctrl) \
	((DOT11_FRM_TYPE(frm_ctrl) == DOT11_FRM_TYPE_DATA) && \
	(DOT11_FRM_SUBTYPE(frm_ctrl) == DOT11_FRM_SUBTYPE_NULL_FUNC))

#define DOT11_FRM_TO_DS(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_TO_DS) != 0)

#define DOT11_FRM_FROM_DS(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_FROM_DS) != 0)

#define DOT11_FRM_MORE_FRAG(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_MORE_FRAG) != 0)

#define DOT11_FRM_RETRY(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_RETRY) != 0)

#define DOT11_FRM_PWR_MGMT(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_PWR_MGMT) != 0)

#define DOT11_FRM_MORE_DATA(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_MORE_DATA) != 0)

#define DOT11_FRM_PRIVACY(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_PRIVACY) != 0)

#define DOT11_FRM_ORDER(frm_ctrl) \
    ((frm_ctrl & DOT11_FRM_CTRL_BIT_ORDER) != 0)


#ifdef ANTENNA_DIVERSITY
#define DSP_CARD_IS_MINICARD(id) \
    ((id & BIT8) == 0)
#endif
//
#define SSID_LENGTH_VALID(len) \
	((len > 0) && (len <= WLAN_SSID_MAXLEN))

#define SUPPORT_RATE_LENGTH_VALID(len) \
	((len > 0) && (len <= WLAN_MAX_RATE_SUPPORT_LEN))

#define IS_MATCH_IN_ESSMODE(macmode,cap) \
	((macmode == WLAN_MACMODE_ESS_STA && CAP_EXPRESS_BSS(cap)) || \
 	  (macmode == WLAN_MACMODE_IBSS_STA && CAP_EXPRESS_IBSS(cap)))

#define IS_MATCH_IN_SSID(ele1,len1,data1,ele2,len2,data2) \
	((ele1 == ele2) &&	\
	 (len1 == len2)  &&		\
	 (0 == sc_memory_cmp(data1,data2,len1)))
	 
#define IS_MATCH_IN_ADDRESS(dst,src) \
	(0 == sc_memory_cmp(dst,src,ETH_LENGTH_OF_ADDRESS))

#define WLAN_COPY_ADDRESS(dst,src) \
	(sc_memory_copy(dst,src,ETH_LENGTH_OF_ADDRESS))
#define GetRssi(r)  (r & 0x7f)
#define GetRssiMsb(r)  (r & 0x80)
/*hardware  frame struct*/
//#pragma pack(1)

// fdeng mask this !
//
//#pragma pack(push,1)


typedef struct USBWLAN_HW_FRMHDR_TX {
    UINT32 len_speed_ctrl;
    UINT32 frmctrl_fragdur;
    UINT32 addr1lo_txdurid;
    UINT32 addr1hi;
    UINT32 addr3lo;
    UINT32 seqctrl_addr3hi;
} USBWLAN_HW_FRMHDR_TX, *PUSBWLAN_HW_FRMHDR_TX;
typedef struct _wpakey_info_tag
{
	BOOLEAN						in_use;    //ind the key has mapped into hw
	BOOLEAN						WEP_on;  //ind the key value is valid
	UINT8						mac_address[WLAN_ADDR_LEN];
	UINT8						key[MAX_WEP_KEY_BUFFER_LEN];
	UINT8						key_len_in_bytes;
	UINT8 					    keyid;
	UINT32                      cyper;   //no use here, get the featuren by encryption mode
	BOOLEAN                     multicast; //
	UINT32                     	index;
}wpakey_info_t;

typedef struct _KEY_CNTL_ADDR
{
	UINT32         hi_mac_addr:16;
	UINT32 	     keyvalid:1;
	UINT32         ciphertype:2;
	UINT32         keyid:2;
	UINT32 	     keyindex:9;
	UINT32 	     xcastKey:1;
	UINT32 	     entryNotDone:1;
} KEY_CNTL_ADDR_T, *PKEY_CNTL_ADDR_T;


//Jakio20070426: mac_addr should be 4bytes array, the highest 16bits are stored in key_cntl register
typedef struct _KEY_MLME_REG
{
	UINT32           key0_31;                      //90
	UINT32           key32_63;                    //94
	UINT32           key64_95;                    //98
	UINT32           key96_127;                 //9c
	UINT8		mac_addr[WLAN_ETHADDR_LEN - 2];      //a0
	//UINT8		mac_addr[WLAN_ETHADDR_LEN];      //a0
	KEY_CNTL_ADDR_T   key_cntl;		//a4
	UINT32           key_size_cntl;		//a8
} KEY_MLME_REG_T, *PKEY_MLME_REG_T;
typedef struct _RESET_TYPE
{
	UINT8           type;
	UINT32           sub_type;
} RESET_TYPE_T, *P_RESET_TYPE_T;

typedef struct _BRS11GOFDM_REG
{
	UINT32         ofdm_rate:8;
	UINT32 	     ofdm_rts_rate:4;
	UINT32         dsss_rts_rate:2;
	UINT32         ctstoself:1;
	UINT32 	     reserved:17;
} BRS11GOFDM_REG_T, *PBRS11GOFDM_REG_T;

typedef struct _BBREG2023_REG
{
	UINT32         bbreg20:8;
	UINT32 	     bbreg21:8;
	UINT32         bbreg22:8;
	UINT32         bbreg23_left:5;
	UINT32         soft_doze:1;
	UINT32         ap_associated:1;
	UINT32         auto_corr:1;
} BBREG2023_REG_T, *PBBREG2023_REG_T;

#if 1		// fdeng@20100422,	It seemed that no one use this !!!

//fdeng
#pragma pack(push,1)


//Struct of SP20 code file in the static buffer
typedef struct _SP20_BIN_CODE {
	UINT32 Size;		// Size of the bin resource file
	UINT32 Port;		// SP20 mem port index
	UINT8   Code[1];	// SP20 code - copied from bin resource
} SP20_BIN_CODE, *PSP20_BIN_CODE;

// 090702 glen added for extend card type support
typedef struct {
	UINT32 m_Mark;	// mark
	UINT32 Size;		// Size of support cardtypes
	UINT32 m_dwID[1];	// cardID
} SP20_SUPPORT_CODETABLE, *PSP20_SUPPORT_CODETABLE;

typedef struct  
{
	UINT32 m_dwID;		// cardID = CodeId+cardID 
	UINT32 m_dwOffset;	// code offset
	UINT32 m_dwLen;		// code Len
	UINT32 m_dwPortBase;		// code portbase
} SP20_BIN_CODE_TABLE,*PSP20_BIN_CODE_TABLE ;

// 090702 glen added for extend card type support end
typedef struct _SP20_FILE_HEAD
{
	UINT8          file_name[4];			// 00h - 03h	File Type Mark, Must be string "SP20"
	UINT32 	     bin_len;				// 04h - 07h	Bin File Data Length, not include this FileHead
	UINT32         file_timestamp;  	  	// 08h - 0Bh	File timestamp
	UINT32         bin_checksum;		// 0Ch - 0Fh	Bin Data checksum
	UINT32         reserve1[4];			// 10h - 1Fh	Reserve space
	UINT32         wlanonly_offset;  		// 20h - 23h	WLAN only DSP code data offset address
	UINT32         wlanonly_len;			// 24h - 27h	WLAN only DSP code data Length
	UINT32         btonly_offset;  		// 28h - 2Bh	BT only DSP code data offset address
	UINT32         btonly_len;			// 2Ch - 2Fh	BT only DSP code data Length
	UINT32         combo_offset;  		// 30h -33h	COMBO DSP code data offset address
	UINT32         combo_len;			// 34h -37h	COMBO DSP code data Length
	UINT32         eeprom_offset;  		// 38h - 3Bh	EEPROM access DSP code data offset address
	UINT32         eeprom_len;			// 3Ch - 3Fh	EEPROM access DSP code data Length
	UINT32         reserve2[4];			// 40h - 4Fh	Reserve space
	UINT32         wlan8051_offset;		// 50h - 53h	WLAN only 8051 code data offset address
	UINT32         wlan8051_len;		// 54h - 57h	WLAN only 8051 code data Length
	UINT32         bt8051_offset;  		// 58h - 5Bh	BT only DSP 8051 data offset address
	UINT32         bt8051_len;			// 5Ch - 5Fh	BT only DSP 8051 data Length
	UINT32         combo8051_offset;  	// 60h -63h	COMBO DSP 8051 data offset address
	UINT32         combo8051_len;		// 64h -67h	COMBO DSP 8051 data Length
	UINT32         reserve3[2];			// 68h - 7Fh	Reserve space
	UINT8           bin_data[1];			// 80H~	WLAN only DSP code data region
									// BT only DSP code data region
									// COMBO DSP code data region
									// EEPROM access DSP code data region
									//  WLAN only 8051 code data region 
									// BT only 8051 code data region
									// COMBO 8051 code data region
} SP20_FILE_HEAD_T, *PSP20_FILE_HEAD_T;


#pragma pack(pop)


#endif



/*--variables---------------------------------------------------------*/


/*
 * User configurable Macros
 */
// #define FIFOS							3		/* cp/cfp/prio */
 #define FIFOS							4		/* cp/cfp/prio/bcn */
 #define TUPLE_SIZE						32		/* 32/64 */
 #define HEADER_NODES					64		/* 20 */		
												/* 32/64 */
 #define DATA_NODES						HEADER_NODES		
												/* 32/64/128/256 */
 #define MSDU_LIST_SIZE					32		/* 32/64 */
 #define TX_BLOCK_SIZE					32		/* 32/64/128 */
 #define DATA_NODE_LENGTH	   			1536	    /* 256/512/1024/2048/2340 */
 #define CP_FIFO_SIZE					3456	/* interms of 64 alignment */
 #define CFP_FIFO_SIZE					3456	/* Interms of 64 alignment */
 #define BCN_FIFO_SIZE					384		/* interms of 64 alignment */
 #define ATIM_FIFO_SIZE					768		/* interms of 64 alignment */
 #define BSS_INTERVAL					4		/* Interms of beacon interval */
 #define NUM_OF_SUPPORTED_STATIONS		16		/* 32number of supported stations */
 #define COUNTER_NODES					8		/* 32/64 */
 #define PS_RETRY_LIMIT					1		/* ps msdu retry limit			*/
 #define TOTAL_TX_FIFO_SIZE				(12 * 1024)	/* Interms of bytes.for ap > 10000 is reqd	*/
 #define AGING_TIME						2000 	/* FIXME */
 #define LIFE_TIME						200000 	/* FIXME */
 #define INACTIVITY_TIME				0x6B49D200 	/* FIXME */
 #define MSDU_THSLD						2304
 #define FAST_DOZE_PERIOD				4096
 /*for rate/length switching*/
 #define RATE_INACTIVE_TIME				600000000		//60 secs in 100ns interval
 #define MIN_MPDU_CNT_TO_DEC			3
 #define MIN_MPDU_CNT_TO_INC			5
 #define ACK_FAIL_TO_MPDU_RATIO_DOWN	2				//average 2 ack failures for every MPDU
 #define ACK_FAIL_TO_MPDU_RATIO_UP		(MIN_MPDU_CNT_TO_INC / 4) //25% (scaled)
 #define LARGE_PKT_THRESHOLD			500				//bytes
 #define NUM_ENTRIES_PER_RATE			3
 #define NUM_AUTORATES_A				7
 #define NUM_AUTORATES_G				11
 #define NUM_AUTORATES_B				4
 #define RATE_UP						WLAN_TRUE
 #define RATE_DOWN						WLAN_FALSE
 #define MAX_LEN_TH						2344			//bytes (not 2346 b/c our card needs it to be word-aligned)
 #define MED_LEN_TH						400				//bytes
 #define MIN_LEN_TH						256				//bytes
 #define IDX_54MBPS_BG					11
 #define IDX_48MBPS_BG					10
 #define IDX_36MBPS_BG					9
 #define IDX_24MBPS_BG					8
 #define IDX_18MBPS_BG					7
 #define IDX_12MBPS_BG					6
 #define IDX_9MBPS_BG					5
 #define IDX_6MBPS_BG					4
 #define IDX_11MBPS_BG					3
 #define IDX_5MBPS_BG					2
 #define IDX_2MBPS_BG					1
 #define IDX_1MBPS_BG					0
 #define BG_TO_A_IDX_OFFSET				4
 #define IDX_54MBPS_A					IDX_54MBPS_BG - BG_TO_A_IDX_OFFSET //7
 #define IDX_48MBPS_A					IDX_48MBPS_BG - BG_TO_A_IDX_OFFSET //6
 #define IDX_36MBPS_A					IDX_36MBPS_BG - BG_TO_A_IDX_OFFSET //5
 #define IDX_24MBPS_A					IDX_24MBPS_BG - BG_TO_A_IDX_OFFSET //4
 #define IDX_18MBPS_A					IDX_18MBPS_BG - BG_TO_A_IDX_OFFSET //3
 #define IDX_12MBPS_A					IDX_12MBPS_BG - BG_TO_A_IDX_OFFSET //2
 #define IDX_9MBPS_A					IDX_9MBPS_BG - BG_TO_A_IDX_OFFSET //1
 #define IDX_6MBPS_A					IDX_6MBPS_BG - BG_TO_A_IDX_OFFSET //0
 #define MAX_FAIL_MULT					8
 /*Jakio20070518: changed the value from 0x7ff to 0x7fe*/
 #define CARDBUS_NUM_OF_RETRY           0x7FE            // used to configure number_of_retry reg; 16-bit value
 #define BSS_DESC_SIZE                  2048
 #define LISTS_SIZE						50
 #define FAILMULT_VAL					10
 #define LASTFEW_IDX					5
 #define RSSI_INC_TH					10
 #define RSSI_ARRAY_SIZE				10
 #define AUTO_CORR_OFF_RSSI_HIGH_TH		0x24 //equivalent to -66.4db
 #define AUTO_CORR_OFF_RSSI_LOW_TH		0x22 //equivalent to -68.8db
 #define RETRY_TIME_THRESHOLD			(100000 /*=10ms*/ * 10)  //in 100ns interval
 #define RX_CORRUPT_MAX_CNT				5
 #define AUTO_CORR_OFF_RSSI_HIGH_TH_V4	0x32 //equivalent to -49.6dBm
 #define AUTO_CORR_OFF_RSSI_LOW_TH_V4	0x2e //equivalent to -54.4dBm
 #define AUTO_RATE_ADJUST_MIN_RSSI_THRESHOLD 0x27//equivalent to -62.8dBm
 /*for RTS/CTS protection in MixedBG environment*/
 #define PKT_PROTECT_LMT				2
 #define RTS_THRESHOLD_WHEN_PRTCTD		24	
 #define RTS_RETRY_LMT_WHEN_PRTCTD      255 //to lessen the effect of the hardware bug #345	


#define  DSP_USB_TX_FW_PACKET                    	       1
#define  DSP_USB_TX_MLME_PACKET             		2
#define  DSP_USB_TX_DATA_PACKET                         3
#define  DSP_USB_TX_RETRY_PACKET                 	       4

#define  DSP_DOWNLOAD_FW_TIMEOUT                    2000
//the packet will be dropped with retry count more than 
#define  DSP_MAX_RETRY_COUNT                  13

//Jakio 2006.12.12: for testing tx/rx fifo, timeout when testing fifo
#define  TEST_FIFO_TIMEOUT			5000 

#define MDIS_RESET_TIMEOUT			200	

#define DSP_8051_FIRMWARE_FRAGMENT_LENGTH	    	(1024)
#define DSP_FIRMWARE_FRAGMENT_LENGTH	    		(1024)

#define DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET		0xA000      //define it later
#define DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET		0xB000      //define it later
/*
	8051 MEM allocate table

	0000h - 0FFFh (code)	4K bytes	ROM
	1000h - 7FFFh (code)	Reserved	Reserved
	8000h - BFFFh (code)	16K bytes	Data memory  in RAM
	C000h - FFFFh (code)	16K bytes	Code memory in RAM
*/
#define DSP_8051FW_FIRST_SEGMENT_SIZE			(4*1024)   
#define DSP_8051FW_FIRST_SEGMENT_NUM			(4)   
#define DSP_8051FW_SECOND_SEGMENT_BEGIN		(0xc000)   
#define DSP_CODE_LENGTH	                       (4*65536+8)
#define BIN_CODE_HEADSZIE                      (0x80)

#define V4_BMEM_ROM_START			(1024*8)
#define V4_BMEM_ROM_LEN			(1024*4)
#define DSP_NOTIFY_STARVE_MAC    	                 (0)

//Jakio 2006.11.17 comment: should be 8???
#define DSP_NOTIFY_FRAME_LEN    	                 (64)


#define DOT11_OUI_LEN           3


#define _3DSP_NUM_TX_FIFOs              4
#define _3DSP_TX_FIFO_IDX_CP            0x00
#define _3DSP_TX_FIFO_IDX_CFP           0x01
#define _3DSP_TX_FIFO_IDX_PRIORITY      0x02
#define _3DSP_TX_FIFO_IDX_BCN           0x03

#define TX_POWER_25_PERCENT			0
#define TX_POWER_50_PERCENT			1
#define TX_POWER_75_PERCENT			2
#define TX_POWER_100_PERCENT			3
#define TX_POWER_MASK_BITS				0x03
#define DSP_TX_POWER_DEFAULT			3
#define DSP_TX_POWER_BATTERY			0

#define DSP_IRP_RESUBMIT_COUNT		20
#define _3DSP_TOTAL_TX_FIFO_SIZE		(12 * 1024)	/* In terms of bytes.for ap > 10000 is reqd */
#define _3DSP_CP_FIFO_SIZE              3456	/* in terms of 64 alignment */
#define _3DSP_CFP_FIFO_SIZE             3456	/* In terms of 64 alignment */
#define _3DSP_BCN_FIFO_SIZE             384		/* in terms of 64 alignment */
#define _3DSP_ATIM_FIFO_SIZE            768		/* in terms of 64 alignment */
#define _3DSP_PRIORITY_FIFO_SIZE        2048	/* in terms of 64 alignment */


#define _3DSP_AVERAGE_RATE_COUNT      		   200
#define _3DSP_AVERAGE_RATE_FAST_COUNT      30



#define _3DSP_MIN_BEACON_PERIOD         100 // TUs 1024 usec
#define _3DSP_MIN_IMP_TBTT_PERIOD       20  // TUs 1024 usec

#define _3DSP_TX_FIFO_MSDU_FLUSH_DELAY_USEC     100 // usec

#if defined(RXFRAG_TEST)
#define MMAC_CORE_INIT_FLAG	((UINT32)(MMAC_CORE_RXFRAG_TEST_INIT))
#else
#define MMAC_CORE_INIT_FLAG	((UINT32)(MMAC_CORE_START_REQ_HD))
#endif

#define 	NUM_OF_SUPP_RATES			12
/*Operational Data Rate Set for 802.11a/b/g*/
#define OP_1_MBPS			0x02
#define OP_2_MBPS			0x04
#define OP_5_MBPS  			0x0B
#define OP_11_MBPS			0x16
#define OP_6_MBPS			0x0C
#define OP_9_MBPS			0x12
#define OP_12_MBPS			0x18
#define OP_18_MBPS			0x24
#define OP_24_MBPS  			0x30
#define OP_36_MBPS			0x48
#define OP_48_MBPS  			0x60
#define OP_54_MBPS			0x6C
 
/*Basic Data Rate Set for 802.11a/b/g*/
#define BA_1_MBPS			0x82
#define BA_2_MBPS			0x84
#define BA_5_MBPS 			0x8B
#define BA_11_MBPS			0x96
#define BA_6_MBPS			0x8C
#define BA_9_MBPS			0x92
#define BA_12_MBPS			0x98
#define BA_18_MBPS			0xA4
#define BA_24_MBPS 			0xB0
#define BA_36_MBPS			0xC8
#define BA_48_MBPS  			0xE0
#define BA_54_MBPS			0xEC




typedef enum {
	MMACRX_NORMAL = 0,
	MMACRX_HOST_BUF_OVERFLOW,
	MMACRX_RX_LENGTH_REG_OVERFLOW,
	MMACRX_INVALID_RX_LENGTH_SEEN_BY_BB
} MMACR_RX_BUF_STATUS_t;

#if 0
/*
 * DOT11 Type
 */
typedef enum
{
	IEEE802_11_A,
	IEEE802_11_B,
	IEEE802_11_G
}dot11_type_t;
#endif

typedef enum
{
	NO_KEY_LEN = 0,
	KEY_BIT40_LEN = 5,
	KEY_BIT104_LEN = 13,
	KEY_BIT128_LEN = 16
}wep_key_len_t;

// TODO:Jackie
#if 0
typedef enum
{
	CIPHER_WEP,		/* 0x0 */
	CIPHER_TKIP,	/* 0x1 */
	CIPHER_CCMP		/* 0x2 */
}cipher_type_t;
#endif


typedef enum
{
	NO_ENCRYPTION,		/* 0x0 */
	BIT40_ENCRYPTION,	/* 0x1 */
	BIT104_ENCRYPTION,	/* 0x2 */
	BIT128_ENCRYPTION	/* 0x3 */
}encryption_keysize_t; // encryption_type_t


typedef enum
{
	UNICAST_KEY,		/* 0x0 */
	GROUPCAST_KEY		/* 0x1 */
}key_type_t;

#if 0   // TODO:Jackie
typedef struct dot11RSNAstats_tag
{
	UINT32			TKIPLocalMICFailure;
	UINT32			TKIPReplays;
	UINT32			CCMPReplays;
	LARGE_INTEGER	MICFailure_time_stamp;

}dot11RSNAstats_t;
#endif

typedef enum
{
	DSSS_CCK,      //support 1,2,5.5,11 Mb/s
	OFDM,	       //support 6,9,12,18,24,36,48,54 Mb/s
	ERP_PBCC,      //support 5.5,11,22,33 Mb/s
	ERP_DSSS_OFDM  //support 6,9,12,18,24,36,48,54 Mb/s
}modulation_type_t;


/*
 * Preamble Type
 */
typedef enum
{
	LONG_PRE_AMBLE,
	SHORT_PRE_AMBLE
}preamble_type_t;


typedef enum
{
	MIB_SUCCESS,	
	MIB_INVALID_ATTRIBUTE,
	MIB_READ_ONLY_ATTRIBUTE,
	MIB_WRITE_ONLY_ATTRIBUTE
}mib_status_t;

typedef enum mac_hw_state_tag
{
	DEV_IDLE,
	DEV_SCAN,
	DEV_JOIN,
	DEV_START=2,
	DEV_DOZE,
	DEV_ACTIVE
}mac_hw_state_t;
typedef enum media_link_state_tag
{
	LINK_FALSE,
   	LINK_OK
}media_link_state_t;


typedef enum assign_ssid_status_tag
{
	ASSIGN_SSID_NO_FOUND,
   	ASSIGN_SSID_FOUND,
   	ASSIGN_SSID_MISMACTH_PRIVACY
}assign_ssid_status_t;

typedef struct _ETHER_HEADER
{
	UINT8	daddr[WLAN_ETHADDR_LEN];
	UINT8	saddr[WLAN_ETHADDR_LEN];
	UINT16	type;
}ETHER_HEADER_T,*PETHER_HEADER_T;
/*--function prototypes-----------------------------------------------*/
#endif //file end
