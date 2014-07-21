/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

        USBIOCTL.H

Abstract:

   This file defines both kernel and user mode IOCTL
   codes supported by the USB core stack.

Environment:

    Kernel & user mode

Revision History:

    09-29-95 : created
    01-06-97 : added user mode hub ioctls
    10-31-99 : cleanup and document, jdunn

--*/

#ifndef   __USBIOCTL_H__
#define   __USBIOCTL_H__

#include "usb100.h"

#ifndef FAR
#define FAR
#endif

#include "usbiodef.h"

#pragma message ("warning: using obsolete header file usbioctl.h")

/*
    IOCTLS definitions
*/

/*
   USB kernel Mode IOCTLS
*/

/* IOCTL_INTERNAL_USB_SUBMIT_URB

   This IOCTL is used by client drivers to submit URB (USB Request Blocks)

   Parameters.Others.Argument1 = pointer to URB

*/

#define IOCTL_INTERNAL_USB_SUBMIT_URB  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_SUBMIT_URB,  \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)


/* IOCTL_INTERNAL_USB_RESET_PORT

    This IOCTL is used by kernel mode drivers to reset their
    upstream port.

    After a successful reset the device is re-configured to the
    same configuration it was in before the reset.  All pipe
    handles, configuration handles and interface handles remain
    valid.

*/

#define IOCTL_INTERNAL_USB_RESET_PORT  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_RESET_PORT, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)


/*  IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO

    This IOCTL is used internally by the hub driver this API will
    return the PhysicalDeviceObject of the root hub enumerated by the
    controller.

    Parameters.Others.Argument1 =
        pointer to be filled in with PDO for the root hub;
    Parameters.Others.Argument2 =
        pointer to be filled in with FDO of the USB Host Controller;

*/

#define IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_ROOTHUB_PDO, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)



/* IOCTL_INTERNAL_USB_GET_PORT_STATUS

    This IOCTL returns the current status of the devices upstream
    port.

    Parameters.Others.Argument1 =
        pointer to port status register (UINT32)

    status bits are:

    USBD_PORT_ENABLED
    USBD_PORT_CONNECTED

*/

#define  USBD_PORT_ENABLED      0x00000001
#define  USBD_PORT_CONNECTED    0x00000002


#define IOCTL_INTERNAL_USB_GET_PORT_STATUS  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_PORT_STATUS, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_ENABLE_PORT

    This IOCTL is obsolete, drivers should use
    IOCTL_INTERNAL_USB_RESET_PORT
*/

#define IOCTL_INTERNAL_USB_ENABLE_PORT      CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_ENABLE_PORT, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION

    This ioctl registers a device to receive notification when a specific
    timeout has expired and it should now be suspended in order to conserve
    power. If all devices on a hub are suspended, then the actual hub
    can be suspended.

*/

#define IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION   CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_IDLE_NOTIFICATION,  \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_HUB_COUNT

    This IOCTL is used internally by the hub driver, it returns the
    number of hubs between the device and the root hub.

    Parameters.Others.Argument1 =
        pointer to be count of hubs in chain;

*/
#define IOCTL_INTERNAL_USB_GET_HUB_COUNT      CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_HUB_COUNT, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_CYCLE_PORT

    This IOCTL will simulate a plug/unplug on the port.
    The device will be removed and re-added by PnP.
*/

#define IOCTL_INTERNAL_USB_CYCLE_PORT  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_CYCLE_PORT, \
                                                METHOD_NEITHER,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_HUB_NAME

*/

#define IOCTL_INTERNAL_USB_GET_HUB_NAME  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_HUB_NAME,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_BUS_INFO

    This IOCTL is obsolete -- it has been replaced by the
    USB_BUSIFFN_QUERY_BUS_INFORMATION service available thru
    the usb stack bus interface.
*/

#define IOCTL_INTERNAL_USB_GET_BUS_INFO         CTL_CODE(FILE_DEVICE_USB,  \
                                                    USB_GET_BUS_INFO,  \
                                                    METHOD_BUFFERED,  \
                                                    FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME

*/

#define IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME  CTL_CODE(FILE_DEVICE_USB,  \
                                                    USB_GET_CONTROLLER_NAME,  \
                                                    METHOD_BUFFERED,  \
                                                    FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_BUSGUID_INFO

*/

#define IOCTL_INTERNAL_USB_GET_BUSGUID_INFO     CTL_CODE(FILE_DEVICE_USB,  \
                                                    USB_GET_BUSGUID_INFO,  \
                                                    METHOD_BUFFERED,  \
                                                    FILE_ANY_ACCESS)

/* IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO

*/

#define IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO   CTL_CODE(FILE_DEVICE_USB,  \
                                                    USB_GET_PARENT_HUB_INFO,  \
                                                    METHOD_BUFFERED,  \
                                                    FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE    CTL_CODE(FILE_DEVICE_USB,  \
                                                   USB_GET_DEVICE_HANDLE, \
                                                   METHOD_NEITHER,  \
                                                   FILE_ANY_ACCESS)

/*
   USB user mode IOCTLS
*/

/************************************************************
The following IOCTLS are always sent to the HCD symbolic
name
*************************************************************/

/* IOCTL_USB_HCD_GET_STATS_1 (OPTIONAL)

    The following IOCTL is used to return internal statictics
    for HCDs

*/

#define IOCTL_USB_HCD_GET_STATS_1          CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_GET_STATS_1,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

/* IOCTL_USB_HCD_GET_STATS_2 (OPTIONAL)

    The following IOCTL is used to return internal statictics
    for HCDs

*/

#define IOCTL_USB_HCD_GET_STATS_2          CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_GET_STATS_2,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_HCD_DISABLE_PORT          CTL_CODE(FILE_DEVICE_USB, \
                                                HCD_DISABLE_PORT, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_HCD_ENABLE_PORT           CTL_CODE(FILE_DEVICE_USB, \
                                                HCD_ENABLE_PORT, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)


/*
   These ioctls are used for USB diagnostic and test applications
*/

#define IOCTL_USB_DIAGNOSTIC_MODE_ON   CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_DIAGNOSTIC_MODE_ON,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_DIAGNOSTIC_MODE_OFF  CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_DIAGNOSTIC_MODE_OFF,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_GET_ROOT_HUB_NAME  CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_GET_ROOT_HUB_NAME,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_GET_HCD_DRIVERKEY_NAME CTL_CODE(FILE_DEVICE_USB,  \
                                                HCD_GET_DRIVERKEY_NAME,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)


/*********************************************************
The following IOCTLS are always sent to symbolic names
created by usbhub
**********************************************************/

/*
    Utility IOCTLS supported by the hub device
*/

/*
   These ioctls are supported by the hub driver for
   use by user mode USB utilities.
*/


#define IOCTL_USB_GET_NODE_INFORMATION   CTL_CODE(FILE_DEVICE_USB,  \
                                               USB_GET_NODE_INFORMATION,  \
                                               METHOD_BUFFERED,  \
                                               FILE_ANY_ACCESS)

#define IOCTL_USB_GET_NODE_CONNECTION_INFORMATION  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_NODE_CONNECTION_INFORMATION,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_GET_NODE_CONNECTION_ATTRIBUTES  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_NODE_CONNECTION_ATTRIBUTES,\
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)                                                

#define IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION   CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_GET_NODE_CONNECTION_NAME     CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_NODE_CONNECTION_NAME,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_DIAG_IGNORE_HUBS_ON   CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_DIAG_IGNORE_HUBS_ON,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_DIAG_IGNORE_HUBS_OFF  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_DIAG_IGNORE_HUBS_OFF,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME  CTL_CODE(FILE_DEVICE_USB,  \
                                                USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,  \
                                                METHOD_BUFFERED,  \
                                                FILE_ANY_ACCESS)

#define IOCTL_USB_GET_HUB_CAPABILITIES  CTL_CODE(FILE_DEVICE_USB,  \
                                               USB_GET_HUB_CAPABILITIES,  \
                                               METHOD_BUFFERED,  \
                                               FILE_ANY_ACCESS)

/*
   structures for user mode ioctls
*/

#include <pshpack1.h>

typedef enum _USB_HUB_NODE {
    UsbHub,
    UsbMIParent
} USB_HUB_NODE;

typedef struct _USB_HUB_INFORMATION {
    /*
       copy of data from hub descriptor
    */
    USB_HUB_DESCRIPTOR HubDescriptor;

    BOOLEAN HubIsBusPowered;

} USB_HUB_INFORMATION, *PUSB_HUB_INFORMATION;

typedef struct _USB_MI_PARENT_INFORMATION {
    UINT32 NumberOfInterfaces;
} USB_MI_PARENT_INFORMATION, *PUSB_MI_PARENT_INFORMATION;

typedef struct _USB_NODE_INFORMATION {
    USB_HUB_NODE NodeType;        /* hub, mi parent */
    union {
        USB_HUB_INFORMATION HubInformation;
        USB_MI_PARENT_INFORMATION MiParentInformation;
    } u;
} USB_NODE_INFORMATION, *PUSB_NODE_INFORMATION;

typedef struct _USB_PIPE_INFO {
    USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    UINT32 ScheduleOffset;
} USB_PIPE_INFO, *PUSB_PIPE_INFO;

typedef struct _USB_HUB_CAPABILITIES {
    /*
        Unlike the USB_HUB_INFORMATION structure used by
        IOCTL_USB_GET_NODE_INFORMATION, this structure can be extended in the
        future to accomodate more data.  The IOCTL will return only as much
        data as indicated by the size of the request buffer, to maintain
        backward compatibility with older callers that don't know about the
        new data.
    */

    UINT32 HubIs2xCapable:1;

} USB_HUB_CAPABILITIES, *PUSB_HUB_CAPABILITIES;


typedef enum _USB_CONNECTION_STATUS {
    NoDeviceConnected,
    DeviceConnected,

    /* failure codes, these map to fail reasons */
    DeviceFailedEnumeration,
    DeviceGeneralFailure,
    DeviceCausedOvercurrent,
    DeviceNotEnoughPower,
    DeviceNotEnoughBandwidth,
    DeviceHubNestedTooDeeply,
    DeviceInLegacyHub
} USB_CONNECTION_STATUS, *PUSB_CONNECTION_STATUS;

typedef struct _USB_NODE_CONNECTION_INFORMATION {
    UINT32 ConnectionIndex;
    /* usb device descriptor returned by this device
       during enumeration */
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    UINT8 CurrentConfigurationValue;
    BOOLEAN LowSpeed;

    BOOLEAN DeviceIsHub;

    UINT16 DeviceAddress;

    UINT32 NumberOfOpenPipes;

    USB_CONNECTION_STATUS ConnectionStatus;
    USB_PIPE_INFO PipeList[0];
} USB_NODE_CONNECTION_INFORMATION, *PUSB_NODE_CONNECTION_INFORMATION;

typedef struct _USB_NODE_CONNECTION_ATTRIBUTES {
    UINT32 ConnectionIndex;
    /* usb device descriptor returned by this device
       during enumeration */
    USB_CONNECTION_STATUS ConnectionStatus;

    /* extended port attributes defined in usb.h*/
    UINT32 PortAttributes;
} USB_NODE_CONNECTION_ATTRIBUTES, *PUSB_NODE_CONNECTION_ATTRIBUTES;

typedef struct _USB_NODE_CONNECTION_DRIVERKEY_NAME {
    UINT32 ConnectionIndex;  /* INPUT */
    UINT32 ActualLength;     /* OUTPUT */
    /* unicode name for the devnode. */
    WCHAR DriverKeyName[1];      /* OUTPUT */
} USB_NODE_CONNECTION_DRIVERKEY_NAME, *PUSB_NODE_CONNECTION_DRIVERKEY_NAME;

typedef struct _USB_NODE_CONNECTION_NAME {
    UINT32 ConnectionIndex;  /* INPUT */
    UINT32 ActualLength;     /* OUTPUT */
    /* unicode symbolic name for this node if it is a hub or parent driver
       null if this node is a device. */
    WCHAR NodeName[1];      /* OUTPUT */
} USB_NODE_CONNECTION_NAME, *PUSB_NODE_CONNECTION_NAME;


typedef struct _USB_HUB_NAME {
    UINT32 ActualLength;     /* OUTPUT */
    /* NULL terminated unicode symbolic name for the root hub */
    WCHAR HubName[1];       /* OUTPUT */
} USB_HUB_NAME, *PUSB_HUB_NAME;

typedef struct _USB_ROOT_HUB_NAME {
    UINT32 ActualLength;     /* OUTPUT */
    /* NULL terminated unicode symbolic name for the root hub */
    WCHAR RootHubName[1];   /* OUTPUT */
} USB_ROOT_HUB_NAME, *PUSB_ROOT_HUB_NAME;

typedef struct _USB_HCD_DRIVERKEY_NAME {
    UINT32 ActualLength;     /* OUTPUT */
    /* NULL terminated unicode driverkeyname for hcd */
    WCHAR DriverKeyName[1];   /* OUTPUT */
} USB_HCD_DRIVERKEY_NAME, *PUSB_HCD_DRIVERKEY_NAME;

typedef struct _USB_DESCRIPTOR_REQUEST {
    UINT32 ConnectionIndex;
    struct {
        UINT8 bmRequest;
        UINT8 bRequest;
        UINT16 wValue;
        UINT16 wIndex;
        UINT16 wLength;
    } SetupPacket;
    UINT8 Data[0];
} USB_DESCRIPTOR_REQUEST, *PUSB_DESCRIPTOR_REQUEST;


/*
   Structure for returning HCD debug and statistic information to
   a user mode application.
*/

typedef struct _HCD_STAT_COUNTERS {
    UINT32 BytesTransferred;

    UINT16 IsoMissedCount;
    UINT16 DataOverrunErrorCount;

    UINT16 CrcErrorCount;
    UINT16 ScheduleOverrunCount;

    UINT16 TimeoutErrorCount;
    UINT16 InternalHcErrorCount;

    UINT16 BufferOverrunErrorCount;
    UINT16 SWErrorCount;

    UINT16 StallPidCount;
    UINT16 PortDisableCount;

} HCD_STAT_COUNTERS, *PHCD_STAT_COUNTERS;


typedef struct _HCD_ISO_STAT_COUNTERS {

    UINT16  LateUrbs;
    UINT16  DoubleBufferedPackets;

    UINT16  TransfersCF_5ms;
    UINT16  TransfersCF_2ms;

    UINT16  TransfersCF_1ms;
    UINT16  MaxInterruptLatency;

    UINT16  BadStartFrame;
    UINT16  StaleUrbs;

    /* total count of packets programmed but not accessed by
       the controller either due to software scheduling
       problems or HW problems */
    UINT16  IsoPacketNotAccesed;
    UINT16  IsoPacketHWError;

    UINT16  SmallestUrbPacketCount;
    UINT16  LargestUrbPacketCount;

    UINT16 IsoCRC_Error;
    UINT16 IsoOVERRUN_Error;
    UINT16 IsoINTERNAL_Error;
    UINT16 IsoUNKNOWN_Error;

    UINT32  IsoBytesTransferred;

    /* count of packets missed due to software scheduling
       problems */
    UINT16 LateMissedCount;
    /* incremented when a packet is scheduled but not
       accessed by the controller */
    UINT16 HWIsoMissedCount;

    UINT32  Reserved7[8];

} HCD_ISO_STAT_COUNTERS, *PHCD_ISO_STAT_COUNTERS;



typedef struct _HCD_STAT_INFORMATION_1 {
    UINT32 Reserved1;
    UINT32 Reserved2;
    UINT32 ResetCounters;
    LARGE_INTEGER TimeRead;
    /*
       stat registers
    */
    HCD_STAT_COUNTERS Counters;

} HCD_STAT_INFORMATION_1, *PHCD_STAT_INFORMATION_1;

typedef struct _HCD_STAT_INFORMATION_2 {
    UINT32 Reserved1;
    UINT32 Reserved2;
    UINT32 ResetCounters;
    LARGE_INTEGER TimeRead;

    INT32 LockedMemoryUsed;
    /*
       stat registers
    */
    HCD_STAT_COUNTERS Counters;
    HCD_ISO_STAT_COUNTERS IsoCounters;

} HCD_STAT_INFORMATION_2, *PHCD_STAT_INFORMATION_2;


/*
   WMI related structures
*/

/* these index in to our array of guids */
#define WMI_USB_DRIVER_INFORMATION      0
#define WMI_USB_DRIVER_NOTIFICATION     1
#define WMI_USB_POWER_DEVICE_ENABLE     2

typedef enum _USB_NOTIFICATION_TYPE {

    /*  the following return a
        USB_CONNECTION_NOTIFICATION structure: */
    EnumerationFailure = 0,
    InsufficentBandwidth,
    InsufficentPower,
    OverCurrent,
    ResetOvercurrent,

    /* the following return a
       USB_BUS_NOTIFICATION structure:*/
    AcquireBusInfo,

    /* the following return a
      USB_ACQUIRE_INFO structure: */
    AcquireHubName,
    AcquireControllerName,

    /* the following return a
       USB_HUB_NOTIFICATION structure: */
    HubOvercurrent,
    HubPowerChange,

    HubNestedTooDeeply,
    ModernDeviceInLegacyHub

} USB_NOTIFICATION_TYPE;

typedef struct _USB_NOTIFICATION {
    /* indicates type of notification */
    USB_NOTIFICATION_TYPE NotificationType;

} USB_NOTIFICATION, *PUSB_NOTIFICATION;

/* this structure is used for connection notification
   codes */

typedef struct _USB_CONNECTION_NOTIFICATION {
    /* indicates type of notification */
    USB_NOTIFICATION_TYPE NotificationType;

    /* valid for all connection notifictaion codes,
       0 indicates global condition for hub or parent
       this value will be a port number for devices
       attached to a hub, otherwise a one based
       index if the device is a child of a composite
       parent */
    UINT32 ConnectionNumber;

    /* valid for InsufficentBandwidth,
       the amount of bandwidth the device
       tried to allocate and was denied. */
    UINT32 RequestedBandwidth;     

    /* valid for EnumerationFailure, 
       gives some indication why the device failed 
       to enumerate */
    UINT32 EnumerationFailReason;

    /* valid for InsufficentPower,
       the amount of power requested to configure 
       this device. */
    UINT32 PowerRequested;

    /* length of the UNICODE symbolic name (in bytes) for the HUB 
       that this device is attached to.
       not including NULL */
    UINT32 HubNameLength;
    
} USB_CONNECTION_NOTIFICATION, *PUSB_CONNECTION_NOTIFICATION;

/*
   This structure is used for the bus notification code 'AcquireBusInfo'
*/

typedef struct _USB_BUS_NOTIFICATION {
    /* indicates type of notification */
    USB_NOTIFICATION_TYPE NotificationType;     /* indicates type of */
                                                /* notification */
    UINT32 TotalBandwidth;
    UINT32 ConsumedBandwidth;

    /* length of the UNICODE symbolic name (in bytes) for the controller
       that this device is attached to.
       not including NULL */
    UINT32 ControllerNameLength;

} USB_BUS_NOTIFICATION, *PUSB_BUS_NOTIFICATION;

/*
   used to acquire user mode filenames to open respective objects
*/

typedef struct _USB_ACQUIRE_INFO {
    /* indicates type of notification */
    USB_NOTIFICATION_TYPE NotificationType;
    /* TotalSize of this struct */
    UINT32 TotalSize;

    WCHAR Buffer[1];
} USB_ACQUIRE_INFO, *PUSB_ACQUIRE_INFO;


/*
typedef
VOID
(*USB_IDLE_CALLBACK)(
    PVOID Context
    );

typedef struct _USB_IDLE_CALLBACK_INFO {
    USB_IDLE_CALLBACK IdleCallback;
    PVOID IdleContext;
} USB_IDLE_CALLBACK_INFO, *PUSB_IDLE_CALLBACK_INFO;
*/

#include <poppack.h>


#endif /* __USBIOCTL_H__ */

