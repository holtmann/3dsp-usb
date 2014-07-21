#ifndef __BTUSB_H
#define __BTUSB_H


#define USB_CONFIGURE_VALUE				0x01
#define USB_BTUSED_INTERFACE			0x01
#define BULKUSB_MAX_TRANSFER_SIZE		65535

#define VENDOR_CMD_IN					0x01
#define VENDOR_CMD_OUT					0x00

#define VENDOR_CMD_QUERY_DMA_SPACE		0x01
#define VENDOR_CMD_SET_DMA_REG			0x02
#define VENDOR_CMD_WRITE_CMD_INDCTOR	0X03	//Jakio20080311: add here

#define VENDOR_CMD_READ_REG			0x00;
#define VENDOR_CMD_WRITE_REG			0x01;
#define VENDOR_CMD_BURST_READ_REG		0x02;
#define VENDOR_CMD_BURST_WRITE_REG		0x03;

#define START_ADDRESS_READ_REGISTER      0xB0
#define START_ADDRESS_WRITE_REGISTER     0xB4
#define START_ADDRESS_CONTROL_REGISTER   0xB8

//#define VENDOR_LENGTH_TYPE									0x01
//#define VENDOR_COMAND_TYPE									0x02

#define VENDOR_3DSPFW_SUCCESS_TYPE                            0x08

#define USB_RW_ACTIVE_BIT										0xff
#define MAILBOX_CMD_BUSY_BITS									0xff
#define MAILBOX_CMD_INVALID									0xff	//jakio20080422:add
#define MAILBOX_START_BASE_ADDRESS							0x100
#define MAILBOX_CMD_VENDORCMD_ADDRESS							(MAILBOX_START_BASE_ADDRESS+0x28)
#define MAILBOX_CMD_TYPE_NULL				   					0x00
#define MAILBOX_CMD_TYPE_READ_3DSP_REGISTER					0x01
#define MAILBOX_CMD_TYPE_WRITE_3DSP_REGISTER					0x02
#define MAILBOX_CMD_TYPE_READ_PROGRAM_REGISTER				0x0B
#define MAILBOX_CMD_BTINITDSPPARAMETERS     							0x0A
//#define MAILBOX_CMD_READ_EEPROM_BUFFER						0x0B
//Jakio20080227: inform 8051 that we are ready to work
#define MAILBOX_CMD_READY_BEGIN_WORK						0x0e
#define MAILBOX_CMD_REQUEST_TO_JOIN							0x0d
#define MAILBOX_CMD_QUERY_SPACE_INFO							0x40
#define MAILBOX_LENGTH_SCO_ADDRESS							(MAILBOX_START_BASE_ADDRESS+0x40)
#define MAILBOX_LENGTH_ACL_OTHERS_ADDRESS						(MAILBOX_START_BASE_ADDRESS+0x20)
#define MAILBOX_CMD_REGAPI_ADDRESS							0x148

#define MAILBOX_CMD_FLUSH_TX_FIFO_MASTER					0x29
#define MAILBOX_CMD_FLUSH_TX_FIFO_SCO						0x2A
#define MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE						0x2B
#define MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF						0x2C
#define MAILBOX_CMD_FLUSH_TX_FIFO_SLAVE1						0x2D

#define MAILBOX_CMD_GET_8051_VERSION							0x30
#define MAILBOX_CMD_GET_DSP_VERSION							0x31
#define	MAILBOX_CMD_BT_PARAMETER_READY     	                 0x32           






#define MAILBOX_DATA_TYPE_FIRMWARE						0x00
#define MAILBOX_DATA_TYPE_MASTER							0x01 //
#define MAILBOX_DATA_TYPE_SCO							0x02 //
#define MAILBOX_DATA_TYPE_SLAVE							0x03 //
#define MAILBOX_DATA_TYPE_SNIFF							0x04 //
#define MAILBOX_DATA_TYPE_SLAVE1							0x05   //not used now, jakio20071219
//define bulk in irp pool
#define MAX_BULKIN_IRPNUM									0x08
#define MAX_INTERRUPT_IRPNUM								0x01
#define NOT_USED											0x00
#define USED												0x01

//define type of interrupt
#define USB_INTERRUPT_TYPE_NULL							0x00
#define USB_INTERRUPT_TYPE_3DSPHW_EVENT_MAP				0x01
#define USB_INTERRUPT_TYPE_3DSP_READ_REGISTER			0x02
#define USB_INTERRUPT_TYPE_DMA_SPACE_STATUS				0x03
#define USB_INTERRUPT_TYPE_INIT_DSP_PARA					0x0a
#define USB_INTERRUPT_TYPE_READ_PROGRAM_REGISTER			0x0b
#define USB_INTERRUPT_TYPE_WAIT							0x0d
#define USB_INTERRUPT_TYPE_BEGIN_WORK						0x0e
#define DSP_FHS_BDADDR_EXIST							0x10    
#define DSP_SCRATCH_SPACE_INFO							0x40
#define USB_INTERRUPT_TYPE_JUMP_MIN_PROCESS				0x18
#define USB_INTERRUPT_TYPE_JUMP_MAIN_PROCESS				0x08
#define USB_8051_VERSION_INFO                               0x30
#define USB_DSP_VERSION_INFO                                0x31
#define	MAILBOX_CMD_BT_PARAMETER_READY     	                 0x32           

#define USB_INTERRUPT_CLOCK_INFO				0x34
#define USB_INTERRUPT_FLUSH									0x2E


//
//some interrupt status,I copy this status from pci driver,but only some of these status  be used,
//finally,we must modify the define,delete some useless.
//
#define BT_DSP_INT_TX_COMP_BIT						BIT_0
#define BT_DSP_INT_RX_COMP_BIT						BIT_1
#define BT_DSP_INT_INQUIRY_RESULT_EVENT				BIT_2
#define BT_DSP_INT_INQUIRY_COMPLETE_EVENT			BIT_3
#define BT_DSP_INT_HARDWARE_ERROR_EVENT				BIT_4
#define BT_DSP_INT_FLUSH_OCCURED_EVENT				BIT_5
#define BT_DSP_INT_ROLE_CHANGE_EVENT				BIT_6
#define BT_DSP_INT_DSP_ACK							BIT_7
#define BT_DSP_INT_DSP_MODE_CHANGE_INTO_HOLD		BIT_8
#define BT_DSP_INT_DSP_CLOCK_READY_EVENT			BIT_9
#define BT_DSP_INT_SCO_TX_COMP_BIT					BIT_10
#define BT_DSP_INT_DSP_MODE_CHANGE_INTO_ACTIVE		BIT_11
#define BT_DSP_INT_DSP_MODE_CHANGE_INTO_SNIFF		BIT_12
#define BT_DSP_INT_ROLE_CHANGE_FAIL_EVENT			BIT_13
#define BT_DSP_INT_REPORT_RX_POWER					BIT_14
#define BT_DSP_INT_SLAVE_TX_COMP_BIT					BIT_15
#define BT_DSP_INT_MASTER_SNIFF_TX_COMP_BIT			BIT_16


#define BULKUSB_TRANSFER_BUFFER_SIZE						65535

//these sub command implement by mailbox
//these command is sub command of 4 operation cmd
//define the index at doc:3dsp_usb_data_interface_and_space_map
#define VCMD_API_REQ_INVALID_CMA		(0x00)		//???
#define VCMD_REQ_READ_PCI_DWORD		(0x01)
#define VCMD_REQ_WRITE_PCI_DWORD		(0x02)
//#define VCMD_REQ_WRITE_PCI_DWORD		(0x03)    use for blue tooth
#define VCMD_REQ_FLUSH_TX_FIFO		(0x04)
//
#define VCMD_API_WRITE_AMEM_REQUEST     (0x05)
#define VCMD_API_RESET_REQUEST  		(0x06)
#define VCMD_API_RELEASE_REQUEST	    (0x07)
#define VCMD_API_RELEASE_CODE_DSPCODE   (0x08)


#define VENDOR_LOOP_MIN			100
#define VENDOR_LOOP_MIDDLE		1000
#define VENDOR_LOOP_MAX			10000
#define BASIC_VCMD_FUNCTION

#define VENDERCMD_BUSY_BITS		    0xff

//define base address to 8051 fw downloaded to
#define DOWNLOAD_8051_FW_FIELD_OFFSET		0x0      //define it later
#define DOWNLOAD_DSP_FW_FIELD_OFFSET		0x2400      //define it later
#define DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET		0xA000      //define it later
#define DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET		0xB000      //define it later


//vendor command length 24 bytes, 1bytes for cmd type, 23 bytes for cmd data
#define VENDOR_CMD_HEAD_BYTES			1
#define VENDOR_CMD_DATA_BYTES			23
#define VENDOR_CMD_LENGTH_BYTES			24


#pragma pack(1)

//marked by peter,because of the modify of the spec
//typedef struct _VCMD_LENGTH{
//	UINT8   Type;
//	UINT8   SubType;
//	UINT16  Length;
// }VCMD_LENGTH,*PVCMD_LENGTH;

typedef struct _VCMD_LENGTH
{
	UINT16 Length;
	UINT8 Reserve[5];
	UINT8 Type;
} VCMD_LENGTH,  *PVCMD_LENGTH;
typedef struct _DSP_FW_DOWN_OK
{
	UINT8 request_cmd;
} DSP_FW_DOWN_OK_T,  *PDSP_FW_DOWN_OK_T;

typedef enum
{
	MMAC_CORE_NOT_RDY_RESERVED = 0,  /* not used */
	MMAC_CORE_RDY = 1,  /* when sp-20 is ready to proceed, it sets the state to this value */
	MMAC_CORE_START_REQ_HD = 2,  /* host requests to dsp to update mmac interface */
	MMAC_CORE_STAGE2_ASSOC_INIT = 3,  /*host init the state to this value when testing association using Stage 2 LB*/
	MMAC_CORE_RXFRAG_TEST_INIT = 4,  /*host init the state to this value when testing rx frag data path using Stage 2 LB*/
	MMAC_CORE_SOFTBOOT_REQ_HD,  /* request to sp-20 to soft-reboot */
	MMAC_CORE_RDY_FOR_INTERFACE_UPDATE_DH,  /* sp-20 reports to host that host may proceed with
	mmac interface. It is in reponse to MMAC_CORE_SOFTBOOT_REQ_HD*/
} mmacCoreState_t;


//jakio20071107: add for bulk writing dsp register 
typedef struct _BTUSB_REG_API_HEADER
{
	UINT8	cmdType;
	UINT8	sumLen;
}BTUSB_REG_API_HEADER, *PBTUSB_REG_API_HEADER;

typedef struct _BTUSB_REG_API_ELE
{
	UINT16	regAddr;
	struct
	{
		UINT8 eleLen:5;
		UINT8 subCmd:3;
	}operand;
	UINT8 data[1];
}BTUSB_REG_API_ELE, *PBTUSB_REG_API_ELE;


//asyn write 3dsp register on dispatch level
#define USB_WRITE_3DSP_REGISTER_ASYN  0x138
#define USB_WRITE_3DSP_REG_ASYN_TYPE  0x04

typedef struct _WRITE_DSP_REG_ASYN{
	UINT8  cmd_type;
	UINT16 cmd_addr;
	UINT8  cmd_len;
	UINT8  cmd_data[4];
//	UINT16 cmd_data;
}WRITE_DSP_REG_ASYN,*PWRITE_DSP_REG_ASYN;

typedef struct _SP20_BIN_CODE {
	UINT32 Size;		// Size of the bin resource file
	UINT32 Port;		// SP20 mem port index
	UINT8 Code[1];	// SP20 code - copied from bin resource
} SP20_BIN_CODE, *PSP20_BIN_CODE;

// 090702 glen added for extend card type support
typedef struct {
	UINT32 m_Mark;	// mark
	UINT32 Size;		// Size of support cardtypes
	UINT32 m_dwID[1];	// cardID
} SP20_SUPPORT_CODETABLE, *PSP20_SUPPORT_CODETABLE;

typedef struct  
{
	UINT32 m_dwID;		// cardID = CodeId+cardID 
	UINT32 m_dwOffset;	// code offset
	UINT32 m_dwLen;		// code Len
	UINT32 m_dwPortBase;		// code portbase
} SP20_BIN_CODE_TABLE,*PSP20_BIN_CODE_TABLE ;

typedef struct _SP20_FILE_HEAD
{
	UINT8         file_name[4];			// 00h - 03h	File Type Mark, Must be string "SP20"
	UINT32 	      bin_len;				// 04h - 07h	Bin File Data Length, not include this FileHead
	UINT32         file_timestamp;  	  	// 08h - 0Bh	File timestamp
	UINT32         bin_checksum;		    // 0Ch - 0Fh	Bin Data checksum
	UINT32         reserve1[4];			// 10h - 1Fh	Reserve space
	UINT32         wlanonly_offset;  	// 20h - 23h	WLAN only DSP code data offset address
	UINT32         wlanonly_len;			// 24h - 27h	WLAN only DSP code data Length
	UINT32         btonly_offset;  		// 28h - 2Bh	BT only DSP code data offset address
	UINT32         btonly_len;			// 2Ch - 2Fh	BT only DSP code data Length
	UINT32         combo_offset;  		// 30h -33h	COMBO DSP code data offset address
	UINT32         combo_len;			// 34h -37h	COMBO DSP code data Length
	UINT32         eeprom_offset;  		// 38h - 3Bh	EEPROM access DSP code data offset address
	UINT32         eeprom_len;			// 3Ch - 3Fh	EEPROM access DSP code data Length
	UINT32         reserve2[4];			// 40h - 4Fh	Reserve space
	UINT32         wlan8051_offset;		// 50h - 53h	WLAN only 8051 code data offset address
	UINT32         wlan8051_len;		    // 54h - 57h	WLAN only 8051 code data Length
	UINT32         bt8051_offset;  		// 58h - 5Bh	BT only DSP 8051 data offset address
	UINT32         bt8051_len;			// 5Ch - 5Fh	BT only DSP 8051 data Length
	UINT32         combo8051_offset;  	// 60h -63h	COMBO DSP 8051 data offset address
	UINT32         combo8051_len;		// 64h -67h	COMBO DSP 8051 data Length
	UINT32         reserve3[2];			// 68h - 7Fh	Reserve space
	UINT8           bin_data[1];			// 80H~	WLAN only DSP code data region
} SP20_FILE_HEAD_T, *PSP20_FILE_HEAD_T;

#pragma pack()

/* Bluetooth USB interface number */
#define	TDSP_BT_INTERFACE_NUM	1
#define	EP_COUNT				5


#define	INDEX_EP_CTRL			0
#define	INDEX_EP_BULK_IN_DATA	5
#define	INDEX_EP_BULK_OUT_ACL	6
#define	INDEX_EP_BULK_IN_INQ	7
#define	INDEX_EP_BULK_OUT_SCO	8
#define	INDEX_EP_INTR			10




//------------------------------------------------------------------//
//--------------------WRITE DSP REG API, JAKIO20071107------------------//
#define BTUSB_REGAPI_FIRST_ELE(start_addr)	\
	((PUINT8)((PUINT8)start_addr + sizeof(BTUSB_REG_API_HEADER)))

#define BTUSB_REGAPI_NEXT_ELE(pEle)	\
	(pEle = (PBTUSB_REG_API_ELE)(pEle->data + pEle->operand.eleLen))	

#define REGAPI_REG_SET_OPERATION		0x00

#define REGAPI_REG_OR_OPERATION		0x04
#define REGAPI_REG_AND_OPERATION		0x02


#define REGAPI_MAX_BUFFER_LENGTH		64

//#define BTUSB_REGAPI_BUFFER_LEN			80;

//------------------------------------------------------------------//



#endif
