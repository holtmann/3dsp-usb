 /****************************************************************
  * Copyright(C) 2006 3DSP Corporation. All Rights Reserved.
  *
  * FILENAME:          CURRENT VERSION: 1.00.01
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
**************************************************************************
$RCSfile: usbwlan_main.c,v $ 
$Revision: 1.25 $ 
$Date: 2010/12/10 04:17:16 $
**************************************************************************/


#include "usbwlan_main.h"
//#include "tdsp_bus.h"

/*--file local constants and types-------------------------------------*/


static char* TDSP_FILE_INDICATOR="WMAIN";
TDSP_STATUS Adap_request_init_txfifo(PDSP_ADAPTER_T pAdap);

extern  channel_list_t static_axLookupFreqByChannel_BOnly[];


VOID 			Adap_Init_Workstate(PDSP_ADAPTER_T pAdap);
VOID 			Adap_Init_Wlan_Attr(PDSP_ATTR_T pAttr);
VOID 			Adap_Cal_Capability(PDSP_ATTR_T pAttr);

VOID 			Adap_PeriodicTimeout(PVOID param);
VOID			Adap_SysTimeout(PVOID param);
VOID			Adap_MngTimeout(PVOID param);
VOID 			Adap_CounterMeasureTimeout(PVOID param);


VOID			Adap_InitTKIPCounterMeasure(PDSP_ADAPTER_T pAdap);
VOID			Adap_Set1xAuthRequest(PDSP_ADAPTER_T pAdap, UINT32 Flags);
VOID			Adap_PrepareTransmit1xFrame(PDSP_ADAPTER_T pAdap);
VOID 			Adap_TKIPCounterMeasureTimeout(PDSP_ADAPTER_T pAdap);
TDSP_STATUS	Adap_Dspfw_Download(PDSP_ADAPTER_T pAdap,PUINT8 buf,UINT32   len,UINT16 offset,UINT8 file_id);
TDSP_STATUS	Adap_Dspfw_Download_Interface(PDSP_ADAPTER_T pAdap,PUINT8 buf,UINT32 total_len,UINT8 file_id);

UINT32 			fcs_generation(UINT8 *input_array, UINT32 num_of_bytes);
VOID			cts_frame_fcs_lookup_init(PDSP_ADAPTER_T pAdap);
TDSP_STATUS	Adap_mmacRestart_initial(PDSP_ADAPTER_T pAdap,UINT32 	channel);
TDSP_STATUS	Adap_mmacinit(PDSP_ADAPTER_T pAdap);
TDSP_STATUS	Adap_mmacV4init(PDSP_ADAPTER_T pAdap);
TDSP_STATUS	Adap_w2lan_init(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type,UINT32 mac_mode);
TDSP_STATUS 	Adap_w2lan_resetting(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_mac_init(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type,UINT32 mac_mode);
TDSP_STATUS 	Adap_WLSMAC_initialize(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type,UINT32  mac_mode);
TDSP_STATUS 	Adap_reset_dsp_chip(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_bb_dsp_init(PDSP_ADAPTER_T pAdap,dot11_type_t	dot11_type,UINT8 	slot_time);
UINT32 			Adap_build_mac_ctrl_reg(PDSP_ADAPTER_T pAdap,dot11_type_t  dot11_type,UINT32 mac_mode);
TDSP_STATUS 	Adap_mac_set_dot11type(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type,UINT32 mac_mode,BOOLEAN short_slot);
UINT32			Adap_get_basicrate_map( PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type);
void 			Adap_set_power_table(PDSP_ADAPTER_T pAdap,UINT8 channel);
TDSP_STATUS	Adap_set_antenna( PDSP_ADAPTER_T pAdap,UINT8 antenna);
TDSP_STATUS 	Adap_set_usepd( PDSP_ADAPTER_T pAdap,UINT8 use_pd);
TDSP_STATUS 	Adap_request_init_txfifo(PDSP_ADAPTER_T pAdap);
TDSP_STATUS	Adap_Get8051FwCodesAndDownload(PDSP_ADAPTER_T pAdap);

TDSP_STATUS 	Adap_add_key_into_mib(PDSP_ADAPTER_T pAdap,UINT8 key_id,
		UINT8 key_len,UINT8 *key,UINT8 *mac_addr,BOOLEAN multi,UINT32     cipher);
TDSP_STATUS 	Adap_push_key_into_hw(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_del_key_from_hw(PDSP_ADAPTER_T	pAdap,UINT32 index);
TDSP_STATUS Adap_key_mib_to_false(    PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_key_hw_to_false(PDSP_ADAPTER_T pAdap);
TDSP_STATUS 	Adap_mac_set_key(PDSP_ADAPTER_T pAdap,PVOID key_map_node);
VOID 			Adap_set_any_wep_prevent_tx_stop(PDSP_ADAPTER_T pAdap);

VOID 			Adap_Set_Driver_State_Force(PDSP_ADAPTER_T pAdap,DSP_DRIVER_STATE_T state);
VOID 			Adap_Set_Usbbus_Enable_Reset(PDSP_ADAPTER_T pAdap);
UINT32 			Adap_Get_8051FW_VERSION(PDSP_ADAPTER_T pAdap);
BOOLEAN 		Adap_check_rssi(PDSP_ADAPTER_T pAdap,PUINT32 new_reg);


TDSP_STATUS	Adap_ParseAndSetHotKey(PDSP_ADAPTER_T pAdap);






/*--file local macros--------------------------------------------------*/
	 
/*--file local variables-----------------------------------------------*/



/* Defines the invalid mac address. If a mac address equals to this address, the 
mac address is invalid. */
static const UINT8 DSP_INVALID_ADDR[] = {0,0,0,0,0,0};

/* Defines the firmware binary codes. We can insert the firmware binary codes
into the array. */
//UINT8 DSP_FIRMWARE_CODES[4][4096];	//justin: 080517.	no use


inline UINT8 Adap_Driver_isWork(void * adapt)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
        return 0;
    if((pAdap->driver_state == DSP_DRIVER_WORK) 
        || (pAdap->driver_state == DSP_STOP_TX))
        return 1;
    return 0;
}

/*******************************************************************
 *   Adap_GetFirmwareCodesAndDownload
 *   
 *   Descriptions:
 *      This function retrieve firmware codes and pass it to bootloader.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      STATUS_SUCCESS: return success.
 *      TDSP_STATUS_xxx: return unsuccessful.
 ******************************************************************/
TDSP_STATUS Adap_GetDspFwCodesAndDownload(PDSP_ADAPTER_T pAdap)
{
    NDIS_STATUS Status;
    UINT32 FileLength;
    //define for sp20.h file mode
#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE
    PSP20_CODE pCodeBuffer = (PSP20_CODE)Usb_SP20CodeWlanOnly;
    PSP20_CODE pCodeBufferTmp; // glen090415 Added for get DSP code abnormal handle
#endif
    UINT16 *psubSystemID;
    UINT16 BasePortStart=0;
    UINT16 BasePortEnd=3;
    UINT16 FlagFindDspCode=0;  // glen090415 Added for get DSP code abnormal handle
    //PUINT8 pbin;
    UINT32  CodeSize = 0;  // glen090415 Added for get DSP code abnormal handle
    PUINT8 CodeHead = NULL;   // glen090415 Added for get DSP code abnormal handle
    ULONG               AddrCode;  // glen090415 Added for get DSP code abnormal handle
    PSP20_BIN_CODE pBinFileBuffer;
    PSP20_BIN_CODE pBinFileBufferTmp;  // glen090415 Added for get DSP code abnormal handle    //

    Adap_ParseAndSetHotKey(pAdap);
	//
	Status = STATUS_SUCCESS;

	if(Vcmd_CardBusNICEnable(pAdap) != STATUS_SUCCESS)
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "Vcmd_CardBusNICEnable failed\n");
	}

	// 20081020 add by glen for support subsystemID extend defined by EEPROM data
	psubSystemID = (PUINT16)&pAdap->u_eeprom[EEPROM_OFFSET_SUBSYSTEMID];
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
	DBG_WLAN__INIT(LEVEL_TRACE, "PortStart=%d PortEnd=%d\n", BasePortStart, BasePortEnd);

#ifdef ANTENNA_DIVERSITY
	switch(*psubSystemID)
	{
	case 0x0100:
	case 0x0400:
	case 0x0500:
	case 0x0600:
	case 0x0A00:
	case 0x0D00:		//dongle mode.
		pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_DISABLE;
		break;
	case 0x0200:
	case 0x0300:
	case 0x0700:
	case 0x0800:
	case 0x0900:
	case 0x0E00:		//Minicard mode.
		pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_ENABLE;
		break;
	default:			//default minicard mode.
		if (((*psubSystemID) & 0x0F00) == 0x0100)
            	{
			pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_DISABLE;
            	}
		else if(((*psubSystemID) & 0x0F00) == 0x0200)
		{
			pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_ENABLE;
		}
		else
		{
			pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_ENABLE;
		}
		break;
	}	
	
	if (pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_DISABLE)
	{	//dongle mode.
		DBG_WLAN__INIT(LEVEL_TRACE, "Usb dongle mode %x\n", *psubSystemID);
	}
	else
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "Minicard mode %x\n", *psubSystemID);
	}	
#else
	pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_ENABLE;
	DBG_WLAN__INIT(LEVEL_TRACE, "not define Antenna diversity , default Minicard mode , subSystemID = %x \n", *psubSystemID));
#endif  

	if (pAdap->wlan_attr.antenna_diversity == ANTENNA_DIVERSITY_DISABLE)
	{	//dongle mode.
		pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;
	}
	else
	{	//Minicard mode.
		UINT8	antenna_flag;
		antenna_flag = sc_bus_get_antenna_flag(pAdap->pBusInterfaceData);			//add by hank 20090710
		if (antenna_flag == 1)
		{
			pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_DISABLE;
			pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;
		}
		else if (antenna_flag == 2)
		{
			pAdap->wlan_attr.antenna_diversity = ANTENNA_DIVERSITY_DISABLE;
			pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MAX;
		}
		else
		{
			pAdap->wlan_attr.antenna_num = DSP_ANTENNA_TYPE_MIN;	
		}
		
		DBG_WLAN__INIT(LEVEL_TRACE, "Minicard mode  antenna_flag = %d antenna_diversity = %d %d\n", 
			antenna_flag, pAdap->wlan_attr.antenna_diversity, pAdap->wlan_attr.antenna_num);
	}
#ifndef DOWNLOAD_CODE_WITH_H_FILE_MODE
    //get fw by reading bin file
    DBG_WLAN__INIT(LEVEL_TRACE, "load 3dspcode.bin \n");
    do{

        Status = Adap_GetDspFw(pAdap);
        if(Status != STATUS_SUCCESS)
        {
            break;
        }
		// 090702 glen added for extend card type support
        FileLength = 0;
        pBinFileBuffer = NULL;

        //
        if(pAdap->fw_buf!= NULL  && pAdap->fw_dspvalid)
        {
            FileLength = pAdap->fw_length;
            pBinFileBuffer= (PSP20_BIN_CODE)pAdap->fw_buf;
            if(pAdap->fw_type == FW_TYPE_DSPEXT)
            {
                BasePortStart = pAdap->fw_ext_baseport;
                BasePortEnd = BasePortStart +3;
            }
       }
    
        if((FileLength ==0) ||(pBinFileBuffer == NULL))
        {
            
		    DBG_WLAN__INIT(LEVEL_ERR, "[%s]firmware must be error!",__FUNCTION__);
        }

   
         // glen090415 Added for get DSP code abnormal handle Begin
        FlagFindDspCode = 0;
        pBinFileBufferTmp = pBinFileBuffer;
        AddrCode = 0;
        while(AddrCode<FileLength)
        {
            CodeSize = pBinFileBufferTmp->Size;
            CodeHead = pBinFileBufferTmp->Code;
            if((pBinFileBufferTmp->Port>=BasePortStart)&&(pBinFileBufferTmp->Port<=BasePortEnd))
            {
                DBG_WLAN__INIT(LEVEL_TRACE, "Find DSP Code Port %d size 0x%X", pBinFileBufferTmp->Port, pBinFileBufferTmp->Size);
                FlagFindDspCode = 1;
                break;
            }           
            else
            {
                 DBG_WLAN__INIT(LEVEL_TRACE, "Serch DSP: Port %-4d size 0x%X skip\n", pBinFileBufferTmp->Port, pBinFileBufferTmp->Size);
            }
            pBinFileBufferTmp = (PSP20_BIN_CODE)((PUINT8)pBinFileBufferTmp+CodeSize+sizeof(pBinFileBufferTmp->Size)+sizeof(pBinFileBufferTmp->Port));           
            AddrCode += CodeSize+sizeof(pBinFileBufferTmp->Size)+sizeof(pBinFileBufferTmp->Port);
        }
        if(FlagFindDspCode==0)
        {       
            DBG_WLAN__INIT(LEVEL_TRACE, "Can not find DSP Code, So we download default Dongle DSP code\n");
            BasePortStart=0*8;
            BasePortEnd=BasePortStart+3;        
        }
         // glen090415 Added for get DSP code abnormal handle end
        Status = STATUS_SUCCESS;
        FlagFindDspCode = 0;   // glen090415 Added for get DSP code abnormal handle
        while(FileLength > 0)
        {
            // 20081020 add by glen for support subsystemID extend defined by EEPROM data
            if((pBinFileBuffer->Port>=BasePortStart)&&(pBinFileBuffer->Port<=BasePortEnd))
            {
                DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X Dsp code\n", pBinFileBuffer->Port, pBinFileBuffer->Size);
                FlagFindDspCode = 1;   // glen090415 Added for get DSP code abnormal handle
                if(pBinFileBuffer->Size > 0 && Status == STATUS_SUCCESS)
                {
                    if(pAdap->run_in_halt_flag == TRUE || (Adap_Driver_isHalt(pAdap)))
                    {
                        Status = STATUS_FAILURE;
                        break;
                    }
                
                    Status = Adap_Dspfw_Download_Interface(
                        pAdap,
                        pBinFileBuffer->Code,
                        pBinFileBuffer->Size,
                        (UINT8)(pBinFileBuffer->Port-BasePortStart));

                    if(Status != STATUS_SUCCESS)
                    {
                        DBG_WLAN__INIT(LEVEL_TRACE, "DOWNLOAD dsp fail 5\n");
                        break;
                    }

                }   
                else
                {
                    DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 555-x\n");
                    break;
                }   
            }
            else if(pBinFileBuffer->Port==(BasePortStart+4))
            {
                DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X  mini Dsp code\n", pBinFileBuffer->Port, pBinFileBuffer->Size);
                if(pBinFileBuffer->Size > 0 && Status == STATUS_SUCCESS)
                {
                    if(pAdap->run_in_halt_flag == TRUE || (Adap_Driver_isHalt(pAdap)))
                    {
                        Status = STATUS_FAILURE;
                        break;
                    }
                
                    Status = Fw_download_8051fw_fragment(
                        pAdap,
                        pBinFileBuffer->Code,
                        (UINT16)pBinFileBuffer->Size,
                        (UINT16)DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET);

                    if(Status != STATUS_SUCCESS)
                    {
                        DBG_WLAN__INIT(LEVEL_TRACE, "DOWNLOAD dsp fail 6\n");
                        break;
                    }
                }   
                else
                {
                    DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 666-x\n");
                    break;
                }   
            }           
            else if(pBinFileBuffer->Port==(BasePortStart+5))
            {
                DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X BT Dsp Parameter\n", pBinFileBuffer->Port, pBinFileBuffer->Size);
                if(pBinFileBuffer->Size > 0 && Status == STATUS_SUCCESS)
                {
                    if(pAdap->run_in_halt_flag == TRUE || (Adap_Driver_isHalt(pAdap)))
                    {
                        Status = STATUS_FAILURE;
                        break;
                    }
                
                    Status = Fw_download_8051fw_fragment(
                        pAdap,
                        pBinFileBuffer->Code,
                        (UINT16)pBinFileBuffer->Size,
                        (UINT16)DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET);

                    if(Status != STATUS_SUCCESS)
                    {
                        DBG_WLAN__INIT(LEVEL_TRACE, "DOWNLOAD dsp fail 7\n");
                        break;
                    }
                }   
                else
                {
                    DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 777-x\n");
                    break;
                }   
                
                sc_sleep(2);
                Vcmd_Funciton_Set_BTPara_Ready(pAdap);
                DBG_WLAN__INIT(LEVEL_TRACE, "BT_PARAMETER_READY_0 \n");
                sc_sleep(200);
            }
            else{
                 DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X  skip\n", pBinFileBuffer->Port, pBinFileBuffer->Size);
            }
            // 20081020 add by glen end
            if(FileLength >= (pBinFileBuffer->Size+sizeof(SP20_BIN_CODE) -1))
            {
                FileLength -= (pBinFileBuffer->Size+sizeof(SP20_BIN_CODE) -1); 
            }
            else
            {
               DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Bin_Initialize: 555-5\n");
            }
            pBinFileBuffer = (PSP20_BIN_CODE)((PUINT8)(pBinFileBuffer)+ pBinFileBuffer->Size+sizeof(SP20_BIN_CODE) -1 );
        #if 0
            NdisMSleep(2000);

            if(FileLength >= (pBinFileBuffer->Size + sizeof(PSP20_BIN_CODE) -1))
            {
                FileLength -= (pBinFileBuffer->Size + sizeof(PSP20_BIN_CODE) -1);
            }
            else
            {
                DBGSTR(("sp20 length error while downloading \n"));
                Status = NDIS_STATUS_FAILURE;
                break;
            }
        #endif          
        }
    }while(FALSE);

    //release all buf
    if(pAdap->fw_buf != NULL)
    {
        sc_memory_free(pAdap->fw_buf);
        pAdap->fw_buf = NULL;
        pAdap->fw_dspvalid = FALSE;
        pAdap->fw_length = 0;
    }

 
    #else
    {
    	//here get fw file from a .h file

    	if(pAdap->dsp_fw_mode ==  INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
        {
    		DBG_WLAN__INIT(LEVEL_TRACE, "load sp20code.h in combo mode\n");
    		FileLength = sizeof_Usb_SP20CodeCombo;
    		DBG_WLAN__INIT(LEVEL_TRACE, "code size %x \n",FileLength);
    		pCodeBuffer = (PSP20_CODE)Usb_SP20CodeCombo;
    	}
    	else
    	{
    		DBG_WLAN__INIT(LEVEL_TRACE, "load sp20code.h in wlan only mode\n");
    		FileLength = sizeof_Usb_SP20CodeWlanOnly;
    		DBG_WLAN__INIT(LEVEL_TRACE, "code size %x \n",FileLength);
    		pCodeBuffer = (PSP20_CODE)Usb_SP20CodeWlanOnly;
    	}
    	
    	ASSERT(FileLength>0);
    	ASSERT(pCodeBuffer->Size != 0);

    	while((FileLength>0)&& (Status == STATUS_SUCCESS))
    	{
    		// 20081020 add by glen for support subsystemID extend defined by EEPROM data
    		if((pCodeBuffer->Port>=BasePortStart)&&(pCodeBuffer->Port<=BasePortEnd))
    		{
    //			DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X DSP code\n", pCodeBuffer->Port, pCodeBuffer->Size);
    			if(pAdap->run_in_halt_flag)
    			{
    				Status = STATUS_FAILURE;
    				break;
    			}
    			
    			Status = Adap_Dspfw_Download_Interface(
    				pAdap,
    				pCodeBuffer->Code,
    				pCodeBuffer->Size,
    				pCodeBuffer->Port-BasePortStart);
    			//DBGSTR_HW_RESET(("@@@@@@@@#$$$Adap_HardwareReset. aaaaaaaa  \n")); 				
    		}			
    		else if(pCodeBuffer->Port==(BasePortStart+4))
    		{
    //			DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X mini DSP code\n", pCodeBuffer->Port, pCodeBuffer->Size);
    			if(pCodeBuffer->Size > 0 && Status == STATUS_SUCCESS)
    			{
    				if(pAdap->run_in_halt_flag)
    				{
    					Status = STATUS_FAILURE;
    					break;
    				}
    			
    				Status = Fw_download_8051fw_fragment(
    					pAdap,
    					pCodeBuffer->Code,
    					(UINT16)pCodeBuffer->Size,
    					(UINT16)DOWNLOAD_8051_MINIDSP_FW_FIELD_OFFSET);

    				if(Status != STATUS_SUCCESS)
    				{
    					DBG_WLAN__INIT(LEVEL_TRACE, "DOWNLOAD dsp fail 6\n");
    					break;
    				}
    			}	
    			else
    			{
    //				DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 666-x\n");
    				break;
    			}	
    		}			
    		else if(pCodeBuffer->Port==(BasePortStart+5))
    		{
    //			DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X BT Dsp Parameter\n", pCodeBuffer->Port, pCodeBuffer->Size);
    			if(pCodeBuffer->Size > 0 && Status == STATUS_SUCCESS)
    			{
    				if(pAdap->run_in_halt_flag)
    				{
    					Status = STATUS_FAILURE;
    					break;
    				}
    			
    				Status = Fw_download_8051fw_fragment(
    					pAdap,
    					pCodeBuffer->Code,
    					(UINT16)pCodeBuffer->Size,
    					(UINT16)DOWNLOAD_8051_BTDSP_PARA_FW_FIELD_OFFSET);

    				if(Status != STATUS_SUCCESS)
    				{
    					DBG_WLAN__INIT(LEVEL_TRACE,"DOWNLOAD dsp fail 7\n");
    					break;
    				}
    			}	
    			else
    			{
    //				DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 777-x\n");
    				break;
    			}	
    			
    			sc_sleep(200);
    			Vcmd_Funciton_Set_BTPara_Ready(pAdap);
    			DBG_WLAN__INIT(LEVEL_TRACE, "BT_PARAMETER_READY_0 \n");
    //			sc_sleep(200);
    		}
    		else
    		{
    //			DBG_WLAN__INIT(LEVEL_TRACE, "Download DSP: Port %-4d size 0x%X skip\n", pCodeBuffer->Port, pCodeBuffer->Size);
    		}
    		// 20081020 add by glen end
    		//download next file
    		//pCodeBuffer = (PSP20_CODE)(pCodeBuffer->Code + pCodeBuffer->Size );
    		if(FileLength >= (pCodeBuffer->Size+sizeof(SP20_CODE) -1))
    		{
    			FileLength -= (pCodeBuffer->Size+sizeof(SP20_CODE) -1); 
    		}
    		else
    		{
    //			DBG_WLAN__INIT(LEVEL_TRACE, "Dsp_Initialize: 777-5\n");
    			FileLength = 0;
    		}
    		pCodeBuffer = (PSP20_CODE)((PUINT8)(pCodeBuffer)+ pCodeBuffer->Size+sizeof(SP20_CODE) -1 );
    	}	

	    sc_sleep(200);
    }
#endif//#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE
	if(pAdap->run_in_halt_flag == TRUE)
		return  STATUS_FAILURE;

	if(Status != STATUS_SUCCESS)
	{	//Justin: 20081210.		To avoid 8051 keep in wait download_dsp stat forever.
		 Vcmd_Set_8051_MinLoop(pAdap,FALSE);
	}

	return (Status);
}


void Adap_Initial_Timer(PDSP_ADAPTER_T pAdap)
{
		
	/* Initialize system timer. */
	wlan_timer_init(&pAdap->sys_timer,
	                Adap_SysTimeout,
		            (PVOID)pAdap);
	
	/* Initialize management module timer. */
	wlan_timer_init(&pAdap->mng_timer,
	                Adap_MngTimeout,
		            (PVOID)pAdap);


	//counter measure
	wlan_timer_init(&pAdap->tx_countermeasure_timer,
	                  Adap_CounterMeasureTimeout,
		             (PVOID)pAdap);		

	/* Initialize PeriodicTimer timer. */
	wlan_timer_init(&pAdap->periodic_timer,
	                    Adap_PeriodicTimeout,
		                (PVOID)pAdap);
	
}

TDSP_STATUS Adap_StartDevice(PDSP_ADAPTER_T pAdap)
{
#if 0
    UINT8	Magic[4] = {0x33, 0x44, 0x53, 0x50};//stand for"3DSP"
	UINT32   loop;
    UINT8    tmpBuf[4];
#endif
    DBG_WLAN__INIT(LEVEL_TRACE, "Enter [%s] ,bus inf is %x!\n",__FUNCTION__,pAdap->pBusInterfaceData);
	/* Set system timer */
	// wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);
	wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
	
#if 1//def CHECK_BULK_STALL_ENABLE
	wlan_period_timer_start(&pAdap->periodic_timer,TIMER_PERIODIC_DELAY);
#endif /*CHECK_BULK_STALL_ENABLE*/

	
	/*Jakio20061208: moved codes here, should do this after mng_init()*/
	/*Jakio 2006.11.09: whenever we change pAdap->wlan_attr->rate,
	  *we should reset rateIndex to keep synchronization between the two*/
    Adap_SetRateIndex(pAdap);

	//1:here download 8051 fw
  	   //first download 8051 firmware
    sc_event_init(&pAdap->is_8051_ready_event);

//////////////////////////////////////////////////////////////////////////////////
    if(Vcmd_8051fw_isWork(pAdap))
    {
       // UINT32 loop = 1000;
    	Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_READY);
    	
     	//into ready state
     	//Drop int for getting message

	    if (STATUS_SUCCESS != Int_SubmitInt(pAdap))
			return STATUS_FAILURE;

    	//Adap_DispatchInterruptIrps(pAdap,1);

    	Adap_Set_Usbbus_Enable_Reset(pAdap);

       //pAdap->wlan_attr.chipID = Adap_get_chipid(pAdap);
        pAdap->wlan_attr.chipID = 4;

#if 0
    	//Justin: 0810.  permanent address should only read here once befor write current address
    	Adap_ReadMacAddress(pAdap,pAdap->permanent_address);
    	// If there is no saved mac address in the registry, the permanent address is acted as mac address.
    	if (sc_memory_cmp(pAdap->current_address, DSP_INVALID_ADDR, ETH_LENGTH_OF_ADDRESS))
    	{
    		WLAN_COPY_ADDRESS(pAdap->current_address, pAdap->permanent_address);
    	}	
    	else
    	// If a saved mac address is in the registry, driver need to write it to the register. 
    	{
    		Adap_WriteMacAddress(pAdap,pAdap->current_address);
    	}

    	// Copies the current address to device address
    	WLAN_COPY_ADDRESS(pAdap->wlan_attr.dev_addr, pAdap->current_address);
    	GetTxPwrTable_from_eeprom(pAdap);
    	//Jakio20070516: print all mac reg
    	//DBGSTR(("print MAC registers after  DSP code run\n"));
    	//Adap_Print_MacReg(pAdap);
     	//send cmd to 8051 to notify 8051 wlan fucntion will work
#endif
    	DBG_WLAN__INIT(LEVEL_TRACE, "[%s] init process with 8051 running\n",__FUNCTION__);

    	sc_event_reset(&pAdap->is_8051_ready_event);
     	if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]: req join vcmd failure\n",__FUNCTION__);
    		return STATUS_FAILURE;
    	}
    	
    	//wait for 8051 ready		
    	if( 0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
    	{
    	 	if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
    		{
    			DBG_WLAN__INIT(LEVEL_ERR, "[%s]: req join vcmd failure\n",__FUNCTION__);
    			return STATUS_FAILURE;
    		}



        	//wait for 8051 ready		
        	if(0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
        	{
        		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Initialize failure : no 51 ready event\n",__FUNCTION__);
        		return STATUS_FAILURE;
        	}
    	
    	}
    	DBG_WLAN__INIT(LEVEL_TRACE, "[%s] join int returned\n",__FUNCTION__);
    }
	else
	{

    	//Adap_DispatchInterruptIrps(pAdap,1);
                
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s] Start to download 8051 firmwire !\n",__FUNCTION__);

		if (Adap_Get8051FwCodesAndDownload(pAdap) != STATUS_SUCCESS)
		{
			DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Adap_Get8051FwCodesAndDownload failure\n",__FUNCTION__);
			return STATUS_FAILURE;
		} 

		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Adap_Get8051FwCodesAndDownload Success!\n",__FUNCTION__);

        Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_READY);
		//Drop int for getting message
		Int_SubmitInt(pAdap);
        /* Delay 2 ms */
		sc_sleep (2);

        //Tell 8051 that wlan want to downlaod dsp code.
        //This can avoid that the 2 drivers do download dsp operation at the same time.
        // -- Begin --
        sc_event_reset(&pAdap->is_8051_ready_event);
        if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
        {
            DBG_WLAN__INIT(LEVEL_TRACE,"222   Dsp_Initialize: req join vcmd failure! \n");
            return STATUS_FAILURE;
        }
        
        //wait for 8051 ready       
        if(0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
        {
            if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
            {
                DBG_WLAN__INIT(LEVEL_TRACE,"333  Dsp_Initialize: req join vcmd failure\n");             
                return STATUS_FAILURE;
            }
            //wait for 8051 ready       
            if(0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
            {
                DBG_WLAN__INIT(LEVEL_TRACE,"4444  Dsp_Initialize: Initialize failure : no 51 ready event\n");
                return STATUS_FAILURE;
            }
        }

        DBG_WLAN__INIT(LEVEL_TRACE,"555  Dsp_Initialize: join int returned\n");

#if 0
        //check whether 8051 has completed initialization
		Basic_ReadBurstRegs(pAdap, tmpBuf, 4, 0x1FC);
		DBG_WLAN__INIT(LEVEL_TRACE ,  "0x1FC reg value is:%2x %2x %2x %2x\n",tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3]);

		loop=0;
		while(0 != sc_memory_cmp(Magic, tmpBuf, 4))
		{
			sc_sleep(10);	
			Basic_ReadBurstRegs(pAdap ,tmpBuf, 4,0x1FC);
			DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: 8051 Firmware init not completed!\n",__FUNCTION__);	
			loop++;

			if(loop>30)
			{
				return STATUS_FAILURE;
			}
		}
#endif
	 
	}

	 //2:here begin init 3dsp chip
	 //here init 3dsp chip after 8051 fw running	


	GetTxPwrTable_from_eeprom(pAdap);
	
	//GET 8051FW
	pAdap->DSP8051_FW_version = 	Adap_Get_8051FW_VERSION(pAdap);
	
	//COPY driver info
	//now this is a null comd 
	//Vcmd_reset_3dsp_request(pAdap);	// Justin: not be used yet
 	//get chip id
	pAdap->wlan_attr.chipID = Adap_get_chipid(pAdap);


	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: chipID is %d!\n",__FUNCTION__,pAdap->wlan_attr.chipID);
	//disable card
	//the cmd should be done before downlaod fw
	// if host can access 3dsp before 8051 enable
    #if 0
    tmp = VcmdR_3DSP_Dword(pAdap,0x2000);
    DBG_WLAN__ENTRY(LEVEL_TRACE, "before: tmp1 is 0x%x\n",tmp);
    tmp = 0x12;
    VcmdW_3DSP_Dword(pAdap,tmp,0x2000);
    tmp = VcmdR_3DSP_Dword(pAdap,0x2000);
    DBG_WLAN__ENTRY(LEVEL_TRACE, "after: tmp1 is 0x%x\n",tmp);
    return STATUS_SUCCESS;
    #endif

	Vcmd_NIC_INTERRUPT_DISABLE(pAdap);
	if(Vcmd_CardBusNICReset(pAdap)  != STATUS_SUCCESS)
	{
		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Vcmd_CardBusNICReset failure\n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Start to download dsp firmwire !\n",__FUNCTION__);
	
	/* Download the firmware codes */
	if (Adap_GetDspFwCodesAndDownload(pAdap) != STATUS_SUCCESS)
	{
		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Adap_GetDspFwCodesAndDownload failure\n",__FUNCTION__);
		return STATUS_FAILURE;
	} 

	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Adap_GetDspFwCodesAndDownload Success!\n",__FUNCTION__);
	
	/* Delay 2 ms */
	sc_sleep (2);

	Adap_Set_Usbbus_Enable_Reset(pAdap);

	// Enable board interrupts
	Vcmd_NIC_INTERRUPT_ENABLE(pAdap);
	//Jakio20070518: open it
	//it is important to keep pci bus stable
	//Vcmd_NIC_ENABLE_RETRY(pAdap, CARDBUS_NUM_OF_RETRY);


	//Justin: 0810.  permanent address should only read here once befor write current address
	Adap_ReadMacAddress(pAdap,pAdap->permanent_address);
	
	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: permanent_address=%02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,
		pAdap->permanent_address[0],pAdap->permanent_address[1],pAdap->permanent_address[2],
		pAdap->permanent_address[3],pAdap->permanent_address[4],pAdap->permanent_address[5]);
	
	// If there is no saved mac address in the registry, the permanent address is acted as mac address.
	if (0 == sc_memory_cmp(pAdap->current_address, (PVOID)DSP_INVALID_ADDR, ETH_LENGTH_OF_ADDRESS))
	{
		WLAN_COPY_ADDRESS(pAdap->current_address, pAdap->permanent_address);
	}	

	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: current_address=%02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,
		pAdap->current_address[0],pAdap->current_address[1],pAdap->current_address[2],
		pAdap->current_address[3],pAdap->current_address[4],pAdap->current_address[5]);

	if(STATUS_SUCCESS != Adap_WLSMAC_initialize(
								pAdap,
								pAdap->wlan_attr.gdevice_info.dot11_type,
								pAdap->wlan_attr.macmode))
	{
		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: WLSMAC_initialize Failed.\n",__FUNCTION__);

//        	Vcmd_NIC_INTERRUPT_DISABLE(pAdap);	//justin: 080407.	no need to do this
//        	Vcmd_NIC_RESET_ALL_BUT_HARDPCI(pAdap);
		return STATUS_FAILURE;
	}

    DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Adap_WLSMAC_initialize Success!\n",__FUNCTION__);
	
#if 0
	//Jakio20070516: print all mac reg
	DBG_WLAN__MAIN(LEVEL_TRACE,"print MAC registers after  DSP code run\n");
	Adap_Print_MacReg(pAdap);
#endif

	//enable interrupt
	Vcmd_unmask_idle_intr(pAdap);

	
	sc_sleep(1); // delay 1ms
	Vcmd_set_next_state(pAdap,DEV_IDLE, 0,0);
	sc_sleep(1); // delay 1ms
	Vcmd_set_next_state(pAdap,DEV_ACTIVE, 0,0);

	sc_sleep(1); // delay 1ms	

	#if 0 //Jakio20070522:  //notify 8051 into main loop later
		/* Set download-ok flag to 8051 */
	Vcmd_Set_Firmware_Download_Ok(pAdap);
	//Jakio20070428: change it to open mode. "1" stands for open, "2" stands wep
	//Jakio20070426:set encryption mode to 8051
	//just for test, fix the mode to be WEP
	Vcmd_Set_Encryption_Mode(pAdap);
	#endif
	//Jakio20070515: here we change the mode to test WPA, add "3" for WPA, not inform jack yet.
	//Vcmd_Set_Encryption_Mode(pAdap,3);
    


#if 0    //do it later
	/* Get  firmware status*/
	firmware_status = Adap_Get_Firmware_Status(pAdap);
	while (firmware_status != USB_FIRMWARE_STATUS_COMPLETE)
	{
		firmware_status = Adap_Get_Firmware_Status(pAdap);
		if (firmware_status == USB_FIRMWARE_STATUS_FAILURE)
		{
			Adap_UnInitialize(pAdap);
			Adap_FreeAdapterObject(pAdap);
			return STATUS_FAILURE;
		}
		sc_sleep (5000);
	}

#endif

	//Jakio20070516: should not read mac address here, because 0x4008 has been cleared
	//the value has been saved in func "Adap_mmacRestart_initial"
	// Read permanent address from eeprom through control pipe.
	//Adap_ReadMacAddress(pAdap, pAdap->permanent_address);


#if 0  // do it later
	/* Read firmware version information. */
	Adap_ReadFirmwareVersion(pAdap);
	
	/* Read CIS information from EEProm. */ 
	Phy_ReadCISInfo(pAdap);
#endif

#if 0
	//Justin: 0810.  permanent address should only read here once befor write current address
	Adap_ReadMacAddress(pAdap,pAdap->permanent_address);
	// If there is no saved mac address in the registry, the permanent address is acted as mac address.
	if (sc_memory_cmp(pAdap->current_address, DSP_INVALID_ADDR, ETH_LENGTH_OF_ADDRESS))
	{
		WLAN_COPY_ADDRESS(pAdap->current_address, pAdap->permanent_address);
	}	
	else
	// If a saved mac address is in the registry, driver need to write it to the register. 
	{
		Adap_WriteMacAddress(pAdap,pAdap->current_address);
	}

	// Copies the current address to device address
	WLAN_COPY_ADDRESS(pAdap->wlan_attr.dev_addr, pAdap->current_address);
	//Jakio20070516: print all mac reg
	//DBGSTR(("print MAC registers after  DSP code run\n"));
	//Adap_Print_MacReg(pAdap);
#endif

#if 0
	//Justin: 0810.  permanent address should only read here once befor write current address
	Adap_ReadMacAddress(pAdap,pAdap->permanent_address);
	// If there is no saved mac address in the registry, the permanent address is acted as mac address.
	if (sc_memory_cmp(pAdap->current_address, DSP_INVALID_ADDR, ETH_LENGTH_OF_ADDRESS))
	{
		WLAN_COPY_ADDRESS(pAdap->current_address, pAdap->permanent_address);
	}	
	else
#endif			
	// If a saved mac address is in the registry, driver need to write it to the register. 
	{
		Adap_WriteMacAddress(pAdap,pAdap->current_address);
	}

	// Copies the current address to device address
	WLAN_COPY_ADDRESS(pAdap->wlan_attr.dev_addr, pAdap->current_address);
	//
	Adap_set_any_wep_prevent_tx_stop(pAdap);
#if 0		
	//Jakio20070523: moved code here, prevent 8051 dead lock when receive data
	/* Set download-ok flag to 8051 */
	Vcmd_Set_Firmware_Download_Ok(pAdap);	
	//Jakio20070528: change it to tkip mode, "3" stands for tkip
	//Jakio20070428: change it to open mode. "1" stands for open, "2" stands wep,
	//Jakio20070426:set encryption mode to 8051
	//just for test, fix the mode to be WEP
	Vcmd_Set_Encryption_Mode(pAdap);


	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
#endif		

//wumin	pAdap->usb_mode = Adap_GetUsbMode(pAdap);

	Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_WORK);

 	/* Dispatch one rx irp and urb */
 //wumin		Adap_DispatchBulkInIrps(pAdap,DSP_RX_IRPPOOL_SIZE_USB20);
 	Rx_restart(pAdap);
 
 	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: init for usb 20 device \n",__FUNCTION__);
	

#if 0		
	/* Set system timer */
	// wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);
	wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
//#ifdef CHECK_BULK_STALL_ENABLE
#if 1//justin: 071212.		this timer not only for check bulk stall 
	wlan_period_timer_start(&pAdap->periodic_timer,TIMER_PERIODIC_DELAY);
#endif /*CHECK_BULK_STALL_ENABLE*/

#endif
#if 0 //do it later
	/* Set basic reset type */
	Adap_SetResetType(pAdap, RESET_TYPE_NOT_SCAN);

	/* Starts card. */
	Adap_StartCard(pAdap);

#ifdef NDIS50_MINIPORT
	/* Start management module. */
	Mng_Start_Win2000(pAdap);
#endif  //NDIS50_MINIPORT

#endif// 0

	
	
	
	Vcmd_NIC_INTERRUPT_ENABLE(pAdap);
	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Completed Init Successfully\n",__FUNCTION__);

	{
		UINT8 verTemp[10];
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: driver version = %s\n",__FUNCTION__,USB_WLAN_VERSION);
		sc_memory_copy(verTemp,(PUINT8)&pAdap->DSP8051_FW_version,sizeof(UINT32));
		verTemp[4] = 0;
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: 8051 version = %02d.%02d.%02d.%02d\n",
	                __FUNCTION__,verTemp[3],verTemp[2],
	                verTemp[1],verTemp[0]);
		sc_memory_copy(verTemp,(PUINT8)&pAdap->DSP_FW_version,sizeof(UINT32));
		verTemp[4] = 0;
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: dsp version = %02d.%02d.%02d.%03d\n",__FUNCTION__,
	                verTemp[3],verTemp[2],
	                verTemp[1],verTemp[0]);
	}
	

	//Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
	/* Set download-ok flag to 8051 */
	if(STATUS_SUCCESS != Vcmd_Set_Firmware_Download_Ok(pAdap,TRUE))
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Initialize failure in Vcmd_Set_Firmware_Download_Ok \n",
                        __FUNCTION__);
		return STATUS_FAILURE;
	}

    DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Vcmd_Set_Firmware_Download_Ok Successfully!\n",__FUNCTION__);

	if(STATUS_SUCCESS != Vcmd_Set_Encryption_Mode(pAdap))
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Initialize failure in Vcmd_Set_Encryption_Mode \n",
            __FUNCTION__);
		return STATUS_FAILURE;
	}
    
    DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Vcmd_Set_Encryption_Mode Successfully!\n",__FUNCTION__);

    return STATUS_SUCCESS;
}

void * Adap_Create(PUSB_INFO usbinfo)
{
    PDSP_ADAPTER_T    pAdap = NULL;
 
	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]  dsp Initialize start...!\n",__FUNCTION__);
    
    DBG_WLAN__ENTRY(LEVEL_TRACE,"%s usbinfo is %x,EP_control is %d,EP_bulkout_HEAD is %d!\n", 
                    __FUNCTION__,
                    (UINT32)usbinfo,
                    usbinfo->EP_control,
                    usbinfo->EP_bulkout_HEAD);
	/* Allocate the Adapter Object, exit if error occurs */
    pAdap = (PDSP_ADAPTER_T)sc_memory_alloc(sizeof(DSP_ADAPTER_T));

    if(NULL == pAdap)
	{
		DBG_WLAN__ENTRY(LEVEL_ERR,"%s: ADAPTER Allocate Memory failed!\n", __FUNCTION__);
		return NULL;
    }
    /* Zero out the adapter object space */
   	sc_memory_set(pAdap, 0, sizeof(DSP_ADAPTER_T));

    sc_memory_copy(&pAdap->usb_info, usbinfo, sizeof(USB_INFO));
   
    return pAdap;
}
/*****************************************************************
 *   Dsp_Initialize
 *
 *   Descriptions:
 *      Starts an adapter and registers resources with the wrapper.
 *   Arguments:
 *      OpenErrorStatus: OUT, extra status bytes for opening adapters.
 *      SelectedMediumIndex: OUT, index of the media type chosen by
 *                           the driver.
 *      MediumArray: IN, array of media types for the driver to chose from.
 *      MediumArraySize : IN, number of entries in the array.
 *      MiniportAdapterHandle : IN, Handle for passing to the wrapper when 
 *                              referring to this adapter.
 *      WrapperConfigurationContext : IN, a handle to pass to NdisOpenConfiguration.
 *   Return Value:
 *           STATUS_SUCCESS for sucessful
 *           TDSP_STATUS_xxx for others
 *
 ****************************************************************/
int	Adap_Initialize(void * adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    TDSP_STATUS        status;
 
	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]  dsp Initialize start,bus inf is %x...!\n",__FUNCTION__,pAdap->pBusInterfaceData);
    
    status = UsbDev_Init(pAdap);
	DBG_WLAN__INIT(LEVEL_TRACE, "pAdap=%p, pAdap->usb_context=%p \n", pAdap, pAdap->usb_context);
	
	if (status != STATUS_SUCCESS)
	{
		//DBG_WLAN__INIT(LEVEL_ERR, "[%s] Usb device Init failed (Status = 0x%x)\n",__FUNCTION__,status);
		//Adap_FreeAdapterObject(pAdap);
		return -1;
    }
//	pAdap->debug_val = 0;
	/* Init some members of adapter context */
	Adap_Init_Workstate(pAdap);
	Adap_Init_Wlan_Attr(&pAdap->wlan_attr);
	Adap_Initial_Timer(pAdap);
	pAdap->run_in_halt_flag = FALSE;
	pAdap->hardware_reset_flag = FALSE;
	pAdap->adap_released = FALSE;

   //init download file variable
    pAdap->fw_buf= NULL;
    pAdap->fw_length= 0;
    pAdap->fw_8051valid    = FALSE;
    pAdap->fw_dspvalid     = FALSE;
    pAdap->txHangFlag = TXHANG_IGNORE;

	sc_event_init(&pAdap->set_key_event);
	
	//Jakio2006.12.27:open it for ioctrl
#ifdef OPEN_DEVICE_CONTROL_INTERFACE
	/* Save the pointer of adapter context into driver block */
//wumin	DSPMiniportBlock.pAdap = pAdap;
#endif

	//add debug flag, should be cancel with release mode
     // TODO:Jackie
    //pAdap->debug = (UINT32)DBG_FLAG;


	//Justin: 1108.	get region must after parsed registry, so that we have gotten the correct region setting.
	Adap_GetRegionDomain(pAdap, 0);

    /*Jakio 2006.11.09: whenever we change pAdap->wlan_attr->rate,
	  *we should reset rateIndex to keep synchronization between the two*/
    //Adap_SetRateIndex(pAdap);
	
	//Jakio2006.12.22: initialize test_mode_flag
	pAdap->test_mode_flag = FALSE;

#if 0//close for combo
	Status = Mng_Init((NDIS_HANDLE)pAdap);
	if (Status != STATUS_SUCCESS)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Mangement module  Init failed (Status = 0x%x)\n", Status);
		Adap_UnInitialize(pAdap);
		Adap_FreeAdapterObject(pAdap);
		return STATUS_FAILURE;
	}
	


	/*Jakio20061208: moved codes here, should do this after mng_init()*/
	/*Jakio 2006.11.09: whenever we change pAdap->wlan_attr->rate,
	  *we should reset rateIndex to keep synchronization between the two*/
	Adap_SetRateIndex(pAdap);
#endif


	/* Initialize task module which is used for ourselves passive thread(mainly used for control pipe) */
	status = Task_Init(pAdap);
	DBG_WLAN__INIT(LEVEL_TRACE, "pAdap=%p, pAdap->usb_context=%p \n", pAdap, pAdap->usb_context);
	if (status != STATUS_SUCCESS)
	{
		DBG_WLAN__INIT(LEVEL_ERR, "[%s] Task Init failed (Status = 0x%x)\n",__FUNCTION__,status);
		Adap_UnInitialize(pAdap);
		return -1;
	}
    
    #if 0
    tmp = Basic_ReadRegByte(pAdap,0x140);
    DBG_WLAN__ENTRY(LEVEL_TRACE, "tmp1 is 0x%x\n",tmp);
	tmp = 0xbf;
	status = Basic_WriteRegByte(pAdap,tmp,0x140);
    DBG_WLAN__ENTRY(LEVEL_TRACE, "tmp2 is 0x%x\n",tmp);
    tmp = Basic_ReadRegByte(pAdap,0x140);
    DBG_WLAN__ENTRY(LEVEL_TRACE, "tmp3 is 0x%x\n",tmp);
    #endif

    pAdap->test_mode_flag = FALSE;

	if (Mng_Init(pAdap) != STATUS_SUCCESS)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"[%s]: Initialize failure in Mng_Init\n", __FUNCTION__);
		Adap_UnInitialize(pAdap);
		return -1;
	}    
	
	if (Int_Init(pAdap) != STATUS_SUCCESS)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"[%s]: Initialize failure in Int_Init\n", __FUNCTION__);
		Adap_UnInitialize(pAdap);
		return -1;
	}    

	if(STATUS_SUCCESS != Rx_init(pAdap))
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Initialize failure in Rx_init \n", __FUNCTION__);
		Adap_UnInitialize(pAdap);
		return -1;
	}
		
	if(STATUS_SUCCESS != Tx_Init(pAdap))
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Initialize failure in Tx_Init \n",
                        __FUNCTION__);
		Adap_UnInitialize(pAdap);
		return -1;
	}
	
	if(STATUS_SUCCESS != Adap_StartDevice(pAdap))
	{
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Initialize failure in Adap_StartDevice \n",
                        __FUNCTION__);
		Adap_UnInitialize(pAdap);
		return -1;
	}

	if (pAdap->pBusInterfaceData != NULL)	//if card plugin first, then insmod driver, it's null
		sc_bus_set_fw_ver(pAdap->pBusInterfaceData, pAdap->DSP8051_FW_version, pAdap->DSP_FW_version);

    return 0;;
} // end Dsp_Initialize


/**************************************************************************
 *   Dsp_Halt 
 *
 *   Descriptions
 *      Removes an adapter instance that was previously initialized.
 *      To "halt" or "remove" an adapter, we disable its interrupt,
 *      abort its receive unit (otherwise it would continue to DMA in
 *      data), and release all of the resources (memory, i/o space,
 *      etc.) that the adapter instance was using.
 *      This routine is only called when the adapter is "stopped"
 *      or unloaded with a "net stop e100b". To see what is called
 *      at machine shutdown see D100ShutdownHandler.
 *   Arguments:
 *      MiniportAdapterContext: IN, pointer to the adapter object data area.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_Halt( void* para )
{
//	UINT32 tmpcounts,icounts;
//	UINT8 tmpchar;
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T) para;
	BOOLEAN bSupriseRemoved = FALSE;
	

	DBG_WLAN__MAIN(LEVEL_TRACE, "[%s] 3DSP_Halt pAdap = %p\n",__FUNCTION__, pAdap);

	//for combo mode, if 8051 is running, wlan driver will not download fw in initialize flow 
	//until 8051 notify it. but at this time, maybe dsp_halt will happen, so we do halt without 
	//hareware reset.

	pAdap->run_in_halt_flag = TRUE;
	
	while((pAdap->driver_state == DSP_HARDWARE_RESET)	
		  || (pAdap->hardware_reset_flag == TRUE))
	{	
		//Here to access reg is to want driver can change its state to suprise remove state when unplug happen.
		//Access reg function will get suprise removing state after suprise happen
		Basic_ReadRegByte(pAdap,REG_USBCTL1);
		sc_sleep(100);		
	}	

	bSupriseRemoved = Adap_Device_Removed(pAdap);
	//Justin: 080319.		to stop dsp, prevent 8051 tx or rx
	//if(!bSupriseRemoved)
	//	Vcmd_set_next_state( pAdap, DEV_IDLE,0, 0);

	DBG_WLAN__MAIN(LEVEL_TRACE, "[%s] dsp halt 1, bSupriseRemoved = %d %d\n",__FUNCTION__, 
		bSupriseRemoved, pAdap->hw_8051_work_mode);


	//if(pAdap->driver_state != DSP_SUPRISE_REMOVE)
	if (!bSupriseRemoved)
	{
	    //Justin: 080122.   restore mac to chip
	    Adap_WriteMacAddress(pAdap,pAdap->permanent_address);
		
	if (pAdap->hw_8051_work_mode != INT_8051_IN_MINILOOP_MODE)
	    Vcmd_Set_8051_MinLoop(pAdap,TRUE);
		
		sc_sleep(1);	
	}

	if(bSupriseRemoved)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "[%s] dsp halt : device has suprised removed and set suprise remove state\n",__FUNCTION__);
		Adap_Set_Driver_State_Force(pAdap,DSP_SUPRISE_REMOVE);
	}
	else	
	{		
//		Adap_request_init_txfifo(pAdap);			by hank 20090713
		DBG_WLAN__MAIN(LEVEL_TRACE, "[%s] dsp halt : set halt state\n",__FUNCTION__);
		Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_HALT);
	}

	/* Cancel all timers. */
//wumin	wlan_timer_kill(&pAdap->mng_timer);
//wumin		wlan_timer_kill(&pAdap->sys_timer);
//wumin		wlan_timer_kill(&pAdap->tx_countermeasure_timer);
//wumin		wlan_timer_kill(&pAdap->periodic_timer);
	

	/* Reset management queue  */
//wumin		MngQueue_Reset((PDSP_MNG_QUEUE_T)pAdap->pmng_queue_list);
	
	/* Reset MCU, BIU and PMU */
//	Adap_Set_SoftReset(pAdap, (UINT16)(BIU_MODULE_SOFT_RESET | MCU_MODULE_SOFT_RESET | PMU_MODULE_SOFT_RESET));

	if(!bSupriseRemoved)
	{
	sc_sleep(20); // delay 20ms
	Tx_Cancel_Data_Frm(pAdap);
	}
 
	/* Free spin lock */
	Adap_Set_Driver_State(pAdap,DSP_DRIVER_HALT);
    DBG_WLAN__MAIN(LEVEL_TRACE,"[%s] dsp halt end\n",__FUNCTION__);
}



/*****************************************************************
 *   Adap_Reset
 *
 *   Descriptions:
 *      Instructs the Miniport to issue a hardware reset to the
 *      network adapter.  The driver also resets its software state.
 *      this function also resets the transmit queues.
 *   Arguments:
 *      AddressingReset: OUT, TRUE if the wrapper needs to call 
 *                   MiniportSetInformation to restore the addressing
 *      MiniportAdapterContext: IN, pointer to the adapter object data area.
 *   Return Value:
 *      STATUS_PENDING - This function sets a timer to complete, so
 *                            pending is always returned
 ****************************************************************/
TDSP_STATUS
Adap_Reset(
		   BOOLEAN* AddressingReset, /* out:   TRUE if the wrapper needs to 
                                                   call MiniportSetInformation 
                                                   to restore the addressing */
		  PDSP_ADAPTER_T pAdap /* in:  pointer to the adapter object data area */
          )
{
	TDSP_STATUS       status;

	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Dsp_Reset\n");

	*AddressingReset = TRUE;

	 status = Adap_Reset_Routine(pAdap);

	 if(STATUS_PENDING != status)
	 {
	 	DBG_WLAN__MAIN(LEVEL_TRACE,"Reset process returned directly with status = %x\n",status);
		return status;
	 }

     /*
    If the driver's MiniportReset function is called, the driver must save the current state of the NIC,
    reset the NIC, and reconfigure the original state. Legacy NICs that saved keys within permanent 
    storage must reload those keys. However, any WEP keys dynamically added through this OID must 
    be discarded.
    */
    Adap_key_mib_to_false(pAdap);
	
	//set timer number to monitor reset procedure	
	pAdap->dsp_pending_count = 0;
	Adap_Set_Driver_State_Force(pAdap,DSP_SYS_RESET);
	pAdap->sys_reset_flag = TRUE;
	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Dsp_Reset out\n");							
	return STATUS_PENDING;
}


TDSP_STATUS
Adap_Reset_Routine(PDSP_ADAPTER_T pAdap)
{
	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Dsp_Reset routine\n");
	
	if(pAdap->driver_state == DSP_POWER_MANAGER)
	{
		return STATUS_SUCCESS;
	}	

#if 0
	if (pAdap->set_information_complete_flag)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Set information complete!\n");
		pAdap->set_information_complete_flag = FALSE;
		NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
		pAdap->dsp_pending_count = 0;
	}
#endif

	//set the two flag to prevent from sending
	pAdap->bStarveMac = TRUE;
	pAdap->wlan_attr.gdevice_info.tx_disable = TRUE;
	//don't set the flag at here, This will cause mismatch disconnect event in 1c_reset of HCT test. 
	//if((pAdap->link_ok == LINK_OK)
	//	||(pAdap->link_is_active == NdisMediaStateConnected))
	//{
	//	Adap_SetLink(pAdap,LINK_FALSE);
	//}


#ifdef ROAMING_SUPPORT
	pAdap->reconnect_status = NO_RECONNECT;
#endif //#ifdef ROAMING_SUPPORT
	


	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"DSP_Reset Enter while halt!\n");
		return STATUS_SUCCESS;//Justin: 080304 //TDSP_STATUS_HARD_ERRORS;//justin: 1127
	}	

	if(pAdap->driver_state == DSP_SYS_RESET
		||pAdap->driver_state == DSP_HARDWARE_RESET)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"DSP_Reset in progress!\n");
		return STATUS_RESET_IN_PROGRESS;
	}

	if(Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_REQJOIN_8051)
		||Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_HARDWARE_RESET))//Justin:080613.	No need to create is already exist.
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"DSP_Reset:	return because already in task!\n");
		return STATUS_SUCCESS;
	}	
	

	Task_RemoveAllMngFrameTask((PDSP_TASK_T)pAdap->ppassive_task);
	if(STATUS_SUCCESS != Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
							DSP_TASK_EVENT_REQJOIN_8051,
							DSP_TASK_PRI_HIGH,
							NULL,	
							0))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Dsp_Reset create task fail\n");		
		Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_WORK);//Justin:	080325.	reset state if fail to create task...
		return STATUS_NOT_RESETTABLE ;
	}
	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Dsp_Reset routine out\n");							
	return STATUS_PENDING;
}







/* self-defined function */

/*******************************************************************
 *   Adap_Init_Workstate
 *   
 *   Descriptions:
 *      This routine initialize some members saved into the adapter context.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_Init_Workstate(PDSP_ADAPTER_T pAdap)
{
	/* Set halt flag */
	//pAdap->halt_flag = FALSE;
	//combo
	Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_INIT);
	/* Set suprised-remove flag */

	pAdap->reconnect_after_reset_interrupt_flag = FALSE;
	pAdap->wlan_attr.need_set_pwr_mng_bit_flag = FALSE;

	//Jakio20070411: add default tx_power_level
	//00--25%, 01--50%, 10--75%, 11--100%
	//pAdap->txpwr_level = TX_POWER_75_PERCENT;

	/* Initialize the event that for some mng flow */
//wumin	sc_event_init(&pAdap->starve_event);
	/* Initialize the event that notify all rx IRPS is idle. */
//wumin	sc_event_init(&pAdap->rx_idle_event);
	/* Initialize the event that notify all int IRPS is idle. */
//wumin	sc_event_init(&pAdap->int_idle_event);
    /* Initialize the event that notify fw is ok. */
    sc_event_init(&pAdap->tx_fm_event);
    /* Initialize the event that verify the download. */
   // sc_event_init(&pAdap->rx_fm_event);
	//Jakio 2007.01.30
	/*Initialize the event that notify 8051 has read a mac register*/
	sc_event_init(&pAdap->DspReadEvent);
   	 /*Justin: 070425; init send ok event and set to indicate that there is no packet being dma*/
//	sc_event_init(&pAdap->DspSendOkEvent);
//	sc_event_set(&pAdap->DspSendOkEvent);
	//pAdap->DspSendOkFlag = 1;
    /**/

	/* The initial value of rssi is zero. */
	pAdap->rssi = 0;
	sc_memory_set(&pAdap->rssi_update, 0, sizeof(RSSI_UPDATE_T));
	/* Set scan flag */
	pAdap->scanning_flag = FALSE;
	/* Set link ok flag */
	pAdap->link_ok = LINK_FALSE;
	/* Set link*/

	/* Set basic reset type */
	pAdap->reset_type = RESET_TYPE_NOT_SCAN;
	pAdap->reset_sub_type = 0;

#ifdef ROAMING_SUPPORT
	//pAdap->can_do_reconnect_flag = FALSE;
	pAdap->reconnect_status = NO_RECONNECT;
#endif

	/* Clear information complete flag */
	pAdap->set_information_complete_flag = FALSE;
	/* First running flag */
	pAdap->first_run_flag = TRUE;
	/* Set OS power state to D0 (default value) */

    // TODO:Jackie
	//pAdap->power = NdisDeviceStateD0;
	/* The usb mode is set usb2.0 when initialized. */
	pAdap->usb_mode = USB_MODE_USB20;
	/* Beacon frame sent flag is set FALSE */
	pAdap->beacon_send_ok = FALSE;
	/* ATIM frame sent flag is set FALSE */
	pAdap->atim_send_ok = FALSE;
	/* We use WZC instead of WCU  to control driver when initialized. */
	pAdap->use_wcu_flag = FALSE;
	/* Set False when initialized. */
	pAdap->is_oid_scan_begin_flag = FALSE;
	/* Set zero when initialized. */
	// pAdap->detect_current_rx_total_packet = 0;
	/* Set the default firmware version. */
	pAdap->firmware_version = 0;
	/* Set the default system timer active flag. */
	pAdap->systimer_active_flag = TRUE;
	/* Set the default need fast check media flag. */
	pAdap->need_fast_checkmedia_flag = FALSE;
	pAdap->must_indicate_uplevel_flag = FALSE;
	/* Set the in_doing_internalreset_flag. */
	// pAdap->in_doing_internalreset_flag = FALSE;
	/* Set the current filter as zero(blocking all packet). */
	pAdap->packet_filter = 0;
	/* Zero all counters. */
	sc_memory_set(&pAdap->mib_stats, 0, sizeof(MIB_STATISTIC_T));
	/* Empty the int IRP list. */
//	QueueInitList(&pAdap->tx_irp_list);

	/* Alloc spin lock,which used to protect tx, rx and so on. */
	sc_spin_lock_init(&pAdap->lock);
	sc_spin_lock_init(&pAdap->fw_lock);

	/* Allocate memory for tx cached. */
	// DSP_ALLOC_MEM(&pAdap->ptx_cached, DSP_TX_BUF_SIZE);

	pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	

	if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
		pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;
	
	//Adap_Init_Wlan_Attr(&pAdap->wlan_attr);
	pAdap->mac_init_done = FALSE;
	//set 3dsp chip into WLAN only mode
	pAdap->chip_function = CHIP_FUNCTION_WLAN_ONLY;
	pAdap->scan_result_new_flag = 0;

    // TODO:Jackie
	//pAdap->ps_state = NdisDeviceStateD0;

	pAdap->rx_error_count = 0;
	pAdap->tx_error_count = 0;

	//combo
	pAdap->dsp_fw_mode  = INT_SUB_TYPE_RESET_WITH_SINGLE_MODE;
	pAdap->OID_not_allow_flag = FALSE;
}

/*******************************************************************
 *   Adap_InitReset
 *   
 *   Descriptions:
 *      This routine initialize some members saved into the adapter context
 *      when initialized.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_InitReset(PDSP_ADAPTER_T pAdap)
{
	/* Set halt flag */
	//pAdap->halt_flag = FALSE;
	//combo
	Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_INIT);
	
//wumin	sc_event_reset(&pAdap->starve_event);
	
	/* Reset event. */
//wumin	sc_event_reset(&pAdap->rx_idle_event);
	/* Reset event. */
//wumin	sc_event_reset(&pAdap->int_idle_event);
    /* Reset event. */
	sc_event_reset(&pAdap->tx_fm_event);
    /* Reset event. */
	//sc_event_reset(&pAdap->rx_fm_event);
	/* The initial value of rssi is zero. */
	sc_memory_set(&pAdap->rssi_update, 0, sizeof(RSSI_UPDATE_T));
	pAdap->rssi = 0;
	/* Set scan flag */
	pAdap->scanning_flag = FALSE;
	/* Set False when initialized. */
	pAdap->is_oid_scan_begin_flag = FALSE;

	/* Set zero when initialized. */
	// pAdap->detect_current_rx_total_packet = 0;
#ifdef ROAMING_SUPPORT
	//pAdap->can_do_reconnect_flag = FALSE;
	pAdap->reconnect_status = NO_RECONNECT;
#endif
	
}

/*******************************************************************
 *   Adap_UnInitialize
 *   
 *   Descriptions:
 *      This routine release some member resources saved into the adapter context.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_UnInitialize(void* adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
		return;

	DBG_WLAN__ENTRY(LEVEL_TRACE, "Enter [%s] ! \n",__FUNCTION__);   
	
	if(pAdap->adap_released)
	{		
		DBG_WLAN__ENTRY(LEVEL_CRIT, "[%s] Adap Has been released early! \n",__FUNCTION__);   
		return;
	}
 
	/* Free spin lock */
	Adap_Set_Driver_State(pAdap,DSP_DRIVER_HALT);

	wlan_timer_kill(&pAdap->mng_timer);
	wlan_timer_kill(&pAdap->sys_timer);
//wumin	wlan_timer_kill(&pAdap->sending_timer);
//wumin	wlan_timer_kill(&pAdap->tx_watch_timer);
	wlan_timer_kill(&pAdap->tx_countermeasure_timer);
	wlan_timer_kill(&pAdap->periodic_timer);
	
//wumin	if (STATUS_TIMEOUT == sc_event_wait(&pAdap->rx_idle_event,100))	//Justin: edit from 10000(10s) to 1000( 1s)
//wumin    {
 //wumin       DBG_WLAN__MAIN(LEVEL_TRACE,"[%s]:! All rx IRPs were not accounted for\n",__FUNCTION__);
//wumin    }   
	// Wait for all of the Int IRPs to be completed . duration is 10s
//	if (STATUS_TIMEOUT == sc_event_wait(&pAdap->int_idle_event,100)) //Justin: edit from 10000(10s) to 1000( 1s)
//	{
//		DBG_WLAN__MAIN(LEVEL_TRACE,"[%s]:! All Int IRPs were not accounted for\n",__FUNCTION__);
//	}



   if(pAdap->fw_buf)
   {
        sc_memory_free(pAdap->fw_buf);
        pAdap->fw_buf = NULL;
   }
    
#if 0
	Rx_stop(pAdap);
	
	Int_CancelInt(pAdap);
#endif
	/* Reset management queue  */
//wumin	MngQueue_Reset((PDSP_MNG_QUEUE_T)pAdap->pmng_queue_list);
		
	Rx_stop(pAdap);	

	Rx_release(pAdap);
	
	Tx_Release(pAdap);	

	Int_CancelInt(pAdap);
	
	Int_Release(pAdap);
	
	/* If management module exists, release its resource */
	if (pAdap->pmanagment)
		Mng_Release(pAdap);

	/* Release ourselves thread resource */
	Task_Release(pAdap);

    UsbDev_Release(pAdap);
    sc_spin_lock_kill(&pAdap->lock);
	sc_spin_lock_kill(&pAdap->fw_lock);
    sc_event_kill(&pAdap->tx_fm_event);
 	sc_event_kill(&pAdap->set_key_event);
    sc_event_kill(&pAdap->DspReadEvent);
    sc_event_kill(&pAdap->is_8051_ready_event);

    pAdap->adap_released = TRUE;

	DBG_WLAN__ENTRY(LEVEL_TRACE, "Leave [%s]!\n",__FUNCTION__);   
}

/*******************************************************************
 *   Adap_FreeAdapterObject
 *   
 *   Descriptions:
 *      This routine releases all resources defined in the ADAPTER
 *      object and returns the ADAPTER object memory to the free pool.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID
Adap_Destroy(void * adapt )
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
		return;

	DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_FreeAdapterObject\n");

	//Jakio20070611: moved code here, should  release retry_list before heap_client
	//Jakio20070301: new policy for retry limit
//wumin	#ifdef NEW_RETRY_LIMIT
//wumin	if(pAdap->ptx_retry_list)
//wumin	{
//wumin		DspList_Release(pAdap, (PDSP_LIST_T)pAdap->ptx_retry_list);
//wumin		pAdap->ptx_retry_list = NULL;
//wumin	}
	
//wumin	#endif/*NEW_RETRY_LIMIT*/

	
//wumin#ifdef NEW_RETRY_LIMIT
//here release retry queue
//wumin{
//wumin	if (pAdap->ptx_retry_list)
//wumin	{
//wumin		DspList_Release(pAdap,(PDSP_LIST_T)pAdap->ptx_retry_list);
//wumin	}		
//wumin}
//wumin#endif
	
//wumin#if 0 // TODO:Jackie
//wumin	
//wumin	if (pAdap->prx_pool)
//wumin		RxPool_Release((NDIS_HANDLE)pAdap);
//wumin#endif //#if 0 // TODO:Jackie
	/* If managment queue exists, release its resources */
//wumin	if (pAdap->pmng_queue_list)
//wumin		MngQueue_Release(pAdap);
//wumin#if 0 // TODO:Jackie	
	/*If packet list exists, release its resources */
//wumin	if (pAdap->ptx_packet_list)
//wumin		PktList_Release(pAdap,(PDSP_PKT_LIST_T)pAdap->ptx_packet_list);
//wumin#endif //#if 0 // TODO:Jackie

//wumin#ifdef NDIS51_MINIPORT
//wumin	if (pAdap->ptx_packet_list)
//wumin		PktList_Release(pAdap,(PDSP_PKT_LIST_T)pAdap->ptx_cancel_packet_list);
//wumin
//wumin	if (pAdap->ptx_packet_list)
//wumin		PktList_Release(pAdap,(PDSP_PKT_LIST_T)pAdap->ptx_nocancel_packet_list);
//wumin#endif

	DBG_WLAN__MAIN(LEVEL_TRACE,"WLAN END \n");

	// free the adapter object itself
	sc_memory_free(pAdap);

	// zero dirver block
	//update it later
	//check it,woody
	//sc_memory_set(&DSPMiniportBlock,sizeof(DRIVER_BLOCK_T));

}


/*******************************************************************
 *   Adap_ParseAndSetHotKey
 *   
 *   Descriptions:
 *      Get HotKey from registry.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NDIS_STATUS_SUCCESS: return success.
 *      NDIS_STATUS_xxx: return unsuccessful.
 ******************************************************************/
 //#define BUS_REGISTRY_PARAMETERS_PATH L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\MFCARD" 
 
TDSP_STATUS Adap_ParseAndSetHotKey(PDSP_ADAPTER_T pAdap)
 {
    DBG_WLAN__INIT(LEVEL_TRACE,"[%s:] bus info is %x \n",__FUNCTION__,pAdap->pBusInterfaceData);
	if (sc_bus_get_hkey_flag(pAdap->pBusInterfaceData))
	{
		DBG_WLAN__INIT(LEVEL_TRACE,"Adap_ParseAndSetHotKey:    Supported HotKey !   \n");
		Basic_WriteRegByte(pAdap,0x5A,0x11C);
	}
	else
	{
		DBG_WLAN__INIT(LEVEL_TRACE,"Adap_ParseAndSetHotKey:    NOT Supported HotKey !   \n");					 	
		Basic_WriteRegByte(pAdap,0x50,0x11C);
	}

	 return STATUS_SUCCESS;
 }


/******************************************************************
 *   Adap_GetRegionDomain
 *
 *   Descriptions: 
 *      This routine will get region domain.
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 static const struct
{
	UINT32  ulChannel;
	UINT32  ulFrequency;
	UINT32  bIsUsedInAmericas;
	UINT32  bIsUsedInEurope;
	UINT32  bIsUsedInJapan;
	UINT32  bIsUsedInRestOfWorld;
}
						19	32		18		36
static_axLookupFreqByChannel_ABG[] = 
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
 ******************************************************************/
VOID Adap_GetRegionDomain(PDSP_ADAPTER_T pAdap, UINT8 by_io)
{

	static UINT8 back_region = 0xff;
/*
HKR, NDI\params\RegulatoryDomain, default,,"1"
HKR, NDI\params\RegulatoryDomain\enum, "0",,"US"
HKR, NDI\params\RegulatoryDomain\enum, "1",,"Europe"
HKR, NDI\params\RegulatoryDomain\enum, "2",,"Japan"
HKR, NDI\params\RegulatoryDomain\enum, "3",,"RestofWorld"
*/

	if (by_io)
	{
		back_region = pAdap->wlan_attr.regoin;
	}
	else
	{
		if (back_region != 0xff)
			pAdap->wlan_attr.regoin = back_region;
	}

	switch(pAdap->wlan_attr.regoin)
	{
		case 0x00:	//  FCC (US) channel 1-11 36-64
			DBG_WLAN__INIT(LEVEL_TRACE, "FCC (US)\n");
			pAdap->wlan_attr.channel_num=0x13;
			pAdap->wlan_attr.channel_len_of_g=0x0b;
			pAdap->wlan_attr.channel_len_of_a=0x08;

			pAdap->wlan_attr.channel_list[0]=0x01;
			pAdap->wlan_attr.channel_list[1]=0x02;
			pAdap->wlan_attr.channel_list[2]=0x03;
			pAdap->wlan_attr.channel_list[3]=0x04;
			pAdap->wlan_attr.channel_list[4]=0x05;
			pAdap->wlan_attr.channel_list[5]=0x06;
			pAdap->wlan_attr.channel_list[6]=0x07;
			pAdap->wlan_attr.channel_list[7]=0x08;
			pAdap->wlan_attr.channel_list[8]=0x09;
			pAdap->wlan_attr.channel_list[9]=0x0a;
			pAdap->wlan_attr.channel_list[10]=0x0b;
			pAdap->wlan_attr.channel_list[11]=0x24;
			pAdap->wlan_attr.channel_list[12]=0x28;
			pAdap->wlan_attr.channel_list[13]=0x2c;
			pAdap->wlan_attr.channel_list[14]=0x30;
			pAdap->wlan_attr.channel_list[15]=0x34;
			pAdap->wlan_attr.channel_list[16]=0x38;
			pAdap->wlan_attr.channel_list[17]=0x3c;
			pAdap->wlan_attr.channel_list[18]=0x40;
			break;

		case 0x01: //"1",,"Europe"	// ETSI (Europe) channel 1-13
			DBG_WLAN__INIT(LEVEL_TRACE, "ETSI \n");
			pAdap->wlan_attr.channel_num=0x20;
			pAdap->wlan_attr.channel_len_of_g=0x0d;
			pAdap->wlan_attr.channel_len_of_a=0x13;

			pAdap->wlan_attr.channel_list[0]=0x01;
			pAdap->wlan_attr.channel_list[1]=0x02;
			pAdap->wlan_attr.channel_list[2]=0x03;
			pAdap->wlan_attr.channel_list[3]=0x04;
			pAdap->wlan_attr.channel_list[4]=0x05;
			pAdap->wlan_attr.channel_list[5]=0x06;
			pAdap->wlan_attr.channel_list[6]=0x07;
			pAdap->wlan_attr.channel_list[7]=0x08;
			pAdap->wlan_attr.channel_list[8]=0x09;
			pAdap->wlan_attr.channel_list[9]=0x0a;
			pAdap->wlan_attr.channel_list[10]=0x0b;
			pAdap->wlan_attr.channel_list[11]=0x0c;
			pAdap->wlan_attr.channel_list[12]=0x0d;//			
			pAdap->wlan_attr.channel_list[13]=0x24;	//36
			pAdap->wlan_attr.channel_list[14]=0x28;
			pAdap->wlan_attr.channel_list[15]=0x2c;
			pAdap->wlan_attr.channel_list[16]=0x30;
			pAdap->wlan_attr.channel_list[17]=0x34;
			pAdap->wlan_attr.channel_list[18]=0x38;
			pAdap->wlan_attr.channel_list[19]=0x3c;
			pAdap->wlan_attr.channel_list[20]=0x40;
			pAdap->wlan_attr.channel_list[21]=0x64;
			pAdap->wlan_attr.channel_list[22]=0x68;
			pAdap->wlan_attr.channel_list[23]=0x6c;
			pAdap->wlan_attr.channel_list[24]=0x70;
			pAdap->wlan_attr.channel_list[25]=0x74;
			pAdap->wlan_attr.channel_list[26]=0x78;
			pAdap->wlan_attr.channel_list[27]=0x7c;
			pAdap->wlan_attr.channel_list[28]=0x80;
			pAdap->wlan_attr.channel_list[29]=0x84;
			pAdap->wlan_attr.channel_list[30]=0x88;
			pAdap->wlan_attr.channel_list[31]=0x8c;
			break;
			
		case 0x02: // "2",,"Japan" // MKK (Japan) channel 14 34 38 42 46
			DBG_WLAN__INIT(LEVEL_TRACE, "MKK \n");
			pAdap->wlan_attr.channel_num=0x12;
			pAdap->wlan_attr.channel_len_of_g=0x0e;
			pAdap->wlan_attr.channel_len_of_a=0x04;
/*			
			pAdap->wlan_attr.channel_list[0]=0x0e;//14			
			pAdap->wlan_attr.channel_list[1]=0x22;	//34
			pAdap->wlan_attr.channel_list[2]=0x26;
			pAdap->wlan_attr.channel_list[3]=0x2a;
			pAdap->wlan_attr.channel_list[4]=0x2e;
//*/
			
//*
			pAdap->wlan_attr.channel_list[0]=0x01;
			pAdap->wlan_attr.channel_list[1]=0x02;
			pAdap->wlan_attr.channel_list[2]=0x03;
			pAdap->wlan_attr.channel_list[3]=0x04;
			pAdap->wlan_attr.channel_list[4]=0x05;
			pAdap->wlan_attr.channel_list[5]=0x06;
			pAdap->wlan_attr.channel_list[6]=0x07;
			pAdap->wlan_attr.channel_list[7]=0x08;
			pAdap->wlan_attr.channel_list[8]=0x09;
			pAdap->wlan_attr.channel_list[9]=0x0a;
			pAdap->wlan_attr.channel_list[10]=0x0b;
			pAdap->wlan_attr.channel_list[11]=0x0c;
			pAdap->wlan_attr.channel_list[12]=0x0d;		
			pAdap->wlan_attr.channel_list[13]=0x0e;//14			
			pAdap->wlan_attr.channel_list[14]=0x22;	//34
			pAdap->wlan_attr.channel_list[15]=0x26;
			pAdap->wlan_attr.channel_list[16]=0x2a;
			pAdap->wlan_attr.channel_list[17]=0x2e;
//*/
			break;
		
		case 0x03:	//"3",,"RestofWorld"
		default:
			pAdap->wlan_attr.channel_num=0x24;
			pAdap->wlan_attr.channel_len_of_g=0x0d;
			pAdap->wlan_attr.channel_len_of_a=0x17;

			pAdap->wlan_attr.channel_list[0]=0x01;
			pAdap->wlan_attr.channel_list[1]=0x02;
			pAdap->wlan_attr.channel_list[2]=0x03;
			pAdap->wlan_attr.channel_list[3]=0x04;
			pAdap->wlan_attr.channel_list[4]=0x05;
			pAdap->wlan_attr.channel_list[5]=0x06;
			pAdap->wlan_attr.channel_list[6]=0x07;
			pAdap->wlan_attr.channel_list[7]=0x08;
			pAdap->wlan_attr.channel_list[8]=0x09;
			pAdap->wlan_attr.channel_list[9]=0x0a;
			pAdap->wlan_attr.channel_list[10]=0x0b;
			pAdap->wlan_attr.channel_list[11]=0x0c;
			pAdap->wlan_attr.channel_list[12]=0x0d;//13
			pAdap->wlan_attr.channel_list[13]=0x20;//36
			pAdap->wlan_attr.channel_list[14]=0x24;
			pAdap->wlan_attr.channel_list[15]=0x28;
			pAdap->wlan_attr.channel_list[16]=0x2c;
			pAdap->wlan_attr.channel_list[17]=0x30;
			pAdap->wlan_attr.channel_list[18]=0x34;
			pAdap->wlan_attr.channel_list[19]=0x38;
			pAdap->wlan_attr.channel_list[20]=0x3c;
			pAdap->wlan_attr.channel_list[21]=0x40;//64
			pAdap->wlan_attr.channel_list[22]=0x64;
			pAdap->wlan_attr.channel_list[23]=0x68;
			pAdap->wlan_attr.channel_list[24]=0x6c;
			pAdap->wlan_attr.channel_list[25]=0x70;
			pAdap->wlan_attr.channel_list[26]=0x74;
			pAdap->wlan_attr.channel_list[27]=0x78;
			pAdap->wlan_attr.channel_list[28]=0x7c;
			pAdap->wlan_attr.channel_list[29]=0x80;
			pAdap->wlan_attr.channel_list[30]=0x84;
			pAdap->wlan_attr.channel_list[31]=0x88;
			pAdap->wlan_attr.channel_list[32]=0x8c;//140
			pAdap->wlan_attr.channel_list[33]=0x95;//149
			pAdap->wlan_attr.channel_list[34]=0x99;
			pAdap->wlan_attr.channel_list[35]=0x9d;
			break;
	}
}

/*******************************************************************
 *   Adap_Init_Wlan_Attr
 *   
 *   Descriptions:
 *      This routine fill some default values into the wlan_attr member
 *      of adapter context.
 *   Arguments:
 *      pAttr: IN, the pointer of wlan_attr member of adapter context.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_Init_Wlan_Attr(PDSP_ATTR_T pAttr)
{
/* 1: here init for para can be get from registry*/
	//update it later
	//check it,woody
	//pAttr->tx_power = DSP_TX_POWER_DEFAULT;		//add by Justin
	//pAttr->auto_frag_threshold_enable = DSP_AUTO_FRAGTHRES_EN_DEFAULT;
	//pAttr->rts_threshold_enable = DSP_RTSTHRES_EN_DEFAULT;
	//	pAttr->force_B_only = ;

	//region   ,in registry 

	//set bit rate
	//set to auto rate
	pAttr->fallback_rate_to_use = FALLBACK_RATE_USE;
	pAttr->rate = OP_11_MBPS;
	pAttr->rate_using = pAttr->rate;

	//power mode ,in registry 
	pAttr->gdevice_info.ps_support = PSS_ACTIVE;

	//power save state, in registry
	pAttr->gdevice_info.ps_mode = PSS_ACTIVE;		// edit by Justin, current ps mode

	//tx power,in registry 
	pAttr->tx_power = DSP_TX_POWER_DEFAULT;		//add by Justin	


	//frag threshold, in registry
	pAttr->frag_threshold = DEFAULT_MAXFRAGLEN;

	//auto frag, in reigstry
	pAttr->frag_threshold_mode = 0;

	//set rts
	pAttr->rts_threshold = (UINT16)DSP_RTSTHRES_DEFAULT;

	//rts enable
	pAttr->rts_threshold_enable = FALSE;


	//ibss
	pAttr->channel_default = (UINT8)DSP_CHANNEl_DEFAULT;
	pAttr->gdevice_info.ibss_dot11_type = IEEE802_11_G;
	pAttr->atim_window = (UINT16)DSP_ATIMWIN_DEFAULT;
	pAttr->ibss_beacon_interval = (UINT16)DSP_BEACONINTV_DEFAULT;
	//do it later
	//how to set ibss/bss mode


/* 2:here init other parameter of wlan*/	

	pAttr->ethconv = WLAN_ETHCONV_RFC1042;
	pAttr->macmode = WLAN_MACMODE_ESS_STA;
	pAttr->hasjoined = JOIN_NOJOINED;

	pAttr->gdevice_info.dot11_type = IEEE802_11_G;//IEEE802_11_B;    //Justin, 070403
	pAttr->current_channel = 11;
	pAttr->wzc_open = 0; // default use WCU
	//set for cts function
	//open it for debugging cts to self
	
	pAttr->cts_to_self_config = CTS_SELF_ENABLE;
	pAttr->cts_en = CTS_SELF_DISABLE;
//	pAttr->erp_en = 0; //disable ERP

	sc_memory_set(pAttr->bssid,WLAN_BSSID_LEN,0x2);
	sc_memory_set(pAttr->dev_addr,WLAN_ADDR_LEN,0x2);
	pAttr->channel_num = 0;
	
	sc_memory_set(pAttr->ssid, 0, WLAN_SSID_MAXLEN);
	pAttr->aid = 0;
	//set encryption mode
	pAttr->gdevice_info.privacy_option = FALSE;//TRUE;
	pAttr->encryption_level = ENCRY_LEVEL_64BIT;

	//set auth mode, auth alg will be got from set extent wlan function
	pAttr->auth_mode = AUTH_MODE_OPEN;
	pAttr->auth_alg = AUTH_ALG_OPEN;    //do at init extend function

	// pAttr->encryption_level = ENCRY_LEVEL_128BIT;
	pAttr->frag_threshold = DEFAULT_MAXFRAGLEN;
	pAttr->rts_threshold = DEFAULT_RTSTHRESHOLD;


	pAttr->fallback_rate_to_use = FALLBACK_RATE_USE;
	pAttr->antenna_diversity = ANTENNA_DIVERSITY_DISABLE;
//	pAttr->rate = RATE_CCK_LONG_11M;
	pAttr->rate = OP_11_MBPS;
	pAttr->rts_rate = OP_11_MBPS;
	pAttr->used_basic_rate = OP_11_MBPS;
//	pAttr->ack_basic_rate = pAttr->used_basic_rate;

	
	sc_memory_set(&pAttr->key_map[0], 0, sizeof(wpakey_info_t) * REAL_KEY_ARRAY_SIZE);
	

	
//	sc_memory_set(pAttr->wep2_tk,(WLAN_WEP2_TKLEN));
//	sc_memory_set(pAttr->mic_key,(WLAN_MIC_KEY_LEN));
//	pAttr->wep_key_id = 0;
//	pAttr->wep_mark_bit = 0;
	pAttr->gdevice_info.privacy_option = FALSE;    //first we test disable mode
	pAttr->wep_mode = WEP_MODE_WEP;
	pAttr->scan_type = SCAN_TYPE_ACTIVE;
#ifdef DSP_WIN98
	pAttr->min_channel_time = DEFAULT_SCAN_MIDDLE_TIMEOUT; //100;//300; // 200ms
	pAttr->max_channel_time = DEFAULT_SCAN_MIDDLE_TIMEOUT; // 500ms
#else
	pAttr->min_channel_time = DEFAULT_SCAN_MIN_TIMEOUT;	//Justin: 0528
	pAttr->max_channel_time = DEFAULT_SCAN_MAX_TIMEOUT;
 #endif


	pAttr->beacon_interval = 0x64;
	pAttr->atim_window = 0;
	pAttr->auth_timeout = DEFAULT_AUTH_TIMEOUT;
	pAttr->asoc_timeout = DEFAULT_ASOC_TIMEOUT;
	pAttr->scan_timeout = DEFAULT_SCAN_TIMEOUT;
	pAttr->join_timeout = DEFAULT_JOIN_TIMEOUT;
	pAttr->rx_max_len = DEFAULT_RX_MAX_LEN;

	pAttr->gdevice_info.sifs = 5;


	sc_memory_set(&pAttr->retry_limit, 0, sizeof(RETRY_LIMIT_T));
	pAttr->retry_limit.short_retry_limit = DEFAULT_SHORT_RETRY_LIMIT;
	pAttr->retry_limit.long_retry_limit = DEFAULT_LONG_RETRY_LIMIT;
	pAttr->listen_interval = DEFAULT_LISTEN_INTERVAL;
	pAttr->atim_interrupt_timer = 2000; // 2ms
	pAttr->pre_tbtt_timer = 4000; // 2ms
	pAttr->tx_frame_lifetime = DEFAULT_TX_MSDU_LIFE_TIME;
	sc_memory_set(pAttr->tbtt_timer, 0, WLAN_TBTT_TIMER_LEN);
	sc_memory_set(pAttr->gen_timer, 0, WLAN_GENERAL_TIMER_LEN);
	pAttr->rx_beacon_capability = 0;
	pAttr->rx_beacon_ds_channel = 0;
	pAttr->rx_beacon_ibss_atim_wm = 0;
	pAttr->rx_average_beacon_threshold = DEFAULT_AVERAGE_THRESHOLD;
	sc_memory_set(&pAttr->cw_win, 0, sizeof(CW_WIN_T));
	pAttr->cw_win.cw_min = DEFAULT_CW_MIN;
	pAttr->cw_win.cw_max = DEFAULT_CW_MAX;
	pAttr->gdevice_info.preamble_type = SHORT_PRE_AMBLE;

	//delete init procedure for support rate
	//set_dot11_type() will work for init for support rate
	//at same time,ext_support_rate will be canced.
	pAttr->is_exist_ibss_b_sta = FALSE;
#if 0	
	//liuwu
	sc_memory_set(pAttr->support_rate,WLAN_MAX_RATE_SUPPORT_LEN);
	pAttr->support_rate_len = 0;
	sc_memory_set(pAttr->ext_support_rate,WLAN_MAX_EXT_RATE_SUPPORT_LEN);
	pAttr->ext_support_rate_len = 0;

	pAttr->support_rate_len = 4;
	pAttr->support_rate[0]=0x82;
	pAttr->support_rate[1]=0x84;
	pAttr->support_rate[2]=0x8b;
	pAttr->support_rate[3]=0x96;
	//Jakio:changed here
	pAttr->ext_support_rate_len = 8;
	//pAttr->ext_support_rate_len = 0;  // for IBSS self-started. WiFi need not 11g.
	pAttr->ext_support_rate[0]=0xc; //6M, not basic rate
	pAttr->ext_support_rate[1]=0x12; //9M, not basic rate
	pAttr->ext_support_rate[2]=0x18; //12M, not basic rate
	pAttr->ext_support_rate[3]=0x24; //18M, not basic rate
	pAttr->ext_support_rate[4]=0x30; //24M, not basic rate
	pAttr->ext_support_rate[5]=0x48; //36M, not basic rate
	pAttr->ext_support_rate[6]=0x60; //48M, not basic rate
	pAttr->ext_support_rate[7]=0x6c; //54M, not basic rate

#endif		
	//liuwu
//	pAttr->channel_default=0x1;
	pAttr->channel_default=0xb;
	pAttr->channel_num=0xe;
/*	
	pAttr->channel_len_of_g=0x0e;	//edit by justin
	pAttr->channel_len_of_a=0x0;
//  pAttr->channel_list[0]=0x1;
	pAttr->channel_list[0]=0x1;
	pAttr->channel_list[1]=0x02;
	pAttr->channel_list[2]=0x03;
	pAttr->channel_list[3]=0x04;
	pAttr->channel_list[4]=0x05;
	pAttr->channel_list[5]=0x06;
	pAttr->channel_list[6]=0x07;
	pAttr->channel_list[7]=0x08;
	pAttr->channel_list[8]=0x09;
	pAttr->channel_list[9]=0x0a;
	pAttr->channel_list[10]=0x0b;
	pAttr->channel_list[11]=0x0c;
	pAttr->channel_list[12]=0xd;
	pAttr->channel_list[13]=0xe;*/


	
	/* pAttr->ssid_len = 6;
	pAttr->ssid[0]='p';
	pAttr->ssid[1]='r';
	pAttr->ssid[2]='o';
	pAttr->ssid[3]='x';
	pAttr->ssid[4]='i';
	pAttr->ssid[5]='m'; */
		
	pAttr->ssid_len = 9;
	pAttr->ssid[0]='N';
	pAttr->ssid[1]='D';
	pAttr->ssid[2]='T';
	pAttr->ssid[3]='E';
	pAttr->ssid[4]='S';
	pAttr->ssid[5]='T';
	pAttr->ssid[6]='A'; 
	pAttr->ssid[7]='P'; 
	pAttr->ssid[8]='2';  
		
	
	// key, mic value, tsc and rsc
//	sc_memory_set(pAttr->wpa_pairwise_key,WLAN_WEP2_TKLEN);
//	sc_memory_set(pAttr->wpa_group_key,WLAN_WPA_MAX_GROUP * sizeof(WPA_GROUP_KEY_T));
	sc_memory_set(pAttr->wpa_pairwise_mic_tx, 0, WLAN_MIC_KEY_LEN);
	sc_memory_set(pAttr->wpa_pairwise_mic_rx, 0, WLAN_MIC_KEY_LEN);
	sc_memory_set(pAttr->wpa_group_mic_tx, 0, WLAN_MIC_KEY_LEN);
	sc_memory_set(pAttr->wpa_group_mic_rx, 0, WLAN_MIC_KEY_LEN*5);
	pAttr->wpa_group_key_index = 0;
	sc_memory_set(&pAttr->wpa_pairwise_tsc_tx_counter, 0, sizeof(WPA_TSC_T));
	sc_memory_set(&pAttr->wpa_pairwise_tsc_rx_counter, 0, sizeof(WPA_TSC_T));
	sc_memory_set(&pAttr->wpa_group_tsc_tx_counter, 0, sizeof(WPA_TSC_T));
	sc_memory_set(&pAttr->wpa_group_tsc_rx_counter, 0, sizeof(WPA_TSC_T));

	// support data for wpa
	pAttr->wpa_capability = 0;
	pAttr->wpa_listen_interval = pAttr->listen_interval;

	// key valid flag
	pAttr->wpa_pairwise_key_valid = 0;

	//Jakio: init sequence number 
	pAttr->seq_num = 0;
	//pAttr->gdevice_info.auto_corr_off = FALSE;
	//pAttr->gdevice_info.ap_associated= FALSE;
	//pAttr->gdevice_info.soft_doze= FALSE;
	sc_memory_set(&pAttr->gdevice_info.bbreg2023, 0, sizeof(BBREG2023_REG_T));
	pAttr->gdevice_info.slot_time = SLOT_TIME_LONG_DOT11G;

    pAttr->regoin = (UINT8)DSP_REGION_DEFAULT;
}



/*******************************************************************
 *   Adap_Cal_Capability
 *   
 *   Descriptions:
 *      This routine calculates some values according to some default
 *      value of wlan_attr member of adapter context and then fill them
 *      into the wlan_attr member of adapter context.
 *   Arguments:
 *      pAttr: IN, the pointer of wlan_attr member of adapter context.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_Cal_Capability(PDSP_ATTR_T pAttr)
{
	UINT32 i;

	pAttr->capability.ess = (pAttr->macmode == WLAN_MACMODE_ESS_STA) ? 1:0;
	pAttr->capability.ibss = (pAttr->macmode == WLAN_MACMODE_ESS_STA) ? 0:1;
	
	pAttr->capability.cf_pollable = 0;
	pAttr->capability.cf_pollreq = 0;

	pAttr->capability.privacy = (pAttr->gdevice_info.privacy_option == TRUE) ? 1:0;

	pAttr->capability.short_preamble = (pAttr->gdevice_info.preamble_type == SHORT_PRE_AMBLE) ? 1:0;
	pAttr->capability.pbcc = 0;
	pAttr->capability.channel_agility = 0;
	pAttr->capability.short_slot_time = (pAttr->gdevice_info.slot_time == SLOT_TIME_SHORT_DOT11G) ? 1:0;
	pAttr->capability.dsss_ofdm = 0;
	pAttr->capability.reserved1 = 0;
	pAttr->capability.reserved2 = 0;
	pAttr->capability.reserved3 = 0;
	
	// pAttr->tx_beacon_length = 47 + pAttr->support_rate_len + pAttr->ssid_len;
	
	// ebit = CaculateExtBitAndFragLen(pAttr->tx_beacon_length + 4, pAttr->txmode_beacon, &txtime);
	//ebit = 0;
	//pAttr->tx_beacon_time = ((UINT16)txtime | ((UINT16)ebit << 15));
	
	// ebit = CaculateExtBitAndFragLen(pAttr->tx_beacon_length + 4, pAttr->txmode_others, &txtime);
	//ebit = 0;
	//pAttr->tx_probe_time = ((UINT16)txtime | ((UINT16)ebit << 15));
	
	pAttr->beacon_window = pAttr->beacon_interval * 100; // beacon interval / 10 * 1000 microseconds
	
		
#ifndef DSP_WIN98
	if (pAttr->macmode == WLAN_MACMODE_IBSS_STA)
	{
		for(i=0;i<ETH_LENGTH_OF_ADDRESS;i++)
		{
			pAttr->bssid[i] = sc_get_random_byte();
		}

		pAttr->bssid[0] &= 0xfe; 
		pAttr->bssid[0] |= 0x02; 
	}
#endif
	
	sc_memory_set(pAttr->tsf, 0, WLAN_TSF_LEN);
}




/*******************************************************************
 *   Adap_SetResetType
 *   
 *   Descriptions:
 *      This routine set the reset type. 
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *      type: IN, the reset type to be set.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_SetResetType(PDSP_ADAPTER_T pAdap, UINT8 type,UINT32 sub_type)
{
	pAdap->reset_type = type;
	pAdap->reset_sub_type = sub_type;
	DBG_WLAN__MLME(LEVEL_TRACE, "reset type = %x, sub type = %x \n",pAdap->reset_type,pAdap->reset_sub_type);
}


/*
	the routine check if the rate is OFDM rate//11g rate
 *   Return Value:
 *      TRUE if OFDM rate
 *      FALSE if not OFDM rate	
*/
static inline BOOLEAN Adap_IsOFDMRates(
	   UINT8 rate)
{
	UINT8 value;
	value = rate & 0x7f;   
	return !((value == 0x02 ) || (value == 0x04 ) || (value == 0x0b ) ||(value == 0x16));
}

/*******************************************************************
 *   Adap_CheckOFDMSupport
 *   
 *   Descriptions:
 *      This routine will check support rates and extention support rates
 *      and determine if it is OFDM		////has 802.11g rates.
 *   Arguments:
 *      sup_rate: IN, the support rate
 *      sup_rate_len: IN, the support rate length
 *      ext_sup_rate: IN, the extention support rate
 *      ext_sup_rate_len: IN, the extention support rate length
 *   Return Value:
 *      TRUE if OFDM Support//		has 802.11g rates.
 *      FALSE if OFDM not support	//has not 802.11g rates.
 ******************************************************************/
BOOLEAN Adap_CheckOFDMSupport(
						   PUINT8 sup_rate,
						   UINT8 sup_rate_len,
						   PUINT8 ext_sup_rate,
						   UINT8 ext_sup_rate_len
						   )
{
	UINT8 j;
	
	for (j = 0; j < sup_rate_len; j++)
	{
		//if (((*(sup_rate + j) | 0x80) > (UINT8)0x8b) && ((*(sup_rate + j)  | 0x80) != (UINT8)0x96))
		if(Adap_IsOFDMRates(*(sup_rate + j) ))
			return TRUE;
	}
	for (j = 0; j < ext_sup_rate_len; j++)
	{
		//if (((*(ext_sup_rate + j) | 0x80) > (UINT8)0x8b) && ((*(ext_sup_rate + j) | 0x80) != (UINT8)0x96))
		if(Adap_IsOFDMRates(*(ext_sup_rate + j) ))
			return TRUE;
	}

	return FALSE;
}



#if 1

TDSP_STATUS Adap_SoftReset(PDSP_ADAPTER_T pAdap)
{
    //TDSP_STATUS Status;
    //UINT8 tmpchar;
    //UINT32 tmpcounts,icounts;
    //UINT8 is_hasjoined;

    pAdap->bStarveMac = TRUE;
    
    /* Cancel  timers. */
//wumin    wlan_timer_kill(&pAdap->sending_timer);
//wumin	wlan_timer_kill(&pAdap->tx_watch_timer);
    wlan_timer_stop(&pAdap->tx_countermeasure_timer);
    wlan_timer_stop(&pAdap->sys_timer);
    wlan_timer_stop(&pAdap->mng_timer);


    sc_sleep(1000);

    //Clear some parameters for managment module.
    Mng_ClearPara(pAdap);
    
    // TODO: pls Justin check the flow to verify whether the rx module has resources to reset
    
    //Jakio20070413: reset mng list, pkt list and retry list
//wumin    MngQueue_Reset(pAdap->pmng_queue_list);


    Tx_Cancel_Data_Frm(pAdap);
	
	Tx_Reset(pAdap);
//wumin        PktList_Reset(pAdap->ptx_packet_list);
    
    Vcmd_Set_Encryption_Mode(pAdap);
	
    pAdap->bStarveMac = FALSE;

	Tx_Send_Next_Frm(pAdap);
	
//wumin wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);    
    /* Set system timer */    // wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);
    wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
    
    return STATUS_SUCCESS;	
	
	
}

//Justin:090107.	to get 8051 running loop through interrupt
//return:		TRUE ---- 8051 in miniloop
BOOLEAN Adap_is8051InMiniLoop(PDSP_ADAPTER_T pAdap)
{
	UINT32	ulLoop = 1000;
	DSP_GET_8051_STATUS info;
	
	info.type = CMD_GET_8051Status;
	pAdap->is8051InMiniLoop = 0xFF;
	
	if(STATUS_SUCCESS != Vcmd_Send_API(pAdap,(PUINT8)&info, sizeof(DSP_GET_8051_STATUS)))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"CMD_GET_8051Status return error\n");

		return FALSE;
	}

	//wait 8051 give the status through an interrupt, and the result will be saved in pAdap->is8051InMiniLoop
	while((ulLoop--) && (pAdap->is8051InMiniLoop == 0xFF))
	{
		sc_sleep(1000);
	}

	DBG_WLAN__MAIN(LEVEL_TRACE,"CMD_GET_8051Status return %x\n",pAdap->is8051InMiniLoop);
	
	return ((pAdap->is8051InMiniLoop==1) ? TRUE:FALSE);
}


TDSP_STATUS Adap_HardwareReset(PDSP_ADAPTER_T pAdap, BOOLEAN sys_reset_flag)
{
	//PMNG_STRU_T     			pt_mng = pAdap->pmanagment;
	DSP_DRIVER_STATE_T		state_backup;
	UINT8 is_hasjoined;
	UINT8 uErrCode = 0;

	DBG_ENTER();
 #if 0
    UINT8 buff[0x200];
    UINT32 val;
    
    Basic_ReadBurstRegs(pAdap, buff, 0x180, 0x00);//256 + 128
    Adap_PrintBuffer(buff, 0x180);//256 + 128

    Tx_print_buffer_msdu(pAdap);
    val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_0_3);  
    DBG_WLAN__MAIN(LEVEL_TRACE,"WLS_MAC__BBREG_0_3 is %08X\n",val);
    val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);   
    DBG_WLAN__MAIN(LEVEL_TRACE,"[%s]:WLS_MAC__TX_FRAG_CNT is %08X\n",__FUNCTION__,val);
#endif
             
	//wumin: 090226 change status first.
	pAdap->hardware_reset_flag = TRUE;	
	state_backup = pAdap->driver_state;
	Adap_Set_Driver_State_Force(pAdap,DSP_HARDWARE_RESET);		//stop tx, oid

	is_hasjoined = pAdap->wlan_attr.hasjoined ;
	
	while(Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_TX_WATCH_TIMEOUT))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "REMOVE one tx timeout Task!\n");
	}
	    //holds current join status, then we can retrieve it latter

	Mng_BreakIfScanning(pAdap);	

	while(Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "REMOVE one Scan Task!\n");
		
	}
	
	    	//Justin:	090107.	before hardware reset, we must confirm that 8051 has been in miniloop.
		//				otherwise, 8051 in mainloop do many process, such as rx, int or else. these maybe cause
		//				hardware reset process wrong.
	if(!Adap_is8051InMiniLoop(pAdap))		//just return(do not reset hardware) if not in miniloop
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$$$$$$$$$Adap_HardwareReset return because 8051 not in miniloop  \n"); 

		//wumin : 090226 clear status.
		pAdap->hardware_reset_flag = FALSE;	
		Adap_Set_Driver_State_Force(pAdap,state_backup);	
	
		return STATUS_SUCCESS;
	}
		
	sc_wlan_netdev_destroy(pAdap);	//add by wanghk for Net device refresh, to infor to uplayer. 20090803

			/* Watch scanning */
#ifdef ROAMING_SUPPORT
	if ((pAdap->is_oid_scan_begin_flag)
		&& (pAdap->reconnect_status == NO_RECONNECT))
#else
	if(pAdap->is_oid_scan_begin_flag)
#endif
	{
			DBG_WLAN__MAIN(LEVEL_TRACE,"complete scan before do hardware reset!\n");
			pAdap->scan_watch_value = 0;
			pAdap->is_oid_scan_begin_flag = FALSE;
			pAdap->scanning_flag = FALSE;
			//NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
	}

		//combo
	if(Adap_Driver_isHalt(pAdap) )
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Do nothing in Adap_HardwareReset(), because adap has halt or been removed\n");
		
		//wumin : 090226 clear status.
		pAdap->hardware_reset_flag = FALSE;	
		Adap_Set_Driver_State_Force(pAdap,state_backup);	

		return STATUS_SUCCESS;
	}


	
	/* Cancel all timers. */
	wlan_timer_stop(&pAdap->mng_timer);
	wlan_timer_stop(&pAdap->sys_timer);
  	wlan_timer_stop(&pAdap->tx_countermeasure_timer);
	wlan_timer_stop(&pAdap->periodic_timer);
	Adap_InitTKIPCounterMeasure(pAdap);

    /* Reset event. */
    sc_event_reset(&pAdap->tx_fm_event);
    
  	//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 1 \n"); 
	    
	Tx_Reset(pAdap);
	    
	/* Don't send data any more. */
	Adap_SetConnection(pAdap,0);
	// pAdap->in_doing_internalreset_flag = TRUE;
	/* Set halt flag to notify receive module(bulk in) not processing any more */
	//wlan_timer_kill(&pAdap->sending_timer, &IsTimerCancelled);

	//wumin: 090218 remove duplicate hardware reset task
	while(Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_HARDWARE_RESET))
	{}
	  
	    //DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 2 \n"); 

#if 0
	    Vcmd_Set_8051_MinLoop(pAdap);
	    sc_sleep(100);	
#endif

	    //DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 3 \n"); 

	    /* Reset MAC and PHY */	/// copy from halt()
	    if(STATUS_SUCCESS != Adap_Set_SoftReset(pAdap, (UINT16)(MAC_MODULE_SOFT_RESET | PHY_MODULE_REG_RESET | PHY_MODULE_SOFT_RESET)))
	    {			
		uErrCode = 1;
		goto _hardware_reset_faile_; //return STATUS_FAILURE;
	    }
	    //DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 4 \n"); 
#if 0
		{//Justin: 0924		Bus Reset Enable 
			UINT8 uchar_1;
			uchar_1 = Basic_ReadRegByte(pAdap,REG_BICR1);
			if(uchar_1 & BIT4)
				DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@# REG_BICR1 busy \n"); 

			uchar_1 = Basic_ReadRegByte(pAdap,REG_BOCR2);
			if(uchar_1 & BIT4)
				DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@# REG_BOCR2 busy \n"); 
		}
#endif







//combo
   
    
    // Wait for all of the Rx IRPs to be completed . duration is 50ms
 //wumin   if (!sc_event_wait(&pAdap->rx_idle_event,500)) //Justin: internal reset must finished quickly as can do
 //wumin   {
  //wumin  	DBG_WLAN__MAIN(LEVEL_TRACE,"! All Rx IRPs were not accounted for");
  //wumin  	}
    
    // Wait for all of the Int IRPs to be completed . duration is 50ms
    //close it, because we never free int irp except halt flow
    //if (!sc_event_wait(&pAdap->int_idle_event,500)) //Justin: internal reset must finished quickly as can do
    //	{
    //    ASSERT(!"All Int IRPs were not accounted for");
    //	}
    

//	Tx_Stop(pAdap);

	Rx_stop(pAdap);
    
	//Int_CancelInt(pAdap);

	Int_SubmitInt(pAdap);

	
	//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 11 \n"); 

	Adap_Set_Usbbus_Enable_Reset(pAdap);


	//don't drop int, in fact we didn't recycle the irp
	//Adap_DispatchInterruptIrps(pAdap,1);

  	if(Vcmd_NIC_INTERRUPT_DISABLE(pAdap)!= STATUS_SUCCESS)
  	{
  		DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset : int disable fail \n"); 
	uErrCode = 2;
  		 goto _hardware_reset_faile_;
  	}

	if(Vcmd_CardBusNICReset(pAdap)  != STATUS_SUCCESS)
	{
		//Adap_UnInitialize(pAdap);
		//Adap_FreeAdapterObject(pAdap);
		//return STATUS_FAILURE;
		uErrCode = 3;
		DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset : bus reset fail \n"); 
		 goto _hardware_reset_faile_;
	}

	if(STATUS_SUCCESS !=Adap_GetDspFwCodesAndDownload(pAdap))
	{
		uErrCode = 4;
		DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset. download dsp fail  \n"); 
	        goto _hardware_reset_faile_;
	}


	 /* Set download-ok flag to 8051 */
	//Vcmd_Set_Firmware_Download_Ok(pAdap))
	
  
	//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 12 \n"); 
       pAdap->wlan_attr.chipID = Adap_get_chipid(pAdap);

       if(STATUS_SUCCESS!= Adap_reset_dsp_chip(pAdap))
	{
		uErrCode = 5;
		DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset. reset dsp fail  \n"); 
	       goto _hardware_reset_faile_;//return STATUS_FAILURE;
	}
	//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 13 \n"); 

  
        if(STATUS_SUCCESS != Vcmd_Set_Encryption_Mode(pAdap))
       {
		uErrCode = 6;
       	DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset.set encryption fail  \n"); 
              goto _hardware_reset_faile_;//return STATUS_FAILURE;
        }
	// DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 14 \n"); 
	
	 if(STATUS_SUCCESS != Adap_w2lan_resetting(pAdap))
        {
		uErrCode =7;
        	DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset. w2lan reset fail  \n"); 
             goto _hardware_reset_faile_;//return STATUS_FAILURE;
        }
 	// DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 15 \n"); 

    
	    /*Jakio2006-11-24: after reset dsp chip, txfifo has been cleared
	    * so we should clear retry_list and bak_list		  */
		//Jakio20070301: new policy for retry limit
#ifdef NEW_RETRY_LIMIT
	//RetryList_FreeInterData((NDIS_HANDLE)pAdap, (PDSP_RETRY_LIST_T)pAdap->ptx_retry_list);
	//first close it ,woody
#endif/*NEW_RETRY_LIMIT*/

#if 1		//Justin: 0907.   don't discard pakets in queue, continue to send after reset
			//justin: 1114.	discard packets in queue, otherwise, it will fail the following packet to send
    //Clear some parameters for managment module.
//    Mng_ClearPara(pAdap);
// Justin: 071120.	continue to mng after reset done. otherwise, fail to oids setting    
    //Jakio20070413: reset mng list, pkt list and retry list
//    MngQueue_Reset(pAdap->pmng_queue_list);

    Tx_Cancel_Data_Frm(pAdap);


//wumin    PktList_Reset(pAdap->ptx_packet_list);
    
#ifdef NEW_RETRY_LIMIT
	// DspList_Reset((PDSP_LIST_T)pAdap->ptx_retry_list);
    //RetryList_Reset(pAdap->ptx_bak_list);
#endif /*NEW_RETRY_LIMIT*/

#endif

	//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 16 \n"); 
#if 0	//Justin: 0904.   for test only.   print dispatch irp number of tx, rx, int
{//
	DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$rx irp = %d, int irp = %d, \n",pAdap->rx_irps_outstanding,pAdap->int_irps_outstanding);
}
#endif 

#if 0
    /* Set download-ok flag to 8051 */
    if(STATUS_SUCCESS !=Vcmd_Set_Firmware_Download_Ok(pAdap))
	   {
		uErrCode = 8;
        goto _hardware_reset_faile_;
	   }
	DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 17 \n"); 
#endif

   sc_sleep(400);
    pAdap->wlan_attr.hasjoined = is_hasjoined;		//retrive connect status

	Task_RemoveAllMngFrameTask((PDSP_TASK_T)pAdap->ppassive_task);

	if(pAdap->driver_state != DSP_SUPRISE_REMOVE)
	{
		Adap_Set_Driver_State_Force(pAdap, DSP_DRIVER_WORK);

		 Rx_restart(pAdap);
		 
//		Tx_Restart( pAdap);
		
		Vcmd_set_next_state(pAdap,DEV_ACTIVE, 0,0);    
		
		sc_sleep(1); 
		
	      /* Set download-ok flag to 8051 */
	     if(STATUS_SUCCESS !=Vcmd_Set_Firmware_Download_Ok(pAdap,TRUE))
		   {
		uErrCode = 9;
	        goto _hardware_reset_faile_;
		   }
		//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset 17 \n"); 


	  	  //begin tx
		Tx_Send_Next_Frm(pAdap);
	  //wumin	  wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);    
	    	/* Set system timer */    // wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);
	  	  wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
		//#ifdef CHECK_BULK_STALL_ENABLE
		wlan_period_timer_start(&pAdap->periodic_timer,TIMER_PERIODIC_DELAY);
		///#endif /*CHECK_BULK_STALL_ENABLE*/    

		//DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset ok \n"); 

	//Re-write beacon here instead of in Adap_w2lan_resetting()
	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)	//ibss mode.	justin:	080506. must recover this after hardware reset.
	{
		Mng_MakeBeaconOrProbersp(pAdap,WLAN_FSTYPE_BEACON,NULL);
	}
			{
				UINT8 verTemp[10];
			DBG_WLAN__MAIN(LEVEL_TRACE,"driver version = %s\n",USB_WLAN_VERSION);
			sc_memory_copy(verTemp,(PUINT8)&pAdap->DSP8051_FW_version,sizeof(UINT32));
			verTemp[4] = 0;
			DBG_WLAN__MAIN(LEVEL_TRACE,"8051 version = %02d.%02d.%02d.%02d\n",verTemp[3],verTemp[2],verTemp[1],verTemp[0]);
			sc_memory_copy(verTemp,(PUINT8)&pAdap->DSP_FW_version,sizeof(UINT32));
			verTemp[4] = 0;
			DBG_WLAN__MAIN(LEVEL_TRACE,"dsp version = %02d.%02d.%02d.%03d\n",verTemp[3],verTemp[2],verTemp[1],verTemp[0]);
			}
	}
	
	//Justin:	081107.	for WPA/WPAPSK or WPA2/WPA2PSK, we must disconnect after hardware_reset. Because the TSC must be clear by hardware_reset.

	//				for open or share(none encryption or WEP), it can run well after hardware_reset.


	if (pAdap->wlan_attr.auth_mode >= Ndis802_11AuthModeWPA)//wpa or wpa2
	{
		Adap_SetLink(pAdap,LINK_FALSE); 
		Adap_UpdateMediaState(pAdap,0);
	}


	pAdap->bStarveMac = FALSE;
	pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;

	pAdap->hardware_reset_flag = FALSE;	

	
	DBG_EXIT();

	sc_wlan_netdev_setup(pAdap);	//add by wanghk for Net device refresh, to infor to uplayer. 20090803
	
    return STATUS_SUCCESS;	
	
_hardware_reset_faile_:

	if (pAdap->run_in_halt_flag)
	{
		Adap_Set_Driver_State_Force(pAdap,state_backup);
	}
	else
	{
     Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_HALT);
	}
     pAdap->hardware_reset_flag = FALSE;	
	DBG_WLAN__MAIN(LEVEL_TRACE,"@@@@@@@@#$$$Adap_HardwareReset fail %d %d\n", pAdap->driver_state, pAdap->hardware_reset_flag); 
   return STATUS_FAILURE;
	
}	

#endif



//Justin: for dsp_reset only
VOID Adap_SysReset(PDSP_ADAPTER_T pAdap)
{

//	BOOLEAN IsTimerCancelled;
//	TDSP_STATUS Status;
//	UINT8 firmware_status, tmpchar;
//	UINT32 tmpcounts,icounts;
//	PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SysReset Enter!\n");
	//combo
	if(Adap_Driver_isHalt(pAdap) || pAdap->driver_state == DSP_HARDWARE_RESET)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Do nothing in Adap_InternalReset(), because adap has halt or been removed\n");
		return;
	}

#if 0
		if (pAdap->set_information_complete_flag || pt_mng->oidscanflag)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SysReset: Set information complete!\n");
			pAdap->set_information_complete_flag = FALSE;
			NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
			pAdap->dsp_pending_count = 0;
		}
#endif

	/* Don't send data any more. */
//Justin: 0731. can not disconnect after reset if already connected.	
/*	Adap_SetConnection(pAdap,0);
	Adap_UpdateMediaState(pAdap, 0);
	*/
#if 0
		{
			UINT32 aa[4];
			UINT32 bb;
			if(pAdap->debug_val !=0)
			{
				pAdap->debug_val =0;
				Basic_ReadBurstRegs(pAdap,(PUINT8)aa,4,0x30);
				Basic_ReadBurstRegs(pAdap,(PUINT8)(aa+1),4,0x34);
				Basic_ReadBurstRegs(pAdap,(PUINT8)(aa+2),4,0x38);
				Basic_ReadBurstRegs(pAdap,(PUINT8)(aa+3),4,0x3c);
				bb = aa[2];				
			}
		}
#endif
	//MiniportReset can reset the parameters of its NIC.
	Adap_HardwareReset(pAdap,TRUE);
 #if 0		
 	Adap_InitAttrForReset(&pAdap->wlan_attr);
	Adap_Init_Wlan_Attr(&pAdap->wlan_attr);
	
	wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);
	/* Set system timer */
	// wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);
	wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
#ifdef CHECK_BULK_STALL_ENABLE
	wlan_period_timer_start(&pAdap->periodic_timer,TIMER_PERIODIC_DELAY);
#endif /*CHECK_BULK_STALL_ENABLE*/

#endif

	
	pAdap->bStarveMac = FALSE;
	pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;

	//combo
	{
//		Task_RemoveAllExistTask((PDSP_TASK_T)pAdap->ppassive_task);
		//NdisMResetComplete(pAdap->dsp_adap_handle,STATUS_SUCCESS,FALSE);
			pAdap->reset_complete_flag = TRUE;
	}

#if 0
	//Justin: 1113
	if(pAdap->reconnect_after_reset_interrupt_flag)
	{
DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$$$$$$$$$dis connect\n"); 
		Mng_BssDisAssoc(pAdap);
		sc_sleep(200);
		Adap_SetResetType(pAdap,RESET_TYPE_KNOWN_SSID_SCAN,0);
		Adap_InternalReset(pAdap);
DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$$$$$$$$$start join....\n"); 
		pAdap->reconnect_after_reset_interrupt_flag = FALSE;
	}
#endif

}

/*******************************************************************
 *   Adap_InternalReset
 *   
 *   Descriptions:
 *      This routine resets the card. 
 *   Arguments:
 *      pAdap: IN, the pointer of adapter context
 *   Return Value:
 *      NONE
 ******************************************************************/

VOID Adap_InternalReset(PDSP_ADAPTER_T pAdap)
{

//	BOOLEAN IsTimerCancelled;
//	TDSP_STATUS Status;
//	UINT8 firmware_status, tmpchar;
//	UINT32 tmpcounts,icounts;
//	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
//	UINT32		ulCompletePacketCounter = 0;

	DBG_WLAN__MAIN(LEVEL_TRACE,"InternalReset Enter, RESET TYPE = %x!\n",pAdap->reset_type);


	wlan_timer_stop(&pAdap->sys_timer);
	wlan_timer_stop(&pAdap->periodic_timer);
	//combo
	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Do nothing in Adap_InternalReset(), because adap has halt or been removed\n");
		return;
	}

	// Justin: 0716
	if(STATUS_SUCCESS!=Adap_WriteMacAddress(pAdap,pAdap->current_address))
	{	
		DBG_WLAN__MAIN(LEVEL_ERR,"Adap_WriteMacAddress FAILURE\n");
		return;
	}
		
	

	/* Don't send data any more. */
	Adap_SetConnection(pAdap,0);

#if 1//Justin: 1205.   discard pakets in queue.	Otherwise, it may occur that some packets pending in queue not complete anymore.
  
	Tx_Cancel_Data_Frm(pAdap);

#if 0
	if (pAdap->reset_type == (UINT8)RESET_TYPE_DO_JOIN_TILL_BEACON)
	{
		UINT32 loop = 1000;
		UINT32 reg_value;
		
		// 1. wait all packet in tx fifo have been send. otherwise, regard as dsp hang
		//Adap_Set_Driver_State(pAdap,DSP_STOP_TX);
		Adap_SetLink(pAdap,LINK_FALSE);//stop tx

		while(loop)
		{
			reg_value = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);
			reg_value &=0x000000ff;
			if(reg_value == 0)
			{
				DBG_WLAN__MAIN(LEVEL_TRACE,"internalreset while tx count = 0 \n");
				break;
			}
			if(loop == 400)
				Adap_request_flush_tx(pAdap);
			
			loop--;
		}
		if(loop == 0)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"internalreset, but can't find tx count = 0 \n");
			Adap_SetResetType(pAdap, RESET_TYPE_NOT_SCAN,0);

			Adap_HardwareReset(pAdap, FALSE);
/*
			if(STATUS_SUCCESS != Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
									DSP_TASK_EVENT_HARDWARE_RESET,
									DSP_TASK_PRI_HIGH,
									NULL,	
									0))
			{
				NULL;
			}
			*/
			return;
		}
		
	}
#endif
   // PktList_Reset(pAdap->ptx_packet_list);
    
#ifdef NEW_RETRY_LIMIT
	    Tx_Reset_Retry_Queue(pAdap);
#endif /*NEW_RETRY_LIMIT*/

#endif
	
//   	wlan_timer_kill(&pAdap->mng_timer, &IsTimerCancelled);

	Mng_InitParas(pAdap);	

#if 0	//Justin: print set ssid info
{
	PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	
	DBG_WLAN__MAIN(LEVEL_TRACE,"***********************\n");
	DBG_WLAN__MAIN(LEVEL_TRACE,"internal reset ssid len = 0x%02X\n",pt_mng->usedbssdes.ssid[0]);
	//Adap_PrintBuffer(pSsidStruct->Ssid,pSsidStruct->SsidLength);
	DBG_WLAN__MAIN(LEVEL_TRACE,"ssid = %02x %02x %02x %02x %02x %02x %02x %02x\n",
			pt_mng->usedbssdes.ssid[0], pt_mng->usedbssdes.ssid[1], pt_mng->usedbssdes.ssid[2],pt_mng->usedbssdes.ssid[3],
			pt_mng->usedbssdes.ssid[4], pt_mng->usedbssdes.ssid[5],pt_mng->usedbssdes.ssid[6],pt_mng->usedbssdes.ssid[7]);
	DBG_WLAN__MAIN(LEVEL_TRACE,"***********************\n");
}
#endif

	// Call liuwu's managment function here
	// TODO: start management without scan
	if (pAdap->reset_type == (UINT8)RESET_TYPE_NOT_SCAN)
	{
		Mng_Reset(pAdap);
	}
	// TODO: start management with scan
	/*
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_WITH_SCAN)
	{
		Adap_SetLink(pAdap,0);
		Mng_Start_Win2000(pAdap);
	}*/
	// TODO: start management with known information(ssid)
	//system set ssid
	//the ssid is same with last connected ssid
	//current is connected state
	//run this case, that mean is:
	//   no link directed to hw
	//   but keep link with up level, cheat os we keep in conntected state
	//   do join 
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_KNOWN_INFO)
	{
		Adap_SetLink(pAdap,LINK_FALSE);
		Mng_StartJoin(pAdap,JOIN_STATE_NORMAL_INIT);
		pAdap->must_indicate_uplevel_flag = TRUE;
	}
	// TODO: start management with link down only
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_DIRECT_DISCONNECT)
	{
		sc_iw_send_bssid_to_up(pAdap->net_dev, NULL);
		Adap_SetLink(pAdap,LINK_FALSE);
		Adap_UpdateMediaState(pAdap,(UINT8)LINK_FALSE);
		Mng_InitParas(pAdap);
	}
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_DISCONNECT)
	{
		sc_iw_send_bssid_to_up(pAdap->net_dev, NULL);
		 // CheckForHung function will indicate status
		Adap_SetLink(pAdap,0); 
	//	pAdap->in_doing_internalreset_flag = FALSE;
	}
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_DISASSOC)
	{
		sc_iw_send_bssid_to_up(pAdap->net_dev, NULL);
		Mng_InitParas(pAdap);
		
		Adap_SetLink(pAdap,LINK_FALSE); 
		if(pAdap->reset_sub_type & RESET_SUB_TYPE_NOT_NOTIFY_UPLEVEL)
		{
			//NULL;
		}
		else 	
		{
			Adap_UpdateMediaState(pAdap,0);
		}
//		pAdap->wlan_attr.ssid_len = 0;		//justin: 0525.... can't do this. it may cover the set_ssid values
//		pAdap->set_information_complete_flag = TRUE;		//Justin: 0816. Don't need to indicate if did not return pending while set OID
	}
	//sys set ssid
	//the ssid is same with last ssid
	//the ssid is not in scan list
	//then:
	//   indicate disconnted stata to hw only.(don't notify uplevel about disconnect message)
	//   scan with the ssid until associated.
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_KNOWN_SSID_SCAN)
	{
		Adap_SetLink(pAdap,LINK_FALSE); 
		Mng_OidScanWithSsid(pAdap);
	}
	//sys set ssid
	//the ssid is in scan list
	//the ssid is different last ssid
	//notify our device into no connected state.
	//begin join
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_KNOWN_INFO_DIFF_SSID)
	{
		Adap_SetLink(pAdap,LINK_FALSE);
		if (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA) // Patch for HCT 12.0
		{
			Adap_UpdateMediaState(pAdap,0);
		}
		Mng_StartJoin(pAdap,JOIN_STATE_NORMAL_INIT);
	}
	//sys set ssid
	//the ssid is not in scan list
	//the ssid is different last ssid
	//then:
	//   disconnect: both up level and hw
	//   scan with the ssid
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_KNOWN_SSID_SCAN_DIFF_SSID)
	{
		Adap_SetLink(pAdap,LINK_FALSE); 
		if (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA) // Patch for HCT 12.0
		{
			Adap_UpdateMediaState(pAdap,0);
		}
		Mng_OidScanWithSsid(pAdap);
	//	pAdap->in_doing_internalreset_flag = FALSE;
	}
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_DO_JOIN_TILL_BEACON)
	{
		Adap_SetLink(pAdap,LINK_FALSE);
		//	pAdap->link_is_active = NdisMediaStateDisconnected;
		//	Adap_UpdateMediaState(pAdap,0);
		if (pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
		{
			Adap_UpdateMediaState(pAdap,0);
		}
		Mng_MidJoinUntilSuccess(pAdap);
	//	pAdap->in_doing_internalreset_flag = FALSE;
	}
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_SSID_OID_REQUEST)
	{
		//only disconnet link status of software
		Adap_SetLink(pAdap,LINK_FALSE);
		//notify up level with link status
		if(pAdap->reset_sub_type & RESET_SUB_TYPE_NOTIFY_UPLEVEL)
		{
			if (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA) // Patch for HCT 12.0
			{
				Adap_UpdateMediaState(pAdap,0);
			}
		}
		//begin join
		if(pAdap->reset_sub_type & RESET_SUB_TYPE_BEGIN_JOIN)
		{
			Mng_StartJoin(pAdap,JOIN_STATE_NORMAL_INIT);
		}
		//begin scan with ssid until join
		else if(pAdap->reset_sub_type & RESET_SUB_TYPE_BEGIN_SCAN)
		{
			Mng_OidScanWithSsid(pAdap);
		}
	}
	//reconnect with bssid
	//the bssid is in scan list
	//do not notify our device into no connected state.
	//begin join
	else if (pAdap->reset_type == (UINT8)RESET_TYPE_KNOWN_BSSID)
	{
		Adap_SetLink(pAdap,LINK_FALSE);		//justin:	080425.  must set this flag, otherwise checkforhang would invoke  updateMediaState to indicate connected status to up-level
		
/*		if (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA) // Patch for HCT 12.0
		{
			Adap_UpdateMediaState(pAdap,0);
		}
		*/
		pAdap->must_indicate_uplevel_flag = TRUE;
		Mng_StartJoin(pAdap,JOIN_STATE_NORMAL_INIT);
	}

	
	if(!Adap_Driver_isHalt(pAdap))
	{
		// wlan_timer_start(&pAdap->sending_timer,1);
		Tx_Send_Next_Frm(pAdap);
		//wlan_timer_start(&pAdap->sending_timer,TIMER_1MS_DELAY);

		DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SetResetType in internalreset\n");
		Adap_SetResetType(pAdap, RESET_TYPE_NOT_SCAN,0);
	/* Set system timer */

		wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);
		wlan_period_timer_start(&pAdap->periodic_timer,TIMER_PERIODIC_DELAY);
	}

}





/**************************************************************************
 *   Adap_SetConnection
 *
 *   Descriptions:
 *      This function will set the connection state of this card
 *      module.	
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *      join: IN, join flag.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_SetConnection(PDSP_ADAPTER_T pAdap,UINT32 join)
{
	if (join)
		pAdap->wlan_attr.hasjoined = JOIN_HASJOINED;
	else
		pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;
}



// del by wumin 2009.03.12
#if 0
/**************************************************************************
 *   Adap_CheckMediaState
 *
 *   Descriptions:
 *      This routine will check and modify media status.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_CheckMediaState(PDSP_ADAPTER_T pAdap)
{
//DBGSTR(("* * * * * Adap_CheckMediaState, %s\n",(pAdap->link_is_active == NdisMediaStateConnected)?"Connected": "Disconnected"));

	if(pAdap->must_indicate_uplevel_flag)
	{
		Adap_UpdateMediaState(pAdap, pAdap->link_ok);
		pAdap->must_indicate_uplevel_flag = FALSE;
		return;
	}
	
	if (pAdap->link_is_active != Adap_GetLinkStatus(pAdap))
	{
		//pAdap->in_doing_internalreset_flag = FALSE;

		// if status has changed
		DBG_WLAN__MAIN(LEVEL_TRACE,"media state changed to %s\n",((pAdap->link_is_active == LINK_OK)? "Disconnected": "Connected"));
		       
        switch ( pAdap->link_is_active )
        {
        case LINK_OK:           
			// changing from connected
			pAdap->link_is_active = LINK_FALSE;
//wumin			NdisMIndicateStatus(pAdap->dsp_adap_handle,NDIS_STATUS_MEDIA_DISCONNECT,NULL,0);
			// NOTE: have to indicate status complete every time you indicate status
//wumin			NdisMIndicateStatusComplete(pAdap->dsp_adap_handle);
			break;
	case LINK_FALSE:        
			// changing from disconnected
			
          		pAdap->link_is_active = LINK_OK;

//wumin			NdisMIndicateStatus(pAdap->dsp_adap_handle,NDIS_STATUS_MEDIA_CONNECT,NULL,0);
			// NOTE: have to indicate status complete every time you indicate status
//wumin			NdisMIndicateStatusComplete(pAdap->dsp_adap_handle);
			break;
        }
    }
	
}
#endif 

/**************************************************************************
 *   Adap_CheckMediaState
 *
 *   Descriptions:
 *      This routine will directly change media status.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *      state: IN, link flag.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_UpdateMediaState(PDSP_ADAPTER_T pAdap,UINT32 state)
{
	PDSP_ATTR_T   			pt_mattr = &pAdap->wlan_attr;

	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Adap_UpdateMediaState, %s\n",(state != 1)? "Disconnected": "Connected");
	if (state)
	{
		//pAdap->link_is_active = LINK_OK;

		pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;

//wumin		NdisMIndicateStatus(pAdap->dsp_adap_handle,NDIS_STATUS_MEDIA_CONNECT,NULL,0);
		// NOTE: have to indicate status complete every time you indicate status
//wumin		NdisMIndicateStatusComplete(pAdap->dsp_adap_handle);

#ifdef ROAMING_SUPPORT
		DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Adap_UpdateMediaState, NdisMediaStateConnected\n");
		//if(pAdap->reconnect_status == DOING_RECONNECT)	//justin:	071226.	no matter what condition, set this after connected!
			DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Adap_UpdateMediaState, DOING --> NO RECONNECT\n");
			pAdap->reconnect_status = NO_RECONNECT;
#endif		
	}
	else
	{
#ifdef PMK_CACHING_SUPPORT_DEL // add by jason 2007.9.3	//Justin:	071225.	move to oid	set_pmkid
		if(	pAdap->pmk_id_caching.Length 
		&&	pAdap->pmk_id_caching.BSSIDInfoCount > 0
		&&	pAdap->pmk_id_caching.BSSIDInfoCount <= MAX_NUM_OF_PMK_IDS)
		{
			pAdap->pmk_id_caching.Length = 0;
			pAdap->pmk_id_caching.BSSIDInfoCount = 0;
			sc_memory_set(pAdap->pmk_id_caching.BSSIDInfo, 0, MAX_NUM_OF_PMK_IDS * sizeof(BSSIDInfo));
			DBG_WLAN__MAIN(LEVEL_TRACE,"PMK clearing pmk valid flag\n"); // add by jason 2007.9.3
		}
#endif
 

#ifdef ROAMING_SUPPORT
		DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Adap_UpdateMediaState, NdisMediaStateDisconnected\n");
		pAdap->reconnect_status = NO_RECONNECT;
#endif		
//		pAdap->wlan_attr.gdevice_info.privacy_option = FALSE;//Justin: can't do this, we will set right privacy in oid SET_802_11_SSID. 
														//otherwise,will get wrong privacy while connect tow encrypt aps continuous,
														// the sencond one will fail to connect
		Mng_InitParas(pAdap);
		pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;

		//pAdap->link_is_active = LINK_FALSE;
//wumin		NdisMIndicateStatus(pAdap->dsp_adap_handle,NDIS_STATUS_MEDIA_DISCONNECT,NULL,0);
		// NOTE: have to indicate status complete every time you indicate status
//wumin		NdisMIndicateStatusComplete(pAdap->dsp_adap_handle);		
		pt_mattr->is_exist_ibss_b_sta = FALSE;
        //Justin: 20090428. The miniport driver must discard WEP keys immediately 
        Adap_key_mib_to_false(pAdap);

	}
	
}
// per 50 milliseconds
VOID Adap_PeriodicTimeout(PVOID param)
{
    PDSP_ADAPTER_T pAdap;
	DSP_WRITE_MAILBOX dsp_mailbox;
	// PDSP_PKT_LIST_T pPktList;

	/*Get the pointer of Adapter context.*/
	pAdap = (PDSP_ADAPTER_T)param;

	//Justin: don't do this during hardware reset
	if(pAdap->driver_state == DSP_HARDWARE_RESET)
	{
		return;
	}

	if(Adap_Driver_isHalt(pAdap))
	{
		return;
	}

	if(Adap_Driver_isWork(pAdap))
	{
		Rx_defragment_timeout(pAdap);
	}
	
//	{
//		static UINT32 i=0;
//
//		i++;
//
//		if(i>=10)
//		{
//			i=0;
//			DBG_WLAN__MAIN(LEVEL_TRACE,"5s.\n ");					
//		}
//	}

	//send null frame when no packet
	if(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
	{
		// 8* 500ms = 4s
		if(	(	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2
			||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK) 
		&&	!pAdap->wlan_attr.wpa_group_key_valid)
		{
			if(++pAdap->wpa_timeout > 10)	// 5 seconds
			{				
				DISASSOCIATE_COM_T disass_cmd;

				DBG_WLAN__MAIN(LEVEL_ERR,"WPA Auth Timeout, need disaccociate.\n ");	
				
				sc_memory_set(disass_cmd.addr,0xff,WLAN_ETHADDR_LEN);
	
				disass_cmd.reason = (UINT16)WAY4_HSHAKE_TIMEOUT;
				disass_cmd.ind = 1;     //ind to os required

				if(STATUS_SUCCESS !=Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
							DSP_TASK_EVENT_OID_DISASSOC,
							DSP_TASK_PRI_HIGH,
							(PUINT8)&disass_cmd,
							sizeof(DISASSOCIATE_COM_T)))
				{
					DBG_WLAN__MAIN(LEVEL_ERR,"Add DSP_TASK_EVENT_OID_DISASSOC Task failure.\n ");	
				}
			}
		}
		else
		{
			if(++pAdap->send_nulll_count > 8)
			{
				pAdap->send_nulll_count = 0;
				if(	Tx_DataFrmlist_IsEmpty(pAdap)
				&&	Tx_MngFrmlist_IsEmpty(pAdap))
				{
					Mng_MakeNullFrame(pAdap);	// to indicate ap our next ps mode
					DBG_WLAN__MAIN(LEVEL_INFO,"tx_send null frame\n");	
				}
			}			
				
		}
	}

	//check rssi
	if(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
	{
		if(Adap_check_rssi(pAdap,&dsp_mailbox.val) == TRUE)
		{
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_ACCESS_DSP_MAILBOX))
			{
#ifdef ANTENNA_DIVERSITY
				dsp_mailbox.type = TASK_MAILBOX_TYPE_CORR;
#endif
				DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^rssi auto corelator check task,%x\n",dsp_mailbox.val);
				dsp_mailbox.addr = WLS_MAC__BBREG_20_23;
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
						DSP_TASK_EVENT_ACCESS_DSP_MAILBOX,DSP_TASK_PRI_NORMAL,(PUINT8)&dsp_mailbox,sizeof(DSP_WRITE_MAILBOX));	
			}
		}
		//check mic failure state
		Adap_StartTKIPCounterMeasure(pAdap,REQUEST_CM_MONITOR_MIC_COUNTER,0);	
	}
	
	
#if 0	
	if(	(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA) &&		//in bss mode
		(pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE) && 
			(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED))
	{
		if(pAdap->rate_up_to_24mbps_count++ > 8)     //10* 500ms = 5s
		{
			pAdap->rate_up_to_24mbps_count = 0;
			Rate_Up_To_Standard(pAdap);
		}
	}
#endif

	//TX hang monitor 
	if(pAdap->tx_hang_count != 0)
	{
		pAdap->tx_hang_count--;
		if(pAdap->tx_hang_count > (3*TX_HANG_TRANSACT_INTERVAL))
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"Tx_hang_cunt with dirty data \n");
			pAdap->tx_hang_count = 0;
		}
	}


#ifdef ROAMING_SUPPORT_DEL
	if(pAdap->reconnect_status == CAN_RECONNECT)//pAdap->can_do_reconnect_flag)
	{
		if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_RECONNECT))
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, periodic timeout, reconnect_status = %d\n",pAdap->reconnect_status);
			//Mng_reconnect(pAdap);
			Task_CreateTask(
						(PDSP_TASK_T)pAdap->ppassive_task,
						DSP_TASK_EVENT_RECONNECT,
						DSP_TASK_PRI_NORMAL,
						NULL,
						0);
		}
	}
#endif//ROAMING_SUPPORT

}
/**************************************************************************
 *   Adap_SysTimeout
 *
 *   Descriptions:
 *      This function is called by a timer indicating driver will do perodic
 *      transaction.
 *   Argumens:
 *      sysspiff1: IN, Points to a system-specific variable, which is opaque 
 *                to MiniportTimer and reserved for system use.
 *      MiniportAdapterContext: IN, the pointer to adapter context.
 *      sysspiff2: IN, Points to a system-specific value that is reserved for
 *                system use. 
 *      sysspiff3: IN, Points to a system-specific value that is reserved for
 *                system use. 
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_SysTimeout(PVOID param)
{
	PDSP_ADAPTER_T 	pAdap;
	PMNG_STRU_T	pt_mng;
//	static UINT32		i=0;
	//UINT8              	val;

	/*Get the pointer of Adapter context.*/
	pAdap = (PDSP_ADAPTER_T)param;
	pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
	//Justin: don't do this during hardware reset
	if(pAdap->driver_state == DSP_HARDWARE_RESET)
	{
		pAdap->ap_alive_counter = (pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_SINGLE_MODE) ?
			DEFAULT_AP_ALIVE_COUNTER : 3*DEFAULT_AP_ALIVE_COUNTER;	

		if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)	//justin: in power save mode, wait more time
			pAdap->ap_alive_counter = 3*DEFAULT_AP_ALIVE_COUNTER;

		return;
	}
	
	if(Adap_Driver_isHalt(pAdap))
	{

	//close it because can't wait at dispatch level
#if 0	
   	   //recycle tx irp
	   {
		   IrpPool_CancelAllPendingIrps((PIRP_POOL_INFO_T)pAdap->ptx_head_irp_pool);
	          if ( IrpPool_Wait((PIRP_POOL_INFO_T)pAdap->ptx_head_irp_pool,500) == STATUS_TIMEOUT )
	         {
	          	 DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SysTimeout! All Tx head IRPs were not accounted for\n");
	          }	
	   }
	   
	   {
		   IrpPool_CancelAllPendingIrps((PIRP_POOL_INFO_T)pAdap->ptx_irp_pool);
	          if ( IrpPool_Wait((PIRP_POOL_INFO_T)pAdap->ptx_irp_pool,500) == STATUS_TIMEOUT )
	         {
	          	 DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SysTimeout! All Tx head IRPs were not accounted for\n");
	          }	
	   }
#endif

	      //Release all TX packet in TX queue when suprising remove happen by 
	      //systimeout() function
	      //at some cases, for example running chariot or other network download 
	      //software, OS will never call halt interface until all TX packet did not 
	      //indicated to up level for release.
		DBG_WLAN__MAIN(LEVEL_TRACE,"Systimeout with halt then release TX packet\n");

    	Tx_Cancel_Data_Frm(pAdap);
    	DBG_WLAN__MAIN(LEVEL_TRACE,"Systimeout with halt then complete sys reset\n");
		
		if(pAdap->driver_state == DSP_HARDWARE_RESET
			||pAdap->driver_state == DSP_SYS_RESET)
			;
			//NdisMResetComplete(pAdap->dsp_adap_handle,STATUS_SUCCESS,FALSE);
#if 1
		if (pAdap->set_information_complete_flag || pt_mng->oidscanflag)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_SysTimeout: Set information complete!\n");
			pAdap->set_information_complete_flag = FALSE;
			pt_mng->oidscanflag = FALSE;
//			DBG_WLAN__MAIN(LEVEL_TRACE,"pt_mng->oidscanflag = FALSE;\n");
			
			//NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
			pAdap->dsp_pending_count = 0;
		}
#endif
		return;
	}

//DBGSTR(("* * * * * Adap_SysTimeout \n"));
	// DBGSTR_NORMAL(("System timer timeout!\n"));
	pt_mng = (PMNG_STRU_T)pAdap->pmanagment;

	//combo
#if 0
	if(pAdap->halt_flag == TRUE)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"systimeout halt_flag == true \n");
		
		//here call dsp_send directly, maybe it is not good way
		//Dsp_Halt(pAdap);//return STATUS_FAILURE;
		pAdap->halt_flag = FALSE;
//		Adap_HardwareReset(pAdap);
		pAdap->halt_flag = FALSE;
	}
#endif	

	
	// Just for HCT test
	if (pAdap->first_run_flag)
	{
		pAdap->first_run_flag = FALSE;
		Adap_UpdateMediaState(pAdap,0);
	}

#if 1
	if (pAdap->sys_reset_flag)
	{
		if (pAdap->reset_complete_flag)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$$$$$$$$$Dsp_Reset  complete\n");

			pAdap->reset_complete_flag = FALSE;
			pAdap->sys_reset_flag = FALSE;
			//NdisMResetComplete(pAdap->dsp_adap_handle,STATUS_SUCCESS,FALSE);
		}
		else
		{
			pAdap->dsp_pending_count++;
			if(pAdap->dsp_pending_count > DSP_PENDING_COUNT)
			{
				//NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
				pAdap->dsp_pending_count = 0;
			}
		}
	}

#endif
#if 0
	if (pAdap->set_information_complete_flag)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Set information complete!\n");
		pAdap->set_information_complete_flag = FALSE;
		NdisMSetInformationComplete(pAdap->dsp_adap_handle, STATUS_SUCCESS);
		pAdap->dsp_pending_count = 0;
	}
#endif	
	/* pPktList = (PDSP_PKT_LIST_T)pAdap->ptx_packet_list;
	if (++pPktList->timercount >= PKTLISTCHECKPERIOD)
	{
		pPktList->timercount = 0;
		PktList_Timeout(pPktList);
	} */

	/* Just watch bulk out pipe. */
	//woody debug
	//IrpPool_Timeout((NDIS_HANDLE)pAdap, (PIRP_POOL_INFO_T)pAdap->ptx_irp_pool);

	/* Check if the checking time of ATPA is comming.*/
	//Atpa_CheckTimeout(pAdap);

	/* Process the timers' timeout for TKIP counter-measures. */
	Adap_TKIPCounterMeasureTimeout(pAdap);

	/* Reading RSSI value from PHY register if there is no received frame in 800ms */
	//Phy_CheckRssiTimeout(pAdap);

#if 1
	//jsutin: 0803 for hct-- 1c_reset,
	//combo
	//	if(Adap_Driver_isWork(pAdap))
	if(pAdap->driver_state == DSP_DRIVER_WORK)
	{
		if((pAdap->link_ok == LINK_OK)
			&&(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
			//&&(pAdap->wlan_attr.gdevice_info.ps_mode == PSS_ACTIVE)
			)//justin: 1009,	 not do this whil in power save mode
		{
			pAdap->ap_alive_counter--;
//			i++;
			
			if(	pAdap->ap_alive_counter%20==0)
//			||	i>=5)
			{
//				i=0;
				DBG_WLAN__MAIN(LEVEL_TRACE,"ap_alive_counter=%u.QoS=%s\n", 
					pAdap->ap_alive_counter,(sc_netq_ifstop(pAdap->net_dev) == 1) ? "stop" : "start");
			}
			
			if(pAdap->ap_alive_counter == 0)
			{
				pAdap->ap_alive_counter = 5*DEFAULT_AP_ALIVE_COUNTER;//DEFAULT_AP_ALIVE_COUNTER;

				
				
			#ifdef ROAMING_SUPPORT
				DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$$$$$$$$$ beacon lost oidscanflag = %d, reconnect_status = %d \n",pt_mng->oidscanflag,pAdap->reconnect_status); 
	

				if ((!pt_mng->oidscanflag) && (pAdap->reconnect_status == NO_RECONNECT))//(!pAdap->can_do_reconnect_flag))//(!pAdap->scanning_flag) && 
				{
					pAdap->reconnect_status = NEED_RECONNECT;
					
					if((!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN))
						&&(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_OID_DISASSOC))
						&&(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_OID_SET_SSID))
						)
					{
						DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, beacon lost----> do scan\n");

						//woody 080630
						//set the two variable at task 
						pAdap->scanning_flag = FALSE;		//justin:	080703.	scan must have done if run here.
						pAdap->scan_watch_value = 0;

						pAdap->bStarveMac = TRUE;		//stop tx... avoid tx hang
						

						//save datarate to recover latter
						pt_mng->rateIndex_b4_roaming = pt_mng->rateIndex;

						pt_mng->bssdeslist.bssdesnum = 0;		//justin: 080902.	init bssdesnum before do beacon lost scan, otherwise, the OLD AP's info is still in list
						pt_mng->oiddeslist.bssdesnum = 0;
						
						if(STATUS_SUCCESS == Task_CreateTask(
								(PDSP_TASK_T)pAdap->ppassive_task,
								DSP_TASK_EVENT_SCAN,
								DSP_TASK_PRI_HIGH,
								NULL,
								0))
						{
							DBG_WLAN__MAIN(LEVEL_TRACE,"Add Scan Task\n");						
						}
						else
						{
							DBG_WLAN__MAIN(LEVEL_ERR,"Add Scan Task Failure.\n");								
						}
					}
						//return STATUS_PENDING;
				}
				
			#else			
				DBG_WLAN__MAIN(LEVEL_TRACE,"beacon lost----> wait beacon\n");
				if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BEACON_LOST))
					Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BEACON_LOST, DSP_TASK_PRI_HIGH, NULL, 0);
					pAdap->ap_alive_counter = 5*DEFAULT_AP_ALIVE_COUNTER;
			#endif
			
			}
		}
		//for ibss beacon live
		else if((pt_mng->statuscontrol.curstatus == JOINOK) &&
			(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA))
		{
			pAdap->ap_alive_counter--;
			if(pAdap->ap_alive_counter == 0)
			{
				if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BEACON_LOST))
					Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BEACON_LOST, DSP_TASK_PRI_HIGH, NULL, 0);
				pAdap->ap_alive_counter = 5*DEFAULT_AP_ALIVE_COUNTER;
			}		
		}
	}	
#endif 
	/* Detect whether there is received frame or not after beacon lost interrupt occurs. */
	/* if (pAdap->detect_rec_frame_flag)
	{
		if (++pAdap->detect_rec_frame_value >= 2) //200~400ms
		{
			DBGSTR_NORMAL(("detect rec frame !\n"));
			pAdap->detect_rec_frame_flag = FALSE;
			pAdap->detect_rec_frame_value = 0;
			if (pAdap->detect_current_rx_total_packet == pAdap->mib_stats.total_packet_rx)
				Task_CreateTaskForce((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_BEACON_LOST,DSP_TASK_PRI_NORMAL,NULL,0);
		}
	} */
	
	/* Set system timer */
	//if ((pAdap->power == NdisDeviceStateD0) && pAdap->systimer_active_flag)
	wlan_timer_start(&pAdap->sys_timer,TIMERPERIOD);

#if 0
	if(pAdap->rx_error_count >= DSP_RX_CHECK_COUNT)
	{
		pAdap->rx_error_count = 0;
		if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BULKIN_ERROR))
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BULKIN_ERROR, DSP_TASK_PRI_NORMAL, NULL, 0);
/*		
		pAdap->rx_error_count = 0;
		val = Basic_ReadRegByte(pAdap,0x00);
		//PCI READY
		if(((val & BIT5) != BIT5) || (val !=0xff))
		{
			pAdap->adap_suprise_remove_flag = 1;			
		}
*/		
	}
#endif
#if 0

	if(pAdap->tx_error_count >= DSP_TX_CHECK_COUNT)
	{
		pAdap->tx_error_count = 0;
		//reset bulk out command
		//Vcmd_Reset_Bulkout_Request(pAdap);

		if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BULKOUT_ERROR))
				Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_BULKOUT_ERROR, DSP_TASK_PRI_NORMAL, NULL, 0);
	}
#endif	

	//Justin:	071219.	Avoid pending packets in tx queue for much long time. Indicate status to up-level for all pending packets while in status LINK_FALSE (diconnect.)
	if(pAdap->link_ok != LINK_OK)
	{
	    Tx_Cancel_Data_Frm(pAdap);
	}
}

/**************************************************************************
 *   Adap_MngTimeout
 *
 *   Descriptions:
 *      This function is called by a timer indicating management module has
 *      something to do.
 *   Argumens:
 *      sysspiff1: IN, Points to a system-specific variable, which is opaque 
 *                to MiniportTimer and reserved for system use.
 *      MiniportAdapterContext: IN, the pointer to adapter context.
 *      sysspiff2: IN, Points to a system-specific value that is reserved for
 *                system use. 
 *      sysspiff3: IN, Points to a system-specific value that is reserved for
 *                system use. 
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_MngTimeout(PVOID param)
{
	PDSP_ADAPTER_T pAdap;
	PMNG_STRU_T     pt_mng;

	pAdap =(PDSP_ADAPTER_T)param;;
	pt_mng = pAdap->pmanagment;
	
	pt_mng->mng_timer_running=FALSE;
	//DBGSTR_MLME(("* * * * * Adap_MngTimeout \n"));
	if(Adap_Driver_isHalt(pAdap))
	{
		return;
	}

	/*Just create a self-defined task. In the routine of the passive task
	 , the real timeout function for management module is called. The reason
	 is that the timeout function of management may do some operators which 
	 is just done in passive level (for example: read or write register).*/
	/* Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
					DSP_TASK_EVENT_MNGTIMEOUT,
					DSP_TASK_PRI_NORMAL,
					NULL,
					0); */

	Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
					DSP_TASK_EVENT_MNGTIMEOUT,
					DSP_TASK_PRI_NORMAL,
					NULL,
					0);
}

/**************************************************************************
 *   Adap_Scan
 *
 *   Descriptions:
 *      This function is called when driver wants to scanning all channels.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_Scan(PDSP_ADAPTER_T pAdap)
{
	//TDSP_STATUS  status;
	//UINT32 tmplong;
	//UINT32 				tmp;

	//disable tx process
	pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;
	pAdap->bStarveMac = TRUE;

	   pAdap->scan_result_new_flag = 0;

	   Mng_OidScan(pAdap);	
}

/**************************************************************************
 *   Adap_BeaconChange
 *
 *   Descriptions:
 *      This function is called when BEACON-CHANGE interrupt event is occured .
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_BeaconChange(PDSP_ADAPTER_T pAdap)
{
#if 0
	// if ((pAdap->wlan_attr.hasjoined == JOIN_HASJOINED) && (!pAdap->scanning_flag))
	if ((pAdap->wlan_attr.hasjoined == JOIN_HASJOINED) && (!pAdap->scanning_flag) && (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA))
	{
		// Adap_SetResetType(pAdap, RESET_TYPE_KNOWN_SSID_SCAN);
		// Adap_InternalReset(pAdap);
		Mng_BeaconChange(pAdap);
	}
#endif	
}


/**************************************************************************
 *   Adap_TbttUpdate
 *
 *   Descriptions:
 *      This function is called when TBTT-UPDATE interrupt event is occured, 
 *      driver will recalculate tbtt point and rewrite the register.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_TbttUpdate(PDSP_ADAPTER_T pAdap)
{
#if 0
	union {
		UINT64 t64;
		UINT32 t32[2];
	} tt;
	
	
	//write next tbtt point
	//tt.t32[0]=Adap_ReadRegDword(pAdap,TSF_REG);
	//tt.t32[1]=Adap_ReadRegDword(pAdap,TSF_REG+4);
//	Adap_ReadMultiDWord(pAdap,(PUINT8)&(tt.t64),(UINT16)sizeof(UINT64),(UINT16)TSF_REG);
	tt.t64 = tt.t64 >> 10;
	tt.t64 /= pAdap->wlan_attr.beacon_interval;
	//tt.t64+=5;
	tt.t64+=2;
	tt.t64*= pAdap->wlan_attr.beacon_interval;
	//
	//Adap_WriteRegDword(pAdap,tt.t32[0],TBTT_TIMER);
	//Adap_WriteRegDword(pAdap,tt.t32[1],TBTT_TIMER+4);
	// woody close , Adap_WriteMultiDWord(pAdap,(PUINT8)&(tt.t64),(UINT16)sizeof(UINT64),(UINT16)TBTT_TIMER);
	#endif
}


/**************************************************************************
 *   Adap_InitTKIPCounterMeasure
 *
 *   Descriptions:
 *      This function is used for TKIP counter-measures. This function is an
 *      initial function and is called in Adap_Init_Workstate function.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_InitTKIPCounterMeasure(PDSP_ADAPTER_T pAdap)
{
	pAdap->tkip_counter_measure.mic_failure_counts = 0;
	pAdap->tkip_counter_measure.mic_failure_rate_timer_counts = 0;
	pAdap->tkip_counter_measure.mic_failure_rate_timer_valid = FALSE; // stop timer
	pAdap->tkip_counter_measure.wait_1x_frame_timer_counts = 0;
	pAdap->tkip_counter_measure.wait_1x_frame_timer_valid = FALSE;   // stop timer
	pAdap->tkip_counter_measure.delay_60_seconds_timer_counts = 0;
	pAdap->tkip_counter_measure.delay_60_seconds_timer_valid = FALSE; // stop timer
	pAdap->tkip_counter_measure.disassociate_frame_sent = FALSE;
}

/**************************************************************************
 *   Adap_StartTKIPCounterMeasure
 *
 *   Descriptions:
 *      This function is used for TKIP counter-measures. This function is called
        when driver receives a DATA frame with MIC failure in TKIP mode.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
UINT32 Adap_StartTKIPCounterMeasure(PDSP_ADAPTER_T pAdap,COUNTER_MEASURE_REQUEST_TYPE cm_type,BOOLEAN is_group)
{
	UINT32  ret_val;
	DISASSOCIATE_COM_T  disass_cmd;


	ret_val = cm_type;

	do{
		// 1: This case used in dsp_send to prevent packet sent when cm happen
		if(cm_type == REQUEST_CM_STOP_CURRENT)
		{
			if(pAdap->tkip_counter_measure.wait_1x_frame_timer_valid)
			{
				//if 1x  packet want be sent ,how to do ??
				DBG_WLAN__MAIN(LEVEL_TRACE,"Stop send due to counter measure\n");
			}
			else
			{
				ret_val =  REQUEST_CM_RETURN_GO_ON;	//go on
			}
			break;
		}

		// 2:this case begins calcu cm count when mic failure happen
		//    mic failure count = 1, start 60s timer
		//    mic failure count = 2, start 1s timer and make key invalid
		if(cm_type == REQUEST_CM_CALCU)
		{
			pAdap->tkip_counter_measure.mic_failure_counts++;

			// notify os about mic error
			Adap_Set1xAuthRequest(
			      pAdap,
			      (is_group) ? NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR : NDIS_802_11_AUTH_REQUEST_GROUP_ERROR);

			//first mic fail
			if(pAdap->tkip_counter_measure.mic_failure_counts  == 1)
			{
				// notify os about mic error
				DBG_WLAN__MAIN(LEVEL_TRACE,"Get one tkip counter measue case \n");

				pAdap->tkip_counter_measure.wait_1x_frame_timer_valid = FALSE;
				pAdap->tkip_counter_measure.disassociate_frame_sent = FALSE;
				//set 60s timer first
				pAdap->tkip_counter_measure.delay_60_seconds_timer_valid = TRUE;
				//set 1m timer to wait next mic error
				wlan_timer_start(&pAdap->tx_countermeasure_timer,SME_WPA_COUNTER_MEASURE_TIMEOUT);	
			}
			//second mic fail
			else if(pAdap->tkip_counter_measure.mic_failure_counts  == 2)
			{
				DBG_WLAN__MAIN(LEVEL_TRACE,"Get second tkip counter measue case in 1m\n");
				pAdap->tkip_counter_measure.delay_60_seconds_timer_valid = FALSE;
				pAdap->tkip_counter_measure.disassociate_frame_sent = FALSE;
				//set 1s timer				
				pAdap->tkip_counter_measure.wait_1x_frame_timer_valid = TRUE;
				//clear valid key flag 
				Adap_PrepareTransmit1xFrame(pAdap);
				//set 1s timer to wait new 1x packet to re-set key
				wlan_timer_start(&pAdap->tx_countermeasure_timer,SME_WPA_COUNTER_MEASURE_FAILURE_REPORT_WAIT);	
			}
			else
			{
				DBG_WLAN__MAIN(LEVEL_TRACE,"Get more tkip counter measue case \n");
				pAdap->tkip_counter_measure.mic_failure_counts = 3;
				if(pAdap->tkip_counter_measure.wait_1x_frame_timer_valid == FALSE)
				{
					DBG_WLAN__MAIN(LEVEL_ERR,"Counter measue wrong case in mismatch counter and timer state\n");
				}
			}
			break;
		 }

		//cm timeout flow
		if(cm_type == REQUEST_CM_TIMEOUT)
		{
			// 1s timer reach
			if(pAdap->tkip_counter_measure.wait_1x_frame_timer_valid == TRUE)
			{
				if(pAdap->wlan_attr.wpa_pairwise_key_valid == 0 ||
					pAdap->wlan_attr.wpa_group_key_valid ==0)
				{
					DBG_WLAN__MAIN(LEVEL_ERR,"MIC counter measure re-set key fail \n");
					if(pAdap->tkip_counter_measure.disassociate_frame_sent == FALSE)
					{
						//send disassoc with reason
						//and disconnect
						//copy bssid into para
						sc_memory_copy(disass_cmd.addr,(PUINT8)pAdap->wlan_attr.bssid,WLAN_ETHADDR_LEN);
						disass_cmd.reason = MIC_FAILURE;
						disass_cmd.ind = 1;   //ind required
						Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
							DSP_TASK_EVENT_OID_DISASSOC,DSP_TASK_PRI_HIGH,(PUINT8)&disass_cmd,sizeof(DISASSOCIATE_COM_T));	
						pAdap->tkip_counter_measure.disassociate_frame_sent = TRUE;
						//wait a 59s period for MIC failure probation period 
						wlan_timer_start(&pAdap->tx_countermeasure_timer,
							SME_WPA_COUNTER_MEASURE_TIMEOUT - SME_WPA_COUNTER_MEASURE_FAILURE_REPORT_WAIT);	
					}
					else
					{
						Adap_InitTKIPCounterMeasure(pAdap);	
						DBG_WLAN__MAIN(LEVEL_TRACE,"End the MIC failure probation period\n");
					}
				}
				else
				{
					Adap_InitTKIPCounterMeasure(pAdap);	
					DBG_WLAN__MAIN(LEVEL_TRACE,"MIC counter measure re-set key success\n");
				}
				break;
			}
			else if(pAdap->tkip_counter_measure.delay_60_seconds_timer_valid == TRUE)
			{
				Adap_InitTKIPCounterMeasure(pAdap);	
				DBG_WLAN__MAIN(LEVEL_TRACE,"MIC counter measure 60s restore without new mic error\n");
			}
			break;
		}

		// in MIC failure probation period don't attempt to connect
		if(cm_type == REQUEST_CM_SET_SSID)
		{//justin:	080901.	counter measure for tkip, no matter tkip used for pairwise or group
			if((pAdap->wlan_attr.wep_mode == WEP_MODE_TKIP || 
				((pAdap->wlan_attr.group_cipher== WEP_MODE_TKIP) &&(pAdap->wlan_attr.wep_mode != WEP_MODE_WEP))) &&
				(pAdap->wlan_attr.gdevice_info.privacy_option == TRUE) &&
				(pAdap->tkip_counter_measure.mic_failure_counts >1 ))
			{
				DBG_WLAN__MAIN(LEVEL_ERR,"Set ssid returned directly in mic failure probation period\n");
			}
			else
			{
				ret_val = REQUEST_CM_RETURN_GO_ON;
			}
			break;
    		}

		//Monitor mic_failure_counts in period timer 
		//the function protect driver not to enter a wrong managment flow while
		//mic_failure_counts didn't be clear in time
		if(cm_type == REQUEST_CM_MONITOR_MIC_COUNTER)
		{
			//period timer is 500ms once
			//count = 2 coming from real mic failure
			//so 360*500ms = 180s = 3m
			//delay 3m  then recover mic failure to regular state
			if(pAdap->tkip_counter_measure.mic_failure_counts > 360)
			{
				Adap_InitTKIPCounterMeasure(pAdap);	
				DBG_WLAN__MAIN(LEVEL_TRACE,"recover the mic failure state to regular state\n");
			}
			else if(pAdap->tkip_counter_measure.mic_failure_counts > 1)
			{
				pAdap->tkip_counter_measure.mic_failure_counts++;
			}
            else
            {
            	Adap_InitTKIPCounterMeasure(pAdap); 
                DBG_WLAN__MAIN(LEVEL_INFO,"MIC counter measure default case\n");
            }

			break;
    		}
		

		//never enter	
		DBG_WLAN__MAIN(LEVEL_ERR,"Adap_StartTKIPCounterMeasure bad para input\n");
	}while(FALSE);

	return ret_val;
}

/**************************************************************************
 *   Adap_Set1xAuthRequest
 *
 *   Descriptions:
 *      This function is used for TKIP counter-measures. This function indicates
 *      some status which is associated with 802.1x frame to uper layer.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_Set1xAuthRequest(PDSP_ADAPTER_T pAdap, UINT32 Flags)
{
	UINT8 tmpind[sizeof(NDIS_802_11_STATUS_INDICATION) + sizeof(NDIS_802_11_AUTHENTICATION_REQUEST)];

	PNDIS_802_11_STATUS_INDICATION ptmpind; 
	PNDIS_802_11_AUTHENTICATION_REQUEST ptmpauth;

	ptmpind = (PNDIS_802_11_STATUS_INDICATION)&tmpind[0];
	ptmpauth = (PNDIS_802_11_AUTHENTICATION_REQUEST)&tmpind[sizeof(NDIS_802_11_STATUS_INDICATION)];
	
	
	ptmpind->StatusType = Ndis802_11StatusType_Authentication;
	ptmpauth->Length = sizeof(NDIS_802_11_AUTHENTICATION_REQUEST);
	WLAN_COPY_ADDRESS(ptmpauth->Bssid, pAdap->wlan_attr.bssid);
	ptmpauth->Flags = Flags;
}

/**************************************************************************
 *   Adap_PrepareTransmit1xFrame
 *
 *   Descriptions:
 *      This function is used for TKIP counter-measures. This function does 
 *      some preparation for 802.1x frame's receiving.  
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_PrepareTransmit1xFrame(PDSP_ADAPTER_T pAdap)
{
#if 1
	pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
	pAdap->wlan_attr.wpa_group_key_valid = 0;
#endif	

}

/**************************************************************************
 *   Adap_TKIPCounterMeasureTimeout
 *
 *   Descriptions:
 *      This function is used for TKIP counter-measures. This function is the 
 *      timeout function for the timers of TKIP counter-measures.
 *   Argumens:
 *      pAdap: IN, the pointer of adapter context.
 *   Return Value:
 *      NONE
 *************************************************************************/
VOID Adap_TKIPCounterMeasureTimeout(PDSP_ADAPTER_T pAdap)
{//justin:	080901.	counter measure for tkip, no matter tkip used for pairwise or group
	if (pAdap->wlan_attr.wep_mode == WEP_MODE_TKIP
		||((pAdap->wlan_attr.group_cipher== WEP_MODE_TKIP) &&(pAdap->wlan_attr.wep_mode != WEP_MODE_WEP))
		)
	{
		if (pAdap->tkip_counter_measure.mic_failure_rate_timer_valid)
		{
			// timer1 time out
			if (--pAdap->tkip_counter_measure.mic_failure_rate_timer_counts == 0)
			{
				pAdap->tkip_counter_measure.mic_failure_counts = 0;
				pAdap->tkip_counter_measure.mic_failure_rate_timer_valid = FALSE; // stop timer1
			}
		}
		
		if (pAdap->tkip_counter_measure.wait_1x_frame_timer_valid)
		{
			// timer2 time out
			if (--pAdap->tkip_counter_measure.wait_1x_frame_timer_counts == 0)
			{
				if (pAdap->wlan_attr.start_8021x_flag) // 802.1x frame has been transmitted
				{
					pAdap->tkip_counter_measure.wait_1x_frame_timer_valid = FALSE; // stop timer2
					
					Adap_SetLink(pAdap,0);
					Adap_UpdateMediaState(pAdap,0); // Indicate disconnected status to upper layer.
					
					pAdap->tkip_counter_measure.delay_60_seconds_timer_valid = TRUE;  // start timer3
					pAdap->tkip_counter_measure.delay_60_seconds_timer_counts = 310; // 62 seconds;
				}
				else
				{
					pAdap->tkip_counter_measure.wait_1x_frame_timer_valid = TRUE; // start timer2 again
					pAdap->tkip_counter_measure.wait_1x_frame_timer_counts = 1;
				}
			}
		}
		
		if (pAdap->tkip_counter_measure.delay_60_seconds_timer_valid)
		{
			if (--pAdap->tkip_counter_measure.delay_60_seconds_timer_counts == 0)
			{
				pAdap->tkip_counter_measure.delay_60_seconds_timer_valid = FALSE; // stop timer3
			}
		}
	}
}		



VOID 			Adap_PrintBuffer(PUINT8 buffer, UINT32 len)
{
	DBG_PRINT_BUFF("Adap_PrintBuffer:",buffer,len);
}

VOID Adap_Print_USBReg(void * adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    UINT16      ulAddr = 0x00,ulDataLen = 0x200;
    UINT8       szTmpData[0x200] = {'0'};
  
    if(pAdap == NULL)
    {
         DBG_WLAN__INIT(LEVEL_ERR,"[%s] adapt poiter is null,maybe release!\n",__FUNCTION__);
         return;
    }
    Basic_ReadBurstRegs_Dut(pAdap, szTmpData, (UINT16)ulDataLen, (UINT16)ulAddr);
    Adap_PrintBuffer(szTmpData, ulDataLen);
}

TDSP_STATUS Adap_Dspfw_Download(
	PDSP_ADAPTER_T pAdap,
	PUINT8 buf,
	UINT32   len,
       UINT16 offset,
       UINT8 file_id)
{
	TDSP_STATUS status;
//	UINT8 		local_buff[2048];
	//UINT32 		val;
	//UINT32 	       loop = 10;


	//DBGSTR(("Adap_Dspfw_Download, len:%d, offset:%X, file_id:%d \n",len,offset,file_id));
	
//first_fw:	
    //download bin file to dsp scratch
    status = Fw_download_dspfw_fragment(pAdap,(PUINT8)buf,(UINT16)len,(UINT16)(DOWNLOAD_DSP_FW_FIELD_OFFSET));
    
    if(STATUS_SUCCESS != status)
    {
        DBG_WLAN__INIT(LEVEL_ERR,"[%s] ERROR status = %x\n",__FUNCTION__,status);
        return status;
    }
	
#if 0//Add for verify download data
    // dma mode
	status = Fw_bulkin_dspfw_fragment(pAdap,(PUINT8)local_buff,(UINT16)len,(UINT16)(DOWNLOAD_DSP_FW_FIELD_OFFSET));
    
	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__INIT(LEVEL_ERR,"ERROR status = %x\n",status);
		return status;
	}

	if(!sc_memory_cmp(buf,local_buff,len))
	{
		DBG_WLAN__INIT(LEVEL_ERR,"Adap_Dspfw_Download compare content fail \n");
	       return STATUS_FAILURE;
	}
#endif
	//send dma parameters by vendor command
    // download bin form scratch to shultte
	status = Vcmd_Transmit_DSPFw_Head(pAdap, offset, len,file_id);

	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__INIT(LEVEL_ERR,"[%s] download firmware fail in head command \n",__FUNCTION__);
		return STATUS_FAILURE;
	}

	return status;
}

TDSP_STATUS Adap_Dspfw_Download_Interface(
	PDSP_ADAPTER_T pAdap,
	PUINT8 buf,
	UINT32 total_len,
	UINT8 file_id)
{
//	UINT32 div;
//	UINT32 mod;
//	UINT32   i;
	UINT32	length, bulk_len, consumed;
	TDSP_STATUS Status = STATUS_FAILURE;
	PUINT8 pbuf;
	
	if (total_len == 0)
		return STATUS_SUCCESS;
	

	length = total_len;
	consumed = 0;
	pbuf = buf;

	while(length)
	{
		if(pAdap->run_in_halt_flag == TRUE)
		{
			break;
		}
		
		if(length < DSP_FIRMWARE_FRAGMENT_LENGTH)
			bulk_len = length;
		else
			bulk_len = DSP_FIRMWARE_FRAGMENT_LENGTH;
		length -= bulk_len;

		//if B_MEM, should jump the rom area
		if(pAdap->wlan_attr.chipID && (file_id == 2))
		{
			if ( (consumed >= V4_BMEM_ROM_START) &&
				(consumed < (V4_BMEM_ROM_START + V4_BMEM_ROM_LEN)) ) 
			{
                
                		consumed += bulk_len;
                		continue; // while(length)
            		}
		}

		Status =  Adap_Dspfw_Download(
					pAdap, 
					pbuf + consumed, 
					bulk_len,
					(UINT16)consumed,
					file_id
				); 
		//ASSERT(STATUS_SUCCESS == Status);
		if (Status != STATUS_SUCCESS)//Justin: 0802. Don't continue to download other if encounter failure
		{
			break;
		}
		consumed += bulk_len;
	}


	return Status;	
}


UINT32 fcs_generation(UINT8 *input_array, UINT32 num_of_bytes)
{
    UINT32   i,j;
    UINT32   temp_value = 0;
    UINT32   fcs_value = 0xFFFFFFFF;
    UINT32   poly = 0x04C11DB7;
    UINT8   input_byte;

    for (i=0; i<num_of_bytes; i++)
	{
		input_byte = input_array[i];
        for(j = 0; j < 8; j++)
        {
            temp_value = (input_byte & 0x1) ^ ((fcs_value >> 31) & 0x1);
            fcs_value = (fcs_value << 1);
            
            if (temp_value == 1) 
                fcs_value = fcs_value ^ poly;

            input_byte = input_byte >> 1;
        }
    }

    fcs_value = fcs_value ^ 0xFFFFFFFF;

    temp_value = 0x0;
    for(i = 0; i < 32; i++)
    {
        temp_value = temp_value << 1;
        temp_value = temp_value | ((fcs_value >> i) & 0x1);
    }

    fcs_value = temp_value;
    return fcs_value;
}


#define DUALMODE_TX_PATCH	10
#define RF_STABLE_DELAY		50
/*
	init fcs value for cts to self mechinsm 
	the feature will be changed in v4 chip
*/
VOID cts_frame_fcs_lookup_init(PDSP_ADAPTER_T pAdap)
{
	   UINT32              cts_frame[3] = { 0x000000c4, 
				  				      0x00000000,
								      0x00000000};
	 
	   UINT32              i, duration;
	   UINT32 		tmp1,tmp2;
	   UINT32 		fcs_tmp;
   

	//woody changing
	WLAN_COPY_ADDRESS(&cts_frame[1],pAdap->current_address);

	//tmp1 = _3dsp_le32_to_cpu(cts_frame[1]);
	//tmp2 = _3dsp_le32_to_cpu(cts_frame[2]);
	tmp1 = cts_frame[1];
	tmp2 = cts_frame[2];

	
	// first two words are the MAC address
	VcmdW_3DSP_Dword(pAdap,tmp1,SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE);
	VcmdW_3DSP_Dword(pAdap,tmp2,SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE +sizeof(UINT32));

	

	//---------------------------------------------------------------------------------
	// 1) The corresponding timer count for 3.75ms is 
	//    3.75ms_TMR_COUNT = 3750 * 160 / MODE_SWITCH_TMR_FACTOR
	//    for MODE_SWITCH_TMR_FACTOR = 256, 3.75ms_TMR_COUNT = 2344
	// 2) 3.75ms_TMR_COUNT / 37 = 63.35, the closest power of 2 is 64
	// 3) 64 corresponding to (64 * MODE_SWITCH_TMR_FACTOR) / 160 = 102.4us
	// 4) Hence, each step should be 103us
	// 5) With 37 entries, the CTS can cover maximum 103 * 37 = 3811us
	//---------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------
	//  WHQL
	//---------------------------------------------------------------------------------
	for (i=0; i<37; i++)
	{
		duration = 312 + RF_STABLE_DELAY + DUALMODE_TX_PATCH;			// The value matches the one in dualmode.h
		duration += i*103;
		cts_frame[0] = (cts_frame[0] & 0x0000FFFF) | (duration << 16);
		fcs_tmp = fcs_generation((UINT8 *)cts_frame, 10);
		VcmdW_3DSP_Dword(pAdap, fcs_tmp,(UINT16)(SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE + (i+2)*4 ));
	}

	//---------------------------------------------------------------------------------
	//  SCO_ADJ
	//---------------------------------------------------------------------------------
	for (i=37; i<74; i++)
	{
		duration = 1250 + RF_STABLE_DELAY + DUALMODE_TX_PATCH;			// The value matches the one in dualmode.h
		duration += (i-37)*103;
		cts_frame[0] = (cts_frame[0] & 0x0000FFFF) | (duration << 16);
		fcs_tmp = fcs_generation((UINT8 *)cts_frame, 10);
		VcmdW_3DSP_Dword(pAdap, fcs_tmp,(UINT16)(SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE + (i+2)*4 ));
	}

	//---------------------------------------------------------------------------------
	//  BT_IDLE / SCO_MIX
	//---------------------------------------------------------------------------------
	for (i=74; i<111; i++)
	{
		duration = 5000 + RF_STABLE_DELAY + DUALMODE_TX_PATCH;		// The value matches the one in dualmode.h
		duration += (i-74)*103;
		cts_frame[0] = (cts_frame[0] & 0x0000FFFF) | (duration << 16);
		fcs_tmp = fcs_generation((UINT8 *)cts_frame, 10);
		VcmdW_3DSP_Dword(pAdap, fcs_tmp,(UINT16)(SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE + (i+2)*4 ));
	}
	//---------------------------------------------------------------------------------
	//  ACL_ONLY
	//---------------------------------------------------------------------------------
	for (i=111; i<148; i++)
	{
		duration = 7500 + RF_STABLE_DELAY + DUALMODE_TX_PATCH;		// The value matches the one in dualmode.h
		duration += (i-111)*103;
		cts_frame[0] = (cts_frame[0] & 0x0000FFFF) | (duration << 16);
		fcs_tmp = fcs_generation((UINT8 *)cts_frame, 10);
		VcmdW_3DSP_Dword(pAdap, fcs_tmp,(UINT16)(SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE + (i+2)*4 ));
	}
	//---------------------------------------------------------------------------------
	//  ACL_MIX
	//---------------------------------------------------------------------------------
	for (i=148; i<185; i++)
	{
		duration = 12500 + RF_STABLE_DELAY + DUALMODE_TX_PATCH;		// The value matches the one in dualmode.h
		duration += (i-148)*103;
		cts_frame[0] = (cts_frame[0] & 0x0000FFFF) | (duration << 16);
		fcs_tmp = fcs_generation((UINT8 *)cts_frame, 10);
		VcmdW_3DSP_Dword(pAdap, fcs_tmp,(UINT16)(SCRATCH_BASE_ADDR + SCRATCH_CTS_FCS_TABLE + (i+2)*4 ));
	}
		
 }

TDSP_STATUS
Adap_set_auto_corr(
	PDSP_ADAPTER_T pAdap
	)
{
	UINT32  		val;
	//UINT32           tmp;

	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_20_23);

	if(pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr)
	{
		val |= BITS_HOST_CTRL_FEATURE__CORRELATOR;
	}
	else
	{
		val = val &~BITS_HOST_CTRL_FEATURE__CORRELATOR;
	}


	//woody debug for combo
	//if(pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
	if(pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze == 1)
	{
		val = val | BITS_SOFT_DOZE_BIT;
	}
	else
	{
		val = val &~BITS_SOFT_DOZE_BIT;
	}

	//
	if(pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated== 1)
	{
		val = val | BITS_AP_ASSOCIATED_BIT;
	}
	else
	{
		val = val &~BITS_AP_ASSOCIATED_BIT;
	}


	//VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_20_23);
	VcmdW_3DSP_Mailbox(pAdap, val, WLS_MAC__BBREG_20_23,(PVOID)__FUNCTION__);

#if 0	
	//triger gen_int1
	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_36_38);
	val |= BITS_BBREG_36_38__GEN_INT_1;
	VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_36_38);
#endif

	return STATUS_SUCCESS;
}



/*
Name					: mmacRestart_initial()
return value			: MMACSTATUS
global variables 		: void
description				: It initializes MiniMAC.
special considerations 	: It needs to be preceeded by mac_init().
see also				: 
TODO					: none
*/
TDSP_STATUS
Adap_mmacRestart_initial(
	PDSP_ADAPTER_T pAdap,
	UINT32 		channel
	)
{
	TDSP_STATUS status = STATUS_SUCCESS;
	UINT32 	        val;
	//UINT32		 loopcnt;
	UINT8		tmp;

	//write auto_corr & preambel 
	pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr = 0;
	pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 0;
	pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze = 0;
	if(STATUS_SUCCESS != Adap_set_auto_corr(pAdap))
	{
		goto error_exit;
	}

	//write code status to MMAC_CORE_INIT_FLAG
	val = (UINT32)MMAC_CORE_INIT_FLAG;
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(
		           pAdap, val,
		           (UINT16)(WLS_SCRATCH__SP20_READY)))
	{
		goto error_exit;
	}

	//we don't set channel here
	//Jakio20070507:open it for debugging v4
	//set default channel to 11
	if(pAdap->wlan_attr.chipID)
	{
		if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap, 0xB, WLS_MAC__BBREG_16_19))
			goto error_exit;

		sc_sleep (1);
		//read chan
		tmp = Adap_get_channel(pAdap);
		Adap_set_usepd(pAdap,1);
	}
	else
	{
		pAdap->m2bMailbox1.channel_num = 0xB;
		pAdap->m2bMailbox1.bb_int = TRUE;
		pAdap->m2bMailbox1.gen_int1 = TRUE;  //jakio20070507
		if(STATUS_SUCCESS != Vcmd_scratch_2_DSP_Amemoffset(pAdap, &pAdap->m2bMailbox1,sizeof(m2bMailbox1_t),0))
		{
			goto error_exit;
		}
	}
	
	//Jakio20070521: moved code here according sheng, it's safer doing this before sp20/host handshaking
	//Jakio20070517: 
	status = Adap_bb_dsp_init(pAdap, 
			pAdap->wlan_attr.gdevice_info.dot11_type,
			pAdap->wlan_attr.gdevice_info.slot_time);
	if(STATUS_SUCCESS != status)
	{
		goto error_exit;
	}

	//
	if(STATUS_SUCCESS != Vcmd_mmacSp20ResetRemoved(pAdap))
	{
		goto error_exit;
	}

	

	//nic start
	// Enable NIC clocks and DMAs. Remove UniPHY core reset.
	// Wait for DSP initialization done
	if(STATUS_SUCCESS != Vcmd_CardBusNICStart(pAdap))
	{
		goto error_exit;
	}

	if(pAdap->wlan_attr.chipID)
	{
		//Jakio20070518: clear bit20~bit24, according sheng's advice
		val = VcmdR_3DSP_Dword(pAdap, WLS_CSR__PCI_CONTROL_REGISTER);
		val &= ~(BIT20|BIT21|BIT22|BIT23|BIT24);
		if(STATUS_SUCCESS !=VcmdW_3DSP_Dword(pAdap, val,WLS_CSR__PCI_CONTROL_REGISTER))
		{
			goto error_exit;
		}
	}
	
	
	return status;
error_exit:
	DBG_WLAN__MAIN(LEVEL_ERR ,"Adap_mmacRestart_initial error exit \n");
	return STATUS_FAILURE;
}
	


/*
Name					: mmacInit()
return value			: MMACSTATUS
global variables 		: void
description				: It initializes MiniMAC.
special considerations 	: It needs to be preceeded by mac_init().
see also				: 
TODO					: none
*/
TDSP_STATUS
Adap_mmacinit(
	PDSP_ADAPTER_T pAdap
	)
{
//	TDSP_STATUS status;
	UINT32 	        val;

	if(STATUS_SUCCESS != Vcmd_flush_rx_fifo(pAdap))
	{
		goto error_exit;
	}

	if(STATUS_SUCCESS != Vcmd_mmacSp20ResetApplied(pAdap))
	{
		goto error_exit;
	}


	/* disable 1. power-save INT caused by DZS,
               2. mac_receive INT caused by SR,
			   3. mac_transmit INT caused by ET
	*/
	//Jakio20070517: close it for v4
	if(!pAdap->wlan_attr.chipID)
	{
		val = 0x29;
		if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,val,WLS_MAC__BBREG_24_27))
		{
			goto error_exit;
		}	
	}

	//reset dma 0
	val = VcmdR_3DSP_Dword(pAdap,WLS_CSR__PCI_CONTROL_REGISTER);
	val |= DMA0_ENABLE_BIT;

	VcmdW_3DSP_Dword(pAdap,val,WLS_CSR__PCI_CONTROL_REGISTER);

#if 0	//Justin: ddddebug
	//init m2bMailbox
	{
	    UINT8 buff1[8]={0x1,0x0,0x0,0x0,0x1,0x0,0x0,0x0};
           sc_memory_copy(&pAdap->m2bMailbox1,buff1,8);

  	Vcmd_scratch_2_DSP_Amemoffset(pAdap,buff1,sizeof(m2bMailbox1_t),0);
		   
	}
#endif

	//
//	cts_frame_fcs_lookup_init(pAdap);
	{
		//UINT32 txpow_default[3]={ 0x30303030, 0x2828221a, 0x2e2e2727};		
		//UINT32 txpow_default[3]={ 0x30303030, 0x36322e2a, 0x36322e2a};	
		UINT32 txpow_default[3]={ 0x36303030, 0x36322e2a, 0x36322e2a};	//Justin: 0712. edit ch11 to power level 54, reference from test result by mfp
		//UINT32 txpow_default[3]={ 0x30303030, 0x2828221a, 0x2828221a};			
		VcmdW_3DSP_Dword(pAdap, txpow_default[0], WLS_SCRATCH__TXPOWER_TABLE);
		VcmdW_3DSP_Dword(pAdap, txpow_default[1], WLS_SCRATCH__TXPOWER_TABLE + 4);
		VcmdW_3DSP_Dword(pAdap, txpow_default[2], WLS_SCRATCH__TXPOWER_TABLE + 8);	
	}

	//
	//
	//update it later
	//how to use channel 
	if(STATUS_SUCCESS != Adap_mmacRestart_initial(pAdap,pAdap->wlan_attr.current_channel))  //set channel 11
	{
		goto error_exit;
	}

	return STATUS_SUCCESS;
error_exit:
	DBG_WLAN__MAIN(LEVEL_ERR ,"Adap_mmacinit error exit \n");
	return STATUS_FAILURE;
}


/*
Name					: mmacV4Init()
return value			: MMACSTATUS
global variables 		: void
description				: It initializes MiniMAC for v4chip.
special considerations 	: It needs to be preceeded by mac_init().
see also				: 
TODO					: none
*/
TDSP_STATUS
Adap_mmacV4init(
	PDSP_ADAPTER_T pAdap
)
{
//	TDSP_STATUS status;
	UINT32 	        val;

	if(STATUS_SUCCESS != Vcmd_flush_rx_fifo(pAdap))
	{
		goto error_exit;
	}

	//Jakio20070521: close it according Sheng. 
 	/*
	if(STATUS_SUCCESS != Vcmd_mmacSp20ResetApplied(pAdap))
	{
		goto error_exit;
	}
	*/


	//v3chip,v4chip
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,  
		    MAC_BBREG_ACCESSS(BBREG_ACCESS_EN, BBREG_ACCESS_WRITE, 0x29, 24),
		    WLS_MAC__BBREG_ACCESS_WD))
	{
		goto error_exit;
	}

	#if 1	//us said don't do this.	080226
	//but now find pci has this bit in code.
	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	//val =val &(~ MAC_ENABLE_ECO_ITEM17);
	val =val |(MAC_ENABLE_ECO_ITEM17);



	if(STATUS_SUCCESS !=VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__CONTROL))
	{
		goto error_exit;
	}
	#endif
		
	//Jakio20070523: clear bit17~bit31
	//Jakio20070518: clear bit20~bit24
	val = VcmdR_3DSP_Dword(pAdap, WLS_CSR__PCI_CONTROL_REGISTER);
	val &= 0x0001ffff;
	if(STATUS_SUCCESS !=VcmdW_3DSP_Dword(pAdap, val,WLS_CSR__PCI_CONTROL_REGISTER))
	{
		goto error_exit;
	}

	//
	cts_frame_fcs_lookup_init(pAdap);
	//
	//update it later
	//how to use channel 
	if(STATUS_SUCCESS != Adap_mmacRestart_initial(pAdap,pAdap->wlan_attr.current_channel))  //set channel 11
	{
		goto error_exit;
	}

	return STATUS_SUCCESS;
error_exit:
	DBG_WLAN__MAIN(LEVEL_ERR ,"Adap_mmacinit error exit \n");
	return STATUS_FAILURE;
}


TDSP_STATUS
Adap_w2lan_init(
	PDSP_ADAPTER_T pAdap,
	dot11_type_t	dot11_type,
	UINT32		mac_mode
	)
{
//	TDSP_STATUS status;


	//get version
	pAdap->wlan_attr.gdevice_info.version = Vcmd_get_version(pAdap);
	if((pAdap->wlan_attr.gdevice_info.version & UM_VERSION_BITS) != UM_VERSION)
	{
		DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] Version Mismatch\n",__FUNCTION__);
 //       	goto error_exit;
	}

	//check if hw support ps feature
#ifdef POWER_SAVE_SUPPORT
	if((pAdap->gdevice_info.version & PS_SUPPORT) != PS_SUPPORT)
	{
		DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] Power Save Feature is not supported\n",__FUNCTION__);
        	goto error_exit;
	}
#endif /* POWER_SAVE_SUPPORT */


	//check if hw support encrytion
	if((pAdap->wlan_attr.gdevice_info.version & ENCRYPTION_SUPPORT) != ENCRYPTION_SUPPORT)
	{
		DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] Encryption Feature is not supported\n",__FUNCTION__);
 //       	goto error_exit;
	}
	
	
	#if 0 //Jakio20070518: close it for v4 test
	//reset mac 
	if(STATUS_SUCCESS != Vcmd_hal_reset(pAdap))
	{
        	goto error_exit;
	}
	#endif

	//Jakio20070517: rewrite 0x4078, 'cause it has been cleared in Vcmd_hal_rest()
	//v3chip,v4chip
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,  
		    MAC_BBREG_ACCESSS(BBREG_ACCESS_EN, BBREG_ACCESS_WRITE, 0x29, 24),
		    WLS_MAC__BBREG_ACCESS_WD))
	{
		goto error_exit;
	}
	//it is suggested that alloc tx fifo size again with state = idle
	//woody 080630
	//if(STATUS_SUCCESS != Adap_alloc_tx_fifo_size(pAdap,pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA))
	//{
       // 	goto error_exit;
	//}
	Adap_request_init_txfifo(pAdap);

#if 0 //Justin: 0810 don't do this , it may write invalide address call this function by initialize
//*	//Justin: 0617, open
//set mac address after reset, necessary
	if(STATUS_SUCCESS != Adap_WriteMacAddress(pAdap,pAdap->current_address))
	{
        	goto error_exit;
	}
//*/
#endif
	//set phy mode
	//ASSERT(IEEE802_11_G == pAdap->wlan_attr.gdevice_info.dot11_type);
	DBG_WLAN__MAIN(LEVEL_TRACE,"[%s] wlan_init phy mode = %x \n",__FUNCTION__,dot11_type);
	
	if(STATUS_SUCCESS != Adap_mac_set_dot11type(
			pAdap,
			dot11_type,
			mac_mode,
			FALSE))
	{
        	goto error_exit;
	}

#if 0		//Justin: 070425... close it, do not need to set channel in init
	if(STATUS_SUCCESS !=Adap_set_channel(pAdap,1))
	{
        	goto error_exit;
	}
/*	//enable interrupt
	if(STATUS_SUCCESS != Vcmd_NIC_INTERRUPT_ENABLE(pAdap))
	{
		goto error_exit;
	}
	*/
{
	UINT8 uchen;
	sc_sleep(20000);
	uchen = Adap_get_channel(pAdap);
}
#endif
	return STATUS_SUCCESS;
error_exit:
	DBG_WLAN__MAIN(LEVEL_ERR ,"Adap_w2lan_init error exit \n");
	return STATUS_FAILURE;
}


/*
	the routine called after reset happen, 
	Wlan attribute will be setting into the hw by the function
*/

TDSP_STATUS
Adap_w2lan_resetting(
	PDSP_ADAPTER_T pAdap
	)
{
    UINT32   tmp1,tmp2;
//    UINT32 reg;
    PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	UINT16 cap;
    
	//restore key
	if(STATUS_SUCCESS != Adap_key_hw_to_false(pAdap))
	{
        	goto error_exit;
	}		

	if(STATUS_SUCCESS != Adap_push_key_into_hw(pAdap))
	{
        	goto error_exit;
	}		

    //write auto_corr 
    if(STATUS_SUCCESS != Adap_set_auto_corr(pAdap))
    {
        goto error_exit;
    }		

    /*		
    //	reg = (UINT32)pAdap->wlan_attr.gdevice_info.mac_ctl_reg;
    if(STATUS_SUCCESS !=Adap_set_mac_ctlr_reg(pAdap, 
    Adap_build_mac_ctrl_reg(pAdap,pt_mng->usedbssdes.networkType,WLAN_MACMODE_ESS_STA)))
    {
    goto error_exit;
    }		
    
      if(STATUS_SUCCESS != 
      VcmdW_3DSP_Dword(pAdap,reg,WLS_MAC__CONTROL))
      {
      goto error_exit;
      }		
    */
    //set support rate and slot and so on according to phy mode
    sc_memory_copy((PUINT8)&cap,(PUINT8)&pAdap->wlan_attr.capability,sizeof(UINT16));
    if(STATUS_SUCCESS != 
        Adap_mac_set_dot11type(
        pAdap,
        pAdap->wlan_attr.gdevice_info.dot11_type,
        pAdap->wlan_attr.macmode,
        CAP_EXPRESS_SHORT_SLOTTIME(cap)))
    {
        goto error_exit;
    }		

	//set mac address
	if(STATUS_SUCCESS != Adap_WriteMacAddress(pAdap,pAdap->current_address))
	{
        	goto error_exit;
	}		

	//set bssid
	if(STATUS_SUCCESS != Vcmd_hal_set_bssid(pAdap,pAdap->wlan_attr.bssid))
	{
        	goto error_exit;
	}		

    //set  channel
    if(STATUS_SUCCESS !=
        Adap_set_channel(pAdap,pAdap->wlan_attr.current_channel))
    {
        goto error_exit;
    }

	//set beacon interval
	if(STATUS_SUCCESS != Vcmd_set_beacon_interval(pAdap,
		(UINT32)pAdap->wlan_attr.beacon_interval,50))
	{
        	goto error_exit;
	}		
	//close due to maybe hardware not ready
	//this will cause bulk out stall after hibernate with ibss wep mode
#if 0
	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)	//ibss mode.	justin:	080506. must recover this after hardware reset.
	{
		Mng_MakeBeaconOrProbersp(pAdap,WLAN_FSTYPE_BEACON,NULL);
	}
#endif	

    /*
    Adap_set_join_para(pAdap,&pt_mng->usedbssdes);
    
      reg = REG_STATE_CONTROL(DEV_ACTIVE, (0 & 0x01), 50);
      VcmdW_3DSP_Dword(pAdap, reg,WLS_MAC__STATE_CONTROL);
    */
	//rts
	tmp1 = (UINT32)pAdap->wlan_attr.retry_limit.short_retry_limit;
	tmp2 = (UINT32)pAdap->wlan_attr.retry_limit.long_retry_limit;
	
	if(STATUS_SUCCESS != Vcmd_set_rts_retrylimit(pAdap,
	    pAdap->wlan_attr.rts_threshold,
	    tmp1,
	    tmp2))	    
	{
        	goto error_exit;
	}		


	//Restore backoff/slot reg
	Mng_Update_Cts_State(pAdap,pt_mng->usedbssdes.Erpinfo);
	return STATUS_SUCCESS;
error_exit:
	DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_w2lan_resetting error exit \n");
	return STATUS_FAILURE;	
}


TDSP_STATUS
Adap_mac_init(
	PDSP_ADAPTER_T pAdap,
	dot11_type_t	dot11_type,
	UINT32 		mac_mode	
	)
{
//	UINT8  chan;  //Jakio20070514: for test
//	UINT32  tmp;
	//w2lan_init
	//dsp sta code, run the code at this position ??
	//chipid
	if(pAdap->wlan_attr.chipID != 0)
	{
		if(STATUS_SUCCESS != Adap_mmacV4init(pAdap))
		{
	        	goto error_exit;
		}	

	}
	else
	{
		if(STATUS_SUCCESS != Adap_mmacinit(pAdap))
		{
	        	goto error_exit;
		}
	}

	if(STATUS_SUCCESS != Adap_w2lan_init(pAdap,dot11_type,mac_mode))
	{
        	goto error_exit;
	}		


	//Jakio20070518: set antenna
	if(STATUS_SUCCESS != Adap_set_antenna(pAdap, (UINT8)pAdap->wlan_attr.antenna_num))
	//if(STATUS_SUCCESS != Adap_set_antenna(pAdap, (UINT8)pAdap->wlan_attr.gdevice_info.antenna_sel))
	{
        	goto error_exit;
	}
	

	//other operation
	return STATUS_SUCCESS;
error_exit:
	DBG_WLAN__MAIN(LEVEL_ERR ,"Adap_mac_init error exit \n");
	return STATUS_FAILURE;		
}

/*
TDSP_STATUS
pAdap_mmacReset(
	PDSP_ADAPTER_T pAdap
	)
{
	UINT32 val;
	UINT8 ch;
	if(STATUS_SUCCESS != Vcmd_flush_rx_fifo(pAdap)) // flush it 
	{
        	goto error_exit;
	}		
	if(STATUS_SUCCESS != Vcmd_mmacSp20ResetApplied(pAdap))
	{
        	goto error_exit;
	}		
	//WriteToReg( pAdapter, WLS_MAC_BBREG24_27_WD, 0x29);
	val = 0x29;
	if(STATUS_SUCCESS != VcmdW_3DSP_Dword(pAdap,val,WLS_MAC__BBREG_24_27))
	{
        	goto error_exit;
	}		
	ch = pAdap->wlan_attr.current_channel;
	if(STATUS_SUCCESS != Adap_mmacRestart_initial(pAdap,ch))
	{
        	goto error_exit;
	}		

	return STATUS_SUCCESS;
error_exit:
	DBGSTR(("Adap_mac_init error exit \n"));
	return STATUS_FAILURE;				
}

*/
TDSP_STATUS
Adap_WLSMAC_initialize(
	PDSP_ADAPTER_T 	pAdap,
	dot11_type_t		dot11_type,
	UINT32 			mac_mode
	)
{
	TDSP_STATUS status;

	pAdap->mac_init_done = FALSE;
	status = Adap_mac_init(pAdap,dot11_type,mac_mode);
	
	if(STATUS_SUCCESS == status)
	{	
		pAdap->mac_init_done = TRUE;
	}
	return status;
}

/*
	passive level 
	called by task process
*/
TDSP_STATUS
Adap_reset_dsp_chip(
	PDSP_ADAPTER_T pAdap
	)
{
	TDSP_STATUS   status;

	status = STATUS_FAILURE;
	
	sc_sleep (2);

	Vcmd_NIC_INTERRUPT_ENABLE(pAdap);
		    //success reset
		    //Vcmd_NIC_ENABLE_RETRY(pAdap,CARDBUS_NUM_OF_RETRY);
	sc_sleep (MDIS_RESET_TIMEOUT);
		    
    		    //		   status = Adap_mmacinit(pAdap);	//Justin: replace with the following setion
	if(pAdap->wlan_attr.chipID != 0)
	{
		status = Adap_mmacV4init(pAdap);
	}
	else
	{
		status = Adap_mmacinit(pAdap);
	}
    
	if (status != STATUS_SUCCESS)
	{
		goto exit_reset_dsp_chip;
	}
	
	status =Adap_w2lan_init(	pAdap,
						        pAdap->wlan_attr.gdevice_info.dot11_type,
						        pAdap->wlan_attr.macmode);

	if(STATUS_SUCCESS != Adap_set_antenna(pAdap, (UINT8)pAdap->wlan_attr.antenna_num)) 
	{
		goto exit_reset_dsp_chip;
	}
    
    
    
	//enable interrupt
	Vcmd_unmask_idle_intr(pAdap);
    
	//	sc_sleep(1000); // delay 1ms
	Vcmd_set_next_state(pAdap,DEV_IDLE, 0,0);
	sc_sleep(1); // delay 1ms
	
    
 	
    //wlan_timer_start(&pAdap->sys_timer,TIMER_1MS_DELAY);    //we do reset setting process in timer routine
exit_reset_dsp_chip:
	return status;
}


TDSP_STATUS Adap_bb_dsp_init(
	PDSP_ADAPTER_T pAdap,
	dot11_type_t	dot11_type,
	UINT8 		slot_time
	)
{
	//UINT8 slot_time,dot11Type;
	UINT32 reg_value;
	UINT32 reg_edca;
		switch (dot11_type){
		    case IEEE802_11_A:
				reg_edca = EDCAPARAMETER0_FOR_11G;
				break;
		    case IEEE802_11_B:
				reg_edca = EDCAPARAMETER0_FOR_11B;
				break;
		    case IEEE802_11_G:
				reg_edca = EDCAPARAMETER0_FOR_11G;
		        break;
				
		    default:
				DBG_WLAN__MAIN(LEVEL_ERR ," Unrecognise phisical mode?[%d] \n ", dot11_type);
				return STATUS_SUCCESS;		
    }

		reg_value = ((BB_CLK_FREQ) | (TX_DELAY_SLOT << OFFSET_TIMINGS_1REG__TX_DELAY_SLOT) | 			\
				    (TX_DELAY_SIFS << OFFSET_TIMINGS_1REG__TX_DELAY_SIFS) | 
				    (RX_DELAY_B << OFFSET_TIMINGS_1REG__RX_DELAY_B) | 
				    (RX_DELAY_A << OFFSET_TIMINGS_1REG__RX_DELAY_A) |
	                		    (MAC_TIME << OFFSET_TIMINGS_1REG__MAC_PROC_DELAY) | 
	                		    (RXDELAYB_US << OFFSET_TIMINGS_1REG__RX_DELAY_US));
	        VcmdW_3DSP_Dword(pAdap, reg_value, WLS_MAC__TIMINGS_1REG);
			
	        // disable QoS mode
		VcmdW_3DSP_Dword( pAdap, MAC_QOS_DIS_BIT, WLS_MAC__QOS_DISABLE_WD);

		//us said set bit 11 to 0.	080226
		//VcmdW_3DSP_Dword(pAdap, MAC_DEBUG_PERFCNT_EN_BIT,WLS_MAC__DEBUG0_WD);	
		reg_value = VcmdR_3DSP_Dword(pAdap,WLS_MAC__DEBUG0_WD);
		//should set the bit for clear perfcount reg by reading
		//otherwise perfcount reg can't be cleared by reading
		//reg_value = reg_value &(~MAC_DEBUG_PERFCNT_EN_BIT);
		reg_value |= MAC_DEBUG_PERFCNT_EN_BIT;

		//wifi 080829
		reg_value &= ~(MAC_DEBUG_BCNBACKOFF_WREN | 
						MAC_DEBUG_PRIBACKOFF_WREN |
						MAC_DEBUG_CPBACKOFF_WREN);
		VcmdW_3DSP_Dword(pAdap, reg_value,WLS_MAC__DEBUG0_WD);	

		reg_value = (V4T_CCA_A) | ((V4T_CCA_B + 1) << 11) 		\
			    | (slot_time << 22) | (RXDELAYA_US << 27);
		//VcmdW_3DSP_Dword(pAdap, (slot_time << 22) | reg_value & (~V4MAC_SLOT_TIME_BIT),WLS_MAC__TIMINGS_2REG);
		VcmdW_3DSP_Dword(pAdap, reg_value, WLS_MAC__TIMINGS_2REG);

		//set slot time by int
		Adap_update_slot_time(pAdap,slot_time);	
		//set aifsn
		reg_value = VcmdR_3DSP_Dword(pAdap,WLS_MAC__CP_BO_CNT);
		VcmdW_3DSP_Dword(pAdap,(reg_value &(~MAC_AIFSN_BIT)) | (2 << 10),WLS_MAC__CP_BO_CNT); 


		return STATUS_SUCCESS;
}

UINT32 Adap_build_mac_ctrl_reg(
	PDSP_ADAPTER_T pAdap,
	dot11_type_t      dot11_type,
	UINT32		mac_mode	
	)
{
	// zyy: read the total origin values in control registry, and then set the specifically bits ????
	MAC_CTL_REG_T  	reg;
	UINT32 			value;

	sc_memory_set(&reg, 0, sizeof(MAC_CTL_REG_T));

	value = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	//reg = (MAC_CTL_REG_T) value;
	sc_memory_copy(&reg,&value,sizeof(UINT32));
	//
	if( IEEE802_11_A == dot11_type)
	{
		reg.dot11a = 1;
		reg.dot11g = 0;
		reg.setacw = 0;
	}
	else if( IEEE802_11_G == dot11_type)
	{
		reg.dot11a = 0;
		reg.dot11g = 1;
		//close it for v4chip
		//reg.setacw = 1;
		reg.setacw = 0;
	}
	else
	{
		reg.dot11a = 0;
		reg.dot11g = 0;
		reg.setacw = 0;
	}

	//wifi 080829
	//for beacon fifo, the setting work
	//set to 1,  get aCWMin of 31 for IBSS Beacon BackOff.
	if(WLAN_MACMODE_IBSS_STA== mac_mode )
	{
		reg.enableEcoItem = 1;	//pci set to 1 for ibss
		reg.dupfilteren =0 ;
		reg.rxAnyBssidBeacon = 0;
		DBG_WLAN__MAIN(LEVEL_TRACE,"Set beacon backoff window to 31 for IBSS \n");
	}
	else
	{
		reg.dupfilteren = 1;
		reg.enableEcoItem = 0;	//us said set to 0.	080226
		reg.rxAnyBssidBeacon = 1;
	}

	//set bss type
	reg.isap = 0;
	reg.bsstype = (WLAN_MACMODE_ESS_STA== mac_mode ) ? 1:0;

	//
	
	reg.promen = 0;
	reg.fcscheckdisable = 0;
	reg.multirxdis = 0;	// Justin: for join,auth...  do not receive multicast frame, it may cause tx stop if it is encrypted 
	reg.wrUndecryptedRx = 0;
	reg.Ignore_ID_on_receive = 0;//Justin: 081118. Set 1 to Ignore Key ID on RX. Must set to 0, to fix bug 3339

	pAdap->wlan_attr.gdevice_info.mac_ctl_reg = reg;
	sc_memory_copy(&value,&reg,sizeof(UINT32));
	return value;
}

TDSP_STATUS Vcmd_mac_set_dot11type_for_scan(PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type)
{
	Adap_set_mac_ctlr_reg(pAdap, 
	Adap_build_mac_ctrl_reg(pAdap,dot11_type,WLAN_MACMODE_ESS_STA));
	return STATUS_SUCCESS;
}

/*
	set support rate list
	set sifs value according both phy mode and cap of ap
	set slot time value according both phy mode and cap of ap

	Adap_build_mac_ctrl_reg(pAdap)  and 
	vcmd_set_mac_ctlr_reg(pAdap) should be called after the function
	to write mac control register

	According to 3dsp spec,
	1.	20us for 802.11b mode of operation of the MAC.
	2.	9us for 802.11a mode of operation of the MAC.
	3.	20us for normal 802.11g mode of operation of the MAC.
	4.	9us for shortSlot option of 802.11g mode of operation of the MAC.

*/
TDSP_STATUS Adap_mac_set_dot11type(
	PDSP_ADAPTER_T pAdap,
	 dot11_type_t dot11_type,
	 UINT32		mac_mode,
	 BOOLEAN short_slot)
{
//	UINT32 dsp_version;
	PDSP_ATTR_T  pattr = (PDSP_ATTR_T)&pAdap->wlan_attr;
	UINT32 	ctl_reg;

	//DBG_WLAN__MAIN(LEVEL_TRACE ,"pattr->support_rate_len = %d\n",pattr->support_rate_len);
	Adap_set_support_rate(pAdap,dot11_type,pattr->support_rate,&pattr->support_rate_len);
	//DBG_WLAN__MAIN(LEVEL_TRACE ,"pattr->support_rate_len = %d\n",pattr->support_rate_len);
	
	switch (dot11_type)
	{
	case IEEE802_11_A:
		pattr->gdevice_info.sifs = 16; 					//16 micro sec
		pattr->gdevice_info.slot_time = SLOT_TIME_SHORT_DOT11G;				// 9 micro sec

		//update it later
		//pAdap->m2bMailbox1.channel_num = channel;
		pAdap->m2bMailbox1.bb_int = TRUE;

		/*hal_set_dot11g_type((uint8 *)ctldev->base_addr, 0);
        	hal_set_contention_window_min((uint8 *)ctldev->base_addr, 0); //ct for sta_v2 chip
        	hal_set_reg((uint8 *)ctldev->base_addr, WLS_MAC__CP_BO_CNT, 0x400); //ct for sta_v2 chip
		hal_set_dot11a_type((uint8 *)ctldev->base_addr, 1);*/
		break;
		
	case IEEE802_11_B:
		pattr->gdevice_info.sifs = 10; 					//16 micro sec
		pattr->gdevice_info.slot_time = SLOT_TIME_LONG_DOT11G;				// 9 micro sec

		
		/*hal_set_dot11g_type((uint8 *)ctldev->base_addr, 0);
        	hal_set_contention_window_min((uint8 *)ctldev->base_addr, 0); //ct for sta_v2 chip
        	hal_set_reg((uint8 *)ctldev->base_addr, WLS_MAC__CP_BO_CNT, 0x400); //ct for sta_v2 chip
		hal_set_dot11a_type((uint8 *)ctldev->base_addr, 0);*/
		break;
		
	case IEEE802_11_G:
		//pattr->gdevice_info.sifs = 16;	//if OFDM: 10 us sifs + 6 us idle time
		pattr->gdevice_info.sifs = 10;	//if OFDM: 10 us sifs + 6 us idle time
		pattr->gdevice_info.slot_time = (short_slot) ? SLOT_TIME_SHORT_DOT11G:SLOT_TIME_LONG_DOT11G;		

		/*
		hal_set_dot11g_type((uint8 *)ctldev->base_addr, 1);
        	hal_set_contention_window_min((uint8 *)ctldev->base_addr, 1); //ct for sta_v2 chip
        	hal_set_reg((uint8 *)ctldev->base_addr, WLS_MAC__CP_BO_CNT, 0x400); //ct for sta_v2 chip
		hal_set_dot11a_type((uint8 *)ctldev->base_addr, 0);*/
		break;
		
		
	default:
		DBG_WLAN__MAIN(LEVEL_ERR ,"Unknown dev type ! 11a ? 11b ? 11g ? \n");
		return STATUS_FAILURE;
	}


	//set for preamble attribute
	//preable should be decided by two factors
	//first is phy mode
	//     11a:  short
	//     11b:  long
	//     11g:  short
	//second is attribue of AP
	//    if AP don't support, we don't support
	/*if(PREAMBLE_TYPE_LONG == pattr->preamble_type)
	{
		pattr->gdevice_info.sifs = 10; 					//16 micro sec
		pattr->gdevice_info.slot_time = 20;
	}
	else
	{
		pattr->gdevice_info.sifs = 16; 					//16 micro sec
		pattr->gdevice_info.slot_time = 9;				// 9 micro sec
	}*/
	


		

	//here for setting mac ctl reg
	ctl_reg = Adap_build_mac_ctrl_reg(pAdap,
								dot11_type,
								mac_mode);
	//clear basic rate bits
	ctl_reg =  ctl_reg & (~MAC_BASICRATESET_BITS_A);
	ctl_reg = ctl_reg | Adap_get_basicrate_map(pAdap,dot11_type);
	//set mac reg
	if(STATUS_SUCCESS != Adap_set_mac_ctlr_reg(pAdap,ctl_reg))
	{
		DBG_WLAN__MAIN(LEVEL_ERR ," Unknown dev type ! 11a ? 11b ? 11g ? \n");
		return STATUS_FAILURE;
	}


	//set ofdm 11g basic rate
	if(STATUS_SUCCESS != Adap_set_basicrate_map_ofdm11g(pAdap))
	{
		DBG_WLAN__MAIN(LEVEL_ERR ," fail in Adap_set_basicrate_map_ofdm11g \n");
		return STATUS_FAILURE;
	}

	
	//set timing reg
	if(STATUS_SUCCESS != Adap_bb_dsp_init(
		pAdap,
		dot11_type,
		pattr->gdevice_info.slot_time))
	{
		DBG_WLAN__MAIN(LEVEL_ERR ," Unknown dev type ! 11a ? 11b ? 11g ? \n");
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}





/* 
	set corresponding registers about scan procedure. 
	consider it later: 
	     if the procedure is slower starting from host. then we will move
	     the flow to mcu with corresponding API function defined.
*/
TDSP_STATUS Adap_set_scan_reg( PDSP_ADAPTER_T pAdap)
{

	PMNG_STRU_T     	pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
//	TDSP_STATUS status;
	UINT32 		 val;
//	UINT32 	        auto_corr = 0;

	DBG_WLAN__MAIN(LEVEL_INFO,"Adap_set_scan_reg \n");


	//pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr = 0;
	//woody debug
	pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr = 0;

	// 2010.4.26 wumin change pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 0 
	// to pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 1,
	// to improve scan result when combo mode.	
	pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 1;
	pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze = 1;
	if(STATUS_SUCCESS != Adap_set_auto_corr(pAdap))
	{
		return STATUS_FAILURE;
	}


      //0424   always flush beacon fifo
      //WOODY 080630
      //Vcmd_hal_select_tx_fifo(pAdap,3);
//	  Vcmd_flush_tx_fifo(pAdap);
	  

	//set phy mode,for scan procedure, phy mode saved in worflag
	if(STATUS_SUCCESS != Vcmd_mac_set_dot11type_for_scan(
					pAdap,pt_mng->statuscontrol.workflag))
	{
		return STATUS_FAILURE;
	}

	//received any beacon
	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	val |= MAC_RX_ANY_BSSID_BEACON_BIT;
	VcmdW_3DSP_Dword(pAdap,val, WLS_MAC__CONTROL);

	//set timing reg
	if(STATUS_SUCCESS != Adap_bb_dsp_init(
		pAdap,
		pt_mng->statuscontrol.workflag,
		pAdap->wlan_attr.gdevice_info.slot_time))
	{
		return STATUS_FAILURE;
	}
	
	
	//set state 
	if(STATUS_SUCCESS != Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0))
	{
		return STATUS_FAILURE;
	}	

#if 0
{
	//verify state
	UINT32 i = VENDOR_LOOP_MIN;

	while(i--)
	{
		sc_sleep(100);
		if( (UINT32)DEV_ACTIVE == Adap_get_current_state_control(pAdap))
		{
			break;
		}
	}

	if(i == 0)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_set_scan_reg, set state to active fail \n");
	}
}
#endif
	//set antenna???? 
	return STATUS_SUCCESS;
}

/*
	description:
	    the function sets basic rate
*/
TDSP_STATUS Adap_set_rate( PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type)
{
	UINT32 ctrl_reg, brs11gofdm_reg;
	TDSP_STATUS status = STATUS_SUCCESS;	
	UINT16 basic_rate_map;

	//set basic rate map
	//6,12,24 for 11a as basic rate
	basic_rate_map = (dot11_type == IEEE802_11_A) ? 0x15 : 0xf;
	//for 11b 0xf is basic rate map
	//for 11g 24mbps as max basic rate,and index of 24mbps is 8.
	basic_rate_map = (dot11_type == IEEE802_11_G) ? 0x8f : basic_rate_map;
	
	//set default rts rate to 24Mbps for OFDM, 1Mbps (really 11Mbps due to h/w bug) for DSSS
	//pAdap->wlan_attr.gdevice_info.rts_bspeed = IDX_24MBPS_BG;

	//set mac control register
#ifdef READ_WRITE_CTL_REG_NO_EVERY_TIME
	ctrl_reg = Adap_build_mac_ctrl_reg(pAdap,dot11_type,pAdap->wlan_attr.macmode);
	ctrl_reg = ctrl_reg & (~MAC_BASICRATESET_BITS_A);
	ctrl_reg = ctrl_reg | ((basic_rate_map & 0xFF) << 8);
	sc_memory_copy(&pAdap->wlan_attr.gdevice_info.mac_ctl_reg,&ctrl_reg,sizeof(UINT32));
#else
	ctrl_reg = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
	ctrl_reg = ctrl_reg & (~MAC_BASICRATESET_BITS_A);
	ctrl_reg = ctrl_reg | ((basic_rate_map & 0xFF) << 8);
	status = VcmdW_3DSP_Dword(pAdap, ctrl_reg, WLS_MAC__CONTROL); //set bit(8-15) as basic_rate_map
#endif	

	//set for 11g ofdm
	if(dot11_type == IEEE802_11_G) {
		brs11gofdm_reg = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BRS11G_OFDM);
		brs11gofdm_reg = brs11gofdm_reg & 0xFFFFFF00;
		brs11gofdm_reg = brs11gofdm_reg |0x15;
		status = VcmdW_3DSP_Dword(pAdap,brs11gofdm_reg, WLS_MAC__BRS11G_OFDM); //set bit(0-7) as basic_rate_map for ERP_OFDM
	}

	return status;
}

/*
	in fact, the routine is replace of adap_set_rate().
*/
UINT32 Adap_get_basicrate_map( PDSP_ADAPTER_T pAdap,dot11_type_t dot11_type)
{
	UINT32 basic_rate_map = 0;

	//here we fixed for basic rate map. we should change it later.
	//set basic rate map
	//6,12,24 for 11a as basic rate
	basic_rate_map = (dot11_type == IEEE802_11_A) ? 0x15 : 0xf;
	//for 11b 0xf is basic rate map
	//for 11g 24mbps as max basic rate,and index of 24mbps is 8.
	basic_rate_map = (dot11_type == IEEE802_11_G) ? 0x08f : basic_rate_map;
	
	//set default rts rate to 24Mbps for OFDM, 1Mbps (really 11Mbps due to h/w bug) for DSSS
	//pAdap->wlan_attr.gdevice_info.rts_bspeed = IDX_24MBPS_BG;
	basic_rate_map = (basic_rate_map & 0xFF) << 8;
	return basic_rate_map;
}

/*
	the routine sets mac reg 0x408c for 11g ofdm basic rate
*/
TDSP_STATUS Adap_set_basicrate_map_ofdm11g( PDSP_ADAPTER_T pAdap)
{
	BRS11GOFDM_REG_T reg;
	UINT32 value;


	//now ratelist not used. 
	//we fixed the reg to 0x15  //6.12.24 as basic rate
	sc_memory_set(&reg, 0, sizeof(BRS11GOFDM_REG_T));
	reg.ofdm_rate = 0x15;

	//
	if((pAdap->wlan_attr.cts_to_self_config == CTS_SELF_ENABLE ) &&
		(pAdap->wlan_attr.cts_en == CTS_SELF_ENABLE )) 
	{
		reg.ofdm_rts_rate = IDX_11MBPS_BG;
		//cts should be sent
		if(pAdap->wlan_attr.cts_current_state == RTSCTS_STATE_CTSSELF_WORK)
		{
			reg.ctstoself = 1;
		}		
	}
	else
	//normal setting	
	{
		if (pAdap->wlan_attr.rate >= OP_24_MBPS)
		{
			reg.ofdm_rts_rate = IDX_24MBPS_BG;
		}
		else if(pAdap->wlan_attr.rate >= OP_12_MBPS)
		{
			reg.ofdm_rts_rate = IDX_12MBPS_BG;
		}
		else if((pAdap->wlan_attr.rate == OP_6_MBPS) ||
			(pAdap->wlan_attr.rate == OP_9_MBPS))
		{
			reg.ofdm_rts_rate = IDX_6MBPS_BG;
		}
		//default
		else
		{
			reg.ofdm_rts_rate = IDX_1MBPS_BG;
			if(pAdap->wlan_attr.rate == OP_11_MBPS)
			{
				reg.dsss_rts_rate = IDX_11MBPS_BG;
			}
			else if((pAdap->wlan_attr.rate == OP_5_MBPS) ||
				(pAdap->wlan_attr.rate == OP_5_MBPS))
			{
				//have bug for 5.5 rts.
				reg.dsss_rts_rate = IDX_2MBPS_BG;
			}
			else
			{
				reg.dsss_rts_rate = IDX_1MBPS_BG;
			}
		}
	}//end normal

	sc_memory_copy(&value,&reg,sizeof(UINT32));

	return VcmdW_3DSP_Dword(pAdap,value, WLS_MAC__BRS11G_OFDM); //set bit(0-7) as basic_rate_map for ERP_OFDM
}


void Adap_set_power_table(PDSP_ADAPTER_T pAdap,UINT8 channel)
{

//#ifdef DSP_ASIC_DEBUG_FLAG
#if 0

		//UINT32 txpow_default[3]={ 0x30303030, 0x2828221a, 0x2e2e2727};		
		//UINT32 txpow_default[3]={ 0x30303030, 0x36322e2a, 0x36322e2a};	
		UINT32 txpow_default[3]={ 0x36303030, 0x36322e2a, 0x36322e2a};	//Justin: 0712. edit ch11 to power level 54, reference from test result by mfp
		//UINT32 txpow_default[3]={ 0x30303030, 0x2828221a, 0x2828221a};			
		VcmdW_3DSP_Dword(pAdap, txpow_default[0], WLS_SCRATCH__TXPOWER_TABLE);
		VcmdW_3DSP_Dword(pAdap, txpow_default[1], WLS_SCRATCH__TXPOWER_TABLE + 4);
		VcmdW_3DSP_Dword(pAdap, txpow_default[2], WLS_SCRATCH__TXPOWER_TABLE + 8);	

#else

	UINT8 index,i = 0;
	while(static_axLookupFreqByChannel_BOnly[i].channel)
	{
		if(static_axLookupFreqByChannel_BOnly[i].channel == channel)
			break;
		i++;
	}

	if(static_axLookupFreqByChannel_BOnly[i].channel == 0)//not found channel request
		return;
	
	index = i*3;

	VcmdW_3DSP_Dword(pAdap, pAdap->TxPwrTable[index] , WLS_SCRATCH__TXPOWER_TABLE);
	VcmdW_3DSP_Dword(pAdap, pAdap->TxPwrTable[index+1] , WLS_SCRATCH__TXPOWER_TABLE + 4);
	VcmdW_3DSP_Dword(pAdap, pAdap->TxPwrTable[index+2] , WLS_SCRATCH__TXPOWER_TABLE + 8);	

#endif
	
}
TDSP_STATUS Adap_set_channel( PDSP_ADAPTER_T pAdap,UINT8 chan)
{
//	TDSP_STATUS status;
//	UINT32 		 val;

	
	Adap_set_power_table(pAdap,chan);	

	if(pAdap->wlan_attr.chipID)
	{
		Adap_request_Set_Channel(pAdap, chan);
		
		/*
		//trigger bb_int
		val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_36_38);
		val |= BITS_BBREG_36_38__BB_INT;
		VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_36_38);

		val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_16_19);
		val &= ~CHANNEL_NUMBER_BIT;
		val |= chan;
		VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_16_19);


		

		//triger gen_int1
		val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_36_38);
		val |= BITS_BBREG_36_38__GEN_INT_1;
		VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_36_38);


		//clear bb_int
		val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_36_38);
		val &= ~BITS_BBREG_36_38__BB_INT;
		VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_36_38);
		*/
		
	}
	else
	{
		Adap_set_state_control(pAdap, (UINT8)DEV_IDLE, 0, 0);
		Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);
		
		pAdap->m2bMailbox1.channel_num = chan;
		pAdap->m2bMailbox1.bb_int = TRUE;
		Vcmd_scratch_2_DSP_Amemoffset(pAdap,&pAdap->m2bMailbox1,sizeof(m2bMailbox1_t),0);
		Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);
	}
	
		
	return STATUS_SUCCESS;
}






TDSP_STATUS Adap_set_antenna( PDSP_ADAPTER_T pAdap,UINT8 antenna)
{
	TDSP_STATUS  state;
	UINT32 	val;
	UINT32    tmp;
	tmp = (antenna == 0) ? 0:ANTENNA_SELECT_BIT;

	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_16_19);
	val &= ~ANTENNA_SELECT_BIT; 
	val |= tmp;
	state = VcmdW_3DSP_Mailbox(pAdap, val, WLS_MAC__BBREG_16_19,(PVOID)__FUNCTION__);

	if(state == STATUS_SUCCESS)
	{
		pAdap->wlan_attr.antenna_num = antenna;
	}
	return state;
}

//Add for loopback power table setting
TDSP_STATUS Adap_set_usepd( PDSP_ADAPTER_T pAdap,UINT8 use_pd)
{
	TDSP_STATUS  state;
	UINT32 	val;
//	UINT32    tmp;

	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_16_19);
	if(use_pd != 0)
	{
		val |= MAILBOX_SET_USEPD;
	}
	else
	{
		val &= ~MAILBOX_SET_USEPD; 	
	}
	state = VcmdW_3DSP_Mailbox(pAdap, val, WLS_MAC__BBREG_16_19,(PVOID)__FUNCTION__);
	return state;
}

TDSP_STATUS Adap_set_join_para( PDSP_ADAPTER_T pAdap,PMNG_DES_T pdes)
{
	UINT32	value;
	PMNG_STRU_T     pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T pt_attr = (PDSP_ATTR_T)&pAdap->wlan_attr;
	UINT8    ucTemp;


	//this is now done in join_req because the chip must be
	//in active mode not idle mode
	//wlan_dsp_hal_set_channel_number(pAdapter, bss_p->channel_num);//pAdapter->g_sme.GUI_scan_res[pAdapter->g_sme.action_index]->channel_num);	
	//note that: call the function with state = active;
	//	Adap_set_state_control(pAdap,(UINT8)DEV_JOIN, 0, 50);//pAdap->wlan_attr.join_timeout);	//Justin: for joinFailTo, 50 is enough

	Adap_InitTKIPCounterMeasure(pAdap);
	value = REG_STATE_CONTROL(DEV_IDLE, (0 & 0x01), 50);
	VcmdW_3DSP_Dword(pAdap, value,WLS_MAC__STATE_CONTROL);
	//it is suggested that alloc tx fifo size again with state = idle
	//if(STATUS_SUCCESS != Adap_alloc_tx_fifo_size(pAdap,pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA))
	//{
       //	return STATUS_FAILURE;
	//}/
	//woody 080630
	Adap_request_init_txfifo(pAdap);


	
	//change wlan attribute of software 
	//set preamble according to CAP of AP
	pAdap->wlan_attr.gdevice_info.preamble_type = 
		(CAP_EXPRESS_SHORT_PREAMBLE(pdes->cap)) ? SHORT_PRE_AMBLE : LONG_PRE_AMBLE;


	//write auto_corr to 0
	//Update to 1 later according RSSI threshold got from AP
	//pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr = 0;
	//woody debug for throughput
	pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr = 0;
	pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated = 1;
	pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze = 1;
	if(STATUS_SUCCESS != Adap_set_auto_corr(pAdap))
	{
		return STATUS_FAILURE;
	}

	//set infrastructure or ibss
	pAdap->wlan_attr.macmode = 
		(CAP_EXPRESS_BSS(pdes->cap)) ? WLAN_MACMODE_ESS_STA : WLAN_MACMODE_IBSS_STA;

	//set phy mode
	pAdap->wlan_attr.gdevice_info.dot11_type = pdes->phy_mode;
	DBG_WLAN__MAIN(LEVEL_TRACE,"start join with %x phy mode \n",pAdap->wlan_attr.gdevice_info.dot11_type);
	if(pdes->phy_mode != IEEE802_11_A)
	{
		if(pdes->suprate[0] != 0)	//Justin:	080414.	check valide suprate. Avoid change default phy_mode set in Mng_IbssScanFail
		pAdap->wlan_attr.gdevice_info.dot11_type =  		//decide phy mode is either 11g or 11b according support rate
			Adap_CheckOFDMSupport(
				&pdes->suprate[1],
				pdes->suprate[0],
				pdes->ExtendedSupportRate,
				pdes->ExtendedSupportRateLen) ? IEEE802_11_G : IEEE802_11_B;						
	}

	DBG_WLAN__MAIN(LEVEL_TRACE,"start join with %x phy mode \n",pAdap->wlan_attr.gdevice_info.dot11_type);


	//rewrite control register(401c) for mac mode change
	//here for setting mac ctl reg
	value = Adap_build_mac_ctrl_reg(pAdap,
								pdes->phy_mode,
								pAdap->wlan_attr.macmode);
	//clear basic rate bits
	value =  value & (~MAC_BASICRATESET_BITS_A);
	value = value | Adap_get_basicrate_map(pAdap,pAdap->wlan_attr.macmode);
	//set mac reg
	if(STATUS_SUCCESS != Adap_set_mac_ctlr_reg(pAdap,value))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE," Unknown dev type ! 11a ? 11b ? 11g ? \n");
		return STATUS_FAILURE;
	}
	
	
	//set support rate and slot and so on according to phy mode
	if(STATUS_SUCCESS != 
		Adap_mac_set_dot11type(
			pAdap,
			pAdap->wlan_attr.gdevice_info.dot11_type,
			pAdap->wlan_attr.macmode,
			CAP_EXPRESS_SHORT_SLOTTIME(pdes->cap)))
	{
		return STATUS_FAILURE;
	}

	//re-cal cap due to preamble type & slot time changed
	Adap_Cal_Capability(&pAdap->wlan_attr);

	#if 1	//justin:	080428.		
		if(pAdap->wlan_attr.gdevice_info.dot11_type ==  IEEE802_11_B)
		{
			if(pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE)//set to 11mbps if default datarate is AUTO in ibss mode.
			{
				DBG_WLAN__MAIN(LEVEL_TRACE,"11b auto rate, set 11mbps\n");
				pt_mng->rateIndex = 3;//		case OP_11_MBPS:			return 3;
			}
			else 													//set to 11mbps if not a 802.11b datarate
			{
				DBG_WLAN__MAIN(LEVEL_TRACE,"not 11b rate, set 11mbps\n");
				ucTemp = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.used_basic_rate);
				pt_mng->rateIndex = (ucTemp>3) ? 3:ucTemp;
			}
		}
		else// g or a mode
		{
			pt_mng->rateIndex = Adap_get_rateindex_from_rate(pAdap,pAdap->wlan_attr.used_basic_rate);
		}
	#endif	
	pt_attr->rate = Adap_get_rate_from_index(pAdap, pt_mng->rateIndex);
	pt_attr->rateshowed = pt_attr->rate;
	pt_attr->last_rateshowed = pt_attr->rate;
	pt_attr->rateshowed_count = 0;
	
	DBG_WLAN__MAIN(LEVEL_TRACE,"pt_attr->rate = %x\n",pt_attr->rate);

	//set bssid
	if(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA)	//bss mode
	{
		Vcmd_hal_set_bssid(pAdap,pdes->bssid);
		
		//not received other beacon after join
		value = VcmdR_3DSP_Dword(pAdap, WLS_MAC__CONTROL);
		value &= ~MAC_RX_ANY_BSSID_BEACON_BIT;
		VcmdW_3DSP_Dword(pAdap,value, WLS_MAC__CONTROL);
	}
	else														//ibss mode
	{
		//Mng_build_ibss_bssid(pAdap,pdes->bssid);
		
		Vcmd_hal_set_bssid(pAdap,pdes->bssid);
		//wifi 080829
		//Adap_hal_set_rx_any_bssid_beacon(pAdap);
		//UINT8 bssid[6]={0xff,0xff,0xff,0xff,0xff,0xff};
		//Vcmd_hal_set_bssid(pAdap,bssid);
	}

	//set beacon interval
	pAdap->wlan_attr.beacon_interval = pdes->beaconinterval;
	Vcmd_set_beacon_interval(pAdap,pdes->beaconinterval,TBTT_OFFSET);

	//set antenna
#ifdef ANTENNA_DIVERSITY	
	pAdap->wlan_attr.antenna_num = pdes->antenna_num;
	Adap_set_antenna(pAdap, pdes->antenna_num);
#endif

	//close it, beacuse key has been set at this point on wep mode
	//Adap_set_any_wep_prevent_tx_stop(pAdap);

	
	
	value = (UINT32)pAdap->wlan_attr.rts_threshold;
	//clear erp state when join
	pAdap->wlan_attr.cts_current_state = RTSCTS_STATE_RTS_WORK;
	pAdap->wlan_attr.cts_en = CTS_SELF_DISABLE;
	Mng_Update_Cts_State(pAdap,pt_mng->usedbssdes.Erpinfo);
	
	
	//delete join state. just use active state is enough
	Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);

	// set flag to start tx
	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);

	Vcmd_reset_power_mng(pAdap);//reset the last setting.....ahaha
	if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)
		pAdap->wlan_attr.need_set_pwr_mng_bit_flag = TRUE;
	else
		pAdap->wlan_attr.need_set_pwr_mng_bit_flag = FALSE;
		
	pAdap->wlan_attr.erp_element = 0;

	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
	{
		Mng_ExchangeMattrAndMng(WRITE_ALL_BEACONINFO_REG, pdes, 0, pAdap);

		Mng_MakeBeaconOrProbersp(pAdap,WLAN_FSTYPE_BEACON,NULL);

		DBG_WLAN__MAIN(LEVEL_TRACE,"adhoc - make beacon frame\n");		
	}
	else
	{	
	}
					
	
	return STATUS_SUCCESS;
}

TDSP_STATUS Adap_set_state_control( PDSP_ADAPTER_T pAdap,UINT8 state, UINT8 scan_type, UINT16 time_value)
{
//	mac_hw_state_t	machw_state;
	mac_hw_state_t	machw_state_cur;
	mac_hw_state_t	machw_state_next;
	UINT32			val;
//	UINT32			loopcnt, delay=0;
	TDSP_STATUS      status;

//	machw_state = (mac_hw_state_t)state;
//	loopcnt = 100000; // set max wait time about one sec
	
	
	status = Vcmd_get_current_state(pAdap,&val);
	if(status != STATUS_SUCCESS)
	{
		return status;
	}
	
	machw_state_cur = (mac_hw_state_t)val;
	machw_state_next =(mac_hw_state_t)state;

	if(machw_state_cur == machw_state_next)
	{
		return STATUS_SUCCESS;
	}

	DBG_WLAN__MAIN(LEVEL_TRACE,"machw_state_cur=%d,machw_state_next=%d\n",machw_state_cur, machw_state_next);
		
	//set idle state
	val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
	
	if(val != 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR,"Dangerous for swtiching HW state , now TX count= %x\n",val);
	}

	val = REG_STATE_CONTROL(DEV_IDLE, (scan_type & 0x01), time_value);
	VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__STATE_CONTROL);

	sc_sleep(100);	
	// set next stae
	val = REG_STATE_CONTROL(state, (scan_type & 0x01), time_value);
	VcmdW_3DSP_Dword(pAdap, val,WLS_MAC__STATE_CONTROL);	
	

	return STATUS_SUCCESS;
}


TDSP_STATUS Adap_request_init_txfifo(PDSP_ADAPTER_T pAdap)
{
	DSP_CONFIG_TXFIFO   info;
	UINT32 	loop = VENDOR_LOOP_MIDDLE;
	UINT32    val;
	UINT16  cp,cfp,bcn;
	PDSP_ATTR_T pattr = (PDSP_ATTR_T)&pAdap->wlan_attr;

	pAdap->wlan_attr.gdevice_info.tx_disable = TRUE;
	//woody 080630
	val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);

	while(loop)
	{
		if((val & 0xff) != 0)
		{
			loop--;
			sc_sleep(1);
			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
		}
		else
		{
			break;
		}
	}

	if(loop == 0)
	{
		DBG_WLAN__INIT(LEVEL_ERR ,"INIT TX FIFO FAIL \n");
	}
	

	//just alloc same size both bss and ibss
	pattr->gdevice_info.fifo_size[MACHW_CONTENTION_FIFO] = TOTAL_TX_FIFO_SIZE -((2 * ALIGN64(ATIM_FIFO_SIZE)) + ALIGN64(BCN_FIFO_SIZE));
	pattr->gdevice_info.fifo_size[MACHW_CONTENTION_FREE_FIFO] = ALIGN64(ATIM_FIFO_SIZE);
	pattr->gdevice_info.fifo_size[MACHW_BEACON_FIFO] = ALIGN64(BCN_FIFO_SIZE);

	cp = pattr->gdevice_info.fifo_size[MACHW_CONTENTION_FIFO] >> 6;
	cfp = pattr->gdevice_info.fifo_size[MACHW_CONTENTION_FREE_FIFO] >> 6;
	bcn = pattr->gdevice_info.fifo_size[MACHW_BEACON_FIFO] >> 6;

	val = ( (cp) | ((cfp)<<OFFSET_TX_FIFO_SIZE__CFP_TXFIFO_SIZE) |
		((bcn)<<OFFSET_TX_FIFO_SIZE__BCN_TXFIFO_SIZE) );//|  (BIT_22) );

      //fill the structure of cmd
	info.type = VCMD_REQ_FLUSH_TX_FIFO;
	info.subtype = 1;
	info.size = val;

		
	if(STATUS_SUCCESS != Vcmd_Send_API(pAdap,(PUINT8)&info, sizeof(DSP_CONFIG_TXFIFO)))
	{
		DBG_WLAN__INIT(LEVEL_ERR ,"Adap_request_flush_tx() return error\n");
	}

	//confirm finish
	loop = VENDOR_LOOP_MIDDLE;
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);

	if(loop == 0)
	{
		DBG_WLAN__INIT(LEVEL_ERR ,"INIT TX FIFO CMD FAIL \n");
	}

	pAdap->wlan_attr.gdevice_info.tx_disable = FALSE;

	return STATUS_SUCCESS;	
}

TDSP_STATUS Adap_GetT8051Fw(PDSP_ADAPTER_T pAdap)
{
    PVOID filp;
    UINT32 offset;
    UINT32 read_len;
    TDSP_STATUS Status = STATUS_FAILURE;
	PSP20_FILE_HEAD_T pSp20_file_head = NULL;
    PUINT8 code_buf = NULL;
    UINT8  bin_name[] = {"/usr/local/3DSP/usb/3dspcode.bin"};
    if(pAdap->fw_8051valid == TRUE && pAdap->fw_buf != NULL)
    {
        DBG_WLAN__INIT(LEVEL_TRACE,"Get 8051 file return with has saved in variable\n");
        return STATUS_SUCCESS;
    }
    
    pAdap->fw_8051valid = FALSE;
    pAdap->fw_length   =0;

    do {

    //step 1:open the file and read the bin code head struct
    	filp = sc_openfile(bin_name);
    	if (sc_open_success(filp))
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"Failed to open bincode file:%s!\n", bin_name );
    		filp = NULL;
    	    break;
    	}

        read_len = BIN_CODE_HEADSZIE;
        pSp20_file_head = sc_memory_alloc(sizeof(SP20_FILE_HEAD_T));
    	if (pSp20_file_head == NULL)
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate code_buf step1\n",__FUNCTION__);
    		break;
    	}	
    	sc_memory_set(pSp20_file_head, 0, sizeof(SP20_FILE_HEAD_T));

        if(read_len != sc_readfile(filp, pSp20_file_head, read_len, 0))
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:failed to read step1!\n",__FUNCTION__);
    		break;
    	}


        //get 8051 firmware
        read_len = pSp20_file_head->combo8051_len;
        offset =   pSp20_file_head->combo8051_offset;

        code_buf = sc_memory_alloc(read_len);
        if(code_buf == NULL)
        {
            DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate for cpAdap->fw_combo_8051\n",__FUNCTION__);
    		break;
        }
        
        sc_memory_set(code_buf,0,read_len);
        if(read_len != sc_readfile(filp,code_buf, read_len, offset))
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:Failed to read fw_combo_8051!\n",__FUNCTION__);
    		break;
    	}
        
        pAdap->fw_buf = sc_memory_alloc(read_len);
      
    	if (pAdap->fw_buf == NULL)
    	{
    	    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate for pAdap->fw_buf\n",__FUNCTION__);
    		break;
    	}	//Open file
    	
        sc_memory_copy(pAdap->fw_buf,code_buf,read_len);

        
        pAdap->fw_length= read_len;
        pAdap->fw_8051valid = TRUE;
        pAdap->fw_type   = FW_TYPE_8051;
        Status = STATUS_SUCCESS;
        sc_memory_free(code_buf);
        code_buf = NULL;
     }while(0);


    if(pSp20_file_head)
    {
        sc_memory_free(pSp20_file_head);
    }
    if(code_buf)       
    {
        sc_memory_free(code_buf);
        code_buf = NULL;
    }
    if(Status != STATUS_SUCCESS)
    {
        if(pAdap->fw_buf)
        {
            sc_memory_free(pAdap->fw_buf);
            pAdap->fw_buf = NULL;
        }
    }
    if(filp)
	{
		sc_closefile(filp);
	}
    return Status;
}

TDSP_STATUS Adap_GetDspFw(PDSP_ADAPTER_T pAdap)
{

    PVOID filp;
    UINT32 offset;
    UINT32 read_len,i;
    UINT32  tmpData;  
    UINT32  nCodeType;  
    UINT32  nCodeIndex;
    UINT16 *psubSystemID;
    PSP20_BIN_CODE_TABLE pCodeTable; 
    PSP20_SUPPORT_CODETABLE pSupportCodeTable; 
    TDSP_STATUS Status = STATUS_FAILURE;
	PSP20_FILE_HEAD_T pSp20_file_head = NULL;
    PUINT8  code_buf = NULL;

    UINT8  bin_name[] = {"/usr/local/3DSP/usb/3dspcode.bin"};

    if(pAdap->fw_dspvalid== TRUE && pAdap->fw_buf!= NULL)
    {
        //driver has get firmware file
        DBG_WLAN__INIT(LEVEL_TRACE,"Get dsp file return with has saved in variable\n");
        return STATUS_SUCCESS;
    }

    pAdap->fw_dspvalid = FALSE;
    if(pAdap->fw_buf!= NULL)
    {
        sc_memory_free(pAdap->fw_buf);
        pAdap->fw_buf= NULL;
        pAdap->fw_length = 0;
    }

 do {

    //step 1:open the file and read the bin code head struct
    	filp = sc_openfile(bin_name);
    	if (sc_open_success(filp))
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"Failed to open bincode file:%s!\n", bin_name );
    		filp = NULL;
    	    break;
    	}

        read_len = BIN_CODE_HEADSZIE;
        pSp20_file_head = sc_memory_alloc(sizeof(SP20_FILE_HEAD_T));
    	if (pSp20_file_head == NULL)
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate code_buf step1\n",__FUNCTION__);
    		break;
    	}	
    	sc_memory_set(pSp20_file_head, 0, sizeof(SP20_FILE_HEAD_T));

        if(read_len != sc_readfile(filp, pSp20_file_head, read_len, 0))
    	{
    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:failed to read step1!\n",__FUNCTION__);
    		break;
    	}   
 
        //get dsp firmware

        DBG_WLAN__INIT(LEVEL_TRACE, "EXTEND DSP CODE MARK: %X\n", pSp20_file_head->reserve1[0]);
        if(pSp20_file_head->reserve1[0]==0x30545845) // "EXT0"
        {
            //step 2: read the ext code part of bin code
            offset = pSp20_file_head->reserve1[1];
            read_len =  2*64*1024+16;

			code_buf = sc_memory_alloc(read_len);
			if (code_buf == NULL)
			{
			    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate code_buf step1\n",__FUNCTION__);
				break;
			}
            sc_memory_set(code_buf, 0, read_len);
	
			if(sc_readfile(filp, code_buf, read_len, offset) <= 0)
			{
				DBG_WLAN__INIT(LEVEL_ERR,"[%s]:Failed to read code_buf step1\n",__FUNCTION__);
				break;
			}
            // Support Code Table
            pSupportCodeTable = (PSP20_SUPPORT_CODETABLE)code_buf;
            if(pSupportCodeTable->m_Mark==0x4C424154)
            {
                DBG_WLAN__INIT(LEVEL_TRACE, "==== Support Card Types = %d ====\n", pSupportCodeTable->Size);
                DBG_WLAN__INIT(LEVEL_TRACE, "   SubSysID     WlOnly     BTOnly    Combo \n");
                for(i=0; i<pSupportCodeTable->Size; i++)
                {
                    switch((pSupportCodeTable->m_dwID[i]&0x0000FFFF))
                    {
                        case 1:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      Y          -         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 2:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      -          Y         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 3:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      Y          Y         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 4:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      -          -         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 5:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      Y          -         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 6:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      -          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 7:
                           DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      Y          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        case 0x0F:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      Y          Y         Y \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                        default:
                            DBG_WLAN__INIT(LEVEL_TRACE, "CardID_%04X      -          -         - \n", (pSupportCodeTable->m_dwID[i]&0xFFFF0000)>>16);
                            break;
                    }
                }           
                DBG_WLAN__INIT(LEVEL_TRACE, "==== Support Card Types End ====\n");
                offset = (pSupportCodeTable->Size)*4+8; // 
                nCodeType = *(PUINT32)&(code_buf[offset+4]); //     
                pCodeTable = (PSP20_BIN_CODE_TABLE)&code_buf[offset+8];
                psubSystemID = (PUINT16)&pAdap->u_eeprom[EEPROM_OFFSET_SUBSYSTEMID];

                if(pAdap->dsp_fw_mode ==  INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
                {
                    tmpData = ((*psubSystemID)<<16)|4;
                }
                else
                {
                    tmpData = ((*psubSystemID)<<16)|1;
                }               
                nCodeIndex = 0;
                for(i=0; i<nCodeType; i++)
                {
                    if(pCodeTable[i].m_dwID==tmpData)
                    {
                        nCodeIndex = i;
                    }
                }
				 //if we do not find the specified card's code in 3dspcode.bin, so download code with type  0x0100.
				 //Joe  2009 - 09 - 15.
				 if(nCodeIndex == 0)
				 {
					 DBG_WLAN__INIT(LEVEL_TRACE, "****** not support CardID, so Download default 0x0100 \n");
					 if(pAdap->dsp_fw_mode ==  INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
					 {
						 tmpData = ((0x0100)<<16)|4;
					 }
					 else
					 {
						 tmpData = ((0x0100)<<16)|1;
					 }               
					 nCodeIndex = 0;
					 for(i=0; i<nCodeType; i++)
					 {
						 if(pCodeTable[i].m_dwID==tmpData)
						 {
							 nCodeIndex = i;
						 }
					 }
				 }
				 //End
                if(pCodeTable[nCodeIndex].m_dwID==tmpData)
                {
                    DBG_WLAN__INIT(LEVEL_TRACE, "  Download CardID %04X CodeID:%08X \n", (pCodeTable[nCodeIndex].m_dwID>>16), pCodeTable[nCodeIndex].m_dwID);
                }
                else
                {
                    DBG_WLAN__INIT(LEVEL_TRACE, "****** not support CardID, so Download default CardID %04X CodeID:%08X \n", (pCodeTable[nCodeIndex].m_dwID>>16), pCodeTable[nCodeIndex].m_dwID);
                }
                
                //save base port of ext code
                pAdap->fw_ext_baseport = (UINT16)pCodeTable[nCodeIndex].m_dwPortBase;

                read_len = pCodeTable[nCodeIndex].m_dwLen;
                offset =   pCodeTable[nCodeIndex].m_dwOffset;

                DBG_WLAN__INIT(LEVEL_TRACE, "******Download Code : Offset=%X  Len=%X PortBase=%d \n", offset, read_len, pCodeTable[nCodeIndex].m_dwPortBase);
				
               
                if(code_buf)
                {
					sc_memory_free(code_buf);
                    code_buf = NULL;
                }

                code_buf = sc_memory_alloc(read_len);
                if(code_buf == NULL)
                {
                    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate for cpAdap->fw_combo_8051\n",__FUNCTION__);
            		break;
                }
                
                sc_memory_set(code_buf,0,read_len);
                if(read_len != sc_readfile(filp,code_buf, read_len, offset))
            	{
            		DBG_WLAN__INIT(LEVEL_ERR,"[%s]:Failed to read fw_combo_8051!\n",__FUNCTION__);
            		break;
            	}
                
                pAdap->fw_buf = sc_memory_alloc(read_len);
              
            	if (pAdap->fw_buf == NULL)
            	{
            	    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate for pAdap->fw_buf\n",__FUNCTION__);
            		break;
            	}	//Open file
    	
                sc_memory_copy(pAdap->fw_buf,code_buf,read_len);
                pAdap->fw_length = read_len;
                pAdap->fw_type = FW_TYPE_DSPEXT;
                sc_memory_free(code_buf);
                code_buf = NULL;
                                      
            }
        }
        
        else
		// 090702 glen added for extend card type support end
        {
            //get combo dsp code           {

            if(pAdap->dsp_fw_mode == INT_SUB_TYPE_RESET_WITH_COMBO_MODE)
            {
                read_len  = pSp20_file_head->combo_len;
                offset =   pSp20_file_head->combo_offset;
                pAdap->fw_type = FW_TYPE_DSPCOMBO;
            }
            
            else
            {
                read_len = pSp20_file_head->wlanonly_len;
                offset =   pSp20_file_head->wlanonly_offset;
                pAdap->fw_type = FW_TYPE_DSPWLAN;
            }

            
			code_buf = sc_memory_alloc(read_len);
			if (code_buf == NULL)
			{
			    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate code_buf step1\n",__FUNCTION__);
				break;
			}
            sc_memory_set(code_buf, 0, read_len);
	
			if(sc_readfile(filp, code_buf, read_len, offset) <= 0)
			{
				DBG_WLAN__INIT(LEVEL_ERR,"[%s]:Failed to read code_buf step1\n",__FUNCTION__);
				break;
			}
        
         	pAdap->fw_buf= sc_memory_alloc(read_len);
			if (pAdap->fw_buf == NULL)
			{
			    DBG_WLAN__INIT(LEVEL_ERR,"[%s]:can not allocate fw_buf buffer,step 2\n",__FUNCTION__);
	            break;
	
			}
            sc_memory_copy(pAdap->fw_buf, code_buf, read_len);
            pAdap->fw_length = read_len;
            sc_memory_free(code_buf);
            code_buf = NULL;
                                    
           
       }       
   
        pAdap->fw_dspvalid = TRUE;
        Status = STATUS_SUCCESS;

   }while(0);

          
	if(pSp20_file_head)
		sc_memory_free(pSp20_file_head);

    if(code_buf)
    {
         sc_memory_free(code_buf);
          code_buf = NULL;
    }
    if(Status != STATUS_SUCCESS)
    {
        if(pAdap->fw_buf)
        {
           sc_memory_free(pAdap->fw_buf);
        }   
    }
    if(filp)
	{
		sc_closefile(filp);
	}
    return Status;  
}



TDSP_STATUS Adap_Get8051FwCodesAndDownload(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS Status;
	UINT32 FileLength, div, mod, i,first_div;
    PSP20_BIN_CODE pBinFileBuffer;
    PVOID MappedBuffer;
    int leftBytes = 0;;
#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE	
    PSP20_CODE pCodeBuffer = (PSP20_CODE)Usb_8051CodeWlanOnly;
#endif
    DBG_WLAN__INIT(LEVEL_TRACE,"Enter [%s]\n",__FUNCTION__);

    // stop 8051 firmware
    if(STATUS_SUCCESS != Fw_Stop_8051fw(pAdap))
	{
        DBG_WLAN__INIT(LEVEL_TRACE,"[%s]: stop firmware fail \n",__FUNCTION__);
        return STATUS_FAILURE;
    }
    
    Status = STATUS_SUCCESS;


{
#ifndef DOWNLOAD_CODE_WITH_H_FILE_MODE
    DBG_WLAN__INIT(LEVEL_TRACE,"load 8051 3dspcode.bin \n");

    do{
            Status = Adap_GetT8051Fw(pAdap);
            if(Status!= STATUS_SUCCESS)
            {
                break;
            }
            FileLength = pAdap->fw_length;
            pBinFileBuffer = (PSP20_BIN_CODE)pAdap->fw_buf;
           
            // Glen090120Glen Added for fix download 8051 code out of range bug 
            // Because the FileLength is include 4bytes code length, 4bytes port num and code data length
            // The real 8051 code size is FileLength - 8, or maybe lead to download 8051 code out of range.
            FileLength -= 8;    
            
            if (FileLength>0x10000)
                FileLength = 0x10000;
                     // Glen090120Glen Added for fix download 8051 code out of range bug end
            //begin download
            div = FileLength / DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
            mod = FileLength % DSP_8051_FIRMWARE_FRAGMENT_LENGTH;

            div = (mod == 0) ? div : div + 1;

            first_div = (div > DSP_8051FW_FIRST_SEGMENT_NUM ) ? DSP_8051FW_FIRST_SEGMENT_NUM: div;
            DBG_WLAN__INIT(LEVEL_TRACE,"Down8051 section : Real 8051 File Length =  %d, div = %d, mod = %d, div = %d, first = %d, \n", FileLength + 8 ,
                div, mod, div, first_div );
            //send frag of firmware
            for (i = 0; i < first_div ; i++)
            {
                Status = Fw_download_8051fw_fragment(
                    pAdap, 
                    (PUINT8)(pBinFileBuffer->Code + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH), 
                    DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
                    (UINT16)(DOWNLOAD_8051_FW_FIELD_OFFSET + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH)); 

                //ASSERT(NDIS_STATUS_SUCCESS == Status);
                if(Status != STATUS_SUCCESS)
                {
                    DBG_WLAN__INIT(LEVEL_TRACE,"Download 8051 fail 3\n");
                    break;
                }   
            }

            if(Status == STATUS_SUCCESS)
            {
                //write code into 0xc000 - 0xffff
                if(div > DSP_8051FW_FIRST_SEGMENT_NUM)
                {
                    //DBGSTR(("Download length of FW is too big\n"));

                    div = (FileLength - DSP_8051FW_SECOND_SEGMENT_BEGIN)/DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
                    mod = (FileLength - DSP_8051FW_SECOND_SEGMENT_BEGIN)%DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
                    
                    div = (mod == 0) ? div : div + 1;
                    DBG_WLAN__INIT(LEVEL_TRACE,"Down8051 section : Real 8051 File Length =  %d, div = %d, mod = %d, div = %d, first = %d, \n", FileLength + 8 ,
                        div, mod, div, first_div );
                    leftBytes = FileLength - DSP_8051FW_SECOND_SEGMENT_BEGIN; 
                    for (i = 0; i < div ; i++)
                    {                       
                        Status = Fw_download_8051fw_fragment(
                            pAdap, 
                            (PUINT8)(pBinFileBuffer->Code + DSP_8051FW_SECOND_SEGMENT_BEGIN + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH), 
                            DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
                            (UINT16)(DSP_8051FW_SECOND_SEGMENT_BEGIN + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH)); 

                            //ASSERT(NDIS_STATUS_SUCCESS == Status);
                            if(Status != STATUS_SUCCESS)
                            {
                                DBG_WLAN__INIT(LEVEL_TRACE,"Download 8051 fail 4\n");
                                break;      
                            }
                            leftBytes = leftBytes - DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
                            DBG_WLAN__INIT(LEVEL_TRACE,"Download 8051 leftbytes = %d \n", leftBytes);
                            /*if(leftBytes < 0)
                            {
                                break;
                            }*/
                    }   
                }
            }
        }while(FALSE);
    
#else   //#ifdef DOWNLOAD_CODE_WITH_H_FILE_MODE     //#ifdef DOWNLOAD_8051_WITH_BIN_FILE_MODE
	{
		//here get fw file from a .h file

		//DBGSTR(pCodeBuffer->Size != 0);
		FileLength =(UINT32)pCodeBuffer->Size;
		div = FileLength / DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
		mod = FileLength % DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
		MappedBuffer = (PVOID)pCodeBuffer->Code;

		div = (mod == 0) ? div : div + 1;

		first_div = (div > DSP_8051FW_FIRST_SEGMENT_NUM ) ? DSP_8051FW_FIRST_SEGMENT_NUM: div;


        DBG_WLAN__INIT(LEVEL_TRACE,"Fw_download_8051fw_fragment 1\n");
		//send frag of firmware
		for (i = 0; i < first_div ; i++)
		{
			Status = Fw_download_8051fw_fragment(
				pAdap, 
				(PUINT8)((PUINT8)MappedBuffer + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH), 
				DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
				(UINT16)(DOWNLOAD_8051_FW_FIELD_OFFSET + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH)); 

			//ASSERT(STATUS_SUCCESS == Status);
			if(Status != STATUS_SUCCESS)
				break;
		}

        DBG_WLAN__INIT(LEVEL_TRACE,"Fw_download_8051fw_fragment 2 \n");
	
		//write code into 0xc000 - 0xffff
		if(Status == STATUS_SUCCESS
			&& div > DSP_8051FW_FIRST_SEGMENT_NUM)
		{
			if(FileLength > DSP_8051FW_SECOND_SEGMENT_BEGIN)
			{
				 DBG_WLAN__INIT(LEVEL_TRACE,"[%s]: Length of 8051FW is too long\n",__FUNCTION__);
			}

			div = (FileLength - DSP_8051FW_SECOND_SEGMENT_BEGIN)/DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
			mod = (FileLength - DSP_8051FW_SECOND_SEGMENT_BEGIN)%DSP_8051_FIRMWARE_FRAGMENT_LENGTH;
			
			div = (mod == 0) ? div : div + 1;
				
			for (i = 0; i < div ; i++)
			{
				Status = Fw_download_8051fw_fragment(
					pAdap, 
					(PUINT8)((PUINT8)MappedBuffer + DSP_8051FW_SECOND_SEGMENT_BEGIN + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH), 
					DSP_8051_FIRMWARE_FRAGMENT_LENGTH,
					(UINT16)(DSP_8051FW_SECOND_SEGMENT_BEGIN + i * DSP_8051_FIRMWARE_FRAGMENT_LENGTH)); 

					//ASSERT(STATUS_SUCCESS == Status);
					if(Status != STATUS_SUCCESS)
						break;		
			}	
				
		}
		
		sc_sleep(5);
	}

#endif
}

    // start 8051 firmware
    if(Status == STATUS_SUCCESS)
	{
		if(STATUS_SUCCESS != Fw_Start_8051fw(pAdap))
		{
			DBG_WLAN__INIT(LEVEL_TRACE,"[%s]: start firmware fail \n",__FUNCTION__);
			return STATUS_FAILURE;
		}
    }

//release 8051 code buffer
    if(pAdap->fw_buf != NULL)
    {
        sc_memory_free(pAdap->fw_buf);
        pAdap->fw_buf = NULL;
        pAdap->fw_8051valid = FALSE;
        pAdap->fw_length = 0;
    }
    
    DBG_WLAN__INIT(LEVEL_TRACE,"Leave [%s]\n",__FUNCTION__);
	return (Status);
}


TDSP_STATUS
Adap_add_key_into_mib(
	PDSP_ADAPTER_T pAdap,
	UINT8 key_id,
	UINT8 key_len,
	UINT8 *key,
	UINT8 *mac_addr,
	BOOLEAN multi,
	UINT32     cipher
)	
{
	wpakey_info_t  *keyspace;
	UINT32 		   index;
	UINT8  gadr[]= {0xff,0xff,0xff,0xff,0xff,0xff};
	
	if(key_id > 4)
    		DBG_WLAN__SME(LEVEL_ERR ,"[%s] !! Key id > 4 \n",__FUNCTION__);

	if(key_len == 0)
    		DBG_WLAN__SME(LEVEL_ERR ,"[%s] !! Key len =0 \n",__FUNCTION__);

	//for group key, put it into first 4 default position
	if(multi && (mac_addr == NULL))
	{
		index = key_id;
	}
	//for wpa rx group key, put it into last position
	else if(multi && (mac_addr != NULL))
	{
		//index = REAL_KEY_ARRAY_SIZE - 1;
		index = key_id + 5;
	}
	//tx pairkey,put it into first non group position
	else if(!multi && (mac_addr != NULL))
	{
		index = MAX_WEP_KEY_NUM;
	}
	//error case
	else
	{
    	DBG_WLAN__SME(LEVEL_ERR ,"[%s] !!set key fail \n",__FUNCTION__);	
		return STATUS_FAILURE;
	}

	keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[index];
	keyspace->multicast = multi;
	keyspace->cyper = cipher;
	sc_memory_copy(keyspace->key,key,key_len);
	keyspace->key_len_in_bytes = key_len;
	keyspace->WEP_on = TRUE;
	keyspace->in_use = FALSE;
	keyspace->keyid = key_id;
	keyspace->index = index;

	if(mac_addr == NULL)
	{
		mac_addr = gadr;
	}
	WLAN_COPY_ADDRESS(keyspace->mac_address,mac_addr);
	/*
	if(multi)
	{
		WLAN_COPY_ADDRESS(keyspace->mac_address,gadr);
	}
	else
	{
		WLAN_COPY_ADDRESS(keyspace->mac_address,mac_addr);
	}
	*/
    	return STATUS_SUCCESS;
} /* end of sme_add_wep_key() */


/*
	The function has two function:
	   1)	   put key into hw
	   	wep_on = true & in_use = false
	   2)    remove key from hw
	      wep_on = false & in_use = true
	      
/*/

TDSP_STATUS
Adap_push_key_into_hw(
	PDSP_ADAPTER_T pAdap
)
{
	wpakey_info_t  *keyspace;
	INT32 i;
	TDSP_STATUS status;

	for(i = 0; i< REAL_KEY_ARRAY_SIZE;i++)
	{
		keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[i];
		if(	keyspace->WEP_on
		&&	!keyspace->in_use)
		{
			status = Adap_mac_set_key(pAdap,(PVOID)keyspace);
			if(status != STATUS_SUCCESS)
			{
				DBG_WLAN__SME(LEVEL_ERR ,"Adap_mac_set_key return failure.\n");
				return status;
			}
			keyspace->in_use = TRUE;
		}
		//woody 0903 for set key
		else if(	!keyspace->WEP_on
			&&	keyspace->in_use)
		{
			status = Adap_del_key_from_hw(pAdap,i);
			if(status != STATUS_SUCCESS)
			{
				DBG_WLAN__SME(LEVEL_ERR ,"Adap_del_key_from_hw return failure.\n");
				return status;
			}
			keyspace->WEP_on = FALSE;
			 keyspace->in_use = FALSE;
		}
			
	}
	return STATUS_SUCCESS;	
}

//Jakio20070629: add for oid_removekey
TDSP_STATUS Adap_del_key_from_hw(
							PDSP_ADAPTER_T	pAdap,
							UINT32				index
							)

{
#ifdef SET_KEY_WITH_8051
	wpakey_info_t  	*keyspace;

	UINT32	 		loop;
	KEY_MLME_REG_T  key_mlme;
	TDSP_STATUS       status;

	keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[index];
	if (!keyspace->in_use)
	{
//		DBG_WLAN__MAIN(LEVEL_TRACE,"%s : in_use = %d, WEP_on = %d, index = %d\n", __FUNCTION__, keyspace->in_use, keyspace->WEP_on, index);
		return STATUS_SUCCESS;
	}
	
	Tx_Stop(pAdap);
	Adap_Set_Driver_State(pAdap,DSP_STOP_TX);

	if(TRUE)
	{
		//Wait untill tx count = 0;
		ULONG val;
		ULONG readCount=2;
		val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
		val &= 0xff;
		while(val != 0)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"OID del_key key : COUNT = %x, times = %d!\n",val, readCount );
			//wait 10ms
			if(readCount == 0)
			{
				break;
			}
			readCount--;
			sc_sleep(200);					
			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
			val &= 0xff;
		}
		
		if(readCount == 0)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"OID del_key,maybe tx hang happen,COUNT = %x, times = %d!\n",val, readCount);
		}
		else
		{
			DBG_WLAN__MAIN(LEVEL_INFO,"OID del_key,continue COUNT = %x, times = %d!\n",val, readCount);
		}
	}

	keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[index];
	keyspace->key_len_in_bytes = 10;
	keyspace->in_use = FALSE;		// indicate to rewrite key
	keyspace->WEP_on = TRUE;

	for(loop=0; loop <MAX_WEP_KEY_BUFFER_LEN; loop++)
		keyspace->key[loop] =0x00;

	keyspace->mac_address[0] = 0x00;
	keyspace->mac_address[1] = 0x00;
	keyspace->mac_address[2] = 0x00;
	keyspace->mac_address[3] = 0x00;
	keyspace->mac_address[4] = 0x00;
	keyspace->mac_address[5] = 0x00;

	keyspace->in_use = FALSE;		
	keyspace->WEP_on = FALSE;


	sc_memory_set(&key_mlme, 0, sizeof(KEY_MLME_REG_T));

	//set control reg
	key_mlme.key_cntl.entryNotDone = 0;   //first we don't set this bit
	key_mlme.key_cntl.keyindex = index;
	key_mlme.key_cntl.keyvalid  = 0;

	loop = VENDOR_LOOP_MIDDLE;
	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);
	//loop to 0, fail
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR ,"Vendor busy when delete key\n");
		return STATUS_FAILURE;		
	}


//atus = VcmdW_3DSP_Regs(pAdap,(PUCHAR)&key_mlme,sizeof(KEY_MLME_REG_T),WLS_MAC__KEY_WORD_0);
	//write content
	//DBG_WLAN__SME(LEVEL_TRACE,"Mac set key before\n");
	status = Basic_WriteBurstRegs(pAdap,(PUINT8)&key_mlme,sizeof(KEY_MLME_REG_T),0x1e0);
	if(status != STATUS_SUCCESS)
	{
		//Jakio20070426: if there something more to handle this exception??
    		DBG_WLAN__MAIN(LEVEL_ERR,"Mac delete key fai\n");
		return status;
	}
	status = Basic_WriteRegByte(pAdap,0x34,0x108);
	if(status != STATUS_SUCCESS)
	{
    	DBG_WLAN__MAIN(LEVEL_ERR,"!! Mac delete key fail \n");
		return status;
	}

	loop = VENDOR_LOOP_MIDDLE*10;
	while(VCMD_VenderCmd_BUSY(pAdap))
	{
		sc_sleep(1);
		loop--;
	}

	//loop to 0, fail
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR,"delete key fail\n");
		return STATUS_FAILURE;		
	}
 
	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
	Tx_Restart(pAdap);

	//DBG_WLAN__SME(LEVEL_TRACE,"!! delete  key success\n");
	
	return STATUS_SUCCESS;	 	

#else	
	wpakey_info_t  *keyspace;
	UINT32	loop;
	UINT32	reg_value;

	//justin: 0913. don't del key from hw, only push an invalide key into hw
	Adap_Set_Driver_State(pAdap,DSP_STOP_TX);
	Tx_Stop(pAdap);

	loop = 1000;
	while(loop)
	{
		reg_value = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);
		reg_value &=0x000000ff;
		if(reg_value == 0)
		{
			DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] DEL KEY while tx count = 0 \n",__FUNCTION__);
			break;
		}
		loop--;
	}
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] DEL KEY but can't find tx count = 0 \n",__FUNCTION__);
	}

	//set dsp to idle
	Adap_set_state_control(pAdap, (UINT8)DEV_IDLE, 0, 0);

	sc_sleep(1000);
	keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[index];
	keyspace->key_len_in_bytes = 10;
	keyspace->in_use = FALSE;		// indicate to rewrite key
	keyspace->WEP_on = TRUE;

	for(loop=0; loop <MAX_WEP_KEY_BUFFER_LEN; loop++)
		keyspace->key[loop] =0x00;

	keyspace->mac_address[0] = 0x00;
	keyspace->mac_address[1] = 0x00;
	keyspace->mac_address[2] = 0x00;
	keyspace->mac_address[3] = 0x00;
	keyspace->mac_address[4] = 0x00;
	keyspace->mac_address[5] = 0x00;

	//reg_value = 0xffffffff;	
	//sc_memory_copy(keyspace->key,(PUINT8)&reg_value,sizeof(reg_value));//keyspace.key_len_in_bytes);
	//sc_memory_set(keyspace->key,keyspace->key_len_in_bytes);

	//woody
	//Adap_push_key_into_hw(pAdap);

	keyspace->in_use = FALSE;		
	keyspace->WEP_on = FALSE;

	 //return STATUS_SUCCESS;


	//firstly, write NULL to key mac addr reg
	reg_value = 0x00000000;
	VcmdW_3DSP_Dword(pAdap, reg_value, WLS_MAC__KEY_MAC_ADDR);

	//Adap_Set_Driver_State(pAdap,DSP_STOP_TX);

	//secondly, write key ctrl reg and addr reg
	reg_value = 0x0000 \
		  	  | ((index << OFFSET_KEY_CNTL_ADDR__KEY_ID) & (BITS_KEY_CNTL_ADDR__KEY_ID)) \
			  | ((index << OFFSET_KEY_CNTL_ADDR__KEY_INDEX) & (BITS_KEY_CNTL_ADDR__KEY_INDEX)) \
			  | BITS_KEY_CNTL_ADDR__ENTRY_NOT_DONE;
	//clear key valid bit
	reg_value &= ~OFFSET_KEY_CNTL_ADDR__KEY_BALID;
	VcmdW_3DSP_Dword(pAdap, reg_value, WLS_MAC__KEY_CNTL_ADDR);

	//wait for hw complete the operation
	sc_sleep(50);

	loop = VENDOR_LOOP_MIDDLE;
	while (loop){
		reg_value= VcmdR_3DSP_Dword(pAdap, WLS_MAC__KEY_CNTL_ADDR);
		if(reg_value & BITS_KEY_CNTL_ADDR__ENTRY_NOT_DONE)
		{
			loop--;
		}
		else
		{
			break;
		}
	}

	Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);

	sc_sleep(1000);

	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
	Tx_Restart(pAdap);

	if(loop == 0)
	{
    		DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] !! delete  key fail\n",__FUNCTION__);
		return STATUS_FAILURE;
	}
	
	DBG_WLAN__MAIN(LEVEL_ERR ,"[%s] !! delete  key success\n",__FUNCTION__);
	return STATUS_SUCCESS;	
	
#endif
}
/*
    the routine set all kep off, in other words, no privacy on
*/
TDSP_STATUS Adap_key_mib_to_false(PDSP_ADAPTER_T pAdap)
{
    wpakey_info_t  *keyspace;
    int i;

    for(i = 0; i< REAL_KEY_ARRAY_SIZE;i++)
    {
        keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[i];
        keyspace->WEP_on = FALSE;
    }   
    return STATUS_SUCCESS; 
}

/*Justin: 20070910
	the routine clear the all of in use of key_map to indicate current key no push into hw,
	driver should write key into hw next step

*/
TDSP_STATUS
Adap_key_hw_to_false(
	PDSP_ADAPTER_T pAdap
)
{
	wpakey_info_t  *keyspace;
	int i;

	for(i = 0; i< REAL_KEY_ARRAY_SIZE;i++)
	{
		keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[i];
		//keyspace->WEP_on = FALSE;
		keyspace->in_use = FALSE;
	}	
	return STATUS_SUCCESS;	
}


TDSP_STATUS
Adap_mac_set_key(
	PDSP_ADAPTER_T pAdap,
	PVOID 			key_map_node
)
{
#ifdef SET_KEY_WITH_8051
	wpakey_info_t  *keyspace;
	KEY_MLME_REG_T  key_mlme;

	UINT32 			 loop = VENDOR_LOOP_MIDDLE;
	TDSP_STATUS	status;

	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	
	ASSERT(key_map_node);

	Tx_Stop(pAdap);
	Adap_Set_Driver_State(pAdap,DSP_STOP_TX);

	while(VCMD_VenderCmd_BUSY(pAdap) && --loop);

	//loop to 0, fail
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR,"Vendor busy when set key\n");
		return STATUS_FAILURE;		
	}


	keyspace = (wpakey_info_t *)key_map_node;

	sc_memory_set(&key_mlme, 0, sizeof(KEY_MLME_REG_T));
	sc_memory_copy(&key_mlme.key0_31,keyspace->key,keyspace->key_len_in_bytes);
 	WLAN_COPY_ADDRESS(&key_mlme.mac_addr,pt_mng->usedbssdes.bssid);	

	
	//set key size
	switch(keyspace->key_len_in_bytes){
		case KEY_BIT40_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP40;
			break;
		case KEY_BIT104_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP104;
			break;
		case KEY_BIT128_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP128;
			break;
		default:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__NOT_SUPPORT;
			//ASSERT(0);
	}
	//set control reg
	key_mlme.key_cntl.ciphertype = keyspace->cyper;
	key_mlme.key_cntl.entryNotDone = 0;   //first we don't set this bit
	key_mlme.key_cntl.keyid = (UINT32)keyspace->keyid;
	key_mlme.key_cntl.keyindex = keyspace->index;
	key_mlme.key_cntl.keyvalid  = 1;
	key_mlme.key_cntl.xcastKey = (UINT32)keyspace->multicast;
 
	 loop = VENDOR_LOOP_MIDDLE*10;
	while(VCMD_VenderCmd_BUSY(pAdap))
	{
		sc_sleep(1);
		loop--;
	}

	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR,"!! Mac set key fail\n");
		return STATUS_FAILURE;		
	}

	//atus = VcmdW_3DSP_Regs(pAdap,(PUCHAR)&key_mlme,sizeof(KEY_MLME_REG_T),WLS_MAC__KEY_WORD_0);
	//write content
	//DBG_WLAN__MAIN(LEVEL_TRACE,"Mac set key before\n");
	status = Basic_WriteBurstRegs(pAdap,(PUINT8)&key_mlme,sizeof(KEY_MLME_REG_T),0x1e0);
	if(status != STATUS_SUCCESS)
	{
		//Jakio20070426: if there something more to handle this exception??
    		DBG_WLAN__MAIN(LEVEL_ERR,"Mac set key fai\n");
		return status;
	}
	status = Basic_WriteRegByte(pAdap,0x34,0x108);
	if(status != STATUS_SUCCESS)
	{
    		DBG_WLAN__MAIN(LEVEL_ERR,"!! Mac set key fail \n");
		return status;
	}

	loop = VENDOR_LOOP_MIDDLE*10;
	while(VCMD_VenderCmd_BUSY(pAdap))
	{
		sc_sleep(1);
		loop--;
	}

	//loop to 0, fail
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_ERR,"Set key fail\n");
		return STATUS_FAILURE;		
	}
	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
	Tx_Restart(pAdap);
	
	//DBG_WLAN__MAIN(LEVEL_TRACE,"Mac set key success \n");
	return STATUS_SUCCESS;
	
#else

	wpakey_info_t  *keyspace;
	KEY_MLME_REG_T  key_mlme;
	UINT32 			 tmp;
	UINT32 			 loop = VENDOR_LOOP_MIDDLE;
	TDSP_STATUS status;
	UINT32 reg_value;
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	
	ASSERT(key_map_node);

	Adap_Set_Driver_State(pAdap,DSP_STOP_TX);
	Tx_Stop(pAdap);

	loop = 1000;
	while(loop)
	{
		reg_value = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);
		reg_value &=0x000000ff;
		if(reg_value == 0)
		{
			DBG_WLAN__MAIN(LEVEL_ERR,"set KEY while tx count = 0 \n");
			break;
		}
		loop--;
	}
	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"DEL KEY but can't find tx count = 0 \n");
	}

	//set dsp to idle
	Adap_set_state_control(pAdap, (UINT8)DEV_IDLE, 0, 0);

	sc_sleep(1000);

	
	keyspace = (wpakey_info_t *)key_map_node;

	sc_memory_set(&key_mlme,sizeof(KEY_MLME_REG_T));
	//cp key
	sc_memory_copy(&key_mlme.key0_31,keyspace->key,keyspace->key_len_in_bytes);
	//cp mac addr
	//WLAN_COPY_ADDRESS(&key_mlme.mac_addr,keyspace->mac_address);
	//Justin: 080612.	The group mac_address got through OID from up-level is a all 'F's value when device works in WPA-TKIP in win vista.
	//				In this case, device will not receive encrypted group frame.	So we need the connected AP's mac to fill and put to HW.
	//				PS:	In win xp, it is ok to fill with mac_address from up-level.
	WLAN_COPY_ADDRESS(&key_mlme.mac_addr,pt_mng->usedbssdes.bssid);	

	
	//set key size
	switch(keyspace->key_len_in_bytes){
		case KEY_BIT40_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP40;
			break;
		case KEY_BIT104_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP104;
			break;
		case KEY_BIT128_LEN:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__WEP128;
			break;
		default:
			key_mlme.key_size_cntl = WEP_KEY_SIZE__NOT_SUPPORT;
			//ASSERT(0);
	}
	//set control reg
	key_mlme.key_cntl.ciphertype = keyspace->cyper;
	key_mlme.key_cntl.entryNotDone = 0;   //first we don't set this bit
	key_mlme.key_cntl.keyid = (UINT32)keyspace->keyid;
	key_mlme.key_cntl.keyindex = keyspace->index;
	key_mlme.key_cntl.keyvalid  = 1;
	key_mlme.key_cntl.xcastKey = (UINT32)keyspace->multicast;

	//Jakio20070426: according to the spec, we should never write 0x4090~0x40a4 if 
	//bit31 of 0x40a4 equals 1
	while (loop){
		tmp = VcmdR_3DSP_Dword(pAdap, WLS_MAC__KEY_CNTL_ADDR);
		if(tmp & BITS_KEY_CNTL_ADDR__ENTRY_NOT_DONE)
		{
			loop--;
		}
		else
		{
			break;
		}
	}
	if(loop == 0)
	{
		ASSERT(0);
		return STATUS_FAILURE;
	}


	status = VcmdW_3DSP_Regs(pAdap,(PUINT8)&key_mlme,sizeof(KEY_MLME_REG_T),WLS_MAC__KEY_WORD_0);
	if(status != STATUS_SUCCESS)
	{
		//Jakio20070426: if there something more to handle this exception??
    		DBG_WLAN__MAIN(LEVEL_TRACE,"!! Mac set key fail \n");
		return status;
	}
	//set write done flag
	key_mlme.key_cntl.entryNotDone = 1;
	sc_memory_copy(&tmp,&key_mlme.key_cntl,sizeof(UINT32));
	VcmdW_3DSP_Dword(pAdap,tmp,WLS_MAC__KEY_CNTL_ADDR);

	sc_sleep(50);

	loop = VENDOR_LOOP_MIDDLE;
	while (loop){
		tmp = VcmdR_3DSP_Dword(pAdap, WLS_MAC__KEY_CNTL_ADDR);
		if(tmp & BITS_KEY_CNTL_ADDR__ENTRY_NOT_DONE)
		{
			loop--;
		}
		else
		{
			break;
		}
	}


	Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);

	sc_sleep(1000);

	Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
	Tx_Restart(pAdap);

	if(loop == 0)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Mac set key fail \n");
		return STATUS_FAILURE;
	}

	DBG_WLAN__MAIN(LEVEL_TRACE,"Mac set key success \n");
	return STATUS_SUCCESS;

#endif
}




/*
	start a bss with the following items
	1:set channel
	2:set bssid
	3:beacon interval
	4:dtim & cfp
	5:mac control reg
	     bss type
	     basic rate
	     role and so on
	6:realloc tx fifo
	7:set to active

	other should be considered
	1: rts_threshold
	2: fragment
	3: cts flag
	4: antenna
	     
*/
void 
Adap_start_bss(
	PDSP_ADAPTER_T pAdap)
{
	/* uint8 channel_num, uint16 probe_delay, uint16 basic_rate,
	    uint8 *bssid, uint16 beacon_interval, boolean is_ibss */
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
//	PDSP_ATTR_T			pt_mattr = &pAdap->wlan_attr;
	PMNG_DES_T			pdes;

	pdes = &pt_mng->usedbssdes;

	//copy wlan attribute to pattr structure due to auth ok
	Mng_ExchangeMattrAndMng(WRITE_ALL_BEACONINFO_REG, &pt_mng->usedbssdes, 0, pAdap);
	Adap_SetRateIndex( pAdap);
#ifdef ROAMING_SUPPORT_DEL
	DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^ROAMING_SUPPORT, Adap_start_bss,  --> NO RECONNECT\n");
	pAdap->reconnect_status = NO_RECONNECT;
#endif

	Adap_set_state_control(pAdap, (UINT8)DEV_ACTIVE, 0, 0);
	DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_start_bss return\n");

}




#ifdef  NEW_SUPRISING_REMOVED


/**************************************************************************
  *Dsp_Suprise_Removed_Handler()
  *Description:
  *	called by task module, when device was suprising removed, this handler is called. 
  *	the flow is almost same with routine adap_halt() except interaction with hardware 
  *	parts and tx part. For the tx case, when removed ,we need not wait for tx complete,
  *	it is meaningless, because device is not exist , so we just release the packet in tx list.
  *	Maybe there are something more to thought over, need to be fixed later!!(Jakio)
  *Para:
  *	pAdap: IN, pointer to adaptor
  *Return Value: 
  *	void
  **************************************************************************/
VOID Dsp_Suprise_Removed_Handler(PDSP_ADAPTER_T pAdap)
{

//	BOOLEAN IsTimerCancelled, IrpCancelStatus;
//	UINT32 tmpcounts,icounts;
//	UINT8 tmpchar;
	UINT32  loop;
			

	DBG_WLAN__MAIN(LEVEL_TRACE,"3DSP_suprise_removed\n");
	if(pAdap->driver_state == DSP_SUPRISE_REMOVE)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"pnp suprise but now driver has into DSP_SUPRISE_REMOVE \n");
		return;
	}

       if(STATUS_SUCCESS == Vcmd_Set_8051_MinLoop(pAdap,1))
       {
       	loop  = 10;
       	while(loop)
		{	
			if(pAdap->hw_8051_work_mode == INT_8051_IN_MINILOOP_MODE)
			{
				break;
			}
			sc_sleep(1);	
			loop--;
		}
       }

       if(pAdap->hw_8051_work_mode != INT_8051_IN_MINILOOP_MODE)
       {
       	DBG_WLAN__MAIN(LEVEL_TRACE,"maybe 8051 fail in enter miniloop when pnp suprise\n");
       }

	DBG_WLAN__MAIN(LEVEL_TRACE,"end pnp suprise task\n");
	Adap_Set_Driver_State(pAdap,DSP_SUPRISE_REMOVE);
	return;

}
#endif /*NEW_SUPRISING_REMOVED*/





//set  any wep key into hw to prevent txstop happen.
//txstop will happen when 3dsp sta is before joinok with a broadcast encrypted frame received.
//the function should be called before init and after reset
VOID Adap_set_any_wep_prevent_tx_stop(PDSP_ADAPTER_T pAdap)
{
	
	wpakey_info_t   keyspace;
	UINT32 i;
	UINT8  bc_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	for(i = 0; i < 3; i++)
	{
		keyspace.multicast = TRUE;
		keyspace.cyper = KEY_CONTROL_CIPHER_TYPE_WEP;
		keyspace.WEP_on = TRUE;
		keyspace.in_use = TRUE;
		keyspace.keyid = 0;
		keyspace.index = i;
		keyspace.key_len_in_bytes = 5;
		keyspace.key[0] = 0x11;
		keyspace.key[1] = 0x11;
		keyspace.key[2] = 0x11;
		keyspace.key[3] = 0x11;
		keyspace.key[4] = 0x11;
		WLAN_COPY_ADDRESS(keyspace.mac_address, bc_addr);

		Adap_mac_set_key(pAdap,(PVOID)&keyspace);	
	}
}

	
	

/*******************************************************************
 *   Rts_running_mode()
 *   
 *   Descriptions:
 *      This routine return either rts or cts to self is used.
 *      these change include:
 *   Arguments:
 *      
 *   Return Value:
 ******************************************************************/
UINT32 Rts_running_mode(
	PDSP_ADAPTER_T pAdap
	)
{
//	BOOLEAN  update_needed;
//	UINT32      new_rts_state;

	if((pAdap->wlan_attr.cts_to_self_config == CTS_SELF_ENABLE) &&
		(pAdap->wlan_attr.cts_en == CTS_SELF_ENABLE) &&
		(pAdap->wlan_attr.cts_current_state == RTSCTS_STATE_CTSSELF_WORK))
	{
		return RTSCTS_STATE_CTSSELF_WORK;
	}

	return RTSCTS_STATE_RTS_WORK;
}


//do not change state in some cases
VOID Adap_Set_Driver_State(PDSP_ADAPTER_T pAdap,DSP_DRIVER_STATE_T state)
{
	if(state == DSP_HARDWARE_RESET
		||state == DSP_SUPRISE_REMOVE
		||state == DSP_DRIVER_HALT
		||state == DSP_POWER_MANAGER
		||state == DSP_SYS_RESET)//justin:	080807.	must change state if in these states....!!!!!
	{
		pAdap->driver_state =state;
		return;
	}
	
	if(pAdap->driver_state == DSP_HARDWARE_RESET
		||pAdap->driver_state == DSP_SUPRISE_REMOVE
		||pAdap->driver_state == DSP_DRIVER_HALT
		||pAdap->driver_state == DSP_POWER_MANAGER
		||pAdap->driver_state == DSP_SYS_RESET)//justin:	080807.	no permition to change state if in these states....!!!!!
	{
		return;
	}
	
	pAdap->driver_state =state;
}


//must set to the state
VOID Adap_Set_Driver_State_Force(PDSP_ADAPTER_T pAdap,DSP_DRIVER_STATE_T state)
{
	pAdap->driver_state =state;
}


//please 8051 do it at start flow
VOID Adap_Set_Usbbus_Enable_Reset(PDSP_ADAPTER_T pAdap)
{
	UINT8 uchar;
	uchar = Basic_ReadRegByte(pAdap,REG_USBCTL1);
	uchar |= BIT6;
	Basic_WriteRegByte(pAdap,uchar,REG_USBCTL1);
}

UINT32 Adap_Get_8051FW_VERSION(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	DSP_8051FW_VERSION   val;

	val.request_cmd = VCMD_API_GET_8051VER_REQUEST;

//readloop:	
	//wait until previous access end
	
	//4 reset event
	sc_event_reset(&pAdap->DspReadEvent);
	
	status = Vcmd_Send_API(pAdap,(PUINT8)&val,sizeof(DSP_8051FW_VERSION));
	
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
		if(pAdap->DspRegRead.type != VCMD_API_GET_8051VER_REQUEST)
			goto err_exit;
		return pAdap->DspRegRead.result;
	}
	
err_exit:
	DBG_WLAN__INIT(LEVEL_ERR ,"[%s] Adap_Get_8051FW_VERSION error exit \n",__FUNCTION__);
	
	return 0;
}

TDSP_STATUS
Adap_update_slot_time(
	PDSP_ADAPTER_T pAdap,UINT8 newSlotTime)
{
	UINT32  		val;
//	UINT32           tmp;

	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TIMINGS_2REG);
	val = ((val & (~V4MAC_SLOT_TIME_BIT)) | (newSlotTime << 22));
	//write timing2
	VcmdW_3DSP_Dword(pAdap,val, WLS_MAC__TIMINGS_2REG);

	//
	val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_16_19);	

	if (newSlotTime == 20)
	{
		val |= BBREG_SLOT_TIME_BIT;
	}
	else
	{
		val  &= (~BBREG_SLOT_TIME_BIT);
	}
	
	
	//VcmdW_3DSP_Dword(pAdap, val, WLS_MAC__BBREG_20_23);
	VcmdW_3DSP_Mailbox(pAdap, val, WLS_MAC__BBREG_16_19,(PVOID)__FUNCTION__);

	return STATUS_SUCCESS;	

}
BOOLEAN Adap_check_rssi(PDSP_ADAPTER_T pAdap,PUINT32 new_reg)
{
	BBREG2023_REG_T   bbreg;
	if(pAdap->wlan_attr.hasjoined != JOIN_HASJOINED)
	{
		return FALSE;
	}	

	//reset memory
	sc_memory_set(&bbreg, 0, sizeof(UINT32));

	//DBGSTR(("CHeck rssi, rssi = %x,auto=%x,doze = %x,ap_as= %x\n",
	//	pAdap->wlan_attr.rssi ,
	//	pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr,
	//	pAdap->wlan_attr.gdevice_info.bbreg2023.soft_doze,
	//	pAdap->wlan_attr.gdevice_info.bbreg2023.ap_associated
	//	));
	
	if(GetRssi(pAdap->wlan_attr.rssi)  > AUTO_CORR_OFF_RSSI_HIGH_TH_V4) 
	{
		//now is power detect mode
		if(pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr == 0)
		{
			bbreg.auto_corr  = 1;
			bbreg.ap_associated =1;
			bbreg.soft_doze = 1;
			sc_memory_copy((PUINT8)new_reg,(PUINT8)&bbreg,sizeof(UINT32));
			return TRUE;
		}			
	}
	else if(GetRssi(pAdap->wlan_attr.rssi)  < AUTO_CORR_OFF_RSSI_LOW_TH_V4) 
	{
		//now is power detect mode
		if(pAdap->wlan_attr.gdevice_info.bbreg2023.auto_corr == 1)
		{
			bbreg.auto_corr  = 0;
			bbreg.ap_associated =1;
			bbreg.soft_doze = 1;
			sc_memory_copy((PUINT8)new_reg,(PUINT8)&bbreg,sizeof(UINT32));
			return TRUE;
		}			
	}
	return FALSE;	
}

VOID Adap_CounterMeasureTimeout(PVOID param)

{
//	TDSP_STATUS status;
	PDSP_ADAPTER_T pAdap;
	
	/*Get the pointer of Adapter context.*/
	pAdap = (PDSP_ADAPTER_T)param;

	DBG_WLAN__MAIN(LEVEL_TRACE,"* * * * * Adap_CounterMeasureTimeout \n");

	//Don't do this during hardware reset
	if(pAdap->driver_state == DSP_POWER_MANAGER
			|| pAdap->driver_state == DSP_HARDWARE_RESET)
	{
		Adap_InitTKIPCounterMeasure(pAdap);
		return;
	}

	if(Adap_Driver_isHalt(pAdap))
	{
		Adap_InitTKIPCounterMeasure(pAdap);
		return;
	}
	Adap_StartTKIPCounterMeasure(pAdap,REQUEST_CM_TIMEOUT,0);	
}

TDSP_STATUS Adap_request_Set_Channel(PDSP_ADAPTER_T pAdap, UINT8  chan)
{

//	UINT8  type;
//	UINT32 	loop = VENDOR_LOOP_MIDDLE;
	DSP_CHANNEL_INFO 	chanInfo;
	UINT32 errloop = 10;
	TDSP_STATUS status;
	

	//send flush tx command
	chanInfo.request_cmd = VCMD_API_SET_CHANNEL_REQUEST;
	chanInfo.chan = chan;

	while(errloop)
	{
		status = Vcmd_Send_API(pAdap,(PUINT8)&chanInfo, sizeof(DSP_CHANNEL_INFO));
		if(STATUS_ADAPTER_NOT_READY != status)
		{
			break;		
		}
		errloop--;
		sc_sleep(2);   // 2ms
	}

	if(status != STATUS_SUCCESS)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Adap_request_Set_Channel() ERROR EXIT \n");
	}
	
	return status;	
}

/*******************************************************************
 *   Adap_SetPowerD3
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_PNP_SET_POWER is set.
 *Jakio20070711 changed this routine:
 *	There are lots things to do when in power save mode, mainly can be devided into three types:
 *	1) Inform Ndis the connection is down, then Ndis will not send packets any more
 *	2) Actions interacting with device:
 *		<1> Set MACHW into IDLE state 
 *		<2> Flush Tx&Rx FIFO
 *		<3> Set MACHW into DOZE state
 *		<4> Inform 8051 into MIN_LOOP to save power
 *	3) Operatons inside driver
 *		<1> Cancel timers
 *		<2> Release all the buffered data: mng, data, and packets backed in retry_list, if exists
 *		<3> Waiting all Irps return
 *		<4> Reset Task, MngQueue,pktlist, and retry module(if exists)
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_SetPowerD3(PDSP_ADAPTER_T pAdap)
{
	PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;

	pAdap->ps_state = 1;	
	//combo
	if(Adap_Driver_isHalt(pAdap))
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "Do nothing in Adap_HardwareReset(), because adap has halt or been removed\n");
		return;
	}

	DBG_WLAN__MAIN(LEVEL_TRACE,"%s entry, %d auth_mode = %d\n", __FUNCTION__, pt_mattr->gdevice_info.privacy_option, pAdap->wlan_attr.auth_mode);

	if (pAdap->link_ok == LINK_OK)
	{
		UINT8	addr[WLAN_ETHADDR_LEN];
		DBG_WLAN__IOCTL(LEVEL_TRACE,"%s:DisAssoc  auth_mode = %d\n", __FUNCTION__, pAdap->wlan_attr.auth_mode);
			
		pAdap->wlan_attr.set_disassoc_flag =1;
		sc_memory_set(addr, 0xff, WLAN_ETHADDR_LEN);

		Mng_BssDisAssoc(pAdap, addr, UNSPE_FAIL);
		Mng_InitParas(pAdap);

        // 12-21 , clear scan list.
    	{
    		PMNG_STRU_T     pt_mng = pAdap->pmanagment;
    		pt_mng->oiddeslist.bssdesnum = 0;
    		sc_memory_set(&(pt_mng->oiddeslist),0, sizeof(MNG_DESLIST_T));
    		sc_memory_set(&(pAdap->mib_stats),0, sizeof(MIB_STATISTIC_T));
    	}
    	//

	}

	Adap_Set_Driver_State_Force(pAdap,DSP_STOP_TX);		//stop tx, oid

	Adap_Set_Usbbus_Enable_Reset(pAdap);

	/* Set halt flag to notify receive module(bulk in) not processing any more */
	//pAdap->halt_flag = TRUE;
	
	/* Cancel all timers. */
	wlan_timer_kill(&pAdap->mng_timer);
	wlan_timer_kill(&pAdap->sys_timer);
	wlan_timer_kill(&pAdap->tx_countermeasure_timer);
	wlan_timer_kill(&pAdap->periodic_timer);

	/* Don't send data any more. */
	Adap_SetConnection(pAdap, JOIN_NOJOINED);

	if(STATUS_SUCCESS != Vcmd_Set_8051_MinLoop(pAdap,FALSE))
	{
		pAdap->is_set_8051_miniloop_failed = TRUE;
		DBG_WLAN__MAIN(LEVEL_TRACE, "Vcmd_Set_8051_MinLoop failed\n");
	}
//	sc_sleep(1);

	Adap_Set_Driver_State_Force(pAdap,DSP_POWER_MANAGER);
	
	Int_CancelInt(pAdap);
	
	sc_sleep(300);
	
	Rx_stop(pAdap);

	sc_sleep(300);

	Tx_Stop(pAdap);

	Tx_Cancel_Data_Frm(pAdap);

	//Reset task
	Task_Reset((PDSP_TASK_T)pAdap->ppassive_task);

	Tx_Reset(pAdap);

	sc_iw_send_bssid_to_up(pAdap->net_dev, NULL);
	
	DBG_WLAN__MAIN(LEVEL_TRACE,"%s exit privacy_option = %d\n", __FUNCTION__, pt_mattr->gdevice_info.privacy_option);
}

/*******************************************************************
 *   Adap_SetPowerD0
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_PNP_SET_POWER is set.
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *   Return Value:
 *      NONE
 ******************************************************************/
VOID Adap_SetPowerD0(PDSP_ADAPTER_T pAdap)
{
	UINT8 uCounter;
	BOOLEAN		bIs_set_8051_miniloop_failed;
	PMNG_STRU_T pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
	PDSP_ATTR_T   	pt_mattr = &pAdap->wlan_attr;

	pAdap->ps_state = 0;
	bIs_set_8051_miniloop_failed = pAdap->is_set_8051_miniloop_failed;
	
	DBG_WLAN__MAIN(LEVEL_TRACE, "***************%s %d %d\n", __FUNCTION__, pt_mattr->gdevice_info.privacy_option, 
		pt_mng->statuscontrol.curstatus);

//	Adap_UpdateMediaState(pAdap, LINK_FALSE);
	//combo
	Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_READY);
	//Adap_Set_Driver_State_Force(pAdap,DSP_SUPRISE_REMOVE);
	pAdap->OID_not_allow_flag = TRUE;

	uCounter = 0;	
	while(Adap_Device_Removed(pAdap))
	{
		if(uCounter++ >= 4)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE, "****%s-device removed\n", __FUNCTION__);
			//NdisMSetInformationComplete(pAdap->dsp_adap_handle,NDIS_STATUS_SUCCESS);//NDIS_STATUS_FAILURE);
			Adap_Set_Driver_State_Force(pAdap,DSP_SUPRISE_REMOVE);
			return;
		}

		DBG_WLAN__MAIN(LEVEL_TRACE, "****%s-device removed check times = %x\n", __FUNCTION__, uCounter);
		sc_sleep(50);
	}

	
	//begin init hw and pipe
	do
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "****** 0 *********%s\n", __FUNCTION__);
		
		//8051 has run
		 if(Vcmd_8051fw_isWork(pAdap))
		 {
			Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_READY);
		    	
			if (STATUS_SUCCESS != Int_SubmitInt(pAdap))
				return ;
			 
			Adap_Set_Usbbus_Enable_Reset(pAdap);

		       pAdap->wlan_attr.chipID = 4;
			   
		    	DBG_WLAN__INIT(LEVEL_TRACE, "[%s] init process with 8051 running\n",__FUNCTION__);
		    	sc_event_reset(&pAdap->is_8051_ready_event);
		     	if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
		    	{
		    		DBG_WLAN__INIT(LEVEL_ERR,"[%s]: req join vcmd failure\n",__FUNCTION__);
		    		return ;
		    	}
		    	
		    	//wait for 8051 ready		
		    	if( 0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
		    	{
		    	 	if(Vcmd_Funciton_Req_JOIN(pAdap) != STATUS_SUCCESS)
		    		{
		    			DBG_WLAN__INIT(LEVEL_ERR, "[%s]: req join vcmd failure\n",__FUNCTION__);
		    			return ;
		    		}

		        	if(0 == sc_event_wait(&pAdap->is_8051_ready_event, 1000))
		        	{
		        		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Initialize failure : no 51 ready event\n",__FUNCTION__);
		        		return ;
		        	}		    	
		    	}
		    	DBG_WLAN__INIT(LEVEL_TRACE, "[%s] join int returned\n",__FUNCTION__);			   
		 }
		else
		{
			 //8051 hasn't run			
			DBG_WLAN__MAIN(LEVEL_TRACE, "****%s-8051 no running\n", __FUNCTION__);
			if(Adap_Get8051FwCodesAndDownload( pAdap) != STATUS_SUCCESS)
			{
				DBG_WLAN__MAIN(LEVEL_TRACE, "****%s- 8051 download fail\n", __FUNCTION__);
				break;
			}
			if (STATUS_SUCCESS != Int_SubmitInt(pAdap))
				return;
		}
		//add for hibernate except
		sc_sleep(5);
	
		DBG_WLAN__MAIN(LEVEL_TRACE, "******1*********%s\n", __FUNCTION__);
		if(Vcmd_CardBusNICReset(pAdap)  != STATUS_SUCCESS)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE, "****%s Vcmd_CardBusNICReset  fail\n", __FUNCTION__);
			break;
		}

		DBG_WLAN__MAIN(LEVEL_TRACE, "******2*********%s\n", __FUNCTION__);

		// Download the firmware codes
		if (Adap_GetDspFwCodesAndDownload(pAdap) != STATUS_SUCCESS)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE, "****%s-device download dsp code failed\n", __FUNCTION__);
			break;
		} 
		
		// Delay 2 ms 
//		sc_sleep (2);

		DBG_WLAN__MAIN(LEVEL_TRACE, "******3*********%s\n", __FUNCTION__);

		if (STATUS_SUCCESS!= Adap_reset_dsp_chip(pAdap))
		{
			DBG_WLAN__MAIN(LEVEL_TRACE, "reset dsp chip failure in %s\n", __FUNCTION__);
			break;
		}
    
		DBG_WLAN__MAIN(LEVEL_TRACE, "******4*********%s\n", __FUNCTION__);
		
		Vcmd_Set_Encryption_Mode(pAdap);
	
//		Adap_w2lan_resetting(pAdap);

		Mng_ClearPara(pAdap);
    
		DBG_WLAN__MAIN(LEVEL_TRACE, "******5*********%s\n", __FUNCTION__);

		    /* Set download-ok flag to 8051 */
		if(STATUS_SUCCESS !=Vcmd_Set_Firmware_Download_Ok(pAdap,TRUE))
	       {
			DBG_WLAN__MAIN(LEVEL_TRACE, "reset dsp chip failure in %s", __FUNCTION__);
			break;
		}
	
		sc_sleep(5);

		DBG_WLAN__MAIN(LEVEL_TRACE, "******6*********%s\n", __FUNCTION__);

		pAdap->wlan_attr.hasjoined = JOIN_NOJOINED;

//		Adap_SetLink(pAdap, LINK_FALSE);		
//		Mng_InitParas(pAdap);

		DBG_WLAN__MAIN(LEVEL_TRACE, "******7*********%s\n", __FUNCTION__);

		//combo
		Adap_Set_Usbbus_Enable_Reset(pAdap);
		Adap_Set_Driver_State_Force(pAdap, DSP_DRIVER_WORK);
	
		//begin rx
		Rx_restart(pAdap);


		Adap_Initial_Timer(pAdap);
  	}while(0);

	if (pAdap->driver_state == DSP_DRIVER_WORK)
	{
	DBG_WLAN__MAIN(LEVEL_TRACE, "******8*********%s %d\n", __FUNCTION__, pt_mng->statuscontrol.curstatus);

	Adap_SetLink(pAdap, LINK_FALSE);

	Tx_Restart(pAdap);
	}
	else
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "******8*********%s, error, driver_state = %d\n", __FUNCTION__, pAdap->driver_state);
		return;
	}
//		Adap_w2lan_resetting(pAdap);
	 
//		Mng_Reset(pAdap);

#if 0//def ROAMING_SUPPORT//justin: 080613.	no need to do this. just disconnect...		do these things maybe cause BSD in vista
	if (pAdap->link_ok == LINK_OK)//confirm the link status only when connected before d3
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"$$$$$ %s oidscanflag = %d, reconnect_status = %d \n",__FUNCTION__,pt_mng->oidscanflag,pAdap->reconnect_status); 

			if ((!pt_mng->oidscanflag) && (pAdap->reconnect_status == NO_RECONNECT))//(!pAdap->can_do_reconnect_flag))//(!pAdap->scanning_flag) && 
			{

				pAdap->reconnect_status = NEED_RECONNECT;
				
				if((!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN))
					&&(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_OID_DISASSOC))
				&&(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_OID_SET_SSID)))
					)
				{
				DBG_WLAN__MAIN(LEVEL_TRACE,"^^^^^^%s, do scan %d\n", __FUNCTION__,pt_mng->usedbssdes.channel);

				//woody 080630
					//set the two variable in task
					pAdap->scanning_flag = FALSE;
					pAdap->scan_watch_value = 0;

				pAdap->bStarveMac = TRUE;		//stop tx... avoid tx hang
						
					//save datarate to recover latter
					pt_mng->rateIndex_b4_roaming = pt_mng->rateIndex;

				pt_mng->bssdeslist.bssdesnum = 0;		//justin: 080902.	init bssdesnum before do beacon lost scan, otherwise, the OLD AP's info is still in list
				pt_mng->oiddeslist.bssdesnum = 0;

//				Mng_Reset(pAdap);
//				Adap_set_channel(pAdap, pt_mng->usedbssdes.channel);
//				Mng_MakeAssocReqFrame(pAdap);
//				Mng_DoJoin(pAdap, 0);
				
				if(STATUS_SUCCESS == Task_CreateTask(
						(PDSP_TASK_T)pAdap->ppassive_task,
						DSP_TASK_EVENT_SCAN,
						DSP_TASK_PRI_HIGH,
						NULL, 0))
				{
					DBG_WLAN__MAIN(LEVEL_TRACE,"Add Scan Task\n");						
				}
				else
				{
					DBG_WLAN__MAIN(LEVEL_ERR,"Add Scan Task Failure.\n");								
				}
				}
			}
	}	
#endif	//#ifdef ROAMING_SUPPORT_DEL		

//	pt_mattr->gdevice_info.privacy_option = FALSE;

	//NdisMSetInformationComplete(pAdap->dsp_adap_handle,NDIS_STATUS_SUCCESS);
	DBG_WLAN__MAIN(LEVEL_TRACE, "***********%s ok, %d %d\n",__FUNCTION__, pt_mattr->gdevice_info.privacy_option, pt_mng->statuscontrol.curstatus);

    Adap_InitTKIPCounterMeasure(pAdap);

	sc_iw_send_bssid_to_up(pAdap->net_dev, NULL);
	
	pAdap->is_set_8051_miniloop_failed = FALSE;	//set FALSE to accept interrupt from 8051
}

/*******************************************************************
 *   Oid_SetAddWep
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_802_11_ADD_WEP is set.
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *      buf: IN,  the pointer to the buffer saved the data .
 *      len: IN, the length of data.
 *    Return Value:
 *      NONE
 ******************************************************************/
VOID Oid_SetAddWep(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len)
{
//	wpakey_info_t*	wpa_key_info= pAdap->wlan_attr.key_map;
	UINT32			index;
//	BOOLEAN		 global_flag;
	PNDIS_802_11_WEP pTemp = (PNDIS_802_11_WEP)buf;
	


	
	//get index
	index = pTemp->KeyIndex & 0x7fffffff;
	index = (index > 3) ? 3 : index;

	//check valid of length
	if(	pTemp->KeyLength != KEY_BIT40_LEN 
	&&	pTemp->KeyLength != KEY_BIT104_LEN
//	&&	pTemp->KeyLength != 0 
	)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE,"Key length error \n");
	}

	
	Adap_del_key_from_hw(pAdap,4);
	Adap_del_key_from_hw(pAdap,5);
	Adap_del_key_from_hw(pAdap,6);
	Adap_del_key_from_hw(pAdap,7);


	//set group key into key_map structure
	if(	pTemp->KeyLength != 0
	&&	STATUS_SUCCESS ==Adap_add_key_into_mib(pAdap,
			(UINT8)index,
			(UINT8)pTemp->KeyLength,
			pTemp->KeyMaterial,
			NULL,//  broad address
			TRUE,   //group
			KEY_CONTROL_CIPHER_TYPE_WEP)
	)
	{
		//set key map into hw
		Adap_push_key_into_hw(pAdap);
	}
	else
	{
	}

	if(pTemp->is_default_key)
	{
		pAdap->wlan_attr.wpa_group_key_index = (UINT8)index;
		pAdap->wlan_attr.default_key_index = index;
	}
	
	pAdap->wlan_attr.wpa_pairwise_key_valid =1;
	pAdap->wlan_attr.wpa_group_key_valid =1;
	
	pAdap->wlan_attr.gdevice_info.privacy_option = TRUE;

			
	//Jakio20070621: inform 8051 the encryption mode
	Vcmd_Set_Encryption_Mode(pAdap);
	//DBGSTR_OID(("^^^^add wep end ^^^^^\n "));	

}

/*******************************************************************
 *   Oid_SetRemoveWep
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_802_11_REMOVE_WEP is set.
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *      buf: IN,  the pointer to the buffer saved the data .
 *      len: IN, the length of data.
 *    Return Value:
 *      NONE
 ******************************************************************/
VOID Oid_SetRemoveWep(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len)
{
	NDIS_802_11_KEY_INDEX KeyIndex;
	UINT32 index;
	wpakey_info_t * wpa_key_info;



	wpa_key_info = pAdap->wlan_attr.key_map;
	sc_memory_copy(&KeyIndex, buf, len);
	index = KeyIndex & 0x7fffffff;

	//ASSERT(index < 3);
	if(pAdap->wlan_attr.default_key_index == index)
	{
		pAdap->wlan_attr.gdevice_info.privacy_option = FALSE;
#if 1		
//		Adap_request_flush_tx(pAdap);
		{//Justin: 0913			set a default key if find some key WEP_on
			wpakey_info_t  *keyspace;
			UINT32 i;

			for(i = 0; i< REAL_KEY_ARRAY_SIZE;i++)
			{
				keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[i];
				if(keyspace->WEP_on && keyspace->in_use)
				{
					pAdap->wlan_attr.default_key_index = (UINT32)i;
					pAdap->wlan_attr.wpa_group_key_index = (UINT8)i;
					pAdap->wlan_attr.wpa_pairwise_key_valid =1;
					pAdap->wlan_attr.wpa_group_key_valid =1;
					pAdap->wlan_attr.gdevice_info.privacy_option = TRUE;
					break;
				}
			}	
		}
#endif
		Vcmd_Set_Encryption_Mode(pAdap);
	}	
	//?? inform 8051

	if(wpa_key_info[index].in_use == TRUE)
	{
		//delete key  from hw
		Adap_del_key_from_hw(pAdap, index);
		
		wpa_key_info[index].in_use = FALSE;
	}
	
	wpa_key_info[index].WEP_on = FALSE;
//	pAdap->stop_tx_flag = FALSE;

}

/*******************************************************************
 *   Oid_SetAddKey
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_802_11_ADD_KEY is set.
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *      buf: IN,  the pointer to the buffer saved the data .
 *      len: IN, the length of data.
 *    Return Value:
 *      NONE
 ******************************************************************/
VOID Oid_SetAddKey(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len)
{
//	PDSP_ATTR_T           			pt_mattr = &pAdap->wlan_attr;	
	PMNG_STRU_T				pt_mng = pAdap->pmanagment;
	PADD_KEY 		pTemp = (PADD_KEY)buf;
	UINT32						index;
//	UINT32						i;

//       DBG_WLAN__MAIN(LEVEL_TRACE,"[%s] group=%u, tx=%u, rsc=0x%016lx, wep=%u, index=%u\n",
//		__FUNCTION__, pTemp->is_group_key, pTemp->is_tx_key, pTemp->KeyRSC, pTemp->wep_mode, pTemp->KeyIndex);
//	DBG_PRINT_BUFF("BSSID", pTemp->BSS_ID,6);
//	DBG_PRINT_BUFF("Key", pTemp->KeyMaterial,pTemp->KeyLength);

  	// bit29, bit28
	//When Bit 29 is set to 1, the KeyRSC should be used to set the initial receive SC for the key. When it is 0, the receive SC should be set by the NIC.
	//When Bit 28 is set to 1, the key is being set by an Authenticator. If the bit is set to zero, the key is being set by a Supplicant. It must be set to 0 in IBSS WPA mode. 

	index = pTemp->KeyIndex;

//	DBG_WLAN__MAIN(LEVEL_TRACE,"***  %s Begin !***  \n", __FUNCTION__);
	if(TRUE)
	{
		//Wait untill tx count = 0;
		ULONG val;
		ULONG readCount=10;
		val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
		val &= 0xff;
		while(val != 0)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"OID set key : COUNT = %x, times = %d!\n",val, readCount );
			//wait 10ms
			if(readCount == 0)
			{
				break;
			}
			readCount--;
			sc_sleep(200);					
			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
			val &= 0xff;
		}
		
		if(readCount == 0)
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"OID set key,maybe tx hang happen,COUNT = %x, times = %d!\n",val, readCount );
		}
		else
		{
			DBG_WLAN__MAIN(LEVEL_INFO,"OID set key,continue COUNT = %x, times = %d!\n",val, readCount );
		}
	}

	//get tx & pairwise flag
	//set for wep case
	if(	pTemp->KeyLength == 5 
	||	pTemp->KeyLength == 13)
	{
		Adap_del_key_from_hw(pAdap,4);
		Adap_del_key_from_hw(pAdap,5);
		Adap_del_key_from_hw(pAdap,6);
		Adap_del_key_from_hw(pAdap,7);
		Adap_del_key_from_hw(pAdap,8);		

		if(pTemp->is_tx_key)
		{
			pAdap->wlan_attr.wpa_group_key_index = index;
			pAdap->wlan_attr.default_key_index = index;
		}

		DBG_WLAN__MAIN(LEVEL_TRACE,"DEFAULT key = %x\n",index);
		
		
		//set group key into key_map structure
		if(STATUS_SUCCESS == Adap_add_key_into_mib(pAdap,
			index,
			pTemp->KeyLength,
			pTemp->KeyMaterial,
			NULL,//  broad address
			TRUE,   //group
			KEY_CONTROL_CIPHER_TYPE_WEP)
		)
		{
			//set key map into hw
			Adap_push_key_into_hw(pAdap);
			pAdap->wlan_attr.wpa_pairwise_key_valid =1;
			pAdap->wlan_attr.wpa_group_key_valid =1;
//			pAdap->wlan_attr.default_key_index = index;
			pAdap->wlan_attr.gdevice_info.privacy_option = TRUE;
#if 1
			pAdap->wlan_attr.wep_mode = WEP_MODE_WEP;
			pAdap->wlan_attr.group_cipher = WEP_MODE_WEP;
	            
			/*if(pAdap->wlan_attr.auth_mode_data.auth_alg == AUTH_ALG_OPEN)*/
			if(pAdap->wlan_attr.auth_alg == AUTH_ALG_OPEN)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_OPEN;
				pAdap->wlan_attr.auth_alg  = AUTH_ALG_OPEN;
			}
			/*else if(pAdap->wlan_attr.auth_mode_data.auth_alg == AUTH_ALG_SHARED_KEY)*/
			else if(pAdap->wlan_attr.auth_alg == AUTH_ALG_SHARED_KEY)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_SHARED_KEY;
				pAdap->wlan_attr.auth_alg  = AUTH_ALG_SHARED_KEY;
			}
#endif
			//Jakio20070621: inform 8051 the encryption mode
			Vcmd_Set_Encryption_Mode(pAdap);
			DBG_WLAN__MAIN(LEVEL_TRACE,"***  %s Success !***  \n", __FUNCTION__);
		}
		else
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"***  %s  fail!***  \n", __FUNCTION__);

        }
	}
	else
	{
		//work for wpa mode
		// Pairwise key
		if (!pTemp->is_group_key)
		{
			pAdap->wlan_attr.wep_mode = pTemp->wep_mode;
		
			//get mic pairwise key of tkip
			if (pTemp->KeyLength == 32)
			{			
				sc_memory_copy(pAdap->wlan_attr.wpa_pairwise_mic_tx, pTemp->KeyMaterial+ WLAN_WEP2_TKLEN, WLAN_MIC_KEY_LEN);
				sc_memory_copy(pAdap->wlan_attr.wpa_pairwise_mic_rx, pTemp->KeyMaterial + WLAN_WEP2_TKLEN + WLAN_MIC_KEY_LEN, WLAN_MIC_KEY_LEN);
			}


			if (pTemp->keyRSC_valid)
			{
				sc_memory_copy(&pAdap->wlan_attr.wpa_pairwise_tsc_rx_counter.tsc64, pTemp->KeyRSC,8);
			}
			else
			{
				//woody add
				pAdap->wlan_attr.wpa_pairwise_tsc_rx_counter.tsc64 = 0;
			}
			
			index = 0;
			
			if(STATUS_SUCCESS == Adap_add_key_into_mib(pAdap,
							index,
							WLAN_WEP2_TKLEN,
							pTemp->KeyMaterial,
							pTemp->BSS_ID,//  broad address
							FALSE,//!pairwise_flag,   //group
							pAdap->wlan_attr.wep_mode))
			{
				//set key map into hw
				Adap_push_key_into_hw(pAdap);
				pAdap->wlan_attr.wpa_pairwise_key_valid = 1;
				pAdap->wlan_attr.gdevice_info.privacy_option = TRUE;
				DBG_WLAN__MAIN(LEVEL_TRACE,"PMK: set add key success\n");
			}
			else
			{
				DBG_WLAN__MAIN(LEVEL_ERR,"PMK: set add key failure\n");
			}

		
		}
		//for group key
		else
		{
		
			//DBG_WLAN__MAIN(LEVEL_TRACE,"Add Group Key\n");
			
			pAdap->wlan_attr.group_cipher = pTemp->wep_mode;

			//get group key
			//NdisMoveMemory(&pAdap->wlan_attr.wpa_group_key[index], pTemp->KeyMaterial, WLAN_WEP2_TKLEN);
			//get mic key of tkip
			if (pTemp->KeyLength == 32)
			{
				sc_memory_copy(pAdap->wlan_attr.wpa_group_mic_tx, &pTemp->KeyMaterial[WLAN_WEP2_TKLEN], WLAN_MIC_KEY_LEN);
			    sc_memory_copy(&pAdap->wlan_attr.wpa_group_mic_rx[index][0], &pTemp->KeyMaterial[WLAN_WEP2_TKLEN + WLAN_MIC_KEY_LEN], WLAN_MIC_KEY_LEN);
                //sc_memory_copy(pAdap->wlan_attr.wpa_group_mic_rx, &pTemp->KeyMaterial[WLAN_WEP2_TKLEN + WLAN_MIC_KEY_LEN], WLAN_MIC_KEY_LEN);
			}

			//set group key index into para
			pAdap->wlan_attr.wpa_group_key_index = index;

			//
			if (pTemp->keyRSC_valid)
			{
				// set wpa_pairwise_tsc_rx_counter
                sc_memory_copy(&pAdap->wlan_attr.wpa_group_tsc_rx_counter.tsc64, pTemp->KeyRSC,8);
			}
			else
			{
				//woody add
				pAdap->wlan_attr.wpa_group_tsc_rx_counter.tsc64 = 0;
			}
			

			if(STATUS_SUCCESS == Adap_add_key_into_mib(pAdap,
							index,
							WLAN_WEP2_TKLEN,
							pTemp->KeyMaterial,
							pt_mng->usedbssdes.bssid,//  broad address
							TRUE,
							pAdap->wlan_attr.group_cipher))
			{
				//set key map into hw
				Adap_push_key_into_hw(pAdap);
				pAdap->wlan_attr.wpa_group_key_valid = 1;				
				pAdap->wlan_attr.wpa_group_key_index = index;
				
				if(pTemp->is_tx_key)
				{
					pAdap->wlan_attr.default_key_index = index;
				}
				
				DBG_WLAN__MAIN(LEVEL_TRACE,"GTK: set add key success\n");
			}
			else
			{
				DBG_WLAN__MAIN(LEVEL_ERR,"GTK: set add key failure\n");
			}
			
		}

		Vcmd_Set_Encryption_Mode(pAdap);

	}
	
	sc_event_set(&pAdap->set_key_event);
//	DBG_WLAN__MAIN(LEVEL_TRACE,"***  %s End !***  \n", __FUNCTION__);
	return;
}

/*******************************************************************
 *   Oid_SetRemoveKey
 *
 *   Descriptions:
 *     This function runs in PASSIVE LEVEL and it is called when OID_802_11_REMOVE_KEY is set.
 *   Arguments:
 *      pAdap: IN, The pointer to adapter context.
 *      buf: IN,  the pointer to the buffer saved the data .
 *      len: IN, the length of data.
 *    Return Value:
 *      NONE
 ******************************************************************/
VOID Oid_SetRemoveKey(PDSP_ADAPTER_T pAdap, PUINT8 buf, UINT32 len)
{
//#ifdef  NDIS51_MINIPORT
	UINT32 i;
	wpakey_info_t * wpa_key_info;
	
	PNDIS_802_11_REMOVE_KEY pTemp = (PNDIS_802_11_REMOVE_KEY)buf;

	wpa_key_info = pAdap->wlan_attr.key_map;
	
//	DBG_WLAN__MAIN(LEVEL_TRACE,"%s Begin\n", __FUNCTION__);
	
	//pairwise key
	if(!pTemp->is_group_key)
	{
		//safe check, now there is only one key space for pairwise key
//		if(!wpa_key_info[MAX_WEP_KEY_NUM].in_use)
//		{
//			DBG_WLAN__MAIN(LEVEL_TRACE,"(!wpa_key_info[MAX_WEP_KEY_NUM].in_use)\n");
//			ASSERT(0);
//		}
		
//		wpa_key_info[MAX_WEP_KEY_NUM].in_use = FALSE;
//		wpa_key_info[MAX_WEP_KEY_NUM].WEP_on = FALSE;

		pAdap->wlan_attr.wpa_pairwise_key_valid = 0;
		//delete key from mac reg
		if(STATUS_SUCCESS !=Adap_del_key_from_hw(pAdap, MAX_WEP_KEY_NUM))
		{
			DBG_WLAN__MAIN(LEVEL_TRACE,"delete key for hw failure\n");
		}
	}
	else		//group key
	{
		if(pTemp->KeyIndex < MAX_WEP_KEY_NUM)
		{
			for(i = 0; i < MAX_WEP_KEY_NUM; i ++)
			{
				if(wpa_key_info[i].in_use== TRUE)
					Adap_del_key_from_hw(pAdap, i);
			}
			pAdap->wlan_attr.wpa_group_key_valid = 0;
		}
		
		if (wpa_key_info[1+5].in_use)
			Adap_del_key_from_hw(pAdap, 1+5);
	}

	//Justin: 0725
	for(i = 0; i < MAX_WEP_KEY_NUM+1; i ++)
	{
		if(wpa_key_info[i].WEP_on)
			break;
	}

	if(i == (MAX_WEP_KEY_NUM+1))//no key valid
	{
		pAdap->wlan_attr.gdevice_info.privacy_option = FALSE;//Justin: 0725
//		Adap_request_flush_tx(pAdap);
		Vcmd_Set_Encryption_Mode(pAdap);
	}
//	pAdap->stop_tx_flag =FALSE;
	
	sc_event_set(&pAdap->set_key_event);

//	DBG_WLAN__MAIN(LEVEL_TRACE,"%s End\n", __FUNCTION__);
	return;
}


VOID Oid_Set_RTS(PDSP_ADAPTER_T pAdap)
{
	PMNG_STRU_T    pt_mng = pAdap->pmanagment;
	Mng_Update_Cts_State(pAdap,pt_mng->usedbssdes.Erpinfo);
	return;
}

VOID Oid_SetInfrastructureMode(PDSP_ADAPTER_T pAdap)
{
	/* flush pairwise key */
	sc_memory_set(pAdap->wlan_attr.wpa_pairwise_mic_rx, 0, WLAN_MIC_KEY_LEN);
	sc_memory_set(pAdap->wlan_attr.wpa_pairwise_mic_tx, 0, WLAN_MIC_KEY_LEN);

	/* flush group key  */
	sc_memory_set(pAdap->wlan_attr.wpa_group_mic_rx, 0, WLAN_MIC_KEY_LEN*5);
	sc_memory_set(pAdap->wlan_attr.wpa_group_mic_tx, 0, WLAN_MIC_KEY_LEN);
}


#if 0
void*	Dsp_Initialize(PUSB_INFO usbinfo)
{
	PDSP_ADAPTER_T    pAdap = NULL;
    TDSP_STATUS        status;
    TDSP_SPINLOCK     lock;
    UINT32	loop;
   	UINT8	tmpBuf[8];
   	UINT8	Magic[4] = {0x33, 0x44, 0x53, 0x50};//stand for"3DSP"
 
    DBG_WLAN__ENTRY(LEVEL_TRACE,"%s usbinfo is %x,EP_control is %d,EP_bulkout_HEAD is %d!\n", 
                    __FUNCTION__,
                    (UINT32)usbinfo,
                    usbinfo->EP_control,
                    usbinfo->EP_bulkout_HEAD);
	/* Allocate the Adapter Object, exit if error occurs */
    sc_spin_lock_init(&lock);
    pAdap = (PDSP_ADAPTER_T)sc_memory_alloc(sizeof(DSP_ADAPTER_T));
	if(NULL == pAdap)
	{
		DBG_WLAN__ENTRY(LEVEL_ERR,"%s: ADAPTER Allocate Memory failed!\n", __FUNCTION__);
		return NULL;
    }
    /* Zero out the adapter object space */
   	sc_memory_set(pAdap, 0, sizeof(DSP_ADAPTER_T));
    status = UsbDev_Init(pAdap, usbinfo);
	DBG_WLAN__INIT(LEVEL_TRACE, "pAdap=%p, pAdap->usb_context=%p \n", pAdap, pAdap->usb_context);
	
	if (status != STATUS_SUCCESS)
	{
		//DBG_WLAN__INIT(LEVEL_ERR, "[%s] Usb device Init failed (Status = 0x%x)\n",__FUNCTION__,status);
		//Adap_FreeAdapterObject(pAdap);
		return NULL;
	}

    status = Tx_Init(pAdap);
    if (status != STATUS_SUCCESS)
	{
		//DBG_WLAN__INIT(LEVEL_ERR, "[%s] Usb device Init failed (Status = 0x%x)\n",__FUNCTION__,status);
		//Adap_FreeAdapterObject(pAdap);
		return NULL;
    }

    //1:here download 8051 fw
  	//first download 8051 firmware
    sc_event_init(&pAdap->is_8051_ready_event);
    sc_event_init(&pAdap->tx_fm_event);
    Adap_Set_Driver_State_Force(pAdap,DSP_DRIVER_INIT);
    
	DBG_WLAN__INIT(LEVEL_TRACE, "[%s] Start to download 8051 firmwire !\n",__FUNCTION__);

	if (Adap_Get8051FwCodesAndDownload(pAdap) != STATUS_SUCCESS)
	{
		DBG_WLAN__INIT(LEVEL_ERR, "[%s]: Adap_Get8051FwCodesAndDownload failure\n",__FUNCTION__);
//wumin			Adap_UnInitialize(pAdap);
//wumin			Adap_FreeAdapterObject(pAdap);
		return NULL;
	} 

	DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: Adap_Get8051FwCodesAndDownload Success!\n",__FUNCTION__);
	/* Delay 2 ms */
	sc_sleep (2);

    //check whether 8051 has completed initialization
	Basic_ReadBurstRegs(pAdap, tmpBuf, 4, 0x1FC);
	DBG_WLAN__INIT(LEVEL_TRACE ,  "0x1FC reg value is:%2x %2x %2x %2x\n",tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3]);

	loop=0;
	while(0 == sc_memory_cmp(Magic, tmpBuf, 4))
	{
		sc_sleep(1);	
		Basic_ReadBurstRegs(pAdap ,tmpBuf, 4,0x1FC);
		DBG_WLAN__INIT(LEVEL_TRACE, "[%s]: 8051 Firmware init not completed!\n",__FUNCTION__);	
		loop++;

		if(loop>30)
		{
			return NULL;
		}
	}
    return (void*)pAdap;
}
#endif
void* Adap_Get_Mac_Addr(void* adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return NULL;
    }
    return pAdap->permanent_address;
}
void Adap_Set_Netdev(void* adapt,void* netdev)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }
    pAdap->net_dev = netdev;
}
void* Adap_Get_Netdev(void* adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return NULL;
    }
    return pAdap->net_dev;
}

void Adap_Set_Netstat(void* adapt,void* netstat)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }
    pAdap->net_stats = netstat;
}

void* Adap_Get_Netstat(void* adapt)
{
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return NULL;
    }
    return pAdap->net_stats;
}


void  Adap_Get_StatData(void* adapt,PSTAT_DATA data)
{	
	PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }
	Tx_get_counts(pAdap, &data->tx_good, &data->tx_payload, &data->tx_err);
	Rx_get_counts(pAdap, &data->rx_good, &data->rx_payload, &data->rx_err, NULL, NULL, NULL, &data->rx_dropped, &data->rx_mc);
}


void Adap_Set_BusInf(void * adapt, void * pBusInf)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }

    pAdap->pBusInterfaceData = pBusInf;
}


void* Adap_Get_BusInf(void * adapt)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return NULL;
    }

    return pAdap->pBusInterfaceData;
}
void Adap_Set_Removed(void * adapt ,unsigned char removed)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }   
    pAdap->device_removed =	removed;
}

void Adap_Suspend(void * adapt, int type)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__INIT(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }   

    if (2 == type) //suspend
	{
		if ((!pAdap->adap_released) && (pAdap->net_dev != NULL))
		{
			if(pAdap->driver_state == DSP_DRIVER_WORK)
				Adap_SetPowerD3(pAdap);
		}
	}
	else if (1 == type)//freeze
	{
		if ((!pAdap->adap_released) && (pAdap->net_dev != NULL))
		{
			if(pAdap->driver_state == DSP_DRIVER_WORK)
				Adap_SetPowerD3(pAdap);
		}
	}

	DBG_WLAN__INIT(LEVEL_TRACE, "exit [%s]: event=%d driver_state = %d %d ",
			__FUNCTION__, type, pAdap->driver_state, pAdap->adap_released);
}


void Adap_Resume(void * adapt, int type)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__MAIN(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return ;
    }

    if (pAdap->pfrag == NULL)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "enter [%s]: but pAdap->pfrag = %p, exit (%d)\n",__FUNCTION__, pAdap->pfrag, type);   
		return ;
	}
	
	DBG_WLAN__ENTRY(LEVEL_TRACE, "enter [%s] %d  ",__FUNCTION__, type);  
		
	if (type == 2)			//PM_EVENT_SUSPEND
	{
		Adap_SetPowerD0(pAdap);
	}
	else if (type == 1)		//PM_EVENT_FREEZE
	{
		sc_sleep(2000);
		 if (Vcmd_8051fw_isWork(pAdap))
		{	
		 	DBG_WLAN__MAIN(LEVEL_TRACE,"%s: 8051 work, return ! \n", __FUNCTION__);
		}
		else
		{
			Adap_SetPowerD0(pAdap);
		}
	}

	DBG_WLAN__MAIN(LEVEL_TRACE, "exit [%s]: event = %d driver_state = %d %d ",__FUNCTION__, type,
		pAdap->driver_state,pAdap->adap_released);   
}

void Adap_Rest_Resume(void * adapt, int type)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__MAIN(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return;
    }

	if (type == 2)			//PM_EVENT_SUSPEND
	{
		if (pAdap->adap_released)
			return;

		Adap_SetPowerD0(pAdap);
	}
}

UINT8 Adap_Use_WPA(void * adapt)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap == NULL)
    {
        DBG_WLAN__MAIN(LEVEL_ERR, "[%s]:input para error,adapt is NULL",__FUNCTION__);	
        return FALSE;
    }
   if(	(	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK) 
	&&	!pAdap->wlan_attr.wpa_group_key_valid)
    {
	    return TRUE;
    }
    return FALSE;
}

void Adap_Set_Tx_Power_Default(void* adapt)
{
	PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;

	if(pAdap->wlan_attr.tx_power != DSP_TX_POWER_DEFAULT)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "[%s]: Enter",__FUNCTION__);	
	}

	pAdap->wlan_attr.tx_power = DSP_TX_POWER_DEFAULT;	

	return;
}

void Adap_Set_Tx_Power_Battery(void* adapt)
{
	PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
	
	if(pAdap->wlan_attr.tx_power != DSP_TX_POWER_BATTERY)
	{
		DBG_WLAN__MAIN(LEVEL_TRACE, "[%s]: Enter",__FUNCTION__);	
	}

	pAdap->wlan_attr.tx_power = DSP_TX_POWER_BATTERY;	

	return;
}

unsigned int Adap_Get_Txpower(void* adapt)
{
	PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
	
	return (25*(pAdap->wlan_attr.tx_power +1));
}