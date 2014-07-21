 /****************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  *
  * FILENAME:     DSP_Adap.c      CURRENT VERSION: 1.00.01
  * PURPOSE:       This is the main file for the 3DSP Corporation DSP 802.11
  *				   wireless LAN controller. This driver conforms to the NDIS 5.1
  *				   miniport interface.
  *
  *
  * DECLARATION:  This document contains confidential proprietary information that
  *               is solely for authorized personnel. It is not to be disclosed to
  *               any unauthorized person without prior written consent of 3DSP
  *               Corporation.
  *
  **********************************************************************/
//#include <linux/usb.h>
#include "precomp.h"
#include "usbwlan_UsbDev.h"
static char* TDSP_FILE_INDICATOR="VENDR";
//#include "usbwlan_vendor.h"
//#include "usbwlan_UsbDev.h"
//#include "usbwlan_Pr.h"
//#include "tdsp_time.h"
/*
#include <ndis.h>
#include <initguid.h>
#include "usbwlan_defs.h"
#include "usbwlan_Sw.h"
#include "usbwlan_8051code.h"
#include "usbwlan_dbg.h"
#include "usbwlan_vendor.h"
#include "usbwlan_tx.h"
#include "usbwlan_mng.h"
#include "usbwlan_pr.h"
*/	
/*--file local constants and types-------------------------------------*/


#define BASIC_VCMD_FUNCTION

const channel_list_t static_axLookupFreqByChannel_BOnly[] = 
{//   ch    freq      US     EUR   JAPAN  WORLD
	{ 1,   2412000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 2,   2417000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 3,   2422000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 4,   2427000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 5,   2432000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 6,   2437000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 7,   2442000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 8,   2447000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 9,   2452000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 10,  2457000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 11,  2462000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 12,  2467000, FALSE,  TRUE,  TRUE,  TRUE },
	{ 13,  2472000, FALSE,  TRUE,  TRUE,  TRUE },
	{ 14,  2484000, FALSE, FALSE,  TRUE, FALSE },
	{ 0, 0, FALSE, FALSE, FALSE, FALSE }
};




const channel_list_t static_axLookupFreqByChannel_ABG[] = 
{//   ch    freq      US     EUR   JAPAN  WORLD
	{ 1,   2412000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 2,   2417000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 3,   2422000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 4,   2427000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 5,   2432000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 6,   2437000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 7,   2442000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 8,   2447000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 9,   2452000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 10,  2457000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 11,  2462000,  TRUE,  TRUE,  TRUE,  TRUE },
	{ 12,  2467000, FALSE,  TRUE,  TRUE,  TRUE },
	{ 13,  2472000, FALSE,  TRUE,  TRUE,  TRUE },
	{ 14,  2484000, FALSE, FALSE,  TRUE, FALSE },
	{ 34,  5170000, FALSE, FALSE,  TRUE, FALSE },
	{ 36,  5180000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 38,  5190000, FALSE, FALSE,  TRUE, FALSE },
	{ 40,  5200000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 42,  5210000, FALSE, FALSE,  TRUE, FALSE },
	{ 44,  5220000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 46,  5230000, FALSE, FALSE,  TRUE, FALSE },
	{ 48,  5240000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 52,  5260000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 56,  5280000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 60,  5300000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 64,  5320000,  TRUE,  TRUE, FALSE,  TRUE },
	{ 100, 5500000, FALSE,  TRUE, FALSE,  TRUE },
	{ 104, 5520000, FALSE,  TRUE, FALSE,  TRUE },
	{ 108, 5540000, FALSE,  TRUE, FALSE,  TRUE },
	{ 112, 5560000, FALSE,  TRUE, FALSE,  TRUE },
	{ 116, 5580000, FALSE,  TRUE, FALSE,  TRUE },
	{ 120, 5600000, FALSE,  TRUE, FALSE,  TRUE },
	{ 124, 5620000, FALSE,  TRUE, FALSE,  TRUE },
	{ 128, 5640000, FALSE,  TRUE, FALSE,  TRUE },
	{ 132, 5660000, FALSE,  TRUE, FALSE,  TRUE },
	{ 136, 5680000, FALSE,  TRUE, FALSE,  TRUE },
	{ 140, 5700000, FALSE,  TRUE, FALSE,  TRUE },
//#if defined(BASEBAND_TEST_GUI)
	{ 149, 5745000,  FALSE, FALSE, FALSE,  TRUE },    // US column changed to false because current RF chip does not support
	{ 153, 5765000,  FALSE, FALSE, FALSE,  TRUE },    // " change back when new RF chip
	{ 157, 5785000,  FALSE, FALSE, FALSE,  TRUE },    // "
	{ 161, 5805000,  FALSE, FALSE, FALSE,  TRUE },    // "

	{ 184, 4920000, FALSE, FALSE, FALSE, FALSE },    // These channels are not used in any country, but we want to support them in the baseband
	{ 188, 4940000, FALSE, FALSE, FALSE, FALSE },    // tester because the radio has this capability
	{ 192, 4960000, FALSE, FALSE, FALSE, FALSE },    // "
	{ 196, 4980000, FALSE, FALSE, FALSE, FALSE },    // "
	{ 208, 5040000, FALSE, FALSE, FALSE, FALSE },    // "
	{ 212, 5060000, FALSE, FALSE, FALSE, FALSE },    // "
	{ 216, 5080000, FALSE, FALSE, FALSE, FALSE },    // "
//#endif
	{ 0, 0, FALSE, FALSE, FALSE, FALSE }
} ;
TDSP_STATUS Basic_VendorCommand_Dut( PDSP_ADAPTER_T pAdap,
							   UINT8 direction,
                               PUINT8 buf,
							   UINT16 len,
							   UINT8 request,
							   UINT16 value,
							   UINT16 index
							   )
{
{
	PUSB_CONTEXT usb_context;
	INT32 ret;

	usb_context = (PUSB_CONTEXT )pAdap->usb_context;

	ret = UsbDev_BuildVendorRequest(usb_context,
        							    direction,
    								    buf,
									    len,
									    request,
									    value,
									    index);

	if (ret < 0)
	{

        DBG_WLAN__HAL(LEVEL_ERR,"[%s] vendor Request failure,len=%x, request=%x,value=%x,index=%x,ret=%x\n",
                                __FUNCTION__,
							    len,
							    request,
							    value,
							    index,
							    ret);
        
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

}
/*******************************************************************
 *   Adap_VendorCommand
 *   
 *   dsp ok
 *   Descriptions:
 *      This routine is used to submit all the SYNCHRONOUS URBS to the 
 *  	device.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.
 *      request: IN, Device specific command request code.
 *      value: IN, USB URB Value.
 *      index: IN, USB URB Index.
 *      direction: IN/OUT.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Basic_VendorCommand( PDSP_ADAPTER_T pAdap,
							   UINT8 direction,
                               PUINT8 buf,
							   UINT16 len,
							   UINT8 request,
							   UINT16 value,
							   UINT16 index
							   )
{
	PUSB_CONTEXT usb_context;
	INT32 ret;

	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s]: Access reg fail due to device in halt!\n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	usb_context = (PUSB_CONTEXT )pAdap->usb_context;

	ret = UsbDev_BuildVendorRequest(usb_context,
        							    direction,
    								    buf,
									    len,
									    request,
									    value,
									    index);

	if (ret < 0)
	{

        DBG_WLAN__HAL(LEVEL_ERR,"[%s] vendor Request failure,len=%x, request=%x,value=%x,index=%x,direction=%x\n",
                                __FUNCTION__,
							    len,
							    request,
							    value,
							    index,
							    ret);
				
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}


/*******************************************************************
 *   Basic_ReadRegByte
 *   
 *   dsp ok
 *   Descriptions:
 *      Read one byte from a register.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      offset: IN, offset of register.
 *   Return Value:
 *      the reading data.
 ******************************************************************/
UINT8 Basic_ReadRegByte(PDSP_ADAPTER_T pAdap,
							   UINT16 offset
							  )
{
/*
	Type	bmReqType	bRequest	  wValue	wIndex	wLength
	RegRead	C0			0		  N.A.	address	1
	RegWrite	40			1		  data	address	0
*/

	UINT8 tmp = 0;
	Basic_VendorCommand(pAdap,
        VCMD_READ_OP,		//direction   1=read
		&tmp,	//buf
		1,		//len
		VCMD_READ_REG,    //request
		0x0,		//value
		offset   //address
		);

	return tmp;
}


/*******************************************************************
 *   Basic_WriteRegByte
 *   
 *   dsp ok
 *   Descriptions:
 *      Write one byte to a register.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Basic_WriteRegByte(PDSP_ADAPTER_T pAdap,
						      UINT8 value,
							  UINT16 offset
						      )
{
/*
	Type	bmReqType	bRequest	  wValue	wIndex	wLength
	RegRead	C0			0		      N.A.	address	1
	RegWrite	40			1		  data	address	0
*/
	return (Basic_VendorCommand(pAdap,
	    VCMD_WRITE_OP,		//direction   0=read
		NULL,	//buf
		0,		//len
		VCMD_WRITE_REG,   //request
		value,		//value
		offset   //address
		));
}

TDSP_STATUS Basic_WriteBurstRegs_single(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset//,
//						   UINT16 singles
						   )
{
	TDSP_STATUS status = STATUS_SUCCESS;
	UINT16 i = 0;
	UINT8 value =0;
	while(i<len)
	{
		value = *(buf+i);
		Basic_WriteRegByte(pAdap,
						   value,
						   offset+i
						  );
		i++;
	}
	return status;
}

TDSP_STATUS Basic_ReadBurstRegs_single(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset//,
//						   UINT16 singles
						   )
{
/*	
	Type		bmReqType	bRequest	 wValue	wIndex		wLength
	BurstRegRead	C0			2		 N.A.	startaddress	length
*/
	UINT16 i = 0;
	UINT8 tmp =0;
	PUINT8 pBuf = NULL;
//	if(!singles)
//		return STATUS_FAILURE;
	while(i< len)
	{
		tmp = Basic_ReadRegByte(pAdap,offset+i);
		pBuf = buf+i;
		*pBuf = tmp; 
		i++;

	}
	return STATUS_SUCCESS;
}
/*******************************************************************
 *   Adap_ReadRegs
 *   dsp ok
 *   Descriptions:
 *      The routine of read many registers.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      buf: IN, the buffer saving the value.
 *      len: IN, the buffer's length wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Basic_ReadBurstRegs(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset
						   )
{
/*	Type		bmReqType	bRequest	 wValue	wIndex		wLength
	BurstRegRead	C0			2		 N.A.	startaddress	length
*/

	return (Basic_VendorCommand(
		pAdap,
		VCMD_READ_OP,    //read direction
		buf,
		len,
		VCMD_READ_BURSTREG,  //request
		0,		//value
		offset    //index, start address
		));
}


TDSP_STATUS Basic_ReadBurstRegs_Dut(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset
						   )
{
/*	Type		bmReqType	bRequest	 wValue	wIndex		wLength
	BurstRegRead	C0			2		 N.A.	startaddress	length
*/

	return (Basic_VendorCommand_Dut(
		pAdap,
		VCMD_READ_OP,    //read direction
		buf,
		len,
		VCMD_READ_BURSTREG,  //request
		0,		//value
		offset    //index, start address
		));
}
/*******************************************************************
 *   
 *   dsp ok
 *   Descriptions:
 *      The routine of write many registers.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      buf: IN, the buffer saving the value.
 *      len: IN, the buffer's length wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Basic_WriteBurstRegs(PDSP_ADAPTER_T pAdap,
						   PUINT8 buf,
						   UINT16 len,
						   UINT16 offset
						   )
{
/*	Type	       bmReqType	bRequest	wValue	wIndex		wLength
	BurstRegWrite	40			3		N.A.		startaddress	length
*/
    UINT8 value;
	if (len == 0)
	{
		return (STATUS_NOT_SUPPORTED);
	}
	if(len == 1)
	{
		//Patch for epc control pipe fail with c0000001
		//for one byte,use single mode is ok when usb1.1 running
		value = (UINT8)(*buf);
		return Basic_WriteRegByte(
			pAdap,
			value,
			offset);
	}
	return (Basic_VendorCommand(
		pAdap,
		VCMD_WRITE_OP,    //direction
		buf,
		len,
		VCMD_WRITE_BURSTREG,    //request
		0,   //value is null, data in buff
		offset //startaddress
		));
}


//the above functions are basic function to access hw register

#define VCMD_FUNCTION
UINT8 Vcmd_ReadMailBoxFlag(
		PDSP_ADAPTER_T	 pAdap)
{
	return Basic_ReadRegByte(pAdap,REG_MAILBOX_HEAD);
}


/*
	get vendor command byte from command space(wlan - 0x108, bt - 0x128)
	command byte is written by host and cleaned by 8051 after command finished.
*/
UINT8 Vcmd_ReadVenderCmdFlag(
		PDSP_ADAPTER_T	 pAdap)
{
	return Basic_ReadRegByte(pAdap, REG_MAILBOX_CMD);
}


/*
	get the byte content that activate flag is included 
	offset is baH
*/
UINT8 Vcmd_ReadActivateBits(
			PDSP_ADAPTER_T	 pAdap)
{
	return Basic_ReadRegByte(pAdap,REG_RWCR + 2); 
}

#define VCMD_EXTENT_FUNCTION

/*
	now the function is not used beacuse mailbox head is defined for packet length cmd.
	according sending packet flow, host needn't check if previous cmd has been done.
*/
BOOLEAN VCMD_MAILBOX_BUSY(PDSP_ADAPTER_T pAdap)
{
	UINT8 tmp;
	tmp = Vcmd_ReadMailBoxFlag(pAdap);
	tmp &= (UINT8)MAILBOX_BUSY_BITS;
	return (tmp == 0) ? FALSE: TRUE;
}


/*
	check if previous vendor command has been finished
	return value:
	   true:  indicate previous command has been done.
	   false: indicate previous command has not been done.
*/
BOOLEAN VCMD_VenderCmd_BUSY(PDSP_ADAPTER_T pAdap)
{
	UINT8 tmp;
	tmp = Vcmd_ReadVenderCmdFlag(pAdap);
	tmp &= (UINT8)VENDERCMD_BUSY_BITS;
	return (tmp == 0) ? FALSE: TRUE;
}


/*
	if activate flag is set
	bit0 of bah
*/
BOOLEAN VCMD_ACTIVATE_BUSY(PDSP_ADAPTER_T pAdap)
{
	UINT8 tmp;
	tmp = Vcmd_ReadActivateBits(pAdap);
	tmp &= (UINT8)BIT_3DSP_ACTIVE_RW;
	return (tmp == 0) ? FALSE: TRUE;
}

/*
	Jakio20070426: finish this routine
	write 3dsp register with burst mode
*/
TDSP_STATUS VcmdW_3DSP_Regs(
  		PDSP_ADAPTER_T	 pAdap,
  		PUINT8			 buff,
  		UINT16 		    len,
  	    UINT16 offset)
{
	/*
	//wait until previous access end
	while(!VCMD_ACTIVATE_BUSY(pAdap));
	
	return Basic_WriteBurstRegs(pAdap,buff,len,offset);
	*/

	/*
		upate it later
	*/
	UINT16 i = 0;
	TDSP_STATUS status = STATUS_SUCCESS;

	//Jakio: only support DWORD write
    if((len % 4 != 0) || (len == 0))
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s]:only support DWORD write,len is %d\n",
                __FUNCTION__,
                len);
    }

	for(i = 0; i < len/4; i++)
	{
		status = VcmdW_3DSP_Dword(pAdap, *(PUINT32)(buff + i*4), (offset + i*4));
		if(status != STATUS_SUCCESS)
			break;
	}
	
	return status;
	
}

 void Fw_print_busy_regs(PDSP_ADAPTER_T pAdap)
{
    UINT8 uTemp[100];
    UINT32 i;

    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0x00);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x00): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);
    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0x20);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x20): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0x30);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0x40);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x40): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0x50);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x50): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0xb0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,4,0xc0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0xc0): %X:%X:%X:%X \n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);


    for(i = 0 ; i <= 0xff ; i++)
    {
        Basic_ReadBurstRegs_single(pAdap,uTemp,4,i);
        DBG_WLAN__HAL(LEVEL_TRACE, "base(0x%X): %X:%X:%X:%X \n",
          i,uTemp[0],uTemp[1],uTemp[2],uTemp[3]);
    
    }
 
}
/*
	write 3dsp register with dword size
	single mode is used for the write 
*/
TDSP_STATUS VcmdW_3DSP_Dword_Old(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT32			 value,
  	       UINT16 			offset
			)
{
	UINT32 	loop = 10000;
	UINT32    ctl = 0;
	UINT8    buff[8];
	//wait until previous access end
	while(VCMD_ACTIVATE_BUSY(pAdap) && loop--);

	//loop to 0, fail
	if(loop == 0)
	{
		return STATUS_FAILURE;		
	}

	//cp value to buff
	sc_memory_copy(buff,&value,sizeof(UINT32));

	//cp ctl word to buff
	//write:1 | single:0 | increase auto:0 |activate :1
	ctl = (UINT32)offset;
 	ctl |= (BIT16|BIT17);
	sc_memory_copy(buff + sizeof(UINT32),&ctl,sizeof(UINT32));
 	
	//put value to b4h register
	if(STATUS_SUCCESS != Basic_WriteBurstRegs(pAdap,buff,2*sizeof(UINT32),REG_SWDR))//0xb4))
	{
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;	
}



UINT32 VcmdR_3DSP_Dword_Old(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset)
{
	UINT32 	loop = 10000;
	UINT32    ctl = 0;
	UINT32    val = 0;
	//wait until previous access end
	while(VCMD_ACTIVATE_BUSY(pAdap) && loop--);
	if(loop == 0)
	{
		goto err_exit;
	}

	//send read command
	ctl = (UINT32)offset;
 	ctl |= (BIT16);
	if(STATUS_SUCCESS != Basic_WriteBurstRegs(pAdap,(PUINT8)&ctl,sizeof(UINT32),REG_RWCR))//0xb8))
	{
		goto err_exit;
	}

	//get read result
	loop = 10000;
	while(VCMD_ACTIVATE_BUSY(pAdap) && loop--);
	if(loop == 0)
	{
		goto err_exit;
	}

	//get result	
	if(STATUS_SUCCESS != Basic_ReadBurstRegs(pAdap, (PUINT8)&val, sizeof(UINT32),REG_SRDR))//0xb0))
	{
		goto err_exit;
	}
	return val;
err_exit:
	ASSERT(0);
	return 0;
}


TDSP_STATUS VcmdW_3DSP_Mailbox(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT32			 value,
  	    UINT16 			offset,
		PVOID           caller_name)
{
	DSP_WRITE_MAILBOX req;
	UINT32           loop = 10;
	TDSP_STATUS  status;

	req.type = INT_TYPE_WRITE_MAILBOX;
	req.sub_type = 0;
	req.addr = offset;
	req.val = value;
	
	while(loop)
	{
		status = Vcmd_Send_API(pAdap,(PUINT8)&req, sizeof(DSP_WRITE_MAILBOX));
		if(STATUS_ADAPTER_NOT_READY != status)
		{
			break;
		}	
		DBG_WLAN__HAL(LEVEL_ERR,"VcmdW_3DSP_Mailbox() return error,c= %x,caller name is %s\n",loop,caller_name);
		loop--;
		sc_sleep(2);   // 2ms
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR," VcmdW_3DSP_Mailbox() ERROR EXIT\n");
	}
	return status;	
}
/*
	Write 3dsp register 4bytes one time
	two steps should be done:
	1: write command data into vendor data space  (0x109)
	2: write command type into vendor type space(0x108)
*/
TDSP_STATUS VcmdW_3DSP_Dword(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT32			 value,
  	       UINT16 			offset
			)
{
	UINT32 	loop = VENDOR_LOOP_MAX;
	UINT8	cmd_type = VCMD_REQ_WRITE_PCI_DWORD;

	//if(pAdap->driver_state == DSP_DRIVER_HALT)
	//woody 2008
	if(Adap_Driver_isHalt(pAdap))
	{
		return STATUS_FAILURE;
	}
	//wait until previous access end
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);

	//loop to 0, fail
	if(loop == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s] W COMMAND timeout due to busy\n",__FUNCTION__);
        Fw_print_busy_regs(pAdap);
		return STATUS_FAILURE;		
	}

	//first write content of command into 8051 
	if(STATUS_SUCCESS != 
		Basic_WriteBurstRegs(pAdap, (PUINT8)&offset, sizeof(UINT16), REG_MAILBOX_DATA))
	{
		return STATUS_FAILURE;	
	}
	if(STATUS_SUCCESS != 
	Basic_WriteBurstRegs(pAdap, (PUINT8)&value, sizeof(UINT32), REG_MAILBOX_DATA+2))
	{
		return STATUS_FAILURE;	
	}


	//then write command type
	if(STATUS_SUCCESS != 
		Basic_WriteBurstRegs(pAdap, (PUINT8)&cmd_type, 1, REG_MAILBOX_CMD))
	{
		return STATUS_FAILURE;	
	}
	//4 
	return STATUS_SUCCESS;	
}


/*
	Read 3dsp register 4bytes one time
	three steps should be done:
	1: write command data into vendor data space  (0x109)
	2: write command type into vendor type space(0x108)
	3: wait 8051 return register value
*/
UINT32 VcmdR_3DSP_Dword(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset)
{
	UINT16 	loop = VENDOR_LOOP_MAX;
	UINT8    cmd_type = VCMD_REQ_READ_PCI_DWORD;
	UINT32    retrynum = 5;

    loop = VENDOR_LOOP_MAX;

	//if(pAdap->driver_state == DSP_DRIVER_HALT)
	//woody
	if(Adap_Driver_isHalt(pAdap))
	{
		return 0;
	}
readloop:	
	//wait until previous access end
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);
	if(loop == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s] R COMMAND timeout due to busy,read offset = %x\n",__FUNCTION__,offset);
		goto err_exit;
	}

	//4 reset event
	sc_event_reset(&pAdap->DspReadEvent);
	
	
	//first write data into 8051
	if(STATUS_SUCCESS != 
		Basic_WriteBurstRegs(pAdap, (PUINT8)&offset, sizeof(UINT16), REG_MAILBOX_DATA))
	{
		goto err_exit;
	}
	// then write cmd type into 8051
	if(STATUS_SUCCESS !=
		//Patch for epc control pipe fail with c0000001
		//for one byte,use single mode is ok when usb1.1 running
		Basic_WriteRegByte(pAdap,cmd_type,REG_MAILBOX_CMD))
	{
		goto err_exit;
	}
	
	//wait event,due to read content returned by interrupt pipe
	if (0 == sc_event_wait(&pAdap->DspReadEvent, VCMD_WAIT_TIME_READ_PCI))
	{
		goto err_exit;
	}
	else
	{
	//must confirm why format is different from I know
		//4 check valid, the int data is saved in pAdap->ReadDspBuffer
		if(pAdap->DspRegRead.type != INT_TYPE_READ_3DSP_REG)
			goto err_exit;
		return pAdap->DspRegRead.result;
	}
	
err_exit:
	if(retrynum >0)
	{
		retrynum--;
		goto readloop;
	}
	//ASSERT(0);
	return 0;
}

UINT32 VcmdR_PowerTable(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset)
{
	TDSP_STATUS status;
	DSP_POWERTABEL_MODE   val;
	val.request_cmd = VCMD_API_READ_POWER_TABLE;
	val.offset = offset;
	

//readloop:	
	//wait until previous access end
	
	//4 reset event
	sc_event_reset(&pAdap->DspReadEvent);
	
	status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_POWERTABEL_MODE));
	
	//first write data into 8051
	if(STATUS_SUCCESS != status)
	{
		goto err_exit;
	}

	//wait event,due to read content returned by interrupt pipe
	if (0 == sc_event_wait(&pAdap->DspReadEvent, VCMD_WAIT_TIME_READ_PCI))
	{
		goto err_exit;
	}
	else
	{
	//must confirm why format is different from I know
		//4 check valid, the int data is saved in pAdap->ReadDspBuffer
		if(pAdap->DspRegRead.type != VCMD_API_READ_POWER_TABLE)
			goto err_exit;
		return pAdap->DspRegRead.result;
	}
	
err_exit:
	
	DBG_WLAN__HAL(LEVEL_ERR,"VcmdR_PowerTable error exit\n");
	return 0;
}

//	Begin Added Joe 2007-8-23
//    For read Program Area size about 16K.

/*
	Read 3dsp register 4bytes one time
	three steps should be done:
	1: write command data into vendor data space  (0x109)
	2: write command type into vendor type space(0x108)
	3: wait 8051 return register value
*/
UINT32 VcmdR_Program_Dword(
 		PDSP_ADAPTER_T	 pAdap,
  	       UINT16 offset)
{
	UINT16 	loop = VENDOR_LOOP_MAX;
	UINT8    cmd_type = VCMD_REQ_READ_PROGRAM_DWORD;
	UINT32    retrynum = 5;


readloop:	
	//wait until previous access end
	loop = VENDOR_LOOP_MAX;
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);
	if(loop == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"R program COMMAND timeout due to busy\n");
		goto err_exit;
	}

	//4 reset event
	sc_event_reset(&pAdap->DspReadEvent);
	
	
	//first write data into 8051
	if(STATUS_SUCCESS != 
		Basic_WriteBurstRegs(pAdap, (PUINT8)&offset, sizeof(UINT16), REG_MAILBOX_DATA))
	{
		goto err_exit;
	}
	// then write cmd type into 8051
	if(STATUS_SUCCESS !=
		Basic_WriteBurstRegs(pAdap, (PUINT8)&cmd_type, sizeof(UINT8), REG_MAILBOX_CMD))
	{
		goto err_exit;
	}
	
	//wait event,due to read content returned by interrupt pipe
	if (0 == sc_event_wait(&pAdap->DspReadEvent, VCMD_WAIT_TIME_READ_PCI))
	{        
        DBG_WLAN__HAL(LEVEL_ERR,"wait dspreadevent timeout\n");
		goto err_exit;
	}
	else
	{
	//must confirm why format is different from I know
		//4 check valid, the int data is saved in pAdap->ReadDspBuffer
		if(pAdap->DspRegRead.type != INT_TYPE_READ_3DSP_REG)
			goto err_exit;
		return pAdap->DspRegRead.result;
	}
	
err_exit:
	if(retrynum >0)
	{
		retrynum--;
		goto readloop;
	}
	ASSERT(0);
	return 0;
}

//	End Added.
/*
	description:
	   send vendor command api to 8051.
	   the command is sent to vendor space of mailbox
*/
TDSP_STATUS Vcmd_Send_API(
  		PDSP_ADAPTER_T	 pAdap,
  		PUINT8			 buff,
  		UINT16 		        len)
{
	UINT32 			loop = VENDOR_LOOP_MIDDLE;
	TDSP_STATUS 	status = STATUS_SUCCESS;

	
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);

	if(loop == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"vendor error due to cmd busy\n");
		return STATUS_ADAPTER_NOT_READY;
	}

	//reset event
	//tdsp_event_reset(&pAdap->DspReadEvent);
	//first data field of cmd should be written into device
	if(len > VENDOR_CMD_HEAD_BYTES && len <= VENDOR_CMD_LENGTH_BYTES)
	{
		status = Basic_WriteBurstRegs(
			pAdap,
			buff + VENDOR_CMD_HEAD_BYTES,
			len - VENDOR_CMD_HEAD_BYTES,
			(UINT16)(REG_MAILBOX_DATA));		
	}

	if(STATUS_SUCCESS != status)
	{
		return status;
	}

	//then write command type into device
	status = Basic_WriteBurstRegs(
			pAdap,
			buff,
			VENDOR_CMD_HEAD_BYTES,
			(UINT16)REG_MAILBOX_CMD);	
	
	return	status;
}

TDSP_STATUS Vcmd_Transmit_DSPFw_Head(
  		PDSP_ADAPTER_T	 pAdap,
  		UINT16			 offset,
  		UINT32 		        len,
  		UINT8 			 file_id
	)
{
	//build fw head
	TDSP_STATUS   status;
	UINT8 SrcPortNo,DstPortNo;		/*Sourc/Destination Port */
	UINT32 secret_steering_bit;		/*Secret Bit*/
	UINT32 dma_src = 0;				/*DMA Source address*/
	UINT32 dma_dst = 0; 			/*DMA destination address*/
	UINT32 destoffset = 0;			/*Destination offset*/
	UINT32 dma_control_word = 0;	/*DMA control word*/

	SrcPortNo = SHUTTLEBUS__PORT_CARDBUS;
	DstPortNo = SHUTTLEBUS__PORT_UNIPHY;

	ASSERT(len <= 1024);

	switch(file_id){
        case SPX_P_MEM:
            secret_steering_bit = 0;
            //destoffset = (offset - DOWNLOAD_DSP_FW_FIELD_OFFSET)/4;   //should be consumed/4
            destoffset =(UINT32) (offset/4);   //should be consumed/4

			dma_control_word = REG_CSR_DMA0_CONTROL(	
									4,		
									0,		
									0,			
									0,							
									0,							
									1,	
									len,							
									0,		
									0,			
									0,		
									1,	
									0,			
									1	
									);

            dma_dst = REG_CSR_DMA0_DST_ADDR(
									DstPortNo, 
									secret_steering_bit | destoffset);

            break;

        case SPX_A_MEM:
            secret_steering_bit = 0;
            //destoffset = (offset - DOWNLOAD_DSP_FW_FIELD_OFFSET);
            destoffset = (offset);

			dma_control_word = REG_CSR_DMA0_CONTROL(
									4,		/* _priority */
									0,		/* _fifo */
									0,		/* _hiperf */
									0,						/* _shuttle2host */
									0,						/* _host2shuttle */
									1,	/* _burstsize */
									len,		            /* _numofbytes */
									0,		/* _readstop */
									0,		/* _writestop */
									0,		/* _readctl */
									1,	/* _mode */
									0,		/* _writelink */
									0		/* _writectl */
								);

                dma_dst = REG_CSR_DMA0_DST_ADDR(
									DstPortNo,
									secret_steering_bit | (destoffset)>>2);
            break;

        case SPX_B_MEM:
            secret_steering_bit = 0x08000000;
            //destoffset = (offset - DOWNLOAD_DSP_FW_FIELD_OFFSET);
            destoffset = (offset );

			dma_control_word = REG_CSR_DMA0_CONTROL(
									4,		/* _priority */
									0,		/* _fifo */
									0,		/* _hiperf */
									0,						/* _shuttle2host */
									0,						/* _host2shuttle */
									1,	/* _burstsize */
									len,		            /* _numofbytes */
									0,		/* _readstop */
									0,		/* _writestop */
									0,		/* _readctl */
									1,	/* _mode */
									0,		/* _writelink */
									0		/* _writectl */
								);

                dma_dst = REG_CSR_DMA0_DST_ADDR(
									DstPortNo,
									secret_steering_bit | (destoffset)>>2);
            break;

        case SPX_T_MEM:
            secret_steering_bit = 0x04000000;
            //destoffset = (offset - DOWNLOAD_DSP_FW_FIELD_OFFSET);
            destoffset = (offset);

			dma_control_word = REG_CSR_DMA0_CONTROL(
									4,		/* _priority */
									0,		/* _fifo */
									0,		/* _hiperf */
									0,						/* _shuttle2host */
									0,						/* _host2shuttle */
									1,	/* _burstsize */
									len,		            /* _numofbytes */
									0,		/* _readstop */
									0,		/* _writestop */
									0,		/* _readctl */
									1,	/* _mode */
									0,		/* _writelink */
									0		/* _writectl */
								);

                dma_dst = REG_CSR_DMA0_DST_ADDR(
									DstPortNo,
									secret_steering_bit | (destoffset)>>2);
            break;
			
		default:
			break;

	    }

	    dma_src = REG_CSR_DMA0_SRC_ADDR (
					SrcPortNo,
					PCI_STEERING_BIT | \
					((UINT32)DOWNLOAD_DSP_FW_FIELD_OFFSET )>>2 );	

        // here, we will write dma data to dsp derectly. 

//        VcmdW_3DSP_Dword(pAdap,offset,);
//        VcmdW_3DSP_Dword(pAdap,len,);
	status = VcmdW_3DSP_Dword(pAdap,0xffffffff,WLS_CSR__CLEAR_STATUS);
	if (STATUS_SUCCESS != status)
	{
	 	DBG_WLAN__HAL(LEVEL_ERR," [%s:] DMA set clear status fail\n",__FUNCTION__);
	    return status;
	}
	sc_sleep(1);

    status = VcmdW_3DSP_Dword(pAdap,dma_src,WLS_CSR__DMA0_SRC_ADDR);
    if (STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR," download run_in_halt_flag 3\n");
        return status;
    }
    status = VcmdW_3DSP_Dword(pAdap,dma_dst,WLS_CSR__DMA0_DST_ADDR);
    if (STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR," download run_in_halt_flag 4\n");
        return status;
    }
    status = VcmdW_3DSP_Dword(pAdap,dma_control_word,WLS_CSR__DMA0_CONTROL);
    if (STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR," download run_in_halt_flag 5\n");
        return status;
    }

	{
		UINT32 ulCounter = 0;
	// wait for dma finished
		while((!VcmdR_3DSP_Dword( pAdap,  WLS_CSR__STATUS)) & BITS_STATUS__DMA0_DONE)//BITS_STATUS__DMA0_BUSY)

		{
		sc_sleep(1);
		ulCounter ++;
        if((ulCounter%100) == 0)
		{

			DBG_WLAN__HAL(LEVEL_TRACE,"wait for dma finished ------ failed,src = 0x%x,dst=0x%x, ctl=0x%x \n",dma_src,dma_dst,dma_control_word);

		
			DBG_WLAN__HAL(LEVEL_TRACE,"wait for dma finished ------ failed,reg0 = 0x%x,counter=0x%x, reg0xc=0x%x \n",
				VcmdR_3DSP_Dword( pAdap,  0),ulCounter,VcmdR_3DSP_Dword( pAdap,  0xc));
		}

		if(ulCounter > 1000)
			{
				DBG_WLAN__HAL(LEVEL_ERR,"wait for dma finished ------ failed \n");
				return STATUS_FAILURE;
			}
		}
	}
//	fw_head.request_cmd = VCMD_API_REQ_DOWNLOAD_DSPCODE;
//	//fw_head->offset = offset;
//	//fw_head->fraglen = len;
//	fw_head.src_word = dma_src;
//	fw_head.dst_word = dma_dst;
//	fw_head.ctrl_word = dma_control_word;
//	status = Vcmd_Send_API(pAdap,(PUINT8)&fw_head,sizeof(DSP_FW_HEAD_T));
	return STATUS_SUCCESS;
}


//Justin: copy from test driver
TDSP_STATUS  Fw_UsbBulkStart(PDSP_ADAPTER_T pAdap,BOOLEAN bSingle, BOOLEAN bDma1Inc, BOOLEAN bDma2Inc,UINT16 endpoint_num)
{
	UINT8       g_yTemp;
	UINT32       g_yTemp_fact;
	TDSP_STATUS    status;
	UINT16 		offset = REG_USBINTR1;//0x05;

	//set int enable
	g_yTemp = 0xff;
	//write 0xff to 0x05
	status = Basic_WriteBurstRegs_single(pAdap,
						  (PUINT8)&g_yTemp,
						  sizeof(UINT8),
						  offset //,   //bulk out 2 ,dma1
//						  1
						  );
	if(status != STATUS_SUCCESS)
	{
		return STATUS_FAILURE;
	}

	//write 0xff to 0x06 
	status = Basic_WriteBurstRegs_single(pAdap,
						  (PUINT8)&g_yTemp,
						  sizeof(UINT8),
						  offset + 1//,   //bulk out 2 ,dma1
//						  1
						  );

	if(status != STATUS_SUCCESS)
	{
		return STATUS_FAILURE;
	}

    //////////////////////////start bulk //////////////////////////////	
    switch(endpoint_num)
    {
    case 1:
        offset = REG_BICR1;//0x20;
        break;
    case 2:
        offset = REG_BOCR2;//0x30;
        break;
    case 3:
        offset = REG_BICR3;//0x40;
        break;
    case 4:
        offset = REG_BOCR4;//0x50;
        break;
    default:
        break;
        return STATUS_FAILURE;
    }
    
	g_yTemp = (bSingle) ? BIT_XFR_EN_SING : BIT_XFR_EN_MULT;

	if (!bDma1Inc)
	{
        	g_yTemp |= BIT_XFR_DMA1_NOTINC;
	}
	
    	if (!bDma2Inc)
    	{
        	g_yTemp |= BIT_XFR_DMA2_NOTINC;
    	}
		
    	g_yTemp |= BIT_XFR_READY; //SetReady

	g_yTemp_fact = (UINT32)g_yTemp & 0xff;	
    
    status = Basic_WriteBurstRegs_single(pAdap,
						  (PUINT8)&g_yTemp_fact,
                          sizeof(UINT32),
                          offset//,  
                          //						  1
                          );
    
    if(status != STATUS_SUCCESS)
    {
        return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;	
}

TDSP_STATUS  Fw_UsbBulkSetDma(PDSP_ADAPTER_T pAdap,UINT16 addr,UINT8 addr_type,UINT16 len,
                              UINT16 endpoint_num,UINT16 dma_num)
{
    TDSP_STATUS  status;
    UINT16     offset = 0;
	UINT32       fact_addr;
    UINT32       fact_len;
    
    if(dma_num == 2)//dma 2
    {
        // new 1.94
        //double-word alignment
        //    offset = (offset + 3) & 0xfffc;    
        fact_addr = addr >> 2;
        //Set BOBR2 baseaddr, dst addr is program memory
        fact_addr = (fact_addr << OFFSET_DMA2_ADDR) & MASK_DMA2_ADDR;// dma2
        fact_addr |= addr_type;//BIT0;                     //  local 64kb program memory
        
        //align len     //double-word alignment
        //    Length = (len + 3) & 0xfffc;    
        fact_len = len >> 2;
        fact_len = (fact_len << 16) & 0x7fff0000;
        //    Length |= BIT0;     //0:Only DMA2 operation,1:DMA1 -> DMA2
    }
    else//dma 1 //not support only dma1 
    {
        return STATUS_FAILURE;
        // old spec...
        //	fact_addr = (UINT32)addr & 0xffff;   // dma1
        //	fact_len = (UINT32)len & 0x7fff;
        //
        //	fact_addr = (fact_addr << 16) & 0xffff0000; // dma2
        //	fact_len = (fact_len << 16) & 0x7fff0000;
        
        //	DBGSTR(("addr = %x, len =%x \n",fact_addr,fact_len));
    }
    
    
    switch(endpoint_num)
    {
    case 1:
        offset = REG_BIBR1;       //BULK OUT 0
        break;
    case 2:
        offset = REG_BOBR2;       //BULK OUT 0
        break;
    case 3:
        offset = REG_BIBR3;       //BULK OUT 0
        break;
    case 4:
        break;
        offset = REG_BOBR4;       //BULK OUT 1
    default:
        return STATUS_FAILURE;
        break;
    }
    
    // write dma addr
	status = Basic_WriteBurstRegs_single(pAdap,
						  (PUINT8)&fact_addr,
						  sizeof(UINT32),
						  offset//,   //bulk out 2 ,dma1   //dma2
//						  1
						  );

	if(status != STATUS_SUCCESS)
	{
		return STATUS_FAILURE;
	}

    // write dma length
	status = Basic_WriteBurstRegs_single(pAdap,
						  (PUINT8)&fact_len,
						  sizeof(UINT32),
						  offset + 4//,   //bulk out 2 ,dma1
//						  1
						  );

	if(status != STATUS_SUCCESS)
	{
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;	
}


TDSP_STATUS  Fw_BulkOutWriteFromHost(PDSP_ADAPTER_T pAdap,PUINT8 OutBuffer, UINT16 OutBufferSize,UINT16 bulkout_num)
{
	TDSP_STATUS status = STATUS_SUCCESS;
    UINT32 time_out;
	DBG_WLAN__HAL(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);

    status = Tx_Transmit_FW_Fragment(pAdap,OutBuffer,OutBufferSize);
    if (STATUS_SUCCESS == status)
	{
	    // TODO:Jakcie
		//time_out =  HZ;				
        time_out =  300;
        status = sc_event_wait(&pAdap->tx_fm_event, time_out);

		if(0 == status)
		{
			DBG_WLAN__HAL(LEVEL_ERR,"[%s] DownLoad8051FirmWare---wait tx fm event failed\n",__FUNCTION__);			
		}
        else
            status = STATUS_SUCCESS;
     
	}	  
    
	return status;	
}

UINT8 Fw_UsbClearStatus(PDSP_ADAPTER_T pAdap,UINT8 wIndex,UINT8 status)
{
	UINT8 tmp;
	
	Basic_ReadBurstRegs_single(pAdap,
						&tmp,
						  1,
						  REG_USBINTR1 + wIndex//0x05 + wIndex//,
//						  1
						  );

    tmp &= (~status);
	
	Basic_WriteBurstRegs_single(pAdap,
						&tmp,
						  1,
						  REG_USBINTR1 + wIndex//,
//						  1
						  );

	
	/*//clear done bit
	tmp1 = tmp;
	tmp1 &=0xfd;
	Basic_WriteBurstRegs(
						&tmp1,
						  1,
						  0x05+wIndex,
						  1
						  );*/
	
    return tmp;
}

UINT8 Fw_UsbGetStatus(PDSP_ADAPTER_T pAdap,UINT8 wIndex)
{
	UINT8 tmp;
/*
	//get realy status
	Basic_ReadBurstRegs(
						&tmp,
						  1,
						  0x30,
						  1
						  );

	
	//get done bit status
	//get 0x05
	Basic_ReadBurstRegs(
						&tmp,
						  1,
						  0x05,
						  1
						  );
	//get 0x06
	Basic_ReadBurstRegs(
						&tmp,
						  1,
						  0x06,
						  1
						  );

	//get 0x07
	Basic_ReadBurstRegs(
						&tmp,
						  1,
						  0x07,
						  1
						  );

	//get 0x08
	Basic_ReadBurstRegs(
						&tmp,
						  1,
						  0x08,
						  1
						  );

*/
	Basic_ReadBurstRegs_single(pAdap,
						&tmp,
						  1,
                          REG_USBINTR1 + wIndex//0x05 + wIndex//,
//						  1
						  );
	/*//clear done bit
	tmp1 = tmp;
	tmp1 &=0xfd;
	Basic_WriteBurstRegs(
						&tmp1,
						  1,
						  0x05+wIndex,
						  1
						  );*/
	
    return tmp;
}

TDSP_STATUS test_stall(PDSP_ADAPTER_T pAdap)
{
    UINT8 uTemp[100];
    UINT8 ubuff;

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    ubuff = 0x1;        // stall all
    Basic_WriteBurstRegs_single(pAdap,&ubuff,1,0x2);
    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);
   Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
   DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    ubuff = 0x0;        // stall none
    Basic_WriteBurstRegs_single(pAdap,&ubuff,1,0x2);
    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    ubuff = 0x2;        // stall once
    Basic_WriteBurstRegs_single(pAdap,&ubuff,1,0x2);
    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);
   Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x0);
   DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);
    
    return STATUS_SUCCESS;
}

void Fw_print_all_regs(PDSP_ADAPTER_T pAdap)
{
    UINT8 uTemp[100];

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x30);
    DBG_WLAN__HAL(LEVEL_TRACE, "base(0x30): %X:%X:%X:%X \n %X:%X:%X:%X \n %X:%X:%X:%X\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3],
        uTemp[4],uTemp[5],uTemp[6],uTemp[7],
        uTemp[8],uTemp[9],uTemp[10],uTemp[11]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0xc0);
     DBG_WLAN__HAL(LEVEL_TRACE, "base(0xc0): %X:%X:%X:%X:\n",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

    Basic_ReadBurstRegs_single(pAdap,uTemp,12,0x05);
     DBG_WLAN__HAL(LEVEL_TRACE, "base(0x05): %X:%X:%X:%X \n ",
        uTemp[0],uTemp[1],uTemp[2],uTemp[3]);

}


TDSP_STATUS  Fw_UsbBulkin(
                          PDSP_ADAPTER_T pAdap,
                          UINT16 addr,
                          UINT8 addr_type,
                          PUINT8 data,
                          UINT16 len,
                          UINT16 endpoint_num,
                          BOOLEAN single,
                          BOOLEAN dma1_inc,
                          BOOLEAN dma2_inc)
{
    INT32  time_out;
    TDSP_STATUS status = STATUS_SUCCESS;
    UINT32 loop = VENDOR_LOOP_MAX;
    
    //set dma
    //UsbE2BulkOutSetDma1(0xc100,OutBufferSize);
    if(single)
    {
        Fw_UsbBulkSetDma(pAdap,addr,addr_type,len,endpoint_num,2);
        //Fw_UsbBulkOutStart(TRUE, TRUE, TRUE,bulkout_num);
    }
    else
    {
        Fw_UsbBulkSetDma(pAdap,addr,addr_type,len,endpoint_num,2);				
    }
    
    //
    Fw_UsbBulkStart(pAdap,single, dma1_inc, dma2_inc,endpoint_num);
    //
    
    // for debug, print all related regs
    //    Fw_print_all_regs(pAdap);

    status =  Tx_Get_FW_Fragment(pAdap,data,len);

    if(STATUS_SUCCESS != status)
    {
        return status;
    }

    // TODO:Jacke
	//time_out =  HZ;
    time_out =  300;
    status = sc_event_wait(&pAdap->tx_fm_event, time_out);
    
    if(0 == status)
    {
        DBG_WLAN__HAL(LEVEL_ERR,"[%s] failed to wait tx fw event\n",__FUNCTION__);
         return status;
    }
    
    //clear done bit
    if(endpoint_num == 1)
    {
        while((Basic_ReadRegByte(pAdap,REG_BICR1) & BIT_XFR_BUSY)  && --loop)
        {
           ;
        }

		
        //while ((!(Fw_UsbGetStatus(3)& 0x02)) && --loop);       /* If done */
        Fw_UsbClearStatus(pAdap,3,BIT_BIDD1);//0x02);
    }
    else if(endpoint_num == 3)
    {
        while((Basic_ReadRegByte(pAdap,REG_BICR3) & BIT_XFR_BUSY)  && --loop)
            //        while ((!(Fw_UsbGetStatus(pAdap,3)& 0x08)) && --loop)
        {
            ;
        }
        Fw_UsbClearStatus(pAdap,3,BIT_BIDD3);//0x08);
    }
    else
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] ERROR HAPPEN WITH OUT OP \n",__FUNCTION__);
    }
    
    if(0 == loop)
    {
        status =  STATUS_FAILURE;
    }
    
    return status;	
}

//woody 
TDSP_STATUS  Fw_SendUsbBulkOut(
    PDSP_ADAPTER_T pAdap,
	UINT16 addr,
    UINT8 addr_type,
	PUINT8 data,
	UINT16 len,
	UINT16 bulkout_num,
	BOOLEAN single,
	BOOLEAN dma1_inc,
	BOOLEAN dma2_inc)
{
    TDSP_STATUS status = STATUS_SUCCESS;
    UINT32 loop = VENDOR_LOOP_MAX;

    DBG_WLAN__HAL(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);
    //set dma
    //UsbE2BulkOutSetDma1(0xc100,OutBufferSize);
    if(single)
    {
        Fw_UsbBulkSetDma(pAdap,addr,addr_type,len,2,2);
        //Fw_UsbBulkOutStart(TRUE, TRUE, TRUE,bulkout_num);
    }
    else
    {
        Fw_UsbBulkSetDma(pAdap,addr,addr_type,len,2,2);				
    }
    
    //
    Fw_UsbBulkStart(pAdap,single, dma1_inc, dma2_inc,2);
    //
    
    // for debug, print all related regs
//    Fw_print_all_regs(pAdap);

    status = Fw_BulkOutWriteFromHost(pAdap,(PUINT8)data,len,bulkout_num);
    if(STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] Fw_BulkOutWriteFromHost \n",__FUNCTION__);
     
        return status;
    }

    //clear done bit
    if(bulkout_num == 0)
    {
        while((Basic_ReadRegByte(pAdap,REG_BOCR2) & BIT_XFR_BUSY)  && --loop)
        {
    		sc_sleep(1);

    		if(pAdap->run_in_halt_flag == TRUE)
    			break;

        }
        //while ((!(Fw_UsbGetStatus(3)& 0x02)) && --loop);       /* If done */
        Fw_UsbClearStatus(pAdap,3,BIT_BODD2);//0x02);
    }
    else if(bulkout_num == 1)
    {
        while((Basic_ReadRegByte(pAdap,REG_BOCR4) & BIT_XFR_BUSY)  && --loop)
//        while ((!(Fw_UsbGetStatus(pAdap,3)& 0x08)) && --loop)
        {
    		sc_sleep(100);
    		
    		if(pAdap->run_in_halt_flag == TRUE)
        		break;
        }
        Fw_UsbClearStatus(pAdap,3,BIT_BODD4);//0x08);
    }
    else
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] ERROR HAPPEN WITH OUT OP \n",__FUNCTION__);
    }
    
    if(0 == loop)
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] clear done bit failed \n",__FUNCTION__);
        status =  STATUS_FAILURE;
    }

    
    return status;		
    
}




TDSP_STATUS Fw_download_8051fw_fragment(PDSP_ADAPTER_T pAdap,
								   PUINT8 buf,
								   UINT16 len,
								   UINT16 offset)
{
    TDSP_STATUS status;
    UINT16 Length;

    DBG_WLAN__HAL(LEVEL_INFO,"Enter [%s]\n",__FUNCTION__);
    DBG_WLAN__HAL(LEVEL_INFO," [%s]: download_8051fw Addr = %x, Len = %x\n",__FUNCTION__,offset, len);

    //double-word alignment
    offset = (offset + 3) & 0xfffc;    

    //align len     //double-word alignment
    Length = (len + 3) & 0xfffc;    

    /*
    Basic_WriteBurstRegs((PUINT8)&offset, sizeof(offset), 0x0034, 1);
    //set len, write to BOLR2:0X0038
    Basic_WritAR)&Length, sizeof(Length), 0x0038, 1);
    //enable dma1, write to BOCR2:0X0030
    tmp = (UINT8)BIT2 ;  //set xfr_ready, increase automatically
    Basic_WriteRegByte(tmp, 0x0030);
    //begin send data through bulkout2
    ndis_status = Fw_BulkOutWriteFromHost(databuf, Length,0);
    */
    status = Fw_SendUsbBulkOut(pAdap,(UINT16)offset,DMA_LOCAL_PROG_MEM,(PUINT8)buf,Length,0,TRUE,TRUE,TRUE);
    
    if(STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] ERROR status = %x\n",__FUNCTION__,status);
        return status;
    }
    
    return STATUS_SUCCESS;
}

TDSP_STATUS Fw_bulkin_dspfw_fragment(PDSP_ADAPTER_T pAdap,
                                     PUINT8 buf,
                                     UINT16 len,
                                     UINT16 offset)
{
    TDSP_STATUS status;
    UINT16 Length;
   DBG_WLAN__HAL(LEVEL_INFO," [%s]: download_dspfw Addr = %x, Len = %x\n",__FUNCTION__,offset, len);

    //double-word alignment
    offset = (offset + 3) & 0xfffc;    
    
    //align len     //double-word alignment
    Length = (len + 3) & 0xfffc;    
    
    status = Fw_UsbBulkin(pAdap,(UINT16)offset,DMA_APPL_SCRATCH_MEM,(PUINT8)buf,Length,
        1,TRUE,TRUE,TRUE);
    
    if(STATUS_SUCCESS != status)
    {
       DBG_WLAN__HAL(LEVEL_ERR, "[%s] ERROR status = %x\n",__FUNCTION__,status);
        return status;
    }
    
    return STATUS_SUCCESS;
}

TDSP_STATUS Fw_download_dspfw_fragment(PDSP_ADAPTER_T pAdap,
								   PUINT8 buf,
								   UINT16 len,
								   UINT16 offset)
{
    TDSP_STATUS status;
    UINT16 Length;
    
    //double-word alignment
    offset = (offset + 3) & 0xfffc;    

    //align len     //double-word alignment
    Length = (len + 3) & 0xfffc;    
    
    /*
    Basic_WriteBurstRegs((PUINT8)&offset, sizeof(offset), 0x0034, 1);
    //set len, write to BOLR2:0X0038
    Basic_WritAR)&Length, sizeof(Length), 0x0038, 1);
    //enable dma1, write to BOCR2:0X0030
    tmp = (UINT8)BIT2 ;  //set xfr_ready, increase automatically
    Basic_WriteRegByte(tmp, 0x0030);
    //begin send data through bulkout2
    ndis_status = Fw_BulkOutWriteFromHost(databuf, Length,0);
    */
    status = Fw_SendUsbBulkOut(pAdap,(UINT16)offset,DMA_APPL_SCRATCH_MEM,(PUINT8)buf,Length,0,TRUE,TRUE,TRUE);
    
    if(STATUS_SUCCESS != status)
    {
        DBG_WLAN__HAL(LEVEL_ERR, "[%s] ERROR status = %x\n",__FUNCTION__,status);
        return status;
    }
    
    return STATUS_SUCCESS;
}


/*******************************************************************
 *   Fw_Start_8051fw
 *   
 *   Descriptions:
 *      The routine starts 8051 fw code after 8051 fw has been downloaded into programed mem
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Fw_Start_8051fw(PDSP_ADAPTER_T pAdap)
{
    TDSP_STATUS status;
	UINT8 tmp;
	//start 8051

#if 0
	//Jakio20070514: for test
	#ifdef SEND_HDR_WITH_BULKOUT1
	status = Basic_WriteRegByte(pAdap, 0x83, REG_8051CR);
	#else
	status = Basic_WriteRegByte(pAdap, 0x81, REG_8051CR);
	#endif
	//status = Basic_WriteRegByte(pAdap,BIT_START8051_ASSO_BT,REG_8051CR);
	sc_sleep(10000);
//	status = Basic_WriteRegByte(pAdap,0x02,REG_8051CR+1);
#endif

#if 1
	tmp = Basic_ReadRegByte(pAdap,0x14);
	tmp &= 0xbf;
	status = Basic_WriteRegByte(pAdap,tmp,0x14);
#endif

    return status;

    //do it later , define it later
//	return (Basic_WriteRegByte(pAdap,VCMD_VAL_RESTART_FW,OFFSET_8051_RESET_FW_REG));
}

/*******************************************************************
 *   Fw_Stop_8051fw
 *   
 *   Descriptions:
 *      The routine stops 8051 fw code before 8051 fw has been downloaded into programed mem
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Fw_Stop_8051fw(PDSP_ADAPTER_T pAdap)
{
    TDSP_STATUS status;
	UINT8 tmp;

	// stop 8051 firmware
#if 0	
	status = Basic_WriteRegByte(pAdap,BIT_START8051_DEFAULT,REG_8051CR);
//	status = Basic_WriteRegByte(pAdap,0x00,REG_8051CR+1);
	sc_sleep(1000);
#endif

#if 1
	tmp = Basic_ReadRegByte(pAdap,0x14);
    	tmp |= BIT6;
	status = Basic_WriteRegByte(pAdap,tmp,0x14);
#endif

    return status;
	//do it later , define it later
//	return (Basic_WriteRegByte(pAdap,VCMD_VAL_RESTART_FW,OFFSET_8051_RESET_FW_REG));
}
// glen20090108 Add for 8051 Version update check
#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE	
UINT32 Get8051VersionFromCode(VOID)
{

	PSP20_CODE pCodeBuffer = (PSP20_CODE)Usb_8051CodeWlanOnly;
	UINT32 ulVersion = 0;
	{
		//here get fw file from a .h file
		DBG_WLAN__HAL(LEVEL_TRACE,"Read 8051 Version from sp20code.h \n");
		DBG_WLAN__HAL(LEVEL_TRACE,"8051 FW Version: %02d.%02d.%02d.%02d \n", 
			pCodeBuffer->Code[0x2C], pCodeBuffer->Code[0x2D],
			pCodeBuffer->Code[0x2E], pCodeBuffer->Code[0x2F]);
		ulVersion = *(UINT32 *)&pCodeBuffer->Code[0x2C];

	}
	return (ulVersion);
}
#endif

BOOLEAN Vcmd_8051fw_isWork(PDSP_ADAPTER_T pAdap)
{
	UINT8 		tmp;
	UINT32       tmpVersion=0;
#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE	
	UINT32       tmpVersionFromcode;
#endif
    ULONG       loop = 0;
	ULONG       maxloop = 100;

	tmp = Basic_ReadRegByte(pAdap,REG_UIR);
    DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:reg 0x14 = %x\n",__FUNCTION__,tmp);

	if((tmp & BIT6) != 0x0)
	{
		DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Return FALSE 1 \n",__FUNCTION__);
		return FALSE;
	}	

	do{

        sc_sleep(500);
		loop++;

    	tmp = Basic_ReadRegByte(pAdap,REG_8051_RUN_FLAG);
    	if(tmp !=  '3')		
        {
    		DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Return FALSE 2,%x \n",__FUNCTION__,tmp);
    		continue;
    	}	

    	tmp = Basic_ReadRegByte(pAdap,REG_8051_RUN_FLAG+1);
    	if(tmp !=  'D')
        {
        	DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Return FALSE3,%x \n",__FUNCTION__,tmp);
        	continue;
        }	
    	
    	tmp = Basic_ReadRegByte(pAdap,REG_8051_RUN_FLAG+2);
    	if(tmp !=  'S')
    	{
    		DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Return FALSE 4,%x \n",__FUNCTION__,tmp);
    		continue;
    	}	
    	tmp = Basic_ReadRegByte(pAdap,REG_8051_RUN_FLAG+3);
    	if(tmp !=  'P')
        {
    		DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Return FALSE 2,%x \n",__FUNCTION__,tmp);
    		continue;
    	}	
        	loop = 0;
    	break;
	}while(loop < maxloop);

	if(loop >= maxloop)
	{
		DBG_WLAN__HAL(LEVEL_TRACE,"Return FALSE Vcmd_8051fw_isWork\n");
		return FALSE;
	}

	Basic_ReadBurstRegs(pAdap, (UINT8*)&tmpVersion, 4, REG_VERSION_8051);

	DBG_WLAN__HAL(LEVEL_TRACE,"Get 8051 Version: %08X\n", tmpVersion);	
 #ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE	
	tmpVersionFromcode = Get8051VersionFromCode();
	if(tmpVersionFromcode != tmpVersion)
	{
		DBG_WLAN__HAL(LEVEL_TRACE,"8051 code version need update\n");	 
		return FALSE;		
	}
	else
	{
		DBG_WLAN__HAL(LEVEL_TRACE,"8051 code version is latest\n");	    
		return TRUE;		
	}
  #else 
       return TRUE;
  #endif 
    
	//tmp = Basic_ReadRegByte(pAdap,0xc2);
	//return ((tmp & BIT1) == BIT1);
}


/*******************************************************************
 *   Vcmd_Set_Firmware_Download_Ok
 *   
 *   Descriptions:
 *      The routine of Firmware_Download_Ok contorl command.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Vcmd_Set_Firmware_Download_Ok(PDSP_ADAPTER_T pAdap, BOOLEAN		bMainLoopTrue)
{
	UINT16 	loop = VENDOR_LOOP_MAX;
	TDSP_STATUS status;
	DSP_FW_DOWN_OK_T   val;
	UINT32  errloop = 10;
	BOOLEAN       exit_flag;

	pAdap->hw_8051_work_mode = INT_8051_IN_MINILOOP_MODE;

{
	UINT16  i;
	//woody debug
	//first clear head date before enter main loop
	for(i=0;i<8;i++)
	{
		Basic_WriteRegByte(pAdap,0x00, REG_MAILBOX_HEAD + i);
	}
}	

	
	val.request_cmd = VCMD_API_RELEASE_CODE_DSPCODE;
	//val.autorate = pAdap->wlan_attr.fallback_rate_to_use;
	//always set auto rate for 8051 in spite of both auto mode and fixed rate mode
	val.autorate = FALLBACK_RATE_USE;

	if(val.autorate == FALLBACK_RATE_USE)
	{
		val.autorate = FALLBACK_RATE_PACKET_COUNTS;
	}
	
	val.reserve = 0x0;
	
	if(bMainLoopTrue)
	{
		val.mainloop_type = 0x00;
		DBG_WLAN__HAL(LEVEL_TRACE, "****SWITCHING:*******8051 go main loop  ************  \n");
	}
	else
	{
		val.mainloop_type = 0x5A;
		DBG_WLAN__HAL(LEVEL_TRACE, "****SWITCHING:*******8051 go main loop, It's Running state  ************  \n");
	}

	if(pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
	{
		val.fw_type = 1;
	}
	else
	{
		val.fw_type = 0;
	}
	exit_flag = FALSE;
	// send cmd into 8051
	while(errloop != 0)
	{
		status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_FW_DOWN_OK_T));
		if(STATUS_ADAPTER_NOT_READY != status)
		{
			break;
		}
		else if(Adap_Driver_isHalt(pAdap))
		{
			exit_flag = TRUE;
			break;
		}
		else
		{
		errloop--;
		DBG_WLAN__HAL(LEVEL_ERR, " Vcmd_Set_Firmware_Download_Ok() err, c = %x \n",errloop);
		sc_sleep(2);   // 2ms
		}	
	}

	//
	if((status != STATUS_SUCCESS) ||(exit_flag == TRUE))
	{
		DBG_WLAN__HAL(LEVEL_ERR, "Vcmd_Set_Firmware_Download_Ok() ERROR EXIT 1\n");
		return status;
	}

	exit_flag = FALSE;
	while((pAdap->hw_8051_work_mode != INT_8051_IN_WORKING_MODE) && (--loop))
	{
		if(loop == (VENDOR_LOOP_MAX/2))
		{
			errloop = 10;
			// send cmd into 8051
			while(errloop != 0)
			{
				status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_FW_DOWN_OK_T));
				if(STATUS_ADAPTER_NOT_READY != status)
				{
					break;
				}
				else if(Adap_Driver_isHalt(pAdap))
				{
					break;
				}
				else
                {
				    errloop--;
				    DBG_WLAN__HAL(LEVEL_ERR, "Vcmd_Set_Firmware_Download_Ok() err, c = %x \n",errloop);
				    sc_sleep(2);   // 2ms
				}	
			}
		}
		sc_sleep(2);  // 2 ms
	    if(Adap_Driver_isHalt(pAdap))
		{
			DBG_WLAN__HAL(LEVEL_ERR, "Vcmd_Set_Firmware_Download_Ok() error exit due to halt\n");	
			exit_flag = TRUE;
			break;
		}

	}	
	
	if((loop == 0) || (exit_flag == TRUE))
	{
		DBG_WLAN__HAL(LEVEL_ERR, "[%s] Vcmd_Set_Firmware_Download_Ok() error exit 1\n",__FUNCTION__);
		status = STATUS_FAILURE;
	}
	return status;
}

/*******************************************************************
 *Vcmd_Set_8051_MinLoop
 *   
 *Descriptions:
 *      The routine of VCMD_API_8051_JUMP_MIN_PROCESS contorl command.
 *Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Vcmd_Set_8051_MinLoop(PDSP_ADAPTER_T pAdap, UINT8 isIndicateBT)
{
	UINT16 	loop = VENDOR_LOOP_MAX;
	TDSP_STATUS status;
	DSP_RESET_REQUEST_T   val;
	UINT32   errloop = 10;
	BOOLEAN		bShouldWait = FALSE;

	pAdap->hw_8051_work_mode = INT_8051_IN_WORKING_MODE;
	
	val.request_cmd = VCMD_API_8051_JUMP_MIN_PROCESS;
	switch(isIndicateBT)
	{
	case 0:
		{
			val.type = 1;
			break;
		}
	case 1:
		{
			val.type = 0;
			break;
		}
	case 2:
		{
			val.type = 2;
			break;
		}
	default:
		{//impossible case.
			val.type = 3;
			break;
		}
	}

    sc_event_reset(&pAdap->is_8051_ready_event);
	//send command into 8051
	while(errloop)
	{
		status = (Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_RESET_REQUEST_T)));
		if(STATUS_SUCCESS == status)
		{
			bShouldWait = TRUE;
			break;
		}
		else if(Adap_Driver_isHalt(pAdap))
		{
			break;
		}
		else
		{
	        DBG_WLAN__HAL(LEVEL_ERR,"[%s]: MINI LOOP command ERROR c= %x \n",__FUNCTION__,errloop);
		    errloop--;
		    sc_sleep(2); // 2ms
		}
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s]:MINI LOOP command ERROR EXIT\n",__FUNCTION__);
		return status;
	}
    if(bShouldWait)
	{
		if(1 == sc_event_wait(&pAdap->is_8051_ready_event, 2 * 1000))//wait for 2 s.
		{
			DBG_WLAN__HAL(LEVEL_TRACE,"* * * * * Vcmd_Set_8051_MinLoop, wait miniloop event ok \n");
		}
		else
		{
			DBG_WLAN__HAL(LEVEL_TRACE,"* * * * * Vcmd_Set_8051_MinLoop, wait miniloop event fail  \n");
		}
	}
/*
	//wait int from 8051
	while((pAdap->hw_8051_work_mode != INT_8051_IN_MINILOOP_MODE) &&(--loop))
	{
		if(loop == (VENDOR_LOOP_MAX/2))
		{
			status = Vcmd_Send_API(pAdap,(UINT8*)&val,sizeof(DSP_RESET_REQUEST_T));
			if(STATUS_SUCCESS == status)
			{
				break;
			}
			else if(Adap_Driver_isHalt(pAdap))
			{
				break;
			}
			else//if(NDIS_STATUS_ADAPTER_NOT_READY != status)
			{
				DBG_WLAN__HAL(LEVEL_ERR, "[%s] MINI LOOP command ERROR c= %x \n",__FUNCTION__,loop);
			}
		}
		sc_sleep(2);   // 2ms
	}
			
	if(loop == 0)
	{
	    DBG_WLAN__HAL(LEVEL_ERR, "[%s] don't received a mini loop int\n",__FUNCTION__);
		status = STATUS_FAILURE;
	}
*/
    DBG_WLAN__HAL(LEVEL_TRACE, "* * * * * %s exit loop = %d %d\n", __FUNCTION__, loop, pAdap->hw_8051_work_mode);
	return status;
}


TDSP_STATUS Vcmd_Funciton_Req_JOIN(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	DSP_RESET_REQUEST_T   val;
	UINT32                  errorloop = 20;

	val.request_cmd = VCMD_API_FUNCTION_REQ_JOIN;
	if (pAdap->dsp_fw_mode ==  INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
    {
		val.type = 1;			//add by hank 20090713
        //Added for tx hang case.
		if(pAdap->txHangFlag == TXHANG_RESETHW)
		{//This should be placed here . Joe 2009 - 12 - 28
			DBG_WLAN__HAL(LEVEL_TRACE,"* * * * * Vcmd_Funciton_Req_JOIN ,real tx hang happen %d  \n", pAdap->txHangFlag);
			val.type = 2;
		}		

    }
    else
    {
		val.type = 0;
    }

    DBG_WLAN__HAL(LEVEL_TRACE, "* * * * * Vcmd_Funciton_Req_JOIN cmd %d:%d  %d\n", 
		val.request_cmd, val.type, pAdap->dsp_fw_mode);

	while(errorloop)
	{
		status = (Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_RESET_REQUEST_T)));
		if(status != STATUS_ADAPTER_NOT_READY)
		{
			break;
		}
		DBG_WLAN__HAL(LEVEL_ERR, "[%s] REQ join command fail C=%x\n",__FUNCTION__,errorloop);
		errorloop--;
		sc_sleep(2);     // 2ms
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR, "[%s] REQ join command fail EXIT\n",__FUNCTION__);
	}

    //Clear tx hang flag every time.
	pAdap->txHangFlag = TXHANG_IGNORE;

	return status;
}
// Add by glen 20081009 for support Bt DSP parameter init

TDSP_STATUS Vcmd_Funciton_Set_BTPara_Ready(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	DSP_RESET_REQUEST_T   val;
	UINT32                errorloop = 20;

	val.request_cmd = MAILBOX_CMD_BT_PARAMETER_READY;
	DBG_WLAN__HAL(LEVEL_TRACE,"* * * * * Vcmd_Funciton_Set_BTPara_Ready\n");

	while(errorloop)
	{
		status = (Vcmd_Send_API(pAdap,(UINT8*)&val,sizeof(DSP_RESET_REQUEST_T)));
		if(status != STATUS_ADAPTER_NOT_READY)
		{
			break;
		}
		DBG_WLAN__HAL(LEVEL_TRACE,"[%s]:Set_BTPara_Ready command fail C=%x\n",__FUNCTION__,errorloop);
		errorloop--;
		sc_sleep(2);     // 2ms
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"Set_BTPara_Ready command fail EXIT\n");
	}
	return status;
}
// Add by glen 20081009 for support Bt DSP parameter init end
/*********************************************************************
 *Vcmd_Set_Encryption_Mode
 *
 *Description:
 *	The routine inform the encryption mode to 8051
 *Arguments:
 *	pAdap: IN, the pointer of adapter context.
 *	mode: encryption mode
 *		   0x00--- 
 *		   0x01--- 
 *		   0x10----
 *		   0x11
 *Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
*********************************************************************/
TDSP_STATUS Vcmd_Set_Encryption_Mode(PDSP_ADAPTER_T pAdap)
{
	DSP_ENCRYPTION_MODE   val;
	TDSP_STATUS   status;
	UINT32  loop;

	if(pAdap->wlan_attr.gdevice_info.privacy_option == FALSE)
	{
		val.wep_mode= 1;
		val.group_mode = 1;
	}
		
	else
	{	
		switch(pAdap->wlan_attr.wep_mode)
		{
			case WEP_MODE_WEP:
				val.wep_mode = 2;
				val.group_mode = 2;
				break;
			case WEP_MODE_TKIP:
				val.wep_mode = 3;
				val.group_mode = 3;
				break;
			case WEP_MODE_AES:
				val.wep_mode = 4;
				if(pAdap->wlan_attr.group_cipher == WEP_MODE_TKIP)
					val.group_mode = 3;
				else if(pAdap->wlan_attr.group_cipher == WEP_MODE_WEP)
					val.group_mode = 2;
				else
					val.group_mode = 4;
				break;
			default:
				ASSERT(0);
		}
	}	
	val.request_cmd = VCMD_API_SET_ENCRYTION_MODE;
	loop = 10;

	while(loop)
	{
		status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_ENCRYPTION_MODE));
		if(status != STATUS_ADAPTER_NOT_READY)
		{
			break;
		}
		loop--;
		DBG_WLAN__HAL(LEVEL_ERR, "[%s]: Vcmd_Set_Encryption_Mode() ERROR, c= %x \n",__FUNCTION__,loop);
		sc_sleep(1);   // 1ms
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR, "[%s]: Vcmd_Set_Encryption_Mode() ERROR EXIT\n",__FUNCTION__);
	}
	
	return status;
}


TDSP_STATUS Vcmd_Reset_Bulkout_Request(PDSP_ADAPTER_T pAdap)
{
	DSP_RESET_BULKOUT_REQUEST_T val;
	val.request_cmd = VCMD_API_RESET_BULKOUT_REQUEST;
	return (Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_ENCRYPTION_MODE)));
}




/*
	Soft reset and leave NIC in a sleep mode.  Leave UniPHY core in reset.
*/
TDSP_STATUS Vcmd_CardBusNICReset(PDSP_ADAPTER_T pAdap)
{
	ULONG val;
	TDSP_STATUS status;
	ULONG loop;

	UINT32 state = VcmdR_3DSP_Dword(pAdap,WLS_CSR__PCI_CONTROL_REGISTER);
#if 0	
#ifndef DSP_ASIC_DEBUG_FLAG	

	state |= PCI_SOFT_RESET_PCI_BIT;

	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		DBG_WLAN__HAL(LEVEL_ERR,"@@@@@@@@#Vcmd_CardBusNICReset 1 \n");
		return STATUS_FAILURE;
	}

	
#else
#endif

	//close here for enable/disable. otherwise access mac reg will fail in re-start procedure.
#if 0
	state = VcmdR_3DSP_Dword(pAdap,WLS_MAC__CONTROL);
	state |= PCI_RESET_WLAN_CORE_BIT;

	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL))
	{
		DBGSTR(("@@@@@@@@#Vcmd_CardBusNICReset 1 \n"));
		return STATUS_FAILURE;
	}

#endif	


#endif
	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_RESET_WLAN_SUBSYSTEM_BIT |
			PCI_RESET_WLAN_SYSTEM_BIT;

	#if 0  //Jakio20070525: closed  here according newest v4 datasheet, no need to set bit17~bit31  
	//Jakio20070521: add for v4
	if(pAdap->wlan_attr.chipID)
	{
		state |= MHDMA_RX_CNTRL_STOP_BIT;
		state |= MHDMA_TX_CNTRL_STOP_BIT;
	}
	#endif
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		DBG_WLAN__HAL(LEVEL_ERR,"@@@@@@@@#Vcmd_CardBusNICReset 2 \n");
		return STATUS_FAILURE;
	}

	state = 	PCI_RESET_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_CORE_BIT |
			PCI_SLEEP_WLAN_SUBSYSTEM_BIT |
			PCI_SLEEP_WLAN_SYSTEM_BIT |
			PCI_SLEEP_MAC_GATED_BIT |
			PCI_SLEEP_MAC_BIT |
			PCI_SLEEP_DEBUG_BIT;

	val = 0;
	status = VcmdW_3DSP_Dword(
		           pAdap, val,
		           (UINT16)(WLS_SCRATCH__SP20_READY));
	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"@@@@@@@@#Vcmd_CardBusNICReset clear ready reg fail\n");
	}
	loop = 0;
	do{
		sc_sleep(20);
		val = VcmdR_3DSP_Dword(pAdap,
		           (UINT16)(WLS_SCRATCH__SP20_READY));
		if(val == 0)
		{
			break;
		}
		loop++;
	}while(loop < 10);

	if(loop >= 10)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"@@@@@@@@#Vcmd_CardBusNICReset clear ready reg fail,loop\n");
	}
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	sc_sleep(100 );
	return status;

}

//Jakio20070521: changed this routine
//enable cardbus nic, main action is to make dma0 enable for download code.
// Enable NIC clocks and DMAs. Leave UniPHY core in reset
TDSP_STATUS Vcmd_CardBusNICEnable(PDSP_ADAPTER_T pAdap)
{
	UINT32 state;
	/*
	UINT32 state =
			PCI_RESET_WLAN_CORE_BIT |
	//	  	MAC_HOST_DMA0_ENABLE_BIT |
	//	  	MAC_HOST_DMA1_ENABLE_BIT |
	  		DMA0_ENABLE_BIT |
	  		DMA1_ENABLE_BIT;
	*/
	//v2
	if(!pAdap->wlan_attr.chipID)
	{
		state  = PCI_SLEEP_DEBUG_BIT |
			 PCI_RESET_WLAN_CORE_BIT |
			 MAC_HOST_DMA0_ENABLE_BIT |
		  	 MAC_HOST_DMA1_ENABLE_BIT |
	  		 DMA0_ENABLE_BIT |
	  		 DMA1_ENABLE_BIT;
	}
	else //v4
	{
		state  = PCI_RESET_WLAN_CORE_BIT |
			 PCI_SLEEP_DEBUG_BIT |
	  		 DMA0_ENABLE_BIT;
	  		 ; 	//DMA1_ENABLE_BIT;  //Jakio20070525: closed  here according newest v4 datasheet, no need to set bit17~bit31
	}

	
	return (VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER));
}

// Enable board interrupts
TDSP_STATUS
Vcmd_NIC_INTERRUPT_ENABLE(PDSP_ADAPTER_T pAdap)
{
	UINT32 state;
	TDSP_STATUS status;
	
	state = (UINT32)WLS_INT_MASK;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__CLEAR_STATUS);
	/* WriteToReg(pAdapter,WLS_CSR_CLEARSTATUS_WD,WLS_INT_MASK); */

	state = (UINT32)BIT15;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__FUNCTION_EVENT);
	/* WriteToReg(pAdapter,WLS_CSR_FUNCTION_EVENT_CLR_WD,0x8000); */

	state = (UINT32)BIT15;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__FUNCTION_EVENT_MASK);
	/* WriteToReg(pAdapter,WLS_CSR_FUNCTION_EVENT_MASK_WD,0x8000); */

	state = (UINT32)(WLS_INT_MASK);
	//add for v4chip debug
	state |= MAILBOX_IEN;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__INTERRUPT_ENABLE);	
	/* WriteToReg(pAdapter,WLS_CSR_IE_WD,WLS_INT_MASK); */
	return status;
}


TDSP_STATUS
Vcmd_NIC_ENABLE_RETRY(PDSP_ADAPTER_T pAdap,UINT32 value)
{	
	return VcmdW_3DSP_Dword(pAdap,value,WLS_CSR__NUMBER_OF_RETRY);	
}
/*
 	host send the api to 8051 to notify it current state into reset state.
 	at reset state, 8051 don't transact hw event and transfer info to host 	
*/
TDSP_STATUS
Vcmd_reset_3dsp_request(PDSP_ADAPTER_T pAdap)
{
	DSP_RESET_REQUEST_T   val;
	val.request_cmd = VCMD_API_RESET_REQUEST;
	return (Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_RESET_REQUEST_T)));
}


TDSP_STATUS
Vcmd_NIC_INTERRUPT_DISABLE(PDSP_ADAPTER_T pAdap)
{
	UINT32 state;
	TDSP_STATUS status;
	
	state = 0;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__FUNCTION_EVENT_MASK);
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__INTERRUPT_ENABLE);	
	return status;
}

TDSP_STATUS
Vcmd_WLS_INT_ENABLE(PDSP_ADAPTER_T pAdap)
{
	UINT32 state;
	TDSP_STATUS status;
	
	state = (UINT32)WLS_INT_MASK;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__INTERRUPT_ENABLE);
	//
	state = (UINT32)BIT15;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__FUNCTION_EVENT_MASK);	
	return status;
 }


TDSP_STATUS
Vcmd_WLS_INT_DISABLE(PDSP_ADAPTER_T pAdap)
{
	UINT32 state;
	TDSP_STATUS status;
	
	state = 0;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__INTERRUPT_ENABLE);
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__FUNCTION_EVENT_MASK);	
	return status;
 }


TDSP_STATUS Vcmd_release_core(PDSP_ADAPTER_T pAdap)
{
	UINT32 val;

	val = VcmdR_3DSP_Dword(pAdap,WLS_CSR__PCI_CONTROL_REGISTER);
	val = val & (~BITS_PCI_CTRL_REG__RESET_WLAN_CORE);
	return VcmdW_3DSP_Dword(pAdap,val, WLS_CSR__PCI_CONTROL_REGISTER);
}

// Enable NIC clocks and DMAs. Remove UniPHY core reset.
// Wait for DSP initialization done
TDSP_STATUS Vcmd_CardBusNICStart(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;	
	UINT32 	val = 0;
	UINT32 	loop = VENDOR_LOOP_MAX; //Jakio20070517: change the value from 500000 to 10000
	UINT32	state;


	//status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);

	//Jakio20070521: add code according to pci_wlan driver
	state =	PCI_SLEEP_DEBUG_BIT |
			DMA0_ENABLE_BIT;
			//Jakio20070525: closed  here according newest v4 datasheet, no need to set bit17~bit31
			//DMA1_ENABLE_BIT 
	  		//MAC_HOST_DMA0_ENABLE_BIT |
	  		//MAC_HOST_DMA1_ENABLE_BIT;


	#if 0  //Jakio20070525: closed  here according newest v4 datasheet, no need to set bit17~bit31
	if(pAdap->wlan_attr.chipID)
	{
		state |= MHDMA_RX_CNTRL_ENABLE_BIT;
		state |= MHDMA_TX_CNTRL_ENABLE_BIT;
		state |= MHDMA_TX_CNTRL_BD_IN_HOST_BIT;
	}
	#endif
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	val = 0;
	while(--loop && (MMAC_CORE_RDY != VcmdR_3DSP_Dword(pAdap, WLS_SCRATCH__SP20_READY))) {
		sc_sleep(1); /* wait */
	}

	if(loop == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s] start card bus nic fail \n",__FUNCTION__);
		pAdap->DSP_FW_version = 0;
		return STATUS_FAILURE;
	}
#if 0
	//Jakio20070516: add for test
	DBGSTR(("Print Mac register after sp20 ready\n"));
	Adap_Print_MacReg(pAdap);
#endif
	//Jakio20070517: closed according to pci_wlan
	//VcmdW_3DSP_Dword(pAdap,BITS_BBREG_36_38__BB_INT, WLS_MAC__BBREG_36_38);

	pAdap->DSP_FW_version = VcmdR_3DSP_Dword(pAdap, WLS_SCRATCH__BASEBAND_VER);
	DBG_WLAN__HAL(LEVEL_TRACE, "[%s] start card bus nic ok \n",__FUNCTION__);
	return STATUS_SUCCESS;
}


TDSP_STATUS Vcmd_NIC_RESET_ALL_BUT_HARDPCI(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT32 state;
	state = PCI_RESET_WLAN_CORE_BIT |\
             PCI_RESET_WLAN_SUBSYSTEM_BIT | PCI_RESET_WLAN_SYSTEM_BIT | PCI_SOFT_RESET_PCI_BIT |\
             PCI_SLEEP_MAC_BIT; 
	state = state | VcmdR_3DSP_Dword(pAdap, WLS_CSR__PCI_CONTROL_REGISTER);

	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);

	return status;	
}	
	
TDSP_STATUS Vcmd_Reset_System(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT32 state;
	state = PCI_RESET_WLAN_CORE_BIT |PCI_SLEEP_MAC_BIT; 
	state = state | VcmdR_3DSP_Dword(pAdap, WLS_CSR__PCI_CONTROL_REGISTER);
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	return status;	
}

TDSP_STATUS Vcmd_Reset_Core(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT32 state;
	state = PCI_RESET_WLAN_CORE_BIT;
	state = state | VcmdR_3DSP_Dword(pAdap, WLS_CSR__PCI_CONTROL_REGISTER);
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	return status;	
}

/*	description				: It puts sp-20 in reset.
	for v4 chip, maybe is unuseful. please refer to mmacV4Init() of pci driver.
	Vcmd_mmacSp20ResetRemoved should be called later if the function called
	
*/

TDSP_STATUS 
Vcmd_mmacSp20ResetApplied(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT32 state;

	/*
	Do not change the following order.
	*/
	state = VcmdR_3DSP_Dword(pAdap,WLS_CSR__PCI_CONTROL_REGISTER);

	state |= (UINT32)PCI_SLEEP_WLAN_CORE_BIT;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);

	state |= (UINT32)PCI_SLEEP_WLAN_SUBSYSTEM_BIT;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);

	state |= (UINT32)PCI_RESET_WLAN_CORE_BIT;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);


	/* sc_sleep(100); */
	DBG_WLAN__HAL(LEVEL_TRACE, "[%s] Resetting SP20 in mmacSp20ResetApplied.\n",__FUNCTION__);
	return status;
}



/*
Name					: mmacSp20ResetRemoved()
return value			: void
global variables 		: void
description			: It takes sp-20 out of reset.
					the functoin should be called if Vcmd_mmacSp20ResetApplied has called in advanced.
special considerations 	: 
see also				: 
TODO					: none
*/
TDSP_STATUS 
Vcmd_mmacSp20ResetRemoved(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT32 state;

/*
	Do not change the following order.
*/

	state = VcmdR_3DSP_Dword(pAdap,WLS_CSR__PCI_CONTROL_REGISTER);

	//Jakio20070521: close code here according Sheng
	#if 0
	state &= (~(PCI_SLEEP_WLAN_SUBSYSTEM_BIT)); /* supply clock */       
	status = VcmdW_3DSP_Dword(pAdap,state, WLS_CSR__PCI_CONTROL_REGISTER);

	state &= (~(PCI_SLEEP_WLAN_CORE_BIT)); /* supply clock */       
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	#endif

	state &= (~(PCI_RESET_WLAN_CORE_BIT)); /* relese reset */        
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);


	 sc_sleep(1); 

	#if 0   //Jakio20070525: closed  here according newest v4 datasheet, no need to set bit17~bit31
	if(pAdap->wlan_attr.chipID)
	{
		state &= (~(MHDMA_RX_CNTRL_STOP_BIT));
		state &= (~(MHDMA_RX_CNTRL_ABORT_BIT));
		state &= (~(MHDMA_TX_CNTRL_STOP_BIT));
		state &= (~(MHDMA_TX_CNTRL_ABORT_BIT));

		status = VcmdW_3DSP_Dword(pAdap,state,WLS_CSR__PCI_CONTROL_REGISTER);
	}
	#endif
	DBG_WLAN__HAL(LEVEL_TRACE, "[%s] Resetting SP20 in Vcmd_mmacSp20ResetRemoved\n",__FUNCTION__);
	return status;
}


/*
	it is suggested to call this routine with state is idle
	here, some steps added for v4chip
*/
TDSP_STATUS 
Vcmd_flush_rx_fifo(PDSP_ADAPTER_T pAdap)
 {
	TDSP_STATUS status;
	UINT32 state;
	UINT32 loopcnt = 0;
	DSP_FLUSH_RXFIFO_T   val;
	
	val.request_cmd = VCMD_REQ_FLUSH_RX_FIFO;
	
	//return (Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_FLUSH_RXFIFO_T)));
#if 1	

	 Adap_set_state_control(pAdap, DEV_IDLE,0,0);
	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	state |=MAC_RXFIFO_FLUSH_BIT;
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);

	loopcnt = 1000;
	while(VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL) & MAC_RXFIFO_FLUSH_BIT)
	{
		if(loopcnt == 0)
			break;
	}

	if(loopcnt == 0)
	{
		DBG_WLAN__HAL(LEVEL_ERR,"[%s] fail in setting DSP into IDLE\n",__FUNCTION__);
	}
#endif
	return STATUS_SUCCESS;
 }


/*
	flush tx fifo
	add someting for v4chip
*/
TDSP_STATUS 
Vcmd_flush_tx_fifo(PDSP_ADAPTER_T pAdap)
 {
	TDSP_STATUS status;
	UINT32 state;
	UINT32 loop = VENDOR_LOOP_MAX;

	DBG_WLAN__HAL(LEVEL_TRACE, "[%s] Flush TX fifo \n",__FUNCTION__);
	
	Adap_set_state_control(pAdap, DEV_IDLE,0,0);
	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	state |=MAC_TXFIFO_FLUSH_BIT;
	 status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);

	//check if flust tx flow has been finished
	 while(loop--)
	 {
	 	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
		if(state & MAC_TXFIFO_FLUSH_BIT)
		{
			//continue;
		}
		else
		{
			break;
		}
	 	
	 }
	  Adap_set_state_control(pAdap, DEV_ACTIVE,0,0);

	//judge if finished
	 if(loop == 0)
	 {
	 	DBG_WLAN__HAL(LEVEL_ERR, "[%s] Flush TX fifo error\n",__FUNCTION__);
	 	return STATUS_FAILURE;
	 }


 	return STATUS_SUCCESS;
 }

TDSP_STATUS 
Vcmd_scratch_2_DSP_Amemoffset(PDSP_ADAPTER_T pAdap,VOID *Buffer, UINT32 len, UINT32 destoffset) 
{
	//build fw head
	TDSP_STATUS   status;
	//DSP_AMEM_T      val;
	DSP_SET_CHANNEL_REQUEST_T val;
	UINT8 SrcPortNo,DstPortNo;		/*Sourc/Destination Port */
	UINT32 dma_src = 0;				/*DMA Source address*/
	UINT32 dma_dst = 0; 			/*DMA destination address*/
	UINT32 scratchbase,scratchoffset;
	UINT32 dma_control_word;
	UINT32 ABmem_steering_bit;
       TDSP_STATUS Status = STATUS_SUCCESS;
	UINT32 div,mod,i;
	PUINT32 pulBuf;

	SrcPortNo = SHUTTLEBUS__PORT_CARDBUS;
	DstPortNo = SHUTTLEBUS__PORT_UNIPHY;

	ASSERT(len <= 8);


	//
	

    	SrcPortNo = SB_PORT_CARDBUS;
    	DstPortNo = SB_PORT_UNIPHY;
//	scratchbase = sizeof(WLSCSR_STRUC) + sizeof(DUMMY1_STRUC);
	scratchbase = SCRATCH_MEM_BASE;


	scratchoffset = SCRATCH_INJECT_ABMEM_OFFSET;// + sizeof(UINT32);

	// dma buffer to scratch
	pulBuf = (PUINT32)Buffer;
	div = len >> 2;
	mod = len % 4;
	if(mod != 0)
		div++;
	for(i=0; i<div; i++)
		VcmdW_3DSP_Dword(pAdap,*(pulBuf+i),(UINT16)(scratchbase + scratchoffset+i*4));
//	if(mod != 0)
//		VcmdW_3DSP_Dword(pAdap,pbuf+i*4,scratchbase + scratchoffset+i);
//	i = VcmdR_3DSP_Dword(pAdap,(UINT16)(scratchbase + scratchoffset));
//	i = VcmdR_3DSP_Dword(pAdap,(UINT16)(scratchbase + scratchoffset + 4));

    	ABmem_steering_bit = 0;
	dma_control_word = SB_CONTROL(
		      len,
		      1,
                    1,
                    1,
                    0,
                    0,
                    0,
		      0,
                    0,
                    0,
                    0,
		      0 );

    dma_dst = SB_ADDRESS(DstPortNo, ABmem_steering_bit | (destoffset)>>2);
	
    dma_src = SB_ADDRESS(SrcPortNo, PCI_STEERING_BIT | (scratchbase + scratchoffset)>>2);

#if 1
	val.ctl_word = dma_control_word;
	val.dst_addr = dma_dst;
	val.src_addr = dma_src;
	val.request_cmd = VCMD_API_SET_CHANNEL_REQUEST;
	sc_memory_copy(val.channel, (PUINT8)Buffer,len);
	status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_SET_CHANNEL_REQUEST_T));

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__HAL(LEVEL_ERR, "Vcmd_scratch_2_DSP_Amemoffset() ERROR \n");
	}
#else	
    status = VcmdW_3DSP_Dword(pAdap,dma_src,WLS_CSR__DMA0_SRC_ADDR);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    status = VcmdW_3DSP_Dword(pAdap,dma_dst,WLS_CSR__DMA0_DST_ADDR);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    status = VcmdW_3DSP_Dword(pAdap,dma_control_word,WLS_CSR__DMA0_CONTROL);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

	{
		UINT32 ulCounter = 0;
	// wait for dma finished
	while(VcmdR_3DSP_Dword( pAdap,  WLS_CSR__STATUS) & BITS_STATUS__DMA0_BUSY)
		{
		ulCounter ++;
		if(ulCounter > 1000000)
			{
				DBGSTR(("wait for dma finished ------ failed \n"));
				return STATUS_FAILURE;
			}
		}
	}

#endif	
	
//	i = VcmdR_3DSP_Dword(pAdap,0x420c);
//	i = VcmdR_3DSP_Dword(pAdap,0x4210);
	
    return Status;
}

UINT32
Vcmd_get_version(PDSP_ADAPTER_T pAdap)
{
 	return (VcmdR_3DSP_Dword(pAdap,WLS_MAC__VERSION));
}
TDSP_STATUS 
Vcmd_hal_reset(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS  status = STATUS_SUCCESS;
	UINT32 val = 0;
	UINT32 loop = VENDOR_LOOP_MIDDLE;
	
	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	val |= MAC_SOFT_RESET_BIT;

	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__CONTROL))
	{
		return STATUS_FAILURE;
	}

	// Verify if Adapter is plugged in: check MAC Signature Register value
	while (loop-- && (VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL) & MAC_SOFT_RESET_BIT))
	{
		sc_sleep(1);
	}

	if(0 == loop)
	{
		DBG_WLAN__HAL(LEVEL_ERR, "vcmd_hal_reset fail \n");
		return STATUS_FAILURE;
	}

	return status;
	
}


TDSP_STATUS 
Vcmd_set_beacon_interval(PDSP_ADAPTER_T pAdap,UINT32 interval,UINT32 tbtt)
{
	return VcmdW_3DSP_Dword(
		pAdap,
		(UINT32)(interval & (BITS_BEACON_INTERVAL__INTERVAL)) |
		((UINT32)((tbtt << OFFSET_BEACON_INTERVAL__IMP_TBTT) & (BITS_BEACON_INTERVAL__IMP_TBTT))) ,
		WLS_MAC__BEACON_INTERVAL);
}



TDSP_STATUS 
Vcmd_set_rts_retrylimit(PDSP_ADAPTER_T pAdap,UINT32 rts_threshold,UINT32 s_retry,UINT32 l_retry)
{
	return VcmdW_3DSP_Dword(
		pAdap,
		(rts_threshold & BITS_RTS_RETRY__RTS_THRESHOLD) |
		((s_retry << OFFSET_RTS_RETRY__SHORT_RETRY) & BITS_RTS_RETRY__SHORT_RETRY) |
		((l_retry << OFFSET_RTS_RETRY__LONG_RETRY) & BITS_RTS_RETRY__LONG_RETRY),
		WLS_MAC__RTS_RETRY);
}
#if 0		//Justin: 0716.  duplicate with function Adap_WriteMacAddress
TDSP_STATUS 
Vcmd_hal_set_mac_addr(PDSP_ADAPTER_T pAdap,PUINT8 addr)
{
	UINT32 val;

	DBGSTR(("Vcmd_hal_set_mac_addr set mac addr = %x %x %x %x %x %x\n",
		addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]));
	//write low address of mac adr	
	val = (addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__MAC_ADDR_LO))
	{
		ASSERT(0);
		return STATUS_FAILURE;
	}

	//write high address of mac adr
	val = (addr[4]) | (addr[5] << 8);
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__MAC_ADDR_HI))
	{
		ASSERT(0);
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}
#endif

TDSP_STATUS 
Vcmd_hal_set_bssid(PDSP_ADAPTER_T pAdap,PUINT8 addr)
{
	UINT32 val;

	//write low address of mac adr	
	val = (addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__BSS_ID_LO))
	{
		ASSERT(0);
		return STATUS_FAILURE;
	}

	//write high address of mac adr
	val = (addr[4]) | (addr[5] << 8);
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__BSS_ID_HI))
	{
		ASSERT(0);
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

TDSP_STATUS 
Vcmd_mac_set_preamble_type( PDSP_ADAPTER_T pAdap,preamble_type_t preamble)
{
	UINT32 	val;
	//v4chip
	//only for v2,why
	if (pAdap->wlan_attr.chipID != 0)
	{
		if(TRUE == pAdap->mac_init_done)
		{
			val = VcmdR_3DSP_Dword(pAdap,WLS_SCRATCH__CTS_PREAMBLE);
			val &= (UINT32)(~BITS_HOST_CTRL_FEATURE__PREAMBLE_TYPE);
			if(SHORT_PRE_AMBLE == preamble)
			{
				val |= BITS_HOST_CTRL_FEATURE__PREAMBLE_TYPE;
			}
		    	VcmdW_3DSP_Dword(pAdap, val, WLS_SCRATCH__CTS_PREAMBLE);
	    	}
    	}
	return STATUS_SUCCESS;
	
}

TDSP_STATUS
Vcmd_hal_select_tx_fifo(PDSP_ADAPTER_T pAdap,UINT32 fifo_num)
{
	UINT32 val;
	val = fifo_num;
	return VcmdW_3DSP_Dword(pAdap,val,WLS_MAC__FIFO_SELECT);
}
	
	

TDSP_STATUS
Vcmd_hal_config_tx_fifo( PDSP_ADAPTER_T pAdap,UINT16 cp,UINT16 cfp,UINT16 bcn)
{ 
	UINT32 val;
	mac_hw_state_t state;
	val = 0;
//	VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__TX_FIFO_SIZE);

	if(Vcmd_get_current_state(pAdap,&val) != STATUS_SUCCESS)
	{
		return STATUS_FAILURE;
	}

	state = (mac_hw_state_t)val;

	if(state != DEV_IDLE)
	{
		Vcmd_set_next_state(pAdap,DEV_IDLE,0,0);
	}

	val = ( (cp) | ((cfp)<<OFFSET_TX_FIFO_SIZE__CFP_TXFIFO_SIZE) |
		((bcn)<<OFFSET_TX_FIFO_SIZE__BCN_TXFIFO_SIZE) );//|  (BIT_22) );
	return VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__TX_FIFO_SIZE);
}


TDSP_STATUS
Vcmd_mlme_config_tx_fifo(
	PDSP_ADAPTER_T pAdap,
    	UINT16	cp_size,
    	UINT16	cfp_size,
    	UINT16	bcn_size)
{
	UINT32 index;
	if(STATUS_SUCCESS != Vcmd_hal_config_tx_fifo(pAdap,cp_size>>6 ,cfp_size>>6,bcn_size>>6))
	{
		return STATUS_FAILURE;
	}

	//flush fifo
	for (index = 0; index <= 3; index++)
	{
		//select fifo
		if(STATUS_SUCCESS != Vcmd_hal_select_tx_fifo(pAdap, index))
		{
			ASSERT(0);
		}

		//flush tx fifo
		if(STATUS_SUCCESS != Vcmd_flush_tx_fifo(pAdap))
		{
			ASSERT(0);
		}
	}

	return STATUS_SUCCESS;
}

TDSP_STATUS    	
Vcmd_hal_set_dtim_cfp_param(
	PDSP_ADAPTER_T pAdap,
	UINT32 	dtim_period,
	UINT32    cfp_period,
	UINT32 	cfp_max_duration)
{
	return VcmdW_3DSP_Dword(pAdap,
		((dtim_period)|((cfp_period)<<OFFSET_CFP__CFP_PERIOD)|
		((cfp_max_duration)<<OFFSET_CFP__CFP_MAX_DURATION)),
		WLS_MAC__CFP);
}

VOID Vcmd_set_listenInterval(PDSP_ADAPTER_T pAdap,UINT16 listenInterval)
{//Justin
	TDSP_STATUS status;
	UINT32 state;

	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__STATE_CONTROL);
	state |= ((listenInterval  << OFFSET_STATE_CONTROL__LISTEN_INTERVAL) & BITS_STATE_CONTROL__LISTEN_INTERVAL);
 	
	 status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__STATE_CONTROL);
}

TDSP_STATUS Vcmd_set_wakeup_DTIM(PDSP_ADAPTER_T pAdap)
 {//Justin
	/*Wakeup DTIM interval.  When set in DOZE state, the core wakes up every DTIM beacons. */
	TDSP_STATUS status;
	UINT32 state;

	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	state |=MAC_WAKEUP_DTIM_BIT;
 	
	 status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);
	 return status;
 }

#define OLD_VENDOR



TDSP_STATUS Vcmd_reset_wakeup_DTIM(PDSP_ADAPTER_T pAdap)
 {//Justin
	/*Wakeup DTIM interval.  When set in DOZE state, the core wakes up every DTIM beacons. */
	TDSP_STATUS status;
	UINT32 state;

	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL) & (~ MAC_WAKEUP_DTIM_BIT);
 	
	 status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);
	 return status;
 }
   

TDSP_STATUS Vcmd_set_power_mng(PDSP_ADAPTER_T pAdap)
 {//Justin
//Power Management.  
//	When set, any frame transmitted by the core will have the pwrMgt bit set in the Frame control field. 
//	When reset, any frame transmitted by the core will have the pwrMgt bit reset in the Frame control field.
	TDSP_STATUS status;
	UINT32 state;

	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	state |=MAC_PWR_MGT_BIT;
 	
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);
	DBG_WLAN__HAL(LEVEL_ERR,"***** set power manager bit, status = 0x%x *****\n",status);
	 return status;
 }

TDSP_STATUS Vcmd_reset_power_mng(PDSP_ADAPTER_T pAdap)
 {//Justin
	TDSP_STATUS status;
	UINT32 state;

	state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL) &(~ MAC_PWR_MGT_BIT);
 	
	status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__CONTROL);
	DBG_WLAN__HAL(LEVEL_ERR,"***** clear power manager bit, status = 0x%x *****\n",status);
	 return status;
 }

TDSP_STATUS Vcmd_get_current_state(PDSP_ADAPTER_T pAdap,PUINT32 state)
 {//Justin
	//UINT32 state = 0;

	if(Adap_Driver_isHalt(pAdap))
	{
		return STATUS_FAILURE;
	}

	*state = VcmdR_3DSP_Dword(pAdap, WLS_MAC__STATE_CONTROL) ;
	*state &= BITS_STATE_CONTROL__CURR_STATE;

	return STATUS_SUCCESS;
 }

TDSP_STATUS Vcmd_get_next_state(PDSP_ADAPTER_T pAdap,PUINT32 state)
 {//Justin
	//UINT32 state = 0;

	if(Adap_Driver_isHalt(pAdap))
	{
		return STATUS_FAILURE;
	}
	
	*state = (VcmdR_3DSP_Dword(pAdap, WLS_MAC__STATE_CONTROL)& (BITS_STATE_CONTROL__NEXT_STATE) 
		>> OFFSET_STATE_CONTROL__NEXT_STATE);
 	
	 //return state;
	return STATUS_SUCCESS; 
 }


/*
 * Name						: Vcmd_set_next_state()
 * arguments				: void
 * return value				: void
 * global variables 		: none 
 * description				: This fn is used to set the hw state.Also it is used to set the
 *							: scan_type during scan, join timeout during join and listen
 *							: interval during doze.
 * see also					: none
 * TODO						: none
 */

void Vcmd_set_next_state(PDSP_ADAPTER_T pAdap, UINT8 state, UINT8 scan_type, UINT16 time_value)
{//Justin
	mac_hw_state_t	machw_state;
	mac_hw_state_t	machw_state_cur;
	mac_hw_state_t	machw_state_next;
	UINT32			loopcnt, delay=0, writes=0;
	UINT32			var;         //Jakio20070528: add here

	machw_state = (mac_hw_state_t)state;
	loopcnt = 10*VENDOR_LOOP_MAX; // set max wait time about one sec
	
//	WLAN_DBGSTR3("%s: %d : hal_set_state_control(%d)\n",FNAMEGET(__FILE__),__LINE__, state);

	// Verify if Adapter is plugged in: check MAC Signature Register value
	while (--loopcnt)// && AdapterPluggedIn(pAdapter))
	{
		// Check the next state - write the new state if not set yet
		//machw_state_next = (mac_hw_state_t)Vcmd_get_next_state(pAdap);
		if(Vcmd_get_next_state(pAdap, &var) != STATUS_SUCCESS)
		{
		   return;
		}

		machw_state_next = (mac_hw_state_t)(var);
		
		if(machw_state_next != machw_state)
		{
			VcmdW_3DSP_Dword(pAdap, 
				REG_STATE_CONTROL(state, (scan_type & 0x01), time_value),
				WLS_MAC__STATE_CONTROL);
//			WriteToReg(pAdapter, WLS_MAC_STATECONTROL_WD,
//					((state << 4) | ((scan_type & 0x01) << 8) | (time_value << 16)));
			writes +=1;
		}
		// Check the current state - exit if set to the new state already
		//machw_state_cur = (mac_hw_state_t)Vcmd_get_current_state(pAdap,&var);
		if(Vcmd_get_current_state(pAdap,&var) != STATUS_SUCCESS)
			return;

		machw_state_cur = (mac_hw_state_t)var;
		
		if(machw_state == machw_state_cur) 
			break;

		sc_sleep(1);
		delay += 10;
	}

	if(loopcnt == 0)
	{ // failed
//		mac_print_rx_stat(pAdapter);
//		mac_print_tx_stat(pAdapter);
//		mmacTxMpduHndlFifoShow(pAdapter, MMAC_TXFIFO_CP);
//		WLAN_ERROR2("hal_set_state_control: fail to go from state %d to %d\n", machw_state_cur, machw_state);
//		CLIENT_PRINTF("macHW debug2=%x, txFrgCnt=%x.\n", ReadFromReg(pAdapter, WLS_MAC_DEBUG2_WD),
//						ReadFromReg(pAdapter, WLS_MAC_TXFRGCOUNT_WD));
		DBG_WLAN__HAL(LEVEL_ERR,"[%s] WLAN set state fail \n",__FUNCTION__);
	}
	else if (delay)
	{ // display using time
//		WLAN_INFO2("hal_set_state_control: state transition took %d ms and %d writes\n", delay / 1000, writes);
	}

//	WLAN_DBGSTR2("%s: %d : hal_set_state_control() returns\n",FNAMEGET(__FILE__),__LINE__);
}


//TDSP_STATUS 
//Vcmd_set_next_state(PDSP_ADAPTER_T pAdap)
// {
//
//	return VcmdW_3DSP_Dword(
//		pAdap,
//		(interval & BITS_BEACON_INTERVAL__INTERVAL) |
//		((tbtt << OFFSET_BEACON_INTERVAL__IMP_TBTT) & BITS_BEACON_INTERVAL__IMP_TBTT) ,
//		WLS_MAC__BEACON_INTERVAL);
//
//	TDSP_STATUS status;
//	UINT32 state;
////(((next_state) << OFFSET_STATE_CONTROL__NEXT_STATE) & (BITS_STATE_CONTROL__NEXT_STATE))
//	state = VcmdR_3DSP_Regs(pAdap, WLS_MAC__STATE_CONTROL);
//	state &=(~BITS_STATE_CONTROL__NEXT_STATE);
//	state
// 	
//	 status = VcmdW_3DSP_Dword(pAdap,state,WLS_MAC__STATE_CONTROL);
//	 return status;
// }


VOID Vcmd_unmask_idle_intr(PDSP_ADAPTER_T pAdap)
{//Justin
	VcmdW_3DSP_Dword(pAdap,ALL_BITS , WLS_MAC__INT_MASK);
	VcmdW_3DSP_Dword(pAdap,ALL_BITS ,WLS_MAC_INTBBMASK_WD);
}

VOID Vcmd_clear_intrpt(PDSP_ADAPTER_T pAdap,UINT32 interupt)
{//Justin
	UINT32 data;
	data = VcmdR_3DSP_Dword(pAdap, WLS_MAC__INT_EVT_CLEAR) & BITS_INT_EVENT__IDLE_INTERRUPT;
	VcmdW_3DSP_Dword(pAdap, data, WLS_MAC__INT_EVT_CLEAR);
}

VOID Vcmd_mask_idle_intr(PDSP_ADAPTER_T pAdap)
{//Justin
	VcmdW_3DSP_Dword(pAdap,ALL_BITS & (~BITS_INT_MASK__MASTER_INT_EN), WLS_MAC__INT_MASK);
	VcmdW_3DSP_Dword(pAdap,ALL_BITS & (~BITS_INT_MASK__MASTER_INT_EN), WLS_MAC_INTBBMASK_WD);
}

/*******************************************************************
 *   Adap_Set_SoftReset
 *   
 *   Descriptions:
 *      The routine of Soft_Reset contorl command.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      bitmap: IN, soft reset bit map.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Adap_Set_SoftReset(PDSP_ADAPTER_T pAdap, UINT16 bitmap)
{
	//Justin: 0920.  must disable interrupt before NIC reset. otherwise, reset maybe fail
 	Vcmd_NIC_INTERRUPT_DISABLE(pAdap);

    DBG_WLAN__HAL(LEVEL_TRACE, "Adap_Set_SoftReset 1 \n"); 
	//Vcmd_flush_rx_fifo(pAdap);


	// Soft reset and leave NIC in a sleep mode.  Leave UniPHY core in reset.
	 if(STATUS_SUCCESS !=Vcmd_CardBusNICReset(pAdap))
	 {
	    
        DBG_WLAN__HAL(LEVEL_TRACE, "Adap_Set_SoftReset 2 \n");
	   	return STATUS_FAILURE;
	 }
	 return STATUS_SUCCESS;
}


/*******************************************************************
 *   Adap_Set_Power_Management_Mode
 *   
 *   Descriptions:
 *      This command is used for driver to inform station is in Active mode or in Power Save mode.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      IsPowerMng: IN, true if device is configured into power save mode, false if device is configured into active mode.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Adap_Set_Power_Management_Mode(PDSP_ADAPTER_T pAdap, BOOLEAN IsPowerSave,BOOLEAN ps_poll_flag)
{// Justin
	UINT8 next_state;
	mac_hw_state_t state;
/*
	if(pAdap->link_ok != LINK_OK
		|| pAdap->wlan_attr.hasjoined != JOINOK)
	{
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		return STATUS_SUCCESS;
	}
//*/

	//Adap_Set_Driver_State(pAdap, DSP_STOP_TX_WHILE_PWRMGR);

	DBG_WLAN__HAL(LEVEL_TRACE, "Adap_Set_Power_Management_Mode, save = %d\n",IsPowerSave);

//	set these:
//	4. control.pwrMgt: Power Management.  
	//	When set, any frame transmitted by the core will have the pwrMgt bit set in the Frame control field.  
	//	When reset, any frame transmitted by the core will have the pwrMgt bit reset in the Frame control field.
	//	mac_set_pow_mgt_bit(pAdapter, (pAdapter->gdevice_info.ps_mode != PSS_ACTIVE));
//	5. tell ap we will change to doze mode
//	1. stateControl.nextState = DOZE
//	2. stateControl.listenInterval
//	3. control.wakeupDTIM: Wakeup DTIM interval. 
	//	When set in DOZE state, the core wakes up every DTIM beacons. 
	if (IsPowerSave)		// to save mode
	{
//		Vcmd_set_power_mng(pAdap);
		
//		Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode

		Vcmd_set_listenInterval(pAdap, pAdap->wlan_attr.listen_interval);
		Vcmd_set_wakeup_DTIM(pAdap);
		next_state = DEV_DOZE;
	}
	else
	{
		//Vcmd_reset_power_mng(pAdap);
		next_state = DEV_ACTIVE;	//zyy:
	}

	Vcmd_get_current_state(pAdap,(PUINT32)&state);

	// before send a frame indicate ap, we must change the core to active state
	if ((pAdap->wlan_attr.gdevice_info.ps_mode != PSS_ACTIVE) 
		//|| (Vcmd_get_current_state(pAdap) != DEV_ACTIVE))
		|| (state != DEV_ACTIVE))
	{
		Vcmd_mask_idle_intr(pAdap);
		Vcmd_set_next_state(pAdap, DEV_IDLE, 0, 0);
		Vcmd_clear_intrpt(pAdap, BITS_INT_EVENT__IDLE_INTERRUPT);	//
		Vcmd_set_next_state(pAdap, DEV_ACTIVE, 0, 0);
		Vcmd_unmask_idle_intr(pAdap);
	}

	if ( !IsPowerSave) 
	{
//		Vcmd_reset_power_mng(pAdap);
		
/*		if (pAdap->wlan_attr.gdevice_info.ps_mode == PSS_FAST_DOZE) 
		{
			pAdap->wlan_attr.gdevice_info.ps_mode = PSS_ACTIVE;	//must set ps_mode before indicate ap
			Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode
		}
		else 
*/
			pAdap->wlan_attr.gdevice_info.ps_mode = PSS_ACTIVE;
#if 0 // TODO:Jackie
        if(ps_poll_flag)
		{
			Mng_MakePsPollFrame(pAdap);	// to indicate ap our next ps mode
		}
		else
		{
			Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode
		}
#endif //#if 0 // TODO:Jackie
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		return STATUS_SUCCESS; 
	}

	///////////////////////////////// change to doze state //////////////////////////
#if 0
	Vcmd_set_power_mng(pAdap);
	
	Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode

	//wait mng packet send complete
	while((!PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->ptx_packet_list))
			||(!PktList_IsEmpty((PDSP_PKT_LIST_T)pAdap->pmng_queue_list)))//no more packet need to send
	{
		tdsp_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);
		Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);
		sc_sleep(50);
	}
#endif
	pAdap->wlan_attr.gdevice_info.ps_mode = PSS_CLASSIC_DOZE;//must set ps_mode before indicate ap

//	if ((hal_get_current_state(pAdap) != DEV_ACTIVE))
//	{
		Vcmd_mask_idle_intr(pAdap);
		Vcmd_set_next_state(pAdap, DEV_IDLE, 0, 0);
		Vcmd_clear_intrpt(pAdap, BITS_INT_EVENT__IDLE_INTERRUPT);	//
		Vcmd_set_next_state(pAdap, DEV_DOZE, 0, pAdap->wlan_attr.listen_interval);
		Vcmd_unmask_idle_intr(pAdap);
//	}
//	pAdap->wlan_attr.gdevice_info.ps_mode = PSS_CLASSIC_DOZE;
	Adap_Set_Driver_State(pAdap, DSP_DRIVER_WORK);

	return STATUS_SUCCESS;
	
}

/*******************************************************************
 *   Adap_Set_Inform_Force_Sending
 *   
 *   Descriptions:
 *      This command is used for driver to inform FW that driver wants to FW sending
 *      a RX or Interrupt frame. This command is usally sent when driver's halt or internalreset
 *      function is called. After this function is called, FW will send rx or interrupt data to
 *      driver and driver will recycle the rx irps or interrupt irps.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      IsBulkin: IN, true if rx data is wanted to be sent, false if interrupt data is wanted to be sent.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Adap_Set_Inform_Force_Sending(PDSP_ADAPTER_T pAdap, BOOLEAN IsBulkin)
{
	UINT16 value = 0;
	
	if (IsBulkin)
		value = 1;

	return STATUS_SUCCESS;
}


/*******************************************************************
 *   Adap_WriteRegByte
 *   
 *   Descriptions:
 *      Write one byte to a register.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
 /*
TDSP_STATUS Adap_WriteRegByte(PDSP_ADAPTER_T pAdap,
						      UINT8 value,
							  UINT16 offset
						      )
{
	return VcmdW_3DSP_Regs(pAdap,&value,1,offset);
	//return (Adap_WriteCommand(pAdap,&value,1,offset,0x0));
}
*/

/*******************************************************************
 *   Adap_WriteRegWord
 *   
 *   Descriptions:
 *      Write one word to a register.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
 /*
TDSP_STATUS Adap_WriteRegWord(PDSP_ADAPTER_T pAdap,
							  UINT16 value,
							  UINT16 offset
							  )
{
	//return (Adap_WriteCommand(pAdap,(PUINT8)&value,2,offset,0x0));
	return VcmdW_3DSP_Regs(pAdap,(PUINT8)&value,2,offset);
}
 */
/*******************************************************************
 *   Adap_WriteRegDword
 *   
 *   Descriptions:
 *      Write one double byte to a register.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
 /*
TDSP_STATUS Adap_WriteRegDword(PDSP_ADAPTER_T pAdap,
							  UINT32 value,
							  UINT16 offset
							  )
{
	//return (Adap_WriteCommand(pAdap,(PUINT8)&value,4,offset,0x0));
	return VcmdW_3DSP_Dword(pAdap,value,offset);
}
*/


/*******************************************************************
 *   Adap_ReadMacAddress
 *   
 *   Descriptions:
 *      Read mac address from eeprom through control pipe.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      addr: OUT, the startting address of the received address.
 *   Return Value:
 *      NONE.
 ******************************************************************/
VOID  Adap_ReadMacAddress(PDSP_ADAPTER_T pAdap,
								PUINT8 addr)
{
	UINT32   val;

	//get low 4bytes of mac addr
	val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__MAC_ADDR_LO);
//	val &= 0xFFFFFF00;			// Justin: for test only. we need the even lowest bit 
//	val |= 0x00000100;

	sc_memory_copy(addr,&val,sizeof(UINT32));

	//get high 2bytes of mac addr
	val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__MAC_ADDR_HI);
	sc_memory_copy(addr + 4,&val,sizeof(UINT16));
}

/*******************************************************************
 *   Adap_WriteMacAddress
 *   
 *   Descriptions:
 *      Write mac address to mac register through control pipe.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *      addr: IN, the startting address.
 *   Return Value:
 *      NONE.
 ******************************************************************/
TDSP_STATUS  Adap_WriteMacAddress(PDSP_ADAPTER_T pAdap,
								PUINT8 addr
								)
{
	UINT32   val;

	//get low 4bytes then set
	val = *((PUINT32)addr);
//	val &= 0xFFFFFF00;			// Justin: for test only. we need the even lowest bit 
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,val,WLS_MAC__MAC_ADDR_LO))
	{
		return STATUS_FAILURE;
	}

	//get high 4bytes then set
	val = *((PUINT32)(addr + 4));
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,val,WLS_MAC__MAC_ADDR_HI))
	{
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

/*******************************************************************
 *   Adap_ReadFirmwareVersion
 *   
 *   Descriptions:
 *      Read firmware version information through control pipe.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE.
 ******************************************************************/
VOID  Adap_ReadFirmwareVersion(PDSP_ADAPTER_T pAdap)
{
	return;
}

  
/*		//justin: 20070927
1. eeprom was map to scratch while card power on
2. copy eeprom to driver memory from scratch
3. parse eeprom to get power table. eeprom format is :
-----------------------------
    ID	1 byte
    len	1 byte
    info	len
--------------------------------
NOTE: the last element is powertable, which ID is 'FF' and followed by info but not len.

*/
TDSP_STATUS GetTxPwrTable_from_eeprom(PDSP_ADAPTER_T pAdap)
{
	UINT16	i = 0;
	UINT16	index = 0;
	TDSP_STATUS Status = STATUS_SUCCESS;
	UINT8	valid_flag[]={0x11,0x22,0x33,0x44};
	UINT32	txpow_default[3]={ 0x36303030, 0x36322e2a, 0x36322e2a};	//Justin: 0712. edit ch11 to power level 54, reference from test result by mfp

	sc_memory_set(pAdap->u_eeprom, 0, sizeof(pAdap->u_eeprom));
	
	(*((PUINT32)(pAdap->u_eeprom+EEPROM_OFFSET_SUBSYSTEMID)) ) = VcmdR_PowerTable(pAdap, EEPROM_OFFSET_SUBSYSTEMID);
	(*((PUINT32)(pAdap->u_eeprom+POWER_TABLE_VALID_FLAG_OFFSET)) ) = VcmdR_PowerTable(pAdap, POWER_TABLE_VALID_FLAG_OFFSET);

	//Get correct table
	if(0 == sc_memory_cmp(pAdap->u_eeprom + POWER_TABLE_VALID_FLAG_OFFSET,valid_flag,POWER_TABLE_VALID_FLAG_LEN))
	{
	    	while(static_axLookupFreqByChannel_BOnly[i].channel)
    		{
			pAdap->TxPwrTable[index] = VcmdR_PowerTable(pAdap, (index << 2) + POWER_TABLE_PROGRAM_OFFSET);
			index++;
			pAdap->TxPwrTable[index] = VcmdR_PowerTable(pAdap, (index << 2) + POWER_TABLE_PROGRAM_OFFSET);
			index++;
			pAdap->TxPwrTable[index] = VcmdR_PowerTable(pAdap, (index << 2) + POWER_TABLE_PROGRAM_OFFSET);
			index++;
    			i++;
    		}
	}
	//set default value
	else
	{
		while(static_axLookupFreqByChannel_BOnly[i].channel)
		{
			pAdap->TxPwrTable[index] = txpow_default[0];
			pAdap->TxPwrTable[index+1] = txpow_default[1];
			pAdap->TxPwrTable[index+2] = txpow_default[2];
			index = index+3;
			i++;
		}
	}

	return Status;
}


BOOLEAN Adap_Device_Removed(PDSP_ADAPTER_T pAdap)
{
	UINT8 val = 0xff;
	val = Basic_ReadRegByte(pAdap,REG_USBCTL1);

	//PCI READY
	if(((val & BIT_PCIRDY) != BIT_PCIRDY) || (val ==0xff) || (val ==0x00))
	{
		return TRUE;
	}

	return FALSE;
}		

TDSP_STATUS Adap_hal_set_rx_any_bssid_beacon(
	PDSP_ADAPTER_T pAdap//, 
//	UINT32 val
	)
{
	UINT32 ulTemp;
	ulTemp = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	ulTemp |= MAC_RX_ANY_BSSID_BEACON_BIT;

	return VcmdW_3DSP_Dword(pAdap,  ulTemp, WLS_MAC__CONTROL);
}

