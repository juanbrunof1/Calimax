
//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
// AJM 07/02/2010 1
#ifndef DKPRSA_H
#define DKPRSA_H

#include <stdlib.h>
#include <string.h>
//Este archivo hace referencia a las variables que se agregan por homologacion
//Agrego JVelazquez 07/08/14
#include "dukpt_defs.h"
#include "rsa.h"

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif



//Se comenta por homologacion JVelazquez 07/08/2014
//#include <ACLStr.h>
//#include <ACLStr.h>
//#include <Acldev.h>
//#include <VXEMVAP_define.h>

#define CRP_DKP_ASK_NEW_OFF		0
#define CRP_DKP_ASK_NEW_STL		1
#define CRP_DKP_ASK_NEW_NOW		2

//Se modifica para compatibilidad con la app
#define  LEFT_              0x00
#define  RIGHT_             0x80
#define  CENTER_            0x88


int inCrpDkpRun(char pchAct[], int inAct);
int inCrpSwpCrdSav(char pchCrd[], int inCrd, char pchMod[]);
int inCrpChpCrdSav(char pchTk2[], int inTk2, char pchNum[], int inNum, bool isCtls);
int inCrpKbdCrdSav(char pchNum[], int inNum, char pchExp[], int inExp);
int inCrpCrdCv2Sav(char pchCv2[], int inCv2, int boCv2);		// AJM 11/04/2011 1
int inCrpKtkRmv(void);
int inCrpCrdRmv(void);
int inAskBinTable(char *chBinId,char *chBinVer, char *chBinRange); //JRS 2016
#ifdef __arm

//#include <svc_sec.h>
#include "crpdkp.h"
#include "usrcmds.h"
//#include "../../include/pdebug.h"
//#include <rsa.h>
//#include "rsa.h"

#define HEX_KEY_SIZ			16
#define ASC_KEY_SIZ			HEX_KEY_SIZ << 1
#define HEX_KSN_SIZ			10
#define ASC_KSN_SIZ			HEX_KSN_SIZ << 1
#define HEX_CHK_SIZ			3
#define ASC_CHK_SIZ			HEX_CHK_SIZ << 1
#define HEX_CRC_SIZ			4
#define ASC_CRC_SIZ			HEX_CRC_SIZ << 1
#define HEX_VER_SIZ			5
#define ASC_VER_SIZ			HEX_VER_SIZ << 1
#define HEX_PAD_SIZ			1
#define ASC_PAD_SIZ			HEX_PAD_SIZ << 1
#define HEX_MOD_SIZ			256
#define ASC_MOD_SIZ			HEX_MOD_SIZ << 1
#define HEX_EXP_SIZ			3
#define ASC_EXP_SIZ			HEX_EXP_SIZ << 1

typedef struct
{
	// Qw
	char chChk[ASC_CHK_SIZ];
	char chVer[ASC_VER_SIZ];
	char chPad[ASC_PAD_SIZ];
	char chCrc[ASC_CRC_SIZ];
	// de62
	char chRsa[ASC_MOD_SIZ];
}
KtkRqt;

#define CRP_ERR_SIZ			2

typedef struct
{
	// Qx
	char chKsn[ASC_KSN_SIZ];
	char chChk[ASC_CHK_SIZ];
	char chErr[CRP_ERR_SIZ];
	char chCrc[ASC_CRC_SIZ];
	// de62
	char chKey[HEX_KEY_SIZ];
}
KtkRsp;

typedef struct
{
	char chKtk[HEX_KEY_SIZ];
	char chKey[HEX_KEY_SIZ];
	char chKsn[HEX_KSN_SIZ];
	KtkRqt srKtkRqt;
	KtkRsp srKtkRsp;
}
CrpKtk;

#define HEX_TK1_SIZ		80
#define HEX_TK2_SIZ		19
#define HEX_CV2_SIZ		5
//Se agregan parentesis para evitar warnings
//Agrego JVelazquez 11/08/14
#define HEX_TV2_SIZ		((HEX_TK2_SIZ) + (HEX_CV2_SIZ))
#define HEX_ASM_SIZ		(HEX_TK2_SIZ + HEX_CV2_SIZ + HEX_TK1_SIZ)
#define ASC_TK1_SIZ		HEX_TK1_SIZ << 1
#define ASC_TK2_SIZ		HEX_TK2_SIZ << 1
#define ASC_TK3_SIZ		110
#define ASC_CV2_SIZ		HEX_CV2_SIZ << 1
#define ASC_TV2_SIZ		HEX_TV2_SIZ << 1
#define ASC_ASM_SIZ		HEX_ASM_SIZ << 1
#define ASC_LT4_SIZ		4
#define CRD_MOD_SIZ		2

typedef struct
{
	int  inTk1;
	char chTk1[ASC_TK1_SIZ];
	char chCrc[ASC_CRC_SIZ];
}
CrpTk1;		// Qy

typedef struct
{
	char chKsn[ASC_KSN_SIZ];
	long lnNmb;
	int  inErr;
	VS_BOOL fTk2;
	char chMod[CRD_MOD_SIZ];
	int  inTk2;
	VS_BOOL fCv2;
	int  inCv2;
	VS_BOOL fTk1;
	char chTk2[ASC_TV2_SIZ];
	char chLt4[ASC_LT4_SIZ];
	char chCrc[ASC_CRC_SIZ];
}
CrpTk2;		// Qz

typedef struct
{
	CrpTk1 srCrpTk1;
	CrpTk2 srCrpTk2;
	int inTk3;
	char chTk1[ASC_TK1_SIZ];
	char chTk2[ASC_TK2_SIZ];
	char chTk3[ASC_TK3_SIZ];
	char chCv2[ASC_CV2_SIZ];
	char chAsm[HEX_ASM_SIZ];
	char chDsm[HEX_ASM_SIZ];
}
CrpCrd;

#define CRD_EXC_NAM		8
#define CRD_EXC_VER		2
#define CRD_EXC_LEN		4
#define CRD_EXC_CRD		320

typedef struct
{
	// Qt
	char chNam[CRD_EXC_NAM];
	char chVer[CRD_EXC_VER];
	char chKsA[ASC_KSN_SIZ];
	char chUse[CRD_EXC_LEN];
	char chSrc[CRD_EXC_LEN+1];
	char chCrp[CRD_EXC_CRD];
	char chCrc[ASC_CRC_SIZ];

	// crd
	char chCrd[CRD_EXC_CRD];

	// key + ksn
	char chKey[HEX_KEY_SIZ];
	char chKsn[HEX_KSN_SIZ];
}
CrdExc;

extern int append_char (char *string, char c);
DllSPEC extern int pad(char *pdest_buf, char *psrc_buf, char pad_char, int pad_size, int align);
int inCrpLod(char pchFil[], void *pvdDst, int inChr);
int inCrpSav(char pchFil[], void *pvdDst, int inChr);
int inCrpDkpSndRcv(void);
int inCrpAsmBgn(void);
int inCrpAsmEnd(void);
int inCrpKtkAsm (void);
int inCrpKtkDsm (void);
int in_ipk_sav (void);
int inCrpKtkRnd(char pchKtk[], int inKtk);
int inCrpRsaLod(char pchRsa[], KtkRqt *psrVer, rsa_context *psrRsa, int bo_fek);
int inCrpKtkRsa(KtkRqt *psrRsa, char pchKtk[], int inKtk);
int inCrpChkVal(char pchKey[], int inKey, char pchChk[], int inChk);
int inKtkRspShwXlt(char pchRsp[], int inRsp);
int inCrpAskNew(void);
int inDkpRspShwXlt(char pchRsp[], int inRsp);
DllSPEC int inHexToAsc(char pchHex[], char pchAsc[], int inHex);
DllSPEC int inAscToHex(char pchAsc[], char pchHex[], int inAsc);
DllSPEC int inGetEnvVar(char pchVar[], char pchFil[]);
DllSPEC long lnGetEnvVar(char pchVar[], char pchFil[]);		// AJM 04/01/2011 1
DllSPEC int inSetEnvVarin(char pchVar[], int inVal, char pchFil[]);
int inSetEnvVarln(char pchVar[], long lnVal, char pchFil[]);		// AJM 04/01/2011 1
int inDspMsg(short shMsgID, short shCol, short shRow, VS_BOOL fBeep, long lnWait, VS_BOOL fClr);
DllSPEC char *pchGetAppVer(char pchAppVer[]);
DllSPEC char *pchGetSrlNmb(char pchSrlNmb[]);
//Esta funcion se comenta dado que no aplica para las Mx9xx
//Comento JVelazquez 07/08/14
//int inCrdDsm(struct TRACK *psrTrk, char chCrd[], int inCrd, const char szWch[]);
int inDsmQx(char pchQx[], int in_Qx);
int inDsmQt(char pchQt[], int in_Qt);
DllSPEC int inAsmQs(char pchQs[], VS_BOOL fDukAsk);
int inAsmQw(char pchQw[]);
int inAsmQz(char pchQz[]);
int inAsmQy(char pchQy[]);
long ln_dir_get_file_sz(char pchFil[]);
int inCrpExcDsm(void);
int inCrpExcSav(void);

#define HRD_COD_RSP

#define CRD_CHP		05
#define CRD_SWP		90
#define CRD_FBK		80
#define CRD_KBD		01

#define EXC_CRD_SIZ		15

typedef struct
{
	char chMin[EXC_CRD_SIZ];
	char chMax[EXC_CRD_SIZ];

	int inMin;
	int inMax;
}
ExcCrd;

int inCrpSavExc(char pchCrd[], int inCrd);
DllSPEC int inCrpExcCrd(char pchAct[], int inAct);

int inNewRsaBgn(int inChr);
int inNewRsaWrt(char pchChr[], int inChr);
int inNewRsaEnd(void);
int inRsaSgnSav(void);

#define SHA_256_SIZ		20

int inRsaSgnLod (char pchVer[], int *pinVer, char pchRsa[], int *pinRsa, char pchSgn[], int *pinSgn);
int inRsaVldSgn (char pchRSA [], char pchVld[], int inVld, char pchSgn[], int inSgn);		// AJM 09/02/2011 1

typedef struct
{
	char chMod[ASC_MOD_SIZ];
	char chExp[ASC_EXP_SIZ];
	char chVer[ASC_VER_SIZ];
	char chPad[ASC_PAD_SIZ];

	int inMod;
	int inExp;
	int inVer;
	int inPad;
}
CrpRsa;

int inCrpRsaSav (char pchVer[], int inVer, char pchRsa[], int inRsa);
int inCrpRsaDsm (CrpRsa *psrCrpRsa, char pchRsa[], int inRsa);
int inCrpRsaAsm (CrpRsa *psrCrpRsa, char pchVer[], int inVer);
int inCrpRsaWrt (CrpRsa *psrCrpRsa);
int inRsaLodNam (char pchNam[]);

#define RSA_BIN_SIZ		6
#define RSA_MRC_SIZ		8
#define RSA_VER_SIZ		2
#define RSA_KEY_SIZ		538
#define RSA_NAM_SIZ		8
#define RSA_SGN_SIZ		512

typedef struct
{
	char chBin[RSA_BIN_SIZ];
	char chMrc[RSA_MRC_SIZ];
	char chVer[RSA_VER_SIZ];
	char chKey[RSA_KEY_SIZ];
	char chNam[RSA_NAM_SIZ];
	char chSgn[RSA_SGN_SIZ];

	int inBin;
	int inMrc;
	int inVer;
	int inKey;
	int inNam;
	int inSgn;
}
CrpSgn;

typedef struct
{
	char *pchChr;
	int *pinChr;
}
ChrSiz;

int inCrpDukErr (int in_ath, int bo_ery);
int inCrdExc (void);		// AJM 17/10/2012 1

#define CPX_ERC_CRC		-50
#define CPX_ERC_CKV		-51
#define CPX_ERC_SYS		-40
#define CPX_ERC_IPK		-41
#define CPX_ERC_KSN		-42
#define CPX_ERC_BIN		-43
#define CPX_ERC_TKA		-45
#define CPX_ERC_QSA		-46
#define CPX_ERC_QWA		-47
#define CPX_ERC_QXD		-48
#define CPX_ERC_QTD		-49
#define CPX_ERC_TK2		-70
#define CPX_ERC_EXC		-71
#define CPX_ERC_DEC		-79
#define CPX_ERC_SCS		-80
#define CPX_ERC_TKC		-81
#define CPX_ERC_AVD		-82
#define CPX_ERC_SAC		-83
#define CPX_ERC_HSM		-84

#endif

#endif

//Migrado por JVelazquez 22/08/14
#endif //MAKE_DUKPT
