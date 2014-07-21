/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:
 *
 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************
 *
 * [lwlan_ioctl.h] description:
 *		This header file contains some MACRO define (IOCTL) and some data-struct
 *		define used by IOCTL or SME.
 *  
 * exports:
 *
 * make'ing:
 *
 * TODO:
 * 
 * see also:
 *
 * This source to be viewed with tabspace 4 (":se ts=4" for ex/vi)
 *
 ************************************************************************/
#ifndef _LWLAN_IOCTL_H_
#define _LWLAN_IOCTL_H_

#ifndef IFNAMSIZ
#define IFNAMSIZ 32
#endif

extern const struct iw_handler_def _wlan_dev_iw_handler_def;


#define 	DSP_PARAM_DUMMY			SIOCIWFIRSTPRIV
#define 	DSP_PARAM_DRIVER_VER		SIOCIWFIRSTPRIV+1
#define 	DSP_PARAM_GET_TXPWR		SIOCIWFIRSTPRIV+3
#define 	DSP_PARAM_SET_DOMAIN	SIOCIWFIRSTPRIV+5
#define 	DSP_PARAM_GET_DOMAIN	SIOCIWFIRSTPRIV+7


enum {
	//DSP_PARAM__CTS_2_SELF = 1,			/* rts to self */
	//DSP_PARAM__BEACON = 2,				/* beacon interval */
	//DSP_PARAM__DTIM = 3,				/* dtim */
	DSP_PARAM__AUTH_MODE = 4,			/* authentication mode */
	DSP_PARAM__RNCRYPT_MODE = 5,		/* encrypt mode */
	DSP_PARAM__TX_POWER = 6,			/* tx power level */
	//DSP_PARAM__BROAD_SSID = 7,		/* broadcast ssid */
	//DSP_PARAM__DEBUG = 8,				/* debug code , print some register */
	//Spear add the type below @08/02/28
	DSP_PARAM__RSSI = 9,					/* receive signal strenth */
};

// It seemed that we can't define this macro bigger than 2048,	fdeng
#define HELP_BUF_LEN				2000		

void  iw_send_bssid_to_up(void *dev, unsigned char *bssid);

#endif//#ifndef _TDSP_IOCTL_H_
