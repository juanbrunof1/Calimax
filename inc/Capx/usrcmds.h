//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
// AJM 07/02/2010 1
#ifndef USRCMDS_H_
#define USRCMDS_H_



#include <stdlib.h>
#include <string.h>
#include "dukpt_defs.h"

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif



#include <stdlib.h>
#include <string.h>
//Se comenta por homologacion JVelazquez 07/08/2014
//#include <vxemvap.h>
//#include <ACLStr.h>
//#include <ACLStr.h>
//#include <Acldev.h>
//#include <Aclconio.h>
//#include <Applidl.h>
//#include <svc_sec.h>
//#include "../include/pdebug.h"
//#include "crpdukrsa/dkprsa.h"
//Este archivo hace referencia a las variables que se agregan por homologacion
//Agrego JVelazquez 07/08/14
#include "dukpt_defs.h"


#define BNMX_MODE 1
#define BBVA_MODE 2

//EXTERN AUXILIAR
DllSPEC extern int inBanco ;
DllSPEC extern int entry_mode;

int in_remove(char pchRmv[]);
int in_file_copy (const char pchOld[], const char pchNew[]);		// AJM 27/01/2011 1
int in_open (const char pchOpn[], int inMod);		// AJM 23/01/2011 1
long ln_lseek (int inHdl, long lnChr, int inOrg);		// AJM 09/02/2011 1
int in_read (int inHdl, char pchChr[], int inChr);
int in_write (int inHdl, char pchChr[], int inChr);
int in_delete (int inHdl, int inChr);		// AJM 21/01/2011 1
int in_close (int inHdl);
DllSPEC int inHexToAsc(char pchHex[], char pchAsc[], int inHex);
DllSPEC int inAscToHex(char pchAsc[], char pchHex[], int inAsc);
int in_crc_src (void);		// AJM 21/01/2011 1
DllSPEC unsigned long ul_crc_32 (char pch_crc [], int in_crc, unsigned long ul_sed);		// AJM 14/02/2011 1
DllSPEC int inGetEnvVar(char pchVar[], char pchFil[]);
DllSPEC int inSetEnvVarin(char pchVar[], int inVal, char pchFil[]);
DllSPEC char *pchGetAppVer(char pchAppVer[]);
DllSPEC char *pchGetSrlNmb(char pchSrlNmb[]);
//Esta funcion se encarga de validar que la informacion del Track sea correcta.
//Ya que para las terminales Mx estas validaciones son diferentes se modififcara esta funcion
//Modifico JVelazquez 07/08/14 (Queda pendiente, solo se cambia para la compilacion)
//int inCrdDsm( void );
DllSPEC extern int inCrdDsm(struct TRACK *psrTrk, char chCrd[], int inCrd, const char szWch[]);
DllSPEC int cryp_msr(char* trk1, char* trk2,char* CVV, bool isFllbck, bool isCtls, int iflgCVV);

DllSPEC void vdSetBanco(int inBank);
int  intGetBanco(void);
DllSPEC void vdsetflagBIN(int inFlag);
DllSPEC int inGetflagBIN(void);


#define S_TRTRK2	37				// MAX DATA ELEMENTS IN TRK2
#define S_TRTRK1	76				// MAX DATA ELEMENTS IN TRK1
#define S_TREXPD 	2				// DATA ELEMENTS FOR EXPORATION DATE

#define CRD_CHP		05
#define CRD_SWP		90
#define CRD_FBK		80
#define CRD_KBD		01


// jbf 20241102
#define CVV_CAPTURED 0
#define CVV_NOTASKED 1
#define CVV_EMVTYPE  2


typedef struct
{
	char chAct[30];
	char chAcH[15];
	char chExp[10];
	char chSvC[10];
	char chHld[50];
	char chTk2[50];
	char chTk1[150];
	char chCv2[10];
	char chMod[5];
	char chRaw[300];		// AJM 29/08/2011 1
	char chErC[3];		// AJM 06/09/2011 1

	int inAct;
	int inAcH;
	int inExp;
	int inSvC;
	int inHld;
	int inTk2;
	int inTk1;
	int inCv2;
	int inMod;
	int inRaw;		// AJM 29/08/2011 1
	int inErC;		// AJM 06/09/2011 1
}
CrdDsm;

int inCrdRawDsm(CrdDsm *psrCrdDsm, char pchRaw[], int inRaw, char pchMod[]);

DllSPEC int inCrpSmlMax (void);		// AJM 04/01/2011 1
unsigned long ulCrpCnt (void);		// AJM 06/01/2011 1

// AJM 23/01/2011 1
int in_SVC_VALID_DATE (char pchVld[]);
// AJM 23/01/2011 1

// AJM 19/10/2011 1
int inCrdExpSwc (char p_chTk2 [], int inTk2, char p_chMod [], int inMod);
// AJM 19/10/2011 1
int inKeyAbmLoad(void);		//Daee 11/12/2014

#define SAV_ERR_BSE		-10
#define SAV_ERR_SRL		-11
#define SAV_ERR_HND		-12
#define SAV_ERR_SIZ		-13
#define SAV_ERR_CRC		-14
#define SAV_ERR_SGN		-15
#define SAV_ERR_CRT		-16

DllSPEC int in_cpx_ipk_opn (void);
DllSPEC int in_cpx_ipk_ktk (char pch_qs [], int * pin_qs, char pch_qw [], int * pin_qw);
DllSPEC int in_cpx_ipk_sav (char pch_qx [], int in_qx);
DllSPEC int in_cpx_ipk_end (void);

DllSPEC int in_cpx_crd_opn (void);
DllSPEC int in_cpx_crd_chp (char pch_tg57 [], int in_tg57, char pch_5f20 [], int in_5f20, bool isCtls);
DllSPEC int in_cpx_crd_mag (char pchRaw[], int inRaw, int bo_fbk, char pchCv2[], int inCv2, bool isCtls);
DllSPEC int in_cpx_crd_kbd (char pch_act [], int in_act, char pch_exd [], int in_exd, char pch_cv2 [], int in_cv2);
DllSPEC int in_cpx_crd_cry (char pch_qs [], int * pin_qs, char pch_qz [], int * pin_qz, char pch_qy [], int * pin_qy);
DllSPEC int in_cpx_crd_end (int in_ath, int bo_ery);

DllSPEC int in_cpx_crd_exc (char pch_qt [], int in_qt);

DllSPEC int inAskBinTbl(char *chBinId,char *chBinVer, char *chBinRange); //JRS 2016

#endif

//Migrado por JVelazquez 22/08/14
#endif //MAKE_DUKPT
