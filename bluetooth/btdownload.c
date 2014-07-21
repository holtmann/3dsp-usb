/***********************************************************************
 * FILENAME:         btdownload.c
 * CURRENT VERSION:
 * CREATE DATE:      2007
 * PURPOSE:  G
 *
 * AUTHORS:  Peter han
 *
 * NOTES:    //
 ***********************************************************************/
#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_hci.h"
#include "btdownload.h"
#include "bt_pr.h"
#include "bt_usbregdef.h"
#include "bt_dbg.h"
#include "bt_usb_vendorcom.h"   
#include "bt_hal.h"
#ifndef BT_DOWNLOAD_BIN_WITH_FILE_MODE
#include "sp20code.h"
#endif


/*****************************************************************
 *ChangeLog:
 *	glen20081031 add for hotkey check
*****************************************************************/
extern unsigned int combodrv;    
NTSTATUS SetHotKeyenable(IN PBT_DEVICE_EXT devExt)
{
	UINT8 tmp;

	BtDelay(100);
if (combodrv && Bus_get_hotkey_flag(devExt->pBusInterface))
		tmp = 0x5A;
	else
		tmp = 0x55;

	BT_DBGEXT(ZONE_DLD | LEVEL0, "Write HotKeyEnable value %02X\n", tmp);
	return VendorCmdBurstRegWrite2(devExt, 0x11C, &tmp, sizeof(UINT8));
}

inline unsigned long sc_readfile(struct file * filp, void* buffer, unsigned long buff_len, unsigned long offset)
{
	mm_segment_t old_fs;
	loff_t pos;
	unsigned long len;
	pos = (loff_t)offset;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	len = (filp->f_op->read(filp, (char*)buffer, buff_len, &pos));
	set_fs(old_fs);
	return len;
}

NTSTATUS DownLoadFirmWare(IN PBT_DEVICE_EXT deviceExtension, IN UINT8 Type, BOOLEAN GotoMainLoop)
{
	NTSTATUS ntStatus;
	if (Type == 0)
	{
		ntStatus = DownLoad8051FirmWare(deviceExtension);
		
		BtDelay(5000);
		
		if(ntStatus != STATUS_SUCCESS)
			return STATUS_UNSUCCESSFUL;
		
		if(STATUS_SUCCESS != Vcmd_CardBusNICReset(deviceExtension))
			return STATUS_UNSUCCESSFUL;
		if(STATUS_SUCCESS != DownLoad3DspFirmWare(deviceExtension, GotoMainLoop))
			return STATUS_UNSUCCESSFUL;

	}
	else if (Type == 1)
	{
		Vcmd_CardBusNICReset(deviceExtension);
		ntStatus = DownLoad3DspFirmWare(deviceExtension,GotoMainLoop);
	}
	else
	{
		return STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}

/*****************************************************************
 *ChangeLog:
 *	Jakio modified this function using SEH tech, to avoid quiting unnormally
*****************************************************************/
NTSTATUS DownLoad8051FirmWare(IN PBT_DEVICE_EXT devExt)
{

#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE // @20080928 by glen for bin code file support
	PUINT8 filename = "/usr/local/3DSP/usb/3dspcode.bin";
	struct file * filp = NULL;
	UINT32				BinCodeLen;
	PUINT8				pBinDataBuf= NULL;			
	PSP20_BIN_CODE pSp20Bincode;
	PSP20_FILE_HEAD_T pSp20_file_head;
#else
	PSP20_CODE pSp20code = NULL;
#endif //  @20080928 by glen

	NTSTATUS ntStatus = STATUS_SUCCESS;
	PUINT8 Buffer = NULL;
	UINT32 Length = DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
	UINT32 offset;
	UINT32 DwordOffset;
	UINT8 FunctionOk = FALSE;
	LARGE_INTEGER TimeOut;
	UINT32 lastLen = 0;
	PUINT8 tmpBuf;


	Length = DSP_8051_FIRMWARE_FRAGMENT_LENGTH;

	offset = 0;
	DwordOffset = 0;

	__try
	{
		Buffer = ExAllocatePool(DSP_8051_FIRMWARE_FRAGMENT_LENGTH, GFP_KERNEL);
		BT_DBGEXT(ZONE_DLD | LEVEL0, "Buffer = %p\n", Buffer);
		if (Buffer == NULL)
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate buffer for download 8051 firmware failure\n");
			__leave;
		}

	
		ntStatus = DownStopRun8051(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "stop run 8051 firmware failure\n");
		}
		BtDelay(5000);

#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE
		BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad 8051 code from bin file\n");
		pBinDataBuf = ExAllocatePool(BIN_CODE_HEADSZIE, GFP_KERNEL);
		if (pBinDataBuf == NULL)
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download 8051 firmware failure:step1\n");
			__leave;
		}	

		filp = filp_open(filename, O_RDONLY, 0);
		if (filp == NULL || IS_ERR((struct file *)filp))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0,  "Failed to open bincode file:%s!\n", filename );
			filp = NULL;
			__leave;
    	}

/*
    		if (( !filp->f_op ) || ( !filp->f_op->read ))
    		{
        		BT_DBGEXT(ZONE_DLD | LEVEL0,  "filp->f_op or file->f_op->read is null!\n" );
        		filp_close( filp, NULL );
        		filp = NULL;
			__leave;
    		}
*/

		Length = BIN_CODE_HEADSZIE;
		if(BIN_CODE_HEADSZIE != sc_readfile(filp, pBinDataBuf, Length, 0))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0,  "Failed to read BIN_CODE_HEAD!\n");
			__leave;
		}

		pSp20_file_head = (PSP20_FILE_HEAD_T )pBinDataBuf;
		offset = pSp20_file_head->combo8051_offset;
		BinCodeLen = pSp20_file_head->combo8051_len;	
		BT_DBGEXT(ZONE_DLD | LEVEL0, "8051 FW offset:%x Len:%x\n",offset,BinCodeLen);
		if(pBinDataBuf!=NULL)
		{
			ExFreePool(pBinDataBuf);
			pBinDataBuf = NULL;
		}
		pBinDataBuf = ExAllocatePool(BinCodeLen, GFP_KERNEL);
		if (pBinDataBuf == NULL)
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step2\n");
			__leave;
		}

		Length = BinCodeLen;
		if(Length != sc_readfile(filp, pBinDataBuf, Length, offset))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
			__leave;
		}
		
		pSp20Bincode = (PSP20_BIN_CODE)pBinDataBuf;
		BT_DBGEXT(ZONE_DLD | LEVEL0, "8051 FW sector:%x Len:%x\n",pSp20Bincode->Port,pSp20Bincode->Size);		

		filp_close(filp, NULL);
		filp = NULL;

		offset = 0;
		DwordOffset = 0;
		do
		{
			DwordOffset = (DOWNLOAD_8051_FW_FIELD_OFFSET + offset + 3) >> 2; // double word align
			RtlZeroMemory(Buffer, DSP_8051_FIRMWARE_FRAGMENT_LENGTH);
			if(pSp20Bincode->Size - offset >= DSP_8051_FIRMWARE_FRAGMENT_LENGTH )
			{
				lastLen =Length = DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
			}
			else
			{
				Length = pSp20Bincode->Size - offset;
				if(Length%4 == 0)
				{
					lastLen = Length;
				}
				else
				{
					lastLen =  Length + (4 - Length%4);
				}
			}
			RtlCopyMemory(Buffer,pSp20Bincode->Code+offset,Length);
			if ((offset >= 4096) && (offset < 0xC000))
			{
				offset += Length;
				continue;
			}
			DownSetDma(devExt, (UINT16)DwordOffset, DMA_LOCAL_PROG_MEM, (UINT16)lastLen);
			DownBulkOutStart(devExt);
			ntStatus = BtUsbWrite(devExt, Buffer, lastLen, MAILBOX_DATA_TYPE_FIRMWARE);
			if ((ntStatus == STATUS_SUCCESS))
			{
				TimeOut.QuadPart =  HZ;				
				ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
				atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
				if(ntStatus == 0)
				{
					if(TRUE == UsbCancelAclIrp(devExt))
					{
						TimeOut.QuadPart =  HZ*5;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
					}
					else
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad8051FirmWare---Error, cancel irp failed\n");
					}
				}
			}
			
			if (!NT_SUCCESS(ntStatus))
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "failure bulkout when download 8051 with status:%8x\n", ntStatus);
				__leave;
			}

			if(devExt->InD3Flag == 1)
			{
				DownSetBulkInDma(devExt, (UINT16)DwordOffset, DMA_LOCAL_PROG_MEM, (UINT16)lastLen);
				DownBulkInStart(devExt);
				BtUsbSubmitReadIrp(devExt);

				TimeOut.QuadPart =  HZ;
				atomic_set(&devExt->RxFwFlag, EVENT_UNSIGNED);
				ntStatus = wait_event_interruptible_timeout(devExt->RxFwEvent, atomic_read(&devExt->RxFwFlag) == EVENT_SIGNED, TimeOut.QuadPart);
				if(ntStatus == 0)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Download8051Firmware---failed to wait rx fw event\n");
				}
				else
				{
					
					tmpBuf = devExt->UsbContext.UsbBulkInContext.pRecvArea[0]->buffer;
					if(0)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Error----fw verify failed, offset:%x\n", offset);
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Orignal fw buffer--------------------------------\n");
						BtPrintBuffer(Buffer, lastLen);
						BT_DBGEXT(ZONE_DLD | LEVEL0, "verified fw buffer--------------------------------\n");
						BtPrintBuffer(tmpBuf, lastLen);
						continue;
					}
					else
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "fw Verify ok\n");
					}
				}				
			}


			
			offset += Length;
		}while (pSp20Bincode->Size != offset);
#else
		pSp20code = (PSP20_CODE)Usb_8051CodeBlueToothOnly;
		


		do
		{
			DwordOffset = (DOWNLOAD_8051_FW_FIELD_OFFSET + offset + 3) >> 2; // double word align
			RtlZeroMemory(Buffer, DSP_8051_FIRMWARE_FRAGMENT_LENGTH);
			if(pSp20code->Size - offset >= DSP_8051_FIRMWARE_FRAGMENT_LENGTH )
			{
				lastLen =Length = DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
			}
			else
			{
				Length = pSp20code->Size - offset;
				if(Length%4 == 0)
				{
					lastLen = Length;
				}
				else
				{
					lastLen =  Length + (4 - Length%4);
				}
			}
			RtlCopyMemory(Buffer,pSp20code->Code+offset,Length);
			if ((offset >= 4096) && (offset < 0xC000))
			{
				offset += Length;
				continue;
			}
			DownSetDma(devExt, (UINT16)DwordOffset, DMA_LOCAL_PROG_MEM, (UINT16)lastLen);
			DownBulkOutStart(devExt);
			ntStatus = BtUsbWrite(devExt, Buffer, lastLen, MAILBOX_DATA_TYPE_FIRMWARE);
			if ((ntStatus == STATUS_SUCCESS))
			{
				TimeOut.QuadPart =  HZ;				
				ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
				atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
				if(ntStatus == 0)
				{
					if(TRUE == UsbCancelAclIrp(devExt))
					{
						TimeOut.QuadPart =  HZ*5;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
					}
					else
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad8051FirmWare---Error, cancel irp failed\n");
					}
				}

			}
			
			if (!NT_SUCCESS(ntStatus))
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "failure bulkout when download 8051 with status:%ld\n", ntStatus);
				__leave;
			}


			if(devExt->InD3Flag == 1)
			{
				DownSetBulkInDma(devExt, (UINT16)DwordOffset, DMA_LOCAL_PROG_MEM, (UINT16)lastLen);
				DownBulkInStart(devExt);
				BtUsbSubmitReadIrp(devExt);

				TimeOut.QuadPart =  HZ;
				atomic_set(&devExt->RxFwFlag, EVENT_UNSIGNED);
				ntStatus = wait_event_interruptible_timeout(devExt->RxFwEvent, atomic_read(&devExt->RxFwFlag) == EVENT_SIGNED, TimeOut.QuadPart);
				if(ntStatus == 0)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Download8051Firmware---failed to wait rx fw event\n");
				}
				else
				{
					
					tmpBuf = devExt->UsbContext.UsbBulkInContext.pRecvArea[0]->buffer;
					if(0)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Error----fw verify failed, offset:0x%lx\n", offset);
						BT_DBGEXT(ZONE_DLD | LEVEL3, "Orignal fw buffer--------------------------------\n");
						BtPrintBuffer(Buffer, lastLen);
						BT_DBGEXT(ZONE_DLD | LEVEL3, "verified fw buffer--------------------------------\n");
						BtPrintBuffer(tmpBuf, lastLen);
						continue;
					}
					else
					{
						BT_DBGEXT(ZONE_DLD | LEVEL4, "fw Verify ok\n");
					}
				}				
			}


			
			offset += Length;
		}while (pSp20code->Size != offset);

#endif

		ntStatus = DownStartRun8051(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "start run 8051 failure\n");
			__leave;
		}
		FunctionOk = TRUE;
	}
	__finally
	{
		if(Buffer)
			ExFreePool(Buffer);
#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE
		if(pBinDataBuf)
			ExFreePool(pBinDataBuf);
		if(filp)
		{
			filp_close(filp, NULL);
		}
#endif
	}

	if(FunctionOk == TRUE)
	{
		BT_DBGEXT(ZONE_DLD | LEVEL3, "DownLoad 8051 firmware ok\n");
		return STATUS_SUCCESS;
	}
	else
	{
		BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad 8051 firmware failure\n");
		return STATUS_UNSUCCESSFUL;
	}


}

/*
Name					: mmacInit()
return value			: MMACSTATUS
global variables 		: void
description				: It initializes MiniMAC.
special considerations 	: It needs to be preceeded by mac_init().
see also				:
 */
NTSTATUS Adap_mmacinit(IN PBT_DEVICE_EXT deviceExtension)
{
	UINT32 val;

#if 1	/* jakio20071030: temply mark here
*/
	if(STATUS_SUCCESS != VendorCmdWrite3DSPDword(deviceExtension,  
		    MAC_BBREG_ACCESSS(BBREG_ACCESS_EN, BBREG_ACCESS_WRITE, 0x29, 24),
		    WLS_MAC__BBREG_ACCESS_WD))
	{
		goto error_exit;
	}

	val = VendorCmdRead3DSPDword(deviceExtension, WLS_MAC__CONTROL);
	val |= MAC_ENABLE_ECO_ITEM17;

	if(STATUS_SUCCESS !=VendorCmdWrite3DSPDword(deviceExtension, val,WLS_MAC__CONTROL))
	{
		goto error_exit;
	}
#endif		
	
	val = VendorCmdRead3DSPDword(deviceExtension, WLS_CSR__PCI_CONTROL_REGISTER);
	val &= 0x0001ffff;
	if(STATUS_SUCCESS !=VendorCmdWrite3DSPDword(deviceExtension, val,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		goto error_exit;
	}
	


	
	if (STATUS_SUCCESS != Adap_mmacRestart_initial(deviceExtension, 11))
	{
		goto error_exit;
	}
	return STATUS_SUCCESS;
	error_exit:
	return STATUS_UNSUCCESSFUL;

}

NTSTATUS StartDspFirmware(IN PBT_DEVICE_EXT deviceExtension)
{
	Vcmd_NIC_INTERRUPT_ENABLE(deviceExtension);
	Vcmd_NIC_ENABLE_RETRY(deviceExtension, CARDBUS_NUM_OF_RETRY);
	if (STATUS_SUCCESS != Adap_mmacinit(deviceExtension))
	{
		Vcmd_NIC_INTERRUPT_DISABLE(deviceExtension);
		Vcmd_NIC_RESET_ALL_BUT_HARDPCI(deviceExtension);
		return STATUS_UNSUCCESSFUL;
	}
	/*
	for(i = 0x2928;i<0x2fd8;i++)
	{
	UsbWriteTo3DspReg(deviceExtension,i,0);
	}
	 */
	/* Set download-ok flag to 8051 */
	BtDelay(100);
	return STATUS_SUCCESS;
}

/*****************************************************************
 *ChangeLog:
 *	Jakio modified this function using SEH tech, to avoid quiting unnormally
*****************************************************************/
NTSTATUS  DownLoad3DspFirmWare(IN PBT_DEVICE_EXT devExt, BOOLEAN GotoMainLoop)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE // @20080928 by glen for bin code file support		
	PUINT8 filename = "/usr/local/3DSP/usb/3dspcode.bin";
	struct file * filp = NULL;
	UINT32				BinCodeLen;
	PUINT8				pBinDataBuf= NULL;			
	PSP20_BIN_CODE pSp20Bincode;
	PSP20_BIN_CODE pSp20BincodeBak;
	PSP20_FILE_HEAD_T pSp20_file_head;
	UINT8 FlagFindDspCode;
	PSP20_SUPPORT_CODETABLE pSupportCodeTable;
	PSP20_BIN_CODE_TABLE pCodeTable;
	UINT32 nCodeType;
	UINT32 tmpData;
	UINT32 nCodeIndex;
#else
	PSP20_CODE pSp20code = NULL;
#endif //  @20080928 by glen

	PUINT8 Buffer = NULL;
	UINT32 Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
	UINT32 RealLength;
	LARGE_INTEGER TimeOut;
	UINT32 offset;
	UINT32 DwordOffset;
       UINT32 CodeLen, AddrCode;
	UINT8 file_id;
	UINT32  CodeSize = 0;
	PUINT8 CodeHead = NULL;
	UINT8 WriteDone, FunctionOK = FALSE;
	UINT16 Loop;
	PUINT16 psubSystemID;
	UINT16 BasePortStart = 0;
	UINT16 BasePortEnd = 3;
	UINT32 i;

	__try
	{
		Buffer = ExAllocatePool(DSP_FIRMWARE_FRAGMENT_LENGTH, GFP_KERNEL);
		if (Buffer == NULL)
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate buffer for download DSP firmware failure\n");
			__leave;
		}
		SetHotKeyenable(devExt);

		// 20081222 add by glen for support subsystemID extend defined by EEPROM data
		if(devExt->LoadEepromFlag)
		{
			BasePortStart=0*8;
			BasePortEnd=BasePortStart+3;			
		}
		else
		{
			BT_DBGEXT(ZONE_DLD | LEVEL1, "Get EEPROM Buffer\n");
			UsbReadFromProgramRegs(devExt, 0x00, 1024/4, devExt->BufferEeprom);
			BtPrintBuffer(devExt->BufferEeprom, 1024);
			psubSystemID = (UINT16 *)&devExt->BufferEeprom[EEPROM_OFFSET_SUBSYSTEMID];
			BT_DBGEXT(ZONE_DLD | LEVEL1, "Download Dsp subsystemID: %04X", *psubSystemID);
			if(((*psubSystemID)&0xFF00)>=0x0100)
			{
				BasePortStart=(((*psubSystemID&0xFF00)>>8)-1)*8;
				BasePortEnd=BasePortStart+3;				
			}
			else
			{
				BasePortStart=0*8;
				BasePortEnd=BasePortStart+3;			
			}
		}
		// 20081020 add by glen end
		
		ntStatus =Vcmd_CardBusNICEnable(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed enable card bus NIC\n");
			__leave;
		}

#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE // @20080928 by glen for bin code file support	
		BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad DSP code from bin file\n");
		pBinDataBuf = vmalloc(BIN_CODE_HEADSZIE);
		if (pBinDataBuf == NULL)
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step1\n");
			__leave;
		}
		RtlZeroMemory(pBinDataBuf, BIN_CODE_HEADSZIE);	
		filp = filp_open(filename, O_RDONLY, 0);
		if (filp == NULL || IS_ERR((struct file *)filp))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0,  "Failed to open bincode file:%s!\n", filename );
			filp = NULL;
			__leave;
    	}
/*
    		if (( !filp->f_op ) || ( !filp->f_op->read ))
    		{
        		BT_DBGEXT(ZONE_DLD | LEVEL0,  "filp->f_op or file->f_op->read is null!\n" );
        		filp_close( filp, NULL );
        		filp = NULL;
			__leave;
    		}
*/	
		Vcmd_CardBusNICReset(devExt);
		
		Length = BIN_CODE_HEADSZIE;
		if(BIN_CODE_HEADSZIE != sc_readfile(filp, pBinDataBuf, Length, 0))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
			__leave;
		}


		ntStatus =Vcmd_CardBusNICEnable(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed enable card bus NIC\n");
			__leave;
		}


		if(devExt->LoadEepromFlag)
		{
		
			pSp20_file_head = (PSP20_FILE_HEAD_T )pBinDataBuf;
			offset = pSp20_file_head->eeprom_offset;
			BinCodeLen = pSp20_file_head->eeprom_len; 	
			if(pBinDataBuf)
				vfree(pBinDataBuf);
	//		pBinDataBuf = sc_memory_alloc(BinCodeLen);	
			pBinDataBuf = vmalloc(BinCodeLen);
			if (pBinDataBuf == NULL)
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step2\n");
				__leave;
			}	
			RtlZeroMemory(pBinDataBuf, BinCodeLen);
			Length = BinCodeLen;
			if(Length != sc_readfile(filp, pBinDataBuf, Length, offset))
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
				__leave;
			}
			
			pSp20Bincode = (PSP20_BIN_CODE)pBinDataBuf;
			filp_close(filp, NULL);
			filp = NULL;

		}
		else
		{

			// get offset and length
			pSp20_file_head = (PSP20_FILE_HEAD_T )pBinDataBuf;
			// 090702 glen added for extend card type support
			BT_DBGEXT(ZONE_DLD | LEVEL0, "EXTEND DSP CODE MARK: %X\n", pSp20_file_head->reserve1[0]);
			if(pSp20_file_head->reserve1[0]==0x30545845) // "EXT0"
			{
				offset = 0;
				BinCodeLen = 0; 	
				// Support Code Table	            
				offset = pSp20_file_head->reserve1[1];
				BinCodeLen = 2*64*1024+16;	
				if(pBinDataBuf)
					vfree(pBinDataBuf);
				pBinDataBuf = vmalloc(BinCodeLen);
				if (pBinDataBuf == NULL)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step2\n");
					__leave;
				}	//Open file
				Length = BinCodeLen;
				if(sc_readfile(filp, pBinDataBuf, Length, offset) <= 0)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
					__leave;
				}

				pSupportCodeTable = (PSP20_SUPPORT_CODETABLE)pBinDataBuf;
				if(pSupportCodeTable->m_Mark==0x4C424154)
				{
					offset = pSupportCodeTable->Size*4+8; // 
					nCodeType = *(PUINT32)&pBinDataBuf[offset+4]; //     
					pCodeTable = (PSP20_BIN_CODE_TABLE)&pBinDataBuf[offset+8];
					tmpData = ((*psubSystemID)<<16)|2;
					nCodeIndex = 0;
					for(i=0; i<nCodeType; i++)
					{
						if(pCodeTable[i].m_dwID==tmpData)
						{
							nCodeIndex = i;
						}
					}
					BT_DBGEXT(ZONE_DLD | LEVEL0, "==== Support Card Types = %d ====\n", pSupportCodeTable->Size);
					BT_DBGEXT(ZONE_DLD | LEVEL0, "   SubSysID     WlOnly     BTOnly    Combo \n");
					for(i=0; i<pSupportCodeTable->Size; i++)
					{
						switch((pSupportCodeTable->m_dwID[i]&0x0000FFFF))
						{
						case 1:
							BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      Y          -         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
							break;
	                        		case 2:
	                           		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      -          Y         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 3:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      Y          Y         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 4:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      -          -         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 5:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      Y          -         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 6:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      -          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 7:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      Y          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		case 0x0F:
	                            		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      Y          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                        		default:
	                           		BT_DBGEXT(ZONE_DLD | LEVEL0, "CardID_%04X      -          -         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
	                            		break;
	                    		}
	                		}           
	                		BT_DBGEXT(ZONE_DLD | LEVEL0, "==== Support Card Types End ====\n");
	                		if(pCodeTable[nCodeIndex].m_dwID==tmpData)
	                		{
	                    		BT_DBGEXT(ZONE_DLD | LEVEL0, "  Download CardID %04X CodeID:%08X \n", (pCodeTable[nCodeIndex].m_dwID>>16), pCodeTable[nCodeIndex].m_dwID);
	                		}
	                		else
	                		{
	                    		BT_DBGEXT(ZONE_DLD | LEVEL0, "****** not support CardID %04X , so Download default CardID %04X CodeID:%08X \n", (*psubSystemID), (pCodeTable[nCodeIndex].m_dwID>>16), pCodeTable[nCodeIndex].m_dwID);
	                		}
	                		BT_DBGEXT(ZONE_DLD | LEVEL0, "******Download Code : Offset=%X  Len=%X PortBase=%d \n", pCodeTable[nCodeIndex].m_dwOffset, pCodeTable[nCodeIndex].m_dwLen, pCodeTable[nCodeIndex].m_dwPortBase);
					// Support Code Table				
					BasePortStart = (UINT16)pCodeTable[nCodeIndex].m_dwPortBase;
					BasePortEnd=BasePortStart+3;			
					offset = pCodeTable[nCodeIndex].m_dwOffset;
	                		BinCodeLen = pCodeTable[nCodeIndex].m_dwLen;
					if(pBinDataBuf)
						vfree(pBinDataBuf);
					pBinDataBuf = vmalloc(BinCodeLen);
					if (pBinDataBuf == NULL)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step2\n");
						__leave;
					}	//Open file
					Length = BinCodeLen;
					if(Length != sc_readfile(filp, pBinDataBuf, Length, offset))
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
						__leave;
					}				
					pSp20Bincode = (PSP20_BIN_CODE)pBinDataBuf;
	            }
				else
				{
					ntStatus = STATUS_DEVICE_DATA_ERROR;					
					BT_DBGEXT(ZONE_DLD | LEVEL0, "BT Dsp_Initialize fail for bin file format error:step9\n");
					__leave;
				}
	        	}
			else
			// 090702 glen added for extend card type support end
			{
				offset = pSp20_file_head->btonly_offset;
				BinCodeLen = pSp20_file_head->btonly_len;	
				if(pBinDataBuf)
					vfree(pBinDataBuf);
				// Get BT_ONLY_DSP_CODE Data	
				pBinDataBuf = vmalloc(BinCodeLen);
				if (pBinDataBuf == NULL)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Allocate pBinDataBuf for download DSP firmware failure:step2\n");
					__leave;
				}	//Open file
				Length = BinCodeLen;
				if(Length != sc_readfile(filp, pBinDataBuf, Length, offset))
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "Failed to read BIN_CODE_HEAD!\n");
					__leave;
				}
				pSp20Bincode = (PSP20_BIN_CODE)pBinDataBuf;
				filp_close(filp, NULL);
				filp = NULL;	
			}
			//pSp20code = (PSP20_CODE)Usb_SP20CodeBlueToothOnly;
		}

		pSp20BincodeBak = pSp20Bincode;
		FlagFindDspCode = 0;
		AddrCode = 0;
		while(AddrCode<BinCodeLen)
		{
			CodeSize = pSp20Bincode->Size;
			CodeHead = pSp20Bincode->Code;
			if((pSp20Bincode->Port>=BasePortStart)&&(pSp20Bincode->Port<=BasePortEnd))
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Find DSP Code Port %d", pSp20Bincode->Port);
				FlagFindDspCode = 1;
				break;
			}			
			else
			{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X skip\n", pSp20Bincode->Port, pSp20Bincode->Size);
			}
			pSp20Bincode = (PSP20_BIN_CODE)((PUINT8)pSp20Bincode+CodeSize+sizeof(pSp20Bincode->Size)+sizeof(pSp20Bincode->Port));			
			AddrCode += CodeSize+sizeof(pSp20Bincode->Size)+sizeof(pSp20Bincode->Port);
		}
		if(FlagFindDspCode==0)
		{		
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Can not find DSP Code, So we download default Dongle DSP code");
			BasePortStart=0*8;
			BasePortEnd=BasePortStart+3;		
			AddrCode = 0;
			pSp20Bincode = pSp20BincodeBak;
		}

		while(AddrCode<BinCodeLen)
		{
			CodeSize = pSp20Bincode->Size;
			CodeHead = pSp20Bincode->Code;
			if((pSp20Bincode->Port>=BasePortStart)&&(pSp20Bincode->Port<=BasePortEnd))
			{
				Length = DSP_FIRMWARE_FRAGMENT_LENGTH;		
				
				offset = 0;
				DwordOffset = 0;
				file_id = (UINT8)pSp20Bincode->Port;
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X DSP code\n", pSp20Bincode->Port, pSp20Bincode->Size);
				do
				{
					RealLength = 0;
					DwordOffset = offset;
					RtlZeroMemory(Buffer, DSP_FIRMWARE_FRAGMENT_LENGTH);
					if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
					{
						Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
					}
					else
					{
						Length = CodeSize - offset;
					}
					if(Length == 0)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Null body, skip it\n");
						continue;
					}
					RtlCopyMemory(Buffer,CodeHead+offset,Length);
					/*if dest is B mem, then the space between 4kb and 8kb should be skipped, because the type of this space is ROM*/
					if(file_id == (2+BasePortStart))
					{
						if ( (offset >= V4_BMEM_ROM_START) && (offset < (V4_BMEM_ROM_START + V4_BMEM_ROM_LEN)) ) 
						{					
							BT_DBGEXT(ZONE_DLD | LEVEL0, "skip offset of BMem, offset = %x\n", offset);
							offset+= Length;
									continue;
						}
					}
				
					
					DownSetDma(devExt, (UINT16)DOWNLOAD_DSP_FW_FIELD_OFFSET >> 2, DMA_APPL_SCRATCH_MEM,  (UINT16)Length);
					DownBulkOutStart(devExt);
					ntStatus = BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE);
					if ((ntStatus == STATUS_SUCCESS))
					{
						TimeOut.QuadPart =  HZ*10;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
                    	atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
						if(ntStatus == 0)
						{
							BT_DBGEXT(ZONE_DLD | LEVEL1, "Dsp fw download timeout\n");
						}
				
					}
				
					WriteDone = 0;
					Loop = 1000;
					while (Loop--)
					{
						VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
						if ((WriteDone &0x10) == 0x10)
						{
							continue;
						}
						else
						{
							VendorCmdRegWrite(devExt, 0x08, 0x20);
							break;
						}
					}
					ntStatus = Vcmd_Transmit_DSPFw_Head(devExt, (UINT16)DwordOffset, Length, file_id-BasePortStart);
					if (!NT_SUCCESS(ntStatus))
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Transmit DSP firmware head error\n");
						__leave;
					}
					offset += Length;
					
				}while (CodeSize != offset);
			}
			else if(pSp20Bincode->Port==(4+BasePortStart))
			{
				Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
				
				
				offset = 0;
				DwordOffset = 0;
				file_id = (UINT8)pSp20Bincode->Port;
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X mini DSP code\n", pSp20Bincode->Port, pSp20Bincode->Size);
				do
				{
					RealLength = 0;
					DwordOffset = offset;
					RtlZeroMemory(Buffer, DSP_FIRMWARE_FRAGMENT_LENGTH);
					if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
					{
						Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
					}
					else
					{
						Length = CodeSize - offset;
					}
					if(Length == 0)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Null body, skip it\n");
						continue;
					}
					if((Length&0x03)!=0)
					{
						Length += (4-(Length&0x03));
					}
					RtlCopyMemory(Buffer,CodeHead+offset,Length);

					DownSetDma(devExt, (UINT16)((UINT16)DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET+DwordOffset) >> 2, DMA_LOCAL_PROG_MEM,  (UINT16)Length);
					DownBulkOutStart(devExt);
					ntStatus = BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE);
					if ((ntStatus == STATUS_SUCCESS))
					{
						TimeOut.QuadPart =  HZ*10;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
                    	atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
						if(ntStatus == 0)
						{
							BT_DBGEXT(ZONE_DLD | LEVEL1, "Dsp fw download timeout\n");
						}
					}
				
					WriteDone = 0;
					Loop = 1000;
					while (Loop--)
					{
						VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
						if ((WriteDone &0x10) == 0x10)
						{
							continue;
						}
						else
						{
							VendorCmdRegWrite(devExt, 0x08, 0x20);
							break;
						}
					}
					offset += Length;
					
				}while (CodeSize != offset);
			}			
			else if(pSp20Bincode->Port==(5+BasePortStart))
			{
				offset = 0;
				DwordOffset = 0;
				file_id = (UINT8)pSp20Bincode->Port;
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X BT DSP initial parameter\n", pSp20Bincode->Port, pSp20Bincode->Size);
				do
				{
					RealLength = 0;
					DwordOffset = offset;
					RtlZeroMemory(Buffer, DSP_FIRMWARE_FRAGMENT_LENGTH);
					if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
					{
						Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
					}
					else
					{
						Length = CodeSize - offset;
					}
					if(Length == 0)
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Null body, skip it\n");
						continue;
					}
					if((Length&0x03)!=0)
					{
						Length += (4-(Length&0x03));
					}
					RtlCopyMemory(Buffer,CodeHead+offset,Length);

					DownSetDma(devExt, (UINT16)((UINT16)DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET+DwordOffset) >> 2, DMA_LOCAL_PROG_MEM,  (UINT16)Length);
					DownBulkOutStart(devExt);
					ntStatus = BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE);
					if ((ntStatus == STATUS_SUCCESS))
					{
						TimeOut.QuadPart =  HZ*10;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
                    	atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
						if(ntStatus == 0)
						{
							BT_DBGEXT(ZONE_DLD | LEVEL1, "Dsp fw download timeout\n");
						}
					}
				
					WriteDone = 0;
					Loop = 1000;
					while (Loop--)
					{
						VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
						if ((WriteDone &0x10) == 0x10)
						{
							continue;
						}
						else
						{
							VendorCmdRegWrite(devExt, 0x08, 0x20);
							break;
						}
					}

					offset += Length;
					
				}while (CodeSize != offset);				
				BtDelay(200);
				VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_BT_PARAMETER_READY);
				BT_DBGEXT(ZONE_DLD | LEVEL0, "BT_PARAMETER_READY_0 \n");
				BtDelay(2000);
			}
			else{
				BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X skip\n", pSp20Bincode->Port, pSp20Bincode->Size);
			}
			pSp20Bincode = (PSP20_BIN_CODE)((PUINT8)pSp20Bincode+CodeSize+sizeof(pSp20Bincode->Size)+sizeof(pSp20Bincode->Port));			
			AddrCode += CodeSize+sizeof(pSp20Bincode->Size)+sizeof(pSp20Bincode->Port);
		}
#else
		pSp20code = (PSP20_CODE)Usb_SP20CodeBlueToothOnly;
		CodeLen = sizeof(Usb_SP20CodeBlueToothOnly);
		AddrCode = 0;
		while(AddrCode<CodeLen)
		{
			CodeSize = pSp20code->Size;
			CodeHead = pSp20code->Code;
            
			if((pSp20code->Port >= BasePortStart)
                			&&(pSp20code->Port <= BasePortEnd))
			{
				Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
			
				offset = 0;
				DwordOffset = 0;
				file_id = pSp20code->Port;
				BT_DBGEXT(ZONE_DLD | LEVEL3, "Download DSP: Port %-4d size 0x%X DSP code\n", pSp20code->Port, pSp20code->Size);
				do
				{
					RealLength = 0;
					DwordOffset = offset;
					RtlZeroMemory(Buffer, DSP_FIRMWARE_FRAGMENT_LENGTH);
					if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
					{
						Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
					}
					else
					{
						Length = CodeSize - offset;
					}
					RtlCopyMemory(Buffer,CodeHead+offset,Length);
					/*if dest is B mem, then the space between 4kb and 8kb should be skipped, because the type of this space is ROM*/
    				if(file_id == (2+BasePortStart))
					{
						if ( (offset >= V4_BMEM_ROM_START) && (offset < (V4_BMEM_ROM_START + V4_BMEM_ROM_LEN)) ) 
						{	                
							BT_DBGEXT(ZONE_DLD | LEVEL3, "skip offset of BMem, offset = 0x%lx\n", offset);
							offset+= Length;
				                	continue;
						}
					}

					DownSetDma(devExt, (UINT16)DOWNLOAD_DSP_FW_FIELD_OFFSET >> 2, DMA_APPL_SCRATCH_MEM,  (UINT16)Length);
					DownBulkOutStart(devExt);

                	ntStatus = BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE);
					if ((ntStatus == STATUS_SUCCESS))
					{
						TimeOut.QuadPart =  HZ*10;
						ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, TimeOut.QuadPart);
                    	atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
						if(ntStatus == 0)
						{
							BT_DBGEXT(ZONE_DLD | LEVEL1, "Dsp fw download timeout\n");
						}
	
					}

					WriteDone = 0;
					Loop = 1000;
					while (Loop--)
					{
						VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
						if ((WriteDone &0x10) == 0x10)
						{
							continue;
						}
						else
						{
							VendorCmdRegWrite(devExt, 0x08, 0x20);
							break;
						}
					}
					ntStatus = Vcmd_Transmit_DSPFw_Head(devExt, (UINT16)DwordOffset, Length, file_id);
					if (!NT_SUCCESS(ntStatus))
					{
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Transmit DSP firmware head error\n");
						__leave;
					}
					offset += Length;
				}while (CodeSize != offset);
            }
            else if(pSp20code->Port ==(4+BasePortStart))
            {
    			Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
    			offset = 0;
    			DwordOffset = 0;
    			file_id = pSp20code->Port;
    			BT_DBGEXT(ZONE_DLD | LEVEL3, "Download DSP: Port %-4d size 0x%X mini DSP code\n", pSp20code->Port, (UINT32)pSp20code->Size);
    			do
    			{
    				RealLength = 0;
    				DwordOffset = offset;
    				RtlZeroMemory(Buffer, DSP_FIRMWARE_FRAGMENT_LENGTH);
    				if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
    				{
    					Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
    				}
    				else
    				{
    					Length = CodeSize - offset;
    				}
    				if(Length == 0)
    				{
    					BT_DBGEXT(ZONE_DLD | LEVEL4, "Null body, skip it\n");
    					continue;
    				}
    				if((Length&0x03)!=0)
    				{
    					Length += (4-(Length&0x03));
    				}
    				RtlCopyMemory(Buffer, CodeHead+offset, Length);
    				DownSetDma(devExt, ((UINT16)DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET+DwordOffset) >> 2, DMA_LOCAL_PROG_MEM,  (UINT16)Length);
    				DownBulkOutStart(devExt);

                    if(STATUS_SUCCESS != BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE)){
                        BT_DBGEXT(ZONE_DLD | LEVEL0, "URB write error\n");
                    }
                    else{
                        //BT_DBGEXT(ZONE_DLD | LEVEL3, "BTUSB Write to Firmware %lu\n", jiffies);
    					ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, 10*HZ);
                        atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
                       	if(0 == ntStatus){
							BT_DBGEXT(ZONE_DLD | LEVEL0, "Dsp firmware download TIMEOUT\n");
						}
        			}
    				//now we must wait for the usb transfer finish,do check here
    				WriteDone = 0;
    				Loop = 1000;
    				while (Loop--)
    				{
    					VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
    					if ((WriteDone &0x10) == 0x10)
    					{
    						continue;
    					}
    					else
    					{
    						VendorCmdRegWrite(devExt, 0x08, 0x20);
    						break;
    					}
    				}
    				offset += Length;
    				
    			}while (CodeSize != offset);
    		}
            else if(pSp20code->Port==(5+BasePortStart)){
    			offset = 0;
    			DwordOffset = 0;
    			file_id = pSp20code->Port;
    			BT_DBGEXT(ZONE_DLD | LEVEL3, "Download DSP: Port %-4d size 0x%X BT DSP initial parameter\n", pSp20code->Port, (UINT32)pSp20code->Size);
    			do
    			{
    				//read data from file
    				RealLength = 0;
    				DwordOffset = offset;
    				memset(Buffer, 0, DSP_FIRMWARE_FRAGMENT_LENGTH);
    				if(CodeSize - offset >= DSP_FIRMWARE_FRAGMENT_LENGTH)
    				{
    					Length = DSP_FIRMWARE_FRAGMENT_LENGTH;
    				}
    				else
    				{
    					Length = CodeSize - offset;
    				}
    				if(Length == 0)
    				{
    					BT_DBGEXT(ZONE_DLD | LEVEL0, "Null body, skip it\n");
    					continue;
    				}
    				if((Length&0x03)!=0)
    				{
    					Length += (4-(Length&0x03));
    				}
    				memcpy(Buffer, CodeHead+offset, Length);
    	
    				//set Dma
    				DownSetDma(devExt, ((UINT16)DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET+DwordOffset) >> 2, DMA_LOCAL_PROG_MEM,  (UINT16)Length);
    				DownBulkOutStart(devExt);

				if(BtUsbWrite(devExt, Buffer, Length, MAILBOX_DATA_TYPE_FIRMWARE)){
					BT_DBGEXT(ZONE_DLD | LEVEL0, "URB write error\n");
				}
				else{
                        //BT_DBGEXT(ZONE_DLD | LEVEL3, "BTUSB Write to Firmware %lu\n", jiffies);
    					ntStatus = wait_event_interruptible_timeout(devExt->BulkOutEvent, atomic_read(&devExt->BulkOutFlag) == EVENT_SIGNED, 10*HZ);
                        atomic_set(&devExt->BulkOutFlag, EVENT_UNSIGNED);
					if(0 == ntStatus){
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Dsp firmware download TIMEOUT\n");
					}
        			}
    			
    				//now we must wait for the usb transfer finish,do check here
    				WriteDone = 0;
    				Loop = 1000;
    				while (Loop--)
    				{
    					VendorCmdRegRead(devExt, REG_BOCR6, &WriteDone);
    					if ((WriteDone &0x10) == 0x10)
    					{
    						continue;
    					}
    					else
    					{
    						VendorCmdRegWrite(devExt, 0x08, 0x20);
    						break;
    					}
    				}
    				offset += Length;
    			}while (CodeSize != offset);
                
    			//notify 8051 BlueTooth DSP init parameter ready
    			BtDelay(200);
    			VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_BT_PARAMETER_READY);
		}
		else{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "Download DSP: Port %-4d size 0x%X  skip\n", pSp20code->Port, pSp20code->Size);
		}
		pSp20code = (PSP20_CODE)((PUINT8)pSp20code+CodeSize+sizeof(pSp20code->Size)+sizeof(pSp20code->Port));			
		AddrCode += CodeSize+sizeof(pSp20code->Size)+sizeof(pSp20code->Port);
            
		}	
#endif

		ntStatus = StartDspFirmware(devExt);
		if(!NT_SUCCESS(ntStatus))
		{
			BT_DBGEXT(ZONE_DLD | LEVEL0, "start dsp firmware failure\n");
			__leave;
		}
		
		BtDelay(1000);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_GET_8051_VERSION);
		
		BtDelay(1000);
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_GET_DSP_VERSION);

		if(GotoMainLoop)
		{
			Loop = 5;
			while(--Loop)
			{
				ntStatus = VendorCmdWriteCmdToMailBox(devExt, NULL, VCMD_API_8051_JUMP_MAIN_PROCESS);
				if(ntStatus != STATUS_SUCCESS)
				{
					BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad3DspFirmWare---vcmd to jump mainloop failed\n");
					__leave;
				}
				else	
				{
					ntStatus = wait_event_interruptible_timeout(devExt->MainLoopEvent, atomic_read(&devExt->mainLoopFlag) == EVENT_SIGNED, 3*HZ);
                    atomic_set(&devExt->mainLoopFlag, EVENT_UNSIGNED);
					if(0 == ntStatus){
						BT_DBGEXT(ZONE_DLD | LEVEL0, "Dsp firmware download TIMEOUT\n");
					}
					else{
						BT_DBGEXT(ZONE_DLD | LEVEL1, "DownLoad3DspFirmWare---wait MainLoop event ok, Loop count: %d\n", Loop);
						break;
					}
				}
			}
		}

		FunctionOK = TRUE;
	}
	__finally
	{
		if(Buffer)
			ExFreePool(Buffer);
#ifdef BT_DOWNLOAD_BIN_WITH_FILE_MODE
		if(pBinDataBuf)
		{
			vfree(pBinDataBuf);
			BT_DBGEXT(ZONE_DLD | LEVEL0, "release bin buffer\n");
		}
		if(filp)
		{
			filp_close(filp, NULL);
		}
#endif
	}
	if(FunctionOK == TRUE){
		
		BT_DBGEXT(ZONE_DLD | LEVEL3, "DownLoad DSP firmware ok\n");
		return STATUS_SUCCESS;	
	}else{
		BT_DBGEXT(ZONE_DLD | LEVEL0, "DownLoad DSP firmware failure\n");
		return STATUS_UNSUCCESSFUL;
	}
}



#if 1
NTSTATUS DownStopRun8051(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	UINT8 tmp;
	/*
	ntStatus = VendorCmdRegRead(
	deviceExtension,
	(UINT16)REG_UIR,//0x14,
	&ResultCode);
	BtDelay(100);
	ResultCode &= 0x7f;
	ResultCode |= BIT_BOOT8051;//0x40;
	ntStatus = VendorCmdRegWrite(
	deviceExtension,
	(UINT16)REG_UIR,//0x14,
	ResultCode);
	BtDelay(50);
	 */
	 #if 0
	ntStatus = VendorCmdRegWrite(deviceExtension, (UINT16)REG_8051CR,  //OFFSET_8051_CTL_REG,
	BIT_START8051_DEFAULT); //0x00);
	if (!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	#endif

	#if 1
	VendorCmdBurstRegRead2(deviceExtension, 0x14, &tmp, sizeof(UINT8));
	tmp |= BIT6;
	VendorCmdBurstRegWrite2(deviceExtension, 0x14, &tmp, sizeof(UINT8));
	#endif
	BtDelay(1000);
	return ntStatus;
}

NTSTATUS DownStartRun8051(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 tmp;
	BtDelay(100);
	/*
	while (Loop--)
	{
		resultcode = BIT_START8051_ASSO_BT; //0x01;
		ntStatus = VendorCmdRegWrite(deviceExtension, REG_8051CR,  //OFFSET_8051_CTL_REG,
		resultcode);
		BtDelay(100);
		resultcode = 0;
		ntStatus = VendorCmdBurstRegRead2(deviceExtension, REG_8051CR,  //OFFSET_8051_CTL_REG,
		 &resultcode, sizeof(UINT8));
		if (resultcode &BIT_START8051_ASSO_BT)
		{
			break;
		}
	}
	*/
	#if 1
	VendorCmdBurstRegRead2(deviceExtension, 0x14, &tmp, sizeof(UINT8));
	tmp &= 0xbf;
	VendorCmdBurstRegWrite2(deviceExtension, 0x14, &tmp, sizeof(UINT8));
	#endif
	return ntStatus;
}

#endif


#if 0
NTSTATUS DownStopRun8051(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 ResultCode;
	
	ntStatus = VendorCmdBurstRegRead(deviceExtension, (UINT16)REG_UIR,&ResultCode, sizeof(UINT8));

	ResultCode |= BIT_BOOT8051;
	ntStatus = VendorCmdBurstRegWrite(deviceExtension, (UINT16)REG_UIR, &ResultCode, sizeof(UINT8));
	
	BtDelay(200);
	return ntStatus;
}

NTSTATUS DownStartRun8051(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 resultcode;
	UINT8 Loop = 0xff;

	VendorCmdBurstRegRead(deviceExtension, (UINT16)REG_UIR,&resultcode, sizeof(UINT8));
	while (--Loop)
	{
		resultcode &= ~BIT_BOOT8051;
		ntStatus = VendorCmdBurstRegWrite(deviceExtension, REG_UIR, &resultcode, sizeof(UINT8));
		BtDelay(100);
		
		resultcode = 0;
		ntStatus = VendorCmdBurstRegRead(deviceExtension, REG_UIR, &resultcode, sizeof(UINT8));
		if ((resultcode &BIT_BOOT8051) == 0)
		{
			break;
		}
	}
	if(Loop == 0)
	{
		BT_DBGEXT(ZONE_DLD | LEVEL0, "start run 8051 failure\n");
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		BT_DBGEXT(ZONE_DLD | LEVEL3, "start run 8051 successful\n");
		return STATUS_SUCCESS;
	}
}
#endif










NTSTATUS DownSetDma(IN PBT_DEVICE_EXT deviceExtension, IN UINT16 addr, IN UINT8 addr_type, IN UINT16 len)
{
	NTSTATUS status;
	UINT16 offset = 0;
	Dma_addr dmaaddr;
	Dma_len dmalen;
	RtlZeroMemory(&dmaaddr, sizeof(Dma_addr));
	RtlZeroMemory(&dmalen, sizeof(Dma_len));
	dmaaddr.addr_type = addr_type; //(addr & 0xc000) >> 14;
	dmaaddr.dma2_addr = addr &0x3fff;
	dmalen.operation = 0; //only dma2
	dmalen.dma2_len = ((len + 3) &0xfffc) >> 2;
	offset = REG_BOBR6; //0x74;
	status = VendorCmdBurstRegWrite2(deviceExtension, offset, (PUINT8) &dmaaddr, sizeof(Dma_addr));
	if (status != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	status = VendorCmdBurstRegWrite2(deviceExtension, offset + 4, (PUINT8) &dmalen, sizeof(Dma_len));
	if (status != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}

NTSTATUS DownSetBulkInDma(IN PBT_DEVICE_EXT deviceExtension, IN UINT16 addr, IN UINT8 addr_type, IN UINT16 len)
{
	NTSTATUS status;
	UINT16 offset = 0;
	Dma_addr dmaaddr;
	Dma_len dmalen;
	RtlZeroMemory(&dmaaddr, sizeof(Dma_addr));
	RtlZeroMemory(&dmalen, sizeof(Dma_len));
	dmaaddr.addr_type = addr_type; //(addr & 0xc000) >> 14;
	dmaaddr.dma2_addr = addr &0x3fff;
	dmalen.operation = 0; //only dma2
	dmalen.dma2_len = ((len + 3) &0xfffc) >> 2;
	offset = REG_BIBR5; //0x64;
	status = VendorCmdBurstRegWrite2(deviceExtension, offset, (PUINT8) &dmaaddr, sizeof(Dma_addr));
	if (status != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	status = VendorCmdBurstRegWrite2(deviceExtension, offset + 4, (PUINT8) &dmalen, sizeof(Dma_len));
	if (status != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}

NTSTATUS DownClearRegister(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 Value;
	UINT32 loop = 1000;
	UINT8 tmp;
	while (--loop)
	{
		ntStatus = VendorCmdRegRead(deviceExtension, REG_BOCR6,  //0x70,
		 &Value);
		if (NT_SUCCESS(ntStatus))
		{
			if (!(Value &0x10))
			{
				break;
			}
		}
	}
	VendorCmdRegRead(deviceExtension, REG_USBINTR1 + 3,  //0x05 + 3,
	 &tmp);
	tmp &= (~0x20);
	VendorCmdRegWrite(deviceExtension, REG_USBINTR1 + 3,  //0x05 + 3,
	tmp);
	return ntStatus;
}

NTSTATUS DownBulkOutStart(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 g_yTemp;
	UINT8 g_ctl;
	UINT32 g_yTemp_fact;
	UINT16 offset = 0x05;
	BOOLEAN Dma1 = TRUE;
	BOOLEAN Dma2 = TRUE;
	UINT8 readtest[12];
	RtlZeroMemory(readtest, 12);
	g_yTemp = 0xff;
	g_ctl = 0;
	offset = REG_BOCR6; //0x70;
	g_ctl = 0;
	g_ctl = BIT_XFR_EN_SING;
	if (!Dma1)
	{
		g_ctl |= BIT_XFR_DMA1_NOTINC;
	}
	if (!Dma2)
	{
		g_ctl |= BIT_XFR_DMA2_NOTINC;
	}
	g_ctl |= BIT_XFR_READY; //SetReady
	g_yTemp_fact = (UINT32)g_ctl &0xff;
	ntStatus = VendorCmdBurstRegWrite2(deviceExtension, offset, (PUINT8) &g_yTemp_fact, sizeof(UINT32));
	if (ntStatus != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}

NTSTATUS DownBulkInStart(IN PBT_DEVICE_EXT deviceExtension)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UINT8 g_yTemp;
	UINT8 g_ctl;
	UINT32 g_yTemp_fact;
	UINT16 offset = 0x05;
	BOOLEAN Dma1 = TRUE;
	BOOLEAN Dma2 = TRUE;
	UINT8 readtest[12];
	RtlZeroMemory(readtest, 12);
	g_yTemp = 0xff;
	g_ctl = 0;
	/*ntStatus = VendorCmdRegWrite(deviceExtension,
	offset,
	g_yTemp
	);
	if(ntStatus != STATUS_SUCCESS)
	{
	return STATUS_UNSUCCESSFUL;
	}*/
	offset = REG_BICR5; //0x60;
	g_ctl = 0;
	g_ctl = BIT_XFR_EN_SING;
	if (!Dma1)
	{
		g_ctl |= BIT_XFR_DMA1_NOTINC;
	}
	if (!Dma2)
	{
		g_ctl |= BIT_XFR_DMA2_NOTINC;
	}
	g_ctl |= BIT_XFR_READY; //SetReady
	g_yTemp_fact = (UINT32)g_ctl &0xff;
	ntStatus = VendorCmdBurstRegWrite2(deviceExtension, offset, (PUINT8) &g_yTemp_fact, sizeof(UINT32));
	if (ntStatus != STATUS_SUCCESS)
	{
		return STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}





VOID BtUsb_Bus_Reset_Enable(IN PBT_DEVICE_EXT deviceExtension)
{
	UINT8 uchar;
	VendorCmdBurstRegRead2(deviceExtension, REG_USBCTL1, &uchar, sizeof(UINT8));
	uchar |= BIT6;
	VendorCmdBurstRegWrite2(deviceExtension, REG_USBCTL1, &uchar, sizeof(UINT8));
}






