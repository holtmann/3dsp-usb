#ifndef __BT_FLUSH_H__
#define __BT_FLUSH_H__ 

/***********************************************************************************
*Jakio20080801: 
*	data struct for flush function
***********************************************************************************/

#include "bt_sw.h"



//Jakio20090220: change buffer length
#define MAX_LEN_BAKUP_MASTER	 	6144	//6144
#define MAX_LEN_BAKUP_SLAVE	 	6144	//6144
#define MAX_LEN_BAKUP_SNIFF		1024	//6144

#define MAX_NUM_BAK_NODES			200


#define BT_FLUSH_QUEUE_SLAVE			0x00	
#define BT_FLUSH_QUEUE_SLAVE1			0x02	
#define BT_FLUSH_QUEUE_MASTER			0x04
#define BT_FLUSH_QUEUE_OTHER			0x06
#define BT_FLUSH_QUEUE_SNIFF			0x06

typedef struct _BTUSB_FLUSH_INFO
{
	UINT8		RebuildFlag;
	BOOLEAN		SendOk;			//Jakio20080925: flag to identify if this packet has been sent to DSP
	UINT8		PacketType;
	UINT8		fpid;
	UINT8		ppid;
	UINT16		DeltaLength;		//Jakio20080927: when rebuild frag, this var record the additional added header length
	ULONG_PTR	pConnDev;
	//UINT8 send_complete_flag;
	BT_FRAG_TX_BLOCK_T TxBlock[MAX_NUM_BAK_NODES];
}BTUSB_FLUSH_INFO, *PBTUSB_FLUSH_INFO;





typedef struct _BTUSB_BAK_LIST_HEAD
{
	BT_LIST_ENTRY_T Link;	//for link management
	UINT32	Ptr_Head;		//used buffer head offset
	UINT32	Ptr_Tail;		//used buffer tail offset
	UINT32	Total_length;	//used buffer length
	UINT32	Num_Nodes;	//node num of this list

	UINT32	SendingLength;	//jakio20080927: record the length of sending bakup packets
	UINT32	DeltaLength;	//jakio20080927: length when additional hcbb header added. 
	UINT32	RetryPacketHead;	//save payload header of the retried packet,   for safe check
	UINT32	FlushValidFlag;	//flag indentify whether need to handle retry queue, set when this queue reachs retry limit
}BTUSB_BAK_LIST_HEAD, *PBTUSB_BAK_LIST_HEAD;


typedef struct _BTUSB_BAK_LIST_NODE
{
	BT_LIST_ENTRY_T 		Link;
	UINT32				Ptr_Head;		//data offset  of this node
	UINT32				Size;			//data length of  this node
	BTUSB_FLUSH_INFO		FlushInfo;			
}BTUSB_BAK_LIST_NODE, *PBTUSB_BAK_LIST_NODE;


typedef struct _BTUSB_FLUSH_T
{
	BTUSB_BAK_LIST_HEAD	MasterBakList;	//list head of master bak data
	BTUSB_BAK_LIST_HEAD	SlaveBakList;	//list head of slave bak data
	BTUSB_BAK_LIST_HEAD	SwapBakList;
	BT_LIST_ENTRY_T		FreeNodeList;	//for free node management
	BTUSB_BAK_LIST_NODE	FlushNodes[MAX_NUM_BAK_NODES];	//nodes

	KSPIN_LOCK			FlushLock;
	
	PUINT8		MasterBakBuffer;		//buffer to hold master bak data
	PUINT8 		SlaveBakBuffer;		//buffer to hold slave bak data
	PUINT8		SwapBuffer;
#ifdef BT_SNIFF_SUPPORT
	BTUSB_BAK_LIST_HEAD	SniffBakList;	//list head of slave bak data
	PUINT8		SniffBuffer;
#endif

}BTUSB_FLUSH_T, *PBTUSB_FLUSH_T;







VOID Flush_RebuildBakupPacket(PBT_DEVICE_EXT DevExt, UINT8 ListIndex, PBTUSB_BAK_LIST_NODE pNode);
VOID Flush_TidyUp_UsedList(PBT_DEVICE_EXT DevExt);
VOID Flush_PrintInfo(PBT_DEVICE_EXT DevExt);
VOID Flush_ResetDeltaLength(PBT_DEVICE_EXT DevExt, UINT8 ListIndex);
VOID Flush_PrintNodeInfo(PBTUSB_BAK_LIST_NODE pNode);
VOID Flush_DelTailNode(PBT_DEVICE_EXT DevExt, PBTUSB_BAK_LIST_NODE pNode, UINT8 ListIndex);
VOID Flush_AddHeadNode(PBT_DEVICE_EXT DevExt, UINT32 length, UINT8 ListIndex);
VOID Flush_DelNodes(PBT_DEVICE_EXT DevExt, UINT8 ListIndex, UINT8 am_addr);
VOID Flush_RefreshNode(PBT_DEVICE_EXT DevExt, PBTUSB_BAK_LIST_HEAD	pList, UINT32 Length, UINT8 ListIndex);

#endif /*__BT_FLUSH_H__*/
