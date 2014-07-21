#ifndef __USBWLAN_VENDOR_H
#define __USBWLAN_VENDOR_H

//#include <ndis.h>
//#include "usbwlan_wlan.h"
//#include "usbwlan_Sw.h"
//#include "usbwlan_proto.h"
//#include "usbwlan_dbg.h"
//#include "usbwlan_rx.h"
//#include "usbwlan_queue.h"
//#include "usbwlan_rxarea.h"

#include "precomp.h"

//wait jazz confirm
#define VCMD_VAL_RESTART_FW         ((UINT8)(0x00))     //define it later
//////////////////////////////////////////////////////////////////////

#define VCMD_WRITE_OP			 	((UINT8)(0x00))
#define VCMD_READ_OP			 	((UINT8)(0x01))

#define VCMD_READ_TIMOUT_COUNT      1000

//Jakio20070528: moved the macro here from usbwlan_vendor.c
#define  VENDOR_LOOP_MIN			100
#define  VENDOR_LOOP_MIDDLE		    1000
#define  VENDOR_LOOP_MAX			10000



//copy from test driver
#define CURRENT_TX_RATE             RATE_TYPE_1M
#define CURRENT_TX_PREAMBLE         PREAMBLE_TYPE_LONG

#define REG_READ                    0
#define REG_WRITE                   1
#define BURST_REG_READ              2
#define BURST_REG_WRITE             3

// update by Justin. to regulate definition and use definition in usbwlan_usbdef.h
// ********* begin Justin ***********
//#define  START_ADDRESS_MAILBOX            0x100
//#define  START_ADDRESS_MAILBOX            0x100
//#define	START_ADDRESS_VENDERCMD		0x108
//#define	START_ADDRESS_VENDERDATA	START_ADDRESS_VENDERCMD + 1
//#define  START_ADDRESS_READ_REGISTER      0xB0
//#define  START_ADDRESS_WRITE_REGISTER     0xB4
//#define  START_ADDRESS_CONTROL_REGISTER   0xB8


//#define BIT_XFR_EN_SING     0x01
//#define BIT_XFR_EN_MULT     0x02
//#define BIT_XFR_READY       0x04
//#define BIT_FORCEZEROLENGTH   0x08
//#define BIT_XFR_BUSY        0x10
//#define BIT_XFR_DMA1_NOTINC 0x20
//#define BIT_XFR_DMA2_NOTINC 0x40

#define MAILBOX_BUSY_BITS           0xff      //define it later
//#define    USB_RW_ACTIVE_BIT		      BIT0      //define it later
#define VENDERCMD_BUSY_BITS		    0xff       

//vendor command request define
#define VCMD_READ_REG			 	(0x00)
#define VCMD_WRITE_REG			 	(0x01)
#define VCMD_READ_BURSTREG			(0x02)
#define VCMD_WRITE_BURSTREG			(0x03)


#define ACCESS_REG_LEN			    8
//#define VCMD_WAIT_TIME_READ_PCI	    2000
#define VCMD_WAIT_TIME_READ_PCI	    200
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
#define VCMD_API_SET_ENCRYTION_MODE	     (0x0a)
#define VCMD_API_ABORT_PIPE_MODE	     (0x0b)
#define VCMD_API_READ_POWER_TABLE      (0xb)	//
#define VCMD_API_FUNCTION_REQ_JOIN		(0xd)
//	Begin Added by Joe 2007-8-23
//	For Read Program Area 16K
#define VCMD_REQ_READ_PROGRAM_DWORD		(0x10)
#define VCMD_REQ_WRITE_PROGRAM_DWORD		(0x11)
#define VCMD_REQ_FLUSH_RX_FIFO				(0x1a)
//	End Added 
#define VCMD_API_8051_JUMP_MIN_PROCESS	(0x18)
#define VCMD_API_SET_CHANNEL_REQUEST     (0x20)		//
#define VCMD_API_RESET_BULKOUT_REQUEST     (0x21)		//
#define VCMD_API_GET_8051VER_REQUEST     (0x30)		//

#define	MAILBOX_CMD_BT_PARAMETER_READY     	                 0x32           


#define	CMD_GET_8051Status  0x33 


//the command added for halt case of combo  mode 
//a device sent the cmd to 8051 before halt,
//8051 do disable card action if non another device is active
//#define VCMD_API_DISABLE_CARD_INTERRUPT	(0x09)


// define (INT type) for (Interrupt message format)     (in spec. - 3dsp_usb_data_interface_and_space_map_v1.6.doc)
#define INT_TYPE_INVALID_MSG            (0x00)          // Indicate this is invalid message
#define INT_TYPE_3DSP_HW_EVENT_MAP      (0x01)          // Indicate 3DSP HW event mapping
#define INT_TYPE_READ_3DSP_REG          (0x02)          // Indicate the content is read from 3DSP register
#define INT_TYPE_TX_STOP_EVENT      (0x03)        //indicate the tx frag count in tx fifo, detailed format is not defined
							//this type is only used for WLAN,  jakio20070525 add here
#define INT_TYPE_READY_FOR_WLAN_TX        (0x04)          // Indicate that the last TX packet has dma finished
//#define INT_TYPE_INVALID_MSG            (0x04)          // Indicate this is invalid message
#define INT_TYPE_RX_OVERFLOW_5_TIMES        (0x06)          // Indicate that dsp rx_fifo overflow 5 times continuous
#define INT_TYPE_AUTO_RATE_ADJUST	        (0x07)          // Indicate driver should up or down tx rate
#define INT_SUB_TYPE_AUTO_RATE_UP	        (0x01)          // Indicate driver should up tx rate
#define INT_SUB_TYPE_AUTO_RATE_DOWN	        (0x02)          // Indicate driver should up down rate

#define INT_READ_POWER_TABLE                        (0x0b)
#define INT_TYPE_RESET_HARDWARE	               (0x0d)          // Indicate driver should reset with different fw code
	#define INT_SUB_TYPE_RESET_WITH_SINGLE_MODE	        (0x00)          // Indicate driver should reset hw with single mode
	#define INT_SUB_TYPE_RESET_WITH_COMBO_MODE	        (0x01)          // Indicate driver should reset hw with combo mode
	//define this as wait cmd for BT
	//#define INT_SUB_TYPE_RESET_WITH_WAIT_MODE	        (0x01)

	//#define INT_SUB_TYPE_RESET_WITH_WATI_MODE	        (0x03)          // Indicate driver should reset then wait another interface finished real hw reset
	//#define INT_SUB_TYPE_RESET_OK_MODE	        			 (0x04)          // Indicate driver should reset then wait another interface finished real hw reset	

#define INT_TYPE_RETRY_LIMIT_EVENT	               (0x11)          // Indicate driver should reset with different fw code
#define INT_TYPE_WRITE_MAILBOX	               (0x12)          // Indicate driver should write a mailbox reg
#define INT_UNENCRYPT                       (0x1a)

#define INT_RETURN_URB_BUFF_LEN         (8)             // As our data format defined, fixed len 8
#define INT_TX_HANG_HAPPEN         		(0xfa)             // As our data format defined, fixed len 8
//20090511 SoftHotKey
#define INT_SOFT_HOTKEY				(0x5A)     //when hotkey pressed, 8051 send this interrupt
// fdeng mask this !
//
//#pragma pack(1)

typedef enum
{
	INT_8051_IN_MINILOOP_MODE,	// Indicate driver that 8051 is in miniloop mode
	INT_8051_IN_WORKING_MODE,	// Indicate driver that 8051 is in normal working mode
} hw_8051_work_mode_t;

//
// This structure contains the information about receive area block which is 
// used to store the packet and buffer indicated to the up layer.
// 
//
/*
typedef struct _FW_HEAD{
       UINT8   		request_cmd;
	UINT16         offset;     //the frag's offset
	UINT16         fraglen;   // len of the fw frag
} FW_HEAD_T, *PFW_HEAD_T;
*/

typedef enum
{
	MACHW_CONTENTION_FIFO,
	MACHW_CONTENTION_FREE_FIFO,
	MACHW_PRIORITY_FIFO,
	MACHW_BEACON_FIFO
} machw_fifo_name_t; /* equates for machw.fifoSel. Do not change order. */
typedef struct m2bMailbox1_tag
{
#ifdef WLAN_BT_CRITICAL_SECTION
    UINT32 channel_num : 8, bcn_len : 12, : 3, wlan_only : 1, txpwrlevel : 6, antenna_sel : 1, slottime : 1;
#else
	UINT32 channel_num : 8, bcn_len : 12, : 3,use_pd:1, txpwrlevel : 6, antenna_sel : 1, slottime : 1;
#endif
    UINT32 : 16, bb_int : 1, gen_int1 : 1, gen_int2 : 1, : 13;
} m2bMailbox1_t; //the abstract of bbreg16_18 and bbreg36_38

typedef struct ctsPreamTypeRate_tag
{
	UINT32 bit_rate : 2, : 14, preamble_type : 1, : 14, correlator_off : 1; 
} ctsPreamTypeRate_t; 


/* define according to b8h*/
typedef struct _USB_DSP_INTERFACE{
      UINT32    start_address : 16;
      UINT32    activate : 1;
      UINT32    r_w : 1;	  //0 read /1 write
      UINT32    s_b : 1;	  //0 single /1 burst
      UINT32    increase_auto : 1;	  //0 auto , 1 no-auto
      UINT32    reserved : 12;
} USB_DSP_INTERFACE_T, *PUSB_DSP_INTERFACE_T;
typedef struct MAC_CTL_REG_TAG
{
	UINT32 reset:1,dot11a:1,bsstype:1,isap:1,dupfilteren:1,promen:1,fcscheckdisable:1,multirxdis:1,basicrate:8;
	UINT32 dtim:1,dot11g:1,setacw:1,excUnencrypted:1,wrUndecryptedRx:1,rxAnyBssidBeacon:1,enableEcoItem:1,cfPollableSTA:1,
		rxFIFOFlush:1,txFIFOFlush:1,reserved1:2,Ignore_ID_on_receive:1,pwrMgt:1,reserved2:1,byteSwapEn:1;
} MAC_CTL_REG_T; //the abstract of MAC CTRL REG 0X401C


// fdeng
#pragma pack(1)


typedef struct _DSP_FW_HEAD{
       UINT8   		request_cmd;
	UINT16         offset;     //the frag's offset
	UINT16         fraglen;   // len of the fw frag
	UINT32           src_word;
	UINT32 		dst_word;
	UINT32 		ctrl_word;
} DSP_FW_HEAD_T, *PDSP_FW_HEAD_T;

typedef struct _DSP_AMEM{
       UINT8   		request_cmd;
	UINT16         offset;     //the frag's offset
	UINT16		len;
	UINT32           src_word;
	UINT32 		dst_word;
	UINT32 		ctrl_word;
	union
    	{
		UINT32		m2bMailbox1_u[2];
		m2bMailbox1_t	m2bMailbox1;
    	};
} DSP_AMEM_T, *PDSP_AMEM_T;


typedef struct _DSP_FW_DOWN_OK{
    UINT8   		request_cmd;
	UINT8           autorate;
	UINT8           fw_type;    //0
	UINT8           reserve;    //0x10B
	UINT8           mainloop_type;    //0x10C

} DSP_FW_DOWN_OK_T, *PDSP_FW_DOWN_OK_T;

typedef struct _DSP_FLUSH_RXFIFO{
       UINT8   		request_cmd;
} DSP_FLUSH_RXFIFO_T, *PDSP_FLUSH_RXFIFO_T;

typedef struct _DSP_ENCRYPTION_MODE{
	UINT8		request_cmd;
	UINT8		wep_mode;
	UINT8		group_mode;
}DSP_ENCRYPTION_MODE;
typedef struct _DSP_POWERTABEL_MODE{
	UINT8		request_cmd;
	UINT16         offset;
}DSP_POWERTABEL_MODE;
typedef struct _DSP_8051FW_VERSION{
	UINT8		request_cmd;
}DSP_8051FW_VERSION;

//Jakio20070619:  request 8051 to set channel
typedef struct _DSP_CHANNEL_INFO{
	UINT8		request_cmd;
	UINT8		chan;
}DSP_CHANNEL_INFO,  *PDSP_CHANNEL_INFO;

typedef struct _DSP_ABORT_PIPE{
	UINT8		request_cmd;
	UINT8		endpoint_num;	//bulck in:1	bulck out:2		
}DSP_ABORT_PIPE;

typedef struct _DSP_RESET_REQUEST{
       UINT8   		request_cmd;
      UINT8 		type;
} DSP_RESET_REQUEST_T, *PDSP_RESET_REQUEST_T;

typedef struct _DSP_RESET_BULKOUT_REQUEST{
       UINT8   		request_cmd;
} DSP_RESET_BULKOUT_REQUEST_T, *PDSP_RESET_BULKOUT_REQUEST_T;


typedef struct _DSP_SET_CHANNEL_REQUEST{
       UINT8   		request_cmd;
	UINT32		src_addr;
	UINT32		dst_addr;
	UINT32		ctl_word;
	UINT8		channel[8];
} DSP_SET_CHANNEL_REQUEST_T, *PDSP_SET_CHANNEL_REQUEST_T;
//interrupt data of read dsp register
typedef struct _DSP_REG_READ{
	UINT8	type;
	UINT8	sub_type;
	UINT16  addr;
	UINT32	result;
}DSP_REG_READ, *PDSP_REG_READ;

typedef struct _DSP_WRITE_MAILBOX{
	UINT8	type;
	UINT8	sub_type;
	UINT16  addr;
	UINT32	val;
}DSP_WRITE_MAILBOX, *PDSP_WRITE_MAILBOX;

typedef struct _DSP_CONFIG_TXFIFO{
	UINT8	type;
	UINT8    subtype;                 //0: only flush cp fifo  1: config all tx fifo
	UINT32	size;
}DSP_CONFIG_TXFIFO, *PDSP_CONFIG_TXFIFO;

typedef struct {
	UINT8		channel;
	UINT32		frequency;
	BOOLEAN		used_usa;
	BOOLEAN		used_europe;
	BOOLEAN		used_japan;
	BOOLEAN		used_rest_of_world;
} channel_list_t;

typedef struct _DSP_GET_8051_STATUS{
	UINT8	type;
//	UCHAR    subtype;                
//	ULONG	size;
}DSP_GET_8051_STATUS, *PDSP_GET_8051_STATUS;


//
// This structure contains the information about receive area module
// 
//

TDSP_STATUS
	Adap_Set_SoftReset(PDSP_ADAPTER_T pAdap, UINT16 bitmap);
TDSP_STATUS
	Vcmd_Set_Firmware_Download_Ok(PDSP_ADAPTER_T pAdap, BOOLEAN		bMainLoopTrue);
TDSP_STATUS 
	Vcmd_Reset_Bulkout_Request(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 
	Vcmd_Set_Encryption_Mode(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Vcmd_Set_8051_MinLoop(PDSP_ADAPTER_T pAdap, UINT8 isIndicateBT);
TDSP_STATUS 
	Vcmd_Funciton_Req_JOIN(PDSP_ADAPTER_T pAdap);
BOOLEAN 
	Vcmd_8051fw_isWork(PDSP_ADAPTER_T pAdap);
TDSP_STATUS
	Adap_Set_Power_Management_Mode(PDSP_ADAPTER_T pAdap, BOOLEAN IsPowerMng,BOOLEAN ps_poll_flag);
TDSP_STATUS
	Adap_Set_Inform_Force_Sending(PDSP_ADAPTER_T pAdap, BOOLEAN IsBulkin);

TDSP_STATUS
	Fw_download_8051fw_fragment(PDSP_ADAPTER_T pAdap,
								   PUINT8 buf,
								   UINT16 len,
								   UINT16 offset
								   );
TDSP_STATUS Fw_bulkin_dspfw_fragment(PDSP_ADAPTER_T pAdap,
                                     PUINT8 buf,
                                     UINT16 len,
                                     UINT16 offset);

TDSP_STATUS Fw_download_dspfw_fragment(PDSP_ADAPTER_T pAdap,
								   PUINT8 buf,
								   UINT16 len,
								   UINT16 offset);
TDSP_STATUS
Vcmd_reset_3dsp_request(PDSP_ADAPTER_T pAdap);



TDSP_STATUS
Vcmd_NIC_ENABLE_RETRY(PDSP_ADAPTER_T pAdap,UINT32 value);

TDSP_STATUS
Vcmd_NIC_RESET_ALL_BUT_HARDPCI(PDSP_ADAPTER_T pAdap);


TDSP_STATUS 
Vcmd_set_beacon_interval(PDSP_ADAPTER_T pAdap,UINT32 interval,UINT32 tbtt);


TDSP_STATUS 
Vcmd_set_rts_retrylimit(PDSP_ADAPTER_T pAdap,UINT32 rts_threshold,UINT32 s_retry,UINT32 l_retry);


TDSP_STATUS    	
Vcmd_hal_set_dtim_cfp_param(
	PDSP_ADAPTER_T pAdap,
	UINT32 	dtim_period,
	UINT32    cfp_period,
	UINT32 	cfp_max_duration);

TDSP_STATUS
Vcmd_set_power_mng(
	PDSP_ADAPTER_T pAdap);

TDSP_STATUS GetTxPwrTable_from_eeprom(PDSP_ADAPTER_T pAdap);


TDSP_STATUS
Vcmd_reset_power_mng(
	PDSP_ADAPTER_T pAdap);

TDSP_STATUS
Vcmd_get_current_state(
	PDSP_ADAPTER_T pAdap,PUINT32 state);


VOID 
Vcmd_set_next_state(
	PDSP_ADAPTER_T pAdap,
	UINT8 state, 
	UINT8 scan_type, 
	UINT16 time_value);


VOID
Vcmd_unmask_idle_intr(
	PDSP_ADAPTER_T pAdap);


VOID
Vcmd_clear_intrpt(
	PDSP_ADAPTER_T pAdap,
	UINT32 interupt);


TDSP_STATUS 
VcmdW_3DSP_Dword(
  			PDSP_ADAPTER_T	 pAdap,
  			UINT32			 value,
  	      		UINT16 			offset
			);
TDSP_STATUS 
Basic_WriteBurstRegs(
			PDSP_ADAPTER_T pAdap,
			PUINT8 buf,
			UINT16 len,
			UINT16 offset
			);

TDSP_STATUS Basic_ReadBurstRegs_Dut(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset
						   );
UINT8 
Basic_ReadRegByte(
			PDSP_ADAPTER_T pAdap,
			UINT16 offset
			);
TDSP_STATUS 
Basic_WriteRegByte(
			PDSP_ADAPTER_T pAdap,
			UINT8  value,
			UINT16 offset
			);

TDSP_STATUS Basic_ReadBurstRegs(
		PDSP_ADAPTER_T pAdap,
	   	PUINT8 buf,
	   	UINT16 len,
	   	UINT16 offset
	   	);
	
TDSP_STATUS Basic_ReadBurstRegs_single(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset//,
//						   UINT16 singles
						   );

TDSP_STATUS test_stall(PDSP_ADAPTER_T pAdap);


TDSP_STATUS 
Vcmd_Send_API(
	PDSP_ADAPTER_T	 pAdap,
	PUINT8			 buff,
	UINT16 		        len
);

TDSP_STATUS Adap_hal_set_rx_any_bssid_beacon(
	PDSP_ADAPTER_T pAdap//, 
	//UINT32 val
	);


BOOLEAN VCMD_VenderCmd_BUSY(PDSP_ADAPTER_T pAdap);

UINT8 Fw_UsbClearStatus(PDSP_ADAPTER_T pAdap,UINT8 wIndex,UINT8 status);



TDSP_STATUS
Vcmd_hal_select_tx_fifo(PDSP_ADAPTER_T pAdap,UINT32 fifo_num);


VOID
Adap_ReadMacAddress(PDSP_ADAPTER_T pAdap,
								PUINT8 addr
								);
VOID
	Adap_ReadFirmwareVersion(PDSP_ADAPTER_T pAdap);
TDSP_STATUS
	Adap_WriteMacAddress(PDSP_ADAPTER_T pAdap,
						  PUINT8 addr
						  );
extern TDSP_STATUS
Fw_Start_8051fw(PDSP_ADAPTER_T pAdap);

TDSP_STATUS Fw_Stop_8051fw(PDSP_ADAPTER_T pAdap);

extern TDSP_STATUS 
Vcmd_CardBusNICReset(PDSP_ADAPTER_T pAdap);

extern TDSP_STATUS
Vcmd_CardBusNICEnable(PDSP_ADAPTER_T pAdap);

extern TDSP_STATUS
Vcmd_NIC_INTERRUPT_DISABLE(PDSP_ADAPTER_T pAdap);

extern TDSP_STATUS
Vcmd_NIC_INTERRUPT_ENABLE(PDSP_ADAPTER_T pAdap);

//TDSP_STATUS Vcmd_Write_Fwctrlword(
//  		PDSP_ADAPTER_T	 pAdap,
//  		UINT16			 offset,
//  		UINT16 		        len
//);


TDSP_STATUS
	Vcmd_Transmit_DSPFw_Head(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT16			 offset,
  		UINT32 		        len,
  		UINT8 			 file_id
	);
TDSP_STATUS
	VcmdW_3DSP_Dword(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT32			 value,
  	       UINT16 			offset
			);

TDSP_STATUS VcmdW_3DSP_Mailbox(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT32			 value,
  	    UINT16 			offset,
		PVOID      caller_name);


UINT32
	VcmdR_3DSP_Dword(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset);
//	Begin Added Joe 2007-08-23
//	For Read Program Area.
UINT32
	VcmdR_Program_Dword(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset);
//	End Added 

UINT32 VcmdR_PowerTable(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset);
TDSP_STATUS 
Vcmd_scratch_2_DSP_Amemoffset
(PDSP_ADAPTER_T pAdap,VOID *Buffer, UINT32 len, UINT32 destoffset); 


TDSP_STATUS 
Vcmd_mmacSp20ResetRemoved(PDSP_ADAPTER_T pAdap);


TDSP_STATUS 
	Vcmd_CardBusNICStart(PDSP_ADAPTER_T pAdap);

TDSP_STATUS
Vcmd_flush_rx_fifo(PDSP_ADAPTER_T pAdap);


TDSP_STATUS 
Vcmd_mmacSp20ResetApplied(PDSP_ADAPTER_T pAdap);


UINT32
Vcmd_get_version(PDSP_ADAPTER_T pAdap);


TDSP_STATUS 
Vcmd_hal_reset(PDSP_ADAPTER_T pAdap);

#if 0//justin: 0716 duplicate function
TDSP_STATUS 
Vcmd_hal_set_mac_addr(PDSP_ADAPTER_T pAdap,PUINT8 addr);
#endif 

TDSP_STATUS 
Vcmd_hal_set_bssid(PDSP_ADAPTER_T pAdap,PUINT8 addr);

TDSP_STATUS
Vcmd_mlme_config_tx_fifo(
	PDSP_ADAPTER_T pAdap,
    	UINT16	cp_size,
    	UINT16	cfp_size,
    	UINT16	bcn_size);

TDSP_STATUS
VcmdW_3DSP_Regs(
  		PDSP_ADAPTER_T	 pAdap,
  		PUINT8			 buff,
  		UINT16 		        len,
  	       UINT16 offset
			);

BOOLEAN Adap_Device_Removed(PDSP_ADAPTER_T pAdap);

void Fw_print_busy_regs(PDSP_ADAPTER_T pAdap);
//
//#pragma pack()


TDSP_STATUS Vcmd_Funciton_Set_BTPara_Ready(PDSP_ADAPTER_T pAdap);


#endif /*file end */

