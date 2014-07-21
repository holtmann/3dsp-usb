#define ALL_BITS	(BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19|BIT20|BIT21|BIT22|BIT23|BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)



//define base address to 8051 fw downloaded to
#define DOWNLOAD_8051_FW_FIELD_OFFSET		0x0
//#define DOWNLOAD_DSP_FW_FIELD_OFFSET	    0x2400       //define it later

//Jakio2007927: modify the retry num for v4 chip
//#define CARDBUS_NUM_OF_RETRY           0x7FF            // used to configure number_of_retry reg; 16-bit value
#define CARDBUS_NUM_OF_RETRY           0x7FE  

//define for mailbox
#define OFFSET_3DSP_CHIP					0x0		    //define it later
#define OFFSET_MAILBOX_FLAG_REG				0x100		//define it later
#define OFFSET_8051_RESET_FW_REG			0x100		//define it later


#define OFFSET_LOCAL_SCRATCH_BASE_ADDRESS	0x100
#define OFFSET_MAILBOX_BASE_ADDRESS			0x200    //define it later


#define OFFSET_8051FW_DMA_SRC_REG			0x100     //define it later
#define OFFSET_8051FW_DMA_DST_REG			0x100
#define OFFSET_8051FW_DMA_CTL_REG			0x100


//add by justin. refer to spec.-- design specification ver0.2
#define OFFSET_USB_CTL_REG1					0x00
#define OFFSET_USB_CTL_REG2					0x04
#define OFFSET_USB_INT_REG1					0x05
#define OFFSET_USB_INT_REG2					0x09
#define OFFSET_USB_MEM_REG					0x0B
#define OFFSET_USB_ID_REG					0x0C
#define OFFSET_USB_INFO_REG					0x14

#define OFFSET_BULK_IN_CTL_REG_1			0x20        //ENDPOINT1
#define OFFSET_BULK_IN_BASE_ADDR_REG_1		0x24
#define OFFSET_BULK_IN_LEN_REG_1			0x28
#define OFFSET_BULK_IN_REAL_LEN_REG_1		0x2C
#define OFFSET_BULK_OUT_CTL_REG_2			0x30        //ENDPOINT2
#define OFFSET_BULK_OUT_BASE_ADDR_REG_2		0x34
#define OFFSET_BULK_OUT_LEN_REG_2			0x38
#define OFFSET_BULK_OUT_REAL_LEN_REG_2		0x3C
#define OFFSET_BULK_IN_CTL_REG_3			0x40        //ENDPOINT3
#define OFFSET_BULK_IN_BASE_ADDR_REG_3		0x44
#define OFFSET_BULK_IN_LEN_REG_3			0x48
#define OFFSET_BULK_IN_REAL_LEN_REG_3		0x4C
#define OFFSET_BULK_OUT_CTL_REG_4			0x50        //ENDPOINT4
#define OFFSET_BULK_OUT_BASE_ADDR_REG_4		0x54
#define OFFSET_BULK_OUT_LEN_REG_4			0x58
#define OFFSET_BULK_OUT_REAL_LEN_REG_4		0x5C
#define OFFSET_BULK_IN_CTL_REG_5			0x60        //ENDPOINT5
#define OFFSET_BULK_IN_BASE_ADDR_REG_5		0x64
#define OFFSET_BULK_IN_LEN_REG_5			0x68
#define OFFSET_BULK_IN_REAL_LEN_REG_5		0x6C
#define OFFSET_BULK_OUT_CTL_REG_6			0x70        //ENDPOINT6
#define OFFSET_BULK_OUT_BASE_ADDR_REG_6		0x74
#define OFFSET_BULK_OUT_LEN_REG_6			0x78
#define OFFSET_BULK_OUT_REAL_LEN_REG_6		0x7C
#define OFFSET_BULK_IN_CTL_REG_7			0x80        //ENDPOINT7
#define OFFSET_BULK_IN_BASE_ADDR_REG_7		0x84
#define OFFSET_BULK_IN_LEN_REG_7			0x88
#define OFFSET_BULK_IN_REAL_LEN_REG_7		0x8C
#define OFFSET_BULK_OUT_CTL_REG_8			0x90        //ENDPOINT8
#define OFFSET_BULK_OUT_BASE_ADDR_REG_8		0x94
#define OFFSET_BULK_OUT_LEN_REG_8			0x98
#define OFFSET_BULK_OUT_REAL_LEN_REG_8		0x9C

#define OFFSET_INT_DATA_REG1_9			    0xA0        //ENDPOINT9
#define OFFSET_INT_DATA_REG2_9			    0xA4        //ENDPOINT9
#define OFFSET_INT_DATA_REG1_10			    0xA8        //ENDPOINT10
#define OFFSET_INT_DATA_REG2_10			    0xAC        //ENDPOINT10

#define OFFSET_SINGLE_READ_DATA_REG  	    0xB0
#define OFFSET_SINGLE_WRITE_DATA_REG  	    0xB4
#define OFFSET_RW_CTRL_REG  			    0xB8

#define OFFSET_8051_CTL_REG					0xC0


/* 8051 control register definitions        offset = 0xc0   */
#define D8051_START_BIT					    BIT0        //C0
#define D8051_LOOP_BACK_MODE_BIT		    BIT1
// BIT2 - BIT7      RESERVED
#define D8051_BOOT_BIT					    BIT8        //C2
#define D8051_SPU_BOOT_LOCATION_BIT		    BIT9
// BIT10 - BI23      RESERVED
#define D8051_SCRATCH_REGISTER_BIT		    (BIT24|BIT25|BIT26|BIT27|BIT28|BIT29|BIT30|BIT31)

#define BIT_XFR_EN_SING     0x01
#define BIT_XFR_EN_MULT     0x02
#define BIT_XFR_READY       0x04
#define BIT_FORCEZEROLENGTH   0x08
#define BIT_XFR_BUSY        0x10
#define BIT_XFR_DMA1_NOTINC 0x20
#define BIT_XFR_DMA2_NOTINC 0x40

#pragma pack(1)

typedef struct _Dma_addr
{
	UINT32 addr_type: 2;
	UINT32 dma1_addr: 14;
	UINT32 reserved: 2;
	UINT32 dma2_addr: 14;
} Dma_addr,  *PDma_addr;

typedef struct _Dma_len
{
	UINT32 operation: 1;
	UINT32 suspend: 1;
	UINT32 dma1_len: 14;
	UINT32 dma2_len: 15;
	UINT32 reserved2: 1;
} Dma_len,  *PDma_len;
#pragma pack()
NTSTATUS DownStopRun8051(IN PBT_DEVICE_EXT deviceExtension);
NTSTATUS DownStartRun8051(IN PBT_DEVICE_EXT deviceExtension);
NTSTATUS DownSetDma(IN PBT_DEVICE_EXT deviceExtension, UINT16 addr, IN UINT8 addr_type, UINT16 len);
NTSTATUS DownSetBulkInDma(IN PBT_DEVICE_EXT deviceExtension, IN UINT16 addr, IN UINT8 addr_type, IN UINT16 len);
NTSTATUS DownClearRegister(IN PBT_DEVICE_EXT deviceExtension);

NTSTATUS DownBulkOutStart(IN PBT_DEVICE_EXT deviceExtension);
NTSTATUS DownBulkInStart(IN PBT_DEVICE_EXT deviceExtension);
NTSTATUS DownLoad8051FirmWare(IN PBT_DEVICE_EXT deviceExtension);
NTSTATUS DownLoad3DspFirmWare(IN PBT_DEVICE_EXT deviceExtension, BOOLEAN GotoMainLoop);

VOID BtUsb_Bus_Reset_Enable(IN PBT_DEVICE_EXT deviceExtension);
//NTSTATUS
// DownTestLoopProgram(IN PBT_DEVICE_EXT deviceExtension);
