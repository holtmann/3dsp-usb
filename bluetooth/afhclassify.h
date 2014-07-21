/************************************************************************
* Copyright (c) 2006, 3DSP Corporation, all rights reserved.  Duplication or
* reproduction of any part of this software (source code, object code or
* comments) without the expressed written consent by 3DSP Corporation is
* forbidden.  For further information please contact:
*
* 3DSP Corporation
* 16271 Laguna Canyon Rd
* Irvine, CA 92618
* www.3dsp.com
*************************************************************************/
/*
	File name:	cvsd.h
	Version:	v1.0 Jan.06/2006
	Programmer: Feng Wu
	First Version: v1.0 Jan.06/2006
*/
#ifndef __afhclassify__
#define __afhclassify__

#define PLRTHRESHOLD		10
#define NGREQMIN			20 
#define QTHRESHOLD			1//172			// [256*(2/3)]
#define ATTENUATE_INDEX		1
#define NSLAVE				2
#define NMIN				22          //the frequency used must more than 20

//
// This structure contains the information about test module
// 
//
typedef struct _BT_AFH
{
	// Spin lock for task module
	KSPIN_LOCK     lock;
	int BPLR_Table[20];
	int BPLR_Table2[80];
	int AHF_MAP[3];	
	int change_map;
	int Prev_AHF_MAP[3];
	int  classification_change_map;
	unsigned char AFH_classification[10];
	unsigned char Prev_AFH_classification[10];
	unsigned char AFH_classification_S[NSLAVE][10];
} BT_AFH_T, *PBT_AFH_T;

NTSTATUS
Afh_Init(PBT_DEVICE_EXT devExt);
NTSTATUS Afh_Release(PBT_DEVICE_EXT devExt);
VOID Afh_Statistic(PBT_DEVICE_EXT devExt, unsigned char badfreq);
UINT8 Afh_GetMap(PBT_DEVICE_EXT devExt, PUINT8 destmap);
UINT8 Afh_GetClassification(PBT_DEVICE_EXT devExt, PUINT8 destmap);
PUINT8 Afh_GetSlaveClaAddress(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice);
VOID Afh_ClearPrevAfhMap(PBT_DEVICE_EXT devExt);

void BlockPLRstatistic(unsigned char badfreq,int* pBPLR_Table2);
void AFHClassify(unsigned char* pAFH_classification,int* pBPLR_Table,unsigned char * Prev_pAFH_classification,int* classification_change_map,int* pBPLR_Table2);
void AFHMAPGen(int* pAFH_MAP,unsigned char* pAFH_classification,unsigned char* pAFH_classification_S,int* Prev_AHF_MAP,int* change_map);
void pAFH_MIN_NUM_USED(unsigned char* pAFH_classification,int* pBPLR_Table,int* pBPLR_Table2);
//int pBPLR_Tablemin(int* pBPLR_Table);
//int pBPLR_Tablemax(int* pBPLR_Table);
#endif
