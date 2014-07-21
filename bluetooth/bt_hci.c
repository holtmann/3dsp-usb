/***********************************************************************
 * FILENAME:     BT_HCI.c
 * CURRENT VERSION: 1.0.0 (optional)
 * CREATE DATE:  2005/08/10
 * PURPOSE:      realize the operaiton of HCI module...
 *                ...
 *
 * AUTHORS:      jason dong
 *
 * NOTES:        description of constraints when using functions of this file
 *
 ***********************************************************************/

#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"        // include some hci operation
#include "bt_task.h"
#include "bt_lmp.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_usb_vendorcom.h"

#include "bt_frag.h"			
#include "afhclassify.h"

#ifdef BT_SCHEDULER_SUPPORT
	#include "sched.h"
#endif
#ifdef BT_LOOPBACK_SUPPORT
	#include "bt_loopback.h"
#endif
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
	#include "afhclassify.h"
#endif
#ifdef BT_SERIALIZE_DRIVER
	#include "bt_serialize.h"
#endif
/*--file local constants and types-------------------------------------*/
/*--file local macros--------------------------------------------------*/
/*--file local variables-----------------------------------------------*/
/*--file local function prototypes-------------------------------------*/
/*--functions --------------------------------------------------------*/
/**********************************************************
 * Hci_Init
 *
 * Description:
 *    This function intialize the HCI module including allocate
 *	 memory for the module and initialize some vars.
 * Arguments:
 *    devExt: IN, pointer to device extension of device to start.
 *
 * Return Value:
 *	 STATUS_SUCCESS.   HCI module memory is allocated with succesfully
 *    STATUS_UNSUCCESSFUL. HCI module   memory is allocated with fail
 *
 **********************************************************/
NTSTATUS Hci_Init(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	UINT16 i;
	LARGE_INTEGER timevalue;
	UINT32 tmp_mask[2] = {BT_HCI_EVENT_MASK_DEFAULT0, BT_HCI_EVENT_MASK_DEFAULT1};


	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init\n");
	pHci = (PBT_HCI_T)ExAllocatePool(sizeof(BT_HCI_T), GFP_KERNEL);
	if (pHci == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Init()--Allocate hci memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	// Zero out the hci module space
	RtlZeroMemory(pHci, sizeof(BT_HCI_T));
	// Initialize spin lock for HCI module
	KeInitializeSpinLock(&pHci->HciLock);
    
#ifndef SERIALIZE_ALL_THREADS
	/* Initialize the poll timer */
	KeInitializeTimer(&pHci->PollTimer);
	/* Initialize the poll timer DPC */
	KeInitializeDpc(&pHci->PollDPC, Hci_PollTimerRoutine, devExt);
#endif /*SERIALIZE_ALL_THREADS*/

	QueueInitList(&pHci->device_free_list);
	QueueInitList(&pHci->device_am_list);
	QueueInitList(&pHci->device_pm_list);
	QueueInitList(&pHci->device_slave_list);
	QueueInitList(&pHci->sco_device_free_list);
	QueueInitList(&pHci->inquiry_result_free_list);
	QueueInitList(&pHci->inquiry_result_used_list);
	for (i = 0; i < BT_MAX_INQUIRY_RESULT_NUM; i++)
	{
		QueuePutTail(&pHci->inquiry_result_free_list, &pHci->inquiry_result_all[i].Link);
	}
	for (i = 0; i < BT_MAX_DEVICE_NUM; i++)
	{
		QueuePutTail(&pHci->device_free_list, &pHci->device_all[i].Link);
	}
	for (i = 0; i < BT_MAX_SCO_DEVICE_NUM; i++)
	{
		pHci->sco_device_all[i].index = (UINT8)i;
		QueuePutTail(&pHci->sco_device_free_list, &pHci->sco_device_all[i].Link);
	}
	RtlZeroMemory(&pHci->addr_table, sizeof(ADDRESS_TABLE_T));
	pHci->addr_table.am_latest_addr = 1;
	pHci->num_inquiry_result_used = 0;
	pHci->num_device_am = 0;
	pHci->num_device_pm = 0;
	pHci->num_device_slave = 0;
#ifdef ACCESS_REGISTER_DIRECTLY
	Hci_Write_One_Byte(devExt, BT_REG_AM_CONNECTION_INDICATOR, 0);
	Hci_Write_One_Byte(devExt, BT_REG_AM_CONNECTION_SLAVE_INDICATOR, 0);
	
#else
	BtUsbHciInit(devExt);
#endif
	pHci->current_opcode = BT_INVALID_OPCODE;
	pHci->command_state = BT_COMMAND_STATE_IDLE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->acl_data_packet_length = BT_MAX_ACL_DATA_PACKET_LEN;
	pHci->sco_data_packet_length = 48; //BT_MAX_SCO_DATA_PACKET_LEN;
	pHci->total_num_acl_data_packet = BT_TOTAL_NUM_ACL_DATA_PACKET;
	pHci->total_num_sco_data_packet = 5;//BT_TOTAL_NUM_SCO_DATA_PACKET;
#ifdef BT_ENHANCED_RATE
    #ifdef BT_2_1_SPEC_SUPPORT
    pHci->hci_version = BT_HCI_VERSION_21;
    pHci->hci_revision = 0; // ??
    pHci->lmp_version = LMP_VERSION_V21;
    pHci->manufacturer_name = MANUFACTURER_3DSP_NAME;
    pHci->lmp_subversion = 0; // ??
    #else
    pHci->hci_version = BT_HCI_VERSION_20;
    pHci->hci_revision = 0; // ??
    pHci->lmp_version = LMP_VERSION_V20;
    pHci->manufacturer_name = MANUFACTURER_3DSP_NAME;
    pHci->lmp_subversion = 0; // ??
    #endif
#else
	pHci->hci_version = BT_HCI_VERSION_12;
	pHci->hci_revision = 0; // ??
	pHci->lmp_version = LMP_VERSION_V12;
	pHci->manufacturer_name = MANUFACTURER_3DSP_NAME;
	pHci->lmp_subversion = 0; // ??
#endif
	/* changed here for EDR
	pHci->hci_version = BT_HCI_VERSION_12;
	pHci->hci_revision = 0; // ??
	pHci->lmp_version = LMP_VERSION_V12;
	pHci->manufacturer_name = MANUFACTURER_3DSP_NAME;
	pHci->lmp_subversion = 0; 
	*/
	
	// Initialize some vars about Read/write BD addr
	RtlZeroMemory(pHci->local_bd_addr, BT_BD_ADDR_LENGTH); // ??
	// Initialize some vars about write_connection_accept_timeout
	pHci->conn_accept_timeout = 0x1fa0; // default time is 5 seconds
	// Initialize some vars about write_page_timeout
	pHci->page_timeout = 0x2000; // default time is 5.12 seconds
	//BT_HAL_WRITE_WORD((PUINT16)((PUINT8)devExt->BaseRegisterAddress + BT_REG_PAGE_TIMEOUT), pHci->page_timeout);
	Hci_Write_One_Word(devExt, BT_REG_PAGE_TIMEOUT, pHci->page_timeout);
	// Initialize some vars about write_pin_type
	pHci->pin_type = BT_PIN_TYPE_VARIABLE;
	// Initialize some vars about read_local_supported_features, now we support nothing
	pHci->lmp_features.byte0.slot3 = 1;
	pHci->lmp_features.byte0.slot5 = 1;

#ifdef BT_ENCRYPTION_SUPPORT
	pHci->lmp_features.byte0.encry = 1;
#else
	pHci->lmp_features.byte0.encry = 0;
#endif

#ifdef BT_ROLESWITCH_SUPPORT
	pHci->lmp_features.byte0.slot_offset = 1;
#else
	pHci->lmp_features.byte0.slot_offset = 0;
#endif

	pHci->lmp_features.byte0.timing_accur = 1;

#ifdef BT_ROLESWITCH_SUPPORT
	pHci->lmp_features.byte0.switchbit = 1;
#else
	pHci->lmp_features.byte0.switchbit = 0;
#endif

	pHci->lmp_features.byte0.hold_mode = 0;

#ifdef BT_SNIFF_SUPPORT
	pHci->lmp_features.byte0.sniff_mode = 1;
#else
	pHci->lmp_features.byte0.sniff_mode = 0;
#endif

	pHci->lmp_features.byte1.park_mode = 0;

#ifdef BT_RSSI_SUPPORT
	pHci->lmp_features.byte1.rssi = 1;
#else
	pHci->lmp_features.byte1.rssi = 0;
#endif

	if(devExt->ComboState == FW_WORK_MODE_COMBO)
	{
		#ifdef BT_SNIFF_SUPPORT	//close rssi to save code space in DSP
		if(devExt->RssiWhenSniff == FALSE)
		{
			pHci->lmp_features.byte1.rssi = 0;	
		}
		#endif	
	}

	pHci->lmp_features.byte1.channel_qua = 1;
	pHci->lmp_features.byte1.sco_link = 1;
	
#ifdef BT_CLOSE_SCO_HV2_TYPE
	pHci->lmp_features.byte1.hv2 = 0;
#else
	pHci->lmp_features.byte1.hv2 = 1;
#endif

#ifdef BT_CLOSE_SCO_HV3_TYPE
	pHci->lmp_features.byte1.hv3 = 0;
#else
	pHci->lmp_features.byte1.hv3 = 1;
#endif

	pHci->lmp_features.byte1.u_law = 1;
	pHci->lmp_features.byte1.a_law = 1;
	pHci->lmp_features.byte2.cvsd = 1;
	pHci->lmp_features.byte2.paging_sch = 0;
	
#ifdef BT_DEVICE_CLASS_TYPE_2
		pHci->lmp_features.byte2.power_control = 0;		
#else
	#ifdef BT_POWER_CONTROL_SUPPORT
		pHci->lmp_features.byte2.power_control = 1;
	#else
		pHci->lmp_features.byte2.power_control = 0;
	#endif
#endif
	
	pHci->lmp_features.byte2.trans_sco = 0;
	pHci->lmp_features.byte2.flow_con_lag0 = 0;
	pHci->lmp_features.byte2.flow_con_lag1 = 0;
	pHci->lmp_features.byte2.flow_con_lag2 = 0;
	pHci->lmp_features.byte2.broadcast_encry = 0;
	pHci->lmp_features.byte3.reserved = 0;
	#ifdef BT_ENHANCED_RATE
	pHci->lmp_features.byte3.enh_rate_acl_2 = 1;
	pHci->lmp_features.byte3.enh_rate_acl_3 = 1;
	#else
	pHci->lmp_features.byte3.enh_rate_acl_2 = 0;
	pHci->lmp_features.byte3.enh_rate_acl_3 = 0;
	#endif
	pHci->lmp_features.byte3.enh_inq_scan = 0;
	pHci->lmp_features.byte3.int_inq_scan = 1;
	pHci->lmp_features.byte3.int_page_scan = 1;
	
#ifdef BT_INQUIRY_RESULT_WITH_RSSI_SUPPORT
	pHci->lmp_features.byte3.rssi_inq_res = 1;
#else
	pHci->lmp_features.byte3.rssi_inq_res = 0;
#endif

#ifdef BT_EXT_SCO_LINK_SUPPORT
	pHci->lmp_features.byte3.ext_sco_link = 1;
#else
	pHci->lmp_features.byte3.ext_sco_link = 0;
#endif

	pHci->lmp_features.byte4.ev4 = 0;
	pHci->lmp_features.byte4.ev5 = 0;
	pHci->lmp_features.byte4.reserved1 = 0;
	
#ifdef BT_AFH_SUPPORT
	pHci->lmp_features.byte4.afh_cap_slave = 1;
	pHci->lmp_features.byte4.afh_cla_slave = 1;
#else
	pHci->lmp_features.byte4.afh_cap_slave = 0;
	pHci->lmp_features.byte4.afh_cla_slave = 0;
#endif

	pHci->lmp_features.byte4.reserved2 = 0;
	pHci->lmp_features.byte4.reserved3 = 0;
	
#ifdef BT_ENHANCED_RATE
	pHci->lmp_features.byte4.slot3_enh_acl = 1;
	pHci->lmp_features.byte5.slot5_enh_acl = 1;
#else
	pHci->lmp_features.byte4.slot3_enh_acl = 0;
	pHci->lmp_features.byte5.slot5_enh_acl = 0;
#endif
	//pHci->lmp_features.byte4.slot3_enh_acl = 0;
	//pHci->lmp_features.byte5.slot5_enh_acl = 0;

	//Jakio20071127: seems to be lack something
	//pHci->lmp_features.byte5.reserved1 = 0;
	//pHci->lmp_features.byte5.reserved2 = 0;
#ifdef BT_AFH_SUPPORT
	pHci->lmp_features.byte5.afh_cap_master = 1;
	pHci->lmp_features.byte5.afh_cla_master = 1;
#else
	pHci->lmp_features.byte5.afh_cap_master = 0;
	pHci->lmp_features.byte5.afh_cla_master = 0;
#endif

	pHci->lmp_features.byte5.enh_rate_esco_2 = 0;
	pHci->lmp_features.byte5.enh_rate_esco_3 = 0;
	pHci->lmp_features.byte5.slot3_enh_esco = 0;
	RtlZeroMemory(pHci->local_name, BT_LOCAL_NAME_LENGTH); // ??
	/* Set the default friendly device name */
    RtlCopyMemory(pHci->local_name, "BTLinuxUSB", sizeof("BTLinuxUSB"));
    
	RtlZeroMemory(pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH); // ??
	pHci->class_of_device[0] = 0x4;
	pHci->class_of_device[1] = 0x1;
	pHci->class_of_device[2] = 0x12;
	pHci->authentication_enable = BT_AUTHENTICATION_DISABLE;
	pHci->encryption_mode = BT_ENCRYPTION_DIABLE;
	pHci->sco_flow_control_enable = 0; // Disable
	pHci->page_scan_interval = 0x800;
	pHci->page_scan_window = 0x12;
	pHci->inquiry_scan_interval = 0x800;
	pHci->inquiry_scan_window = 0x12;
	RtlZeroMemory(&pHci->scan_option, sizeof(SCAN_OPTION_T));
	pHci->auto_packet_select_flag = 1;
	pHci->sco_link_count = 0;
	pHci->protect_hci_command_ack_timer_valid = 0;
	pHci->protect_hci_command_ack_timer_count = 0;
	pHci->acl_temp_pending_flag = 0;
	pHci->inquiry_mode = BT_INQUIRYMODE_STANDED;
	pHci->inquiry_scan_type = BT_INQUIRYSCAN_TYPE_STANDED;
	pHci->page_scan_type = BT_PAGESCAN_TYPE_STANDED;
	pHci->afh_ch_assessment_mode = BT_CONTROLLER_ASSESSMENT_ENABLED;
	pHci->hold_mode_activity = 0;

	
	
#ifdef BT_USE_ONE_SLOT_FOR_ACL
	pHci->acl_force_1_slot = 1;
#else
	pHci->acl_force_1_slot = 0;
#endif
	pHci->am_connection_indicator = 0;
#ifdef BT_AUX1_INSTEAD_DH1
	pHci->aux1_instead_dh1_flag = 1;
#else
	pHci->aux1_instead_dh1_flag = 0;
#endif
	pHci->hv1_use_dv_instead_dh1_flag = 0;
	pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
	pHci->test_flag = 0;
	pHci->test_mode_active = 0;
	RtlFillMemory(pHci->afh_channel_map, BT_MAX_CHANNEL_MAP_NUM, 0xff);
	pHci->afh_timer_count = 0;
	pHci->afh_timer_valid = 1;
	pHci->num_current_iac = 1;
	pHci->is_in_inquiry_flag = 0;
	pHci->page_scan_mode = BT_PAGESCAN_MODE_MANDATORY;

	pHci->driver_send_poll_in_sco_flag = 1;

	pHci->page_extra_flag = 0;
	pHci->inquiry_extra_flag = 0;

	pHci->only_use_hv3_flag = 0;

	pHci->only_allow_one_acl_link_flag = 0;

	pHci->slave_sco_master_not_coexist_flag = 0;

	pHci->need_coordinate_dsp_state_flag = 0;

	pHci->role_switching_flag = 0;

	pHci->scan_enable = 2;
	pHci->scan_enable_mask = BT_FOR_LC_SCAN_TYPE_INQUIRY_SCAN | BT_FOR_LC_SCAN_TYPE_PAGE_SCAN;

	pHci->need_write_afh_command = 0;

	pHci->upper_set_classification = 0;

	RtlFillMemory(pHci->classification_channel_map, BT_MAX_CHANNEL_MAP_NUM, 0x55);
	pHci->period_inquiry_timer_valid = 0;
	pHci->period_inquiry_timer_count = 0;
	pHci->period_inquiry_flag = 0;
	pHci->max_period_length = 0;
	pHci->min_period_length = 0;
	pHci->period_inquiry_length = 0;
	pHci->period_num_responses = 0;

	memcpy(pHci->event_mask, tmp_mask, sizeof(pHci->event_mask));

	pHci->current_page_number = 0;
	pHci->extended_feature_pointer[0] = (PVOID)&pHci->lmp_features;
	pHci->voice_setting=0x0060;
	pHci->Default_Link_Policy_Settings=0x0005;
#ifdef BT_USE_ANTENNA_0
	Hci_Write_One_Byte(devExt, BT_REG_ANTENNA_MODE, 0);
#else
   	//BT_HAL_WRITE_BYTE((PUINT8)devExt->BaseRegisterAddress + BT_REG_ANTENNA_MODE, 1);
	Hci_Write_One_Byte(devExt, BT_REG_ANTENNA_MODE, 1);
#endif
	devExt->pHci = (PVOID)pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init()--The size of structure struct _BT_HCI: %d bytes\n", sizeof(BT_HCI_T));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init()--The size of structure _CONNECT_DEVICE: %d bytes\n", sizeof(CONNECT_DEVICE_T));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init()--The size of structure _SCO_CONNECT_DEVICE: %d bytes\n", sizeof(SCO_CONNECT_DEVICE_T));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init()--Set Poll Timer as zero to make timer signaled\n");
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Init()--The size of structure struct _HCI_SCO_HEADER: %d bytes\n", sizeof(HCI_SCO_HEADER_T));

	/*
	fhs_packet = (PFHS_PACKET_T)ExAllocatePool(NonPagedPool, sizeof(FHS_PACKET_T));
	if (fhs_packet == NULL)
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Init()--Allocate FHS PACKET memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	devExt->pFhsPacket = (PVOID)fhs_packet;
	*/
	#ifndef SERIALIZE_ALL_THREADS
		timevalue.QuadPart = 0;
		KeSetTimer(&pHci->PollTimer, timevalue, &pHci->PollDPC);
	#endif /*SERIALIZE_ALL_THREADS*/
	return STATUS_SUCCESS;
}

void PostInit(PBT_DEVICE_EXT devExt)
{
	FHS_PACKET_T fhs_packet;
	PUINT64 ptmplonglong;
	UINT8 tempaccesscode[9];
	UINT8 Datastr[12];
	UINT8 giac[3] =
	{
		0x33, 0x8b, 0x9e
	};
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tempChar;
	UINT16		length;
    PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;


    /******Step 1: Access code****/
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);

	AccessCode_Gen(pHci->local_bd_addr, tempaccesscode);
	RtlZeroMemory(Datastr, 12);
	UsbReadFrom3DspRegs(devExt, BT_REG_LOCAL_ACCESS_CODE + 8, 1, Datastr + 8);
	RtlCopyMemory(Datastr, tempaccesscode, 9);
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_LOCAL_ACCESS_CODE, pEle, 9, Datastr);
	
	ptmplonglong = (PUINT64)tempaccesscode;
	*ptmplonglong =  *ptmplonglong >> 4;
	RtlZeroMemory(&fhs_packet, sizeof(FHS_PACKET_T));
	RtlCopyMemory(&fhs_packet, ptmplonglong, 8);
	fhs_packet.undefined = 0;
	fhs_packet.sr = 0;
	#ifdef BT_ENHANCED_RATE
	fhs_packet.sp = 2; // This is reserved filed for EDR and it should be set as "10" (2). 
	#else
	fhs_packet.sp = 0;
	#endif
	fhs_packet.uap = pHci->local_bd_addr[3];
	fhs_packet.nap = *(PUINT16)(&pHci->local_bd_addr[4]);
	RtlCopyMemory(fhs_packet.class_of_device, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	fhs_packet.am_addr = 0;
	fhs_packet.page_scan_mode = 0;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_FHS_FOR_INQUIRY_SCAN, pEle, BT_FHS_PACKET_SIZE, (PUINT8) &fhs_packet);

	AccessCode_Gen(giac, tempaccesscode);
	RtlZeroMemory(Datastr, 12);
	UsbReadFrom3DspRegs(devExt, BT_REG_INQUIRY_SCAN_ACCESS_CODE + 8, 1, Datastr + 8);
	RtlCopyMemory(Datastr, tempaccesscode, 9);
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_SCAN_ACCESS_CODE, pEle, 9, Datastr);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pEle - buf));
	

	/* jakio20080805: read it back and verify
*/
	RtlZeroMemory(Datastr, 12);
	UsbReadFrom3DspRegs(devExt, BT_REG_INQUIRY_SCAN_ACCESS_CODE, 3, Datastr);
	if(!RtlEqualMemory(Datastr, tempaccesscode, 9))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Command_Read_BD_Addr----registers verify failed, write again\n");
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pEle - buf));
	}
}


/**************************************************************************
 *   Hci_Release
 *
 *   Descriptions:
 *      release HCI module, including free memory for the HCI
 *      module and so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Release(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Release\n");
	// Cancel poll timer .
	#ifndef SERIALIZE_ALL_THREADS
		KeCancelTimer(&pHci->PollTimer);
	#endif /*SERIALIZE_ALL_THREADS*/
	// Empty the device free list, device am list and device pm list
	QueueInitList(&pHci->device_free_list);
	QueueInitList(&pHci->device_am_list);
	QueueInitList(&pHci->device_pm_list);
	QueueInitList(&pHci->device_slave_list);
	// Empty sco device free list.
	QueueInitList(&pHci->sco_device_free_list);
	// Empty the inquiry result free list and inquiry used list
	QueueInitList(&pHci->inquiry_result_free_list);
	QueueInitList(&pHci->inquiry_result_used_list);
	// Set initial value
	pHci->num_inquiry_result_used = 0;
	pHci->num_device_am = 0;
	pHci->num_device_pm = 0;
	pHci->num_device_slave = 0;

	/*
	if(devExt->pFhsPacket)
	{
		ExFreePool(devExt->pFhsPacket);
		devExt->pFhsPacket = NULL;
	}
	*/
	
	// Free HCI module memory
	ExFreePool(pHci);
	// Set NULL pointer;
	devExt->pHci = NULL;
}
/**************************************************************************
 *   Hci_Add_Inquiry_Result
 *
 *   Descriptions:
 *      Add a inquiry result. If the result has already existed, we just update
 *      the information in it.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *      repetition_mode: IN,  page scan repetition mode
 *      period_mode: IN,  page scan period mode
 *      scan_mode: IN, page scan mode
 *      class_of_device: IN, class of device
 *      clock_offset: IN, clock offset
 *   Return Value:
 *      pointer to the inquiry result
 *************************************************************************/
PINQUIRY_RESULT_T Hci_Add_Inquiry_Result(PBT_HCI_T pHci, PUINT8 bd_addr, UINT8 repetition_mode, UINT8 period_mode, UINT8 scan_mode, PUINT8 class_of_device, UINT16 clock_offset)
{
	KIRQL oldIrql;
	PINQUIRY_RESULT_T pInquiryResult;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pInquiryResult = (PINQUIRY_RESULT_T)QueueGetHead(&pHci->inquiry_result_used_list);
	while (pInquiryResult != NULL)
	{
		if (RtlEqualMemory(pInquiryResult->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			pInquiryResult->page_scan_repetition_mode = repetition_mode;
			pInquiryResult->page_scan_period_mode = period_mode;
			pInquiryResult->page_scan_mode = scan_mode;
			RtlCopyMemory(pInquiryResult->class_of_device, class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
			pInquiryResult->clock_offset = clock_offset;
			break;
		}
		pInquiryResult = (PINQUIRY_RESULT_T)QueueGetNext(&pInquiryResult->Link);
	}
	if (pInquiryResult == NULL)
	{
		pInquiryResult = (PINQUIRY_RESULT_T)QueuePopHead(&pHci->inquiry_result_free_list);
		if (pInquiryResult != NULL)
		{
			RtlCopyMemory(pInquiryResult->bd_addr, bd_addr, BT_BD_ADDR_LENGTH);
			pInquiryResult->page_scan_repetition_mode = repetition_mode;
			pInquiryResult->page_scan_period_mode = period_mode;
			pInquiryResult->page_scan_mode = scan_mode;
			RtlCopyMemory(pInquiryResult->class_of_device, class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
			pInquiryResult->clock_offset = clock_offset;
			QueuePutTail(&pHci->inquiry_result_used_list, &pInquiryResult->Link);
			pHci->num_inquiry_result_used++;
		}
		else
		{
			pInquiryResult = (PINQUIRY_RESULT_T)QueueGetHead(&pHci->inquiry_result_used_list);
			if (pInquiryResult != NULL)
			{
				BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Add_Inquiry_Result()--There is no any free inquiry result, we just discard existed one and update it!\n");
				RtlCopyMemory(pInquiryResult->bd_addr, bd_addr, BT_BD_ADDR_LENGTH);
				pInquiryResult->page_scan_repetition_mode = repetition_mode;
				pInquiryResult->page_scan_period_mode = period_mode;
				pInquiryResult->page_scan_mode = scan_mode;
				RtlCopyMemory(pInquiryResult->class_of_device, class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
				pInquiryResult->clock_offset = clock_offset;
			}
		}
	}
	pHci->pcurrent_inquiry_result = pInquiryResult;
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquiry result num = %d\n", pHci->num_inquiry_result_used);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return pInquiryResult;
}
/**************************************************************************
 *   Hci_Clear_Inquiry_Result_List
 *
 *   Descriptions:
 *      Clear inquiry result list. This function may be called when driver is reset
 *   Arguments:
 *      pHci: IN , pointer to HCI module.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Clear_Inquiry_Result_List(PBT_HCI_T pHci)
{
	KIRQL oldIrql;
	UINT16 i;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	QueueInitList(&pHci->inquiry_result_free_list);
	QueueInitList(&pHci->inquiry_result_used_list);
	for (i = 0; i < BT_MAX_INQUIRY_RESULT_NUM; i++)
	{
		QueuePutTail(&pHci->inquiry_result_free_list, &pHci->inquiry_result_all[i].Link);
	}
	pHci->num_inquiry_result_used = 0;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_Delete_Inquiry_Result_By_BDAddr
 *
 *   Descriptions:
 *      Delete a inquiry result by the BD address. This function may be called
 *      when our device is connected others device.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN,  bd address
 *
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Delete_Inquiry_Result_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PINQUIRY_RESULT_T pInquiryResult;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pInquiryResult = (PINQUIRY_RESULT_T)QueueGetHead(&pHci->inquiry_result_used_list);
	while (pInquiryResult != NULL)
	{
		if (RtlEqualMemory(pInquiryResult->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			QueueRemoveEle(&pHci->inquiry_result_used_list, &pInquiryResult->Link);
			QueuePutTail(&pHci->inquiry_result_free_list, &pInquiryResult->Link);
			pHci->num_inquiry_result_used--;
			break;
		}
		pInquiryResult = (PINQUIRY_RESULT_T)QueueGetNext(&pInquiryResult->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_Find_Inquiry_Result_By_BDAddr
 *
 *   Descriptions:
 *      Find a inquiry result by the BD address.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN,  bd address
 *
 *   Return Value:
 *      pointer to the inquiry result
 *************************************************************************/
PINQUIRY_RESULT_T Hci_Find_Inquiry_Result_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PINQUIRY_RESULT_T pInquiryResult = NULL;
	if (pHci == NULL)
	{
		return (NULL);
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pInquiryResult = (PINQUIRY_RESULT_T)QueueGetHead(&pHci->inquiry_result_used_list);
	while (pInquiryResult != NULL)
	{
		if (RtlEqualMemory(pInquiryResult->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return (pInquiryResult);
		}
		pInquiryResult = (PINQUIRY_RESULT_T)QueueGetNext(&pInquiryResult->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return (NULL);
}
/**********************************************************
 * Hci_Allocate_Am_address
 *
 * Description:
 *    This function allocate an idle am address
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 pOutAddr: OUT,  pointer to the allocated address
 *
 * Return Value:
 *	 STATUS_SUCCESS.   Am address is allocated with successful
 *    STATUS_UNSUCCESSFUL. Am address is allocated with fail
 *
 **********************************************************/
NTSTATUS Hci_Allocate_Am_address(PBT_HCI_T pHci, PUINT8 pOutAddr)
{
	UINT16 i;
	UINT8 j;
	KIRQL				oldIrql;
	PCONNECT_DEVICE_T  pConnectDevice;
	UINT8 div, mod, tmpcount;
	BOOLEAN found = FALSE;

	if (pHci == NULL)
		return STATUS_UNSUCCESSFUL;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	if (pHci->addr_table.am_latest_addr < 1)
		pHci->addr_table.am_latest_addr = 1;

	if (pHci->addr_table.am_latest_addr >= (BT_AM_ADDRESS_SIZE_BY_BIT * 8))
		pHci->addr_table.am_latest_addr = BT_AM_ADDRESS_SIZE_BY_BIT * 8 - 1;

	div = pHci->addr_table.am_latest_addr / 8;
	mod = pHci->addr_table.am_latest_addr % 8;
	tmpcount = 0;

	for (i = div; i < BT_AM_ADDRESS_SIZE_BY_BIT; i++)
	{
		if (i == div)
			tmpcount = mod;
		else
			tmpcount = 0;

		for (j = tmpcount; j < 8; j++)
		{
			if ((pHci->addr_table.am_addr_table[i] & (0x01 << j)) == 0)
			{
				if ((i != 0) || ((i == 0) && (j != 0)))
				{
					found = FALSE;
					if (pHci->num_device_slave > 0)
					{
						pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
						while (pConnectDevice != NULL)
						{
							if (pConnectDevice->am_addr == (UINT8)(i * 8 + j))
							{
								found = TRUE;
								break;
							}

							pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
						}
						
					}
					if (!found)
					{
						pHci->addr_table.am_addr_table[i] |= (0x01 << j);

						*pOutAddr = (UINT8)(i * 8 + j);

						BT_DBGEXT(ZONE_HCI | LEVEL3, "Am address is allocated. value = %d\n", *pOutAddr);

						pHci->addr_table.am_latest_addr = (*pOutAddr == (BT_AM_ADDRESS_SIZE_BY_BIT * 8 - 1)) ? 1 : (*pOutAddr + 1);

						BT_DBGEXT(ZONE_HCI | LEVEL3, "latest am address is: %d\n", pHci->addr_table.am_latest_addr);

						KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

						return STATUS_SUCCESS;
					}

				}
			}
			
		}
	}

	// The first section is end. We should rewind. Scan the second section
	for (i = 0; i < div; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if ((pHci->addr_table.am_addr_table[i] & (0x01 << j)) == 0)
			{
				// The first address can not be allocated
				if ((i != 0) || ((i == 0) && (j != 0)))
				{
					found = FALSE;
					if (pHci->num_device_slave > 0)
					{
						pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
						while (pConnectDevice != NULL)
						{
							// If we found the item which has the same bd addr, we take it from am list and return it to free list
							if (pConnectDevice->am_addr == (UINT8)(i * 8 + j))
							{
								found = TRUE;
								break;
							}

							pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
						}
						
					}
					if (!found)
					{
						// Set this bit as "1" that expresses that this address is already in use and it
						// can not be used until it is released.
						pHci->addr_table.am_addr_table[i] |= (0x01 << j);

						// Output the address
						*pOutAddr = (UINT8)(i * 8 + j);

						BT_DBGEXT(ZONE_HCI | LEVEL3, "Am address is allocated. value = %d\n", *pOutAddr);

						pHci->addr_table.am_latest_addr = (*pOutAddr == (BT_AM_ADDRESS_SIZE_BY_BIT * 8 - 1)) ? 1 : (*pOutAddr + 1);

						BT_DBGEXT(ZONE_HCI | LEVEL3, "latest am address is: %d\n", pHci->addr_table.am_latest_addr);

						KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

						return STATUS_SUCCESS;
					}

				}
			}
			
		}
	}

	// The second section is end. Scan the left bits.
	i = div;
	for (j = 0; j < mod; j++)
	{
		if ((pHci->addr_table.am_addr_table[i] & (0x01 << j)) == 0)
		{
			// The first address can not be allocated
			if ((i != 0) || ((i == 0) && (j != 0)))
			{
				found = FALSE;
				if (pHci->num_device_slave > 0)
				{
					pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
					while (pConnectDevice != NULL)
					{
						// If we found the item which has the same bd addr, we take it from am list and return it to free list
						if (pConnectDevice->am_addr == (UINT8)(i * 8 + j))
						{
							found = TRUE;
							break;
						}

						pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
					}
					
				}
				if (!found)
				{
					// Set this bit as "1" that expresses that this address is already in use and it
					// can not be used until it is released.
					pHci->addr_table.am_addr_table[i] |= (0x01 << j);

					*pOutAddr = (UINT8)(i * 8 + j);

					BT_DBGEXT(ZONE_HCI | LEVEL3, "Am address is allocated. value = %d\n", *pOutAddr);

					pHci->addr_table.am_latest_addr = (*pOutAddr == (BT_AM_ADDRESS_SIZE_BY_BIT * 8 - 1)) ? 1 : (*pOutAddr + 1);

					BT_DBGEXT(ZONE_HCI | LEVEL3, "latest am address is: %d\n", pHci->addr_table.am_latest_addr);

					KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

					return STATUS_SUCCESS;
				}

			}
		}
		
	}


	BT_DBGEXT(ZONE_HCI | LEVEL1, "All am address is used. We allocate nothing.");

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
	return STATUS_UNSUCCESSFUL;
}

/**********************************************************
 * Hci_Free_Am_address
 *
 * Description:
 *    This function releases an am address
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 InAddr: IN,  the am address to be released
 *
 * Return Value:
 *	 STATUS_SUCCESS.   Am address is released with successful
 *    STATUS_UNSUCCESSFUL. Am address is released with fail
 *
 **********************************************************/
NTSTATUS Hci_Free_Am_address(PBT_HCI_T pHci, UINT8 InAddr)
{
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if ((InAddr == 0) || (InAddr > BT_AM_ADDRESS_SIZE - 1))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Free_Am_address()--Input am address is invalid!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	pHci->addr_table.am_addr_table[InAddr / 8] &= ~(0x01 << (InAddr % 8));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Am address is released. value = %d\n", InAddr);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_SUCCESS;
}
/**********************************************************
 * Hci_Allocate_Pm_address
 *
 * Description:
 *    This function allocate an idle pm address
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 pOutAddr: OUT,  pointer to the allocated address
 *
 * Return Value:
 *	 STATUS_SUCCESS.   Pm address is allocated with successful
 *    STATUS_UNSUCCESSFUL. Pm address is allocated with fail
 *
 **********************************************************/
NTSTATUS Hci_Allocate_Pm_address(PBT_HCI_T pHci, PUINT16 pOutAddr)
{
	UINT16 i;
	UINT8 j;
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	for (i = 0; i < BT_PM_ADDRESS_SIZE_BY_BIT; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if ((pHci->addr_table.pm_addr_table[i] &(0x01 << j)) == 0)
			{
				if ((i != 0) || ((i == 0) && (j != 0)))
				{
					pHci->addr_table.pm_addr_table[i] |= (0x01 << j);
					*pOutAddr = (UINT16)(i *8+j);
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Pm address is allocated. value = %d\n",  *pOutAddr);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					return STATUS_SUCCESS;
				}
			}
		}
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "All pm address is used. We allocate nothing.");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**********************************************************
 * Hci_Free_Pm_address
 *
 * Description:
 *    This function releases a pm address
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 InAddr: IN,  the pm address to be released
 *
 * Return Value:
 *	 STATUS_SUCCESS.   pm address is released with successful
 *    STATUS_UNSUCCESSFUL. pm address is released with fail
 *
 **********************************************************/
NTSTATUS Hci_Free_Pm_address(PBT_HCI_T pHci, UINT16 InAddr)
{
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	// Limit the address range
	if ((InAddr == 0) || (InAddr > BT_PM_ADDRESS_SIZE - 1))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Input pm address is invalid!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	// Set this bit as "0" that expresses that this address can now be used again.
	pHci->addr_table.pm_addr_table[InAddr / 8] &= ~(0x01 << (InAddr % 8));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Pm address is released. value = %d\n", InAddr);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_SUCCESS;
}
/**********************************************************
 * Hci_Allocate_Conn_Handle
 *
 * Description:
 *    This function allocate a idle connection handle
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 pOutHandle: OUT,  pointer to the allocated connection handle
 *
 * Return Value:
 *	 STATUS_SUCCESS.   connection handle is allocated with successful
 *    STATUS_UNSUCCESSFUL. connection handle is allocated with fail
 *
 **********************************************************/
NTSTATUS Hci_Allocate_Conn_Handle(PBT_HCI_T pHci, PUINT16 pOutHandle)
{
	UINT16 i;
	UINT8 j;
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	for (i = 0; i < BT_CONNECTION_HANDLE_SIZE_BY_BIT; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if ((pHci->addr_table.conn_handle_table[i] &(0x01 << j)) == 0)
			{
				if ((i != 0) || ((i == 0) && (j != 0)))
				{
					pHci->addr_table.conn_handle_table[i] |= (0x01 << j);
					*pOutHandle = (UINT16)(i *8+j);
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Allocate_Conn_Handle()--Connection handle is allocated. value = %d\n",  *pOutHandle);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					return STATUS_SUCCESS;
				}
			}
		}
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Allocate_Conn_Handle()--All connection handle is used. We allocate nothing.");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**********************************************************
 * Hci_Free_Conn_Handle
 *
 * Description:
 *    This function releases a connection handle
 * Arguments:
 *    pHci: IN, pointer to HCI module.
 *	 InHandle: IN,  the connection handle to be released
 *
 * Return Value:
 *	 STATUS_SUCCESS.   Connection handle is released with successful
 *    STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *
 **********************************************************/
NTSTATUS Hci_Free_Conn_Handle(PBT_HCI_T pHci, UINT16 InHandle)
{
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	// Limit the handle range
	if ((InHandle == 0) || (InHandle > BT_CONNECTION_HANDLE_SIZE - 1))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Free_Conn_Handle()--Input conncetion handle is invalid!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	// Set this bit as "0" that expresses that this handle can now be used again.
	pHci->addr_table.conn_handle_table[InHandle / 8] &= ~(0x01 << (InHandle % 8));
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Free_Conn_Handle()--Connection handle is released. value = %d\n", InHandle);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_SUCCESS;
}
/**************************************************************************
 *   Hci_Add_Connect_Device
 *
 *   Descriptions:
 *      Add a connected device. Make this device active mode (in am list)
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *      packet_type: IN, packet type
 *      repetition_mode: IN,  page scan repetition mode
 *      scan_mode: IN, page scan mode
 *      clock_offset: IN, clock offset
 *      role_switch: IN, allow role switch
 *   Return Value:
 *      STATUS_SUCCESS.   Add connection with successful
 *      STATUS_UNSUCCESSFUL. Add connection with fail
 *************************************************************************/
NTSTATUS Hci_Add_Connect_Device(PBT_DEVICE_EXT devExt, PUINT8 bd_addr, UINT16 packet_type, UINT8 repetition_mode, UINT8 scan_mode, UINT16 clock_offset, UINT8 role_switch)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 am_addr;
	UINT16 conn_handle;
	NTSTATUS status;
	UINT8 value[4];
	UINT8 tmpVar;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	// Remove a idle connect-device from the head of connect-device free list.
	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_free_list);
	if (pConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Add_Connect_Device()--There is no any more connect-device resources!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	status = Hci_Allocate_Am_address(pHci, &am_addr);
	if (status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Add_Connect_Device()--Allocate am address fail!\n");
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		// Return the connect-device resource
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	status = Hci_Allocate_Conn_Handle(pHci, &conn_handle);
	if (status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Add_Connect_Device()--Allocate connection handle fail!\n");
		Hci_Free_Am_address(pHci, am_addr);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}


	
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Add_Connect_Device()--Add connect handle,  pConnectDevice = 0x%x \n", pConnectDevice);
	pConnectDevice->am_addr = am_addr;
	pConnectDevice->connection_handle = conn_handle;
	RtlCopyMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH);
	RtlZeroMemory(pConnectDevice->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	pConnectDevice->packet_type = packet_type;
	pConnectDevice->page_scan_repetition_mode = repetition_mode;
	pConnectDevice->page_scan_mode = scan_mode;
	pConnectDevice->clock_offset = clock_offset;
	pConnectDevice->allow_role_switch = role_switch;
	pConnectDevice->link_type = BT_LINK_TYPE_ACL;
	pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
	pConnectDevice->current_role = BT_ROLE_MASTER;
	pConnectDevice->raw_role = BT_ROLE_MASTER;
	pConnectDevice->link_supervision_timeout = 0x7d00; // default value (20 seconds)
	pConnectDevice->real_link_supervision_timeout = 0x7d00;  // default value (20 seconds)
	pConnectDevice->link_policy_settings = 0x7; // enable role switch, enable hold, enable sniff, disable park
	pConnectDevice->per_poll_interval = 0x28; // default value (25 milliseconds)
	pConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pConnectDevice->timer_valid = 0;
	pConnectDevice->timer_counter = 0;
	pConnectDevice->tempflag = 0;
	pConnectDevice->is_in_encryption_process = 0;
	pConnectDevice->pScoConnectDevice = NULL;
	if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH5)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DM5)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM5;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH3)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH3;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_3_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DM3)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_3_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH1)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH1;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_1_SLOT;
	}
	else
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_1_SLOT;
	}

	pConnectDevice->rx_sco_con_null_count = 0;
	pConnectDevice->rssi = 0;
	pConnectDevice->flush_flag = 0;
	pConnectDevice->flush_timeout = 0;
	pConnectDevice->mode_current_mode = 0;
	pConnectDevice->hold_mode_max_interval = 0;
	pConnectDevice->hold_mode_min_interval = 0;
	pConnectDevice->hold_mode_interval = 0;
	pConnectDevice->sniff_max_interval = 0;
	pConnectDevice->sniff_min_interval = 0;
	pConnectDevice->sniff_attempt = 0;
	pConnectDevice->sniff_timeout = 0;
	pConnectDevice->sniff_mode_interval = 0;
	pConnectDevice->afh_mode = 1;
	pConnectDevice->max_slot = BT_MAX_SLOT_BY_FEATURE;
	pConnectDevice->role_switching_flag = 0;
	pConnectDevice->role_switching_role = BT_ROLE_MASTER;
	pConnectDevice->tx_count_frame = 0;
	pConnectDevice->tx_power = TX_POWER_DEFAULT;
	pConnectDevice->l2cap_flow_control_flag = 0;
	pConnectDevice->timer_l2cap_flow_valid = 0;
	pConnectDevice->timer_l2cap_flow_counter = 0;
	pConnectDevice->l2cap_rx_flow_control_flag = 0;
	pConnectDevice->is_in_disconnecting = 0;
	pConnectDevice->valid_flag = 1;
	pConnectDevice->send_classification_flag = 0;
	pConnectDevice->classification_timer_count = 0;
	pConnectDevice->classification_timer_valid = 1;
	pConnectDevice->classification_interval = BT_MAX_AFH_TIMER_COUNT;
	pConnectDevice->is_afh_sent_flag = 0;
	pConnectDevice->is_in_remote_name_req_flag = 0;
	pConnectDevice->slave_index = 0;


	pConnectDevice->rev_auto_rate = 0;

	pConnectDevice->local_slot_offset = 1;
	pConnectDevice->edr_mode = 0;
	pConnectDevice->edr_change_flag = 0;

#ifdef BT_ALLOCATE_FRAG_FOR_EACH_CONNECTION
	pConnectDevice->get_packet_num_from_hc = 0;
	pConnectDevice->not_complet_packet_num = 0;
#endif


	#ifdef  BT_INTERNAL_QOS_SETUP
	  pConnectDevice->internal_qos_flag=0;
	#endif

	pConnectDevice->current_page_number = 0xFF;
	pConnectDevice->max_page_number = 0;

	pConnectDevice->connection_timer_valid = 0;
	pConnectDevice->connection_timer_count = 0;
	pConnectDevice->connection_state = 0;	//not connected

	pConnectDevice->MsKb_PacketCount = 0;

	pConnectDevice->Sniff_RetryCount = 0;
	pConnectDevice->mode_Sniff_debug1 = 0;

	pConnectDevice->pause_encryption_status = 0;
	pConnectDevice->pause_command_flag = 0;

	QueuePutTail(&pHci->device_am_list, &pConnectDevice->Link);
	pHci->num_device_am++;
	pHci->current_connection_handle = conn_handle;
	pHci->pcurrent_connect_device = pConnectDevice;
	pHci->role = BT_ROLE_MASTER;
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Add_Connect_Device()--Add connect handle ok, number of am list = %d \n", pHci->num_device_am);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_SUCCESS;
}
/**************************************************************************
 *   Hci_Del_Connect_Device_By_BDAddr
 *
 *   Descriptions:
 *      Delete a connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (RtlEqualMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Connect_Device_By_BDAddr()--Delete connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			// Remove the connect device block from the used am list.
			QueueRemoveEle(&pHci->device_am_list, &pConnectDevice->Link);
			ASSERT(0);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			// Release connect handle
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			Hci_Free_Am_address(pHci, pConnectDevice->am_addr);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// Insert the connect device block into the tail of free list.
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_am--;
			if (pHci->num_device_am == 0)
			{
				pHci->role = BT_ROLE_SLAVE;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Connect_Device_By_BDAddr()--Delete connect device by bd address is ok, number of am list = %d \n", pHci->num_device_am);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL0, "Delete connect handle by bd address is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Del_Connect_Device_By_AMAddr
 *
 *   Descriptions:
 *      Delete a connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->am_addr == am_addr)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Connect_Device_By_AMAddr()--Delete connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			// Remove the connect device block from the used am list.
			QueueRemoveEle(&pHci->device_am_list, &pConnectDevice->Link);
			//ASSERT(0);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			Hci_Free_Am_address(pHci, pConnectDevice->am_addr);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// Insert the connect device block into the tail of free list.
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_am--;
			if (pHci->num_device_am == 0)
			{
				pHci->role = BT_ROLE_SLAVE;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Connect_Device_By_AMAddr()--Delete connect device by am address is ok, number of am list = %d \n", pHci->num_device_am);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Del_Connect_Device_By_AMAddr()--Delete connect handle by am address is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Del_Connect_Device_By_ConnHandle
 *
 *   Descriptions:
 *      Delete a connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->connection_handle == conn_handle)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			// Remove the connect device block from the used am list.
			QueueRemoveEle(&pHci->device_am_list, &pConnectDevice->Link);
			ASSERT(0);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			Hci_Free_Am_address(pHci, pConnectDevice->am_addr);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// Insert the connect device block into the tail of free list.
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_am--;
			if (pHci->num_device_am == 0)
			{
				pHci->role = BT_ROLE_SLAVE;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete connect device by connection handle address is ok, number of am list = %d \n", pHci->num_device_am);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL0, "Delete connect handle by connection handle is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Find_Connect_Device_By_BDAddr
 *
 *   Descriptions:
 *      Find a connected device from am list by bd address.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (RtlEqualMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Connect_Device_By_BDAddr()--Find connect handle,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Connect_Device_By_BDAddr()--Can not find any connect device by this bd address.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Connect_Device_By_AMAddr
 *
 *   Descriptions:
 *      Find a connected device from am list by am address.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *   Return Value:
 *		Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->am_addr == am_addr)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Connect_Device_By_AMAddr()--Find connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Connect_Device_By_AMAddr()--Can't find any device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Connect_Device_By_ConnHandle
 *
 *   Descriptions:
 *      Find a connected device from am list by connection handle.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->connection_handle == conn_handle)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Connect_Device_By_ConnHandle()--Find connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Find_Connect_Device_By_ConnHandle()--Can't find any device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Add_Slave_Connect_Device
 *
 *   Descriptions:
 *      Add a slave connected device.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      am_addr: IN, am address
 *		bd_addr: IN, bd address
 *		class_of_device: IN, class of device
 *      packet_type: IN, packet type
 *      repetition_mode: IN,  page scan repetition mode
 *      scan_mode: IN, page scan mode
 *      clock_offset: IN, clock offset
 *      role_switch: IN, allow role switch
 *   Return Value:
 *      STATUS_SUCCESS.   Add connection with successful
 *      STATUS_UNSUCCESSFUL. Add connection with fail
 *************************************************************************/
 //Jakio20071025: change here for EDR
	NTSTATUS Hci_Add_Slave_Connect_Device(PBT_HCI_T pHci, UINT8 am_addr, PUINT8 bd_addr, PUINT8 class_of_device, UINT16 packet_type, UINT8 repetition_mode, UINT8 scan_mode, UINT16 clock_offset, UINT8 role_switch, UINT8 slave_index)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT16 conn_handle;
	NTSTATUS status;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_free_list);
	if (pConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Add_Slave_Connect_Device()--There is no any more connect-device resources!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	status = Hci_Allocate_Conn_Handle(pHci, &conn_handle);
	if (status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Hci_Add_Slave_Connect_Device()--Allocate connection handle fail!\n");
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Add_Slave_Connect_Device()--Add device connect handle,  pConnectDevice = 0x%x \n", pConnectDevice);
	pConnectDevice->am_addr = am_addr;
	pConnectDevice->connection_handle = conn_handle;
	RtlCopyMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH);
	RtlCopyMemory(pConnectDevice->class_of_device, class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	pConnectDevice->packet_type = packet_type;
	pConnectDevice->page_scan_repetition_mode = repetition_mode;
	pConnectDevice->page_scan_mode = scan_mode;
	pConnectDevice->clock_offset = clock_offset;
	pConnectDevice->allow_role_switch = role_switch;
	pConnectDevice->link_type = BT_LINK_TYPE_ACL;
	pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
	pConnectDevice->current_role = BT_ROLE_SLAVE;
	pConnectDevice->raw_role = BT_ROLE_SLAVE;
	pConnectDevice->link_supervision_timeout = 0x7d00; // default value (20 seconds)
	pConnectDevice->real_link_supervision_timeout = 0x7d00;  // default value (20 seconds)
	pConnectDevice->link_policy_settings = 0x7; // enable role switch, enable hold, enable sniff, disable park
	pConnectDevice->per_poll_interval = 0x28; // default value (25 milliseconds)
	pConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pConnectDevice->timer_valid = 0;
	pConnectDevice->timer_counter = 0;
	pConnectDevice->tempflag = 0;
	pConnectDevice->is_in_encryption_process = 0;
	pConnectDevice->pScoConnectDevice = NULL;
	if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH5)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DM5)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM5;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_5_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH3)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH3;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_3_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DM3)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM3;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_3_SLOT;
	}
	else if (pConnectDevice->packet_type & BT_PACKET_TYPE_DH1)
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DH1;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_1_SLOT;
	}
	else
	{
	  pConnectDevice->current_packet_type = BT_ACL_PACKET_DM1;
	  pConnectDevice->tx_max_slot = BT_MAX_SLOT_1_SLOT;
	}

	pConnectDevice->rx_sco_con_null_count = 0;
	pConnectDevice->rssi = 0;
	pConnectDevice->flush_flag = 0;
	pConnectDevice->flush_timeout = 0;
	pConnectDevice->mode_current_mode = 0;
	pConnectDevice->hold_mode_max_interval = 0;
	pConnectDevice->hold_mode_min_interval = 0;
	pConnectDevice->hold_mode_interval = 0;
	pConnectDevice->sniff_max_interval = 0;
	pConnectDevice->sniff_min_interval = 0;
	pConnectDevice->sniff_attempt = 0;
	pConnectDevice->sniff_timeout = 0;
	pConnectDevice->sniff_mode_interval = 0;
	pConnectDevice->afh_mode = 1;
	pConnectDevice->max_slot = BT_MAX_SLOT_BY_FEATURE;
	pConnectDevice->role_switching_flag = 0;
	pConnectDevice->role_switching_role = BT_ROLE_SLAVE;
	pConnectDevice->tx_count_frame = 0;
	pConnectDevice->tx_power = TX_POWER_DEFAULT;
	pConnectDevice->l2cap_flow_control_flag = 0;
	pConnectDevice->timer_l2cap_flow_valid = 0;
	pConnectDevice->timer_l2cap_flow_counter = 0;
	pConnectDevice->l2cap_rx_flow_control_flag = 0;
	pConnectDevice->is_in_disconnecting = 0;
	pConnectDevice->valid_flag = 1;
	pConnectDevice->send_classification_flag = 0;
	pConnectDevice->classification_timer_count = 0;
	pConnectDevice->classification_timer_valid = 1;
	pConnectDevice->classification_interval = BT_MAX_AFH_TIMER_COUNT;
	pConnectDevice->is_afh_sent_flag = 0;
	pConnectDevice->is_in_remote_name_req_flag = 0;
	pConnectDevice->slave_index = slave_index;

	pConnectDevice->rev_auto_rate = 0;

	pConnectDevice->local_slot_offset = 1;
	pConnectDevice->edr_mode = 0;
	pConnectDevice->edr_change_flag = 0;
	
#ifdef BT_ALLOCATE_FRAG_FOR_EACH_CONNECTION
	pConnectDevice->get_packet_num_from_hc = 0;
	pConnectDevice->not_complet_packet_num = 0;
#endif
	
	#ifdef  BT_INTERNAL_QOS_SETUP
	  pConnectDevice->internal_qos_flag=0;
	#endif

	pConnectDevice->current_page_number = 0xFF;
	pConnectDevice->max_page_number = 0;

	pConnectDevice->pause_encryption_status = 0;
	pConnectDevice->pause_command_flag = 0;

	pConnectDevice->connection_timer_valid = 1;
	pConnectDevice->connection_timer_count = 5;
	pConnectDevice->connection_state = 0;	//not connected

	pConnectDevice->MsKb_PacketCount = 0;
	pConnectDevice->Sniff_RetryCount = 0;
	pConnectDevice->mode_Sniff_debug1 = 0;

	QueuePutTail(&pHci->device_slave_list, &pConnectDevice->Link);
	pHci->num_device_slave++;
	pHci->current_connection_handle = conn_handle;
	pHci->role = BT_ROLE_SLAVE;
	pHci->pcurrent_connect_device = pConnectDevice;
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Add_Slave_Connect_Device()--Add slave connect handle ok, number of am list = %d \n", pHci->num_device_slave);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *   Hci_Del_Slave_Connect_Device_By_BDAddr
 *
 *   Descriptions:
 *      Delete a slave connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Slave_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (RtlEqualMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			QueueRemoveEle(&pHci->device_slave_list, &pConnectDevice->Link);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_slave--;
			if (pHci->num_device_slave == 0)
			{
				pHci->role = BT_ROLE_MASTER;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete slave connect device by bd address is ok, number of am list = %d \n", pHci->num_device_slave);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL0, "Delete slave connect handle by bd address is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Del_Slave_Connect_Device_By_AMAddr
 *
 *   Descriptions:
 *      Delete a slave connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Slave_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->am_addr == am_addr)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Slave_Connect_Device_By_AMAddr()--Delete slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			// Remove the connect device block from the used am list.
			QueueRemoveEle(&pHci->device_slave_list, &pConnectDevice->Link);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			// Release connect handle
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// Insert the connect device block into the tail of free list.
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_slave--;
			if (pHci->num_device_slave == 0)
			{
				pHci->role = BT_ROLE_MASTER;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Slave_Connect_Device_By_AMAddr()--Delete slave connect device by am address is ok, number of am list = %d \n", pHci->num_device_slave);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Del_Slave_Connect_Device_By_AMAddr()--Delete slave connect handle by am address is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Del_Slave_Connect_Device_By_ConnHandle
 *
 *   Descriptions:
 *      Delete a slave connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail
 *************************************************************************/
NTSTATUS Hci_Del_Slave_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->connection_handle == conn_handle)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			// Remove the connect device block from the used am list.
			QueueRemoveEle(&pHci->device_slave_list, &pConnectDevice->Link);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			// Insert the connect device block into the tail of free list.
			QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
			pHci->num_device_slave--;
			if (pHci->num_device_slave == 0)
			{
				pHci->role = BT_ROLE_MASTER;
			}
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete slave connect device by connection handle address is ok, number of am list = %d \n", pHci->num_device_slave);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return STATUS_SUCCESS;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL0, "Delete slave connect handle by connection handle is fail.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index
 *
 *   Descriptions:
 *      Delete a slave connected device from am list and return it to the free list.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *   Return Value:
 *      STATUS_SUCCESS.   Connection handle is released with successful
 *      STATUS_UNSUCCESSFUL. Connection handle is released with fail 
 *************************************************************************/
NTSTATUS Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(PBT_HCI_T pHci, UINT8 am_addr, UINT8 slave_index)
{
	KIRQL				oldIrql;
	PCONNECT_DEVICE_T  pConnectDevice;

	if (pHci == NULL)
		return STATUS_UNSUCCESSFUL;

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);

	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if ((pConnectDevice->am_addr == am_addr) && (pConnectDevice->slave_index == slave_index))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete slave connect device,  pConnectDevice = 0x%x \n",pConnectDevice);

			// Remove the connect device block from the used am list. 
			QueueRemoveEle(&pHci->device_slave_list,&pConnectDevice->Link);

			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

			// Release connect handle
			Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);

			KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		
			// Insert the connect device block into the tail of free list. 
			QueuePutTail(&pHci->device_free_list,&pConnectDevice->Link);

			pHci->num_device_slave--;
			if (pHci->num_device_slave == 0)
				pHci->role = BT_ROLE_MASTER;

			BT_DBGEXT(ZONE_HCI | LEVEL3, "Delete slave connect device by am address is ok, number of am list = %d \n",pHci->num_device_slave);

			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

			return STATUS_SUCCESS;
		}

		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}

	BT_DBGEXT(ZONE_HCI | LEVEL0, "Delete slave connect handle by am address is fail.\n");

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	return STATUS_UNSUCCESSFUL;
}
/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_BDAddr
 *
 *   Descriptions:
 *      Find a slave connected device from am list by bd address.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		bd_addr: IN, bd address
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (RtlEqualMemory(pConnectDevice->bd_addr, bd_addr, BT_BD_ADDR_LENGTH))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Slave_Connect_Device_By_BDAddr()--Find slave connect handle,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Find_Slave_Connect_Device_By_BDAddr()--Can not find any slave connect device by this bd address.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_AMAddr
 *
 *   Descriptions:
 *      Find a slave connected device from am list by am address.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *   Return Value:
 *		Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->am_addr == am_addr)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Slave_Connect_Device_By_AMAddr()--Find slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Find_Slave_Connect_Device_By_AMAddr()--Can't find any slave device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_ConnHandle
 *
 *   Descriptions:
 *      Find a slave connected device from am list by connection handle.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->connection_handle == conn_handle)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Find_Slave_Connect_Device_By_ConnHandle()--Find slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Find_Slave_Connect_Device_By_ConnHandle()--Can't find any slave device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index
 *
 *   Descriptions:
 *      Find a slave connected device from am list by am address and slave index.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		am_addr: IN, am address
 *      slave_index: IN, slave index
 *   Return Value:
 *		Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(PBT_HCI_T pHci, UINT8 am_addr, UINT8 slave_index)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if ((pConnectDevice->am_addr == am_addr) && (pConnectDevice->slave_index == slave_index))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Find slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Can't find any slave device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}

/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_Index
 *
 *   Descriptions:
 *      Find a slave connected device from slave list by slave index.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      slave_index: IN, slave index
 *   Return Value:
 *		Connet device block 
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_Index(PBT_HCI_T pHci, UINT8 slave_index)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	
	if (pHci == NULL)
		return NULL;
	
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->slave_index == slave_index)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Find slave connect device, pConnectDevice = 0x%x \n", pConnectDevice);
			
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Can't find any slave device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Add_Sco_Connect_Device
 *
 *   Descriptions:
 *      Add a sco connected device.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      packet_type: IN, packet type.
 *		pConnectDevice: IN, pointer to the connect device.
 *   Return Value:
 *      NULL.   Add connection with successful
 *      Not NULL. the pointer to a sco connect device.
 *************************************************************************/
PVOID Hci_Add_Sco_Connect_Device(PBT_HCI_T pHci, UINT16 packet_type, PVOID pConnectDevice)
{
	KIRQL oldIrql;
	UINT16 conn_handle;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	NTSTATUS status;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)QueuePopHead(&pHci->sco_device_free_list);
	if (pScoConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "There is no any more sco connect device resources!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return NULL;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	status = Hci_Allocate_Conn_Handle(pHci, &conn_handle);
	if (status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Allocate connection handle fail!\n");
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		QueuePutTail(&pHci->sco_device_free_list, &pScoConnectDevice->Link);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Add connect handle,  pScoConnectDevice = 0x%x, connection handle = %d \n", pScoConnectDevice, conn_handle);
	pScoConnectDevice->connection_handle = conn_handle;
	pScoConnectDevice->link_type = BT_LINK_TYPE_SCO;
	pScoConnectDevice->packet_type = packet_type;
	pScoConnectDevice->pConnectDevice = pConnectDevice;
	pScoConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pScoConnectDevice->timer_valid = 0;
	pScoConnectDevice->timer_counter = 0;
	pScoConnectDevice->current_packet_type = BT_SCO_PACKET_HV3;
	pScoConnectDevice->state = BT_DEVICE_STATE_IDLE;
	pScoConnectDevice->is_in_disconnecting = 0;
	pScoConnectDevice->using_esco_command_flag = 0;
	if (pScoConnectDevice->packet_type &BT_PACKET_TYPE_HV1)
	{
		pScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV1;
	}
	if (pScoConnectDevice->packet_type &BT_PACKET_TYPE_HV2)
	{
		pScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV2;
	}
	if (pScoConnectDevice->packet_type &BT_PACKET_TYPE_HV3)
	{
		pScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV3;
	}
	pScoConnectDevice->esco_amaddr = 0;
	pScoConnectDevice->current_esco_rx_packet_type = BT_ESCO_PACKET_EV3;
	pScoConnectDevice->wesco = 0;
    // Reset the 240 Byte buffer for SCO
    pScoConnectDevice->lenTx240 = 0;
    pScoConnectDevice->pTx240Buf = NULL;
    pScoConnectDevice->txRxBalance = 0;
    
	pScoConnectDevice->NeedSendScoNull = TRUE;
	
	pHci->sco_link_count++;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return ((PVOID)pScoConnectDevice);
}
/**************************************************************************
 *   Hci_Add_eSco_Connect_Device
 *
 *   Descriptions:
 *      Add a sco connected device.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      packet_type: IN, packet type.
 *		pConnectDevice: IN, pointer to the connect device.
 *   Return Value:
 *      NULL.   Add connection with successful
 *      Not NULL. the pointer to a sco connect device.
 *************************************************************************/
PVOID Hci_Add_eSco_Connect_Device(PBT_HCI_T pHci, UINT32 transmit_bandwidth, UINT32 receive_bandwidth, UINT16 max_latency, UINT16 voice_setting, UINT8 retransmission_effort, UINT16 packet_type, PVOID pConnectDevice)
{
	KIRQL oldIrql;
	UINT16 conn_handle;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	NTSTATUS status;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	// Remove a idle sco connect device from the head of sco connect device free list.
	pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)QueuePopHead(&pHci->sco_device_free_list);
	if (pScoConnectDevice == NULL)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "There is no any more sco connect device resources!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return NULL;
	}
	// It will acquire the spin lock in the below function, so we must release it first.
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	status = Hci_Allocate_Conn_Handle(pHci, &conn_handle);
	if (status != STATUS_SUCCESS)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Allocate connection handle fail!\n");
		//		Hci_Free_Am_address(pHci, am_addr);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		// Return the connect-device resource
		QueuePutTail(&pHci->sco_device_free_list, &pScoConnectDevice->Link);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Add esco connect handle,  pScoConnectDevice = 0x%x, connection handle = %d \n", pScoConnectDevice, conn_handle);
	pScoConnectDevice->connection_handle = conn_handle;
	pScoConnectDevice->link_type = BT_LINK_TYPE_ESCO;
	pScoConnectDevice->pConnectDevice = pConnectDevice;
	pScoConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pScoConnectDevice->timer_valid = 0;
	pScoConnectDevice->timer_counter = 0;
	pScoConnectDevice->current_packet_type = BT_SCO_PACKET_HV3;
	pScoConnectDevice->state = BT_DEVICE_STATE_IDLE;
	pScoConnectDevice->is_in_disconnecting = 0;
	pScoConnectDevice->using_esco_command_flag = 1;
	pScoConnectDevice->transmit_bandwidth = transmit_bandwidth;
	pScoConnectDevice->receive_bandwidth = receive_bandwidth;
	pScoConnectDevice->max_latency = max_latency;
	pScoConnectDevice->voice_setting = voice_setting;
	pScoConnectDevice->retransmission_effort = retransmission_effort;
	pScoConnectDevice->esco_packet_type = packet_type;
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV1)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV1;
	}
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV2)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV2;
	}
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV3)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV3;
	}
	pScoConnectDevice->esco_amaddr = 0;
	pScoConnectDevice->current_esco_rx_packet_type = BT_ESCO_PACKET_EV3;
	pScoConnectDevice->wesco = 0;

	pScoConnectDevice->NeedSendScoNull = TRUE;
	
	pHci->sco_link_count++;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return ((PVOID)pScoConnectDevice);
}
/**************************************************************************
 *   Hci_Modify_eSco_Connect_Device
 *
 *   Descriptions:
 *      Modify an already existed esco connection.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      packet_type: IN, packet type.
 *		pConnectDevice: IN, pointer to the connect device.
 *   Return Value:
 *      NULL.   Add connection with successful
 *      Not NULL. the pointer to a sco connect device.
 *************************************************************************/
VOID Hci_Modify_eSco_Connect_Device(PBT_HCI_T pHci, UINT32 transmit_bandwidth, UINT32 receive_bandwidth, UINT16 max_latency, UINT16 voice_setting, UINT8 retransmission_effort, UINT16 packet_type, PVOID pTempScoConnectDevice)
{
	KIRQL oldIrql;
	UINT16 conn_handle;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	NTSTATUS status;
	if ((pHci == NULL) || (pTempScoConnectDevice == NULL))
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempScoConnectDevice;
	pScoConnectDevice->transmit_bandwidth = transmit_bandwidth;
	pScoConnectDevice->receive_bandwidth = receive_bandwidth;
	pScoConnectDevice->max_latency = max_latency;
	pScoConnectDevice->voice_setting = voice_setting;
	pScoConnectDevice->retransmission_effort = retransmission_effort;
	pScoConnectDevice->esco_packet_type = packet_type;
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV1)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV1;
	}
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV2)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV2;
	}
	if (pScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV3)
	{
		pScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV3;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return ;
}
/**************************************************************************
 *   Hci_Del_Sco_Connect_Device
 *
 *   Descriptions:
 *      Delete a sco connected device.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		pConnectDevice: IN, pointer to the connect device.
 *      pScoConnectDevice: IN, pointer to the sco connect device.
 *   Return Value:
 *      NULL
 *************************************************************************/
VOID Hci_Del_Sco_Connect_Device(PBT_HCI_T pHci, PVOID pConnectDevice, PVOID pScoConnectDevice)
{
	KIRQL oldIrql;
	if (pHci == NULL)
	{
		return ;
	}
	if ((pConnectDevice == NULL) || (pScoConnectDevice == NULL))
	{
		return ;
	}
	ASSERT(pConnectDevice == ((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->pConnectDevice);
	Hci_Free_Conn_Handle(pHci, ((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->connection_handle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->packet_type = 0;
	((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->esco_packet_type = 0;
	((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->pConnectDevice = NULL;
	QueuePutTail(&pHci->sco_device_free_list, &((PSCO_CONNECT_DEVICE_T)pScoConnectDevice)->Link);
	((PCONNECT_DEVICE_T)pConnectDevice)->pScoConnectDevice = NULL;
	if (pHci->sco_link_count > 0)
	{
		pHci->sco_link_count--;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle
 *
 *   Descriptions:
 *      Find a connected device from am list by connection handle and sco connection handle.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *      plink_type: OUT, the pointer to link_type.
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle, PUINT8 plink_type)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->connection_handle == conn_handle)
		{
			*plink_type = BT_LINK_TYPE_ACL;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		if (pConnectDevice->pScoConnectDevice != NULL)
		{
			if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->connection_handle == conn_handle)
			{
				//BT_DBGEXT(ZONE_HCI | LEVEL3, "Find sco connect device,  pConnectDevice = 0x%x, pScoConnectDevice = 0x%x\n", pConnectDevice, pConnectDevice->pScoConnectDevice);
				*plink_type = BT_LINK_TYPE_SCO;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				return pConnectDevice;
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Can't find any device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle
 *
 *   Descriptions:
 *      Find a slave connected device from am list by connection handle and sco connection handle.
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *		conn_handle: IN, connection handle.
 *      plink_type: OUT, the pointer to link_type.
 *   Return Value:
 *      Connet device block
 *************************************************************************/
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle, PUINT8 plink_type)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	if (pHci == NULL)
	{
		return NULL;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->connection_handle == conn_handle)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Find slave connect device,  pConnectDevice = 0x%x \n", pConnectDevice);
			*plink_type = BT_LINK_TYPE_ACL;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			return pConnectDevice;
		}
		if (pConnectDevice->pScoConnectDevice != NULL)
		{
			if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->connection_handle == conn_handle)
			{
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Find slave sco connect device,  pConnectDevice = 0x%x, pScoConnectDevice = 0x%x\n", pConnectDevice, pConnectDevice->pScoConnectDevice);
				*plink_type = BT_LINK_TYPE_SCO;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				return pConnectDevice;
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL1, "Can't find any slave device connect.\n");
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return NULL;
}
/**************************************************************************
 *   Hci_StartTimer
 *
 *   Descriptions:
 *      Start a timer for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartTimer(PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_type, UINT16 timer_count)
{
	if (pConnectDevice == NULL)
	{
		return ;
	}
	pConnectDevice->timer_valid = 1;
	pConnectDevice->timer_type = timer_type;
	pConnectDevice->timer_counter = timer_count;
}
/**************************************************************************
 *   Hci_StopTimer
 *
 *   Descriptions:
 *      Start a timer for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopTimer(PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "HCI Stop timer\n");
	pConnectDevice->timer_valid = 0;
	pConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pConnectDevice->timer_counter = 0;
}
/**************************************************************************
 *   Hci_Timeout
 *
 *   Descriptions:
 *      Time out function for HCI module
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *   Return Value:
 *      None
 *Changelog: jakio20070808
 *	Add a local var 'Timer_flag' to identify whether we need
 *	set a timer to notify uplayer to read event. The reason for
 *	doing this dues to the fact: we decreased the IRQL of this
 *	routine. so, if we set timer too early in routine whose name
 *	like "Task_**_send_**_event()", the flow will be disordered.
 *	To prevent this case happening, I decide to set the timer at
 *	the end of the routine when needed.
 *************************************************************************/
VOID Hci_Timeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 tmpamaddr[8];
	UINT8 tmpslaveamaddr[3];
	UINT8 tmpslaveindex[3];
	UINT8 i;
	UINT8 tmplen = 0;
	UINT8 tmpslavelen = 0;
	UINT8 slaveindex = 0;
	UINT8 Datastr[12];
	LARGE_INTEGER timevalue;
	UINT8 Timer_flag = 0;

	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	//BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Timeout() entered\n");
	// Scan master list
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->timer_valid == 1)
		{
			if (--pConnectDevice->timer_counter == 0)
			{
				if (pConnectDevice->timer_type == BT_TIMER_TYPE_PAGE_TIMEOUT)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Page timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					// Page timeout for remote name request
					if (pConnectDevice->tempflag == 1)
					{
						Hci_StopTimer(pConnectDevice);
						// Write disconnect register to hardware
						// BT_WRITE_COMMAND_INDICATOR(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						// May be delay several milliseconds
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						pConnectDevice->status = BT_HCI_ERROR_PAGE_TIMEOUT;
						// Stop all timer
						Hci_StopAllTimer(pConnectDevice);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_InitLMPMembers(devExt, pConnectDevice);
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
						RtlZeroMemory(pConnectDevice->remote_name, BT_LOCAL_NAME_LENGTH);
						Hci_InitTxForConnDev(devExt, pConnectDevice);
						Hci_ReleaseScoLink(devExt, pConnectDevice);
						Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						Timer_flag = 1;
						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
						//Hci_Del_Connect_Device_By_AMAddr(pHci,pConnectDevice->am_addr);
						//timevalue.QuadPart = 0;
						//tasklet_schedule(&devExt->TxSendingTimer);	
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					else
					{
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						pConnectDevice->status = BT_HCI_ERROR_PAGE_TIMEOUT;
						Hci_StopAllTimer(pConnectDevice);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_InitLMPMembers(devExt, pConnectDevice);
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
						Hci_InitTxForConnDev(devExt, pConnectDevice);
						Hci_ReleaseScoLink(devExt, pConnectDevice);
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						Timer_flag = 1;
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Link supervision timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					Hci_StopTimer(pConnectDevice);
					if (pConnectDevice->state == BT_DEVICE_6TPOLL_TIMEOUT)
					{
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
						Hci_StopAllTimer(pConnectDevice);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_InitLMPMembers(devExt, pConnectDevice);
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
						Hci_InitTxForConnDev(devExt, pConnectDevice);
						Hci_ReleaseScoLink(devExt, pConnectDevice);
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					else if (pConnectDevice->state == BT_DEVICE_6TPOLL_TIMEOUT_FOR_DISCONN)
					{
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
						Hci_StopAllTimer(pConnectDevice);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_InitLMPMembers(devExt, pConnectDevice);
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
						#endif
						Hci_InitTxForConnDev(devExt, pConnectDevice);
						Hci_ReleaseScoLink(devExt, pConnectDevice);
						Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					else
					{
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;
						pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;
						Hci_StopAllTimer(pConnectDevice);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_InitLMPMembers(devExt, pConnectDevice);
						#ifdef ACCESS_REGISTER_DIRECTLY
							Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
							Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
							#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
								Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
							#else
								BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
							#endif
						#else
							BtUsbHciTimeout(devExt, pConnectDevice->am_addr);
						#endif
						Hci_InitTxForConnDev(devExt, pConnectDevice);
						Hci_ReleaseScoLink(devExt, pConnectDevice);
						Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					}
					Timer_flag = 1;
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_CONN_ACCEPT)
				{
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_MONITER_CONN_COMP)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Monitor connection complete timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					Hci_StopTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					#ifdef ACCESS_REGISTER_DIRECTLY
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
						#endif
					#else
						BtUsbHciTimeout(devExt, pConnectDevice->am_addr);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_MONITER_LINK_SUPERVISION)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL1, "Disconnect maseter because link supersion timer is not set after link is connected!\n");
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					#ifdef ACCESS_REGISTER_DIRECTLY
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
						#endif
					#else
						BtUsbHciTimeout(devExt, pConnectDevice->am_addr);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else
				{
				}
			}
		}
		if (pConnectDevice->pScoConnectDevice != NULL)
		{
			if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_valid == 1)
			{
				if (--((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_counter == 0)
				{
					if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_type == BT_TIMER_TYPE_MONITER_CONN_COMP)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL1, "Monitor connection complete timeout, pScoConnectDevice = 0x%x\n", pConnectDevice->pScoConnectDevice);
						Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice));
						((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_HOST_TIMEOUT;
						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Frag_InitTxScoForConnDev(devExt, pConnectDevice);
						Task_HCI2HC_Send_Sco_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						Timer_flag = 1;
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						Frag_InitTxScoForAllDev(devExt);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						if (pHci->sco_link_count == 0)
						{
							pHci->auto_packet_select_flag = 1;
						}
						{
							LARGE_INTEGER timevalue;
							Timer_flag = 1;
						
						}
					}
				}
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->timer_valid == 1)
		{
			if (--pConnectDevice->timer_counter == 0)
			{
				slaveindex = pConnectDevice->slave_index;
				if (pConnectDevice->timer_type == BT_TIMER_TYPE_PAGE_TIMEOUT)
				{
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_LINK_SUPERVISION)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Link supervision timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					Hci_StopTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					Hci_Clear_AM_Addr(devExt, 0);
					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);

					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
				
					
					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_CONN_ACCEPT)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL1, "Connect accept timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					Hci_StopTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					Hci_Clear_AM_Addr(devExt, 0);
					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);

					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_MONITER_CONN_COMP)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL1, "Moniter connect complete timeout, pConnectDevice = 0x%x\n", pConnectDevice);
					Hci_StopTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					Hci_Clear_AM_Addr(devExt, 0);

					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else if (pConnectDevice->timer_type == BT_TIMER_TYPE_MONITER_LINK_SUPERVISION)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL1, "Disconnect slave because link supersion timer is not set after link is connected!\n");
					Hci_StopTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;
					Hci_StopAllTimer(pConnectDevice);
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					Hci_Clear_AM_Addr(devExt, 0);
					
					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
					Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						BtUsbWriteClearComIndicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, WriteIndicator);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_flag = 1;
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else
				{
				}
			}
		}
		if (pConnectDevice->pScoConnectDevice != NULL)
		{
			if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_valid == 1)
			{
				if (--((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_counter == 0)
				{
					if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->timer_type == BT_TIMER_TYPE_MONITER_CONN_COMP)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL1, "Monitor connection complete timeout, pScoConnectDevice = 0x%x\n", pConnectDevice->pScoConnectDevice);
						Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice));
						((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_HOST_TIMEOUT;
						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}					
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Frag_InitTxScoForConnDev(devExt, pConnectDevice);
						Task_HCI2HC_Send_Sco_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
						Timer_flag = 1;
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						if (pHci->sco_link_count == 0)
						{
							pHci->auto_packet_select_flag = 1;
						}
						{
							LARGE_INTEGER timevalue;
							Timer_flag = 1;
							
						}
					}
				}
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	for (i = 0; i < tmplen; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Timeout()--delete conn in turn, index:%d, am_addr:%d\n\n", i, tmpamaddr[i]);
		Hci_Del_Connect_Device_By_AMAddr(pHci, tmpamaddr[i]);
	}
	for (i = 0; i < tmpslavelen; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Timeout()--delete conn in turn, index:%d, am_addr:%d\n\n", i, tmpslaveamaddr[i]);
		Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(pHci,tmpslaveamaddr[i], tmpslaveindex[i]);
	}
	#ifdef BT_AUTO_SEL_PACKET
		if ((tmplen > 0) || (tmpslavelen > 0))
		{
			if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
			{
				devExt->TxAclSendCount = 0;
				devExt->StartRetryCount = 0;
				devExt->EndRetryCount = 0;
				devExt->TxRetryCount = 0;
				Hci_ClearMainlyCommandIndicator(devExt);
			}
		}
	#endif
	if (Timer_flag == 1)
	{
		timevalue.QuadPart = 0;
		tasklet_schedule(&devExt->taskletRcv);
	}
}
/**************************************************************************
 *   Hci_ScoStartTimer
 *
 *   Descriptions:
 *      Start a sco timer for HCI module
 *   Arguments:
 *      pScoConnectDevice: IN, pointer to sco connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ScoStartTimer(PSCO_CONNECT_DEVICE_T pScoConnectDevice, UINT8 timer_type, UINT16 timer_count)
{
	if (pScoConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ScoStartTimer() entered\n");
	pScoConnectDevice->timer_valid = 1;
	pScoConnectDevice->timer_type = timer_type;
	pScoConnectDevice->timer_counter = timer_count;
}
/**************************************************************************
 *   Hci_ScoStopTimer
 *
 *   Descriptions:
 *      Stop a sco timer for HCI module
 *   Arguments:
 *      pScoConnectDevice: IN, pointer to sco connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ScoStopTimer(PSCO_CONNECT_DEVICE_T pScoConnectDevice)
{
	if (pScoConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ScoStopTimer() entered\n");
	pScoConnectDevice->timer_valid = 0;
	pScoConnectDevice->timer_type = BT_TIMER_TYPE_INVALID;
	pScoConnectDevice->timer_counter = 0;
}
/**************************************************************************
 *   Hci_StartL2capFlowTimer
 *
 *   Descriptions:
 *      Start a timer for l2cap flow control
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartL2capFlowTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_StartL2capFlowTimer\n");
	pConnectDevice->timer_l2cap_flow_valid = 1;
	pConnectDevice->timer_l2cap_flow_counter = 0;

}
/**************************************************************************
 *   Hci_StopL2capFlowTimer
 *
 *   Descriptions:
 *      Start a timer for l2cap flow control
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopL2capFlowTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	LARGE_INTEGER timevalue;
	if (pConnectDevice == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_StopL2capFlowTimer\n");
	pConnectDevice->timer_l2cap_flow_valid = 0;
	pConnectDevice->timer_l2cap_flow_counter = 0;

	//Frag_StartQueue(devExt, FRAG_MASTER_LIST);
	//Frag_StartQueue(devExt, FRAG_SLAVE_LIST);
	//Frag_StartQueue(devExt, FRAG_SLAVE1_LIST);
	timevalue.QuadPart = (kOneMillisec); // Just raise the IRQL and process the task sdft in another DPC process.
	KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);

}
/**************************************************************************
 *   Hci_L2capFlowTimeout
 *
 *   Descriptions:
 *      Time out function for l2cap flow control
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *   Return Value:
 *      None
 *    ChangeLog:
 *	Jakio change this routine accroding netcafe
 *************************************************************************/
VOID Hci_L2capFlowTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 tmpamaddr[8];
	UINT8 tmpslaveamaddr[3];
	UINT8 tmpslaveindex[3];
	UINT8 i;
	UINT8 tmplen = 0;
	UINT8 tmpslavelen = 0;
	UINT8 linkrelflag = 0;
	UINT8 slaveindex  = 0;
	LARGE_INTEGER timevalue;
		
	if (devExt == NULL)
		return ;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;


	// Scan master list
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		// If we found the item which has the same bd addr, we take it from am list and return it to free list
		if (pConnectDevice->timer_l2cap_flow_valid == 1)
		{
			linkrelflag = 0;
			/* jakio20080312: for test
*/
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_L2capFlowTimeout--Device:%8x,Class:%2x %2x %2x\n",pConnectDevice,
				pConnectDevice->class_of_device[0],pConnectDevice->class_of_device[1], pConnectDevice->class_of_device[2]);
		#ifdef BT_MATCH_CLASS_OF_DEVICE
			if ((pConnectDevice->class_of_device[1] & 0x1f) == 0x5) // mouse and keyboard
		#else
			if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
		#endif
			{
				if (++pConnectDevice->timer_l2cap_flow_counter >= BT_L2CAP_TIMEOUT_SNIFF_COUNT)
					linkrelflag = 1;

			}
			else
			{
				if (++pConnectDevice->timer_l2cap_flow_counter >= BT_L2CAP_TIMEOUT_ACTIVE_COUNT)
					linkrelflag = 1;
			}
			if (linkrelflag)
			{
				Hci_StopL2capFlowTimer(devExt,pConnectDevice);
				pConnectDevice->l2cap_flow_control_flag = 0;

				BT_DBGEXT(ZONE_HCI | LEVEL3, "L2cap flow timeout in slave list\n");
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Link supervision timeout, pConnectDevice = 0x%x\n", pConnectDevice);
		
				Hci_StopTimer(pConnectDevice);
				pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			
				pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;

				pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;

				Hci_StopAllTimer(pConnectDevice);
	
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Hci_InitLMPMembers(devExt, pConnectDevice);

				Hci_InitTxForConnDev(devExt, pConnectDevice);

				Hci_ReleaseScoLink(devExt, pConnectDevice);


				devExt->AllowIntSendingFlag = FALSE;
				Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 1);


				Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);
				Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

				#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
				#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
				#endif


				Hci_Del_Connect_Device_By_AMAddr(pHci, pConnectDevice->am_addr);
				
				#ifdef BT_AUTO_SEL_PACKET
				if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
				{
					devExt->TxAclSendCount = 0;
					devExt->StartRetryCount = 0;
					devExt->EndRetryCount = 0;
					devExt->TxRetryCount = 0;
					Hci_ClearMainlyCommandIndicator(devExt);
				}
				#endif
				
				

				devExt->AllowIntSendingFlag = TRUE;
				timevalue.QuadPart = 0;
				// Just raise the IRQL and process the task sdft in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);

				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
		}

		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}

	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->timer_l2cap_flow_valid == 1)
		{
			linkrelflag = 0;
		#ifdef BT_MATCH_CLASS_OF_DEVICE
			if ((pConnectDevice->class_of_device[1] & 0x1f) == 0x5) // mouse and keyboard
		#else
			if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
		#endif
			{
				if (++pConnectDevice->timer_l2cap_flow_counter >= BT_L2CAP_TIMEOUT_SNIFF_COUNT)
					linkrelflag = 1;

			}
			else
			{
				if (++pConnectDevice->timer_l2cap_flow_counter >= BT_L2CAP_TIMEOUT_ACTIVE_COUNT)
					linkrelflag = 1;
			}
			if (linkrelflag)
			{
				Hci_StopL2capFlowTimer(devExt,pConnectDevice);
				pConnectDevice->l2cap_flow_control_flag = 0;

				BT_DBGEXT(ZONE_HCI | LEVEL3, "Link supervision timeout, pConnectDevice = 0x%x\n", pConnectDevice);

				
				Hci_StopTimer(pConnectDevice);

				pConnectDevice->state = BT_DEVICE_STATE_IDLE;
				
				pConnectDevice->status = BT_HCI_ERROR_CONNECTION_TIMEOUT;

				pConnectDevice->current_reason_code = BT_HCI_ERROR_CONNECTION_TIMEOUT;

				Hci_StopAllTimer(pConnectDevice);

				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Hci_InitLMPMembers(devExt, pConnectDevice);

				slaveindex = pConnectDevice->slave_index;
				slaveindex = 0;
				



				Hci_InitTxForConnDev(devExt, pConnectDevice);

				Hci_ReleaseScoLink(devExt, pConnectDevice);

				devExt->AllowIntSendingFlag  = FALSE;
				Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 0);

				Hci_Write_One_Byte(devExt,  BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
								Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
			#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif

				Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(pHci, pConnectDevice->am_addr, slaveindex);

				#ifdef BT_AUTO_SEL_PACKET
				if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
				{
					devExt->TxAclSendCount = 0;
					devExt->StartRetryCount = 0;
					devExt->EndRetryCount = 0;
					devExt->TxRetryCount = 0;
					Hci_ClearMainlyCommandIndicator(devExt);
				}
				#endif
				
				

				devExt->AllowIntSendingFlag = TRUE;

				timevalue.QuadPart = 0;
				// Just raise the IRQL and process the task sdft in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);



				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);


}



/**************************************************************************
 *   Hci_Receive_From_LMP
 *
 *   Descriptions:
 *      Process the condition that HCI receives a event from LMP
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      event: IN, event type
 *      para: IN, parameters
 *      len: IN, the length of parameters
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Receive_From_LMP(PBT_DEVICE_EXT devExt, UINT16 event, PUINT8 para, UINT16 len)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	LARGE_INTEGER timevalue;
	PBT_FRAG_T pFrag;
	UINT8 id;
	UINT8 tmpamaddr = 0;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	// Get connect device structure
	RtlCopyMemory(&pTempConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pTempConnectDevice == NULL)
	{
		return ;
	}
	switch (event)
	{
		case (BT_LMP_EVENT_SETUP_COMPLETE): 
			switch (pTempConnectDevice->state)
			{
				case BT_DEVICE_STATE_PAGED: 
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP setup complete event in state BT_DEVICE_STATE_PAGED\n");
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout * 625) / (1000 * 1000)  + 1)); 
					if (pTempConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						pTempConnectDevice->state = BT_DEVICE_STATE_CONNECTED;
						#ifdef BT_AFH_ADJUST_MAP_SUPPORT
							Afh_ClearPrevAfhMap(devExt);
						#endif

						#ifdef BT_AFH_SUPPORT // Set AFH for this connection
						pHci->afh_timer_count = 0; // Reset check afh timer count
						pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_SET_AFH_FOR_ONE_CONNECTION;
						pHci->pcurrent_connect_device = pTempConnectDevice;
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						#endif
					}
					else
					{
						pTempConnectDevice->state = BT_DEVICE_STATE_SLAVE_CONNECTED;
					}
					pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
					break;
				case BT_DEVICE_STATE_ACCEPTED:
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP setup complete event in state BT_DEVICE_STATE_ACCEPTED\n");
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout * 625) / (1000 * 1000)  + 1)); 
					if (pTempConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						pTempConnectDevice->state = BT_DEVICE_STATE_CONNECTED;
						#ifdef BT_AFH_ADJUST_MAP_SUPPORT
							Afh_ClearPrevAfhMap(devExt);
						#endif
						
						#ifdef BT_AFH_SUPPORT // Set AFH for this connection
						pHci->afh_timer_count = 0; // Reset check afh timer count
						pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_SET_AFH_FOR_ONE_CONNECTION;
						pHci->pcurrent_connect_device = pTempConnectDevice;
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						#endif
					}
					else
					{
						pTempConnectDevice->state = BT_DEVICE_STATE_SLAVE_CONNECTED;
					}
					pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
					break;
				}
			break;
		case (BT_LMP_EVENT_DETACH): 
			switch (pTempConnectDevice->state)
			{
				case BT_DEVICE_STATE_PAGED:
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_PAGED\n");

					pTempConnectDevice->state = BT_DEVICE_STATE_PAGED_DISCONNECT;
					Hci_StartPollTimer(devExt, pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));


					timevalue.QuadPart = 0; // Just raise the IRQL and process the task sdft in another DPC process.
					tasklet_schedule(&devExt->taskletRcv);
					break;
				case BT_DEVICE_STATE_NOT_ACCEPTED:
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_NOT_ACCEPTED\n");


					pTempConnectDevice->state = BT_DEVICE_STATE_NOT_ACCEPTED_DISCONNECT;
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));


					timevalue.QuadPart = 0; // Just raise the IRQL and process the task sdft in another DPC process.
					tasklet_schedule(&devExt->taskletRcv);
					break;
				case BT_DEVICE_STATE_SLAVE_CONNECTED: 
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_SLAVE_CONNECTED\n");
					pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN;
					pTempConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_LIMITED_RES;
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *3 * 625 / 5000) + 1));
					break;
				case BT_DEVICE_STATE_CONNECTED: 
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_CONNECTED\n");
					pTempConnectDevice->state = BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN;
					pTempConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_LIMITED_RES;
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *6 * 625 / 5000) + 1));
					break;
				case BT_DEVICE_STATE_WAIT_CONN_REQ: 
				case BT_DEVICE_STATE_ACCEPTED: 
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_WAIT_CONN_REQ or BT_DEVICE_STATE_ACCEPTED\n");

					pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED_DISCONNECT;
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));


					tasklet_schedule(&devExt->taskletRcv);
					break;
				case BT_DEVICE_STATE_DISCONNECT: 
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP detach event in state BT_DEVICE_STATE_DISCONNECT\n");
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)(((UINT32)pTempConnectDevice->per_poll_interval *6 * 625 / 5000) + 1));
					break;
				case BT_DEVICE_STATE_SCO_REMOVED:
				case BT_DEVICE_STATE_ESCO_REMOVED:
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive LMP detach event in state BT_DEVICE_STATE_SCO_REMOVED or BT_DEVICE_STATE_ESCO_REMOVED\n");
					KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
					pTempConnectDevice->state = pTempConnectDevice->oldstate;
					KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pTempConnectDevice, 4);
					break;
			}
			break;
		case BT_LMP_EVENT_CLKOFFSET_RES: 
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP CLKOFFSET event\n");
			Task_HCI2HC_Send_ClkOffset_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			break;
		case BT_LMP_EVENT_NAME_RES: 
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP NAME event\n");
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if (pTempConnectDevice)
			{
				if (pTempConnectDevice->is_in_remote_name_req_flag == 0)
				{
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					break;
				}
				pTempConnectDevice->is_in_remote_name_req_flag = 0;
			}
			pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			if (pTempConnectDevice->tempflag != 1)
			{
				Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			}
			if (pTempConnectDevice->tempflag == 1)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_TEMP_DISCONNECT;
				Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 *625 / 5000) + 1));
			}
		break;
		case (BT_LMP_EVENT_FEATURE_RES):
			switch (pTempConnectDevice->state)
			{
			case BT_DEVICE_STATE_PAGED: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP FEATURE event in state BT_DEVICE_STATE_PAGED\n");
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CREATE_CONNECTION);
				break;
			default:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP FEATURE event in normal state\n");
				Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			}
			break;
		case BT_LMP_EVENT_EXTENDED_FEATURE_RES:
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive LMP EXTENDED FEATURE event in normal state\n");

			ASSERT(0);
			break;
		case BT_LMP_EVENT_VERSION_RES:
			switch (pTempConnectDevice->state)
			{
			case (BT_DEVICE_STATE_PAGED): 
                BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP VERSION event in state BT_DEVICE_STATE_PAGED\n");
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_FEATURE_REQ);
				break;
			default:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP VERSION event in normal state\n");
				Task_HCI2HC_Send_RemoteVerInfo_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
			}
			break;
		case (BT_LMP_EVENT_HOST_CONN_REQ): 
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LMP HOST_CONN_REQ event\n");
	#ifdef BT_EVENT_FILTER_SUPPORT
		    {
			UINT8 auto_accept_flag;

			BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive LMP HOST_CONN_REQ event\n");
			auto_accept_flag = 	EventFilter_MatchConnectionSetup((PBT_EVENT_FILTER_T)devExt->pEventFilter, pTempConnectDevice->class_of_device, pTempConnectDevice->bd_addr, BT_ACL_IN_USE);
			if (auto_accept_flag == EVENT_FILTER_AUTO_ACCEPT_FLAG_OFF)
			{
				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
				pTempConnectDevice->state = BT_DEVICE_STATE_WAIT_ACCEPT_CONN_REQ;
				pTempConnectDevice->acl_or_sco_flag = BT_ACL_IN_USE;
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
				Task_HCI2HC_Send_Conn_Req_Event(devExt, (PUINT8)&pTempConnectDevice);
			}
			else if (EVENT_FILTER_AUTO_ACCEPT_FLAG_ON_NO_ROLE_SWITCH)
			{
				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

				pTempConnectDevice->acl_or_sco_flag = BT_ACL_IN_USE;
				pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
				Hci_StopTimer(pTempConnectDevice);
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);

				pHci->pcurrent_connect_device = pTempConnectDevice;
				pHci->current_connection_handle = pTempConnectDevice->connection_handle;
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN);
			}
			else // auto accept with role switching
			{
				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

				pTempConnectDevice->acl_or_sco_flag = BT_ACL_IN_USE;

				pTempConnectDevice->state = BT_DEVICE_STATE_WAIT_ROLE_SWITCH;
				Hci_StopTimer(pTempConnectDevice);
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);

				pTempConnectDevice->role_switching_flag = 1;
				pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

				
				Hci_Write_One_Byte(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);

				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
				Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE, 10);

				pHci->pcurrent_connect_device = pTempConnectDevice;
				pHci->current_connection_handle = pTempConnectDevice->connection_handle;
				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
				
			}
		}
	#else
		        BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive LMP HOST_CONN_REQ event\n");
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				pTempConnectDevice->state = BT_DEVICE_STATE_WAIT_ACCEPT_CONN_REQ;
				pTempConnectDevice->acl_or_sco_flag = BT_ACL_IN_USE;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_HCI2HC_Send_Conn_Req_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
	#endif
                break;
			case BT_LMP_EVENT_LMP_TIMEOUT: 
				if (((pTempConnectDevice->raw_role == BT_ROLE_MASTER) && (pTempConnectDevice->state != BT_DEVICE_STATE_CONNECTED)) ||
					((pTempConnectDevice->raw_role == BT_ROLE_SLAVE) && (pTempConnectDevice->state != BT_DEVICE_STATE_SLAVE_CONNECTED)))
				{
					pTempConnectDevice->state = BT_DEVICE_STATE_LMP_TIMEOUT_DISCONNECT;
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));

				}
		
				timevalue.QuadPart = 0; // Just raise the IRQL and process the task sdft in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);
				break;
			case BT_LMP_EVENT_LINK_SUPERVISION_TIMEOUT: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LINK_SUPERVISION TIMEOUT event\n");
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
        		if (pTempConnectDevice->real_link_supervision_timeout == 0)
        		{
        			pTempConnectDevice->link_supervision_timeout = 0xffff;
        		}
        		else
        		{
        			if (pTempConnectDevice->real_link_supervision_timeout < 8000) // The min value driver supports is 8000, 5 seconds
        				pTempConnectDevice->link_supervision_timeout = 8000;
        			else
        				pTempConnectDevice->link_supervision_timeout = pTempConnectDevice->real_link_supervision_timeout;
        		}
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pTempConnectDevice->link_supervision_timeout *625) / (1000 *1000) + 1));
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				break;
			case BT_LMP_EVENT_LINK_KEY_REQ: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LINK_KEY_REQ event\n");
				Task_HCI2HC_Send_Link_Key_Req_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_AUTH_FAILURE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive AUTH_FAILURE event\n");
				pTempConnectDevice->state = BT_DEVICE_STATE_AUTH_FAILURE_DISCONNECT;
				Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));
				
				//Jakio20070808: set IntSendingTimer
				timevalue.QuadPart = 0; // Just raise the IRQL and process the task sdft in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);
				break;
			case BT_LMP_EVENT_LINK_KEY_NOTIFICATION: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive LINK_KEY_NOTIFICATION event\n");
				Task_HCI2HC_Send_Link_Key_Notification_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_PIN_CODE_REQ: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive PIN_CODE_REQ event\n");
				Task_HCI2HC_Send_Pin_Code_Req_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_PAIR_NOT_ALLOW: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive PAIR_NOT_ALLOW event\n");
				pTempConnectDevice->state = BT_DEVICE_STATE_PAIR_NOT_ALLOW_DISCONNECT;
				Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));
			
				//Jakio20070808: set IntSendingTimer
				timevalue.QuadPart = 0;
				tasklet_schedule(&devExt->taskletRcv);
				break;
			case BT_LMP_EVENT_ENCRYPTION_SUCCESS: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ENCRYPTION_SUCCESS event\n");
				pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
				Task_HCI2HC_Send_Encryption_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_ENCRYPTION_FAILURE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ENCRYPTION_FAILURE event\n");
				pTempConnectDevice->status = pTempConnectDevice->current_reason_code;
				Task_HCI2HC_Send_Encryption_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_ENCRYPTION_NOT_COMP: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ENCRYPTION_NOT_COMP event\n");
				pTempConnectDevice->state = BT_DEVICE_STATE_ENCRYPTION_NOT_COMP_DISCONNECT;
				Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));
				timevalue.QuadPart = 0;
				tasklet_schedule(&devExt->taskletRcv);
				break;
			case BT_LMP_EVENT_AUTH_COMP_SUCCESS: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive AUTH_COMP_SUCCESS event\n");
		
				if (pTempConnectDevice->pause_command_flag == 4)
					pTempConnectDevice->pause_command_flag = 0;
		
				pTempConnectDevice->status = BT_HCI_STATUS_SUCCESS;
				Task_HCI2HC_Send_Authentication_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_AUTH_COMP_FAILURE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive AUTH_COMP_FAILURE event\n");

				if (pTempConnectDevice->pause_command_flag == 4)
					pTempConnectDevice->pause_command_flag = 0;
				pTempConnectDevice->status = pTempConnectDevice->current_reason_code;
				Task_HCI2HC_Send_Authentication_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_SCO_LINK_REQ: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive SCO_LINK_REQ event\n");
				if (pTempConnectDevice->pScoConnectDevice == NULL)
				{
					pTempConnectDevice->pScoConnectDevice = Hci_Add_Sco_Connect_Device(pHci, BT_PACKET_TYPE_HV1, (PVOID)pTempConnectDevice);
					if (pTempConnectDevice->pScoConnectDevice != NULL)
					{
				#ifdef BT_TEST_SCO_DELAY_COUNTS
					devExt->RecvTotalScoCount = 0;
					devExt->RecvRealScoCount = 0;
					devExt->RecvScoNullCount = 0;
					devExt->RecvAddExtraCount = 0;
					devExt->TxTotalScoCount = 0;
					devExt->TxRealScoCount = 0;
					devExt->TxDiscardSpilthScoCount = 0;
					devExt->TxReservedScoCount = 0;
				#endif
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						pTempConnectDevice->acl_or_sco_flag = BT_SCO_IN_USE;
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->initiator = 0;
						Hci_ScoStartTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice), BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						if (pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt) || !Frag_IsEleEmpty(devExt))
						{
							pHci->acl_temp_pending_flag = 1;
							pHci->auto_packet_select_flag = 0;
							pHci->sco_need_1_slot_for_acl_flag = 1;
							pHci->hv1_use_dv_instead_dh1_flag = 0;
							Frag_StopQueueForSco(devExt);
							UsbQueryDMASpace(devExt);
						}
						id = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->index;
						ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
						BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--SCO index = %d\n", id);
						/*Get pointer to the test */
						pFrag = (PBT_FRAG_T)devExt->pFrag;
						pFrag->RxScoElement[id].currentpos = 0;
						pFrag->RxScoElement[id].currentlen = 0;
						pFrag->RxScoElement[id].totalcount = 0;
						
						if (pHci->acl_temp_pending_flag)
						{
							Task_Normal_Send_Conn_Req(devExt, (PUINT8) &pTempConnectDevice, 1);
						}
						else
						{
							pHci->auto_packet_select_flag = 0;
							pHci->sco_need_1_slot_for_acl_flag = 1;
							pHci->hv1_use_dv_instead_dh1_flag = 0;
							Task_HCI2HC_Send_Conn_Req_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
						}
					}
				}
				else
				{
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					if (pTempConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->initiator == 0)
						{
							pHci->pcurrent_connect_device = pTempConnectDevice;
							KeReleaseSpinLock(&pHci->HciLock, oldIrql);
							Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 1);
							Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT);
							LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ADD_SCO);
							KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						}
					}
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				}
				break;
			case BT_LMP_EVENT_SCO_LMP_TIMEOUT: 
			case BT_LMP_EVENT_SCO_DETACH:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive SCO_LMP_TIMEOUT or SCO_DETACH event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));
					if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->initiator)
					{
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_USER_ENDED_CONNECTION;
					}
					else
					{
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_CONNECTION_TERMINATED_BY_LOCAL_HOST;
					}
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
					{
						Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->esco_amaddr);
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
					}
					Frag_InitTxScoForConnDev(devExt, pTempConnectDevice);
					if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->using_esco_command_flag)
					{
						Task_HCI2HC_Send_Sync_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
					}
					else
					{
						Task_HCI2HC_Send_Sco_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
					}
					Hci_Del_Sco_Connect_Device(pHci, (PVOID)pTempConnectDevice, pTempConnectDevice->pScoConnectDevice);
					Frag_InitTxScoForAllDev(devExt);
					if (pHci->sco_link_count == 0)
					{
						Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
						pHci->auto_packet_select_flag = 1;
					}
					
					if(pHci->acl_temp_pending_flag == 1)
					{
						pHci->acl_temp_pending_flag = 0;
						Frag_StartQueueForSco(devExt, 0);	
					}

					{
						LARGE_INTEGER timevalue;
						timevalue.QuadPart =  0;
						tasklet_schedule(&devExt->taskletRcv);
					}
				}
				break;
			case BT_LMP_EVENT_SCO_CONNECTED: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive SCO_CONNECTED event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->using_esco_command_flag)
					{
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					if ((((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_packet_type == BT_SCO_PACKET_HV2) ||
						(((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_packet_type == BT_SCO_PACKET_HV1))
						pHci->sco_need_1_slot_for_acl_flag = 1;
					else
						pHci->sco_need_1_slot_for_acl_flag = 0;
						if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_packet_type == BT_SCO_PACKET_HV1)
						{
							pHci->hv1_use_dv_instead_dh1_flag = 1;
						}
						else
						{
							pHci->hv1_use_dv_instead_dh1_flag = 0;
						}
						Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->state = BT_DEVICE_STATE_CONNECTED;
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_STATUS_SUCCESS;
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						Task_HCI2HC_Send_Sync_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
					}
					else
					{
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_packet_type == BT_SCO_PACKET_HV3)
						{
							pHci->sco_need_1_slot_for_acl_flag = 0;
						}
						else
						{
							pHci->sco_need_1_slot_for_acl_flag = 1;
						}
						if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->current_packet_type == BT_SCO_PACKET_HV1)
						{
							pHci->hv1_use_dv_instead_dh1_flag = 1;
						}
						else
						{
							pHci->hv1_use_dv_instead_dh1_flag = 0;
						}
						Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->state = BT_DEVICE_STATE_CONNECTED;
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_STATUS_SUCCESS;
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);

						Frag_StartQueueForSco(devExt, 1);
						Task_HCI2HC_Send_Sco_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
						
					}
				}
				break;
			case BT_LMP_EVENT_SCO_REMOVED: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive SCO_REMOVED event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));

					pTempConnectDevice->oldstate = pTempConnectDevice->state;
					pTempConnectDevice->state = BT_DEVICE_STATE_SCO_REMOVED;
				#ifdef BT_TESTMODE_SUPPORT
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));
				#else
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, 1);
				#endif

					Frag_InitScoRxBuffer(devExt, pTempConnectDevice);

				}
				break;
			case BT_LMP_EVENT_MODE_CHANGE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive MODE_CHANGE event\n");
				pTempConnectDevice->status = pTempConnectDevice->current_reason_code;
				Task_HCI2HC_Send_Mode_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_ROLE_SWITCH_FAIL: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ROLE_SWITCH_FAIL event\n");
				/*
				if((pHci->role_switch_fail_count<3)&&(pTempConnectDevice->state==BT_DEVICE_STATE_SLAVE_CONNECTED))
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive ROLE_SWITCH_FAIL event:retry role change, just retry it!\n");
					break;
				}
				else
				{
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive ROLE_SWITCH_FAIL event:retry role change not match, just do it!\n");
				}
				*/
				// Change the hci command state
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				if (pHci->protect_hci_command_ack_timer_valid)
				{
					if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL3, "Stop hci timer for role switch fail\n");
						Hci_StopProtectHciCommandTimer(pHci);
					}
				}
				pTempConnectDevice->status = BT_HCI_ERROR_ROLE_SWITCH_FAILED;
						pTempConnectDevice->role_switching_flag = 0;
				pHci->role_switching_flag = 0;
				if (pTempConnectDevice->pause_command_flag == 1) // Add for pause encryption by Lewis.wang
					pTempConnectDevice->pause_command_flag = 0;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				if (pTempConnectDevice->role_switching_role == BT_ROLE_MASTER)
				{
					if (pTempConnectDevice->am_addr_for_fhs != 0)
					{
						Hci_Free_Am_address(pHci, pTempConnectDevice->am_addr_for_fhs);
					}
				}
				Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 0);

				if (pTempConnectDevice->state == BT_DEVICE_STATE_WAIT_ROLE_SWITCH)
				{
					
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
					Hci_StopTimer(pTempConnectDevice);
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
					pHci->pcurrent_connect_device = pTempConnectDevice;
					pHci->current_connection_handle = pTempConnectDevice->connection_handle;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN);
	
				}				
						
				{ //Jakio20071126: add here accroding netcafe
					UINT8 tmpbdaddr[6] = {0,0,0,0,0,0};
					Hci_Write_BD_Address(devExt, BT_REG_OP_BD_ADDR_DSP, tmpbdaddr);
				}

				timevalue.QuadPart = 0; // Just raise the IRQL and process the task immudiatly in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);
	
				break;
			case BT_LMP_EVENT_ESCO_CONNECTED: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ESCO_CONNECTED event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					pHci->sco_need_1_slot_for_acl_flag = 0;
					pHci->hv1_use_dv_instead_dh1_flag = 0;
					Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));
					((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->state = BT_DEVICE_STATE_CONNECTED;
					((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_STATUS_SUCCESS;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Sync_Connection_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				}
				break;
			case BT_LMP_EVENT_ESCO_CHANGED: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ESCO_CHANGED event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_STATUS_SUCCESS;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Sync_Connection_Changed_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				}
				break;
			case BT_LMP_EVENT_ESCO_CHANGE_FAIL: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ESCO_CHANGE_FAIL event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
					((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_LMP_PDU_NOT_ALLOWED;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Task_HCI2HC_Send_Sync_Connection_Changed_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				}
				break;
			case BT_LMP_EVENT_ESCO_REMOVED:  // The same process as the SCO Removed.
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ESCO_REMOVED event\n");
				if (pTempConnectDevice->pScoConnectDevice != NULL)
				{
					Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice));
					pTempConnectDevice->oldstate = pTempConnectDevice->state;
					pTempConnectDevice->state = BT_DEVICE_STATE_ESCO_REMOVED;
				#ifdef BT_TESTMODE_SUPPORT
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, (UINT16)((64 * 625 / 5000) + 1));
				#else
					Hci_StartPollTimer(devExt,pHci, pTempConnectDevice, 1);
				#endif
			
					Frag_InitScoRxBuffer(devExt, pTempConnectDevice);
			
				}
				//Jakio20070808: set IntSendingTimer
				timevalue.QuadPart = 0;
				tasklet_schedule(&devExt->taskletRcv);
				break;
			case BT_LMP_EVENT_ESCO_LINK_REQ: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive ESCO_LINK_REQ event\n");
				if (pTempConnectDevice->pScoConnectDevice == NULL)
				{
					pTempConnectDevice->pScoConnectDevice = Hci_Add_eSco_Connect_Device(pHci, 0, 0, 0, 0, 0, 0, (PVOID)pTempConnectDevice);
					if (pTempConnectDevice->pScoConnectDevice != NULL)
					{
				#ifdef BT_TEST_SCO_DELAY_COUNTS
					devExt->RecvTotalScoCount = 0;
					devExt->RecvRealScoCount = 0;
					devExt->RecvScoNullCount = 0;
					devExt->RecvAddExtraCount = 0;
					devExt->TxTotalScoCount = 0;
					devExt->TxRealScoCount = 0;
					devExt->TxDiscardSpilthScoCount = 0;
					devExt->TxReservedScoCount = 0;
				#endif
						KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						pTempConnectDevice->acl_or_sco_flag = BT_SCO_IN_USE;
						((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->initiator = 0;
						Hci_ScoStartTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice), BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
						KeReleaseSpinLock(&pHci->HciLock, oldIrql);
						if (pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt) || !Frag_IsEleEmpty(devExt))
						{
							pHci->acl_temp_pending_flag = 1;
							pHci->auto_packet_select_flag = 0;
							pHci->sco_need_1_slot_for_acl_flag = 1;
							pHci->hv1_use_dv_instead_dh1_flag = 0;
							Frag_StopQueueForSco(devExt);
							UsbQueryDMASpace(devExt);
						}
						id = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->index;
						ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
						BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--SCO index = %d\n", id);
						/*Get pointer to the test */
						pFrag = (PBT_FRAG_T)devExt->pFrag;
						pFrag->RxScoElement[id].currentpos = 0;
						pFrag->RxScoElement[id].currentlen = 0;
						pFrag->RxScoElement[id].totalcount = 0;
						
						if (pTempConnectDevice->current_role == BT_ROLE_MASTER)
						{
							if (((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->esco_amaddr == 0)
							{
								Hci_Allocate_Am_address(pHci, &tmpamaddr);
								((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->esco_amaddr = tmpamaddr;
							}
						}
						if (pHci->acl_temp_pending_flag)
						{
							Task_Normal_Send_Conn_Req(devExt, (PUINT8) &pTempConnectDevice, 1);
						}
						else
						{
							pHci->auto_packet_select_flag = 0;
							pHci->sco_need_1_slot_for_acl_flag = 1;
							pHci->hv1_use_dv_instead_dh1_flag = 0;
							Task_HCI2HC_Send_Conn_Req_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
						}
					}
				}
				break;
			case BT_LMP_EVENT_CHANGE_TX_MAX_SLOT: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive CHANGE_TX_MAX_SLOT event, current tx max slot = %d\n", pTempConnectDevice->tx_max_slot);
				Frag_GetCurrentPacketType(devExt, pTempConnectDevice);
				if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
				{
					pTempConnectDevice->current_packet_type = BT_ACL_PACKET_DH5;
				}
				else if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
				{
					pTempConnectDevice->current_packet_type = BT_ACL_PACKET_DH3;
				}
				else
				{
					pTempConnectDevice->current_packet_type = BT_ACL_PACKET_DH1;
				}
				Task_HCI2HC_Send_Max_Slot_Changed_Event(devExt, (PUINT8)&pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_QOS_SETUP_COMPLETE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive QOS_SETUP_COMPLETE event\n");
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				pTempConnectDevice->status = pTempConnectDevice->current_reason_code;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_HCI2HC_Send_Qos_Setup_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE: 
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Receive_From_LMP()--Receive FLOW_SPECIFICATION_COMPLETE event\n");
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				pTempConnectDevice->status = pTempConnectDevice->current_reason_code;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Task_HCI2HC_Send_Flow_Specification_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
				break;

			case BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive CHANGE_SCO_PACKET_TYPE_COMPLETE event\n");
				Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event(devExt, (PUINT8)&pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive CHANGE_PACKET_TYPE_COMPLETE event\n");
				Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(devExt, (PUINT8)&pTempConnectDevice, 1);
				break;
			case BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY:
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Receive REFRESH_ENCRYPTION_KEY event\n");
				ASSERT(0);
				/*
				Task_HCI2HC_Send_Refresh_Encryption_Key_Changed_Complete_Event(devExt, (PUINT8)&pTempConnectDevice);
				if (pTempConnectDevice->pause_command_flag == 1)
				{
					Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8)&pTempConnectDevice);
				}
				else if (pTempConnectDevice->pause_command_flag == 3)
				{
				}
				pTempConnectDevice->pause_command_flag = 0;
				*/
				break;
	}
	tasklet_schedule(&devExt->taskletRcv);
}
/**************************************************************************
 *   Hci_Command_Reset
 *
 *   Descriptions:
 *      Process HCI command: Reset
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Reset(PBT_DEVICE_EXT devExt)
{
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Command_Reset Enter!\n");

	Task_HC2HCI_Reset(devExt);
}
/**************************************************************************
 *   Hci_Response_Reset
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Reset.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Reset(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Buffer_Size
 *
 *   Descriptions:
 *      Process HCI command: Read_Buffer_Size
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Buffer_Size(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_BUFFER_SIZE);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Buffer_Size
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Buffer_Size.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->acl_data_packet_length;
	dest += sizeof(UINT16);
	*dest = pHci->sco_data_packet_length;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->total_num_acl_data_packet;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pHci->total_num_sco_data_packet;
	dest += sizeof(UINT16);
	*pOutLen = 8;
}
/**************************************************************************
 *   Hci_Command_Read_Local_Version_Info
 *
 *   Descriptions:
 *      Process HCI command: Read_Local_Version_Info
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Local_Version_Info(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_LOCAL_VERSION_INFORMATION);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Local_Version_Info
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Local_Version_Info.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Local_Version_Info(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->hci_version;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->hci_revision;
	dest += sizeof(UINT16);
	*dest = pHci->lmp_version;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->manufacturer_name;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pHci->lmp_subversion;
	dest += sizeof(UINT16);
	*pOutLen = 9;
}
/**************************************************************************
 *   Hci_Command_Read_BD_Addr
 *
 *   Descriptions:
 *      Process HCI command: Read_BD_Addr
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_BD_Addr(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();

 	pHci = (PBT_HCI_T)devExt->pHci;

    KeAcquireSpinLock(&pHci->HciLock, &oldIrql);    

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_BD_ADDR);
    KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_BD_Addr
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_BD_Addr.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_BD_Addr(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pHci->local_bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	*pOutLen = 7;
}

void NotifyDspScanEnable(PBT_DEVICE_EXT devExt)
{
    UINT8 ScanEnable;
	PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;

    ScanEnable = pHci->scan_enable;
	if (ScanEnable & pHci->scan_enable_mask & BT_FOR_LC_SCAN_TYPE_INQUIRY_SCAN){
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT);
	}
	else{
		Hci_Clear_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT);
	}
	if (ScanEnable & pHci->scan_enable_mask & BT_FOR_LC_SCAN_TYPE_PAGE_SCAN){
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT);
	}
	else{
		Hci_Clear_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT);
	}
}

/**************************************************************************
 *   Hci_Command_Write_Scan_Enable
 *
 *   Descriptions:
 *      Process HCI command: Write_Scan_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ScanEnable: IN, Scan_Enable
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Scan_Enable(PBT_DEVICE_EXT devExt, UINT8 ScanEnable)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_SCAN_ENABLE);

	pHci->scan_enable = ScanEnable;
    NotifyDspScanEnable(devExt);
    
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Scan_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Scan_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Scan_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Scan_Enable
 *
 *   Descriptions:
 *      Process HCI command: Read_Scan_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Scan_Enable(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_SCAN_ENABLE);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Scan_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Scan_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Scan_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->scan_enable;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Conn_Accept_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Write_Conn_Accept_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnAcceptTimeout: IN, Conn_Accept_Timeout
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnAcceptTimeout)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
    // Get pointer of hci module
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_CONNECTION_ACCEPT_TIMEOUT);

	pHci->conn_accept_timeout = ConnAcceptTimeout;

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Conn_Accept_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Conn_Accept_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Conn_Accept_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Read_Conn_Accept_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_CONNECTION_ACCEPT_TIMEOUT);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Conn_Accept_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Conn_Accept_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->conn_accept_timeout;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Write_Page_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Write_Page_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      PageTimeout: IN, Page_Timeout
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Page_Timeout(PBT_DEVICE_EXT devExt, UINT16 PageTimeout)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#else	
	UINT8 Datastr[4];
#endif

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_PAGE_TIMEOUT);

	pHci->page_timeout = PageTimeout;
	if (pHci->page_extra_flag)
		pHci->page_timeout += 4096; // extend 2.5 seconds for combo mode

	BT_DBGEXT(ZONE_HCI | LEVEL3, "page timeout value = 0x%x\n", pHci->page_timeout);

    PageTimeout = PageTimeout > 0x4000 ? 0x4000 : PageTimeout;

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 2;
	offset = BT_REG_PAGE_TIMEOUT;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8)&PageTimeout);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else	
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, BT_REG_PAGE_TIMEOUT - 1, 1, Datastr);
	RtlCopyMemory(Datastr + 1, (PUINT8) &PageTimeout, 2);
	UsbWriteTo3DspRegs(devExt, BT_REG_PAGE_TIMEOUT - 1, 1, Datastr);
#endif

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Page_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Page_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Page_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Page_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Read_Page_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Page_Timeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_PAGE_TIMEOUT);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Page_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Page_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Page_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->page_timeout;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Write_Pin_Type
 *
 *   Descriptions:
 *      Process HCI command: Write_Pin_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      PinType: IN, IN_Type
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Pin_Type(PBT_DEVICE_EXT devExt, UINT8 PinType)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_PIN_TYPE);

	pHci->pin_type = PinType;

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Pin_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Pin_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Pin_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Pin_Type
 *
 *   Descriptions:
 *      Process HCI command: Read_Pin_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Pin_Type(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_PIN_TYPE);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Pin_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Pin_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Pin_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->pin_type;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Read_Local_Supported_Features
 *
 *   Descriptions:
 *      Process HCI command: Read_Local_Supported_Features
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Local_Supported_Features(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_FEATURES);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Local_Supported_Features
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Local_Supported_Features.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Local_Supported_Features(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	LMP_FEATURES_T features;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	RtlCopyMemory(&features, &pHci->lmp_features, sizeof(LMP_FEATURES_T));
	features.byte3.ext_sco_link = 1; // for bluez4.30

	RtlCopyMemory(dest, &features, sizeof(LMP_FEATURES_T));
	dest += sizeof(LMP_FEATURES_T);
	*pOutLen = 9;
}
/**************************************************************************
 *   Hci_Command_Read_Local_Extended_Features
 *
 *   Descriptions:
 *      Process HCI command: Read_Local_Extended_Features
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pagenumber: IN, page number
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Local_Extended_Features(PBT_DEVICE_EXT devExt, UINT8 pagenumber)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_Command_Read_Local_Extended_Features Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_LOCAL_EXTENDED_FEATURES);
	}

	pHci->current_page_number = pagenumber;

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );


	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

}

/**************************************************************************
 *   Hci_Response_Read_Local_Extended_Features
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Local_Extended_Features.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Local_Extended_Features(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*dest = pHci->current_page_number;
	dest += sizeof(UINT8);

	*dest = BT_MAX_FEATURE_PAGE_NUMBER;
	dest += sizeof(UINT8);

	if (pHci->current_page_number > BT_MAX_FEATURE_PAGE_NUMBER)
	{
		RtlZeroMemory(dest, sizeof(LMP_FEATURES_T));
	}
	else
	{
		RtlCopyMemory(dest, pHci->extended_feature_pointer[pHci->current_page_number], sizeof(LMP_FEATURES_T));
	}
	dest += sizeof(LMP_FEATURES_T);

	*pOutLen = 11;
}

/**************************************************************************
 *   Hci_Command_Change_Local_Name
 *
 *   Descriptions:
 *      Process HCI command: Change_Local_Name
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Name: IN, the pointer to Name
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Change_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 Name)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_CHANGE_LOCAL_NAME);
	RtlCopyMemory(pHci->local_name, Name, BT_LOCAL_NAME_LENGTH);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Change_Local_Name
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Change_Local_Name.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Change_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Local_Name
 *
 *   Descriptions:
 *      Process HCI command: Read_Local_Name
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Local_Name(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_LOCAL_NAME);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Local_Name
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Local_Name.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pHci->local_name, BT_LOCAL_NAME_LENGTH);
	dest += BT_LOCAL_NAME_LENGTH;
	*pOutLen = 249;
}

void NotifyDspDevClass(PBT_DEVICE_EXT devExt)
{
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
	PBT_HCI_T pHci;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	offset = BT_REG_FHS_FOR_INQUIRY_SCAN + 8 +3;
	length = BT_CLASS_OF_DEVICE_LENGTH;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, pHci->class_of_device);

	offset = BT_REG_FHS_FOR_PAGE + 8 +3;
	length = BT_CLASS_OF_DEVICE_LENGTH;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, pHci->class_of_device);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Write_Class_Of_Device
 *
 *   Descriptions:
 *      Process HCI command: Write_Class_Of_Device
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pClassOfDevice: IN, the pointer to Class_of_Device
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 pClassOfDevice)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;

	ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_CLASS_OF_DEVICE);
	RtlCopyMemory(pHci->class_of_device, pClassOfDevice, BT_CLASS_OF_DEVICE_LENGTH);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "pHci->class_of_device: %02x %02x %02x\n", pHci->class_of_device[0], pHci->class_of_device[1], pHci->class_of_device[2]);

	NotifyDspDevClass(devExt);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
	EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Class_Of_Device
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Class_Of_Device.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Write_Authentication_Enable
 *
 *   Descriptions:
 *      Process HCI command: Write_Authentication_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      AuthEnable: IN, Authentication_Enable
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Authentication_Enable(PBT_DEVICE_EXT devExt, UINT8 AuthEnable)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_AUTHENTICATION_ENABLE);

	pHci->authentication_enable = AuthEnable;
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Authentication_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Authentication_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Authentication_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Authentication_Enable
 *
 *   Descriptions:
 *      Process HCI command: Read_Authentication_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Authentication_Enable(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_AUTHENTICATION_ENABLE);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Authentication_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Authentication_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Authentication_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->authentication_enable;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Read_Class_Of_Device
 *
 *   Descriptions:
 *      Process HCI command: Read_Class_Of_Device
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Class_Of_Device(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_CLASS_OF_DEVICE);


    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Class_Of_Device
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Class_Of_Device.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	dest += BT_CLASS_OF_DEVICE_LENGTH;
	*pOutLen = 4;
}
/**************************************************************************
 *   Hci_Command_Write_Encryption_Mode
 *
 *   Descriptions:
 *      Process HCI command: Write_Encryption_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      EncryptionMode: IN, Encryption_Mode
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Encryption_Mode(PBT_DEVICE_EXT devExt, UINT8 EncryptionMode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_ENCRYPTION_MODE);

	pHci->encryption_mode = EncryptionMode;

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Encryption_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Encryption_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Encryption_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Encryption_Mode
 *
 *   Descriptions:
 *      Process HCI command: Read_Encryption_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Encryption_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_ENCRYPTION_MODE);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Encryption_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Encryption_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Encryption_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->encryption_mode;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Page_Scan_Activity
 *
 *   Descriptions:
 *      Process HCI command: Write_Page_Scan_Activity
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Interval: IN, Page_Scan_Interval
 *      Windwo: IN, Page_Scan_Window
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Page_Scan_Activity(PBT_DEVICE_EXT devExt, UINT16 Interval, UINT16 Window)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	UINT8 Datastr[4];
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_PAGE_SCAN_ACTIVITY);

	pHci->page_scan_interval = Interval;
	pHci->page_scan_window = Window;
	
#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 2;
	offset = BT_REG_PAGE_SCAN_WINDOW;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8)&Window);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else	
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, BT_REG_PAGE_SCAN_WINDOW - 1, 1, Datastr);
	RtlCopyMemory(Datastr + 1, (PUINT8) &Window, 2);
	UsbWriteTo3DspRegs(devExt, BT_REG_PAGE_SCAN_WINDOW - 1, 1, Datastr);
#endif

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Page_Scan_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Scan_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Page_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Page_Scan_Activity
 *
 *   Descriptions:
 *      Process HCI command: Read_Scan_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Page_Scan_Activity(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_PAGE_SCAN_ACTIVITY);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}

/**************************************************************************
 *   Hci_Response_Read_Page_Scan_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Scan_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Page_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->page_scan_interval;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pHci->page_scan_window;
	dest += sizeof(UINT16);
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Write_Inquiry_Scan_Activity
 *
 *   Descriptions:
 *      Process HCI command: Write_Inquiry_Scan_Activity
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Interval: IN, Inquiry_Scan_Interval
 *      Windwo: IN, Inquiry_Scan_Window
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, UINT16 Interval, UINT16 Window)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	UINT8 Datastr[4];
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_ACTIVITY);

	pHci->inquiry_scan_interval = Interval;
	pHci->inquiry_scan_window = Window;

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 2;
	offset = BT_REG_INQUIRY_SCAN_WINDOW;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8)&Window);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, BT_REG_INQUIRY_SCAN_WINDOW - 1, 1, Datastr);
	RtlCopyMemory(Datastr + 1, (PUINT8) &Window, 2);
	UsbWriteTo3DspRegs(devExt, BT_REG_INQUIRY_SCAN_WINDOW - 1, 1, Datastr);
#endif
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Inquiry_Scan_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Inquiry_Scan_Activity.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Inquiry_Scan_Activity
 *
 *   Descriptions:
 *      Process HCI command: Read_Inquiry_Scan_Activity
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_INQUIRY_SCAN_ACTIVITY);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}

/**************************************************************************
 *   Hci_Response_Read_Inquiry_Scan_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Inquiry_Scan_Activity.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->inquiry_scan_interval;
	dest += sizeof(UINT16);
	*(PUINT16)dest = pHci->inquiry_scan_window;
	dest += sizeof(UINT16);
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Inquiry
 *
 *   Descriptions:
 *      Process HCI command: Inquiry
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      InquiryPara: IN, the pointer to inquiry parameters
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Inquiry(PBT_DEVICE_EXT devExt, PUINT8 InquiryPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 tempaccesscode[9];
	UINT8 eventcode;
	UINT8 DataStr[12];
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Inquiry() Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_INQUIRY);

	if (pHci->is_in_inquiry_flag
        || (pHci->sco_link_count > 0 && pHci->hv1_use_dv_instead_dh1_flag)
        || (pHci->only_allow_one_acl_link_flag && pHci->num_device_am > 0)
       )
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquiry disallowed\n");

		pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		eventcode = BT_HCI_EVENT_COMMAND_STATUS;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
		return ;
	}

	pHci->is_in_inquiry_flag = 1;
	pHci->current_inquiry_length = *(InquiryPara + 3);
	pHci->current_inquiry_result_num = 0;
	pHci->current_num_responses = *(InquiryPara + 4);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	if (pHci->inquiry_extra_flag)
		pHci->current_inquiry_length += *(InquiryPara + 3); // doulbe for combo mode

	BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquiry length = %d, num responses = %d\n", pHci->current_inquiry_length, pHci->current_num_responses);

	Hci_StartProtectInquiryTimer(pHci, pHci->current_inquiry_length);

	RtlZeroMemory(DataStr, 12);

	AccessCode_Gen(InquiryPara, tempaccesscode);
	RtlCopyMemory(DataStr, tempaccesscode, 9);
	//Write inquiry length from IVT
	RtlCopyMemory(DataStr + 9, InquiryPara + 3, 1);
#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 10;
	offset = BT_REG_INQUIRY_ACCESS_CODE;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, DataStr);
	pEle = RegApi_Write_Cmd_Indicator(devExt, pEle, BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else	
	UsbWriteTo3DspRegs(devExt, BT_REG_INQUIRY_ACCESS_CODE, 3, DataStr);
	Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT);
#endif	
	Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_DSPACK, 2);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "clear inquiry result list \n");
	Hci_Clear_Inquiry_Result_List(pHci);
}
/**************************************************************************
 *   Hci_Response_Inquiry_Result
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Inquiry.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Inquiry_Result(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	PINQUIRY_RESULT_T pTempInquiryResult;
	pHci = (PBT_HCI_T)devExt->pHci;
	pTempInquiryResult = pHci->pcurrent_inquiry_result;
	*dest = 1;
	dest += sizeof(UINT8);
	if (pTempInquiryResult != NULL)
	{
		RtlCopyMemory(dest, pTempInquiryResult->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
		*dest = pTempInquiryResult->page_scan_repetition_mode;
		dest += sizeof(UINT8);
		*dest = pTempInquiryResult->page_scan_period_mode;
		dest += sizeof(UINT8);
		*dest = pTempInquiryResult->page_scan_mode;
		dest += sizeof(UINT8);
		RtlCopyMemory(dest, pTempInquiryResult->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
		dest += BT_CLASS_OF_DEVICE_LENGTH;
		*(PUINT16)dest = pTempInquiryResult->clock_offset;
		dest += sizeof(UINT16);
	}
	*pOutLen = 15;
}
/**************************************************************************
 *   Hci_Response_Inquiry_Result_with_Rssi
 *
 *   Descriptions:
 *      Send HC Inquiry_Result_with_Rssi event to respond to the HCI command Inquiry result.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Inquiry_Result_With_Rssi(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	PINQUIRY_RESULT_T pTempInquiryResult;
	pHci = (PBT_HCI_T)devExt->pHci;
	pTempInquiryResult = pHci->pcurrent_inquiry_result;
	*dest = 1;
	dest += sizeof(UINT8);
	if (pTempInquiryResult != NULL)
	{
		RtlCopyMemory(dest, pTempInquiryResult->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
		*dest = pTempInquiryResult->page_scan_repetition_mode;
		dest += sizeof(UINT8);
		*dest = 0;
		dest += sizeof(UINT8);
		RtlCopyMemory(dest, pTempInquiryResult->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
		dest += BT_CLASS_OF_DEVICE_LENGTH;
		*(PUINT16)dest = pTempInquiryResult->clock_offset;
		dest += sizeof(UINT16);
		*dest = 0;
		dest += sizeof(UINT8);
	}
	*pOutLen = 15;
}
/**************************************************************************
 *   Hci_Response_Inquiry_Complete
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Inquiry.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Inquiry_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Inquiry_Cancel
 *
 *   Descriptions:
 *      Process HCI command: Inquiry_Cancel
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Inquiry_Cancel(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Inquiry_Cancel Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_INQUIRY_CANCEL);

	if (pHci->is_in_inquiry_flag == 0
        || (pHci->sco_link_count > 0 && pHci->hv1_use_dv_instead_dh1_flag)
       )
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquriy cancel disallowed\n");
		pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		eventcode = BT_HCI_EVENT_COMMAND_COMPLETE;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		Task_Normal_Send_Event(devExt, eventcode, 1);
		return ;
	}

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
	Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_DSPACK, 2);
}
/**************************************************************************
 *   Hci_Response_Inquiry_Cancel
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Inquiry_Cancel.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Inquiry_Cancel(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_StartProtectInquiryTimer
 *
 *   Descriptions:
 *      Start a timer for protecting inquiry hci command
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      timercount: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartProtectInquiryTimer(PBT_HCI_T pHci, UINT8 timercount)
{
	UINT8 realtimercount = timercount;

	if (pHci == NULL)
		return ;

	if (realtimercount < 1)
		realtimercount = 1;
	if (realtimercount > 0x30)
		realtimercount = 0x30;

	pHci->protect_inquiry_timer_valid = 1;
	realtimercount = (realtimercount * 128 / 100) + 2;
	pHci->protect_inquiry_timer_count = realtimercount;

	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_StartProtectInquiryTimer, timecount = %d\n", pHci->protect_inquiry_timer_count);
}

/**************************************************************************
 *   Hci_StopProtectInquiryTimer
 *
 *   Descriptions:
 *      Stop the protect inquuiry timer that set before
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopProtectInquiryTimer(PBT_HCI_T pHci)
{
	if (pHci == NULL)
		return ;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_StopProtectInquiryTimer\n");

	pHci->protect_inquiry_timer_valid = 0;
	pHci->protect_inquiry_timer_count = 0;
}

/**************************************************************************
 *   Hci_ProtectInquiryTimeout
 *
 *   Descriptions:
 *      protect inquiry timer timeout function
 *   Arguments:
 *      devExt: IN, pointer to device extention.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ProtectInquiryTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL				oldIrql;
	PBT_HCI_T		pHci;

	if (devExt == NULL)
		return ;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
	if (pHci->protect_inquiry_timer_valid)
	{
		if (--pHci->protect_inquiry_timer_count <= 0)
		{
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_ProtectInquiryTimeout work!\n");
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
			
			BtProcessInquiryCompleteInt(devExt);
			Hci_StopProtectInquiryTimer(pHci);
			return;
		}
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
}

/**************************************************************************
 *   Hci_Command_Periodic_Inquiry_Mode
 *
 *   Descriptions:
 *      Process HCI command: Periodic Inquiry Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      InquiryPara: IN, the pointer to inquiry parameters
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 InquiryPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 tempaccesscode[9];
	UINT8 eventcode;

	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Periodic_Inquiry_Mode Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_PERIODIC_INQUIRY_MODE);
	}

	if (pHci->period_inquiry_flag)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "periodic inquiry already exists\n");
		pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		return;
	}

	if (pHci->is_in_inquiry_flag)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "An inquiry is in processing when perodic inquiy is occured\n");
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		return;
	}

	pHci->max_period_length = *(PUINT16)InquiryPara;
	pHci->min_period_length = *(PUINT16)(InquiryPara + 2);
	RtlCopyMemory(pHci->period_lap, InquiryPara + 4, BT_EACH_IAC_LAP_COUNT);
	pHci->period_inquiry_length = *(InquiryPara + 7);
	pHci->period_num_responses = *(InquiryPara + 8);

	if ((pHci->max_period_length < pHci->min_period_length) || (pHci->min_period_length < (UINT16)pHci->period_inquiry_length))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "periodic inquiry parameters error\n");
		pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		return;
	}

	pHci->period_inquiry_flag = 1;

	Hci_StartPeriodicInquiryTimer(pHci, pHci->max_period_length);

	if (pHci->sco_link_count > 0)
	{
		if (pHci->hv1_use_dv_instead_dh1_flag)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "An inquiry is ocuured when sco packet type is HV1, so disallowed\n");
			pHci->command_status = BT_HCI_STATUS_SUCCESS; 
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			return;
		}
	}

	if (pHci->only_allow_one_acl_link_flag)
	{
		if (pHci->num_device_am > 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "An inquiry is ocuured when only_allow_one_acl_link_flag is set, so disallowed\n");
			pHci->command_status = BT_HCI_STATUS_SUCCESS; 
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			return;
		}
	}	

	pHci->is_in_inquiry_flag = 1;

    //        InquiryPara, 3);

	pHci->current_inquiry_length = pHci->period_inquiry_length;
	pHci->current_inquiry_result_num = 0;
	pHci->current_num_responses = pHci->period_num_responses;

	if (pHci->inquiry_extra_flag)
		pHci->current_inquiry_length += pHci->period_inquiry_length; // doulbe for combo mode

	BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquiry length = %d, num responses = %d\n", pHci->current_inquiry_length, pHci->current_num_responses);

	Hci_StartProtectInquiryTimer(pHci, pHci->current_inquiry_length);

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	AccessCode_Gen(pHci->period_lap, tempaccesscode);

	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	pEle = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_LENGTH, pEle, 1, *(InquiryPara + 7));//pHci->period_inquiry_length);
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_ACCESS_CODE,
							 pEle, 9, tempaccesscode);
	pEle = RegApi_Write_Cmd_Indicator(devExt, pEle, BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT);

	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pEle - buf));

	Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_DSPACK, 2);
		
	Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "clear inquiry result list \n");
	Hci_Clear_Inquiry_Result_List(pHci);
}

/**************************************************************************
 *   Hci_Response_Periodic_Inquiry_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Periodic Inquiry Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Exit_Periodic_Inquiry_Mode
 *
 *   Descriptions:
 *      Process HCI command: Exit_Periodic_Inquiry_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Exit_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Exit_Periodic_Inquiry_Mode Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_EXIT_PERIODIC_INQUIRY_MODE);
	}

	if (pHci->period_inquiry_flag == 0)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "No periodic inquiry is to be exited\n");
		pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		return;
	}

	if (pHci->is_in_inquiry_flag)
	{
    	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		BT_DBGEXT(ZONE_HCI | LEVEL3, "inquiry is to be cancelled when exit periodic inquiry\n");
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
    	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		pHci->is_in_inquiry_flag = 0;
		Hci_StopProtectInquiryTimer(pHci);
	}

	pHci->period_inquiry_flag = 0;
	pHci->max_period_length = 0;
	pHci->min_period_length = 0;
	pHci->period_inquiry_length = 0;
	pHci->period_num_responses = 0;
	
	Hci_StopPeriodicInquiryTimer(pHci);

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt,BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Exit_Periodic_Inquiry_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Exit_Periodic_Inquiry_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Exit_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_StartPeriodicInquiryTimer
 *
 *   Descriptions:
 *      Start a timer for protecting inquiry hci command
 *   Arguments:
 *      pHci: IN, pointer to HCI module.
 *      timercount: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartPeriodicInquiryTimer(PBT_HCI_T pHci, UINT16 timercount)
{
	UINT16 realtimercount = timercount;

	if (pHci == NULL)
		return ;

	if (realtimercount < 3)
		realtimercount = 3;
	
	pHci->period_inquiry_timer_valid = 1;
	realtimercount = (realtimercount * 128 / 100) + 2;
	pHci->period_inquiry_timer_count = realtimercount;

	BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_StartPeriodicInquiryTimer, timecount = %d\n", pHci->period_inquiry_timer_count);
}

/**************************************************************************
 *   Hci_StopPeriodicInquiryTimer
 *
 *   Descriptions:
 *      Stop the periodic inquuiry timer that set before
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopPeriodicInquiryTimer(PBT_HCI_T pHci)
{
	if (pHci == NULL)
		return ;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_StopPeriodicInquiryTimer\n");

	pHci->period_inquiry_timer_valid = 0;
	pHci->period_inquiry_timer_count = 0;
}

/**************************************************************************
 *   Hci_PeriodicInquiryTimeout
 *
 *   Descriptions:
 *      periodic inquiry timer timeout function
 *   Arguments:
 *      devExt: IN, pointer to device extention.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_PeriodicInquiryTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL				oldIrql;
	PBT_HCI_T		pHci;

	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	
	if (devExt == NULL)
		return ;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
	if (pHci->period_inquiry_timer_valid)
	{
		if (--pHci->period_inquiry_timer_count <= 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_PeriodicInquiryTimeout work!\n");
			Hci_StopPeriodicInquiryTimer(pHci);
			if (pHci->period_inquiry_flag)
			{
				UINT8 tempaccesscode[9];

				Hci_StartPeriodicInquiryTimer(pHci, pHci->max_period_length);
				if (pHci->is_in_inquiry_flag)
				{
					BT_DBGEXT(ZONE_HCI | LEVEL1, "An inquiry is in processing when perodic inquiy is occured\n");
					KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
					return;
				}

				if (pHci->sco_link_count > 0)
				{
					if (pHci->hv1_use_dv_instead_dh1_flag)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL1, "An inquiry is ocuured when sco packet type is HV1, so ignore\n");
						KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
						return;
					}
				}

				if (pHci->only_allow_one_acl_link_flag)
				{
					if (pHci->num_device_am > 0)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL1, "An inquiry is ocuured when only_allow_one_acl_link_flag is set, so ignore\n");
						KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
						return;
					}
				}	

				pHci->is_in_inquiry_flag = 1;

				pHci->current_inquiry_length = pHci->period_inquiry_length;
				pHci->current_inquiry_result_num = 0;
				pHci->current_num_responses = pHci->period_num_responses;

				if (pHci->inquiry_extra_flag)
					pHci->current_inquiry_length += pHci->period_inquiry_length; // doulbe for combo mode

				BT_DBGEXT(ZONE_HCI | LEVEL3, "Inquiry length = %d, num responses = %d\n", pHci->current_inquiry_length, pHci->current_num_responses);

				Hci_StartProtectInquiryTimer(pHci, pHci->current_inquiry_length);

				KeReleaseSpinLock ( &pHci->HciLock, oldIrql );


				AccessCode_Gen(pHci->period_lap, tempaccesscode);



				pEle = BTUSB_REGAPI_FIRST_ELE(buf);
				pEle = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_LENGTH, pEle, 1, pHci->period_inquiry_length);
				pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_ACCESS_CODE,
										 pEle, 9, tempaccesscode);
				pEle = RegApi_Write_Cmd_Indicator(devExt, pEle, BT_HCI_COMMAND_INDICATOR_INQUIRY_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pEle - buf));

				BT_DBGEXT(ZONE_HCI | LEVEL3, "clear inquiry result list \n");
				Hci_Clear_Inquiry_Result_List(pHci);

				KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
			}
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			return;
		}
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
}

static void DumpAccessCode(UINT8 *pac)
{
    BT_DBGEXT(ZONE_HCI | LEVEL0, "Access: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                                 pac[0],
                                 pac[1],
                                 pac[2],
                                 pac[3],
                                 pac[4],
                                 pac[5],
                                 pac[6],
                                 pac[7],
                                 pac[8]);

}

static void DumpFHS(FHS_PACKET_T *pFhsPkt)
{
   BT_DBGEXT(ZONE_HCI | LEVEL0, "FHS Packet:\n") ;
   BT_DBGEXT(ZONE_HCI | LEVEL0, "Parity: 0x%08x\n"
                                "LAP: 0x%06x\n"
                                "Undef: 0x%02x\n"
                                "sr: 0x%02x\n"
                                "sp: 0x%02x\n"
                                "UAP: 0x%02x\n"
                                "NAP: 0x%04x\n"
                                "ltAddr: 0x%02x\n"
                                "ClockOFF: 0x%08x\n"
                                "PSM: 0x%02x\n"
                                "Class of Dev: 0x%02x%02x%02x\n",
                                (unsigned int)pFhsPkt->parity_bits,
                                pFhsPkt->lap,
                                pFhsPkt->undefined,
                                pFhsPkt->sr,
                                pFhsPkt->sp,
                                pFhsPkt->uap,
                                pFhsPkt->nap,
                                pFhsPkt->am_addr,
                                pFhsPkt->clk27_2,
                                pFhsPkt->page_scan_mode,
                                pFhsPkt->class_of_device[0], pFhsPkt->class_of_device[1], pFhsPkt->class_of_device[2]);
}
/**************************************************************************
 *   Hci_Command_Create_Connection
 *
 *   Descriptions:
 *      Process HCI command: Inquiry
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnPara: IN, the pointer to create connection parameters
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Create_Connection(PBT_DEVICE_EXT devExt, PUINT8 ConnPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	FHS_PACKET_T fhs_packet;
	PUINT64 ptmplonglong;
	UINT8 tempaccesscode[9];
	UINT8 DataStr[12];
	NTSTATUS status;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pConnectDevice;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif
#ifdef BT_MATCH_CLASS_OF_DEVICE
	PINQUIRY_RESULT_T pTempInquiryResult;
#endif

	ENTER_FUNC();

	pHci = (PBT_HCI_T)devExt->pHci;   
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS; // temp
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_CREATE_CONNECTION);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	if (Hci_Find_Connect_Device_By_BDAddr(pHci, ConnPara) || Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, ConnPara))
	{
		pHci->command_status = BT_HCI_ERROR_ACL_CONNECTIONS_EXISTS;
		eventcode = BT_HCI_EVENT_COMMAND_STATUS;
		Task_Normal_Send_Event(devExt, eventcode, 1);
		return ;
	}

	#ifdef BT_COMBO_SNIFF_SUPPORT
	if(pHci->lmp_features.byte0.sniff_mode == 1)
	{
		for(i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "sniff index is: %d\n", i);
			pConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice;
			if(pConnectDevice != NULL)
			{
				status = Send_UnsniffReq_PDU(devExt, pConnectDevice);	
			}
			
		}	
	}
	#endif
    
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->sco_link_count > 0
        || (pHci->only_allow_one_acl_link_flag && pHci->num_device_am > 0)
        || (pHci->slave_sco_master_not_coexist_flag && LMP_CheckSlaveSCO(devExt) == 1)
       )
	{
		if (pHci->hv1_use_dv_instead_dh1_flag)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "create connection but sco packet type is hv1, so disallowed\n");
			pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
			eventcode = BT_HCI_EVENT_COMMAND_STATUS;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_Normal_Send_Event(devExt, eventcode, 1);
			return ;
		}
	}
  	

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    BT_DBGEXT(ZONE_HCI | LEVEL1, "Clock OFF 0x%x\n", *(PUINT16)(ConnPara + 10));
	status = Hci_Add_Connect_Device(devExt, ConnPara, *(PUINT16)(ConnPara + 6), *(ConnPara + 8), *(ConnPara + 9), *(PUINT16)(ConnPara + 10), *(ConnPara + 12));

	if (status == STATUS_SUCCESS)
	{
        //Workaround the periodic inquiry issue before making connection in Bluez4.30
        if(pHci->period_inquiry_flag != 0 || pHci->is_in_inquiry_flag != 0)
        {
            BT_DBGEXT(ZONE_HCI | LEVEL1, "Workaround: Cancel Inquiry before creating connection\n");
        	Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
            BtProcessInquiryCompleteInt(devExt);
        }
    
    #ifdef BT_MATCH_CLASS_OF_DEVICE
    	// Match property class of device from inquiry result. We can not get its class of device from other location.
    	pTempInquiryResult = Hci_Find_Inquiry_Result_By_BDAddr(pHci, ConnPara);
        //	sc_spin_lock_bh(&pHci->HciLock);
    	if (pTempInquiryResult)
    	{
    		RtlCopyMemory(pHci->pcurrent_connect_device->class_of_device, pTempInquiryResult->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
    	}
    #else
        //	sc_spin_lock_bh(&pHci->HciLock);
    #endif
		pHci->command_status = BT_HCI_STATUS_SUCCESS; // temp
		Hci_Write_Data_To_EightBytes(devExt, BT_REG_PAGED_BD_ADDR, ConnPara, 6);
		RtlZeroMemory(DataStr, 12);
		AccessCode_Gen(ConnPara, tempaccesscode);
        DumpAccessCode(tempaccesscode);
		Hci_Write_Page_Access_Code(devExt, tempaccesscode);
		AccessCode_Gen(pHci->local_bd_addr, tempaccesscode);
        DumpAccessCode(tempaccesscode);
		BT_DBGEXT(ZONE_HCI | LEVEL3, "The length of FHS_PACKET_T = %d\n", sizeof(FHS_PACKET_T));
		ptmplonglong = (PUINT64)tempaccesscode;
		*ptmplonglong =  *ptmplonglong >> 4;
		RtlCopyMemory(&fhs_packet, ptmplonglong, 8);
		fhs_packet.undefined = 0;
		fhs_packet.sr = *(ConnPara + 8);
		#ifdef BT_ENHANCED_RATE
		fhs_packet.sp = 2; // This is reserved filed for EDR and it should be set as "10" (2). 
		#else
		fhs_packet.sp = *(ConnPara + 9);
		#endif
		fhs_packet.uap = pHci->local_bd_addr[3];
		fhs_packet.nap = *(PUINT16)(&pHci->local_bd_addr[4]);
		RtlCopyMemory(fhs_packet.class_of_device, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
        fhs_packet.class_of_device[2] = 0x12;
		fhs_packet.am_addr = pHci->pcurrent_connect_device->am_addr;
		fhs_packet.page_scan_mode = 0;
        fhs_packet.clk27_2 = pHci->pcurrent_connect_device->clock_offset;
		DumpFHS(&fhs_packet);
		UsbWriteTo3DspRegs(devExt, BT_REG_FHS_FOR_PAGE, sizeof(FHS_PACKET_T) / 4, &fhs_packet);

		RtlZeroMemory(DataStr, 12);
		RtlCopyMemory(DataStr, pHci->pcurrent_connect_device->bd_addr, 6);
		RtlCopyMemory(DataStr + 6, (PUINT8) &pHci->pcurrent_connect_device->clock_offset, 2);

	#ifdef DSP_REG_WRITE_API
		pEle = BTUSB_REGAPI_FIRST_ELE(buf);
		length = 1;
		offset = BT_REG_AM_ADDR + pHci->pcurrent_connect_device->am_addr;
		pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &pHci->pcurrent_connect_device->am_addr);
		offset = BT_REG_CURRENT_PAGE_SCAN_REP_MODE;
		length =  1;
		pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &pHci->pcurrent_connect_device->page_scan_repetition_mode);
		offset = BT_REG_CURRENT_PROCESS_AM_ADDR;
		length =  1;
		pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &pHci->pcurrent_connect_device->am_addr);
		pHci->am_connection_indicator |= (UINT16)(0x1 << pHci->pcurrent_connect_device->am_addr);
        offset = BT_REG_AM_CONNECTION_INDICATOR;
        length =  2;
        pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8)&pHci->am_connection_indicator);
        //local bd addr
		offset = BT_REG_AM_BD_ADDR + pHci->pcurrent_connect_device->am_addr *(BT_BD_ADDR_LENGTH + BT_CLOCK_OFFSET_LENGTH);
		length =  8;
		pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, DataStr);
		pEle = RegApi_Write_Cmd_Indicator(devExt, pEle, BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT);
		length = (UINT16)(pEle - buf);
		RegAPI_SendTo_MailBox(devExt, buf, length);
	#else
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Clock offset: %x\n", pHci->pcurrent_connect_device->clock_offset);
		Hci_Write_AM_Addr(devExt, pHci->pcurrent_connect_device->am_addr, pHci->pcurrent_connect_device->am_addr);
		UsbWriteTo3DspReg(devExt, BT_REG_CURRENT_PAGE_SCAN_REP_MODE, pHci->pcurrent_connect_device->page_scan_repetition_mode);
		Hci_Write_Byte_In_FourByte(devExt, BT_REG_CURRENT_PROCESS_AM_ADDR, 0, pHci->pcurrent_connect_device->am_addr);

		UsbWriteTo3DspRegs(devExt, BT_REG_AM_BD_ADDR + pHci->pcurrent_connect_device->am_addr *(BT_BD_ADDR_LENGTH + BT_CLOCK_OFFSET_LENGTH), 2, DataStr);
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT);
	#endif		
		Hci_StartTimer(pHci->pcurrent_connect_device, BT_TIMER_TYPE_PAGE_TIMEOUT, (UINT16)(((UINT32)pHci->page_timeout * 625) / (1000 * 1000)  + 3)); 
		pHci->pcurrent_connect_device->state = BT_DEVICE_STATE_PAGING;
		if (pHci->sco_link_count > 0)
		{
			pHci->pcurrent_connect_device->max_slot = BT_MAX_SLOT_1_SLOT;
		}
		Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_DSPACK, 2);
	}
	else
	{
		pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
 		// We create a task to send HC command_complete event. In this event, there are some parameters.
 	}
	eventcode = BT_HCI_EVENT_COMMAND_STATUS;
	Task_Normal_Send_Event(devExt, eventcode, 1);
 }
/**************************************************************************
 *   Hci_Response_Connection_Complete
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command create connection.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Connection_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
		*dest = pHci->pcurrent_connect_device->link_type;
		dest += sizeof(UINT8);
		*dest = pHci->pcurrent_connect_device->encryption_mode;
		dest += sizeof(UINT8);
	}
	*pOutLen = 11;
}
/**************************************************************************
 *   Hci_Command_Write_Link_Policy_Settings
 *
 *   Descriptions:
 *      Process HCI command: Write_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *      LinkPolicySettings: IN, Link_Policy_Settings
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 LinkPolicySettings)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_WRITE_LINK_POLICY_SETTINGS);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->link_policy_settings = LinkPolicySettings;
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->link_policy_settings = LinkPolicySettings;
		}
	}
	if (pTempConnectDevice != NULL)
		BT_DBGEXT(ZONE_HCI | LEVEL3, "link_policy_settings= 0x%x, BD0= 0x%x!\n", pTempConnectDevice->link_policy_settings, pTempConnectDevice->bd_addr[0]);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Link_Policy_Settings
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Link_Policy_Settings.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Read_Link_Policy_Settings
 *
 *   Descriptions:
 *      Process HCI command: Read_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
    
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_READ_LINK_POLICY_SETTINGS);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Link_Policy_Settings
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Link_Policy_Settings.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*(PUINT16)dest = pHci->pcurrent_connect_device->link_policy_settings;
		dest += sizeof(UINT16);
	}
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Write_Link_Supervision_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Write_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *      LinkSupervisionTimeout: IN, Link_Supervision_Timeout
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 LinkSupervisionTimeout)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_LINK_SUPERVISION_TIMEOUT);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	// Change the hci command state
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->real_link_supervision_timeout = LinkSupervisionTimeout;
		if (LinkSupervisionTimeout == 0)
		{
			pTempConnectDevice->link_supervision_timeout = 0xffff;
		}
		else
		{
			if (LinkSupervisionTimeout < 8000) // The min value driver supports is 8000, 5 seconds
				pTempConnectDevice->link_supervision_timeout = 8000;
			else
				pTempConnectDevice->link_supervision_timeout = LinkSupervisionTimeout;
		}
		
		Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)(pTempConnectDevice->link_supervision_timeout) * 625) / (1000 * 1000)  + 1)); 
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice != NULL)
		{
			pTempConnectDevice->real_link_supervision_timeout = LinkSupervisionTimeout;
			if (LinkSupervisionTimeout == 0)
			{
				pTempConnectDevice->link_supervision_timeout = 0xffff;
			}
			else
			{
				if (LinkSupervisionTimeout < 8000) // The min value driver supports is 8000, 5 seconds
					pTempConnectDevice->link_supervision_timeout = 8000;
				else
					pTempConnectDevice->link_supervision_timeout = LinkSupervisionTimeout;
			}

			Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)(pTempConnectDevice->link_supervision_timeout) * 625) / (1000 * 1000)  + 1)); 
		}
		else
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_LINK_SUPERVISION_TIMEOUT);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}

/**************************************************************************
 *   Hci_Response_Write_Link_Supervision_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Link_Supervision_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Read_Clock_Offset
 *
 *   Descriptions:
 *      Process HCI command: Read_Clock_Offset
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Clock_Offset(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Clock_Offset Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_READ_CLOCK_OFFSET);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	/* BlueZ ONLY, delay it to the moment of complete event */
    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
    	// Send event to LMP module
    	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CLK_OFFSET_REQ);
    }
}
/**************************************************************************
 *   Hci_Response_Read_Clock_Offset
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Clock_Offset.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Clock_Offset(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*(PUINT16)dest = pHci->pcurrent_connect_device->clock_offset;
		dest += sizeof(UINT16);
	}
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Remote_Name_Request
 *
 *   Descriptions:
 *      Process HCI command: Remote_Name_Request
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pRemoteNamePara: IN, the pointer to parameters of this command
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Remote_Name_Request(PBT_DEVICE_EXT devExt, PUINT8 pRemoteNamePara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	NTSTATUS status;
	PCONNECT_DEVICE_T pTempConnectDevice;
	FHS_PACKET_T fhs_packet;
	PUINT64 ptmplonglong;
	UINT8 tempaccesscode[9];
	UINT8 Datastr[12];
#ifdef DSP_REG_WRITE_API
	UINT8		buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#endif

	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Remote_Name_Request Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REMOTE_NAME_REQUEST);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pRemoteNamePara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pRemoteNamePara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			if ((pHci->sco_link_count > 0 && pHci->hv1_use_dv_instead_dh1_flag)
                || (pHci->only_allow_one_acl_link_flag && pHci->num_device_am > 0)
                || (pHci->slave_sco_master_not_coexist_flag && LMP_CheckSlaveSCO(devExt) == 1)
               )
			{
				BT_DBGEXT(ZONE_HCI | LEVEL1, "An remote name request is ocuured when sco packet type is HV1, so disallowed\n");
				pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
			}
            else
			{
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				status = Hci_Add_Connect_Device(devExt, pRemoteNamePara, BT_ACL_PACKET_DM1, *(pRemoteNamePara + 6), *(pRemoteNamePara + 7), *(PUINT16)(pRemoteNamePara + 8), 0);
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				if (status != STATUS_SUCCESS)
				{
					pHci->pcurrent_connect_device = NULL;
					pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
				}
				else
				{
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					pHci->pcurrent_connect_device->tempflag = 1;
					pHci->pcurrent_connect_device->is_in_remote_name_req_flag = 1;
				#ifdef DSP_REG_WRITE_API
					pEle = BTUSB_REGAPI_FIRST_ELE(buf);
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PAGED_BD_ADDR, pEle, 6, pRemoteNamePara);
				#else	
					Hci_Write_Data_To_EightBytes(devExt, BT_REG_PAGED_BD_ADDR, pRemoteNamePara, 6);
				#endif
					AccessCode_Gen(pRemoteNamePara, tempaccesscode);

				#ifdef DSP_REG_WRITE_API
					length = 9;
					offset = BT_REG_PAGE_ACCESS_CODE;
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, tempaccesscode);
					length = (UINT16)(pEle - buf);
					RegAPI_SendTo_MailBox(devExt, buf, length);
				#else
					Hci_Write_Page_Access_Code(devExt, tempaccesscode);
				#endif

					AccessCode_Gen(pHci->local_bd_addr, tempaccesscode);

					
					BT_DBGEXT(ZONE_HCI | LEVEL3, "The length of FHS_PACKET_T = %d\n", sizeof(FHS_PACKET_T));
					ptmplonglong = (PUINT64)tempaccesscode;
					*ptmplonglong =  *ptmplonglong >> 4;
					RtlCopyMemory(&fhs_packet, ptmplonglong, 8);
					fhs_packet.undefined = 0;
					fhs_packet.sr = 0;
					#ifdef BT_ENHANCED_RATE
					fhs_packet.sp = 2; // This is reserved filed for EDR and it should be set as "10" (2). 
					#else
					fhs_packet.sp = 0;
					#endif
					fhs_packet.uap = pHci->local_bd_addr[3];
					fhs_packet.nap = *(PUINT16)(&pHci->local_bd_addr[4]);
					RtlCopyMemory(fhs_packet.class_of_device, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
					fhs_packet.am_addr = pHci->pcurrent_connect_device->am_addr;
					fhs_packet.page_scan_mode = 0;
					UsbWriteTo3DspRegs(devExt, BT_REG_FHS_FOR_PAGE, sizeof(FHS_PACKET_T) / 4,  &fhs_packet);
					
					RtlZeroMemory(Datastr, 12);
					RtlCopyMemory(Datastr, pHci->pcurrent_connect_device->bd_addr, 6);
					RtlCopyMemory(Datastr + 6, (PUINT8) &pHci->pcurrent_connect_device->clock_offset, 2);

					
				#ifdef DSP_REG_WRITE_API
					pEle = BTUSB_REGAPI_FIRST_ELE(buf);
					length = 1;
					offset = BT_REG_AM_ADDR + pHci->pcurrent_connect_device->am_addr;
					tmpChar = pHci->pcurrent_connect_device->am_addr;
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &tmpChar);
					offset = BT_REG_CURRENT_PAGE_SCAN_REP_MODE;
					length = 1;
					tmpChar = pHci->pcurrent_connect_device->page_scan_repetition_mode;
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &tmpChar);
					offset = BT_REG_CURRENT_PROCESS_AM_ADDR;
					length = 1;
					tmpChar = pHci->pcurrent_connect_device->am_addr;
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &tmpChar);
            		//Jakio20090318: write am connection indicator here
            		pHci->am_connection_indicator |= (UINT16)(0x1 << pHci->pcurrent_connect_device->am_addr);
                    offset = BT_REG_AM_CONNECTION_INDICATOR;
                    length =  2;
                    pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, (PUINT8)&pHci->am_connection_indicator);

					offset = BT_REG_AM_BD_ADDR + pHci->pcurrent_connect_device->am_addr *(BT_BD_ADDR_LENGTH + BT_CLOCK_OFFSET_LENGTH);
					length = 8;
					pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, Datastr);
					length = (UINT16)(pEle - buf);
					RegAPI_SendTo_MailBox(devExt, buf, length);

					
					pEle = BTUSB_REGAPI_FIRST_ELE(buf);
					pEle = RegApi_Write_Cmd_Indicator(devExt, pEle, BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT);
					length = (UINT16)(pEle - buf);
					RegAPI_SendTo_MailBox(devExt, buf, length);
				#else
					Hci_Write_AM_Addr(devExt, pHci->pcurrent_connect_device->am_addr, pHci->pcurrent_connect_device->am_addr);
					UsbWriteTo3DspReg(devExt, BT_REG_CURRENT_PAGE_SCAN_REP_MODE, pHci->pcurrent_connect_device->page_scan_repetition_mode);
					Hci_Write_Byte_In_FourByte(devExt, BT_REG_CURRENT_PROCESS_AM_ADDR, 0, pHci->pcurrent_connect_device->am_addr);
					UsbWriteTo3DspRegs(devExt, BT_REG_AM_BD_ADDR + pHci->pcurrent_connect_device->am_addr *(BT_BD_ADDR_LENGTH + BT_CLOCK_OFFSET_LENGTH), 2, Datastr);
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_CREATE_CONNECTION_BIT);
				#endif
					Hci_StartTimer(pHci->pcurrent_connect_device, BT_TIMER_TYPE_PAGE_TIMEOUT, (UINT16)(((UINT32)pHci->page_timeout *625) / (1000 *1000) + 1));
					pHci->pcurrent_connect_device->state = BT_DEVICE_STATE_PAGING;
					if (pHci->sco_link_count > 0)
					{
						pHci->pcurrent_connect_device->max_slot = BT_MAX_SLOT_1_SLOT;
					}
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				}
			}
		}
		else
		{
			if (pTempConnectDevice->is_in_remote_name_req_flag)
			{
				BT_DBGEXT(ZONE_HCI | LEVEL1, "A remote name request is in processing...\n");
				pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
			}
			else
			{
				pTempConnectDevice->is_in_remote_name_req_flag = 1;
				pHci->pcurrent_connect_device = pTempConnectDevice;
				pHci->current_connection_handle = pTempConnectDevice->connection_handle;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_NAME_REQ);
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
		}
	}
	else
	{
		if (pTempConnectDevice->is_in_remote_name_req_flag)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "A remote name request is in processing...\n");
			pHci->command_status = BT_HCI_ERROR_COMMAND_DISALLOWED;
		}
		else
		{
			pTempConnectDevice->is_in_remote_name_req_flag = 1;
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_NAME_REQ);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_Response_Remote_Name_Request
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Remote_Name_Request.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Remote_Name_Request(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->remote_name, BT_LOCAL_NAME_LENGTH);
		dest += BT_LOCAL_NAME_LENGTH;
	}
	*pOutLen = 255;
}
/**************************************************************************
 *   Hci_Command_Remote_Name_Request_Cancel
 *
 *   Descriptions:
 *      Process HCI command: Remote_Name_Request_Cancel
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pRemoteNamePara: IN, the pointer to parameters of this command
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Remote_Name_Request_Cancel(PBT_DEVICE_EXT devExt, PUINT8 pRemoteNamePara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	NTSTATUS status;
	PCONNECT_DEVICE_T pTempConnectDevice;
	FHS_PACKET_T fhs_packet;
	PUINT64 ptmplonglong;
	UINT8 tempaccesscode[9];
	LARGE_INTEGER timevalue;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Remote_Name_Request_Cancel Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REMOTE_NAME_REQUEST_CANCEL);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pRemoteNamePara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pRemoteNamePara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->pcurrent_connect_device = NULL;
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
		else
		{
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
			if (pTempConnectDevice->is_in_remote_name_req_flag == 0)
			{
				BT_DBGEXT(ZONE_HCI | LEVEL3, "No remote name request is to be cancelled\n");
				pHci->command_status = BT_HCI_ERROR_INVALID_HCI_COMMAND_PARAMETERS;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);

				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			}
			else
			{
				pTempConnectDevice->is_in_remote_name_req_flag = 0;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);

                /* BlueZ ONLY, delayed to command complete */

				pTempConnectDevice->status = BT_HCI_ERROR_NO_CONNECTION;

				Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
				if (pTempConnectDevice->tempflag == 1)
				{
					Task_Normal_Rel_Resouce_For_Remote_Req(devExt, (PUINT8) &pTempConnectDevice);
				}
				KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

				timevalue.QuadPart = 0; // Just raise the IRQL and process the task immudiatly in another DPC process.
				tasklet_schedule(&devExt->taskletRcv);
			}
		}
	}
	else
	{
		pHci->pcurrent_connect_device = pTempConnectDevice;
		pHci->current_connection_handle = pTempConnectDevice->connection_handle;
		if (pTempConnectDevice->is_in_remote_name_req_flag == 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "No remote name request is to be cancelled\n");
			pHci->command_status = BT_HCI_ERROR_INVALID_HCI_COMMAND_PARAMETERS;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
		else
		{
			pTempConnectDevice->is_in_remote_name_req_flag = 0;
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			pTempConnectDevice->status = BT_HCI_ERROR_NO_CONNECTION;
			Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
			if (pTempConnectDevice->tempflag == 1)
			{
				Task_Normal_Rel_Resouce_For_Remote_Req(devExt, (PUINT8) &pTempConnectDevice);
			}
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			//Jakio20070724: set hci timer
			timevalue.QuadPart = 0; // Just raise the IRQL and process the task immudiatly in another DPC process.
			tasklet_schedule(&devExt->taskletRcv);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_Response_Remote_Name_Request_Cancel
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Remote_Name_Request_Cancel.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Remote_Name_Request_Cancel(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
	}
	*pOutLen = 7;
}
/**************************************************************************
 *   Hci_Command_Role_Discovery
 *
 *   Descriptions:
 *      Process HCI command: Role_Discovery
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Role_Discovery(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
    
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_ROLE_DISCOVERY);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Role_Discovery
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Role_Discovery.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Role_Discovery(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*dest = pHci->pcurrent_connect_device->current_role;
		dest += sizeof(UINT8);
	}
	*pOutLen = 4;
}
/**************************************************************************
 *   Hci_Command_Disconnect
 *
 *   Descriptions:
 *      Process HCI command: Disconnect
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *	    Reason: IN, Reason
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Disconnect(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 Reason)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 link_type = BT_LINK_TYPE_ACL;

    ENTER_FUNC();
    // Get pointer of hci module
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_reason_code = Reason;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_DISCONNECT);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);
	if (pTempConnectDevice == NULL)
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);
        
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
            /* Fail status event */
            Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
		}
		else
		{
			if (link_type == BT_LINK_TYPE_ACL)
			{
				pTempConnectDevice->current_reason_code = Reason;
				pTempConnectDevice->state = BT_DEVICE_STATE_DISCONNECT;
			}
			else
			{
				ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);
				((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->current_reason_code = Reason;
			}
		}
	}
	else
	{
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (link_type == BT_LINK_TYPE_ACL)
		{
			pTempConnectDevice->current_reason_code = Reason;
			pTempConnectDevice->state = BT_DEVICE_STATE_DISCONNECT;
		}
		else
		{
			ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);
			((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->current_reason_code = Reason;
		}
    	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	}
    
	if (link_type == BT_LINK_TYPE_ACL)
	{
		/* BlueZ ONLY, delay it */
		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_DETACH);
	}
	else
	{
		if (pTempConnectDevice != NULL)
		{
		    /* BlueZ ONLY, delay it */
			
			UINT32 cmd;
			if (pTempConnectDevice->pScoConnectDevice != NULL)
			{
				if (((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->link_type == BT_LINK_TYPE_ESCO)
				{
				    cmd = BT_TASK_EVENT_HCI2LMP_REMOVE_ESCO;
				}
				else
				{
					cmd = BT_TASK_EVENT_HCI2LMP_REMOVE_SCO;
				}
			}
			else
			{
				cmd = BT_TASK_EVENT_HCI2LMP_REMOVE_SCO;
			}

			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, cmd);
		}
	}

}
/**************************************************************************
 *   Hci_Response_Disconnect_Complete
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Disconnect.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Disconnect_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	*dest = pHci->current_reason_code;
	dest += sizeof(UINT8);
	*pOutLen = 4;
}
/**************************************************************************
 *   Hci_Event_Connection_Request
 *
 *   Descriptions:
 *      Send HC Connection_Request event .
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Event_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	pTempConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	RtlCopyMemory(dest, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	dest += BT_BD_ADDR_LENGTH;
	RtlCopyMemory(dest, pTempConnectDevice->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	dest += BT_CLASS_OF_DEVICE_LENGTH;
	*dest = pTempConnectDevice->link_type;
	dest += sizeof(UINT8);
	*pOutLen = 10;
}
/**************************************************************************
 *   Hci_Command_Accept_Connection_Request
 *
 *   Descriptions:
 *      Process HCI command: Accept_Connection_Request
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pAcceptConnPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Accept_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 pAcceptConnPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 need_role_switch = 0;
	UINT8 DataStr[8];

    ENTER_FUNC();    
	pHci = (PBT_HCI_T)devExt->pHci;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_ACCEPT_CONNECTION_REQUEST);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pAcceptConnPara);
	if (pTempConnectDevice == NULL)
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pAcceptConnPara);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->role_switching_role = *(pAcceptConnPara + 6);
			if (pTempConnectDevice->acl_or_sco_flag == BT_ACL_IN_USE)
			{
				if (pTempConnectDevice->role_switching_role == BT_ROLE_SLAVE)
				{
					pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
					Hci_StopTimer(pTempConnectDevice);
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				}
				else
				{
					need_role_switch = 1;
					pTempConnectDevice->state = BT_DEVICE_STATE_WAIT_ROLE_SWITCH;
					Hci_StopTimer(pTempConnectDevice);
					Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
					pTempConnectDevice->role_switching_flag = 1;
					pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
					Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
					Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE, 10);
				}
			}
			else
			{
				ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);

				Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.
				
				((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->packet_type = (BT_PACKET_TYPE_HV1 | BT_PACKET_TYPE_HV2 | BT_PACKET_TYPE_HV3);
				((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->esco_packet_type = (BT_PACKET_TYPE_ESCO_HV1 | BT_PACKET_TYPE_ESCO_HV2 | BT_PACKET_TYPE_ESCO_HV3);
			}
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
		}
	}
	else
	{
		pTempConnectDevice->role_switching_role = *(pAcceptConnPara + 6);
		if (pTempConnectDevice->acl_or_sco_flag == BT_ACL_IN_USE)
		{
			if (pTempConnectDevice->role_switching_role == BT_ROLE_SLAVE)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_ACCEPTED;
				Hci_StopTimer(pTempConnectDevice);
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
			}
			else
			{
				need_role_switch = 1;
				pTempConnectDevice->state = BT_DEVICE_STATE_WAIT_ROLE_SWITCH;
				Hci_StopTimer(pTempConnectDevice);
				Hci_StartTimer(pTempConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
				pTempConnectDevice->role_switching_flag = 1;
				pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
				Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
				Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE, 10);
			}
		}

		else
		{
			ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);

			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.
			
			((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->packet_type = (BT_PACKET_TYPE_HV1 | BT_PACKET_TYPE_HV2 | BT_PACKET_TYPE_HV3);
			((PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice)->esco_packet_type = (BT_PACKET_TYPE_ESCO_HV1 | BT_PACKET_TYPE_ESCO_HV2 | BT_PACKET_TYPE_ESCO_HV3);
		}
		pHci->pcurrent_connect_device = pTempConnectDevice;
		pHci->current_connection_handle = pTempConnectDevice->connection_handle;
	}
	if (!need_role_switch)
	{
		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN);
	}

    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
}


/**************************************************************************
 *   Hci_Command_Reject_Connection_Request
 *
 *   Descriptions:
 *      Process HCI command: Accept_Connection_Request
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pRejectConnPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Reject_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 pRejectConnPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
    
    ENTER_FUNC();
    
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REJECT_CONNECTION_REQUEST);


	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pRejectConnPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pRejectConnPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			if (pTempConnectDevice->state == BT_ACL_IN_USE)
			{
				pTempConnectDevice->state = BT_DEVICE_STATE_NOT_ACCEPTED;
			}
			pTempConnectDevice->current_reason_code = *(pRejectConnPara + 6);
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
		}
	}
	else
	{
		if (pTempConnectDevice->state == BT_ACL_IN_USE)
		{
			pTempConnectDevice->state = BT_DEVICE_STATE_NOT_ACCEPTED;
		}
		pTempConnectDevice->current_reason_code = *(pRejectConnPara + 6);
		pHci->pcurrent_connect_device = pTempConnectDevice;
		pHci->current_connection_handle = pTempConnectDevice->connection_handle;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
    	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_CONN);
    }

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Read_Link_Supervision_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Read_Link_Supervision_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_LINK_SUPERVISION_TIMEOUT);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    ENTER_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Link_Supervision_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Link_Supervision_Timeout.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*(PUINT16)dest = pHci->pcurrent_connect_device->link_supervision_timeout;
		dest += sizeof(UINT16);
	}
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Read_Remote_Supported_Features
 *
 *   Descriptions:
 *      Process HCI command: Read_Remote_Supported_Features
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Remote_Supported_Features(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Remote_Supported_Features Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REMOTE_SUPPORTED_FEATURES);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
	    LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_FEATURE_REQ);
    }
}


/**************************************************************************
 *   Hci_Command_Read_Remote_Extended_Features
 *
 *   Descriptions:
 *      Process HCI command: Read_Remote_Supported_Features
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle:  IN, Connection_Handle.
 *      page_number: IN, feature page number.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Remote_Extended_Features(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 page_number)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Remote_Extended_Features Enter!\n");
	
	// Change the hci command state
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return;
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REMOTE_EXTENDED_FEATURES);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	
	pHci->pcurrent_connect_device = pTempConnectDevice;
	if (pTempConnectDevice != NULL)
		pTempConnectDevice->current_page_number = page_number;
	
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
	Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_EXTENDED_FEATURE_REQ), BT_TASK_PRI_NORMAL, (PUINT8)&pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return;
}

/**************************************************************************
 *   Hci_Command_Read_Remote_Version_Info
 *
 *   Descriptions:
 *      Process HCI command: Read_Remote_Supported_Features
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Remote_Version_Info(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_READ_REMOTE_VERSION_INFORMATION);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
    	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_VERSION_REQ);
    }

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Link_Keq_Request_Reply
 *
 *   Descriptions:
 *      Process HCI command: Link_Keq_Request_Reply
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pLinkKeyPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Link_Keq_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 pLinkKeyPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Link_Keq_Request_Reply Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_LINK_KEY_REQUEST_REPLY);


	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pLinkKeyPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pLinkKeyPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			RtlCopyMemory(pTempConnectDevice->link_key, pLinkKeyPara + 6, BT_LINK_KEY_LENGTH);
		}
	}
	else
	{
		RtlCopyMemory(pTempConnectDevice->link_key, pLinkKeyPara + 6, BT_LINK_KEY_LENGTH);
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_LINK_KEY_REPLY);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Link_Keq_Request_Reply
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Link_Keq_Request_Reply.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Link_Keq_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device != NULL)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		// Move the destination buffer pointer to next position.
		dest += BT_BD_ADDR_LENGTH;
	}
	*pOutLen = 7;
}
/**************************************************************************
 *   Hci_Command_Link_Keq_Request_Negative_Reply
 *
 *   Descriptions:
 *      Process HCI command: Link_Keq_Request_Negative_Reply
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pLinkKeyNegPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Link_Keq_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 pLinkKeyNegPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Link_Keq_Request_Negative_Reply Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_LINK_KEY_REQUEST_NEGATIVE_REPLY);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pLinkKeyNegPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pLinkKeyNegPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			RtlZeroMemory(pTempConnectDevice->link_key, BT_LINK_KEY_LENGTH);
		}
	}
	else
	{
		RtlZeroMemory(pTempConnectDevice->link_key, BT_LINK_KEY_LENGTH);
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_LINK_KEY_REPLY);
}
/**************************************************************************
 *   Hci_Response_Link_Keq_Request_Negative_Reply
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Link_Keq_Request_Negative_Reply.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Link_Keq_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device != NULL)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		// Move the destination buffer pointer to next position.
		dest += BT_BD_ADDR_LENGTH;
	}
	*pOutLen = 7;
}
/**************************************************************************
 *   Hci_Command_Pin_Code_Request_Negative_Reply
 *
 *   Descriptions:
 *      Process HCI command: Pin_Code_Request_Negative_Reply
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pLinkKeyPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Pin_Code_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 pCodeReqNegPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Pin_Code_Request_Negative_Reply Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_PIN_CODE_REQUEST_NEGATIVE_REPLY);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pCodeReqNegPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pCodeReqNegPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	// The response event use the BD_ADDR field of connect-device. So we must store it.
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_PIN_CODE_NEG_REPLY);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}
/**************************************************************************
 *   Hci_Response_Pin_Code_Request_Negative_Reply
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Pin_Code_Request_Negative_Reply.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Pin_Code_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device != NULL)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
	}
	*pOutLen = 7;
}
/**************************************************************************
 *   Hci_Command_Pin_Code_Request_Reply
 *
 *   Descriptions:
 *      Process HCI command: Pin_Code_Request_Reply
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pCodeReqReplyPara: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Pin_Code_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 pCodeReqReplyPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Pin_Code_Request_Reply Enter!\n");
	// Change the hci command state
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_PIN_CODE_REQUEST_REPLY);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pCodeReqReplyPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pCodeReqReplyPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->pin_code_length = *(pCodeReqReplyPara + 6);
			RtlCopyMemory(pTempConnectDevice->pin_code, pCodeReqReplyPara + 7, BT_LINK_KEY_LENGTH);
		}
	}
	else
	{
		pTempConnectDevice->pin_code_length = *(pCodeReqReplyPara + 6);
		RtlCopyMemory(pTempConnectDevice->pin_code, pCodeReqReplyPara + 7, BT_LINK_KEY_LENGTH);
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_PIN_CODE_REPLY);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}
/**************************************************************************
 *   Hci_Response_Pin_Code_Request_Reply
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Pin_Code_Request_Reply.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Pin_Code_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	if (pHci->pcurrent_connect_device != NULL)
	{
		RtlCopyMemory(dest, pHci->pcurrent_connect_device->bd_addr, BT_BD_ADDR_LENGTH);
		dest += BT_BD_ADDR_LENGTH;
	}
	*pOutLen = 7;
}
/**************************************************************************
 *   Hci_Command_Set_Connection_Encryption
 *
 *   Descriptions:
 *      Process HCI command: Set_Connection_Encryption
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, the connection handle.
 *      encryp_en: IN, encryption_enable.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Set_Connection_Encryption(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 encryp_en)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Set_Connection_Encryption Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_SET_CONNECTION_ENCRYPTION);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	
	
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
        /* BlueZ ONLY: delay the status event to moment of command complete */	
    	if (encryp_en){
    		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_ON);
    	}
    	else{
    		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_OFF);
    	}
    }

}
/**************************************************************************
 *   Hci_Command_Authentication_Requested
 *
 *   Descriptions:
 *      Process HCI command: Authentication_Requested
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, the connection handle.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Authentication_Requested(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_AUTHENTICATION_REQUESTED);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	// Change the hci command state
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	if (pTempConnectDevice != NULL)
		pTempConnectDevice->pause_command_flag = 4;

    //if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
    Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    //}
    //else{
    LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_AUTH_REQUEST);
   // }

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Change_Connection_Packet_Type
 *
 *   Descriptions:
 *      Process HCI command: Change_Connection_Packet_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, the connection handle.
 *      Packet_Type: IN, the packet type.
 *   Return Value:
 *      None
 *************************************************************************/

VOID Hci_Command_ChangeConnectionPacketType(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PSCO_CONNECT_DEVICE_T pTempScoConnectDevice;
	UINT8 eventcode;
	UINT8 needchangeflag = 0;
	UINT8 link_type = BT_LINK_TYPE_ACL;
	
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_ChangeConnectionPacketType Enter!\n");
	
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_CHANGE_CONNECTION_PACKET_TYPE);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);

	if (pTempConnectDevice == NULL)
	{

		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);

	}
	
	if (pTempConnectDevice == NULL)
	{
		pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
	}
	else
	{
		if (link_type == BT_LINK_TYPE_ACL)
		{
			pTempConnectDevice->packet_type = Packet_Type;
			pTempConnectDevice->changed_max_slot = pTempConnectDevice->tx_max_slot;
			pTempConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			
			if (pTempConnectDevice->edr_mode == 0)
			{
				if (pHci->lmp_features.byte3.enh_rate_acl_2 == 1 && pTempConnectDevice->lmp_features.byte3.enh_rate_acl_2 == 1 && (pTempConnectDevice->packet_type & BT_PACKET_TYPE_2DH1) == 0)
				{
					//Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE), BT_TASK_PRI_NORMAL, (PUINT8)&pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
					LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE);
				}
				else
				{
					if (Packet_Type & (BT_PACKET_TYPE_DM1 | BT_PACKET_TYPE_DH1 | BT_PACKET_TYPE_DM3 | BT_PACKET_TYPE_DH3 | BT_PACKET_TYPE_DM5 | BT_PACKET_TYPE_DH5))
					{
						if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
						{
							if ((Packet_Type & (BT_PACKET_TYPE_DM5 | BT_PACKET_TYPE_DH5)) == 0)
							{
								needchangeflag = 1;
								if (Packet_Type & (BT_PACKET_TYPE_DM3 | BT_PACKET_TYPE_DH3))
									pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_3_SLOT;
								else
									pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_1_SLOT;
							}
						}
						else if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
						{
							if (Packet_Type & (BT_PACKET_TYPE_DM5 | BT_PACKET_TYPE_DH5))
							{
								needchangeflag = 1;
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_5_SLOT;
							}
							else
							{
								if ((Packet_Type & (BT_PACKET_TYPE_DM3 | BT_PACKET_TYPE_DH3)) == 0)
								{
									needchangeflag = 1;
									pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_1_SLOT;
								}
							}
						}
						else // 1 slot
						{
							if (Packet_Type & (BT_PACKET_TYPE_DM5 | BT_PACKET_TYPE_DH5))
							{
								needchangeflag = 1;
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_5_SLOT;
							}
							else if (Packet_Type & (BT_PACKET_TYPE_DM3 | BT_PACKET_TYPE_DH3))
							{
								needchangeflag = 1;
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_3_SLOT;
							}
						}
					}
				}
			}
			else // edr_mode = 1
			{
				if ((pTempConnectDevice->packet_type & BT_PACKET_TYPE_2DH1) != 0)
				{
					LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE);
				}
				else
				{
					if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_5_SLOT)
					{
						if (((Packet_Type & BT_PACKET_TYPE_3DH5) != 0 || pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] == 0) 
							&& ((Packet_Type & BT_PACKET_TYPE_2DH5) != 0 || pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] == 0))
						{
							needchangeflag = 1;
							if (((Packet_Type & BT_PACKET_TYPE_3DH3) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] == 1) 
								|| ((Packet_Type & BT_PACKET_TYPE_2DH3) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 1))
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_3_SLOT;
							else
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_1_SLOT;
						}
					}
					else if (pTempConnectDevice->tx_max_slot == BT_MAX_SLOT_3_SLOT)
					{
						if (((Packet_Type & BT_PACKET_TYPE_3DH5) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] == 1) 
							|| ((Packet_Type & BT_PACKET_TYPE_2DH5) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] == 1))
						{
							needchangeflag = 1;
							pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_5_SLOT;
						}
						else
						{
							if (((Packet_Type & BT_PACKET_TYPE_3DH3) != 0 || pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] == 0) 
								&& ((Packet_Type & BT_PACKET_TYPE_2DH3) != 0 || pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 0))
							{
								needchangeflag = 1;
								pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_1_SLOT;
							}
						}
					}
					else // 1 slot
					{
						if (((Packet_Type & BT_PACKET_TYPE_3DH5) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH5] == 1) 
							|| ((Packet_Type & BT_PACKET_TYPE_2DH5) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH5] == 1))
						{
							needchangeflag = 1;
							pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_5_SLOT;
						}
						else if (((Packet_Type & BT_PACKET_TYPE_3DH3) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_3DH3] == 1) 
							|| ((Packet_Type & BT_PACKET_TYPE_2DH3) == 0 && pTempConnectDevice->EDR_FeatureBit[BT_ACL_PACKET_2DH3] == 1))
						{
							needchangeflag = 1;
							pTempConnectDevice->changed_max_slot = BT_MAX_SLOT_3_SLOT;
						}
					}
				}
			}
		}
		else
		{
			pTempScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
			ASSERT(pTempScoConnectDevice != NULL);
			pTempScoConnectDevice->packet_type = Packet_Type;
			if (Packet_Type & BT_PACKET_TYPE_HV1)
				pTempScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV1;
			if (Packet_Type & BT_PACKET_TYPE_HV2)
				pTempScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV2;
			if (Packet_Type & BT_PACKET_TYPE_HV3)
				pTempScoConnectDevice->esco_packet_type |= BT_PACKET_TYPE_ESCO_HV3;
			
			pTempScoConnectDevice->changed_packet_type = pTempScoConnectDevice->current_packet_type;
			pTempScoConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			
			if (Packet_Type & (BT_PACKET_TYPE_HV1 | BT_PACKET_TYPE_HV2 | BT_PACKET_TYPE_HV3))
			{
				if (pTempScoConnectDevice->current_packet_type == BT_SCO_PACKET_HV1)
				{
					if ((Packet_Type & BT_PACKET_TYPE_HV1) != BT_PACKET_TYPE_HV1)
					{
						needchangeflag = 1;
						if (Packet_Type & BT_PACKET_TYPE_HV3)
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV3;
						else
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV2;
					}
				}
				else if (pTempScoConnectDevice->current_packet_type == BT_SCO_PACKET_HV2)
				{
					if ((Packet_Type & BT_PACKET_TYPE_HV2) != BT_PACKET_TYPE_HV2)
					{
						needchangeflag = 1;
						if (Packet_Type & BT_PACKET_TYPE_HV3)
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV3;
						else
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV1;
					}
				}
				else // HV3
				{
					if ((Packet_Type & BT_PACKET_TYPE_HV3) != BT_PACKET_TYPE_HV3)
					{
						needchangeflag = 1;
						if (Packet_Type & BT_PACKET_TYPE_HV2)
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV2;
						else
							pTempScoConnectDevice->changed_packet_type = BT_SCO_PACKET_HV1;
					}
				}
			}
		}
	}
	
	
    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        /* Fail to process this command */
    	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 0);
        return;
    }
	
	if (needchangeflag)
	{
		if (link_type == BT_LINK_TYPE_ACL)
		{
			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_PACKET_TYPE);
		}
		else
		{
			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_SCO_PACKET_TYPE);
		}
	}
	else
	{
		if (link_type == BT_LINK_TYPE_ACL)
		{
			Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(devExt, (PUINT8)&pTempConnectDevice, 1);
		}
		else
		{
			Task_HCI2HC_Send_Sco_Conn_Packet_Type_Changed_Event(devExt, (PUINT8)&pTempConnectDevice, 1);
		}
	}
}

VOID Hci_Command_Change_Connection_Packet_Type(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Change_Connection_Packet_Type Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return ;
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_CHANGE_CONNECTION_PACKET_TYPE);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->packet_type = Packet_Type;
		}
	}
	else
	{
		pTempConnectDevice->packet_type = Packet_Type;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 0);
	Task_HCI2HC_Send_Conn_Packet_Type_Changed_Event(devExt, (PUINT8) &pTempConnectDevice, 1);
}

/**************************************************************************
 *   Hci_Command_Add_Sco_Connection
 *
 *   Descriptions:
 *      Process HCI command: Add_Sco_Connection
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, the connection handle.
 *      Packet_Type: IN, the packet type.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Add_Sco_Connection(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_FRAG_T pFrag;
	UINT8 id;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Add_Sco_Connection Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_ADD_SCO_CONNECTION);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			if (pTempConnectDevice->pScoConnectDevice != NULL)
			{
				pHci->command_status = BT_HCI_ERROR_MAX_NUMBER_OF_SCO_CONNECTIONS;
			}
			else
			{
				pTempConnectDevice->pScoConnectDevice = Hci_Add_Sco_Connect_Device(pHci, Packet_Type, (PVOID)pTempConnectDevice);
			}
			
		}
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		if (pTempConnectDevice->pScoConnectDevice != NULL)
		{
			pHci->command_status = BT_HCI_ERROR_MAX_NUMBER_OF_SCO_CONNECTIONS;
		}
		else
		{
			pTempConnectDevice->pScoConnectDevice = Hci_Add_Sco_Connect_Device(pHci, Packet_Type, (PVOID)pTempConnectDevice);
		}
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	}

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        /* Fail to add SCO connection */
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);

    }
    else if (pTempConnectDevice  && pTempConnectDevice->pScoConnectDevice)
	{
		devExt->ScoAmaddr = pTempConnectDevice->am_addr;
		pHci->pcurrent_connect_device = pTempConnectDevice;
		Hci_ScoStartTimer((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice), BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
		((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->initiator = 1;
		id = ((PSCO_CONNECT_DEVICE_T)(pTempConnectDevice->pScoConnectDevice))->index;
		ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
		BT_DBGEXT(ZONE_HCI | LEVEL3, "SCO index = %d\n", id);
		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		pFrag->RxScoElement[id].currentpos = 0;
		pFrag->RxScoElement[id].currentlen = 0;
		pFrag->RxScoElement[id].totalcount = 0;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		
		if (pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt) || !Frag_IsEleEmpty(devExt))
		{
			pHci->acl_temp_pending_flag = 1;
			pHci->auto_packet_select_flag = 0;
			pHci->sco_need_1_slot_for_acl_flag = 1;
			pHci->hv1_use_dv_instead_dh1_flag = 0;
			Frag_StopQueueForSco(devExt);
			UsbQueryDMASpace(devExt);
		}
		
		#ifdef BT_TEST_SCO_DELAY_COUNTS
			devExt->RecvTotalScoCount = 0;
			devExt->RecvRealScoCount = 0;
			devExt->RecvScoNullCount = 0;
			devExt->RecvAddExtraCount = 0;
			devExt->TxTotalScoCount = 0;
			devExt->TxRealScoCount = 0;
			devExt->TxDiscardSpilthScoCount = 0;
			devExt->TxReservedScoCount = 0;
		#endif
		if (pHci->acl_temp_pending_flag)
		{
			Task_Normal_Add_Sco(devExt, (PUINT8) &pHci->pcurrent_connect_device, 0);
		}
		else
		{
			pHci->auto_packet_select_flag = 0;
			pHci->sco_need_1_slot_for_acl_flag = 1;
			pHci->hv1_use_dv_instead_dh1_flag = 0;
			Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 1);
			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT);
			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ADD_SCO);
		}
    }

}

   
/**************************************************************************
 *   Hci_Command_Write_Sco_Flow_Control_Enable
 *
 *   Descriptions:
 *      Process HCI command: Write_Sco_Flow_Control_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ScoFlowControlEnable: IN, SCO_Flow_Control_Enable.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, UINT8 ScoFlowControlEnable)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_SCO_FLOW_CONTROL_ENABLE);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Sco_Flow_Control_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Sco_Flow_Control_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Sco_Flow_Control_Enable
 *
 *   Descriptions:
 *      Process HCI command: Read_Sco_Flow_Control_Enable
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_SCO_FLOW_CONTROL_ENABLE);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Sco_Flow_Control_Enable
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Sco_Flow_Control_Enable.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->sco_flow_control_enable;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Flush
 *
 *   Descriptions:
 *      Process HCI command: Flush
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Flush(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_FLUSH);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		Frag_FlushTxForConnDev(devExt, pTempConnectDevice);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pTempConnectDevice->flush_flag = 1;
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Frag_FlushTxForConnDev(devExt, pTempConnectDevice);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			pTempConnectDevice->flush_flag = 1;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Flush
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Link_Policy_Settings.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Flush(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Write_Automatic_Flush_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Write_Automatic_Flush_Timeout
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *      FlushTimeout: IN, Flush_Timeout
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 FlushTimeout)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_AUTOMATIC_FLUSH_TIMEOUT);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->flush_timeout = FlushTimeout;
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->flush_timeout = FlushTimeout;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Automatic_Flush_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Link_Policy_Settings.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Read_Automatic_Flush_Timeout
 *
 *   Descriptions:
 *      Process HCI command: Read_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_AUTOMATIC_FLUSH_TIMEOUT);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Automatic_Flush_Timeout
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Link_Policy_Settings.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*(PUINT16)dest = pHci->pcurrent_connect_device->flush_timeout;
		dest += sizeof(UINT16);
	}
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Read_Stored_Link_Key
 *
 *   Descriptions:
 *      Process HCI command: Read_Stored_Link_Key
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pStoredLinkKey: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 read_all_flag;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);    
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_STORED_LINK_KEY);

	read_all_flag = *(pStoredLinkKey + BT_BD_ADDR_LENGTH);
	if (read_all_flag)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "read all link key!\n");
		pHci->pcurrent_connect_device = NULL;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	  	//Jakio20071126: add here according netcafe
	  	 //JakioToDo: there is event handler for this event. check it
		if (pHci->num_device_am + pHci->num_device_pm + pHci->num_device_slave > 0)
		{
			Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_RETURN_LINK_KEYS_EVENT), BT_TASK_PRI_NORMAL, (PUINT8)&pHci->pcurrent_connect_device, sizeof(PCONNECT_DEVICE_T));
		}

		Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		return ;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pStoredLinkKey);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pStoredLinkKey);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pHci->pcurrent_connect_device = pTempConnectDevice;
		}
	}
	else
	{
		pHci->pcurrent_connect_device = pTempConnectDevice;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	  //Jakio20071126: add here according netcafe
	  //JakioToDo: there is event handler for this event. check it
	if (pTempConnectDevice != NULL)
	{
		Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2HC_EVENT(BT_TASK_EVENT_HCI2HC_SEND_RETURN_LINK_KEYS_EVENT), BT_TASK_PRI_NORMAL, (PUINT8)&pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Stored_Link_Key
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Stored_Link_Key.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = BT_MAX_DEVICE_NUM;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device == NULL)
	{
		if (pHci->command_status == BT_HCI_STATUS_SUCCESS)
		{
			*(PUINT16)dest = pHci->num_device_am + pHci->num_device_pm + pHci->num_device_slave;
			dest += sizeof(UINT16);
		}
		else
		{
			*(PUINT16)dest = 0;
			dest += sizeof(UINT16);
		}
	}
	else
	{
		*(PUINT16)dest = 1;
		dest += sizeof(UINT16);
	}
	*pOutLen = 5;
}
/**************************************************************************
 *   Hci_Command_Write_Stored_Link_Key
 *
 *   Descriptions:
 *      Process HCI command: Write_Stored_Link_Key
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pStoredLinkKey: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 num_keys_to_write, i, num_keys_written;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_STORED_LINK_KEY);

	num_keys_to_write =  *pStoredLinkKey;
	BT_DBGEXT(ZONE_HCI | LEVEL3, "num_keys_to_write: %d\n", num_keys_to_write);
	num_keys_written = 0;
	for (i = 0; i < num_keys_to_write; i++)
	{
		pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pStoredLinkKey + 1 + i * (BT_BD_ADDR_LENGTH + BT_LINK_KEY_LENGTH));
		if (pTempConnectDevice == NULL)
		{
			pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pStoredLinkKey + 1 + i * (BT_BD_ADDR_LENGTH + BT_LINK_KEY_LENGTH));
			if (pTempConnectDevice != NULL)
			{
				RtlCopyMemory(pTempConnectDevice->link_key, pStoredLinkKey + 1 + i * (BT_BD_ADDR_LENGTH + BT_LINK_KEY_LENGTH) + BT_BD_ADDR_LENGTH, BT_LINK_KEY_LENGTH);
				num_keys_written++;
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Find slave, i = %d pTempConnectDevice = 0x%x\n", i, pTempConnectDevice);
			}
		}
		else
		{
			RtlCopyMemory(pTempConnectDevice->link_key, pStoredLinkKey + 1 + i * (BT_BD_ADDR_LENGTH + BT_LINK_KEY_LENGTH) + BT_BD_ADDR_LENGTH, BT_LINK_KEY_LENGTH);
			num_keys_written++;
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Find master, i = %d pTempConnectDevice = 0x%x\n", i, pTempConnectDevice);
		}
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pHci->num_keys_written = num_keys_written;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Stored_Link_Key
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Stored_Link_Key.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->num_keys_written;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Delete_Stored_Link_Key
 *
 *   Descriptions:
 *      Process HCI command: Delete_Stored_Link_Key
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      pStoredLinkKey: IN, the pointer to the parameter of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Delete_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 delete_all_flag, i;

    ENTER_FUNC();

    pHci = (PBT_HCI_T)devExt->pHci;
    KeAcquireSpinLock(&pHci->HciLock, &oldIrql);    
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_DELETE_STORED_LINK_KEY);

	delete_all_flag = *(pStoredLinkKey + BT_BD_ADDR_LENGTH);
	if (delete_all_flag)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "delete all link key\n");
		for (i = 0; i < BT_MAX_DEVICE_NUM; i++)
		{
			RtlZeroMemory(pHci->device_all[i].link_key, BT_LINK_KEY_LENGTH);
		}
		pHci->num_keys_written = pHci->num_device_am + pHci->num_device_pm + pHci->num_device_slave;
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, pStoredLinkKey);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, pStoredLinkKey);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if (pTempConnectDevice == NULL)
			{
				for (i = 0; i < BT_MAX_DEVICE_NUM; i++)
				{
					if (RtlEqualMemory(pHci->device_all[i].bd_addr, pStoredLinkKey, BT_BD_ADDR_LENGTH))
					{
						BT_DBGEXT(ZONE_HCI | LEVEL3, "Find one connect device, i = %d\n", i);
						RtlZeroMemory(pHci->device_all[i].link_key, BT_LINK_KEY_LENGTH);
						pHci->num_keys_written = 1;
						break;
					}
				}
				if (i == BT_MAX_DEVICE_NUM)
				{
					pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
					pHci->num_keys_written = 0;
				}
			}
			else
			{
				RtlZeroMemory(pTempConnectDevice->link_key, BT_LINK_KEY_LENGTH);
				pHci->num_keys_written = 1;
				BT_DBGEXT(ZONE_HCI | LEVEL3, "delete link key, slave pTempConnectDevice = 0x%x\n", pTempConnectDevice);
			}
		}
		else
		{
			RtlZeroMemory(pTempConnectDevice->link_key, BT_LINK_KEY_LENGTH);
			pHci->num_keys_written = 1;
			BT_DBGEXT(ZONE_HCI | LEVEL3, "delete link key, master pTempConnectDevice = 0x%x\n", pTempConnectDevice);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Delete_Stored_Link_Key
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Delete_Stored_Link_Key.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Delete_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->num_keys_written;
	dest += sizeof(UINT16);
	*pOutLen = 3;
}
/**************************************************************************
 *   Hci_Command_Hold_Mode
 *
 *   Descriptions:
 *      Process HCI command: Hold_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *      MaxInterval: IN, Hold_Mode_Max_Interval
 *      MinInterval: IN, Hold_Mode_Min_Interval
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Hold_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 MaxInterval, UINT16 MinInterval)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 Datastr[8];

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_HOLD_MODE);


	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->hold_mode_max_interval = MaxInterval;
		pTempConnectDevice->hold_mode_min_interval = MinInterval;
		pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_HODE_MODE;
		pHci->pcurrent_connect_device = pTempConnectDevice;
		Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
		Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE, 2);
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->hold_mode_max_interval = MaxInterval;
			pTempConnectDevice->hold_mode_min_interval = MinInterval;
			pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_HODE_MODE;
			pHci->pcurrent_connect_device = pTempConnectDevice;
			Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
			Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE, 2);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Sniff_Mode
 *
 *   Descriptions:
 *      Process HCI command: Sniff_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *      MaxInterval: IN, Sniff_Max_Interval
 *      MinInterval: IN, Sniff_Min_Interval
 *      Attempt: IN, Sniff_Attempt
 *      Timeout: IN, Sniff_Timeout
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Sniff_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 MaxInterval, UINT16 MinInterval, UINT16 Attempt, UINT16 Timeout)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	UINT8 Datastr[8];

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_SNIFF_MODE);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice != NULL)
	{
		pTempConnectDevice->sniff_max_interval = MaxInterval;
		pTempConnectDevice->sniff_min_interval = MinInterval;
		pTempConnectDevice->sniff_attempt = Attempt;
		pTempConnectDevice->sniff_timeout = Timeout;
		pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_SNIFF_MODE;
		pHci->pcurrent_connect_device = pTempConnectDevice;
		Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
		Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE, 2);
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->sniff_max_interval = MaxInterval;
			pTempConnectDevice->sniff_min_interval = MinInterval;
			pTempConnectDevice->sniff_attempt = Attempt;
			pTempConnectDevice->sniff_timeout = Timeout;
			pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_SNIFF_MODE;
			pHci->pcurrent_connect_device = pTempConnectDevice;
			Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
			Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE, 2);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Exit_Sniff_Mode
 *
 *   Descriptions:
 *      Process HCI command: Sniff_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Exit_Sniff_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();    
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_EXIT_SNIFF_MODE);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);

			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_EXIT_SNIFF_REQ);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_EXIT_SNIFF_REQ);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    EXIT_FUNC();
}

/**************************************************************************
 *   Hci_Command_Switch_Role
 *
 *   Descriptions:
 *      Process HCI command: Switch_Role
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      RolePara: IN, the parameters of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Switch_Role(PBT_DEVICE_EXT devExt, PUINT8 RolePara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	BOOLEAN is_in_switch_role_flag = FALSE;
	UINT8 Datastr[8];
	/* jakio20070808: add two vars
*/
	LARGE_INTEGER timevalue;
	UINT8 Timer_Flag = 0;

	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_SWITCH_ROLE);
	
	pHci->role_switch_fail_count=0;	
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, RolePara);
	if (pTempConnectDevice == NULL)
	{
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, RolePara);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			if (pTempConnectDevice->role_switching_flag)
			{
				is_in_switch_role_flag = TRUE;
			}
			else
			{
				if (pTempConnectDevice->pScoConnectDevice || (pTempConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE))
				{
					pHci->command_status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
				}
				else
				{
					pTempConnectDevice->role_switching_role = *(RolePara + 6);
					if (pTempConnectDevice->role_switching_role != pTempConnectDevice->raw_role)
					{
						pTempConnectDevice->pause_command_flag = 1; // Add for pause encryption by Lewis.wang
						pTempConnectDevice->role_switching_flag = 1;
						pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
						pHci->pcurrent_connect_device = pTempConnectDevice;
						Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
						Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE, 10);
					}
				}
			}
		}
	}
	else
	{
		if (pTempConnectDevice->role_switching_flag)
		{
			is_in_switch_role_flag = TRUE;
		}
		else
		{
			if (pTempConnectDevice->pScoConnectDevice || (pTempConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE))
			{
				pHci->command_status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
			}
			else
			{
				pTempConnectDevice->role_switching_role = *(RolePara + 6);
				if (pTempConnectDevice->role_switching_role != pTempConnectDevice->raw_role)
				{
					pTempConnectDevice->pause_command_flag = 1; // Add for pause encryption by Lewis.wang
					pTempConnectDevice->role_switching_flag = 1;
					pTempConnectDevice->clock_ready_flag = BT_CLOCK_READY_FLAG_ROLE_SWITCH;
					pHci->pcurrent_connect_device = pTempConnectDevice;
					Hci_Write_Data_To_EightBytes(devExt, BT_REG_OP_BD_ADDR_DSP, pTempConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
					Hci_StartProtectHciCommandTimer(pHci, BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE, 10);
				}
			}
		}
	}
    
	if (pHci->command_status == BT_HCI_STATUS_SUCCESS)
	{
		if (!is_in_switch_role_flag)
		{
			if (pTempConnectDevice->role_switching_role == pTempConnectDevice->raw_role)
			{
				Task_HCI2HC_Send_Role_Change_Event(devExt, (PUINT8) &pTempConnectDevice, 0);
			}
		}
	}
	else
	{
		Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 0);
	}
    tasklet_schedule(&devExt->taskletRcv);
}
/**************************************************************************
 *   Hci_Command_Read_Local_Supported_Commands
 *
 *   Descriptions:
 *      Process HCI command: Read_Local_Supported_Commands
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Local_Supported_Commands(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS, BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_COMMANDS);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Local_Supported_Commands
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Local_Supported_Commands.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Local_Supported_Commands(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	#ifdef BT_USING_ADD_SCO_COMMAND
		*dest = 0x7f;
	#else
		*dest = 0x3f;
	#endif
	dest += sizeof(UINT8);
	*dest = 0xff;
	dest += sizeof(UINT8);
	*dest = 0xf9;
	dest += sizeof(UINT8);
	*dest = 0x3;
	dest += sizeof(UINT8);
	*dest = 0xcc;
	dest += sizeof(UINT8);
	*dest = 0xff;
	dest += sizeof(UINT8);
	*dest = 0xee;
	dest += sizeof(UINT8);
	*dest = 0xff;
	dest += sizeof(UINT8);
	*dest = 0xff;
	dest += sizeof(UINT8);
	*dest = 0xff;
	dest += sizeof(UINT8);
	*dest = 0x1c;
	dest += sizeof(UINT8);
	*dest = 0x3;
	dest += sizeof(UINT8);
	*dest = 0xf2;
	dest += sizeof(UINT8);
	*dest = 0xf;
	dest += sizeof(UINT8);
	*dest = 0xe8;
	dest += sizeof(UINT8);
	*dest = 0x6e;
	dest += sizeof(UINT8);
	*dest = 0x38;
#ifdef BT_LOOPBACK_SUPPORT
	*dest |= 0x3;
#endif
#ifdef BT_TESTMODE_SUPPORT
	*dest |= 0x4;
#endif
	dest += sizeof(UINT8);
	RtlZeroMemory(dest, 64-17);
	*pOutLen = 65;
}
/**************************************************************************
 *   Hci_Command_Write_Inquiry_Mode
 *
 *   Descriptions:
 *      Process HCI command: Write_Inquiry_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      InquiryMode: IN, Inquiry_Mode
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Inquiry_Mode(PBT_DEVICE_EXT devExt, UINT8 InquiryMode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_INQUIRY_MODE);

	pHci->inquiry_mode = InquiryMode;
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Inquiry_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Inquiry_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Inquiry_Mode
 *
 *   Descriptions:
 *      Process HCI command: Read_Inquiry_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Inquiry_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_INQUIRY_MODE);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Inquiry_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Inquiry_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->inquiry_mode;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Inquiry_Scan_Type
 *
 *   Descriptions:
 *      Process HCI command: Inquiry_Scan_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      InquiryScanType: IN, Inquiry_Scan_Type
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, UINT8 InquiryScanType)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_TYPE);

	pHci->inquiry_scan_type = InquiryScanType;
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Inquiry_Scan_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Inquiry_Scan_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Inquiry_Scan_Type
 *
 *   Descriptions:
 *      Process HCI command: Read_Inquiry_Scan_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_INQUIRY_SCAN_TYPE);

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Inquiry_Scan_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Inquiry_Scan_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->inquiry_scan_type;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Page_Scan_Type
 *
 *   Descriptions:
 *      Process HCI command: Page_Scan_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      PageScanType: IN, Page_Scan_Type
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Page_Scan_Type(PBT_DEVICE_EXT devExt, UINT8 PageScanType)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_PAGE_SCAN_TYPE);
 	pHci->page_scan_type = PageScanType;
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Page_Scan_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Page_Scan_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Page_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Page_Scan_Type
 *
 *   Descriptions:
 *      Process HCI command: Read_Page_Scan_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Page_Scan_Type(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_PAGE_SCAN_TYPE);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Page_Scan_Type
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Page_Scan_Type.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Page_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->page_scan_type;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_AFH_Channel_Assessment_Mode
 *
 *   Descriptions:
 *      Process HCI command: Write_AFH_Channel_Assessment_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      AFHChMode: IN, AFH_Channel_Assessment_Mode.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, UINT8 AFHChMode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_AFH_CHANNEL_ASSESSMENT_MODE);
 	pHci->afh_ch_assessment_mode = AFHChMode;
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_AFH_Channel_Assessment_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_AFH_Channel_Assessment_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_AFH_Channel_Assessment_Mode
 *
 *   Descriptions:
 *      Process HCI command: Read_Page_Scan_Type
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_AFH_CHANNEL_ASSESSMENT_MODE);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_AFH_Channel_Assessment_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_AFH_Channel_Assessment_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->afh_ch_assessment_mode;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Hold_Mode_Activity
 *
 *   Descriptions:
 *      Process HCI command: Write_Hold_Mode_Activity
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      HoldModeActivity: IN, Hold_Mode_Activity.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, UINT8 HoldModeActivity)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_HOLD_MODE_ACTIVITY);
 	pHci->hold_mode_activity = HoldModeActivity;
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Hold_Mode_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Hold_Mode_Activity.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Hold_Mode_Activity
 *
 *   Descriptions:
 *      Process HCI command: Read_Hold_Mode_Activity
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Hold_Mode_Activity(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
    pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
    pHci->command_status = BT_HCI_STATUS_SUCCESS;
    pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_HOLD_MODE_ACTIVITY);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Hold_Mode_Activity
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Hold_Mode_Activity.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->hold_mode_activity;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Set_AFH_Host_Channel_Classification
 *
 *   Descriptions:
 *      Process HCI command: Set_AFH_Host_Channel_Classification
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Channel_Classification: IN, the pointer to parameter.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Set_AFH_Host_Channel_Classification(PBT_DEVICE_EXT devExt, PUINT8 Channel_Classification)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	UINT8 i, j, classification[BT_MAX_CHANNEL_MAP_NUM];

	ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
 	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_SET_AFH_HOST_CHANNEL_CLASSIFICATION);

 	RtlFillMemory(classification, BT_MAX_CHANNEL_MAP_NUM, 0xFF);
	for (i = 0; i < BT_MAX_CHANNEL_MAP_NUM; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (((Channel_Classification[i] >> (2 * j)) & 0x03) == 0x03)
				classification[i] = classification[i] & (~(0x03 << (2 * j)));
		}
	}
	RtlCopyMemory(pHci->classification_channel_map, classification, BT_MAX_CHANNEL_MAP_NUM);
	pHci->upper_set_classification = 1;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
	EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Set_AFH_Host_Channel_Classification
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Set_AFH_Host_Channel_Classification.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Set_AFH_Host_Channel_Classification(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Write_Current_Iac_Lap
 *
 *   Descriptions:
 *      Process HCI command: Write_Current_Iac_Lap
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      param: IN, the pointer to parameter.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Current_Iac_Lap(PBT_DEVICE_EXT devExt, PUINT8 param)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	UINT8 i;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
 	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_CURRENT_IAC_LAP);

 	pHci->num_current_iac =  *param;
	if (pHci->num_current_iac > BT_MAX_IAC_LAP_NUM)
	{
		pHci->num_current_iac = BT_MAX_IAC_LAP_NUM;
	}
	if (pHci->num_current_iac < 1)
	{
		pHci->num_current_iac = 1;
	}
	RtlCopyMemory((PUINT8) &(pHci->iac_lap[0]), param + 1, pHci->num_current_iac *BT_EACH_IAC_LAP_COUNT);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Num_Current_IAC: %d\n", pHci->num_current_iac);
	for (i = 0; i < pHci->num_current_iac; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "IAC_LAP[%d]: %02x %02x %02x\n", i, pHci->iac_lap[i].iac_lap[0], pHci->iac_lap[i].iac_lap[1], pHci->iac_lap[i].iac_lap[2]);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Current_Iac_Lap
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Current_Iac_Lap.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Current_Iac_Lap(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Page_Scan_Mode
 *
 *   Descriptions:
 *      Process HCI command: Read_Page_Scan_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Page_Scan_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_PAGE_SCAN_MODE);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Page_Scan_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Page_Scan_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Page_Scan_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->page_scan_mode;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}

/**************************************************************************
 *   Hci_Command_Set_Event_Filter
 *
 *   Descriptions:
 *      Process HCI command: Set_Event_Filter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      filter_type: IN, Filter_Type.
 *      filter_condition_type: IN, Filter_Condition_Type.
 *      condition: IN, condition.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Set_Event_Filter(PBT_DEVICE_EXT devExt, PUINT8 param)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Set_Event_Filter Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_SET_EVENT_FILTER);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

#ifdef BT_EVENT_FILTER_SUPPORT
    /** TODO **/
#endif

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Set_Event_Filter
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Set_Event_Filter.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Set_Event_Filter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Unknown_Command
 *
 *   Descriptions:
 *      Process an unknown HCI command.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      opcode: IN, opcode
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Unknown_Command(PBT_DEVICE_EXT devExt, UINT16 opcode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Unknown_Command Enter!\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return ;
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_ERROR_UNKNOWN_HCI_COMMAND;
		pHci->current_opcode = opcode;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}
/**************************************************************************
 *   Hci_Response_Unknown_Command
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the unknown HCI command.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Unknown_Command(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_Rssi
 *
 *   Descriptions:
 *      Process HCI command: Read_Rssi
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Rssi(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_STATUS_PARAMETERS, BT_HCI_COMMAND_READ_RSSI);

 	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Rssi
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Rssi.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Rssi(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*dest = (UINT8)pHci->pcurrent_connect_device->rssi;
		dest += sizeof(UINT8);
	}
	*pOutLen = 4;
}
/**************************************************************************
 *   Hci_Command_Read_AFH_Channel_Map
 *
 *   Descriptions:
 *      Process HCI command: Read_AFH_Channel_Map
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle: IN, Connection_Handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_AFH_Channel_Map(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = ConnHandle;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_STATUS_PARAMETERS, BT_HCI_COMMAND_READ_AFH_CHANNEL_MAP);

 	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	pHci->pcurrent_connect_device = pTempConnectDevice;
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_AFH_Channel_Map
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_AFH_Channel_Map.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_AFH_Channel_Map(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);
	if (pHci->pcurrent_connect_device != NULL)
	{
		*dest = (UINT8)pHci->pcurrent_connect_device->afh_mode;
		dest += sizeof(UINT8);
		RtlCopyMemory(dest, pHci->afh_channel_map, BT_MAX_CHANNEL_MAP_NUM);
		dest += BT_MAX_CHANNEL_MAP_NUM;
	}
	*pOutLen = 14;
}
/**************************************************************************
 *   Hci_Command_Read_Loopback_Mode
 *
 *   Descriptions:
 *      Process HCI command: Read_Loopback_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Loopback_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_TESTING, BT_HCI_COMMAND_READ_LOOPBACK_MODE);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_Loopback_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Loopback_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_Loopback_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*dest = pHci->loopback_mode;
	dest += sizeof(UINT8);
	*pOutLen = 2;
}
/**************************************************************************
 *   Hci_Command_Write_Loopback_Mode
 *
 *   Descriptions:
 *      Process HCI command: Write_Loopback_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Loopback_Mode(PBT_DEVICE_EXT devExt, UINT8 Loopback_Mode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_TESTING, BT_HCI_COMMAND_WRITE_LOOPBACK_MODE);

 	#ifdef BT_LOOPBACK_SUPPORT
		if (pHci->loopback_mode == BT_TEST_LOOPBACK_MODE_NO_LOOPBACK)
		{
			if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK)
			{
				if ((pHci->num_device_am > 0) || (pHci->num_device_slave > 0))
				{
					pHci->command_status = BT_HCI_ERROR_ACL_CONNECTIONS_EXISTS;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
				else
				{
					if (Loopback_Allocate_ConnHandles_For_Local_Loopback(devExt) != STATUS_SUCCESS)
					{
						pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
 						Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
					}
					else
					{
						pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK;
 						Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
						Loopback_Responce_Conn_Handles_For_Local_Loopback(devExt);
					}
				}
			}
			else if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK)
			{
				if ((pHci->num_device_am + pHci->num_device_slave) > 1)
				{
					pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
				else
				{
					pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
			}
			else
			{
 				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			}
		}
		else if (pHci->loopback_mode == BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK)
		{
			if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_NO_LOOPBACK)
			{
				pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
 				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				Loopback_Exit_Local_Loopback(devExt);
			}
			else if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK)
			{
				pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
 				Loopback_Exit_Local_Loopback(devExt);
				if ((pHci->num_device_am + pHci->num_device_slave) > 1)
				{
					pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
				else
				{
					pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
			}
			else
			{
 				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			}
		}
		else if (pHci->loopback_mode == BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK)
		{
			if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_NO_LOOPBACK)
			{
				Loopback_Release_Remote_Loopback_Resource(devExt);
				pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_NO_LOOPBACK;
 				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			}
			else if (Loopback_Mode == BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK)
			{
				Loopback_Release_Remote_Loopback_Resource(devExt);
				if ((pHci->num_device_am > 0) || (pHci->num_device_slave > 0))
				{
					pHci->command_status = BT_HCI_ERROR_ACL_CONNECTIONS_EXISTS;
 					Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
				}
				else
				{
					if (Loopback_Allocate_ConnHandles_For_Local_Loopback(devExt) != STATUS_SUCCESS)
					{
						pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
 						Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
					}
					else
					{
						pHci->loopback_mode = BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK;
 						Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
						Loopback_Responce_Conn_Handles_For_Local_Loopback(devExt);
					}
				}
			}
			else
			{
 				Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
			}
		}
		else
		{
 			Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
		}
	#else

 		Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
	#endif

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Write_Loopback_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Loopback_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Write_Loopback_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Enable_Device_Under_Test_Mode
 *
 *   Descriptions:
 *      Process HCI command: Enable_Device_Under_Test_Mode
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Enable_Device_Under_Test_Mode(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
 	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_TESTING, BT_HCI_COMMAND_ENABLE_DEVICE_UNDER_TEST_MODE);

 	pHci->test_flag = 1;
 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Enable_Device_Under_Test_Mode
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Enable_Device_Under_Test_Mode.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Enable_Device_Under_Test_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	*pOutLen = 1;
}
/**************************************************************************
 *   Hci_Command_Read_IVT_Encryption_Key
 *
 *   Descriptions:
 *      Process HCI command: Read_Encryption_Key_From_IVT
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_IVT_Encryption_Key(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
 	pHci = (PBT_HCI_T)devExt->pHci;
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_VENDOR_SPECIFIC, BT_HCI_COMMAND_READ_ENCRYPTION_KEY_FROM_IVT);

 	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Response_Read_IVT_Encryption_Key
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Encryption_Key_From_IVT.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None
 *************************************************************************/
VOID Hci_Response_Read_IVT_Encryption_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	pHci = (PBT_HCI_T)devExt->pHci;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	RtlCopyMemory(dest, pHci->encryption_key_from_ivt, BT_IVT_ENCRYPTION_KEY_LENGTH);
	dest += BT_IVT_ENCRYPTION_KEY_LENGTH;
	*pOutLen = 17;
}
/**************************************************************************
 *   Hci_Command_Host_Buffer_Size
 *
 *   Descriptions:
 *      Process HCI command: Host_Buffer_Size
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      dest:   IN, destination buffer.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Host_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Host_Buffer_Size Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_HOST_BUFFER_SIZE);
	}

	BT_DBGEXT(ZONE_HCI | LEVEL1, "acl_len: 0x%x, sco_len: 0x%x, acl_pack_num: 0x%x, sco_pack_num: 0x%x\n", *(PUINT16)dest, *(dest + 2), *(PUINT16)(dest + 3), *(PUINT16)(dest + 5));
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Host_Buffer_Size
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Host_Buffer_Size.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Host_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Set_Event_Mask
 *
 *   Descriptions:
 *      Process HCI command: Set_Event_Mask
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      dest:   IN, destination buffer.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Set_Event_Mask(PBT_DEVICE_EXT devExt, PUINT8 dest)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Set_Event_Mask Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_SET_EVENT_MASK);
	}

	memcpy(pHci->event_mask, dest, sizeof(pHci->event_mask));
		
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Set_Event_Mask
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Set_Event_Mask.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Set_Event_Mask(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Is_Discard_Command
 *
 *   Descriptions:
 *      Determine whether this command is a discard command(can not be processed
 *      for some reasons).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      opcode: IN, opcode
 *   Return Value:
 *      None
 *************************************************************************/
UINT8 Hci_Command_Is_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (opcode == BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_RESET))
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return 0;
	}
	if (pHci->current_opcode != opcode)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "This command would be discarded!\n");
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return 1;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return 0;
}
/**************************************************************************
 *   Hci_Command_Discard_Command
 *
 *   Descriptions:
 *      Determine whether this command is a discard command(can not be processed
 *      for some reasons).
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      opcode: IN, opcode
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode)
{
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Discard_Command Enter!\n");
}
/**************************************************************************
 *   Hci_Command_Setup_Sync_Connection
 *
 *   Descriptions:
 *      Process HCI command: Hci_Command_Setup_Sync_Connection
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      SyncPara: IN, parameter
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Setup_Sync_Connection(PBT_DEVICE_EXT devExt, PUINT8 SyncPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	UINT16 ConnHandle;
	PSCO_CONNECT_DEVICE_T pScoConnectDevice;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_FRAG_T pFrag;
	UINT8 id;
	UINT8 tmpamaddr = 0;
	UINT8 link_type = BT_LINK_TYPE_ACL;
	UINT8 processcode = 0;
	pHci = (PBT_HCI_T)devExt->pHci;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_connection_handle = *(PUINT16)SyncPara;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_SETUP_SYNC_CONNECTION);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	ConnHandle = *(PUINT16)SyncPara;
	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(pHci, ConnHandle, &link_type);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			if (link_type == BT_LINK_TYPE_ACL)
			{
				pTempConnectDevice->pScoConnectDevice = Hci_Add_eSco_Connect_Device(pHci, *(PUINT32)(SyncPara + 2), *(PUINT32)(SyncPara + 6), *(PUINT16)(SyncPara + 10), *(PUINT16)(SyncPara + 12), *(SyncPara + 14), *(PUINT16)(SyncPara + 15), (PVOID)pTempConnectDevice);
				processcode = 1; // Add a new synchronours connection for slave
			}
			else
			{
				ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);
				pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
				if (pScoConnectDevice->using_esco_command_flag)
				{
					Hci_Modify_eSco_Connect_Device(pHci, *(PUINT32)(SyncPara + 2), *(PUINT32)(SyncPara + 6), *(PUINT16)(SyncPara + 10), *(PUINT16)(SyncPara + 12), *(SyncPara + 14), *(PUINT16)(SyncPara + 15), (PVOID)pScoConnectDevice);
					processcode = 2; // Modify a new synchronours connection for slave
				}
				else
				{
					processcode = 20; // Modify a sco connection, this is illegal.
				}
			}
		}
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	}
	else
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		if (link_type == BT_LINK_TYPE_ACL)
		{
			pTempConnectDevice->pScoConnectDevice = Hci_Add_eSco_Connect_Device(pHci, *(PUINT32)(SyncPara + 2), *(PUINT32)(SyncPara + 6), *(PUINT16)(SyncPara + 10), *(PUINT16)(SyncPara + 12), *(SyncPara + 14), *(PUINT16)(SyncPara + 15), (PVOID)pTempConnectDevice);
			processcode = 3; // Add a new synchronours connection for master

			if (pTempConnectDevice->pScoConnectDevice == NULL)
    		{
    			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
    		}

		}
		else
		{
			ASSERT(pTempConnectDevice->pScoConnectDevice != NULL);
			pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
			if (pScoConnectDevice->using_esco_command_flag)
			{
				Hci_Modify_eSco_Connect_Device(pHci, *(PUINT32)(SyncPara + 2), *(PUINT32)(SyncPara + 6), *(PUINT16)(SyncPara + 10), *(PUINT16)(SyncPara + 12), *(SyncPara + 14), *(PUINT16)(SyncPara + 15), (PVOID)pScoConnectDevice);
				processcode = 4; // Modify a new synchronours connection for master
			}
			else
			{
				processcode = 20; // Modify a sco connection, this is illegal.
			}
		}
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	}

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else if(pTempConnectDevice && pTempConnectDevice->pScoConnectDevice)
    {
    	switch (processcode)
    	{
    		case (1): case (3):
    		// Add a new synchronours
    		BT_DBGEXT(ZONE_HCI | LEVEL3, "Add a new synchronours connection\n");
    		pScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
    		// Store current pointer to connect-device
    		pHci->pcurrent_connect_device = pTempConnectDevice;
    		// Start monitor timer
    		Hci_ScoStartTimer(pScoConnectDevice, BT_TIMER_TYPE_MONITER_CONN_COMP, BT_TIMER_MONITER_CONN_COMP_VALUE);
    		pScoConnectDevice->initiator = 1;
    		id = pScoConnectDevice->index;
    		ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
    		BT_DBGEXT(ZONE_HCI | LEVEL3, "SCO index = %d\n", id);
    		/*Get pointer to the test */
    		pFrag = (PBT_FRAG_T)devExt->pFrag;
    		pFrag->RxScoElement[id].currentpos = 0;
    		pFrag->RxScoElement[id].currentlen = 0;
    		pFrag->RxScoElement[id].totalcount = 0;
    		//if (pHci->acl_temp_pending_flag || !Frag_IsEleEmpty(devExt))
    		//if(pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt) || !Frag_IsEleEmpty(devExt))
    		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    		//if(pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt))
    		if (pHci->acl_temp_pending_flag || !BtIsBDEmpty(devExt) || !Frag_IsEleEmpty(devExt))
    		{
    			pHci->acl_temp_pending_flag = 1;
    			pHci->auto_packet_select_flag = 0;
    			pHci->sco_need_1_slot_for_acl_flag = 1;
    			pHci->hv1_use_dv_instead_dh1_flag = 0;
    			Frag_StopQueueForSco(devExt);
    			UsbQueryDMASpace(devExt);
    		}
    		
    	#ifdef BT_TEST_SCO_DELAY_COUNTS
    		devExt->RecvTotalScoCount = 0;
    		devExt->RecvRealScoCount = 0;
    		devExt->RecvScoNullCount = 0;
    		devExt->RecvAddExtraCount = 0;
    		devExt->TxTotalScoCount = 0;
    		devExt->TxRealScoCount = 0;
    		devExt->TxDiscardSpilthScoCount = 0;
    		devExt->TxReservedScoCount = 0;
    	#endif
    		if (pHci->acl_temp_pending_flag)
    		{
    			Task_Normal_Add_Sco(devExt, (PUINT8) &pHci->pcurrent_connect_device, 0);
    		}
    		else
    		{
    			pHci->auto_packet_select_flag = 0;
    			pHci->sco_need_1_slot_for_acl_flag = 1;
    			pHci->hv1_use_dv_instead_dh1_flag = 0;
    			Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 1);
    			// Set max slot as one slot
    			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT);
                //Combo mode does not support eSco, so we in default create Sco connection
                #if 0
    			if (pScoConnectDevice->esco_packet_type &(BT_PACKET_TYPE_ESCO_EV3 | BT_PACKET_TYPE_ESCO_EV4 | BT_PACKET_TYPE_ESCO_EV5))
    			{
    				if (pTempConnectDevice->current_role == BT_ROLE_MASTER)
    				{
    					if (pScoConnectDevice->esco_amaddr == 0)
    					{
    						Hci_Allocate_Am_address(pHci, &tmpamaddr);
    						pScoConnectDevice->esco_amaddr = tmpamaddr;
    					}
    				}

    				// Select ESCO first
    				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ADD_ESCO);
    			}
    			else
                #endif
    			{
    				// If there is no esco packet type, we select SCO
    				LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ADD_SCO);
    			}
    		}

    		break;
    		case (2): case (4):
    		// Modify synchronours
    		BT_DBGEXT(ZONE_HCI | LEVEL3, "Modify an existed synchronours connection\n");
    		// Store current pointer to connect-device
    		pHci->pcurrent_connect_device = pTempConnectDevice;
    		KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_ESCO);
    		break;
    		case (20):
    		// Modify a sco connection, this is illegal.
    		BT_DBGEXT(ZONE_HCI | LEVEL3, "Modify a sco connection, this is illegal.");
    		pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
    		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    		//Jakio20070724: changed to direct function call
    		// We create a task to send HC command_complete event. In this event, there are some parameters.
    		//eventcode = BT_HCI_EVENT_COMMAND_STATUS;
    		//Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_SEND_EVENT), BT_TASK_PRI_NORMAL, &eventcode, 1);
    		break;
    	}
    }
}
/**************************************************************************
 *   Hci_Command_Accept_Sync_Connection_Request
 *
 *   Descriptions:
 *      Process HCI command: Hci_Command_Accept_Sync_Connection_Request
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      SyncPara: IN, parameter
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Accept_Sync_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 SyncPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PSCO_CONNECT_DEVICE_T pTempScoConnectDevice;
	UINT8 need_role_switch = 0;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_ACCEPT_SYNC_CONNECTION_REQ);

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, SyncPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, SyncPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			if (pTempConnectDevice->pScoConnectDevice == NULL)
			{
				pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
			}
			else
			{
				pTempScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
				pTempScoConnectDevice->transmit_bandwidth = *(PUINT32)(SyncPara + 6);
				pTempScoConnectDevice->receive_bandwidth = *(PUINT32)(SyncPara + 10);
				pTempScoConnectDevice->max_latency = *(PUINT16)(SyncPara + 14);
				pTempScoConnectDevice->voice_setting = *(PUINT16)(SyncPara + 16);
				pTempScoConnectDevice->retransmission_effort = *(SyncPara + 18);
				pTempScoConnectDevice->esco_packet_type = *(PUINT16)(SyncPara + 19);
				if (pTempScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV1)
				{
					pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV1;
				}
				if (pTempScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV2)
				{
					pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV2;
				}
				if (pTempScoConnectDevice->esco_packet_type &BT_PACKET_TYPE_ESCO_HV3)
				{
					pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV3;
				}
				if (pTempScoConnectDevice->using_esco_command_flag)
				{
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);

					Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.

					LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_ESCO_CONN);
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				}
				else
				{
					pTempScoConnectDevice->using_esco_command_flag = 1;
					KeReleaseSpinLock(&pHci->HciLock, oldIrql);
					Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.

					LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN);
					KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
				}
			}
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
		}
	}
	else
	{

		pTempScoConnectDevice = (PSCO_CONNECT_DEVICE_T)pTempConnectDevice->pScoConnectDevice;
		
		pTempScoConnectDevice->transmit_bandwidth = *(PUINT32)(SyncPara + 6);
		pTempScoConnectDevice->receive_bandwidth = *(PUINT32)(SyncPara + 10);
		pTempScoConnectDevice->max_latency = *(PUINT16)(SyncPara + 14);
		pTempScoConnectDevice->voice_setting = *(PUINT16)(SyncPara + 16);
		pTempScoConnectDevice->retransmission_effort = *(SyncPara + 18);
		pTempScoConnectDevice->esco_packet_type = *(PUINT16)(SyncPara + 19);
		if (pTempScoConnectDevice->esco_packet_type & BT_PACKET_TYPE_ESCO_HV1)
			pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV1;
		if (pTempScoConnectDevice->esco_packet_type & BT_PACKET_TYPE_ESCO_HV2)
			pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV2;
		if (pTempScoConnectDevice->esco_packet_type & BT_PACKET_TYPE_ESCO_HV3)
			pTempScoConnectDevice->packet_type |= BT_PACKET_TYPE_HV3;
		
		if (pTempScoConnectDevice->using_esco_command_flag)
		{
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.

			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_ESCO_CONN);
			KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		}
		else
		{
			pTempScoConnectDevice->using_esco_command_flag = 1;
			KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
			Hci_SetMaxSlotForAllDevice(devExt, BT_MAX_SLOT_1_SLOT); // Add by Lewis.wang.

			LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN);
			KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		}
		
		
		pHci->pcurrent_connect_device = pTempConnectDevice;
		pHci->current_connection_handle = pTempConnectDevice->connection_handle;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }

}
/**************************************************************************
 *   Hci_Command_Reject_Sync_Connection_Request
 *
 *   Descriptions:
 *      Process HCI command: Hci_Command_Reject_Sync_Connection_Request
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      SyncPara: IN, parameter
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Reject_Sync_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 SyncPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_REJECT_SYNC_CONNECTION_REQ);
    KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    
	pTempConnectDevice = Hci_Find_Connect_Device_By_BDAddr(pHci, SyncPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_BDAddr(pHci, SyncPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->current_reason_code = *(SyncPara + 6);
			pHci->pcurrent_connect_device = pTempConnectDevice;
			pHci->current_connection_handle = pTempConnectDevice->connection_handle;
		}
	}
	else
	{
		pHci->command_status = BT_HCI_ERROR_UNSPECIFIED_ERROR;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

    if(BT_HCI_STATUS_SUCCESS != pHci->command_status){
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 1);
    }
    else{
    	LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_ESCO_CONN);
    }
}
/**************************************************************************
 *   Hci_Command_Qos_Setup
 *
 *   Descriptions:
 *      Process HCI command: Qos_Setup
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      RolePara: IN, the parameters of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Qos_Setup(PBT_DEVICE_EXT devExt, PUINT8 QosPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;
	LARGE_INTEGER timevalue;
	UINT8 Timer_Flag;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_QOS_SETUP);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, *(PUINT16)QosPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, *(PUINT16)QosPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->qos_flags = *(QosPara + 2);
			pTempConnectDevice->qos_service_type = *(QosPara + 3);
			pTempConnectDevice->qos_token_rate = *(PUINT32)(QosPara + 4);
			pTempConnectDevice->qos_peak_bandwidth = *(PUINT32)(QosPara + 8);
			pTempConnectDevice->qos_latency = *(PUINT32)(QosPara + 12);
			pTempConnectDevice->qos_delay_variation = *(PUINT32)(QosPara + 16);

			#ifdef  BT_INTERNAL_QOS_SETUP
			  pTempConnectDevice->internal_qos_flag=1;
			#endif
		}
	}
	else
	{
		pTempConnectDevice->qos_flags = *(QosPara + 2);
		pTempConnectDevice->qos_service_type = *(QosPara + 3);
		pTempConnectDevice->qos_token_rate = *(PUINT32)(QosPara + 4);
		pTempConnectDevice->qos_peak_bandwidth = *(PUINT32)(QosPara + 8);
		pTempConnectDevice->qos_latency = *(PUINT32)(QosPara + 12);
		pTempConnectDevice->qos_delay_variation = *(PUINT32)(QosPara + 16);

		#ifdef  BT_INTERNAL_QOS_SETUP
			  pTempConnectDevice->internal_qos_flag=1;
		#endif
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	if (pHci->command_status == BT_HCI_STATUS_SUCCESS)
	{
		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_QOS_SETUP);
	}
    else{
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 0);
    }

	tasklet_schedule(&devExt->taskletRcv);

    EXIT_FUNC();
}
/**************************************************************************
 *   Hci_Command_Flow_Specification
 *
 *   Descriptions:
 *      Process HCI command: Flow_Specification
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      RolePara: IN, the parameters of this command.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Flow_Specification(PBT_DEVICE_EXT devExt, PUINT8 QosPara)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

    ENTER_FUNC();
	pHci = (PBT_HCI_T)devExt->pHci;

	pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
	pHci->command_status = BT_HCI_STATUS_SUCCESS;
	pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_FLOW_SPECIFICATION);

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, *(PUINT16)QosPara);
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, *(PUINT16)QosPara);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
		else
		{
			pTempConnectDevice->qos_flags = *(QosPara + 2);
			pTempConnectDevice->qos_flow_direction = *(QosPara + 3);
			pTempConnectDevice->qos_service_type = *(QosPara + 4);
			pTempConnectDevice->qos_token_rate = *(PUINT32)(QosPara + 5);
			pTempConnectDevice->qos_token_bucket_size = *(PUINT32)(QosPara + 9);
			pTempConnectDevice->qos_peak_bandwidth = *(PUINT32)(QosPara + 13);
			pTempConnectDevice->qos_latency = *(PUINT32)(QosPara + 17);
		}
	}
	else
	{
		pTempConnectDevice->qos_flags = *(QosPara + 2);
		pTempConnectDevice->qos_flow_direction = *(QosPara + 3);
		pTempConnectDevice->qos_service_type = *(QosPara + 4);
		pTempConnectDevice->qos_token_rate = *(PUINT32)(QosPara + 5);
		pTempConnectDevice->qos_token_bucket_size = *(PUINT32)(QosPara + 9);
		pTempConnectDevice->qos_peak_bandwidth = *(PUINT32)(QosPara + 13);
		pTempConnectDevice->qos_latency = *(PUINT32)(QosPara + 17);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
    
	if (pHci->command_status == BT_HCI_STATUS_SUCCESS)
	{
		LMP_HC2LM_Command_Process(devExt, pTempConnectDevice, BT_TASK_EVENT_HCI2LMP_FLOW_SPECIFICATION);
	}
    else{
        Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_STATUS, 0);
    }

	tasklet_schedule(&devExt->taskletRcv);
    EXIT_FUNC();
}

/**************************************************************************
 *   Hci_Set_Inquiry_Scan_FHS
 *
 *   Descriptions:
 *      Set inquiry scan FHS packet.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Set_Inquiry_Scan_FHS(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	FHS_PACKET_T fhs_packet;
	PUINT64 ptmplonglong;
	UINT8 tempaccesscode[9];
	UINT8 giac[3] = {0x33,0x8b,0x9e};
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8 pbuf;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Set_Inquiry_Scan_FHS Enter!\n");
	
	AccessCode_Gen(pHci->local_bd_addr, tempaccesscode);

	pbuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pbuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_LOCAL_ACCESS_CODE, 
							pbuf, 9, tempaccesscode);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pbuf - buf));
          //  tempaccesscode, 9);

	BT_DBGEXT(ZONE_HCI | LEVEL1, "The length of FHS_PACKET_T = %d\n", sizeof(FHS_PACKET_T));

	ptmplonglong = (PUINT64)tempaccesscode;
	*ptmplonglong  = *ptmplonglong >> 4;

	RtlCopyMemory(&fhs_packet, ptmplonglong, 8);

#ifdef BT_2_1_SPEC_SUPPORT
	fhs_packet.undefined = 0;
       if(pHci->Extended_Inquiry_Response_Packet_Type!=0xff)
       	{
			fhs_packet.eir = 1;
			BT_DBGEXT(ZONE_HCI | LEVEL1, "===>Set EIR bit of FHS_PACKET_T to 1,packte type=%d!\n",pHci->Extended_Inquiry_Response_Packet_Type);
       	}
	else
		{
			fhs_packet.eir = 0;
		}
	
#else
	fhs_packet.undefined = 0;
#endif

	fhs_packet.sr = 0;
#ifdef BT_ENHANCED_RATE
	fhs_packet.sp = 2; // This is reserved filed for EDR and it should be set as "10" (2). 
#else
	fhs_packet.sp = 0;
#endif
	fhs_packet.uap = pHci->local_bd_addr[3];
	fhs_packet.nap = *(PUINT16)(&pHci->local_bd_addr[4]);
	RtlCopyMemory(fhs_packet.class_of_device, pHci->class_of_device, BT_CLASS_OF_DEVICE_LENGTH);
	fhs_packet.am_addr = 0;
	fhs_packet.page_scan_mode = 0;

	pbuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pbuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_FHS_FOR_INQUIRY_SCAN, 
							pbuf, sizeof(FHS_PACKET_T), (PUINT8)&fhs_packet);
	
	
         //   (PUINT8)&fhs_packet, sizeof(FHS_PACKET_T));
	
	AccessCode_Gen(giac, tempaccesscode);

         //   tempaccesscode, 9);
         pbuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_INQUIRY_SCAN_ACCESS_CODE, 
							pbuf, 9, tempaccesscode);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pbuf - buf));
	
}

/**************************************************************************
 *   Hci_StartPollTimer
 *
 *   Descriptions:
 *      Start a poll timer for this device
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_count: IN, timer count value. Its value must be the times of 5 millseconds.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartPollTimer(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, PCONNECT_DEVICE_T pConnectDevice, UINT16 timer_count)
{
	LARGE_INTEGER timevalue;
	if (pHci == NULL)
	{
		return ;
	}
	if (pConnectDevice == NULL)
	{
		return ;
	}
	
	pConnectDevice->poll_timer_valid = 1;
	pConnectDevice->poll_timer_counter = 1;
	BT_DBGEXT(ZONE_HCI | LEVEL2, "start poll timer\n");
	pHci->start_poll_flag = 1;
	if(timer_count > 0)
	{
		BtDelay(timer_count*1000);
	}

    /* Call the Task_thread_routine */
	Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_POLL_TIMER), 
				BT_TASK_PRI_NORMAL, NULL, 0);
    
	
}
/**************************************************************************
 *   Hci_StopPollTimer
 *
 *   Descriptions:
 *      Stop a poll timer
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopPollTimer(PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice == NULL)
	{
		return ;
	}
	pConnectDevice->poll_timer_valid = 0;
	pConnectDevice->poll_timer_counter = 0;
}


/**************************************************************************
 *   Hci_PollDevStatus
 *
 *   Descriptions:
 *	This routine is modified based Hci_pollTimerRoutine. This routine
 *	is called in task thread, unlike Hci_pollTimerRoutine(which is a timer
 *	routine), promise it is called in passive level since it will access
 *	hardware registers.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Hci_PollDevStatus(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 tmpamaddr[8];
	UINT8 tmpslaveamaddr[3];
	UINT8 tmpslaveindex[3];
	UINT8 i;
	LARGE_INTEGER timevalue;
	#ifdef BT_SERIALIZE_DRIVER
		KIRQL globalIrql;
	#endif
	UINT8 tmplen = 0;
	UINT8 tmpslavelen = 0;
	UINT8 totallistcount = 0;
	UINT8 notimercount = 0;
	UINT8 slavetotallistcount = 0;
	UINT8 slavenotimercount = 0;
	UINT8 Timer_Flag = 0;
	UINT8 slaveindex = 0;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	if (!BtIsPluggedIn(devExt))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL0, "Card is plugged out!!\n"); 
		return;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_PollDevStatus routine enter!\n");

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->poll_timer_valid == 1)
		{
			if (--pConnectDevice->poll_timer_counter == 0)
			{
				Hci_StopPollTimer(pConnectDevice);

				if (pConnectDevice->raw_role == BT_ROLE_SLAVE)
				{
					slaveindex = pConnectDevice->slave_index;
				}
				if (pConnectDevice->state == BT_DEVICE_WAIT_3TPOLL_TIMEOUT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					#ifdef ACCESS_REGISTER_DIRECTLY
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
					#else
						BtUsbHciPollTimerRoutine(devExt);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_Flag = 1;
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else if (pConnectDevice->state == BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					#ifdef ACCESS_REGISTER_DIRECTLY
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
					#else
						BtUsbHciPollTimerRoutine(devExt);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_Flag = 1;
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_6TPOLL_TIMEOUT_FOR_DISCONN;
					BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_PollDevStatus()--Write command inicator: disconnect\n");
					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);
					#ifdef ACCESS_REGISTER_DIRECTLY
						Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
						Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
					#else
						BtUsbHciPollTimerRoutine(devExt);
					#endif
					BtDelay(10);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_Flag = 1;
					tmpamaddr[tmplen] = pConnectDevice->am_addr;
					tmplen++;
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_TEMP_DISCONNECT)
				{
					if (pConnectDevice->tempflag == 1)
					{

						Task_HCI2HC_Send_Remote_NameReq_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
						Timer_Flag = 1;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif



						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_PAGED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;

					pConnectDevice->status = pConnectDevice->current_reason_code;

					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						
						
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);
						

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						
					
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_NOT_ACCEPTED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					
					pConnectDevice->status = pConnectDevice->current_reason_code;

					Hci_StopAllTimer(pConnectDevice);
					
					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}

				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ACCEPTED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					
					pConnectDevice->status = BT_HCI_ERROR_USER_ENDED_CONNECTION;

					Hci_StopAllTimer(pConnectDevice);
					
					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_LMP_TIMEOUT_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						if (pConnectDevice->state != BT_DEVICE_STATE_CONNECTED)
						{
						
							pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
							pConnectDevice->status = BT_HCI_ERROR_LMP_RESPONSE_TIMEOUT;

							Hci_StopAllTimer(pConnectDevice);

							
							Hci_InitLMPMembers(devExt, pConnectDevice);

							Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

							Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif

							

							Hci_InitTxForConnDev(devExt, pConnectDevice);

							Hci_ReleaseScoLink(devExt, pConnectDevice);

							

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

							tmpamaddr[tmplen] = pConnectDevice->am_addr;
							tmplen++;
						}
					}
					else
					{
						if (pConnectDevice->state != BT_DEVICE_STATE_SLAVE_CONNECTED)
						{
						
							pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
							pConnectDevice->status = BT_HCI_ERROR_LMP_RESPONSE_TIMEOUT;

							Hci_StopAllTimer(pConnectDevice);

							Hci_InitLMPMembers(devExt, pConnectDevice);
							
							Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
							
							Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif

							

							Hci_InitTxForConnDev(devExt, pConnectDevice);

							Hci_ReleaseScoLink(devExt, pConnectDevice);

							

						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

							tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
							tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
							tmpslavelen++;

						}
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_AUTH_FAILURE_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_AUTHENTICATION_FAILURE;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
					
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_AUTHENTICATION_FAILURE;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
					
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_PAIR_NOT_ALLOW_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_PAIRING_NOT_ALLOWD;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_PAIRING_NOT_ALLOWD;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ENCRYPTION_NOT_COMP_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;

						pConnectDevice->status = pConnectDevice->current_reason_code;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);
								
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
						pConnectDevice->status = pConnectDevice->current_reason_code;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_SCO_REMOVED)
				{
					if (pConnectDevice->pScoConnectDevice != NULL)
					{

						if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
						{
							Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr);
							((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
						}

						Frag_InitTxScoForConnDev(devExt, pConnectDevice);

						Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 0);
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						Frag_InitTxScoForAllDev(devExt);
					#ifdef BT_SCHEDULER_SUPPORT
						Sched_SendPollFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
					#endif

						
						if (pHci->sco_link_count == 0)
						{
							Hci_Write_One_Byte(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
							pHci->auto_packet_select_flag = 1;
						}

						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}

						{
							LARGE_INTEGER timevalue;


							Timer_Flag = 1;

							
						}
					}
					pConnectDevice->state = pConnectDevice->oldstate;
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ESCO_REMOVED)
				{
					if (pConnectDevice->pScoConnectDevice != NULL)
					{

						if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
						{
							Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr);
							((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
						}

						Frag_InitTxScoForConnDev(devExt, pConnectDevice);

						Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 0);
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						Frag_InitTxScoForAllDev(devExt);
					#ifdef BT_SCHEDULER_SUPPORT
						Sched_SendPollFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
					#endif


						if (pHci->sco_link_count == 0)
						{
							Hci_Write_One_Byte(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
							pHci->auto_packet_select_flag = 1;
						}

						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}

						{
							LARGE_INTEGER timevalue;


							Timer_Flag = 1;

							
						}
					}

					pConnectDevice->state = pConnectDevice->oldstate;
				}
				else
				{
					pConnectDevice->state = BT_DEVICE_6TPOLL_TIMEOUT;

					Hci_StartTimer(pConnectDevice, BT_TIMER_TYPE_LINK_SUPERVISION, (UINT16)(((UINT32)pConnectDevice->link_supervision_timeout * 625) / (1000 * 1000)  + 1)); 
				}
				notimercount++;
			}
		}
		else
		{
			BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_PollDevStatus()--Poll timer: device = 0x%x, count = %d\n", pConnectDevice, pConnectDevice->poll_timer_counter);
			notimercount++;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
		totallistcount++;
	}
	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->poll_timer_valid == 1)
		{
			if (--pConnectDevice->poll_timer_counter == 0)
			{
				slaveindex = pConnectDevice->slave_index;
				if (pConnectDevice->state == BT_DEVICE_WAIT_3TPOLL_TIMEOUT)
				{
					Hci_StopPollTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);
				#ifdef ACCESS_REGISTER_DIRECTLY
					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
					Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif
					#else
						BtUsbHciPollTimerRoutine(devExt);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_Flag = 1;
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else if ((pConnectDevice->state == BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN) || (pConnectDevice->state == BT_DEVICE_STATE_DISCONNECT))
				{
					Hci_StopPollTimer(pConnectDevice);
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					pConnectDevice->status = BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE;
					Hci_StopAllTimer(pConnectDevice);
					Hci_InitLMPMembers(devExt, pConnectDevice);

					Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
					Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif
					Hci_InitTxForConnDev(devExt, pConnectDevice);
					Hci_ReleaseScoLink(devExt, pConnectDevice);
					Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
					Timer_Flag = 1;
					tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
					tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
					tmpslavelen++;
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_PAGED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;

					pConnectDevice->status = pConnectDevice->current_reason_code;

					Hci_StopAllTimer(pConnectDevice);

					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_NOT_ACCEPTED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					
					pConnectDevice->status = pConnectDevice->current_reason_code;

					Hci_StopAllTimer(pConnectDevice);

					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}

				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ACCEPTED_DISCONNECT)
				{
					pConnectDevice->state = BT_DEVICE_STATE_IDLE;
					
					pConnectDevice->status = BT_HCI_ERROR_USER_ENDED_CONNECTION;

					Hci_StopAllTimer(pConnectDevice);

					Hci_InitLMPMembers(devExt, pConnectDevice);

					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else
					{
						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_LMP_TIMEOUT_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
						if (pConnectDevice->state != BT_DEVICE_STATE_CONNECTED)
						{
						
							pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
							pConnectDevice->status = BT_HCI_ERROR_LMP_RESPONSE_TIMEOUT;

							Hci_StopAllTimer(pConnectDevice);

							Hci_InitLMPMembers(devExt, pConnectDevice);

							Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);
							Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif

							

							Hci_InitTxForConnDev(devExt, pConnectDevice);

							Hci_ReleaseScoLink(devExt, pConnectDevice);

							
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

							tmpamaddr[tmplen] = pConnectDevice->am_addr;
							tmplen++;
						}
					}
					else
					{
						if (pConnectDevice->state != BT_DEVICE_STATE_SLAVE_CONNECTED)
						{
						
							pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
							pConnectDevice->status = BT_HCI_ERROR_LMP_RESPONSE_TIMEOUT;

							Hci_StopAllTimer(pConnectDevice);

							Hci_InitLMPMembers(devExt, pConnectDevice);

							Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
							
							Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

						#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
							Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
						#else
							Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
						#endif

							

							Hci_InitTxForConnDev(devExt, pConnectDevice);

							Hci_ReleaseScoLink(devExt, pConnectDevice);

							
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

							tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
							tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
							tmpslavelen++;

						}
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_AUTH_FAILURE_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_AUTHENTICATION_FAILURE;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_AUTHENTICATION_FAILURE;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_PAIR_NOT_ALLOW_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_PAIRING_NOT_ALLOWD;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
							
						pConnectDevice->status = BT_HCI_ERROR_PAIRING_NOT_ALLOWD;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ENCRYPTION_NOT_COMP_DISCONNECT)
				{
					if (pConnectDevice->raw_role == BT_ROLE_MASTER)
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;

						pConnectDevice->status = pConnectDevice->current_reason_code;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);
								
						Hci_Write_One_Byte(devExt, BT_REG_AM_ADDR + pConnectDevice->am_addr, 0);

						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpamaddr[tmplen] = pConnectDevice->am_addr;
						tmplen++;
					}
					else // Slave
					{
					
						pConnectDevice->state = BT_DEVICE_STATE_IDLE;
								
						pConnectDevice->status = pConnectDevice->current_reason_code;

						Hci_StopAllTimer(pConnectDevice);

						Hci_InitLMPMembers(devExt, pConnectDevice);

						Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
						Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);

					#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
						Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
					#else
						Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
					#endif

						

						Hci_InitTxForConnDev(devExt, pConnectDevice);

						Hci_ReleaseScoLink(devExt, pConnectDevice);

						
						
						Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8)&pConnectDevice,0);

						tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
						tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
						tmpslavelen++;
					}
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_SCO_REMOVED)
				{
					if (pConnectDevice->pScoConnectDevice != NULL)
					{

						if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
						{
							Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr);
							((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
						}

						Frag_InitTxScoForConnDev(devExt, pConnectDevice);

						Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 0);
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						Frag_InitTxScoForAllDev(devExt);
					#ifdef BT_SCHEDULER_SUPPORT
						Sched_SendPollFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
					#endif

						
						if (pHci->sco_link_count == 0)
						{
							Hci_Write_One_Byte(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
							pHci->auto_packet_select_flag = 1;
						}

						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}

						{
							LARGE_INTEGER timevalue;


							Timer_Flag=1;

							
						}
					}
					pConnectDevice->state = pConnectDevice->oldstate;
				}
				else if (pConnectDevice->state == BT_DEVICE_STATE_ESCO_REMOVED)
				{
					if (pConnectDevice->pScoConnectDevice != NULL)
					{

						if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
						{
							Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr);
							((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
						}

						Frag_InitTxScoForConnDev(devExt, pConnectDevice);

						Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, (PUINT8)&pConnectDevice, 0);
						Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
						Frag_InitTxScoForAllDev(devExt);
					#ifdef BT_SCHEDULER_SUPPORT
						Sched_SendPollFrame(devExt, (PBT_SCHED_T)devExt->pSched, pConnectDevice);
					#endif

						
						if (pHci->sco_link_count == 0)
						{
							Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
							pHci->auto_packet_select_flag = 1;
						}

						if(pHci->acl_temp_pending_flag == 1)
						{
							pHci->acl_temp_pending_flag = 0;
							Frag_StartQueueForSco(devExt, 0);	
						}

						{
							LARGE_INTEGER timevalue;


							Timer_Flag=1;

							
						}
					}
					pConnectDevice->state = pConnectDevice->oldstate;
				}
				else
				{
				}
				slavenotimercount++;
			}
		}
		else
		{
			slavenotimercount++;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
		slavetotallistcount++;
	}
	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
	if ((notimercount == totallistcount) && (slavenotimercount == slavetotallistcount))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_PollDevStatus()--poll completed\n");
		pHci->start_poll_flag = 0;
	}
	else
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_PollDevStatus()--Set real timer when timeout!\n");
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	for (i = 0; i < tmplen; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_PollDevStatus()--del conn device, index:%d, am_addr:%d\n", i, tmpamaddr[i]);
		Hci_Del_Connect_Device_By_AMAddr(pHci, tmpamaddr[i]);
	}
	for (i = 0; i < tmpslavelen; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Hci_PollDevStatus()--del conn device, index:%d, am_addr:%d\n", i, tmpslaveindex[i]);
		Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(pHci,tmpslaveamaddr[i], tmpslaveindex[i]);
	}
	
	#ifdef BT_AUTO_SEL_PACKET
		if ((tmplen > 0) || (tmpslavelen > 0))
		{
			if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
			{
				devExt->TxAclSendCount = 0;
				devExt->StartRetryCount = 0;
				devExt->EndRetryCount = 0;
				devExt->TxRetryCount = 0;
				
				Hci_ClearMainlyCommandIndicator(devExt);
			}
			else
			{
			}
		}
	#endif

	{
		timevalue.QuadPart = 0; // Just raise the IRQL and process the task immudiatly in another DPC process.
		tasklet_schedule(&devExt->taskletRcv);
	}
}


/**************************************************************************
 *   Hci_StopAllHciTimer
 *
 *   Descriptions:
 *      Stop all timers for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopAllTimer(PCONNECT_DEVICE_T pConnectDevice)
{
	Hci_StopTimer(pConnectDevice);
	Hci_StopPollTimer(pConnectDevice);
	LMP_PDU_StopTimer(pConnectDevice);
}
/**************************************************************************
 *   Hci_InitLMPMembers
 *
 *   Descriptions:
 *      Initialize some members such as lmp_states for LMP module.
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_InitLMPMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice->pScoConnectDevice)
	{
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
		BtDelay(2);
	}
	LMP_ResetMembers(devExt, pConnectDevice);
}
/**************************************************************************
 *   Hci_InitTxForConnDev
 *
 *   Descriptions:
 *      Cancel the sending BD and pending IRP assocaited with the connect device.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_InitTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{

	pConnectDevice->edr_change_flag = 0;
	pConnectDevice->role_switching_flag = 0;
	Frag_InitTxForConnDev(devExt, pConnectDevice);

	return ;

}
/**************************************************************************
 *   Hci_ReleaseScoLink
 *
 *   Descriptions:
 *      If this ACL link has already haven a SCO link, this SCO link should be
 *      disconnected first.
 *   Arguments:
 *		devExt: IN, pointer to device extension of device to start.
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ReleaseScoLink(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	LARGE_INTEGER timevalue;
	UINT8 id;
	PBT_FRAG_T pFrag;
	PBT_FRAG_ELEMENT_T pEle = NULL;
	PBT_FRAG_ELEMENT_T pTmpEle = NULL;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_ReleaseScoLink()--Error: pHci==NULL\n");
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ReleaseScoLink enter!\n");
	if (pConnectDevice->pScoConnectDevice != NULL)
	{
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		Hci_ScoStopTimer((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice));
		((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->status = BT_HCI_ERROR_USER_ENDED_CONNECTION;
		((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->is_in_disconnecting = 0;
		id = ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->index;
		ASSERT(id < BT_TOTAL_SCO_LINK_COUNT);
		BT_DBGEXT(ZONE_HCI | LEVEL3, "SCO index = %d\n", id);
		/*Get pointer to the test */
		pFrag = (PBT_FRAG_T)devExt->pFrag;
		pFrag->RxScoElement[id].currentpos = 0;
		pFrag->RxScoElement[id].currentlen = 0;
		pFrag->RxScoElement[id].totalcount = 0;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);

		KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
		pEle = (PBT_FRAG_ELEMENT_T)QueueGetHead(&pFrag->Sco_FragList);
		while(pEle != NULL)
		{
			if(pEle->pConnectDevice == (ULONG_PTR)pConnectDevice)
			{
				pTmpEle = pEle;
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);

				QueueRemoveEle(&pFrag->Sco_FragList, &pTmpEle->link);
				QueuePutTail(&pFrag->Free_FragList, &pTmpEle->link);
			}
			else
			{
				pEle = (PBT_FRAG_ELEMENT_T)QueueGetNext(&pEle->link);
			}
		}
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		
		if (((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr != 0)
		{
			Hci_Free_Am_address(pHci, ((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr);
			((PSCO_CONNECT_DEVICE_T)(pConnectDevice->pScoConnectDevice))->esco_amaddr = 0;
		}
		LMP_ReleaseSCOMembers(devExt, pConnectDevice);
		Task_HCI2HC_Send_Sco_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
		Hci_Del_Sco_Connect_Device(pHci, (PVOID)pConnectDevice, pConnectDevice->pScoConnectDevice);
		Frag_InitTxScoForAllDev(devExt);
		if (pHci->sco_link_count == 0)
		{
			Hci_Write_DWord_To_3DspReg(devExt, BT_REG_PREPARE_SCO_FLAG, 0);
			pHci->auto_packet_select_flag = 1;
			pHci->hv1_use_dv_instead_dh1_flag = 0;
		}
		if(pHci->acl_temp_pending_flag == 1)
		{
			pHci->acl_temp_pending_flag = 0;
			Frag_StartQueueForSco(devExt, 0);	
		}
		timevalue.QuadPart =  (kOneMillisec);
		tasklet_schedule(&devExt->taskletRcv);
	}
}
/**************************************************************************
 *   Hci_ReleaseAllConnection
 *
 *   Descriptions:
 *      Release all connection when reset.
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *   Return Value:
 *      None
 *************************************************************************/
 #if 0
VOID Hci_ReleaseAllConnection_org(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 tmpamaddr[8];
	UINT8 tmpslaveamaddr[3];
	UINT8 tmpslaveindex[3];
	UINT8 slaveindex = 0;
	UINT8 i;
	UINT8 tmplen = 0;
	UINT8 tmpslavelen = 0;
	LARGE_INTEGER timevalue;
	UINT8 Timer_Flag = 0;

	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->state != BT_DEVICE_STATE_CONNECTED)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a connect, pConnectDevice = 0x%x, state is not in connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_InitLMPMembers(devExt, pConnectDevice);
			#ifdef ACCESS_REGISTER_DIRECTLY
				Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
				Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
				#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
					Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
				#else
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
				#endif
			#else
				BtUsbHciRealeaseAllConnect(devExt);
			#endif
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			tmpamaddr[tmplen] = pConnectDevice->am_addr;
			tmplen++;
		}
		else
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a connect, pConnectDevice = 0x%x, state is in connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_InitLMPMembers(devExt, pConnectDevice);
			#ifdef ACCESS_REGISTER_DIRECTLY
				Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
				Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
				#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
					Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
				#else
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
				#endif
			#else
				BtUsbHciRealeaseAllConnect(devExt);
			#endif
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			tmpamaddr[tmplen] = pConnectDevice->am_addr;
			tmplen++;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		slaveindex = pConnectDevice->slave_index;
		
		if (pConnectDevice->state != BT_DEVICE_STATE_SLAVE_CONNECTED)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a slave connect, pConnectDevice = 0x%x, state is not in slave connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			
			Hci_InitLMPMembers(devExt, pConnectDevice);
			Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
			Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
			#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif
			
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
			tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
			tmpslavelen++;
		}
		else
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a slave connect, pConnectDevice = 0x%x, state is in slave connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			pConnectDevice->current_reason_code = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);	
			
			Hci_InitLMPMembers(devExt, pConnectDevice);

			Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);

			
			Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
			#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif
			
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			tmpslaveamaddr[tmpslavelen] = pConnectDevice->am_addr;
			tmpslaveindex[tmpslavelen] = pConnectDevice->slave_index;
			tmpslavelen++;
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	for (i = 0; i < tmplen; i++)
	{
		Hci_Del_Connect_Device_By_AMAddr(pHci, tmpamaddr[i]);
	}
	for (i = 0; i < tmpslavelen; i++)
	{
		
		Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(pHci,tmpslaveamaddr[i], tmpslaveindex[i]);
	}

	#ifdef BT_AUTO_SEL_PACKET
		if ((tmplen > 0) || (tmpslavelen > 0))
		{
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
			{
				devExt->TxAclSendCount = 0;
				devExt->StartRetryCount = 0;
				devExt->EndRetryCount = 0;
				devExt->TxRetryCount = 0;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Hci_ClearMainlyCommandIndicator(devExt);
			}
			else
			{
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			}
			
		}
	#endif
	if (Timer_Flag == 1)
	{
		timevalue.QuadPart = 0;
		tasklet_schedule(&devExt->TxSendingTimer);
	}
}
#endif
/**************************************************************************
 *   Hci_ReleaseAllConnection
 *
 *   Jakio20080821: rewrite this routine
 *************************************************************************/
VOID Hci_ReleaseAllConnection(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 tmpamaddr[8];
	UINT8 tmpslaveamaddr[3];
	UINT8 tmpslaveindex[3];
	UINT8 slaveindex = 0;
	UINT8 i;
	UINT8 tmplen = 0;
	UINT8 tmpslavelen = 0;
	LARGE_INTEGER timevalue;
	UINT8 Timer_Flag = 0;

	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->state != BT_DEVICE_STATE_CONNECTED)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a connect, pConnectDevice = 0x%x, state is not in connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_InitLMPMembers(devExt, pConnectDevice);
			#ifdef ACCESS_REGISTER_DIRECTLY
				Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
				Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
				#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
					Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
				#else
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
				#endif
			#else
				BtUsbHciRealeaseAllConnect(devExt);
			#endif
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
		}
		else
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a connect, pConnectDevice = 0x%x, state is in connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			Hci_InitLMPMembers(devExt, pConnectDevice);
			#ifdef ACCESS_REGISTER_DIRECTLY
				Hci_Clear_AM_Addr(devExt, pConnectDevice->am_addr);
				Hci_Write_AM_Connection_Indicator(devExt, pHci, BT_AM_CONNECTION_IND_MODE_DEL, pConnectDevice->am_addr);
				#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
					Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
				#else
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
				#endif
			#else
				BtUsbHciRealeaseAllConnect(devExt);
			#endif
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
		}
		
		Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
		Hci_Free_Am_address(pHci, pConnectDevice->am_addr);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pHci->num_device_am--;
		if (pHci->num_device_am == 0)
		{
			pHci->role = BT_ROLE_SLAVE;
		}
		QueuePutTail(&pHci->device_free_list, &pConnectDevice->Link);
		pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_am_list);
		tmplen++;
	}
	pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		slaveindex = pConnectDevice->slave_index;
		
		if (pConnectDevice->state != BT_DEVICE_STATE_SLAVE_CONNECTED)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a slave connect, pConnectDevice = 0x%x, state is not in slave connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			
			Hci_InitLMPMembers(devExt, pConnectDevice);
			Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);
			Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
			#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif
			
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Connection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
			

		}
		else
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Release a slave connect, pConnectDevice = 0x%x, state is in slave connected state\n", pConnectDevice);
			Hci_StopTimer(pConnectDevice);
			pConnectDevice->state = BT_DEVICE_STATE_IDLE;
			pConnectDevice->status = BT_HCI_ERROR_HOST_TIMEOUT;
			pConnectDevice->current_reason_code = BT_HCI_ERROR_HOST_TIMEOUT;
			Hci_StopAllTimer(pConnectDevice);
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);	
			
			Hci_InitLMPMembers(devExt, pConnectDevice);

			Hci_Write_One_Byte(devExt, BT_REG_SLAVE1_AM_ADDR + ((slaveindex + 1) % 2), 0);

			
			Hci_Write_AM_Connection_Indicator(devExt,pHci, BT_AM_CONNECTION_IND_MODE_DEL, slaveindex * 8);
			#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
				Hci_Write_Command_Indicator_Safe(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT, pConnectDevice);
			#else
				Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			#endif
			
			Hci_InitTxForConnDev(devExt, pConnectDevice);
			Hci_ReleaseScoLink(devExt, pConnectDevice);
			Task_HCI2HC_Send_Disconnection_Complete_Event(devExt, (PUINT8) &pConnectDevice, 0);
			Timer_Flag = 1;
		}

		Hci_Free_Conn_Handle(pHci, pConnectDevice->connection_handle);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pHci->num_device_slave--;
		if (pHci->num_device_slave == 0)
			pHci->role = BT_ROLE_MASTER;
		QueuePutTail(&pHci->device_free_list,&pConnectDevice->Link);
		pConnectDevice = (PCONNECT_DEVICE_T)QueuePopHead(&pHci->device_slave_list);
		tmpslavelen++;
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	

	#ifdef BT_AUTO_SEL_PACKET
		if ((tmplen > 0) || (tmpslavelen > 0))
		{
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
			if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
			{
				devExt->TxAclSendCount = 0;
				devExt->StartRetryCount = 0;
				devExt->EndRetryCount = 0;
				devExt->TxRetryCount = 0;
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
				Hci_ClearMainlyCommandIndicator(devExt);
			}
			else
			{
				KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			}
			
		}
	#endif
	if (Timer_Flag == 1)
	{
		timevalue.QuadPart = 0;
		tasklet_schedule(&devExt->taskletRcv);
	}
}


/**************************************************************************
 *   Hci_For_LC_Scan_AddNewItem
 *
 *   Descriptions:
 *      Add a new scan-item for LC scan.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *      pos: IN, the position in scan table.
 *   Return Value:
 *      BT_FOR_LC_SCAN_ADD_NEW_SUCC     success.
 *      BT_FOR_LC_SCAN_ADD_NEW_FULL     full failed.
 *      BT_FOR_LC_SCAN_ADD_NEW_EXIST    already exist in this position failed.
 *      BT_FOR_LC_SCAN_ADD_NEW_ERROR    other error failed.
 *************************************************************************/
UINT8 Hci_For_LC_Scan_AddNewItem(PBT_HCI_T pHci, UINT8 pos)
{
	UINT8 i;
	if (pHci == NULL)
	{
		return (BT_FOR_LC_SCAN_ADD_NEW_ERROR);
	}
	if ((pos >= BT_MAX_SCAN_TABLE_COUNT) || (pHci->scan_option.total_scan_numbers >= BT_MAX_SCAN_TABLE_COUNT))
	{
		return (BT_FOR_LC_SCAN_ADD_NEW_FULL);
	}
	if (pHci->scan_option.scan_bit_map &(0x1 << pos))
	{
		return (BT_FOR_LC_SCAN_ADD_NEW_EXIST);
	}
	pHci->scan_option.scan_bit_map |= (0x1 << pos);
	pHci->scan_option.scan_table[pos].counter = 0;
	pHci->scan_option.scan_table[pos].interval = BT_FOR_LC_SCAN_DEFAULT_INTERVAL;
	pHci->scan_option.scan_table[pos].enable_flag = 1;
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		if (pHci->scan_option.index_table[i] == pos)
		{
			break;
		}
	}
	if (i == pHci->scan_option.total_scan_numbers)
	{
		pHci->scan_option.index_table[pHci->scan_option.total_scan_numbers] = pos;
		if (pHci->scan_option.current_scan_index >= pHci->scan_option.total_scan_numbers)
		{
			pHci->scan_option.current_scan_index = pHci->scan_option.total_scan_numbers;
		}
		pHci->scan_option.total_scan_numbers++;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Display scan option information:\n");
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.total_scan_numbers = %d\n", pHci->scan_option.total_scan_numbers);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_bit_map = %x\n", pHci->scan_option.scan_bit_map);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.current_scan_index = %d\n", pHci->scan_option.current_scan_index);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.index_table:\n");
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.index_table[%d] = %d\n", i, pHci->scan_option.index_table[i]);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_table:\n");
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_table[%d], counter = %d, interval = %d, enable_flag = %d\n", pHci->scan_option.index_table[i], pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].counter, pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].interval, pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].enable_flag);
	}
	return (BT_FOR_LC_SCAN_ADD_NEW_SUCC);
}
/**************************************************************************
 *   Hci_For_LC_Scan_DelOneItem
 *
 *   Descriptions:
 *      Delete an exist item by its position for LC scan.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *      pos: IN, the position in scan table.
 *   Return Value:
 *		None.
 *************************************************************************/
VOID Hci_For_LC_Scan_DelOneItem(PBT_HCI_T pHci, UINT8 pos)
{
	UINT8 i, j;
	if (pHci == NULL)
	{
		return ;
	}
	if (pos >= BT_MAX_SCAN_TABLE_COUNT)
	{
		return ;
	}
	if ((pHci->scan_option.scan_bit_map &(0x1 << pos)) == 0)
	{
		return ;
	}
	pHci->scan_option.scan_bit_map &= (~(0x1 << pos));
	pHci->scan_option.scan_table[pos].counter = 0;
	pHci->scan_option.scan_table[pos].interval = 0;
	pHci->scan_option.scan_table[pos].enable_flag = 0;
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		if (pHci->scan_option.index_table[i] == pos)
		{
			break;
		}
	}
	if (i != pHci->scan_option.total_scan_numbers)
	{
		for (j = i; j < pHci->scan_option.total_scan_numbers; j++)
		{
			pHci->scan_option.index_table[j] = pHci->scan_option.index_table[j + 1];
		}
		if (pHci->scan_option.current_scan_index >= pHci->scan_option.total_scan_numbers)
		{
			pHci->scan_option.current_scan_index = pHci->scan_option.total_scan_numbers;
		}
		if (pHci->scan_option.total_scan_numbers > 0)
		{
			if ((i == pHci->scan_option.total_scan_numbers - 1) && (pHci->scan_option.current_scan_index == i))
			{
				pHci->scan_option.current_scan_index = 0;
			}
			pHci->scan_option.total_scan_numbers--;
		}
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "Display scan option information:\n");
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.total_scan_numbers = %d\n", pHci->scan_option.total_scan_numbers);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_bit_map = %x\n", pHci->scan_option.scan_bit_map);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.current_scan_index = %d\n", pHci->scan_option.current_scan_index);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.index_table:\n");
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.index_table[%d] = %d\n", i, pHci->scan_option.index_table[i]);
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_table:\n");
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "scan_option.scan_table[%d], counter = %d, interval = %d, enable_flag = %d\n", pHci->scan_option.index_table[i], pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].counter, pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].interval, pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].enable_flag);
	}
	return ;
}
/**************************************************************************
 *   Hci_For_LC_Scan_UpdateCounter
 *
 *   Descriptions:
 *      Update counter for each scan item.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *   Return Value:
 *		None.
 *************************************************************************/
VOID Hci_For_LC_Scan_UpdateCounter(PBT_HCI_T pHci)
{
	UINT8 i;
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		if (pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].enable_flag)
		{
			pHci->scan_option.scan_table[pHci->scan_option.index_table[i]].counter++;
		}
	}
}
/**************************************************************************
 *   Hci_For_LC_Scan_PointToNextIndex
 *
 *   Descriptions:
 *      Point to next index.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *   Return Value:
 *		None.
 *************************************************************************/
VOID Hci_For_LC_Scan_PointToNextIndex(PBT_HCI_T pHci)
{
	if ((++pHci->scan_option.current_scan_index) >= pHci->scan_option.total_scan_numbers)
	{
		pHci->scan_option.current_scan_index = 0;
	}
}
/**************************************************************************
 *   Hci_For_LC_Scan_FindChangedItem
 *
 *   Descriptions:
 *      Find changed item according to scan bit map.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *      scan_enable: IN, input scan enable flag.
 *      ppos: IN, the pointer to a position (an array).
 *   Return Value:
 *		counts of the item to be changed.
 *************************************************************************/
UINT8 Hci_For_LC_Scan_FindChangedItem(PBT_HCI_T pHci, UINT8 scan_enable, PUINT8 ppos)
{
	UINT8 i;
	UINT8 counts = 0;
	if (pHci->scan_option.scan_bit_map == scan_enable)
	{
		return 0;
	}
	for (i = 0; i < 8; i++)
	{
		if ((scan_enable &(0x1 << i)) == 0)
		{
			if (pHci->scan_option.scan_bit_map &(0x1 << i))
			{
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Cancel one scan bit, pos = %d\n", i);
				*ppos = (i | 0x80); // if the most bit is 1, it means delete a scan item.
				counts++;
				ppos++;
			}
		}
		else
		{
			if ((pHci->scan_option.scan_bit_map &(0x1 << i)) == 0)
			{
				BT_DBGEXT(ZONE_HCI | LEVEL3, "Add one scan bit, pos = %d\n", i);
				*ppos = i; // if the most bit is 0, it means add a scan item.
				counts++;
				ppos++;
			}
		}
	}
	return (counts);
}
/**************************************************************************
 *   Hci_For_LC_Scan_DoSched
 *
 *   Descriptions:
 *      This function is called every one second. If the counter of one scan
 *      item reaches the interval, driver should send a command to LC (write
 *      register).
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *   Return Value:
 *		None.
 *************************************************************************/
VOID Hci_For_LC_Scan_DoSched(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 index;
	UINT8 i;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->scan_option.total_scan_numbers == 0)
	{
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		return ;
	}
	Hci_For_LC_Scan_UpdateCounter(pHci);
	for (i = 0; i < pHci->scan_option.total_scan_numbers; i++)
	{
		index = pHci->scan_option.index_table[pHci->scan_option.current_scan_index];
		if (pHci->scan_option.scan_table[index].enable_flag)
		{
			if (pHci->scan_option.scan_table[index].counter >= pHci->scan_option.scan_table[index].interval)
			{
				pHci->scan_option.scan_table[index].counter = 0;
				if (index == BT_FOR_LC_SCAN_INDEX_INQUIRY_SCAN)
				{
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_INQUIRY_SCAN_EN_BIT);
				}
				else if (index == BT_FOR_LC_SCAN_INDEX_PAGE_SCAN)
				{
					Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_WRITE_PAGE_SCAN_EN_BIT);
				}
			}
			Hci_For_LC_Scan_PointToNextIndex(pHci);
			break;
		}
		else
		{
			Hci_For_LC_Scan_PointToNextIndex(pHci);
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}
/**************************************************************************
 *   Hci_For_LC_Scan_ChangeScanEnable
 *
 *   Descriptions:
 *      Start to do the real changing for scan-enable parameter.
 *   Arguments:
 *      pHci: IN, pointer to hci module.
 *      scan_enable: IN, input scan enable flag.
 *   Return Value:
 *		counts of the item to be changed.
 *************************************************************************/
VOID Hci_For_LC_Scan_ChangeScanEnable(PBT_HCI_T pHci, UINT8 scan_enable)
{
	UINT8 counts;
	UINT8 pos[8];
	UINT8 i;
	counts = Hci_For_LC_Scan_FindChangedItem(pHci, scan_enable, pos);
	BT_DBGEXT(ZONE_HCI | LEVEL3, "counts = %d\n", counts);
	if (counts == 0)
	{
		return ;
	}
	for (i = 0; i < counts; i++)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "pos[%d] = %d\n", i, pos[i]);
		if (pos[i] &0x80)
		{
			Hci_For_LC_Scan_DelOneItem(pHci, pos[i] &0x7f);
		}
		else
		{
			Hci_For_LC_Scan_AddNewItem(pHci, pos[i]);
		}
	}
}
/**************************************************************************
 *   Hci_StartProtectHciCommandTimer
 *
 *   Descriptions:
 *      Start a timer for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StartProtectHciCommandTimer(PBT_HCI_T pHci, UINT8 type, INT8 timercount)
{
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "HCI StartProtectHciCommandTimer 0x%p type 0x%x\n", pHci, type);
	pHci->protect_hci_command_ack_timer_valid = 1;
	pHci->protect_hci_command_timer_type = type;
	pHci->protect_hci_command_ack_timer_count = timercount;
}
/**************************************************************************
 *   Hci_StopProtectHciCommandTimer
 *
 *   Descriptions:
 *      Start a timer for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_StopProtectHciCommandTimer(PBT_HCI_T pHci)
{
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "HCI StopProtectHciCommandTimer 0x%p type 0x%x\n", pHci, pHci->protect_hci_command_timer_type);
	pHci->protect_hci_command_ack_timer_valid = 0;
	pHci->protect_hci_command_timer_type = 0;
	pHci->protect_hci_command_ack_timer_count = 0;
}
/**************************************************************************
 *   Hci_ProtectHciCommandTimeout
 *
 *   Descriptions:
 *      Start a timer for HCI module
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ProtectHciCommandTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->protect_hci_command_ack_timer_valid)
	{
		if (--pHci->protect_hci_command_ack_timer_count <= 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ProtectHciCommandTimeout work!\n");
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_DSPACK)
			{
				BtProcessDspAckInt(devExt);
			}
			else if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE)
			{
				BtProcessRoleChangeFailInt(devExt);
			}
			else if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE)
			{
				BtProcessModeChangeInt(devExt, BT_MODE_CURRENT_MODE_ACTIVE);
			}
			Hci_StopProtectHciCommandTimer(pHci);
		#ifdef BT_FATAL_ERROR_PROCESS
			FatalError_SetFatal(devExt, FATALERROR_TYPE_PROTECT_TIMER_TIMEOUT_ERROR);
			FatalError_DoFatal(devExt, FATALERROR_TYPE_PROTECT_TIMER_TIMEOUT_ERROR);
		#endif
			return ;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}




/**************************************************************
 *   Hci_ResetHciCommandStatus
 *
 *   Descriptions:
 *     We use this routine to cancel all the pending cmd. It is necessary when our device
 *	convert from d0 to d3 state, this is a common when in combo state
 *   Arguments:
 *      
 *   Return Value:
 *      None
**************************************************************/
VOID Hci_ResetHciCommandStatus(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if (pHci->protect_hci_command_ack_timer_valid)
	{
		{
			BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ResetHciCommandStatus work!\n");
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_DSPACK)
			{
				BtProcessDspAckInt(devExt);
			}
			else if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE)
			{
				BtProcessRoleChangeFailInt(devExt);
			}
			else if (pHci->protect_hci_command_timer_type == BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE)
			{
				BtProcessModeChangeInt(devExt, BT_MODE_CURRENT_MODE_ACTIVE);
			}
			Hci_StopProtectHciCommandTimer(pHci);
		#ifdef BT_FATAL_ERROR_PROCESS
			FatalError_SetFatal(devExt, FATALERROR_TYPE_PROTECT_TIMER_TIMEOUT_ERROR);
			FatalError_DoFatal(devExt, FATALERROR_TYPE_PROTECT_TIMER_TIMEOUT_ERROR);
		#endif
			return ;
		}
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}



/**************************************************************************
 *   Hci_AFHCheckTimeout
 *
 *   Descriptions:
 *      Check AFH timer.
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_AFHCheckTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	if (pHci->afh_timer_valid)
	{
		if (++pHci->afh_timer_count >= BT_MAX_AFH_TIMER_COUNT)
		{
			pHci->afh_timer_count = 0;
			pHci->clock_ready_flag = BT_CLOCK_READY_FLAG_SET_AFH;
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_READ_CLOCK_BIT);
		}
	}
}
/**************************************************************************
 *   Hci_ClassificationCheckTimeout
 *
 *   Descriptions:
 *      Check Classification timer.
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *Changelog: Jakio20070731
 *	this function is not called at now, in future, this will be
 *	called in watchdogitem, which is a work item, so I change
 *	the  task call into direct function call first.
 *************************************************************************/
VOID Hci_ClassificationCheckTimeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->state == BT_DEVICE_STATE_SLAVE_CONNECTED)
		{
			if (pConnectDevice->send_classification_flag)
			{
				if (pConnectDevice->classification_timer_valid)
				{
					if (++pConnectDevice->classification_timer_count >= pConnectDevice->classification_interval)
					{
						BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ClassificationCheckTimeout work!\n");
						pConnectDevice->classification_timer_count = 0;
						#ifdef BT_AFH_ADJUST_MAP_SUPPORT
							if (pHci->upper_set_classification == 1 || Afh_GetClassification(devExt, pHci->classification_channel_map))
							{
							pHci->upper_set_classification = 0;
								KeReleaseSpinLock(&pHci->HciLock, oldIrql);
								LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_SET_CLASSIFICATION);
								KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
							}
						#else
							KeReleaseSpinLock(&pHci->HciLock, oldIrql);
							LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_SET_CLASSIFICATION);
							KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
						#endif
					}
				}
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

/*********************************************************
*Description:
*	Jakio20080304: check slave connection timeout. this is used in rx
*	state machine. 
*********************************************************/
VOID Hci_SlaveConnectionTimeOut(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if(pConnectDevice->connection_timer_valid == 1)
		{
			pConnectDevice->connection_timer_count--;
			if(pConnectDevice->connection_timer_count == 0)
			{
				pConnectDevice->connection_state = 1; //where 1 means timeout or connected
				pConnectDevice->connection_timer_valid = 0;
			}
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

/***************************************************************
*Description:
*	Jakio20080304: change slave connection state. used for rx state machine
***************************************************************/
VOID Hci_SlaveConnected(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;

	if ((devExt == NULL) || (pConnectDevice == NULL))
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	if(pConnectDevice->connection_timer_valid == 1)
	{
		pConnectDevice->connection_timer_count = 2;	
		pConnectDevice->connection_state = 0;
		BT_DBGEXT(ZONE_HCI | LEVEL3, "Pre_process--change device:%2x to connected state\n", pConnectDevice->bd_addr[0]);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}




/**************************************************************************
 *   Hci_SetAFHForAllDevice
 *
 *   Descriptions:
 *      Set AFH for all device.
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type: IN, timer type.
 *      timer_count: IN, timer count value.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_SetAFHForAllDevice(PBT_DEVICE_EXT devExt, UINT32 AfhInstant)
{
	KIRQL oldIrql;
	UINT8 needcommand = 0;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	PCONNECT_DEVICE_T  pTempConnectDevice = NULL;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_SetAFHForAllDevice enter\n");

	devExt->AfhPduCount = 0;
	devExt->AfhWriteCmdFailTimes = 0;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->state == BT_DEVICE_STATE_CONNECTED)
		{
			
			needcommand = 1;
			pTempConnectDevice = pConnectDevice;
			
			KeReleaseSpinLock(&pHci->HciLock, oldIrql);


			Hci_SetAfhForConnectDevice(devExt, pConnectDevice, AfhInstant);
			
			
			

			
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);

	if (needcommand)
	{
		Task_CreateTaskForce((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND), BT_TASK_PRI_NORMAL, (PUINT8)&pTempConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
}

VOID Hci_SetAfhForConnectDevice(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 AfhInstant)
{
	NTSTATUS status;

	if((devExt == NULL) || (pConnectDevice == NULL))
	{
		return;
	}
	if (pConnectDevice->current_role != BT_ROLE_MASTER || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cap_master == 0 || pConnectDevice->lmp_features.byte4.afh_cap_slave == 0)
		return;


#ifdef BT_AFH_DISABLE_WHEN_SNIFF
	if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
	{
		if (pConnectDevice->afh_mode == AFH_ENABLED && pConnectDevice->is_afh_sent_flag == 1)
		{
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cla_master == 1 && pConnectDevice->lmp_features.byte4.afh_cla_slave == 1)
				status = Send_ChannelClassificationReq_PDU(devExt, pConnectDevice, AFH_REPORTING_DISABLED);
			
			status = Send_SetAFH_PDU_WithoutWritingReg(devExt, pConnectDevice, AFH_DISABLED, 0, AfhInstant);	
		}
		break;
	}
#endif
	pConnectDevice->afh_mode = AFH_ENABLED;
	if (pConnectDevice->role_switching_flag == 0 && (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_ACTIVE || pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF))
	{
		status = Send_SetAFH_PDU_WithoutWritingReg(devExt, pConnectDevice, AFH_ENABLED, 0, AfhInstant);
		
		if (pConnectDevice->is_afh_sent_flag == 0)
		{
			pConnectDevice->is_afh_sent_flag = 1; /* not-sending channel_classification_req */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cla_master == 1 && pConnectDevice->lmp_features.byte4.afh_cla_slave == 1)
				status = Send_ChannelClassificationReq_PDU(devExt, pConnectDevice, AFH_REPORTING_ENABLED);
		}
	}
}


/**************************************************************************
 *   Hci_SetMaxSlotForAllDevice
 *
 *   Descriptions:
 *      Send max slot lmp pdu for all device.
 *   Arguments:
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_SetMaxSlotForAllDevice(PBT_DEVICE_EXT devExt, UINT8 maxslot)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	PCONNECT_DEVICE_T pConnectDevice;
	if (devExt == NULL)
	{
		return ;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_SetMaxSlotForAllDevice enter\n");
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		pConnectDevice->max_slot = maxslot;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_MAX_SLOT);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		pConnectDevice->max_slot = maxslot;
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
		LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_CHANGE_MAX_SLOT);
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
}

/**************************************************************************
 *   Hci_ParseRegistryParameter
 *
 *   Descriptions:
 *      Set some value according to registry parameter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ParseRegistryParameter(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci;
	UINT32 enummode = 3;
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
	{
		return ;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ParseRegistryParameter\n");
	if (enummode == 3)
	{
		pHci->lmp_features.byte0.slot_offset = 0;
		pHci->lmp_features.byte0.switchbit = 0;
		pHci->lmp_features.byte4.afh_cap_slave = 0;
		pHci->lmp_features.byte4.afh_cla_slave = 0;
		pHci->lmp_features.byte5.afh_cap_master = 0;
		pHci->lmp_features.byte5.afh_cla_master = 0;
	}
	BT_DBGEXT(ZONE_HCI | LEVEL3, "lmp_feature byte0 = %x, byte4 = %x, byte5 = %x\n", *(PUINT8) &pHci->lmp_features.byte0, *(PUINT8) &pHci->lmp_features.byte4, *(PUINT8) &pHci->lmp_features.byte5);
}

#if 0
/**************************************************************************
 *   Hci_ParseRegistryParameter
 *
 *   Descriptions:
 *      Set some value according to registry parameter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
 VOID Hci_ParseRegistryParameter(PBT_DEVICE_EXT devExt)
 {
	 PBT_HCI_T pHci;
	 
	 pHci = (PBT_HCI_T)devExt->pHci;

	 if (pHci == NULL)
		 return;

	 BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ParseRegistryParameter\n");

	  // BtGetRegistryDword(BT_REGISTRY_PARAMETERS_PATH, 
	 //					 L"RoleSwitchSupport",
     //                   &roleswitchsupport);

	 if (devExt->enummode == 3)
	 {
		 if (devExt->chipID) // chip is V4 version
		 {

			 // In combo mode, some function may not be supportted (such as role/switch, sniff, AFH and so on)
		#ifdef BT_ENHANCED_RATE
			pHci->hci_version = BT_HCI_VERSION_20;
			pHci->hci_revision = 0; // ??
			pHci->lmp_version = LMP_VERSION_V20;
			pHci->lmp_subversion = 0; // ??
		#else
			pHci->hci_version = BT_HCI_VERSION_12;
			pHci->hci_revision = 0; // ??
			pHci->lmp_version = LMP_VERSION_V12;
			pHci->lmp_subversion = 0; // ??
		#endif

			 // Disable roleswitch
			 // pHci->lmp_features.byte0.slot_offset = 0;
			 // pHci->lmp_features.byte0.switchbit = 0;

			 // Disable sniff
			 pHci->lmp_features.byte0.sniff_mode = 0;

			 // Disable AFH
			 // pHci->lmp_features.byte4.afh_cap_slave = 0;
			 // pHci->lmp_features.byte4.afh_cla_slave = 0;
			 // pHci->lmp_features.byte5.afh_cap_master = 0;
			 // pHci->lmp_features.byte5.afh_cla_master = 0;

			 // Disable bluetooth 2.0 feature
			 // pHci->lmp_features.byte4.slot3_enh_acl = 0;
			 // pHci->lmp_features.byte5.slot5_enh_acl = 0;
			 // pHci->lmp_features.byte3.enh_rate_acl_2 = 0;
			 // pHci->lmp_features.byte3.enh_rate_acl_3 = 0;

			 // Driver should enlarge the page timeout value for combo mode.
			 pHci->page_extra_flag = 1;
			 // Driver should enlarge the inquiry length for combo mode.
			 pHci->inquiry_extra_flag = 1;
			 // Driver should use only HV3 packet type in combo mode.
			 pHci->only_use_hv3_flag = 1;
			 // Driver only allow one acl link in combo mode.
			 // pHci->only_allow_one_acl_link_flag = 1;
			 // Driver does not allow slave sco and master coexist.
			 pHci->slave_sco_master_not_coexist_flag = 1;
			 // Driver check DSP state every one second in combo mode. And And if DSP state is not match with the driver
			 // state, driver would correct it
			 pHci->need_coordinate_dsp_state_flag = 1;

			 // Scan enable mask. Set default value as 0 which means our device can be neither inquiried nor paged.
			 // pHci->scan_enable_mask = 0;

			 // Disable power control for V4 co-exist mode
			 pHci->lmp_features.byte2.power_control = 0;
		 }
		 else  // chip is V2 version
		 {
			 // In combo mode, some function may not be supportted (such as role/switch, sniff, AFH and so on)
			 pHci->hci_version = BT_HCI_VERSION_12;
			 pHci->hci_revision = 0; // ??
			 pHci->lmp_version = LMP_VERSION_V12;
			 // pHci->manufacturer_name = MANUFACTURER_3DSP_NAME;
			 pHci->lmp_subversion = 0; // ??

			 // Disable roleswitch
			 pHci->lmp_features.byte0.slot_offset = 0;
			 pHci->lmp_features.byte0.switchbit = 0;

			 // Disable sniff
			 pHci->lmp_features.byte0.sniff_mode = 0;

			 // Disable AFH
			 pHci->lmp_features.byte4.afh_cap_slave = 0;
			 pHci->lmp_features.byte4.afh_cla_slave = 0;
			 pHci->lmp_features.byte5.afh_cap_master = 0;
			 pHci->lmp_features.byte5.afh_cla_master = 0;

			 // Disable bluetooth 2.0 feature
			 pHci->lmp_features.byte4.slot3_enh_acl = 0;
			 pHci->lmp_features.byte5.slot5_enh_acl = 0;
			 pHci->lmp_features.byte3.enh_rate_acl_2 = 0;
			 pHci->lmp_features.byte3.enh_rate_acl_3 = 0;

			 // Driver should enlarge the page timeout value for combo mode.
			 pHci->page_extra_flag = 1;
			 // Driver should enlarge the inquiry length for combo mode.
			 pHci->inquiry_extra_flag = 1;
			 // Driver should use only HV3 packet type in combo mode.
			 pHci->only_use_hv3_flag = 1;
			 // Driver only allow one acl link in combo mode.
			 pHci->only_allow_one_acl_link_flag = 1;
			 // Driver does not allow slave sco and master coexist.
			 pHci->slave_sco_master_not_coexist_flag = 1;
			 // Driver check DSP state every one second in combo mode. And And if DSP state is not match with the driver
			 // state, driver would correct it
			 pHci->need_coordinate_dsp_state_flag = 1;

			 // Scan enable mask. Set default value as 0 which means our device can be neither inquiried nor paged.
			 pHci->scan_enable_mask = 0;
		 }
	 }
	 
	 BT_DBGEXT(ZONE_HCI | LEVEL3, "lmp_feature byte0 = %x, byte4 = %x, byte5 = %x\n", *(PUINT8)&pHci->lmp_features.byte0, *(PUINT8)&pHci->lmp_features.byte4, *(PUINT8)&pHci->lmp_features.byte5);
 }
#endif
 /**************************************************************************
 *   Hci_CoordinateDspState
 *
 *   Descriptions:
 *      Coordinate DSP state so that driver has the same state as DSP's state.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
 VOID Hci_CoordinateDspState(PBT_DEVICE_EXT devExt)
 {
	 KIRQL				oldIrql;
	 PBT_HCI_T		pHci;
	 UINT32 DspState;

	 if (devExt == NULL)
		return;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;
	DspState = Hci_Read_DWord_From_3DspReg(devExt, BT_REG_DEBUG0);
	if (DspState == 1) // Connection state
	{
		if ((pHci->num_device_am == 0) && (pHci->num_device_slave == 0))
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Driver is in standby state but DSP is in connection state, so correct it\n");
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT | BT_HCI_COMMAND_INDICATOR_DISCONNECT_BIT);
			Hci_ClearMainlyCommandIndicator(devExt);
		}
	}
	else if (DspState == 6) // Inquiry state
	{
		if (pHci->is_in_inquiry_flag == 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Driver is in no inquiry state but DSP is in inquiry state, so correct it\n");
			Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_INQUIRY_CANCEL_BIT);
		}
	}
	else
	{
	}
 }

/**************************************************************************
 *   Hci_Command_Read_Voice_Setting
 *
 *   Descriptions:
 *      Read voice setting.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Voice_Setting(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Voice_Setting Enter! 0x%x\n", pHci->voice_setting);

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_VOICE_SETTING);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Read_Voice_Setting
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Voice_Setting.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Voice_Setting(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Voice setting 0x%x\n", pHci->voice_setting);
	*(UINT16 *)dest = pHci->voice_setting;  	// Voice setting
	dest += sizeof(UINT16);
	
	*pOutLen = 3;
}

/**************************************************************************
 *   Hci_Command_Write_Voice_Setting
 *
 *   Descriptions:
 *      Write voice setting.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      VoiceSetting:IN,voice setting.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Voice_Setting(PBT_DEVICE_EXT devExt, UINT16 VoiceSetting)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Write_Voice_Setting Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_VOICE_SETTING);
	}

	pHci->voice_setting = VoiceSetting;
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Command_Read_Num_Of_Supported_Iac
 *
 *   Descriptions:
 *      Read_Num_Of_Supported_Iac.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *  
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Num_Of_Supported_Iac(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Num_Of_Supported_Iac Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_NUMBER_OF_SUPPORTED_IAC);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}


/**************************************************************************
 *   Hci_Response_Read_Num_Of_Supported_Iac
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Num_Of_Supported_Iac
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Num_Of_Supported_Iac(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*dest=BT_MAX_IAC_LAP_NUM;
	dest += sizeof(UINT8);
	
	*pOutLen = 2;
}

/**************************************************************************
 *   Hci_Command_Read_Current_Iac_Lap
 *
 *   Descriptions:
 *      Read_Current_Iac_Lap.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *  
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Current_Iac_Lap(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Current_Iac_Lap Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_CURRENT_IAC_LAP);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}


/**************************************************************************
 *   Hci_Response_Read_Current_Iac_Lap
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Current_Iac_Lap
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Current_Iac_Lap(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*dest=pHci->num_current_iac;
	dest += sizeof(UINT8);

        RtlCopyMemory(dest, (PUINT8) &(pHci->iac_lap[0]), pHci->num_current_iac * BT_EACH_IAC_LAP_COUNT);
	
	*pOutLen = 2 + pHci->num_current_iac * BT_EACH_IAC_LAP_COUNT;
}

/**************************************************************************
 *   Hci_Response_Write_Voice_Setting
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Voice_Setting
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Write_Voice_Setting(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Read_Num_Broadcast_Retransmissions
 *
 *   Descriptions:
 *      Read_Num_Broadcast_Retransmissions.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *  
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Num_Broadcast_Retransmissions Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_NUM_BROADCAST_RETRANSMISSIONS);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}


/**************************************************************************
 *   Hci_Response_Read_Num_Broadcast_Retransmissions
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Num_Broadcast_Retransmissions
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*dest=0; 	// Num_Broadcast_Retransmissions
	dest += sizeof(UINT8);
	
	*pOutLen = 2;
}

/**************************************************************************
 *   Hci_Command_Write_Num_Broadcast_Retransmissions
 *
 *   Descriptions:
 *      Process HCI command: Write_Num_Broadcast_Retransmissions
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Num_Broadcast_Retransmissions: number of Broadcast Retransmissions
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, UINT8 Num_Broadcast_Retransmissions)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Write_Num_Broadcast_Retransmissions Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_WRITE_NUM_BROADCAST_RETRANSMISSIONS);
	}
	
	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Write_Num_Broadcast_Retransmissions
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Write_Num_Broadcast_Retransmissions
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Write_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Read_Default_Link_Policy_Settings
 *
 *   Descriptions:
 *      Process HCI command: Read_Default_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Default_Link_Policy_Settings Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_READ_DEFAULT_LINK_POLICY_SETTINGS);
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Read_Default_Link_Policy_Settings
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Default_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*(UINT16 *)dest=pHci->Default_Link_Policy_Settings;
	dest += sizeof(UINT16);

	*pOutLen = 3;
}

/**************************************************************************
 *   Hci_Command_Write_Default_Link_Policy_Settings
 *
 *   Descriptions:
 *      Process HCI command: Write_Default_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      Default_Link_Policy_Settings:IN,default link policy
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Write_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 Default_Link_Policy_Settings)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Write_Default_Link_Policy_Settings Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_POLICY, BT_HCI_COMMAND_WRITE_DEFAULT_LINK_POLICY_SETTINGS);
	}

	pHci->Default_Link_Policy_Settings=Default_Link_Policy_Settings;

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

}

/**************************************************************************
 *   Hci_Response_Write_Default_Link_Policy_Settings
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Default_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Write_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;

	*dest = pHci->command_status;
	dest += sizeof(UINT8);
	
	*pOutLen = 1;
}

/**************************************************************************
 *   Hci_Command_Read_Failed_Contact_Counter
 *
 *   Descriptions:
 *      Process HCI command: Read_Failed_Contact_Counter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle:IN,link handle.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Failed_Contact_Counter(PBT_DEVICE_EXT devExt,UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Failed_Contact_Counter Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_STATUS_PARAMETERS, BT_HCI_COMMAND_READ_FAILED_CONTACT_COUNTER);
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}


	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Read_Failed_Contact_Counter
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Failed_Contact_Counter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	
	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);

	*(PUINT16)dest = 0;  // Failed_Contact_Counter
	dest += sizeof(UINT16);
	
	*pOutLen = 5;
}

/**************************************************************************
 *   Hci_Command_Reset_Failed_Contact_Counter
 *
 *   Descriptions:
 *      Process HCI command: Write_Default_Link_Policy_Settings
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle:IN,link handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Reset_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Reset_Failed_Contact_Counter Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_STATUS_PARAMETERS, BT_HCI_COMMAND_RESET_FAILED_CONTACT_COUNTER);
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}


	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Reset_Failed_Contact_Counter
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Failed_Contact_Counter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Reset_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	
	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);

	*pOutLen = 3;
}

/**************************************************************************
 *   Hci_Command_Read_Transmit_Power_Level
 *
 *   Descriptions:
 *      Process HCI command: Read_Transmit_Power_Level
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle:IN,link handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_Transmit_Power_Level(PBT_DEVICE_EXT devExt, UINT16 ConnHandle,UINT8 type)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_Transmit_Power_Level Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND, BT_HCI_COMMAND_READ_TRANSMIT_POWER_LEVEL);
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}


	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);
}

/**************************************************************************
 *   Hci_Response_Read_Transmit_Power_Level
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Failed_Contact_Counter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_Transmit_Power_Level(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	
	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);

	*dest = 0;  //Transmit_Power_Level
	dest += sizeof(UINT8);

	*pOutLen = 4;
}

/**************************************************************************
 *   Hci_Command_Read_LMP_Handle
 *
 *   Descriptions:
 *      Process HCI command: Read_LMP_Handle
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      ConnHandle:IN,link handle
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_Command_Read_LMP_Handle(PBT_DEVICE_EXT devExt, UINT16 ConnHandle)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 eventcode;
	PCONNECT_DEVICE_T pTempConnectDevice;

	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_Command_Read_LMP_Handle Enter!\n");

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pHci->command_state != BT_COMMAND_STATE_IDLE)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Another command but prior command not completed. Ignore it!\n");
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		return;
		
	}
	else
	{
		pHci->command_state = BT_COMMAND_STATE_WAIT_COMPLETE;
		pHci->command_status = BT_HCI_STATUS_SUCCESS;
		pHci->current_connection_handle = ConnHandle;
		pHci->current_opcode = BT_MAKE_OPCODE(BT_HCI_COMMAND_OGF_LINK_CONTROL, BT_HCI_COMMAND_READ_LMP_HANDLE);
	}

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	pTempConnectDevice = Hci_Find_Connect_Device_By_ConnHandle(pHci, ConnHandle);

	KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );

	if (pTempConnectDevice == NULL)
	{
		KeReleaseSpinLock ( &pHci->HciLock, oldIrql );
		pTempConnectDevice = Hci_Find_Slave_Connect_Device_By_ConnHandle(pHci, ConnHandle);
		KeAcquireSpinLock ( &pHci->HciLock, &oldIrql );
		if (pTempConnectDevice == NULL)
		{
			pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;
		}
	}
	
	if (pTempConnectDevice->pScoConnectDevice == NULL)
		pHci->command_status = BT_HCI_ERROR_NO_CONNECTION;

	pHci->pcurrent_connect_device = pTempConnectDevice;

	KeReleaseSpinLock ( &pHci->HciLock, oldIrql );

	Task_Normal_Send_Event(devExt, BT_HCI_EVENT_COMMAND_COMPLETE, 1);

}

/**************************************************************************
 *   Hci_Response_Read_LMP_Handle
 *
 *   Descriptions:
 *      Send HC command_complete event to respond to the HCI command Read_Failed_Contact_Counter
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *      buf: IN, destination buffer
 *      pOutLen: IN, output length
 *   Return Value:
 *		None 
 *************************************************************************/
VOID Hci_Response_Read_LMP_Handle(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen)
{
	PBT_HCI_T pHci;
	PUINT8 pStatus;
	UINT8 i;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	
	pStatus=dest;
	*dest = pHci->command_status;
	dest += sizeof(UINT8);

	*(PUINT16)dest = pHci->current_connection_handle;
	dest += sizeof(UINT16);

	if (pHci->command_status != BT_HCI_STATUS_SUCCESS)
	{
		*dest = 0;  //LMP handle
	}
	else
	{
		*dest = 0;
		for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); )
		{
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice == pHci->pcurrent_connect_device)
			{
				*dest = ((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle;
				break;
			}
			
			i++;
		}
		
		for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); )
		{
			if (((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pHci->pcurrent_connect_device)
			{
				*dest = ((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle;
				break;
			}
			
			i++;
		}
	}
	
	dest += sizeof(UINT8);

	RtlZeroMemory(dest, 4);  //for reserved
	*pOutLen = 8;
}

/**************************************************************************
 *   Hci_ClearMainlyCommandIndicator
 *
 *   Descriptions:
 *      Clear mainly command indicator when dsp is in standby state.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      None
 *************************************************************************/
VOID Hci_ClearMainlyCommandIndicator(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T	pHci = NULL;
	pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ClearMainlyCommandIndicator enter\n");

	if(pHci != NULL)
	{
		pHci->role_switching_flag = 0;
	}

	/*
	BT_CLEAR_COMMAND_INDICATOR(devExt, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT | BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_BIT | 
				BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT | BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT |
				BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT | BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH);
	*/
	//Jakio20081106: reset hci role switch flag
	Hci_Clear_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT | BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_BIT | 
				BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT |
				BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT | BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH);
}

#ifdef DBG
	VOID Hci_DebugAmList(PBT_HCI_T pHci)
	{
		KIRQL oldIrql;
		PCONNECT_DEVICE_T pConnectDevice;
		UINT8 i = 0;
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		BT_DBGEXT(ZONE_HCI | LEVEL3, "############################### Total: %d ############################### \n", pHci->num_device_am);
		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
		while (pConnectDevice != NULL)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "\nConnectDevice[%d]: \n", i);
			BT_DBGEXT(ZONE_HCI | LEVEL3, "bd addr[%d]: %02x %02x %02x %02x %02x %02x\n", i, pConnectDevice->bd_addr[0],  \
				pConnectDevice->bd_addr[1], pConnectDevice->bd_addr[2], pConnectDevice->bd_addr[3],  \
				pConnectDevice->bd_addr[4], pConnectDevice->bd_addr[5]);
			BT_DBGEXT(ZONE_HCI | LEVEL3, "am addr[%d] : %d\n", i, pConnectDevice->am_addr);
			BT_DBGEXT(ZONE_HCI | LEVEL3, "connection handle[%d] : %d\n", i, pConnectDevice->connection_handle);
			pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
			i++;
		}
		BT_DBGEXT(ZONE_HCI | LEVEL3, "\n############################### End ############################### \n", pHci->num_device_am);
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	}
#endif


NTSTATUS Hci_Write_Command_Indicator(PBT_DEVICE_EXT devExt, UINT32 value)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT32 ReadIndicator = 0;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tempChar;
	UINT16		length;
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	tempChar = 0x04;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, pEle, 1, &tempChar);

	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_OR_OPERATION,
			BT_REG_HCI_COMMAND_INDICATOR, pEle, 4, (PUINT8)&value);

	tempChar = 0x00;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, pEle, 1, &tempChar);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	Status = UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x4);
	Status = UsbReadFrom3DspReg(devExt, BT_REG_HCI_COMMAND_INDICATOR, &ReadIndicator);
	ReadIndicator |= value;
	Status = UsbWriteTo3DspReg(devExt, BT_REG_HCI_COMMAND_INDICATOR, ReadIndicator);
	Status = UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x0);
#endif
	return Status;
}





NTSTATUS Hci_Clear_Command_Indicator(PBT_DEVICE_EXT devExt, UINT32 value)
{
	NTSTATUS Status = STATUS_SUCCESS;

#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tempChar;
	UINT16		length;
#else
	UINT32 ReadIndicator = 0;
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	tempChar = 0x04;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, pEle, 1, &tempChar);

	value = ~value;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_AND_OPERATION,
			BT_REG_HCI_COMMAND_INDICATOR, pEle, 4, (PUINT8)&value);

	tempChar = 0x00;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION,
			BT_REG_DSP_TX_DISABLE, pEle, 1, &tempChar);
	length = (UINT16)(pEle - buf);
	Status = RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	Status = UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x4);
	Status = UsbReadFrom3DspReg(devExt, BT_REG_HCI_COMMAND_INDICATOR, &ReadIndicator);
	value = ~value;
	ReadIndicator &= value;
	Status = UsbWriteTo3DspReg(devExt, BT_REG_HCI_COMMAND_INDICATOR, ReadIndicator);
	Status = UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0x0);
#endif

	return Status;
}

NTSTATUS Hci_Write_AM_Connection_Indicator(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, UINT32 mode, UINT8 am_addr)
{


	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;


	if (mode == BT_AM_CONNECTION_IND_MODE_ADD)
	{
		pHci->am_connection_indicator |= (UINT16)(0x1 << am_addr);
	}
	else if (mode == BT_AM_CONNECTION_IND_MODE_DEL)
	{
		pHci->am_connection_indicator &= (UINT16)(~(0x1 << am_addr));
	}
	else if (mode == BT_AM_CONNECTION_IND_MODE_CLR)
	{
		pHci->am_connection_indicator = (UINT16)am_addr;
	}


	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 2;
	offset = BT_REG_AM_CONNECTION_INDICATOR;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length,(PUINT8)&pHci->am_connection_indicator);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);

	return STATUS_SUCCESS;
}


NTSTATUS Hci_Write_AM_Connection_Indicator_Asyn(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, UINT32 mode, UINT8 am_addr)
{
	WRITE_DSP_REG_ASYN		RegAsyn;
	NTSTATUS Status = STATUS_SUCCESS;
	RtlZeroMemory(&RegAsyn,8);

	if (mode == BT_AM_CONNECTION_IND_MODE_ADD)
	{
		pHci->am_connection_indicator |= (UINT16)(0x1 << am_addr);
	}
	else if (mode == BT_AM_CONNECTION_IND_MODE_DEL)
	{
		pHci->am_connection_indicator &= (UINT16)(~(0x1 << am_addr));
	}
	else if (mode == BT_AM_CONNECTION_IND_MODE_CLR)
	{
		pHci->am_connection_indicator = (UINT16)am_addr;
	}


	RegAsyn.cmd_type = USB_WRITE_3DSP_REG_ASYN_TYPE;
	RegAsyn.cmd_addr = BT_REG_AM_CONNECTION_INDICATOR;
	RegAsyn.cmd_len  = 2;
	RtlCopyMemory(RegAsyn.cmd_data,(PUINT8)&pHci->am_connection_indicator,2);


	
	
	Status = VendorCmdBurstRegWriteAnyn(devExt, USB_WRITE_3DSP_REGISTER_ASYN, &RegAsyn, 8, VENDOR_CMD_WRITE_CMD_INDCTOR);
	

	return Status;
}
#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
	NTSTATUS Hci_Write_Command_Indicator_Safe(PBT_DEVICE_EXT devExt, UINT32 value, PCONNECT_DEVICE_T pConnectDevice)
	{
		PCONNECT_DEVICE_T pConnectDevice1;
		UINT8 findflag = 0;
		PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;
		pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
		while (pConnectDevice1 != NULL)
		{
			if (pConnectDevice1 != pConnectDevice)
			{
				findflag = 1;
				break;
			}
			pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);
		}
		if (findflag == 0)
		{
			pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
			while (pConnectDevice1 != NULL)
			{
				if (pConnectDevice1 != pConnectDevice)
				{
					findflag = 1;
					break;
				}
				pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);
			}
		}
		if (findflag == 0)
		{
			BT_DBGEXT(ZONE_HCI | LEVEL3, "Write disconnect bit\n");
			Hci_Write_Command_Indicator(devExt, value);

			/*
			BtDelay(5000);
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_MASTER);
			BtDelay(10);
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF);
			BtDelay(10);
			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SCO);
			*/
		}
		return STATUS_SUCCESS;
	}
#endif


NTSTATUS Hci_Write_Command_FlushFifo(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	PCONNECT_DEVICE_T pConnectDevice1;
	UINT8 findflag = 0;
	PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;
	pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice1 != NULL)
	{
		if (pConnectDevice1 != pConnectDevice)
		{
			findflag = 1;
			break;
		}
		pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);
	}
	if (findflag == 0)
	{
		pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
		while (pConnectDevice1 != NULL)
		{
			if (pConnectDevice1 != pConnectDevice)
			{
				findflag = 1;
				break;
			}
			pConnectDevice1 = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice1->Link);
		}
	}
	if (findflag == 0)
	{
		BT_DBGEXT(ZONE_HCI | LEVEL3, "flush fifo\n");

		BtDelay(5000);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_MASTER);
		BtDelay(10);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF);
		BtDelay(10);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SCO);
	}
	return STATUS_SUCCESS;
}


NTSTATUS Hci_Write_AM_Addr(PBT_DEVICE_EXT devExt, UINT8 AmAddrOffset, UINT8 value)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#else
	UINT8 Datastr[8];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 1;
	offset = BT_REG_AM_ADDR + AmAddrOffset;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 8);
	UsbReadFrom3DspRegs(devExt, BT_REG_AM_ADDR, 2, Datastr);
	Datastr[AmAddrOffset] = value;
	UsbWriteTo3DspRegs(devExt, BT_REG_AM_ADDR, 2, Datastr);
#endif
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Clear_AM_Addr(PBT_DEVICE_EXT devExt, UINT8 AmAddrOffset)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#else
	UINT8 Datastr[8];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 1;
	offset = BT_REG_AM_ADDR + AmAddrOffset;
	tmpChar = 0;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, &tmpChar);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 8);
	UsbReadFrom3DspRegs(devExt, BT_REG_AM_ADDR, 2, Datastr);
	Datastr[AmAddrOffset] = 0;
	UsbWriteTo3DspRegs(devExt, BT_REG_AM_ADDR, 2, Datastr);
#endif
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_BD_Address(PBT_DEVICE_EXT devExt, UINT32 offsetAddr, PUINT8 value)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#else
	UINT8 Datastr[8];
#endif


#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = BT_BD_ADDR_LENGTH;
	offset = (UINT16)offsetAddr;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 8);
	UsbReadFrom3DspRegs(devExt, offsetAddr, 1, Datastr + 4);
	RtlCopyMemory(Datastr, value, BT_BD_ADDR_LENGTH);
	UsbWriteTo3DspRegs(devExt, offsetAddr, 2, Datastr);
#endif
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Page_Access_Code(PBT_DEVICE_EXT devExt, PUINT8 pAccessCode)
{
	NTSTATUS Status = STATUS_SUCCESS;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		offset;
#else
	UINT8 Datastr[12];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	length = 9;
	offset = BT_REG_PAGE_ACCESS_CODE;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, offset, pEle, length, pAccessCode);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 12);
	UsbReadFrom3DspRegs(devExt, BT_REG_PAGE_ACCESS_CODE, 3, Datastr);
	RtlCopyMemory(Datastr, pAccessCode, 9);
	UsbWriteTo3DspRegs(devExt, BT_REG_PAGE_ACCESS_CODE, 3, Datastr);
#endif
	return Status;
}

UINT8 Hci_Read_Byte_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT8 DataNum)
{
	UINT8 Datastr[4];
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, AddressOffset, 1, Datastr);
	return Datastr[DataNum];
}


UINT16 Hci_Read_Word_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT8 DataNum)
{
	UINT8 Datastr[4];
	UINT16 TmpValue = 0;
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, AddressOffset, 1, Datastr);
	TmpValue = *(PUINT16)(Datastr + DataNum);
	return TmpValue;
}


NTSTATUS Hci_Write_Word_To_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT32 Offset, UINT16 DataValue)
{
	UINT8 DataStr[4];
	RtlZeroMemory(DataStr, 4);
	if(STATUS_SUCCESS != UsbReadFrom3DspRegs(devExt, AddressOffset, 1, DataStr))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Write_Word_To_3DspReg--read reg fail1\n");
		return STATUS_UNSUCCESSFUL;
	}
	RtlCopyMemory(DataStr + Offset, (PUINT8) &DataValue, 2);
	if(STATUS_SUCCESS != UsbWriteTo3DspRegs(devExt, AddressOffset, 1, DataStr))
	{
		BT_DBGEXT(ZONE_HCI | LEVEL1, "Hci_Write_Word_To_3DspReg--read reg fail2\n");
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

UINT32 Hci_Read_DWord_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset)
{
	UINT8 Datastr[4];
	UINT32 TmpValue = 0;
	RtlZeroMemory(Datastr, 4);
	UsbReadFrom3DspRegs(devExt, AddressOffset, 1, Datastr);
	TmpValue = *(PUINT32)Datastr;
	return TmpValue;
}

NTSTATUS Hci_Write_DWord_To_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT32 Value)
{
	NTSTATUS state;
	state = UsbWriteTo3DspReg(devExt, AddressOffset, Value);
	return state;
}


NTSTATUS Hci_Write_Sniff(PBT_DEVICE_EXT devExt, UINT16 value_DSniff, UINT16 value_Sniff_Attemp, UINT16 value_Sniff_timeout, UINT16 value_TSniff)
{
	UINT8 Datastr[8];
	RtlZeroMemory(Datastr, 8);
	RtlCopyMemory(Datastr, (PUINT8) &value_DSniff, 2);
	RtlCopyMemory(Datastr + 2, (PUINT8) &value_Sniff_Attemp, 2);
	RtlCopyMemory(Datastr + 4, (PUINT8) &value_Sniff_timeout, 2);
	RtlCopyMemory(Datastr + 6, (PUINT8) &value_TSniff, 2);
	UsbWriteTo3DspRegs(devExt, BT_REG_D_SNIFF, 2, Datastr);
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Hold_Mode_Interval(PBT_DEVICE_EXT devExt, UINT8 HodeNumber, UINT16 value)
{
	UINT32 Offset = BT_REG_HOLD_MODE_INTERVAL;
	UINT8 TmpValue[4];
	if (HodeNumber > 2)
	{
		Offset += 4;
	}
	RtlZeroMemory(TmpValue, 4);
	UsbReadFrom3DspRegs(devExt, Offset, 1, TmpValue);
	if (HodeNumber % 2 == 0)
	{
		RtlCopyMemory(TmpValue, (PUINT8) &value, 2);
	}
	else
	{
		RtlCopyMemory(TmpValue + 2, (PUINT8) &value, 2);
	}
	UsbWriteTo3DspRegs(devExt, Offset, 1, TmpValue);
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Sco_Am_Addr(PBT_DEVICE_EXT devExt, UINT8 Offset, UINT8 Value)
{
	UINT8 Datastr[4];
	RtlZeroMemory(Datastr, 4);
	Datastr[1+Offset] = Value;
	UsbWriteTo3DspRegs(devExt, BT_REG_AM_SCO - 1, 1, Datastr);
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_DT_SCO(PBT_DEVICE_EXT devExt, UINT8 D_Sco_Value, UINT8 T_Scp_Value)
{
	UINT8 Datastr[4];
	RtlZeroMemory(Datastr, 4);
	Datastr[0] = D_Sco_Value;
	Datastr[3] = T_Scp_Value;
	UsbWriteTo3DspRegs(devExt, BT_REG_D_SCO, 1, Datastr);
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Encryption_Enable(PBT_DEVICE_EXT devExt, UINT8 Offset, UINT8 Value)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;
#else
	UINT8 DataStr[4];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	
	addr = BT_REG_ENCRYPTION_ENABLE + Offset;
	length = 1;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, &Value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	if (Offset > 3)
	{
		RtlZeroMemory(DataStr, 4);
		UsbReadFrom3DspRegs(devExt, BT_REG_ENCRYPTION_ENABLE + 4, 1, DataStr);
		DataStr[Offset - 4] = Value;
		UsbWriteTo3DspRegs(devExt, BT_REG_ENCRYPTION_ENABLE + 4, 1, DataStr);
	}
	else
	{
		RtlZeroMemory(DataStr, 4);
		UsbReadFrom3DspRegs(devExt, BT_REG_ENCRYPTION_ENABLE, 1, DataStr);
		DataStr[Offset] = Value;
		UsbWriteTo3DspRegs(devExt, BT_REG_ENCRYPTION_ENABLE, 1, DataStr);
	}
#endif
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Byte_In_FourByte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Offset, UINT8 Value)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;
#else
	UINT8 DataStr[4];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	addr = (UINT16)(RegAddr + Offset);
	length = 1;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, &Value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(DataStr, 4);
	UsbReadFrom3DspRegs(devExt, RegAddr, 1, DataStr);
	DataStr[Offset] = Value;
	UsbWriteTo3DspRegs(devExt, RegAddr, 1, DataStr);
#endif
	return STATUS_SUCCESS;
}


NTSTATUS Hci_Write_One_Byte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Value)
{
	NTSTATUS state;
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;
#else
	UINT8 DataStr[4];
	UINT32	Mod, Div;
#endif


#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	addr = (UINT16)(RegAddr);
	length = 1;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, &Value);
	length = (UINT16)(pEle - buf);
	state = RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	Mod = RegAddr%4;
	Div = RegAddr/4;
	RtlZeroMemory(DataStr, 4);
	state = UsbReadFrom3DspRegs(devExt, Mod*4, 1, DataStr);
	DataStr[Div] = Value;
	state = UsbWriteTo3DspRegs(devExt, Mod*4, 1, DataStr);
#endif
	return state;
}

NTSTATUS Hci_Write_One_Word(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT16 Value)
{
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;

	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	addr = (UINT16)(RegAddr);
	length = 2;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, (PUINT8)&Value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);

	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Byte_In_EightByte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Offset, UINT8 Value)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;
#else
	UINT8 DataStr[4];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	addr = (UINT16)(RegAddr + Offset);
	length = 1;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, &Value);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	if (Offset > 3)
	{
		RtlZeroMemory(DataStr, 4);
		UsbReadFrom3DspRegs(devExt, RegAddr + 4, 1, DataStr);
		DataStr[Offset - 4] = Value;
		UsbWriteTo3DspRegs(devExt, RegAddr + 4, 1, DataStr);
	}
	else
	{
		RtlZeroMemory(DataStr, 4);
		UsbReadFrom3DspRegs(devExt, RegAddr, 1, DataStr);
		DataStr[Offset] = Value;
		UsbWriteTo3DspRegs(devExt, RegAddr, 1, DataStr);
	}
#endif
	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_Data_To_FourBytes(PBT_DEVICE_EXT devExt, UINT32 RegAddr, PUINT8 pValue, UINT8 Length)
{
	NTSTATUS state;	
	UINT8 Datastr[4];
	RtlZeroMemory(Datastr, 4);
	RtlCopyMemory(Datastr, pValue, Length);
	state = UsbWriteTo3DspRegs(devExt, RegAddr, 1, Datastr);
	return state;
}

NTSTATUS Hci_Write_Data_To_EightBytes(PBT_DEVICE_EXT devExt, UINT32 RegAddr, PUINT8 pValue, UINT8 Length)
{
#ifdef DSP_REG_WRITE_API
	UINT8	buf[REGAPI_MAX_BUFFER_LENGTH];
	PUINT8		pEle;
	UINT8		tmpChar;
	UINT16		length;
	UINT16		addr;
#else
	UINT8 Datastr[8];
#endif

#ifdef DSP_REG_WRITE_API
	pEle = BTUSB_REGAPI_FIRST_ELE(buf);
	addr = (UINT16)RegAddr;
	length = Length;
	pEle = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, addr, pEle, length, pValue);
	length = (UINT16)(pEle - buf);
	RegAPI_SendTo_MailBox(devExt, buf, length);
#else
	RtlZeroMemory(Datastr, 8);
	RtlCopyMemory(Datastr, pValue, Length);
	UsbWriteTo3DspRegs(devExt, RegAddr, 2, Datastr);
#endif

	return STATUS_SUCCESS;
}

NTSTATUS Hci_Write_About_ESco(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Value_Second_LT_Addr, UINT8 Value_WEsco, UINT8 Value_Esco_Packet_Type)
{
	UINT8 Datastr[4];
	RtlZeroMemory(Datastr, 4);
	Datastr[0] = Value_Second_LT_Addr;
	Datastr[1] = Value_WEsco;
	Datastr[2] = Value_Esco_Packet_Type;
	UsbWriteTo3DspRegs(devExt, RegAddr, 1, Datastr);
	return STATUS_SUCCESS;
}


VOID Hci_Write_Afh_Channel_MapAndNum(PBT_DEVICE_EXT devExt, PUINT8 Afh_Channel_Map, UINT8 Afh_Channel_Num)
{
	UINT8 Datastr[12];
	RtlZeroMemory(Datastr, 12);
	RtlCopyMemory(Datastr, Afh_Channel_Map, 10);
	Datastr[10] = Afh_Channel_Num;
	UsbWriteTo3DspRegs(devExt, BT_REG_AFH_CHANNEL_MAP_SLAVE0, 3, Datastr);
}

VOID Hci_Write_nByte_To_DWord(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Byte0, UINT8 Byte1, UINT8 Byte2, UINT8 Byte3)
{
	UINT8 Datastr[4];
	Datastr[0] = Byte0;
	Datastr[1] = Byte1;
	Datastr[2] = Byte2;
	Datastr[3] = Byte3;
	UsbWriteTo3DspRegs(devExt, RegAddr, 1, Datastr);
}




VOID Hci_ComboStateInit(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_ComboSateInit entered-----\n");
	if(pHci == NULL)
	{
		return;
	}
	pHci->lmp_features.byte0.sniff_mode = 0;
#ifdef BT_RSSI_SUPPORT
	pHci->lmp_features.byte1.rssi = 1;
#else
	pHci->lmp_features.byte1.rssi = 0;
#endif

	#ifdef BT_SNIFF_SUPPORT
	pHci->lmp_features.byte0.sniff_mode = 1;
	if(devExt->RssiWhenSniff == FALSE)
	{
		pHci->lmp_features.byte1.rssi = 0;	
	}
	#endif

	 // Driver should enlarge the page timeout value for combo mode.
	 pHci->page_extra_flag = 1;
	 // Driver should enlarge the inquiry length for combo mode.
	 pHci->inquiry_extra_flag = 1;
	 // Driver should use only HV3 packet type in combo mode.
	 pHci->only_use_hv3_flag = 1;
	 // Driver does not allow slave sco and master coexist.
	 pHci->slave_sco_master_not_coexist_flag = 1;
	 // Disable power control for V4 co-exist mode
	 //pHci->lmp_features.byte2.power_control = 0;

	// Disable esco
	pHci->lmp_features.byte3.ext_sco_link = 0;
}

VOID Hci_SingleStateInit(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T pHci = (PBT_HCI_T)devExt->pHci;

	BT_DBGEXT(ZONE_HCI | LEVEL2, "Hci_SingleStateInit entered-----\n");
	if(pHci == NULL)
	{
		return;
	}
	
#ifdef BT_SNIFF_SUPPORT
	pHci->lmp_features.byte0.sniff_mode = 1;
#else
	pHci->lmp_features.byte0.sniff_mode = 0;
#endif

	 pHci->page_extra_flag = 0;
	 pHci->inquiry_extra_flag = 0;
	 pHci->only_use_hv3_flag = 0;
	 pHci->slave_sco_master_not_coexist_flag = 0;
	 //pHci->lmp_features.byte2.power_control = 1;
}

