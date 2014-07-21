#include "bt_sw.h"
#include "bt_dbg.h"
#include "bt_hci.h"
#include "afhclassify.h"
#include "bt_usb_vendorcom.h"


/**************************************************************************
 *   Afg_Init
 *
 *   Descriptions:
 *      Initialize afh module, including alloc memory for the test module and 
 *		so on.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value:
 *      STATUS_SUCCESS.   Test module memory is allocated with succesfully
 *      STATUS_UNSUCCESSFUL. Test module   memory is allocated with fail 
 *************************************************************************/
NTSTATUS
Afh_Init(PBT_DEVICE_EXT devExt)
{
	PBT_AFH_T pAfh;
	int i,j;
	
	BT_DBGEXT(ZONE_AFH | LEVEL3, "Afh_Init devExt = 0x%p\n",devExt);

	/* Alloc memory for test module */
	pAfh = (PBT_AFH_T)ExAllocatePool(sizeof(BT_AFH_T), GFP_KERNEL);
	if (pAfh == NULL)
	{
		BT_DBGEXT(ZONE_AFH | LEVEL0, "Allocate afh memory failed!\n");
		return STATUS_UNSUCCESSFUL;
	}

	/* Save task module pointer into device extention context */
	devExt->pAfh = (PVOID)pAfh;

	BT_DBGEXT(ZONE_AFH | LEVEL3, "Afh_Init afh = 0x%p, size = %d\n",pAfh, sizeof(BT_AFH_T));

	/* Zero out the test module space */
    RtlZeroMemory(pAfh, sizeof(BT_AFH_T));

	/* Alloc spin lock,which used to protect test rx operator */
	KeInitializeSpinLock(&pAfh->lock);

	for(j=0;j<NSLAVE;j++)
	{
		for(i=0;i<10;i++)
		{
			pAfh->AFH_classification_S[j][i]=0x55;
		}
	}

	//initiate the master's stastics 
	for(i=0;i<20;i++)
	{
		
		if(pAfh->BPLR_Table[i]<=0)
		{
			pAfh->BPLR_Table[i] = ATTENUATE_INDEX;
		}
		
		//pAfh->BPLR_Table[i] = 0;
	}
	
	return (STATUS_SUCCESS);
}

/**************************************************************************
 *   Afh_Release
 *
 *   Descriptions:
 *      Release test module, including free memory for the test module.
 *   Arguments:
 *      devExt: IN, pointer to device extension of device to start.
 *   Return Value: 
 *      STATUS_SUCCESS.   Task module memory is released with succesfully
 *      STATUS_UNSUCCESSFUL. Task module  memory is released with fail 
 *************************************************************************/
NTSTATUS Afh_Release(PBT_DEVICE_EXT devExt)
{
	PBT_AFH_T pAfh;
		
	BT_DBGEXT(ZONE_AFH | LEVEL3, "Afh_Release\n");

	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return STATUS_UNSUCCESSFUL;
	
	BT_DBGEXT(ZONE_AFH | LEVEL3, "Afh_Release devExt = 0x%p, pAfh = 0x%p\n",devExt,pAfh);

	/*Free the test module memory */
	if (pAfh != NULL)
		ExFreePool(pAfh);

	devExt->pAfh = NULL;

	return (STATUS_SUCCESS);
}

VOID Afh_Statistic(PBT_DEVICE_EXT devExt, unsigned char badfreq)
{
	PBT_AFH_T pAfh;
	KIRQL oldIrql;
	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return;

	//jakio20080229: safe check
	if(badfreq >= 80)
	{
		BT_DBGEXT(ZONE_AFH | LEVEL0, "Afh_Statistic---Exceeds arry limits:%d\n", badfreq);
		return;
	}

	// Take out the Write list lock, since we'll insert this IRP
    // onto the write queue
    //
    KeAcquireSpinLock(&pAfh->lock, &oldIrql);
	BlockPLRstatistic(badfreq,pAfh->BPLR_Table2);
	KeReleaseSpinLock(&pAfh->lock, oldIrql);
}

UINT8 Afh_GetMap(PBT_DEVICE_EXT devExt, PUINT8 destmap)
{
	PBT_AFH_T pAfh;
	KIRQL oldIrql;
	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return 0;

	// Take out the Write list lock, since we'll insert this IRP
    // onto the write queue
    //
    KeAcquireSpinLock(&pAfh->lock, &oldIrql);
	AFHClassify(pAfh->AFH_classification, pAfh->BPLR_Table, pAfh->Prev_AFH_classification, &pAfh->classification_change_map,pAfh->BPLR_Table2);
	AFHMAPGen(pAfh->AHF_MAP,pAfh->AFH_classification,&(pAfh->AFH_classification_S[0][0]),pAfh->Prev_AHF_MAP, &pAfh->change_map);
	if (pAfh->change_map)
		RtlCopyMemory(destmap, (PUINT8)pAfh->AHF_MAP, 10);
	KeReleaseSpinLock(&pAfh->lock, oldIrql);

	return ((UINT8)pAfh->change_map);
}

UINT8 Afh_GetClassification(PBT_DEVICE_EXT devExt, PUINT8 destmap)
{
	PBT_AFH_T pAfh;
	KIRQL oldIrql;
	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return 0;

	// Take out the Write list lock, since we'll insert this IRP
    // onto the write queue
    //
    KeAcquireSpinLock(&pAfh->lock, &oldIrql);
	AFHClassify(pAfh->AFH_classification, pAfh->BPLR_Table,pAfh->Prev_AFH_classification, &pAfh->classification_change_map,pAfh->BPLR_Table2);
	if (pAfh->classification_change_map)
		RtlCopyMemory(destmap, (PUINT8)pAfh->AFH_classification, 10);
	KeReleaseSpinLock(&pAfh->lock, oldIrql);

	return ((UINT8)pAfh->classification_change_map);
}

PUINT8 Afh_GetSlaveClaAddress(PBT_DEVICE_EXT devExt, PCONNECT_DEVICE_T pConnectDevice)
{
	PBT_AFH_T pAfh;

	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return NULL;

	return (&(pAfh->AFH_classification_S[0][0]));
}

VOID Afh_ClearPrevAfhMap(PBT_DEVICE_EXT devExt)
{
	PBT_AFH_T pAfh;

	/*Get pointer to the test */
	pAfh = (PBT_AFH_T)devExt->pAfh;
	if (pAfh == NULL)
		return;

	RtlZeroMemory(pAfh->Prev_AHF_MAP, 3);
}

/*void BlockPLRstatistic(unsigned char badfreq,int* pBPLR_Table,int * pAFH_MAP)
{
	int badfreq32div;
	int newbadfreq;
	int badfreq32mod;
    newbadfreq=badfreq;
	badfreq = badfreq>>2; 
	if(badfreq<=0)
	{
		pBPLR_Table[0] = pBPLR_Table[0] + 1;
	}
	else if(badfreq>=19)
	{
		pBPLR_Table[19] = pBPLR_Table[19] + 1;
	}
	else
 {
  
  badfreq32div = (newbadfreq-4)>>5;
  badfreq32mod = (newbadfreq-4)-(badfreq32div<<5);
  pBPLR_Table[badfreq] = pBPLR_Table[badfreq] + 1;
  if((pAFH_MAP[badfreq32div] & (int)(~(0xf<<badfreq32mod)))!= 0)
  {
   pBPLR_Table[badfreq-1] = pBPLR_Table[badfreq-1] + 1;
  }
 }
}*/

void BlockPLRstatistic(unsigned char badfreq,int* pBPLR_Table2)
{ 
		pBPLR_Table2[badfreq] = pBPLR_Table2[badfreq] + 1;
}

/*
classify the frequencies status as the format the spec specified
The threshhold need be revised to variable
*/
void AFHClassify(unsigned char* pAFH_classification,int* pBPLR_Table,unsigned char* Prev_pAFH_classification,int *classification_change_map,int* pBPLR_Table2)
{
	int i;

	for(i=0;i<20;i++)
	{
      		pBPLR_Table[i]=pBPLR_Table2[4*i]+pBPLR_Table2[4*i+1]+pBPLR_Table2[4*i+2]+pBPLR_Table2[4*i+3];
	}

 	/*
 	for(i=0;i<20;i++)
	{
 		BT_DBGEXT(ZONE_AFH | LEVEL0, "pBPLR_Table = %d\n",pBPLR_Table[i]);
	}
 	BT_DBGEXT(ZONE_AFH | LEVEL3, "**********************************\n");  
 	*/
 	
	for(i=0;i<10;i++)
	{
		pAFH_classification[i] = 0;
	}
	pAFH_MIN_NUM_USED(pAFH_classification,pBPLR_Table,pBPLR_Table2);

	for(i=0;i<10;i++)
	{ 
		if(Prev_pAFH_classification[i]!=pAFH_classification[i]) 
		{	
			*classification_change_map=1;
			i=12;
		}
		else  
		{
			*classification_change_map=0;	
		}
	}
	for(i=0;i<10;i++)
	{ 
		Prev_pAFH_classification[i]=pAFH_classification[i];
	}

}
/*
generate AFH-MAP from the master and slaves' classification results.
*/
void AFHMAPGen(int* pAFH_MAP,unsigned char* pAFH_classification,unsigned char* pAFH_classification_S,int* Prev_AHF_MAP,int* change_map)
{
	int i,j;
	int i2div,i32div,i8mod;
	int i8div;
	int i32mod;
	int num_unused;
	int Qi;
	unsigned char bitresult;

	num_unused  = 0;
	pAFH_MAP[0] = 0xffffffff;
	pAFH_MAP[1] = 0xffffffff;
	pAFH_MAP[2] = 0xffffffff;
	
    /*******************************************/
   
		for(i=0;i<79;i+=2)
		{	
			i2div = i>>1; 
			i32div= i>>5;
			i8div = i>>3;
			i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);
			i32mod= (i2div<<1) - (((i2div<<1)>>5)<<5);
			Qi = 0;

			bitresult = ((pAFH_classification[i8div] & (0x3 << i8mod))>>i8mod) & 0x03;
			Qi = (bitresult==3?0:1);
            if(Qi<QTHRESHOLD)
			{
				j=NSLAVE;
			}
			else 
			{
				j=0;
			}

	        for(;j<NSLAVE;j++) 
			{ 
			bitresult = ((pAFH_classification_S[i8div] & (0x3 << i8mod))>>i8mod) & 0x03;
			//BT_DBGEXT(ZONE_AFH | LEVEL3, "map = %d\n",(int)pAFH_classification_S[i8div]);
				Qi = (bitresult==3?0:1);
				if(Qi<QTHRESHOLD)
				{
					j=NSLAVE;
				}

			}	
 					
			//Qi = (bitresult==3?0:1);
	        //////////////////////////////////////////////////////////////////
			if(Qi<QTHRESHOLD)
			{
				if(num_unused<(79-NMIN))
				{
					pAFH_MAP[i32div] = pAFH_MAP[i32div] & (int)(~(0x3<<i32mod));
					num_unused = num_unused +2;
					
				}
			}
			
		}
  	 //pAFH_MAP[0] = 0x00000fff;
   	 //pAFH_MAP[1] = 0x00000000;
     //pAFH_MAP[2] = 0xfffffff0;

       for(i=0;i<3;i++)
	   { 
			if(Prev_AHF_MAP[i]!=pAFH_MAP[i]) 
			{	
				*change_map=1;
				i=4;
			}
			else  *change_map=0;
	   }
	   for(i=0;i<3;i++)
	   { 
			Prev_AHF_MAP[i]=pAFH_MAP[i];
	   }
	
}


void pAFH_MIN_NUM_USED(unsigned char* pAFH_classification,int* pBPLR_Table,int* pBPLR_Table2)
{
	int i,j;
	int i2div,i8mod;
	int i8div;
	int num_unused;
	int Qi;


	int k;
	int a[79];
	int b[20];
	unsigned char bitresult;
	int temp1;
	int temp2;
	int bigIndex;   

	num_unused  = 10;
	
    /*******************************************/
    for(j=0;j<20;j++)
	{ 
		b[j]=j;
	}

    for(i=0;i<19;i++) 
	{
        bigIndex=i; 
	    for(j=i+1;j<20;j++) 
		{
			if(pBPLR_Table[j]>pBPLR_Table[bigIndex]) 
			bigIndex=j; 
		}
		if(bigIndex!=i) 
		{ 
			temp1=pBPLR_Table[i]; 
			temp2=b[i];
			pBPLR_Table[i]=pBPLR_Table[bigIndex]; 
			b[i]=b[bigIndex]; 
			pBPLR_Table[bigIndex]=temp1; 
			b[bigIndex]=temp2; 
		}
	}

   /**************************************************/
	    for(j=0;j<20;j++)
		{ 
			a[j*4]=b[j]*4;
            a[j*4+2]=b[j]*4+2;
		}

			//////////////////////////////////////////
	    if(pBPLR_Table[0]<10)  k=100;
		   else  k=0;
		temp1=k-num_unused;
		while (temp1<0)
		{	
			i=a[k];
			i2div = i>>1; 
			i8div = i>>3;
			i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);

				pAFH_classification[i8div] = pAFH_classification[i8div] & (unsigned char)(~(0xf<<i8mod)) | (unsigned char)(0xf<<i8mod);
 
				i=a[k]-4;
				if (i>=0)
				{
				i2div = i>>1; 
				i8div = i>>3;
				i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);

				pAFH_classification[i8div] = pAFH_classification[i8div] & (unsigned char)(~(0xf<<i8mod)) | (unsigned char)(0xf<<i8mod);
				}

				i=a[k]+4;
				if (i<80)
				{
				i2div = i>>1; 
				i8div = i>>3;
				i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);

				pAFH_classification[i8div] = pAFH_classification[i8div] & (unsigned char)(~(0xf<<i8mod)) | (unsigned char)(0xf<<i8mod);
				}
				
				i=a[k]+8;
					if (i<80)
					{
					i2div = i>>1; 
					i8div = i>>3;
					i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);

					pAFH_classification[i8div] = pAFH_classification[i8div] & (unsigned char)(~(0xf<<i8mod)) | (unsigned char)(0xf<<i8mod);
					}

				if ((a[k+2]<a[k]-4)&&(a[k+2]<a[k]+8))
		              num_unused=num_unused+2;
		    		   
			i=k>>2;
		    if(pBPLR_Table[i]<8)  k=100;
			   else	   k=k+2;
		    temp1=k-num_unused;
		} 
			/////////////////////////////////////////
 			BT_DBGEXT(ZONE_AFH | LEVEL3, "num_unused = %d\n",num_unused);

		num_unused  = 0;
		for(k=0;k<79;k+=2)
		{	
			i=a[k];
			i2div = i>>1; 
			i8div = i>>3;
			i8mod = (i2div<<1) - (((i2div<<1)>>3)<<3);
			Qi = 0;

			 
			bitresult = ((pAFH_classification[i8div] & (0x3 << i8mod))>>i8mod) & 0x03;
			Qi = (bitresult==3?0:1);
			if(Qi<QTHRESHOLD)
			{
				if(num_unused<(79-NMIN))		num_unused = num_unused +2;
				      else
                    pAFH_classification[i8div] = pAFH_classification[i8div] & (unsigned char)(~(0xf<<i8mod)) | (unsigned char)(0x5<<i8mod);
				
				if (pBPLR_Table2[i]>50)  pBPLR_Table2[i]=50;
					  //if (pBPLR_Table2[i]>800)   pBPLR_Table2[i]=pBPLR_Table2[i]-100;
					    else  if (pBPLR_Table2[i]>3) pBPLR_Table2[i]=pBPLR_Table2[i]-1;

				
				if (pBPLR_Table2[i+1]>50)  pBPLR_Table2[i+1]=50;
                       //if (pBPLR_Table2[i+1]>800)    pBPLR_Table2[i+1]=pBPLR_Table2[i+1]-100;
					       else  if (pBPLR_Table2[i+1]>3)  pBPLR_Table2[i+1]=pBPLR_Table2[i+1]-1;

			}
			else
            {
				pBPLR_Table2[i]=0;
                pBPLR_Table2[i+1]=0;
			}

		}

	
}
