#ifndef DSP_CHARACT_H
   #define DSP_CHARACT_H

/***********************************************************************
 * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
 *
 * FILENAME:     DSP_Charact.h     CURRENT VERSION: 1.00.01
 * PURPOSE:      Constants and Specialized Traits of DSP driver.
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
// Driver version
#define	DSP_VERSION_MAJOR			5
#define	DSP_VERSION_MINOR			0

// Medium Specific
//#define	DSP_MEDIUM_TYPE			NdisMedium802_3 
#define	DSP_802_3_MAX_LIST_SIZE	32 /* NDIS Test requirement (>=) */				

// OID Constants
#define	DSP_MAX_LOOKAHEAD		256
#define	DSP_MAX_FRAMESIZE		1500		// w/o header
#define	DSP_MAX_TOTAL_SIZE		1514		// incl. header
#define	DSP_MAX_ETHER_SIZE		1536		// incl. header
#define	DSP_ETHER_HEAD_SIZE		12 		// incl. header
#define	DSP_MAX_LINKSPEED		10000000L
#define	DSP_TRANSMIT_BLOCK		256
#define	DSP_RECEIVE_BLOCK		256

#define DSP_TX_BUFFER_SPACE	16000				
#define DSP_RX_BUFFER_SPACE	16000

#define	DSP_MAX_SEND_PACKETS	4

#define	DSP_VENDOR_DESCRIPTION	"3DSP USB Wireless A+G Notebook Adapter"//"DSP NDIS Miniport Driver"
#define	DSP_VENDOR_VERSION	"02.01.01.02"  // Driver version

#define	DSP_TX_HEAD_IRPPOOL_SIZE		1
#define	DSP_TX_IRPPOOL_SIZE			8
#define	DSP_RX_IRPPOOL_SIZE			8
#define	DSP_RX_IRPPOOL_SIZE_USB20		1

#define	DSP_INT_IRPPOOL_SIZE			8
#define DSP_CNTRL_IRPPOOL_SIZE 		8



#define	DSP_TX_HEAP_COUNTS	(DSP_TX_IRPPOOL_SIZE)


	//Jakio20070420: set critical value for bak packets
#define  MAX_RETRY_MSDU_NUM		DSP_TX_IRPPOOL_SIZE

#define	DSP_RX_HEAP_COUNTS	(DSP_RX_IRPPOOL_SIZE)
#define	DSP_INT_HEAP_COUNTS	(DSP_INT_IRPPOOL_SIZE)
#define DSP_CNTRL_HEAP_COUNTS  (DSP_CNTRL_IRPPOOL_SIZE)

#define	DSP_TX_URB_COUNTS	(DSP_TX_IRPPOOL_SIZE)
#define	DSP_RX_URB_COUNTS	(DSP_RX_IRPPOOL_SIZE)
#define	DSP_INT_URB_COUNTS	(DSP_INT_IRPPOOL_SIZE)
#define DSP_CNTRL_URB_COUNTS  (DSP_CNTRL_IRPPOOL_SIZE)

#define DSP_PROTOCOL_RESERVED_LEN  (16)
#define DSP_PACKET_HEADER_SIZE     (14)

#define DSP_PENDING_COUNT               (5)

// TODO: 
// Add other #defines and/or KNdisAdapterTraits<DSPAdapter> specializations as needed

/*--constants and types------------------------------------------------*/

/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/

#endif 
