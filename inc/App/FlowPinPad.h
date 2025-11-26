#ifndef _FLOWBNMX_H_
#define _FLOWBNMX_H_

//SYSTEM INCLUDES
#include <string>
#include <stdio.h>
#include <malloc.h>
#include <cstring>

#include "App/encryp_aes.h"

#define MAX_CMD_LEN                        4
#define EOT_CMD                               -4 
#define ACK_CMD_OK                          0
#define FALLA_CHIP                          -10
#define CARD_REMOVED                  -23
#define FALLA_CARGA_LLAVE_UNI  -30
#define FAILURE_LOAD_KEY	       -82
#define UNI_ADQUIRIENTE                   0
#define MULTI_ADQUIRIENTE               1
#define SHOW_ALERT			          1
#define SHOW_AD  			          2

#define TARJ_BLOQUEADA					-101
#define EMV_ABORT						-102



//VARIABLES
//To detect the type of input
//CHIP, MSR, KBD OR CTLS
extern unsigned char chTechnology;
extern unsigned char count_fallback;
extern unsigned char flagC51;
extern unsigned char flagZ10;
extern unsigned char flagZ4;
extern               int acqmul; // FAG 03-ABRIL-2017

extern bool boCtlsON;
extern bool isMacroCmdUni;
extern bool boDUKPTasm;

//PROTOTIPOS
void vdCmdFlow(void);
int inGetCmdBBVA(char* chCmd,  char* chDataCmd, int *inLenDataCmd);
int inDoCmdBBVA(char *chCmdBBV, char *chOutCmdData);

int inDoY10BBVA(char *chOutCmdData);  //FAG 08-NOV-2016
int inDoY11BBVA(char *chOutCmdData); // FAG 08-NOV-2016
int inDoC14BBVA(char *chOutCmdData);
int inDoCA6Bnmx(char *chOutCmdData);
int inDoCA8Bnmx(char *chOutCmdData);
int inDoC51BBVA(char *chOutCmdData);
int inDoZ4BBVA(char *chOutCmdData);
int inDoC54BBVA(char *chOutCmdData);
int inDoC25BBVA(char *chOutCmdData);
int inDoC12BBVA(char *chOutCmdData);
int inDoZ3BBVA(char *chOutCmdData);
int inDoZ2BBVA(char *chOutCmdData);
int inDoC23BBVA(char *chOutCmdData);
int inDoI02BBVA(char *chOutCmdData);

int C54_EMV();
int C54_CTLS();


int inWaitAck(char *chMsOrig,int inMsOrig, int inTry);


// jbf 20171207
void count_fallback_zero(void);
void count_fallback_inc(void);
int count_fallback_get(void);
void reloadMacroComandoUni (char *Buff);



//2 PROTOTIPOS DE COMANDOS BANORTE UNIADQUIRENCIA

//3  FAG 07-NOV-2016



int inDoC30Uni(char *chOutCmdData);
int inDoC31Uni(char *chOutCmdData);
int inDoC34Uni(char *chOutCmdData);
int inDoQ8Uni(char *chOutCmdData);
int inDoZ10Uni(char *chOutCmdData);
int inDoZ11Uni(char *chOutCmdData);
int in_c14_dsm (void);  // FAG 07-NOV-2016  
int C34_EMV(void);// FAG 20-DIC-2016
int inDsmZ11( LdkDat *Data ); // FAG 30-ENE-2017
void status_key();	// FAG 13-FEB-2017
void show_error(int in_val);

//NRC 7-XI-18
bool isSECommand();
int initTransaction51(char *chOutCmdData);
int requestTransportKeyZ10(char *chOutCmdData);
int injectKeyZ11(char *chOutCmdData);
int endTransaction54(char *chOutCmdData);
int exceptionBINList14(char *chOutCmdData);
int parse14Command(char receivedCommand[], char receivedBuffer[], int bufferLength);
int processHostAuthForEMV();
int processHostAuthForCTLS();

int updateDate(void);

#endif  /* _FLOWBNMX_H_ */
