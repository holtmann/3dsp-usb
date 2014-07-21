#ifndef DSP_TASK_H
   #define DSP_TASK_H

/***********************************************************************
 * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
 *
 * FILENAME:    DSP_Task.h      CURRENT VERSION: 1.00.01
 * PURPOSE:      mainly define the structrue that store the information
 *               about task module and the external function for task 
 *               module.
 * 
 *
 * DECLARATION:  This document contains confidential proprietary information that
 *               is solely for authorized personnel. It is not to be disclosed to
 *               any unauthorized person without prior written consent of 3DSP
 *               Corporation.        
 *
 **********************************************************************/
#include "precomp.h"	
//#include "usbwlan_sw.h"
/*--macros------------------------------------------------------------*/

#define     MAXTASKN					(96)	/* The numbers of the total task  in the free task pool*/
#define     MAXTASKDATAN			(512)	/* The bytes of each task having */ 

/* The pri value  of our own task */
#define     DSP_TASK_PRI_NORMAL		(50)
#define     DSP_TASK_PRI_HIGH		(20)
#define     DSP_TASK_PRI_LOW		(80)

/* The event number of task */
#define	DSP_TASK_EVENT_CALL_MANAGEMENT			(1) 
#define	DSP_TASK_EVENT_MNGTIMEOUT				(2)
#define	DSP_TASK_EVENT_INTERNALRESET			(3)
#define	DSP_TASK_EVENT_SCAN						(4)
#define	DSP_TASK_EVENT_BEACON_CHANGE			(5)
#define	DSP_TASK_EVENT_TBTT_UPDATE				(6)
#define	DSP_TASK_EVENT_OID_ADD_WEP				(7)
#define	DSP_TASK_EVENT_OID_REMOVE_WEP			(8)
#define	DSP_TASK_EVENT_OID_ADD_KEY				(9)
#define	DSP_TASK_EVENT_OID_REMOVE_KEY			(10)
#define	DSP_TASK_EVENT_OID_ASSOC_INFO			(11)
#define	DSP_TASK_EVENT_OID_POWER_D3			(12)
#define	DSP_TASK_EVENT_OID_POWER_D0			(13)
#define	DSP_TASK_EVENT_BEACON_LOST				(14)
#define	DSP_TASK_EVENT_OID_SET_SSID				(15)
#define	DSP_TASK_EVENT_OID_DISASSOC				(16)
#define	DSP_TASK_EVENT_ATPA_CHECK_TP			(17)
#define	DSP_TASK_EVENT_DIRECTLY_DISCONNECTED	(18)
#define	DSP_TASK_EVENT_CHECK_RSSI				(19)
#define	DSP_TASK_EVENT_OID_RELOAD_DEFAULT		(20)
#define	DSP_TASK_EVENT_OID_INFRASTRUCTURE_MODE  (21)
#define	DSP_TASK_EVENT_CHECK_BULK_STALL			(22)
#define	DSP_TASK_EVENT_HANDLE_TX_STOPPED		(23)  
#define	DSP_TASK_EVENT_BULKOUT_ERROR			(24)  
#define	DSP_TASK_EVENT_BULKIN_ERROR				(25)  
#define	DSP_TASK_EVENT_AUTORATE					(26)  
#define	DSP_TASK_EVENT_REQJOIN_8051				(27)  
#define	DSP_TASK_WLANCFG_SETPARAMETERS		(28)  
#define	DSP_TASK_EVENT_OID_SET_RTS				(29)		//add for wumin by wanghk

#define	DSP_TASK_EVENT_HARDWARE_RESET			(30)
#define	DSP_TASK_EVENT_SOFT_RESET				(31)
       	
#define	DSP_TASK_EVENT_SYS_RESET					(32)
#define	DSP_TASK_EVENT_SET_POWER_SAVE			(33)
#define	DSP_TASK_EVENT_SET_POWER_ACTIVE		(34)
       	
#define	DSP_TASK_EVENT_RECONNECT				(35)
       	
#define	DSP_TASK_EVENT_SET_POWER_MNG_BIT		(36)

//justin: 080403.		to recover all regs changed by roaming scan
#define	DSP_TASK_EVENT_RECOVER_REGS_JOINED		(37)
#define	DSP_TASK_EVENT_ACCESS_DSP_MAILBOX		(38)
#define	DSP_TASK_EVENT_TX_WATCH_TIMEOUT		(39)


#define	DSP_TASK_EVENT_SUPRISING_REMOVED		(50)
#define	DSP_TASK_EVENT_FIFO_TEST_COMPLETED		(51)
#define	DSP_TASK_EVENT_FIFO_TEST_BEGIN			(52)
#define	DSP_TASK_EVENT_RETRY_LIMIT				(53)

/* for DUT(MFP) */
#define	DSP_TASK_EVENT_DUTRESET					(100)
#define	DSP_TASK_EVENT_DUTTX						(101)
#define	DSP_TASK_EVENT_DUTRXWAIT				(102)
#define	DSP_TASK_EVENT_DUTTX_CONTINUE			(103)


/* for test */
#define     DSP_TASK_EVENT_TEST						(200)

#ifdef ANTENNA_DIVERSITY
#define	TASK_MAILBOX_TYPE_ANTENNA			0
#define	TASK_MAILBOX_TYPE_CORR				1
#endif

#define     TASK_MIN(a, b) (((a) < (b)) ? (a) : (b))

/*--constants and types------------------------------------------------*/

// fdeng mask this for debug wlan in Android! 
//
//#pragma pack(1)

//
// This structure contains the information about task data block which is 
// used to store the task inforamtion created by ourselves.
// 
//
typedef struct _TASK_DATA_BLOCK {
	
	DSP_LIST_ENTRY_T 	Link;
//	struct delayed_work	dwork; 	
//	struct work_struct		work;
//	PVOID				adap;
	
	// in list flag. If this block is in free or task list, this flag is set as 1.
	// And if this block is not in any lists, this flag is set as 0. This flag is used
	// to determine whether this block is in list or not. If this block is already in
	// list, driver must not insert this block into list again.;

	UINT32	task_event	:8;
	UINT32	task_pri		:8;
	UINT32	in_list_flag	:1;
	UINT32	data_len	:12;
	UINT32	reserved	:3;

	UINT8	ID;
	UINT8	*data;
//	UINT8	data[MAXTASKDATAN];

} TASK_DATA_BLOCK_T, *PTASK_DATA_BLOCK_T;


// This structure contains the information about task module
typedef struct _DSP_TASK 
{

	PTASK_DATA_BLOCK_T  ptask_block;	// Head pointer of total task block.
	DSP_LIST_ENTRY_T  task_free_pool;	// Task pool
	DSP_LIST_ENTRY_T  task_list;			// Task current used list
	DSP_LIST_ENTRY_T  task_list_high;		// Task current used list for high level	
	
	TDSP_SPINLOCK lock;			// Spin lock for task module
	UINT8	exit_thread_flag;	// If driver is halt, this flag is set
	PUINT8	ptask_block_buffer;

	TDSP_WORKLET worklet;
//	struct task_struct *task_thread;
//	TDSP_EVENT event;
} DSP_TASK_T, *PDSP_TASK_T;



// fdeng mask this for debug wlan in Android! 
//
//#pragma pack()



/*--variables---------------------------------------------------------*/
	
/*--function prototypes-----------------------------------------------*/
TDSP_STATUS Task_Init(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Task_Release( PDSP_ADAPTER_T pAdap);
VOID Task_Reset(PDSP_TASK_T pTask);
TDSP_STATUS Task_CreateTask(PDSP_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
#define Task_CreateTaskForce_DEL(pTask, event, pri, para, len)  Task_CreateTask(pTask, event, ((pri & 0xff) | 0x100), para, len)
//TDSP_STATUS Task_CreateTaskForce_DEL(PDSP_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
UINT32 Task_CheckExistTask(PDSP_TASK_T pTask, UINT32 event);
UINT32 Task_RemoveExistTask(PDSP_TASK_T pTask, UINT32 event);
UINT32 Task_RemoveAllExistTask(PDSP_TASK_T pTask);
UINT32 Task_RemoveAllMngFrameTask(PDSP_TASK_T pTask);
PTASK_DATA_BLOCK_T Task_GetHighLevelTask_DEL(PDSP_TASK_T pTask);

/* VOID Task_Test1(UINT32 pri, UINT32 len, PUCHAR data);
VOID Task_Test2(UINT32 pri, UINT32 len, PUCHAR data);
VOID Task_Test3(UINT32 pri, UINT32 len, PUCHAR data);
VOID Task_Test4(UINT32 pri, UINT32 len, PUCHAR data); */
#endif
