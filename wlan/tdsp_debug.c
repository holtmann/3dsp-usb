/*************************************************************************
 *
 *	(c) 2004-05, 3DSP Corporation, all rights reserved.  Duplication or
 *	reproduction of any part of this software (source code, object code or
 *	comments) without the expressed written consent by 3DSP Corporation is
 *	forbidden.  For further information please contact:
 *
 *	3DSP Corporation
 *	16271 Laguna Canyon Rd
 *	Irvine, CA 92618
 *	www.3dsp.com 
 *
 *************************************************************************
 *
 * [tdsp_main.c] description:
 * 	This source file defines general interface of VKI sub-module.
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


#include "tdsp_basetypes.h"
#include "tdsp_debug.h"




UINT32	g_wlan_dbg_module = 0xFFFFFFFF;
UINT8	g_wlan_dbg_level = LEVEL_TRACE;





#ifdef DEBUG_OPEN__WLAN


char*	level_chars[] = {
	"A",		//assert
	"E",		//error
	"C",		//critical
	"T",		//trace
	"I"};		//info




void		tdsp_print_buffer(PUINT8 buffer,UINT32 length)
{
	UINT32 i;
	
	sc_print("tdsp_print_buffer: Address = %p, len = %d\n",buffer, length);

	
	//sc_print("     :  0  1  2  3  4  5  6  7     8  9  A  B  C  D  E  F\n");
	sc_print("----------------------------------------------------------------------------------------------------------------\n");

	for(i=0;length>=32;length-=32,i+=32)
	{
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26],buffer[i+27],buffer[i+28],buffer[i+29],buffer[i+30],buffer[i+31]);
	}

	switch(length)
	{
	case 1:
		sc_print("%04x : %02X\n",
			i,buffer[i]);		
		break;
	case 2:
		sc_print("%04x : %02X %02X\n",
			i,buffer[i],buffer[i+1]);		
		break;
	case 3:
		sc_print("%04x : %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2]);		
		break;
	case 4:
		sc_print("%04x : %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3]);		
		break;
	case 5:
		sc_print("%04x : %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4]);		
		break;
	case 6:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5]);		
		break;
	case 7:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6]);		
		break;
	case 8:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7]);		
		break;
	case 9:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8]);		
		break;
	case 10:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9]);		
		break;
	case 11:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10]);		
		break;
	case 12:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11]);		
		break;
	case 13:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12]);		
		break;
	case 14:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13]);		
		break;
	case 15:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14]);		
		break;	
	case 16:
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15]);		
		break;	
	case 17:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16]);	
		break;
	case 18:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17]);	
		break;
	case 19:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18]);	
		break;
	case 20:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19]);	
		break;
	case 21:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20]);	
		break;	
	case 22:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21]);	
		break;	
	case 23:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22]);	
		break;	
	case 24:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23]);	
		break;	
	case 25:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24]);	
		break;	
	case 26:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25]);	
		break;	
	case 27:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26]);	
		break;	
	case 28:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26],buffer[i+27]);	
		break;	
	case 29:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26],buffer[i+27],buffer[i+28]);	
		break;	
	case 30:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26],buffer[i+27],buffer[i+28],buffer[i+29]);	
		break;			
	case 31:		
		sc_print("%04x : %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X\n",
			i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7],
			buffer[i+8],buffer[i+9],buffer[i+10],buffer[i+11],buffer[i+12],buffer[i+13],buffer[i+14],buffer[i+15],
			buffer[i+16],buffer[i+17],buffer[i+18],buffer[i+19],buffer[i+20],buffer[i+21],buffer[i+22],buffer[i+23],
			buffer[i+24],buffer[i+25],buffer[i+26],buffer[i+27],buffer[i+28],buffer[i+29],buffer[i+30]);	
		break;	
		
	default:
		break;
	}
	sc_print("----------------------------------------------------------------------------------------------------------------\n");
}


void tdsp_dbgmsg_print_buff(INT8* comments, volatile UINT8* buff,UINT32  len,char *file_name,UINT32 line_number)  
{		
	sc_print("3D[B:%s:%05d]%s\n",file_name,line_number,comments);
	tdsp_print_buffer((PUINT8)buff,len);
}



#endif //#ifdef DEBUG_OPEN__WLAN


void tdsp_set_debug(void)
{
#ifdef DEBUG_OPEN__WLAN
	// init sub-moudule of VKI
	g_wlan_dbg_level = LEVEL_TRACE;

	g_wlan_dbg_module = ~(0x00); 
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__ISR)); 
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__RX));
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__MLME));
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__SME));
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__TX));
	//g_wlan_dbg_module &= (UINT32)(~(MOD_WLAN__MAC_CORE));

	return;
#endif //#ifdef DEBUG_OPEN__WLAN
}


