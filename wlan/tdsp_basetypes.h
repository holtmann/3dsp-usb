/*
 * base_types.h:
 * 	Copyright (c) 2004 3DSP
 *		All rights Reserved.
 *
 * This source to be viewed with tabspace 2 (":se ts=2" for ex/vi)
 *
 */

#ifndef _TDSP_VKI__BASETYPES_H_
#define _TDSP_VKI__BASETYPES_H_

#include "usbwlan_define.h"

//Spear add the lines below @08/03/07
// compiler property
#define	SUPPORT_DATA_PACKED		//told the compiler to pack data structure


#ifdef SUPPORT_DATA_PACKED
#define ALIGN_BYTE	1
#else
#define ALIGN_BYTE	4
#endif


// TODO:		this seemed have some problem in xscale platform, do not use this now !	fdeng
//Structure packing with the GNU C compiler
//#define STRUCTURE_PACKING __attribute__((__packed__))




/*-----------------------------------------------------------
*                                                           |
* 					for linux style							|
*                                                           |
*----------------------------------------------------------*/




#ifndef boolean
typedef enum
{
	FALSE,
	TRUE
}boolean;
#endif


typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_FAILURE,
    STATUS_TIMEOUT,
    STATUS_NOT_SUPPORTED,
    STATUS_INSUFFICIENT_RESOURCES,
    STATUS_NOT_RESETTABLE,
    STATUS_PENDING,
    STATUS_RESET_IN_PROGRESS,
    STATUS_ADAPTER_NOT_READY,
    STATUS_DUMMY,
}TDSP_STATUS;


//typedef struct 
//{
//	int QuadPart;
//} LARGE_INTEGER, *PLARGE_INTEGER;


typedef unsigned char				UINT8, *PUINT8;
typedef char						INT8, *PINT8;

typedef unsigned short				UINT16, *PUINT16;
typedef short						INT16, *PINT16;

typedef	unsigned int				UINT32, *PUINT32;
typedef	int						INT32, *PINT32;

typedef	unsigned long				ULONG, *PULONG;

typedef unsigned long long			UINT64, *PUINT64;
typedef long long					INT64, *PINT64;

typedef UINT32					UINT_PTR,* PUINT_PTR;

typedef boolean			BOOLEAN;

typedef void				VOID, *PVOID;

#include "tdsp_endian.h"


#define Bit(x)				(1 << (x))
/* Define bit */
#define BIT_0                     (0x0001)
#define BIT_1                     (0x0002)
#define BIT_2                     (0x0004)
#define BIT_3                     (0x0008)
#define BIT_4                     (0x0010)
#define BIT_5                     (0x0020)
#define BIT_6                     (0x0040)
#define BIT_7                     (0x0080)
#define BIT_8                     (0x0100)
#define BIT_9                     (0x0200)
#define BIT_10                    (0x0400)
#define BIT_11                    (0x0800)
#define BIT_12                    (0x1000)
#define BIT_13                    (0x2000)
#define BIT_14                    (0x4000)
#define BIT_15                    (0x8000)
#define BIT_16                    (0x00010000)
#define BIT_17                    (0x00020000)
#define BIT_18                    (0x00040000)
#define BIT_19                    (0x00080000)
#define BIT_20                    (0x00100000)
#define BIT_21                    (0x00200000)
#define BIT_22                    (0x00400000)
#define BIT_23                    (0x00800000)
#define BIT_24                    (0x01000000)
#define BIT_25                    (0x02000000)
#define BIT_26                    (0x04000000)
#define BIT_27                    (0x08000000)
#define BIT_28                    (0x10000000)
#define BIT_29                    (0x20000000)
#define BIT_30                    (0x40000000)
#define BIT_31                    (0x80000000)

#define BIT0	0x00000001
#define BIT1	0x00000002
#define BIT2	0x00000004
#define BIT3	0x00000008
#define BIT4	0x00000010
#define BIT5	0x00000020
#define BIT6	0x00000040
#define BIT7	0x00000080
#define BIT8	0x00000100
#define BIT9	0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

#define BIT_0_1     0x0003
#define BIT_0_2     0x0007
#define BIT_0_3     0x000F
#define BIT_0_4     0x001F
#define BIT_0_5     0x003F
#define BIT_0_6     0x007F
#define BIT_0_7     0x00FF
#define BIT_0_8     0x01FF
#define BIT_0_11    0x0FFF
#define BIT_0_13    0x3FFF
#define BIT_0_15    0xFFFF
#define BIT_1_2     0x0006
#define BIT_1_3     0x000E
#define BIT_2_5     0x003C
#define BIT_3_4     0x0018
#define BIT_4_5     0x0030
#define BIT_4_6     0x0070
#define BIT_4_7     0x00F0
#define BIT_5_7     0x00E0
#define BIT_5_9     0x03E0
#define BIT_5_11    0x0FE0
#define BIT_5_12    0x1FE0
#define BIT_5_15    0xFFE0
#define BIT_5_16    0x1FFE0
#define BIT_6_7     0x00c0
#define BIT_7_11    0x0F80
#define BIT_8_11    0x0F00
#define BIT_8_15    0xFF00
#define BIT_9_13    0x3E00
#define BIT_10_17   0x3FC00
#define BIT_12_13   0x00003000
#define BIT_12_15   0x0000F000
#define BIT_15_18   0x78000
#define BIT_16_18   0x00070000
#define BIT_16_20   0x001F0000
#define BIT_17_18   0x00060000
#define BIT_19_20   0x00180000
#define BIT_21_25   0x03E00000
#define BIT_22_27   0x0FC00000
#define BIT_22_31   0xFFC00000
#define BIT_24_28   0x1F000000
#define BIT_26_27   0x0C000000
#define BIT_22_26   0x07C00000



#ifndef NULL
#define NULL		((void *)0)
#endif


#define	Bit(x)		(1 << (x))
#endif //#ifndef _TDSP_VKI__BASETYPES_H_


