#ifndef _TRANS_H_
#define _TRANS_H_

//Type of transaction
#define CHIP   1
#define MSR	 2
#define KBD    3
#define CTLS   4

//Status about transaction
#define SUCCESS_TRX  	 0
#define ERROR_INP_MODE	-99

//Define to verify if there is inserted an AMEX Card 
#define AMEX_CARD	    1
#define NO_AMEX_CARD    0

//Status about parse of token B5 y B6
#define DSM_OK			0
#define DSM_FAIL	   -1

//Para obtener tag C2
#define Apl_ErC_ESC 1
#define Apl_ErC_ERR 0

//Para saber que token parsear si E1 o E2 
#define C53		1
#define C54 		2
#define C33		3
#define C34 		4

//  byte correspondiente al modo  manual
#define KBD_ACT		0x80
#define KBD_EXP		0x40
#define KBD_CV2		0x20
#define KBD_COR        0x10

// comando
#define cmd_C30		0
#define cmd_C31          1


// tipo de comando ingreso manual
#define KBD_C30	  10
#define KBD_C51	  11	

//case error
#define FAILURE_CVV			-84
#define FAILURE_LUHN     	-85
#define USE_CHIP			-40 	// FAG 07-SEP-2017




int MSR_Transaction(int MSRead);
int KBD_Transaction(char *PAN, int szPan); // funciion  de ingreso manual para bancomer
int KBD_Transaction_banorte(char *PAN, int szPan, int type_cmd); // funcion d ingreso manual para banorte(uni,multi)
int EMV_Transaction();
int CTLS_Transaction();
int CTLS_CHIP(int erg);
int CTLS_MSR(int erg);

int tr2_ctls(int* sztrk2,char* trk2,int* sztrk1,char* trk1Asc );
int read_PAN_MSR(int MSRead,char* pan_out,int* szPAN);
int read_PAN_EMV(char* Pan);
int read_PAN_CTLS(char* PanEMV);


unsigned char EMVFLOW(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType);

                //trk1       sztrk1      trk2         sztrk2      CVV          szCVV2      MSR     
int in_txn_crd (char* buff1, int szbuff1,char* buff2, int szbuff2,char* buff3, int szbuff3, int type_inp, int flgCVV);
int crypCAPX(int tech);
int check_key();

int inAsmBnmxQ8 (char pchSnd []);
int inAsmBnmxQ9 (char Q9 [],unsigned char chTechnology);
int inAsmBnmxQE (char* QE,int tech);

int GetTag(unsigned long* inTag, unsigned char *chOut, unsigned short *uLen, int FLen, int inCond, unsigned char *bitmap, char chCh,int tech);

int Host_Response();
int build_C54();
int build_C53_C54Offline();
void asm_tke1(char* tknE1,int* szTknE1,int type_cmd);
void asmtknE2(char* TOKEN_out,int* szTOKEN_out,int type_cmd);
void tkn_cz(char* TOKEN_out,int* szTOKEN_out);
void tokR1(char* tokR1, int* szTkR1);

void asmTknE2C54(char* tknE2,int* szTknE2);

int identify_AMEX_card(int tech);


//2 TRANSACCION PARA UNIADQUIRENCIA

int MSR_Transaction_UNI(int MSRead);
int EMV_Transaction_UNI();
int EMV_Transaction_C30_UNI();
int bo_crp_crd (char pch_pan [], int in_pan);
int build_C30_C33_CHIP(char cmd_asm);
unsigned char EMVFLOW_UNI(EMV_CT_SELECT_TYPE* xEMVStartData, EMV_CT_SELECTRES_TYPE* xSelectRes, EMV_TRX_CT_LOG_REC* xTrxRec, EMV_CT_TRANSAC_TYPE* xEMVTransType,char cmd_process);
int build_C34();
unsigned char setCT_EMV_TAG(unsigned long options, unsigned char *tlvBuffer, unsigned short bufferLength); // FAG 27-DIC-2016
void asmtkn(char* TOKEN_out,int* szTOKEN_out); // FAG 02-FEB-2017

int bo_crp_chp (void);	   // FAG 28-FEB-2017
bool display_card_info(void);
std::string maskPan(char pan[21]);
int fallback_MSR(void);
unsigned long convert_ArrayHex2int(char* hex,int szhex);
bool checkLuhn(const std::string &acctNumber);
bool check_service_code(char data_input[]);
int incheck_expdate(char *inputASC);  // FAG 18-SEP-2017
int CTLS_ON(void);		//Daee 18/11/2015

unsigned char set_EMV_TAG(unsigned long options, unsigned char *tlvBuffer, unsigned short bufferLength,int tech);


//NRC 18-X-18
int buildInitTransactionReply53();
bool isExceptionBin(char pch_pan [], int in_pan);
void assembleTokenE1(char* tknE1,int* szTknE1, int type_cmd);
void assembleTokenE2(char* TOKEN_out,int* szTOKEN_out,int type_cmd);
int buildEndTransactionReply54();
int processMSRTransaction(int MSRead,int itWasRead=0);
bool isMerchantExceptionBIN(char accountNumber[], int accountNumberLength);
bool isMerchantExceptionBINRange(char accountNumber[], int accountNumberLength);

int AddTag9F6E(char* pTokenBuffer, int inForce = 0);
void vdGetFFIMC(char *ch9F6E,char *chFFI);
int inCTQvalue (char *chT9F6C);
int inGet9F34Ctls(unsigned char pch_val [] );

#endif  /* _TRANS_H_ */
