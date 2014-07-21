/************************************************************************
 * Copyright (c) 2003, 3DSP Corporation, all rights reserved.  Duplication or
 * reproduction of any part of this software (source code, object code or
 * comments) without the expressed written consent by 3DSP Corporation is
 * forbidden.  For further information please contact:
 *
 * 3DSP Corporation
 * 16271 Laguna Canyon Rd
 * Irvine, CA 92618
 * www.3dsp.com
 *************************************************************************/

/*
 * REVISION HISTORY
 *   ...
 *
2005.10.14 Add a new MACRO BT_MAC_ADDR to define the register address of MAC address
2005.11.4  Add some new macros:
BT_REG_AM_CONNECTION_INDICATOR
BT_REG_SWITCH_SLOT_OFFSET
BT_REG_SWITCH_INSTANT
BT_REG_LOCAL_ACCESS_CODE
BT_REG_OP_BD_ADDR_DSP
BT_REG_INQUIRY_ACCESS_CODE
BT_DSP_INT_DSP_MODE_CHANGE
BT_DSP_INT_DSP_ROLE_SWITCH_FINISH
BT_DSP_INT_DSP_HODE_MODE_FINISH
BT_DSP_INT_DSP_SNIFF_MODE_FINISH
2005.11.11  Add some new macros:
BT_REG_INQUIRY_ACCESS_CODE
BT_REG_PAGE_ACCESS_CODE
BT_REG_INQUIRY_SCAN_ACCESS_CODE
BT_REG_FHS_FOR_INQUIRY_SCAN
BT_REG_FHS_FOR_PAGE
2005.11.17 Add some new macros for LC scanning function (inquiry scan and page scan)
BT_REG_FHS_FOR_INQUIRY_RESULT
BT_HCI_COMMAND_INDICATOR_ROLE_SWITCH_PREPARE
BT_HCI_COMMAND_INDICATOR_HODE_MODE_PREPARE
BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_PREPARE
BT_HCI_COMMAND_INDICATOR_PARK_MODE_PREPARE
BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE
BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN
BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT
BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT
BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT
BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT
BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT
BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT
BT_HCI_COMMAND_INDICATOR_CHANGE_CONN_KEY_BIT
BT_HCI_COMMAND_INDICATOR_MASTER_KEY_BIT
BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT
BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_BIT
BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT
BT_HCI_COMMAND_INDICATOR_PARK_MODE_BIT
BT_HCI_COMMAND_INDICATOR_EXIT_PARK_MODE_BIT
BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT
BT_HCI_COMMAND_INDICATOR_RESET_BIT
BT_HCI_COMMAND_INDICATOR_FLUSH_BIT
BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT
BT_HCI_COMMAND_INDICATOR_WRITE_VOICE_SETTING_BIT
BT_HCI_COMMAND_INDICATOR_WRITE_HOLD_MODE_AC_BIT
BT_HCI_COMMAND_INDICATOR_WRITE_LOOPBACK_MODE_BIT
BT_HCI_COMMAND_INDICATOR_ROLE_SWITCH_PREPARE_BIT
BT_HCI_COMMAND_INDICATOR_HODE_MODE_PREPARE_BIT
BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_PREPARE_BIT
BT_HCI_COMMAND_INDICATOR_PARK_MODE_PREPARE_BIT
BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE_BIT
BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT
BT_WRITE_COMMAND_INDICATOR
2005.12.7 Add a new macros for register DSP_TX_DISABLE. If this register is set as 1,
DSP does not check the TX BD. And if this register is set as 0, DSP does check
the TX BD.
BT_REG_DSP_TX_DISABLE
2005.12.15 Adjusted values for some registers according to the new spec.
BT_REG_LOCAL_BD_ADDR
...
BT_END_REGISTER
...
BT_DSP_INT_DSP_CLOCK_READY_EVENT
...
BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT
...
BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT
2005.12.19 Adjusted values for some registers
from BT_REG_HCI_COMMAND_INDICATOR to BT_REG_END_REGISTER
2005.12.23 Add three registers in scratch memory to transfer SCO data frame from host to device.
BT_HOST_P_SCO_PTR_REG
BT_DSP_C_SCO_PTR_REG
BT_BD_SCO_BASE_ADDR
Add one interrupt event that expresses that one SCO BD is processed.
BT_DSP_INT_SCO_TX_COMP_BIT
2005.12.28 Adjusted values for some registers
from BT_REG_HCI_COMMAND_INDICATOR to BT_REG_END_REGISTER
Add a register to save the page scan FHS frame
BT_REG_FHS_FOR_PAGE_SCAN
2005.12.29 Add a new register to indicate the current processing am address.
BT_REG_CURRENT_PROCESS_AM_ADDR
2006.1.9 Chage the value of register "BT_REG_T_SCO" from 0x2ad4 to 0x2ad3
2006.2.24 We add some code for accessing scratch pad through DMA0
#ifdef BT_ACCESS_SCRATCH_BY_DMA0
...
#endif
Add a new macro BT_WRITE_COMMAND_INDICATOR_SAFE
2006.3.28 Add a new register: BT_REG_CLOCK_INFO
Add two new interrupt event : BT_DSP_INT_DSP_MODE_CHANGE_INTO_ACTIVE, BT_DSP_INT_DSP_MODE_CHANGE_INTO_SNIFF
Add one command indicator bit: BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT
We change the way reading and writing registers
2006.4.7  Add some new registers for AFH
BT_REG_AFH_INSTANT
BT_REG_AFH_CHANNEL_MAP
BT_REG_AFH_MODE
BT_REG_AFH_CHANNEL_NUM
Add one command indicator bit for AFH
BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT
Add new function:BtInitBlueToothRegs
2006.5.10 Chage the registers' value for all FHS packets because the old value takes the space for WLAN
#ifdef BT_FHS_PACKET_TAKE_WLAN_SPACE
#define BT_REG_FHS_FOR_INQUIRY_SCAN				0x2c40
#define BT_REG_FHS_FOR_PAGE						0x2c54
#define BT_REG_FHS_FOR_INQUIRY_RESULT			0x2c68
#define BT_REG_FHS_FOR_PAGE_SCAN				0x2c7c
#else
#define BT_REG_FHS_FOR_INQUIRY_SCAN				0x2bb0
#define BT_REG_FHS_FOR_PAGE						0x2bc4
#define BT_REG_FHS_FOR_INQUIRY_RESULT			0x2bd8
#define BT_REG_FHS_FOR_PAGE_SCAN				0x2bec
#endif
Add two macro for role/switch
BT_DSP_INT_ROLE_CHANGE_FAIL_EVENT
BT_HCI_COMMAND_INDICATOR_FHS_PACKET_READY_BIT
2006.5.18 Add some values
BT_REG_ROLE_SWITCH_INSTANT
BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH
2006.5.31 Add some value for power control
BT_DSP_INT_REPORT_RX_POWER
Add a macro BT_WRITE_AM_CONNECTION_INDICATOR to write am connection indicator register.
2006.6.14 Add some registers for test mode
BT_REG_TEST_ITEMS
BT_REG_HOPPING_MODE
BT_REG_TX_FREQUENCY_DUT
BT_REG_RX_FREQUENCY_DUT
2006.6.21 Add one command indicator for test mode
BT_HCI_COMMAND_INDICATOR_TEST_MODE
2006.7.7  Add one register for combo codes
BT_REG_PREPARE_SCO_FLAG
2006.7.26 Add one register for esco
BT_REG_SECOND_LT_ADDR
BT_REG_WESCO
BT_REG_ESCO_PACKET_TYPE
BT_REG_ESCO_RX_LENGTH
BT_REG_ESCO_TX_LENGTH
2006.8.9  Add one register for debugging
BT_REG_DEBUG0
2006.8.25  Add one register for discarding spilth sco in bytes
BT_REG_SPILTH_SCO_BYTES
2006.9.12  Change some registers' offset
BT_REG_SNIFF_ATTEMPT
BT_REG_SNIFF_TIMEOUT
BT_REG_T_SNIFF
2006.9.22 Add one macro "BT_CLEAR_COMMAND_INDICATOR" to clear some command indicator. (Normally,
driver just set command indicator and DSP will clear this command indicator later with
automatic. But for some command indicator such as inquiry scan enable and page scan
enable, driver set command indicator and DSP does not clear it. So driver may clear it
by itself. This function just do this job)
 */

#ifndef __BT_HAL_H
    #define __BT_HAL_H

    //
    // Define some macros related to hardware
    //

    #include "bt_usb_vendorcom.h"

    //
    // Define CSR register
    //
    #define BT_CSR_PCI_CONTROL_REG					0x0
    #define BT_CSR_NUM_RETRY_REG					0x4
    #define BT_CSR_INT_ENABLE_REG					0x8
    #define BT_CSR_STATUS_REG						0xc
    #define BT_CSR_STATUS_CLR_REG					0x10
    #define BT_CSR_DMA0_SRC_ADDR_REG				0x20
    #define BT_CSR_DMA0_DST_ADDR_REG				0x24
    #define BT_CSR_DMA0_CONTROL						0x28
    #define BT_CSR_FUNC_EVENT_REG					0x40
    #define BT_CSR_FUNC_EVENT_MASK_REG				0x44
    #define BT_CSR_FUNC_PRESENT_STATE_REG			0x48
    #define BT_CSR_FUNC_FORCE_EVENT_REG				0x4c











//
// Define some scratch region macro
//

// Scratch memory start address
#define BT_SCRATCH_MEM_BASE_ADDR	0x2000
// BD start address
#define BT_BD_BASE_ADDR             0x2140
// Rx buffer start address
#define BT_RX_BUFFER_BASE_ADDR      0x2400
// HostPptr offset
#define BT_HOST_P_PTR_REG			0x2100
// DSPCptr offset
#define BT_DSP_C_PTR_REG			0x2104
// DSPPptr offset
#define BT_DSP_P_PTR_REG			0x2108
// HostCptr offset
#define BT_HOST_C_PTR_REG			0x210c
// DSPIntStatus
#define BT_DSP_INT_STATUS_REG		0x2110

// For Sco queue
// HostScoPptr offset
#define BT_HOST_P_SCO_PTR_REG		0x2120
// DSPScoCptr offset
#define BT_DSP_C_SCO_PTR_REG		0x2124
// SCO BD start address
#define BT_BD_SCO_BASE_ADDR         0x2160

// For Slave queue
// SlavePptr offset
#define BT_HOST_P_SLAVE_PTR_REG		0x2128
// SlaveCptr offset
#define BT_DSP_C_SLAVE_PTR_REG		0x212c
// Slave BD start address
#define BT_BD_SLAVE_BASE_ADDR       0x2380

// For Master Sniff queue
// MasterSniffPptr offset
#define BT_HOST_P_MASTER_SNIFF_PTR_REG		0x2130
// MasterSniffCptr offset
#define BT_DSP_C_MASTER_SNIFF_PTR_REG		0x2134
// Master Sniff BD start address
#define BT_BD_MASTER_SNIFF_BASE_ADDR        0x2398

// For Slave1 queue
// Slave1Pptr offset
#define BT_HOST_P_SLAVE1_PTR_REG		0x20f8
// Slave1Cptr offset
#define BT_DSP_C_SLAVE1_PTR_REG		0x20fc
// Master Sniff BD start address
#define BT_BD_SLAVE1_BASE_ADDR        0x20e0

// For the forth region of scratch memory
#define BT_BASEBAND_VER				0x2c00
#define BT_SP20_READY				0x2c04

// for the mac address in the mac region
#define BT_MAC_ADDR					0x4008


// Define rx buffer tail
#define BT_RX_BUFFER_TAIL      (BT_RX_BUFFER_BASE_ADDR + MAX_RX_BUF_COUNT)

// Define Bluetooth register
#define BT_REG_LOCAL_BD_ADDR					0x2180
#define BT_REG_INQUIRY_SCAN_ACCESS_CODE			0x2188
#define BT_REG_INQUIRY_SCAN_WINDOW				0x2191
#define BT_REG_INQUIRY_ACCESS_CODE				0x2194
#define BT_REG_INQUIRY_LENGTH					0x219d
#define BT_REG_PAGE_ACCESS_CODE					0x21a0
#define BT_REG_PAGE_TIMEOUT						0x21a9
#define BT_REG_SLAVE1_AM_ADDR					0x21ab
#define BT_REG_AM_ADDR							0x21ac
#define BT_REG_AM_BD_ADDR				0x21b4
#define 	BT_REG_AM_BD_ADDR0			0x21b4
#define BT_REG_AM_CLK_OFFSET_SLAVE0				0x21ba
#define BT_REG_AM_BD_ADDR_MASTER				0x21bc
#define BT_REG_AM_CLK_OFFSET_MASTER				0x21c2
#define BT_REG_AM_BD_ADDR_SLAVE1				0x21c4
#define BT_REG_AM_CLK_OFFSET_SLAVE1				0x21ca
#define BT_REG_ENCRYPTION_KEY					0x21f4
#define BT_REG_HCI_COMMAND_INDICATOR			0x2274
#define BT_REG_D_SNIFF							0x2278
#define BT_REG_SNIFF_ATTEMPT					0x227a
#define BT_REG_SNIFF_TIMEOUT					0x227c
#define BT_REG_T_SNIFF							0x227e
#define BT_REG_SNIFF_AMADDR						0x2280
#define BT_REG_CURRENT_PAGE_SCAN_REP_MODE		0x2282
#define BT_REG_ENCRYPTION_KEY_SLAVE1			0x2284
#define BT_REG_CLOCK_INFO_SLAVE1				0x2294
#define BT_REG_AFH_INSTANT_SLAVE1				0x2298
#define BT_REG_AFH_CHANNEL_MAP_SLAVE1			0x229c
#define BT_REG_AFH_CHANNEL_NUM_SLAVE1			0x22a6
#define BT_REG_HODE_INSTANT						0x22ac
#define BT_REG_HOLD_MODE_INTERVAL				0x22b0
#define BT_REG_SCO_CONNECTION_INDICATOR			0x22c0
#define BT_REG_AM_SCO							0x22c1
#define BT_REG_LOCAL_ACCESS_CODE				0x22c4
#define BT_REG_PAGE_SCAN_WINDOW					0x22cd
#define BT_REG_D_SCO							0x22d0
#define BT_REG_T_SCO							0x22d3
#define BT_REG_ROLE_SWITCH_INSTANT				0x22d4
#define BT_REG_ENCRYPTION_ENABLE				0x22d8
 #define BT_REG_ENCRYPTION_KEY_LEN				0x22e0
#define BT_REG_OP_BD_ADDR						0x22e8
#define BT_REG_OP_BD_ADDR_DSP					0x22f0
#define BT_REG_AM_TRANSMITT_POWER				0x22f8
#define BT_REG_RX_POWER_SLAVE1  				0x22fa
#define BT_REG_SECOND_LT_ADDR					0x22fc
#define BT_REG_WESCO							0x22fd
#define BT_REG_ESCO_PACKET_TYPE					0x22fe
#define BT_REG_SWITCH_SLOT_OFFSET				0x2300
#define BT_REG_DSP_TX_DISABLE					0x2304
#define BT_REG_TRANSMITT_POWER_CONTROL			0x2308
#define BT_REG_TRANSMITT_POWER_CONTROL_SLAVE1	0x230a
#define BT_REG_CURRENT_PROCESS_AM_ADDR			0x230c
#define BT_REG_AM_CONNECTION_INDICATOR			0x230d
#define BT_REG_AM_CONNECTION_SLAVE_INDICATOR	0x230e
#define BT_REG_PAGED_BD_ADDR					0x2310
#define BT_REG_AFH_INSTANT						0x231c
#define BT_REG_AFH_CHANNEL_MAP					0x2320
#define BT_REG_AFH_CHANNEL_NUM					0x232a
#define BT_REG_PREPARE_SCO_FLAG					0x232c
#define BT_REG_TEST_ITEMS						0x2330
#define BT_REG_HOPPING_MODE						0x2331
#define BT_REG_TX_FREQUENCY_DUT					0x2332
#define BT_REG_RX_FREQUENCY_DUT					0x2333
#define BT_REG_PACKET_TYPE_DUT					0x2334 /* registers overlap below */
#define BT_REG_PACKET_LEN_DUT					0x2335 /* registers overlap below */
#define BT_REG_ESCO_RX_LENGTH					0x2334
#define BT_REG_ESCO_TX_LENGTH					0x2336
#define BT_REG_DEBUG0							0x2338
#define BT_REG_SPILTH_SCO_BYTES					0x233c
#define BT_REG_AFH_INSTANT_SLAVE0				0x2340
#define BT_REG_AFH_CHANNEL_MAP_SLAVE0			0x2344
#define BT_REG_AFH_CHANNEL_NUM_SLAVE0			0x234e
#define BT_REG_ROLE_SWITCH_AMADDR				0x2350
#define BT_REG_CLOCK_INFO						0x2354
#define BT_REG_CLOCK_INFO_SLAVE0				0x2358
#define BT_REG_SLOT_OFFSET_SLAVE0				0x235c
#define BT_REG_SLOT_OFFSET_SLAVE1				0x235e
#define BT_REG_EDR_MODE         				0x2360
#define BT_REG_EDR_RATE         				0x2362
#define BT_REG_AFH_MODE							0x2364
#define BT_REG_DEVICE_TXPOWER					0x2368
#define BT_REG_ANTENNA_MODE    					0x236a
#define BT_REG_FLUSH_INDICATOR					0x236c
#define BT_REG_FLUSH_MASTER_PHY_ADDR			0x2370
#define BT_REG_FLUSH_SLAVE0_PHY_ADDR			0x2374
#define BT_REG_FLUSH_SLAVE1_PHY_ADDR			0x2378
#define BT_REG_END_REGISTER						0x237c

// Define begin and end of bluetooth register (for backup and restore registers)
#define BT_BEGIN_REGISTER						BT_REG_LOCAL_BD_ADDR
#define BT_END_REGISTER							BT_REG_END_REGISTER 

// for a extention of Blue tooth register 
#ifdef BT_FHS_PACKET_TAKE_WLAN_SPACE
#define BT_REG_FHS_FOR_INQUIRY_SCAN				0x2c40
#define BT_REG_FHS_FOR_PAGE						0x2c54
#define BT_REG_FHS_FOR_INQUIRY_RESULT			0x2c68
#define BT_REG_FHS_FOR_PAGE_SCAN				0x2c7c
#else
#define BT_REG_FHS_FOR_INQUIRY_SCAN				0x23b0
#define BT_REG_FHS_FOR_PAGE						0x23c4
#define BT_REG_FHS_FOR_INQUIRY_RESULT			0x23d8
#define BT_REG_FHS_FOR_PAGE_SCAN				0x23ec
#endif






#if 0

    //
    // Define some scratch region macro
    //

    #define BT_RX_PPTR_REG					0x2900
    #define BT_RX_CPTR_REG					0x2904
    #define BT_ACL_TX_PPTR_REG				0x2908
    #define BT_ACL_TX_CPTR_REG				0x290c
    #define BT_SCO_TX_PPTR_REG				0x2910
    #define BT_SCO_TX_CPTR_REG				0x2914

    #define BT_RX_MEM_START_ADDRESS			0x2400
    #define BT_ACL_TX_MEM_START_ADDRESS		0x2000
    #define BT_SCO_TX_MEM_START_ADDRESS     0x2c10

    #define BT_DSP_INT_STATUS_REG		    0x2938
    #define BT_DSP_INT_STATUS_CLEAR_REG     0x293c

    // For the forth region of scratch memory
    #define BT_BASEBAND_VER				0x2c00
    #define BT_SP20_READY				0x2c04


    // Scratch memory start address
    #define BT_SCRATCH_MEM_BASE_ADDR	0x2000
    // BD start address
    #define BT_BD_BASE_ADDR             0x2940
    // Rx buffer start address
    #define BT_RX_BUFFER_BASE_ADDR      0x2400
    // HostPptr offset
    #define BT_HOST_P_PTR_REG			0x2900
    // DSPCptr offset
    #define BT_DSP_C_PTR_REG			0x2904



    // For Sco queue
    // HostScoPptr offset
    #define BT_HOST_P_SCO_PTR_REG		0x2920
    // DSPScoCptr offset
    #define BT_DSP_C_SCO_PTR_REG		0x2924
    // SCO BD start address
    #define BT_BD_SCO_BASE_ADDR         0x2960



    // for the mac address in the mac region
    #define BT_MAC_ADDR					0x4008


    // Define rx buffer tail
    #define BT_RX_BUFFER_TAIL      (BT_RX_BUFFER_BASE_ADDR + MAX_RX_BUF_COUNT)

    // Define Bluetooth register
    #define BT_REG_LOCAL_BD_ADDR					0x2940
    #define BT_REG_INQUIRY_SCAN_ACCESS_CODE			0x2948
    #define BT_REG_INQUIRY_SCAN_WINDOW				0x2951
    #define BT_REG_INQUIRY_ACCESS_CODE				0x2954
    #define BT_REG_INQUIRY_LENGTH					0x295d
    #define BT_REG_PAGE_ACCESS_CODE					0x2960
    #define BT_REG_PAGE_TIMEOUT						0x2969


#define BT_REG_SLAVE1_AM_ADDR					0x21ab	
#define BT_REG_AM_ADDR							0x296c
#define BT_REG_AM_BD_ADDR						0x2974
#define BT_REG_AM_BD_ADDR0						0x2974
//   #define BT_REG_AM_CLK_OFFSET0					0x297a
#define BT_REG_AM_CLK_OFFSET_SLAVE0				0x217a
#define BT_REG_AM_BD_ADDR_MASTER				0x21bc
#define BT_REG_AM_CLK_OFFSET_MASTER				0x21c2
#define BT_REG_AM_BD_ADDR_SLAVE1				0x21c4
#define BT_REG_AM_CLK_OFFSET_SLAVE1				0x218a

//Jakio20071126: mark these unused reg define
/*
    #define BT_REG_AM_BD_ADDR1						0x297c
    #define BT_REG_AM_CLK_OFFSET1					0x2982
    #define BT_REG_AM_BD_ADDR2						0x2984
    #define BT_REG_AM_CLK_OFFSET2					0x298a
    #define BT_REG_AM_BD_ADDR3						0x298c
    #define BT_REG_AM_CLK_OFFSET3					0x2992
    #define BT_REG_AM_BD_ADDR4						0x2994
    #define BT_REG_AM_CLK_OFFSET4					0x299a
    #define BT_REG_AM_BD_ADDR5						0x299c
    #define BT_REG_AM_CLK_OFFSET5					0x29a2
    #define BT_REG_AM_BD_ADDR6						0x29a4
    #define BT_REG_AM_CLK_OFFSET6					0x29aa
    #define BT_REG_AM_BD_ADDR7						0x29ac
    #define BT_REG_AM_CLK_OFFSET7					0x29b2
*/


	
    #define BT_REG_ENCRYPTION_KEY					0x29b4
    #define BT_REG_HCI_COMMAND_INDICATOR			0x2a34
    #define BT_REG_D_SNIFF							0x2a38
    // #define BT_REG_SNIFF_ATTEMPT					0x2a7c
    #define BT_REG_SNIFF_ATTEMPT					0x2a3a
    // #define BT_REG_SNIFF_TIMEOUT					0x2a8c
    #define BT_REG_SNIFF_TIMEOUT					0x2a3c
    #define BT_REG_T_SNIFF							0x2a3e
    #define	BT_REG_SNIFF_AMADDR						0x2a40
    //#define BT_REG_CURRENT_PAGE_SCAN_REP_MODE		0x2a44

//JakioToDo:check these regs, make sure they are correct.
#define BT_REG_CURRENT_PAGE_SCAN_REP_MODE		0x2a42
#define BT_REG_ENCRYPTION_KEY_SLAVE1			0x2a44
#define BT_REG_CLOCK_INFO_SLAVE1				0x2a54
#define BT_REG_AFH_INSTANT_SLAVE1				0x2a58
#define BT_REG_AFH_CHANNEL_MAP_SLAVE1			0x2a5c
#define BT_REG_AFH_CHANNEL_NUM_SLAVE1			0x2a66
/*
#define BT_REG_CURRENT_PAGE_SCAN_REP_MODE		0x2282
#define BT_REG_ENCRYPTION_KEY_SLAVE1			0x2284
#define BT_REG_CLOCK_INFO_SLAVE1				0x2294
#define BT_REG_AFH_INSTANT_SLAVE1				0x2298
#define BT_REG_AFH_CHANNEL_MAP_SLAVE1			0x229c
#define BT_REG_AFH_CHANNEL_NUM_SLAVE1			0x22a6
*/

    #define BT_REG_HODE_INSTANT						0x2a6c
    #define BT_REG_HOLD_MODE_INTERVAL				0x2a70
    #define BT_REG_SCO_CONNECTION_INDICATOR			0x2a80
    #define BT_REG_AM_SCO							0x2a81
    #define BT_REG_LOCAL_ACCESS_CODE				0x2a84
    #define BT_REG_PAGE_SCAN_WINDOW					0x2a8d
    #define BT_REG_D_SCO							0x2a90
    #define BT_REG_T_SCO							0x2a93
    #define BT_REG_ROLE_SWITCH_INSTANT				0x2a94
    #define BT_REG_ENCRYPTION_ENABLE				0x2a98

//JakioToDo: there is no encryption_key_len reg define in pci code, so where do they store the encryption length, check it out!
    #define BT_REG_ENCRYPTION_KEY_LEN				0x2aa0
	
    #define BT_REG_OP_BD_ADDR						0x2aa8
    #define BT_REG_OP_BD_ADDR_DSP					0x2ab0
    #define BT_REG_AM_TRANSMITT_POWER				0x2ab8
//JakioToDo: check and make correct
#define BT_REG_RX_POWER_SLAVE1  				0x2aba
	
    #define BT_REG_SECOND_LT_ADDR					0x2abc
    #define BT_REG_WESCO							0x2abd
    #define BT_REG_ESCO_PACKET_TYPE					0x2abe

    #define BT_REG_SWITCH_SLOT_OFFSET				0x2ac0
    #define BT_REG_DSP_TX_DISABLE					0x2ac4
    #define BT_REG_TRANSMITT_POWER_CONTROL			0x2ac8
	
//JakioToDo: check and make correct
#define BT_REG_TRANSMITT_POWER_CONTROL_SLAVE1		0x2aca

    #define BT_REG_CURRENT_PROCESS_AM_ADDR			0x2acc
    #define BT_REG_AM_CONNECTION_INDICATOR			0x2acd
	
//JakioToDo: check and make correct
#define BT_REG_AM_CONNECTION_SLAVE_INDICATOR		0x2ace

    #define BT_REG_PAGED_BD_ADDR					0x2ad0
    //modify
    #define BT_REG_AFH_INSTANT						0x2adc
    #define BT_REG_AFH_CHANNEL_MAP					0x2ae0
    //modify
    //#define BT_REG_AFH_CHANNEL_NUM					0x2aeb
    #define BT_REG_AFH_CHANNEL_NUM					0x2aea
    #define BT_REG_PREPARE_SCO_FLAG					0x2aec
    #define BT_REG_TEST_ITEMS						0x2af0
    #define BT_REG_HOPPING_MODE						0x2af1
    #define BT_REG_TX_FREQUENCY_DUT					0x2af2
    #define BT_REG_RX_FREQUENCY_DUT					0x2af3

//JakioToDo: check and make correct
//#define BT_REG_PACKET_TYPE_DUT					0x2af4 /* registers overlap below */
//#define BT_REG_PACKET_LEN_DUT					0x2af5 /* registers overlap below */

    #define BT_REG_ESCO_RX_LENGTH					0x2af4
    #define BT_REG_ESCO_TX_LENGTH					0x2af6
    #define BT_REG_DEBUG0							0x2af8
    #define BT_REG_SPILTH_SCO_BYTES					0x2afc

    //new add
    #define BT_REG_AFH_INSTANT_SLAVE0				0x2b00
    #define BT_REG_AFH_CHANNEL_MAP_SLAVE0			0x2b04
    #define BT_REG_AFH_CHANNEL_NUM_SLAVE0			0x2b0e

    #define BT_REG_ROLE_SWITCH_AMADDR				0x2b10
    #define BT_REG_CLOCK_INFO						0x2b14
    #define BT_REG_CLOCK_INFO_SLAVE0				0x2b18
    #define BT_REG_SLOT_OFFSET_SLAVE0				0x2b1c
	
//JakioToDo: check and make correct	
#define BT_REG_SLOT_OFFSET_SLAVE1				0x2b1e

    #define BT_REG_EDR_MODE							0x2b20
//JakioToDo: check and make correct
#define BT_REG_EDR_RATE         						0x2b22
	
    #define BT_REG_AFH_MODE							0x2b24

//JakioToDo: check and make correct
//#define BT_REG_END_REGISTER						0x2b28
#define BT_REG_DEVICE_TXPOWER					0x22b8
#define BT_REG_ANTENNA_MODE    					0x22ba
#define BT_REG_FLUSH_INDICATOR					0x22bc
#define BT_REG_FLUSH_MASTER_PHY_ADDR			0x22c0
#define BT_REG_FLUSH_SLAVE0_PHY_ADDR			0x22c4
#define BT_REG_FLUSH_SLAVE1_PHY_ADDR			0x22c8
#define BT_REG_EXTENDED_INQUIRY_RESPONSE		0x22cc
#define BT_REG_END_REGISTER						0x22d0




    // Define begin and end of bluetooth register (for backup and restore registers)
    #define BT_BEGIN_REGISTER						BT_REG_LOCAL_BD_ADDR
    #define BT_END_REGISTER							BT_REG_END_REGISTER

    // for a extention of Blue tooth register
    #ifdef BT_FHS_PACKET_TAKE_WLAN_SPACE
        #define BT_REG_FHS_FOR_INQUIRY_SCAN				0x2c40
        #define BT_REG_FHS_FOR_PAGE						0x2c54
        #define BT_REG_FHS_FOR_INQUIRY_RESULT			0x2c68
        #define BT_REG_FHS_FOR_PAGE_SCAN				0x2c7c
    #else
        #define BT_REG_FHS_FOR_INQUIRY_SCAN				0x2b70
        #define BT_REG_FHS_FOR_PAGE						0x2b84
        #define BT_REG_FHS_FOR_INQUIRY_RESULT			0x2b98
        #define BT_REG_FHS_FOR_PAGE_SCAN				0x2bac
    #endif

#endif


    //
    // Define some mac register macro
    //
    #define BT_MAC_BASE_ADDR			0x4000
    #define BT_MAC_BB_REG0_3			0x4200
    #define BT_MAC_BB_REG4_7			0x4204
    #define BT_MAC_BB_REG8_11			0x4208
    #define BT_MAC_BB_REG12_15			0x420c
    #define BT_MAC_BB_REG16_19			0x4210
    #define BT_MAC_BB_REG20_23			0x4214
    #define BT_MAC_BB_REG24_27			0x4218
    #define BT_MAC_BB_REG28_31			0x421c
    #define BT_MAC_BB_REG32_35			0x4220
    #define BT_MAC_BB_REG36_38			0x4224

    //
    // We use DSPIntStatus register(offset 0x10), which lies in the third region of scratch memory
    // as our DSP-interrupt status register. So we will define some bits to express some interrupt
    // events.
    //
    #define BT_DSP_INT_TX_COMP_BIT						BIT_0
    #define BT_DSP_INT_RX_COMP_BIT						BIT_1
    #define BT_DSP_INT_INQUIRY_RESULT_EVENT				BIT_2
    #define BT_DSP_INT_INQUIRY_COMPLETE_EVENT			BIT_3
    #define BT_DSP_INT_HARDWARE_ERROR_EVENT				BIT_4
    #define BT_DSP_INT_FLUSH_OCCURED_EVENT				BIT_5
    #define BT_DSP_INT_ROLE_CHANGE_EVENT				BIT_6
    #define BT_DSP_INT_DSP_ACK							BIT_7
    #define BT_DSP_INT_DSP_MODE_CHANGE_INTO_HOLD		BIT_8
    #define BT_DSP_INT_DSP_CLOCK_READY_EVENT			BIT_9
    #define BT_DSP_INT_SCO_TX_COMP_BIT					BIT_10
    #define BT_DSP_INT_DSP_MODE_CHANGE_INTO_ACTIVE		BIT_11
    #define BT_DSP_INT_DSP_MODE_CHANGE_INTO_SNIFF		BIT_12
    #define BT_DSP_INT_ROLE_CHANGE_FAIL_EVENT			BIT_13
    #define BT_DSP_INT_REPORT_RX_POWER					BIT_14



//Jakio20081023: element offset and length in eeprom
#define EEPROM_DATA_VERSION_OFFSET				0xB0
#define EEPROM_DATA_VERSION_LENGTH				4

#define EEPROM_VALID_FLAG_OFFSET				0xB4
#define EEPROM_VALID_FLAG_LENGTH				4

#define EEPROM_BD_ADDRESS_OFFSET				0xC0
#define EEPROM_BD_ADDRESS_LENGTH			8

#define EEPROM_LICENSE_ID_OFFSET				0xD0
#define EEPROM_LICENSE_ID_LENGTH				16		//this length maybe variable, now we take it as 16bytes

#define EEPROM_TX_POWER_TABLE_OFFSET				0x180
#define EEPROM_TX_POWER_TABLE_LENGTH				(12*14)		 

#define EEPROM_OFFSET_SUBSYSTEMID    0x011A

    //
    // Define some macros related to the interrupt event or status
    //

    // Related to BT_CSR_FUNC_EVENT_REG(0x40), BT_CSR_FUNC_EVENT_MASK_REG(0x44),
    // BT_CSR_FUNC_PRESENT_STATE_REG (0x48) and BT_CSR_FUNC_FORCE_EVENT_REG (0x4c)
    #define BT_PC_CARD_STATUS_INTR_BIT		BIT_15
    #define BT_PC_CARD_STATUS_GWAKE_BIT		BIT_4
    #define BT_PC_CARD_STATUS_BVD1_BIT		BIT_3
    #define BT_PC_CARD_STATUS_BVD2_BIT		BIT_2
    #define BT_PC_CARD_STATUS_READY_BIT		BIT_1
    #define BT_PC_CARD_STATUS_WP_BIT		BIT_0

    // Related to BT_CSR_INT_ENABLE_REG (0x8), BT_CSR_STATUS_REG (0xc) and
    // BT_CSR_STATUS_CLR_REG (0x10)
    #define BT_INT_STATUS_DMA0_BUSY_BIT					BIT_0
    #define BT_INT_STATUS_DMA0_DONE_BIT					BIT_1
    #define BT_INT_STATUS_DMA0_ABORT_BIT				BIT_2
    #define BT_INT_STATUS_DMA1_ABORT_BIT				BIT_5
    #define BT_INT_STATUS_MAC_HOST_DMA0_BUSY_BIT		BIT_6
    #define BT_INT_STATUS_MAC_HOST_DMA0_DONE_BIT		BIT_7
    #define BT_INT_STATUS_MAC_HOST_DMA0_ABORT_BIT		BIT_8
    #define BT_INT_STATUS_MAC_HOST_DMA1_BUSY_BIT		BIT_9
    #define BT_INT_STATUS_MAC_HOST_DMA1_DONE_BIT		BIT_10
    #define BT_INT_STATUS_MAC_HOST_DMA1_ABORT_BIT		BIT_11
    #define BT_INT_STATUS_DS_C0_DONE_BIT				BIT_12
    #define BT_INT_STATUS_DS_C1_DONE_BIT				BIT_13
    #define BT_INT_STATUS_DS_C2_DONE_BIT				BIT_14
    #define BT_INT_STATUS_DS_C3_DONE_BIT				BIT_15
    #define BT_INT_STATUS_DS_C4_DONE_BIT				BIT_16
    #define BT_INT_STATUS_DS_C5_DONE_BIT				BIT_17
    #define BT_INT_STATUS_MASTER_ABORT_BIT				BIT_22
    #define BT_INT_STATUS_TARGET_ABORT_BIT				BIT_23
    #define BT_INT_STATUS_LT_TIMEOUT_BIT				BIT_24
    #define BT_INT_STATUS_REPORTED_PARITY_BIT			BIT_25
    #define BT_INT_STATUS_DETECTED_PARITY_BIT			BIT_26
    #define BT_INT_STATUS_SYSTEM_ERROR_BIT				BIT_27
    #define BT_INT_STATUS_MAC_RX_BIT					BIT_28
    #define BT_INT_STATUS_MAC_TX_BIT					BIT_29
    #define BT_INT_STATUS_MAILBOX_BIT					BIT_30
    #define BT_INT_STATUS_MAC_EVENT_BIT					BIT_31


    // Define some combination-bit
    #define BT_INT_STATUS_MASK_BITS		(BT_INT_STATUS_DMA0_ABORT_BIT | BT_INT_STATUS_MASTER_ABORT_BIT \
    | BT_INT_STATUS_TARGET_ABORT_BIT | BT_INT_STATUS_LT_TIMEOUT_BIT | BT_INT_STATUS_REPORTED_PARITY_BIT \
    | BT_INT_STATUS_DETECTED_PARITY_BIT | BT_INT_STATUS_SYSTEM_ERROR_BIT | BT_INT_STATUS_MAC_RX_BIT \
    | BT_INT_STATUS_MAC_TX_BIT | BT_INT_STATUS_MAILBOX_BIT | BT_INT_STATUS_MAC_EVENT_BIT \
    )

    // Related to BT_CSR_PCI_CONTROL_REG (0x0)
    #define BT_PCI_CONTROL_RESET_WLAN_CORE			BIT_0
    #define BT_PCI_CONTROL_RESET_WLAN_SUB_SYS		BIT_1
    #define BT_PCI_CONTROL_RESET_WLAN_SYS			BIT_2
    #define BT_PCI_CONTROL_RESET_PCI				BIT_3
    #define BT_PCI_CONTROL_SOFT_RESET_PCI			BIT_4
    #define BT_PCI_CONTROL_SLEEP_WLAN_CORE			BIT_8
    #define BT_PCI_CONTROL_SLEEP_WLAN_SUB_SYS		BIT_9
    #define BT_PCI_CONTROL_SLEEP_WLAN_SYS			BIT_10
    #define BT_PCI_CONTROL_SLEEP_MAC_GATED			BIT_11
    #define BT_PCI_CONTROL_SLEEP_MAC				BIT_12
    #define BT_PCI_CONTROL_SLEEP_DBG				BIT_13
    #define BT_PCI_CONTROL_DMA0_ENABLE				BIT_16
    #define BT_PCI_CONTROL_DMA1_ENABLE				BIT_17
    #define BT_PCI_CONTROL_MAC_HOST_DMA0_ENABLE		BIT_18
    #define BT_PCI_CONTROL_MAC_HOST_DMA1_ENABLE		BIT_19

    // Define some value for sp20 state
    #define BT_MMAC_CORE_NOT_RDY_RESERVED			0
    #define BT_MMAC_CORE_RDY						1
    #define BT_MMAC_CORE_START_REQ_HD				2
    #define BT_MMAC_CORE_STAGE2_ASSOC_INIT			3
    #define BT_MMAC_CORE_RXFRAG_TEST_INIT			4

	#define MAC_SIGNATURE_VALUE 0x0A0B0C0D
	#define WLS_MAC_SIGNATURE_WD             0x4000

    //
    // For shuttle bus
    //
    // SB port
    #define BT_SB_PORT_MAC							0
    #define BT_SB_PORT_UNIPHY						1
    #define BT_SB_PORT_CARDBUS						3

    // Transfer types
    #define BT_SB_TX_RESET							0
    #define BT_SB_TX_NORM							1
    #define BT_SB_TX_CHAIN							2
    #define BT_SB_TX_INF							3

    // TS sizes
    #define BT_SB_TS1								0
    #define BT_SB_TS2								1
    #define BT_SB_TS4								2
    #define BT_SB_TS8								3

    // SB Port defines
    #define BT_SB_PORT_0							0
    #define BT_SB_PORT_1							1
    #define BT_SB_PORT_2							2
    #define BT_SB_PORT_3							3
    #define BT_SB_PORT_4							4
    #define BT_SB_PORT_5							5
    #define BT_SB_PORT_6							6
    #define BT_SB_PORT_7							7
    #define BT_SB_PORT_8							8
    #define BT_SB_PORT_9							9
    #define BT_SB_PORT_10							10
    #define BT_SB_PORT_11							11
    #define BT_SB_PORT_12							12
    #define BT_SB_PORT_13							13
    #define BTSB_PORT_14							14
    #define BTSB_PORT_15							15

    // SB Priorities
    #define BT_SB_PRI0								0
    #define BT_SB_PRI1								1
    #define BT_SB_PRI2								2
    #define BT_SB_PRI3								3
    #define BT_SB_PRI4								4
    #define BT_SB_PRI5								5
    #define BT_SB_PRI6								6
    #define BT_SB_PRI7								7
    #define BT_SB_PRI8								8
    #define BT_SB_PRI9								9
    #define BT_SB_PRI10					SB_PRI10_UNDEFINED
    #define BT_SB_PRI11					SB_PRI11_UNDEFINED
    #define BT_SB_PRI12					SB_PRI12_UNDEFINED
    #define BT_SB_PRI13					SB_PRI13_UNDEFINED
    #define BT_SB_PRI14					SB_PRI14_UNDEFINED
    #define BT_SB_PRI_MAT							15


    #define BT_SPX_P_MEM							0
    #define BT_SPX_A_MEM							1
    #define BT_SPX_B_MEM							2
    #define BT_SPX_T_MEM							3
    #define BT_SPX_CS0_MEM							4
    #define BT_SPX_CS3_MEM							5
    #define BT_SPX_CS1_MEM							12
    #define BT_SPX_CS2_MEM							13
    #define BT_SPX_INST_MEM							14

    //
    // bit field for SB control
    //
    #define BT_BF_SB_WRCTL_L					0x0001
    #define BT_BF_SB_WRLNK_L					0x0002
    #define BT_BF_SB_RDCTL_L					0x0010

    //--other control definition------------------------------------
    #define BT_SB_WR_LNK_ON						1
    #define BT_SB_WR_LNK_OFF					0
    #define BT_SB_RD_CTL_ON						1
    #define BT_SB_RD_CTL_OFF					0
    #define BT_SB_RD_STP_ON						1
    #define BT_SB_RD_STP_OFF					0
    #define BT_SB_WR_CTL_ON						1
    #define BT_SB_WR_CTL_OFF					0
    #define BT_SB_WR_STP_ON						1
    #define BT_SB_WR_STP_OFF					0

    #define BT_SHUTTLE2HOST_ON					1
    #define BT_SHUTTLE2HOST_OFF					0
    #define BT_HOST2SHUTTLE_ON					1
    #define BT_HOST2SHUTTLE_OFF					0

    //---MAC HOST DMA control definition------------------------------
    #define BT_XFER_ACTIVATE_ON					1
    #define BT_XFER_ACTIVATE_OFF				0
    #define BT_HOST2TXFIFO_ON					1
    #define BT_RXFIFO2HOST_ON					0
    #define BT_BURSTSIZE_512WORD				0
    #define BT_BURSTSIZE_32WORD					1
    #define BT_BURSTSIZE_4WORD					2
    #define BT_BURSTSIZE_1WORD					3

    #define BT_SCRATCH_XFER_LENGTH				1024
    #define BT_SCRATCH_XFER_OFFSET				1024
    #define BT_SCRATCH_INJECT_ABMEM_OFFSET		580

    #define BT_PCI_STEERING_BIT              0x08000000





// Define some const macros
#define EEPROM_IN_SCRATCH_SIZE				1024
#define EEPROM_VERSION_SIZE					2
#define BT_IVT_KEY_LENGTH					16

// Define Eeprom index
#define BT_EEPROM_INDEX_BD_ADDR								838
#define BT_EEPROM_INDEX_IVT_KEY								844
#define EEPROM_TXPWR_TABLE_LEN								576






    //
    // Define some pci configuration space register offset
    //
    #define BT_PCI_CONFIG_VENDOR_ID			0x0
    #define BT_PCI_CONFIG_DEVICE_ID			0x2
    #define BT_PCI_CONFIG_COMMAND			0x4
    #define BT_PCI_CONFIG_STATUS			0x6
    #define BT_PCI_CONFIG_REV_ID			0x8
    #define BT_PCI_CONFIG_CLASS_CODE		0x9
    #define BT_PCI_CONFIG_LATENCY_TIMER		0xc
    #define BT_PCI_CONFIG_HEADER_TYPE		0xe

    //
    // Define hci command indicator table
    //
    #define BT_HCI_COMMAND_INDICATOR_INQUIRY				0x1
    #define BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL			0x2
    #define BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION		0x3
    #define BT_HCI_COMMAND_INDICATOR_DISCONNECT				0x4
    #define BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN			0x5
    #define BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION	0x6
    #define BT_HCI_COMMAND_INDICATOR_CHANGE_CONN_KEY		0x7
    #define BT_HCI_COMMAND_INDICATOR_MASTER_KEY				0x8
    #define BT_HCI_COMMAND_INDICATOR_HOLD_MODE				0x9
    #define BT_HCI_COMMAND_INDICATOR_SNIFF_MODE				0xa
    #define BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE		0xb
    #define BT_HCI_COMMAND_INDICATOR_PARK_MODE				0xc
    #define BT_HCI_COMMAND_INDICATOR_EXIT_PARK_MODE			0xd
    #define BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE			0xe
    #define BT_HCI_COMMAND_INDICATOR_RESET					0xf
    #define BT_HCI_COMMAND_INDICATOR_FLUSH					0x10
    // #define BT_HCI_COMMAND_INDICATOR_WRITE_SCAN_EN		0x11
    #define BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN	0x11
    #define BT_HCI_COMMAND_INDICATOR_WRITE_VOICE_SETTING	0x12
    #define BT_HCI_COMMAND_INDICATOR_WRITE_HOLD_MODE_AC		0x13
    #define BT_HCI_COMMAND_INDICATOR_WRITE_LOOPBACK_MODE	0x14
    #define BT_HCI_COMMAND_INDICATOR_ROLE_SWITCH_PREPARE	0x15
    #define BT_HCI_COMMAND_INDICATOR_HODE_MODE_PREPARE		0x16
    #define BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_PREPARE		0x17
    #define BT_HCI_COMMAND_INDICATOR_PARK_MODE_PREPARE		0x18
    #define BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE		0x19
    #define BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN		0x1a

    //
    // Define hci command indicator table by bits
    //
    #define BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT				BIT_0
    #define BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT			BIT_1
    #define BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT		BIT_2
    #define BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT				BIT_3
    #define BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT			BIT_4
    #define BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT	BIT_5
    #define BT_HCI_COMMAND_INDICATOR_CHANGE_CONN_KEY_BIT		BIT_6
    #define BT_HCI_COMMAND_INDICATOR_MASTER_KEY_BIT				BIT_7
    #define BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT				BIT_8
    #define BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_BIT				BIT_9
    #define BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT		BIT_10
    #define BT_HCI_COMMAND_INDICATOR_PARK_MODE_BIT				BIT_11
    #define BT_HCI_COMMAND_INDICATOR_EXIT_PARK_MODE_BIT			BIT_12
    #define BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT			BIT_13
    #define BT_HCI_COMMAND_INDICATOR_RESET_BIT					BIT_14
    #define BT_HCI_COMMAND_INDICATOR_FLUSH_BIT					BIT_15
    // #define BT_HCI_COMMAND_INDICATOR_WRITE_SCAN_EN_BIT		BIT_16

	//jakio20071024 modify here
	#define BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT	BIT_16
#define BT_HCI_COMMAND_INDICATOR_CHANGE_SLAVE1_AFH_MODE_BIT	BIT_17
#define BT_HCI_COMMAND_INDICATOR_WRITE_HOLD_MODE_AC_BIT		BIT_18
#define BT_HCI_COMMAND_INDICATOR_WRITE_LOOPBACK_MODE_BIT	BIT_19
#define BT_HCI_COMMAND_INDICATOR_TRANSMIT_POWER_BIT			BIT_20
#define BT_HCI_COMMAND_INDICATOR_CHANGE_SCO_CONNECTION_BIT	BIT_21
#define BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT			BIT_22
#define BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE_BIT		BIT_23
#define BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT		BIT_24
#define BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT				BIT_25
#define BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT		BIT_26
#define BT_HCI_COMMAND_INDICATOR_TEST_MODE					BIT_27
#define BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH			BIT_28
#define BT_HCI_COMMAND_INDICATOR_PACKET_TYPE_EDR_MODE		BIT_29
#define BT_HCI_COMMAND_INDICATOR_CHANGE_SLAVE0_AFH_MODE_BIT	BIT_30


/*	
    #define BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT	BIT_16
    #define BT_HCI_COMMAND_INDICATOR_WRITE_VOICE_SETTING_BIT	BIT_17
    #define BT_HCI_COMMAND_INDICATOR_WRITE_HOLD_MODE_AC_BIT		BIT_18
    #define BT_HCI_COMMAND_INDICATOR_WRITE_LOOPBACK_MODE_BIT	BIT_19
    #define BT_HCI_COMMAND_INDICATOR_TRANSMIT_POWER_BIT			BIT_20
    #define BT_HCI_COMMAND_INDICATOR_CHANGE_SCO_CONNECTION_BIT	BIT_21
    #define BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT			BIT_22
    #define BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE_BIT		BIT_23
    #define BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT		BIT_24
    #define BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT				BIT_25
    #define BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT		BIT_26
    #define BT_HCI_COMMAND_INDICATOR_TEST_MODE					BIT_27
    #define BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH			BIT_28
*/
    //
    // We define the interfaces that can access the hardware resource and so no.
    //

    // read mapped register to 8051
    //#define BT_USB_READ_BYTES(devExt,reg,buf,count) BtUsbRead3DSPRegs(devExt,buf,count,reg)
    //#define BT_USB_READ_WORDS(devExt,reg,buf,count) BtUsbRead3DSPRegs(devExt,buf,sizeof(UINT16)*count,reg)
    //#define BT_USB_READ_DWORDS(devExt,reg,buf,count)(devExt,reg,(UINT8)sizeof(UINT32)*count,buf)


    // write mapped register to 8051
    //#define BT_USB_WRITE_BYTES(devExt,reg,buf,count) BtUsbWrite3DSPRegs(devExt,buf,count,reg)
    //#define BT_USB_WRITE_DWORDS(devExt,reg,buf,count) BtUsbWrite3DSPRegs(devExt,buf,sizeof(UINT32)*count,reg)


    // Interrupt operation
    #define BT_INT_ENABLE(devExt) \
    { \
    BtWriteToReg(devExt,BT_CSR_INT_ENABLE_REG,BT_INT_STATUS_MASK_BITS);	\
    BtWriteToReg(devExt,BT_CSR_FUNC_EVENT_MASK_REG,BT_PC_CARD_STATUS_INTR_BIT);	/* enable cint */ \
    }

    #define BT_INT_DISABLE(devExt) \
    { \
    BtWriteToReg(devExt,BT_CSR_FUNC_EVENT_REG,0xffffffff);	/* disable cint */ \
    BtWriteToReg(devExt,BT_CSR_FUNC_EVENT_MASK_REG,0);	/* disable cint */ \
    BtWriteToReg(devExt,BT_CSR_INT_ENABLE_REG,0);	/* disable machw.interrupt_n */ \
    }

    #define BT_AM_CONNECTION_IND_MODE_ADD		1
    #define BT_AM_CONNECTION_IND_MODE_DEL		2
    #define BT_AM_CONNECTION_IND_MODE_CLR		3




    #ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY

        #define BT_WRITE_COMMAND_INDICATOR_SAFE(devExt, value, pConnectDevice)	\
        {	\
        PCONNECT_DEVICE_T  pConnectDevice1;	\
        UINT8 findflag = 0;	\
        PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;	\
        pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);	\
        while (pConnectDevice1 != NULL)	\
        {	\
        if (pConnectDevice1 != pConnectDevice)	\
        {	\
        findflag = 1;	\
        break;	\
        }	\
        pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);	\
        }	\
        if (findflag == 0)	\
        {	\
        pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);	\
        while (pConnectDevice1 != NULL)	\
        {	\
        if (pConnectDevice1 != pConnectDevice)	\
        {	\
        findflag = 1;	\
        break;	\
        }	\
        pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);	\
        }	\
        }	\
        if (findflag == 0)	\
        {	\
        BtUsbWriteCommandIndicatorSafe(devExt, value); \
        }	\
        }

    #endif


    #define BT_CARDBUS_WRITE_CONTROL_REG(devExt, state)	\
    	{	\
    	BtWriteToReg(devExt,BT_CSR_PCI_CONTROL_REG,state);	\
    	}

    #define BT_CARDBUS_READ_CONTROL_REG(devExt)	BtReadFromReg(devExt, BT_CSR_PCI_CONTROL_REG)

    #define BT_SB_CONTROL(NoOfBytes, Priority, BurstSize, TxType, WriteLink, ReadCtl, ReadStop, WriteCtl, WriteStop, HiPerf,shuttle2host,host2shuttle) \
    ((Priority << 28) | (HiPerf << 26) | (host2shuttle << 25) | (shuttle2host << 24) | (BurstSize << 21) | \
    (((NoOfBytes-1) & (0x3fff)) << 7) | (TxType << 2) | (WriteLink << 1) | (ReadCtl << 4) | \
    (ReadStop << 6) | (WriteCtl << 0) | (WriteStop << 5))

    #define BT_SB_ADDRESS(PortNo, Address) ((PortNo << 28) | (Address << 0))

    VOID BtBackupBlueToothRegs(PBT_DEVICE_EXT devExt);
    NTSTATUS BtRestoreBlueToothRegs(PBT_DEVICE_EXT devExt);
	NTSTATUS BtInitBlueToothRegs(PBT_DEVICE_EXT devExt);

	BOOLEAN BtIsPluggedIn(PBT_DEVICE_EXT devExt);



    NTSTATUS BtTransmitFWFragment(PBT_DEVICE_EXT devExt, PUINT8 pbuff, UINT32 len);
    /*
    NTSTATUS BtTransmitDSPFwHead(
    PBT_DEVICE_EXT devExt,
    UINT16			 offset,
    UINT32 		        len,
    UINT8 			 file_id
    );
     */
#endif
