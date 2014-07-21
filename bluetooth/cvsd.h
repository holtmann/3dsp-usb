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
#ifndef __cvsd__
#define __cvsd__

// #define HH           32.0
// #define Beida        1024.0
#define HH           32
#define Beida        1024
#define Delta_Min    (10*1024)
#define Delta_Max    (1280*1024)
// #define Y_Min        -4194303  //(-2^22+1)
// #define Y_Max        4194303
#define Y_Min        -2097151  //(-2^22+1)
#define Y_Max        2097151
#define MaxProcLen   640  // bit
#define Sample_Len   4
#define Sample_Rate  8  // The ratio between the 64 KHz sample rate and original signal frequency.
// #define Sample_Period (1.0/Sample_Rate)

#define UP_SAMPLE_RATE 8

#define QuanMax      32768

void CVSD_Encoder(int *TxIn, int *TxOut, int ProcessLen);
void CVSD_Decoder(int *RxIn, int *RxOut, int ProcessLen);
int CVSD_Step_Control(int Delta, int CurrBit);
int CVSD_Accumulator(int CurrBit, int Delta, int PredValue);
int CVSD_Step_Control_Rx(int Delta, int CurrBit);
int Min(int Para1, int Para2);
int Max(int Para1, int Para2);
int sgn(int Para1);
void UpFIR(int *TxFIRIn, int *EnCVSDIn, int Lgh);

unsigned char linear2cvsd(short pcm_val);
short cvsd2linear(unsigned char a_val);

void cvsd2lineartest(unsigned char a_val, short *CVSDout);

#endif
