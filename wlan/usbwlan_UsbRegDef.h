///*************************************************************************
//                     Syntek Semiconductor Co., Ltd.
//              Copyright (c) 2006 All rights reserved
//
// File Name:   	 Usb.h      --->   usbwlan_UsbRegDef.h
// Author:  		 Jazz Chang
// Created: 		 2006.09.25
// Version: 		 1.0
// Modified:
//              1. Edit to USB_spec_v1.96. by Justin, on 2007.02.14
//*************************************************************************

#ifndef __USB_REG_DEF_H__
#define __USB_REG_DEF_H__

#include "precomp.h"
//#include "TypeDef.h"
//#include <reg51.h>

#define REG_USBCTL1         0x00
#define REG_USBCTL2         0x04
#define REG_USBINTR1        0x05
#define REG_USBINTR2        0x09
#define REG_USBMEMR         0x0B
#define REG_USBIDR          0x0C
#define REG_UIR             0x14
#define REG_BICR1           0x20
#define REG_BIBR1           0x24
#define REG_BILR1           0x28
#define REG_BIRLR1          0x2C
#define REG_BOCR2           0x30
#define REG_BOBR2           0x34
#define REG_BOLR2           0x38
#define REG_BORLR2          0x3C
#define REG_BICR3           0x40
#define REG_BIBR3           0x44
#define REG_BILR3           0x48
#define REG_BIRLR3          0x4C
#define REG_BOCR4           0x50
#define REG_BOBR4           0x54
#define REG_BOLR4           0x58
#define REG_BORLR4          0x5C
#define REG_BICR5           0x60
#define REG_BIBR5           0x64
#define REG_BILR5           0x68
#define REG_BIRLR5          0x6C
#define REG_BOCR6           0x70
#define REG_BOBR6           0x74
#define REG_BOLR6           0x78
#define REG_BORLR6          0x7C
#define REG_BICR7           0x80
#define REG_BIBR7           0x84
#define REG_BILR7           0x88
#define REG_BIRLR7          0x8C
#define REG_BOCR8           0x90
#define REG_BOBR8           0x94
#define REG_BOLR8           0x98
#define REG_BORLR8          0x9C
#define REG_IDR1E9          0xA0
#define REG_IDR2E9          0xA4
#define REG_IDR1E10         0xA8
#define REG_IDR2E10         0xAC
#define REG_SRDR            0xB0
#define REG_SWDR            0xB4
#define REG_RWCR            0xB8
#define REG_8051CR          0xC0
#define REG_MAILBOX_HEAD    0x100
#define REG_MAILBOX_FIFO_SEL    0x101  
#define REG_MAILBOX_FIFO_LEN    0x102    
#define REG_MAILBOX_CMD     0x108
#define REG_MAILBOX_FLAG    0xEF
#define REG_MAILBOX_DATA    0x109

#define REG_8051_RUN_FLAG	0x1fc	//  get  '3dsp' if 8051 has run
#define REG_VERSION_8051	0x134

#define REG_USBFIFO         0x400



// REG_USBCTL1+0 (0x00)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Descriptor Ready in Memory (0: Normal, 1: Descriptor data is ready)
// | | | | | | +----> Interrupt 8051 Pending (0: Normal, 1: There is an interrupt to CPU)
// | | | | | +------> Interrupt Endpoint9 Pending (0: Default, 1: Data is ready)
// | | | | +--------> Interrupt Endpoint10 Pending (0: Default, 1: Data is ready)
// | | | +----------> Soft Disconnect (0: Normal, 1: The device will be disconnected)
// | | +------------> PCI Interface Ready (0: Not ready, 1: Ready)
// | +--------------> Bus Reset Enable (0: Disable, 1: Enable)
// +----------------> USB Speed (0: Full Speed = 12Mbps, 1: High Speed = 480Mbps)
#define BIT_MDR             0x01
#define BIT_I8P             0x02
#define BIT_IP9             0x04
#define BIT_IP10            0x08
#define BIT_SD              0x10
#define BIT_PCIRDY          0x20
#define BIT_BRE             0x40
#define BIT_HF              0x80

// REG_USBINTR1+1 (0x01)
// Bulk In/Out Stall
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In Stall Endpoint1 (0: not stalled, 1: stalled)
// | | | | | | +----> Bulk Out Stall Endpoint2 (0: not stalled, 1: stalled)
// | | | | | +------> Bulk In Stall Endpoint3 (0: not stalled, 1: stalled)
// | | | | +--------> Bulk Out Stall Endpoint4 (0: not stalled, 1: stalled)
// | | | +----------> Bulk In Stall Endpoint5 (0: not stalled, 1: stalled)
// | | +------------> Bulk Out Stall Endpoint6 (0: not stalled, 1: stalled)
// | +--------------> Bulk In Stall Endpoint7 (0: not stalled, 1: stalled)
// +----------------> Bulk Out Stall Endpoint8 (0: not stalled, 1: stalled)
#define BIT_BIS1            0x01
#define BIT_BOS2            0x02
#define BIT_BIS3            0x04
#define BIT_BOS4            0x08
#define BIT_BIS5            0x10
#define BIT_BOS6            0x20
#define BIT_BIS7            0x40
#define BIT_BOS8            0x80

// REG_USBINTR1+2 (0x02)
// Bulk In/Out Stall All/Once
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In Stall All Endpoint1
// | | | | | | +----> Bulk In Stall Once Endpoint1
// | | | | | +------> Bulk Out Stall All Endpoint2
// | | | | +--------> Bulk Out Stall Once Endpoint2
// | | | +----------> Bulk In Stall All Endpoint3
// | | +------------> Bulk In Stall Once Endpoint3
// | +--------------> Bulk Out Stall All Endpoint4
// +----------------> Bulk Out Stall Once Endpoint4
#define BIT_BISA1           0x01
#define BIT_BISO1           0x02
#define BIT_BOSA2           0x04
#define BIT_BOSO2           0x08
#define BIT_BISA3           0x10
#define BIT_BISO3           0x20
#define BIT_BOSA4           0x40
#define BIT_BOSO4           0x80

// REG_USBINTR1+2 (0x03)
// Bulk In/Out Stall All/Once
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In Stall All Endpoint5
// | | | | | | +----> Bulk In Stall Once Endpoint5
// | | | | | +------> Bulk Out Stall All Endpoint6
// | | | | +--------> Bulk Out Stall Once Endpoint6
// | | | +----------> Bulk In Stall All Endpoint7
// | | +------------> Bulk In Stall Once Endpoint7
// | +--------------> Bulk Out Stall All Endpoint8
// +----------------> Bulk Out Stall Once Endpoint8
#define BIT_BISA5           0x01
#define BIT_BISO5           0x02
#define BIT_BOSA6           0x04
#define BIT_BOSO6           0x08
#define BIT_BISA7           0x10
#define BIT_BISO7           0x20
#define BIT_BOSA8           0x40
#define BIT_BOSO8           0x80

// REG_USBCTL2+0 (0x04)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Interrupt Stall All Endpoint9
// | | | | | | +----> Interrupt Stall Once Endpoint9
// | | | | | +------> Interrupt Stall All Endpoint10
// | | | | +--------> Interrupt Stall Once Endpoint10
// | | | +----------> Interrupt Stall Endpoint9
// | | +------------> Interrupt Stall Endpoint10
// | +--------------> Reserved
// +----------------> Reserved
#define BIT_INTA9           0x01
#define BIT_INTO9           0x02
#define BIT_INTA10          0x04
#define BIT_INTO10          0x08
#define BIT_INTS9           0x10
#define BIT_INTS10          0x20

// REG_USBINTR1+0 (0x05)
// Bulk In/Out transfer Request interrupt Enable
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In transfer Request Interrupt Enable Endpoint1 (0: not cause, 1: cause) 
// | | | | | | +----> Bulk Out transfer Request Interrupt Enable Endpoint2 (0: not cause, 1: cause) 
// | | | | | +------> Bulk In transfer Request Interrupt Enable Endpoint3 (0: not cause, 1: cause) 
// | | | | +--------> Bulk Out transfer Request Interrupt Enable Endpoint4 (0: not cause, 1: cause) 
// | | | +----------> Bulk In transfer Request Interrupt Enable Endpoint5 (0: not cause, 1: cause) 
// | | +------------> Bulk Out transfer Request Interrupt Enable Endpoint6 (0: not cause, 1: cause) 
// | +--------------> Bulk In transfer Request Interrupt Enable Endpoint7 (0: not cause, 1: cause) 
// +----------------> Bulk Out transfer Request Interrupt Enable Endpoint8 (0: not cause, 1: cause) 
#define BIT_BIRE1           0x01
#define BIT_BORE2           0x02
#define BIT_BIRE3           0x04
#define BIT_BORE4           0x08
#define BIT_BIRE5           0x10
#define BIT_BORE6           0x20
#define BIT_BIRE7           0x40
#define BIT_BORE8           0x80

// REG_USBINTR1+1 (0x06)
// Bulk In/Out transfer Done interrupt Enable
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In transfer Done Interrupt Enable Endpoint1 (0: not cause, 1: cause)
// | | | | | | +----> Bulk Out transfer Done Interrupt Enable Endpoint2 (0: not cause, 1: cause)
// | | | | | +------> Bulk In transfer Done Interrupt Enable Endpoint3 (0: not cause, 1: cause)
// | | | | +--------> Bulk Out transfer Done Interrupt Enable Endpoint4 (0: not cause, 1: cause)
// | | | +----------> Bulk In transfer Done Interrupt Enable Endpoint5 (0: not cause, 1: cause)
// | | +------------> Bulk Out transfer Done Interrupt Enable Endpoint6 (0: not cause, 1: cause)
// | +--------------> Bulk In transfer Done Interrupt Enable Endpoint7 (0: not cause, 1: cause)
// +----------------> Bulk Out transfer Done Interrupt Enable Endpoint8 (0: not cause, 1: cause)
#define BIT_BIDE1           0x01
#define BIT_BODE2           0x02
#define BIT_BIDE3           0x04
#define BIT_BODE4           0x08
#define BIT_BIDE5           0x10
#define BIT_BODE6           0x20
#define BIT_BIDE7           0x40
#define BIT_BODE8           0x80

// REG_USBINTR1+2 (0x07)
// Bulk In/Out transfer Request Data
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In transfer Request Data Endpoint1 (0: Normal, 1: Request, cleared by writing "1")
// | | | | | | +----> Bulk Out transfer Request Data Endpoint2 (0: Normal, 1: Request, cleared by writing "1")
// | | | | | +------> Bulk In transfer Request Data Endpoint3 (0: Normal, 1: Request, cleared by writing "1")
// | | | | +--------> Bulk Out transfer Request Data Endpoint4 (0: Normal, 1: Request, cleared by writing "1")
// | | | +----------> Bulk In transfer Request Data Endpoint5 (0: Normal, 1: Request, cleared by writing "1")
// | | +------------> Bulk Out transfer Request Data Endpoint6 (0: Normal, 1: Request, cleared by writing "1")
// | +--------------> Bulk In transfer Request Data Endpoint7 (0: Normal, 1: Request, cleared by writing "1")
// +----------------> Bulk Out transfer Request Data Endpoint8 (0: Normal, 1: Request, cleared by writing "1")
#define BIT_BIRD1           0x01
#define BIT_BORD2           0x02
#define BIT_BIRD3           0x04
#define BIT_BORD4           0x08
#define BIT_BIRD5           0x10
#define BIT_BORD6           0x20
#define BIT_BIRD7           0x40
#define BIT_BORD8           0x80

// REG_USBINTR1+3 (0x08)
// Bulk In/Out transfer Done Data
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Bulk In transfer Done Data Endpoint1 (0: Normal, 1: Done, cleared by writing "1")
// | | | | | | +----> Bulk Out transfer Done Data Endpoint2 (0: Normal, 1: Done, cleared by writing "1")
// | | | | | +------> Bulk In transfer Done Data Endpoint3 (0: Normal, 1: Done, cleared by writing "1")
// | | | | +--------> Bulk Out transfer Done Data Endpoint4 (0: Normal, 1: Done, cleared by writing "1")
// | | | +----------> Bulk In transfer Done Data Endpoint5 (0: Normal, 1: Done, cleared by writing "1")
// | | +------------> Bulk Out transfer Done Data Endpoint6 (0: Normal, 1: Done, cleared by writing "1")
// | +--------------> Bulk In transfer Done Data Endpoint7 (0: Normal, 1: Done, cleared by writing "1")
// +----------------> Bulk Out transfer Done Data Endpoint8 (0: Normal, 1: Done, cleared by writing "1")
#define BIT_BIDD1           0x01
#define BIT_BODD2           0x02
#define BIT_BIDD3           0x04
#define BIT_BODD4           0x08
#define BIT_BIDD5           0x10
#define BIT_BODD6           0x20
#define BIT_BIDD7           0x40
#define BIT_BODD8           0x80

// REG_USBINTR2+0 (0x09)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Resume Interrupt Enable (0: The resume will not cause an interrupt to CPU, 1: will cause)
// | | | | | | +----> Suspend Interrupt Enable (0: not cause an interrupt to CPU, 1: will cause)
// | | | | | +------> Bus Reset Interrupt Enable (0: not, 1: will cause)
// | | | | +--------> Reserved
// | | | +----------> DMA1 Interrupt Enable
// | | +------------> DMA2 Interrupt Enable
// | +--------------> Reserved
// +----------------> Reserved
#define BIT_RSIE             0x01
#define BIT_SPIE             0x02
#define BIT_BRIE             0x04
#define BIT_DMA1E           0x10
#define BIT_DMA2E           0x20

// REG_USBINTR2+1 (0x0A)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Resume Data
// | | | | | | +----> Suspend Data
// | | | | | +------> Bus Reset Data
// | | | | +--------> MailBox Interrupt (0: disable, 1: enable)
// | | | +----------> DMA1 Interrupt
// | | +------------> DMA2 Interrupt
// | +--------------> Reserved
// +----------------> Reserved
#define BIT_RSD             0x01
#define BIT_SPD             0x02
#define BIT_BRD             0x04
#define BIT_MIN             0x08
#define BIT_DMA1            0x10
#define BIT_DMA2            0x20

// REG_USBMEMR (0x0B)
// 7--0               MemOwnerIdleFrames
#define BIT_MOIF            0xFF

// REG_USBIDR (0x0C)
// 31--16              VID: Vendor ID       (0x0E -- 0x0F)
// 15--0               PID: Product ID      (0x0C -- 0x0D)
#define BIT_VID             0xFFFF
#define BIT_PID             0xFFFF

// REG_UIR+0 (0x14)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> iManufacturer (0: Default, StringIndex = 1. 1: StringIndex = 0)
// | | | | | | +----> iProduct (0: Default, StringIndex = 2, 1: StringIndex = 0)
// | | | | | +------> iSerialNumber (0: Default, StringIndex = 3, 1: StringIndex = 0)
// | | | | +--------> Power State (00: Self powered 100mA)
// | | | +----------> Power State (10: Bus powered 100mA, 11: Bus powered 500mA)
// | | +------------> Descriptor Source
// | +--------------> Boot 8051 (BT. 0: Enable 8051 Boot, 1: Disable 8051 boot until Start 8051 bit is set)
// +----------------> CPU Boot Location (0: Boot from address 0x0000(ROM), 1: Boot from address oxFFC0(SRAM))
#define BIT_MANUFACTURER    0x01
#define BIT_PRODUCT         0x02
#define BIT_SERIALNUMBER    0x04
#define BIT_MASK_POWER      0xE7
#define BIT_SELFPOWER100MA  0x00
#define BIT_BUSPOWER100MA   0x10
#define BIT_BUSPOWER500MA   0x18
#define BIT_DESCSCR         0x20
#define BIT_BOOT8051        0x40
#define BIT_BOOTLOCATION    0x80

// REG_BICR1 (0x20), REG_BOCR2 (0x30), REG_BICR3 (0x40), REG_BOCR4 (0x50)
// REG_BICR5 (0x60), REG_BOCR6 (0x70), REG_BICR7 (0x80), REG_BOCR8 (0x90)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> XfrEnableSingle is set to do a transfer, which uses a single contiguous block of memory
// | | | | | | +----> XfrEnableMultiple is set to do a transfer, which uses non-contiguous memory
// | | | | | +------> XfrReady is set to start transferring data
// | | | | +--------> ForceZeroLength: Force device to return a zero length packet.
// | | | +----------> XfrBusy: It is set after XfrReady
// | | +------------> DMA1 address (0: Auto, 1: Not auto)
// | +--------------> DMA2 address (0: Auto, 1: Not auto)
// +----------------> Threshold control of endpointer's fifo 
//                    (1: host can read after fifo is full for every packet,
//                     0: host can read after fifo has burst data)
#define BIT_XFR_EN_SING         0x01
#define BIT_XFR_EN_MULT         0x02
#define BIT_XFR_READY           0x04
#define BIT_FORCEZEROLENGTH     0x08
#define BIT_XFR_BUSY            0x10
#define BIT_XFR_DMA1_NOTINC     0x20
#define BIT_XFR_DMA2_NOTINC     0x40
#define BIT_XFR_THRESHOLD_CTL   0x80

// REG_BIBR1 (0x24), REG_BOBR1 (0x34), REG_BIBR1 (0x24), REG_BOBR1 (0x34)
// REG_BIBR1 (0x24), REG_BOBR1 (0x34), REG_BIBR1 (0x24), REG_BOBR1 (0x34)
// bit 31:18 This is the DMA2 starting address of Bulk In transfer in memory,this address is double-word alignment
// bit 17:16 Reserved
// bit 15:2  This is the DMA1 starting address of Bulk In transfer in memory, this address is double-word alignment 
// bit 1:0   00: Application side scratch memory
//           01: Local 64KB program memory
//           1X: Local 512B scratch memory
#define MASK_DMA2_ADDR      0xFFFC0000
#define OFFSET_DMA2_ADDR    18
#define MASK_DMA1_ADDR      0x0000FFFC
#define OFFSET_DMA1_ADDR    2
#define DMA_APPL_SCRATCH_MEM    0x00
#define DMA_LOCAL_PROG_MEM      0x01
#define DMA_LOCAL_SCRATCH_MEM   0x10

// REG_RWCR+2 (0xBA)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Activate R/W transfer (1: active)
// | | | | | | +----> R/W direction (0:read, 1:write)
// | | | | | +------> Access (0: Singne, 1:Burst)
// | | | | +--------> Burst access address increases (0: Auto, 1: Not auto)
// | | | +----------> (0: Application side scratch memory, 1: Loacal 64k program memory)
// | | +------------> Reserved
// | +--------------> Reserved
// +----------------> Reserved
#define BIT_3DSP_ACTIVE_RW  0x01
#define BIT_3DSP_WRITE      0x02
#define BIT_3DSP_BURST      0x04
#define BIT_3DSP_NOTINC     0x08
#define BIT_3DSP_MEM_ACCESS 0x10

// REG_8051CR (0xC0)
// 7 6 5 4 3 2 1 0
// | | | | | | | |
// | | | | | | | +--> Start 8051 (ST. 0: Default; Boot after reset. 1: Associate with BT strap.)
// | | | | | | +----> Loop Back Mode (0: Default; 64KB code memory space, 1: Upper 32KB space is data memory; Lower 32KB space is code memory)
// | | | | | +------> Reserved
// | | | | +--------> Reserved
// | | | +----------> Reserved
// | | +------------> Reserved
// | +--------------> Reserved
// +----------------> Reserved
#define BIT_START8051_DEFAULT       0x00
#define BIT_START8051_ASSO_BT       0x01
#define BIT_LOOPBACKMODE            0x02

//Vendor Commmand
//#define CMD_NULL            0x00
//#define CMD_READ_REG        0x01
//#define CMD_WRITE_REG       0x02

//Header Command
//#define CMD_HEAD            0x01



#endif // __USB_REG_DEF_H__
