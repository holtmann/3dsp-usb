/************************************************************************
(c) 2003-07, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
	3DSP Corporation
	16271 Laguna Canyon Rd
	Irvine, CA 92618
	www.3dsp.com
**************************************************************************
$RCSfile: precomp.h,v $ 
$Revision: 1.2 $ 
$Date: 2010/08/16 01:52:19 $
**************************************************************************/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#if 0
#include "usbwlan_define.h"

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <net/iw_handler.h>
#include <linux/wireless.h>

#include "tdsp_basetypes.h"
#include "tdsp_endian.h"
#include "tdsp_debug.h"
#include "tdsp_string.h"
#include "tdsp_time.h"
#include "tdsp_timer.h"
#include "tdsp_mutex.h"
#include "tdsp_event.h"
#include "tdsp_tasklet.h"


#include "usbwlan_version.h"


#endif
#include "tdsp_basetypes.h"
#include "tdsp_debug.h"
#include "ndis_def50.h"
//#include "usbwlan_version.h"

struct _DSP_ADAPTER;

typedef struct _DSP_ADAPTER  DSP_ADAPTER_T, *PDSP_ADAPTER_T;


#include "usbwlan_baseband.h"
#include "usbwlan_charact.h"
#include "usbwlan_defs.h"
#include "usbwlan_equates.h"
#include "usbwlan_regvars.h"
#include "usbwlan_usbConfig.h"
#include "usbwlan_UsbRegDef.h"
#include "usbwlan_wlan.h"

#include "usbwlan_MacConfig.h"

#include "usbwlan_queue.h"

#include "usbwlan_timer.h"
#include "usbwlan_firmware.h"
#include "usbwlan_syscall.h"
#include "usbwlan_vendor.h"
#include "usbwlan_UsbDev.h"
#include "usbwlan_sw.h"


#include "usbwlan_rx.h"
#include "usbwlan_proto.h"
#include "usbwlan_mng.h"
#include "usbwlan_main.h"
#include "usbwlan_rate.h"
#include "usbwlan_Interrupt.h"
#include "usbwlan_Task.h"

#include "usbwlan_tx.h"



#endif /* !defined(_PRECOMP_H_) */
