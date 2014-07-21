/***********************************************************************
 * FILENAME:         BT_USB_VendorCom.c
 * CURRENT VERSION:  1.00.01
 * CREATE DATE:      2006/10/16
 * PURPOSE:  vendor command ,read write registers,APIs of HCI and LMP.
 *
 * AUTHORS:  scott
 *
 * NOTES:    //
 ***********************************************************************/
#include "bt_sw.h"
#include "bt_dbg.h"        // include debug function
#include "bt_usb_vendorcom.h"        // include accessing hardware resources function
#include "btusb.h"                //include USB device build vendor command req
/*******************************************************************
 *   BtUsbVendorCommand
 *
 *   Descriptions:
 *      This routine is used to submit all the SYNCHRONOUS URBS to the
 *  	device.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.    (byte)
 *      request: IN, Device specific command request code,system definem0-3,we start 4   (code)
 *      resourcecode:different content different number      (vulue)
 *      index:
 *      direction:IN/OUT read is 1 write is 0
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbVendorCommand( PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT8 request,
UINT16 resourcecode,
UINT16 index,
UINT8 direction							
)
{
PURB pUrb;
NTSTATUS venStatus;
VENDOR_CMD_PARAMETERS VenCmdPara;
VenCmdPara.TransferBuffer =buf ;
VenCmdPara.TransferBufferLength = len;	
VenCmdPara.Request = request;
VenCmdPara.Value = resourcecode;
VenCmdPara.Index = index;
VenCmdPara.bIn = direction;
VenCmdPara.RequestTypeReservedBits  = 0;
VenCmdPara.bShortOk = FALSE;
#if 0
venStatus = UsbVendorRequest(devExt,&VenCmdPara);
if (venStatus!= STATUS_SUCCESS)
{
return STATUS_UNSUCCESSFUL;
}
#endif	
return STATUS_SUCCESS;
}
 */
/*******************************************************************
 *   BtBasicUsbWriteReg
 *
 *   Descriptions:
 *      The routine of Write Command contorl command.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.
 *      resourcecode:different content different number
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtBasicUsbWriteReg(PBT_DEVICE_EXT devExt,
UINT8 value,
UINT16 offset
)
{
return (BtUsbVendorCommand( devExt,NULL,1,ReqWriteRegByte,value,offset,WriteRegs));
}
 */
/*******************************************************************
 *   BtBasicUsbReadReg
 *
 *   Descriptions:
 *      The routine of Read Command contorl command.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.
 *      resourcecode:different content different number
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
UINT8 BtBasicUsbReadReg(PBT_DEVICE_EXT devExt,
UINT16 offset			
)
{
UINT8 tmp;
BtUsbVendorCommand( devExt,&tmp,1,ReqReadRegByte ,0x0,offset,ReadRegs);
return tmp;
}
 */
/*******************************************************************
 *   BtBasicUsbWriteBurstRegs
 *
 *   Descriptions:
 *      The routine of Write Command contorl command.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.
 *      resourcecode:different content different number
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtBasicUsbWriteBurstRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT16 offset
)
{
if (len <=0)
{
return STATUS_UNSUCCESSFUL;
}
return (BtUsbVendorCommand( devExt,buf,len,ReqBurstWrite,0,offset,WriteRegs));
//value is null, data in buff
}
 */
/*******************************************************************
 *   BtBasicUsbReadBurstRegs
 *
 *   Descriptions:
 *      The routine of Read Command contorl command.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf: IN, pointer to buffer that will be submitted.
 *      len: IN, length of buffer pointed to by buf.
 *      resourcecode:different content different number
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtBasicUsbReadBurstRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT16 offset						 							
)
{
return (BtUsbVendorCommand( devExt,buf,len,ReqRBurstRead ,0,offset,ReadRegs));
}
 */
/*
NTSTATUS BtUsbWrite3DSPRegs(
PBT_DEVICE_EXT devExt,
PUINT8			 buf,
UINT32 		        len,
UINT32			offset
)
{
NTSTATUS status = STATUS_SUCCESS;
return status;
}
 */
/*******************************************************************
 *   BtUsbWrite3DSPByte
 *
 *   Descriptions:
 *      Write one byte to a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      offset: IN, offset of register.
 *      value: IN, the data wants to be writen.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbWrite3DSPByte(PBT_DEVICE_EXT devExt,
UINT8 value,
UINT32 offset
)
{
UINT16 tmpoffset =0;
return UsbWriteTo3DspRegs(devExt,tmpoffset,1,&Value);
//	return BtUsbWrite3DSPRegs(devExt,&value,1,tmpoffset);
}
 */
/*******************************************************************
 *   BtUsbWrite3DSPWord
 *
 *   Descriptions:
 *      Write one word to a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbWrite3DSPWord(PBT_DEVICE_EXT devExt,
UINT16 value,
UINT32 offset
)
{
UINT16 tmpoffset =0;
return BtUsbWrite3DSPRegs(devExt,(PUINT8)&value,2,tmpoffset);
}
 */
/*******************************************************************
 *   BtUsbWrite3DSPDword
 *
 *   Descriptions:
 *      Write one word to a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbWrite3DSPDword(
PBT_DEVICE_EXT devExt,
UINT32			 value,
UINT32 			offset
)
{
UINT16 tmpoffset =0 ;
return BtUsbWrite3DSPRegs(devExt,(PUINT8)&value,sizeof(UINT32),tmpoffset);
}
 */
/*******************************************************************
 *   BtUsbRead3DSPRegs
 *
 *   Descriptions:
 *      Write one word to a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value: IN, the data wants to be writen.
 *      offset: IN, offset of register.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbRead3DSPRegs(
PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32  len,
UINT32 offset
)
{
return STATUS_SUCCESS;
}
 */
/*******************************************************************
 *   BtUsbRead3DSPByte
 *
 *   Descriptions:
 *      Read one byte from a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      offset: IN, offset of register.
 *   Return Value:
 *      the reading data.
 ******************************************************************/
/*
UINT8 BtUsbRead3DSPByte(PBT_DEVICE_EXT devExt,
UINT32 offset
)
{
UINT8 tmp;	
UINT16 tmpoffset =0;
if (BtUsbRead3DSPRegs (devExt,&tmp,sizeof(UINT8),tmpoffset)!= STATUS_SUCCESS)
{
tmp = 0;
}
return tmp;
}
 */
/*******************************************************************
 *   BtUsbRead3DSPWord
 *
 *   Descriptions:
 *      Read one word from a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      offset: IN, offset of register.
 *   Return Value:
 *      the reading data.
 ******************************************************************/
/*
UINT16 BtUsbRead3DSPWord(PBT_DEVICE_EXT devExt,
UINT32 offset
)
{
UINT16 tmp;
UINT16 tmpoffset =0;
if (BtUsbRead3DSPRegs (devExt,(PUINT8)&tmp,sizeof(UINT16),tmpoffset) != STATUS_SUCCESS)
{
tmp = 0;
}
return tmp;
}
 */
/*******************************************************************
 *   BtUsbRead3DSPDword
 *
 *   Descriptions:
 *      Read one double byte from a register.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      offset: IN, offset of register.
 *   Return Value:
 *      the reading data.
 ******************************************************************/
/*
UINT32 BtUsbRead3DSPDword(PBT_DEVICE_EXT devExt,
UINT32 offset
)
{
UINT32 tmp = 0;
UINT16 tmpoffset =0;
if (BtUsbRead3DSPRegs (devExt,(PUINT8)&tmp,sizeof(UINT32),tmpoffset)!= STATUS_SUCCESS)
{
tmp = 0;
}
return tmp;
}
 */
/************************************************************
 *   BtUsbReadWriteRegsAPI
 *
 *   Descriptions:
 *      read or write Registers API.
 *   Arguments:
 *      devExt: IN, the pointer to adapter context.
 *   Return Value
 *      NTSTATUS_SUCCESS if successfully
 *      NTSTATUS_xxx if unsucessfully
 ***********************************************************/
/*
NTSTATUS  BtUsbReadWriteRegsAPI(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len
)
{
return BtBasicUsbWriteBurstRegs(devExt,buf,len,(UINT16)OFFSET_MAILBOX_BASE_ADDRESS);
}
 */
// HAL API
/*******************************************************************
 *  BtUsbInitBluetoothRegs
 *
 *   Descriptions:
 *      This command is used for driver to init blue tooth registers.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 *    there are 3 pieces write regs
 *    write 0 to BaseRegisterAddress + BT_REG_AFH_MODE
 *    write 0 to BaseRegisterAddress + BT_REG_ENCRYPTION_ENABLE
 *    write 0 to BaseRegisterAddress + BT_REG_ENCRYPTION_ENABLE + 4
 ******************************************************************/
/*
NTSTATUS BtUsbHalInitBTRegs(PBT_DEVICE_EXT devExt,UINT32 value)
{
VCMD_API_T  val;
val.ReqCmdNum = ReqInitBtReg;
val.ULval0 = value;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *  BtUsbBackRestoreBTRegs
 *
 *   Descriptions:
 *      This command is used for driver to back up blue tooth registers.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      buf:IN, the buff which to read from regs
 *      size:IN
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 *    there are 2 pieces read regs to buffer
 *    read BaseRegisterAddress + BT_REG_LOCAL_BD_ADDR , size - 80
 *    read BaseRegisterAddress +BT_REG_FHS_FOR_INQUIRY_SCAN + size - 80,80
 ******************************************************************/
/*
NTSTATUS  BtUsbHalBackRestoreBTRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32 len,
UINT8 direction
)
{
VCMD_API_T val;
val.ReqCmdNum = ReqBackRestoreReg;
val.UCval0= buf;
val.ULval0 = len;
val.Act = direction;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *  BtUsbWriteClearComIndicator
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *      WriteClear:
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 *
 ******************************************************************/
/*
NTSTATUS  BtUsbWriteClearComIndicator(PBT_DEVICE_EXT devExt,
UINT32 value,
UINT8  WriteClear
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqWriteClearInd;
val.ULval0 = value;
val.Act = WriteClear;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *  BtUsbWriteCommandIndicatorSafe
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 *
 ******************************************************************/
/*
NTSTATUS  BtUsbWriteCommandIndicatorSafe(PBT_DEVICE_EXT devExt,
UINT32 value									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqWriteCmdSafe;
val.ULval0 = value;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *  BtUsbWriteCommandIndicatorSafe
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 *	// Write AM_CONNECTION_INDICATOR as 0. It means that no any connection is established.
 *	// Write HCI_COMMAND_INDICATOR as 0. It means that it is no command now.
 *
 ******************************************************************/
/*
NTSTATUS  BtUsbHciInit(PBT_DEVICE_EXT devExt
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciInit;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciTimeout
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS  BtUsbHciTimeout(PBT_DEVICE_EXT devExt	,
UINT8 amaddr
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciTimeout;
val.Act = amaddr;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciReceiveFromLmp
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS  BtUsbHciReceiveFromLmp(PBT_DEVICE_EXT devExt,
UINT8 amaddr
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciRecvLmp;
val.Act = amaddr;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciWriteClassOfDevice
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS  BtUsbHciWriteClassOfDevice(PBT_DEVICE_EXT devExt,
PUINT8 pClassOfDevice
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciClassDev;
val.UCval0= pClassOfDevice;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciComInquiry
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbHciComInquiry(PBT_DEVICE_EXT devExt
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciCmdInquiry;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciComCreateConnection
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS  BtUsbHciComCreateConnection(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciCreateCon;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciComRemoteNameRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS  BtUsbHciComRemoteNameRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciRemoteName;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciAcceptNameReq
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbHciAcceptNameReq(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciAcceptName;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciComMode
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbHciComMode(PBT_DEVICE_EXT devExt,
UINT8 bMode
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciCmdMode;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciPollTimerRoutine
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbHciPollTimerRoutine(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciPollTimer;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbHciRealeaseAllConnect
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbHciRealeaseAllConnect(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqHciResAllCon;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}	
 */
/*******************************************************************
 * BtUsbLmpAccceptedRequestSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedRequestSco(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccSco;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpAccceptedRemoveSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedRemoveSco(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpRemoveSco;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpAcceptedEncryptionEnable
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedEncryptionEnable(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccEncryptionEn;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpAcceptedSniffMode
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedSniffMode(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccSniff;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpAcceptedUnSniffMode
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedUnSniffMode(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccUnSniff;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpAcceptedTestMode
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptedTestMode(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccTestMode;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpNotAcceptedRemoveSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpNotAcceptedRemoveSco(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpNotAccRemoveSco;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpNotAcceptedSwitch
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpNotAcceptedSwitch(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpNotAccSwitch;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpNotAcceptedHoldUnsniff
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpNotAcceptedHoldUnsniff(PBT_DEVICE_EXT devExt,
UINT8 notAccHoldUnSniff
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpNotAccHold;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpStartStopEncryption
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpStartStopEncryption(PBT_DEVICE_EXT devExt,
UINT8  bStartStop
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpEncryption;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpSwitchRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSwitchRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSwitchReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpHold
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpHold(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpHold;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpHoldRequestSend
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpHoldRequestSend(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpHoldReqSend;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpRequestAccept
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpRequestAccept(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpReqAcc;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpsniffRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpsniffRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpsniffReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpUnsniffRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpUnsniffRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpUnsniffReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpScoLinkRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpScoLinkRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpScoLinkReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpRemoveScoLinkRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpRemoveScoLinkRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpRemoveScoLink;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpTestControl
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpTestControl(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpTestControl;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpSetAFHEnable
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSetAFHEnable(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSetAFHEnable;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpSetAFH
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSetAFH(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSetAFH;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpAcceptExteSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptExteSco(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccExtSco;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpAcceptExtRemoveeSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpAcceptExtRemoveeSco(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpAccRemoveSco;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpNotAcceptedExt
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpNotAcceptedExt(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpNotAccExt;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpeScoLinkRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpeScoLinkRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpeScoLinkReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpRemoveeScolinkRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpRemoveeScolinkRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpRemoveeScoLinkReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpHostAccepteSCOLink
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpHostAccepteSCOLink(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpHostAccScoLink;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpSendEnReqPDU
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSendEnReqPDU(PBT_DEVICE_EXT devExt,
UINT8 bEnable
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSendEnReqPDU;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpSendSetAFHPDUEnable
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSendSetAFHPDUEnable(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSendAFHPDUEnable;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpSendSetAFHPDU
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSendSetAFHPDU(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSendAFHPDU;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpSendHoldPDU
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSendHoldPDU(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSendHoldPDU;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpSendSwitchReqPDU
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpSendSwitchReqPDU(PBT_DEVICE_EXT devExt,
UINT8 bRole
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpSendSwitchReqPDU;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpPDUTimeout
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpPDUTimeout(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpPDUTimeout;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmp2LmProcess
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmp2LmProcess(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmp2LmProcess;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpInit
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpInit(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpInit;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbLmpReset
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpReset(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpReset;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbLmpReleaseSco
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbLmpReleaseScoMembers(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqLmpResScoMembers;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbMsicDeviceGetTxData
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbMsicDeviceGetRxData(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqMsicDevGetRxData;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbMsicDeviceSetTxData
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbMsicDeviceSetTxData(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqMsicDevSetTxData;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbMsicDeviceTestSlave
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbMsicDeviceTestSlave(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqMsicDevTestSlave;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbSendRecvRoleMasterInt
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbSendRecvRoleMasterInt(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqSendRecvRoleMasInt;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbSendRecvRoleInt
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbSendRecvRoleInt(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqSendRecvRoleInt;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbSendRecvRoleSlaveInt
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbSendRecvRoleSlaveInt(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqSnedRecvRoleSlaveInt;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbSendRecvRolePageScan
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbSendRecvRolePageScan(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqSendRecvRolePageScan;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbSendRecvRxPoolFrame
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbSendRecvRxPoolFrame(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqSendRecvRxPoolFrame;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbTestProcessRxFrame
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbTestProcessRxFrame(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqTestProcessRxFrame;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 * BtUsbTaskHc2HciReset
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbTaskHc2HciReset(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqTaskHc2HciReset;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*******************************************************************
 *BtUsbTaskReleaseRemoteRequest
 *
 *   Descriptions:
 *      This command is used for driver to command indicactor safe.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *      value:IN, the value
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 *    Note:
 ******************************************************************/
/*
NTSTATUS BtUsbTaskReleaseRemoteRequest(PBT_DEVICE_EXT devExt									
)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqTaskResRemoteReq;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
 */
/*
NTSTATUS BtUsbTransmitDSPFwHead(
PBT_DEVICE_EXT devExt,
UINT32 dma_src,				
UINT32 dma_dst ,
UINT32 dma_control_word
)
{
DSP_FW_HEAD_T      fw_head;
NTSTATUS status;
fw_head.request_cmd = ReqDownLoadDspCode;
fw_head.src_word = dma_src;
fw_head.dst_word = dma_dst;
fw_head.ctrl_word = dma_control_word;
status = BtUsbReadWriteRegsAPI(devExt,(PUINT8)&fw_head,sizeof(DSP_FW_HEAD_T));
return status;
}
 */
/*******************************************************************
 *   BtUsbWriteFwCtrlWord
 *
 *   Descriptions:
 *      The routine of Inquiry Firmware Status contorl command.
 *   Arguments:
 *      devExt: IN, the pointer of adapter context.
 *   Return Value:
 *      current status.
 ******************************************************************/
/*
NTSTATUS BtUsbWriteFwCtrlWord(
PBT_DEVICE_EXT devExt,
UINT16			 offset,
UINT16 		        len
)
{
//
NTSTATUS   status;
UINT32    src;
UINT32    dst;
UINT32    ctrl_word;
//caculate the three variable according offset & len
//src = pls //tell me
dst = DOWNLOAD_FW_FIELD_OFFSET + offset;
//ctl = pls //tell me
//fill result into dma reg
status = BtBasicUsbWriteBurstRegs(devExt,(PUINT8)&src,sizeof(UINT32),OFFSET_8051FW_DMA_SRC_REG);
status = BtBasicUsbWriteBurstRegs(devExt,(PUINT8)&dst,sizeof(UINT32),OFFSET_8051FW_DMA_DST_REG);
status = BtBasicUsbWriteBurstRegs(devExt,(PUINT8)&ctrl_word,sizeof(UINT32),OFFSET_8051FW_DMA_CTL_REG);
return status;
}
 */
/*******************************************************************
 *   BtUsbStart8051Fw
 *
 *   Descriptions:
 *      The routine starts 8051 fw code after 8051 fw has been downloaded into programe mem
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NDIS_STATUS_SUCCESS: return success.
 *      NDIS_STATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbStart8051Fw(PBT_DEVICE_EXT devExt)
{
//do it later , define it later
return (BtBasicUsbWriteReg(devExt,VCMD_VAL_RESTART_FW,OFFSET_8051_RESET_FW_REG));
}
 */
/*******************************************************************
 *   BtUsbSetFwDownloadOk
 *
 *   Descriptions:
 *      The routine of Firmware_Download_Ok contorl command.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NTSTATUS_SUCCESS: return success.
 *      NTSTATUS_xxx: return unsuccessful.
 ******************************************************************/
/*
NTSTATUS BtUsbSetFwDownloadOk(PBT_DEVICE_EXT devExt)
{
VCMD_API_T val;
val.ReqCmdNum =  ReqDownLoadFirmWareOK;
return (BtUsbReadWriteRegsAPI(devExt,(PUINT8)&val,sizeof(VCMD_API_T)));
}
   */
