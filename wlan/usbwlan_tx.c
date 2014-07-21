/************************************************************************
(c) 2003-04, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:

3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
**************************************************************************
$RCSfile: usbwlan_tx.c,v $ 
$Revision: 1.14 $ 
$Date: 2010/08/16 01:53:24 $
**************************************************************************/
/*
 * description:
 *	Entry point for the kernel.
 *	Initializes kernel and user modules.
 *	Calls message queue scheduler.
 *	  
 */

#include "precomp.h"
static char* TDSP_FILE_INDICATOR="TRANS";
/*
#include <ndis.h>
#include <initguid.h>
#include "usbwlan_Task.h"
#include "usbwlan_Sw.h"
#include "usbwlan_mng.h"
#include "usbwlan_proto.h"
#include "usbwlan_frag.h"
#include "usbwlan_Pr.h"
//#include "usbwlan_phy.h"
//#include "usbwlan_atpa.h"
#include "usbwlan_Oid.h"
#include "usbwlan_dbg.h"
#include "usbwlan_usbdev.h"
#include "usbwlan_pktlist.h"
#include "usbwlan_tx.h"
#include "usbwlan_defs.h"
*/
#define TIME_STARVE_FRAME_RETRY_INTERVAL   200
// Put these in a better palce later
unsigned char rfc1042_header[DOT11_SNAP_HDR_LEN_MINUS_TYPE] =
{ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
unsigned char bridge_tunnel_header[DOT11_SNAP_HDR_LEN_MINUS_TYPE] =
{ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };
/*****************************************************************************************
 *		Micros used in mic-calculation
******************************************************************************************/
UINT32 rotl(UINT32 val, UINT32 bits)
{
	return (val << bits) | (val >> (32 - bits));
}


UINT32 rotr(UINT32 val, UINT32 bits)
{
	return (val >> bits) | (val << (32 - bits));
}

UINT32 xswap(UINT32 val)
{
	return ((val & 0x00ff00ff) << 8) | ((val & 0xff00ff00) >> 8);
}

UINT32 get_le32(const PUINT8 p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void put_le32(PUINT8 p, UINT32 v)
{
	p[0] = (UINT8)v;
	p[1] = (UINT8)(v >> 8);
	p[2] = (UINT8)(v >> 16);
	p[3] = (UINT8)(v >> 24);
}

#define michael_block(l, r)	\
do {				\
	r ^= rotl(l, 17);	\
	l += r;			\
	r ^= xswap(l);		\
	l += r;			\
	r ^= rotl(l, 3);	\
	l += r;			\
	r ^= rotr(l, 2);	\
	l += r;			\
} while (0)


/*****************************************************************************************/
UINT16 get_current_seqnum(PDSP_ADAPTER_T pAdap)
{
	UINT16 tmp;

	ASSERT(pAdap);

	tmp = pAdap->wlan_attr.seq_num++;

	if(pAdap->wlan_attr.seq_num >= 0x0fff)
		pAdap->wlan_attr.seq_num = 0;

	return tmp;
}

//Jakio: this code was copied from AP project, meanwhile, referenced  project
UINT16 _durn(PDSP_ADAPTER_T pAdap, UINT8 short_preamble,
	UINT16  frm_len,  UINT8	rate)
{
	UINT32   length;
	UINT16 len;
	UINT16 duration;
	UINT16 preamble_len;
	UINT16 plcp_hdr_len;
	ASSERT(pAdap);

	length   = ((((frm_len << 3)) * 2) / rate);
	len	  = (UINT16)length;
	duration = 0;

	if (short_preamble) 
	{
		preamble_len = PREAMBLE_LENGTH_SHORT;
		plcp_hdr_len = PLCP_HEADER_TIME_SHORT;
	}
	else 
	{
		preamble_len = PREAMBLE_LENGTH_LONG;
		plcp_hdr_len = PLCP_HEADER_TIME_LONG;
	}

	if (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_B) 
	{
		if(rate == 0xb || rate == 0x16) // 5.5 or 11Mbps
		{
			duration = preamble_len + plcp_hdr_len + len + 1;
		}
		else if(rate == 0x4) // 2Mbps
		{
		    duration = preamble_len + plcp_hdr_len + len;
		}
		else // 1Mbps can only use long preamble
		{
			duration = PREAMBLE_LENGTH_LONG + PLCP_HEADER_TIME_LONG + len;	
		}	
	}
	else if (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A)
	{	
		duration = (20 + ((((UINT16)((22 + (frm_len << 3)) / (rate << 1))) + 1) << 2));
	}
	else if (pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_G) 
	{ 
		if ( rate == 0x0b || rate == 0x16) 
		{
			duration = preamble_len + plcp_hdr_len + len + 1;
		}
		else if(rate == 0x4) // 2Mbps
		{
		      duration = preamble_len + plcp_hdr_len + len;
		}
		else if(rate == 0x02) // 1Mbps can only use long preamble // 1Mbps can only use long preamble
		{
			duration = PREAMBLE_LENGTH_LONG + PLCP_HEADER_TIME_LONG + len;	
		}	
		else 
		{	
			//Jakio:Can this happen??? Just do as 11b_mode
			//duration = preamble_len + plcp_hdr_len + len;
			//0531 woody
			//duration = (20 + ((((UINT16)((22 + (frm_len << 3)) / (rate << 1))) + 1) << 2));
			duration = 6+ (20 + ((((UINT16)((22 + (frm_len << 3)) / (rate << 1))) + 1) << 2));
		}
	}
	return duration;
}



UINT16 _duration_ack(PDSP_ADAPTER_T pAdap, UINT8 short_preamble, UINT8 rate)
{

	UINT16 return_val;

	ASSERT(pAdap);

	
	if(rate == 0xC || rate == 0x12) {   //6 used for 6,9 
		return_val = pAdap->wlan_attr.gdevice_info.sifs+ 
			_durn(pAdap, short_preamble, WLAN_ACK_LEN, 0xC);
	}
	else if(rate == 0x18 || rate == 0x24) { //12 used for 12,18
		return_val = pAdap->wlan_attr.gdevice_info.sifs + 
			_durn(pAdap, short_preamble, WLAN_ACK_LEN, 0x18);
	}
	else if(rate == 0x30 || rate == 0x48 || rate == 0x60 || rate == 0x6C) {
		//24 used for 24,36,48,54
		return_val = pAdap->wlan_attr.gdevice_info.sifs + 
			_durn(pAdap, short_preamble, WLAN_ACK_LEN, 0x30);
	} 
	else { //DSSS rates
		return_val = pAdap->wlan_attr.gdevice_info.sifs + 
			_durn(pAdap, short_preamble, WLAN_ACK_LEN, 
			(rate & 0x7F));
	}

	return return_val;
}

#if 0
//only failure state returned that indicate retry is needed
TDSP_STATUS Adap_send_starve(PDSP_ADAPTER_T pAdap,UINT32 num)
{
	TDSP_STATUS  status = STATUS_FAILURE;
	if(num < SCAN_SUB_STATE_STARVE_MAX_NUM)
	{
		tdsp_event_reset(&pAdap->starve_event);
		status = Adap_Transmit_Starve_frame(pAdap);
		//not get resource,Keep sending starve
		if(status == STATUS_SUCCESS)
		{
		//get resource for send starve frame		
			//waitevent();
			//event set by completion
			if (!tdsp_event_wait(&pAdap->starve_event,200)) 
			{
				status = TDSP_STATUS_NOT_ACCEPTED;
				DBGSTR_TX(("timeout to wait starve frame return \n"));
			}
			else
			{
				status = STATUS_SUCCESS;
				DBGSTR_TX(("starve frame return ok \n"));
			}
		}
	}
	//num exceed
	else
	{
		status = TDSP_STATUS_RESOURCES;
		DBGSTR_TX(("retry limit to send starve frame \n"));
	}
	return status;		
}
#endif


void _tx_print_msdu(PTX_MSDU_DATA msdu)
{
	DBG_WLAN__TX(LEVEL_TRACE,"@@@@@@@@@@@@@@@tx print msdu buffer begin@@@@@@@@@@@@@\n");
	DBG_PRINT_BUFF("MSDU HEAD:", msdu->head_buff, msdu->head_len);
	//DBG_PRINT_BUFF("MSDU PAYLOAD:", msdu->payload_buff, msdu->payload_len);
    DBG_WLAN__TX(LEVEL_TRACE,"mdsu frm_type is %d,msg type is %d\n",
                msdu->frm_type,
                msdu->msg_type);
    DBG_WLAN__TX(LEVEL_TRACE,"@@@@@@@@@@@@@@@tx print msdu buffer end@@@@@@@@@@@@@@@\n");
	
}

#if 1
void _tx_buffer_msdu(PTX_CONTEXT tx_context,PTX_MSDU_DATA tx_msdu)
{

    PTX_MSDU_POOL     tx_buffer_pool = &tx_context->tx_buffer_pool;
    PTX_MSDU_DATA   new_msdu;
    sc_spin_lock(&tx_buffer_pool->pool_lock);
	new_msdu = (PTX_MSDU_DATA)QueuePopHead(&tx_buffer_pool->msdu_free_list);
    new_msdu->frm_type = tx_msdu->frm_type;
    new_msdu->head_len = tx_msdu->head_len;
    new_msdu->payload_len = tx_msdu->payload_len;
    new_msdu->seqnum = tx_msdu->seqnum;
    sc_memory_set(new_msdu->head_buff,0,tx_buffer_pool->head_size);
    sc_memory_set(new_msdu->payload_buff,0,tx_buffer_pool->payload_size);
    sc_memory_copy(new_msdu->head_buff, tx_msdu->head_buff, tx_msdu->head_len);
    sc_memory_copy(new_msdu->payload_buff, tx_msdu->payload_buff, tx_msdu->payload_len);
    QueuePutTail(&tx_buffer_pool->msdu_free_list, &(new_msdu->link));
	sc_spin_unlock(&tx_buffer_pool->pool_lock);
}

void _tx_print_buffer_msdu(PTX_CONTEXT tx_context)
{
    int i;
    PTX_MSDU_DATA print_msdu = NULL;
    PTX_MSDU_POOL     tx_buffer_pool = &tx_context->tx_buffer_pool;
 
    DBG_WLAN__TX(LEVEL_TRACE,"**********buffer tx_msdu begin**********\n");

    for(i = 0; i < 4; i++ )
    {
        print_msdu = (PTX_MSDU_DATA)QueuePopHead(&tx_buffer_pool->msdu_free_list);
        if(print_msdu == NULL)
        {
            return;
        }
        if(print_msdu->payload_len != 0)//if this msdu is allocated
        {
            _tx_print_msdu(print_msdu);
        }
        QueuePutTail(&tx_buffer_pool->msdu_free_list,&(print_msdu->link));
    }

     DBG_WLAN__TX(LEVEL_TRACE,"**********buffer tx_msdu end**********\n");
}

void Tx_print_buffer_msdu(PDSP_ADAPTER_T pAdap)
{
    PTX_CONTEXT tx_context = (PTX_CONTEXT)pAdap->tx_context;
    if(tx_context == NULL)
    {
        DBG_WLAN__TX(LEVEL_TRACE,"[%s:]tx context is null\n",__FUNCTION__);
        return;
    }
    _tx_print_buffer_msdu(tx_context);

}
#endif

void _tx_msdu_pool_init(
	PTX_MSDU_POOL pool,
	UINT32	   node_num,
	UINT32	   head_size,
	UINT32	   payload_size) 
{
	UINT32 i = 0, alloc_head_size,alloc_payload_size;
	UINT8* node;
	UINT8* hdr_buf;
	UINT8 *buff;
	TX_MSDU_DATA * tx_msdu ;

	//DBG_WLAN__POOL(LEVEL_INFO, "%s: %d : enter\n",__FUNCTION__,__LINE__);

	if ((NULL == pool)
		|| (0 == node_num)
		|| (0 == head_size)
		|| (0 == payload_size)) 
	{
		goto fail;
	}

	//alloc memory for each node
	node = sc_memory_alloc(node_num * sizeof(TX_MSDU_DATA));
	if (NULL == node) {
		DBG_WLAN__TX(LEVEL_INFO, "%s: %d : Alloc tx_msdu_pool fail !!!\n",__FUNCTION__,__LINE__);
		goto fail;
	}
	sc_memory_set(node, 0, node_num * sizeof(TX_MSDU_DATA));

	//alloc head buffer for node
	alloc_head_size = head_size + PAD4(head_size);
	hdr_buf = sc_memory_alloc(node_num * alloc_head_size);
	if (NULL == hdr_buf) {
		DBG_WLAN__TX(LEVEL_INFO, "%s: %d : Alloc tx_msdu_pool fail !!!\n",__FUNCTION__,__LINE__);
		goto fail;
	}
	sc_memory_set(hdr_buf, 0, node_num * alloc_head_size);

	//alloc payload buffer for node
	alloc_payload_size = payload_size + PAD4(payload_size); 
	buff = sc_memory_alloc( node_num * alloc_payload_size);
	if (NULL == buff) {
		DBG_WLAN__TX(LEVEL_INFO, "%s: %d : Alloc tx_msdu_pool fail !!!\n",__FUNCTION__,__LINE__);
		goto fail;
	}
	sc_memory_set(buff, 0, node_num * alloc_payload_size);

    //INIT msdu free list 
	QueueInitList(&pool->msdu_free_list);
	for (i = 0; i < node_num; i++)
    {   
		tx_msdu = (TX_MSDU_DATA *)(node +  i * sizeof(TX_MSDU_DATA));
		tx_msdu->payload_buff = buff + i * alloc_payload_size;
		tx_msdu->head_buff	= hdr_buf + i * alloc_head_size;
        tx_msdu->head_len    =0;
        tx_msdu->payload_len = 0;
		QueuePutTail(&pool->msdu_free_list,&tx_msdu->link);		
	}

    //init pool data member
	pool->payload_size = payload_size;
	pool->head_size = head_size;
	pool->node_num = node_num;
	pool->node_free = node_num;
	pool->node_used = 0;
	pool->node_buffer  = node;
	pool->head_buff	= hdr_buf;
	pool->payload_buff = buff;
	
	sc_spin_lock_init(&pool->pool_lock);
	DBG_WLAN__TX(LEVEL_TRACE, "[%s]: pool type is %d ,node free is %d\n",
				__FUNCTION__,
				pool->pool_type,
				pool->node_free);
	return;
	
fail:

	kd_assert(0);
	return;
}

/*
 * Name						: pool_alloc_tx_msdu()
 * arguments				: uint16 datalen
 * return value				: mac_data_t *
 * global variables		 : none
 * description				: This function is used to allocate tx_msdu from tx_msdu_pool
 * special considerations	 : None
 * see also					: 
 * TODO						:
 */


PTX_MSDU_DATA _tx_alloc_msdu(
    PDSP_ADAPTER_T pAdap, 
	PTX_MSDU_POOL	 pool,
	UINT32		   payload_size)
{
	PTX_MSDU_DATA tx_msdu ;

	//DBG_WLAN__TX(LEVEL_INFO, "%s: %d : enter\n",__FUNCTION__,__LINE__);
	
	if (NULL == pool)
	{
		return (NULL);
	}
    
	if (payload_size > pool->payload_size) {
		DBG_WLAN__TX(LEVEL_ERR, "%s: %d : Alloc tx_msdul fail !!!\n",__FUNCTION__,__LINE__);
		DBG_WLAN__TX(LEVEL_ERR, "try to alloc too big node, request size  is %d\n", payload_size);
		return (NULL);
	}

	if (QueueEmpty(&pool->msdu_free_list)) {
		DBG_WLAN__TX(LEVEL_ERR, "%s: %d : Alloc tx_msdul fail !!!\n",__FUNCTION__,__LINE__ );
		DBG_WLAN__TX(LEVEL_ERR, "the pool(%d) is exausted!!!\n", pool->pool_type);
        sc_netq_stop(pAdap->net_dev);
        return NULL;
	}

	sc_spin_lock_bh(&pool->pool_lock);
	tx_msdu = (PTX_MSDU_DATA)QueuePopHead(&pool->msdu_free_list);
	pool->node_free--;
	pool->node_used++;
	sc_spin_unlock_bh(&pool->pool_lock);
	
	return tx_msdu;
}



void _tx_release_msdu(
	PTX_CONTEXT tx_context,
	PTX_MSDU_DATA	tx_msdu)
{

	PTX_MSDU_POOL pool;
	PDSP_ADAPTER_T pAdap;
    ULONG flags;
	DBG_WLAN__TX(LEVEL_INFO, "Enter [%s] \n",__FUNCTION__); 

	ASSERT(tx_context);
	ASSERT(tx_msdu);
    
    if( (tx_msdu->frm_type == TX_DATA_FRM)
            || (tx_msdu->frm_type == TX_FW_FRM)
            || (tx_msdu->frm_type == TX_RETRY_FRM))
	{
		pool = &(tx_context->tx_msdu_pool);
	}   
	else
	{
		pool = &(tx_context->tx_mmpdu_pool);   
	}
    //return msdu too msdu pool
	sc_memory_set(tx_msdu->head_buff, 0, pool->head_size);
	sc_memory_set(tx_msdu->payload_buff, 0, pool->payload_size);
    tx_msdu->is_multicast  = FALSE;
    tx_msdu->retry_count   = 0;
    tx_msdu->seqnum        = 0;
    tx_msdu->priority      = 0;
    tx_msdu->DspSendOkFlag = 0;
    tx_msdu->txRate        = 0;
    tx_msdu->Tx_head_irp_in_working = FALSE;
    tx_msdu->Tx_data_irp_in_working = FALSE;
	sc_spin_lock_irqsave(&pool->pool_lock,flags);
    QueuePutTail(&pool->msdu_free_list,&tx_msdu->link);
	pool->node_free++;
	pool->node_used--;
	sc_spin_unlock_irqrestore(&pool->pool_lock,flags);

	DBG_WLAN__TX(LEVEL_INFO, "[%s]: node free = %d, frm type = %d,pool type is %d\n",
				__FUNCTION__,
				pool->node_free,
				tx_msdu->frm_type,
				pool->pool_type); 



    //start net_dev
	pAdap = (PDSP_ADAPTER_T)tx_context->pAdap; 

    if( pAdap->wlan_attr.hasjoined == JOIN_HASJOINED
	    && (1 == sc_netq_ifstop(pAdap->net_dev)))
	{
		sc_netq_start(pAdap->net_dev);
	}
	return;
}



void _tx_msdu_pool_release(PTX_MSDU_POOL pool)
{

	if (NULL == pool) {
		return;
	}
	sc_spin_lock_kill(&pool->pool_lock);
	sc_memory_free(pool->head_buff);
	sc_memory_free(pool->payload_buff);	 
	sc_memory_free(pool->node_buffer);
	return;
}

void _tx_process_sent_msdu(PDSP_ADAPTER_T pAdap, PTX_MSDU_DATA msdu_data)
{

	PTX_CONTEXT tx_context;
#ifdef NEW_RETRY_LIMIT
	UINT32 count;
	PTX_MSDU_DATA msdu_tmp = NULL;
	PUINT32 p_count = &count;
#endif
	tx_context = pAdap->tx_context;
	ASSERT(tx_context);

	wlan_timer_stop(&tx_context->tx_watching_timer);
    if(tx_context->is_time_out)
    {
        tx_context->is_time_out = FALSE;
    }
    
	//set busy flag to false
    tx_context->sending_msdu = NULL;
    tx_context->tx_busy_sending = FALSE;

    
	//update tx stats
	tx_context->tx_good_cnt++;
	tx_context->tx_good_payload_cnt += msdu_data->payload_len;
    
#if 1
    _tx_buffer_msdu(tx_context,msdu_data);
#endif

    #if 0
    if(tx_context->tx_good_cnt % 4 == 0)
    {
        _tx_print_buffer_msdu(tx_context);
    }
    #endif
    
	//always drop a data packet into retry queue
	//and don't care  if retry valid
#ifdef NEW_RETRY_LIMIT		
	//Close ibss mode condition to make ibss support auto rate algrithm 
	if(//	(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA) &&		//in bss mode
	    (pAdap->wlan_attr.hasjoined == JOIN_HASJOINED))
	{

		if(msdu_data->frm_type == TX_DATA_FRM)
		{
			//clear sending null packet flag
			pAdap->send_nulll_count = 0;
			msdu_data->frm_type  = TX_RETRY_FRM;
			sc_spin_lock(&tx_context->tx_list_lock);
			QueueGetCount(&tx_context->retry_frm_que,p_count);
			//put the retry packet context into tail of retry queue
			if(DSP_RETRY_LIST_NUM -1 > count)
			{	
				QueuePutTail(&tx_context->retry_frm_que,&msdu_data->link);
				QueueGetCount(&tx_context->retry_frm_que,p_count);
				DBG_WLAN__TX(LEVEL_INFO,"Insert packet into retry queue tail, and count = %x\n",count);
			}
			else
			{
				//retry queue full
				//so first release head
				//second put into tail
				msdu_tmp = (PTX_MSDU_DATA)QueuePopHead(&tx_context->retry_frm_que);
				QueuePutTail(&tx_context->retry_frm_que,&msdu_data->link);	
				QueueGetCount(&tx_context->retry_frm_que, p_count); 
				DBG_WLAN__TX(LEVEL_INFO,"retry queue is full, and count = %x\n",count);
			}
			sc_spin_unlock(&tx_context->tx_list_lock);
            if(msdu_tmp != NULL)
            {
                msdu_tmp->frm_type= TX_DATA_FRM;  
				_tx_release_msdu(tx_context,msdu_tmp);
            }
                
		}
 
		//For retry packet, release context and retry queue
		//woody debug
		else if(msdu_data->frm_type == TX_RETRY_FRM)
		{
			sc_spin_lock(&tx_context->tx_list_lock);
			QueueGetCount(&tx_context->retry_frm_que, p_count); 
			sc_spin_unlock(&tx_context->tx_list_lock);

			DBG_WLAN__TX(LEVEL_INFO,"[%s]:has sent retry frm ,retry count = %d,retry index is %d\n",
				__FUNCTION__,
				count,
				tx_context->tx_retry_index);
            
				//if reach the retry max limit ,drop the frame
				++ msdu_data->retry_count;
		}
		else
		{
			//release mng frame
			//release download fw frame
			//no release tx retry frame				
			//return it to msdu pool
			_tx_release_msdu(tx_context,msdu_data);
		}
	}
	else //release msdu 
	{
		_tx_release_msdu(tx_context,msdu_data);
	}
	
#else  //new retry
	_tx_release_msdu(tx_context,msdu_data);
#endif//new retry

	//send next frame
	sc_tasklet_schedule(&tx_context->tx_tasklet);
}
/**************************************************************************
 *   Adap_CompletionTxRoutine
 *
 *   Descriptions:
 *	  This completion routine process the condition that a tx irp is responsed.
 *   Argumens:
 *	  DeviceObject: IN, pointer to device object.
 *	  pIrp: IN, Irp that just completed.
 *	  Context: IN, Context structure for Irp to be completed.
 *   Return Value:
 *	  STATUS_SUCCESS: return success.
 *	  STATUS_xxx: return unsuccessful.
 *************************************************************************/
void  _tx_completion(PDSP_ADAPTER_T pAdap,
						   INT32 actual_len,
						   PVOID context)
{
	PTX_CONTEXT tx_context;
	PTX_MSDU_DATA msdu_data;
	/*Get the pointer of context.*/
	
	msdu_data = (PTX_MSDU_DATA)context;
	tx_context = (PTX_CONTEXT)pAdap->tx_context;

	ASSERT(msdu_data);
	ASSERT(tx_context);

    //urb return failed
	if(actual_len < 0)
	{
		DBG_WLAN__TX(LEVEL_ERR, "[%s] tx transfer - failed\n", __FUNCTION__);
		if(!Adap_Driver_isHalt(pAdap))
		{
			  pAdap->mib_stats.error_packet_tx++;
			  tx_context->tx_error_cnt++;
		}
		_tx_print_msdu(msdu_data);
	}
	else
	{
		DBG_WLAN__TX(LEVEL_INFO, "[%s] tx has download real length %d  bytes \n",__FUNCTION__,
					  actual_len);		
		if(msdu_data->payload_len!= actual_len)
		{				
			DBG_WLAN__TX(LEVEL_ERR, "[%s] tx real length error,payload len is %d\n",
						__FUNCTION__,
						msdu_data->payload_len);	
		}
		pAdap->mib_stats.good_packet_tx++;
	}

	if (msdu_data->frm_type == TX_DATA_FRM)
	{
		//clear sending null packet flag
		pAdap->send_nulll_count = 0;
	}
	
#ifndef SERIALIZE_HEADER_DATA
	if(msdu_data->DspSendOkFlag >0)
	{
		msdu_data->DspSendOkFlag--;
	}	

	msdu_data->Tx_data_irp_in_working = FALSE;

	if(msdu_data->DspSendOkFlag == 0)
	{
		_tx_process_sent_msdu(pAdap,msdu_data);
	}		   
#else
	_tx_process_sent_msdu(pAdap,msdu_data);
#endif /*SERIALIZE_HEADER_DATA*/
}



/**************************************************************************
 *   Adap_CompletionTxRoutine
 *
 *   Descriptions:
 *	  This completion routine process the condition that a tx irp is responsed.
 *   Argumens:
 *	  DeviceObject: IN, pointer to device object.
 *	  pIrp: IN, Irp that just completed.
 *	  Context: IN, Context structure for Irp to be completed.
 *   Return Value:
 *	  STATUS_SUCCESS: return success.
 *	  STATUS_xxx: return unsuccessful.
 *************************************************************************/
void  _tx_head_completion(PDSP_ADAPTER_T pAdap,
								  INT32 actual_len, 
								  PVOID context)
{
	PTX_CONTEXT tx_context = NULL;
	PTX_MSDU_DATA msdu_data = NULL;
	/*Get the pointer of context.*/
	
	msdu_data = (PTX_MSDU_DATA)context;
	tx_context = (PTX_CONTEXT)pAdap->tx_context;

	ASSERT(msdu_data);
	ASSERT(tx_context);
	
	if(actual_len < 0)
	{
		DBG_WLAN__TX(LEVEL_ERR, "[%s] tx transfer - failed\n", __FUNCTION__);
		if(!Adap_Driver_isHalt(pAdap))
		{
			  pAdap->mib_stats.error_packet_tx++;
			  tx_context->tx_error_cnt++;
		}
	}	
	else
	{

		DBG_WLAN__USB(LEVEL_INFO, "[%s] tx has download real length %d  bytes \n",
					   __FUNCTION__,
					   actual_len);			
		#if 0 // TODO:Jackie
		if(msdu_data->payload_len!= actual_len)
		{	
				
			DBG_WLAN__TX(LEVEL_ERR, "[%s] tx real length error\n",__FUNCTION__);
			
		}
		#endif
		pAdap->mib_stats.good_packet_tx++;

	}
#ifndef SERIALIZE_HEADER_DATA
	if(msdu_data->DspSendOkFlag > 0)
	{
		msdu_data->DspSendOkFlag--;
	}	

	msdu_data->Tx_head_irp_in_working = FALSE;

	if(msdu_data->DspSendOkFlag == 0)
	{
		_tx_process_sent_msdu(pAdap,msdu_data);
	}		   
#else
	if(STATUS_SUCCESS != UsbDev_BuildBulkOutTransfer(pAdap->usb_context,
								msdu_data->payload_buff,
								msdu_data->payload_len,
								_tx_completion,
								msdu_data);)
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]:submit payload buffer failed \n",__FUNCTION__);	
	}
#endif /*SERIALIZE_HEADER_DATA*/
	
}



/**************************************************************************
 *   Adap_CompletionTxRoutine
 *
 *   Descriptions:
 *	  This completion routine process the condition that a tx irp is responsed.
 *   Argumens:
 *	  DeviceObject: IN, pointer to device object.
 *	  pIrp: IN, Irp that just completed.
 *	  Context: IN, Context structure for Irp to be completed.
 *   Return Value:
 *	  STATUS_SUCCESS: return success.
 *	  STATUS_xxx: return unsuccessful.
 *************************************************************************/
void _tx_fw_completion(PDSP_ADAPTER_T pAdap, 
								INT32 actual_len,
								PVOID context)
{

	PTX_CONTEXT tx_context;
	PTX_MSDU_DATA msdu_data;

	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	msdu_data = (PTX_MSDU_DATA)context;
	ASSERT(tx_context);
	
	if (actual_len >= 0)
	{
		DBG_WLAN__TX(LEVEL_INFO, "[%s] tx FW has download real length %d  bytes \n",
					  __FUNCTION__,
					  actual_len);			
		if(msdu_data->payload_len!= actual_len)
		{	
		    DBG_WLAN__TX(LEVEL_ERR, "[%s] tx FW real length error\n",__FUNCTION__);	
		}
	}
	else
	{
		DBG_WLAN__TX(LEVEL_ERR, "[%s] tx FW transfer - failed\n", __FUNCTION__);	
	}

    //wak up the tx_fm_event 
	sc_event_set(&pAdap->tx_fm_event);
    
	if(NULL != msdu_data)
	{
		_tx_release_msdu(tx_context,msdu_data);
	}
	 
}

VOID _tx_watch_timeout(PVOID param)
{
	PDSP_ADAPTER_T pAdap;
	pAdap = (PDSP_ADAPTER_T)param;

	DBG_WLAN__TX(LEVEL_TRACE, "* * * * * Tx Watch Timeout \n");

	//Don't do this during hardware reset
	if(pAdap->driver_state == DSP_POWER_MANAGER
			|| pAdap->driver_state == DSP_HARDWARE_RESET)
	{
        DBG_WLAN__TX(LEVEL_TRACE, "[%s]: do nothing,driver is in %d!\n",
                    __FUNCTION__,
                    pAdap->driver_state);
        return;
	}

	if(Adap_Driver_isHalt(pAdap))
	{
		return;
	}

	if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_TX_WATCH_TIMEOUT))
			  Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_TX_WATCH_TIMEOUT,DSP_TASK_PRI_HIGH,NULL,0);
	
}




#if 0
//Jakio2006.11.27: changed this func, get tx_speed, data_len etc. from hw_header.
/*Jakio: this function handle downrate after retry limist happened, reset some bits of hw header */
TDSP_STATUS tx_reset_hw_frmctrl(PDSP_ADAPTER_T pAdap, PUSBWLAN_HW_FRMHDR_TX pHdr)
{

	UINT16  tx_speed, data_len, short_preamble, frm_ctrl;
	UINT8  rate, more_frag;
	UINT16 i;
	UINT16 frag_dur, duration;
	 
	PDSP_ATTR_T pt_mattr = (PDSP_ATTR_T)&pAdap->wlan_attr;

	//Get original parameters from hardware header
	tx_speed = (UINT16)((pHdr->len_speed_ctrl >> _3DSP_HWHDR_POS_TX_SPEED) & 0x000f);
	short_preamble = (UINT16)(pHdr->len_speed_ctrl & ~_3DSP_HWHDR_BIT_SHORT_PREAMBLE);
	data_len = (UINT16)(pHdr->len_speed_ctrl >> _3DSP_HWHDR_POS_TX_LENGTH);
	data_len = data_len - sizeof(USBWLAN_HW_FRMHDR_TX) - WLAN_CRC_LEN;
	
	frm_ctrl = (UINT16)(pHdr->frmctrl_fragdur >> _3DSP_HWHDR_POS_FRM_CTRL);
	more_frag = WLAN_GET_FC_MOREFRAG(frm_ctrl);

	//Jakio20070529: changed here, rate may be not same with pt_mattr->rate
	//rate  = pt_mattr->rate & 0x7f;
	//Adap_get_rate_from_index(pAdap, &rate, tx_speed);
	
	//down rate
	//Jakio20070613: down rate up to 2 levels
	for(i = 0; i < 2; i++)
	{
		if(((pAdap->wlan_attr.gdevice_info.dot11_type != IEEE802_11_A) && (tx_speed > 0x00)) ||
		((pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A) && (tx_speed > 0x04)))
		{
			tx_speed --;
			
		}

	}
	rate = Adap_get_rate_from_index(pAdap,tx_speed);

	//frag_duration
	frag_dur = (UINT16)(3 * pt_mattr->gdevice_info.sifs) + 
			_durn(pAdap, (UINT8)(short_preamble), WLAN_CTS_LEN, rate&0x7f) +
			_durn(pAdap, (UINT8)(short_preamble), WLAN_ACK_LEN, rate&0x7f) +
			_durn(pAdap, (UINT8)(short_preamble), (data_len+WLAN_CRC_LEN),rate&0x7f);

	//duration
	duration = 0;
	//Jakio 2006.11.09: no need recalcultate duration,
	//because next frag's rate keeps unchanged
	#if 0
	if(more_frag)
	{
		duration = (UINT16)3 * pt_mattr->gdevice_info.sifs +
				2 * _durn(pAdap, (UINT8)(short_preamble), WLAN_ACK_LEN,rate&0x7f) +
				_durn(pAdap, (UINT8)(short_preamble), data_len+WLAN_CRC_LEN,rate&0x7f);
	}
	else
		duration = _duration_ack(pAdap, (UINT8)(short_preamble), rate & 0x7f);
	#endif
	//reset tx_speed
	pHdr->len_speed_ctrl &= ~_3DSP_HWHDR_BIT_TX_SPEED;
	pHdr->len_speed_ctrl |= (tx_speed & 0x0f) << _3DSP_HWHDR_POS_TX_SPEED;

	//reset frag_dur
	pHdr->frmctrl_fragdur &= 0xffff0000; //Jakio20070528: bug fixed, changed from 0x0000 to 0xffff0000
	pHdr->frmctrl_fragdur |= frag_dur;

	//Jakio 2006.11.09: no need reset duration
	#if 0
	//reset duration
	pHdr->addr1lo_txdurid &= 0x0000;
	pHdr->addr1lo_txdurid |= duration;
	#endif

	return STATUS_SUCCESS;
}
#endif

VOID _tx_calc_mic(PDSP_ADAPTER_T pAdap, 
						PUINT8		data, 
						UINT32		data_len, 
						PUINT8		key, 
						PUINT8		hdr, 
						PUINT8		mic
					)
{
	UINT32 l, r;
	int i, blocks, last;

	l = get_le32(key);
	r = get_le32(key + 4);

	/* Michael MIC pseudo header: DA, SA, 3 x 0, Priority */
	l ^= get_le32(hdr);
	michael_block(l, r);
	l ^= get_le32(&hdr[4]);
	michael_block(l, r);
	l ^= get_le32(&hdr[8]);
	michael_block(l, r);
	l ^= get_le32(&hdr[12]);
	michael_block(l, r);

	/* 32-bit blocks of data */
	blocks = data_len / 4;
	last = data_len % 4;
	for (i = 0; i < blocks; i++) {
		l ^= get_le32(&data[4 * i]);
		michael_block(l, r);
	}

	/* Last block and padding (0x5a, 4..7 x 0) */
	switch (last) {
	case 0:
		l ^= 0x5a;
		break;
	case 1:
		l ^= data[4 * i] | 0x5a00;
		break;
	case 2:
		l ^= data[4 * i] | (data[4 * i + 1] << 8) | 0x5a0000;
		break;
	case 3:
		l ^= data[4 * i] | (data[4 * i + 1] << 8) |
			(data[4 * i + 2] << 16) | 0x5a000000;
		break;
	}
	michael_block(l, r);
	/* l ^= 0; */
	michael_block(l, r);

	put_le32(mic, l);
	put_le32(mic + 4, r);
}


VOID _tx_tkip_enmic(PDSP_ADAPTER_T pAdap,
							PUINT8		data, 
							UINT32		data_len, 
							PUINT8		key, 
							PUINT8		daddr, 
							PUINT8		saddr, 
							PUINT8		mic
						)
{
	UINT8	hdr[16];
	sc_memory_set(hdr, 0, 16);
	sc_memory_copy(hdr, daddr, 6);
	sc_memory_copy(hdr+6, saddr, 6);
	_tx_calc_mic(pAdap, data, data_len, key, hdr, mic);
}
/*Jakio: fixed an error when caculate duration. 2006.11.2*/
/*void  tx_get_speed_and_duartion(PDSP_ADAPTER_T pAdap, UINT8 more_flag, UINT16 payload_len, 
				UINT8 * tx_speed, UINT8 * short_preamble, 
				UINT16 *frag_dur, UINT16 *duration, 
				UINT8 * cts_self, UINT16 next_frag_len)*/
void  tx_get_speed_and_duartion(
			PDSP_ADAPTER_T pAdap, 
			UINT8 			speed,
			PHW_HDR_PARSE_CONTEXT pcontext,
			PHW_HDR_PARSE_RESULT	presult,
			BOOLEAN  use_lower_speed)

{
	//Jakio: for caculating duration
	UINT8 data_rate;
	UINT8 rts_rate;
//	PMNG_STRU_T	 pt_mng = pAdap->pmanagment;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	UINT16  cur_len = 0;
	UINT16  next_len = 0;
	PTX_CONTEXT tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);

	//Jakio20070529: add here for different rate between mng and data frame
	//when transmitting mng and ctrl frame, use the min tx speed
	//woody close
	//if((pcontext->type != WLAN_FTYPE_DATA) ||
	//	((pcontext->type == WLAN_FTYPE_DATA) &&(pcontext->sub_type == WLAN_FSTYPE_DATA_802_1X)))
	if(pcontext->type != WLAN_FTYPE_DATA)
	{
		if(pAdap->wlan_attr.gdevice_info.dot11_type == IEEE802_11_A)
			presult->tx_speed = 0x04;
		else
			presult->tx_speed = 0x00;
		data_rate = Adap_get_rate_from_index(pAdap,presult->tx_speed);
	}
	else		//justin;	current frame is data
	{
		if(use_lower_speed)
		{
			presult->tx_speed = 0x00;
			data_rate = Adap_get_rate_from_index(pAdap,presult->tx_speed);
		}
		else
		{
			presult->tx_speed  = speed;
			data_rate = Adap_get_rate_from_index(pAdap,presult->tx_speed);
			//Make rate show smooth from OS angle
			Rate_Calc_Smooth(pAdap,data_rate);
		}		
	}
	
	//caculate current frag len and next frag len
	//PS POLL 
	if((pcontext->type == WLAN_FTYPE_CTL) && (pcontext->sub_type == WLAN_FSTYPE_PSPOLL))
	{
		ASSERT(pcontext->payload_len == 0);
		cur_len = sizeof(USBWLAN_HW_FRMHDR_TX) + WLAN_CRC_LEN - 2*sizeof(UINT32);
		next_len = 0;
	}
	//other frame
	else
	{
		cur_len = sizeof(USBWLAN_HW_FRMHDR_TX) + WLAN_CRC_LEN + pcontext->payload_len;
		if(pcontext->more_flag)
			next_len = sizeof(USBWLAN_HW_FRMHDR_TX) + WLAN_CRC_LEN + pcontext->next_frag_len;
	}
	
	//encypt frms when the payload_len bigger than 0
	if(pcontext->privacy && pcontext->payload_len)
	{
		switch(pt_mattr->wep_mode)
		{
		case WEP_MODE_WEP:
			cur_len += 8;
			next_len += 8;
			break;
		case WEP_MODE_TKIP:
			cur_len += 12;
			next_len += 12;
			break;
		case WEP_MODE_AES:
			cur_len += 16;
			next_len += 16;
			break;
		default:
			cur_len += 0;
			next_len += 0;
		}

	}

	//jakio 20070406: record the frame's air length
	presult->air_len = cur_len;
	
#if 0	
	*short_preamble = 1;
	if((*tx_speed < 4)&&(dot11_type == IEEE802_11_A))
		*short_preamble = 0;
#endif	
	presult->short_preamble = (pt_mattr->gdevice_info.preamble_type == SHORT_PRE_AMBLE) ? 1:0;
	
	//send with 11b rate at 11g mode, short preamble should be set to 0
	/*
	if((*short_preamble == 1) && (dot11_type == IEEE802_11_G) && (*tx_speed < 4))
	{
		*short_preamble = 0;	
	}*/

	//close here, otherwise we can't fix dlink 624 ap 's bug
/*
	//update by woody
	//the code from AP's code
	//it indicate 11b rate is always with long preamble
	//if fact, only 1m rate can't work at short preamble
	//maybe for 3dsp, all of 11b rate always support short preamble
	if((dot11_type != IEEE802_11_A) && (presult->tx_speed < 4))
	{
		presult->short_preamble = 0;	
	}
*/
	presult->cts_self = (UINT8)Rts_running_mode(pAdap);
	rts_rate = get_true_rts_rate(pAdap);

#if 0
	//frag_duration	//duration_rts
	presult->frag_dur = 3 * pt_mattr->gdevice_info.sifs + 
			_durn(pAdap, presult->short_preamble, WLAN_CTS_LEN, data_rate) +
			_durn(pAdap, presult->short_preamble, WLAN_ACK_LEN, data_rate) +
			_durn(pAdap, presult->short_preamble, cur_len, data_rate);
#else
		//frag_duration	//duration_rts
		if(presult->cts_self )
		{
			presult->frag_dur = DURATION_CTSTOSELF(pAdap,cur_len,data_rate,rts_rate);			
		}
		else
		{
			presult->frag_dur = 2 * pt_mattr->gdevice_info.sifs + 
			_durn(pAdap, presult->short_preamble, WLAN_CTS_LEN, rts_rate) +
			_durn(pAdap, presult->short_preamble, cur_len, data_rate);
		}	
#endif


	//duration
	presult->duration = 0;
	if(pcontext->more_flag)
	{
		presult->duration = 3 * pt_mattr->gdevice_info.sifs +
				2 * _durn(pAdap, presult->short_preamble, WLAN_ACK_LEN,data_rate) +
				_durn(pAdap, presult->short_preamble, next_len,data_rate);
	}
	else
	{
		presult->duration = _duration_ack(pAdap, presult->short_preamble, data_rate & 0x7f);
	}	
	
}




void  wlan_build_data_frmctrl(PDSP_ADAPTER_T pAdap, BOOLEAN privacy, UINT8 more_flag,UINT16 *frm_ctrl)
{

	ASSERT(pAdap);

	*frm_ctrl = 0x00;

	//Type&SubType
	*frm_ctrl = (UINT16)( WLAN_SET_FC_FTYPE(WLAN_FTYPE_DATA) |  
				WLAN_SET_FC_FSTYPE(WLAN_FSTYPE_DATAONLY));
	//Wep_use?
	//Jakio2006.11.27: get privacy option from p_attr->gdevice_info.privacy_option
	if(privacy)
		*frm_ctrl |= (UINT16)WLAN_SET_FC_ISWEP(1);
	//To Ds
	if(pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
	{
		*frm_ctrl |= (UINT16)(WLAN_SET_FC_TODS(1));
	}

	//Power Save
	//if(pAdap->wlan_attr->ps_state == PS_POWERSAVE_HIGH)
	if(pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)
		*frm_ctrl |=(UINT16)WLAN_SET_FC_PWRMGT(1);

	//more frag
	if(more_flag)
		*frm_ctrl |= (UINT16)(WLAN_SET_FC_MOREFRAG(1));

	
}


VOID fill_hw_data_header(
	PDSP_ADAPTER_T pAdap,
	PTX_MSDU_DATA  msdu_data,
	PUSBWLAN_HW_FRMHDR_TX header, 
	UINT8 more_frag, 
	UINT16 payload_len,
	UINT16 seq_num, 
	UINT16 next_frag_len,
	BOOLEAN  is_a1_group,
	BOOLEAN  is_a3_group)
{
	//UINT8   tx_speed, privacy, short_preamble;
	//UINT16  frag_dur, duration;
	UINT16  tx_len;
	//UINT8   cts_self;
	UINT8	privacy;
	UINT16 frm_ctrl;
	UINT8   txpwr_lvl;
	UINT8	key_valid;
	PDSP_ATTR_T  p_attr;
	//Jakio2006.12.15: add here
	HW_HDR_PARSE_CONTEXT	context;
	HW_HDR_PARSE_RESULT	result;
	PTX_CONTEXT tx_context;
    UINT8 rate;
	UINT8 index;

	ASSERT(pAdap);
	ASSERT(header);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	p_attr = (PDSP_ATTR_T)&pAdap->wlan_attr;
	
	sc_memory_set(header, 0, sizeof(USBWLAN_HW_FRMHDR_TX));
	sc_memory_set(&result, 0, sizeof(HW_HDR_PARSE_RESULT));
	

	//Jakio20070419: should add additional length becauseof encryption.
	//tx_len = sizeof(USBWLAN_HW_FRMHDR_TX) + payload_len  + WLAN_CRC_LEN;

	//Jakio20070531: should check key valid flag.
	//key_valid = p_attr->wpa_pairwise_key_valid;
	//woody
	key_valid = (is_a1_group) ? p_attr->wpa_group_key_valid :p_attr->wpa_pairwise_key_valid;
	privacy = (key_valid && p_attr->gdevice_info.privacy_option);
	//For dlink 1310 ap
	if((pAdap->wpa_1x_no_encrypt != 0 )
		&& (msdu_data->is_tx_1x_packet) 
		&& (!is_a1_group))
	{
		pAdap->wpa_1x_no_encrypt--;
		privacy = 0;
	}
	if(payload_len == 0)	//justin: 0920 add to deal with NULL frame
    {   
		privacy = 0;
    }
    /*Always encrpte data frame*/
	//privacy  = p_attr->gdevice_info.privacy_option;
	//woody fixed it for debug
	//p_attr->gdevice_info.privacy_option = FALSE;
	//privacy  = 0;
	
	//txpwr_lvl = pAdap->txpwr_level;
	//txpwr_lvl = pAdap->wlan_attr.tx_power;

	context.more_flag = more_frag;
	context.next_frag_len = next_frag_len;
	context.payload_len = payload_len;
	context.privacy = privacy;
	//Jakio20070528 add here
	context.type = WLAN_FTYPE_DATA;
	context.sub_type = WLAN_FSTYPE_DATAONLY; 
	rate = msdu_data->txRate;
	index = Adap_get_rateindex_from_rate(pAdap,rate);

	//Jakio2006.12.15: changed here
	//calculate some fields
	tx_get_speed_and_duartion(pAdap, index,&context, &result,is_a1_group ||is_a3_group ||msdu_data->is_tx_1x_packet);

	//always set tx power max in 75% while 54mbps TX
	txpwr_lvl = pAdap->wlan_attr.tx_power;
	if((txpwr_lvl == 3) && (result.tx_speed == 11))
	{
		//DBGSTR(("power = %x \n",txpwr_lvl));
		txpwr_lvl = 2;
	}
	//Jakio20070419: 
	tx_len =  result.air_len;
	
	//build 802.11 frame control fields
	wlan_build_data_frmctrl(pAdap, privacy, more_frag, &frm_ctrl);
	
	//begin fill the hw frame header
	 header->len_speed_ctrl |=  (result.tx_speed << _3DSP_HWHDR_POS_TX_SPEED) | 
				(tx_len << _3DSP_HWHDR_POS_TX_LENGTH) |
				_3DSP_HWHDR_BIT_INT_ON_TX|//_3DSP_HWHDR_BIT_WR_ACK |	//Justin: not need ack
				(result.short_preamble? _3DSP_HWHDR_BIT_SHORT_PREAMBLE:0) |
				(privacy?_3DSP_HWHDR_BIT_PRIVACY:0);
	//This bit is toggled to indicate every new MSDU fragment.  
	//For fragments, which are part of the same MSDU, the "seqn" remains the same
	//Otherwise no backoff runing

	if(pAdap->seq_tag)
	{
		 header->len_speed_ctrl	|= _3DSP_HWHDR_BIT_NEW_SEQ_NUM;
	}
	else
	{
		 header->len_speed_ctrl	&=~ _3DSP_HWHDR_BIT_NEW_SEQ_NUM;
	}

	pAdap->seq_tag = (!pAdap->seq_tag);
	
	header->len_speed_ctrl |= ((pAdap->wlan_attr.default_key_index & 0x03) << _3DSP_HWHDR_POS_DEFAULT_KEY_ID);
	//For wpa pairwise packet
	if(is_a1_group && pAdap->wlan_attr.wep_mode != WEP_MODE_WEP)
	{
		header->len_speed_ctrl |= ((0) << _3DSP_HWHDR_POS_DEFAULT_KEY_ID);		
	}

	if(!is_a1_group)
	{
		header->len_speed_ctrl |= _3DSP_HWHDR_BIT_RETRY_ENABLE;
	}

	//header->len_speed_ctrl &=(~((UINT32)_3DSP_HWHDR_BIT_TX_CTS_SELF));	 
	   if(result.cts_self)
		   header->len_speed_ctrl |=(UINT32)_3DSP_HWHDR_BIT_TX_CTS_SELF; 
	//Tx power level
	header->len_speed_ctrl |= (txpwr_lvl & 0x03) << _3DSP_HWHDR_POS_TX_PWR_LVL;


	 header->frmctrl_fragdur |= result.frag_dur | (frm_ctrl << _3DSP_HWHDR_POS_FRM_CTRL);

	//duration
	if(is_a1_group)
	{
		result.duration = 0;
        //Set the bit for IBSS mode while broadcast packet sent
		//Otherwise retry limit reach always returned with broadcast sent under ibss mode
		header->len_speed_ctrl  &= ~_3DSP_HWHDR_BIT_DONT_EXPECT_ACK;
		header->len_speed_ctrl |=   _3DSP_HWHDR_BIT_DONT_EXPECT_ACK;

	}
	header->addr1lo_txdurid |= result.duration;
	//sequnce control
	header->seqctrl_addr3hi |= seq_num << _3DSP_HWHDR_POS_SEQ_CTRL;

		
}

/*
  *IN pAdaptor:   pointer to the global data struct
  *IN pcontext:	irp context, buffered for the 802.3 and 802.11 fragments
  *IN len:			  802.3 frame length
  * frag_len:		 length of one fragment
  *ret_len:			total length of fragements 
  */
void _tx_fragment_procedure(PDSP_ADAPTER_T pAdap,
											PTX_MSDU_DATA msdu_data,
											UINT8*   data,
											UINT16   len, 
											UINT16 frag_len, 
											PUINT32 ret_len, 
											PUINT16 frag_num)
{

	UINT8 * psrc	 = NULL;
	UINT8 * pbody  = NULL; //pointer to body field of frag
	UINT8 * addr1 = NULL;
	UINT8 * addr3 = NULL;
	UINT8	more_frag;
	UINT16  eth_type, eth_len;
	//Record the payload length of current mpdu, for the hardware header fields' caculation
	UINT16  payload_len;
	//Record the read pointer of 802.3 frame
	UINT16  r_offset;
	UINT16  w_offset;
	UINT16  seq_num;
	UINT16  next_frag_len;  //Jakio add 2006.11.2, next frag payload length
	ETHER_HEADER_T eth_header;
	//Jakio20070604: add for tkip mic calculation
	UINT8	privacy, key_valid;   
	UINT8	mic_value[WLAN_MIC_KEY_LEN];
	UINT16 tmp_frag_num = 0;
	PUSBWLAN_HW_FRMHDR_TX  hw_header;	
	UINT8  data_rate,tmp;
	PDSP_ATTR_T   pt_mattr = &pAdap->wlan_attr;
	
	ASSERT(pAdap);
	
	//Jakio20070605: save ether header
	sc_memory_copy(&eth_header, data, sizeof(ETHER_HEADER_T));
	psrc =  data + sizeof(ETHER_HEADER_T);

	//addresses for ESS STA
	if (pAdap->wlan_attr.macmode == WLAN_MACMODE_ESS_STA)
	{
		addr1 = pAdap->wlan_attr.bssid;
		addr3 = eth_header.daddr;
	}
	else
	{
		addr1 = eth_header.daddr;
		addr3 = pAdap->wlan_attr.bssid;
	}
    
	//reserve space for frag number
	//seq_num = get_current_seqnum(pAdap) << 4;
	if(msdu_data->frm_type == TX_RETRY_FRM)
	{
		//don't change sequence number of retry packet
		DBG_WLAN__TX(LEVEL_TRACE, "[%s]: retry frame seq = %x, last seq num = %x\n",
                        __FUNCTION__,
                        msdu_data->seqnum,
                        pAdap->wlan_attr.seq_num);
		seq_num = msdu_data->seqnum;
	}
	else
	{
		//get new sequence number for packet sent first time
		seq_num = get_current_seqnum(pAdap) << 4;
		msdu_data->seqnum = seq_num;
	}
	

	//type decision
	eth_type = UshortByteSwap(eth_header.type);
	if(eth_type < WLAN_ENCAP_TYPE_BASE) //has snap header
	{
		//802.3 frame type
		eth_len = eth_type;
	}
	else //dixII type
	{
		//ethernet frame type
		psrc -= 8;   //preserve 8 bytes for snap header and type length
		sc_memory_copy(psrc, rfc1042_header, 6);
		sc_memory_copy(psrc+6, &eth_header.type, 2);
		eth_len = (UINT16)(len - sizeof(ETHER_HEADER_T) +8); //"8" stands for the snap header and type length
	}

	//Jakio20070604: add here for tkip mic value
	key_valid = pAdap->wlan_attr.wpa_pairwise_key_valid;
	privacy = pAdap->wlan_attr.gdevice_info.privacy_option;
	if(privacy && (pAdap->wlan_attr.wep_mode == WEP_MODE_TKIP) && key_valid)
	{
		//wpa_1x_no_encrypt is not 0 mean current packet should be encryped
		//so no mic should be attached
		//if(pAdap->wpa_1x_no_encrypt != 0 && pAdap->is_tx_1x_packet)
		if(pAdap->wpa_1x_no_encrypt != 0 && msdu_data->is_tx_1x_packet)
		{
			DBG_WLAN__TX(LEVEL_TRACE,"[%s]: 8021x without encryption so without mic\n",__FUNCTION__);
		}
		else
		{

		    _tx_tkip_enmic(pAdap, 
						psrc, 
						eth_len, 
						pAdap->wlan_attr.wpa_pairwise_mic_tx, 
						eth_header.daddr, 
						eth_header.saddr, 
						mic_value);
         }
		//sc_memory_copy(pcontext->TxCached + eth_len, mic_value, WLAN_MIC_KEY_LEN);
		sc_memory_copy(psrc + eth_len, mic_value, WLAN_MIC_KEY_LEN);
		eth_len += WLAN_MIC_KEY_LEN;
	}
		
	//Result size exeeds the buffer size, failure
	if((eth_len + ((eth_len/frag_len)+1)*sizeof(USBWLAN_HW_FRMHDR_TX)) > DSP_TX_BUF_SIZE)
		return ;

	tmp_frag_num = 0;
	r_offset = 0;
	w_offset = 0;
	while(1)
	{
		seq_num &=0xfff0;
		seq_num |= (tmp_frag_num & 0x0f);
		if((tmp_frag_num + 1)*frag_len < eth_len)
			more_frag = 1;
		else 
			more_frag = 0;
		
		hw_header = (PUSBWLAN_HW_FRMHDR_TX)(msdu_data->payload_buff + w_offset);
		pbody = (UINT8 *)hw_header + sizeof(USBWLAN_HW_FRMHDR_TX);

		//copy the data body 
		if(more_frag)
		{

			//sc_memory_copy(pbody+w_offset, psrc+r_offset, frag_len);
			sc_memory_copy(pbody, psrc+r_offset, frag_len);
			r_offset += (UINT16)frag_len;
			w_offset += (UINT16)(frag_len + sizeof(USBWLAN_HW_FRMHDR_TX));
			payload_len  = frag_len;
		}
		else
		{
			payload_len = eth_len%frag_len;
			//just align with frag_len
			payload_len = ((eth_len !=0) &&(payload_len == 0)) ? frag_len : payload_len;
			//copy left bytes of pcontext->TxChached
			//sc_memory_copy(pbody+w_offset, psrc+r_offset, payload_len);
			sc_memory_copy(pbody, psrc+r_offset, payload_len);
			r_offset += payload_len;
			w_offset += payload_len + (sizeof(USBWLAN_HW_FRMHDR_TX));
		}

		tmp_frag_num ++;

		/*Jakio 2006.11.2: get nex frag len, for caculating duration*/
		if(more_frag)
		{
			next_frag_len = ((len - r_offset) >= frag_len) ? (frag_len) : (len - r_offset);
		}
		else
			next_frag_len = 0;


		//justin:	081009.	for ibss, must set a rate less than the max rate ibss supported
		if((pt_mattr->macmode == WLAN_MACMODE_IBSS_STA)
			&& (!MAC_ADDR_IS_GROUP(addr1)))
		{	
		    data_rate = pt_mattr->rate;//Adap_get_rate_from_index(pAdap, pt_mng->rateIndex);	//get current rate
			//DBGSTR(("ibss current rate = 0x%x\n",data_rate));
			tmp = (Mng_GetMaxIbssSupportRate(pAdap,addr1) & 0x7f);					//get max support rate
			//DBGSTR(("ibss max support rate = 0x%x\n",tmp));
			
			if(data_rate > tmp)
			{
				//pt_mng->rateIndex = Adap_get_rateindex_from_rate(pAdap, tmp);//0x00;
				//data_rate = tmp;//Adap_get_rate_from_index(pAdap,presult->tx_speed);
				pt_mattr->rate = tmp;
			}

            Adap_SetRateIndex(pAdap);
			//woody1224
			if(msdu_data->txRate > pt_mattr->rate)
			{
				msdu_data->txRate = pt_mattr->rate;
			}			

		}

		
		//next to do, at now I just leave where it is
		fill_hw_data_header(pAdap, msdu_data,hw_header,more_frag,payload_len, seq_num,next_frag_len,
							MAC_ADDR_IS_GROUP(addr1),MAC_ADDR_IS_GROUP(addr3));
		
		//fill all the addrs
		hw_header->addr1lo_txdurid |= ((addr1[1] << 24) | (addr1[0] << 16));
				hw_header->addr1hi = ((addr1[5] << 24) | (addr1[4] << 16) |
							 (addr1[3] << 8)  | addr1[2]);
				hw_header->addr3lo = ((addr3[3] << 24) | (addr3[2] << 16) |
							 (addr3[1] << 8)  | addr3[0]);
				hw_header->seqctrl_addr3hi |= ((addr3[5] << 8 ) | addr3[4]);

		if(!more_frag)
			break;	
	}

	*ret_len = w_offset;
	*frag_num = tmp_frag_num;
}



/**********************************************************************************************
*Tx_frame_Is_802_1x
*	Jakio20070531 add, judge whether a frame is 802.1x or not
*802.1x frame format*
	+------------+-------------+---------------+...+...
	+  d_addr(6B)	+   s_addr(6B)   + flag(2B):88 8e	 +...+....
	+-----------------------------------------------------+..
************************************************************************************************/
BOOLEAN tx_frame_Is_802_1x(PUINT8 pbuf, UINT16 len)
{
	PUINT8 hi_byte, lo_byte;

	//simple safe check
	ASSERT(pbuf && (len > (sizeof(ETHERNET_HEADER_SIZE))));

	lo_byte = pbuf + ETHERNET_HEADER_SIZE -2;
	hi_byte = lo_byte + 1;

	if((*lo_byte == 0x88) && (*hi_byte == 0x8e))
		return TRUE;
	else
		return FALSE;
	
}

//Jakio20070604: calculate mic value, copied from AP project
/*
* returns result in buffer pointed by mic arg, must be 8 bytes at least
*/



TDSP_STATUS _tx_submit_msdu_head( PDSP_ADAPTER_T pAdap,PTX_MSDU_DATA msdu_data)
{

	//PNDIS_PACKET		 Packet; 
	TDSP_STATUS		 status;
	PDATA_HEAD_FORMAT_T  phead;
	PTX_CONTEXT tx_context; 
	phead = (PDATA_HEAD_FORMAT_T)msdu_data->head_buff;
	tx_context = (PTX_CONTEXT)pAdap->tx_context;

  #ifdef DSP_SINGLE_MODE_FOR_HEAD
	if((phead->fifo_num == _3DSP_TX_FIFO_IDX_BCN) || (phead->fragcount > 1))   // frage > 1 , burst mode used
	{
		//beacon frame with burst mode
		//pContext->headBuffer[3] = _3DSP_TX_FIFO_IDX_CP;
		
		status = UsbDev_BuildVendorRequestAsyn(
				pAdap->usb_context,
				msdu_data->head_buff,
				DSP_TX_HEAD_BUF_SIZE/2,   //4//4bytes in length
				VCMD_WRITE_BURSTREG,
				0,
				VCMD_WRITE_OP,
				REG_MAILBOX_HEAD + 4*phead->sub_type,	//sub_type indicate head1 or head2
				_tx_head_completion,
				(PVOID)msdu_data);
	}
	else
	//no fraged packet, Just frag = 1
	{
			//no beacon frame with single mode
			status = UsbDev_BuildVendorRequestAsyn(
						pAdap->usb_context,
						NULL,
						0,//DSP_TX_HEAD_BUF_SIZE,   //pContext->length, jakio20070402
						VCMD_WRITE_REG,
						(UINT16)phead->dword_len,
						VCMD_WRITE_OP,
						REG_MAILBOX_HEAD + 4*phead->sub_type + 3,	//sub_type indicate head1 or head2
						_tx_head_completion,
						(PVOID)msdu_data);	
	}

#else   //DSP_SINGLE_MODE_FOR_HEAD	
	status =  UsbDev_BuildVendorRequestAsyn(
					pAdap->usb_context,
					msdu_data->head_buff,
					DSP_TX_HEAD_BUF_SIZE,   //pContext->length, jakio20070402
					VCMD_WRITE_BURSTREG,
					0,
					VCMD_WRITE_OP,
					REG_MAILBOX_HEAD,
					_tx_head_completion,
					(PVOID)msdu_data);//OFFSET_MAILBOX_FLAG_REG,//0x100	 zyy.	
#endif   //DSP_SINGLE_MODE_FOR_HEAD
	return status;

}

BOOLEAN _tx_submit_msdu( PDSP_ADAPTER_T pAdap,PTX_MSDU_DATA msdu_data)
{

	//PNDIS_PACKET		 Packet; 
	TDSP_STATUS		 status;
	PDATA_HEAD_FORMAT_T  phead;
	PTX_CONTEXT tx_context;

    ASSERT(pAdap);
    ASSERT(msdu_data);
    
	phead = (PDATA_HEAD_FORMAT_T)msdu_data->head_buff;
	tx_context = (PTX_CONTEXT)pAdap->tx_context;

    if(TX_RETRY_FRM == msdu_data->frm_type)
    {
    	//Indicate the packet retry limit reach
    	if(pAdap->Retrylimit_Reach != 0 )
    	{
    		DBG_WLAN__TX(LEVEL_INFO,"[%s]: Retry packet  old rate = %x\n",__FUNCTION__, msdu_data->txRate);
    		pAdap->Retrylimit_Reach = 0; 
            if(((msdu_data->txRate & 0x7f) == 0x02) && 
			    (msdu_data->retry_count< DSP_MAX_RETRY_COUNT))
		    {
			    msdu_data->retry_count= DSP_MAX_RETRY_COUNT;
		    }
    		msdu_data->txRate = Get_Lower_Txrate(pAdap,msdu_data->txRate);
    		DBG_WLAN__TX(LEVEL_INFO,"[%s]: Retry packet  real rate = %x\n",__FUNCTION__, msdu_data->txRate);
    	}	
    	//indicate not this packet retry limit reach,So use current rate to send
    	else
    	{
    		msdu_data->txRate = pAdap->wlan_attr.rate;
    	}
    }
    
    ASSERT(0 == msdu_data->DspSendOkFlag);
	msdu_data->DspSendOkFlag = 2;  
	msdu_data->Tx_head_irp_in_working = TRUE;
	msdu_data->Tx_data_irp_in_working = TRUE;
	wlan_timer_start(&tx_context->tx_watching_timer,2000);	//begin to wait tx complete

	DBG_WLAN__TX(LEVEL_INFO, "[%s]: tx transmint msdu type is %d, len is %d\n",
						__FUNCTION__,
						msdu_data->frm_type,
						msdu_data->payload_len); 

	status = _tx_submit_msdu_head(pAdap,msdu_data);

	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]:submit head buffer failed \n",__FUNCTION__);	
		tx_context->tx_error_cnt++;
		return FALSE;
	}

    //DBG_WLAN__TX(LEVEL_TRACE, "[%s]: tx transmint msdu head success!\n",__FUNCTION__);
#ifndef SERIALIZE_HEADER_DATA
	status = UsbDev_BuildBulkOutTransfer(pAdap->usb_context,
								msdu_data->payload_buff,
								msdu_data->payload_len,
								_tx_completion,
								msdu_data);

	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]:submit payload buffer failed \n",__FUNCTION__);	
		tx_context->tx_error_cnt++;

		return FALSE;
	}
#endif	
    //DBG_WLAN__TX(LEVEL_TRACE, "[%s]: tx transmint msdu body success!\n",__FUNCTION__);
    //we always send head packet first
    sc_spin_lock(&tx_context->tx_list_lock);
	tx_context->sending_msdu = msdu_data;
    tx_context->tx_busy_sending = TRUE;
    sc_spin_unlock(&tx_context->tx_list_lock);
    return TRUE;
	
}

void _tx_routine(ULONG param)
{

	PDSP_ADAPTER_T	  pAdap;
	PTX_CONTEXT		 tx_context;	

	PTX_MSDU_DATA msdu_data ;
#ifdef NEW_RETRY_LIMIT
	PDSP_LIST_ENTRY_T pblock;
    UINT32	i = 0;
	UINT32	count = 0;
	PUINT32 p_count = &count;
#endif
	//send next packet
	//first check notify queue and mng queue
	//second check retry queue
	//third check data queue
	pAdap = (PDSP_ADAPTER_T)param;
	tx_context = pAdap->tx_context;
	ASSERT(tx_context);
    
    // TODO: Jakcie need to confirm
	if(tx_context->tx_busy_sending                 //the prev frame head or body not returned
	   ||pAdap->wlan_attr.gdevice_info.tx_disable
		|| (pAdap->driver_state == DSP_POWER_MANAGER)
		|| (pAdap->driver_state == DSP_STOP_TX)) 
	{
		DBG_WLAN__TX(LEVEL_INFO,"[%s]:do nothing,tx_busy_sending = %d,tx_disable = %d,diver state = %d\n",
			__FUNCTION__,
			tx_context->tx_busy_sending,
			pAdap->wlan_attr.gdevice_info.tx_disable,
			pAdap->driver_state);	
		return;
	}
	//Justin: 0802, 
	if(!Adap_Driver_isTxWork(pAdap))
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]: fail --- driver_state = %d \n",
					 __FUNCTION__,
					 pAdap->driver_state); 
		return;
	}
  //here first transact high level mng frame including notyfy and probe response
	sc_spin_lock(&tx_context->tx_list_lock);
	msdu_data = (PTX_MSDU_DATA)QueuePopHead(&tx_context->mng_frm_high_que);
	sc_spin_unlock(&tx_context->tx_list_lock);

   
	if(NULL != msdu_data)
	{
		DBG_WLAN__TX(LEVEL_TRACE,"[%s]:mng_frm_normal_que type is %d\n",__FUNCTION__,msdu_data->msg_type);	
   
		if (msdu_data->msg_type == TX_MNG_TYPE_PROBERSP)
		{
			if(!_tx_submit_msdu(pAdap,msdu_data))
			{
				 //return msdu to pool
				sc_spin_lock(&tx_context->tx_list_lock);
				QueuePushHead(&tx_context->mng_frm_high_que,&msdu_data->link);
				sc_spin_unlock(&tx_context->tx_list_lock);
			}
			return;
		}
		else
		{
			//insert msdu to que head
			sc_spin_lock(&tx_context->tx_list_lock);
			QueuePutTail(&tx_context->mng_frm_high_que,&msdu_data->link);
			sc_spin_unlock(&tx_context->tx_list_lock);
			msdu_data = NULL;
		}
	}

	   //here transact normal mng frame
	sc_spin_lock(&tx_context->tx_list_lock);
	msdu_data = (PTX_MSDU_DATA)QueuePopHead(&tx_context->mng_frm_normal_que);
	sc_spin_unlock(&tx_context->tx_list_lock);
   
	if(NULL != msdu_data)
	{
	 //Support for null data frame
		//Support for ps-poll frame, we send ps-poll just as normal mng frames
	  
		if ((msdu_data->msg_type == TX_MNG_TYPE_NORMAL)
		   || (msdu_data->msg_type== TX_MNG_TYPE_PSPOLL)
		   ||(msdu_data->msg_type == TX_MNG_TYPE_NULLFRM)
		   ||(msdu_data->msg_type == TX_MNG_TYPE_PROBEREQ))
		{
			if(!_tx_submit_msdu(pAdap,msdu_data))
			{
				//insert msdu to que head
				sc_spin_lock(&tx_context->tx_list_lock);
				QueuePushHead(&tx_context->mng_frm_normal_que,&msdu_data->link);
				sc_spin_unlock(&tx_context->tx_list_lock);
			}
			return;
		}

		//insert msdu to que tail
		sc_spin_lock(&tx_context->tx_list_lock);
		QueuePutTail(&tx_context->mng_frm_normal_que,&msdu_data->link);
		sc_spin_unlock(&tx_context->tx_list_lock);
		msdu_data = NULL;
	}
	
   	//These case don't send data packet
	if(TRUE == pAdap->bStarveMac) 
	{
		return;
	}
    
	//send retry frame
	#ifdef NEW_RETRY_LIMIT
	{
	  	//Close ibss mode condition to make ibss support auto rate algrithm 
		if(	//(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA) &&		//in bss mode
		(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED)
		&&(tx_context->tx_retry_valid != 0))
		{
          
			sc_spin_lock(&tx_context->tx_list_lock);
			QueueGetCount(&tx_context->retry_frm_que, p_count);
            DBG_WLAN__TX(LEVEL_INFO,"[%s]:before Submit retry queue count is %d\n",
                        __FUNCTION__,
                        count); 
            sc_spin_unlock(&tx_context->tx_list_lock);
            
            //if index== count, it mean a null point will be get
			ASSERT(tx_context->tx_retry_index <= count);

            if(tx_context->int_before_urb)
            {
                QueueGetCount(&tx_context->retry_frm_que, p_count);
                DBG_WLAN__TX(LEVEL_INFO,"[%s]:int_before_urb happened,retry queue count is %d\n",
                        __FUNCTION__,
                        count);  
                tx_context->int_before_urb = FALSE;
            }

            //get the retry index msdu
            sc_spin_lock(&tx_context->tx_list_lock);
            //insert the block before the index to the tail
            pblock = QueueGetHead(&tx_context->retry_frm_que);
            for(i = 0;i <tx_context->tx_retry_index;i++)
            {
                if(pblock == NULL)
                    break;
                pblock = QueueGetNext(pblock);
            }
            sc_spin_unlock(&tx_context->tx_list_lock);
            
			if(pblock != NULL)
            {
				msdu_data = (PTX_MSDU_DATA)(pblock);
				if(_tx_submit_msdu(pAdap,msdu_data))
				{	
				    sc_spin_lock(&tx_context->tx_list_lock);
                    tx_context->tx_retry_index++;
                    sc_spin_unlock(&tx_context->tx_list_lock);
                    QueueGetCount(&tx_context->retry_frm_que, p_count);
                    DBG_WLAN__TX(LEVEL_INFO,"[%s]:Submit retry index = %d frame successful,queue count is %d!\n",
                        __FUNCTION__,
                        tx_context->tx_retry_index,
                        count);  
				}
                return;
           }
           else
           {
                //Retry queue null, so we clear retry flag
				tx_context->tx_retry_valid = 0;			
				tx_context->tx_retry_index = 0;
				DBG_WLAN__TX(LEVEL_INFO,"[%s]:Retry queue end run \n",__FUNCTION__);	
            
           }
               
        }
	}
#endif/*NEW_RETRY_LIMIT*/
	//here send packet queued in pkt queue
	sc_spin_lock(&tx_context->tx_list_lock);
	msdu_data = (PTX_MSDU_DATA)QueuePopHead(&tx_context->data_frm_que);
	sc_spin_unlock(&tx_context->tx_list_lock);
    if(NULL != msdu_data)
    {
        if(!_tx_submit_msdu(pAdap,msdu_data))
		{
			//insert msdu to que head
			sc_spin_lock(&tx_context->tx_list_lock);
			QueuePushHead(&tx_context->data_frm_que,&msdu_data->link);
			sc_spin_unlock(&tx_context->tx_list_lock);
		}
	}   
}	



/**********************************************************************
 *   Adap_Transmit
 *
 *   Descriptions:
 *	  transmit a packet through the adapter onto the medium.
 *   Arguments:
 *	  pAdap: IN,  the pointer to the adapter context.
 *	  Packet: IN, a pointer to a descriptor for the packet that is to be transmitted.
 *   Return Value: 
 *	  STATUS_SUCCESS if Packet is sent successfully and can be released
 *	  STATUS_PENDING if Packet is just pent and released later 
 *********************************************************************/
void
_tx_transmit_data_frm (
		   PDSP_ADAPTER_T pAdap, 
		   PTX_MSDU_DATA msdu_data, 
		   UINT8* data,
		   UINT32 data_len)
{
	PTX_CONTEXT tx_context;
	UINT32	  return_len;
	//Jakio added here
	UINT16	frag_len;
	UINT16	frag_count;
	//Jakio20070420
	PDSP_ATTR_T pwlan_attr = &pAdap->wlan_attr;
	PDATA_HEAD_FORMAT_T  phead;
    //DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s]\n",__FUNCTION__);
	ASSERT(pAdap);
	frag_len = pAdap->wlan_attr.frag_threshold;

	ASSERT(data_len!=0 && data_len < MAX_ETHER_LEN);

	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);

    //woody1224
	msdu_data->is_tx_1x_packet = tx_frame_Is_802_1x(data, (UINT16)data_len);
	msdu_data->txRate = pAdap->wlan_attr.rate;;
	
	//Jakio20070531: if encryption used, packets can be sent only when there is a key
	//execpt 802.1x data
	//DBGSTR_TX(("^^^^privacy_option= %d,  wpa_group_key_valid = %d ^^^^^\n ",
	//			pAdap->wlan_attr.gdevice_info.privacy_option,pAdap->wlan_attr.wpa_group_key_valid));	
	if((pAdap->wlan_attr.gdevice_info.privacy_option == TRUE) &&
		(pAdap->wlan_attr.wpa_group_key_valid == 0))
	{
		if(!msdu_data->is_tx_1x_packet)
		{

			DBG_WLAN__TX(LEVEL_ERR, "[%s]: dsp send fail due to wpa with no 1x frame\n",__FUNCTION__);
			_tx_release_msdu(tx_context, msdu_data);
			return;
		}
		else
		{
			DBG_WLAN__TX(LEVEL_TRACE, "[%s]: An 8021x packet \n",__FUNCTION__);
			pAdap->wpa_1x_no_encrypt++;
		}	
	}

	//justin:	071226	//for tkip counter measurer
	if(msdu_data->is_tx_1x_packet)
	{
		pwlan_attr->start_8021x_flag = 1;
	}

	/*  jakio will finish the fragment procedure
		 include: 802.3 to 80211
		  here just handle data packet
	  */
	//caculate frag count
	ASSERT(frag_len);
	if (pAdap->wlan_attr.gdevice_info.ps_support != PSS_ACTIVE)
	{
	//	if(!Tx_frame_Is_802_1x(pcontext->TxCached, (UINT16)len)
	//		&& pwlan_attr->need_set_pwr_mng_bit_flag)
		if(pwlan_attr->need_set_pwr_mng_bit_flag)
		{
			//Vcmd_set_power_mng(pAdap);
			if(!Task_CheckExistTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_MNG_BIT))
					  Task_CreateTask((PDSP_TASK_T)pAdap->ppassive_task,DSP_TASK_EVENT_SET_POWER_MNG_BIT,DSP_TASK_PRI_NORMAL,NULL,0);
			
			pwlan_attr->need_set_pwr_mng_bit_flag = FALSE;
		}
	}

	_tx_fragment_procedure(pAdap,msdu_data,data,(UINT16)data_len,frag_len ,&return_len, &frag_count);

	if(frag_count > 1)
	{
		DBG_WLAN__TX(LEVEL_TRACE, "[%s]: fragcount = %d \n", __FUNCTION__,frag_count);
	}

	return_len = (return_len + 3) & 0xfffffffc;	//Justin: DWORD align
	
#ifdef SET_TX_FRAG_WITH_DMA2
	//pcontext->length =  returnlen + 4;

	if(frag_count > 1)
	{
	   DBG_WLAN__TX(LEVEL_TRACE, "[%s]: fragcount = %d,frag len is %d,data len is %d,return len = %d\n",
					__FUNCTION__,
					frag_count,
					frag_len,
					data_len,
					return_len);
	}
	
	msdu_data->payload_len=  return_len + 4*frag_count;

#else

	msdu_data->payload_len = return_len;

#endif

	//build head packet
	msdu_data->head_len = sizeof(DATA_HEAD_FORMAT_T);
	phead = (PDATA_HEAD_FORMAT_T)msdu_data->head_buff;
	phead->pad = 0;
	phead->fifo_num = _3DSP_TX_FIFO_IDX_CP;
	phead->fragcount = (UINT8)(frag_count & 0xff);
	phead->total_len = (UINT16)(return_len & 0xffff);
	//phead->type = 1;//DSP_USB_TX_DATA_MLME_PACKET;	//Justin: only 1 can be recognize
#ifdef DSP_SINGLE_MODE_FOR_HEAD
{
	UINT16 tmp;
	tmp = (UINT16)(phead->total_len % 1024);
	//phead->fifo_num = (UINT8)(tmp /4);
	phead->dword_len= (UINT8)(tmp /4);
	if(phead->total_len >= 1024)
	{
		//phead->fifo_num+=1;
		phead->dword_len+=1;
		phead->sub_type = 1;		//indicate length of packet > 1024
		
	}
	else
	{
		phead->sub_type = 0;
	}
}
#endif
	sc_spin_lock(&tx_context->tx_list_lock);
	QueuePutTail(&tx_context->data_frm_que, &msdu_data->link);
	sc_spin_unlock(&tx_context->tx_list_lock);
	sc_tasklet_schedule(&tx_context->tx_tasklet);
	
}

VOID	Tx_get_counts(	PDSP_ADAPTER_T pAdap, 
PUINT32		p_good_cnt,
PUINT32		p_good_payload_cnt,
PUINT32		p_error_cnt)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);   
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	if(NULL == tx_context)
    {
        DBG_WLAN__TX(LEVEL_ERR, "[%s]: tx_context is null,maybe has been released\n",
					__FUNCTION__);
        
        return;
    } 
	if(NULL != p_good_cnt)
	{
		*p_good_cnt = tx_context->tx_good_cnt;
	}
	if(NULL != p_good_payload_cnt)
	{
		*p_good_payload_cnt = tx_context->tx_good_payload_cnt;
	}
	if(NULL != p_error_cnt)
	{
		*p_error_cnt = tx_context->tx_error_cnt;
	}
}

BOOLEAN tx_is_congested(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	 //Must put this judge setence here, otherwise hlat function will not be called.
	if(!Adap_Driver_isTxWork(pAdap) || (pAdap->link_ok != LINK_OK) )
	{
		return TRUE;
	}

#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status != NO_RECONNECT)	//justin: 080403. return while doing reconnect
	{
		return TRUE;
	}
#endif   
	if(tx_context->tx_msdu_pool.node_free == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

TDSP_STATUS Tx_Init(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_data;
    UINT8 i;
	DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
	tx_data = (PTX_CONTEXT)sc_memory_alloc(sizeof(TX_CONTEXT));
	if(NULL == tx_data)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	sc_memory_set(tx_data, 0, sizeof(TX_CONTEXT));
	tx_data->tx_msdu_pool.pool_type = TX_MSDU_DATA_POOL;
	_tx_msdu_pool_init(&(tx_data->tx_msdu_pool), 
					  MSDU_ALLOC_NUM, 
					  DSP_TX_HEAD_BUF_SIZE,
					  MSDU_ALLOC_SIZE);

	tx_data->tx_mmpdu_pool.pool_type = TX_MSDU_MNG_POOL;
	_tx_msdu_pool_init(&(tx_data->tx_mmpdu_pool), 
					  MMPDU_ALLOC_NUM, 
					  DSP_TX_HEAD_BUF_SIZE,
					  MMPDU_ALLOC_SIZE);
 #if 1   
    _tx_msdu_pool_init(&(tx_data->tx_buffer_pool), 
					  4, 
					  DSP_TX_HEAD_BUF_SIZE,
					  MSDU_ALLOC_SIZE);
#endif

    //INIT skb free list
	QueueInitList(&tx_data->tx_skb_free_list);
 	for (i = 0; i < TX_SKB_BUF_NUM; i++)
    {   
		QueuePutTail(&tx_data->tx_skb_free_list,&tx_data->tx_skb_buffer[i].link);		
	}
    
    sc_tasklet_init( &tx_data->tx_tasklet, 
				 _tx_routine, 
				 (ULONG)pAdap);
	/* Init list */
	QueueInitList(&tx_data->data_frm_que);
	QueueInitList(&tx_data->mng_frm_high_que);
	QueueInitList(&tx_data->mng_frm_normal_que);
  #ifdef NEW_RETRY_LIMIT
	QueueInitList(&tx_data->retry_frm_que);
  #endif
    sc_spin_lock_init(&tx_data->tx_list_lock);
	wlan_timer_init(&tx_data->tx_watching_timer, _tx_watch_timeout, (PVOID)pAdap);
	tx_data->pAdap = pAdap;
	pAdap->tx_context = tx_data;
	
	return STATUS_SUCCESS;
}

VOID Tx_Watch_TimeOut_Routine(PDSP_ADAPTER_T pAdap)
{
	TDSP_STATUS status;
	UINT8 uTemp[8];
    //UINT8 txbusy;
	UINT8 buff[0x200];
    //UINT32 val;
    BOOLEAN body_returned = TRUE;
    BOOLEAN head_returend = TRUE;
    PTX_CONTEXT tx_context = NULL;
	PTX_MSDU_DATA current_msdu = NULL;
	DSP_DRIVER_STATE_T			preState = pAdap->driver_state;
	PUINT8		pChar0x30 = &(buff[0x30]), pChar0x100 = &(buff[0x100]);
	
	
	DBG_WLAN__TX(LEVEL_TRACE,"Enter [%s]: \n",__FUNCTION__);

	tx_context = pAdap->tx_context;
	ASSERT(tx_context);

    tx_context->is_time_out = TRUE;
  
	current_msdu = tx_context->sending_msdu;
    if(NULL != current_msdu)
	{

        if(current_msdu->Tx_head_irp_in_working)
        {
    		DBG_WLAN__TX(LEVEL_TRACE,"* * * * * Adap_TxWatchTimeout_Routine: head irp in working \n");
        }
    	if(current_msdu->Tx_data_irp_in_working)
        {
    		DBG_WLAN__TX(LEVEL_TRACE,"* * * * * Adap_TxWatchTimeout_Routine: data irp in working \n");
        }
        //_tx_print_msdu( current_msdu);
    	Basic_ReadBurstRegs(pAdap, buff, 0x180, 0x00);//256 + 128
    	Adap_PrintBuffer(buff, 0x180);//256 + 128
#if 0
        _tx_print_buffer_msdu(tx_context);
        val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__BBREG_0_3);   
        DBG_WLAN__TX(LEVEL_TRACE,"[%s]:WLS_MAC__BBREG_0_3 is %08X\n",__FUNCTION__,val);
        val = VcmdR_3DSP_Dword(pAdap, WLS_MAC__TX_FRAG_CNT);   
        DBG_WLAN__TX(LEVEL_TRACE,"[%s]:WLS_MAC__TX_FRAG_CNT is %08X\n",__FUNCTION__,val);
#endif        
        //if Tx time out happened, It will fail for driver to access dsp register.So do not access dsp here.
        //but we can read or write USB register here.
        sc_memory_set(uTemp,0,8);

    	if((*(pChar0x100 + 3) != 0) || (*(pChar0x100 + 7) != 0))
        {//Head information still in USB register.
    		if((*(pChar0x30 + 1)) & 0x01)
    		{//0x31 bit 0 , equals to 1, means tx dma is on going.
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]:tx is on going, dma may busy! \n",__FUNCTION__);
    		}
    		else
    		{
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]:tx is not on going, dma not busy! \n",__FUNCTION__);
    		}
            //first stop Tx.
    	    pAdap->driver_state = DSP_STOP_TX;
    	    //then recycle the tx irps.(head & data)
    	    //-- begin --
    	    // Wait for all of the Tx IRPs to be completed
            if(current_msdu->Tx_head_irp_in_working)
    	    {
               if(!UsbDev_WaitAsyRequestIdle((PUSB_CONTEXT)pAdap->usb_context,5))
               {
                    UsbDev_CancelAsyRequest((PUSB_CONTEXT)pAdap->usb_context);
                    if(!UsbDev_WaitAsyRequestIdle((PUSB_CONTEXT)pAdap->usb_context,5))
                    {
                        DBG_WLAN__TX(LEVEL_ERR," [%s]:cant wait asy request return! \n",__FUNCTION__);
                    }
                     head_returend = FALSE;
               }
               else
               {
                    head_returend = TRUE;
               }
              
    	    }

            if(current_msdu->Tx_data_irp_in_working)
            {
                if(!UsbDev_WaitBulkOutIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                {
                    UsbDev_CancelBulkOutTransfer((PUSB_CONTEXT)pAdap->usb_context);
                    if(!UsbDev_WaitBulkOutIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                    {
                        DBG_WLAN__TX(LEVEL_ERR," [%s]:cant wait bulk out return! \n",__FUNCTION__);
                    }
                    body_returned = FALSE;
                }
                else
                {
                    body_returned = TRUE;
                }
            }
            
            if(head_returend & body_returned)
            {
                DBG_WLAN__TX(LEVEL_TRACE," [%s]: wait body and head returned,do nothing!\n",__FUNCTION__);
                pAdap->driver_state = preState;
                wlan_timer_stop(&tx_context->tx_watching_timer);
                return;
            }

            //-- end --

            //third, clear each USB registers.
    		status = Basic_WriteBurstRegs(pAdap,(PUINT8)&uTemp,8,0x100);
    		{
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: write tx 0x100 failure! status = %x  \n",__FUNCTION__,status);
    		}
    		uTemp[0] = 0xE2;//ohters are ZERO.
    		status = Basic_WriteBurstRegs(pAdap,(PUINT8)&uTemp,8,0x30);
    		{
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: write tx 0x30 failure! status = %x  \n",__FUNCTION__,status);
    		}
    		uTemp[0] = 0x0;//ohters are ZERO.
    		status = Basic_WriteBurstRegs(pAdap,(PUINT8)&uTemp,8,0x38);
    		{
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: write tx 0x38 failure! status = %x  \n",__FUNCTION__,status);
    		}
            
    		//forth. read an DSP register, if read OK, means the send procedure can be continue. or the error was not recovered.
    		{//This may happen vendor command busy event.
    			ULONG val;
    			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
    			//val &= 0xff;
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: 11,40b0: reg = %x \n",__FUNCTION__,val );
    		}
    		//fifth. Continue Tx
    		pAdap->driver_state = preState;
        }
    		
        else
        {//may be tx busy in 0x30 register.
    		if((*(pChar0x30 + 1)) & 0x01)
    		{//0x31 bit 0 , equals to 1, means tx dma is on going.
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: tx is on going, dma may busy!I can do nothing, pls restart your PC! \n",__FUNCTION__);
    		}
    		else
    		{//Head = 0, and dma not busy.
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]: tx is not on going, head = 0, dma not busy. cancel the data urb  \n",__FUNCTION__);
    			//first stop Tx.
    			//pAdap->driver_state = DSP_STOP_TX;
    			//then recycle the tx irps.(head & data)
    			//-- begin --
    			// Wait for all of the Tx IRPs to be completed
    			    // Wait for all of the Tx IRPs to be completed
                 if(current_msdu->Tx_head_irp_in_working)
        	     {
                       if(!UsbDev_WaitAsyRequestIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                       {
                            UsbDev_CancelAsyRequest((PUSB_CONTEXT)pAdap->usb_context);
                            if(!UsbDev_WaitAsyRequestIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                            {
                                DBG_WLAN__TX(LEVEL_ERR," [%s]:cant wait asy request return! \n",__FUNCTION__);
                            }
                       }
                       else
                       {
                            head_returend = TRUE;
                       }
                  
    	        }

                if(current_msdu->Tx_data_irp_in_working)
                {
                    if(!UsbDev_WaitBulkOutIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                    {
                        UsbDev_CancelBulkOutTransfer((PUSB_CONTEXT)pAdap->usb_context);
                        if(!UsbDev_WaitBulkOutIdle((PUSB_CONTEXT)pAdap->usb_context,500))
                        {
                            DBG_WLAN__TX(LEVEL_ERR," [%s]:cant wait bulk out return! \n",__FUNCTION__);
                        }
                    }
                    else
                    {
                        body_returned = TRUE;
                    }
                }
            
                if(head_returend & body_returned)
                {
                    DBG_WLAN__TX(LEVEL_TRACE," [%s]: wait body and head returned,do nothing!\n",__FUNCTION__);
                    pAdap->driver_state = preState;
                    wlan_timer_stop(&tx_context->tx_watching_timer);
                    return;
                }
    			//-- end --
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]:All tx urb was canceled  \n",__FUNCTION__);
    			Basic_ReadBurstRegs(pAdap, buff, 0x180, 0x00);//256 + 128
    			Adap_PrintBuffer(buff, 0x180);//256 + 128
    			//forth. read an DSP register, if read OK, means the send procedure can be continue. or the error was not recovered.
    			{//This may happen vendor command busy event.
    				ULONG val;
    				val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
    				//val &= 0xff;
    				DBG_WLAN__TX(LEVEL_TRACE," [%s]:22,40b0: reg = %x \n",__FUNCTION__,val);
    			}
    			//fifth. Continue Tx
    			//pAdap->driver_state = preState;
    		}
    		//forth. read an DSP register, if read OK, means the send procedure can be continue. or the error was not recovered.
    		{//This may happen vendor command busy event.
    			ULONG val;
    			val = VcmdR_3DSP_Dword(pAdap,WLS_MAC__TX_FRAG_CNT);
    			//val &= 0xff;
    			DBG_WLAN__TX(LEVEL_TRACE," [%s]:40b0: reg = %x \n",__FUNCTION__,val);
    		}
        }
    }
}


TDSP_STATUS
Tx_Get_FW_Fragment (
		   PDSP_ADAPTER_T pAdap, 
		   PUINT8		   pbuff,
		   UINT32		   len
			)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);   
	tx_context = (PTX_CONTEXT)pAdap->tx_context;

	ASSERT(tx_context);
	//copy mng frame from temp buffer of mng to context buffer
	sc_memory_set(pbuff,0,len);

	//need not allocate msdu,just call usb interface
	return  UsbDev_BuildBulkInTransfer((PUSB_CONTEXT)pAdap->usb_context,
										   (PVOID)pbuff,
										   (UINT32)len,
											_tx_fw_completion,
											(PVOID)NULL);

}


TDSP_STATUS
Tx_Transmit_FW_Fragment (
		   PDSP_ADAPTER_T pAdap, 
		   PUINT8		   pbuff,
		   UINT32		   len
			)
{
	TDSP_STATUS status;
	PTX_CONTEXT  tx_context;
	PTX_MSDU_DATA msdu_data;

	ASSERT(len!=0 && len <= DSP_TX_BUF_SIZE);

	//update it later
	//1 request irp for this packet
	//2 request tx context for this packet
	//if fail, push the pakcet into pkt queue then return pending 
	tx_context = pAdap->tx_context;
	ASSERT(tx_context);
	msdu_data = _tx_alloc_msdu(pAdap,&tx_context->tx_msdu_pool, len);

   
	if(NULL == msdu_data)
	{

		DBG_WLAN__TX(LEVEL_ERR, "[%s]: can not alloc msdu for !\n",__FUNCTION__);
		return STATUS_INSUFFICIENT_RESOURCES;
	}	
	
    // TODO:Jakie
    //msdu_data->msdu_time_stamp = tdsp_get_current_time();
	msdu_data->frm_type = TX_FW_FRM;
	msdu_data->payload_len = len;
	
	//copy mng frame from temp buffer of mng to context buffer
	sc_memory_copy(msdu_data->payload_buff,pbuff,len);

	status = UsbDev_BuildBulkOutTransfer((PUSB_CONTEXT)pAdap->usb_context,
										   (PVOID)msdu_data->payload_buff,
										   (UINT32)len,
											_tx_fw_completion,
											(PVOID)msdu_data);
	if(STATUS_SUCCESS != status)
	{
		DBG_WLAN__TX(LEVEL_ERR, "[%s]: submit bulk out transer failed!\n",__FUNCTION__);
		   //return msdu to pool
		_tx_release_msdu(tx_context,msdu_data); 
	}
	return status;
	
}

BOOLEAN Tx_Transmit_mng_frame (
		   PDSP_ADAPTER_T pAdap, 
		   UINT8*			pbuff,
		   UINT32		   len,
		   TX_MNG_FRM_PRIORITY priority,
		   TX_MNG_FRM_TYPE	 type)
{
	PDATA_HEAD_FORMAT_T  phead;
	PTX_CONTEXT tx_context ;
	PTX_MSDU_DATA msdu_data;
	UINT16	seqNum;
	UINT16   sub_type;	
	UINT16   tmp;
	UINT32	 pad;
	PUSBWLAN_HW_FRMHDR_TX  pHwHdr = (PUSBWLAN_HW_FRMHDR_TX)pbuff;
	PMNG_STRU_T		pt_mng = pAdap->pmanagment;
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	//woody debug
	if(!Adap_Driver_isWork(pAdap))
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]:fail with no work state\n",__FUNCTION__);
		return FALSE;
	}

	DBG_WLAN__TX(LEVEL_INFO,"[%s]:msg type is %d,priority is %d\n",
				__FUNCTION__,
				type,
				priority);

	
	pad = len;
	len = (len + 3) & 0xfffc;
	pad = len - pad;
	
	
	//update it later
	//1 //request irp for this packet
	//2 //request tx context for this packet
	//if fail, push the pakcet into pkt queue then return pending 

	msdu_data = _tx_alloc_msdu(pAdap,&tx_context->tx_mmpdu_pool, len);
   
	if(NULL == msdu_data)
	{

		DBG_WLAN__TX(LEVEL_ERR, "[%s]: can not alloc msdu for mng frame!\n",__FUNCTION__);
		return FALSE;
		
	}

	ASSERT(len!=0 && len < 1600);

	//msdu_data->msdu_time_stamp = tdsp_get_current_time();
	msdu_data->msg_type = type;
	
	msdu_data->frm_type = (priority == TX_MNG_FRM_PRIORITY_HIGH) ? TX_MNG_FRM_HIGH : TX_MNG_FRM_NORMAL;
	//Here we should add the sequence number to framectrl field
	//for ps-poll frame, no squence number exists
	sub_type = WLAN_GET_FC_FSTYPE(pHwHdr->frmctrl_fragdur >> 16);
	if(sub_type != WLAN_FSTYPE_PSPOLL)
	{
		seqNum = get_current_seqnum(pAdap) << 4;
		pHwHdr->seqctrl_addr3hi |= seqNum << _3DSP_HWHDR_POS_SEQ_CTRL;
	}
	
	//copy mng frame from temp buffer of mng to context buffer
	sc_memory_copy(msdu_data->payload_buff,pbuff,len);
#ifdef SET_TX_FRAG_WITH_DMA2
	//if(pt_mng->is_probe)
    if(type == TX_MNG_TYPE_PROBEREQ)
    {
		//pt_mng->is_probe = FALSE;
		msdu_data->payload_len= len + 4*SEND_PROBE_TIMES;
	}
	else
	{
		msdu_data->payload_len = len + 4;
	}
#else
		msdu_data->payload_len = len;
#endif

	//build head
	msdu_data->head_len = sizeof(DATA_HEAD_FORMAT_T);
	phead = (PDATA_HEAD_FORMAT_T)msdu_data->head_buff;
	phead->pad = 0;
	phead->fifo_num = DOT11_FRM_IS_BEACON(pHwHdr->frmctrl_fragdur >> 16) ?
			_3DSP_TX_FIFO_IDX_BCN : _3DSP_TX_FIFO_IDX_CP;	

	//if(pt_mng->is_probe)
    if(type == TX_MNG_TYPE_PROBEREQ)
    {
		pt_mng->is_probe = FALSE;
		phead->fragcount = SEND_PROBE_TIMES;
	}


	phead->total_len = (UINT16)len;
	//phead->type = 1;//DSP_USB_TX_DATA_MLME_PACKET;		//Justin......
#ifdef DSP_SINGLE_MODE_FOR_HEAD
	if(phead->fifo_num  != _3DSP_TX_FIFO_IDX_BCN)
	{
		//no beacon frame
		tmp = (UINT16)(phead->total_len % 1024);
		//phead->fifo_num = (UINT8)(tmp /4);
		phead->dword_len = (UINT8)(tmp /4);

		if(phead->total_len >= 1024)
		{
			//phead->fifo_num += 1;
			phead->dword_len += 1;
			phead->sub_type = 1;
		}
		else
		{
			phead->sub_type = 0;
		}
	}
	else
	{
		//beacon frame
		//Assume beacon size never longer than 1024 bytes
		ASSERT((phead->total_len < 1024));
		// sub_type indication the packet should be put into first 4 byts. for beacon frame wil always be put here.
		phead->sub_type = 0;
		phead->pad = (UINT8)pad;

		//beacon frame
		tmp = (UINT16)(phead->total_len % 1024);
		phead->dword_len = (UINT8)(tmp /4);
		//phead->total_len =(UINT16)pad;	//?????
	}
#endif	
	if(TX_MNG_FRM_PRIORITY_HIGH == priority)
	{
	    sc_spin_lock(&tx_context->tx_list_lock);
		QueuePutTail(&tx_context->mng_frm_high_que, &msdu_data->link);
        sc_spin_unlock(&tx_context->tx_list_lock);
    }
	else
	{
	    sc_spin_lock(&tx_context->tx_list_lock);
		QueuePutTail(&tx_context->mng_frm_normal_que, &msdu_data->link);
        sc_spin_unlock(&tx_context->tx_list_lock);
    }
	sc_tasklet_schedule(&tx_context->tx_tasklet);
	return TRUE;
}


void  Tx_Release(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_data;
	//UINT32 count;
	//UINT32 i;
	//PTX_MSDU_DATA cur_msdu;
	//PUINT32 p_count = &count;
	ASSERT(pAdap);
	DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
	tx_data = (PTX_CONTEXT)pAdap->tx_context;
	if(NULL == tx_data)
		return;

	#if 0
	//cancel all working urbs
	QueueGetCount(&tx_data->sending_msdu_list, p_count);
	for(i =0 ; i < count; ++i)
	{
		cur_msdu = (PTX_MSDU_DATA)QueuePopHead(&tx_data->sending_msdu_list);
		if(NULL == cur_msdu)
		{   
			break;
		}
		if((cur_msdu->Tx_head_irp_in_working) && (!UsbDev_CancelBulkOutTransfer(pAdap->usb_context)))
		{
			 DBG_WLAN__TX(LEVEL_ERR, "[%s]: cancel BulkOutTransfer failed!\n",__FUNCTION__);	   
		}
		if((cur_msdu->Tx_data_irp_in_working) && (!UsbDev_CancelBulkOutTransfer(pAdap->usb_context)))
		{
			DBG_WLAN__TX(LEVEL_ERR, "[%s]: cancel BulkOutTransfer failed!\n",__FUNCTION__);
	   
		}
	}
  #endif
	sc_tasklet_kill(&(tx_data->tx_tasklet));
	_tx_msdu_pool_release(&(tx_data->tx_msdu_pool));
	_tx_msdu_pool_release(&(tx_data->tx_mmpdu_pool));
    #if 1
    _tx_msdu_pool_release(&(tx_data->tx_buffer_pool));
    #endif
	wlan_timer_kill(&(tx_data->tx_watching_timer));
	sc_spin_lock_kill(&tx_data->tx_list_lock);
	sc_memory_free(tx_data);
	pAdap->tx_context = NULL;
    DBG_WLAN__TX(LEVEL_TRACE, "Leave [%s] \n",__FUNCTION__); 
	
}


void  Tx_Reset(PDSP_ADAPTER_T pAdap)
{

	PTX_CONTEXT tx_data;
	PTX_MSDU_DATA msdu_data = NULL;
	ASSERT(pAdap);
	DBG_WLAN__TX(LEVEL_TRACE, "Enter [%s] \n",__FUNCTION__); 
	tx_data = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_data);

    //stop timer
	wlan_timer_stop(&(tx_data->tx_watching_timer));
    if(tx_data->is_time_out)
    {
        tx_data->is_time_out = FALSE;
    }
    #if 1
	//cancel all working urbs
	if(NULL != tx_data->sending_msdu)
	{   
		 msdu_data = tx_data->sending_msdu;
	
    	if((msdu_data->Tx_data_irp_in_working) && (!UsbDev_CancelBulkOutTransfer(pAdap->usb_context)))
    	{
    		 DBG_WLAN__TX(LEVEL_ERR, "[%s]: cancel BulkOutTransfer failed!\n",__FUNCTION__);	   
    	}
    	if((msdu_data->Tx_head_irp_in_working) && (!UsbDev_CancelAsyRequest(pAdap->usb_context)))
    	{
    		DBG_WLAN__TX(LEVEL_ERR, "[%s]: cancel head Transfer failed!\n",__FUNCTION__);   
    	}
    }
    #endif
	//empty data,mng and retry queue
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->data_frm_que);
		if(NULL == msdu_data)
		{
			break;
		}
		_tx_release_msdu(tx_data, msdu_data);
	}
#ifdef   NEW_RETRY_LIMIT
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->retry_frm_que);
		if(NULL == msdu_data)
		{
			break;
		}
		_tx_release_msdu(tx_data, msdu_data);
	}
#endif
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->mng_frm_high_que);
		if(NULL == msdu_data)
		{
			break;
		}
		_tx_release_msdu(tx_data, msdu_data);
	}
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->mng_frm_normal_que);
		if(NULL == msdu_data)
		{
			break;
		}
		_tx_release_msdu(tx_data, msdu_data);
	}
	
	/* Init list */
	QueueInitList(&tx_data->data_frm_que);
	QueueInitList(&tx_data->mng_frm_high_que);
	QueueInitList(&tx_data->mng_frm_normal_que);
#ifdef   NEW_RETRY_LIMIT
	QueueInitList(&tx_data->retry_frm_que);
#endif
	//rest flags
	tx_data->tx_good_cnt = 0;
	tx_data->tx_good_payload_cnt = 0;
	tx_data->tx_error_cnt = 0;

}


void  Tx_Cancel_Data_Frm(PDSP_ADAPTER_T pAdap)
{
	PTX_MSDU_DATA msdu_data;
	PTX_CONTEXT tx_data;
    UINT32 counter = 0;
	ASSERT(pAdap);
	DBG_WLAN__TX(LEVEL_INFO, "Enter [%s] \n",__FUNCTION__); 
	tx_data = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_data);

	if(QueueEmpty(&tx_data->data_frm_que))
	{
		return;
	}

	//empty data queue
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->data_frm_que);
		if(NULL == msdu_data)
		{
			break;
		}
		_tx_release_msdu(tx_data, msdu_data);
        counter ++;
	}
    /* Init data frame list */
	QueueInitList(&tx_data->data_frm_que);
    DBG_WLAN__TX(LEVEL_TRACE, "[%s]: calcel %d data frames!\n",__FUNCTION__,counter); 

#ifdef NEW_RETRY_LIMIT
  //empty retry queue
	while(1)	
	{
		msdu_data =(PTX_MSDU_DATA) QueuePopHead(&tx_data->retry_frm_que);
		if(NULL == msdu_data)
		{
			break;
		}
        msdu_data->frm_type= TX_DATA_FRM;  
		_tx_release_msdu(tx_data, msdu_data);
	}

    /* Init retry frame list */
	QueueInitList(&tx_data->retry_frm_que);
#endif

	//stop timer
	wlan_timer_stop(&(tx_data->tx_watching_timer));
    if(tx_data->is_time_out)
    {
        tx_data->is_time_out = FALSE;
    }
}

void Tx_Process_Retry_Int(PDSP_ADAPTER_T pAdap,UINT32 retry_num)
{
  	PTX_CONTEXT tx_context;  
#ifdef NEW_RETRY_LIMIT
    UINT8 i;
    UINT32	count;
	UINT32	retrycount;
    UINT32  returnedcount;
    ULONG flags;
	PTX_MSDU_DATA tx_msdu;
    PTX_MSDU_DATA release_retry_arr[DSP_RETRY_LIST_NUM];
	PUINT32 p_count = &count;
#endif
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	
#ifdef NEW_RETRY_LIMIT
	if(!Adap_Driver_isTxWork(pAdap))
	{
		DBG_WLAN__TX(LEVEL_TRACE,"Tx_Process_Retry_Int, tx stop, do not do retry procedure,TX count = %x,retry vaild = %x,link_ok = %d,driver_state = %d ! \n",
            retry_num,
            tx_context->tx_retry_valid,
            pAdap->link_ok , 
            pAdap->driver_state);
		return ;
	}

		if(	//(pAdap->wlan_attr.macmode != WLAN_MACMODE_IBSS_STA) &&		//in bss mode
			//(pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE) && 
			(pAdap->wlan_attr.hasjoined == JOIN_HASJOINED))
		{
			DBG_WLAN__TX(LEVEL_INFO,"[%s] :*TX limit reach ,Retry count = %x,retry vaild = %x,retry index = %x\n",
							__FUNCTION__,		
							retry_num,
							tx_context->tx_retry_valid,
							tx_context->tx_retry_index);
            DBG_WLAN__TX(LEVEL_INFO,"[%s] : Retry limit happen wlan_attr rate = %x\n",
                        __FUNCTION__,
                        pAdap->wlan_attr.rate);         
			//set the flag to notify tx transmit should down current retry packet rate
			pAdap->Retrylimit_Reach = 1;

			//plus 1 because maybe one packet is on usb bus instead of tx fifo
			returnedcount = retry_num;

			//tx fifo max num is 4,make sure the return is valid
			if(returnedcount > MAX_TX_PACKETS_NUM)
			{
				returnedcount = MAX_TX_PACKETS_NUM;
			}

			if((tx_context->tx_retry_index == 0)
				&&(tx_context->tx_retry_valid == 1))
			{
				//retry limit happen,and retry queue has run. but retry index = 0.
				//do nothing about retry queue
				//this case cause by last data packet in bus after retry limit happen
				DBG_WLAN__TX(LEVEL_TRACE,"[%s] :*It is retry special case,maybe last data packet retry\n",__FUNCTION__);

				//run auto rate
			}
			else 
			{
				
				//retry index !=0 case
				if(tx_context->tx_retry_valid == 1)
				{				
                 
                    tx_context->tx_retry_index = (returnedcount > tx_context->tx_retry_index) ? 
					              0 : (tx_context->tx_retry_index - returnedcount);
					
					//Only change speed for auto rate
					if(pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE)
					{	
     				    //Rate_Save_UsingRate(pAdap);
     				    //Rate_down_Directly(pAdap,0);
					}
                    
					sc_tasklet_schedule(&tx_context->tx_tasklet);
                   
                }
				//retry valid = 0
				else          
				{
					//save using rate when retry queue first work
					sc_spin_lock_irqsave(&tx_context->tx_list_lock,flags);
					QueueGetCount(&tx_context->retry_frm_que, p_count);
                    sc_spin_unlock_irqrestore(&tx_context->tx_list_lock,flags);
					retrycount = (returnedcount > count) ? 
								   count : returnedcount;
                    //there has a msdu urb not returned
                    if(tx_context->tx_busy_sending)
                    {
                        tx_context->int_before_urb = TRUE;
                        DBG_WLAN__TX(LEVEL_INFO,"[%s] Recv retry before all urb returned\n",__FUNCTION__);
	
                    }
                    //remove packet sent successfully,keep the queue size as the interrupt required
                    for(i = count; i > retrycount; i--)
					{
					    sc_spin_lock_irqsave(&tx_context->tx_list_lock,flags);
						tx_msdu = (PTX_MSDU_DATA)QueuePopHead(&tx_context->retry_frm_que);
                        sc_spin_unlock_irqrestore(&tx_context->tx_list_lock,flags);
						if(tx_msdu != NULL)
						{
							//set type to data packet for release this pcontext
							 tx_msdu->frm_type =  TX_DATA_FRM;
							 _tx_release_msdu(tx_context,tx_msdu);
						}	
						else
						{
							DBG_WLAN__TX(LEVEL_TRACE,"[%s] :TX retry queue has except \n",__FUNCTION__);
							break;
						}
					}
                    
                    sc_spin_lock_irqsave(&tx_context->tx_list_lock,flags);
                    QueueGetCount(&tx_context->retry_frm_que, p_count);
                    sc_spin_unlock_irqrestore(&tx_context->tx_list_lock,flags);

                    //delete the retry frame which exceed retry max num
                    sc_memory_set(release_retry_arr, 0, DSP_RETRY_LIST_NUM * sizeof(PTX_MSDU_DATA));
                    for(i = 0 ; i < count ; i++)
                    {
                        tx_msdu =(PTX_MSDU_DATA)QueueGetHead(&tx_context->retry_frm_que);
                        if(NULL == tx_msdu)
                        {
                            DBG_WLAN__TX(LEVEL_TRACE,"[%s] :TX retry queue except ,count = %d ,i =%d \n",__FUNCTION__,count,i);
							break;
                        }
                        if(tx_msdu->retry_count >= MSDU_RETRY_MAX)
                        {
                             //set type to data packet for release this pcontext
                             release_retry_arr[i] = tx_msdu;   
                             QueueRemoveHead(&tx_context->retry_frm_que);
                        }
                        else
                        {
                            break;
                        }
                    }
                             
                    for(i = 0; i < DSP_RETRY_LIST_NUM; i++)
                    {
                         if(NULL != (tx_msdu = release_retry_arr[i]))
                         {
                            tx_msdu->frm_type =  TX_DATA_FRM;
						    _tx_release_msdu(tx_context,tx_msdu);
                         }
                    }

                    sc_spin_lock_irqsave(&tx_context->tx_list_lock,flags);
                    QueueGetCount(&tx_context->retry_frm_que, p_count);	
                    sc_spin_unlock_irqrestore(&tx_context->tx_list_lock,flags);
                    if(count >0)
                    {
                        DBG_WLAN__TX(LEVEL_INFO,"[%s] :Retry queue begin run, and retry count = %x\n",__FUNCTION__,count);
					tx_context->tx_retry_index = 0;
					tx_context->tx_retry_valid = 1;
                    }
					//send next packet	
					if(pAdap->wlan_attr.fallback_rate_to_use == FALLBACK_RATE_USE)
					{
						//Rate_Save_UsingRate(pAdap);
						//Rate_down_Directly(pAdap,0);
					}
					sc_tasklet_schedule(&tx_context->tx_tasklet);
				}
			}	
		}	
#endif
}

UINT8 Adap_Tx_Available(void * adapt)
{
    PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T )adapt;
    if(adapt == NULL)
    {
        DBG_WLAN__TX(LEVEL_ERR, "%s: %d:adapt is null!\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if(( tx_is_congested(pAdap)) 
       || ( pAdap->wlan_attr.hasjoined != JOIN_HASJOINED )
       || ( pAdap->reconnect_status == DOING_RECONNECT ))
    {
        return FALSE;
    }

    return TRUE;
}

// 
INT32 Adap_Send(void* adapt, UINT8* data, UINT32 data_len)
{
    PTX_MSDU_DATA msdu_data;
    PTX_SKB_DATA  skb_data;
	PTX_CONTEXT tx_context;
	UINT8 *data_p;
	PDSP_ADAPTER_T pAdap = (PDSP_ADAPTER_T )adapt;
    
	DBG_WLAN__TX(LEVEL_INFO, "%s: %d: Enter\n", __FUNCTION__, __LINE__);

	ASSERT(data_len!=0 && data_len < MAX_ETHER_LEN);
	
	//Must put this judge setence here, otherwise hlat function will not be called.
	if(!Adap_Driver_isTxWork(pAdap) || (pAdap->link_ok != LINK_OK) )
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]: fail --- link_ok = %X, halt = %d, hw_reset_flag =%d \n",
					 __FUNCTION__,
					(pAdap->link_ok!=LINK_OK)?0:1,
					Adap_Driver_isHalt(pAdap), 
					(pAdap->driver_state == DSP_HARDWARE_RESET)); 
		return FALSE;
	}

#ifdef ROAMING_SUPPORT
	if(pAdap->reconnect_status != NO_RECONNECT)	//justin: 080403. return while doing reconnect
	{
		DBG_WLAN__TX(LEVEL_ERR,"[%s]: reconnect status is %d\n",
							__FUNCTION__,
							 pAdap->reconnect_status);
		return FALSE;
	}
#endif

	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
    
    sc_spin_lock_bh(&tx_context->tx_list_lock);
    skb_data = (PTX_SKB_DATA)QueuePopHead(&tx_context->tx_skb_free_list);
    sc_spin_unlock_bh(&tx_context->tx_list_lock);  

    if(skb_data == NULL)
    {
        DBG_WLAN__TX(LEVEL_ERR,"[%s]: tx_skb_free_list is empty\n",__FUNCTION__);
        return FALSE;
    }
    
    msdu_data = _tx_alloc_msdu(pAdap,&tx_context->tx_msdu_pool, data_len);
	if(msdu_data == NULL)
	{
		DBG_WLAN__TX(LEVEL_ERR, "%s: %d: Alloc tx_msdu failed!!!\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

    //copy skb data to skb_data buffer
    sc_memory_set(skb_data->skb_buffer,0,MAX_ETHER_LEN);
    sc_memory_copy(skb_data->skb_buffer, data, data_len);
    
    //msdu_data->msdu_time_stamp = tdsp_get_current_time();
	msdu_data->frm_type = TX_DATA_FRM;
	msdu_data->retry_count = 0;
	data_p = skb_data->skb_buffer;
	//save the dest_addr
	sc_memory_copy( msdu_data->dest_addr, data_p , 6);
	data_p+=6;

	// save the src addr
	sc_memory_copy( msdu_data->src_addr, data_p , 6);
	data_p+=6;

	_tx_transmit_data_frm(pAdap, msdu_data,skb_data->skb_buffer, data_len);

    sc_spin_lock_bh(&tx_context->tx_list_lock);
    QueuePutTail(&tx_context->tx_skb_free_list,&skb_data->link);
    sc_spin_unlock_bh(&tx_context->tx_list_lock);  
	// Deserialized drivers always return pending.
	return TRUE;
}

BOOLEAN Tx_DataFrmlist_IsEmpty(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	return QueueEmpty(&tx_context->data_frm_que);
}


BOOLEAN Tx_MngFrmlist_IsEmpty(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
 
	if(QueueEmpty(&tx_context->mng_frm_high_que) && QueueEmpty(&tx_context->mng_frm_normal_que))
	{
		return TRUE;
	}
	return FALSE;
}


VOID Tx_Send_Next_Frm(PDSP_ADAPTER_T pAdap)
{
	PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	if( 1 == sc_netq_ifstop( pAdap->net_dev))
	{
		sc_netq_start(pAdap->net_dev);
	}
	sc_tasklet_schedule(&tx_context->tx_tasklet);
}

void Tx_Stop(PDSP_ADAPTER_T pAdap)
{
    PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
    if(!tx_context->tx_stop)
    {
        tx_context->tx_stop = TRUE;
        //need to cancel the watch timer
        wlan_timer_stop(&tx_context->tx_watching_timer);
        if(tx_context->is_time_out)
        {
            tx_context->is_time_out = FALSE;
        }
        sc_tasklet_disable(&tx_context->tx_tasklet);
    }
    
}

void Tx_Restart(PDSP_ADAPTER_T pAdap)
{
    PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
    if(tx_context->tx_stop)
    {
        tx_context->tx_stop = FALSE;
        sc_tasklet_enable(&tx_context->tx_tasklet);
        sc_tasklet_schedule(&tx_context->tx_tasklet);
    }
}

VOID Tx_Reset_Retry_Queue(PDSP_ADAPTER_T pAdap)
{
#ifdef   NEW_RETRY_LIMIT
    PTX_CONTEXT tx_context;
	ASSERT(pAdap);
	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	ASSERT(tx_context);
	sc_spin_lock(&tx_context->tx_list_lock);
	tx_context->tx_retry_valid  = 0;
	sc_spin_unlock(&tx_context->tx_list_lock);
#endif
}


VOID Tx_Print_Status(PVOID adapt)
{    
    PTX_CONTEXT tx_context;
    UINT32	count;
    PUINT32 p_count = &count;
    PDSP_ADAPTER_T    pAdap = (PDSP_ADAPTER_T)adapt;
    PTX_MSDU_DATA current_msdu = NULL;
    if(pAdap == NULL)
    {
        sc_print("[%s] adapt poiter is null,maybe released!\n",__FUNCTION__);
        return;
    }

	tx_context = (PTX_CONTEXT)pAdap->tx_context;
	if(tx_context == NULL)
    {
        sc_print("[%s] tx context is null,maybe released!\n",__FUNCTION__);
        return;
    }

    if( NULL == pAdap->net_dev)
    {
         sc_print("[%s] net_dev is null!\n",__FUNCTION__);
    }
    else if(1 == sc_netq_ifstop(pAdap->net_dev))
    {
			sc_print("OS tx switch:     stopped!!!\n");
    }
    else
    {
			sc_print("OS tx switch:     start!!!\n");
    }
    sc_print("tx msdu pool free code is %d\n",tx_context->tx_msdu_pool.node_free);
    sc_print("tx mmpdu pool free code is %d\n",tx_context->tx_mmpdu_pool.node_free);

    //print data queue count
    sc_spin_lock(&tx_context->tx_list_lock);
    QueueGetCount(&tx_context->data_frm_que, p_count);	
    sc_spin_unlock(&tx_context->tx_list_lock);
    sc_print("tx data queue count is %d\n",count);

    //print retry queuen count
#ifdef   NEW_RETRY_LIMIT
    sc_spin_lock(&tx_context->tx_list_lock);
    QueueGetCount(&tx_context->retry_frm_que, p_count);	
    sc_spin_unlock(&tx_context->tx_list_lock);
    sc_print("tx_retry_queue count is %d\n",count);
    sc_print("tx_retry_valid is %d\n",tx_context->tx_retry_valid);
    sc_print("tx_retry_index is %d\n",tx_context->tx_retry_index);
#endif
    //if msdu is timeout,print the msdu
    if(tx_context->is_time_out)
    {
        sc_print("tx is being timeout\n");
        current_msdu = tx_context->sending_msdu;
        if(NULL != current_msdu)
	    {
            if(current_msdu->Tx_head_irp_in_working)
    		    sc_print("head irp in working \n");

    	    if(current_msdu->Tx_data_irp_in_working)
    		    sc_print("data irp in working \n");
         _tx_print_msdu( current_msdu);
        }
    }               
    else
    {
        
    }
}
