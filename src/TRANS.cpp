//SYSTEM INCLUDES
#include <string>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
//#include <svc.h>

//ADK INCLUDES
#include "html/gui.h"
#include "msr/msr.h"
#include <log/liblog.h>
#include "sysinfo/sysinfo.h"

#include "sysinfo/syspm.h"
#include "sysinfo/sysbar.h"
#include "sysinfo/sysbeep.h"

//SDK INCLUDES

#include <vault/Pinentry.h>



//USER INCLUDES
#include "EmvRdc/VosEMVProcess.h"
#include "Trans/TRANS.h"
#include "PinPad/PP_Cmd.h"
#include "CapX/usrcmds.h"
#include "CapX/dkprsa.h"
#include "CapX/debug.h"
#include "App/encryp_aes.h" // FAG 08-NOV-2016
#include "App/FlowPinPad.h"
#include "Convertion/Convertion.h"
#include "ComRdc/ComRdc.h" // FAG 08-NOV-2017

extern MSR_DecodedData Datas;
extern int inHex2Asc(char pchHex[], char pchAsc[], int inHex);
extern int in_Cx_EDta(char pchData[] ,int inData,char PchDOut[], int *inDOut);
char *pchGetEnvVar(char pchVar[], char pchVal[], int inMaxChr, char pchFil[]);

//using namespace std;
using namespace std;
using namespace vfigui;
using namespace vfisysinfo;
// Prototypes
bool getSerialLast7(char *sn7);
int getserialnumber(char * Serial_Number);
void PrintMsrDecodedData(MSR_DecodedData msrDecodedData)
{
    int status = 0;
	// Track1
	if (msrDecodedData.t1.valid)
	{
		LOGF_TRACE("Track 1 is valid ");
		LOGF_TRACE("PAN %s Name %s", msrDecodedData.t1.pan, msrDecodedData.t1.name);
		LOGF_TRACE("Exp Data %s Service Code %s Discretionary Data %s", msrDecodedData.t1.exp_date,
				msrDecodedData.t1.service_code, msrDecodedData.t1.disc_data);
	}
	else
	{
		LOGF_TRACE("Track 1 is invalid ");
		if (Tracks.t1.status == 0)
		{
			LOGF_TRACE("But is Present!!");
			LOGAPI_HEXDUMP_TRACE("TRACK1 DUMP", Tracks.t1.data, strlen(Tracks.t1.data));
			  LOGF_TRACE("PAN %s Name %s", msrDecodedData.t1.pan, msrDecodedData.t1.name);
        	LOGF_TRACE("Exp Data %s Service Code %s Discretionary Data %s", msrDecodedData.t1.exp_date, msrDecodedData.t1.service_code, msrDecodedData.t1.disc_data);

			//Monderos: Samborns, Sears, Palacio de Hierro % XXX --> %BXXX
			if (Tracks.t1.data[0] == 0x25 && Tracks.t1.data[1] == 0x20)
			{
				Tracks.t1.data[1] = 0x42;
				LOGAPI_HEXDUMP_TRACE("TRACK1 FIXED DUMP", Tracks.t1.data, strlen(Tracks.t1.data));
			}
		}
		else
		{
			LOGF_TRACE("CARD TYPE [%d]",Tracks.card_type);

			if(Tracks.card_type == MSR_TYPE_SAMSUNG)
			{
				LOGF_TRACE("******SAMSUNG PAY MST*********");
			}
			else
			{
				LOGF_TRACE("******MSR FISICA*********");
				//status = INVALID_MSR_DATA;	
			}
			
		}
	}

	// Track2
	if (msrDecodedData.t2.valid)
	{
		LOGF_TRACE("Track 2 is valid ");
		LOGF_TRACE("PAN %s ", msrDecodedData.t2.pan);
		LOGF_TRACE("Exp Data %s Service Code %s Discretionary Data %s", msrDecodedData.t2.exp_date,
				msrDecodedData.t2.service_code, msrDecodedData.t2.disc_data);
	}
	else
	{
		LOGF_TRACE("Track 2 is invalid ");
		status = INVALID_MSR_DATA;
	}

}

int MSR_Transaction(int MSRead)
{
	char dataMSR[256] = {0};
	int resultMSR = 0;
	//TO store the card data
	char trk1[100] = {0};
	int sztrk1 = 0;
	char trk2[50] = {0};
	int sztrk2 = 0;
	char trk3[100] = {0};
	int sztrk3 = 0;	
	char CrdHold[35] = {0};
	int szCrdHold = 0;
	char PAN[20] = {0};
	int szPAN = 0;
	char ExpDate[5] = {0};
	int szExpDate = 0;
	char ServCode[5] = {0};
	int szServCode = 0;
	char DiscData[30] = {0};
	int szDiscData = 0;
	char pchAsc[128] = {0};
	char pchCipher[128]={0};
	char posEntry[5]={0};
	int inencrypt=0;
	int in_val=0;
	int inposEntry=0;
	
	//Variables necesarias para armar el campo C53
	char PanC53[40] = {0};	
	int szPanC53 = 0;
	char CardHoldC53[30] = {0};
	int szCardHoldC53 = 0;

	char LengthC53[3] = {0};
	int inLengthC53 = 0;
	
	//Para evaluar el nible alto del byte del tipo de transacci�n
    	char nibleTypeTxn = 0;
	char Txn_typ = 0;
	int szTxn_typ = 0;
		
	//FLAGS
	//char flagCVV = 0;
	char flagCVV = CVV_NOTASKED;
	char flag_encrypt=0;

	int status = 0;
	int statusTrack1 = 0;
	
	
	char CVV[6] = {0};
	int szCVV2 = 4;
	 
	 char dataENCRYPT[32];
	 int szdataENCRYPT = 0;
	 char tmpbuf[32];

	 char ch_es [100] = {0}; //TOKEN ES
	 char ch_ez [150] = {0}; //TOKEN EZ
	 char ch_ey [200] = {0}; //TOKEN EY
	 int  in_es   = 0;	 //SIZE OF TOKEN ES
	 int  in_ez   = 0;	 //SIZE OF TOKEN EZ
	 int  in_ey   = 0;	 //SIZE OF TOKEN EY

	char LenHex[3]={0};
	char lenfld = 0;	


	LOGF_TRACE("\n----------MSR technology detected!!!");	
	
	if( (MSRead == MSR_OK) || (MSRead == MSR_ACTIVE) )
		MSRead = MSR_OK;
	else
		{
		LOGF_TRACE("MSR_ERROR!!");				
	}

	MSRead = Read_MSRdata();
	
	PrintMsrDecodedData(Datas);	
	
	LOGF_TRACE("\n MSRread = %i",MSRead);
	
	if(MSRead == MSR_OK )
	{
		statusTrack1 = getMSR_data((char *)"Track1",trk1,&sztrk1);
		LOGF_TRACE("\nTrack1: %s",trk1);
		if(statusTrack1<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"Track2",trk2,&sztrk2);
		LOGF_TRACE("\nTrack2: %s",trk2);
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"Track3",trk3,&sztrk3);
		LOGF_TRACE("\nTrack3: %s",trk3);		
		/*if(status<SUCCESS)
			return status; */
		
		status = getMSR_data((char *)"Cardholder",CrdHold,&szCrdHold);
		LOGF_TRACE("\ncardholder: %s",CrdHold);		
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"Pan",PAN,&szPAN);
		LOGF_TRACE("\nPan: %s",PAN);		
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"ExpDate",ExpDate,&szExpDate);
		LOGF_TRACE("\nExpiration Date: %s",ExpDate);		
		if(status<SUCCESS)
			return status;
		
		status =  getMSR_data((char *)"ServCode",ServCode,&szServCode);
		LOGF_TRACE("\nService Code: %s",ServCode);			
		if(status<SUCCESS)
			return status;
		
		status =  getMSR_data((char *)"DiscData",DiscData,&szDiscData);
		LOGF_TRACE("\nDiscretionary data: %s",DiscData);		

		
	}
	else if(MSRead == MSR_ERROR )
	{
		LOGF_TRACE("MSR_ERROR!!");
		//uiInvokeURL(1,"ERROR_BANDA.html");
		//uiInvokeURL(0,value,"ERROR_BANDA.html");
		//sleep(2);
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
		
	}
	else if(MSRead == MSR_TIMEOUT )
	{
		LOGF_TRACE("MSR_TIMEOUT!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
	}
	else if(MSRead == MSR_ABORTED)
	{
		LOGF_TRACE("MSR_ABORTED!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
	}
	
	if(MSRead == MSR_OK)
	{
		if(check_service_code(ServCode)==false)
		{
			LOGF_TRACE("*****Use chip*****");
			return USE_CHIP;
		}
		
		//**********************************
		//Aqu� armamos el campo del Pan para el campo C53 para una transancci�n de banda magn�tica
		if ( bo_crp_crd (PAN,szPAN))
		{
			LOGF_TRACE("*****FLAG 01*****");
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x01",3);	
			flag_encrypt=1;
		}
		
		else
		{
			LOGF_TRACE("*****FLAG 00*****");
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x00",3);	
		
		}
			
		inLengthC53 = inLengthC53 + 3;

		/* PAN */		
		memcpy(PanC53,"\xC1",1);
		PanC53[1] = szPAN;
		memcpy(&PanC53[2],PAN,szPAN);		
		vdSetByteValuePP((char*)"PAN",PanC53,szPAN + 2);
		inLengthC53 = inLengthC53 + szPAN + 2;

		/* EXP DATE */
		memset(pchAsc,0,sizeof(pchAsc));
		memcpy(pchAsc,"\xC1",1);
		pchAsc[1] = szExpDate;
		memcpy(&pchAsc[2],ExpDate,szExpDate);
		vdSetByteValuePP((char*)"EXP_DATE",pchAsc,szExpDate+2);
			
			
		inLengthC53 = inLengthC53 + szExpDate + 2;
		//if((szPAN % 2) == 0)	
		//szPanC53 = szPAN/2; 
		//else
		//	szPanC53 = (szPAN/2) + 1;
			
		//PanC53[1] = szPanC53;
		//vdAsc2Hex (PAN,&PanC53[2],szPanC53);
		//vdSetByteValuePP((char*)"PAN",PanC53,szPanC53 + 2);
		/* CARD HOLDER */
		memcpy(CardHoldC53,"\xC1",1);
		CardHoldC53[1] = szCrdHold;
		memcpy(&CardHoldC53[2],CrdHold,szCrdHold);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHoldC53,szCrdHold+2);

		inLengthC53 = inLengthC53 + szCrdHold + 2;
			
		//**********************************
		//Evaluamos el nible alto del byte Txn type que llega en el C51 para saber si pedir CCV
			
		/*szTxn_typ = inGetByteValuePP((char *)"TRANSTYP",&Txn_typ);

		nibleTypeTxn = Txn_typ & 0xF0;
		nibleTypeTxn = nibleTypeTxn >> 4;

		if(nibleTypeTxn == 0x01) 
			flagCVV	= 0;
		else
			flagCVV = 1; */

		
			
		//if(flagCVV>0)
		if (statusTrack1 == VALID_MSR_DATA){
			in_val=screenSec_Code(CVV,szCVV2,"INGRESE CODIGO DE SEGURIDAD","");
			if(in_val< UI_ERR_OK)
				return FAILURE_CVV;
			flagCVV = CVV_CAPTURED;
		}
		else{
			
			memset(CVV,0x20,3);
		}
		
		if(flag_encrypt==1)
		{
			LOGF_TRACE("encriptar el CVV-AES");
			memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
			in_Cx_EDta(CVV,strlen(CVV),dataENCRYPT,&szdataENCRYPT); 
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = szdataENCRYPT;
			memcpy(tmpbuf + 2,dataENCRYPT,szdataENCRYPT + 2);
			vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,szdataENCRYPT + 2);
			inLengthC53 = inLengthC53 + inencrypt + 2;				

		}
		else
		{
			memset(tmpbuf,0x00,sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(CVV);
			memcpy(tmpbuf + 2,CVV,strlen(CVV));
			vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,strlen(CVV) + 2);
			inLengthC53 = inLengthC53 + strlen(CVV) + 2;				

		}

		
			
		if (sztrk2>0)
		{

			if(flag_encrypt==1)
			{
				LOGF_TRACE("***  CIPHER TRACKII AES  ***");
				in_val=in_Cx_EDta(trk2,sztrk2,pchCipher,&(inencrypt)); //	
							
				/* TRACK2 AES */
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = inencrypt;
				memcpy(&pchAsc[2], pchCipher, inencrypt);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,inencrypt+2);
				inLengthC53 = inLengthC53 + inencrypt + 2;
			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk2;
				memcpy(&pchAsc[2], trk2,sztrk2);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,sztrk2+2);
				inLengthC53 = inLengthC53+sztrk2+ 2;		

			}
			
		}

		if (sztrk1>0)
		{
			if(flag_encrypt==1)
			{
				memset(pchCipher,0,sizeof(pchCipher));
				inencrypt = inGetByteValuePP((char *)"TRACK1",pchCipher);
				LOGAPI_HEXDUMP_TRACE( "track 1 antes de llenar",pchCipher, sizeof(pchCipher));
				LOGF_TRACE("***  CIPHER TRACKI AES	***");
				memset(pchCipher,0,sizeof(pchCipher));
				in_val=in_Cx_EDta(trk1,sztrk1,pchCipher,&(inencrypt)); //	
				LOGAPI_HEXDUMP_TRACE( "antes de llenar",pchCipher, sizeof(pchCipher));
				/* TRACK1 AES */
				memset(pchAsc,0,sizeof(pchAsc));			
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = inencrypt;
				memcpy(&pchAsc[2], pchCipher, inencrypt);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,inencrypt+2);
				inLengthC53 = inLengthC53 + inencrypt + 2;

			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk1;
				memcpy(&pchAsc[2], trk1,sztrk1);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,sztrk1+2);
				inLengthC53 = inLengthC53+sztrk1+ 2;		

			}
		
			
		}
		
		inposEntry = 4;
		//if(count_fallback>2)
		if(count_fallback_get() > 1)
		{
			LOGF_TRACE("banda por fallback");
			memcpy(posEntry,"\xC1\x02\x38\x30",inposEntry);
			//count_fallback=0;	
			count_fallback_zero();
		}
		else
		{
			LOGF_TRACE("banda");
			memcpy(posEntry,"\xC1\x02\x39\x30",inposEntry);
		}
		vdSetByteValuePP((char*)"POSENTRY",posEntry,inposEntry);

		inLengthC53 = inLengthC53 + inposEntry;
			
		//sizeCmd += szposEntry;
			
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			
	//	inLengthC53 = inLengthC53 + 2;

		vdSetByteValuePP((char*)"TOKE1",(char *)"\xE1\x01\x00",3);
		inLengthC53 = inLengthC53 + 3;
		vdSetByteValuePP((char*)"TOKE2",(char *)"\xE2\x01\x00",3);
		inLengthC53 = inLengthC53 + 3;
	}

	if( boDUKPTasm )
	{
		MSRead = in_txn_crd (trk1,sztrk1,trk2,sztrk2,CVV,szCVV2,MSR,flagCVV);
		in_es = inGetByteValuePP((char*)"TOKES",ch_es);
		in_ez = inGetByteValuePP((char*)"TOKEZ",ch_ez);
		in_ey = inGetByteValuePP((char*)"TOKEY",ch_ey);

		inLengthC53=inLengthC53+in_es+in_ez+in_ey;
	}
	else
		MSRead = SUCCESS_TRX;

	//build_C53_C54Offline();
	memset(LenHex,0x00,sizeof(LenHex));
	lenfld = 0;
	lenfld |= inLengthC53;	//length
	LenHex[1] = lenfld;
	lenfld = 0;
	lenfld |= inLengthC53 >> 8;
	LenHex[0] = lenfld;
		
	//LengthC53[1] = inLengthC53 & 0x000000FF;
	//LengthC53[0] = inLengthC53 & 0x0000FF00;
	
	vdSetByteValuePP((char*)"LENGH",LenHex,2);
	LOGF_TRACE("MSReaf [%i]",MSRead);
	return MSRead;
}


int processMSRTransaction(int MSRead,int itWasRead)
{
	char dataMSR[256] = {0};
	int resultMSR = 0;
	//TO store the card data
	char trk1[100] = {0};
	int sztrk1 = 0;
	char trk2[50] = {0};
	int sztrk2 = 0;
	char trk3[100] = {0};
	int sztrk3 = 0;	
	char CrdHold[35] = {0};
	int szCrdHold = 0;
	char PAN[20] = {0};
	int szPAN = 0;
	char ExpDate[5] = {0};
	int szExpDate = 0;
	char ServCode[5] = {0};
	int szServCode = 0;
	char DiscData[30] = {0};
	int szDiscData = 0;
	char pchAsc[128] = {0};
	char pchCipher[128]={0};
	char posEntry[5]={0};
	int inencrypt=0;
	int in_val=0;
	int inposEntry=0;
	
	//Variables necesarias para armar el campo C53
	char PanC53[40] = {0};	
	int szPanC53 = 0;
	char CardHoldC53[30] = {0};
	int szCardHoldC53 = 0;

	char LengthC53[3] = {0};
	int inLengthC53 = 0;
	
	//Para evaluar el nible alto del byte del tipo de transacci�n
    	char nibleTypeTxn = 0;
	char Txn_typ = 0;
	int szTxn_typ = 0;
		
	//FLAGS
	//char flagCVV = 0;
	char flagCVV = CVV_NOTASKED;
	char flag_encrypt=0;

	int status = 0;
	
	
	char CVV[6] = {0};
	int szCVV2 = 4;
	 
	 char dataENCRYPT[32];
	 int szdataENCRYPT = 0;
	 char tmpbuf[32];

	 char ch_es [100] = {0}; //TOKEN ES
	 char ch_ez [150] = {0}; //TOKEN EZ
	 char ch_ey [200] = {0}; //TOKEN EY
	 int  in_es   = 0;	 //SIZE OF TOKEN ES
	 int  in_ez   = 0;	 //SIZE OF TOKEN EZ
	 int  in_ey   = 0;	 //SIZE OF TOKEN EY

	char LenHex[3]={0};
	char lenfld = 0;
	int statusTrack1 = 0;	


	LOGF_TRACE("\n----------processMSRTransaction----");	
	
	if((MSRead == MSR_OK) || (MSRead == MSR_ACTIVE))
		MSRead = MSR_OK;
	else
		LOGF_TRACE("MSR_ERROR!!");				
	

	if(!itWasRead)
		MSRead = Read_MSRdata();

	
	PrintMsrDecodedData(Datas);	
	
	LOGF_TRACE("\n MSRread = %i",MSRead);
	
	if(MSRead == MSR_OK )
	{
		statusTrack1 = getMSR_data((char *)"Track1",trk1,&sztrk1);


		LOGF_TRACE("Track1: %s",trk1);
		//MAR 2019-02_14 Gil pidio que se recortara los centinelas y el lrc
		char ch_aux[80];
		memset(ch_aux, 0x00, sizeof(ch_aux));
		memcpy(ch_aux, trk1+1, sztrk1 - 3 );
		memset(trk1, 0x00, sizeof(trk1));
		memcpy(trk1, ch_aux, sztrk1 - 3 );
		sztrk1 -= 3;
		LOGF_TRACE("Track1: %s",trk1); //jbf 20250106
		LOGF_TRACE("\nStatus {%d}",statusTrack1);
		if(statusTrack1<SUCCESS)
			return statusTrack1;
		
		status = getMSR_data((char *)"Track2",trk2,&sztrk2);
		LOGF_TRACE("\nTrack2: %s",trk2);

		//MAR 2019-02_14 Gil pidio que se recortara los centinelas y el lrc
		memset(ch_aux, 0x00, sizeof(ch_aux));
		memcpy(ch_aux, trk2+1, sztrk2 - 3 );
		memset(trk2, 0x00, sizeof(trk2));

		memcpy(trk2, ch_aux, sztrk2 - 3 );
		sztrk2 -= 3;

		LOGF_TRACE("\nTrack2_2: %s",trk2);
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"Track3",trk3,&sztrk3);
		LOGF_TRACE("\nTrack3: %s",trk3);		
		/*if(status<SUCCESS)
			return status; */
		
		status = getMSR_data((char *)"Cardholder",CrdHold,&szCrdHold);
		LOGF_TRACE("\ncardholder: %s",CrdHold);		
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"Pan",PAN,&szPAN);
		LOGF_TRACE("\nPan: %s",PAN);		
		if(status<SUCCESS)
			return status;
		
		status = getMSR_data((char *)"ExpDate",ExpDate,&szExpDate);
		LOGF_TRACE("\nExpiration Date: %s",ExpDate);		
		if(status<SUCCESS)
			return status;
		
		status =  getMSR_data((char *)"ServCode",ServCode,&szServCode);
		LOGF_TRACE("\nService Code: %s",ServCode);			
		if(status<SUCCESS)
			return status;
		
		status =  getMSR_data((char *)"DiscData",DiscData,&szDiscData);
		LOGF_TRACE("\nDiscretionary data: %s",DiscData);		

		/*		
		if(status<SUCCESS)
			return status; */
		
		
	}
	else if(MSRead == MSR_ERROR )
	{
		LOGF_TRACE("MSR_ERROR!!");
		//uiInvokeURL(1,"ERROR_BANDA.html");
		//uiInvokeURL(0,value,"ERROR_BANDA.html");
		//sleep(2);
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
		
	}
	else if(MSRead == MSR_TIMEOUT )
	{
		LOGF_TRACE("MSR_TIMEOUT!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
	}
	else if(MSRead == MSR_ABORTED)
	{
		LOGF_TRACE("MSR_ABORTED!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
	}
	
	if(MSRead == MSR_OK)
	{
		if(check_service_code(ServCode)==false)
		{
			LOGF_TRACE("*****Use chip*****");
			return USE_CHIP;
		}
		
		//**********************************
		//Aqu� armamos el campo del Pan para el campo C53 para una transancci�n de banda magn�tica
		if ( isExceptionBin(PAN,szPAN) || isMerchantExceptionBIN(PAN,szPAN) || isMerchantExceptionBINRange(PAN,szPAN) )
		{
			flag_encrypt=0;
		}
		
		else
		{
			LOGF_TRACE("*****FLAG 01*****");
			flag_encrypt=1;
		}
		

		/* PAN */
		LOGF_TRACE("*****PAN = %s*****",PAN);		
		memcpy(PanC53,"\xC1",1);
		/*if(szPAN%2!=0){
			PAN[szPAN++]='F';
		}*/
		//PanC53[1] = szPAN/2;
		PanC53[1] = szPAN;
		//AscHex((unsigned char *)&PanC53[2],PAN,szPAN/2);
		memcpy(PanC53+2,PAN,szPAN);		
		//vdSetByteValuePP((char*)"PAN",PanC53,szPAN/2 + 2);
		vdSetByteValuePP((char*)"PAN",PanC53,szPAN + 2);
		LOGF_TRACE("*****PAN CMD = %s [%d]*****",PanC53,szPAN+2);
		//inLengthC53 = inLengthC53 + szPAN/2 + 2;
		inLengthC53 = inLengthC53 + szPAN + 2;
	
		
	/* CARD HOLDER */
		memcpy(CardHoldC53,"\xC1",1);
		CardHoldC53[1] = szCrdHold;
		memcpy(&CardHoldC53[2],CrdHold,szCrdHold);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHoldC53,szCrdHold+2);

		inLengthC53 = inLengthC53 + szCrdHold + 2;
			
		//**********************************
		//Evaluamos el nible alto del byte Txn type que llega en el C51 para saber si pedir CCV
			
		/*szTxn_typ = inGetByteValuePP((char *)"TRANSTYP",&Txn_typ);

		nibleTypeTxn = Txn_typ & 0xF0;
		nibleTypeTxn = nibleTypeTxn >> 4;

		if(nibleTypeTxn == 0x01) 
			flagCVV	= 0;
		else
			flagCVV = 1; */

		
		if (statusTrack1 == VALID_MSR_DATA){
			in_val=screenSec_Code(CVV,szCVV2,"INGRESE CODIGO DE SEGURIDAD","");
			if(in_val< UI_ERR_OK)
				return FAILURE_CVV;
			flagCVV = CVV_CAPTURED;
		}
		else{
			
			memset(CVV,0x20,3);
		}

			
		if(flag_encrypt==1)
		{
			LOGF_TRACE("CVV will be encrypted and this field will be null.");
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1\x00",2);
			vdSetByteValuePP((char*)"SEC_CODE",tmpbuf, 2);
			inLengthC53+= 2;				

		}
		else
		{
			memset(tmpbuf,0x00,sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(CVV);
			memcpy(tmpbuf + 2,CVV,strlen(CVV));
			vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,strlen(CVV) + 2);
			inLengthC53 = inLengthC53 + strlen(CVV) + 2;				

		}

		

		
		
			
		if (sztrk2>0)
		{

			if(flag_encrypt==1)
			{
				LOGF_TRACE("*** Info is Encrypted, thus, this field is nill ***");
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1\x00",2);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,2);
				inLengthC53 += 2;
			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk2;
				memcpy(&pchAsc[2], trk2,sztrk2);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,sztrk2+2);
				inLengthC53 = inLengthC53+sztrk2+ 2;		

			}
			
		}

		if (sztrk1>0)
		{
			if(flag_encrypt==1)
			{
				LOGF_TRACE("*** Info is Encrypted, thus, this field is nill ***");
				memset(pchAsc,0,sizeof(pchAsc));			
				memcpy(pchAsc,"\xC1\x00",2);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,2);
				inLengthC53 += 2;

			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk1;
				memcpy(&pchAsc[2], trk1,sztrk1);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,sztrk1+2);
				inLengthC53 = inLengthC53+sztrk1+ 2;		

			}
		
			
		}
		
		inposEntry = 4;
		//if(count_fallback>2)
		if(count_fallback_get() > 1)
		{
			LOGF_TRACE("banda por fallback");
			memcpy(posEntry,"\xC1\x02\x38\x30",inposEntry);
			//count_fallback=0;	
			//count_fallback_zero();
		}
		else
		{
			LOGF_TRACE("banda");
			memcpy(posEntry,"\xC1\x02\x39\x30",inposEntry);
		}
		vdSetByteValuePP((char*)"POSENTRY",posEntry,inposEntry);

		inLengthC53 = inLengthC53 + inposEntry;
			
		//sizeCmd += szposEntry;
			
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			
	//	inLengthC53 = inLengthC53 + 2;

		vdSetByteValuePP((char*)"TOKE1",(char *)"\xE1\x00",2);
		inLengthC53 = inLengthC53 + 2;
		vdSetByteValuePP((char*)"TOKE2",(char *)"\xE2\x00",2);
		inLengthC53 = inLengthC53 + 2;
	}

	if( boDUKPTasm && flag_encrypt==1)
	{
		MSRead = in_txn_crd (trk1,sztrk1,trk2,sztrk2,CVV, szCVV2, MSR, flagCVV);

		in_es = inGetByteValuePP((char*)"TOKES",ch_es);
		in_ez = inGetByteValuePP((char*)"TOKEZ",ch_ez);
		in_ey = inGetByteValuePP((char*)"TOKEY",ch_ey);

		inLengthC53=inLengthC53+in_es+in_ez+in_ey;
	}
	else{
		MSRead = SUCCESS_TRX;
		char ch_es[100];
		int in_es;
		in_es=inAsmQs(ch_es,FALSE);
		vdSetByteValuePP((char*)"TOKES",ch_es,in_es);
		inLengthC53=inLengthC53+in_es;
	}
		

	//build_C53_C54Offline();
	memset(LenHex,0x00,sizeof(LenHex));
	lenfld = 0;
	lenfld |= inLengthC53;	//length
	LenHex[1] = lenfld;
	lenfld = 0;
	lenfld |= inLengthC53 >> 8;
	LenHex[0] = lenfld;
		
	//LengthC53[1] = inLengthC53 & 0x000000FF;
	//LengthC53[0] = inLengthC53 & 0x0000FF00;
	
	vdSetByteValuePP((char*)"LENGH",LenHex,2);
	LOGF_TRACE("MSReaf [%i]",MSRead);

	if(count_fallback_get() > 1)
	{
		LOGF_TRACE("Fallback reset");
		count_fallback_zero();
	}

	return MSRead;
}


int KBD_Transaction(char *PAN, int szPan)
{
	int KBD_Result = 0;	
	char Exp_Date[6] = {0};
	int szExpDate = 4;
	char CVV[6] = {0};	
	int szCVV = 4;
	char PanC53[30];
	int szPanC53 = 0;
	//Para evaluar el nible alto del byte del tipo de transacci�n
	char nibleTypeTxn = 0;
	char Txn_typ = 0;
	int szTxn_typ = 0;
	char flagCVV = 0;
	char flagExpDate = 0;
	
	char flag2CVV = CVV_NOTASKED;

	LOGF_TRACE("\n--KBD_flow--");

	//**********************************
	//Aqu� armamos el campo del Pan para el campo C53 para una transancci�n digitada

	memcpy(PanC53,"\xC1",1);

	if((szPan % 2) == 0)	
		szPanC53 = szPan/2;	
	else
		szPanC53 = (szPan/2) + 1;

	PanC53[1] = szPanC53;
	vdAsc2Hex (PAN,&PanC53[2],szPanC53);
	vdSetByteValuePP((char*)"PAN",PanC53,szPanC53 + 2);

	//**********************************

	//******************************************
	//Evaluamos el nible alto del byte Txn type que llega en el C51 para saber si pedir CCV y Exp_date
	
	szTxn_typ = inGetByteValuePP((char *)"TRANSTYP",&Txn_typ);
	
	nibleTypeTxn = Txn_typ & 0xF0;
	nibleTypeTxn = nibleTypeTxn >> 4;
	
	LOGF_TRACE("nibleTypeTxn = %i",nibleTypeTxn);
	
	if( nibleTypeTxn == 0x03  )
	{
		flagCVV = 0;
		flagExpDate = 0;
	}
	else if(nibleTypeTxn == 0x02)
	{
		flagCVV = 1;
		flagExpDate = 0;
	}
	else if(nibleTypeTxn == 0x01 )
	{
		flagCVV = 0;
		flagExpDate = 1;
	}
	else	//Banorte does not check both exp and cvv
	{
		flagCVV = 1;
		flagExpDate = 1;
		
		flagCVV = 0;
		flagExpDate = 0;
	}

	if(flagExpDate)
	{	
		KBD_Result = screenExpDate(Exp_Date,"FECHA DE EXPIRACION","MM/YY");	
		//if(KBD_Result < UI_ERR_OK)
		if(KBD_Result != 0)
		{
			//if(KBD_Result == UI_ERR_TIMEOUT)
			if(KBD_Result == 4)
			{
				LOGF_TRACE("\n--UI_ERR_TIMEOUT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
			}
			//if(KBD_Result == UI_ERR_ABORT)
            //if (KBD_Result == 1 )
            else
			{
				LOGF_TRACE("\n--UI_ERR_ABORT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
			}
		}
		else
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
	}

	if(flagCVV)
	{

		KBD_Result = screenSec_Code(CVV,szCVV,"INGRESE CODIGO DE SEGURIDAD","");	
		if(KBD_Result < UI_ERR_OK)
		{
			if(KBD_Result == UI_ERR_ABORT)
			{
				LOGF_TRACE("\n--UI_ERR_ABORT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
			}
			if(KBD_Result == UI_ERR_TIMEOUT)
			{
				LOGF_TRACE("\n--UI_ERR_TIMEOUT--");	
				vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
			}
		}
		else {
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			flag2CVV = CVV_CAPTURED;
		}
	}

		LOGF_TRACE("Antes de encriptar");
		KBD_Result = in_txn_crd (PAN,szPan,Exp_Date,szExpDate,CVV, szCVV,KBD, flag2CVV);	
		
		build_C53_C54Offline();
			
	return KBD_Result;
	
}


int KBD_Transaction_banorte(char *PAN, int szPan, int type_cmd)
{
	char tmpbuf[32];
	int mov  = 0 ;
	
	char LenHex[3]={0};
	char lenfld = 0;
	char aux = 0;
	int KBD_Result = 0; 
	char Exp_Date[6] = {0};
	int szExpDate = 4;
	char CVV[6] = {0};	
	int szCVV = 4;
	char Txn_typ	  = 0;
	int szTxn_typ	  = 0;
	char flagCVV	  = 0;
	char flagExpDate  = 0;
	char flagPAN	  = 0;
	int r = -1;
	int inMedio = -1;
	map<string,string> inputIdl;
	int id = 0;
	char intStr[10] = {0};
	unsigned char ucOptions[6] = {0, 0, 0, 0, 0, CTS_PURE_CARD_DETECTION};
	int prev_timeout=0;
	int region = 0;
	int inMax = 16 ;
	int inMin =  3 ;
	int inTimeOut = 30;
	
	char flag2CVV = CVV_NOTASKED;
	
	char chManual[20] = {0};
	char chHeader[20] = {0};
	char chMsg	 [50] = {0};
	
	char dataENCRYPT[32];
	int szdataENCRYPT = 0;
	
	int inRes = 0;
	char chRcvbuff[32];

	int cont =0;
	char flag_encrypt=0;


	LOGF_TRACE("\n--KBD_flow uni--");
	memset(intStr,0,sizeof(intStr));
	sprintf(intStr,"%d",inMax);
	inputIdl["maxLen"] = string(intStr);
	uiInvokeCancel(id);
	szTxn_typ = inGetByteValuePP((char *)"PMOVILCORRESP",&Txn_typ);
	LOGF_TRACE("--PMOVILCORRESP [%x]--",Txn_typ);
	flagPAN 	= Txn_typ & KBD_ACT;
	flagExpDate = Txn_typ & KBD_EXP;
	flagCVV 	= Txn_typ & KBD_CV2;
	LOGF_TRACE("--flagPAN	  [%x]--",flagPAN);
	LOGF_TRACE("--flagExpDate [%x]--",flagExpDate);
	LOGF_TRACE("--flagCVV	  [%x]--",flagCVV);
      	string temp=""; 

	
	inputIdl["HEADER_TITLE"]="";
	inputIdl["BODY_HEADER"]="INGRESO MANUAL"; // FAG 03-ABRIL-2017
	inputIdl["BODY_MSG"]=""; // FAG 03-ABRIL-2017
	inputIdl["maxLen"] = "19";

	while(1)
	{	   

	
		//uiSetPropertyInt(UI_PROP_TIMEOUT, inTimeOut*1000);
		uiSetPropertyInt(UI_PROP_TIMEOUT, 2500);
		//cts_StopSelection();
		memset(intStr,0,sizeof(intStr));
		sprintf(intStr,"%d",inTimeOut);
		inputIdl["timeOut"] =string(intStr);
//		inputIdl["HEADER_TITLE"]="TARJETA DIGITADA";
//		inputIdl["BODY_HEADER"]="INGRESE NUMERO DE TARJETA";
//		inputIdl["BODY_MSG"]="16 Digitos";
//		inputIdl["maxLen"] = "16";

		while (1)
		{
			//LOGF_TRACE("inicio del while antes de invocar la pantalla");
			//LOGF_TRACE("temp = %s",temp.c_str());
			inputIdl["manual"]=temp;
			
			r = uiInvokeURL(inputIdl,"manual.html");
		
			if(r == -3)
			{
					cont+=1;
					LOGF_TRACE("manual = %s",inputIdl["manual"].c_str());
					temp=inputIdl["manual"];
		
					LOGF_TRACE("RECIBIMOS DATOS POR  EL PUERTO");
					memset(chRcvbuff,0,sizeof(chRcvbuff));
					inRes = inRcvAdk(chRcvbuff,sizeof(chRcvbuff),500);
						
					//ANALIZAMOS PAQUETE
					LOGF_TRACE("inRes [%d] \n",inRes);
					//LOGF_TRACE("datos del ingreso manual despues de polear el buffer -> %s",inputIdl["manual"].c_str());
					if(!memcmp(chRcvbuff, "\x02\x37\x32\x03\x06",5))
					{
						cts_StopSelection();
						LOGF_TRACE("****CANCELADA por comando*****\n");
						return EMV_IDL_CANCEL; //CANCEL
					}
					if(cont == 15)
					{
					//	LOGF_TRACE("contador = 15");
					//	LOGF_TRACE("datos del ingreso manual -> %s",inputIdl["manual"].c_str());
						break;
					}

			}

			else if( r==13 || r==-1)
			{
				LOGF_TRACE("aceptar o cancelar");
				//LOGF_TRACE("datos del ingreso  -> %s",inputIdl["manual"].c_str());
				//LOGF_TRACE("datos del temporal  -> %s",temp.c_str());
				break;
			}
			//LOGF_TRACE("fin del while invocar pantalla y poleo del puerto");
			//LOGF_TRACE("datos del ingreso manual final pantalla y pole-> %s",inputIdl["manual"].c_str());
			//LOGF_TRACE("datos del temporal final pantalla y pole -> %s",temp.c_str());
		}
		
		uiSetPropertyInt(UI_PROP_TIMEOUT,prev_timeout);
		if (r == 13) //ENTER
		{		
			if(inputIdl["manual"].length() < inMin)
			{
				//inputIdl["error"]="Longitud Invalida";
				LOGF_TRACE("PAN MANUAL [%s]\n",inputIdl["manual"].c_str());
				temp="";
				continue;
			}
	
			strcat(chManual,inputIdl["manual"].c_str());
			LOGF_TRACE("PAN MANUAL [%s]\n",chManual);
			cts_StopSelection();
			//return EMV_IDL_MANUAL;
			break;
		}
		else if(r == -1)
		{
			cts_StopSelection();
			LOGF_TRACE("****CANCELADA*****\n");
			return EMV_IDL_CANCEL; //CANCELADA
		}
	
		else if(r == -3)
		{
			LOGF_TRACE("****TIME OUT*****\n");
			cts_StopSelection();
			return EMV_IDL_TIME_OUT; //TIME OUT

		}
		//LOGF_TRACE("fin del while principal");
		//LOGF_TRACE("datos del ingreso manual -> %s",inputIdl["manual"].c_str());
		//LOGF_TRACE("datos de temp -> %s",temp.c_str());
	}

	if(checkLuhn(inputIdl["manual"])==false)
	{
		LOGF_TRACE("*** INVALID PAN ***");
		return FAILURE_LUHN;

	}
		
	if(flagExpDate > 0)
	{	
		KBD_Result = screenExpDate(Exp_Date,"FECHA DE EXPIRACION","MM/YY");
		LOGF_TRACE("--KBD_Result  [%i]--",KBD_Result);
		if(KBD_Result < UI_ERR_OK)
		{
			if(KBD_Result == UI_ERR_ABORT)
			{
				LOGF_TRACE("\n--UI_ERR_ABORT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
				return KBD_Result;
			}
			if(KBD_Result == UI_ERR_TIMEOUT)
			{
				LOGF_TRACE("\n--UI_ERR_TIMEOUT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
				return KBD_Result;
			}
		}

		else
			vdSetByteValuePP((char*)"STATUS",(char*)"20",2);
	}
	
	if(flagCVV > 0 )
	{	
		KBD_Result = screenSec_Code(CVV,szCVV,"INGRESE CODIGO DE SEGURIDAD",""); 
		if(KBD_Result < UI_ERR_OK)
		{
			if(KBD_Result == UI_ERR_ABORT)
			{
				LOGF_TRACE("\n--UI_ERR_ABORT--");
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
				return KBD_Result; 
			}
			if(KBD_Result == UI_ERR_TIMEOUT)
			{
				LOGF_TRACE("\n--UI_ERR_TIMEOUT--"); 
				vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
				return KBD_Result;
			}
			if(KBD_Result == FAILURE_CVV)
			{
				LOGF_TRACE("\n--CVV VACIO--"); 
				//vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
				return KBD_Result;
			}
		}

		else
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"20",2);
			flag2CVV = CVV_CAPTURED;			
		}
			
	}
	
	if(KBD_Result == UI_ERR_OK)
	{
		if(type_cmd == KBD_C30)
		{
			mov = 0;
			if ( bo_crp_crd (chManual,strlen(chManual)))
			{
				LOGF_TRACE("*****FLAG 01*****");
				vdSetByteValuePP((char*)"FLAG_ENCRYPT",(char *)"\xC1\x01\x01",3);	
				flag_encrypt=1;
			}
			else
			{
				LOGF_TRACE("*****FLAG 00*****");
				vdSetByteValuePP((char*)"FLAG_ENCRYPT",(char *)"\xC1\x01\x00",3);	
			}

			mov += 3;

			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(chManual);
			memcpy(tmpbuf + 2,chManual,strlen(chManual));
			vdSetByteValuePP((char*)"PAN",tmpbuf,strlen(chManual) + 2);
			mov += strlen(chManual) + 2;
			
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(Exp_Date);
			memcpy(tmpbuf + 2,Exp_Date + 2, 2);
			memcpy(tmpbuf + 4,Exp_Date, 2);
			//memcpy(tmpbuf + 2,Exp_Date,strlen(Exp_Date));
			vdSetByteValuePP((char*)"EXP_DATE",tmpbuf,strlen(Exp_Date) + 2);
			mov += strlen(Exp_Date) + 2;

			if(flag_encrypt==1)
			{
				LOGF_TRACE("encriptar el CVV-AES");
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
				in_Cx_EDta(CVV,strlen(CVV),dataENCRYPT,&szdataENCRYPT);	
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2,dataENCRYPT,szdataENCRYPT + 2);
				vdSetByteValuePP((char*)"CVV2",tmpbuf,szdataENCRYPT + 2);
				mov += szdataENCRYPT + 2;
			}

			else
			{
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = strlen(CVV);
				memcpy(tmpbuf + 2,CVV,strlen(CVV)+2);
				vdSetByteValuePP((char*)"CVV2",tmpbuf,strlen(CVV) + 2);
				mov += strlen(CVV) + 2;
			}

			memset(LenHex,0x00,sizeof(LenHex));
			lenfld = 0;
			lenfld |= mov;	//length
			LenHex[1] = lenfld;
			lenfld = 0;
			lenfld |= mov >> 8;
			LenHex[0] = lenfld;
			LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",LenHex,sizeof(LenHex));			
			vdSetByteValuePP((char*)"LENGH",LenHex,2);
			
		}

		else if(type_cmd == KBD_C51)
		{
			
			if ( bo_crp_crd (chManual,strlen(chManual)))
			{
				LOGF_TRACE("*****FLAG 01*****");
				vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x01",3);	
				flag_encrypt=1;
			}
			
			else
			{
				LOGF_TRACE("*****FLAG 00*****");
				vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x00",3);	
			}

			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(chManual);
			memcpy(tmpbuf + 2,chManual,strlen(chManual));
			vdSetByteValuePP((char*)"PAN",tmpbuf,strlen(chManual) + 2);
					
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,"\xC1",1);
			tmpbuf[1] = strlen(Exp_Date);
			memcpy(tmpbuf + 2,Exp_Date + 2, 2);
			memcpy(tmpbuf + 4,Exp_Date, 2);
			//memcpy(tmpbuf + 2,Exp_Date,strlen(Exp_Date));
			vdSetByteValuePP((char*)"EXP_DATE",tmpbuf,strlen(Exp_Date) + 2);

			if(flag_encrypt==1)
			{
				LOGF_TRACE("encriptar el CVV-AES");
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
				in_Cx_EDta(CVV,strlen(CVV),dataENCRYPT,&szdataENCRYPT);	
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2,dataENCRYPT,szdataENCRYPT + 2);
				vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,szdataENCRYPT + 2);
			
			}

			else
			{
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = strlen(CVV);
				memcpy(tmpbuf + 2,CVV,strlen(CVV)+2);
				vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,strlen(CVV) + 2);
				
			}
			
			//memset(tmpbuf, 0x00, sizeof(tmpbuf));
		//	memcpy(tmpbuf,"\xC1",1);
		//	tmpbuf[1] = szdataENCRYPT;
		//	memcpy(tmpbuf + 2,dataENCRYPT,szdataENCRYPT + 2);
		//	vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,szdataENCRYPT + 2);
	
			LOGF_TRACE("Antes de encriptar con CAP X");
			//KBD_Result = in_txn_crd (PAN,szPan,Exp_Date,szExpDate,CVV, szCVV,KBD);
			if( boDUKPTasm )
				KBD_Result = in_txn_crd (chManual,strlen(chManual),Exp_Date,szExpDate,CVV, szCVV,KBD,flag2CVV);
			else
				KBD_Result = SUCCESS_TRX;
			build_C53_C54Offline();

		}
		
	}
			
	return KBD_Result;
	
}


int EMV_Transaction()
{	
	int i = 0;
	int emvRes = 0;
	int res1stGenAC = 0;
	unsigned char ucReader=0;
	
	//Valores que llegan del comado C51
	unsigned char Amount[14] = {0};
	int szAmount = 0;	

	unsigned int AmountAux = 0;
	unsigned char AmountBCD[14] = {0};
	int szAmountBCD = 0;

	
	unsigned char cashback[14] = {0};
	int szCashBack = 0;	
	unsigned int CashBackAux = 0;
	
	unsigned char txntype = 0;
	int sztxntype = 0;
	unsigned char Emv_txntype = 0;
	unsigned char currCode[2] ={0};
	int szcurrCode = 0;	
	unsigned char merch_destion = 0;
	int szmerch_destion = 0;

	int Remove_card = 0;

	//TYPE TXN
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];
	map<string,string> value;

	LOGF_TRACE("\nEMV Transaction");

	LOGF_TRACE("\n\n");	

	vdSetPinMsgs("PIN","INGRESE PIN:");

	
	szAmount = inGetByteValuePP((char*)"AMOUNT",(char*)Amount);
	AmountAux = 0;
	for(i = 0; i< szAmount; i++)
	{
		AmountAux |= Amount[i];		
		if(i < (szAmount - 1))		
			AmountAux <<= 8;					
	}
	
	LOGF_TRACE("Amount = %i",AmountAux);	
	szCashBack = inGetByteValuePP((char*)"CASHBACK",(char*)cashback);	
	CashBackAux = 0;

	for(i = 0; i< szCashBack; i++)
	{
		CashBackAux |= cashback[i];		
		if(i < (szCashBack - 1))		
			CashBackAux <<= 8;					
	}	
	
	LOGF_TRACE("cashback = %i",CashBackAux);
	AmountAux = AmountAux + CashBackAux;
	LOGF_TRACE("Amount = %i",AmountAux);

	sprintf((char*)AmountBCD,"%4d",(char*)AmountAux);
	
	sztxntype = inGetByteValuePP((char*)"TRANSTYP",(char*)&txntype);
	szcurrCode = inGetByteValuePP((char*)"CURRCODE",(char*)&currCode);
	szmerch_destion = inGetByteValuePP((char*)"MERDESC",(char*)&merch_destion);

	LOGF_TRACE("\nAmount = %s",AmountBCD);
	LOGF_TRACE("\nTransaction type = %X",txntype);

	//if((txntype == 0x01) || (txntype == 0x03) )
	{
		LOGF_TRACE("Venta");
		Emv_txntype = EMV_ADK_TRAN_TYPE_GOODS_SERVICE;  //VENTA 
	}

/*	TODO:
		CHECK REFUND SUPPORT
	if((txntype == 0x02) || (txntype == 0x04))
	{
		LOGF_TRACE("devolucion");
		Emv_txntype = EMV_ADK_TRAN_TYPE_REFUND;
	}
*/

	memset(cashback,0x00,sizeof(cashback));  // se limpia el campo de Cash back, ya que se suma al monto
						
	Set_CT_TransactionData(&TxDataCT, (char *) AmountBCD,(char*)cashback,Emv_txntype);  //Setup CT transaction data
	
	emvRes = EMV_AppSelection(&TxDataCT,&xSelectResCT);

	Remove_card = _CHECK_INSERTED_CARD();
	if( (Remove_card == FALSE))
		return CARD_REMOVED;	

	//Start the application Selection

	switch(emvRes){

		case EMV_ADK_OK:           // application selected, everything OK
			LOGF_TRACE("\n----------Appl selection OK!");
				
			EMV_Setup_TXN_CT_Data(&AdditionalTxDataCT,Amount, cashback, &TxDataCT);   // copy additional TXN Data, known after final select			
			
			emvRes = EMVFLOW(&TxDataCT,&xSelectResCT,&pxTransRes,&AdditionalTxDataCT);
			LOGF_TRACE("\nemvRes = %i",emvRes);	

			if(emvRes == EMV_CT_PIN_INPUT_ABORT || emvRes == EMV_CT_PIN_INPUT_TIMEOUT)
				return emvRes;
			
		break;
		case EMV_ADK_FALLBACK:     // perform fallback to magstripe
			// Add reaction to local fallback rules or other local chip
			LOGF_TRACE("\n----------Appl selection FALLBACK! %#.2x", emvRes);
			//uiInvokeURL(1,"Fallback.html");
			//uiInvokeURL(0,value,"Fallback.html");
			//sleep(2);
			break ;
			//return emvRes;
		break;
		case EMV_ADK_ABORT:
			LOGF_TRACE("\n----------Appl selection ABORT! %#.2x", emvRes);		   // no fallback, TRX definitely finished
			return emvRes;
		break;
		case EMV_ADK_APP_BLOCKED:  // application blocked
			LOGF_TRACE("\n----------Appl Blocked %#.2x", emvRes);		   // App blocked
			return emvRes;
		break;
		case EMV_ADK_NOAPP:        // no application found						
			LOGF_TRACE("\n----------No application found %#.2x", emvRes);		   //No application Found
			return emvRes;
		break;
		default:
			// please feel free to add any other text and reaction if there is any other local chip
			LOGF_TRACE("\n----------Appl selection UNKWN! %#.2x", emvRes);
			//EMV_EndTransactionCT("\nTransaction Error", ucReader, TRUE);					
			return EMV_ADK_ABORT;
		break;
		}


	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9C;
	getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

	if(buffTAG[2] == 0x20)
	{
		LOGF_TRACE("Devoluci�n");
		emvRes = EMV_ADK_ARQC;
	}
	
	

	int shouldEncrypt = 1;

	unsigned short szPAN = 0;
	unsigned long idPAN = 0x5A;
	unsigned char buffPAN[50] = {0};
	memset(buffPAN,0x00,sizeof(buffPAN));
	long unsigned int iResult = getCT_EMV_TAG(0, &idPAN, 1,buffPAN, sizeof(buffPAN), &szPAN,CTS_CHIP);
	LOGF_TRACE("iResult[%d]",iResult);
	
	if((iResult == EMV_ADK_OK) && (szTAG > 0)){
		char panASCII[20];
		int panASCIILength = buffPAN[1];
		char *tem = (char *)buffPAN;
		inHex2Asc(tem + 2,panASCII,panASCIILength);
		LOGAPI_HEXDUMP_TRACE("PAN:",panASCII,sizeof(panASCII));

		if(isMerchantExceptionBIN(panASCII,panASCIILength) || isMerchantExceptionBINRange(panASCII,panASCIILength)){
				shouldEncrypt=0;
		}
	}
	
	LOGF_TRACE("boDUKPTasm[%d] shouldEncrypt[%d]",boDUKPTasm,shouldEncrypt);
	
	//Check the result of First Gen AC	
	
	switch(emvRes){
		case EMV_ADK_ARQC:                // go online
			LOGF_TRACE("\n----------Tx needs to go online!");
			// memset((char*)&xOnlineInputCT,0,sizeof(xOnlineInputCT));

			Info_TRX_CT_EMV();		

			if( boDUKPTasm && shouldEncrypt == 1)
				emvRes = in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP,CVV_EMVTYPE);
			else{
				LOGF_TRACE("\n----------Sin cifrado");
				emvRes = SUCCESS_TRX;
			}
				

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			LOGF_TRACE("Desplegamos PAN y tarjeta en pantalla");
			display_card_info();
			//build_C53_C54Offline();
			break;
		case EMV_ADK_TC:                  // approved offline
			LOGF_TRACE("\n----------Tx approved offline!");
			Info_TRX_CT_EMV();		

			if( boDUKPTasm && shouldEncrypt==1)
				in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP,CVV_EMVTYPE);
			else		
				LOGF_TRACE("\n----------Sin cifrado");

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			//build_C53_C54Offline();			
			
		break;
		case EMV_ADK_FALLBACK:            // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
		   	LOGF_TRACE("\n----------Appl selection FALLBACK!");
			//uiInvokeURL(1,"Fallback.html");
			//uiInvokeURL(0,value,"Fallback.html");
			//sleep(2);
			//EMV_CTLS_SmartPowerOff(ucReader);
			//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
		   	//EMV_EndTransactionCT(FALLBACK_MESSAGE, ucReader, FALSE);
			break;
			//return emvRes;
		case EMV_ADK_AAC:                 // Denied offline
			LOGF_TRACE("\n----------Tx Declined!");
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			
			Info_TRX_CT_EMV();
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
		break;
		case EMV_ADK_AAR:
		case EMV_ADK_ABORT:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_CVM:
		case EMV_ADK_CARDERR_FORMAT:
		default:
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
			//build_C53_C54Offline();	
			//EMV_CTLS_SmartPowerOff(ucReader);
			//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
		return emvRes;
		}	


	return emvRes;

}	


int CTLS_Transaction()
{
	int iResult = 0;	
	int typCTLS = 0;
	
	LOGF_TRACE("\n----------Contactless technology detected!!!");
	iResult = CTLS_FirstGenerateAC(&CTLSTransRes);

	debug("\niResult = %i",iResult);

	typCTLS = Check_typ_CTLS();	
	
	if(typCTLS == CTLS_TXN_CHIP)
	{
		debug("\n***CTLS CHIP****");
		iResult=CTLS_CHIP(iResult);
	}
	else if(typCTLS == CTLS_TXN_MSR)
	{			
		debug("\n***CTLS MSR****");
		iResult=CTLS_MSR(iResult);
	}
	else
	{
		debug("\n***TAP ERROR****");
	}

	dump(&CTLSTransRes,sizeof(CTLSTransRes),(char *)"\nCTLSTransRes:");
	return iResult;
}

int CTLS_CHIP(int erg)
{
	int iResult = 0;
	debug("\n--CTLS_CHIP--");
	switch(erg)
	{
		case EMV_ADK_ARQC:	//Txn needs to go online
			debug("\n*** ARQC**** ");					

			if( boDUKPTasm )
				in_txn_crd (NULL,0,NULL,0,NULL,0,CTS_CTLS, CVV_EMVTYPE);

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);			
			LOGF_TRACE("erg = %X",iResult);

			//build_C53_C54Offline();
			
			
		break;

		case EMV_ADK_TC:                  // approved offline
			debug("\n*** TC**** ");
			debug("\n----------Tx approved offline!");			

			if( boDUKPTasm )
				in_txn_crd (NULL,0,NULL,0,NULL,0,CTS_CTLS,CVV_EMVTYPE);

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);			
			debug("erg = %X",iResult);

			//build_C53_C54Offline();

		break;

		case EMV_ADK_FALLBACK:            // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
			debug("\n----------Appl selection FALLBACK!");
			debug("\n*** FALLBACK**** ");
			vfigui::uiInvoke(1,"<br>    Intente por chip");
		return iResult;
		case EMV_ADK_AAC:                 // Denied offline
			debug("\n----------Tx Declined!");
			debug("\n*** AAC**** ");

			if( boDUKPTasm )
				in_txn_crd (NULL,0,NULL,0,NULL,0,CTS_CTLS,CVV_EMVTYPE);

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);			
			debug("erg = %X",iResult);
			//build_C53_C54Offline();		
			
  		    EMV_EndTransactionCTLS("\nTransaccion Declinada", 0, FALSE);

			//sleep(2);
		case EMV_ADK_AAR:
		case EMV_ADK_ABORT:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_CVM:
		case EMV_ADK_CARDERR_FORMAT:
		default:

			debug("\n***Otro error**** ");
			EMV_EndTransactionCTLS("\nError en Transaccion", 0, FALSE);		
		return iResult;
		}

		//Swith para terminar la transacción
		Info_TRX_CTLS_EMV();
		
	return iResult;
}

int CTLS_MSR(int erg)	
{
	LOGF_TRACE("\n--CTLS_MSR--");
	int iResult = 0;
	char trk1[100] = {0};
	int sztrk1 = 0;
	char trk2[50] = {0};
	int sztrk2 = 0;	
	char CVV[6] = {0};
	int szCVV2 = 4;
	map<string,string> value;
	
	//Variables necesarias para armar el comando C53
	if( boDUKPTasm )
	{
		tr2_ctls(&sztrk2,trk2,&sztrk1,trk1);	

		dump((char*)&CTLSTransRes,sizeof(CTLSTransRes),(char *)"Result structure");	
		
		iResult = in_txn_crd (trk1,sztrk1,trk2,sztrk2,CVV, szCVV2,CTLS,CVV_EMVTYPE);
	}

	//Valores necesaros para construir el mensaje C53 para CTLS MSR	
	//build_C53_C54Offline();

	return iResult;
}


int tr2_ctls(int* sztrk2Asc,char* trk2Asc,int* sztrk1,char* trk1Asc)
{
	int iResult = 0;
	unsigned char LRC = 0;
	char trk2[50] = {0};
	int sztrk2 = 0;
	char aux;
	int i = 0;
	int idx_trk1 = 0;
	int idx_trk1_aux = 0;

	unsigned long TAG = 0;
	unsigned char buffTAG[150] = {0};
	unsigned short szTAG = 0;

	*sztrk2Asc = 0;
	
	LOGF_TRACE("\nCTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2len = %i ",CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2len);
	LOGAPI_HEXDUMP_TRACE("\nTrack 2",&CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2data,sizeof(CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2data));

	memcpy(trk2 + sztrk2,CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2data,CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2len);
	sztrk2+=CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2len;	
	
	vdHex2Asc(trk2,&trk2Asc[1],sztrk2);
	memcpy(trk2Asc,";",1);
	
	*sztrk2Asc = (2*sztrk2) + 1;

	for(i = 1; i < (*sztrk2Asc) ; i++)
	{
		if(!memcmp(&trk2Asc[i],"D",1))
		{
		   memcpy(&trk2Asc[i],"=",1);
		   idx_trk1 = i;
		   idx_trk1_aux = i;
		}
		if(!memcmp(&trk2Asc[i],"F",1))
		{
		   memcpy(&trk2Asc[i],"?",1);
		}
	}

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F20;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS,true);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE((char *)"\ncARD HOLDER NAME",buffTAG,szTAG); 
		memcpy(CTLSTransRes.T_5F20_Cardholder.crdName,(char*)buffTAG,szTAG);
		CTLSTransRes.T_5F20_Cardholder.crdNameLen = szTAG;
		   
	}
	else
	{
		LOGF_TRACE("\nTcARD HOLDER NAME is not available");
		if(strlen ((char*)CTLSTransRes.T_5F20_Cardholder.crdName) == 0)
		{
			memcpy(CTLSTransRes.T_5F20_Cardholder.crdName,"                          ",26);
			CTLSTransRes.T_5F20_Cardholder.crdNameLen = 26;
		}
	}

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x56;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS,true);
	if( (iResult == EMV_ADK_OK) && (szTAG > 2) )
	{
		  LOGAPI_HEXDUMP_TRACE((char *)"\nTrack 1 Equivalent Data",buffTAG,szTAG);    
		  memcpy(trk1Asc,"%",1);
		  strcat(trk1Asc,(char*)buffTAG);
		  *sztrk1 = strlen(trk1Asc);
	}
	else
	{
		LOGF_TRACE("\nTrack 1 Equivalent Data is not available asm");
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		memcpy(trk1Asc,"%B",2);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		memcpy(trk1Asc + 2 ,&trk2Asc[1],idx_trk1-1);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1+=1;
		memcpy(trk1Asc + idx_trk1,"^",1);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1++;

		memcpy(trk1Asc + idx_trk1,CTLSTransRes.T_5F20_Cardholder.crdName,CTLSTransRes.T_5F20_Cardholder.crdNameLen);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1+=CTLSTransRes.T_5F20_Cardholder.crdNameLen;

		memcpy(trk1Asc + idx_trk1," ",1);//TODO solo un espacio
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1+=1;

		memcpy(trk1Asc + idx_trk1,&trk2Asc[1] + (idx_trk1_aux+16),5);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1+=5;//strlen(&trk2Asc[1])-(idx_trk1_aux+16);	

		memcpy(trk1Asc + idx_trk1,"^",1);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1++;
		//memcpy(trk1Asc + idx_trk1,&trk2Asc[1] + (idx_trk1_aux),strlen(&trk2Asc[1])-(idx_trk1_aux-5));        
		// idx_trk1+=strlen(&trk2Asc[1])-(idx_trk1_aux-5);   
		// *sztrk1 = idx_trk1;
		memcpy(trk1Asc + idx_trk1,&trk2Asc[1] + (idx_trk1_aux),sztrk2-6);
		LOGF_TRACE("Track1 [%s]",trk1Asc);
		idx_trk1+=sztrk2-6;
		
		memcpy(trk1Asc + idx_trk1,"?",1);
		LOGF_TRACE("Track1 [%s]",trk1Asc);	
		idx_trk1++;
		
		// memcpy(trk1Asc + idx_trk1,"\x01",1);
		// LOGF_TRACE("Track1 [%s]",trk1Asc);	
		// idx_trk1++;
		*sztrk1 = idx_trk1;
	}

	LOGF_TRACE("\nTrack 1 = %s",trk1Asc);	
	LOGF_TRACE("\nTrack 2: %s",trk2Asc);
	LOGF_TRACE("\nCrdHold : %s",CTLSTransRes.T_5F20_Cardholder.crdName);
	   
	
	return iResult;
}


int read_PAN_MSR(int MSRead,char* pan_out,int* szPAN)
{
	int in_val = 0;
	char PAN[20] = {0};
	int lenPAN = 0;

	LOGF_TRACE("\n---read_PAN_MSR---");	
	
	if( (MSRead == MSR_OK) || (MSRead == MSR_ACTIVE) )
		in_val = MSR_OK;
	else
	{
		LOGF_TRACE("MSR_ERROR!!");				
	}
			
	in_val = Read_MSRdata();	
	LOGF_TRACE("\n in_val = %i",in_val);
	
	if(in_val == MSR_OK )
	{		
		in_val = getMSR_data((char *)"Pan",PAN,&lenPAN);
		if( (lenPAN % 2) == 0)
			*szPAN = lenPAN/2;
		else
			*szPAN = (lenPAN/2) + 1;
		vdAsc2Hex (PAN, pan_out, *szPAN);
		LOGF_TRACE(pan_out,*szPAN,"\n****PAN from MSR*****");
	}
	else if(in_val == MSR_ERROR )
	{
		LOGF_TRACE("MSR_ERROR!!");
	}
	else if(in_val == MSR_TIMEOUT )
	{
		LOGF_TRACE("MSR_TIMEOUT!!");
	}
	else if(in_val == MSR_ABORTED)
	{
		LOGF_TRACE("MSR_ABORTED!!");
	}
	
	return in_val;
}

int read_PAN_EMV(char* PanEMV)
{
	int in_val = 0;
	int inResult = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];

	char TAG5A[20] = {0};
	int sztag5A = 0;

	unsigned char Amount[14] = {0};
	unsigned char ucReader=0;
	unsigned char cashback[14] = {0};

	LOGF_TRACE("\n--read_PAN_EMV--");
	LOGF_TRACE("\n\n");
	

	memset((void*)&xSelectResCT,0x00,sizeof(xSelectResCT)); // result of the contact selection process
	memset((void*)&AdditionalTxDataCT,0x00,sizeof(AdditionalTxDataCT));
	memset((void*)&TxDataCT,0x00,sizeof(TxDataCT)); //CT Data. Parameters for start transaction (application selection processing)
	memset((void*)&xEMVTransResCT,0x00,sizeof(xEMVTransResCT));
	memset((void*)&xOnlineInputCT,0x00,sizeof(xOnlineInputCT));
	memset((void*)&pxTransRes,0x00,sizeof(pxTransRes));

	
	Set_CT_TransactionData(&TxDataCT, (char *) Amount,(char*)cashback,0x00);  //Setup CT transaction data 
		
	in_val = EMV_AppSelection(&TxDataCT,&xSelectResCT);

	LOGF_TRACE("\nin_Val = %i\n",in_val);

    in_val = EMV_ReadRecords(&TxDataCT,&xSelectResCT,&pxTransRes,&AdditionalTxDataCT); 

	LOGF_TRACE("\nin_Val = %i\n",in_val);
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5A;
	inResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
	if( (inResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		sztag5A = szTAG - 2;		
		//vdHex2Asc((char*)(buffTAG + 2), TAG5A, sztag5A);
		memcpy(PanEMV,buffTAG + 2,sztag5A);
		//debug("PAN from EMV = %s",TAG5A);
		LOGF_TRACE(PanEMV,sztag5A,"*****PAN EMV*****");
	}
	else
		LOGF_TRACE("\nPAN is not available");
	
    EMV_CT_EndTransaction(0);
	return sztag5A;
}

unsigned char EMVFLOW(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType)
{
	int Remove_card = 0;
	int iResult = 0;
	unsigned char buffTAG[15] = {0};  // FAG 18-SEP-2017
	char buffTAGASC[12]={0};
	unsigned long TAG = 0;// FAG 18-SEP-2017 
	unsigned short szTAG = 0; // FAG 18-SEP-2017   
	unsigned char eEMVInfo;
	map<string,string> value;  // FAG 22-SEP-2017
	LOGF_TRACE("\n--EMVFLOW--");

    	eEMVInfo = EMV_ReadRecords(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType); 
	LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	Remove_card = _CHECK_INSERTED_CARD();
	if( (Remove_card == FALSE))
		return CARD_REMOVED;	


	// METEMOS LA VALIDACION DE LA FECHA DE LA TARJETA CUANDO ES CHIP
	TAG = 0x5F24;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("EXP DATE",buffTAG,szTAG);
		vdHex2Asc((char *)buffTAG+3,buffTAGASC,2);
		LOGAPI_HEXDUMP_TRACE("EXP DATE ASC",buffTAGASC,sizeof(buffTAGASC));
		iResult=incheck_expdate(buffTAGASC);
		LOGF_TRACE("** iResult  = %i**",iResult);
		if(iResult<0)
		{
			LOGF_TRACE("\nCancelada por timeout");
			//value["BODY_MSG"]="TIEMPO EXCEDIDO";
			//value["NAME_IMG"]="EXTLOADKEY";
			value["HEADER_TITLE"]="";
			value["BODY_HEADER"]="TARJETA EXPIRADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTCARDEXPIRED";
		
			uiInvokeURL(value,"GenScreen_alert.html");
			
			sleep(SHOW_ALERT);
			
			value["HEADER_TITLE"]="COMANDO";
			value["BODY_HEADER"]="PROCESANDO";
			value["BODY_MSG"]="ESPERE UN MOMENTO";
			
			uiInvokeURL(value,"Procesando.html");

			sleep(SHOW_ALERT);
		}
	
	}

	
	eEMVInfo = EMV_DataAuthentication(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
	LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	Remove_card = _CHECK_INSERTED_CARD();
	if(Remove_card == FALSE)
		return CARD_REMOVED;	
	
	eEMVInfo = EMV_CardHolderVerification(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
	LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	Remove_card = _CHECK_INSERTED_CARD();
	if(Remove_card == FALSE)
		return CARD_REMOVED;

	if(eEMVInfo == EMV_CT_PIN_INPUT_ABORT)
	{
		LOGF_TRACE("\n Pin cancelado por el usuario");
		return EMV_CT_PIN_INPUT_ABORT;
	}

	if(eEMVInfo == EMV_CT_PIN_INPUT_TIMEOUT)
	{
		LOGF_TRACE("\n Pin cancelado por Timeout");
		return EMV_CT_PIN_INPUT_TIMEOUT;
	}

	if(eEMVInfo != EMV_ADK_APP_REQ_CVM_END)
	{
		
		eEMVInfo = EMV_CardHoldVerificationProcess(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
		LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	}
	Remove_card = _CHECK_INSERTED_CARD();
	if(Remove_card == FALSE)
		return CARD_REMOVED;	

	eEMVInfo = EMV_RiskManagement(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
	LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	Remove_card = _CHECK_INSERTED_CARD();
	if(Remove_card == FALSE)
		return CARD_REMOVED;	

	eEMVInfo = EMV_FirstGenerateAC(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
	LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
	Remove_card = _CHECK_INSERTED_CARD();
	if(Remove_card == FALSE)
		return CARD_REMOVED;	

	return eEMVInfo;
}


int in_txn_crd (char* buff1, int szbuff1, char* buff2, int szbuff2, char* buff3, int szbuff3, int type_inp, int flgCVV)
{
	int in_val = SUCCESS_TRX;
	int typCTLS = 0;
	char ch_es [100] = {0};	//TOKEN ES
	char ch_ez [150] = {0};	//TOKEN EZ
	char ch_ey [200] = {0};	//TOKEN EY
	char ch_q9 [300] = {0};	//TOKEN q9
	char ch_q8 [300] = {0};	//TOKEN q8
	char ch_qe [60]  = {0}; //TOKEN qe
	
	int  in_es   = 0;	//SIZE OF TOKEN ES
	int  in_ez   = 0;	//SIZE OF TOKEN EZ
	int  in_ey   = 0;	//SIZE OF TOKEN EY
	int  in_q8   = 0;	//SIZE OF TOKEN Q8
	int  in_q9   = 0;	//SIZE OF TOKEN Q9
	int  in_qe   = 0;   //SIZE OF TOKEN QE

	int chk_AMEX = 0;
	bool isFllback = false;
	
	LOGF_TRACE ("\n\n--in_txn_crd--");
	LOGF_TRACE("\nTHE ENCRYPTION BEGINS!!");


	in_val = SUCCESS_TRX;

	in_val = in_cpx_crd_opn ();

	LOGF_TRACE("\n");
	LOGF_TRACE("buff1 = %s",buff1);
	LOGF_TRACE("\n");
	LOGF_TRACE("buff2 = %s",buff2);
	LOGF_TRACE("\n");
	LOGF_TRACE("buff3 = %s",buff3);


	if(in_val == SUCCESS_TRX)	
	{				
		switch(type_inp)
		{
			case CHIP:  
				LOGF_TRACE("\n ENCRYPTION OF EMV DATA");
				in_val = crypCAPX(CTS_CHIP);
			break;
			case MSR:
				LOGF_TRACE("\n ENCRYPTION OF MSR DATA");
				if(count_fallback_get() > 1)
					isFllback = true;
				
				in_val = cryp_msr(buff1,buff2,buff3,isFllback,false,flgCVV);			
			break;
			case KBD:
				LOGF_TRACE("\n ENCRYPTION OF KBD DATA");
				in_val = in_cpx_crd_kbd (buff1, strlen(buff1),buff2, strlen(buff2), buff3, strlen(buff3));
			break;
			case CTLS:
				LOGF_TRACE("\n ENCRYPTION OF CTLS DATA");

				typCTLS = Check_typ_CTLS();	
				
				if(typCTLS == CTLS_TXN_CHIP)
				{
					LOGF_TRACE("\n***CTLS CHIP****");
					in_val = crypCAPX(CTS_CTLS);					
				}
				else if(typCTLS == CTLS_TXN_MSR)
				{		
					LOGF_TRACE("\n***CTLS MSR****");
					in_val = cryp_msr(buff1,buff2,buff3,false,true,CVV_NOTASKED);
				}
				else
				{
					LOGF_TRACE("\n***TAP ERROR****");
				}
				
			break;
			default:
				in_val = ERROR_INP_MODE;
				LOGF_TRACE("\n UNKNOW INPUT MODE!!");
			break;
			
		}		
	}
	
	if(in_val == SUCCESS_TRX)
	{
		LOGF_TRACE("\nGenerating of TOKEN ES, EZ & EY");
		memset (ch_es, 0, sizeof (ch_es));
		memset (ch_ez, 0, sizeof (ch_ez));
		memset (ch_ey, 0, sizeof (ch_ey));
		in_es = 0;
		in_ez = 0;
		in_ey = 0;

		in_val = in_cpx_crd_cry (ch_es, &in_es, ch_ez, &in_ez, ch_ey, &in_ey);
		
		//We store the value of the generated tokens

		vdSetByteValuePP((char*)"TOKES",ch_es,in_es);
		vdSetByteValuePP((char*)"TOKEZ",ch_ez,in_ez);
		vdSetByteValuePP((char*)"TOKEY",ch_ey,in_ey);
		
		LOGF_TRACE("\n\n");
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN ES",ch_es, in_es);
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN EZ",ch_ez, in_ez);
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN EY",ch_ey, in_ey);		

		memset (ch_es, 0, sizeof (ch_es));
		memset (ch_ez, 0, sizeof (ch_ez));
		memset (ch_ey, 0, sizeof (ch_ey));
		in_es = 0;
		in_ez = 0;
		in_ey = 0;
	
		in_es = inGetByteValuePP((char*)"TOKES",ch_es);
		in_ez = inGetByteValuePP((char*)"TOKEZ",ch_ez);
		in_ey = inGetByteValuePP((char*)"TOKEY",ch_ey);

		LOGF_TRACE("\n\n");
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN ES",ch_es, in_es);
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN EZ",ch_ez, in_ez);
		LOGAPI_HEXDUMP_TRACE((char *)"TOKEN EY",ch_ey, in_ey);		
	}
	
	return in_val;
}

int identify_AMEX_card(int tech)
{
	int iResult = 0;
	
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100] = {0};
	unsigned char TAG_4F[100] = {0};
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,tech);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		if(szTAG > 2)
			{			
			memcpy(TAG_4F,buffTAG+2,szTAG-2);
			dump(TAG_4F,szTAG-2,(char *)"AID");
			}
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
			{				
				szTAG = pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				memcpy(TAG_4F,pxTransRes.xEMVTransRes.T_9F06_AID.AID,szTAG);
				dump(TAG_4F,szTAG,(char *)"AID");
			}
			else if(CTLSTransRes.T_9F06_AID.aidlen > 0)
			{
				szTAG = CTLSTransRes.T_9F06_AID.aidlen;
				memcpy(TAG_4F,CTLSTransRes.T_9F06_AID.AID,szTAG);
				dump(TAG_4F,szTAG,(char *)"AID");
			}
			else	
			{
				LOGF_TRACE("\nAID is not available");
				iResult = NO_AMEX_CARD;	
			}
		} 
			
	else
	{
		LOGF_TRACE("\nAID is not available");
		iResult = NO_AMEX_CARD;
	}

	if(!memcmp(TAG_4F,"\xA0\x00\x00\x00\x25",5))
	{
		LOGF_TRACE("\nEs una tarjeta AMEX");
		iResult = AMEX_CARD;
	}
	else 
	{
		LOGF_TRACE("\nNo es una tarjeta AMEX");
		iResult = NO_AMEX_CARD;

	}
	return iResult;
}


int crypCAPX(int tech){
	int inResult = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100] = {0};

	unsigned char tag57[25]= {0};
	int sztag57 = 0;

	unsigned char tag5F20[30]= {0};
	int sztag5F20 = 0;

	bool isCtls = false; 

	if(tech == CTS_CTLS)
		isCtls = true;
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x57;
	inResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech );
	if( (inResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		dump(buffTAG,szTAG,(char *)"\nTrack 2 Equivalent Data");	
		sztag57 = szTAG;
		memcpy(tag57,buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F20;
	inResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,tech);
	if( (inResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		dump(buffTAG,szTAG,(char *)"Cardholder Name");	
		sztag5F20 = szTAG;
		memcpy(tag5F20,buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nCardholder Name is not available");
	

	inResult = in_cpx_crd_chp ((char*)tag57, sztag57, (char*)tag5F20, sztag5F20,isCtls);
	return inResult;

}

//TOKENS BANAMEX

int inAsmBnmxQ8 (char pchSnd [])
{
	
	char chVal [30] = {0};
	char pchVal[5+1] = {0};
	int inVal;
	int inSnd;
	char CVMR[7] = {0};
	unsigned short usLen;
	char temp[6]= {0};
	int iCvm=0;

	debug("\n--inAsmBnmxQ8--\n");
	//debug((char *)"inAsmBnmxQ8");

	inSnd = 0;

	memcpy (pchSnd + inSnd, "! Q8", 4);

	dump (pchSnd + inSnd, 4,(char *) "hdr");
	inSnd += 4;

	memcpy (pchSnd + inSnd, "00026", 5);
	dump (pchSnd + inSnd, 5, (char*)"siz");
	inSnd += 5;

	memcpy (pchSnd + inSnd, " ", 1);
	dump (pchSnd + inSnd, 1,(char*) "fld");
	inSnd += 1;

	memset (chVal, 0, sizeof (chVal));
	pchGetAppVer (chVal);
	memcpy (pchSnd + inSnd, chVal + 0, 7);

	dump (pchSnd + inSnd, 7,(char*) "apl");

	inSnd += 7;

	memcpy (pchSnd + inSnd, chVal + 7, 3);

	dump (pchSnd + inSnd, 3, (char*)"ver");

	inSnd += 3;

	memset (chVal, 0, sizeof (chVal));
	pchGetSrlNmb (chVal);
	pad (chVal, chVal, ' ', 12, RIGHT_);
	memcpy (pchSnd + inSnd, chVal, 12);

	dump (pchSnd + inSnd, 12, (char*)"srl");

	inSnd += 12;

	memset(pchVal,0,sizeof(pchVal));
	strcat(pchVal,"0");

	//if (inPinTried)
	//pchVal = "1";
	//memset(pchVal,0,sizeof(pchVal));
	//strcat(pchVal,"1");
	

	/*if (strcmp(chComando810, "CA8"))
	{
		if(usEMVGetTLVFromColxn(TAG_9F34_CVM_RESULTS, CVMR, &usLen) == EMV_SUCCESS)
		{

			iCvm = CVMR[0]&0x3F;
	                 
			
			if(iCvm == 3 || iCvm == 5 )// || (CVMR[0]&0x3F) == 0x05 )
			{
				pchVal = "2";
			}

		}
	}*/
	
	
	memcpy (pchSnd + inSnd, pchVal, 1);
	dump (pchSnd + inSnd, 1,(char*) "sgn");
	inSnd += 1;

	//pchVal = "001";
	memset(pchVal,0,sizeof(pchVal));
	strcat(pchVal,"001");
	/*pobTran = pstGet_pobTran ();
	
	//SI ES FLUJO EMV
	if (pobTran)
	{
		//CTLS
		#ifdef VX820
		if(id_entrada==1)                                       //CTLS MCHIP
			pchVal = "071";
		else if(id_entrada==2)                                  //CTLS MagStripe
			pchVal = "911";
		//RHM
		else
		#endif
		{
			if (!memcmp(g_srCrdC51.chMod, "05", 2))
			pchVal = "051";
			else if (!memcmp(g_srCrdC51.chMod, "90", 2))		
			pchVal = "901";
			else if (!memcmp(g_srCrdC51.chMod, "80", 2))		//RHM_55 3
			{
				//Antes
				//pchVal = "801";			
				//Parche para caso especial de Fallback con tarjeta con CHIP pero con service code 1XX. RHM
				if(pobTran->szServiceCode[0] == '2' || pobTran->szServiceCode[0] == '6')				//Si la tarjeta trae chip, es fallback puro
					pchVal = "801";
				else											//Si la tarjeta es puramente banda
					pchVal = "901";
				//Parche para caso especial de Fallback con tarjeta con CHIP pero con service code 1XX. RHM
			}
			else
			pchVal = "011";
		}
	}*/

	memcpy (pchSnd + inSnd, pchVal, 3);
	dump (pchSnd + inSnd, 3, (char*)"pem");
	inSnd += 3;
	dump (pchSnd, inSnd,(char*) "TOKEN Q8");
	
	return inSnd;
}

int inAsmBnmxQ9 (char Q9 [],unsigned char chTechnology)
{
	int inB2I = 0;
	int sztrk1 = 0;
	int szCrdHold = 0;
	int inPan = 0;
	int status = 0;
	unsigned char bAppLabel [50] = {0};
	unsigned char bAppPrefName [50] = {0};
	unsigned char btCardholder [50] = {0};
	unsigned char btCardholdere_aux [50] = {0};
	unsigned char bTSI [10] = {0};
	unsigned short tagLen = 0;
	unsigned long TAG = 0;
	unsigned char chPan [25] = {0};
	unsigned char chPanAsc[25] = {0};
	unsigned char chTrk1 [100] = {0};
	char byPinKSN[20] = {0};
	char temp [100] = {0};
	
	LOGF_TRACE("\n--inAsmBnmxQ9--\n");
	
	// Q9
	inB2I = 0;
	Q9[inB2I] = '!';
	inB2I+=1;
	Q9[inB2I] = ' ';
	inB2I+=1;
	strcpy((char *)&Q9[inB2I], "Q9");
	inB2I+=2;

	strcpy((char *)&Q9[inB2I], "00238");
	inB2I+=5;
	Q9[inB2I] = ' ';
	inB2I+=1;
	
	
	memset(bAppLabel,0,sizeof(bAppLabel));
	memset(bAppPrefName,0,sizeof(bAppPrefName));
	memset(bTSI,0,sizeof(bTSI));

	TAG = 0x50;
	if( getCT_EMV_TAG(0,&TAG,1,bAppLabel,sizeof(bAppLabel),&tagLen,chTechnology) == EMV_ADK_OK )  //ISSUE 57 JRS
	{
		sprintf(&Q9[inB2I],"% 16s",bAppLabel + 2);
		inB2I+=16;
		//debug ("TAG_50_APPL_LABEL ASCII= %s",bAppLabel);
	}
	else
	{

		memset(&Q9[inB2I],0x20,16);
		inB2I+=16;
		
	}
	
	TAG = 0x9F12;
	if( getCT_EMV_TAG(0,&TAG,1,bAppPrefName,sizeof(bAppPrefName),&tagLen,chTechnology) == EMV_ADK_OK ) 
	{

		//if (!boPrtChr (bAppPrefName, (int) tagLen))
		if(!tagLen)
		{
			//memset (&Q9[inB2I], 0x20, sizeof (bAppPrefName));
			memset (&Q9[inB2I], 0x20, 16);
			inB2I+=16;
		}
		else
		{
			sprintf(&Q9[inB2I],"% 16s",bAppPrefName + 3);
			inB2I+=16;
			
		}
	}
	else
	{

		memset(&Q9[inB2I],0x20,16);
		inB2I+=16;
		
	}

	TAG = 0x9B;
	if( getCT_EMV_TAG(0,&TAG,1,bTSI,sizeof(bTSI),&tagLen,chTechnology) == EMV_ADK_OK )
	{
		//E8 00 00 00
		//debug("*****************TAG_9B00_TSI ASCII*********************");
		memset(temp,0,sizeof(temp));
		sprintf(temp,"%02X%02X",bTSI[2],bTSI[3]);
		memcpy(&Q9[inB2I],temp,strlen(temp));
		inB2I+=4;
	}
	else
	{
		memset(&Q9[inB2I],0,4);
		inB2I+=4;
	}
		
	memset (Q9 + inB2I, ' ', 30);
	inB2I += 30;
	
	if(chTechnology == CTS_MSR)
	{
		status = getMSR_data((char *)"Pan",(char*)chPan,&inPan);
		sprintf(&Q9[inB2I],"%03d",inPan);
		inB2I+=3;
		
		pad ((char*)chPan, (char*)chPan, ' ', 40, LEFT_);	
		sprintf(&Q9[inB2I],"%s",chPan);
		inB2I+=40;
		
		status = getMSR_data((char *)"Track1",(char*)chTrk1,&sztrk1);
		sprintf(&Q9[inB2I],"%03d",sztrk1);
		inB2I+=3;
		
		pad ((char*)chTrk1,(char*) chTrk1, ' ', 80, LEFT_);
		sprintf(&Q9[inB2I],"%s",chTrk1);
		inB2I+=80;
		
		memset((char*)byPinKSN,70,sizeof(byPinKSN));
		strcpy(Q9 + inB2I,byPinKSN);
		inB2I+=16;
		
		status = getMSR_data((char *)"Cardholder",(char*)btCardholder,&szCrdHold);
		pad ((char*)btCardholder, (char*)btCardholder, ' ', 30, LEFT_);	
		strcpy(Q9 + inB2I,(char*)btCardholder);
		inB2I+=30;
	
	}
	if((chTechnology == CTS_CHIP) || (chTechnology == CTS_CTLS))
	{
		TAG = 0x5A;
		if( getCT_EMV_TAG(0,&TAG,1,chPan,sizeof(chPan),&tagLen,chTechnology) == EMV_ADK_OK )
		{
			sprintf(&Q9[inB2I],"%03d",2*(tagLen - 2));
		}
		else
			strcpy(&Q9[inB2I],"000");
		inB2I+=3;

		vdHex2Asc((const char*)chPan + 2,(char*) chPanAsc, tagLen - 2);
		
		pad ((char*)chPanAsc,(char*) chPanAsc, ' ', 40, LEFT_);	
		sprintf(&Q9[inB2I],"%s",chPanAsc);
		inB2I+=40;
		//CHIP NO TIENE TRACK1
		strcpy(&Q9[inB2I],"000");
		inB2I+=3;
		memset(Q9 + inB2I,' ',80);
		inB2I+=80;
		memset((char*)byPinKSN,70,sizeof(byPinKSN));
		strcpy(&Q9[inB2I],byPinKSN);
		inB2I+=16;
		
		TAG = 0x5f20;
		if( getCT_EMV_TAG(0,&TAG,1,btCardholder,sizeof(btCardholder),&tagLen,chTechnology) != EMV_ADK_OK )
		{
			btCardholder[0] = 0;
		}
		
		pad ((char*)btCardholdere_aux,(char*) btCardholder + 3, ' ', 30, LEFT_);
		strcpy(Q9 + inB2I,(char*)btCardholdere_aux);
		inB2I+=30;
	}

	if(chTechnology == CTS_KBD)
	{	
		inPan = 0;

		sprintf(&Q9[inB2I],"%03d",inPan);
		inB2I+=3;

		memset(chPan,0x00,sizeof(chPan));		
		pad ((char*)chPan, (char*)chPan, ' ', 40, LEFT_);	
		sprintf(&Q9[inB2I],"%s",chPan);
		inB2I+=40;
		
		sztrk1 = 0;		
		sprintf(&Q9[inB2I],"%03d",sztrk1);
		inB2I+=3;

		memset(chTrk1,0x00,sizeof(chTrk1));
		pad ((char*)chTrk1,(char*) chTrk1, ' ', 80, LEFT_);
		sprintf(&Q9[inB2I],"%s",chTrk1);
		inB2I+=80;
		
		memset((char*)byPinKSN,70,sizeof(byPinKSN));
		strcpy(Q9 + inB2I,byPinKSN);
		inB2I+=16;

		memset(btCardholder,0x00,sizeof(btCardholder));
		pad ((char*)btCardholder, (char*)btCardholder, ' ', 30, LEFT_);	
		strcpy(Q9 + inB2I,(char*)btCardholder);
		inB2I+=30;
	}
	dump(Q9,inB2I,(char *)"\nTOKEN Q9");

	
	if(inB2I != 248) //POR ESPECIFICACION
		inB2I = -1;
		
return inB2I;
		
}


int inAsmBnmxQE (char* QE,int tech)
{
	int szQE = 0;
	int iResult = 0;
	unsigned short tagLen = 0;
	unsigned long TAG = 0;
	unsigned char bufftag[50] = {0};
	unsigned char TAG_9F5A [50] = {0};
	unsigned char TAG_9F5B [50] = {0};
	
	debug("\n--inAsmBnmxQE--\n");

	memcpy(QE + szQE,"!",1);
	szQE += 1;

	memcpy(QE+ szQE," ",1);
	szQE += 1;

	memcpy(QE+ szQE,"QE",2);
	szQE += 2;

	memcpy(QE+ szQE,"00040",5);
	szQE += 5;

	memcpy(QE+ szQE," ",1);
	szQE += 1;
			//9F5A - AMEX-MBR-PROD-ID
	TAG = 0x9F5A;
	tagLen = 0;
	memset(TAG_9F5A,0x00,sizeof(TAG_9F5A));
	memset(bufftag,0x00,sizeof(bufftag));
	if( getCT_EMV_TAG(0,&TAG,1,bufftag,sizeof(TAG_9F5A),&tagLen,tech) == EMV_ADK_OK )  
	{	
		memcpy(TAG_9F5A,bufftag+3,tagLen-3);
		pad ((char*)TAG_9F5A, (char*)TAG_9F5A, ' ', 8, RIGHT_);
		memcpy(QE+ szQE,TAG_9F5A,8);
		dump(QE+ szQE,8,(char *)"\nTAG 9F5A");
		szQE+=8;  //Checar con spec
	}
	else
	{

		memset(&QE[szQE],0x20,8);
		szQE+=8;
		
	}

	//9F5B - AMEX-MBR-PROD-NBR
	TAG = 0x9F5B;
	tagLen = 0;
	memset(TAG_9F5B,0x00,sizeof(TAG_9F5B));
	memset(bufftag,0x00,sizeof(bufftag));
	if( getCT_EMV_TAG(0,&TAG,1,bufftag,sizeof(TAG_9F5B),&tagLen,tech) == EMV_ADK_OK )  
	{
		memcpy(TAG_9F5B,bufftag+3,tagLen-3);
		pad ((char*)TAG_9F5B, (char*)TAG_9F5B, ' ', 32, RIGHT_);
		memcpy(QE+ szQE,TAG_9F5B,32);
		dump(QE+ szQE,32,(char *)"\nTAG 9F5B");
		szQE+=32;
	}
	else
	{

		memset(&QE[szQE],0x20,32);
		szQE+=32;
		
	}

	dump(QE,szQE,(char *)"TOKEN QE");
	
	return szQE;
}

int build_C53_C54Offline()
{
	int iResult = 0;
	//Variables para armar el campo
	char Length[5] = {0};
	int szLength = 0;
	char lenfld = 0;
	char ciphermode[4] = {0};
	int szciphermode = 0;
	char pan[50] = {0};
	int szpan = 0; 
	char CardHold[50] = {0};
	int szCardHold = 0; 
	char trk2[50] = {0};
	int sztrk2 = 0; 
	char trk1[50] = {0};
	int sztrk1 = 0; 
	char sec_code[50] = {0};
	int szsec_code = 0; 
	char posEntry[50] = {0};
	int szposEntry = 0; 
	char toke1[1024] = {0};
	int sztoke1 = 0;
	char toke2[1024] = {0};
	int sztoke2 = 0;	
	int sizeCmd = 0;
	//char tokR1[30] = {0};
	//int szTokR1 = 0;
	char tokCz[100] = {0};
	int sztokCz = 0;
	//PAra verificar tipo de CTLS
	int typctls = 0;
	//Variables para recuperar el tama�o de los tokens generados

	char ch_es [100] = {0};	//TOKEN ES
	char ch_ez [150] = {0};	//TOKEN EZ
	char ch_ey [200] = {0};	//TOKEN EY	
	
	int  in_es   = 0;	//SIZE OF TOKEN ES
	int  in_ez   = 0;	//SIZE OF TOKEN EZ
	int  in_ey   = 0;	//SIZE OF TOKEN EY
	
	
	
	unsigned char ch_val[4] = {0};
	char ch_asc [70] = {0};
	int  in_asc = 0;
	char tmpbuff[50] = {0};
	int sztmp =0;
	char CVV[6] = {0};
	int szCVV2 = 4;
	char dataENCRYPT[32];
	int szdataENCRYPT = 0;
	char tmpbuf[32];
	int inencrypt=0;
	char pchCipher[128]={0};
	char pchAsc[128] = {0};

	//Variables necesarias para EMV CHIP y CTLS CHIP
	unsigned long TAG = 0;
	unsigned char buffTAG[50] = {0};
	unsigned short szTAG = 0;
	int tech = 0;
	
	LOGF_TRACE("--build_C53--");	

	typctls = Check_typ_CTLS();

	LOGF_TRACE("C53 chTechnology %d", chTechnology);
	
	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		if(chTechnology == CTS_CHIP)
		{
			LOGF_TRACE("C53 Para EMV Chip");
			tech = CTS_CHIP;
		}
		else if(chTechnology == CTS_CTLS)
		{
			LOGF_TRACE("C53 Para CTLS CHIP");
			tech = CTS_CTLS;
		}
		
	
		  
		//Pan
		memcpy(pan,"\xC1",1);				
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5A;		
		iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			//szpan = szTAG ;
			//pan[1] = szpan - 2;
			//memcpy(&pan[2],buffTAG + 2,szpan - 2);
			LOGAPI_HEXDUMP_TRACE((char *)"*****PAN EMV/CTLS*****",buffTAG,szTAG);
			in_asc = inHexToAsc ((char *)buffTAG + 2, pan + 2, szTAG - 2);
			pan[1]=in_asc;
			LOGAPI_HEXDUMP_TRACE("Track 2 in ASC",pan,sizeof(pan));
			LOGF_TRACE("-- in_asc [%i]--", in_asc);
			
		}
		else
		{
			LOGF_TRACE("\nPAN is not available");
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x57;
			iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech );
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				LOGAPI_HEXDUMP_TRACE((char *)"Track 2 Equivalent Data",buffTAG,szTAG);	
				
			}
			else
			{
				LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
			
			}
			
			if (iResult == SUCCESS)
			{
				char ch_tk2_pan [50] = {0};
				int  in_tk2 = 0;
				char * pch_equ = NULL;
				in_tk2 = inHexToAsc ((char *)buffTAG + 2, ch_tk2_pan, szTAG - 2);

				pch_equ = (char *)memchr (ch_tk2_pan, 'D', in_tk2);

				if (pch_equ)
				{
					*pch_equ = 0;

					in_tk2 = pch_equ - ch_tk2_pan;
				}

				LOGAPI_HEXDUMP_TRACE("TRACK 2 antes de bo_crp_crd",ch_tk2_pan,in_tk2);
				LOGF_TRACE("--valor de in_tk2  [%i]--",in_tk2);
				pan[1]=in_tk2;
				memcpy(pan + 2 ,ch_tk2_pan , in_tk2);
				in_asc = in_tk2;
				
			}
			else
			{
				szpan = 2;
				memcpy(pan,"\xC1\x00",szpan);
			}
		}	

//		LOGF_TRACE("szpan = %i",szpan);

		//sizeCmd+=szpan;
		sizeCmd+=in_asc+2;

		// metemos logica de bines una vez obtenido el pan  FAG

		memset(ch_val,0x00,sizeof(ch_val));
		memcpy(ch_val,"\xC1\x01",2);
			
		if (bo_crp_chp())
		{	
			ch_val[2] = 0x01;
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)ch_val,3);
		}
		else
		{
			ch_val[2] = 0x00;
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)ch_val,3);
		}
		
		sizeCmd+=3;

		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5F24;
		iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			LOGAPI_HEXDUMP_TRACE("EXP_DATE",buffTAG,szTAG);
		else
			LOGF_TRACE("\n EXP DATE ");
			
		sztmp = 0;
		sztmp |=szTAG;
		memset(tmpbuff, 0x00, sizeof(tmpbuff));
		vdHex2Asc((char *)buffTAG + 3 ,tmpbuff + 2,2);
		memcpy(tmpbuff,"\xC1",1);
		tmpbuff[1] = 4;
		vdSetByteValuePP((char*)"EXP_DATE",(char *)tmpbuff,4 + 2);
		//mov += 4 + 2 ;
		sizeCmd+=6;
		
		//Cardholder name
		memcpy(CardHold,"\xC1",1);		
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5F20;		
		iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			szCardHold = szTAG - 3;			
			memcpy(&CardHold[2],buffTAG + 3,szCardHold);
			CardHold[1] = szCardHold; 
			szCardHold+=2;
			//debug("PAN from EMV = %s",TAG5A);
			dump(CardHold,szCardHold,(char *)"*****CARDHOLDER EMV/CTLS*****");
		}
		else
		{
			LOGF_TRACE("CARDHOLDER is not available");
			szCardHold = 2;
			memcpy(CardHold,"\xC1\x00",szCardHold);
		}
		
		sizeCmd+=szCardHold;

		vdSetByteValuePP((char*)"PAN",pan,in_asc+2);	
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,szCardHold);	

		//Track 2
		sztrk2 = 2;
		memcpy(trk2,"\xC1\x00",sztrk2);
		sizeCmd+=sztrk2;
		
		//Track 1
		sztrk1 = 2;
		memcpy(trk1,"\xC1\x00",sztrk1);
		sizeCmd+=sztrk1;
		
		//CCV/4DBC
		szsec_code = 2;
		memcpy(sec_code,"\xC1\x00",szsec_code);
		sizeCmd+=szsec_code;


		vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	

		vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	

		vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);	

		//Pos entry mode
		szposEntry = 4;
		if(chTechnology == CTS_CHIP)
			memcpy(posEntry,"\xC1\x02\x30\x35",szposEntry);
		else if(chTechnology == CTS_CTLS)				
			memcpy(posEntry,"\xC1\x02\x30\x37",szposEntry);		
		sizeCmd += szposEntry;

		//Token E1		
		asm_tke1(toke1,&sztoke1, C53);
		sizeCmd+=sztoke1;
		
		//TOKEN E2
		asmtknE2(toke2,&sztoke2,C53);
		sizeCmd += sztoke2;
		
		
	}	
	else if(/*(chTechnology == CTS_MSR) || */( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) ))
	{	
		char ch_pan [50] = {0};
		int  in_pan = 0;
		char ch_exp[10]={0};
		int  in_exp=0;
		char * pch_equ = NULL;
		bool flag_encrypt=false;
		
		LOGF_TRACE("C53 CTLS/MSR");

		tr2_ctls(&sztrk2,trk2,&sztrk1,trk1);
	
		pch_equ = (char *)memchr (trk2, '=', sztrk2);

		if (pch_equ)
		{
			in_pan = pch_equ - trk2 - 1;
			
			memcpy (ch_pan, trk2 + 1, in_pan);
			
			LOGAPI_HEXDUMP_TRACE("ACC",ch_pan,in_pan);

			pch_equ += 1;
			
			memcpy (ch_exp, pch_equ, 4);
			in_exp = 4;
			LOGAPI_HEXDUMP_TRACE("EXP",ch_exp,in_exp);
		}

		
		
		//Aqu� armamos el campo del Pan para el campo C53 para una transancci�n de banda magn�tica
		if ( bo_crp_crd (ch_pan,in_pan))
		{
			LOGF_TRACE("*****FLAG 01*****");
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x01",3);	
			flag_encrypt=1;
		}
		
		else
		{
			LOGF_TRACE("*****FLAG 00*****");
			vdSetByteValuePP((char*)"CIPHERMODE",(char *)"\xC1\x01\x00",3);	
		
		}
			
		sizeCmd = sizeCmd + 3;

		/* PAN */		
		memcpy(pan,"\xC1",1);
		pan[1] = in_pan;
		memcpy(&pan[2],ch_pan,in_pan);		
		vdSetByteValuePP((char*)"PAN",pan,in_pan + 2);
		sizeCmd = sizeCmd + in_pan + 2;

		/* EXP DATE */
		memset(tmpbuff, 0x00, sizeof(tmpbuff));
		memcpy(tmpbuff,"\xC1",1);
		tmpbuff[1] = in_exp;
		memcpy(&tmpbuff[2],ch_exp,in_exp);
		vdSetByteValuePP((char*)"EXP_DATE",tmpbuff,in_exp+2);
		
		
		sizeCmd = sizeCmd + in_exp + 2;
	

		/* CARD HOLDER */
		memcpy(CardHold,"\xC1",1);
		CardHold[1] = CTLSTransRes.T_5F20_Cardholder.crdNameLen;
		memcpy(&CardHold[2],CTLSTransRes.T_5F20_Cardholder.crdName,CTLSTransRes.T_5F20_Cardholder.crdNameLen);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,CTLSTransRes.T_5F20_Cardholder.crdNameLen+2);

		sizeCmd = sizeCmd + CTLSTransRes.T_5F20_Cardholder.crdNameLen + 2;
			
		//**********************************

		if ( ! inGetEnvVar("NOCV2CTLS",NULL) ) //Daee 14/09/2016
		{
			
			iResult=screenSec_Code(CVV,szCVV2,"INGRESE CODIGO DE SEGURIDAD","");

			//if(iResult==FAILURE_CVV)
			if(iResult< UI_ERR_OK)
			{
				return FAILURE_CVV;
			}
			if(flag_encrypt==1)
			{
				LOGF_TRACE("encriptar el CVV-AES");
				memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
				in_Cx_EDta(CVV,strlen(CVV),dataENCRYPT,&szdataENCRYPT); 
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2,dataENCRYPT,szdataENCRYPT + 2);
				vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,szdataENCRYPT + 2);
				sizeCmd = sizeCmd + inencrypt + 2;				

			}
			else
			{
				memset(tmpbuf,0x00,sizeof(tmpbuf));
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = strlen(CVV);
				memcpy(tmpbuf + 2,CVV,strlen(CVV));
				vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,strlen(CVV) + 2);
				sizeCmd = sizeCmd + strlen(CVV) + 2;				

			}
		
			
		}
		else
		{
			vdSetByteValuePP((char*)"SEC_CODE",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}
			
		if (sztrk2>0)
		{

			if(flag_encrypt==1)
			{
				LOGF_TRACE("***  CIPHER TRACKII AES  ***");
				iResult=in_Cx_EDta(trk2,sztrk2,pchCipher,&(inencrypt)); //	
							
				/* TRACK2 AES */
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = inencrypt;
				memcpy(&pchAsc[2], pchCipher, inencrypt);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,inencrypt+2);
				sizeCmd = sizeCmd + inencrypt + 2;
			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk2;
				memcpy(&pchAsc[2], trk2,sztrk2);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,sztrk2+2);
				sizeCmd = sizeCmd+sztrk2+ 2;		

			}
			
		}
		else
		{
			vdSetByteValuePP((char*)"TRACK2",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}

		if (sztrk1>0)
		{
			if(flag_encrypt==1)
			{
				memset(pchCipher,0,sizeof(pchCipher));
				inencrypt = inGetByteValuePP((char *)"TRACK1",pchCipher);
				LOGAPI_HEXDUMP_TRACE( "track 1 antes de llenar",pchCipher, sizeof(pchCipher));
				LOGF_TRACE("***  CIPHER TRACKI AES	***");
				memset(pchCipher,0,sizeof(pchCipher));
				in_Cx_EDta(trk1,sztrk1,pchCipher,&(inencrypt)); //	
				LOGAPI_HEXDUMP_TRACE( "antes de llenar",pchCipher, sizeof(pchCipher));
				/* TRACK1 AES */
				memset(pchAsc,0,sizeof(pchAsc));			
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = inencrypt;
				memcpy(&pchAsc[2], pchCipher, inencrypt);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,inencrypt+2);
				sizeCmd = sizeCmd + inencrypt + 2;

			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				pchAsc[1] = sztrk1;
				memcpy(&pchAsc[2], trk1,sztrk1);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,sztrk1+2);
				sizeCmd = sizeCmd+sztrk1+ 2;		

			}
		
			
		}
			else
		{
			vdSetByteValuePP((char*)"TRACK1",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}
		
		
		//PEM
		LOGF_TRACE("banda CTLS");
		szposEntry = 4;
		memcpy(posEntry,"\xC1\x02\x39\x31",szposEntry);
		
		sizeCmd = sizeCmd + 4;
			
			
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);

		//Token E1		
		sztoke1 = 3;
		memcpy(toke1,(char*)"\xE1\x01\x00",sztoke1);
		sizeCmd = sizeCmd + 3;
		
		//TOKEN E2
		sztoke2 = 3;
		memcpy(toke2,(char*)"\xE2\x01\x00",sztoke2);
		sizeCmd = sizeCmd + 3;

		//CTLS BANDA ASM 9F6E
		memset(tmpbuff,0,sizeof tmpbuff);		
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x9F6E;		
		iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			memcpy(tmpbuff,(char*)buffTAG,szTAG);
			LOGAPI_HEXDUMP_TRACE( "0x9F6E",tmpbuff, szTAG);
			
			memcpy(toke2 + sztoke2,tmpbuff,szTAG);
			sizeCmd = sizeCmd + szTAG;
			sztoke2 += szTAG;
		}
		else
		{
			LOGF_TRACE("0x9F6E is not available");
		}
		

	}
	else if(chTechnology == CTS_KBD)
	{
		LOGF_TRACE("C53 para digitada");	
		vdSetByteValuePP((char*)"STATUS",(char *)"00",2);
		
		//Pan
		memset(pan, 0x00, sizeof(pan));
		szpan = inGetByteValuePP((char *)"PAN",pan);	
		sizeCmd += szpan;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		//Cardholder
		szCardHold = 2;
		memcpy(CardHold,"\xC1\x00",szCardHold);
		sizeCmd+=szCardHold;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,szCardHold);
		
		//Track 2
		sztrk2 = 2;
		memcpy(trk2,"\xC1\x00",sztrk2);
		sizeCmd+=sztrk2;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//Track 1
		sztrk1 = 2;
		memcpy(trk1,"\xC1\x00",sztrk1);
		sizeCmd+=sztrk1;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//CCV/4DBC
	//	szsec_code = 2;
	//	memcpy(sec_code,"\xC1\x00",szsec_code);
	
		szsec_code = inGetByteValuePP((char *)"SEC_CODE",sec_code);	
		sizeCmd += szsec_code;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	

		vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	

		vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);	
		
		//Pos entry mode
		szposEntry = 4;		
		memcpy(posEntry,"\xC1\x02\x30\x31",szposEntry);				
		sizeCmd += szposEntry;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		//Token E1
		sztoke1 = 3;
		memcpy(toke1,"\xE1\x01\x00",sztoke1);
		sizeCmd+=sztoke1;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//Token E2
	//	LOGAPI_HEXDUMP_TRACE("E2",toke2,sizeof(toke2));	
		memset(toke2, 0x00, sizeof(toke2));
		sztoke2 = 3;
		memcpy(toke2,"\xE2\x01\x00",sztoke2);
		sizeCmd+=sztoke2;
	//	LOGAPI_HEXDUMP_TRACE("E2 after clean",toke2,sizeof(toke2));	
		LOGF_TRACE("size cmd [%i]", sizeCmd);

	//	Length[1] = sizeCmd & 0x000000FF;
	//	Length[0] = sizeCmd & 0x0000FF00;
	//	szLength = 2;
	}	

	//tkn_cz(tokCz,&sztokCz);
	//sizeCmd += sztokCz;
	LOGF_TRACE("size cmd [%i]", sizeCmd);
	
	//vdSetByteValuePP((char*)"CIPHERMODE",trk2,sztrk2);	

	//vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	

	//vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	

	//vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);	

	vdSetByteValuePP((char*)"POSENTRY",posEntry,szposEntry);	

	vdSetByteValuePP((char*)"TOKE1",toke1,sztoke1);	

	vdSetByteValuePP((char*)"TOKE2",toke2,sztoke2);	
	
	//vdSetByteValuePP((char*)"TOKCZ",tokCz,sztokCz);	

	if( boDUKPTasm )
	{
		//Obtenemos la longitud de los tokens generados anteriormente

		in_es = inGetByteValuePP((char *)"TOKES",ch_es);	
		sizeCmd += in_es;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		in_ez = inGetByteValuePP((char *)"TOKEZ",ch_ez);	
		sizeCmd += in_ez;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		in_ey = inGetByteValuePP((char *)"TOKEY",ch_ey);	
		sizeCmd += in_ey;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
	}

	//Calculamos la longiutd de todo el campo C53
	memset(Length,0x00,sizeof(Length));
	lenfld = 0;
	lenfld |= sizeCmd;	//length
	Length[1] = lenfld;
	lenfld = 0;
	lenfld |= sizeCmd >> 8;
	Length[0] = lenfld;
	LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",Length,sizeof(Length));	

//	Length[1] = sizeCmd & 0x000000FF;
//	Length[0] = sizeCmd & 0x0000FF00;
	szLength = 2;
	vdSetByteValuePP((char*)"LENGH",Length,szLength);
	
	return iResult;
}

int buildInitTransactionReply53()
{
	int iResult = 0;
	//Variables para armar el campo
	char Length[5] = {0};
	int szLength = 0;
	char lenfld = 0;
	char ciphermode[4] = {0};
	int szciphermode = 0;
	char pan[50] = {0};
	char panAux[50] = {0};
	int szpan = 0; 
	char CardHold[50] = {0};
	int szCardHold = 0; 
	char trk2[50] = {0};
	int sztrk2 = 0; 
	char trk1[50] = {0};
	int sztrk1 = 0; 
	char sec_code[50] = {0};
	int szsec_code = 0; 
	char posEntry[50] = {0};
	int szposEntry = 0; 
	char toke1[1024] = {0};
	int sztoke1 = 0;
	char toke2[1024] = {0};
	int sztoke2 = 0;	
	int sizeCmd = 0;
	//char tokR1[30] = {0};
	//int szTokR1 = 0;
	char tokCz[100] = {0};
	int sztokCz = 0;
	//PAra verificar tipo de CTLS
	int typctls = 0;
	//Variables para recuperar el tama�o de los tokens generados

	char ch_es [100] = {0};	//TOKEN ES
	char ch_ez [150] = {0};	//TOKEN EZ
	char ch_ey [200] = {0};	//TOKEN EY	
	
	int  in_es   = 0;	//SIZE OF TOKEN ES
	int  in_ez   = 0;	//SIZE OF TOKEN EZ
	int  in_ey   = 0;	//SIZE OF TOKEN EY
	
	//ExceptionBIN check
	char panASCII[20];
	int panASCIILength=0;
	int shouldEncrypt=1;
	
	unsigned char ch_val[4] = {0};
	char ch_asc [70] = {0};
	int  in_asc = 0;
	char tmpbuff[50] = {0};
	int sztmp =0;
	char CVV[6] = {0};
	int szCVV2 = 4;
	char dataENCRYPT[32];
	int szdataENCRYPT = 0;
	char tmpbuf[32];
	int inencrypt=0;
	char pchCipher[128]={0};
	char pchAsc[128] = {0};

	char rsptokR1[30] = {0};
	int szrspTokR1 = 0;
	
	//Variables necesarias para EMV CHIP y CTLS CHIP
	unsigned long TAG = 0;
	unsigned char buffTAG[50] = {0};
	unsigned short szTAG = 0;
	int tech = 0;

	char ch_pan [50] = {0};
	int  in_pan = 0;
	char ch_exp[10]={0};
	int  in_exp=0;
	char * pch_equ = NULL;

	char ch_tk2_pan [50] = {0};
	int  in_tk2 = 0;
	
	LOGF_TRACE("--buildInitTransactionReply53--");	

	typctls = Check_typ_CTLS();

	LOGF_TRACE("C53 chTechnology %d", chTechnology);
	
	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		if(chTechnology == CTS_CHIP)
		{
			LOGF_TRACE("C53 Para EMV Chip");
			tech = CTS_CHIP;
		}
		else if(chTechnology == CTS_CTLS)
		{
			LOGF_TRACE("C53 Para CTLS CHIP");
			tech = CTS_CTLS;
		}
		
	
		  
		//Pan
		//memcpy(pan,"\xC1",1);				
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5A;		
		iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			//szpan = szTAG ;
			//pan[1] = szpan - 2;
			//memcpy(&pan[2],buffTAG + 2,szpan - 2);
			LOGAPI_HEXDUMP_TRACE((char *)"*****PAN EMV/CTLS*****",buffTAG,szTAG);
			in_asc = inHexToAsc ((char *)buffTAG + 2, pan + 2, szTAG - 2);
			if(identify_AMEX_card(tech))
			{
				LOGF_TRACE("NUVC ARMAMOS PAN AMEX");
				in_asc = in_asc - 1;
				memcpy(panAux,pan+2,in_asc);
				memset(pan, 0x00,sizeof(pan));
				memcpy(pan,"\xC1",1);				
				pan[1]=in_asc;
				memcpy(pan+2,panAux,in_asc);
				LOGAPI_HEXDUMP_TRACE("Track 2 in ASC",pan,in_asc+2);
				LOGF_TRACE("-- in_asc [%i]--", in_asc);
			}
			else
			{
				memcpy(pan,"\xC1",1);
				pan[1]=in_asc;
				LOGAPI_HEXDUMP_TRACE("Track 2 in ASC",pan,in_asc+2);
				LOGF_TRACE("-- in_asc [%i]--", in_asc);
			}
		}
		else
		{
			LOGF_TRACE("\nPAN is not available");
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x57;
			iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech );
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				LOGAPI_HEXDUMP_TRACE((char *)"Track 2 Equivalent Data",buffTAG,szTAG);	
				
			}
			else
			{
				LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
			
			}
			
			if (iResult == SUCCESS)
			{
				
				in_tk2 = inHexToAsc ((char *)buffTAG + 2, ch_tk2_pan, szTAG - 2);

				pch_equ = (char *)memchr (ch_tk2_pan, 'D', in_tk2);

				if (pch_equ)
				{
					*pch_equ = 0;

					in_tk2 = pch_equ - ch_tk2_pan;
				}

				LOGAPI_HEXDUMP_TRACE("TRACK 2 antes de bo_crp_crd",ch_tk2_pan,in_tk2);
				LOGF_TRACE("--valor de in_tk2  [%i]--",in_tk2);
				memcpy(pan,"\xC1",1);
				pan[1]=in_tk2;
				memcpy(pan + 2 ,ch_tk2_pan , in_tk2);
				in_asc = in_tk2;
				
			}
			else
			{
				LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
				in_asc = 2;
				memcpy(pan,"\xC1\x00",in_asc);
			}
		}	

		//LOGF_TRACE("szpan = %i",szpan);
		//sizeCmd+=szpan;
		sizeCmd+=in_asc+2;
		vdSetByteValuePP((char*)"PAN",pan,in_asc+2);		

		//inHex2Asc(pan+2,panASCII,6);
		memcpy(panASCII,pan+2,6);
		panASCII[7]=0;
		panASCIILength=6;

		//Cardholder name
		memcpy(CardHold,"\xC1",1);		
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5F20;		
		iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			szCardHold = szTAG - 3;			
			memcpy(&CardHold[2],buffTAG + 3,szCardHold);
			CardHold[1] = szCardHold; 
			szCardHold+=2;
			//debug("PAN from EMV = %s",TAG5A);
			dump(CardHold,szCardHold,(char *)"*****CARDHOLDER EMV/CTLS*****");
		}
		else
		{
			LOGF_TRACE("CARDHOLDER is not available");
			szCardHold = 2;
			memcpy(CardHold,"\xC1\x00",szCardHold);
		}
		
		sizeCmd+=szCardHold;
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,szCardHold);	

		if(isMerchantExceptionBIN(panASCII,panASCIILength) || isMerchantExceptionBINRange(panASCII,panASCIILength)){
			shouldEncrypt=0;
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x57;
			iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,tech );
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				LOGAPI_HEXDUMP_TRACE((char *)"Track 2 Equivalent Data",buffTAG,szTAG);	
				char ch_tk2_pan [60] = {0};
				int  in_tk2 = 0;
				char * pch_equ = NULL;
				in_tk2 = inHexToAsc ((char *)buffTAG + 2, ch_tk2_pan, szTAG - 2);
				
				pch_equ = (char *)memchr (ch_tk2_pan, 'D', in_tk2);

				if (pch_equ)
				{
					*pch_equ = '=';
				}

				sztrk2 = 1;
				memcpy(trk2,"\xC1",sztrk2);
				in_tk2 = AdjustTrack2(ch_tk2_pan, in_tk2);
				
				trk2[1]=in_tk2;
				sztrk2++;

				memcpy(trk2+sztrk2,ch_tk2_pan,in_tk2);
				sztrk2+=in_tk2;
				
				sizeCmd+=sztrk2;
				vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	
			}

		}else{
			//Track 2
			sztrk2 = 2;
			memcpy(trk2,"\xC1\x00",sztrk2);
			sizeCmd+=sztrk2;
			vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	
		}

		LOGF_TRACE("CHIP: shouldEncrypt: [%s] [%d]",panASCII, shouldEncrypt);
	
		
		//Track 1
		sztrk1 = 2;
		memcpy(trk1,"\xC1\x00",sztrk1);
		sizeCmd+=sztrk1;
		vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	
		
		//CCV/4DBC
		szsec_code = 2;
		memcpy(sec_code,"\xC1\x00",szsec_code);
		sizeCmd+=szsec_code;
		vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);

		//Pos entry mode
		szposEntry = 4;
		if(chTechnology == CTS_CHIP)
			memcpy(posEntry,"\xC1\x02\x30\x35",szposEntry);
		else if(chTechnology == CTS_CTLS)				
			memcpy(posEntry,"\xC1\x02\x30\x37",szposEntry);		
		sizeCmd += szposEntry;

		//Token E1		
		assembleTokenE1(toke1,&sztoke1, C53);
		sizeCmd+=sztoke1;
		
		//TOKEN E2
		assembleTokenE2(toke2,&sztoke2,C53);
		sizeCmd += sztoke2;			
	}	
	
	else if(/*(chTechnology == CTS_MSR) || */( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) ))
	{	
			
		LOGF_TRACE("C53 CTLS/MSR");

		tr2_ctls(&sztrk2,trk2,&sztrk1,trk1);
	
		pch_equ = (char *)memchr (trk2, '=', sztrk2);

		if (pch_equ)
		{
			in_pan = pch_equ - trk2 - 1;
			
			memcpy (ch_pan, trk2 + 1, in_pan);
			
			LOGAPI_HEXDUMP_TRACE("ACC",ch_pan,in_pan);

			pch_equ += 1;
			
			memcpy (ch_exp, pch_equ, 4);
			in_exp = 4;
			LOGAPI_HEXDUMP_TRACE("EXP",ch_exp,in_exp);
		}

		
		
		//Aqu� armamos el campo del Pan para el campo C53 para una transancci�n de banda magn�tica
		//if (isExceptionBin (ch_pan,in_pan))
		if(isMerchantExceptionBINRange(ch_pan,in_pan))
		{
			LOGF_TRACE("*****FLAG 00*****");
			shouldEncrypt=0;
		}
		
		else
		{
			LOGF_TRACE("*****FLAG 01*****");
			shouldEncrypt=1;
		}
			
		//sizeCmd = sizeCmd + 3;

		/* PAN */		
		memcpy(pan,"\xC1",1);
		pan[1] = in_pan;
		memcpy(&pan[2],ch_pan,in_pan);		
		vdSetByteValuePP((char*)"PAN",pan,in_pan + 2);
		sizeCmd = sizeCmd + in_pan + 2;

		/* CARD HOLDER */
		memcpy(CardHold,"\xC1",1);
		CardHold[1] = CTLSTransRes.T_5F20_Cardholder.crdNameLen;
		memcpy(&CardHold[2],CTLSTransRes.T_5F20_Cardholder.crdName,CTLSTransRes.T_5F20_Cardholder.crdNameLen);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,CTLSTransRes.T_5F20_Cardholder.crdNameLen+2);

		sizeCmd = sizeCmd + CTLSTransRes.T_5F20_Cardholder.crdNameLen + 2;
			
		//**********************************

		if ( ! inGetEnvVar("NOCV2CTLS",NULL) ) //Daee 14/09/2016
		{
			
			iResult=screenSec_Code(CVV,szCVV2,"INGRESE CODIGO DE SEGURIDAD","");

			//if(iResult==FAILURE_CVV)
			if(iResult< UI_ERR_OK)
			{
				return FAILURE_CVV;
			}
			if(shouldEncrypt==1)
			{
				vdSetByteValuePP((char*)"SEC_CODE",(char *)"\xC1\x00",2);
				sizeCmd = sizeCmd + 2;			
			}
			else
			{
				memset(tmpbuf,0x00,sizeof(tmpbuf));
				memcpy(tmpbuf,"\xC1",1);
				tmpbuf[1] = strlen(CVV);
				memcpy(tmpbuf + 2,CVV,strlen(CVV));
				vdSetByteValuePP((char*)"SEC_CODE",tmpbuf,strlen(CVV) + 2);
				sizeCmd = sizeCmd + strlen(CVV) + 2;				

			}
		
			
		}
		else
		{
			vdSetByteValuePP((char*)"SEC_CODE",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}
			
		if (sztrk2>0)
		{

			if(shouldEncrypt==1)
			{
				vdSetByteValuePP((char*)"TRACK2",(char *)"\xC1\x00",2);
				sizeCmd = sizeCmd + 2;
			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				sztrk2 = CleanTrack2(trk2,sztrk2);
				LOGF_TRACE("TRACK2 %s - Long %d",trk2, sztrk2);
				pchAsc[1] = sztrk2;
				memcpy(&pchAsc[2], trk2,sztrk2);
				vdSetByteValuePP((char*)"TRACK2",pchAsc,sztrk2+2);
				sizeCmd = sizeCmd+sztrk2+ 2;		

			}
			
		}
		else
		{
			vdSetByteValuePP((char*)"TRACK2",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}

		if (sztrk1>0)
		{
			if(shouldEncrypt==1)
			{
				vdSetByteValuePP((char*)"TRACK1",(char *)"\xC1\x00",2);
				sizeCmd = sizeCmd + 2;

			}

			else
			{
				memset(pchAsc,0,sizeof(pchAsc));
				memcpy(pchAsc,"\xC1",1);
				sztrk1 = CleanTrack1(trk1,sztrk1);
				LOGF_TRACE("TRACK1 %s - Long %d",trk1, sztrk1);
				pchAsc[1] = sztrk1;
				memcpy(&pchAsc[2], trk1,sztrk1);
				vdSetByteValuePP((char*)"TRACK1",pchAsc,sztrk1+2);
				sizeCmd = sizeCmd+sztrk1+ 2;		

			}
		
			
		}
		else
		{
			vdSetByteValuePP((char*)"TRACK1",(char *)"\xC1\x00",2);
			sizeCmd = sizeCmd + 2;
		}
		
		
		//PEM
		LOGF_TRACE("banda CTLS");
		szposEntry = 4;
		memcpy(posEntry,"\xC1\x02\x39\x31",szposEntry);
		
		sizeCmd = sizeCmd + 4;
			
			
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);

		//Token E1		
		sztoke1 = 3;
		memcpy(toke1,(char*)"\xE1\x01\x00",sztoke1);
		sizeCmd = sizeCmd + 3;
		
		//TOKEN E2
		sztoke2 = 3;
		memcpy(toke2,(char*)"\xE2\x01\x00",sztoke2);
		sizeCmd = sizeCmd + 3;

		//CTLS BANDA ASM 9F6E
		memset(tmpbuff,0,sizeof tmpbuff);		
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x9F6E;		
		iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			memcpy(tmpbuff,(char*)buffTAG,szTAG);
			LOGAPI_HEXDUMP_TRACE( "0x9F6E",tmpbuff, szTAG);
			
			memcpy(toke2 + sztoke2,tmpbuff,szTAG);
			sizeCmd = sizeCmd + szTAG;
			sztoke2 += szTAG;
		}
		else
		{
			LOGF_TRACE("0x9F6E is not available");
		}
		

	}
	else if(chTechnology == CTS_KBD)
	{
		LOGF_TRACE("C53 para digitada");	
		vdSetByteValuePP((char*)"STATUS",(char *)"00",2);
		
		//Pan
		memset(pan, 0x00, sizeof(pan));
		szpan = inGetByteValuePP((char *)"PAN",pan);	
		sizeCmd += szpan;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		//Cardholder
		szCardHold = 2;
		memcpy(CardHold,"\xC1\x00",szCardHold);
		sizeCmd+=szCardHold;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHold,szCardHold);
		
		//Track 2
		sztrk2 = 2;
		memcpy(trk2,"\xC1\x00",sztrk2);
		sizeCmd+=sztrk2;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//Track 1
		sztrk1 = 2;
		memcpy(trk1,"\xC1\x00",sztrk1);
		sizeCmd+=sztrk1;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//CCV/4DBC
		//	szsec_code = 2;
		//	memcpy(sec_code,"\xC1\x00",szsec_code);
	
		szsec_code = inGetByteValuePP((char *)"SEC_CODE",sec_code);	
		sizeCmd += szsec_code;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	

		vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	

		vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);	
		
		//Pos entry mode
		szposEntry = 4;		
		memcpy(posEntry,"\xC1\x02\x30\x31",szposEntry);				
		sizeCmd += szposEntry;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		//Token E1
		sztoke1 = 3;
		memcpy(toke1,"\xE1\x01\x00",sztoke1);
		sizeCmd+=sztoke1;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
		
		//Token E2
		//	LOGAPI_HEXDUMP_TRACE("E2",toke2,sizeof(toke2));	
		memset(toke2, 0x00, sizeof(toke2));
		sztoke2 = 3;
		memcpy(toke2,"\xE2\x01\x00",sztoke2);
		sizeCmd+=sztoke2;
		//	LOGAPI_HEXDUMP_TRACE("E2 after clean",toke2,sizeof(toke2));	
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		//	Length[1] = sizeCmd & 0x000000FF;
		//	Length[0] = sizeCmd & 0x0000FF00;
		//	szLength = 2;
	}	



	//tokR1(rsptokR1, &szrspTokR1);
	//sizeCmd += szrspTokR1;
	//LOGF_TRACE("size cmd [%i]", sizeCmd);
	
	
	memset(tokCz,0,sizeof(tokCz));
	tkn_cz(tokCz,&sztokCz);
	sizeCmd += sztokCz;
	LOGF_TRACE("size cmd [%i]", sizeCmd);
	//vdSetByteValuePP((char*)"TOKCZ",tokCz,sztokCz);
	
	//tkn_cz(tokCz,&sztokCz);
	//sizeCmd += sztokCz;
	LOGF_TRACE("size cmd [%i]", sizeCmd);
	
	//vdSetByteValuePP((char*)"CIPHERMODE",trk2,sztrk2);	

	//vdSetByteValuePP((char*)"TRACK2",trk2,sztrk2);	

	//vdSetByteValuePP((char*)"TRACK1",trk1,sztrk1);	

	//vdSetByteValuePP((char*)"SEC_CODE",sec_code,szsec_code);	

	vdSetByteValuePP((char*)"POSENTRY",posEntry,szposEntry);	

	vdSetByteValuePP((char*)"TOKE1",toke1,sztoke1);	

	vdSetByteValuePP((char*)"TOKE2",toke2,sztoke2);	
	
	vdSetByteValuePP((char*)"TOKCZ",tokCz,sztokCz);	
	
	//vdSetByteValuePP((char*)"TOKCZ",tokCz,sztokCz);

	//vdSetByteValuePP((char*)"TOKR1",rsptokR1,szrspTokR1);

	if( boDUKPTasm && shouldEncrypt==1)
	{
		//Obtenemos la longitud de los tokens generados anteriormente

		in_es = inGetByteValuePP((char *)"TOKES",ch_es);	
		sizeCmd += in_es;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		in_ez = inGetByteValuePP((char *)"TOKEZ",ch_ez);	
		sizeCmd += in_ez;
		LOGF_TRACE("size cmd [%i]", sizeCmd);

		in_ey = inGetByteValuePP((char *)"TOKEY",ch_ey);	
		sizeCmd += in_ey;
		LOGF_TRACE("size cmd [%i]", sizeCmd);
	}else{
		
		memset(ch_es,0,sizeof(ch_es));
		vdSetByteValuePP((char*)"TOKEY",ch_es,in_es);
		vdSetByteValuePP((char*)"TOKEZ",ch_es,in_es);
		in_es=inAsmQs(ch_es,FALSE);
		vdSetByteValuePP((char*)"TOKES",ch_es,in_es);
		sizeCmd+=in_es;
	}

	//Calculamos la longiutd de todo el campo C53
	memset(Length,0x00,sizeof(Length));
	lenfld = 0;
	lenfld |= sizeCmd;	//length
	Length[1] = lenfld;
	lenfld = 0;
	lenfld |= sizeCmd >> 8;
	Length[0] = lenfld;
	LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",Length,sizeof(Length));	

	//	Length[1] = sizeCmd & 0x000000FF;
	//	Length[0] = sizeCmd & 0x0000FF00;
	szLength = 2;
	vdSetByteValuePP((char*)"LENGH",Length,szLength);
	
	return iResult;
}

void tkn_cz(char* TOKEN_out,int* szTOKEN_out)
{
	int idx = 0;
	int inRetval = 0;
	unsigned char bitmap[7 + 1] = {0};
	unsigned short tagLen = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100] = {0};
	char Tg9F6E[50] = {0};
	int szTag9F6E = 0;
	LOGF_TRACE("Token CZ");

	memcpy(TOKEN_out,"!",1);
	idx += 1;
	memcpy(TOKEN_out+idx," ",1);
	idx += 1;
	memcpy(TOKEN_out+idx,"CZ",2);
	idx += 2;
	//memcpy(TOKEN_out + idx,"40",2);  // FAG 26-oct-2017  falla la longitud del token CZ pruebas en bancomer
	memcpy(TOKEN_out + idx,"00040",5);  // FAG 26-oct-2017
	idx += 5;
	memcpy(TOKEN_out + idx," ",1);  // FAG 26-oct-2017
	idx += 1;

	LOGAPI_HEXDUMP_TRACE("&TOKEN_out[idx]",TOKEN_out,idx+4);

	memset(buffTAG,0,sizeof(buffTAG));
	//if((chTechnology == CTS_KBD) || (chTechnology == CTS_MSR) || (chTechnology == CTS_CTLS && Check_typ_CTLS() == CTLS_TXN_MSR))
	if((chTechnology == CTS_KBD) || (chTechnology == CTS_MSR))
	{
		//memset(TOKEN_out + idx,0x20,4);
		//memset(buffTAG + idx,0x20,4);
		memset(buffTAG,0x20,4);
		LOGF_TRACE("***ATC NO CHIP***");
	}
	else
	{
		//TAG_9F36_ATC
		TAG = 0;
		TAG = 0x9F36;
		//inRetval = GetTag(&TAG,(unsigned char*)&TOKEN_out[idx], &tagLen, 4, 0x02, &bitmap[0], '0',chTechnology);
		inRetval = GetTag(&TAG,(unsigned char*)buffTAG, &tagLen, 4, 0x02, &bitmap[0], '0',chTechnology);

		if(buffTAG[0] == 0x00 && buffTAG[1] == 0x00 && buffTAG[2] == 0x00&& buffTAG[3] == 0x00)
		{
			LOGF_TRACE("NO ATC DATA");
			memset(buffTAG,0x20,4);
		}

	}
	memcpy(&TOKEN_out[idx],buffTAG,4);
	idx+=4;

	LOGAPI_HEXDUMP_TRACE("&TOKEN_out[idx]",TOKEN_out,idx+4);
	

	if(chTechnology == CTS_CTLS)
	{
		//Para obtener el valor de los siguientes campos se verifica la aplicación para saber si es MC o VISA
		//TAG 47	
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x4F;
		inRetval = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CTLS);
		if( (inRetval == EMV_ADK_OK) && (szTAG > 0) )
			if(szTAG > 2)
				LOGAPI_HEXDUMP_TRACE((char *)"AID",buffTAG,szTAG);
			else
			{
				memset(buffTAG,0x00,sizeof(buffTAG));
				if(CTLSTransRes.T_9F06_AID.aidlen > 0)
				{				
					memcpy(buffTAG,CTLSTransRes.T_9F06_AID.AID,CTLSTransRes.T_9F06_AID.aidlen);
				}	
				else
				{
					LOGF_TRACE("\nAID is not available");								
				} 	
			}

		if(!memcmp(buffTAG,"\xA0\x00\x00\x00\x04",5))
		{	
			LOGF_TRACE("CZ PARA MASTER CARD");
			memset(buffTAG,0x00,sizeof(buffTAG));
			LOGAPI_HEXDUMP_TRACE("&TOKEN_out[idx]",TOKEN_out,idx+4);
			/*if( AddTag9F6E((char*)buffTAG) < 9 )
			{
				memset(&TOKEN_out[idx], '\x20', 8);
			}
			else*/
			if( AddTag9F6E((char*)buffTAG) > 0 )
			{

				LOGAPI_HEXDUMP_TRACE("&TOKEN_out[idx]",TOKEN_out,idx+4);
				//FFI MC
				vdGetFFIMC((char*)&buffTAG[3],&TOKEN_out[idx]);
				//vdHex2Asc((const char*)&buffTAG[7], &TOKEN_out[idx], 2);
				//memset(&TOKEN_out[idx + 4], '\x30', 4);
			}
			else
				memset(&TOKEN_out[idx], '\x20', 8);


			LOGAPI_HEXDUMP_TRACE("TAG 9F6E", &TOKEN_out[idx], 8 );
			idx+=8;

			//USR-FLD-ASI  NO APLICA PARA VISA
			memset(TOKEN_out + idx, 0x20, 28);
			idx += 28;
		}	
		else if((!memcmp(buffTAG,"\xA0\x00\x00\x00\x03",5)))
		{
			//FFI  Para VISA
			LOGF_TRACE("CZ PARA VISA");
			memset(buffTAG,0x00,sizeof(buffTAG));
			if( AddTag9F6E((char*)buffTAG) != 7 )
			{
				memset(&TOKEN_out[idx], '\x20', 8);
			}
			else
			{
				vdHex2Asc((const char*)&buffTAG[3], &TOKEN_out[idx], 4);
			}

			LOGAPI_HEXDUMP_TRACE("TAG 9F6E", &TOKEN_out[idx], 8 );
			idx+=8;

			//USR-FLD-ASI  NO APLICA PARA VISA
			memset(TOKEN_out + idx, 0x20, 28);
			idx += 28;
		}						   
		else if((!memcmp(buffTAG, "\xA0\x00\x00\x00\x25", 5)))
		{
			//FFI Para AMEX
			LOGF_TRACE("CZ PARA AMEX");
			memset(buffTAG,0x00,sizeof(buffTAG));
			if( AddTag9F6E((char*)buffTAG) != 7 )
			{
				memset(&TOKEN_out[idx], '\x20', 8);
			}
			else
			{
				vdHex2Asc((const char*)&buffTAG[3], &TOKEN_out[idx], 4);
			}

			LOGAPI_HEXDUMP_TRACE("TAG 9F6E", &TOKEN_out[idx], 8 );
			idx+=8;

			//USR-FLD-ASI  NO APLICA PARA AMEX
			memset(TOKEN_out + idx, 0x20, 28);
			idx += 28;
		}
		else
		{

			LOGF_TRACE("NO VISA NI MASTER CARD");
			//FFI 
			//memset(TOKEN_out + idx,0x30,8); // FAG 26-OCT-2017
			memset(TOKEN_out + idx,0x20,8);
			idx += 8;
			
			//USR-FLD-ASI
			//memset(TOKEN_out + idx,0x30,28);   // FAG 26-OCT-2017
			memset(TOKEN_out + idx,0x20,28);
			idx += 28;

		}
	}
	else  //chip, banda, digitada, CTLS MSR
	{	
			LOGF_TRACE("NO CTLS");
			//FFI 
			//memset(TOKEN_out + idx,0x30,8); // FAG 26-oct-2017
			memset(TOKEN_out + idx,0x20,8); 
			idx += 8;
			
			//USR-FLD-ASI
			//memset(TOKEN_out + idx,0x30,28);  // FAG 26-oct-2017
			memset(TOKEN_out + idx,0x20,28);
			idx += 28;
	}
	
	*szTOKEN_out = idx;
}

void tokR1(char* tokR1, int* szTkR1)
{
	int idx = 0;
	LOGF_TRACE("--Token R1--");
	
	memcpy(tokR1,"!",1);
	idx += 1;

	memcpy(tokR1+idx," ",1);
	idx += 1;

	memcpy(tokR1+idx,"R1",2);
	idx += 2;

	//memcpy(tokR1,"00016",5);
	//idx += 5;
	memcpy(tokR1+idx,"00000",5);
	idx += 5;

	memcpy(tokR1+idx," ",1);
	idx += 1;

	//memcpy(tokR1,"5590897678967654",16);
	//idx += 16;

	*szTkR1 = idx;	
}

void asm_tke1(char* tknE1,int* szTknE1, int type_cmd)
{
	unsigned long TAG = 0;
	unsigned short szTAG = 0;

	char ch_cvmend [10]= {0}; 					
	int  in_cvmend = 0;
	
	unsigned char buffTAG[50] = {0};
	//unsigned char tknE1[512] = {0};	
	int iResult = 0;
	int idx = 0;

	int iSigRequired = 0;

	*szTknE1=0;	
	memset(tknE1,0x00,sizeof(tknE1));


	if(type_cmd==C53)
	{
		idx=3;
	}

	//TAG 47	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		if(szTAG > 2)
			LOGAPI_HEXDUMP_TRACE((char *)"AID",buffTAG,szTAG);
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
			{
				memcpy(tknE1 + idx,"\x4F",1);
				idx++;
				tknE1[idx++] = pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				memcpy(tknE1 + idx,pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				idx += pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				LOGAPI_HEXDUMP_TRACE((char *)"AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
			}
			else if(CTLSTransRes.T_9F06_AID.aidlen > 0)
			{
				memcpy(tknE1 + idx,"\x4F",1);
				idx++;
				tknE1[idx++] = CTLSTransRes.T_9F06_AID.aidlen;
				memcpy(tknE1 + idx,CTLSTransRes.T_9F06_AID.AID,CTLSTransRes.T_9F06_AID.aidlen);
				idx += CTLSTransRes.T_9F06_AID.aidlen;
				LOGAPI_HEXDUMP_TRACE((char *)"AID",CTLSTransRes.T_9F06_AID.AID,CTLSTransRes.T_9F06_AID.aidlen);

			}	
			else
			{
				LOGF_TRACE("\nAID is not available");				
				memcpy(tknE1 + idx,"\x4F\x00",2);
				idx += 2;
			} 	
		}
		else
		{
				LOGF_TRACE("\nAID is not available");				
				memcpy(tknE1 + idx,"\x4F\x00",2);
				idx += 2;
		} 	

//TAG 9F12

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F12;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Application Preferred Name",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nApplication Preferred Name is not available");
	
	//TAG 50

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x50;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Application Level",buffTAG,szTAG);
		//printf("%s\n",buffTAG+2);
	}
	else
		LOGF_TRACE("\nApplication Level is not available");


	//TAG 5F30 
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));		
	TAG = 0x5F30;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Service Code",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("Service Code is not available");


	//TAG 5F20
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F20;		
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"CARDHOLDER",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("CARDHOLDER is not available");

	
	//TAG 57
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x57;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE((char *)"\nTrack 2 Equivalent Data",buffTAG,szTAG);	
				
	}
	else
	{
		LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
	}
	if (bo_crp_chp())
	{
		int inRetval;
		char ch57crypt[50]={0};
		int  in57crypt=0;
		ushort ustk2=0;
			
		char ch_asc [70] = {0};
		int  in_asc = 0;
		in_asc = inHexToAsc ((char *)buffTAG + 2, ch_asc, szTAG - 2);
		LOGAPI_HEXDUMP_TRACE("Track 2 in ASC",ch_asc,sizeof(ch_asc));
		LOGF_TRACE("-- in_asc [%i]--", in_asc);
		if (in_Cx_EDta(ch_asc,in_asc,ch57crypt,&in57crypt) < VS_SUCCESS)
		{
			LOGF_TRACE("--falla al momento de encriptar--");
//			return RET_ECL_E_CIF_CX;
		}
		memcpy(tknE1 + idx,"\x57",1);
		idx+=1;
		tknE1[idx]=in57crypt;
		idx+=1;
		memcpy(tknE1 + idx,ch57crypt,in57crypt);
		idx+=in57crypt;
					
	}
	
	else
	{
		memcpy(tknE1 + idx,"\x57",1);
		idx+=1;
		tknE1[idx]=szTAG;
		idx+=1;
		memcpy(tknE1 + idx, buffTAG,szTAG);
		idx+=szTAG;							
	}

	//TAG 5A
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5A;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		
		memcpy(tknE1 + idx, buffTAG,szTAG);
		idx+=szTAG;							
		LOGAPI_HEXDUMP_TRACE((char *)" PAN ",buffTAG,szTAG);	
	}
	else
	LOGF_TRACE("\nPAN is not available");
			
	
	// TAG 5F24
	szTAG = 0;
	TAG = 0;
	TAG = 0x5F24;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx, buffTAG,szTAG);
		idx+=szTAG;							
		LOGAPI_HEXDUMP_TRACE((char *)"EMV_AEP",buffTAG,szTAG);	
	}
	else
		LOGF_TRACE("\nfail TAG 5F24");
		

	

	//TAG 5F34

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG)); //Cambiar
	TAG = 0x5F34;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Aplicaci�n N�mero de cuenta principal (PAN) N�mero de Secuencia",buffTAG,szTAG); 
	}
	else
	LOGF_TRACE("\nAplicaci�n N�mero de cuenta principal (PAN) N�mero de Secuencia");


	//AUTH_STATUS
	memcpy(tknE1 + idx,"\xC2\x01\x00",3);
	idx+=3;




//TAG 9F34  para validar si fue por pin offline

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F34;	
	iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		
			memcpy(tknE1 + idx,buffTAG,szTAG);
			idx+=szTAG;
			LOGAPI_HEXDUMP_TRACE((char *)"CVMethod Result",buffTAG,szTAG); 
		
	}
	else
	{
		if (Check_typ_CTLS() == CTLS_TXN_CHIP && TAG==0x9F34)
		{
			memset(buffTAG,0x00,sizeof(buffTAG));
			if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
			{
				//JRS, 13022019 Para cumplir con Eglobal BBVA solicita que modifiquemos CVMR dependiento del valor de CTQ
				//Si el signature flag regresa un valor 02 en el CVMR se espera que se informe el tag 9F34 = 1F 00 00
				//Si el signature flag regresa un valor 01 en el CVMR se espera que se informe el tag 9F34 = 1E 00 00
				//Si el signature Flag regresa un valor 03 en el CVMR se espera que se informe el tag 9F34= 41 00 00

				szTAG = 0;
				TAG = 0;
				memset(buffTAG,0x00,sizeof(buffTAG));
				TAG = 0x9F6C;
				iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
				iSigRequired = inCTQvalue((char*)&buffTAG[3]);
				memset(buffTAG,0x00,sizeof(buffTAG));
				LOGF_TRACE("isSigRequired [%d]", iSigRequired);
				if(iSigRequired == 1)
					memcpy(buffTAG,"\x9F\x34\x03\x1E\x00\x00",6);
				else if(iSigRequired == 2)
					memcpy(buffTAG,"\x9F\x34\x03\x1F\x00\x00",6);
				else if(iSigRequired == 3)
					memcpy(buffTAG,"\x9F\x34\x03\x41\x00\x00",6);
				else
					memcpy(buffTAG,"\x9F\x34\x03\x00\x00\x00",6);
				szTAG = 6;
			}
			else
			{
				//ESTO NO ES FUNCIONAL
				memcpy(buffTAG,"\x9F\x34\x03",3);
				szTAG=3;
			    inGet9F34Ctls(buffTAG+szTAG);
			    szTAG+=3;	
			}
			
		    
		    memcpy(tknE1 + idx,buffTAG,szTAG);
			idx+=szTAG;
			LOGAPI_HEXDUMP_TRACE((char *)"CVMethod Result",buffTAG,szTAG);
		    
		}			
		else
			LOGF_TRACE("\nCardholder Verification Method Result is not available");
	}


	//C�digo para tag C2 (signature flag)

	memcpy (tknE1 + idx, "\xC2\x01",2); 	
	idx+= 2;   

	memcpy(ch_cvmend,buffTAG+3,3);
	in_cvmend = (unsigned char)ch_cvmend[0];
	in_cvmend &= 0x3F;

	if ((in_cvmend < 1) || (in_cvmend > 5)) 		   
	{		  
		LOGF_TRACE("NO PIN!!  \n\n");		  
		iResult = Apl_ErC_ERR;
	}			 
	else if (ch_cvmend [2] != 2) 
	{
		LOGF_TRACE("Unsuccesfull  \n\n");
		iResult = Apl_ErC_ERR;
	}
	else 
		iResult = Apl_ErC_ESC;

	if (iResult > Apl_ErC_ERR)
	{
		LOGF_TRACE("Fue por pin \n\n");
		//memcpy (tknE1 + idx, "\x02", 1);	
		memcpy (tknE1 + idx, "\x01", 1);	
		idx++;
	}
	else
	{	  
		LOGF_TRACE("Signature or Error Pin \n\n"); 
		//memcpy (tknE1 + idx, "\x01", 1);	
		memcpy (tknE1 + idx, "\x00", 1);	
		idx++;
	}



//TAG 95

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Terminal Verification Result",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nTerminal Verification Result is not available");


//TAG 9B
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Transaction Status Information",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nTransaction Status Infortation is not available");



//TAG 9F27

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Cryptogram information Data",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nCryptogram information Data is not available");



//TAG 8A

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Authorization Response Code",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nAuthorization Response Code is not available");



	if(type_cmd==C53)
	{	
		//TAG 9F34 

			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x9F34;	
			iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				
					memcpy(tknE1 + idx,buffTAG,szTAG);
					idx+=szTAG;
					LOGAPI_HEXDUMP_TRACE((char *)"CVMethod Result",buffTAG,szTAG); 
				
			}
			else
			{
				if (Check_typ_CTLS() == CTLS_TXN_CHIP && TAG==0x9F34)
				{
					memset(buffTAG,0x00,sizeof(buffTAG));
					if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
					{
						//JRS, 13022019 Para cumplir con Eglobal BBVA solicita que modifiquemos CVMR dependiento del valor de CTQ
						//Si el signature flag regresa un valor 02 en el CVMR se espera que se informe el tag 9F34 = 1F 00 00
						//Si el signature flag regresa un valor 01 en el CVMR se espera que se informe el tag 9F34 = 1E 00 00
						//Si el signature Flag regresa un valor 03 en el CVMR se espera que se informe el tag 9F34= 41 00 00

						szTAG = 0;
						TAG = 0;
						memset(buffTAG,0x00,sizeof(buffTAG));
						TAG = 0x9F6C;
						iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
						iSigRequired = inCTQvalue((char*)&buffTAG[3]);
						memset(buffTAG,0x00,sizeof(buffTAG));
						LOGF_TRACE("isSigRequired [%d]", iSigRequired);
						if(iSigRequired == 1)
							memcpy(buffTAG,"\x9F\x34\x03\x1E\x00\x00",6);
						else if(iSigRequired == 2)
							memcpy(buffTAG,"\x9F\x34\x03\x1F\x00\x00",6);
						else if(iSigRequired == 3)
							memcpy(buffTAG,"\x9F\x34\x03\x41\x00\x00",6);
						else
							memcpy(buffTAG,"\x9F\x34\x03\x00\x00\x00",6);
						szTAG = 6;
					}
					else
					{
						//ESTO NO ES FUNCIONAL
						memcpy(buffTAG,"\x9F\x34\x03",3);
						szTAG=3;
					    inGet9F34Ctls(buffTAG+szTAG);
					    szTAG+=3;	
					}
					
				    
				    memcpy(tknE1 + idx,buffTAG,szTAG);
					idx+=szTAG;
					LOGAPI_HEXDUMP_TRACE((char *)"CVMethod Result",buffTAG,szTAG);
				    
				}			
				else
					LOGF_TRACE("\nCardholder Verification Method Result is not available");
			}



		//TAG 9F26	
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x9F26;
			iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				memcpy(tknE1 + idx,buffTAG,szTAG);
				idx+=szTAG;
				LOGAPI_HEXDUMP_TRACE((char *)"Application Cryptogram",buffTAG,szTAG);
			}
			else
				LOGF_TRACE("\nApplication Cryptogram Data is not available");



		//TAG 9F39

			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x9F39;
			iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				memcpy(tknE1 + idx,buffTAG,szTAG);
				idx+=szTAG;
				LOGAPI_HEXDUMP_TRACE((char *)"Point-of-Service (POS) Entry Mode",buffTAG,szTAG);	
			}
			else
				LOGF_TRACE("\nPoint-of-Service (POS) Entry Mode is not available");
				

		//TAG 99

			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = TAG_99_TRANS_PIN_DATA;
			iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				memcpy(tknE1 + idx,buffTAG,szTAG);
				idx+=szTAG;
				LOGAPI_HEXDUMP_TRACE((char *)"Transaction Personal Identification Number (PIN) Data",buffTAG,szTAG);
				LOGF_TRACE("size Tag 99 = %i",szTAG);
			}
			else
				LOGF_TRACE("\nTransaction Personal Identification Number (PIN) Data is not available");


			idx += AddTag9F6E(tknE1 + idx,1);

		//TAG 9F6E
		/*
				szTAG = 0;
				TAG = 0;
				memset(buffTAG,0x00,sizeof(buffTAG));
				TAG = 0x9F6E;
				iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
				if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
				{
					memcpy(tknE1 + idx,buffTAG,szTAG);
					idx+=szTAG;
					LOGAPI_HEXDUMP_TRACE((char *)"Form Factor Indicator",buffTAG,szTAG);
					LOGF_TRACE("size of 9F6E = %i",szTAG);
				}
				else
					LOGF_TRACE("\nForm Factor Indicator is not available");	
		*/
		
	}

	LOGF_TRACE("tama�o del token E1 %i",idx);

	if(type_cmd==C53)
	{
		//Token E1
		/*
		tknE1[0] = 0xE1;
		tknE1[1] = idx; 	
		*/
		tknE1[0] = 0xE1;
		tknE1[1] = 0x01; 	
		tknE1[2] = 0x00;
	}
	
	*szTknE1 = idx;

}

void assembleTokenE1(char* tknE1,int* szTknE1, int type_cmd)
{
	unsigned long TAG = 0;
	unsigned short szTAG = 0;

	char ch_cvmend [10]= {0}; 					
	int  in_cvmend = 0;
	
	unsigned char buffTAG[50] = {0};
	//unsigned char tknE1[512] = {0};	
	int iResult = 0;
	int idx = 0;

	int iSigRequired = 0;

	*szTknE1=0;	
	memset(tknE1,0x00,sizeof(tknE1));
	LOGF_TRACE("assembleTokenE1");

	if(type_cmd==C53)
	{
		idx=2;
	}

	//TAG 4F	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,chTechnology);
	LOGF_TRACE("\nTAG AID 4F Len= %i", szTAG);	// PGC  19SEPT25
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		if(szTAG > 2)
		{
			LOGAPI_HEXDUMP_TRACE((char *)"AID",buffTAG,szTAG);  // PGC  19SEPT25
			memcpy(tknE1 + idx,buffTAG,szTAG);  // PGC  19SEPT25
     		idx+=szTAG;                         // PGC  19SEPT25
		}
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
			{
				memcpy(tknE1 + idx,"\x4F",1);
				idx++;
				tknE1[idx++] = pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				memcpy(tknE1 + idx,pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				idx += pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				LOGAPI_HEXDUMP_TRACE((char *)"AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
			}
			else if(CTLSTransRes.T_9F06_AID.aidlen > 0)
			{
				memcpy(tknE1 + idx,"\x4F",1);
				idx++;
				tknE1[idx++] = CTLSTransRes.T_9F06_AID.aidlen;
				memcpy(tknE1 + idx,CTLSTransRes.T_9F06_AID.AID,CTLSTransRes.T_9F06_AID.aidlen);
				idx += CTLSTransRes.T_9F06_AID.aidlen;
				LOGAPI_HEXDUMP_TRACE((char *)"AID",CTLSTransRes.T_9F06_AID.AID,CTLSTransRes.T_9F06_AID.aidlen);

			}	
			else
			{
				LOGF_TRACE("\nAID is not available");				
				memcpy(tknE1 + idx,"\x4F\x00",2);
				idx += 2;
			} 	
		} 	
    }
	else
	{
				LOGF_TRACE("\nAID is not available");				
				memcpy(tknE1 + idx,"\x4F\x00",2);
				idx += 2;
	}

	//TAG 9F12
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F12;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Application Preferred Name",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nApplication Preferred Name is not available");
	
	//TAG 50

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x50;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Application Level",buffTAG,szTAG);
		//printf("%s\n",buffTAG+2);
	}
	else
		LOGF_TRACE("\nApplication Level is not available");


	//TAG 5F30 
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));		
	TAG = 0x5F30;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Service Code",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("Service Code is not available");

	//TAG 5F34

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG)); //Cambiar
	TAG = 0x5F34;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Aplicaci�n N�mero de cuenta principal (PAN) N�mero de Secuencia",buffTAG,szTAG); 
	}
	else
	LOGF_TRACE("\nAplicaci�n N�mero de cuenta principal (PAN) N�mero de Secuencia");

	// TAG 5F24
	/*szTAG = 0;
	TAG = 0;
	TAG = 0x5F24;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx, buffTAG,szTAG);
		idx+=szTAG;							
		LOGAPI_HEXDUMP_TRACE((char *)"EMV_AEP",buffTAG,szTAG);	
	}
	else
		LOGF_TRACE("\nfail TAG 5F24");
	*/	
	//TAG 9F34  para validar si fue por pin offline
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F34;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 3) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Cardholder Verification Method Result",buffTAG,szTAG); 
	}
	else{

		if (Check_typ_CTLS() == CTLS_TXN_CHIP && TAG==0x9F34)
		{
			memset(buffTAG,0x00,sizeof(buffTAG));
			if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
			{
				//JRS, 13022019 Para cumplir con Eglobal BBVA solicita que modifiquemos CVMR dependiento del valor de CTQ
				//Si el signature flag regresa un valor 02 en el CVMR se espera que se informe el tag 9F34 = 1F 00 00
				//Si el signature flag regresa un valor 01 en el CVMR se espera que se informe el tag 9F34 = 1E 00 00
				//Si el signature Flag regresa un valor 03 en el CVMR se espera que se informe el tag 9F34= 41 00 00

				szTAG = 0;
				TAG = 0;
				memset(buffTAG,0x00,sizeof(buffTAG));
				TAG = 0x9F6C;
				iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
				iSigRequired = inCTQvalue((char*)&buffTAG[3]);
				memset(buffTAG,0x00,sizeof(buffTAG));
				LOGF_TRACE("isSigRequired [%d]", iSigRequired);
				if(iSigRequired == 1)
					memcpy(buffTAG,"\x9F\x34\x03\x1E\x00\x00",6);
				else if(iSigRequired == 2)
					memcpy(buffTAG,"\x9F\x34\x03\x1F\x00\x00",6);
				else if(iSigRequired == 3)
					memcpy(buffTAG,"\x9F\x34\x03\x41\x00\x00",6);
				else
					memcpy(buffTAG,"\x9F\x34\x03\x00\x00\x00",6);
				szTAG = 6;
			}
			else
			{
				//ESTO NO ES FUNCIONAL
				memcpy(buffTAG,"\x9F\x34\x03",3);
				szTAG=3;
			    inGet9F34Ctls(buffTAG+szTAG);
			    szTAG+=3;	
			}
			
		    
		    memcpy(tknE1 + idx,buffTAG,szTAG);
			idx+=szTAG;
			LOGAPI_HEXDUMP_TRACE((char *)"CVMethod Result",buffTAG,szTAG);
		    
		}			
		else
			LOGF_TRACE("\nCardholder Verification Method Result is not available");
	}

	

	//C�digo para tag C2 (signature flag)

	memcpy (tknE1 + idx, "\xC2\x01",2); 	
	idx+= 2;   

	memcpy(ch_cvmend,buffTAG+3,3);
	in_cvmend = (unsigned char)ch_cvmend[0];
	in_cvmend &= 0x3F;

	if ((in_cvmend < 1) || (in_cvmend > 5)) 		   
	{		  
		LOGF_TRACE("NO PIN!!  \n\n");		  
		iResult = Apl_ErC_ERR;
	}			 
	else if (ch_cvmend [2] != 2) 
	{
		LOGF_TRACE("Unsuccesfull  \n\n");
		iResult = Apl_ErC_ERR;
	}
	else 
		iResult = Apl_ErC_ESC;

	if (iResult > Apl_ErC_ERR)
	{
		LOGF_TRACE("Fue por pin \n\n");
		//memcpy (tknE1 + idx, "\x02", 1);	
		memcpy (tknE1 + idx, "\x00", 1);	
		idx++;
	}
	else
	{	  
		LOGF_TRACE("Signature or Error Pin \n\n"); 
		memcpy (tknE1 + idx, "\x01", 1);	
		//memcpy (tknE1 + idx, "\x00", 1);	
		idx++;
	}



	//TAG 95
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Terminal Verification Result",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nTerminal Verification Result is not available");



	//TAG 9F27
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Cryptogram information Data",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nCryptogram information Data is not available");

	//TAG 9F26	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F26;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Application Cryptogram",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nApplication Cryptogram Data is not available");


	//TAG 9B
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Transaction Status Information",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nTransaction Status Infortation is not available");


	//TAG 9F39

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F39;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGF_TRACE("\nTechnology %d",chTechnology);
		if(chTechnology==CTS_CTLS){
			memcpy(tknE1 + idx,"\x9F\x39\x01\x07",4);
			idx+=4;
			LOGF_TRACE("\nPOS Entry Mode");
		}else{
			memcpy(tknE1 + idx,buffTAG,szTAG);
			idx+=szTAG;
			LOGAPI_HEXDUMP_TRACE((char *)"Point-of-Service (POS) Entry Mode",buffTAG,szTAG);
		}
	}
	else
		LOGF_TRACE("\nPoint-of-Service (POS) Entry Mode is not available");
				

	//TAG 8A
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Authorization Response Code",buffTAG,szTAG);
	}
	else
		LOGF_TRACE("\nAuthorization Response Code is not available");



	//TAG 99
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = TAG_99_TRANS_PIN_DATA;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		/*memcpy(tknE1 + idx,buffTAG,szTAG);
		idx+=szTAG;
		LOGAPI_HEXDUMP_TRACE((char *)"Transaction Personal Identification Number (PIN) Data",buffTAG,szTAG);
		LOGF_TRACE("size Tag 99 = %i",szTAG);*/
		memcpy(tknE1 + idx,"\x99\x00",2);
		idx+=2;
		//LOGAPI_HEXDUMP_TRACE((char *)"Transaction Personal Identification Number (PIN) Data",buffTAG,szTAG);
		LOGF_TRACE("\nsize Tag 99 = %i",2);
	}
	else
		LOGF_TRACE("\nTransaction Personal Identification Number (PIN) Data is not available");


	idx += AddTag9F6E(tknE1 + idx,1);
		

	LOGF_TRACE("\nTamanio del token E1 %d",idx);

	if(type_cmd==C53)
	{
		//Token E1
		LOGF_TRACE("\nTamanio del token E1 %d",idx);
		tknE1[0] = 0xE1;
		tknE1[1] = 0xFF & (idx-2);
	}
	LOGAPI_HEXDUMP_TRACE((char *)"Token E1 assembled",tknE1,idx);
	*szTknE1 = idx;
	LOGF_TRACE("\nassembleTokenE1 --End");
}


int build_C54()
{
	int iResult = 0;
	char LengthC54[3] = {0};
	int szLengthC54 = 0;
	char TknE2[512] = {0};
	int szTknE2 = 0;

	int szCmd = 0;
	int typctls = 0;
	int i = 0;	
	int statuscard = 0;

	LOGF_TRACE("--build_C54--");

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
	
		LOGF_TRACE("Response for EMV or CTLS CHIP");
		
		if(chTechnology == CTS_CHIP)
		{
			statuscard = _CHECK_INSERTED_CARD();
			LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);

			if(!statuscard)
			{
				LOGF_TRACE("Se ha removido la tarjeta");
				memset(TknE2,0x00,sizeof(TknE2));
				szTknE2 = 0;
				szCmd+=szTknE2; 
				//uiInvokeURL(1,"tarjeta_removida.html");
				//uiInvokeURL(0,value,"tarjeta_removida.html");
				//sleep(2);
			}
			else
			{
				//Token E2
				asmtknE2(TknE2,&szTknE2,C54);
				szCmd+=szTknE2; 	
			}
							
		}
		else
		{
			
			//Token E2
			asmtknE2(TknE2,&szTknE2,C54);
			szCmd+=szTknE2; 	
		}	
	}
	if( (chTechnology == CTS_MSR || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) )  || chTechnology == CTS_KBD))
	{
		LOGF_TRACE("Response for MSR or CTLS MSR");
		szTknE2 = 2;
		memcpy(TknE2,"\xE2\x00",szTknE2);
		szCmd += szTknE2;
	}	  	

	//Calculamos la longiutd de todo el campo C54
	
	if(chTechnology == CTS_CHIP)
	{
		statuscard = _CHECK_INSERTED_CARD();
		
		LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);
		if(!statuscard)
		{
			LengthC54[1] = 0x00;
			LengthC54[0] = 0x00;
			szLengthC54 = 2;
		}
		
		else
		{
			LengthC54[1] = szCmd & 0x000000FF;
			LengthC54[0] = szCmd & 0x0000FF00;	
			szLengthC54 = 2;
		}
	}
	else
	{
		LengthC54[1] = szCmd & 0x000000FF;
		LengthC54[0] = szCmd & 0x0000FF00;	
		szLengthC54 = 2;
	}

	vdSetByteValuePP((char*)"TOKE2",TknE2,szTknE2);
	vdSetByteValuePP((char*)"LENGH",LengthC54,szLengthC54);
	 
	return iResult;
}

int buildEndTransactionReply54()
{
	int iResult = 0;
	char LengthC54[3] = {0};
	int szLengthC54 = 0;
	char TknE2[512] = {0};
	int szTknE2 = 0;

	int szCmd = 0;
	int typctls = 0;
	int i = 0;	
	int statuscard = 0;

	LOGF_TRACE("--buildEndTransactionReply54--");

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
	
		LOGF_TRACE("Response for EMV or CTLS CHIP");
		
		if(chTechnology == CTS_CHIP)
		{
			statuscard = _CHECK_INSERTED_CARD();
			LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);

			if(!statuscard)
			{
				LOGF_TRACE("Se ha removido la tarjeta");
				memset(TknE2,0x00,sizeof(TknE2));
				szTknE2 = 0;
				szCmd+=szTknE2; 
				//uiInvokeURL(1,"tarjeta_removida.html");
				//uiInvokeURL(0,value,"tarjeta_removida.html");
				//sleep(2);
			}
			else
			{
				//Token E2
				assembleTokenE2(TknE2,&szTknE2,C54);
				szCmd+=szTknE2; 	
			}
							
		}
		else
		{
			
			//Token E2
			assembleTokenE2(TknE2,&szTknE2,C54);
			szCmd+=szTknE2; 	
		}	
	}
	if( (chTechnology == CTS_MSR || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) )  || chTechnology == CTS_KBD))
	{
		LOGF_TRACE("Response for MSR or CTLS MSR");
		szTknE2 = 2;
		memcpy(TknE2,"\xE2\x00",szTknE2);
		szCmd += szTknE2;
	}	  	

	//Calculamos la longiutd de todo el campo C54
	
	if(chTechnology == CTS_CHIP)
	{
		statuscard = _CHECK_INSERTED_CARD();
		
		LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);
		if(!statuscard)
		{
			LengthC54[1] = 0x00;
			LengthC54[0] = 0x00;
			szLengthC54 = 2;
		}
		
		else
		{
			LengthC54[1] = szCmd & 0x000000FF;
			LengthC54[0] = szCmd & 0x0000FF00;	
			szLengthC54 = 2;
		}
	}
	else
	{
		LengthC54[1] = szCmd & 0x000000FF;
		LengthC54[0] = szCmd & 0x0000FF00;	
		szLengthC54 = 2;
	}

	vdSetByteValuePP((char*)"TOKE2",TknE2,szTknE2);
	vdSetByteValuePP((char*)"LENGH",LengthC54,szLengthC54);
	 
	return iResult;
}

void asmtknE2(char* TOKEN_out,int* szTOKEN_out,int type_cmd)
{
	//Variables necesarias para parsear los del token E1 o E2
//Variables necesarias para parsear los del token E1 o E2
		
	char TOKEN[500] = {0}; 
	int szTOKEN = 0;
	unsigned char TAGS[500] = {0};
	unsigned short szTAGS = 0;
	int idx = 0;
	int idx_tkne2 = 0;
	//int idx_tkne2 = 1;
	//unsigned long* request_tags;
	unsigned short numoftag = 0;
	unsigned long aux_tag = 0;
	unsigned char auxtag[2] = {0};	
	unsigned char Remove_card;
	//int lTag8Aagregado = FALSE;

	int iResult = 0;
	int iSigRequired = 0;
		
	//request_tags = (unsigned long*)malloc(szTOKEN*4);
	
	LOGF_TRACE("--amstknE2--");

	//memset(&request_tags,0x00,sizeof(request_tags));

	
	if(type_cmd == C53 ||type_cmd == C33)
		szTOKEN = inGetByteValuePP((char *)"RTOKE1",TOKEN);	
	else if(type_cmd == C54 || type_cmd == C34)
		szTOKEN = inGetByteValuePP((char *)"TOKE2",TOKEN);	
	
	
	LOGF_TRACE("Tamaño del buffer donde viene token E2 = %i",szTOKEN);
		
	LOGAPI_HEXDUMP_TRACE((char *)"Request tags",TOKEN,szTOKEN);
	
	LOGAPI_HEXDUMP_TRACE((char *)"ESTRUCTURA CTLS",&CTLSTransRes,sizeof(CTLSTransRes));

	
		
	for(idx = 0; idx < szTOKEN; idx++)
	{		
		aux_tag = 0L;
		if(!memcmp(&TOKEN[idx],"\x9F",1) || !memcmp(&TOKEN[idx],"\x5F",1))	
		{
			LOGF_TRACE("Tag de dos posiciones\n");
			memcpy(auxtag,&TOKEN[idx],2);			
			aux_tag &= 0x000000FF;
			aux_tag = auxtag[0];
			aux_tag = aux_tag << 8;
			aux_tag |= auxtag[1];	
			aux_tag &= 0x0000FFFF;				
			//request_tags[numoftag] = aux_tag;
			//LOGF_TRACE("\nTAGS = %X \n",request_tags[numoftag]);
			idx++;
			numoftag++;
		}
		else
		{
			LOGF_TRACE("Tag de una posición\n");
			aux_tag = TOKEN[idx];
			aux_tag &= 0x000000FF;
			//request_tags[numoftag] = aux_tag;
			//LOGF_TRACE("\nTAGS = %X \n",request_tags[numoftag]);
			numoftag++;
		}	
		
					
		szTAGS = 0;						
		iResult =  getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&aux_tag, 1,TAGS, sizeof(TAGS), &szTAGS,chTechnology);

		LOGF_TRACE("Tag %X obtained",aux_tag);
		LOGF_TRACE("size %i obtained",szTAGS);
		LOGF_TRACE("idx_tkne2 -> %i",idx_tkne2);

		if(aux_tag == 0x8A && szTAGS > 0)
		{
		//	lTag8Aagregado = TRUE;
			LOGF_TRACE("TAG 8A AGREGADO");
		}

		if (aux_tag == 0x9F6E)
		{
			if (szTAGS <= 0)
				idx_tkne2 += AddTag9F6E(&TOKEN_out[idx_tkne2 + 2],1);
			else if (chTechnology != CTS_CTLS)
			{
				szTAGS = 3;
				TAGS[2] = 0x00;
			}
		}

		if( (iResult == EMV_ADK_OK) && (szTAGS > 0) )
		{
			if (aux_tag == 0x9F27 && chTechnology == CTS_CTLS && type_cmd == C54)
			{
				LOGF_TRACE("Se encontro el tag 0x9F27 para CTLS");
				if( lnGetEnvVar((char*)"STATUS", NULL) == 0)
				{
					LOGF_TRACE("La transacción fue autorizada");
					TAGS[3] = 0x40;
				}
			}

			if(aux_tag==0x9F34 && chTechnology==CTS_CTLS)
			{

				LOGF_TRACE("Se encontro el tag 0x9F34 para CTLS");
				memset(TAGS,0x00,sizeof(TAGS));
			    memcpy(TAGS,"\x9F\x34\x03",3);
			    szTAGS=3;
			    memcpy(&TAGS[szTAGS],CTLSTransRes.T_9F34_CVM_Res,sizeof(CTLSTransRes.T_9F34_CVM_Res));
			    szTAGS+=3;
			    LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F34 CTLS",TAGS,szTAGS);
						
			}
	
			if(aux_tag==0x9F09 && chTechnology==CTS_CTLS)
			{
				LOGAPI_HEXDUMP_TRACE((char *)"BAD TAG obtained",TAGS,szTAGS);
				LOGAPI_HEXDUMP_TRACE((char *)"STRUCT TAG obtained",CTLSTransRes.T_9F09_VerNum,sizeof(CTLSTransRes.T_9F09_VerNum));


				LOGF_TRACE("Se encontro el tag 0x9F09 para CTLS");
				memcpy(TAGS,"\x9F\x09\x02",3);
				TAGS[3]=0x00;
				TAGS[4]=0x01;
				szTAGS=5;	
				LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F09 CTLS",TAGS,szTAGS);

			} 
			if(aux_tag==0x9F1E && chTechnology==CTS_CTLS)
			{
				LOGF_TRACE("Se encontro el tag 9F1E para CTLS");
				memset(TAGS,0x00,sizeof(TAGS));
				szTAGS=getserialnumber((char*)TAGS);
				LOGF_TRACE("szTAGS %i",szTAGS);	
				LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F1E CTLS",TAGS,szTAGS);
			
			}
			if(aux_tag==0x9F53 &&chTechnology==CTS_CTLS)
			{
				LOGF_TRACE("Se encontro el tag 0x9F53");	
				if(chTechnology==CTS_CTLS)
				{
					LOGF_TRACE("Se encontro el tag 9F53 para CTLS");
					memcpy(TAGS,"\x9F\x53\x01",3);
					TAGS[3]=0x52;
					szTAGS=4;	
					LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F53 CTLS",TAGS,szTAGS);
					
				}
			}
			
			LOGF_TRACE("size %i obtained",szTAGS);
			LOGAPI_HEXDUMP_TRACE((char *)"TAG obtained",TAGS,szTAGS);
			memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
			idx_tkne2+=szTAGS;			
		}
		else
		{

			if(Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F03 && szTAGS == 0)
			{
					memcpy(TAGS,"\x9F\x03\x06\x00\x00\x00\x00\x00\x00",9);
					szTAGS=9;
					memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
					idx_tkne2+=szTAGS;

			}
			else if(Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F53)
			{
				LOGF_TRACE("Se encontro el tag 0x9F53");	
				if(chTechnology==CTS_CTLS)
				{
					LOGF_TRACE("Se encontro el tag 9F53 para CTLS");
					memcpy(TAGS,"\x9F\x53\x01",3);
					TAGS[3]=0x52;
					szTAGS=4;	
					LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F53 CTLS",TAGS,szTAGS);
					memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
					idx_tkne2+=szTAGS;	
				}
			}
			else if (Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F34)
			{
				memset(TAGS,0x00,sizeof(TAGS));

				if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
				{
					aux_tag = 0x9F6C;
					iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&aux_tag, 1,TAGS, sizeof(TAGS), &szTAGS,chTechnology);
					iSigRequired = inCTQvalue((char*)&TAGS[3]);
					memset(TAGS,0x00,sizeof(TAGS));

					if(iSigRequired == 1)
						memcpy(TAGS,"\x9F\x34\x03\x1E\x00\x00",6);
					else if(iSigRequired == 2)
						memcpy(TAGS,"\x9F\x34\x03\x1F\x00\x00",6);
					else if(iSigRequired == 3)
						memcpy(TAGS,"\x9F\x34\x03\x41\x00\x00",6);
					else
						memcpy(TAGS,"\x9F\x34\x03\x00\x00\x00",6);
					szTAGS = 6;
				}
				else
				{
					memcpy(TAGS,"\x9F\x34\x03",3);
					szTAGS=3;
				    inGet9F34Ctls(TAGS+szTAGS);
				    //memcpy(&TAGS[szTAGS],TAGS,3);
				    szTAGS+=3;
				}
			    LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F34 CTLS",TAGS,szTAGS);
			    memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
				idx_tkne2+=szTAGS;
			}

		}
					

		aux_tag = 0;		
	}

	TOKEN_out[0] = 0xE2;		
	TOKEN_out[1] = idx_tkne2;
	*szTOKEN_out = idx_tkne2 + 2;	
	LOGF_TRACE("sizeof token E2 cmd C53 = %i",*szTOKEN_out);
	//free(request_tags);
	LOGF_TRACE("Done token E2");
	
	LOGAPI_HEXDUMP_TRACE((char *)"E2 RESULT",TOKEN_out,*szTOKEN_out);

	/*	
	TOKEN_out[0] = 0xE2;		
	TOKEN_out[1] = 0x01;
	TOKEN_out[2] = 0x00;
	*szTOKEN_out = idx_tkne2 + 2;	
	LOGAPI_HEXDUMP_TRACE((char *)"E2 after",TOKEN_out,idx_tkne2);
	LOGF_TRACE("sizeof token E2 cmd C53 = %i",*szTOKEN_out);
	free(request_tags);
	*/
	
}

void assembleTokenE2(char* TOKEN_out,int* szTOKEN_out,int type_cmd)
{
	//Variables necesarias para parsear los del token E1 o E2
		
	char TOKEN[500] = {0}; 
	int szTOKEN = 0;
	unsigned char TAGS[500] = {0};
	unsigned short szTAGS = 0;
	int idx = 0;
	int idx_tkne2 = 0;
	unsigned long* request_tags;
	unsigned short numoftag = 0;
	unsigned long aux_tag = 0;
	unsigned char auxtag[2] = {0};	

	int  iResult = 0;
	bool boGetFromTAGS = true;
	int iSigRequired=0;
		
	request_tags = (unsigned long*)malloc(szTOKEN*4);
	
	memset(request_tags,0x00,sizeof(request_tags));

			
	LOGF_TRACE("--assembleTokenE2--");
	if(type_cmd == C53 ||type_cmd == C33)
		szTOKEN = inGetByteValuePP((char *)"RTOKE1",TOKEN);	
	else if(type_cmd == C54 || type_cmd == C34)
		szTOKEN = inGetByteValuePP((char *)"TOKE2",TOKEN);	
	
	LOGF_TRACE("Tama�o del buffer donde viene token E2 = %i",szTOKEN);
		
	LOGAPI_HEXDUMP_TRACE((char *)"Request tags",TOKEN,szTOKEN);
		
	for(idx = 0; idx < szTOKEN; idx++)
	{		
		if(!memcmp(&TOKEN[idx],"\x9F",1) || !memcmp(&TOKEN[idx],"\x5F",1))	
		{
			LOGF_TRACE("Tag de dos posiciones\n");
			memcpy(auxtag,&TOKEN[idx],2);			
			aux_tag &= 0x000000FF;
			aux_tag = auxtag[0];
			aux_tag = aux_tag << 8;
			aux_tag |= auxtag[1];	
			aux_tag &= 0x0000FFFF;				
			//request_tags[numoftag] = aux_tag;
			LOGF_TRACE("\nTAGS = %X \n",request_tags[numoftag]);
			idx++;
			numoftag++;
		}
		else
		{
			LOGF_TRACE("Tag de una posici�n\n");
			aux_tag = TOKEN[idx];
			aux_tag &= 0x000000FF;
			//request_tags[numoftag] = aux_tag;
			LOGF_TRACE("\nTAGS = %X \n",request_tags[numoftag]);
			numoftag++;
		}	
		
		// jbf 20171228
		boGetFromTAGS = true;
		if ( (aux_tag == 0x9F53) || (aux_tag == 0x9F16) || (aux_tag == 0x9F1C) || (aux_tag == 0x9F1E) || (aux_tag == 0x9F39) ) {
			char POSSN_LAST7[8+1];
			char POS_PTID[8+1];
			
			LOGF_TRACE("\nTag = [%X] \n",aux_tag);
			boGetFromTAGS = false;
			iResult = EMV_ADK_OK;
			switch (aux_tag) {
				case 0x9F53:
					szTAGS = 4;
					memcpy(TAGS, (char *)"\x9F\x53\x01\x52",szTAGS);
					break;
				case 0x9F16:
					szTAGS = 3;
					memcpy(TAGS, (char *)"\x9F\x16\x00",szTAGS);
					break;
				case 0x9F1C:
					szTAGS = 11;
					LOGF_TRACE("\nTag = 9F1C \n");
					
					memcpy(TAGS, (char *)"\x9F\x1C\x08\x33", 4);
					if ( getSerialLast7(POSSN_LAST7) ) {
						memcpy(&TAGS[4], POSSN_LAST7, szTAGS - 4);						
					}
					else {
						memcpy(&TAGS[4], (char *)"\x00\x00\x00\x00\x00\x00\x00\x00", szTAGS - 4);
					}
					break;
				case 0x9F1E:
					szTAGS = 11;
					LOGF_TRACE("\nTag = 9F1E \n");
					/* jbf 20180110
					memcpy(TAGS, (char *)"\x9F\x1E\x08", 3);
					if ( getPTID(POS_PTID) ) {
						LOGF_TRACE("\nPOS_PTID = [%s] \n",POS_PTID);
						memcpy(&TAGS[3], POS_PTID, szTAGS - 3);						
					}
					else {
						memcpy(&TAGS[3], (char *)"\x00\x00\x00\x00\x00\x00\x00\x00", szTAGS - 3);
					}
					*/
					memcpy(TAGS, (char *)"\x9F\x1E\x08\x33", 4);
					if ( getSerialLast7(POSSN_LAST7) ) {
						memcpy(&TAGS[4], POSSN_LAST7, szTAGS - 4);						
					}
					else {
						memcpy(&TAGS[4], (char *)"\x00\x00\x00\x00\x00\x00\x00\x00", szTAGS - 4);
					}
					break;
				case  0x9F39:
					if(chTechnology==CTS_CTLS){
						szTAGS = 4;
						memcpy(TAGS, (char *)"\x9F\x39\x01\x07",szTAGS);
					}else{
						szTAGS = 4;
						memcpy(TAGS, (char *)"\x9F\x39\x01\x05",szTAGS);
					}
				break;
			}
		}	
		
		if (boGetFromTAGS) {
            LOGAPI_HEXDUMP_TRACE("TAGS",TAGS,szTAGS); //PGC 03SEPT25
			szTAGS = 0;						
			iResult =  getCT_EMV_TAG(0,&aux_tag, 1,TAGS, sizeof(TAGS), &szTAGS,chTechnology);
			LOGF_TRACE("tagValueeeeee: %X", aux_tag);  //PGC 03SEPT25
			
		}


		if (aux_tag == 0x9F6E)
		{
			if (szTAGS <= 0)
				idx_tkne2 += AddTag9F6E(&TOKEN_out[idx_tkne2 + 2],1);
			else if (chTechnology != CTS_CTLS)
			{
				szTAGS = 3;
				TAGS[2] = 0x00;
			}
		}


		//if( (iResult == EMV_ADK_OK) && (szTAGS > 3) )
		if((iResult == EMV_ADK_OK) && (szTAGS > 3) || (aux_tag == 0x9C)) // PGC 03SEP2T5
		{
			/*LOGF_TRACE("Tag %X obtained",aux_tag);
			LOGF_TRACE("idx_tkne2 -> %i",idx_tkne2);
			LOGAPI_HEXDUMP_TRACE((char *)"TAG obtained",TAGS,szTAGS);
			memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
			idx_tkne2+=szTAGS;*/
			if (aux_tag == 0x9F27 && chTechnology == CTS_CTLS && type_cmd == C54)
			{
				LOGF_TRACE("Se encontro el tag 0x9F27 para CTLS");
				if( lnGetEnvVar((char*)"STATUS", NULL) == 0)
				{
					LOGF_TRACE("La transacción fue autorizada");
					TAGS[3] = 0x40;
				}
			}

			if(aux_tag==0x9F34 && chTechnology==CTS_CTLS)
			{

				LOGF_TRACE("Se encontro el tag 0x9F34 para CTLS");
				memset(TAGS,0x00,sizeof(TAGS));
			    memcpy(TAGS,"\x9F\x34\x03",3);
			    szTAGS=3;
			    memcpy(&TAGS[szTAGS],CTLSTransRes.T_9F34_CVM_Res,sizeof(CTLSTransRes.T_9F34_CVM_Res));
			    szTAGS+=3;
			    LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F34 CTLS",TAGS,szTAGS);
						
			}
	
			if(aux_tag==0x9F09 && chTechnology==CTS_CTLS)
			{
				LOGAPI_HEXDUMP_TRACE((char *)"BAD TAG obtained",TAGS,szTAGS);
				LOGAPI_HEXDUMP_TRACE((char *)"STRUCT TAG obtained",CTLSTransRes.T_9F09_VerNum,sizeof(CTLSTransRes.T_9F09_VerNum));


				LOGF_TRACE("Se encontro el tag 0x9F09 para CTLS");
				memcpy(TAGS,"\x9F\x09\x02",3);
				TAGS[3]=0x00;
				TAGS[4]=0x01;
				szTAGS=5;	
				LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F09 CTLS",TAGS,szTAGS);

			} 
			if(aux_tag==0x9F1E && chTechnology==CTS_CTLS)
			{
				LOGF_TRACE("Se encontro el tag 9F1E para CTLS");
				memset(TAGS,0x00,sizeof(TAGS));
				szTAGS=getserialnumber((char*)TAGS);
				LOGF_TRACE("szTAGS %i",szTAGS);	
				LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F1E CTLS",TAGS,szTAGS);
			
			}
			if(aux_tag==0x9F53 &&chTechnology==CTS_CTLS)
			{
				LOGF_TRACE("Se encontro el tag 0x9F53");	
				if(chTechnology==CTS_CTLS)
				{
					LOGF_TRACE("Se encontro el tag 9F53 para CTLS");
					memcpy(TAGS,"\x9F\x53\x01",3);
					TAGS[3]=0x52;
					szTAGS=4;	
					LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F53 CTLS",TAGS,szTAGS);
					
				}
			}
			
			LOGF_TRACE("size %i obtained",szTAGS);
			LOGAPI_HEXDUMP_TRACE((char *)"TAG obtained",TAGS,szTAGS);
			memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
			idx_tkne2+=szTAGS;					
		}
		else
		{

			if(Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F03 && szTAGS == 0)
			{
					memcpy(TAGS,"\x9F\x03\x06\x00\x00\x00\x00\x00\x00",9);
					szTAGS=9;
					memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
					idx_tkne2+=szTAGS;

			}
			else if(Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F53)
			{
				LOGF_TRACE("Se encontro el tag 0x9F53");	
				if(chTechnology==CTS_CTLS)
				{
					LOGF_TRACE("Se encontro el tag 9F53 para CTLS");
					memcpy(TAGS,"\x9F\x53\x01",3);
					TAGS[3]=0x52;
					szTAGS=4;	
					LOGAPI_HEXDUMP_TRACE((char *)"TAG 9F53 CTLS",TAGS,szTAGS);
					memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
					idx_tkne2+=szTAGS;	
				}
			}
			else if (Check_typ_CTLS() == CTLS_TXN_CHIP && aux_tag==0x9F34)
			{
				memset(TAGS,0x00,sizeof(TAGS));

				if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
				{
					aux_tag = 0x9F6C;
					iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&aux_tag, 1,TAGS, sizeof(TAGS), &szTAGS,chTechnology);
					iSigRequired = inCTQvalue((char*)&TAGS[3]);
					memset(TAGS,0x00,sizeof(TAGS));

					if(iSigRequired == 1)
						memcpy(TAGS,"\x9F\x34\x03\x1E\x00\x00",6);
					else if(iSigRequired == 2)
						memcpy(TAGS,"\x9F\x34\x03\x1F\x00\x00",6);
					else if(iSigRequired == 3)
						memcpy(TAGS,"\x9F\x34\x03\x41\x00\x00",6);
					else
						memcpy(TAGS,"\x9F\x34\x03\x00\x00\x00",6);
					szTAGS = 6;
				}
				else
				{
					memcpy(TAGS,"\x9F\x34\x03",3);
					szTAGS=3;
				    inGet9F34Ctls(TAGS+szTAGS);
				    //memcpy(&TAGS[szTAGS],TAGS,3);
				    szTAGS+=3;
				}
			    LOGAPI_HEXDUMP_TRACE((char *)"TAG 0x9F34 CTLS",TAGS,szTAGS);
			    memcpy(&TOKEN_out[idx_tkne2 + 2],TAGS,szTAGS);
				idx_tkne2+=szTAGS;
			}

		}		

		aux_tag = 0;		
	}

	/*
	TOKEN_out[0] = 0xE2;		
	TOKEN_out[1] = idx_tkne2;
	*szTOKEN_out = idx_tkne2 + 2;	
	LOGF_TRACE("sizeof token E2 cmd C53 = %i",*szTOKEN_out);
	free(request_tags);
	LOGF_TRACE("Done token E2");
	*/
	LOGAPI_HEXDUMP_TRACE((char *)"E2",TOKEN_out,5);
		
	TOKEN_out[0] = 0xE2;		
	TOKEN_out[1] = 0XFF & (idx_tkne2);
	*szTOKEN_out = idx_tkne2 + 2;
	//LOGAPI_HEXDUMP_TRACE((char *)"E2 after",TOKEN_out,idx_tkne2);
	LOGAPI_HEXDUMP_TRACE((char *)"E2 after",TOKEN_out,*szTOKEN_out);
	LOGF_TRACE("sizeof token E2 cmd C53 = %i",*szTOKEN_out);
	free(request_tags);
	LOGF_TRACE("Done token E2");
}

void asmTknE2C54(char* tknE2,int* szTknE2)
{
	int idx = 0;
	int iResult = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[50] = {0};
	
	LOGF_TRACE("--asmTknE2C54--");

	
	//TAG 9F26	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F26;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Application Cryptogram");
	}
	else
		debug("\nApplication Cryptogram Data is not available");

	
	//TAG 9F27
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Cryptogram information Data");
	}
	else
		debug("\nCryptogram information Data is not available");
	
	//TAG 9F36
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F3;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Aplication Transaction Counter"); 
	}
	else
		debug("Aplication Transaction Counter is not available");
	
	//TAG 95
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Terminal Verification Result");
	}
	else
		debug("\nTerminal Verification Result is not available");
	
	//TAG 9F10
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F10;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Issuer Application Data");
	}
	else
		debug("\nIssuer Application DataName is not available");
	
	//TAG 9F37

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F37;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Unpredictable Number");	
	}
	else
		debug("Unpredictable Numbere is not available");
	
	//TAG 9B
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Transaction Status Information");
	}
	else
		debug("\nTransaction Status Infortation is not available");
	
	//TAG 8A

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		memcpy(tknE2 + idx,buffTAG,szTAG);
		idx+=szTAG;
		dump(buffTAG,szTAG,(char *)"Authorization Response Code");
	}
	else
		debug("\nAuthorization Response Code is not available");

	LOGF_TRACE("sizeof Token E2 %i",idx);

	*szTknE2 = idx;

}


int Host_Response()
{
	int iResult = 0;
	//Variables para obtener la informaci�n que viene en el comando C54
	char Status_Host[3] = {0};
	int szStatus_Host;
	char HostResponse[3] = {0};
	int szHost_Response = 0;
	static unsigned char Tag91[32] = {0}; // 20180125 define as static
	int szTg91 = 0;
	unsigned char resp_code[2] = {0};
	int szresp_code = 0;
	unsigned char AuthCode[7] = {0};
	int szAuthCode = 0;	 
	
	//Variables para almacenar los scripts que vienen en el comando C25	
	static unsigned char Script71[5000] = {0}; // 20180125 define as static
	int szScript71 = 0;	
	static unsigned char Script71_aux[5000] = {0}; // 20180125 define as static
	int szScript71_aux = 0;	
	static unsigned char Script72[5000] = {0};	// 20180125 define as static
	int szScript72 = 0;	
	static unsigned char Script72_aux[5000] = {0}; // 20180125 define as static
	int szScript72_aux = 0;	

	LOGF_TRACE("--Host_Response para Chip y CTLS chip--");

	memset(Tag91, 0x00,sizeof(Tag91)); // 20180125 
	//Informaci�n que viene dentro del comando C54 o C34
	szStatus_Host = inGetByteValuePP((char*)"STATUS",Status_Host);
	szHost_Response = inGetByteValuePP((char*)"HOSTRESPONSE",(char*)HostResponse);
	szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);	
	szTg91 = inGetByteValuePP((char*)"ISS_UTH",(char*)&Tag91[2]);
	szAuthCode = inGetByteValuePP((char*)"AUHTRES",(char*)AuthCode);

	memset(Script71, 0x00,sizeof(Script71)); // 20180125 
	memset(Script72, 0x00,sizeof(Script72)); // 20180125 
	//Informaci�n que viene dentro del comando C25	
	szScript71 = inGetByteValuePP((char*)"SCRIPT71",(char*)&Script71[2]);
	szScript72 = inGetByteValuePP((char*)"SCRIPT72",(char*)&Script72[2]);
			
	if(szTg91 > 0)
	{
		LOGF_TRACE("--TAG 91--");
		Tag91[0] = 0x91;
		Tag91[1] = szTg91;
		LOGAPI_HEXDUMP_TRACE("ISS_UTH",Tag91,szTg91+2);

		xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_AUTHDATA;
		iResult=set_EMV_TAG(EMV_ADK_UPDATETAGS_ERROR_ON_NOT_ALLOWED_TAG,(unsigned char*)Tag91,szTg91+2,CTS_CHIP);
		LOGF_TRACE("**ISS_UTH RES %d **",iResult);
		iResult=0;
	}
	else
	{
		LOGF_TRACE("THERE IS NO Tag 91");
		xOnlineInputCT.LenAuth = 0;
		xOnlineInputCT.AuthData = NULL;
	}

	if(szScript71 > 0)
	{
		if(chTechnology == CTS_CHIP)
		{
			LOGF_TRACE("There are Scripts 71");
			szScript71 = Script_Format((const char*)&Script71[2], szScript71, 0x71, (char*)Script71, 512);

			xOnlineInputCT.LenScriptCrit = szScript71;
			xOnlineInputCT.ScriptCritData = Script71;
			xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_SCRIPTCRIT;

			LOGF_TRACE("--SCRIPT 71--");
			LOGAPI_HEXDUMP_TRACE("Script 71", Script71, szScript71);
		}
	}
	else 
	{
		LOGF_TRACE("There is not any Script 71");
		xOnlineInputCT.LenScriptCrit = 0;
		xOnlineInputCT.ScriptCritData = NULL;
	}

	if(szScript72 > 0)
	{
		if(chTechnology == CTS_CHIP)			
		{	
			LOGF_TRACE("There are Scripts 72");
			szScript72 = Script_Format((const char*)&Script72[2], szScript72, 0x72, (char*)Script72, 512);

			xOnlineInputCT.LenScriptUnCrit = szScript72;
			xOnlineInputCT.ScriptUnCritData = Script72;
			xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_SCRIPTUNCRIT;

			LOGF_TRACE("--SCRIPT 72--");
			LOGAPI_HEXDUMP_TRACE("Script 72", Script72, szScript72);
		}
	}
	else 
	{
		LOGF_TRACE("There is not any Script 72");
		xOnlineInputCT.LenScriptUnCrit = 0;
		xOnlineInputCT.ScriptUnCritData = NULL;
	}

	/*if(szScript71 > 0)
	{
		LOGF_TRACE("--SCRIPT 71--");
		LOGAPI_HEXDUMP_TRACE("Script 71",Script71,szScript71);
		//vdAsc2Hex ((const char*)szScript71_aux,(char*)&Script71[2],szScript71);		

		//Script71[0] = 0x71;
		//Script71[1] = szScript71;
		if(chTechnology == CTS_CHIP)				
		{				
			xOnlineInputCT.LenScriptCrit = szScript71;
			xOnlineInputCT.ScriptCritData = Script71;
			xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_SCRIPTCRIT;
		}		
     
	}
	if(szScript72 > 0)
	{
		LOGF_TRACE("--SCRIPT 72--");
		LOGAPI_HEXDUMP_TRACE("Script 72",Script72,szScript72);
		//vdAsc2Hex ((const char*)szScript72_aux ,(char*)Script72[2], szScript72);
		//Script72[0] = 0x72;
		//Script72[1] = szScript72;
		if(chTechnology == CTS_CHIP)			
		{	
			//dump(Script72,tkn_b6->szLngScrp,"SCRIPT 72 CHIP");
			xOnlineInputCT.LenScriptUnCrit = szScript72;
			xOnlineInputCT.ScriptUnCritData = Script72;
			xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_SCRIPTUNCRIT;
		}		

	}
	else 
	{
		LOGF_TRACE("--No hay scripts--");
		xOnlineInputCT.LenScriptCrit = 0;
		xOnlineInputCT.ScriptCritData = NULL;
		xOnlineInputCT.LenScriptUnCrit = 0;
		xOnlineInputCT.ScriptUnCritData = NULL;
		xOnlineInputCTLS.LenScriptData= 0;
		xOnlineInputCTLS.ScriptData = NULL;
	}*/
	
	LOGAPI_HEXDUMP_TRACE("Satus Host : ",Status_Host, szStatus_Host);
	LOGAPI_HEXDUMP_TRACE("HostResponse : ",HostResponse, szHost_Response);
	LOGAPI_HEXDUMP_TRACE("AUHTRES : ",AuthCode, szAuthCode);

	iResult=0;
	if(!memcmp(Status_Host,"\x00",szStatus_Host))
	{
		if(!memcmp(HostResponse,"00",szHost_Response)) // falta probar que pasa con un host reponse "70"
		{
			  iResult=1;
			  LOGF_TRACE("--Respuesta exitosa por el Host--");	  
			  if(chTechnology == CTS_CHIP)
			  {
			  	  xOnlineInputCT.OnlineResult = TRUE;
			      //memcpy(xOnlineInputCT.AuthResp, resp_code, szresp_code);
				  memcpy(xOnlineInputCT.AuthResp, HostResponse, szHost_Response);			
				  memcpy(xOnlineInputCT.AuthorizationCode, AuthCode, szAuthCode);
				  
				  xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_ONLINE_RESP | INPUT_ONL_ONLINE_AC | INPUT_ONL_AUTHCODE;
			  }
			  if(chTechnology == CTS_CTLS)
			  {		 
			   	memcpy(xOnlineInputCTLS.AuthResp,HostResponse, szHost_Response);
				//memcpy(xOnlineInputCTLS.AuthResp,resp_code, szresp_code);
				xOnlineInputCTLS.OnlineResult = TRUE;
				
				xOnlineInputCTLS.Info_Included_Data[0] |= INPUT_CTLS_ONL_ONLINE_RESP | INPUT_CTLS_ONL_AUTH_RESP;		  
			  }
		}
		else
		{
			LOGF_TRACE("--Declinada por HostResponse != 00 --");
				if(chTechnology == CTS_CHIP)
				{			
					  memcpy(xOnlineInputCT.AuthorizationCode, AuthCode, szAuthCode);
					  memcpy(xOnlineInputCT.AuthResp, "\x30\x35", 2);
					  //memcpy(xOnlineInputCT.AuthResp,resp_code, szresp_code);
					  xOnlineInputCT.OnlineResult = TRUE; 
					  xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_ONLINE_RESP | INPUT_ONL_ONLINE_AC| INPUT_ONL_AUTHCODE;
				}
				if(chTechnology == CTS_CTLS)
				{		
					 memcpy(xOnlineInputCTLS.AuthResp,"\x30\x35", 2);
					 xOnlineInputCTLS.OnlineResult = TRUE;					 
					 xOnlineInputCTLS.Info_Included_Data[0] |= INPUT_CTLS_ONL_ONLINE_RESP | INPUT_CTLS_ONL_AUTH_RESP;			
				}

		}
		
	}
	
	//else if(!memcmp(HostResponse,"01",szHost_Response))
	else if(!memcmp(Status_Host,"\x01",szStatus_Host))
	{
		LOGF_TRACE("--Declinada por el Host--");
		if(chTechnology == CTS_CHIP)
		{			
			  memcpy(xOnlineInputCT.AuthorizationCode, AuthCode, szAuthCode);
			  memcpy(xOnlineInputCT.AuthResp, "\x30\x35", 2);
		      //memcpy(xOnlineInputCT.AuthResp,resp_code, szresp_code);
			  xOnlineInputCT.OnlineResult = TRUE;
			  
			  xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_ONLINE_RESP | INPUT_ONL_ONLINE_AC| INPUT_ONL_AUTHCODE;
		}
		if(chTechnology == CTS_CTLS)
		{		
 			 memcpy(xOnlineInputCTLS.AuthResp,"\x30\x35", 2);
 			 xOnlineInputCTLS.OnlineResult = TRUE;
			 
			 xOnlineInputCTLS.Info_Included_Data[0] |= INPUT_CTLS_ONL_ONLINE_RESP | INPUT_CTLS_ONL_AUTH_RESP;			
		}
	}
	
	//else if(!memcmp(HostResponse,"02",szHost_Response))
	else if(!memcmp(Status_Host,"\x02",szStatus_Host))
	{
	  LOGF_TRACE("--Error de comunicacion con Host--");
		if(chTechnology == CTS_CHIP)
		{
		      memcpy(xOnlineInputCT.AuthResp, "\x30\x35", 2);
			  xOnlineInputCT.OnlineResult = FALSE;
			  xOnlineInputCT.Info_Included_Data[0] |= INPUT_ONL_ONLINE_RESP | INPUT_ONL_ONLINE_AC;
		}
		if(chTechnology == CTS_CTLS)
		{
	  		 memcpy(xOnlineInputCTLS.AuthResp, "\x30\x35", 2);
 			 xOnlineInputCTLS.OnlineResult = FALSE;
			 xOnlineInputCTLS.Info_Included_Data[0] |= INPUT_CTLS_ONL_ONLINE_RESP | INPUT_CTLS_ONL_AUTH_RESP;	
		}
	}
	//else if(!memcmp(HostResponse,"03",szHost_Response))
	else if(!memcmp(Status_Host,"\x03",szStatus_Host))
	{
	  LOGF_TRACE("\n--Abortar transacci�n--");	  
	  memcpy(xOnlineInputCTLS.AuthResp, "\x30\x35", 2);
	  xOnlineInputCTLS.OnlineResult = FALSE;
	  xOnlineInputCTLS.Info_Included_Data[0] |= INPUT_CTLS_ONL_ONLINE_RESP | INPUT_CTLS_ONL_AUTH_RESP;	 
	}
	LOGF_TRACE("-- End Host_Response--");
	return iResult;
}

int GetTag(unsigned long* inTag, unsigned char *chOut, unsigned short *uLen, int FLen, int inCond, unsigned char *bitmap, char chCh,int tech)
{
int inCic;
int inRetval;
unsigned char chData[63 + 1];
int offset = 0;

debug("\nTag = %X ",*inTag);    
inRetval = getCT_EMV_TAG(0,inTag,1,chData,sizeof(chData),uLen,tech);
if(*inTag <= 0xFF)
	offset = 2;
if( (*inTag >= 0xFF) && (*inTag <= 0xFFFF) )
	offset = 3;
if (inRetval == EMV_ADK_OK)
   {
   *bitmap|=inCond;
   vdHex2Asc((char*)(chData + offset),(char*)chOut, *uLen);
   for (inCic = ((*uLen) * 2); inCic < FLen; inCic++)
       chOut[inCic] = chCh;
   }
   else
   {
       for (inCic = 0; inCic < FLen; inCic++)
          chOut[inCic] = chCh;
   }
return inRetval;
}

static void voChangeSign(char *chTrack2)
{
	int inIndex = 0;

    while (chTrack2[inIndex] > 0)
    {
        if (chTrack2[inIndex] == '=')
        {
            chTrack2[inIndex] = 'D';
            break;
        }
        inIndex++;
    }
}
/***************************************************************************
verifica si hay llave IPEK cargada
***************************************************************************/

int check_key()
{
	int iResult = 0;

	LOGF_TRACE("\n--Check_key--\n");

	if(inGetEnvVar((char*)"DKPKEYLOD", NULL) )
	{	//VErify if there is a loaded key 	
		LOGF_TRACE("\nHay llave cargada!!");
		iResult = 0;
		LOGF_TRACE("\niResult = %i",iResult);
	}
	else 
	{
		LOGF_TRACE("\nNo existe llave cargada");		
		iResult = CPX_ERC_IPK;
		LOGF_TRACE("\niResult = %i",iResult);
	}

	return iResult;
}


//2 TRANSACCIONES PARA UNIADQUIRENCIA

int MSR_Transaction_UNI(int MSRead)

{
	char dataENCRYPT[256]  ;
	int szdataENCRYPT =  0 ;
	char tmpbuf[256] ;
	int in_val ;
  	int mov = 0 ;
	char dataMSR[256] = {0};
	int resultMSR = 0;
	//TO store the card data
	char trk1[100] = {0};
	int sztrk1 = 0;
	char trk2[50] = {0};
	int sztrk2 = 0;
	char trk3[100] = {0};
	int sztrk3 = 0;	
	char CrdHold[35] = {0};
	int szCrdHold = 0;
	char PAN[20] = {0};
	int szPAN = 0;
	char ExpDate[5] = {0};
	int szExpDate = 0;
	char ServCode[5] = {0};
	int szServCode = 0;
	char DiscData[30] = {0};
	int szDiscData = 0;
	
	//Variables necesarias para armar el campo C30
	char PanC30[20] = {0};	
	int szPanC30 = 0;
	char CardHoldC30[30] = {0};
	int szCardHoldC30 = 0;
	char ExpDataC30[10] = {0};
	int szExpDataC30 = 0;

	//Para evaluar el nible alto del byte del tipo de transacci�n
    char nibleTypeTxn = 0;
	char Txn_typ = 0;
	int szTxn_typ = 0;
		
	//FLAGS
	char flagCVV = 0;
	int status = 0;
	char flag_encrypt=0;

	char CVV[6] = {0};
	int szCVV2 = 4;


	char LenHex[3]={0};
	char lenfld = 0;
	char aux = 0;
	 
	LOGF_TRACE("\n----------MSR technology detected!!!");	
	
	if( (MSRead == MSR_OK) || (MSRead == MSR_ACTIVE) )
		MSRead = MSR_OK;
	else
		{
		LOGF_TRACE("MSR_ERROR!!");				
		}
			
	MSRead = Read_MSRdata();	
	LOGF_TRACE("\n MSRread = %i",MSRead);
	
	if(MSRead == MSR_OK )
	{
		status = getMSR_data((char *)"Track1",trk1,&sztrk1);
		LOGF_TRACE("\nTrack1: %s  longitud %i status %i",trk1,sztrk1,status);
		if(status<SUCCESS)
			return status;
		status = getMSR_data((char *)"Track2",trk2,&sztrk2);
		LOGF_TRACE("\nTrack2: %s longitud %i status %i",trk2,sztrk2,status);
		if(status<SUCCESS)
			return status;
		status = getMSR_data((char *)"Track3 ",trk3,&sztrk3);
		LOGF_TRACE("\nTrack3: %s longitud %i status %i",trk3,sztrk3,status);
		/*if(status<SUCCESS)
			return status;*/
		status = getMSR_data((char *)"Cardholder",CrdHold,&szCrdHold);
		LOGF_TRACE("\ncardholder: %s longitud %i status %i",CrdHold,szCrdHold,status);	
		if(status<SUCCESS)
			return status;
		status = getMSR_data((char *)"Pan",PAN,&szPAN);
		LOGF_TRACE("\nPan: %s longitud %i status %i",PAN,szPAN,status);	
		if(status<SUCCESS)
			return status;
		status = getMSR_data((char *)"ExpDate",ExpDate,&szExpDate);
		LOGF_TRACE("\nExpiration Date: %s longitud %i status %i",ExpDate,szExpDate,status);	
		if(status<SUCCESS)
			return status;
		status =  getMSR_data((char *)"ServCode",ServCode,&szServCode);
		LOGF_TRACE("\nService Code: %s longitud %i status %i",ServCode,szServCode,status);	 
		if(status<SUCCESS)
			return status;
		status =  getMSR_data((char *)"DiscData",DiscData,&szDiscData);
		LOGF_TRACE("\nDiscretionary data: %s longitud %i status %i",DiscData,szDiscData,status);
		/*if(status<SUCCESS)
			return status;*/
		
	}
	else if(MSRead == MSR_ERROR )
	{
		LOGF_TRACE("MSR_ERROR!!");
		//uiInvokeURL(1,"ERROR_BANDA.html");
		//uiInvokeURL(0,value"ERROR_BANDA.html");
		//sleep(2);
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
		
	}
	else if(MSRead == MSR_TIMEOUT )
	{
		LOGF_TRACE("MSR_TIMEOUT!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"06",2);
	}
	else if(MSRead == MSR_ABORTED)
	{
		LOGF_TRACE("MSR_ABORTED!!");
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
	}
	
	if(MSRead == MSR_OK)
	{

		if(check_service_code(ServCode)==false)
		{
			LOGF_TRACE("*****Use chip*****");
			return USE_CHIP;
		}
		
		//**********************************
		//Aqu� armamos el campo del Pan para el campo C30 para una transancci�n de banda magn�tica
		vdSetByteValuePP((char*)"STATUS",(char *)"21",2);
		mov = 0;
		if ( bo_crp_crd (PAN,szPAN))
			{
			
				LOGF_TRACE("*****FLAG 01*****");
				vdSetByteValuePP((char*)"FLAG_ENCRYPT",(char *)"\xC1\x01\x01",3);	
				flag_encrypt=1;
			}

		else
			{

				LOGF_TRACE("*****FLAG 00*****");
				vdSetByteValuePP((char*)"FLAG_ENCRYPT",(char *)"\xC1\x01\x00",3);	

			}
		mov += 3;
		LOGF_TRACE("*****MOV = %i *****", mov);
		memcpy(PanC30,"\xC1",1);
		lenfld = 0;
		lenfld |= szPAN;	//length
		PanC30[1] = lenfld;
		memcpy(PanC30 + 2,PAN,szPAN);
		vdSetByteValuePP((char*)"PAN",PanC30,szPAN + 2);
		mov += szPAN + 2 ;
		LOGF_TRACE("*****MOV = %i *****", mov);
		
		memcpy(ExpDataC30,"\xC1",1);
		ExpDataC30[1] = szExpDate;
		memcpy(ExpDataC30 + 2,ExpDate,szExpDate);
		vdSetByteValuePP((char*)"EXP_DATE",ExpDataC30,szExpDate + 2);
		mov += szExpDate + 2 ;
		LOGF_TRACE("*****MOV = %i *****", mov);

		memcpy(CardHoldC30,"\xC1",1);
		CardHoldC30[1] = szCrdHold;
		memcpy(&CardHoldC30[2],CrdHold,szCrdHold);
		vdSetByteValuePP((char*)"CARDHOLDER",CardHoldC30,szCrdHold + 2);
		mov += szCrdHold + 2;
		LOGF_TRACE("*****MOV = %i *****", mov);

		if(flag_encrypt==1)
		{
			LOGF_TRACE("*****ENCRIPTAR EL TRACK 2 *****");
			memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,trk2,sztrk2);
			aux = 0;
			LOGAPI_HEXDUMP_TRACE("COPIA : ",tmpbuf, sizeof(tmpbuf));
			if((sztrk2 % 16)!=0 && tmpbuf[sztrk2 - 1] == 0x20 )
			{	
				memcpy(tmpbuf + sztrk2,"\x01",1);
				LOGAPI_HEXDUMP_TRACE("ADD 0x01 : ",tmpbuf, sizeof(tmpbuf));
				aux = 1 ;
			}
	
			in_val = in_Cx_EDta(tmpbuf,sztrk2 + aux,dataENCRYPT,&szdataENCRYPT);
		
			//if(in_val < SUCCESS)
			//{
			//	LOGF_TRACE("*****FALLA AL ENCRIPTAR TRACK 2 *****");

		//	}

			if(in_val >FAILURE)
			{
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf, "\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2, dataENCRYPT,szdataENCRYPT);
				vdSetByteValuePP((char*)"TRACK2",tmpbuf,szdataENCRYPT + 2);
				mov += szdataENCRYPT + 2 ;
				LOGF_TRACE("*****MOV = %i *****", mov);
				
			}

		}

		else
		{
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf, "\xC1",1);
			tmpbuf[1]=sztrk2;
			memcpy(tmpbuf+2,trk2,sztrk2);
			vdSetByteValuePP((char*)"TRACK2",tmpbuf,sztrk2 + 2);
			mov += sztrk2 + 2 ;
			LOGF_TRACE("*****MOV = %i *****", mov);

		}

		

		if(flag_encrypt==1)
		{
			LOGF_TRACE("*****ENCRIPTAR EL TRACK 1 *****");
			memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf,trk1,sztrk1);
			LOGAPI_HEXDUMP_TRACE("COPIA : ",tmpbuf, sizeof(tmpbuf));
			aux = 0 ; 
			if((sztrk1 % 16)!=0 && tmpbuf[sztrk1 - 1] == 0x20)
			{
				memcpy(tmpbuf + sztrk1,"\x01",1);
				LOGAPI_HEXDUMP_TRACE("ADD 0x01 : ",tmpbuf, sizeof(tmpbuf));
				aux = 1 ;
			}
	
			in_val = in_Cx_EDta(tmpbuf,sztrk1 + aux,dataENCRYPT,&szdataENCRYPT);
		
			if(in_val < SUCCESS)
			{
				LOGF_TRACE("*****FALLA AL ENCRIPTAR TRACK 1 *****");

			}

			if(in_val >FAILURE)
			{
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf, "\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2, dataENCRYPT,szdataENCRYPT);
				vdSetByteValuePP((char*)"TRACK1",tmpbuf,szdataENCRYPT + 2);
				mov += szdataENCRYPT + 2;
				LOGF_TRACE("*****MOV = %i *****", mov);
			
			}
		}	

		else
		{
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf, "\xC1",1);
			tmpbuf[1]=sztrk1;
			memcpy(tmpbuf,trk1,sztrk1);
			vdSetByteValuePP((char*)"TRACK1",tmpbuf,sztrk1 + 2);
			mov += sztrk1 + 2;
			LOGF_TRACE("*****MOV = %i *****", mov);
			
		}
		
		//**********************************
		//Evaluamos el nible alto del byte Txn type que llega en el C51 para saber si pedir CCV
		
		//szTxn_typ = inGetByteValuePP((char *)"TRANSTYP",&Txn_typ);

		//nibleTypeTxn = Txn_typ & 0xF0;
		//nibleTypeTxn = nibleTypeTxn >> 4;

		//if(nibleTypeTxn == 0x01) 
		//	flagCVV	= 0;
		//else
		//	flagCVV = 1;

		if(in_val > FAILURE )
		{
			flagCVV = 1;	//Daee 06/06/2018  /*Req AOV*/
			//flagCVV = 0; // FAG 03-ABRIL-2017  //Daee 06/06/2018  /*Req AOV*/
			if(flagCVV)
			{
				in_val = screenSec_Code(CVV,szCVV2,"INGRESE CODIGO DE SEGURIDAD",""); 
	
			}

		}

	
		if(flag_encrypt==1)
		{

			if(in_val > FAILURE)
			{
				LOGF_TRACE("*****ENCRIPTAR CVV *****");
				memset(dataENCRYPT, 0x00, sizeof(dataENCRYPT));
				in_val = in_Cx_EDta(CVV,strlen(CVV),dataENCRYPT,&szdataENCRYPT);

			}

		//	if(in_val < SUCCESS)
		//	{
		//		LOGF_TRACE("*****FALLA AL ENCRIPTAR CVV *****");

		//	}

			if(in_val >FAILURE)
			{
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				memcpy(tmpbuf, "\xC1",1);
				tmpbuf[1] = szdataENCRYPT;
				memcpy(tmpbuf + 2, dataENCRYPT,szdataENCRYPT);
				vdSetByteValuePP((char*)"CVV2",tmpbuf,szdataENCRYPT + 2);
				mov += szdataENCRYPT + 2;
				LOGF_TRACE("*****MOV = %i *****", mov);
			
			}
			
		}
		else
		{
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			memcpy(tmpbuf, "\xC1",1);
			tmpbuf[1] = strlen(CVV);
			memcpy(tmpbuf + 2, CVV,strlen(CVV));
			vdSetByteValuePP((char*)"CVV2",tmpbuf,szCVV2 + 2);
			mov += strlen(CVV)+ 2;
			LOGF_TRACE("*****MOV = %i *****", mov);

		}
		
		
		
		//vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
		
	}
	
	//MSRead = in_txn_crd (trk1,sztrk1,trk2,sztrk2,CVV, szCVV2,MSR);
	// ponemos mi funcion para la encripcion con AES
	
	//build_C53_C54Offline();
	memset(LenHex,0x00,sizeof(LenHex));
	lenfld = 0;
	lenfld |= mov;	//length
	LenHex[1] = lenfld;
	lenfld = 0;
	lenfld |= mov >> 8;
	LenHex[0] = lenfld;
	LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",LenHex,sizeof(LenHex));			
	vdSetByteValuePP((char*)"LENGH",LenHex,2);
	return MSRead;
}


int EMV_Transaction_UNI()
{	
	int i = 0;
	int emvRes = 0;
	int res1stGenAC = 0;
	unsigned char ucReader=0;
	unsigned char inVAL   =0;
	//Valores que llegan del comado C31
	char Amount[14] = {0};
	int szAmount = 0;	
	char cashback[14] = {0};
       int szCashBack  = 0;	
	unsigned char txntype = 0;
	int sztxntype = 0;
	unsigned char Emv_txntype = 0;
	unsigned char currCode[2] ={0};
	int szcurrCode = 0;	
	unsigned char merch_destion = 0;
	int szmerch_destion = 0;

	//TYPE TXN
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[32];

	char buffASC[64] = {0};
	unsigned int Decimal = 0;
	char buffhex[10] = {0};
	
	int  inHostApproved = 0;

	map<string,string> value; 

	
	LOGF_TRACE("\nEMV Transaction");

	vdSetPinMsgs("PIN","INGRESE PIN:");

	// ACTUALIZAR EL MONTO PARA LA TRANSACCION ANTES DEL FIRST GENERATE
	szAmount = inGetByteValuePP((char*)"AMOUNT",(char*)Amount);
	LOGAPI_HEXDUMP_TRACE("Amount",Amount,szAmount);
	Decimal =  convert_ArrayHex2int(Amount,szAmount);
	LOGF_TRACE("Decimal = %lu\n",Decimal);
	sprintf(buffASC,"%12.12lu",Decimal);
	LOGF_TRACE("Decimal = %s\n\n",buffASC);
	vdAsc2Hex (buffASC,buffhex,6);
	LOGAPI_HEXDUMP_TRACE("buffer del monto",buffhex,sizeof(buffhex));
	memset(buffTAG, 0x00, sizeof(buffTAG));	
	memcpy(buffTAG,"\x9F\x02\x06",3);
	memcpy(buffTAG+3,buffhex,6);
	szTAG=9;
	LOGAPI_HEXDUMP_TRACE("TLV -> ",buffTAG,szTAG);
	inVAL = setCT_EMV_TAG(0,buffTAG,szTAG);
	LOGF_TRACE("inVAL -> [%i]",inVAL);

	szCashBack = inGetByteValuePP((char*)"CASHBACK",(char*)cashback);	
	LOGAPI_HEXDUMP_TRACE("CASHBACK",cashback,szCashBack);
	Decimal =  convert_ArrayHex2int(cashback,szCashBack);	
	LOGF_TRACE("Decimal = %lu\n",Decimal);
	memset(buffASC, 0x00, sizeof(buffASC));
	sprintf(buffASC,"%12.12lu",Decimal);
	LOGF_TRACE("Decimal = %s\n\n",buffASC);
	memset(buffhex,0x00,sizeof(buffhex));
	vdAsc2Hex (buffASC,buffhex,6);
	LOGAPI_HEXDUMP_TRACE("buffer de cash back",buffhex,sizeof(buffhex));
	memset(buffTAG, 0x00, sizeof(buffTAG)); 
	memcpy(buffTAG,"\x9F\x03\x06",3);
	memcpy(buffTAG+3,buffhex,6);
	szTAG=9;
	LOGAPI_HEXDUMP_TRACE("TLV -> ",buffTAG,szTAG);
	inVAL = setCT_EMV_TAG(0,buffTAG,szTAG);
	LOGF_TRACE("inVAL -> [%i]",inVAL);


	sztxntype = inGetByteValuePP((char*)"TRANSTYP",(char*)&txntype);
	szcurrCode = inGetByteValuePP((char*)"CURRCODE",(char*)&currCode);
	szmerch_destion = inGetByteValuePP((char*)"MERDESC",(char*)&merch_destion);


	LOGF_TRACE("Transaction type = %x",txntype);

	if((txntype == 0x00) ||(txntype == 0x01) || (txntype == 0x03)){
		LOGF_TRACE("Venta");
		Emv_txntype = EMV_ADK_TRAN_TYPE_GOODS_SERVICE;  //VENTA 
	}
	
	if((txntype == 0x02) || (txntype == 0x04)){
		LOGF_TRACE("devolucion");
		Emv_txntype = EMV_ADK_TRAN_TYPE_REFUND;			//DEVOLUCI�N
	}

	memset(cashback,0x00,sizeof(cashback));  // se limpia el campo de Cash back, ya que se suma al monto
						
	//Set_CT_TransactionData(&TxDataCT, (char *) AmountBCD,(char*)cashback,Emv_txntype);  //Setup CT transaction data
	
	//emvRes = EMV_AppSelection(&TxDataCT,&xSelectResCT);

	//Start the application Selection

//	switch(emvRes){

//		case EMV_ADK_OK:           // application selected, everything OK
//			LOGF_TRACE("\n----------Appl selection OK!");
				
			//EMV_Setup_TXN_CT_Data(&AdditionalTxDataCT,Amount, cashback, &TxDataCT);   // copy additional TXN Data, known after final select			
			emvRes = EMVFLOW_UNI(&TxDataCT,&xSelectResCT,&pxTransRes,&AdditionalTxDataCT, cmd_C31);
			LOGF_TRACE("\n *******emvRes **** = %X",emvRes);	

			LOGF_TRACE("\nemvRes = %i",emvRes);	

			if(emvRes == EMV_CT_PIN_INPUT_ABORT || emvRes == EMV_CT_PIN_INPUT_TIMEOUT)
				return emvRes;
			
			
	//	break;
	//	case EMV_ADK_FALLBACK:     // perform fallback to magstripe
			// Add reaction to local fallback rules or other local chip
	//		LOGF_TRACE("\n----------Appl selection FALLBACK! %#.2x", emvRes);
			//uiInvokeURL(1,"Fallback.html");
	//		uiInvokeURL("Fallback.html");
	//		sleep(2);
	//		return emvRes;
	//	break;
	//	case EMV_ADK_ABORT:
	//		LOGF_TRACE("\n----------Appl selection ABORT! %#.2x", emvRes);		   // no fallback, TRX definitely finished
	//		return emvRes;
	//	break;
	//	case EMV_ADK_APP_BLOCKED:  // application blocked
	//		LOGF_TRACE("\n----------Appl Blocked %#.2x", emvRes);		   // App blocked
	//		return emvRes;
	//	break;
	//	case EMV_ADK_NOAPP:        // no application found						
	//		LOGF_TRACE("\n----------No application found %#.2x", emvRes);		   //No application Found
	//		return emvRes;
	//	break;
	//	default:
			// please feel free to add any other text and reaction if there is any other local chip
	//		LOGF_TRACE("\n----------Appl selection UNKWN! %#.2x", emvRes);
			//EMV_EndTransactionCT("\nTransaction Error", ucReader, TRUE);					
	//		return EMV_ADK_ABORT;
	//	break;
	//	}


	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9C;
	getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

	if(buffTAG[2] == 0x20)
	{
		LOGF_TRACE("Devoluci�n");
		emvRes = EMV_ADK_ARQC;
	}
	
	
	//Check the result of First Gen AC	
	switch(emvRes){
		case EMV_ADK_ARQC:                // go online
			LOGF_TRACE("\n----------Tx needs to go online!");
			// memset((char*)&xOnlineInputCT,0,sizeof(xOnlineInputCT));
			LOGF_TRACE("desplegamos la informacion de la tarjeta");
			display_card_info();
			Info_TRX_CT_EMV();		

			//in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP);

			emvRes = SUCCESS_TRX;

			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			build_C30_C33_CHIP(C33);
			// HostResponse(&xOnlineInputCT);						 
						 
			//In xEMVTransRes are all the data needed for the online message
			// ***Call your host here and fill out xOnlineInputCT with the answer from the host						
			//emvRes = EMV_SecondGenerateAC(&xOnlineInputCT, &xEMVTransResCT);  //EMV transaction (handling of host response including 2nd cryptogram)
		break;
		case EMV_ADK_TC:                  // approved offline
			LOGF_TRACE("\n----------Tx approved offline!");
			Info_TRX_CT_EMV();		

			//in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP);			
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			build_C30_C33_CHIP(C33);			
			
		break;
		case EMV_ADK_FALLBACK:            // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
		   	LOGF_TRACE("\n----------Appl selection FALLBACK!");
			//uiInvokeURL(1,"Fallback.html");
			//uiInvokeURL(0,value,"Fallback.html");
			//sleep(2);
			//EMV_CTLS_SmartPowerOff(ucReader);
			//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
		   	//EMV_EndTransactionCT(FALLBACK_MESSAGE, ucReader, FALSE);
		   	return emvRes;
		case EMV_ADK_AAC:                 // Denied offline
			LOGF_TRACE("\n----------Tx Declined!");
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			if((xEMVTransResCT.T_9F27_CryptInfo & EMV_ADK_CARD_REQUESTS_ADVICE) == EMV_ADK_CARD_REQUESTS_ADVICE)
			{
				LOGF_TRACE("\nAdvice!!!");
				// vStoreAdviceData(xTrxRec); // store the advice data if it need and it'll be sent before reconcilation
			}
			Info_TRX_CT_EMV();
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
			
			// jbf 20171204 ADVT Tarjeta 4
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			build_C30_C33_CHIP(C33);
			
			
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			
//			if (inHostApproved == 0)
//			{
//				value["BODY_HEADER"]="DECLINADA";
//			}
//			else
			{
				value["BODY_HEADER"]="DECLINADA EMV";
			}
			value["BODY_MSG"]="";
			
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
			break;
		case EMV_ADK_AAR:
		case EMV_ADK_ABORT:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_CVM:
		case EMV_ADK_CARDERR_FORMAT:
		default:
			LOGF_TRACE(" *** DECLINADA****");
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			value["BODY_HEADER"]="DECLINADA  EMV";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINEEMV";
			
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
			//build_C53_C54Offline();	
			EMV_CTLS_SmartPowerOff(ucReader);
			EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
			
		return emvRes;
		
		}	
	
	return emvRes;

}


int EMV_Transaction_C30_UNI()
{	
	int i = 0;
	int emvRes = 0;
	int res1stGenAC = 0;
	unsigned char ucReader=0;

	int Remove_card = 0;	


	unsigned char Amount[14] = {0};
	int szAmount = 0;	
	
	unsigned int AmountAux = 0;
	unsigned char AmountBCD[14] = {0};
	int szAmountBCD = 0;
	
	unsigned char cashback[14] = {0};
	int szCashBack = 0; 
	unsigned int CashBackAux = 0;

	unsigned char txntype = 0;
	int sztxntype = 0;
	unsigned char Emv_txntype = 0;
	
	//TYPE TXN
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];

	LOGF_TRACE("\nEMV Transaction C30");

	LOGF_TRACE("\n\n");	

	LOGF_TRACE("Venta");
	Emv_txntype = EMV_ADK_TRAN_TYPE_GOODS_SERVICE;  //VENTA 
						
	Set_CT_TransactionData(&TxDataCT, (char *) AmountBCD,(char*)cashback,Emv_txntype);  //Setup CT transaction data
	
	emvRes = EMV_AppSelection(&TxDataCT,&xSelectResCT);

	Remove_card = _CHECK_INSERTED_CARD();
	if( (Remove_card == FALSE))
		return CARD_REMOVED;	

	//Start the application Selection

	switch(emvRes){

		case EMV_ADK_OK:           // application selected, everything OK
			LOGF_TRACE("\n----------Appl selection OK!");
				
			EMV_Setup_TXN_CT_Data(&AdditionalTxDataCT,Amount, cashback, &TxDataCT);   // copy additional TXN Data, known after final select			
			
			emvRes = EMVFLOW_UNI(&TxDataCT,&xSelectResCT,&pxTransRes,&AdditionalTxDataCT, cmd_C30);
			LOGF_TRACE("\nemvRes = %i",emvRes);		   
						
			
		break;
		case EMV_ADK_FALLBACK:     // perform fallback to magstripe
			// Add reaction to local fallback rules or other local chip
			LOGF_TRACE("\n----------Appl selection FALLBACK! %#.2x", emvRes);
			//uiInvokeURL(1,"Fallback.html");
			//uiInvokeURL(0,value,"Fallback.html");
			//sleep(2);
			break;
			//return emvRes;
		break;
		case EMV_ADK_ABORT:
			LOGF_TRACE("\n----------Appl selection ABORT! %#.2x", emvRes);		   // no fallback, TRX definitely finished
			return emvRes;
		break;
		
		case EMV_ADK_CARD_BLOCKED: // card blocked 
		case EMV_ADK_APP_BLOCKED:  // application blocked 
			LOGF_TRACE("\n----------Appl Blocked %#.2x", emvRes);		   // App blocked
			// jbf 20180122
			//return emvRes;
			return TARJ_BLOQUEADA;
			
		break;
		case EMV_ADK_NOAPP:        // no application found						
			LOGF_TRACE("\n----------No application found %#.2x", emvRes);		   //No application Found
			return emvRes;
		break;
		default:
			// please feel free to add any other text and reaction if there is any other local chip
			LOGF_TRACE("\n----------Appl selection UNKWN! %#.2x", emvRes);
			//EMV_EndTransactionCT("\nTransaction Error", ucReader, TRUE);					
			return EMV_ADK_ABORT;
		break;
		}
	
	if(emvRes== EMV_ADK_FALLBACK)
		{
			int MSRead = 0;
			
			Remove_card = _CHECK_INSERTED_CARD();
			if(Remove_card == FALSE)
				emvRes = EMV_ADK_ABORT;
			else {
				count_fallback_inc();
				emvRes = fallback_MSR();		
			}
			
			if((emvRes == CTS_OK) && (chTechnology == CTS_MSR))
			{
				LOGF_TRACE("Input from MAGNETIG STRIPE READER in fallback");
				emvRes = MSR_Transaction_UNI(MSRead);	
			}
	
		}

/*
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9C;
	getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

	if(buffTAG[2] == 0x20)
	{
		LOGF_TRACE("Devoluci�n");
		emvRes = EMV_ADK_ARQC;
	}
	
*/
	//Check the result of First Gen AC
	
//	switch(emvRes){
//		case EMV_ADK_ARQC:                // go online
//			LOGF_TRACE("\n----------Tx needs to go online!");
			// memset((char*)&xOnlineInputCT,0,sizeof(xOnlineInputCT));

			Info_TRX_CT_EMV();		

		//	in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP);

			//vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			LOGF_TRACE("**************************************************");
			build_C30_C33_CHIP(cmd_C30);

			//build_C53_C54Offline();
			// HostResponse(&xOnlineInputCT);						 
						 
			//In xEMVTransRes are all the data needed for the online message
			// ***Call your host here and fill out xOnlineInputCT with the answer from the host						
			//emvRes = EMV_SecondGenerateAC(&xOnlineInputCT, &xEMVTransResCT);  //EMV transaction (handling of host response including 2nd cryptogram)
//		break;
//		case EMV_ADK_TC:�                  // approved offline
/*
			LOGF_TRACE("\n----------Tx approved offline!");
			Info_TRX_CT_EMV();		

			in_txn_crd (NULL,0,NULL,0,NULL,0,CHIP);			
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			LOGF_TRACE("REVISAR QUE RESPUESTA SE MANDA EN EL C53");
			//build_C53_C54Offline();			
			
		break;
		case EMV_ADK_FALLBACK:            // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
		   	LOGF_TRACE("\n----------Appl selection FALLBACK!");
			//uiInvokeURL(1,"Fallback.html");
			uiInvokeURL("Fallback.html");
			sleep(2);
			//EMV_CTLS_SmartPowerOff(ucReader);
			//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
		   	//EMV_EndTransactionCT(FALLBACK_MESSAGE, ucReader, FALSE);
		   	return emvRes;
		case EMV_ADK_AAC:                 // Denied offline
			LOGF_TRACE("\n----------Tx Declined!");
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			if((xEMVTransResCT.T_9F27_CryptInfo & EMV_ADK_CARD_REQUESTS_ADVICE) == EMV_ADK_CARD_REQUESTS_ADVICE)
			{
				LOGF_TRACE("\nAdvice!!!");
				// vStoreAdviceData(xTrxRec); // store the advice data if it need and it'll be sent before reconcilation
			}
			Info_TRX_CT_EMV();
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
		break;
		case EMV_ADK_AAR:
		case EMV_ADK_ABORT:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_CVM:
		case EMV_ADK_CARDERR_FORMAT:
		default:
			EMV_EndTransactionCT("DECLINADA", ucReader, TRUE);
			vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
			//build_C53_C54Offline();	
			//EMV_CTLS_SmartPowerOff(ucReader);
			//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
		return emvRes;
		}
*/
	LOGF_TRACE("**************************************************",emvRes);
	return emvRes;

}

int bo_crp_crd (char pch_pan [], int in_pan)		
{
	int bo_val;

	LOGF_TRACE("bo_crp_crd");
	
	bo_val = FALSE;

	if ( in_Cx_fkld () )
	{
		bo_val = TRUE;

		if ( inCrpExcCrd (pch_pan, in_pan) == VS_SUCCESS )
		{
			bo_val = FALSE;
		}
	}

	LOGF_TRACE("bo_crp_crd [%s]" ,(bo_val? ":::::: BIN Encrypt ::::::::":"::::::::: BIN Exception!!!!!!!!"));
	LOGF_TRACE("bo_val=%d", bo_val);

	return bo_val;
}

bool isExceptionBin(char pch_pan [], int in_pan)		
{
	bool isException;

	LOGF_TRACE("isExceptionBin");
	
	isException = false;
	LOGF_TRACE("bo_val=%d", isException);

	if ( inCrpExcCrd (pch_pan, in_pan) == VS_SUCCESS )
	{
		isException = true;
	}
	LOGF_TRACE("bo_val=%d", isException);

	LOGF_TRACE("isExceptionBin [%s]" ,(isException? ":::::: it is ExceptionBin ::::::::":"::::::::: it is not Exception BIN::::"));
	

	return isException;
}





int build_C30_C33_CHIP(char cmd_asm)
{
	int iResult = 0;
	char dataENCRYPT[512]  ;
	int szdataENCRYPT =  0 ;
	int in_val ;
	//Variables para armar el campo
	char tmpbuff[512] = {0};
	int sztmp =0;
	int mov = 0;
	char LenHex[3]={0};
	char lenfld = 0;
	char aux = 0;

	unsigned char ch_val[4] = {0};

	char ch_cvmend [10]= {0}; 					
	int  in_cvmend = 0;

	char toke1[1024] = {0};
	int sztoke1 = 0;	
	char toke2[1024] = {0};
	int sztoke2 = 0;

	//PAra verificar tipo de CTLS
	int typctls = 0;
	//Variables necesarias para EMV CHIP y CTLS CHIP
	unsigned long TAG = 0;
	unsigned char buffTAG[50] = {0};
	unsigned short szTAG = 0;
	int tech = 0;

	
	LOGF_TRACE("--build_C30_C33-chip--");	
	LOGF_TRACE("-- CMD-ASM  : [%x]--",cmd_asm);	

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		if(chTechnology == CTS_CHIP)
		{
			LOGF_TRACE("C3X Para EMV Chip");
			tech = CTS_CHIP;
		}
		else if(chTechnology == CTS_CTLS)
		{
			LOGF_TRACE("C30 Para CTLS CHIP");
			tech = CTS_CTLS;
		}
		
		vdSetByteValuePP((char*)"STATUS",(char *)"00",2);

		if(cmd_asm == cmd_C30)
		{
		
			// TAG 4F
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x4F;
			iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				if(szTAG > 2)
				{
					LOGAPI_HEXDUMP_TRACE("AID",buffTAG,szTAG);
				}
			
					
				else
				{
					if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
					{
						LOGAPI_HEXDUMP_TRACE("AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
						memset(tmpbuff, 0x00, sizeof(tmpbuff));
						memcpy(tmpbuff,"\x4F",1);
						sztmp |= pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
						LOGF_TRACE("SIZE TAG 4F AID : %i",sztmp);
						tmpbuff[1] = sztmp;
						memcpy(tmpbuff + 2,(char *)pxTransRes.xEMVTransRes.T_9F06_AID.AID,sztmp  + 2);
					
						vdSetByteValuePP((char*)"TAG4F",tmpbuff,sztmp + 2);
						mov += sztmp+ 2;
					}
				
					else
						LOGF_TRACE("\nAID is not available");
				} 

			}
		
			else
			{
				LOGF_TRACE("\nAID is not available");

			}
	
			LOGF_TRACE("*****MOV = %i *****", mov);

			//TAG 9F12
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x9F12;
			iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				LOGAPI_HEXDUMP_TRACE((char *)"Application Preferred Name",buffTAG,szTAG);
			}
			else
				LOGF_TRACE("\nApplication Preferred Name is not available");	

			sztmp = 0;
			sztmp |=szTAG;
			vdSetByteValuePP((char*)"TAG9F12",(char *)buffTAG,sztmp);
			mov 	+= sztmp;
			LOGF_TRACE("*****MOV = %i *****", mov);
			
			//TAG 50
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x50;
			iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
				{
					LOGAPI_HEXDUMP_TRACE((char *)"Application Level",buffTAG,szTAG);
				
				}
			else
				LOGF_TRACE("\nApplication Level is not available");

			sztmp = 0;
			sztmp |=szTAG;	
			vdSetByteValuePP((char*)"TAG50",(char *)buffTAG,sztmp);
			mov += sztmp;
			LOGF_TRACE("*****MOV = %i *****", mov);

			LOGF_TRACE("ARMADO DE RESPUESTA C30");
			memset(LenHex,0x00,sizeof(LenHex));
			lenfld = 0;
			lenfld |= mov;	//length
			LenHex[1] = lenfld;
			lenfld = 0;
			lenfld |= mov >> 8;
			LenHex[0] = lenfld;
			LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",LenHex,sizeof(LenHex));			
			vdSetByteValuePP((char*)"LENGH",LenHex,2);
			return iResult;	
	
		}

		else if(cmd_asm == C33)
		{
			
			// metemos la validacion de los  bines de excepcion como en la 820
			memset(ch_val,0x00,sizeof(ch_val));
			memcpy(ch_val,"\xC1\x01",2);
			if (bo_crp_chp())
			{
				ch_val[2] = 0x01;
				vdSetByteValuePP((char*)"FLAG_ENCRYPT_DATA",(char *)ch_val,3);
			}
			
			mov += 3;
			LOGF_TRACE("*****MOV = %i *****", mov);
			//TOKEN E1
			asm_tke1(toke1,&sztoke1,C33);
			mov+=sztoke1;
			//TOKEN E2
			asmtknE2(toke2,&sztoke2,C33);
			mov+=sztoke2;
			
			vdSetByteValuePP((char*)"TOKE1",toke1,sztoke1); 
			vdSetByteValuePP((char*)"TOKE2",toke2,sztoke2); 	

			memset(tmpbuff, 0x00, sizeof(tmpbuff));
			memcpy(tmpbuff,"\xC1\x00",2);
			vdSetByteValuePP((char*)"DUMMY1",tmpbuff,2);
			mov += 2 ;
			LOGF_TRACE("*****MOV = %i *****", mov);

			memset(tmpbuff, 0x00, sizeof(tmpbuff));
			memcpy(tmpbuff,"\x99\x00",2);
			vdSetByteValuePP((char*)"TAG99",tmpbuff,2);
			
			// jbf 20171230 looks like an error
			//mov += 3 ;
			mov += 2 ;
			
			LOGF_TRACE("*****MOV = %i *****", mov);

	
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x5F24;
			iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
			if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
				LOGAPI_HEXDUMP_TRACE("EXP_DATE",buffTAG,szTAG);
			else
				LOGF_TRACE("\n EXP DATE ");
	
			sztmp = 0;
			sztmp |=szTAG;
			memset(tmpbuff, 0x00, sizeof(tmpbuff));
			vdHex2Asc((char *)buffTAG + 3 ,tmpbuff + 2,2);
			memcpy(tmpbuff,"\xC1",1);
			tmpbuff[1] = 4;
			vdSetByteValuePP((char*)"EXP_DATE",(char *)tmpbuff,4 + 2);
			mov += 4 + 2 ;
			LOGF_TRACE("*****MOV = %i *****", mov);

			memset(LenHex,0x00,sizeof(LenHex));
			lenfld = 0;
			lenfld |= mov;	//length	
			LenHex[1] = lenfld;
			lenfld = 0;
			lenfld |= mov >> 8;
			LenHex[0] = lenfld;
			LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",LenHex,sizeof(LenHex));			
			vdSetByteValuePP((char*)"LENGH",LenHex,2);
			return iResult;	
		}


   }
	
	return iResult;	
}


unsigned char EMVFLOW_UNI(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType,char cmd_process)
{

	unsigned char eEMVInfo;
	int Remove_card = 0;
	int iResult = 0;
	unsigned char buffTAG[15] = {0};  // FAG 18-SEP-2017
	char buffTAGASC[12]={0};
	unsigned long TAG = 0;// FAG 18-SEP-2017 
	unsigned short szTAG = 0; // FAG 18-SEP-2017   
	map<string,string> value;  // FAG 22-SEP-2017
	LOGF_TRACE("********EMVFLOW- UNI**********");

	if(cmd_process == 0)
	{
		eEMVInfo = EMV_ReadRecords(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType); 
		LOGF_TRACE("ReadRecords");
		LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
		LOGF_TRACE("\neEMVInfo = %i",eEMVInfo);
		Remove_card = _CHECK_INSERTED_CARD();
		// jbf 20171211
		//if( (Remove_card == FALSE))
		if( eEMVInfo == EMV_ADK_ABORT )
			return EMV_ADK_FALLBACK;
		if( (Remove_card == FALSE) || (eEMVInfo == EMV_ADK_ABORT) )
			return CARD_REMOVED;	


		// METEMOS LA VALIDACION DE LA FECHA DE LA TARJETA CUANDO ES CHIP
		TAG = 0x5F24;
		iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			LOGAPI_HEXDUMP_TRACE("EXP DATE",buffTAG,szTAG);
			vdHex2Asc((char *)buffTAG+3,buffTAGASC,2);
			LOGAPI_HEXDUMP_TRACE("EXP DATE ASC",buffTAGASC,sizeof(buffTAGASC));
			iResult=incheck_expdate(buffTAGASC);
			LOGF_TRACE("** iResult  = %i**",iResult);
			if(iResult<0)
			{
				LOGF_TRACE("\nTARJETA EXPIRADA");
				//value["BODY_MSG"]="TIEMPO EXCEDIDO";
				//value["NAME_IMG"]="EXTLOADKEY";
				value["HEADER_TITLE"]="";
				value["BODY_HEADER"]="TARJETA EXPIRADA";
				value["BODY_MSG"]="";
				value["NAME_IMG"]="EXTCARDEXPIRED";
				
				uiInvokeURL(value,"GenScreen_alert.html");
				
				sleep(SHOW_ALERT);
				
				value["HEADER_TITLE"]="COMANDO";
				value["BODY_HEADER"]="PROCESANDO";
				value["BODY_MSG"]="ESPERE UN MOMENTO";
				
				uiInvokeURL(value,"Procesando.html");

				sleep(SHOW_ALERT);
			}
	
	}
	}

	else
	{
    		eEMVInfo = EMV_DataAuthentication(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
		LOGF_TRACE("DataAuthentication");
		LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
		Remove_card = _CHECK_INSERTED_CARD();
		if(Remove_card == FALSE)
			return CARD_REMOVED;	
		
		eEMVInfo = EMV_CardHolderVerification(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
		LOGF_TRACE("CardHolderVerification");
		LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
		Remove_card = _CHECK_INSERTED_CARD();
		if(Remove_card == FALSE)
			return CARD_REMOVED;

		if(eEMVInfo == EMV_CT_PIN_INPUT_ABORT)
		{
			LOGF_TRACE("\n Pin cancelado por el usuario");
			return EMV_CT_PIN_INPUT_ABORT;
		}

		if(eEMVInfo == EMV_CT_PIN_INPUT_TIMEOUT)
		{
			LOGF_TRACE("\n Pin cancelado por Timeout");
			return EMV_CT_PIN_INPUT_TIMEOUT;
		}

		if(eEMVInfo != EMV_ADK_APP_REQ_CVM_END)
		{
			eEMVInfo = EMV_CardHoldVerificationProcess(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
			LOGF_TRACE("CardHoldVerificationProcess");	
			LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
			Remove_card = _CHECK_INSERTED_CARD();
			if(Remove_card == FALSE)
				return CARD_REMOVED;	
		}

		eEMVInfo = EMV_RiskManagement(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
		LOGF_TRACE("RiskManagement");
		LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
		Remove_card = _CHECK_INSERTED_CARD();
		if(Remove_card == FALSE)
			return CARD_REMOVED;	

		eEMVInfo = EMV_FirstGenerateAC(xEMVStartData,xSelectRes,xTrxRec,xEMVTransType);
		LOGF_TRACE("FirstGenerateAC");	
		LOGF_TRACE("eEMVInfo = %X",eEMVInfo);
		Remove_card = _CHECK_INSERTED_CARD();
		if(Remove_card == FALSE)
			return CARD_REMOVED;	
	}

	return eEMVInfo;
}



int build_C34()
{
	int iResult = 0;
	char LengthC34[2] = {0};
	int szLengthC34 = 0;
	char Tkn[512] = {0};
	int szTkn= 0;

	int szCmd = 0;
	int typctls = 0;
	int i = 0;	
	int statuscard = 0;
	map<string,string> value; 

	LOGF_TRACE("--build_C34--");

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
	
		LOGF_TRACE("Response for EMV or CTLS CHIP");			
		if(chTechnology == CTS_CHIP)
		{
			statuscard = _CHECK_INSERTED_CARD();
			LOGF_TRACE("Tarjeta insertada? %X",statuscard);

			if(!statuscard)
			{
				LOGF_TRACE("Se ha removido la tarjeta");
				memset(Tkn,0x00,sizeof(Tkn));
				szTkn = 0;
				szCmd+=szTkn; 
				//uiInvokeURL(1,"tarjeta_removida.html");
				value["BODY_HEADER"]="TARJETA REMOVIDA";
				value["BODY_MSG"]="verifique por favor";
				
				uiInvokeURL(value,"GenScreen_card_removed.html");

				sleep(SHOW_ALERT);

			}
			else
			{
				//Token
				asmtkn(Tkn,&szTkn);
				szCmd+=szTkn; 	
			}
							
		}
		else
		{
			
			//Token
			asmtkn(Tkn,&szTkn);
			szCmd+=szTkn; 	
		}	
	}
	if( (chTechnology == CTS_MSR || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) )  || chTechnology == CTS_KBD))
	{
		LOGF_TRACE("Response for MSR or CTLS MSR");
		szTkn = 2;
		memcpy(Tkn,"\xE2\x00",szTkn);
		szCmd += szTkn;
	}	  	

	//Calculamos la longiutd de todo el campo C34
	
	if(chTechnology == CTS_CHIP)
	{
		statuscard = _CHECK_INSERTED_CARD();
		LOGF_TRACE("Tarjeta insertada? %X",statuscard);
		if(!statuscard)
		{
			LengthC34[1] = 0x00;
			LengthC34[0] = 0x00;
			szLengthC34 = 2;
		}
		
		else
		{
			LengthC34[1] = szCmd & 0x000000FF;
			LengthC34[0] = szCmd & 0x0000FF00;	
			szLengthC34 = 2;
		}
	}
	else
	{
		LengthC34[1] = szCmd & 0x000000FF;
		LengthC34[0] = szCmd & 0x0000FF00;	
		szLengthC34 = 2;
	}

	vdSetByteValuePP((char*)"TOKE1",Tkn,szTkn);
	vdSetByteValuePP((char*)"LENGH",LengthC34,szLengthC34);
	 
	return iResult;
}


//2 FUNCION PARA ACTUALIZAR UN TAG EMV

unsigned char setCT_EMV_TAG(unsigned long options, unsigned char *tlvBuffer, unsigned short bufferLength)
{

unsigned char iResult;
iResult = EMV_CT_updateTxnTags (options, tlvBuffer,bufferLength);
return iResult;
}


void asmtkn(char* TOKEN_out,int* szTOKEN_out)
{
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100] = {0};
	int iResult = 0;

	// TAG 95
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Terminal Verification Result",buffTAG,szTAG);
		memcpy(TOKEN_out + (*szTOKEN_out) , buffTAG , szTAG);
		*szTOKEN_out  +=  szTAG;
	}
	else
	{
		LOGF_TRACE("\nTerminal Verification Result is not available");

	}	

	//TAG 9B
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE((char *)"Transaction Status Information",buffTAG,szTAG);
		memcpy(TOKEN_out + (*szTOKEN_out) , buffTAG , szTAG);
		*szTOKEN_out  +=  szTAG;

	}
	else
	{
		LOGF_TRACE("\nTransaction Status Infortation is not available");

	}

	//TAG 9F27	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Cryptogram information Data",buffTAG,szTAG);
		memcpy(TOKEN_out + (*szTOKEN_out) , buffTAG , szTAG);
		*szTOKEN_out  +=  szTAG;

	}
	else
	{
		LOGF_TRACE("\nCryptogram information Data is not available");

	}

	
	//TAG 8A
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE((char *)"Authorization Response Code",buffTAG,szTAG);
		memcpy(TOKEN_out + (*szTOKEN_out) , buffTAG , szTAG);
		*szTOKEN_out  +=  szTAG;

	}
	else
	{
		LOGF_TRACE("\nAuthorization Response Code is not available");

	}
	
	
	// C2 
	memcpy(TOKEN_out + (*szTOKEN_out) , "\xC2\x01\x00" , 3);
	*szTOKEN_out  +=  3 ;
	
	LOGF_TRACE("sizeof token  = %i",*szTOKEN_out);
	LOGAPI_HEXDUMP_TRACE((char *)"E1 trace -> ",TOKEN_out,*szTOKEN_out);	
	LOGF_TRACE("Done asmtkn");

}

 int bo_crp_chp (void)		// AJM 26/08/2014 1
{
	char ch_hex [50] = {0};
	ushort us_hex = 0;
	int  bo_val = FALSE;
	int  in_ret;
	unsigned long TAG = 0;
	unsigned char buffTAG[50] = {0};
	unsigned short szTAG = 0;
	int iResult = 0;

	LOGF_TRACE("bo_crp_chp");

	// meter logica de chip. ctls, banda o manual

	
	//in_ret = usEMVGetTLVFromColxn_0 (TAG_57_TRACK2_EQ_DATA, (uchar *) ch_hex, &us_hex);
	//TAG 57
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x57;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE((char *)"Track 2 Equivalent Data",buffTAG,szTAG);	
		
	}
	else
	{
		LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
	
	}
	
	if (iResult == SUCCESS)
	{
		char ch_tk2 [50] = {0};
		int  in_tk2 = 0;
		char * pch_equ = NULL;
		in_tk2 = inHexToAsc ((char *)buffTAG + 2, ch_tk2, szTAG - 2);

		pch_equ = (char *)memchr (ch_tk2, 'D', in_tk2);

		if (pch_equ)
		{
			*pch_equ = 0;

			in_tk2 = pch_equ - ch_tk2;
		}

		LOGAPI_HEXDUMP_TRACE("TRACK 2 antes de bo_crp_crd",ch_tk2,in_tk2);
		LOGF_TRACE("--valor de in_tk2  [%i]--",in_tk2);
		bo_val = bo_crp_crd (ch_tk2, in_tk2);
		
	}

	LOGF_TRACE("bo_val=%d", bo_val);

	return bo_val;
}



bool display_card_info(void)
{ 
	
	unsigned long TAG = 0;
	unsigned char buffTAG[50] = {0};
	unsigned short szTAG = 0;
   	int iResult = 0 ; 
	char buffASC[50] = {0};
	char ch_tk2 [50] = {0};
	int  in_tk2 = 0;
	char tmpbuff[32] ={0};
	char *pch_equ = NULL;
	char icon_name[20]= {0};
	int  inGetTag50=0;
	map<string,string> value;
		
	 //TAG 57
   	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x57;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		   LOGAPI_HEXDUMP_TRACE((char *)"\nTrack 2 Equivalent Data",buffTAG,szTAG);    
		   
	}
	else
	{
		   LOGF_TRACE("\nTrack 2 Equivalent Data is not available");
	}
	   
	iResult = inHex2Asc (( char *)buffTAG + 2, buffASC, szTAG - 2);
	LOGAPI_HEXDUMP_TRACE("Track 2 in ASC",buffASC , sizeof(buffASC));
	LOGF_TRACE("-- in_asc [%i]--", iResult);
	pch_equ = (char *)memchr (buffASC, 'D',iResult );

	if (pch_equ)
	{
			*pch_equ = 0;

			in_tk2 = pch_equ - ch_tk2;
	}

	value["MASKPAN"]=maskPan(buffASC);

	
	//TAG 9F12
	inGetTag50 = 0;
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F12;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		//if (szTAG > 2) {
		if (strlen((char *)buffTAG) > 2) {	
			LOGAPI_HEXDUMP_TRACE((char *)"Preferred Name",buffTAG,szTAG);
			value["TYPE_CARD"].assign((char *)buffTAG + 2);
			LOGF_TRACE("  Preferred Name : %s", value["TYPE_CARD"].c_str());
		}
		else {
			inGetTag50 = 1;
		}
	}
	if (inGetTag50 == 1) {
		LOGF_TRACE("\nPreferred Name is not available");
	
		//TAG 50
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x50;
		iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology );
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			LOGAPI_HEXDUMP_TRACE((char *)"Application label",buffTAG,szTAG);
		}
		else {
			LOGF_TRACE("\nApplication label is not available");
		}
		value["TYPE_CARD"].assign((char *)buffTAG + 2);
		LOGF_TRACE("  Application label : %s", value["TYPE_CARD"].c_str());
	}
	//value["TYPE_CARD"].assign((char *)buffTAG + 2);
 	//LOGF_TRACE("  Application Level : %s", value["TYPE_CARD"].c_str());

	
	// TAG 4F
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		if(szTAG > 2)
		{
			LOGAPI_HEXDUMP_TRACE("AID",buffTAG,szTAG);
		}
				
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
			{
				LOGAPI_HEXDUMP_TRACE("AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				//sztmp |= pxTransRes.xEMVTransRes.T_9F06_AID.aidlen;
				LOGF_TRACE("SIZE TAG 4F AID : %i",pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				memcpy(tmpbuff ,(char *)pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				LOGAPI_HEXDUMP_TRACE("AID display card_info",tmpbuff,sizeof(tmpbuff));
				if(!memcmp(tmpbuff, "\xA0\x00\x00\x00\x03",5))
				{
					LOGF_TRACE("Tarjeta visa");
					memcpy(icon_name,"visa",4);
				}
				else if(!memcmp(tmpbuff, "\xA0\x00\x00\x00\x04",5))
				{
					LOGF_TRACE("Tarjeta mastercard");
					memcpy(icon_name,"mastercard",10);
				}
				else if(!memcmp(tmpbuff, "\xA0\x00\x00\x00\x25",5))
				{
					LOGF_TRACE("Tarjeta amex");
					memcpy(icon_name,"amex",4);
				}
			
			
				
			}
				
			else
			{
				LOGF_TRACE("\nAID is not available");

			}
			
		}
	}
	
	else
	{
		LOGF_TRACE("\nAID is not available");
	}	
		
	value["BODY_HEADER"] = "NUMERO DE CUENTA";
 	value["ICON"].assign(icon_name);
	LOGF_TRACE("  ICONO : %s", value["ICON"].c_str());
	
	uiInvokeURL(value,"GenScreen_card_info.html");

	sleep(SHOW_ALERT);

}


std::string maskPan(char pan[21])
{

 	std::string maskedpan="",strpan="";
 	strpan.insert(0,pan);
 	//memset(&pan[0],0,sizeof(pan));
 	LOGAPI_HEXDUMP_TRACE((char *)"PAN maskPan",pan,21); 
 	for(int i=0;i<=(strpan.length()-4);i++)
  	{
 		 maskedpan = maskedpan + "*";
 	 }
 	for(int i=(strpan.length()-4);i<strpan.length();i++)
 	 {
  		maskedpan = maskedpan + strpan[i];
 	 }
 	return maskedpan;
}



int fallback_MSR()
{
	int in_val = FALLA_CHIP;
	int inFlagManual = 0;   // se deshabilita el teclado
	unsigned char Tech_selected = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	int inTimeOut = 15;
	unsigned char dataBuffer[2056] = {0};
	unsigned short dataBufferLength = sizeof(dataBuffer);
	map<string,string> value;
	
	LOGF_TRACE("\nFALLBACK");

	
	value["HEADER_TITLE"]="";
	value["BODY_HEADER"]="FALLA EN CHIP";
	value["BODY_MSG"]="";
	value["NAME_IMG"]="EXTCHIPERROR";
	
	uiInvokeURL(value,"GenScreen_alert.html");
	sleep(SHOW_ALERT);
		
	Remove_Card();
	sleep(SHOW_ALERT);
	
	LOGF_TRACE("count_fallback = [%d]", count_fallback_get());
	
	//if(count_fallback>2)
	if(count_fallback_get() > 1)
	{
		in_val = Detect_input(CTS_MSR, &Tech_selected, inFlagManual,inMin, inMax, chManual, inTimeOut,
		dataBuffer,&dataBufferLength,(char*)" ",(char*)" ",(char *)" ");
		chTechnology = Tech_selected;
		//LOGF_TRACE("count_fallback = 0");
		//count_fallback=0;
	}

	return in_val;

}


unsigned long convert_ArrayHex2int(char* hex,int szhex)
{
	unsigned long Dec = 0;
	int i = 0;

	for(i = 0;i < szhex ; i++)
	{
		Dec |= (unsigned char)hex[i];		
		if(i < (szhex - 1) )
		{
			LOGF_TRACE("Decimal = %lu\n",Dec);
			Dec <<= 8;
		}
	}
	return Dec;	
}


bool checkLuhn(const std::string &acctNumber)
{
    int acctLen = 0, index = 0;
    unsigned char chkSum = 0, value = 0;

    acctLen = acctNumber.length();

    for (index = (acctLen - 2); index >= 0; index -= 2) {
        value = (acctNumber[index] - '0') * 2;
        if (value >= 10)
            chkSum += (value % 10) + 1;
        else
            chkSum += value;

        if ((index - 1) < 0)
            break;
        else
            chkSum += (acctNumber[index - 1] - '0');
    }

    if (chkSum % 10)
        chkSum = ((chkSum / 10) + 1) * 10 - chkSum;
    else
        chkSum = 0;

    if ((acctNumber[acctLen - 1] - '0') == chkSum)
        return true;

    return false;
}

bool check_service_code(char data_input[])
{
	LOGF_TRACE("check_service_code");
	//if((data_input[0]==0x32||data_input[0]==0x36) && count_fallback<=3)
	if((data_input[0]==0x32||data_input[0]==0x36) && count_fallback_get() < 2)
		return false;
	return true;
}

int incheck_expdate(char *inputASC)  // FAG 18-SEP-2017
{
	char chActualDate[10] = {0};
	char chAnioActual[3] = {0};
	char chMesActual[3] = {0};
	int inyearact=0;
	int inmonthact=0;
	int inyeardate=0;
	int inmonthdate=0;
	map<string,string> value;

	LOGF_TRACE("*** incheck_expdate ***");
	vdGetDate((unsigned char*) chActualDate, sizeof(chActualDate));
	sprintf(chAnioActual,"%02x",chActualDate[0]);
	sprintf(chMesActual,"%02x",chActualDate[1]);
	LOGF_TRACE("******ANIO ACTUAL [%s]**********\n",chAnioActual);
	LOGF_TRACE("******MES ACTUAL [%s]**********\n",chMesActual);
	LOGF_TRACE("******DATO  INGRESADO [%s]**********\n",inputASC);
	memset(chActualDate,0x00,sizeof(chActualDate));
	memcpy(chActualDate,inputASC,2);
	inyeardate=atoi(chActualDate);
	memcpy(chActualDate,inputASC+2,2);
	inmonthdate=atoi(chActualDate);	
	inyearact=atoi(chAnioActual);
	inmonthact=atoi(chMesActual);
	LOGF_TRACE("******ANIO ACTUAL entero [%i]**********",inyearact);
	LOGF_TRACE("******MES ACTUAL entero [%i]**********",inmonthact);
	LOGF_TRACE("******ANIO tarjeta entero [%i]**********",inyeardate);
	LOGF_TRACE("******MES tarjeta entero [%i]**********",inmonthdate);
	if(inyearact>inyeardate)
	{
		return -1;
	}
	else if(inyeardate>inyearact)
	{
		return 0;
	}
	else if(inyeardate==inyearact)
	{
		if(inmonthdate>inmonthact)
		{
			return 1;
		}
		else if(inmonthact>inmonthdate)
		{
			return -2;
		}
		else if(inmonthact==inmonthdate)
		{
			return 2;
		}
	}
}

// jbf 20180103
bool getSerialLast7(char *sn7) 
{
	string serial_number;
	char POSSN[16+1];

	//Numero de Serie
	sysGetPropertyString(SYS_PROP_HW_SERIALNO,serial_number);
	memset(POSSN, 0x00, sizeof(POSSN)); 
	strcpy(POSSN,serial_number.c_str());
	purge_char(POSSN, '-'); 
	memcpy(sn7, &POSSN[2], 7);
	return true;
}


int CTLS_ON(void)		//Daee 18/11/2015
{
	char tmp[2]={0};
	int inRet = false;

	LOGF_TRACE ("-- CTLS_ON --");

	if( inGetEnvVar((char*)"CTLSON", NULL) )
	{
		LOGF_TRACE("\nCtls ON !!");
		inRet = true;
	}
	else 
	{
		LOGF_TRACE("\nCtls OFF !!!");		
		inRet = false;
	}

	return inRet;
}

unsigned char set_EMV_TAG(unsigned long options, unsigned char *tlvBuffer, unsigned short bufferLength,int tech)
{

	unsigned char iResult;

	if(tech == CTS_CHIP)
	iResult = EMV_CT_updateTxnTags (options, tlvBuffer,bufferLength);
	else if (tech == CTS_CTLS)
	//iResult = EMV_CTLS_fetchTxnTags  (options,requestedTags,noOfRequestedTags,tlvBuffer,bufferLength,tlvDataLength);


return iResult;
}

bool isMerchantExceptionBINRange(char accountNumber[], int accountNumberLength){
	string accountNum(accountNumber,6);
	
	LOGF_TRACE ("-- isMerchantExceptionBINRange --");
	string line;
	std::stringstream fileContent;
	std::ifstream exceptionBINFile ("./flash/themes/themes/Aboutsoft/exceptionRange.txt");
	bool isExceptionBIN=false;
	int a=0,b=0;

	LOGF_TRACE("AccountNumber: %s",accountNum.c_str());
	//Rodo: Por si Gil exige que se soporten dos digitos como BIN de excepción, aqui hay que hacer el cambio. NRC

	if (exceptionBINFile.is_open()){
		while ( getline (exceptionBINFile,line) )
		{
			sscanf(line.c_str(),"%d-%d",&a,&b);
			LOGF_TRACE("line %s",line.c_str());
			LOGF_TRACE("Rango:%d-%d",a,b);
			if(atoi(accountNum.c_str()) >= a && atoi(accountNum.c_str()) <= b){
				isExceptionBIN=true;
				break;
			}
			fileContent << line << '\n';
		}
		exceptionBINFile.close();
	}else{
		fileContent << "Unable to open file"; 
	} 

	LOGF_TRACE("File: %s",fileContent.str().c_str());
	LOGF_TRACE("isExceptionBin [%s]" ,(isExceptionBIN? ":::::: it is MerchantExceptionBin ::::::::":"::::::::: it is not MerchantException BIN::::"));
	return isExceptionBIN;

}

bool isMerchantExceptionBIN(char accountNumber[], int accountNumberLength){
	string accountNum(accountNumber,6);
	LOGF_TRACE ("-- isMerchantExceptionBIN --");
	string line;
	std::stringstream fileContent;
	std::ifstream exceptionBINFile ("./flash/themes/themes/Aboutsoft/exception.txt");
	bool isExceptionBIN=false;

	LOGF_TRACE("AccountNumber: %s",accountNum.c_str());
	LOGF_TRACE("std::string::npos: ", std::string::npos);
					
	//Rodo: Por si Gil exige que se soporten dos digitos como BIN de excepción, aqui hay que hacer el cambio. NRC
	if (exceptionBINFile.is_open()){
		while ( getline (exceptionBINFile,line) )
		{
			if(line.find(accountNum)!=std::string::npos){
	
					
				isExceptionBIN=true;
				break;
			}
			fileContent << line << '\n';
		}
		exceptionBINFile.close();
		}
	else{
		fileContent << "Unable to open file"; 
	} 

	LOGF_TRACE("File: %s",fileContent.str().c_str());
	LOGF_TRACE("isExceptionBin [%s]" ,(isExceptionBIN? ":::::: it is MerchantExceptionBin ::::::::":"::::::::: it is not MerchantException BIN::::"));

	return isExceptionBIN;
}

int AddTag9F6E(char* pTokenBuffer, int inForce)
{
	unsigned char lBufferTag[100] = {0};
	unsigned short lTagLength = 0;
	unsigned long lTagName = 0;
	int lResult = 0;
	int inLenTlv = 0;

	//if(inVtaCtls==1)
	{
		lTagName = 0x9F6E;
		memset(lBufferTag,0x00,sizeof(lBufferTag));
		lResult = getCT_EMV_TAG(0, &lTagName, 1, lBufferTag, sizeof(lBufferTag), &lTagLength, chTechnology);

		inLenTlv = (int) lBufferTag[2]; 

		if( (lResult == EMV_ADK_OK) && (lTagLength > 0) )
		{
			LOGF_TRACE("chTechnology = %i", chTechnology);
			if(chTechnology != CTS_CTLS)
			{
				lTagLength = 3;
				memcpy(pTokenBuffer, "\x9F\x6E\x00", lTagLength);
			}
			else
			{
				inLenTlv = (int) lBufferTag[2];
				if(inLenTlv > 0)
					memcpy(pTokenBuffer, lBufferTag, lTagLength);
				else
				{
					if(inForce)
					{
						lTagLength = 3;
						memcpy(pTokenBuffer, "\x9F\x6E\x00", lTagLength);
					}
					else
						return 0;
				}

			}

			LOGAPI_HEXDUMP_TRACE((char *)"Form Factor Indicator", lBufferTag, lTagLength);
			LOGF_TRACE("size of 9F6E = %i", lTagLength);
			return lTagLength;
		}

		LOGF_TRACE("\nForm Factor Indicator is not available");
	}

	return 0;
}

void vdGetFFIMC(char *ch9F6E,char *chFFI)
{

	char chSub1[2] = {0};
	char chSub2[2] = {0};
	char chTemp[10] = {0};
	char chDeviceType[2] = {0};  
	

	LOGAPI_HEXDUMP_TRACE("FFI MC",ch9F6E,8);
	LOGAPI_HEXDUMP_TRACE("chFFI",chFFI,8);

	memcpy(chSub1,ch9F6E,2);
	memcpy(chSub2,&ch9F6E[2],2);
	memcpy(chDeviceType,&ch9F6E[4],2);

	LOGF_TRACE("SUB CAMPO 1 [%02x %02x]",chSub1[0],chSub1[1]);
	LOGF_TRACE("SUB CAMPO 2 [%02x %02x]",chSub2[0],chSub2[1]);
	LOGF_TRACE("DEVICE TYPE [%02x %02x]",chDeviceType[0],chDeviceType[1]);

	if((chDeviceType[0] >= 0x30 && chDeviceType[0] <= 0x39) && (chDeviceType[1] >= 0x30 && chDeviceType[1] <= 0x39))
	//if((chDeviceType[0] >= 0x00 && chDeviceType[0] <= 0x09) && (chDeviceType[1] >= 0x00 && chDeviceType[1] <= 0x09))
	{
		memset(chTemp,0x30,sizeof(chTemp)); 
		chTemp[0] = (chDeviceType[0]>>4)+0x30;
		chTemp[1] = chDeviceType[0];
		chTemp[2] = (chDeviceType[1]>>4)+0x30;
		chTemp[3] = chDeviceType[1];

		memcpy(chFFI,chTemp,8);
	}
	else
	{
		LOGF_TRACE("****DEFAULT FFI****");
		strcat(chFFI , "30300000");
	}

	LOGF_TRACE("FORM FACTOR INDICATOR [%s]",chFFI);
}



//Only applies for VISA
int inCTQvalue (char *chT9F6C)
{
	unsigned char cashback[14] = {0};
	unsigned char Amount[14] = {0};
	unsigned int CashBackAux = 0;
	unsigned int AmountAux = 0;
	int szCashBack = 0;
	int szAmount = 0;

	szAmount = inGetByteValuePP((char*)"AMOUNT",(char*)Amount);
	AmountAux = 0;
	for(int i = 0; i< szAmount; i++)
	{
		AmountAux |= Amount[i];
		if(i < (szAmount - 1))
			AmountAux <<= 8;
	}

	LOGF_TRACE("Amount = %i",AmountAux);
	szCashBack = inGetByteValuePP((char*)"CASHBACK",(char*)cashback);
	CashBackAux = 0;

	for(int i = 0; i< szCashBack; i++)
	{
		CashBackAux |= cashback[i];
		if(i < (szCashBack - 1))
			CashBackAux <<= 8;
	}

	LOGF_TRACE("cashback = %i",CashBackAux);
	AmountAux = AmountAux + CashBackAux;
	LOGF_TRACE("TotalAmount = %d",AmountAux);

	LOGF_TRACE("CTQ RESULT [%02x] [%02x]",chT9F6C[0],chT9F6C[1]);
	//Check Visa Contactless Payment Specification pdf
	// 	Byte 1
	// bit 8: 1 = Online PIN Required
	// bit 7: 1 = Signature Required
	// bit 6: 1 = Go Online if Offline Data Authentication Fails and Reader is online capable.
	// bit 5: 1 = Switch Interface if Offline Data Authentication fails and Reader supports VIS.
	// bit 4: 1 = Go Online if Application Expired
	// bit 3: 1 = Switch Interface for Cash Transactions
	// bit 2: 1 = Switch Interface for Cashback Transactions
	// bit 1: RFU (0)
	// Byte 2
	// bit 8: 1 = Consumer Device CVM Performed
	// Note: Bit 8 is not used by cards compliant
	// to this specification, and is set to 0b.
	// bit 7: 1 = Card supports Issuer Update
	// Processing at the POS
	// bits 6-1: RFU (000000) 
	if( (chT9F6C[0] == 0x00 && chT9F6C[1] == 0x00) || !memcmp(chT9F6C,"\x00\x00",2) )
	{
		LOGF_TRACE("No CVM Required");
		return 2;
	}
	else 
	{
		LOGF_TRACE("Checking if is 80");
	}
	if( (chT9F6C[1] & 0x80) == 0x80 )
	{
		return 3;
	}
	else 
	{
		LOGF_TRACE("Checking if is 40");
	}
	if( (chT9F6C[0] & 0x40) == 0x40 )
	{
		return 1;
	}
	else {
		LOGF_TRACE("Checking if Amount is less than 400.00");
	}
	if( (int)AmountAux <= (int)40000)
	{
		LOGF_TRACE("Amount under CVM Req Limit");
		return 2;
	}
	else {
		LOGF_TRACE("Some error occured, defaulting to 0");
	}
		

	// if( (chT9F6C[0] & 0x80) == 0x80 ) //online pin required
	// 	return 0;

	// if( (chT9F6C[0] & 0x40) == 0x40 ) //signature required
	// 	return ((AmountAux <= 40000) ? 2 : 1);

	// if ((chT9F6C[1] & 0x80) == 0x80) //Consumer device CVM Performed
	// 	//return (AmountAux <= 25000) ? 2 : 1;
	// 	return 3;

	return 0;
}



int inGet9F34Ctls(unsigned char pch_val [] )
{
	int i= 0;
	int iResult = 0;
	int in_ret = 0;
	int szAmount = 0;
	int szCashBack = 0;	
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned short us_siz = 3;
	unsigned int CashBackAux = 0;
	unsigned int AmountAux = 0;
	unsigned short LszFormFactor = 0L;
	char chFFI = 0;
	unsigned char Amount[14] = {0};
	unsigned char cashback[14] = {0};
	unsigned char szFormFactor[32+1] = {0};
	unsigned char buffTAG[50] = {0};

	
	if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
	{
		LOGF_TRACE("CTLS TAG");
		//Validacion BBVA CVM
		szAmount = inGetByteValuePP((char*)"AMOUNT",(char*)Amount);
		AmountAux = 0;
		for(i = 0; i< szAmount; i++)
		{
			AmountAux |= Amount[i];		
			if(i < (szAmount - 1))		
				AmountAux <<= 8;					
		}

		LOGF_TRACE("Amount = %i",AmountAux);	
		szCashBack = inGetByteValuePP((char*)"CASHBACK",(char*)cashback);	
		CashBackAux = 0;

		for(i = 0; i< szCashBack; i++)
		{
			CashBackAux |= cashback[i];		
			if(i < (szCashBack - 1))		
				CashBackAux <<= 8;					
		}	
		
		LOGF_TRACE("cashback = %i",CashBackAux);
		AmountAux = AmountAux + CashBackAux;
		LOGF_TRACE("TotalAmount = %d",AmountAux);

		TAG=0x9f6E;
		iResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
		if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			LszFormFactor = szTAG -3; //TAG - LEN
			memcpy(szFormFactor,&buffTAG[3],LszFormFactor);

		}
		else
			return 1; //ERROR
	
		if(AmountAux <= 25000)
		{
			// a) Deberá evaluar el monto CVM Limit si es menor a $250 el tag 9F34= 1F 00 00 
		 	// (indica que el CVM no fue requerido para cualquier tipo de device)
			pch_val[0] = 0x1F;
			pch_val[1] = 0x00;
			pch_val[2] = 0x00;

		}
		else
		{
			//VALIDAR DEVICE TYPE MC Y VC
			if(LszFormFactor > 0)
			{

				if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x03",5))
				{
					//VC
					chFFI = szFormFactor[0];
					chFFI&=0x1F;
					if(chFFI == 0x00) //Standar Card
					{

						// b) Si el monto es mayor al CVM Limit y además se trata de una tarjeta o sticker (device type=00), 
	     				// el tag 9F34= 1E 00 00 (indica que el método de verificación será por firma autógrafa)
						pch_val[0] = 0x1E;
						pch_val[1] = 0x00;
						pch_val[2] = 0x00;

					}
					else
					{
						// c) Si el monto es mayor al CVM Limit y la autenticación del tarjetahabiente es através de un dispositivo 
						// alterno como teléfono celular, pulsera, llavero, etc (device type diferente de 00, o no recupea el tag 
						// correspondiente al device type) el tag 9F34= 22 00 00 (indica que el método de autenticación se reserva 
						// para sistemas de pagos futuros)
						pch_val[0] = 0x22;
						pch_val[1] = 0x00;
						pch_val[2] = 0x00;

					}
				}
				else if(!memcmp(CTLSTransRes.T_9F06_AID.AID,"\xA0\x00\x00\x00\x04",5))
				{
					// MC
					LOGF_TRACE("CTLS TAG");
					if(szFormFactor[4] == 0x30 && szFormFactor[5] == 0x30)
					{
						// b) Si el monto es mayor al CVM Limit y además se trata de una tarjeta o sticker (device type=00), 
	     				// el tag 9F34= 1E 00 00 (indica que el método de verificación será por firma autógrafa)
						pch_val[0] = 0x1E;
						pch_val[1] = 0x00;
						pch_val[2] = 0x00;
					}
					else
					{
						// c) Si el monto es mayor al CVM Limit y la autenticación del tarjetahabiente es através de un dispositivo 
						// alterno como teléfono celular, pulsera, llavero, etc (device type diferente de 00, o no recupea el tag 
						// correspondiente al device type) el tag 9F34= 22 00 00 (indica que el método de autenticación se reserva 
						// para sistemas de pagos futuros)
						pch_val[0] = 0x22;
						pch_val[1] = 0x00;
						pch_val[2] = 0x00;
					}
				}
			}
			else
			{

				LOGF_TRACE("CTLS TAG");
				// c) Si el monto es mayor al CVM Limit y la autenticación del tarjetahabiente es através de un dispositivo 
				// alterno como teléfono celular, pulsera, llavero, etc (device type diferente de 00, o no recupea el tag 
				// correspondiente al device type) el tag 9F34= 22 00 00 (indica que el método de autenticación se reserva 
				// para sistemas de pagos futuros)
				pch_val[0] = 0x22;
				pch_val[1] = 0x00;
				pch_val[2] = 0x00;
			}

		}
	}
	return 0;
}


int getserialnumber(char * Serial_Number)
{
	string serial_number;
	char PRESN[16+1]={0};
	char POSSN[16+1]={0};
	int x=0;
	int y=0;
	x= sysGetPropertyString(SYS_PROP_HW_SERIALNO,serial_number);
	LOGF_TRACE(" x = [%i]",x);
	strcpy(PRESN,serial_number.c_str());
	LOGAPI_HEXDUMP_TRACE("\nSERIAL NUMBER",PRESN,sizeof(PRESN));
	for(x=0; x<=strlen(PRESN);x++)
	{
		if(PRESN[x]!=0x2D) //" -"
		{
			POSSN[y]=PRESN[x];
			y++;		
		}
			
	}
	memcpy(Serial_Number,"\x9F\x1E\x08",3);
	memcpy(Serial_Number +3,POSSN,8);
	LOGAPI_HEXDUMP_TRACE("\nSERIAL NUMBER",Serial_Number,16);
	return strlen(Serial_Number);
}



int read_PAN_CTLS(char* PanEMV)
{
	int in_val = 0;
	int inResult = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];

	char TAG5A[50] = {0};
	int sztag5A = 0;
	int typCTLS;

	unsigned char Amount[14] = {0};
	unsigned char ucReader=0;
	unsigned char cashback[14] = {0};

	LOGF_TRACE("\n--read_PAN_CTLS--");
	LOGF_TRACE("\n\n");

	in_val = CTLS_FirstGenerateAC(&CTLSTransRes);
	LOGF_TRACE("\nin_val = %x",in_val);

	typCTLS = Check_typ_CTLS();	
	
	if(typCTLS == CTLS_TXN_CHIP)
	{
		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x5A;
		inResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );
		if( (inResult == EMV_ADK_OK) && (szTAG > 0) )
		{
			sztag5A = szTAG - 2;		
			//vdHex2Asc((char*)(buffTAG + 2), TAG5A, sztag5A);
			memcpy(PanEMV,buffTAG + 2,sztag5A);
			//debug("PAN from EMV = %s",TAG5A);
			LOGF_TRACE(PanEMV,sztag5A,"*****read_PAN_CTLS*****");
		}
		else
		{
			LOGF_TRACE("\nPAN is not available");
			LOGF_TRACE("\n TRY GET FROM TK2");
			szTAG = 0;
			TAG = 0;
			memset(buffTAG,0x00,sizeof(buffTAG));
			TAG = 0x57;
			inResult = getCT_EMV_TAG(EMV_ADK_FETCHTAGS_NO_EMPTY, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );
			if( (inResult == EMV_ADK_OK) && (szTAG > 0) )
			{
				char ch_tk2_pan [50] = {0};
				int  in_tk2_pan = 0;
				char * pch_equ = NULL;
				in_tk2_pan = inHexToAsc ((char *)buffTAG + 2, ch_tk2_pan, szTAG - 2);

				pch_equ = (char *)memchr (ch_tk2_pan, 'D', in_tk2_pan);

				if (pch_equ)
				{
					*pch_equ = 0;

					in_tk2_pan = pch_equ - ch_tk2_pan;
				}

				LOGAPI_HEXDUMP_TRACE("PAN",ch_tk2_pan,in_tk2_pan);
				inAscToHex(ch_tk2_pan,PanEMV,in_tk2_pan);
				//memcpy(PanEMV,ch_tk2_pan,in_tk2_pan);
				sztag5A = in_tk2_pan / 2;
			}
		}
	}
	else if(typCTLS == CTLS_TXN_MSR)
	{
		char ch_tk2_pan [50] = {0};
		int  in_tk2_pan = 0;
		char * pch_equ = NULL;

		in_tk2_pan = inHexToAsc ((char *)CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2data, ch_tk2_pan, CTLSTransRes.T_DF5D_CL_MAGSTRIPE_T2.tr2len - 2);

		pch_equ = (char *)memchr (ch_tk2_pan, 'D', in_tk2_pan);

		if (pch_equ)
		{
			*pch_equ = 0;

			in_tk2_pan = pch_equ - ch_tk2_pan;
		}

		LOGAPI_HEXDUMP_TRACE("PAN",ch_tk2_pan,in_tk2_pan);
		inAscToHex(ch_tk2_pan,PanEMV,in_tk2_pan);
		//memcpy(PanEMV,ch_tk2_pan,in_tk2_pan);
		sztag5A = in_tk2_pan / 2;
	}
	
    EMV_CTLS_EndTransaction(0);
	return sztag5A;
}
