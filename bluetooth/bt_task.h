#ifndef BT_TASK_H
#define BT_TASK_H

/***********************************************************************
 *
 * FILENAME:    BT_Task.h      CURRENT VERSION: 1.00.01
 * CREATE DATE: 2005/08/15
 * PURPOSE:      mainly define the structrue that store the information
 *               about task module and the external function for task
 *               module.
 *
 * AUTHORS:      jason dong
 *
 *
 **********************************************************************/


#include "bt_queue.h"
/*--macros------------------------------------------------------------*/
#define MAXTASKN        (64)   /* The numbers of the total task  in the free task pool*/
#define MAXTASKDATAN    (1280) //(2048)  /* The bytes of each task having */

/* The pri value  of our own task */
#define BT_TASK_PRI_NORMAL     (50)
#define BT_TASK_PRI_HIGH       (20)
#define BT_TASK_PRI_LOW        (80)

/* The event type */
#define BT_TASK_EVENT_TYPE_HCI2LMP		(1)
#define BT_TASK_EVENT_TYPE_DSP2LMP		(2)
#define BT_TASK_EVENT_TYPE_HCI2HC		(3)
#define BT_TASK_EVENT_TYPE_DSP2HC		(4)
#define BT_TASK_EVENT_TYPE_HC2HCI		(5)
#define BT_TASK_EVENT_TYPE_NORMAL		(6)
#define BT_TASK_EVENT_TYPE_LMP2HCI		(7)
#define BT_TASK_EVENT_TYPE_HCI2LOOPBACK	(8)
#define BT_TASK_EVENT_TYPE_HCI2TESTMODE	(9)

/* The event number of task */
// Define HCI2LMP event num
#define BT_TASK_EVENT_HCI2LMP_TEST						(1)
#define BT_TASK_EVENT_HCI2LMP_CREATE_CONNECTION			(2)
#define BT_TASK_EVENT_HCI2LMP_LINK_SUPERVISION_TIMEOUT	(3)
#define BT_TASK_EVENT_HCI2LMP_CLK_OFFSET_REQ			(4)
#define BT_TASK_EVENT_HCI2LMP_NAME_REQ					(5)
#define BT_TASK_EVENT_HCI2LMP_FEATURE_REQ				(6)
#define BT_TASK_EVENT_HCI2LMP_VERSION_REQ				(7)
#define BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN				(8)
#define BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_CONN			(9)
#define BT_TASK_EVENT_HCI2LMP_DETACH					(10)
#define BT_TASK_EVENT_HCI2LMP_LINK_KEY_REPLY			(11)
#define BT_TASK_EVENT_HCI2LMP_PIN_CODE_NEG_REPLY		(12)
#define BT_TASK_EVENT_HCI2LMP_PIN_CODE_REPLY			(13)
#define BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_ON			(14)
#define BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_OFF		(15)
#define BT_TASK_EVENT_HCI2LMP_AUTH_REQUEST				(16)
#define BT_TASK_EVENT_HCI2LMP_ADD_SCO					(17)
#define BT_TASK_EVENT_HCI2LMP_REMOVE_SCO				(18)
#define BT_TASK_EVENT_HCI2LMP_HODE_REQ					(19)
#define BT_TASK_EVENT_HCI2LMP_SNIFF_REQ					(20)
#define BT_TASK_EVENT_HCI2LMP_EXIT_SNIFF_REQ			(21)
#define BT_TASK_EVENT_HCI2LMP_SET_AFH					(22)
#define BT_TASK_EVENT_HCI2LMP_CHANGE_MAX_SLOT			(23)
#define BT_TASK_EVENT_HCI2LMP_ROLE_SWITCH				(24)
#define BT_TASK_EVENT_HCI2LMP_POWER_UP					(25)
#define BT_TASK_EVENT_HCI2LMP_POWER_DOWN				(26)
#define BT_TASK_EVENT_HCI2LMP_ADD_ESCO					(27)
#define BT_TASK_EVENT_HCI2LMP_CHANGE_ESCO				(28)
#define BT_TASK_EVENT_HCI2LMP_ACCEPT_ESCO_CONN			(29)
#define BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_ESCO_CONN		(30)
#define BT_TASK_EVENT_HCI2LMP_REMOVE_ESCO				(31)
#define BT_TASK_EVENT_HCI2LMP_SET_CLASSIFICATION		(32)
#define BT_TASK_EVENT_HCI2LMP_QOS_SETUP					(33)
#define BT_TASK_EVENT_HCI2LMP_FLOW_SPECIFICATION		(34)
#define BT_TASK_EVENT_HCI2LMP_CLOCK_READY				(35)
//Jakio20071025: add here for EDR
#define     BT_TASK_EVENT_HCI2LMP_CHANGE_SCO_PACKET_TYPE	(36)
#define     BT_TASK_EVENT_HCI2LMP_CHANGE_PACKET_TYPE		(37)
#define BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE			(38)
#define     BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND			(39)
//NEWLY ADDED
#define     BT_TASK_EVENT_HCI2LMP_EXTENDED_FEATURE_REQ		(40)
#define     BT_TASK_EVENT_HCI2LMP_REFRESH_ENCRYPTION_KEY     (41)
#define     BT_TASK_EVENT_HCI2LMP_SNIFF_SUBRATING                    (42)
//jakio20080227: add a task to write afh registers
#define     BT_TASK_EVENT_HCI2LMP_WRITE_AFH_REGISTERS			(43)


// Define DSP2LMP event num
#define BT_TASK_EVENT_DSP2LMP_TEST       (1)


// Define HCI2HC event num
#define BT_TASK_EVENT_HCI2HC_SEND_NUM_OF_COMP_PACKET_EVENT			(1)
#define BT_TASK_EVENT_HCI2HC_SEND_CONNECTION_COMPLETE_EVENT			(2)
#define BT_TASK_EVENT_HCI2HC_SEND_DISCONNECTION_COMPLETE_EVENT		(3)
#define BT_TASK_EVENT_HCI2HC_SEND_CLKOFFSET_COMPLETE_EVENT			(4)
#define BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMEREQ_COMPLETE_EVENT		(5)
#define BT_TASK_EVENT_HCI2HC_SEND_REMOTE_NAMESUPPFEA_COMPLETE_EVENT	(6)
#define BT_TASK_EVENT_HCI2HC_SEND_REMOTE_VERINFO_COMPLETE_EVENT		(7)
#define BT_TASK_EVENT_HCI2HC_SEND_CONN_REQ_EVENT					(8)
#define BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_REQ_EVENT				(9)
#define BT_TASK_EVENT_HCI2HC_SEND_LINK_KEY_NOTIFICATION_EVENT		(10)
#define BT_TASK_EVENT_HCI2HC_SEND_PIN_CODE_REQ_EVENT				(11)
#define BT_TASK_EVENT_HCI2HC_SEND_ENCRYPTION_CHANGE_EVENT			(12)
#define BT_TASK_EVENT_HCI2HC_SEND_AUTHENTICATION_COMPLETE_EVENT		(13)
#define BT_TASK_EVENT_HCI2HC_SEND_CONN_PACKET_TYPE_CHANGED_EVENT	(14)
#define BT_TASK_EVENT_HCI2HC_SEND_SCO_CONNECTION_COMPLETE_EVENT		(15)
#define BT_TASK_EVENT_HCI2HC_SEND_SCO_DISCONNECTION_COMPLETE_EVENT	(16)
#define BT_TASK_EVENT_HCI2HC_SEND_MODE_CHANGE_EVENT					(17)
#define BT_TASK_EVENT_HCI2HC_SEND_ROLE_CHANGE_EVENT					(18)
#define BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_COMPLETE_EVENT	(19)
#define BT_TASK_EVENT_HCI2HC_SEND_SYNC_CONNECTION_CHANGED_EVENT		(20)
#define BT_TASK_EVENT_HCI2HC_SEND_QOS_SETUP_COMPLETE_EVENT			(21)
#define BT_TASK_EVENT_HCI2HC_SEND_FLOW_SPECIFICATION_COMPLETE_EVENT		(22)
//Jakio20071025: add here for EDR
#define     BT_TASK_EVENT_HCI2HC_SEND_SCO_CONN_PACKET_TYPE_CHANGED_EVENT		(23)
#define     BT_TASK_EVENT_HCI2HC_SEND_MAX_SLOT_CHANGED_EVENT					(24)
#define     BT_TASK_EVENT_HCI2HC_SEND_HARDWARE_ERROR_EVENT					(25)


#define     BT_TASK_EVENT_HCI2HC_SEND_RETURN_LINK_KEYS_EVENT	(26)
#define     BT_TASK_EVENT_HCI2HC_SEND_REMOTE_EXTENDED_FEA_COMPLETE_EVENT	(27)
#define     BT_TASK_EVENT_HCI2HC_SEND_LINK_SUPERVISION_TIMEOUT_CHANGE_EVENT (28)
#define     BT_TASK_EVENT_HCI2HC_SEND_REFRESH_ENCRYPTION_KEY_CHANGED_COMPLETE_EVENT (29)
#define     BT_TASK_EVENT_HCI2HC_SEND_SNIFF_SUBRATING_EVENT  (30)
#define      BT_TASK_EVENT_HCI2HC_SEND_IO_CAPABILITY_REQUEST_EVENT  (31)
#define      BT_TASK_EVENT_HCI2HC_SEND_IO_CAPABILITY_RESPONSE_EVENT  (32)
#define      BT_TASK_EVENT_HCI2HC_SEND_USER_CONFIRMATION_REQUEST_EVENT  (33)
#define      BT_TASK_EVENT_HCI2HC_SEND_USER_PASSKEY_REQUEST_EVENT  (34)
#define      BT_TASK_EVENT_HCI2HC_SEND_REMOTE_OOB_DATA_REQUEST_EVENT  (35)
#define      BT_TASK_EVENT_HCI2HC_SEND_SIMPLE_PAIRING_COMPLETE_EVENT  (36)
#define      BT_TASK_EVENT_HCI2HC_SEND_ENHANCED_FLUSH_COMPLETE_EVENT  (37)
#define      BT_TASK_EVENT_HCI2HC_SEND_USER_PASSKEY_NOTIFICATION_EVENT (38)
#define      BT_TASK_EVENT_HCI2HC_SEND_KEYPRESS_NOTIFICATION_EVENT (39)
#define      BT_TASK_EVENT_HCI2HC_SEND_REMOTE_HOST_SUPPORTED_FEATURES_NOTIFICATION_EVENT  (40)


// Define DSP2HC event num
#define BT_TASK_EVENT_DSP2HC_TEST        (1)


// Define HC2HCI event num
#define BT_TASK_EVENT_HC2HCI_RESET       (1)

// Define Normal event num
#define BT_TASK_EVENT_NORMAL_SEND_EVENT						(1)
#define BT_TASK_EVENT_NORMAL_ADD_SCO						(2)
#define BT_TASK_EVENT_NORMAL_SEND_CONN_REQ					(3)
#define BT_TASK_EVENT_NORMAL_RES_DISCARD_COMMAND			(4)
#define BT_TASK_EVENT_NORMAL_REL_RESOURCE_FOR_REMOTE_REQ	(5)
#define BT_TASK_EVENT_CHECK_HCI_LMP_TIMEOUT					(6)
#define BT_TASK_EVENT_RX_INT_PROCESS							(7)
#define BT_TASK_EVENT_RX_POLL_FRAME_PROCESS				(8)
#define BT_TASK_EVENT_RX_CRCERROR_FRAME_PROCESS			(9)
#define BT_TASK_EVENT_RX_AFH_CH_BAD_FRAME_PROCESS			(10)
#define BT_TASK_EVENT_RX_LMP_PDU_PROCESS					(11)
#define BT_TASK_EVENT_NORMAL_LEAVE_SNIFF					(12)
#define BT_TASK_EVENT_NORMAL_ENTER_SNIFF					(13)
#define BT_DSP_INT_ENTER_SNIFF								(14)
#define BT_TASK_EVENT_NORMAL_EDR_MODE_CHANGE                		(15)
#define BT_TASK_EVENT_NORMAL_RESUME_ENCRYPTION              		(16)
#define BT_TASK_EVENT_ROLE_CHANGE_INT                       			(17)
#define BT_TASK_EVENT_ROLE_CHANGE_FAIL_INT                  			(18)
#define BT_TASK_EVENT_POWER_STATE_D0						(19)
#define BT_TASK_EVENT_POWER_STATE_D3						(20)
#define BT_TASK_EVENT_START_STAGE_MAINLOOP						(21)
#define BT_TASK_EVENT_RX_FHS_FRAME_PROCESS						(22)	//Jakio20080226:add for fhs(poll) process
#define BT_TASK_EVENT_INQUIRY_RESULT_PROCESS						(23)	//Jakio20080305:add for inquiry result process
#define BT_TASK_EVENT_STOP_AND_GO						(24)
#define BT_TASK_EVENT_REQUEST_8051_MAINLOOP						(25)
#define BT_TASK_EVENT_RESUBMIT_READ_IRP							(26)
#define BT_TASK_EVENT_RESUBMIT_INQUIRY_IRP						(27)
#define BT_TASK_EVENT_WRITE_TEST_CMD_INDICATOR					(28)	//Jakio20080724: add for testmode
#define BT_TASK_EVENT_PROCESS_FLUSH								(29)	//Jakio20080811: add for flush
#define BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_SEND				(30)	//Jakio20081024: add for role switch
#define BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_RECV				(31)	//Jakio20081024: add for role switch
/* For connection management */
#define BT_TASK_EVENT_POLL_TIMER                        (32)

#define BT_TASK_EVENT_CHECK_SCO_SNIFF							(39)
#define BT_TASK_EVENT_SEND_SCO_LINK_REQ_PDU						(40)

// Define LMP2HCI event num
#define BT_TASK_EVENT_LMP2HCI_TEST								(1)
#define BT_TASK_EVENT_LMP2HCI_SETUP_COMPLETE					(2)
#define BT_TASK_EVENT_LMP2HCI_DETACH							(3)
#define BT_TASK_EVENT_LMP2HCI_CLKOFFSET_RES						(4)
#define BT_TASK_EVENT_LMP2HCI_NAME_RES							(5)
#define BT_TASK_EVENT_LMP2HCI_FEATURE_RES						(6)
#define BT_TASK_EVENT_LMP2HCI_VERSION_RES						(7)
#define BT_TASK_EVENT_LMP2HCI_HOST_CONN_REQ						(8)
#define BT_TASK_EVENT_LMP2HCI_LMP_TIMEOUT						(9)
#define BT_TASK_EVENT_LMP2HCI_LINK_SUPERVISION_TIMEOUT			(10)
#define BT_TASK_EVENT_LMP2HCI_LINK_KEY_REQ						(11)
#define BT_TASK_EVENT_LMP2HCI_AUTH_FAILURE						(12)
#define BT_TASK_EVENT_LMP2HCI_LINK_KEY_NOTIFICATION				(13)
#define BT_TASK_EVENT_LMP2HCI_PIN_CODE_REQ						(14)
#define BT_TASK_EVENT_LMP2HCI_PAIR_NOT_ALLOW					(15)
#define BT_TASK_EVENT_LMP2HCI_ENCRYPTION_SUCCESS				(16)
#define BT_TASK_EVENT_LMP2HCI_ENCRYPTION_FAILURE				(17)
#define BT_TASK_EVENT_LMP2HCI_ENCRYPTION_NOT_COMP				(18)
#define BT_TASK_EVENT_LMP2HCI_AUTH_COMP_SUCCESS					(19)
#define BT_TASK_EVENT_LMP2HCI_AUTH_COMP_FAILURE					(20)
#define BT_TASK_EVENT_LMP2HCI_SCO_LINK_REQ						(21)
#define BT_TASK_EVENT_LMP2HCI_SCO_LMP_TIMEOUT					(22)
#define BT_TASK_EVENT_LMP2HCI_SCO_DETACH						(23)
#define BT_TASK_EVENT_LMP2HCI_SCO_CONNECTED						(24)
#define BT_TASK_EVENT_LMP2HCI_SCO_REMOVED						(25)
#define BT_TASK_EVENT_LMP2HCI_MODE_CHANGE						(26)
#define BT_TASK_EVENT_LMP2HCI_ROLE_SWITCH_FAIL					(27)
#define BT_TASK_EVENT_LMP2HCI_ESCO_CONNECTED					(28)
#define BT_TASK_EVENT_LMP2HCI_ESCO_CHANGED						(29)
#define BT_TASK_EVENT_LMP2HCI_ESCO_CHANGE_FAIL					(30)
#define BT_TASK_EVENT_LMP2HCI_ESCO_REMOVED						(31)
#define BT_TASK_EVENT_LMP2HCI_ESCO_LINK_REQ						(32)
#define BT_TASK_EVENT_LMP2HCI_CHANGE_TX_MAX_SLOT				(33)
#define BT_TASK_EVENT_LMP2HCI_QOS_SETUP_COMPLETE				(34)
#define BT_TASK_EVENT_LMP2HCI_FLOW_SPECIFICATION_COMPLETE		(35)
//Jakio20071025: add  here for EDR
#define     BT_TASK_EVENT_LMP2HCI_CHANGE_SCO_PACKET_TYPE_COMPLETE	(36)
#define     BT_TASK_EVENT_LMP2HCI_CHANGE_PACKET_TYPE_COMPLETE		(37)
#define     BT_TASK_EVENT_LMP2HCI_EXTENDED_FEATURE_RES				(38)
#define     BT_TASK_EVENT_LMP2HCI_REFRESH_ENCRYPTION_KEY     (40)

// Define HCI2Loopback event num
#define BT_TASK_EVENT_HCI2LOOPBACK_RESPONCE_CONN_HANDLES_FOR_LOCAL_LOOPBACK				(1)
#define BT_TASK_EVENT_HCI2LOOPBACK_SEND_CONN_COMPLETE_EVENT								(2)
#define BT_TASK_EVENT_HCI2LOOPBACK_EXIT_LOCAL_LOOPBACK									(3)
#define BT_TASK_EVENT_HCI2LOOPBACK_SEND_DISCONN_COMPLETE_EVENT							(4)
#define BT_TASK_EVENT_HCI2LOOPBACK_SEND_COMMAND_TO_HC									(5)
#define BT_TASK_EVENT_HCI2LOOPBACK_SEND_ACL_DATA_TO_HC									(6)
#define BT_TASK_EVENT_HCI2LOOPBACK_SEND_SCO_DATA_TO_HC									(7)

// Define HCI2Testmode event num
#define BT_TASK_EVENT_HCI2TESTMODE_START_TESTER_DATA									(1)

#define TASK_MIN(a, b) (((a) < (b)) ? (a) : (b))

/* The event macros */
#define BT_TASK_HCI2LMP_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_HCI2LMP << 16) | (UINT32)x)
#define BT_TASK_DSP2LMP_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_DSP2LMP << 16) | (UINT32)x)
#define BT_TASK_HCI2HC_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_HCI2HC << 16) | (UINT32)x)
#define BT_TASK_DSP2HC_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_DSP2HC << 16) | (UINT32)x)
#define BT_TASK_HC2HCI_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_HC2HCI << 16) | (UINT32)x)
#define BT_TASK_NORMAL_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_NORMAL << 16) | (UINT32)x)
#define BT_TASK_LMP2HCI_EVENT(x)		(((UINT32)BT_TASK_EVENT_TYPE_LMP2HCI << 16) | (UINT32)x)
#define BT_TASK_HCI2LOOPBACK_EVENT(x)	(((UINT32)BT_TASK_EVENT_TYPE_HCI2LOOPBACK << 16) | (UINT32)x)
#define BT_TASK_HCI2TESTMODE_EVENT(x)	(((UINT32)BT_TASK_EVENT_TYPE_HCI2TESTMODE << 16) | (UINT32)x)

#define BT_TASK_GET_EVENT_TYPE(x)    (x >> 16)
#define BT_TASK_GET_EVENT_NUMBER(x)    (x & 0x0000ffff)

/*--constants and types------------------------------------------------*/

#pragma pack(1)

#pragma pack()

//
// This structure contains the information about task data block which is
// used to store the task inforamtion created by ourselves.
//
//
typedef struct _TASK_DATA_BLOCK
{

	// For link
	BT_LIST_ENTRY_T Link;

	//Jakio20080825: for Linux thread
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)

	struct delayed_work	dwork;
#else

	struct work_struct dwork;
#endif

	UINT8   tmpFlag;
	PVOID	devExt;

	// in list flag. If this block is in free or task list, this flag is set as 1.
	// And if this block is not in any lists, this flag is set as 0. This flag is used
	// to determine whether this block is in list or not. If this block is already in
	// list, driver must not insert this block into list again.;
	UINT8 in_list_flag;

	// Task event
	UINT32 task_event;
	// Task pri
	UINT32 task_pri;
	// Task data length
	UINT32 data_len;
	// Task para
	UINT8 data[MAXTASKDATAN];

} TASK_DATA_BLOCK_T,  *PTASK_DATA_BLOCK_T;


//
// This structure contains the information about task module
//
//
typedef struct _BT_TASK
{
    void *pDevExt;
	// Head pointer of total task block.
	PTASK_DATA_BLOCK_T ptask_block;
	// Task pool
	BT_LIST_ENTRY_T task_free_pool;

	//Jakio20080301:Task pool for rx data
	BT_LIST_ENTRY_T Task_RxFree_Pool;
	// Task current used list
	BT_LIST_ENTRY_T task_list;

	
	// Spin lock for task module
	KSPIN_LOCK lock;

	// Delay task current used list
	BT_LIST_ENTRY_T  delay_task_list;

	
	//work queue
	struct workqueue_struct *queue;

} BT_TASK_T,  *PBT_TASK_T;







/*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/

NTSTATUS Task_Init(PBT_DEVICE_EXT devExt);
NTSTATUS Task_Release(PBT_DEVICE_EXT devExt);
NTSTATUS Task_Stop(PBT_TASK_T pTask);

// VOID Task_SetExitThreadFlag(PBT_TASK_T pTask);
// VOID Task_StartThread(PBT_DEVICE_EXT devExt);
VOID Task_Reset(PBT_TASK_T pTask);
NTSTATUS Task_CreateTask(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
NTSTATUS Task_CreateTaskForRxData(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
NTSTATUS Task_CreateDelayTask(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
NTSTATUS Task_CreateTaskForce(PBT_TASK_T pTask, UINT32 event, UINT32 pri, PUINT8 para, UINT32 len);
BOOLEAN Task_CheckExistTask(PBT_TASK_T pTask, UINT32 event);
//VOID Task_SetExitThreadFlag(PBT_TASK_T pTask);
VOID Task_StartThread(PBT_DEVICE_EXT devExt);




VOID Task_HC2HCI_Reset(PBT_DEVICE_EXT devExt);
VOID Task_Reset_BeforeFirmwareReset(PBT_DEVICE_EXT devExt);
VOID Task_Normal_Send_Event(PBT_DEVICE_EXT devExt, UINT8 eventcode, UINT8 urgent);
VOID Task_HCI2HC_Send_Num_Of_Comp_Packet_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Disconnection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_ClkOffset_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Remote_NameReq_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_RemoteExtendedFea_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_RemoteVerInfo_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Conn_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Link_Key_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Link_Key_Notification_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Pin_Code_Req_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Encryption_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Authentication_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sco_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Mode_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_Normal_Add_Sco(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_Normal_Send_Conn_Req(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_Normal_Res_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode, UINT8 urgent);
VOID Task_Normal_Rel_Resouce_For_Remote_Req(PBT_DEVICE_EXT devExt, PUINT8 para);
VOID Task_HCI2HC_Send_Role_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sync_Connection_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sync_Connection_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Qos_Setup_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Flow_Specification_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Max_Slot_Changed_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Hardware_Error_Event(PBT_DEVICE_EXT devExt, UINT8 Hardware_Code, UINT8 urgent);
VOID Task_HCI2HC_Send_Link_Supervision_Timeout_Change_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Return_Link_Keys_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Refresh_Encryption_Key_Changed_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Sniff_Subrating_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_IO_Capability_Request_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_IO_Capability_Response_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_User_Confirmation_Request_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_User_Passkey_Request_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Remote_OOB_Data_Request_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Simple_Pairing_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Enhanced_Flush_Complete_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_User_Passkey_Notification_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Keypress_Notification_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
VOID Task_HCI2HC_Send_Remote_Host_Supported_Features_Notification_Event(PBT_DEVICE_EXT devExt, PUINT8 para, UINT8 urgent);
/* VOID Task_HCI2LMP_Test(UINT32 pri, UINT32 len, PUINT8 data);
VOID Task_DSP2LMP_Test(UINT32 pri, UINT32 len, PUINT8 data);
VOID Task_HCI2HC_Test(UINT32 pri, UINT32 len, PUINT8 data);
VOID Task_DSP2HC_Test(UINT32 pri, UINT32 len, PUINT8 data);  */
#endif
