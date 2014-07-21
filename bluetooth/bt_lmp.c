
/***********************************************************************
  * FILENAME:         BT_Lmp.c
  * CURRENT VERSION:  1.00.01
  * CREATE DATE:      2005/08/16
  * PURPOSE:  We package all the LMP procedures into this file. 
  *
  * AUTHORS:  Lewis Wang
  *
  * NOTES:    //
 ***********************************************************************/
	
/*
 *  REVISION HISTORY
 */

#include "bt_sw.h"         /* include <WDM.H> and PBT_DEVICE_EXT structure */
#include "bt_lmp.h"
#include "bt_task.h"
#include "bt_frag.h"
#include "bt_pr.h"
#include "bt_hal.h"        /* include accessing hardware resources function */
#include "bt_dbg.h"        /* include debug function */
#include "bt_usb_vendorcom.h"
#ifdef BT_TESTMODE_SUPPORT
#include "bt_testmode.h"
#endif
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
#include "afhclassify.h"
#endif
#ifdef BT_SERIALIZE_DRIVER
#include "bt_serialize.h"
#endif

/*--file local macros--------------------------------------------------*/

/* the different error reasons used in LMP */
#define  AUTHENTICATION_FAILURE             0x05
#define  KEY_MISSING                        0x06
#define  MAX_SCO_CONNECTIONS                0x0A
#define  REJECTED_LIMITED_RESOURCES         0x0D
#define  REJECTED_SECURITY_REASONS          0x0E
#define  REJECTED_PERSONAL_DEVICE           0x0F
#define  HOST_TIMEOUT                       0x10
#define  UNSUPPORTED_FEATURE_OR_PARAMETER   0x11
#define  TERMINATED_USER_ENDED              0x13
#define  TERMINATED_LOW_RESOURCES           0x14
#define  TERMINATED_POWEROFF                0x15
#define  TERMINATED_LOCAL_HOST              0x16
#define  REPEATED_ATTEMPTS                  0x17
#define  PAIRING_NOT_ALLOWED                0x18
#define  UNKNOWN_LMP_PDU                    0x19
#define  UNSUPPORTED_LMP_FEATURE            0x1A
#define  SCO_OFFSET_REJECTED                0x1B
#define  SCO_INTERVAL_REJECTED              0x1C
#define  SCO_AIR_MODE_REJECTED              0x1D
#define  INVALID_LMP_PARAMETERS             0x1E
#define  UNSPECIFIED_ERROR                  0x1F
#define  UNSUPPORTED_PARAMETER              0x20
#define  SWITCH_NOT_ALLOWED                 0x21
#define  LMP_RESPONSE_TIMEOUT               0x22
#define  ERROR_TRANSACTION_COLLISION        0x23
#define  PDU_NOT_ALLOWED                    0x24
#define  ENCRYPTION_MODE_NOT_ACCEPTABLE     0x25
#define  UNIT_KEY_USED                      0x26
#define  QOS_NOT_SUPPORTED                  0x27
#define  INSTANT_PASSED                     0x28
#define  PAIRING_UNITKEY_NOT_SUPPORTED      0x29
#define  DIFFERENT_TRANSACTION_COLLISION    0x2A
#define  QOS_UNACCEPTABLE_PARAMETER         0x2C
#define  QOS_REJECTED                       0x2D
#define  INSUFFICIENT_SECURITY              0x2F
#define  PARAMETER_OUT_RANGE                0x30
#define  ROLE_SWITCH_PENDING                0x32
#define  RESERVED_SLOT_VIOLATION            0x34
#define  SIMPLE_PAIRING_HOST_NOT_SUPPORTED  0x37
#define  HOST_BUSY_PAIRING                  0x38

/* Encryption Key Size range */
#define MIN_EN_KEY_SIZE         1
#define MAX_EN_KEY_SIZE         16

#define SLAVE_REGISTER_OFFSET   0

/* eSCO packet */
#define ESCO_PACKET_2EV3        0x26
#define ESCO_PACKET_3EV3        0x37
#define ESCO_PACKET_2EV5        0x2C
#define ESCO_PACKET_3EV5        0x3D

/*--file local constants and types-------------------------------------*/

/*--local function prototypes------------------------------------------*/

UINT8 LMP_ChangeSniffParaForSCO(PBT_DEVICE_EXT devExt, PBT_SNIFF_T pSniff);
void  LMP_SCONeedUnsniff(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
void LMP_Unsniff_SCO(PBT_DEVICE_EXT devExt);
UINT8 LMP_GetOffsetOfSCO(PBT_DEVICE_EXT devExt, UINT8 interval, UINT8 offset);

NTSTATUS LMP_SendPDUToLC(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);
NTSTATUS LMP_SendPDUToLCHead(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);
void     LMP_StartPDUTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_type, UINT16 timer_count);
void     LMP_StopPDUTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
void     ChangeLmpState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state);
void     ChangeLmpExtraState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state);
void     CancelLmpBitState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state, UINT8 extra_flag);

UINT8    CheckSCOPara(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_SCO_T psco, UINT8 flag);
UINT8    SCONotAccepted_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ErrorReason);
void     DeleteSCOLink(PBT_DEVICE_EXT devExt, UINT8 index);
void     DeleteESCOLink(PBT_DEVICE_EXT devExt, UINT8 index);
void     After_eSCO_Complete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco);
UINT8    CheckESCOPara(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco, PUINT8 pChangeFlag);
UINT8    GetESCORetransReg(UINT8 PacketM, UINT8 PacketL, UINT8 RetransSlot);
UINT8    GetESCOPacketReg(UINT8 PacketM, UINT8 PacketL);
void     DeleteSniffLink(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS ResLmpNotAccepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 ErrReason);
NTSTATUS ResLmpNotAcceptedExt(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 ErrReason);
NTSTATUS ResLmpDetach(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ErrReason);
NTSTATUS Res_InRand_Accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Authentication_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Pairing_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS PauseResumeEncryption_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS After_Conn_Authentication(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS After_Conn_SetupComplete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_SetupComplete_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_AuRand_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 TransID);
NTSTATUS Send_InRand_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_CombKey_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_EnModReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_EnKeySizeReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid, UINT8 keysize);
NTSTATUS Send_StartEnReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_StopEnReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_SCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_eSCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_SetAFH_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_mode, UINT8 flag);
//NTSTATUS Send_ChannelClassificationReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_reporting_mode);
NTSTATUS Send_ChannelClassification_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ChannelAssessmentMode);
NTSTATUS Send_TimingAccuracyReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_AutoRate_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_MaxSlot_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_QualityService_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 pollinterval, UINT8 Nbc, UINT8 flag);
NTSTATUS Send_SniffReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_HoldReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 HoldTime, UINT8 transid);
NTSTATUS Send_SlotOffset_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_SwitchReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_PowerControl_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 direction, UINT8 transid);
NTSTATUS Send_UnsniffReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_FeatureReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
NTSTATUS Send_PacketTypeTableReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 EdrMode);
NTSTATUS Send_FeatureReqExt_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 page_number);
NTSTATUS Send_PauseEncryptionReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);
NTSTATUS Send_ResumeEncryptionReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid);

NTSTATUS LMP_Unknown_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                /* not specified in protocol */
NTSTATUS LMP_name_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 1  */
NTSTATUS LMP_name_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 2  */
NTSTATUS LMP_accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 3  */
NTSTATUS LMP_not_accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 4  */
NTSTATUS LMP_clkoffset_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 5  */
NTSTATUS LMP_clkoffset_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 6  */
NTSTATUS LMP_detach(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                     /* 7  */
NTSTATUS LMP_in_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                    /* 8  */
NTSTATUS LMP_comb_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 9  */
NTSTATUS LMP_unit_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 10 */
NTSTATUS LMP_au_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                    /* 11 */
NTSTATUS LMP_sres(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                       /* 12 */
NTSTATUS LMP_temp_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 13 */
NTSTATUS LMP_temp_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 14 */
NTSTATUS LMP_encryption_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 15 */
NTSTATUS LMP_encryption_key_size_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);    /* 16 */
NTSTATUS LMP_start_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);       /* 17 */
NTSTATUS LMP_stop_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 18 */
NTSTATUS LMP_switch_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                 /* 19 */
NTSTATUS LMP_hold(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                       /* 20 */
NTSTATUS LMP_hold_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 21 */
                                                                                                                                               /* 22 */
NTSTATUS LMP_sniff_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 23 */
NTSTATUS LMP_unsniff_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                /* 24 */
NTSTATUS LMP_park_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 25 */
                                                                                                                                               /* 26 */
NTSTATUS LMP_set_broadcast_scan_window(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);  /* 27 */
NTSTATUS LMP_modify_beacon(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 28 */
NTSTATUS LMP_unpark_BD_ADDR_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);         /* 29 */
NTSTATUS LMP_unpark_PM_ADDR_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);         /* 30 */
NTSTATUS LMP_incr_power_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);             /* 31 */
NTSTATUS LMP_decr_power_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);             /* 32 */
NTSTATUS LMP_max_power(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 33 */
NTSTATUS LMP_min_power(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 34 */
NTSTATUS LMP_auto_rate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 35 */
NTSTATUS LMP_preferred_rate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);             /* 36 */
NTSTATUS LMP_version_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                /* 37 */
NTSTATUS LMP_version_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                /* 38 */
NTSTATUS LMP_features_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 39 */
NTSTATUS LMP_features_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 40 */
NTSTATUS LMP_quality_of_service(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);         /* 41 */
NTSTATUS LMP_quality_of_service_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);     /* 42 */
NTSTATUS LMP_SCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 43 */
NTSTATUS LMP_remove_SCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 44 */
NTSTATUS LMP_max_slot(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                   /* 45 */
NTSTATUS LMP_max_slot_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 46 */
NTSTATUS LMP_timing_accuracy_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 47 */
NTSTATUS LMP_timing_accuracy_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 48 */
NTSTATUS LMP_setup_complete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);             /* 49 */
NTSTATUS LMP_use_semi_permanent_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);     /* 50 */
NTSTATUS LMP_host_connection_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 51 */
NTSTATUS LMP_slot_offset(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                /* 52 */
NTSTATUS LMP_page_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 53 */
NTSTATUS LMP_page_scan_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);         /* 54 */
NTSTATUS LMP_supervision_timeout(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 55 */
NTSTATUS LMP_test_activate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 56 */
NTSTATUS LMP_test_control(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 57 */
NTSTATUS LMP_encryption_key_size_mask_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength); /* 58 */
NTSTATUS LMP_encryption_key_size_mask_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength); /* 59 */
NTSTATUS LMP_set_AFH(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                      /* 60 */
NTSTATUS LMP_encapsulated_header(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);          /* 61 */
NTSTATUS LMP_encapsulated_payload(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);         /* 62 */
NTSTATUS LMP_simple_pairing_confirm(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);       /* 63 */
NTSTATUS LMP_simple_pairing_number(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 64 */
NTSTATUS LMP_DHkey_check(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                  /* 65 */
NTSTATUS LMP_accepted_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);               /* 127/01 */
NTSTATUS LMP_not_accepted_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);           /* 127/02 */
NTSTATUS LMP_features_req_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);           /* 127/03 */
NTSTATUS LMP_features_res_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);           /* 127/04 */
NTSTATUS LMP_packet_type_table_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);      /* 127/11 */
NTSTATUS LMP_eSCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);              /* 127/12 */
NTSTATUS LMP_remove_eSCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);       /* 127/13 */
NTSTATUS LMP_channel_classification_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength); /* 127/16 */
NTSTATUS LMP_channel_classification(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);     /* 127/17 */
NTSTATUS LMP_sniff_subrating_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 127/21 */
NTSTATUS LMP_sniff_subrating_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);        /* 127/22 */
NTSTATUS LMP_pause_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);       /* 127/23 */
NTSTATUS LMP_resume_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);      /* 127/24 */
NTSTATUS LMP_IO_capability_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);          /* 127/25 */
NTSTATUS LMP_IO_capability_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);          /* 127/26 */
NTSTATUS LMP_numeric_comparison_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);  /* 127/27 */
NTSTATUS LMP_passkey_entry_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);       /* 127/28 */
NTSTATUS LMP_OOB_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);                 /* 127/29 */
NTSTATUS LMP_keypress_notification(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength);      /* 127/30 */
/*--file local variables-----------------------------------------------*/

const UINT8 Remote_Feature_Default[] = 
{
	0xFF, 0xFF, 0x8F, 0xFE, 0x9B, 0xF9, 0x00, 0x80
};

const UINT8 SCO_PacketType_Transform[] = 
{
/*  0: HV1             1: HV2             2: HV3 */
	BT_SCO_PACKET_HV1, BT_SCO_PACKET_HV2, BT_SCO_PACKET_HV3
};

const UINT8 PDU_OPCode_in_TestMode[] = 
{
	4, 7, 31, 32, 33, 34, 39, 56, 57, 60, ESCAPE_OPCODE4 + 16
};

/* the suffix of this array indicates the opcode of PDU procedure, 
   the content indicates the function processing different PDU received, 
   and the length of different PDU, and opcode of the response PDU. */
static LMP_PDU_RX_T LMP_PDU_Rx_Data[] = 
{
    {LMP_Unknown_PDU,                  0,    0 },  /* 0  */   /*no this opcode*/
    {LMP_name_req,                     2,    2 },  /* 1  */
    {LMP_name_res,                     17,   1 },  /* 2  */
    {LMP_accepted,                     2,    0 },  /* 3  */
    {LMP_not_accepted,                 3,    0 },  /* 4  */
    {LMP_clkoffset_req,                1,    6 },  /* 5  */
    {LMP_clkoffset_res,                3,    0 },  /* 6  */
    {LMP_detach,                       2,    0 },  /* 7  */
    {LMP_in_rand,                      17,   0 },  /* 8  */
    {LMP_comb_key,                     17,   0 },  /* 9  */
    {LMP_unit_key,                     17,   0 },  /* 10 */
    {LMP_au_rand,                      17,   12},  /* 11 */
    {LMP_sres,                         5,    0 },  /* 12 */
    {LMP_temp_rand,                    17,   0 },  /* 13 */
    {LMP_temp_key,                     17,   0 },  /* 14 */
    {LMP_encryption_mode_req,          2,    0 },  /* 15 */
    {LMP_encryption_key_size_req,      2,    0 },  /* 16 */
    {LMP_start_encryption_req,         17,   0 },  /* 17 */
    {LMP_stop_encryption_req,          1,    0 },  /* 18 */
    {LMP_switch_req,                   5,    0 },  /* 19 */
    {LMP_hold,                         7,    0 },  /* 20 */
    {LMP_hold_req,                     7,    0 },  /* 21 */
    {LMP_Unknown_PDU,                  0,    0 },  /* 22 */   /*no this opcode*/
    {LMP_sniff_req,                    10,   0 },  /* 23 */
    {LMP_unsniff_req,                  1,    0 },  /* 24 */
    {LMP_park_req,                     17,   0 },  /* 25 */
    {LMP_Unknown_PDU,                  0,    0 },  /* 26 */   /*no this opcode*/
    {LMP_set_broadcast_scan_window,    6,    0 },  /* 27 */   /*the length is 4 or 6*/                              
    {LMP_modify_beacon,                13,   0 },  /* 28 */   /*the length is 11 or 13*/                            
    {LMP_unpark_BD_ADDR_req,           17,   0 },  /* 29 */   /*the length is variable, 17 if unparks two slaves*/  
    {LMP_unpark_PM_ADDR_req,           15,   0 },  /* 30 */   /*the length is variable, 15 if unparks seven slaves*/
    {LMP_incr_power_req,               2,    0 },  /* 31 */
    {LMP_decr_power_req,               2,    0 },  /* 32 */
    {LMP_max_power,                    1,    0 },  /* 33 */
    {LMP_min_power,                    1,    0 },  /* 34 */
    {LMP_auto_rate,                    1,    0 },  /* 35 */
    {LMP_preferred_rate,               2,    0 },  /* 36 */
    {LMP_version_req,                  6,    38},  /* 37 */
    {LMP_version_res,                  6,    0 },  /* 38 */
    {LMP_features_req,                 9,    40},  /* 39 */
    {LMP_features_res,                 9,    0 },  /* 40 */
    {LMP_quality_of_service,           4,    0 },  /* 41 */
    {LMP_quality_of_service_req,       4,    0 },  /* 42 */
    {LMP_SCO_link_req,                 7,    0 },  /* 43 */
    {LMP_remove_SCO_link_req,          3,    0 },  /* 44 */
    {LMP_max_slot,                     2,    0 },  /* 45 */
    {LMP_max_slot_req,                 2,    0 },  /* 46 */
    {LMP_timing_accuracy_req,          1,    0 },  /* 47 */
    {LMP_timing_accuracy_res,          3,    0 },  /* 48 */
    {LMP_setup_complete,               1,    0 },  /* 49 */
    {LMP_use_semi_permanent_key,       1,    0 },  /* 50 */
    {LMP_host_connection_req,          1,    0 },  /* 51 */
    {LMP_slot_offset,                  9,    0 },  /* 52 */
    {LMP_page_mode_req,                3,    0 },  /* 53 */
    {LMP_page_scan_mode_req,           3,    0 },  /* 54 */
    {LMP_supervision_timeout,          3,    0 },  /* 55 */
    /*-------------below for LMP V1.2, V2.0 and V2.1 -------------*/
    {LMP_test_activate,                1,    0 },  /* 56 */
    {LMP_test_control,                 10,   0 },  /* 57 */
    {LMP_encryption_key_size_mask_req, 1,    0 },  /* 58 */
    {LMP_encryption_key_size_mask_res, 3,    0 },  /* 59 */
    {LMP_set_AFH,                      16,   0 },  /* 60 */
    {LMP_encapsulated_header,          4,    0 },  /* 61 */
    {LMP_encapsulated_payload,         17,   0 },  /* 62 */
    {LMP_simple_pairing_confirm,       17,   0 },  /* 63 */
    {LMP_simple_pairing_number,        17,   0 },  /* 64 */
    {LMP_DHkey_check,                  17,   0 },  /* 65 */
    /* Escape opcode 127 below */
    {LMP_accepted_ext,                 4,    0 },  /* 127/01 */
    {LMP_not_accepted_ext,             5,    0 },  /* 127/02 */
    {LMP_features_req_ext,             12,   0 },  /* 127/03 */
    {LMP_features_res_ext,             12,   0 },  /* 127/04 */
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/05 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/06 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/07 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/08 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/09 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/10 */   /*no this opcode*/
    {LMP_packet_type_table_req,        3,    0 },  /* 127/11 */   /* just for LMP V2.0 */
    {LMP_eSCO_link_req,                16,   0 },  /* 127/12 */
    {LMP_remove_eSCO_link_req,         4,    0 },  /* 127/13 */
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/14 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/15 */   /*no this opcode*/
    {LMP_channel_classification_req,   7,    0 },  /* 127/16 */
    {LMP_channel_classification,       12,   0 },  /* 127/17 */
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/18 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/19 */   /*no this opcode*/
    {LMP_Unknown_PDU,                  0,    0 },  /* 127/20 */   /*no this opcode*/
	/*----------------below for LMP V2.1  ------------------------*/
    {LMP_sniff_subrating_req,          9,    0 },  /* 127/21 */
    {LMP_sniff_subrating_res,          9,    0 },  /* 127/22 */
    {LMP_pause_encryption_req,         2,    0 },  /* 127/23 */
    {LMP_resume_encryption_req,        2,    0 },  /* 127/24 */
    {LMP_IO_capability_req,            5,    0 },  /* 127/25 */
    {LMP_IO_capability_res,            5,    0 },  /* 127/26 */
    {LMP_numeric_comparison_failed,    2,    0 },  /* 127/27 */
    {LMP_passkey_entry_failed,         2,    0 },  /* 127/28 */
    {LMP_OOB_failed,                   2,    0 },  /* 127/29 */
    {LMP_keypress_notification,        3,    0 }   /* 127/30 */
};

/*--file local function prototypes-------------------------------------*/




/*
 * Dump the Lmp Pdu for debug using
 */
void Lmp_Debug_Dump(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConn, 
                    PUINT8 pLmpPdu, UINT8 PduLength, UINT8 tx)
{
    struct sk_buff *dskb;
	struct lmp_dump *pDump;
    
	if(!PduLength){
		BT_DBGEXT(ZONE_LMP | LEVEL3, "0 Length Lmp, Ignore!!!!\n");
		return;
	}
    
    if(skb_queue_len(&devExt->debug.lmpDumpQ) > 100){
		BT_DBGEXT(ZONE_LMP | LEVEL1, "Lmp Dump Buffer Full\n");
		return;
	}

    dskb = alloc_skb(sizeof(*pDump), GFP_ATOMIC);
    pDump = (struct lmp_dump *)skb_put(dskb, sizeof(*pDump));
    RtlZeroMemory(pDump, sizeof(*pDump));
    
	if(!pConn || !pConn->valid_flag){
		BT_DBGEXT(ZONE_LMP | LEVEL1, "Invalid Connection\n");
		pDump->hConn = 0xFFFF;
	}
	else{
		pDump->hConn = pConn->connection_handle;
	}
	pDump->tx = tx;
	/**
	kt = ktime_get_real();
	pDump->tsec = *((UINT32 *)&kt.tv64 + 1);
	pDump->tms = (*(UINT32 *)&kt.tv64) / 1000 / 1000;
	**/
    pDump->tsec = (jiffies_to_msecs(jiffies) / 1000);
    pDump->tms = (jiffies_to_msecs(jiffies) % 1000);
    
    RtlCopyMemory(&pDump->lmpPdu, pLmpPdu, PduLength);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "OpCode 0x%04x, TID %d\n", pDump->lmpPdu.OpCode, pDump->lmpPdu.TransID);

    skb_queue_tail(&devExt->debug.lmpDumpQ, dskb);
    // Wake -up
	devExt->debug.lmpEvtFlag = 1;
	wake_up_interruptible(&devExt->debug.lmpDumpEvt);
}




/*************************************************************
 *   LMP_PDU_LC2LM_Process
 *
 *   Descriptions:
 *      Handle the PDUs received from LC.
 *
 *   Arguments:
 *      devExt:         IN, pointer to the device.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      pLmpPdu:        IN, pointer to the PDU received.
 *      PduLength:      IN, the length of PDU.
 *
 *   Return Value: 
 *      STATUS_SUCCESS:              if PDU is processed successfully.
 *      STATUS_INFO_LENGTH_MISMATCH: if the PduLength is wrong.
 *      RPC_NT_NULL_REF_POINTER:     if the IN pointer is NULL.
 *      STATUS_INVALID_PARAMETER:    if PDU's opcode is out range.
 *      ......
 *      STATUS_UNSUCCESSFUL:         all the remain of unsuccessful situation.
 *************************************************************/
NTSTATUS LMP_PDU_LC2LM_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength)
{
	NTSTATUS status;
	UINT8    i, TempOPCode;
	LMP_PUD_PACKAGE_T LocalPdu;
	
	if (devExt == NULL || pConnectDevice == NULL || pLmpPdu == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_PDU_LC2LM_Process function operates ERROR, devExt= 0x%p, pConnectDevice= 0x%p, pLmpPdu= 0x%p\n", devExt, pConnectDevice, pLmpPdu);
		return RPC_NT_NULL_REF_POINTER;
	}
	
	if (PduLength < MIN_PDU_LENGTH || PduLength > MAX_PDU_LENGTH)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_PDU_LC2LM_Process function operates ERROR (BD0=0x%x), PduLength= %d\n", pConnectDevice->bd_addr[0], PduLength);
		return STATUS_INFO_LENGTH_MISMATCH;
	}


#ifdef LMP_DEBUG_DUMP
	Lmp_Debug_Dump(devExt, pConnectDevice, pLmpPdu, PduLength, 0);
#endif

	
	RtlCopyMemory(&LocalPdu, pLmpPdu, PduLength);
	
	if (((PBT_HCI_T)devExt->pHci)->test_mode_active == 1)
	{
		if (LocalPdu.OpCode > ESCAPE_OPCODE1)
			TempOPCode = LocalPdu.OpCode + LocalPdu.contents[0];
		else
			TempOPCode = LocalPdu.OpCode;
		
		for (i = 0; i < sizeof(PDU_OPCode_in_TestMode); i++)
		{
			if (TempOPCode == PDU_OPCode_in_TestMode[i])
				break;
		}
		if (i >= sizeof(PDU_OPCode_in_TestMode))
		{
			BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Out range PDU Received in TsetMode (BD0=0x%x), OPCode= %d, contents[0]= %d\n", pConnectDevice->bd_addr[0], LocalPdu.OpCode, LocalPdu.contents[0]);
			return STATUS_INVALID_PARAMETER;
		}
	}
	
	if (LocalPdu.OpCode > MAX_NORMAL_OPCODE && LocalPdu.OpCode < ESCAPE_OPCODE4)
	{
		status = LMP_Unknown_PDU(devExt, pConnectDevice, &LocalPdu, PduLength);
		return status;
	}
	
	if (LocalPdu.OpCode == ESCAPE_OPCODE4)
		status = LMP_PDU_Rx_Data[LocalPdu.contents[0] + ESCAPE_OPCODE4_OFFSET].Rx_Fun(devExt, pConnectDevice, &LocalPdu, PduLength);
	else
		status = LMP_PDU_Rx_Data[LocalPdu.OpCode].Rx_Fun(devExt, pConnectDevice, &LocalPdu, PduLength);
	
	return status;
}

NTSTATUS LMP_Unknown_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                /* not specified in protocol */
{
	NTSTATUS status;
	
	PduLength = PduLength;
	if (pLmpPdu->OpCode < ESCAPE_OPCODE1)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Unknown PDU OPCode: %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->OpCode);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNKNOWN_LMP_PDU);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Unknown PDU EscapeOPCode: %d, ExtendedOPCode: %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->OpCode, pLmpPdu->contents[0]);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNKNOWN_LMP_PDU);
	}

	return status;
}

/*------Name Request Transaction-----------------------------------*/
NTSTATUS LMP_name_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 1  */
{
	NTSTATUS status;
	UINT8    ErrReason;
	UINT8    ResLen;
	PUINT8   localname;
	UINT8    n;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Name Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Name Req, NameOffset= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	localname = ((PBT_HCI_T)devExt->pHci)->local_name;
	ResLen    = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;
	pdu.contents[0] = pLmpPdu->contents[0];
	pdu.contents[1] = (UINT8)strlen(localname);
	
	if (pdu.contents[0] >= pdu.contents[1])
	{
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	for (n = 0; n < 14; n++)
	{
		if (n + pdu.contents[0] < pdu.contents[1])
			pdu.contents[2 + n] = *(localname + pdu.contents[0] + n);
		else
			pdu.contents[2 + n] = 0;
	}
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Name Res\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	return status;
}

NTSTATUS LMP_name_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 2  */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    ResLen;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Name Res, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Name Res, NameOffset= %d, NameLength= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0], pLmpPdu->contents[1]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_NAME_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	if (pLmpPdu->contents[0] + 14 < pLmpPdu->contents[1])
	{
		RtlCopyMemory(pConnectDevice->remote_name + pLmpPdu->contents[0], &(pLmpPdu->contents[2]), 14);
		
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;
		pdu.contents[0] = (UINT8)(pLmpPdu->contents[0] + 14);
		ResLen = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
		if (NT_SUCCESS(status))
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Name Req, NameOffset= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);
			if (pConnectDevice->lmp_timer_valid == 0)
				LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_NAME_REQ, LMP_NAME_REQUEST_TIME);
		}
	}
	else
	{
		if (pLmpPdu->contents[0] >= pLmpPdu->contents[1] || pLmpPdu->contents[1] > BT_LOCAL_NAME_LENGTH)
		{
			pConnectDevice->current_reason_code = INVALID_LMP_PARAMETERS;
			*pConnectDevice->remote_name        = 0;
			status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		}
		else
		{
			RtlCopyMemory(pConnectDevice->remote_name + pLmpPdu->contents[0], &(pLmpPdu->contents[2]), pLmpPdu->contents[1] - pLmpPdu->contents[0]);
			*(pConnectDevice->remote_name + pLmpPdu->contents[1]) = 0;
		}
		
		if (pConnectDevice->tempflag == 1)
			status = ResLmpDetach(devExt, pConnectDevice, TERMINATED_LOCAL_HOST);
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_NAME_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return status;
}

NTSTATUS LMP_accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 3  */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER     timevalue;
	LMP_PUD_PACKAGE_T pdu;
	UINT8  tempvar = 0, buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	UINT16 value_temp = 0;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Accepted, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}

	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	switch (pLmpPdu->contents[0])
	{
		case 51: /* Create Connection */
			if ((pConnectDevice->lmp_states & LMP_SEND_CONN_REQ) == 0)
				break;
			
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_CONN_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Connection\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);

			if (((PBT_HCI_T)devExt->pHci)->authentication_enable == BT_AUTHENTICATION_ENABLE)
			{
				status = Authentication_Process(devExt, pConnectDevice);
				break;
			}

			status = Send_SetupComplete_PDU(devExt, pConnectDevice);
			break;

		case 43: /* SCO Link Req */
			if (pConnectDevice->current_role != BT_ROLE_MASTER || (pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_SCO_LINK_REQ 
				|| ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SCOLINK_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted SCO Link Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;

			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco               = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco;
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode            = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].air_mode;
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type = SCO_PacketType_Transform[((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_packet];
			
			tempvar = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].timing_control_flags;
			tempvar = ((((tempvar >> 1) & 0x01) + 1) << 6) + ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco;
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_D_SCO, pBuf, 1, &tempvar);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_T_SCO, pBuf, 1, (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_packet + 1) << 1);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AM_SCO, pBuf, 1, pConnectDevice->am_addr + (1 << 4));
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);

			((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
			if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_ADD_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
				((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) + 1) << 6);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_CONNECTED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV3)
					((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag = 0;
				else
					((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag = 1;
				if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV1)
					((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag = 1;
				else
					((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag = 0;
				
				Frag_InitScoRxBuffer(devExt, pConnectDevice);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CHANGE_SCO_CONNECTION_BIT + BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_CHANGE_LINK)
				{
					((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->changed_packet_type = 0;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = BT_HCI_STATUS_SUCCESS;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}
			
			break;

		case 44: /* Remove SCO Link Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_REMOVE_SCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_SCOLINK_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Remove SCO Link Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
#ifdef BT_TESTMODE_SUPPORT
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
			DeleteSCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;

		case 8: /* InRand */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_INRAND)
				break;

			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_INRAND */
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_INRAND)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Initialization Rand\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;

			status = E22_InitAndMaster_KeyGen(pConnectDevice->random, pConnectDevice->pin_code, pConnectDevice->pin_code_length, pConnectDevice->bd_addr, pConnectDevice->init_key);
			if (!NT_SUCCESS(status))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----E22_InitAndMaster_KeyGen function operates ERROR\n");
				pConnectDevice->current_reason_code = AUTHENTICATION_FAILURE;
				status = ResLmpDetach(devExt, pConnectDevice, AUTHENTICATION_FAILURE);
				break;
			}

			if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
				status = Send_CombKey_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			else
				ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_COM_KEY);

			break;

		case 15: /* EnModeReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENMODE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_ENKEYSIZE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STOPEN_REQ)
				break;
			
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ENMODE_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted EncryptionModeReq\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
			{
				if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
					status = Send_EnKeySizeReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID, MAX_EN_KEY_SIZE);
				else
					status = Send_StopEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			}
			else
			{
				if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
					ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_ENKEYSIZE_REQ);
				else
					ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STOPEN_REQ);

				status = STATUS_SUCCESS;
			}

			break;

		case 16: /* EnKeySizeReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENKEYSIZE_REQ)
				break;

			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ENKEYSIZE_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted EncryptionKeySizeReq\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;

			if (pConnectDevice->current_role == BT_ROLE_MASTER)
				status = Send_StartEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			else
				ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STARTEN_REQ);

			break;

		case 17: /* StartEnReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_STARTEN_REQ || pConnectDevice->current_role != BT_ROLE_MASTER)
				break;

			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_STARTEN_REQ */
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_STARTEN_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted StartEncryptionReq\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			/* Write EncryptionEnable and SetConnectionEncryption register to hardware */
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, (BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr), pBuf, 1, 3);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			
			pConnectDevice->is_in_encryption_process = 0;
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
			{
				if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
					status = Send_SetupComplete_PDU(devExt, pConnectDevice);
			}
			else
			{
				if (pConnectDevice->pause_encryption_status == 0)
				{
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					pConnectDevice->pause_encryption_status = 0;
					pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}

			//Jakio, check tx queue
			timevalue.QuadPart = 0;
			//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
			UsbQueryDMASpace(devExt);
			break;
			
		case 18: /* StopEnReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_STOPTEN_REQ || pConnectDevice->current_role != BT_ROLE_MASTER)
				break;

			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_STOPTEN_REQ */
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_STOPTEN_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted StopEncryptionReq\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;

			/* Write EncryptionEnable and SetConnectionEncryption register to hardware */
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, (BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr), pBuf, 1, 0);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			
			if (pConnectDevice->pause_encryption_status == 0)
			{
				pConnectDevice->is_in_encryption_process = 0;
				
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				//Jakio20071220, check tx queue
				timevalue.QuadPart = 0;
				//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
				UsbQueryDMASpace(devExt);
			}
			else
			{
				if (pConnectDevice->pause_encryption_status == 2 && pLmpPdu->TransID == MASTER_TRANSACTION_ID)
				{
					if (pConnectDevice->pause_command_flag == 1)
					{
						status = Send_SwitchReq_PDU(devExt, pConnectDevice);
					}
					else
					{
						pConnectDevice->encryption_mode = BT_ENCRYPTION_ONLY_P2P;
						status = Send_StartEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
					}
				}
			}
			break;
			
		case 19: /* Switch Req */
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_SWITCH_REQ) == 0)
				break;
			
			CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_SWITCH_REQ, 1);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Switch Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			break;
			
		case 21: /* Hold Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_HOLD_REQ)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_HOLD_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Hold Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_HOLD_ACCEPTED);
			pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;

		case 23: /* Sniff Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_SNIFF_REQ || ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SNIFF_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Sniff Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			((PBT_LMP_T)devExt->pLmp)->sniff_number++;
			Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
			LMP_Task_EnterSniff(devExt, (PUINT8)&pConnectDevice);
			break;
			
		case 24: /* Unsniff Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_UNSNIFF_REQ || ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_UNSNIFF_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Unsniff Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			LMP_Task_LeaveSniff(devExt, (PUINT8)&pConnectDevice);
			break;
			
		case 42: /* Quality of Service Req */
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Quality of Service Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			pConnectDevice->qos_flags           = 0;
			pConnectDevice->qos_service_type    = 1; /* Best Effort */
			pConnectDevice->qos_token_rate      = 0;
			pConnectDevice->qos_peak_bandwidth  = 0;
			pConnectDevice->qos_latency         = 25000; /* ms */
			pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_QOS_REQ) == 0)
			{
				pConnectDevice->qos_token_bucket_size = 0;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_QOS_REQ, 1);
				pConnectDevice->qos_delay_variation = 0xFFFFFFFF;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_QOS_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			break;
			
		case 46: /* Max Slot Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_MAX_SLOT_REQ)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_MAX_SLOT_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Max Slot Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
			if (pConnectDevice->changed_max_slot != pConnectDevice->tx_max_slot)
			{
				pConnectDevice->tx_max_slot = pConnectDevice->changed_max_slot;
				devExt->AllowIntSendingFlag = FALSE;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_TX_MAX_SLOT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				devExt->AllowIntSendingFlag = TRUE;
				pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
				Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
				Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case 56: /* Test Activate */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_TEST_ACTIVATE || pConnectDevice->current_role != BT_ROLE_MASTER)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Test Activate\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;
			
#ifdef BT_TESTMODE_SUPPORT
			((PBT_TESTMODE_T)devExt->pTestMode)->role = 1;
#endif			
			break;

		case 57: /* Test Control */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_TEST_CONTROL || pConnectDevice->current_role != BT_ROLE_MASTER)
				break;

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Accepted Test Control\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
			status = STATUS_SUCCESS;

#ifdef BT_TESTMODE_SUPPORT
			tempvar = (((PBT_TESTMODE_T)devExt->pTestMode)->configuration).test_scenario;
			
			if (tempvar == 1 || tempvar == 2 || tempvar == 3 || tempvar == 4 || tempvar == 7 || tempvar == 8 || tempvar == 9)
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x80);
			else if (tempvar == 5 || tempvar == 6)
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x90);
			else if (tempvar == 255)
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x10);
			else
				break;
			
			if (tempvar == 255)
			{
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOPPING_MODE, pBuf, 1, 1);
			}
			else
			{
				pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOPPING_MODE, pBuf, 1, &((((PBT_TESTMODE_T)devExt->pTestMode)->configuration).hopping_mode));
				pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TX_FREQUENCY_DUT, pBuf, 1, &((((PBT_TESTMODE_T)devExt->pTestMode)->configuration).RX_frequency));
				pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_RX_FREQUENCY_DUT, pBuf, 1, &((((PBT_TESTMODE_T)devExt->pTestMode)->configuration).TX_frequency));
			}
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_TYPE_DUT, pBuf, 1, &((((PBT_TESTMODE_T)devExt->pTestMode)->configuration).packet_type));
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			TestMode_WriteCommandIndicator(devExt);
#endif
			break;
			
		default :
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Accepted PDU (TransID=%d, BD0=0x%x): opcode=%d, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
			status = STATUS_SUCCESS;
			break;
	}

	if (!NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Accepted PDU (TransID=%d, BD0=0x%x): opcode=%d, unexpected or operate Error\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);

	return status;
}

NTSTATUS LMP_not_accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 4  */
{
	UINT8 ErrReason, tempvalue = 0;
	LARGE_INTEGER timevalue;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	//tom:0409
	UINT8		ListIndex;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Not Accepted, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	switch (pLmpPdu->contents[0])
	{
		case 51: /* Create Connection */
			if ((pConnectDevice->lmp_states & LMP_SEND_CONN_REQ) == 0)
				break;

			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_CONN_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Connection, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);

			pConnectDevice->current_reason_code = pLmpPdu->contents[1];
			status = ResLmpDetach(devExt, pConnectDevice, pLmpPdu->contents[1]);
			break;

		case 43: /* SCO Link Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_SCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SCOLINK_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted SCO Link Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_ADD_LINK)
			{
				if (pConnectDevice->current_role == BT_ROLE_SLAVE)
				{
					RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_SCO_T));
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					ErrReason = SCONotAccepted_Process(devExt, pConnectDevice, pLmpPdu->contents[1]);
					if (ErrReason == 2)
					{
						status = Send_SCOLinkReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
					}
					else
					{
						((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
						RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_SCO_T));
						((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					}
				}
			}
			else if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_CHANGE_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = pLmpPdu->contents[1];
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			break;

		case 44: /* Remove SCO Link Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_REMOVE_SCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_SCOLINK_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Remove SCO Link Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
#ifdef BT_TESTMODE_SUPPORT
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			DeleteSCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;

		case 11: /* AuRand */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_AURAND && 
				((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) < LMP_SEND_ENMODE_REQ || 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) > LMP_SEND_STOPTEN_REQ))
				break;

			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND)
				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_AURAND */
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_AURAND)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Authentication, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			
			if (pLmpPdu->contents[1] == ERROR_TRANSACTION_COLLISION)
			{
				if (pConnectDevice->current_role != BT_ROLE_SLAVE)
					break;

				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) == 0)
				{
					status = Send_AuRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
					break;
				}

				if ((pConnectDevice->lmp_states & LMP_AU_COLLISION_OR_SEND_SRES) == LMP_AU_COLLISION_OR_SEND_SRES)
				{
					/* means already sent Secret Response then send au_rand again */
					status = Send_AuRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
					
					CancelLmpBitState(devExt, pConnectDevice, LMP_AU_COLLISION_OR_SEND_SRES, 0);
				}
				else
				{
					/* means received NotAccepted PDU LMP_AU_COLLISION */
					ChangeLmpState(devExt, pConnectDevice, LMP_AU_COLLISION_OR_SEND_SRES);
					pConnectDevice->init_key[0] = pLmpPdu->TransID; /* init_key[0] holds the TransID casually */
					status = STATUS_SUCCESS;
				}
			}
			else if (pLmpPdu->contents[1] == KEY_MISSING)
			{
				status = Pairing_Process(devExt, pConnectDevice);
			}
			else if (pLmpPdu->contents[1] == PDU_NOT_ALLOWED)
			{
				if ((pConnectDevice->lmp_states & LMP_AU_ENCRYPTION_COLLISION) == LMP_AU_ENCRYPTION_COLLISION)
					CancelLmpBitState(devExt, pConnectDevice, LMP_AU_ENCRYPTION_COLLISION, 0);
				
				status = STATUS_SUCCESS;
			}

			break;

		case 8: /* InRand */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_INRAND && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_COM_KEY)
				break;

			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_INRAND)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			
			status = STATUS_SUCCESS;
			if (pLmpPdu->contents[1] == PAIRING_NOT_ALLOWED)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Initialization Rand, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				{
					pConnectDevice->current_reason_code = PAIRING_NOT_ALLOWED;
					devExt->AllowIntSendingFlag = FALSE;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PAIR_NOT_ALLOW, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					status = ResLmpDetach(devExt, pConnectDevice, PAIRING_NOT_ALLOWED);
					
					devExt->AllowIntSendingFlag = TRUE;
					timevalue.QuadPart = 0;
					tasklet_schedule(&devExt->taskletRcv);
				}
				else
				{
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_INRAND */
					pConnectDevice->current_reason_code = KEY_MISSING;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}
			else if (pLmpPdu->contents[1] == ERROR_TRANSACTION_COLLISION)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Initialization Rand, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			}

			break;

		case 15: /* EnModeReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENMODE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_ENKEYSIZE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STOPEN_REQ)
				break;
			
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ENMODE_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted EncryptionModeReq, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;

			if (pLmpPdu->contents[1] == ERROR_TRANSACTION_COLLISION)
			{
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_ENMODE_REQ)
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_ENMODE_REQ */

				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0 && 
					(pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
					status = Send_SetupComplete_PDU(devExt, pConnectDevice);
			}
			else
			{
				pConnectDevice->is_in_encryption_process = 0;
				
				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_ENMODE_REQ */
				pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				{
					if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
						status = Send_SetupComplete_PDU(devExt, pConnectDevice);
				}
				else
				{
					pConnectDevice->current_reason_code = pLmpPdu->contents[1];
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}

				//Jakio20071220, check tx queue
				timevalue.QuadPart = 0;
				//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
				UsbQueryDMASpace(devExt);
			}

			break;

		case 16: /* EnKeySizeReq */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENKEYSIZE_REQ)
				break;

			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ENKEYSIZE_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted EncryptionKeySizeReq, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;

			pConnectDevice->is_in_encryption_process = 0;
			
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_ENKEYSIZE_REQ */
			pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
			{
				if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
					status = Send_SetupComplete_PDU(devExt, pConnectDevice);
			}
			else
			{
				pConnectDevice->current_reason_code = pLmpPdu->contents[1];
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}

			//Jakio20071220, check tx queue
			timevalue.QuadPart = 0;
			//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
			UsbQueryDMASpace(devExt);
			break;
			
		case 19: /* Switch Req */
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_SWITCH_REQ) == 0)
				break;
			
			CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_SWITCH_REQ, 1);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Switch Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			if ((((PBT_HCI_T)devExt->pHci)->role_switch_fail_count < 3) && (pConnectDevice->state == BT_DEVICE_STATE_SLAVE_CONNECTED))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Retry role switch!\n");
				break;
			}

			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CANCEL_ROLE_SWITCH);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));

			if (pConnectDevice->pause_encryption_status == 2)
			{
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_RESUME_ENCRYPTION), BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
				

			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));

			//tom:0409
			Frag_GetQueueHead(devExt, pConnectDevice, &ListIndex);
			Frag_StartQueue(devExt, ListIndex);

			break;
			
		case 21: /* Hold Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_HOLD_REQ)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			CancelLmpBitState(devExt, pConnectDevice, LMP_HOLD_ACCEPTED, 1);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_HOLD_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Hold Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CANCEL_HODE_MODE_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			
			pConnectDevice->mode_current_mode   = BT_MODE_CURRENT_MODE_ACTIVE;
			pConnectDevice->current_reason_code = pLmpPdu->contents[1];
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;

		case 23: /* Sniff Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_SNIFF_REQ || ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SNIFF_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Sniff Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			if (((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].init_flag == SNIFF_INIT_FLAG)
			{
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index], sizeof(BT_SNIFF_T));
				pConnectDevice->sniff_mode_interval = 0;
				pConnectDevice->current_reason_code = pLmpPdu->contents[1];
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case 24: /* Unsniff Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_UNSNIFF_REQ || ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_UNSNIFF_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Unsniff Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			Frag_ListStopSwitch(devExt, pConnectDevice, 0);
			
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_UNSNIFF_DETACH) == LMP_SEND_UNSNIFF_DETACH)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
				{
					Frag_FlushFragAndBD(devExt, NULL, FRAG_SNIFF_LIST);
					tempvalue = 1;
				}
				else
				{
					tempvalue = (UINT8)(pConnectDevice->slave_index << 1);
				}
				
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_AMADDR, pBuf, 1, (tempvalue << 4) + pConnectDevice->am_addr);
				pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				
				pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
				DeleteSniffLink(devExt, pConnectDevice);
				CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_DETACH, 1);
				status = ResLmpDetach(devExt, pConnectDevice, pConnectDevice->current_reason_code);
			}
			else
			{
				pConnectDevice->current_reason_code = pLmpPdu->contents[1];
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case 31: /* Increase Power Req */
			pConnectDevice->rx_power = RX_POWER_MAX_FLAG;
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Increase Power Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			break;

		case 32: /* Decrease Power Req */
			pConnectDevice->rx_power = RX_POWER_MIN_FLAG;
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Decrease Power Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			break;

		case 42: /* Quality of Service Req */
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Quality of Service Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			pConnectDevice->current_reason_code = pLmpPdu->contents[1];
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_QOS_REQ) == 0)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_FLOW_SPECIFICATION_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_QOS_REQ, 1);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_QOS_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			break;
			
		case 46: /* Max Slot Req */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_MAX_SLOT_REQ)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_MAX_SLOT_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Accepted Max Slot Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			
			pConnectDevice->current_reason_code = pLmpPdu->contents[1];
			Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		default :
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Not Accepted PDU (TransID=%d, BD0=0x%x): opcode=%d, reason= 0x%x, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0], pLmpPdu->contents[1]);
			status = STATUS_SUCCESS;
			break;
	}

	if (!NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Not Accepted PDU (TransID=%d, BD0=0x%x): opcode=%d, reason= 0x%x, unexpected or operate Error\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0], pLmpPdu->contents[1]);

	return status;
}

NTSTATUS LMP_clkoffset_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 5  */
{
	NTSTATUS status;
	UINT8  ErrReason;
	UINT8  ResLen;
	UINT16 clock_offset = 0;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Clock Offset Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if (pConnectDevice->current_role != BT_ROLE_SLAVE)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Clock Offset Req, local host is a master\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		ErrReason = PDU_NOT_ALLOWED;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Clock Offset Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	ResLen = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));

	pdu.TransID  = pLmpPdu->TransID;
	pdu.OpCode   = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;
	if (pConnectDevice->slave_index == 0)
		clock_offset = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_AM_CLK_OFFSET_SLAVE0);
	else
		clock_offset = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_AM_CLK_OFFSET_SLAVE1);
	RtlCopyMemory(pdu.contents, &clock_offset, sizeof(clock_offset));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Clock Offset Res, ClkOffset= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], clock_offset);
	return status;
}

NTSTATUS LMP_clkoffset_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 6  */
{
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Clock Offset Res, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		return STATUS_SUCCESS;
	}

	if (pConnectDevice->current_role != BT_ROLE_MASTER)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Clock Offset Res, local host is a slave\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		return STATUS_SUCCESS; /* ignore this res */
	}

	RtlCopyMemory(&pConnectDevice->clock_offset, pLmpPdu->contents, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Clock Offset Res, ClkOffset= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pConnectDevice->clock_offset);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_CLKOFFSET_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);

	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CLKOFFSET_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return STATUS_SUCCESS;
}

NTSTATUS LMP_detach(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                     /* 7  */
{
	NTSTATUS status;
	UINT8    ErrReason;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Detach, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Detach, reason: 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);	
	ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
	ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
	
#ifdef BT_TESTMODE_SUPPORT
	TestMode_ResetMembers(devExt, pConnectDevice);
#endif
	if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
	{
		LMP_ReleaseSniffMembers(devExt, pConnectDevice);
		pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
	}
	LMP_ReleaseSCOMembers(devExt, pConnectDevice);
	LMP_StopPDUTimer(devExt, pConnectDevice);
	
	pConnectDevice->current_reason_code = pLmpPdu->contents[0];
	pConnectDevice->is_in_disconnecting = 1;
	
	if (pLmpPdu->contents[0] == AUTHENTICATION_FAILURE)
	{
		devExt->AllowIntSendingFlag = FALSE;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		devExt->AllowIntSendingFlag = TRUE;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	else
	{
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return STATUS_SUCCESS;
}

NTSTATUS LMP_in_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                    /* 8  */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8    ErrReason, transid;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Initialization Rand, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Initialization Rand\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SETUP_COMPLETE)
		LMP_StopPDUTimer(devExt, pConnectDevice);

	/* means already received NotAccepted PDU LMP_AU_COLLISION then cancel LMP_AU_COLLISION_OR_SEND_SRES */
	if ((pConnectDevice->lmp_states & LMP_AU_COLLISION_OR_SEND_SRES) == LMP_AU_COLLISION_OR_SEND_SRES)
		CancelLmpBitState(devExt, pConnectDevice, LMP_AU_COLLISION_OR_SEND_SRES, 0);

	transid = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_INRAND && pLmpPdu->TransID != transid && pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		ErrReason = ERROR_TRANSACTION_COLLISION;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_INRAND)
		LMP_StopPDUTimer(devExt, pConnectDevice);

	if (((PBT_HCI_T)devExt->pHci)->pin_type == PIN_FIXED)
	{
		if (pLmpPdu->TransID == transid)
		{
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_INRAND */
			devExt->AllowIntSendingFlag = FALSE;
			status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PAIRING_NOT_ALLOWED);
			devExt->AllowIntSendingFlag = TRUE;
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
			{
				pConnectDevice->current_reason_code = PAIRING_NOT_ALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PAIR_NOT_ALLOW, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				pConnectDevice->current_reason_code = KEY_MISSING;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			return status;
		}
		else
		{
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_INRAND)
			{
				status = Send_InRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
				return status;
			}
		}
	}
	else
	{
		if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_INRAND)
		{
			RtlCopyMemory(pConnectDevice->link_key, pLmpPdu->contents, BT_LINK_KEY_LENGTH);
			status = Res_InRand_Accepted(devExt, pConnectDevice, pLmpPdu->TransID);
			return status;
		}
	}
	
	
	RtlCopyMemory(pConnectDevice->link_key, pLmpPdu->contents, BT_LINK_KEY_LENGTH); /* link_key holds the random casually */
	pConnectDevice->aco[0] = pLmpPdu->TransID; /* aco[0] holds the TransID casually */

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_PIN_REQ)
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PIN_CODE_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));

	ChangeLmpState(devExt, pConnectDevice, LMP_PIN_REQ_REC_INRAND);
	return status;
}

NTSTATUS LMP_comb_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 9  */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    i, RemoteUnitKey[BT_LINK_KEY_LENGTH], TempKey[BT_LINK_KEY_LENGTH];
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Combination Key, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Combination Key\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_COMB_KEY)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_COM_KEY && pConnectDevice->key_type_key != LINK_KEY_COMBINATION)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNIT_KEY_USED);
		return status;
	}
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_WAIT_COM_KEY)
		RtlCopyMemory(TempKey, pConnectDevice->init_key, BT_LINK_KEY_LENGTH);
	else
		RtlCopyMemory(TempKey, pConnectDevice->link_key, BT_LINK_KEY_LENGTH);
	
	if (pLmpPdu->TransID != ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = pLmpPdu->TransID;
		pdu.OpCode  = 9;
		for (i = 0; i < 16; i++)
		{
			pConnectDevice->random[i] = Rand_UCHAR_Gen();
			pdu.contents[i] = (UINT8)(pConnectDevice->random[i] ^ TempKey[i]);
		}
		
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Combination Key\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}
	
	E21_UnitAndCombination_KeyGen(pConnectDevice->random, ((PBT_HCI_T)devExt->pHci)->local_bd_addr, pConnectDevice->link_key);
	for (i = 0; i < 16; i++)
		pConnectDevice->random[i] = (UINT8)(pLmpPdu->contents[i] ^ TempKey[i]);
	E21_UnitAndCombination_KeyGen(pConnectDevice->random, pConnectDevice->bd_addr, RemoteUnitKey);
	
	pConnectDevice->key_type_key = LINK_KEY_COMBINATION;
	for (i = 0; i < 16; i++)
		pConnectDevice->link_key[i] = (UINT8)(pConnectDevice->link_key[i] ^ RemoteUnitKey[i]);
	
	ChangeLmpState(devExt, pConnectDevice, LMP_MUTUAL_AU);
	if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
		status = Send_AuRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
	else
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
	
	return status;
}

NTSTATUS LMP_unit_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 10 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    i;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Unit Key, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_COM_KEY)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----PDU Received (TransID=%d, BD0=0x%x): Unit Key, but unexpected\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		return STATUS_UNSUCCESSFUL;
	}

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Unit Key\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_COMB_KEY)
		LMP_StopPDUTimer(devExt, pConnectDevice);

	if (pLmpPdu->TransID != ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = pLmpPdu->TransID;
		pdu.OpCode  = 9;
		
		/* ignore random */
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Combination Key\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}

	pConnectDevice->key_type_key = LINK_KEY_REMOTEUNIT;
	for (i = 0; i < 16; i++)
		pConnectDevice->link_key[i] = (UINT8)(pLmpPdu->contents[i] ^ pConnectDevice->init_key[i]);

	ChangeLmpState(devExt, pConnectDevice, LMP_MUTUAL_AU);
	if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
		status = Send_AuRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
	else
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);

	return status;
}

NTSTATUS LMP_au_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                    /* 11 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 ErrReason, ResLen, i;
	LARGE_INTEGER timevalue;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Authentication Rand, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----PDU Received (TransID=%d, BD0=0x%x): Authentication Rand\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) >= LMP_WAIT_ENKEYSIZE_REQ && 
		(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ)
	{
		ErrReason = PDU_NOT_ALLOWED;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_PIN_REQ || 
		(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_INRAND)
	{
		ErrReason = KEY_MISSING;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if ((pConnectDevice->current_role == BT_ROLE_MASTER 
		&& ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND 
		    || (pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_NO_LINKKEY)) || 
		(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_NO_LINKKEY_REC_AURAND || 
		(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_REC_AURAND_NO_LINKKEY)
	{
		ErrReason = ERROR_TRANSACTION_COLLISION;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_NO_LINKKEY)
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_NO_LINKKEY_REC_AURAND);
		pConnectDevice->aco[0] = pLmpPdu->TransID; /* aco[0] holds the TransID casually */
		RtlCopyMemory(pConnectDevice->random, pLmpPdu->contents, BT_LINK_KEY_LENGTH);
		return status;
	}

	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SETUP_COMPLETE)
		LMP_StopPDUTimer(devExt, pConnectDevice);

#ifdef BT_SET_LINK_KEY_USING_APP
	RtlCopyMemory(pConnectDevice->link_key, ((PBT_HCI_T)devExt->pHci)->tmp_link_key, BT_LINK_KEY_LENGTH);
#endif

	for (i = 0; i < 16; i++)
	{
		if (pConnectDevice->link_key[i] != 0)
			break;
	}
	if (i == 16)
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_REC_AURAND_NO_LINKKEY);
		pConnectDevice->aco[0] = pLmpPdu->TransID; /* aco[0] holds the TransID casually */
		RtlCopyMemory(pConnectDevice->random, pLmpPdu->contents, BT_LINK_KEY_LENGTH);
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	else
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = pLmpPdu->TransID;
		pdu.OpCode  = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;
		ResLen = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;

		E1_Authentication_KeyGen(pConnectDevice->link_key, pLmpPdu->contents, ((PBT_HCI_T)devExt->pHci)->local_bd_addr, pdu.contents, pConnectDevice->aco);
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
		if (NT_SUCCESS(status))
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Secret Response\n", pdu.TransID, pConnectDevice->bd_addr[0]);
			
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
			{
				if ((pConnectDevice->lmp_states & LMP_AU_COLLISION_OR_SEND_SRES) == LMP_AU_COLLISION_OR_SEND_SRES)
				{
					/* means already received NotAccepted PDU LMP_AU_COLLISION then send aurand again */
					CancelLmpBitState(devExt, pConnectDevice, LMP_AU_COLLISION_OR_SEND_SRES, 0);
					status = Send_AuRand_PDU(devExt, pConnectDevice, pConnectDevice->init_key[0]);
					return status;
				}
				else if (pConnectDevice->current_role == BT_ROLE_SLAVE)
				{
					/* means sent Secret Response */
					ChangeLmpState(devExt, pConnectDevice, LMP_AU_COLLISION_OR_SEND_SRES);
				}
			}

			if ((pConnectDevice->lmp_states & LMP_MUTUAL_AU) == LMP_MUTUAL_AU)
			{
				if ((pConnectDevice->lmp_states & LMP_RECV_SRES_OR_SEND_SRES) == LMP_RECV_SRES_OR_SEND_SRES) /* means already received Secret Response */
				{
					devExt->AllowIntSendingFlag = FALSE;
					
					CancelLmpBitState(devExt, pConnectDevice, LMP_MUTUAL_AU + LMP_RECV_SRES_OR_SEND_SRES, 0);
					if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
					{
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_NOTIFICATION, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
						status = After_Conn_Authentication(devExt, pConnectDevice);
					}
					else
					{
						ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
						if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
							status = PauseResumeEncryption_Process(devExt, pConnectDevice);
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_NOTIFICATION, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
						if (pConnectDevice->pause_command_flag == 4)
							Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					}

					devExt->AllowIntSendingFlag = TRUE;
					timevalue.QuadPart = 0;
					tasklet_schedule(&devExt->taskletRcv);
				}
				else
				{
					ChangeLmpState(devExt, pConnectDevice, LMP_RECV_SRES_OR_SEND_SRES); /* means sent Secret Response */
					if (pLmpPdu->TransID != ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
						status = Send_AuRand_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
				}
			}
		} /* end if (NT_SUCCESS(status)) */
	}

	return status;
}

NTSTATUS LMP_sres(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                       /* 12 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 AU_SRes[4], ErrReason;
	UINT32 result;
	LARGE_INTEGER timevalue;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Secret Response, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Secret Response\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_AURAND)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_AURAND && (pConnectDevice->lmp_states & LMP_AU_ENCRYPTION_COLLISION) == 0)
		return STATUS_SUCCESS;
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND)
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_SEND_AURAND */

	E1_Authentication_KeyGen(pConnectDevice->link_key, pConnectDevice->random, pConnectDevice->bd_addr, AU_SRes, pConnectDevice->aco);
	result = RtlEqualMemory(AU_SRes, pLmpPdu->contents, 4);
	devExt->AllowIntSendingFlag = FALSE;
	if (result == 1)
	{
		if ((pConnectDevice->lmp_states & LMP_MUTUAL_AU) == LMP_MUTUAL_AU)
		{
			if ((pConnectDevice->lmp_states & LMP_RECV_SRES_OR_SEND_SRES) == LMP_RECV_SRES_OR_SEND_SRES) /* means already received Secret Response */
			{
				CancelLmpBitState(devExt, pConnectDevice, LMP_MUTUAL_AU + LMP_RECV_SRES_OR_SEND_SRES, 0);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_NOTIFICATION, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				{
					status = After_Conn_Authentication(devExt, pConnectDevice);
				}
				else
				{
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
					if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
						status = PauseResumeEncryption_Process(devExt, pConnectDevice);
				}
			}
			else
			{
				ChangeLmpState(devExt, pConnectDevice, LMP_RECV_SRES_OR_SEND_SRES); /* means received Secret Response */
			}
		}
		else
		{
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
			{
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) > LMP_SEND_ENMODE_REQ && 
					(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ)
				{
					if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
						status = Send_SetupComplete_PDU(devExt, pConnectDevice);
				}
				else
				{
					status = After_Conn_Authentication(devExt, pConnectDevice);
				}
			}
			else
			{
				if ((pConnectDevice->lmp_states & LMP_AU_ENCRYPTION_COLLISION) == 0)
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
		}
	}
	else
	{
		pConnectDevice->current_reason_code = AUTHENTICATION_FAILURE;

		if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
		{
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			status = ResLmpDetach(devExt, pConnectDevice, AUTHENTICATION_FAILURE);
		}
		else
		{
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
	}

	if ((pConnectDevice->lmp_states & LMP_AU_ENCRYPTION_COLLISION) == LMP_AU_ENCRYPTION_COLLISION)
		CancelLmpBitState(devExt, pConnectDevice, LMP_AU_ENCRYPTION_COLLISION, 0);

	devExt->AllowIntSendingFlag = TRUE;
	timevalue.QuadPart = 0;
	tasklet_schedule(&devExt->taskletRcv);
	return status;
}

NTSTATUS LMP_temp_rand(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 13 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Temp Rand, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_temp_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 14 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Temp Key, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_encryption_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 15 */
{
	NTSTATUS status;
	UINT8    ErrReason;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Encryption Mode Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Encryption Mode Req, mode= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_ENMODE_REQ && pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ERROR_TRANSACTION_COLLISION);
		return status;
	}
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.encry == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENMODE_REQ && pConnectDevice->encryption_mode == pLmpPdu->contents[0])
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ENCRYPTION_MODE_NOT_ACCEPTABLE);
		return status;
	}
	
	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND)
		ChangeLmpState(devExt, pConnectDevice, LMP_AU_ENCRYPTION_COLLISION);
	
	pConnectDevice->encryption_mode          = pLmpPdu->contents[0];
	pConnectDevice->is_in_encryption_process = 1;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 15;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted EnModeReq, mode=%d\n", pdu.TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
				status = Send_EnKeySizeReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID, MAX_EN_KEY_SIZE);
			else
				status = Send_StopEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
		}
		else
		{
			if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
				ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_ENKEYSIZE_REQ);
			else
				ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STOPEN_REQ);
		}
	}

	return status;
}

NTSTATUS LMP_encryption_key_size_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)    /* 16 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    ErrReason;
	LARGE_INTEGER timevalue;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Encryption Key Size Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_ENKEYSIZE_REQ &&
		(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_ENKEYSIZE_REQ)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Encryption Key Size Req, but unexpected\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		return status;
	}	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Encryption Key Size Req, size= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ENKEYSIZE_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);

	ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel flag */
	if (pLmpPdu->contents[0] < MIN_EN_KEY_SIZE)
	{
		pConnectDevice->current_reason_code = UNSUPPORTED_PARAMETER;
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, pConnectDevice->current_reason_code);

		pConnectDevice->is_in_encryption_process = 0;
		pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
		if (pLmpPdu->TransID == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
		{
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_NOT_COMP, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			else
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		
		//Jakio20071220, check tx queue
		timevalue.QuadPart = 0;
		//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
		UsbQueryDMASpace(devExt);
	}
	else if (pLmpPdu->contents[0] > MAX_EN_KEY_SIZE)
	{
		status = Send_EnKeySizeReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID, MAX_EN_KEY_SIZE);
	}
	else
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 16;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted EnKeySizeReq, size=%d\n", pdu.TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
		
			pConnectDevice->encryption_key_size = pLmpPdu->contents[0];
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
			{
				/* Write EncryptionKeySize register to hardware */
				
				status = Send_StartEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			}
			else
			{
				/* Write EncryptionKeySize register to hardware */

				ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STARTEN_REQ);
			}
		}
	}

	return status;
}

NTSTATUS LMP_start_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)       /* 17 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8 ErrReason, reg_offset;
	UINT8 COF[BT_ACO_LENGTH], Kc[BT_LINK_KEY_LENGTH], ApostropheKc[BT_LINK_KEY_LENGTH];
	LARGE_INTEGER timevalue;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Start Encryption Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STARTEN_REQ || pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Start Encryption Req, but unexpected\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		return status;
	}	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Start Encryption Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);	
	ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_WAIT_STARTEN_REQ */

	/* Calculate EncryptionKey and write EncryptionKey register to hardware */
	RtlCopyMemory(COF, pConnectDevice->aco, BT_ACO_LENGTH);
	E3_Encryption_KeyGen(pConnectDevice->link_key, pLmpPdu->contents, COF, Kc);
	Encryption_ApostropheKeyGen(Kc, pConnectDevice->encryption_key_size, ApostropheKc);
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	reg_offset = pConnectDevice->slave_index * 8;
	if (pConnectDevice->slave_index == 0)
	{
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_KEY, pBuf, BT_LINK_KEY_LENGTH, ApostropheKc);
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_KEY_SLAVE1, pBuf, BT_LINK_KEY_LENGTH, ApostropheKc);
	}
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + reg_offset, pBuf, 1, 3);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AM_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 17;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted StartEncryptionReq\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		pConnectDevice->is_in_encryption_process = 0;

		if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
		{
			if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
				status = Send_SetupComplete_PDU(devExt, pConnectDevice);
		}
		else
		{
			if (pConnectDevice->pause_encryption_status == 0)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else
			{
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_RESUMEENCRYPTION_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				
				pConnectDevice->encryption_mode = BT_ENCRYPTION_ONLY_P2P;
				pConnectDevice->pause_encryption_status = 0;
				pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
		}

		//Jakio20071220, check tx queue
		timevalue.QuadPart = 0;
		//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
		UsbQueryDMASpace(devExt);
	}

	return status;
}

NTSTATUS LMP_stop_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 18 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8 reg_offset;
	LARGE_INTEGER timevalue;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Stop Encryption Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}

	if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STOPEN_REQ || pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Stop Encryption Req, but unexpected\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		return status;
	}	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Stop Encryption Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);	
	ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP); /* cancel LMP_WAIT_STOPEN_REQ */
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	reg_offset = pConnectDevice->slave_index * 8;
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + reg_offset, pBuf, 1, 0);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 18;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted StopEncryptionReq\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (pConnectDevice->pause_encryption_status == 0)
		{
			pConnectDevice->is_in_encryption_process = 0;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			//Jakio20071220, check tx queue
			timevalue.QuadPart = 0;
			//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
			UsbQueryDMASpace(devExt);
		}
		else
		{
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PAUSEENCRYPTION_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
			
			pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
			ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STARTEN_REQ);
			if (pConnectDevice->pause_encryption_status == 2 && pLmpPdu->TransID == SLAVE_TRANSACTION_ID)
			{
				if (pConnectDevice->pause_command_flag == 1)
				{
					status = Send_SlotOffset_PDU(devExt, pConnectDevice, SLAVE_TRANSACTION_ID);
					status = Send_SwitchReq_PDU(devExt, pConnectDevice);
				}
				else
				{
					status = Send_ResumeEncryptionReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
				}
			}
		}
	}

	return status;
}

NTSTATUS LMP_switch_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                 /* 19 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT32 SwitchInstant;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	UINT8		ListFlag;

	BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Received (TransID=%d, BD0=0x%x): Switch Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Switch Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&SwitchInstant, pLmpPdu->contents, 4);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Switch Req, SwitchInstant= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], SwitchInstant);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot_offset == 0 || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.switchbit == 0 || (pConnectDevice->link_policy_settings & 0x0001) == 0 
		|| pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE || pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE || pConnectDevice->pScoConnectDevice != NULL 
		|| ((pConnectDevice->lmp_states & LMP_SEND_CONN_REQ) == LMP_SEND_CONN_REQ && pConnectDevice->allow_role_switch == 0))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----link_policy_settings= 0x%x, mode_current_mode= %d\n", pConnectDevice->link_policy_settings, pConnectDevice->mode_current_mode);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, SWITCH_NOT_ALLOWED);
		return status;
	}
	
	if (((PBT_HCI_T)devExt->pHci)->role_switching_flag == 1)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Role switching flag has been set\n");
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, SWITCH_NOT_ALLOWED);
		return status;
	}

	if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, SWITCH_NOT_ALLOWED);
		return status;
	}

#ifdef BT_ROLESWITCH_UNALLOWED_WHEN_MULTI_SLAVE
	if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_am > 1)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, SWITCH_NOT_ALLOWED);
		return status;
	}
#endif

	if((devExt->ComboState == FW_WORK_MODE_COMBO) && (pConnectDevice->current_role == BT_ROLE_MASTER))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP---Role switch from slave is not allowed in combo state\n");
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, SWITCH_NOT_ALLOWED);
		return status;
	}

	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	Frag_StopQueue(devExt, ListFlag);
	if(Frag_IsBDEmpty(devExt, ListFlag))
	{
		Frag_StartQueue(devExt, ListFlag);
		
		((PBT_HCI_T)devExt->pHci)->role_switching_flag = 1;
		pConnectDevice->role_switching_flag = 1;
		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			status = Send_SlotOffset_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			pConnectDevice->role_switching_role = BT_ROLE_MASTER;
			BtWriteFHSPacket(devExt, pConnectDevice);
		}
		else
		{
			pConnectDevice->role_switching_role = BT_ROLE_SLAVE;
		}
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (1 << 30));
		else
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (pConnectDevice->slave_index << 31));

		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = pLmpPdu->OpCode;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Switch Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_switch_req----BD not empty, try it later\n");	
		UsbQueryDMASpace(devExt);

		RtlCopyMemory(buf, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		RtlCopyMemory(buf + sizeof(PCONNECT_DEVICE_T), pLmpPdu, PduLength);
		status = Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_RECV), 
					BT_TASK_PRI_NORMAL, buf, sizeof(PCONNECT_DEVICE_T)+PduLength);
	}
	return status;
	
	/*
	((PBT_HCI_T)devExt->pHci)->role_switching_flag = 1;
	pConnectDevice->role_switching_flag = 1;
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		status = Send_SlotOffset_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
		pConnectDevice->role_switching_role = BT_ROLE_MASTER;
		BtWriteFHSPacket(devExt, pConnectDevice);
	}
	else
	{
		pConnectDevice->role_switching_role = BT_ROLE_SLAVE;
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (1 << 30));
	else
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (pConnectDevice->slave_index << 31));

	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	sc_zero_memory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = pLmpPdu->OpCode;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Switch Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	return status;
	*/
}

NTSTATUS LMP_hold(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                       /* 20 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8  RegisterIndex = SLAVE_REGISTER_OFFSET;
	UINT16 HoldTime;
	UINT32  HoldInstant;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Hold, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&HoldTime, pLmpPdu->contents, sizeof(UINT16));
	RtlCopyMemory(&HoldInstant, pLmpPdu->contents + 2, 4);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Hold, HoldTime= 0x%x, HoldInstant= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], HoldTime, HoldInstant);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.hold_mode == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	if ((pConnectDevice->lmp_ext_states & LMP_HOLD_ACCEPTED) == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if (pConnectDevice->afh_mode == 1)
			Send_SetAFH_PDU(devExt, pConnectDevice, AFH_ENABLED, 1);
		
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = pLmpPdu->TransID;
		pdu.OpCode  = 20;
		
		RegisterIndex = pConnectDevice->am_addr;
		HoldTime      = (HoldTime > pConnectDevice->hold_mode_interval) ? pConnectDevice->hold_mode_interval : HoldTime;
		HoldInstant   = HoldInstant + pConnectDevice->per_poll_interval * 6;
		
		RtlCopyMemory(pdu.contents, &HoldTime, sizeof(UINT16));
		RtlCopyMemory(pdu.contents + 2, &HoldInstant, 4);
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 7);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Hold, HoldTime= 0x%x, HoldInstant= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], HoldTime, HoldInstant);
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOLD_MODE_INTERVAL + 2 * RegisterIndex, pBuf, 2, (PUINT8)&HoldTime);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HODE_INSTANT, pBuf, 4, (PUINT8)&HoldInstant);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	pConnectDevice->mode_current_mode   = BT_MODE_CURRENT_MODE_HOLD;
	pConnectDevice->hold_mode_interval  = HoldTime;
	pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return status;
}

NTSTATUS LMP_hold_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 21 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8  RegisterIndex = SLAVE_REGISTER_OFFSET;
	UINT16 HoldTime;
	UINT32  HoldInstant;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Hold Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&HoldTime, pLmpPdu->contents, sizeof(UINT16));
	RtlCopyMemory(&HoldInstant, pLmpPdu->contents + 2, 4);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Hold Req, HoldTime= 0x%x, HoldInstant= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], HoldTime, HoldInstant);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.hold_mode == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	if ((pConnectDevice->link_policy_settings & 0x0002) == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_HOLD_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (HoldTime < pConnectDevice->hold_mode_min_interval)
	{
		CancelLmpBitState(devExt, pConnectDevice, LMP_HOLD_ACCEPTED, 1);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_PARAMETER);

		if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_HOLD_REQ)
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
			
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));

			pConnectDevice->current_reason_code = UNSUPPORTED_PARAMETER;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
	}
	else if (HoldTime > pConnectDevice->hold_mode_max_interval)
	{
		if (pConnectDevice->current_role == BT_ROLE_MASTER && pConnectDevice->afh_mode == 1 && (pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_HOLD_REQ)
			Send_SetAFH_PDU(devExt, pConnectDevice, AFH_ENABLED, 1);
		status = Send_HoldReq_PDU(devExt, pConnectDevice, pConnectDevice->hold_mode_max_interval, pLmpPdu->TransID);
	}
	else
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 21;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_HOLD_ACCEPTED);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Hold Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_HOLD_REQ)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER && pConnectDevice->afh_mode == 1)
					Send_SetAFH_PDU(devExt, pConnectDevice, AFH_ENABLED, 1);
				
				pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_HOLD;
			}
			else
			{
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			}
			
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
				RegisterIndex = pConnectDevice->am_addr;
			
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOLD_MODE_INTERVAL + 2 * RegisterIndex, pBuf, 2, (PUINT8)&HoldTime);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HODE_INSTANT, pBuf, 4, (PUINT8)&HoldInstant);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			
			pConnectDevice->hold_mode_interval  = HoldTime;
			pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
	}

	return status;
}
    
NTSTATUS LMP_sniff_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 23 */
{
	NTSTATUS   status = STATUS_SUCCESS;
	BT_SNIFF_T temp_sniff;
	LMP_PUD_PACKAGE_T pdu;
	UINT8  tempvalue;
	UINT16 offset_value;
	UINT32    instant = 0;
	PBT_HCI_T	pHci;
	KIRQL		oldIrql;
	BOOLEAN		FindSco = FALSE;
	PCONNECT_DEVICE_T	pTempDevice;

	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
		return STATUS_UNSUCCESSFUL;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Sniff Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&temp_sniff.timing_control_flags, pLmpPdu->contents, 9);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Sniff Req, TimingFlag= %d, D= 0x%x, T= 0x%x, Attempt= 0x%x, Timeout= 0x%x\n", 
		pLmpPdu->TransID, pConnectDevice->bd_addr[0], temp_sniff.timing_control_flags, temp_sniff.offset, temp_sniff.interval, temp_sniff.attempt, temp_sniff.timeout);
	
/*	
#ifdef BT_SNIFF_ONLY_ONE_CONNECTION
	if (((PBT_HCI_T)devExt->pHci)->num_device_am + ((PBT_HCI_T)devExt->pHci)->num_device_slave > 1)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
		return status;
	}
#endif
*/
#ifdef BT_EXIT_SNIFF_WHEN_EXIST_SLAVE
	if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
		return status;
	}
#endif
#ifdef BT_SCO_SNIFF_BONDING_WHEN_MS
	if ((pConnectDevice->current_role == BT_ROLE_SLAVE && LMP_CheckMasterSCO(devExt) == 1) 
		|| (pConnectDevice->current_role == BT_ROLE_MASTER && LMP_CheckSlaveSCO(devExt) == 1))
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
		return status;
	}
#endif
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.sniff_mode == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}

	//Jakio20090302: if in combo state, we don't accept request from phone devices
	if(devExt->ComboState == FW_WORK_MODE_COMBO)
	{
		//Jakio20090904: don't allow sniff request when sco link exists
		if((pConnectDevice->pScoConnectDevice != NULL) || ((pConnectDevice->class_of_device[1] & 0x1f) == 0x04))
		{
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP_sniff_req-----audio devices, don't accept\n");
			status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
			return status;	
		}
	}

	//Jakio20090914: check if sco link exists, if so, reject this sniff request
	if(pConnectDevice->manufacturer_name != 56)
	{
		KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		pTempDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
		while(pTempDevice != NULL)
		{
			if(pTempDevice->pScoConnectDevice != NULL)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "Lmp_sniff_req---Find sco device: [0x%x]\n", pConnectDevice->bd_addr[0]);
				FindSco = TRUE;
				break;
			}
			pTempDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pTempDevice->Link);
		}
		if(!FindSco)
		{
			pTempDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
			while(pTempDevice != NULL)
			{
				if(pTempDevice->pScoConnectDevice != NULL)
				{
					BT_DBGEXT(ZONE_LMP | LEVEL3, "Lmp_sniff_req---Find slave sco device: [0x%x]\n", pConnectDevice->bd_addr[0]);
					FindSco = TRUE;
					break;
				}
				pTempDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pTempDevice->Link);
			}	
		}
		KeReleaseSpinLock(&pHci->HciLock, oldIrql);

		if(FindSco)
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_sniff_req----Not accept becauseof sco link\n");
			status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
			return status;	
		}
	}
	
	if(pConnectDevice->is_in_disconnecting == 1)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP_sniff_req-----device[%02x] is disconnecting, do not accept\n", pConnectDevice->bd_addr[0]);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
		return status;
	}

    
	if ((pConnectDevice->link_policy_settings & 0x0004) == 0 || pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----link_policy_settings= 0x%x, mode_current_mode= %d\n", pConnectDevice->link_policy_settings, pConnectDevice->mode_current_mode);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	if (((PBT_LMP_T)devExt->pLmp)->sniff_number >= SNIFF_MAX_NUMBER)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----the amount of sniff link expires\n");
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_SNIFF_REQ)
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SNIFF_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);
	}
	else
	{
		pConnectDevice->sniff_min_interval = SNIFF_MIN_INTERVAL;
		pConnectDevice->sniff_max_interval = SNIFF_MAX_INTERVAL;
		if (pConnectDevice->sniff_max_interval > pConnectDevice->link_supervision_timeout)
		{
			if (pConnectDevice->link_supervision_timeout % 2 == 0)
				pConnectDevice->sniff_max_interval = pConnectDevice->link_supervision_timeout;
			else
				pConnectDevice->sniff_max_interval = pConnectDevice->link_supervision_timeout - 1;
		}
	}
	
	if (temp_sniff.interval < pConnectDevice->sniff_min_interval || temp_sniff.interval > pConnectDevice->sniff_max_interval 
		|| temp_sniff.interval % 2 != 0 || temp_sniff.offset % 2 != 0 || temp_sniff.offset > (temp_sniff.interval - 2) 
		|| temp_sniff.attempt == 0 || temp_sniff.attempt > (temp_sniff.interval >> 1) || temp_sniff.timeout > SNIFF_MAX_TIMEOUT)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_PARAMETER);
		if (((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_number].init_flag == SNIFF_INIT_FLAG)
		{
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_number], sizeof(BT_SNIFF_T));
			pConnectDevice->sniff_mode_interval = 0;
			pConnectDevice->current_reason_code = UNSUPPORTED_PARAMETER;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		
		return status;
	}
	
	((PBT_LMP_T)devExt->pLmp)->sniff_index = ((PBT_LMP_T)devExt->pLmp)->sniff_number;
	RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timing_control_flags), pLmpPdu->contents, 9);
	((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice = pConnectDevice;
	
	tempvalue = LMP_ChangeSniffParaForSCO(devExt, &(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index]));
	if (tempvalue == 2)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSPECIFIED_ERROR);
		if (((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].init_flag == SNIFF_INIT_FLAG)
		{
			RtlZeroMemory(&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index]), sizeof(BT_SNIFF_T));
			pConnectDevice->sniff_mode_interval = 0;
			pConnectDevice->current_reason_code = UNSPECIFIED_ERROR;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		
		return status;
	}
	else if (tempvalue == 1)
	{
		status = Send_SniffReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
		return status;
	}

	if((temp_sniff.timeout == 0) && (temp_sniff.attempt == 1))
	{
		temp_sniff.timeout = 1;

		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = pLmpPdu->TransID;;
		pdu.OpCode  = 23;
		
		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			if (pConnectDevice->slave_index == 0)
				instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
			else
				instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
		}
		else
		{
			instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
		}
		
		instant = ((instant >> 27) & 0x00000001) << 1;
		temp_sniff.timing_control_flags = (UINT8)instant;
		
		RtlCopyMemory(pdu.contents, &(temp_sniff.timing_control_flags), 9);
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 10);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SNIFF_REQ);
			if (pConnectDevice->lmp_timer_valid == 0)
				LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_SNIFF_REQ, LMP_MAX_RESPONSE_TIME);
			
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Sniff Req, TimingFlag= %d, D= 0x%x, T= 0x%x, Attempt= 0x%x, Timeout= 0x%x\n", 
				pdu.TransID, pConnectDevice->bd_addr[0], temp_sniff.timing_control_flags, temp_sniff.offset, temp_sniff.interval, temp_sniff.attempt, temp_sniff.timeout);
		}
		
		return status;
		
	}
	else
	{
		Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
	
		((PBT_LMP_T)devExt->pLmp)->sniff_number++;
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 23;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
		{      
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Sniff Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);	
		}
	}

/**    
	Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
	
	((PBT_LMP_T)devExt->pLmp)->sniff_number++;
	sc_zero_memory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 23;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Sniff Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
**/	
	return status;
}

NTSTATUS LMP_unsniff_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                /* 24 */
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 i;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Unsniff Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Unsniff Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 24;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Unsniff Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.sniff_mode == 0 || pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_SNIFF)
		return status;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
			break;
	}
	if (i >= ((PBT_LMP_T)devExt->pLmp)->sniff_number)
		return status;
	
	if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_UNSNIFF_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);
	}
	
	Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
	return status;
}
    
NTSTATUS LMP_park_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 26 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Park Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_set_broadcast_scan_window(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)  /* 27 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Set Broadcast Scan Window, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_modify_beacon(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 28 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Modify Beacon, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_unpark_BD_ADDR_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)         /* 29 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Unpark BD_ADDR Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_unpark_PM_ADDR_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)         /* 30 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Unpark PM_ADDR Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_incr_power_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)             /* 31 */
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	UINT8 reg_offset, maxpower;
	UINT32 tx_power_reg;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Increase Power Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Increase Power Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.power_control == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pConnectDevice->lmp_features.byte1.rssi == 0)
	{
		status = Send_PowerControl_PDU(devExt, pConnectDevice, 3, pLmpPdu->TransID); /* max power PDU */
		return status;
	}

#ifdef BT_EEPROM_POWER
	maxpower = ((PBT_HCI_T)devExt->pHci)->tx_maxpower;
#else
	maxpower = TX_POWER_MAX;
#endif
	
	if (pConnectDevice->tx_power >= maxpower)
	{
		status = Send_PowerControl_PDU(devExt, pConnectDevice, 3, pLmpPdu->TransID); /* max power PDU */
	}
	else
	{
		if (maxpower - pConnectDevice->tx_power < TX_POWER_ADJUST_STEP)
			pConnectDevice->tx_power = maxpower;
		else
			pConnectDevice->tx_power += TX_POWER_ADJUST_STEP;
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			reg_offset = pConnectDevice->am_addr;
		}
		else
		{
			reg_offset = pConnectDevice->slave_index * 8;
		}
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_TRANSMITT_POWER_CONTROL, pBuf, 4, (0x1 << (reg_offset * 2)));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_TRANSMITT_POWER_CONTROL, pBuf, 4, (~(0x1 << (reg_offset * 2 + 1))));
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_TRANSMIT_POWER_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	}
	
	return status;
}

NTSTATUS LMP_decr_power_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)             /* 32 */
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	UINT8 reg_offset, minpower;
	UINT32 tx_power_reg;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Decrease Power Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Decrease Power Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.power_control == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pConnectDevice->lmp_features.byte1.rssi == 0)
	{
		status = Send_PowerControl_PDU(devExt, pConnectDevice, 4, pLmpPdu->TransID); /* min power PDU */
		return status;
	}

#ifdef BT_EEPROM_POWER
	minpower = ((PBT_HCI_T)devExt->pHci)->tx_minpower;
#else
	minpower = TX_POWER_MIN;
#endif
	
	if (pConnectDevice->tx_power <= minpower)
	{
		status = Send_PowerControl_PDU(devExt, pConnectDevice, 4, pLmpPdu->TransID); /* min power PDU */
	}
	else
	{
		if (pConnectDevice->tx_power - minpower < TX_POWER_ADJUST_STEP)
			pConnectDevice->tx_power = minpower;
		else
			pConnectDevice->tx_power -= TX_POWER_ADJUST_STEP;
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			reg_offset = pConnectDevice->am_addr;
		}
		else
		{
			reg_offset = pConnectDevice->slave_index * 8;
		}
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_TRANSMITT_POWER_CONTROL, pBuf, 4, (0x1 << (reg_offset * 2 + 1)));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_TRANSMITT_POWER_CONTROL, pBuf, 4, (~(0x1 << (reg_offset * 2))));
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_TRANSMIT_POWER_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	}
	
	return status;
}

NTSTATUS LMP_max_power(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 33 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Max Power, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	pConnectDevice->rx_power = RX_POWER_MAX_FLAG;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Max Power\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	return status;
}

NTSTATUS LMP_min_power(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 34 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Min Power, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	pConnectDevice->rx_power = RX_POWER_MIN_FLAG;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Min Power\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	return status;
}

NTSTATUS LMP_auto_rate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 35 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Auto Rate, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Auto Rate\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);

	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.channel_qua == 1 && pConnectDevice->lmp_features.byte1.channel_qua == 1)
		pConnectDevice->rev_auto_rate = 1;
	
	return status;
}

NTSTATUS LMP_preferred_rate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)             /* 36 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Preferred Rate, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Preferred Rate, DataRate= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.channel_qua == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_preferred_rate: exit\n");
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_preferred_rate: exit\n");
	return status;
}

NTSTATUS LMP_version_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                /* 37 */
{
	NTSTATUS status;
	UINT8  ErrReason;
	UINT8  ResLen;
	UINT16 content[2];
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Version Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	RtlCopyMemory(content, &pLmpPdu->contents[1], sizeof(content));
	pConnectDevice->lmp_version       = pLmpPdu->contents[0];
	pConnectDevice->manufacturer_name = content[0];
	pConnectDevice->lmp_subversion    = content[1];

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Version Req, %d %d %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pConnectDevice->lmp_version, pConnectDevice->manufacturer_name, pConnectDevice->lmp_subversion);

	ResLen = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));

	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;
	pdu.contents[0] = ((PBT_HCI_T)devExt->pHci)->lmp_version;
	content[0]      = ((PBT_HCI_T)devExt->pHci)->manufacturer_name;
	content[1]      = ((PBT_HCI_T)devExt->pHci)->lmp_subversion;
	RtlCopyMemory(&pdu.contents[1], content, sizeof(content));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Version Res, %d %d %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0], content[0], content[1]);
	return status;
}

NTSTATUS LMP_version_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                /* 38 */
{
	UINT16 content[2];
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Version Res, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		return STATUS_SUCCESS;
	}
	
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_VER_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	RtlCopyMemory(content, &pLmpPdu->contents[1], sizeof(content));
	pConnectDevice->lmp_version       = pLmpPdu->contents[0];
	pConnectDevice->manufacturer_name = content[0];
	pConnectDevice->lmp_subversion    = content[1];
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Version Res, %d %d %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pConnectDevice->lmp_version, pConnectDevice->manufacturer_name, pConnectDevice->lmp_subversion);
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_VERSION_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return STATUS_SUCCESS;
}

NTSTATUS LMP_features_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 39 */
{
	NTSTATUS status;
	UINT8    ErrReason;
	UINT8    ResLen;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Features Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Features Req, remote feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7]);
	
	RtlCopyMemory(&(pConnectDevice->lmp_features), pLmpPdu->contents, sizeof(LMP_FEATURES_T));
	
	ResLen = LMP_PDU_Rx_Data[LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode].PDU_Length;
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = pLmpPdu->TransID;
	pdu.OpCode  = LMP_PDU_Rx_Data[pLmpPdu->OpCode].ResOpcode;

#ifdef BT_COMBO_SNIFF_SUPPORT
	if(devExt->ComboState == FW_WORK_MODE_COMBO)
	{
		RtlCopyMemory(&tmpLocalFeature, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
		tmpLocalFeature.byte0.sniff_mode = 0;
		RtlCopyMemory(pdu.contents, &tmpLocalFeature, sizeof(LMP_FEATURES_T));	
	}
	else
	{
		RtlCopyMemory(pdu.contents, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
	}
#else
	RtlCopyMemory(pdu.contents, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
#endif
	
#ifdef BT_DISABLE_SLAVE_SNIFF_WHEN_MS
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && ((PBT_HCI_T)devExt->pHci)->num_device_am > 0)
		pdu.contents[0] = pdu.contents[0] & 0x7F;
#endif
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, ResLen);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Features Res, local feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			pdu.contents[0], pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7]);
	}
	
	return status;
}

NTSTATUS LMP_features_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 40 */
{
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Features Res, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		return STATUS_SUCCESS;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Features Res, remote feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_FEATURES_REQ)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	RtlCopyMemory(&(pConnectDevice->lmp_features), pLmpPdu->contents, sizeof(LMP_FEATURES_T));
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_FEATURE_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return STATUS_SUCCESS;
}

NTSTATUS LMP_quality_of_service(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)         /* 41 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT16 PollInterval;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Quality of Service, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&PollInterval, pLmpPdu->contents, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Quality of Service, PollInterval= %d, Nbc= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PollInterval, pLmpPdu->contents[2]);
	
	if (pConnectDevice->current_role != BT_ROLE_SLAVE)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	pConnectDevice->qos_flags           = 0;
	pConnectDevice->qos_service_type    = 1; /* Best Effort */
	pConnectDevice->qos_token_rate      = 0;
	pConnectDevice->qos_peak_bandwidth  = 0;
	pConnectDevice->qos_latency         = 25000;  /* ms */
	pConnectDevice->qos_delay_variation = 0xFFFFFFFF;
	pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_QOS_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	
	return status;
}

NTSTATUS LMP_quality_of_service_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)     /* 42 */
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;
	UINT16 PollInterval;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Quality of Service Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&PollInterval, pLmpPdu->contents, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Quality of Service Req, PollInterval= %d, Nbc= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PollInterval, pLmpPdu->contents[2]);
	
	/*
	if (pConnectDevice->current_role == BT_ROLE_MASTER && PollInterval < 40)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, QOS_UNACCEPTABLE_PARAMETER);
		return status;
	}
	*/
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));	
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 42;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Quality of Service Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	pConnectDevice->qos_flags           = 0;
	pConnectDevice->qos_service_type    = 1; /* Best Effort */
	pConnectDevice->qos_token_rate      = 0;
	pConnectDevice->qos_peak_bandwidth  = 0;
	pConnectDevice->qos_latency         = 25000; /* ms */
	pConnectDevice->qos_delay_variation = 0xFFFFFFFF;
	pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_QOS_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	
	return status;
}

NTSTATUS LMP_SCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 43 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 ErrReason, indicator, i, flag = 0;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	LARGE_INTEGER timevalue;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): SCO Link Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): SCO Link Req, contents= %d %d %d %d %d %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5]);
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && ((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->slave_sco_master_not_coexist_flag == 1)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	LMP_SCONeedUnsniff(devExt, pConnectDevice);
	
	indicator = ((PBT_LMP_T)devExt->pLmp)->sco_indicator;
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if (pLmpPdu->contents[0] == 0)
		{
			if ((indicator >> 6) >= SCO_MAX_NUMBER)
			{
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, MAX_SCO_CONNECTIONS);
				return status;
			}

			RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->sco[indicator >> 6].sco_handle), pLmpPdu->contents, 6);
			((PBT_LMP_T)devExt->pLmp)->sco[indicator >> 6].pDevice = pConnectDevice;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)((indicator >> 6) + (indicator & 0xfc));

			ChangeLmpExtraState(devExt, pConnectDevice, LMP_RECV_SCO_LINK_REQ);
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_LINK_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			return STATUS_SUCCESS;
		}
		else
		{
			for (i = 0; i < (indicator >> 6); i++)
			{
				if (((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle == pLmpPdu->contents[0])
				{
					if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice == pConnectDevice)
						flag = 1;

					break;
				}
			}
			if (flag == 0)
			{
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
				return status;
			}
			
			RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle), pLmpPdu->contents, 6);
			ErrReason = CheckSCOPara(devExt, pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->sco[i]), 1); /* check parameters for changing a SCO */
			if (ErrReason != 0)
			{
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
				return status;
			}
			
			Frag_InitScoRxBuffer(devExt, pConnectDevice);
			((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag = 0;
			((PBT_LMP_T)devExt->pLmp)->sco[i].D_sco  = SCO_DEFAULT_D;
			((PBT_LMP_T)devExt->pLmp)->sco_counter   = 0;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (indicator & 0xfc));
			status = Send_SCOLinkReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
			return status;
		} /* end pLmpPdu->contents[0] is not zero */
	}
	else /* slave */
	{
		if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_SCO_LINK_REQ)
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SCOLINK_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
		}

		for (i = 0; i < (indicator >> 6); i++)
		{
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle == pLmpPdu->contents[0])
			{
				if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice == pConnectDevice)
					flag = 1;
				
				break;
			}
		}
		if (flag == 0)
		{
			i = (UINT8)(indicator >> 6);
			if (i >= SCO_MAX_NUMBER)
			{
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, MAX_SCO_CONNECTIONS);
				return status;
			}
			
			if (pLmpPdu->contents[0] == 0)
			{
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
				if (((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag == SCO_ADD_LINK)
				{
					RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[i], sizeof(BT_SCO_T));
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				
				return status;
			}

			RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle), pLmpPdu->contents, 6);
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag != SCO_ADD_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice = pConnectDevice;
				((PBT_LMP_T)devExt->pLmp)->sco_indicator  = (UINT8)(i + (indicator & 0xfc));
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_RECV_SCO_LINK_REQ);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_LINK_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				return STATUS_SUCCESS;
			}
			else
			{
				ErrReason = CheckSCOPara(devExt, pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->sco[i]), 2); /* check parameters for adding a SCO */
				if (ErrReason != 0)
				{
				
					devExt->AllowIntSendingFlag = FALSE;
					status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);

					RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[i], sizeof(BT_SCO_T));
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));

					devExt->AllowIntSendingFlag = TRUE;
					timevalue.QuadPart = 0;
					tasklet_schedule(&devExt->taskletRcv);	
					return status;
				}
			}
		} /* end if (flag == 0) */
		else
		{
			RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle), pLmpPdu->contents, 6);
			ErrReason = CheckSCOPara(devExt, pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->sco[i]), 3); /* check parameters for changing a SCO */
			if (ErrReason != 0)
			{
				devExt->AllowIntSendingFlag = FALSE;
				status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
				if (((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag == SCO_CHANGE_LINK)
				{
					((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag = 0;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = ErrReason;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}

				devExt->AllowIntSendingFlag = TRUE;
				timevalue.QuadPart = 0;
				tasklet_schedule(&devExt->taskletRcv);
				return status;
			}
		}
		
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco               = pLmpPdu->contents[2];
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode            = pLmpPdu->contents[5];
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type = SCO_PacketType_Transform[pLmpPdu->contents[4]];

		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_D_SCO, pBuf, 1, (((((pLmpPdu->contents[1] >> 1) & 0x01) + 1) << 6) + pLmpPdu->contents[2]));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_T_SCO, pBuf, 1, &(pLmpPdu->contents[3]));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AM_SCO, pBuf, 1, pConnectDevice->am_addr + (pConnectDevice->slave_index << 5));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
		
		if (flag == 0)
		{
			((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice = pConnectDevice;
			((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag = 0;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)((i + 1) << 6);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
			devExt->AllowIntSendingFlag = FALSE;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_CONNECTED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		else
		{
			if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV3)
				((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag = 0;
			else
				((PBT_HCI_T)devExt->pHci)->sco_need_1_slot_for_acl_flag = 1;
			if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV1)
				((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag = 1;
			else
				((PBT_HCI_T)devExt->pHci)->hv1_use_dv_instead_dh1_flag = 0;
			
			Frag_InitScoRxBuffer(devExt, pConnectDevice);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CHANGE_SCO_CONNECTION_BIT + BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag == SCO_CHANGE_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag = 0;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->changed_packet_type = 0;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = BT_HCI_STATUS_SUCCESS;
				devExt->AllowIntSendingFlag = FALSE;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
		}

		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 43;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted SCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		{
			devExt->AllowIntSendingFlag = TRUE;
			timevalue.QuadPart = 0;
			tasklet_schedule(&devExt->taskletRcv);
		}
	} /* end "else (slave)" */

	return status;
}

NTSTATUS LMP_remove_SCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 44 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 ErrReason, i;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	LARGE_INTEGER timevalue;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Remove SCO Link Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Remove SCO Link Req, contents= %d 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0], pLmpPdu->contents[1]);
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}

	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle == pLmpPdu->contents[0])
		{
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice != pConnectDevice)
				i = ((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6;
			
			break;
		}
	}
	if (i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
	{
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = pLmpPdu->contents[1];
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type           = BT_LINK_TYPE_SCO;
		devExt->AllowIntSendingFlag = FALSE;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		DeleteSCOLink(devExt, i);
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 44;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Remove SCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	if (devExt->AllowIntSendingFlag == FALSE)
	{
		devExt->AllowIntSendingFlag = TRUE;
		timevalue.QuadPart = 0;
		tasklet_schedule(&devExt->taskletRcv);
	}
	return status;
}

NTSTATUS LMP_max_slot(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                   /* 45 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Max Slot, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Max Slot, maxslots= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	
	if (pLmpPdu->contents[0] > BT_MAX_SLOT_5_SLOT)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	{
		pConnectDevice->tx_max_slot = pLmpPdu->contents[0];
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_TX_MAX_SLOT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return status;
}

NTSTATUS LMP_max_slot_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 46 */
{
	NTSTATUS status;
	UINT8    maxslot;
	LMP_PUD_PACKAGE_T pdu;

	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Max Slot Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Max Slot Req, maxslots= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0]);
	
#ifdef BT_USE_ONE_SLOT_FOR_ACL
	maxslot = BT_MAX_SLOT_1_SLOT;
#else
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot5 == 1)
		maxslot = BT_MAX_SLOT_5_SLOT;
	else if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot3 == 1)
		maxslot = BT_MAX_SLOT_3_SLOT;
	else
		maxslot = BT_MAX_SLOT_1_SLOT;
	if (pConnectDevice->max_slot != BT_MAX_SLOT_BY_FEATURE)
		maxslot = (pConnectDevice->max_slot > maxslot) ? maxslot : pConnectDevice->max_slot;
#endif

	if (pLmpPdu->contents[0] > maxslot)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_PARAMETER);
	}
	else
	{
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));	
		pdu.TransID     = pLmpPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 46;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Max Slot Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}

	return status;
}

NTSTATUS LMP_timing_accuracy_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 47 */
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Timing Accuracy Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Timing Accuracy Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.timing_accur == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));	
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 48;
	pdu.contents[0] = 250; /* ppm */
	pdu.contents[1] = 10;  /* us */
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Timing Accuracy Res, drift= %d, jitter= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0], pdu.contents[1]);
	
	return status;
}

NTSTATUS LMP_timing_accuracy_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 48 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Timing Accuracy Res, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Timing Accuracy Res, drift= %d, jitter= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[0], pLmpPdu->contents[1]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.timing_accur == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	return status;
}

NTSTATUS LMP_setup_complete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)             /* 49 */
{
	NTSTATUS status;
	UINT8    ErrReason;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Setup complete, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		ErrReason = INVALID_LMP_PARAMETERS;
		status    = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, ErrReason);
		return status;
	}	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Setup complete\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	ChangeLmpState(devExt, pConnectDevice, LMP_RECV_SETUP_COMPLETE);	
	if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == LMP_SEND_SETUP_COMPLETE)
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SETUP_COMPLETE)
			LMP_StopPDUTimer(devExt, pConnectDevice);
		
		After_Conn_SetupComplete(devExt, pConnectDevice);
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return STATUS_SUCCESS;
}

NTSTATUS LMP_use_semi_permanent_key(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)     /* 50 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Use Semi-permanent Key, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_host_connection_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 51 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Connection Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if (pConnectDevice->current_role != BT_ROLE_SLAVE)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Connection Req, local host is a master\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Connection Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	ChangeLmpState(devExt, pConnectDevice, LMP_RECV_CONN_REQ);
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_HOST_CONN_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	
	/*
	if (*((PUINT8)(&pConnectDevice->lmp_features.byte0)) == 0)
		status = Send_FeatureReq_PDU(devExt, pConnectDevice);
	*/
	
	return status;
}

NTSTATUS LMP_slot_offset(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                /* 52 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT16 slot_offset;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Slot Offset, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&slot_offset, pLmpPdu->contents, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Slot Offset, SlotOffset= %d, BD_ADDR= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		slot_offset, pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot_offset == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	if (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
#ifdef BT_TESTMODE_SUPPORT
	pConnectDevice->local_slot_offset = (UINT16)((1250 - slot_offset) % 1250);
#endif
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SWITCH_SLOT_OFFSET, pBuf, 2, (PUINT8)&slot_offset);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	return status;
}

NTSTATUS LMP_page_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 53 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Page Mode Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_page_scan_mode_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)         /* 54 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Page Scan Mode Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_supervision_timeout(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 55 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT16 Timeout;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Supervision Timeout, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if (pConnectDevice->current_role != BT_ROLE_SLAVE)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Supervision Timeout, but local host isn't a slave\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	RtlCopyMemory(&Timeout, pLmpPdu->contents, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Supervision Timeout, Timeout= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], Timeout);
	
	if (Timeout < 0x0190 && Timeout > 0)
		Timeout = 0x0190;
	pConnectDevice->real_link_supervision_timeout = Timeout;
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_SUPERVISION_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return status;
}

NTSTATUS LMP_test_activate(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 56 */
{
	NTSTATUS  status = STATUS_SUCCESS;
	PBT_HCI_T pHci;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Test Activate, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Test Activate\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
#ifdef BT_TESTMODE_SUPPORT
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci->test_flag == 0 || (pHci->num_device_am + pHci->num_device_slave) > 1 || pHci->test_mode_active == 1 || pConnectDevice->current_role != BT_ROLE_SLAVE 
		|| pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE || pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE || pConnectDevice->pScoConnectDevice != NULL)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	RtlZeroMemory(&(((PBT_TESTMODE_T)devExt->pTestMode)->configuration), sizeof(DUT_CONFIG));
	((PBT_TESTMODE_T)devExt->pTestMode)->pDevice           = pConnectDevice;
	((PBT_TESTMODE_T)devExt->pTestMode)->role              = 2;
	((PBT_TESTMODE_T)devExt->pTestMode)->being_test        = 0;
	((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = DATA_WHITENING;
	((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_WHITENING;
	pHci->test_mode_active = 1;
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = MASTER_TRANSACTION_ID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 56;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Test Activate\n", pdu.TransID, pConnectDevice->bd_addr[0]);
#else
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
#endif
	
	return status;
}

/* bit0: RF Tx Test;  bit1: RF Rx Test;      bit2: Transmitter test; bit3: Loop Back; 
   bit4: whitening;   bit5: Pause Test Mode; bit6: Exit Test Mode;   bit7: tester */
NTSTATUS LMP_test_control(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 57 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8  i;
	UINT16 data_len;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Test Control, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	for (i = 0; i < PduLength - 1; i++)
		pLmpPdu->contents[i] = (UINT8)(pLmpPdu->contents[i] ^ 0x55);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Test Control, contents(byte-orders)= %d %d %d %d %d %d %d %d %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7], pLmpPdu->contents[8]);

#ifdef BT_TESTMODE_SUPPORT
	if (pConnectDevice->current_role != BT_ROLE_SLAVE || ((PBT_HCI_T)devExt->pHci)->test_mode_active == 0 || ((PBT_TESTMODE_T)devExt->pTestMode)->pDevice != pConnectDevice)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	((PBT_TESTMODE_T)devExt->pTestMode)->being_test = 0;
	switch (pLmpPdu->contents[0])
	{
		case 1: /* Transmitter test - 0 pattern */
		case 2: /* Transmitter test - 1 pattern */
		case 3: /* Transmitter test - 1010 pattern */
		case 4: /* Pseudorandom bit sequence */
		case 9: /* Transmitter test - 1111 0000 pattern */
			((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = ((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening;
			((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_NO_WHITENING;
			
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_TYPE_DUT, pBuf, 1, &(pLmpPdu->contents[6]));
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x04);
			break;
			
		case 5: /* Closed Loop Back - ACL packets */
		case 6: /* Closed Loop Back - Synchronous packets */
			((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = ((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening;
			((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_WHITENING;
			
			RtlCopyMemory(&data_len, pLmpPdu->contents + 7, sizeof(UINT16));
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_TYPE_DUT, pBuf, 1, &(pLmpPdu->contents[6]));
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_LEN_DUT, pBuf, 2, (PUINT8)&data_len);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x18);
			break;
			
		case 7: /* ACL Packets without whitening */
		case 8: /* Synchronous Packets without whitening */
			((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = ((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening;
			((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_NO_WHITENING;
			
			RtlCopyMemory(&data_len, pLmpPdu->contents + 7, sizeof(UINT16));
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_TYPE_DUT, pBuf, 1, &(pLmpPdu->contents[6]));
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_PACKET_LEN_DUT, pBuf, 2, (PUINT8)&data_len);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x08);
			break;
			
		case 255: /* Exit Test Mode */
			((PBT_TESTMODE_T)devExt->pTestMode)->being_test        = 2;
			((PBT_TESTMODE_T)devExt->pTestMode)->pDevice           = NULL;
			((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening     = ((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening;
			((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening = DATA_WHITENING;
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x40);
			((PBT_HCI_T)devExt->pHci)->test_mode_active = 0;
			RtlZeroMemory(&(((PBT_TESTMODE_T)devExt->pTestMode)->configuration), sizeof(DUT_CONFIG));
			
			((PBT_TESTMODE_T)devExt->pTestMode)->PacketType    = 0;
			((PBT_TESTMODE_T)devExt->pTestMode)->PaylaodHeader = 0;
			((PBT_TESTMODE_T)devExt->pTestMode)->PacketLength  = 0;
			status = STATUS_UNSUCCESSFUL;
			break;
			
		case 0: /* Pause Test Mode */
			((PBT_TESTMODE_T)devExt->pTestMode)->being_test    = 0;
			((PBT_TESTMODE_T)devExt->pTestMode)->old_whitening = ((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening;
			if (((PBT_TESTMODE_T)devExt->pTestMode)->present_whitening == DATA_WHITENING)
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x14);
			else
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TEST_ITEMS, pBuf, 1, 0x04);
			
			(((PBT_TESTMODE_T)devExt->pTestMode)->configuration).test_scenario = 0;
			status = STATUS_UNSUCCESSFUL;
			break;
			
		default :
			status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
			return status;
			break;
	}
	
	if (NT_SUCCESS(status))
	{
		RtlCopyMemory(&(((PBT_TESTMODE_T)devExt->pTestMode)->configuration), pLmpPdu->contents, sizeof(DUT_CONFIG));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOPPING_MODE, pBuf, 1, &(pLmpPdu->contents[1]));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_TX_FREQUENCY_DUT, pBuf, 1, &(pLmpPdu->contents[2]));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_RX_FREQUENCY_DUT, pBuf, 1, &(pLmpPdu->contents[3]));
	}

	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 3;
	pdu.contents[0] = 57;

	TestMode_FlushFIFO(devExt, pConnectDevice); 

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Test Control\n", pdu.TransID, pConnectDevice->bd_addr[0]);

	devExt->AllowCmdIndicator = FALSE;
	Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_WRITE_TEST_CMD_INDICATOR), BT_TASK_PRI_NORMAL, NULL, 0);
#else
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
#endif
	
	return status;
}

NTSTATUS LMP_encryption_key_size_mask_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength) /* 58 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): EncryptionKeySize Mask Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_encryption_key_size_mask_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength) /* 59 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): EncryptionKeySize Mask Res, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_set_AFH(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                    /* 60 */
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	UINT32 AFHInstant;
	UINT8 i = 0, j = 0, channel_num = 0;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->OpCode].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Set AFH, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if (pConnectDevice->current_role != BT_ROLE_SLAVE)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Set AFH, but local host isn't a slave\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	RtlCopyMemory(&AFHInstant, pLmpPdu->contents, 4);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Set AFH, instants= 0x%x, mode= %d, channel_map=\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], AFHInstant, pLmpPdu->contents[4]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.afh_cap_slave == 0)
	{
		status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pConnectDevice->afh_mode = pLmpPdu->contents[4];
	if (pLmpPdu->contents[4] == AFH_ENABLED)
	{
		for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "0x%x \n", pLmpPdu->contents[i]);
		
		pLmpPdu->contents[14] &= 0x7f;
		for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
		{
			for (j = 0; j < 8; j++)
				if (((pLmpPdu->contents[i] >> j) & 0x1) == 1)
					channel_num++;
		}
		
		if (pConnectDevice->slave_index == 0)
		{
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_NUM_SLAVE0, pBuf, 1, &channel_num);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_MAP_SLAVE0, pBuf, BT_MAX_CHANNEL_MAP_NUM, &(pLmpPdu->contents[5]));
		}
		else
		{
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_NUM_SLAVE1, pBuf, 1, &channel_num);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_MAP_SLAVE1, pBuf, BT_MAX_CHANNEL_MAP_NUM, &(pLmpPdu->contents[5]));
		}
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_AFH_MODE + 1, pBuf, 1, (0x2 << (pConnectDevice->slave_index << 1)));
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_AFH_MODE + 1, pBuf, 1, (~(0x3 << (pConnectDevice->slave_index << 1))));
		pConnectDevice->send_classification_flag = AFH_REPORTING_DISABLED;
		pConnectDevice->classification_interval  = 30; /* 30s */
	}
	
	if (pConnectDevice->slave_index == 0)
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_INSTANT_SLAVE0, pBuf, 4, AFHInstant << 1);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CHANGE_SLAVE0_AFH_MODE_BIT);
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_INSTANT_SLAVE1, pBuf, 4, AFHInstant << 1);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CHANGE_SLAVE1_AFH_MODE_BIT);
	}
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	return status;
}

NTSTATUS LMP_encapsulated_header(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)          /* 61 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Encapsulated Header, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_encapsulated_payload(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)         /* 62 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Encapsulated Payload, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_simple_pairing_confirm(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)       /* 63 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Simple Pairing Confirm, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_simple_pairing_number(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 64 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Simple Pairing Number, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_DHkey_check(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                  /* 65 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): DHkey Check, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAccepted(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	
	return status;
}

NTSTATUS LMP_accepted_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)               /* 127/01 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Ext Accepted, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (pLmpPdu->contents[1] == ESCAPE_OPCODE4)
	{
		switch (pLmpPdu->contents[2])
		{
			case 12: /* eSCO Link Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_ESCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ESCOLINK_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Ext Accepted eSCO Link Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
				status = STATUS_SUCCESS;
				
				After_eSCO_Complete(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11]);
				((PBT_LMP_T)devExt->pLmp)->esco_counter = 0;
				break;
				
			case 13: /* Remove eSCO Link Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_REMOVE_ESCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Ext Accepted Remove eSCO Link Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
				status = STATUS_SUCCESS;
				
#ifdef BT_TESTMODE_SUPPORT
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
				DeleteESCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
				
			case 11: /* Packet Type Table Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_PACKET_TYPE_TABLE_REQ)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Ext Accepted Packet Type Table Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
				status = STATUS_SUCCESS;
				
				Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
				pConnectDevice->edr_mode = (pConnectDevice->edr_mode + 1) % 2;
				Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_EDR_MODE_CHANGE), BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
				
			default :
				BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2]);
				status = STATUS_SUCCESS;
				break;
		}
		
		if (!NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, unexpected or operate Error\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2]);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Received Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2]);
		status = STATUS_SUCCESS;
	}
	
	return status;
}

NTSTATUS LMP_not_accepted_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)           /* 127/02 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER	timevalue;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if (pLmpPdu->contents[1] == ESCAPE_OPCODE4)
	{
		switch (pLmpPdu->contents[2])
		{
			case 12: /* eSCO Link Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_ESCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ESCOLINK_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted eSCO Link Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;
				
				if (((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == ESCO_ADD_LINK)
				{
					RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_ESCO_T));
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else if (((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == ESCO_CHANGE_LINK)
				{
					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				
				((PBT_LMP_T)devExt->pLmp)->esco_counter = 0;
				break;
				
			case 13: /* Remove eSCO Link Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_REMOVE_ESCO_LINK_REQ || ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted Remove eSCO Link Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;
				
#ifdef BT_TESTMODE_SUPPORT
				pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
				DeleteESCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
				
			case 16: /* Channel Classification Req */
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted Channel Classification Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				pConnectDevice->is_afh_sent_flag = 0;
				status = STATUS_SUCCESS;
				break;
				
			case 11: /* Packet Type Table Req */
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_SEND_PACKET_TYPE_TABLE_REQ)
					break;
				
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted Packet Type Table Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;
				
				pConnectDevice->edr_change_flag = 0;
				UsbQueryDMASpace(devExt);
				break;
				
			case 23: /* Pause Encryption Req */
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_SEND_PAUSEENCRYPTION_REQ && (pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STOPEN_REQ)
					break;
				
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted Pause Encryption Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;
				
				if (pLmpPdu->contents[3] == ERROR_TRANSACTION_COLLISION && pConnectDevice->current_role == BT_ROLE_SLAVE)
					break;
				
				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PAUSEENCRYPTION_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);
				
				pConnectDevice->is_in_encryption_process = 0;

				if (pConnectDevice->pause_encryption_status == 2)
				{
					if (pConnectDevice->pause_command_flag == 1)
					{
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					}
					else
					{
						pConnectDevice->current_reason_code = pLmpPdu->contents[3];
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					}
				}
				
				pConnectDevice->pause_encryption_status = 0;
				//Jakio20071220, check tx queue
				timevalue.QuadPart = 0;
				//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
				UsbQueryDMASpace(devExt);
				break;

			case 24: /* Resume Encryption Req */
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_WAIT_STARTEN_REQ || pConnectDevice->current_role != BT_ROLE_SLAVE)
					break;

				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Not Ext Accepted Resume Encryption Req, reason= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;

				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
				if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_RESUMEENCRYPTION_REQ)
					LMP_StopPDUTimer(devExt, pConnectDevice);

				pConnectDevice->is_in_encryption_process = 0;
				
				if (pConnectDevice->pause_encryption_status == 2 || pConnectDevice->pause_command_flag == 1)
				{
					pConnectDevice->current_reason_code = pLmpPdu->contents[3];
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				
				pConnectDevice->pause_encryption_status = 0;
				//Jakio20071220, check tx queue
				timevalue.QuadPart = 0;
				//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
				UsbQueryDMASpace(devExt);
				break;
				
			default :
				BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----Received Not Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, reason= 0x%x, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3]);
				status = STATUS_SUCCESS;
				break;
		}
		
		if (!NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Received Not Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, reason= 0x%x, unexpected or operate Error\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3]);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Received Not Ext Accepted PDU (TransID=%d, BD0=0x%x): EscapeOPCode= %d, ExtendedOPCode= %d, reason= 0x%x, unknown or unsupported\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3]);
		status = STATUS_SUCCESS;
	}
	
	return status;
}

NTSTATUS LMP_features_req_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)           /* 127/03 */
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;
	LMP_FEATURES_T	tmpLocalFeature;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Features Req Ext, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Features Req Ext, feature_page= %d, max_page= %d, remote feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7], pLmpPdu->contents[8], pLmpPdu->contents[9], pLmpPdu->contents[10]);
	
	pConnectDevice->max_page_number = pLmpPdu->contents[2];
	if (pLmpPdu->contents[1] == 0)
		RtlCopyMemory(&(pConnectDevice->lmp_features), &(pLmpPdu->contents[3]), sizeof(LMP_FEATURES_T));
	else if (pLmpPdu->contents[1] == 1)
		RtlCopyMemory(&(pConnectDevice->extended_lmp_features), &(pLmpPdu->contents[3]), sizeof(LMP_EXTEND_FEATURES_T));
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;;
	pdu.OpCode      = 127;
	pdu.contents[0] = 4;
	pdu.contents[1] = pLmpPdu->contents[1];
	pdu.contents[2] = BT_MAX_FEATURE_PAGE_NUMBER;
	if (pdu.contents[1] == 0)
	{
	#ifdef BT_COMBO_SNIFF_SUPPORT
		if(devExt->ComboState == FW_WORK_MODE_COMBO)		
		{
			RtlCopyMemory(&tmpLocalFeature, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
			tmpLocalFeature.byte0.sniff_mode = 0;
			RtlCopyMemory(&(pdu.contents[3]), &tmpLocalFeature, sizeof(LMP_FEATURES_T));
		}
		else
		{
			RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
		}
	#else
		RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
	#endif
	}
	else if (pdu.contents[1] == 1)
		RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->extended_lmp_features), sizeof(LMP_EXTEND_FEATURES_T));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 12);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Features Res Ext, feature_page= %d, max_page= %d, local feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7], pdu.contents[8], pdu.contents[9], pdu.contents[10]);
	}
	
	return status;
}

NTSTATUS LMP_features_res_ext(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)           /* 127/04 */
{
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Features Res Ext, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		return STATUS_SUCCESS;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Features Res Ext, feature_page= %d, max_page= %d, remote feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7], pLmpPdu->contents[8], pLmpPdu->contents[9], pLmpPdu->contents[10]);
	if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_FEATURES_REQ_EXT)
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	pConnectDevice->max_page_number = pLmpPdu->contents[2];
	if (pLmpPdu->contents[1] == 0)
		RtlCopyMemory(&(pConnectDevice->lmp_features), &(pLmpPdu->contents[3]), sizeof(LMP_FEATURES_T));
	else if (pLmpPdu->contents[1] == 1)
		RtlCopyMemory(&(pConnectDevice->extended_lmp_features), &(pLmpPdu->contents[3]), sizeof(LMP_EXTEND_FEATURES_T));
	
	if (pConnectDevice->current_page_number != 0xFF && pConnectDevice->current_page_number == pLmpPdu->contents[1])
	{
		RtlCopyMemory(&(pConnectDevice->remote_extended_feature), &(pLmpPdu->contents[3]), 8);
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_EXTENDED_FEATURE_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return STATUS_SUCCESS;
}

NTSTATUS LMP_packet_type_table_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)      /* 127/11 */
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Packet Type Table Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Packet Type Table Req, ppt= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_2 == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	if ((pConnectDevice->packet_type & BT_PACKET_TYPE_2DH1) != 0 || (pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_PARAMETER);
		return status;
	}
	if (pLmpPdu->contents[1] > 1)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_PACKET_TYPE_TABLE_REQ)
	{
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, ERROR_TRANSACTION_COLLISION);
			return status;
		}
		
		pConnectDevice->edr_change_flag = 0;
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);
	}
	
	if (pConnectDevice->edr_change_flag == 1)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	if (pLmpPdu->contents[1] != pConnectDevice->edr_mode)
	{
		Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
		pConnectDevice->edr_change_flag = 1;
		pConnectDevice->edr_mode = pLmpPdu->contents[1];
		Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_EDR_MODE_CHANGE), BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 1;
	pdu.contents[1] = 127;
	pdu.contents[2] = 11;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Ext Accepted Packet Type Table Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	return status;
}

NTSTATUS LMP_eSCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)              /* 127/12 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    indicator, i, checkvalue, ChangeFlag = 0;
	LMP_PUD_PACKAGE_T pdu;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): eSCO Link Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): eSCO Link Req, contents(byte-orders)= %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], 
		pLmpPdu->contents[1], pLmpPdu->contents[2], pLmpPdu->contents[3], pLmpPdu->contents[4], pLmpPdu->contents[5], pLmpPdu->contents[6], pLmpPdu->contents[7], 
		pLmpPdu->contents[8], pLmpPdu->contents[9], pLmpPdu->contents[10], pLmpPdu->contents[11], pLmpPdu->contents[12], pLmpPdu->contents[13], pLmpPdu->contents[14]);
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && ((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->slave_sco_master_not_coexist_flag == 1)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	LMP_SCONeedUnsniff(devExt, pConnectDevice);
	
	if (pLmpPdu->contents[13] >= SCO_MAX_AIRMODE_PARA || (((PBT_LMP_T)devExt->pLmp)->air_mode_feature & (1 << pLmpPdu->contents[13])) == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, SCO_AIR_MODE_REJECTED);
		return status;
	}
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && (pLmpPdu->contents[1] == 0 || pLmpPdu->contents[2] == 0 || pLmpPdu->contents[2] > 7))
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_ESCO_LINK_REQ)
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ESCOLINK_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);
	}
	
	indicator = ((PBT_LMP_T)devExt->pLmp)->sco_indicator;
	for (i = 0; i < (indicator >> 6); i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle == pLmpPdu->contents[1] && ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pConnectDevice)
			break;
	}
	if (i >= (indicator >> 6))
	{
		i = (UINT8)(indicator >> 6);
		if (i >= SCO_MAX_NUMBER)
		{
			status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, MAX_SCO_CONNECTIONS);
			return status;
		}
		
		if (((PBT_LMP_T)devExt->pLmp)->esco[i].change_flag != ESCO_ADD_LINK)
		{
			RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle), &(pLmpPdu->contents[1]), 14);
			((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice = pConnectDevice;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator   = (UINT8)(i + (indicator & 0xfc));
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_RECV_ESCO_LINK_REQ);
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_LINK_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			return STATUS_SUCCESS;
		}
		else
		{
			if (((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice != pConnectDevice || (pConnectDevice->current_role == BT_ROLE_MASTER && (((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle != pLmpPdu->contents[1] || ((PBT_LMP_T)devExt->pLmp)->esco[i].esco_LT_addr != pLmpPdu->contents[2])))
			{
				status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
				
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[i], sizeof(BT_ESCO_T));
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				return status;
			}
		}
	}
	
	RtlCopyMemory(&(((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle), &(pLmpPdu->contents[1]), 14);
	checkvalue = CheckESCOPara(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[i], &ChangeFlag);
	if (checkvalue != BT_HCI_STATUS_SUCCESS)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, checkvalue);
		
		if (((PBT_LMP_T)devExt->pLmp)->esco[i].change_flag == ESCO_ADD_LINK)
		{
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[i], sizeof(BT_ESCO_T));
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		else if (((PBT_LMP_T)devExt->pLmp)->esco[i].change_flag == ESCO_CHANGE_LINK)
		{
			((PBT_LMP_T)devExt->pLmp)->esco[i].change_flag = 0;
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		
		return status;
	}
	
	((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (indicator & 0xfc));
	if (ChangeFlag == 1)
	{
		status = Send_eSCOLinkReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
		return status;
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 1;
	pdu.contents[1] = 127;
	pdu.contents[2] = 12;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Ext Accepted eSCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	After_eSCO_Complete(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[i]);
	return status;
}

NTSTATUS LMP_remove_eSCO_link_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)       /* 127/13 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 i;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Remove eSCO Link Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Remove eSCO Link Req, eSCOHandle= %d, ErrorCode= 0x%x\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], pLmpPdu->contents[2]);
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 1;
	pdu.contents[1] = 127;
	pdu.contents[2] = 13;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Ext Accepted Remove eSCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle == pLmpPdu->contents[1] && ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pConnectDevice)
			break;
	}
	if (i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
	{
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		DeleteESCOLink(devExt, i);

		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = pLmpPdu->contents[2];
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type           = BT_LINK_TYPE_ESCO;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return status;
}

NTSTATUS LMP_channel_classification_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength) /* 127/16 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT16 MinInterval, MaxInterval;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length || pLmpPdu->TransID != MASTER_TRANSACTION_ID)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Channel Classification Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	RtlCopyMemory(&MinInterval, pLmpPdu->contents + 2, sizeof(UINT16));
	RtlCopyMemory(&MaxInterval, pLmpPdu->contents + 4, sizeof(UINT16));
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Channel Classification Req, ReportingMode= %d, MinInterval= %d, MaxInterval= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], pLmpPdu->contents[1], MinInterval, MaxInterval);
	
	if (pConnectDevice->current_role != BT_ROLE_SLAVE || pConnectDevice->afh_mode != AFH_ENABLED)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.afh_cla_slave == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pLmpPdu->contents[1] == AFH_REPORTING_ENABLED)
	{
		MaxInterval = (MaxInterval > 0xBB80) ? 0xBB80 : MaxInterval;
		MaxInterval = (MaxInterval < 0x0640) ? 0x0640 : MaxInterval;
		MinInterval = (MinInterval > 0xBB80) ? 0xBB80 : MinInterval;
		MinInterval = (MinInterval < 0x0640) ? 0x0640 : MinInterval;
		MaxInterval = (MaxInterval < MinInterval) ? MinInterval : MaxInterval;
		
		MaxInterval = (MaxInterval + MinInterval) / 2;
		if (MaxInterval % 1600 == 0)
			pConnectDevice->classification_interval  = (UINT8)(MaxInterval / 1600);
		else
			pConnectDevice->classification_interval  = (UINT8)(MaxInterval / 1600 + 1);
		
		pConnectDevice->send_classification_flag = AFH_REPORTING_ENABLED;
	}
	else
	{
		pConnectDevice->send_classification_flag = AFH_REPORTING_DISABLED;
		pConnectDevice->classification_interval  = 30; /* 30s */
	}
	
	return status;
}

NTSTATUS LMP_channel_classification(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)     /* 127/17 */
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    i = 0;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Channel Classification, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Channel Classification,  ChannelClassificationMap= \n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);

	if (pConnectDevice->current_role != BT_ROLE_MASTER || pConnectDevice->is_afh_sent_flag != 1 || pConnectDevice->afh_mode != AFH_ENABLED)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cla_master == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	for (i = 1; i < 1 + BT_MAX_CHANNEL_MAP_NUM; i++)
		BT_DBGEXT(ZONE_LMP | LEVEL3, "0x%x \n", pLmpPdu->contents[i]);
	
#ifdef BT_AFH_ADJUST_MAP_SUPPORT
	RtlCopyMemory(Afh_GetSlaveClaAddress(devExt, pConnectDevice), &(pLmpPdu->contents[1]), BT_MAX_CHANNEL_MAP_NUM);
#endif
	
	return status;
}

NTSTATUS LMP_sniff_subrating_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 127/21 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Sniff Subrating Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_sniff_subrating_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)        /* 127/22 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Sniff Subrating Res, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_pause_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)       /* 127/23 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Pause Encryption Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Pause Encryption Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.pause_encryption == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}

	if (pConnectDevice->encryption_mode == BT_ENCRYPTION_DIABLE)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}

	pConnectDevice->is_in_encryption_process = 1;
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_WAIT_STOPEN_REQ)
		{
			if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PAUSEENCRYPTION_REQ)
				LMP_StopPDUTimer(devExt, pConnectDevice);
		}
		
		if (pConnectDevice->pause_encryption_status == 0)
			pConnectDevice->pause_encryption_status = 1;
		
		status = Send_PauseEncryptionReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
	}
	else
	{
		if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_PAUSEENCRYPTION_REQ && pLmpPdu->TransID == SLAVE_TRANSACTION_ID)
		{
			status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, ERROR_TRANSACTION_COLLISION);
			return status;
		}

		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_PAUSEENCRYPTION_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);

		if (pConnectDevice->pause_encryption_status == 0)
			pConnectDevice->pause_encryption_status = 1;

		pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
		status = Send_StopEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
	}
	
	return status;
}

NTSTATUS LMP_resume_encryption_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)      /* 127/24 */
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (PduLength != LMP_PDU_Rx_Data[pLmpPdu->contents[0] + ESCAPE_OPCODE4_OFFSET].PDU_Length)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----Error PDU Received (TransID=%d, BD0=0x%x): Resume Encryption Req, length= %d\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0], PduLength);
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, INVALID_LMP_PARAMETERS);
		return status;
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): Resume Encryption Req\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.pause_encryption == 0)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
		return status;
	}
	
	if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE || pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, PDU_NOT_ALLOWED);
		return status;
	}
	
	pConnectDevice->encryption_mode = BT_ENCRYPTION_ONLY_P2P;
	status = Send_StartEnReq_PDU(devExt, pConnectDevice, pLmpPdu->TransID);
	return status;
}

NTSTATUS LMP_IO_capability_req(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)          /* 127/25 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): IO Capability Req, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_IO_capability_res(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)          /* 127/26 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): IO Capability Res, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_numeric_comparison_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)  /* 127/27 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Numeric Comparison Failed, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_passkey_entry_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)       /* 127/28 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Received (TransID=%d, BD0=0x%x): Passkey Entry Failed, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_OOB_failed(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)                 /* 127/29 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): OOB Failed, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

NTSTATUS LMP_keypress_notification(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)      /* 127/30 */
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	PduLength = PduLength;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Received (TransID=%d, BD0=0x%x): KeyPress Notification, but unsupported now\n", pLmpPdu->TransID, pConnectDevice->bd_addr[0]);
	status = ResLmpNotAcceptedExt(devExt, pConnectDevice, pLmpPdu, UNSUPPORTED_LMP_FEATURE);
	return status;
}

/*---------------------------------------------------------------------*/

void ChangeLmpState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state)
{
	KIRQL  oldIrql;
	UINT16 tempstate;

	if (state > LMP_STATE_ANDVALUE)
		tempstate = pConnectDevice->lmp_states | state;
	else if (state > LMP_CONN_IDLE)
		tempstate = (pConnectDevice->lmp_states & (~LMP_STATE_ANDVALUE)) + state;
	else
		tempstate = state;
	
	pConnectDevice->lmp_states = tempstate;
	
	return;
}

void ChangeLmpExtraState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state)
{
	KIRQL  oldIrql;
	UINT16 tempstate;

	if (state > LMP_STATE_ANDVALUE)
		tempstate = pConnectDevice->lmp_ext_states | state;
	else if (state > LMP_CONN_IDLE)
		tempstate = (pConnectDevice->lmp_ext_states & (~LMP_STATE_ANDVALUE)) + state;
	else
		tempstate = state;
	
	pConnectDevice->lmp_ext_states = tempstate;
	
	return;
}

void CancelLmpBitState(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 state, UINT8 extra_flag)
{
	KIRQL  oldIrql;

	if (extra_flag == 0)
		pConnectDevice->lmp_states &= (~state);
	else
		pConnectDevice->lmp_ext_states &= (~state);

	return;
}

NTSTATUS ResLmpNotAccepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 ErrReason)
{
	NTSTATUS          status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 4;
	pdu.contents[0] = pLmpPdu->OpCode;
	pdu.contents[1] = ErrReason;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Not Accepted, OPCode: %d, ErrReason: 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0], pdu.contents[1]);
	
	return status;
}

NTSTATUS ResLmpNotAcceptedExt(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 ErrReason)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = pLmpPdu->TransID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 2;
	pdu.contents[1] = pLmpPdu->OpCode;
	pdu.contents[2] = pLmpPdu->contents[0];
	pdu.contents[3] = ErrReason;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 5);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Not Ext Accepted, EscapeOPCode: %d, ExtendedOPCode: %d, ErrReason: 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], pLmpPdu->OpCode, pLmpPdu->contents[0], ErrReason);
	
	return status;
}

NTSTATUS ResLmpDetach(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ErrReason)
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 i;
	
	if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_DETACH);
		if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
			return STATUS_SUCCESS;
		
		for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
		{
			if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
				break;
		}
		if (i < ((PBT_LMP_T)devExt->pLmp)->sniff_number)
		{
			((PBT_LMP_T)devExt->pLmp)->sniff_index = i;
			status = Send_UnsniffReq_PDU(devExt, pConnectDevice);
			return status;
		}
		
		pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode      = 7;
	pdu.contents[0] = ErrReason;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
		LMP_ReleaseSCOMembers(devExt, pConnectDevice);
#ifdef BT_TESTMODE_SUPPORT
		TestMode_ResetMembers(devExt, pConnectDevice);
#endif
		LMP_StopPDUTimer(devExt, pConnectDevice);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Detach, reason: 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], ErrReason);
		
		if (pConnectDevice->tempflag != 1)
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return status;
}

NTSTATUS Res_InRand_Accepted(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status = STATUS_SUCCESS;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 3;
	pdu.contents[0] = 8;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_STATE_ANDVALUE);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Initialization Rand\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}

	/* link_key holds the random casually */
	status = E22_InitAndMaster_KeyGen(pConnectDevice->link_key, pConnectDevice->pin_code, pConnectDevice->pin_code_length, ((PBT_HCI_T)devExt->pHci)->local_bd_addr, pConnectDevice->init_key);
	if (!NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----E22_InitAndMaster_KeyGen function operates ERROR\n");
		pConnectDevice->current_reason_code = AUTHENTICATION_FAILURE;
		status = ResLmpDetach(devExt, pConnectDevice, AUTHENTICATION_FAILURE);
		return status;
	}

	if (transid == ((pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID))
		status = Send_CombKey_PDU(devExt, pConnectDevice, transid);
	else
		ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_COM_KEY);

	return status;
}

/* return value: 0(HV1); 1(HV2); 2(HV3) */
UINT8 GetDefaultSCOPacket(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 feature, feature2;
	
	if (devExt == NULL || pConnectDevice == NULL)
		return 0;

	if (((PBT_HCI_T)devExt->pHci)->only_use_hv3_flag == 1)
		return 2;

	RtlCopyMemory(&feature, &(((PBT_HCI_T)devExt->pHci)->lmp_features.byte1), 1);
	RtlCopyMemory(&feature2, &(pConnectDevice->lmp_features.byte1), 1);
	feature = feature & feature2;
	feature = (UINT8)((feature >> 3) & (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->packet_type >> 5));
	if ((feature & 0x04) != 0)
		return 2;
	else if ((feature & 0x02) != 0)
		return 1;
	else
		return 0;
}

/* flag: 0(master/add); 1(master/change); 2(slave/add); 3(slave/change) */
UINT8 CheckSCOPara(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_SCO_T psco, UINT8 flag)
{
	UINT8 feature;
	
	if (psco->air_mode >= SCO_MAX_AIRMODE_PARA || (((PBT_LMP_T)devExt->pLmp)->air_mode_feature & (1 << psco->air_mode)) == 0)
	{
		if (flag == 0)
			psco->air_mode = SCO_DEFAULT_AIRMODE;
		else
			return SCO_AIR_MODE_REJECTED;
	}
	
	RtlCopyMemory(&feature, &(((PBT_HCI_T)devExt->pHci)->lmp_features.byte1), 1);
	if (flag == 0 || flag == 2)
		feature = (UINT8)((feature >> 3) & (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->packet_type >> 5));
	else
		feature = feature >> 3;
	if (psco->sco_packet >= SCO_MAX_PACKET_PARA || (feature & (1 << psco->sco_packet)) == 0 
		|| (((PBT_HCI_T)devExt->pHci)->only_use_hv3_flag == 1 && psco->sco_packet != 2))
	{
		if (flag == 0)
			psco->sco_packet = GetDefaultSCOPacket(devExt, pConnectDevice);
		else
			return SCO_INTERVAL_REJECTED;
	}
	
	if (psco->T_sco != ((psco->sco_packet + 1) << 1))
	{
		if (flag == 0)
			psco->T_sco = (UINT8)((psco->sco_packet + 1) << 1);
		else
			return INVALID_LMP_PARAMETERS;
	}
	
	if (flag == 2 || flag == 3)
		if (psco->D_sco >= psco->T_sco || (psco->D_sco % 2) != 0)
			return INVALID_LMP_PARAMETERS;
		
		return 0;
}

void DeleteSCOLink(PBT_DEVICE_EXT devExt, UINT8 index)
{
	UINT8 i, j;

	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		if (i == index)
		{
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[i], sizeof(BT_SCO_T));
			for (j = i + 1; j < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); j++)
			{
				RtlCopyMemory(&((PBT_LMP_T)devExt->pLmp)->sco[j - 1], &((PBT_LMP_T)devExt->pLmp)->sco[j], sizeof(BT_SCO_T));
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[j], sizeof(BT_SCO_T));
			}

			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) - 1) << 6);
			break;
		}
	} /* end for */

	return;
}

/* Return Value: 1 discard this sco_link_req; 2 retry sco_link_req with new para */
UINT8 SCONotAccepted_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ErrorReason)
{
	UINT8 status, i;
	UINT8 indicator, airmode_counter, packettype_counter, feature, feature2;

	indicator          = ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11;
	airmode_counter    = (UINT8)(((PBT_LMP_T)devExt->pLmp)->sco_counter & 0x0f);
	packettype_counter = (UINT8)(((PBT_LMP_T)devExt->pLmp)->sco_counter >> 4);
	if (ErrorReason == SCO_AIR_MODE_REJECTED)
	{
		airmode_counter++;
		for (i = ((PBT_LMP_T)devExt->pLmp)->sco[indicator].air_mode + 1; ; i++)
		{
			if (airmode_counter >= SCO_MAX_AIRMODE_PARA)
			{
				status = 1;
				break;
			}
			
			if ((((PBT_LMP_T)devExt->pLmp)->air_mode_feature & (1 << (i % SCO_MAX_AIRMODE_PARA))) == 0)
			{
				airmode_counter++;
			}
			else
			{
				((PBT_LMP_T)devExt->pLmp)->sco[indicator].air_mode = (UINT8)(i % SCO_MAX_AIRMODE_PARA);
				status = 2;
				break;
			}
		} /* end for() */
	}
	else if (ErrorReason == SCO_INTERVAL_REJECTED)
	{
		packettype_counter++;
		RtlCopyMemory(&feature, &(((PBT_HCI_T)devExt->pHci)->lmp_features.byte1), 1);
		RtlCopyMemory(&feature2, &(pConnectDevice->lmp_features.byte1), 1);
		feature = feature & feature2;
		feature = (UINT8)((feature >> 3) & (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->packet_type >> 5));
		for (i = ((PBT_LMP_T)devExt->pLmp)->sco[indicator].sco_packet + 1; ; i++)
		{
			if (packettype_counter >= SCO_MAX_PACKET_PARA)
			{
				status = 1;
				break;
			}
			
			if ((feature & (1 << (i % SCO_MAX_PACKET_PARA))) == 0)
			{
				packettype_counter++;
			}
			else
			{
				((PBT_LMP_T)devExt->pLmp)->sco[indicator].sco_packet = (UINT8)(i % SCO_MAX_PACKET_PARA);
				status = 2;
				break;
			}
		} /* end for() */
	}
	else if (ErrorReason == SCO_OFFSET_REJECTED)
	{
		if ((devExt->ComboState == FW_WORK_MODE_COMBO) && (devExt->chipID != 0)) /* V4 Combo mode */
		{
			status = 1;
		}
		else
		{
			if (((PBT_LMP_T)devExt->pLmp)->sco[indicator].D_sco + 2 >= ((((PBT_LMP_T)devExt->pLmp)->sco[indicator].sco_packet + 1) << 1))
			{
				status = 1;
			}
			else
			{
				((PBT_LMP_T)devExt->pLmp)->sco[indicator].D_sco += 2;
				status = 2;
			}
		}
	}
	else
	{
		/* PDU_NOT_ALLOWED, MAX_SCO_CONNECTIONS, INVALID_LMP_PARAMETERS, user reason */
		status = 1;
	}

	((PBT_LMP_T)devExt->pLmp)->sco_counter = (UINT8)(airmode_counter + (packettype_counter << 4));
	return status;
}

void CalculateBandwidth(PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco)
{
	UINT16 remainder;
	
	if (pConnectDevice == NULL || pConnectDevice->pScoConnectDevice == NULL || pEsco == NULL)
		return ;
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth == 0xFFFF)
		{
			remainder = 0;
			if (((pEsco->packet_len_m2s * 1600) % pEsco->T_esco) > (pEsco->T_esco / 3))
				remainder = 1;
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth = ((pEsco->packet_len_m2s * 1600) / pEsco->T_esco) + remainder;
		}
		
		if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth == 0xFFFF)
		{
			remainder = 0;
			if (((pEsco->packet_len_s2m * 1600) % pEsco->T_esco) > (pEsco->T_esco / 3))
				remainder = 1;
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth = ((pEsco->packet_len_s2m * 1600) / pEsco->T_esco) + remainder;
		}
	}
	else
	{
		if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth == 0xFFFF)
		{
			remainder = 0;
			if (((pEsco->packet_len_s2m * 1600) % pEsco->T_esco) > (pEsco->T_esco / 3))
				remainder = 1;
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth = ((pEsco->packet_len_s2m * 1600) / pEsco->T_esco) + remainder;
		}
		
		if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth == 0xFFFF)
		{
			remainder = 0;
			if (((pEsco->packet_len_m2s * 1600) % pEsco->T_esco) > (pEsco->T_esco / 3))
				remainder = 1;
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth = ((pEsco->packet_len_m2s * 1600) / pEsco->T_esco) + remainder;
		}
	}
	
	return ;
}

UINT16 CalculatePacketLen(UINT32 bandwidth, UINT16 slot_num)
{
	UINT16 remainder = 0;
	
	if ((bandwidth % slot_num) > (UINT16)(slot_num / 3))
		remainder = 1;
	
	return (UINT16)(bandwidth / slot_num + remainder);
}

UINT8 GetESCOPacketReg(UINT8 PacketM, UINT8 PacketL)
{
	UINT8  Packet[] = {BT_ESCO_PACKET_NULL, BT_ESCO_PACKET_POLL, BT_ESCO_PACKET_EV3, BT_ESCO_PACKET_EV4, BT_ESCO_PACKET_EV5, ESCO_PACKET_2EV3, ESCO_PACKET_3EV3, ESCO_PACKET_2EV5, ESCO_PACKET_3EV5};
	UINT8  i, j;
	
	for (i = 0; i < sizeof(Packet); i++)
	{
		if (PacketM == Packet[i])
			break;
	}
	if (i >= sizeof(Packet))
		i = 0;
	
	for (j = 0; j < sizeof(Packet); j++)
	{
		if (PacketL == Packet[j])
			break;
	}
	if (j >= sizeof(Packet))
		j = 0;
	
	return (UINT8)((i << 4) + j);
}

UINT8 GetESCORetransReg(UINT8 PacketM, UINT8 PacketL, UINT8 RetransSlot)
{
	UINT8  Packet[] = {BT_ESCO_PACKET_NULL, BT_ESCO_PACKET_POLL, BT_ESCO_PACKET_EV3, BT_ESCO_PACKET_EV4, BT_ESCO_PACKET_EV5, ESCO_PACKET_2EV3, ESCO_PACKET_3EV3, ESCO_PACKET_2EV5, ESCO_PACKET_3EV5};
	UINT8  slot[]   = {1, 1, 1, 3, 3, 1, 1, 3, 3};
	UINT8  i, j;
	
	for (i = 0; i < sizeof(Packet); i++)
	{
		if (PacketM == Packet[i])
			break;
	}
	if (i >= sizeof(Packet))
		i = 0;
	
	for (j = 0; j < sizeof(Packet); j++)
	{
		if (PacketL == Packet[j])
			break;
	}
	if (j >= sizeof(Packet))
		j = 0;
	
	return (UINT8)(RetransSlot / (slot[i] + slot[j]));
}

UINT8 GetValidESCOPacketFeature(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT16 pPacketFeature)
{
	UINT16 LocalPacketFeature = 0, airmode = 0, T_latency = 0;
	UINT8  Tesco[] = {6, 16, 16, 6, 6, 16, 16}; /* slot */
	UINT8  i;
	
	if (devExt == NULL || pConnectDevice == NULL || pConnectDevice->pScoConnectDevice == NULL)
		return UNSPECIFIED_ERROR;
	
	/* get local esco packet feature */
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 1)
		LocalPacketFeature |= 0x0008;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.ev4 == 1)
		LocalPacketFeature |= 0x0010;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.ev5 == 1)
		LocalPacketFeature |= 0x0020;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.enh_rate_esco_2 == 1)
		LocalPacketFeature |= 0x0040;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.enh_rate_esco_3 == 1)
		LocalPacketFeature |= 0x0080;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot3_enh_esco == 1)
		LocalPacketFeature |= 0x0300;
	
	/* get the valid airmode on voice_setting */
	if ((((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->voice_setting & 0x0003) == 0x0000)
	{
		if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.cvsd == 0 || pConnectDevice->lmp_features.byte2.cvsd == 0)
			return SCO_AIR_MODE_REJECTED;
		airmode = 0x00C8;
	}
	else if ((((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->voice_setting & 0x0003) == 0x0001)
	{
		if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.u_law == 0 || pConnectDevice->lmp_features.byte1.u_law == 0)
			return SCO_AIR_MODE_REJECTED;
		airmode = 0x00C8;
	}
	else if ((((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->voice_setting & 0x0003) == 0x0010)
	{
		if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.a_law == 0 || pConnectDevice->lmp_features.byte1.a_law == 0)
			return SCO_AIR_MODE_REJECTED;
		airmode = 0x00C8;
	}
	else
	{
		if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.trans_sco == 0 || pConnectDevice->lmp_features.byte2.trans_sco == 0)
			return SCO_AIR_MODE_REJECTED;
		airmode = 0x03F8;
	}
	
	/* get the valid Tesco on max_latency */
	for (i = 0; i < sizeof(Tesco); i++)
	{
		if ((((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->max_latency * 16) >= (Tesco[i] * 10))
			T_latency |= (0x0008 << i);
	}
	
	*pPacketFeature = LocalPacketFeature & ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_packet_type & airmode & T_latency;
	return BT_HCI_STATUS_SUCCESS;
}

UINT8 GetDefaultESCOParameters(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco)
{
	UINT16 ValidPacketFeature = 0, RemotePacketFeature = 0, Rate_bandwidth = 0, PacketFeature = 0;
	UINT32  bandwidth;
	UINT8  Packet[]   = {BT_ESCO_PACKET_EV3, BT_ESCO_PACKET_EV4, BT_ESCO_PACKET_EV5, ESCO_PACKET_2EV3, ESCO_PACKET_3EV3, ESCO_PACKET_2EV5, ESCO_PACKET_3EV5};
	UINT8  Tesco[]    = {6, 16, 16, 6, 6, 16, 16}; /* slot */
	UINT16 Rate[]     = {8000, 12000, 18000, 16000, 24000, 36000, 54000}; /* Bytes/second */
	UINT8  codemode[] = {BT_AIRMODE_CVSD, BT_AIRMODE_MU_LAW, BT_AIRMODE_A_LAW, BT_AIRMODE_TRANS};
	INT8  i;
	
	if (devExt == NULL || pConnectDevice == NULL || pEsco == NULL || pConnectDevice->pScoConnectDevice == NULL)
		return UNSPECIFIED_ERROR;
	
	i = GetValidESCOPacketFeature(devExt, pConnectDevice, &ValidPacketFeature);
	if (i != BT_HCI_STATUS_SUCCESS)
		return i;
	
	/* get remote esco packet feature */
	if (pConnectDevice->lmp_features.byte3.ext_sco_link == 1)
		RemotePacketFeature |= 0x0008;
	if (pConnectDevice->lmp_features.byte4.ev4 == 1)
		RemotePacketFeature |= 0x0010;
	if (pConnectDevice->lmp_features.byte4.ev5 == 1)
		RemotePacketFeature |= 0x0020;
	if (pConnectDevice->lmp_features.byte5.enh_rate_esco_2 == 1)
		RemotePacketFeature |= 0x0040;
	if (pConnectDevice->lmp_features.byte5.enh_rate_esco_3 == 1)
		RemotePacketFeature |= 0x0080;
	if (pConnectDevice->lmp_features.byte5.slot3_enh_esco == 1)
		RemotePacketFeature |= 0x0300;
	
	if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth >= ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth)
		bandwidth = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth;
	else
		bandwidth = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth;
	for (i = sizeof(Rate) - 1; i >= 0; )
	{
		if (bandwidth <= Rate[i])
		{
			Rate_bandwidth = 0x0008 << i;
			PacketFeature  = ValidPacketFeature & RemotePacketFeature & Rate_bandwidth;
			if (PacketFeature != 0)
				break;
		}
		
		if (i == 0)
			return PARAMETER_OUT_RANGE;
		else
			i--;
	}
	
	pEsco->packet_type_m2s = Packet[i];
	pEsco->packet_type_s2m = Packet[i];
	pEsco->T_esco          = Tesco[i];
	pEsco->air_mode        = codemode[((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->voice_setting & 0x0003];
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		pEsco->packet_len_m2s = CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth * Tesco[i], 1600);
		pEsco->packet_len_s2m = CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth * Tesco[i], 1600);
	}
	else
	{
		pEsco->packet_len_m2s = CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth * Tesco[i], 1600);
		pEsco->packet_len_s2m = CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth * Tesco[i], 1600);
	}
	
	if (pEsco->packet_len_m2s == 0)
		pEsco->packet_type_m2s = BT_ESCO_PACKET_POLL;
	if (pEsco->packet_len_s2m == 0)
		pEsco->packet_type_s2m = BT_ESCO_PACKET_NULL;
	
	return BT_HCI_STATUS_SUCCESS;
}

void DeleteESCOLink(PBT_DEVICE_EXT devExt, UINT8 index)
{
	UINT8 i, j;
	
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		if (i == index)
		{
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[i], sizeof(BT_ESCO_T));
			for (j = i + 1; j < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); j++)
			{
				RtlCopyMemory(&((PBT_LMP_T)devExt->pLmp)->esco[j - 1], &((PBT_LMP_T)devExt->pLmp)->esco[j], sizeof(BT_ESCO_T));
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[j], sizeof(BT_ESCO_T));
			}
			
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) - 1) << 6);
			break;
		}
	} /* end for */
	
	return;
}

/* pChangeFlag: 0(accept para); 1(change para) */
UINT8 CheckESCOPara(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco, PUINT8 pChangeFlag)
{
	UINT16 ValidPacketFeature = 0;
	UINT8  i, GetDefaultFlag;
	UINT8  Packet[]   = {BT_ESCO_PACKET_EV3, BT_ESCO_PACKET_EV4, BT_ESCO_PACKET_EV5, ESCO_PACKET_2EV3, ESCO_PACKET_3EV3, ESCO_PACKET_2EV5, ESCO_PACKET_3EV5};
	UINT8  codemode[] = {BT_AIRMODE_CVSD, BT_AIRMODE_MU_LAW, BT_AIRMODE_A_LAW, BT_AIRMODE_TRANS};
	UINT16 Length[]   = {30, 120, 180, 60, 90, 360, 540};
	
	if (devExt == NULL || pConnectDevice == NULL || pConnectDevice->pScoConnectDevice == NULL)
		return UNSPECIFIED_ERROR;
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if (pEsco->packet_len_m2s != CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth * pEsco->T_esco, 1600) || 
			pEsco->packet_len_s2m != CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth * pEsco->T_esco, 1600))
			return PARAMETER_OUT_RANGE;
	}
	else
	{
		if (pEsco->packet_len_m2s != CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->receive_bandwidth * pEsco->T_esco, 1600) || 
			pEsco->packet_len_s2m != CalculatePacketLen(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmit_bandwidth * pEsco->T_esco, 1600))
			return PARAMETER_OUT_RANGE;
	}
	
	if (pEsco->air_mode != codemode[((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->voice_setting & 0x0003])
		return SCO_AIR_MODE_REJECTED;
	
	pEsco->negotiation_state = ESCO_NEGO_PREFERRED;
	*pChangeFlag = 0;
	
	if (pEsco->packet_len_m2s == 0 && pEsco->packet_type_m2s != BT_ESCO_PACKET_POLL)
	{
		pEsco->packet_type_m2s   = BT_ESCO_PACKET_POLL;
		pEsco->negotiation_state = ESCO_NEGO_PREFERRED;
		*pChangeFlag = 1;
	}
	
	if (pEsco->packet_len_s2m == 0 && pEsco->packet_type_s2m != BT_ESCO_PACKET_NULL)
	{
		pEsco->packet_type_s2m   = BT_ESCO_PACKET_NULL;
		pEsco->negotiation_state = ESCO_NEGO_PREFERRED;
		*pChangeFlag = 1;
	}
	
	if (pEsco->D_esco > (pEsco->T_esco - 2) || pEsco->D_esco % 2 != 0)
	{
		pEsco->D_esco = ESCO_DEFAULT_D;
		pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
		*pChangeFlag = 1;
	}
	
	if (pEsco->W_esco > 6 || pEsco->W_esco % 2 != 0)
	{
		pEsco->W_esco = ESCO_DEFAULT_W;
		pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
		*pChangeFlag = 1;
	}
	
	GetDefaultFlag = 0;
	if (pEsco->packet_len_m2s != 0)
	{
		for (i = 0; i < sizeof(Packet); i++)
		{
			if (pEsco->packet_type_m2s == Packet[i])
				break;
		}
		if (i >= sizeof(Packet))
		{
			pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
			*pChangeFlag   = 1;
			GetDefaultFlag = 1;
		}
		else
		{
			if (pEsco->packet_len_m2s > Length[i])
			{
				pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
				*pChangeFlag   = 1;
				GetDefaultFlag = 1;
			}
			else
			{
				GetValidESCOPacketFeature(devExt, pConnectDevice, &ValidPacketFeature);
				if ((ValidPacketFeature & (0x0008 << i)) == 0)
				{
					pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
					*pChangeFlag   = 1;
					GetDefaultFlag = 1;
				}
			}
		}
		
		if (GetDefaultFlag == 1)
			return  GetDefaultESCOParameters(devExt, pConnectDevice, pEsco);
	}
	
	GetDefaultFlag = 0;
	if (pEsco->packet_len_s2m != 0)
	{
		for (i = 0; i < sizeof(Packet); i++)
		{
			if (pEsco->packet_type_s2m == Packet[i])
				break;
		}
		if (i >= sizeof(Packet))
		{
			pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
			*pChangeFlag   = 1;
			GetDefaultFlag = 1;
		}
		else
		{
			if (pEsco->packet_len_s2m > Length[i])
			{
				pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
				*pChangeFlag   = 1;
				GetDefaultFlag = 1;
			}
			else
			{
				GetValidESCOPacketFeature(devExt, pConnectDevice, &ValidPacketFeature);
				if ((ValidPacketFeature & (0x0008 << i)) == 0)
				{
					pEsco->negotiation_state = ESCO_NEGO_NO_SUPPORTED;
					*pChangeFlag   = 1;
					GetDefaultFlag = 1;
				}
			}
		}
		
		if (GetDefaultFlag == 1)
			return  GetDefaultESCOParameters(devExt, pConnectDevice, pEsco);
	}
	
	if (pEsco->T_esco < 6 || pEsco->T_esco > 18 || pEsco->T_esco % 2 != 0 || pEsco->T_esco > ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->max_latency)
		return  GetDefaultESCOParameters(devExt, pConnectDevice, pEsco);
	
	return BT_HCI_STATUS_SUCCESS;
}

void After_eSCO_Complete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PBT_ESCO_T pEsco)
{
	UINT8 role_flag;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco                 = pEsco->D_esco;
	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmission_interval = pEsco->T_esco;
	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->retransmission_window = pEsco->W_esco;
	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode              = pEsco->air_mode;
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type         = pEsco->packet_type_m2s;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_esco_rx_packet_type = pEsco->packet_type_s2m;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->rx_packet_length            = pEsco->packet_len_s2m;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->tx_packet_length            = pEsco->packet_len_m2s;
	}
	else
	{
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type         = pEsco->packet_type_s2m;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_esco_rx_packet_type = pEsco->packet_type_m2s;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->rx_packet_length            = pEsco->packet_len_m2s;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->tx_packet_length            = pEsco->packet_len_s2m;
	}
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
		role_flag = 1;
	else
		role_flag = (UINT8)(pConnectDevice->slave_index << 1);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AM_SCO, pBuf, 1, pConnectDevice->am_addr + (role_flag << 4));
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SECOND_LT_ADDR, pBuf, 1, &pEsco->esco_LT_addr);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_D_SCO, pBuf, 1, (((((pEsco->timing_control_flags >> 1) & 0x01) + 1) << 6) + pEsco->D_esco));
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_T_SCO, pBuf, 1, &pEsco->T_esco);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, GetESCORetransReg(pEsco->packet_type_m2s, pEsco->packet_type_s2m, pEsco->W_esco));
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_PACKET_TYPE, pBuf, 1, GetESCOPacketReg(pEsco->packet_type_m2s, pEsco->packet_type_s2m)); /* LSB4: rx  MSB4: tx */
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_RX_LENGTH, pBuf, 2, (PUINT8)&pEsco->packet_len_s2m);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_TX_LENGTH, pBuf, 2, (PUINT8)&pEsco->packet_len_m2s);
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_PACKET_TYPE, pBuf, 1, GetESCOPacketReg(pEsco->packet_type_s2m, pEsco->packet_type_m2s)); /* LSB4: rx  MSB4: tx */
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_RX_LENGTH, pBuf, 2, (PUINT8)&pEsco->packet_len_m2s);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ESCO_TX_LENGTH, pBuf, 2, (PUINT8)&pEsco->packet_len_s2m);
	}
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
	if (pEsco->change_flag == ESCO_ADD_LINK)
	{
		pEsco->change_flag = 0;
		((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) + 1) << 6);
		pEsco->pDevice = pConnectDevice;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CONNECTED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	else
	{
		pEsco->change_flag = 0;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	
	return ;
}

NTSTATUS HostAccept_eSCOLink(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8    transid, indicator, checkvalue, ChangeFlag = 0;
	LMP_PUD_PACKAGE_T pdu;
	
	transid   = (pConnectDevice->current_role == BT_ROLE_MASTER) ? SLAVE_TRANSACTION_ID : MASTER_TRANSACTION_ID;
	indicator = ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11;
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		if ((UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle) == 0 
			|| ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr == 0 || ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr > 7)
		{
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[indicator], sizeof(BT_ESCO_T));
			pdu.TransID     = SLAVE_TRANSACTION_ID;
			pdu.OpCode      = 127;
			pdu.contents[0] = 12;
			status = ResLmpNotAcceptedExt(devExt, pConnectDevice, &pdu, UNSPECIFIED_ERROR);
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			return status;
		}
		
		((PBT_LMP_T)devExt->pLmp)->esco[indicator].esco_LT_addr = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr;
		((PBT_LMP_T)devExt->pLmp)->esco[indicator].esco_handle  = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
	}
	else
	{
		((PBT_LMP_T)devExt->pLmp)->esco[indicator].hc_esco_handle = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr = ((PBT_LMP_T)devExt->pLmp)->esco[indicator].esco_LT_addr;
	}
	
	CalculateBandwidth(pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->esco[indicator]));
	((PBT_LMP_T)devExt->pLmp)->esco[indicator].change_flag = ESCO_ADD_LINK;
	
	checkvalue = CheckESCOPara(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[indicator], &ChangeFlag);
	if (checkvalue != BT_HCI_STATUS_SUCCESS)
	{
		RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[indicator], sizeof(BT_ESCO_T));
		pdu.TransID     = transid;
		pdu.OpCode      = 127;
		pdu.contents[0] = 12;
		status = ResLmpNotAcceptedExt(devExt, pConnectDevice, &pdu, checkvalue);
		
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		return status;
	}
	
	if (ChangeFlag == 1 || pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		status = Send_eSCOLinkReq_PDU(devExt, pConnectDevice, transid);
		return status;
	}
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 127;
	pdu.contents[0] = 1;
	pdu.contents[1] = 127;
	pdu.contents[2] = 12;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Ext Accepted eSCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	After_eSCO_Complete(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[indicator]);
	return status;
}

void DeleteSniffLink(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 i, j;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
			break;
	}
	if (i >= ((PBT_LMP_T)devExt->pLmp)->sniff_number)
		return;
	
	RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[i], sizeof(BT_SNIFF_T));
	for (j = i + 1; j < ((PBT_LMP_T)devExt->pLmp)->sniff_number; j++)
	{
		RtlCopyMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[j - 1], &((PBT_LMP_T)devExt->pLmp)->sniff[j], sizeof(BT_SNIFF_T));
		RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[j], sizeof(BT_SNIFF_T));
	}
	
	((PBT_LMP_T)devExt->pLmp)->sniff_number--;
	return;
}

void CheckSniffPara(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice->sniff_max_interval % 2 != 0)
		pConnectDevice->sniff_max_interval += 1;
	if (pConnectDevice->sniff_max_interval > pConnectDevice->link_supervision_timeout)
		pConnectDevice->sniff_max_interval = pConnectDevice->link_supervision_timeout;
	if (pConnectDevice->sniff_max_interval > SNIFF_MAX_INTERVAL)
		pConnectDevice->sniff_max_interval = SNIFF_MAX_INTERVAL;
	else if (pConnectDevice->sniff_max_interval < SNIFF_MIN_INTERVAL)
		pConnectDevice->sniff_max_interval = SNIFF_MIN_INTERVAL;
	
	if (pConnectDevice->sniff_min_interval % 2 != 0)
		pConnectDevice->sniff_min_interval += 1;
	if (pConnectDevice->sniff_min_interval > pConnectDevice->sniff_max_interval)
		pConnectDevice->sniff_min_interval = pConnectDevice->sniff_max_interval;
	else if (pConnectDevice->sniff_min_interval < SNIFF_MIN_INTERVAL)
		pConnectDevice->sniff_min_interval = SNIFF_MIN_INTERVAL;
	
	pConnectDevice->sniff_mode_interval = (UINT16)((pConnectDevice->sniff_min_interval + pConnectDevice->sniff_max_interval) >> 1);
	if (pConnectDevice->sniff_mode_interval % 2 != 0)
		pConnectDevice->sniff_mode_interval += 1;
	
	if (pConnectDevice->sniff_attempt == 0)
		pConnectDevice->sniff_attempt = 1;
	if (pConnectDevice->sniff_attempt > (UINT16)(pConnectDevice->sniff_mode_interval >> 1))
		pConnectDevice->sniff_attempt = (UINT16)(pConnectDevice->sniff_mode_interval >> 1);
	
	if (pConnectDevice->sniff_timeout > SNIFF_MAX_TIMEOUT)
		pConnectDevice->sniff_timeout = SNIFF_MAX_TIMEOUT;
	
	return;
}

NTSTATUS Pairing_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	ChangeLmpState(devExt, pConnectDevice, LMP_PIN_REQ);
	Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PIN_CODE_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	return STATUS_SUCCESS;
}

NTSTATUS Authentication_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 i;

#ifdef BT_SET_LINK_KEY_USING_APP
	RtlCopyMemory(pConnectDevice->link_key, ((PBT_HCI_T)devExt->pHci)->tmp_link_key, BT_LINK_KEY_LENGTH);
#endif

	for (i = 0; i < 16; i++)
	{
		if (pConnectDevice->link_key[i] != 0)
			break;
	}	
	if (i == 16)
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_NO_LINKKEY);
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LINK_KEY_REQ, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	else
	{
		i = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
		status = Send_AuRand_PDU(devExt, pConnectDevice, i);
	}
	
	return status;
}

NTSTATUS PauseResumeEncryption_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 transid;

	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.pause_encryption == 1 && pConnectDevice->lmp_features.byte5.pause_encryption == 1 
		&& pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
	{
		pConnectDevice->pause_encryption_status = 2;
		transid = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
		status  = Send_PauseEncryptionReq_PDU(devExt, pConnectDevice, transid);
	}

	return status;
}

NTSTATUS After_Conn_Authentication(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (((PBT_HCI_T)devExt->pHci)->encryption_mode != BT_ENCRYPTION_DIABLE && pConnectDevice->encryption_mode == BT_ENCRYPTION_DIABLE && 
		((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.encry == 1 && pConnectDevice->lmp_features.byte0.encry == 1)
	{
		pConnectDevice->encryption_mode = ((PBT_HCI_T)devExt->pHci)->encryption_mode;
		status = Send_EnModReq_PDU(devExt, pConnectDevice);
	}
	else
	{
		if ((pConnectDevice->lmp_states & LMP_SEND_SETUP_COMPLETE) == 0)
			status = Send_SetupComplete_PDU(devExt, pConnectDevice);
	}
	
	return status;
}

NTSTATUS After_Conn_SetupComplete(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.channel_qua == 1 && pConnectDevice->lmp_features.byte1.channel_qua == 1)
		Send_AutoRate_PDU(devExt, pConnectDevice);
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.timing_accur == 1 && pConnectDevice->lmp_features.byte0.timing_accur == 1)
		Send_TimingAccuracyReq_PDU(devExt, pConnectDevice);
	
	Send_MaxSlot_PDU(devExt, pConnectDevice);
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && (*((PUINT8)(&pConnectDevice->lmp_features.byte0)) == 0))
	{
#ifdef BT_TESTMODE_SUPPORT
		RtlCopyMemory(&(pConnectDevice->lmp_features), Remote_Feature_Default, sizeof(LMP_FEATURES_T));
#endif
		status = Send_FeatureReq_PDU(devExt, pConnectDevice);
		
		if (pConnectDevice->lmp_timer_valid == 1 && pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_FEATURES_REQ)
			LMP_StopPDUTimer(devExt, pConnectDevice);
	}
	
	/*
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && ((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_2 == 1 && pConnectDevice->lmp_features.byte3.enh_rate_acl_2 == 1 
		&& pConnectDevice->edr_mode == 0 && pConnectDevice->edr_change_flag == 0 && (pConnectDevice->packet_type & BT_PACKET_TYPE_2DH1) == 0)
	{
		pConnectDevice->edr_change_flag = 1;
		Send_PacketTypeTableReq_PDU(devExt, pConnectDevice, 1);
	}
	*/


	Hci_SlaveConnected(devExt, pConnectDevice);
	
	return status;
}

NTSTATUS Send_FeatureReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 39;
#ifdef BT_COMBO_SNIFF_SUPPORT
	if(devExt->ComboState == FW_WORK_MODE_COMBO)
	{
		RtlCopyMemory(&tmpLocalFeature, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
		tmpLocalFeature.byte0.sniff_mode = 0;
		RtlCopyMemory(pdu.contents, &tmpLocalFeature, sizeof(LMP_FEATURES_T));	
	}
	else
	{
		RtlCopyMemory(pdu.contents, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));	
	}
#else
	RtlCopyMemory(pdu.contents, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
#endif
	
#ifdef BT_DISABLE_SLAVE_SNIFF_WHEN_MS
	if (pConnectDevice->current_role == BT_ROLE_SLAVE && ((PBT_HCI_T)devExt->pHci)->num_device_am > 0)
		pdu.contents[0] = pdu.contents[0] & 0x7F;
#endif
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 9);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Features Req, local feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			pdu.contents[0], pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7]);
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_FEATURES_REQ, 10);
	}
	
	return status;
}

NTSTATUS Send_CombKey_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT8    i;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 9;
	for (i = 0; i < 16; i++)
	{
		pConnectDevice->random[i] = Rand_UCHAR_Gen();
		pdu.contents[i] = (UINT8)(pConnectDevice->random[i] ^ pConnectDevice->init_key[i]);
	}

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_COM_KEY);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Combination Key\n", pdu.TransID, pConnectDevice->bd_addr[0]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_COMB_KEY, LMP_MAX_RESPONSE_TIME);
	}

	return status;
}

NTSTATUS Send_InRand_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT8    i;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 8;
	for (i = 0; i < 16; i++)
	{
		pConnectDevice->random[i] = Rand_UCHAR_Gen();
		pdu.contents[i] = pConnectDevice->random[i];
	}

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_INRAND);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Initialization Rand\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_INRAND, LMP_MAX_RESPONSE_TIME);		
	}

	return status;
}

NTSTATUS Send_AuRand_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT8    i;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 11;
	for (i = 0; i < 16; i++)
	{
		pConnectDevice->random[i] = Rand_UCHAR_Gen();
		pdu.contents[i] = pConnectDevice->random[i];
	}

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_AURAND);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Authentication Rand\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_AURAND, LMP_MAX_RESPONSE_TIME);		
	}

	return status;
}

NTSTATUS Send_EnModReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode      = 15;
	pdu.contents[0] = pConnectDevice->encryption_mode;

	pConnectDevice->is_in_encryption_process = 1;

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_ENMODE_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Encryption Mode Req, mode= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_ENMODE_REQ, LMP_MAX_RESPONSE_TIME);
	}

	return status;
}

NTSTATUS Send_EnKeySizeReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid, UINT8 keysize)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 16;
	pdu.contents[0] = keysize;

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_ENKEYSIZE_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Encryption Key Size Req, size= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_ENKEYSIZE_REQ, LMP_MAX_RESPONSE_TIME);

		/* Write EncryptionKeySize register to hardware 
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			BT_HAL_WRITE_BYTE((PUINT8)devExt->BaseRegisterAddress + BT_REG_ENCRYPTION_KEY_LEN + pConnectDevice->am_addr, keysize);
		else
			BT_HAL_WRITE_BYTE((PUINT8)devExt->BaseRegisterAddress + BT_REG_ENCRYPTION_KEY_LEN + SLAVE_REGISTER_OFFSET, keysize);
		*/
		pConnectDevice->encryption_key_size = keysize;
	}

	return status;
}

NTSTATUS Send_StartEnReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT8 i, COF[BT_ACO_LENGTH], Kc[BT_LINK_KEY_LENGTH], ApostropheKc[BT_LINK_KEY_LENGTH];
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 17;	
	for (i = 0; i < 16; i++)
		pdu.contents[i] = Rand_UCHAR_Gen();

	/* Calculate EncryptionKey and write EncryptionKey register to hardware */
	RtlCopyMemory(COF, pConnectDevice->aco, BT_ACO_LENGTH);
	E3_Encryption_KeyGen(pConnectDevice->link_key, pdu.contents, COF, Kc);
	Encryption_ApostropheKeyGen(Kc, pConnectDevice->encryption_key_size, ApostropheKc);
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_KEY + pConnectDevice->am_addr * BT_LINK_KEY_LENGTH, pBuf, BT_LINK_KEY_LENGTH, ApostropheKc);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr, pBuf, 1, 1);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 17);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_STARTEN_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Start Encryption Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_STARTEN_REQ, LMP_MAX_RESPONSE_TIME);
	}

	return status;
}

NTSTATUS Send_StopEnReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 18;

	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr, pBuf, 1, 2);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_STOPTEN_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Stop Encryption Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_STOPTEN_REQ, LMP_MAX_RESPONSE_TIME);
	}

	return status;
}

NTSTATUS Send_SetupComplete_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 49;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_SEND_SETUP_COMPLETE);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Setup Complete\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if ((pConnectDevice->lmp_states & LMP_RECV_SETUP_COMPLETE) == LMP_RECV_SETUP_COMPLETE)
		{
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			After_Conn_SetupComplete(devExt, pConnectDevice);
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SETUP_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
		else
		{
			if (pConnectDevice->lmp_timer_valid == 0)
				LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_SETUP_COMPLETE, LMP_MAX_RESPONSE_TIME);
		}
	}
	
	return status;
}

NTSTATUS Send_SCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT32    instant = 0;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 43;
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
		if ((devExt->ComboState == FW_WORK_MODE_COMBO) && (devExt->chipID != 0)) /* V4 Combo mode */
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco = (UINT8)(((instant >> 1) & 0x03FFFFFE) % 6);
		
		instant = ((instant >> 27) & 0x00000001) << 1;
	}
	((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].timing_control_flags = (UINT8)instant;
	
	pdu.contents[0] = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_handle;
	pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].timing_control_flags;
	pdu.contents[2] = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco;
	pdu.contents[4] = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_packet;
	pdu.contents[3] = (UINT8)((pdu.contents[4] + 1) << 1);
	pdu.contents[5] = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].air_mode;

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 7);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SCO_LINK_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): SCO Link Req, contents= %d %d %d %d %d %d\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			       pdu.contents[0], pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_SCOLINK_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Send_eSCOLinkReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT32    instant = 0;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 127;
	pdu.contents[0] = 12;
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
			instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
		else
			instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
	}
	else
	{
		instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
	}

	instant = ((instant >> 27) & 0x00000001) << 1;
	((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].timing_control_flags = (UINT8)instant;
	
	RtlCopyMemory(&pdu.contents[1], &(((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].esco_handle), 14);
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 16);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_ESCO_LINK_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): eSCO Link Req, contents(byte-orders)= %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7], 
			pdu.contents[8], pdu.contents[9], pdu.contents[10], pdu.contents[11], pdu.contents[12], pdu.contents[13], pdu.contents[14]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_ESCOLINK_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

/* flag: 0(normal AFH_map); 1(AHS(79)) */
NTSTATUS Send_SetAFH_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_mode, UINT8 flag)
{
	NTSTATUS status;
	KIRQL  oldIrql;
	UINT32  AFHInstant;
	UINT8  i = 0, j = 0, channel_num = 0;
	UINT16 interval, slot_offset = 0;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice->current_role == BT_ROLE_MASTER)
		{
			interval = ((PBT_LMP_T)devExt->pLmp)->sniff[i].interval * 3;
			if (slot_offset < interval)
				slot_offset = interval;
		}
	}

	if (slot_offset < 12 * MAX_T_POLL_SLOTS)
		slot_offset = 12 * MAX_T_POLL_SLOTS;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	if (AFH_mode == AFH_ENABLED)
	{
		if (flag == 0)
		{
			RtlCopyMemory(&pdu.contents[5], ((PBT_HCI_T)devExt->pHci)->afh_channel_map, BT_MAX_CHANNEL_MAP_NUM);
			pdu.contents[14] &= 0x7f;
			for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
				for (j = 0; j < 8; j++)
					if (((pdu.contents[i] >> j) & 0x1) == 1)
						channel_num++;
		}
		else
		{
			RtlFillMemory(&pdu.contents[5], BT_MAX_CHANNEL_MAP_NUM, 0xff);
			pdu.contents[14] &= 0x7f;
			channel_num = 79;
		}
	}
	else
	{
		pConnectDevice->afh_mode = AFH_DISABLED;
		pConnectDevice->is_afh_sent_flag = 0; /* for sending channel_classification_req */
	}
	
	AFHInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
	AFHInstant = ((AFHInstant >> 2) << 1) + slot_offset;
	RtlCopyMemory(pdu.contents, &AFHInstant, 4);

	pdu.TransID = MASTER_TRANSACTION_ID;
	pdu.OpCode  = 60;
	pdu.contents[4] = AFH_mode;

	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (AFH_mode == AFH_ENABLED)
	{
		for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "0x%x \n", pdu.contents[i]);
		
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_NUM, pBuf, 1, &channel_num);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_MAP, pBuf, BT_MAX_CHANNEL_MAP_NUM, &(pdu.contents[5]));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_AFH_MODE, pBuf, 1, (0x1 << pConnectDevice->am_addr));
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_AFH_MODE, pBuf, 1, (~(0x1 << pConnectDevice->am_addr)));
	}
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_INSTANT, pBuf, 4, AFHInstant << 1);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));


	
	status = LMP_SendPDUToLCHead(devExt, pConnectDevice, &pdu, 16);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Set AFH, instants= 0x%x, mode= %d, channel_map=\n", pdu.TransID, pConnectDevice->bd_addr[0], AFHInstant, pdu.contents[4]);

		
		((PBT_HCI_T)devExt->pHci)->need_write_afh_command++;
	}
	
	return status;
}


NTSTATUS LMP_WriteReg_AfterSendingLMP(PBT_DEVICE_EXT devExt, PUINT8 buf, UINT32 len)
{
	NTSTATUS status;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP--WriteReg After Sending AFH LMP---\n");
	status = RegAPI_SendTo_MailBox(devExt, buf, len);	
	return status;
}

/* flag: 0(normal AFH_map); 1(AHS(79)) */
NTSTATUS Send_SetAFH_PDU_WithoutWritingReg(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, 
						UINT8 AFH_mode, UINT8 flag, UINT32 AfhInstant)
{
	NTSTATUS status;
	KIRQL  oldIrql;
	UINT8  i = 0, j = 0, channel_num = 0;
	UINT16 interval, slot_offset = 0;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
	{
		if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice->current_role == BT_ROLE_MASTER)
		{
			interval = ((PBT_LMP_T)devExt->pLmp)->sniff[i].interval * 3;
			if (slot_offset < interval)
				slot_offset = interval;
		}
	}
	if (slot_offset < 6 * MAX_T_POLL_SLOTS)
		slot_offset = 6 * MAX_T_POLL_SLOTS;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	if (AFH_mode == AFH_ENABLED)
	{
		if (flag == 0)
		{
			RtlCopyMemory(&pdu.contents[5], ((PBT_HCI_T)devExt->pHci)->afh_channel_map, BT_MAX_CHANNEL_MAP_NUM);
			pdu.contents[14] &= 0x7f;
			for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
				for (j = 0; j < 8; j++)
					if (((pdu.contents[i] >> j) & 0x1) == 1)
						channel_num++;
		}
		else
		{
			RtlFillMemory(&pdu.contents[5], BT_MAX_CHANNEL_MAP_NUM, 0xff);
			pdu.contents[14] &= 0x7f;
			channel_num = 79;
		}
	}
	else
	{
		pConnectDevice->afh_mode = AFH_DISABLED;
		pConnectDevice->is_afh_sent_flag = 0; /* for sending channel_classification_req */
	}
	
	AfhInstant = ((AfhInstant >> 2) << 1) + slot_offset;
	RtlCopyMemory(pdu.contents, &AfhInstant, 4);

	pdu.TransID = MASTER_TRANSACTION_ID;
	pdu.OpCode  = 60;
	pdu.contents[4] = AFH_mode;

	
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (AFH_mode == AFH_ENABLED)
	{
		for (i = 5; i < 5 + BT_MAX_CHANNEL_MAP_NUM; i++)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "0x%x \n", pdu.contents[i]);
		
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_NUM, pBuf, 1, &channel_num);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_CHANNEL_MAP, pBuf, BT_MAX_CHANNEL_MAP_NUM, &(pdu.contents[5]));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_AFH_MODE, pBuf, 1, (0x1 << pConnectDevice->am_addr));
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_AFH_MODE, pBuf, 1, (~(0x1 << pConnectDevice->am_addr)));
	}
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AFH_INSTANT, pBuf, 4, AfhInstant << 1);
	Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_HCI2LMP_EVENT(BT_TASK_EVENT_HCI2LMP_WRITE_AFH_REGISTERS), BT_TASK_PRI_NORMAL, buf, (UINT32)(pBuf - buf));
	
	
	status = LMP_SendPDUToLCHead(devExt, pConnectDevice, &pdu, 16);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Set AFH, instants= 0x%x, mode= %d, channel_map=\n", pdu.TransID, pConnectDevice->bd_addr[0], AfhInstant, pdu.contents[4]);

		/* jakio20070228: record afh pdu count
*/
		devExt->AfhPduCount++;
		
		((PBT_HCI_T)devExt->pHci)->need_write_afh_command++;
	}
	
	return status;
}


NTSTATUS Send_ChannelClassificationReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 AFH_reporting_mode)
{
	NTSTATUS status;
	UINT16 MinInterval, MaxInterval;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = MASTER_TRANSACTION_ID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 16;
	pdu.contents[1] = AFH_reporting_mode;

	MinInterval = AFH_MIN_INTERVAL;
	MaxInterval = AFH_MAX_INTERVAL;
	RtlCopyMemory(pdu.contents + 2, &MinInterval, sizeof(UINT16));
	RtlCopyMemory(pdu.contents + 4, &MaxInterval, sizeof(UINT16));

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 7);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Channel Classification Req, ReportingMode= %d, MinInterval= %d, MaxInterval= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[1], MinInterval, MaxInterval);
	
	return status;
}

NTSTATUS Send_ChannelClassification_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 ChannelAssessmentMode)
{
	NTSTATUS status;
	UINT8    i = 0;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = SLAVE_TRANSACTION_ID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 17;
	
	RtlCopyMemory(&pdu.contents[1], ((PBT_HCI_T)devExt->pHci)->classification_channel_map, BT_MAX_CHANNEL_MAP_NUM);
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 12);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Channel Classification,  ChannelClassificationMap= \n", pdu.TransID, pConnectDevice->bd_addr[0]);
		for (i = 1; i < 1 + BT_MAX_CHANNEL_MAP_NUM; i++)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "0x%x \n", pdu.contents[i]);
	}
	
	return status;
}

NTSTATUS Send_TimingAccuracyReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 47;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Timing Accuracy Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	return status;
}

NTSTATUS Send_AutoRate_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 35;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Auto Rate\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	
	return status;
}

NTSTATUS Send_PreferredRate_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 flag = 0, slotflag = 0;
	
	if (pConnectDevice->edr_mode == 0 || pConnectDevice->rev_auto_rate == 0)
		return STATUS_SUCCESS;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 36;
	
#ifdef BT_USE_ONE_SLOT_FOR_ACL
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_3 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_3 == 1))
		pdu.contents[0] = 0x31;
	else
		pdu.contents[0] = 0x29;
#else
	if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.enh_rate_acl_3 == 1) && (pConnectDevice->lmp_features.byte3.enh_rate_acl_3 == 1))
		flag = 0x11;
	else
		flag = 0x09;
	
	if (pConnectDevice->max_slot == BT_MAX_SLOT_BY_FEATURE)
	{
		if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot5 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot5_enh_acl == 1) && (pConnectDevice->lmp_features.byte5.slot5_enh_acl == 1))
			slotflag = 0x3;
		else if ((((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot3 == 1) && (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.slot3_enh_acl == 1) && (pConnectDevice->lmp_features.byte4.slot3_enh_acl == 1))
			slotflag = 0x2;
		else
			slotflag = 0x1;
	}
	else
	{
		if (pConnectDevice->max_slot == BT_MAX_SLOT_5_SLOT && ((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot5_enh_acl == 1 && pConnectDevice->lmp_features.byte5.slot5_enh_acl == 1)
			slotflag = 0x3;
		else if (pConnectDevice->max_slot == BT_MAX_SLOT_3_SLOT && ((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.slot3_enh_acl == 1 && pConnectDevice->lmp_features.byte4.slot3_enh_acl == 1)
			slotflag = 0x2;
		else
			slotflag = 0x1;
	}
	
	pdu.contents[0] = (slotflag << 5 ) | flag;
#endif
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Preferred Rate, DataRate= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);
	
	return status;
}

NTSTATUS Send_MaxSlot_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 45;
	
#ifdef BT_USE_ONE_SLOT_FOR_ACL
	pdu.contents[0] = BT_MAX_SLOT_1_SLOT;
#else
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot5 == 1)
		pdu.contents[0] = BT_MAX_SLOT_5_SLOT;
	else if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot3 == 1)
		pdu.contents[0] = BT_MAX_SLOT_3_SLOT;
	else
		pdu.contents[0] = BT_MAX_SLOT_1_SLOT;
	if (pConnectDevice->max_slot != BT_MAX_SLOT_BY_FEATURE)
		pdu.contents[0] = (pConnectDevice->max_slot > pdu.contents[0]) ? pdu.contents[0] : pConnectDevice->max_slot;
#endif
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Max Slot, MaxSlots= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);
	
	return status;
}

NTSTATUS Send_MaxSlotReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 maxslot)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 46;
	pdu.contents[0] = maxslot;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_MAX_SLOT_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Max Slot Req, MaxSlots= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_MAX_SLOT_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

/* flag: 0 (Quality of Service PDU); 1 (Quality of Service Req PDU) */
NTSTATUS Send_QualityService_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 pollinterval, UINT8 Nbc, UINT8 flag)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = (UINT8)(41 + flag);
	
	RtlCopyMemory(pdu.contents, &pollinterval, sizeof(UINT16));
	pdu.contents[2] = Nbc;
	status = STATUS_SUCCESS;
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
	if (NT_SUCCESS(status))
	{
		if (flag == 0)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Quality of Service, PollInterval= %d, Nbc= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pollinterval, pdu.contents[2]);
		else
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Quality of Service Req, PollInterval= %d, Nbc= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pollinterval, pdu.contents[2]);
	}
	
	return status;
}

NTSTATUS Send_SniffReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	UINT32    instant = 0;
	BT_SNIFF_T  temp_sniff;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 23;
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
			instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
		else
			instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
	}
	else
	{
		instant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
	}
	
	instant = ((instant >> 27) & 0x00000001) << 1;
	((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timing_control_flags = (UINT8)instant;
	
	RtlCopyMemory(pdu.contents, &(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timing_control_flags), 9);
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 10);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SNIFF_REQ);
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_SNIFF_REQ, LMP_MAX_RESPONSE_TIME);
		
		RtlCopyMemory(&temp_sniff, &(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index]), sizeof(BT_SNIFF_T));
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Sniff Req, TimingFlag= %d, D= 0x%x, T= 0x%x, Attempt= 0x%x, Timeout= 0x%x\n", 
			pdu.TransID, pConnectDevice->bd_addr[0], temp_sniff.timing_control_flags, temp_sniff.offset, temp_sniff.interval, temp_sniff.attempt, temp_sniff.timeout);
	}
	
	return status;
}

NTSTATUS Send_UnsniffReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	Frag_ListStopSwitch(devExt, pConnectDevice, MAX_LIST_STOP_TIME);
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 24;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Unsniff Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		if (pConnectDevice->lmp_timer_valid == 0)
		{
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_UNSNIFF_DETACH) == LMP_SEND_UNSNIFF_DETACH)
				LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_UNSNIFF_REQ, 5);
			else
				LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_UNSNIFF_REQ, LMP_MAX_RESPONSE_TIME);
		}
	}
	
	return status;
}

NTSTATUS Send_HoldReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT16 HoldTime, UINT8 transid)
{
	NTSTATUS status;
	UINT8 RegisterIndex = SLAVE_REGISTER_OFFSET;
	UINT32 HoldInstant;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
			HoldInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
		else
			HoldInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
	}
	else
	{
		HoldInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
	}
	HoldInstant = ((HoldInstant >> 2) << 1) + 9 * pConnectDevice->per_poll_interval;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 21;
	RtlCopyMemory(pdu.contents, &HoldTime, sizeof(UINT16));
	RtlCopyMemory(pdu.contents + 2, &HoldInstant, 4);
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 7);
	if (NT_SUCCESS(status))
	{
		pConnectDevice->mode_current_mode  = BT_MODE_CURRENT_MODE_HOLD;
		pConnectDevice->hold_mode_interval = HoldTime;
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			RegisterIndex = pConnectDevice->am_addr;
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HOLD_MODE_INTERVAL + 2 * RegisterIndex, pBuf, 2, (PUINT8)&HoldTime);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_HODE_INSTANT, pBuf, 4, (PUINT8)&HoldInstant);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_HOLD_MODE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_HOLD_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Hold Req, HoldTime= 0x%x, HoldInstant= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], HoldTime, HoldInstant);
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_HOLD_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Send_SlotOffset_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	UINT16 slot_offset = 1250;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
#ifdef BT_TESTMODE_SUPPORT
	slot_offset = pConnectDevice->local_slot_offset;
#else
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
			slot_offset = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_SLOT_OFFSET_SLAVE0);
		else
			slot_offset = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_SLOT_OFFSET_SLAVE1);
	}
	
	if (slot_offset >= 1250)
		slot_offset = 1249; /* us */
#endif
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = 52;
	RtlCopyMemory(pdu.contents, &slot_offset, sizeof(UINT16));
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
		RtlCopyMemory(pdu.contents + 2, pConnectDevice->bd_addr, BT_BD_ADDR_LENGTH);
	else
		RtlCopyMemory(pdu.contents + 2, ((PBT_HCI_T)devExt->pHci)->local_bd_addr, BT_BD_ADDR_LENGTH);
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 9);
	if (NT_SUCCESS(status))
	{
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SWITCH_SLOT_OFFSET, pBuf, 2, (PUINT8)&slot_offset);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Slot Offset, SlotOffset= %d, BD_ADDR= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
			pdu.TransID, pConnectDevice->bd_addr[0], slot_offset, pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7]);
	}
	
	return status;
}



NTSTATUS Send_SwitchReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	UINT32 SwitchInstant, SwitchInstant2;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	UINT8		ListIndex;
	PBT_HCI_T	pHci = NULL;


	if(pConnectDevice == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci != NULL)
	{
		pHci->role_switching_flag = 1;
	}
	
	Frag_GetQueueHead(devExt, pConnectDevice, &ListIndex);
	Frag_StopQueue(devExt, ListIndex);
	if(1)
	{

		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			if (pConnectDevice->slave_index == 0)
				SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
			else
				SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
		}
		else
		{
			SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
		}
		if ((devExt->ComboState == FW_WORK_MODE_COMBO) && (devExt->chipID != 0)) /* Combo mode */
			SwitchInstant = ((SwitchInstant >> 2) << 1) + 100 * MAX_SWITCH_INSTANT_OFFSET;
		else
			SwitchInstant = ((SwitchInstant >> 2) << 1) + MAX_SWITCH_INSTANT_OFFSET;
		SwitchInstant2 = SwitchInstant & 0x07FFFFFF;
		
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
		pdu.OpCode  = 19;
		RtlCopyMemory(pdu.contents, &SwitchInstant2, 4);
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			BtWriteFHSPacket(devExt, pConnectDevice);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (pConnectDevice->slave_index << 31));
		}
		else
		{
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SWITCH_SLOT_OFFSET, pBuf, 2, 0);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (1 << 30));
		}
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 5);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SWITCH_REQ);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Switch Req, SwitchInstant= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], SwitchInstant2);
		}
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "Send_SwitchReq_PDU----BD not empty, try it later\n");
		UsbQueryDMASpace(devExt);
		status = Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_SEND), 
					BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}

	return status;
}

#if 0
NTSTATUS Send_SwitchReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status;
	UINT32 SwitchInstant, SwitchInstant2;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		if (pConnectDevice->slave_index == 0)
			SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
		else
			SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
	}
	else
	{
		SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
	}
	if ((devExt->ComboState == FW_WORK_MODE_COMBO) && (devExt->chipID != 0)) /* Combo mode */
		SwitchInstant = ((SwitchInstant >> 2) << 1) + 100 * MAX_SWITCH_INSTANT_OFFSET;
	else
		SwitchInstant = ((SwitchInstant >> 2) << 1) + MAX_SWITCH_INSTANT_OFFSET;
	SwitchInstant2 = SwitchInstant & 0x07FFFFFF;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode  = 19;
	RtlCopyMemory(pdu.contents, &SwitchInstant2, 4);
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	if (pConnectDevice->current_role == BT_ROLE_SLAVE)
	{
		BtWriteFHSPacket(devExt, pConnectDevice);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (pConnectDevice->slave_index << 31));
	}
	else
	{
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SWITCH_SLOT_OFFSET, pBuf, 2, 0);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (1 << 30));
	}
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
	pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 5);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SWITCH_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Switch Req, SwitchInstant= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], SwitchInstant2);
	}
	
	return status;
}
#endif

/* direction: 1(increase); 2(decrease); 3(max power); 4(min power) */
NTSTATUS Send_PowerControl_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 direction, UINT8 transid)
{
	NTSTATUS status;
	UINT8    PduLength = 2;
	LMP_PUD_PACKAGE_T pdu;
	
	if (direction > 2)
		PduLength = 1;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = transid;
	pdu.OpCode  = (UINT8)(30 + direction);

	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, PduLength);
	if (NT_SUCCESS(status))
	{
		if (direction == 1)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Increase Power Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		else if (direction == 2)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Decrease Power Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		else if (direction == 3)
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Max Power\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		else
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Min Power\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}
	
	return status;
}

NTSTATUS Send_PacketTypeTableReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 EdrMode)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 11;
	pdu.contents[1] = EdrMode;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
	if (NT_SUCCESS(status))
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_PACKET_TYPE_TABLE_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Packet Type Table Req, ppt= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[1]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Send_FeatureReqExt_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 page_number)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	pdu.OpCode      = 127;
	pdu.contents[0] = 3;
	pdu.contents[1] = page_number;
	pdu.contents[2] = BT_MAX_FEATURE_PAGE_NUMBER;
	if (pdu.contents[1] == 0)
	{
	#ifdef BT_COMBO_SNIFF_SUPPORT
		if(devExt->ComboState == FW_WORK_MODE_COMBO)
		{
			RtlCopyMemory(&tmpLocalFeature, &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
			tmpLocalFeature.byte0.sniff_mode = 0;
			RtlCopyMemory(&(pdu.contents[3]), &tmpLocalFeature, sizeof(LMP_FEATURES_T));	
		}
		else
		{
			RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
		}
	#else
		RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->lmp_features), sizeof(LMP_FEATURES_T));
	#endif
	}
	else if (pdu.contents[1] == 1)
		RtlCopyMemory(&(pdu.contents[3]), &(((PBT_HCI_T)devExt->pHci)->extended_lmp_features), sizeof(LMP_EXTEND_FEATURES_T));
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 12);
	if (NT_SUCCESS(status))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Features Req Ext, feature_page= %d, max_page= %d, local feature= 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], 
			pdu.contents[1], pdu.contents[2], pdu.contents[3], pdu.contents[4], pdu.contents[5], pdu.contents[6], pdu.contents[7], pdu.contents[8], pdu.contents[9], pdu.contents[10]);

		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_FEATURES_REQ_EXT, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Send_PauseEncryptionReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 127;
	pdu.contents[0] = 23;
	
	pConnectDevice->is_in_encryption_process = 1;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			ChangeLmpState(devExt, pConnectDevice, LMP_SEND_PAUSEENCRYPTION_REQ);
		else
			ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STOPEN_REQ);

		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Pause Encryption Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_PAUSEENCRYPTION_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Send_ResumeEncryptionReq_PDU(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 transid)
{
	NTSTATUS status;
	LMP_PUD_PACKAGE_T pdu;
	
	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID     = transid;
	pdu.OpCode      = 127;
	pdu.contents[0] = 24;
	
	status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
	if (NT_SUCCESS(status))
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_WAIT_STARTEN_REQ);
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Resume Encryption Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (pConnectDevice->lmp_timer_valid == 0)
			LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_RESUMEENCRYPTION_REQ, LMP_MAX_RESPONSE_TIME);
	}
	
	return status;
}

NTSTATUS Command_AcceptConn(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LMP_PUD_PACKAGE_T pdu;
	UINT8 tempvalue;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	if ((pConnectDevice->lmp_states & LMP_RECV_CONN_REQ) == LMP_RECV_CONN_REQ)
	{
		ChangeLmpState(devExt, pConnectDevice, LMP_STATE_ANDVALUE);
		
		pdu.TransID     = MASTER_TRANSACTION_ID;
		pdu.OpCode      = 3;
		pdu.contents[0] = 51;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Connection\n", pdu.TransID, pConnectDevice->bd_addr[0]);
		
		if (((PBT_HCI_T)devExt->pHci)->authentication_enable == BT_AUTHENTICATION_ENABLE)
		{
			status = Authentication_Process(devExt, pConnectDevice);
			return status;
		}
		
		status = Send_SetupComplete_PDU(devExt, pConnectDevice);
	}
	else if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_RECV_SCO_LINK_REQ && ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice == pConnectDevice)
	{
		ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			CheckSCOPara(devExt, pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11]), 0); /* check parameters for adding a new SCO */
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_handle  = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco       = SCO_DEFAULT_D;
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = SCO_ADD_LINK;
			((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
			
			status = Send_SCOLinkReq_PDU(devExt, pConnectDevice, SLAVE_TRANSACTION_ID);
		}
		else
		{
			tempvalue = CheckSCOPara(devExt, pConnectDevice, &(((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11]), 2); /* check parameters for adding a new SCO */
			if (tempvalue != 0)
			{
				pdu.TransID = MASTER_TRANSACTION_ID;
				pdu.OpCode  = 43;
				status = ResLmpNotAccepted(devExt, pConnectDevice, &pdu, tempvalue);

				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_SCO_T));
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				return status;
			}

			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco               = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco;
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode            = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].air_mode;
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type = SCO_PacketType_Transform[((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_packet];
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].hc_sco_handle = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
			
			tempvalue = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].timing_control_flags;
			tempvalue = ((((tempvalue >> 1) & 0x01) + 1) << 6) + ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].D_sco;
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_D_SCO, pBuf, 1, &tempvalue);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_T_SCO, pBuf, 1, (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].sco_packet + 1) << 1);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_AM_SCO, pBuf, 1, pConnectDevice->am_addr + (pConnectDevice->slave_index << 5));
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_ADD_SCO_CONN_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
			
			pdu.TransID     = MASTER_TRANSACTION_ID;
			pdu.OpCode      = 3;
			pdu.contents[0] = 43;
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
			if (NT_SUCCESS(status))
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted SCO Link Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
			
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) + 1) << 6);
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_CONNECTED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
	}

	return status;
}

NTSTATUS Command_ChangeSCOPacket(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UINT8 i, handle1, handle2, transid, feature, feature2, packet_type;
	
	if (pConnectDevice->pScoConnectDevice == NULL || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 0 || pConnectDevice->lmp_features.byte1.sco_link == 0)
	{
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = UNSUPPORTED_FEATURE_OR_PARAMETER;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		return STATUS_UNSUCCESSFUL;
	}
	
	RtlCopyMemory(&feature, &(((PBT_HCI_T)devExt->pHci)->lmp_features.byte1), 1);
	RtlCopyMemory(&feature2, &(pConnectDevice->lmp_features.byte1), 1);
	feature = feature & feature2;
	feature = (UINT8)((feature >> 3) & (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->packet_type >> 5));
	if ((feature & 0x04) != 0)
		packet_type = 2; /* 0(HV1); 1(HV2); 2(HV3) */
	else if ((feature & 0x02) != 0)
		packet_type = 1;
	else if ((feature & 0x01) != 0)
		packet_type = 0;
	else
		return STATUS_UNSUCCESSFUL;
	
	if (((PBT_HCI_T)devExt->pHci)->only_use_hv3_flag == 1 && packet_type != 2)
		return STATUS_UNSUCCESSFUL;
	
	handle1 = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			handle2 = ((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle;
		else
			handle2 = ((PBT_LMP_T)devExt->pLmp)->sco[i].hc_sco_handle;
		if (handle2 == handle1)
		{
			if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice != pConnectDevice)
				i = ((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6;
			
			break;
		}
	}
	if (i >= (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
	{
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
		((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = PARAMETER_OUT_RANGE;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		return STATUS_UNSUCCESSFUL;
	}
	
	LMP_SCONeedUnsniff(devExt, pConnectDevice);
	Frag_InitScoRxBuffer(devExt, pConnectDevice);

	((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->changed_packet_type = SCO_PacketType_Transform[packet_type];;
	((PBT_LMP_T)devExt->pLmp)->sco[i].sco_packet  = packet_type;
	((PBT_LMP_T)devExt->pLmp)->sco[i].D_sco       = SCO_DEFAULT_D;
	((PBT_LMP_T)devExt->pLmp)->sco[i].air_mode    = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->air_mode;
	((PBT_LMP_T)devExt->pLmp)->sco[i].change_flag = SCO_CHANGE_LINK;
	((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));
	
	transid = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
	status  = Send_SCOLinkReq_PDU(devExt, pConnectDevice, transid);
	return status;
}

/*************************************************************
 *   LMP_SendPDUToLC
 *
 *   Descriptions:
 *      Inserts one PDU to the linked list for Tx.
 *
 *   Arguments:
 *      devExt:         IN, pointer to the device.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      PduLength:      IN, the length of PDU.
 *      pLmpPdu:        IN, pointer to the PDU received.
 *
 *   Return Value:
 *      STATUS_SUCCESS:      if inserted into the linked list successfully.
 *      STATUS_UNSUCCESSFUL: if inserted into the linked list failed.
 *************************************************************/
NTSTATUS LMP_SendPDUToLC(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)
{
	PBT_LMP_T  pLmp;
	PBT_FRAG_T pFrag;
	PPDU_TX_LIST_T ptmpdatablock;
	KIRQL oldIrql;
	LARGE_INTEGER	timevalue;
	UINT32	count;
	if (devExt == NULL || pConnectDevice == NULL || pLmpPdu == NULL || PduLength < MIN_PDU_LENGTH || PduLength > MAX_PDU_LENGTH)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLC function operates ERROR, devExt= 0x%p, pConnectDevice= 0x%p, pLmpPdu= 0x%p, PduLength= %d\n", devExt, pConnectDevice, pLmpPdu, PduLength);
		return STATUS_UNSUCCESSFUL;
	}
	
	/* Get pointer of the LMP and Frag module */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	pLmp  = (PBT_LMP_T)devExt->pLmp;
	if ((pLmp == NULL) || (pFrag == NULL))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLC function operates ERROR, devExt= 0x%p, pLmp= 0x%p, pFrag= 0x%p\n", devExt, pLmp, pFrag);
		return STATUS_UNSUCCESSFUL;
	}

	
	/* Lock */
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);

	QueueGetCount(&pFrag->Pdu_FreeList, count);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_SendPDUToLC---lmp free list count: %d\n",count);
	
	/*Get a PDU data block from the LMP free pool */
	ptmpdatablock = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_FreeList);
	if (ptmpdatablock == NULL)
	{
		/* Unlock */
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLC function operates ERROR, LMP Tx queue is full, devExt= 0x%p\n", devExt);
		return STATUS_UNSUCCESSFUL;
	}
	pFrag->FreePdu_Count--;
	
	/* Fill some paras */
	ptmpdatablock->pConnectDevice = pConnectDevice;
	ptmpdatablock->data_len = PduLength;
	RtlCopyMemory(ptmpdatablock->data, pLmpPdu, PduLength);
	
	/* Insert the PDU data block into the active list */
	QueuePutTail(&pFrag->Pdu_UsedList, &ptmpdatablock->Link);
	
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

	//Jakio20071220, check tx queue
	timevalue.QuadPart = 0;
	UsbQueryDMASpace(devExt);
	/* Unlock */

	return STATUS_SUCCESS;
}

/*************************************************************
 *   LMP_SendPDUToLCHead
 *
 *   Descriptions:
 *      Inserts one PDU to the linked list head for Tx.
 *
 *   Arguments:
 *      devExt:         IN, pointer to the device.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      PduLength:      IN, the length of PDU.
 *      pLmpPdu:        IN, pointer to the PDU received.
 *
 *   Return Value:
 *      STATUS_SUCCESS:      if inserted into the linked list successfully.
 *      STATUS_UNSUCCESSFUL: if inserted into the linked list failed.
 *************************************************************/
NTSTATUS LMP_SendPDUToLCHead(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PLMP_PUD_PACKAGE_T pLmpPdu, UINT8 PduLength)
{
	PBT_LMP_T  pLmp;
	PBT_FRAG_T pFrag;
	PPDU_TX_LIST_T ptmpdatablock;
	KIRQL oldIrql;
	LARGE_INTEGER	timevalue;
	UINT32 count;
	if (devExt == NULL || pConnectDevice == NULL || pLmpPdu == NULL || PduLength < MIN_PDU_LENGTH || PduLength > MAX_PDU_LENGTH)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLCHead function operates ERROR, devExt= 0x%p, pConnectDevice= 0x%p, pLmpPdu= 0x%p, PduLength= %d\n", devExt, pConnectDevice, pLmpPdu, PduLength);
		return STATUS_UNSUCCESSFUL;
	}
	
	/* Get pointer of the LMP and Frag module */
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	pLmp  = (PBT_LMP_T)devExt->pLmp;
	if ((pLmp == NULL) || (pFrag == NULL))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLCHead function operates ERROR, devExt= 0x%p, pLmp= 0x%p, pFrag= 0x%p\n", devExt, pLmp, pFrag);
		return STATUS_UNSUCCESSFUL;
	}
	
	/* Lock */
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	QueueGetCount(&pFrag->Pdu_FreeList, count);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_SendPDUToLCHead---lmp free list count: %d\n",count);
	/*Get a PDU data block from the LMP free pool */
	ptmpdatablock = (PPDU_TX_LIST_T)QueuePopHead(&pFrag->Pdu_FreeList);
	if (ptmpdatablock == NULL)
	{
		/* Unlock */
		KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_SendPDUToLCHead function operates ERROR, LMP Tx queue is full, devExt= 0x%p\n", devExt);
		return STATUS_UNSUCCESSFUL;
	}
	pFrag->FreePdu_Count--;
	
	/* Fill some paras */
	ptmpdatablock->pConnectDevice = pConnectDevice;
	ptmpdatablock->data_len = PduLength;
	RtlCopyMemory(ptmpdatablock->data, pLmpPdu, PduLength);
	
	/* Insert the PDU data block into the active list */
	QueuePushHead(&pFrag->Pdu_UsedList, &ptmpdatablock->Link);
	
	/* Unlock */
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);

	timevalue.QuadPart = 0;
	UsbQueryDMASpace(devExt);
	return STATUS_SUCCESS;
}

/*************************************************************
 *   LMP_PDUEnter_Task
 *
 *   Descriptions:
 *      Inserts one PDU to the linked list for Tx.
 *
 *   Arguments:
 *      devExt:         IN, pointer to the device.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      pLmpPdu:        IN, pointer to the PDU processed.
 *      PduLength:      IN, the length of PDU.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_PDUEnter_Task(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, PUINT8 pLmpPdu, UINT8 PduLength)
{
	if (devExt == NULL || pConnectDevice == NULL || pLmpPdu == NULL || PduLength < MIN_PDU_LENGTH || PduLength > MAX_PDU_LENGTH)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "LMP----LMP_PDUEnterBD_Task function operates ERROR, devExt= 0x%p, pConnectDevice= 0x%p, pLmpPdu= 0x%p, PduLength= %d\n", devExt, pConnectDevice, pLmpPdu, PduLength);
		return;
	}

	if (((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 3 && ((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 23) /* accepted sniff */
	{
		pConnectDevice->mode_Sniff_debug1 = 1;
		pConnectDevice->Sniff_RetryCount = 0;

		Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_ENTER_SNIFF), BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		return;
	}

	if (((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 3 && ((PLMP_PUD_PACKAGE_T)pLmpPdu)->contents[0] == 24) /* accepted unsniff */
	{
		pConnectDevice->Sniff_RetryCount = 0;

		Task_CreateTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_LEAVE_SNIFF), BT_TASK_PRI_NORMAL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		return;
	}

	if (((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 60)
	{
		if(devExt->AfhPduCount > 0)
			devExt->AfhPduCount--;
	}

	#ifdef BT_TESTMODE_SUPPORT
	if(((PLMP_PUD_PACKAGE_T)pLmpPdu)->OpCode == 3)
	{
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "Testmode lmp pdu has been put into dsp buffer, allow writing cmd indicator\n");
			devExt->AllowCmdIndicator = TRUE;
		}
	}
	#endif
	return;
}

/*************************************************************
 *   LMP_StartPDUTimer
 *
 *   Descriptions:
 *      Start a timer for LMP PDU, with KeAcquireSpinLock().
 *
 *   Arguments:
 *      devExt:         IN, pointer to device extention context.
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type:     IN, timer type.
 *      timer_count:    IN, timer count value.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_StartPDUTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_type, UINT16 timer_count)
{
	PBT_HCI_T pHci;
	KIRQL     oldIrql;
	
	if (devExt == NULL || pConnectDevice == NULL)
		return;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;
	
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	pConnectDevice->lmp_timer_valid   = 1;           /* Set timer start flag  */
	pConnectDevice->lmp_timer_counter = timer_count; /* Set timer count value */
	pConnectDevice->lmp_timer_type    = timer_type;  /* Set timer count type  */
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP------LMP_StartPDUTimer (BD0=0x%x), type= %d, count= %d\n", pConnectDevice->bd_addr[0], timer_type, timer_count);
	return;
}

/*************************************************************
 *   LMP_StopPDUTimer
 *
 *   Descriptions:
 *      Stop a timer for LMP PDU, with KeAcquireSpinLock().
 *
 *   Arguments:
 *      devExt:         IN, pointer to device extention context.
 *      pConnectDevice: IN, pointer to connect device.
 *      timer_type:     IN, timer type.
 *      timer_count:    IN, timer count value.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_StopPDUTimer(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	KIRQL oldIrql;
	PBT_HCI_T pHci;
	UINT8 timer_type = PDU_TIMER_IDLE;
	
	if (devExt == NULL || pConnectDevice == NULL)
		return;
	
	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;
	
	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
	timer_type = pConnectDevice->lmp_timer_type;
	pConnectDevice->lmp_timer_valid   = 0;               /* Clear timer start flag  */
	pConnectDevice->lmp_timer_counter = 0;               /* Clear timer count value */
	pConnectDevice->lmp_timer_type    = PDU_TIMER_IDLE;  /* Clear timer count type  */
	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	
	if (timer_type != PDU_TIMER_IDLE)
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP------LMP_StopPDUTimer (BD0=0x%x), type= %d\n", pConnectDevice->bd_addr[0], timer_type);
	
	return;
}

/*************************************************************
 *   LMP_PDU_StopTimer
 *
 *   Descriptions:
 *      Stop a timer of LMP PDU without KeAcquireSpinLock(), Just for HCI module.
 *
 *   Arguments:
 *      pConnectDevice: IN, pointer to connect device.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_PDU_StopTimer(PCONNECT_DEVICE_T pConnectDevice)
{
	if (pConnectDevice == NULL)
		return;
	
	pConnectDevice->lmp_timer_valid   = 0;              /* Clear timer start flag  */
	pConnectDevice->lmp_timer_counter = 0;              /* Clear timer count value */
	pConnectDevice->lmp_timer_type    = PDU_TIMER_IDLE; /* Clear timer count type  */

	return;
}

/*************************************************************
 *   LMP_PDU_Timeout_Function
 *
 *   Descriptions:
 *      Time out executing function for LMP module.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      timer_type:     IN, the timer type.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_PDU_Timeout_Function(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT8 timer_type)
{
	UINT8 tempvalue = 0;
	LARGE_INTEGER timevalue;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	devExt->AllowIntSendingFlag = FALSE;
	switch (timer_type)
	{
		case PDU_TIMER_SENT_CLKOFFSET_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Clock Offset Req\n", pConnectDevice->bd_addr[0]);

			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CLKOFFSET_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_VER_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Version Req\n", pConnectDevice->bd_addr[0]);

			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			switch (pConnectDevice->state)
			{
			case (BT_DEVICE_STATE_PAGED): 
    			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			default:
    			Task_HCI2HC_Send_RemoteVerInfo_Complete_Event(devExt, (PUINT8) &pConnectDevice, 1);
			}
			break;
			
		case PDU_TIMER_SENT_FEATURES_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Features Req\n", pConnectDevice->bd_addr[0]);

			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			switch (pConnectDevice->state)
			{
			case (BT_DEVICE_STATE_PAGED): 
    			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			default:
                /* Send Complete Event with failure status */
    			Task_HCI2HC_Send_RemoteNameSuppFea_Complete_Event(devExt, (PUINT8) &pConnectDevice, 1);
			}
			break;
			
		case PDU_TIMER_SENT_NAME_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Name Req\n", pConnectDevice->bd_addr[0]);

			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			pConnectDevice->remote_name[0]      = 0;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_NAME_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
            /**
			if (pConnectDevice->tempflag != 1)
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				**/
			break;
			
		case PDU_TIMER_SENT_AURAND:
		case PDU_TIMER_SENT_INRAND:
		case PDU_TIMER_SENT_COMB_KEY:
			if (timer_type == PDU_TIMER_SENT_AURAND)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Authentication Rand\n", pConnectDevice->bd_addr[0]);
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND)
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			}
			else if (timer_type == PDU_TIMER_SENT_INRAND)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Initialization Rand\n", pConnectDevice->bd_addr[0]);
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_INRAND)
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			}
			else
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Combination Key\n", pConnectDevice->bd_addr[0]);
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_WAIT_COM_KEY)
					ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			}
			
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) == 0)
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_ENMODE_REQ:
		case PDU_TIMER_SENT_ENKEYSIZE_REQ:
		case PDU_TIMER_SENT_STARTEN_REQ:
		case PDU_TIMER_SENT_STOPTEN_REQ:
			if (timer_type == PDU_TIMER_SENT_ENMODE_REQ)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Encryption Mode Req\n", pConnectDevice->bd_addr[0]);
			}
			else if (timer_type == PDU_TIMER_SENT_ENKEYSIZE_REQ)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Encryption Key Size Req\n", pConnectDevice->bd_addr[0]);
			}
			else if (timer_type == PDU_TIMER_SENT_STARTEN_REQ)
			{
				pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr, pBuf, 1, 0);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Start Encryption Req\n", pConnectDevice->bd_addr[0]);
			}
			else
			{
				pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + pConnectDevice->am_addr, pBuf, 1, 3);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SET_CONN_ENCRYPTION_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Stop Encryption Req\n", pConnectDevice->bd_addr[0]);
			}
			
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) >= LMP_SEND_ENMODE_REQ || (pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ)
				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			pConnectDevice->is_in_encryption_process = 0;
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) == 0)
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_HOLD_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Hold Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_HOLD_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);

			CancelLmpBitState(devExt, pConnectDevice, LMP_HOLD_ACCEPTED, 1);
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;

		case PDU_TIMER_SENT_SNIFF_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Sniff Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_SNIFF_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			if (((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].init_flag == SNIFF_INIT_FLAG)
			{
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index], sizeof(BT_SNIFF_T));
				pConnectDevice->sniff_mode_interval = 0;
				pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_UNSNIFF_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Unsniff Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			Frag_ListStopSwitch(devExt, pConnectDevice, 0);
			
			if ((pConnectDevice->lmp_ext_states & LMP_SEND_UNSNIFF_DETACH) == LMP_SEND_UNSNIFF_DETACH)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
				{
					Frag_FlushFragAndBD(devExt, NULL, FRAG_SNIFF_LIST);
					tempvalue = 1;
				}
				else
				{
					tempvalue = (UINT8)(pConnectDevice->slave_index << 1);
				}
				
				pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
				pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_AMADDR, pBuf, 1, (tempvalue << 4) + pConnectDevice->am_addr);
				pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
				pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT);
				RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				
				pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
				DeleteSniffLink(devExt, pConnectDevice);
				CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_DETACH, 1);
				ResLmpDetach(devExt, pConnectDevice, pConnectDevice->current_reason_code);
			}
			else
			{
				pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case PDU_TIMER_SENT_SCOLINK_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), SCO Link Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_SCO_LINK_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_ADD_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_SCO_T));
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == SCO_CHANGE_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = LMP_RESPONSE_TIMEOUT;
				Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_SCO_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case PDU_TIMER_SENT_REMOVE_SCOLINK_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Remove SCO Link Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_REMOVE_SCO_LINK_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
#ifdef BT_TESTMODE_SUPPORT
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			DeleteSCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_ESCOLINK_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), eSCO Link Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_ESCO_LINK_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			if (((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == ESCO_ADD_LINK)
			{
				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_ESCO_T));
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			else if (((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag == ESCO_CHANGE_LINK)
			{
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].change_flag = 0;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			((PBT_LMP_T)devExt->pLmp)->esco_counter = 0;
			break;
			
		case PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Remove eSCO Link Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_REMOVE_ESCO_LINK_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
#ifdef BT_TESTMODE_SUPPORT
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			DeleteESCOLink(devExt, ((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11);
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_CONN_REQ:
		case PDU_TIMER_SENT_SETUP_COMPLETE:
			if (timer_type == PDU_TIMER_SENT_CONN_REQ)
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Connection Req\n", pConnectDevice->bd_addr[0]);
			else
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Setup Complete\n", pConnectDevice->bd_addr[0]);
			
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			ResLmpDetach(devExt, pConnectDevice, pConnectDevice->current_reason_code);
			break;
			
		case PDU_TIMER_SENT_MAX_SLOT_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Max Slot Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_MAX_SLOT_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_CHANGE_PACKET_TYPE_COMPLETE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case PDU_TIMER_SENT_PACKET_TYPE_TABLE_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Packet Type Table Req\n", pConnectDevice->bd_addr[0]);
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_PACKET_TYPE_TABLE_REQ)
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			pConnectDevice->edr_change_flag = 0;
			break;

		case PDU_TIMER_SENT_FEATURES_REQ_EXT:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Features Req Ext\n", pConnectDevice->bd_addr[0]);
			
			pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
			Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_LMP_TIMEOUT, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			if (pConnectDevice->current_page_number != 0xFF)
			{
				pConnectDevice->max_page_number = 0;
				RtlZeroMemory(&(pConnectDevice->remote_extended_feature), 8);
				Hci_Receive_From_LMP(devExt,BT_LMP_EVENT_EXTENDED_FEATURE_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;
			
		case PDU_TIMER_SENT_PAUSEENCRYPTION_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Pause Encryption Req\n", pConnectDevice->bd_addr[0]);
			
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			pConnectDevice->is_in_encryption_process = 0;
			
			if (pConnectDevice->pause_encryption_status == 2)
			{
				if (pConnectDevice->pause_command_flag == 1)
				{
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}
			
			pConnectDevice->pause_encryption_status = 0;
			//Jakio20071220, check tx queue
			timevalue.QuadPart = 0;
			//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
			UsbQueryDMASpace(devExt);
			break;
			
		case PDU_TIMER_SENT_RESUMEENCRYPTION_REQ:
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Timeout (BD0=0x%x), Resume Encryption Req\n", pConnectDevice->bd_addr[0]);
			
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_SETUP);
			
			pConnectDevice->is_in_encryption_process = 0;
			
			if (pConnectDevice->pause_encryption_status == 2 || pConnectDevice->pause_command_flag == 1)
			{
				pConnectDevice->current_reason_code = LMP_RESPONSE_TIMEOUT;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			
			pConnectDevice->pause_encryption_status = 0;
			//Jakio20071220, check tx queue
			timevalue.QuadPart = 0;
			//KeSetTimer(&((PBT_FRAG_T)devExt->pFrag)->TxTimer, timevalue, &((PBT_FRAG_T)devExt->pFrag)->TxDPC);
			UsbQueryDMASpace(devExt);
			break;
			
		default :
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----PDU Timeout (BD0=0x%x), TimerType= %d, no this type\n", pConnectDevice->bd_addr[0], timer_type);
			break;
	}
	
	devExt->AllowIntSendingFlag = TRUE;
	timevalue.QuadPart = 0;
	tasklet_schedule(&devExt->taskletRcv);
	return;
}

/*************************************************************
 *   LMP_PDU_Timeout
 *
 *   Descriptions:
 *      Time out function for LMP module.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extention context.
 *
 *   Return Value:
 *      None
 *************************************************************/
void LMP_PDU_Timeout(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_HCI_T  pHci;
	UINT8 timer_type = PDU_TIMER_IDLE;
	PCONNECT_DEVICE_T pConnectDevice;

	if (devExt == NULL)
		return;

	pHci = (PBT_HCI_T)devExt->pHci;
	if (pHci == NULL)
		return;

	KeAcquireSpinLock(&pHci->HciLock, &oldIrql);

	/* Scan master list */
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_am_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->lmp_timer_valid == 1 && --pConnectDevice->lmp_timer_counter == 0)
		{
			timer_type = pConnectDevice->lmp_timer_type;
			LMP_PDU_StopTimer(pConnectDevice);

			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			LMP_PDU_Timeout_Function(devExt, pConnectDevice, timer_type);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}

		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}

	/* Scan slave list */
	pConnectDevice = (PCONNECT_DEVICE_T)QueueGetHead(&pHci->device_slave_list);
	while (pConnectDevice != NULL)
	{
		if (pConnectDevice->lmp_timer_valid == 1 && --pConnectDevice->lmp_timer_counter == 0)
		{
			timer_type = pConnectDevice->lmp_timer_type;
			LMP_PDU_StopTimer(pConnectDevice);

			KeReleaseSpinLock(&pHci->HciLock, oldIrql);
			LMP_PDU_Timeout_Function(devExt, pConnectDevice, timer_type);
			KeAcquireSpinLock(&pHci->HciLock, &oldIrql);
		}

		pConnectDevice = (PCONNECT_DEVICE_T)QueueGetNext(&pConnectDevice->Link);
	}

	KeReleaseSpinLock(&pHci->HciLock, oldIrql);
	return;
}

/*************************************************************
 *   LMP_HC2LM_Command_Process
 *
 *   Descriptions:
 *      Handle the commands received from HC.
 *
 *   Arguments:
 *      devExt:         IN, pointer to the device.
 *      pConnectDevice: IN, pointer to the ConnectDevice.
 *      Command:        IN, the command from HC.
 *
 *   Return Value:
 *      STATUS_SUCCESS:          if command is processed successfully.
 *      RPC_NT_NULL_REF_POINTER: if the IN pointer is NULL.
 *      ......
 *      STATUS_UNSUCCESSFUL:     all the remain of unsuccessful situation.
 *************************************************************/
NTSTATUS LMP_HC2LM_Command_Process(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice, UINT32 Command)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LMP_PUD_PACKAGE_T pdu;
	UINT16 content[2];
	UINT8  i, tempvalue;
	LARGE_INTEGER timevalue;
	PSCO_CONNECT_DEVICE_T	pScoConnectDev = NULL;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	if (devExt == NULL || pConnectDevice == NULL)
		return RPC_NT_NULL_REF_POINTER;

	RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
	pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;

	switch (Command)
	{
		case BT_TASK_EVENT_HCI2LMP_SET_AFH: /* 22 */
			if (pConnectDevice->current_role != BT_ROLE_MASTER || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cap_master == 0 || pConnectDevice->lmp_features.byte4.afh_cap_slave == 0)
				break;
			
			status = STATUS_SUCCESS;
#ifdef BT_AFH_DISABLE_WHEN_SNIFF
			if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
			{
				if (pConnectDevice->afh_mode == AFH_ENABLED && pConnectDevice->is_afh_sent_flag == 1)
				{
					if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cla_master == 1 && pConnectDevice->lmp_features.byte4.afh_cla_slave == 1)
						status = Send_ChannelClassificationReq_PDU(devExt, pConnectDevice, AFH_REPORTING_DISABLED);
					
					status = Send_SetAFH_PDU(devExt, pConnectDevice, AFH_DISABLED, 0);
				}
				break;
			}
#endif
			pConnectDevice->afh_mode = AFH_ENABLED;
			if (pConnectDevice->role_switching_flag == 0 && (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_ACTIVE || pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF))
			{
				status = Send_SetAFH_PDU(devExt, pConnectDevice, AFH_ENABLED, 0);
				
				if (pConnectDevice->is_afh_sent_flag == 0)
				{
					pConnectDevice->is_afh_sent_flag = 1; /* not-sending channel_classification_req */
					if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.afh_cla_master == 1 && pConnectDevice->lmp_features.byte4.afh_cla_slave == 1)
						status = Send_ChannelClassificationReq_PDU(devExt, pConnectDevice, AFH_REPORTING_ENABLED);
				}
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_WRITE_AFH_COMMAND: /* 39 */
			if (devExt->pHci)
			{
				if (((PBT_HCI_T)devExt->pHci)->need_write_afh_command)
				{
					BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Write change master afh mode command indicator, need_write_afh_command= %d\n", ((PBT_HCI_T)devExt->pHci)->need_write_afh_command);
					pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
					pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_CHANGE_AFH_MODE_BIT);
					RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
				}
				else
				{
					BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Can not write change master afh mode command indicator\n");
				}
			}
			status = STATUS_SUCCESS;
			break;
			
		case BT_TASK_EVENT_HCI2LMP_SET_CLASSIFICATION: /* 32 */
			if (pConnectDevice->current_role != BT_ROLE_SLAVE || pConnectDevice->send_classification_flag != AFH_REPORTING_ENABLED)
				break;

			status = STATUS_SUCCESS;
			if (pConnectDevice->role_switching_flag == 0)
				status = Send_ChannelClassification_PDU(devExt, pConnectDevice, ((PBT_HCI_T)devExt->pHci)->afh_ch_assessment_mode);
			break;

		case BT_TASK_EVENT_HCI2LMP_ACCEPT_CONN: /* 8 */
			status = Command_AcceptConn(devExt, pConnectDevice);
			break;

		case BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_CONN: /* 9 */
			if ((pConnectDevice->lmp_states & LMP_RECV_CONN_REQ) == LMP_RECV_CONN_REQ)
			{
				CancelLmpBitState(devExt, pConnectDevice, LMP_RECV_CONN_REQ, 0); /* cancel LMP_RECV_CONN_REQ */
				
				pdu.TransID = MASTER_TRANSACTION_ID;
				pdu.OpCode  = 51;
				status = ResLmpNotAccepted(devExt, pConnectDevice, &pdu, pConnectDevice->current_reason_code);
			}
			else if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_RECV_SCO_LINK_REQ && ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice == pConnectDevice)
			{
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);

				RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_SCO_T));
				pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? SLAVE_TRANSACTION_ID : MASTER_TRANSACTION_ID;
				pdu.OpCode  = 43;
				status = ResLmpNotAccepted(devExt, pConnectDevice, &pdu, pConnectDevice->current_reason_code);
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_ROLE_SWITCH: /* 24 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.slot_offset == 0 || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.switchbit == 0 || (pConnectDevice->link_policy_settings & 0x0001) == 0 
				|| pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE || pConnectDevice->pScoConnectDevice != NULL)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			if (((PBT_HCI_T)devExt->pHci)->role_switching_flag == 1)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----Role switching flag has been set\n");
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
#ifdef BT_ROLESWITCH_UNALLOWED_WHEN_MULTI_SLAVE
			if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_am > 1)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
#endif
			if (pConnectDevice->encryption_mode == BT_ENCRYPTION_DIABLE)
			{
				((PBT_HCI_T)devExt->pHci)->role_switching_flag = 1;
				if (pConnectDevice->current_role == BT_ROLE_SLAVE)
					status = Send_SlotOffset_PDU(devExt, pConnectDevice, SLAVE_TRANSACTION_ID);
				
				status = Send_SwitchReq_PDU(devExt, pConnectDevice);
			}
			else
			{
				if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.pause_encryption == 1 && pConnectDevice->lmp_features.byte5.pause_encryption == 1)
				{
					((PBT_HCI_T)devExt->pHci)->role_switching_flag = 1;
					pConnectDevice->pause_encryption_status = 2;
					status = Send_PauseEncryptionReq_PDU(devExt, pConnectDevice, pdu.TransID);
				}
				else
				{
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ROLE_SWITCH_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_CREATE_CONNECTION: /* 2 */
			if (pConnectDevice->current_role != BT_ROLE_MASTER)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
			pdu.OpCode = 51;
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
			if (NT_SUCCESS(status))
			{
				ChangeLmpState(devExt, pConnectDevice, LMP_SEND_CONN_REQ);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Connection Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
				
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_CONN_REQ, LMP_MAX_RESPONSE_TIME);
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_LINK_SUPERVISION_TIMEOUT: /* 3 */
			if (pConnectDevice->current_role != BT_ROLE_MASTER)
				break;

			pdu.OpCode = 55;
			RtlCopyMemory(pdu.contents, &pConnectDevice->real_link_supervision_timeout, sizeof(UINT16));
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
			if (NT_SUCCESS(status))
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Supervision Timeout, Timeout= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pConnectDevice->real_link_supervision_timeout);
			break;

		case BT_TASK_EVENT_HCI2LMP_DETACH: /* 10 */
			if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
			{
				status = STATUS_SUCCESS;
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_DETACH);
				if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
					break;
				
				for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
				{
					if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
						break;
				}
				if (i < ((PBT_LMP_T)devExt->pLmp)->sniff_number)
				{
					((PBT_LMP_T)devExt->pLmp)->sniff_index = i;
					Send_UnsniffReq_PDU(devExt, pConnectDevice);
					break;
				}
				
				pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
			}

			pdu.OpCode = 7;
			pdu.contents[0] = pConnectDevice->current_reason_code;

			if (pConnectDevice->current_reason_code == TERMINATED_USER_ENDED)
				pConnectDevice->current_reason_code = TERMINATED_LOCAL_HOST; /* test protocol 5.7.4.3(close link) */

			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
			if (NT_SUCCESS(status))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Detach, reason: 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0]);

				ChangeLmpState(devExt, pConnectDevice, LMP_CONN_IDLE);
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_IDLE);
				LMP_ReleaseSCOMembers(devExt, pConnectDevice);
#ifdef BT_TESTMODE_SUPPORT
				TestMode_ResetMembers(devExt, pConnectDevice);
#endif
				LMP_StopPDUTimer(devExt, pConnectDevice);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_AUTH_REQUEST: /* 16 */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_SEND_AURAND || 
				((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) >= LMP_SEND_ENMODE_REQ 
				&& (pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ))
			{
				pConnectDevice->current_reason_code = PDU_NOT_ALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}

			status = Authentication_Process(devExt, pConnectDevice);
			break;

		case BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_ON:  /* 14 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.encry == 0 || pConnectDevice->lmp_features.byte0.encry == 0)
			{
				pConnectDevice->current_reason_code = UNSUPPORTED_LMP_FEATURE;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}

			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) >= LMP_SEND_ENMODE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ)
				break;

			if (pConnectDevice->encryption_mode != BT_ENCRYPTION_DIABLE)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				status = STATUS_SUCCESS;
			}
			else
			{
				/* need judge whether equaling 2 */
				pConnectDevice->encryption_mode = BT_ENCRYPTION_ONLY_P2P;
				status = Send_EnModReq_PDU(devExt, pConnectDevice);
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_SET_ENCRYPTION_OFF: /* 15 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.encry == 0 || pConnectDevice->lmp_features.byte0.encry == 0)
			{
				pConnectDevice->current_reason_code = UNSUPPORTED_LMP_FEATURE;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}

			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) >= LMP_SEND_ENMODE_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) <= LMP_SEND_STOPTEN_REQ)
				break;
			
			if (pConnectDevice->encryption_mode == BT_ENCRYPTION_DIABLE)
			{
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ENCRYPTION_SUCCESS, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				status = STATUS_SUCCESS;
			}
			else
			{
				pConnectDevice->encryption_mode = BT_ENCRYPTION_DIABLE;
				status = Send_EnModReq_PDU(devExt, pConnectDevice);
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_LINK_KEY_REPLY: /* 11 */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_NO_LINKKEY && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_NO_LINKKEY_REC_AURAND && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_REC_AURAND_NO_LINKKEY)
				break;
			
			status = STATUS_SUCCESS;
			for (i = 0; i < 16; i++)
			{
				if (pConnectDevice->link_key[i] != 0)
					break;
			}	
			if (i == 16)
			{
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_NO_LINKKEY)
				{
					pdu.TransID     = pConnectDevice->aco[0];
					pdu.OpCode      = 4;
					pdu.contents[0] = 11;
					pdu.contents[1] = KEY_MISSING;
					
					status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
					if (NT_SUCCESS(status))
						BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Not Accepted Authentication Rand, ErrReason: KEY_MISSING\n", pdu.TransID, pConnectDevice->bd_addr[0]);
				}

				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_REC_AURAND_NO_LINKKEY)
					status = Pairing_Process(devExt, pConnectDevice);
				else
					ChangeLmpState(devExt, pConnectDevice, LMP_STATE_ANDVALUE); /* cancel LMP_REC_AURAND_NO_LINKKEY */
			}
			else
			{
				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_NO_LINKKEY)
				{
					pdu.TransID = pConnectDevice->aco[0];
					pdu.OpCode  = 12;
					
					E1_Authentication_KeyGen(pConnectDevice->link_key, pConnectDevice->random, ((PBT_HCI_T)devExt->pHci)->local_bd_addr, pdu.contents, pConnectDevice->aco);
					status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 5);
					if (NT_SUCCESS(status))
						BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Secret Response\n", pdu.TransID, pConnectDevice->bd_addr[0]);
				}

				if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_REC_AURAND_NO_LINKKEY)
				{
					i = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
					status = Send_AuRand_PDU(devExt, pConnectDevice, i);
				}
				else
				{
					ChangeLmpState(devExt, pConnectDevice, LMP_STATE_ANDVALUE); /* cancel LMP_REC_AURAND_NO_LINKKEY */
				}
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_PIN_CODE_NEG_REPLY: /* 12 */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_PIN_REQ && 
				(pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_PIN_REQ_REC_INRAND)
				break;

			status = STATUS_SUCCESS;
			devExt->AllowIntSendingFlag = FALSE;
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_PIN_REQ)
			{
				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				{
					pConnectDevice->current_reason_code = PAIRING_NOT_ALLOWED;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PAIR_NOT_ALLOW, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					status = ResLmpDetach(devExt, pConnectDevice, PAIRING_NOT_ALLOWED);
				}
				else
				{
					pConnectDevice->current_reason_code = KEY_MISSING;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
			}
			else
			{
				if ((pConnectDevice->lmp_states & (LMP_SEND_CONN_REQ + LMP_RECV_CONN_REQ)) != 0)
				{
					pConnectDevice->current_reason_code = PAIRING_NOT_ALLOWED;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_PAIR_NOT_ALLOW, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}
				else
				{
					pConnectDevice->current_reason_code = KEY_MISSING;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_AUTH_COMP_FAILURE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				}

				pdu.TransID = pConnectDevice->aco[0];
				pdu.OpCode  = 8;
				status = ResLmpNotAccepted(devExt, pConnectDevice, &pdu, PAIRING_NOT_ALLOWED);
			}

			ChangeLmpState(devExt, pConnectDevice, LMP_STATE_ANDVALUE); /* cancel flags */
			
			devExt->AllowIntSendingFlag = TRUE;
			timevalue.QuadPart = 0;
			tasklet_schedule(&devExt->taskletRcv);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_PIN_CODE_REPLY: /* 13 */
			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_PIN_REQ && (pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) != LMP_PIN_REQ_REC_INRAND)
				break;

			if ((pConnectDevice->lmp_states & LMP_STATE_ANDVALUE) == LMP_PIN_REQ)
			{
				status = Send_InRand_PDU(devExt, pConnectDevice, pdu.TransID);
			}
			else
			{
				if (((PBT_HCI_T)devExt->pHci)->pin_type == PIN_FIXED)
					status = Send_InRand_PDU(devExt, pConnectDevice, pConnectDevice->aco[0]); /* aco[0] holds the TransID casually */
				else
					status = Res_InRand_Accepted(devExt, pConnectDevice, pConnectDevice->aco[0]);
			}
			break;

		case BT_TASK_EVENT_HCI2LMP_CLK_OFFSET_REQ: /* 4 */
			if (pConnectDevice->current_role != BT_ROLE_MASTER)
			{
				if (pConnectDevice->slave_index == 0)
					content[0] = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_AM_CLK_OFFSET_SLAVE0);
				else
					content[0] = Usb_Read_2Bytes_From_3DspReg(devExt, BT_REG_AM_CLK_OFFSET_SLAVE1);

				RtlCopyMemory(&pConnectDevice->clock_offset, &content[0], sizeof(UINT16));
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_CLKOFFSET_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				status = STATUS_SUCCESS;
				break;
			}

			pdu.OpCode = 5;
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 1);
			if (NT_SUCCESS(status))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Clock Offset Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_CLKOFFSET_REQ, 10);
			}
			break;
		case BT_TASK_EVENT_HCI2LMP_NAME_REQ: /* 5 */


			pdu.OpCode      = 1;
			pdu.contents[0] = 0;

			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
			if (NT_SUCCESS(status))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Name Req, NameOffset= 0\n", pdu.TransID, pConnectDevice->bd_addr[0]);
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_NAME_REQ, 10);
			}
			break;
		case BT_TASK_EVENT_HCI2LMP_FEATURE_REQ: /* 6 */
			status = Send_FeatureReq_PDU(devExt, pConnectDevice);
			break;

		case BT_TASK_EVENT_HCI2LMP_EXTENDED_FEATURE_REQ: /* 40 */
			if (pConnectDevice->current_page_number == 0xFF || pConnectDevice->lmp_features.byte7.extended_features == 0)
			{
				pConnectDevice->max_page_number = 0;
				RtlZeroMemory(&(pConnectDevice->remote_extended_feature), 8);
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_EXTENDED_FEATURE_RES, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				status = STATUS_SUCCESS;
				break;
			}

			status = Send_FeatureReqExt_PDU(devExt, pConnectDevice, pConnectDevice->current_page_number);
			break;

		case BT_TASK_EVENT_HCI2LMP_VERSION_REQ: /*   7   */
			pdu.OpCode      = 37;
			pdu.contents[0] = ((PBT_HCI_T)devExt->pHci)->lmp_version;
			content[0]      = ((PBT_HCI_T)devExt->pHci)->manufacturer_name;
			content[1]      = ((PBT_HCI_T)devExt->pHci)->lmp_subversion;
			RtlCopyMemory(&pdu.contents[1], content, sizeof(content));

			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 6);
			if (NT_SUCCESS(status))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Version Req, %d %d %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0], content[0], content[1]);
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_VER_REQ, 10);
			}
			break;
			
		case BT_TASK_EVENT_HCI2LMP_ACCEPT_ESCO_CONN: /* 29 */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_RECV_ESCO_LINK_REQ || pConnectDevice->pScoConnectDevice == NULL 
				|| ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);
			status = HostAccept_eSCOLink(devExt, pConnectDevice);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_NOT_ACCEPT_ESCO_CONN: /* 30 */
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) != LMP_RECV_ESCO_LINK_REQ || pConnectDevice->pScoConnectDevice == NULL 
				|| ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice != pConnectDevice)
				break;
			
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_CONN_SETUP);			
			RtlZeroMemory(&((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11], sizeof(BT_ESCO_T));
			
			pdu.TransID     = (pConnectDevice->current_role == BT_ROLE_MASTER) ? SLAVE_TRANSACTION_ID : MASTER_TRANSACTION_ID;
			pdu.OpCode      = 127;
			pdu.contents[0] = 12;
			status = ResLmpNotAcceptedExt(devExt, pConnectDevice, &pdu, pConnectDevice->current_reason_code);
			
			((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
			break;
			
		case BT_TASK_EVENT_HCI2LMP_REMOVE_ESCO: /* 31 */
			if (pConnectDevice->pScoConnectDevice == NULL || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 0 || pConnectDevice->lmp_features.byte3.ext_sco_link == 0)
			{
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			pdu.contents[0] = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
			for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle;
				else
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->esco[i].hc_esco_handle;
				
				if (pdu.contents[1] == pdu.contents[0] && ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pConnectDevice)
					break;
			}
			if (i >= (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
			{
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));
			
			pdu.OpCode      = 127;
			pdu.contents[0] = 13;
			pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle;
			pdu.contents[2] = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code;
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 4);
			if (NT_SUCCESS(status))
			{
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_REMOVE_ESCO_LINK_REQ);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Remove eSCO Link Req, eSCOHandle= %d, ErrorCode= %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[1], pdu.contents[2]);
				
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ, LMP_MAX_RESPONSE_TIME);
			}
			
#ifndef BT_TESTMODE_SUPPORT
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code == TERMINATED_USER_ENDED)
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_reason_code = TERMINATED_LOCAL_HOST;
			
			break;
			
		case BT_TASK_EVENT_HCI2LMP_CHANGE_ESCO: /* 28 */
			if (pConnectDevice->pScoConnectDevice == NULL || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 0 || pConnectDevice->lmp_features.byte3.ext_sco_link == 0)
			{
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			pdu.contents[0] = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
			for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->esco[i].esco_handle;
				else
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->esco[i].hc_esco_handle;
				
				if (pdu.contents[1] == pdu.contents[0] && ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pConnectDevice)
					break;
			}
			if (i >= (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
			{
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			tempvalue = GetDefaultESCOParameters(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[i]);
			if (tempvalue != BT_HCI_STATUS_SUCCESS)
			{
				pConnectDevice->current_reason_code = tempvalue;
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_ESCO_CHANGE_FAIL, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			((PBT_LMP_T)devExt->pLmp)->esco[i].D_esco = ESCO_DEFAULT_D;
			((PBT_LMP_T)devExt->pLmp)->esco[i].D_esco = LMP_GetOffsetOfSCO(devExt, ((PBT_LMP_T)devExt->pLmp)->esco[i].T_esco, ((PBT_LMP_T)devExt->pLmp)->esco[i].D_esco);
			LMP_SCONeedUnsniff(devExt, pConnectDevice);

			((PBT_LMP_T)devExt->pLmp)->esco[i].W_esco            = ESCO_DEFAULT_W;
			((PBT_LMP_T)devExt->pLmp)->esco[i].negotiation_state = ESCO_NEGO_INIT;
			((PBT_LMP_T)devExt->pLmp)->esco[i].change_flag       = ESCO_CHANGE_LINK;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));
			status = Send_eSCOLinkReq_PDU(devExt, pConnectDevice, pdu.TransID);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_ADD_ESCO: /* 27 */
			if (pConnectDevice->pScoConnectDevice == NULL || (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) >= SCO_MAX_NUMBER || 
				((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 0 || pConnectDevice->lmp_features.byte3.ext_sco_link == 0)
			{
				if (pConnectDevice->pScoConnectDevice != NULL && ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr != 0)
					Hci_Free_Am_address((PBT_HCI_T)devExt->pHci, ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr);
				
				/* no break here for executing add_sco below continually */
			}
			else
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
				{
					if ((UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle) == 0 
						|| ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr == 0 || ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr > 7)
					{
						((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
						break;
					}
					
					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].esco_LT_addr = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->esco_amaddr;
					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].esco_handle  = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
					((PBT_LMP_T)devExt->pLmp)->esco_counter = 0;
				}
				else
				{
					if (((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->slave_sco_master_not_coexist_flag == 1)
					{
						((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
						Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
						status = STATUS_SUCCESS;
						break;
					}

					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].hc_esco_handle = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].esco_LT_addr   = 0;
					((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].esco_handle    = 0;
				}
				
				tempvalue = GetDefaultESCOParameters(devExt, pConnectDevice, &((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6]);
				if (tempvalue != BT_HCI_STATUS_SUCCESS)
				{
					pConnectDevice->current_reason_code = tempvalue;
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_ESCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					break;
				}
				
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_esco = ESCO_DEFAULT_D;
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_esco = LMP_GetOffsetOfSCO(devExt, ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].T_esco, ((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_esco);
				LMP_SCONeedUnsniff(devExt, pConnectDevice);

				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].W_esco            = ESCO_DEFAULT_W;
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].negotiation_state = ESCO_NEGO_INIT;
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].pDevice           = pConnectDevice;
				((PBT_LMP_T)devExt->pLmp)->esco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].change_flag       = ESCO_ADD_LINK;
				((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));
				status = Send_eSCOLinkReq_PDU(devExt, pConnectDevice, pdu.TransID);
				break;
			}
			
		case BT_TASK_EVENT_HCI2LMP_ADD_SCO: /* 17 */
			if (pConnectDevice->pScoConnectDevice == NULL || (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) >= SCO_MAX_NUMBER || 
				((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 0 || pConnectDevice->lmp_features.byte1.sco_link == 0)
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "Task_event_hci2lmp_add_sco error\n");
				BT_DBGEXT(ZONE_LMP | LEVEL3, "sco indicator:%d\n",((PBT_LMP_T)devExt->pLmp)->sco_indicator);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "sco link feature1:%d\n",pConnectDevice->lmp_features.byte1.sco_link);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "sco link feature2:%d\n",((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link);
				((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
			{
				if ((UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle) == 0)
				{
					BT_DBGEXT(ZONE_LMP | LEVEL3, "Task_event_hci2lmp_add_sco error: connection handle is 0\n");
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					break;
				}
				
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].sco_handle = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_sco      = SCO_DEFAULT_D;
				((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
			}
			else
			{
				if (((PBT_HCI_T)devExt->pHci)->num_device_am > 0 && ((PBT_HCI_T)devExt->pHci)->slave_sco_master_not_coexist_flag == 1)
				{
					((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->link_type = BT_LINK_TYPE_SCO;
					Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_DETACH, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
					status = STATUS_SUCCESS;
					break;
				}

				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].hc_sco_handle = (UINT8)(((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->connection_handle);
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].sco_handle    = 0;
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_sco         = 0;
			}
			
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].sco_packet = GetDefaultSCOPacket(devExt, pConnectDevice);
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
				((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_sco = LMP_GetOffsetOfSCO(devExt, (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].sco_packet + 1) * 2, ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].D_sco);
			LMP_SCONeedUnsniff(devExt, pConnectDevice);
			
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].air_mode    = SCO_DEFAULT_AIRMODE;
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].pDevice     = pConnectDevice;
			((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6].change_flag = SCO_ADD_LINK;
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)((((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6) + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));

			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
				status = Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_SEND_SCO_LINK_REQ_PDU), BT_TASK_PRI_NORMAL, (PUINT8)(&pConnectDevice), sizeof(PCONNECT_DEVICE_T));
			else
				status = Send_SCOLinkReq_PDU(devExt, pConnectDevice, pdu.TransID);
			break;

		case BT_TASK_EVENT_HCI2LMP_REMOVE_SCO: /* 18 */
			pScoConnectDev = (PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice;
			if (pScoConnectDev == NULL || ((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 0 || pConnectDevice->lmp_features.byte1.sco_link == 0)
			{
				pScoConnectDev->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			pdu.contents[0] = (UINT8)(pScoConnectDev->connection_handle);
			for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
			{
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle;
				else
					pdu.contents[1] = ((PBT_LMP_T)devExt->pLmp)->sco[i].hc_sco_handle;
				if (pdu.contents[1] == pdu.contents[0])
				{
					if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice != pConnectDevice)
						i = ((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6;

					break;
				}
			}
			if (i >= (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6))
			{
				pScoConnectDev->link_type = BT_LINK_TYPE_SCO;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_SCO_REMOVED, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			((PBT_LMP_T)devExt->pLmp)->sco_indicator = (UINT8)(i + (((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0xfc));

			pdu.OpCode      = 44;
			pdu.contents[0] = ((PBT_LMP_T)devExt->pLmp)->sco[i].sco_handle;
			pdu.contents[1] = pScoConnectDev->current_reason_code;
			status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 3);
			if (NT_SUCCESS(status))
			{
				ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_REMOVE_SCO_LINK_REQ);
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Remove SCO Link Req, contents= %d %d\n", pdu.TransID, pConnectDevice->bd_addr[0], pdu.contents[0], pdu.contents[1]);
				
				if (pConnectDevice->lmp_timer_valid == 0)
					LMP_StartPDUTimer(devExt, pConnectDevice, PDU_TIMER_SENT_REMOVE_SCOLINK_REQ, LMP_MAX_RESPONSE_TIME);
			}
			
#ifndef BT_TESTMODE_SUPPORT
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
#endif
			

			pScoConnectDev = (PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice;
			if(pScoConnectDev != NULL)
			{
				if(pScoConnectDev->current_reason_code == TERMINATED_USER_ENDED)
					pScoConnectDev->current_reason_code = TERMINATED_LOCAL_HOST; /* test protocol 5.7.17.11(close link) */
			}
			break;
			
		case BT_TASK_EVENT_HCI2LMP_CHANGE_SCO_PACKET_TYPE: /* 36 */
			status = Command_ChangeSCOPacket(devExt, pConnectDevice);
			break;

		case BT_TASK_EVENT_HCI2LMP_CLOCK_READY: /* 35 */
			if (((PBT_LMP_T)devExt->pLmp)->ClockReadyFlag == LMP_CLOCK_READY_FLAG_IDLE || ((PBT_LMP_T)devExt->pLmp)->ClockReadyFlag > LMP_CLOCK_READY_FLAG_MAX_VALUE)
				break;
			
			status = STATUS_SUCCESS;
			if ((((PBT_LMP_T)devExt->pLmp)->ClockReadyFlag & LMP_CLOCK_READY_FLAG_SCO) == LMP_CLOCK_READY_FLAG_SCO)
			{
				((PBT_LMP_T)devExt->pLmp)->ClockReadyFlag &= (~LMP_CLOCK_READY_FLAG_SCO);
				if (pConnectDevice->current_role == BT_ROLE_MASTER)
				{
					if (((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].clock_flag == 1) /* add sco */
						LMP_HC2LM_Command_Process(devExt, pConnectDevice, BT_TASK_EVENT_HCI2LMP_ADD_SCO);
					else
						status = STATUS_UNSUCCESSFUL;
				}
			}
			break;
			
		case BT_TASK_EVENT_HCI2LMP_CHANGE_MAX_SLOT: /* 23 */
			status = Send_MaxSlot_PDU(devExt, pConnectDevice);
			status = Send_PreferredRate_PDU(devExt, pConnectDevice);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_EXIT_SNIFF_REQ: /* 21 */
			if (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_SNIFF)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			if ((pConnectDevice->lmp_ext_states & LMP_STATE_ANDVALUE) == LMP_SEND_UNSNIFF_REQ)
			{
				status = STATUS_SUCCESS;
				break;
			}
			
			for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
			{
				if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
					break;
			}
			if (i >= ((PBT_LMP_T)devExt->pLmp)->sniff_number)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}

			((PBT_LMP_T)devExt->pLmp)->sniff_index = i;
			status = Send_UnsniffReq_PDU(devExt, pConnectDevice);
			break;

		case BT_TASK_EVENT_HCI2LMP_SNIFF_REQ: /* 20 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.sniff_mode == 0 || pConnectDevice->lmp_features.byte0.sniff_mode == 0 || ((PBT_LMP_T)devExt->pLmp)->sniff_number >= SNIFF_MAX_NUMBER 
				|| (pConnectDevice->link_policy_settings & 0x0004) == 0 || pConnectDevice->link_type != BT_LINK_TYPE_ACL)
			{
				pConnectDevice->current_reason_code = UNSUPPORTED_FEATURE_OR_PARAMETER;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
		/*Jakio20090904	
#ifdef BT_SNIFF_ONLY_ONE_CONNECTION
			if (((PBT_HCI_T)devExt->pHci)->num_device_am + ((PBT_HCI_T)devExt->pHci)->num_device_slave > 1)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
#endif
		*/
#ifdef BT_EXIT_SNIFF_WHEN_EXIST_SLAVE
			if (pConnectDevice->current_role == BT_ROLE_MASTER && ((PBT_HCI_T)devExt->pHci)->num_device_slave > 0)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
#endif
#ifdef BT_SCO_SNIFF_BONDING_WHEN_MS
			if ((pConnectDevice->current_role == BT_ROLE_SLAVE && LMP_CheckMasterSCO(devExt) == 1) 
				|| (pConnectDevice->current_role == BT_ROLE_MASTER && LMP_CheckSlaveSCO(devExt) == 1))
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
#endif
			if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
			{
				if (pConnectDevice->sniff_mode_interval < pConnectDevice->sniff_min_interval || pConnectDevice->sniff_mode_interval > pConnectDevice->sniff_max_interval)
				{
					pConnectDevice->current_reason_code = UNSUPPORTED_FEATURE_OR_PARAMETER;
				}
				else
				{
					status = STATUS_SUCCESS;
					pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
				}
				
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			if (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_ACTIVE)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			CheckSniffPara(devExt, pConnectDevice);
			((PBT_LMP_T)devExt->pLmp)->sniff_index = ((PBT_LMP_T)devExt->pLmp)->sniff_number;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].offset    = 0;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].interval  = pConnectDevice->sniff_mode_interval;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].attempt   = pConnectDevice->sniff_attempt;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timeout   = pConnectDevice->sniff_timeout;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].pDevice   = pConnectDevice;
			((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].init_flag = SNIFF_INIT_FLAG;
			
			tempvalue = LMP_ChangeSniffParaForSCO(devExt, &(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index]));
			if (tempvalue == 2)
			{
				RtlZeroMemory(&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index]), sizeof(BT_SNIFF_T));
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			status = Send_SniffReq_PDU(devExt, pConnectDevice, pdu.TransID);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_HODE_REQ: /* 19 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte0.hold_mode == 0 || pConnectDevice->lmp_features.byte0.hold_mode == 0 || (pConnectDevice->link_policy_settings & 0x0002) == 0)
			{
				pConnectDevice->current_reason_code = UNSUPPORTED_FEATURE_OR_PARAMETER;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}

			if (pConnectDevice->current_role == BT_ROLE_MASTER && pConnectDevice->afh_mode == 1)
				Send_SetAFH_PDU(devExt, pConnectDevice, AFH_ENABLED, 1);

			status = Send_HoldReq_PDU(devExt, pConnectDevice, pConnectDevice->hold_mode_max_interval, pdu.TransID);
			break;

		case BT_TASK_EVENT_HCI2LMP_POWER_UP: /* 25 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.rssi == 0 || pConnectDevice->lmp_features.byte2.power_control == 0)
				break;
			
			status = STATUS_SUCCESS;
			if (pConnectDevice->rx_power != RX_POWER_MAX_FLAG)
			{
				pConnectDevice->rx_power = 0;
				status = Send_PowerControl_PDU(devExt, pConnectDevice, 1, pdu.TransID); /* increase power req PDU */
			}
			break;
			
		case BT_TASK_EVENT_HCI2LMP_POWER_DOWN: /* 26 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.rssi == 0 || pConnectDevice->lmp_features.byte2.power_control == 0)
				break;
			
			status = STATUS_SUCCESS;
			if (pConnectDevice->rx_power != RX_POWER_MIN_FLAG)
			{
				pConnectDevice->rx_power = 0;
				status = Send_PowerControl_PDU(devExt, pConnectDevice, 2, pdu.TransID); /* decrease power req PDU */
			}
			break;
			
		case BT_TASK_EVENT_HCI2LMP_QOS_SETUP: /* 33 */
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_QOS_REQ);
		case BT_TASK_EVENT_HCI2LMP_FLOW_SPECIFICATION: /* 34 */
			status = Send_QualityService_PDU(devExt, pConnectDevice, 40, 2, 1);
			break;

		case BT_TASK_EVENT_HCI2LMP_CHANGE_PACKET_TYPE: /* 37 */
			status = Send_MaxSlotReq_PDU(devExt, pConnectDevice, pConnectDevice->changed_max_slot);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_CHANGE_EDR_MODE: /* 38 */
			if (pConnectDevice->edr_change_flag == 1)
				break;
			
			pConnectDevice->edr_change_flag = 1;
			status = Send_PacketTypeTableReq_PDU(devExt, pConnectDevice, (pConnectDevice->edr_mode + 1) % 2);
			break;
			
		case BT_TASK_EVENT_HCI2LMP_REFRESH_ENCRYPTION_KEY: /* 41 */
			if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.pause_encryption == 0 || pConnectDevice->lmp_features.byte5.pause_encryption == 0)
			{
				pConnectDevice->current_reason_code = UNSUPPORTED_FEATURE_OR_PARAMETER;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			if (pConnectDevice->encryption_mode == BT_ENCRYPTION_DIABLE)
			{
				pConnectDevice->current_reason_code = BT_HCI_ERROR_COMMAND_DISALLOWED;
				Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_REFRESH_ENCRYPTION_KEY, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
				break;
			}
			
			pConnectDevice->pause_encryption_status = 2;
			status = Send_PauseEncryptionReq_PDU(devExt, pConnectDevice, pdu.TransID);
			break;
			
		default :
			BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----LMP_HC2LM_Command_Process function operates ERROR (BD0=0x%x), unknown HC2LM Command, devExt= 0x%p, pConnectDevice= 0x%p, Command= %d\n", pConnectDevice->bd_addr[0], devExt, pConnectDevice, Command);
			status = STATUS_SUCCESS;
			break;
	} /* end switch */

	if (!NT_SUCCESS(status))
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP----LMP_HC2LM_Command_Process function operates ERROR or operates an unexpected Command (BD0=0x%x), devExt= 0x%p, pConnectDevice= 0x%p, Command= %d\n", pConnectDevice->bd_addr[0], devExt, pConnectDevice, Command);

	return status;
}

/*************************************************************
 *   LMP_ProcessTxPDU
 *
 *   Descriptions:
 *      This function process LMPPDU frame.
 *
 *   Arguments:
 *      devExt:   IN, pointer to device extension.
 *      ListFlag: the type of the new fragment.
 *
 *   Return Value: 
 *    BT_FRAG_RESOURCE_ERROR, BT_FRAG_SUCCESS, BT_FRAG_LIST_NULL.
 *************************************************************/
UINT32 LMP_ProcessTxPDU(PBT_DEVICE_EXT devExt, UINT8 ListFlag)
{
	NTSTATUS status;
	KIRQL oldIrql;
	PBT_LMP_T  pLmp;
	PBT_FRAG_T pFrag;
	PPDU_TX_LIST_T ptmpdatablock, ptmpdatablock2;
	UINT8 ListFlag2, RemoveFlag;
	UINT32 returnvalue = BT_FRAG_SUCCESS;
	UINT32	count;
	if (devExt == NULL)
		return BT_FRAG_RESOURCE_ERROR;
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	pLmp = (PBT_LMP_T)devExt->pLmp;
	if ((pFrag == NULL) || (pLmp == NULL))
		return BT_FRAG_RESOURCE_ERROR;
	
	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	ptmpdatablock = (PPDU_TX_LIST_T)QueueGetHead(&pFrag->Pdu_UsedList);
	
	while (ptmpdatablock != NULL)
	{
		RemoveFlag = 0;
		Frag_GetQueueHead(devExt, ptmpdatablock->pConnectDevice, &ListFlag2);
		if (ListFlag2 == ListFlag)
		{
			KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
			
			status = BtProcessTxLMPDU(devExt, ptmpdatablock->pConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len);
			KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
			if (NT_SUCCESS(status))
			{
		#ifdef LMP_DEBUG_DUMP
		        Lmp_Debug_Dump(devExt, ptmpdatablock->pConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len, 1);
		#endif
				RemoveFlag = 1;
				returnvalue = BT_FRAG_SUCCESS;
			}
			else
			{
				returnvalue = BT_FRAG_RESOURCE_ERROR;
				break;
			}
		}
		
		if (RemoveFlag == 1)
		{
			LMP_PDUEnter_Task(devExt, ptmpdatablock->pConnectDevice, ptmpdatablock->data, ptmpdatablock->data_len);
			
			ptmpdatablock2 = ptmpdatablock;
			ptmpdatablock  = (PPDU_TX_LIST_T)QueueGetNext(&ptmpdatablock->Link);
			QueueRemoveEle(&pFrag->Pdu_UsedList, &ptmpdatablock2->Link);
			QueuePutTail(&pFrag->Pdu_FreeList, &ptmpdatablock2->Link);
			pFrag->FreePdu_Count++;

			QueueGetCount(&pFrag->Pdu_FreeList, count);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_ProcessTxPDU---LMP free list count: %d\n",count);
		}
		else
		{
			ptmpdatablock = (PPDU_TX_LIST_T)QueueGetNext(&ptmpdatablock->Link);

			QueueGetCount(&pFrag->Pdu_FreeList, count);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_ProcessTxPDU---LMP free list count: %d\n",count);
		}
	}

	
	
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
	return returnvalue;
}

/*************************************************************
 * LMP_CheckTxQueue
 *
 * Description:
 *    This routine check if there is pending PDU in queue, if have, handle one 
 *  every time when it is called. this routine is expected to replace the 
 *  LMP_ThreadRoutine. It is called in task thread.
 *
 * Arguments:
 *    devExt: IN, pointer to device extension.
 *
 * Return Value:
 *    BT_FRAG_RESOURCE_ERROR, BT_FRAG_SUCCESS, BT_FRAG_LIST_NULL.
 *************************************************************/
UINT32 LMP_CheckTxQueue(IN PBT_DEVICE_EXT devExt)
{
	UINT32 returnvalue = BT_FRAG_SUCCESS;
	UINT8 i;

	
	for(i = FRAG_MASTER_LIST; i <= FRAG_SNIFF_LIST; i++)
	{
		returnvalue = LMP_ProcessTxPDU(devExt, i);	
		if(returnvalue != BT_FRAG_SUCCESS)
			break;
	}
	return returnvalue;
}

/*************************************************************
* LMP_Init 
* 
* Description: 
*    This function intializes the LMP module.
*
* Arguments: 
*    devExt: IN, pointer to device extension.
*       
* Return Value: 
*    STATUS_SUCCESS:      LMP initialized succesfully.
*    STATUS_UNSUCCESSFUL: LMP initialized failed. 
**************************************************************/
NTSTATUS LMP_Init(PBT_DEVICE_EXT devExt)
{
	UINT32 i;
	PBT_LMP_T pLmp;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Init devExt = 0x%p\n", devExt);
	
	/* Alloc memory for LMP module */
	pLmp = (PBT_LMP_T)ExAllocatePool(sizeof(BT_LMP_T), GFP_KERNEL);
	if (pLmp == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "Allocate LMP memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Init Lmp = 0x%p, size = %d\n", pLmp, sizeof(BT_LMP_T));
	
	/* Save LMP module pointer into device extention context */
	devExt->pLmp = (PVOID)pLmp;
	
	/* Zero out the LMP module space */
	RtlZeroMemory(pLmp, sizeof(BT_LMP_T));
	
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.u_law == 1)
		pLmp->air_mode_feature |= 1;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.a_law == 1)
		pLmp->air_mode_feature |= 2;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.cvsd == 1)
		pLmp->air_mode_feature |= 4;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte2.trans_sco == 1)
		pLmp->air_mode_feature |= 8;
	
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.sco_link == 1)
		pLmp->packet_type_feature |= 0x0001;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.hv2 == 1)
		pLmp->packet_type_feature |= 0x0002;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte1.hv3 == 1)
		pLmp->packet_type_feature |= 0x0004;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte3.ext_sco_link == 1)
		pLmp->packet_type_feature |= 0x0008;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.ev4 == 1)
		pLmp->packet_type_feature |= 0x0010;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte4.ev5 == 1)
		pLmp->packet_type_feature |= 0x0020;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.enh_rate_esco_2 == 1)
		pLmp->packet_type_feature |= 0x0040;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.enh_rate_esco_3 == 1)
		pLmp->packet_type_feature |= 0x0080;
	if (((PBT_HCI_T)devExt->pHci)->lmp_features.byte5.slot3_enh_esco == 1)
		pLmp->packet_type_feature |= 0x0300;
	
	return (STATUS_SUCCESS);
}

/*************************************************************
 *   LMP_Release
 *
 *   Descriptions:
 *      Releases LMP module.
 *
 *   Arguments:
 *      devExt: IN, pointer to device extension.
 *
 *   Return Value: 
 *      STATUS_SUCCESS:      LMP released succesfully.
 *      STATUS_UNSUCCESSFUL: LMP released failed. 
 *************************************************************/
NTSTATUS LMP_Release(PBT_DEVICE_EXT devExt)
{
	PBT_LMP_T pLmp;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Release\n");
	
	/* Get pointer of the LMP module */
	pLmp = (PBT_LMP_T)devExt->pLmp;
	if (pLmp == NULL)
		return STATUS_UNSUCCESSFUL;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Release devExt = 0x%p, pLmp = 0x%p\n", devExt, pLmp);
	
	
	/*Free the LMP module memory */
	if (pLmp != NULL)
		ExFreePool(pLmp);
	devExt->pLmp = NULL;
	
	return (STATUS_SUCCESS);
}

/* used for ACL detaching exceptionally */
void LMP_ReleaseSniffMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 i, tempvalue;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (devExt == NULL || pConnectDevice == NULL || devExt->pLmp == NULL)
		return;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; )
	{
		if (((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice == pConnectDevice)
		{
			if (pConnectDevice->current_role == BT_ROLE_MASTER)
			{
				Frag_FlushFragAndBD(devExt, NULL, FRAG_SNIFF_LIST);
				tempvalue = 1;
			}
			else
			{
				tempvalue = (UINT8)(pConnectDevice->slave_index << 1);
			}
			((PBT_FRAG_T)devExt->pFrag)->list_stop_flag[FRAG_SNIFF_LIST] = 0;
			((PBT_FRAG_T)devExt->pFrag)->list_timer[FRAG_SNIFF_LIST]     = 0;
			
			pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_AMADDR, pBuf, 1, (tempvalue << 4) + pConnectDevice->am_addr);
			pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT);
			RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));

			DeleteSniffLink(devExt, pConnectDevice);
			continue;
		}
		
		i++;
	}
	
	if (pConnectDevice->lmp_timer_valid == 1 && (pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SNIFF_REQ || pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_UNSNIFF_REQ))
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	return;
}

void LMP_ResetMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	KIRQL oldIrql;
	UINT8 reg_offset;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;

	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		reg_offset = pConnectDevice->am_addr;
	}
	else
	{
		reg_offset = pConnectDevice->slave_index * 8;
	}
	
	pConnectDevice->lmp_states      = LMP_CONN_IDLE;
	pConnectDevice->lmp_ext_states  = LMP_CONN_IDLE;
	pConnectDevice->lmp_timer_valid = 0;
	RtlZeroMemory(&(pConnectDevice->lmp_features), sizeof(LMP_FEATURES_T));
	RtlZeroMemory(&(pConnectDevice->extended_lmp_features), sizeof(LMP_EXTEND_FEATURES_T));
	RtlZeroMemory(&(pConnectDevice->remote_extended_feature), 8);
	RtlZeroMemory(pConnectDevice->remote_name, BT_LOCAL_NAME_LENGTH);
	
	pConnectDevice->key_type_key             = 0xff;
	pConnectDevice->encryption_mode          = BT_ENCRYPTION_DIABLE;
	pConnectDevice->is_in_encryption_process = 0;
	pConnectDevice->pause_encryption_status  = 0;
	pConnectDevice->pause_command_flag       = 0;
	RtlZeroMemory(pConnectDevice->link_key, BT_LINK_KEY_LENGTH);
	RtlZeroMemory(pConnectDevice->pin_code, BT_LINK_KEY_LENGTH);
	
	pConnectDevice->rev_auto_rate = 0;
	pConnectDevice->max_slot = BT_MAX_SLOT_BY_FEATURE;
	pConnectDevice->tx_power = TX_POWER_DEFAULT;
	pConnectDevice->rx_power = 0;
	pConnectDevice->allow_role_switch   = 0;
	pConnectDevice->hold_mode_interval  = 0;
	pConnectDevice->sniff_mode_interval = 0;
	LMP_ReleaseSniffMembers(devExt, pConnectDevice);
	
	pConnectDevice->afh_mode                 = AFH_ENABLED;
	pConnectDevice->send_classification_flag = AFH_REPORTING_DISABLED;
	pConnectDevice->classification_interval  = 30; /* 30s */
	pConnectDevice->is_afh_sent_flag         = 0;
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ENCRYPTION_ENABLE + reg_offset, pBuf, 1, 0);
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_AFH_MODE, pBuf, 1, (~(0x1 << pConnectDevice->am_addr)));
	else
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_AFH_MODE + 1, pBuf, 1, (~(0x3 << (pConnectDevice->slave_index << 1))));
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_TRANSMITT_POWER_CONTROL, pBuf, 4, (~(0x11 << (reg_offset * 2))));
	pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_EDR_MODE, pBuf, 2, (~(1 << reg_offset)));
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
	
	return;
}

/* used for ACL detaching exceptionally */
void LMP_ReleaseSCOMembers(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	UINT8 i;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (devExt == NULL || pConnectDevice == NULL || devExt->pLmp == NULL)
		return;
	
	pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
	((PBT_LMP_T)devExt->pLmp)->sco_counter = 0;
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); )
	{
		if (((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice == pConnectDevice)
		{
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0xFF);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);

			DeleteSCOLink(devExt, i);
			continue;
		}
		
		i++;
	}
	
	((PBT_LMP_T)devExt->pLmp)->esco_counter = 0;
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); )
	{
		if (((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice == pConnectDevice)
		{
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_WESCO, pBuf, 1, 0);
			pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_DISCONNECT_SCO_BIT);

			DeleteESCOLink(devExt, i);
			continue;
		}
		
		i++;
	}
	
	RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));

	if (pConnectDevice->lmp_timer_valid == 1 && (pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_SCOLINK_REQ || pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_SCOLINK_REQ 
		|| pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_ESCOLINK_REQ || pConnectDevice->lmp_timer_type == PDU_TIMER_SENT_REMOVE_ESCOLINK_REQ))
		LMP_StopPDUTimer(devExt, pConnectDevice);
	
	return;
}

/* return value: 0(have no master sco); 1(have master sco) */
UINT8 LMP_CheckMasterSCO(PBT_DEVICE_EXT devExt)
{
	UINT8 i;
	PCONNECT_DEVICE_T pDevice;
	
	if (devExt == NULL || devExt->pLmp == NULL)
		return 0;
	
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		pDevice = ((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice;
		if (pDevice != NULL && pDevice->current_role == BT_ROLE_MASTER)
			return 1;
		
		pDevice = ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice;
		if (pDevice != NULL && pDevice->current_role == BT_ROLE_MASTER)
			return 1;
	}
	
	return 0;
}

/* return value: 0(have no slave sco); 1(have slave sco) */
UINT8 LMP_CheckSlaveSCO(PBT_DEVICE_EXT devExt)
{
	UINT8 i;
	PCONNECT_DEVICE_T pDevice;
	
	if (devExt == NULL || devExt->pLmp == NULL)
		return 0;
	
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		pDevice = ((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice;
		if (pDevice != NULL && pDevice->current_role == BT_ROLE_SLAVE)
			return 1;
		
		pDevice = ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice;
		if (pDevice != NULL && pDevice->current_role == BT_ROLE_SLAVE)
			return 1;
	}
	
	return 0;
}

/* return value: 0(no change); 1(change); 2(no supported) */
UINT8 LMP_ChangeSniffParaForSCO(PBT_DEVICE_EXT devExt, PBT_SNIFF_T pSniff)
{
	UINT8 i, interval;
	PCONNECT_DEVICE_T pConnectDevice;
	
	if (devExt == NULL || pSniff == NULL)
		return 0;
	
	if (pSniff->change_for_sco == 1)
		return 0;
	
	for (i = 0; i < (((PBT_LMP_T)devExt->pLmp)->sco_indicator >> 6); i++)
	{
		pConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sco[i].pDevice;
		if (pConnectDevice != NULL)
		{
			if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV1)
				return 2;
			
			if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV2)
			{
				/* if (pSniff->interval % 4 != 0)
				{
					pSniff->change_for_sco = 1;
					pSniff->interval = ((pSniff->interval >> 2) << 2) + 4;
					if (pSniff->interval > pSniff->pDevice->sniff_max_interval)
						pSniff->interval -= 4;
				} */
				
				if (pSniff->interval % 4 == 0 && (pSniff->offset % 4) == ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco)
				{
					pSniff->change_for_sco = 1;
					pSniff->offset = (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco + 2) % 4;
				}
			}
			else if (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->current_packet_type == BT_SCO_PACKET_HV3)
			{
				/* if (pSniff->interval % 6 != 0)
				{
					pSniff->change_for_sco = 1;
					pSniff->interval = ((pSniff->interval / 6) * 6) + 6;
					if (pSniff->interval > pSniff->pDevice->sniff_max_interval)
						pSniff->interval -= 6;
				} */
				
				if (pSniff->interval % 6 == 0 && (pSniff->offset % 6) == ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco)
				{
					pSniff->change_for_sco = 1;
					pSniff->offset = (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco + 4) % 6;
				}
			}
		}
		else
		{
			pConnectDevice = ((PBT_LMP_T)devExt->pLmp)->esco[i].pDevice;
			interval = ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->transmission_interval;
			/* if (pSniff->interval % interval != 0)
			{
				pSniff->change_for_sco = 1;
				pSniff->interval = ((pSniff->interval / interval) * interval) + interval;
				if (pSniff->interval > pSniff->pDevice->sniff_max_interval)
					pSniff->interval -= interval;
			} */
			
			if (pSniff->interval % interval == 0 && (pSniff->offset % interval) == ((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco)
			{
				pSniff->change_for_sco = 1;
				pSniff->offset = (((PSCO_CONNECT_DEVICE_T)pConnectDevice->pScoConnectDevice)->D_sco + 4) % interval;
			}
		}
		
		break;
	}
	
	if (pSniff->change_for_sco == 1)
		return 1;
	else
		return 0;
}

UINT8 LMP_GetOffsetOfSCO(PBT_DEVICE_EXT devExt, UINT8 interval, UINT8 offset)
{
	UINT8 i;
	PBT_SNIFF_T pSniff;
	
	if (devExt == NULL || devExt->pLmp == NULL)
		return offset;
	
	if (interval == 2) /* HV1 */
		return offset;
	
	for (i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
	{
		pSniff = &(((PBT_LMP_T)devExt->pLmp)->sniff[i]);
		if (pSniff->interval % interval == 0 && (pSniff->offset % interval) == offset)
		{
			if (interval == 4) /* HV2 */
				offset = (offset + 2) % interval;
			else if (interval == 6) /* HV3, EV3 */
				offset = (offset + 4) % interval;

			break;
		}
	}
	
	return offset;
}

void LMP_SCONeedUnsniff(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pVoidPtr)
{
	UINT8 i;
	PBT_HCI_T	pHci;
	
	if (devExt == NULL|| devExt->pLmp == NULL)
		return;

	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
		return;

	if(pHci->lmp_features.byte0.sniff_mode == 1)
	{
		PCONNECT_DEVICE_T	pTempConnectDevice = NULL;
		for(i = 0; i < ((PBT_LMP_T)devExt->pLmp)->sniff_number; i++)
		{
			pTempConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice;
			if((pTempConnectDevice != NULL) && (pTempConnectDevice->manufacturer_name != 56))
			{
				BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_SCONeedUnsniff----unsniff with device: [%x]\n", pTempConnectDevice->bd_addr[0]);
				((PBT_LMP_T)devExt->pLmp)->sniff_index = i;
				Send_UnsniffReq_PDU(devExt, pTempConnectDevice);	
			}
			
		}	
	}	
	
	return;
}



void LMP_Unsniff_SCO(PBT_DEVICE_EXT devExt)
{
	UINT8 i;
	PCONNECT_DEVICE_T pTempConnectDevice;
	PBT_LMP_T	pLmp = NULL;
	
	if (devExt == NULL)
		return;

	pLmp = (PBT_LMP_T)devExt->pLmp;
	if(pLmp == NULL)
		return;

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Unsniff_SCO entered\n");

	
	for (i = 0; i < pLmp->sniff_number; i++)
	{
		pTempConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sniff[i].pDevice;
		if (pTempConnectDevice != NULL && pTempConnectDevice->pScoConnectDevice != NULL)
		{
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Unsniff_SCO----Device: [%x] in sniff state, unsniff it\n", pTempConnectDevice->bd_addr[0]);
			pLmp->sniff_index = i;
			Send_UnsniffReq_PDU(devExt, pTempConnectDevice);	
		}
	}
	
	return;
}

UINT8 LMP_NeedClockReady(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T *ppConnectDevice)
{
	PBT_LMP_T pLmp;
	UINT8 returnvalue = 0;
	
	if (devExt == NULL || ppConnectDevice == NULL)
		return 0;
	
	/* Get pointer of the LMP module */
	pLmp = (PBT_LMP_T)devExt->pLmp;
	if (pLmp == NULL || pLmp->ClockReadyFlag == LMP_CLOCK_READY_FLAG_IDLE)
		return 0;

	/* now this function isn't used */
	return 0;
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----LMP_NeedClockReady function: ClockReadyFlag= 0x%x\n", pLmp->ClockReadyFlag);
	
	if ((pLmp->ClockReadyFlag & LMP_CLOCK_READY_FLAG_SCO) == LMP_CLOCK_READY_FLAG_SCO)
	{
		*ppConnectDevice = ((PBT_LMP_T)devExt->pLmp)->sco[((PBT_LMP_T)devExt->pLmp)->sco_indicator & 0x11].pDevice;
		returnvalue = 1;
	}
	else if ((pLmp->ClockReadyFlag & LMP_CLOCK_READY_FLAG_ESCO) == LMP_CLOCK_READY_FLAG_ESCO)
	{
		returnvalue = 1;
	}
	
	return returnvalue;
}

void LMP_Task_EnterSniff(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	PCONNECT_DEVICE_T pConnectDevice;
	UINT8 ListFlag, role_flag, temp_value;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	PBT_HCI_T	pHci = NULL;
	PBT_FRAG_T	pFrag = NULL;
	
	if (devExt == NULL || para == NULL)
		return;
	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL2, "LMP_Task_EnterSniff---Null HCI module pointer\n");
		return;
	}
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL2, "LMP_Task_EnterSniff---Null Frag module pointer\n");
		return;
	}
	
	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if ((pConnectDevice == NULL) || (pConnectDevice->valid_flag == 0) || (pConnectDevice->is_in_disconnecting == 1))
	{
		BT_DBGEXT(ZONE_LMP | LEVEL2, "LMP_Task_EnterSniff----Connect device not valid, start tx queue\n");
		if(pConnectDevice != NULL)
		{
			DeleteSniffLink(devExt, pConnectDevice);
		}
		{
			pFrag->list_stop_flag[FRAG_MASTER_LIST] = 0;
			pFrag->list_stop_flag[FRAG_SNIFF_LIST] = 0;
		}
		return;
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_EnterSniff enter! BD0=0x%x\n", pConnectDevice->bd_addr[0]);
	
	if (pConnectDevice->mode_current_mode == BT_MODE_CURRENT_MODE_SNIFF)
		return;

	pConnectDevice->Sniff_RetryCount++;
	if(pConnectDevice->Sniff_RetryCount < 2)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_EnterSniff---First time, retry it later\n");
		VendorCmdWriteCmdToMailBox(devExt, NULL, MAILBOX_CMD_FLUSH_TX_FIFO_SNIFF);

		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_ENTER_SNIFF), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return;
	}
	
	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if (pConnectDevice->current_role == BT_ROLE_SLAVE || Frag_IsBDEmpty(devExt, ListFlag))
	{
		pConnectDevice->Sniff_RetryCount = 0;
		pConnectDevice->sniff_mode_interval = ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].interval;
		pConnectDevice->mode_current_mode   = BT_MODE_CURRENT_MODE_SNIFF;
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			Frag_MoveFrag(devExt, pConnectDevice, &((PBT_FRAG_T)devExt->pFrag)->Sniff_FragList, &((PBT_FRAG_T)devExt->pFrag)->Master_FragList);
#ifdef BT_FORCE_MASTER_SNIFF_ONE_SLOT
			Frag_RebuildFrag(devExt, pConnectDevice);
#endif
			role_flag = 1;
		}
		else
		{
			role_flag = (UINT8)(pConnectDevice->slave_index << 1);
		}
		
		temp_value = ((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timing_control_flags;
		temp_value = (UINT8)(((((temp_value >> 1) & 0x01) + 1) << 6) + (role_flag << 4) + pConnectDevice->am_addr);
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_AMADDR, pBuf, 1, &temp_value);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_D_SNIFF, pBuf, 2, (PUINT8)&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].offset));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_T_SNIFF, pBuf, 2, (PUINT8)&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].interval));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_ATTEMPT, pBuf, 2, (PUINT8)&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].attempt));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_TIMEOUT, pBuf, 2, (PUINT8)&(((PBT_LMP_T)devExt->pLmp)->sniff[((PBT_LMP_T)devExt->pLmp)->sniff_index].timeout));
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SNIFF_MODE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		((PBT_FRAG_T)devExt->pFrag)->list_stop_flag[ListFlag] = 0;
		((PBT_FRAG_T)devExt->pFrag)->list_timer[ListFlag]     = 0;
		
		pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
		Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
	}
	else
	{
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_ENTER_SNIFF), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
	}
	
	return;
}

void LMP_Task_LeaveSniff(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	PCONNECT_DEVICE_T pConnectDevice, ptmpConnectDevice;
	UINT8 ListFlag, role_flag;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	PBT_HCI_T	pHci = NULL;
	PBT_FRAG_T	pFrag = NULL;

	if (devExt == NULL || para == NULL)
		return;

	pHci = (PBT_HCI_T)devExt->pHci;
	if(pHci == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP_Task_LeaveSniff---Null HCI module pointer\n");
		return;
	}

	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP_Task_LeaveSniff---Null Frag module pointer\n");
		return;
	}
	
	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pConnectDevice == NULL || pConnectDevice->valid_flag == 0)
	{
		return;
	}
		
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_LeaveSniff enter! BD0=0x%x\n", pConnectDevice->bd_addr[0]);

	ptmpConnectDevice = Hci_Find_Connect_Device_By_AMAddr(pHci, pConnectDevice->am_addr);
	if(ptmpConnectDevice != pConnectDevice)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL1, "LMP_Task_LeaveSniff----Connect device not valid, start tx queue\n");
		if(pFrag->list_stop_flag[FRAG_SNIFF_LIST] == LIST_STOP_FLAG)
		{
			pFrag->list_stop_flag[FRAG_MASTER_LIST] = 0;
			pFrag->list_stop_flag[FRAG_SNIFF_LIST] = 0;
		}
		return;
	}

	if (pConnectDevice->mode_current_mode != BT_MODE_CURRENT_MODE_SNIFF)
		return;


	pConnectDevice->Sniff_RetryCount++;
	if(pConnectDevice->Sniff_RetryCount < 5)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_LeaveSniff---%d time, retry it later\n", pConnectDevice->Sniff_RetryCount);
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_LEAVE_SNIFF), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
		return;
	}

	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if (pConnectDevice->current_role == BT_ROLE_SLAVE || Frag_IsBDEmpty(devExt, ListFlag))
	{
    	//Jakio20090227: reset retry count
    	pConnectDevice->Sniff_RetryCount = 0;
		DeleteSniffLink(devExt, pConnectDevice);
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
		{
			Frag_MoveFrag(devExt, pConnectDevice, &((PBT_FRAG_T)devExt->pFrag)->Master_FragList, &((PBT_FRAG_T)devExt->pFrag)->Sniff_FragList);
			role_flag = 1;
		}
		else
		{
			role_flag = (UINT8)(pConnectDevice->slave_index << 1);
		}
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SNIFF_AMADDR, pBuf, 1, (role_flag << 4) + pConnectDevice->am_addr);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_EXIT_SNIFF_MODE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		((PBT_FRAG_T)devExt->pFrag)->list_stop_flag[ListFlag] = 0;
		((PBT_FRAG_T)devExt->pFrag)->list_timer[ListFlag]     = 0;

		pConnectDevice->mode_Sniff_debug1 = 0;
		
		pConnectDevice->mode_current_mode = BT_MODE_CURRENT_MODE_ACTIVE;
		if ((pConnectDevice->lmp_ext_states & LMP_SEND_UNSNIFF_DETACH) == LMP_SEND_UNSNIFF_DETACH)
		{
			CancelLmpBitState(devExt, pConnectDevice, LMP_SEND_UNSNIFF_DETACH, 1);
			ResLmpDetach(devExt, pConnectDevice, pConnectDevice->current_reason_code);
		}
		else
		{
			pConnectDevice->sniff_mode_interval = 0;
			pConnectDevice->current_reason_code = BT_HCI_STATUS_SUCCESS;
			Hci_Receive_From_LMP(devExt, BT_LMP_EVENT_MODE_CHANGE, (PUINT8)&pConnectDevice, sizeof(PCONNECT_DEVICE_T));
		}
	}
	else
	{
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_LEAVE_SNIFF), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
	}
	
	return;
}

void LMP_Task_EdrModeChange(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	PCONNECT_DEVICE_T pConnectDevice;
	UINT16 temp_value;
	UINT8 ListFlag, reg_offset;
	BOOLEAN		ret =FALSE;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], *pBuf;
	
	if (devExt == NULL || para == NULL)
		return;
	
	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pConnectDevice == NULL || pConnectDevice->valid_flag == 0)
		return;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_EdrModeChange enter! BD0=0x%x, EdrMode=%d\n", pConnectDevice->bd_addr[0], pConnectDevice->edr_mode);
	
	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if(ListFlag == FRAG_MASTER_LIST)
		ret = BtIsMasterBDEmpty(devExt);
	else if(ListFlag == FRAG_SLAVE_LIST)
		ret = BtIsSlaveBDEmpty(devExt);
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL0, "Error, wrong queue\n");
	}
	if(ret)
	{
		Frag_SetConnectionFeatureBit(devExt, pConnectDevice);
		Frag_GetCurrentPacketType(devExt, pConnectDevice);
		Frag_RebuildFrag(devExt, pConnectDevice);
		
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			reg_offset = pConnectDevice->am_addr;
		else
			reg_offset = pConnectDevice->slave_index * 8;

		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_AND_OPERATION, BT_REG_EDR_MODE, pBuf, 2, (~(1 << reg_offset)));
		pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_OR_OPERATION, BT_REG_EDR_MODE, pBuf, 2, (pConnectDevice->edr_mode << reg_offset));
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));


		/*
		temp_value = Hci_Read_Word_From_3DspReg(devExt,BT_REG_EDR_MODE,0);
		temp_value = temp_value & (~(1 << reg_offset)) | (pConnectDevice->edr_mode << reg_offset);
		Hci_Write_Word_To_3DspReg(devExt,BT_REG_EDR_MODE, 0,temp_value);
		*/

		
		Hci_Write_Command_Indicator(devExt, BT_HCI_COMMAND_INDICATOR_PACKET_TYPE_EDR_MODE);
		
		Frag_ListStopSwitch(devExt, pConnectDevice, 0);
		pConnectDevice->edr_change_flag = 0;
		Send_PreferredRate_PDU(devExt, pConnectDevice);

		UsbQueryDMASpace(devExt);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "bd is not empty\n");
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_NORMAL_EDR_MODE_CHANGE), BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
	}
	
	return;
}





/***************************************************************************************
*Routine:
*	LMP_Task_RoleChange_Send
*Description:
*	This routine is used to check if it is time to send switch role request. If the DSP buffer is empty, it means
*	we can do it right now, otherwise, we should wait a while to check the DSP buffer again.
*Parameter:
*	devExt------Module pointer
*	para--------connect device pointer
*Changelog:
*	Jakio 20081024 add this routine
***************************************************************************************/
void LMP_Task_RoleChange_Send(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	PCONNECT_DEVICE_T pConnectDevice;
	NTSTATUS	status;
	UINT8		ListFlag;
	UINT32		SwitchInstant, SwitchInstant2;
	LMP_PUD_PACKAGE_T		pdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], * pBuf;		
	

	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if ((pConnectDevice == NULL) || (pConnectDevice->valid_flag == 0))
	{
		if(pConnectDevice != NULL)
		{
			Frag_ListStopSwitch(devExt, pConnectDevice, 0);
		}	
		return;
	}

	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if(Frag_IsBDEmpty(devExt, ListFlag))
	{
		Frag_StartQueue(devExt, ListFlag);

		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			if (pConnectDevice->slave_index == 0)
				SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE0);
			else
				SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO_SLAVE1);
		}
		else
		{
			SwitchInstant = Usb_Read_4Bytes_From_3DspReg(devExt, BT_REG_CLOCK_INFO);
		}
		if ((devExt->ComboState == FW_WORK_MODE_COMBO) && (devExt->chipID != 0)) /* Combo mode */
			SwitchInstant = ((SwitchInstant >> 2) << 1) + 100 * MAX_SWITCH_INSTANT_OFFSET;
		else
			SwitchInstant = ((SwitchInstant >> 2) << 1) + MAX_SWITCH_INSTANT_OFFSET;
		SwitchInstant2 = SwitchInstant & 0x07FFFFFF;
		
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID = (pConnectDevice->current_role == BT_ROLE_MASTER) ? MASTER_TRANSACTION_ID : SLAVE_TRANSACTION_ID;
		pdu.OpCode  = 19;
		RtlCopyMemory(pdu.contents, &SwitchInstant2, 4);
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			BtWriteFHSPacket(devExt, pConnectDevice);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (pConnectDevice->slave_index << 31));
		}
		else
		{
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_SWITCH_SLOT_OFFSET, pBuf, 2, 0);
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, ((SwitchInstant << 1) & 0x3FFFFFFF) + (1 << 30));
		}
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 5);
		if (NT_SUCCESS(status))
		{
			ChangeLmpExtraState(devExt, pConnectDevice, LMP_SEND_SWITCH_REQ);
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Switch Req, SwitchInstant= 0x%x\n", pdu.TransID, pConnectDevice->bd_addr[0], SwitchInstant2);
		}
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_RoleChange_Send---BD is not empty\n");
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_SEND), 
					BT_TASK_PRI_NORMAL, para, sizeof(PCONNECT_DEVICE_T));
	}
}



/***************************************************************************************
*Routine:
*	LMP_Task_RoleChange_Recv
*Description:
*	This routine is used to check if it is time to response a device's switch role request. If the DSP buffer is 
*	empty, it means we can do it right now, otherwise, we should wait a while to check the DSP buffer again.
*Parameter:
*	devExt------Module pointer
*	para--------connect device pointer and a lmp pdu buffer
*Changelog:
*	Jakio 20081024 add this routine
***************************************************************************************/
void LMP_Task_RoleChange_Recv(PBT_DEVICE_EXT devExt, PUINT8 para, UINT32 Length)
{
	PCONNECT_DEVICE_T pConnectDevice;
	NTSTATUS	status;
	UINT8		ListFlag;
	UINT32		SwitchInstant;
	LMP_PUD_PACKAGE_T 	pdu;
	PLMP_PUD_PACKAGE_T	pRecvPdu;
	UINT8 buf[REGAPI_MAX_BUFFER_LENGTH], * pBuf;

	if(Length <= sizeof(PCONNECT_DEVICE_T))
	{
		return;
	}
	
	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if ((pConnectDevice == NULL) || (pConnectDevice->valid_flag == 0))
	{
		if(pConnectDevice != NULL)
		{
			Frag_ListStopSwitch(devExt, pConnectDevice, 0);
		}	
		return;
	}

	pRecvPdu = (PLMP_PUD_PACKAGE_T)(para + sizeof(PCONNECT_DEVICE_T));
	RtlCopyMemory(&SwitchInstant, pRecvPdu->contents, 4);
	
	Frag_GetQueueHead(devExt, pConnectDevice, &ListFlag);
	if(Frag_IsBDEmpty(devExt, ListFlag))
	{
		Frag_StartQueue(devExt, ListFlag);
		
		((PBT_HCI_T)devExt->pHci)->role_switching_flag = 1;
		pConnectDevice->role_switching_flag = 1;
		if (pConnectDevice->current_role == BT_ROLE_SLAVE)
		{
			status = Send_SlotOffset_PDU(devExt, pConnectDevice, pRecvPdu->TransID);
			pConnectDevice->role_switching_role = BT_ROLE_MASTER;
			BtWriteFHSPacket(devExt, pConnectDevice);
		}
		else
		{
			pConnectDevice->role_switching_role = BT_ROLE_SLAVE;
		}
		
		pBuf = BTUSB_REGAPI_FIRST_ELE(buf);
		if (pConnectDevice->current_role == BT_ROLE_MASTER)
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (1 << 30));
		else
			pBuf = RegApi_Fill_Cmd_Body_Constant(devExt, REGAPI_REG_SET_OPERATION, BT_REG_ROLE_SWITCH_INSTANT, pBuf, 4, (SwitchInstant << 1) + (pConnectDevice->slave_index << 31));

		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Fill_Cmd_Body(devExt, REGAPI_REG_SET_OPERATION, BT_REG_OP_BD_ADDR_DSP, pBuf, BT_BD_ADDR_LENGTH, pConnectDevice->bd_addr);
		pBuf = RegApi_Write_Cmd_Indicator(devExt, pBuf, BT_HCI_COMMAND_INDICATOR_SWITCH_ROLE_BIT);
		RegAPI_SendTo_MailBox(devExt, buf, (UINT32)(pBuf - buf));
		
		RtlZeroMemory(&pdu, sizeof(LMP_PUD_PACKAGE_T));
		pdu.TransID     = pRecvPdu->TransID;
		pdu.OpCode      = 3;
		pdu.contents[0] = pRecvPdu->OpCode;
		status = LMP_SendPDUToLC(devExt, pConnectDevice, &pdu, 2);
		if (NT_SUCCESS(status))
			BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP----PDU Inserted Tx list (TransID=%d, BD0=0x%x): Accepted Switch Req\n", pdu.TransID, pConnectDevice->bd_addr[0]);
	}
	else
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_RoleChange_Send---BD is not empty\n");
		UsbQueryDMASpace(devExt);
		Task_CreateDelayTask((PBT_TASK_T)devExt->pTask, BT_TASK_NORMAL_EVENT(BT_TASK_EVENT_PROCESSING_ROLE_CHANGE_RECV), 
					BT_TASK_PRI_NORMAL, para, Length);
	}
	
}







void LMP_Task_ResumeEncryption(PBT_DEVICE_EXT devExt, PUINT8 para)
{
	KIRQL oldIrql;
	PCONNECT_DEVICE_T pConnectDevice;
	
	if (devExt == NULL || para == NULL)
		return;
	
	RtlCopyMemory(&pConnectDevice, para, sizeof(PCONNECT_DEVICE_T));
	if (pConnectDevice == NULL || pConnectDevice->valid_flag == 0)
		return;
	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Task_ResumeEncryption enter! BD0=0x%x\n", pConnectDevice->bd_addr[0]);
	
	if (pConnectDevice->current_role == BT_ROLE_MASTER)
	{
		pConnectDevice->encryption_mode = BT_ENCRYPTION_ONLY_P2P;
		Send_StartEnReq_PDU(devExt, pConnectDevice, MASTER_TRANSACTION_ID);
	}
	else
	{
		Send_ResumeEncryptionReq_PDU(devExt, pConnectDevice, SLAVE_TRANSACTION_ID);
	}
	
	return;
}





void LMP_Reset_AfhLmpCount(PBT_DEVICE_EXT devExt)
{
	KIRQL oldIrql;
	PBT_LMP_T	pLmp;
	PBT_FRAG_T	pFrag;
	PPDU_TX_LIST_T	pPduListNode = NULL;
	PLMP_PUD_PACKAGE_T	pLmpPdu = NULL;
	UINT32		CurrentAfhCount = 0;
	UINT32		LmpPduCount;
	pLmp = (PBT_LMP_T)devExt->pLmp;
	if(pLmp == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Reset_AfhLmpcount---Error, LMP module pointer is NULL\n");
		return;
	}
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Reset_AfhLmpcount---Error, FRAG module pointer is NULL\n");
		return;
	}

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	LmpPduCount = MAX_PDU_TX_QUEUE - pFrag->FreePdu_Count;
	pPduListNode = (PPDU_TX_LIST_T)QueueGetHead(&pFrag->Pdu_UsedList);
	while(pPduListNode && LmpPduCount)
	{
		pLmpPdu = (PLMP_PUD_PACKAGE_T)pPduListNode->data;
		if (pLmpPdu->OpCode == 60)
		{
			CurrentAfhCount++;
		}
		LmpPduCount--;
		pPduListNode = (PPDU_TX_LIST_T)QueueGetNext(&pPduListNode->Link);
	}
	BT_DBGEXT(ZONE_LMP | LEVEL3, "Afh Pdu count in lmp list is: %d\n", CurrentAfhCount);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "Afh Pdu record count is: %d\n", devExt->AfhPduCount);
	devExt->AfhPduCount = CurrentAfhCount;
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
}


void LMP_Clear_LmpPdu(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnDev)
{
	PBT_LMP_T	pLmp;
	KIRQL oldIrql;
	PBT_FRAG_T	pFrag;
	PPDU_TX_LIST_T	pNode = NULL;
	PPDU_TX_LIST_T	pTmpNode = NULL;
	PLMP_PUD_PACKAGE_T	pLmpPdu = NULL;
	UINT32		LmpPduCount = 0;

	BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Clear_LmpPdu entered\n");
	if(pConnDev == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Clear_LmpPdu---Null pointer\n");
		return;
	}

	pLmp = (PBT_LMP_T)devExt->pLmp;
	if(pLmp == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Clear_LmpPdu---Error, LMP module pointer is NULL\n");
		return;
	}
	pFrag = (PBT_FRAG_T)devExt->pFrag;
	if(pFrag == NULL)
	{
		BT_DBGEXT(ZONE_LMP | LEVEL3, "LMP_Clear_LmpPdu---Error, FRAG module pointer is NULL\n");
		return;
	}

	KeAcquireSpinLock(&pFrag->FragTxLock, &oldIrql);
	BT_DBGEXT(ZONE_LMP | LEVEL3, "Lmp pdu count: %d\n", pFrag->FreePdu_Count);
	
	LmpPduCount = MAX_PDU_TX_QUEUE - pFrag->FreePdu_Count;
	pNode = (PPDU_TX_LIST_T)QueueGetHead(&pFrag->Pdu_UsedList);
	while(pNode && LmpPduCount)
	{
		pTmpNode = pNode;
		pNode = (PPDU_TX_LIST_T)QueueGetNext(&pNode->Link);
		if(pTmpNode->pConnectDevice == pConnDev)
		{
			pLmpPdu = (PLMP_PUD_PACKAGE_T)pTmpNode->data;
			BT_DBGEXT(ZONE_LMP | LEVEL3, "This Lmp pdu should be dropped\n");
			BT_DBGEXT(ZONE_LMP | LEVEL3, "Lmp Opcode is %d\n", pLmpPdu->OpCode);
			if((pLmpPdu->OpCode == 23) || (pLmpPdu->OpCode == 24))
			{
				{
					pFrag->list_stop_flag[FRAG_SNIFF_LIST] = 0;
					pFrag->list_stop_flag[FRAG_MASTER_LIST] = 0;
				}
			}
			QueueRemoveEle(&pFrag->Pdu_UsedList, &pTmpNode->Link);
			QueuePutTail(&pFrag->Pdu_FreeList, &pTmpNode->Link);
			pFrag->FreePdu_Count++;
		}
		LmpPduCount--;
		
	}
	
	BT_DBGEXT(ZONE_LMP | LEVEL3, "Lmp pdu count: %d\n", pFrag->FreePdu_Count);
	KeReleaseSpinLock(&pFrag->FragTxLock, oldIrql);
	
}





/*--end of file--------------------------------------------------------*/
