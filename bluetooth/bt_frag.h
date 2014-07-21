#ifndef BT_FRAG_H
    #define BT_FRAG_H

    /***********************************************************************
     *
     * FILENAME:    BT_Frag.h      CURRENT VERSION: 1.00.01
     * CREATE DATE: 2005/11/29
     * PURPOSE:      mainly define the structrue for fragment and defragment
     *
     * AUTHORS:      jason dong
     *
     *
     **********************************************************************/

    /*
     * HISTORY OF CHANGES
     *
    2005.12.12 Add one parameter for function Frag_ProcessFrag and Frag_ProcessTx
    2005.12.15 Add a member in struct BT_FRAG_ELEMENT_T for the support of SCO
    typedef struct _BT_FRAG
    {
    ...
    UINT8 link_type;
    ...
    }
    2005.12.23 We add another fragment element for SCO data. So we should consider the two conditions:
    one is for the processing of ACL data, the other is for the processing of SCO data. So
    some new functions is added for processing SCO data. Some members is also added in struct
    _BT_FRAG
    typedef struct _BT_FRAG
    {
    ...
    BT_FRAG_ELEMENT_T TxScoFragElement[BT_FRAG_MAX_ELEMENTS];
    UINT32 ele_TxScoPid;
    UINT32 ele_TxScoCid;
    UINT32 ele_TxScoTid;
    ...
    }
    A new macro is added for the threshold of SCO data
    BT_FRAG_SCO_THRESHOLD
    Some functions declaration is modifed:
    Frag_BuildFrag, Frag_ProcessThisIRP, Frag_ProcessTx
    Some functions is added:
    Frag_IsScoEleEmpty, Frag_IsScoEleFull, Frag_ProcessScoFrag, Frag_ProcessTxScoInt
    2005.12.28 Add new function Frag_InitTxScoForConnDev
    2005.12.29 Because other c files maybe use var "PACKET_MAX_BYTES", this var should be declared in
    this file using "extern".
    2005.12.30 Add new function Frag_AutoSelAclPacketType for auto-select-packet-type function
    2006.1.6  Found a bug. var PACKET_MAX_BYTES should be UINT16 type not UINT8 type
    2006.1.9  Because the IRP received from IVT program may contain more than one SCO frame, driver should do some processing about it.
    The way we used is that driver copy all the frames in one IRP into one continous memory and then frag this continous memory.
    So some codes should be modified.
    Add a new struct _BT_SCO_TOTAL_SPACE
    Add a member "TxScoSpace" in struct _BT_FRAG
    2006.1.13  Driver adds another three SCO rx cache. And one SCO link has one rx cache. And
    these caches is separated from Rx cache which is used for ACL link. This is very
    useful for the SCO packet's combination as to one SCO link.
    Add some macros: BT_SCO_RX_CACH_SIZE, BT_SCO_MAX_RX_CACH_COUNT, BT_TOTAL_SCO_LINK_COUNT, BT_SCO_DEFRAG_LIMIT,
    BT_FRAG_SCO_THRESHOLD
    Add some structures: struct _BT_SCO_RX_CACH_BLOCK, struct _BT_SCO_RX_ELEMENT
    Add a member "RxScoElement" in struct _BT_FRAG.
    Add some declarations of function: Frag_GetNextScoRxCachId, Frag_IsScoRxCachEmpty, Frag_IsScoRxCachFull,
    Frag_ProcessRxScoData,Frag_IsAllScoRxCachEmpty,Frag_IsAllScoRxCachEmptyAndRetId
    2006.2.24  Add two members "Valid" and "send_complete_flag" in struct _BT_FRAG_ELEMENT.
    2006.3.9 Now we add some source codes to release some resource such as frag element and so on after all SCO
    link is released.
    Add one function Frag_InitTxScoForAllDev.
    2006.3.22 Add one function Frag_FlushTxForConnDev for flush
    2006.4.18 When our card is connected with both an ACL link and an SCO link, it can only send a little data, then pending
    (ACL link, File Transfer Service). After debugging with sniffer, I found IVT would send 32K bytes data through
    ACL link continually (That is the voice data could not be sent duration the sending of ACL data) if both an
    ACL link and an SCO link is established. So if the cache is so small, the latency would be very large and this
    may lead to IVT program hung. The cache for the old driver is very small. Now I add caches both for ACL link and
    for SCO link (each has 36K bytes cache). Then driver could cache data first, then it would tranfer data from
    cache to Tx BD.
    Change the max elements count from (4 to 36)
    Add one macro "BT_MAX_TOTAL_BYTES_ONE_IRP" (expresses max total bytes one IRP has)
    Add one member "memory" in struct _BT_FRAG_ELEMENT.
    delete member "TxScoSpace" in struct _BT_FRAG.
     */

#include "bt_lmp.h"

/*--macros------------------------------------------------------------*/
// #define BT_FRAG_TIMER_INTERVAL   50
#define BT_SCO_TOTAL_BYTES_ONE_IRP  480

#define BT_FRAG_MAX_COUNT   100
#define BT_FRAG_MAX_ELEMENTS 50	

//#define BT_FRAG_THRESHOLD   17

#define BT_FRAG_SEND_PKTNUM_COUNT	4

#define BT_FRAG_THRESHOLD  339

//add for EDR
#define BT_FRAG_THRESHOLD_ENHANCED 1021

#define MIN_PDU_LENGTH               1
#define MAX_PDU_LENGTH               17

#define BT_SCO_RX_COUNT_LIMIT	10
#define BT_SCO_RX_CACH_SIZE     512
#define BT_MAX_TOTAL_BYTES_ONE_IRP  1028
#define BT_SCO_MAX_RX_CACH_COUNT 11
#define BT_TOTAL_SCO_LINK_COUNT 3
#define BT_SCO_DEFRAG_LIMIT     120

#define BT_TX_CMD_MAX_LEN		256

#define BT_FRAG_SCO_THRESHOLD  (BT_SCO_DEFRAG_LIMIT * 2)
/*--constants and types------------------------------------------------*/

#define  BT_FRAG_SUCCESS              0
#define  BT_FRAG_LIST_NULL            1
#define  BT_FRAG_RESOURCE_ERROR       2
#define  BT_FRAG_PENDING              3

#define	MASK_BITS_ALLOW_SENDING			0x001f
#define	MASTER_LIST_ALLOW_SENDING_BIT	0x0001
#define	SLAVE_LIST_ALLOW_SENDING_BIT		0x0002
#define	SNIFF_LIST_ALLOW_SENDING_BIT		0x0004
#define	SLAVE1_LIST_ALLOW_SENDING_BIT	0x0008
#define	SCO_LIST_ALLOW_SENDING_BIT		0x0010



#define	TIMER_SOURCE_MASTER_BIT		0x01
#define	TIMER_SOURCE_SLAVE_BIT		0x02
#define	TIMER_SOURCE_SNIFF_BIT		0x04
#define	TIMER_SOURCE_SLAVE1_BIT		0x08
#define	TIMER_SOURCE_SCO_BIT		0x10
#define	TIMER_SOURCE_ACL_BITS		0x0f



#define FRAG_ALL_LIST     0
#define FRAG_MASTER_LIST  1
#define FRAG_SLAVE_LIST   2
#define FRAG_SNIFF_LIST   3
#define FRAG_SLAVE1_LIST  4
#define FRAG_SCO_LIST	     5	
#define MAX_LIST_NUM         5
#define MAX_REAL_LIST_NUM    (MAX_LIST_NUM - 1)

#define MAX_LIST_STOP_TIME   30 /* unit: second */
#define LIST_STOP_FLAG       1


#pragma pack(1)

typedef struct _BT_FRAG_TX_BLOCK
{
	UINT32 len;
	UINT16 bufpos;
} BT_FRAG_TX_BLOCK_T,  *PBT_FRAG_TX_BLOCK_T;

typedef struct _BT_FRAG_ELEMENT
{
	BT_LIST_ENTRY_T	link;
	UINT8 fpid;
	UINT8 fcid;
	UINT8 ppid;
	UINT8 link_type;
	PVOID CurrentIrp;
	UINT32 total_len;
	//jakio20080616: changed for 64bit OS
	ULONG_PTR pConnectDevice;
	UINT8 Valid;
	UINT8 failure_times;
	//UINT8 send_complete_flag;
	BT_FRAG_TX_BLOCK_T TxBlock[BT_FRAG_MAX_COUNT];
	UINT8 memory[BT_MAX_TOTAL_BYTES_ONE_IRP];
	COMPLETE_PACKETS_T	CompletPkt;	//Jakio20070906
	UINT8	scoHeader[8];
	
} BT_FRAG_ELEMENT_T,  *PBT_FRAG_ELEMENT_T;

#pragma pack()

//
// Because one IRP may contains more than one SCO frame, driver should collect all the bytes which is in each frame
// into one continuous space. Although this could take CPU more time copying the memory, it could predigest the
// driver's arithmetic.
//
// typedef struct _BT_SCO_TOTAL_SPACE
// {
//	UINT8 memory[BT_SCO_TOTAL_BYTES_ONE_IRP];
// } BT_SCO_TOTAL_SPACE_T, *PBT_SCO_TOTAL_SPACE_T;


typedef struct _BT_SCO_RX_CACH_BLOCK
{
	UINT8 RxCach[BT_SCO_RX_CACH_SIZE];
} BT_SCO_RX_CACH_BLOCK_T,  *PBT_SCO_RX_CACH_BLOCK_T;

typedef struct _BT_SCO_RX_ELEMENT
{
	BT_SCO_RX_CACH_BLOCK_T RxCachBlock[BT_SCO_MAX_RX_CACH_COUNT];
	UINT32 RxCachPid;
	UINT32 RxCachCid;
	UINT32 totalcount;
	UINT32 currentpos;
	UINT32 currentlen;
} BT_SCO_RX_ELEMENT_T,  *PBT_SCO_RX_ELEMENT_T;



typedef struct _PDU_TX_LIST
{
	BT_LIST_ENTRY_T Link; /* For linked list management */

	/* in_list_flag: If this block is in free or active list, this flag is set as 1.
	And if this block is not in any lists, this flag is set as 0. This flag is used
	to determine whether this block is in list or not. If this block is already in
	list, driver must not insert this block into list again. */
	// UINT8 in_list_flag;

	PCONNECT_DEVICE_T pConnectDevice; /*pointer to ConnectDevice*/

	UINT8 data_len; /* PDU data length */
	UINT8 data[MAX_PDU_LENGTH]; /* PDU para */
} PDU_TX_LIST_T,  *PPDU_TX_LIST_T;


//
// This structure contains the information about test module
//
//
typedef struct _BT_FRAG
{

	//Add for flush function. we can disable all tx from driver by setting this flag to "1"
	UINT8 TxDisable_Flag;

	BT_FRAG_ELEMENT_T 	TxFragElement[BT_FRAG_MAX_ELEMENTS];
	//BT_LIST_ENTRY_T		Acl_FragList;
	BT_LIST_ENTRY_T		Slave_FragList;
	BT_LIST_ENTRY_T		Slave1_FragList;
	BT_LIST_ENTRY_T		Master_FragList;
	BT_LIST_ENTRY_T		Sniff_FragList;

	BT_LIST_ENTRY_T		Sco_FragList;

	BT_LIST_ENTRY_T		Free_FragList;
	UINT16				FreeFrag_Count;

	PDU_TX_LIST_T		TxPduElement[MAX_PDU_TX_QUEUE];
	BT_LIST_ENTRY_T		Pdu_UsedList;

	BT_LIST_ENTRY_T		Pdu_FreeList;
	UINT16				FreePdu_Count;

	//Add this flag to identify each queue state, each bit stands for one queue,
	//if according bit is set to 1,then allow sending, otherwise, not allow sending.
	UINT16				AllowSendingFlag;

	//This stop flag is indicate if we should stop sending.
	/* 0: continue; 1: stop */
	UINT8 list_stop_flag[MAX_LIST_NUM+1];

	UINT8 AclNeedProcFlag[MAX_LIST_NUM + 1];

	UINT32	AclProcCounter;
	//This timer used to superwise the state of a queue, but no more measures be taken.
	/* unit: second */
	UINT8 list_timer[MAX_LIST_NUM];     

	KSPIN_LOCK			FragTxLock;		

	BT_SCO_RX_ELEMENT_T RxScoElement[BT_TOTAL_SCO_LINK_COUNT];


	// space info of sending queues, size unit is byte
	UINT32		IdleSpace[MAX_LIST_NUM + 1];

	// Timer used for tx data
	struct timer_list   TxTimer;

	
} BT_FRAG_T,  *PBT_FRAG_T;



/*--variables---------------------------------------------------------*/
extern const UINT16 PACKET_MAX_BYTES[0x10];
extern const UINT16 EDR_PACKET_MAX_BYTES[0x10]; //jakio20071026 add here for EDR
extern const UINT16 PacketSlotFlag[6];
/*--function prototypes-----------------------------------------------*/
NTSTATUS Frag_Init(PBT_DEVICE_EXT devExt);
NTSTATUS Frag_Release(PBT_DEVICE_EXT devExt);
VOID Frag_Reset(PBT_DEVICE_EXT devExt);

UINT32 Frag_GetNextEleId(UINT32 id);
BOOLEAN Frag_IsEleEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN Frag_IsEleEmpty_New(PBT_DEVICE_EXT devExt, UINT8 ListFlag);
BOOLEAN Frag_IsEleFull(PBT_FRAG_T pFrag);

BOOLEAN Frag_IsScoEleEmpty(PBT_FRAG_T pFrag);
BOOLEAN Frag_IsScoEleFull(PBT_FRAG_T pFrag);

NTSTATUS Frag_BuildFrag(PBT_DEVICE_EXT devExt, struct sk_buff *skb);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)  
void thread_processing_cmd(struct work_struct *pwork);
#else  
void thread_processing_cmd(PVOID pvoid);
#endif
VOID Frag_InitTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
VOID Frag_FlushTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

UINT32 Frag_ProcessScoFrag(PBT_DEVICE_EXT devExt);
//    VOID Frag_ProcessTxScoInt(PBT_DEVICE_EXT devExt);
VOID Frag_InitTxScoForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
VOID Frag_InitTxScoForAllDev(PBT_DEVICE_EXT devExt);
VOID Frag_AutoSelAclPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 retrycount);

UINT32 Frag_GetNextScoRxCachId(UINT32 id);
BOOLEAN Frag_IsScoRxCachEmpty(PBT_FRAG_T pFrag, UINT8 index);
BOOLEAN Frag_IsScoRxCachFull(PBT_FRAG_T pFrag, UINT8 index);

NTSTATUS Frag_ProcessRxScoData(PBT_DEVICE_EXT devExt, PUINT8 source, UINT32 inlen, PUINT8 dest, PUINT32 poutlen);
BOOLEAN Frag_IsAllScoRxCachEmpty(PBT_FRAG_T pFrag);
BOOLEAN Frag_IsAllScoRxCachEmptyAndRetId(PBT_FRAG_T pFrag, PUINT8 pid);
UINT32 Frag_ProcessAclFrag(PBT_DEVICE_EXT devExt, UINT8 ListIndex);
UINT32 Frag_ProcessCmdFrag(PBT_DEVICE_EXT devExt);
PBT_LIST_ENTRY_T Frag_GetQueueHead(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pListFlag);
void Frag_MoveFrag(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_LIST_ENTRY_T pDestList, PBT_LIST_ENTRY_T pSourceList);
VOID Frag_DisableTxQueue(PBT_DEVICE_EXT devExt, UINT8 ListIndex);
VOID Frag_EnableTxQueue(PBT_DEVICE_EXT devExt, UINT8 ListIndex);

void Frag_FlushFragAndBD(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ListFlag);
BOOLEAN Frag_IsBDEmpty(PBT_DEVICE_EXT devExt, UINT8 ListFlag);
void Frag_ListStopSwitch(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_value);

void Frag_GetCurrentPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Frag_SetConnectionFeatureBit(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Frag_GetFragThreshold(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT16 pFragThreshold);
void Frag_RebuildFrag(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
VOID Frag_ExtentAutoSelAclPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 retrycount);
VOID Frag_InitScoRxBuffer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Frag_GetFragPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 len, PUINT8 pPacketType);

VOID Frag_DataSendingTimer(ULONG_PTR SystemContext);
VOID Frag_ListRebuild(PBT_DEVICE_EXT devExt, UINT8 ListFlag);
VOID Frag_ListRebuildForSco(PBT_DEVICE_EXT devExt);
VOID Frag_StopQueue(PBT_DEVICE_EXT devExt, UINT8 ListFlag);
VOID Frag_StartQueue(PBT_DEVICE_EXT devExt, UINT8 ListFlag);
VOID Frag_StopQueueForSco(PBT_DEVICE_EXT devExt);
VOID Frag_StartQueueForSco(PBT_DEVICE_EXT devExt, UINT8 NeedRebuildFlag);
void Frag_CheckTxQueue(PBT_DEVICE_EXT devExt);
VOID Frag_ProcessSpaceInt(PBT_DEVICE_EXT devExt, PUINT8 pBuffer);
VOID Frag_SetTimerType(PBT_DEVICE_EXT devExt, UINT8 ListIndex);
VOID Frag_TxTimerInstant(PBT_DEVICE_EXT devExt, PLARGE_INTEGER timeValue);
VOID Frag_CheckAndAddScoNullFrame(PBT_DEVICE_EXT devExt);

#endif
