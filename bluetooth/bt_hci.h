#ifndef BT_HCI_H
    #define BT_HCI_H

    /**************************************************************************
     * FILENAME:     bt_hci.h
     * CURRENT VERSION: 1.0.0 (optional)
     * CREATE DATE:  2005/08/09
     * PURPOSE:      define some types, macros and functions for HCI module
     *
     * AUTHORS:      jason dong
     *
     * NOTES:        description of constraints when using functions of this file
     *
     **************************************************************************/

    /*
     * REVISION HISTORY
     *   ...
    2005.9.19  We'd better add a member "class_of_device" to be saved before the slave connection is created.
    function declaration is changed as the following:
    NTSTATUS Hci_Add_Slave_Connect_Device(
    PBT_HCI_T pHci,
    UINT8 am_addr,
    PUINT8 bd_addr,
    PUINT8 class_of_device,
    UINT16 packet_type,
    UINT8  repetition_mode,
    UINT8 scan_mode,
    UINT16 clock_offset,
    UINT8  role_switch
    );
    2005.9.28 Add some macros for processing authentication (link key, pin code and so on).
    BT_LINK_KEY_LENGTH
    BT_LMP_EVENT_LINK_KEY_REQ		
    BT_LMP_EVENT_AUTH_FAILURE		
    BT_LMP_EVENT_LINK_KEY_NOTIFICATION
    BT_LMP_EVENT_PIN_CODE_REQ		
    BT_LMP_EVENT_PAIR_NOT_ALLOW		
    Add some members in struct CONNECT_DEVICE_T for processing authentication (link key, pin code and so on).
    key_type_key;
    link_key[BT_LINK_KEY_LENGTH];
    unit_key[BT_LINK_KEY_LENGTH];
    init_key[BT_LINK_KEY_LENGTH];
    pin_code_length;
    pin_code[BT_LINK_KEY_LENGTH];
    Add some declarations of function for processing authentication (link key, pin code and so on).
    Hci_Command_Link_Keq_Request_Reply
    Hci_Response_Link_Keq_Request_Reply
    Hci_Command_Pin_Code_Request_Negative_Reply
    Hci_Response_Pin_Code_Request_Negative_Reply
    Hci_Command_Pin_Code_Request_Reply
    Hci_Response_Pin_Code_Request_Reply
    2005.10.10 Add functions for processing HCI command:BT_HCI_COMMAND_LINK_KEY_REQUEST_NEGATIVE_REPLY
    Hci_Command_Link_Keq_Request_Negative_Reply
    Hci_Response_Link_Keq_Request_Negative_Reply
    2005.10.20 Add some macros for processing encryption
    BT_LMP_EVENT_ENCRYPTION_SUCCESS
    BT_LMP_EVENT_ENCRYPTION_FAILURE
    Add declaration of function for processing encryption
    Hci_Command_Set_Connection_Encryption
    2005.10.24 Add some macros for processing encryption
    BT_LMP_EVENT_ENCRYPTION_NOT_COMP
    2005.10.24 Add a flag in struct CONNECT_DEVICE_T. If the connection is in encryption process,
    this flag is set as 1.Otherwise this flag is set as 0. When driver set a connection,
    it should initialize this flag as 0.
    typedef struct _CONNECT_DEVICE
    {
    ...
    UINT8 is_in_encryption_process;
    }
    2005.10.26 Add some macros for processing authentication after connection is established.
    BT_LMP_EVENT_AUTH_COMP_SUCCESS
    BT_LMP_EVENT_AUTH_COMP_FAILURE		
    Add some declarations of function for processing authentication (link key, pin code and so on).
    Hci_Command_Authentication_Requested
    2005.11.11 Add a structure to express the format of FHS packet.
    // define FHS packet
    typedef struct _FHS_PACKET
    {
    struct
    {
    UINT64 parity_bits  : 34;
    UINT64 lap		   : 24;
    UINT64 undefined    : 2;
    UINT64 sr		   : 2;
    UINT64 sp		   : 2;
    };
    UINT8 uap;
    UINT16 nap;
    UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH];
    struct
    {
    UINT32 am_addr		   : 3;
    UINT32 clk27_2		   : 26;
    UINT32 page_scan_mode   : 3;
    };
    } FHS_PACKET_T, *PFHS_PACKET_T;
    2005.11.18 Add some macros about scanning (inquiry scan or page scan) for LC.
    BT_MAX_SCAN_TABLE_COUNT
    BT_FOR_LC_SCAN_DEFAULT_INTERVAL
    BT_FOR_LC_SCAN_ADD_NEW_SUCC
    BT_FOR_LC_SCAN_ADD_NEW_FULL
    BT_FOR_LC_SCAN_ADD_NEW_EXIST
    BT_FOR_LC_SCAN_ADD_NEW_ERROR
    BT_FOR_LC_SCAN_TYPE_INQUIRY_SCAN
    BT_FOR_LC_SCAN_TYPE_PAGE_SCAN
    BT_FOR_LC_SCAN_INDEX_INQUIRY_SCAN
    BT_FOR_LC_SCAN_INDEX_PAGE_SCAN
    Add some structures about scanning (inquiry scan or page scan) for LC.
    typedef struct _SCAN_TABLE
    {
    UINT8 counter;
    UINT8 interval;
    UINT8 enable_flag;
    } SCAN_TABLE_T, *PSCAN_TABLE_T;

    typedef struct _SCAN_OPTION
    {
    SCAN_TABLE_T scan_table[BT_MAX_SCAN_TABLE_COUNT];
    UINT8 index_table[BT_MAX_SCAN_TABLE_COUNT];
    UINT8 scan_bit_map;
    UINT8 current_scan_index;
    UINT8 total_scan_numbers;
    } SCAN_OPTION_T, *PSCAN_OPTION_T;
    Add a member in structure BT_HCI_T about scanning (inquiry scan or page scan) for LC.
    typedef struct _BT_HCI
    {
    ...
    // For each scan(inquiry scan or page scan).
    SCAN_OPTION_T scan_option;
    }BT_HCI_T, *PBT_HCI_T;
    Add some declarations of function about scanning (inquiry scan or page scan) for LC.
    Hci_For_LC_Scan_AddNewItem
    Hci_For_LC_Scan_DelOneItem
    Hci_For_LC_Scan_UpdateCounter
    Hci_For_LC_Scan_PointToNextIndex
    Hci_For_LC_Scan_FindChangedItem
    Hci_For_LC_Scan_DoSched
    Hci_For_LC_Scan_ChangeScanEnable
    2005.12.7  Change the format of struct HCBB_PAYLOAD_HEADER_T. We define one word payload header before.
    But now we need one double word for payload header. So we change this structure.
    typedef struct _HCBB_PAYLOAD_HEADER
    {
    ...
    UINT32 reserved: 16;
    }
    Add two functions' declaration.
    Hci_Command_Change_Connection_Packet_Type
    Hci_InitTxForConnDev
    2005.12.15 Add some macros for SCO.
    BT_PACKET_TYPE_DM1
    BT_PACKET_TYPE_DH1
    BT_PACKET_TYPE_DM3
    BT_PACKET_TYPE_DH3
    BT_PACKET_TYPE_DM5
    BT_PACKET_TYPE_DH5
    BT_PACKET_TYPE_HV1
    BT_PACKET_TYPE_HV2
    BT_PACKET_TYPE_HV3
    BT_LMP_EVENT_SCO_LINK_REQ
    BT_LMP_EVENT_SCO_LMP_TIMEOUT
    BT_LMP_EVENT_SCO_DETACH
    BT_LMP_EVENT_SCO_CONNECTED
    BT_LMP_EVENT_SCO_REMOVED
    BT_CLOCK_READY_FLAG_SCO_REQ
    BT_CLOCK_READY_FLAG_OTHER
    Add a struct for SCO.
    typedef struct _SCO_CONNECT_DEVICE
    {
    ...
    }SCO_CONNECT_DEVICE_T, *PSCO_CONNECT_DEVICE_T;
    Add some members in struct _CONNECT_DEVICE for SCO.
    typedef struct _CONNECT_DEVICE
    {
    ...
    // Link SCO connect device
    PVOID pScoConnectDevice;

    // Flag for determining which link (ACL link or SCO link) owns this message(Hci command or event).
    UINT8 acl_or_sco_flag;

    // Flag for determine what should we do next when driver receive clock ready interrupt.
    UINT8 clock_ready_flag;
    }CONNECT_DEVICE_T, *PCONNECT_DEVICE_T;
    Add some members in struct _BT_HCI for SCO.
    typedef struct _BT_HCI
    {
    ...
    SCO_CONNECT_DEVICE_T sco_device_all[BT_MAX_DEVICE_NUM];
    // free device list
    BT_LIST_ENTRY_T  sco_device_free_list;
    ...
    }BT_HCI_T, *PBT_HCI_T;
    Add some functions' declaration for SCO.
    Hci_Add_Sco_Connect_Device,Hci_Del_Sco_Connect_Device, Hci_Command_Add_Sco_Connection, Hci_ReleaseScoLink,
    Hci_ScoStartTimer, Hci_ScoStopTimer
    2005.12.16  Add some members in struct _SCO_CONNECT_DEVICE for SCO
    typedef struct _SCO_CONNECT_DEVICE
    {
    ...
    UINT8 timing_control_flags;
    UINT8 D_sco;
    UINT8 T_sco;
    UINT8 air_mode;
    UINT32 clock;
    ...
    }SCO_CONNECT_DEVICE_T, *PSCO_CONNECT_DEVICE_T;
    2005.12.19  Add a member "switch_role" to save the role value from Hci command "Accept_Connection_Request"
    typedef struct _BT_HCI
    {
    ...
    // For Accept_Connection_Request HCI command, saved the Role value.
    UINT8 switch_role;
    }BT_HCI_T, *PBT_HCI_T;
    2005.12.23  Because we added the source codes for SCO. Some macros should be modifed
    BT_MAX_SCO_DATA_PACKET_LEN (from 0 to 512)
    BT_TOTAL_NUM_SCO_DATA_PACKET (from 0 t0 3)
    Add a new macros that expresses the total number of data packet (BT_TOTAL_NUM_ACL_DATA_PACKET
    + BT_TOTAL_NUM_SCO_DATA_PACKET)
    BT_TOTAL_NUM_DATA_PACKET
    Change the count of "connection_handle member" and "num_of_complete_packets member" in
    struct _COMPLETE_PACKETS (from 3 to 6)
    Add a new member "sco_flow_control_enable" in struct _BT_HCI
    Add some functions for hci command "write_sco_flow_control_enable" and "read_sco_flow_control_enable".
    function Hci_Command_Write_Sco_Flow_Control_Enable, Hci_Response_Write_Sco_Flow_Control_Enable
    Hci_Command_Read_Sco_Flow_Control_Enable, Hci_Response_Read_Sco_Flow_Control_Enable
    2005.12.28  Driver delete some registers because some information is expressed by FHS packet. It does not need these
    register any longer. So driver add some vars to save some parameters sent by host controller. These
    parameters is saved into the registers before.
    typedef struct _BT_HCI
    {
    ...
    // for read/write Page_Scan_Activity
    UINT16 page_scan_interval;
    UINT16 page_scan_window;

    // for read/write Inquiry_Scan_Activity
    UINT16 inquiry_scan_interval;
    UINT16 inquiry_scan_window;
    ...
    }

    Driver should complete sending the frame in cache or hardware buffer before SCO link is set up
    because there may be some frame which packet type is DM3, DH3, DM5 or DH5. However driver should
    only send DM1 frame after SCO link is setup. So driver setup some vars to control this process.
    typedef struct _BT_HCI
    {
    ...
    // If this var is set as "1", packet tye can be up or down according to the performance of channel.
    //  Otherwise packet type is a fixed value and can not be up or down.
    UINT8 auto_packet_select_flag;

    // If this var is set as "1", it means that the next IRP would be pended. After a moment, if this
    //  var is set as "0", the next IRP would be reprocessed.
    UINT8 acl_temp_pending_flag;

    //  Sco link count.
    UINT16 sco_link_count;
    ...
    }

    typedef struct _CONNECT_DEVICE
    {
    ...
    UINT8 current_packet_type;
    }

    typedef struct _SCO_CONNECT_DEVICE
    {
    ...
    UINT8 current_packet_type;
    }
    2005.12.29 Change the value of macro "BT_MAX_SCO_DATA_PACKET_LEN" from 512 to 480
    Add four new macros for air mode type
    BT_AIRMODE_MU_LAW
    BT_AIRMODE_A_LAW
    BT_AIRMODE_CVSD
    BT_AIRMODE_TRANS
    2005.12.30 Add a new member "tx_retry_count" in struct _HCBB_PAYLOAD_HEADER for auto-select-packet-type function
    typedef struct _HCBB_PAYLOAD_HEADER
    {
    ...
    UINT32 tx_retry_count: 16;
    }
    2005.12.31 Add a member "lmp_ext_states" in struct __CONNECT_DEVICE for LMP module. This member is used to express
    the extra state mainly for SCO link. (lmp_states is mainly for ACL link).
    typedef struct _CONNECT_DEVICE
    {
    ...
    UINT16 lmp_ext_states;
    ...
    }
    2006.1.9 Add a new struct _HCI_SCO_HEADER
    2006.1.13 Change values of some macros
    BT_MAX_SCO_DATA_PACKET_LEN (from 160 to 240)
    BT_TOTAL_NUM_SCO_DATA_PACKET (from 3 to 2)
    Add a new macro BT_MAX_SCO_DEVICE_NUM (value is 3) to save the total sco connect device
    We add a member "index" in struct _SCO_CONNECT_DEVICE. This member is used to save the index (from 0 to 2) of each SCO connect device. When
    driver receives a SCO frame, it should look for its sco connect device according to the am address first, then driver should save this
    frame into the sco rx cache whose index is the index saved into the sco connect device.
    Change the numbers (from 64 to 3) of "sco_device_all" member in struct _BT_HCI.
    2006.1.20 Driver add a member "rx_sco_con_null_count" in struct _CONNECT_DEVICE. This is used to save the bytes
    that driver receives continous sco null frame. If this value is greater than 120000 (about 15 seconds),
    driver consider it could not receive any sco frame in 15 seconds. So driver would disconnect all the link
    (including ACL and SCO).
    Driver add a member "state" in _SCO_CONNECT_DEVICE. This member is used to save the current state of
    the sco connect device. Now the state has two state: IDLE and CONNECTED.
    2006.3.7   Add one member "encryption_key_size" in structure _CONNECT_DEVICE.
    2006.3.20  Add one member "rssi" in structure _CONNECT_DEVICE.
    Add two members "protect_hci_command_ack_timer_valid" and "protect_hci_command_ack_timer_count" in structure _BT_HCI
    Add declarations for some functions:
    Hci_Response_Read_Rssi
    Hci_Command_Is_Discard_Command
    Hci_Command_Discard_Command
    Hci_StartProtectHciCommandTimer
    Hci_StopProtectHciCommandTimer
    Hci_ProtectHciCommandTimeout
    2006.3.22  Add some macros for Bluetooth spec 2.0
    Add two members: "flush_flag" and "flush_timeout" in struct _CONNECT_DEVICE for flush
    typedef struct _CONNECT_DEVICE
    {
    ...
    // When Flush command is received, this flag is set as 1
    UINT8 flush_flag;

    // Flush Timeout
    UINT16 flush_timeout;
    }CONNECT_DEVICE_T, *PCONNECT_DEVICE_T;
    new six functions' declaration are added:
    Hci_Command_Flush
    Hci_Response_Flush
    Hci_Command_Write_Automatic_Flush_Timeout
    Hci_Response_Write_Automatic_Flush_Timeout
    Hci_Command_Read_Automatic_Flush_Timeout
    Hci_Response_Read_Automatic_Flush_Timeout
    2006.3.28  Add some codes for hode mode and sniff mode
    Add some event
    BT_LMP_EVENT_MODE_CHANGE
    BT_CLOCK_READY_FLAG_HODE_MODE
    BT_CLOCK_READY_FLAG_SNIFF_MODE
    BT_CLOCK_READY_FLAG_OTHER
    BT_MODE_CURRENT_MODE_ACTIVE
    BT_MODE_CURRENT_MODE_HOLD
    BT_MODE_CURRENT_MODE_SNIFF
    Add some members in struct _CONNECT_DEVICE
    typedef struct _CONNECT_DEVICE
    {
    ...
    // If this connection is in hold mode, this value is set as 1.
    // If this connection is in sniff mode, this value is set as 2.
    //Otherwise(active), this value is set as 0.
    UINT8 mode_current_mode;
    // Hold_Mode_Max_Interval
    UINT16 hold_mode_max_interval;
    // Hold_Mode_Min_Interval
    UINT16 hold_mode_min_interval;
    // Used hold interval
    UINT16 hold_mode_interval;

    // Sniff_Max_Interval
    UINT16 sniff_max_interval;
    // Sniff_Min_Interval
    UINT16 sniff_min_interval;
    // Sniff_Attempt
    UINT16 sniff_attempt;
    // Sniff_Timeout
    UINT16 sniff_timeout;
    // Used sniff interval
    UINT16 sniff_mode_interval;
    }CONNECT_DEVICE_T, *PCONNECT_DEVICE_T;
    Add some functions' declaration:
    Hci_Command_Hold_Mode, Hci_Command_Sniff_Mode, Hci_Command_Exit_Sniff_Mode

    Add some codes for stored link key
    Add one member "num_keys_written" in struct _BT_HCI
    Add some functions' declaration:
    Hci_Command_Read_Stored_Link_Key, Hci_Response_Read_Stored_Link_Key, Hci_Command_Write_Stored_Link_Key,
    Hci_Response_Write_Stored_Link_Key, Hci_Command_Delete_Stored_Link_Key, Hci_Response_Delete_Stored_Link_Key
    2006.4.7  Add some codes for AFH
    Add new function: Hci_Command_Write_AFH_Channel_Assessment_Mode, Hci_Response_Write_AFH_Channel_Assessment_Mode
    Hci_Command_Read_AFH_Channel_Assessment_Mode, Hci_Response_Read_AFH_Channel_Assessment_Mode
    Hci_Command_Set_AFH_Host_Channel_Classification, Hci_Response_Set_AFH_Host_Channel_Classification
    Hci_Command_Read_AFH_Channel_Map, Hci_Response_Read_AFH_Channel_Map, Hci_AFHCheckTimeout
    Hci_SetAFHForAllDevice

    Add some codes for Hode mode activity
    Add new function: Hci_Command_Write_Hold_Mode_Activity, Hci_Response_Write_Hold_Mode_Activity
    Hci_Command_Read_Hold_Mode_Activity, Hci_Response_Read_Hold_Mode_Activity

    Add some codes for others hci 1.2 command
    Add new function: Hci_Command_Read_Local_Supported_Commands, Hci_Response_Read_Local_Supported_Commands
    Hci_Command_Write_Inquiry_Mode, Hci_Response_Write_Inquiry_Mode, Hci_Command_Read_Inquiry_Mode
    Hci_Response_Read_Inquiry_Mode, Hci_Command_Write_Inquiry_Scan_Type, Hci_Response_Write_Inquiry_Scan_Type
    Hci_Command_Read_Inquiry_Scan_Type, Hci_Response_Read_Inquiry_Scan_Type, Hci_Command_Write_Page_Scan_Type
    Hci_Response_Write_Page_Scan_Type, Hci_Command_Read_Page_Scan_Type, Hci_Response_Read_Page_Scan_Type

    Add some codes for inquiry with rssi. Add new function: Hci_Response_Inquiry_Result_With_Rssi

    Add one parameter for function Hci_InitLMPMembers.

    Expand local support feature. Add struct _LMP_FEATURES_BYTE3, _LMP_FEATURES_BYTE4, _LMP_FEATURES_BYTE5
    Add three members in struct _LMP_FEATURES: byte3, byte4, byte5

    Add some macros: BT_MAX_CHANNEL_MAP_NUM, BT_MAX_AFH_TIMER_COUNT, BT_CLOCK_READY_FLAG_SET_AFH, BT_INQUIRYMODE_STANDED
    BT_INQUIRYMODE_RESULT_WITH_RSSI, BT_INQUIRYSCAN_TYPE_STANDED, BT_INQUIRYSCAN_TYPE_INTERLACED
    BT_PAGESCAN_TYPE_STANDED, BT_PAGESCAN_TYPE_INTERLACED, BT_CONTROLLER_ASSESSMENT_DISABLED,BT_CONTROLLER_ASSESSMENT_ENABLED
    2006.4.18 When our card is connected with both an ACL link and an SCO link, it should need the other
    card (slave) send data with only 1 slot (DM1 or DH1).
    Add the declration of function Hci_SetMaxSlotForAllDevice.
    Add some new macros:
    BT_MAX_SLOT_BY_FEATURE
    BT_MAX_SLOT_1_SLOT
    BT_MAX_SLOT_3_SLOT
    BT_MAX_SLOT_5_SLOT
    Add one member "max_slot" in struct _CONNECT_DEVICE
    2006.4.24 Add some codes for specific vendor ogf. Now this ogf is used for reading IVT encryption key from hci to
    IVT program.
    Add macros:
    MANUFACTURER_3DSP_NAME
    BT_HCI_COMMAND_OGF_VENDOR_SPECIFIC
    BT_HCI_COMMAND_READ_ENCRYPTION_KEY_FROM_IVT
    BT_HCI_COMMAND_WRITE_ENCRYPTION_KEY_FROM_IVT
    BT_IVT_ENCRYPTION_KEY_LENGTH
    Add one member "encryption_key_from_ivt" in struct _BT_HCI.
    Add the declaration of functions: Hci_Command_Read_IVT_Encryption_Key, Hci_Response_Read_IVT_Encryption_Key
    2006.5.11 Add some macro for role/switch
    BT_LMP_EVENT_ROLE_SWITCH_FAIL
    BT_DEVICE_STATE_WAIT_ROLE_SWITCH
    BT_CLOCK_READY_FLAG_ROLE_SWITCH
    Add some members in struct _CONNECT_DEVICE
    role_switching_flag
    role_switching_role
    Add the declaration of function Hci_Command_Switch_Role for role/switch
    2006.5.18 Add some type macro for protect hci command timer
    BT_PROTECT_HCI_COMMAND_TYPE_DSPACK
    BT_PROTECT_HCI_COMMAND_TYPE_READCLK
    BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE
    BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE
    Add one member "protect_hci_command_timer_type" in struct _BT_HCI for protect hci commmand timer type
    Add two members in struct _CONNECT_DEVICE
    am_addr_for_fhs
    tx_count_frame
    Add one parameter: type for function Hci_StartProtectHciCommandTimer.
    2006.5.24 Add two members in struct _HCI
    acl_force_1_slot
    sco_need_1_slot_for_acl_flag
    These two members is used to force sending one slot frame.
    2006.5.31 Add one member "tx_power" in struct CONNECT_DEVICE_T for power control
    Add one member "am_connection_indicator" in struct BT_HCI_T to write am connection indicator register
    instead of writing am connection indicator directly.
    2006.6.5  Add two members "aux1_instead_dh1_flag" and "hv1_use_dv_instead_dh1_flag" for DV and AUX1
    2006.6.6  Add one member "l2cap_flow_control_flag" in struct CONNECT_DEVICE_T for l2cap flow control
    2006.6.12 Add some macros for ESCO
    BT_ESCO_PACKET_NULL
    BT_ESCO_PACKET_POLL
    BT_ESCO_PACKET_EV3
    BT_ESCO_PACKET_EV4
    BT_ESCO_PACKET_EV5
    BT_LINK_TYPE_ESCO
    BT_PACKET_TYPE_ESCO_HV1
    BT_PACKET_TYPE_ESCO_HV2
    BT_PACKET_TYPE_ESCO_HV3
    BT_PACKET_TYPE_ESCO_EV3
    BT_PACKET_TYPE_ESCO_EV4
    BT_PACKET_TYPE_ESCO_EV5
    BT_PACKET_TYPE_ESCO_2EV3
    BT_PACKET_TYPE_ESCO_3EV3
    BT_PACKET_TYPE_ESCO_2EV5
    BT_PACKET_TYPE_ESCO_3EV5
    BT_LMP_EVENT_ESCO_CONNECTED
    BT_LMP_EVENT_ESCO_CHANGED
    BT_LMP_EVENT_ESCO_CHANGE_FAIL
    BT_LMP_EVENT_ESCO_REMOVED
    BT_LMP_EVENT_ESCO_LINK_REQ
    Add some members in struct SCO_CONNECT_DEVICE_T for ESCO
    using_esco_command_flag
    transmit_bandwidth
    receive_bandwidth
    max_latency
    voice_setting
    retransmission_effort
    esco_packet_type
    transmission_interval
    retransmission_window
    rx_packet_length
    tx_packet_length
    content_format
    Add some functions for ESCO
    Hci_Add_eSco_Connect_Device
    Hci_Modify_eSco_Connect_Device
    Hci_Command_Setup_Sync_Connection
    Hci_Command_Accept_Sync_Connection_Request
    Hci_Command_Reject_Sync_Connection_Request
    2006.6.14 Add one macro BT_LMP_EVENT_CHANGE_TX_MAX_SLOT for receiving the max slot from remote device.
    Add one member tx_max_slot for saving the max slot from remote device.
    2006.6.16 Add some codes for loopback module
    2006.6.21 Add one member "is_in_disconnecting" in struct CONNECT_DEVICE_T.
    Add one member "test_mode_active" in struct BT_HCI_T.
    2006.7.7  Add some codes for classification
    Add some members in struct CONNECT_DEVICE_T
    "classification_timer_valid", "classification_timer_count", "send_classification_flag", "classification_interval",
    "is_afh_sent_flag"
    Add one member "classification_channel_map" in struct BT_HCI_T.
    Add new function Hci_ClassificationCheckTimeout
    2006.7.14 Add some codes for Qos
    Add some members in struct CONNECT_DEVICE_T
    Pls search"qos_..."
    Add some macros
    BT_LMP_EVENT_QOS_SETUP_COMPLETE
    BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE
    Add two new functions
    Hci_Command_Qos_Setup
    Hci_Command_Flow_Specification
    Add a new function Hci_ParseRegistryParameter to read roleswitchsupport value from registry.
    2006.7.26 Add some codes for eSco
    Add some members in struct SCO_CONNECT_DEVICE_T
    Pls search"esco..."
    Add one member "l2cap_rx_flow_control_flag" in struct CONNECT_DEVICE_T to control rx flow.
    2006.7.28 Add patch. If ACL connectin is established but the link supersion timer is not set, the connection would be there forever.
    So driver add another timer whose timeout value is 120 second. If the connection is established and link supersion timer is not
    set, driver would wait 120 second and then disconnect this connection.
    Add macro: BT_TIMER_MONITER_LINK_SUPERVISION_VALUE and BT_TIMER_TYPE_MONITER_LINK_SUPERVISION
    2006.8.4  Add a member "valid_flag" in struct CONNECT_DEVICE_T. This flag is set as 1 when a connection is connected. And this flag is
    set as 0 when the connection is disconnected. And if this flag is set as 0, LMPDU and some others frames should not be sent.
    2006.8.18 Add one timer for l2 flow control. Some members is added in struct CONNECT_DEVICE_T
    typedef struct _CONNECT_DEVICE
    {
    ...
    // protect l2cap flow flag timer
    UINT8 timer_l2cap_flow_valid;
    UINT16 timer_l2cap_flow_counter; // second level
    }
    three functions is added:
    Hci_StartL2capFlowTimer
    Hci_StopL2capFlowTimer
    Hci_L2capFlowTimeout
    To pass BQB, two HCI command (BT_HCI_COMMAND_WRITE_CURRENT_IAC_LAP and BT_HCI_COMMAND_READ_PAGE_SCAN_MODE) should be processed.
    two vars (num_current_iac and page_scan_mode) is also added to store the paramters from these two hci commands.
    four functions is added:
    Hci_Command_Write_Current_Iac_Lap
    Hci_Response_Write_Current_Iac_Lap
    Hci_Command_Read_Page_Scan_Mode
    Hci_Response_Read_Page_Scan_Mode
    2006.8.25 Add a var "is_in_inquiry_flag". If an inquiry is being done, this var is set as 1. Otherwise this var is set as 0.
    This var supplies a way that can protect the repeat hci command "inquiry" and "inquiry cancel". If driver did not
    check the repeated hci command "inquiry" and "inquiry cancel", IVT software would be hung.
    Add a var "is_in_remote_name_req_flag". If an remote-name-request is being done, this var is set as 1. Otherwise
    this var is set as 0. This var supplies a way that can protect the repeat hci command "remote name request" and
    "remote name request cancel". If driver did not  check the repeated hci command, IVT software would be hung.
    Add new function Hci_Command_Remote_Name_Request_Cancel and Hci_Response_Remote_Name_Request_Cancel
    2006.9.12 Add parts codes for new architecture.
    Pls search "BT_USE_NEW_ARCHITECTURE" for details.
    2006.9.20 Add one state macro "BT_DEVICE_STATE_TEMP_DISCONNECT" for fixing the following bug:
    If IVT send remote-name-request hci-command, our driver may response it with the same name.
     */

#include "bt_sw.h"
#include "bt_queue.h"

    /*--macros------------------------------------------------------------*/

    // Our blue tooth manufacturer id
    #define MANUFACTURER_3DSP_NAME			0x49 // temp value

    // Define HCI packet type
    #define BT_HCI_PACKET_COMMAND		0x1
    #define BT_HCI_PACKET_ACL_DATA		0x2
    #define BT_HCI_PACKET_SCO_DATA		0x3
    #define BT_HCI_PACKET_EVENT			0x4


    // Define HCI command OGF code
    #define BT_HCI_COMMAND_OGF_LINK_CONTROL						0x01
    #define BT_HCI_COMMAND_OGF_LINK_POLICY						0x02
    #define BT_HCI_COMMAND_OGF_HOST_CONTROLLER_BASEBAND			0x03
    #define BT_HCI_COMMAND_OGF_INFORMATION_PARAMETERS			0x04
    #define BT_HCI_COMMAND_OGF_STATUS_PARAMETERS				0x05
    #define BT_HCI_COMMAND_OGF_TESTING							0x06
    #define BT_HCI_COMMAND_OGF_VENDOR_SPECIFIC					0x3f


    // Define HCI command OCF code for link control (OGF = 0x01)
    #define BT_HCI_COMMAND_INQUIRY								0x0001
    #define BT_HCI_COMMAND_INQUIRY_CANCEL						0x0002
    #define BT_HCI_COMMAND_PERIODIC_INQUIRY_MODE				0x0003
    #define BT_HCI_COMMAND_EXIT_PERIODIC_INQUIRY_MODE			0x0004
    #define BT_HCI_COMMAND_CREATE_CONNECTION					0x0005
    #define BT_HCI_COMMAND_DISCONNECT							0x0006
    #define BT_HCI_COMMAND_ADD_SCO_CONNECTION					0x0007
    #define BT_HCI_COMMAND_CREATE_CONNECTION_CANCEL				0x0008
    #define BT_HCI_COMMAND_ACCEPT_CONNECTION_REQUEST			0x0009
    #define BT_HCI_COMMAND_REJECT_CONNECTION_REQUEST			0x000a
    #define BT_HCI_COMMAND_LINK_KEY_REQUEST_REPLY				0x000b
    #define BT_HCI_COMMAND_LINK_KEY_REQUEST_NEGATIVE_REPLY		0x000c
    #define BT_HCI_COMMAND_PIN_CODE_REQUEST_REPLY				0x000d
    #define BT_HCI_COMMAND_PIN_CODE_REQUEST_NEGATIVE_REPLY		0x000e
    #define BT_HCI_COMMAND_CHANGE_CONNECTION_PACKET_TYPE		0x000f
    #define BT_HCI_COMMAND_AUTHENTICATION_REQUESTED				0x0011
    #define BT_HCI_COMMAND_SET_CONNECTION_ENCRYPTION			0x0013
    #define BT_HCI_COMMAND_CHANGE_CONNECTION_LINK_KEY			0x0015
    #define BT_HCI_COMMAND_MASTER_LINK_KEY						0x0017
    #define BT_HCI_COMMAND_REMOTE_NAME_REQUEST					0x0019
    #define BT_HCI_COMMAND_REMOTE_NAME_REQUEST_CANCEL			0x001a
    #define BT_HCI_COMMAND_REMOTE_SUPPORTED_FEATURES			0x001b
    #define BT_HCI_COMMAND_REMOTE_EXTENDED_FEATURES				0x001c
    #define BT_HCI_COMMAND_READ_REMOTE_VERSION_INFORMATION		0x001d
    #define BT_HCI_COMMAND_READ_CLOCK_OFFSET					0x001f
    #define BT_HCI_COMMAND_READ_LMP_HANDLE						0x0020
    #define BT_HCI_COMMAND_SETUP_SYNC_CONNECTION				0x0028
    #define BT_HCI_COMMAND_ACCEPT_SYNC_CONNECTION_REQ			0x0029
    #define BT_HCI_COMMAND_REJECT_SYNC_CONNECTION_REQ			0x002a
#define BT_HCI_COMMAND_HCI_IO_CAPABILITY_REQUEST_REPLY          				0x002b
#define BT_HCI_COMMAND_USER_CONFIRMATION_REQUEST_REPLY           			0x002c
#define BT_HCI_COMMAND_USER_CONFIRMATION_REQUEST_NEGATIVE_REPLY           0x002d
#define BT_HCI_COMMAND_USER_PASSKEY_REQUEST_REPLY                       			0x002e
#define BT_HCI_COMMAND_USER_PASSKEY_REQUEST_NEGATIVE_REPLY                      0x002f
#define BT_HCI_COMMAND_REMOTE_OOB_DATA_REQUEST_REPLY         				0x0030
#define BT_HCI_COMMAND_REMOTE_OOB_DATA_REQUEST_NEGATIVE_REPLY  		0x0033
#define BT_HCI_COMMAND_HCI_IO_CAPABILITY_REQUEST_NEGATIVE_REPLY              0x0034

    // Define HCI command OCF code for link policy (OGF = 0x02)
    #define BT_HCI_COMMAND_HOLD_MODE							0x0001
    #define BT_HCI_COMMAND_SNIFF_MODE							0x0003
    #define BT_HCI_COMMAND_EXIT_SNIFF_MODE						0x0004
    #define BT_HCI_COMMAND_PARK_MODE							0x0005
    #define BT_HCI_COMMAND_EXIT_PARK_MODE						0x0006
    #define BT_HCI_COMMAND_QOS_SETUP							0x0007
    #define BT_HCI_COMMAND_ROLE_DISCOVERY						0x0009
    #define BT_HCI_COMMAND_SWITCH_ROLE							0x000b
    #define BT_HCI_COMMAND_READ_LINK_POLICY_SETTINGS			0x000c
    #define BT_HCI_COMMAND_WRITE_LINK_POLICY_SETTINGS			0x000d
    #define BT_HCI_COMMAND_READ_DEFAULT_LINK_POLICY_SETTINGS	0x000e
    #define BT_HCI_COMMAND_WRITE_DEFAULT_LINK_POLICY_SETTINGS	0x000f
    #define BT_HCI_COMMAND_FLOW_SPECIFICATION					0x0010
#define BT_HCI_COMMAND_SNIFF_SUBRATING                                     0x0011


    // Define HCI command OCF code for host controller & baseband (OGF = 0x03)
    #define BT_HCI_COMMAND_SET_EVENT_MASK						0x0001
    #define BT_HCI_COMMAND_RESET								0x0003
    #define BT_HCI_COMMAND_SET_EVENT_FILTER						0x0005
    #define BT_HCI_COMMAND_FLUSH								0x0008
    #define BT_HCI_COMMAND_READ_PIN_TYPE						0x0009
    #define BT_HCI_COMMAND_WRITE_PIN_TYPE						0x000a
    #define BT_HCI_COMMAND_CREATE_NEW_UNIT_KEY					0x000b
    #define BT_HCI_COMMAND_READ_STORED_LINK_KEY					0x000d
    #define BT_HCI_COMMAND_WRITE_STORED_LINK_KEY				0x0011
    #define BT_HCI_COMMAND_DELETE_STORED_LINK_KEY				0x0012
    #define BT_HCI_COMMAND_CHANGE_LOCAL_NAME					0x0013
    #define BT_HCI_COMMAND_READ_LOCAL_NAME						0x0014
    #define BT_HCI_COMMAND_READ_CONNECTION_ACCEPT_TIMEOUT		0x0015
    #define BT_HCI_COMMAND_WRITE_CONNECTION_ACCEPT_TIMEOUT		0x0016
    #define BT_HCI_COMMAND_READ_PAGE_TIMEOUT					0x0017
    #define BT_HCI_COMMAND_WRITE_PAGE_TIMEOUT					0x0018
    #define BT_HCI_COMMAND_READ_SCAN_ENABLE						0x0019
    #define BT_HCI_COMMAND_WRITE_SCAN_ENABLE					0x001a
    #define BT_HCI_COMMAND_READ_PAGE_SCAN_ACTIVITY				0x001b
    #define BT_HCI_COMMAND_WRITE_PAGE_SCAN_ACTIVITY				0x001c
    #define BT_HCI_COMMAND_READ_INQUIRY_SCAN_ACTIVITY			0x001d
    #define BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_ACTIVITY			0x001e
    #define BT_HCI_COMMAND_READ_AUTHENTICATION_ENABLE			0x001f
    #define BT_HCI_COMMAND_WRITE_AUTHENTICATION_ENABLE			0x0020
    #define BT_HCI_COMMAND_READ_ENCRYPTION_MODE					0x0021
    #define BT_HCI_COMMAND_WRITE_ENCRYPTION_MODE				0x0022
    #define BT_HCI_COMMAND_READ_CLASS_OF_DEVICE					0x0023
    #define BT_HCI_COMMAND_WRITE_CLASS_OF_DEVICE				0x0024
    #define BT_HCI_COMMAND_READ_VOICE_SETTING					0x0025
    #define BT_HCI_COMMAND_WRITE_VOICE_SETTING					0x0026
    #define BT_HCI_COMMAND_READ_AUTOMATIC_FLUSH_TIMEOUT			0x0027
    #define BT_HCI_COMMAND_WRITE_AUTOMATIC_FLUSH_TIMEOUT		0x0028
    #define BT_HCI_COMMAND_READ_NUM_BROADCAST_RETRANSMISSIONS	0x0029
    #define BT_HCI_COMMAND_WRITE_NUM_BROADCAST_RETRANSMISSIONS	0x002a
    #define BT_HCI_COMMAND_READ_HOLD_MODE_ACTIVITY				0x002b
    #define BT_HCI_COMMAND_WRITE_HOLD_MODE_ACTIVITY				0x002c
    #define BT_HCI_COMMAND_READ_TRANSMIT_POWER_LEVEL			0x002d
    #define BT_HCI_COMMAND_READ_SCO_FLOW_CONTROL_ENABLE			0x002e
    #define BT_HCI_COMMAND_WRITE_SCO_FLOW_CONTROL_ENABLE		0x002f
    #define BT_HCI_COMMAND_SET_HOST_CONTROLLER_TO_HOST_FLOW_CONTROL		0x0031
    #define BT_HCI_COMMAND_HOST_BUFFER_SIZE						0x0033
    #define BT_HCI_COMMAND_HOST_NUMBER_OF_COMPLETED_PACKETS		0x0035
    #define BT_HCI_COMMAND_READ_LINK_SUPERVISION_TIMEOUT		0x0036
    #define BT_HCI_COMMAND_WRITE_LINK_SUPERVISION_TIMEOUT		0x0037
    #define BT_HCI_COMMAND_READ_NUMBER_OF_SUPPORTED_IAC			0x0038
    #define BT_HCI_COMMAND_READ_CURRENT_IAC_LAP					0x0039
    #define BT_HCI_COMMAND_WRITE_CURRENT_IAC_LAP				0x003a
    #define BT_HCI_COMMAND_READ_PAGE_SCAN_PERIOD_MODE			0x003b
    #define BT_HCI_COMMAND_WRITE_PAGE_SCAN_PERIOD_MODE			0x003c
    #define BT_HCI_COMMAND_READ_PAGE_SCAN_MODE					0x003d
    #define BT_HCI_COMMAND_WRITE_PAGE_SCAN_MODE					0x003e
    #define BT_HCI_COMMAND_SET_AFH_HOST_CHANNEL_CLASSIFICATION	0x003f
    #define BT_HCI_COMMAND_READ_INQUIRY_SCAN_TYPE				0x0042
    #define BT_HCI_COMMAND_WRITE_INQUIRY_SCAN_TYPE				0x0043
    #define BT_HCI_COMMAND_READ_INQUIRY_MODE					0x0044
    #define BT_HCI_COMMAND_WRITE_INQUIRY_MODE					0x0045
    #define BT_HCI_COMMAND_READ_PAGE_SCAN_TYPE					0x0046
    #define BT_HCI_COMMAND_WRITE_PAGE_SCAN_TYPE					0x0047
    #define BT_HCI_COMMAND_READ_AFH_CHANNEL_ASSESSMENT_MODE		0x0048
    #define BT_HCI_COMMAND_WRITE_AFH_CHANNEL_ASSESSMENT_MODE	0x0049
#define BT_HCI_COMMAND_READ_EXTENDED_INQUIRY_RESPONSE          0x0051
#define BT_HCI_COMMAND_WRITE_EXTENDED_INQUIRY_RESPONSE        0x0052
#define BT_HCI_COMMAND_REFRESH_ENCRYPTION_KEY                          0x0053
#define BT_HCI_COMMAND_READ_SIMPLE_PAIRING_MODE                       0x0055
#define BT_HCI_COMMAND_WRITE_SIMPLE_PAIRING_MODE                     0x0056
#define BT_HCI_COMMAND_READ_LOCAL_OOB_DATA         						0x0057
#define BT_HCI_COMMAND_READ_INQUIRY_RESPONSE_TRANSMIT_POWER_LEVEL  0x0058
#define BT_HCI_COMMAND_WRITE_INQUIRY_TRANSMIT_POWER_LEVEL          0x0059
#define BT_HCI_COMMAND_SEND_KEYPRESS_NOTIFICATION                           0x0060
#define BT_HCI_COMMAND_READ_DEFAULT_ERRONEOUS_DATA_REPORTING   0x005a
#define BT_HCI_COMMAND_WRITE_DEFAULT_ERRONEOUS_DATA_REPORTING 0x005b
#define BT_HCI_COMMAND_ENHANCED_FLUSH                                                  0x005f



    // Define HCI command OCF code for information parameters (OGF = 0x04)
    #define BT_HCI_COMMAND_READ_LOCAL_VERSION_INFORMATION		0x0001
    #define BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_COMMANDS		0x0002
    #define BT_HCI_COMMAND_READ_LOCAL_SUPPORTED_FEATURES		0x0003
    #define BT_HCI_COMMAND_READ_LOCAL_EXTENDED_FEATURES			0x0004
    #define BT_HCI_COMMAND_READ_BUFFER_SIZE						0x0005
    #define BT_HCI_COMMAND_READ_COUNTRY_CODE					0x0007
    #define BT_HCI_COMMAND_READ_BD_ADDR							0x0009


    // Define HCI command OCF code for status parameters (OGF = 0x05)
    #define BT_HCI_COMMAND_READ_FAILED_CONTACT_COUNTER			0x0001
    #define BT_HCI_COMMAND_RESET_FAILED_CONTACT_COUNTER			0x0002
    #define BT_HCI_COMMAND_GET_LINK_QUALITY						0x0003
    #define BT_HCI_COMMAND_READ_RSSI							0x0005
    #define BT_HCI_COMMAND_READ_AFH_CHANNEL_MAP					0x0006
    #define BT_HCI_COMMAND_READ_CLOCK							0x0007


    // Define HCI command OCF code for testing (OGF = 0x06)
    #define BT_HCI_COMMAND_READ_LOOPBACK_MODE					0x0001
    #define BT_HCI_COMMAND_WRITE_LOOPBACK_MODE					0x0002
    #define BT_HCI_COMMAND_ENABLE_DEVICE_UNDER_TEST_MODE		0x0003
#define BT_HCI_COMMAND_WRITE_SIMPLE_PAIRING_DEBUG_MODE   0x0004

    // Define HCI command OCF code for vendor specific (OGF = 0x3f)
    #define BT_HCI_COMMAND_READ_ENCRYPTION_KEY_FROM_IVT			0x003d
    #define BT_HCI_COMMAND_WRITE_ENCRYPTION_KEY_FROM_IVT		0x003e


    // Define HCI event code
    #define BT_HCI_EVENT_INQUIRY_COMPLETE						0x01
    #define BT_HCI_EVENT_INQUIRY_RESULT							0x02
    #define BT_HCI_EVENT_CONNECTION_COMPLETE					0x03
    #define BT_HCI_EVENT_CONNECTION_REQUEST						0x04
    #define BT_HCI_EVENT_DISCONNECTION_COMPLETE					0x05
    #define BT_HCI_EVENT_AUTHENTICATION_COMPLETE				0x06
    #define BT_HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE			0x07
    #define BT_HCI_EVENT_ENCRYPTION_CHANGE						0x08
    #define BT_HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE	0x09
    #define BT_HCI_EVENT_MASTER_LINK_KEY_COMPLETE				0x0a
    #define BT_HCI_EVENT_READ_REMOTE_SUPPORTED_REATURES_COMPLETE	0x0b
    #define BT_HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE	0x0c
    #define BT_HCI_EVENT_QOS_SETUP_COMPLETE						0x0d
    #define BT_HCI_EVENT_COMMAND_COMPLETE						0x0e
    #define BT_HCI_EVENT_COMMAND_STATUS							0x0f
    #define BT_HCI_EVENT_HARDWARE_ERROR							0x10
    #define BT_HCI_EVENT_FLUSH_OCCURRED							0x11
    #define BT_HCI_EVENT_ROLE_CHANGE							0x12
    #define BT_HCI_EVENT_NUMBER_OF_COMPLETED_PACKET				0x13
    #define BT_HCI_EVENT_MODE_CHANGE							0x14
    #define BT_HCI_EVENT_RETURN_LINK_KEYS						0x15
    #define BT_HCI_EVENT_PIN_CODE_REQUEST						0x16
    #define BT_HCI_EVENT_LINK_KEY_REQUEST						0x17
    #define BT_HCI_EVENT_LINK_KEY_NOTIFICATION					0x18
    #define BT_HCI_EVENT_LOOPBACK_COMMAND						0x19
    #define BT_HCI_EVENT_DATA_BUFFER_OVERFLOW					0x1a
    #define BT_HCI_EVENT_MAX_SLOTS_CHANGE						0x1b
    #define BT_HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE				0x1c
    #define BT_HCI_EVENT_CONNECTION_PACKET_TYPE_CHANGED			0x1d
    #define BT_HCI_EVENT_QOS_VIOLATION							0x1e
    #define BT_HCI_EVENT_PAGE_SCAN_MODE_CHANGE					0x1f
    #define BT_HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE		0x20
    #define BT_HCI_EVENT_FLOW_SPECIFICATION_COMPLETE			0x21
    #define BT_HCI_EVENT_INQUIRY_RESULT_WITH_RSSI				0x22
    #define BT_HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE		0x23
    #define BT_HCI_EVENT_SYNC_CONNECTION_COMPLETE				0x2c
    #define BT_HCI_EVENT_SYNC_CONNECTION_CHANGED				0x2d
#define BT_HCI_EVENT_SNIFF_SUBRATING            					      0x2e
#define BT_HCI_EVENT_EXTENDED_INQUIRY_RESULT                                0x2f
#define BT_HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE               0x30
#define BT_HCI_EVENT_IO_CAPABILITY_REQUEST            					0x31
#define BT_HCI_EVENT_IO_CAPABILITY_RESPONSE          					0x32
#define BT_HCI_EVENT_USER_CONFIRMATION_REQUEST  					0x33
#define BT_HCI_EVENT_USER_PASSKEY_REQUEST             				0x34
#define BT_HCI_EVENT_REMOTE_OOB_DATA_REQUEST      				0x35
#define BT_HCI_EVENT_SIMPLE_PAIRING_COMPLETE         				0x36
#define BT_HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED  		0x38
#define BT_HCI_EVENT_ENHANCED_FLUSH_COMPLETE             			0x39
#define BT_HCI_EVENT_USER_PASSKEY_NOTIFICATION      				0x3b
#define BT_HCI_EVENT_KEYPRESS_NOTIFICATION         					0x3c
#define BT_HCI_EVENT_REMOTE_HOST_SUPPORTED_FEATURES_NOTIFICATION  0x3d


// Define HCI error code
#define BT_HCI_STATUS_SUCCESS									0x00
#define BT_HCI_ERROR_UNKNOWN_HCI_COMMAND						0x01
#define BT_HCI_ERROR_NO_CONNECTION								0x02
#define BT_HCI_ERROR_HARDWARE_FAILURE							0x03
#define BT_HCI_ERROR_PAGE_TIMEOUT								0x04
#define BT_HCI_ERROR_AUTHENTICATION_FAILURE						0x05
#define BT_HCI_ERROR_KEY_MISSING								0x06
#define BT_HCI_ERROR_MEMORY_FULL								0x07
#define BT_HCI_ERROR_CONNECTION_TIMEOUT							0x08
#define BT_HCI_ERROR_MAX_NUMBER_OF_CONNECTIONS					0x09
#define BT_HCI_ERROR_MAX_NUMBER_OF_SCO_CONNECTIONS				0x0a
#define BT_HCI_ERROR_ACL_CONNECTIONS_EXISTS						0x0b
#define BT_HCI_ERROR_COMMAND_DISALLOWED							0x0c
#define BT_HCI_ERROR_HOST_REJECTED_LIMITED_RES					0x0d
#define BT_HCI_ERROR_HOST_REJECTED_SECURITY_REASONS				0x0e
#define BT_HCI_ERROR_HOST_REJECTED_PERSONAL_DEVICE				0x0f
#define BT_HCI_ERROR_HOST_TIMEOUT								0x10
#define BT_HCI_ERROR_UNSUPPORTED_FEATURE						0x11
#define BT_HCI_ERROR_INVALID_HCI_COMMAND_PARAMETERS				0x12
#define BT_HCI_ERROR_USER_ENDED_CONNECTION						0x13
#define BT_HCI_ERROR_LOW_RESOURCES								0x14
#define BT_HCI_ERROR_ABOUT_TO_POWER_OFF							0x15
#define BT_HCI_ERROR_CONNECTION_TERMINATED_BY_LOCAL_HOST		0x16
#define BT_HCI_ERROR_REPEATED_ATTEMPTS							0x17
#define BT_HCI_ERROR_PAIRING_NOT_ALLOWD							0x18
#define BT_HCI_ERROR_UNKNOWN_LMP_PDU							0x19
#define BT_HCI_ERROR_UNSUPPORTED_REMOTE_FEATURE					0x1a
#define BT_HCI_ERROR_SCO_OFFSET_REJECTED						0x1b
#define BT_HCI_ERROR_SCO_INTERVAL_REJECTED						0x1c
#define BT_HCI_ERROR_SCO_AIR_MODE_REJECTED						0x1d
#define BT_HCI_ERROR_INVALID_LMP_PARAMETERS						0x1e
#define BT_HCI_ERROR_UNSPECIFIED_ERROR							0x1f
#define BT_HCI_ERROR_UNSUPPORTED_LMP_PARAMETER					0x20
#define BT_HCI_ERROR_ROLE_CHANGE_NOT_ALLOWED					0x21
#define BT_HCI_ERROR_LMP_RESPONSE_TIMEOUT						0x22
#define BT_HCI_ERROR_LMP_TRANSACTION_COLLOSION					0x23
#define BT_HCI_ERROR_LMP_PDU_NOT_ALLOWED						0x24
#define BT_HCI_ERROR_ENCRYPTION_MODE_NOT_ACCEPTABLE				0x25
#define BT_HCI_ERROR_UNIT_KEY_USED								0x26
#define BT_HCI_ERROR_QOS_NOT_SUPPORTED							0x27
#define BT_HCI_ERROR_INSTANT_PASSED								0x28
#define BT_HCI_ERROR_PAIRING_UNIT_KEY_NOT_SUPPORTED				0x29
#define BT_HCI_ERROR_DIFFERENT_TRANSACTION_COLLISION			0x2a
#define BT_HCI_ERROR_QOS_UNACCEPTABLE_PARAMETER					0x2c
#define BT_HCI_ERROR_QOS_REJECTED		  						0x2d
#define BT_HCI_ERROR_CHANNEL_CLASSIFICATION_NOT_SUPPORTED		0x2e
#define BT_HCI_ERROR_INSUFFICIENT_SECURITY						0x2f
#define BT_HCI_ERROR_PARAMETER_OUT_OF_MANDATORY_RANGE			0x30
#define BT_HCI_ERROR_ROLE_SWITCH_PENDING						0x32
#define BT_HCI_ERROR_RESERVED_SLOT_VIOLATION					0x34
#define BT_HCI_ERROR_ROLE_SWITCH_FAILED							0x35

// Define event mask
#define BT_HCI_EVENT_MASK_NONE											0x0000000000000000
#define BT_HCI_EVENT_MASK_INQUIRY_COMPLETE								0x0000000000000001
#define BT_HCI_EVENT_MASK_INQUIRY_RESULT								0x0000000000000002
#define BT_HCI_EVENT_MASK_CONNECTION_COMPLETE							0x0000000000000004
#define BT_HCI_EVENT_MASK_CONNECTION_REQUEST							0x0000000000000008
#define BT_HCI_EVENT_MASK_DISCONNECTION_COMPLETE						0x0000000000000010
#define BT_HCI_EVENT_MASK_AUTHENTICATION_COMPLETE						0x0000000000000020
#define BT_HCI_EVENT_MASK_REMOTE_NAME_REQ_COMPLETE						0x0000000000000040
#define BT_HCI_EVENT_MASK_ENCRYPTION_CHANGE								0x0000000000000080
#define BT_HCI_EVENT_MASK_CHANGE_CONNECTION_LINK_KEY_COMPLETE			0x0000000000000100
#define BT_HCI_EVENT_MASK_MASTER_LINK_KEY_COMPLETE						0x0000000000000200
#define BT_HCI_EVENT_MASK_READ_REMOTE_SUPP_FEA_COMPLETE					0x0000000000000400
#define BT_HCI_EVENT_MASK_READ_REMOTE_VER_INFOR_COMPLETE				0x0000000000000800
#define BT_HCI_EVENT_MASK_QOS_SETUP_COMPLETE							0x0000000000001000
#define BT_HCI_EVENT_MASK_HARDWARE_ERROR								0x0000000000008000
#define BT_HCI_EVENT_MASK_FLUSH_OCCURRED								0x0000000000010000
#define BT_HCI_EVENT_MASK_ROLE_CHANGE									0x0000000000020000
#define BT_HCI_EVENT_MASK_MODE_CHANGE									0x0000000000080000
#define BT_HCI_EVENT_MASK_RETURN_LINK_KEYS								0x0000000000100000
#define BT_HCI_EVENT_MASK_PIN_CODE_REQUEST								0x0000000000200000
#define BT_HCI_EVENT_MASK_LINK_KEY_REQUEST								0x0000000000400000
#define BT_HCI_EVENT_MASK_LINK_KEY_NOTIFICATION							0x0000000000800000
#define BT_HCI_EVENT_MASK_LOOPBACK_COMMAND								0x0000000001000000
#define BT_HCI_EVENT_MASK_DATA_BUFFER_OVERFLOW							0x0000000002000000
#define BT_HCI_EVENT_MASK_MAX_SLOTS_CHANGE								0x0000000004000000
#define BT_HCI_EVENT_MASK_READ_CLOCK_OFFSET_COMPLETE					0x0000000008000000
#define BT_HCI_EVENT_MASK_CONNECTION_PACKET_TYPE_CHANGED				0x0000000010000000
#define BT_HCI_EVENT_MASK_QOS_VIOLATION									0x0000000020000000
#define BT_HCI_EVENT_MASK_PAGE_SCAN_MODE_CHANGED						0x0000000040000000
#define BT_HCI_EVENT_MASK_PAGE_SCAN_REP_MODE_CHANGED					0x0000000080000000
#define BT_HCI_EVENT_MASK_FLOW_SPECIFICATION_COMPLETE					0x0000000100000000
#define BT_HCI_EVENT_MASK_INQUIRY_RESULT_WITH_RSSI						0x0000000200000000
#define BT_HCI_EVENT_MASK_READ_REMOTE_EXTENDED_FEA_COMPLETE				0x0000000400000000
#define BT_HCI_EVENT_MASK_SYNC_CONNECTION_COMPLETE						0x0000080000000000
#define BT_HCI_EVENT_MASK_SYNC_CONNECTION_CHANGED						0x0000100000000000
#define BT_HCI_EVENT_MASK_SNIFF_SUBRATING 	                                             0x0000200000000000
#define BT_HCI_EVENT_MASK_EXTENDED_INQUIRY_RESULT	                                0x0000400000000000
#define BT_HCI_EVENT_MASK_ENCRYPTION_KEY_REFRESH_COMPLETE			0x0000800000000000 
#define BT_HCI_EVENT_MASK_IO_CAPABILITY_REQUEST							0x0001000000000000
#define BT_HCI_EVENT_MASK_IO_CAPABILITY_REQUEST_REPLY					0x0002000000000000 
#define BT_HCI_EVENT_MASK_USER_CONFIRMATION_REQUEST					0x0004000000000000 
#define BT_HCI_EVENT_MASK_USER_PASSKEY_REQUEST							0x0008000000000000 
#define BT_HCI_EVENT_MASK_REMOTE_OOB_DATA_REQUEST						0x0010000000000000 
#define BT_HCI_EVENT_MASK_SIMPLE_PAIRING_COMPLETE						0x0020000000000000 
#define BT_HCI_EVENT_MASK_RESERVED											0x0040000000000000 
#define BT_HCI_EVENT_MASK_LINK_SUPERVISION_TIMEOUT_CHANGED			0x0080000000000000 
#define BT_HCI_EVENT_MASK_ENHANCED_FLUSH_COMPLETE						0x0100000000000000 
#define BT_HCI_EVENT_MASK_USER_PASSKEY_NOTIFICATION					0x0400000000000000 
#define BT_HCI_EVENT_MASK_KEYPRESS_NOTIFICATION							0x0800000000000000 
#define BT_HCI_EVENT_MASK_REMOTE_HOST_SUPPORTED_FEATURES_NOTIFICATION		0x1000000000000000 
#define BT_HCI_EVENT_MASK_RESERVED_FOR_FUTURE_USE						 0xE000000000000000 


#define  BT_MAX_MASTER_IDLE_SPACE_SIZE		0x900
#define  BT_MAX_SLAVE_IDLE_SPACE_SIZE		0x80c	//0x820//0x808
#define  BT_MAX_SCO_IDLE_SPACE_SIZE		0x80c	//0x808
#define  BT_MAX_SNIFF_IDLE_SPACE_SIZE		0x100
#define  BT_HCI_EVENT_MASK_DEFAULT0			0xffffffff
#define  BT_HCI_EVENT_MASK_DEFAULT1         0x00001fff

// Define some const macros
#define EEPROM_IN_SCRATCH_SIZE				1024
#define EEPROM_VERSION_SIZE					2
#define BT_BD_ADDR_LENGTH					6
#define BT_IVT_KEY_LENGTH					16
#define BT_CLOCK_OFFSET_LENGTH				2
#define BT_LOCAL_NAME_LENGTH				248
#define BT_CLASS_OF_DEVICE_LENGTH			3
#define BT_MAX_DEVICE_NUM					16  //  64->16
#define BT_MAX_SCO_DEVICE_NUM				3
#define BT_MAX_INQUIRY_RESULT_NUM			32
#define BT_MAX_ACL_DATA_PACKET_LEN			1021  //Jakio20070816 add here, for test
// #define BT_MAX_ACL_DATA_PACKET_LEN			367

#define BT_MAX_SCO_DATA_PACKET_LEN			240
#define BT_TOTAL_NUM_ACL_DATA_PACKET			4 //(BD_MAX_COUNT - 1)
#define BT_TOTAL_NUM_SCO_DATA_PACKET		2
#define BT_TOTAL_NUM_DATA_PACKET            (BT_TOTAL_NUM_ACL_DATA_PACKET + BT_TOTAL_NUM_SCO_DATA_PACKET)
#define BT_TOTAL_CONNECT_DEVICE				256
#define BT_LINK_KEY_LENGTH					16
#define BT_ACO_LENGTH						12
#define BT_MAX_CHANNEL_MAP_NUM				10
#define BT_MAX_AFH_TIMER_COUNT				10 // 20
#define BT_MAX_FEATURE_PAGE_NUMBER			0

#define BT_AM_ADDRESS_SIZE					8
#define BT_PM_ADDRESS_SIZE					(BT_TOTAL_CONNECT_DEVICE)
#define BT_CONNECTION_HANDLE_SIZE			(BT_TOTAL_CONNECT_DEVICE)

#define BT_AM_ADDRESS_SIZE_BY_BIT			(BT_AM_ADDRESS_SIZE / 8)
#define BT_PM_ADDRESS_SIZE_BY_BIT			(BT_TOTAL_CONNECT_DEVICE / 8)
#define BT_CONNECTION_HANDLE_SIZE_BY_BIT	(BT_TOTAL_CONNECT_DEVICE / 8)

#define BT_EACH_IAC_LAP_COUNT				3
#define BT_MAX_IAC_LAP_NUM					64


#define BT_L2CAP_TIMEOUT_SNIFF_COUNT			1	//jakio20080222, original value is 2. 
#define BT_L2CAP_TIMEOUT_ACTIVE_COUNT		60


#define BT_HCI_VERSION_10B					0x0
#define BT_HCI_VERSION_11					0x1
#define BT_HCI_VERSION_12					0x2
#define BT_HCI_VERSION_20					0x3
#define BT_HCI_VERSION_21					0x4

#define BT_PIN_TYPE_VARIABLE				0x0
#define BT_PIN_TYPE_FIXED					0x1

#define BT_AUTHENTICATION_DISABLE			0x0
#define BT_AUTHENTICATION_ENABLE			0x1

#define BT_ENCRYPTION_DIABLE				0x0
#define BT_ENCRYPTION_ONLY_P2P				0x1
#define BT_ENCRYPTION_P2P_BROADCAST			0x2

#define BT_ACL_PACKET_NULL					0x0
#define BT_ACL_PACKET_POLL					0x1
#define BT_ACL_PACKET_FHS					0x2					
#define BT_ACL_PACKET_DM1					0x3					
#define BT_ACL_PACKET_DH1					0x4					
#define BT_ACL_PACKET_AUX1					0x9					
#define BT_ACL_PACKET_DM3					0xa					
#define BT_ACL_PACKET_DH3					0xb					
#define BT_ACL_PACKET_DM5					0xe
#define BT_ACL_PACKET_DH5					0xf
//Jakio20071025: add here for EDR
#define BT_ACL_PACKET_2DH1					0x4
#define BT_ACL_PACKET_3DH1					0x8
#define BT_ACL_PACKET_2DH3					0xa
#define BT_ACL_PACKET_3DH3					0xb
#define BT_ACL_PACKET_2DH5					0xe
#define BT_ACL_PACKET_3DH5					0xf

#define BT_SCO_PACKET_NULL					0x0
#define BT_SCO_PACKET_POLL					0x1
#define BT_SCO_PACKET_FHS					0x2					
#define BT_SCO_PACKET_DM1					0x3					
#define BT_SCO_PACKET_HV1					0x5					
#define BT_SCO_PACKET_HV2					0x6					
#define BT_SCO_PACKET_HV3					0x7					
#define BT_SCO_PACKET_DV					0x8	

#define BT_ESCO_PACKET_NULL					0x0
#define BT_ESCO_PACKET_POLL					0x1
#define BT_ESCO_PACKET_EV3					0x7					
#define BT_ESCO_PACKET_EV4					0xc	
#define BT_ESCO_PACKET_EV5					0xd	
#define BT_ESCO_PACKET_2EV3					0x6						
#define BT_ESCO_PACKET_3EV3					0x7
#define BT_ESCO_PACKET_2EV5					0xc
#define BT_ESCO_PACKET_3EV5					0xd

#define BT_LCH_TYPE_CON_L2CAP				0x1
#define BT_LCH_TYPE_START_L2CAP				0x2
#define BT_LCH_TYPE_LMP						0x3

#define BT_LINK_TYPE_SCO					0
#define BT_LINK_TYPE_ACL					0x1
#define BT_LINK_TYPE_ESCO					0x2

#define BT_PACKET_TYPE_DM1					0x0008
#define BT_PACKET_TYPE_DH1					0x0010
#define BT_PACKET_TYPE_DM3					0x0400
#define BT_PACKET_TYPE_DH3					0x0800
#define BT_PACKET_TYPE_DM5					0x4000
#define BT_PACKET_TYPE_DH5					0x8000
#define BT_PACKET_TYPE_HV1					0x0020
#define BT_PACKET_TYPE_HV2					0x0040
#define BT_PACKET_TYPE_HV3					0x0080

//Jakio20071025: add here for EDR
#define BT_PACKET_TYPE_2DH1					0x0002
#define BT_PACKET_TYPE_3DH1					0x0004
#define BT_PACKET_TYPE_2DH3					0x0100
#define BT_PACKET_TYPE_3DH3					0x0200
#define BT_PACKET_TYPE_2DH5					0x1000
#define BT_PACKET_TYPE_3DH5					0x2000

#define BT_PACKET_TYPE_ESCO_HV1				0x0001
#define BT_PACKET_TYPE_ESCO_HV2				0x0002
#define BT_PACKET_TYPE_ESCO_HV3				0x0004
#define BT_PACKET_TYPE_ESCO_EV3				0x0008
#define BT_PACKET_TYPE_ESCO_EV4				0x0010
#define BT_PACKET_TYPE_ESCO_EV5				0x0020
#define BT_PACKET_TYPE_ESCO_2EV3			0x0040
#define BT_PACKET_TYPE_ESCO_3EV3			0x0080
#define BT_PACKET_TYPE_ESCO_2EV5			0x0100
#define BT_PACKET_TYPE_ESCO_3EV5			0x0200

#define BT_LMP_EVENT_SETUP_COMPLETE				1
#define BT_LMP_EVENT_DETACH						2
#define BT_LMP_EVENT_CLKOFFSET_RES				3
#define BT_LMP_EVENT_NAME_RES					4
#define BT_LMP_EVENT_FEATURE_RES				5
#define BT_LMP_EVENT_VERSION_RES				6
#define BT_LMP_EVENT_HOST_CONN_REQ				7
#define BT_LMP_EVENT_LMP_TIMEOUT				8
#define BT_LMP_EVENT_LINK_SUPERVISION_TIMEOUT	9
#define BT_LMP_EVENT_LINK_KEY_REQ				10
#define BT_LMP_EVENT_AUTH_FAILURE				11
#define BT_LMP_EVENT_LINK_KEY_NOTIFICATION		12
#define BT_LMP_EVENT_PIN_CODE_REQ				13
#define BT_LMP_EVENT_PAIR_NOT_ALLOW				14
#define BT_LMP_EVENT_ENCRYPTION_SUCCESS			15
#define BT_LMP_EVENT_ENCRYPTION_FAILURE			16
#define BT_LMP_EVENT_ENCRYPTION_NOT_COMP		17
#define BT_LMP_EVENT_AUTH_COMP_SUCCESS			18
#define BT_LMP_EVENT_AUTH_COMP_FAILURE			19
#define BT_LMP_EVENT_SCO_LINK_REQ				20
#define BT_LMP_EVENT_SCO_LMP_TIMEOUT			21
#define BT_LMP_EVENT_SCO_DETACH					22
#define BT_LMP_EVENT_SCO_CONNECTED				23
#define BT_LMP_EVENT_SCO_REMOVED				24
#define BT_LMP_EVENT_MODE_CHANGE				25
#define BT_LMP_EVENT_ROLE_SWITCH_FAIL			26
#define BT_LMP_EVENT_ESCO_CONNECTED				27
#define BT_LMP_EVENT_ESCO_CHANGED				28
#define BT_LMP_EVENT_ESCO_CHANGE_FAIL			29
#define BT_LMP_EVENT_ESCO_REMOVED				30
#define BT_LMP_EVENT_ESCO_LINK_REQ				31
#define BT_LMP_EVENT_CHANGE_TX_MAX_SLOT			32
#define BT_LMP_EVENT_QOS_SETUP_COMPLETE			33
#define BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE			34
//Jakio20071026: add here for EDR
#define BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE	35
#define BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE	36
#define BT_LMP_EVENT_EXTENDED_FEATURE_RES				37
#define BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY              40

#define BT_ROLE_MASTER						0
#define BT_ROLE_SLAVE						1

#define BT_TIMER_MONITER_CONN_COMP_VALUE	60  // 60 seconds
#define BT_TIMER_MONITER_LINK_SUPERVISION_VALUE	120  // 120 seconds

#define BT_RX_SCO_CON_NULL_COUNT_LIMIT		120000 // 15 seconds

#define BT_IVT_ENCRYPTION_KEY_LENGTH		16

#define BT_SCO_IN_USE						0
#define BT_ACL_IN_USE						1

//Jakio20070904
#define BT_MAX_PENDING_IRP					16

// Define some exception macros
#define BT_INVALID_OPCODE			0xffff

// Define some state macros
#define BT_COMMAND_STATE_IDLE       0
#define BT_COMMAND_STATE_WAIT_COMPLETE       1

#define BT_DEVICE_STATE_IDLE						0
#define BT_DEVICE_STATE_PAGING						1
#define BT_DEVICE_STATE_PAGED						2
#define BT_DEVICE_STATE_CONNECTED					3
#define BT_DEVICE_STATE_DETACH						4
#define BT_DEVICE_STATE_NAME_REQ					5
#define BT_DEVICE_STATE_WAIT_CONN_REQ				6
#define BT_DEVICE_STATE_WAIT_ACCEPT_CONN_REQ		7
#define BT_DEVICE_STATE_ACCEPTED					8
#define BT_DEVICE_STATE_SLAVE_CONNECTED				9
#define BT_DEVICE_STATE_NOT_ACCEPTED				10
#define BT_DEVICE_STATE_DISCONNECT					11
#define BT_DEVICE_WAIT_3TPOLL_TIMEOUT				12
#define BT_DEVICE_6TPOLL_TIMEOUT					13
#define BT_DEVICE_WAIT_3TPOLL_TIMEOUT_FOR_DISCONN	14
#define BT_DEVICE_6TPOLL_TIMEOUT_FOR_DISCONN		15
#define BT_DEVICE_STATE_WAIT_ROLE_SWITCH			16
#define BT_DEVICE_STATE_TEMP_DISCONNECT				17
#define BT_DEVICE_STATE_PAGED_DISCONNECT			18
#define BT_DEVICE_STATE_NOT_ACCEPTED_DISCONNECT		19
#define BT_DEVICE_STATE_ACCEPTED_DISCONNECT			20
#define BT_DEVICE_STATE_LMP_TIMEOUT_DISCONNECT      21
#define BT_DEVICE_STATE_AUTH_FAILURE_DISCONNECT		22
#define BT_DEVICE_STATE_PAIR_NOT_ALLOW_DISCONNECT	23
#define BT_DEVICE_STATE_ENCRYPTION_NOT_COMP_DISCONNECT	24
#define BT_DEVICE_STATE_SCO_REMOVED					25
#define BT_DEVICE_STATE_ESCO_REMOVED				26

// Define timer type
#define BT_TIMER_TYPE_INVALID				0
#define BT_TIMER_TYPE_PAGE_TIMEOUT			1
#define BT_TIMER_TYPE_LINK_SUPERVISION		2
#define BT_TIMER_TYPE_CONN_ACCEPT			3
#define BT_TIMER_TYPE_MONITER_CONN_COMP		4
#define BT_TIMER_TYPE_MONITER_LINK_SUPERVISION		5

// For LC scan
#define BT_MAX_SCAN_TABLE_COUNT				8
#define BT_FOR_LC_SCAN_DEFAULT_INTERVAL		1 // default interval is 1 second

#define BT_FOR_LC_SCAN_ADD_NEW_SUCC			0
#define BT_FOR_LC_SCAN_ADD_NEW_FULL			1
#define BT_FOR_LC_SCAN_ADD_NEW_EXIST		2
#define BT_FOR_LC_SCAN_ADD_NEW_ERROR		3

#define BT_FOR_LC_SCAN_TYPE_INQUIRY_SCAN	BIT_0
#define BT_FOR_LC_SCAN_TYPE_PAGE_SCAN		BIT_1

#define BT_FOR_LC_SCAN_INDEX_INQUIRY_SCAN	0
#define BT_FOR_LC_SCAN_INDEX_PAGE_SCAN		1

#define BT_CLOCK_READY_FLAG_SCO_REQ			1
#define BT_CLOCK_READY_FLAG_HODE_MODE		2
#define BT_CLOCK_READY_FLAG_SNIFF_MODE		3
#define BT_CLOCK_READY_FLAG_SET_AFH			4
#define BT_CLOCK_READY_FLAG_ROLE_SWITCH		5
#define BT_CLOCK_READY_FLAG_SET_AFH_FOR_ONE_CONNECTION			6   //jakio20071023
#define BT_CLOCK_READY_FLAG_OTHER			0xa

#define BT_MODE_CURRENT_MODE_ACTIVE			0
#define BT_MODE_CURRENT_MODE_HOLD			1
#define BT_MODE_CURRENT_MODE_SNIFF			2

// For Air mode type
#define BT_AIRMODE_MU_LAW					0
#define BT_AIRMODE_A_LAW					1
#define BT_AIRMODE_CVSD						2
#define BT_AIRMODE_TRANS					3

// For Inquiry Mode
#define BT_INQUIRYMODE_STANDED				0
#define BT_INQUIRYMODE_RESULT_WITH_RSSI		1
#define BT_INQUIRYMODE_RESULT_WITH_EXTENDED_RESPONSE  2

// For Inquiry Scan Type
#define BT_INQUIRYSCAN_TYPE_STANDED			0
#define BT_INQUIRYSCAN_TYPE_INTERLACED		1

// For Page Scan Type
#define BT_PAGESCAN_TYPE_STANDED			0
#define BT_PAGESCAN_TYPE_INTERLACED			1

// For Page Scan Mode
#define BT_PAGESCAN_MODE_MANDATORY			0
#define BT_PAGESCAN_MODE_OPTIONAL_I			1
#define BT_PAGESCAN_MODE_OPTIONAL_II		2
#define BT_PAGESCAN_MODE_OPTIONAL_III		3

// For AFH Channel Assessment Mode
#define BT_CONTROLLER_ASSESSMENT_DISABLED	0
#define BT_CONTROLLER_ASSESSMENT_ENABLED	1

// For max slot
#define BT_MAX_SLOT_BY_FEATURE				0
#define BT_MAX_SLOT_1_SLOT					1
#define BT_MAX_SLOT_3_SLOT					3
#define BT_MAX_SLOT_5_SLOT					5

// For protect hci command timer type
#define BT_PROTECT_HCI_COMMAND_TYPE_DSPACK			1
#define BT_PROTECT_HCI_COMMAND_TYPE_READCLK			2
#define BT_PROTECT_HCI_COMMAND_TYPE_SWITCHROLE		3
#define BT_PROTECT_HCI_COMMAND_TYPE_MODECHANGE		4

// For test loopback mode
#define BT_TEST_LOOPBACK_MODE_NO_LOOPBACK           0
#define BT_TEST_LOOPBACK_MODE_LOCAL_LOOPBACK        1
#define BT_TEST_LOOPBACK_MODE_REMOTE_LOOPBACK       2

// For HCBB master slave flag
#define BT_HCBB_MASTER_FLAG					        1
#define BT_HCBB_SLAVE_FLAG					        0



//Jakio20080301: define pre-processed bits
#define	BT_PRE_PROCESS_BAD_CHANNEL			BIT0
#define	BT_PRE_PROCESS_SCO					BIT1
#define	BT_PRE_PROCESS_SCO_NULL				BIT2
#define	BT_PRE_PROCESS_CRC_ERROR				BIT3
#define	BT_PRE_PROCESS_ACL_DATA				BIT4


#define	MAGIC_NUM_HEAD		0x155
#define	MAGIC_NUM_TAIL			0xaa


// Define some macros for bluetooth spec 2.1
#ifdef BT_2_1_SPEC_SUPPORT
// Define SCO or packet status flag
#define BT_SCO_PACKET_STATUS_FLAG_GOOD    0
#define BT_SCO_PACKET_STATUS_FLAG_NOT_GOOD    1
#define BT_SCO_PACKET_STATUS_FLAG_LOST    2
#define BT_SCO_PACKET_STATUS_FLAG_PART_LOST    3
#endif

#define SIMPLE_PAINRING_MODE_DISABLE 0x00
#define SIMPLE_PAINRING_MODE_ENABLE  0x01

#define SIMPLE_PAINRING_DEBUG_MODE_DISABLE 0x00
#define SIMPLE_PAINRING_DEBUG_MODE_ENABLE  0x01
// Define some action macros
#define BT_MAKE_OPCODE(x,y)			((((UINT16)x & 0x3f) << 10) | ((UINT16)y & 0x3ff))

#define BT_GET_OGF(x)			(((UINT16)x & 0xfc00) >> 10)
#define BT_GET_OCF(x)			((UINT16)x & 0x03ff)


/* Default SCO packet len for BlueZ stack */
#define PROTO_SCO_PACKET_LEN    (48)


#define BT_FHS_PACKET_SIZE     (18)
    /*--constants and types-----------------------------------------------*/

    #pragma pack(1)

    // define ocf and ogf
    //typedef struct _BT_OCF_OGF
    //{
    //	UINT16 ocf : 10;
    //	UINT16 ogf : 6;
    //}BT_OCF_OGF_T, *PBT_OCF_OGF_T;

    // define the opcode union
    typedef union _BT_OPCODE
    {
        // store some vars used for HCI command
        UINT16 opcode;
        struct
        {
            UINT16 ocf: 10;
            UINT16 ogf: 6;
        };
    }
    BT_OPCODE_T,  *PBT_OPCODE_T;

    // define hci command header
    typedef struct _HCI_COMMAND_HEADER
    {
        BT_OPCODE_T opcode;
        UINT8 total_len;
        UINT8 para;
    } HCI_COMMAND_HEADER_T,  *PHCI_COMMAND_HEADER_T;

    // define hci event header
    typedef struct _HCI_EVENT_HEADER
    {
        UINT8 eventcode;
        UINT8 total_len;
        UINT16 para;
    } HCI_EVENT_HEADER_T,  *PHCI_EVENT_HEADER_T;

    // define hci data header
    typedef struct _HCI_DATA_HEADER
    {
        UINT32 connection_handle: 12;
        UINT32 pb_flag: 2;
        UINT32 bc_flag: 2;
        UINT32 total_len: 16;
    } HCI_DATA_HEADER_T,  *PHCI_DATA_HEADER_T;

    // define sco data header
    typedef struct _HCI_SCO_HEADER
    {
        struct
        {
            UINT16 connection_handle: 12;
            UINT16 reserved: 4;
        };
        UINT8 total_len;
    } HCI_SCO_HEADER_T,  *PHCI_SCO_HEADER_T;

    // define payload header for single-slot packets
    typedef struct _PAYLOAD_HEADER_SINGLE_SLOT
    {
        UINT8 l_ch: 2;
        UINT8 flow: 1;
        UINT8 length: 5;
    } PAYLOAD_HEADER_SINGLE_SLOT_T,  *PPAYLOAD_HEADER_SINGLE_SLOT_T;

    // define payload header for multi-slot packets
#ifdef BT_ENHANCED_RATE
	typedef struct _PAYLOAD_HEADER_MULTI_SLOT
	{
		UINT16 l_ch : 2;
		UINT16 flow : 1;
		UINT16 length : 10;
		UINT16 undefined : 3;
	} PAYLOAD_HEADER_MULTI_SLOT_T, *PPAYLOAD_HEADER_MULTI_SLOT_T;
#else
    typedef struct _PAYLOAD_HEADER_MULTI_SLOT
    {
        UINT16 l_ch: 2;
        UINT16 flow: 1;
        UINT16 length: 9;
        UINT16 undefined: 4;
    } PAYLOAD_HEADER_MULTI_SLOT_T,  *PPAYLOAD_HEADER_MULTI_SLOT_T;
#endif
    // define hc-bb payload header
    /* typedef struct _HCBB_PAYLOAD_HEADER
    {
    UINT16 am_addr : 3;
    UINT16 type : 4;
    UINT16 length : 9;
    }HCBB_PAYLOAD_HEADER_T, *PHCBB_PAYLOAD_HEADER_T; */

    // #ifdef BT_USE_NEW_ARCHITECTURE
    typedef struct _HCBB_PAYLOAD_HEADER
    {
        UINT32 am_addr: 3;
        UINT32 type: 4;
        UINT32 length: 10;
        UINT32 master_slave_flag: 1;
        UINT32 slave_index: 1;
        UINT32 tx_retry_count: 13;
    } HCBB_PAYLOAD_HEADER_T,  *PHCBB_PAYLOAD_HEADER_T;
    //typedef struct _HCBB_PAYLOAD_HEADER
    //{
    //	UINT32 am_addr : 3;
    //	UINT32 type : 4;
    //	UINT32 length : 10;
    //	UINT32 master_slave_flag : 1;
    //	UINT32 slave_index : 1;
    //	UINT32 tx_retry_count: 13;
    // } HCBB_PAYLOAD_HEADER_T, *PHCBB_PAYLOAD_HEADER_T;
    //#else
    //typedef struct _HCBB_PAYLOAD_HEADER
    //{
    //	UINT32 am_addr : 3;
    //	UINT32 type : 4;
    //	UINT32 length : 9;
    //	UINT32 tx_retry_count: 16;
    // }HCBB_PAYLOAD_HEADER_T, *PHCBB_PAYLOAD_HEADER_T;
    // #endif

    // define FHS packet
    typedef struct _FHS_PACKET
    {
        struct
        {
            UINT64 parity_bits: 34;
            UINT64 lap: 24;
            UINT64 undefined: 2;
            UINT64 sr: 2;
            UINT64 sp: 2;
        };
        UINT8 uap;
        UINT16 nap;
        UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH];
        struct
        {
            UINT32 am_addr: 3;
            UINT32 clk27_2: 26;
            UINT32 page_scan_mode: 3;
        };
        UINT16 unused;
    }
    FHS_PACKET_T,  *PFHS_PACKET_T;

	typedef struct _FHS_TEMP_PACKET
	{
		UINT8  bd_addr[BT_BD_ADDR_LENGTH];
		UINT8  am_addr;
		UINT8  page_scan_mode;
		UINT8  sr;
		UINT8  class_of_device[BT_CLASS_OF_DEVICE_LENGTH];
		UINT16 clk27_2;
		UINT8  tmpslaveid;

	}FHS_TEMP_PACKET,*PFHS_TEMP_PACKET;


    // see below
    typedef struct _SCAN_TABLE
    {
        UINT8 counter;
        UINT8 interval;
        UINT8 enable_flag;
    } SCAN_TABLE_T,  *PSCAN_TABLE_T;

    // define something about inquiry scan or page scan. For example: We should define
    // interval for inquiry scan and page scan. Now all the interval default value is
    // 1 second. And we do scan with alternate. that is if we send scan command for
    // inquiry scan this times, we should send scan command for page scan next times.
    typedef struct _SCAN_OPTION
    {
        SCAN_TABLE_T scan_table[BT_MAX_SCAN_TABLE_COUNT];
        UINT8 index_table[BT_MAX_SCAN_TABLE_COUNT];
        UINT8 scan_bit_map;
        UINT8 current_scan_index;
        UINT8 total_scan_numbers;
    } SCAN_OPTION_T,  *PSCAN_OPTION_T;

    // define ACL packet charactis
    typedef struct _ACL_PACKET_CHARACT
    {
        UINT8 packet_type;
        UINT8 payload_head_len;
        UINT16 max_len;
        UINT8 fec_type;
        BOOLEAN crc_enable;
    } ACL_PACKET_CHARACT_T,  *PACL_PACKET_CHARACT_T;

    // define lmp features byte0
    typedef struct _LMP_FEATURES_BYTE0
    {
        UINT8 slot3: 1;
        UINT8 slot5: 1;
        UINT8 encry: 1;
        UINT8 slot_offset: 1;
        UINT8 timing_accur: 1;
        UINT8 switchbit: 1;
        UINT8 hold_mode: 1;
        UINT8 sniff_mode: 1;
    } LMP_FEATURES_BYTE0_T,  *PLMP_FEATURES_BYTE0_T;

    // define lmp features byte1
    typedef struct _LMP_FEATURES_BYTE1
    {
        UINT8 park_mode: 1;
        UINT8 rssi: 1;
        UINT8 channel_qua: 1;
        UINT8 sco_link: 1;
        UINT8 hv2: 1;
        UINT8 hv3: 1;
        UINT8 u_law: 1;
        UINT8 a_law: 1;
    } LMP_FEATURES_BYTE1_T,  *PLMP_FEATURES_BYTE1_T;

    // define lmp features byte2
    typedef struct _LMP_FEATURES_BYTE2
    {
        UINT8 cvsd: 1;
        UINT8 paging_sch: 1;
        UINT8 power_control: 1;
        UINT8 trans_sco: 1;
        UINT8 flow_con_lag0: 1;
        UINT8 flow_con_lag1: 1;
        UINT8 flow_con_lag2: 1;
        UINT8 broadcast_encry: 1;
    } LMP_FEATURES_BYTE2_T,  *PLMP_FEATURES_BYTE2_T;

    // define lmp features byte3
    typedef struct _LMP_FEATURES_BYTE3
    {
        UINT8 reserved: 1;
        UINT8 enh_rate_acl_2: 1;
        UINT8 enh_rate_acl_3: 1;
        UINT8 enh_inq_scan: 1;
        UINT8 int_inq_scan: 1;
        UINT8 int_page_scan: 1;
        UINT8 rssi_inq_res: 1;
        UINT8 ext_sco_link: 1;
    } LMP_FEATURES_BYTE3_T,  *PLMP_FEATURES_BYTE3_T;

    // define lmp features byte4
    typedef struct _LMP_FEATURES_BYTE4
    {
        UINT8 ev4: 1;
        UINT8 ev5: 1;
        UINT8 reserved1: 1;
        UINT8 afh_cap_slave: 1;
        UINT8 afh_cla_slave: 1;
        UINT8 reserved2: 1;
        UINT8 reserved3: 1;
        UINT8 slot3_enh_acl: 1;
    } LMP_FEATURES_BYTE4_T,  *PLMP_FEATURES_BYTE4_T;

    // define lmp features byte5
    typedef struct _LMP_FEATURES_BYTE5
    {
        UINT8 slot5_enh_acl: 1;
	UINT8 sniff_subrating : 1;
	UINT8 pause_encryption  : 1;
        UINT8 afh_cap_master: 1;
        UINT8 afh_cla_master: 1;
        UINT8 enh_rate_esco_2: 1;
        UINT8 enh_rate_esco_3: 1;
        UINT8 slot3_enh_esco: 1;
    } LMP_FEATURES_BYTE5_T,  *PLMP_FEATURES_BYTE5_T;

// define lmp features byte6
typedef struct _LMP_FEATURES_BYTE6
{
	UINT8 extended_inquiry_response  : 1;
	UINT8 reserved : 2;
	UINT8 secure_simple_pairing  : 1;
	UINT8 encapsulated_pdu  : 1;
	UINT8 erroneous_data_reporting  : 1;
	UINT8 nonflushable_packet_boundary : 1; 
	UINT8 reserved1 : 1;
}LMP_FEATURES_BYTE6_T, *PLMP_FEATURES_BYTE6_T;


// define lmp features byte7
typedef struct _LMP_FEATURES_BYTE7
{
	UINT8 link_supervision_timeout_changed_event  : 1;
	UINT8 inquiry_response_tx_power_level : 1;
	UINT8 reserved  : 5;
	UINT8 extended_features  : 1;
}LMP_FEATURES_BYTE7_T, *PLMP_FEATURES_BYTE7_T;

// define lmp features
typedef struct _LMP_FEATURES
{
	LMP_FEATURES_BYTE0_T byte0;
	LMP_FEATURES_BYTE1_T byte1;
	LMP_FEATURES_BYTE2_T byte2;
	LMP_FEATURES_BYTE3_T byte3;
	LMP_FEATURES_BYTE4_T byte4;
	LMP_FEATURES_BYTE5_T byte5;
	LMP_FEATURES_BYTE6_T byte6;
	LMP_FEATURES_BYTE7_T byte7;	
}LMP_FEATURES_T, *PLMP_FEATURES_T;

// define lmp features byte7
typedef struct _LMP_EXTEND_FEATURES_BYTE0
{
	UINT8 secure_simple_pairing_host : 1;
	UINT8 reserved : 7;
}LMP_EXTEND_FEATURES_BYTE0_T, *PLMP_EXTEND_FEATURES_BYTE0_T;


// define extend lmp features
typedef struct _LMP_EXTEND_FEATURES
{
	LMP_EXTEND_FEATURES_BYTE0_T byte0;
	UINT8 reserved[7];
}LMP_EXTEND_FEATURES_T, *PLMP_EXTEND_FEATURES_T;


    // define IAC_LAP struct
    typedef struct _IAC_LAP
    {
        UINT8 iac_lap[BT_EACH_IAC_LAP_COUNT];
    } IAC_LAP_T,  *PIAC_LAP_T;


#pragma pack()

// define sco connected device struct
typedef struct _SCO_CONNECT_DEVICE
{
    // for link
    BT_LIST_ENTRY_T Link;

    UINT16 connection_handle;
    UINT16 packet_type;
    UINT8 link_type;
    PVOID pConnectDevice;

    // Current status;
    UINT8 status;

    // Current state
    UINT8 state;

    // Who initiates this SCO link. If this SCO link is initiated, this flag is set as 1. Otherwise this flag is set as 0.
    UINT8 initiator;

    // store current reason. Normally for disconnection
    UINT8 current_reason_code;

    // UINT8 timing_control_flags;
    UINT8 D_sco;
    // UINT8 T_sco;
    UINT8 air_mode;
    // UINT32 clock;

    UINT8 timer_valid;
    UINT16 timer_counter; // second level
    UINT8 timer_type;

    // Saving the current SCO packet type:  BT_SCO_PACKET_HV1 or BT_SCO_PACKET_HV2 or BT_SCO_PACKET_HV3
    UINT8 current_packet_type;
    // Saving the index that this sco connect device in all sco device. This is only used for SCO defragment.
    UINT8 index;

    // This flag is set before driver removes a soc connection.
    UINT8 is_in_disconnecting;

    // If this flag is set as 1,  we should use command related to esco(HCI_Synchronous_Connection_Complete and so on)
    // instead of sco(Hci_Connection_Complete and so on).
    UINT8 using_esco_command_flag;

    UINT32 transmit_bandwidth; // esco transmit_bandwidth
    UINT32 receive_bandwidth; // esco receive_bandwidth
    UINT16 max_latency; // esco max_latency
    UINT16 voice_setting; // esco voice_setting
    UINT8 retransmission_effort; // esco retransmission_effor
    UINT16 esco_packet_type; // Normally this var could be the same meaning as "packet_type". But unfortunately the value
    // of these two vars has the different value. For example: HV1 is 0x0020 for SCO, is 0x0001 for
    // ESCO
    UINT8 transmission_interval; // esco transmission_interval
    UINT8 retransmission_window; // esco retransmission_window
    UINT16 rx_packet_length; // esco rx_packet_length
    UINT16 tx_packet_length; // esco tx_packet_length
    UINT16 content_format; // esco content_format

    // Saving the current eSCO rx packet type
    UINT8 current_esco_rx_packet_type;
    UINT8 wesco;
    UINT8 esco_amaddr;

    //Jakio20071025: add this var for EDR
    // Saving the changed SCO packet type:  BT_SCO_PACKET_HV1 or BT_SCO_PACKET_HV2 or BT_SCO_PACKET_HV3
    UINT8 changed_packet_type;

    //For BlueZ, assemble the SCO TX data to 240 bytes
    PUINT8 pTx240Buf;
    UINT32 lenTx240;
    VOID *pEle;

    // Tx/Rx Hci Sco data balance
    UINT32 txRxBalance;
    
	//Jakio20081128: this flag is identify whether we should send sco null packets to the connected device
	BOOLEAN	NeedSendScoNull;
	

} SCO_CONNECT_DEVICE_T,  *PSCO_CONNECT_DEVICE_T;

    // define connected device struct
    typedef struct _CONNECT_DEVICE
    {
        // for link
        BT_LIST_ENTRY_T Link;

        UINT16 connection_handle;
        union
        {
            UINT8 am_addr;
            UINT8 pm_addr;
        };
        UINT8 bd_addr[BT_BD_ADDR_LENGTH];
        UINT16 link_policy_settings;
        UINT16 link_supervision_timeout;
	UINT16 real_link_supervision_timeout;
        UINT8 current_role;
        UINT16 packet_type;
        UINT8 page_scan_repetition_mode;
        UINT8 page_scan_mode;
        UINT16 clock_offset;
        UINT8 allow_role_switch;
        UINT8 link_type;
        UINT8 encryption_mode;
        UINT16 lmp_states;
        UINT16 lmp_ext_states;
        UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH];

        UINT8 lmp_version;
        UINT16 manufacturer_name;
        UINT16 lmp_subversion;

        // for remote_name_request
        UINT8 remote_name[BT_LOCAL_NAME_LENGTH];

        // for read_remote_supported_features
        LMP_FEATURES_T lmp_features;
	LMP_EXTEND_FEATURES_T extended_lmp_features;

        // for pool timer
        UINT16 per_poll_interval;

        // Define the vars related to timer HCI module used. In fact, we need not use the real timer as
        // this module's timer because the time out value for this module is always so long (5~30 seconds).
        // We can use system periodic timer whose periodic value is 1 second as this module's timer. This
        // can decrease the count of real timer. In addition, there are total 3 timers in this module(Page out
        // timer, connect accept timer and link supervision timer) and these timer can not be started at the
        // same time. So we may only setup one timer and use a var "timer_type" to mark which timer is currently
        // used.
        UINT8 timer_valid;
        UINT16 timer_counter; // second level
        UINT8 timer_type;

        // Current state
        UINT8 state;
	UINT8 oldstate;
        // Current status;
        UINT8 status;
        // temp flag
        UINT8 tempflag;
        // Raw role
        UINT8 raw_role;
        // store current reason. Normally for disconnection
        UINT8 current_reason_code;

        // For Key type key
        UINT8 key_type_key;
        // For link key
        UINT8 link_key[BT_LINK_KEY_LENGTH];
        UINT8 init_key[BT_LINK_KEY_LENGTH];
        UINT8 random[BT_LINK_KEY_LENGTH];
        UINT8 aco[BT_ACO_LENGTH];

        // For pin code
        UINT8 pin_code_length;
        UINT8 pin_code[BT_LINK_KEY_LENGTH];

        // pool timer. The basic timer interval is 5 millseconds.
        UINT8 poll_timer_valid;
        UINT16 poll_timer_counter; // the value must be the times of 5 millseconds.

        // for LMP timer
        UINT8 lmp_timer_valid;
        UINT16 lmp_timer_counter; // second level
        UINT8 lmp_timer_type;

        // If this connection is in encryption process, this value is set as 1.
        //Otherwise, this value is set as 0.
        UINT8 is_in_encryption_process;

        // Link SCO connect device
        PVOID pScoConnectDevice;

        // Flag for determining which link (ACL link or SCO link) owns this message(Hci command or event).
        UINT8 acl_or_sco_flag;

        // Flag for determine what should we do next when driver receive clock ready interrupt.
        UINT8 clock_ready_flag;

        // Saving the current maximum permitted ACL packet type:  For example, if
        // current_packet_type = BT_ACL_PACKET_DM3, driver only should send DM3 and DM1.
        // If current_packet_type = BT_ACL_PACKET_DM5, driver could send DM5, DM3 and DM1 packet.
        UINT8 current_packet_type;

        // The count that driver receives the continous null sco frame. If this value is
        // greater than a certain value, it seemed that this connect device could not receive
        // any sco frame. So driver should disconnect this ACL link.
        UINT32 rx_sco_con_null_count;

        // A temp value for encryption
        UINT8 encryption_key_size;

        // rssi value, it's a signed integer. range:-128~127
        INT8 rssi;

        // When Flush command is received, this flag is set as 1
        UINT8 flush_flag;

        // Flush Timeout
        UINT16 flush_timeout;

        // If this connection is in hold mode, this value is set as 1.
        // If this connection is in sniff mode, this value is set as 2.
        //Otherwise(active), this value is set as 0.
        UINT8 mode_current_mode;
        // Hold_Mode_Max_Interval
        UINT16 hold_mode_max_interval;
        // Hold_Mode_Min_Interval
        UINT16 hold_mode_min_interval;
        // Used hold interval
        UINT16 hold_mode_interval;

        // Sniff_Max_Interval
        UINT16 sniff_max_interval;
        // Sniff_Min_Interval
        UINT16 sniff_min_interval;
        // Sniff_Attempt
        UINT16 sniff_attempt;
        // Sniff_Timeout
        UINT16 sniff_timeout;
        // Used sniff interval
        UINT16 sniff_mode_interval;

	//SNiff Subrate,Maximum Latency
	UINT16 Maximum_Latency;
	//SNiff Subrate,Maximum Remote Timeout
	UINT16 Minimum_Remote_Timeout;
	//SNiff Subrate,Maximum Local Timeout
	UINT16 Minimum_Local_Timeout;
	//SNiff Subrate,Maximum Transmit Latency
	UINT16 Maximum_Transmit_Latency;
	//SNiff Subrate, Maximum Receive Latency
	UINT16 Maximum_Receive_Latency;
        // AFH_Mode
        UINT8 afh_mode;

        // Max slot: 0 according to feature, 1  1slot, 3  3slot, 5, 5slot
        UINT8 max_slot;

        // If this flag is set as 1, it means that the role-switching process will be performed. So
        // driver should pending the l2cap packet.
        UINT8 role_switching_flag;
        UINT8 role_switching_role; // the role which would be switched.

        // am address for FHS packet
        UINT8 am_addr_for_fhs;

        // tx count
        UINT32 tx_count_frame;

        // tx power
        UINT8 tx_power;

	// rx power
	UINT8 rx_power;
        // flow control flag
        UINT8 l2cap_flow_control_flag;

        // rx flow control flag
        UINT8 l2cap_rx_flow_control_flag;

        // Sending max slot. This is the max slot our device sends the packet. 1  1slot, 3  3slot, 5, 5slot
        UINT8 tx_max_slot;

        // This flag is set before driver removes a connection.
        UINT8 is_in_disconnecting;

        // If this connect device is valid, this flag is set as 1. Otherwise this flag is set as 0.
        UINT8 valid_flag;

        // Classification timer
        UINT8 classification_timer_valid;
        UINT8 classification_timer_count;
        UINT8 send_classification_flag; // 1, need send classification. 0, don't need send classification
        UINT8 classification_interval; // second level

        UINT8 is_afh_sent_flag;

        // Qos parameters
        UINT8 qos_flags;
        UINT8 qos_service_type;
        UINT8 qos_flow_direction;
        UINT32 qos_token_rate;
        UINT32 qos_token_bucket_size;
        UINT32 qos_peak_bandwidth;
        UINT32 qos_latency;
        UINT32 qos_delay_variation;

        // protect l2cap flow flag timer
        UINT8 timer_l2cap_flow_valid;
        UINT16 timer_l2cap_flow_counter; // second level

        UINT8 is_in_remote_name_req_flag;


        // The max number of which our device acts as slave is 2. So a index is needed. the index is 0 or 1.
        UINT8 slave_index;

	//Jakio20071025: add vars below for EDR
	// changed max slot. This is the max slot our device require . 1  1slot, 3  3slot, 5, 5slot
	UINT8 changed_max_slot;
	// local slot offset
	UINT16 local_slot_offset;
	UINT8 edr_change_flag;
	UINT8 edr_mode; // 0: 1.2; 1: 2.0 enhanced
	UINT8 EDR_FeatureBit[0x10];
	UINT8  EDR_PacketBit[0x10];
#ifdef BT_ALLOCATE_FRAG_FOR_EACH_CONNECTION
	INT16 get_packet_num_from_hc;
	INT16 not_complet_packet_num;
#endif
	UINT8 rev_auto_rate; // 0: default; 1: received auto_rate pdu

	#ifdef  BT_INTERNAL_QOS_SETUP
	  UINT8 internal_qos_flag;
	#endif

	// For read remote extended feature
	UINT8 current_page_number; // 0xFF indicates no-requesting temporarily
	UINT8 max_page_number;
	UINT8 remote_extended_feature[8];

	// Add by Lewis.wang
	// 0: no pause; 1: pause; 2: pause initiator;
	UINT8 pause_encryption_status;
	// 0: default; 1: Switch Role Command; 2: Refresh Encryption Key Command; 
	// 3: Change Connection Link Key Command; 4: Authentication Requested Command
	UINT8 pause_command_flag;


	//Jakio20080304:add for Rx pre_process, used in rx state machine
	UINT8	connection_timer_valid;
	UINT8	connection_timer_count;
	UINT8	connection_state;	//0: not connected; 1:connected or timeout

	//Jakio20080617: add packet couter for mouse and keyboard
	//this is used for pre_process, prevent too much data packet when connection is not completed
	UINT32	MsKb_PacketCount;
    
    UINT8  mode_Sniff_debug1;
	UINT32	Sniff_RetryCount;

    }CONNECT_DEVICE_T,  *PCONNECT_DEVICE_T;

    // define inquiry result struct
    typedef struct _INQUIRY_RESULT
    {
        // for link
        BT_LIST_ENTRY_T Link;

        UINT8 bd_addr[BT_BD_ADDR_LENGTH];
        UINT8 page_scan_repetition_mode;
        UINT8 page_scan_period_mode;
        UINT8 page_scan_mode;
        UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH];
        UINT16 clock_offset;
    } INQUIRY_RESULT_T,  *PINQUIRY_RESULT_T;

    // define the address table struct
    typedef struct _ADDRESS_TABLE
    {
        UINT8 am_addr_table[BT_AM_ADDRESS_SIZE_BY_BIT];
        UINT8 pm_addr_table[BT_PM_ADDRESS_SIZE_BY_BIT];
        UINT8 conn_handle_table[BT_CONNECTION_HANDLE_SIZE_BY_BIT];
	UINT8 am_latest_addr;
    } ADDRESS_TABLE_T,  *PADDRESS_TABLE_T;

    // define completed packets struct
    typedef struct _COMPLETE_PACKETS
    {
        UINT8 number_of_handles;
        UINT16 connection_handle[BT_TOTAL_NUM_DATA_PACKET];
        UINT16 num_of_complete_packets[BT_TOTAL_NUM_DATA_PACKET];
    } COMPLETE_PACKETS_T,  *PCOMPLETE_PACKETS_T;

    // define the module struct
    typedef struct _BT_HCI
    {
        // store some vars used for HCI command
        UINT16 current_opcode;
        UINT32 command_state;
        UINT8 command_status;

        // store current connection handle
        UINT16 current_connection_handle;

        // store current pointer to connect device
        PCONNECT_DEVICE_T pcurrent_connect_device;

        // store current pointer to inquiry result
        PINQUIRY_RESULT_T pcurrent_inquiry_result;

        // store current reason. Normally for disconnection
        UINT8 current_reason_code;

        // for read_buffer_size
        UINT16 acl_data_packet_length;
        UINT8 sco_data_packet_length;
        UINT16 total_num_acl_data_packet;
        UINT16 total_num_sco_data_packet;

        // for read_local_version_information
        UINT8 hci_version;
        UINT16 hci_revision;
        UINT8 lmp_version;
        UINT16 manufacturer_name;
        UINT16 lmp_subversion;

        // for Read/write BD addr
        UINT8 local_bd_addr[BT_BD_ADDR_LENGTH];

        // for write_connection_accept_timeout
        UINT16 conn_accept_timeout;

        // for write_page_timeout
        UINT16 page_timeout;

        // for write_pin_type
        UINT8 pin_type;

        // for read_local_supported_features
        LMP_FEATURES_T lmp_features;

	LMP_EXTEND_FEATURES_T extended_lmp_features;
        // for change_local_name
        UINT8 local_name[BT_LOCAL_NAME_LENGTH];

        // for write_class_of_device
        UINT8 class_of_device[BT_CLASS_OF_DEVICE_LENGTH];

        // for write_authentication_enable
        UINT8 authentication_enable;

        // for write_encryption_mode
        UINT8 encryption_mode;

        // for read/write sco_flow_control_enable
        UINT8 sco_flow_control_enable;

        // for read/write Page_Scan_Activity
        UINT16 page_scan_interval;
        UINT16 page_scan_window;

        // for read/write Inquiry_Scan_Activity
        UINT16 inquiry_scan_interval;
        UINT16 inquiry_scan_window;

        // role
        UINT8 role;

        // for inquiry and inquiry result
        INQUIRY_RESULT_T inquiry_result_all[BT_MAX_INQUIRY_RESULT_NUM];
        // free inquiry result list
        BT_LIST_ENTRY_T inquiry_result_free_list;
        // used inquiry result list
        BT_LIST_ENTRY_T inquiry_result_used_list;
        // used inquiry result counts
        UINT16 num_inquiry_result_used;

        // connect device
        CONNECT_DEVICE_T device_all[BT_MAX_DEVICE_NUM];
        // free device list
        BT_LIST_ENTRY_T device_free_list;
        // am device list
        BT_LIST_ENTRY_T device_am_list;
        // slave device list, store the device information if our device is acted as a slave device.
        BT_LIST_ENTRY_T device_slave_list;
        // pm device list
        BT_LIST_ENTRY_T device_pm_list;
        // am device counts
        UINT16 num_device_am;
        // pm device counts
        UINT16 num_device_pm;
        // pm device counts
        UINT16 num_device_slave;

        // sco connect device
        SCO_CONNECT_DEVICE_T sco_device_all[BT_MAX_SCO_DEVICE_NUM];
        // free device list
        BT_LIST_ENTRY_T sco_device_free_list;

        // store am, pm and connection handle address table. One bit expresses one address.
        // If a bit is set as "1", it means this address is ready in use. Otherwise it means
        // this address is not used yet and we can use this address as our needed address.
        ADDRESS_TABLE_T addr_table;

        // spin lock for hci resources
        KSPIN_LOCK HciLock;


        // For each scan(inquiry scan or page scan).
        SCAN_OPTION_T scan_option;

        // If this var is set as "1", packet tye can be up or down according to the performance of channel.
        //  Otherwise packet type is a fixed value and can not be up or down.
        UINT8 auto_packet_select_flag;

        // If this var is set as "1", it means that the next IRP would be pended. After a moment, if this
        //  var is set as "0", the next IRP would be reprocessed.
        UINT8 acl_temp_pending_flag;

        //  Sco link count.
        UINT16 sco_link_count;

        // This timer is a second level timer (1~2 second).  It is used for protecting the response of hci command
        // from host controller back in time. Sometimes when hci proecesses a hci command, it does not respond this
        // command (does not send hci command status event) until it receive a DSP ack interrupt from DSP(device).
        // But if this interrupt is not occured for some reasons, it would lead to driver not processing any other
        // hci command (state machine lock). So this timer can unlock the state machine(in one second, if this
        // interrupt does not be occured, driver would process this event with manual)
        UINT8 protect_hci_command_ack_timer_valid;
        INT8 protect_hci_command_ack_timer_count;
        UINT8 protect_hci_command_timer_type;

        // num_keys_written
        UINT8 num_keys_written;

        // Inquiry_Mode
        UINT8 inquiry_mode;

        // Inquiry_Scan_Type
        UINT8 inquiry_scan_type;

        // Page_Scan_Type
        UINT8 page_scan_type;

        // AFH_Channel_Assessment_Mode
        UINT8 afh_ch_assessment_mode;

        // Hold_Mode_Activity
        UINT8 hold_mode_activity;

        // AFH_Channel_Map
        UINT8 afh_channel_map[BT_MAX_CHANNEL_MAP_NUM];
        UINT8 afh_timer_valid;
        UINT8 afh_timer_count;

        // Flag for determine what should we do next when driver receive clock ready interrupt.
        UINT8 clock_ready_flag;

        // Encryption key from IVT.
        UINT8 encryption_key_from_ivt[BT_IVT_ENCRYPTION_KEY_LENGTH];

        // When driver wants to send ACL data by 1 slot when bluetooth device is connected with
        // SCO link, this flag is set as 1. Otherwise this flag is set as 0.
        UINT8 acl_force_1_slot;

        // When driver want to add a sco link, it would set this flag as 1 which means all the
        // acl link would only send data by 1 slot. Another case is when SCO link transfer data
        // by HV2 type, this flag is also set as 1. For others case, this flag is set as 0.
        UINT8 sco_need_1_slot_for_acl_flag;

        // We use this member to save the current valid am address. If each bit of this byte is
        // 1, it means the am address that expressed by this bit is valid.
        UINT16 am_connection_indicator;

        // If this flag is set, we should send AUX1 frame instead of DH1 frame.
        UINT8 aux1_instead_dh1_flag;

        // If this flag is set, we should send DV frame instead of DH1 frame when sco type is HV1.
        UINT8 hv1_use_dv_instead_dh1_flag;

        // Loopback_mode for test
        UINT8 loopback_mode;

        // If this flag is set, this device is a device under test (DUT)
        UINT8 test_flag;

        // If this flag is set, this device now enter real test mode and will do some test items.
        UINT8 test_mode_active;

        // Classification_Channel_Map
        UINT8 classification_channel_map[BT_MAX_CHANNEL_MAP_NUM];
	UINT8 upper_set_classification; /* 0: default; 1: ever set */

        // write_current_iac_lap
        UINT8 num_current_iac;
        IAC_LAP_T iac_lap[BT_MAX_IAC_LAP_NUM];

        // Page_Scan_Mode
        UINT8 page_scan_mode;

        // Inquiry
        UINT8 current_inquiry_length;
        UINT8 current_num_responses;
        UINT8 current_inquiry_result_num;
        UINT8 is_in_inquiry_flag;
	UINT8 protect_inquiry_timer_valid;
	UINT8 protect_inquiry_timer_count;

        //Jakio20070719: flag for hci_poll
        UINT8 start_poll_flag;
	//Some functions but not including in features
	UINT8 driver_send_poll_in_sco_flag;
	//Page Extra Flag. If this flag is set, driver should enlarge the page timeout value.(this is used for 
	// combo mode)
	UINT8 page_extra_flag;
	//Inquiry Extra Flag. If this flag is set, driver should enlarge the inquiry length value.(this is used for 
	// combo mode)
	UINT8 inquiry_extra_flag;
	// If this flag is set, driver only use HV3 packet type when SCO connection is connected. Otherwise driver may
	// use HV3 or HV2 or HV1 packet type. Normally this flag is set as 1 in combo mode and set as 0 in bluetooth only
	// mode
	UINT8 only_use_hv3_flag;
	// If this flag is set, driver only allow one acl link to be created. This flag is set as 1 only when device is in
	// combo mode
	UINT8 only_allow_one_acl_link_flag;
	// If this flag is set, driver should check DSP state every one second. And if DSP state is not match with the driver
	// state, driver would correct it. Now this flag is only set in combo mode. Maybe in future, it is also set in bluetooth
	// only mode.
	UINT8 need_coordinate_dsp_state_flag;

	// driver should pending the l2cap packet. This flag is set for specific connect device before. But this is a global flag
	// in new architecture.
	UINT8 role_switching_flag;
	// If this flag is set, driver would check slave sco. And if it exist slave sco, driver does not create connection (as master)
	// any more.
	UINT8 slave_sco_master_not_coexist_flag;
	// Scan enable
	UINT8 scan_enable;
	UINT8 scan_enable_mask;
	//Jakio20071026: If it is not zero, lmp should write afh command indicator. Otherwise, it should not write afh command indicator.
	UINT8 need_write_afh_command;

	UINT8 tx_maxpower;  // save max tx power,read from EEPROM.
	UINT8 tx_minpower;  // save min tx power,read from EEPROM.

	// Save all the eeprom content. BD address, license key and tx power maybe use it.
	UINT8 eprombuffer[EEPROM_IN_SCRATCH_SIZE];

#ifdef BT_SET_LINK_KEY_USING_APP
	UINT8 tmp_link_key[BT_LINK_KEY_LENGTH];
#endif

	// For Periodic inquiry 
	UINT8 period_inquiry_timer_valid;
	UINT16 period_inquiry_timer_count;
	UINT8 period_inquiry_flag;
	UINT16 max_period_length;
	UINT16 min_period_length;
	UINT8 period_lap[BT_EACH_IAC_LAP_COUNT];
	UINT8 period_inquiry_length;
	UINT8 period_num_responses;

	// For event mask
	UINT32 event_mask[2];

	FHS_TEMP_PACKET fhs_temppacket;

	// For read local extended feature
	UINT8 current_page_number;
	PVOID extended_feature_pointer[BT_MAX_FEATURE_PAGE_NUMBER + 1];
	UINT16 voice_setting;  // for voice setting.
	UINT16 Default_Link_Policy_Settings;  //for default Link_Policy_Settings
	UINT32 role_switch_fail_count;
    } BT_HCI_T,  *PBT_HCI_T;

    /*--variables---------------------------------------------------------*/

/*--function prototypes-----------------------------------------------*/
NTSTATUS Hci_Init(PBT_DEVICE_EXT devExt);
VOID Hci_Release(PBT_DEVICE_EXT devExt);
NTSTATUS Hci_Write_Command_Indicator(PBT_DEVICE_EXT devExt, UINT32 value);

NTSTATUS Hci_Clear_Command_Indicator(PBT_DEVICE_EXT devExt, UINT32 value);
NTSTATUS Hci_Write_AM_Connection_Indicator(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, UINT32 mode, UINT8 am_addr);
NTSTATUS Hci_Write_AM_Connection_Indicator_Asyn(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, UINT32 mode, UINT8 am_addr);
#ifdef BT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
NTSTATUS Hci_Write_Command_Indicator_Safe(PBT_DEVICE_EXT devExt, UINT32 value, PCONNECT_DEVICE_T pConnectDevice);
#endif

NTSTATUS Hci_Write_Command_FlushFifo(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

NTSTATUS Hci_Write_AM_Addr(PBT_DEVICE_EXT devExt, UINT8 AmAddrOffset, UINT8 value);
NTSTATUS Hci_Clear_AM_Addr(PBT_DEVICE_EXT devExt, UINT8 AmAddrOffset);
NTSTATUS Hci_Write_BD_Address(PBT_DEVICE_EXT devExt, UINT32 offsetAddr, PUINT8 value);
NTSTATUS Hci_Write_Page_Access_Code(PBT_DEVICE_EXT devExt, PUINT8 pAccessCode);
UINT8 Hci_Read_Byte_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT8 DataNum);
UINT16 Hci_Read_Word_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT8 DataNum);
NTSTATUS Hci_Write_Word_To_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT32 Offset, UINT16 DataValue);
UINT32 Hci_Read_DWord_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset);
NTSTATUS Hci_Write_DWord_To_3DspReg(PBT_DEVICE_EXT devExt, UINT32 AddressOffset, UINT32 Value);
NTSTATUS Hci_Write_Sniff(PBT_DEVICE_EXT devExt, UINT16 value_DSniff, UINT16 value_Sniff_Attemp, UINT16 value_Sniff_timeout, UINT16 value_TSniff);
NTSTATUS Hci_Write_Hold_Mode_Interval(PBT_DEVICE_EXT devExt, UINT8 HodeNumber, UINT16 value);
NTSTATUS Hci_Write_Sco_Am_Addr(PBT_DEVICE_EXT devExt, UINT8 Offset, UINT8 Value);
NTSTATUS Hci_Write_DT_SCO(PBT_DEVICE_EXT devExt, UINT8 D_Sco_Value, UINT8 T_Scp_Value);
NTSTATUS Hci_Write_Encryption_Enable(PBT_DEVICE_EXT devExt, UINT8 Offset, UINT8 Value);
NTSTATUS Hci_Write_Byte_In_FourByte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Offset, UINT8 Value);
NTSTATUS Hci_Write_Byte_In_EightByte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Offset, UINT8 Value);
NTSTATUS Hci_Write_Data_To_FourBytes(PBT_DEVICE_EXT devExt, UINT32 RegAddr, PUINT8 pValue, UINT8 Length);
NTSTATUS Hci_Write_Data_To_EightBytes(PBT_DEVICE_EXT devExt, UINT32 RegAddr, PUINT8 pValue, UINT8 Length);
VOID Hci_Write_nByte_To_DWord(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Byte0, UINT8 Byte1, UINT8 Byte2, UINT8 Byte3);
NTSTATUS Hci_Write_About_ESco(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Value_Second_LT_Addr, UINT8 Value_WEsco, UINT8 Value_Esco_Packet_Type);
VOID Hci_Write_Afh_Channel_MapAndNum(PBT_DEVICE_EXT devExt, PUINT8 Afh_Channel_Map, UINT8 Afh_Channel_Num);
PINQUIRY_RESULT_T Hci_Add_Inquiry_Result(PBT_HCI_T pHci, PUINT8 bd_addr, UINT8 repetition_mode, UINT8 period_mode, UINT8 scan_mode, PUINT8 class_of_device, UINT16 clock_offset);
VOID Hci_Clear_Inquiry_Result_List(PBT_HCI_T pHci);
VOID Hci_Delete_Inquiry_Result_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
PINQUIRY_RESULT_T Hci_Find_Inquiry_Result_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
NTSTATUS Hci_Allocate_Am_address(PBT_HCI_T pHci, PUINT8 pOutAddr);
NTSTATUS Hci_Free_Am_address(PBT_HCI_T pHci, UINT8 InAddr);
NTSTATUS Hci_Allocate_Pm_address(PBT_HCI_T pHci, PUINT16 pOutAddr);
NTSTATUS Hci_Free_Pm_address(PBT_HCI_T pHci, UINT16 InAddr);
NTSTATUS Hci_Allocate_Conn_Handle(PBT_HCI_T pHci, PUINT16 pOutHandle);
NTSTATUS Hci_Free_Conn_Handle(PBT_HCI_T pHci, UINT16 InHandle);
NTSTATUS Hci_Add_Connect_Device(PBT_DEVICE_EXT devExt, PUINT8 bd_addr, UINT16 packet_type, UINT8 repetition_mode, UINT8 scan_mode, UINT16 clock_offset, UINT8 role_switch);
NTSTATUS Hci_Del_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
NTSTATUS Hci_Del_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr);
NTSTATUS Hci_Del_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle);

PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr);
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle);

// #ifdef BT_USE_NEW_ARCHITECTURE
NTSTATUS Hci_Add_Slave_Connect_Device(PBT_HCI_T pHci, UINT8 am_addr, PUINT8 bd_addr, PUINT8 class_of_device, UINT16 packet_type, UINT8 repetition_mode, UINT8 scan_mode, UINT16 clock_offset, UINT8 role_switch, UINT8 slave_index);
//#else
//    NTSTATUS Hci_Add_Slave_Connect_Device(PBT_HCI_T pHci, UINT8 am_addr, PUINT8 bd_addr, PUINT8 class_of_device, UINT16 packet_type, UINT8 repetition_mode, UINT8 scan_mode, UINT16 clock_offset, UINT8 role_switch);
// #endif
NTSTATUS Hci_Del_Slave_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
NTSTATUS Hci_Del_Slave_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr);
NTSTATUS Hci_Del_Slave_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle);
NTSTATUS Hci_Del_Slave_Connect_Device_By_AMAddr_And_Index(PBT_HCI_T pHci, UINT8 am_addr, UINT8 slave_index);

PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_BDAddr(PBT_HCI_T pHci, PUINT8 bd_addr);
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_AMAddr(PBT_HCI_T pHci, UINT8 am_addr);
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle);
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_AMAddr_And_Index(PBT_HCI_T pHci, UINT8 am_addr, UINT8 slave_index);
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_Index(PBT_HCI_T pHci, UINT8 slave_index);


PVOID Hci_Add_Sco_Connect_Device(PBT_HCI_T pHci, UINT16 packet_type, PVOID pConnectDevice);
PVOID Hci_Add_eSco_Connect_Device(PBT_HCI_T pHci, UINT32 transmit_bandwidth, UINT32 receive_bandwidth, UINT16 max_latency, UINT16 voice_setting, UINT8 retransmission_effort, UINT16 packet_type, PVOID pConnectDevice);
VOID Hci_Modify_eSco_Connect_Device(PBT_HCI_T pHci, UINT32 transmit_bandwidth, UINT32 receive_bandwidth, UINT16 max_latency, UINT16 voice_setting, UINT8 retransmission_effort, UINT16 packet_type, PVOID pTempScoConnectDevice);
VOID Hci_Del_Sco_Connect_Device(PBT_HCI_T pHci, PVOID pConnectDevice, PVOID pScoConnectDevice);
PCONNECT_DEVICE_T Hci_Find_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle, PUINT8 plink_type);
PCONNECT_DEVICE_T Hci_Find_Slave_Connect_Device_By_ConnHandle_And_Sco_ConnHandle(PBT_HCI_T pHci, UINT16 conn_handle, PUINT8 plink_type);

VOID Hci_StartTimer(PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_type, UINT16 timer_count);
VOID Hci_StopTimer(PCONNECT_DEVICE_T pConnectDevice);
VOID Hci_Timeout(PBT_DEVICE_EXT devExt);

VOID Hci_Receive_From_LMP(PBT_DEVICE_EXT devExt, UINT16 event, PUINT8 para, UINT16 len);

// The following function is for HCI commands
VOID Hci_Command_Reset(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Reset(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Buffer_Size(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Host_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest);
VOID Hci_Response_Host_Buffer_Size(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Set_Event_Mask(PBT_DEVICE_EXT devExt, PUINT8 dest);
VOID Hci_Response_Set_Event_Mask(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Read_Voice_Setting(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Voice_Setting(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Voice_Setting(PBT_DEVICE_EXT devExt, UINT16 VoiceSetting);
VOID Hci_Response_Write_Voice_Setting(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, UINT8 Num_Broadcast_Retransmissions);
VOID Hci_Response_Write_Num_Broadcast_Retransmissions(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 Default_Link_Policy_Settings);
VOID Hci_Response_Write_Default_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Failed_Contact_Counter(PBT_DEVICE_EXT devExt,UINT16 ConnHandle);
VOID Hci_Response_Read_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Reset_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Reset_Failed_Contact_Counter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Transmit_Power_Level(PBT_DEVICE_EXT devExt, UINT16 ConnHandle,UINT8 type);
VOID Hci_Response_Read_Transmit_Power_Level(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_LMP_Handle(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_LMP_Handle(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Local_Version_Info(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Local_Version_Info(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_BD_Addr(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_BD_Addr(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Scan_Enable(PBT_DEVICE_EXT devExt, UINT8 ScanEnable);
VOID Hci_Response_Write_Scan_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Scan_Enable(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Scan_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnAcceptTimeout);
VOID Hci_Response_Write_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Conn_Accept_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Page_Timeout(PBT_DEVICE_EXT devExt, UINT16 PageTimeout);
VOID Hci_Response_Write_Page_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Page_Timeout(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Page_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);


VOID Hci_Command_Write_Pin_Type(PBT_DEVICE_EXT devExt, UINT8 PinType);
VOID Hci_Response_Write_Pin_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Pin_Type(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Pin_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Local_Supported_Features(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Local_Supported_Features(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Read_Local_Extended_Features(PBT_DEVICE_EXT devExt, UINT8 pagenumber);
VOID Hci_Response_Read_Local_Extended_Features(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Change_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 Name);
VOID Hci_Response_Change_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Local_Name(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Local_Name(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 pClassOfDevice);
VOID Hci_Response_Write_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Class_Of_Device(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Class_Of_Device(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Authentication_Enable(PBT_DEVICE_EXT devExt, UINT8 AuthEnable);
VOID Hci_Response_Write_Authentication_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Authentication_Enable(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Authentication_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Encryption_Mode(PBT_DEVICE_EXT devExt, UINT8 EncryptionMode);
VOID Hci_Response_Write_Encryption_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Encryption_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Encryption_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Page_Scan_Activity(PBT_DEVICE_EXT devExt, UINT16 Interval, UINT16 Window);
VOID Hci_Response_Write_Page_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Page_Scan_Activity(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Page_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, UINT16 Interval, UINT16 Window);
VOID Hci_Response_Write_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Inquiry_Scan_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);


VOID Hci_Command_Inquiry(PBT_DEVICE_EXT devExt, PUINT8 InquiryPara);
VOID Hci_Response_Inquiry_Result(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Response_Inquiry_Result_With_Rssi(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Response_Extended_Inquiry_Response_Result(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Response_Inquiry_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 InquiryPara);
VOID Hci_Response_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Exit_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Exit_Periodic_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_StartPeriodicInquiryTimer(PBT_HCI_T pHci, UINT16 timercount);
VOID Hci_StopPeriodicInquiryTimer(PBT_HCI_T pHci);
VOID Hci_PeriodicInquiryTimeout(PBT_DEVICE_EXT devExt);

VOID Hci_Command_Inquiry_Cancel(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Inquiry_Cancel(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Create_Connection(PBT_DEVICE_EXT devExt, PUINT8 ConnPara);
VOID Hci_Response_Connection_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 LinkPolicySettings);
VOID Hci_Response_Write_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Link_Policy_Settings(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_Link_Policy_Settings(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Sniff_Subrating(PBT_DEVICE_EXT devExt, UINT16 ConnHandle,UINT16 Maximum_Latency,UINT16 Minimum_Remote_Timeout,UINT16 Minimum_Local_Timeout);
VOID Hci_Response_Sniff_Subrating(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 LinkSupervisionTimeout);
VOID Hci_Response_Write_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Clock_Offset(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_Clock_Offset(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Remote_Name_Request(PBT_DEVICE_EXT devExt, PUINT8 pRemoteNamePara);
VOID Hci_Response_Remote_Name_Request(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Remote_Name_Request_Cancel(PBT_DEVICE_EXT devExt, PUINT8 pRemoteNamePara);
VOID Hci_Response_Remote_Name_Request_Cancel(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Role_Discovery(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Role_Discovery(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Disconnect(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 Reason);
VOID Hci_Response_Disconnect_Complete(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Event_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Accept_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 pAcceptConnPara);

VOID Hci_Command_Reject_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 pRejectConnPara);

VOID Hci_Command_Read_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_Link_Supervision_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Remote_Supported_Features(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);

VOID Hci_Command_Read_Remote_Extended_Features(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 page_number);
VOID Hci_Command_Read_Remote_Version_Info(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);

VOID Hci_Command_Link_Keq_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 pLinkKeyPara);
VOID Hci_Response_Link_Keq_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Link_Keq_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 pLinkKeyNegPara);
VOID Hci_Response_Link_Keq_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Pin_Code_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 pCodeReqNegPara);
VOID Hci_Response_Pin_Code_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Pin_Code_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 pCodeReqReplyPara);
VOID Hci_Response_Pin_Code_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Set_Connection_Encryption(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT8 encryp_en);

VOID Hci_Command_Authentication_Requested(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);

VOID Hci_Command_Change_Connection_Packet_Type(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type);
VOID Hci_Command_ChangeConnectionPacketType(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type);

VOID Hci_Command_Add_Sco_Connection(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 Packet_Type);

VOID Hci_Command_Write_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, UINT8 ScoFlowControlEnable);
VOID Hci_Response_Write_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Sco_Flow_Control_Enable(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Flush(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Flush(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 FlushTimeout);
VOID Hci_Response_Write_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_Automatic_Flush_Timeout(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey);
VOID Hci_Response_Read_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Write_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey);
VOID Hci_Response_Write_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Delete_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 pStoredLinkKey);
VOID Hci_Response_Delete_Stored_Link_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Hold_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 MaxInterval, UINT16 MinInterval);

VOID Hci_Command_Sniff_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle, UINT16 MaxInterval, UINT16 MinInterval, UINT16 Attempt, UINT16 Timeout);

VOID Hci_Command_Exit_Sniff_Mode(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);

VOID Hci_Command_Switch_Role(PBT_DEVICE_EXT devExt, PUINT8 RolePara);

VOID Hci_Command_Read_Local_Supported_Commands(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Local_Supported_Commands(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Inquiry_Mode(PBT_DEVICE_EXT devExt, UINT8 InquiryMode);
VOID Hci_Response_Write_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Inquiry_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Inquiry_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, UINT8 InquiryScanType);
VOID Hci_Response_Write_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Inquiry_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Page_Scan_Type(PBT_DEVICE_EXT devExt, UINT8 PageScanType);
VOID Hci_Response_Write_Page_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Page_Scan_Type(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Page_Scan_Type(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, UINT8 AFHChMode);
VOID Hci_Response_Write_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_AFH_Channel_Assessment_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, UINT8 HoldModeActivity);
VOID Hci_Response_Write_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Hold_Mode_Activity(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Hold_Mode_Activity(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Set_AFH_Host_Channel_Classification(PBT_DEVICE_EXT devExt, PUINT8 Channel_Classification);
VOID Hci_Response_Set_AFH_Host_Channel_Classification(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Current_Iac_Lap(PBT_DEVICE_EXT devExt, PUINT8 param);
VOID Hci_Response_Write_Current_Iac_Lap(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Page_Scan_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Page_Scan_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Set_Event_Filter(PBT_DEVICE_EXT devExt, PUINT8 param);
VOID Hci_Response_Set_Event_Filter(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Unknown_Command(PBT_DEVICE_EXT devExt, UINT16 opcode);
VOID Hci_Response_Unknown_Command(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Rssi(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_Rssi(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_AFH_Channel_Map(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);
VOID Hci_Response_Read_AFH_Channel_Map(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Loopback_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Loopback_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Loopback_Mode(PBT_DEVICE_EXT devExt, UINT8 Loopback_Mode);
VOID Hci_Response_Write_Loopback_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Enable_Device_Under_Test_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Enable_Device_Under_Test_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Simple_Pairing_Debug_Mode(PBT_DEVICE_EXT devExt,UINT8 Simple_Pairing_Debug_Mode);
VOID Hci_Response_Write_Simple_Pairing_Debug_Mode(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);
VOID Hci_Command_Read_IVT_Encryption_Key(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_IVT_Encryption_Key(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Setup_Sync_Connection(PBT_DEVICE_EXT devExt, PUINT8 SyncPara);
VOID Hci_Command_Accept_Sync_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 SyncPara);
VOID Hci_Command_Reject_Sync_Connection_Request(PBT_DEVICE_EXT devExt, PUINT8 SyncPara);

VOID Hci_Command_Qos_Setup(PBT_DEVICE_EXT devExt, PUINT8 QosPara);
VOID Hci_Command_Flow_Specification(PBT_DEVICE_EXT devExt, PUINT8 QosPara);

UINT8 Hci_Command_Is_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode);
VOID Hci_Command_Discard_Command(PBT_DEVICE_EXT devExt, UINT16 opcode);

VOID Hci_Command_Read_Default_Erroneous_Data_Reporting(PBT_DEVICE_EXT devExt);
VOID Hci_Respone_Read_Default_Erroneous_Data_Reporting(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Default_Erroneous_Data_Reporting(PBT_DEVICE_EXT devExt, UINT8 Erroneous_Data_Reporting);
VOID Hci_Respone_Write_Default_Erroneous_Data_Reporting(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Refresh_Encryption_Key(PBT_DEVICE_EXT devExt, UINT16 ConnHandle);

UINT32 Set_Local_Extended_Inquiry_Response_Data(PBT_DEVICE_EXT devExt, UINT8 FEC_Required,UINT8 * ptr_Extended_Inquiry_Response,UINT16 paramlen);

VOID Hci_Command_Write_Extended_Inquiry_Response(PBT_DEVICE_EXT devExt, UINT8 FEC_Required,UINT8 * ptr_Extended_Inquiry_Response,UINT16 paramlen);
VOID Hci_Response_Write_Extended_Inquiry_Response(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Extended_Inquiry_Response(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Extended_Inquiry_Response(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Simple_Pairing_Mode(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Simple_Pairing_Mode(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Simple_Pairing_Mode(PBT_DEVICE_EXT devExt,UINT8 Simple_Pairing_Mode);
VOID Hci_Response_Write_Simple_Pairing_Mode(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Enhanced_Flush(PBT_DEVICE_EXT devExt,UINT16 ConnHandle,UINT8 PacketType);
VOID Hci_Respone_Enhanced_Flush(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Send_Keypress_Notification(PBT_DEVICE_EXT devExt,PUINT8  p_bdaddr,UINT8 Notification_Type);
VOID Hci_Respone_Send_Keypress_Notification(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Inquiry_Response_Transmit_Power_Level(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Inquiry_Response_Transmit_Power_Level(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Write_Inquiry_Transmit_Power_Level(PBT_DEVICE_EXT devExt, UINT8 TX_Power);
VOID Hci_Respone_Write_Inquiry_Transmit_Power_Level(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_IO_Capability_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR,UINT8 IO_Capability,UINT8 OOB_Data_Present,UINT8 Authentication_Requirements);
VOID Hci_Response_IO_Capability_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_IO_Capability_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR,UINT8 Reason);
VOID Hci_Response_IO_Capability_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_User_Confirmation_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR);
VOID Hci_Response_User_Confirmation_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_User_Confirmation_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR);
VOID Hci_Response_User_Confirmation_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_User_Passkey_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR,PUINT32 pNumeric_Value);
VOID Hci_Response_User_Passkey_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_User_Passkey_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR);
VOID Hci_Response_User_Passkey_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Remote_OOB_Data_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR,PUINT8 pC,PUINT8 pR);
VOID Hci_Response_Remote_OOB_Data_Request_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Remote_OOB_Data_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 BD_ADDR);
VOID Hci_Response_Remote_OOB_Data_Request_Negative_Reply(PBT_DEVICE_EXT devExt, PUINT8 dest, PUINT16 pOutLen);

VOID Hci_Command_Read_Local_OOB_Data(PBT_DEVICE_EXT devExt);
VOID Hci_Response_Read_Local_OOB_Data(PBT_DEVICE_EXT devExt,PUINT8 dest, PUINT16 pOutLen);
VOID Hci_StartPollTimer(PBT_DEVICE_EXT devExt, PBT_HCI_T pHci, PCONNECT_DEVICE_T pConnectDevice, UINT16 timer_count);
VOID Hci_StopPollTimer(PCONNECT_DEVICE_T pConnectDevice);

VOID Hci_StopAllTimer(PCONNECT_DEVICE_T pConnectDevice);

VOID Hci_InitLMPMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

VOID Hci_InitTxForConnDev(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

VOID Hci_ReleaseScoLink(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);

VOID Hci_ReleaseAllConnection(PBT_DEVICE_EXT devExt);

VOID Hci_PollDevStatus(PBT_DEVICE_EXT devExt);

UINT8 Hci_For_LC_Scan_AddNewItem(PBT_HCI_T pHci, UINT8 pos);
VOID Hci_For_LC_Scan_DelOneItem(PBT_HCI_T pHci, UINT8 pos);
VOID Hci_For_LC_Scan_UpdateCounter(PBT_HCI_T pHci);
VOID Hci_For_LC_Scan_PointToNextIndex(PBT_HCI_T pHci);
UINT8 Hci_For_LC_Scan_FindChangedItem(PBT_HCI_T pHci, UINT8 scan_enable, PUINT8 ppos);
VOID Hci_For_LC_Scan_DoSched(PBT_DEVICE_EXT devExt);
VOID Hci_For_LC_Scan_ChangeScanEnable(PBT_HCI_T pHci, UINT8 scan_enable);

VOID Hci_ScoStartTimer(PSCO_CONNECT_DEVICE_T pScoConnectDevice, UINT8 timer_type, UINT16 timer_count);
VOID Hci_ScoStopTimer(PSCO_CONNECT_DEVICE_T pScoConnectDevice);

VOID Hci_StartProtectHciCommandTimer(PBT_HCI_T pHci, UINT8 type, INT8 timercount);
VOID Hci_StopProtectHciCommandTimer(PBT_HCI_T pHci);
VOID Hci_ProtectHciCommandTimeout(PBT_DEVICE_EXT devExt);
VOID Hci_ResetHciCommandStatus(PBT_DEVICE_EXT devExt);

VOID Hci_StartL2capFlowTimer(PBT_DEVICE_EXT devExt,PCONNECT_DEVICE_T pConnectDevice);
VOID Hci_StopL2capFlowTimer(PBT_DEVICE_EXT devExt,PCONNECT_DEVICE_T pConnectDevice);
VOID Hci_L2capFlowTimeout(PBT_DEVICE_EXT devExt);

VOID Hci_AFHCheckTimeout(PBT_DEVICE_EXT devExt);
VOID Hci_SetAFHForAllDevice(PBT_DEVICE_EXT devExt, UINT32 AfhInstant);
VOID Hci_SlaveConnected(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
VOID Hci_SlaveConnectionTimeOut(PBT_DEVICE_EXT devExt);
VOID Hci_SetMaxSlotForAllDevice(PBT_DEVICE_EXT devExt, UINT8 maxslot);

VOID Hci_ClassificationCheckTimeout(PBT_DEVICE_EXT devExt);

VOID Hci_ParseRegistryParameter(PBT_DEVICE_EXT devExt);

VOID Hci_StartProtectInquiryTimer(PBT_HCI_T pHci, UINT8 timercount);
VOID Hci_StopProtectInquiryTimer(PBT_HCI_T pHci);
VOID Hci_ProtectInquiryTimeout(PBT_DEVICE_EXT devExt);

VOID Hci_Set_Inquiry_Scan_FHS(PBT_DEVICE_EXT devExt);

VOID Hci_CoordinateDspState(PBT_DEVICE_EXT devExt);
VOID Hci_ClearMainlyCommandIndicator(PBT_DEVICE_EXT devExt);

NTSTATUS Hci_Write_One_Byte(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT8 Value);
NTSTATUS Hci_Write_One_Word(PBT_DEVICE_EXT devExt, UINT32 RegAddr, UINT16 Value);

VOID Hci_SetAfhForConnectDevice(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 AfhInstant);

VOID Hci_SingleStateInit(PBT_DEVICE_EXT devExt);
VOID Hci_ComboStateInit(PBT_DEVICE_EXT devExt);


VOID Hci_DebugAmList(PBT_HCI_T pHci);


int hci_notify_frame(struct sk_buff *skb);
void PostInit(PBT_DEVICE_EXT devExt);
void NotifyDspDevClass(PBT_DEVICE_EXT devExt);
void NotifyDspScanEnable(PBT_DEVICE_EXT devExt);


#endif /* ---------------------------------------------END OF FILE---*/
