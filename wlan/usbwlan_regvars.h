

#ifndef DSP_REGVARS_H
#define DSP_REGVARS_H

/***********************************************************************
* Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
*
* FILENAME:     DSP_Regvars.h      CURRENT VERSION: 1.00.01
* CREATE DATE:  2006/04/13
* PURPOSE:      mainly define some const used for parsing registry.
* 
*
* DECLARATION:  This document contains confidential proprietary information that
*               is solely for authorized personnel. It is not to be disclosed to
*               any unauthorized person without prior written consent of 3DSP
*               Corporation.        
*
**********************************************************************/

#include "precomp.h"

/*--macros------------------------------------------------------------*/
 #define			PS_SUPPORT					BIT17
 #define			CCMP_SUPPORT	    			BIT18
 #define			TKIP_SUPPORT    			BIT19
 #define			ENCRYPTION_SUPPORT			BIT20//WEP only
 #define			DOT_11K_SUPPORT				BIT21
 #define			DOT_11D_SUPPORT				BIT22
 #define			DOT_11I_SUPPORT				BIT23
 #define			DOT_11H_SUPPORT				BIT24
 #define			DOT_11G_SUPPORT				BIT25
 #define			DOT_11E_SUPPORT				BIT26
 #define			DOT_11B_SUPPORT				BIT27
 #define			DOT_11A_SUPPORT				BIT28
 #define			ADHOC_SUPPORT				BIT29
 #define			PCF_SUPPORT					BIT30
 #define			AP_SUPPORT					BIT31
 #define 			UM_VERSION_BITS				0x7F
 #define 			RELEASE_TYPE_BIT			BIT7

 #define		UM_VERSION					32		/* Depending upon um version of hw */
 #define		TBTT_OFFSET					50 //1000 		/* in micro sec */
 #define		ALIGN64(_x)					(((_x) / 64) * 64)



#define CUR_CHANNEL_NUM_BIT		 BIT_0_7
#define BB_TX_ERR_PROCESS_DONE_BIT       BIT_18

/*--constants and types------------------------------------------------*/
/*
const ULONG DSP_CHANNEl_DEFAULT   =       1;
const ULONG DSP_CHANNEl_MIN       =       1;
const ULONG DSP_CHANNEl_MAX       =       14;

const ULONG DSP_MACMODE_DEFAULT    =      2;
const ULONG DSP_MACMODE_MIN        =      1;
const ULONG DSP_MACMODE_MAX        =      2;

const ULONG DSP_SSIDLEN_MAX        =      32;

const ULONG DSP_RATE_DEFAULT       =   6;
const ULONG DSP_RATE_MIN           =   0;
const ULONG DSP_RATE_MAX           =  100;

const ULONG DSP_AUTHTYPE_DEFAULT   =       0;
const ULONG DSP_AUTHTYPE_MIN       =      0;
const ULONG DSP_AUTHTYPE_MAX       =      2;

const ULONG DSP_ENCRYPTKEY_DEFAULT  =        0;
const ULONG DSP_ENCRYPTKEY_MIN      =        0;
const ULONG DSP_ENCRYPTKEY_MAX      =        3;

const ULONG DSP_BOARDTYPE_DEFAULT  =        0;
const ULONG DSP_BOARDTYPE_MIN      =        0;
const ULONG DSP_BOARDTYPE_MAX      =        1;

const ULONG DSP_WEPMODE_DEFAULT  =        0;
const ULONG DSP_WEPMODE_MIN      =        0;
const ULONG DSP_WEPMODE_MAX      =        2;

const ULONG DSP_ENCRYPTLEN_DEFAULT   =       1;
const ULONG DSP_ENCRYPTLEN_MIN       =       1;
const ULONG DSP_ENCRYPTLEN_MAX       =       2;

const ULONG DSP_WEPUSE_DEFAULT       =   0;
const ULONG DSP_WEPUSE_MIN           =   0;
const ULONG DSP_WEPUSE_MAX           =   1;

const ULONG DSP_FRAGTHRES_DEFAULT    =  2342;
const ULONG DSP_FRAGTHRES_MIN        =   256;
const ULONG DSP_FRAGTHRES_MAX        =   2342;

const ULONG DSP_RTSTHRES_DEFAULT     =  2342;
const ULONG DSP_RTSTHRES_MIN         =  256;
const ULONG DSP_RTSTHRES_MAX         =  2342;

const ULONG DSP_ATIMWIN_DEFAULT      = 0;
const ULONG DSP_ATIMWIN_MIN          = 0;
const ULONG DSP_ATIMWIN_MAX          = 20;

const ULONG DSP_BEACONINTV_DEFAULT   =    100;
const ULONG DSP_BEACONINTV_MIN       =    20;
const ULONG DSP_BEACONINTV_MAX       =    1000;

const ULONG DSP_RETRYLONG_DEFAULT    =    4;
const ULONG DSP_RETRYLONG_MIN        =   0;
const ULONG DSP_RETRYLONG_MAX        =   16;

const ULONG DSP_RETRYSHORT_DEFAULT   =     7;
const ULONG DSP_RETRYSHORT_MIN       =    0;
const ULONG DSP_RETRYSHORT_MAX       =    16;

const ULONG DSP_RATE_TYPE_DEFAULT    =    4;
const ULONG DSP_RATE_TYPE_MIN        =    0;
const ULONG DSP_RATE_TYPE_MAX        =    12;

//const ULONG DSP_PREAMBLE_TYPE_DEFAULT    =    0;
//const ULONG DSP_PREAMBLE_TYPE_MIN        =    0;
//const ULONG DSP_PREAMBLE_TYPE_MAX        =    1;

const ULONG DSP_PSMODE_DEFAULT = 0	;
const ULONG DSP_PSMODE_MIN     = 0	;
const ULONG DSP_PSMODE_MAX     = 3	;

const ULONG DSP_FALLBACKUSE_DEFAULT       =   0;
const ULONG DSP_FALLBACKUSE_MIN           =   0;
const ULONG DSP_FALLBACKUSE_MAX           =   1;

const ULONG DSP_REGION_DEFAULT     =   0;
const ULONG DSP_REGION_MIN         =   0;
const ULONG DSP_REGION_MAX         =   6;

const ULONG DSP_ANTENNA_TYPE_DEFAULT    =    0;
const ULONG DSP_ANTENNA_TYPE_MIN        =    0;
const ULONG DSP_ANTENNA_TYPE_MAX        =    1;

const ULONG DSP_SELFTOCTS_TYPE_DEFAULT    =    0;
const ULONG DSP_SELFTOCTS_TYPE_MIN        =    0;
const ULONG DSP_SELFTOCTS_TYPE_MAX        =    1;

const ULONG DSP_WLANMODE_TYPE_DEFAULT    =    0;
const ULONG DSP_WLANMODE_TYPE_MIN        =    0;
const ULONG DSP_WLANMODE_TYPE_MAX        =    1;

const ULONG DSP_INTERNAL_SETTING_DEFAULT    =    0;
const ULONG DSP_INTERNAL_SETTING_MIN        =    0;
const ULONG DSP_INTERNAL_SETTING_MAX        =    1;

const ULONG DSP_WZCOPEN_TYPE_DEFAULT    =    0;
const ULONG DSP_WZCOPEN_TYPE_MIN        =    0;
const ULONG DSP_WZCOPEN_TYPE_MAX        =    1;
*/


#define DSP_CHANNEl_DEFAULT        1
#define DSP_CHANNEl_MIN            1
#define DSP_CHANNEl_MAX            14

#define DSP_MACMODE_DEFAULT        2
#define DSP_MACMODE_MIN            1
#define DSP_MACMODE_MAX            2

#define DSP_SSIDLEN_MAX            32

#define DSP_RATE_DEFAULT        6
#define DSP_RATE_MIN            0
#define DSP_RATE_MAX           100

#define DSP_AUTHTYPE_DEFAULT        0
#define DSP_AUTHTYPE_MIN           0
#define DSP_AUTHTYPE_MAX           2

#define DSP_ENCRYPTKEY_DEFAULT        0
#define DSP_ENCRYPTKEY_MIN            0
#define DSP_ENCRYPTKEY_MAX            3

#define DSP_BOARDTYPE_DEFAULT        0
#define DSP_BOARDTYPE_MIN            0
#define DSP_BOARDTYPE_MAX            1

#define DSP_WEPMODE_DEFAULT        0
#define DSP_WEPMODE_MIN            0
#define DSP_WEPMODE_MAX            2

#define DSP_ENCRYPTLEN_DEFAULT        1
#define DSP_ENCRYPTLEN_MIN            1
#define DSP_ENCRYPTLEN_MAX            2

#define DSP_WEPUSE_DEFAULT        0
#define DSP_WEPUSE_MIN            0
#define DSP_WEPUSE_MAX            1

#define DSP_FRAGTHRES_DEFAULT    2346
#define DSP_FRAGTHRES_MIN         256
#define DSP_FRAGTHRES_MAX         2346

#define DSP_RTSTHRES_DEFAULT     2347
#define DSP_RTSTHRES_MIN         256
#define DSP_RTSTHRES_MAX         2347

#define DSP_ATIMWIN_DEFAULT       0
#define DSP_ATIMWIN_MIN           0
#define DSP_ATIMWIN_MAX           20

#define DSP_BEACONINTV_DEFAULT     100
#define DSP_BEACONINTV_MIN         20
#define DSP_BEACONINTV_MAX         1000

#define DSP_RETRYLONG_DEFAULT      4
#define DSP_RETRYLONG_MIN         0
#define DSP_RETRYLONG_MAX         16

#define DSP_RETRYSHORT_DEFAULT      7
#define DSP_RETRYSHORT_MIN         0
#define DSP_RETRYSHORT_MAX         16

#define DSP_RATE_TYPE_DEFAULT      4
#define DSP_RATE_TYPE_MIN          0
#define DSP_RATE_TYPE_MAX          12

//#define DSP_PREAMBLE_TYPE_DEFAULT      0
//#define DSP_PREAMBLE_TYPE_MIN          0
//#define DSP_PREAMBLE_TYPE_MAX          1

#define DSP_PSMODE_DEFAULT  0	
#define DSP_PSMODE_MIN      0	
#define DSP_PSMODE_MAX      3	

#define DSP_FALLBACKUSE_DEFAULT        0
#define DSP_FALLBACKUSE_MIN            0
#define DSP_FALLBACKUSE_MAX            1

#define DSP_REGION_DEFAULT      1
#define DSP_REGION_MIN          0
#define DSP_REGION_MAX          3

#define DSP_ANTENNA_TYPE_DEFAULT      0
#define DSP_ANTENNA_TYPE_MIN          0
#define DSP_ANTENNA_TYPE_MAX          1

#define DSP_SELFTOCTS_TYPE_DEFAULT      0
#define DSP_SELFTOCTS_TYPE_MIN          0
#define DSP_SELFTOCTS_TYPE_MAX          1

#define DSP_WLANMODE_TYPE_DEFAULT      0
#define DSP_WLANMODE_TYPE_MIN          0
#define DSP_WLANMODE_TYPE_MAX          1

#define DSP_INTERNAL_SETTING_DEFAULT      0
#define DSP_INTERNAL_SETTING_MIN          0
#define DSP_INTERNAL_SETTING_MAX          1

#define DSP_WZCOPEN_TYPE_DEFAULT      0
#define DSP_WZCOPEN_TYPE_MIN          0
#define DSP_WZCOPEN_TYPE_MAX          1




/*--variables---------------------------------------------------------*/
/*--function prototypes-----------------------------------------------*/

#endif

