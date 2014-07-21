
#include "usbwlan_syscall.h"
#include "tdsp_debug.h"

static char* TDSP_FILE_INDICATOR="USBSC";
SYS_CALL* g_syscall;

VOID syscall_set(volatile PSYS_CALL sc)
{
    g_syscall = sc;
    #if 0
    g_syscall.memory_alloc     = sc->memory_alloc;
    g_syscall.memory_set       = sc->memory_set;
    g_syscall.memory_free      = sc->memory_free;
    g_syscall.memory_copy      = sc->memory_copy;
    g_syscall.memory_cmp       = sc->memory_cmp;
    g_syscall.event_init       = sc->event_init;
    g_syscall.event_set        = sc->event_set;
    g_syscall.event_reset      = sc->event_reset;
    g_syscall.event_wait       = sc->event_wait;
    g_syscall.event_kill       = sc->event_kill;
    g_syscall.spin_lock_init   = sc->spin_lock_init;
    g_syscall.spin_lock        = sc->spin_lock;
    g_syscall.spin_unlock      = sc->spin_unlock;
    g_syscall.spin_lock_irqsave= sc->spin_lock_irqsave;
    g_syscall.spin_unlock_irqrestore= sc->spin_unlock_irqrestore;
    g_syscall.spin_lock_bh     = sc->spin_lock_bh;
    g_syscall.spin_unlock_bh   = sc->spin_unlock_bh;
   
    g_syscall.spin_lock_kill   = sc->spin_lock_kill;
    g_syscall.tasklet_init     = sc->tasklet_init;
    g_syscall.tasklet_schedule = sc->tasklet_schedule;
    g_syscall.tasklet_kill     = sc->tasklet_kill;
    g_syscall.tasklet_enable   = sc->tasklet_enable;
    g_syscall.tasklet_disable  = sc->tasklet_disable;
    g_syscall.worklet_init     = sc->worklet_init;
    g_syscall.worklet_running  = sc->worklet_running;
    g_syscall.worklet_schedule = sc->worklet_schedule;
    g_syscall.worklet_kill     = sc->worklet_kill;
   
    g_syscall.timer_init       = sc->timer_init;
    g_syscall.timer_start      = sc->timer_start;
    g_syscall.timer_stop       = sc->timer_stop;
    g_syscall.timer_reset      = sc->timer_reset;
    g_syscall.timer_kill       = sc->timer_kill;
    g_syscall.sleep            = sc->sleep;  
    g_syscall.ms_to_ticks      = sc->ms_to_ticks;
    g_syscall.get_random_byte  = sc->get_random_byte;
    g_syscall.get_random_dword = sc->get_random_dword;
    g_syscall.time_downout_duration = sc->time_downout_duration;
    
    g_syscall.urb_alloc        = sc->urb_alloc;
    g_syscall.urb_free         = sc->urb_free;
    g_syscall.urb_unlink       = sc->urb_unlink;
    g_syscall.urb_kill         = sc->urb_kill;
    g_syscall.urb_getlen       = sc->urb_getlen;
    g_syscall.urb_getstatus    = sc->urb_getstatus;
    g_syscall.urb_getcntxt     = sc->urb_getcntxt;
    g_syscall.usb_issrmv       = sc->usb_issrmv;
    g_syscall.usb_alloc_ctrlreq = sc->usb_alloc_ctrlreq;
    g_syscall.usb_set_ctrlreq   = sc->usb_set_ctrlreq;
    g_syscall.usb_free_ctrlreq  = sc->usb_free_ctrlreq;
    g_syscall.usb_ctrlmsg      = sc->usb_ctrlmsg;
    g_syscall.usb_ctrlreq      = sc->usb_ctrlreq;
    g_syscall.usb_rcvint       = sc->usb_rcvint;
    g_syscall.usb_bulkin       = sc->usb_bulkin;
    g_syscall.usb_bulkout      = sc->usb_bulkout;

    g_syscall.skb_alloc        = sc->skb_alloc;
    g_syscall.skb_free         = sc->skb_free;
    g_syscall.skb_sbmt         = sc->skb_sbmt;
    g_syscall.netq_ifstop      = sc->netq_ifstop;
    g_syscall.netq_start       = sc->netq_start;
    g_syscall.netq_stop        = sc->netq_stop;
    g_syscall.bus_set_fw_ver   = sc->bus_set_fw_ver;
    g_syscall.bus_get_hkey_flag = sc->bus_get_hkey_flag;
    g_syscall.bus_get_antenna_flag = sc->bus_get_antenna_flag; 
 
    g_syscall.print            = sc->print;
    #endif
    DBG_WLAN__ENTRY(LEVEL_INFO,"[%s]:sc's is %p,syscall's is %p\n",
                        __FUNCTION__,
                        sc->spin_lock_init,
                        g_syscall->spin_lock_init);  

}

inline VOID sc_spin_lock_init(PTDSP_SPINLOCK lock) 
{
   DBG_WLAN__ENTRY(LEVEL_INFO,"Enter %s\n",__FUNCTION__);
   DBG_WLAN__ENTRY(LEVEL_INFO,"spin_lock_init is %p\n",g_syscall->spin_lock_init);  
   (*(g_syscall->spin_lock_init))(lock);
}

inline VOID sc_spin_lock(PTDSP_SPINLOCK lock)
{         
    (*(g_syscall->spin_lock))(lock);
}

inline VOID sc_spin_lock_bh(PTDSP_SPINLOCK lock)
{         
    (*(g_syscall->spin_lock_bh))(lock);
}

inline VOID sc_spin_unlock(PTDSP_SPINLOCK lock) 
{ 
    (*(g_syscall->spin_unlock))(lock);
}

inline VOID sc_spin_unlock_bh(PTDSP_SPINLOCK lock) 
{ 
    (*(g_syscall->spin_unlock_bh))(lock);
}

inline VOID sc_spin_unlock_irqrestore(PTDSP_SPINLOCK lock,UINT32 flags) 
{ 
    (*(g_syscall->spin_unlock_irqrestore))(lock,flags);
}

inline VOID sc_spin_lock_kill(PTDSP_SPINLOCK lock) 
{ 
    (*(g_syscall->spin_lock_kill))(lock);
}

inline PVOID sc_memory_alloc(UINT32 required_size) 
{ 
    return (*(g_syscall->memory_alloc))(required_size);
}

inline VOID sc_memory_set(PVOID p, UINT8 value, UINT32 len) 
{ 
    (*(g_syscall->memory_set))(p,value,len);
}

inline PVOID  sc_memory_vmalloc(unsigned long size)
{
	return (*(g_syscall->memory_vmalloc))(size);
}
inline VOID sc_memory_vfree(PVOID  p)
{
	(*(g_syscall->memory_vfree))(p);
}

inline INT32 sc_memory_cmp(const PVOID dest, const PVOID src, UINT32 len) 
{ 
    return (*(g_syscall->memory_cmp))(dest,src,len);
}
inline VOID sc_memory_free(PVOID p) 
{ 
    (*(g_syscall->memory_free))(p);
}

inline VOID sc_memory_copy(PVOID dest,const PVOID src,UINT32 len) 
{ 
    (*(g_syscall->memory_copy))(dest,src,len);
}
inline VOID sc_event_init(PTDSP_EVENT event)
{
    (*(g_syscall->event_init))(event);
}

inline VOID sc_event_set(PTDSP_EVENT event)
{
    (*(g_syscall->event_set))(event);
}

inline VOID sc_event_reset(PTDSP_EVENT event)
{
    (*(g_syscall->event_reset))(event);
}

inline INT32 sc_event_wait(PTDSP_EVENT event,UINT32 timeout)
{
    return (*(g_syscall->event_wait))(event,timeout);
}

inline VOID sc_event_kill(PTDSP_EVENT event)
{
    (*(g_syscall->event_kill))(event);
}
inline VOID sc_tasklet_init(PTDSP_TASKLET t,VOID (*func)(ULONG), ULONG data)
{
    (*(g_syscall->tasklet_init))(t,func,data);
}

inline VOID sc_tasklet_schedule(PTDSP_TASKLET t)
{
    (*(g_syscall->tasklet_schedule))(t);
}

inline VOID sc_tasklet_enable(PTDSP_TASKLET t)
{
    (*(g_syscall->tasklet_enable))(t);
}

inline VOID sc_tasklet_disable(PTDSP_TASKLET t)
{
    (*(g_syscall->tasklet_disable))(t);
}

inline VOID sc_tasklet_kill(PTDSP_TASKLET t)
{
    (*(g_syscall->tasklet_kill))(t);
}



inline VOID sc_worklet_init(PTDSP_WORKLET w, VOID (*func)(ULONG),ULONG para)
{
    (*(g_syscall->worklet_init))(w,func,para);
}

inline VOID sc_worklet_kill(PTDSP_WORKLET w)
{
    (*(g_syscall->worklet_kill))(w);
}

inline VOID sc_worklet_schedule(PTDSP_WORKLET w)
{
    (*(g_syscall->worklet_schedule))(w);
}

inline UINT8 sc_worklet_running(PTDSP_WORKLET w)
{
     return (*(g_syscall->worklet_running))(w);
}

inline VOID sc_timer_init(PTDSP_TIMER t, VOID (*func)(ULONG),ULONG para)
{
    (*(g_syscall->timer_init))(t,func,para);
}

inline VOID sc_timer_stop(PTDSP_TIMER t)
{
    (*(g_syscall->timer_stop))(t);
}

inline VOID sc_timer_start(PTDSP_TIMER t, UINT32 sec)
{
    (*(g_syscall->timer_start))(t,sec);
}

inline VOID sc_timer_reset(PTDSP_TIMER t, UINT32 sec)
{
    (*(g_syscall->timer_reset))(t,sec);
}

inline VOID sc_timer_kill(PTDSP_TIMER t)
{
    (*(g_syscall->timer_kill))(t);
}


VOID sc_sleep(UINT32 delay_ms)
{
    (*(g_syscall->sleep))(delay_ms);
}

unsigned long long sc_ms_to_ticks(UINT32 ms)
{
     return (*(g_syscall->ms_to_ticks))(ms);
}


UINT8	sc_get_random_byte(VOID)
{
    return (*(g_syscall->get_random_byte))();
}

UINT32	sc_get_random_dword(VOID)
{
     return (*(g_syscall->get_random_dword))();
}


UINT8 sc_time_downout_duration(VOID)
{
   return (*(g_syscall->time_downout_duration))();
}

inline  PVOID sc_urb_alloc(VOID)
{
     return (*(g_syscall->urb_alloc))();

}

inline VOID sc_urb_free(PVOID urb)
{
     (*(g_syscall->urb_free))(urb);

}

inline VOID sc_urb_unlink(PVOID urb)
{
     (*(g_syscall->urb_unlink))(urb);

}


inline VOID sc_urb_kill(PVOID urb)
{
     (*(g_syscall->urb_kill))(urb);

}


inline INT32 sc_urb_getstatus(PVOID urb)
{
    return (*(g_syscall->urb_getstatus))(urb);

}

inline PVOID  sc_urb_getcntxt(PVOID urb)
{
    return (*(g_syscall->urb_getcntxt))(urb);

}

inline INT32 sc_usb_issrmv(INT32 status)
{
    return (*(g_syscall->usb_issrmv))(status);

}
inline INT32 sc_urb_getlen(PVOID urb)
{
   return (*(g_syscall->urb_getlen))(urb);
}

inline PVOID sc_usb_alloc_ctrlreq(void)
{
    return (*(g_syscall->usb_alloc_ctrlreq))();
}

inline VOID sc_usb_set_ctlrreq(void * ctrl_req,void * req)
{
    (*(g_syscall->usb_set_ctrlreq))(ctrl_req,req);
}

inline VOID sc_usb_free_ctrlreq(PVOID * set_up)
{
     (*(g_syscall->usb_free_ctrlreq))(set_up);
}

inline INT32 sc_usb_ctrlmsg(PVOID usbdev,
                                 UINT8 ep,
                                 UINT8 type,
                                 PVOID buffer,
                                 INT32 len,
         					     PVOID request)
{
   return (*(g_syscall->usb_ctrlmsg))(usbdev,ep,type,
                                    buffer,len,request);

}
inline INT32 sc_usb_ctrlreq(PVOID urb,
                                  PVOID usbdev,
                                  UINT8 ep,
                                  PVOID buffer,
                                  INT32 len,
                                  PVOID context,
                                  PVOID request)
{
    return  (*(g_syscall->usb_ctrlreq))(urb,usbdev,ep,buffer,
                                    len,context,request);
}

inline INT32 sc_usb_rcvint(PVOID urb,
                         PVOID usbdev,
                         UINT8 ep,
                         PVOID buffer,
                         INT32 len,
                         PVOID context,
                         INT32 interval)
{
    return  (*(g_syscall->usb_rcvint))(urb,usbdev,ep,buffer,
                                    len,context,interval);
}

inline INT32 sc_usb_bulkin(PVOID urb,
                                 PVOID usbdev,
                                 UINT8 ep,
                                 PVOID buffer,
                                 INT32 len,
                                 PVOID context)
{
    return  (*(g_syscall->usb_bulkin))(urb,usbdev,ep,buffer,
                                    len,context);
}
inline INT32 sc_usb_bulkout(PVOID urb,
                                 PVOID usbdev,
                                 UINT8 ep,
                                 PVOID buffer,
                                 INT32 len,
                                 PVOID context)
{
    return  (*(g_syscall->usb_bulkout))(urb,usbdev,ep,buffer,
                                    len,context);
}

inline PVOID sc_skb_alloc(UINT32 buf_len,
                          PVOID* para)
{
    return  (*(g_syscall->skb_alloc))(buf_len,para);
}

inline VOID sc_skb_sbmt(PVOID net_dev, 
                        PVOID pkt_buff,
                        UINT32 buff_len)
{
    (*(g_syscall->skb_sbmt))(net_dev,pkt_buff,buff_len);
}

inline VOID sc_skb_free(PVOID pkt_buff)
{
    (*(g_syscall->skb_free))(pkt_buff);

}

inline INT32 sc_netq_ifstop(PVOID net_dev)
{
    return (*(g_syscall->netq_ifstop))(net_dev);

}

inline VOID sc_netq_start(PVOID net_dev)
{
    (*(g_syscall->netq_start))(net_dev);
}

inline VOID sc_netq_stop(PVOID net_dev)
{
    (*(g_syscall->netq_stop))(net_dev);
}

inline void sc_bus_set_fw_ver(void *pIntf, ULONG c8051_version, ULONG dsp_version)
{
    (*(g_syscall->bus_set_fw_ver))(pIntf,c8051_version,dsp_version);
}

inline UINT8 sc_bus_get_hkey_flag(void *pIntf)
{
    DBG_WLAN__INIT(LEVEL_TRACE,"[%s:] bus info is %x \n",__FUNCTION__,pIntf);
    return (*(g_syscall->bus_get_hkey_flag))(pIntf);
}

inline UINT8 sc_bus_get_antenna_flag(void *pIntf)
{
    return (*(g_syscall->bus_get_antenna_flag))(pIntf);
}

inline void sc_iw_send_bssid_to_up(void *dev, unsigned char *bssid)
{
	(*(g_syscall->iw_send_bssid_to_up))(dev, bssid);
}

inline void sc_wlan_netdev_setup(void* adapt)
{
	(*(g_syscall->wlan_netdev_setup))(adapt);
}

inline void sc_wlan_netdev_destroy(void* adapt)
{
	(*(g_syscall->wlan_netdev_destroy))(adapt);
}

inline void* sc_openfile(const char* filename)
{
	return ((*(g_syscall->openfile))(filename));
}
inline void sc_closefile(void* file_p)
{
	(*(g_syscall->closefile))(file_p);
}
inline int sc_open_success(void* file_p)
{
	return (*(g_syscall->open_success))(file_p);
}
inline unsigned long sc_readfile(void* file_p, void* buffer, unsigned long buff_len, unsigned long offset)
{
	return (*(g_syscall->readfile))(file_p,buffer,buff_len,offset);
}