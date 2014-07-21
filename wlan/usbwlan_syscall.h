#ifndef _USBWLAN_SYS_CALL_H_
#define _USBWLAN_SYS_CALL_H_

#include "usbwlan_lib.h"
#include "tdsp_basetypes.h"

VOID syscall_set(PSYS_CALL sc);


VOID sc_spin_lock_init(PTDSP_SPINLOCK lock);
VOID sc_spin_lock(PTDSP_SPINLOCK lock);
VOID sc_spin_lock_bh(PTDSP_SPINLOCK lock);

#define sc_spin_lock_irqsave(lock, flags)    (*(g_syscall->spin_lock_irqsave))(lock, &flags)

VOID sc_spin_unlock(PTDSP_SPINLOCK lock); 
VOID sc_spin_unlock_bh(PTDSP_SPINLOCK lock); 
VOID sc_spin_unlock_irqrestore(PTDSP_SPINLOCK lock,
                                    UINT32 flags); 
VOID sc_spin_lock_kill(PTDSP_SPINLOCK lock); 

PVOID sc_memory_alloc(UINT32 required_size);
VOID sc_memory_set(PVOID p, UINT8 value, UINT32 len) ;

VOID sc_memory_free(PVOID p); 
VOID sc_memory_copy(PVOID dest,const PVOID src,UINT32 len); 
INT32 sc_memory_cmp(const PVOID dest, const PVOID src, UINT32 len);
PVOID sc_memory_vmalloc(unsigned long size);
VOID sc_memory_vfree(void* p);

VOID sc_event_init(PTDSP_EVENT event);

VOID sc_event_set(PTDSP_EVENT event);

VOID sc_event_reset(PTDSP_EVENT event);
INT32 sc_event_wait(PTDSP_EVENT event,UINT32 timeout);

VOID sc_event_kill(PTDSP_EVENT event);

VOID sc_tasklet_init(PTDSP_TASKLET t,VOID (*func)(ULONG), ULONG data);

VOID sc_tasklet_schedule(PTDSP_TASKLET t);

VOID sc_tasklet_enable(PTDSP_TASKLET t);

VOID sc_tasklet_disable(PTDSP_TASKLET t);

VOID sc_tasklet_kill(PTDSP_TASKLET t);

VOID sc_worklet_init(PTDSP_WORKLET w, VOID (*func)(ULONG),ULONG para);

VOID sc_worklet_kill(PTDSP_WORKLET w);

VOID sc_worklet_schedule(PTDSP_WORKLET w);

UINT8 sc_worklet_running(PTDSP_WORKLET w);

VOID sc_timer_init(PTDSP_TIMER t,VOID (*func)(ULONG),ULONG para);

VOID sc_timer_stop(PTDSP_TIMER t);

VOID sc_timer_start(PTDSP_TIMER t, UINT32 sec);

VOID sc_timer_reset(PTDSP_TIMER t, UINT32 sec);


VOID sc_timer_kill(PTDSP_TIMER t);

VOID sc_sleep(UINT32 delay_ms);

unsigned long long sc_ms_to_ticks(UINT32 ms);

UINT8	sc_get_random_byte(VOID);
UINT32	sc_get_random_dword(VOID);
UINT8  sc_time_downout_duration(VOID);

PVOID sc_urb_alloc(VOID);

VOID sc_urb_free(PVOID urb);
VOID sc_urb_unlink(PVOID urb);

VOID sc_urb_kill(PVOID urb);

INT32 sc_urb_getstatus(PVOID urb);

PVOID  sc_urb_getcntxt(PVOID urb);
PVOID sc_usb_alloc_ctrlreq(void);
VOID sc_usb_set_ctlrreq(void * ctrl_req,void* req);
VOID sc_usb_free_ctrlreq(PVOID * set_up);
INT32 sc_usb_issrmv(INT32 status);
INT32 sc_urb_getlen(PVOID urb);
INT32 sc_usb_ctrlmsg(PVOID  usbdev,
                         UINT8 ep,
                         UINT8 type,
                         PVOID buffer,
                         INT32 len,
 					     PVOID request);
INT32 sc_usb_ctrlreq(PVOID urb,
                      PVOID usbdev,
                      UINT8 ep,
                      PVOID buffer,
                      INT32 len,
                      PVOID context,
                      PVOID request);
INT32 sc_usb_rcvint(PVOID urb,
                         PVOID usbdev,
                         UINT8 ep,
                         PVOID buffer,
                         INT32 len,
                         PVOID context,
                         INT32 interval);
INT32 sc_usb_bulkin(PVOID urb,
                                 PVOID usbdev,
                                 UINT8 ep,
                                 PVOID buffer,
                                 INT32 len,
                                 PVOID context);
INT32 sc_usb_bulkout(PVOID urb,
                                 PVOID usbdev,
                                 UINT8 ep,
                                 PVOID buffer,
                                 INT32 len,
                                 PVOID context);


void* sc_skb_alloc(unsigned int buf_len,
                          void** para);

void sc_skb_sbmt(void* net_dev, 
                        void* pkt_buff,
                        unsigned int buff_len);
void sc_skb_free(void* pkt_buff);
int sc_netq_ifstop(void* net_dev);
void sc_netq_start(void* net_dev);
void sc_netq_stop(void* net_dev);

void sc_bus_set_fw_ver(void *pIntf, ULONG c8051_version, ULONG dsp_version);

UINT8 sc_bus_get_hkey_flag(void *pIntf);
UINT8 sc_bus_get_antenna_flag(void *pIntf);

void sc_iw_send_bssid_to_up(void *dev, unsigned char *bssid);
void sc_wlan_netdev_setup(void* adapt);
void sc_wlan_netdev_destroy(void* adapt);

void* sc_openfile(const char* filename);
void sc_closefile(void* file_p);
int sc_open_success(void* file_p);
unsigned long sc_readfile(void* file_p, void* buffer, unsigned long buff_len, unsigned long offset);



//VOID sc_print(const char *fmt,...);

#endif

