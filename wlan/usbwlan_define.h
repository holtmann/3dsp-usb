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
 *************************************************************************/
#ifndef _USBWLAN_DEFINE_H_
#define _USBWLAN_DEFINE_H_

#define DEBUG_OPEN
#define  DEBUG_OPEN__WLAN





#define ANTENNA_DIVERSITY

#define SET_KEY_WITH_8051
//#define DOWNLOAD_CODE_WITH_H_FILE_MODE
#define NDIS50_MINIPORT
//#define DOWNLOAD_BIN_WITH_FILE_MODE

#define  NEW_SUPRISING_REMOVED			//Justin: open for suprise remove test
#define  NOTIFY_PACKET_ENABLE
#define  DSP_IBSS_OPEN
#define  NEW_RETRY_LIMIT

#define	DSP_DEVICE_NDISMREG
//#define	CHECK_BULK_STALL_ENABLE
//#define	DSP_TX_SUSPEND_MODE
#define DSP_SINGLE_MODE_FOR_HEAD

#ifndef  DSP_TX_SUSPEND_MODE
	#define	SET_TX_FRAG_WITH_DMA2
	//#define	SEND_HDR_WITH_BULKOUT1
	//#define	SERIALIZE_HEADER_DATA
#endif

#ifndef DSP_WPA2
#define	DSP_WPA2 
#endif

#ifndef WPA2_SUPPORT
#define WPA2_SUPPORT
#endif

#ifndef PMK_CACHING_SUPPORT
#define PMK_CACHING_SUPPORT
#endif

#ifndef ROAMING_SUPPORT
#define ROAMING_SUPPORT
#endif

#ifndef DEBUG_RSSI
//#define DEBUG_RSSI
#endif


//justin:	071229.	add to sopport download dsp and 8051 codes from file 'Sp20code.h'
//				Download dsp and 8051 codes from .bin files if not define this
#ifndef DOWNLOAD_BIN_WITH_FILE_MODE
#define DOWNLOAD_BIN_WITH_FILE_MODE
#endif

#ifndef RX_RESUBMIT_IRP_B4_PROCESSING
//#define RX_RESUBMIT_IRP_B4_PROCESSING
#endif

#define  PARSE_REGISTRY

#define  READ_WRITE_CTL_REG_NO_EVERY_TIME 

#define  DEBUG_FOR_CLOSE_ASSERT

#define  OPEN_DEVICE_CONTROL_INTERFACE
//#define  PRINT_PING_REQ_NUMBER

#define  DSP_ASIC_DEBUG_FLAG
#define  DSP_ASIC_COMBO_FLAG


#endif

