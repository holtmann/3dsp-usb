/*++ BUILD Version: 0001        // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddndis.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the Network driver interface device.

Author:

    NDIS/ATM Development Team

Revision History:

    added the correct values for NDIS 3.0.
    added Pnp IoCTLs and structures
    added general co ndis oids.
    added PnP and PM OIDs.

--*/

#ifndef _NTDDNDIS_50_
#define _NTDDNDIS_50_


#include "precomp.h"

// new types for Media Specific Indications

#define NDIS_802_11_LENGTH_SSID         32
#define NDIS_802_11_LENGTH_RATES        8
#define NDIS_802_11_LENGTH_RATES_EX     16


typedef UINT8   NDIS_802_11_MAC_ADDRESS[6];


// mask for authentication/integrity fields
#define NDIS_802_11_AUTH_REQUEST_AUTH_FIELDS        0x0f

#define NDIS_802_11_AUTH_REQUEST_REAUTH             0x01
#define NDIS_802_11_AUTH_REQUEST_KEYUPDATE          0x02
#define NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR     0x06
#define NDIS_802_11_AUTH_REQUEST_GROUP_ERROR        0x0E



#define NDIS_802_11_AI_REQFI_CAPABILITIES      1
#define NDIS_802_11_AI_REQFI_LISTENINTERVAL    2
#define NDIS_802_11_AI_REQFI_CURRENTAPADDRESS  4

#define NDIS_802_11_AI_RESFI_CAPABILITIES      1
#define NDIS_802_11_AI_RESFI_STATUSCODE        2
#define NDIS_802_11_AI_RESFI_ASSOCIATIONID     4

typedef enum
{
	MLME_STA_ACTIVE,
	MLME_STA_FAST_POWER_SAVE,
	MLME_STA_CLASSIC_POWER_SAVE
}pwrmgt_mode_t;

#ifdef NDIS50_MINIPORT
typedef UINT8 NDIS_802_11_PMKID_VALUE[16];
#endif

#ifdef WPA2_SUPPORT //add for WPA2 
typedef struct _BSSIDInfo
{
  NDIS_802_11_MAC_ADDRESS  BSSID;
  NDIS_802_11_PMKID_VALUE  PMKID;
} BSSIDInfo, *PBSSIDInfo;
#endif//#ifdef WPA2_SUPPORT //add for WPA2 



#ifdef NDIS50_MINIPORT


#ifndef DSP_WPA2
#define	DSP_WPA2 
#endif

#ifndef WPA2_SUPPORT
#define WPA2_SUPPORT
#endif


//
// IEEE 802.11 Structures and definitions
//
// new types for Media Specific Indications

typedef enum _NDIS_802_11_STATUS_TYPE
{
    Ndis802_11StatusType_Authentication,
	Ndis802_11StatusType_PMKID_CandidateList = 2,
    Ndis802_11StatusTypeMax    // not a real type, defined as an upper bound
} NDIS_802_11_STATUS_TYPE, *PNDIS_802_11_STATUS_TYPE;


#pragma pack(1)

typedef struct _NDIS_802_11_STATUS_INDICATION
{
    NDIS_802_11_STATUS_TYPE StatusType;
} NDIS_802_11_STATUS_INDICATION, *PNDIS_802_11_STATUS_INDICATION;

// mask for authentication/integrity fields
//#define NDIS_802_11_AUTH_REQUEST_AUTH_FIELDS        0x0f

//#define NDIS_802_11_AUTH_REQUEST_REAUTH             0x01
//#define NDIS_802_11_AUTH_REQUEST_KEYUPDATE          0x02
//#define NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR     0x06
//#define NDIS_802_11_AUTH_REQUEST_GROUP_ERROR        0x0E

typedef struct _NDIS_802_11_AUTHENTICATION_REQUEST
{
    UINT32 Length;            // Length of structure
    NDIS_802_11_MAC_ADDRESS Bssid;
    UINT32 Flags;
} NDIS_802_11_AUTHENTICATION_REQUEST, *PNDIS_802_11_AUTHENTICATION_REQUEST;

// Added new types for OFDM 5G and 2.4G
typedef enum _NDIS_802_11_NETWORK_TYPE
{
    Ndis802_11FH,
    Ndis802_11DS,
    Ndis802_11OFDM5,
    Ndis802_11OFDM24,
    Ndis802_11NetworkTypeMax    // not a real type, defined as an upper bound
} NDIS_802_11_NETWORK_TYPE, *PNDIS_802_11_NETWORK_TYPE;

typedef struct _NDIS_802_11_NETWORK_TYPE_LIST
{
    UINT32                       NumberOfItems;  // in list below, at least 1
    NDIS_802_11_NETWORK_TYPE    NetworkType [1];
} NDIS_802_11_NETWORK_TYPE_LIST, *PNDIS_802_11_NETWORK_TYPE_LIST;

typedef enum _NDIS_802_11_POWER_MODE
{
    Ndis802_11PowerModeCAM,
    Ndis802_11PowerModeMAX_PSP,
    Ndis802_11PowerModeFast_PSP,
    Ndis802_11PowerModeMax      // not a real mode, defined as an upper bound
} NDIS_802_11_POWER_MODE, *PNDIS_802_11_POWER_MODE;

typedef UINT32   NDIS_802_11_TX_POWER_LEVEL; // in milliwatts

//
// Received Signal Strength Indication
//
typedef INT32   NDIS_802_11_RSSI;           // in dBm

typedef struct _NDIS_802_11_CONFIGURATION_FH
{
    UINT32           Length;             // Length of structure
    UINT32           HopPattern;         // As defined by 802.11, MSB set
    UINT32           HopSet;             // to one if non-802.11
    UINT32           DwellTime;          // units are Kusec
} NDIS_802_11_CONFIGURATION_FH, *PNDIS_802_11_CONFIGURATION_FH;

typedef struct _NDIS_802_11_CONFIGURATION
{
    UINT32           Length;             // Length of structure
    UINT32           BeaconPeriod;       // units are Kusec
    UINT32           ATIMWindow;         // units are Kusec
    UINT32           DSConfig;           // Frequency, units are kHz
    NDIS_802_11_CONFIGURATION_FH    FHConfig;
} NDIS_802_11_CONFIGURATION, *PNDIS_802_11_CONFIGURATION;

typedef struct _NDIS_802_11_STATISTICS
{
    UINT32           Length;             // Length of structure
    UINT64   TransmittedFragmentCount;
    UINT64   MulticastTransmittedFrameCount;
    UINT64   FailedCount;
    UINT64   RetryCount;
    UINT64   MultipleRetryCount;
    UINT64   RTSSuccessCount;
    UINT64   RTSFailureCount;
    UINT64   ACKFailureCount;
    UINT64   FrameDuplicateCount;
    UINT64   ReceivedFragmentCount;
    UINT64   MulticastReceivedFrameCount;
    UINT64   FCSErrorCount;
} NDIS_802_11_STATISTICS, *PNDIS_802_11_STATISTICS;

typedef UINT32  NDIS_802_11_KEY_INDEX;
//typedef ULONGLONG   NDIS_802_11_KEY_RSC;


typedef struct _NDIS_802_11_ADD_KEY
{

    UINT64					    KeyRSC;
    UINT8						BSS_ID[6];
    UINT8						wep_mode;
    UINT8						KeyIndex;           
    UINT8						KeyLength;          // length of key in bytes
    UINT8					   is_group_key;
    UINT8					   is_tx_key;
    UINT8					    keyRSC_valid;
    UINT8           				KeyMaterial[1];     // variable length depending on above field
} NDIS_802_11_ADD_KEY, *PNDIS_802_11_ADD_KEY;

typedef struct _NDIS_802_11_REMOVE_KEY
{
    UINT8          				KeyIndex;           
    UINT8					    is_group_key;
} NDIS_802_11_REMOVE_KEY, *PNDIS_802_11_REMOVE_KEY;

typedef struct _NDIS_802_11_WEP
{
    UINT32           Length;             // Length of this structure
    UINT32           KeyIndex;           // 0 is the per-client key, 1-N are the
                                        // global keys
    UINT32           KeyLength;          // length of key in bytes
    UINT8		     is_default_key;
    UINT8           KeyMaterial[1];     // variable length depending on above field
} NDIS_802_11_WEP, *PNDIS_802_11_WEP;



#pragma pack()

typedef enum _NDIS_802_11_NETWORK_INFRASTRUCTURE
{
    Ndis802_11IBSS,
    Ndis802_11Infrastructure,
    Ndis802_11AutoUnknown,
    Ndis802_11InfrastructureMax         // Not a real value, defined as upper bound
} NDIS_802_11_NETWORK_INFRASTRUCTURE, *PNDIS_802_11_NETWORK_INFRASTRUCTURE;

// Add new authentication modes
typedef enum _NDIS_802_11_AUTHENTICATION_MODE
{
    Ndis802_11AuthModeOpen,
    Ndis802_11AuthModeShared,
    Ndis802_11AuthModeAutoSwitch,
    Ndis802_11AuthModeWPA,
    Ndis802_11AuthModeWPAPSK,
    Ndis802_11AuthModeWPANone,
#ifdef DSP_WPA2
	Ndis802_11AuthModeWPA2,
	Ndis802_11AuthModeWPA2PSK
#else
    Ndis802_11AuthModeMax               // Not a real mode, defined as upper bound
#endif
} NDIS_802_11_AUTHENTICATION_MODE, *PNDIS_802_11_AUTHENTICATION_MODE;

typedef UINT8   NDIS_802_11_RATES[NDIS_802_11_LENGTH_RATES];        // Set of 8 data rates
typedef UINT8   NDIS_802_11_RATES_EX[NDIS_802_11_LENGTH_RATES_EX];  // Set of 16 data rates

typedef struct _NDIS_802_11_SSID
{
    UINT32   SsidLength;         // length of SSID field below, in bytes;
                                // this can be zero.
    UINT8   Ssid[NDIS_802_11_LENGTH_SSID];           // SSID information field
} NDIS_802_11_SSID, *PNDIS_802_11_SSID;


typedef struct _NDIS_WLAN_BSSID
{
    UINT32                               Length;             // Length of this structure
    NDIS_802_11_MAC_ADDRESS             MacAddress;         // BSSID
    UINT8                               Reserved[2];
    NDIS_802_11_SSID                    Ssid;               // SSID
    UINT32                               Privacy;            // WEP encryption requirement
    NDIS_802_11_RSSI                    Rssi;               // receive signal
                                                            // strength in dBm
    NDIS_802_11_NETWORK_TYPE            NetworkTypeInUse;
    NDIS_802_11_CONFIGURATION           Configuration;
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode;
    NDIS_802_11_RATES                   SupportedRates;
} NDIS_WLAN_BSSID, *PNDIS_WLAN_BSSID;

typedef struct _NDIS_802_11_BSSID_LIST
{
    UINT32           NumberOfItems;      // in list below, at least 1
    NDIS_WLAN_BSSID Bssid[1];
} NDIS_802_11_BSSID_LIST, *PNDIS_802_11_BSSID_LIST;

// Added Capabilities, IELength and IEs for each BSSID
typedef struct _NDIS_WLAN_BSSID_EX
{
    UINT32                               Length;             // Length of this structure
    NDIS_802_11_MAC_ADDRESS             MacAddress;         // BSSID
    UINT8                               Reserved[2];
    NDIS_802_11_SSID                    Ssid;               // SSID
    UINT32                               Privacy;            // WEP encryption requirement
    NDIS_802_11_RSSI                    Rssi;               // receive signal
                                                            // strength in dBm
    NDIS_802_11_NETWORK_TYPE            NetworkTypeInUse;
    NDIS_802_11_CONFIGURATION           Configuration;
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode;
    NDIS_802_11_RATES_EX                SupportedRates;
    UINT32                               IELength;
    UINT8                               IEs[1];
} NDIS_WLAN_BSSID_EX, *PNDIS_WLAN_BSSID_EX;

typedef struct _NDIS_802_11_BSSID_LIST_EX
{
    UINT32                   NumberOfItems;      // in list below, at least 1
    NDIS_WLAN_BSSID_EX      Bssid[1];
} NDIS_802_11_BSSID_LIST_EX, *PNDIS_802_11_BSSID_LIST_EX;

typedef struct _NDIS_802_11_FIXED_IEs 
{
    UINT8 Timestamp[8];
    UINT16 BeaconInterval;
    UINT16 Capabilities;
} NDIS_802_11_FIXED_IEs, *PNDIS_802_11_FIXED_IEs;

typedef struct _NDIS_802_11_VARIABLE_IEs 
{
    UINT8 ElementID;
    UINT8 Length;    // Number of bytes in data field
    UINT8 data[1];
} NDIS_802_11_VARIABLE_IEs, *PNDIS_802_11_VARIABLE_IEs;

typedef  UINT32   NDIS_802_11_FRAGMENTATION_THRESHOLD;

typedef  UINT32   NDIS_802_11_RTS_THRESHOLD;

typedef  UINT32   NDIS_802_11_ANTENNA;

typedef enum _NDIS_802_11_PRIVACY_FILTER
{
    Ndis802_11PrivFilterAcceptAll,
    Ndis802_11PrivFilter8021xWEP
} NDIS_802_11_PRIVACY_FILTER, *PNDIS_802_11_PRIVACY_FILTER;

// Added new encryption types
// Also aliased typedef to new name
typedef enum _NDIS_802_11_WEP_STATUS
{
    Ndis802_11WEPEnabled,
    Ndis802_11Encryption1Enabled = Ndis802_11WEPEnabled,
    Ndis802_11WEPDisabled,
    Ndis802_11EncryptionDisabled = Ndis802_11WEPDisabled,
    Ndis802_11WEPKeyAbsent,
    Ndis802_11Encryption1KeyAbsent = Ndis802_11WEPKeyAbsent,
    Ndis802_11WEPNotSupported,
    Ndis802_11EncryptionNotSupported = Ndis802_11WEPNotSupported,
    Ndis802_11Encryption2Enabled,
    Ndis802_11Encryption2KeyAbsent,
    Ndis802_11Encryption3Enabled,
    Ndis802_11Encryption3KeyAbsent
} NDIS_802_11_WEP_STATUS, *PNDIS_802_11_WEP_STATUS,
  NDIS_802_11_ENCRYPTION_STATUS, *PNDIS_802_11_ENCRYPTION_STATUS;

typedef enum _NDIS_802_11_RELOAD_DEFAULTS
{
    Ndis802_11ReloadWEPKeys
} NDIS_802_11_RELOAD_DEFAULTS, *PNDIS_802_11_RELOAD_DEFAULTS;

//#define NDIS_802_11_AI_REQFI_CAPABILITIES      1
//#define NDIS_802_11_AI_REQFI_LISTENINTERVAL    2
//#define NDIS_802_11_AI_REQFI_CURRENTAPADDRESS  4

//#define NDIS_802_11_AI_RESFI_CAPABILITIES      1
//#define NDIS_802_11_AI_RESFI_STATUSCODE        2
//#define NDIS_802_11_AI_RESFI_ASSOCIATIONID     4

typedef struct _NDIS_802_11_AI_REQFI
{
    UINT16 Capabilities;
    UINT16 ListenInterval;
    NDIS_802_11_MAC_ADDRESS  CurrentAPAddress;
} NDIS_802_11_AI_REQFI, *PNDIS_802_11_AI_REQFI;

typedef struct _NDIS_802_11_AI_RESFI
{
    UINT16 Capabilities;
    UINT16 StatusCode;
    UINT16 AssociationId;
} NDIS_802_11_AI_RESFI, *PNDIS_802_11_AI_RESFI;

typedef struct _NDIS_802_11_ASSOCIATION_INFORMATION
{
    UINT32                   Length;
    UINT16                  AvailableRequestFixedIEs;
    NDIS_802_11_AI_REQFI    RequestFixedIEs;
    UINT32                   RequestIELength;
    UINT32                   OffsetRequestIEs;
    UINT16                  AvailableResponseFixedIEs;
    NDIS_802_11_AI_RESFI    ResponseFixedIEs;
    UINT32                   ResponseIELength;
    UINT32                   OffsetResponseIEs;
} NDIS_802_11_ASSOCIATION_INFORMATION, *PNDIS_802_11_ASSOCIATION_INFORMATION;

typedef struct _NDIS_802_11_AUTHENTICATION_EVENT
{
    NDIS_802_11_STATUS_INDICATION       Status;
    NDIS_802_11_AUTHENTICATION_REQUEST  Request[1];
} NDIS_802_11_AUTHENTICATION_EVENT, *PNDIS_802_11_AUTHENTICATION_EVENT;
        
typedef struct _NDIS_802_11_TEST
{
    UINT32 Length;
    UINT32 Type;
    union
    {
        NDIS_802_11_AUTHENTICATION_EVENT AuthenticationEvent;
        NDIS_802_11_RSSI RssiTrigger;
    };
} NDIS_802_11_TEST, *PNDIS_802_11_TEST;

#ifdef WPA2_SUPPORT //add for WPA2 

typedef struct _NDIS_802_11_AUTHENTICATION_ENCRYPTION
{
  NDIS_802_11_AUTHENTICATION_MODE  AuthModeSupported;
  NDIS_802_11_ENCRYPTION_STATUS  EncryptStatusSupported;
} NDIS_802_11_AUTHENTICATION_ENCRYPTION, *PNDIS_802_11_AUTHENTICATION_ENCRYPTION;

typedef struct _NDIS_802_11_CAPABILITY
{
  UINT32  Length;
  UINT32  Version;
  UINT32  NoOfPMKIDs;
  UINT32  NoOfAuthEncryptPairsSupported;
  NDIS_802_11_AUTHENTICATION_ENCRYPTION AuthenticationEncryptionSupported[1];
} NDIS_802_11_CAPABILITY, *PNDIS_802_11_CAPABILITY;

typedef struct _NDIS_802_11_PMKID
{
  UINT32  Length;
  UINT32  BSSIDInfoCount;
  BSSIDInfo BSSIDInfo[1];
} NDIS_802_11_PMKID, *PNDIS_802_11_PMKID;

#endif
//
// IRDA objects
//
#define OID_IRDA_RECEIVING                      0x0A010100
#define OID_IRDA_TURNAROUND_TIME                0x0A010101
#define OID_IRDA_SUPPORTED_SPEEDS               0x0A010102
#define OID_IRDA_LINK_SPEED                     0x0A010103
#define OID_IRDA_MEDIA_BUSY                     0x0A010104

#define OID_IRDA_EXTRA_RCV_BOFS                 0x0A010200
#define OID_IRDA_RATE_SNIFF                     0x0A010201
#define OID_IRDA_UNICAST_LIST                   0x0A010202
#define OID_IRDA_MAX_UNICAST_LIST_SIZE          0x0A010203
#define OID_IRDA_MAX_RECEIVE_WINDOW_SIZE        0x0A010204
#define OID_IRDA_MAX_SEND_WINDOW_SIZE           0x0A010205
#define OID_IRDA_RESERVED1                      0x0A01020A  // The range between OID_IRDA_RESERVED1
#define OID_IRDA_RESERVED2                      0x0A01020F  // and OID_IRDA_RESERVED2 is reserved

//
// BPC OIDs
//
#define OID_BPC_ADAPTER_CAPS                    0x0B010100
#define OID_BPC_DEVICES                         0x0B010101
#define OID_BPC_DEVICE_CAPS                     0x0B010102
#define OID_BPC_DEVICE_SETTINGS                 0x0B010103
#define OID_BPC_CONNECTION_STATUS               0x0B010104
#define OID_BPC_ADDRESS_COMPARE                 0x0B010105
#define OID_BPC_PROGRAM_GUIDE                   0x0B010106
#define OID_BPC_LAST_ERROR                      0x0B020107
#define OID_BPC_POOL                            0x0B010108

#define OID_BPC_PROVIDER_SPECIFIC               0x0B020109
#define OID_BPC_ADAPTER_SPECIFIC                0x0B02010A

#define OID_BPC_CONNECT                         0x0B01010B
#define OID_BPC_COMMIT                          0x0B01010C
#define OID_BPC_DISCONNECT                      0x0B01010D
#define OID_BPC_CONNECTION_ENABLE               0x0B01010E
#define OID_BPC_POOL_RESERVE                    0x0B01010F
#define OID_BPC_POOL_RETURN                     0x0B010110
#define OID_BPC_FORCE_RECEIVE                   0x0B010111

#define OID_BPC_LAST                            0x0B020112

//
// IEEE1394 mandatory general OIDs.
//
#define OID_1394_LOCAL_NODE_INFO                0x0C010101
#define OID_1394_VC_INFO                        0x0C010102

//
// The following OIDs are not specific to a media.
//

//
// These are objects for Connection-oriented media call-managers.
//
#define OID_CO_ADD_PVC                          0xFE000001
#define OID_CO_DELETE_PVC                       0xFE000002
#define OID_CO_GET_CALL_INFORMATION             0xFE000003
#define OID_CO_ADD_ADDRESS                      0xFE000004
#define OID_CO_DELETE_ADDRESS                   0xFE000005
#define OID_CO_GET_ADDRESSES                    0xFE000006
#define OID_CO_ADDRESS_CHANGE                   0xFE000007
#define OID_CO_SIGNALING_ENABLED                0xFE000008
#define OID_CO_SIGNALING_DISABLED               0xFE000009
#define OID_CO_AF_CLOSE                         0xFE00000A

//
// Objects for call-managers and MCMs that support TAPI access.
//
#define OID_CO_TAPI_CM_CAPS                     0xFE001001
#define OID_CO_TAPI_LINE_CAPS                   0xFE001002
#define OID_CO_TAPI_ADDRESS_CAPS                0xFE001003
#define OID_CO_TAPI_TRANSLATE_TAPI_CALLPARAMS   0xFE001004
#define OID_CO_TAPI_TRANSLATE_NDIS_CALLPARAMS   0xFE001005
#define OID_CO_TAPI_TRANSLATE_TAPI_SAP          0xFE001006
#define OID_CO_TAPI_GET_CALL_DIAGNOSTICS        0xFE001007
#define OID_CO_TAPI_REPORT_DIGITS               0xFE001008 
#define OID_CO_TAPI_DONT_REPORT_DIGITS          0xFE001009

//
//  PnP and PM OIDs
//
#define OID_PNP_CAPABILITIES                    0xFD010100
#define OID_PNP_SET_POWER                       0xFD010101
#define OID_PNP_QUERY_POWER                     0xFD010102
#define OID_PNP_ADD_WAKE_UP_PATTERN             0xFD010103
#define OID_PNP_REMOVE_WAKE_UP_PATTERN          0xFD010104
#define OID_PNP_WAKE_UP_PATTERN_LIST            0xFD010105
#define OID_PNP_ENABLE_WAKE_UP                  0xFD010106

//
//  PnP/PM Statistics (Optional).
//
#define OID_PNP_WAKE_UP_OK                      0xFD020200
#define OID_PNP_WAKE_UP_ERROR                   0xFD020201

//
//  The following bits are defined for OID_PNP_ENABLE_WAKE_UP
//
#define NDIS_PNP_WAKE_UP_MAGIC_PACKET           0x00000001
#define NDIS_PNP_WAKE_UP_PATTERN_MATCH          0x00000002
#define NDIS_PNP_WAKE_UP_LINK_CHANGE            0x00000004

//
//  TCP/IP OIDs
//
#define OID_TCP_TASK_OFFLOAD                    0xFC010201
#define OID_TCP_TASK_IPSEC_ADD_SA               0xFC010202
#define OID_TCP_TASK_IPSEC_DELETE_SA            0xFC010203
#define OID_TCP_SAN_SUPPORT                     0xFC010204


//
//  Defines for FFP
//
#define OID_FFP_SUPPORT                         0xFC010210
#define OID_FFP_FLUSH                           0xFC010211
#define OID_FFP_CONTROL                         0xFC010212
#define OID_FFP_PARAMS                          0xFC010213
#define OID_FFP_DATA                            0xFC010214

#define OID_FFP_DRIVER_STATS                    0xFC020210
#define OID_FFP_ADAPTER_STATS                   0xFC020211

//
//  Defines for QOS
//
#define OID_QOS_TC_SUPPORTED                    0xFB010100
#define OID_QOS_REMAINING_BANDWIDTH             0xFB010101
#define OID_QOS_ISSLOW_FLOW                     0xFB010102
#define OID_QOS_BESTEFFORT_BANDWIDTH            0xFB010103
#define OID_QOS_LATENCY                         0xFB010104
#define OID_QOS_FLOW_IP_CONFORMING              0xFB010105
#define OID_QOS_FLOW_COUNT                      0xFB010106
#define OID_QOS_NON_BESTEFFORT_LIMIT            0xFB010107
#define OID_QOS_SCHEDULING_PROFILES_SUPPORTED   0xFB010108
#define OID_QOS_CURRENT_SCHEDULING_PROFILE      0xFB010109
#define OID_QOS_TIMER_RESOLUTION                0xFB01010A
#define OID_QOS_STATISTICS_BUFFER               0xFB01010B
#define OID_QOS_HIERARCHY_CLASS                 0xFB01010C
#define OID_QOS_FLOW_MODE                       0xFB01010D
#define OID_QOS_ISSLOW_FRAGMENT_SIZE            0xFB01010E
#define OID_QOS_FLOW_IP_NONCONFORMING           0xFB01010F
#define OID_QOS_FLOW_8021P_CONFORMING           0xFB010110
#define OID_QOS_FLOW_8021P_NONCONFORMING        0xFB010111
#define OID_QOS_ENABLE_AVG_STATS                0xFB010112
#define OID_QOS_ENABLE_WINDOW_ADJUSTMENT        0xFB010113

//




#define OFFLOAD_MAX_SAS             3

#define OFFLOAD_INBOUND_SA          0x0001
#define OFFLOAD_OUTBOUND_SA         0x0002


//
//  Protocol types supported by ndis. These values need to be consistent with ADDRESS_TYPE_XXX defined in TDI.H
//
#define NDIS_PROTOCOL_ID_DEFAULT        0x00
#define NDIS_PROTOCOL_ID_TCP_IP         0x02
#define NDIS_PROTOCOL_ID_IPX            0x06
#define NDIS_PROTOCOL_ID_NBF            0x07
#define NDIS_PROTOCOL_ID_MAX            0x0F
#define NDIS_PROTOCOL_ID_MASK           0x0F


#define NETWORK_ADDRESS_LENGTH_IP sizeof (NETWORK_ADDRESS_IP)

#define NETWORK_ADDRESS_LENGTH_IPX sizeof (NETWORK_ADDRESS_IPX)


#define READABLE_LOCAL_CLOCK                    0x00000001
#define CLOCK_NETWORK_DERIVED                   0x00000002
#define CLOCK_PRECISION                         0x00000004
#define RECEIVE_TIME_INDICATION_CAPABLE         0x00000008
#define TIMED_SEND_CAPABLE                      0x00000010
#define TIME_STAMP_CAPABLE                      0x00000020

//
// the following flags define the -enabled- wake-up capabilities of the device
// passed in the Flags field of NDIS_PNP_CAPABILITIES structure
//
#define NDIS_DEVICE_WAKE_UP_ENABLE                          0x00000001
#define NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE            0x00000002
#define NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE             0x00000004


// Flags used in NDIS_WAN_PROTOCOL_CAPS
//
#define WAN_PROTOCOL_KEEPS_STATS    0x00000001



//
// The following is set on a per-packet basis as OOB data with NdisClass802_3Priority
//
typedef UINT32   Priority_802_3;         // 0-7 priority levels


#ifndef _NDIS_
typedef int NDIS_STATUS, *PNDIS_STATUS;
#endif


#define fNDIS_GUID_TO_OID           0x00000001
#define fNDIS_GUID_TO_STATUS        0x00000002
#define fNDIS_GUID_ANSI_STRING      0x00000004
#define fNDIS_GUID_UNICODE_STRING   0x00000008
#define fNDIS_GUID_ARRAY            0x00000010
#define fNDIS_GUID_ALLOW_READ       0x00000020
#define fNDIS_GUID_ALLOW_WRITE      0x00000040

//
// Ndis Packet Filter Bits (OID_GEN_CURRENT_PACKET_FILTER).
//
#define NDIS_PACKET_TYPE_DIRECTED               0x00000001
#define NDIS_PACKET_TYPE_MULTICAST              0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST          0x00000004
#define NDIS_PACKET_TYPE_BROADCAST              0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING         0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS            0x00000020
#define NDIS_PACKET_TYPE_SMT                    0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL              0x00000080
#define NDIS_PACKET_TYPE_GROUP                  0x00001000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL         0x00002000
#define NDIS_PACKET_TYPE_FUNCTIONAL             0x00004000
#define NDIS_PACKET_TYPE_MAC_FRAME              0x00008000


//
// Ndis Token-Ring Ring Status Codes (OID_802_5_CURRENT_RING_STATUS).
//
#define NDIS_RING_SIGNAL_LOSS                   0x00008000
#define NDIS_RING_HARD_ERROR                    0x00004000
#define NDIS_RING_SOFT_ERROR                    0x00002000
#define NDIS_RING_TRANSMIT_BEACON               0x00001000
#define NDIS_RING_LOBE_WIRE_FAULT               0x00000800
#define NDIS_RING_AUTO_REMOVAL_ERROR            0x00000400
#define NDIS_RING_REMOVE_RECEIVED               0x00000200
#define NDIS_RING_COUNTER_OVERFLOW              0x00000100
#define NDIS_RING_SINGLE_STATION                0x00000080
#define NDIS_RING_RING_RECOVERY                 0x00000040


//
// Ndis protocol option bits (OID_GEN_PROTOCOL_OPTIONS).
//
#define NDIS_PROT_OPTION_ESTIMATED_LENGTH               0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK                    0x00000002
#define NDIS_PROT_OPTION_NO_RSVD_ON_RCVPKT              0x00000004
#define NDIS_PROT_OPTION_SEND_RESTRICTED                0x00000008

//
// Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
//
#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA             0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED              0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND              0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK                     0x00000008
#define NDIS_MAC_OPTION_FULL_DUPLEX                     0x00000010
#define NDIS_MAC_OPTION_EOTX_INDICATION                 0x00000020
#define NDIS_MAC_OPTION_8021P_PRIORITY                  0x00000040
#define NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE  0x00000080
#define NDIS_MAC_OPTION_RECEIVE_AT_DPC                  0x00000100
#define NDIS_MAC_OPTION_8021Q_VLAN                      0x00000200
#define NDIS_MAC_OPTION_RESERVED                        0x80000000

//
//  NDIS media capabilities bits (OID_GEN_MEDIA_CAPABILITIES).
//
#define NDIS_MEDIA_CAP_TRANSMIT                 0x00000001  // Supports sending data
#define NDIS_MEDIA_CAP_RECEIVE                  0x00000002  // Supports receiving data

//
//  NDIS MAC option bits for OID_GEN_CO_MAC_OPTIONS.
//
#define NDIS_CO_MAC_OPTION_DYNAMIC_LINK_SPEED   0x00000001

//
// The following is set on a per-packet basis as OOB data with NdisClassIrdaPacketInfo
// This is the per-packet info specified on a per-packet basis
//
typedef struct _NDIS_IRDA_PACKET_INFO
{
    UINT32                       ExtraBOFs;
    UINT32                       MinTurnAroundTime;
} NDIS_IRDA_PACKET_INFO, *PNDIS_IRDA_PACKET_INFO;


#ifdef WIRELESS_WAN

//
// Wireless WAN structure definitions
//
//
// currently defined Wireless network subtypes
//
typedef enum _NDIS_WW_NETWORK_TYPE
{
    NdisWWGeneric,
    NdisWWMobitex,
    NdisWWPinpoint,
    NdisWWCDPD,
    NdisWWArdis,
    NdisWWDataTAC,
    NdisWWMetricom,
    NdisWWGSM,
    NdisWWCDMA,
    NdisWWTDMA,
    NdisWWAMPS,
    NdisWWInmarsat,
    NdisWWpACT,
    NdisWWFlex,
    NdisWWIDEN
} NDIS_WW_NETWORK_TYPE;

//
// currently defined header formats
//
typedef enum _NDIS_WW_HEADER_FORMAT
{
    NdisWWDIXEthernetFrames,
    NdisWWMPAKFrames,
    NdisWWRDLAPFrames,
    NdisWWMDC4800Frames,
    NdisWWNCLFrames
} NDIS_WW_HEADER_FORMAT;

//
// currently defined encryption types
//
typedef enum _NDIS_WW_ENCRYPTION_TYPE
{
    NdisWWUnknownEncryption = -1,
    NdisWWNoEncryption,
    NdisWWDefaultEncryption,
    NdisWWDESEncryption,
    NdisWWRC2Encryption,
    NdisWWRC4Encryption,
    NdisWWRC5Encryption
} NDIS_WW_ENCRYPTION_TYPE, *PNDIS_WW_ENCRYPTION_TYPE;

typedef enum _WW_ADDRESS_FORMAT
{
    WW_IEEE_ADDRESS = 0,
    WW_MOBITEX_MAN_ADDRESS,
    WW_DATATAC_RDLAP_ADDRESS,
    WW_DATATAC_MDC4800_ADDRESS,
    WW_DATATAC_RESERVED,
    WW_IPv4_ADDRESS,
    WW_IPv6_ADDRESS,
    WW_PROPRIETARY_ADDRESS,
} WW_ADDRESS_FORMAT;

typedef enum _WW_GEN_SUM_EXCEPTION
{
    SIM_STATUS_OK = 0,
    SIM_STATUS_ERROR,
    SIM_STATUS_MISSING,
    SIM_STATUS_NO_RESPONSE,
    SIM_STATUS_REMOVED,
    SIM_STATUS_CRYPT_ERROR,
    SIM_STATUS_AUTH_ERROR,
    SIM_STATUS_NEED_PIN,
    SIM_STATUS_NEED_PUK,
    SIM_STATUS_WRONG,
} WW_GEN_SIM_EXCEPTION;


//
// OID_WW_GEN_INDICATION_REQUEST
//
#if 0 //wumin
typedef struct _NDIS_WW_INDICATION_REQUEST
{
    NDIS_OID            Oid;                    // IN
    UINT32               uIndicationFlag;        // IN
    UINT32               uApplicationToken;      // IN OUT
    HANDLE              hIndicationHandle;      // IN OUT
    INT                 iPollingInterval;       // IN OUT
    NDIS_VAR_DATA_DESC  InitialValue;           // IN OUT
    NDIS_VAR_DATA_DESC  OIDIndicationValue;     // OUT - only valid after indication
    NDIS_VAR_DATA_DESC  TriggerValue;           // IN
} NDIS_WW_INDICATION_REQUEST, *PNDIS_WW_INDICATION_REQUEST;

#define OID_INDICATION_REQUEST_ENABLE           0x0000
#define OID_INDICATION_REQUEST_CANCEL           0x0001

//
// OID_WW_GEN_DEVICE_INFO
//
typedef struct _WW_DEVICE_INFO
{
    NDIS_VAR_DATA_DESC  Manufacturer;
    NDIS_VAR_DATA_DESC  ModelNum;
    NDIS_VAR_DATA_DESC  SWVersionNum;
    NDIS_VAR_DATA_DESC  SerialNum;
} WW_DEVICE_INFO, *PWW_DEVICE_INFO;

//
// OID_WW_GEN_OPERATION_MODE
//
typedef INT WW_OPERATION_MODE;                  //  0 = Normal mode
                                                //  1 = Power saving mode
                                                // -1 = mode unknown

//
// OID_WW_GEN_LOCK_STATUS
//
typedef INT WW_LOCK_STATUS;                     //  0 = unlocked
                                                //  1 = locked
                                                // -1 = unknown lock status

//
// OID_WW_GEN_DISABLE_TRANSMITTER
//
typedef INT WW_DISABLE_TRANSMITTER;             //  0 = transmitter enabled
                                                //  1 = transmitter disabled
                                                // -1 = unknown value

//
// OID_WW_GEN_NETWORK_ID
//
typedef NDIS_VAR_DATA_DESC  WW_NETWORK_ID;

//
// OID_WW_GEN_PERMANENT_ADDRESS 
//
typedef NDIS_VAR_DATA_DESC  WW_PERMANENT_ADDRESS;

//
// OID_WW_GEN_CURRENT_ADDRESS   
//
typedef struct _WW_CURRENT_ADDRESS
{
    NDIS_WW_HEADER_FORMAT   Format;
    NDIS_VAR_DATA_DESC      Address;
} WW_CURRENT_ADDRESS, *PWW_CURRENT_ADDRESS;

//
// OID_WW_GEN_SUSPEND_DRIVER
//
typedef BOOLEAN WW_SUSPEND_DRIVER;              // 0 = driver operational
                                                // 1 = driver suspended
//
// OID_WW_GEN_BASESTATION_ID
//
typedef NDIS_VAR_DATA_DESC  WW_BASESTATION_ID;

//
// OID_WW_GEN_CHANNEL_ID
//
typedef NDIS_VAR_DATA_DESC  WW_CHANNEL_ID;

//
// OID_WW_GEN_ENCRYPTION_STATE
//
typedef BOOLEAN WW_ENCRYPTION_STATE;            // 0 = if encryption is disabled
                                                // 1 = if encryption is enabled

//
// OID_WW_GEN_CHANNEL_QUALITY
//
typedef INT     WW_CHANNEL_QUALITY;             //  0 = Not in network contact,
                                                // 1-100 = Quality of Channel (100 is highest quality).
                                                // -1 = channel quality is unknown

//
// OID_WW_GEN_REGISTRATION_STATUS
//
typedef INT     WW_REGISTRATION_STATUS;         //  0 = Registration denied
                                                //  1 = Registration pending
                                                //  2 = Registered
                                                // -1 = unknown registration status

//
// OID_WW_GEN_RADIO_LINK_SPEED
//
typedef UINT32   WW_RADIO_LINK_SPEED;            // Bits per second.

//
// OID_WW_GEN_LATENCY
//
typedef UINT32   WW_LATENCY;                     //  milliseconds

//
// OID_WW_GEN_BATTERY_LEVEL
//
typedef INT     WW_BATTERY_LEVEL;               //  0-100 = battery level in percentage
                                                //  (100=fully charged)
                                                // -1 = unknown battery level.

//
// OID_WW_GEN_EXTERNAL_POWER
//
typedef INT     WW_EXTERNAL_POWER;              //  0 = no external power connected
                                                //  1 = external power connected
                                                //  -1 = unknown

//
// Ping Address structure
//
typedef struct _WW_PING_ADDRESS
{
    WW_ADDRESS_FORMAT   Format;                 // IN
    NDIS_VAR_DATA_DESC  TargetAddress;          // IN
    UINT                uTime;                  // OUT in milleseconds
} WW_PING_ADDRESS;


//
// RSSI structure
//
typedef struct _WW_RECEIVE_SIGNAL_STRENGTH_INDICATOR
{
    INT                 iDecibels;              // value in DB
    INT                 iFactor;                // power of 10
} WW_RECEIVE_SIGNAL_STRENGTH_INDICATOR;


//
// SIM status structure
//
typedef struct _WW_SIM_STATUS
{
    BOOLEAN             bHasSIM;                // TRUE = SIM required
    BOOLEAN             bBlocked;               // TRUE = SIM PIN access blocked
    BOOLEAN             bLocked;                // TRUE = PIN need to access device
    BOOLEAN             bInitialized;           // TRUE = SIM initialized
    UINT                uCountdown;             // = remaining number of attempt to
                                                // enter correct PIN
} WW_SIM_STATUS;

//
// enable SIM PIN structure
//
typedef struct _WW_ENABLE_SIM_PIN
{
    BOOLEAN             bEnabled;               // TRUE = security feature of SIM enabled
    NDIS_VAR_DATA_DESC  CurrentPIN;             // describes buffer containing PIN value
} WW_ENABLE_SIM_PIN;

//
// SIM PIN structure
//
typedef struct _WW_CHANGE_SIM_PIN
{
    NDIS_VAR_DATA_DESC  OldPIN;                 // describes buffer containing OLD PIN
    NDIS_VAR_DATA_DESC  NewPIN;                 // describes buffer containing new PIN
} WW_CHANGE_SIM_PIN;


//
// new change SIM PUK structure
//
typedef NDIS_VAR_DATA_DESC      WW_ENABLE_SIM_PUK;


//
// OID_WW_MET_FUNCTION
//
typedef NDIS_VAR_DATA_DESC      WW_MET_FUNCTION;

//
// OID_WW_TAC_COMPRESSION
//
typedef BOOLEAN WW_TAC_COMPRESSION;             // Determines whether or not network level compression
                                                // is being used.

//
// OID_WW_TAC_SET_CONFIG
//
// The DataTAC OID that referenced this object has been superceeded. The
// definition is still included for historical purposes only and should not
// be used
//
typedef struct _WW_TAC_SETCONFIG
{
    NDIS_VAR_DATA_DESC  RCV_MODE;               // Select confirmed/unconfirmed
                                                // receive mode
    NDIS_VAR_DATA_DESC  TX_CONTROL;             // Enable or Disable transmitter
    NDIS_VAR_DATA_DESC  RX_CONTROL;             // Enable or disable radio in
                                                // the modem
    NDIS_VAR_DATA_DESC  FLOW_CONTROL;           // Set flow control between DTE
                                                // and DCE
    NDIS_VAR_DATA_DESC  RESET_CNF;              // Reset configuration to
                                                // default
    NDIS_VAR_DATA_DESC  READ_CNF;               // Read the current
                                                // configuration
} WW_TAC_SETCONFIG, *PWW_TAC_SETCONFIG;

//
// OID_WW_TAC_GET_STATUS
//
// The DataTAC OID that referenced this object has been superceeded. The
// definition is still included for historical purposes only and should not
// be used
//
typedef struct _WW_TAC_GETSTATUS
{
    BOOLEAN                 Action;             // Set = Execute command.
    NDIS_VAR_DATA_DESC      Command;
    NDIS_VAR_DATA_DESC      Option;
    NDIS_VAR_DATA_DESC      Response;           // The response to the requested command
                                                // - max. length of string is 256 octets.
} WW_TAC_GETSTATUS, *PWW_TAC_GETSTATUS;

//
// OID_WW_TAC_USER_HEADER
//
typedef NDIS_VAR_DATA_DESC  WW_TAC_USERHEADER;  // This will hold the user header - Max. 64 octets.

// August 25, 1998 @14:16 EDT by Emil Sturniolo - WRQ
// added new DataTAC get response structure
typedef  struct _WW_TAC_GET_RESPONSE
{
    UINT                SDUTag;                 // previousl assigned token
    NDIS_VAR_DATA_DESC  Response;               // response - max 2048 octets
} WW_TAC_GET_RESPONSE;

//
// DataTAC disable receiver structure
//
typedef INT WW_TAC_DISABLE_RECEIVER;            // 0 = receiver enabled
                                                // 1 = receiver disabled
                                                // -1 = state of recevier unknown

//
// DataTAC antenna mode structure
//
typedef INT WW_TAC_ANTENNA_MODE;                // 0 = Automatic Antenna selection
                                                // 1 = Always use primary antenna
                                                // 2 = Always use secondary antenna
                                                // -1 = Antenna algorithm unknown

//
// DataTAC get response structure
//
typedef INT WW_TAC_FLUSH_DATA;                  // 1 = flush buffered data destine to net
                                                // 2 = flush buffered data received from net
                                                // 3 = flush all buffered data

//
// DataTAC shutdown device structure
//
typedef INT WW_TAC_SHUTDOWN_DEVICE;             // 0 = device enabled
                                                // 1 = device disabled
                                                // -1 = state of device unknown

//
// DataTAC transmitter keyed structure
//
typedef BOOLEAN WW_TAC_TRANSMITTER_KEYED;

//
// added new DataTAC system table structure
//
typedef struct _WW_TAC_SYSTEM_TABLE
{
    UINT        SystemCount;
    UINT8       ContryTable[32];
    UINT8       PrefixTable[32];
    UINT8       IDTable[32];
} WW_TAC_SYSTEM_TABLE;

//
// added new DataTAC channel table structure
//
typedef struct _WW_TAC_CHANNEL_TABLE
{
    UINT        ChannelCount;
    UINT8       ChannelTable[64];
    UINT8       AttrTable[64];
} WW_TAC_CHANNEL_TABLE;


//
// added new DataTAC statistics structure
//
typedef NDIS_VAR_DATA_DESC WW_TAC_STATISTICS;



//
// OID_WW_ARD_SNDCP
//
// The ARDIS OIDs that referenced these object have been deprecated and merged
// with the new DataTAC objects. Their definition are still included for
// historical purposes only and should not be used.
//
typedef struct _WW_ARD_SNDCP
{
    NDIS_VAR_DATA_DESC  Version;                // The version of SNDCP protocol supported.
    INT                 BlockSize;              // The block size used for SNDCP
    INT                 Window;                 // The window size used in SNDCP
} WW_ARD_SNDCP, *PWW_ARD_SNDCP;

//
// OID_WW_ARD_TMLY_MSG
//
typedef BOOLEAN WW_ARD_CHANNEL_STATUS;          // The current status of the inbound RF Channel.

//
// OID_WW_ARD_DATAGRAM
//
typedef struct _WW_ARD_DATAGRAM
{
    BOOLEAN             LoadLevel;              // Byte that contains the load level info.
    INT                 SessionTime;            // Datagram session time remaining.
    NDIS_VAR_DATA_DESC  HostAddr;               // Host address.
    NDIS_VAR_DATA_DESC  THostAddr;              // Test host address.
} WW_ARD_DATAGRAM, *PWW_ARD_DATAGRAM;

//
// OID_WW_CDPD_SPNI
//
typedef struct _WW_CDPD_SPNI
{
    UINT32   SPNI[10];                           //10 16-bit service provider network IDs
    INT     OperatingMode;                      // 0 = ignore SPNI,
                                                // 1 = require SPNI from list,
                                                // 2 = prefer SPNI from list.
                                                // 3 = exclude SPNI from list.
} WW_CDPD_SPNI, *PWW_CDPD_SPNI;

//
// OID_WW_CDPD_WASI
//
typedef struct _WW_CDPD_WIDE_AREA_SERVICE_ID
{
    UINT32   WASI[10];                           //10 16-bit wide area service IDs
    INT     OperatingMode;                      // 0 = ignore WASI,
                                                // 1 = Require WASI from list,
                                                // 2 = prefer WASI from list
                                                // 3 = exclude WASI from list.
} WW_CDPD_WIDE_AREA_SERVICE_ID, *PWW_CDPD_WIDE_AREA_SERVICE_ID;

//
// OID_WW_CDPD_AREA_COLOR
//
typedef INT     WW_CDPD_AREA_COLOR;

//
// OID_WW_CDPD_TX_POWER_LEVEL
//
typedef UINT32   WW_CDPD_TX_POWER_LEVEL;

//
// OID_WW_CDPD_EID
//
typedef NDIS_VAR_DATA_DESC  WW_CDPD_EID;
//
// OID_WW_CDPD_HEADER_COMPRESSION
//
typedef INT WW_CDPD_HEADER_COMPRESSION;         //  0 = no header compression,
                                                //  1 = always compress headers,
                                                //  2 = compress headers if MD-IS does
                                                // -1 = unknown

//
// OID_WW_CDPD_DATA_COMPRESSION
//
typedef INT WW_CDPD_DATA_COMPRESSION;           // 0  = no data compression,
                                                // 1  = data compression enabled
                                                // -1 = unknown

//
// OID_WW_CDPD_CHANNEL_SELECT
//
typedef struct _WW_CDPD_CHANNEL_SELECT
{
    UINT32   ChannelID;                          // channel number
    UINT32   fixedDuration;                      // duration in seconds
} WW_CDPD_CHANNEL_SELECT, *PWW_CDPD_CHANNEL_SELECT;

//
// OID_WW_CDPD_CHANNEL_STATE
//
typedef enum _WW_CDPD_CHANNEL_STATE
{
    CDPDChannelNotAvail,
    CDPDChannelScanning,
    CDPDChannelInitAcquired,
    CDPDChannelAcquired,
    CDPDChannelSleeping,
    CDPDChannelWaking,
    CDPDChannelCSDialing,
    CDPDChannelCSRedial,
    CDPDChannelCSAnswering,
    CDPDChannelCSConnected,
    CDPDChannelCSSuspended
} WW_CDPD_CHANNEL_STATE, *PWW_CDPD_CHANNEL_STATE;

//
// OID_WW_CDPD_NEI
//
typedef enum _WW_CDPD_NEI_FORMAT
{
    CDPDNeiIPv4,
    CDPDNeiCLNP,
    CDPDNeiIPv6
} WW_CDPD_NEI_FORMAT, *PWW_CDPD_NEI_FORMAT;

typedef enum _WW_CDPD_NEI_TYPE
{
    CDPDNeiIndividual,
    CDPDNeiMulticast,
    CDPDNeiBroadcast
} WW_CDPD_NEI_TYPE;


typedef struct _WW_CDPD_NEI
{
    UINT32               uNeiIndex;
    WW_CDPD_NEI_FORMAT  NeiFormat;
    WW_CDPD_NEI_TYPE    NeiType;
    WORD                NeiGmid;                // group member identifier, only
                                                // meaningful if NeiType ==
                                                // CDPDNeiMulticast
    NDIS_VAR_DATA_DESC  NeiAddress;
} WW_CDPD_NEI;

//
// OID_WW_CDPD_NEI_STATE
//

typedef enum _WW_CDPD_NEI_STATE
{
    CDPDUnknown,
    CDPDRegistered,
    CDPDDeregistered
} WW_CDPD_NEI_STATE, *PWW_CDPD_NEI_STATE;

typedef enum _WW_CDPD_NEI_SUB_STATE
{
    CDPDPending,                                // Registration pending
    CDPDNoReason,                               // Registration denied - no reason given
    CDPDMDISNotCapable,                         // Registration denied - MD-IS not capable of
                                                //  handling M-ES at this time
    CDPDNEINotAuthorized,                       // Registration denied - NEI is not authorized to
                                                //  use this subnetwork
    CDPDInsufficientAuth,                       // Registration denied - M-ES gave insufficient
                                                //  authentication credentials
    CDPDUnsupportedAuth,                        // Registration denied - M-ES gave unsupported
                                                //  authentication credentials
    CDPDUsageExceeded,                          // Registration denied - NEI has exceeded usage
                                                //  limitations
    CDPDDeniedThisNetwork                       // Registration denied on this network, service
                                                //  may be obtained on alternate Service Provider
                                                //  network
} WW_CDPD_NEI_SUB_STATE;

typedef struct _WW_CDPD_NEI_REG_STATE
{
    UINT32               uNeiIndex;
    WW_CDPD_NEI_STATE   NeiState;
    WW_CDPD_NEI_SUB_STATE NeiSubState;
} WW_CDPD_NEI_REG_STATE, *PWW_CDPD_NEI_REG_STATE;

//
// OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER
//
typedef struct _WW_CDPD_SERVICE_PROVIDER_ID
{
    UINT32   SPI[10];                            //10 16-bit service provider IDs
    INT     OperatingMode;                      // 0 = ignore SPI,
                                                // 1 = require SPI from list,
                                                // 2 = prefer SPI from list.
                                                // 3 = SPI from list is excluded
} WW_CDPD_SERVICE_PROVIDER_ID, *PWW_CDPD_SERVICE_PROVIDER_ID;

//
// OID_WW_CDPD_SLEEP_MODE
//
typedef INT WW_CDPD_SLEEP_MODE;

//
// OID_WW_CDPD_TEI
//
typedef UINT32   WW_CDPD_TEI;

//
// OID_WW_CDPD_CIRCUIT_SWITCHED
//
// The CDPD OID that referenced this object has been deprecated and superceeded
// by new discrete CDPD objects. The definition is still included for
// historical purposes only and should not be used.
//
typedef struct _WW_CDPD_CIRCUIT_SWITCHED
{
    INT                 service_preference;  // -1 = unknown,
                                                //  0 = always use packet switched CDPD,
                                                //  1 = always use CS CDPD via AMPS,
                                                //  2 = always use CS CDPD via PSTN,
                                                //  3 = use circuit switched via AMPS only
                                                //  when packet switched is not available.
                                                //  4 = use packet switched only when circuit
                                                //  switched via AMPS is not available.
                                                //  5 = device manuf. defined service
                                                //  preference.
                                                //  6 = device manuf. defined service
                                                //  preference.
    
    INT                 service_status;         // -1 = unknown,
                                                //  0 = packet switched CDPD,
                                                //  1 = circuit switched CDPD via AMPS,
                                                //  2 = circuit switched CDPD via PSTN.
    
    INT                 connect_rate;           //  CS connection bit rate (bits per second).
                                                //  0 = no active connection,
                                                // -1 = unknown

                                                //  Dial code last used to dial.
    NDIS_VAR_DATA_DESC  dial_code[20];
    
    UINT32               sid;                    //  Current AMPS system ID
    
    INT                 a_b_side_selection;     // -1 = unknown,
                                                //  0 = no AMPS service
                                                //  1 = AMPS "A" side channels selected
                                                //  2 = AMPS "B" side channels selected
    
    INT                 AMPS_channel;           // -1= unknown
                                                //  0 = no AMPS service.
                                                //  1-1023 = AMPS channel number in use
    
    UINT32               action;                 //  0 = no action
                                                //  1 = suspend (hangup)
                                                //  2 = dial
    
                                                //  Default dial code for CS CDPD service
                                                //  encoded as specified in the CS CDPD
                                                //  implementor guidelines.
    NDIS_VAR_DATA_DESC  default_dial[20];
    
                                                //  Number for the CS CDPD network to call
                                                //  back the mobile, encoded as specified in
                                                //  the CS CDPD implementor guidelines.
    NDIS_VAR_DATA_DESC  call_back[20];
    
    UINT32               sid_list[10];           //  List of 10 16-bit preferred AMPS
                                                //  system IDs for CS CDPD.
    
    UINT32               inactivity_timer;       //  Wait time after last data before dropping
                                                //  call.
                                                //  0-65535 = inactivity time limit (seconds).
    
    UINT32               receive_timer;          //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               conn_resp_timer;        //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               reconn_resp_timer;      //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               disconn_timer;          //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               NEI_reg_timer;          //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               reconn_retry_timer;     //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               link_reset_timer;       //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               link_reset_ack_timer;   //  secs. per CS-CDPD Implementor Guidelines.
    
    UINT32               n401_retry_limit;       //  per CS-CDPD Implementor Guidelines.
    
    UINT32               n402_retry_limit;       //  per CS-CDPD Implementor Guidelines.
    
    UINT32               n404_retry_limit;       //  per CS-CDPD Implementor Guidelines.
    
    UINT32               n405_retry_limit;       //  per CS-CDPD Implementor Guidelines.
} WW_CDPD_CIRCUIT_SWITCHED, *WW_PCDPD_CIRCUIT_SWITCHED;

typedef UINT32   WW_CDPD_RSSI;

//
// cs-cdpd service preference structure
//
typedef INT WW_CDPD_CS_SERVICE_PREFERENCE;      // 0 = use packet switched CDPD only
                                                // 1 = use CS-CDPD via AMPS only
                                                // 2 = use CS-CDPD via PSTN only
                                                // 3 = use CS-CDPD via AMPS only
                                                //     when packet switched is N/A
                                                // 4 = use packet switched CDPD only
                                                //     when  CS-CDPD via AMPS is N/A
                                                // 5 = Device manufacture defined
                                                //     service preference
                                                // 6 = device manufacture defined
                                                //     service preference
                                                // -1 = unknown

//
// cs-cdpd service status structure
//
typedef INT WW_CDPD_CS_SERVICE_STATUS;          // 0 = Packet switched CDPD
                                                // 1 = CS-CDPD via AMPS
                                                // 2 = CS-CDPD via PSTN
                                                // -1 = unknown



//
// cs-cdpd info structure
//
typedef struct _WW_CDPD_CS_INFO {
    INT                 ConnectRage;            // 0 = no active connection
                                                // -1 = unknown
                                                // all other values represent BPS
    NDIS_VAR_DATA_DESC  DialCode;               // describes buffer of last dial code
    UINT                SID;                    // Current AMPS System ID
    INT                 ABSideSelection;        // 0 = no AMPS service
                                                // 1 = AMPS "A" side channel selected
                                                // 2 = AMPS "B" side channel selected
    INT                 AMPSChannel;            // 0 = no AMPS service
                                                // 1-1023 = current AMPS channel
                                                // -1 = Unknown
                                                // all other values reserved
} WW_CDPD_CS_INFO;



//
// cs-cdpd suspend structure
//
typedef UINT WW_CDPD_CS_SUSPEND;                // 0 = nop; 1 = hang up


//
// cs-cdpd default dial code structure
//
typedef NDIS_VAR_DATA_DESC WW_CDPD_DEFAULT_DIAL_CODE;   // max 20 octets

//
// cs-cdpd callback structure
//
typedef struct _WW_CDPD_CS_CALLBACK
{
    UINT                Enabled;                // 0 = disable; 1 = enable; -1 = unknown
    NDIS_VAR_DATA_DESC  Number;                 // descibes buffer contianing dial code
                                                // max 20 octets
} WW_CDPD_CS_CALLBACK;


//
// cs-cdpd system id list structure
//
typedef struct _WW_CDPD_CS_SID_LIST
{
    UINT    AMPSystemId[10];
} WW_CDPD_CS_SID_LIST;

//
// cs-cdpd configuration structure
//
typedef struct _WW_CDPD_CS_CONFIGURATION
{
    UINT    InactivityTimer;                    // in seconds
    UINT    ReceiveTimer;                       // in seconds
    UINT    ConnResTimer;                       // in seconds
    UINT    ReconnRespTimer;                    // in seconds
    UINT    DisconnTimer;                       // in seconds
    UINT    NEIRegTimer;                        // in seconds
    UINT    ReconnRetryTimer;                   // in seconds
    UINT    LinkResetTimer;                     // in seconds
    UINT    LinkResetAckTimer;                  // in seconds
    UINT    n401RetryLimit;                     // per CS-CDPD Implementers guidelines
    UINT    n402RetryLimit;                     // per CS-CDPD Implementers guidelines
    UINT    n404RetryLimit;                     // per CS-CDPD Implementers guidelines
    UINT    n405RetryLimit;                     // per CS-CDPD Implementers guidelines
} WW_CDPD_CS_CONFIGURATION;


//
// OID_WW_PIN_LOC_AUTHORIZE
//
// The Pin Point OIDs that referenced the structures below have been
// deprecated from the PCCA STD-201 standard. Their definitions are still
// included for historical purposes only and should not be used.
//
typedef INT WW_PIN_AUTHORIZED;                  // 0  = unauthorized
                                                // 1  = authorized
                                                // -1 = unknown

//
// OID_WW_PIN_LAST_LOCATION
// OID_WW_PIN_LOC_FIX
//
typedef struct _WW_PIN_LOCATION
{
    INT     Latitude;                           // Latitude in hundredths of a second
    INT     Longitude;                          // Longitude in hundredths of a second
    INT     Altitude;                           // Altitude in feet
    INT     FixTime;                            // Time of the location fix, since midnight,  local time (of the
                                                // current day), in tenths of a second
    INT     NetTime;                            // Current local network time of the current day, since midnight,
                                                // in tenths of a second
    INT     LocQuality;                         // 0-100 = location quality
    INT     LatReg;                             // Latitude registration offset, in hundredths of a second
    INT     LongReg;                            // Longitude registration offset, in hundredths of a second
    INT     GMTOffset;                          // Offset in minutes of the local time zone from GMT
} WW_PIN_LOCATION, *PWW_PIN_LOCATION;


//
// The following is set on a per-packet basis as OOB data with NdisClassWirelessWanMbxMailbox
//
typedef UINT32   WW_MBX_MAILBOX_FLAG;            // 1 = set mailbox flag, 0 = do not set mailbox flag

//
// OID_WW_MBX_SUBADDR
//
typedef struct _WW_MBX_PMAN
{
    BOOLEAN             ACTION;                 // 0 = Login PMAN,  1 = Logout PMAN
    UINT32               MAN;
    UINT8               PASSWORD[8];            // Password should be null for Logout and indications.
                                                // Maximum length of password is 8 chars.
} WW_MBX_PMAN, *PWW_MBX_PMAN;

//
// OID_WW_MBX_FLEXLIST
//
typedef struct  _WW_MBX_FLEXLIST
{
    INT     count;                              //  Number of MAN entries used.
                                                // -1=unknown.
    UINT32   MAN[7];                             //  List of MANs.
} WW_MBX_FLEXLIST;

//
// OID_WW_MBX_GROUPLIST
//
typedef struct  _WW_MBX_GROUPLIST
{
    INT  count;                                 //  Number of MAN entries used.
                                                // -1=unknown.
    UINT32   MAN[15];                            //  List of MANs.
} WW_MBX_GROUPLIST;

//
// OID_WW_MBX_TRAFFIC_AREA
//
typedef enum    _WW_MBX_TRAFFIC_AREA
{
    unknown_traffic_area,                       // The driver has no information about the current traffic area.
    in_traffic_area,                            // Mobile unit has entered a subscribed traffic area.
    in_auth_traffic_area,                       // Mobile unit is outside traffic area but is authorized.
    unauth_traffic_area                         // Mobile unit is outside traffic area but is un-authorized.
} WW_MBX_TRAFFIC_AREA;

//
// OID_WW_MBX_LIVE_DIE
//
typedef INT WW_MBX_LIVE_DIE;                    //  0 = DIE last received   
                                                //  1 = LIVE last received
                                                // -1 = unknown

//
// OID_WW_MBX_TEMP_DEFAULTLIST
//
typedef struct _WW_MBX_CHANNEL_PAIR
{
    UINT32               Mobile_Tx;
    UINT32               Mobile_Rx;
} WW_MBX_CHANNEL_PAIR, *PWW_MBX_CHANNEL_PAIR;

typedef struct _WW_MBX_TEMPDEFAULTLIST
{
    UINT32               Length;
    WW_MBX_CHANNEL_PAIR ChannelPair[1];
} WW_MBX_TEMPDEFAULTLIST, *WW_PMBX_TEMPDEFAULTLIST;

#endif //if 0 wumin

#endif // WIRELESS_WAN


#endif//#if defined(NDIS50_MINIPORT)


//#pragma pack(pop)



#endif // _NTDDNDIS_50_




