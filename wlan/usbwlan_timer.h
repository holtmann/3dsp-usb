#ifndef __USBWLAN_TIMER_H__
#define __USBWLAN_TIMER_H__

#include "tdsp_basetypes.h"
#include "usbwlan_syscall.h"

typedef VOID (*WLAN_TIMER_ROUTINE)(VOID *);


typedef enum _WLAN_TIMER_TYPE
{
	WLAN_TIMER_ONCE,
	WLAN_TIMER_PERIODIC
} WLAN_TIMER_TYPE,*PTDSP_TIMER_TYPE;




typedef struct  _WLAN_TIMER
{
	TDSP_TIMER phy_timer;		//timer obj will be registered to OS
	
	WLAN_TIMER_ROUTINE user_handler;	//timeout function
	VOID* user_data;					//timeout handler parameter
	
	UINT32 timeout;						//expire timeout, measured in ticks
	WLAN_TIMER_TYPE timer_type;			//timer type: once or periodic
	BOOLEAN  start;
}WLAN_TIMER, *PWLAN_TIMER;


VOID wlan_timer_start(PWLAN_TIMER timer, UINT32 timeout);
VOID wlan_period_timer_start(PWLAN_TIMER timer, UINT32 timeout);

VOID wlan_timer_stop(PWLAN_TIMER timer);


VOID wlan_timer_reset(PWLAN_TIMER timer, UINT32 timeout);


VOID _wlan_timer_kill(PWLAN_TIMER timer);

boolean	_wlan_timer_init(
	PWLAN_TIMER 		timer,
	WLAN_TIMER_ROUTINE	func,
	VOID*				para);



#define	wlan_timer_init( timer, func, para)	\
			_wlan_timer_init(timer, func, para)

#define	wlan_timer_kill(timer)	\
			_wlan_timer_kill(timer)

UINT8	wlan_get_random_byte(VOID);

UINT32	wlan_get_random_dword(VOID);

#endif
