
#include "usbwlan_timer.h"
#include "tdsp_debug.h"


static char* TDSP_FILE_INDICATOR="TIMER";

VOID timeout_handler(unsigned long data)
{
	PWLAN_TIMER timer = (PWLAN_TIMER)data;
	if(timer == NULL)
    { 
        DBG_WLAN__TIMER(LEVEL_ERR, "[%s]: input data is null\n",__FUNCTION__);
        return;
    }
    if(timer->user_handler == NULL)
    {
        DBG_WLAN__TIMER(LEVEL_ERR, "[%s]: user_handler of timer is null\n",__FUNCTION__);
        return;
    }
	timer->user_handler(timer->user_data);

	if (timer->timer_type == WLAN_TIMER_PERIODIC) 
    {
        sc_timer_reset(&(timer->phy_timer),timer->timeout);
	}

}


boolean	_wlan_timer_init(
	PWLAN_TIMER 		timer,
	WLAN_TIMER_ROUTINE	func,
	VOID*				para)
{
	
	//initialize physical timer[E]
	timer->user_handler = func;				//the real timeout handler
	timer->user_data = para;	//the real timeout parameter
    timer->start     = FALSE;

    //initialize physical timer[S]
    //assign the timeout parameter, NOTE, real timeout parameter is wrapped in it,
	//which type is unsigned long(we assign a pointer)
    sc_timer_init(&timer->phy_timer,timeout_handler,(ULONG)timer);
	
	return TRUE;
}


VOID _wlan_timer_start(PWLAN_TIMER timer, WLAN_TIMER_TYPE timer_type, UINT32 timeout)
{

    if(timer->start)
    {
         sc_timer_reset(&(timer->phy_timer), timeout);
    }
	//DBG_TRACE();
	//register tiemer to OS, OS will start it
    else
    {	
        timer->timer_type = timer_type;
	    timer->timeout = timeout;
	    sc_timer_start(&timer->phy_timer,timeout);
        timer->start = TRUE;
    }
	//DBG_TRACE();
}



/*
 * this function register timer to OS and start it. when timeout expired, fire the timeout handler
 * timeout: mesured in ticks
*/
VOID wlan_timer_start(PWLAN_TIMER timer, UINT32 timeout)
{
    _wlan_timer_start( timer, WLAN_TIMER_ONCE, timeout);
}

VOID wlan_period_timer_start(PWLAN_TIMER timer, UINT32 timeout)
{
    _wlan_timer_start( timer, WLAN_TIMER_PERIODIC, timeout);
}


VOID wlan_timer_stop(PWLAN_TIMER timer)
{
	//remove timer from OS, meanwhile stop it
	if(timer->start)
    {
        sc_timer_stop(&(timer->phy_timer));
        timer->start = FALSE;
    }
}


VOID wlan_timer_reset(PWLAN_TIMER timer, UINT32 timeout)
{
    sc_timer_reset(&(timer->phy_timer), timeout);
    timer->start = TRUE;
}


VOID _wlan_timer_kill(PWLAN_TIMER timer)
{
	//remove timer from OS, meanwhile stop it
	sc_timer_kill(&(timer->phy_timer));
    timer->start = FALSE;
}

