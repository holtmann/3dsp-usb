
#include "precomp.h"
#include "usbwlan_lib.h"

static char* TDSP_FILE_INDICATOR="IOCTL";

static inline
BOOLEAN		_ioctl_is_channel_allowed(void * adapt,    int channel)
{
	unsigned int i,end;
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
	if(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A)
	{
		i = pAdap->wlan_attr.channel_len_of_g;
		end = pAdap->wlan_attr.channel_num;
	}
	else
	{
		i = 0;
		end = pAdap->wlan_attr.channel_len_of_g;
	}


	for( ;i<end;i++)
	{
		if(((int)(pAdap->wlan_attr.channel_list[i])) == channel)
		{
			return TRUE;
		}
	}

	return FALSE;
}


unsigned int Adap_Get_DotType(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    return pAdap->wlan_attr.gdevice_info.dot11_type;
}

unsigned char Adap_Get_Mode(void *  adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
	// only supported in IBSS mode
	return pAdap->wlan_attr.macmode;
}

int Adap_Set_Defult_Channel(void *  adapt, int channel)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
	if(!_ioctl_is_channel_allowed(pAdap, channel))
	{
		DBG_WLAN__IOCTL(LEVEL_ERR, "_ioctl_iw_set_freq: channel=%u not allowed.\n",channel);
		return -1;
	}

	// the new freq would active when we start ibss mode next time;
	pAdap->wlan_attr.channel_default = (unsigned char)channel;
    return 0;
}

unsigned char Adap_Get_Channel(void * adapt)
{
    unsigned char channel;
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
	if (pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
	{
		channel = pAdap->wlan_attr.channel_default;
	}
	else
	{
		channel = pAdap->wlan_attr.current_channel;
	}
    return channel;
}


void Adap_Set_Mode(void * adapt,unsigned char mode)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    pAdap->wlan_attr.macmode = mode;
    Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_INFRASTRUCTURE_MODE,DSP_TASK_PRI_HIGH,NULL,0);

}


unsigned char Adap_Get_Channel_Num(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    return pAdap->wlan_attr.channel_len_of_g;
}


int Adap_Start_Scan(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    PMNG_STRU_T     pt_mng = pAdap->pmanagment;

	DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: =>\n", __FUNCTION__);
	
	if(!Adap_Driver_isWork(pAdap))
	{
		return -1;
	}
		
	

	pAdap->wlan_attr.set_disassoc_flag =0;

	//DBG_ENTER();
	
	if(pAdap->scan_result_new_flag !=0)
	{	
	//	DBG_TRACE();
		pAdap->scan_result_new_flag  = 0;
		DBG_WLAN__IOCTL(LEVEL_INFO, "scan_result_new_flag != 0\n");
		return -1;
	}
	
	//	DBG_TRACE();
	//justin:	080626	if create a task for scan during dsp_reset or hardware-reset, it maybe cause another dsp_reset coming 
	//				because scan pending timeout..		specialy in vista and combo working mode.
	if(	pAdap->driver_state == DSP_SYS_RESET
	||	pAdap->driver_state == DSP_HARDWARE_RESET)
	{
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_TRACE, "DSP_Reset in progress, don't scan\n");
		return -1;
	}


	//	DBG_TRACE();
	if(pAdap->link_ok == LINK_OK)	//justin ;	080410.	don't scan if has connected
	{
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_TRACE, "link_ok = 1 when scan request\n");
		return -1;
	}


	//	DBG_TRACE();
	if(	pt_mng->statuscontrol.curstatus == SCANNING
	||	pt_mng->statuscontrol.curstatus == JOINNING
	||	pt_mng->statuscontrol.curstatus == JOINOK
	||	pt_mng->statuscontrol.curstatus == AUTHOK
	||	pt_mng->statuscontrol.curstatus == ASSOCOK)
	{
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_CRIT, "pt_mng->statuscontrol.curstatus = %d when scan request\n",pt_mng->statuscontrol.curstatus);
		return -1;
	}
	
	//	DBG_TRACE();
//	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
//	{
//		DBG_WLAN__IOCTL(LEVEL_CRIT, "not oid scan in ibss mode\n");
//		return -1;
//	}


	if(Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN))
	{
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_TRACE, "_ioctl_iw_start_scan:	return because already in task!\n");
		return 0;
	}

	//	DBG_TRACE();
	
	if (pAdap->scanning_flag)
	{	
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_TRACE,"Scan_flag == 1 so return directly \n");
		return 0;
	}

	//	DBG_TRACE();
	if(	Task_CreateTask(
			(PDSP_TASK_T)pAdap->ppassive_task,
			DSP_TASK_EVENT_SCAN,
			DSP_TASK_PRI_HIGH,
			NULL,
			0) != STATUS_SUCCESS)
	{
	//	DBG_TRACE();
		DBG_WLAN__IOCTL(LEVEL_CRIT,"scan task create failed\n");
		return -1;
	}

	//	DBG_TRACE();
	//jsutin:	080612.	set flags must after create task suceed.
	//woody 080630
	//set scan_flag only task begin
	pAdap->is_oid_scan_begin_flag = TRUE;
	pAdap->scan_watch_value = 0;
	
	//DBG_EXIT();

	return 0;
}


void Adap_Get_Scan_Result(void * adapt, UINT32* count,PSCAN_RES* scan_res)
{
    int i;
    PMNG_DES_T		p_bss_list;
    PDSP_ADAPTER_T	pAdap;
    PMNG_STRU_T     pt_mng; 
    PSCAN_RES        result_arry;  
   //DBG_WLAN__IOCTL(LEVEL_TRACE,"%s: =>\n", __FUNCTION__);
    
    pAdap = (PDSP_ADAPTER_T)adapt;
    pt_mng = (PMNG_STRU_T)pAdap->pmanagment;
	pAdap->scan_result_new_flag = 0;
    result_arry = pt_mng->scan_result;
	if(pAdap->wlan_attr.set_disassoc_flag)
	{
		*count = 0;
	}

    if(pt_mng->oiddeslist.bssdesnum > WLAN_BSSDES_NUM)
		pt_mng->oiddeslist.bssdesnum = WLAN_BSSDES_NUM;
	
	*count = pt_mng->oiddeslist.bssdesnum;
    p_bss_list = pt_mng->oiddeslist.bssdesset;

    sc_memory_set(result_arry, 0, sizeof(SCAN_RES) * WLAN_BSSDES_NUM);
    for(i =0; i < (*count); ++i)
    {

        sc_memory_copy(&(result_arry[i].cap), &(p_bss_list[i].cap),sizeof(CAPABILITY_T));
        sc_memory_copy(result_arry[i].bssid,p_bss_list[i].bssid,WLAN_ETHADDR_LEN);
        
        //modified for hiding ssid
      
		if( pt_mng->hide_ssid_found
		    && (0 == sc_memory_cmp(p_bss_list[i].bssid,pt_mng->hide_bssid,WLAN_ETHADDR_LEN)))
		{
		    result_arry[i].ssid[0] = pt_mng->hide_ssid[0];
		    sc_memory_copy(&(result_arry[i].ssid[1]),&(pt_mng->hide_ssid[1]),result_arry[i].ssid[0]);
		}
		else
		{
		    result_arry[i].ssid[0] = p_bss_list[i].ssid[0];
		    sc_memory_copy(&(result_arry[i].ssid[1]),&(p_bss_list[i].ssid[1]),result_arry[i].ssid[0]);
        }

        result_arry[i].channel = p_bss_list[i].channel;
        result_arry[i].is80211g = (p_bss_list[i].networkType == Ndis802_11OFDM24) ? 1 :0;
        result_arry[i].suprate[0] = p_bss_list[i].suprate[0];
        sc_memory_copy(&result_arry[i].suprate[1],&p_bss_list[i].suprate[1],result_arry[i].suprate[0]);
        result_arry[i].beaconinterval = p_bss_list[i].beaconinterval;

        result_arry[i].rssi = GetRssi(p_bss_list[i].rssi);
        
        result_arry[i].offset_wpa  = p_bss_list[i].offset_wpa;
        result_arry[i].offset_wpa2 = p_bss_list[i].offset_wpa2;

        sc_memory_copy(&result_arry[i].IEs, &p_bss_list[i].IEs, NO_FIXED_IE_LEN);        
    }

    *scan_res = result_arry;
    //DBG_WLAN__IOCTL(LEVEL_TRACE,"%s: count is %d\n", __FUNCTION__,count);
  
}


void Adap_Get_BSSID(void * adapt,unsigned char * bssid)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
	if(	pAdap->wlan_attr.hasjoined == JOIN_NOJOINED
	||	pAdap->link_ok != LINK_OK
	||	pAdap->wlan_attr.set_disassoc_flag)
	{
		sc_memory_set(bssid, 0xff, ETH_LENGTH_OF_ADDRESS);
	}
	else
	{
		sc_memory_copy(bssid, pAdap->wlan_attr.bssid,ETH_LENGTH_OF_ADDRESS);
	}	

}


void Adap_Disassociate(void * adapt)
{
    DISASSOCIATE_COM_T disass_cmd;
	PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
	pAdap->wlan_attr.set_disassoc_flag =1;

	DBG_WLAN__IOCTL(LEVEL_TRACE,"^^^^_ioctl_disassociate ^^^^^\n ");	
	
	if(	pAdap->wlan_attr.hasjoined == JOIN_NOJOINED
	||	pAdap->link_ok != LINK_OK)
	{
		return;//NDIS_STATUS_ADAPTER_NOT_READY;//NDIS_STATUS_SUCCESS;
	}

	sc_memory_set(disass_cmd.addr,0xff,WLAN_ETHADDR_LEN);
	
	disass_cmd.reason = (UINT16)UNSPE_FAIL;
	disass_cmd.ind = 1;     //ind to os required
	
	if(STATUS_SUCCESS !=Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
							DSP_TASK_EVENT_OID_DISASSOC,
							DSP_TASK_PRI_HIGH,
							(unsigned char *)&disass_cmd,
							sizeof(DISASSOCIATE_COM_T)))
	{
		DBG_WLAN__IOCTL(LEVEL_ERR,"Add DSP_TASK_EVENT_OID_DISASSOC Task failure.\n ");	
	}

	sc_sleep(1000);

}


int Adap_Set_SSID(void * adapt, unsigned char* p_ssid, unsigned char ssid_len)
{
    unsigned int			find_ssid_result;
	BOOLEAN 				ssid_diff_flag;
	RESET_TYPE_T 			reset_type;
	DISASSOCIATE_COM_T		disass_cmd;
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
    PMNG_STRU_T     pt_mng = pAdap->pmanagment;
    if(	IS_MATCH_IN_SSID(SSID_EID,pAdap->wlan_attr.ssid_len,pAdap->wlan_attr.ssid,SSID_EID,ssid_len,p_ssid))
	{
		if(	pAdap->wlan_attr.hasjoined == JOIN_HASJOINED
		||	pt_mng->statuscontrol.curstatus == JOINNING
		||	pt_mng->statuscontrol.curstatus == JOINOK
		||	pt_mng->statuscontrol.curstatus == AUTHOK
		||	pt_mng->statuscontrol.curstatus == ASSOCOK)
		{
			return 0;
		}
	}
	
	DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: =>\n", __FUNCTION__);
	
	if(ssid_len == 0)
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "ssid_len==0.Disaccociate.\n");
		Adap_Disassociate(pAdap);
        return 0;
    }

	sc_memory_set(&pAdap->rssi_update, 0x00,sizeof(RSSI_UPDATE_T));

	if(REQUEST_CM_RETURN_GO_ON !=
		Adap_StartTKIPCounterMeasure(pAdap,REQUEST_CM_SET_SSID,0))
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE,"^^^^Oid_SetSsid: return by tkip counter measure ^^^^^\n ");	
		return  -1;
	}



	if(	pAdap->wlan_attr.hasjoined == JOIN_HASJOINED
	&&	(!pAdap->wlan_attr.set_disassoc_flag))
	{
		if(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
		{
			//copy bssid into para
			
			DBG_WLAN__IOCTL(LEVEL_TRACE, "Already connectted as BSS, Disaccociate. BSSID=%02x:%02x:%02x:%02x:%02x:%02x/n",
				pAdap->wlan_attr.bssid[0],pAdap->wlan_attr.bssid[1],pAdap->wlan_attr.bssid[2],
				pAdap->wlan_attr.bssid[3],pAdap->wlan_attr.bssid[4],pAdap->wlan_attr.bssid[5]);
			sc_memory_copy(disass_cmd.addr,(unsigned char *)pAdap->wlan_attr.bssid,WLAN_ETHADDR_LEN);
			disass_cmd.reason = (UINT16)UNSPE_REASON;

			disass_cmd.ind = 0;     //ind to os no required
			Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,
				DSP_TASK_EVENT_OID_DISASSOC,DSP_TASK_PRI_HIGH,(unsigned char *)&disass_cmd,sizeof(DISASSOCIATE_COM_T));	
		}
		else
		{
			DBG_WLAN__IOCTL(LEVEL_TRACE, "Already connectted as IBSS, Change Media State. /n");
			Adap_SetLink(pAdap,LINK_FALSE); 
			Adap_UpdateMediaState(pAdap,0);
		}
	}

	reset_type.sub_type = 0;	
	

#ifdef DEBUG_OPEN
	if (ssid_len < 32)
	{
		char		ssid_print[32 + 1];
		sc_memory_copy(ssid_print, p_ssid,ssid_len);
		ssid_print[ssid_len]='\0';
		DBG_WLAN__IOCTL(LEVEL_TRACE, "START JOIN. SSID=\'%s\'\n",ssid_print);
	}
	else
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "START JOIN. SSID len =%d\n",ssid_len);
	}
#endif

	//Begin -- added by Joe .2009 - 12 -14; Fix bug 111.
	//We should clear fragmentation before doing connect.
	Frag_ClearInfo(pAdap);
	//End --


	ssid_diff_flag = 	!(IS_MATCH_IN_SSID(SSID_EID,pAdap->wlan_attr.ssid_len,pAdap->wlan_attr.ssid,SSID_EID,ssid_len,p_ssid) 
				||	(pAdap->wlan_attr.ssid_len == 0) 
				|| 	(Adap_GetLinkStatus(pAdap) != LINK_OK));


	//woody fixe for debug 070424
//	pAdap->wlan_attr.gdevice_info.privacy_option = FALSE;
//	gdevice_info.privacy_option = FALSE;//TRUE;
	//							
	find_ssid_result = Mng_OidAssignSsid(p_ssid, ssid_len, TRUE,pAdap);	
	
	if(ASSIGN_SSID_NO_FOUND ==find_ssid_result)
	{
		find_ssid_result = Mng_OidAssignSsid(p_ssid, ssid_len, FALSE,pAdap);	
	}

	pt_mng->usedbssdes.ssid[0] = (unsigned char)wlan_min(ssid_len,WLAN_SSID_MAXLEN);
	sc_memory_copy(&pt_mng->usedbssdes.ssid[1],p_ssid,pt_mng->usedbssdes.ssid[0]);

	//added for hiding ssid
	pt_mng->hide_ssid[0] = (unsigned char)wlan_min(ssid_len,WLAN_SSID_MAXLEN);
	sc_memory_copy(&pt_mng->hide_ssid[1],p_ssid,pt_mng->hide_ssid[0]);
	
	//ssid is differ from last ssid
	reset_type.type = RESET_TYPE_DISCONNECT;
	reset_type.sub_type = 0;


	pAdap->ap_in_list = (find_ssid_result != ASSIGN_SSID_NO_FOUND);

	if (find_ssid_result != ASSIGN_SSID_MISMACTH_PRIVACY)
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "Found SSID not Match.\n");
		
		reset_type.sub_type |= (find_ssid_result == ASSIGN_SSID_FOUND) ? 
			RESET_SUB_TYPE_BEGIN_JOIN : RESET_SUB_TYPE_BEGIN_SCAN;
		
		reset_type.sub_type |= (ssid_diff_flag ) ? 
			RESET_SUB_TYPE_NOTIFY_UPLEVEL : 0;		
	}


	//found assigned ssid 
	if (find_ssid_result == ASSIGN_SSID_FOUND)
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "Found SSID.\n");
		
		reset_type.type = (ssid_diff_flag) ? RESET_TYPE_KNOWN_INFO_DIFF_SSID : RESET_TYPE_KNOWN_INFO; 
	}

	if (find_ssid_result == ASSIGN_SSID_NO_FOUND)
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "Can't find SSID.\n");
		reset_type.type = (ssid_diff_flag) ? RESET_TYPE_KNOWN_SSID_SCAN_DIFF_SSID : RESET_TYPE_KNOWN_SSID_SCAN; 
	}
    else if(find_ssid_result == ASSIGN_SSID_MISMACTH_PRIVACY)
    {
        DBG_WLAN__IOCTL(LEVEL_TRACE, "SSID mismatch privay.\n");
    }
	pAdap->wlan_attr.set_disassoc_flag =0;
		
	if(pAdap->wlan_attr.macmode != WLAN_MACMODE_ESS_STA)
	{
		reset_type.type = RESET_TYPE_KNOWN_SSID_SCAN_DIFF_SSID;
	
		DBG_WLAN__IOCTL(LEVEL_TRACE, "OIDOID find_ssid_result = %d, ssid_diff_flag = %d, reset_type.type = %d, reset_type.sub_type = %d,mac_mode = %x\n",
		find_ssid_result,ssid_diff_flag,reset_type.type,reset_type.sub_type,pAdap->wlan_attr.macmode);	
	}

#ifdef ROAMING_SUPPORT
	pAdap->reconnect_status = NO_RECONNECT;
#endif



	Mng_BreakIfScanning(pAdap);
	
	while(Task_RemoveExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_SCAN))
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "REMOVE one Scan Task!\n");
	}
	
	DBG_WLAN__IOCTL(LEVEL_TRACE, "SET reset_type=%u, sub_type=%u.\n", reset_type.type, reset_type.sub_type);

	if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task, DSP_TASK_EVENT_OID_SET_SSID))//Justin:080613.	No need to create is already exist.
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "Add DSP_TASK_EVENT_OID_SET_SSID to Task list!\n");
		//Task_CreateTaskForce((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID,DSP_TASK_PRI_NORMAL,&tmptype,1);
		Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_SSID,DSP_TASK_PRI_HIGH,(unsigned char *)&reset_type,sizeof(RESET_TYPE_T));
	}
	else
	{
		DBG_WLAN__IOCTL(LEVEL_TRACE, "DSP_TASK_EVENT_OID_SET_SSID already in Task list!\n");
	}	
	
	sc_sleep(1000);

    return 0;
}



void Adap_Get_SSID(void * adapt, unsigned char * p_ssid, unsigned char* ssid_len)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    PMNG_STRU_T			pt_mng = pAdap->pmanagment;
	
    if(pAdap->wlan_attr.macmode == WLAN_MACMODE_IBSS_STA)
	{		
		if(	pAdap->wlan_attr.hasjoined == JOIN_HASJOINED
		||	pt_mng->statuscontrol.curstatus == JOINOK
		||	pt_mng->statuscontrol.curstatus == JOINNING)
		{
			*ssid_len = pAdap->wlan_attr.ssid_len;

			sc_memory_copy(p_ssid, pAdap->wlan_attr.ssid, pAdap->wlan_attr.ssid_len);
		}
		else
		{
			*ssid_len = 0;
		}
	}
	else
	{
		if(	pAdap->wlan_attr.hasjoined == JOIN_NOJOINED
		||	pAdap->link_ok != LINK_OK
		||	pAdap->wlan_attr.set_disassoc_flag)//Justin: 0622, fix hct. ..Driver must return 0 for SSIDLength when not associated 
		{
			*ssid_len= 0;
		}
		else 
		{
			*ssid_len = pAdap->wlan_attr.ssid_len;
			sc_memory_copy(p_ssid, pAdap->wlan_attr.ssid, pAdap->wlan_attr.ssid_len);
		}
	}

}


void Adap_Set_Rts(void * adapt,int value)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
	if(value == -1)
	{
		pAdap->wlan_attr.rts_threshold_enable = FALSE;
		pAdap->wlan_attr.rts_threshold = DSP_RTSTHRES_MAX;
	}
	else
	{			
    	pAdap->wlan_attr.rts_threshold_enable = TRUE;
		pAdap->wlan_attr.rts_threshold = value;
	}

	Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_SET_RTS, DSP_TASK_PRI_NORMAL,NULL,0);
}

void Adap_Get_Rts(void * adapt, PRTS_DATA data)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    data->rts_disable =  pAdap->wlan_attr.rts_threshold_enable ? 0 : 1;
    data->rts_value   = pAdap->wlan_attr.rts_threshold;
}


void Adap_Set_Frag(void * adapt,int frag)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
    if(frag == -1)
    {
        pAdap->wlan_attr.frag_threshold_mode = 0;	// disable
	    pAdap->wlan_attr.frag_threshold = DSP_FRAGTHRES_MAX;
    }
    else
    {
        pAdap->wlan_attr.frag_threshold_mode = 2; // user defined
		pAdap->wlan_attr.frag_threshold = (UINT16)(frag & (~0x1)); // even numbers only 
    }
}


void Adap_Get_Frag(void * adapt, PFRAG_DATA data)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
  
    data->frag_disable = pAdap->wlan_attr.frag_threshold_mode == 0 ? 1 : 0;

	data->frag_value   = pAdap->wlan_attr.frag_threshold;
	

}


void Adap_Set_Rate(void * adapt, int rate, unsigned char fixed)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    pAdap->wlan_attr.rate = (unsigned char)(rate/500000);   
	pAdap->wlan_attr.fallback_rate_to_use = fixed ? FALLBACK_RATE_NOUSE: FALLBACK_RATE_USE ;
	Adap_SetRateIndex(pAdap);

}


void Adap_Get_Rate(void * adapt, PRATE_DATA data)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    data->rate_value = pAdap->wlan_attr.rate * 500000;

	data->fixed = pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE? 0 : 1;
}

unsigned char  Adap_Get_Rssi(void * adapt)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    
    return GetRssi(pAdap->wlan_attr.rssi);
}

int Adap_Set_Auth(void * adpat, unsigned char auth_mode, unsigned char value)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adpat;
	PMNG_STRU_T     		pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   			pt_mattr = &pAdap->wlan_attr;
	BOOLEAN					need_change=FALSE;
    
    if(	pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
	{
		return 0;
	}

	if(	pt_mng->statuscontrol.curstatus == JOINNING
	||	pt_mng->statuscontrol.curstatus == JOINOK
	||	pt_mng->statuscontrol.curstatus == AUTHOK
	||	pt_mng->statuscontrol.curstatus == ASSOCOK)
	{
		return -1;
	}
#if 0		
    switch(auth_mode)
    {
        case AUTH_WPA_VERSION:
             pAdap->wlan_attr.auth_mode_data.wpa_version = value;
            break;
        case AUTH_CIPHER_PAIRWISE:
            pAdap->wlan_attr.auth_mode_data.parwise_cipher= value;
            break;
        case AUTH_CIPHER_GROUP:
            pAdap->wlan_attr.auth_mode_data.group_cipher = value;
            break;
        case AUTH_KEY_MGMT:
            pAdap->wlan_attr.auth_mode_data.key_mng = value;
            break;

        case AUTH_80211_AUTH_ALG:
            pAdap->wlan_attr.auth_mode_data.auth_alg = value;
            break;

       default:
        break;
    }

    pAdap->wlan_attr.key_mng = pAdap->wlan_attr.auth_mode_data.key_mng;
    
    if(pAdap->wlan_attr.auth_mode_data.key_mng == KEY_MNG_NONE)
    {
        if( pAdap->wlan_attr.auth_mode_data.auth_alg == AUTH_ALG_SHARED_KEY)
        {    

            if( (   pAdap->wlan_attr.auth_mode_data.parwise_cipher == WEP_MODE_WEP)
            &&  (   pAdap->wlan_attr.auth_mode_data.group_cipher == WEP_MODE_WEP))

            {
                pAdap->wlan_attr.auth_alg = AUTH_ALG_SHARED_KEY;
                pAdap->wlan_attr.auth_mode = AUTH_MODE_SHARED_KEY;
                pAdap->wlan_attr.wep_mode = WEP_MODE_WEP;
                pAdap->wlan_attr.group_cipher = WEP_MODE_WEP;
    		    pt_mattr->gdevice_info.privacy_option = TRUE;
            }
        }
        else
        {
            pAdap->wlan_attr.auth_alg = AUTH_ALG_OPEN;
            pAdap->wlan_attr.auth_mode = AUTH_MODE_OPEN;
    		pt_mattr->gdevice_info.privacy_option = FALSE;
        }
    }
    else if(    pAdap->wlan_attr.auth_mode_data.key_mng == KEY_MNG_PSK
            ||   pAdap->wlan_attr.auth_mode_data.key_mng == KEY_MNG_1X)
    {
        if(pAdap->wlan_attr.auth_mode_data.auth_alg  == AUTH_ALG_OPEN)
        {
            if( (    pAdap->wlan_attr.auth_mode_data.parwise_cipher == WEP_MODE_TKIP
                ||   pAdap->wlan_attr.auth_mode_data.parwise_cipher == WEP_MODE_AES)
            &&  (   pAdap->wlan_attr.auth_mode_data.group_cipher == WEP_MODE_TKIP
                ||  pAdap->wlan_attr.auth_mode_data.group_cipher == WEP_MODE_AES)) 
            {

                pAdap->wlan_attr.auth_alg = AUTH_ALG_OPEN;
                if(pAdap->wlan_attr.auth_mode_data.wpa_version  == WPA_VERSION_WPA)
                {
        			pAdap->wlan_attr.use_wpa2 = FALSE;
                    if(pAdap->wlan_attr.auth_mode_data.key_mng == KEY_MNG_PSK)
                    {
                        pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA_PSK;
                    }
                    else
                    {
                        pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA;
                    }
                }
                else
                {        
        			pAdap->wlan_attr.use_wpa2 = TRUE;
                    if(pAdap->wlan_attr.auth_mode_data.key_mng  == KEY_MNG_PSK)
                    {
                        pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA2_PSK;
                    }
                    else
                    {
                        pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA2;
                    }
                }

                if(pAdap->wlan_attr.auth_mode_data.parwise_cipher == WEP_MODE_TKIP)
                {
        			pAdap->wlan_attr.wep_mode = WEP_MODE_TKIP;
                }
                else
                {
        			pAdap->wlan_attr.wep_mode = WEP_MODE_AES;
                }

                if( pAdap->wlan_attr.auth_mode_data.group_cipher == WEP_MODE_TKIP)
                {
        			pAdap->wlan_attr.group_cipher = WEP_MODE_TKIP;
                }
                else
                {
        			pAdap->wlan_attr.group_cipher = WEP_MODE_AES;
                }
         
        		pt_mattr->gdevice_info.privacy_option = TRUE;
            }
        }
    }
#endif
      switch(auth_mode)
    {
        case AUTH_WPA_VERSION:
             pAdap->wlan_attr.use_wpa2 = ((WPA_VERSION_WPA2 == value) ? TRUE: FALSE); 
             need_change = TRUE;
            
            break;
        case AUTH_CIPHER_PAIRWISE:
            if(value == WEP_MODE_NONE)
            {
                pt_mattr->gdevice_info.privacy_option = FALSE;
            }
            else if((value == WEP_MODE_WEP)
            || (value == WEP_MODE_TKIP)
            || (value == WEP_MODE_AES))
            {
                pAdap->wlan_attr.wep_mode = value;
            }
            break;
        case AUTH_CIPHER_GROUP:
            if(value == WEP_MODE_NONE)
            {
                pt_mattr->gdevice_info.privacy_option = FALSE;
            }
            else if((value == WEP_MODE_WEP)
                || (value == WEP_MODE_TKIP)
                || (value == WEP_MODE_AES))
            {
                pAdap->wlan_attr.group_cipher = value;
            }
            break;
        case AUTH_KEY_MGMT:
            pAdap->wlan_attr.key_mng = value;
            need_change = TRUE;
            break;

        case AUTH_80211_AUTH_ALG:
            pAdap->wlan_attr.auth_alg = value;
            need_change = TRUE;
            break;

       default:
        break;
    }
    if(need_change)
	{
		if(	pAdap->wlan_attr.key_mng != KEY_MNG_NONE
		&&	pAdap->wlan_attr.auth_alg == AUTH_ALG_OPEN)
		{
			if(	pAdap->wlan_attr.use_wpa2
			&&	pAdap->wlan_attr.key_mng == KEY_MNG_PSK)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA2_PSK;
			}
			else if(	pAdap->wlan_attr.use_wpa2
				&&	pAdap->wlan_attr.key_mng == KEY_MNG_1X)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA2;
			}
			else if(	!pAdap->wlan_attr.use_wpa2 
				&&	pAdap->wlan_attr.key_mng == KEY_MNG_PSK)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA_PSK;
			}
			else if(	!pAdap->wlan_attr.use_wpa2 
				&&	pAdap->wlan_attr.key_mng == KEY_MNG_1X)
			{
				pAdap->wlan_attr.auth_mode = AUTH_MODE_WPA;
			}	
			else
			{
				DBG_WLAN__IOCTL(LEVEL_ERR, "Unsupported auth mode!\n");				
			}
		}
		else
		{			
			pAdap->wlan_attr.auth_mode = pAdap->wlan_attr.auth_alg;
		}

		if(	pAdap->wlan_attr.auth_mode != AUTH_ALG_OPEN)
		{
			pt_mattr->gdevice_info.privacy_option = TRUE;
		}
				
		DBG_WLAN__IOCTL(LEVEL_TRACE, "Change Auth alg to %u, auth mode to %u.\n",
				pAdap->wlan_attr.auth_alg, pAdap->wlan_attr.auth_mode );	
	}
	DBG_WLAN__IOCTL(LEVEL_TRACE, "Change  key mng to %u,Auth alg to %u, auth mode to %u, wep mode to %u,group cipher to %u.\n",
			pAdap->wlan_attr.key_mng,
			pAdap->wlan_attr.auth_alg, 
			pAdap->wlan_attr.auth_mode,
			pAdap->wlan_attr.wep_mode,
			pAdap->wlan_attr.group_cipher);	
	
    return 0;
}



unsigned char Adap_Get_Auth_Alg(void * adapt)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    return pAdap->wlan_attr.auth_alg; 
}


unsigned char Adap_Get_Key_Mng(void * adapt)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    return pAdap->wlan_attr.key_mng;
}

void Adap_Disalbe_Encode(void * adapt, unsigned char index)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    NDIS_802_11_KEY_INDEX	KeyIndex = (index == 0) ? pAdap->wlan_attr.default_key_index :index;
	Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_REMOVE_WEP,DSP_TASK_PRI_HIGH,(unsigned char*)&KeyIndex,sizeof(NDIS_802_11_KEY_INDEX));
	
}

void Adap_Set_Encode(void * adapt, unsigned char index, unsigned char* key, unsigned char len)
{
	unsigned char					key_buffer[32];
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
    PNDIS_802_11_WEP 		p_802_wep = (PNDIS_802_11_WEP)key_buffer;
	if(index == 0)
	{
		index = pAdap->wlan_attr.default_key_index;	
	}
    else if(index < 1 || index > 4)
    {
        return;    
    }
    if(len != KEY_BIT40_LEN
	  && len != KEY_BIT104_LEN)
	{
        return;
    }
		
	pAdap->wlan_attr.gdevice_info.privacy_option = TRUE;		
	pAdap->wlan_attr.wep_mode = WEP_MODE_WEP;
	
	p_802_wep->KeyLength =len;
	p_802_wep->KeyIndex = index;
    sc_memory_copy(p_802_wep->KeyMaterial,key,len);
	Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_ADD_WEP,DSP_TASK_PRI_HIGH,(unsigned char *)p_802_wep,32);

}

void Adap_Get_Encode(void * adapt, unsigned char index,PWEP_KEY wep)
{
     PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
   
    //check th cipher_type first
	if(pAdap->wlan_attr.wep_mode != WEP_MODE_WEP)
	{
        wep->len = 0;
        return;
    }
    if (index == 0)
    {
        index = pAdap->wlan_attr.default_key_index;
    }
	
	if (	(!pAdap->wlan_attr.key_map[index].WEP_on)
	||	(pAdap->wlan_attr.key_map[index].key_len_in_bytes == 0 ))
	{
	    wep->len = 0;
		return;
	}
		
	wep->len = pAdap->wlan_attr.key_map[index].key_len_in_bytes;
	sc_memory_copy(wep->key, pAdap->wlan_attr.key_map[index].key, wep->len);

}


BOOLEAN _is_wpa_auth_mod(PDSP_ADAPTER_T pAdap)
{
    if(	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA_PSK
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2
		||	pAdap->wlan_attr.auth_mode == AUTH_MODE_WPA2_PSK)
    {
        return TRUE;
    }

    return FALSE;
}

int Adap_Remove_Key_Ext(void * adapt, unsigned char index,unsigned char group)

{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
	PMNG_STRU_T     				pt_mng = pAdap->pmanagment;
	NDIS_802_11_REMOVE_KEY	remove_key ;
    TDSP_STATUS status;
	DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: IW_ENCODE_DISABLED, index = %d\n", __FUNCTION__, index);

    if(index >= 4)
    {
        DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: input index error\n", __FUNCTION__);
        return -1;
    }
   
    if (index == 0)
    {
        index = pAdap->wlan_attr.default_key_index;
    }

    	
	if((pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
	    && (!_is_wpa_auth_mod(pAdap)))
	{
        return	0;
    }
    
	if(	((pt_mng->statuscontrol.curstatus == JOINNING)
	    ||	(pt_mng->statuscontrol.curstatus == JOINOK)
	    ||	(pt_mng->statuscontrol.curstatus == AUTHOK)
	    ||	(pt_mng->statuscontrol.curstatus == ASSOCOK))
        &&(!_is_wpa_auth_mod(pAdap)))
    {
		return	-1;
		
	}

		remove_key.KeyIndex = index;
		remove_key.is_group_key = group ? TRUE:FALSE;
		status = Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_REMOVE_KEY,DSP_TASK_PRI_HIGH,(unsigned char *)&remove_key,64);

		if(status!=STATUS_SUCCESS)
		{
			DBG_WLAN__IOCTL(LEVEL_ERR, "%s: No free task block.\n", __FUNCTION__);
			return -1;
		}
		
	sc_event_reset(&pAdap->set_key_event);
	status = sc_event_wait(&pAdap->set_key_event, sc_ms_to_ticks(1500));

	if(status == 0)
	{
		DBG_WLAN__IOCTL(LEVEL_ERR, "%s: Wait Event Timeout.\n", __FUNCTION__);
		return 0;
	}
		return 0;
}

int Adap_Add_Key_Ext(void * adapt, unsigned char index, PADD_KEY add_key)
{
    PDSP_ADAPTER_T	pAdap = (PDSP_ADAPTER_T)adapt;
	PMNG_STRU_T     			pt_mng = pAdap->pmanagment;
	TDSP_STATUS                 status;
    if(index >= 4)
    {
        DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: input index error\n", __FUNCTION__);
        return -1;
    }
   
    if (index == 0)
    {
        index = pAdap->wlan_attr.default_key_index;
    }

    	
	if((pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
	    && (!_is_wpa_auth_mod(pAdap)))
	{
	    DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: wlan has joined is %d,wpa_auth_mod is %d\n",
                        __FUNCTION__,
                        pAdap->wlan_attr.hasjoined,
                        pAdap->wlan_attr.auth_mode);
        return	0;
    }
    
	if(	((pt_mng->statuscontrol.curstatus == JOINNING)
	    ||	(pt_mng->statuscontrol.curstatus == JOINOK)
	    ||	(pt_mng->statuscontrol.curstatus == AUTHOK)
	    ||	(pt_mng->statuscontrol.curstatus == ASSOCOK))
        &&(!_is_wpa_auth_mod(pAdap)))
	{
	    DBG_WLAN__IOCTL(LEVEL_ERR, "%s: pt_mng->statuscontrol.curstatus is %d,wpa_auth_mod is %d\n",
                __FUNCTION__,
               pt_mng->statuscontrol.curstatus,
               pAdap->wlan_attr.auth_mode);
		return	-1;
		
	}

    if(STATUS_SUCCESS != Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_OID_ADD_KEY,DSP_TASK_PRI_HIGH,(unsigned char *)add_key,64))
    {
	    DBG_WLAN__IOCTL(LEVEL_ERR, "%s: No free task block.\n", __FUNCTION__);
   	    return -1;
    }
	
	Adap_Set_Driver_State(pAdap,DSP_STOP_TX);
	Tx_Stop(pAdap);

	sc_event_reset(&pAdap->set_key_event);
	status = sc_event_wait(&pAdap->set_key_event, sc_ms_to_ticks(1500));

	if(status == 0)
	{
		DBG_WLAN__IOCTL(LEVEL_ERR, "%s: Wait Event Timeout.\n", __FUNCTION__);
		Adap_Set_Driver_State(pAdap,DSP_DRIVER_WORK);
		Tx_Restart(pAdap);
		return 0;
	}
	return 0;
}

unsigned char Adap_Is_Encode_Enabled(void * adpat)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adpat;
	PDSP_ATTR_T   			pt_mattr = &pAdap->wlan_attr;

    return pt_mattr->gdevice_info.privacy_option == TRUE ? 1:0; 
}


unsigned char Adap_Get_Encode_Mode(void * adapt, unsigned char group)
{
    unsigned char wep_mode;
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;

    if(group == 1)
    {
        wep_mode = pAdap->wlan_attr.group_cipher;
    }
    else
    {
         wep_mode =  pAdap->wlan_attr.wep_mode;
    }
    return wep_mode;
}


int Adap_Get_Encode_Key(void * adapt, unsigned char index,unsigned char* key)
{	
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    wpakey_info_t  			*keyspace;
    unsigned char key_len;

    if(index!= 0)
	{
		if(index < 1 || index > 4)
		{
			DBG_WLAN__IOCTL(LEVEL_TRACE, "%s: invalid key index: %d\n", __FUNCTION__, index);
			return -1;
		} 
		else
		{
			index--;	//key index: 0~3
		}
	}
	else
	{
		index = pAdap->wlan_attr.default_key_index;
	}
    
    keyspace = (wpakey_info_t *)&pAdap->wlan_attr.key_map[index];
   
	
	key_len = keyspace->key_len_in_bytes;
	
	sc_memory_copy(key, keyspace->key, key_len);
    return key_len;

}


void Adap_Set_Domain(void * adapt, unsigned char domain)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
  
 	if (pAdap->wlan_attr.regoin != domain)
	{
		pAdap->wlan_attr.regoin = domain;

		//get channel list accroding domain
		Adap_GetRegionDomain(pAdap, 1);
	}

	DBG_WLAN__IOCTL(LEVEL_INFO, "number of supported channels is %d\n",
					pAdap->wlan_attr.channel_num);
}

unsigned int Adap_Get_Domain(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
	return pAdap->wlan_attr.regoin;
}


unsigned int Adap_Get_Dsp_Version(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    return pAdap->DSP_FW_version;
}


unsigned char Adap_Has_Joined(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    return (pAdap->wlan_attr.hasjoined == JOIN_HASJOINED ? 1 : 0);
}


unsigned char Adap_Has_Connected(void * adapt)
{
    PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
    	if (	pAdap->wlan_attr.hasjoined == JOIN_NOJOINED
	||	pAdap->link_ok != LINK_OK
	||	pAdap->wlan_attr.set_disassoc_flag) 
    {
        return 0;
    }
    return 1;
}

unsigned char Adap_Is_Privacy_On(void * adapt)
{
     PDSP_ADAPTER_T			pAdap = (PDSP_ADAPTER_T)adapt;
     return pAdap->wlan_attr.gdevice_info.privacy_option ? TRUE : FALSE;
}


