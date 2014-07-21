/************************************************************************
 * Copyright (c) 2006, 3DSP Corporation, all rights reserved.  Duplication or
 * reproduction of any part of this software (source code, object code or
 * comments) without the expressed written consent by 3DSP Corporation is
 * forbidden.  For further information please contact:
 *
 * 3DSP Corporation
 * 16271 Laguna Canyon Rd
 * Irvine, CA 92618
 * www.3dsp.com
 *************************************************************************/

#ifndef __SCHED_H_
#define __SCHED_H_

#define MAXSCHEDDATAN  8
#define MAXSCHEDBLOCKNUM  32

#define SCHEDMINTXFRAME	10

/* The pri value  of our own task */
#define SCHED_PRI_NORMAL     (50)
#define SCHED_PRI_HIGH       (20)
#define SCHED_PRI_LOW        (80)

/* Define event */
#define SCHED_EVENT_SEND_POLL   1
#define SCHED_EVENT_NULL_ACL_STOP   2
#define SCHED_EVENT_NULL_ACL_CONTINUE   3

#define SCHED_MIN(a, b) (((a) < (b)) ? (a) : (b))

//
// This structure contains the information about test module
//
//
typedef struct _BT_SCHED_BLOCK
{
	// For link
	BT_LIST_ENTRY_T Link;

	// in list flag. If this block is in free or task list, this flag is set as 1.
	// And if this block is not in any lists, this flag is set as 0. This flag is used
	// to determine whether this block is in list or not. If this block is already in
	// list, driver must not insert this block into list again.;
	UINT8 in_list_flag;

	// Sched event
	UINT32 sched_event;
	// Sched pri
	UINT32 sched_pri;
	// Sched data length
	UINT32 sched_len;
	// Sched para
	UINT8 data[MAXSCHEDDATAN];
} BT_SCHED_BLOCK_T,  *PBT_SCHED_BLOCK_T;

typedef struct _BT_SCHED
{
	// Head pointer of total task block.
	BT_SCHED_BLOCK_T sched_block[MAXSCHEDBLOCKNUM];
	// Task pool
	BT_LIST_ENTRY_T sched_free_pool;
	// Task current used list
	BT_LIST_ENTRY_T sched_list;

	// Spin lock for task module
	KSPIN_LOCK lock;

} BT_SCHED_T,  *PBT_SCHED_T;

NTSTATUS Sched_Init(PBT_DEVICE_EXT devExt);
NTSTATUS Sched_Release(PBT_DEVICE_EXT devExt);

BOOLEAN Sched_CheckExistSched(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, UINT32 event, PUINT8 para, UINT32 len);

NTSTATUS Sched_DoOneSched(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);

NTSTATUS Sched_SendPollFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice);

NTSTATUS Sched_SendAclNullStopFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice);

NTSTATUS Sched_SendAclNullContinueFrame(PBT_DEVICE_EXT devExt, PBT_SCHED_T pSched, PCONNECT_DEVICE_T pConnectDevice);
VOID Sched_Excute(PBT_DEVICE_EXT devExt);
VOID Sched_ScanAllLink(PBT_DEVICE_EXT devExt);
VOID Sched_DeleteSched(PBT_SCHED_T pSched, UINT32 event, PUINT8 para, UINT32 len);
VOID Sched_DeleteAllPoll(PBT_DEVICE_EXT devExt);



#endif
