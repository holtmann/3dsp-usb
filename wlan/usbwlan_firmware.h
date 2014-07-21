/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:
 *
 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************/
#ifndef _USBWLAN_FIRMWARE_H_
#define _USBWLAN_FIRMWARE_H_

#include "tdsp_basetypes.h"
#include "precomp.h"
 
#pragma pack(1) 

typedef struct _SP20_CODE 
{
	UINT32	Size;		// Size of the bin resource file
	UINT8 	Port;		// SP20 mem port index
	UINT8 	Code[1];	// SP20 code - copied from bin resource
} SP20_CODE, *PSP20_CODE;

#pragma pack()

extern const UINT8 Usb_SP20CodeWlanOnly[];
extern const UINT8 Usb_SP20CodeBlueToothOnly[];
extern const UINT8 Usb_SP20CodeCombo[];
extern const UINT8 Usb_SP20CodeEeprom[];
extern const UINT8 Usb_8051CodeWlanOnly[];
extern const UINT8 Usb_8051CodeBlueToothOnly[];
extern const UINT8 Usb_8051CodeCombo[];

extern const UINT32 sizeof_Usb_SP20CodeWlanOnly;
extern const UINT32 sizeof_Usb_SP20CodeBlueToothOnly;
extern const UINT32 sizeof_Usb_SP20CodeCombo;
extern const UINT32 sizeof_Usb_SP20CodeEeprom;
extern const UINT32 sizeof_Usb_8051CodeWlanOnly;
extern const UINT32 sizeof_Usb_8051CodeBlueToothOnly;
extern const UINT32 sizeof_Usb_8051CodeCombo;

#endif

