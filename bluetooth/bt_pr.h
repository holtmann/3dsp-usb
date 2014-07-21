/************************************************************************
 * Copyright (c) 2003, 3DSP Corporation, all rights reserved.  Duplication or
 * reproduction of any part of this software (source code, object code or
 * comments) without the expressed written consent by 3DSP Corporation is
 * forbidden.  For further information please contact:
 *
 * 3DSP Corporation
 * 16271 Laguna Canyon Rd
 * Irvine, CA 92618
 * www.3dsp.com
 *************************************************************************/

#ifndef __BT_PR_H
#define __BT_BR_H

//
// We define the external interfaces to the blue tooth driver.
// These routines are only external to permit separate
// compilation.  Given a truely fast compiler they could
// all reside in a single file and be static.
//




//
// Shared routines
//
NTSTATUS UsbInitialize(IN PBT_DEVICE_EXT deviceExtension);
VOID UsbUnInitialize(IN PBT_DEVICE_EXT deviceExtension);
VOID Usb_ResetResource(PBT_DEVICE_EXT devExt);


VOID UsbAsynConReset(IN PBT_DEVICE_EXT devExt);
VOID UsbAsynConRelease(IN PBT_DEVICE_EXT devExt);
NTSTATUS UsbAsynConInit(IN PBT_DEVICE_EXT devExt);

NTSTATUS UsbActiveDevice(PBT_DEVICE_EXT devExt);
NTSTATUS UsbVendorRequest(PBT_DEVICE_EXT devExt, PVENDOR_CMD_PARAMETERS VenderCmdParameters);
NTSTATUS UsbVendorRequestAsyn(PBT_DEVICE_EXT devExt, PVENDOR_CMD_PARAMETERS VenderCmdParameters, UINT32 cmdSrc);
VOID Usb_VendorAsynCompletion(PURB pUrb);
UINT8 VendorCmdActivateByte(PBT_DEVICE_EXT devExt);
UINT8 VendorCmdRegAPIByte(PBT_DEVICE_EXT devExt);
NTSTATUS VendorCmdRegRead(PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PUINT8 pValue);
NTSTATUS VendorCmdRegWrite(PBT_DEVICE_EXT devExt, IN UINT16 address, IN UINT8 pValue);





//NTSTATUS VendorCmdBurstRegRead(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length);
NTSTATUS VendorCmdBurstRegRead2(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length);
NTSTATUS VendorCmdBurstRegWrite(IN PBT_DEVICE_EXT devExt,IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length);
NTSTATUS VendorCmdBurstRegWrite2(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, IN UINT32 Length);
NTSTATUS VendorCmdWrite3DSPDword(IN PBT_DEVICE_EXT devExt, IN UINT32 value, IN UINT16 offset);
UINT32 VendorCmdRead3DSPDword(IN PBT_DEVICE_EXT devExt, IN UINT16 offset);
NTSTATUS VendorCmdBurstRegWriteAnyn(IN PBT_DEVICE_EXT devExt, IN UINT16 address, IN OUT PVOID pValue, 
								IN UINT32 Length, UINT32 cmdSrc);
NTSTATUS VendorCmdWriteLengthToMailBox(PBT_DEVICE_EXT devExt, IN PVOID value, IN UINT8 CmdType);
NTSTATUS VendorCmdWriteAclAndOthersLength(PBT_DEVICE_EXT devExt, IN PVOID value, IN UINT8 CmdType);
NTSTATUS VendorCmdWriteCmdToMailBox(PBT_DEVICE_EXT devExt, IN PVOID value, IN UINT8 CmdType);
NTSTATUS VendorCmdWriteDataLength(PBT_DEVICE_EXT devExt, IN UINT16 Length, IN UINT8 CmdType);

NTSTATUS Vcmd_CardBusNICEnable(PBT_DEVICE_EXT devExt);
NTSTATUS Vcmd_Transmit_DSPFw_Head(PBT_DEVICE_EXT devExt, UINT16 offset, UINT32 len, UINT8 file_id);

NTSTATUS Vcmd_mmacSp20ResetApplied(PBT_DEVICE_EXT devExt);


NTSTATUS Vcmd_Set_Firmware_Download_Ok(PBT_DEVICE_EXT devExt);
NTSTATUS Vcmd_NIC_INTERRUPT_ENABLE(PBT_DEVICE_EXT devExt);
NTSTATUS Vcmd_NIC_ENABLE_RETRY(PBT_DEVICE_EXT devExt, UINT32 value);
NTSTATUS Vcmd_flush_tx_fifo(PBT_DEVICE_EXT devExt);

NTSTATUS Adap_mmacRestart_initial(PBT_DEVICE_EXT devExt, UINT32 channel);

NTSTATUS Vcmd_NIC_INTERRUPT_DISABLE(PBT_DEVICE_EXT devExt);
NTSTATUS Vcmd_CardBusNICReset(PBT_DEVICE_EXT devExt);
NTSTATUS Vcmd_PowerSave_CardBusNICReset(PBT_DEVICE_EXT devExt);


NTSTATUS Vcmd_NIC_RESET_ALL_BUT_HARDPCI(PBT_DEVICE_EXT devExt);

VOID UsbInsertBulkInFreeQueue(PBT_DEVICE_EXT devExt, UINT8 iNumber);
VOID UsbInsertBulkInUsedQueue(PBT_DEVICE_EXT devExt, UINT8 iNumber);

NTSTATUS BtUsbInt(PBT_DEVICE_EXT devExt);

BOOLEAN UsbCancelInterruptReq(PBT_DEVICE_EXT devExt);
BOOLEAN UsbCancelRxReq(PBT_DEVICE_EXT devExt);
BOOLEAN UsbCancelInquiryIrp(PBT_DEVICE_EXT devExt);
NTSTATUS UsbCancelAclWriteIrp(PBT_DEVICE_EXT devExt);
NTSTATUS UsbCancelScoWriteIrp(PBT_DEVICE_EXT devExt);
BOOLEAN UsbCancelAclIrp(PBT_DEVICE_EXT devExt);
VOID BtUsbWriteIrpTimeOut(PBT_DEVICE_EXT devExt);
NTSTATUS BtUsbSubmitReadIrp(PBT_DEVICE_EXT devExt);
NTSTATUS BtUsbSubmitIquiryIrp(PBT_DEVICE_EXT devExt);

VOID BtUsbRxFailTimeOut(PBT_DEVICE_EXT devExt);

NTSTATUS BtUsbWrite(PBT_DEVICE_EXT devExt,  IN PVOID Buffer, IN UINT32 Length, IN UINT32 DataType);
NTSTATUS BtUsbWriteAll(PBT_DEVICE_EXT devExt, IN PVOID Buffer, IN UINT32 Length, IN UINT8 dataType, UINT8 UpdateSpace);
NTSTATUS UsbWriteTo3DspReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT32 Datas);
NTSTATUS UsbWriteTo3DspRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT8 DataNum, IN PVOID Datas);
NTSTATUS UsbReadFrom3DspReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, OUT PUINT32 Datas);
NTSTATUS UsbReadFrom3DspRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT8 DataNum, OUT PUINT8 Datas);
NTSTATUS DownLoadFirmWare(IN PBT_DEVICE_EXT deviceExtension, IN UINT8 Type, BOOLEAN GotoMainLoop);

UINT16 Usb_Read_2Bytes_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 reg);
UINT32 Usb_Read_4Bytes_From_3DspReg(PBT_DEVICE_EXT devExt, UINT32 reg);

NTSTATUS UsbReadFromProgramReg(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, OUT PUINT32 Datas);
NTSTATUS UsbReadFromProgramRegs(IN PBT_DEVICE_EXT devExt, IN UINT32 offset, IN UINT16 DataNum, OUT PUINT8 Datas);






NTSTATUS BtAddDevice(PBT_DEVICE_EXT devExt);
VOID BtWriteToDevice(PBT_DEVICE_EXT devExt);


VOID BtRequestIncrement(PBT_DEVICE_EXT devExt);
VOID BtRequestDecrement(PBT_DEVICE_EXT devExt);
VOID BtClearQueues(PBT_DEVICE_EXT DevExt);
VOID BtClearIvtData(PBT_DEVICE_EXT DevExt);
// The following function is used to find the correct serial number




NTSTATUS BtStartDevice(PBT_DEVICE_EXT devExt);
NTSTATUS BtStartDevice_2Phase(PBT_DEVICE_EXT devExt, UINT8 mode);
NTSTATUS ReleasePreStart(PBT_DEVICE_EXT devExt);

VOID BtReleaseResources(PBT_DEVICE_EXT devExt);
BOOLEAN BtStartReadOnDevice(IN PVOID SynchronizeContext);
BOOLEAN BtStartWriteOnDevice(IN PVOID SynchronizeContext);

NTSTATUS BtSetupShareMemory(PBT_DEVICE_EXT devExt);
VOID BtDeleteShareMemory(PBT_DEVICE_EXT devExt);

VOID BtTxThread_Func(IN PVOID DeferredContext);

UINT8 BtGetFreeLMPCach(PBT_DEVICE_EXT devExt);
NTSTATUS BtProcessTxCommand(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
NTSTATUS BtProcessTxLMPDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength);
BT_TX_FRAME_TYPE BtGetTxFrameType(PUINT8 buf, UINT32 len);

NTSTATUS BtProcessTxPollFrame(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS BtProcessTxAclNullFrame(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 flow);

NTSTATUS BtProcessRx(PBT_DEVICE_EXT devExt, PUINT32 pOutLen);
NTSTATUS BtReadData(IN PBT_DEVICE_EXT devExt, IN OUT PUINT32 pOutLen);
NTSTATUS BtProcessRxData(PBT_DEVICE_EXT devExt, PUINT8 source, UINT32 inlen, PUINT8 dest, PUINT32 poutlen, PUINT8 pPktType);

VOID BtProcessRxInt(PBT_DEVICE_EXT devExt, UINT8 RecvAreaIndex);
VOID BtProcessRx_New(PBT_DEVICE_EXT devExt, UINT8 RecvAreaIndex);
VOID BtPreProcessPacket(PBT_DEVICE_EXT devExt, UINT32 PacketType, PUINT8 buffer, UINT32 length);
VOID BtTaskProcessInquiryResult(PBT_DEVICE_EXT devExt, PUINT8 pbuffer, UINT32 length);
UINT32 BtSetPacketFilter(PBT_DEVICE_EXT devExt);
VOID BtTaskProcessRx(PBT_DEVICE_EXT devExt, PUINT8 buffer, UINT32 BufLength);
VOID BtProcessInquiryResultInt(PBT_DEVICE_EXT devExt);
VOID BtProcessInquiryCompleteInt(PBT_DEVICE_EXT devExt);
VOID BtProcessHardErrorInt(PBT_DEVICE_EXT devExt);
VOID BtProcessFlushOccuredInt(PBT_DEVICE_EXT devExt);
VOID BtProcessRoleChangeInt(PBT_DEVICE_EXT devExt);
VOID BtProcessDspAckInt(PBT_DEVICE_EXT devExt);
VOID BtProcessModeChangeInt(PBT_DEVICE_EXT devExt, UINT8 mode);
VOID BtProcessClockReadyInt(PBT_DEVICE_EXT devExt, UINT32 AfhInstant);
VOID BtProcessRoleChangeFailInt(PBT_DEVICE_EXT devExt);
VOID BtProcessReportRxPowerInt(PBT_DEVICE_EXT devExt);



UINT32 BtGetBDAddrById(UINT32 id);
UINT32 BtGetNextBDId(UINT32 id);
BOOLEAN BtIsBDEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsMasterBDEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsSlaveBDEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsTxPoolFull(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsTxScoPoolFull(PBT_DEVICE_EXT devExt);
PUINT8 BtGetValidTxPool(PBT_DEVICE_EXT devExt);
PUINT8 BtGetValidTxScoPool(PBT_DEVICE_EXT devExt);
VOID ReleaseBulkOut1Buf(PVOID pBuffer, PBT_DEVICE_EXT devExt);
VOID ReleaseBulkOut2Buf(PVOID pBuffer, PBT_DEVICE_EXT devExt);
UINT32 BtGetNextRxCachId(UINT32 id);
BOOLEAN BtIsRxCachEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsRxCachFull(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsRxCachToBeFull(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsRxCachGood(PBT_DEVICE_EXT devExt);
UINT32 BtGetScoBDAddrById(UINT32 id);
BOOLEAN BtIsScoBDEmpty(PBT_DEVICE_EXT devExt);
BOOLEAN BtIsScoBDFull(PBT_DEVICE_EXT devExt);

BT_RX_FRAME_TYPE BtGetRxFrameType(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 datalen, PUINT8 Buf);
VOID BtTransferRxFrameToRxCach(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 tailaddr, UINT32 datalen, PUINT8 srcBuf, PUINT8 destbuf);
VOID BtTransferRxFrameBodyToRxCach(PBT_DEVICE_EXT devExt, UINT32 startaddr, UINT32 tailaddr, UINT32 datalen, PUINT8 srcBuf, PUINT8 destbuf);
VOID BtTransferRxFrameToRxLMPPDUCach(PBT_DEVICE_EXT devExt, PUINT8 Buf, UINT32 datalen);
VOID BtProcessRxLMPPDU(PBT_DEVICE_EXT devExt, UINT8 index);
VOID BtProcessRxNULLFrame(PBT_DEVICE_EXT devExt, UINT32 tmpstartaddr, PUINT8 pBuf);
VOID BtProcessRxNULLFrame_New(PBT_DEVICE_EXT devExt, UINT32 tmpstartaddr, PUINT8 pBuf);
VOID BtProcessRxPoolFrame(PBT_DEVICE_EXT devExt, PUINT8 pBuf);
VOID BtTaskProcessRxPoolFrame(PBT_DEVICE_EXT devExt);
VOID BtProcessRxCRCErrorFrame(PBT_DEVICE_EXT devExt, UINT8 master_slave_flag, UINT8 amaddr, UINT8 slaveIndex);
VOID BtProcessRxAFHChBadFrame(PBT_DEVICE_EXT devExt, UINT32 BadChanNum);

// Timer related function
VOID BtInitializeTimer(PBT_DEVICE_EXT devExt);
// Timeout function
VOID BtTriggerRcv(ULONG_PTR SystemContext);
VOID BtWatchdogTimer(ULONG_PTR DeviceObject);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 21)   
VOID BtWatchdogItem(struct work_struct *pwork);
#else
VOID BtWatchdogItem(PVOID pvoid);
#endif
VOID BtEnableInterrupt(PBT_DEVICE_EXT devExt);
VOID BtDisableInterrupt(PBT_DEVICE_EXT devExt);
UINT32 BtReadFromReg(PBT_DEVICE_EXT devExt, UINT32 offset);
VOID BtWriteToReg(PBT_DEVICE_EXT devExt, UINT32 offset, UINT32 value);
VOID BtDelay(UINT32 millseconds);
UINT32 BtReadDSPInt(PBT_DEVICE_EXT devExt);
VOID BtWriteDSPInt(PBT_DEVICE_EXT devExt, UINT32 value);
VOID BtCardBusNICEnable(PBT_DEVICE_EXT devExt);
VOID BtCardBusNICReset(PBT_DEVICE_EXT devExt);
VOID BtCardBusNICStart(PBT_DEVICE_EXT devExt);
NTSTATUS BtNICReloadSP20Code(PBT_DEVICE_EXT devExt);
NTSTATUS BtScratch_2_DSP_MemoryBank(PBT_DEVICE_EXT devExt, VOID *Buffer, UINT32 FileLength, UINT8 memport);
NTSTATUS BtDSP_Memorybank_2_Scratch(PBT_DEVICE_EXT devExt, VOID *Buffer, UINT32 FileLength, UINT8 memport);

VOID BtSp20ResetApplied(PBT_DEVICE_EXT devExt);
VOID BtSp20ResetRemoved(PBT_DEVICE_EXT devExt);

NTSTATUS BtInitHw(PBT_DEVICE_EXT devExt);

NTSTATUS BtGetInfoFromEEPROM(PBT_DEVICE_EXT devExt);
NTSTATUS BtUsbParseEEPROM(PBT_DEVICE_EXT devExt);

VOID BtInitSomeCSR(PBT_DEVICE_EXT devExt);
VOID BtInitSomeVars(PBT_DEVICE_EXT devExt);
//VOID BtSetDMA1(PBT_DEVICE_EXT devExt);

VOID BtWriteFHSPacket(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pTempConnectDevice);


VOID BtPower_SetPowerD0(PBT_DEVICE_EXT devExt, UINT8 DownFwFlag, UINT8 Req8051MainLoop, UINT8 PowerFlag);
VOID BtPower_SetPowerD3(PBT_DEVICE_EXT devExt, UINT8 CancelIntIrp, UINT8 Req8051MinLoop, UINT8 PowerFlag);

NTSTATUS UsbInitDspPara(PBT_DEVICE_EXT devExt);
NTSTATUS UsbQueryDMASpace(PBT_DEVICE_EXT devExt);
VOID BtInitDSPParameters(PBT_DEVICE_EXT devExt);
BOOLEAN Bt_SupportSniff(UINT16 SubSystemId);

BT_RX_FRAME_TYPE BtGetRxFrameTypeAndRetryCount(PBT_DEVICE_EXT devExt, UINT32 startaddr, PUINT32 pretrycount, PUINT8 pam_addr, PUINT8 ptype, PUINT8 flow, PUINT8 master_slave_flag, PUINT8 slave_index, PUINT8 pHeadDesc);

NTSTATUS UsbResetPipe(PBT_DEVICE_EXT devExt, UINT8 PipeNum);

VOID BtCheckAndChangeRxState(PBT_DEVICE_EXT devExt);

VOID BtPrintBuffer(PUINT8 buffer, UINT32 len);

NTSTATUS LMP_WriteReg_AfterSendingLMP(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
NTSTATUS Send_SetAFH_PDU_WithoutWritingReg(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_mode, UINT8 flag, UINT32 AfhInstant);
NTSTATUS Send_ChannelClassificationReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_reporting_mode);


//jakio20071107: add for bulk writing api
PUINT8	RegApi_Fill_Cmd_Body(PBT_DEVICE_EXT devExt, UINT8 subCmd, UINT16 addr, PUINT8 destBuf, UINT16 len, PUINT8 srcBuf);
NTSTATUS RegAPI_SendTo_MailBox(IN PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len);
PUINT8 RegApi_Fill_Cmd_Body_Constant(PBT_DEVICE_EXT devExt, UINT8 subCmd, UINT16 addr, PUINT8 destBuf, UINT16 len, UINT32 value);
PUINT8 RegApi_Write_Cmd_Indicator(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 value);
VOID RegAPI_Print_Buffer(PUINT8 buffer);
//#endif

BOOLEAN Vcmd_8051fw_isWork(PBT_DEVICE_EXT devExt);
BOOLEAN BtUsb_Device_Removed(PBT_DEVICE_EXT devExt);





NTSTATUS Flush_Init(PBT_DEVICE_EXT DevExt);
VOID Flush_Reset(PBT_DEVICE_EXT DevExt);
VOID Flush_Release(PBT_DEVICE_EXT DevExt);
NTSTATUS Flush_BakupData(PBT_DEVICE_EXT DevExt, PUINT8 buffer, UINT32 length, UINT8 ListIndex);
UINT32 Flush_SendBakup_Packet(PBT_DEVICE_EXT DevExt, UINT8 ListIndex);
VOID Flush_SetNewPacketType(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Flush_ProcessFlushInt(PBT_DEVICE_EXT DevExt, PUINT8 pBuffer);
BOOLEAN	Flush_HasData_ToSend(PBT_DEVICE_EXT DevExt, PUINT8 pListIndex);
unsigned char Bus_get_hotkey_flag(void *pIntf);

/* URB complete tasklet function */
void usb_complete_task(ULONG_PTR arg);
void _urb_queue_init(struct _urb_queue *q);
void _urb_queue_head(struct _urb_entry *ue, struct _urb_queue *q);
void _urb_queue_tail(struct _urb_entry *ue, struct _urb_queue *q);
struct _urb_entry *_urb_dequeue(struct _urb_queue *q);
struct _urb_entry *_get_free_urb_entry(struct _urb_queue *q);


static inline void DumpUsbRegs(PBT_DEVICE_EXT devExt)
{
    UINT32 tmp = 0;
    UINT32 i = 0, j;

    while(i < 48){
        j = 0;
        printk(KERN_INFO "0x%x ~ 0x%x:", i*8, i*8+7);
        while(j < 8){
            VendorCmdBurstRegRead2(devExt, i*8+j, &tmp, 1);
            printk(KERN_INFO " 0x%x: 0x%x", i*8+j, tmp);
            j++;
        }
        printk(KERN_INFO "\n");
        i++;
    }
}

#endif
