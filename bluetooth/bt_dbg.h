/************************************************************************
 * Copyright (c) 2003, 3DSP Corporation, all rights reserved.  Duplication or
 * reproduction of any part of this software (source code, object code or
 * comments) without the expressed written consent by 3DSP Corporation is
 * forbidden.  For further information please contact:
 *
 * 3DSP Corporation
 * 16271 Laguna Canyon Rd
 * Irvine, CA 92618
 * www.3dsp.com
 *************************************************************************/

#ifndef __BT_DBG_H
#define __BT_DBG_H


#define ACCESS_REGISTER_DIRECTLY
//
// We define the external interfaces to the blue tooth driver.
// These routines are only external to permit separate
// compilation.  Given a truely fast compiler they could
// all reside in a single file and be static.
//

#define DOWNLOAD_8051_FIRMWARE
#define DOWNLOAD_3DSP_FIRMWARE
#define DOWNLOAD_8051_WITH_BIN_FILE_MODE
#define DOWNLOAD_3DSP_WITH_BIN_FILE_MODE

//combine all the passive thread
#define SERIALIZE_ALL_THREADS

#define TOTAL_HCI_EVENTS    (0x3D)
#define TOTAL_LINK_CTRL_CMD (0x34)
#define TOTAL_LINK_POLICY_CMD   (0x11)
#define TOTAL_BASEBAND_CMD  (0x5F)
#define TOTAL_INFO_CMD  (0x09)
#define TOTAL_STATUS_CMD    (0x07)


/* Debug message print with level, totally 16 levels*/
/* Level0, failure or error */
#define LEVEL0      (0x1 << 0)
/* For basic debug information */
#define LEVEL1      (0x1 << 1)
/* Enter/Exit functions */
#define LEVEL2      (0x1 << 2)
/* Data flow indication */
#define LEVEL3      (0x1 << 3)
#define LEVEL4      (0x1 << 4)
#define LEVEL5      (0x1 << 5)



/* Debug zone, totally 16 zones*/
#define ZONE_TEST    (0x1 << 16)
#define ZONE_SCHED   (0x1 << 17)

#define ZONE_USB    (0x1 << 18)
#define ZONE_LMP    (0x1 << 19)
#define ZONE_SDRCV  (0x1 << 20)
#define ZONE_FRG    (0x1 << 21)
#define ZONE_DLD    (0x1 << 22)
#define ZONE_AFH    (0x1 << 23)
#define ZONE_CANCEL    (0x1 << 24)
#define ZONE_FLUSH     (0x1 << 25)
#define ZONE_HAL       (0x1 << 26)
#define ZONE_HCI       (0x1 << 27)
#define ZONE_MAIN      (0x1 << 28)
#define ZONE_PNP       (0x1 << 29)
#define ZONE_POWER     (0x1 << 30)
#define ZONE_TASK      (0x1 << 31)


#define LEVEL_MASK  (LEVEL0)
#define ZONE_MASK   (0xFFFFFFFF)

extern unsigned int dbg_zone, dbg_level;

/* Dynamic debug masking */
#define BT_DBGEXT(gate, fmt, arg...) \
    (((dbg_zone) & (gate)) && ((dbg_level) & (gate))) ? (printk(KERN_INFO "%s: " fmt "" , __FUNCTION__ , ## arg)), 1 : 0

#define BT_DBGSUB(gate, fmt, arg...) \
    (((dbg_zone) & (gate)) && ((dbg_level) & (gate))) ? (printk(KERN_INFO fmt "" , ## arg)), 1 : 0

#define BT_INFOEXT(fmt, arg...) \
    (printk(KERN_ALERT fmt "" , ## arg))

#define ENTER_FUNC()    BT_DBGEXT(ZONE_MAIN | LEVEL5, "++++++\n")
#define EXIT_FUNC()    BT_DBGEXT(ZONE_MAIN | LEVEL5, "------\n")


/* IOCTL instance for debug */
#define HCISETDBGZONE 	    _IOW('H', 101, int)
#define HCICODECOVERAGE 	_IOW('H', 102, int)
#define IOCTL_READ_SCRATCH  _IOW('H', 103, int)
#define IOCTL_READ_DSP_REG      _IOW('H', 104, int)
#define IOCTL_READ_USB_REG      _IOW('H', 105, int)
#define IOCTL_WRITE_USB_REG      _IOW('H', 106, int)
#define IOCTL_DUMP_LMP_PKT      _IOW('H', 107, int)
#define IOCTL_SYS_DIAG       _IOW('H', 108, int)
#define IOCTL_DBG_CRASH      _IOW('H', 109, int)
#define IOCTL_WRITE_DSP_REG      _IOW('H', 110, long)


struct dbg_dsp_reg{
    unsigned long offset;
    unsigned long len;
    unsigned char out[256 * 4];
};

struct dbg_usb_reg{
    unsigned long addr;
    unsigned long len;
    unsigned char out[1024];
};

#endif
