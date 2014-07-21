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
File name:	CVSD_Encoder.c
Object:		CVSD Encoder.
Input:		TxIn: a set of 16 bit signed samples.
ProcessLen: the length of samples to be encoded.
Output:		TxOut: the bits after being coded by CVSD encoder.
Function:	Implement CVSD encoding, where the input is 64 ksamples/s linear PCM, the output is 64kb/s coded data.
Version:	v1.0 Jan.06/2006
Programmer: Feng Wu
First Version: v1.0 Jan.06/2006
 */
#include "bt_sw.h"
#include "cvsd.h"
void CVSD_Encoder(int *TxIn, int *TxOut, int ProcessLen)
{
	static int TxPredValue = 0, TxDelta = Delta_Min;
	int i;
	for (i = 0; i < ProcessLen; i++)
	{
		//===========For detail, please refer figure 12.1 at page 140 of Bluetooth Specification 1.0==========
		TxOut[i] = sgn(TxIn[i] - TxPredValue);
		TxDelta = CVSD_Step_Control(TxDelta, TxOut[i]);
		TxPredValue = CVSD_Accumulator(TxOut[i], TxDelta, TxPredValue);
		TxOut[i] = (int)(TxOut[i] ==  - 1);
	}
}

/*
File name:	CVSD_DeCoding.c
Object:		CVSD Decoder.
Input:		RxIn: the bits to be decoded by CVSD decoder.
ProcessLen: the length of bits to be decoded.
Output:		RxOut: the samples of 16 bits after being decoded by CVSD decoder.
Function:	Implement CVSD decoding, where the input is 64kb/s coded data and the output is 64 ksamples/s linear PCM samples.
.
Version:	v1.0 Jan.06/2006
Programmer: Feng Wu
First Version: v1.0 Jan.06/2006
 */
void CVSD_Decoder(int *RxIn, int *RxOut, int ProcessLen)
{
	static int RxPredValue = 0, RxDelta = Delta_Min;
	int tmpint;
	int i;
	for (i = 0; i < ProcessLen; i++)
	{
		//===========For details, please refer to figure 12.2 at page 140 of Bluetooth Specification 1.0==========
		// RxIn[i]     = (int)(2*((RxIn[i]==0)-0.5));
		tmpint = RxIn[i] == 0 ? 1 :  - 1;
		RxOut[i] = RxPredValue;
		RxDelta = CVSD_Step_Control_Rx(RxDelta, tmpint);
		RxPredValue = CVSD_Accumulator(tmpint, RxDelta, RxPredValue);
	}
}

/*
File name:	CVSD_tools.c
Object:		CVSD step control and accumulator function.
Version:	v1.0 Jan.06/2006
Programmer: Feng Wu
First Version: v1.0 Jan.06/2006
 */
int CVSD_Step_Control(int Delta, int CurrBit)
{
	static int Last4bits[3] =
	{
		1,  - 1, 1
	}; // Last3bits adjusts the Alfa in the step size control module.
	int Alfa = 0, Sum_JJ, output, Para1, Para2;
	//The following instructions is to obtain Alfa according to (EQ 16) at page 141
	Sum_JJ = Last4bits[0] + Last4bits[1];
	Sum_JJ += Last4bits[2] + CurrBit;
	Last4bits[0] = Last4bits[1];
	Last4bits[1] = Last4bits[2];
	Last4bits[2] = CurrBit;
	if ((Sum_JJ == 4) || (Sum_JJ ==  - 4))
	{
		Alfa = 1;
	}
	//The remain instructions is to calculate Beida according to (EQ 17) at page 141
	if (Alfa == 0)
	{
		Para1 = (int)(Delta - Delta / Beida); //Beida*Delta;
		Para2 = Delta_Min;
		output = Max(Para1, Para2);
	}
	else
	{
		Para1 = Delta + Delta_Min;
		Para2 = Delta_Max;
		output = Min(Para1, Para2);
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
int CVSD_Step_Control_Rx(int Delta, int CurrBit)
{
	static int Last4bitsRx[3] =
	{
		1,  - 1, 1
	}; // Last3bits adjusts the Alfa in the step size control module.
	int Alfa = 0, Sum_JJ, output, Para1, Para2;
	//The following instructions is to obtain Alfa according to (EQ 16) at page 141
	Sum_JJ = Last4bitsRx[0] + Last4bitsRx[1];
	Sum_JJ += Last4bitsRx[2] + CurrBit;
	Last4bitsRx[0] = Last4bitsRx[1];
	Last4bitsRx[1] = Last4bitsRx[2];
	Last4bitsRx[2] = CurrBit;
	if ((Sum_JJ == 4) || (Sum_JJ ==  - 4))
	{
		Alfa = 1;
	}
	//The remain instructions is to calculate Beida according to (EQ 17) at page 141
	if (Alfa == 0)
	{
		Para1 = (int)(Delta - Delta / Beida); //Beida*Delta;
		Para2 = Delta_Min;
		output = Max(Para1, Para2);
	}
	else
	{
		Para1 = Delta + Delta_Min;
		Para2 = Delta_Max;
		output = Min(Para1, Para2);
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
int CVSD_Accumulator(int CurrBit, int Delta, int PredValue)
{
	int output, Para2;
	//Please refer Figure 12.3 at page 140 for more details
	output = CurrBit * Delta / 16;
	output += PredValue; // Calculate predicted y(k) from (EQ 19) ata page 141
	//The following instructions is to obtain y(k) according to (EQ 18) at page 141
	if (output >= 0)
	{
		Para2 = Y_Max;
		output = Min(output, Para2);
	}
	else
	{
		Para2 = Y_Min;
		output = Max(output, Para2);
	}
	output = (int)((HH *output - output) / HH);
	return output;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
int sgn(int Para)
{
	int output;
	if (Para >= 0)
	{
		output = 1;
	}
	else
	{
		output =  - 1;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
int Min(int Para1, int Para2)
{
	int output;
	if (Para1 >= Para2)
	{
		output = Para2;
	}
	else
	{
		output = Para1;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
int Max(int Para1, int Para2)
{
	int output;
	if (Para1 >= Para2)
	{
		output = Para1;
	}
	else
	{
		output = Para2;
	}
	return output;
}

/*
File name:	CVSD_tools.c
Object:		UpFIR.
Input:		TxIn: a set of 16 bit signed samples to be processed.
Lgh: the length of samples to be upsampled.
Output:		EnCVSDIn: the samples of 16 bits after being upsampling filtered. Output 8 samples for each inputted sample.
Function:	Upsample the input voice PCM signals by FIR filter.
Version:	v1.0 Jan.09/2006
Programmer: Feng Wu
First Version: v1.0 Jan.09/2006
 */
void UpFIR(int *TxFIRIn, int *EnCVSDIn, int Lgh)
{
	int i, j, k, Len, FIRTemp;
	/**
	static int Filter[24]={414,   894,  1491,  2196,  2979,  3810,  4644,  5433,  6138,  6708,  7110,  \
		    		7317,  7317,  7110,  6708,  6138,  5433,  4644,  3810,  2979,  2196,  1491,   894,   414};
	**/
	static int Filter[64] =
	{
		 - 259,  - 303,  - 302,  - 238,  - 104, 96, 346, 616, 864, 1044, 1105, 1010,  \
			732, 269,  - 352,  - 1077,  - 1822,  - 2484,  - 2946,  - 3091,  - 2820,  - 2059,  - 776, 1012, 3237, 5781, 8485,  \
			11161, 13612, 15647, 17104, 17864, 17864, 17104, 15647, 13612, 11161, 8485, 5781, 3237, 1012,  - 776,  \
			 - 2059,  - 2820,  - 3091,  - 2946,  - 2484,  - 1822,  - 1077,  - 352, 269, 732, 1010, 1105, 1044, 864, 616,  \
			346, 96,  - 104,  - 238,  - 302,  - 303,  - 259
	};
	static int Last2Sample[7] =
	{
		0, 0, 0, 0, 0, 0, 0
	};
	Len = 7;
	for (i = 0; i < Lgh / UP_SAMPLE_RATE; i++)
	{
		for (j = 0; j < UP_SAMPLE_RATE; j++)
		{
			FIRTemp = Filter[j] *TxFIRIn[i];
			for (k = 0; k < Len; k++)
			{
				FIRTemp += Filter[(k + 1) *UP_SAMPLE_RATE + j] *Last2Sample[Len - 1-k];
			}
			EnCVSDIn[i *UP_SAMPLE_RATE + j] = (FIRTemp) >> 8;
		}
		for (k = 0; k < (Len - 1); k++)
		{
			Last2Sample[k] = Last2Sample[k + 1];
		}
		Last2Sample[Len - 1] = TxFIRIn[i];
	}
	/*
	static int Filter[32]={-3300,-1238,-1212,-965,-461,322,1383,2699,4216,5861,7548,9177,10630,11824,12669,\
	13107,13107,12669,11824,10630,9177,7548,5861,4216,2699,1383,322,-461,-965,-1212,-1238,-3300};
	static int Last2Sample[3] = {0,0,0};
	for(i=0; i<Lgh/UP_SAMPLE_RATE; i++)
	{
	for(j=0; j<UP_SAMPLE_RATE; j++)
	{
	FIRTemp  = Filter[UP_SAMPLE_RATE*3+j]*Last2Sample[0];
	FIRTemp += Filter[UP_SAMPLE_RATE*2+j]*Last2Sample[1];
	FIRTemp += Filter[UP_SAMPLE_RATE*1+j]*Last2Sample[2];
	FIRTemp += Filter[j]*TxFIRIn[i];
	EnCVSDIn[i*UP_SAMPLE_RATE+j] = (FIRTemp>>5);
	}
	Last2Sample[0] = Last2Sample[1];
	Last2Sample[1] = Last2Sample[2];
	Last2Sample[2] = TxFIRIn[i];
	}
	 */
	/*
	static int Filter[48]={751,560,665,690,602,382,26,-447,-990,-1535,-1992,-2265,-2259,\
	-1894,-1120,75,1655,3541,5609,7703,9656,11299,12485,13107,13107,12485,11299,9656,\
	7703,5609,3541,1655,75,-1120,-1894,-2259,-2265,-1992,-1535,-990,-447,26,382,602,690,665,560,751};
	static int Last2Sample[5] = {0,0,0,0,0};
	for(i=0; i<Lgh/UP_SAMPLE_RATE; i++)
	{
	for(j=0; j<UP_SAMPLE_RATE; j++)
	{
	FIRTemp  = Filter[UP_SAMPLE_RATE*5+j]*Last2Sample[0];
	FIRTemp += Filter[UP_SAMPLE_RATE*4+j]*Last2Sample[1];
	FIRTemp += Filter[UP_SAMPLE_RATE*3+j]*Last2Sample[2];
	FIRTemp += Filter[UP_SAMPLE_RATE*2+j]*Last2Sample[3];
	FIRTemp += Filter[UP_SAMPLE_RATE*1+j]*Last2Sample[4];
	FIRTemp += Filter[j]*TxFIRIn[i];
	EnCVSDIn[i*UP_SAMPLE_RATE+j] = (FIRTemp>>5);
	}
	Last2Sample[0] = Last2Sample[1];
	Last2Sample[1] = Last2Sample[2];
	Last2Sample[2] = Last2Sample[3];
	Last2Sample[3] = Last2Sample[4];
	Last2Sample[4] = TxFIRIn[i];
	}
	 */
}

/***************************************************************/
/* Following function is added by jason dong
*************************************************************/
unsigned char linear2cvsd(short pcm_val) /* 2's complement (16-bit range) */
{
	unsigned char i;
	int tmpout[8];
	int tmptxout;
	unsigned char retval = 0;
	int tmpin = (int)pcm_val;
	UpFIR(&tmpin, tmpout, UP_SAMPLE_RATE);
	for (i = 0; i < 8; i++)
	{
		CVSD_Encoder(&tmpout[i], &tmptxout, 1);
		if (tmptxout)
		{
			retval |= (0x1 << i);
		}
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
short cvsd2linear(unsigned char a_val)
{
	unsigned char i;
	int tmprxin[8];
	int rxout[8];
	for (i = 0; i < 8; i++)
	{
		tmprxin[i] = (a_val &(0x1 << i)) ? 1 : 0;
	}
	CVSD_Decoder(tmprxin, rxout, 8);
	return ((short)(rxout[3] >> 6));
}
