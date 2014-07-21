#ifndef DSP_USBCONFIG_H
#define DSP_USBCONFIG_H

#include "precomp.h"

/***********************************************************************
 * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
 *
 * FILENAME:     DSP_usbConfig.h      CURRENT VERSION: 1.00.01
 * PURPOSE:      mainly define some macros used for usb configuration
 * 
 *
 * DECLARATION:  This document contains confidential proprietary information that
 *               is solely for authorized personnel. It is not to be disclosed to
 *               any unauthorized person without prior written consent of 3DSP
 *               Corporation.       
 *
 **********************************************************************/
	
/*--macros------------------------------------------------------------*/
// macro between usb host and usb device
#define DSP_COMMAND_GET_ETHERNET_DESC   0x00

#define DSP_COMMAND_GET_REGISTER_VALUE  0x10
#define DSP_COMMAND_SET_REGISTER_VALUE  0x11

#define DSP_COMMAND_SET_MAC_SOFT_RESET  0x20
#define DSP_COMMAND_SET_MAC_ENABLE   0x21


// in or out type
#define  IO_TYPE_BYTE                     1
#define  IO_TYPE_WORD                     2
#define  IO_TYPE_DWORD                    3

// USB mode 
#define  USB_MODE_USB20                   1
#define  USB_MODE_USB11                   2

// USB max packet size
#define  USB_MAXPACKETSIZE_USB20          512
#define  USB_MAXPACKETSIZE_USB11          64

// macro for others
#define DSP_CNTRL_TIMEOUT  10000
#define DSP_INT_BUF_SIZE      32
#define DSP_TX_BUF_SIZE       2048
#define DSP_TX_HEAD_BUF_SIZE       8

#define DSP_FIXED_INT_SIZE	8  	//Jakio 2007.02.05, 

#define DSP_MAX_RECDATA_LEN    2048
#define DSP_MAX_TXDATA_LEN     2048

/* Max PSQ size */
#define DSP_MAX_PSQ_SIZE       16

/* Timer period */
#define TIMERPERIOD              200 /* Timer is 100ms periodic timer */

#define TIMERDELAY               200 /* delay 200ms to detect cp, cfp and atim descriptor */

#define TIMER_PENDING_COUNT               2000 /* delay 2s to detect if OID Power indicate lost*/

/* Macro used for the return value of Adap_Get_Firmware_Status */
#define USB_FIRMWARE_STATUS_NOT_COMPLETE      (0x00)
#define USB_FIRMWARE_STATUS_COMPLETE      (0xff)
#define USB_FIRMWARE_STATUS_FAILURE      (0x55)

/* Macro used for soft-reset bitmap */
#define MAC_MODULE_SOFT_RESET             (0x0001)
#define PHY_MODULE_SOFT_RESET             (0x0002)
#define BIU_MODULE_SOFT_RESET             (0x0004)
#define MCU_MODULE_SOFT_RESET             (0x0008)
#define PMU_MODULE_SOFT_RESET             (0x0010)
#define USB_MODULE_SOFT_RESET             (0x0020)
#define PHY_MODULE_REG_RESET              (0x0040)

//#define ETHERNET_ADDRESS_LENGTH         (USHORT)(6)
//#define ETHERNET_HEADER_SIZE				(14)
//#define MINIMUM_ETHERNET_PACKET_SIZE		(60)
//#define MAXIMUM_ETHERNET_PACKET_SIZE    	(1514)
//#define RCB_BUFFER_SIZE						(1520) // 0x5F0

#define AUTO_RATE_UP_COUNTS             30
#define AUTO_RATE_DOWN_COUNTS           2


/*--constants and types------------------------------------------------*/

#pragma pack(1)

/* define the struction about inquiry descriptor list status. */
typedef struct _USB_DESCRIPTOR_LIST_STATUS {
	UINT8 ucDesNum_CP;
	UINT8 ucDesNum_CFP;
	UINT8 ucdesNUM_ATIM;
	UINT8 reserved;
} USB_DESCRIPTOR_LIST_STATUS_T, *PUSB_DESCRIPTOR_LIST_STATUS_T;

/* define the struction for SPI operation in the data stage. */
typedef struct _USB_SPI_FORMAT {
	UINT32 data_value;
	UINT32 control_value;
} USB_SPI_FORMAT_T, *PUSB_SPI_FORMAT_T;

/* define the struction for driver to configure RF chip with 4-wire SPI port. */
typedef struct _USB_GLOBAL_FORMAT {
	UINT32 config_content;
	UINT8 write_control;
} USB_GLOBAL_FORMAT_T, *PUSB_GLOBAL_FORMAT_T;

#pragma pack()


//-------------------------------------------------------------------------
// Ethernet Frame Sizes
//-------------------------------------------------------------------------
//const USHORT	ETHERNET_ADDRESS_LENGTH         =	6;
//const USHORT	ETHERNET_HEADER_SIZE			=	14;
//const USHORT	MINIMUM_ETHERNET_PACKET_SIZE	=	60;
//const USHORT	MAXIMUM_ETHERNET_PACKET_SIZE    =	1514;
//const USHORT	RCB_BUFFER_SIZE					=	1520; // 0x5F0

/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/

#endif

