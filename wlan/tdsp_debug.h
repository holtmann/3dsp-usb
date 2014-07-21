/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:

 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************
 *
 * [debug.h] description:
 * 	Provide debug levle and debug module support.
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


/*
 * REVISION HISTORY:
 *
 * 	Rev/D/A:		1.0.0	2007/02/09	daniel
 *	Description:	first draft!
 *
 */


#ifndef _TDSP_VKI__DEBUG_H_
#define _TDSP_VKI__DEBUG_H_

#include <linux/kernel.h>

#include "usbwlan_define.h"
#include "tdsp_basetypes.h"
#include "usbwlan_syscall.h"


extern SYS_CALL* g_syscall;

#define  sc_print  (*(g_syscall->print))
/****************************************************************
 *																* 
 *		 		M3DSP DRIVER DEBUG MACROS						*
 * 																*
 ***************************************************************/



//Define driver debug level

#define LEVEL_ASSERT		0
#define LEVEL_ERR			1
#define LEVEL_CRIT			2	//key info
#define LEVEL_TRACE			3
#define LEVEL_INFO			4




#ifdef DEBUG_OPEN__WLAN
//#undef DEBUG_OPEN		//remove the debug information
// This macro indicate if we support dsp source code level debug!!!
#define DEBUG__DSP_SOURCE_CODE_LVL

extern char*	level_chars[];


//Define 3dsp's sc_print function, which will be called in module debug macros.
#define M3DSP_PRINTK(level, arg...)	do{ \
										sc_print("3D[%s:%s:%05d]",level_chars[level],TDSP_FILE_INDICATOR,__LINE__);\
										sc_print(arg);\
									}while(0)
									
#define M3DSP_HPRINTK(level, arg...)	do{ \
										sc_print("3D[%s:%s:%05d]",level_chars[level],__FUNCTION__,__LINE__);\
										sc_print(arg);\
									}while(0)
#else									
#define M3DSP_PRINTK(level, arg...)	
#define M3DSP_HPRINTK(level, arg...)
#endif


/**************************************************************************** 
 *																			*
 *						debug interface for WLAN							*
 *																			*
 ***************************************************************************/

#ifdef DEBUG_OPEN__WLAN

//Globle vars which is controlled by ioctl.
extern UINT32 g_wlan_dbg_module;
extern UINT8 g_wlan_dbg_level;



//define driver modules which needs debuging

#define MOD_WLAN__DOT11				(1 << 0) 	
#define MOD_WLAN__HEADER 				(1 << 1) 
#define MOD_WLAN__2					(1 << 2)	
#define MOD_WLAN__3					(1 << 3) 
#define MOD_WLAN__4					(1 << 4)
#define MOD_WLAN__5					(1 << 5)
#define MOD_WLAN__6					(1 << 6)
#define MOD_WLAN__7					(1 << 7)
											// separate line
#define MOD_WLAN__8					(1 << 8) 	
#define MOD_WLAN__IOCTL	 			(1 << 9) 
#define MOD_WLAN__INIT				(1 << 10)	
#define MOD_WLAN__11				(1 << 11) 
#define MOD_WLAN__12				(1 << 12)
#define MOD_WLAN__HAL				(1 << 13)
#define MOD_WLAN__USB				(1 << 14)
#define MOD_WLAN__NETDEV			(1 << 15)
											// separate line
#define MOD_WLAN__ENTRY				(1 << 16) 	
#define MOD_WLAN__RX	 			(1 << 17) 
#define MOD_WLAN__MAC_CORE			(1 << 18)
#define MOD_WLAN__MLME				(1 << 19)
#define MOD_WLAN__SME				(1 << 20)	
#define MOD_WLAN__MDOZE				(1 << 21)
#define MOD_WLAN__ISR				(1 << 22)	
#define MOD_WLAN_TIMER				(1 << 23)
											// separate line
#define MOD_WLAN__LQUEUE			(1 << 24)
#define MOD_WLAN__TX				(1 << 25) 
#define MOD_WLAN__SIMU	 			(1 << 26)	
#define MOD_WLAN__UTIL	 			(1 << 26)
#define MOD_WLAN__WLS				(1 << 27) 
#define MOD_WLAN__TASK				(1 << 28) //add for usb
#define MOD_WLAN__MAIN				(1 << 29)
#define MOD_WLAN__30				(1 << 30)
#define MOD_WLAN__31				(1 << 31)	

void tdsp_dbgmsg_print_buff(INT8* comments, volatile UINT8* buff,UINT32  len,char *file_name,UINT32 line_number);

#define DBG_PRINT_BUFF(comments,buff, len)  tdsp_dbgmsg_print_buff(comments,(PUINT8)(buff),(UINT32)(len),TDSP_FILE_INDICATOR,__LINE__) 
#define DBG_HPRINT_BUFF(comments,buff, len)  tdsp_dbgmsg_print_buff(comments,(PUINT8)(buff),(UINT32)(len),__FUNCTION__,__LINE__) 
	

#define DBG_TRACE() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_PRINTK(LEVEL_TRACE, "TRACE\n"); \
	}	\
}while(0)

#define DBG_ENTER() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_PRINTK(LEVEL_TRACE, ">>ENTER [%s] >>\n",__FUNCTION__); \
	}	\
}while(0)

#define DBG_EXIT() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_PRINTK(LEVEL_TRACE, "<<EXIT [%s] <<\n",__FUNCTION__); \
	}	\
}while(0)


#define DBG_HTRACE() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_HPRINTK(LEVEL_TRACE, "TRACE\n"); \
	}	\
}while(0)

#define DBG_HENTER() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_HPRINTK(LEVEL_TRACE, ">>ENTER [%s] >>\n",__FUNCTION__); \
	}	\
}while(0)

#define DBG_HEXIT() do{ 	\
	if( g_wlan_dbg_level >= LEVEL_TRACE){ \
			M3DSP_HPRINTK(LEVEL_TRACE, "<<EXIT [%s] <<\n",__FUNCTION__); \
	}	\
}while(0)


#define DBG_WLAN__HEADER(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__HEADER) == MOD_WLAN__HEADER) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_HPRINTK(level, arg); \
	}	\
}while(0)



#define DBG_WLAN__DOT11(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__DOT11) == MOD_WLAN__DOT11) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)


#define DBG_WLAN__IOCTL(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__IOCTL) == MOD_WLAN__IOCTL) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)

#define DBG_WLAN__INIT(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__INIT) == MOD_WLAN__INIT) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)



#define DBG_WLAN__USB(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__USB) == MOD_WLAN__USB) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)



#define DBG_WLAN__NETDEV(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__NETDEV) == MOD_WLAN__NETDEV) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)



#define DBG_WLAN__ENTRY(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__ENTRY) == MOD_WLAN__ENTRY) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)


#define DBG_WLAN__RX(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__RX) == MOD_WLAN__RX) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)


#define DBG_WLAN__MAC_CORE(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__MAC_CORE) == MOD_WLAN__MAC_CORE) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)

	
#define DBG_WLAN__MLME(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__MLME) == MOD_WLAN__MLME) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)


#define DBG_WLAN__SME(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__SME) == MOD_WLAN__SME) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)

	
#define DBG_WLAN__MDOZE(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__MDOZE) == MOD_WLAN__MDOZE) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)


#define DBG_WLAN__INT(level, arg...) do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__ISR) == MOD_WLAN__ISR) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
}while(0)



#define DBG_WLAN__TIMER(level, arg...) do{ 	\
if( ((g_wlan_dbg_module & MOD_WLAN_TIMER) == MOD_WLAN_TIMER) && \
	(g_wlan_dbg_level >= level)){ \
		M3DSP_PRINTK(level, arg); \
}	\
}while(0)

#define DBG_WLAN__TX(level, arg...)	do{ 	\
if( ((g_wlan_dbg_module & MOD_WLAN__TX) == MOD_WLAN__TX) && \
	(g_wlan_dbg_level >= level)){ \
		M3DSP_PRINTK(level, arg); \
}	\
}while(0)  

#define DBG_WLAN__SIMU(level, arg...)	do{ 	\
if( ((g_wlan_dbg_module & MOD_WLAN__SIMU) == MOD_WLAN__SIMU) && \
	(g_wlan_dbg_level >= level)){ \
		M3DSP_PRINTK(level, arg); \
}	\
}while(0)


#define DBG_WLAN__UTIL(level, arg...)	do{ 	\
		if( ((g_wlan_dbg_module & MOD_WLAN__UTIL) == MOD_WLAN__UTIL) && \
			(g_wlan_dbg_level >= level)){ \
				M3DSP_PRINTK(level, arg); \
		}	\
		}while(0)


#define DBG_WLAN__WLS(level, arg...)	do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__WLS) == MOD_WLAN__WLS) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
	}while(0)


#define DBG_WLAN__TASK(level, arg...)	do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__TASK) == MOD_WLAN__TASK) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
	}while(0)	

#define DBG_WLAN__MAIN(level, arg...)	do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__MAIN) == MOD_WLAN__MAIN) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
	}while(0)


#define DBG_WLAN__HAL(level, arg...)	do{ 	\
	if( ((g_wlan_dbg_module & MOD_WLAN__HAL) == MOD_WLAN__HAL) && \
		(g_wlan_dbg_level >= level)){ \
			M3DSP_PRINTK(level, arg); \
	}	\
	}while(0)

#else

//1 USE THESE MACRO IN HEADER FILE
#define DBG_HTRACE() 
#define DBG_HENTER() 
#define DBG_HEXIT() 
#define DBG_WLAN__HEADER(level, arg...) 
#define DBG_HPRINT_BUFF(comments,buff, len)
//1 END


#define DBG_PRINT_BUFF(comments,buff, len)

#define DBG_TRACE()
#define DBG_ENTER()
#define DBG_EXIT()


#define DBG_WLAN__DOT11(level, arg...)

#define DBG_WLAN__IOCTL(level, arg...)
#define DBG_WLAN__INIT(level, arg...)


#define DBG_WLAN__USB(level, arg...)
#define DBG_WLAN__NETDEV(level, arg...)
#define DBG_WLAN__ENTRY(level, arg...) 
#define DBG_WLAN__RX(level, arg...) 
#define DBG_WLAN__MAC_CORE(level, arg...) 
#define DBG_WLAN__MLME(level, arg...) 
#define DBG_WLAN__SME(level, arg...) 
#define DBG_WLAN__MDOZE(level, arg...) 
#define DBG_WLAN__INT(level, arg...) 
#define DBG_WLAN__TIMER(level, arg...) 
#define DBG_WLAN__TX(level, arg...)
#define DBG_WLAN__SIMU(level, arg...)
#define DBG_WLAN__UTIL(level, arg...)
#define DBG_WLAN__WLS(level, arg...)
#define DBG_WLAN__TASK(level, arg...)
#define DBG_WLAN__MAIN(level, arg...)
#define DBG_WLAN__HAL(level, arg...)
#endif	//End DEBUG_OPEN__WLAN



#ifdef DEBUG_OPEN
#define	kd_assert(_x)												\
		{															\
			if (!(_x))												\
			{														\
				M3DSP_PRINTK(LEVEL_ASSERT,"Assertion failed.\n");		\
				M3DSP_PRINTK(LEVEL_ASSERT,# _x);					\
				sc_print("\n");											\
			}														\
		}


#else

#define kd_assert(_x)

#endif


#define ASSERT(_x)            kd_assert(_x)


void tdsp_set_debug(void);


#endif//_TDSP_VKI__DEBUG_H_
