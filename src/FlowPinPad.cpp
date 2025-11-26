//SYSTEM INCLUDES
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <cstring>
#include <time.h>
#include "sys/time.h"
#include <unistd.h>
#include <fcntl.h>

// jbf 20171124
#include <svc.h>

//ADK INCLUDES
#include "html/gui.h"
#include "msr/msr.h"
#include <log/liblog.h>
#include <sysinfo/sysbar.h>
#include <sysinfo/sysinfo.h>


//USER INCLUDES
#include "App/FlowPinPad.h"
#include "App/encryp_aes.h" // FAG 08-NOV-2016
#include "ComRdc/ComRdc.h"
#include "PinPad/PP_Cmd.h"
#include "CapX/usrcmds.h"
#include "CapX/dkprsa.h"
#include "CapX/debug.h"
#include "EmvRdc/VosEMVProcess.h"
#include "Trans/TRANS.h"
#include "Convertion/Convertion.h"
#include "ISO/ISO.h"

using namespace std;
using namespace vfigui;
using namespace vfisysinfo;


//To detect the type of input
//CHIP, MSR, KBD OR CTLS
unsigned char chTechnology  = 0; 
unsigned char count_fallback = 0;
//To know if we have to execute an action when the CC4 command comes
unsigned char flagZ4 = 0;
unsigned char flagC51 = 0;
unsigned char flagY10 = 0;
unsigned char flagZ10 = 0;
unsigned char flagC30 = 0;  // FAG 07-NOV-2016
unsigned char flagkey = 0;  // FAG 13-FEB-2017
		int    acqmul = 0 ; // FAG 03-ABRIL-2017

int inTransInProgress = 0 ;
//int  inTimeOut = 15;

//Daee 08/01/2017
bool boCtlsON = false;
bool isMacroCmdUni = false;
bool boDUKPTasm    = false; //4 ONLY ASM TOKENS DUKPT FOR MACROCMDS MULTIADQ, IN MACROCMDS UNIADQ DOES NOT ASM



			
// ***************************************************
//
// PROTOTYPES
//
// ***************************************************			
void voUpdatePPClock(char *Date, char *Time);
int GetTransactionTimeout(void);



void vdCmdFlow(void)
{
	int font_size;
	int inRes = 0;
	int inVal = 0;
	
	char chAckNack[1] ={0};
	char chCmdRcv[MAX_CMD_LEN+1] = {0};
	char chSendbuff[1024] = {0};
	char chRcvbuff[1024] = {0};
	std::string szTmp = "RS232_profileUSB.xml";
	map<string,string> value;

	
	//memset(szTmp, '\0', 100);  
	
	//sprintf(szTmp, "RS232_profile.xml");

	LOGF_TRACE("--vdCmdFlow--");
	
	if(!inInitComAdk())
	{	

//		inRes = inComConnectAdk(szTmp.c_str(),90000,3);
		vdSetConnectionProfile(szTmp);
		inRes = inComConnectAdk(90000,3);
		
		if(!inUSB_LINK())
		{
			szTmp = "RS232_profileCOM.xml";
			vdSetConnectionProfile(szTmp);
			inRes = inComConnectAdk(90000,3);
		}
	
		if(!inRes)
		{
			//FLUJO PRINCIPAL
			while(1)
			{			
				status_key();	
				LOGF_TRACE("flagC51 -> %x  flagC30-> %x, flagkey -> %x ",flagC51,flagC30,flagkey);
				LOGF_TRACE("flagZ4 -> %i, flagC51 -> %i ", flagZ4, flagC51);

				if(flagkey == 0)
				{
					value["HEADER_TITLE"]="ERROR";
					value["BODY_HEADER"]="LLAVE NO INYECTADA";
					value["BODY_MSG"]="CARGAR LLAVE";
					value["NAME_IMG"]="EXTLOADKEY";
					uiInvokeURL(value,"GenScreen_alert.html");
				} 
				
				
				//if( (flagC51 == 0) && (flagZ10 == 0) && (flagY10 == 0) && (flagC30 == 0))
				//else if( (flagC51 == 0) && (flagC30 == 0)) // FAG 03-ABRIL-2017
				//else if( 1)
				else if((flagC51 == 0))   // PGC 13FEB25
				//else if((flagZ4 == 0))
				{
					LOGF_TRACE("IDLE");
					value["input"]="";
					//uiInvokeURL(1,value,"IdlePP.html");
					uiInvokeURL(value,"IdlePP.html");	
				}

				
				inRes = 0 ;
				
				
				while(inRes <= 0)
				{
					memset(chRcvbuff,0,sizeof(chRcvbuff));
					//inRes = inRcvAdk(chRcvbuff,sizeof(chRcvbuff),1000);
					inRes = inRcvAdk(chRcvbuff,sizeof(chRcvbuff),50);
					if(inRes == -15)
					{
						//TODO: SOLO EN SERVERMODE USAR  0
						//inRes = inComConnectAdk(szTmp.c_str(),0,1);	
						inRes = inComConnectAdk(0,3);	

					}
				}
				
				//ANALIZAMOS PAQUETE
				LOGF_TRACE("inRes [%d] \n",inRes);
				memset(chCmdRcv,0,sizeof(chCmdRcv));
				if(!inGetCmdBBVA(chCmdRcv,chRcvbuff,(int *)&inRes))
				{
					if(inRes == 1) //ACK, NACK, ENQ, EOT 
						inVal = XML_CMD_OK;
					else
					{
						if(!memcmp(chCmdRcv,"Z2",2))
						{	
							LOGF_TRACE("ES UN COMANDO Z2. PARSEO APARTE");
							LOGAPI_HEXDUMP_TRACE("SUB----",chRcvbuff + 3,1);
							vdSetByteValuePP((char *)"SUB",chRcvbuff + 3,1);
							LOGAPI_HEXDUMP_TRACE("TEXT---",chRcvbuff + 4,inRes -6);
							vdSetByteValuePP((char *)"TEXT",chRcvbuff + 4,inRes -6);
						}
						else if(!memcmp(chCmdRcv,"SE14",4) || !memcmp(chCmdRcv,"C14",3)){
							parse14Command(chCmdRcv,chRcvbuff,inRes);	
						}
						else
						{
							inVal = PP_CMD_unpack(chCmdRcv,chRcvbuff,inRes);

							LOGF_TRACE("SALIENDO DE UNPACK");
						}
					}
					
					if (inVal == XML_CMD_OK || inVal == NO_CMD_DATA)
					{
						//COMANDO RECIBIDO CORRECTAMENTE RESPONDEMOS ACK
						//-// jbf 20171222 Enviando ACK moved below
						chAckNack[0]=0x06;
						inSendAdk(chAckNack,1);
						
						LOGF_TRACE("******inDoCmd*******\n");
						memset(chSendbuff,0,sizeof(chSendbuff));
						inDoCmdBBVA(chCmdRcv,chSendbuff);
					}
					else if(inVal == LRC_ERROR)
					{
						chAckNack[0]=0x15;
						inSendAdk(chAckNack,1);
					}
					else
					{
						//Formato Invalido
						memset(chSendbuff,0,sizeof(chSendbuff));
						inRes = 0;
						LOGF_TRACE("\n*****Fallo al parsear*****\n");
						chSendbuff[inRes] = 0x02;
						inRes++;
						if(isMacroCmdUni)
							memcpy(chCmdRcv + 1,"3",1);
						memcpy(chSendbuff + inRes,chCmdRcv,strlen(chCmdRcv));
						inRes += strlen(chCmdRcv);
						memcpy(chSendbuff + inRes , "02" , 2);
						inRes += 2;
						chSendbuff[inRes] = 0x03;
						inRes++;

						chSendbuff[inRes] = CalcLRC((unsigned char*)chSendbuff,inRes);
						inRes++;

						LOGAPI_HEXDUMP_TRACE("RESP FALLO",chSendbuff,inRes);
						LOGF_TRACE("\n******Enviando*******\n");
						inVal = inSendAdk(chSendbuff,strlen(chSendbuff));
						if(inVal == inRes)
						{	
							LOGF_TRACE("\n******Ack*******\n");
							//in_val = inWaitAck(chOutCmdData,inLong,inTry);
						}

						
					}	
				}
				else
				{
					memset(chSendbuff,0,sizeof(chSendbuff));
					sprintf(chSendbuff,"%s COMANDO INVALIDO",chCmdRcv);
				}
				
			}
			//FINAL FLUJO PRINCIPAL
		}
		else 
		{
			uiDisplay(uiPrint("CONNECT FAIL! - %d\n", font_size) );
		}
		
		inDestroyComAdk();
	}
	else
		uiDisplay(uiPrint("INIT COMO FAIL! - %d\n", font_size) );	
	
	return ;
}

int inGetCmdBBVA(char* chCmd,  char* chDataCmd, int *inLenDataCmd)
{
	
	int inRet = -1;
	LOGF_TRACE("--inGetCmd--");
	isMacroCmdUni = false;
	boDUKPTasm = true;
	//Mensajes de control (ACK/NAK/ENQ/EOT)
	//memset(chCmd,0x00,sizeof(chCmd));
	if(*inLenDataCmd == 1)
	{
		if(chDataCmd[0] == 0x05 || chDataCmd[0] == 0x06 || chDataCmd[0] == 0x04 || chDataCmd[0] == 0x15)
		{
			memcpy(chCmd,chDataCmd,1);
			inRet = 0;
		}
	}
	//MENSAJES Prosa o Elavon
   else {
	if(chDataCmd[0] == 0x06) {
		int ii=0;
		(*inLenDataCmd)--;
		for (ii=0 ; ii < *inLenDataCmd ; ii++) {
			chDataCmd[ii] = chDataCmd[ii + 1];
		}
		chDataCmd[ii] = 0x00;
	}
	
	if(!memcmp(chDataCmd+1,"C51",3) || !memcmp(chDataCmd+1,"C54",3) || !memcmp(chDataCmd+1,"C14",3) || !memcmp(chDataCmd+1,"C12",3) 
			|| !memcmp(chDataCmd+1,"C25",3) || !memcmp(chDataCmd+1,"Z10",3) || !memcmp(chDataCmd+1,"Z11",3)) 
	{
		memcpy(chCmd,&chDataCmd[1],3);
		LOGF_TRACE("Mensaje Recibido = %s",chCmd);
		inRet = 0;
	} else if(!memcmp(chDataCmd+1,"SE51",4) || !memcmp(chDataCmd+1,"SE54",4) || !memcmp(chDataCmd+1,"SE14",4) || !memcmp(chDataCmd+1,"SE12",4) 
			|| !memcmp(chDataCmd+1,"SE25",4)) 
	{
		if(isSECommand()){
			memcpy(chCmd,&chDataCmd[1],4);
			chCmd[4]=0;
			LOGF_TRACE("Mensaje Recibido = %s",chCmd);
			inRet = 0;
		}else{
			LOGF_TRACE("Recibi comando SE pero la bandera CXXSEXX no esta habilitada");
		}
		
	} 
	else if(!memcmp(chDataCmd+1,"Z4",2) || !memcmp(chDataCmd+1,"Z3",2) || !memcmp(chDataCmd+1,"Z2",2) || !memcmp(chDataCmd+1,"72",2) || !memcmp(chDataCmd+1,"Q8",2) || !memcmp(chDataCmd+1,"I02",2) || !memcmp(chDataCmd+1,"C23",2))
	{
		memcpy(chCmd,&chDataCmd[1],2);
		LOGF_TRACE("Mensaje Recibido = %s",chCmd);
		inRet = 0;
	}
	else 
	{
		LOGF_TRACE("Error en Mensaje Recibido len[%d]",*inLenDataCmd);
		LOGAPI_HEXDUMP_TRACE("DATOS RECIBIDOS",chDataCmd,*inLenDataCmd);
	}
   }

	return inRet;
}

int inDoCmdBBVA(char *chCmdBBVA, char *chOutCmdData)
{
	int inRet = -1;
	LOGF_TRACE("\n******chCmd [%s]*******\n",chCmdBBVA);
		
	//*****************************MENSAJES DE CONTROL**************************
	if(!memcmp(chCmdBBVA,"\x04",1)) //EOT
	{
		memcpy(chOutCmdData,"\x06",1);
		inRet = 0;
	}
	else if(!memcmp(chCmdBBVA,"\x05",1)) //ENQ
	{
		memcpy(chOutCmdData,"\x06",1);
		inRet = 0;
	}
	else if(!memcmp(chCmdBBVA,"\x06",1)) //ACK
	{
		memcpy(chOutCmdData,"\x06",1);
		inRet = 0;
	}	
	else if(!memcmp(chCmdBBVA,"\x15",1)) //NAK
	{
		memcpy(chOutCmdData,"\x06",1);
		inRet = 0;
	}	
	/* MENSAJES								*/
	else if(!memcmp(chCmdBBVA,"Y10",3)) //INICIALIZANDO LLAVES
	{
		flagY10 = 1;
		LOGF_TRACE("******inDoCmdY10BNTE*******\n");
		inRet = inDoY10BBVA(chOutCmdData);
		//inRet = 0;
	}	
	else if(!memcmp(chCmdBBVA,"Y11",3)) //INICIALIZANDO LLAVES
	{
		LOGF_TRACE("******inDoCmdY11BNTE*******\n");
		inRet = inDoY11BBVA(chOutCmdData);		
		flagY10 = 0;
		//inRet = 0;
	}	
	else if(!memcmp(chCmdBBVA,"C14",3)||!memcmp(chCmdBBVA,"SE14",4)) //CARGA DE TABLA DE BINES
	{
		LOGF_TRACE("******inDoCmdC14*******\n");
		//inRet = inDoC14BBVA(chOutCmdData);
		inRet = exceptionBINList14(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"C51",3) || !memcmp(chCmdBBVA,"SE51",4)) //INICIA UNA VENTA 
	{		
		if(!flagZ4)	
			chTechnology = 0;
		flagC51 = 1;
		LOGF_TRACE("******inDoC51*******");
		//inRet = inDoC51BBVA(chOutCmdData);				
		inRet = initTransaction51(chOutCmdData);				
	}
	else if(!memcmp(chCmdBBVA,"Z4",2))	//PARA PEDIR EL PAN DE LA TARJETA
	{		
		chTechnology = 0;
		debug("******inDoCmdZ4BBVA*******\n");
		inRet = inDoZ4BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"C54",3) || !memcmp(chCmdBBVA,"SE54",4))	//RESPUESTA PARA LA ECR
	{
		LOGF_TRACE("******inDoC54*******");
		LOGF_TRACE("FlagC51 = %i",flagC51);
		
		if(flagC51)
		{
			LOGF_TRACE("******inDoCmdC54_*******\n");
			//inRet = inDoC54BBVA(chOutCmdData);			
			inRet = endTransaction54(chOutCmdData);			
			//Limpiamos el objeto map			
			flagC51 = 0;
			clear_parse();
		}		
	}
	else if(!memcmp(chCmdBBVA,"C12",3) || !memcmp(chCmdBBVA,"SE12",4))	//COMANDO DONDE VIAJAN LOS SCRIPTS
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmdC12_*******\n");
		inRet = inDoC12BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"C25",3) || !memcmp(chCmdBBVA,"SE25",4))	//COMANDO DONDE VIAJAN LOS SCRIPTS
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmdC25_*******\n");
		inRet = inDoC25BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"Z3",2))	//COMANDO DONDE viaja el nombre de la tabla de bines de a caja
	{		
		//chTechnology = 0;
		debug("******inDoCmdZ3BBVA*******\n");
		inRet = inDoZ3BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"72",2))	//COMANDO para cancelar servicios pendientes en el PP
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmd_72_*******\n");
		flagC51 = 0;
		flagC30 = 0; 
		flagZ4 = 0; 
		clear_parse();		
	}
	else if(!memcmp(chCmdBBVA,"C23",2))	//COMANDO para cancelar servicios pendientes en el PP
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmd_C23_*******\n");
		flagC51 = 0;
		flagC30 = 0; 
		clear_parse();
		inRet = inDoC23BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"I02",2))	//COMANDO para cancelar servicios pendientes en el PP
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmd_I02_*******\n");
		inRet = inDoI02BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"Z2",2))	//COMANDO para cancelar servicios pendientes en el PP
	{		
		//chTechnology = 0;
		LOGF_TRACE("******inDoCmd_Z2_*******\n");
		inRet = inDoZ2BBVA(chOutCmdData);
	}
	else if(!memcmp(chCmdBBVA,"Q8",2)) //SOLICITUD DE NUMERO DE SERIE 
	{			
		chTechnology = 0;
		LOGF_TRACE("******inDoQ8Uni*******\n");	
		inRet = inDoQ8Uni(chOutCmdData);				
	}
	
	else if(!memcmp(chCmdBBVA,"Z10",3)) //INICIALIZANDO LLAVES
	{
		flagZ10 = 1;
		LOGF_TRACE("******inDoCmdZ10BVVA*******\n");	
		//inRet = inDoZ10Uni(chOutCmdData);
		inRet = requestTransportKeyZ10(chOutCmdData);
		//inRet = 0;
	}	
	else if(!memcmp(chCmdBBVA,"Z11",3)) //INICIALIZANDO LLAVES
	{
		LOGF_TRACE("******inDoCmdZ11BVVA*******\n");
		//inRet = inDoZ11Uni(chOutCmdData);		
		inRet = injectKeyZ11(chOutCmdData);
		flagZ10 = 0;
		//inRet = 0;
	}	
	// jbf 20171222
	else
	{
		LOGF_TRACE("******Comando no reconocido*******\n");
	}

	return inRet;
	
}

static int in_Tx_Result(char *szDispStringR) //Daee 23/09/2013
{
	int bo_find = FALSE;
	char temp[206];
	int i;
	map<string,string> value;   
	
	LOGF_TRACE("in_Tx_Result");

	memset(temp , 0 , sizeof(temp));
	memcpy(temp , szDispStringR , strlen(szDispStringR));

	for(i=0;temp[i];i++)		//Daee 23/09/2013 	//convertimos a Mayusculas para la busqueda constante		
		temp[i] = toupper(temp[i]);

	value.clear();

	if (strstr (temp,"APROBAD"))
	{
		value["NAME_IMG"]="EXTAPROVED";
		bo_find = TRUE;
	}
	else if (strstr (temp, "DECLINAD"))
	{
		value["NAME_IMG"]="EXTDECLINE";
		bo_find = TRUE;
	}

	if(bo_find)
	{
		value["BODY_HEADER"]=string(szDispStringR);
		value["BODY_MSG"]="";
		
		uiInvokeURL(value,"GenScreen_alert.html");

		sleep(SHOW_AD);
	}

  return bo_find;
}


int inDoZ2BBVA(char *chOutCmdData)
{
	int iResult = 0;
	char SUB[2] = {0};
	int szSUB = 0;
	char Text[206] = {0};
	int szText = 0;
	map<string,string> value;	

	LOGF_TRACE("inDoZ2BBVA");
	
	szSUB = inGetByteValuePP((char*)"SUB",SUB);
	szText = inGetByteValuePP((char*)"TEXT",Text);	
	
	LOGF_TRACE("TEXT: %s",Text);

	if( !in_Tx_Result(Text) )
	{
		value["HEADER_TITLE"]= "";
		value["BODY_HEADER"]=Text;
		value["L2_C"]= "" ;
		uiInvokeURL(value,"GenScreen_info.html");

		sleep(SHOW_ALERT);
	}
	return iResult;
	
}

int inDoI02BBVA(char *chOutCmdData)
{
	int iResult=0;
	char Status[3] = {0};
	int szStatus = 0;
	int inLong = 0;

	LOGF_TRACE("--inDoI02BBVA--");

	iResult=_CHECK_INSERTED_CARD();
	LOGF_TRACE("_CHECK_INSERTED_CARD %X",iResult);
	if(iResult==TRUE)
	{
		LOGF_TRACE("****Pedimos que se retire la tarjeta****");
		Remove_Card();
		LOGF_TRACE("****tarjeta retirada****");
	}
	szStatus = 2;
 	memcpy(Status,"00",szStatus);

	vdSetByteValuePP((char*)"STATUS",Status,szStatus);
	iResult = PP_CMD_Pack((char*)"I02",chOutCmdData,&inLong);

	 

		if(iResult == XML_CMD_OK)
		{
			debug("******inDoI02BBVA PACK*******\n");
			iResult = inSendAdk(chOutCmdData,inLong);
			if(iResult == inLong)
			{	
				//iResult = inWaitAck(chOutCmdData,inLong,inTry);
			}
		}
}
int inDoC23BBVA(char *chOutCmdData)
{
	
 int iResult = 0;
 char Status[3] = {0};
 int szStatus = 0;

 int inLong = 0;
 int inTry = 3;
 LOGF_TRACE("--inDoC23BBVA--");

 //Se reponde un Eco
 szStatus = 2;
 memcpy(Status,"00",szStatus);

vdSetByteValuePP((char*)"STATUS",Status,szStatus);
iResult = PP_CMD_Pack((char*)"C23",chOutCmdData,&inLong);

 

	if(iResult == XML_CMD_OK)
	{
		debug("******inDoC23BBVA PACK*******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

 return iResult;
	
}

int inDoZ3BBVA(char *chOutCmdData)
{
	int iResult = 0;
	char Text[10] = {0};
	int szText = 0;

	
	debug("--inDoZ3BBVA--");

	szText = inGetByteValuePP((char*)"TEXT",Text);
	putEnvFile((char *)"perm",(char *)"BINECR",Text);

	debug("Text %s",Text);

	debug("--End of inDoZ3BBVA--");
	
	return iResult;
}


int inDoC12BBVA(char *chOutCmdData)
{
 int iResult = 0;
 char Status[3] = {0};
 int szStatus = 0;

 int inLong = 0;
 int inTry = 3;
 LOGF_TRACE("--inDoC12BBVA--");

 //Se reponde un Eco
 szStatus = 2;
 memcpy(Status,"00",szStatus);

 vdSetByteValuePP((char*)"STATUS",Status,szStatus);
if(isSECommand()){
	iResult = PP_CMD_Pack((char*)"SE12",chOutCmdData,&inLong);
}else{
	iResult = PP_CMD_Pack((char*)"C12",chOutCmdData,&inLong);
}
 

	if(iResult == XML_CMD_OK)
	{
		debug("******inDoC12BBVA PACK*******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

 return iResult;
}

int inDoC25BBVA(char *chOutCmdData)
{
	int iResult = 0;
	//Variables para obtener la informaci�n que viene en el comando C25
	char typ_Script[5] = {0}; 
	int sztyp_Script = 0;	
	char flagfile[2] = {0};
	int szflagfile = 0;
	char num_scripts[3] = {0};
	int sznum_scripts = 0;	
	char Script[5000] = {0};
	int szScript = 0;
	char Script_aux[5000] = {0};	
	char Script71[5000] = {0};
	int szScript71 = 0;
	char Script72[5000] = {0};
	int szScript72 = 0;

	char tmp[512] = {0};
	int    sztmp = 0;

	int inHdl1 = 0;

	int idx_Script = 0;

	int inLong = 0;
	int inTry = 3;
		
	
	LOGF_TRACE("--inDoC25BBVA--");

	sztyp_Script = inGetByteValuePP((char*)"TYPE_SCRIPT",(char*)typ_Script);
	szflagfile = inGetByteValuePP((char*)"FLAG_FILE",(char*)flagfile);
	sznum_scripts = inGetByteValuePP((char*)"NUM_SCRIPT",(char*)num_scripts);
	szScript = inGetByteValuePP((char*)"SCRIPT",(char*)Script);

	if(szScript > 0)
	{
		LOGF_TRACE("LLeg� un script");

		if(!memcmp(typ_Script,"\x37\x31",2))
		{
			LOGF_TRACE("Script 71");
			//Script71[0] = 0x71;	
			//Script71[1] = szScript;			
			if(!memcmp(num_scripts,"\x30\x31",2))
			{
				LOGF_TRACE("Valor del capo_num: 01");
				if(!memcmp(flagfile,"\x31",1))
				{	
					int borra = 0;
					vdAsc2Hex ((const char*)Script,(char*)&Script71[0],szScript);
					vdSetByteValuePP((char*)"SCRIPT71",Script71,(szScript/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT71 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT71",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					borra = unlink ( "././flash/Script.dat");
					inHdl1 = open("././flash/Script.dat",  O_CREAT  | O_WRONLY , 0666);	
					write(inHdl1,Script,szScript/2);
					LOGF_TRACE("borra = %i",borra);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);		
					iResult = 0;
				}
				else if(!memcmp(flagfile,"\x30",1))
				{
					vdAsc2Hex ((const char*)Script,(char*)&Script71[0],szScript);
					vdSetByteValuePP((char*)"SCRIPT71",Script71,(szScript/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT71 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT71",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					inHdl1 = open("/home/usr1/flash/Script.dat", O_APPEND | O_WRONLY , 0666);   //To create a file where the script will be put
					write(inHdl1,Script71,szScript/2);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);						
					iResult = 0;
				}
			}
			if(!memcmp(num_scripts,"\x38\x31",2))
			{
				LOGF_TRACE("Valor del capo_num: 81");
				vdSetByteValuePP((char*)"SCRIPT_aux",Script,szScript);			
			}
			if(!memcmp(num_scripts,"\x39\x31",2))
			{
				LOGF_TRACE("Valor del capo_num: 91");
				idx_Script = inGetByteValuePP((char*)"SCRIPT_aux",(char*)Script_aux);
				
				memcpy(Script_aux + idx_Script,Script,szScript);			
				
				if(!memcmp(flagfile,"\x31",1))
				{
					int borra = 0;
					vdAsc2Hex ((const char*)Script_aux,(char*)&Script71[0],szScript+idx_Script);
					vdSetByteValuePP((char*)"SCRIPT71",Script71,((szScript+idx_Script)/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT71 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT71",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					borra = unlink ( "././flash/Script.dat");
					inHdl1 = open("././flash/Script.dat",  O_CREAT  | O_WRONLY , 0666);	
					write(inHdl1,Script_aux,szScript+idx_Script);
					LOGF_TRACE("borra = %i",borra);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);	
					iResult = 0;
									
				}
				else if(!memcmp(flagfile,"\x30",1))
				{	
					vdAsc2Hex ((const char*)Script_aux,(char*)&Script71[0],szScript+idx_Script);
					vdSetByteValuePP((char*)"SCRIPT71",Script71,((szScript+idx_Script)/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT71 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT71",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					inHdl1 = open("/home/usr1/flash/Script.dat", O_APPEND | O_WRONLY , 0666);   //To create a file where the script will be put
					write(inHdl1,Script_aux,szScript+idx_Script);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);
					iResult = 0;
				}
			}
			
		}
		else if(!memcmp(typ_Script,"\x37\x32",2))
		{
			LOGF_TRACE("Script 72");
			//Script72[0] = 0x72;
			//Script72[1] = szScript;			
			if(!memcmp(num_scripts,"\x30\x31",2))
			{
				if(!memcmp(flagfile,"\x31",1))
				{	
					int borra = 0;
					vdAsc2Hex ((const char*)Script,(char*)&Script72[0],szScript);
					vdSetByteValuePP((char*)"SCRIPT72",Script72,(szScript/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT72 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT72",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					borra = unlink ( "././flash/unScript.dat");
					inHdl1 = open("././flash/unScript.dat",  O_CREAT  | O_WRONLY , 0666);	
					write(inHdl1,Script,szScript+2);
					LOGF_TRACE("borra = %i",borra);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);								
					iResult = 0;
				}
				else if(!memcmp(flagfile,"\x30",1))
				{
					vdAsc2Hex ((const char*)Script,(char*)&Script72[0],szScript);
					vdSetByteValuePP((char*)"SCRIPT72",Script72,(szScript/2)+0);
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT72 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT72",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					inHdl1 = open("/home/usr1/flash/unScript.dat", O_APPEND | O_WRONLY , 0666);   //To create a file where the script will be put
					write(inHdl1,Script,szScript+2);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);					
					iResult = 0;
				}
			}
			if(!memcmp(num_scripts,"\x38\x31",2))
			{	
				LOGF_TRACE("Valor del capo_num: 81");
				vdSetByteValuePP((char*)"SCRIPT_aux",Script,szScript);					
			}
			if(!memcmp(num_scripts,"\x39\x31",2))
			{
				LOGF_TRACE("Valor del capo_num: 91");
				idx_Script = inGetByteValuePP((char*)"SCRIPT_aux",(char*)Script_aux);
				memcpy(Script_aux + idx_Script,Script,szScript);
				
				if(!memcmp(flagfile,"\x31",1))
				{
					int borra = 0;
					vdAsc2Hex ((const char*)Script_aux,(char*)&Script72[2],szScript+idx_Script);
					vdSetByteValuePP((char*)"SCRIPT72",Script72,((szScript+idx_Script)/2+2));
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT72 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT72",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					borra = unlink ( "././flash/unScript.dat");
					inHdl1 = open("././flash/unScript.dat",  O_CREAT  | O_WRONLY , 0666);	
					write(inHdl1,Script_aux,szScript+idx_Script);
					LOGF_TRACE("borra = %i",borra);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);
					iResult = 0;
				}
				else if(!memcmp(flagfile,"\x30",1))
				{
					
					vdAsc2Hex ((const char*)Script_aux,(char*)&Script72[2],szScript+idx_Script);
					vdSetByteValuePP((char*)"SCRIPT72",Script72,((szScript+idx_Script)/2+2));
					LOGF_TRACE(" +++ CONTENIDO DEL SCRIPT72 +++ ");
					sztmp = inGetByteValuePP((char*)"SCRIPT72",tmp);
					LOGAPI_HEXDUMP_TRACE("***  tmp *** ",tmp,sizeof( tmp ) );
					inHdl1 = open("/home/usr1/flash/unScript.dat", O_APPEND | O_WRONLY , 0666);   //To create a file where the script will be put
					write(inHdl1,Script_aux,szScript+idx_Script);
					LOGF_TRACE("inHdl1 = %i",inHdl1);
					close(inHdl1);	
					iResult = 0;
				}
			}
		}
		else 
		{
			LOGF_TRACE("Script desconocido");		
			iResult = -1;
		}
	}
	else
	{
		LOGF_TRACE("No hay Scripts");
		iResult = -1;
	}

	if(!iResult)
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
	else
		vdSetByteValuePP((char*)"STATUS",(char*)"02",2);   //Formato de datos invalido

	if(isSECommand()){
		iResult = PP_CMD_Pack((char*)"SE25",chOutCmdData,&inLong);
	}else{
		iResult = PP_CMD_Pack((char*)"C25",chOutCmdData,&inLong);
	}
	

	if(iResult == XML_CMD_OK)
	{
		debug("******inDoC25BBVA PACK*******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}
	
	
	return iResult;
}

int inDoY10BBVA(char *chOutCmdData)
{
	char ch_es[1024] =  {0};
	char ch_ew[1024] = {0};
	char LenHex[2]    =  {0};
	char lenfld =  0;
	int in_es     = 0;
	int in_ew    = 0;
	int inLong  = 0;
	int in_val    = 0;
	int inTry     = 3;
	int mov      = 0;

	map<string,string> value;	

	if(acqmul==UNI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"62",2);
	}

	else
	{
		//value["HEADER_TITLE"]= "COMANDO";
		value["BODY_HEADER"]="";
		value["L2_C"]= "SOLICITANDO";
		value["L3_C"]= "LLAVE MULTI";
		
		uiInvokeURL(value,"GenScreen_info.html");

		sleep(SHOW_ALERT);			
		LOGF_TRACE("******inDoY10BBVA*******\n");
		in_val = in_cpx_ipk_ktk (ch_es, &in_es, ch_ew, &in_ew);
		 
	//	LOGF_TRACE("******inDoY10BBVA [%d]*******\n",in_val);
	//	LOGF_TRACE("******inDoY10BBVA [%d]*******\n",in_val);
	//	LOGF_TRACE("******inDoY10BBVA [%d]*******\n",in_val);
	//	LOGF_TRACE("******inDoY10BBVA [%d]*******\n",in_val);
	//	LOGF_TRACE("******inDoY10BBVA [%d]*******\n",in_val);
		if(in_val == VS_SUCCESS)
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			vdSetByteValuePP((char*)"TOKEW",ch_ew,in_ew);
			vdSetByteValuePP((char*)"TOKES",ch_es,in_es);
			LOGF_TRACE("******TOKEW [%i]*******\n",in_ew);
			LOGF_TRACE("******TOKES  [%i]*******\n",in_es);
			mov = in_ew + in_es ;
			LOGF_TRACE("****** valor de MOV   [%i]*******\n",mov);		
			lenfld = 0;
			lenfld |= mov;	//length
			LenHex[1] = lenfld;
			lenfld = 0;
			lenfld |= mov >> 8;
			LenHex[0] = lenfld;
			vdSetByteValuePP((char*)"LENGH", LenHex,2);
		
			
		}
		else
		{
			//Nothing
		}
	}	

	in_val = PP_CMD_Pack((char*)"Y10",chOutCmdData,&inLong);

	if(in_val == XML_CMD_OK)
	{
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}

int inDoY11BBVA(char *chOutCmdData)
{

	char ch_ex[1024] = {0};
	char chCodResp[5+1] ={0};
	int in_ex = 0;
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	map<string,string> value;	

	if(acqmul==UNI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"62",2);

	}

	else
	{
		value["BODY_HEADER"]="";
		value["L2_C"]="INYECTANDO";
		value["L3_C"]="LLAVE MULTI";
		uiInvokeURL(value,"GenScreen_info.html");
		sleep(SHOW_ALERT);
		
		LOGF_TRACE("******inDoY11BBVA*******\n");
		
		in_ex = inGetByteValuePP((char*)"TOKEX",ch_ex);
		
		in_val = in_cpx_ipk_sav(ch_ex, in_ex);
		
		LOGF_TRACE("******inDoY11BBVA [%d]*******\n",in_val);
		
		if(in_val == VS_SUCCESS)
		{
			debug("******inDoY11BBVA SUCCESS*******\n");
			value["L2_C"]="EXITOSO";
			value["L3_C"]="";
			uiInvokeURL(value,"GenScreen_info.html");
			strcat(chCodResp,"00");
		}
		else
		{
			debug("******inDoY11BBVA* FAIL*******\n");
			
			if(in_val == CPX_ERC_CRC)
				strcat(chCodResp,"12");
			else if(in_val == CPX_ERC_CKV )
				strcat(chCodResp,"13");
			else
				strcat(chCodResp,"99");	

			value["L2_C"]="FALLIDO";
			value["L3_C"]="";
			uiInvokeURL(value,"GenScreen_info.html");		
			
			debug("******inDoY11BBVA FAIL [%s]*******\n",chCodResp);
		}
			
		vdSetByteValuePP((char*)"STATUS",chCodResp,2);
	
	}

	in_val = PP_CMD_Pack((char*)"Y11",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		debug("******inDoY11BBVA PACK*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}


int requestTransportKeyZ10(char *chOutCmdData)
{
	char ch_es[1024] =  {0};
	char ch_ew[1024] = {0};
	char LenHex[2]    =  {0};
	char lenfld =  0;
	int in_es     = 0;
	int in_ew    = 0;
	int inLong  = 0;
	int in_val    = 0;
	int inTry     = 3;
	int mov      = 0;

	map<string,string> value;	

	if(acqmul==UNI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"62",2);
	}

	else
	{
		//value["HEADER_TITLE"]= "COMANDO";
		value["BODY_HEADER"]="";
		value["L2_C"]= "SOLICITANDO";
		value["L3_C"]= "LLAVE";
		
		uiInvokeURL(value,"GenScreen_info.html");

		sleep(SHOW_ALERT);			
		LOGF_TRACE("******requestTransportKeyZ10*******\n");
		in_val = in_cpx_ipk_ktk (ch_es, &in_es, ch_ew, &in_ew);
		 
	//	LOGF_TRACE("******requestTransportKeyZ10 [%d]*******\n",in_val);
		if(in_val == VS_SUCCESS)
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			vdSetByteValuePP((char*)"TOKEW",ch_ew,in_ew);
			vdSetByteValuePP((char*)"TOKES",ch_es,in_es);
			LOGF_TRACE("******TOKEW [%i]*******\n",in_ew);
			LOGF_TRACE("******TOKES  [%i]*******\n",in_es);
			mov = in_ew + in_es ;
			LOGF_TRACE("****** valor de MOV   [%i]*******\n",mov);		
			lenfld = 0;
			lenfld |= mov;	//length
			LenHex[1] = lenfld;
			lenfld = 0;
			lenfld |= mov >> 8;
			LenHex[0] = lenfld;
			vdSetByteValuePP((char*)"LENGH", LenHex,2);
		
			
		}
		else
		{
			//Nothing
		}
	}	

	in_val = PP_CMD_Pack((char*)"Z10",chOutCmdData,&inLong);

	if(in_val == XML_CMD_OK)
	{
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}

int injectKeyZ11(char *chOutCmdData)
{

	char ch_ex[1024] = {0};
	char chCodResp[5+1] ={0};
	int in_ex = 0;
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	map<string,string> value;	

	if(acqmul==UNI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"62",2);

	}

	else
	{
		value["BODY_HEADER"]="";
		value["L2_C"]="INYECTANDO";
		value["L3_C"]="LLAVE";
		uiInvokeURL(value,"GenScreen_info.html");
		sleep(SHOW_ALERT);
		
		LOGF_TRACE("******injectKeyZ11*******\n");
		
		in_ex = inGetByteValuePP((char*)"TOKEX",ch_ex);
		
		in_val = in_cpx_ipk_sav(ch_ex, in_ex);
		
		LOGF_TRACE("******injectKeyZ11 [%d]*******\n",in_val);
		
		if(in_val == VS_SUCCESS)
		{
			debug("******injectKeyZ11 SUCCESS*******\n");
			value["L2_C"]="EXITOSO";
			value["L3_C"]="";
			uiInvokeURL(value,"GenScreen_info.html");
			strcat(chCodResp,"00");
		}
		else
		{
			debug("******injectKeyZ11* FAIL*******\n");
			
			if(in_val == CPX_ERC_CRC)
				strcat(chCodResp,"12");
			else if(in_val == CPX_ERC_CKV )
				strcat(chCodResp,"13");
			else
				strcat(chCodResp,"99");	

			value["L2_C"]="FALLIDO";
			value["L3_C"]="";
			uiInvokeURL(value,"GenScreen_info.html");		
			
			debug("******injectKeyZ11 FAIL [%s]*******\n",chCodResp);
		}
			
		vdSetByteValuePP((char*)"STATUS",chCodResp,2);
	
	}

	in_val = PP_CMD_Pack((char*)"Z11",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		debug("******injectKeyZ11 Send*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}

int parse14Command(char receivedCommand[], char receivedBuffer[], int bufferLength){
	int error=0;
	LOGF_TRACE("ES UN COMANDO 14. PARSEO APARTE");

	if(memcmp(receivedCommand,"C14",3)==0){
		LOGAPI_HEXDUMP_TRACE("---TOKEN_ET----",receivedBuffer + 4,bufferLength-6);
		vdSetByteValuePP((char *)"TOKEN_ET",receivedBuffer + 4,bufferLength-6);
		error=SUCCESS;
	}
	else if(memcmp(receivedCommand,"SE14",4)==0){
		LOGAPI_HEXDUMP_TRACE("---TOKEN_ET----",receivedBuffer + 5,bufferLength-7);
		vdSetByteValuePP((char *)"TOKEN_ET",receivedBuffer + 5,bufferLength-7);
		error=SUCCESS;
	}
	else{
		error=FAILURE;
	}
	
	return error;

}

int exceptionBINList14(char *chOutCmdData)
{

	char tokenET[1024] = {0};
	char status[5+1] ={0};
	int tokenETLength = 0;
	int inLong = 0;
	int error = 0;
	int inTry = 3;
	map<string,string> value;	


	value["BODY_HEADER"]="";
	value["L2_C"]="INYECTANDO";
	value["L3_C"]="TOKEN ET";
	uiInvokeURL(value,"GenScreen_info.html");
	sleep(SHOW_ALERT);
	
	LOGF_TRACE("******exceptionBINList14*******\n");
	
	tokenETLength = inGetByteValuePP((char*)"TOKEN_ET",tokenET);
	
	LOGAPI_HEXDUMP_TRACE("*** TOKEN ET *** ",tokenET,tokenETLength );

	error = in_cpx_crd_exc(tokenET,tokenETLength);
			
	LOGF_TRACE("******exceptionBINList14 [%d]*******\n",error);
	
	if(error == VS_SUCCESS)
	{
		debug("******exceptionBINList14 SUCCESS*******\n");
		value["L2_C"]="EXITOSO";
		value["L3_C"]="";
		uiInvokeURL(value,"GenScreen_info.html");
		strcat(status,"00");
	}
	else
	{
		debug("******exceptionBINList14* FAIL*******\n");
		
		if(error == CPX_ERC_CRC)
			strcat(status,"12");
		else if(error == CPX_ERC_CKV )
			strcat(status,"13");
		else
			strcat(status,"99");	

		value["L2_C"]="FALLIDO";
		value["L3_C"]="";
		uiInvokeURL(value,"GenScreen_info.html");		
		
		debug("******exceptionBINList14 FAIL [%s]*******\n",status);
	}
		
	vdSetByteValuePP((char*)"STATUS",status,2);
	
	if(isSECommand()){
		error = PP_CMD_Pack((char*)"SE14",chOutCmdData,&inLong);
	}else{
		error = PP_CMD_Pack((char*)"C14",chOutCmdData,&inLong);
	}
	
	if(error == XML_CMD_OK)
	{
		debug("******exceptionBINList14 Send*******\n");
		error = inSendAdk(chOutCmdData,inLong);
		if(error == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return error;
	
}

int inDoC14BBVA(char *chOutCmdData)
{
	int inRetVal;
	char chRsp[5+1] = {0};
	char *pchCod;
	int infld;
	char szfldval[256] = {0};
	int in_val = 0;
	int inLong = 0;
	int inTry = 3;
	map<string,string> value;	
	
	LOGF_TRACE("******inDoC14 BANORTE*******\n");

	inRetVal = SUCCESS;
	if (inRetVal == SUCCESS)
		{
			inRetVal = in_c14_dsm ();
		}

	memset(chRsp, 0, sizeof(chRsp));
	pchCod = (char *)"00";
	if (inRetVal < SUCCESS)
	pchCod = (char *)"53";

	memcpy(chRsp, pchCod, 2);
	if (inRetVal == SUCCESS)
		{
			inRetVal = in_exc_dsm (chRsp);
		}

	if (inRetVal == SUCCESS)
		{
			inRetVal = in_Crp_Exc_Sav (chRsp);
		}

//	if (inRetVal == SUCCESS)
//		{
				//in_dsp_rw2 (MSG_TABLA_DE_BINES, MSG_RECIBIDA);
				// mostrar en pantalla
//		}
			

	if(inRetVal == SUCCESS)
	{
		
		if (bo_exc_avl())		// AJM 26/08/2014 1
			{
				//vdACPL_SetDefaultIdlePrompt ("");
				//inSaveConfRec (gszConfigFileName, SIZE_APP_CONF_REC, 0, (char *) &gstAppConfig);
				//iGCL_DisplayIdlePrompt ();
				LOGF_TRACE("***BO_EXC_AVL***");
			}
		else
			{
				in_ipk_wrn ();
			}
		
	}

	if ( !memcmp (chRsp, "00", 2) )		// AJM 07/10/2014 1
		{
			LOGF_TRACE("****NO ERROR***");
			value["L2_C"]="TABLA DE BINES\n RECIBIDA";
			uiInvokeURL(0,value,"GenScreen_info.html");
			
			sleep(SHOW_ALERT);
	
		}

	else
		{

			value["L2_C"]="ERROR TABLA DE BINES";
			uiInvokeURL(value,"GenScreen_info.html");
			sleep(SHOW_ALERT);

		}
	
			//memcpy (chRsp, "55", 2);

			//memcpy (gvcTx + giTxSize, chRsp, 2);
			//pdump  (gvcTx + giTxSize, 2, "rsp");
			//giTxSize += 2;

			//pdump  (gvcTx, giTxSize, "gvcTx");

	LOGF_TRACE("**** STATUS = [%s]",chRsp);
	vdSetByteValuePP((char*)"STATUS",chRsp,2);

	in_val = PP_CMD_Pack((char*)"C14",chOutCmdData,&inLong);

			
	if(in_val == XML_CMD_OK)
	{
		LOGF_TRACE("******inDoC14BBVA PACK*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
			{	
				//in_val = inWaitAck(chOutCmdData,inLong,inTry);
			}

	}


}

int inDoCA6Bnmx(char *chOutCmdData)
{

	char chCodResp[5+1] ={0};
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	
	char chBinId [CRD_EXC_NAM+1] = {0};
	char chBinVer [CRD_EXC_VER+1] = {0};
	char chBinRange [1024] = {0};
	
	debug("******inDoCA6Bnmx*******\n");
	
	in_val = inAskBinTbl(chBinId,chBinVer,chBinRange);
	
	debug("in_val [%d]\n",in_val);
	debug("chBinId [%s]\n",chBinId);
	debug("chBinVer [%s]\n",chBinVer);
	debug("chBinRange [%s]\n",chBinRange);
	if (in_val > 0)
	{
		vdSetByteValuePP((char*)"IDBINES",chBinId,strlen(chBinId));
		vdSetByteValuePP((char*)"VERTABLE",chBinVer,strlen(chBinVer));
		vdSetByteValuePP((char*)"MINBINMAX",chBinRange,strlen(chBinRange));
	}
	

	in_val = PP_CMD_Pack((char*)"CA6",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		debug("******inDoCA6Bnmx PACK*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}

int inDoCA8Bnmx(char *chOutCmdData)
{

	char chCodResp[5+1] ={0};
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	
	char chES [100] = {0};
	char chQ8 [100] = {0};
	
	
	debug("******inDoCA8Bnmx*******\n");
	
	//TOKEN ES CAPX
	inAsmQs(chES,FALSE);
	vdSetByteValuePP((char*)"TOKENES",chES,70);
	//TOKEN Q8 BNMX
	inAsmBnmxQ8(chQ8);
	vdSetByteValuePP((char*)"TOKENQ8",chQ8,36);

	in_val = PP_CMD_Pack((char*)"CA8",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		debug("******inDoCA8Bnmx PACK*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	return in_val;
	
}

int inDoC51BBVA(char *chOutCmdData)
{
	
	char chCodResp[5+1] ={0};
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	int MSRead = 0;	
	unsigned char Dev_type = 0; 
	//char chHeaderIdle [50] = {0};
	char stHeaderMsg[50] = {0};
	char stDispMsg[50] = {0};
	char stBodyMsg[50] = {0};
	
	//int inFlagManual = 1;
	int inFlagManual = 0; // FAG 03-MAR-2017 cancelamos ingreso manual en detectinput
	unsigned char Tech_selected = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	unsigned char dataBuffer[2056] = {0};
	unsigned short dataBufferLength = sizeof(dataBuffer);	
	map<string,string> value;

	//Valores que llegan del comado C51	
	char Timeout[3] = {0};
	int inTimeOut = 15;
	char chGetElements [50] = {0};
	int sztimeout = 0;	
	
	unsigned char txntype = 0;
	int sztxntype = 0;
	char Amount[14]={0};  //para CTLS
	int szAmount = 0;	  //Para CTLS	
	unsigned int AmountAux = 0;  //Para CTLS
	unsigned char AmountBCD[14] = {0};  //Para CTLS
	int szAmountBCD = 0;    //Para CTLS
	unsigned char cashback[14] = {0};
	int szCashBack = 0;	
	unsigned int CashBackAux = 0;
	int i = 0;
	
	//3 Timeout Control
	unsigned long secstart=0L;		//Daee 07/06/2018
	unsigned long inNewTime=0L;
	unsigned long secend=0L;

	LOGF_TRACE("****** inDoC51 Command *******");

	
	//EMV CHIP Structures
	memset((void*)&xSelectResCT,0x00,sizeof(xSelectResCT)); // result of the contact selection process
	memset((void*)&AdditionalTxDataCT,0x00,sizeof(AdditionalTxDataCT));
	memset((void*)&TxDataCT,0x00,sizeof(TxDataCT)); //CT Data. Parameters for start transaction (application selection processing)
	memset((void*)&xEMVTransResCT,0x00,sizeof(xEMVTransResCT));
	memset((void*)&xOnlineInputCT,0x00,sizeof(xOnlineInputCT));
	memset((void*)&pxTransRes,0x00,sizeof(pxTransRes));

	//EMV CTLS Structures

	memset((void*)&TxStartInputCTLS,0x00,sizeof(TxStartInputCTLS)); // result of the contact selection process
	memset((void*)&xSelectResCTLS,0x00,sizeof(xSelectResCTLS)); // result of the contact selection process
	memset((void*)&CTLSTransRes,0x00,sizeof(CTLSTransRes)); // result of the contact selection process
	memset((void*)&xOnlineInputCTLS,0x00,sizeof(xOnlineInputCTLS)); // result of the contact selection process

	
	LOGF_TRACE("\n****** inDoC51 *******\n");

	Dev_type |= CTS_CHIP|CTS_MSR;

	in_val = check_key();
	
	if(acqmul==UNI_ADQUIRIENTE  && !isMacroCmdUni)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"62",2);  /* Set field for response C53 Value 62 = comando no valido */
		in_val = PP_CMD_Pack((char*)"C53",chOutCmdData,&inLong);
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDoC51_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	
		LOGF_TRACE("in_val = %i", in_val);
		LOGF_TRACE("--End inDoC51BBVA--");
		return in_val;
	}

	else if(in_val == CPX_ERC_IPK   && !isMacroCmdUni)
	{
		LOGF_TRACE("******inDoC51*******");
		LOGF_TRACE("\n******NO KEY*******\n");
		
		show_error(in_val);
		
		in_val = PP_CMD_Pack((char*)"C53",chOutCmdData,&inLong);
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDoC51_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	
		LOGF_TRACE("in_val = %i", in_val);
		LOGF_TRACE("--End inDoC51BBVA--");
		return in_val;
	}
	else
	{	
		memset(chGetElements,0,sizeof(chGetElements));
		sztimeout = inGetByteValuePP((char*)"TIMEOUT",chGetElements);
		vdHex2Asc(chGetElements,Timeout,sztimeout);
		inTimeOut = atoi(Timeout);
		LOGF_TRACE("\ninTimeOut = %i",inTimeOut);
		/*RETRIEVE TRANSTYP*/
		sztxntype = inGetByteValuePP((char*)"TRANSTYP",(char*)&txntype);
		LOGF_TRACE("\nTRANSTYP = %2x",txntype);
		
		txntype = 0;
		sztxntype = inGetByteValuePP((char*)"PMOVILCORRESP",(char*)&txntype);
		LOGF_TRACE("--PMOVILCORRESP [%x]--",txntype);
		
		if( (txntype & KBD_ACT) > 0)
		{
			LOGF_TRACE("--ACTIVAMOS INGRESO MANUAL para C51--");
			chTechnology = CTS_KBD;
			value["HEADER_TITLE"]="";
			value["BODY_HEADER"]="";
			value["BODY_MSG"]="";
			
			uiInvokeURL(value,"banorte_movil.html");
			sleep(1);
			in_val = KBD_Transaction_banorte(chManual, strlen(chManual), KBD_C51);
			LOGF_TRACE("-- in_val = [%i]", in_val);

			if(in_val < CTS_OK)
				show_error(in_val);

		}
		else
		{
			strcpy(stHeaderMsg,"");
			strcpy(stDispMsg,"INSERTE DESLICE");
			strcpy(stBodyMsg,"TARJETA");
			
			if(boCtlsON) //if Ctls Enable
			{	
				//Se establece el monto para CTLS
				memset(Amount,0,sizeof(Amount));
				
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

				LOGF_TRACE("\nAmount: %s",AmountBCD);
			
			
				usleep(3000);
				vdSetCTLSTransactionData(&TxStartInputCTLS,(char*)AmountBCD);

				/*in_val = ActivateTransactionCTLS(&TxStartInputCTLS,&xSelectResCTLS);
				debug("\nDespu�s del Active transaction");
				if(in_val != EMV_ADK_OK)
				{
					debug("\nError in Setup Transaction: %X",in_val);
					return in_val;
				}*/
				
				Dev_type |= CTS_CTLS;

				memset (stBodyMsg , 0 ,sizeof stBodyMsg);
				strcpy(stBodyMsg,"APROXIME TARJETA");
			}
			
			//Lector banda magn�tica
			LOGF_TRACE("\nMSR ACTIVATION");
			MSRead = Enable_swp();
			LOGF_TRACE("\nMSR ACTIVATION = %i",MSRead);

			
			do
			{	
				//Daee 07/06/2018
				//3 Timeout Control
				//JRS 15022018
                if(secstart == 0)
					secstart = time (NULL);
				
                LOGF_TRACE("****TIME OUT [%d]****",inTimeOut);                                                    

                LOGF_TRACE("****NEW TIME OUT [%d]****",inNewTime);                                                       

                if(inNewTime != 0 )
                {
					if(inNewTime > inTimeOut)
						inTimeOut = 1;
					else
	                {
	                    inTimeOut = inTimeOut - inNewTime;
	                    secstart = time (NULL);
	                    secend = 0;
	                }
	                inNewTime = 0;
                }
				//3 Timeout Control

				if(Dev_type & CTS_CTLS) //if Ctls Enable
				{
					in_val = ActivateTransactionCTLS(&TxStartInputCTLS,&xSelectResCTLS);
					
					debug("in_val = %d",in_val);

					if(in_val != EMV_ADK_OK)
					{
						debug("\nError in Setup Ctls Transaction: %X",in_val);
						return in_val;
					}
				}
				
				in_val = Detect_input(Dev_type, &Tech_selected, inFlagManual,inMin, inMax, chManual, inTimeOut,
									dataBuffer,&dataBufferLength,stHeaderMsg,stDispMsg,stBodyMsg); 

				LOGAPI_HEXDUMP_TRACE("DATOS DESPU�S EL DETECT INPUT",dataBuffer,dataBufferLength);

				chTechnology = Tech_selected;
				LOGF_TRACE("\nTech_selected = %i",Tech_selected);
				LOGF_TRACE("\nUsed_techology = %i",chTechnology);
				LOGF_TRACE("\nin_val = %i",in_val);

				if( in_val == CTS_NO_CHIP ||  in_val == CTS_ERROR) 
				{
					LOGF_TRACE("caso fallback");
					//count_fallback+=1;
					count_fallback_inc();

					LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

					value.clear();
					value["HEADER_TITLE"]="";
					value["BODY_HEADER"]="FALLA EN CHIP";
					value["BODY_MSG"]="";
					value["NAME_IMG"]="EXTCHIPERROR";
					
					uiInvokeURL(value,"GenScreen_alert.html");
					sleep(SHOW_ALERT);

					Remove_Card();

					if(Dev_type & CTS_CTLS) //if Ctls Enable
					{
						EMV_EndTransactionCTLS("", 0, false);
					}
					
					//if(count_fallback>2)
					if(count_fallback_get() > 1)
					{
						 Dev_type = CTS_MSR;
					}

					in_val=FALLA_CHIP;
				}
				else if(in_val < CTS_OK)
				{
					show_error(in_val);
				}
				else if(in_val == CTS_OK)
				{
					switch(chTechnology)
					{
						case CTS_MSR:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from MAGNETIG STRIPE READER");
							in_val = MSR_Transaction(MSRead);
						break;

						case CTS_CHIP:
						case CTS_CHIP|CTS_MSR: //Detection improve for Ux if not remove card and detecting two technologies  //Daee 27/10/2017
							chTechnology &= ~CTS_MSR; //Clear Msr Flag
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from EMV CHIP");
							in_val = EMV_Transaction();
							if(in_val == EMV_ADK_FALLBACK)
							{
									LOGF_TRACE("caso fallback");
									//count_fallback+=1;
									count_fallback_inc();

									LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

									value.clear();
									value["HEADER_TITLE"]="";
									value["BODY_HEADER"]="FALLA EN CHIP";
									value["BODY_MSG"]="";
									value["NAME_IMG"]="EXTCHIPERROR";
									
									uiInvokeURL(value,"GenScreen_alert.html");
									sleep(SHOW_ALERT);

									Remove_Card();

									if(Dev_type & CTS_CTLS) //if Ctls Enable
									{
										EMV_EndTransactionCTLS("", 0, false);
									}
									
									//if(count_fallback>2)
									if(count_fallback_get() > 1)
									{
										 Dev_type = CTS_MSR;
									}

									in_val=FALLA_CHIP;
							}
							else
							{
								build_C53_C54Offline();
							}
						break;

						case CTS_CTLS:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from CONTACTLESS READER");
							in_val = CTLS_Transaction();

							build_C53_C54Offline();
						break;			

						default:
						break;
					}

					switch (in_val)
					{
						case DATA_NOT_FOUND:
							LOGF_TRACE("DATA_NOT_FOUND");
						case INVALID_MSR_DATA:
							LOGF_TRACE("INVALID_MSR_DATA");
							value.clear();
							value["HEADER_TITLE"]="ERROR";
							value["BODY_HEADER"]="FALLO LECTURA";
							value["BODY_MSG"]="REINTENTE";
							value["NAME_IMG"]="error_icon";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							
							break;
						case USE_CHIP:
							LOGF_TRACE("USE_CHIP");
							value.clear();
							value["HEADER_TITLE"]="";
							value["BODY_HEADER"]="UTILICE LECTOR";
							value["BODY_MSG"]="DE CHIP";
							value["NAME_IMG"]="EXTUSECHIPREAD";

							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_ABORT:
							LOGF_TRACE("EMV_CT_PIN_INPUT_ABORT");
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN CANCELADO";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_TIMEOUT:
							LOGF_TRACE("EMV_CT_PIN_INPUT_TIMEOUT");
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN TIMEOUT";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						default:
							show_error(in_val);
							break;
					}
	
				}

				//Daee 07/06/2018
				//3 Timeout Control
				//JRS NEW TIME OUT 15022018
                secend = time (NULL);
                inNewTime = (secend - secstart);
				//3 Timeout Control

		    }while( (in_val==USE_CHIP) || (in_val==DATA_NOT_FOUND) ||(in_val==INVALID_MSR_DATA) || (in_val==FALLA_CHIP) );		

		}

	
		LOGF_TRACE("in val = [%i]",in_val);
		if(in_val == SUCCESS_TRX)
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			value["HEADER_TITLE"]="COMANDO";
			value["BODY_HEADER"]="PROCESANDO";
			value["BODY_MSG"]="ESPERE UN MOMENTO";
			
			uiInvokeURL(value,"Procesando.html");

			sleep(SHOW_ALERT);
		}		
		else if(in_val == EMV_ADK_ABORT)
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
		}
		else if((in_val == EMV_CT_PIN_INPUT_ABORT) || (in_val == EMV_CT_PIN_INPUT_TIMEOUT))
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"23",2);				
		}

	}
		
	in_val = PP_CMD_Pack((char*)"C53",chOutCmdData,&inLong);

	//4 check Macrocomando Uniadquirencia

	if(isMacroCmdUni) //if MacroCmd Uni
		reloadMacroComandoUni (chOutCmdData);

	//::::::::::::::::::::::::::::::::::::
	
	LOGF_TRACE("\n******Enviando*******\n");
	in_val = inSendAdk(chOutCmdData,inLong);
	if(in_val == inLong)
	{	
		LOGF_TRACE("\n******inDoC51_Ack*******\n");
		//in_val = inWaitAck(chOutCmdData,inLong,inTry);
	}
	
	MSRead = Disable_swp();
	LOGF_TRACE("\nMSRead = %i",MSRead);
	
	LOGF_TRACE("\n");
	LOGF_TRACE("in_val = %i", in_val);
	LOGF_TRACE("--End inDoC51BBVA--");
	clear_parse();
	count_fallback_zero();
	return in_val;
	
	
}

int initTransaction51(char *chOutCmdData) //NRC X-18
{
	char chCodResp[5+1] ={0};
	int inLong = 0;
	int in_val = 0;
	int inTry = 3;
	int MSRead = 0;	
	unsigned char Dev_type = 0; 
	//char chHeaderIdle [50] = {0};
	char stHeaderMsg[50] = {0};
	char stDispMsg[50] = {0};
	char stBodyMsg[50] = {0};
	
	//int inFlagManual = 1;
	int inFlagManual = 0; // FAG 03-MAR-2017 cancelamos ingreso manual en detectinput
	unsigned char Tech_selected = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	unsigned char dataBuffer[2056] = {0};
	unsigned short dataBufferLength = sizeof(dataBuffer);	
	map<string,string> value;

	//Valores que llegan del comado 51	
	char Timeout[3] = {0};
	int inTimeOut = 15;
	char chGetElements [50] = {0};
	int sztimeout = 0;	
	
	unsigned char txntype = 0;
	int sztxntype = 0;
	char Amount[14]={0};  //para CTLS
	int szAmount = 0;	  //Para CTLS	
	unsigned int AmountAux = 0;  //Para CTLS
	unsigned char AmountBCD[14] = {0};  //Para CTLS
	int szAmountBCD = 0;    //Para CTLS
	unsigned char cashback[14] = {0};
	int szCashBack = 0;	
	unsigned int CashBackAux = 0;
	int i = 0;
	
	//3 Timeout Control
	unsigned long secstart=0L;		//Daee 07/06/2018
	unsigned long inNewTime=0L;
	unsigned long secend=0L;

	LOGF_TRACE("****** initTransaction51 *******");

	
	//EMV CHIP Structures
	memset((void*)&xSelectResCT,0x00,sizeof(xSelectResCT)); // result of the contact selection process
	memset((void*)&AdditionalTxDataCT,0x00,sizeof(AdditionalTxDataCT));
	memset((void*)&TxDataCT,0x00,sizeof(TxDataCT)); //CT Data. Parameters for start transaction (application selection processing)
	memset((void*)&xEMVTransResCT,0x00,sizeof(xEMVTransResCT));
	memset((void*)&xOnlineInputCT,0x00,sizeof(xOnlineInputCT));
	memset((void*)&pxTransRes,0x00,sizeof(pxTransRes));

	//EMV CTLS Structures

	memset((void*)&TxStartInputCTLS,0x00,sizeof(TxStartInputCTLS)); // result of the contact selection process
	memset((void*)&xSelectResCTLS,0x00,sizeof(xSelectResCTLS)); // result of the contact selection process
	memset((void*)&CTLSTransRes,0x00,sizeof(CTLSTransRes)); // result of the contact selection process
	memset((void*)&xOnlineInputCTLS,0x00,sizeof(xOnlineInputCTLS)); // result of the contact selection process

	updateDate();
	
	LOGF_TRACE("\n****** initTransaction51 *******\n");

	Dev_type |= CTS_CHIP|CTS_MSR;

	in_val = check_key();

	if(in_val == CPX_ERC_IPK   && !isMacroCmdUni)
	{
		LOGF_TRACE("******initTransaction511*******");
		LOGF_TRACE("\n******NO KEY*******\n");
		
		show_error(in_val); //Show and set STATUS for command that will be sent.
		if(isSECommand()){
			in_val = PP_CMD_Pack((char*)"SE53",chOutCmdData,&inLong);
		}
		else{
			in_val = PP_CMD_Pack((char*)"C53",chOutCmdData,&inLong);
		}
		
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******initTransaction51*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	
		LOGF_TRACE("in_val = %i", in_val);
		LOGF_TRACE("--End initTransaction51--");
		return in_val;
	}
	else
	{	
		memset(chGetElements,0,sizeof(chGetElements));
		sztimeout = inGetByteValuePP((char*)"TIMEOUT",chGetElements);
		vdHex2Asc(chGetElements,Timeout,sztimeout);
		inTimeOut = atoi(Timeout);
		LOGF_TRACE("\ninTimeOut = %i",inTimeOut);
		/*RETRIEVE TRANSTYP*/
		sztxntype = inGetByteValuePP((char*)"TRANSTYP",(char*)&txntype);
		LOGF_TRACE("\nTRANSTYP = %2x",txntype);
		
		
		if( (txntype & KBD_ACT) > 0)
		{
			LOGF_TRACE("--ACTIVAMOS INGRESO MANUAL para Comando 51--");
			chTechnology = CTS_KBD;
			value["HEADER_TITLE"]="";
			value["BODY_HEADER"]="";
			value["BODY_MSG"]="";
			
			uiInvokeURL(value,"banorte_movil.html");
			sleep(1);
			in_val = KBD_Transaction_banorte(chManual, strlen(chManual), KBD_C51);
			LOGF_TRACE("-- in_val = [%i]", in_val);

			if(in_val < CTS_OK)
				show_error(in_val);

		}
		else
		{
			strcpy(stHeaderMsg,"");
			strcpy(stDispMsg,"INSERTE DESLICE");
			strcpy(stBodyMsg,"TARJETA");
			
			if(boCtlsON) //if Ctls Enable
			{	
				//Se establece el monto para CTLS
				memset(Amount,0,sizeof(Amount));
				
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

				LOGF_TRACE("\nAmount: %s",AmountBCD);
			
			
				usleep(3000);
				vdSetCTLSTransactionData(&TxStartInputCTLS,(char*)AmountBCD);

				/*in_val = ActivateTransactionCTLS(&TxStartInputCTLS,&xSelectResCTLS);
				debug("\nDespu�s del Active transaction");
				if(in_val != EMV_ADK_OK)
				{
					debug("\nError in Setup Transaction: %X",in_val);
					return in_val;
				}*/
				
				Dev_type |= CTS_CTLS;

				memset (stBodyMsg , 0 ,sizeof stBodyMsg);
				strcpy(stBodyMsg,"APROXIME TARJETA");
			}
			
			//Lector banda magn�tica
			LOGF_TRACE("\nMSR ACTIVATION");
			MSRead = Enable_swp();
			LOGF_TRACE("\nMSR ACTIVATION = %i",MSRead);

			if(!flagZ4)
			{
				count_fallback_zero();
				
			}

			do
			{	



				if(flagZ4 && chTechnology != CTS_CTLS )
				{
					//in_val = inCheckRemovedCard(); 
					LOGF_TRACE("Z4 chTechnology [%d]", chTechnology);
					flagZ4 = 0;
					inTransInProgress = 1;

					if(secstart == 0)
						secstart = time (NULL);
				}	
				else {


					//Daee 07/06/2018
					//3 Timeout Control
					//JRS 15022018
	                if(secstart == 0)
						secstart = time (NULL);
					
	                LOGF_TRACE("****TIME OUT [%d]****",inTimeOut);                                                    

	                LOGF_TRACE("****NEW TIME OUT [%d]****",inNewTime);                                                       

	                if(inNewTime != 0 )
	                {
						if(inNewTime > inTimeOut)
							inTimeOut = 1;
						else
		                {
		                    inTimeOut = inTimeOut - inNewTime;
		                    secstart = time (NULL);
		                    secend = 0;
		                }
		                inNewTime = 0;
	                }
					//3 Timeout Control

					if(Dev_type & CTS_CTLS) //if Ctls Enable
					{
						in_val = ActivateTransactionCTLS(&TxStartInputCTLS,&xSelectResCTLS);
						
						debug("in_val = %d",in_val);

						if(in_val != EMV_ADK_OK)
						{
							debug("\nError in Setup Ctls Transaction: %X",in_val);
							return in_val;
						}
					}
					
					in_val = Detect_input(Dev_type, &Tech_selected, inFlagManual,inMin, inMax, chManual, inTimeOut,
										dataBuffer,&dataBufferLength,stHeaderMsg,stDispMsg,stBodyMsg); 

					LOGAPI_HEXDUMP_TRACE("DATOS DESPU�S EL DETECT INPUT",dataBuffer,dataBufferLength);

					chTechnology = Tech_selected;
				}
				LOGF_TRACE("\nTech_selected = %i",Tech_selected);
				LOGF_TRACE("\nUsed_techology = %i",chTechnology);
				LOGF_TRACE("\nin_val = %i",in_val);

				if( in_val == CTS_NO_CHIP ||  in_val == CTS_ERROR) 
				{
					LOGF_TRACE("caso fallback");
					//count_fallback+=1;
					count_fallback_inc();

					LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

					value.clear();
					value["HEADER_TITLE"]="";
					value["BODY_HEADER"]="FALLA EN CHIP";
					value["BODY_MSG"]="";
					value["NAME_IMG"]="EXTCHIPERROR";
					
					uiInvokeURL(value,"GenScreen_alert.html");
					sleep(SHOW_ALERT);

					Remove_Card();

					if(Dev_type & CTS_CTLS) //if Ctls Enable
					{
						EMV_EndTransactionCTLS("", 0, false);
					}
					
					//if(count_fallback>2)
					if(count_fallback_get() > 1)
					{
						 Dev_type = CTS_MSR;
					}

					in_val=FALLA_CHIP;
				}
				else if(in_val < CTS_OK)
				{
					show_error(in_val);
				}
				else if(in_val == CTS_OK)
				{
					switch(chTechnology)
					{
						case CTS_MSR:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from MAGNETIG STRIPE READER");
							in_val = processMSRTransaction(MSRead,inTransInProgress);
							LOGF_TRACE("in_val{%d} = processMSRTransaction(MSRead);",in_val);
						break;

						case CTS_CHIP:
						case CTS_CHIP|CTS_MSR: //Detection improve for Ux if not remove card and detecting two technologies  //Daee 27/10/2017
							chTechnology &= ~CTS_MSR; //Clear Msr Flag
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from EMV CHIP");
							in_val = EMV_Transaction();
							if(in_val == EMV_ADK_FALLBACK)
							{
									LOGF_TRACE("caso fallback");
									//count_fallback+=1;
									count_fallback_inc();

									LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

									value.clear();
									value["HEADER_TITLE"]="";
									value["BODY_HEADER"]="FALLA EN CHIP";
									value["BODY_MSG"]="";
									value["NAME_IMG"]="EXTCHIPERROR";
									
									uiInvokeURL(value,"GenScreen_alert.html");
									sleep(SHOW_ALERT);

									Remove_Card();

									if(Dev_type & CTS_CTLS) //if Ctls Enable
									{
										EMV_EndTransactionCTLS("", 0, false);
									}
									
									//if(count_fallback>2)
									if(count_fallback_get() > 1)
									{
										 Dev_type = CTS_MSR;
									}

									in_val=FALLA_CHIP;
							}
							else
							{
								buildInitTransactionReply53();
							}
						break;

						case CTS_CTLS:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from CONTACTLESS READER");
							in_val = CTLS_Transaction();

							buildInitTransactionReply53();
						break;			

						default:
						break;
					}

					switch (in_val)
					{
						case DATA_NOT_FOUND:
						case INVALID_MSR_DATA:
							value.clear();
							value["HEADER_TITLE"]="ERROR";
							value["BODY_HEADER"]="FALLO LECTURA";
							value["BODY_MSG"]="REINTENTE";
							value["NAME_IMG"]="error_icon";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							
							break;
						case USE_CHIP:
							value.clear();
							value["HEADER_TITLE"]="";
							value["BODY_HEADER"]="UTILICE LECTOR";
							value["BODY_MSG"]="DE CHIP";
							value["NAME_IMG"]="EXTUSECHIPREAD";

							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_ABORT:
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN CANCELADO";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_TIMEOUT:
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN TIMEOUT";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						default:
							show_error(in_val);
							break;
					}
	
				}

				//Daee 07/06/2018
				//3 Timeout Control
				//JRS NEW TIME OUT 15022018
                secend = time (NULL);
                inNewTime = (secend - secstart);
				//3 Timeout Control

		    }while( (in_val==USE_CHIP) || (in_val==DATA_NOT_FOUND) ||(in_val==INVALID_MSR_DATA) || (in_val==FALLA_CHIP) );		

		}

	
		LOGF_TRACE("\nin val = [%i]",in_val);
		if(in_val == SUCCESS_TRX)
		{
			if(chTechnology==CTS_MSR){
				vdSetByteValuePP((char*)"STATUS",(char*)"21",2);
			}else{
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			}
			value["HEADER_TITLE"]="COMANDO";
			value["BODY_HEADER"]="PROCESANDO";
			value["BODY_MSG"]="ESPERE UN MOMENTO";
			
			uiInvokeURL(value,"Procesando.html");

			sleep(SHOW_ALERT);
		}		
		else if(in_val == EMV_ADK_ABORT)
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
		}
		else if((in_val == EMV_CT_PIN_INPUT_ABORT) || (in_val == EMV_CT_PIN_INPUT_TIMEOUT))
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"23",2);				
		}else 
		{
			vdSetByteValuePP((char*)"STATUS",(char*)"99",2);				
		}

	}
	if(isSECommand()){
		in_val = PP_CMD_Pack((char*)"SE53",chOutCmdData,&inLong);
	}else{
		in_val = PP_CMD_Pack((char*)"C53",chOutCmdData,&inLong);
	}
	//::::::::::::::::::::::::::::::::::::
	
	LOGF_TRACE("\n******Enviando*******\n");
	in_val = inSendAdk(chOutCmdData,inLong);
	if(in_val == inLong)
	{	
		LOGF_TRACE("\n******initTransaction51*******\n");
		//in_val = inWaitAck(chOutCmdData,inLong,inTry);
	}
	
	MSRead = Disable_swp();
	LOGF_TRACE("\nMSRead = %i",MSRead);
	
	LOGF_TRACE("\n");
	LOGF_TRACE("in_val = %i", in_val);
	LOGF_TRACE("--End initTransaction51--");
	clear_parse();
	count_fallback_zero();
	return in_val;
	
	
}


int inDoZ4BBVA(char* chOutCmdData)
{
	int in_val = 0;
	char chGetElements [50] = {0};
	char shSetElements [50] = {0};
	int szchSetElements = 0;
	char statuscode[2] = {0};
	int MSRead = 0;	

	//Variables para la funci�n Detect_input
	unsigned char dataBuffer[2056] = {0};
	int inTimeOut = 15;
	int inFlagManual = 1;
	//unsigned char chTechnology = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	unsigned short dataBufferLength = sizeof(dataBuffer);
	
	int inLong = 0;
	int inTry = 3;
	char AmountBCD[14]={0};
	map<string,string> value;
	flagZ4 = 0;

	LOGF_TRACE("\n******inDoZ4BBVA*******\n");


	memset(chGetElements,0,sizeof(chGetElements));
	
	inTimeOut = GetTransactionTimeout();
	LOGF_TRACE("\ninTimeOut = %i",inTimeOut);

	
	MSRead = Enable_swp();
		LOGF_TRACE("\nMSRead = %i",MSRead);


	AmountBCD[0]=0x31; //0.1
	vdSetCTLSTransactionData(&TxStartInputCTLS,(char*)AmountBCD);
	in_val = ActivateTransactionCTLS(&TxStartInputCTLS,&xSelectResCTLS);
	LOGF_TRACE("\nDespues del Active transaction");
	
	if(in_val != EMV_ADK_OK)
	{
		LOGF_TRACE("\nError in Setup Transaction: %X",in_val);
	}
		
		in_val = Detect_input(CTS_CHIP|CTS_MSR|CTS_CTLS, &chTechnology, inFlagManual,inMin, inMax, chManual, inTimeOut,
				dataBuffer,&dataBufferLength, (char *)"",(char *)"",(char*)"INSERTE DESLICE\nTARJETA"); // quitar los nulos, solo son de prueba para cmpilar


		LOGF_TRACE("\nUsed_techology = %i",chTechnology);
		LOGF_TRACE("\n in_val = %i",in_val);

		if(in_val == EMV_IDL_CANCEL)
		{
			//CANCELADA
			//SE AGREGA EL CPODIGO DE ERROR
			LOGF_TRACE("\nCancelada por el usuario");			
			memcpy(statuscode,"99",2);	//OTRA FALLA

		}
		else if(in_val == EMV_IDL_TIME_OUT)
		{
			//CANCELADA
			//SE AGREGA EL CPODIGO DE ERROR
			LOGF_TRACE("\nCancelada por timeout");
			memcpy(statuscode,"06",2);	//FALLA POR TIMEOUT
			//turn in_val;
		}
		else if(in_val == CTS_OK)
		{

			value["HEADER_TITLE"]="COMANDO";
			value["BODY_HEADER"]="PROCESANDO";
			value["BODY_MSG"]="ESPERE UN MOMENTO";
			
			uiInvokeURL(value,"Procesando.html");

			switch(chTechnology)
			{
				case CTS_MSR:
					LOGF_TRACE("\n");
					LOGF_TRACE("Input from MAGNETIG STRIPE READER");
					memset(shSetElements,0x00,sizeof(shSetElements));		
					in_val = read_PAN_MSR(MSRead,&shSetElements[2],&szchSetElements);
					shSetElements[0] = 0xC1;
					shSetElements[1] = szchSetElements;
					vdSetByteValuePP((char*)"PAN",shSetElements,szchSetElements + 2);
					if(in_val == MSR_OK){
						flagZ4 = 1;
						memcpy(statuscode,"00",2);
					}
					else
						memcpy(statuscode,"63",2);		//ERROR DE LECTURA				
					
				break;

				case CTS_CHIP:
					LOGF_TRACE("\n");
					LOGF_TRACE("Input from EMV CHIP");
					memset(shSetElements,0x00,sizeof(shSetElements));	
					szchSetElements = read_PAN_EMV(&shSetElements[2]);
					shSetElements[0] = 0xC1;
					shSetElements[1] = szchSetElements;
					//szchSetElements = strlen(shSetElements);					
					vdSetByteValuePP((char*)"PAN",shSetElements,szchSetElements+2);
					if( szchSetElements >= 0)
					{				
						flagZ4 = 1;		
						memcpy(statuscode,"00",2);		//LECTURA EXITOSA			
					}
					else
						memcpy(statuscode,"10",2);		//ERROR DE LECTURA DE CHIP

				break;

				case CTS_CTLS:
					LOGF_TRACE("Input from CTLS");
					memset(shSetElements,0x00,sizeof(shSetElements));	
					szchSetElements = read_PAN_CTLS(&shSetElements[2]);
					shSetElements[0] = 0xC1;
					shSetElements[1] = szchSetElements;
					vdSetByteValuePP((char*)"PAN",shSetElements,szchSetElements+2);
					if( szchSetElements >= 0)
					{
						flagZ4 = 1;
						memcpy(statuscode,"00",2);		//LECTURA EXITOSA
					}
					else
					{
						shSetElements[0] = 0xC1;
						shSetElements[1] = 0x00;
						memcpy(statuscode,"10",2);		//ERROR DE LECTURA DE CHIP
						vdSetByteValuePP((char*)"PAN",(char *)shSetElements,2); //JRS 04052018 - 237
					}
					break;

				default:
					
				break;
			}
		}	
			
	
		MSRead = Disable_swp();
		LOGF_TRACE("\nMSRead = %i\n",MSRead);
		
		LOGF_TRACE("\n");
		LOGF_TRACE("in_val = %i", in_val);

	in_val = check_key();

	if(in_val == CPX_ERC_IPK)
		memcpy(statuscode,"62",2);   //Verifica si no hay llave

	vdSetByteValuePP((char*)"STATUS",statuscode,2);  
	
	in_val = PP_CMD_Pack((char*)"Z4",chOutCmdData,&inLong);
		
		if(in_val == XML_CMD_OK)
		{

			LOGF_TRACE("******inDoZ4BBVA PACK*******\n");
			in_val = inSendAdk(chOutCmdData,inLong);
			if(in_val == inLong)
			{	
				in_val = inWaitAck(chOutCmdData,inLong,inTry);
			}
		}

	LOGF_TRACE("\n--End inDoZ4BBVA--\n");
	return in_val;
}


//********************************************************************************************************************************
/*
* @brief Processing for C54 command
* @param[in] chOutCmdData buffer

*******************************************************************************************************************************/
int inDoC54BBVA(char *chOutCmdData)
{
	
	int iResult = 0;
	//Variables que se requieren para procesar la informaci�n del comadno C54
	char Host_Response[3] = {0};
	int szHost_Response = 0;
	char Status_Host[3] = {0};
	int szStatus_Host;
	char AuthCode[7] = {0};
	int szAuthCode = 0;	
	char resp_code[5] = {0};
	int szresp_code = 0;
	char tg91[40] = {0};
	int sztg91 = 0;
	char txn_date[7]= {0};
	int sztxn_date = 0;
	char txn_time[7]= {0};
	int sztxn_time = 0;
	char tknE2[20] = {0};
	int sztknE2 = 0;

	int typctls = 0;

	//Variables para cerrar capitulo 10
	int  in_ath = FAILED_TO_CONNECT;

	//Variable para el status del Mensaje C54
	char Status[3] = {0};
	int szStatus = 0;

	int inLong = 0;
	int inTry = 0;
	//	map<string,string> value;  //Daee 18/06/2018  /*Reques AOV Command c54 does not show transaction result*/
	
	LOGF_TRACE("\n--inDoC54--\n");
	
	if(acqmul==UNI_ADQUIRIENTE  && !isMacroCmdUni)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char *)"62",2);	
		iResult = PP_CMD_Pack((char*)"C54",chOutCmdData,&inLong);
		if(iResult == XML_CMD_OK)
		{
			LOGF_TRACE("******inDoC54 BUID RESPONSE *******\n");
			iResult = inSendAdk(chOutCmdData,inLong);
			if(iResult == inLong)
			{	
				//iResult = inWaitAck(chOutCmdData,inLong,inTry);
			}
		}

		return iResult;
				  
	}
 
	szStatus_Host = inGetByteValuePP((char*)"STATUS",Status_Host);
	szHost_Response = inGetByteValuePP((char*)"HOSTRESPONSE",(char*)Host_Response);
	LOGF_TRACE(" HOSTRESPONSE received = %s ",Host_Response);
	szAuthCode = inGetByteValuePP((char*)"AUHTRES",(char*)AuthCode);
	LOGF_TRACE("AUHTRES received = %s",AuthCode);
	
	//szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
	//LOGF_TRACE("RESPCODE received = %s",resp_code);
	
	sztg91 = inGetByteValuePP((char*)"ISS_UTH",(char*)tg91);	
	
	sztxn_date = inGetByteValuePP((char*)"TRANSDATE",(char*)txn_date);		
	LOGF_TRACE("TRANSDATE received = %s",txn_date);
	sztxn_time = inGetByteValuePP((char*)"TRANSTIME",(char*)txn_time);		
	LOGF_TRACE("TRANSTIME received = %s",txn_time);
	sztknE2 = inGetByteValuePP((char*)"TOKE2",(char*)tknE2);
	
	LOGF_TRACE("+++++ Host  Response ++++");
	//   if(!memcmp(Host_Response,"00",szHost_Response))
	if(!memcmp(Status_Host,"\x00",szStatus_Host))
	{
		LOGF_TRACE(" HOST_AUTHORISED ");
		in_ath = HOST_AUTHORISED;		
	}
	//	else if(!memcmp(Host_Response,"01",szHost_Response))
	else if(!memcmp(Status_Host,"\x01",szStatus_Host))
	{
		in_ath = HOST_DECLINED;
		LOGF_TRACE(" HOST_DECLINED ");
		
	}
	//	else if(!memcmp(Host_Response,,szHost_Response))
	else if(!memcmp(Status_Host,"\x02",szStatus_Host))
	{
		in_ath = FAILED_TO_CONNECT;     
		LOGF_TRACE(" FAILED_TO_CONNECT ");
	}
	else	
	{
		in_ath = FAILED_TO_CONNECT;     
		LOGF_TRACE(" UNKNOWN HOST RESPONSE  ");
	}

	LOGF_TRACE("Antes de in_cpx_crd_end");
	
	in_cpx_crd_end (szHost_Response, !memcmp(Host_Response, "70", 2));

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		LOGF_TRACE("****Responde to EMV or CTLS CHIP transaction****");

		if(chTechnology == CTS_CHIP)		
			iResult = C54_EMV();		
		else 
			iResult = C54_CTLS();		

		iResult=_CHECK_INSERTED_CARD();
		LOGF_TRACE("_CHECK_INSERTED_CARD %X",iResult);
		if(iResult==TRUE)
		{
			LOGF_TRACE("****Pedimos que se retire la tarjeta****");
			Remove_Card();
			LOGF_TRACE("****tarjeta retirada****");
		}

	}
	
	else if( (chTechnology == CTS_MSR) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) ) )
	{
		LOGF_TRACE("****Resonse to MSR or TLS MSR Transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);
			//szStatus = 2;
		//	memcpy(Status,"\x30\x30",szStatus);
		//			value["BODY_HEADER"]="APROBADA";
		//			value["BODY_MSG"]="";
		//			value["NAME_IMG"]="EXTAPROVED";
			
		//			uiInvokeURL(value,"GenScreen_alert.html");

		//			sleep(SHOW_AD);
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			//			value["BODY_HEADER"]="DECLINADA";
			//			value["BODY_MSG"]="";
			//			value["NAME_IMG"]="EXTDECLINE";
						
			//			uiInvokeURL(value,"GenScreen_alert.html");

			//			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Communication Error");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			//			value["BODY_HEADER"]="DECLINADA";
			//			value["BODY_MSG"]="";
			//			value["NAME_IMG"]="EXTDECLINE";
						
			//			uiInvokeURL(value,"GenScreen_alert.html");

			//			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//TODO ui
		}
		
		vdSetByteValuePP((char*)"STATUS",Status,szStatus);	
		build_C54();  
	}
	
	else if(chTechnology == CTS_KBD)
	{
		LOGF_TRACE("****Responde to KBD transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");			
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);			
			//szStatus = 2;
		//	memcpy(Status,"\x30\x30",szStatus);
		//			value["BODY_HEADER"]="APROBADA";
		//			value["BODY_MSG"]="";
		//			value["NAME_IMG"]="EXTAPROVED";
					
		//			uiInvokeURL(value,"GenScreen_alert.html");
		//			sleep(SHOW_AD);
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			//			value["BODY_HEADER"]="DECLINADA";
			//			value["BODY_MSG"]="";
			//			value["NAME_IMG"]="EXTDECLINE";
						
			//			uiInvokeURL(value,"GenScreen_alert.html");

			//			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			//			value["BODY_HEADER"]="DECLINADA";
			//			value["BODY_MSG"]="";
			//			value["NAME_IMG"]="EXTDECLINE";
						
			//			uiInvokeURL(value,"GenScreen_alert.html");

			//			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//TODO screen	
		}	
		
		vdSetByteValuePP((char*)"STATUS",Status,szStatus);	
		build_C54();  
	}
 

	iResult = PP_CMD_Pack((char*)"C54",chOutCmdData,&inLong);

	//4 check Macrocomando Uniadquirencia

	if(isMacroCmdUni) //if MacroCmd Uni
		reloadMacroComandoUni (chOutCmdData);

	//::::::::::::::::::::::::::::::::::::
		
	if(iResult == XML_CMD_OK)
	{
		LOGF_TRACE("******inDoC54 BUID RESPONSE *******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	chTechnology = 0;	
	flagC51=0;
	clear_parse();
	LOGF_TRACE("\n DONE inDoC54 command \n");
	
	return iResult;
}

//********************************************************************************************************************************
/*
* @brief Processing for C54 command
* @param[in] chOutCmdData buffer

*******************************************************************************************************************************/
int endTransaction54(char *chOutCmdData)
{
	
	int iResult = 0;
	//Variables que se requieren para procesar la informaci�n del comadno C54
	char Host_Response[3] = {0};
	int szHost_Response = 0;
	char Status_Host[3] = {0};
	int szStatus_Host;
	char AuthCode[7] = {0};
	int szAuthCode = 0;	
	char resp_code[5] = {0};
	int szresp_code = 0;
	char tg91[40] = {0};
	int sztg91 = 0;
	char txn_date[7]= {0};
	int sztxn_date = 0;
	char txn_time[7]= {0};
	int sztxn_time = 0;
	char tknE2[20] = {0};
	int sztknE2 = 0;

	int typctls = 0;

	//Variables para cerrar capitulo 10
	int  in_ath = FAILED_TO_CONNECT;

	//Variable para el status del Mensaje C54
	char Status[3] = {0};
	int szStatus = 0;

	int inLong = 0;
	int inTry = 0;
	map<string,string> value;  //Daee 18/06/2018  /*Reques AOV Command c54 does not show transaction result*/
	
	LOGF_TRACE("\n--endTransaction54--\n");
	 
	szStatus_Host = inGetByteValuePP((char*)"STATUS",Status_Host);
	szHost_Response = inGetByteValuePP((char*)"HOSTRESPONSE",(char*)Host_Response);
	LOGF_TRACE(" HOSTRESPONSE received = %s ",Host_Response);
	szAuthCode = inGetByteValuePP((char*)"AUHTRES",(char*)AuthCode);
	LOGF_TRACE("AUHTRES received = %s",AuthCode);
	
	//szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
	//LOGF_TRACE("RESPCODE received = %s",resp_code);
	
	sztg91 = inGetByteValuePP((char*)"ISS_UTH",(char*)tg91);	
	
	sztxn_date = inGetByteValuePP((char*)"TRANSDATE",(char*)txn_date);		
	LOGF_TRACE("TRANSDATE received = %s",txn_date);
	sztxn_time = inGetByteValuePP((char*)"TRANSTIME",(char*)txn_time);		
	LOGF_TRACE("TRANSTIME received = %s",txn_time);
	sztknE2 = inGetByteValuePP((char*)"TOKE2",(char*)tknE2);
	
	LOGF_TRACE("+++++ Host  Response ++++");
	//   if(!memcmp(Host_Response,"00",szHost_Response))
	if(!memcmp(Status_Host,"\x00",szStatus_Host))
	{
		LOGF_TRACE(" HOST_AUTHORISED ");
		in_ath = HOST_AUTHORISED;		
	}
	//	else if(!memcmp(Host_Response,"01",szHost_Response))
	else if(!memcmp(Status_Host,"\x01",szStatus_Host))
	{
		in_ath = HOST_DECLINED;
		LOGF_TRACE(" HOST_DECLINED ");
		
	}
	//	else if(!memcmp(Host_Response,,szHost_Response))
	else if(!memcmp(Status_Host,"\x02",szStatus_Host))
	{
		in_ath = FAILED_TO_CONNECT;     
		LOGF_TRACE(" FAILED_TO_CONNECT ");
	}
	else	
	{
		in_ath = FAILED_TO_CONNECT;     
		LOGF_TRACE(" UNKNOWN HOST RESPONSE  ");
	}

	LOGF_TRACE("Antes de in_cpx_crd_end");
	
	in_cpx_crd_end (szHost_Response, !memcmp(Host_Response, "70", 2));

	typctls = Check_typ_CTLS();

	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		LOGF_TRACE("****Responde to EMV or CTLS CHIP transaction****");

		if(chTechnology == CTS_CHIP)		
			iResult = processHostAuthForEMV();		
		else 
			iResult = processHostAuthForCTLS();		

		iResult=_CHECK_INSERTED_CARD();
		LOGF_TRACE("_CHECK_INSERTED_CARD %X",iResult);
		if(iResult==TRUE)
		{
			LOGF_TRACE("****Pedimos que se retire la tarjeta****");
			Remove_Card();
			LOGF_TRACE("****tarjeta retirada****");
		}

	}
	
	else if( (chTechnology == CTS_MSR) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) ) )
	{
		LOGF_TRACE("****Resonse to MSR or TLS MSR Transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);
			//szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			value["BODY_HEADER"]="APROBADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTAPROVED";
			uiInvokeURL(value,"GenScreen_alert.html");
			sleep(SHOW_AD);
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
						
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Communication Error");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
						
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//TODO ui
		}
		
		vdSetByteValuePP((char*)"STATUS",Status,szStatus);	
		buildEndTransactionReply54();  
	}
	
	else if(chTechnology == CTS_KBD)
	{
		LOGF_TRACE("****Responde to KBD transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");			
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);			
			//szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			value["BODY_HEADER"]="APROBADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTAPROVED";
					
			uiInvokeURL(value,"GenScreen_alert.html");
			sleep(SHOW_AD);
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
						
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
						
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//TODO screen	
		}	
		
		vdSetByteValuePP((char*)"STATUS",Status,szStatus);	
		buildEndTransactionReply54();  
	}
 
	if(isSECommand()){
		iResult = PP_CMD_Pack((char*)"SE54",chOutCmdData,&inLong);
	}else{
		iResult = PP_CMD_Pack((char*)"C54",chOutCmdData,&inLong);
	}
	
	//::::::::::::::::::::::::::::::::::::
		
	if(iResult == XML_CMD_OK)
	{
		LOGF_TRACE("******endTransaction54 BUID RESPONSE *******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	chTechnology = 0;	
	flagC51=0;
	clear_parse();
	LOGF_TRACE("\n DONE endTransaction54 command \n");
	
	return iResult;
}

int C54_EMV()
{
	int iResult = 0;
	int statuscard = 0;
	int inHostApproved = 0;

	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];
	map<string,string> value;	
		
	//unsigned char resp_code[2] = {0};
	//int szresp_code = 0;
	
	LOGF_TRACE("\n--C54_EMV--");

	statuscard = _CHECK_INSERTED_CARD();
	LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);

	if(statuscard==TRUE)
	{
		LOGF_TRACE("\n--Analizamos la respuesta del HOST--");
		inHostApproved = Host_Response();
		LOGF_TRACE("\n--Second Generate AC--");	
		iResult = EMV_SecondGenerateAC(&xOnlineInputCT,&pxTransRes.xEMVTransRes);

		LOGF_TRACE("\nEMV_SecondGenerateAC = %X",iResult);	

		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x9C;
		getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

		if(buffTAG[2] == 0x20)
		{
			LOGF_TRACE(" REFUND TRANSACTION not supported yet ");
			/*szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
			if(!memcmp(resp_code,"\x30\x30",szresp_code))
				iResult = EMV_ADK_TC;
			*/
		}

		switch(iResult){		
			case EMV_ADK_TC:                  // approved by host
				LOGF_TRACE("\n----------Tx EMV_ADK_TC APROVED");
				//Construir respuesta para CC4			
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				build_C54();
				EMV_EndTransactionCT("APROBADA", 0, TRUE);			
//				value.clear();
//			    value["BODY_HEADER"]="APROBADA";				//Daee 18/06/2018  /*Reques AOV Command c54 does not show transaction result*/
//		        value["BODY_MSG"]="";
//				value["NAME_IMG"]="EXTAPROVED";
				
//				uiInvokeURL(value,"GenScreen_alert.html");

//				sleep(SHOW_AD);
				
			break;		
			case EMV_ADK_AAC:                 // Denied offline
				LOGF_TRACE("\n----------Tx EMV_ADK_AAC DECLINED OFFLINE ");
				//Construir respuesta para CC4;						
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				build_C54();
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);				
				value.clear();
				if (inHostApproved != 0)
				{
					value["BODY_HEADER"]="DECLINADA EMV";
					value["NAME_IMG"]="EXTDECLINEEMV";
				
					value["BODY_MSG"]="";
				
					uiInvokeURL(value,"GenScreen_alert.html");

					sleep(SHOW_AD);
				}
				
				break;
			case EMV_ADK_ABORT:
				LOGF_TRACE("\n----------Tx EMV_ADK_ABORT DECLINED  ");
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
				build_C54();
				value.clear();
				value["BODY_HEADER"]="DECLINADA EMV";
				value["BODY_MSG"]="";
				value["NAME_IMG"]="EXTDECLINEEMV";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				break;	
			case EMV_ADK_INTERNAL:
			case EMV_ADK_PARAM:
			case EMV_ADK_ONLINE_PIN_RETRY:
			case EMV_ADK_CVM:
			case EMV_ADK_CARDERR:
			case EMV_ADK_BADAPP:	
			default:
				LOGF_TRACE("\n----------Tx DECLINED  ");
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
				build_C54();
				value.clear();
				value["BODY_HEADER"]="DECLINADA EMV";
				value["NAME_IMG"]="EXTDECLINEEMV";
				value["BODY_MSG"]="";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				//EMV_CTLS_SmartPowerOff(ucReader);
				//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
			return iResult;
		}
		LOGF_TRACE("\n--------------------------Result about transaction------------------------");
		Info_TRX_CT_EMV();
	}
	else
	{
		LOGF_TRACE(" CARD NOT INSERTED %X",statuscard);	
		vdSetByteValuePP((char*)"STATUS",(char*)"23",2);   //Card REMOVED
		build_C54(); 
		EMV_EndTransactionCT("\nTransaction Error", 0, TRUE);
		value.clear();
		value["BODY_HEADER"]="TARJETA REMOVIDA";
		value["BODY_MSG"]="verifique por favor";
		//value["BODY_MSG"]="TARJETA REMOVIDA";
		
		uiInvokeURL(value,"GenScreen_card_removed.html");

		sleep(SHOW_AD);

		
	}
	
	LOGF_TRACE("\n--End C54_EMV--");
	return iResult;
}

int processHostAuthForEMV()
{
	int iResult = 0;
	int statuscard = 0;
	int inHostApproved = 0;

	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];
	map<string,string> value;	
		
	//unsigned char resp_code[2] = {0};
	//int szresp_code = 0;
	
	LOGF_TRACE("\n--processHostAuthForEMV--");

	statuscard = _CHECK_INSERTED_CARD();
	LOGF_TRACE("_CHECK_INSERTED_CARD %X",statuscard);

	if(statuscard==TRUE)
	{
		LOGF_TRACE("\n--Analizamos la respuesta del HOST--");
		inHostApproved = Host_Response();
		LOGF_TRACE("\n--Second Generate AC--");	
		iResult = EMV_SecondGenerateAC(&xOnlineInputCT,&pxTransRes.xEMVTransRes);

		LOGF_TRACE("\nEMV_SecondGenerateAC = %X",iResult);	

		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x9C;
		getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

		if(buffTAG[2] == 0x20)
		{
			LOGF_TRACE(" REFUND TRANSACTION not supported yet ");
			/*szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
			if(!memcmp(resp_code,"\x30\x30",szresp_code))
				iResult = EMV_ADK_TC;
			*/
		}

		switch(iResult){		
			case EMV_ADK_TC:                  // approved by host
				LOGF_TRACE("\n----------Tx EMV_ADK_TC APROVED");
				//Construir respuesta para CC4			
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				buildEndTransactionReply54();
				EMV_EndTransactionCT("APROBADA", 0, TRUE);			
				value.clear();
			    value["BODY_HEADER"]="APROBADA";				//Daee 18/06/2018  /*Reques AOV Command c54 does not show transaction result*/
		        value["BODY_MSG"]="";
				value["NAME_IMG"]="EXTAPROVED";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				
			break;		
			case EMV_ADK_AAC:                 // Denied offline
				LOGF_TRACE("\n----------Tx EMV_ADK_AAC DECLINED OFFLINE ");
				//Construir respuesta para CC4;						
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				buildEndTransactionReply54();
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);				
				value.clear();
				if (inHostApproved != 0)
				{
					value["BODY_HEADER"]="DECLINADA EMV";
					value["NAME_IMG"]="EXTDECLINEEMV";
				
					value["BODY_MSG"]="";
				
					uiInvokeURL(value,"GenScreen_alert.html");

					sleep(SHOW_AD);
				}
				
				break;
			case EMV_ADK_ABORT:
				LOGF_TRACE("\n----------Tx EMV_ADK_ABORT DECLINED  ");
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
				buildEndTransactionReply54();
				value.clear();
				value["BODY_HEADER"]="DECLINADA EMV";
				value["BODY_MSG"]="";
				value["NAME_IMG"]="EXTDECLINEEMV";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				break;	
			case EMV_ADK_INTERNAL:
			case EMV_ADK_PARAM:
			case EMV_ADK_ONLINE_PIN_RETRY:
			case EMV_ADK_CVM:
			case EMV_ADK_CARDERR:
			case EMV_ADK_BADAPP:	
			default:
				LOGF_TRACE("\n----------Tx DECLINED  ");
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
				buildEndTransactionReply54();
				value.clear();
				value["BODY_HEADER"]="DECLINADA EMV";
				value["NAME_IMG"]="EXTDECLINEEMV";
				value["BODY_MSG"]="";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				//EMV_CTLS_SmartPowerOff(ucReader);
				//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
			return iResult;
		}
		LOGF_TRACE("\n--------------------------Result about transaction------------------------");
		Info_TRX_CT_EMV();
	}
	else
	{
		LOGF_TRACE(" CARD NOT INSERTED %X",statuscard);	
		vdSetByteValuePP((char*)"STATUS",(char*)"23",2);   //Card REMOVED
		buildEndTransactionReply54(); 
		EMV_EndTransactionCT("\nTransaction Error", 0, TRUE);
		value.clear();
		value["BODY_HEADER"]="TARJETA REMOVIDA";
		value["BODY_MSG"]="verifique por favor";
		//value["BODY_MSG"]="TARJETA REMOVIDA";
		
		uiInvokeURL(value,"GenScreen_card_removed.html");

		sleep(SHOW_AD);

		
	}
	
	LOGF_TRACE("\n--End processHostAuthForEMV--");
	return iResult;
}

int C54_CTLS()
{
	int iResult = DSM_OK;
	int inHostApproved = 0;

	debug("\n--CC4_CTLS--");

	
	if(iResult == DSM_OK)
	{
		debug("\n--Analizamos la respuesta del HOST--");
		inHostApproved = Host_Response();
		debug("\n--CTLS_END_transaction--");
		iResult = CTLS_END_transaction(&xOnlineInputCTLS,&CTLSTransRes);	
		printf("\niResult = %i",iResult);
	}	
						
	switch(iResult)
	{		
		case EMV_ADK_TC:				  // approved offline
			debug("\n****TC****");
			debug("\n----------Tx approved online!");			
			debug("\n EMV CLTS");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			build_C54();
			EMV_EndTransactionCTLS("APROBADA", 0, TRUE);				
		break;
		case EMV_ADK_AAC:			  // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
						
			debug("\n----------Tx Declined!");
			debug("\n****AAC****");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			build_C54();
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);	
			if((CTLSTransRes.T_9F27_CryptInfo & EMV_ADK_CARD_REQUESTS_ADVICE) == EMV_ADK_CARD_REQUESTS_ADVICE)
			{
				// vStoreAdviceData(xTrxRec); // store the advice data if it need and it'll be sent before reconcilation
			}			
			
			break;
			
		case EMV_ADK_ABORT: 			  // Denied offline 				
								
			debug("\n****Error Desconocido****\n ");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);			
			build_C54();
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);	
		break;
		case EMV_ADK_ONLINE_PIN_RETRY:
		case EMV_ADK_CVM:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_BADAPP:
		default:			
			debug("\n***Otro error**** ");
			EMV_EndTransactionCTLS("\Error en Transacción", 0, TRUE);				
			 							
		break;
		}
	Info_TRX_CTLS_EMV();  
			
	
	return iResult;
}

int processHostAuthForCTLS()
{
	int iResult = DSM_OK;
	int inHostApproved = 0;
	map<string,string> value;

	debug("\n--processHostAuthForCTLS--");

	
	if(iResult == DSM_OK)
	{
		LOGF_TRACE("\n--Analizamos la respuesta del HOST--");
		inHostApproved = Host_Response();
		LOGF_TRACE("\n--CTLS_END_transaction--");
		iResult = CTLS_END_transaction(&xOnlineInputCTLS,&CTLSTransRes);	
		LOGF_TRACE("\niResult = %i",iResult);
	}	
						
	switch(iResult)
	{		
		case EMV_ADK_TC:				  // approved offline
			LOGF_TRACE("\n****TC****");
			LOGF_TRACE("\n----------Tx approved online!");			
			LOGF_TRACE("\n EMV CLTS");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			buildEndTransactionReply54();
			EMV_EndTransactionCTLS("APROBADA", 0, TRUE);	

			value.clear();
			value["BODY_HEADER"]="APROBADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTAPROVED";
			
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);

		break;
		case EMV_ADK_AAC:			  // perform fallback to magstripe. Add reaction to local fallback rules or other local chip
						
			LOGF_TRACE("\n----------Tx Declined!");
			LOGF_TRACE("\n****AAC****");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
			buildEndTransactionReply54();
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);	
			if((CTLSTransRes.T_9F27_CryptInfo & EMV_ADK_CARD_REQUESTS_ADVICE) == EMV_ADK_CARD_REQUESTS_ADVICE)
			{
				// vStoreAdviceData(xTrxRec); // store the advice data if it need and it'll be sent before reconcilation
			}			
			value.clear();
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
			
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
			break;
			
		case EMV_ADK_ABORT: 			  // Denied offline 				
								
			LOGF_TRACE("\n****Error Desconocido****\n ");
			vdSetByteValuePP((char*)"STATUS",(char*)"00",2);			
			buildEndTransactionReply54();
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);	
			value.clear();
			value["BODY_HEADER"]="DECLINADA";
			value["BODY_MSG"]="";
			value["NAME_IMG"]="EXTDECLINE";
			
			uiInvokeURL(value,"GenScreen_alert.html");

			sleep(SHOW_AD);
		break;
		case EMV_ADK_ONLINE_PIN_RETRY:
		case EMV_ADK_CVM:
		case EMV_ADK_INTERNAL:
		case EMV_ADK_PARAM:
		case EMV_ADK_CARDERR:
		case EMV_ADK_BADAPP:
		default:			
			LOGF_TRACE("\n***Otro error**** ");
			EMV_EndTransactionCTLS("\Error en Transacción", 0, TRUE);				
			 							
		break;
		}
	Info_TRX_CTLS_EMV();  
			
	
	return iResult;
}

int inWaitAck(char *chMsOrig,int inMsOrig, int inMaxTry)
{
	
	char chRespAck[1] ={0};
	char chRecvCA1[1] = {0};
	int inResult = ACK_CMD_OK;
	int inTry = 0;
	while(1)
	{
		memset(chRecvCA1,0,sizeof(chRecvCA1));
		inResult = inRcvAdk(chRecvCA1,1,1000);
		
		if(inResult > 0)
		{
			if(chRecvCA1[0] == 0x06)
			{
				inResult = ACK_CMD_OK;
				break;
			}
			else if(chRecvCA1[0] == 0x15)
			{
				inTry+=1;
				
				if( inTry >= inMaxTry)
				{
					chRespAck[0] = 0x04;
					inSendAdk(chRespAck,1);
					inResult = EOT_CMD;
					break;
				}
				else
				{
					inSendAdk(chMsOrig,inMsOrig);
					continue;
				}
			}
			else
				continue;
		}
	}
	return inResult;
}


//2 COMANDOS DE UNIADQUIRENCIA BANORTE


int inDoC30Uni(char *chOutCmdData)
{
	char chCodResp[5+1] ={0};
	int inLong = 0;
	int in_val = SUCCESS;
	int inTry = 3;
	int MSRead = 0; 
	unsigned char Dev_type = 0; 		
	int inFlagManual = 0;   // se cambia a 0  para cancelar el ingreso manual
	unsigned char Tech_selected = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	unsigned char dataBuffer[2056] = {0};
	unsigned short dataBufferLength = sizeof(dataBuffer);	
	map<string,string> value;
	
	//Valores que llegan del comado 
	char Timeout[3] = {0};
	int inTimeOut = 15;
	char chGetElements [50] = {0};
	int sztimeout = 0;	
	// jbf 
	char txnDate [3 + 1] = {0};
	char txnTime [3 + 1] = {0};
	int  txnDateLen=0;
	int  txnTimeLen=0;
		
	unsigned char txntype = 0;
	int sztxntype = 0;
	
	
	char chAckNack[1] ={0};

	//3 Timeout Control
	unsigned long secstart=0L;		//Daee 07/06/2018
	unsigned long inNewTime=0L;
	unsigned long secend=0L;
	
	LOGF_TRACE("******inDoC30UNI*******");
	
	//EMV CHIP Structures
	memset((void*)&xSelectResCT,0x00,sizeof(xSelectResCT)); // result of the contact selection process
	memset((void*)&AdditionalTxDataCT,0x00,sizeof(AdditionalTxDataCT));
	memset((void*)&TxDataCT,0x00,sizeof(TxDataCT)); //CT Data. Parameters for start transaction (application selection processing)
	memset((void*)&xEMVTransResCT,0x00,sizeof(xEMVTransResCT));
	memset((void*)&xOnlineInputCT,0x00,sizeof(xOnlineInputCT));
	memset((void*)&pxTransRes,0x00,sizeof(pxTransRes));
	
	//EMV CTLS Structures
	
	memset((void*)&TxStartInputCTLS,0x00,sizeof(TxStartInputCTLS)); // result of the contact selection process
	memset((void*)&xSelectResCTLS,0x00,sizeof(xSelectResCTLS)); // result of the contact selection process
	memset((void*)&CTLSTransRes,0x00,sizeof(CTLSTransRes)); // result of the contact selection process
	memset((void*)&xOnlineInputCTLS,0x00,sizeof(xOnlineInputCTLS)); // result of the contact selection process

	Dev_type |= CTS_CHIP|CTS_MSR;
	
	//in_val = check_key(); // tengo que poner el estatus de la llave AES
	//LOGF_TRACE("******HARD CODE*******");
	//in_val = 0;
	if(acqmul==MULTI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"64",2);	
		in_val = PP_CMD_Pack((char*)"C30",chOutCmdData,&inLong);
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDo30uni_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
		
		return in_val;
		
	}
	else if(boCtlsON)
	{
		LOGF_TRACE("CTLS ON NOT SUPPORTED C30s");
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"02",2);	
		in_val = PP_CMD_Pack((char*)"C30",chOutCmdData,&inLong);
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDo30uni_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
		
		return in_val;
	}

	//in_val = StatusLlave();
	in_val = in_Cx_fkld();
	//if(in_val < SUCCESS)
	if(in_val == FALSE_AES)
	{
		LOGF_TRACE("\n******NO KEY*******\n");
		
		show_error(FALLA_CARGA_LLAVE_UNI);
		in_val = AES_ERROR;
	}
	else
	{	
		memset(chGetElements,0,sizeof(chGetElements));
		sztimeout = inGetByteValuePP((char*)"TIMEOUT",chGetElements);
		vdHex2Asc(chGetElements,Timeout,sztimeout);
		inTimeOut = atoi(Timeout);
		LOGF_TRACE("\ninTimeOut = %i",inTimeOut);
		LOGF_TRACE("\ninTimeOut = %i",inTimeOut);
	
		sztxntype = inGetByteValuePP((char*)"TRANSTYP",(char*)&txntype);
	
		sztxntype = inGetByteValuePP((char*)"PMOVILCORRESP",(char*)&txntype);
		LOGF_TRACE("--PMOVILCORRESP [%x]--",txntype);

		// jbf
		txnDateLen = inGetByteValuePP((char*)"DATE",(char*)&txnDate);
		txnTimeLen = inGetByteValuePP((char*)"TIME",(char*)&txnTime);
		LOGF_TRACE("--DATE&TIME [%x&%x]--",txnDate,txnTime);
		
		if( (txntype & KBD_ACT) > 0)
		{
			LOGF_TRACE("--ACTIVAMOS INGRESO MANUAL--");
			chTechnology = CTS_KBD;
			value["HEADER_TITLE"]="";
			value["BODY_HEADER"]="";
			value["BODY_MSG"]="";
			
			uiInvokeURL(value,"banorte_movil.html");
			sleep(1);
			in_val = KBD_Transaction_banorte(chManual, strlen(chManual),KBD_C30);
			LOGF_TRACE("-- in_val = [%i]", in_val);
			if(in_val !=CTS_OK)
				show_error(in_val);
		}
		else
		{
			//Se activa el lector de banda magn�tica
			LOGF_TRACE("\nACTIVAMOS EL LECTOR DE BANDA MAGN�TICA");
			MSRead = Enable_swp();
			LOGF_TRACE("\nMSRead = %i",MSRead);
			
			do
			{			

				//Daee 07/06/2018
				//3 Timeout Control
				//JRS 15022018
                if(secstart == 0)
					secstart = time (NULL);
				
                LOGF_TRACE("****TIME OUT [%d]****",inTimeOut);                                                    

                LOGF_TRACE("****NEW TIME OUT [%d]****",inNewTime);                                                       

                if(inNewTime != 0 )
                {
					if(inNewTime > inTimeOut)
						inTimeOut = 1;
					else
	                {
	                    inTimeOut = inTimeOut - inNewTime;
	                    secstart = time (NULL);
	                    secend = 0;
	                }
	                inNewTime = 0;
                }
				//3 Timeout Control

				
				in_val = Detect_input(Dev_type, &Tech_selected, inFlagManual,inMin, inMax, chManual, inTimeOut,
										dataBuffer,&dataBufferLength,(char *)"",(char *)"INSERTE DESLICE",(char *)"TARJETA");
				
				LOGAPI_HEXDUMP_TRACE("DATOS DESPU�S EL DETECT INPUT",dataBuffer,dataBufferLength);
				
				chTechnology = Tech_selected;
				LOGF_TRACE("\nTech_selected = %i",Tech_selected);
				LOGF_TRACE("\nUsed_techology = %i",chTechnology);
				LOGF_TRACE("\nin_val = %i",in_val);
					
				LOGF_TRACE("\n - \nchTechnology = %02Xx - \nin_val = %i", chTechnology, in_val);
				
				if( in_val == CTS_NO_CHIP ||  in_val == CTS_ERROR) 
				{
					LOGF_TRACE("caso fallback");
					//count_fallback+=1;
					count_fallback_inc();

					LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

					value.clear();
					value["HEADER_TITLE"]="";
					value["BODY_HEADER"]="FALLA EN CHIP";
					value["BODY_MSG"]="";
					value["NAME_IMG"]="EXTCHIPERROR";
					
					uiInvokeURL(value,"GenScreen_alert.html");
					sleep(SHOW_ALERT);

					Remove_Card();
					
					//if(count_fallback>2)
					if(count_fallback_get() > 1)
					{
						 Dev_type = CTS_MSR;
					}

					in_val=FALLA_CHIP;
				}
				else if(in_val < CTS_OK)
				{
					show_error(in_val);
				}
				else if(in_val == CTS_OK)
				{
					switch(chTechnology)
					{
						case CTS_MSR:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from MAGNETIG STRIPE READER");
							in_val = MSR_Transaction_UNI(MSRead);
							break;
				
						case CTS_CHIP:
						case CTS_CHIP|CTS_MSR: //Detection improve for Ux if not remove card and detecting two technologies  //Daee 27/10/2017
							chTechnology &= ~CTS_MSR; //Clear Msr Flag
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from EMV CHIP");
							in_val = EMV_Transaction_C30_UNI();	

							if(in_val == EMV_ADK_FALLBACK)
							{
									LOGF_TRACE("caso fallback");
									//count_fallback+=1;
									count_fallback_inc();

									LOGF_TRACE("count_fallback = [%d]", count_fallback_get());

									value.clear();
									value["HEADER_TITLE"]="";
									value["BODY_HEADER"]="FALLA EN CHIP";
									value["BODY_MSG"]="";
									value["NAME_IMG"]="EXTCHIPERROR";
									
									uiInvokeURL(value,"GenScreen_alert.html");
									sleep(SHOW_ALERT);

									Remove_Card();
									
									//if(count_fallback>2)
									if(count_fallback_get() > 1)
									{
										 Dev_type = CTS_MSR;
									}

									in_val=FALLA_CHIP;
							}
							break;
				
						/*case CTS_CTLS:
							LOGF_TRACE("\n");
							LOGF_TRACE("Input from CONTACTLESS READER");
							in_val = CTLS_Transaction();
							break;	*/		
				
						default:
							break;
					}

					LOGF_TRACE("in_val = [%d]", in_val);

					switch (in_val)
					{
						case INSUFFICIENT_MSR_DATA:
						case INVALID_MSR_DATA:
						case DATA_NOT_FOUND:
							value.clear();
							value["HEADER_TITLE"]=" ";
							value["BODY_HEADER"]="ESPERE";
							value["BODY_MSG"]="POR FAVOR";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							
//							value.clear();
//							value["HEADER_TITLE"]="LECTURA BANDA";
//							value["BODY_HEADER"]="RETIRE TARJETA";
//							value["BODY_MSG"]="POR FAVOR";
//							
//							uiInvokeURL(value,"GenScreen_alert.html");
//							sleep(SHOW_ALERT);
//							
							in_val = DATA_NOT_FOUND;
							
							break;
						//case DATA_NOT_FOUND:
						//case INVALID_MSR_DATA:
							value.clear();
							value["HEADER_TITLE"]="ERROR";
							value["BODY_HEADER"]="FALLO LECTURA";
							value["BODY_MSG"]="REINTENTE";
							value["NAME_IMG"]="error_icon";
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case USE_CHIP:
							value.clear();
							value["HEADER_TITLE"]="";
							value["BODY_HEADER"]="UTILICE LECTOR";
							value["BODY_MSG"]="DE CHIP";
							value["NAME_IMG"]="EXTUSECHIPREAD";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_ABORT:
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN CANCELADO";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						case EMV_CT_PIN_INPUT_TIMEOUT:
							value.clear();
							value["HEADER_TITLE"]="OPERACION";
							value["BODY_HEADER"]="CANCELADA";
							value["BODY_MSG"]="PIN TIMEOUT";
							value["NAME_IMG"]="EXTOPTCANCEL";
							
							uiInvokeURL(value,"GenScreen_alert.html");
							sleep(SHOW_ALERT);
							break;
						default:
							show_error(in_val);
							break;
					}
					
				}
				
				//Daee 07/06/2018
				//3 Timeout Control
				//JRS NEW TIME OUT 15022018
                secend = time (NULL);
                inNewTime = (secend - secstart);
				//3 Timeout Control
				
			}while( (in_val==USE_CHIP) || (in_val==DATA_NOT_FOUND) || (in_val==INVALID_MSR_DATA)  || (in_val==INSUFFICIENT_MSR_DATA)
			                    || (in_val==FALLA_CHIP) );
			
	  	}
	
	}
	
	
	LOGF_TRACE("in val = [%i]",in_val);
	if(in_val == SUCCESS_TRX)
	{
		value["HEADER_TITLE"]="COMANDO";
		value["BODY_HEADER"]="PROCESANDO";
		value["BODY_MSG"]="ESPERE UN MOMENTO";
		
		uiInvokeURL(value,"Procesando.html");

		sleep(SHOW_ALERT);
	}
			
	// jbf 20171103
	if (in_val == -6) //TimeOut
	{
		LOGF_TRACE("\nCancelada por timeout");
		
		//in_val = TimeOutResp((char*)"C30", chOutCmdData, &inLong);
		in_val = ErrEventResp((char*)"C30", (char*)"06\x00\x00", 4, chOutCmdData, &inLong);
		LOGF_TRACE("\n****** Enviando TimeOut Response *******\n");
		
	}
	else if (in_val == EXIT_BY_CANCEL) // CANCEL KEY
	{
		LOGF_TRACE("\nCancelada CANCEL KEY");
		
		//in_val = TimeOutResp((char*)"C30", chOutCmdData, &inLong);
		in_val = ErrEventResp((char*)"C30", (char*)"08\x00\x00", 4, chOutCmdData, &inLong);
		LOGF_TRACE("\n****** Enviando CANCEL Response *******\n");
		
		value.clear();
		value["HEADER_TITLE"]="";
		value["BODY_HEADER"]="OPERACION";
		value["BODY_MSG"]="CANCELADA";
		value["NAME_IMG"]="EXTOPTCANCEL";
		
		uiInvokeURL(value,"GenScreen_alert.html");
		sleep(SHOW_ALERT);
	}
	else 
	{
		in_val = PP_CMD_Pack((char*)"C30",chOutCmdData,&inLong);	
		LOGF_TRACE("\n****** Going to PP_CMD_Pack() *******\n");
	}
	

	//-// jbf 20171222 Enviando ACK
	//-LOGF_TRACE("\n******Enviando ACK*******\n");
	//-chAckNack[0]=0x06;
	//inSendAdk(chAckNack,1);
	

	LOGF_TRACE("\n******Enviando*******\n");
	in_val = inSendAdk(chOutCmdData,inLong);
	if(in_val == inLong)
	{	
		LOGF_TRACE("\n******inDo30uni_Ack*******\n");
		//in_val = inWaitAck(chOutCmdData,inLong,inTry);
	}
		
	MSRead = Disable_swp();
	LOGF_TRACE("\nMSRead = %i",MSRead);
		
	LOGF_TRACE("\n");
	LOGF_TRACE("in_val = %i", in_val);
	LOGF_TRACE("--End inDoC30UNI--");
	clear_parse();
	clear_parse();
	count_fallback_zero();

	// jbf
	voUpdatePPClock(txnDate, txnTime);
	
	return in_val;


}


int inDoC31Uni(char *chOutCmdData)
{
	int in_val = SUCCESS;
	int inTry  = 3;
	int statuscard = 0;
	int inLong = 0;
	int MSRead = 0;	
	char chCodResp[5+1] ={0};
	char cmd_snd[3+1]={0};
	unsigned char Dev_type = 0; 
	
	int inFlagManual = 0;  // cancelo el ingreso manual
	unsigned char Tech_selected = 0;
	int inMin = 16; 
	int inMax = 19;
	char chManual[20+1]={0};
	unsigned char dataBuffer[2056] = {0};
	unsigned short dataBufferLength = sizeof(dataBuffer);	
	map<string,string> value;

	//Valores que llegan del comado C31	
	char Timeout[3] = {0};
	//int inTimeOut = 15;
	char chGetElements [50] = {0};
	int sztimeout = 0;	
	
	unsigned char txntype = 0;
	int sztxntype = 0;

	LOGF_TRACE("******inDoC31UNI*******");

//EMV CHIP Structures
/*
	memset((void*)&xSelectResCT,0x00,sizeof(xSelectResCT)); // result of the contact selection process
	memset((void*)&AdditionalTxDataCT,0x00,sizeof(AdditionalTxDataCT));
	memset((void*)&TxDataCT,0x00,sizeof(TxDataCT)); //CT Data. Parameters for start transaction (application selection processing)
	memset((void*)&xEMVTransResCT,0x00,sizeof(xEMVTransResCT));
	memset((void*)&xOnlineInputCT,0x00,sizeof(xOnlineInputCT));
	memset((void*)&pxTransRes,0x00,sizeof(pxTransRes));

//EMV CTLS Structures

	memset((void*)&TxStartInputCTLS,0x00,sizeof(TxStartInputCTLS)); // result of the contact selection process
	memset((void*)&xSelectResCTLS,0x00,sizeof(xSelectResCTLS)); // result of the contact selection process
	memset((void*)&CTLSTransRes,0x00,sizeof(CTLSTransRes)); // result of the contact selection process
	memset((void*)&xOnlineInputCTLS,0x00,sizeof(xOnlineInputCTLS)); // result of the contact selection process
*/
	//			case CTS_CHIP:
	LOGF_TRACE("Input from EMV CHIP");
	if(acqmul==MULTI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"64",2);	
		in_val = PP_CMD_Pack((char*)"C33",chOutCmdData,&inLong);
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDo33uni_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
			
		return in_val;
			
	}

	//in_val = StatusLlave();
	in_val = in_Cx_fkld();
	//if(in_val < SUCCESS)
	if(in_val == FALSE_AES)
	{
		LOGF_TRACE("\n******NO KEY*******\n");
		
		//flagC30 = 0;
		//in_val = FALLA_CARGA_LLAVE_UNI;
		memcpy(cmd_snd,"C31",3);
		show_error(FALLA_CARGA_LLAVE_UNI);
		in_val = AES_ERROR;
		
	}
	else
	{
		in_val = _CHECK_INSERTED_CARD();
	
		if(in_val == FALSE)
		{
			show_error(CARD_REMOVED);
		
			memcpy(cmd_snd,"C33",3);
		}

		else
		{
			in_val = EMV_Transaction_UNI();
			memcpy(cmd_snd,"C33",3);
			LOGF_TRACE("in val = [%i]",in_val);

			switch (in_val)
			{
				case EMV_CT_PIN_INPUT_ABORT:
					value.clear();
					value["HEADER_TITLE"]="OPERACION";
					value["BODY_HEADER"]="CANCELADA";
					value["BODY_MSG"]="PIN CANCELADO";
					value["NAME_IMG"]="EXTOPTCANCEL";
					
					uiInvokeURL(value,"GenScreen_alert.html");
					sleep(SHOW_ALERT);
					break;
				case EMV_CT_PIN_INPUT_TIMEOUT:
					value.clear();
					value["HEADER_TITLE"]="OPERACION";
					value["BODY_HEADER"]="CANCELADA";
					value["BODY_MSG"]="PIN TIMEOUT";
					value["NAME_IMG"]="EXTOPTCANCEL";
					
					uiInvokeURL(value,"GenScreen_alert.html");
					sleep(SHOW_ALERT);
					break;
				default:
					show_error(in_val);
					break;
			}

			if(in_val == SUCCESS_TRX)
			{
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				value["HEADER_TITLE"]="COMANDO";
				value["BODY_HEADER"]="PROCESANDO";
				value["BODY_MSG"]="ESPERE UN MOMENTO";
				
				uiInvokeURL(value,"Procesando.html");

				sleep(SHOW_ALERT);
			}		
			else if(in_val == EMV_ADK_ABORT)
			{
				vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
			}
			else if((in_val == EMV_CT_PIN_INPUT_ABORT) || (in_val == EMV_CT_PIN_INPUT_TIMEOUT))
			{
				vdSetByteValuePP((char*)"STATUS",(char*)"23",2);	

			}
			else
			{	
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);   //Otro Error, provisional					
			}

		}
	
	}
	
	in_val = PP_CMD_Pack(cmd_snd,chOutCmdData,&inLong);
	LOGF_TRACE("\n******Enviando*******\n");
	in_val = inSendAdk(chOutCmdData,inLong);
	if(in_val == inLong)
	{	
		//in_val = inWaitAck(chOutCmdData,inLong,inTry);
	}

	
	LOGF_TRACE("--End inDoC31 UNI--");
	clear_parse();
	return in_val;
	
}


int inDoC34Uni(char *chOutCmdData)
{

	int iResult = 0;
	int in_val = SUCCESS;
	//Variables que se requieren para procesar la informaci�n del comadno C34
	char HostResponse[2] = {0};
	int szHost_Response = 0;
	char AuthCode[7] = {0};
	int szAuthCode = 0; 
	char resp_code[5] = {0};
	int szresp_code = 0;
	//char tg91[20] = {0};
	//int sztg91 = 0;
	char txn_date[5]= {0};
	int sztxn_date = 0;
	char txn_time[5]= {0};
	int sztxn_time = 0;
	char tknE2[20] = {0};
	int sztknE2 = 0;
	int typctls = 0;
	//Variables para cerrar capitulo 10
	int  in_ath = FAILED_TO_CONNECT;
		
	//Variable para el status del Mensaje C54
	char Status[3] = {0};
	int szStatus = 0;
		
	int inLong = 0;
	int inTry = 0;

	char vTagIssAuth[32+1];
	int lTagIssAuth=0;
	
	int  sztmpbuf  = 0;
	char tmpbuf[64]={0};
//	map<string,string> value;    //Daee 18/06/2018  /*Reques AOV Command c34 does not show transaction result*/
			
	LOGF_TRACE("--inDoC34-UNI--");
	memset(vTagIssAuth, 0x00, sizeof(vTagIssAuth));
	if(acqmul==MULTI_ADQUIRIENTE)
	{
		clear_parse();
		vdSetByteValuePP((char*)"STATUS",(char*)"64",2);	
		in_val = PP_CMD_Pack((char*)"C34",chOutCmdData,&inLong);
		LOGF_TRACE("\n******Enviando*******\n");
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			LOGF_TRACE("\n******inDo34uni_Ack*******\n");
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
			
		return in_val;
			
	}
	//in_val = StatusLlave();
	in_val = in_Cx_fkld();
	//if(in_val < SUCCESS)
	if(in_val == FALSE_AES)
	{
		LOGF_TRACE("\n******NO KEY*******\n");
		show_error(FALLA_CARGA_LLAVE_UNI);
		
		iResult = PP_CMD_Pack((char*)"C34",chOutCmdData,&inLong);
				
		if(iResult == XML_CMD_OK)
		{
			LOGF_TRACE("******inDoC34-UNI PACK*******\n");
			iResult = inSendAdk(chOutCmdData,inLong);
			if(iResult == inLong)
			{	
				//iResult = inWaitAck(chOutCmdData,inLong,inTry);
			}
		}
		
//		chTechnology = 0;	
//		flagC30 = 0;
		LOGF_TRACE("\n--End inDoC34-UNI--\n");
		return iResult;
		
	}

	LOGF_TRACE("\n--PARSEAMOS EL TOKE E1--");
	sztmpbuf = inGetByteValuePP((char*)"TOKE1",tmpbuf);
	LOGF_TRACE("LONGITUD E1 [%i]", sztmpbuf);
	LOGAPI_HEXDUMP_TRACE("E1",tmpbuf,sztmpbuf);
	if(sztmpbuf > 1 )
	{
		char *p1=tmpbuf;
		char vTag[256];
		int lTag = 0;
		int iCntBuf=0;
		vdSetByteValuePP((char*)"AUHTRES",tmpbuf + 2 ,sztmpbuf - 6);
//		vdSetByteValuePP((char*)"HOSTRESPONSE",tmpbuf +  10 ,2);	
		while (iCntBuf < sztmpbuf)
		{
			/*
		<FIELD0 Flow="A"  defined="TLV" len="2" type="NUM" >STATUS</FIELD0>
		<FIELD1 Flow="T"  defined="FIX" len="2" type="NUM" >LENGH</FIELD1>	
		<FIELD2 Flow="H"  defined="TLV" len="6" type="HEX" >AUHTRES</FIELD2>		
		<FIELD3 Flow="H"  defined="TLV" len="2" type="HEX" >HOSTRESPONSE</FIELD3>
		<FIELD4 Flow="H"  defined="TLV" len="16" type="NUM" >ISS_UTH</FIELD4>
		<FIELD5 Flow="H"  defined="TLV" len="3" type="NUM" >TRANSDATE</FIELD5>
		<FIELD6 Flow="H"  defined="TLV" len="3" type="NUM" >TRANSTIME</FIELD6>
		<FIELD7 Flow="A"  defined="TLV" len="144" type="NUM" >TOKE2</FIELD7>
			*/
			memset(vTag, 0x00, sizeof(vTag));
			if (*p1 == 0x89)
			{
				p1++;
				lTag=*p1++;
				memcpy(vTag, p1, lTag);
				p1 += lTag;
				vdSetByteValuePP((char*)"AUHTRES",vTag ,lTag);
				lTag += 2; // 1 tag + 1 len
			}
			else if (*p1 == 0x8A)
			{
				p1++;
				lTag=*p1++;
				memcpy(vTag, p1, lTag);
				p1 += lTag;
				vdSetByteValuePP((char*)"HOSTRESPONSE",vTag ,lTag);
				lTag += 2; // 1 tag + 1 len
			}
			else if (*p1 == 0x91)
			{
				p1++;
				lTag=*p1++;
				memcpy(vTag, p1, lTag);
				p1 += lTag;
				
				// MOved below to check if transaction is host approved
				//vdSetByteValuePP((char*)"ISS_UTH",vTag ,lTag);
				lTagIssAuth = lTag;
				// 20180221 Last time Adjust
				//memcpy(vTagIssAuth, p1, lTag);
				memcpy(vTagIssAuth, vTag, lTag);
				// ------------------------------------
				
				lTag += 2; // 1 tag + 1 len
			}
			else if (*p1 == 0x9A)
			{
				p1++;
				lTag=*p1++;
				memcpy(vTag, p1, lTag);
				p1 += lTag;
				vdSetByteValuePP((char*)"TRANSDATE",vTag ,lTag);
				lTag += 2; // 1 tag + 1 len
			}
			else if ((*p1 == 0x9F) && (*(p1 + 1) == 0x21))
			{
				p1++; p1++;
				lTag=*p1++;
				memcpy(vTag, p1, lTag);
				p1 += lTag;
				vdSetByteValuePP((char*)"TRANSTIME",vTag ,lTag);
				lTag += 3; // 2 tag + 1 len
			}
			else 
			{
				break;
			}
			iCntBuf += lTag;
		}
	}
		
	szHost_Response = inGetByteValuePP((char*)"STATUS",(char*)HostResponse);
	//szAuthCode = inGetByteValuePP((char*)"AUHTRES",(char*)AuthCode);
	//szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
	//sztg91 = inGetByteValuePP((char*)"ISS_UTH",(char*)tg91);	
	//sztxn_date = inGetByteValuePP((char*)"TRANSDATE",(char*)txn_date);		
	//sztxn_time = inGetByteValuePP((char*)"TRANSTIME",(char*)txn_time);		
	//sztknE2 = inGetByteValuePP((char*)"TOKE2",(char*)tknE2);
		
	LOGF_TRACE("\nEvalua la respuesta del host");
			
	if(!memcmp(HostResponse,"\x00",szHost_Response))
	{
		LOGF_TRACE("Autorizada por el Host");
		in_ath = HOST_AUTHORISED;		
		
		// Only for Authorized transactions 
		vdSetByteValuePP((char*)"ISS_UTH", vTagIssAuth , lTagIssAuth);
		
	}

	if(!memcmp(HostResponse,"\x01",szHost_Response))
	{
		in_ath = HOST_DECLINED;
		LOGF_TRACE("Declinada por el Host");
				
	}

	if(!memcmp(HostResponse,"\x02",szHost_Response))
	{
		in_ath = FAILED_TO_CONNECT; 	
		LOGF_TRACE("Fallo conexi�n con el Host");
				
	}		
			
	//LOGF_TRACE("Antes de in_cpx_crd_end");
	//in_cpx_crd_end (szresp_code, !memcmp(resp_code, "70", 2));
		
	typctls = Check_typ_CTLS();
		
	if( (chTechnology == CTS_CHIP) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_CHIP) ) )
	{
		LOGF_TRACE("****Responde to EMV or CTLS CHIP transaction****");
		
		if(chTechnology == CTS_CHIP)		
		{
			iResult = C34_EMV();		
		}	
		else 
		{
			iResult = C54_CTLS(); // cambiar por el C34_CTLS		
		}

		iResult=_CHECK_INSERTED_CARD();
		LOGF_TRACE("_CHECK_INSERTED_CARD %X",iResult);
		if(iResult==TRUE)
		{
			LOGF_TRACE("****Pedimos que se retire la tarjeta****");
			Remove_Card();
			LOGF_TRACE("****tarjeta retirada****");
		}
		
	}

	else if( (chTechnology == CTS_MSR) || ( (chTechnology == CTS_CTLS) && (typctls == CTLS_TXN_MSR) ) )
	{
			
		LOGF_TRACE("****Response to MSR or TLS MSR Transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");			
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);
			//szStatus = 2;
			//memcpy(Status,"\x30\x30",szStatus);
//			value["BODY_HEADER"]="APROBADA";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTAPROVED";
			
//			uiInvokeURL(value,"GenScreen_alert.html");

//			sleep(SHOW_AD);
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
//			value["BODY_HEADER"]="DECLINADA";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTDECLINE";
			
			
//			uiInvokeURL(value,"GenScreen_alert.html");

//			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Communication Error");
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
//			value["BODY_HEADER"]="DECLINADA";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTDECLINE";
			
//			uiInvokeURL(value,"GenScreen_alert.html");

//			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//Pantalla	
		}
				
		vdSetByteValuePP((char*)"STATUS",Status,szStatus); 
		build_C34();  
	}
			
	else if(chTechnology == CTS_KBD)
	{
		LOGF_TRACE("****Response to KBD transaction****");
		if(in_ath ==  HOST_AUTHORISED)
		{				
			LOGF_TRACE("Aproved Transaction");			
			//Para mensaje iso 39
			fld_39(resp_code,szresp_code, Status,&szStatus);			
			//szStatus = 2;
		//	memcpy(Status,"\x30\x30",szStatus);
//			value["BODY_HEADER"]="APROBADA";
//			value["BODY_MSG"]="123456";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTAPROVED";
			
//			uiInvokeURL(value,"GenScreen_alert.html");
//			sleep(SHOW_AD);
			
		}			
		else if(in_ath == HOST_DECLINED)
		{			
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
//			value["BODY_HEADER"]="DECLINADA";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTDECLINE";
			
//			uiInvokeURL(value,"GenScreen_alert.html");

//			sleep(SHOW_AD);
		}
		else if(in_ath == FAILED_TO_CONNECT)
		{	
			LOGF_TRACE("Declined Transaction");						
			szStatus = 2;
			memcpy(Status,"\x30\x30",szStatus);
			EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);			
//			value["BODY_HEADER"]="DECLINADA";
//			value["BODY_MSG"]="";
//			value["NAME_IMG"]="EXTDECLINE";
			
//			uiInvokeURL(value,"GenScreen_alert.html");

//			sleep(SHOW_AD);
		}
		else
		{			
			LOGF_TRACE("\nInvalid answer");			
			//Pantalla	
		}	
				
		vdSetByteValuePP((char*)"STATUS",Status,szStatus); 
		build_C34();  
	}
		 
		
	iResult = PP_CMD_Pack((char*)"C34",chOutCmdData,&inLong);
				
	if(iResult == XML_CMD_OK)
	{
		LOGF_TRACE("******inDoC34-UNI PACK*******\n");
		iResult = inSendAdk(chOutCmdData,inLong);
		if(iResult == inLong)
		{	
			//iResult = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}
		
	chTechnology = 0;	
	flagC30 = 0;
	LOGF_TRACE("\n--End inDoC34-UNI--\n");
	return iResult;
	

}


int inDoQ8Uni(char *chOutCmdData)
{
	string serial_number;
	char POSSN[16+1];
	unsigned char CRC32[4 +1];
	char VersionApp[64];  // NOMBRE DE LA VERSION DE APP
	char szfldval[64] = {0};
	int fp;
	char KeyAES_F[32];
	int mov=0;		//FAG 11-MAR-2016  agregamos la variable move para tener el mismo comportamiento que una MX925
	int in_val;
	int inLong = 0;
	int inTry=3;
	char LenHex[3]={0};
	char lenfld = 0;
	int status;
	int inRet=0;
	char szAppVer[35+1]={0};
	map<string,string> value;	
	
	LOGF_TRACE("---------------inDoQ8Uni--------------");
	value["L2_C"]="SOLICITANDO\n   SERIE";   // FAG 13-FEB-2017
	//value["L3_C"]="SERIE";
	
	uiInvokeURL(value,"GenScreen_info.html");

	sleep(SHOW_ALERT);			
	//Numero de Serie
	sysGetPropertyString(SYS_PROP_HW_SERIALNO,serial_number);
	memset(POSSN, 0x00, sizeof(POSSN)); 
	strcpy(POSSN,serial_number.c_str());
	purge_char(POSSN, '-'); 
	//Separador
	memset(szfldval, 0x00, sizeof(szfldval));
	memcpy(szfldval,"\xC1",1);
	szfldval[1]=strlen(POSSN);
	memcpy(szfldval +2 ,POSSN,strlen(POSSN));
	LOGAPI_HEXDUMP_TRACE("SERIAL_NUMBER:",szfldval,16);
	vdSetByteValuePP((char*)"SERIAL_NUMBER",szfldval,strlen(POSSN) + 2);
	mov += strlen(POSSN) + 2;
	//Version App, fecha y hora de compilacion
	
	memset(szfldval, 0x00, sizeof(szfldval));
	memset(VersionApp, 0x00, sizeof(VersionApp));

	inRet = getEnvFile((char *)"application", (char *)"VERSION", szAppVer,sizeof(szAppVer));
	
	if (inRet == 0){
		strcpy(VersionApp, "UNDEFINED");
	}
	else
	{
		strcpy(VersionApp, szAppVer);
		memset(szAppVer, 0x00, sizeof(szAppVer));
		inRet = getEnvFile((char *)"application", (char *)"TIMESTAMP", szAppVer,sizeof(szAppVer));
		
		if (inRet != 0){
			strcat(VersionApp, szAppVer);
		}
	}
	
	//memcpy(VersionApp,"VFMxP400v06_02_111516_16:34:00",30);
	
	memcpy(szfldval,"\xC1",1);
	szfldval[1] = strlen(VersionApp);
	memcpy(szfldval + 2,VersionApp,strlen(VersionApp));
	LOGAPI_HEXDUMP_TRACE("APP VERSION:",szfldval,35);
	vdSetByteValuePP((char*)"APPVERSION",szfldval,strlen(VersionApp) + 2);
    mov +=	strlen(VersionApp) + 2 ;
	//Status de inyeccion de llave
	//status = StatusLlave( );
	status = in_Cx_fkld();
	memset(szfldval, 0x00, sizeof(szfldval));
	memcpy(szfldval,"\xC1\x01",2);
	
	//if(status<0)
	if(status == FALSE_AES)
	{
		LOGF_TRACE("hubo falla de parte de statusllave");
		//in_val=FAILURE_LOAD_KEY;
		//show_error(FAILURE_LOAD_KEY); <<<----- <<<----- <<<----- <<<----- <<<----- <<<----- <<<----- this line was removed jbf 20180212
		//vdSetByteValuePP((char*)"STATUS",(char *)"82",2);
		memcpy(szfldval + 2,"\x01",1);
	

	vdSetByteValuePP((char*)"STATUS", (char *)"00", 2);
	
	LOGAPI_HEXDUMP_TRACE("STATUS_INYECTION:",szfldval,3);
	vdSetByteValuePP((char*)"STATUS_INYECTION",szfldval,3);
   	mov += 3;

	memset(szfldval, 0x00, sizeof(szfldval));
	memcpy(szfldval,"\xC1\x00",2);
	vdSetByteValuePP((char*)"CRC32",szfldval, 2);
	mov += 2;
	
	
	}

	else
	{
		LOGF_TRACE("hay llave inyectada");
		memcpy(szfldval + 2,"\x00",1);
		
		vdSetByteValuePP((char*)"STATUS", (char *)"00", 2);

		
	LOGAPI_HEXDUMP_TRACE("STATUS_INYECTION:",szfldval,3);
	vdSetByteValuePP((char*)"STATUS_INYECTION",szfldval,3);
   	 mov += 3;
	memset(KeyAES_F,0x00,sizeof(KeyAES_F));
    	fp = open( FILE_AES,  O_RDWR );
    	//Se extrae la llave
    	status =incryptoRead(fp,KeyAES_F,32);
    	close(fp);
   	 memset(CRC32,0x00, sizeof(CRC32));
    	//Se calcula CRC32 de la llave insertada
    	getCRC32Data(( unsigned char*)KeyAES_F,CRC32,32);
	memset(szfldval, 0x00, sizeof(szfldval));
	memcpy(szfldval,"\xC1\x04",2);
	memcpy(szfldval + 2, CRC32, sizeof(CRC32));
	vdSetByteValuePP((char*)"CRC32",szfldval, 4+2);
	mov += 4 + 2;
	
	}

	/*
	LOGAPI_HEXDUMP_TRACE("STATUS_INYECTION:",szfldval,4);
	vdSetByteValuePP((char*)"STATUS_INYECTION",szfldval,3);
   	 mov += 3;
	memset(KeyAES_F,0x00,sizeof(KeyAES_F));
    	fp = open( FILE_AES,  O_RDWR );
    	//Se extrae la llave
    	status =incryptoRead(fp,KeyAES_F,32);
    	close(fp);
   	 memset(CRC32,0x00, sizeof(CRC32));
    	//Se calcula CRC32 de la llave insertada
    	getCRC32Data(( unsigned char*)KeyAES_F,CRC32,32);
	memset(szfldval, 0x00, sizeof(szfldval));
	memcpy(szfldval,"\xC1\x04",2);
	memcpy(szfldval + 2, CRC32, sizeof(CRC32));
	vdSetByteValuePP((char*)"CRC32",szfldval, 4+2);
	mov += 4 + 2;
	*/
	
	LOGF_TRACE(" mov [%i]", mov);
	lenfld = 0;
	lenfld |= mov;	//length
	LenHex[1] = lenfld;
	lenfld = 0;
	lenfld |= mov >> 8;
	LenHex[0] = lenfld;
	LOGAPI_HEXDUMP_TRACE("LONGITUD DE TODO EL COMANDO EN HEX",LenHex,sizeof(LenHex));			
	vdSetByteValuePP((char*)"LEN",LenHex,2);
	in_val = PP_CMD_Pack((char*)"Q8",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}
	
	
}



int inDoZ10Uni(char *chOutCmdData)
{


//como en la 820 , 

	Rqkrnd	Rqk;
	char temp[10]={0};
	int  intemp;
	int  inret;
	char buff_temp[64] = {0};
	map<string,string> value;
	char LenHex[3]={0};
	char lenfld = 0;
	int  mov  = 0 ;
	int in_val = 0;
	int inLong = 0;
	int inTry = 3;
	
	LOGF_TRACE("************inDoZ10Uni**************");
	value["HEADER_TITLE"]= "COMANDO";
	value["BODY_HEADER"]="";
	value["L2_C"]= "SOLICITANDO";
	value["L3_C"]= "LLAVE UNI";
	
	uiInvokeURL(value,"GenScreen_info.html");
	sleep(SHOW_ALERT);
	inret = in_Cx_Krq(&Rqk);

	if(inret == VS_SUCCESS)
	{
		vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
		memcpy(buff_temp ,"\xC1",1);
		memcpy(buff_temp + 1,"\x10",1);
    		//Llave
    		 memcpy( buff_temp + 2,Rqk.chRndKey, Rqk.inRndKey);
		vdSetByteValuePP((char*)"RANDOM_KEY", buff_temp,Rqk.inRndKey + 2);
		mov += Rqk.inRndKey + 2;
		memset(buff_temp, 0x00, sizeof(buff_temp));
		memcpy(buff_temp ,"\xC1",1);
		memcpy(buff_temp + 1,"\x04",1);
		memcpy(buff_temp + 2, Rqk.chCrc32, Rqk.inCrc32);
		vdSetByteValuePP((char*)"CRC32", buff_temp,Rqk.inCrc32 + 2);
		mov += Rqk.inCrc32 + 2;
		memset(LenHex,0x00,sizeof(LenHex));
		lenfld = 0;
		lenfld |= mov;	//length
		LenHex[1] = lenfld;
		lenfld = 0;
		lenfld |= mov >> 8;
		LenHex[0] = lenfld;
		vdSetByteValuePP((char*)"LENGH", LenHex,2);

	}
	
	else
	{
		vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
	}

	in_val = PP_CMD_Pack((char*)"Z10",chOutCmdData,&inLong);
	
	if(in_val == XML_CMD_OK)
	{
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}


	return in_val;


}



int inDoZ11Uni(char *chOutCmdData)
{
	LdkDat DataCx;
	char status[5]={0};
	int  inret = VS_SUCCESS;
	int in_dsp;
	char chRsp[5+1] = {0};
	int in_val = 0;
	int inLong = 0;
	int inTry  = 3;
	map<string,string> value;	
	LOGF_TRACE("*********************inDoZ11Uni*****************************");
	memset(&DataCx,0,sizeof(DataCx));
	inret = inDsmZ11(&DataCx);
						
	if (inret == VS_SUCCESS)
	{
		inret = in_Cx_Kld(&DataCx);
	}

	//sprintf(status,"%02d",inDefRspCode_Cx(inret));

	if (in_Cx_fkld())
	{
		LOGF_TRACE("*******");
		//vdACPL_SetDefaultIdlePrompt ("");
		//inSaveConfRec (gszConfigFileName, SIZE_APP_CONF_REC, 0, (char *) &gstAppConfig);
		//iGCL_DisplayIdlePrompt ();
	}
	else
	{
		LOGF_TRACE("++++++++++++++++++++");
		in_ipk_wrn ();		// AJM 29/05/2014 1
	}

	if(inret!=VS_SUCCESS)
		show_error(inret);


	value["HEADER_TITLE"]= "COMANDO";
	value["BODY_HEADER"]="";
	value["L2_C"]= "INYECTANDO";
	value["L3_C"]= "LLAVE UNI";
	
	uiInvokeURL(value,"GenScreen_info.html");
	
	sleep(SHOW_ALERT);
	if(inret <  VS_SUCCESS )
	{
		LOGF_TRACE("HUBO FALLA, PROCESO FALLIDO");
			value["L2_C"] ="FALLIDO";
			value["L3_C"]= "";	

		// jbf 20180111	- Reporta fallo en el proceso de inyeccion de llaves
		vdSetByteValuePP((char*)"CODRESP",(char*)"83",2);
	}

	else
	{
		LOGF_TRACE("NO HUBO FALLAS, PROCESO EXITOSO");
		value["L2_C"] ="EXITOSO";	
		value["L3_C"]= "";
		vdSetByteValuePP((char*)"CODRESP",(char*)"00",2);
	}
	
	uiInvokeURL(value,"GenScreen_info.html");

	sleep(SHOW_ALERT);
	
	//vdSetByteValuePP((char*)"CODRESP",chRsp,2);
	in_val = PP_CMD_Pack((char*)"Z11",chOutCmdData,&inLong);

	LOGF_TRACE("\n inDoZ11Uni() \n RESULT_PACK - in_val=%d,  LONG_MSGZ11 - inLong=%d", in_val, inLong);
	
	if(in_val == XML_CMD_OK)
	{
		in_val = inSendAdk(chOutCmdData,inLong);
		if(in_val == inLong)
		{	
			//in_val = inWaitAck(chOutCmdData,inLong,inTry);
		}
	}

	if(inret >=VS_SUCCESS && acqmul==MULTI_ADQUIRIENTE)
	{
		char buffout[1024]={0};
		LOGF_TRACE("MANDAMOS LLAVES MULTI");
		flagY10 = 1;
		inDoY10BBVA(buffout);
	}
		
	return inret;
	
}


int in_c14_dsm (void)
{
	CrdExc_AES srCrdExc = {0};
	int in_val;
	int in_siz;
	int infld =0;
	char szfldval[64];
	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		if (! in_Cx_fkld ())
		{
			in_val = FAILURE;
		}
	}

	LOGF_TRACE("***in_val  -> %i", in_val);

	if (in_val > FAILURE)
	{

		memset(szfldval, 0x00, sizeof(szfldval));
		infld = inGetByteValuePP((char*)"VERSION_TAB",szfldval);
		LOGAPI_HEXDUMP_TRACE("VERSION_TAB : ",szfldval, infld + 3 );
		LOGF_TRACE("***infld = [%i]***",infld);	
		in_siz = infld;
				
		if ((in_siz < 1) || (in_siz > 10))
		{
			in_val = FAILURE;
			LOGF_TRACE("***FAILURE in_val =[] ***",in_val);
		}
		
	}
	
	if (in_val > FAILURE)
	{
		
		//memcpy (srCrdExc.chNam, pch_c14 + in_idx, in_siz);
		memcpy (srCrdExc.chNam, szfldval, infld);
		LOGAPI_HEXDUMP_TRACE( "ver",srCrdExc.chNam,20);
	}

	if (in_val > FAILURE)
	{
		memset(szfldval, 0x00, sizeof(szfldval));
		infld = inGetByteValuePP((char*)"BINES_EXCEPTION",szfldval);
		LOGAPI_HEXDUMP_TRACE("BINES_EXCEPTION",szfldval, infld + 3);
		LOGF_TRACE("***infld = [%i]***",infld); 	
		in_siz = infld;
		if ((in_siz < 16) || (in_siz > 256))
		{
			in_val = FAILURE;
		}

		if (in_siz % 16)
		{
			in_val = FAILURE;
		}

	}	
	
	if (in_val > FAILURE)
	{
		char ch_siz [5] = {0};
		int  in_num = 0;
		in_num = sprintf (ch_siz, "%d", in_siz);
		memcpy (srCrdExc.chUse, ch_siz, in_num);
		LOGAPI_HEXDUMP_TRACE("siz",srCrdExc.chUse, in_num);
//		memcpy (srCrdExc.chCrp, pch_c14 + in_idx, in_siz);
		memcpy (srCrdExc.chCrp, szfldval, in_siz);
		LOGAPI_HEXDUMP_TRACE("bin", srCrdExc.chCrp, in_siz);
	}

	if (in_val > FAILURE)
	{
		memset(szfldval, 0x00, sizeof(szfldval));
		infld = inGetByteValuePP((char*)"CRC32",szfldval);
		LOGAPI_HEXDUMP_TRACE("CRC32",szfldval, infld + 3);
		LOGF_TRACE("***infld = [%i]***",infld); 	
		in_siz = infld;


		if (in_siz != 4)
		{
			in_val = FAILURE;
		}
		
	}

	if (in_val > FAILURE)
	{
		memcpy (srCrdExc.chCrc, szfldval, in_siz);
		LOGAPI_HEXDUMP_TRACE("crc",srCrdExc.chCrc, in_siz);
	}

	if (in_val > FAILURE)
	{
		//in_val = inCrpSav ("CrdExc.dat", &srCrdExc, sizeof(srCrdExc));
		in_val = in_Crp_Sav((char  *)FILE_BINES, &srCrdExc, sizeof(srCrdExc));
		memset (&srCrdExc, 0, sizeof (srCrdExc));
	}

	LOGF_TRACE("in_val=%d", in_val);

	return in_val;

}


int C34_EMV()
{
	int iResult    = 0;
	int statuscard = 0;
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];

	unsigned char resp_code[2] = {0};
	int szresp_code = 0;
	int  inHostApproved = 0;

	map<string,string> value; 
	
	LOGF_TRACE("\n--C34_EMV--");

	statuscard = _CHECK_INSERTED_CARD();
	LOGF_TRACE("Tarjeta insertada? %X",statuscard);

	if(statuscard)
	{
		
		LOGF_TRACE("\n--Analizamos la respuesta del HOST--");
		inHostApproved = Host_Response();
		LOGF_TRACE("\n--Second Generate AC--");	
		iResult = EMV_SecondGenerateAC(&xOnlineInputCT,&pxTransRes.xEMVTransRes);

		LOGF_TRACE("\nResultado 2ndGenAC = %X",iResult);	

		szTAG = 0;
		TAG = 0;
		memset(buffTAG,0x00,sizeof(buffTAG));
		TAG = 0x9C;
		getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,chTechnology);

		if(buffTAG[2] == 0x20)
		{
			LOGF_TRACE("Devoluci�n");
			szresp_code = inGetByteValuePP((char*)"RESPCODE",(char*)resp_code);
			if(!memcmp(resp_code,"\x30\x30",szresp_code))
				iResult = EMV_ADK_TC;
		}

		switch(iResult){		
			case EMV_ADK_TC:                  // approved by host
				LOGF_TRACE("\n----------Tx aprobada por el host!");
				//Construir respuesta para C34			
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				build_C34();
				EMV_EndTransactionCT("APROBADA", 0, TRUE);
//				value.clear();
//				value["BODY_HEADER"]="APROBADA";			 //Daee 18/06/2018  /*Reques AOV Command c34 does not show transaction result*/
//				value["BODY_MSG"]="";
//				value["NAME_IMG"]="EXTAPROVED";
				
//				uiInvokeURL(value,"GenScreen_alert.html");

//				sleep(SHOW_AD);
			break;		
			case EMV_ADK_AAC:                 // Denied offline
				LOGF_TRACE("\n----------Tx declinada por el host!");
				//Construir respuesta para CC4;						
				vdSetByteValuePP((char*)"STATUS",(char*)"00",2);
				build_C34();
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				value.clear();
				if (inHostApproved != 0)
				{
					value["BODY_HEADER"]="DECLINADA EMV";
					value["NAME_IMG"]="EXTDECLINEEMV";
				
					value["BODY_MSG"]="";
					
					uiInvokeURL(value,"GenScreen_alert.html");

					sleep(SHOW_AD);
				}

				break;
			case EMV_ADK_ABORT:
				LOGF_TRACE("\n----EMV ADK ABORT!");
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"23",2);
				build_C34();				
				value["BODY_HEADER"]="DECLINADA EMV";
				value["NAME_IMG"]="EXTDECLINEEMV";
				value["BODY_MSG"]="";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
			break;	
			case EMV_ADK_INTERNAL:
			case EMV_ADK_PARAM:
			case EMV_ADK_ONLINE_PIN_RETRY:
			case EMV_ADK_CVM:
			case EMV_ADK_CARDERR:
			case EMV_ADK_BADAPP:	
			default:			
				LOGF_TRACE("\n-----DEFAULTt!");
				value["BODY_HEADER"]="DECLINADA EMV";
				value["NAME_IMG"]="EXTDECLINEEMV";
				value["BODY_MSG"]="";
				
				uiInvokeURL(value,"GenScreen_alert.html");

				sleep(SHOW_AD);
				EMV_EndTransactionCT("DECLINADA", 0, TRUE);
				vdSetByteValuePP((char*)"STATUS",(char*)"99",2);
				build_C34();
				//EMV_CTLS_SmartPowerOff(ucReader);
				//EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);
			return iResult;
			}
		LOGF_TRACE("\n--------------------------Result about transaction------------------------");
		Info_TRX_CT_EMV();
	}
	else
	{
		vdSetByteValuePP((char*)"STATUS",(char*)"23",2);   //Si la tarjeta fue removida
		build_C34();  
		EMV_EndTransactionCT("\nTransaction Error", 0, TRUE); // ya no  se despliega la pantalla de error
		value.clear();
		value["BODY_HEADER"]="TARJETA REMOVIDA";
		value["BODY_MSG"]="verifique por favor";
	//	value["BODY_MSG"]="TARJETA REMOVIDA";
	
		uiInvokeURL(value,"GenScreen_card_removed.html");

		sleep(SHOW_ALERT);
			
	}
		
	LOGF_TRACE("\n--End C34_EMV--");
	return iResult;

}

//int inDsmZ11(char pchCmd[], int inCmd, LdkDat *Data)
int inDsmZ11(LdkDat *Data)
{
	int ret = VS_SUCCESS;
	char status[5]={0};
	int szstatus=0;
	int idx = 0;
	int lentag;
	int lentotal;
	LOGF_TRACE("inDsmZ11");
	if(ret >= VS_SUCCESS)
	{
		szstatus = inGetByteValuePP((char*)"HOST_RESP_KEY",status);
		if (*status)
		{
			ret = VS_ERROR;
		}
		LOGAPI_HEXDUMP_TRACE ("status",status, szstatus);
				
	}


	if(ret >= VS_SUCCESS)
	{
			Data->inSrlN = inGetByteValuePP((char*)"SERIAL_NUMBER",Data->chSrlN);
			LOGF_TRACE(" LONGITUD DE NUMERO DE SERIE : [%i]", Data->inSrlN);
			// TODO: REMOVE MY DEBUG !!!  REVD
//			memset (Data->chSrlN , 0 ,sizeof(Data->chSrlN));
//			strcpy(Data->chSrlN , "540008165");
//			strcpy(Data->chSrlN , "540005784");
			LOGAPI_HEXDUMP_TRACE("srln",Data->chSrlN,Data->inSrlN);
	}
		

	if(ret >= VS_SUCCESS)
	{
		Data->inEncK = inGetByteValuePP((char*)"ENCRYPTED_KEY",Data->chEncK);
		if (Data->inEncK == 0x20)
		{
			LOGF_TRACE(" LONGITUD DE LA LLAVE : [%i]", Data->inEncK);
			LOGAPI_HEXDUMP_TRACE(" encrypted key ",Data->chEncK,Data->inEncK);
		}
		else
		{
			ret = CX_IPEKE_ERR;
		}
	
	}
		

	if(ret >= VS_SUCCESS)
	{
		Data->inCrc32 = inGetByteValuePP((char*)"CRC32",Data->chCrc32);
		LOGF_TRACE(" LONGITUD DEL CRC 32  : [%i]", Data->inCrc32);
		LOGAPI_HEXDUMP_TRACE("CRC 32 cmd",Data->chCrc32,Data->inCrc32);
	
	}
	

	if (ret < VS_SUCCESS)		// AJM 05/09/2014 1
	{
		LOGF_TRACE(" ERROR ");
		ret = CX_IPEKE_ERR;
	}
	
	LOGF_TRACE(" Ret =   [%i]", ret );
	return ret;
}


void status_key()
{	
	int in_val = FAILURE;
	flagkey = 0 ;

	if(acqmul==0)
	{
		//in_val = StatusLlave();
		in_val = in_Cx_fkld();
		 //if(in_val > FAILURE)
		 if(in_val == TRUE_AES)
		 {
			LOGF_TRACE(" LLAVE CARGADA DE AES : %I",in_val);
			flagkey = 1;
		 }		

	}
	 
	else if(acqmul==1)
	{
	 	in_val = check_key();
		if(in_val > FAILURE)
	 	{
			LOGF_TRACE(" LLAVE CARGADA DE CAP X : %I",in_val);
			flagkey = 1;
	 	}
	 
	}

}

void show_error(int in_val)
{
	LOGF_TRACE("--- show error ---");
	map<string,string> value;
	
	value.clear();
							
	switch(in_val)
	{
		
		case UI_ERR_TIMEOUT:	
		case EMV_IDL_TIME_OUT:
								LOGF_TRACE("\nCancelada por timeout");
								//value["BODY_MSG"]="TIEMPO EXCEDIDO";
								//value["NAME_IMG"]="EXTLOADKEY";
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="";
								value["BODY_MSG"]="TIEMPO EXCEDIDO";
								value["NAME_IMG"]="EXTTIMEOUT";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								
								sleep(SHOW_ALERT);
								vdSetByteValuePP((char*)"STATUS",(char *)"06",2);						
								break;
								
		case EMV_IDL_CANCEL:	LOGF_TRACE("\nCancelada por el usuario");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="OPERACION";
								value["BODY_MSG"]="CANCELADA";
								value["NAME_IMG"]="EXTOPTCANCEL";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								
								/*
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="";
								value["BODY_MSG"]="TIEMPO EXCEDIDO";
								value["NAME_IMG"]="EXTTIMEOUT";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								*/
								
								vdSetByteValuePP((char*)"STATUS",(char *)"08",2);
								sleep(SHOW_ALERT);								
								break;

		case FALLA_CHIP:		LOGF_TRACE("Error en lectura de chip");
								vdSetByteValuePP((char*)"STATUS",(char *)"10",2);
								break;

		// jbf 20180122
		case TARJ_BLOQUEADA:	// TARJETA/APLICACION BLOQUEADA
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="TARJETA BLOQUEADA";
								value["BODY_MSG"]="";
								value["NAME_IMG"]="error_icon";
								
								uiInvokeURL(value,"GenScreen_alert.html");
		
								LOGF_TRACE("Error en lectura de chip");
								vdSetByteValuePP((char*)"STATUS",(char *)"24",2);
								
								sleep(SHOW_ALERT);

								break;

		// jbf 20180124
		case EMV_ABORT:	// TARJETA/APLICACION BLOQUEADA
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="EMV ABORT";
								value["BODY_MSG"]="";
								value["NAME_IMG"]="error_icon";
								
								uiInvokeURL(value,"GenScreen_alert.html");
		
								LOGF_TRACE("EMV_ABORT");
								vdSetByteValuePP((char*)"STATUS",(char *)"24",2);
								
								sleep(SHOW_ALERT);

								break;
								
		case CARD_REMOVED: 	LOGF_TRACE("Error en lectura de chip");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="TARJETA REMOVIDA";
								value["BODY_MSG"]="";
								value["NAME_IMG"]="EXTCARDREMOVED";
								
								uiInvokeURL(value,"GenScreen_alert.html");

								vdSetByteValuePP((char*)"STATUS",(char *)"23",2);
								sleep(SHOW_ALERT);
								break;

		case  CX_SRLND_ERR: 	LOGF_TRACE(" FALLA NUMERO DE SERIE ");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="NUMERO DE SERIE FALLO";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								
								vdSetByteValuePP((char*)"CODRESP",(char *)"79",2);
								sleep(SHOW_ALERT);	 
								break;						
																					
		case  CX_DECKR_ERR:	LOGF_TRACE(" FALLA AL DESCENCRIPTAR ");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="INICIALIZACION DE LLAVES INCORRECTA";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								vdSetByteValuePP((char*)"CODRESP",(char *)"80",2);
								sleep(SHOW_ALERT);
								break;
								
		case  CX_CRC32_ERR:		LOGF_TRACE(" FALLA CRC ");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="FALLA CRC\nNO COINCIDE";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								
								vdSetByteValuePP((char*)"CODRESP",(char *)"81",2);
								sleep(SHOW_ALERT);
							  	break;
								
		case FAILURE_CVV:		LOGF_TRACE("ERROR CVV VACIO");
								vdSetByteValuePP((char*)"STATUS",(char *)"84",2);
								break;

				
		case CPX_ERC_IPK:
		case FAILURE_LOAD_KEY: // revisar con angel este caso de error 82
		case FALLA_CARGA_LLAVE_UNI:
								LOGF_TRACE("\n******NO KEY*******\n");
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="CARGAR LLAVE";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");
								vdSetByteValuePP((char*)"STATUS",(char*)"85",2);	
								vdSetByteValuePP((char*)"LENGH",(char*)"\x00\x00",2);
								sleep(SHOW_ALERT);
								break;

		case  CX_KSAVE_ERR:	LOGF_TRACE(" FALLA AL GUARDAR LA LLAVE ");
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="LLAVE NO GUARDADA";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");									
								vdSetByteValuePP((char*)"CODRESP",(char *)"79",2);
								sleep(SHOW_ALERT);
								break;				
								
		case FAILURE_LUHN:		LOGF_TRACE("\nFALLA ALGORITMO LUHN");
								
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="CUENTA ERRONEA";
								value["BODY_MSG"]="";
								value["NAME_IMG"]="EXTACCERR";
								
								uiInvokeURL(value,"GenScreen_alert.html");			
		
								sleep(SHOW_ALERT);
								vdSetByteValuePP((char*)"STATUS",(char *)"99",2);
								break;
   
			  
		case  VS_ERROR_AES:  	LOGF_TRACE(" NO HAY CLAVE HC o host no fue exitoso");
								value["HEADER_TITLE"]="";
								value["BODY_HEADER"]="LLAVE NO INYECTADA";
								value["BODY_MSG"]="FALLA EN INYECCION DE LLAVE";
								value["NAME_IMG"]="EXTLOADKEY";
								
								uiInvokeURL(value,"GenScreen_alert.html");									
								vdSetByteValuePP((char*)"CODRESP",(char *)"99",2);
								sleep(SHOW_ALERT);
								break;
							
								
		case CTS_OK:			LOGF_TRACE("Not ERROR");
								break;

		default:					LOGF_TRACE("Error not Found");
								vdSetByteValuePP((char*)"CODRESP",(char *)"99",2);
								break;
					
	}


}

// jbf
void voUpdatePPClock(char *Date, char *Time)
{
	/*
	setDateTime( )
	- As user processes are not allowed to set the Linux RTC, applications must call
		setDateTime(), passing the date and time in the same format used in the shell
		command date: MMDDhhmmYYYY.ss
	*/
	char buf[20] = {0};
	char aa=Date[0];
	char mm1=Date[1];
	char dd=Date[2];
	char hh=Time[0];
	char mm2=Time[1];
	char ss=Time[2];
	sprintf(buf, "%02X%02X%02X%02X20%02X.%02X", mm1, dd, hh, mm2, aa, ss);
	LOGF_TRACE("Parametro para establecer FECHA y HORA: [%s]", buf);
	if (setDateTime(buf) == 0) {
		setRTC();
	}
	return;
}



// jbf 20171207
void count_fallback_zero(void)
{
    LOGF_TRACE("count_fallback_zero()");
    count_fallback = 0;
}

void count_fallback_inc(void)
{
    LOGF_TRACE("count_fallback_inc() - count_fallback [%d]", count_fallback);
    count_fallback += 1;
    LOGF_TRACE("count_fallback_inc() - count_fallback new [%d]", count_fallback);
}

int count_fallback_get(void)
{
    LOGF_TRACE("count_fallback_get() - count_fallback [%d]", count_fallback);
    return count_fallback;
}

void reloadMacroComandoUni (char *Buff) //Daee 11/01/2017
{
	char *NameCmd;

	NameCmd = strstr(Buff,"C53");
	if(NameCmd != NULL)
	{
		strncpy(NameCmd , "C33" , 3);
	}
	else 
	{
		NameCmd = strstr(Buff,"C54");
		if(NameCmd != NULL)
		{
			strncpy(NameCmd , "C34" , 3);
		}
	}
}

bool isSECommand(void){
	char keyValue[3]={0};
	int error=0;
	error = getEnvFile((char *)"application", (char *)"CXXSEXX",keyValue ,sizeof(keyValue));

	if(error<1){
		LOGF_TRACE("Could not read CXXSEXX Parameter");
		return false;
	}
	else{
		LOGF_TRACE("CXXSXX [%s]",keyValue); 
		return atoi(keyValue)==1?true:false;
	}
	return false;
}


int GetTransactionTimeout(void)
{
	static int lTransactionTimeout = 0;
	char chGetElements [50] = {0};
	char Timeout[3] = {0};
	int sztimeout = 0;

	memset(chGetElements,0,sizeof(chGetElements));
	sztimeout = inGetByteValuePP((char*)"TIMEOUT",chGetElements);
	vdHex2Asc(chGetElements,Timeout,sztimeout);

	if(sztimeout <= 0 || Timeout[0] == 0x00)
	{
		// LOGF_TRACE("TRANSACTION TIMEOUT: %d", lTransactionTimeout);
		// return lTransactionTimeout;
		lTransactionTimeout = 30;
	}

	lTransactionTimeout = atoi(Timeout);
	if(lTransactionTimeout <= 30 )
	{
		lTransactionTimeout = 30;
	}
	LOGF_TRACE("TRANSACTION TIMEOUT: %d", lTransactionTimeout);

	return lTransactionTimeout;
}



int updateDate(void)
 {
	std::string sysDate="";
	char date_cmd[6]={0};
	int szdate_cmd=0;
	char aux[15]={0};
	int inRet=0;
	LOGF_TRACE("-- update date --");

	//DIABLITO
	//strcat(date_cmd,"110201");
	//szdate_cmd = 6;

	szdate_cmd= inGetByteValuePP((char*)"DATE",date_cmd +1); // YYMMDD

	LOGAPI_HEXDUMP_TRACE("DATE IN CMD",date_cmd,sizeof(date_cmd));
	LOGF_TRACE("-- szdate_cmd = [%i] --",szdate_cmd);	 
	memcpy(date_cmd,"\x20",1);
	szdate_cmd+=1;
	vdHex2Asc(date_cmd,aux,szdate_cmd);
	//LOGAPI_HEXDUMP_TRACE("AUX",aux,sizeof(aux));
	memset(date_cmd,0x00,sizeof(date_cmd));
	inRet= inGetByteValuePP((char*)"TIME",date_cmd); //HHMMSS
	//LOGAPI_HEXDUMP_TRACE("TIME IN CMD",date_cmd,sizeof(date_cmd));
	LOGF_TRACE("-- inRet = [%i] --",inRet);	  
	vdHex2Asc(date_cmd,aux+(szdate_cmd*2),inRet);
	LOGAPI_HEXDUMP_TRACE("AUX",aux,sizeof(aux));
	inRet=sysGetPropertyString(SYS_PROP_RTC,sysDate);//yyyymmddhhmmss
	LOGF_TRACE("-- inRet = [%i] --",inRet);   
	LOGF_TRACE("-- sysDate = [%s] --",sysDate.c_str());
	sysDate.clear();
       sysDate.assign(aux);
	LOGF_TRACE("-- sysDate = [%s] --",sysDate.c_str());
 	if(sysDate.length()==14)
  	{
		inRet=sysSetPropertyString(SYS_PROP_RTC,sysDate);
		sysRefreshStatusBar(1);
	}
	else
	{
		inRet=-1;
	}
	
	LOGF_TRACE("-- inRet = [%i] --",inRet);
	return inRet;
 }
