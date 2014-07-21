
#ifndef BT_TESTMODE_H
	#define BT_TESTMODE_H

/***********************************************************************
  * FILENAME:         BT_TestModule.h
  * CURRENT VERSION:  1.00.01
  * CREATE DATE:      2006/06/18
  * PURPOSE:  mainly define the structrue that store the information
  *           about LMP test module and the external function.
  * 
  * AUTHORS:  Lewis Wang
  *
  * NOTES:    //
 ***********************************************************************/

/*
 *  REVISION HISTORY
 */

#include "bt_sw.h"     /* include <WDM.H> and PBT_DEVICE_EXT structure */
#include "bt_hci.h"

/*--macros-------------------------------------------------------------*/

#define  DATA_WHITENING     0
#define  DATA_NO_WHITENING  1

#define  MAX_TEST_DATA_BUF  1040 /* unit: byte */

#define  TESTMODE_POOL_TAG  (UINT32) 'suTM'

#define RETRY_COUNT_LEFT_SHIFT_BIT_TESTMODE 11

/*--constants and types------------------------------------------------*/
typedef struct _DUT_CONFIG 
{
	UINT8 test_scenario;
	UINT8 hopping_mode;
	UINT8 TX_frequency;
	UINT8 RX_frequency;
	UINT8 power_control_mode;
	UINT8 poll_period;
	UINT8 packet_type;
	UINT8 data_length_L;
	UINT8 data_length_H;
} DUT_CONFIG, *PDUT_CONFIG;

typedef struct _BT_TESTMODE 
{
	KSPIN_LOCK         lock;       /* Spin lock for test module */
	
	PCONNECT_DEVICE_T  pDevice;    /* indicate the connection of test module */
	
	DUT_CONFIG         configuration;
	UINT8              old_whitening;
	UINT8              present_whitening;
	UINT8              being_test; /* 0: default; 1: enter transmitter testing; 2: exit testing */

	UINT8              role;       /* 0: default; 1: tester; 2: DUT */
	
	/* for transmitter test */
	UINT8              PacketType;
	UINT8              PaylaodHeader;
	UINT16             PacketLength;
	UINT8              PacketData[MAX_TEST_DATA_BUF];
} BT_TESTMODE_T, *PBT_TESTMODE_T;

typedef struct _TESTMODE_WORKITEM_CONTEXT 
{
	PBT_DEVICE_EXT  devExt;
	PIO_WORKITEM    pitem;
} TESTMODE_WORKITEM_CONTEXT, *PTESTMODE_WORKITEM_CONTEXT;



/*--variables----------------------------------------------------------*/

/*--function prototypes------------------------------------------------*/

extern NTSTATUS TestMode_Init(PBT_DEVICE_EXT devExt);
extern NTSTATUS TestMode_Release(PBT_DEVICE_EXT devExt);
extern void     TestMode_ResetMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

extern NTSTATUS TestMode_Start_Tester_Data(PBT_DEVICE_EXT devExt, PUINT8 para, UINT32 length);
extern UINT32 TestMode_TransmitData(PBT_DEVICE_EXT devExt);
extern NTSTATUS TestMode_FlushFIFO(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
extern UINT16   TestMode_GetWhitening(PBT_DEVICE_EXT devExt, PUINT8 pLmpPdu);
extern NTSTATUS TestMode_WriteCommandIndicator(PBT_DEVICE_EXT devExt);

#endif
/*--end of file--------------------------------------------------------*/
