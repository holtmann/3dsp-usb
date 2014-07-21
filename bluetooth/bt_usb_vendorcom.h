#ifndef BT_USB_VENDORCOM_H
#define BT_USB_VENDORCOM_H


#define APIMAXBUFLEN                                 32
#define VCMD_VAL_RESTART_FW			((UINT8)(0x00))     //define it later


//jakio20071114: add for power save
#define VCMD_API_8051_JUMP_MIN_PROCESS	(0x18)
#define VCMD_API_8051_JUMP_MAIN_PROCESS	(0x08)

#define VCMD_SUB_CMD_DRIVER_UNLOAD	0x00
#define VCMD_SUB_CMD_POWER_D3		0x01


//usb direct command

#define ReqReadRegByte 					0x00
#define ReqWriteRegByte 					0x01
#define ReqRBurstRead 					0x02
#define ReqBurstWrite 					0x03
#define ReqReadWriteMulWord                     0x04

//HAL
#define ReqInitBtReg                                     0x06
#define ReqWriteClearInd                     		0x07
#define ReqWriteCmdSafe                             0x08
#define ReqBackRestoreReg                          0x09

//HCI
#define ReqHciInit                                         0x0a
#define ReqHciTimeout                                  0x0b
#define ReqHciRecvLmp                                 0x0c
#define ReqHciClassDev                                0x0d
#define ReqHciCmdInquiry                            0x0f
#define ReqHciCreateCon                              0x10
#define ReqHciRemoteName                         0x11
#define ReqHciAcceptName                          0x12

#define ReqHciCmdMode                              0x14
#define ReqHciPollTimer                              0x15
#define ReqHciResAllCon                             0x16
#define ReqLmpAccSco                                0x17
#define ReqLmpRemoveSco                        0x18
#define ReqLmpAccEncryptionEn                0x19
#define ReqLmpAccSniff                             0x1a
#define ReqLmpAccUnSniff                         0x1b
#define ReqLmpAccTestMode                     0x1c
#define ReqLmpNotAccRemoveSco            0x1d
#define ReqLmpNotAccSwitch                    0x1f
#define ReqLmpNotAccHold                       0x20
#define ReqLmpEncryption                        0x21
#define ReqLmpSwitchReq                         0x22
#define ReqLmpHold                                  0x23
#define ReqLmpHoldReqSend                    0x24
#define ReqLmpReqAcc                              0x25
#define ReqLmpsniffReq                             0x26
#define ReqLmpUnsniffReq                         0x27
#define ReqLmpRemoveScoLink                0x28
#define ReqLmpTestControl                       0x29
#define ReqLmpSetAFHEnable                   0x2a
#define ReqLmpSetAFH                              0x2b
#define ReqLmpAccExtSco                         0x2c
#define ReqLmpAccRemoveSco                 0x2d
#define ReqLmpNotAccExt                         0x2f
#define ReqLmpScoLinkReq                       0x30
#define ReqLmpeScoLinkReq                     0x31
#define ReqLmpRemoveeScoLinkReq        0x32
#define ReqLmpHostAccScoLink                0x33
#define ReqLmpSendEnReqPDU                 0x34
#define ReqLmpSendAFHPDUEnable          0x35
#define ReqLmpSendAFHPDU                     0x36
#define ReqLmpSendHoldPDU                    0x37
#define ReqLmpSendSwitchReqPDU           0x38
#define ReqLmpPDUTimeout                      0x39
#define ReqLmp2LmProcess                      0x3a
#define ReqLmpInit                                    0x3b
#define ReqLmpReset                                 0x3c
#define ReqLmpResScoMembers                0x3d
#define ReqMsicDevGetRxData                   0x3e
#define ReqMsicDevSetTxData                   0x3f
#define ReqMsicDevTestSlave                    0x40
#define ReqSendRecvRoleMasInt               0x41
#define ReqSendRecvRoleInt                      0x42
#define ReqSnedRecvRoleSlaveInt             0x43
#define ReqSendRecvRolePageScan           0x44
#define ReqSendRecvRxPoolFrame            0x45
#define ReqTestProcessRxFrame               0x46
#define ReqTaskHc2HciReset                     0x47
#define ReqTaskResRemoteReq                 0x48
#define ReqDownLoadFirmWareOK            0x49
#define ReqDownLoadDspCode                  0x4a


/////////////////////////////////////////////////////////////////////

#define OFFSET_MAILBOX_BASE_ADDRESS			0x200    //define it later

//define structure for vendor

typedef struct _VCMD_API
{
	UINT8 ReqCmdNum; //index =reqnum;
	UINT8 Act;
	UINT32 ULval0;
	PUINT8 UCval0;
	UINT32 ULval1;
	PUINT8 UCval1;
} VCMD_API_T,  *PVCMD_API_T;

typedef struct _DSP_FW_HEAD
{
	UINT8 request_cmd;
	UINT16 offset; //the frag's offset
	UINT16 fraglen; // len of the fw frag
	UINT32 src_word;
	UINT32 dst_word;
	UINT32 ctrl_word;
} DSP_FW_HEAD_T,  *PDSP_FW_HEAD_T;

//
/*
NTSTATUS BtUsbVendorCommand( PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT8 request,
UINT16 resourcecode,
UINT16 index,
UINT8 direction							
);
 */
/*
NTSTATUS BtBasicUsbWriteReg(PBT_DEVICE_EXT devExt,
UINT8 value,
UINT16 offset
);
 */
/*
UINT8 BtBasicUsbReadReg(PBT_DEVICE_EXT devExt,
UINT16 offset			
);
 */
/*
NTSTATUS BtBasicUsbWriteBurstRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT16 offset
);
 */
/*
NTSTATUS BtBasicUsbReadBurstRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len,
UINT16 offset						 							
);






NTSTATUS BtUsbWrite3DSPWord(PBT_DEVICE_EXT devExt,
UINT16 value,
UINT32 offset
);


NTSTATUS BtUsbWrite3DSPDword(
PBT_DEVICE_EXT devExt,
UINT32			 value,
UINT32 			offset
);


NTSTATUS BtUsbWrite3DSPRegs(
PBT_DEVICE_EXT devExt,
PUINT8			 buf,
UINT32 		        len,
UINT32			offset);

 */
/*
NTSTATUS BtUsbRead3DSPRegs(
PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32  len,
UINT32 offset
);
 */
/*
UINT8 BtUsbRead3DSPByte(PBT_DEVICE_EXT devExt,
UINT32 offset
);

UINT16 BtUsbRead3DSPWord(PBT_DEVICE_EXT devExt,
UINT32 offset
);

UINT32 BtUsbRead3DSPDword(PBT_DEVICE_EXT devExt,
UINT32 offset
);



NTSTATUS  BtUsbReadWriteRegsAPI(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT16 len
);

//hal
//NTSTATUS BtUsbHalInitBTRegs(PBT_DEVICE_EXT devExt,UINT32 value);
NTSTATUS  BtUsbHalBackRestoreBTRegs(PBT_DEVICE_EXT devExt,
PUINT8 buf,
UINT32 len,
UINT8 direction
);
NTSTATUS  BtUsbWriteClearComIndicator(PBT_DEVICE_EXT devExt,
UINT32 value,
UINT8  WriteClear
);
NTSTATUS  BtUsbWriteCommandIndicatorSafe(PBT_DEVICE_EXT devExt,
UINT32 value									
);
//hci
NTSTATUS  BtUsbHciInit(PBT_DEVICE_EXT devExt);
NTSTATUS  BtUsbHciTimeout(PBT_DEVICE_EXT devExt,
UINT8 amaddr
);
NTSTATUS  BtUsbHciReceiveFromLmp(PBT_DEVICE_EXT devExt,
UINT8 amaddr
);
NTSTATUS  BtUsbHciWriteClassOfDevice(PBT_DEVICE_EXT devExt,
PUINT8 pClassOfDevice
);
NTSTATUS BtUsbHciComInquiry(PBT_DEVICE_EXT devExt
);
NTSTATUS  BtUsbHciComCreateConnection(PBT_DEVICE_EXT devExt									
);
NTSTATUS  BtUsbHciComRemoteNameRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbHciAcceptNameReq(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbHciComMode(PBT_DEVICE_EXT devExt,
UINT8 bMode
);

NTSTATUS BtUsbHciPollTimerRoutine(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbHciRealeaseAllConnect(PBT_DEVICE_EXT devExt									
);

//LMP
NTSTATUS BtUsbLmpAcceptedRequestSco(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptedRemoveSco(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptedEncryptionEnable(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptedSniffMode(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptedUnSniffMode(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptedTestMode(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpNotAcceptedRemoveSco(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpNotAcceptedSwitch(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpNotAcceptedHoldUnsniff(PBT_DEVICE_EXT devExt,
UINT8 notAccHoldUnSniff
);
NTSTATUS BtUsbLmpStartStopEncryption(PBT_DEVICE_EXT devExt,
UINT8  bStartStop
);
NTSTATUS BtUsbLmpSwitchRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpHold(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpHoldRequestSend(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpRequestAccept(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpsniffRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpUnsniffRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpScoLinkRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpRemoveScoLinkRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpTestControl(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSetAFHEnable(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSetAFH(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptExteSco(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpAcceptExtRemoveeSco(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpNotAcceptedExt(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpeScoLinkRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpRemoveeScolinkRequest(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpHostAccepteSCOLink(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSendEnReqPDU(PBT_DEVICE_EXT devExt,
UINT8 bEnable									
);
NTSTATUS BtUsbLmpSendSetAFHPDUEnable(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSendSetAFHPDU(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSendHoldPDU(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpSendSwitchReqPDU(PBT_DEVICE_EXT devExt,
UINT8 bRole
);
NTSTATUS BtUsbLmpPDUTimeout(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmp2LmProcess(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpInit(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpReset(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbLmpReleaseScoMembers(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbMsicDeviceGetRxData(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbMsicDeviceSetTxData(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbMsicDeviceTestSlave(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbSendRecvRoleMasterInt(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbSendRecvRoleInt(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbSendRecvRoleSlaveInt(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbSendRecvRolePageScan(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbSendRecvRxPoolFrame(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbTestProcessRxFrame(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbTaskHc2HciReset(PBT_DEVICE_EXT devExt									
);
NTSTATUS BtUsbTaskReleaseRemoteRequest(PBT_DEVICE_EXT devExt									
);
//firmware

NTSTATUS BtUsbTransmitDSPFwHead(
PBT_DEVICE_EXT devExt,
UINT32 dma_src,				
UINT32 dma_dst ,
UINT32 dma_control_word
);
 */
/*
NTSTATUS BtUsbWriteFwCtrlWord(
PBT_DEVICE_EXT devExt,
UINT16			 offset,
UINT16 		        len
);
 */
//NTSTATUS BtUsbStart8051Fw(PBT_DEVICE_EXT devExt);
//NTSTATUS BtUsbSetFwDownloadOk(PBT_DEVICE_EXT devExt);

#endif
