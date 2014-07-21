/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:
 *
 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************
 *
 * [usbwlan_main.h] description:
 *  
 * exports:
 *
 * make'ing:
 *
 * TODO:
 * 
 * see also:
 *
 * This source to be viewed with tabspace 4 (":se ts=4" for ex/vi)
 *
 ************************************************************************/
#ifndef _USBWLAN_MAIN_H_
#define _USBWLAN_MAIN_H_

#include "precomp.h"



int	Dsp_Initialize(void * adapt);
VOID Dsp_Halt( void* para );
TDSP_STATUS	Dsp_Reset(   BOOLEAN* AddressingReset, PDSP_ADAPTER_T pAdap);


TDSP_STATUS	Adap_Reset_Routine(PDSP_ADAPTER_T pAdap);
VOID 			Adap_InitReset(PDSP_ADAPTER_T pAdap);
VOID 			Dsp_UnInitialize(PVOID adapt);
VOID 			Dsp_Destrory(PVOID adapt);

VOID			Adap_SetResetType(PDSP_ADAPTER_T pAdap, UINT8 type,UINT32 sub_type);

BOOLEAN 		Adap_CheckOFDMSupport(PUINT8 sup_rate,UINT8 sup_rate_len,PUINT8 ext_sup_rate,UINT8 ext_sup_rate_len );

static inline TDSP_STATUS Adap_CheckBulkStall(PDSP_ADAPTER_T pAdap) 
{
	return STATUS_SUCCESS;
}

TDSP_STATUS 	Adap_SoftReset(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_HardwareReset(PDSP_ADAPTER_T pAdap, BOOLEAN sys_reset_flag);
VOID 			Adap_SysReset(PDSP_ADAPTER_T pAdap);
VOID 			Adap_InternalReset(PDSP_ADAPTER_T pAdap);

VOID 			Adap_SetConnection(PDSP_ADAPTER_T pAdap,UINT32 join);

static inline media_link_state_t Adap_GetLinkStatus(PDSP_ADAPTER_T pAdap)
{
	return pAdap->link_ok;
}

static inline VOID Adap_SetLink(PDSP_ADAPTER_T pAdap, UINT32 linkok)
{
	// Reset check media counter as zero.
	pAdap->check_media_counts = 0;
	pAdap->link_ok = linkok;
}

VOID 			Adap_CheckMediaState(PDSP_ADAPTER_T pAdap);
VOID 			Adap_UpdateMediaState(PDSP_ADAPTER_T pAdap,UINT32 state);


VOID 			Adap_Scan(PDSP_ADAPTER_T pAdap);
VOID 			Adap_BeaconChange(PDSP_ADAPTER_T pAdap);
VOID 			Adap_TbttUpdate(PDSP_ADAPTER_T pAdap);

UINT32 			Adap_StartTKIPCounterMeasure(PDSP_ADAPTER_T pAdap,COUNTER_MEASURE_REQUEST_TYPE cm_type,BOOLEAN is_group);
	
VOID 			Adap_PrintBuffer(PUINT8 buffer, UINT32 len);

TDSP_STATUS	Adap_set_auto_corr(PDSP_ADAPTER_T pAdap);

TDSP_STATUS 	Adap_set_state_control( PDSP_ADAPTER_T pAdap,UINT8 state, UINT8 scan_type, UINT16 time_value);

static inline UINT32 Adap_get_current_state_control( PDSP_ADAPTER_T pAdap)
{
	return VcmdR_3DSP_Dword(pAdap,WLS_MAC__STATE_CONTROL) & 0xf;
}

static inline UINT32 Adap_get_next_state_control( PDSP_ADAPTER_T pAdap)
{
	return (VcmdR_3DSP_Dword(pAdap,WLS_MAC__STATE_CONTROL) >> 4) & 0xf;
}


TDSP_STATUS 	Adap_set_scan_reg( PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_set_basicrate_map_ofdm11g( PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_set_channel( PDSP_ADAPTER_T pAdap,UINT8 chan);
static inline UINT8	Adap_get_channel( PDSP_ADAPTER_T pAdap)
{
	return (UINT8)(VcmdR_3DSP_Dword(pAdap,WLS_MAC__BBREG_12_15) & CUR_CHANNEL_NUM_BIT);
}

TDSP_STATUS 	Adap_set_join_para( PDSP_ADAPTER_T pAdap,PMNG_DES_T pdes);
void 			Adap_start_bss(PDSP_ADAPTER_T pAdap);
VOID 			Dsp_Suprise_Removed_Handler(PDSP_ADAPTER_T pAdap);

static inline UINT32 Adap_get_chipid(PDSP_ADAPTER_T pAdap)
{
	return VcmdR_3DSP_Dword(pAdap, WLS_CSR__CHIP_ID);
}

static inline TDSP_STATUS Adap_set_mac_ctlr_reg(PDSP_ADAPTER_T pAdap,UINT32 reg)
{
	sc_memory_copy(&pAdap->wlan_attr.gdevice_info.mac_ctl_reg,(PUINT8)&reg,sizeof(UINT32));
	return VcmdW_3DSP_Dword(pAdap, reg,WLS_MAC__CONTROL);
}


UINT32 Rts_running_mode(PDSP_ADAPTER_T pAdap);

static inline BOOLEAN Adap_Driver_isHalt(PDSP_ADAPTER_T pAdap)
{
	return (pAdap->driver_state == DSP_DRIVER_HALT
			|| pAdap->driver_state == DSP_SUPRISE_REMOVE);
}

static inline BOOLEAN Adap_Driver_isTxWork(PDSP_ADAPTER_T pAdap)
{
	if(pAdap->driver_state == DSP_DRIVER_WORK)
		return TRUE;
	else return FALSE;
}

static inline BOOLEAN Adap_Driver_isRxWork(PDSP_ADAPTER_T pAdap)
{
	if(pAdap->driver_state == DSP_DRIVER_WORK
		||pAdap->driver_state == DSP_STOP_TX
		||pAdap->driver_state == DSP_STOP_TX_WHILE_PWRMGR
		||pAdap->driver_state == DSP_SYS_RESET)
		return TRUE;
	else return FALSE;
}

static inline BOOLEAN Adap_Driver_isIntWork(PDSP_ADAPTER_T pAdap)
{
	if(pAdap->driver_state != DSP_DRIVER_HALT
		&& pAdap->driver_state != DSP_DRIVER_INIT
		&& pAdap->driver_state != DSP_SUPRISE_REMOVE
		&& pAdap->driver_state != DSP_POWER_MANAGER
	)
		return TRUE;
	else return FALSE;
}

static inline BOOLEAN Adap_Driver_isReady(PDSP_ADAPTER_T pAdap)
{
	return ((pAdap->driver_state == DSP_DRIVER_READY));
}

VOID Adap_SetPowerD3(PDSP_ADAPTER_T pAdap);
VOID Adap_SetPowerD0(PDSP_ADAPTER_T pAdap);
	
VOID 			Adap_Set_Driver_State(PDSP_ADAPTER_T pAdap,DSP_DRIVER_STATE_T state);

TDSP_STATUS 	Adap_update_slot_time(PDSP_ADAPTER_T pAdap,UINT8 newSlotTime);

TDSP_STATUS 	Adap_request_Set_Channel(PDSP_ADAPTER_T pAdap, UINT8  chan);

TDSP_STATUS	Adap_set_antenna( PDSP_ADAPTER_T pAdap,UINT8 antenna);

VOID 			Oid_SetAddWep(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len);
VOID 			Oid_SetRemoveWep(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len);
VOID 			Oid_SetAddKey(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len);
VOID 			Oid_SetRemoveKey(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len);
VOID 			Oid_Set_RTS(PDSP_ADAPTER_T pAdap);
VOID 			Oid_SetInfrastructureMode(PDSP_ADAPTER_T pAdap);
VOID 			Adap_GetRegionDomain(PDSP_ADAPTER_T pAdap, UINT8 by_io);

VOID            Adap_Print_USBReg(void * adapt);

TDSP_STATUS Adap_GetT8051Fw(PDSP_ADAPTER_T pAdap);
TDSP_STATUS Adap_GetDspFw(PDSP_ADAPTER_T pAdap);

#endif

