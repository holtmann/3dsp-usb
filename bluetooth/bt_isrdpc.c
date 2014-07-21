/************************************************************************
(c) 2003-05, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:
3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
 **************************************************************************
$RCSfile: bt_isrdpc.c,v $
$Revision: 1.1 $
$Date: 2009/03/11 07:57:20 $
 **************************************************************************/
#include "BT_Sw.h"        // include <WDM.H> and data structure for us
#include "BT_Dbg.h"        // include debug function
#include "BT_Hal.h"        // include accessing hardware resources function
#include "BT_Hci.h"
#include "BT_Pr.h"        // include most functions of declaration for us
#include "BT_Frag.h"			

#ifdef BT_TESTDRIVER
#endif
//
// Synchronize Functions
//
///////////////////////////////////////////////////////////////////////////////
//
//  ReadIsDone
//
//    This is a synchronize function, called with the ISR spinlock held, that
//    checks and potentially updates the READ COMPLETE bit in the IntCsr copy
//    that we keep in our device extension.  These bits must be updated under
//    lock.
//
//  INPUTS:
//
//    ServiceContext - Address of our device extension.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      TRUE, if a read is complete, FALSE otherwise.
//
//  IRQL:
//
//      This routine is called at IRQL == DIRQL, specifically the Synchronize
//      IRQL for the device.
//
//  NOTES:
//
//      Remember: A read operation to us is actually called a WRITE operation
//                on the AMCC device.  Ugh.  HARDWARE people!
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN ReadIsDone(IN PVOID SynchronizeContext)
{
	PBT_DEVICE_EXT devExt = (PBT_DEVICE_EXT)SynchronizeContext;
	//
	// Is a READ operation complete on the device?
	// (Yes, the correct bit to check is _WRITE_COMP!)
	//
	//if(devExt->IntCsr & AMCC_INT_WRITE_COMP)  {
	//        devExt->IntCsr &=  ~AMCC_INT_WRITE_COMP;
	//      return(TRUE);
	// }
	return (FALSE);
}
///////////////////////////////////////////////////////////////////////////////
//
//  WriteIsDone
//
//    This is a synchronize function, called with the ISR spinlock held, that
//    checks and potentially updates the WRITE COMPLETE bit in the IntCsr copy
//    that we keep in our device extension.  These bits must be updated under
//    lock.
//
//  INPUTS:
//
//    ServiceContext - Address of our device extension.
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      TRUE, if a write is complete, FALSE otherwise.
//
//  IRQL:
//
//      This routine is called at IRQL == DIRQL, specifically the Synchronize
//      IRQL for the device.
//
//  NOTES:
//
//      Remember: A write operation to us is actually called a READ operation
//                on the AMCC device.  Go figure...
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN WriteIsDone(IN PVOID SynchronizeContext)
{
	PBT_DEVICE_EXT devExt = (PBT_DEVICE_EXT)SynchronizeContext;
	//
	// Is a WRITE operation complete on the device?
	// (Yes, the correct bit to check is _READ_COMP!)
	//
	// if(devExt->IntCsr & AMCC_INT_READ_COMP)  {
	//   devExt->IntCsr &=  ~AMCC_INT_READ_COMP;
	// return(TRUE);
	//}
	return (FALSE);
}
