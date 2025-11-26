//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
// AJM 14/02/2010 1
#ifndef CRPDKP_H
#define CRPDKP_H

//#include "define.h"
//#include "vmacload.h"
#include "dkprsa.h"

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif


#define DKP_NEW_KEY		0
#define DKP_NEW_KEY_1	1
#define DKP_NEW_KEY_2	2
#define DKP_NEW_KEY_3	3
#define DKP_NEW_KEY_4	4
#define DKP_RQT_DTA_1	5
#define DKP_RQT_DTA_2	6

#define ACT_NUM_SIZ		12
#define HEX_KEY_SIZ		16
#define HEX_KSN_SIZ		10
#define FUT_KEY_NUM		21
#define CRP_REC_SIZ		8

typedef struct
{
	char chKey[HEX_KEY_SIZ];		// 32 hex
	char chLrc;

}
FutKey;

typedef struct
{
	// full ped life
	char chAct[ACT_NUM_SIZ];			// 12 dec
	char chKsn[HEX_KSN_SIZ];			// 20 hex
	FutKey srFutKey[FUT_KEY_NUM + 1];	// 21 rec - 34 hex
	unsigned long ulCnt;				// 21 bit

	// temp base
	int inKey;
	unsigned long ulSft;				// 21 bit
	char chCrp1[CRP_REC_SIZ];			// 16 hex
	char chCrp2[CRP_REC_SIZ];			// 16 hex
	char chKey[HEX_KEY_SIZ];			// 32 hex
	char chMac[HEX_KEY_SIZ];			// 32 hex
	char chDta[HEX_KEY_SIZ];			// 32 hex
	char chPin[HEX_KEY_SIZ];			// 32 hex
}
DkpEnv;

typedef struct
{
	char chKey[HEX_KEY_SIZ];
	char chKsn[HEX_KSN_SIZ];
	int inMod;
}
CrpDuk;

int inDkpLclLbl(DkpEnv *psrDkpEnv, int inLclLbl, CrpDuk *psrCrpDuk);
int inDkpIpkLod(char pchKey[], int inKey, char pchKsn[], int inKsn);
int inDkpRqtDta(void);
int inDkpRqtKey(char pchKey[], char pchKsn[]);		// AJM 01/08/2010 1
int inDkpSetBit(DkpEnv *psrDkpEnv);
int inDkpNonRvrKeyGenPrc(DkpEnv *psrDkpEnv);
int inTrpDeaEncPin(DkpEnv *psrDkpEnv);
int inTrpDeaEncDta(DkpEnv *psrDkpEnv, CrpDuk *psrCrpDuk);		// AJM 01/08/2010 1
int inTrpDes(int inTch, char pchKey[], char pchSrc[], int inSrc, char pchEnd[]);
int inDkpChkSftBit(unsigned long ulSft);
int inDkpHowMnyOne(unsigned long ulEncCnt);
unsigned long ulDkpAddCrpAndShf(DkpEnv *psrDkpEnv);
int inDkpRmvCurKey(DkpEnv *psrDkpEnv);
int inDkpKsnCrpRed(char pchKsn[], unsigned long *pulCrpNmb);
int inDkpKsnCrpWrt(char pchKsn[], unsigned long ulCrpNmb);
int inXor(char pchEnd[], char pchOp1[], char pchOp2[], int inChr);
int inDkpDmp(DkpEnv *psrDkpEnv);

#define DUK_CRP_CHR		0
#define DUK_ASK_KEY		1

#endif
// AJM 14/02/2010 1

//Migrado por JVelazquez 22/08/14
#endif //MAKE_DUKPT
