/************************************************************************
(c) 2003-04, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
	3DSP Corporation
	16271 Laguna Canyon Rd
	Irvine, CA 92618
	www.3dsp.com
**************************************************************************
$RCSfile: usbwlan_equates.h,v $ 
$Revision: 1.2 $ 
$Date: 2010/08/16 01:52:27 $
**************************************************************************/

#ifndef _USBWLAN_EQUATES_H
#define _USBWLAN_EQUATES_H

#include "precomp.h"
//-------------------------------------------------------------------------
// Ethernet Frame Sizes
//-------------------------------------------------------------------------
#define ETHERNET_ADDRESS_LENGTH         6
#define ETH_LENGTH_OF_ADDRESS			6
#define PROTOCOL_TYPE_LENGTH            2
#define ETHERNET_HEADER_SIZE            14
#define MINIMUM_ETHERNET_PACKET_SIZE    60

// max ethernet frame size (1500) + ethernet header (14)
#define MAXIMUM_ETHERNET_PACKET_SIZE    1514

#define TXALIGNMENT 2

#define IEEE802_1X_TYPE_LE              0x8E88
#define IEEE802_1X_TYPE_BE              0x888E


#define SIZEOF_TXPOWER_TABLE			66
#define MAX_MULTICAST_ADDRESSES         32
#define TCB_BUFFER_SIZE                 0xE0 // 224
#define COALESCE_BUFFER_SIZE            2048
//#define ETH_MAX_COPY_LENGTH             0x80 // 128

#define MAXIMUM_WLAN_PACKET_SIZE		2346
#define MINIMUM_WLAN_PACKET_SIZE		25

#define DMA_MAP_REGISTERS 1
#define DMA_SCATTER_GATHER 2

#define WEP_OVERHEAD   (8)    // IV(4) and ICV(4)
#define TKIP_OVERHEAD (12)    // IV(4), EIV(4) and ICV(4)
#define CCMP_OVERHEAD (16)    // CCMP Header(8) and MIC(8)
//NOTE: If any encryption overhead is added/changed so that
//      CCMP_OVERHEAD is not the largest value, then the
//      ETH_RX_BUFFER_STRUC definition MUST be updated or
//      else the Rx buffers will not be large enough!!!

// Make receive area 1536 for 16 bit alignment.
#define RCB_BUFFER_SIZE       ((MAXIMUM_ETHERNET_PACKET_SIZE + 15) / 16) * 16

//- Area reserved for all Non Transmit command blocks
//#define MAX_NON_TX_CB_AREA              512

//-------------------------------------------------------------------------
// Ndis/Adapter driver constants
//-------------------------------------------------------------------------
#define MAX_PHYS_DESC               16
#define MAX_RECEIVE_DESCRIPTORS     1024 // 0x400
#define NUM_RMD                     10

//--------------------------------------------------------------------------
// System wide Equates
//--------------------------------------------------------------------------
//#define MAX_NUMBER_OF_EISA_SLOTS    15
//#define MAX_NUMBER_OF_PCI_SLOTS     15

//--------------------------------------------------------------------------
//    Equates Added for NDIS 4
//--------------------------------------------------------------------------
#define  NUM_BYTES_PROTOCOL_RESERVED_SECTION    16
#define  MAX_NUM_ALLOCATED_RFDS                 64
#define  MIN_NUM_RFD                            4
#define  MAX_ARRAY_SEND_PACKETS                 8
// limit our receive routine to indicating this many at a time
#define  MAX_ARRAY_RECEIVE_PACKETS              16
#define  MAC_RESERVED_SWRFDPTR                  0
#define  MAX_PACKETS_TO_ADD                     32

#endif      // _EQUATES_H
