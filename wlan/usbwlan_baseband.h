/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:

 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************
 *
 * Wlan_dsp_bb.c:
 *
 * created:
 * 	??/??/????	3DSP/Wirpo/DCM
 *
 * description:
 * 	This header file,contains the register description as specified  datasheet
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

/*
 * REVISION HISTORY:
 *
 * 	Revision:		1.1.1.1 
 *	Date:			2006/01/10
 *	modified:		fdeng
 *	description:	Add some macro define 
 *
 */

#ifndef _BASEBAND_H_
#define _BASEBAND_H_


#include "precomp.h"


//This defines needs to be changed according to the wlan_dsp baseband
#define 	CCA_MODE_SUPPORTED				0x07  
#define 	NUM_OF_SUPPORTED_PWR_LEVEL		0x08  



/********************************************************************
 *																	|
 *				Timings Register 1 (timingsReg1) Field Names		|
 *																	|
 *******************************************************************/
 
/*
 * [DCM]:
 * In WLAN_STA_ChipDataSsheet.pdf MAC Clock operation frequency is 80MHz
 * BootUp and Default frequency is 16MHz
 *
 * Rahul : sp20 runs at 64 mhz
 */

 
#ifdef MAC_DBG_MACCLK_80MHZ
#define	BB_CLK_FREQ						0x50 //80Mhz
#else
#define	BB_CLK_FREQ						0x40
#endif

// TODO: 	why different ?			fdeng
/*
 * in 3DSP-00135 :
 *
 * This is the baseband frequency value, on which the MAC core works.  
 * On reset, the `RESET_BBCLK_FREQ value gets loaded in this field.  
 * Default = 0x3c = 0b0111100.
 */



/*
 * MAC will switch on the baseband txDelaySlot microseconds before the 
 * actual slot in air for a transmission.  On reset, the `TX_DELAY_SLOT 
 * value gets loaded in this field.  Default = 0x3 = 0b0011.
 */
#define TX_DELAY_SLOT					0x03


/*
 * MAC will switch on the baseband txDelaySIFS microseconds before the actual
 * SIFS boundary in air for a transmission.  On reset, the `TX_DELAY_SIFS 
 * value gets loaded in this field.  Default = 0x3 = 0b0011.
 */
#define TX_DELAY_SIFS					0x03

/*
 * MAC uses the rxDelayB microseconds value to compensate internal timings 
 * for the receive chain delay (this is RF processing  + Baseband processing 
 * Delay for a 802.11b baseband and radio). On reset, the `RX_DELAY_B value
 * gets loaded in this field.  Default = 0x6 = 0b0110.
 */
#define RX_DELAY_B						0x06
#define RX_DELAY_B_CHANGE				0x08

/*
 * MAC uses the rxDelayA microseconds value to compensate internal timings 
 * for the receive chain delay (this is RF processing  + Baseband processing 
 * Delay for a 802.11a baseband and radio). On reset, the `RX_DELAY_A value 
 * gets loaded in this field.  Default = 0xc = 0b1100.
 */
#define RX_DELAY_A						0x0A

//v4chip
 #define   	MAC_TIME						0x1
 #define 		RXDELAYA_US                         10
#define 		RXDELAYB_US                         06
#define 		AIFSN_V3                             2//for all modulations


#define OFFSET_TX_DELAY_SLOT			7
#define OFFSET_TX_DELAY_SIFS			11
#define OFFSET_RX_DELAY_B				15
#define OFFSET_RX_DELAY_A				19

//add for v4chip
#define	BB_CLK_FREQ						0x40	// for 64mhz
//#if defined (STA_V3_L0)
	#define	V4T_CCA_A							473 // BBCLK count for 11a. 
	#define	V4T_CCA_B							256 // BBCLK count for 11g/b. 
//#else
	#define	V2T_CCA_A							493 // BBCLK count for 11a. 
	#define	V2T_CCA_B							243 // BBCLK count for 11g/b. 
//#endif



/********************************************************************
 *																	|
 *				Timings Register 2 (timingsReg2) Field Names		|
 *																	|
 *******************************************************************/
 
#ifdef MAC_DBG_MACCLK_80MHZ
/*
 * The value of this field is in number of BBCLK cycles.  It is the delay 
 * between the last bit of the frame in air and the falling edge of CCA 
 * from the 802.11a baseband.
 * MAC uses the tCCAa to compensate its internal timings and maintain 
 * IFS timings in sync with the medium.
 * On reset, the `T_CCA_A value gets loaded in this field.  Default = 0.
 */
#define	T_CCA_A							616 //(RX_DELAY_A * BB_CLK_FREQ)
/*
 * The value of this field is in number of BBCLK cycles.  It is the delay 
 * between the last bit of the frame in air and the falling edge of CCA 
 * from the 802.11b baseband.
 * MAC uses the tCCAb to compensate its internal timings and maintain 
 * IFS timings in sync with the medium.
 * On reset, the `T_CCA_B value gets loaded in this field.  Default = 0.
 */
#define	T_CCA_B							350 //(RX_DELAY_B * BB_CLK_FREQ) 
#else
#define	T_CCA_A							493 //(RX_DELAY_A * BB_CLK_FREQ)
#define	T_CCA_B							280 //(RX_DELAY_B * BB_CLK_FREQ) 
#endif


/*
 * This field is programmed with the SLOT time to be used for frame exchange.
 * It should be programmed to:
 * 	1.	20 s for 802.11b mode of operation of the MAC.
 * 	2.	9 s for 802.11a mode of operation of the MAC.
 * 	3.	20 s for normal 802.11g mode of operation of the MAC.
 * 	4.	9 s for shortSlot option of 802.11g mode of operation of the MAC.
 * On reset, the `SLOT_TIME_DOT11B value gets loaded in this field.
 *
 * [DCM]:
 * Removed compiler directives under which SLOT_TIME value used to vary
 * as per 802.11 type(a/b/g).This is done to support dynamic configuration
 * of baseband type 
 */

#define SLOT_TIME_DOT11A						0x9
#define SLOT_TIME_DOT11B						0x14
// refer section 19.8.4 of 802.11g Specs(9 microseconds)
#define SLOT_TIME_SHORT_DOT11G					0x09
// refer section 19.8.4 of 802.11g Specs(20 microseconds)
#define SLOT_TIME_LONG_DOT11G					0x14


#define OFFSET_T_CCA_B				12
#define OFFSET_SLOT_TIME			24



 
/* 
 * Prototype Declarations
 */


#endif /* _BASEBAND_H_ */

