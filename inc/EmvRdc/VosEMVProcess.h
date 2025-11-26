#ifndef _VOS_EMVPROCESS_H_
#define _VOS_EMVPROCESS_H_


#if (defined _VRXEVO)
  #if (defined EMV_IMPORT)
  #define DllSPEC __declspec(dllimport)
  #else
  #define DllSPEC   __declspec(dllexport)
  #endif
#else
  #define DllSPEC
#endif


#include <emv/EMV_CT_Interface.h> //Library of ADK
#include <emv/EMV_CTLS_Interface.h> //Library of ADK
#include <emv/EMV_Common_Interface.h>
#include <emv/E2E_EMV_CT_Serialize.h>
#include <emv/E2E_EMV_CTLS_Serialize.h>
#include <emv/btlv.h>
#include <tec/tec.h>
#include <string>
#include "msr/msr.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#define GENERALTIMEOUT 12
#define ENTER_PIN_MESSAGE "Introduce PIN"
#define RETAP_CARD_MESSAGE "Vuelva a intentar"
#define UI_STR_SELECT_APP "SELECCIONE"

#define BEEPVOLUME 90
#define REMOVE_CARD_MESSAGE "Retire su tarjeta por favor"
#define THANKS				"Gracias"

//Status after waiting an event
#define EMV_IDL_ANY_CARD  100
#define EMV_IDL_MANUAL    101
#define EMV_IDL_CANCEL   -1
#define EMV_INV_EXPDATE -5
#define EMV_IDL_TIME_OUT -6
#define EMV_EXPIRED_DATE -7


//Define to magnetic stripe reader
#define VALID_MSR_DATA  	    0	
#define READ_ST_MSR_DATA      1
#define VALID_MSR_NOT_ST_DAT  2
#define DATA_NOT_FOUND      -14
#define INVALID_MSR_DATA    -15
#define INSUFFICIENT_MSR_DATA	-16


//Define to set a value when the input is from keyboard
#define CTS_KBD				5

// jbf
//Define to set a value when <CANCEL> is pressed
#define EXIT_BY_CANCEL			-8

//Define to verify the type of CTLS Card
#define CTLS_TXN_CHIP  1
#define CTLS_TXN_MSR   2
#define CTLS_ERROR     3

typedef struct TrxResult{
  EMV_CT_TRANSRES_TYPE xEMVTransRes;
  unsigned char eEMVInfo;
}EMV_TRX_CT_LOG_REC;


typedef unsigned char (*guiCallbackT)(void);

//Daee 07/12/2017
typedef std::string (*DisplayGetText_CB)(int error_id);


typedef struct {      //Struct to params to ask aPIN
  unsigned char  ucPinAlgo;
  unsigned char  ucAutoEnter;
  unsigned char  ucClearAll;
  unsigned long  ulEchoChar;
  unsigned long  ulBypassKey;
  long           lTimeoutMs;
  guiCallbackT   callback;
  char           *amount;
  char           *currency;
} guiPinParam;

//JRS 
#define EMV_CUST_READ_CARD_IN_PROGRESS 217

//error code for emv init
#define EMV_INIT_ERR_NONE             0x00
#define EMV_INIT_ERR_GET_MEM          0x01
#define EMV_INIT_ERR_INIT_KERNEL      0x02
#define EMV_INIT_ERR_INIT_PP_COMM_BUF 0x03
#define EMV_INIT_ERR_PP_IDENT         0x04
#define EMV_INIT_ERR_SET_TERM_DATA    0x05
#define EMV_INIT_ERR_SET_APPLI_DATA   0x06
#define EMV_INIT_ERR_SET_CAP_KEY      0x07
#define EMV_INIT_ERR_APPLY_CFG        0x08


//Declara estructuras necesarias para contacto
DllSPEC extern EMV_CT_SELECTRES_TYPE xSelectResCT; // result of the contact selection process
DllSPEC extern EMV_CT_TRANSAC_TYPE AdditionalTxDataCT;
DllSPEC extern EMV_CT_SELECT_TYPE TxDataCT; //CT Data. Parameters for start transaction (application selection processing)
DllSPEC extern EMV_CT_TRANSRES_TYPE xEMVTransResCT;
DllSPEC extern EMV_CT_HOST_TYPE xOnlineInputCT;
DllSPEC extern EMV_TRX_CT_LOG_REC  pxTransRes;

//Declara estructuras necesarias para contactactless
DllSPEC extern EMV_CTLS_START_TYPE TxStartInputCTLS; //CTLS Data
DllSPEC extern EMV_CTLS_STARTRES_TYPE xSelectResCTLS;
DllSPEC extern EMV_CTLS_TRANSRES_TYPE CTLSTransRes;
DllSPEC extern EMV_CTLS_HOST_TYPE xOnlineInputCTLS;
DllSPEC extern MSR_DecodedData Datas;
DllSPEC extern unsigned char EMV_Region;

DllSPEC extern MSR_TrackData2 Tracks;
DllSPEC extern MSR_DecodedData Datas;


//********************************************************************************************************************
//INIT_EMV_CLTS
//Function to Initialize EMV Kernel to CTLS
//********************************************************************************************************************
DllSPEC unsigned char Init_EMV_CTLS(unsigned char terminalType);  //Function to initialize EMV Kernel to CTLS

//********************************************************************************************************************
//ApplyCTLSConfiguration
//Function to transfer the stored configuration to reader. Only functional in context of VFI reader. 
//********************************************************************************************************************
DllSPEC EMV_ADK_INFO  ApplyCTLSConfiguration();

//********************************************************************************************************************
//vdSetCTLSTransactionData
//Function to transfer the set the basic data before of transaction
//********************************************************************************************************************
DllSPEC void vdSetCTLSTransactionData(EMV_CTLS_START_TYPE *xSelectInput, char *szAmount);

//********************************************************************************************************************
//ActivateTransactionCTLS
//Function to Set up a CTLS EMV transaction. 
//********************************************************************************************************************
DllSPEC unsigned char ActivateTransactionCTLS(EMV_CTLS_START_TYPE* TxStartInputCTLS,EMV_CTLS_STARTRES_TYPE* xSelectResCTLS);

//********************************************************************************************************************
//CTLS_FirstGenerateAC
//Function to perform the CTLS_FirstGenerateAC
//********************************************************************************************************************
DllSPEC unsigned char CTLS_FirstGenerateAC(EMV_CTLS_TRANSRES_TYPE* CTLSTransRes);

//********************************************************************************************************************
//CTLS_END_transaction
//Function to end the CTLS Transaction
//********************************************************************************************************************
DllSPEC unsigned char CTLS_END_transaction(EMV_CTLS_HOST_TYPE *xOnlineInput, EMV_CTLS_TRANSRES_TYPE *xTransRes);

//********************************************************************************************************************
//Info_TRX_CTLS_EMV
//Function to show information about transaction
//********************************************************************************************************************
DllSPEC void Info_TRX_CTLS_EMV();

//********************************************************************************************************************
//EMV_EndTransactionCTLS
//Function to disable the reader
//********************************************************************************************************************
DllSPEC void EMV_EndTransactionCTLS(const char *szStatusMessage, unsigned char ucReader, int inExitWithError);

//********************************************************************************************************************
//Check_typ_CTLS
//Function to verify if we neither have  a CTLS CHIP card or have CTLS MSR card
//********************************************************************************************************************
DllSPEC int Check_typ_CTLS();

//********************************************************************************************************************
//INIT_EMV
//Function to Initialize EMV Kernel
//********************************************************************************************************************
DllSPEC unsigned char Init_EMV(unsigned char terminalType); //Function to initialize EMV Kernel

//********************************************************************************************************************
//EMV_AppSelection
//Function to Initialize EMV Kernel
//********************************************************************************************************************

DllSPEC unsigned char EMV_AppSelection(EMV_CT_SELECT_TYPE* xterminaldata,EMV_CT_SELECTRES_TYPE* xSelectRes);

//********************************************************************************************************************
//Detect_input
//Function to Wait an event (swipe, inserted card, swiped card)
//********************************************************************************************************************

DllSPEC int Detect_input(unsigned char supportTech, unsigned char* chTechnology, int inFlagManual,int inMin, int inMax,char *chManual,int inTimeOut,
        unsigned char *dataBuffer,unsigned short *dataBufferLength,char *chHeader,char *chMsgLine,char *chMsgLine2);

//*******************************************************************************************************************
//Set_CT_TransactionData
//Continue to provide the remaining transaction data (this is the reason why the same "substructure" is included StartTransaction and the ContinueOffline)
//********************************************************************************************************************

DllSPEC void Set_CT_TransactionData(EMV_CT_SELECT_TYPE* xEMVStartData, char * szAmount,char* cashback,unsigned char txntype);

//********************************************************************************************************************
//EMV_Setup_TXN_CT_Data
//Function to Set the mandatory tags before the App Selection
//********************************************************************************************************************

DllSPEC void EMV_Setup_TXN_CT_Data(EMV_CT_TRANSAC_TYPE* xEMVTransType, unsigned char* amount, unsigned char* ucCashbackAmount, EMV_CT_SELECT_TYPE* xEMVStartData);

//********************************************************************************************************************
//EMV_ReadRecord
//Function to Read Records
//********************************************************************************************************************

//unsigned char EMV_ReadRecords(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_ReadRecords(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* pxTransRes, EMV_CT_TRANSAC_TYPE* pxTransactionInput);

//********************************************************************************************************************
//EMV_DataAuthentication
//Function to do the Data Authentication
//********************************************************************************************************************

//unsigned char EMV_DataAuthentication(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_DataAuthentication(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec,EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//EMV_CardholderVerification
//Function to perform the cardholder verification
//********************************************************************************************************************

//unsigned char EMV_CardHolderVerification(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_CardHolderVerification(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//EMV_CardHolderVerificationEnd
//Function to verify the cardholder Verification
//********************************************************************************************************************

//unsigned char EMV_CardHoldVerificationProcess(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_CardHoldVerificationProcess(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//EMV_RiskManagement
//Function to perform Terminal Risk Management
//********************************************************************************************************************

//unsigned char EMV_RiskManagement(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_RiskManagement(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//EMV_FirstGenerateAC
//Function to perform First Generate AC
//********************************************************************************************************************

//unsigned char EMV_FirstGenerateAC(EMV_CT_TRANSAC_TYPE*  pxTransactionInput, EMV_TRX_CT_LOG_REC*  pxTransRes);
DllSPEC unsigned char EMV_FirstGenerateAC(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//EMV_SecondGenerateAC
//Function to perform Second Generate AC
//********************************************************************************************************************

DllSPEC unsigned char EMV_SecondGenerateAC(EMV_CT_HOST_TYPE *pxOnlineInput, EMV_CT_TRANSRES_TYPE *pxTransRes);

//********************************************************************************************************************
//EMV_steps
//Function that determines the next step in the EMV flow
//********************************************************************************************************************

DllSPEC unsigned char EMV_steps(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

//********************************************************************************************************************
//_CHECK_INSERTED_CARD
//Function to CHECK IF A CARD IS INSERTED
//********************************************************************************************************************

DllSPEC unsigned char _CHECK_INSERTED_CARD(void);

//********************************************************************************************************************
//getCT_EMV_TAG
//Function to obtain any tag for Contact or CTLS
//********************************************************************************************************************

DllSPEC unsigned char getCT_EMV_TAG(unsigned long  options,unsigned long *  requestedTags,unsigned short  noOfRequestedTags,unsigned char *  tlvBuffer,unsigned short  bufferLength, unsigned short *	tlvDataLength,int tech, bool boOnlyVal=false);

//********************************************************************************************************************
//Reult_TRX
//Function to obtain some information about contact transaction
//********************************************************************************************************************

DllSPEC void Info_TRX_CT_EMV();

#if 0
//********************************************************************************************************************
//dataforPIN
//Function to fill the structure to sent to the function EMV_CT_Enter_PIN_extended() in ENTER PIN() function
//********************************************************************************************************************

static void dataforPIN(EMV_CT_ENTER_PIN_STRUCT* dataPin);


//********************************************************************************************************************
//ENTER_PIN
//Function to ask a PIN
//********************************************************************************************************************
unsigned char ENTER_PIN(unsigned char pintype, unsigned char bypass,
            unsigned char* pucUnpredNb, unsigned char* pucPINResultData,
                        unsigned short KeyLength, unsigned char* pucPINPublicKey,
                        unsigned long ulPINPublicKeyExp);

//********************************************************************************************************************
//PIN_OFFLINE
//TO send PIN to Kernel
//********************************************************************************************************************

unsigned char PIN_OFFLINE(EMV_CT_ENTER_PIN_STRUCT* dataPin,unsigned char* pucPINResult);

//********************************************************************************************************************
//CallbackInit_OutTLV and emvCallback
//To dump EMV TAGS
//********************************************************************************************************************

void CallbackInit_OutTLV(unsigned char *pucReceive, unsigned short *psReceiveSize);

void emvCallback(unsigned char *pucSend, unsigned short sSendSize, unsigned char *pucReceive, unsigned short *psReceiveSize, void* externalData);

#endif

//********************************************************************************************************************
//dumpEMV
//To dump EMV TAGS
//********************************************************************************************************************

DllSPEC void dumpEMV(void* input,int size,const char* name);

//********************************************************************************************************************
//EMV_EndTransactionCT
//To End the transaction
//********************************************************************************************************************

DllSPEC void EMV_EndTransactionCT(const char *szStatusMessage, unsigned char ucReader, int inExitWithError);

//********************************************************************************************************************
//Remove_Card
//To ask user to remove card when the transaction has ended
//********************************************************************************************************************

DllSPEC void Remove_Card();

//********************************************************************************************************************
//inGUIInputAction
//To Enter any action
//********************************************************************************************************************

DllSPEC int inGUIInputAction(int inOption, const char *szTitle, char *szAux, char *szResult, int is_Aproved);

//********************************************************************************************************************
//Enable_swp
//To Activate the Magnetic stripe reader
//********************************************************************************************************************

DllSPEC int Enable_swp();

//********************************************************************************************************************
//Disable_swp
//To Disable the Magnetic stripe reader
//********************************************************************************************************************

DllSPEC int Disable_swp();

//********************************************************************************************************************
//Read_MSRdata
//To read Magnetic stripe data
//********************************************************************************************************************

DllSPEC int Read_MSRdata();

//********************************************************************************************************************
//getMSR_data
//To get any Magnetic stripe datum
//********************************************************************************************************************

//unsigned char getMSR_data(char* rqData, char* outBuff,int* szbuff);
DllSPEC int getMSR_data(char* rqData, char* outBuff,int* szbuff);

//********************************************************************************************************************
//screenExpDate
//To set the Expiration Date
//********************************************************************************************************************
//int screenExpDate(char* Exp_Date);
DllSPEC int screenExpDate(char* Exp_Date,char *chMsgHeader, char *chMsgBody);

//********************************************************************************************************************
//screenExpDate
//To set the Security Code
//********************************************************************************************************************

DllSPEC int screenSec_Code(char* SECCODE,int maxlentgh,char *chMsgHeader,char *chMsgBody);

DllSPEC unsigned char UIEnterPin(guiPinParam *pinParam); // prueba

extern char gszCurrecyCode[3+1]; // FAG 25-01-17

DllSPEC bool guiCallback(void *data);  // FAG 25-01-17

DllSPEC void getversionHW(char * output); // FAG 28-07-17

DllSPEC void vdSetPinMsgs(char *sMsgHeader, char *sMsgBody);

//Daee 07/12/2017
DllSPEC void setDisplayGetText_CB(DisplayGetText_CB cb);

/*
unsigned char _EMVADK_cbk_duringPINinput(void);

unsigned char ucInputPIN( unsigned char pintype, unsigned char bypass, unsigned char* pucPINResultData);

*/

//********************************************************************************************************************
//vdGetDate
//To get Date Terminal
//********************************************************************************************************************
DllSPEC void vdGetDate(unsigned char *buf, size_t bufLen);


#if 0

//********************************************************************************************************************
//SetRegionPathEMV
//To set the desired Region Path
//********************************************************************************************************************
int SetRegionPathEMV(char* region);

//********************************************************************************************************************
//GetRegionPathEMV
//To Get the desired Region Path
//********************************************************************************************************************
int GetRegionPathEMV();

#endif

#endif /* _VOS_EMVPROCESS_H_ */
