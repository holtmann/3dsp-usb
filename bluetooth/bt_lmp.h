#ifndef BT_LMP_H
	#define BT_LMP_H

/***********************************************************************
 * FILENAME:         BT_Lmp.h
 * CURRENT VERSION:  1.00.01
 * CREATE DATE:      2005/08/16
 * PURPOSE:  mainly define the structrue that store the information
 *           about LMP module and the external function.
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
#include "bt_queue.h"

/*--macros-------------------------------------------------------------*/

#define LMP_VERSION_V10       0
#define LMP_VERSION_V11       1
#define LMP_VERSION_V12       2
#define LMP_VERSION_V20       3
#define LMP_VERSION_V21       4

#define MIN_PDU_LENGTH               1
#define MAX_PDU_LENGTH               17
#define MAX_NORMAL_OPCODE            60
#define ESCAPE_OPCODE1               124
#define ESCAPE_OPCODE2               125
#define ESCAPE_OPCODE3               126
#define ESCAPE_OPCODE4               127
#define ESCAPE_OPCODE4_OFFSET        65
#define MAX_PDU_TX_QUEUE             64
#define MAX_T_POLL_SLOTS             50
#define MAX_SWITCH_INSTANT_OFFSET    600 //500    /* unit: slot;  CSR default value 1d8/2 = 236 */

#define MASTER_TRANSACTION_ID 0
#define SLAVE_TRANSACTION_ID  1

/* pConnectDevice->lmp_states value */
#define LMP_CONN_IDLE                 0       /* also for pConnectDevice->lmp_ext_states */
#define LMP_CONN_SETUP                1       /* also for pConnectDevice->lmp_ext_states */
#define LMP_NO_LINKKEY                2
#define LMP_NO_LINKKEY_REC_AURAND     3
#define LMP_REC_AURAND_NO_LINKKEY     4
#define LMP_SEND_AURAND               5
#define LMP_PIN_REQ                   6
#define LMP_SEND_INRAND               7
#define LMP_PIN_REQ_REC_INRAND        8
#define LMP_WAIT_COM_KEY              9
#define LMP_SEND_ENMODE_REQ           10
#define LMP_WAIT_ENKEYSIZE_REQ        11
#define LMP_WAIT_STOPEN_REQ           12
#define LMP_WAIT_STARTEN_REQ          13
#define LMP_SEND_ENKEYSIZE_REQ        14
#define LMP_SEND_STARTEN_REQ          15
#define LMP_SEND_STOPTEN_REQ          16
#define LMP_SEND_PAUSEENCRYPTION_REQ  17
#define LMP_SEND_CONN_REQ             0x8000  /* this two CONN_REQ indicate whether ACL */
#define LMP_RECV_CONN_REQ             0x4000  /* link has been connected */
#define LMP_SEND_SETUP_COMPLETE       0x2000
#define LMP_RECV_SETUP_COMPLETE       0x1000
#define LMP_AU_COLLISION_OR_SEND_SRES 0x0800
#define LMP_MUTUAL_AU                 0x0400
#define LMP_RECV_SRES_OR_SEND_SRES    0x0200
#define LMP_AU_ENCRYPTION_COLLISION   0x0100
#define LMP_STATE_ANDVALUE            0x00FF  /* also for pConnectDevice->lmp_ext_states */

/* pConnectDevice->lmp_ext_states value */
#define LMP_SEND_SCO_LINK_REQ          2
#define LMP_RECV_SCO_LINK_REQ          3
#define LMP_SEND_REMOVE_SCO_LINK_REQ   4
#define LMP_SEND_ESCO_LINK_REQ         5
#define LMP_RECV_ESCO_LINK_REQ         6
#define LMP_SEND_REMOVE_ESCO_LINK_REQ  7
#define LMP_SEND_HOLD_REQ              8
#define LMP_SEND_SNIFF_REQ             9
#define LMP_SEND_UNSNIFF_REQ           10
//Jakio20071025: add here for EDR
#define LMP_SEND_MAX_SLOT_REQ          11
#define LMP_SEND_TEST_ACTIVATE         12
#define LMP_SEND_TEST_CONTROL          13
#define LMP_SEND_PACKET_TYPE_TABLE_REQ 14
#define LMP_HOLD_ACCEPTED              0x8000  /* indicates whether force_hold can be done */
#define LMP_SEND_SWITCH_REQ            0x4000
#define LMP_SEND_QOS_REQ               0x2000
#define LMP_SEND_UNSNIFF_DETACH        0x1000  /* firstly send unsniff when detach */

/* LMP PDU timer type */
#define PDU_TIMER_IDLE                       0
#define PDU_TIMER_SENT_CLKOFFSET_REQ         1
#define PDU_TIMER_SENT_VER_REQ               2
#define PDU_TIMER_SENT_FEATURES_REQ          3
#define PDU_TIMER_SENT_NAME_REQ              4
#define PDU_TIMER_SENT_AURAND                5
#define PDU_TIMER_SENT_INRAND                6
#define PDU_TIMER_SENT_COMB_KEY              7
#define PDU_TIMER_SENT_ENMODE_REQ            8
#define PDU_TIMER_SENT_ENKEYSIZE_REQ         9
#define PDU_TIMER_SENT_STARTEN_REQ           10
#define PDU_TIMER_SENT_STOPTEN_REQ           11
#define PDU_TIMER_SENT_HOLD_REQ              12
#define PDU_TIMER_SENT_SNIFF_REQ             13
#define PDU_TIMER_SENT_UNSNIFF_REQ           14
#define PDU_TIMER_SENT_SCOLINK_REQ           15
#define PDU_TIMER_SENT_REMOVE_SCOLINK_REQ    16
#define PDU_TIMER_SENT_ESCOLINK_REQ          17
#define PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ   18
#define PDU_TIMER_SENT_CONN_REQ              19
#define PDU_TIMER_SENT_SETUP_COMPLETE        20
//Jakio20071025: add here for EDR
#define PDU_TIMER_SENT_MAX_SLOT_REQ          21
#define PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ 22
#define PDU_TIMER_SENT_FEATURES_REQ_EXT      23
#define PDU_TIMER_SENT_PAUSEENCRYPTION_REQ   24
#define PDU_TIMER_SENT_RESUMEENCRYPTION_REQ  25

/* clock flag */
#define LMP_CLOCK_READY_FLAG_IDLE          0
#define LMP_CLOCK_READY_FLAG_SCO           0x0001
#define LMP_CLOCK_READY_FLAG_ESCO          0x0002
#define LMP_CLOCK_READY_FLAG_MAX_VALUE     0x0002

#define LMP_MAX_RESPONSE_TIME              30 /* unit: second */
#define LMP_NAME_REQUEST_TIME              10 /* jakio20070726 */

/* Link Key Type */
#define LINK_KEY_COMBINATION                 0
#define LINK_KEY_LOCALUNIT                   1
#define LINK_KEY_REMOTEUNIT                  2
#define LINK_KEY_DEBUG_COMBINATION           3
#define LINK_KEY_UNAUTHENTICATED_COMBINATION 4
#define LINK_KEY_AUTHENTICATED_COMBINATION   5
#define LINK_KEY_CHANGED_COMBINATION         6

/* PIN Type */
#define PIN_VARIABLE            0
#define PIN_FIXED               1

/* SCO */
#define SCO_MAX_NUMBER          1
#define SCO_MAX_AIRMODE_PARA    4
#define SCO_MAX_PACKET_PARA     3
#define SCO_ADD_LINK            1
#define SCO_CHANGE_LINK         2
#define SCO_DEFAULT_D           0
#define SCO_DEFAULT_PACKET      2 /* 0: HV1; 1: HV2; 2: HV3 */
#define SCO_DEFAULT_AIRMODE     2 /* 0: ¦Ì-law; 1: A-law; 2: CVSD; 3: transparent data */

/* eSCO */
#define ESCO_ADD_LINK                  1
#define ESCO_CHANGE_LINK               2 /* just used for initiating side */
#define ESCO_DEFAULT_D                 0
#define ESCO_DEFAULT_W                 2
#define ESCO_NEGO_INIT                 0
#define ESCO_NEGO_PREFERRED            1
#define ESCO_NEGO_SLTO_VIOLATION       2
#define ESCO_NEGO_LATENCY_VIOLATION    3
#define ESCO_NEGO_NO_SUPPORTED         4

/* AFH */
#define AFH_DISABLED            0
#define AFH_ENABLED             1
#define AFH_REPORTING_DISABLED  0
#define AFH_REPORTING_ENABLED   1
#define AFH_MAX_INTERVAL        0x12C0 /* unit: slot;  CSR default value 3s */
#define AFH_MIN_INTERVAL        0x0960 /* unit: slot;  CSR default value 1.5s */

/* POWER CONTROL */
#ifdef BT_POWER_CONTROL_2829
#define TX_POWER_DEFAULT        16
#define TX_POWER_MAX            56
#define TX_POWER_MIN            0
#define TX_POWER_ADJUST_STEP    8
#else
#define TX_POWER_DEFAULT        12
#define TX_POWER_MAX            52
#define TX_POWER_MIN            4
#define TX_POWER_ADJUST_STEP    8
#endif
#define RX_POWER_MAX_FLAG       1
#define RX_POWER_MIN_FLAG       2

/* sniff */
#define SNIFF_MAX_NUMBER        1
#define SNIFF_MAX_INTERVAL      0x1000 /* 0x0540 is defined by protocol */
#define SNIFF_MIN_INTERVAL      0x0006
#define SNIFF_MAX_TIMEOUT       0x0028
#define SNIFF_INIT_FLAG         1

/*--constants and types------------------------------------------------*/
#pragma pack(1)


typedef struct _LMP_PUD_PACKAGE 
{
	UINT8 TransID : 1;
	UINT8 OpCode  : 7; /* specify the escape opcode when having 15bit opcode */
	UINT8 contents[MAX_PDU_LENGTH - 1]; /* contents[0] specifies the extended opcode when having 15bit opcode */
} LMP_PUD_PACKAGE_T, *PLMP_PUD_PACKAGE_T;

typedef struct _BT_SNIFF_T 
{
	PCONNECT_DEVICE_T pDevice; /* indicate sniff owner */
	UINT8 init_flag; /* for sniff failure */
	UINT8 change_for_sco; /* 1: has been adjusted the interval and offset */
	
	UINT8  reserved; /* for 64bit alignment */
	UINT8  timing_control_flags;
	UINT16 offset;
	UINT16 interval;
	UINT16 attempt;
	UINT16 timeout;
} BT_SNIFF_T, *PBT_SNIFF_T;

typedef struct _BT_ESCO_T 
{
	PCONNECT_DEVICE_T pDevice; /* indicate ESCO owner */
	UINT8 change_flag;         /* 0: default; 1(ESCO_ADD_LINK); 2(ESCO_CHANGE_LINK) */
	UINT8 hc_esco_handle;      /* just used for slave */

	UINT8 clock_flag;
	UINT8 reserved; /* for 64bit alignment */

	/* remain the temporary para */
	UINT8  esco_handle; /* also indicate the hc_sco_handle if be master */
	UINT8  esco_LT_addr;
	UINT8  timing_control_flags;
	UINT8  D_esco;
	UINT8  T_esco;
	UINT8  W_esco;
	UINT8  packet_type_m2s;
	UINT8  packet_type_s2m;
	UINT16 packet_len_m2s;
	UINT16 packet_len_s2m;
	UINT8  air_mode;
	UINT8  negotiation_state;
} BT_ESCO_T, *PBT_ESCO_T;

typedef struct _BT_SCO_T 
{
	PCONNECT_DEVICE_T pDevice; /* indicate SCO owner */
	UINT8 change_flag;         /* 0: default; 1(SCO_ADD_LINK): add a new SCO */
	UINT8 hc_sco_handle;       /* just used for slave */

	/* remain the temporary para */
	UINT8 sco_handle; /* also indicate the hc_sco_handle if be master */
	UINT8 timing_control_flags;
	UINT8 D_sco;
	UINT8 T_sco;
	UINT8 sco_packet;
	UINT8 air_mode;

	UINT8 clock_flag; /* 0: no clock;  1: add sco;  2: accepted sco;  3: change sco */
} BT_SCO_T, *PBT_SCO_T;

/* This structure contains the information about LMP module */
typedef struct _BT_LMP 
{

	// If driver is halt, this flag is set
	UINT8 exit_thread_flag;

	UINT16 ClockReadyFlag;
	UINT8  air_mode_feature;    /* LSB according to BT 2.0 */
	UINT16 packet_type_feature; /* LSB according to BT 2.0, Setup Synchronous Connection Command parameter */
	UINT8  sco_indicator;       /* the most 2 bits indicates 3 SCO's validity; the least 2 bit indicates which sco is used currently */
	
	BT_SCO_T sco[SCO_MAX_NUMBER];
	UINT8    sco_counter;      /* MSB 4: packet type not-accepted number; LSB 4: air_mode not-accepted number */
	PCONNECT_DEVICE_T pDevice; /* remain SCO_adding device pointer when needs change other SCO para */

	BT_ESCO_T esco[SCO_MAX_NUMBER];
	UINT8     esco_counter; /* MSB 4: packet type not-accepted number; LSB 4: air_mode not-accepted number */

	BT_SNIFF_T sniff[SNIFF_MAX_NUMBER];
	UINT8      sniff_number; /* indicates the sniff validity */
	UINT8      sniff_index;  /* indicates which sniff is used */
} BT_LMP_T, *PBT_LMP_T;

typedef struct _LMP_PDU_RX 
{
	NTSTATUS (*Rx_Fun)(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);
	UINT8    PDU_Length;
	UINT8    ResOpcode;
} LMP_PDU_RX_T, *PLMP_PDU_RX_T;

#pragma pack()

// Lmp Dump
struct lmp_dump{
    UINT16 hConn;
    UINT8 tx;
    UINT32 tsec;
    UINT32 tms;
    LMP_PUD_PACKAGE_T lmpPdu;
};



/*--variables----------------------------------------------------------*/
	
/*--function prototypes------------------------------------------------*/

extern NTSTATUS LMP_Init(PBT_DEVICE_EXT devExt);
extern NTSTATUS LMP_Release(PBT_DEVICE_EXT devExt);

extern NTSTATUS LMP_SendPDUToLC(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);
extern NTSTATUS LMP_SendPDUToLCHead(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);
extern NTSTATUS LMP_PDU_LC2LM_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength);
extern NTSTATUS LMP_HC2LM_Command_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 Command);

extern void  LMP_PDU_StopTimer(PCONNECT_DEVICE_T pConnectDevice);
extern void  LMP_PDU_Timeout(PBT_DEVICE_EXT devExt);
extern void  LMP_PDUEnter_Task(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength);
extern void  LMP_ResetMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
extern void  LMP_ReleaseSCOMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
extern void  LMP_ReleaseSniffMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
extern UINT8 LMP_NeedClockReady(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T *ppConnectDevice);
extern UINT8 LMP_CheckSlaveSCO(PBT_DEVICE_EXT devExt);
extern UINT8 LMP_CheckMasterSCO(PBT_DEVICE_EXT devExt);

extern void LMP_Task_EnterSniff(PBT_DEVICE_EXT devExt, PUINT8 para);
extern void LMP_Task_LeaveSniff(PBT_DEVICE_EXT devExt, PUINT8 para);
extern void LMP_Task_EdrModeChange(PBT_DEVICE_EXT devExt, PUINT8 para);
extern void LMP_Task_ResumeEncryption(PBT_DEVICE_EXT devExt, PUINT8 para);
void LMP_Reset_AfhLmpCount(PBT_DEVICE_EXT devExt);
void LMP_Clear_LmpPdu(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnDev);
void LMP_Task_RoleChange_Recv(PBT_DEVICE_EXT devExt, PUINT8 para, UINT32 Length);
void LMP_Task_RoleChange_Send(PBT_DEVICE_EXT devExt, PUINT8 para);

extern void     E1_Authentication_KeyGen(unsigned char *pKey, unsigned char *pRand, unsigned char *pAddress, unsigned char *pSRes, unsigned char *pACO);
extern void     E21_UnitAndCombination_KeyGen(unsigned char *pRand, unsigned char *pAddress, unsigned char *pKey);
extern NTSTATUS E22_InitAndMaster_KeyGen(unsigned char *pRand, unsigned char *pPIN, unsigned char pLength, unsigned char *pBDAddress, unsigned char *pKey);
extern void     E3_Encryption_KeyGen(unsigned char *pKey, unsigned char *pEnRand, unsigned char *COF, unsigned char *Kc);
extern void     Encryption_ApostropheKeyGen(unsigned char *Kc, unsigned char KeyLength, unsigned char *ApostropheKc);
extern void     AccessCode_Gen(unsigned char *pLAP, unsigned char *pAccessCode);
extern UINT8    Rand_UCHAR_Gen(void);

extern UINT32 LMP_CheckTxQueue(IN PBT_DEVICE_EXT devExt);

void  LMP_SCONeedUnsniff(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
void LMP_Unsniff_SCO(PBT_DEVICE_EXT devExt);

NTSTATUS Send_SCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_eSCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);

#endif
/*--end of file--------------------------------------------------------*/
