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

#ifndef __BT_IOCTL_H
    #define __BT_IOCTL_H

    //
    // We define most macros that related to IOCTL codes which is used to communicate
    // between application and driver.
    //


#include "base_types.h"

#define TEST_TYPE 40000

    // Read register or value from scratch memory
    #define IOCTL_3DSP_BT_READ_SCRATCH       CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
    // Write register or value to scratch memory
    #define IOCTL_3DSP_BT_WRITE_SCRATCH      CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
    // Read common register (including CSR and so on)
    #define IOCTL_3DSP_BT_READ_REGISTER      CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)
    // Write common register (including CSR and so on)
    #define IOCTL_3DSP_BT_WRITE_REGISTER     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
    // Start download dsp codes
    #define IOCTL_3DSP_BT_DOWNLOAD_DSPCODES    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x850,METHOD_BUFFERED,FILE_ANY_ACCESS)
    // Download firmware and start running device
    #define IOCTL_3DSP_BT_DOWNLOAD_FIRMWARE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x851,METHOD_BUFFERED,FILE_ANY_ACCESS)



//registers
#define IOCTL_TEST_READ_REGISTERS  \
	CTL_CODE( TEST_TYPE, 0x1023, METHOD_BUFFERED, FILE_ANY_ACCESS)
//registers
#define IOCTL_TEST_WRITE_REGISTERS  \
	CTL_CODE( TEST_TYPE, 0x1024, METHOD_BUFFERED, FILE_ANY_ACCESS)

//Jakio20071031: add for test
//read dsp registers
#define IOCTL_TEST_READ_DSP_REGISTERS  \
	CTL_CODE( TEST_TYPE, 0x1025, METHOD_BUFFERED, FILE_ANY_ACCESS)
	//write dsp registers
#define IOCTL_TEST_WRITE_DSP_REGISTERS  \
	CTL_CODE( TEST_TYPE, 0x1026, METHOD_BUFFERED, FILE_ANY_ACCESS)
//Read Program Memory.
#define IOCTL_TEST_PROGRAMMEMORY \
	CTL_CODE( TEST_TYPE, 0x1501, METHOD_BUFFERED, FILE_ANY_ACCESS)

//Start join, we use this code to bulkout without setting DMA
#define IOCTL_START_JOIN_TEST  \
CTL_CODE( TEST_TYPE, 0x1031, METHOD_BUFFERED, FILE_ANY_ACCESS) 


//for wb
#define IOCTL_WB_GET_DRIVER_VERSION \
	CTL_CODE( TEST_TYPE, 0x1600, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WB_GET_8051_VERSION \
	CTL_CODE( TEST_TYPE, 0x1601, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WB_GET_DSP_VERSION \
	CTL_CODE( TEST_TYPE, 0x1602, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTRL_GETANDSETFW_DRIVERSTATUS \
	CTL_CODE( TEST_TYPE, 0x1603, METHOD_BUFFERED, FILE_ANY_ACCESS )
//Jakio20080623:
#define IOCTRL_GOTO_D3_STATE \
	CTL_CODE( TEST_TYPE, 0x1604, METHOD_BUFFERED, FILE_ANY_ACCESS )
//Jakio20080805:
#define IOCTRL_GET_BDADDRESS \
	CTL_CODE( TEST_TYPE, 0x1605, METHOD_BUFFERED, FILE_ANY_ACCESS )


    // Validate if this device is 3dsp's bluetooth device
    #define IOCTL_3DSP_BT_VALIDATE_COM     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)

    // The following codes is just for testing and they should be deleted later
    #define IOCTL_3DSP_BT_CREATE_CONNECTION     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_DEL_CONNECTION_BY_BD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_DEL_CONNECTION_BY_AM     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_DEL_CONNECTION_BY_HANDLE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_PRINT_AM_LIST     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_TIMER     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_TASK     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80b,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_POLL_TIMER     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80c,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_GET_EVENT_HANDLE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80d,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_GET_RX_DATA    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80e,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_TX_DATA    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80f,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_CLEAR_EVENT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x810,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_EVENT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x811,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SEND_DATA    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x812,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_PAGE_POLL    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x813,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_SLAVE_REGS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x814,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_BB_ACK    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x815,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_DISCONNECT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x816,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_MASTER_REGS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x817,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_RESET    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x818,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_GET_LOCAL_INFO    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x819,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_RECV_DATA    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81a,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_GET_DISPLAY_EVENT_HANDLE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81b,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_CLEAR_DISPLAY_EVENT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81c,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_ENCRYP_FLAG    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81d,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_CLR_ENCRYP_FLAG    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81e,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_CHANGE_SCAN_ENABLE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x81f,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_DATA_COUNTS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x820,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_START_RECORD       CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x821,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_STOP_RECORD        CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x822,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_MEM_COUNT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x823,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_MEM_BUFFER    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x824,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_WRITE_MEM_COUNT    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x825,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_WRITE_MEM_BUFFER    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x826,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_MEM_COUNT_AID    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x827,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_MEM_BUFFER_AID    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x828,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_READ_BD_ADDRESS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x829,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_TEST_SET_IVT_KEY    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x82a,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_START_TESTER_DATA    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x82b,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_QUERY_TEST_MODE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x82c,METHOD_BUFFERED,FILE_ANY_ACCESS)
    #define IOCTL_3DSP_BT_SET_TEST_MODE    CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x82d,METHOD_BUFFERED,FILE_ANY_ACCESS)

//jakio20071114: add for power save
#define IOCTL_BT_NORMAL_SET_POWER  CTL_CODE( TEST_TYPE, 0x1029, METHOD_BUFFERED, FILE_ANY_ACCESS)

    //blue r&W
    //#define TEST_TYPE 40000
    #define IOCTL_TEST_BT_READ_WRITE  \
    CTL_CODE( TEST_TYPE, 0x1028, METHOD_BUFFERED, FILE_ANY_ACCESS)

    // Validate sentence
    #define VALIDATE_SENTENCE          "Welcome to use the blue tooth device of 3DSP"

    #pragma pack(1)

    // Read/Write command input buffer struct
    typedef struct INPUTBUF
    {
        UINT32 offset;
        UINT32 size;
    } INPUTBUF_T,  *PINPUTBUF_T;

    // Read command output buffer struct
    typedef struct OUTPUTBUF
    {
        UINT32 counts;
        UINT8 data[4096];
    } OUTPUTBUF_T,  *POUTPUTBUF_T;

    // Read/Write command extent input buffer struct
    typedef struct INPUTBUFEXT
    {
        UINT32 startoffset;
        UINT32 size;
        UINT8 buf[4096];
    } INPUTBUFEXT_T,  *PINPUTBUFEXT_T;

typedef struct _DSP_VERSION_STR
{
	UINT16   len;
	UINT8    ver[1];
}DSP_VERSION_STR, *PDSP_VERSION_STR;

    #pragma pack()

#endif