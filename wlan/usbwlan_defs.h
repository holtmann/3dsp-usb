/************************************************************************
(c) 2003-04, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
	3DSP Corporation
	16271 Laguna Canyon Rd
	Irvine, CA 92618
	www.3dsp.com
**************************************************************************/
/*
 * description:
 * 	This Header file contains the defines and structures needed for hardware access
 *
 */

#ifndef _USBWLAN_DEFS_H
#define _USBWLAN_DEFS_H

#include "usbwlan_equates.h"
#include "usbwlan_wlan.h"
#include "tdsp_basetypes.h"

#include "precomp.h"
//-------------------------------------------------------------------------
// WLAN_STA Action Commands
//-------------------------------------------------------------------------
/*
#define CB_NOP                  0
#define CB_IA_ADDRESS           1
#define CB_CONFIGURE            2
#define CB_MULTICAST            3
#define CB_TRANSMIT             4
#define CB_LOAD_MICROCODE       5
#define CB_DUMP                 6
#define CB_DIAGNOSE             7


//-------------------------------------------------------------------------
// Command Block (CB) Field Definitions
//-------------------------------------------------------------------------
//- CB Command Word
#define CB_EL_BIT               BIT_15          // CB EL Bit
#define CB_S_BIT                BIT_14          // CB Suspend Bit
#define CB_I_BIT                BIT_13          // CB Interrupt Bit
#define CB_TX_SF_BIT            BIT_3           // TX CB Flexible Mode
#define CB_CMD_MASK             BIT_0_2         // CB 3-bit CMD Mask
*/
//add for v4chip
#define WLS_MAC_TSFOFFSETLO_WD           0x4068
#define WLS_MAC_TSFOFFSETHI_WD           0x406C
#define WLS_MAC_RANDSEED_WD              0x4070
#define WLS_MAC_GENTIMER_WD              0x4074

//#ifdef STA_V3_L0
#define BBREG_ACCESS_EN                  1
#define BBREG_ACCESS_DIS                 0
#define BBREG_ACCESS_WRITE               0
#define BBREG_ACCESS_READ                1
#define MAC_BBREG_ACCESSS(Enable, Direction, Data, BBRegNum) \
			((Enable << 17) | (Direction << 16) | ((Data & 0xff) << 8) | (BBRegNum & 0xff))


//- CB Status Word
#define CB_STATUS_MASK          BIT_12_15       // CB Status Mask (4-bits)
#define CB_STATUS_COMPLETE      BIT_15          // CB Complete Bit
#define CB_STATUS_OK            BIT_13          // CB OK Bit
#define CB_STATUS_UNDERRUN      BIT_12          // CB A Bit
#define CB_STATUS_FAIL          BIT_11          // CB Fail (F) Bit
/*
//misc command bits
#define CB_TX_EOF_BIT           BIT_15          // TX CB/TBD EOF Bit
*/

//-------------------------------------------------------------------------
// Receive Frame Descriptor Fields
//-------------------------------------------------------------------------

//- RFD Status Bits
#define RFD_RECEIVE_COLLISION   BIT_0           // Collision detected on Receive
#define RFD_IA_MATCH            BIT_1           // Indv Address Match Bit
#define RFD_RX_ERR              BIT_4           // RX_ERR pin on Phy was set
#define RFD_FRAME_TOO_SHORT     BIT_7           // Receive Frame Short
#define RFD_DMA_OVERRUN         BIT_8           // Receive DMA Overrun
#define RFD_NO_RESOURCES        BIT_9           // No Buffer Space
#define RFD_ALIGNMENT_ERROR     BIT_10          // Alignment Error
#define RFD_CRC_ERROR           BIT_11          // CRC Error
#define RFD_STATUS_OK           BIT_13          // RFD OK Bit
#define RFD_STATUS_COMPLETE     BIT_15          // RFD Complete Bit
/*
//- RFD Command Bits
#define RFD_EL_BIT              BIT_15          // RFD EL Bit
#define RFD_S_BIT               BIT_14          // RFD Suspend Bit
#define RFD_H_BIT               BIT_4           // Header RFD Bit
#define RFD_SF_BIT              BIT_3           // RFD Flexible Mode

//- RFD misc bits
#define RFD_EOF_BIT             BIT_15          // RFD End-Of-Frame Bit
#define RFD_F_BIT               BIT_14          // RFD Buffer Fetch Bit
#define RFD_ACT_COUNT_MASK      BIT_0_13        // RFD Actual Count Mask
#define RFD_HEADER_SIZE         0x10            // Size of RFD Header (16 bytes)

//-------------------------------------------------------------------------
// Receive Buffer Descriptor Fields
//-------------------------------------------------------------------------
#define RBD_EOF_BIT             BIT_15          // RBD End-Of-Frame Bit
#define RBD_F_BIT               BIT_14          // RBD Buffer Fetch Bit
#define RBD_ACT_COUNT_MASK      BIT_0_13        // RBD Actual Count Mask

#define SIZE_FIELD_MASK         BIT_0_13        // Size of the associated buffer
#define RBD_EL_BIT              BIT_15          // RBD EL Bit
*/
//#define SHUTTLE_BYTE_XFER_LENGTH 1024
#define SHUTTLE_BYTE_XFER_LENGTH 2360

#define SCRATCH_MEM_BASE               0x2000
#define BASEBAND_TEST_SCRATCH_OFFSET   0x0c00
#define BASEBAND_TEST_CP_BD_NUM_OF_ENTRIES  25

#define SCRATCH_XFER_LENGTH 1024
#define SCRATCH_XFER_OFFSET 1024
#define SCRATCH_INJECT_ABMEM_OFFSET 580

#define DEFAULT_TX_FIFO_LIMIT       0x08
#define DEFAULT_RX_FIFO_LIMIT       0x08
#define DEFAULT_UNDERRUN_RETRY      0x01
#define MINIMUM_RX_PACKET_SIZE_IN_HOST	0x1c

//Justin:	080102.		read eeprom from program which has been filled by 8051
#define LOCAL_PROGRAM_MEM_BASE               0x8000
#define POWER_TABLE_PROGRAM_OFFSET   0x0180
#define POWER_TABLE_VALID_FLAG_OFFSET   0xb4
#define POWER_TABLE_VALID_FLAG_LEN	     4

// 20081020 add by glen for support subsystemID extend defined by EEPROM data
#define EEPROM_OFFSET_SUBSYSTEMID    0x011A
/*
USB INTERFACE
Board    Build     Chip  Inter-  Rom   Chip   Eeprom      BBT      Base Eeprom
Rf       process   Rev   face    Ver   ID     SubsysID    ver      File Name
=======================================================================================
Airoha   0.18 mic  V4    USB     V3    4      0x0100      3.0.4    eeprom8_ar2230_v4_usb.in
Airoha   0.18 mic  V4    USB     V3    4      0x0200      3.2.0    eeprom8_ar2230_v4_usbMini.in

Airoha   0.15 mic  V4    USB     V4b   4      0x0500      3.1.2    eeprom8_ar2230_v4_v4brom_usb.in
Airoha   0.15 mic  V4    USB     V4b   4      0x0300      3.2.0    eeprom8_ar2230_v4_v4brom_usbMini.in
  
SST      0.18 mic  V4    USB     V4a   4      0x0400      3.1.2    eeprom8_sst_v4_usb.in
SST      0.18 mic  V4    USB     V4a   4      0x0700      3.2.0    eeprom8_sst_v4_usbMini.in

SST      0.15 mic  V4    USB     V4b   4      0x0600      3.1.2    eeprom8_sst_v4_v4brom_usb.in
SST      0.15 mic  V4    USB     V4b   4      0x0800      3.2.0    eeprom8_sst_v4_v4brom_usbMini.in
*/
#define EEPROM8_AR2230_V4_USB            0x0100 
#define EEPROM8_AR2230_V4_USBMINI        0x0200 
#define EEPROM8_AR2230_V4_V4BROM_USB     0x0500 
#define EEPROM8_AR2230_V4_V4BROM_USBMINI 0x0300 
#define EEPROM8_SST_V4_USB               0x0400 
#define EEPROM8_SST_V4_USBMINI           0x0700 
#define EEPROM8_SST_V4_V4BROM_USB        0x0600 
#define EEPROM8_SST_V4_V4BROM_USBMINI    0x0800 
// 20081020 add by glen end


#pragma pack(1)

#define THREADTIMEOUT 1000
#define ALLOCQUESIZE 100


#define SMELOOPTIME 30 //milliseconds


#define VERSION_1   0x01

#define MAC_SIGNATURE_VALUE 0x0A0B0C0D
#define DSAP        0xAA
#define SSAP        0xAA
#define SNAPCONTROL 0x03
#define SNAPORGCODE 0x0

#define SNAPORGCODE_802_1H	0x00,0x00,0xF8
#define SNAPORGCODE_RFC1042	0x00,0x00,0x00
#define SNAPTYPELENGTH_IPX		0x8137
#define SNAPTYPELENGTH_AARP	0x80f3


#define POWERTABLEDEFAULT0 0x30303030
#define POWERTABLEDEFAULT1 0x2828221a
#define POWERTABLEDEFAULT2 0x2828221a

#define POWERTABLEDEFAULT3 0x30303030
#define POWERTABLEDEFAULT4 0x2828221a
#define POWERTABLEDEFAULT5 0x2828221a


//Jakio20070411: self defined error bit
#define VCMD_READ_3DSP_REG_ERROR  BIT0


#define WEP_SET_OK					   BIT16     //Jakio20070426 add here to test wep


//-------------------------------------------------------------------------
// Ethernet Frame Structure
//-------------------------------------------------------------------------
//- Ethernet 6-byte Address
typedef struct _ETH_ADDRESS_STRUC {
    UINT8       EthNodeAddress[ETHERNET_ADDRESS_LENGTH];
} ETH_ADDRESS_STRUC, *PETH_ADDRESS_STRUC;


//- Ethernet 14-byte Header
typedef struct _ETH_HEADER_STRUC {
    UINT8       Destination[ETHERNET_ADDRESS_LENGTH];
    UINT8       Source[ETHERNET_ADDRESS_LENGTH];
    UINT16      TypeLength;
} ETH_HEADER_STRUC, *PETH_HEADER_STRUC;


//- Ethernet Buffer (Including Ethernet Header) for Transmits
typedef struct _ETH_TX_BUFFER_STRUC {
    ETH_HEADER_STRUC    TxMacHeader;
    UINT8               TxBufferData[(TCB_BUFFER_SIZE - sizeof(ETH_HEADER_STRUC))];
} ETH_TX_BUFFER_STRUC, *PETH_TX_BUFFER_STRUC;

//- Ethernet Buffer (Including Ethernet Header) for Receives,
//- Must take into account encryption overhead that is still in buffer when this buffer is used.
typedef struct _ETH_RX_BUFFER_STRUC {
    ETH_HEADER_STRUC    RxMacHeader;
    UINT8               RxBufferData[(RCB_BUFFER_SIZE + CCMP_OVERHEAD - sizeof(ETH_HEADER_STRUC))];
} ETH_RX_BUFFER_STRUC, *PETH_RX_BUFFER_STRUC;


//SNAP HEADER
typedef struct _SNAP_HEADER_STRUC {
    UINT8       dsap;
    UINT8       ssap;
    UINT8       control;
    UINT8       orgcode1;
    UINT8       orgcode2;
    UINT8       orgcode3;
    UINT16      TypeLength;
} SNAP_HEADER_STRUC, *PSNAP_HEADER_STRUC;

typedef enum {	
	MACSW_IN_NONAP_STA = 0,
	MACSW_IBSS
} macswNonAPType_t;

typedef enum {
	MACSW_QOS_BEACON = 0,
	MACSW_QOS_MULTICAST,
	MACSW_QOS_CFP,
	MACSW_QOS_CP
} macswQoSTC_t;

typedef enum {
	MACSW_TX_COMPLETED = 0,
	MACSW_TX_REQ_TIMEOUT,
	MACSW_TX_ERROR
} macswTxReqCbStatus_t;

typedef enum
{
	MACSW_REORDERABLE_MULTICAST,
	MACSW_STRICTLY_ORDERED
}macswServiceClass_t; /* It is same as as service_class_t */

#if 0 // TODO: Jackie
#define MP_FRAG_ELEMENT SCATTER_GATHER_ELEMENT 
#define NIC_MAX_PHYS_BUF_COUNT 8

typedef struct _MP_FRAG_LIST {
    UINT32 NumberOfElements;
    ULONG_PTR Reserved;
    MP_FRAG_ELEMENT Elements[NIC_MAX_PHYS_BUF_COUNT];
} MP_FRAG_LIST, *PMP_FRAG_LIST;
#endif

//-------------------------------------------------------------------------
// Xfer pad
//-------------------------------------------------------------------------

typedef struct _XFER_PAD_STRUC {
    UINT8       pad[SHUTTLE_BYTE_XFER_LENGTH];            //xfer pad
} XFER_PAD_STRUC, *PXFER_PAD_STRUC;

//-------------------------------------------------------------------------
// Rcv pad
//-------------------------------------------------------------------------

typedef struct _RCV_PAD_STRUC {
    UINT8       pad[SHUTTLE_BYTE_XFER_LENGTH];            //receive pad
} RCV_PAD_STRUC, *PRCV_PAD_STRUC;

typedef struct _allocque {
   UINT32 *orgVAHndl;
   UINT32 LOWorgPAHndl;
   UINT32 HIGHorgPAHndl;
   UINT32 sizeInBytes;
   BOOLEAN  cached;
} allocque, *pallocque;


/*
*
* UNIPHY2
*
*
********************************************************/

typedef struct _WLSCSR_STRUC {
    
    UINT32      WLSControl0;                   //0x00
    UINT32      WLSNumber_of_Retry; 
    UINT32      WLSInterrupt_Enable; 
    UINT32      WLSStatus; 
    UINT32      WLSStatus_Clear;               //0x10
    UINT32      WLS_ChipID;                    //0x14
    UINT32      Reserved2;
    UINT32      Reserved3;
    UINT32      WLSDMA0_src_addr;              //0x20
    UINT32      WLSDMA0_dst_addr;
    UINT32      WLSDMA0_control;
    UINT32      WLSPCI_system_shuttle_port_address;
    UINT32      WLSDMA1_Host_base_addr;        //0x30
    UINT32      WLSDMA1_Abort_Word_addr;
    UINT32      Reserved4;
    UINT32      Reserved5;
    UINT32      FUNCTION_event_clr;            //0x40
    UINT32      FUNCTION_event_mask;
    UINT32      FUNCTION_present_state;
    UINT32      FUNCTION_force_event;
    UINT32      WLS_MAC_HOST_DMA0_addr8;       //0x50
    UINT32      WLS_MAC_HOST_DMA0_size8;
    UINT32      WLS_MAC_HOST_DMA0_addr7;
    UINT32      WLS_MAC_HOST_DMA0_size7;
    UINT32      WLS_MAC_HOST_DMA0_addr6;       //0x60
    UINT32      WLS_MAC_HOST_DMA0_size6;
    UINT32      WLS_MAC_HOST_DMA0_addr5;
    UINT32      WLS_MAC_HOST_DMA0_size5;    
    UINT32      WLS_MAC_HOST_DMA0_addr4;       //0x70
    UINT32      WLS_MAC_HOST_DMA0_size4;
    UINT32      WLS_MAC_HOST_DMA0_addr3;
    UINT32      WLS_MAC_HOST_DMA0_size3;
    UINT32      WLS_MAC_HOST_DMA0_addr2;       //0x80
    UINT32      WLS_MAC_HOST_DMA0_size2;
    UINT32      WLS_MAC_HOST_DMA0_addr1;
    UINT32      WLS_MAC_HOST_DMA0_size1;
    UINT32      WLS_MAC_HOST_DMA0_addr0;       //0x90
    UINT32      WLS_MAC_HOST_DMA0_size0;
    UINT32      WLS_MAC_HOST_DMA0_control;  
    UINT32      WLS_MAC_HOST_DMA1_addr2;
    UINT32      WLS_MAC_HOST_DMA1_size2;       //0xA0
    UINT32      WLS_MAC_HOST_DMA1_addr1;
    UINT32      WLS_MAC_HOST_DMA1_size1;
    UINT32      WLS_MAC_HOST_DMA1_addr0;
    UINT32      WLS_MAC_HOST_DMA1_size0;       //0xB0 
    UINT32      WLS_MAC_HOST_DMA1_control;
    UINT32      WLS_MAC_HOST_DMA0_reenable;
    UINT32      WLS_MAC_HOST_DMA1_reenable;

} WLSCSR_STRUC;

typedef struct _MAC_STRUC1 {
    UINT32      WLSsignature;
    UINT32      WLSversion;
    UINT32      WLSmacAddrLo;
    UINT32      WLSmacAddrHi;
    UINT32      WLSbssIDLo;
    UINT32      WLSbssIDHi;
    UINT32      WLSstateControl;
    UINT32      WLScontrol;
    UINT32      WLSprobeTimer;
    UINT32      WLSbeaconInterval;
    UINT32      WLStxBcnLength;
    UINT32      WLSscanChTimer_atim;
    UINT32      WLSrtsThreshRtLim;
    UINT32      WLSintEventSet;
    UINT32      WLSintEventClear;
    UINT32      WLSintMask;
    UINT32      WLSstatus;
    UINT32      WLStsfLo;
    UINT32      WLStsfHi;
    UINT32      WLScfp;
    UINT32      WLStxFIFOSize;
    UINT32      WLSfifoWatermark;
    UINT32      WLSfifoSel;
    UINT32      WLSfifoStatus;
    UINT32      WLStimingsReg1;
    UINT32      WLStimingsReg2;
    UINT32      WLStsfOffsetLo;
    UINT32      WLStsfOffsetHi;
    UINT32      WLSrandSeed;
    UINT32      WLSgenTimer;
    UINT32      WLSBBAccess; //Change
    UINT32      WLSradioAccess;
    UINT32      WLSIntBBMask; //
    UINT32      WLStscPnLmtRched0;
    UINT32      WLStscPnLmtRched1;
    UINT32      reserved1;
    UINT32      WLSwepKey0;
    UINT32      WLSwepKey1;
    UINT32      WLSwepKey2;
    UINT32      WLSwepKey3;
    UINT32      WLSwepKeyAddr;
    UINT32      WLSwepKeyCntlAddr;
    UINT32      WLSkeySize;//
    UINT32      WLSkeyOperationCntl;
    UINT32      WLStxFrgCount;
    UINT32      WLSrxFrgCount;
    UINT32      WLSbcnBOCnt;
    UINT32      WLSpriBOCnt;
    UINT32      WLStaForNoKeyLow;
    UINT32      WLStaForNoKeyHi; //change
    UINT32      WLScpBOCnt;
    UINT32      WLSnavCWCnt;
    UINT32      WLSdebug0;
    UINT32      WLSdebug1;
    UINT32      WLSdebug2;
    UINT32      WLSledControl;
    UINT32      WLSstaRetryCnt;
    UINT32      WLScpRetryCnt;
    UINT32      WLScfpRetryCnt;
    UINT32      WLSperfCnt4;//
    UINT32      WLSperfCnt0;
    UINT32      WLSperfCnt1;
    UINT32      WLSperfCnt2;
    UINT32      WLSperfCnt3;
} MAC_STRUC1;

typedef struct _MAC_STRUC2 {
    UINT32      BBReg00_03;
    UINT32      BBReg04_07;
    UINT32      BBReg08_11;
    UINT32      BBReg12_15;
    UINT32      BBReg16_19;
    UINT32      BBReg20_23;
    UINT32      BBReg24_27;
    UINT32      BBReg28_31;
    UINT32      BBReg32_35;
    UINT32      BBReg36_38;
} MAC_STRUC2;

typedef struct _MAC_STRUC3 {
    UINT32      txFIFOWrPort_txFIFORdPort;
} MAC_STRUC3;

typedef struct _MAC_STRUC4 {
    UINT32      rxFIFORdPort_rxFIFOWrPort;
} MAC_STRUC4;




typedef struct _DUMMYa {
    UINT8    da[0x200 - sizeof(MAC_STRUC1)];
} DUMMYa_STRUC;

typedef struct _DUMMYb {
    UINT8    db[0x200 - sizeof(MAC_STRUC2)];
} DUMMYb_STRUC;

typedef struct _DUMMYc {
    UINT8    db[0x400 - sizeof(MAC_STRUC3)];
} DUMMYc_STRUC;

typedef struct _DUMMYd {
    UINT8    db[0x3800 - sizeof(MAC_STRUC4)];
} DUMMYd_STRUC;

typedef struct _MAC_STRUC {
MAC_STRUC1    mac1;
DUMMYa_STRUC  dumba;
MAC_STRUC2    mac2;
DUMMYb_STRUC  dumbb;
MAC_STRUC3    mac3;
DUMMYc_STRUC  dumbc;
MAC_STRUC4    mac4;
DUMMYd_STRUC  dumbd;
} MAC_STRUC;
//BUGBUG: sizeof(MAC_STRUC3/4) probably is not the actual value here

typedef struct _DUMMY1 {
    UINT8    d[0x2000 - sizeof(WLSCSR_STRUC)];
} DUMMY1_STRUC;

#define SCRATCHRAM_SIZE     0x1000
typedef struct _SCRATCH {
    UINT8    scratchram[SCRATCHRAM_SIZE];
} SCRATCH_STRUC;

typedef struct _DUMMY1_5 {
    UINT8  d1_5[(sizeof(SCRATCH_STRUC) + sizeof(DUMMY1_STRUC) + sizeof(WLSCSR_STRUC))];
} DUMMY1_5_STRUC;

typedef struct _DUMMY2 {
    UINT8  d2[0x4000 - (sizeof(DUMMY1_5_STRUC))];
} DUMMY2_STRUC;


typedef struct _THEMAP_STRUC {
    WLSCSR_STRUC      CSRAddress;
    DUMMY1_STRUC   dummy1;
    SCRATCH_STRUC  scratchAddress;
    DUMMY2_STRUC   dummy2;
    MAC_STRUC      MACAddress;
} THEMAP_STRUC, *PTHEMAP_STRUC;



//-------------------------------------------------------------------------
// Error Counters
//-------------------------------------------------------------------------
/*
typedef struct _ERR_COUNT_STRUC {
    UINT32       XmtGoodFrames;          // Good frames transmitted
    UINT32       XmtMaxCollisions;       // Fatal frames -- had max collisions
    UINT32       XmtLateCollisions;      // Fatal frames -- had a late coll.
    UINT32       XmtUnderruns;           // Transmit underruns (fatal or re-transmit)
    UINT32       XmtLostCRS;             // Frames transmitted without CRS
    UINT32       XmtDeferred;            // Deferred transmits
    UINT32       XmtSingleCollision;     // Transmits that had 1 and only 1 coll.
    UINT32       XmtMultCollisions;      // Transmits that had multiple coll.
    UINT32       XmtTotalCollisions;     // Transmits that had 1+ collisions.
    UINT32       RcvGoodFrames;          // Good frames received
    UINT32       RcvCrcErrors;           // Aligned frames that had a CRC error
    UINT32       RcvAlignmentErrors;     // Receives that had alignment errors
    UINT32       RcvResourceErrors;      // Good frame dropped due to lack of resources
    UINT32       RcvOverrunErrors;       // Overrun errors - bus was busy
    UINT32       RcvCdtErrors;           // Received frames that encountered coll.
    UINT32       RcvShortFrames;         // Received frames that were to short
//    UINT32       CommandComplete;        // A005h indicates cmd completion
} ERR_COUNT_STRUC, *PERR_COUNT_STRUC;

*/
//-------------------------------------------------------------------------
// Command Block (CB) Generic Header Structure
//-------------------------------------------------------------------------
typedef struct _CB_HEADER_STRUC {
    UINT16      CbStatus;               // Command Block Status
    UINT16      CbCommand;              // Command Block Command
    UINT32       CbLinkPointer;          // Link To Next CB
} CB_HEADER_STRUC, *PCB_HEADER_STRUC;


//-------------------------------------------------------------------------
// NOP Command Block (NOP_CB)
//-------------------------------------------------------------------------
typedef struct _NOP_CB_STRUC {
    CB_HEADER_STRUC     NopCBHeader;
} NOP_CB_STRUC, *PNOP_CB_STRUC;


//-------------------------------------------------------------------------
// Individual Address Command Block (IA_CB)
//-------------------------------------------------------------------------
typedef struct _IA_CB_STRUC {
    CB_HEADER_STRUC     IaCBHeader;
    UINT8               IaAddress[ETHERNET_ADDRESS_LENGTH];
} IA_CB_STRUC, *PIA_CB_STRUC;




//-------------------------------------------------------------------------
// Size Of Dump Buffer
//-------------------------------------------------------------------------
#define DUMP_BUFFER_SIZE            600         // size of the dump buffer

//-------------------------------------------------------------------------
// Dump Command Block (DUMP_CB)
//-------------------------------------------------------------------------
typedef struct _DUMP_CB_STRUC {
    CB_HEADER_STRUC     DumpCBHeader;
    UINT32               DumpAreaAddress;        // Dump Buffer Area Address
} DUMP_CB_STRUC, *PDUMP_CB_STRUC;

//-------------------------------------------------------------------------
// Dump Area structure definition
//-------------------------------------------------------------------------
typedef struct _DUMP_AREA_STRUC {
    UINT8       DumpBuffer[DUMP_BUFFER_SIZE];
} DUMP_AREA_STRUC, *PDUMP_AREA_STRUC;

//-------------------------------------------------------------------------
// Diagnose Command Block (DIAGNOSE_CB)
//-------------------------------------------------------------------------
typedef struct _DIAGNOSE_CB_STRUC {
    CB_HEADER_STRUC     DiagCBHeader;
} DIAGNOSE_CB_STRUC, *PDIAGNOSE_CB_STRUC;

//-------------------------------------------------------------------------
// Transmit Command Block (TxCB)
//-------------------------------------------------------------------------
typedef struct _GENERIC_TxCB {
    CB_HEADER_STRUC     TxCbHeader;
    UINT32               TxCbTbdPointer;         // TBD address
    UINT16              TxCbCount;              // Data Bytes In TCB past header
    UINT8               TxCbThreshold;          // TX Threshold for FIFO Extender
    UINT8               TxCbTbdNumber;
    ETH_TX_BUFFER_STRUC TxCbData;
    UINT32               pad0;
    UINT32               pad1;
    UINT32               pad2;
    UINT32               pad3;
} TXCB_STRUC, *PTXCB_STRUC;

typedef enum {
	MMAC_CORE_NOT_RDY_RESERVED = 0, /* not used */
	MMAC_CORE_RDY = 1, /* when sp-20 is ready to proceed, it sets the state to this value */
	MMAC_CORE_START_REQ_HD = 2, /* host requests to dsp to update mmac interface */
	MMAC_CORE_STAGE2_ASSOC_INIT = 3, /*host init the state to this value when testing association using Stage 2 LB*/
	MMAC_CORE_RXFRAG_TEST_INIT = 4,  /*host init the state to this value when testing rx frag data path using Stage 2 LB*/
	MMAC_CORE_SOFTBOOT_REQ_HD, /* request to sp-20 to soft-reboot */
	MMAC_CORE_RDY_FOR_INTERFACE_UPDATE_DH, /* sp-20 reports to host that host may proceed with 
		mmac interface. It is in reponse to MMAC_CORE_SOFTBOOT_REQ_HD*/
} mmacCoreState_t;

//-------------------------------------------------------------------------
// Transmit Buffer Descriptor (TBD)
//-------------------------------------------------------------------------
typedef struct _TBD_STRUC {
    UINT32       TbdBufferAddress;       // Physical Transmit Buffer Address
    UINT32    TbdCount :14;
    UINT32             :1 ;           // always 0
    UINT32    EndOfList:1 ;           // EL bit in Tbd
    UINT32             :16;           // field that is always 0's in a TBD
} TBD_STRUC, *PTBD_STRUC;


//-------------------------------------------------------------------------
// Receive Frame Descriptor (RFD)
//-------------------------------------------------------------------------
typedef struct _RFD_STRUC {
    CB_HEADER_STRUC     RfdCbHeader;
    UINT32               RfdRbdPointer;  // Receive Buffer Descriptor Addr
    UINT16              RfdActualCount; // Number Of Bytes Received
    UINT16              RfdSize;        // Number Of Bytes In RFD
    ETH_RX_BUFFER_STRUC RfdBuffer;      // Data buffer in TCB
} RFD_STRUC, *PRFD_STRUC;


//-------------------------------------------------------------------------
// Receive Buffer Descriptor (RBD)
//-------------------------------------------------------------------------
typedef struct _RBD_STRUC {
    UINT16      RbdActualCount;         // Number Of Bytes Received
    UINT16      RbdFiller;
    UINT32       RbdLinkAddress;         // Link To Next RBD
    UINT32       RbdRcbAddress;          // Receive Buffer Address
    UINT16      RbdSize;                // Receive Buffer Size
    UINT16      RbdFiller1;
} RBD_STRUC, *PRBD_STRUC;


typedef struct _RSSI_UPDATE_
{
	UINT8	prev_count;	//holds the position saved last rssi value
//	UINT8	curr_count; //holds the position saved last rssi value
	
	UINT8	total_count;	//holds the number of saved rssi value(valid)
	INT32	rssi_acc;	//holds the sum of  saved rssi value(valid)

	INT32	rssi_array[RSSI_ARRAY_SIZE];	//holds times of rssi value
}RSSI_UPDATE_T,* PRSSI_UPDATE_T;

#ifdef NDIS50_MINIPORT
#ifdef PMK_CACHING_SUPPORT // add by jason 2007.8.23 begin
	typedef struct PMKID_CANDIDATE {
			 NDIS_802_11_MAC_ADDRESS BSSID;
			 UINT32 Flags;
	 } PMKID_CANDIDATE;
 
	#define NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED 0x01
 
	typedef struct NDIS_802_11_PMKID_CANDIDATE_LIST {
			UINT32 Version;
			UINT32 NumCandidates;
			PMKID_CANDIDATE CandidateList[1];
	} NDIS_802_11_PMKID_CANDIDATE_LIST;
#endif // add by jason 2007.8.23 end
#endif
#define MAX_NUM_OF_PMK_IDS		3
#ifdef PMK_CACHING_SUPPORT
typedef struct _NDIS_802_11_USED_PMKID
{
  UINT32  Length;
  UINT32  BSSIDInfoCount;
  BSSIDInfo BSSIDInfo[MAX_NUM_OF_PMK_IDS];
} NDIS_802_11_USED_PMKID, *PNDIS_802_11_USED_PMKID;
#endif //PMK_CACHING_SUPPORT


#define BSS_NOT_FOUND                    0xFFFFFFFF

#ifdef ROAMING_SUPPORT

#define WPA1AKMBIT	1
#define WPA2AKMBIT	2
#define WPA1PSKAKMBIT   4
#define WPA2PSKAKMBIT   8

#define ROAM_THRESHOLD_RSSI_DBM         -60
#define ROAM_THRESHOLD_DELTA_RSSI_DBM   6


typedef struct  _RSN_IE_HEADER_STRUCT	
{
	UINT8		Eid;
	UINT8		Length;
	UINT16		Version;	// Little endian format
}	RSN_IE_HEADER_STRUCT, *PRSN_IE_HEADER_STRUCT;

// Cipher suite selector types
typedef struct  _CIPHER_SUITE_STRUCT	
{
	UINT8		Oui[3];
	UINT8		Type;
}	CIPHER_SUITE_STRUCT, *PCIPHER_SUITE_STRUCT;

// Authentication and Key Management suite selector
typedef struct  _AKM_SUITE_STRUCT	
{
	UINT8		Oui[3];
	UINT8		Type;
}	AKM_SUITE_STRUCT, *PAKM_SUITE_STRUCT;

#endif//ROAMING_SUPPORT

#pragma pack()

//-------------------------------------------------------------------------
// WLAN_STA chip PCI Register Definitions
// Refer To The PCI Specification For Detailed Explanations
//-------------------------------------------------------------------------
//- Register Offsets
#define PCI_VENDOR_ID_REGISTER      0x00    // PCI Vendor ID Register
#define PCI_DEVICE_ID_REGISTER      0x02    // PCI Device ID Register
#define PCI_CONFIG_ID_REGISTER      0x00    // PCI Configuration ID Register
#define PCI_COMMAND_REGISTER        0x04    // PCI Command Register
#define PCI_STATUS_REGISTER         0x06    // PCI Status Register
#define PCI_REV_ID_REGISTER         0x08    // PCI Revision ID Register
#define PCI_CLASS_CODE_REGISTER     0x09    // PCI Class Code Register
#define PCI_CACHE_LINE_REGISTER     0x0C    // PCI Cache Line Register
#define PCI_LATENCY_TIMER_REGISTER  0x0D    // PCI Latency Timer Register
#define PCI_HEADER_TYPE_REGISTER    0x0E    // PCI Header Type Register
#define PCI_BIST_REGISTER           0x0F    // PCI Built-In SelfTest Register
#define PCI_BAR_0_REGISTER          0x10    // PCI Base Address Register 0
#define PCI_BAR_1_REGISTER          0x14    // PCI Base Address Register 1
#define PCI_BAR_2_REGISTER          0x18    // PCI Base Address Register 2
#define PCI_BAR_3_REGISTER          0x1C    // PCI Base Address Register 3
#define PCI_BAR_4_REGISTER          0x20    // PCI Base Address Register 4
#define PCI_BAR_5_REGISTER          0x24    // PCI Base Address Register 5
#define PCI_SUBVENDOR_ID_REGISTER   0x2C    // PCI SubVendor ID Register
#define PCI_SUBDEVICE_ID_REGISTER   0x2E    // PCI SubDevice ID Register
#define PCI_EXPANSION_ROM           0x30    // PCI Expansion ROM Base Register
#define PCI_INTERRUPT_LINE_REGISTER 0x3C    // PCI Interrupt Line Register
#define PCI_INTERRUPT_PIN_REGISTER  0x3D    // PCI Interrupt Pin Register
#define PCI_MIN_GNT_REGISTER        0x3E    // PCI Min-Gnt Register
#define PCI_MAX_LAT_REGISTER        0x3F    // PCI Max_Lat Register
#define PCI_NODE_ADDR_REGISTER      0x40    // PCI Node Address Register

//#define HARDWARE_NOT_RESPONDING(adapter)




/***************************************************************
* Shuttle Bus Macros
***************************************************************/
// Builds the long int that is passed as the Control parameter to ShuttleTxStart
//###############################################################################
// SB Macros
//###############################################################################

/***************************************************************
* ShuttleBus macros and constants
***************************************************************/


/* Null, for use when read control, read stop, write control, write stop, 
 * write link are all set to zero (most often used case) */

#define SB_TX_NULL (0, 0, 0, 0, 0)
#define SB_READ_STOP (0, 0, 1, 0, 0)
#define SB_WRITE_STOP (0, 0, 0, 0, 1)


/***************************************************************
* bit field for SB control
***************************************************************/
#define BF_SB_WRCTL_L	0x0001
#define BF_SB_WRLNK_L	0x0002
#define BF_SB_RDCTL_L	0x0010
//--other control definition------------------------------------
#define SB_WR_LNK_ON	 1
#define SB_WR_LNK_OFF	 0
#define SB_RD_CTL_ON	 1
#define SB_RD_CTL_OFF	 0
#define SB_RD_STP_ON	 1
#define SB_RD_STP_OFF	 0
#define SB_WR_CTL_ON	 1
#define SB_WR_CTL_OFF	 0
#define SB_WR_STP_ON     1
#define SB_WR_STP_OFF	 0

#define SHUTTLE2HOST_ON  1
#define SHUTTLE2HOST_OFF 0 
#define HOST2SHUTTLE_ON  1
#define HOST2SHUTTLE_OFF 0 

//---MAC HOST DMA control definition------------------------------
#define XFER_ACTIVATE_ON   1
#define XFER_ACTIVATE_OFF  0
#define HOST2TXFIFO_ON	   1
#define RXFIFO2HOST_ON	   0
#define BURSTSIZE_512WORD  0
#define BURSTSIZE_32WORD   1
#define BURSTSIZE_4WORD    2
#define BURSTSIZE_1WORD    3

#define MAC_HOST_DMA_CONTROL(Activation, XferDirect, BurstSize, NoOfRequest) \
			((Activation << 7) | (XferDirect << 6) | (BurstSize << 4) | (NoOfRequest-1))


#define SB_CONTROL(NoOfBytes, Priority, BurstSize, TxType, WriteLink, ReadCtl, ReadStop, WriteCtl, WriteStop, HiPerf,shuttle2host,host2shuttle) \
            ((Priority << 28) | (HiPerf << 26) | (host2shuttle << 25) | (shuttle2host << 24) | (BurstSize << 21) | \
             (((NoOfBytes-1) & (0x3fff)) << 7) | (TxType << 2) | (WriteLink << 1) | (ReadCtl << 4) | \
             (ReadStop << 6) | (WriteCtl << 0) | (WriteStop << 5))

#define SB_ADDRESS(PortNo, Address) ((PortNo << 28) | (Address << 0))

#define SB_CONTROL_REG_VAL(RegisterDest, RegisterSrc) RegisterDest[20:9] = RegisterSrc

#define SB_ADDRESS_REG_VAL(RegisterDest, RegisterSrc) RegisterDest[27:0] = RegisterSrc

#define LO_HI(RegisterDest, Value)  RegisterDest.l = .lo16(Value); RegisterDest.h = .hi16(Value);

/* SB ports */
#define SB_PORT_MAC 0
#define SB_PORT_UNIPHY 1
#define SB_PORT_CARDBUS 3

/* Transfer types */
#define SB_TX_RESET 0
#define SB_TX_NORM 1
#define SB_TX_CHAIN 2
#define SB_TX_INF 3

/* TS sizes */
#define SB_TS1 0
#define SB_TS2 1
#define SB_TS4 2
#define SB_TS8 3

/* SB Port defines */
#define SB_PORT_0 0
#define SB_PORT_1 1
#define SB_PORT_2 2
#define SB_PORT_3 3
#define SB_PORT_4 4
#define SB_PORT_5 5
#define SB_PORT_6 6
#define SB_PORT_7 7
#define SB_PORT_8 8
#define SB_PORT_9 9
#define SB_PORT_10 10
#define SB_PORT_11 11
#define SB_PORT_12 12
#define SB_PORT_13 13
#define SB_PORT_14 14
#define SB_PORT_15 15

/* Ports on the SB for SP5, SP20 */
#if defined(__R5__) || defined(__R4__) || defined(__SP20__)
#define SB_PORT_PMEM SB_PORT_0
#define SB_PORT_DMEM SB_PORT_1
#define SB_PORT_XPORT2 SB_PORT_2
#define SB_PORT_XPORT1 SB_PORT_3
#define SB_PORT_EXTMEM SB_PORT_4
#endif // _R5__ | _R4__

#if defined(__R6__)
#define SB_PORT_YMEM SB_PORT_0
#define SB_PORT_PMEM SB_PORT_1
#define SB_PORT_DMEM SB_PORT_1
#define SB_PORT_XPORT2 SB_PORT_2
#define SB_PORT_XPORT1 SB_PORT_3
#define SB_PORT_EXTMEM SB_PORT_2
#endif // __R6__ 

/* SB Priorities */ 
#define SB_PRI0 0
#define SB_PRI1 1
#define SB_PRI2 2
#define SB_PRI3 3
#define SB_PRI4 4
#define SB_PRI5 5
#define SB_PRI6 6
#define SB_PRI7 7
#define SB_PRI8 8
#define SB_PRI9 9
#define SB_PRI10 SB_PRI10_UNDEFINED
#define SB_PRI11 SB_PRI11_UNDEFINED
#define SB_PRI12 SB_PRI12_UNDEFINED
#define SB_PRI13 SB_PRI13_UNDEFINED
#define SB_PRI14 SB_PRI14_UNDEFINED
#define SB_PRI_MAT 15


#define SPX_P_MEM            0
#define SPX_A_MEM            1
#define SPX_B_MEM            2
#define SPX_T_MEM            3
#define SPX_CS0_MEM          4
#define SPX_CS3_MEM          5
#define SPX_CS1_MEM         12
#define SPX_CS2_MEM         13
#define SPX_INST_MEM        14



#define WLS_CSR_PCI_CONTROL_WD           0x00
//bits
#define PCI_RESET_WLAN_CORE_BIT       BIT_0
#define PCI_RESET_WLAN_SUBSYSTEM_BIT  BIT_1
#define PCI_RESET_WLAN_SYSTEM_BIT     BIT_2
#define PCI_RESET_PCI_BIT             BIT_3
#define PCI_SOFT_RESET_PCI_BIT        BIT_4
#define PCI_SLEEP_WLAN_CORE_BIT       BIT_8
#define PCI_SLEEP_WLAN_SUBSYSTEM_BIT  BIT_9
#define PCI_SLEEP_WLAN_SYSTEM_BIT     BIT_10
#define PCI_SLEEP_MAC_GATED_BIT    BIT_11
#define PCI_SLEEP_MAC_BIT             BIT_12
#define PCI_SLEEP_DEBUG_BIT             BIT_13
#define DMA0_ENABLE_BIT               BIT_16
#define DMA1_ENABLE_BIT               BIT_17
#define MAC_HOST_DMA0_ENABLE_BIT      BIT_18
#define MAC_HOST_DMA1_ENABLE_BIT      BIT_19

#define WLS_CSR_RETRIES_WD               0x04

#define WLS_CSR_IE_WD                    0x08
//bits
#define DMA0_BUSY_BIT                 BIT_0
#define DMA0_DONE_BIT                 BIT_1
#define DMA0_ABORT_BIT                BIT_2
#define DMA1_ABORT_BIT                BIT_5
#define MAC_HOST_DMA0_BUSY_BIT        BIT_6
#define MAC_HOST_DMA0_DONE_BIT        BIT_7
#define MAC_HOST_DMA0_ABORT_BIT       BIT_8
#define MAC_HOST_DMA1_BUSY_BIT        BIT_9
#define MAC_HOST_DMA1_DONE_BIT        BIT_10
#define MAC_HOST_DMA1_ABORT_BIT       BIT_11
#define DSP_SHUTTLE_0_DONE_BIT        BIT_12
#define DSP_SHUTTLE_1_DONE_BIT        BIT_13
#define DSP_SHUTTLE_2_DONE_BIT        BIT_14
#define DSP_SHUTTLE_3_DONE_BIT        BIT_15
#define DSP_SHUTTLE_4_DONE_BIT        BIT_16

#define MASTER_ABORT_BIT              BIT_22
#define TARGET_ABORT_BIT              BIT_23
#define LATENCY_TIMEOUT_BIT           BIT_24
#define REPORTED_PARITY_BIT           BIT_25
#define DETECTED_PARITY_BIT           BIT_26
#define SYSTEM_ERROR_BIT              BIT_27
#define MAC_RX_IEN		              BIT_28
#define MAC_TX_IEN		              BIT_29
#define MAILBOX_IEN		              BIT_30
#define WLS_MAC_EVENT_EN			  BIT_31

#define WLS_DMA_ERRORS	((DMA0_ABORT_BIT) | (DMA1_ABORT_BIT) | \
		(MAC_HOST_DMA0_ABORT_BIT) | (MAC_HOST_DMA1_ABORT_BIT))

#define WLS_DMA_0_MASK	(DMA0_ABORT_BIT)

#define WLS_PCI_INT_MASK ((MASTER_ABORT_BIT) | (TARGET_ABORT_BIT) | \
		(LATENCY_TIMEOUT_BIT) | (REPORTED_PARITY_BIT) | (DETECTED_PARITY_BIT) | \
		(SYSTEM_ERROR_BIT) )


#define WLS_MAC_RELAY_MASK \
		( \
		(MAC_RX_IEN) | (MAC_TX_IEN) | (WLS_MAC_EVENT_EN) \
		)   

#define WLS_INT_MASK \
		(\
		(WLS_DMA_0_MASK) | (WLS_PCI_INT_MASK) | \
		(WLS_MAC_RELAY_MASK) | (WLS_MAC_EVENT_EN)\
		)   

#define WLS_ACK_MASK	WLS_INT_MASK

#define PCI_STEERING_BIT              0x08000000

#define WLS_ENABLE_CHIP_NO_CORE (PCI_RESET_WLAN_CORE_BIT | DMA0_ENABLE_BIT)





/*     3DSP register definination     */


/************************************************************************
*																		*
*					DSP CODE macros and constants						*
*																		*
************************************************************************/

#define PMEM_FILE_NAME "dsp_Pmem.bin"
#define AMEM_FILE_NAME "dsp_Amem.bin"
#define BMEM_FILE_NAME "dsp_Bmem.bin"
#define TMEM_FILE_NAME "dsp_Emem.bin"

#define PMEM_FILE_ID	0
#define AMEM_FILE_ID	1
#define BMEM_FILE_ID	2
#define TMEM_FILE_ID	3







/************************************************************************
*																		*
*					MAC HOST DMA control definition						*
*																		*
************************************************************************/

/*
 *	Activation BITA). 
 *	set by writes from the DSP-Shuttle and cleared by hardware once 
 *	the transfer has completed or aborted.
 *	1b:	transfer is activated.
 *	0b:	transfer is not active.
 */
#define M_H_DMA_CTRL_ACTIVATE_ON		   	1
#define M_H_DMA_CTRL_ACTIVATE_OFF  			0



/*
 *	Direction of the transfer(D). 
 *	1b:	Host to MAC TX FIFO transfer.
 *	0b:	MAC RX FIFO to Host transfer.
 */
#define M_H_DMA_CTRL_HOST2TXFIFO		   	1
#define M_H_DMA_CTRL_RXFIFO2HOST  			0



/*
 *	Burst Size: Size of each PCI burst for the current DMA transfer.
 *	00b:	512 words
 *	01b:	32 words
 *	10b:	4 words
 *	11b:	1 word
 */
#define M_H_DMA_CTRL_BURSTSIZE_512  		0	// 00
#define M_H_DMA_CTRL_BURSTSIZE_32   		1	// 01
#define M_H_DMA_CTRL_BURSTSIZE_4    		2	// 10
#define M_H_DMA_CTRL_BURSTSIZE_1    		3	// 11


/*
 *	Number of Requests: 
 *	Number of requests in the requestor for the current DMA transfer.
 *	0000b:		1 requests
 *	0001b:		2 requests
 *	0010b:		3 requests
 *	0011b:		4 requests (only for MAC_Host_DMA0)
 *	0100b:		5 requests (only for MAC_Host_DMA0)
 *	0101b:		6 requests (only for MAC_Host_DMA0)
 *	0110b:		7 requests (only for MAC_Host_DMA0)
 *	0111b:		8 requests (only for MAC_Host_DMA0)
 *	1000b:		9 requests (only for MAC_Host_DMA0)
 *
 */
#define M_H_DMA_CTRL_REQ_NUM_1  		0	// 0000
#define M_H_DMA_CTRL_REQ_NUM_2   		1	// 0001
#define M_H_DMA_CTRL_REQ_NUM_3    		2	// 0010
#define M_H_DMA_CTRL_REQ_NUM_4    		3	// 0011
#define M_H_DMA_CTRL_REQ_NUM_5    		4	// 0100
#define M_H_DMA_CTRL_REQ_NUM_6    		5	// 0101
#define M_H_DMA_CTRL_REQ_NUM_7    		6	// 0110
#define M_H_DMA_CTRL_REQ_NUM_8    		7	// 0111
#define M_H_DMA_CTRL_REQ_NUM_9    		8	// 1000







/*
 *
 * <<DSP-Shuttle User's Guide>>, Chapter 2, DSP-Shuttle Command Formats
 *
 * --------------------------------------------------------------------------
 * |	 				Source/Destination	Word							|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |Source Address			| 0-27	| 00001111 11111111 11111111 11111111	|
 * |Source Port				| 28-31	| 11110000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 * |Dest Address			| 0-27	| 00001111 11111111 11111111 11111111	|
 * |Dest Port				| 28-31	| 11110000 00000000 00000000 00000000	|
 * -------------------------------------------------------------------------- 
 */
#define BITS_SHUTTLE__SD_WORD__ADDR				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27)
#define BITS_SHUTTLE__SD_WORD__PORT				(BIT28|BIT29|BIT30|BIT31)

#define OFFSET_SHUTTLE__SD_WORD__ADDR			0
#define OFFSET_SHUTTLE__SD_WORD__PORT			28




/*
 *
 * <<DSP-Shuttle User's Guide>>, Chapter 2, DSP-Shuttle Command Formats
 *
 * --------------------------------------------------------------------------
 * |	 						Control Word								|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |WrCtl					| 0		| 00000000 00000000 00000000 00000001	|
 * |WrLnk					| 1		| 00000000 00000000 00000000 00000010	|
 * |Mode					| 2-3	| 00000000 00000000 00000000 00001100	|
 * |RdCtl					| 4		| 00000000 00000000 00000000 00010000	|
 * |WrStp					| 5		| 00000000 00000000 00000000 00100000	|
 * |RdStp					| 6		| 00000000 00000000 00000000 01000000	|
 * |Number of Bytes			| 7-20	| 00000000 00011111 11111111 10000000	|
 * |Burst Size				| 21-23	| 00000000 11100000 00000000 00000000	|
 * | 						| 24-25	| Reserved								|
 * |HiP						| 26	| 00000100 00000000 00000000 00000000	|
 * |FIFO					| 27	| 00001000 00000000 00000000 00000000	|
 * |Priority				| 28-31	| 11110000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_SHUTTLE__CTRL_WORD__WR_CTL					BIT0
#define BITS_SHUTTLE__CTRL_WORD__WR_LNK					BIT1
#define BITS_SHUTTLE__CTRL_WORD__MODE					(BIT2|BIT3)
#define BITS_SHUTTLE__CTRL_WORD__RD_CTL					BIT4
#define BITS_SHUTTLE__CTRL_WORD__WR_STP					BIT5
#define BITS_SHUTTLE__CTRL_WORD__RD_STP					BIT6
#define BITS_SHUTTLE__CTRL_WORD__NUM_OF_BYTES			(BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20)
#define BITS_SHUTTLE__CTRL_WORD__BURST_SIZE				(BIT21|BIT22|BIT23)
														//Reserved
#define BITS_SHUTTLE__CTRL_WORD__HIP					BIT26
#define BITS_SHUTTLE__CTRL_WORD__FIFO					BIT27
#define BITS_SHUTTLE__CTRL_WORD__PRIORITY				(BIT28|BIT29|BIT30|BIT31)


#define OFFSET_SHUTTLE__CTRL_WORD__WR_CTL				0
#define OFFSET_SHUTTLE__CTRL_WORD__WR_LNK				1
#define OFFSET_SHUTTLE__CTRL_WORD__MODE					2
#define OFFSET_SHUTTLE__CTRL_WORD__RD_CTL				4
#define OFFSET_SHUTTLE__CTRL_WORD__WR_STP				5
#define OFFSET_SHUTTLE__CTRL_WORD__RD_STP				6
#define OFFSET_SHUTTLE__CTRL_WORD__NUM_OF_BYTES			7
#define OFFSET_SHUTTLE__CTRL_WORD__BURST_SIZE			21
#define OFFSET_SHUTTLE__CTRL_WORD__HIP					26
#define OFFSET_SHUTTLE__CTRL_WORD__FIFO					27
#define OFFSET_SHUTTLE__CTRL_WORD__PRIORITY				28





#define MAC_HOST_DMA_CONTROL(Activation, XferDirect, BurstSize, NoOfRequest) \
			((Activation << 7) | (XferDirect << 6) | (BurstSize << 4) | (NoOfRequest-1))


#define SB_CONTROL(NoOfBytes, Priority, BurstSize, TxType, WriteLink, ReadCtl, ReadStop, WriteCtl, WriteStop, HiPerf,shuttle2host,host2shuttle) \
            ((Priority << 28) | (HiPerf << 26) | (host2shuttle << 25) | (shuttle2host << 24) | (BurstSize << 21) | \
             (((NoOfBytes-1) & (0x3fff)) << 7) | (TxType << 2) | (WriteLink << 1) | (ReadCtl << 4) | \
             (ReadStop << 6) | (WriteCtl << 0) | (WriteStop << 5))

#define SB_ADDRESS(PortNo, Address) ((PortNo << 28) | (Address << 0))


/* SB ports */

#define SB_PORT_MAC 0
#define SB_PORT_UNIPHY 1
#define SB_PORT_CARDBUS 3

#define PCI_STEERING_BIT              0x08000000











/************************************************************************
*																		*
*					ShuttleBus macros and constants						*
*																		*
************************************************************************/



/* ShuttleBus ports */
#define SHUTTLEBUS__PORT_UNIPHY				1
#define SHUTTLEBUS__PORT_CARDBUS			3



/* Transfer types */
#define SB_TX_RESET 0
#define SB_TX_NORM 1
#define SB_TX_CHAIN 2
#define SB_TX_INF 3

/* TS sizes */
#define SB_TS1 0
#define SB_TS2 1
#define SB_TS4 2
#define SB_TS8 3


/* SB Port defines */
#define SB_PORT_0 0
#define SB_PORT_1 1
#define SB_PORT_2 2
#define SB_PORT_3 3
#define SB_PORT_4 4
#define SB_PORT_5 5
#define SB_PORT_6 6
#define SB_PORT_7 7
#define SB_PORT_8 8
#define SB_PORT_9 9
#define SB_PORT_10 10
#define SB_PORT_11 11
#define SB_PORT_12 12
#define SB_PORT_13 13
#define SB_PORT_14 14
#define SB_PORT_15 15





/************************************************************************
*																		*
*						DSP-Shuttle Control Word						*
*																		*
************************************************************************/

/*
 * Priority:  
 * The Channel number that the request will be put into.  This must be a 
 * valid channel number.  Range 0-15 .
 * (0 = highest Priority,14 = lowest Priority,15 = Matrix Definition Channel)
 */
#define SHUTTLE_PRIORITY_0		0
#define SHUTTLE_PRIORITY_1		1
#define SHUTTLE_PRIORITY_2		2
#define SHUTTLE_PRIORITY_3		3
#define SHUTTLE_PRIORITY_4		4
#define SHUTTLE_PRIORITY_5		5
#define SHUTTLE_PRIORITY_6		6
#define SHUTTLE_PRIORITY_7		7
#define SHUTTLE_PRIORITY_8		8
#define SHUTTLE_PRIORITY_9		9
#define SHUTTLE_PRIORITY_10		10
#define SHUTTLE_PRIORITY_11		11
#define SHUTTLE_PRIORITY_12		12
#define SHUTTLE_PRIORITY_13		12
#define SHUTTLE_PRIORITY_14		14
#define SHUTTLE_PRIORITY_15		15


/*
 * FIFO:  
 * When this bit is set to 1, the transfer will be part of a FIFO transfer.  
 * It is only valid when the corresponding channel is configured as FIFO 
 * channel in the hardware.
 */
#define SHUTTLE_FIFO_ON	 		1
#define SHUTTLE_FIFO_OFF	 	0


/*
 * HiP:  
 * When this bit is set to 1, the Read Request for a waiting chained command 
 * can be sent out as soon as the previous command in the chain has finished 
 * using its read buffers.  This increases the performance and ensures that 
 * the peripheral that is being read from sees no break in the read requests.
 * If this bit is not set then the waiting chained command must wait for the 
 * previous command to finish flushing it write buffers before it can have 
 * access to the read buffers.  
 * This bit may only be set to '1' when Mode = '10' (chained transfer).  
 * In other modes, it has no effect.
 */
#define SHUTTLE_HIP_ON	 		1
#define SHUTTLE_HIP_OFF	 		0



/*
 * Burst Size:  
 * 	Encoded version of the burst size to be used; 
 *	'000' = Burst size of 1, 
 *	'001' = Size of 2, 
 *	'010' = Size of 4, 
 *	'011' = Size of 8, 
 *	'1xx' = reserved for future expansion. 
 */
#define SHUTTLE_BURST_SIZE_1		0	// 0000
#define SHUTTLE_BURST_SIZE_2		1	// 0001
#define SHUTTLE_BURST_SIZE_3		2	// 0010
#define SHUTTLE_BURST_SIZE_8		3	// 0011


/*
 * Number of Bytes:  
 * Gives the total number of bytes to be moved over the Data Bus by this 
 * transfer.  This number is encoded such that 0x0000 means move one byte, 
 * 0x0001 means move 2 bytes etc, 0x3fff means move 16384 bytes.  
 *
 * Note that if UnPacked data is transferred then only one useful byte is 
 * moved for each 32 bits that is transferred over the Data Bus, but because 
 * one word (32 bit) is moved over the Data Bus then this is counted as 4 
 * bytes when calculating Number of Bytes.
 */



/*
 * RdStp:
 * When this bit is set to '1' the Read Port will not increment its 
 * address between each data read from the peripheral.  
 */
#define SHUTTLE_R_STOP_ON	 	1
#define SHUTTLE_R_STOP_OFF	 	0


/*
 * WrStp:  
 * When this bit is set to '1' the Write Port will not increment its 
 * address between each data written to the peripheral.  
 */
#define SHUTTLE_W_STOP_ON	 	1
#define SHUTTLE_W_STOP_OFF	 	0


/*
 * RdCtl:  
 * This is a multi-purpose bit.  It is passed to the source peripherals as is.
 */
#define SHUTTLE_R_CTRL_ON	 	1
#define SHUTTLE_R_CTRL_OFF	 	0


/*
 * Mode:  
 *	'00' = Abort transfer, 
 *	'01' = Normal transfer, 
 *	'10' = Chained transfer, 
 *	'11' = Infinite transfer.  
 *
 * Note that if a Abort transfer is written to a channel only the Mode bits 
 * are written into the Channel register.  All other bits are left untouched.
 * The Requestor uses these bits to directly control the peripheral.  For 
 * example, they can be used as a cache lock bit if the peripheral is a cache.
 */
#define SHUTTLE_MODE_TX_ABORT	 		0	// 00
#define SHUTTLE_MODE_TX_NORMAL	 		1	// 01
#define SHUTTLE_MODE_TX_CHAINED	 		2	// 10
#define SHUTTLE_MODE_TX_INFINITE	 	3	// 11



/*
 * WrLnk:  
 * When this bit is set to '1' then the Bus Arbitrator will check to see if
 * the Write Port is vacant before issuing the Read Request to the Read
 * Port for each burst, so that the slow port will not stall the fast ports.
 */
#define SHUTTLE_W_LINK_ON	 	1
#define SHUTTLE_W_LINK_OFF	 	0



/*
 * WrCtl:  
 * This is a multi-purpose bit.  It is passed to the destination peripherals as is.
 */
#define SHUTTLE_W_CTRL_ON	 	1
#define SHUTTLE_W_CTRL_OFF	 	0




/* SB Priorities */ 
#define SB_PRI0 0
#define SB_PRI1 1
#define SB_PRI2 2
#define SB_PRI3 3
#define SB_PRI4 4
#define SB_PRI5 5
#define SB_PRI6 6
#define SB_PRI7 7
#define SB_PRI8 8
#define SB_PRI9 9
#define SB_PRI10 SB_PRI10_UNDEFINED
#define SB_PRI11 SB_PRI11_UNDEFINED
#define SB_PRI12 SB_PRI12_UNDEFINED
#define SB_PRI13 SB_PRI13_UNDEFINED
#define SB_PRI14 SB_PRI14_UNDEFINED
#define SB_PRI_MAT 15


#define SPX_P_MEM            0
#define SPX_A_MEM            1
#define SPX_B_MEM            2
#define SPX_T_MEM            3
#define SPX_CS0_MEM          4
#define SPX_CS3_MEM          5
#define SPX_CS1_MEM         12
#define SPX_CS2_MEM         13
#define SPX_INST_MEM        14




/*
 * Memory-mapped Host Address Space:
 *
 * ----------------------------------------------------------------------
 * |  Region Name			|  Addr[14:0] (Hex)		|  Reserved Size	|			
 * ----------------------------------------------------------------------
 * |  CSR region			|  0x0000 - 0x1FFF		|  8 KB				|
 * ----------------------------------------------------------------------
 * |  Scratchpad region		|  0x2000 - 0x3FFF		|  8 KB				|
 * ----------------------------------------------------------------------
 * |  MAC region			|  0x4000 - 0x7FFF		|  16 KB			|
 * ----------------------------------------------------------------------
 *
 * CSR:  CardBus control and status registers
 *
 */


#define _OFFSET_CSR		0x0000
#define _OFFSET_SPD		0x2000
#define _OFFSET_MAC	0x4000






/**************************************************************************
 **************************************************************************
 **																		 **
 **																		 **
 **			3DSP Wireless LAN Station (CSR)  	Registers Macros		 **
 **																		 **
 **																		 **
 **************************************************************************
 *************************************************************************/
#define WLS_CSR__PCI_CONTROL_REGISTER		0x0000	//PCI Control Register
#define WLS_CSR__NUMBER_OF_RETRY			0x0004	//Number of Retry
#define WLS_CSR__INTERRUPT_ENABLE			0x0008	//Interrupt Enable
#define WLS_CSR__STATUS						0x000C	//Status
#define WLS_CSR__CLEAR_STATUS				0x0010	//Status Clear
#define WLS_CSR__CHIP_ID					0x0014	//chip id

													//Reserved
													
#define WLS_CSR__DMA0_SRC_ADDR				0x0020	//DMA0_src_addr
#define WLS_CSR__DMA0_DST_ADDR				0x0024	//DMA0_dst_addr
#define WLS_CSR__DMA0_CONTROL				0x0028	//DMA0_control


	



// CardBus PC Card Status Registers			(0x0040 - 0x004C) 
#define WLS_CSR__FUNCTION_EVENT				0x0040	//Function Event
#define WLS_CSR__FUNCTION_EVENT_MASK		0x0044	//Function Event Mask
#define WLS_CSR__FUNCTION_PRESENT_STATE		0x0048	//Function Present State
#define WLS_CSR__FUNCTION_FORCE_EVENT		0x004C	//Function Force Event




// Registers Related to MAC_Host_DMA
#define WLS_CSR__MAC_HOST_DMA0_CTRL_REG				0x0098	//Control Register



#define WLS_CSR__MAC_HOST_DMA1_ADDR_REG0			0x00AC	//Addr Register 0
#define WLS_CSR__MAC_HOST_DMA1_SIZE_REG0			0x00B0	//Size Register 0

#define WLS_CSR__MAC_HOST_DMA1_CTRL_REG				0x00B4	//Control Register
#define WLS_CSR__MAC_HOST_DMA0_REENABLE_WD	0xB8
#define WLS_CSR__MAC_HOST_DMA1_REENABLE_WD	0xBC

#define WLS_MAC_HOST_DMA_STATE_MACHINE_WD              0xC0
#define WLS_MAC_HOST_DMA_BURST_SIZE_WD                 0xC4
#define WLS_MAC_HOST_DMA_TX_BD_BASE_ADDR_WD            0xC8    //not used when BD in chip scratch RAM
#define WLS_MAC_HOST_DMA_EDCA_TX_BD_START_ADDR_WD      0xCC    //offset from base of scratch
#define WLS_MAC_HOST_DMA_EDCA_PRODUCER_PTR_WD          0xD0    //ditto
#define WLS_MAC_HOST_DMA_EDCA_CONSUMER_PTR_WD          0xD4    //ditto
#define WLS_MAC_HOST_DMA_HCCA_TX_BD_START_ADDR_WD      0xD8    //ditto
#define WLS_MAC_HOST_DMA_HCCA_PRODUCER_PTR_WD          0xDC    //ditto
#define WLS_MAC_HOST_DMA_HCCA_CONSUMER_PTR_WD          0xE0    //ditto
#define WLS_MAC_HOST_DMA_TX_FIFO_CONSUMER_PTR_WD       0xE4
#define WLS_MAC_HOST_DMA_MPDU_INFO_WD                  0xE8

#define WLS_MAC_HOST_DMA_EDCA_BYTES_SENT_WD            0xEC
#define WLS_MAC_HOST_DMA_HCCA_BYTES_SENT_WD            0xF0

#define WLS_MAC_HOST_DMA_RX_PRODUCER_PTR_WD            0xF4
#define WLS_MAC_HOST_DMA_RX_BD_START_ADDR_WD           0xF8
#define WLS_MAC_HOST_DMA_RX_COMSUMER_INDEX_PTR         0xFC
#define WLS_MAC_HOST_DMA_RX_PRODUCER_NEXT_INDEX_PTR_WD 0x100
#define WLS_MAC_HOST_DMA_RX_NUMBER_SEGMENTS_WD         0x104
#define WLS_MAC_HOST_DMA_RX_SEGMENT_AVAILABLE_SIZE_WD  0x108
#define WLS_MAC_HOST_DMA_RX_FRAGMENT_LENGTH_WD         0x10C








/*
 * #define WLS_CSR__PCI_CONTROL_REGISTER		0x0000
 *
 * --------------------------------------------------------------------------
 * |	 	 					PCI Control Register 						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |pci_reset_wlan_core		| 0		| 00000000 00000000 00000000 00000001	|
 * |pci_reset_wlan_subsys	| 1		| 00000000 00000000 00000000 00000010	|
 * |pci_reset_wlan_system	| 2		| 00000000 00000000 00000000 00000100	|
 * |pci_reset_pci			| 3		| 00000000 00000000 00000000 00001000	|
 * |pci_soft_reset_pci		| 4		| 00000000 00000000 00000000 00010000	|
 * |						| 5-7	| Reserved								|
 * |pci_sleep_wlan_core		| 8		| 00000000 00000000 00000001 00000000	|
 * |pci_sleep_wlan_subsys	| 9		| 00000000 00000000 00000010 00000000	|
 * |pci_sleep_wlan_system	| 10	| 00000000 00000000 00000100 00000000	|
 * |pci_sleep_mac_gated		| 11	| 00000000 00000000 00001000 00000000	|
 * |pci_sleep_mac			| 12	| 00000000 00000000 00010000 00000000	|
 * |pci_sleep_debug			| 13	| 00000000 00000000 00100000 00000000	|
 * |						| 14-15	| Reserved								|
 * |DMA0_enable				| 16	| 00000000 00000001 00000000 00000000	|
 * |DMA1_enable				| 17	| 00000000 00000010 00000000 00000000	|
 * |MAC_Host_DMA0_enable	| 18	| 00000000 00000100 00000000 00000000	|
 * |MAC_Host_DMA1_enable	| 19	| 00000000 00001000 00000000 00000000	|
 * | 						| 20-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_PCI_CTRL_REG__RESET_WLAN_CORE				BIT0
#define BITS_PCI_CTRL_REG__RESET_WLAN_SUB_SYS			BIT1
#define BITS_PCI_CTRL_REG__RESET_WLAN_SYS				BIT2
#define BITS_PCI_CTRL_REG__RESET_PCI					BIT3
#define BITS_PCI_CTRL_REG__SOFT_RESET_PCI				BIT4
														//Reserved
#define BITS_PCI_CTRL_REG__SLEEP_WLAN_CORE				BIT8
#define BITS_PCI_CTRL_REG__SLEEP_WLAN_SUB_SYS			BIT9
#define BITS_PCI_CTRL_REG__SLEEP_WLAN_SYS				BIT10
#define BITS_PCI_CTRL_REG__SLEEP_MAC_GATED				BIT11
#define BITS_PCI_CTRL_REG__SLEEP_MAC					BIT12
#define BITS_PCI_CTRL_REG__SLEEP_DEBUG					BIT13
														//Reserved
#define BITS_PCI_CTRL_REG__DMA0_EN						BIT16
#define BITS_PCI_CTRL_REG__DMA1_EN						BIT17
#define BITS_PCI_CTRL_REG__MAC_HOST_DMA0_EN				BIT18
#define BITS_PCI_CTRL_REG__MAC_HOST_DMA1_EN				BIT19
														//Reserved







/*
 * #define WLS_CSR__INTERRUPT_ENABLE			0x0008
 *
 * --------------------------------------------------------------------------
 * |	 	 				Interrupt Enable Register						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |DMA0_busy_ien			| 0		| 00000000 00000000 00000000 00000001	|
 * |DMA0_done_ien			| 1		| 00000000 00000000 00000000 00000010	|
 * |DMA0_abort_ien			| 2		| 00000000 00000000 00000000 00000100	|
 * | 						| 3		| Reserved								|
 * | 						| 4		| Reserved								|
 * |DMA1_abort_ien			| 5		| 00000000 00000000 00000000 00100000	|
 * |MAC_Host_DMA0_busy_ien	| 6		| 00000000 00000000 00000000 01000000	|
 * |MAC_Host_DMA0_done_ien	| 7		| 00000000 00000000 00000000 10000000	|
 * |MAC_Host_DMA0_abort_ien	| 8		| 00000000 00000000 00000001 00000000	|
 * |MAC_Host_DMA1_busy_ien	| 9		| 00000000 00000000 00000010 00000000	|
 * |MAC_Host_DMA1_done_ien	| 10	| 00000000 00000000 00000100 00000000	|
 * |MAC_Host_DMA1_abort_ien	| 11	| 00000000 00000000 00001000 00000000	|
 * | DS_C0_done_ien			| 12	| 00000000 00000000 00010000 00000000	|
 * | DS_C1_done_ien			| 13	| 00000000 00000000 00100000 00000000	|
 * | DS_C2_done_ien			| 14	| 00000000 00000000 01000000 00000000	|
 * | DS_C3_done_ien			| 15	| 00000000 00000000 10000000 00000000	|
 * | DS_C4_done_ien			| 16	| 00000000 00000001 00000000 00000000	|
 * | DS_C5_done_ien			| 17	| 00000000 00000010 00000000 00000000	|
 * | 						| 18-21	| Reserved								|
 * | Master-abort_ien		| 22	| 00000000 01000000 00000000 00000000	|
 * | Target-abort_ien		| 23	| 00000000 10000000 00000000 00000000	|
 * | Lt-timeout_ien			| 24	| 00000001 00000000 00000000 00000000	|
 * | Reported-parity_ien	| 25	| 00000010 00000000 00000000 00000000	|
 * | Detected-parity_ien	| 26	| 00000100 00000000 00000000 00000000	|
 * | System-error_ien		| 27	| 00001000 00000000 00000000 00000000	|
 * | mac_rx_ien				| 28	| 00010000 00000000 00000000 00000000	|
 * | mac_tx_ien				| 29	| 00100000 00000000 00000000 00000000	|
 * | mailbox_ien			| 30	| 01000000 00000000 00000000 00000000	|
 * | mac_event_ien			| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_INTERRUPT_EN__DMA0_BUSY					BIT0
#define BITS_INTERRUPT_EN__DMA0_DONE					BIT1
#define BITS_INTERRUPT_EN__DMA0_ABORT					BIT2
														//Res
														//Reserved
#define BITS_INTERRUPT_EN__DMA1_ABORT					BIT5
#define BITS_INTERRUPT_EN__MAC_HOST_DMA0_BUSY			BIT6
#define BITS_INTERRUPT_EN__MAC_HOST_DMA0_DONE			BIT7
#define BITS_INTERRUPT_EN__MAC_HOST_DMA0_ABORT			BIT8
#define BITS_INTERRUPT_EN__MAC_HOST_DMA1_BUSY			BIT9
#define BITS_INTERRUPT_EN__MAC_HOST_DMA1_DONE			BIT10
#define BITS_INTERRUPT_EN__MAC_HOST_DMA1_ABORT			BIT11
#define BITS_INTERRUPT_EN__DS_C0_DONE					BIT12
#define BITS_INTERRUPT_EN__DS_C1_DONE					BIT13
#define BITS_INTERRUPT_EN__DS_C2_DONE					BIT14
#define BITS_INTERRUPT_EN__DS_C3_DONE					BIT15
#define BITS_INTERRUPT_EN__DS_C4_DONE					BIT16
#define BITS_INTERRUPT_EN__DS_C5_DONE					BIT17
														//Reserved
#define BITS_INTERRUPT_EN__MASTER_ABORT					BIT22
#define BITS_INTERRUPT_EN__TARGET_ABORT					BIT23
#define BITS_INTERRUPT_EN__LT_TIMEOUT					BIT24
#define BITS_INTERRUPT_EN__REPORTED_PARTY				BIT25
#define BITS_INTERRUPT_EN__DETECTED_PARTY				BIT26
#define BITS_INTERRUPT_EN__SYSTERM_ERR					BIT27
#define BITS_INTERRUPT_EN__MAC_RX						BIT28
#define BITS_INTERRUPT_EN__MAC_TX						BIT29
#define BITS_INTERRUPT_EN__MAILBOX						BIT30
#define BITS_INTERRUPT_EN__MAC_EVENT					BIT31







/*
 * #define WLS_CSR__STATUS						0x000C
 *
 * --------------------------------------------------------------------------
 * |	 	 					PCI Control Register 						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |DMA0_busy				| 0		| 00000000 00000000 00000000 00000001	|
 * |DMA0_done				| 1		| 00000000 00000000 00000000 00000010	|
 * |DMA0_abort				| 2		| 00000000 00000000 00000000 00000100	|
 * |						| 3-4	| Reserved								|
 * |DMA1_abort				| 5		| 00000000 00000000 00000000 00100000	|
 * |MAC_Host_DMA0_busy		| 6		| 00000000 00000000 00000000 01000000	|
 * |MAC_Host_DMA0_done		| 7		| 00000000 00000000 00000000 10000000	|
 * |MAC_Host_DMA0_Abort		| 8		| 00000000 00000000 00000001 00000000	|
 * |MAC_Host_DMA1_busy		| 9		| 00000000 00000000 00000010 00000000	|
 * |MAC_Host_DMA1_done		| 10	| 00000000 00000000 00000100 00000000	|
 * |MAC_Host_DMA1_Abort		| 11	| 00000000 00000000 00001000 00000000	|
 * |DS_C0_done				| 12	| 00000000 00000000 00010000 00000000	|
 * |DS_C1_done				| 13	| 00000000 00000000 00100000 00000000	|
 * |DS_C2_done				| 14	| 00000000 00000000 01000000 00000000	|
 * |DS_C3_done				| 15	| 00000000 00000000 10000000 00000000	|
 * |DS_C4_done				| 16	| 00000000 00000001 00000000 00000000	|
 * |DS_C5_done				| 17	| 00000000 00000010 00000000 00000000	|
 * |						| 18-21	| Reserved								|
 * |Master-abort			| 22	| 00000000 01000000 00000000 00000000	|
 * |Target-abort			| 23	| 00000000 10000000 00000000 00000000	|
 * |Lt_timeout				| 24	| 00000001 00000000 00000000 00000000	|
 * |Reported parity			| 25	| 00000010 00000000 00000000 00000000	|
 * |Detected parity error	| 26	| 00000100 00000000 00000000 00000000	|
 * |System error			| 27	| 00001000 00000000 00000000 00000000	|
 * |mac_rx_int				| 28	| 00010000 00000000 00000000 00000000	|
 * |mac_tx_int				| 29	| 00100000 00000000 00000000 00000000	|
 * |mailbox_int				| 30	| 01000000 00000000 00000000 00000000	|
 * |mac_event_int			| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_STATUS__DMA0_BUSY					BIT0
#define BITS_STATUS__DMA0_DONE					BIT1
#define BITS_STATUS__DMA0_ABORT					BIT2
												//Reserved
#define BITS_STATUS__DMA1_ABORT					BIT5
#define BITS_STATUS__MAC_HOST_DMA0_BUSY			BIT6
#define BITS_STATUS__MAC_HOST_DMA0_DONE			BIT7
#define BITS_STATUS__MAC_HOST_DMA0_ABORT		BIT8
#define BITS_STATUS__MAC_HOST_DMA1_BUSY			BIT9
#define BITS_STATUS__MAC_HOST_DMA1_DONE			BIT10
#define BITS_STATUS__MAC_HOST_DMA1_ABORT		BIT11
#define BITS_STATUS__DS_C0_DONE					BIT12
#define BITS_STATUS__DS_C1_DONE					BIT13
#define BITS_STATUS__DS_C2_DONE					BIT14
#define BITS_STATUS__DS_C3_DONE					BIT15
#define BITS_STATUS__DS_C4_DONE					BIT16
#define BITS_STATUS__DS_C5_DONE					BIT17
												//Reserved
#define BITS_STATUS__MASTER_ABORT				BIT22
#define BITS_STATUS__TARGET_ABORT				BIT23
#define BITS_STATUS__LT_TIMEOUT					BIT24
#define BITS_STATUS__REPORTED_PARTY				BIT25
#define BITS_STATUS__DETECTED_PARTY_ERR			BIT26
#define BITS_STATUS__SYSTERM_ERR					BIT27
#define BITS_STATUS__MAC_RX_INT					BIT28
#define BITS_STATUS__MAC_TX_INT					BIT29
#define BITS_STATUS__MAILBOX_INT					BIT30
#define BITS_STATUS__MAC_EVENT_INT				BIT31
























/*
 * #define WLS_CSR__DMA0_SRC_ADDR				0x0020
 * #define WLS_CSR__DMA0_DST_ADDR				0x0024
 *
 * For understanding this register you need read:
 * 1. <<WLAN_STA CardBus Interface>>
 *	Chapter 3 - CardBus Control and Status Registers
 * 2. <<DSP-Shuttle User's Guide>>, 
 *	Chapter 2, DSP-Shuttle Command Formats
 *
 * --------------------------------------------------------------------------
 * |	 				DMA0 SRC/DST ADDRESS Register						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |Source Address			| 0-27	| 00001111 11111111 11111111 11111111	|
 * |access type for CardBus	| 26-27	| 00001100 00000000 00000000 00000000	|
 * |Source Port				| 28-31	| 11110000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 * |Dest Address			| 0-27	| 00001111 11111111 11111111 11111111	|
 * |access type for CardBus	| 26-27	| 00001100 00000000 00000000 00000000	|
 * |Dest Port				| 28-31	| 11110000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------  
 */
#define BITS_DMA0_SD_ADDR__ADDR			BITS_SHUTTLE__SD_WORD__ADDR
#define BITS_DMA0_SD_ADDR__TYPE			(BIT26|BIT27)
#define BITS_DMA0_SD_ADDR__PORT			BITS_SHUTTLE__SD_WORD__PORT

#define OFFSET_DMA0_SD_ADDR__ADDR			OFFSET_SHUTTLE__SD_WORD__ADDR
#define OFFSET_DMA0_SD_ADDR__TYPE			26
#define OFFSET_DMA0_SD_ADDR__PORT			OFFSET_SHUTTLE__SD_WORD__PORT




#define REG_CSR_DMA0_SRC_ADDR(_port,_address) \
		((((_port) << OFFSET_DMA0_SD_ADDR__PORT) & (BITS_DMA0_SD_ADDR__PORT)) \
		  | (((_address) << OFFSET_DMA0_SD_ADDR__ADDR) & (BITS_DMA0_SD_ADDR__ADDR)) )



#define REG_CSR_DMA0_DST_ADDR(_port,_address) \
		((((_port) << OFFSET_DMA0_SD_ADDR__PORT) & (BITS_DMA0_SD_ADDR__PORT)) \
		  | (((_address) << OFFSET_DMA0_SD_ADDR__ADDR) & (BITS_DMA0_SD_ADDR__ADDR)) )




/*
 * #define WLS_CSR__DMA0_CONTROL				0x0028
 *
 * For understanding this register you need read:
 * 1. <<WLAN_STA CardBus Interface>>
 *	Chapter 3 - CardBus Control and Status Registers
 * 2. <<DSP-Shuttle User's Guide>>, 
 *	Chapter 2, DSP-Shuttle Command Formats
 *
 * --------------------------------------------------------------------------
 * |	 					DMA0 control Register							|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |WrCtl					| 0		| 00000000 00000000 00000000 00000001	|
 * |WrLnk					| 1		| 00000000 00000000 00000000 00000010	|
 * |Mode					| 2-3	| 00000000 00000000 00000000 00001100	|
 * |RdCtl					| 4		| 00000000 00000000 00000000 00010000	|
 * |WrStp					| 5		| 00000000 00000000 00000000 00100000	|
 * |RdStp					| 6		| 00000000 00000000 00000000 01000000	|
 * |Number of Bytes			| 7-20	| 00000000 00011111 11111111 10000000	|
 * |Burst Size				| 21-23	| 00000000 11100000 00000000 00000000	|
 * |Host2Shuttle 			| 24	| 00000001 00000000 00000000 00000000	|
 * |Shuttle2Host 			| 25	| 00000010 00000000 00000000 00000000	|
 * |HiP						| 26	| 00000100 00000000 00000000 00000000	|
 * |FIFO					| 27	| 00001000 00000000 00000000 00000000	|
 * |Priority				| 28-31	| 11110000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_DMA0_CTRL__WR_CTL					BITS_SHUTTLE__CTRL_WORD__WR_CTL
#define BITS_DMA0_CTRL__WR_LNK					BITS_SHUTTLE__CTRL_WORD__WR_LNK
#define BITS_DMA0_CTRL__MODE					BITS_SHUTTLE__CTRL_WORD__MODE
#define BITS_DMA0_CTRL__RD_CTL					BITS_SHUTTLE__CTRL_WORD__RD_CTL
#define BITS_DMA0_CTRL__WR_STP					BITS_SHUTTLE__CTRL_WORD__WR_STP
#define BITS_DMA0_CTRL__RD_STP					BITS_SHUTTLE__CTRL_WORD__RD_STP
#define BITS_DMA0_CTRL__NUM_OF_BYTES			BITS_SHUTTLE__CTRL_WORD__NUM_OF_BYTES
#define BITS_DMA0_CTRL__BURST_SIZE				BITS_SHUTTLE__CTRL_WORD__BURST_SIZE
#define BITS_DMA0_CTRL__HOST2SHUTTLE			BIT24
#define BITS_DMA0_CTRL__SHUTTLE2HOST			BIT25
#define BITS_DMA0_CTRL__HIP						BITS_SHUTTLE__CTRL_WORD__HIP
#define BITS_DMA0_CTRL__FIFO					BITS_SHUTTLE__CTRL_WORD__FIFO
#define BITS_DMA0_CTRL__PRIORITY				BITS_SHUTTLE__CTRL_WORD__PRIORITY


#define OFFSET_DMA0_CTRL__WR_CTL				OFFSET_SHUTTLE__CTRL_WORD__WR_CTL
#define OFFSET_DMA0_CTRL__WR_LNK				OFFSET_SHUTTLE__CTRL_WORD__WR_LNK
#define OFFSET_DMA0_CTRL__MODE					OFFSET_SHUTTLE__CTRL_WORD__MODE
#define OFFSET_DMA0_CTRL__RD_CTL				OFFSET_SHUTTLE__CTRL_WORD__RD_CTL
#define OFFSET_DMA0_CTRL__WR_STP				OFFSET_SHUTTLE__CTRL_WORD__WR_STP
#define OFFSET_DMA0_CTRL__RD_STP				OFFSET_SHUTTLE__CTRL_WORD__RD_STP
#define OFFSET_DMA0_CTRL__NUM_OF_BYTES			OFFSET_SHUTTLE__CTRL_WORD__NUM_OF_BYTES
#define OFFSET_DMA0_CTRL__BURST_SIZE			OFFSET_SHUTTLE__CTRL_WORD__BURST_SIZE
#define OFFSET_DMA0_CTRL__HOST2SHUTTLE			24
#define OFFSET_DMA0_CTRL__SHUTTLE2HOST			25
#define OFFSET_DMA0_CTRL__HIP					OFFSET_SHUTTLE__CTRL_WORD__HIP
#define OFFSET_DMA0_CTRL__FIFO					OFFSET_SHUTTLE__CTRL_WORD__FIFO
#define OFFSET_DMA0_CTRL__PRIORITY				OFFSET_SHUTTLE__CTRL_WORD__PRIORITY



#define REG_CSR_DMA0_CONTROL(_priority,_fifo,_hiperf,_shuttle2host,_host2shuttle,_burstsize,_numofbytes,_readstop,_writestop,_readctl,_mode,_writelink,_writectl) \
		((((_priority) << OFFSET_DMA0_CTRL__PRIORITY) & (BITS_DMA0_CTRL__PRIORITY)) \
		 | (((_fifo) << OFFSET_DMA0_CTRL__FIFO) & (BITS_DMA0_CTRL__FIFO)) \
		 | (((_hiperf) << OFFSET_DMA0_CTRL__HIP) & (BITS_DMA0_CTRL__HIP)) \
		 | (((_shuttle2host) << OFFSET_DMA0_CTRL__SHUTTLE2HOST) & (BITS_DMA0_CTRL__SHUTTLE2HOST)) \
		 | (((_host2shuttle) << OFFSET_DMA0_CTRL__HOST2SHUTTLE) & (BITS_DMA0_CTRL__HOST2SHUTTLE)) \
		 | (((_burstsize) << OFFSET_DMA0_CTRL__BURST_SIZE) & (BITS_DMA0_CTRL__BURST_SIZE)) \
		 | ((((_numofbytes)-1) << OFFSET_DMA0_CTRL__NUM_OF_BYTES) & (BITS_DMA0_CTRL__NUM_OF_BYTES)) \
		 | (((_readstop) << OFFSET_DMA0_CTRL__RD_STP) & (BITS_DMA0_CTRL__RD_STP)) \
		 | (((_writestop) << OFFSET_DMA0_CTRL__WR_STP) & (BITS_DMA0_CTRL__WR_STP)) \
		 | (((_readctl) << OFFSET_DMA0_CTRL__RD_CTL) & (BITS_DMA0_CTRL__RD_CTL)) \
		 | (((_mode) << OFFSET_DMA0_CTRL__MODE) & (BITS_DMA0_CTRL__MODE)) \
		 | (((_writelink) << OFFSET_DMA0_CTRL__WR_LNK) & (BITS_DMA0_CTRL__WR_LNK)) \
	     | (((_writectl) << OFFSET_DMA0_CTRL__WR_CTL) & (BITS_DMA0_CTRL__WR_CTL)) )







/*
 * #define WLS_CSR__FUNCTION_EVENT				0x0040
 * #define WLS_CSR__FUNCTION_EVENT_MASK			0x0044
 * #define WLS_CSR__FUNCTION_PRESENT_STATE		0x0048
 * #define WLS_CSR__FUNCTION_FORCE_EVENT		0x004C
 *
 * --------------------------------------------------------------------------
 * |	 	 			CardBus PC Card Status Registers					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |Write Protect			| 0		| 00000000 00000000 00000000 00000001	|
 * |Ready/Busy				| 1		| 00000000 00000000 00000000 00000010	|
 * |Battery Voltage Detect2	| 2		| 00000000 00000000 00000000 00000100	|
 * |Battery Voltage Detect1	| 3		| 00000000 00000000 00000000 00001000	|
 * |General Wakeup			| 4		| 00000000 00000000 00000000 00010000	|
 * |RSVD					| 5-14	| 00000000 00000000 01111111 11100000	|
 * |INTR					| 15	| 00000000 00000000 10000000 00100000	|
 * | 						| 16-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_FUNCTION__WRITE_PROTECT				BIT0
#define BITS_FUNCTION__READY						BIT1
#define BITS_FUNCTION__BVD2							BIT2
#define BITS_FUNCTION__BVD1							BIT3
#define BITS_FUNCTION__GENERAL_WAKEUP				BIT4
#define BITS_FUNCTION__RSVD							(BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14)
#define BITS_FUNCTION__INTR							BIT15
													//Reserved

/*
 * --------------------------------------------------------------------------
 * | Registers				| INTR Field									|
 * --------------------------------------------------------------------------
 * |Function Event			| Interrupt bit field is set (1) when the INTR 	|
 * |(0x0040)				| field in the Function Force Event Register  	|
 * |						| is set.  The host system clears the INTR 		|
 * |						| field by writing a 1 to this field. Writing  	|
 * |						| a 0 to this field has no effect.  The state 	|
 * |						| after reset is 0. 							|
 * --------------------------------------------------------------------------
 * |Function Event Mask		| Interrupt Mask.  When cleared (0), setting 	|
 * |(0x0044)				| of the INTR field in either the Function		|
 * |						| Present State register or the Function Event 	|
 * |						| register will neither cause assertion of the 	|
 * |						| functional interrupt on the CINT# line while 	|
 * |						| the CardBus PC Card interface is powered up, 	|
 * |						| nor the system Wakeup while the interface   	| 
 * | 						| is powered off.  Setting this field to 1, 	|
 * |						| enables the INTR field in both the Function  	|
 * |						| Present State register and the Function Event	|
 * |						| register to generate the functional interrupt	|
 * --------------------------------------------------------------------------
 * |Function Present State	| Interrupt field represents the internal state	|
 * |(0x0048)				| of a function specific interrupt request.This	|
 * |						| field remains set (1), until the condition 	|
 * |						| that caused the interrupt has been serviced. 	|
 * |						| It is cleared (0) by the function when the 	|
 * |						| event has been serviced. The value of INTR 	|
 * |						| field is available even if the interrupts 	|
 * |						| have not been configured. This field is not 	| 
 * |						| affected by CRST#. This field is set when any	|
 * |						| bit in Status Register (0x000C) is set and 	|
 * |						| the corresponding bit in Interrupt Enable 	|
 * |						| Register (0x0008) is set.						|
 * --------------------------------------------------------------------------
 * |Function Force Event 	| Writing a 1 to this bit field sets the INTR 	|
 * |(0x004C)				| field in the Function Event register.However,	|
 * |						| the INTR field in the Function Present State 	|
 * |						| register is not affected and continues to  	|
 * |						| reflect the current state of the functional	|
 * |						| interrupt.  Writing a 0 to this field has no 	|
 * |						| effect.										| 
 * --------------------------------------------------------------------------
 */



















/*
 * #define WLS_CSR__MAC_HOST_DMA0_CTRL_REG				0x0098
 * #define WLS_CSR__MAC_HOST_DMA1_CTRL_REG				0x00B4
 *
 * --------------------------------------------------------------------------
 * |	 MAC_Host_DMA0_Control_Register / MAC_Host_DMA1_Control_Register	|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |Number of Requests		| 0-3	| 00000000 00000000 00000000 00001111	|
 * |Burst Size				| 4-5	| 00000000 00000000 00000000 00110000	|
 * |D						| 6		| 00000000 00000000 00000000 01000000	|
 * |A						| 7		| 00000000 00000000 00000000 10000000	|
 * | 						| 8-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_MAC_HOST_DMA_CTRL_REG__REQ_NUM			(BIT0|BIT1|BIT2|BIT3)
#define BITS_MAC_HOST_DMA_CTRL_REG__BURST				(BIT4|BIT5)
#define BITS_MAC_HOST_DMA_CTRL_REG__D					BIT6
#define BITS_MAC_HOST_DMA_CTRL_REG__A					BIT7

#define OFFSET_MAC_HOST_DMA_CTRL_REG__BURST				4
#define OFFSET_MAC_HOST_DMA_CTRL_REG__D					6
#define OFFSET_MAC_HOST_DMA_CTRL_REG__A					7


#define REG_M_H_DMA_CONTROL(Activation, XferDirect, BurstSize, NoOfRequest) \
		((((Activation) << OFFSET_MAC_HOST_DMA_CTRL_REG__A) & (BITS_MAC_HOST_DMA_CTRL_REG__A)) \
		 | (((XferDirect) << OFFSET_MAC_HOST_DMA_CTRL_REG__D) & (BITS_MAC_HOST_DMA_CTRL_REG__D)) \
		 | (((BurstSize) << OFFSET_MAC_HOST_DMA_CTRL_REG__BURST) & (BITS_MAC_HOST_DMA_CTRL_REG__BURST)) \
		 | (((NoOfRequest)-1) & (BITS_MAC_HOST_DMA_CTRL_REG__REQ_NUM)))











/**************************************************************************
 **************************************************************************
 **																		 **
 **																		 **
 **		3DSP Wireless LAN Station (Scratchpad)  Registers Macros 		 **
 **																		 **
 **																		 **
 **************************************************************************
 *************************************************************************/


#define SCRATCH__OFFSET_PAD					(_OFFSET_SPD + 0x0C00)

#define WLS_SCRATCH__BASEBAND_VER	    	(SCRATCH__OFFSET_PAD + 0x0000)	//Baseband version number
#define WLS_SCRATCH__SP20_READY    		(	SCRATCH__OFFSET_PAD + 0x0004)	//SP-20 Ready
#define WLS_SCRATCH__RX_OVERFLOW	    	(SCRATCH__OFFSET_PAD + 0x0008)	//RxBufOverflowFlag
#define WLS_SCRATCH__CTS_PREAMBLE	    	(SCRATCH__OFFSET_PAD + 0x000C)	//CTSPreambleType_Bitrate

#define WLS_SCRATCH__RX_C_PTR    			(SCRATCH__OFFSET_PAD + 0x0010)	//rx consumer pointer
#define WLS_SCRATCH__TX_PRIORITY_PTR    	(SCRATCH__OFFSET_PAD + 0x0014)	//tx producer pointer for Priority FIFO
#define WLS_SCRATCH__TX_CFP_PTR   	 		(SCRATCH__OFFSET_PAD + 0x0018)	//tx producer pointer for Contention Free Period FIFO
#define WLS_SCRATCH__TX_CP_PTR   	 		(SCRATCH__OFFSET_PAD + 0x001C)	//tx producer pointer for Contention Period FIFO


#define WLS_SCRATCH__TX_BCN_PPTR    		(SCRATCH__OFFSET_PAD + 0x0020)	//TxBeaconPptr
#define WLS_SCRATCH__RX_BUF_SEG_PPTR    	(SCRATCH__OFFSET_PAD + 0x0024)	//RxBufSegNextToInitialPptr
#define WLS_SCRATCH__RX_BUF_INIT_PPTR  	 (SCRATCH__OFFSET_PAD + 0x0028)	//RxBufInitialPptr
#define WLS_SCRATCH__RX_BUF_SEG  	 		(SCRATCH__OFFSET_PAD + 0x002C)	//RxBufSegs(0 < RxBufSegs <= 0x10)


#define WLS_SCRATCH__RX_SEG_BD    			(SCRATCH__OFFSET_PAD + 0x0030)	//RxSegBD[0]











/*
 * Host controlled features
 * Some special feaures can be controlled through configuring the bit 
 * fields in CTSPreambleType_Bitrate word which has the following bit 
 * definitions,
 *
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | bitrate				| 0-1	| 00000000 00000000 00000000 00000011	|
 * |						| 2-15	| reserved								|
 * | preamble type			| 16	| 00000000 00000001 00000000 00000000	|
 * |						| 17-30	| reserved								|
 * | Correlator on/off		| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 * 
 * bitrate: 			0 => 1 Mbps
 *						1 => 2 Mbps
 *						2 => 5.5 Mbps
 *						3 => 11 Mbps
 *
 * preamble type : 		0 => long
 *						1 => short
 *
 * Correlator on/off:	1 => correlator off
 *						0 => correlator on
 *
 * NOTE: The correlator can be turned on and off dynamically if the Host 
 * is acting as a station; for access point only the value set during 
 * initialization will take effect.
 */
#define BITS_HOST_CTRL_FEATURE__BITRATE					(BIT0|BIT1)
														//Reserved
#define BITS_HOST_CTRL_FEATURE__PREAMBLE_TYPE			BIT16
														//Reserved
#define BITS_HOST_CTRL_FEATURE__CORRELATOR				BIT31

//get from ftang,FOR combo cts
#define BITS_AP_ASSOCIATED_BIT					BIT30
#define BITS_SOFT_DOZE_BIT						BIT29


#define OFFSET_HOST_CTRL_FEATURE__PREAMBLE_TYPE			16
#define OFFSET_HOST_CTRL_FEATURE__CORRELATOR			31

#define VALUE__HOST_CTRL_FEATURE(correlator, preamble_type, bitrate) \
		((((correlator) << OFFSET_HOST_CTRL_FEATURE__CORRELATOR) & (BITS_HOST_CTRL_FEATURE__CORRELATOR)) \
		 |(((preamble_type) << OFFSET_HOST_CTRL_FEATURE__PREAMBLE_TYPE) & (BITS_HOST_CTRL_FEATURE__PREAMBLE_TYPE)) \
		 | ((bitrate) & (BITS_HOST_CTRL_FEATURE__BITRATE)))










/*
 * The Tx power Look-up table for each channel is 3-word long and has the 
 * following format,
 *
 * High                                                   Low
 * ----------------------------------------------------------
 * |    11 Mbps	|     5.5 Mbps	  |  2 Mbps		|  1 Mbps	| low
 * ----------------------------------------------------------
 * |    6 Mbps	|     12 Mbps	  |  24 Mbps	|  48 Mbps	|
 * ----------------------------------------------------------
 * |    9 Mbps	|     18 Mbps	  |  36 Mbps	|  54 Mbps	| high
 * ----------------------------------------------------------
 *
 * Each octet contains a value from 0~63 representing the optimal transmit 
 * power for the corresponding transmit bitrate.  The HOST software needs to
 * load this 3-word table to scratch memory address 0xFD8 prior to a channel 
 * configuration in order for the baseband software to load into DSP memory.
 *
 */
#define WLS_SCRATCH__TXPOWER_TABLE			(_OFFSET_SPD + 0xFD8	)			//txpower table









#define CIS_BASEBAND_VERSION 0x2C00



#define SCRATCH_BASE_ADDR 		   		0x2000
#define SCRATCH_PAD_ADDR 	   	      		0xC00
#define SCRATCH_CTS_FCS_TABLE   		0x00000400

#define SCRATCH_RAM_OFFSET 1024





//mailbox
//unsigned int channel_num : 8, bcn_len : 12, : 4, txpwrlevel : 6, antenna_sel : 1, slottime : 1;
//unsigned int : 16, bb_int : 1, gen_int1 : 1, gen_int2 : 1, : 13;

#define MAILBOX_CHANNEL_POS                         0
#define MAILBOX_CHANNEL_BITMAP                   (BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)

#define MAILBOX_ANTENNA_POS                        6
#define MAILBOX_ANTENNA_BITMAP                  BIT6












/**************************************************************************
 **************************************************************************
 **																		 **
 **																		 **
 **			3DSP Wireless LAN Station (MAC)  Registers Macros			 **
 **																		 **
 **																		 **
 **************************************************************************
 *************************************************************************/
#define WLS_MAC__SIGNATURE				_OFFSET_MAC + 0x0000	//signature
#define WLS_MAC__VERSION				_OFFSET_MAC + 0x0004	//version
#define WLS_MAC__MAC_ADDR_LO			_OFFSET_MAC + 0x0008	//macAddrLo
#define WLS_MAC__MAC_ADDR_HI			_OFFSET_MAC + 0x000C	//macAddrHi
#define WLS_MAC__BSS_ID_LO				_OFFSET_MAC + 0x0010	//bssIDLo
#define WLS_MAC__BSS_ID_HI				_OFFSET_MAC + 0x0014	//bssIDHi
#define WLS_MAC__STATE_CONTROL			_OFFSET_MAC + 0x0018	//stateControl
#define WLS_MAC__CONTROL				_OFFSET_MAC + 0x001C	//control 
#define WLS_MAC__PROBE_TIMER			_OFFSET_MAC + 0x0020	//probeTimer 
#define WLS_MAC__BEACON_INTERVAL		_OFFSET_MAC + 0x0024	//beaconInterval 
#define WLS_MAC__TX_BCN_LENGTH			_OFFSET_MAC + 0x0028	//txBcnLength 

#define WLS_MAC__SCAN_CH_TIMER			_OFFSET_MAC + 0x002C	//scanChTimer 
#define WLS_MAC__ATIM					_OFFSET_MAC + 0x002C	//atim 

#define WLS_MAC__RTS_RETRY				_OFFSET_MAC + 0x0030	//rtsThreshRtLim 
#define WLS_MAC__INT_EVT_SET			_OFFSET_MAC + 0x0034	//intEventSet 
#define WLS_MAC__INT_EVT_CLEAR			_OFFSET_MAC + 0x0038	//intEventClear 
#define WLS_MAC__INT_MASK				_OFFSET_MAC + 0x003C	//intMask 
#define WLS_MAC__STATUS					_OFFSET_MAC + 0x0040	//status


#define WLS_MAC__CFP					_OFFSET_MAC + 0x004C	//cfp


#define WLS_MAC__TX_FIFO_SIZE			_OFFSET_MAC + 0x0050	//txFIFOSize 
#define WLS_MAC__FIFO_SELECT			_OFFSET_MAC + 0x0058	//fifoSel 




#define WLS_MAC__TIMINGS_1REG			_OFFSET_MAC + 0x0060	//timings1Reg 
#define WLS_MAC__TIMINGS_2REG			_OFFSET_MAC + 0x0064	//timings2Reg 

//bit define
#define V4MAC_SLOT_TIME_BIT                	BIT_22_26
#define V2MAC_SLOT_TIME_BIT		       BIT_24_28
#define WLS_MAC__GEN_TIMER				_OFFSET_MAC + 0x0074	//genTimer
//v3chip
#define WLS_MAC__BBREG_ACCESS_WD          _OFFSET_MAC  + 0x0078

#define WLS_MAC__RADIO_ACCESS			_OFFSET_MAC + 0x007C	//radioAccess

#define WLS_MAC_INTBBMASK_WD 				_OFFSET_MAC + 0x0080

#define WLS_MAC__BRS11G_OFDM			_OFFSET_MAC + 0x008C	//BRS11GOFDM
#define BRS11G_BASIC_OFDM_BIT               		BIT_0_7
#define BRS11G_BASIC_OFDM_RTS_BIT 	       BIT_8_11
#define BRS11G_BASIC_DSSS_RTS_BIT 	       	BIT_12_13
#define BRS11G_OFDM_CTS_SELF_BIT                 BIT14


#define WLS_MAC__KEY_WORD_0				_OFFSET_MAC + 0x0090	//Key Word0 Reg
#define WLS_MAC__KEY_WORD_1				_OFFSET_MAC + 0x0094	//Key Word1 Reg
#define WLS_MAC__KEY_WORD_2				_OFFSET_MAC + 0x0098	//Key Word2 Reg
#define WLS_MAC__KEY_WORD_3				_OFFSET_MAC + 0x009C	//Key Word3 Reg
#define WLS_MAC__KEY_MAC_ADDR			_OFFSET_MAC + 0x00A0	//keyAddr 
#define WLS_MAC__KEY_CNTL_ADDR			_OFFSET_MAC + 0x00A4	//keyCntlAddr 
#define WLS_MAC__KEY_SIZE_CTRL			_OFFSET_MAC + 0x00A8	//keySizeCntl

#define WLS_MAC__KEYOPERATIONCNTL_WD	_OFFSET_MAC + 0x40AC



#define WLS_MAC__TX_FRAG_CNT			_OFFSET_MAC + 0x00B0	//txFrgCnt 
#define WLS_MAC__RX_FRAG_CNT			_OFFSET_MAC + 0x00B4	//rxFrgCount 
#define WLS_MAC__BCNBOCNT_WD              _OFFSET_MAC + 0x00B8
#define WLS_MAC__PRIBOCNT_WD              _OFFSET_MAC + 0x00BC
#define WLS_MAC__TAFORNOKEYLOW_WD	_OFFSET_MAC +  0x00C0



#define WLS_MAC__CP_BO_CNT			_OFFSET_MAC + 0x00C8	//cpBOCnt
#define WLS_MAC__NAVCWCNT_WD              _OFFSET_MAC + 0x00CC

#define WLS_MAC__DEBUG0_WD                   _OFFSET_MAC + 0x00D0
#define WLS_MAC__DEBUG1_WD                   _OFFSET_MAC + 0x00D4
#define WLS_MAC__DEBUG2				_OFFSET_MAC + 0x00D8

#define MAC_AIFSN_BIT                    BIT_10_17



//v4chip
#define WLS_MAC__STARETRYCNT_WD           _OFFSET_MAC + 0x00E0
#define WLS_MAC__CPRETRYCNT_WD            _OFFSET_MAC + 0x00E4
#define WLS_MAC__CFPRETRYCNT_WD           _OFFSET_MAC + 0x00E8


#define WLS_MAC__PERF_COUNT0			_OFFSET_MAC + 0x00F0	//perfCount0 
#define WLS_MAC__PERF_COUNT1			_OFFSET_MAC + 0x00F4	//perfCount1 
#define WLS_MAC__PERF_COUNT2			_OFFSET_MAC + 0x00F8	//perfCount2 
#define WLS_MAC__PERF_COUNT3			_OFFSET_MAC + 0x00Fc	//perfCount3 


#define WLS_MAC__CHMEASUREREGLO_WD       _OFFSET_MAC + 0x0100
#define WLS_MAC__CHMEASUREREGHI_WD        _OFFSET_MAC + 0x0104
#define WLS_MAC__QOS_DISABLE_WD           _OFFSET_MAC + 0x0108
//bits
#define MAC_QOS_DIS_BIT	  			     BIT_0

#define WLS_MAC__EDCAPARAM0_WD            _OFFSET_MAC + 0x010C
#define WLS_MAC__EDCAPARAM1_WD            _OFFSET_MAC + 0x0110
#define WLS_MAC__EDCAPARAM2_WD            _OFFSET_MAC + 0x0114
#define WLS_MAC__EDCAPARAM3_WD            _OFFSET_MAC + 0x0118
#define WLS_MAC__DEFAULTPARAMINDEX_WD     _OFFSET_MAC + 0x011C
#define WLS_MAC__RETRYTXPOLICY_WD         _OFFSET_MAC + 0x0120
#define WLS_MAC__CURTXOPDEBUG_WD          _OFFSET_MAC + 0x0124

#define WLS_MAC__DEBUG3_WD                _OFFSET_MAC + 0x0130
#define WLS_MAC__DEBUG4_WD                _OFFSET_MAC + 0x0134
#define WLS_MAC__TSCACCESSLO_WD           _OFFSET_MAC + 0x0138
#define WLS_MAC__TSCACCESSCNTLHI_WD       _OFFSET_MAC + 0x013C



#define WLS_MAC__BBREG_0_3				_OFFSET_MAC + 0x0200	//
#define WLS_MAC__BBREG_4_7				_OFFSET_MAC + 0x0204	//
#define WLS_MAC__BBREG_8_11				_OFFSET_MAC + 0x0208	//
#define WLS_MAC__BBREG_12_15			_OFFSET_MAC + 0x020C	//
#define WLS_MAC__BBREG_16_19			_OFFSET_MAC + 0x0210	//
#define WLS_MAC__BBREG_20_23			_OFFSET_MAC + 0x0214	//
#define WLS_MAC__BBREG_24_27			_OFFSET_MAC + 0x0218	//
#define WLS_MAC__BBREG_28_31			_OFFSET_MAC + 0x021C	//
#define WLS_MAC__BBREG_32_35			_OFFSET_MAC + 0x0220	//
#define WLS_MAC__BBREG_36_38			_OFFSET_MAC + 0x0224	//


#define BBREG_SLOT_TIME_BIT  BIT_31

#define WLS_MAC__TX_FIFO_RW_PORT		_OFFSET_MAC + 0x0400	//txFIFOWrPort/ txFIFORdPort
#define WLS_MAC__RX_FIFO_RW_PORT		_OFFSET_MAC + 0x0800	//rxFIFORdPort/rxFIFOWrPort


#define ANTENNA_SELECT_BIT               BIT_30
#define CHANNEL_NUMBER_BIT               BIT_0_7
#define MAILBOX_SET_USEPD                 BIT_24




/*
 * #define WLS_MAC__VERSION				_OFFSET_MAC + 0x0004
 *
 * --------------------------------------------------------------------------
 * |	 					 Version Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | UM version				| 0-6	| 00000000 00000000 00000000 01111111	|
 * | I/E Release			| 7		| 00000000 00000000 00000000 10000000	|
 * | Release number			| 8-13	| 00000000 00000000 00111111 00000000	|
 * | Phase number			| 14-16	| 00000000 00000001 11000000 00000000	|
 * | PS						| 17	| 00000000 00000010 00000000 00000000	|
 * | CCMP					| 18	| 00000000 00000100 00000000 00000000	|
 * | TKIP					| 19	| 00000000 00001000 00000000 00000000	|
 * | WEP					| 20	| 00000000 00010000 00000000 00000000	|
 * | 11k					| 21	| 00000000 00100000 00000000 00000000	|
 * | 11j					| 22	| 00000000 01000000 00000000 00000000	|
 * | 11i					| 23	| 00000000 10000000 00000000 00000000	|
 * | 11h					| 24	| 00000001 00000000 00000000 00000000	|
 * | 11g					| 25	| 00000010 00000000 00000000 00000000	|
 * | 11e					| 26	| 00000100 00000000 00000000 00000000	|
 * | 11b					| 27	| 00001000 00000000 00000000 00000000	|
 * | 11a					| 28	| 00010000 00000000 00000000 00000000	|
 * | IBSS					| 29	| 00100000 00000000 00000000 00000000	|
 * | PCF					| 30	| 01000000 00000000 00000000 00000000	|
 * | AP						| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_VERSION__UM_VERSION				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6)
#define BITS_VERSION__IE_RELEASE				BIT7
#define BITS_VERSION__RELEASE_NUM				(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13)
#define BITS_VERSION__PJASE_NUM				(BIT14|BIT15|BIT16)
#define BITS_VERSION__PS						BIT17
#define BITS_VERSION__CCMP						BIT18
#define BITS_VERSION__TKIP						BIT19
#define BITS_VERSION__WEP						BIT20
#define BITS_VERSION__11K						BIT21
#define BITS_VERSION__11J						BIT22
#define BITS_VERSION__11I						BIT23
#define BITS_VERSION__11H						BIT24
#define BITS_VERSION__11G						BIT25
#define BITS_VERSION__11E						BIT26
#define BITS_VERSION__11B						BIT27
#define BITS_VERSION__11A						BIT28
#define BITS_VERSION__IBSS						BIT29
#define BITS_VERSION__PCF						BIT30
#define BITS_VERSION__AP						BIT31







/*
 * #define WLS_MAC__STATE_CONTROL			_OFFSET_MAC + 0x0018
 *
 * --------------------------------------------------------------------------
 * |	 				State Control Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | currentState			| 0-3	| 00000000 00000000 00000000 00001111	|
 * | nextState				| 4-7	| 00000000 00000000 00000000 11110000	|
 * | activeScan				| 8		| 00000000 00000000 00000001 00000000	|
 * | 						| 9-15	| Reserved								|
 * | listenInterval			| 16-31	| 11111111 11111111 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_STATE_CONTROL__CURR_STATE				(BIT0|BIT1|BIT2|BIT3)
#define BITS_STATE_CONTROL__NEXT_STATE				(BIT4|BIT5|BIT6|BIT7)
#define BITS_STATE_CONTROL__ACTIVE_SCAN				BIT8
													//Reserved
#define BITS_STATE_CONTROL__LISTEN_INTERVAL			(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)


#define OFFSET_STATE_CONTROL__CURR_STATE				0
#define OFFSET_STATE_CONTROL__NEXT_STATE				4
#define OFFSET_STATE_CONTROL__ACTIVE_SCAN				8
#define OFFSET_STATE_CONTROL__LISTEN_INTERVAL			16

#define REG_STATE_CONTROL(next_state, active_scan, listen_interval) \
		((((next_state) << OFFSET_STATE_CONTROL__NEXT_STATE) & (BITS_STATE_CONTROL__NEXT_STATE)) \
		 | (((active_scan) << OFFSET_STATE_CONTROL__ACTIVE_SCAN) & (BITS_STATE_CONTROL__ACTIVE_SCAN)) \
		 | ((((listen_interval)) << OFFSET_STATE_CONTROL__LISTEN_INTERVAL ) & (BITS_STATE_CONTROL__LISTEN_INTERVAL)))
//		 | ((((listen_interval)-1) << OFFSET_STATE_CONTROL__LISTEN_INTERVAL ) & (BITS_STATE_CONTROL__LISTEN_INTERVAL)))




/*
 * #define WLS_MAC__CONTROL				_OFFSET_MAC + 0x001C
 *
 * --------------------------------------------------------------------------
 * |	 					Control Register Fields							|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | softReset				| 0		| 00000000 00000000 00000000 00000001	|
 * | doat11a				| 1		| 00000000 00000000 00000000 00000010	|
 * | bssType				| 2		| 00000000 00000000 00000000 00000100	|
 * | ap						| 3		| 00000000 00000000 00000000 00001000	|
 * | dupFilterEn			| 4		| 00000000 00000000 00000000 00010000	|
 * | promiscuousEn			| 5		| 00000000 00000000 00000000 00100000	|
 * | fcsCheckDisable		| 6		| 00000000 00000000 00000000 01000000	|
 * | multicastRxDis			| 7		| 00000000 00000000 00000000 10000000	|
 * | basicRateSet[7:0]		| 8-15	| 00000000 00000000 11111111 00000000	|
 * | wakeupDTIM				| 16	| 00000000 00000001 00000000 00000000	|
 * | dot11g					| 17	| 00000000 00000010 00000000 00000000	|
 * | setACWMIN15			| 18	| 00000000 00000100 00000000 00000000	|
 * | excUnencrypted			| 19	| 00000000 00001000 00000000 00000000	|
 * | wrUndecryptedRx		| 20	| 00000000 00010000 00000000 00000000	|
 * | 						| 21-22	| Reserved								|
 * | cfPollableSTA			| 23	| 00000000 10000000 00000000 00000000	|
 * | rxFIFOFlush			| 24	| 00000001 00000000 00000000 00000000	|
 * | txFIFOFlush			| 25	| 00000010 00000000 00000000 00000000	|
 * | 						| 26-27	| Reserved								|
 * | Ignore_ID_on_receive	| 28	| 00010000 00000000 00000000 00000000	|
 * | pwrMgt					| 29	| 00100000 00000000 00000000 00000000	|
 * | 						| 30	| Reserved								|
 * | byteSwapEn				| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_CONTROL__SOFT_RESET				BIT0
#define BITS_CONTROL__DOT11A					BIT1
#define BITS_CONTROL__BSS_TYPE					BIT2
#define BITS_CONTROL__AP						BIT3
#define BITS_CONTROL__DUP_FILTER_FN				BIT4
#define BITS_CONTROL__PROMISCUOUS_EN			BIT5
#define BITS_CONTROL__FCS_CHECK_DISABLE			BIT6
#define BITS_CONTROL__MULTICAST_RX_DIS			BIT7
#define BITS_CONTROL__BASIC_RATE_SET			(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_CONTROL__WAKEUP_DTIM				BIT16
#define BITS_CONTROL__DOT11G					BIT17
#define BITS_CONTROL__SET_ACWMIN_15				BIT18
#define BITS_CONTROL__EXC_UNENCRYPTED			BIT19
#define BITS_CONTROL__WR_UNDECRYPTED_RX			BIT20
												// Reserved
#define BITS_CONTROL__CF_POLLABLE_STA			BIT23
#define BITS_CONTROL__RX_FIFO_FLUSH				BIT24
#define BITS_CONTROL__TX_FIFO_FLUSH				BIT25
												// Reserved
#define BITS_CONTROL__IGNORE_ID_ON_RCV			BIT28
#define BITS_CONTROL__PWR_MGT					BIT29
												// Reserved
#define BITS_CONTROL__BYTE_SWAP_EN				BIT31



#define BITS_CONTROL__RATESET_SPECIAL			(BIT8|BIT9|BIT10|BIT11)




#define BITS_CONTROL__BASIC_RATE_SET_P1			(BIT8|BIT9|BIT10|BIT11)
#define BITS_CONTROL__BASIC_RATE_SET_P2			(BIT12|BIT13|BIT14|BIT15)


#define OFFSET_CONTROL__BASIC_RATE_SET			8
#define OFFSET_CONTROL__BASIC_RATE_SET_P2		12
#define OFFSET_CONTROL__EXC_UNENCRYPTED			19



/*
 * BSS Basic Rate Set.
 * 	This field is used to determine the transmission speed of some of the 
 * 	packets that are sent by hardware.
 *
 * The decoding of this field is as follows:
 * --------------------------------------------------------------------------
 * | In the 802.11 a mode:													|
 * |																		|
 * |	If basicRateSet[0] is set, 6 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[1] is set, 9 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[2] is set, 12 Mbps belongs to the BSSBasicRateSet 	|
 * |	If basicRateSet[3] is set, 18 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[4] is set, 24 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[5] is set, 36 Mbps belongs to the BSSBasicRateSet 	|
 * |	If basicRateSet[6] is set, 48 Mbps belongs to the BSSBasicRateSet 	|
 * |	If basicRateSet[7] is set, 54 Mbps belongs to the BSSBasicRateSet	|
 * --------------------------------------------------------------------------
 * | In the 802.11 b mode:													|
 * |																		|
 * |	If basicRateSet[0] is set, 1 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[1] is set, 2 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[2] is set, 5.5 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[3] is set, 11 Mbps belongs to the BSSBasicRateSet	|
 * --------------------------------------------------------------------------
 * | In the 802.11g mode:													|
 * |																		|
 * |	If basicRateSet[0] is set, 1 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[1] is set, 2 Mbps belongs to the BSSBasicRateSet	|
 * |	If basicRateSet[2] is set, 5.5 Mbps belongs to the BSSBasicRateSet 	|
 * |	If basicRateSet[3] is set, 11 Mbps belongs to the BSSBasicRateSet	|
 * |																		|
 * | The upper 4 bits, basicRateSet [4:7], are used to provide the Max		|
 * | Basic Rate to hardware.												|
 * --------------------------------------------------------------------------
 */





/*
 * #define WLS_MAC__PROBE_TIMER			_OFFSET_MAC + 0x0020
 *
 * --------------------------------------------------------------------------
 * |	 				  Probe Timer Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | probeDelay				| 0-15	| 00000000 00000000 11111111 11111111	|
 * | 						| 16-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_PROBE_TIMER__PROBE_DELAY				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)





/*
 * #define WLS_MAC__BEACON_INTERVAL		_OFFSET_MAC + 0x0024
 *
 * --------------------------------------------------------------------------
 * |	 				 Beacon Interval Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | beaconInt				| 0-15	| 00000000 00000000 11111111 11111111	|
 * | impTBTTPeriod			| 16-31	| 11111111 11111111 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_BEACON_INTERVAL__INTERVAL				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_BEACON_INTERVAL__IMP_TBTT				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define OFFSET_BEACON_INTERVAL__IMP_TBTT			16








/*
 * #define WLS_MAC__TX_BCN_LENGTH			_OFFSET_MAC + 0x0028
 *
 * --------------------------------------------------------------------------
 * |	 				 Beacon Interval Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | txBcnLength			| 0-15	| 00000000 00000000 11111111 11111111	|
 * | locateCFPCnt			| 16-23	| 00000000 11111111 00000000 00000000	|
 * | 						| 24-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_TX_BCN_LENGTH__TX_BCN_LENGTH				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_TX_BCN_LENGTH__LOCALE_CFP_CNT				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23)

#define OFFSET_TX_BCN_LENGTH__LOCALE_CFP_CNT			16











/*
 * #define WLS_MAC__SCAN_CH_TIMER			_OFFSET_MAC + 0x002C
 * 
 * (When the stateControl.nextState = SCAN)
 *
 * --------------------------------------------------------------------------
 * |	 					 Scan Channel Timer Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | minChanTime			| 0-15	| 00000000 00000000 11111111 11111111	|
 * | incChanTime			| 16-31	| 11111111 11111111 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_SCAN_CH_TIMER__MIN_CH_TIME				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_SCAN_CH_TIMER__INC_CH_TIME				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define OFFSET_SCAN_CH_TIMER__INC_CH_TIME			16




/*
 * #define WLS_MAC__ATIM					_OFFSET_MAC + 0x002C
 * 
 * ( When control.ap = 0 and control.bss is 0, that is the device is a part of 
 * an ad-hoc network, and control.nextState = DOZE/ACTIVE )
 *
 * --------------------------------------------------------------------------
 * |	 						 ATIM Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | atimW					| 0-15	| 00000000 00000000 11111111 11111111	|
 * | 						| 16-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_ATIM__ATIM_W				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)











/*
 * #define WLS_MAC__RTS_RETRY				_OFFSET_MAC + 0x0030
 *
 * --------------------------------------------------------------------------
 * |	 			 RTS Threshold and Retry Limit Register Fields			|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | rtsThreshold			| 0-11	| 00000000 00000000 00001111 11111111	|
 * | 						| 12-15	| Reserved								|
 * | shortRetryLimit		| 16-23	| 00000000 11111111 00000000 00000000	|
 * | longRetryLimit			| 24-31	| 11111111 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_RTS_RETRY__RTS_THRESHOLD			(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11)
#define BITS_RTS_RETRY__SHORT_RETRY				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23)
#define BITS_RTS_RETRY__LONG_RETRY				(BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)


#define OFFSET_RTS_RETRY__SHORT_RETRY				16
#define OFFSET_RTS_RETRY__LONG_RETRY				24






/*
 * #define WLS_MAC__INT_EVT_SET				_OFFSET_MAC + 0x0034
 * #define WLS_MAC__INT_EVT_CLEAR			_OFFSET_MAC + 0x0038
 *
 * --------------------------------------------------------------------------
 * |	 	 	Interrupt Event Register Fields - set and clear				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | frgTxed				| 0		| 00000000 00000000 00000000 00000001	|
 * | frgRxed				| 1		| 00000000 00000000 00000000 00000010	|
 * | txErr					| 2		| 00000000 00000000 00000000 00000100	|
 * | aTIMRxed				| 3		| 00000000 00000000 00000000 00001000	|
 * | dupFiltered			| 4		| 00000000 00000000 00000000 00010000	|
 * | rxErr					| 5		| 00000000 00000000 00000000 00100000	|
 * | impTBTT				| 6		| 00000000 00000000 00000000 01000000	|
 * | tscPnLmtRched			| 7		| 00000000 00000000 00000000 10000000	|
 * | bbRdWr					| 8		| 00000000 00000000 00000001 00000000	|
 * | idleInterrupt			| 9		| 00000000 00000000 00000010 00000000	|
 * | genTimer				| 10	| 00000000 00000000 00000100 00000000	|
 * | atimWOver/cfpEnd		| 11	| 00000000 00000000 00001000 00000000	|
 * | 						| 12-15	| Reserved								|
 * | txFifoUnderrun			| 16	| 00000000 00000001 00000000 00000000	|
 * | rxFifoOverFlow			| 17	| 00000000 00000010 00000000 00000000	|
 * | rxUnicastKeyNotFound	| 18	| 00000000 00000100 00000000 00000000	|
 * | 						| 19-31	| Reserved								|
 * --------------------------------------------------------------------------
 */

#define BITS_INT_EVENT__FRG_TXED				BIT0
#define BITS_INT_EVENT__FRG_RXED				BIT1
#define BITS_INT_EVENT__TX_ERR					BIT2
#define BITS_INT_EVENT__ATIM_RXED				BIT3
#define BITS_INT_EVENT__DUP_FILTERED			BIT4
#define BITS_INT_EVENT__RXERR					BIT5
#define BITS_INT_EVENT__IMP_TBTT				BIT6
#define BITS_INT_EVENT__TSC_PNLMT_RCHED			BIT7
#define BITS_INT_EVENT__BB_RD_WR				BIT8
#define BITS_INT_EVENT__IDLE_INTERRUPT			BIT9
#define BITS_INT_EVENT__GEN_TIMER				BIT10
#define BITS_INT_EVENT__ATIM_WIN_OVER			BIT11
												// Reserved
#define BITS_INT_EVENT__TX_FIFO_UNDERRUN		BIT16
#define BITS_INT_EVENT__RX_FIFO_UNDERRUN		BIT17
#define BITS_INT_EVENT__RX_UNI_KEY_NOTFND		BIT18
												// Reserved





/*
 * #define WLS_MAC__INT_MASK				_OFFSET_MAC + 0x003C
 *
 * --------------------------------------------------------------------------
 * |	 			 		Interrupt Mask Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | Mask bits for Interrupts defined in bits 30:0 in intEvent register		|
 * | 																		|
 * |						| 0-30	| 01111111 11111111 11111111 11111111	|
 * | masterIntEn			| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_INT_MASK__MASTER_INT_EN			BIT31





/*
 * #define WLS_MAC__STATUS					_OFFSET_MAC + 0x0040
 *
 * --------------------------------------------------------------------------
 * |	 			 			Status Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | 						| 0		| Reserved								|
 * | cpFrgTxed 				| 1		| 00000000 00000000 00000000 00000010	|
 * | 						| 2		| Reserved								|
 * | cfpFrgTxed				| 3		| 00000000 00000000 00000000 00001000	|
 * | beaconTxed				| 4		| 00000000 00000000 00000000 00010000	|
 * | priorityFrgTxed		| 5		| 00000000 00000000 00000000 00100000	|
 * | 						| 6		| Reserved								|
 * | cfpOn					| 7		| 00000000 00000000 00000000 10000000	|
 * | privacyICVErr			| 8		| 00000000 00000000 00000001 00000000	|
 * | privacyExcluded		| 9		| 00000000 00000000 00000010 00000000	|
 * | privacyRxUndecryptable	| 10	| 00000000 00000000 00000100 00000000	|
 * | ccmpErr 				| 11	| 00000000 00000000 00001000 00000000	|
 * | phyRxErr				| 12	| 00000000 00000000 00010000 00000000	|
 * | fcsErr					| 13	| 00000000 00000000 00100000 00000000	|
 * | ackRTSFail				| 14	| 00000000 00000000 01000000 00000000	|
 * | undefErr				| 15	| 00000000 00000000 10000000 00000000	|
 * | cfpShortRTLimit		| 16	| 00000000 00000001 00000000 00000000	|
 * | cfpLongRTLimit			| 17	| 00000000 00000010 00000000 00000000	|
 * | cpShortRTLimit			| 18	| 00000000 00000100 00000000 00000000	|
 * | cpLongRTLimit			| 19	| 00000000 00001000 00000000 00000000	|
 * | txStopped				| 20	| 00000000 00010000 00000000 00000000	|
 * | privacyCpTxUnencrypt	| 21	| 00000000 00100000 00000000 00000000	| 
 * | privacyCfpTxUnencrypt	| 22	| 00000000 01000000 00000000 00000000	|
 * | 						| 23-26	| Reserved								|
 * | cpTxFIFOUnderrun		| 27	| 00001000 00000000 00000000 00000000	|
 * | cfpTxFIFOUnderrun		| 28	| 00010000 00000000 00000000 00000000	|
 * | priorityTxFIFOUnderrun	| 29	| 00100000 00000000 00000000 00000000	|
 * | beaconTxFIFOUnderrun	| 30	| 01000000 00000000 00000000 00000000	|
 * | 						| 31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_STATUS__CP_FRG_TXED				BIT1
												//Reserved
#define BITS_STATUS__CFP_FRG_TXED				BIT3
#define BITS_STATUS__BCN_TXED					BIT4
#define BITS_STATUS__PRIO_FRG_TXED				BIT5
												//Reserved
#define BITS_STATUS__CFP_ON						BIT7
#define BITS_STATUS__PRIV_ICV_ERR				BIT8
#define BITS_STATUS__PRIV_EXCLUDED				BIT9
#define BITS_STATUS__PRIV_RX_UNDECRYPT			BIT10
#define BITS_STATUS__CCMP_ERR					BIT11
#define BITS_STATUS__PHY_RX_ERR					BIT12
#define BITS_STATUS__FCS_ERR					BIT13
#define BITS_STATUS__ACK_RTS_FAIL				BIT14
#define BITS_STATUS__UNDEF_ERR					BIT15
#define BITS_STATUS__CFP_SHORT_RT				BIT16
#define BITS_STATUS__CFP_LONG_RT				BIT17
#define BITS_STATUS__CP_SHORT_RT				BIT18
#define BITS_STATUS__CP_LONG_RT					BIT19
#define BITS_STATUS__TX_STOPPED					BIT20
#define BITS_STATUS__CP_TX_UNENCRYPT			BIT21
#define BITS_STATUS__CFP_TX_UNENCRYPT			BIT22
												//Reserved
#define BITS_STATUS__CP_TXFIFO_UDRUN			BIT27
#define BITS_STATUS__CFP_TXFIFO_UDRUN			BIT28
#define BITS_STATUS__PRIO_TXFIFO_UDRUN			BIT29
#define BITS_STATUS__BCN_TXFIFO_UDRUN			BIT30
												//Reserved







/*
 * #define WLS_MAC__CFP					_OFFSET_MAC + 0x004C
 * 
 * --------------------------------------------------------------------------
 * |	 				Transmit FIFO Size Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | dtimPeriod				| 0-7	| 00000000 00000000 00000000 11111111	|
 * | cfpPeriod				| 8-15	| 00000000 00000000 11111111 00000000	|
 * | cfpMaxDuration			| 16-31	| 11111111 11111111 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_CFP__DTIM_PERIOD				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)
#define BITS_CFP__CFP_PERIOD				(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_CFP__CFP_MAX_DURATION		(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)


#define OFFSET_CFP__CFP_PERIOD				8
#define OFFSET_CFP__CFP_MAX_DURATION		16









/*
 * #define WLS_MAC__TX_FIFO_SIZE			_OFFSET_MAC + 0x0050
 * 
 * --------------------------------------------------------------------------
 * |	 				Transmit FIFO Size Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | cpTxFIFOSize			| 0-7	| 00000000 00000000 00000000 11111111	|
 * | cfpTxFIFOSize			| 8-15	| 00000000 00000000 11111111 00000000	|
 * | 						| 16-21	| Reserved								|
 * | forceRxFragCntToOne	| 22	| 00000000 01000000 00000000 00000000	|
 * | bcnTxFrgCntAutoIncr	| 23	| 00000000 10000000 00000000 00000000	|
 * | bcnTxFIFOSize			| 24-31	| 11111111 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 *
 * cpTxFIFOSize(Contention Period Transmit FIFO Size) :
 * ---------------------------------------------
 * This field is programmed with the size of the CP Transmit FIFO 
 * partition in multiples of 16 quadlets (that is in multiples of 64 bytes).
 * Default = 0d192 = 0xC0.
 *
 */
#define BITS_TX_FIFO_SIZE__CP_TXFIFO_SIZE				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)
#define BITS_TX_FIFO_SIZE__CFP_TXFIFO_SIZE			(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_TX_FIFO_SIZE__FOURCE_RX_FRAG_CNT		BIT22
#define BITS_TX_FIFO_SIZE__BCN_TX_FRAG_CNT			BIT23
#define BITS_TX_FIFO_SIZE__BCN_TXFIFO_SIZE			(BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define OFFSET_TX_FIFO_SIZE__CFP_TXFIFO_SIZE			8
#define OFFSET_TX_FIFO_SIZE__FOURCE_RX_FRAG_CNT		22
#define OFFSET_TX_FIFO_SIZE__BCN_TX_FRAG_CNT		23
#define OFFSET_TX_FIFO_SIZE__BCN_TXFIFO_SIZE			24






/*
 * #define WLS_MAC__FIFO_SELECT			_OFFSET_MAC + 0x0058
 * 
 * --------------------------------------------------------------------------
 * |	 					FIFO Select Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | txFIFOSel				| 0-1	| 00000000 00000000 00000000 00000011	|
 * | 						| 2-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_FIFO_SELECT__TX_TXFIFO_SELECT			(BIT0|BIT1)








/*
 * #define WLS_MAC__TIMINGS_1REG			_OFFSET_MAC + 0x0060
 * 
 * --------------------------------------------------------------------------
 * |	 					Timings Register 1 Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | bbClkFreq				| 0-6	| 00000000 00000000 00000000 01111111	|
 * | txDelaySlot			| 7-10	| 00000000 00000000 00000111 10000000	|
 * | txDelaySIFS			| 11-14	| 00000000 00000000 01111000 00000000	|
 * | rxDelayB				| 15-18	| 00000000 00000111 10000000 00000000	|
 * | rxDelayA				| 19-22	| 00000000 01111000 00000000 00000000	|
 * | macProcDelay			| 23-25	| 00000011 10000000 00000000 00000000	|
 * | 						| 26-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_TIMINGS_1REG__BB_CLK_FREQ				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6)
#define BITS_TIMINGS_1REG__TX_DELAY_SLOT				(BIT7|BIT8|BIT9|BIT10)
#define BITS_TIMINGS_1REG__TX_DELAY_SIFS				(BIT11|BIT12|BIT13|BIT14)
#define BITS_TIMINGS_1REG__RX_DELAY_B				(BIT15|BIT16|BIT17|BIT18)
#define BITS_TIMINGS_1REG__RX_DELAY_A				(BIT19|BIT20|BIT21|BIT22)
#define BITS_TIMINGS_1REG__MAC_PROC_DELAY			(BIT23|BIT24|BIT25)



#define OFFSET_TIMINGS_1REG__TX_DELAY_SLOT			7
#define OFFSET_TIMINGS_1REG__TX_DELAY_SIFS			11
#define OFFSET_TIMINGS_1REG__RX_DELAY_B				15
#define OFFSET_TIMINGS_1REG__RX_DELAY_A				19
#define OFFSET_TIMINGS_1REG__MAC_PROC_DELAY			23
#define OFFSET_TIMINGS_1REG__RX_DELAY_US				26

#define REG_TIMINGS_1REG(bbClkFreq, txDelaySlot, txDelaySIFS, rxDelayB, rxDelayA, macProcDelay) \
		(((bbClkFreq) & (BITS_TIMINGS_1REG__BB_CLK_FREQ)) \
		 | (((txDelaySlot) << OFFSET_TIMINGS_1REG__TX_DELAY_SLOT) & (BITS_TIMINGS_1REG__TX_DELAY_SLOT)) \
		 | (((txDelaySIFS) << OFFSET_TIMINGS_1REG__TX_DELAY_SIFS) & (BITS_TIMINGS_1REG__TX_DELAY_SIFS)) \
		 | (((rxDelayB) << OFFSET_TIMINGS_1REG__RX_DELAY_B) & (BITS_TIMINGS_1REG__RX_DELAY_B)) \
		 | (((rxDelayA) << OFFSET_TIMINGS_1REG__RX_DELAY_A) & (BITS_TIMINGS_1REG__RX_DELAY_A)) \
		 | (((macProcDelay) << OFFSET_TIMINGS_1REG__MAC_PROC_DELAY ) & (BITS_TIMINGS_1REG__MAC_PROC_DELAY)))







/*
 * #define WLS_MAC__TIMINGS_2REG			_OFFSET_MAC + 0x0064
 * 
 * --------------------------------------------------------------------------
 * |	 					Timings Register 2 Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | tCCAa					| 0-11	| 00000000 00000000 00001111 11111111	|
 * | tCCAb					| 12-23	| 00000000 11111111 11110000 10000000	|
 * | slotTime				| 24-28	| 00011111 00000000 10000000 00000000	|
 * | 						| 26-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_TIMINGS_2REG__T_CCA_A			(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11)
#define BITS_TIMINGS_2REG__T_CCA_B			(BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23)
#define BITS_TIMINGS_2REG__SLOT_TIME			(BIT24|BIT25|BIT26|BIT27|BIT28)



#define OFFSET_TIMINGS_2REG__T_CCA_B		12
#define OFFSET_TIMINGS_2REG__SLOT_TIME		24

#define REG_TIMINGS_2REG(tCCAa, tCCAb, slotTime) \
		(((tCCAa) & (BITS_TIMINGS_2REG__T_CCA_A)) \
		 | (((tCCAb) << OFFSET_TIMINGS_2REG__T_CCA_B) & (BITS_TIMINGS_2REG__T_CCA_B)) \
		 | (((slotTime) << OFFSET_TIMINGS_2REG__SLOT_TIME ) & (BITS_TIMINGS_2REG__SLOT_TIME)))













/*
 * #define WLS_MAC__GEN_TIMER				_OFFSET_MAC + 0x0074
 * 
 * --------------------------------------------------------------------------
 * |	 			General Purpose Timer Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | genTimer				| 0-15	| 00000000 00000000 11111111 11111111	|
 * | freeRunTimer[15:0]		| 16-31	| 11111111 11111111 00000000 10000000	|
 * --------------------------------------------------------------------------
 */

#define BITS_GEN_TIMER__GEN					(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_GEN_TIMER__FREE_RUN				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define OFFSET_GEN_TIMER__FREE_RUN			16









/*
 * #define WLS_MAC__RADIO_ACCESS			_OFFSET_MAC + 0x007C
 * 
 * --------------------------------------------------------------------------
 * |	 				Radio Access Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | radioData				| 0-19	| 00000000 00001111 11111111 11111111	|
 * | wrNotDone				| 20	| 00000000 00010000 00000000 10000000	|
 * | ifCS					| 21	| 00000000 00100000 00000000 00000000	|
 * | 						| 22-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_RADIO_ACCESS__RADIO_DATA			(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19)
#define BITS_RADIO_ACCESS__WR_NOTDONE			BIT20
#define BITS_RADIO_ACCESS__IFCS					BIT21








/*
 * #define WLS_MAC__BRS11G_OFDM			_OFFSET_MAC + 0x008C
 * 
 * --------------------------------------------------------------------------
 * |	 				Radio Access Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | brs11gOFDM				| 0-7	| 00000000 00001111 11111111 11111111	|
 * | 						| 8-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_BRS11G_OFDM__BRS11GOFDM			(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)






/*
 * #define WLS_MAC__KEY_WORD_0				_OFFSET_MAC + 0x0090
 * 
 * --------------------------------------------------------------------------
 * |	 					Key Word 0 Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | privacyKey[31:0]		| 0-31	| 11111111 11111111 11111111 11111111	|
 * --------------------------------------------------------------------------
 *
 *
 * #define WLS_MAC__KEY_WORD_1				_OFFSET_MAC + 0x0094
 * 
 * --------------------------------------------------------------------------
 * |	 					Key Word 1 Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | key[63:32]				| 0-31	| 11111111 11111111 11111111 11111111	|
 * --------------------------------------------------------------------------
 *
 *
 * #define WLS_MAC__KEY_WORD_2				_OFFSET_MAC + 0x0098
 * 
 * --------------------------------------------------------------------------
 * |	 					Key Word 2 Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | key[95:64]				| 0-31	| 11111111 11111111 11111111 11111111	|
 * --------------------------------------------------------------------------
 *
 *
 * #define WLS_MAC__KEY_WORD_3				_OFFSET_MAC + 0x009C
 * 
 * --------------------------------------------------------------------------
 * |	 					Key Word 3 Register Fields						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | key[127:96]			| 0-31	| 11111111 11111111 11111111 11111111	|
 * --------------------------------------------------------------------------
 */
 






/*
 * #define WLS_MAC__KEY_MAC_ADDR			_OFFSET_MAC + 0x00A0
 * 
 * --------------------------------------------------------------------------
 * |	 			Privacy Key MAC Address Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | keyMACAddr[31:0]		| 0-31	| 11111111 11111111 11111111 11111111	|
 * --------------------------------------------------------------------------
 */



/*
 * #define WLS_MAC__KEY_CNTL_ADDR			_OFFSET_MAC + 0x00A4
 * 
 * --------------------------------------------------------------------------
 * |	 			Key Control and Address Register Fields					|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | keyMacAddr[47:32]		| 0-15	| 00000000 00000000 11111111 11111111	|
 * | keyValid				| 16	| 00000000 00000001 00000000 10000000	|
 * | cipherType				| 17-18	| 00000000 00000110 00000000 00000000	|
 * | keyID					| 19-20	| 00000000 00011000 00000000 00000000	|
 * | keyIndex				| 21-30	| 01111111 11100000 00000000 00000000	|
 * | xcastKey				| 30	| 01000000 00000000 00000000 00000000	|
 * | entryNotDone			| 31	| 10000000 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 */
#define BITS_KEY_CNTL_ADDR__MAC_ADDR					(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_KEY_CNTL_ADDR__KEY_VALID				BIT16
#define BITS_KEY_CNTL_ADDR__CIPHER_TYPE				(BIT17|BIT18)
#define BITS_KEY_CNTL_ADDR__KEY_ID					(BIT19|BIT20)
#define BITS_KEY_CNTL_ADDR__KEY_INDEX				(BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30)
#define BITS_KEY_CNTL_ADDR__XCAST_KEY				BIT30
#define BITS_KEY_CNTL_ADDR__ENTRY_NOT_DONE			BIT31

#define OFFSET_KEY_CNTL_ADDR__MAC_ADDR				0
#define OFFSET_KEY_CNTL_ADDR__KEY_BALID				16
#define OFFSET_KEY_CNTL_ADDR__CIPHER_TYPE			17
#define OFFSET_KEY_CNTL_ADDR__KEY_ID					19
#define OFFSET_KEY_CNTL_ADDR__KEY_INDEX				21
#define OFFSET_KEY_CNTL_ADDR__XCAST_KEY				30
#define OFFSET_KEY_CNTL_ADDR__ENTRY_NOT_DONE		31









/*
 * #define WLS_MAC__KEY_SIZE_CTRL			_OFFSET_MAC + 0x00A8
 * 
 * --------------------------------------------------------------------------
 * |	 							 Key Size								|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | keySize				| 0-1	| 00000000 00000000 00000000 00000011	|
 * | 						| 8-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_KEY_SIZE_CTRL__KEYSIZE					(BIT0|BIT1)



/*
 *	For the case of keyCntlAddr.cipherType == WEP, 
 *	00: WEP is not supported.
 *	01: 40-bit secret key is used for encryption.
 *	10: 104-bit secret key is used for encryption.
 *	11: 128-bit secret key is used for encryption.
 *
 *	The size of WEP Seed is keyCntl.keySize + 24.
 *	For the case of keyCntlAddr.cipherType ==  TKIP or CCMP, this field 
 *	needs to be set to 2'b11; any other value will cause undefined behavior.
 */
#define WEP_KEY_SIZE__NOT_SUPPORT			0	// 00
#define WEP_KEY_SIZE__WEP40					1	// 01
#define WEP_KEY_SIZE__WEP104				2	// 10
#define WEP_KEY_SIZE__WEP128				3	// 11



//17,18bit of Key Control and Address Register
#define KEY_CONTROL_CIPHER_TYPE_WEP			0   //WEP
#define KEY_CONTROL_CIPHER_TYPE_TKIP			1   //TKIP
#define KEY_CONTROL_CIPHER_TYPE_CCMP			2   //ccmp

//Justin: 0717.    prepare for OID_802_11_RELOAD_DEFAULTS
// these #define should really be in some more appropriate place.
#define  SME_MAX_KEY_LEN_IN_HW 16 
#define  MIB_MACADDR_SPECIFIC_KEY 4
#define  REGISTRY_DEFAULT_WEP_KEY0 "123456789abcdef0" // may be in registry
#define  REGISTRY_DEFAULT_WEP_KEY1 "0fedcba987654321"
#define  REGISTRY_DEFAULT_WEP_KEY2 "0102030405060708"
#define  REGISTRY_DEFAULT_WEP_KEY3 "1020304050607080"


/*
 * #define WLS_MAC__TX_FRAG_CNT			_OFFSET_MAC + 0x00B0
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Transmit Fragment Count Register Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | cpTxFrgCnt				| 0-7	| 00000000 00000000 00000000 11111111	|
 * | cfpTxFrgCnt			| 8-15	| 00000000 00000000 11111111 00000000	|
 * | priorityTxFrgCnt		| 16-23	| 00000000 11111111 00000000 00000000	|
 * | beaconTxFrgCnt			| 24	| 00000001 00000000 00000000 00000000	|
 * | 						| 25-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_TX_FRAG_CNT__CP_TXFRG_CNT				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)
#define BITS_TX_FRAG_CNT__CFP_TXFRG_CNT				(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_TX_FRAG_CNT__PRIO_TXFRG_CNT				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23)
#define BITS_TX_FRAG_CNT__BCN_TXFRG_CNT				BIT24

#define OFFSET_TX_FRAG_CNT__CP_TXFRG_CNT			0
#define OFFSET_TX_FRAG_CNT__CFP_TXFRG_CNT			8
#define OFFSET_TX_FRAG_CNT__PRIO_TXFRG_CNT			16
#define OFFSET_TX_FRAG_CNT__BCN_TXFRG_CNT			24








/*
 * #define WLS_MAC__RX_FRAG_CNT			_OFFSET_MAC + 0x00B4
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Transmit Fragment Count Register Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | rxFrgCount				| 0-7	| 00000000 00000000 00000000 11111111	|
 * | 						| 8-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_RX_FRAG_CNT__RX_FRG_CNT				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)






/*
 * #define WLS_MAC__CP_BO_CNT				_OFFSET_MAC + 0x00C8
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Contention Period Back-off Count Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | swCPBOCnt				| 0-9	| 00000000 00000000 00000011 11111111	|
 * | aifsn					| 10-17	| 00000000 00000011 11111100 00000000	|
 * | 						| 18	| Reserved								|
 * | bOCnt					| 19-28	| 00011111 11111000 00000000 00000000	|
 * | 						| 29-31	| Reserved								|
 * --------------------------------------------------------------------------
 *
 * aifsn :
 * Arbitration Inter Frame Spacing Number.  This field is programmed with 
 * the number of slot times after SIFS that hardware should wait before 
 * starting backoff.
 */
#define BITS_CP_BO_CNT__SW_CPBO_CNT				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9)
#define BITS_CP_BO_CNT__AIFSN						(BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17)
												// Reserved
#define BITS_CP_BO_CNT__BO_CNT					(BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28)
												// Reserved



/*
 * #define WLS_MAC__DEBUG0_WD                   _OFFSET_MAC + 0x00D0
*/
#define MAC_CLR_DEBUG_MODE				0
#define MAC_DEBUG_BCNBACKOFF_WREN		BIT_1
#define MAC_DEBUG_PRIBACKOFF_WREN		BIT_2
#define MAC_DEBUG_CPBACKOFF_WREN		BIT_3
#define MAC_DEBUG_FIFO_EN_BIT			 	BIT_8
#define MAC_DEBUG_RXFRGCNT_INC_BIT		BIT_9
#define MAC_DEBUG_TXFRGCNT_DEC_BIT		BIT_10
#define MAC_DEBUG_PERFCNT_EN_BIT		 	BIT_11



/*
 * #define WLS_MAC__DEBUG2				_OFFSET_MAC + 0x00D8
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Contention Period Back-off Count Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | rxControlCs			| 0-6   | 00000000 00000000 00000000 01111111	|
 * | 		 				| 7-8   | Reserved								|
 * | transmitControllerCs	| 9-14 	| 00000000 00000000 01111110 00000000	|
 * | 						| 15-16	| Reserved								|
 * | scanCs					| 17-21	| 00000000 00111110 00000000 00000000	|
 * | reserved               | 22-24	| Reserved								|
 * | joinActiveCs	        | 25-30	| 01111110 00000000 00000000 00000000	|
 * | reserved    			| 31    | Reserved                             	|
 * --------------------------------------------------------------------------
 *
 * aifsn :
 * Arbitration Inter Frame Spacing Number.  This field is programmed with 
 * the number of slot times after SIFS that hardware should wait before 
 * starting backoff.
 */
#define BITS_DEBUG2__TX_CONTROLLER_CS			(BIT9|BIT10|BIT11|BIT12|BIT13|BIT14)
#define OFFSET_DEBUG2__TX_CONTROLLER_CS			9

#define TX_CONTROLLER_CS_WORK_STATE				0







/*
 * #define WLS_MAC__PERF_COUNT0			_OFFSET_MAC + 0x00F0
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Performance Counter Register 0 Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | icvErrCnt				| 0-5	| 00000000 00000000 00000000 00111111	|
 * | 						| 6-7	| Reserved								|
 * | privacyExclCnt			| 8-13	| 00000000 00000000 00111111 00000000	|
 * | 						| 14-15	| Reserved								|
 * | ackFailCnt				| 16-21	| 00000000 00111111 00000000 00000000	|
 * | 						| 12-23	| Reserved								|
 * | frmDupCnt				| 24-29	| 00111111 00000000 00000000 00000000	|
 * | 						| 30-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_PERF_COUNT0__ICV_ERR_CNT				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5)
#define BITS_PERF_COUNT0__EXCLUDED_CNT				(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13)
#define BITS_PERF_COUNT0__ACK_FAIL_CNT				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21)
#define BITS_PERF_COUNT0__FRM_DUP_CNT				(BIT24|BIT25|BIT26|BIT27|BIT28|BIT29)


#define OFFSET_PERF_COUNT0__EXCLUDED_CNT			8
#define OFFSET_PERF_COUNT0__ACK_FAIL_CNT			16
#define OFFSET_PERF_COUNT0__FRM_DUP_CNT				24




/*
 * #define WLS_MAC__PERF_COUNT1			_OFFSET_MAC + 0x00F4
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Performance Counter Register 1 Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |rtsSuccCnt				| 0-5	| 00000000 00000000 00000000 00111111	|
 * | 						| 6-7	| Reserved								|
 * |rtsFailCnt				| 8-13	| 00000000 00000000 00111111 00000000	|
 * | 						| 14-15	| Reserved								|
 * |fcsErrCnt				| 16-21	| 00000000 00111111 00000000 00000000	|
 * | 						| 12-23	| Reserved								|
 * |privacyundecryptableCnt	| 24-29	| 00111111 00000000 00000000 00000000	|
 * | 						| 30-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_PERF_COUNT1__RTS_SUCC_CNT				(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5)
#define BITS_PERF_COUNT1__RTS_FAIL_CNT				(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13)
#define BITS_PERF_COUNT1__FCS_ERR_CNT				(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21)
#define BITS_PERF_COUNT1__UNDECRYPTABLE_CNT			(BIT24|BIT25|BIT26|BIT27|BIT28|BIT29)


#define OFFSET_PERF_COUNT1__RTS_FAIL_CNT				8
#define OFFSET_PERF_COUNT1__FCS_ERR_CNT					16
#define OFFSET_PERF_COUNT1__UNDECRYPTABLE_CNT			24






// TODO:		we need the define of  Performance Counter Register 2 Fields
/*
 * #define WLS_MAC__PERF_COUNT2			_OFFSET_MAC + 0x00F8
 * 
 * --------------------------------------------------------------------------
 * |	 			 	Performance Counter Register 2 Fields				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * |						| 0-5	| 00000000 00000000 00000000 00000000	|
 * | 						| 6-7	| Reserved								|
 * |						| 8-13	| 00000000 00000000 00000000 00000000	|
 * | 						| 14-15	| Reserved								|
 * |						| 16-21	| 00000000 00000000 00000000 00000000	|
 * | 						| 12-23	| Reserved								|
 * |						| 24-29	| 00000000 00000000 00000000 00000000	|
 * | 						| 30-31	| Reserved								|
 * --------------------------------------------------------------------------
 */
#define BITS_PERF_COUNT2__RETRY_CNT					(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7)
#define BITS_PERF_COUNT2__FAILED_CNT					(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_PERF_COUNT2__MULTIPLE_RETRY_CNT		(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23)


#define OFFSET_PERF_COUNT2__FAILED_CNT				8
#define OFFSET_PERF_COUNT2__MULTIPLE_RETRY_CNT		16


/*
 *#define WLS_MAC__EDCAPARAM0_WD            _OFFSET_MAC + 0x010C
*
*/
//woody 080429
// For V4 chip, the 'setCWMIN15' bit in Mac Control reg has no use; cwMin has to be set
	// thru the EDCAParam0~3 registers.
	// By default (i.e. hw reset) EDCAParam0 is selected by MacHw, so here we only program
	// EDCAParam0.
	//
	//   if (_x == 1)
	//      cwMin = 15, cwMax = 1023
	//   else
	//      cwMin = 31, cwMax = 1023
	//
	// If K is the value programmed for CWMin(CWMax), the actual value 
	// used by MacHw is in fact, 
	//                  (K+1)
	// CWMin(CWMax) =  2      - 1
	//
//	WriteToRegNL( pNic, WLS_MAC_EDCAPARAM0_WD, (((4-_x)<<16) | (9<<20)));
//	WriteToRegNL( pNic, WLS_MAC_EDCAPARAM0_WD, (((4-_x)<<16) | ((4-_x)<<20)));
//This change can make 3dsp usb wlan has a good throughput when co-work with other wireless sta
#define  EDCAPARAMETER0_FOR_11B		0x00440000
#define  EDCAPARAMETER0_FOR_11G		0x00330000





/*
 * #define WLS_MAC__BBREG_16_19			_OFFSET_MAC + 0x0210
 * 
 * --------------------------------------------------------------------------
 * |	 			 BBReg16-BBReg19 Fields for OFDM and DSS				|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | BBReg16				| 0-7	| 00000000 00000000 00000000 11111111	|
 * | BBReg17				| 8-15	| 00000000 00000000 11111111 00000000	|
 * | BBReg18				| 16-23	| 00000000 11111111 00000000 00000000	|
 * | BBReg19				| 24-31	| 11111111 00000000 00000000 00000000	|
 * --------------------------------------------------------------------------
 *
 * This is a general-purpose mailbox from UniPHY to the host processor.  
 * It connects to the UniPHY MAC_to_DSP Mailbox Register 1.
 */





/*
 * #define WLS_MAC__BBREG_24_27			_OFFSET_MAC + 0x0218
 * 
 * --------------------------------------------------------------------------
 * |	 			 	BBReg24 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | SR						| 0		| xxxxxxxx xxxxxxxx xxxxxxxx 00000001	|
 * | ER						| 1		| xxxxxxxx xxxxxxxx xxxxxxxx 00000010	|
 * | ST						| 2		| xxxxxxxx xxxxxxxx xxxxxxxx 00000100	|
 * | ET						| 3		| xxxxxxxx xxxxxxxx xxxxxxxx 00001000	|
 * | DZC					| 4		| xxxxxxxx xxxxxxxx xxxxxxxx 00010000	|
 * | DZS					| 5		| xxxxxxxx xxxxxxxx xxxxxxxx 00100000	|
 * | 						| 6-7	| Reserved								|
 * --------------------------------------------------------------------------
 * |	 			 		BBReg25 Fields for OFDM 						|
 * --------------------------------------------------------------------------
 * | General Purpose		| 0-7	| xxxxxxxx xxxxxxxx 11111111 xxxxxxxx	|
 * --------------------------------------------------------------------------
 * |	 			 		BBReg25 Fields for DSS							|
 * --------------------------------------------------------------------------
 * | RSSI					| 0-7	| xxxxxxxx xxxxxxxx 11111111 xxxxxxxx	|
 * -------------------------------------------------------------------------- 
 * |	 			 	BBReg26 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO CFP SEG		| 0-7	| xxxxxxxx 11111111 xxxxxxxx xxxxxxxx	|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg27 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO CFP SEG		| 0-4	| 00011111 xxxxxxxx xxxxxxxx xxxxxxxx	|
 * | 						| 5-6	| Reserved								|
 * | V						| 7		| 10000000 xxxxxxxx xxxxxxxx xxxxxxxx	|
 * --------------------------------------------------------------------------
 *
 * TX FIFO Contention Free Period Segment's available entries in words.
 */
#define BITS_BBREG_24_27__SR						BIT0
#define BITS_BBREG_24_27__ER						BIT1
#define BITS_BBREG_24_27__ST						BIT2
#define BITS_BBREG_24_27__ET						BIT3
#define BITS_BBREG_24_27__DZC						BIT4
#define BITS_BBREG_24_27__DZS						BIT5
													// Reserved
#define BITS_BBREG_24_27__GEN_PURPOSE			(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_BBREG_24_27__RSSI						(BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
#define BITS_BBREG_24_27__TXFIFO_CFP_SEG			(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28)
													// Reserved
#define BITS_BBREG_24_27__TXFIFO_CFP_V			BIT31




#define OFFSET_BBREG_24_27__TXFIFO_CFP_SEG			16
#define OFFSET_BBREG_24_27__TXFIFO_CFP_V			31








/*
 * #define WLS_MAC__BBREG_28_31			_OFFSET_MAC + 0x021C
 * 
 * --------------------------------------------------------------------------
 * |	 			 	BBReg28 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | TX FIFO CP SEG			| 0-7	| xxxxxxxx xxxxxxxx xxxxxxxx 11111111	|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg29 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO CP SEG			| 0-4	| xxxxxxxx xxxxxxxx 00011111 xxxxxxxx	|
 * | 						| 5-6	| Reserved								|
 * | V						| 7		| xxxxxxxx xxxxxxxx 10000000 xxxxxxxx	|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg30 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO PRIOR SEG		| 0-7	| xxxxxxxx 11111111 xxxxxxxx xxxxxxxx	|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg31 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO PRIOR SEG		| 0-4	| 00011111 xxxxxxxx xxxxxxxx xxxxxxxx	|
 * | 						| 5-6	| Reserved								|
 * | V						| 7		| 10000000 xxxxxxxx xxxxxxxx xxxxxxxx	|
 * --------------------------------------------------------------------------
 *
 * TX FIFO Contention Period Segment's available entries in words.
 * TX FIFO Priority Segment's available entries in words.
 */
#define BITS_BBREG_28_31__TXFIFO_CP_SEG			(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12)
													// Reserved
#define BITS_BBREG_28_31__TXFIFO_CP_V				BIT15
#define BITS_BBREG_28_31__TXFIFO_PRIOR_SEG		(BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28)
													// Reserved
#define BITS_BBREG_28_31__TXFIFO_PRIOR_V			BIT31




#define OFFSET_BBREG_28_31__TXFIFO_CP_V				15
#define OFFSET_BBREG_28_31__TXFIFO_PRIOR_SEG		16
#define OFFSET_BBREG_28_31__TXFIFO_PRIOR_V			31






/*
 * #define WLS_MAC__BBREG_36_38			_OFFSET_MAC + 0x0224
 * 
 * --------------------------------------------------------------------------
 * |	 			 	BBReg36 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | Field Name				| Bit	| value									|
 * --------------------------------------------------------------------------
 * | DSP to MAC				| 		| 										|
 * | General Purpose 1 MMCR	| 0-5	| xxxxxxxx xxxxxxxx xxxxxxxx xx111111	|
 * | 						| 6-7	| Reserved								|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg37 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | TX FIFO SEL			| 0-1	| xxxxxxxx xxxxxxxx 00000011 xxxxxxxx	|
 * | SEL_EN					| 2		| xxxxxxxx xxxxxxxx 00000100 xxxxxxxx	|
 * | Increment				| 3		| xxxxxxxx xxxxxxxx 00001000 xxxxxxxx	|
 * | Flush					| 4		| xxxxxxxx xxxxxxxx 00010000 xxxxxxxx	|
 * | 						| 5-7	| Reserved								|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg38 Fields for OFDM and DSS						|
 * --------------------------------------------------------------------------
 * | BB_INT					| 0		| xxxxxxxx 00000001 xxxxxxxx xxxxxxxx	|
 * | GEN_INT_1				| 1		| xxxxxxxx 00000010 xxxxxxxx xxxxxxxx	|
 * | GEN_INT_2				| 2		| xxxxxxxx 00000100 xxxxxxxx xxxxxxxx	|
 * | 						| 3-7	| Reserved								|
 * --------------------------------------------------------------------------
 * |	 			 	BBReg39 Fields (blank								|
 * --------------------------------------------------------------------------
 * |																		|
 * --------------------------------------------------------------------------
 *
 * 1.Each PCI write of '1' to the bit/bits in BBReg38 triggers corresponding 
 *   UniPHY interrupts as shown below. UniPHY does not need to clear the  
 *   bits in BBReg38. Reads from this register return zero.
 * 2.Corresponding interrupts are as follows:
 * 	 ------------------------------------
 * 	 | Field 		| SP-20 Interrupt	|
 * 	 ------------------------------------
 * 	 | BB_INT 		| Interrupt_vec[14]	|
 * 	 | GEN_INT_1 	| Interrupt_vec[27]	|
 * 	 | GEN_INT_2 	| Interrupt_vec[28]	|
 * 	 ------------------------------------
 */
#define BITS_BBREG_36_38__GEN_PURPOSE_1_MMCR	(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5)
													// Reserved
#define BITS_BBREG_36_38__TX_FIFO_SEL				(BIT8|BIT9)
#define BITS_BBREG_36_38__SEL_EN					BIT10
#define BITS_BBREG_36_38__INCREMENT				BIT11
#define BITS_BBREG_36_38__FLUSH					BIT12
													// Reserved
#define BITS_BBREG_36_38__BB_INT					BIT16
#define BITS_BBREG_36_38__GEN_INT_1				BIT17
#define BITS_BBREG_36_38__GEN_INT_2				BIT18
													// Reserved















/*
 * /------------------------------------------------------------------------\
 * |																		|
 * |					Receive  Frame  Format								|
 * |																		|
 * \------------------------------------------------------------------------/
 *
 *
 * 
 * --------------------------------------------------------------------------
 * |	 			 				offset Hi								| 
 * --------------------------------------------------------------------------
 * |	 			 				offset Lo								| 
 * --------------------------------------------------------------------------
 * | 				Reserved		 					|		RSSI		|
 * --------------------------------------------------------------------------
 * |	 			 			rxMACFrameBody (cntd.						|
 * --------------------------------------------------------------------------
 * |	 			 			ExtendedIV(if present)						|
 * --------------------------------------------------------------------------
 * |	 			 			IV/KeyID(if present)						|
 * --------------------------------------------------------------------------
 * | 	rxPaddingData(if present)	| 		rxAddress4 Hi (if present)		|
 * --------------------------------------------------------------------------
 * |	 			 	rxAddress4 Lo (if present)							|
 * --------------------------------------------------------------------------
 * | 		rxSequenceControl		| 			rxAddress3 Hi				|
 * --------------------------------------------------------------------------
 * |	 			 			rxAddress3 Lo								| 
 * --------------------------------------------------------------------------
 * |	 			 			rxAddress2 Hi								|
 * --------------------------------------------------------------------------
 * | 		rxAddress2 Lo			| 			rxAddress1 Hi				|
 * --------------------------------------------------------------------------
 * |	 			 			rxAddress1 Lo								|
 * --------------------------------------------------------------------------
 * |	 	rxDuration/ID			|			rxFrameControl				|
 * --------------------------------------------------------------------------
 * |	rxLength	 	|  rxSpeed	|				Reserved				|
 * --------------------------------------------------------------------------
 * 31				  20 19		  16 15										0
 *
 *
 * rxSpeed:
 * --------
 * This field indicates the rate at which the following fragment is received 
 * by the core.  The encoding is as follows:
 *     RxSpeed =4'b0000 = 1 Mb/s
 *     RxSpeed =4'b0001 = 2 Mb/s
 *     RxSpeed =4'b0010 = 5.5 Mb/s
 *     RxSpeed =4'b0011 = 11 Mb/s
 *     RxSpeed =4'b0100 = 6 Mb/s
 *     RxSpeed =4'b0101 = 9 Mb/s
 *     RxSpeed =4'b0110 = 12 Mb/s
 *     RxSpeed =4'b0111 = 18 Mb/s
 *     RxSpeed =4'b1000 = 24 Mb/s
 *     RxSpeed =4'b1001 = 36 Mb/s
 *     RxSpeed =4'b1010 = 48 Mb/s
 *     RxSpeed =4'b1011 = 54 Mb/s
 * 
 * rxLength:
 * ---------
 * This field indicates the length in bytes of the received fragment on Air,
 * that is this is the length directly received from Baseband. 
 * 
 * 
 * 
 * 
 * 
 *  
 */

/*--------------------------------------------------------------------------|
 |				Receive  Frame  Format	(First Word)						|
 |--------------------------------------------------------------------------*/
													// Reserved
#define BITS_RX_FRM__RX_SPEED						(BIT16|BIT17|BIT18|BIT19)
#define BITS_RX_FRM__RX_LENGTH					(BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define OFFSET_RX_FRM__RX_SPEED					16
#define OFFSET_RX_FRM__RX_LENGTH					20


/*--------------------------------------------------------------------------|
 |				Receive  Frame  Format	(Second Word						|
 |--------------------------------------------------------------------------*/
























#define ALL_BITS	(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)



//define base address to 8051 fw downloaded to
#define DOWNLOAD_8051_FW_FIELD_OFFSET		0x0      //define it later
#define DOWNLOAD_DSP_FW_FIELD_OFFSET		0x2400      //define it later

//vendor command length 24 bytes, 1bytes for cmd type, 23 bytes for cmd data
#define VENDOR_CMD_HEAD_BYTES			1
#define VENDOR_CMD_DATA_BYTES			23
#define VENDOR_CMD_LENGTH_BYTES			24




//add by justin. refer to spec.-- design specification ver0.2
//#define OFFSET_USB_CTL_REG1					0x00
//#define OFFSET_USB_CTL_REG2					0x04
//#define OFFSET_USB_INT_REG1					0x05
//#define OFFSET_USB_INT_REG2					0x09
//#define OFFSET_USB_MEM_REG					0x0B
//#define OFFSET_USB_ID_REG					0x0C
//#define OFFSET_USB_INFO_REG					0x14

//#define OFFSET_BULK_IN_CTL_REG_1			0x20        //ENDPOINT1
//#define OFFSET_BULK_IN_BASE_ADDR_REG_1		0x24
//#define OFFSET_BULK_IN_LEN_REG_1			0x28
//#define OFFSET_BULK_IN_REAL_LEN_REG_1		0x2C
//#define OFFSET_BULK_OUT_CTL_REG_2			0x30        //ENDPOINT2
//#define OFFSET_BULK_OUT_BASE_ADDR_REG_2		0x34
//#define OFFSET_BULK_OUT_LEN_REG_2			0x38
//#define OFFSET_BULK_OUT_REAL_LEN_REG_2		0x3C
//#define OFFSET_BULK_IN_CTL_REG_3			0x40        //ENDPOINT3
//#define OFFSET_BULK_IN_BASE_ADDR_REG_3		0x44
//#define OFFSET_BULK_IN_LEN_REG_3			0x48
//#define OFFSET_BULK_IN_REAL_LEN_REG_3		0x4C
//#define OFFSET_BULK_OUT_CTL_REG_4			0x50        //ENDPOINT4
//#define OFFSET_BULK_OUT_BASE_ADDR_REG_4		0x54
//#define OFFSET_BULK_OUT_LEN_REG_4			0x58
//#define OFFSET_BULK_OUT_REAL_LEN_REG_4		0x5C
//#define OFFSET_BULK_IN_CTL_REG_5			0x60        //ENDPOINT5
//#define OFFSET_BULK_IN_BASE_ADDR_REG_5		0x64
//#define OFFSET_BULK_IN_LEN_REG_5			0x68
//#define OFFSET_BULK_IN_REAL_LEN_REG_5		0x6C
//#define OFFSET_BULK_OUT_CTL_REG_6			0x70        //ENDPOINT6
//#define OFFSET_BULK_OUT_BASE_ADDR_REG_6		0x74
//#define OFFSET_BULK_OUT_LEN_REG_6			0x78
//#define OFFSET_BULK_OUT_REAL_LEN_REG_6		0x7C
//#define OFFSET_BULK_IN_CTL_REG_7			0x80        //ENDPOINT7
//#define OFFSET_BULK_IN_BASE_ADDR_REG_7		0x84
//#define OFFSET_BULK_IN_LEN_REG_7			0x88
//#define OFFSET_BULK_IN_REAL_LEN_REG_7		0x8C
//#define OFFSET_BULK_OUT_CTL_REG_8			0x90        //ENDPOINT8
//#define OFFSET_BULK_OUT_BASE_ADDR_REG_8		0x94
//#define OFFSET_BULK_OUT_LEN_REG_8			0x98
//#define OFFSET_BULK_OUT_REAL_LEN_REG_8		0x9C

//#define OFFSET_INT_DATA_REG1_9			    0xA0        //ENDPOINT9
//#define OFFSET_INT_DATA_REG2_9			    0xA4        //ENDPOINT9
//#define OFFSET_INT_DATA_REG1_10			    0xA8        //ENDPOINT10
//#define OFFSET_INT_DATA_REG2_10			    0xAC        //ENDPOINT10

//#define OFFSET_SINGLE_READ_DATA_REG  	    0xB0
//#define OFFSET_SINGLE_WRITE_DATA_REG  	    0xB4
//#define OFFSET_RW_CTRL_REG  			    0xB8

//#define OFFSET_8051_CTL_REG					0xC0


///* 8051 control register definitions        offset = 0xc0   */
//#define D8051_START_BIT					    BIT0        //C0       
//#define D8051_LOOP_BACK_MODE_BIT		    BIT1
//// BIT2 - BIT7      RESERVED
//#define D8051_BOOT_BIT					    BIT8        //C1
//#define D8051_SPU_BOOT_LOCATION_BIT		    BIT9
//// BIT10 - BI23      RESERVED
//#define D8051_SCRATCH_REGISTER_BIT		    (BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)



//bits
#define MAC_TX_STOPPED_BIT				 BIT_20

#define MAC_SEL_CP_FIFO_BIT				 0
#define MAC_SEL_CFP_FIFO_BIT			 BIT0
#define MAC_SEL_ATIM_FIFO_BIT			 BIT1
#define MAC_SEL_BEACON_FIFO_BIT		     BIT_0_1

#if 0
#define MAC_SOFT_RESET_BIT				 BIT_0
#define MAC_DOT11A_BIT					 BIT_1
#define MAC_BSS_BIT						 BIT_2
#define MAC_AP_BIT						 BIT_3
#define MAC_DUP_FILTER_EN_BIT			 BIT_4
#define MAC_PROMISCUOUS_EN_BIT			 BIT_5
#define MAC_FCS_CHECK_DIS_BIT			 BIT_6
#define MAC_MULTICAST_RXDIS_BIT			 BIT_7
#define MAC_MULTICAST_RX_EN_BIT			 0
#define MAC_A6MPS_B1MPS_G1MPS_BIT		 BIT_8
#define MAC_A9MPS_B2MPS_G2MPS_BIT		 BIT_9
#define MAC_A12MPS_B5MPS_G5MPS_BIT		 BIT_10
#define MAC_A18MPS_B11MPS_G11MPS_BIT	 BIT_11
#define MAC_A24MPS_BIT					 BIT_12
#define MAC_A36MPS_BIT					 BIT_13
#define MAC_A48MPS_BIT					 BIT_14
#define MAC_A54MPS_BIT					 BIT_15
#define MAC_BASICRATESET_BITS_A			 BIT_8_15
#define MAC_BASICRATESET_BITS_BG		 BIT_8_11
#define	MAC_RTS_RATE_G					 BIT_12_15
#define MAC_WAKEUP_DTIM_BIT				 BIT_16
#define MAC_DOT11G_BIT					 BIT_17
#define MAC_CW_MIN_BIT					 BIT_18
#define MAC_EXC_UNENCRYPTED_BIT			 BIT_19
#define MAC_WR_UNDECRYPT_RX_BIT			 BIT_20
//v3chip,v4chip
#define MAC_ENABLE_ECO_ITEM17			 BIT_22
#define MAC_CFP_POLLABLE_BIT			 BIT_23
#define MAC_RXFIFO_FLUSH_BIT			 BIT_24
#define MAC_TXFIFO_FLUSH_BIT			 BIT_25
#define MAC_IGNORE_ID_ON_RCV_BIT		 BIT_28
#define MAC_PWR_MGT_BIT					 BIT_29
#define MAC_RX_FRG_COUNT_TO_1_BIT		 BIT_30
#endif

#define MAC_SOFT_RESET_BIT				 BIT_0
#define MAC_DOT11A_BIT					 BIT_1
#define MAC_BSS_BIT						 BIT_2
#define MAC_AP_BIT						 BIT_3
#define MAC_DUP_FILTER_EN_BIT			 BIT_4
#define MAC_PROMISCUOUS_EN_BIT			 BIT_5
#define MAC_FCS_CHECK_DIS_BIT			 BIT_6
#define MAC_MULTICAST_RXDIS_BIT			 BIT_7
#define MAC_MULTICAST_RX_EN_BIT			 0
#define MAC_A6MPS_B1MPS_G1MPS_BIT		 BIT_8
#define MAC_A9MPS_B2MPS_G2MPS_BIT		 BIT_9
#define MAC_A12MPS_B5MPS_G5MPS_BIT		 BIT_10
#define MAC_A18MPS_B11MPS_G11MPS_BIT	 BIT_11
#define MAC_A24MPS_BIT					 BIT_12
#define MAC_A36MPS_BIT					 BIT_13
#define MAC_A48MPS_BIT					 BIT_14
#define MAC_A54MPS_BIT					 BIT_15
#define MAC_BASICRATESET_BITS_A			 BIT_8_15
#define MAC_BASICRATESET_BITS_BG		 BIT_8_11
#define	MAC_RTS_RATE_G					 BIT_12_15
#define MAC_WAKEUP_DTIM_BIT				 BIT_16
#define MAC_DOT11G_BIT					 BIT_17
#define MAC_CW_MIN_BIT					 BIT_18
#define MAC_EXC_UNENCRYPTED_BIT			 BIT_19
#define MAC_WR_UNDECRYPT_RX_BIT			 BIT_20
#define MAC_RX_ANY_BSSID_BEACON_BIT		 BIT_21
#define MAC_ENABLE_ECO_ITEM17			 BIT_22
#define MAC_CFP_POLLABLE_BIT			 BIT_23
#define MAC_RXFIFO_FLUSH_BIT			 BIT_24
#define MAC_TXFIFO_FLUSH_BIT			 BIT_25
#define MAC_IGNORE_ID_ON_RCV_BIT		 BIT_28
#define MAC_PWR_MGT_BIT					 BIT_29
#define MAC_RX_FRG_COUNT_TO_1_BIT		 BIT_30




//#ifdef STA_V3_L0

#define MHDMA_RX_CNTRL_ENABLE_BIT     BIT_20
#define MHDMA_RX_CNTRL_STOP_BIT       BIT_21
#define MHDMA_RX_CNTRL_ABORT_BIT      BIT_22
#define MHDMA_RX_CNTRL_EARLYINT_BIT   BIT_23

#define MHDMA_TX_CNTRL_ENABLE_BIT     BIT_24
#define MHDMA_TX_CNTRL_STOP_BIT       BIT_25
#define MHDMA_TX_CNTRL_ABORT_BIT      BIT_26
#define MHDMA_TX_CNTRL_BD_IN_HOST_BIT BIT_27
#define MHDMA_TX_CNTRL_TXMODE1_BIT    BIT_28
#define MHDMA_TX_CNTRL_TXMODE2_BIT    BIT_29
#define MHDMA_TX_CNTRL_MULTREAD_BIT   BIT_30
#define MHDMA_TX_CNTRL_READLINE_BIT   BIT_31
//#endif


#define FRAME_TX_ISR			BIT_0	/* bit pos 0  */
#define FRAME_RX_ISR			BIT_1	/* bit pos 1  */
#define TX_ERR_ISR				BIT_2	/* bit pos 2  */
#define ATIM_RXED_ISR			BIT_3  	/* bit pos 3  */
#define DUP_FILTERED_ISR		BIT_4  	/* bit pos 4 */
#define RCV_ERR_ISR				BIT_5  	/* bit pos 5 */
#define IMP_TBTT_ISR			BIT_6  	/* bit pos 6 */
#define TSCPN_LMT_RCHED_ISR	BIT_7  	/* bit pos 7  */
#define BBRW_ISR				BIT_8  	/* bit pos 8 */
#define IDLE_ISR					BIT_9  	/* bit pos 9 */
#define GEN_TIMER_ISR			BIT_10 	/* bit pos 10 */
#define ATIM_CFP_OVER_ISR		BIT_11 	/* bit pos 11 */

#define TX_FIFO_UNDERRUN_ISR	BIT_16	 /* bit pos 16 */
#define RX_FIFO_OVER_FLOW_ISR	BIT_17 	    /* bit pos 17 */
#define RX_UNICAST_KEY_NOT_FOUND_ISR BIT_18  /* bit pos 18  */



//#define ALL_BITS	(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define MACHW_INT_N_MASK	\
		(  \
			(TX_ERR_ISR) |   \
		   	(RCV_ERR_ISR) | \
		   	(TX_FIFO_UNDERRUN_ISR) | (IDLE_ISR) | \
			(RX_FIFO_OVER_FLOW_ISR) |   \
			(RX_UNICAST_KEY_NOT_FOUND_ISR) | \
			(BIT_31) \
		)
/*   end    register 3dsp define  */












/*

static unsigned int static_MAC_reg_restore_list[] = 
{
    WLS_MAC_ADDRLO_WD,
    WLS_MAC_ADDRHI_WD,
    WLS_MAC_BSSIDLO_WD,
    WLS_MAC_BSSIDHI_WD,
//    WLS_MAC_STATECONTROL_WD,
    WLS_MAC_CONTROL_WD,
    WLS_MAC_PROBETIMER_WD,
    WLS_MAC_BEACONINTERVAL_WD,
    WLS_MAC_TXBCNLENGTH_WD,
    WLS_MAC_SCANCHTIMER_ATIM_WD,
    WLS_MAC_RTSTHRESHRTLIM_WD,
//    WLS_MAC_INTEVENTSET_WD,
//    WLS_MAC_INTEVENTCLEAR_WD,
    WLS_MAC_INTMASK_WD,
//    WLS_MAC_STATUS_WD,
//    WLS_MAC_TSFLO_WD,
//    WLS_MAC_TSFHI_WD,
    WLS_MAC_CFP_WD,
    WLS_MAC_TXFIFOSIZE_WD,
//    WLS_MAC_FIFOWATERMARK_WD,
    WLS_MAC_FIFOSEL_WD,
//    WLS_MAC_FIFOSTATUS_WD,
    WLS_MAC_TIMINGSREG1_WD,
    WLS_MAC_TIMINGSREG2_WD,
//    WLS_MAC_TSFOFFSETLO_WD,
//    WLS_MAC_TSFOFFSETHI_WD,
    WLS_MAC_RANDSEED_WD,
    WLS_MAC_GENTIMER_WD,
    WLS_MAC_RADIOACCESS_WD,
    WLS_MAC_INTBBMASK_WD,
//    WLS_MAC_TSCPNLMTRCHED0_WD,
//    WLS_MAC_TSCPNLMTRCHED1_WD,
    WLS_MAC_BRS11GOFDM_WD,
    WLS_MAC_WEPKEY0_WD,
    WLS_MAC_WEPKEY1_WD,
    WLS_MAC_WEPKEY2_WD,
    WLS_MAC_WEPKEY3_WD,
    WLS_MAC_WEPKEYADDR_WD,
    WLS_MAC_WEPKEYCNTLADDR_WD,
    WLS_MAC_WEPKEYSIZE_WD,
    WLS_MAC_KEYOPERATIONCNTL_WD,
//    WLS_MAC_TXFRGCOUNT_WD,
//    WLS_MAC_RXFRGCOUNT_WD,
    WLS_MAC_BCNBOCNT_WD,
    WLS_MAC_PRIBOCNT_WD,
//    WLS_MAC_TAFORNOKEYLOW_WD,
//    WLS_MAC_TAFORNOKEYHIGH_WD,
    WLS_MAC_CPBOCNT_WD,
//    WLS_MAC_NAVCWCNT_WD,
    WLS_MAC_DEBUG0_WD,
    WLS_MAC_DEBUG1_WD,
//    WLS_MAC_DEBUG2_WD,
    WLS_MAC_LED_CTRL_WD,
//    WLS_MAC_STARETRYCNT_WD,
//    WLS_MAC_CPRETRYCNT_WD,
//    WLS_MAC_CFPRETRYCNT_WD,
//    WLS_MAC_PERFCNT4_WD,
//    WLS_MAC_PERFCNT0_WD,
//    WLS_MAC_PERFCNT1_WD,
//    WLS_MAC_PERFCNT2_WD,
//    WLS_MAC_PERFCNT3_WD,
//    WLS_MAC_BBREG0_3_WD,
//    WLS_MAC_BBREG4_7_WD,
//    WLS_MAC_BBREG8_11_WD,
//    WLS_MAC_BBREG12_15_WD,
    WLS_MAC_BBREG16_19_WD,
    WLS_MAC_BBREG20_23_WD,
    WLS_MAC_BBREG24_27_WD
//    WLS_MAC_BBREG28_31_WD,
//    WLS_MAC_BBREG32_35_WD,
//    WLS_MAC_BBREG36_38_WD
};
*/
#define WLS_SCRATCH_READ_BACK_MAX 10

#define     OFDM_SIGNAL_EXTENSION_11G   6

#endif          // _3DSP_DEFS_H

