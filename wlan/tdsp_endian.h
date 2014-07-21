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
 * [vki.h] description:
 *		This header file contains prototypes of 
 *		functions for handling endian-ness related issues.
 *
 *		fn_name		functionality
 *		------		--------------
 *		LE_2_CPU	convert quantity from little-endian to cpu's endian-ness
 *		BE_2_CPU	convert quantity from big-endian to cpu's endian-ness
 *		CPU_2_LE	convert quantity from cpu's endian-ness to little-endian
 *		CPU_2_BE	convert quantity from cpu's endian-ness to big-endian
 *  
 *		LE_2_CPU_P	convert by pointer from little-endian to cpu's endian-ness
 *		BE_2_CPU_P	convert by pointer from big-endian to cpu's endian-ness
 *		CPU_2_LE_P	convert by pointer from cpu's endian-ness to little-endian
 *		CPU_2_BE_P	convert by pointer from cpu's endian-ness to big-endian
 *  
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


#ifndef _VKI__ENDIAN_H_
#define _VKI__ENDIAN_H_

#include "tdsp_basetypes.h"




//////////////////////////////////////////////////////////////////////////////
//																			//
//				generic implementation of swap macros(V)					//
//																			//
//////////////////////////////////////////////////////////////////////////////


#ifndef reverse_u16
#define	reverse_u16(_val)	\
		(((((_val) & 0xFF00) >> 8) | ((_val) << 8)) & 0xFFFF)
#endif


#ifndef reverse_u32
#define	reverse_u32(_val)									\
		((UINT32)((((UINT32)(_val)) << 24) |				\
				  (((UINT32)(_val) << 8) & 0x00ff0000) |	\
				  (((UINT32)(_val) >> 8) & 0x0000ff00) |	\
				  ((UINT32)(_val) >> 24)))
#endif




/* for the time being define it as little endian architecture,
 * because we are using it on x86 linux machine.
 * This definition is changed to WLAN_BIG_ENDIAN and will get defined by Makefile
 */
#ifndef WLAN_BIG_ENDIAN

#define CPU_2_BE16(_x_) reverse_u16((_x_))
#define CPU_2_BE32(_x_) reverse_u32((_x_))
#define BE16_2_CPU(_x_) reverse_u16((_x_))
#define BE32_2_CPU(_x_) reverse_u32((_x_))

#define CPU_2_LE16(_x_) (_x_)
#define CPU_2_LE32(_x_) (_x_)
#define LE16_2_CPU(_x_) (_x_)
#define LE32_2_CPU(_x_) (_x_)


#else	
/* big endian architecture */

#define CPU_2_BE16(_x_) (_x_)
#define CPU_2_BE32(_x_) (_x_)
#define BE16_2_CPU(_x_) (_x_)
#define BE32_2_CPU(_x_) (_x_)

#define CPU_2_LE16(_x_) reverse_u16((_x_))
#define CPU_2_LE32(_x_) reverse_u32((_x_))
#define LE16_2_CPU(_x_) reverse_u16((_x_))
#define LE32_2_CPU(_x_) reverse_u32((_x_))

#endif 






//////////////////////////////////////////////////////////////////////////////
//																			//
//				generic implementation of swap macros(P)					//
//																			//
//////////////////////////////////////////////////////////////////////////////



#ifndef reverse_u16_p
#define reverse_u16_p(_p)							\
		{											\
			UINT32		value;						\
													\
			value = *(UINT16 *)(_p);				\
			*(UINT16 *)(_p) = reverse_u16(value);	\
		}
#endif


#ifndef reverse_u32_p
#define reverse_u32_p(_p)							\
		{											\
			UINT32		value;						\
													\
			value = *(UINT32 *)(_p);				\
			*(UINT32 *)(_p) = reverse_u32(value);	\
		}
#endif


#ifndef WLAN_BIG_ENDIAN
	/* little endian architecture */

#define CPU_2_BE16_P(_p_) reverse_u16_p((_p_))
#define CPU_2_BE32_P(_p_) reverse_u32_p((_p_))
#define BE16_2_CPU_P(_p_) reverse_u16_p((_p_))
#define BE32_2_CPU_P(_p_) reverse_u32_p((_p_))

#define CPU_2_LE16_P(_p_) (_p_)
#define CPU_2_LE32_P(_p_) (_p_)
#define LE16_2_CPU_P(_p_) (_p_)
#define LE32_2_CPU_P(_p_) (_p_)

#define	get_u32_p(_p)					\
		((*(3 + (PUINT8)(_p)) << 24) |	\
		 (*(2 + (PUINT8)(_p)) << 16) |	\
		 (*(1 + (PUINT8)(_p)) <<  8) |	\
		 (*(0 + (PUINT8)(_p)) <<  0))

#define	get_u16_p(_p)					\
		((*(1 + (PUINT8)(_p)) <<  8) |	\
		 (*(0 + (PUINT8)(_p)) <<  0))

#define	set_u32_p(_p, _val)							\
		{											\
			*(3 + (PUINT8)(_p)) = (_val) >> 24;	\
			*(2 + (PUINT8)(_p)) = (_val) >> 16;	\
			*(1 + (PUINT8)(_p)) = (_val) >>  8;	\
			*(0 + (PUINT8)(_p)) = (_val) >>  0;	\
		}

#define	set_u16_p(_p, _val)							\
		{											\
			*(1 + (PUINT8)(_p)) = (_val) >>  8;	\
			*(0 + (PUINT8)(_p)) = (_val) >>  0;	\
		}





#else
/* big endian architecture */

#define CPU_2_BE16_P(_p_) (_p_)
#define CPU_2_BE32_P(_p_) (_p_)
#define BE16_2_CPU_P(_p_) (_p_)
#define BE32_2_CPU_P(_p_) (_p_)

#define CPU_2_LE16_P(_p_) reverse_u16_p((_p_))
#define CPU_2_LE32_P(_p_) reverse_u32_p((_p_))
#define LE16_2_CPU_P(_p_) reverse_u16_p((_p_))
#define LE32_2_CPU_P(_p_) reverse_u32_p((_p_))


#define	get_u32_p(_p)					\
		((*(0 + (PUINT8)(_p)) << 24) |	\
		 (*(1 + (PUINT8)(_p)) << 16) |	\
		 (*(2 + (PUINT8)(_p)) <<  8) |	\
		 (*(3 + (PUINT8)(_p)) <<  0))

#define	get_u16_p(_p)					\
		((*(0 + (PUINT8)(_p)) <<  8) |	\
		 (*(1 + (PUINT8)(_p)) <<  0))

#define	set_u16_p(_p, _val)							\
		{											\
			*(0 + (PUINT8)(_p)) = (_val) >>  8;	\
			*(1 + (PUINT8)(_p)) = (_val) >>  0;	\
		}
#define	set_u32_p(_p, _val)							\
		{											\
			*(0 + (PUINT8)(_p)) = (_val) >>  24;	\
			*(1 + (PUINT8)(_p)) = (_val) >>  16;	\
			*(2 + (PUINT8)(_p)) = (_val) >>  8;	\
			*(3 + (PUINT8)(_p)) = (_val) >>  0;	\
		}



#endif 




#endif /* !_VKI__ENDIAN_H_ */
