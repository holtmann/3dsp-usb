#ifndef BT_TEST_H
#define BT_TEST_H

/***********************************************************************
 *
 * FILENAME:    BT_Test.h      CURRENT VERSION: 1.00.01
 * CREATE DATE: 2005/08/15
 * PURPOSE:      mainly define the structrue that store the information
 *               about test module
 *
 * AUTHORS:      jason dong
 *
 *
 **********************************************************************/

/*
 * HISTORY OF CHANGES
 *
2005.9.20   Add some mebers for testing data from or to socket directly (passby hardware) in
structure _BT_TEST

typedef struct _BT_TEST
{
...
UINT8  buf[512];
UINT32 len;
BOOLEAN datavalid;
...
}

2005.9.20   Add some functions for testing data from or to socket directly (passby hardware).
Test_RecvFromSockDirectly
Test_GetRxFrameType
Test_TransferRxFrameToRxCach
Test_ProcessRxNULLFrame
Test_ProcessRxPoolFrame
Test_TransferRxFrameToRxLMPPDUCach
Test_CopyTxBufAndNotifyApp
2005.10.21   Add a structure for receiving data from device and sending it to application directily.
typedef struct _BT_TEST_RX_BLOCK
{
UINT32 len;
UINT8 buf[512];
} BT_TEST_RX_BLOCK_T, *PBT_TEST_RX_BLOCK_T;
Add some members for receiving data from device and sending it to application directily in
structure _BT_TEST

typedef struct _BT_TEST
{
HANDLE hRxEvent;
PRKEVENT  RxEvent;
BT_TEST_RX_BLOCK_T RxBlock[16];
UINT8 curIndex;
UINT8 lastIndex;
// Spin lock for task module
KSPIN_LOCK     lock;
} BT_TEST_T, *PBT_TEST_T;

Add some functions for receiving data from device and sending it to application directily.
Test_GetNextIndex
Test_GetRecvData
2005.10.24 Add some members in struct BT_TEST_T to define some events which is used to notify
application receiving the data and dispalying it.
typedef struct _BT_TEST
{
...
HANDLE hRxDispEvent;
PRKEVENT  RxDispEvent;
...
}
Add some function for displaying the received data.
Test_InitShareEventForDisplay
Test_ClrEventForDisplay
2006.3.7  Add some members in struct BT_TEST_T for supporting record-vox-2-file and play-vox-from-file function.
typedef struct _BT_TEST
{
...
#ifdef BT_RECORD_VOX_TO_FILE
UINT8 recordflag;
PUINT8 mem;
UINT32 mempos;
#endif
}
Add some functions for supporting record-vox-2-file and play-vox-from-file function.
Test_StartRecordVoxFile
Test_StoptRecordVoxFile
Test_RecordVox2File
Test_ReadVoxMemCount
Test_ReadVoxMemBuf
Test_WriteVoxMemBuf
Test_PlayVoxFromFile
2006.3.28  Add some members in struct BT_TEST_T for supporting record-vox-2-filefunction.
typedef struct _BT_TEST
{
...
PUINT8 mem1;
UINT32 mempos1;
...
}
Add some functions for supporting record-vox-2-file function.
Test_RecordVox2FileAid
Test_ReadVoxMemCountAid
Test_ReadVoxMemBufAid
 */

/*--macros------------------------------------------------------------*/

// For the tx and rx temp buffer address
#define BT_TEST_TX_BUF				0x2c00
#define BT_TEST_RX_BUF				0x2e00

// For notify tx frame
#define BT_DSP_INT_TEST_TX			BIT_8
#define BT_DSP_INT_TEST_RX			BIT_9

// For Tx infor register
#define BT_TEST_TX_INFO_REG			0x2914

// For Rx infor register
#define BT_TEST_RX_INFO_REG			0x2918

#define NIC_RESET_ALL_BUT_HARDPCI(_devExt, _value) { \
_value = BtReadFromReg(devExt, BT_CSR_PCI_CONTROL_REG) | BT_PCI_CONTROL_RESET_WLAN_CORE |\
BT_PCI_CONTROL_RESET_WLAN_SUB_SYS | BT_PCI_CONTROL_RESET_WLAN_SYS | BT_PCI_CONTROL_SOFT_RESET_PCI |\
BT_PCI_CONTROL_SLEEP_MAC; \
BtWriteToReg(devExt, BT_CSR_PCI_CONTROL_REG, _value);}

#define NIC_RELEASE_CORE(_devExt, _value) { \
_value = BtReadFromReg(devExt, BT_CSR_PCI_CONTROL_REG) & (~BT_PCI_CONTROL_RESET_WLAN_CORE); \
BtWriteToReg(devExt, BT_CSR_PCI_CONTROL_REG, _value); }

#ifdef BT_RECORD_VOX_TO_FILE
#define MAX_VOX_MEM_LEN		2*1024*1024
#endif
/*--constants and types------------------------------------------------*/

#pragma pack(1)

typedef struct _BT_TEST_RX_BLOCK
{
	UINT32 len;
	UINT8 buf[512];
} BT_TEST_RX_BLOCK_T,  *PBT_TEST_RX_BLOCK_T;

//
// This structure contains the information about test module
//
//
typedef struct _BT_TEST
{
	BT_TEST_RX_BLOCK_T RxBlock[16];
	UINT8 curIndex;
	UINT8 lastIndex;
	// Spin lock for task module
	KSPIN_LOCK lock;
} BT_TEST_T,  *PBT_TEST_T;


typedef struct _BT_TEST_HEADER
{
	UINT32 Reserved0: 7;
	UINT32 Length: 10;
	UINT32 Reserved1: 15;
} BT_TEST_HEADER,  *PBT_TEST_HEADER;

#pragma pack()

/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/
NTSTATUS Test_Init(PBT_DEVICE_EXT devExt);
NTSTATUS Test_Release(PBT_DEVICE_EXT devExt);
NTSTATUS Test_SendTestData(PBT_DEVICE_EXT devExt, PUINT8 pLmpPdu, UINT8 PduLength);
NTSTATUS Test_SendPagePoll(PBT_DEVICE_EXT devExt);
NTSTATUS Test_SendBBAck(PBT_DEVICE_EXT devExt);
VOID Test_RecvFromSockDirectly(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
BT_RX_FRAME_TYPE Test_GetRxFrameType(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
VOID Test_TransferRxFrameToRxCach(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len, PUINT8 destbuf);
VOID Test_ProcessRxNULLFrame(PBT_DEVICE_EXT devExt, PUINT8 buf);
VOID Test_ProcessRxPoolFrame(PBT_DEVICE_EXT devExt, PUINT8 buf);
VOID Test_TransferRxFrameToRxLMPPDUCach(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
VOID Test_CopyTxBufAndNotifyApp(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
NTSTATUS Test_LoadOneFileToDSP(PBT_DEVICE_EXT devExt, UINT8 memport, PCSZ filename);
NTSTATUS Test_LoadFilesToDSP(PBT_DEVICE_EXT devExt);

UINT8 Test_GetNextIndex(UINT8 index);
UINT32 Test_GetRecvData(PBT_DEVICE_EXT devExt, PUINT8 buf);


VOID Test_StartRecordVoxFile(PBT_DEVICE_EXT devExt);
VOID Test_StoptRecordVoxFile(PBT_DEVICE_EXT devExt);
VOID Test_RecordVox2File(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT16 len);
VOID Test_RecordVox2FileAid(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT16 len);
UINT32 Test_ReadVoxMemCount(PBT_DEVICE_EXT devExt);
VOID Test_ReadVoxMemBuf(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
VOID Test_WriteVoxMemBuf(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);

#endif
