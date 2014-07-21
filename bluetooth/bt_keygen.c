/***********************************************************************
 * FILENAME:         BT_KeyGen.c
 * CURRENT VERSION:  1.00.01
 * CREATE DATE:      2005/09/06
 * PURPOSE:  Generate the authentication and encryption key.
 *           Described in Baseband Specification Section 14.5
 *           in "Specification of the Bluetooth System Version 1.1".
 * AUTHORS:  Lewis Wang
 *
 * NOTES:    //
 ***********************************************************************/
/*
 *  REVISION HISTORY
 */
#include "bt_sw.h"         /* include <WDM.H> */

/*--file local function prototypes-------------------------------------*/
void KeyOffset(unsigned char *pKey, unsigned char *pKeywave)
{
	/* pKeywave[] = (pKey[] + n) mod 256 */
	pKeywave[0] = (unsigned char)(pKey[0] + 233);
	pKeywave[2] = (unsigned char)(pKey[2] + 223);
	pKeywave[4] = (unsigned char)(pKey[4] + 179);
	pKeywave[6] = (unsigned char)(pKey[6] + 149);
	pKeywave[9] = (unsigned char)(pKey[9] + 229);
	pKeywave[11] = (unsigned char)(pKey[11] + 193);
	pKeywave[13] = (unsigned char)(pKey[13] + 167);
	pKeywave[15] = (unsigned char)(pKey[15] + 131);
	/* pKeywave[] = pKey[] xor n */
	pKeywave[1] = (unsigned char)(pKey[1] ^ 229);
	pKeywave[3] = (unsigned char)(pKey[3] ^ 193);
	pKeywave[5] = (unsigned char)(pKey[5] ^ 167);
	pKeywave[7] = (unsigned char)(pKey[7] ^ 131);
	pKeywave[8] = (unsigned char)(pKey[8] ^ 233);
	pKeywave[10] = (unsigned char)(pKey[10] ^ 223);
	pKeywave[12] = (unsigned char)(pKey[12] ^ 179);
	pKeywave[14] = (unsigned char)(pKey[14] ^ 149);
	return ;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
void Boxes_ee_ll_and_Vectors_BB(unsigned char *ee, unsigned char *ll, unsigned char BB[18][16])
{
	unsigned int xx[257], pp[18], BB0[18][16];
	unsigned int m, n, min;
	unsigned char temp[256];
	xx[0] = 1;
	for (m = 0; m < 256; m++)
	{
		xx[m + 1] = (xx[m] *45) % 257; /* calc (45^m) mod 257 */
		ee[m] = (unsigned char)xx[m]; /* calc ((45^m) mod 257) mod 256 */
		temp[m] = ee[m];
		ll[m] = (unsigned char)m;
	}
	/* temp is sorted from ee in ascending order.
	ll is the corresponding indices in ee. */
	for (m = 0; m < 255; m++)
	{
		min = m;
		for (n = m + 1; n < 256; n++)
			if (temp[n] < temp[min])
		    {
				min = n;
		    }
		if (min != m)
		{
			temp[min] = temp[m];
			temp[m] = ll[m];
			ll[m] = ll[min];
			ll[min] = temp[m];
		}
	}
	pp[0] = 1;
	for (m = 1; m < 18; m++)
	{
		pp[m] = (pp[m - 1] *103) % 257; /* calc (45^(17m)) mod 257 */
		for (n = 0; n < 16; n++)
		{
			BB0[m][n] = (pp[m] *xx[n + 1]) % 257; /* calc (45^(17m+n+1)) mod 257 */
			BB[m][n] = (unsigned char)xx[BB0[m][n]]; /* calc ((45^((45^(17m+n+1)) mod 257)) mod 257) mod 256 */
		}
	}
	return ;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
void KeySchedule(unsigned char *KeyIn, unsigned char BB[18][16], unsigned char KK[18][16])
{
	unsigned char Key[17], temp;
	unsigned int m, n;
	Key[16] = 0;
	for (n = 0; n < 16; n++)
	{
		Key[n] = KeyIn[n];
		Key[16] = (unsigned char)(Key[16] ^ KeyIn[n]); /* sum octets bit_by_bit modulo 2 */
		KK[1][n] = Key[n];
	}
	for (m = 2; m < 18; m++)
	{
		/* rotate each octet left by 3 bit positions */
		for (n = 0; n < 17; n++)
		{
			temp = (unsigned char)((Key[n] << 3) &0xf8);
			Key[n] = (unsigned char)((Key[n] >> 5) &0x07);
			Key[n] = (unsigned char)(Key[n] | temp);
		}
		for (n = 0; n < 16; n++)
		{
			KK[m][n] = (unsigned char)(Key[(n + m - 1) % 17] + BB[m][n]);
		}
	}
	return ;
}

///////////////////////////////////////////////////////////////////////////
//                                                                      ///
///////////////////////////////////////////////////////////////////////////
void Ar_or_Arhat_RandGen(unsigned char flag, unsigned char *Rand, unsigned char *ee, unsigned char *ll, unsigned char KK[18][16], unsigned char *Rand0)
{
	unsigned int order[16] =
	{
		8, 11, 12, 15, 2, 1, 6, 5, 10, 9, 14, 13, 0, 7, 4, 3
	};
	unsigned int m, n, k, num;
	unsigned char temp1, temp2;
	unsigned char temp[16];
	/* assign Rand to Rand0 */
	for (m = 0; m < 16; m++)
	{
		Rand0[m] = Rand[m];
	}
	/* 8 rounds for Rand generating */
	for (n = 1; n <= 8; n++)
	{
		/* flag=1 identifies this is Arhat Rand generating */
		if (flag == 1 && n == 3)
		{
			for (m = 1; m < 16; m += 4)
			{
				/* calc sets 1 5 9 13 */
				num = m;
				Rand0[num] = (unsigned char)(Rand0[num] + Rand[num]);
				/* calc sets 2 6 10 14 */
				num = m + 1;
				Rand0[num] = (unsigned char)(Rand0[num] + Rand[num]);
				/* calc sets 3 7 11 15 */
				num = m + 2;
				Rand0[num] = (unsigned char)(Rand0[num] ^ Rand[num]);
				/* calc sets 4 8 12 0 */
				num = (m + 3) % 16;
				Rand0[num] = (unsigned char)(Rand0[num] ^ Rand[num]);
			}
		}
		for (m = 1; m < 16; m += 4)
		{
			/* calc sets 1 5 9 13 */
			num = m;
			Rand0[num] = (unsigned char)(Rand0[num] + KK[2 *n - 1][num]);
			Rand0[num] = ll[Rand0[num]];
			Rand0[num] = (unsigned char)(Rand0[num] ^ KK[2 *n][num]);
			/* calc sets 2 6 10 14 */
			num = m + 1;
			Rand0[num] = (unsigned char)(Rand0[num] + KK[2 *n - 1][num]);
			Rand0[num] = ll[Rand0[num]];
			Rand0[num] = (unsigned char)(Rand0[num] ^ KK[2 *n][num]);
			/* calc sets 3 7 11 15 */
			num = m + 2;
			Rand0[num] = (unsigned char)(Rand0[num] ^ KK[2 *n - 1][num]);
			Rand0[num] = ee[Rand0[num]];
			Rand0[num] = (unsigned char)(Rand0[num] + KK[2 *n][num]);
			/* calc sets 4 8 12 0 */
			num = (m + 3) % 16;
			Rand0[num] = (unsigned char)(Rand0[num] ^ KK[2 *n - 1][num]);
			Rand0[num] = ee[Rand0[num]];
			Rand0[num] = (unsigned char)(Rand0[num] + KK[2 *n][num]);
		}
		/* calc PHT */
		for (m = 0; m < 8; m++)
		{
			temp1 = Rand0[2 *m];
			temp2 = Rand0[2 *m + 1];
			Rand0[2 *m] = (unsigned char)(2 *temp1 + temp2);
			Rand0[2 *m + 1] = (unsigned char)(temp1 + temp2);
		}
		for (k = 1; k < 4; k++)
		{
			/* permute by order[] */
			for (m = 0; m < 16; m++)
			{
				temp[m] = Rand0[m];
			}
			for (m = 0; m < 16; m++)
			{
				Rand0[m] = temp[order[m]];
			}
			/* calc PHT */
			for (m = 0; m < 8; m++)
			{
				temp1 = Rand0[2 *m];
				temp2 = Rand0[2 *m + 1];
				Rand0[2 *m] = (unsigned char)(2 *temp1 + temp2);
				Rand0[2 *m + 1] = (unsigned char)(temp1 + temp2);
			}
		} /* end for (k = 1; k < 4; k++) */
	} /* end for (n = 1; n <= 8; n++) */
	for (m = 1; m < 16; m += 4)
	{
		/* calc sets 1 5 9 13 */
		num = m;
		Rand0[num] = (unsigned char)(Rand0[num] + KK[17][num]);
		/* calc sets 2 6 10 14 */
		num = m + 1;
		Rand0[num] = (unsigned char)(Rand0[num] + KK[17][num]);
		/* calc sets 3 7 11 15 */
		num = m + 2;
		Rand0[num] = (unsigned char)(Rand0[num] ^ KK[17][num]);
		/* calc sets 4 8 12 0 */
		num = (m + 3) % 16;
		Rand0[num] = (unsigned char)(Rand0[num] ^ KK[17][num]);
	}
	return ;
}
/*************************************************************
 *   E1_Authentication_KeyGen
 *
 *   Descriptions:
 *      Generate the authentication/secret key(SRes).
 *
 *   Arguments:
 *      pKey:     IN,  pointer to the Key[16], namely 128Bit.
 *      pRand:    IN,  pointer to the Rand[16], namely 128Bit.
 *      pAddress: IN,  pointer to the Address[6], namely 48Bit.
 *      pSRes:    OUT, pointer to the authentication/secret key, SRes[4] namely 32Bit.
 *      pACO:     OUT, pointer to the Authenticated Ciphering Offset, ACO[12] namely 96Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
void E1_Authentication_KeyGen(unsigned char *pKey, unsigned char *pRand, unsigned char *pAddress, unsigned char *pSRes, unsigned char *pACO)
{
	unsigned int n;
	unsigned char ee[256], ll[256];
	unsigned char BB[18][16], KK[18][16];
	unsigned char Rand0[16], Rand1[16], Keywave[16];
	Boxes_ee_ll_and_Vectors_BB(ee, ll, BB);
	KeySchedule(pKey, BB, KK);
	Ar_or_Arhat_RandGen(0, pRand, ee, ll, KK, Rand0);
	for (n = 0; n < 16; n++)
	{
		Rand0[n] = (unsigned char)(pRand[n] ^ Rand0[n]);
		Rand0[n] = (unsigned char)(Rand0[n] + pAddress[n % 6]);
	}
	KeyOffset(pKey, Keywave);
	KeySchedule(Keywave, BB, KK);
	Ar_or_Arhat_RandGen(1, Rand0, ee, ll, KK, Rand1);
	for (n = 0; n < 16; n++)
	{
		if (n < 4)
		{
			pSRes[n] = Rand1[n];
		}
		else
		{
			pACO[n - 4] = Rand1[n];
		}
	}
	return ;
}
/*************************************************************
 *   E21_UnitAndCombination_KeyGen
 *
 *   Descriptions:
 *      Generate the Unit and Combination key.
 *
 *   Arguments:
 *      pRand:    IN,  pointer to the Rand[16], namely 128Bit.
 *      pAddress: IN,  pointer to the Address[6], namely 48Bit.
 *      pKey:     OUT, pointer to the Key[16], namely 128Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
void E21_UnitAndCombination_KeyGen(unsigned char *pRand, unsigned char *pAddress, unsigned char *pKey)
{
	unsigned int n;
	unsigned char ee[256], ll[256];
	unsigned char BB[18][16], KK[18][16];
	unsigned char XX[16], YY[16];
	for (n = 0; n < 16; n++)
	{
		XX[n] = pRand[n];
		YY[n] = pAddress[n % 6];
	}
	XX[15] = (unsigned char)(XX[15] ^ 6);
	Boxes_ee_ll_and_Vectors_BB(ee, ll, BB);
	KeySchedule(XX, BB, KK);
	Ar_or_Arhat_RandGen(1, YY, ee, ll, KK, pKey); /* pKey=Ar'(XX,YY) */
	return ;
}
/*************************************************************
 *   E22_InitAndMaster_KeyGen
 *
 *   Descriptions:
 *      Generate the Initializing and Master key.
 *
 *   Arguments:
 *      pRand:      IN,  pointer to the Rand[16], namely 128Bit.
 *      pPIN:       IN,  pointer to the PIN[], max 16Byte.
 *      Length:     IN,  the length of pPIN.
 *      pBDAddress: IN,  pointer to the BDAddress[6], namely 48Bit.
 *      pKey:       OUT, pointer to the Key[16], namely 128Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
NTSTATUS E22_InitAndMaster_KeyGen(unsigned char *pRand, unsigned char *pPIN, unsigned char Length, unsigned char *pBDAddress, unsigned char *pKey)
{
	unsigned char n, length1;
	unsigned char XX[16], YY[16], PIN1[16];
	unsigned char ee[256], ll[256];
	unsigned char BB[18][16], KK[18][16];
	if (Length < 1 || Length > 16)
	{
		return STATUS_UNSUCCESSFUL;
	}
	length1 = (unsigned char)((Length + 6 > 16) ? 16 : (Length + 6));
	for (n = 0; n < Length; n++)
	{
		PIN1[n] = pPIN[n];
	}
	for (n = Length; n < length1; n++)
	{
		PIN1[n] = pBDAddress[n - Length];
	}
	for (n = 0; n < 16; n++)
	{
		XX[n] = PIN1[n % length1];
		YY[n] = pRand[n];
	}
	YY[15] = (unsigned char)(YY[15] ^ length1);
	Boxes_ee_ll_and_Vectors_BB(ee, ll, BB);
	KeySchedule(XX, BB, KK);
	Ar_or_Arhat_RandGen(1, YY, ee, ll, KK, pKey); /* pKey=Ar'(XX,YY) */
	return STATUS_SUCCESS;
}
/*************************************************************
 *   E3_Encryption_KeyGen
 *
 *   Descriptions:
 *      Generate the encryption key.
 *
 *   Arguments:
 *      pKey:    IN,  pointer to the Key[16], namely 128Bit.
 *      pEnRand: IN,  pointer to the EnRand[16], namely 128Bit.
 *      COF:     IN,  pointer to the Ciphering OFfset number, COF[12] namely 96Bit.
 *      Kc:      OUT, pointer to the encryption key, Kc[16] namely 128Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
void E3_Encryption_KeyGen(unsigned char *pKey, unsigned char *pEnRand, unsigned char *COF, unsigned char *Kc)
{
	unsigned int n;
	unsigned char ee[256], ll[256];
	unsigned char BB[18][16], KK[18][16];
	unsigned char Rand0[16], Keywave[16];
	Boxes_ee_ll_and_Vectors_BB(ee, ll, BB);
	KeySchedule(pKey, BB, KK);
	Ar_or_Arhat_RandGen(0, pEnRand, ee, ll, KK, Rand0);
	for (n = 0; n < 16; n++)
	{
		Rand0[n] = (unsigned char)(pEnRand[n] ^ Rand0[n]);
		Rand0[n] = (unsigned char)(Rand0[n] + COF[n % 12]);
	}
	KeyOffset(pKey, Keywave);
	KeySchedule(Keywave, BB, KK);
	Ar_or_Arhat_RandGen(1, Rand0, ee, ll, KK, Kc);
	return ;
}
/*************************************************************
 *   Encryption_ApostropheKeyGen
 *
 *   Descriptions:
 *      Generate the encryption apostrophe key(K'c) based on Kc.
 *
 *   Arguments:
 *      Kc:           IN,  pointer to the Encryption Key[16], namely 128Bit.
 *      KeyLength:    IN,  the effective key length in number of octets.
 *      ApostropheKc: OUT, pointer to the encryption apostrophe key, ApostropheKc[16] namely 128Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
void Encryption_ApostropheKeyGen(unsigned char *Kc, unsigned char KeyLength, unsigned char *ApostropheKc)
{
	int i, temp_g1[4], temp_g2[4], temp_gx[4], KCC[4];
	int KKC[4] =
	{
		0
	};
	int GG1[64] =
	{
		0x00000000, 0x00000000, 0x00000000, 0x0000011d, 0x00000000, 0x00000000, 0x00000000, 0x0001003f, 0x00000000, 0x00000000, 0x00000000, 0x010000db, 0x00000000, 0x00000000, 0x00000001, 0x000000af, 0x00000000, 0x00000000, 0x00000100, 0x00000039, 0x00000000, 0x00000000, 0x00010000, 0x00000291, 0x00000000, 0x00000000, 0x01000000, 0x00000095, 0x00000000, 0x00000001, 0x00000000, 0x0000001b, 0x00000000, 0x00000100, 0x00000000, 0x00000609, 0x00000000, 0x00010000, 0x00000000, 0x00000215, 0x00000000, 0x01000000, 0x00000000, 0x0000013b, 0x00000001, 0x00000000, 0x00000000, 0x000000dd, 0x00000100, 0x00000000, 0x00000000, 0x0000049d, 0x00010000, 0x00000000, 0x00000000, 0x0000014f, 0x01000000, 0x00000000, 0x00000000, 0x000000e7, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	int GG2[64] =
	{
		0x00e275a0, 0xabd218d4, 0xcf928b9b, 0xbf6cb08f, 0x0001e3f6, 0x3d7659b3, 0x7f18c258, 0xcff6efef, 0x000001be, 0xf66c6c3a, 0xb1030a5a, 0x1919808b, 0x00000001, 0x6ab89969, 0xde17467f, 0xd3736ad9, 0x00000000, 0x01630632, 0x91da50ec, 0x55715247, 0x00000000, 0x00002c93, 0x52aa6cc0, 0x54468311, 0x00000000, 0x000000b3, 0xf7fffce2, 0x79f3a073, 0x00000000, 0x00000000, 0xa1ab805b, 0xc7ec8025, 0x00000000, 0x00000000, 0x0002c980, 0x11d8b04d, 0x00000000, 0x00000000, 0x0000058e, 0x24f9a4bb, 0x00000000, 0x00000000, 0x0000000c, 0xa76024d7, 0x00000000, 0x00000000, 0x00000000, 0x1c9c26b9, 0x00000000, 0x00000000, 0x00000000, 0x0026d9e3, 0x00000000, 0x00000000, 0x00000000, 0x00004377, 0x00000000, 0x00000000, 0x00000000, 0x00000089, 0x00000000, 0x00000000, 0x00000000, 0x00000001
	};
	/* get the polynomial g1 and g2 corresponding the KeyLength */
	for (i = 0; i < 4; i++)
	{
		temp_g1[i] = GG1[(KeyLength - 1) *4+i];
		temp_g2[i] = GG2[(KeyLength - 1) *4+i];
	}
	/* convert the Kc from one-bytes to four-bytes */
	for (i = 0; i < 4; i++)
	{
		KCC[3-i] = *(unsigned int*)(Kc + 4 * i);
	}
	/* calculate mod(Kc,g1) */
	for (i = 0; i < ((16-KeyLength) *8-1); i++)
	{
		temp_gx[0] = (temp_g1[1] >> 31) &0x00000001;
		temp_gx[1] = (temp_g1[2] >> 31) &0x00000001;
		temp_gx[2] = (temp_g1[3] >> 31) &0x00000001;
		temp_g1[0] <<= 1;
		temp_g1[1] <<= 1;
		temp_g1[2] <<= 1;
		temp_g1[3] <<= 1;
		temp_g1[0] = (temp_g1[0] &0xFFFFFFFE) | temp_gx[0];
		temp_g1[1] = (temp_g1[1] &0xFFFFFFFE) | temp_gx[1];
		temp_g1[2] = (temp_g1[2] &0xFFFFFFFE) | temp_gx[2];
		temp_g1[3] = (temp_g1[3] &0xFFFFFFFE);
	}
	for (i = 0; i < (128-8 * KeyLength); i++)
	{
		if (KCC[0] &0x80000000)
		{
			KCC[0] = KCC[0] ^ temp_g1[0];
			KCC[1] = KCC[1] ^ temp_g1[1];
			KCC[2] = KCC[2] ^ temp_g1[2];
			KCC[3] = KCC[3] ^ temp_g1[3];
		}
		temp_gx[0] = (KCC[1] >> 31) &0x00000001;
		temp_gx[1] = (KCC[2] >> 31) &0x00000001;
		temp_gx[2] = (KCC[3] >> 31) &0x00000001;
		KCC[0] <<= 1;
		KCC[1] <<= 1;
		KCC[2] <<= 1;
		KCC[3] <<= 1;
		KCC[0] = (KCC[0] &0xFFFFFFFE) | temp_gx[0];
		KCC[1] = (KCC[1] &0xFFFFFFFE) | temp_gx[1];
		KCC[2] = (KCC[2] &0xFFFFFFFE) | temp_gx[2];
		KCC[3] = (KCC[3] &0xFFFFFFFE);
	}
	/* calculate K'c */
	for (i = 0; i < (8 *KeyLength); i++)
	{
		temp_gx[0] = (KKC[1] >> 31) &0x00000001;
		temp_gx[1] = (KKC[2] >> 31) &0x00000001;
		temp_gx[2] = (KKC[3] >> 31) &0x00000001;
		KKC[0] <<= 1;
		KKC[1] <<= 1;
		KKC[2] <<= 1;
		KKC[3] <<= 1;
		KKC[0] = (KKC[0] &0xFFFFFFFE) | temp_gx[0];
		KKC[1] = (KKC[1] &0xFFFFFFFE) | temp_gx[1];
		KKC[2] = (KKC[2] &0xFFFFFFFE) | temp_gx[2];
		KKC[3] = (KKC[3] &0xFFFFFFFE);
		if (KCC[0] &0x80000000)
		{
			KKC[0] = KKC[0] ^ temp_g2[0];
			KKC[1] = KKC[1] ^ temp_g2[1];
			KKC[2] = KKC[2] ^ temp_g2[2];
			KKC[3] = KKC[3] ^ temp_g2[3];
		}
		temp_gx[0] = (KCC[1] >> 31) &0x00000001;
		temp_gx[1] = (KCC[2] >> 31) &0x00000001;
		temp_gx[2] = (KCC[3] >> 31) &0x00000001;
		KCC[0] <<= 1;
		KCC[1] <<= 1;
		KCC[2] <<= 1;
		KCC[3] <<= 1;
		KCC[0] = (KCC[0] &0xFFFFFFFE) | temp_gx[0];
		KCC[1] = (KCC[1] &0xFFFFFFFE) | temp_gx[1];
		KCC[2] = (KCC[2] &0xFFFFFFFE) | temp_gx[2];
		KCC[3] = (KCC[3] &0xFFFFFFFE);
	}
	for (i = 0; i < 4; i++)
	{
		*(unsigned int*)(ApostropheKc + 4 * i) = KKC[3-i];
	}
	return ;
}
/*************************************************************
 *   AccessCode_Gen
 *
 *   Descriptions:
 *      Generate the accesscode.
 *
 *   Arguments:
 *      pLAP:        IN,  pointer to LowerAddressPart of the BD_Address consisting of 24 bits.
 *      pAccessCode: OUT, pointer to the AccessCode[9], namely 72Bit.
 *
 *   Return Value:
 *      None
 *************************************************************/
void AccessCode_Gen(unsigned char *pLAP, unsigned char *pAccessCode)
{
	unsigned char i, j;
	unsigned char AppendLAP[30];
	unsigned char Codeword[64] =
	{
		0
	};
	unsigned char Preamble[4] =
	{
		0, 1, 0, 1
	};
	unsigned char Append[6] =
	{
		0, 0, 1, 1, 0, 1
	};
	unsigned char BlockCode[35] =
	{
		1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1
	};
	unsigned char PNCode[64] =
	{
		0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1
	};
	/* Format the 30 information bits to encode */
	for (i = 0; i < 24; i++)
	{
		AppendLAP[i] = (unsigned char)((pLAP[i / 8] >> (i % 8)) &0x01);
	}
	for (i = 24; i < 30; i++)
	{
		AppendLAP[i] = (unsigned char)((AppendLAP[23] + Append[i - 24]) % 2);
	}
	/* Add the information covering part of the PN overlay sequence.
	p=3F2A33DD69B121C1, where the left-most bit is p0=0. */
	for (i = 0; i < 30; i++)
	{
		Codeword[34+i] = (unsigned char)((AppendLAP[i] + PNCode[34+i]) % 2);
	}
	/* Generate parity bits of the (64,30) expurgated block code. Its generation matrix is
	G(D)=260534236651(octal notation). the left-most bit corresponds to the high-oder(g34) coefficient. */
	for (i = 0; i < 30; i++)
	{
		if (Codeword[63-i] == 0)
		{
			continue;
		}
		/* c(D)=D34x(D) mod g(D), namely Codeword[0-33] */
		for (j = 1; j < 35; j++)
		{
			Codeword[63-i - j] = (unsigned char)((Codeword[63-i - j] + BlockCode[j]) % 2);
		}
	}
	/* get c0-c33 */
	for (i = 0; i < 34; i++)
	{
		Codeword[i] = (unsigned char)((Codeword[i] + PNCode[i]) % 2);
	}
	RtlZeroMemory(pAccessCode, 9);
	for (i = 0; i < 4; i++)
	{
		pAccessCode[0] = (unsigned char)(pAccessCode[0] + (((Codeword[0] + Preamble[i]) % 2) << (i % 8)));
	}
	for (i = 4; i < 38; i++)
	{
		pAccessCode[i / 8] = (unsigned char)(pAccessCode[i / 8] + (Codeword[i - 4] << (i % 8)));
	}
	for (i = 38; i < 68; i++)
	{
		pAccessCode[i / 8] = (unsigned char)(pAccessCode[i / 8] + (AppendLAP[i - 38] << (i % 8)));
	}
	for (i = 68; i < 72; i++)
	{
		pAccessCode[8] = (unsigned char)(pAccessCode[8] + (((AppendLAP[23] + Preamble[i - 68]) % 2) << (i % 8)));
	}
	return ;
}
/*************************************************************
 *   Rand_UCHAR_Gen
 *
 *   Descriptions:
 *      Generate one random number [0, 256), utilizing linear congruential arithmetic.
 *
 *   Arguments:
 *      None
 *
 *   Return Value:
 *      The random number.
 *************************************************************/
unsigned char Rand_UCHAR_Gen(void)
{
	//LARGE_INTEGER CurrentTime;
	static UINT32 seed = 1;
	
	//KeQuerySystemTime(&CurrentTime);
	seed += (UINT32)(jiffies);
	
	
	/* linear congruential */
	seed = (29 * seed + 73) % 1021;
	return (UINT8)(seed % 256);
}
 /*--end of file--------------------------------------------------------*/
