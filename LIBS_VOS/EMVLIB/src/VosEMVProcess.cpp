#include <stdio.h>
#include <string.h>
#include <string>
#ifdef _VRXEVO
#include <stdlib.h> 
#else
#include <malloc.h> 
#endif

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#ifndef _VRXEVO
#include "sys/time.h"
#endif
#include <vector>
#include <svc.h>
#include "sysinfo/sysbeep.h"
#include <sysinfo/sysinfo.h>
#include "tec/tec.h"
#include <html/gui.h>	
#ifndef _VRXEVO
#include <vault/Pinentry.h>
#endif
#include <msr/msr.h>
#include <log/liblog.h>
#include <sysinfo/leds.h>


#include "VosEmvprocess.h"

#define lengthof(x) sizeof(x)/sizeof(x[0])

#ifdef _VRXEVO
#define EMV_PIN 0x0A
	
#endif
static int uiSelectionTimeout = 0;

int fRemovedCard = 0;

MSR_TrackData2 Tracks;
MSR_DecodedData Datas;


//Declara estructuras necesarias para contacto
EMV_CT_SELECTRES_TYPE xSelectResCT; // result of the contact selection process
EMV_CT_TRANSAC_TYPE AdditionalTxDataCT;
EMV_CT_SELECT_TYPE TxDataCT; //CT Data. Parameters for start transaction (application selection processing)
EMV_CT_TRANSRES_TYPE xEMVTransResCT;
EMV_CT_HOST_TYPE xOnlineInputCT;
EMV_TRX_CT_LOG_REC  pxTransRes;

//Estructuras para CTLS
EMV_CTLS_START_TYPE TxStartInputCTLS; //CTLS Data
EMV_CTLS_STARTRES_TYPE xSelectResCTLS;
EMV_CTLS_TRANSRES_TYPE CTLSTransRes;
EMV_CTLS_HOST_TYPE xOnlineInputCTLS;

//Daee 07/12/2017
int cbk_Error_Id = 0;
DisplayGetText_CB displayGetText_CB = NULL;



//**************************************************************************************************************************************************************************************************
//Prototypes of functions

int inFormatAmountForDisplay(unsigned char *HexAmount, char *szFormattedAmount, unsigned int inFormattedAmountLength);
void vdFormatCurrencyCode(unsigned char *pucHexCCode, char * szFormattedCCode);
void vdGetTime(unsigned char *buf, size_t bufLen);
//void vdGetDate(unsigned char *buf, size_t bufLen);  // se elimina de aqui y se cambia al .h  // FAG 18-SEP-2017
int bcdfromulong(unsigned char *bcd, int size, unsigned long value);
//static void dataforPIN(EMV_CT_ENTER_PIN_STRUCT* dataPin);		//Function to fill the structure to sent to the function EMV_CT_Enter_PIN_extended() in ENTER PIN() function
unsigned char ENTER_PIN(unsigned char pintype, unsigned char bypass,
						unsigned char* pucUnpredNb, unsigned char* pucPINResultData,
                        unsigned short KeyLength, unsigned char* pucPINPublicKey,
                        unsigned long ulPINPublicKeyExp);  		//Function to ask a PIN
//unsigned char PIN_OFFLINE(EMV_CT_ENTER_PIN_STRUCT* dataPin,unsigned char* pucPINResult);		//TO send PIN to Kernel
void CallbackInit_OutTLV(unsigned char *pucReceive, unsigned short *psReceiveSize);		//CallbackInit_OutTLV and emvCallback
void emvCallback(unsigned char *pucSend, unsigned short sSendSize, unsigned char *pucReceive, unsigned short *psReceiveSize, void* externalData);  //CallbackInit_OutTLV and emvCallback
unsigned char EMV_SetTermData();			//Set Terminal data
unsigned char EMV_SetAppData(unsigned char terminalType);		//Set Application data
unsigned char EMV_SetCAPKeys();		//Set CAPKeys data

static int tec_callback_reached = 0;
int uiIDLE_reset_tec_status();
int uiIDLE_notify_tec_callback();
static void default_tec_selection_cb(void *prt);
//int vdEmvIdle(unsigned char chTechnology, int inFlagManual,int inMin, int inMax,char *chManual,int inTimeOut,int in_chip,char *chHeader,char *chMsg, char *chMsg2);
int vdEmvIdle(unsigned char chTechnology,unsigned char *dataBuffer,unsigned short *dataBufferLength,unsigned char *typ_card, int inFlagManual,int inMin, int inMax,char *chManual,int inTimeOut,int in_chip,char *chHeader,char *chMsg, char *chMsg2);

int UI_ShowSelection(int timeout, const char* title, const char* elements[], const int num_elements, int preselect);
int UI_ShowMessage(const char* title, const char* text);

//JRS 18/01/2017
void vdIncraseTSNCts();
void  vdAsc2Hex (const char *dsp, char *hex, int pairs);

//JRS WRAPPERS 
int _getEnvFile_(char *chSection,char *chVar, char *chValue, int inLen );
int _putEnvFile_(char *chSection,char *chVar, char *chValue);

//**************************************************************************************************************************************************************************************************
//Global Variables
#define PINOKMESSAGE "PIN Verification OK"
#define DISPLAYTIME 3

guiCallbackT guiCb = NULL;
char gszCurrecyCode[3+1];
unsigned char gucAmountConfirmed = FALSE;
unsigned char EMV_Region = 0;																										
enum GUIOptions {DISPLAYMESSAGE=1, DISPLAYMESSAGEWITHIMAGE, AMOUNTINUTOPTION, PININPUTOPTION, CONFIRMAMOUNTOPTION};
using namespace vfigui;
using namespace std;
using namespace vfisysinfo;
using namespace vfihtml;
typedef std::map<std::string, std::string> UIParams;


UIParams uiPinParams;

    //APP_TRACE("--> WHERE IS THE CARD ???????");

    //APP_TRACE("start PIN entry: Bypass active: YES");
    //APP_TRACE("start PIN entry: Bypass active: NO");
  /*APP_TRACE("Amount extracted: %02X%02X%02X%02X%02X%02X",
                     Buffer[3], Buffer[4],
                     Buffer[5], Buffer[6],
                     Buffer[7], Buffer[8]);*/
    // PIN Entry using GUI-ADK
    //APP_TRACE("=== result of PIN entry: %d ===", rsp);
      //APP_TRACE("PIN Bypass from customer");
    //APP_TRACE("=== result of PIN offline: %d ===", rsp);
    // with Online PIN of CTLS there is no need to check card insertion
    //APP_TRACE("=== result of PIN entry: %d ===", rsp);
    // Online PIN needs to be fetched by the local/domestic VSS script handling the online PIN block
    // transmission towards the acquirer/network provider
    // custom CVM method
    //APP_TRACE("=== result of custom CVM method: OK");
//**************************************************************************************************************************************************************************************************
//******************************************************************INIT EMV CTLS*******************************************************************************************************************
unsigned char Init_EMV_CTLS(unsigned char terminalType)
{
	//unsigned long ulInitOptions = EMV_CTLS_INIT_OPT_CALC_CHKSUM | EMV_CTLS_INIT_OPT_AUTO_RETAP | EMV_CTLS_INIT_OPT_L1_DUMP;
	//unsigned long ulInitOptions = EMV_CTLS_INIT_OPT_CALC_CHKSUM | EMV_CTLS_INIT_OPT_AUTO_RETAP | EMV_CTLS_INIT_OPT_L1_DUMP | EMV_CTLS_INIT_OPT_NEW_CFG_INTF;
	unsigned long ulInitOptions = EMV_CTLS_INIT_OPT_CALC_CHKSUM | EMV_CTLS_INIT_OPT_AUTO_RETAP | EMV_CTLS_INIT_OPT_NEW_CFG_INTF;
	void *pvExternalData = NULL;
	EMV_ADK_INFO erg;
	int numberOfAIDs = 10;
	unsigned long ulResult=0;
	unsigned char ucRes;
	unsigned char iResult = 0;
	unsigned short width_led=0;
	unsigned short height_led=0;
	unsigned short xRegion_led=0;
	unsigned short yRegion_led=0;
	unsigned short wRegion_led=0;
	unsigned short hRegion_led=0;
	char tmpl[16]={0};
	int h,w;  // FAG 01-SEP-2017
	
	LOGF_TRACE("\n--Init CTLS--");
	
	//ulInitOptions |= EMV_CTLS_INIT_OPT_TRACE | EMV_CTLS_INIT_OPT_TRACE_CLT; //Uncomment to enable framework / client trace
	//ulInitOptions |= EMV_CTLS_INIT_OPT_TRACE; //Uncomment to enable framework / client trace
	//ulInitOptions |= EMV_CTLS_INIT_OPT_LED_VFI_INT; Uncomment to handle LED through the VFI Reader. If not set, LED display depends on #EMV_CTLS_INIT_OPT_LED_CBK_EXT
	//ulInitOptions |= EMV_CTLS_INIT_OPT_LED_CBK_EXT;  se comena para usar los leds que pinta el GUI server
	
	iResult = EMV_CTLS_Init_Framework(numberOfAIDs, emvCallback, pvExternalData, ulInitOptions,&ulResult); //Framework must be initialized before using EMV ADK APIs
	LOGF_INFO("***** iResult -> %i ******",iResult);
	if (iResult == EMV_ADK_OK){

		//getversionHW(tmpl);
		uiGetPropertyInt(UI_DEVICE_WIDTH,&w );
		uiGetPropertyInt(UI_DEVICE_HEIGHT,&h );
		LOGF_TRACE("UI_DEVICE_WIDTH [%d]",w);
		LOGF_TRACE("UI_DEVICE_HEIGHT [%d]",h);
			
		if(h==480)
		{
			LOGF_TRACE("MODEL 480 px");
			if(w>=800)
			{
				LOGF_TRACE("MODEL >=800px");
				width_led=20;
				height_led=20;
				xRegion_led=w;
				yRegion_led=52;
				//wRegion_led=320; 
				wRegion_led=w; 
				hRegion_led=25;
			}
			else
			{
				width_led=20;
				height_led=20;
				xRegion_led=0;
				yRegion_led=86;
				//wRegion_led=320; 
				wRegion_led=w; 
				hRegion_led=25;

			}
				
		}
		//else if(!strcmp(tmpl,MODEL_P200))
		else if(h==320)
		{
			LOGF_TRACE("MODEL P200");
			width_led=13;
			height_led=13;
			xRegion_led=0;
			yRegion_led=40;
			wRegion_led=240;
			hRegion_led=20;
		}
		
		
		unsigned char col_on[3] = {39, 203, 88};
		unsigned char col_off[3] = {255, 255, 255};
		LOGF_TRACE("\n----------Init EMV CTLS Framework OK");
		  		  
		  
		//Only needed if not physical leds present
		//ucRes=EMV_CTLS_LED_ConfigDesign(20, 20, col_off, col_on, 0, 85,320, 25); 	
		ucRes=EMV_CTLS_LED_ConfigDesign(width_led, height_led, col_off, col_on, xRegion_led, yRegion_led,wRegion_led, hRegion_led);	
		LOGF_TRACE("\n----------Init EMV CTLS Leds 0x%02X", ucRes);
		ucRes=EMV_CTLS_LED_SetMode(CONTACTLESS_LED_MODE_EMV);						
		LOGF_TRACE("\n----------Init EMV CTLS Leds Mode 0x%02X", ucRes);
		//ucRes=EMV_CTLS_LED(CONTACTLESS_LED_ALL, CONTACTLESS_LED_OFF);				
		//LOGF_TRACE("\n----------Init EMV CTLS Leds OFF 0x%02X", ucRes);
		//ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FIRST, CONTACTLESS_LED_IDLE_BLINK);
		//EMV_CTLS_LED( 	CONTACTLESS_LED_THIRD    , CONTACTLESS_LED_ONLINE_BLINK   );						
		//LOGF_TRACE("\n----------Init EMV CTLS Leds Blink 0x%02X", ucRes);
   }
   else{
      LOGF_TRACE("\n----------Init EMV CTLS framework error 0x%02X", erg);
      EMV_CTLS_Exit_Framework();
   }
   
   return iResult;	
}

//**************************************************************************************************************************************************************************************************
//******************************************************************ApplyCTLSConfiguration**********************************************************************************************************

EMV_ADK_INFO  ApplyCTLSConfiguration()
{
	EMV_ADK_INFO erg;
	LOGF_TRACE("\n--ApplyCTLSConfiguration--\n");
	erg=EMV_CTLS_ApplyConfiguration(CTLS_APPLY_CFG_ALL | CTLS_APPLY_CFG_FORCE); /* VFI Reader needs injection of the Configuration */
	if (erg != EMV_ADK_OK){
		LOGF_TRACE("\n----------Apply CTLS Configuration Error %#.2x", erg);
		return 1;
	}
	LOGF_TRACE("\n----------Apply CTLS Configuration OK");
	return erg;
}

//**************************************************************************************************************************************************************************************************
//******************************************************************vdSetCTLSTransactionData**********************************************************************************************************
void vdSetCTLSTransactionData(EMV_CTLS_START_TYPE *xSelectInput, char *szAmount)
{
	LOGF_TRACE("\n--vdSetCTLSTransactionData--\n");
	xSelectInput->TransType =  EMV_ADK_TRAN_TYPE_GOODS_SERVICE;
	xSelectInput->Info_Included_Data[0] |= INPUT_CTLS_SEL_TTYPE;    //raise the flag

	bcdfromulong (xSelectInput->TXN_Data.Amount, sizeof(xSelectInput->TXN_Data.Amount),(unsigned long)atol(szAmount));
	xSelectInput->Info_Included_Data[0] |= INPUT_CTLS_SEL_AMOUNT;    //Availability bit
	LOGAPI_HEXDUMP_TRACE("Amount",xSelectInput->TXN_Data.Amount,sizeof(xSelectInput->TXN_Data.Amount));
	//dumpEMV(xSelectInput->TXN_Data.Amount,sizeof(xSelectInput->TXN_Data.Amount),"Amount");
	
	xSelectInput->TXN_Data.Online_Switch = FALSE;
	xSelectInput->Info_Included_Data[1] |= INPUT_CTLS_SEL_ONLINE_SWITCH;    //raise the flags

	memset(xSelectInput->TxnOptions, 0, sizeof(xSelectInput->TxnOptions));
	//xSelectInput->TxnOptions[2] |=CLTRXOP_STOP_ON_CHKSUM_DIFF;
	xSelectInput->TxnOptions[2] |= CLTRXOP_L1_ERROR_CALLBACK;  //TODO: This flag must be activated to detect text fallback EMV_ADK_TXT_2_CARDS_IN_FIELD. However is not working properly.
	xSelectInput->Info_Included_Data[1] |= INPUT_CTLS_SEL_TXN_OPTIONS;    //raise the flag

	vdGetDate((unsigned char*) xSelectInput->TXN_Data.Date, sizeof(xSelectInput->TXN_Data.Date));
	xSelectInput->Info_Included_Data[0] |= INPUT_CTLS_SEL_DATE;    //raise the flag

	vdGetTime((unsigned char*) xSelectInput->TXN_Data.Time, sizeof(xSelectInput->TXN_Data.Time));
	xSelectInput->Info_Included_Data[0] |= INPUT_CTLS_SEL_TIME;    //raise the flag

	xSelectInput->passthroughCardTypes = CLTRX_PASSTHROUGH_OFF;
	xSelectInput->Info_Included_Data[1] |= INPUT_CTLS_SEL_PASSTHROUGH;
}

//**************************************************************************************************************************************************************************************************
//******************************************************************ActivateTransactionCTLS**********************************************************************************************************
unsigned char ActivateTransactionCTLS(EMV_CTLS_START_TYPE* TxStartInputCTLS,EMV_CTLS_STARTRES_TYPE* xSelectResCTLS)
{
	unsigned char iResult = 0;
	LOGF_TRACE("\n--ActivateTransactionCTLS--\n");
	iResult = EMV_CTLS_SetupTransaction(TxStartInputCTLS,xSelectResCTLS); //Setup transaction
 	return iResult;
}

//**************************************************************************************************************************************************************************************************
//******************************************************************CTLS_FirstGenerateAC************************************************************************************************************
unsigned char CTLS_FirstGenerateAC(EMV_CTLS_TRANSRES_TYPE* CTLSTransRes)
{
	unsigned char iResult = 0;	
	LOGF_TRACE("\n--CTLS_FirstGenerateAC--");
	do{
		iResult = EMV_CTLS_ContinueOffline(CTLSTransRes);
			if (iResult == EMV_ADK_OK)
			{
				LOGF_TRACE("\n----------CTLS continue offline: (%#.2x)", iResult);
				EMV_CTLS_SmartPowerOff(0);
				LOGF_TRACE("\n----------CTLS smart power off");
				if (CTLSTransRes->T_DF61_Info_Received_Data[0] & TRX_CTLS_STATUSINFO)
				{
					LOGF_TRACE("\n----------CTLS card has been read  successfully");
				}
			}
	    usleep(100);
    }while ((iResult == EMV_ADK_NO_CARD) || (iResult == EMV_ADK_CONTINUE));
	return iResult;
}

//**************************************************************************************************************************************************************************************************
//******************************************************************CTLS_END_transaction************************************************************************************************************
unsigned char CTLS_END_transaction(EMV_CTLS_HOST_TYPE *xOnlineInput, EMV_CTLS_TRANSRES_TYPE *xTransRes)
{
	unsigned char iResult = 0;
	iResult = EMV_CTLS_ContinueOnline(xOnlineInput, xTransRes);
    return iResult;
}
//************************************************************************************************************************************************************
//*************************************************Info_TRX_CTLS_EMV********************************************************************************************

void Info_TRX_CTLS_EMV()
{
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100];
	int iResult;

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F1A;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("\nTerminal Country Code",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"\nTerminal Country Code");	
	else
		LOGF_TRACE("\nTerminal Country Code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F28;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Country Code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Country Code");	
	else
		LOGF_TRACE("\nIssuer Country Code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F1B;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Terminal Floor Limit",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Terminal Floor Limit");
	else
		LOGF_TRACE("\nTerminal Floor Limit is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F33;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CTLS);	
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Teminal Capabilities",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Teminal Capabilities");
	else
		LOGF_TRACE("\nTeminal Capabilities is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x84;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );	
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("DF Name",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"DF Name");
	else
		LOGF_TRACE("\nDF Name is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		if(szTAG > 2)
			LOGAPI_HEXDUMP_TRACE("AID",buffTAG,szTAG);
			//dumpEMV(buffTAG,szTAG,"AID");
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
				LOGAPI_HEXDUMP_TRACE("AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				//dumpEMV(pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen,"AID");
			else
				LOGF_TRACE("\nAID is not available");
		} 
			
	else
		LOGF_TRACE("\nAID is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F12;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Application Preferred Name",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Application Preferred Name");
	}
	else
		LOGF_TRACE("\nApplication Preferred Name is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x50;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Application Level",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Application Level");
		//LOGF_TRACE("%s\n",buffTAG+2);
	}
	else
		LOGF_TRACE("\nApplication Level is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x82;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Application Interchange Profile",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Application Interchange Profile");	
	else
		LOGF_TRACE("\nApplication Interchange Profile is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F2A;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Transaction currency code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Transaction currency code");
	else
		LOGF_TRACE("\nTransaction currency code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F02;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Amount Authorised Number",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Amount Authorised Number");
	else
		LOGF_TRACE("\nAmount Authorised Number is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F03;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Amount Other Number",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Amount Other Number");
	else
		LOGF_TRACE("\nAmount Other Number is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8F;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Certification Authory Public Key Index",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Certification Authory Public Key Index");
	else
		LOGF_TRACE("\nCertification Authory Public Key Index is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8E;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cardholder Verification Method List",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Cardholder Verification Method List");
	else
		LOGF_TRACE("\nCardholder Verification Method List is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0E;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Denial",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Denial");
	else
		LOGF_TRACE("\nIssuer Action Code - Denial is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0F;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Online",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Online");
	else
		LOGF_TRACE("\nIssuer Action Code - Online is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0D;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Default",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Default");
	else
		LOGF_TRACE("\nIssuer Action Code - Default is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Transaction Status Information",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Transaction Status Information");
	else
		LOGF_TRACE("\nTransaction Status Infortation is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Terminal Verification Result",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Terminal Verification Result");
	else
		LOGF_TRACE("\nTerminal Verification Result is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cryptogram information Data",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Cryptogram information Data");
	else
		LOGF_TRACE("\nCryptogram information Data is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Authorization Response Code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Authorization Response Code");
	else
		LOGF_TRACE("\nAuthorization Response Code is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F34;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cardholder Verification Method Result",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Cardholder Verification Method Result");	
	else
		LOGF_TRACE("\nCardholder Verification Method Result is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F07;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CTLS);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Application Usage Control",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Application Usage Control");	
	else
		LOGF_TRACE("\nApplication Usage Control is not available");
	
}

//**************************************************************************************************************************************************************************************************
//******************************************************************INIT EMV************************************************************************************************************************
unsigned char Init_EMV(unsigned char terminalType)
{
	unsigned char iResult;
	//unsigned long ulInitOptions = EMV_CT_INIT_OPT_L1_DUMP;
	unsigned long ulInitOptions = EMV_CT_INIT_OPT_L1_DUMP|EMV_CT_INIT_OPT_TRACE_CLT|EMV_CT_INIT_OPT_TRACE;
	//unsigned long ulInitOptions = EMV_CT_INIT_OPT_L1_DUMP;
	void *pvExternalData = NULL;
	

   LOGF_TRACE("\n--Init_EMV--");
   
   //*************Init the framework
   LOGF_TRACE("\nInit EMV Framework");
   iResult = EMV_CT_Init_Framework(64,(EMV_CT_CALLBACK_FnT)emvCallback, /*pvExternalData*/pvExternalData, (/*virtTerm | EMV_CT_INIT_OPT_L1_DUMP | EMV_CT_INIT_OPT_TRACE | EMV_CT_INIT_OPT_TRACE_CLT*/ ulInitOptions));
   if(iResult != EMV_ADK_OK)
   {
		LOGF_TRACE("\nInit EMV CT framework Failed  [%X]",iResult);
		return(EMV_INIT_ERR_INIT_KERNEL);
   }   
   LOGF_TRACE("\nSuccessfull Init EMV CT framework [%X]",iResult);

#ifndef XMLFILES

   //************Set Terminal Data
   LOGF_TRACE("\nSet Terminal Data");
   iResult = EMV_SetTermData();   
   if(iResult != EMV_ADK_OK)
   {
		return EMV_INIT_ERR_SET_TERM_DATA;	
		LOGF_TRACE("\nSet CT Terminal Data Failed  (%#.2x) ",iResult); 
   }
   LOGF_TRACE("\nTerminal Data was succesfull [%d]",iResult);

   
   //************Set Application Data
   LOGF_TRACE("\nSet Application Data");
   iResult = EMV_SetAppData(terminalType);
   if(iResult != EMV_ADK_OK)
   {
		return EMV_INIT_ERR_SET_APPLI_DATA;	
		LOGF_TRACE("\nSet CT Terminal Data Failed (%#.2x)",iResult); 
   }
   LOGF_TRACE("\nApplication Data was succesfully [%d]",iResult);
   
   
   //************Store CAP Keys
   LOGF_TRACE("\nSet CAP Keys");
   iResult = EMV_SetCAPKeys();
   if(iResult != EMV_ADK_OK)
   {
		return EMV_INIT_ERR_SET_CAP_KEY;	
		LOGF_TRACE("\nSet CT CAP Keys Failed (%#.2x)",iResult);
   }
   LOGF_TRACE("\nSetting of CAP Keys was succesfull [%d]",iResult);
#endif

return iResult;
}

//**************************************************************************************************************************************************************************************************
//******************************************************************EMV_SetTermData*****************************************************************************************************************

unsigned char EMV_SetTermData()
{
	unsigned char iResult;
	unsigned char buff[50];
	EMV_CT_TERMDATA_TYPE xEMVTermdata;
	
	LOGF_TRACE("\n--EMV_SetTermData--");
#ifdef XMLFILES
	LOGF_TRACE("\n-Config using XML file--");
	iResult = EMV_CT_GetTermData(&xEMVTermdata);
	LOGAPI_HEXDUMP_TRACE("\nTermCountryCode",xEMVTermdata.TermCountryCode,sizeof(xEMVTermdata.TermCountryCode));
	//dumpEMV(xEMVTermdata.TermCountryCode,sizeof(xEMVTermdata.TermCountryCode),"\nTermCountryCode");
	LOGAPI_HEXDUMP_TRACE("\nTermAddCap",xEMVTermdata.TermAddCap,sizeof(xEMVTermdata.TermAddCap));
	//dumpEMV(xEMVTermdata.TermAddCap,sizeof(xEMVTermdata.TermAddCap),"\nTermAddCap");
	LOGF_TRACE("\nResult of EMV_CT_GetTermData [%X]",iResult);
	if( iResult != EMV_ADK_OK )
		return iResult;
#else	
	LOGF_TRACE("\n-Config using structures--");	
	memcpy((char*)&xEMVTermdata, (char*)&ProsaTermData, sizeof(xEMVTermdata));  //This is to debug
#endif
	
	iResult = EMV_CT_SetTermData(&xEMVTermdata);
	
	return iResult;	
}

//**************************************************************************************************************************************************************************************************
//******************************************************************EMV_SetAppData******************************************************************************************************************

unsigned char EMV_SetAppData(unsigned char terminalType)
{
	unsigned char iResult;
	unsigned char buff[50];
	EMV_CT_APPLI_TYPE xAID;
    EMV_CT_APPLIDATA_TYPE xAppliData;
	int i = 0;

	memset(&xAID,0,sizeof(EMV_CT_APPLI_TYPE));
	memset(&xAppliData,0,sizeof(EMV_CT_APPLIDATA_TYPE));
	LOGF_TRACE("\n--EMV_SetAppData--");
	
	/* clear all applications */	
    
	iResult = EMV_CT_SetAppliData(EMV_ADK_CLEAR_ALL_RECORDS, (EMV_CT_APPLI_TYPE*)&xAID, (EMV_CT_APPLIDATA_TYPE*)&xAppliData);	
	LOGF_TRACE("\nSet CT Clear Appli Data: (%#.2x)",iResult);  //ACM 08/09/2015
	if(iResult != EMV_ADK_OK)
		return(iResult);

#if XMLFILES

#if 0
	LOGF_TRACE("\n-Config using XML file--");

	memset(xAID.AID,"A000000004",10);
	xAID.ailen = 10;

	EMV_CT_GetAppliData(EMV_ADK_READ_FIRST , &xAID,&xAppliData);	
	LOGAPI_HEXDUMP_TRACE("\nAID",xAID.AID,sizeof(xAID.AID));	
	//dumpEMV(xAID.AID,sizeof(xAID.AID),"\nAID");		
	LOGF_TRACE("\nResult of EMV_CT_GetAppliData [%X]\n",iResult);
	if( iResult != EMV_ADK_OK )
		return iResult;
	
	iResult = EMV_CT_SetAppliData(EMV_ADK_SET_ONE_RECORD, (EMV_CT_APPLI_TYPE*)&xAID, (EMV_CT_APPLIDATA_TYPE*)&xAppliData);
	LOGF_TRACE("\nSet CT Set Appli Data: (%#.2x)", iResult);
	if(iResult != EMV_ADK_OK)
		return iResult;	
#endif	
#else

	LOGF_TRACE("\n-Config using structures--");	
	
	while((ptxAID[i] != NULL) && (ptxAppliData[i] != NULL))
	{
		/* get the AIDs and application data */
		memcpy((void*)&xAID, (void*)ptxAID[i], sizeof(xAID));
		memcpy((void*)&xAppliData, (void*)ptxAppliData[i], sizeof(xAppliData));		
		
		xAppliData.App_TermTyp = terminalType;	
		/* initializing the applications */
		iResult =EMV_CT_SetAppliData(EMV_ADK_SET_ONE_RECORD, (EMV_CT_APPLI_TYPE*)&xAID, (EMV_CT_APPLIDATA_TYPE*)&xAppliData);
		LOGF_TRACE("\nSet CT Set Appli Data: (%#.2x)", iResult);
		if(iResult != EMV_ADK_OK)
		  break;
		i++;		
	}
#endif

	return(iResult);	
}

//**************************************************************************************************************************************************************************************************
//******************************************************************EMV_SetCAPKeys******************************************************************************************************************

unsigned char EMV_SetCAPKeys()
{	
	unsigned char iResult=0;
	int count_keys = 0;
	unsigned char* pucMax;	
	EMV_CT_CAPREAD_TYPE pxKeyData;
	EMV_CT_CAPKEY_TYPE KeyData;
	
	LOGF_TRACE("\n--EMV_SetCAPKeys--");

#ifdef XMLFILES
#if 0
*pucMax = 40;
	iResult = EMV_CT_ReadCAPKeys(&pxKeyData,pucMax);
	LOGF_TRACE("Result of EMV_CT_ReadCAPKeys = %i",iResult);
	memcpy(KeyData.RID,pxKeyData.RID,5);
	LOGAPI_HEXDUMP_TRACE("pxKeyData.RID",pxKeyData.RID,5);
	//dumpEMV(pxKeyData.RID,5,"pxKeyData.RID");
	LOGAPI_HEXDUMP_TRACE("KeyData.RID",KeyData.RID,5);
	//dumpEMV(KeyData.RID,5,"KeyData.RID");	
	LOGAPI_HEXDUMP_TRACE("pucMax",pucMax,1);
	//dumpEMV(pucMax,1,"pucMax");
	iResult =EMV_CT_StoreCAPKey(EMV_ADK_SET_ONE_RECORD, &xKeyData[count_keys]);  
#endif	
#else

	iResult = EMV_CT_StoreCAPKey(EMV_ADK_CLEAR_ALL_RECORDS, &KeyData);
	if(iResult != EMV_ADK_OK)
		return(iResult);
	
	// check if it's at the end of the data struct
	  while(count_keys < (sizeof(xKeyData)/sizeof(xKeyData[0])))
	  {
		/* initializing the applications */
		iResult =EMV_CT_StoreCAPKey(EMV_ADK_SET_ONE_RECORD, &xKeyData[count_keys]);   
		LOGF_TRACE("\nSet CT Set CAP Keys Data: (%#.2x)", iResult);
		if(iResult != EMV_ADK_OK)
			break;
		count_keys++;
	  }
#endif
	return(iResult);
}

//************************************************************************************************************************************************************
//*************************************************SELECT APPLICATION*****************************************************************************************

unsigned char EMV_AppSelection(EMV_CT_SELECT_TYPE* xterminaldata,EMV_CT_SELECTRES_TYPE* xSelectRes){
unsigned char eEMVInfo;
map<string,string> value;

	LOGF_TRACE("\nEMV APPLICATION SELECTION");
	do
	{


		//xterminaldata->SEL_Data.xFallback_MS->ucFallback |= FB_NEVER;
		//xterminaldata->TxnOptions[0] |= EMV_CT_ALLOW_BLOCKED;
		// call kernel: select application from terminal- and card-/list
	
		if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}

		uiSelectionTimeout = 0;
		eEMVInfo = EMV_CT_StartTransaction(xterminaldata, // IN: terminaldata
						xSelectRes); // OUT: select info


		vdIncraseTSNCts();


		if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
		
		if(eEMVInfo == EMV_ADK_BADAPP)
		{
			//JRS ADVT 11 ESTE ES EL UNICO CASO QUE HE ENCONTRADO CON ESTA CONDICION POR LO CUAL MUESTRO EL MENSAJE ESPERADO POR BBVA
	  		value["HEADER_TITLE"]="";
			value["BODY_HEADER"]="APLICACION BLOQUEDA";
			value["BODY_MSG"]="REINTENTE";
			value["NAME_IMG"]="error_emv_icon";
			
			uiInvokeURL(value,"EmvResults.html");
			
			sysBeepError(BEEPVOLUME);
			sleep(4);

		
		   xterminaldata->InitTXN_Buildlist = REUSE_LIST_REMOVE_AID;
		   xterminaldata->Info_Included_Data[0] |= INPUT_SEL_BUILDLIST | INPUT_SEL_EXCLUDE_AID;
		   xterminaldata->Info_Included_Data[2] |= INPUT_SEL_MOD_CANDLIST;
		}

		  LOGAPI_HEXDUMP_TRACE("T_DF62_ErrorData",xSelectRes->T_DF62_ErrorData,sizeof(xSelectRes->T_DF62_ErrorData));
		  LOGAPI_HEXDUMP_TRACE("T_DF61_Info_Received_Data",xSelectRes->T_DF61_Info_Received_Data,sizeof(xSelectRes->T_DF61_Info_Received_Data));
		  LOGF_TRACE("MSG INDEX RESULT [%02x]",xSelectRes->T_DF63_DisplayText);
		  LOGF_TRACE("countFallbackMS [%02x]",xSelectRes->countFallbackMS);
		  LOGF_TRACE("ENV CALLBACK RESULT [%02x]",eEMVInfo);
		  

	} while(eEMVInfo == EMV_ADK_BADAPP);

	return eEMVInfo;					
}
//************************************************************************************************************************************************************
//*************************************************READ RECORD************************************************************************************************

//unsigned char EMV_ReadRecords(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes){
unsigned char EMV_ReadRecords(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
	unsigned char iResult;
	  // start EMV-payment
  // do the transaction in several substeps
  	pxTransactionInput->TxnSteps[0] = RETURN_AFTER_READ_RECORD | RETURN_AFTER_DATA_AUTH | RETURN_FOR_CVM_FINISH | RETURN_FOR_CVM_PROCESS | RETURN_AFTER_RISK_MANGEMENT;
  	pxTransactionInput->TxnSteps[1] = 0;  	
	pxTransactionInput->TxnSteps[2] = MS_RETURN_CALLBACKS; // all callbacks disabled
    pxTransactionInput->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag

    //JRS FIX EMV KERNEL
   // pxTransactionInput->TxnOptions[0] |= EMV_CT_ALLOW_BLOCKED;
   // xEMVStartData->TxnOptions[0] |= EMV_CT_ALLOW_BLOCKED;
	
	LOGF_TRACE("\nEMV_ReadRecords");
	LOGAPI_HEXDUMP_TRACE("T_DF64_KernelDebugData",pxTransRes->xEMVTransRes.T_DF64_KernelDebugData,sizeof(pxTransRes->xEMVTransRes.T_DF64_KernelDebugData));		
	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);	
	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);		

	LOGF_TRACE("\nREAD RECORD PROCESS");		
	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	LOGF_TRACE("\niResult  = %X",iResult);
	//iResult = EMV_steps(pxTransactionInput,pxTransRes);

	if(iResult == EMV_ADK_FALLBACK)
	{
		LOGF_TRACE("\nFALLBACK!!");
		return pxTransRes->eEMVInfo;
	}

	if(iResult != EMV_ADK_APP_REQ_READREC)
	{ 
		LOGF_TRACE("\nREAD RECORD ERROR!!");
		pxTransRes->eEMVInfo = EMV_ADK_ABORT;
	}

	return pxTransRes->eEMVInfo;
}


//************************************************************************************************************************************************************
//************************************************DATA AUTHENTICATION*****************************************************************************************

//unsigned char EMV_DataAuthentication(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes){
unsigned char EMV_DataAuthentication(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
	unsigned char iResult;

	LOGF_TRACE("\n EMV_DataAuthentication");

	 //JRS FIX EMV KERNEL
    //pxTransactionInput->TxnOptions[0] |= EMV_CT_ALLOW_BLOCKED;
    //xEMVStartData->TxnOptions[0] |= EMV_CT_ALLOW_BLOCKED;

    pxTransactionInput->TxnSteps[0] |= RETURN_AFTER_GPO;
    pxTransactionInput->TxnSteps[0] |= RETURN_AFTER_READ_RECORD;
    pxTransactionInput->TxnSteps[0] |= RETURN_AFTER_DATA_AUTH;
    pxTransactionInput->TxnSteps[0] |= RETURN_FOR_CVM_PROCESS;
    pxTransactionInput->TxnSteps[0] |= RETURN_FOR_CVM_FINISH;
    pxTransactionInput->TxnSteps[0] |= RETURN_AFTER_RISK_MANGEMENT;
    


	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);
	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);	
	
	LOGF_TRACE("\nDATA AUTHENTICATION PROCESS");		
	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	LOGF_TRACE("\niResult = %X",iResult);
	//xEMVTransResCT = pxTransRes->xEMVTransRes;
	LOGAPI_HEXDUMP_TRACE("EMV DEBUG DATA",pxTransRes->xEMVTransRes.T_DF64_KernelDebugData,sizeof(pxTransRes->xEMVTransRes.T_DF64_KernelDebugData));
//		iResult = EMV_steps(pxTransactionInput,pxTransRes);
	
	if(iResult != EMV_ADK_APP_REQ_DATAAUTH)
	{ 
		LOGF_TRACE("\nDATA AUTHENTICATION ERROR!!");
		iResult = EMV_ADK_ABORT;
	}

	return iResult;
}


//************************************************************************************************************************************************************
//*************************************************CARDHOLDER VERIFICATION************************************************************************************

//unsigned char EMV_CardHolderVerification(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes){
unsigned char EMV_CardHolderVerification(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
	unsigned char iResult = 0;
	unsigned char pinResult = 0;
	unsigned char pucPINResult[32] = {0};
	unsigned char maskCVM = 0;
	map<string,string> valueHtml;
	

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}

	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	LOGF_TRACE("dentro del DO - EMV_STEPS   iResult -> %i",iResult);
	LOGF_TRACE("dentro del DO - EMV_STEPS   pxTransRes -> %X",pxTransRes->eEMVInfo);
	if(iResult==EMV_CT_PIN_INPUT_ABORT||iResult==EMV_CT_PIN_INPUT_TIMEOUT||iResult==EMV_CT_PIN_INPUT_OTHER_ERR||iResult==EMV_CT_PIN_UNCRIT_TIMEOUT)
	{
		LOGF_TRACE("PIN ABORTADO");
		LOGF_TRACE("EMV_STEPS   iResult -> %i",iResult);
		LOGF_TRACE("EMV_STEPS   pxTransRes -> %x",pxTransRes->eEMVInfo);
		return iResult;
	}
		
	do{			
		LOGF_TRACE("dentro del DO - EMV_STEPS   iResult -> %i",iResult);
		LOGF_TRACE("dentro del DO - EMV_STEPS   pxTransRes -> %x",pxTransRes->eEMVInfo);
		if( (iResult == EMV_ADK_APP_REQ_PLAIN_PIN) || (iResult == EMV_ADK_APP_REQ_OFL_PIN) )
		{

			if(_CHECK_INSERTED_CARD() == FALSE)
			{
				fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
				LOGF_TRACE("DURANTE PROCESO EMV");
			}

			pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);

			if(_CHECK_INSERTED_CARD() == FALSE)
			{
				fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
				LOGF_TRACE("DURANTE PROCESO EMV");
			}

			maskCVM = pxTransRes->xEMVTransRes.T_9F34_CVM_Res[2];
			maskCVM &= 0x03;
			LOGF_TRACE("\nMasCVM = %i",maskCVM);
			if(maskCVM == 1)
			{
				LOGF_TRACE("\nError pin");
				
				if(_CHECK_INSERTED_CARD() == FALSE)
				{
					fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
					LOGF_TRACE("DURANTE PROCESO EMV");
				}
				iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);

				if(_CHECK_INSERTED_CARD() == FALSE)
				{
					fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
					LOGF_TRACE("DURANTE PROCESO EMV");
				}

				LOGF_TRACE("EMV_STEPS   iResult -> %X",iResult);				
				//if(iResult == EMV_CT_PIN_INPUT_ABORT || iResult== EMV_CT_PIN_INPUT_TIMEOUT)
				if(iResult==EMV_CT_PIN_INPUT_ABORT||iResult==EMV_CT_PIN_INPUT_TIMEOUT||iResult==EMV_CT_PIN_INPUT_OTHER_ERR||iResult==EMV_CT_PIN_UNCRIT_TIMEOUT)
				{
					LOGF_TRACE("PIN ABORTADO");
					LOGF_TRACE("EMV_STEPS   iResult -> %i",iResult);
					LOGF_TRACE("EMV_STEPS   pxTransRes -> %x",pxTransRes->eEMVInfo);
					return iResult;
					//break;
				}
				else if(iResult == EMV_ADK_APP_REQ_CVM_END)//NUVC
					break; //CT Reentrance mode: Application requested return cardholder verification
			}
			
		}
	}while(maskCVM == 1);

	LOGF_TRACE("dentro del DO - EMV_STEPS   iResult -> %i",iResult);
	LOGF_TRACE("dentro del DO - EMV_STEPS   pxTransRes -> %x",pxTransRes->eEMVInfo);
	LOGF_TRACE("\nCARDHOLDER VERIFICATION");	
	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	LOGF_TRACE("EMV_STEPS   iResult -> %X",iResult);	
	
	//iResult = EMV_steps(pxTransactionInput,pxTransRes);		
	//return iResult;
	return pxTransRes->eEMVInfo;
	
}


//************************************************************************************************************************************************************
//*************************************************CARDHOLDER VERIFICATION END********************************************************************************

//unsigned char EMV_CardHoldVerificationProcess(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes){
unsigned char EMV_CardHoldVerificationProcess(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
	unsigned char iResult = 0;
	map<string,string> valueHtml;

	LOGF_TRACE("\n EMV_CardHoldVerificationProcess");

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);

	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);

	
	LOGF_TRACE("\nPERFORMING CARDHOLDER VERIFICATION PROCESS");
	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	LOGF_TRACE("\niResult = %X", iResult);

	//iResult = EMV_steps(pxTransactionInput,pxTransRes);

	if(iResult != EMV_ADK_APP_REQ_CVM_END)
	{ 
		LOGF_TRACE("\nCARDHOLDER VERIFICTION END ERROR!!");
		iResult = EMV_ADK_ABORT;
	}

	return iResult;
}


//************************************************************************************************************************************************************
//*************************************************RISK MANAGEMENT********************************************************************************************

//unsigned char EMV_RiskManagement(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes){
unsigned char EMV_RiskManagement(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
unsigned char iResult;	

	LOGF_TRACE("\nEMV_RiskManagement");

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}

	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);
	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);

	LOGF_TRACE("\n(eEMVInfo) = %X",pxTransRes->eEMVInfo);
	LOGF_TRACE("\niResult = %X", iResult);
	//iResult =  EMV_steps(pxTransactionInput,pxTransRes);

	if(iResult != EMV_ADK_APP_REQ_RISK_MAN)
	{ 
		LOGF_TRACE("\nRISK MANAGEMENT ERROR!!");
		iResult = EMV_ADK_ABORT;
	}

	return iResult;
}


//************************************************************************************************************************************************************
//*************************************************FIRST GENERATE AC******************************************************************************************

unsigned char EMV_FirstGenerateAC(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput)
{
unsigned char iResult = 0;

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	pxTransRes->eEMVInfo = EMV_CT_ContinueOffline(pxTransactionInput, &pxTransRes->xEMVTransRes);

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	LOGF_TRACE("\nFIRST GENERATE AC");		
	iResult = EMV_steps(xEMVStartData,xSelectRes,pxTransRes,pxTransactionInput);
	LOGF_TRACE("\niResult = %X",iResult);
	
	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	LOGAPI_HEXDUMP_TRACE("TVR FIRST GENAC",pxTransRes->xEMVTransRes.T_95_TVR,sizeof(pxTransRes->xEMVTransRes.T_95_TVR));
	LOGAPI_HEXDUMP_TRACE("EMV DEBUG DATA",pxTransRes->xEMVTransRes.T_DF64_KernelDebugData,sizeof(pxTransRes->xEMVTransRes.T_DF64_KernelDebugData));
	return iResult;
}

//************************************************************************************************************************************************************
//*************************************************SECOND GENERATE AC*****************************************************************************************

unsigned char EMV_SecondGenerateAC(EMV_CT_HOST_TYPE *pxOnlineInput, EMV_CT_TRANSRES_TYPE *pxTransRes){
	unsigned char eEMVInfo;
	LOGF_TRACE("\n---------------Second Generate---------------\n");

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	LOGAPI_HEXDUMP_TRACE("Script 71 enviado", pxOnlineInput->ScriptCritData, pxOnlineInput->LenScriptCrit);
	LOGAPI_HEXDUMP_TRACE("Script 72 enviado", pxOnlineInput->ScriptUnCritData, pxOnlineInput->LenScriptUnCrit);
	eEMVInfo = EMV_CT_ContinueOnline(pxOnlineInput, pxTransRes);	
	return eEMVInfo;
}

//************************************************************************************************************************************************************
//*************************************************EMV Steps**************************************************************************************************
unsigned char EMV_steps(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType){

unsigned char  pinResult = 0;
unsigned char  pucPINResult[32] = {0};
unsigned char iResult = 0;
EMV_CT_APPLIDATA_TYPE *xAppliData;

	LOGF_TRACE("\n(eEMVInfo) = %X",xTrxRec->eEMVInfo);
	switch(xTrxRec->eEMVInfo)
    {
      case EMV_ADK_APP_REQ_READREC:
       	LOGF_TRACE("\n--> REENTRANCE READ RECORDS");
        // you may change parameters here, e.g. xEMVTransType or even AID data (see EMV_CT_SetAppliData(TEMP_UPDATE, NULL, (EMV_CT_APPLIDATA_TYPE*)&xAppliData) above)
        // do not forget to set the flags accordingly
        
#if 0
        memset(xAppliData->Info_Included_Data, 0x00, sizeof(xAppliData->Info_Included_Data));
        //xAppliData->App_FlowCap[0] |= FORCE_RISK_MANAGEMENT; 
        //xAppliData->App_FlowCap[1] |= EMV_CT_CHECK_INCONS_TRACK2_NO_EXP|EMV_CT_CHECK_INCONS_TRACK2_PAN; 
        //xAppliData->App_FlowCap[2] |= EMV_CT_SDA_SELECTED_TVR_ON; 
        xAppliData->Info_Included_Data[2] |= EMV_CT_INPUT_APL_FLOW_CAPS; // set flag that this parameter is changed (if more flags are raised, all the other params will be rest as well, not necessary, even it will not do any harm here because we just read the current config a couple of lines above ...)

        EMV_CT_SetAppliData(EMV_ADK_TEMP_UPDATE, NULL, (EMV_CT_APPLIDATA_TYPE*)xAppliData);
#endif
		iResult = EMV_ADK_APP_REQ_READREC;
        break;

      case EMV_ADK_APP_REQ_DATAAUTH:       
		LOGF_TRACE("\n--> REENTRANCE DATA AUTH");
        // you may change parameters here, e.g. xEMVTransType or even AID data (see EMV_CT_SetAppliData(TEMP_UPDATE, NULL, (EMV_CT_APPLIDATA_TYPE*)&xAppliData) above)
        // do not forget to set the flags accordingly
        iResult = EMV_ADK_APP_REQ_DATAAUTH;
        break;

      case EMV_ADK_APP_REQ_CVM_END:
        LOGF_TRACE("\n--> REENTRANCE CVM");

        // you may change parameters here, e.g. xEMVTransType or even AID data (see EMV_CT_SetAppliData(TEMP_UPDATE, NULL, (EMV_CT_APPLIDATA_TYPE*)&xAppliData) above)
        // do not forget to set the flags accordingly
        iResult = EMV_ADK_APP_REQ_CVM_END;
		break;

      case EMV_ADK_APP_REQ_RISK_MAN:
        LOGF_TRACE("\n--> REENTRANCE RISK MANAGEMENT");

        // you may change parameters here, e.g. xEMVTransType or even AID data (see EMV_CT_SetAppliData(TEMP_UPDATE, NULL, (EMV_CT_APPLIDATA_TYPE*)&xAppliData) above)
        // do not forget to set the flags accordingly
        iResult = EMV_ADK_APP_REQ_RISK_MAN; 
        break;

      case EMV_ADK_APP_REQ_ONL_PIN:
		LOGF_TRACE("\n--> REENTRANCE DATA AUTH");

        // Do PIN entry here
        //pinResult = ucInputPIN(EMV_CT_PIN_INPUT_ONLINE, FALSE, 0, pucPINResult, 0, NULL, 0);
        //
        if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
		pinResult = ENTER_PIN(EMV_CT_PIN_INPUT_ONLINE, FALSE, 0, pucPINResult, 0, NULL, 0);		

		if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
        if(pinResult == EMV_CT_PIN_INPUT_BYPASS)
        {
          xEMVTransType->TxnSteps[2] |= MS_PIN_BYPASS;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
        }
        else if(pinResult == EMV_CT_PIN_INPUT_ABORT || pinResult == EMV_CT_PIN_INPUT_TIMEOUT || pinResult == EMV_CT_PIN_INPUT_OTHER_ERR)
        {
          LOGF_TRACE("---EMV_CT_PIN_INPUT_ABORT || pinResult == EMV_CT_PIN_INPUT_TIMEOUT || pinResult == EMV_CT_PIN_INPUT_OTHER_ERR---");
          xEMVTransType->TxnSteps[2] |= MS_ABORT_TXN;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
          // or simply do not re-enter and end here
        }

		iResult = EMV_ADK_APP_REQ_ONL_PIN;
        break;

		
      case EMV_ADK_APP_REQ_CUST_CVM:
        LOGF_TRACE("\n--> REENTRANCE CUSTOm CVM");

        // Do PIN entry here
        //pinResult = ucInputPIN(EMV_CT_CVM_CUSTOM, FALSE, 0, pucPINResult, 0, NULL, 0);
        if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
        pinResult = ENTER_PIN(EMV_CT_CVM_CUSTOM, FALSE, 0, pucPINResult, 0, NULL, 0);
        if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
        if(pinResult == EMV_CT_PIN_INPUT_OKAY)
        {
          xEMVTransType->TxnSteps[2] |= MS_CUST_CVM_OK;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
        }
        else if(pinResult == EMV_CT_PIN_INPUT_ABORT || pinResult == EMV_CT_PIN_INPUT_TIMEOUT || pinResult == EMV_CT_PIN_INPUT_OTHER_ERR)
        {
          xEMVTransType->TxnSteps[2] |= MS_ABORT_TXN;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
          // or simply do not re-enter and end here
        }

		iResult = EMV_ADK_APP_REQ_CUST_CVM;
        break;

      case EMV_ADK_APP_REQ_OFL_PIN:
        LOGF_TRACE("\n--> REENTRANCE OFFLINE PIN");

        // Do PIN entry here
        //pinResult = ucInputPIN(EMV_CT_PIN_INPUT_ENCIPHERED, FALSE, 0, pucPINResult, 0, NULL, 0);
      
		pinResult = ENTER_PIN(EMV_CT_PIN_INPUT_ENCIPHERED, FALSE, 0, pucPINResult, 0, NULL, 0);	

        if(pinResult == EMV_CT_PIN_INPUT_BYPASS)
        {
          xEMVTransType->TxnSteps[2] |= MS_PIN_BYPASS;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
        }
        else if(pinResult == EMV_CT_PIN_INPUT_ABORT || pinResult == EMV_CT_PIN_INPUT_TIMEOUT || pinResult == EMV_CT_PIN_INPUT_OTHER_ERR)
        {
          xEMVTransType->TxnSteps[2] |= MS_ABORT_TXN;
          xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_STEPS; //raise the flag
          // or simply do not re-enter and end here
          if(pinResult == EMV_CT_PIN_INPUT_ABORT)
		  {				
		    	LOGF_TRACE("Pin Cancelado por el usuario");
				iResult = EMV_CT_PIN_INPUT_ABORT;
		  } 
		  else if( (pinResult == EMV_CT_PIN_INPUT_TIMEOUT) || (pinResult ==EMV_CT_PIN_INPUT_OTHER_ERR) || (pinResult == EMV_CT_PIN_INPUT_COMM_ERR))
		  {
				LOGF_TRACE("Pin Cancelado por timeout u otro error");
				iResult = EMV_CT_PIN_INPUT_TIMEOUT; 
		  }
        }
		else
		{
			LOGF_TRACE("Pin Offline Exitoso");
			iResult = EMV_ADK_APP_REQ_OFL_PIN;
		}
        break;

      case EMV_ADK_APP_REQ_PLAIN_PIN:
        LOGF_TRACE("\n--> REENTRANCE PLAIN PIN");
        // Do PIN entry here	

		pinResult = ENTER_PIN(EMV_CT_PIN_INPUT_PLAIN, FALSE, 0, pucPINResult, 0, NULL, 0);
		
		LOGF_TRACE("\npinResult = %i",pinResult);

		if(_CHECK_INSERTED_CARD() == FALSE)
		{
			fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
			LOGF_TRACE("DURANTE PROCESO EMV");
		}
		
		if(pinResult == EMV_CT_PIN_INPUT_BYPASS)
		{
			  xEMVTransType->TxnSteps[2] |= MS_PIN_BYPASS;
			  xEMVTransType->Info_Included_Data[0] |=	INPUT_OFL_TXN_STEPS; //raise the flag
		}
		else if(pinResult == EMV_CT_PIN_INPUT_ABORT || pinResult == EMV_CT_PIN_INPUT_TIMEOUT || pinResult == EMV_CT_PIN_INPUT_OTHER_ERR || pinResult ==EMV_CT_PIN_INPUT_COMM_ERR )
		{
			  xEMVTransType->TxnSteps[2] |= MS_ABORT_TXN;
			  xEMVTransType->Info_Included_Data[0] |=	INPUT_OFL_TXN_STEPS; //raise the flag
			  LOGF_TRACE(" EMV_CT_PIN_INPUT_ABORT");
			  // or simply do not re-enter and end here
			  if(pinResult == EMV_CT_PIN_INPUT_ABORT)
			  {				
				LOGF_TRACE("\nPin Cancelado por el usuario");
				iResult = EMV_CT_PIN_INPUT_ABORT;
			  } 
			  else if ((pinResult ==EMV_CT_PIN_INPUT_OTHER_ERR))
			  {
			  		LOGF_TRACE("\nPin Cancelado por Otro tipo de error");
			  		iResult = EMV_CT_PIN_INPUT_OTHER_ERR;
			  }
			  else if( (pinResult == EMV_CT_PIN_INPUT_TIMEOUT) || (pinResult == EMV_CT_PIN_INPUT_COMM_ERR))
			  {
				LOGF_TRACE("\nPin Cancelado por timeout");
					iResult = EMV_CT_PIN_INPUT_TIMEOUT;				 
			  }
		}
		else
		{

			if(_CHECK_INSERTED_CARD() == FALSE)
			{
				fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
				LOGF_TRACE("DURANTE PROCESO EMV");
			}
			LOGF_TRACE("\nPin Exitoso");
			iResult = EMV_ADK_APP_REQ_PLAIN_PIN;
		}
	 break;
	 case EMV_ADK_ARQC: 
		iResult = EMV_ADK_ARQC;
		LOGF_TRACE("\n******************Transaction must be resolved on line [ARQC]: %X",iResult);
	 break;
	 case EMV_ADK_TC:
		iResult = EMV_ADK_TC;
		LOGF_TRACE("\n******************Transaction was aproved offline [TC]: %X",iResult);
	 break;
	 case EMV_ADK_AAC:
		iResult = EMV_ADK_AAC;
		LOGF_TRACE("\n******************Transaction was Ddeclined offline [AAC]: %X",iResult);
	 break;   
	 case EMV_ADK_FALLBACK:   
	 	LOGF_TRACE("*****EMV_ADK_FALLBACK*****");
	 	LOGF_TRACE("*****T_DF63_DisplayText [%02x]*****",xTrxRec->xEMVTransRes.T_DF63_DisplayText);
	 	LOGAPI_HEXDUMP_TRACE("T_DF64_KernelDebugData",xTrxRec->xEMVTransRes.T_DF64_KernelDebugData,sizeof(xTrxRec->xEMVTransRes.T_DF64_KernelDebugData));
	 	LOGAPI_HEXDUMP_TRACE("T_DF62_ErrorData",xTrxRec->xEMVTransRes.T_DF62_ErrorData,sizeof(xTrxRec->xEMVTransRes.T_DF62_ErrorData));
	 	if(xTrxRec->xEMVTransRes.T_DF64_KernelDebugData[0] == 0x69 && xTrxRec->xEMVTransRes.T_DF64_KernelDebugData[1] == 0x85)
	 		return EMV_ADK_ABORT;

	 	iResult = EMV_ADK_FALLBACK;
	 	
	 break;
      default:
	  	LOGF_TRACE("\nUNKNOWN STEP");
		iResult = xTrxRec->eEMVInfo;
		LOGF_TRACE("\niResult  = %i",iResult);
      break; // nothing to do
    }

    if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	LOGF_TRACE("iResult : %i ",iResult);
	return iResult;

}




unsigned char UIEnterPin(guiPinParam *pinParam)
{
	int inResult;
	UIParams values;
	map<string,string>::iterator i;
	char pchPINResult[9];

	guiCb = pinParam->callback;
	// set properties
	uiSetPropertyInt(UI_PROP_PIN_ALGORITHM, (int) pinParam->ucPinAlgo);
	uiSetPropertyInt(UI_PROP_PASSWORD_CHAR, (int) pinParam->ulEchoChar);
	uiSetPropertyInt(UI_PROP_PIN_AUTO_ENTER, (int) pinParam->ucAutoEnter);
	uiSetPropertyInt(UI_PROP_PIN_BYPASS_KEY, (int) pinParam->ulBypassKey);
	uiSetPropertyInt(UI_PROP_PIN_CLEAR_ALL, (int) pinParam->ucClearAll);
	uiSetPropertyInt(UI_PROP_TIMEOUT, (int) pinParam->lTimeoutMs);

	unsigned long TAGR = 0x84;
	unsigned char Buffer[32];
	unsigned short TAGL;

	EMV_CT_fetchTxnTags(0, &TAGR, 1, Buffer, sizeof(Buffer), &TAGL);
	if(TAGL > 0){
	  LOGF_TRACE("----------AID Value: (%#.2x %#.2x %#.2x %#.2x %#.2x)",Buffer[2],Buffer[3],Buffer[4],Buffer[5],Buffer[6]);
	}
	memset(pchPINResult, 0, sizeof(pchPINResult));
	 LOGF_TRACE("ANTES DE inGUIInputAction");
	inResult=inGUIInputAction(PININPUTOPTION, ENTER_PIN_MESSAGE, pinParam->amount, NULL, 0);
	LOGF_TRACE("----------inGUIInputAction: %d",inResult);
	
	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}

  return inResult;
}


//**************************************************************************************************************************************************************
//*************************************************_CHECK_INSERTED_CARD*****************************************************************************************

unsigned char _CHECK_INSERTED_CARD(void)
{

	unsigned char inRes = 0 ;

	inRes = EMV_CT_SmartDetect(0); 
	LOGF_TRACE("\n--_CHECK_INSERTED_CARD [%u] [%d]--",inRes,fRemovedCard);
	
	//JRS  EMV_CUST_READ_CARD_IN_PROGRESS 
	if((inRes != EMV_ADK_SMART_STATUS_OK && inRes != EMV_CUST_READ_CARD_IN_PROGRESS )|| fRemovedCard == 1)
	//if(inRes != EMV_ADK_SMART_STATUS_OK)
	{
		//fRemovedCard = 0;
		LOGF_TRACE("\n-->The Card is not inserted ");
		return(FALSE);
	}

  return(TRUE);
}


unsigned char PINinputCallback(void)
{
  if(EMV_CT_SmartDetect(0) != EMV_ADK_SMART_STATUS_OK){
	  LOGF_TRACE("----------CARD NOT PRESENT...");
     return(true);
  }
  return(false);
}

//************************************************************************************************************************************************************
//*************************************************ENTER_PIN**************************************************************************************************
unsigned char ENTER_PIN(unsigned char pintype, unsigned char bypass,
						unsigned char* pucUnpredNb, unsigned char* pucPINResultData,
                        unsigned short KeyLength, unsigned char* pucPINPublicKey,
                        unsigned long ulPINPublicKeyExp)
{
unsigned char ucResult;
int inResult = 0;
unsigned long BypassKey = 0;

unsigned char iResult = 0;
unsigned char r = 0;
unsigned char rsp = 0;
//EMV_CT_ENTER_PIN_STRUCT dataPin;
unsigned char  pucPINResult[32] = {0};
guiPinParam pinParam;
unsigned long TAGR;
unsigned char Buffer[32];
unsigned short TAGL = 0;

LOGF_TRACE("\n--ENTER_PIN--"); 

//memset(&dataPin,0x00,sizeof(dataPin));

if(bypass)
 {
   LOGF_TRACE("\nstart PIN entry: Bypass active: YES");
   BypassKey = 13; // Bypass activated with Enter Key 0 digits
 }
 else
 {
   LOGF_TRACE("\nstart PIN entry: Bypass active: NO");
   BypassKey = 0;
 }

  pinParam.ucPinAlgo = EMV_PIN;
  pinParam.ucAutoEnter = 0;
  pinParam.ucClearAll = 0;
  pinParam.ulEchoChar = '*';
  pinParam.ulBypassKey = BypassKey;
  pinParam.lTimeoutMs = GENERALTIMEOUT * 1000;
  pinParam.callback = PINinputCallback;
  pinParam.currency = gszCurrecyCode;

  TAGR = 0x9F02; //Tx Amount

 if((pintype == EMV_CT_PIN_INPUT_ENCIPHERED) || (pintype == EMV_CT_PIN_INPUT_PLAIN))
  {   
   
   //dataforPIN(&dataPin);
    // This works only for V/OS
    LOGF_TRACE("*antes de UIEnterPin**");
    ucResult = UIEnterPin(&pinParam);
    LOGF_TRACE("----------UIEnterPin returned: %d ===", ucResult);
   

	if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
   //rsp = PIN_OFFLINE(&dataPin,pucPINResult);

   //LOGF_TRACE("\n=== result of PIN offline: %d ===", rsp);

    //switch(rsp)
    if(ucResult == EMV_CT_PIN_INPUT_BYPASS)
    	LOGF_TRACE("----------PIN Bypass from customer");

    if(ucResult != EMV_CT_PIN_INPUT_OKAY)
    	return(ucResult);


    if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}
	
    ucResult = EMV_CT_Send_PIN_Offline(pucPINResultData);

    if(_CHECK_INSERTED_CARD() == FALSE)
	{
		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
		LOGF_TRACE("DURANTE PROCESO EMV");
	}

    LOGF_TRACE("----------result of PIN offline: %d ***", ucResult);
    LOGF_TRACE("----------pucPINResultData: %x %x ===", pucPINResultData[0],pucPINResultData[1]);
    switch(ucResult)
    {
      case EMV_ADK_SMART_STATUS_OK:
        if(pucPINResultData[0] == 0x63)
        {
			  if(pucPINResultData[1] == 0xC1)
			  {
			  		// metemos pantalla de intente de nuevo con la alerta amarrila
			  		
					//inResult=inGUIInputAction(CONFIRMAMOUNTOPTION, (char *)"Invalid PIN", (char *)"Last Try!!!", NULL, 0);
					 {
					map<string,string> valueP;
					valueP.clear();
					valueP["HEADER_TITLE"]="PIN ERROR";
					valueP["BODY_HEADER"]="PIN INVALIDO";
					valueP["BODY_MSG"]="Ultimo Intento";
					
					uiInvokeURL(valueP,"EmvResults.html");
					
					sleep(3);
			  		}
					LOGF_TRACE("\nUltimo intento");			  
			  }
			  else{
			  		// metemos pantalla de de intente de nuevo
					//inResult=inGUIInputAction(CONFIRMAMOUNTOPTION, (char *)"Invalid PIN", (char *)"Try Again", NULL, 0);
					{
					map<string,string> valueP;
					valueP.clear();
					valueP["HEADER_TITLE"]="PIN ERROR";
					valueP["BODY_HEADER"]="PIN INVALIDO";
					valueP["BODY_MSG"]="Intente de Nuevo";
					
					uiInvokeURL(0,valueP,"EmvResults.html");

					sleep(3);
			  		}
					//inResult=inGUIInputAction(OTHERPIN, (char *)"PIN INVALIDO", (char *)"Intente de Nuevo", NULL, 0);
					LOGF_TRACE("\nIntenta otra vez");
			  }

			  if(inResult == -1)
			  	{
			  		LOGF_TRACE("************EMV_CT_PIN_INPUT_ABORT***********");// revisar esta variable
					
				//	return(EMV_CT_PIN_INPUT_ABORT);
			  	}
				
        }
		
        if((pucPINResultData[0] == 0x90)&&((pucPINResultData[1] == 0x00))){
		LOGF_TRACE("Mensaje aprobacion");
        	//inGUIInputAction(DISPLAYMESSAGE,(char *) PINOKMESSAGE, (char *) NULL, (char *)NULL, 0);
        	//sleep(DISPLAYTIME);
        }
        return(EMV_CT_PIN_INPUT_OKAY); // need to check SW1 SW2

      default:
    	  return(EMV_CT_PIN_INPUT_COMM_ERR);
    }
  }
  else if(pintype == EMV_CT_PIN_INPUT_ONLINE) //|| (pintype == EMV_CTLS_PIN_INPUT_ONLINE))
  {
	  // with Online PIN of CTLS there is no need to check card insertion
	  //if(pintype == EMV_CTLS_PIN_INPUT_ONLINE)
      //	pinParam.callback = NULL;
	  ucResult = UIEnterPin(&pinParam);
	  LOGF_TRACE("----------result of PIN entry: %d ===", ucResult);
	  if(ucResult != EMV_CT_PIN_INPUT_OKAY)
		  return(ucResult);
    // Online PIN needs to be fetched by the local/domestic VSS script handling the online PIN block
    // transmission towards the acquirer/network provider
    return(EMV_CT_PIN_INPUT_OKAY);
  }
  else if(pintype == EMV_CT_CVM_CUSTOM)
  {
  // custom CVM method
    LOGF_TRACE("\n=== result of custom CVM method: OK");
    return(EMV_CT_PIN_INPUT_OKAY);
  }
  return(EMV_CT_PIN_INPUT_OTHER_ERR);
}


//************************************************************************************************************************************************************
//*************************************************getCT_EMV_TAG**********************************************************************************************


unsigned char getCT_EMV_TAG(unsigned long  options,unsigned long *  requestedTags,unsigned short  noOfRequestedTags,unsigned char *  tlvBuffer,unsigned short  bufferLength, unsigned short *	tlvDataLength,int tech, bool boOnlyVal)
{
	unsigned char iResult;
	unsigned char buffer[300];
	unsigned short len=0;

	if(tech == CTS_CHIP)
		iResult = EMV_CT_fetchTxnTags  (options,requestedTags,noOfRequestedTags,buffer,sizeof(buffer),&len);   
	else if (tech == CTS_CTLS)
		iResult = EMV_CTLS_fetchTxnTags  (options,requestedTags,noOfRequestedTags,buffer,sizeof(buffer),&len);

	 if(iResult == EMV_ADK_OK)	//Daee 22/11/2017
	 {
		 if(boOnlyVal && len >= 3)
		 {
		    	if(*requestedTags>0xFF) 
		    	{
		    		len-=3;
		    		memmove(buffer, &buffer[3],len);
		    	}
		    	else 
				{
		    		len-=2;
		    		memmove(buffer, &buffer[2],len);
		    	}
		 }
		 if(len > 0)
		 	memcpy(tlvBuffer , buffer , len);
		 *tlvDataLength = len;
	 }

	return iResult;
}

//************************************************************************************************************************************************************
//*************************************************Info_TRX_CT_EMV********************************************************************************************

void Info_TRX_CT_EMV()
{
	unsigned long TAG = 0;
	unsigned short szTAG = 0;
	unsigned char buffTAG[100] = {0};
	int iResult = 0;

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F1A;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("\nTerminal Country Code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"\nTerminal Country Code");	
	else
		LOGF_TRACE("\nTerminal Country Code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F28;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		//dumpEMV(buffTAG,szTAG,"Issuer Country Code");
		LOGAPI_HEXDUMP_TRACE("Issuer Country Code",buffTAG,szTAG);	
	else
		LOGF_TRACE("\nIssuer Country Code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F1B;
	iResult = getCT_EMV_TAG(0, &TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Terminal Floor Limit",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Terminal Floor Limit");
	else
		LOGF_TRACE("\nTerminal Floor Limit is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F33;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );	
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Teminal Capabilities",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Teminal Capabilities");
	else
		LOGF_TRACE("\nTeminal Capabilities is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x84;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);	
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("DF Name",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"DF Name");
	else
		LOGF_TRACE("\nDF Name is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x4F;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG ,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		if(szTAG > 2)
			LOGAPI_HEXDUMP_TRACE("AID",buffTAG,szTAG);
			//dumpEMV(buffTAG,szTAG,"AID");
		else
		{
			if(pxTransRes.xEMVTransRes.T_9F06_AID.aidlen > 0)
				LOGAPI_HEXDUMP_TRACE("AID",pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen);
				//dumpEMV(pxTransRes.xEMVTransRes.T_9F06_AID.AID,pxTransRes.xEMVTransRes.T_9F06_AID.aidlen,"AID");
			else
				LOGF_TRACE("\nAID is not available");
		} 
			
	else
		LOGF_TRACE("\nAID is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F12;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Application Preferred Name",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Application Preferred Name");
	}
	else
		LOGF_TRACE("\nApplication Preferred Name is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x50;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP );
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
	{
		LOGAPI_HEXDUMP_TRACE("Application Level",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Application Level");
		//LOGF_TRACE("%s\n",buffTAG+2);
	}
	else
		LOGF_TRACE("\nApplication Level is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x82;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Application Interchange Profile",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Application Interchange Profile");	
	else
		LOGF_TRACE("\nApplication Interchange Profile is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x5F2A;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Transaction currency code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Transaction currency code");
	else
		LOGF_TRACE("\nTransaction currency code is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F02;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Amount Authorised Number",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Amount Authorised Number");
	else
		LOGF_TRACE("\nAmount Authorised Number is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F03;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Amount Other Number",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Amount Other Number");
	else
		LOGF_TRACE("\nAmount Other Number is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8F;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Certification Authory Public Key Index",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Certification Authory Public Key Index");
	else
		LOGF_TRACE("\nCertification Authory Public Key Index is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8E;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cardholder Verification Method List",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Cardholder Verification Method List");
	else
		LOGF_TRACE("\nCardholder Verification Method List is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0E;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Denial",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Denial");
	else
		LOGF_TRACE("\nIssuer Action Code - Denial is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0F;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Online",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Online");
	else
		LOGF_TRACE("\nIssuer Action Code - Online is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F0D;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Issuer Action Code - Default",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Issuer Action Code - Default");
	else
		LOGF_TRACE("\nIssuer Action Code - Default is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9B;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Transaction Status Information",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Transaction Status Information");
	else
		LOGF_TRACE("\nTransaction Status Infortation is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x95;
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Terminal Verification Result",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Terminal Verification Result");
	else
		LOGF_TRACE("\nTerminal Verification Result is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F27;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cryptogram information Data",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Cryptogram information Data");
	else
		LOGF_TRACE("\nCryptogram information Data is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x8A;
	iResult =  getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Authorization Response Code",buffTAG,szTAG);
		//dumpEMV(buffTAG,szTAG,"Authorization Response Code");
	else
		LOGF_TRACE("\nAuthorization Response Code is not available");

	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F34;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Cardholder Verification Method Result",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Cardholder Verification Method Result");	
	else
		LOGF_TRACE("\nCardholder Verification Method Result is not available");
	
	szTAG = 0;
	TAG = 0;
	memset(buffTAG,0x00,sizeof(buffTAG));
	TAG = 0x9F07;	
	iResult = getCT_EMV_TAG(0,&TAG, 1,buffTAG, sizeof(buffTAG), &szTAG,CTS_CHIP);
	if( (iResult == EMV_ADK_OK) && (szTAG > 0) )
		LOGAPI_HEXDUMP_TRACE("Application Usage Control",buffTAG,szTAG);	
		//dumpEMV(buffTAG,szTAG,"Application Usage Control");	
	else
		LOGF_TRACE("\nApplication Usage Control is not available");
	
}

/*************************************************dataforPIN*************************************************************************************************

static void dataforPIN(EMV_CT_ENTER_PIN_STRUCT* dataPin){

int r, digits, pinAlgo, autoEnter, clearAll, echoChar, bypassKey, timeoutMs;
LOGF_TRACE("\n--dataforPIN--");

dataPin->pcHtml= "@helper_pin.html";
dataPin->ucPinAlgo = EMV_PIN;
dataPin->ucAutoEnter = 0;
dataPin->ucClearAll = 0;
dataPin->ulEchoChar = '*';
dataPin->ulBypassKey = 0;
dataPin->lTimeoutMs = 30000;
dataPin->ucEnableCallback =1;

dataPin->InfoIncludedData[0] |= EMV_CT_ENTER_PIN_HTML | EMV_CT_ENTER_PIN_PIN_ALGO | EMV_CT_ENTER_PIN_CLEAR_ALL | EMV_CT_ENTER_PIN_ECHO_CHAR | EMV_CT_ENTER_PIN_TIMEOUT | EMV_CT_ENTER_PIN_ENABLE_CALLBACK;
dataPin->pcExtra = "Introduzca PIN";                 //forwarded as value["extra"] to GUI
dataPin->InfoIncludedData[2] |= EMV_CT_ENTER_PIN_EXTRA;

if(EMV_Region > 0)
{	
	switch(EMV_Region)
	{
		case 1:
			dataPin->pcCurrentRegionPath = "1";
		break;	
		case 2:
			dataPin->pcCurrentRegionPath = "2";
		break;	
		case 3:
			dataPin->pcCurrentRegionPath = "3";
		break;	
	}	
	dataPin->InfoIncludedData[3] |= EMV_CT_ENTER_PIN_CUR_REGION_PATH;	
}	

}*/

//************************************************************************************************************************************************************
/*************************************************PIN_OFFLINE************************************************************************************************

unsigned char PIN_OFFLINE(EMV_CT_ENTER_PIN_STRUCT* dataPin,unsigned char* pucPINResult){
	int rsp = 0;
	int status = -1;
	char region[10] = {0};	

	LOGF_TRACE("\n--PIN_OFFLINE--");		

	rsp = EMV_CT_Enter_PIN_extended(dataPin);
		LOGF_TRACE("\n Result of EMV_CT_Enter_PIN_extended: %i",rsp);

	memset(pucPINResult,0x00,sizeof(pucPINResult));	
	
	if(rsp == EMV_CT_PIN_INPUT_OKAY) 	
		rsp = EMV_CT_Send_PIN_Offline(pucPINResult);

	if(EMV_Region > 0)	
		uiLeaveRegion();		
	
	
	return rsp;
}
*/

//************************************************************************************************************************************************************
//*************************************************dumpEMV****************************************************************************************************

void dumpEMV(void* input,int size,const char* name){
	char* input_frame = (char*)input;
	int input_idx = 0;
	int idx_asc = 0;
	int char_idx = 0;
	int aux = 0;
	int  output_idx = 0;
	int column = 1;
	int i = 0;
	
	LOGF_TRACE("%s:",name);
	for(input_idx = 0;input_idx < size;input_idx++){				
		for(char_idx = 0; char_idx < 2; char_idx++){
			if(char_idx == 0){
			aux = input_frame[input_idx] & 0xF0;
			aux >>= 4;
			aux &= 0x0F;			
		}else
			aux = input_frame[input_idx] & 0x0F;															
		
		if( (aux >= 0) && (aux <= 9) )		
			printf("%X",aux);
			 
		if( (aux >= 10) && (aux <= 15)){	
			printf("%X",aux);		
			aux = aux - 1;
			aux = aux & 0x07;		
			}		
		output_idx ++;				
		}	
	if(column == 8)		
		printf(" ");	
	if(column == 16){
		printf("  |  ");
			for(i = 0;i < column; i++)
			{
				if((input_frame[idx_asc] == 0x00) || ((input_frame[idx_asc] >= 0x07) && (input_frame[idx_asc] <= 0x0D)))
						printf(".");
					else
						printf("%c",input_frame[idx_asc]);
				idx_asc++;
			}
		printf("\n");
		column = 0;
	}
	else
		printf(" ");	
	column ++;
	}
	if(column > 1)
	{
		for(i = 0;i < (50 - (3*column));i++)
			printf(" ");
	
		printf("    |  ");
				for(i = 0;i < column; i++)
				{
					if((input_frame[idx_asc] == 0x00) || ((input_frame[idx_asc] >= 0x07) && (input_frame[idx_asc] <= 0x0D)))
						printf(".");
					else
						printf("%c",input_frame[idx_asc]);
					idx_asc++;
				}
	}
	printf("\n--------------------------------------------------\n");
}

/****************************************************************************************
 * @param pNode data node containing one tag or a root node
 * @return Selected application index
 ***************************************************************************************/
int SelectEmvApplication(struct BTLVNode *pNode)
{
	char lApplicationNames[EMV_ADK_DEFAULT_AIDSUPP][17];
	const char *lApplications[EMV_ADK_DEFAULT_AIDSUPP];
	BTLVTagBufType TagBufchar;
	struct BTLVNode *lBTLVNode = NULL;
	int lNumApplications = 0;
	int lEmvApplication = 0;

	while( ( (lBTLVNode = pxBTLVFindNextTag(pNode, pcBTLVTagStr2(TAG_50_APP_LABEL,TagBufchar), lBTLVNode)) != NULL ) && ( lNumApplications  < EMV_ADK_DEFAULT_AIDSUPP) )
	{
		memset(lApplicationNames[lNumApplications], 0x00, sizeof(lApplicationNames[lNumApplications]));
		snprintf(lApplicationNames[lNumApplications], MIN(lBTLVNode->uSize+1,sizeof(lApplicationNames[lNumApplications])),"%s",lBTLVNode->pucData); // for up to 9 applications
		lApplications[lNumApplications] = lApplicationNames[lNumApplications];
		lNumApplications++;
	}

	lEmvApplication = UI_ShowSelection(30000, UI_STR_SELECT_APP, lApplications, lNumApplications, 0);
	if( lEmvApplication < 0 )
		return lEmvApplication;

	//framework numbering starts with 1, increase selected number by one
	LOGF_TRACE("\nSelected Candidate %d", lEmvApplication);
	lEmvApplication++;

	LOGF_TRACE("cAppName[ucSelection - 1] = %s", lApplicationNames[lEmvApplication - 1]);
	LOGF_TRACE("\nSelected Candidate %X", lEmvApplication);
	LOGF_TRACE("\nSelected Candidate %s", lApplicationNames[lEmvApplication - 1]);
	UI_ShowMessage("APLICACION SELECCIONADA", (const char *) lApplicationNames[lEmvApplication - 1]);

	return lEmvApplication;
}

//************************************************************************************************************************************************************
//*************************************************emvCallback************************************************************************************************

void emvCallback(unsigned char *pucSend, unsigned short sSendSize, unsigned char *pucReceive, unsigned short *psReceiveSize, void* externalData)
{
  struct BTLVNode *node = NULL;
  int tag = 0;
  unsigned char init = 0;
  struct BTLVNode xBtlv;
  int inResult;
  char TagBufchar[9];

  LOGF_TRACE("EMV CALLBACK");

  vBTLVInit(&xBtlv,NULL);
  if((iBTLVImport(&xBtlv, pucSend, sSendSize, NULL, NULL)!=0)||((node=pxBTLVFindTag(&xBtlv, pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar)))==NULL)||((node = pxBTLVGetChild(node, 0))==NULL))// import TLV stream
  {
    init = 1; // import of message failed or message contains no F0-tag
  }
  
  if(init == 0)
  {
    sscanf(node->tcName, "%X", &tag);
	
    switch(tag)
    {
    	case TAG_BF7F_CTLS_CBK_TRACE:{ //Trace callback. This option happens when EMV_CT_INIT_OPT_TRACE or EMV_CT_INIT_OPT_TRACE are enabled during the Framework Init.
    		struct BTLVNode *x = NULL;
   		    char *str = NULL;
   		    
			if ((x = pxBTLVFindTag(node, pcBTLVTagStr2(TAG_TRACE,TagBufchar))) != NULL) // TAG_TRACE
			{
				str = (char*) malloc(x->uSize + 1);
				if (str){
					memcpy(str, x->pucData, x->uSize);
					str[x->uSize] = 0x00;
					LOGF_TRACE("EMV ADK: %s", str);
					free(str);
				}
			}
			init=1;
			break;
    	}

    	//Refer to EMV_CT_Interface.h for details: e.g. Tag DF70 is what application gets from the Framework and tag DF70 is what application is supposed to send back
    	case TAG_BF0B_INIT_CALLBACK_THREAD:{         
              init = 1;
              break;
    	}

    	case TAG_BF01_CBK_MERCHINFO:{ 				//Refer to EMV_CT_Interface.h for details: e.g. Tag DF70 is what application gets from the Framework and tag DF70 is what application is supposed to send back
    		unsigned char merchantInfo;
    		LOGF_TRACE("----------> CALLBACK MERCHANT INFO");
    		if(iBTLVExtractTag(node, pcBTLVTagStr2(TAG_DF70_CBK_MERCHINFO, TagBufchar),&merchantInfo, sizeof(merchantInfo)) == sizeof(merchantInfo))
    			LOGF_TRACE("Merchant Info: (%#.2x)\n", merchantInfo);
    		else
    			init = 1;
    		break;
    	}

    	case TAG_BF02_CBK_AMOUNTCONF: {				// Callback for possible Amount confirmation during the transaction if not done in combination with PIN entry
    		unsigned char tucAmount[6];
    		char szFormattedAmount[14];

    		LOGF_TRACE("----------> CALLBACK AMOUNT CONFIRMATION");
    		if(iBTLVExtractTag(node, pcBTLVTagStr2(TAG_9F02_NUM_AMOUNT_AUTH,TagBufchar), tucAmount, sizeof(tucAmount)) == sizeof(tucAmount))
    		{
    			inFormatAmountForDisplay(tucAmount, szFormattedAmount, sizeof(szFormattedAmount));
    			sprintf((char *)szFormattedAmount,"%s %.2f",gszCurrecyCode,atof((char *)szFormattedAmount)); //Add currency and confirmation text
    			inResult=inGUIInputAction(CONFIRMAMOUNTOPTION, "Confirm Amount", szFormattedAmount, NULL, 0);
    			if(inResult == 0)
    				gucAmountConfirmed = TRUE;
    			vBTLVClear(&xBtlv);
    			if((node = pxBTLVAppendTag(&xBtlv, (const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0)) == NULL){
    				init = 1;
    				break;
    			}
    			if(pxBTLVAppendTag(node, (const char *) pcBTLVTagStr2(TAG_DF71_CBK_AMOUNTCONF,TagBufchar), &gucAmountConfirmed, 1) == NULL){
    				init = 1;
    				break;
    			}
    		}
    		else
    			init = 1;
    		break;
    	}
		// Callback after Read Records to modify parameters, once we know the PAN
        case TAG_BF06_CBK_LOCAL_CHECKS: {  //Braces are needed in VOS to begin-end the case.
            unsigned char T_57_PAN[10]; // 10 hex characters = 19+1 asccii digits
            unsigned char T_9F1B_FloorLimit[4];
            unsigned char T_5F24_AppExpDate[3];
            unsigned char T_9F02_Amount[6];
            unsigned char T_9F42_Currency[2];

            LOGF_TRACE("----------> CALLBACK DOMESTIC CHECK AND CHANGES");
            if(iBTLVExtractTag(node, pcBTLVTagStr2(TAG_5A_APP_PAN,TagBufchar), T_57_PAN, sizeof(T_57_PAN)) > 0)
            {
                iBTLVExtractTag(node,pcBTLVTagStr2(TAG_9F1B_TRM_FLOOR_LIMIT,TagBufchar), T_9F1B_FloorLimit,sizeof(T_9F1B_FloorLimit));
                iBTLVExtractTag(node,pcBTLVTagStr2(TAG_5F24_APP_EXP_DATE,TagBufchar), T_5F24_AppExpDate, sizeof(T_5F24_AppExpDate));
                iBTLVExtractTag(node,pcBTLVTagStr2(TAG_9F02_NUM_AMOUNT_AUTH,TagBufchar), T_9F02_Amount,sizeof(T_9F02_Amount));
                iBTLVExtractTag(node,pcBTLVTagStr2(TAG_9F42_APP_CURRENCY_CODE,TagBufchar), T_9F42_Currency,sizeof(TAG_9F42_APP_CURRENCY_CODE));
				// More parameters can be fetched in the same way. Few tags are being fetched only for demo purposes. Refer to TAG_BF06_CBK_LOCAL_CHECKS to see all possible tags.
                vBTLVClear(&xBtlv);
                if((node = pxBTLVAppendTag(&xBtlv, (const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0)) == NULL){
                  init = 1;
                  break;
                }
                T_9F1B_FloorLimit[3]++; // Change the floorlimit value: increase 1 unit of currency
                T_9F1B_FloorLimit[3]--; // decrease 1 unit of currency
                if(pxBTLVAppendTag(node, (const char *) pcBTLVTagStr2(TAG_9F1B_TRM_FLOOR_LIMIT,TagBufchar), T_9F1B_FloorLimit, sizeof(T_9F1B_FloorLimit)) == NULL){
                  init = 1;
                  break;
                }
                if(pxBTLVAppendTag(node, (const char *) pcBTLVTagStr2(TAG_9F02_NUM_AMOUNT_AUTH,TagBufchar), T_9F02_Amount, sizeof(T_9F02_Amount)) == NULL){
                  init = 1;
                  break;
                }
                vdFormatCurrencyCode(T_9F42_Currency,gszCurrecyCode);
                if(pxBTLVAppendTag(node, (const char *) pcBTLVTagStr2(TAG_9F42_APP_CURRENCY_CODE,TagBufchar), T_9F42_Currency, sizeof(T_9F42_Currency)) == NULL){
				  init = 1;
				  break;
				}
            }
            else
            {
                init = 1;
            }
            break;
		}
        case TAG_BF07_CBK_DCC: {					// Callback for Dynamic currency change after Read Records
        	unsigned char T_57_PAN[10]; // 19+1
            unsigned char T_9F1B_FloorLimit[4];
            unsigned char tucAmount[6];
            unsigned char T_9F42_AppCurrencyCode[2];
            unsigned char ucDccMode;

            LOGF_TRACE("----------> CALLBACK DYNAMIC CURRENCY CHANGE");
            if(iBTLVExtractTag(node, pcBTLVTagStr2(TAG_5A_APP_PAN,TagBufchar), T_57_PAN, sizeof(T_57_PAN)) > 0)
            {
            	iBTLVExtractTag(node, pcBTLVTagStr2(TAG_9F1B_TRM_FLOOR_LIMIT,TagBufchar), T_9F1B_FloorLimit, sizeof(T_9F1B_FloorLimit));
                iBTLVExtractTag(node, pcBTLVTagStr2(TAG_9F42_APP_CURRENCY_CODE,TagBufchar), T_9F42_AppCurrencyCode, sizeof(T_9F42_AppCurrencyCode));
                iBTLVExtractTag(node, pcBTLVTagStr2(TAG_9F02_NUM_AMOUNT_AUTH,TagBufchar), tucAmount, sizeof(tucAmount));
                vBTLVClear(&xBtlv); // This Demo shows case 1: No DCC performed
                if((node = pxBTLVAppendTag(&xBtlv, (const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0)) == NULL){
                	init = 1;
                    break;
                }
                ucDccMode = MODE_DCC_NO_TRX_CONTINUE;// No DCC necessary (If DCC is necessary, change and add the parameters as mentioned above or restart the transaction with the new values)
                if(pxBTLVAppendTag(node,(const char *) pcBTLVTagStr2(TAG_DF7D_CBK_DCC_CHECK,TagBufchar), &ucDccMode, sizeof(ucDccMode)) == NULL){
                    init = 1;
                    break;
                }
            }
            else
                init = 1;
			break;
		}
		case TAG_BF08_CBK_PIN: {					// Callback for PIN entry
                struct BTLVNode *x = NULL;
                unsigned char pinType, bypass, pinResult;
                unsigned char* pucUnpredNb = NULL;
                unsigned char* pucPINPublicKey = NULL;
                unsigned char pucPINResult[32];
                unsigned short KeyLength = 0;
                unsigned long PINPublicKeyExp = 0;
                unsigned char buf[4];

                LOGF_TRACE("----------> CALLBACK PIN");
                if((iBTLVExtractTag(node, pcBTLVTagStr2(TAG_DF79_CBK_PIN_INFO,TagBufchar),&pinType, 1) == 1) && (iBTLVExtractTag(node, pcBTLVTagStr2(TAG_DF41_PIN_BYPASS,TagBufchar),&bypass, 1) == 1))
				{
					if((x = pxBTLVFindTag(node,pcBTLVTagStr2(TAG_9F37_UNPREDICTABLE_NB,TagBufchar))) != NULL){
						pucUnpredNb = x->pucData;
					}
					if((x = pxBTLVFindTag(node,pcBTLVTagStr2(TAG_DF7A_CBK_PIN_KEY_DATA,TagBufchar))) != NULL){
						KeyLength = x->uSize;
						pucPINPublicKey = x->pucData;
					}
					if(iBTLVExtractTag(node, pcBTLVTagStr2(TAG_DF7B_CBK_PIN_KEY_EXP,TagBufchar),buf, 4) == 4){
						PINPublicKeyExp = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
					}
					LOGF_TRACE("----------> PIN TYPE: %d", pinType);
					pinResult = ENTER_PIN(pinType, bypass, pucUnpredNb, pucPINResult,KeyLength, pucPINPublicKey, PINPublicKeyExp);// finally request the PIN
					vBTLVClear(&xBtlv);
					if((node = pxBTLVAppendTag(&xBtlv, (const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0)) == NULL){
						init = 1;
						break;
					}
					if(pxBTLVAppendTag(node,(const char *) pcBTLVTagStr2(TAG_DF79_CBK_PIN_INFO,TagBufchar), &pinResult, 1) == NULL){
						init = 1;
						break;
					}
					if((pinResult == EMV_CT_PIN_INPUT_OKAY)&& ((pinType == EMV_CT_PIN_INPUT_ENCIPHERED || pinType == EMV_CT_PIN_INPUT_PLAIN)))
					{
						if(pxBTLVAppendTag(node,(const char *) pcBTLVTagStr2(TAG_DF6F_CBK_PIN_ICC_RESP,TagBufchar),pucPINResult, 2) == NULL){
						  init = 1;
						  break;
						}
					}
                }
				else
					init = 1;
			break;
		}
		//This cb is activated several times during CTLS flow with different values of leds
		case TAG_BF10_CTLS_CBK_LEDS:{
			  unsigned char ucLeds;

			  iBTLVExtractTag(node,pcBTLVTagStr2(0xC8,TagBufchar),&ucLeds, sizeof(ucLeds));
			  LOGF_TRACE("---------- LED Setting: (%#.2x)", ucLeds);
			  //inGUISetLeds(ucLeds);
			
			init = 1;
			break;
		}		
		case TAG_BF14_CBK_TEXT_DISPLAY:{ //TODO: EMV_ADK_TXT_2_CARDS_IN_FIELD should be received when detecting 2 cards. However is not working properly even though CLTRXOP_L1_ERROR_CALLBACK is set.
			unsigned char callback_textID;
			char szDisplayMessage[21];

			memset(szDisplayMessage, 0, sizeof(szDisplayMessage));
			iBTLVExtractTag(node,pcBTLVTagStr2(TAG_DF8F12_DISPLAY_TEXT,TagBufchar),&callback_textID, sizeof(callback_textID));
			
			cbk_Error_Id = callback_textID;

			if(displayGetText_CB == NULL)
			{
				switch (callback_textID)
				{ 
					case EMV_ADK_TXT_RETAP_SAME:
					case EMV_ADK_TXT_RETAP_SAME_L1:
					    uiIDLE_reset_tec_status();
						memcpy(szDisplayMessage, "ERROR",5);		
						inGUIInputAction(DISPLAYMESSAGE, szDisplayMessage, (char *)RETAP_CARD_MESSAGE, NULL, 0);												
						break;
					case EMV_ADK_TXT_2_CARDS_IN_FIELD:
					    uiIDLE_reset_tec_status();
						LOGF_TRACE("----------Callback Text Display:(%#.2x) 2 card detected in the field", callback_textID);
						memcpy(szDisplayMessage, "2 Cards Detected", 16);
						inGUIInputAction(DISPLAYMESSAGE, szDisplayMessage, (char *)RETAP_CARD_MESSAGE, NULL, 0);
						break;
					default:
						memcpy(szDisplayMessage, "**********", 10);	
				}
				LOGF_TRACE("----------Callback Text Display:(%#.2x) %s", callback_textID, szDisplayMessage);
			}
			
			
			//inGUIInputAction(DISPLAYMESSAGE, szDisplayMessage, (char *)RETAP_CARD_MESSAGE, NULL, 0);
			init = 1;
			break;
		}
#ifdef Old_pindefined
		case TAG_BF0A_CBK_ENTER_PIN: {
          unsigned char ucAbort = EMV_CT_PIN_INPUT_OKAY;

          LOGF_TRACE("\n--> CALLBACK FOR ASK PIN");
          if(EMV_CT_SmartDetect(0) != EMV_ADK_SMART_STATUS_OK)
          {
				LOGF_TRACE("\n-> WHERE IS THE CARD ???????");
				ucAbort = EMV_CT_PIN_INPUT_ABORT;
          }
		  
          vBTLVClear(&xBtlv);
          if((node = pxBTLVAppendTag(&xBtlv,(const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0))== NULL)
          {
				init = 1;
				break;
          }

          if(pxBTLVAppendTag(node,(const char *) pcBTLVTagStr2(TAG_DF6C_CBK_PIN_CANCEL,TagBufchar),&ucAbort, 1) == NULL)
          {
				LOGF_TRACE("\n*****CANCELACION DE PIN***");       	
				init = 1;
				break;
          }
          break;
        }
#endif		
	// Callback for reducing the candidate --> Application Selection by Application
    // Here: Default answer: Highest prio app is automatically selected
    // TODO: Demo needs to be extended here
		case TAG_BF04_CBK_REDUCE_CAND: {
			  char ucSelection = 1;
			  unsigned long TAG = 0x4F;		  	 //ACM	
			  unsigned short TAGL;
			  unsigned char buffer[250]={0};		 
			  int i = 0;
			  unsigned char auxtmp;

			  LOGF_TRACE("--> CALLBACK APPLICATION SELECTION");  
				// to read the candidate list, TAG_50_APP_LABEL has to be searched n times inside the stream
				// ASCII, subsequently can be displayed and number 1...n includes the selection result
			  LOGF_TRACE("Apps Selection Start");

			  LOGF_TRACE("\ni %i", i);
			  ucSelection = SelectEmvApplication(node);

			  vBTLVClear(&xBtlv);
			  if( (node = pxBTLVAppendTag(&xBtlv,(const char *) pcBTLVTagStr2(TAG_F0_EMV_TEMPLATE,TagBufchar), NULL, 0) ) == NULL)
			  {
			  	LOGF_TRACE(" paso 1");
				init = 1;
				break;
			  }

			  if(pxBTLVAppendTag(node,(const char *) pcBTLVTagStr2(TAG_DF75_CBK_APP_NO,TagBufchar), (unsigned char*)&ucSelection,1) == NULL)
			  {
			  	LOGF_TRACE(" paso 2");
				init = 1;
				break;
			  }
			  
			  auxtmp=EMV_CT_fetchTxnTags(0, &TAG, 1, buffer, sizeof(buffer), &TAGL);	
			 // LOGAPI_HEXDUMP_TRACE("\nSELECTED APPLICATION",buffer,TAGL);
			  LOGAPI_HEXDUMP_TRACE("\nSELECTED APPLICATION",buffer,15);
 			  LOGF_TRACE("\nSELECTED APPLICATION TAGL : %i",TAGL);
			  LOGF_TRACE("auxtmp: %x",auxtmp);
			  //dumpEMV(buffer,TAGL,"\nSELECTED APPLICATION");
			  memset(buffer,0x00,sizeof(buffer));
			  strcpy((char*)buffer,pcBTLVTagStr2(TAG_4F_APP_ID,TagBufchar));
			  
			  LOGF_TRACE("\nSELECTED APPLICATION: %s",buffer);
			  LOGF_TRACE("\nApps Selection End");

			  break;
			}

		case TAG_BF09_CBK_CARDHOLDERINFO:
		{
			
			unsigned char T_DF64_Kernel_dbg [32] = {0};
			unsigned char cardholderInfo;
			unsigned char cardholderInforet [20] = {0};

			LOGF_TRACE("CALLBACK CARDHOLDER INFO");
   		    
   		    if((iBTLVExtractTag(node, pcBTLVTagStr2(TAG_DF6E_CBK_CARDHOLDERINFO,TagBufchar),&cardholderInfo, 1) == 1))
   		    {
   		    	LOGF_TRACE("TAG_DF6E_CBK_CARDHOLDERINFO [%d]",cardholderInfo);
   		    }

   		    if(_CHECK_INSERTED_CARD() == FALSE)
			{
				fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
				LOGF_TRACE("DURANTE PROCESO EMV");
			}
		   		    
			break;
		}
		default:
			LOGF_TRACE("\n----------Callback [%X] not defined!!!. Call back logic should be added", tag);
			init = 1;
			break;
	}
  }
  if(init == 0)
  {
	  if((*psReceiveSize = iBTLVExport(&xBtlv, pucReceive, *psReceiveSize)) <= 0){	// export TLV stream
		  init = 1; // export of message failed
	  }
  }
  vBTLVClear(&xBtlv);
  if(init != 0)
  {
	  CallbackInit_OutTLV(pucReceive, psReceiveSize);
  }
  return;
}


//************************************************************************************************************************************************************
//*************************************************emvCallback************************************************************************************************
void CallbackInit_OutTLV(unsigned char *pucReceive, unsigned short *psReceiveSize)
{
  if(pucReceive != NULL && *psReceiveSize >= 2)
  {
    memcpy(pucReceive, "\xF0\x00", 2);
    *psReceiveSize = 2;
  }
}

//************************************************************************************************************************************************************
//*************************************************inFormatAmountForDisplay***********************************************************************************

int inFormatAmountForDisplay(unsigned char *HexAmount, char *szFormattedAmount, unsigned int inFormattedAmountLength)
{
	unsigned char tucAmountAscii[13];

	if (inFormattedAmountLength<sizeof(tucAmountAscii))
		return -1;
	memset(szFormattedAmount,0,inFormattedAmountLength);
	sprintf((char *)tucAmountAscii,"%02X%02X%02X%02X%02X%02X", HexAmount[0],HexAmount[1],HexAmount[2],HexAmount[3],HexAmount[4],HexAmount[5]);
	memcpy(szFormattedAmount,tucAmountAscii,10);
	szFormattedAmount[10]='.';
	memcpy(&szFormattedAmount[11],&tucAmountAscii[10],2);
	return TRUE;
}

//************************************************************************************************************************************************************
//*************************************************vdFormatCurrencyCode***************************************************************************************
void vdFormatCurrencyCode(unsigned char *pucHexCCode, char * szFormattedCCode)
{
	sprintf(szFormattedCCode,"%X%X",pucHexCCode[0], pucHexCCode[1]);
	switch (atoi(szFormattedCCode)){   //Add more currency codes as needed
		case 124:
			strcpy(szFormattedCCode,"CA$");
			break;
		case 937:
			strcpy(szFormattedCCode,"Bs.");
			break;
		case 840:
			strcpy(szFormattedCCode,"$");
			break;
		default:
			strcpy(szFormattedCCode,"US$");
	}
}

//************************************************************************************************************************************************************
//*************************************************inGUIInputAction*******************************************************************************************
int inGUIInputAction(int inOption, const char *szTitle, char *szAux, char *szResult, int is_approved)
{
	std::vector<std::string> value;
	int hasTouch = 0;
	int displayHeight = 0;
	int inResult = 0;
	//vector<string> value(1);	
	int prev_timeout = 0;
	map<string,string> valueHtml;
	UIParams values;
	std::string temp1;

	map<string,string>::iterator i; // variable nueva
	int digits;
	
	
	
	uiGetPropertyInt(UI_DEVICE_SUPPORTS_TOUCH, &hasTouch);
	uiGetPropertyInt(UI_DEVICE_HEIGHT, &displayHeight);
	
	switch(inOption) {
	case DISPLAYMESSAGE:
		LOGF_TRACE("\nDISPLAYMESSAGE");
		/*if (szAux !=NULL)			
			uiDisplay(1,string("<p align='center' style='background-color:white;color:black'>" + string(szTitle) + "<br/><br/></p><p align='center' style='background-color:blue;color:white'>" + string(szAux) + "<br><span style='color:#ff0000'></center></p>"));
		else
			uiDisplay(1,string("<p align='center' style='background-color:blue;color:white'>" + string(szTitle) + "</p>"));
		*/
		/*valueHtml["TITLE"].assign(szTitle);
		LOGF_TRACE("  titulo : %s", valueHtml["TITLE"].c_str());
		if(is_approved)
			valueHtml["IMAGE"]="Aprobada";
		else
			valueHtml["IMAGE"]="ERROR";

		inResult = strlen(szAux);
		
		if(inResult > 0 )
		valueHtml["MSJ_LINE"].assign(szAux);

		LOGF_TRACE("  mensaje linea : %s", valueHtml["MSJ_LINE"].c_str());
		inResult = 0;
		
		uiInvokeURL(valueHtml,"EmvResults.html");*/   //Not use

		sleep(4);
		break;

	case DISPLAYMESSAGEWITHIMAGE:
		LOGF_TRACE("\nDISPLAYMESSAGEWITHIMAGE");
		
		uiDisplay(string("<p align='center' style='background-color:blue;color:white'>" + string(szTitle) + "</p><br/><center><img src='" + szAux + "'></center>"));

		sleep(3);
		break;

	case AMOUNTINUTOPTION:
		LOGF_TRACE("\nAMOUNTINUTOPTION");
		uiSetPropertyString(UI_PROP_DECIMAL_SEPARATOR,".");		// set some properties for formatted display.  Note that this can be done just once.
		uiSetPropertyString(UI_PROP_THOUSANDS_SEPARATOR,",");
		value[0]="10000";
		/*do{
			inResult = uiInput("input",value, "<p align='center' style='background-color:green;color:white'>" + string(szTitle) + "</p><p align='center' style='background-color:white;color:black'><br/><br/><center><input type='number' precision='2' prefix='$'></center></p>");
		}while(atol(value[0].c_str())==0);
		if (inResult==0){
			memset(szResult,0,inLength);
			memcpy(szResult, value[0].c_str(), strlen(value[0].c_str()));
		}
		else
			return -1;
		*/
		//memset(szResult,0,inLength);
		//memcpy(szResult, value[0].c_str(), strlen(value[0].c_str()));
		break;

	case PININPUTOPTION:
		

		LOGF_TRACE("\nPININPUTOPTION");
		
		//value[0]="";
		values.clear();
		//values["msgPIN"] = "INGRESE PIN";
		//values["amount"] = "1.0";//szAux;

		// comentamos para probar nada mas, adecuar por regiones
		
		//	inResult = uiInput("input",value, "<p align='center' style='background-color:blue;color:white'>" + string(szTitle) + "</p><p align='center' style='background-color:white;color:blue'><br/><br/><center><input size='6' type='pin' maxlength='6' allowed_chars='0123456789' style='background-color:white;color:#436599'></center>");

		//cambio en codigo para version ADK 4.3.1

		/*
		
		inResult = uiInvokeURL(values, "helper_pin.html", &guiCallback, NULL);
		*/
		//while (inResult  == UI_ERR_PERMISSION)
		
		uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);
		uiSetPropertyInt(UI_PROP_TIMEOUT, 30000);
		//values["HEADER_TITLE"]="PIN";
		values["HEADER_TITLE"]=uiPinParams["HEADER_MSG"];
		values["BODY_HEADER"]=uiPinParams["BODY_MSG"];
		sysBeepError(BEEPVOLUME);
		while ((inResult = uiInvokeURL(values, "helper_pin.html", &guiCallback, NULL)) == UI_ERR_PERMISSION)
		{
      		  //LOGF_TRACE("**** WHILE DE HELPER PIN****");
		  //usleep(10000);
		}
		//inResult = UI_ERR_TIMEOUT;
		
		if(_CHECK_INSERTED_CARD() == FALSE)
	    {
	  		fRemovedCard = 1; // FLAG AUXILAIR PARA VALIDAR TARJETA RETIRADA EN PROCESO EMV
	  		LOGF_TRACE("DURANTE PROCESO EMV");
	    }

		LOGF_TRACE("****PIN RESULT [%d]****",inResult);
		uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
		switch (inResult)
		{
			case UI_ERR_OK:
				 LOGF_TRACE(" UI ERR OK");

				for (i = values.begin(); i != values.end(); ++i)
					{
						if (!strcmp(i->first.c_str(), "pin")) 
							{
								break;
							}
					}

				if (i == values.end())
					{
						 LOGF_TRACE("EMV_CT_PIN_INPUT_OTHER_ERR");
		        			return EMV_CT_PIN_INPUT_OTHER_ERR;
		       			 }
				
		        	if (sscanf(i->second.c_str(), "%d", &digits) == 1)
					{
						 LOGF_TRACE("CT PIN INPUT OK");
		        			return EMV_CT_PIN_INPUT_OKAY;
		        		}
					
		        	if (!strcmp(i->second.c_str(), "bypass"))
					{
						LOGF_TRACE("CT PIN INPUT BYPASS");
		        			return EMV_CT_PIN_INPUT_BYPASS;
		        		}
					
		        	if (!strcmp(i->second.c_str(), "cancel"))
					{
						LOGF_TRACE("CT PIN INPUT ABORT");
		        			return EMV_CT_PIN_INPUT_ABORT;
		        		}
				 LOGF_TRACE("****return EMV_CT_PIN_INPUT_OTHER_ERR****");	
				return EMV_CT_PIN_INPUT_OTHER_ERR;
					
			case UI_ERR_TIMEOUT:
					LOGF_TRACE("UI ERR INPUT TIMEOUT");
					return EMV_CT_PIN_INPUT_TIMEOUT;

			case UI_ERR_ABORT:
					LOGF_TRACE("CT PIN ABORT");
						return EMV_CT_PIN_INPUT_ABORT;
			case UI_ERR_CANCELLED:
					LOGF_TRACE("UI_ERR_CANCELLED");
		        		return EMV_CT_PIN_INPUT_OTHER_ERR;
		    	default:

				LOGF_TRACE("****return EMV_CT_PIN_INPUT_OTHER_ERR***º");
				return EMV_CT_PIN_INPUT_OTHER_ERR;

		}

		break;

	case CONFIRMAMOUNTOPTION:{
		UIParams values;
		LOGF_TRACE("\nCONFIRMAMOUNTOPTION");
		values.clear();
		values["title"] = szTitle;
		values["text"] = szAux;
		//values["logo"] = "/home/usr1/www/images/ctls-icon-tiny-bw.png";
		uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);
		uiSetPropertyInt(UI_PROP_TIMEOUT, GENERALTIMEOUT * 1000);		
		while ((inResult = uiInvokeURL(values, "helper_message_ok_cancel.html")) == UI_ERR_PERMISSION){	
			LOGF_TRACE("  while helper_message_ok_cancel");
			usleep(10000);
			}
		uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
		return inResult;
		break;
		}
	
	default:{
		LOGF_TRACE("\nNinguna de las anteriores");
		break;
		}	
	}
	return 0;
	
}

//************************************************************************************************************************************************************
//*************************************************vdSetCTTransactionData*************************************************************************************

void Set_CT_TransactionData(EMV_CT_SELECT_TYPE* xEMVStartData, char * szAmount,char* cashback,unsigned char txntype)
{
	LOGF_TRACE("\n--Set_CT_TransactionData--\n");
	memset((unsigned char*)xEMVStartData, 0x00, sizeof(EMV_CT_SELECT_TYPE));
    // You should use the TXN_Data struct and TXNOptions in the subsequent functions (ContinueOffline) as well, just update them, once a new transaction parameter is known or valid
	xEMVStartData->InitTXN_Buildlist = BUILD_NEW;
	xEMVStartData->Info_Included_Data[0] |=  INPUT_SEL_BUILDLIST; //raise the flag

	xEMVStartData->TransType = txntype;
	xEMVStartData->Info_Included_Data[0] |=  INPUT_SEL_TTYPE; //raise the flag

    // Application Selection Parameters
	xEMVStartData->SEL_Data.No_DirectorySelect = FALSE;
	xEMVStartData->Info_Included_Data[1] |=  INPUT_SEL_NO_PSE; //raise the flag

	xEMVStartData->SEL_Data.ucCardholderConfirmation = CARD_CONF_YES; // cardholder confirmation possible and allowed
	xEMVStartData->Info_Included_Data[1] |=  INPUT_SEL_CARDCONF; //raise the flags

//AMOUNT
	bcdfromulong (xEMVStartData->TXN_Data.Amount, sizeof(xEMVStartData->TXN_Data.Amount),(unsigned long)atol(szAmount));
	xEMVStartData->Info_Included_Data[0] |=  INPUT_SEL_AMOUNT; //raise the flag
	LOGAPI_HEXDUMP_TRACE("Amount",xEMVStartData->TXN_Data.Amount,6);
	//dumpEMV(xEMVStartData->TXN_Data.Amount,6,"Amount");

	xEMVStartData->TXN_Data.Force_Online = FALSE; // no force Online
	xEMVStartData->TXN_Data.Force_Acceptance = FALSE; // no force acceptance
	xEMVStartData->TXN_Data.Online_Switch = FALSE;
	xEMVStartData->Info_Included_Data[1] |=  INPUT_SEL_FORCE_ONLINE | INPUT_SEL_FORCE_ACCEPT | INPUT_SEL_ONLINE_SWITCH; //raise the flags


  xEMVStartData->TxnOptions[0] |= EMV_CT_SELOP_CBCK_APPLI_SEL;
  xEMVStartData->TxnOptions[1] |= EMV_CT_TRXOP_AMOUNT_CONF | EMV_CT_TRXOP_MULTIPLE_RANDOM_NUMBERS;
  xEMVStartData->TxnOptions[1] |= EMV_CT_TRXOP_PIN_BYPASS_NO_SUBSEQUENT;
  xEMVStartData->TxnOptions[2] |= EMV_CT_TRXOP_LOCAL_CHCK_CALLBACK|EMV_CT_TRXOP_CARDHINFO_CALLBACK;
  xEMVStartData->TxnOptions[1] |= EMV_CT_TRXOP_NO_FALLBACK_AFTER_CVM;
  xEMVStartData->TxnOptions[4] |= EMV_CT_CUST_APPLI_SELECTION_PERFORMED;

  xEMVStartData->SEL_Data.xFallback_MS[0].ucFallback |= FB_CHIP_APP;


  xEMVStartData->Info_Included_Data[1] |=  INPUT_SEL_TXN_OPTIONS; //raise the flag
  xEMVStartData->Info_Included_Data[1] |=  INPUT_SEL_FALLBACK_MSR; //JRS
  //xEMVStartData->Info_Included_Data[2] |=  INPUT_SEL_TXN_COUNTER; //JRS
 
//DATE
	vdGetDate((unsigned char*) xEMVStartData->TXN_Data.Date, sizeof(xEMVStartData->TXN_Data.Date));
	xEMVStartData->Info_Included_Data[0] |= INPUT_SEL_DATE;    //raise the flag
//TIME
	vdGetTime((unsigned char*) xEMVStartData->TXN_Data.Time, sizeof(xEMVStartData->TXN_Data.Time));
	xEMVStartData->Info_Included_Data[0] |= INPUT_SEL_TIME;    //raise the flag

//Cash back
	LOGF_TRACE("ATOL CASHBACK [%ld]",(unsigned long)atol(cashback));
	if((unsigned long)atol(cashback) > 0L )
	{
		bcdfromulong (xEMVStartData->TXN_Data.Cashback_Amount, sizeof(xEMVStartData->TXN_Data.Cashback_Amount),(unsigned long)atol(cashback));
		xEMVStartData->Info_Included_Data[2] |= INPUT_SEL_CB_AMOUNT;	//raise the flag
		LOGAPI_HEXDUMP_TRACE("Cashback Amount",xEMVStartData->TXN_Data.Cashback_Amount,6);
	}	

	
}

//************************************************************************************************************************************************************
//*************************************************EMV_Setup_TXN_CT_Data**************************************************************************************
void EMV_Setup_TXN_CT_Data(EMV_CT_TRANSAC_TYPE* xEMVTransType, unsigned char* amount, unsigned char* ucCashbackAmount, EMV_CT_SELECT_TYPE* xEMVStartData)
{
  memset((void*) xEMVTransType, 0, sizeof(EMV_CT_TRANSAC_TYPE));
  LOGF_TRACE("\nEMV_SETUP_TXN_CT_DATA\n");  //ACM 09/09/2015
  /* Byte 1 */
  // #define  INPUT_OFL_TXN_COUNTER         0x01  ///< B1b1: @c transaction counter
  // #define  INPUT_OFL_ADD_TAGS            0x02  ///< B1b2: @c additional tags to be presented in the transaction results
  // #define  INPUT_OFL_CB_AMOUNT           0x04  ///< B1b3: @c cashback amount
  // #define  INPUT_OFL_ACCOUNT_TYPE        0x08  ///< B1b4: @c account type
  // #define  INPUT_OFL_LANGUAGE            0x10  ///< B1b5: @c language preselected
  // #define  INPUT_OFL_AMOUNT_CONF         0x20  ///< B1b6: @c amount confirmation
  // #define  INPUT_OFL_TXN_OPTIONS         0x40  ///< B1b7: @c transaction options
  // #define  INPUT_OFL_TXN_STEPS           0x80  ///< B1b8: @c transaction steps if interrupt needed instead of 1 step processing
  /* Byte 2 */
  // #define  INPUT_OFL_AMOUNT              0x01  ///< B2b1: @c txn amount
  // #define  INPUT_OFL_AMOUNT_CURRENCY     0x02  ///< B2b2: @c currency of txn
  // #define  INPUT_OFL_CUREXPONENT         0x04  ///< B2b3: @c currency exponent of txn
  // #define  INPUT_OFL_DATE                0x08  ///< B2b4: @c date of txn
  // #define  INPUT_OFL_TIME                0x10  ///< B2b5: @c time of txn
  // #define  INPUT_OFL_FORCE_ONLINE        0x20  ///< B2b6: @c force it online (suspicious, EMVCo)
  // #define  INPUT_OFL_FORCE_ACCEPT        0x40  ///< B2b7: @c force acceptance
  // #define  INPUT_OFL_ONLINE_SWITCH       0x80  ///< B2b8: @c force it online (domestic need)

  // here we continue to prvide the remaining trasnaction data (this is the reason why the same "substructure" is included StartTransaction and the ContinueOffline)
  // transaction counter (example: if the transaction counter is per terminal, you can provide with StartTransaction already, if it is per acquirer or per AID, you just know the value after final select)
  /*xEMVTransType->TXN_Data.TransCount[0] = 0;
  xEMVTransType->TXN_Data.TransCount[1] = 0;
  xEMVTransType->TXN_Data.TransCount[2] = 0;
  xEMVTransType->TXN_Data.TransCount[3] = 1;
  xEMVTransType->Info_Included_Data[0] |= INPUT_OFL_TXN_COUNTER;*/
  // cashback amount: e.g. not supported by this AID, so we set explicitely to 0, otherwise we provide the correct value to activate it
  LOGF_TRACE("ATOL CASHBACK [%ld]",(unsigned long)atol((char*)ucCashbackAmount));
  if((unsigned long)atol((char*)ucCashbackAmount) > 0L )
  {
	//memcpy((unsigned char*)xEMVTransType->TXN_Data.Cashback_Amount, ucCashbackAmount, sizeof(xEMVTransType->TXN_Data.Cashback_Amount));
	bcdfromulong (xEMVTransType->TXN_Data.Cashback_Amount, sizeof(xEMVTransType->TXN_Data.Cashback_Amount),(unsigned long)atol((char*)ucCashbackAmount));
	xEMVTransType->Info_Included_Data[0] |= INPUT_OFL_CB_AMOUNT;  
	LOGAPI_HEXDUMP_TRACE("Cashback Amount",xEMVStartData->TXN_Data.Cashback_Amount,6);
	//dumpEMV(xEMVStartData->TXN_Data.Cashback_Amount,6,"Cashback Amount");
  }
  
  // account type
  xEMVTransType->TXN_Data.uc_AccountType = 0;
  // well, if not supported, you can simply skip this
  // preselected language (typical example: At ATMs (or other attended machines) you will have this preselected language already at the beginning,
  // in all other cases it is expected that the application performs language selection after final select, once the card supported languages are known.
  // If not done by the application, a default will be chosen automatically == this example
  xEMVTransType->TXN_Data.PreSelected_Language = EMV_ADK_LANG_NO_LANG; // no preselected language
  xEMVTransType->Info_Included_Data[0] |= INPUT_OFL_LANGUAGE;

  // amount confirmation before/after PIN entry
  xEMVTransType->TXN_Data.uc_AmountConfirmation = CONFIRM_AMOUNT_BEFORE_CVM;
  xEMVTransType->Info_Included_Data[0] |= INPUT_OFL_AMOUNT_CONF;

  // you can fetch additional card/transaction data by adding a tag list, just for demonstration we add terminal caps and used CAP key Index
  xEMVTransType->TXN_Data.Additional_Result_Tags.anztag = 2;
  xEMVTransType->TXN_Data.Additional_Result_Tags.tags[0] = 0x9F33;
  xEMVTransType->TXN_Data.Additional_Result_Tags.tags[1] = 0x8F;
  xEMVTransType->Info_Included_Data[0] |= INPUT_OFL_ADD_TAGS;

  // Remark: Any change of date, time, amount, force online ... is possible here as well
  // Also the transaction options can be updated here, e.g. additionally activate DCC and amount confirmation here
  memcpy(xEMVTransType->TxnOptions, xEMVStartData->TxnOptions, sizeof(xEMVTransType->TxnOptions));
  // add DCC callback (will be called once configured for the selected AID)
  xEMVTransType->TxnOptions[1] |= EMV_CT_TRXOP_PIN_BYPASS_NO_SUBSEQUENT;
  xEMVTransType->TxnOptions[1] |= EMV_CT_TRXOP_DCC_CALLBACK_ALWAYS | EMV_CT_TRXOP_DCC_CALLBACK;
  //xEMVTransType->TxnOptions[2] |= EMV_CT_TRXOP_RND_CALLBACK;

  xEMVTransType->Info_Included_Data[0] |=  INPUT_OFL_TXN_OPTIONS; //raise the flag
  LOGF_TRACE("\nEnd EMV_Setup_TXN_CT_DATA\n");	//ACM  09/09/2015
  return;  
}
//=====================================================================================================================
//Detect_input
//---------------------------------------------------------------------------------------------------------------------
int Detect_input(unsigned char supportTech,unsigned char* chTechnology, int inFlagManual,int inMin, int inMax,char *chManual,int inTimeOut,
				unsigned char *dataBuffer,unsigned short *dataBufferLength,char *chHeader,char *chMsgLine,char *chMsgLine2)
{
	int Result = 0;
	int in_chip = 0;
	unsigned char typ_card = 0;	
	
	LOGF_TRACE("\n--Detect_input--");

	uiPinParams.clear();

	fRemovedCard = 0;
	
	if(supportTech == CTS_MSR )
	{
		in_chip = 0;		
	}
	else 
		in_chip = 1;
	
	Result = vdEmvIdle(supportTech,  dataBuffer, dataBufferLength,chTechnology, inFlagManual,inMin, inMax,chManual,inTimeOut,in_chip,chHeader,chMsgLine,chMsgLine2);
	LOGF_TRACE("\nResult = %i",Result);
	if(Result == CTS_OK)
	{			
		LOGF_TRACE("\nLOGF_TRACE Cts_Wait_selection");
		//Result = cts_WaitSelection(&typ_card, dataBuffer, dataBufferLength, 0);
		//*chTechnology = typ_card;		
		//LOGF_TRACE("\nCTLSread = %d",typ_card);
		LOGF_TRACE("\nCTLSread = %02x",*chTechnology);
	}
	//else if(Result == EMV_IDL_MANUAL)
	if(Result == EMV_IDL_MANUAL)
			*chTechnology = CTS_KBD;		
	
	return Result;
}

//=====================================================================================================================
//vdEmvIdle
//---------------------------------------------------------------------------------------------------------------------
int vdEmvIdle(unsigned char chTechnology,unsigned char *dataBuffer,unsigned short *dataBufferLength,unsigned char *typ_card, int inFlagManual,int inMin, int inMax,char *chManual,int inTimeOut,int in_chip,char *chHeader,char *chMsg , char *chMsg2)
{              

	//TEST uiInput
	int r = -1;
	int inMedio = -1;
	map<string,string> inputIdl;
	int id = 0;
	char intStr[10] = {0};
	unsigned char ucOptions[6] = {0, 0, 0, 0, 0, CTS_PURE_CARD_DETECTION};
	int prev_timeout=0;
	int region = 0;
	string temp=""; 
	int cont =0;
	string nameHTML="";
	unsigned char options[10];

	int Result = 0;
	//unsigned char typ_card = 0;

  	// variables para meter el comando de cancelacion 72
	int inRes = 0;
	char chRcvbuff[32] = {0};

	LOGF_TRACE("\n vdEmvIdle");
	inputIdl["manual"] ="";

	memset(intStr,0,sizeof(intStr));
	sprintf(intStr,"%d",inTimeOut);
	inputIdl["timeOut"] =string(intStr);
	
	inputIdl["MSG_HEADER"] = string(chHeader);
	inputIdl["BODY_MSG1"]  = string(chMsg);
	inputIdl["BODY_MSG2"]  = string(chMsg2);

	if(inFlagManual)
		inputIdl["flagmanual"] ="1";
	else
		inputIdl["flagmanual"] ="0";

	if(chTechnology & CTS_CTLS)
	{
		inputIdl["LOGO_CTLS"] ="true";	
	}
	else
	{
		inputIdl["LOGO_CTLS"] ="false";	
	}

	LOGF_TRACE("---- in_chip[%d]----",in_chip);
	if(in_chip)
		id = uiInvokeURLAsync(inputIdl,"idlemv.html",NULL,NULL);			
	else
		id = uiInvokeURLAsync(inputIdl,"fallback.html",NULL,NULL);
    
	sleep(1);
	
	memset(options, 0, sizeof(options));

	options[6] = CTS_PURE_CARD_DETECTION;
	options[8]= 0xff;							//TIMEOUT lectura MSR, exclusivo UX
	options[9]= 0xff;
/* DEPRECATED LED USAGE. USE uiConfiGLEDS
		unsigned char col_on[3] = {39, 203, 88};
		unsigned char col_off[3] = {255, 255, 255};
		LOGF_TRACE("\n----------Init EMV CTLS Framework OK");
		  		  
		  
		//Only needed if not physical leds present
		//ucRes=EMV_CTLS_LED_ConfigDesign(20, 20, col_off, col_on, 0, 85,320, 25); 	
		EMV_CTLS_LED_ConfigDesign(13, 13, col_off, col_on, 0, 40,240,20);	

*/
	unsigned char col_on[3] = {39, 203, 88};
	unsigned char col_off[3] = {194, 194, 194};
	//EMV_CTLS_LED_SetMode(CONTACTLESS_LED_MODE_API);
	//EMV_CTLS_LED_ConfigDesign(13, 13, col_on, col_off, 0, 100,240,20);	
	unsigned char ucRes;
	
	//ucRes=EMV_CTLS_LED(CONTACTLESS_LED_THIRD, CONTACTLESS_LED_ONLINE_BLINK );	
	Result = cts_StartSelection(chTechnology, inTimeOut+10, default_tec_selection_cb, NULL, options, sizeof(options));
	ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FIRST, CONTACTLESS_LED_ON);
	LOGF_TRACE("-------SLEEPING!");
	//sleep(5);

	while(1)
	{	
		//ctlsLedsChangeState ( CTLS_LED_0 | CTLS_LED_1 | CTLS_LED_2 | CTLS_LED_3 );
		//logoLedChangeState (SWITCH_ON);
		//sysBeepError(100);
		/* Descomentar para que se prendan uno por uno. (menos el primero)
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_SECOND, CONTACTLESS_LED_ON);	
		usleep(500000);
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_SECOND, CONTACTLESS_LED_OFF);	
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_THIRD, CONTACTLESS_LED_ON);	
		usleep(500000);
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_THIRD, CONTACTLESS_LED_OFF);
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FOURTH, CONTACTLESS_LED_ON);
		usleep(500000);
		ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FOURTH, CONTACTLESS_LED_OFF);
		*/
		sleep(2);
		LOGF_TRACE("ANALIZANDO MEDIOS\n");
		//sysBeepNormal(50);
		LOGF_TRACE("ANALIZANDO MEDIOS\n");
		r = uiInvokeWait(id,10);
		LOGF_TRACE("ANALIZANDO MEDIOS\n");
		LOGF_TRACE("\n r = %i",r);
	
		Result = cts_WaitSelection(typ_card, dataBuffer, dataBufferLength, 0);

		LOGF_TRACE("RESULTADO DE r[%d]",r);
		LOGF_TRACE("RESULTADO DE LECTURA[%d]",Result);
		if( r == -8)
		{
			LOGF_TRACE("\n***RETAP****");
			//inputIdl["manual"] ="";
			//sleep(2);
			uiInvokeCancel(id);
			
			id = uiInvokeURLAsync(inputIdl,"idlemv.html",NULL,NULL);	
						
			
		}		
		else if(r >= 0)
		{
			LOGF_TRACE("INGRESO MANUAL TECLA[%d]\n",r);

			memset(intStr,0,sizeof(intStr));
			sprintf(intStr,"%d",r);
			inputIdl["manual"] = string(intStr);
			memset(intStr,0,sizeof(intStr));
			sprintf(intStr,"%d",inMax);
			inputIdl["maxLen"] = string(intStr);
			uiInvokeCancel(id);
			while(1)
			{
			   
				uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);
				uiSetPropertyInt(UI_PROP_TIMEOUT, inTimeOut*1000);
				//cts_StopSelection();
				memset(intStr,0,sizeof(intStr));
				sprintf(intStr,"%d",inTimeOut);
				inputIdl["timeOut"] =string(intStr);
				
				r = uiInvokeURL(inputIdl,"manual.html");
   
				uiSetPropertyInt(UI_PROP_TIMEOUT,prev_timeout);

				if (r == 13) //ENTER
				{

					if(inputIdl["manual"].length() < inMin)
					{
						//inputIdl["error"]="Longitud Invalida";
						continue;
					}

					strcat(chManual,inputIdl["manual"].c_str());
					LOGF_TRACE("PAN MANUAL [%s]\n",chManual);
					cts_StopSelection();
					return EMV_IDL_MANUAL;
				}
				else if(r == -1)
				{
					cts_StopSelection();
					return EMV_IDL_CANCEL; //CANCELADA
				}

				else if(r == -3)
				{
					LOGF_TRACE("****TIME OUT*****\n");
					cts_StopSelection();
					return EMV_IDL_TIME_OUT; //TIME OUT

				}
			}
		}
		else if(r == -1)
		{
			uiInvokeCancel(id);
			cts_StopSelection();
			return EMV_IDL_CANCEL; //CANCELADA
		}
		else if(r == -6)
		{
			uiInvokeCancel(id);
			cts_StopSelection();
			LOGF_TRACE("****TIME OUT*****\n");
			return EMV_IDL_TIME_OUT;
		}

		if (Result == CTS_ERROR )
		{
			LOGF_TRACE("cts_StartSelection Error\n");
			continue;
		}
		//else if (/*inMedio == CTS_IN_PROGRESS &&*/ tec_callback_reached == 1 && r == -12 )
		else if ((Result == CTS_IN_PROGRESS && r == -12) || Result == CTS_TIMEOUT  )
		{

			LOGF_TRACE("****TIME OUT*****\n");
			LOGF_TRACE("****TIME OUT*****\n");
			//TIME OUT ERROR CTLS
			uiInvokeCancel(id);
			cts_StopSelection();
			LOGF_TRACE("****TIME OUT*****\n");
			return EMV_IDL_TIME_OUT;   
		}	
		else if ( Result  == CTS_OK )
		{
			//Solo si es CTLS.
			LOGF_TRACE("-----typ_card [%d]",(int)*typ_card);
			if((int)*typ_card == CTS_CTLS){
				ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FIRST, CONTACTLESS_LED_ON);
				ucRes=EMV_CTLS_LED(CONTACTLESS_LED_SECOND, CONTACTLESS_LED_ON);
				ucRes=EMV_CTLS_LED(CONTACTLESS_LED_THIRD, CONTACTLESS_LED_ON);
				ucRes=EMV_CTLS_LED(CONTACTLESS_LED_FOURTH, CONTACTLESS_LED_ON);
				usleep(2000000); //menos de un segundo
				ucRes=EMV_CTLS_LED(CONTACTLESS_LED_ALL, CONTACTLESS_LED_OFF);
			}	
			else{
				usleep(500000); //dejará prendido el primer led un momento y luego se apagará solo con cts_stopselection()
			}
			LOGF_TRACE("cts_StartSelection In Progress\n");
			uiInvokeCancel(id);
			LOGF_TRACE("cts_StartSelection In Progress\n");
			LOGF_TRACE("cts_StartSelection Detected\n");
			//cts_StopSelection();
			break;
		}
		else if ((Result != CTS_OK) &&  (Result != CTS_IN_PROGRESS) )
		{
			
			LOGF_TRACE("ALGUN ERROR EN LA LECTURA");
			break; 
		}
		else
			LOGF_TRACE("cts_StartSelection [%d]\n",Result);
			
		if(displayGetText_CB != NULL && cbk_Error_Id != 0)
		{


			LOGF_TRACE("displayGetText_CB");
			inputIdl["MSG_HEADER"] = "";
			// El mensaje de displayGetText_CB 
			inputIdl["BODY_MSG1"]  = ""; //"ERROR";
			inputIdl["BODY_MSG2"]  = displayGetText_CB(cbk_Error_Id);
			//inputIdl["BODY_MSG2"]  = 
			//uiInvokeCancel(id);
			LOGF_TRACE("-------# DEBUG! in_chip[%d]",in_chip);
			if(in_chip)
				id = uiInvokeURLAsync(inputIdl,"idlemv.html",NULL,NULL);			
			else
				id = uiInvokeURLAsync(inputIdl,"fallback.html",NULL,NULL);
		    
			cbk_Error_Id = 0;
		}
	}
	//End
	cts_StopSelection();
	//uiInvokeCancel(id);
	LOGF_TRACE("RESULTADO DE LECTURA[%d]",Result);

	return Result;

}

//=====================================================================================================================
//default_tec_selection_cb
//---------------------------------------------------------------------------------------------------------------------
 
static void default_tec_selection_cb(void *prt)
{
    uiIDLE_notify_tec_callback();
}

//=====================================================================================================================
//uiIDLE_notify_tec_callback
//---------------------------------------------------------------------------------------------------------------------
 
int uiIDLE_notify_tec_callback()
{
	LOGF_TRACE("\n*********uiIDLE_notify_tec_callback********\n");
	
	tec_callback_reached = 1;
    
    return tec_callback_reached;
}

//=====================================================================================================================
//uiIDLE_reset_tec_status
//---------------------------------------------------------------------------------------------------------------------

int uiIDLE_reset_tec_status()
{
	tec_callback_reached = 0;
    return tec_callback_reached;
}


//=====================================================================================================================
//-------------------------------------------vdGetTime-----------------------------------------------------------------
void vdGetTime(unsigned char *buf, size_t bufLen)
{
   time_t t = time(0);
   struct tm *_tm = localtime(&t);
   if (!_tm) return;

   if (bufLen < 3) return;

   memset(buf, 0, bufLen);
   bcdfromulong(buf, 1, _tm->tm_hour);
   bcdfromulong(buf + 1, 1, _tm->tm_min);
   bcdfromulong(buf + 2, 1, _tm->tm_sec);
}

//=====================================================================================================================
//-------------------------------------------vdGetDate-----------------------------------------------------------------
void vdGetDate(unsigned char *buf, size_t bufLen)
{
   time_t t = time(0);
   struct tm *_tm = localtime(&t);
   if (!_tm) return;

   if (bufLen < 3) return;

   memset(buf, 0, bufLen);
   bcdfromulong(buf, 1, _tm->tm_year % 100);
   // tm_mon is 0-11, thus + 1
   bcdfromulong(buf + 1, 1, _tm->tm_mon + 1);
   bcdfromulong(buf + 2, 1, _tm->tm_mday);
}

//=====================================================================================================================
//-------------------------------------------bcdfromulong--------------------------------------------------------------
int bcdfromulong(unsigned char *bcd, int size, unsigned long value)
{
   memset(bcd,0,size);
   while(--size>=0) {
      bcd[size]= (unsigned char) (value%10);
      value/=10;
      bcd[size]+= (unsigned char) (value%10*16);
      value/=10;
   }
   if(value) return 1;
   return 0;
}

//=====================================================================================================================
//-------------------------------------------EMV_EndTransaction_CT-----------------------------------------------------
void EMV_EndTransactionCT(const char *szStatusMessage, unsigned char ucReader, int inExitWithError)
{
	map<string,string> value;

	if (!inExitWithError)
		sysBeepError(BEEPVOLUME);
	LOGF_TRACE("\n----------Ending EMV CT Transaction");
	cts_StopSelection();
	EMV_CT_SmartPowerOff(ucReader);
	//inGUIInputAction(DISPLAYMESSAGE, szStatusMessage, (char *)REMOVE_CARD_MESSAGE, NULL, inExitWithError);
	//sleep(2);		
}
//=====================================================================================================================
//-------------------------------------------Remove_Card---------------------------------------------------------------

void Remove_Card()
{
	map<string,string> value;
	int inResult = 0;
	value["HEADER_TITLE"]="";
	value["BODY_HEADER"]="RETIRE TARJETA";
	value["BODY_MSG"]="Por Favor";
	do{
		sysBeepNormal(BEEPVOLUME);
		inResult=cts_WaitCardRemoval2(1);
		if(inResult== CTS_OK)
			break;
		else
		{			
		
		    uiInvokeURL(value,"remove_card.html");
			
			if(inResult== CTS_ERROR)
			{				
				LOGF_TRACE("\n----------cts_WaitCardRemoval2 Error %02X", inResult);
				break;
			}
		}
	}while(inResult==CTS_TIMEOUT);	
	
}



//=====================================================================================================================
//-------------------------------------------EMV_EndTransactionCTLS-----------------------------------------------------
void EMV_EndTransactionCTLS(const char *szStatusMessage, unsigned char ucReader, int inExitWithError)
{
	unsigned char ucRes;
	LOGF_TRACE("\n--EMV_EndTransactionCTLS--");
	if (!inExitWithError)
		sysBeepError(BEEPVOLUME);

	cts_StopSelection();
	EMV_CTLS_SmartPowerOff(ucReader);
	//inGUIInputAction(DISPLAYMESSAGE, szStatusMessage, (char *)THANKS, NULL, inExitWithError);
	//sleep(2);
	//ucRes=EMV_CTLS_LED(0, CONTACTLESS_LED_IDLE_BLINK);						//Only needed if not physical leds present
	LOGF_TRACE("\n----------Init EMV CTLS Leds Blink 0x%02X", ucRes);
}

//=====================================================================================================================
//-------------------------------------------getMSR_data---------------------------------------------------------------

int Check_typ_CTLS()
{
int iResult = 0;	
if(CTLSTransRes.T_DF61_Info_Received_Data[0] & TRX_CTLS_STATUSINFO)
	{
	    if(CTLSTransRes.StatusInfo & EMV_ADK_SI_CONTACTLESS_CHIP)
		{
			LOGF_TRACE("\n***CTLS CHIP****");
			iResult = CTLS_TXN_CHIP;
		}
		else if(CTLSTransRes.StatusInfo & EMV_ADK_SI_CONTACTLESS_MSR)
		{			
			LOGF_TRACE("\n***CTLS MSR****");
			iResult = CTLS_TXN_MSR;
		}
		else 
			iResult = CTLS_ERROR;
    }
return iResult;
}

//=====================================================================================================================
//-------------------------------------------getMSR_data---------------------------------------------------------------

int getMSR_data(char* rqData, char* outBuff,int* szbuff)
{
	int iResult = 0;
	*szbuff = 0;

	
	LOGF_TRACE("getMSR_data");
	LOGF_TRACE("Tracks.t1.status[%d]  Datas.t1.valid[%d]",Tracks.t1.status,Datas.t1.valid);
	LOGF_TRACE("Tracks.t2.status[%d]  Datas.t2.valid[%d]",Tracks.t2.status,Datas.t2.valid);

	if(!memcmp("Track1",rqData,6) || !memcmp("track1",rqData,6))
	{
		if(Tracks.t1.status == 0 && Datas.t1.valid == 1)
		{
			strcpy(outBuff,Tracks.t1.data);
			*szbuff = strlen(Tracks.t1.data);
			iResult = VALID_MSR_DATA;
		}
		else
		{
			//Track 1 exist but format not is correct but if track2 is valid we build track1 dummy
			if (Tracks.t1.status == 0  && Datas.t2.valid == 1)
			{

				LOGF_TRACE("BUILDING TRACK1 DUMMY");
				memset(Datas.t1.name,0x20,sizeof(Datas.t1.name)-1);
				memcpy(Datas.t1.pan,Datas.t2.pan,sizeof(Datas.t2.pan));
				memcpy(Datas.t1.exp_date,Datas.t2.exp_date,sizeof(Datas.t2.exp_date));
				memcpy(Datas.t1.service_code,Datas.t2.service_code,sizeof(Datas.t2.service_code));
				memcpy(Datas.t1.disc_data,Datas.t2.disc_data,sizeof(Datas.t2.disc_data));
				sprintf(outBuff,"\%B%s^%s^%s%s%s",
					Datas.t1.pan,
					Datas.t1.name,
					Datas.t1.exp_date,
					Datas.t1.service_code
					,Datas.t1.disc_data);
				Datas.t1.valid = 1;
				LOGAPI_HEXDUMP_TRACE("TRACK1 Dummy",outBuff,strlen(outBuff));
				iResult = VALID_MSR_NOT_ST_DAT;

			}
			else
				iResult = INVALID_MSR_DATA;
			
		}
	}
	else if(!memcmp("Track2",rqData,6) || !memcmp("track2",rqData,6))
		{
			if(Tracks.t2.status == 0  && Datas.t2.valid == 1)
			{
				strcpy(outBuff,Tracks.t2.data);
				*szbuff = strlen(Tracks.t2.data);
				iResult = VALID_MSR_DATA;
			}
			else
				iResult = INVALID_MSR_DATA;
		}
	else if(!memcmp("Track3",rqData,6) || !memcmp("track3",rqData,6))
		{
			if(Tracks.t3.status == 0)
			{
				strcpy(outBuff,Tracks.t3.data);
				*szbuff = strlen(Tracks.t3.data);
				iResult = VALID_MSR_DATA;
			}
			else
				iResult = INVALID_MSR_DATA;
		}
	else if(!memcmp("cardholder",rqData,10) || !memcmp("Cardholder",rqData,10))
		{
			if(Datas.t1.valid == 1)
			{				
				strcpy(outBuff,Datas.t1.name);
				*szbuff = strlen(Datas.t1.name);
				iResult = VALID_MSR_DATA;
			}
			else
				iResult = INVALID_MSR_DATA;
			
		}
	else if(!memcmp("Pan",rqData,3) || !memcmp("pan",rqData,3))
		{
			if(Datas.t1.valid == 1)
			{				
				strcpy(outBuff,Datas.t1.pan);
				*szbuff = strlen(Datas.t1.pan);
				iResult = VALID_MSR_DATA;
			}
			else if(Datas.t2.valid == 1)
				{					
					strcpy(outBuff,Datas.t2.pan);
					*szbuff = strlen(Datas.t2.pan);
					iResult = VALID_MSR_DATA;
				}
				else
					iResult = INVALID_MSR_DATA;
		}
	else if(!memcmp("ExpDate",rqData,7)|| !memcmp("expdate",rqData,7))
		{
			if(Datas.t1.valid == 1)
			{				
				strcpy(outBuff,Datas.t1.exp_date);
				*szbuff = strlen(Datas.t1.exp_date);
				iResult = VALID_MSR_DATA;
			}
			else if(Datas.t2.valid == 1)
				{					
					strcpy(outBuff,Datas.t2.exp_date);
					*szbuff = strlen(Datas.t2.exp_date);
					iResult = VALID_MSR_DATA;
				}
				else
					iResult = INVALID_MSR_DATA;
		}
	else if(!memcmp("ServCode",rqData,8) || !memcmp("servcode",rqData,8))
		{
			if(Datas.t1.valid == 1)
			{				
				strcpy(outBuff,Datas.t1.service_code);
				*szbuff = strlen(Datas.t1.service_code);
				iResult = VALID_MSR_DATA;
			}
			else if(Datas.t2.valid == 1)
				{					
					strcpy(outBuff,Datas.t2.service_code);
					*szbuff = strlen(Datas.t2.service_code);
					iResult = VALID_MSR_DATA;
				}
				else
					iResult = INVALID_MSR_DATA;
		}
	else if(!memcmp("DiscData",rqData,8) || !memcmp("discdata",rqData,8))
		{
			if(Datas.t1.valid == 1)
			{				
				strcpy(outBuff,Datas.t1.disc_data);
				*szbuff = strlen(Datas.t1.disc_data);
				iResult = VALID_MSR_DATA;
			}
			else if(Datas.t2.valid == 1)
				{					
					strcpy(outBuff,Datas.t2.disc_data);
					*szbuff = strlen(Datas.t2.disc_data);
					iResult = VALID_MSR_DATA;
				}
				else
					iResult = INVALID_MSR_DATA;
		}
	else
		iResult = DATA_NOT_FOUND;

	return iResult;

	
	LOGF_TRACE("getMSR_data iResult=%d",iResult);
	
}

//=====================================================================================================================
//-------------------------------------------Read_MSRdata--------------------------------------------------------------

int Read_MSRdata()
{	
	memset(&Tracks,0x00,sizeof(Tracks));
	memset(&Datas,0x00,sizeof(Datas));
	
	int MSRread = 0;
	//MSRread = MSR_GetData(10,&Tracks,&Datas);	
	MSRread = MSR_GetData2(10,&Tracks,&Datas);

	LOGF_TRACE("CARD TYPE [%d]",Tracks.card_type);

	LOGAPI_HEXDUMP_TRACE("MSR DATA",&Tracks,sizeof(MSR_TrackData2));
	LOGAPI_HEXDUMP_TRACE("MSR TRK1",&Tracks.t1,sizeof(MSR_Track_1));
	LOGAPI_HEXDUMP_TRACE("MSR TRK2",&Tracks.t2,sizeof(MSR_Track_2));
	LOGAPI_HEXDUMP_TRACE("MSR TRK2",&Tracks.t3,sizeof(MSR_Track_3));

	return MSRread;

}
//=====================================================================================================================
//-------------------------------------------Disable_swp---------------------------------------------------------------

int Disable_swp(){
	int inRet = 0;		
	LOGF_TRACE("\n--Disable_swp--");	
	inRet = MSR_Deactivate();																	
	return inRet;		
}

//=====================================================================================================================
//-------------------------------------------Enable_swp----------------------------------------------------------------

int Enable_swp(){
	int inRet = 0;																						
	LOGF_TRACE("\n--Enable_swp--");
	inRet = MSR_Activate(NULL,NULL);																	
	return inRet;	
}


//=====================================================================================================================
//screenExpDate
//---------------------------------------------------------------------------------------------------------------------
int screenExpDate(char* Exp_Date,char *chMsgHeader, char *chMsgBody){

	int inRes = -1;
	int btnResult = 0;
	int prev_timeout = 0;
	char chExpDate[6+1] = {0};
	char chMes[3+1] = {0};
	char chAnio[3+1] = {0};
	
	char chActualDate[10+1] = {0};
	char chAnioActual[3+1] = {0};
	char chMesActual[3+1] = {0};
	string temp=""; 

	std::map<std::string, std::string> InfoCard;
	

	LOGF_TRACE("\n--screenExpDate--");
	
	//OBTENEMOS FECHA ACTUAL
	vdGetDate((unsigned char*) chActualDate, sizeof(chActualDate));

	sprintf(chAnioActual,"%02x",chActualDate[0]);
	sprintf(chMesActual,"%02x",chActualDate[1]);
	LOGF_TRACE("******FECHA ACTUAL MM [%s] / YY [%s]**********\n",chMesActual,chAnioActual);

	uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);
	
	//InfoCard["BODY_HEADER"]="FECHA DE EXPIRACION";
	//InfoCard["BODY_MSG"]="MM/YY";

	InfoCard["BODY_HEADER"]=string(chMsgHeader);
	InfoCard["BODY_MSG"]=string(chMsgBody);

	InfoCard["manual"] = "\x00";
	InfoCard["maxLen"] = "4";
	
	
	while(inRes < 0)
	{
		uiSetPropertyInt(UI_PROP_TIMEOUT, 30000);
		InfoCard["manual"]=temp;
		sysBeepError(BEEPVOLUME);
		
		btnResult = uiInvokeURL(InfoCard, "manualExpDate.html");

		temp=InfoCard["manual"];
		
		if(btnResult == 27)
		{
			LOGF_TRACE("CANCELADA POR USUARIO");
			inRes = 1;
		}
		else if(btnResult == -3)
		{
			LOGF_TRACE("EXP DATE TIME OUT");
			inRes = 4;
		}
		else if((btnResult == 13) && (temp.length() == 4))
		{
			//VALIDAMOS FECHA
			inRes = 0; //FECHA VALIDA
			strcat(chExpDate,temp.c_str());
			memcpy(chMes,chExpDate,2);
			memcpy(chAnio,&chExpDate[2],2);
			LOGF_TRACE("******FECHA INGRESADA MM [%s] / YY [%s]**********\n",chMes,chAnio);

			if( atoi(chMes) <= 0 || atoi(chMes) > 12)
				inRes = 2; //FECHA INVALIDA
			else 
			{
				if(atoi(chAnioActual) > atoi(chAnio)) //EXPIRADA
				{
					LOGF_TRACE("TARJETA EXPIRADA POR A?");
					inRes = 3;
				}
				else if(atoi(chAnioActual) == atoi(chAnio))
				{
					if(atoi(chMesActual) > atoi(chMes)) //EXPIRADA
					{
						LOGF_TRACE("TARJETA EXPIRADA POR MES");
						inRes = 3;
					}
					else
					{
						LOGF_TRACE("FECHA VALIDA");
						sprintf(Exp_Date,"%s%s",chAnio,chMes);
					}
				}
				else
					{
						LOGF_TRACE("FECHA VALIDA");
						sprintf(Exp_Date,"%s%s",chAnio,chMes);
					}
			}
		}
	}

	uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
	return inRes;

}

//=====================================================================================================================
//screenSec_Code
//---------------------------------------------------------------------------------------------------------------------
int screenSec_Code(char* SECCODE,int maxlentgh,char *chMsgHeader,char *chMsgBody){
	char maxlen[3] = {0};
	unsigned char SEC_CODE[5] = {0};
	char chSECCODE[5] = {0};
	long amount = 0;
	int counterErrors = 0;
	std::map<std::string, std::string> InfoCard;
	int prev_timeout = 0;
	int flagSECCODE = -1;
	int btnCancel = 0;
	std::string MAXLEN = "";
	string temp=""; 
	int cont =0;
	// variables para meter el comando de cancelacion 72
	int inRes = 0;
	char chRcvbuff[16] = {0};

	LOGF_TRACE("\n--screenSECCODE--");	

	memset(SECCODE,0,sizeof(SECCODE));	
	sprintf(maxlen,"%d",maxlentgh);
	LOGF_TRACE("\nmaxlen = %s",maxlen);
	MAXLEN.clear();
	MAXLEN.assign((const char*)maxlen);
	LOGF_TRACE("\nMAXLEN = %s",MAXLEN.c_str());	

	counterErrors = 1;	
	
	InfoCard["dataCard"] = "Codigo de seguridad:";
	InfoCard["manual"] = "\x00";
	//InfoCard["length"] = "4";		
	InfoCard.insert(pair<string,string>( "maxLen",MAXLEN)); 
	//InfoCard["BODY_HEADER"]="INGRESE CODIGO DE SEGURIDAD";     // FAG 23-FEB-2017
	//InfoCard["BODY_MSG"]="";                          // FAG 23-FEB-2017
	InfoCard["BODY_HEADER"]=string(chMsgHeader);
	InfoCard["BODY_MSG"]=string(chMsgBody);
	
	uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);

	do{
	
		//uiSetPropertyInt(UI_PROP_TIMEOUT, 12000);
		uiSetPropertyInt(UI_PROP_TIMEOUT, 30000);
		sysBeepError(BEEPVOLUME);
		
		btnCancel = uiInvokeURL(InfoCard, "manualSecCode.html");
		LOGF_TRACE("\nbtnCancel = %i",btnCancel);
		if(btnCancel == -3)
		{
			LOGF_TRACE("btnCancel = -3 ");
			uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
			//return btnCancel;
			return -6;
		}
			
		else if(btnCancel==-1)
		{
			LOGF_TRACE("cancelado por el usuario");
			LOGF_TRACE("datos del ingreso  -> %s",InfoCard["manual"].c_str());
			LOGF_TRACE("datos del temporal	-> %s",temp.c_str());
			uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
			return btnCancel;			
		}

		else if(btnCancel==13)
		{
			LOGF_TRACE("aceptar");
			LOGF_TRACE("datos del ingreso  -> %s",InfoCard["manual"].c_str());
			LOGF_TRACE("datos del temporal	-> %s",temp.c_str());
			//break;
			
			//memcpy(SEC_CODE,InfoCard["manual"].c_str(),4);
			memcpy(SEC_CODE,InfoCard["manual"].c_str(),InfoCard["manual"].length());
			memcpy(chSECCODE,SEC_CODE,4);
			uiSetPropertyInt(UI_PROP_TIMEOUT,prev_timeout);
		
			if(strlen(chSECCODE) >= 3 && strlen(chSECCODE) <= 4)
			{	
				flagSECCODE = UI_ERR_OK;
				memcpy(SECCODE,chSECCODE,strlen(chSECCODE));	
				LOGF_TRACE("\nCVV2 in screenCVV = %s",SECCODE);
				return flagSECCODE;
			}
			//else if(strlen(chSECCODE)==0)
			else if(strlen(chSECCODE)<3)
			{
				LOGF_TRACE("No se escribio ningun CVV o no cumple con el minimo");
				memset(SEC_CODE,0,sizeof(SEC_CODE));
				
				//JRS FIX BBVA
				flagSECCODE = UI_ERR_OK;
				memset(SEC_CODE,0x20,3);
				return -84;
				
			}

			else

			{
				counterErrors = counterErrors + 1;
			}

		}

		
		
	}while(counterErrors <= 5 && flagSECCODE == -1);
	LOGF_TRACE("\ncounterErrors = %i",counterErrors);
	if(counterErrors > 4)
	{
		btnCancel = -1;
		memset(SECCODE,0,sizeof(SECCODE)); //Clean CVV2 due to there was not a valid value
		return btnCancel;	
	}
	
	return flagSECCODE;
}

int UI_ShowSelection(int timeout, const char* title, const char* elements[], const int num_elements, int preselect)
{
 
  #ifdef _VRXEVO
  //struct UIMenuEntry *menu_items;
  struct UIMenuEntry menu_items[20];
  #else
  struct UIMenuEntry menu_items[num_elements];
  #endif
  
   //struct UIMenuEntry menu_items[10];
  int prev_timeout;  
  int r = 0;

 /*#ifdef _VRXEVO
	menu_items = (UIMenuEntry*)malloc(num_elements);
 #endif*/

  for(int i = 0; i < num_elements; i++)
  {
    menu_items[i].options = 0;
    menu_items[i].text = elements[i];
    menu_items[i].value = i;
  }

    // Use the select template for the menu
  uiGetPropertyInt(UI_PROP_TIMEOUT, &prev_timeout);
  uiSetPropertyInt(UI_PROP_TIMEOUT, timeout);  
    // Loading helper_select.tmpl
  if(EMV_Region > 0)
	//r = uiMenu(EMV_Region,"helper_select", title, menu_items, lengthof(menu_items), preselect);
	r = uiMenu(EMV_Region,"helper_select", title, menu_items, num_elements, preselect);
  else 
	//r = uiMenu("helper_select", title, menu_items, lengthof(menu_items), preselect);
	r = uiMenu("helper_select", title, menu_items, num_elements, preselect);
  
  
  uiSetPropertyInt(UI_PROP_TIMEOUT, prev_timeout);
  LOGF_TRACE("----------uiMenu Key = %i", r);

  uiSelectionTimeout = r;
  
 /*#ifdef _VRXEVO
	free(menu_items);
 #endif
 */

  return(r);
}

int UI_ShowMessage(const char* title, const char* text)
{
  int iResult = 0;
  UIParams values;
  LOGF_TRACE("UI_ShowMessage");
  //values["title"] = title;
  values["title"].assign(title);
  //values["text"] = text;
  values["text"].assign(text);
  LOGF_TRACE("title = %s",values["title"].c_str());
  LOGF_TRACE("text = %s",values["text"].c_str());
 
iResult = uiInvokeURL(values, "helper_message.html");

  return iResult;
}



bool guiCallback(void *data) {
  if (guiCb != NULL) {
    return guiCb() == 0 ? true : false;
  }
  return true;
}


void getversionHW(char * output)
{
	std::string version="";
	sysGetPropertyString(SYS_PROP_HW_MODEL_NAME,version);
	LOGF_TRACE("HW = %s",version.c_str());
	strcpy(output,version.c_str());
}

void setDisplayGetText_CB(DisplayGetText_CB cb) 
{
	displayGetText_CB = cb;
}

void vdIncraseTSNCts()
{

	int inRet = 0;
	int inAux = 0;
	char tmp [10] = {0};
	char chTag9F41[16] = {0};
	unsigned char tlvBuffer[8] = {0};

	LOGF_TRACE("****vdIncraseTSNCts****");

	inRet = _getEnvFile_((char *)"EMVUSRDATA", (char *)"9F41CT", tmp,sizeof(tmp));

	LOGF_TRACE("inRet [%d] ", inRet);

	inAux = atoi(tmp);

	if(inRet != 0)
		sprintf(chTag9F41,"9F4104%08d",inAux);
	else
		strcat(chTag9F41,"9F410400000001");

	LOGF_TRACE("chTag9F41 [%s] ", chTag9F41);
	vdAsc2Hex(chTag9F41,(char*)tlvBuffer,7);
	LOGAPI_HEXDUMP_TRACE("TAG 9F41",tlvBuffer,7);

	inRet = EMV_CT_updateTxnTags (EMV_ADK_UPDATETAGS_ERROR_ON_NOT_ALLOWED_TAG, tlvBuffer,7);

	
	if(inAux == 999999)
		inAux = 1;
	else
		inAux += 1;
	
	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp,"%d",inAux);
	_putEnvFile_((char *)"EMVUSRDATA",(char *)"9F41CT",tmp);

	LOGF_TRACE("Result: [%d]",inRet);
}


void  vdAsc2Hex (const char *dsp, char *hex, int pairs)
{
	//SUPPOSE dsp CONTAINS THE ASSCII ARRAY "12345F" AND WE EXECUTE THIS FUNCTION
	//THEN THE ARRAY  AT hex WILL CONTAIN 12H,34H, 5FH
	int i;
	for(i=0;i<pairs;i++)
	{
		hex[i] =  ((dsp[i * 2] <= 0x39) ? dsp[i * 2] - 0x30 : (dsp[i * 2] & 0xDF) - 0x41 + 10);
	    hex[i] = hex[i] << 4;
	    hex[i] += ((dsp[i * 2 + 1] <= 0x39) ? dsp[i * 2 + 1] - 0x30 : (dsp[i * 2 + 1] & 0xDF) - 0x41 + 10);
	}	
	//return;
}

void print_structMSR(void)
{
	LOGAPI_HEXDUMP_TRACE("estructura de MSR Tracks-> ",&Tracks,sizeof(Tracks));
	LOGAPI_HEXDUMP_TRACE("estructura de MSR Datas-> ",&Datas,sizeof(Datas));

}

int getPAN_structMSR(char* inputbuff)  // FAG 17-nov-2017
{
	char tmpbuff[41]={0};
	char *pos1=NULL;
	int in_val=0;
	if(Tracks.t2.status==0)
	{
	
		strcpy(tmpbuff,Tracks.t2.data+1);	
		LOGAPI_HEXDUMP_TRACE("tmpbuff-> ",tmpbuff,sizeof(tmpbuff));
		pos1=strstr(tmpbuff,"=");
		//LOGAPI_HEXDUMP_TRACE("pos-> ",pos1,2);
		if(pos1==NULL)
		{
			LOGF_TRACE("SIGNO NO ENCONTRADO");
		}
		else
		{
			*pos1=0;
		}
		
		//LOGAPI_HEXDUMP_TRACE("pos-> ",pos1,2);
		LOGAPI_HEXDUMP_TRACE("tmpbuff 2 -> ",tmpbuff,sizeof(tmpbuff));
		strcpy(inputbuff,tmpbuff);
		LOGAPI_HEXDUMP_TRACE("inputbuff-> ",inputbuff,25);
		//memset(inputbuff,0x00,sizeof(inputbuff));
		//memcpy(inputbuff,tmpbuff,16);
		in_val=strlen(inputbuff);
	}
	else
	{
		LOGF_TRACE("INVALID READ");
		in_val=INVALID_MSR_DATA;

	}
	return in_val;

}

int getstatus_tracks(int numtrack)  // FAG 27-nov-2017
{
	if(numtrack==1)
	{
		return Tracks.t1.status;
	}
	else if(numtrack==2)
	{
		return Tracks.t2.status;
	}

	else if(numtrack==3)
	{
		return Tracks.t3.status;
	}
}

int getCardholder_structMSR(char* inputbuff)  // FAG 30-nov-2017
{
	char tmpbuff[41]={0};
	char *pos1=NULL;
	int in_val=0;
	if(Tracks.t1.status==0)
	{
		pos1=strstr(Tracks.t1.data,"^");
		if(pos1==NULL)
		{
			LOGF_TRACE("SIGNO NO ENCONTRADO");
			strcpy(inputbuff,Tracks.t1.data+1);
			LOGAPI_HEXDUMP_TRACE("inputbuff ",inputbuff,25);			
			
		}
		else
		{

		
			LOGAPI_HEXDUMP_TRACE("pos-> ",pos1,2);
			strcpy(tmpbuff,pos1+1);	
			LOGAPI_HEXDUMP_TRACE("tmpbuff-> ",tmpbuff,sizeof(tmpbuff));
			pos1=strstr(tmpbuff,"^");
			if(pos1==NULL)
			{
				LOGF_TRACE("SIGNO NO ENCONTRADO");
				
			}
			else
			{
				*pos1=0;
			}
			LOGAPI_HEXDUMP_TRACE("pos-> ",pos1,3);
			strcpy(inputbuff,tmpbuff);
			LOGAPI_HEXDUMP_TRACE("inputbuff-> ",inputbuff,25);
			
		}
		
		in_val=strlen(inputbuff);
	}
	else
	{
		LOGF_TRACE("INVALID READ");
		in_val=INVALID_MSR_DATA;

	}
	return in_val;

}


int getMSR_discdata(char* rqData, char* outBuff,int* szbuff)  // FAG 02 FEB 2018

{
	//unsigned char iResult = 0;
	int iResult = 0;
	*szbuff = 0;

	if(!memcmp("DiscData",rqData,8) || !memcmp("discdata",rqData,8))
		{
			 if(Datas.t2.valid == 1)
			{					
				strcpy(outBuff,Datas.t2.disc_data);
				*szbuff = strlen(Datas.t2.disc_data);
				iResult = VALID_MSR_DATA;
			}
			else
				iResult = INVALID_MSR_DATA;
		}
	
	LOGF_TRACE(" iResult %i !!",iResult);	
	return iResult;	
	
}




#if 0
//SET REGION PATH
int SetRegionPathEMV(char* region)
{	
	int result = 0;
	
	LOGF_TRACE("\n SetRegionPath");
	result = uiSetCurrentRegionPath(region);	
	
	return result;
}

//GET REGION PATH
int GetRegionPathEMV()
{
	int result = 0;
	char regionCurr[3] = {0};
	//LOGF_TRACE("\n--GetRegionPathEMV--");

	//strcpy(regionCurr,uiCurrentRegionPath().c_str());
	LOGF_TRACE("nolha");
	LOGF_TRACE("nolha");
	result = atoi(regionCurr);
	LOGF_TRACE("\nresult = %i",result);
	
	return 1;
}


#endif

void vdSetPinMsgs(char *sMsgHeader, char *sMsgBody)
{
	uiPinParams["HEADER_MSG"] = string(sMsgHeader); 
	uiPinParams["BODY_MSG"] = string(sMsgBody); 
}

int IsSelectionTimeout(void)
{
	return (uiSelectionTimeout == -3);
}


int _getEnvFile_(char *chSection,char *chVar, char *chValue, int inLen )
{
	int inRet = 0;
	
#ifdef _VRXEVO
	 inRet = get_env(chVar, chValue, inLen);		
#else
	inRet = getEnvFile((char *)chSection, (char *)chVar, chValue,inLen);
#endif	
	return inRet;
}

int _putEnvFile_(char *chSection,char *chVar, char *chValue)
{
	int inRet = 0;
	
#ifdef _VRXEVO
	
	inRet = put_env(chVar, chValue, strlen(chValue));	 
#else
	inRet = putEnvFile((char *)chSection,(char *)chVar,chValue);
#endif	
	return inRet;
}



