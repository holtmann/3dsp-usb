#ifndef __DSP_MNG_H
#define __DSP_MNG_H
#include "precomp.h"
/*******************************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:		DSP_mng.h	CURRENT VERSION: 1.00.00
* PURPOSE:		Define macro, structure and functions related to fragment module
* NOTES:		This is DSP_mng.c header file.
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.
********************************************************************************/
#define COUNTRY_DOMAIN
#define MAX_ESR_LEN                   255

/*--macros------------------------------------------------------------*/

#define MAX_ATIM_WINDOW			    0xff

#define  ERP_HAVE_NONERP_STA          0x01
#define  ERP_USE_PROTECTION	          0x02

//#define WEP_MODE_WEP                   0
//#define WEP_MODE_TKIP                  1
//#define WEP_MODE_NONE                  2
//#define WEP_MODE_AES                   3


#define WRITE_BSSID_REG               (UINT16)(0x0001)
#define WRITE_SSID_REG                (UINT16)(0x0002)
#define WRITE_BEACONINTERVAL_REG      (UINT16)(0x0004)
#define WRITE_CAP_REG                 (UINT16)(0x0008)
#define WRITE_SPRATE_REG              (UINT16)(0x0010)
#define WRITE_ATIM_REG                (UINT16)(0x0020)
#define WRITE_CHANNEL_REG             (UINT16)(0x0040)
#define WRITE_AID_REG                 (UINT16)(0x0080)
#define WRITE_COUNRTR_REG             (UINT16)(0x0100)
#define WRITE_ERP_ENABLE              (UINT16)(0x0200)
#define WRITE_MAX_BASICRATE           (UINT16)(0x0400)
#define WRITE_ALL_BEACONINFO_REG      (UINT16)(WRITE_COUNRTR_REG|WRITE_BSSID_REG |WRITE_SSID_REG | WRITE_BEACONINTERVAL_REG | WRITE_CAP_REG |WRITE_SPRATE_REG|WRITE_ATIM_REG|WRITE_CHANNEL_REG|WRITE_AID_REG|WRITE_ERP_ENABLE|WRITE_MAX_BASICRATE)


#define RSNVERSION                (UINT16)(0x0001)
#define RSNOUI                    (UINT32)(0x01f25000)

#define AUTH_SUITE_RSN            (UINT32)(0x01f25000)
#define AES_AUTH_SUITE_RSN        (UINT32)(0x01f25000)
#define AUTH_SUITE_RSN_PSK        (UINT32)(0x02f25000)

#define KEY_SUITE_WEP40           (UINT32)(0x01f25000)
#define KEY_SUITE_TKIP            (UINT32)(0x02f25000)
#define KEY_SUITE_WRAP            (UINT32)(0x03f25000)
#define KEY_SUITE_CCMP            (UINT32)(0x04f25000)
#define KEY_SUITE_WEP104          (UINT32)(0x05f25000)

//Jakio20070703: add for wpa2
#define KEY_SUITE_TKIP_WPA2		(UINT32)(0x02ac0f00)
#define KEY_SUITE_CCMP_WPA2		(UINT32)(0x04ac0f00)

#define AUTH_SUITE_RSN_WPA2		(UINT32)(0x01ac0f00)
#define AUTH_SUITE_RSN_PSK_WPA2	(UINT32)(0x02ac0f00)
#define MNDIS_802_11_AI_REQFI_CAPABILITIES      (UINT32)(0x01)
#define MNDIS_802_11_AI_REQFI_LISTENINTERVAL    (UINT32)(0x02)
#define MNDIS_802_11_AI_REQFI_CURRENTAPADDRESS  (UINT32)(0x04)
#define MNDIS_802_11_AI_RESFI_CAPABILITIES      (UINT32)(0x01)
#define MNDIS_802_11_AI_RESFI_STATUSCODE        (UINT32)(0x02)
#define MNDIS_802_11_AI_RESFI_ASSOCIATIONID     (UINT32)(0x04)




#define PAIR_CIPHER_SUITE_NUM       10
#define AUTH_MGT_SUITE_NUM          10
#define MAX_BEACON_RSP_INFO_LENGTH  (360+4*PAIR_CIPHER_SUITE_NUM+4*AUTH_MGT_SUITE_NUM) 
#define MAX_ASSOC_REQ_NOFIXED_NUM   (34/*SSID*/+10/*sup rate*/+8+4+4*PAIR_CIPHER_SUITE_NUM+4*AUTH_MGT_SUITE_NUM/*RSN*/+10/*padding*/)
#define MAX_ASSOC_RSP_NOFIXED_NUM   (20)


#define     SCANFAILDEALY          10000

#define     MAXSTANUM_IBSS		    8

//#define     SCAN_NUMBER_ONETIME     3
#define     SCAN_NUMBER_ONETIME     2
//#define     SCAN_MIDDLE_TIME        100
#define     SCAN_MIDDLE_TIME        50

#define     IBSSTIMER			    50   //MS

#define     MAXIBSSSCANNUM          2
#define     MAXSCANNUM              	5
#define     MAXAUTHNUM              	5
#define     MAXASOCNUM              	5
#define     MAXJOINNUM              	5
#define     WLAN_BSSDES_NUM         20// 08.13    //old is 8
#define     MAX_SCAN_WAIT_COUNT     8


#define     SCAN_STATE_ACTIVE_FLAG				0x01
#define     SCAN_STATE_SSID_FLAG				0x02
#define     SCAN_STATE_ADDR_FLAG				0x04
#define     SCAN_STATE_MID_STOP_FLAG			0x08
#define     SCAN_STATE_OID_FLAG					0x10
#define     SCAN_STATE_RETRY_FLAG				0x20
#define     SCAN_STATE_BSSID_PRESENT_FLAG		0x40
#define     SCAN_STATE_FOLLOW_JOIN_FLAG 		0x80
#define     SCAN_STATE_11G_FLAG					0x100
#define     SCAN_STATE_11A_FLAG					0x200
#define     SCAN_STATE_11B_FLAG					0x400     //only scan for combo mode
#define     SCAN_STATE_WAIT_STATUS				0x800
//high 16bit as sub state of scan
//scan sub type
//#define     SCAN_SUBTYPE_WAIT_CHAN_CFM  		0x10000
//#define     SCAN_SUBTYPE_WAIT_HW_IDLE  		0x20000
/////////////////////////////////////////////////////////////////////////////////////////

#define     JOIN_STATE_NORMAL					0x01
#define     JOIN_STATE_KEEP_WITHOUT_BEACON	0x02   /* use for ibss */
#define     JOIN_STATE_KEEP_WITH_BEACON		0x04
//#define     JOIN_STATE_INIT_STAGE				0x08
//define   join's sub type, will be put into statuscontrol.worktype's high 16 bits
#define     JOIN_SUBTYPE_INIT  					0x100
#define     JOIN_SUBTYPE_WAIT_HW_IDLE  			0x200
#define     JOIN_SUBTYPE_WAIT_CHAN_CFM  		0x400
#define     JOIN_SUBTYPE_WAIT_BEACON	  		0x800

#define     JOIN_STATE_NORMAL_INIT		  		(JOIN_STATE_NORMAL | JOIN_SUBTYPE_WAIT_CHAN_CFM |JOIN_SUBTYPE_INIT)


#define     JOIN_SUBTYPE_CLEAR_BITS  			0x00ff
#define     JOIN_SUBTYPE_EXPRESS_BITS  			0xff00

#define     JOIN_CFM_RETRY_NUM		 			5//100	//Justin: debug wzc
#define     JOIN_PROCESS_RETRY_NUM		 		5
#define     JOIN_PROCESS_ALWAYS					200


#define     CONFIRM_CHANNEL_TIMEOUT		5    //5ms
#define     MNG_WAIT_HW_IDLE_TIMEOUT		50    //5ms


#define MAXSTANUM_IBSS			    8

#define MNGTXBUF_MAXLENGTH			2048
#define MNGRXBUF_MAXLENGTH			2048

#define MAXAUTHNUM					5
#define MAXASOCNUM					5
#define MAXJOINNUM					5

#define SCAN_ERR					0
#define AUTH_RETRY_ERR				1
#define AUTH_CHALLENGE_ERR			2
#define AUTH_ALG_ERR				3
#define ASSOC_RETRY_ERR				4


#define SUPPORTRATELEN			    12  //8
#define CHALLENGE_TEXT_LEN			128

#define IDLE						0x00
#define SCANNING					0x01
#define JOINNING					0x02
#define JOINOK						0x03
#define AUTHOK						0x04
#define ASSOCOK						0x05
/* #define OIDSCAN						0x10	 */

#define AUTH_INIT					0x0000
#define WAIT_AUTH2					0x0001
#define WAIT_AUTH3					0x0002
#define WAIT_AUTH4					0x0003
#define WAIT_AUTH4_OK				0x0004
#define WAIT_AUTH_DELAY				0x0005

#define AUTH_SEQ1					0x0001
#define AUTH_SEQ2					0x0002
#define AUTH_SEQ3					0x0003
#define AUTH_SEQ4					0x0004

/*  define  status codes */
#define SUCCESS						0x0000
#define UNSPE_FAIL					0x0001
#define NO_SUP_CAP					0x000a
#define REASSOC_DENY				0x000b
#define ASSOC_DENY					0x000c
#define AUTH_ALG_NO_SUP				0x000d
#define AUTH_SEQ_OUT				0x000e
#define CHALLENGE_FAIL				0x000f
#define AUTH_FAIL_TIMOUT			0x0010
#define ASSOC_DENY_BEYOND			0x0011
#define ASSOC_DENY_RATE				0x0012
/* reason code */
#define UNSPE_REASON				0x0001
#define AUTH_NOLONGER_VALID			0x0002
#define DEAUTH_LEAVE				0x0003
#define DISASSOC_INACTIVITY			0x0004
#define DISASSOC_AP_FULL			0x0005
#define CLASS2ERR					0x0006 
#define CLASS3ERR					0x0007
#define DISASSOC_LEAVE				0x0008
#define REQ_ASSOC_NO_AUTH			0x0009
#define INVALID_INFO_ELEMENT		0x000d
#define MIC_FAILURE					0x000e
#define WAY4_HSHAKE_TIMEOUT			0x000f
#define GROUP_UPDATE_TIMEOUT		0x0010
#define INFO_DIFF_BEACON			0x0011
#define MULTI_CIPHER_INVALID		0x0012
#define UNI_CIPHER_INVALID			0x0013
#define AKMP_INVALID				0x0014
#define UNSUPPORTED_RSNE			0x0015
#define INVALID_RSNE_CAP			0x0016
#define IEEE8021X_AUTH_FAILED		0x0017
//Jakio20070719: when AP not support open auth mode, it will deauth with this reason code
#define DEAUTH_ERROR_MODE		0x000d

/* auth alg */
//#define OPEN_SYSTEM					0x0000
//#define SHARED_KEY					0x0001


#define TXBUF_LEN					500
#define RXBUF_LEN					500


#define ASSOCREQ					0x00
#define ASSOCRSP					0x10
#define REASSOCREQ					0x20
#define REASSOCRSP					0x30
#define PROBEREQ					0x40
#define PROBERSP					0x50
#define BEACON						0x80
#define ATIM						0x90
#define DISASSOC					0xA0
#define AUTH						0xB0
#define DEAUTH						0xC0

#define SSID_EID					0x00
#define SUPRATE_EID					0x01
#define FHPARA_EID					0x02
#define DSPARA_EID					0x03
#define CFPARA_EID					0x04
#define TIM_EID						0x05
#define IBSSPARA_EID				0x06
#define COUNTRY_ELD					0x07
#define REQUEST_ELD					0x0A
#define CHALLENGE_EID				0x10
//802.11g
#define ERPINFORMATION_EID			0x2a
#define RESERVE_1_EID				0x2f

#define EXTENDRATE_EID				0x32

//Jakio2006.12.18: EIDs for wpa and wpa2
#define WPA1_EID					0xdd

#ifdef DSP_WPA2
#define WPA2_EID					0x30
#endif
//endof Jakio2006.12.18


/* FRAME HEADER  */

#define NO_TODS_BIT					0x0000




#define  ACTIVE_SCAN_FIRST_STAGE     1
#define  ACTIVE_SCAN_SECOND_STAGE     2



#define MERGE_TIME	               0x400

#define MAX_11GRATE_INDEX             11
#define MAX_11BRATE_INDEX             3
#define MAX_11GCHANNEL_INDEX		14


#define 	SEND_PROBE_TIMES			3//justin: 080618.	more chance to get probe-respose. 	normaly, do not set to a number bigger than 10. 
									//	because we set to DMA a mng packet must < 1024 bytes. and a probe should be about 70 bytes.

#define  AP_LIFE_TIME_IN_SCAN	           2

/*--constants and types------------------------------------------------*/

#pragma pack(1)

typedef struct _MNG_NORMAL_ADDR
{
	UINT8   	a1[WLAN_ADDR_LEN];
	UINT8		a2[WLAN_ADDR_LEN];
	UINT8		a3[WLAN_ADDR_LEN];
}MNG_NORMAL_ADDR_T,*PMNG_NORMAL_ADDR_T;


typedef struct _MNG_IBSS_ADDR
{
	UINT8   	da[WLAN_ADDR_LEN];
	UINT8		sa[WLAN_ADDR_LEN];
	UINT8		bssid[WLAN_ADDR_LEN];
}MNG_IBSS_ADDR_T,*PMNG_IBSS_ADDR_T;


typedef struct _MNG_BSSTX_ADDR
{
	UINT8   	bssid[WLAN_ADDR_LEN];
	UINT8		sa[WLAN_ADDR_LEN];
	UINT8		da[WLAN_ADDR_LEN];
}MNG_BSSTX_ADDR_T,*PMNG_BSSTX_ADDR_T;


typedef struct _MNG_BSSRX_ADDR
{
	UINT8   	da[WLAN_ADDR_LEN];
	UINT8		bssid[WLAN_ADDR_LEN];
	UINT8		sa[WLAN_ADDR_LEN];
}MNG_BSSRX_ADDR_T,*PMNG_BSSRX_ADDR_T;

typedef union _MNG_ADDR
{
	MNG_IBSS_ADDR_T		ibss;
	MNG_BSSTX_ADDR_T	bsstx;
	MNG_BSSRX_ADDR_T	bssrx;
	MNG_NORMAL_ADDR_T   addr;
}MNG_ADDR_T,*PMNG_ADDR_T;


/* typedef struct _MNG_FRAME_HEAD
{
	UINT16                  fc;
	UINT16                  duration;
	MNG_ADDR_T				adr;
}MNG_FRAME_HEAD_T,*PMNG_FRAME_HEAD_T;
*/


typedef struct _MNG_COUNTRY_CHAN
{
	UINT8			firstch;
	UINT8			numofch;
	UINT8			maxpower;/*The Maximum Transmit Power Level field is a signed number and shall be 1 octet in length. It shall indicate
the maximum power, in dBm, allowed to be transmitted.*/		// 802.11d		zyy
}MNG_COUNTRY_CHAN_T,*PMNG_COUNTRY_CHAN_T;


//this structure save country info from beacon
typedef struct _MNG_COUNTRY_INFO
{
	UINT8		country[3];
	UINT8       	maxpower;
	UINT8		channelnum;
	UINT8       	channellist[WLAN_MAX_CHANNEL_LIST];
}MNG_COUNTRY_INFO_T,*PMNG_COUNTRY_INFO_T;


typedef struct _BEACON_FIXED_FIELD
{
	UINT8		    timestamp[8];
	UINT16		    beaconinterval;
	UINT16		    cap;
}BEACON_FIXED_FIELD_T,*PBEACON_FIXED_FIELD_T;



typedef struct _MNG_IBSS_INFO
{
	UINT8       addr[WLAN_ETHADDR_LEN];
	BOOLEAN    ps;
}MNG_IBSS_INFO_T,*PMNG_IBSS_INFO_T;


typedef struct _MNG_IBSS_INFO_LIST
{
	UINT32				num;
	MNG_IBSS_INFO_T     ibsssta[MAXSTANUM_IBSS];
}MNG_IBSS_INFO_LIST_T,*PMNG_IBSS_INFO_LIST_T;

typedef struct {
	UINT16      Capabilities;
	UINT16      ListenInterval;
    UINT8       CurrentAPAddress[WLAN_ADDR_LEN];
}REQFIXEDIES;


typedef struct {
	UINT16      Capabilities;
	UINT16      StatusCode;
    UINT16      AssociationId;
}RSPFIXEDIES;



typedef struct {
	UINT16        AvailableRequestFixedIEs;
	REQFIXEDIES   reqIe;
	UINT32         RequestIELength;
	UINT8         ReqBuff[MAX_ASSOC_REQ_NOFIXED_NUM];
	UINT16        AvailableResponseFixedIEs;
	RSPFIXEDIES   rspIe;	
	UINT32         ResponseIELength;
	UINT8         RspBuff[MAX_ASSOC_RSP_NOFIXED_NUM];
}REQ_RSP_FIXEDIES,*PREQ_RSP_FIXEDIES;

//Jakio2006.12.18: moved definitons here
typedef struct _MNG_NOFIXED_ALL
{
	UINT8	element;
	UINT8	length;
	UINT8	data[1];
}MNG_NOFIXED_ELE_ALL_T,*PMNG_NOFIXED_ELE_ALL_T;



// fdeng
#pragma pack()



typedef struct _MNG_DES
{
#ifdef ANTENNA_DIVERSITY
	UINT8 			antenna_num;
#endif
	UINT8			bssid[WLAN_ETHADDR_LEN];
	UINT8			oldBssid[WLAN_ADDR_LEN];
	UINT8          		addr[WLAN_ETHADDR_LEN];
	UINT8			channel;
	UINT16	        	beaconinterval;
	UINT16         	atimw;
	UINT16         	cap;
	UINT16         	aid;
	INT32	   		rssi;
	UINT8          		suprate[SUPPORTRATELEN+1]; /* first byte is length    */
	UINT8          		ssid[WLAN_SSID_MAXLEN+1];      /* first byte is length */
	UINT8          		oldssid[WLAN_SSID_MAXLEN+1];      /* first byte is length */
    	MNG_COUNTRY_INFO_T  countryinfo;
	UINT32  IELength;
	UINT8  IEs[MAX_BEACON_RSP_INFO_LENGTH];
	
	UINT8 ExtendedSupportRate[MAX_ESR_LEN];
	UINT8 ExtendedSupportRateLen;
	UINT8 Erpflag;
	UINT8 Erpinfo;
	NDIS_802_11_NETWORK_TYPE networkType;// Justin: 0625.  must use type enum NDIS_802_11_NETWORK_TYPE
//	UINT32 networkType;		// Justin: 0625.  must use type enum NDIS_802_11_NETWORK_TYPE
	//add by woody
	UINT32 phy_mode;
	//Jakio2006.12.18:add here for wpa2
	//MNG_NOFIXED_ELE_ALL_T wpaIe;

	
	//Jakio20060629: add for wpa2
	//PMNG_NOFIXED_ELE_ALL_T  pWpaIe;
	//PMNG_NOFIXED_ELE_ALL_T  pWpa2Ie;
	UINT16	offset_wpa;
	UINT16 offset_wpa2;


	// add by Justin
	UINT32	offsetHi;
	UINT32	offsetLo;
	UINT32    lifetime;
}MNG_DES_T,*PMNG_DES_T;

typedef struct _MNG_DESLIST
{
	/* int                 curbssdesnum; */
	UINT32			bssdesnum;
	MNG_DES_T		bssdesset[WLAN_BSSDES_NUM];
}MNG_DESLIST_T,*PMNG_DESLIST_T;



typedef struct _MNG_STATUS_CONTROL
{
	UINT32					curstatus;   //IDLE ,SCANNING, JOINNING, JOINOK, AUTHOK,ASSOCOK
	UINT32					worktype;	//ACTIVE_SCAN_FLAG,
										//SSID_SCAN_FLAG,ADDR_SCAN_FLAG,
										//MID_STOP_SCAN_FLAG,OID_SCAN_FLAG,
										//RETRY_SCAN_FLAG,
										//BSSID_PRESENT_SCAN_FLAG,FOLLOW_JOIN_SCAN_FLAG,
										//SCAN_SUBTYPE_WAIT_CHAN_CFM,SCAN_SUBTYPE_WAIT_HW_IDLE
										//NORMAL_JOIN,KEEP_JOIN_WITHOUT_BEACON,KEEP_JOIN_WITH_BEACON,
					//JOIN_SUBTYPE_WAIT_HW_IDLE,JOIN_SUBTYPE_WAIT_CHAN_CFM,JOIN_SUBTYPE_WAIT_TIMEOUT,
	UINT32			        	retrynum;	//for join: the variable is used to indicate retry num: switch chann & switch state to idle
	UINT32					workcount;   //for join: save the retry num of join
	UINT32				    	workflag;	//for scan, flag is either 11g or 11a
										//   first scan 11g,then scan 11a
}MNG_STATUS_CONTROL_T,*PMNG_STATUS_CONTROL_T;


/* structure define */
typedef struct _MNG_STRU
{
	/* UINT8					oldBssid[WLAN_ADDR_LEN]; */
	BOOLEAN					mng_timer_running;
	INT32					rssi;
	UINT8 	    			recbuf[MNGTXBUF_MAXLENGTH];
	UINT8 	    			tranbuf[MNGRXBUF_MAXLENGTH];
	BOOLEAN			is_probe;
	UINT8					rec_challenge_text[CHALLENGE_TEXT_LEN];
	UINT16					txlen;
	UINT16					rxlen;
	UINT8					body_padding_len;// justin; 0623
	UINT8					curchannel;     
	BOOLEAN					oidscanflag;
	UINT32					scantype;
	UINT32					scan_stage;		//ACTIVE_SCAN_FIRST_STAGE , ACTIVE_SCAN_SECOND_STAGE
	UINT32					haveSignal;
	UINT32					reassocflag;
	UINT8					scan_chan_list[15];
	UINT8					scan_chan_num;
	MNG_STATUS_CONTROL_T    statuscontrol;
	REQ_RSP_FIXEDIES		reqRspInfo;
	
	MNG_DESLIST_T			bssdeslist;
	MNG_DESLIST_T			oiddeslist;
	MNG_DESLIST_T			beacondeslist;
#ifdef ROAMING_SUPPORT
	MNG_DESLIST_T			ssid_rssisort_deslist;	// rssi sorted des list which have same SSID, so the first one in the list is the one has highest rssi
	MNG_DESLIST_T			OutTab;
	UINT8					rateIndex_b4_roaming;	// save before roaming scan ,and recover datarate if found the same AP after beacon lost scan....
#endif
	MNG_DES_T				usedbssdes;			//justin:	071212.	holds the current AP's addr which we want to connect but not connected yet.
												//	it is to say, get the description after set ssid_oid from the scan list results
	MNG_DES_T				olddes;
	MNG_DES_T				middledes;
	MNG_IBSS_INFO_LIST_T	ibssinfo; 	
	UINT8                   rateIndex;  //Jakio: variable stands for tx_speed index of current rate, range 0x00~0x0b;
	UINT8			 flag11g;
	UINT8  default_wep_keys[MIB_MACADDR_SPECIFIC_KEY] [SME_MAX_KEY_LEN_IN_HW];//Justin: 0717
	UINT8          		    hide_ssid[WLAN_SSID_MAXLEN+1];      /* first byte is length, added for hiding ssid */
    BOOLEAN                 hide_ssid_found;
    UINT8          		    hide_bssid[WLAN_ETHADDR_LEN];

    SCAN_RES               scan_result[WLAN_BSSDES_NUM];
}MNG_STRU_T,*PMNG_STRU_T;



// fdeng
#pragma pack(1)

typedef struct _MNG_HEAD80211
{
	UINT16                  fc;
	UINT16                  duration;
	MNG_ADDR_T          adr;
	UINT16					seq;
}MNG_HEAD80211_T,*PMNG_HEAD80211_T;



typedef struct _MNG_AUTHFRAME_DATA
{
	UINT16		authalg;
	UINT16         authseq;
	UINT16		status;
}MNG_AUTHFRAME_DATA_T,*PMNG_AUTHFRAME_DATA_T;

typedef struct _MNG_DISASSOC_DATA
{
	UINT16		reason;
}MNG_DISASSOC_DATA_T,*PMNG_DISASSOC_DATA_T;


typedef struct _MNG_ASSOCREQ_DATA
{
	UINT16			cap;
	UINT16          listenInt;
	//UINT8		    ssidEle;
	//UINT8			ssidLen;
}MNG_ASSOCREQ_DATA_T,*PMNG_ASSOCREQ_DATA_T;


typedef struct _MNG_REASSOCREQ_DATA
{
	UINT16			cap;
	UINT16          listenInt;
	UINT8		    apaddr[WLAN_BSSID_LEN];
	//UINT8		    ssidEle;
	//UINT8			ssidLen;
}MNG_REASSOCREQ_DATA_T,*PMNG_REASSOCREQ_DATA_T;



/*typedef struct _MNG_NOFIXED_ELE
{
	UINT8	element;
	UINT8	length;
}MNG_NOFIXED_ELE_T,*PMNG_NOFIXED_ELE_T;


typedef struct _MNG_NOFIXED_ELE_HEAD
{
	UINT8	element;
	UINT8	length;
}MNG_NOFIXED_ELE_HEAD_T,*PMNG_NOFIXED_ELE_HEAD_T;
*/

/*
typedef struct _MNG_NOFIXED_ALL
{
	UINT8	element;
	UINT8	length;
	UINT8	data[1];
}MNG_NOFIXED_ELE_ALL_T,*PMNG_NOFIXED_ELE_ALL_T;
*/

typedef struct _MNG_RSN_FIXED
{
	UINT32	oui;
	UINT16	version;
	UINT32	groupSuite;
}MNG_RSNFIXED_T,*PMNG_RSNFIXED_T;

//Jakio20070629: add for wpa2
//according 802.11i doc, there is no oui field
typedef struct _MNG_WPA2_FIXED
{
	UINT16		version;
	UINT32		groupSuite;
}MNG_WPA2_RSNFIXED_T, *PMNG_WPA2_RSNFIXED_T;

typedef struct _MNG_RSN_COUNT
{
	UINT16	count;
}MNG_RSN_COUNT_T,*PMNG_RSN_COUNT_T;

typedef struct _MNG_RSN_SUITE
{
	UINT32	suite;
}MNG_RSN_SUITE_T,*PMNG_RSN_SUITE_T;


typedef struct _MNG_ASSOCRSP_DATA
{
	UINT16			cap;
	UINT16          status;
	UINT16		    aid;
	//UINT8			suprateEle;
	//UINT8			suprateLen;
}MNG_ASSOCRSP_DATA_T,*PMNG_ASSOCRSP_DATA_T;

#pragma pack()

/* ---------------------------------*/
/*
#define WriteElement((UINT8)elementId,(PUINT8)elementAddr,(PUINT8)contentAddr,(UINT8)contentLen) \
{ \
	PMNG_NOFIXED_ELE_HEAD_T ptmp; \
	ptmp = (PMNG_NOFIXED_ELE_HEAD_T)elementAddr; \
	ptmp->element = elementId; \
	ptmp->length = contentLen; \
	tdsp_memcpy(((PUINT8)elementAddr + sizeof(MNG_NOFIXED_ELE_HEAD_T)), contentAddr, contentLen); \
}
*/
/*--function prototypes-----------------------------------------------*/

TDSP_STATUS Mng_Init(PDSP_ADAPTER_T 		pAdap);
TDSP_STATUS Mng_Release(PDSP_ADAPTER_T 		pAdap);

VOID Mng_Receive(PUINT8 rxbuf,UINT32 rxlen,PRX_TO_MNG_PARAMETER pMngPara,PDSP_ADAPTER_T pAdap);
BOOLEAN Mng_GetLinkStatus(PDSP_ADAPTER_T pAdap);
VOID Mng_Reset(PDSP_ADAPTER_T pAdap);
VOID  Mng_OidScan(PDSP_ADAPTER_T pAdap);
VOID Mng_OidScanWithSsid(PDSP_ADAPTER_T pAdap);
VOID Mng_GetBssList(PMNG_DES_T *bsslist,UINT32 *num,PDSP_ADAPTER_T pAdap);
UINT8 Mng_GetCurCh(PDSP_ADAPTER_T pAdap);   
UINT32 Mng_OidAssignSsid(PUINT8 ssid,UINT8 len,BOOLEAN cmp_bssid,PDSP_ADAPTER_T pAdap);
VOID Mng_Timeout(PDSP_ADAPTER_T pAdap);
VOID Mng_StartJoin(PDSP_ADAPTER_T pAdap,UINT32 state);
VOID Mng_DoJoin(PDSP_ADAPTER_T pAdap,UINT32 state);
VOID Mng_MidJoinUntilSuccess(PDSP_ADAPTER_T pAdap);
VOID Mng_DisConnect(PDSP_ADAPTER_T pAdap);
UINT32 Mng_GetPsStatus(PUINT8 paddr,PDSP_ADAPTER_T pAdap);
PREQ_RSP_FIXEDIES Mng_GetAssocInfoStructure(PDSP_ADAPTER_T pAdap);
VOID Mng_BssDisAssoc(PDSP_ADAPTER_T pAdap,PUINT8 disassoc_bssid,UINT16 reason);
BOOLEAN Mng_Merge(PDSP_ADAPTER_T pAdap);
BOOLEAN Mng_GetSSIDFromBeacon(PUINT8 recbuf,PUINT8 pssid,UINT8 *len);
VOID Mng_GetRateIndex(PDSP_ADAPTER_T pAdap);
VOID Mng_BeaconChange(PMNG_DES_T curdes,PMNG_DES_T newdes,PDSP_ADAPTER_T pAdap);
VOID Mng_ClearPara(PDSP_ADAPTER_T pAdap);

VOID Mng_SetPowerActive(PDSP_ADAPTER_T pAdap,BOOLEAN ps_poll_flag);
VOID Mng_SetPowerSave(PDSP_ADAPTER_T pAdap);

TDSP_STATUS Mng_MakePsPollFrame(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Mng_MakeNullFrame(PDSP_ADAPTER_T pAdap);


extern TDSP_STATUS
Adap_set_join_para(
	PDSP_ADAPTER_T pAdap,
	PMNG_DES_T pdes
);


/*VOID Mng_DoJoin(PDSP_ADAPTER_T pAdap);*/
VOID Mng_StartAuth(PDSP_ADAPTER_T pAdap);
VOID Mng_DoAuth(PDSP_ADAPTER_T pAdap);
VOID Mng_StartAssoc(PDSP_ADAPTER_T pAdap);
VOID Mng_DoAssoc(PDSP_ADAPTER_T pAdap);
VOID Mng_Join_Timeout(PDSP_ADAPTER_T pAdap);


TDSP_STATUS Mng_MakeBeaconOrProbersp(PDSP_ADAPTER_T pAdap,UINT8 type,PUINT8 probeaddr);
TDSP_STATUS Mng_MakeAuthFrame1(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Mng_MakeAuthFrame3(PDSP_ADAPTER_T pAdap);
#ifdef PMK_CACHING_SUPPORT
BOOLEAN Mng_GetRSNCapabilities(
//     NDIS_802_11_AUTHENTICATION_MODE auth, 
     PUINT16		pRSNCap,
     MNG_DES_T *pBss);
#endif
TDSP_STATUS Mng_MakeAssocReqFrame(PDSP_ADAPTER_T pAdap);





VOID Mng_RestoreRegValue(PDSP_ADAPTER_T pAdap);
//VOID Mng_RestoreForJumpScan(PDSP_ADAPTER_T pAdap);
VOID Mng_ScanFail(PDSP_ADAPTER_T pAdap);

BOOLEAN Mng_GetBeaconInfo(PMNG_DES_T pcurdes,PDSP_ADAPTER_T pAdap);
BOOLEAN Mng_GetBeaconInfo_1(PMNG_DES_T pcurdes,PDSP_ADAPTER_T pAdap);
/*VOID Mng_AddBeaconBssDes(PMNG_DES_T des,PDSP_ADAPTER_T pAdap);*/
BOOLEAN Mng_GetNoExistEle(PUINT8 pbssid,UINT32 beaconlen,PUINT8 *pbuf,UINT32 *len,PDSP_ADAPTER_T pAdap);
/*VOID Mng_AddtoBssDesSetXp(PMNG_DES_T pdes,PDSP_ADAPTER_T pAdap); */
VOID Mng_WriteBeaconInfoReg(UINT16 flag,PMNG_DES_T pdes,PDSP_ADAPTER_T pAdap);
VOID Mng_ExchangeMattrAndMng(UINT16 flag,PMNG_DES_T pdes,UINT32 direction,PDSP_ADAPTER_T pAdap);
VOID Mng_AuthTimeout(PDSP_ADAPTER_T pAdap);

//VOID Mng_InitMgt(PDSP_ADAPTER_T pAdap);	
VOID Mng_InitParas(PDSP_ADAPTER_T pAdap);
/*VOID Mng_AddBssDes(PMNG_DES_T pdes,PDSP_ADAPTER_T pAdap);*/
TDSP_STATUS Mng_MakeReAsocReqFrame(PDSP_ADAPTER_T pAdap);	


VOID Mng_SetTimerPro(UINT32 delayv,PDSP_ADAPTER_T pAdap);
VOID Mng_ClearTimerPro(PDSP_ADAPTER_T pAdap);
//TDSP_STATUS Mng_SendPacket(PUINT8 source,UINT16 len,PDSP_ADAPTER_T pAdap);
VOID Mng_SetLinkToUp(PDSP_ADAPTER_T pAdap);
VOID Mng_ClearScanFlag(PDSP_ADAPTER_T pAdap);
VOID Mng_ResourceAvailable(PDSP_ADAPTER_T pAdap);	
VOID WriteElement(UINT8 elementId,PMNG_NOFIXED_ELE_ALL_T elementAddr,PUINT8 contentAddr,UINT8 contentLen);
//UINT8 Mng_MaxBasicRate(PDSP_ADAPTER_T pAdap,PUINT8 exrate,UINT8 exlen,PUINT8 suprate,UINT8 suplen);
BOOLEAN Mng_GetCountryWhenJoin(PDSP_ADAPTER_T pAdap);
VOID Mng_AuthFail(PDSP_ADAPTER_T pAdap);
BOOLEAN Mng_GetOneElementFromBeacon(PDSP_ADAPTER_T pAdap,UINT8 ele,PUINT8 pbuf,PUINT8 plen);

BOOLEAN Mng_GetWPA_FROM_RSN(PDSP_ADAPTER_T pAdap,PUINT8 pbuf,PUINT32 pinfo);

VOID Mng_UpdataIEs(PUINT8 iesBuf,PUINT32 pIesLength);
BOOLEAN Mng_MatchWPA1_FromRsn(PDSP_ADAPTER_T pAdap,PUINT8 pbuf, UINT32 wep_mode);
BOOLEAN Mng_WpaGroupCipherMode(PDSP_ADAPTER_T pAdap, PUINT8 pbuf, PUINT8 mode);
BOOLEAN Mng_MatchWPA2_FromRsn(PDSP_ADAPTER_T pAdap,PUINT8 pbuf, UINT32 wep_mode);
BOOLEAN Mng_Wpa2GroupCipherMode(PDSP_ADAPTER_T pAdap, PUINT8 pbuf, PUINT8 mode);
VOID Mng_Update_Cts_State(PDSP_ADAPTER_T pAdap,UINT8 erp);

VOID Mng_BreakIfScanning(PDSP_ADAPTER_T pAdap);

VOID Mng_Calc_Capbility(PDSP_ADAPTER_T pAdap);
BOOLEAN Mng_IBSS_valid_probe(PDSP_ADAPTER_T pAdap);

VOID Mng_UpdateRssi(PDSP_ADAPTER_T pAdap, INT32 rssi);
UINT8 Mng_GetMaxIbssSupportRate(PDSP_ADAPTER_T pAdap, PUINT8 paddr);

VOID BssTableInit(
     MNG_DESLIST_T *Tab) ;

UINT32 BssTableSearch(
     MNG_DESLIST_T *Tab, 
     PUINT8    pBssid,
     UINT8     Channel) ;

UINT32 BssSsidTableSearch(
	 MNG_DESLIST_T *Tab, 
	 PUINT8    pBssid,
	 PUINT8    pSsid,
	 UINT8     SsidLen,
	 UINT8     Channel) ;

VOID BssTableDeleteEntry(
	 	MNG_DESLIST_T *Tab, 
			PUINT8    pBssid);

INT32 rssi_reg_value_to_dbm(INT32 rssi);
VOID Mng_Renew_BssList(PDSP_ADAPTER_T pAdap, PMNG_DES_T pdes);
VOID Mng_Append_Item_BssList(PDSP_ADAPTER_T pAdap, PMNG_DES_T pdes,PMNG_DESLIST_T plist);

VOID BssTableSortByRssi(
      MNG_DESLIST_T *OutTab) ;

#ifdef ROAMING_SUPPORT

BOOLEAN Mng_CheckAKMAndCipher(
     NDIS_802_11_AUTHENTICATION_MODE auth, 
     MNG_DES_T *pBss,
    UINT8 pwCipher,//pairwise cipher
    UINT8 gpCipher);


VOID BssTableSsidSort(
    PDSP_ADAPTER_T pAdap, 
    MNG_DESLIST_T *OutTab, 
    INT8  Ssid[], 
    UINT8 SsidLen) ;


BOOLEAN Mng_roam_threshold_reached( PDSP_ADAPTER_T pAdap, PMNG_DES_T pBestBss);

VOID  Mng_recover_regs_joined(PDSP_ADAPTER_T pAdap);

VOID Mng_reconnect(PDSP_ADAPTER_T pAdap);
#endif//ROAMING_SUPPORT
VOID Mng_Fail(PDSP_ADAPTER_T pAdap);

BOOLEAN Mng_IsScanFinished(PDSP_ADAPTER_T pAdap);

BOOLEAN Mng_AssocRetrylimit(PDSP_ADAPTER_T pAdap);

#endif /*file end */
