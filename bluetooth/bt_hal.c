/************************************************************************
(c) 2003-05, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
 **************************************************************************
$RCSfile: bt_hal.c,v $
$Revision: 1.4 $
$Date: 2010/09/06 05:17:26 $
 **************************************************************************/
/*
 * REVISION HISTORY
 *   ...
 *
2005.9.27  Add two new functions to backup and restore blue tooth register.
BtBackupBlueToothRegs function is used to backup blue tooth
registers before driver is reset. And BtRestoreBlueToothRegs function
is used to restore blue tooth registers after driver is reset.
2005.10.14 Change the loop times from 1000 (total 100 seconds) to 100 (total 10 seconds)
when our driver can not handshake with DSP.
function BtCardBusNICStart
{
UINT32 loop = 100;
...
}
2005.12.7  We add more registers. So driver should support these registers.
function BtInitHW
{
...
...
}
function BtBackupBlueToothRegs
function BtRestoreBlueToothRegs
2005.12.15 Change the ending address of bluetooth registers.
function BtBackupBlueToothRegs
{
...
size = BT_END_REGISTER - BT_BEGIN_REGISTER + 60;
...
}
function BtRestoreBlueToothRegs
{
...
size = BT_END_REGISTER - BT_BEGIN_REGISTER + 60;
...
}
2005.12.23 We add another four BDs and one pptr and one cptr to transfer tx SCO frames. So we
should initialize it.
function BtInitHw
{
...
BtWriteToReg(devExt, BT_HOST_P_SCO_PTR_REG, BT_BD_SCO_BASE_ADDR);
BtWriteToReg(devExt, BT_DSP_C_SCO_PTR_REG, BT_BD_SCO_BASE_ADDR);
...
}
2005.12.28 Drive add another FHS packet for page scan, so the now count of total register is 20 more than the old one.
So when backup and restore registers, the size of the new registers is 20 more than the old one.
function BtBackupBlueToothRegs
{
...
size = BT_END_REGISTER - BT_BEGIN_REGISTER + 80;
...
((PUINT8)devExt->BaseRegisterAddress + BT_REG_LOCAL_BD_ADDR, devExt->pBTRegMems, size - 80);
((PUINT8)devExt->BaseRegisterAddress + BT_REG_FHS_FOR_INQUIRY_SCAN, devExt->pBTRegMems + size - 80, 80);
...
}
function BtRestoreBlueToothRegs
{
...
size = BT_END_REGISTER - BT_BEGIN_REGISTER + 80;
...
...
}
2006.1.20 Change the function BtReadFromReg. Now driver would read register twice and driver return the register value only if these
two value is the same value.
2006.2.24 We integrate the driver codes which is based on the Bus Driver into the normal driver codes. So some code is added.
function BtEnableInterrupt
{
#ifdef BT_INTERFACE_BUS_DRIVER
Bus_SubmitIrpForIntEvent(devExt);
#else
...
}
function BtDisableInterrupt
{
#ifdef BT_INTERFACE_BUS_DRIVER
Bus_NotifyCompleteIntIrp(devExt);
#else
...
}
We add some code for accessing scratch pad through DMA0
#ifdef BT_ACCESS_SCRATCH_BY_DMA0
...
#endif
2006.3.14  Add a condition: if DSP codes is downloaded by Bus Driver, blue tooth driver
should delete such codes that be used to download DSP code.
function BtNICReloadSP20Code
{
...
#ifndef BT_INTERFACE_BUS_DRIVER
...
#endif
...
}
2006.3.28 We change the way reading and writing registers.
2006.4.7 Some blue tooth register should be initialized before DSP codes is reset.
Add a new function:BtInitBlueToothRegs
function BtInitHw
{
...
BtInitBlueToothRegs(devExt);
}
 */
#include "bt_sw.h"        // include <WDM.H> and data structure for us
#include "bt_dbg.h"        // include debug function
#include "bt_hal.h"        // include accessing hardware resources function
#include "bt_hci.h"
#include "bt_pr.h"        // include most functions of declaration for us
#include "bt_lmp.h"
#ifdef BT_SERIALIZE_DRIVER
#include "bt_serialize.h"
#endif

#ifdef BT_TESTDRIVER
#endif


BOOLEAN BtIsPluggedIn(PBT_DEVICE_EXT devExt)
{
	if((devExt->DriverHaltFlag == 1) || (devExt->State == STATE_SURPRISE_REMOVED) ||
		(devExt->State == STATE_REMOVED) || (devExt->State == STATE_STOPPED))
	{
		return FALSE;
	}
	else
		return TRUE;

	
/*
	if (!devExt->AdapterUnplugged)
	{
		if(MAC_SIGNATURE_VALUE != BT_HAL_READ_DWORD((PUINT32)((PUINT8)devExt->BaseRegisterAddress + WLS_MAC_SIGNATURE_WD)))
		{
		#ifdef BT_SERIALIZE_DRIVER
			Serialize_ProcessUnPlug(devExt);
		#endif
		}
	}
	return (!devExt->AdapterUnplugged);
*/
}
VOID BtEnableInterrupt(PBT_DEVICE_EXT devExt)
{
	BT_DBGEXT(ZONE_HAL | LEVEL2, "BtEnableInterrupt: enter\n");
	BtWriteToReg(devExt, BT_CSR_STATUS_CLR_REG, BT_INT_STATUS_MASK_BITS);
	BtWriteToReg(devExt, BT_CSR_FUNC_EVENT_REG, BT_PC_CARD_STATUS_INTR_BIT);
	BtWriteToReg(devExt, BT_CSR_FUNC_EVENT_MASK_REG, BT_PC_CARD_STATUS_INTR_BIT);
	BtWriteToReg(devExt, BT_CSR_INT_ENABLE_REG, BT_INT_STATUS_MASK_BITS);
}
VOID BtDisableInterrupt(PBT_DEVICE_EXT devExt)
{
	BT_DBGEXT(ZONE_HAL | LEVEL2, "BtDisableInterrupt: enter\n");
	UsbWriteTo3DspReg(devExt, BT_CSR_FUNC_EVENT_MASK_REG, 0);
	
	UsbWriteTo3DspReg(devExt, BT_CSR_INT_ENABLE_REG, 0);
}
UINT32 BtReadDSPInt(PBT_DEVICE_EXT devExt)
{
	UINT32 tmplong;
	BT_DBGEXT(ZONE_HAL | LEVEL2, "BtReadDSPInt: enter\n");
	UsbReadFrom3DspReg(devExt, BT_DSP_INT_STATUS_REG, &tmplong);
	return tmplong;
}
VOID BtWriteDSPInt(PBT_DEVICE_EXT devExt, UINT32 value)
{
	UsbWriteTo3DspReg(devExt, BT_DSP_INT_STATUS_REG, value);
}
UINT32 BtReadFromReg(PBT_DEVICE_EXT devExt, UINT32 offset)
{

	
	UINT32 tmplong;
	tmplong = Hci_Read_DWord_From_3DspReg(devExt, offset);
		
	return (tmplong);
}
VOID BtWriteToReg(PBT_DEVICE_EXT devExt, UINT32 offset, UINT32 value)
{

		UsbWriteTo3DspReg(devExt, offset, value);
		

}
 VOID BtDelay(UINT32 miliseconds)
{
	if(in_atomic() == 0)
	{
		udelay(miliseconds);
	}
	else
	{
		msleep(miliseconds/1000);
	}
	/*
	if(in_atomic() == 0)
	{
		udelay(miliseconds);

	}
	else
	{
		msleep(miliseconds/1000);
	}
	*/
	
}
VOID BtCardBusNICEnable(PBT_DEVICE_EXT devExt)
{
	UINT32 state = BT_PCI_CONTROL_RESET_WLAN_CORE | BT_PCI_CONTROL_MAC_HOST_DMA0_ENABLE | BT_PCI_CONTROL_MAC_HOST_DMA1_ENABLE | BT_PCI_CONTROL_DMA0_ENABLE | BT_PCI_CONTROL_DMA1_ENABLE;
	BT_CARDBUS_WRITE_CONTROL_REG(devExt, state);
}
VOID BtCardBusNICReset(PBT_DEVICE_EXT devExt)
{
	UINT32 state = BT_CARDBUS_READ_CONTROL_REG(devExt);
	state |= BT_PCI_CONTROL_SOFT_RESET_PCI;
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, state);
	state = BT_PCI_CONTROL_RESET_WLAN_CORE | BT_PCI_CONTROL_RESET_WLAN_SUB_SYS | BT_PCI_CONTROL_RESET_WLAN_SYS;
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, state);
	state = BT_PCI_CONTROL_RESET_WLAN_CORE | BT_PCI_CONTROL_SLEEP_WLAN_CORE | BT_PCI_CONTROL_SLEEP_WLAN_SUB_SYS | BT_PCI_CONTROL_SLEEP_WLAN_SYS | BT_PCI_CONTROL_SLEEP_MAC_GATED | BT_PCI_CONTROL_SLEEP_MAC | BT_PCI_CONTROL_SLEEP_DBG;
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, state);
}
VOID BtCardBusNICStart(PBT_DEVICE_EXT devExt)
{
	UINT32 loop = 100;
	UINT32 lmp = 0;
	UINT32 state = BT_PCI_CONTROL_MAC_HOST_DMA0_ENABLE | BT_PCI_CONTROL_MAC_HOST_DMA1_ENABLE | BT_PCI_CONTROL_DMA0_ENABLE | BT_PCI_CONTROL_DMA1_ENABLE;
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, state);
	while (--loop)
	{
		UsbReadFrom3DspReg(devExt, BT_SP20_READY, &lmp);
		if (lmp == BT_MMAC_CORE_RDY)
		{
			BT_DBGEXT(ZONE_HAL | LEVEL3, "sp20 ready!\n");
			break;
		}
		BT_DBGEXT(ZONE_HAL | LEVEL1, "Sp20 not ready, driver will check again. times = %d!\n", 100-loop);
		BtDelay(100); /* wait */
	}
}
NTSTATUS BtNICReloadSP20Code(PBT_DEVICE_EXT devExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	return Status;
}
NTSTATUS BtScratch_2_DSP_MemoryBank(PBT_DEVICE_EXT devExt, VOID *Buffer, UINT32 FileLength, UINT8 memport)
{
	UINT32 dma_control_word, dma_src, dma_dst, length, dma_length, consumed, destoffset;
	UINT32 secret_steering_bit, PCIsteering_bit, waitcount;
	UINT8 SrcPortNo, DstPortNo;
	KIRQL oldIrql;
	NTSTATUS Status = STATUS_SUCCESS;
	SrcPortNo = BT_SB_PORT_CARDBUS;
	DstPortNo = BT_SB_PORT_UNIPHY;
	consumed = 0;
	length = FileLength; //bytes
	while (length)
	{
		if (length > BT_SCRATCH_XFER_LENGTH)
		{
			dma_length = BT_SCRATCH_XFER_LENGTH;
		}
		else
		{
			dma_length = length;
		}
		length -= dma_length;
		UsbWriteTo3DspRegs(devExt, BT_SCRATCH_MEM_BASE_ADDR + BT_SCRATCH_XFER_OFFSET, (UINT8)dma_length, ((PUINT8)Buffer + consumed));
		switch (memport)
		{
			case BT_SPX_P_MEM:
				secret_steering_bit = 0;
				destoffset = consumed / 4;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_ON, BT_SB_WR_STP_OFF, 0, 0, 0);
				dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | destoffset);
				break;
			case BT_SPX_A_MEM:
				secret_steering_bit = 0;
				destoffset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
				break;
			case BT_SPX_B_MEM:
				secret_steering_bit = 0x08000000;
				destoffset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
				break;
			case BT_SPX_T_MEM:
				secret_steering_bit = 0x04000000;
				destoffset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
				break;
            default:
                BT_DBGEXT(ZONE_HAL | LEVEL0, "Error memory port\n");
                return STATUS_UNSUCCESSFUL;
		}
		BtWriteToReg(devExt, BT_CSR_STATUS_CLR_REG, 0xffffffff);
		dma_src = BT_SB_ADDRESS(SrcPortNo, BT_PCI_STEERING_BIT | (BT_SCRATCH_MEM_BASE_ADDR + BT_SCRATCH_XFER_OFFSET) >> 2);
		BtWriteToReg(devExt, BT_CSR_DMA0_SRC_ADDR_REG, dma_src);
		BtWriteToReg(devExt, BT_CSR_DMA0_DST_ADDR_REG, dma_dst);
		BtWriteToReg(devExt, BT_CSR_DMA0_CONTROL, dma_control_word);
		waitcount = 0;
		while (1)
		{
			if (BT_INT_STATUS_DMA0_DONE_BIT &BtReadFromReg(devExt, BT_CSR_STATUS_REG))
			{
				break;
			}
			else
			{
				waitcount++;
				BtDelay(1);
				if (waitcount >= 1000)
				{
					Status = STATUS_UNSUCCESSFUL;
					BT_DBGEXT(ZONE_HAL | LEVEL1, "ScratchRAM to DSP Memory Transfer timed out waiting for DONE bit!!\n");
					return Status;
				}
			}
		}
		consumed += dma_length;
	}
	return Status;
}
NTSTATUS BtDSP_Memorybank_2_Scratch(PBT_DEVICE_EXT devExt, VOID *Buffer, UINT32 FileLength, UINT8 memport)
{
	UINT32 dma_control_word, dma_src, dma_dst, length, dma_length, consumed, src_offset;
	UINT32 secret_steering_bit, waitcount;
	UINT8 SrcPortNo, DstPortNo;
	KIRQL oldIrql;
	NTSTATUS Status = STATUS_SUCCESS;
	SrcPortNo = BT_SB_PORT_UNIPHY;
	DstPortNo = BT_SB_PORT_CARDBUS;
	consumed = 0;
	length = FileLength;
	while (length)
	{
		if (length > BT_SCRATCH_XFER_LENGTH)
		{
			dma_length = BT_SCRATCH_XFER_LENGTH;
		}
		else
		{
			dma_length = length;
		}
		length -= dma_length;
		switch (memport)
		{
		    default:
			case BT_SPX_P_MEM:
				Status = STATUS_INVALID_DEVICE_REQUEST;
				return Status;
				break;
			case BT_SPX_A_MEM:
				secret_steering_bit = 0;
				src_offset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				break;
			case BT_SPX_B_MEM:
				secret_steering_bit = 0x08000000;
				src_offset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				break;
			case BT_SPX_T_MEM:
				secret_steering_bit = 0x04000000;
				src_offset = consumed;
				dma_control_word = BT_SB_CONTROL(dma_length, BT_SB_PRI4, BT_SB_TS2, BT_SB_TX_NORM, BT_SB_WR_LNK_OFF, BT_SB_RD_CTL_OFF, BT_SB_RD_STP_OFF, BT_SB_WR_CTL_OFF, BT_SB_WR_STP_OFF, 0, 0, 0);
				break;
		}
		BtWriteToReg(devExt, BT_CSR_STATUS_CLR_REG, 0xffffffff);
		dma_src = BT_SB_ADDRESS(SrcPortNo, secret_steering_bit | (src_offset) >> 2);
		dma_dst = BT_SB_ADDRESS(DstPortNo, BT_PCI_STEERING_BIT | (BT_SCRATCH_MEM_BASE_ADDR + BT_SCRATCH_XFER_OFFSET) >> 2);
		BtWriteToReg(devExt, BT_CSR_DMA0_SRC_ADDR_REG, dma_src);
		BtWriteToReg(devExt, BT_CSR_DMA0_DST_ADDR_REG, dma_dst);
		BtWriteToReg(devExt, BT_CSR_DMA0_CONTROL, dma_control_word);
		waitcount = 0;
		while (1)
		{
			if (BT_INT_STATUS_DMA0_DONE_BIT &BtReadFromReg(devExt, BT_CSR_STATUS_REG))
			{
				break;
			}
			else
			{
				waitcount++;
				if (waitcount >= 1000)
				{
					Status = STATUS_UNSUCCESSFUL;
					BT_DBGEXT(ZONE_HAL | LEVEL1, "DSP Memory to ScratchRAM Transfer timed out waiting for DONE bit!!\n");
					return Status;
					break;
				}
			}
		}
		UsbReadFrom3DspRegs(devExt, (BT_SCRATCH_MEM_BASE_ADDR + BT_SCRATCH_XFER_OFFSET), (UINT8)dma_length, ((PUINT8)Buffer + consumed));
		consumed += dma_length;
	}
	return Status;
}
VOID BtSp20ResetApplied(PBT_DEVICE_EXT devExt)
{
	UINT32 tmplong = 0;
	UsbReadFrom3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, &tmplong);
	tmplong |= BT_PCI_CONTROL_SLEEP_WLAN_CORE; // Stop clock to WLAN_CORE
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
	tmplong |= BT_PCI_CONTROL_SLEEP_WLAN_SUB_SYS; // Stop clock to WLAN_SUBSYSTEM
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
	tmplong |= BT_PCI_CONTROL_RESET_WLAN_CORE; // Apply reset to WLAN_CORE
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
}
VOID BtSp20ResetRemoved(PBT_DEVICE_EXT devExt)
{
	UINT32 tmplong = 0;
	UsbReadFrom3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, &tmplong);
	tmplong &= (~(BT_PCI_CONTROL_SLEEP_WLAN_SUB_SYS)); // supply clock
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
	tmplong &= (~(BT_PCI_CONTROL_SLEEP_WLAN_CORE)); // supply clock
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
	tmplong &= (~(BT_PCI_CONTROL_RESET_WLAN_CORE)); // Relese reset
	UsbWriteTo3DspReg(devExt, BT_CSR_PCI_CONTROL_REG, tmplong);
}
NTSTATUS BtInitHw(PBT_DEVICE_EXT devExt)
{
	UINT32 Interrupt = 0;
	
	
	if(STATUS_SUCCESS != Hci_Write_DWord_To_3DspReg(devExt, BT_REG_HCI_COMMAND_INDICATOR, 0))
	{
    	BT_DBGEXT(ZONE_POWER | LEVEL0,  "BT_REG_HCI_COMMAND_INDICATOR failed\n");
	}
	
	if(STATUS_SUCCESS != UsbReadFrom3DspReg(devExt, 0x0008, &Interrupt))
	{
	    BT_DBGEXT(ZONE_POWER | LEVEL0,  "0x0008 failed\n");
	}

	
	if (Interrupt)
	{
		Interrupt |= BIT30;
	}
	if (STATUS_SUCCESS != VendorCmdWrite3DSPDword(devExt, Interrupt, 0x0008))
	{
	    BT_DBGEXT(ZONE_POWER | LEVEL0,  "0x0008 failed2\n");
	}
	if (STATUS_SUCCESS != UsbWriteTo3DspReg(devExt, BT_DSP_INT_STATUS_REG, 0))
	{
    	BT_DBGEXT(ZONE_POWER | LEVEL0,  "BT_DSP_INT_STATUS_REG failed\n");
	}
	
	if (STATUS_SUCCESS != UsbWriteTo3DspReg(devExt, BT_REG_DSP_TX_DISABLE, 0))
	{
	    BT_DBGEXT(ZONE_POWER | LEVEL0,  "BT_REG_DSP_TX_DISABLE failed\n");
	}
	
	if (STATUS_SUCCESS != BtInitBlueToothRegs(devExt))
	{
	    BT_DBGEXT(ZONE_POWER | LEVEL0,  "BtInitBlueToothRegs failed\n");
	}
	/* jakio20080523: only support v4 chip
*/
	devExt->chipID = 1;
	return STATUS_SUCCESS;
}
VOID BtBackupBlueToothRegs(PBT_DEVICE_EXT devExt)
{

	
	#if 0
	UINT16 size;
	size = BT_END_REGISTER - BT_BEGIN_REGISTER + 80;
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Backup blue tooth registers, count = %d\n", size);
	if (devExt->pBTRegMems == NULL)
	{
		devExt->pBTRegMems = (PUINT8)ExAllocatePool(size, GFP_ATOMIC);
		BT_DBGEXT(ZONE_HAL | LEVEL3, "Backup blue tooth registers, allocate new memory = 0x%x\n", devExt->pBTRegMems);
	}
	if (devExt->pBTRegMems != NULL)
	{
		UsbReadFrom3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, (size - 80) / 4, devExt->pBTRegMems);
		UsbReadFrom3DspRegs(devExt, BT_REG_FHS_FOR_INQUIRY_SCAN, 80 / 4, devExt->pBTRegMems + size - 80);
	}
	else
	{
		ASSERT(0);
		BT_DBGEXT(ZONE_HAL | LEVEL1, "Backup blue tooth registers fail for insuffcount memory\n");
	}
	#endif
}
NTSTATUS BtRestoreBlueToothRegs(PBT_DEVICE_EXT devExt)
{


	return STATUS_SUCCESS;
#if 0
	UINT16 size;
	NTSTATUS state;
	BOOLEAN	FuncitonOk = FALSE;
	PUINT8	buffer = NULL;
	size = BT_END_REGISTER - BT_BEGIN_REGISTER + 80;
	if (devExt->pBTRegMems != NULL)
	{
		BT_DBGEXT(ZONE_HAL | LEVEL3, "Restore blue tooth registers, count = %d\n", size);
		buffer = (PUINT8)ExAllocatePool(size, GFP_ATOMIC);
		if(buffer == NULL)
		{
			BT_DBGEXT(ZONE_HAL | LEVEL0, "Can't allocate buffer to verify registers\n");
		}
		__try
		{
			state = UsbWriteTo3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, (size - 80) / 4, devExt->pBTRegMems);
			if(state != STATUS_SUCCESS)
			{
				BT_DBGEXT(ZONE_HAL | LEVEL0, "Restore Bluetooth registers failed1\n");
				__leave;
			}

			if(buffer)
			{
				state = UsbReadFrom3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, (size - 80) / 4, buffer);
				if(state != STATUS_SUCCESS)
				{
					BT_DBGEXT(ZONE_HAL | LEVEL0, "Error, read registers failed\n");
				}
				else
				{
					if(RtlEqualMemory(buffer, devExt->pBTRegMems, (size - 80)))
					{
						BT_DBGEXT(ZONE_HAL | LEVEL0, "Error, verify registers failed1\n");
						BT_DBGEXT(ZONE_HAL | LEVEL1, "write reg buffer:\n");
						BtPrintBuffer(devExt->pBTRegMems, size - 80);
						BT_DBGEXT(ZONE_HAL | LEVEL1, "Read back buffer:\n");
						BtPrintBuffer(buffer, size - 80);
					}
					else
					{
						BT_DBGEXT(ZONE_HAL | LEVEL3, "Registers restore ok1\n");
					}
				}
			}
		
			
			state = UsbWriteTo3DspRegs(devExt, BT_REG_FHS_FOR_INQUIRY_SCAN, 80 / 4, devExt->pBTRegMems + size - 80);
			if(state != STATUS_SUCCESS)
			{
				BT_DBGEXT(ZONE_HAL | LEVEL0, "Restore Bluetooth registers failed2\n");
				__leave;
			}

			if(buffer)
			{
				state = UsbReadFrom3DspRegs(devExt, BT_REG_FHS_FOR_INQUIRY_SCAN, 80 / 4, buffer);
				if(state != STATUS_SUCCESS)
				{
					BT_DBGEXT(ZONE_HAL | LEVEL0, "Error, read registers failed\n");
				}
				else
				{
					if(RtlEqualMemory(buffer, (devExt->pBTRegMems + size - 80), 80))
					{
						BT_DBGEXT(ZONE_HAL | LEVEL0, "Error, verify registers failed2\n");
						BT_DBGEXT(ZONE_HAL | LEVEL1, "write reg buffer:\n");
						BtPrintBuffer(devExt->pBTRegMems + size - 80, 80);
						BT_DBGEXT(ZONE_HAL | LEVEL1, "Read back buffer:\n");
						BtPrintBuffer(buffer, 80);
					}
					else
					{
						BT_DBGEXT(ZONE_HAL | LEVEL3, "Registers restore ok2\n");
					}
				}
			}

			
			FuncitonOk = TRUE;
		}
		__finally
		{
			
			ExFreePool(devExt->pBTRegMems);
			devExt->pBTRegMems = NULL;	
			if(buffer)
			{
				ExFreePool(buffer);
			}
		}
		if(FuncitonOk)
		{
			BT_DBGEXT(ZONE_HAL | LEVEL3, "Restore blue tooth registers ok, memory is 0x%x, then delete memory\n", devExt->pBTRegMems);
			return STATUS_SUCCESS;
		}
		else
			return STATUS_UNSUCCESSFUL;
		
	}
	else
	{
		return STATUS_SUCCESS;
	}
#endif
}
NTSTATUS BtInitBlueToothRegs(PBT_DEVICE_EXT devExt)
{
	UINT32 Value = 0;
	BOOLEAN FunctionOk = FALSE;

	__try
	{
		if (STATUS_SUCCESS != Hci_Write_One_Word(devExt, BT_REG_AFH_MODE, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != UsbWriteTo3DspReg(devExt, BT_REG_ENCRYPTION_ENABLE, Value))
		{
			__leave;
		}
		if (STATUS_SUCCESS != UsbWriteTo3DspReg(devExt, BT_REG_ENCRYPTION_ENABLE + 4, Value))
		{
			__leave;
		}


		if (STATUS_SUCCESS != Hci_Write_One_Byte(devExt, BT_REG_ENCRYPTION_ENABLE + 8, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_EDR_MODE,0, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_EDR_RATE-2,2, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_TRANSMITT_POWER_CONTROL, 0,0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_TRANSMITT_POWER_CONTROL_SLAVE1-2,2, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_AM_TRANSMITT_POWER, 0,0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_Word_To_3DspReg(devExt, BT_REG_RX_POWER_SLAVE1-2,2, 0))
		{
			__leave;
		}
		if (STATUS_SUCCESS != Hci_Write_One_Byte(devExt, BT_REG_ROLE_SWITCH_AMADDR, 0))
		{
			__leave;
		}
		
		FunctionOk = TRUE;
	}
	__finally
	{
		
	}
	if(FunctionOk)
		return STATUS_SUCCESS;
	else
		return STATUS_UNSUCCESSFUL;

}


/*******************************************************************
 *   BtUsbTransmitDSPFwHead
 *
 *   Descriptions:
 *      The routine of downloading firmware .
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, the address of firmware binary stream.
 *      len: IN, the length of the firmware binary stream.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 ******************************************************************/
NTSTATUS BtTransmitDSPFwHead(PBT_DEVICE_EXT devExt, UINT16 offset, UINT32 len, UINT8 file_id)
{
	NTSTATUS status;
	DSP_FW_HEAD_T fw_head;
	UINT8 SrcPortNo, DstPortNo; /*Sourc/Destination Port */
	UINT32 secret_steering_bit; /*Secret Bit*/
	UINT32 dma_src = 0; /*DMA Source address*/
	UINT32 dma_dst = 0; /*DMA destination address*/
	UINT32 destoffset = 0; /*Destination offset*/
	UINT32 dma_control_word = 0; /*DMA control word*/
	SrcPortNo = SHUTTLEBUS__PORT_CARDBUS;
	DstPortNo = SHUTTLEBUS__PORT_UNIPHY;
	ASSERT(len <= 1024);
	switch (file_id)
	{
		case SPX_P_MEM:
			secret_steering_bit = 0;
			destoffset = (UINT32)(offset / 4); //should be consumed/4
			dma_control_word = BT_SB_CONTROL(len, SHUTTLE_PRIORITY_4, SHUTTLE_BURST_SIZE_2, SHUTTLE_MODE_TX_NORMAL, SHUTTLE_W_LINK_OFF, SHUTTLE_R_CTRL_OFF, SHUTTLE_R_STOP_OFF, SHUTTLE_W_CTRL_ON, SHUTTLE_W_STOP_OFF, SHUTTLE_HIP_OFF, 0, 0);
			dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | destoffset);
			break;
		case SPX_A_MEM:
			secret_steering_bit = 0;
			destoffset = (offset);
			dma_control_word = BT_SB_CONTROL(len, SHUTTLE_PRIORITY_4, SHUTTLE_BURST_SIZE_2, SHUTTLE_MODE_TX_NORMAL, SHUTTLE_W_LINK_OFF, SHUTTLE_R_CTRL_OFF, SHUTTLE_R_STOP_OFF, SHUTTLE_W_CTRL_ON, SHUTTLE_W_STOP_OFF, SHUTTLE_HIP_OFF, 0, 0);
			dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		case SPX_B_MEM:
			secret_steering_bit = 0x08000000;
			destoffset = (offset);
			dma_control_word = BT_SB_CONTROL(len, SHUTTLE_PRIORITY_4, SHUTTLE_BURST_SIZE_2, SHUTTLE_MODE_TX_NORMAL, SHUTTLE_W_LINK_OFF, SHUTTLE_R_CTRL_OFF, SHUTTLE_R_STOP_OFF, SHUTTLE_W_CTRL_ON, SHUTTLE_W_STOP_OFF, SHUTTLE_HIP_OFF, 0, 0);
			dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		case SPX_T_MEM:
			secret_steering_bit = 0x04000000;
			destoffset = (offset);
			dma_control_word = BT_SB_CONTROL(len, SHUTTLE_PRIORITY_4, SHUTTLE_BURST_SIZE_2, SHUTTLE_MODE_TX_NORMAL, SHUTTLE_W_LINK_OFF, SHUTTLE_R_CTRL_OFF, SHUTTLE_R_STOP_OFF, SHUTTLE_W_CTRL_ON, SHUTTLE_W_STOP_OFF, SHUTTLE_HIP_OFF, 0, 0);
			dma_dst = BT_SB_ADDRESS(DstPortNo, secret_steering_bit | (destoffset) >> 2);
			break;
		default:
			break;
	}
	dma_src = BT_SB_ADDRESS(SrcPortNo, PCI_STEERING_BIT |  \
		((UINT32)(DOWNLOAD_DSP_FW_FIELD_OFFSET + _OFFSET_SPD + SCRATCH_RAM_OFFSET)) >> 2);
	return STATUS_SUCCESS;
}


/*******************************************************************
 *   BtUsb8051FwDownLoad
 *
 *   Descriptions:
 *      The routine of downloading firmware .
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, the address of firmware binary stream.
 *      len: IN, the length of the firmware binary stream.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsb8051FwDownLoad(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT16 offset
)
{
NTSTATUS status;
status = BtUsbWriteFwCtrlWord(devExt,offset,len);
if(STATUS_SUCCESS != status)
{
BT_DBGEXT(ZONE_HAL | LEVEL3, "download firmware fail in head command \n");
return STATUS_UNSUCCESSFUL;
}
status = BtTransmitFWFragment(devExt,buf,len);
return status;
}
 */
/*******************************************************************
 *   BtUsbDspFwDownLoad
 *
 *   Descriptions:
 *      The routine of downloading firmware .
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, the address of firmware binary stream.
 *      len: IN, the length of the firmware binary stream.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbDspFwDownLoad(
PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32   len,
UINT16 offset,
UINT8 file_id)
{
NTSTATUS status;
status = BtTransmitDSPFwHead(devExt, offset, len,file_id);
if(STATUS_SUCCESS != status)
{
BT_DBGEXT(ZONE_HAL | LEVEL0, "download firmware fail in head command \n");
return STATUS_UNSUCCESSFUL;
}
status = BtTransmitFWFragment(devExt,buf,len);
return status;
}
 */
/*******************************************************************
 *   BtUsbDspFwDownloadInterface
 *
 *   Descriptions:
 *      The routine of downloading firmware .
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, the address of firmware binary stream.
 *      len: IN, the length of the firmware binary stream.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbDspFwDownloadInterface(
PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32 total_len,
UINT8 file_id)
{
UINT32 div;
UINT32 mod;
UINT32   i;
NTSTATUS Status;
PUINT8 pbuf;
div = total_len / DSP_FIRMWARE_FRAGMENT_LENGTH;
mod = total_len % DSP_FIRMWARE_FRAGMENT_LENGTH;
pbuf = buf;
for (i = 0; i < div; i++)
{
Status = BtUsbDspFwDownLoad(devExt,
pbuf + i * DSP_FIRMWARE_FRAGMENT_LENGTH,
DSP_FIRMWARE_FRAGMENT_LENGTH,
(UINT16)(i * DSP_FIRMWARE_FRAGMENT_LENGTH),
file_id);
ASSERT(STATUS_SUCCESS == Status);
}	
if(mod)
{
Status = BtUsbDspFwDownLoad(devExt,
pbuf + i * DSP_FIRMWARE_FRAGMENT_LENGTH,
mod,
(UINT16)(i * DSP_FIRMWARE_FRAGMENT_LENGTH),
file_id);
}	
BtDelay(5000);
return Status;	
}
 */
/*******************************************************************
 *   BtUsbGet8051FwCodesAndDownload
 *
 *   Descriptions:
 *      The routine of downloading firmware .
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, the address of firmware binary stream.
 *      len: IN, the length of the firmware binary stream.
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbGet8051FwCodesAndDownload(PBT_DEVICE_EXT devExt)
{
NTSTATUS Status;
UINT32 FileLength, div, mod, i;
OBJECT_ATTRIBUTES  InitiaObject;
UNICODE_STRING  ObjectName;
ANSI_STRING  AnsiString;
HANDLE  FileHandle;
SIZE_T  ViewSize;
IO_STATUS_BLOCK  IoStatus;
FILE_STANDARD_INFORMATION fileinfo;
PVOID MappedBuffer =NULL;
PM8051_CODE pCodeBuffer = (PM8051_CODE)M8051Code;
#ifdef DOWNLOAD_8051_WITH_BIN_FILE_MODE
RtlInitAnsiString(&AnsiString, "3dsp8051.bin");
RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, TRUE);	
InitializeObjectAttributes(&InitiaObject, &ObjectName, OBJ_KERNEL_HANDLE |  OBJ_CASE_INSENSITIVE, NULL, NULL);
Status= ZwOpenFile(&FileHandle, FILE_OPEN_IF | SYNCHRONIZE, &InitiaObject, &IoStatus, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
if (Status != STATUS_SUCCESS)
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return (STATUS_SUCCESS);
}
Status =  ZwQueryInformationFile(FileHandle, &IoStatus, &fileinfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
if (Status != STATUS_SUCCESS)
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return (STATUS_SUCCESS);
}
Status = ZwReadFile(FileHandle,
NULL,
NULL,
NULL,
&IoStatus,
(PVOID)MappedBuffer,
fileinfo.EndOfFile.LowPart,
NULL,
NULL
);
if (Status == STATUS_SUCCESS)
{
FileLength = fileinfo.EndOfFile.LowPart;
div = FileLength / DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
mod = FileLength % DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
for (i = 0; i < div; i++)
{
Status = BtUsb8051FwDownLoad(
devExt,
(PUINT8)((PUINT8)MappedBuffer + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH),
DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
(UINT16)(i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH));
ASSERT(STATUS_SUCCESS == Status);
}	
if(mod)
{
Status = BtUsb8051FwDownLoad(
devExt,
(PUINT8)MappedBuffer + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
(UINT16)mod,
(UINT16)(i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH));
}	
BtDelay(50000);
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return (STATUS_SUCCESS);
}
else
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return STATUS_UNSUCCESSFUL;
}
#else
ASSERT(pCodeBuffer->Size != 0);
FileLength =(UINT32)pCodeBuffer->Size;
div = FileLength / DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
mod = FileLength % DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
MappedBuffer = (PVOID)pCodeBuffer->Code;
for (i = 0; i < div; i++)
{
Status = BtUsb8051FwDownLoad(
devExt,
(PUINT8)MappedBuffer + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
(UINT16)(i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH));
ASSERT(STATUS_SUCCESS == Status);
}	
if(mod)
{
Status = BtUsb8051FwDownLoad(
devExt,
(PUINT8)MappedBuffer + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
(UINT16)mod,
(UINT16)(i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH));
}	
BtDelay(50000);
#endif
return (Status);
}
 */
/*******************************************************************
 *   BtUsbGetDspFwCodesAndDownload
 *
 *   Descriptions:
 *      This function retrieve firmware codes and pass it to bootloader.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NDIS_STATUS_SUCCESS: return success.
 *      NDIS_STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbGetDspFwCodesAndDownload(PBT_DEVICE_EXT devExt)
{
NTSTATUS Status;
HANDLE FileHandle;
OBJECT_ATTRIBUTES  InitiaObject;
UNICODE_STRING  ObjectName;
ANSI_STRING  AnsiString;
SIZE_T  ViewSize;
IO_STATUS_BLOCK  IoStatus;
FILE_STANDARD_INFORMATION fileinfo;
PVOID MappedBuffer =NULL;
int i;
UINT32 FileLength;
PSP20_CODE pCodeBuffer = (PSP20_CODE)SP20Code;
UINT8 *bin_name[] = {"dsp_Pmem.bin","dsp_Amem.bin","dsp_Bmem.bin","dsp_Emem.bin"};
BtCardBusNICEnable(devExt);
#ifdef DOWNLOAD_3DSP_WITH_BIN_FILE_MODE
for(i = 0; i < 4; i++)
{	
RtlInitAnsiString(&AnsiString, (PUINT8)&bin_name[i]);
RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, TRUE);	
InitializeObjectAttributes(&InitiaObject, &ObjectName, OBJ_KERNEL_HANDLE |  OBJ_CASE_INSENSITIVE, NULL, NULL);
Status= ZwOpenFile(&FileHandle, FILE_OPEN_IF | SYNCHRONIZE, &InitiaObject, &IoStatus, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
if (Status != STATUS_SUCCESS)
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return (STATUS_SUCCESS);
}
Status =  ZwQueryInformationFile(FileHandle, &IoStatus, &fileinfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
if (Status != STATUS_SUCCESS)
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
return (STATUS_SUCCESS);
}
Status = ZwReadFile(FileHandle,
NULL,
NULL,
NULL,
&IoStatus,
(PVOID)MappedBuffer,
fileinfo.EndOfFile.LowPart,
NULL,
NULL
);
if (Status == STATUS_SUCCESS)
{
FileLength = fileinfo.EndOfFile.LowPart;
Status = BtUsbDspFwDownloadInterface(
devExt,
MappedBuffer,
FileLength,
(UINT8)i);
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
}
else
{
RtlFreeUnicodeString(&ObjectName);
ZwClose(FileHandle);
}
}
#else
{
ASSERT(pCodeBuffer->Size != 0);
while(pCodeBuffer->Size)
{
Status = BtUsbDspFwDownloadInterface(
devExt,
pCodeBuffer->Code,
pCodeBuffer->Size,
pCodeBuffer->Port);
pCodeBuffer = (PSP20_CODE)((PUINT8)pCodeBuffer + pCodeBuffer->Size + sizeof(SP20_CODE) - 1);
}	
BtDelay(2000);
}//else end
#endif
return (Status);
}
 */
NTSTATUS BtGetInfoFromEEPROM(PBT_DEVICE_EXT devExt)
{
	NTSTATUS status;
	
	UINT8 temp[] = {0x24,0x90, 0x02, 0x83,0x15,0x00,0x00,0x00};
	status = UsbReadFromProgramRegs(devExt, 0x00, 1024/4, devExt->BufferEeprom);
    if(!NT_SUCCESS(status)){
        return status;
    }
	BtUsbParseEEPROM(devExt);
	UsbWriteTo3DspRegs(devExt, BT_REG_LOCAL_BD_ADDR, 2, ((PBT_HCI_T)devExt->pHci)->local_bd_addr);
	return STATUS_SUCCESS;
	
}

NTSTATUS BtUsbParseEEPROM(PBT_DEVICE_EXT devExt)
{
	PBT_HCI_T	pHci = NULL;
	UINT8 	ValidFlag[4] = {0x11, 0x22, 0x33, 0x44};
	PUINT8  pTuple;
	UINT8 tmp_maxpower,maxpower,minpower;
	UINT32 tmpLong;
	UINT16 SubSystemId;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
	{
		BT_DBGEXT(ZONE_HAL | LEVEL0, "error, hci module pointer is null\n");
		return STATUS_UNSUCCESSFUL;
	}

	/*check valid flag*/
	if(0 != memcmp(ValidFlag, devExt->BufferEeprom+EEPROM_VALID_FLAG_OFFSET,EEPROM_VALID_FLAG_LENGTH))
	{
		BT_DBGEXT(ZONE_HAL | LEVEL0, "EEPROM data not valid");
		return STATUS_UNSUCCESSFUL;
	}

	/*bd addr*/
	RtlCopyMemory(pHci->local_bd_addr, devExt->BufferEeprom+EEPROM_BD_ADDRESS_OFFSET, BT_BD_ADDR_LENGTH);
	BT_DBGEXT(ZONE_HAL | LEVEL3, "local bd address is: %2x %2x %2x %2x %2x %2x\n",pHci->local_bd_addr[0],pHci->local_bd_addr[1],
		pHci->local_bd_addr[2],pHci->local_bd_addr[3],pHci->local_bd_addr[4],pHci->local_bd_addr[5]);

	/*version*/
	RtlCopyMemory(&devExt->EEPROM_Version, devExt->BufferEeprom+EEPROM_DATA_VERSION_OFFSET, sizeof(UINT32));
	BT_DBGEXT(ZONE_HAL | LEVEL3, "EEPROM data version:%8x\n", devExt->EEPROM_Version);

	/*license id*/
	RtlCopyMemory(pHci->encryption_key_from_ivt, devExt->BufferEeprom+EEPROM_LICENSE_ID_OFFSET, BT_IVT_ENCRYPTION_KEY_LENGTH);
	
	#ifdef DBG
	BT_DBGEXT(ZONE_HAL | LEVEL3, "license key:\n");
	BtPrintBuffer(pHci->encryption_key_from_ivt, BT_IVT_ENCRYPTION_KEY_LENGTH);
	#endif

	/* Get subsystem Id */
	SubSystemId = *(PINT16)&devExt->BufferEeprom[EEPROM_OFFSET_SUBSYSTEMID];
	if(Bt_SupportSniff(SubSystemId))
	{
		BT_DBGEXT(ZONE_HAL | LEVEL3, "Sub system Id: 0x%04x, support RSSI when combo sniff\n", SubSystemId);
		devExt->RssiWhenSniff = TRUE;
	}
	else
	{	
		devExt->RssiWhenSniff = FALSE;
	}

	/*compony id*/

	/*tx power table*/
	pTuple = devExt->BufferEeprom + EEPROM_TX_POWER_TABLE_OFFSET;
	tmp_maxpower=*(pTuple+7);
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Tx power: channel 1, 6M = %d\n", *(pTuple+7));
	tmp_maxpower += *(pTuple+5*12+7);
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Tx power: channel 6, 6M = %d\n", *(pTuple+5*12+7));
	tmp_maxpower += *(pTuple+10*12+7);
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Tx power: channel 11, 6M = %d\n", *(pTuple+10*12+7));
	
	maxpower=tmp_maxpower/3;
	if(maxpower > 63)
		maxpower = 63;

	maxpower = 60;
	minpower=maxpower%TX_POWER_ADJUST_STEP;
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Tx power: max average max power (1,6,11 channel) 6M = %d\n", maxpower);
	BT_DBGEXT(ZONE_HAL | LEVEL3, "Tx power: calculate min power = %d\n", minpower);
	
	pHci->tx_maxpower=maxpower;  //save to future use.
	pHci->tx_minpower=minpower; //save to future use.
	
	tmpLong = (maxpower << 8) | minpower;
	tmpLong |= (devExt->AntennaNum << 16);
	UsbWriteTo3DspReg(devExt, BT_REG_DEVICE_TXPOWER, tmpLong);

	return STATUS_SUCCESS;
	
}




VOID BtInitDSPParameters(PBT_DEVICE_EXT devExt)
{

	
	UsbInitDspPara(devExt);
/*
	while (i < size)
	{
		pdsp_para = (PDSP_PARA)(DSPPara + i);
		BT_DBGEXT(ZONE_HAL | LEVEL3, "DSPPara: BaseRegister = 0x%x, len = %d\n", pdsp_para->basereg, pdsp_para->size);
		BT_HAL_WRITE_BYTES((PUINT8)devExt->BaseRegisterAddress + pdsp_para->basereg, pdsp_para->para, pdsp_para->size);
		i += (sizeof(UINT32) + sizeof(UINT32) + pdsp_para->size);
	}
	*/
}

BOOLEAN Bt_SupportSniff(UINT16 SubSystemId)
{
	PBT_SUB_SYSTEM_ID	pSysId = (PBT_SUB_SYSTEM_ID)&SubSystemId;

	BT_DBGEXT(ZONE_HAL | LEVEL3, "Bt_SupportSniff---SubSystemId: %x\n", SubSystemId);

	/* Check old format of subsystem ID */
	if((SubSystemId == 0x0300) ||
		(SubSystemId == 0x0400) ||
		(SubSystemId == 0x0500) ||
		(SubSystemId == 0x0600) ||
		(SubSystemId == 0x0700) ||
		(SubSystemId == 0x0800))
	{
		return TRUE;
	}

	if((SubSystemId == 0x0100) ||
		(SubSystemId == 0x0200) ||
		(SubSystemId == 0x0900))
	{
		return FALSE;
	}

			

	/* Check new format of subsystem ID */
	if(pSysId->Chip == ROM_VER_V4B)
	{
		BT_DBGEXT(ZONE_HAL | LEVEL3, "Chip: %x, Board: %x\n", pSysId->Chip, pSysId->Board);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
