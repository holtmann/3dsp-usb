/*
 * base_types.h:
 * 	Copyright (c) 2004 3DSP
 *		All rights Reserved.
 *
 * This source to be viewed with tabspace 2 (":se ts=2" for ex/vi)
 *
 */

#ifndef _BASE_TYPES_H_
#define _BASE_TYPES_H_
#include <linux/types.h>
//#include <asm/semaphore.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#include <asm/io.h>

#include "ntstatus.h"



#define EVENT_UNSIGNED    0
#define EVENT_SIGNED      1


typedef enum
{
	M3DSP_FALSE,
	M3DSP_TRUE
} boolean;

typedef	signed char		INT8, *PINT8;
typedef	signed short		INT16, *PINT16;
typedef signed int		       INT32, *PINT32;
typedef __s64			INT64, *PINT64;
typedef	unsigned char		UINT8, *PUINT8;
typedef	unsigned short	UINT16, *PUINT16;
typedef unsigned int		UINT32, *PUINT32;
typedef __u64			UINT64, *PUINT64;


typedef unsigned long		ULONG_PTR; // LP64, in LLP64, should be unsigned long long

typedef boolean BOOLEAN;

typedef struct urb	URB, *PURB;
typedef	unsigned long  KIRQL; // for dummy para in KeAcquireSpinLock and KeReleaseSpinLock
typedef	spinlock_t  KSPIN_LOCK;
#define KeInitializeSpinLock(lock)        spin_lock_init(lock)
#define KeAcquireSpinLock(lock, dummy)    do {spin_lock_bh(lock); *(dummy) = 0;} while(0)
#define KeReleaseSpinLock(lock, dummy)    do {spin_unlock_bh(lock); (dummy) = 0;} while(0)

/* For spin lock used in interrupt context */

#define KeAcquireIRQSpinLock(lock, flags)    do {spin_lock_irqsave(lock, flags);} while(0)
#define KeReleaseIRQSpinLock(lock, flags)    do {spin_unlock_irqrestore(lock, flags);} while(0)


#define consistent_sync(a,b,c)		

//for windows style timer and DPC function
typedef struct 
{
	unsigned long QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#define KeSetTimer(timer,expires,dummy)	do{	\
		mod_timer(timer,(unsigned long)(jiffies + expires.QuadPart));		\
	}while(0)

#define kTenMilliSec	1

typedef  void VOID, *PVOID;


#define INLINE inline

//SEH for linux
#define __try
#define __leave	do {goto SEH_ERROR;}while(0)
#define __finally	SEH_ERROR:


#define ASSERT(x)	((void)0)


//----End  Define for windows code.---------

#define	Bit(x)		(1 << (x))



#ifndef NULL
#define	NULL			((void *)0x0)
#endif

#define FALSE 0
#define TRUE 1

#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))


#define BT_CRASH()  do{*(char *)0 = 1;}while(0)
//define memory operations for linux OS
//#define ExAllocatePool(_SIZE, _GFP)	kmalloc(_SIZE, _GFP)
//#define ExFreePool		kfree

static inline void *ExAllocatePool(size_t _SIZE, gfp_t _GFP)
{
    if(GFP_KERNEL != _GFP){
        // Crash driver
        BT_CRASH();
        return 0;
    }
    //printk(KERN_ALERT "ExAllocatePool: %d, %dK\n", _SIZE, _SIZE / 1024);
    //return vmalloc(_SIZE);
    return kzalloc(_SIZE, _GFP);
}

static inline void ExFreePool(void *pBlock)
{
    //vfree(pBlock);
    kfree(pBlock);
}


/**
// Allocate by kmalloc
static inline void *ExAllocateKPool(size_t _SIZE, gfp_t _GFP)
{
    printk(KERN_ALERT "ExAllocateKPool: %d\n", _SIZE);
    return kmalloc(_SIZE, _GFP);
}

static inline void ExFreeKPool(void *pBlock)
{
    kfree(pBlock);
}
**/

#define IN
#define OUT


#endif /*_BASE_TYPES_H_*/
