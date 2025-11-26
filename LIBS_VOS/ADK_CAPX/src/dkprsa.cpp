#ifdef MAKE_DUKPT
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _VRXEVO
#include <svc_sec.h>
#else
#include "svcsec.h"
#endif

#include "svc.h"
#include "debug.h" 
#include "bignum.h" 
#include "rsa.h"
#include "dkprsa.h"
#include "log/liblog.h"




#ifdef _VRXEVO
#define CRPRSA "I:1/CrpRsa.dat"
#define CRPCRD_DAT "I:1/CrpCrd.dat"
#define CRPKTK_DAT "I:1/CrpKtk.dat"
#define CRDEXC_DAT "I:1/CrdExc.dat"
#define EXCCRD_DAT "I:1/ExcCrd.dat"
#define NEWRSA_DAT "I:1/NewRsa.dat"
#define RSASGN_DAT "I:1/RsaSgn.dat"
#else
#define CRPRSA "./flash/themes/themes/BrushedMetal/CrpRsa.dat"
#define CRPCRD_DAT "/home/usr1/flash/CrpCrd.dat"
#define CRPKTK_DAT "/home/usr1/flash/CrpKtk.dat"
#define CRDEXC_DAT "/home/usr1/flash/CrdExc.dat"
#define EXCCRD_DAT "/home/usr1/flash/ExcCrd.dat"
#define NEWRSA_DAT "/home/usr1/flash/NewRsa.dat"
#define RSASGN_DAT "/home/usr1/flash/RsaSgn.dat"
#endif

#ifdef __arm

#include <time.h>
//#include "../../include/usrcmds.h"
//#include "rsa.h"
//#include "../polarssl/include/polarssl/sha1.h"
//#include "../../include/application.h"
//#include "../../include/Misc.h"

int inSetEnvVarpch(char pchVar[], char pchVal[], char pchFil[]);
char *pchGetEnvVar(char pchVar[], char pchVal[], int inMaxChr, char pchFil[]);

int inCrpLod(char pchFil[], void *pvdDst, int inChr)
{
	int inHdl;
	int inRetVal;
	int inTmp0;

	LOGF_TRACE("--- inCrpLod ---"); 
	
	LOGF_TRACE("---> pchFil = %s",pchFil);  
	LOGF_TRACE("---> pvdDst = %s",pvdDst);  
	LOGF_TRACE("---> inChr = %i",inChr); 

	#ifdef _VRXEVO
		inHdl = open( pchFil,  O_RDWR);
	#else
		inHdl = open( pchFil,  O_RDWR, 0666 );
	#endif
	
	LOGF_TRACE("---> inHdl = %i",inHdl); 

	if (inHdl < 0)
	{
		return VS_ERROR;
	}

	memset(pvdDst, 0, inChr);
	LOGAPI_HEXDUMP_TRACE("---> pvdDst",pvdDst,inChr);  

	#ifdef _VRXEVO
		inTmp0 = crypto_read(inHdl, (char*)pvdDst, inChr);
	#else
		inTmp0 = cryptoRead(inHdl, (char*)pvdDst, inChr);
	#endif
	
	LOGF_TRACE("---> inTmp0 = %i",inTmp0); 
	LOGAPI_HEXDUMP_TRACE("---> pvdDst: ",pvdDst,inChr); 

	close(inHdl);

	LOGAPI_HEXDUMP_TRACE("---> pvdDst:",pvdDst, inTmp0 > 0 ? inTmp0 : 0);
	inRetVal = VS_SUCCESS;

	if (inTmp0 < inChr)
		inRetVal = VS_ERROR;

	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	LOGF_TRACE("--- End inCrpLod ---");
	
	return inRetVal;
	
}

int inCrpSav(char pchFil[], void *pvdDst, int inChr)		// AJM 03/02/2010 1
{
	int inHdl = 0;
	int inRetVal = 0;
	int inTmp0 = 0;

	LOGF_TRACE("--- inCrpSav ---");
	LOGF_TRACE("---> pchFil = %s",pchFil); 
	LOGAPI_HEXDUMP_TRACE("---> pvdDst:",pvdDst, inChr);

	#ifdef _VRXEVO
		inHdl = open(pchFil, O_CREAT | O_TRUNC | O_WRONLY);
	#else
		inHdl = open(pchFil, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	#endif
	
	LOGF_TRACE("---> inHdl = %i",inHdl);

	if (inHdl < 0)
	{
		LOGF_TRACE("--- End inCrpSav ---");
		return VS_ERROR;
	}

	#ifdef _VRXEVO
		inTmp0 = crypto_write(inHdl,(char*)pvdDst, inChr);
	#else
		inTmp0 = cryptoWrite(inHdl,(char*)pvdDst, inChr);
	#endif
	
	LOGF_TRACE("---> inTmp0 = %i",inTmp0);
	LOGAPI_HEXDUMP_TRACE("---> pvdDst: ",pvdDst,inChr);

	close(inHdl);

	inRetVal = VS_SUCCESS;
	
	if (inTmp0 < inChr)
		inRetVal = VS_ERROR;

	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	LOGF_TRACE("--- End inCrpSav ---");

	return inRetVal;
	
}

int inCrpAsmBgn(void)
{
	CrpCrd srCrpCrd;
	int inRetVal;

	LOGF_TRACE("--- inCrpAsmBgn ---"); 
	
	inRetVal = VS_ERROR;

	if (inCrpLod((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)
	{
		inRetVal = VS_ESCAPE;

		if (srCrpCrd.srCrpTk2.fTk2)
		{
			char chAsm[(ASC_ASM_SIZ) + 1];
			char chAsc[ASC_TK1_SIZ];
			int inIdx;

			inIdx = 0;

			memset(chAsm, 0, sizeof(chAsm));

			pad(chAsm + inIdx, srCrpCrd.chTk2, 'F', ASC_TK2_SIZ, LEFT_);
			LOGAPI_HEXDUMP_TRACE("---> trk 2:",chAsm + inIdx, ASC_TK2_SIZ);
			inIdx += ASC_TK2_SIZ;

			pad(chAsm + inIdx, srCrpCrd.chCv2, 'F', ASC_CV2_SIZ, LEFT_);
			LOGAPI_HEXDUMP_TRACE("---> cvv 2:",chAsm + inIdx, ASC_CV2_SIZ);
			inIdx += ASC_CV2_SIZ;

			memset(chAsc, 0, sizeof(chAsc));
			inHexToAsc(srCrpCrd.chTk1, chAsc, srCrpCrd.srCrpTk1.inTk1);
			pad(chAsm + inIdx, chAsc, 'F', ASC_TK1_SIZ, LEFT_);
			LOGAPI_HEXDUMP_TRACE("---> trk 1:",chAsm + inIdx, ASC_TK1_SIZ);
			inIdx += ASC_TK1_SIZ;

			inAscToHex(chAsm, srCrpCrd.chAsm, inIdx);
			LOGAPI_HEXDUMP_TRACE("---> hex:",srCrpCrd.chAsm, sizeof(srCrpCrd.chAsm));

			inRetVal = inCrpSav((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
			
		}
		
	}

	memset(&srCrpCrd, 0, sizeof(srCrpCrd));

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpAsmBgn ---");

	return inRetVal;
	
}

int inCrpAsmEnd(void)
{
	CrpCrd srCrpCrd;
	int inRetVal;

	LOGF_TRACE("--- inCrpAsmEnd ---");
	
	inRetVal = VS_ERROR;

	if (inCrpLod((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)
	{
		int inIdx;
		unsigned long ulCrc;

		inIdx = 0;

		inHexToAsc(srCrpCrd.chDsm + inIdx, srCrpCrd.srCrpTk2.chTk2, HEX_TV2_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> trk 2:",srCrpCrd.srCrpTk2.chTk2, ASC_TV2_SIZ);
		inIdx += HEX_TV2_SIZ;

		ulCrc = ul_crc_32(srCrpCrd.srCrpTk2.chTk2, ASC_TV2_SIZ, 0xFFFFFFFF);
		inHexToAsc((char *) &ulCrc, srCrpCrd.srCrpTk2.chCrc, HEX_CRC_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> crc 2:",srCrpCrd.srCrpTk2.chCrc, ASC_CRC_SIZ);

		if (srCrpCrd.srCrpTk2.fTk1)
		{
			inHexToAsc(srCrpCrd.chDsm + inIdx, srCrpCrd.srCrpTk1.chTk1, HEX_TK1_SIZ);
			LOGAPI_HEXDUMP_TRACE("---> trk 1:",srCrpCrd.srCrpTk1.chTk1, ASC_TK1_SIZ);
			inIdx += HEX_TK1_SIZ;

			ulCrc = ul_crc_32(srCrpCrd.srCrpTk1.chTk1, ASC_TK1_SIZ, 0xFFFFFFFF);
			inHexToAsc((char *) &ulCrc, srCrpCrd.srCrpTk1.chCrc, HEX_CRC_SIZ);
			LOGAPI_HEXDUMP_TRACE("---> crc 1:",srCrpCrd.srCrpTk1.chCrc, ASC_CRC_SIZ);
		}

		
		srCrpCrd.srCrpTk2.inErr = inGetEnvVar("DKPERR", NULL);

		LOGF_TRACE("--inCrpAsmEnd [%d]--",srCrpCrd.srCrpTk2.inErr);
		inRetVal = inCrpSav(CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
	}

	memset(&srCrpCrd, 0, sizeof(srCrpCrd));

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpAsmEnd ---");

	return inRetVal;
	
}

int inCrpKtkAsm (void)
{
	CrpKtk srCrpKtk;
	int inRetVal;

	LOGF_TRACE("--- inCrpKtkAsm ---");
	
	memset(&srCrpKtk, 0, sizeof(srCrpKtk));
	inCrpKtkRnd(srCrpKtk.chKtk, sizeof(srCrpKtk.chKtk));
	inRetVal = inCrpKtkRsa(&srCrpKtk.srKtkRqt, srCrpKtk.chKtk, sizeof(srCrpKtk.chKtk));

	if (inRetVal == VS_SUCCESS)
	{
		inCrpChkVal(srCrpKtk.chKtk, sizeof(srCrpKtk.chKtk), srCrpKtk.srKtkRqt.chChk, sizeof(srCrpKtk.srKtkRqt.chChk));
		inRetVal = inCrpSav((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk));
	}

	memset(&srCrpKtk, 0, sizeof(srCrpKtk));

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpKtkAsm ---");

	return inRetVal;
}

int inCrpKtkDsm (void)
{
	CrpKtk srCrpKtk;
	int inRetVal;

	LOGF_TRACE("--- inCrpKtkDsm ---");

	inRetVal = CPX_ERC_SYS;

	if (inCrpLod((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk)) == VS_SUCCESS)
	{
		inRetVal = VS_SUCCESS;

		if (inRetVal == VS_SUCCESS)
		{
			LOGAPI_HEXDUMP_TRACE("---> err:",srCrpKtk.srKtkRsp.chErr, sizeof(srCrpKtk.srKtkRsp.chErr));

			if (memcmp(srCrpKtk.srKtkRsp.chErr, "00", 2))
			{
				inRetVal = inKtkRspShwXlt(srCrpKtk.srKtkRsp.chErr, 2);
			}
		}

		if (inRetVal == VS_SUCCESS)
		{
			char chAsc[ASC_KEY_SIZ]={0};  // FAG 30-oct-2017
			unsigned long ulCrc;

			inHexToAsc(srCrpKtk.srKtkRsp.chKey, chAsc, HEX_KEY_SIZ);

			#ifdef HRD_COD_RSP	// FAG 13-SEP-2017
			LOGF_TRACE("VERSION HARDCODE");

			memset(chAsc,0,sizeof(chAsc));
			LOGF_TRACE("---> chAsc = %i",chAsc); 
			memcpy(chAsc,"C4C79AB88F365F2EEF113ADA652876D7",32);
			#endif

			ulCrc = ul_crc_32(chAsc, ASC_KEY_SIZ, 0xFFFFFFFF);
			inHexToAsc((char *) &ulCrc, chAsc, sizeof(ulCrc));
			LOGAPI_HEXDUMP_TRACE("---> crc:",srCrpKtk.srKtkRsp.chCrc, sizeof(srCrpKtk.srKtkRsp.chCrc));
			LOGAPI_HEXDUMP_TRACE("---> crc 1:",chAsc, sizeof(chAsc));

			//***************************************** ONLY TEST ********************************************************************
			///*
			if (memcmp(chAsc, srCrpKtk.srKtkRsp.chCrc, ASC_CRC_SIZ))
			{
				inRetVal = CPX_ERC_CRC;
			}
			//*/
			
		}

		if (inRetVal == VS_SUCCESS)
		{
			char chChk[ASC_CHK_SIZ]={0};  // FAG 30-oct-2017

			inTrpDes(TDES2KD, srCrpKtk.chKtk, srCrpKtk.srKtkRsp.chKey, sizeof(srCrpKtk.srKtkRsp.chKey), srCrpKtk.chKey);
			inCrpChkVal(srCrpKtk.chKey, sizeof(srCrpKtk.chKey), chChk, sizeof(chChk));
			LOGAPI_HEXDUMP_TRACE("---> chk val:",srCrpKtk.srKtkRsp.chChk, sizeof(srCrpKtk.srKtkRsp.chChk));

			//***************************************** ONLY TEST ********************************************************************
			///*
			if (memcmp(chChk, srCrpKtk.srKtkRsp.chChk, ASC_CHK_SIZ))
			{
				LOGF_TRACE("<<< Chk Value Error!! >>>"); 
				inRetVal = CPX_ERC_CKV;
			}
			//*/
			
		}

		#if defined (HRD_COD_RSP)		// AJM 12/12/2012 1
		LOGF_TRACE("VERSION HARDCODE");

		if (inRetVal == VS_SUCCESS)
		{
			char chKey [(ASC_KEY_SIZ) + 5] = {0};

			LOGAPI_HEXDUMP_TRACE("---> key",srCrpKtk.chKey, sizeof (srCrpKtk.chKey));
			pchGetEnvVar ((char *)"CRPDUKKEY", chKey, sizeof (chKey), NULL);
			inAscToHex (chKey, srCrpKtk.chKey, ASC_KEY_SIZ);
			LOGAPI_HEXDUMP_TRACE("---> key",srCrpKtk.chKey, sizeof (srCrpKtk.chKey));
		}

		#endif

		if (inRetVal == VS_SUCCESS)
		{
			inAscToHex(srCrpKtk.srKtkRsp.chKsn, srCrpKtk.chKsn, sizeof(srCrpKtk.srKtkRsp.chKsn));

			LOGF_TRACE("<<< If check Value is succesfull >>>");   
			inRetVal = inCrpSav((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk)); 

			if (inRetVal < VS_SUCCESS)
			{
				inRetVal = CPX_ERC_SYS;
			}
		}
	}

	memset(&srCrpKtk, 0, sizeof(srCrpKtk));

	LOGF_TRACE("---> inRetVal = %i",inRetVal);  
	LOGF_TRACE("--- End inCrpKtkDsm ---");
	
	return inRetVal;
}

int in_ipk_sav (void)
{
	CrpKtk srCrpKtk;
	int in_val;

	LOGF_TRACE("--- in_ipk_sav ---");

	memset (&srCrpKtk, 0, sizeof (srCrpKtk));

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inCrpLod ((char *)CRPKTK_DAT, &srCrpKtk, sizeof (srCrpKtk));

		if (in_ret < SUCCESS)
		{
			in_val = CPX_ERC_SYS;
		}
	}

	if (in_val > FAILURE)
	{
		int in_ret;

		inSetEnvVarin ((char *)"DKPKEYLOD", 0, NULL);
		in_ret = inDkpIpkLod (srCrpKtk.chKey, sizeof (srCrpKtk.chKey), srCrpKtk.chKsn, sizeof (srCrpKtk.chKsn));

		if (in_ret < SUCCESS)
		{
			in_val = CPX_ERC_IPK;
		}
		else
		{
			inSetEnvVarin ((char *)"DKPKEYLOD", 1, NULL);
		}
	}

	memset (&srCrpKtk, 0, sizeof (srCrpKtk));

	LOGF_TRACE("---> in_val=%d", in_val);
	LOGF_TRACE("--- End in_ipk_sav ---");

	return in_val;
	
}

int inCrpKtkRnd(char pchKtk[], int inKtk)
{
	int inIdx;

	LOGF_TRACE("--- inCrpKtkRnd ---");   

	//    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	//    APP_TRACE(szDebugMsg);

//	#endif

	#if defined (HRD_COD_RSP)
	LOGF_TRACE("VERSION HARDCODE");
	memcpy(pchKtk, "\x01\x56\x10\x5A\x6D\x12\xB2\x2F\x47\x62\xE8\x08\xB9\x05\x5A\xE9", HEX_KEY_SIZ);

	#else

	srand(time(NULL));

	for (inIdx = 0; inIdx < inKtk; inIdx++)
	{
		pchKtk[inIdx] = rand() & 0xFF;
	}
	#endif
	
	LOGF_TRACE("<<< Generate the transport key >>>");  
	LOGAPI_HEXDUMP_TRACE("---> ktk:",pchKtk, inKtk);

	LOGF_TRACE("--- End inCrpKtkRnd ---");
	
	return inKtk;
}

static int in_rsa_fek (unsigned char puc_crp [], int in_crp)
{
	unsigned char uc_fek [] = 
	{
        0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,   0X08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
        0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,   0X07, 0X06, 0X05, 0X04, 0X03, 0X02, 0X01, 0X00,
        0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,   0X80, 0X90, 0XA0, 0XB0, 0XC0, 0XD0, 0XE0, 0XF0,
        0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80,   0X70, 0X60, 0X50, 0X40, 0X30, 0X20, 0X10, 0X00,

        0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,   0X07, 0X06, 0X05, 0X04, 0X03, 0X02, 0X01, 0X00,
        0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,   0X08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
        0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80,   0X70, 0X60, 0X50, 0X40, 0X30, 0X20, 0X10, 0X00,
        0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,   0X80, 0X90, 0XA0, 0XB0, 0XC0, 0XD0, 0XE0, 0XF0,

        0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,   0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,
        0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,   0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,
        0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,   0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80,
        0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80,   0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,

        0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,   0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,
        0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07,   0X0F, 0X0E, 0X0D, 0X0C, 0X0B, 0X0A, 0X09, 0X08,
        0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80,   0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,
        0X00, 0X10, 0X20, 0X30, 0X40, 0X50, 0X60, 0X70,   0XF0, 0XE0, 0XD0, 0XC0, 0XB0, 0XA0, 0X90, 0X80
	};
	unsigned char uc_hex [ASC_MOD_SIZ];
	int  in_hex;
	int  in_idx;

	LOGF_TRACE("--- in_rsa_fek ---");
	
	LOGAPI_HEXDUMP_TRACE("---> crp:",puc_crp, in_crp);

	memset (uc_hex, 0, sizeof (uc_hex));
	in_hex = inAscToHex ((char *) puc_crp, (char *) uc_hex, in_crp);

	for (in_idx = 0; in_idx < in_hex; in_idx++)
	{
		uc_hex [in_idx] ^= uc_fek [in_idx];
	}

	memset (puc_crp, 0, in_crp);
	in_crp = inHexToAsc ((char *) uc_hex, (char *) puc_crp, in_hex);

	LOGAPI_HEXDUMP_TRACE("---> crp:",puc_crp, in_crp);
	
	LOGF_TRACE("--- End in_rsa_fek ---");

	return in_crp;
	
}

int inCrpRsaLod(char pchRsa[], KtkRqt *psrVer, rsa_context *psrRsa, int bo_fek)
{
	int inRetVal;
	int inHdl;
	int inChr;
	int inAsc;
	char chTmp[30];

	LOGF_TRACE("--- inCrpRsaLod ---");	
	LOGF_TRACE("---> pchRsa=%s",pchRsa);
	LOGF_TRACE("---> pchRsa = %i",pchRsa); 
	
	inRetVal = VS_SUCCESS;

	if (inRetVal == VS_SUCCESS)
	{
		inHdl = in_open(pchRsa, O_RDONLY);
		LOGF_TRACE("---> inHdl Rsa = %i",inHdl);
		
		if (inHdl < 0)
		{
			inRetVal = VS_ERROR;
		}
	}

	if (inRetVal == VS_SUCCESS)
	{
		memset(psrRsa, 0, sizeof(*psrRsa));
		rsa_init(psrRsa, RSA_PKCS_V15, 0, NULL, NULL);		

		// mod len
		memset(chTmp, 0, sizeof(chTmp));
		inChr = in_read(inHdl, chTmp, 3);

		if (inChr < 3)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> len:",chTmp, inChr);

		inChr = atoi(chTmp);
		psrRsa->len = inChr;
	}

	if (inRetVal == VS_SUCCESS)
	{
		char chMod[1024];

		// mod
		memset(chMod, 0, sizeof(chMod));
		inAsc = inChr << 1;
		inChr = in_read(inHdl, chMod, inAsc);

		if (inChr < inAsc)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> mod:",chMod, inChr);


		if (FALSE)
		{
			in_rsa_fek ((unsigned char *) chMod, inChr);
			LOGAPI_HEXDUMP_TRACE("---> mod:",chMod, inChr);

		}

		mpi_read_string(&psrRsa->N, 16, chMod);
	}

	if (inRetVal == VS_SUCCESS)
	{
		// exp len
		memset(chTmp, 0, sizeof(chTmp));
		inChr = in_read(inHdl, chTmp, 2);

		if (inChr < 2)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> len:",chTmp, inChr);

		inChr = atoi(chTmp);
	}

	if (inRetVal == VS_SUCCESS)
	{
		// exp
		memset(chTmp, 0, sizeof(chTmp));
		inAsc = inChr << 1;
		inChr = in_read(inHdl, chTmp, inAsc);

		if (inChr < inAsc)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> exp:",chTmp, inChr);

		mpi_read_string(&psrRsa->E, 16, chTmp);
	}

	if (inRetVal == VS_SUCCESS)
	{
		// ver len
		memset(chTmp, 0, sizeof(chTmp));
		inChr = in_read(inHdl, chTmp, 2);

		if (inChr < 2)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> len:",chTmp, inChr);

		inChr = atoi(chTmp);
	}

	if (inRetVal == VS_SUCCESS)
	{
		// ver
		memset(chTmp, 0, sizeof(chTmp));
		inAsc = inChr;
		inChr = in_read(inHdl, chTmp, inAsc);

		if (inChr < inAsc)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> ver:",chTmp, inChr);

		memcpy(psrVer->chVer, chTmp, sizeof(psrVer->chVer));
	}

	if (inRetVal == VS_SUCCESS)
	{
		// pad len
		memset(chTmp, 0, sizeof(chTmp));
		inChr = in_read(inHdl, chTmp, 2);

		if (inChr < 2)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> len:",chTmp, inChr);
		
		inChr = atoi(chTmp);
	}

	if (inRetVal == VS_SUCCESS)
	{
		// pad
		memset(chTmp, 0, sizeof(chTmp));
		inAsc = inChr << 1;
		inChr = in_read(inHdl, chTmp, inAsc);

		if (inChr < inAsc)
		{
			inRetVal = VS_ERROR;
		}

		LOGAPI_HEXDUMP_TRACE("---> pad:",chTmp, inChr);

		memcpy(psrVer->chPad, chTmp, sizeof(psrVer->chPad));
	}

	in_close(inHdl);
	LOGF_TRACE("<<< RSA loaded >>>");  
	LOGF_TRACE("--- End inCrpRsaLod ---");

	return inRetVal;
	
}

int inCrpKtkRsa(KtkRqt *psrRsa, char pchKtk[], int inKtk)
{
	int inRetVal;

	char chCrp[512];

	// FAG 30-oct-2017
	rsa_context *srRsa = (rsa_context *)malloc(sizeof(rsa_context));

	if( srRsa == NULL )
	{
		LOGF_TRACE("RSA: Memory error.");
		return -1;
	}









	
	//dump(pchKtk, inKtk, "(Transport Key)ktk:");  

	inRetVal = VS_SUCCESS;

	if (inRetVal == VS_SUCCESS)
	{
		//inRetVal = inCrpRsaLod("CrpRsa.dat", psrRsa, &srRsa, TRUE);CRPRSA
		inRetVal = inCrpRsaLod(CRPRSA, psrRsa, srRsa, TRUE);  // FAG 30-oct-2017

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

			debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
			APP_TRACE(szDebugMsg);

		#endif
	}

	if (inRetVal == VS_SUCCESS)
	{
		inRetVal = rsa_check_pubkey(srRsa);

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

			debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
			APP_TRACE(szDebugMsg);

		#endif
	}

	if (inRetVal == VS_SUCCESS)
	{
		memset(chCrp, 0, sizeof(chCrp));

		inRetVal = rsa_pkcs1_encrypt(srRsa, RSA_PUBLIC, inKtk, (unsigned char *) pchKtk, (unsigned char *) chCrp);  // FAG 30-oct-2017

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

			debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
			APP_TRACE(szDebugMsg);

		#endif
	}

	LOGAPI_HEXDUMP_TRACE("---> pchKey:",chCrp, srRsa->len);


	LOGF_TRACE("...inCrpKtkRsa...");

	if (inRetVal == VS_SUCCESS)
	{
		unsigned long ulCrc;

		//dump(chCrp, sizeof(chCrp), "crp:");

//		memcpy(psrRsa->chRsa, chCrp, srRsa.len);

		inHexToAsc(chCrp, psrRsa->chRsa, HEX_MOD_SIZ);

		//dump(psrRsa->chRsa, sizeof(psrRsa->chRsa), "rsa(ktk):");

//		ulCrc = ulCrc32(psrRsa->chRsa, srRsa.len);

		ulCrc = ul_crc_32(psrRsa->chRsa, ASC_MOD_SIZ, 0xFFFFFFFF);

		inHexToAsc((char *) &ulCrc, psrRsa->chCrc, sizeof(ulCrc));

		//dump(psrRsa->chCrc, ASC_CRC_SIZ, "crc:");
	}

	//rsa_free(&srRsa); //STRK
	free(srRsa);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

		debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
		APP_TRACE(szDebugMsg);


	#endif

	return inRetVal;
}

int inCrpChkVal(char pchKey[], int inKey, char pchChk[], int inChk)
{
	char chSrc[HEX_KEY_SIZ];
	char chEnd[HEX_KEY_SIZ];

	LOGF_TRACE("--- inCrpChkVal ---"); 

	LOGAPI_HEXDUMP_TRACE("---> pchKey:",pchKey, inKey);

	memset(chSrc, 0, sizeof(chSrc));
	memset(chEnd, 0, sizeof(chEnd));
	inTrpDes(TDES2KE, pchKey, chSrc, sizeof(chSrc), chEnd);
	inHexToAsc(chEnd, pchChk, inChk >> 1);
	
    LOGF_TRACE("<<< 3DES key (ASCII) >>>"); 
	LOGAPI_HEXDUMP_TRACE("---> pchChk:",pchChk, inChk);
	LOGF_TRACE("--- End inCrpChkVal ---"); 

	return VS_SUCCESS;
	
}

int inKtkRspShwXlt(char pchRsp[], int inRsp)
{
	typedef struct
	{
		char *pchRsp;
		int inShw;
	}
	RspShw;
	
	RspShw srRspShw[] =
	{
		{(char *)"00", CPX_ERC_SCS},
		{(char *)"01", CPX_ERC_TKC},
		{(char *)"02", CPX_ERC_AVD},
		{(char *)"03", CPX_ERC_SAC},
		{(char *)"04", CPX_ERC_HSM}
	};
	
	RspShw *psrRspShw;
	int inMax;
	int inIdx;
	int inRetVal;
	
	LOGF_TRACE("--- inKtkRspShwXlt ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchRsp:",pchRsp, inRsp);

	inRetVal = CPX_ERC_DEC;
	inMax = sizeof(srRspShw) / sizeof(*srRspShw);

	for (inIdx = 0; inIdx < inMax; inIdx++)
	{
		psrRspShw = srRspShw + inIdx;

		if (!memcmp(psrRspShw->pchRsp, pchRsp, inRsp))
		{
			inRetVal = psrRspShw->inShw;
			break;
		}
	}

	LOGF_TRACE("---> inRetVal = %i",inRetVal); 
	LOGF_TRACE("--- End inKtkRspShwXlt ---");
	
	return inRetVal;
	
}
#endif


int inCrpDkpRun(char pchAct[], int inAct)	
{
	int inRetVal;

	LOGF_TRACE("--- inCrpDkpRun ---");

	inRetVal = VS_ESCAPE;

	#ifdef __arm

	inRetVal = VS_SUCCESS;

	if (inRetVal == VS_SUCCESS)
	{
		if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))
		{
			inRetVal = VS_ESCAPE;
		}
	}

	if (inRetVal == VS_SUCCESS)
	{
		#ifdef MAKE_DUKPT
		int BinExcepcion;
		#endif
		
		if (inCrpExcCrd(pchAct, inAct) == VS_SUCCESS)
		{
			inRetVal = CPX_ERC_EXC;
		}

		#ifdef MAKE_DUKPT
		BinExcepcion = FALSE;
		if(BinExcepcion)
			inRetVal = CPX_ERC_EXC;
		#endif
	}

	if (inRetVal == VS_SUCCESS)
	{
		if (inCrpAsmBgn() < VS_SUCCESS)
		{
			inRetVal = VS_ERROR;
		}
	}

	if (inRetVal == VS_SUCCESS)
	{
		if (inDkpRqtDta() < VS_SUCCESS)
		{
			inRetVal = VS_ERROR;
		}
	}

	if (inRetVal == VS_SUCCESS)
	{
		if (inCrpAsmEnd() < VS_SUCCESS)
		{
			inRetVal = VS_ERROR;
		}
	}

	#endif

	LOGF_TRACE("---> inRetVal=%d",inRetVal);
	LOGF_TRACE("--- End inCrpDkpRun ---");

	return inRetVal;
	
}

int inCrpSwpCrdSav(char pchCrd[], int inCrd, char pchMod[])
{
	int inRetVal;

	LOGF_TRACE("--- inCrpSwpCrdSav ---");

	LOGAPI_HEXDUMP_TRACE("---> pchCrd:",pchCrd, inCrd);
	LOGAPI_HEXDUMP_TRACE("---> pchMod:",pchMod, 2);

//	gfCrpCrd = VS_FALSE;
	inRetVal = VS_ESCAPE;

	#ifdef __arm

	if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))
	{
		LOGF_TRACE("--- End inCrpSwpCrdSav ---");
		return inRetVal;
	}

	inRetVal = VS_ERROR;

	if (inCrd > 0)
	{
		CrpCrd srCrpCrd;
		struct TRACK srTrk;
		int inDsm;
		int inChr;

		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
		memcpy(srCrpCrd.srCrpTk2.chMod, pchMod, 2);
		inDsm = inCrdDsm(&srTrk, pchCrd, inCrd, "1");
		LOGF_TRACE("---> inDsm=%d", inDsm);

		if (inDsm == TK_1)
		{
			inChr = strlen(srTrk.track);
			LOGAPI_HEXDUMP_TRACE("---> trk 1:",srTrk.track, inChr);

			srCrpCrd.srCrpTk2.fTk1 = VS_TRUE;
			memcpy(srCrpCrd.chTk1, srTrk.track, inChr);
			srCrpCrd.srCrpTk1.inTk1 = inChr;
			LOGAPI_HEXDUMP_TRACE("---> trk 1:",srCrpCrd.chTk1, srCrpCrd.srCrpTk1.inTk1);
		}

		inDsm = inCrdDsm(&srTrk, pchCrd, inCrd, "2");
		LOGF_TRACE("---> inDsm=%d", inDsm);

		if (inDsm == TK_2)
		{
			char *pchTmp;

			inChr = strlen(srTrk.track);
			LOGAPI_HEXDUMP_TRACE("---> trk 2:",srTrk.track, inChr);
			
			srCrpCrd.srCrpTk2.fTk2 = VS_TRUE;
			pchTmp = strchr(srTrk.track, '=');
			
			if (pchTmp)
				*pchTmp = 'D';

			memcpy(srCrpCrd.chTk2, srTrk.track, inChr);
			srCrpCrd.srCrpTk2.inTk2 = inChr;
			LOGAPI_HEXDUMP_TRACE("---> trk 2:",srCrpCrd.chTk2, srCrpCrd.srCrpTk2.inTk2);
			
			pchTmp = srTrk.acct;
			inChr = strlen(pchTmp);
			
			if (inChr > ASC_LT4_SIZ)
				pchTmp += inChr - ASC_LT4_SIZ;

			memcpy(srCrpCrd.srCrpTk2.chLt4, pchTmp, ASC_LT4_SIZ);
			LOGAPI_HEXDUMP_TRACE("---> chLt4:",srCrpCrd.srCrpTk2.chLt4, ASC_LT4_SIZ);
			
		}

		inDsm = inCrdDsm(&srTrk, pchCrd, inCrd, "3");
		LOGF_TRACE("---> inDsm=%d", inDsm);

		if (inDsm == TK_3)
		{
			inChr = strlen(srTrk.track);

			LOGAPI_HEXDUMP_TRACE("---> trk 3:",srTrk.track, inChr);
			memcpy(srCrpCrd.chTk3, srTrk.track, inChr);
			srCrpCrd.inTk3 = inChr;
			LOGAPI_HEXDUMP_TRACE("---> trk 3:",srCrpCrd.chTk3, srCrpCrd.inTk3);
		}

		inRetVal = inCrpSav((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
	}

	#endif

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpSwpCrdSav ---");

	return inRetVal;
	
}

int inCrpChpCrdSav(char pchTk2[], int inTk2, char pchNum[], int inNum, bool isCtls)
{
	int inRetVal;

	LOGF_TRACE("--- inCrpChpCrdSav ---");

	LOGAPI_HEXDUMP_TRACE("---> pchTk2:",pchTk2, inTk2);
	LOGAPI_HEXDUMP_TRACE("---> pchNum:",pchNum, inNum);

//	gfCrpCrd = VS_FALSE;
	inRetVal = VS_ESCAPE;

	#ifdef __arm

	if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))
	{
		LOGF_TRACE("--- End inCrpChpCrdSav ---");
		return inRetVal;
	}

	inRetVal = VS_ERROR;
	LOGF_TRACE("<<< After inGetEnvVar >>>");
	
	if (inTk2 > 0)
	{
		CrpCrd srCrpCrd;
		char *pchTmp;
		
		LOGF_TRACE("---> inTk2 = %i",inTk2);  

		memset(&srCrpCrd, 0, sizeof(srCrpCrd));

		LOGF_TRACE("\n--------entry_mode = %i", entry_mode);
		if(isCtls)
		{
			memcpy(srCrpCrd.srCrpTk2.chMod, "07", 2);
			LOGF_TRACE("\n--------entry_mode = %s",srCrpCrd.srCrpTk2.chMod);

		}

		else
		{
			memcpy(srCrpCrd.srCrpTk2.chMod, "05", 2);

			LOGF_TRACE("\n--------entry_mode = %s",srCrpCrd.srCrpTk2.chMod);
		}

		//memcpy(srCrpCrd.srCrpTk2.chMod, "05", 2);

		srCrpCrd.srCrpTk2.fTk2 = VS_TRUE;

		memcpy(srCrpCrd.chTk2, pchTk2, inTk2);
		srCrpCrd.srCrpTk2.inTk2 = inTk2;
		pchTmp = (char*)memchr(srCrpCrd.chTk2, '=', inTk2);
		
		if (pchTmp)
			*pchTmp = 'D';


		LOGAPI_HEXDUMP_TRACE("---> trk 2:",srCrpCrd.chTk2, srCrpCrd.srCrpTk2.inTk2);
		pchTmp = pchNum;
		
		if (inNum > ASC_LT4_SIZ)
			pchTmp += inNum - ASC_LT4_SIZ;

		memcpy(srCrpCrd.srCrpTk2.chLt4, pchTmp, ASC_LT4_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> lt4:",srCrpCrd.srCrpTk2.chLt4, ASC_LT4_SIZ);

		LOGF_TRACE("<<< Crea Archivo CRPCRD_DAT >>>");
		inRetVal = inCrpSav((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
		LOGF_TRACE("---> inRetVal = %i ",inRetVal);
		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
		
	}

	#endif

	LOGF_TRACE("---> inRetVal = %i ",inRetVal);
	LOGF_TRACE("--- End inCrpChpCrdSav ---");

	return inRetVal;
	
}

int inCrpKbdCrdSav(char pchNum[], int inNum, char pchExp[], int inExp)
{
	int inRetVal;

	LOGF_TRACE("--- inCrpKbdCrdSav ---");

	LOGAPI_HEXDUMP_TRACE("---> pchNum:",pchNum, inNum);
	LOGAPI_HEXDUMP_TRACE("---> pchExp:",pchExp, inExp);

//	gfCrpCrd = VS_FALSE;
	inRetVal = VS_ESCAPE;

	#ifdef __arm

	if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))
	{
		LOGF_TRACE("--- End inCrpKbdCrdSav ---");
		return inRetVal;
	}

	inRetVal = VS_ERROR;

	if (inNum > 0 && inExp > 0)
	{
		CrpCrd srCrpCrd;
		int inIdx;
		char *pchLt4;

		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
		memcpy(srCrpCrd.srCrpTk2.chMod, "01", 2);
		srCrpCrd.srCrpTk2.fTk2 = VS_TRUE;

		inIdx = 0;
		memcpy(srCrpCrd.chTk2 + inIdx, pchNum, inNum);
		inIdx += inNum;

		memcpy(srCrpCrd.chTk2 + inIdx, "D", 1);
		inIdx += 1;

		memcpy (srCrpCrd.chTk2 + inIdx, pchExp + 2, 2);	
		inIdx += 2;

		memcpy (srCrpCrd.chTk2 + inIdx, pchExp + 0, 2);
		inIdx += 2;

		srCrpCrd.srCrpTk2.inTk2 = inIdx;
		LOGAPI_HEXDUMP_TRACE("---> trk 2:",srCrpCrd.chTk2, srCrpCrd.srCrpTk2.inTk2);

		pchLt4 = pchNum;
		if (inNum > ASC_LT4_SIZ)
			pchLt4 += inNum - ASC_LT4_SIZ;

		memcpy(srCrpCrd.srCrpTk2.chLt4, pchLt4, ASC_LT4_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> lt4:",srCrpCrd.srCrpTk2.chLt4, ASC_LT4_SIZ);

		inRetVal = inCrpSav((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
		
	}

	#endif

	LOGF_TRACE("---> inRetVal = %i ",inRetVal);
	LOGF_TRACE("--- End inCrpKbdCrdSav ---");

	return inRetVal;
	
}

int inCrpCrdCv2Sav(char pchCv2[], int inCv2, int boCv2)	
{
	int inRetVal;

	LOGF_TRACE("--- inCrpCrdCv2Sav ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchCv2:",pchCv2, inCv2);

//	gfCrpCrd = VS_FALSE;
	inRetVal = VS_ESCAPE;

	#ifdef __arm

	if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))
	{
		LOGF_TRACE("--- End inCrpCrdCv2Sav ---");
		return inRetVal;
	}

	inRetVal = VS_ERROR;

//	if (inCv2 > 0)
	{
		CrpCrd srCrpCrd;

		if (inCrpLod((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)
		{
		
			LOGF_TRACE("--inCrpCrdCv2Sav--");

			srCrpCrd.srCrpTk2.fCv2 = '0';
			//srCrpCrd.srCrpTk2.fCv2 = VS_FALSE;
			LOGF_TRACE("--inCrpCrdCv2Sav boCv2 [%d]--", boCv2);

			/*
			if (!boCv2)
			{
				LOGF_TRACE("--inCrpCrdCv2Sav--");
				//srCrpCrd.srCrpTk2.fCv2 = 'A'; //JRS, 04102018 Se actualiza espec 2.22 en la cual desaparece la 'A' VERSIONES  <= M05/M55
				srCrpCrd.srCrpTk2.fCv2 = '0';
			}

			else if(boCv2)
			{
				LOGF_TRACE("--inCrpCrdCv2Sav inCv2 [%d]--", inCv2);

				if (inCv2 > 0)
				{
					LOGF_TRACE("--inCrpCrdCv2Sav--");
					srCrpCrd.srCrpTk2.fCv2 = '1';
					memcpy(srCrpCrd.chCv2, pchCv2, inCv2);
					srCrpCrd.srCrpTk2.inCv2 = inCv2;
					//dump(srCrpCrd.chCv2, srCrpCrd.srCrpTk2.inCv2, "cvv2:");
				}

			}	
			*/

			if(boCv2 == CVV_CAPTURED)
			{
				LOGF_TRACE("--inCrpCrdCv2Sav inCv2 [%d]--", inCv2);

				if (inCv2 > 0)
				{
					LOGF_TRACE("--inCrpCrdCv2Sav--");
					srCrpCrd.srCrpTk2.fCv2 = '1';
					memcpy(srCrpCrd.chCv2, pchCv2, inCv2);
					srCrpCrd.srCrpTk2.inCv2 = inCv2;
					//dump(srCrpCrd.chCv2, srCrpCrd.srCrpTk2.inCv2, "cvv2:");
				}

			}
			else // (boCv2 == CVV_CAPTURED) || (boCv2 == CVV_CAPTURED) || anything else
			{
				LOGF_TRACE("--inCrpCrdCv2Sav--");
				//srCrpCrd.srCrpTk2.fCv2 = 'A'; //JRS, 04102018 Se actualiza espec 2.22 en la cual desaparece la 'A' VERSIONES  <= M05/M55
				srCrpCrd.srCrpTk2.fCv2 = '0';
			}

			inRetVal = inCrpSav(CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
			LOGF_TRACE("--inCrpCrdCv2Sav inRetVal [%d]--",inRetVal);
		}



		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
	}

	#endif

	LOGF_TRACE("---> inRetVal = %i ",inRetVal);
	LOGF_TRACE("--- End inCrpCrdCv2Sav ---");

	return inRetVal;
	
}


//-----------------------------------------------------------------------------
//!  \brief     Esta funcion valida si se estara utilizando DUKPT en la aplicacion o no.
//-----------------------------------------------------------------------------
int inCrpKtkRmv(void)
{
	int inRetVal;

	LOGF_TRACE("--- inCrpKtkRmv ---");

	inRetVal = VS_ESCAPE;

	if (inGetEnvVar((char *)"CRPDKPRSA", NULL))
	{
		CrpKtk srCrpKtk;
		
		memset(&srCrpKtk, 0, sizeof(srCrpKtk));
		inRetVal = inCrpSav((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk));
	}

	LOGF_TRACE("---> inRetVal = %i ",inRetVal);
	LOGF_TRACE("--- End inCrpKtkRmv ---");

	return inRetVal;
	
}

int inCrpCrdRmv(void)
{
	int inRetVal;
	
	LOGF_TRACE("--- inCrpCrdRmv ---");

	inRetVal = VS_ESCAPE;
	LOGF_TRACE("---> inGetEnvVar(\"CRPDKPRSA\", NULL)  = %i",inGetEnvVar((char *)"CRPDKPRSA", NULL));
	
	if (inGetEnvVar((char *)"CRPDKPRSA", NULL))
	{
		CrpCrd srCrpCrd;

		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
		inRetVal = inCrpSav((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
		LOGF_TRACE("---> CRPCRD_DAT =  %s",CRPCRD_DAT);
	}

	LOGF_TRACE("---> inRetVal = %i ",inRetVal);
	LOGF_TRACE("--- End inCrpCrdRmv ---");

	return inRetVal;
	
}


DllSPEC int inAsmQs(char pchQs[], VS_BOOL fDukAsk)
{
	char szTmp0[50];
	int inQs;

	LOGF_TRACE("--- inAsmQs ---");

	memset(pchQs, 0, 80);
	inQs = 0;

	memcpy(pchQs + inQs, "!", 1);
	LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQs + inQs, 1);
	inQs += 1;

	memcpy(pchQs + inQs, " ", 1);
	LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQs + inQs, 1);
	inQs += 1;

	memcpy(pchQs + inQs, "ES", 2);
	LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQs + inQs, 2);
	inQs += 2;

	memcpy(pchQs + inQs, "00060", 5);
	LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQs + inQs, 5);
	inQs += 5;

	memcpy(pchQs + inQs, " ", 1);
	LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQs + inQs, 1);
	inQs += 1;

	memset(szTmp0, 0, sizeof(szTmp0));
	pchGetAppVer(szTmp0);
	pad(szTmp0, szTmp0, ' ', 20, LEFT_);		
	memcpy(pchQs + inQs, szTmp0, 20);
	LOGAPI_HEXDUMP_TRACE("---> app ver:",pchQs + inQs, 20);
	inQs += 20;

	memset(szTmp0, 0, sizeof(szTmp0));
	pchGetSrlNmb(szTmp0);
	pad(szTmp0, szTmp0, ' ', 20, LEFT_);
	memcpy(pchQs + inQs, szTmp0, 20);
	LOGAPI_HEXDUMP_TRACE("---> trm srl:",pchQs + inQs, 20);
	inQs += 20;

	memset(szTmp0, 0, sizeof(szTmp0));
	*szTmp0 = '0';
	
	if (inGetEnvVar((char *)"DKPKEYLOD", NULL))
		*szTmp0 = '5';
	
	memcpy(pchQs + inQs, szTmp0, 1);
	LOGAPI_HEXDUMP_TRACE("---> crp cfg:",pchQs + inQs, 1);
	inQs += 1;

	pchGetEnvVar((char *)"BINECR", szTmp0, 10, NULL);		
//	pchGetEnvVar("BINPNP", szTmp0, 10, NULL);
	pad(szTmp0, szTmp0, ' ', CRD_EXC_NAM, RIGHT_);
	memcpy(pchQs + inQs, szTmp0, CRD_EXC_NAM);
	LOGAPI_HEXDUMP_TRACE("---> bin ecr:",pchQs + inQs, CRD_EXC_NAM);
	inQs += CRD_EXC_NAM;

	pchGetEnvVar((char *)"BINPNP", szTmp0, 10, NULL);
	pad(szTmp0, szTmp0, ' ', CRD_EXC_NAM, RIGHT_);
	memcpy(pchQs + inQs, szTmp0, CRD_EXC_NAM);
	LOGAPI_HEXDUMP_TRACE("---> bin ecr:",pchQs + inQs, CRD_EXC_NAM);
	inQs += CRD_EXC_NAM;

	pchGetEnvVar((char *)"BINVER", szTmp0, 5, NULL);
	pad(szTmp0, szTmp0, ' ', CRD_EXC_VER, RIGHT_);
	memcpy(pchQs + inQs, szTmp0, CRD_EXC_VER);
	LOGAPI_HEXDUMP_TRACE("---> bin ecr:",pchQs + inQs, CRD_EXC_VER);
	inQs += CRD_EXC_VER;

	memset(szTmp0, 0, sizeof(szTmp0));
	*szTmp0 = '0';
	
	if (fDukAsk)
		*szTmp0 = '1';

	memcpy(pchQs + inQs, szTmp0, 1);
	LOGAPI_HEXDUMP_TRACE("---> new key:",pchQs + inQs, 1);
	inQs += 1;

	LOGAPI_HEXDUMP_TRACE("---> Qs:",pchQs, inQs);

	LOGF_TRACE("--- End inAsmQs ---");

	return inQs;
	
}

int inAsmQw(char pchQw[])
{
	CrpKtk srCrpKtk;
	int inQw;

	LOGF_TRACE("--- inAsmQw ---");

	inQw = 0;

	if (inCrpLod((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk)) == VS_SUCCESS)
	{
		memset(pchQw, 0, 550);
		inQw = 0;

		memcpy(pchQw + inQw, "!", 1);
		LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQw + inQw, 1);
		inQw += 1;

		memcpy(pchQw + inQw, " ", 1);
		LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQw + inQw, 1);
		inQw += 1;

		memcpy(pchQw + inQw, "EW", 2);
		LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQw + inQw, 2);	
		inQw += 2;

		memcpy(pchQw + inQw, "00538", 5);
		LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQw + inQw, 5);
		inQw += 5;

		memcpy(pchQw + inQw, " ", 1);
		LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQw + inQw, 1);
		inQw += 1;

		memcpy(pchQw + inQw, srCrpKtk.srKtkRqt.chRsa, ASC_MOD_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> rsa(ktk):",pchQw + inQw, ASC_MOD_SIZ);
		inQw += ASC_MOD_SIZ;

		memcpy(pchQw + inQw, srCrpKtk.srKtkRqt.chChk, ASC_CHK_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> chk val:",pchQw + inQw, ASC_CHK_SIZ);
		inQw += ASC_CHK_SIZ;

		memcpy(pchQw + inQw, srCrpKtk.srKtkRqt.chVer, ASC_VER_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> rsa ver:",pchQw + inQw, ASC_VER_SIZ);
		inQw += ASC_VER_SIZ;

		memcpy(pchQw + inQw, srCrpKtk.srKtkRqt.chPad, ASC_PAD_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> pad ver:",pchQw + inQw, ASC_PAD_SIZ);
		inQw += ASC_PAD_SIZ;

		memcpy(pchQw + inQw, srCrpKtk.srKtkRqt.chCrc, ASC_CRC_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> crc:",pchQw + inQw, ASC_CRC_SIZ);
		inQw += ASC_CRC_SIZ;
		
	}

	memset(&srCrpKtk, 0, sizeof(srCrpKtk));
	LOGAPI_HEXDUMP_TRACE("---> Qw:",pchQw, inQw);

	LOGF_TRACE("--- End inAsmQw ---");

	return inQw;
	
}

int inDsmQx(char pchQx[], int in_Qx)
{
	CrpKtk srCrpKtk;
	int inQx;

	LOGF_TRACE("--- inDsmQx ---");

	LOGAPI_HEXDUMP_TRACE("---> pchQx:",pchQx, in_Qx);
	inQx = 0;

	if ( (inCrpLod((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk)) == VS_SUCCESS) ) 
	{
		LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQx + inQx, 1);
		inQx += 1;

		LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQx + inQx, 1);
		inQx += 1;

		LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQx + inQx, 2);
		inQx += 2;

		LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQx + inQx, 5);
		inQx += 5;

		LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQx + inQx, 1);
		inQx += 1;

		inAscToHex(pchQx + inQx, srCrpKtk.srKtkRsp.chKey, ASC_KEY_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> ktk(duk):",srCrpKtk.srKtkRsp.chKey, HEX_KEY_SIZ);
		inQx += ASC_KEY_SIZ;

		memcpy(srCrpKtk.srKtkRsp.chKsn, pchQx + inQx, ASC_KSN_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> ksn:",srCrpKtk.srKtkRsp.chKsn, ASC_KSN_SIZ);
		
		#if defined (HRD_COD_RSP)		
		{
			LOGF_TRACE("VERSION HARDCODE");
			char chKsn [(ASC_KSN_SIZ) + 5] = {0};
			pchGetEnvVar ((char *)"CRPDUKKSN", chKsn, sizeof (chKsn), NULL);
			memcpy(srCrpKtk.srKtkRsp.chKsn, chKsn, ASC_KSN_SIZ);
			LOGAPI_HEXDUMP_TRACE("---> ksn:",srCrpKtk.srKtkRsp.chKsn, ASC_KSN_SIZ);
		}
		#endif
		
		inQx += ASC_KSN_SIZ;

		memcpy(srCrpKtk.srKtkRsp.chChk, pchQx + inQx, ASC_CHK_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> chk val:",srCrpKtk.srKtkRsp.chChk, ASC_CHK_SIZ);
		inQx += ASC_CHK_SIZ;

		memcpy(srCrpKtk.srKtkRsp.chErr, pchQx + inQx, CRP_ERR_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> err:",srCrpKtk.srKtkRsp.chErr, CRP_ERR_SIZ);
		inQx += CRP_ERR_SIZ;

		memcpy(srCrpKtk.srKtkRsp.chCrc, pchQx + inQx, ASC_CRC_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> crc:",srCrpKtk.srKtkRsp.chCrc, ASC_CRC_SIZ);
		inQx += ASC_CRC_SIZ;

		LOGF_TRACE("---> srCrpKtk.chKtk = %s",srCrpKtk.chKtk);
		LOGF_TRACE("---> srCrpKtk.srKtkRsp.chKey = %s",srCrpKtk.srKtkRsp.chKey);
		LOGF_TRACE("---> srCrpKtk.srKtkRsp.chKsn = %s",srCrpKtk.srKtkRsp.chKsn);
		
		inCrpSav((char *)CRPKTK_DAT, &srCrpKtk, sizeof(srCrpKtk));
		
	}

	memset(&srCrpKtk, 0, sizeof(srCrpKtk));
	LOGAPI_HEXDUMP_TRACE("---> pchQx:",pchQx, inQx);

	LOGF_TRACE("--- End inDsmQx ---");

	return inQx;
	
}

int inAsmQz(char pchQz[])
{
	CrpCrd srCrpCrd;
	int inQz;

	LOGF_TRACE("--- inAsmQz ---");

	inQz = 0;

	if (inCrpLod((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)  
	{
		memset(pchQz, 0, 110);
		inQz = 0;

		memcpy(pchQz + inQz, "!", 1);
		LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQz + inQz, 1);
		inQz += 1;

		memcpy(pchQz + inQz, " ", 1);
		LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQz + inQz, 1);
		inQz += 1;

		memcpy(pchQz + inQz, "EZ", 2);
		LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQz + inQz, 2);
		inQz += 2;

		memcpy(pchQz + inQz, "00098", 5);
		LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQz + inQz, 5);
		inQz += 5;

		memcpy(pchQz + inQz, " ", 1);
		LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQz + inQz, 1);
		inQz += 1;

		memcpy(pchQz + inQz, srCrpCrd.srCrpTk2.chKsn, ASC_KSN_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> ksn:",pchQz + inQz, ASC_KSN_SIZ);
		inQz += ASC_KSN_SIZ;

		sprintf(pchQz + inQz, "%7.7ld", srCrpCrd.srCrpTk2.lnNmb);
		LOGAPI_HEXDUMP_TRACE("---> crp num:",pchQz + inQz, 7);
		inQz += 7;

		sprintf(pchQz + inQz, "%2.2d", srCrpCrd.srCrpTk2.inErr);
		LOGAPI_HEXDUMP_TRACE("---> err num:",pchQz + inQz, 2);
		inQz += 2;

		sprintf(pchQz + inQz, "%1.1d", srCrpCrd.srCrpTk2.fTk2 ? 1 : 0);
		LOGAPI_HEXDUMP_TRACE("---> tk2:",pchQz + inQz, 1);
		inQz += 1;

		memcpy(pchQz + inQz, srCrpCrd.srCrpTk2.chMod, CRD_MOD_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> mod:",pchQz + inQz, CRD_MOD_SIZ);
		inQz += CRD_MOD_SIZ;

		sprintf(pchQz + inQz, "%2.2d", srCrpCrd.srCrpTk2.inTk2);
		LOGAPI_HEXDUMP_TRACE("---> siz tk2:",pchQz + inQz, 2);
		
		inQz += 2;

//		sprintf(pchQz + inQz, "%1.1d", srCrpCrd.srCrpTk2.fCv2 ? 1 : 0);		// AJM 11/04/2011 1
		//Por problemas de warnings se modifica esta instruccion
		//Modifico JVelazquez 12/08/14
		//sprintf(pchQz + inQz, "%1.1c", srCrpCrd.srCrpTk2.fCv2);

		if(srCrpCrd.srCrpTk2.fCv2 == '1')
			memcpy(pchQz + inQz, "1",1 );
		else
			memcpy(pchQz + inQz, "0",1 );

		LOGAPI_HEXDUMP_TRACE("---> cv2:",pchQz + inQz, 1);
		inQz += 1;

		sprintf(pchQz + inQz, "%2.2d", srCrpCrd.srCrpTk2.inCv2);
		LOGAPI_HEXDUMP_TRACE("---> siz cv2:",pchQz + inQz, 2);
		inQz += 2;

		sprintf(pchQz + inQz, "%1.1d", srCrpCrd.srCrpTk2.fTk1 ? 1 : 0);
		LOGAPI_HEXDUMP_TRACE("---> tk1:",pchQz + inQz, 1);
		inQz += 1;

		memcpy(pchQz + inQz, srCrpCrd.srCrpTk2.chTk2, ASC_TV2_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> tk2 + cv2:",pchQz + inQz, ASC_TV2_SIZ);
		inQz += ASC_TV2_SIZ;

		memcpy(pchQz + inQz, srCrpCrd.srCrpTk2.chLt4, ASC_LT4_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> lt4:",pchQz + inQz, ASC_LT4_SIZ);
		inQz += ASC_LT4_SIZ;

		memcpy(pchQz + inQz, srCrpCrd.srCrpTk2.chCrc, ASC_CRC_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> crc:",pchQz + inQz, ASC_CRC_SIZ);
		inQz += ASC_CRC_SIZ;
		
	}

	memset(&srCrpCrd, 0, sizeof(srCrpCrd));
	LOGAPI_HEXDUMP_TRACE("---> Qz:",pchQz, inQz);

	LOGF_TRACE("--- End inAsmQz ---");

	return inQz;
	
}

int inAsmQy(char pchQy[])
{
	CrpCrd srCrpCrd;
	int inQy;

	LOGF_TRACE("--- inAsmQy ---");

	inQy = 0;

	if (inCrpLod((char *)CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)  
	{
//		if (srCrpCrd.srCrpTk2.fTk1)
		{
			char * pchSiz;

			memset(pchQy, 0, 190);
			inQy = 0;

			memcpy(pchQy + inQy, "!", 1);
			LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQy + inQy, 1);
			inQy += 1;

			memcpy(pchQy + inQy, " ", 1);
			LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQy + inQy, 1);
			inQy += 1;

			memcpy(pchQy + inQy, "EY", 2);
			LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQy + inQy, 2);
			inQy += 2;

			//memcpy(pchQy + inQy, "00172", 5);
			pchSiz = (char *)"00000";
			if (srCrpCrd.srCrpTk2.fTk1)
			pchSiz = (char *)"00172";
			memcpy(pchQy + inQy, pchSiz, 5);
			LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQy + inQy, 5);
			inQy += 5;

			memcpy(pchQy + inQy, " ", 1);
			LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQy + inQy, 1);
			inQy += 1;

			if (srCrpCrd.srCrpTk2.fTk1)
			{
				sprintf(pchQy + inQy, "%4.4d", srCrpCrd.srCrpTk1.inTk1);
				LOGAPI_HEXDUMP_TRACE("---> siz tk1:",pchQy + inQy, 4);
				inQy += 4;

				memcpy(pchQy + inQy, srCrpCrd.srCrpTk1.chTk1, ASC_TK1_SIZ);
				LOGAPI_HEXDUMP_TRACE("---> tk1:",pchQy + inQy, ASC_TK1_SIZ);
				inQy += ASC_TK1_SIZ;

				memcpy(pchQy + inQy, srCrpCrd.srCrpTk1.chCrc, ASC_CRC_SIZ);
				LOGAPI_HEXDUMP_TRACE("---> crc:",pchQy + inQy, ASC_CRC_SIZ);
				inQy += ASC_CRC_SIZ;
			}
		}
	}

	memset(&srCrpCrd, 0, sizeof(srCrpCrd));
	LOGAPI_HEXDUMP_TRACE("---> Qy:",pchQy, inQy);

	LOGF_TRACE("--- End inAsmQy ---");

	return inQy;
	
}

int inDsmQt(char pchQt[], int in_Qt)	
{
	int inRetVal;

	LOGF_TRACE("--- inDsmQt ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchQt:",pchQt, in_Qt);

	inRetVal = VS_ESCAPE;

	if (!inGetEnvVar((char *)"DKPKEYLOD", NULL))  
	{
		LOGF_TRACE("--- End inDsmQt ---");
		return inRetVal;
	}

	inRetVal = VS_ERROR;

	if (in_Qt > 0)
	{
		CrdExc srCrdExc;
		int inQt;

		memset(&srCrdExc, 0, sizeof(srCrdExc));
		inQt = 0;

		LOGAPI_HEXDUMP_TRACE("---> eye cth:",pchQt + inQt, 1);
		inQt += 1;

		LOGAPI_HEXDUMP_TRACE("---> usr fld 1:",pchQt + inQt, 1);
		inQt += 1;

		LOGAPI_HEXDUMP_TRACE("---> tkn id:",pchQt + inQt, 2);
		inQt += 2;

		LOGAPI_HEXDUMP_TRACE("---> dta len:",pchQt + inQt, 5);
		inQt += 5;

		LOGAPI_HEXDUMP_TRACE("---> usr fld 2:",pchQt + inQt, 1);
		inQt += 1;

		memcpy(srCrdExc.chNam, pchQt + inQt, CRD_EXC_NAM);
		LOGAPI_HEXDUMP_TRACE("---> crd id:",srCrdExc.chNam, CRD_EXC_NAM);
		inQt += CRD_EXC_NAM;

		memcpy(srCrdExc.chVer, pchQt + inQt, CRD_EXC_VER);
		LOGAPI_HEXDUMP_TRACE("---> ver:",srCrdExc.chVer, CRD_EXC_VER);
		inQt += CRD_EXC_VER;

		memcpy(srCrdExc.chKsA, pchQt + inQt, ASC_KSN_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> ksn:",srCrdExc.chKsA, ASC_KSN_SIZ);
		inQt += ASC_KSN_SIZ;

		memcpy(srCrdExc.chUse, pchQt + inQt, CRD_EXC_LEN);
		LOGAPI_HEXDUMP_TRACE("---> use:",srCrdExc.chUse, CRD_EXC_LEN);
		inQt += CRD_EXC_LEN;

		memcpy(srCrdExc.chSrc, pchQt + inQt, CRD_EXC_LEN);
		LOGAPI_HEXDUMP_TRACE("---> src:",srCrdExc.chSrc, CRD_EXC_LEN);
		inQt += CRD_EXC_LEN;

		memcpy(srCrdExc.chCrp, pchQt + inQt, CRD_EXC_CRD);
		LOGAPI_HEXDUMP_TRACE("---> crp:",srCrdExc.chCrp, CRD_EXC_CRD);
		inQt += CRD_EXC_CRD;

		memcpy(srCrdExc.chCrc, pchQt + inQt, ASC_CRC_SIZ);
		LOGAPI_HEXDUMP_TRACE("---> crc:",srCrdExc.chCrc, ASC_CRC_SIZ);
		inQt += ASC_CRC_SIZ;

		inRetVal = inCrpSav((char *)CRDEXC_DAT, &srCrdExc, sizeof(srCrdExc));
		memset(&srCrdExc, 0, sizeof(srCrdExc));
		
	}

	LOGF_TRACE("---> inRetVal = %i",inRetVal); 
	LOGF_TRACE("--- End inDsmQt ---");

	return inRetVal;
	
}

int inCrpExcDsm (void)
{
	CrdExc srCrdExc;
	int inRetVal;

	LOGF_TRACE("--- inCrpExcDsm ---");

	inRetVal = CPX_ERC_SYS;

	if (inCrpLod((char *)CRDEXC_DAT, &srCrdExc, sizeof(srCrdExc)) == VS_SUCCESS)
	{
		int inUse;

		inRetVal = VS_SUCCESS;

		if (inRetVal == VS_SUCCESS)
		{
			char chAsc[ASC_KEY_SIZ];
			unsigned long ulCrc;
			char chUse[10];

			memset(chUse, 0, sizeof(chUse));
			memcpy(chUse, srCrdExc.chUse, CRD_EXC_LEN);
			inUse = atoi(chUse);
			
			LOGF_TRACE("---> inUse = %i",inUse); 
			ulCrc = ul_crc_32(srCrdExc.chCrp, inUse, 0xFFFFFFFF);
			inHexToAsc((char *) &ulCrc, chAsc, sizeof(ulCrc));
			LOGAPI_HEXDUMP_TRACE("---> crc:",srCrdExc.chCrc, sizeof(srCrdExc.chCrc));

			if (memcmp(chAsc, srCrdExc.chCrc, ASC_CRC_SIZ))
			{
				inRetVal = CPX_ERC_CRC;
			}
			
		}

		if (inRetVal == VS_SUCCESS)
		{
			if (inDkpRqtKey(srCrdExc.chKey, srCrdExc.chKsn) < VS_SUCCESS)
			{
				inRetVal = CPX_ERC_IPK;
			}
		}

		#if defined (HRD_COD_RSP)

		LOGF_TRACE("VERSION HARDCODE");
		if (inRetVal == VS_SUCCESS)		
		{
			char chCrd [CRD_EXC_CRD];
			char chCrp [CRD_EXC_CRD];
			char chSiz [10];
			int inCrd;
			int inSiz;

			inHexToAsc (srCrdExc.chKsn, srCrdExc.chKsA, HEX_KSN_SIZ);
			memset (srCrdExc.chCrd, 'F', inUse);

			#ifdef _VRXEVO
				inRetVal = put_env((char *)"CRPCRDEXC", (char *)"A3C",3); 
			#else
				inRetVal = putEnvFile((char *)"perm",(char *)"CRPCRDEXC", (char *)"A3C");
			#endif
			
			pchGetEnvVar ((char *)"CRPCRDEXC", chCrp, sizeof (chCrp), NULL);
			LOGAPI_HEXDUMP_TRACE("---> CRPCRDEXC",chCrp,sizeof (chCrp));
			
			inSiz = strlen (chCrp);
			LOGF_TRACE("---> inSiz = %i",inSiz);  
			memcpy (srCrdExc.chCrd, chCrp, inSiz);
			inCrd = inAscToHex (srCrdExc.chCrd, chCrd, inUse);
			inTrpDes (TDES2KD, srCrdExc.chKey, chCrd, inCrd, chCrp);			
			inHexToAsc (chCrp, srCrdExc.chCrp, inCrd);
			memset (chSiz, 0, sizeof (chSiz));
			sprintf (chSiz, "%4.4d", inSiz);
			LOGAPI_HEXDUMP_TRACE("---> chSiz",chSiz,sizeof(chSiz)); 
			
			memcpy (srCrdExc.chSrc, chSiz, CRD_EXC_LEN);
			LOGF_TRACE("---> srCrdExc.chSrc = %s",srCrdExc.chSrc);
			LOGF_TRACE("---> inSiz=%d", inSiz);
			
		}

		#endif

		if (inRetVal == VS_SUCCESS)
		{
			char chKsn[ASC_KSN_SIZ];

			inAscToHex(srCrdExc.chKsA, chKsn, sizeof(srCrdExc.chKsA));
			LOGAPI_HEXDUMP_TRACE("---> ksn:",srCrdExc.chKsn, sizeof(srCrdExc.chKsn));

			if (memcmp(chKsn, srCrdExc.chKsn, HEX_KSN_SIZ))
			{
				inRetVal = CPX_ERC_KSN;
			}
		}

		if (inRetVal == VS_SUCCESS)
		{
			char chCrp[CRD_EXC_CRD];
			char chCrd[CRD_EXC_CRD];
			char chSrc[10];
			int inCrp;
			int inSrc;

			inCrp = inAscToHex(srCrdExc.chCrp, chCrp, inUse);
			inTrpDes(TDES2KE, srCrdExc.chKey, chCrp, inCrp, chCrd);
			inHexToAsc(chCrd, srCrdExc.chCrd, inCrp);
			memset(chSrc, 0, sizeof(chSrc));
			memcpy(chSrc, srCrdExc.chSrc, CRD_EXC_LEN);

			LOGAPI_HEXDUMP_TRACE("---> srCrdExc.chSrc",srCrdExc.chSrc,CRD_EXC_LEN);  
			LOGAPI_HEXDUMP_TRACE("---> srCrdExc.chSrc",chSrc,CRD_EXC_LEN);	
			
			inSrc = atoi(chSrc);
			LOGF_TRACE("---> inSrc=%d", inSrc);

			if (inSrc < 1)	
			{
				inSrc = 1;
			}

			LOGF_TRACE("---> inSrc=%d", inSrc);
			LOGF_TRACE("---> srCrdExc.chCrd=%s", srCrdExc.chCrd);

			LOGF_TRACE("<<< befere CPX_ERC_BIN >>>");
			LOGF_TRACE("---> srCrdExc.chCrd[inSrc - 1] = %c",srCrdExc.chCrd[inSrc - 1]);
			LOGF_TRACE("---> inScr = %i",inSrc);  
			
			if (srCrdExc.chCrd[inSrc - 1] != 'C')
			{
				inRetVal = CPX_ERC_BIN;
				LOGF_TRACE("<<< AQUI FALLA !!!!! >>>");
			}
			
		}

		if (inRetVal == VS_SUCCESS)
		{
			inRetVal = inCrpSav((char *)CRDEXC_DAT, &srCrdExc, sizeof(srCrdExc));

			if (inRetVal < VS_SUCCESS)
			{
				inRetVal = CPX_ERC_SYS;
			}
			
		}
		
	}

	memset(&srCrdExc, 0, sizeof(srCrdExc));
	LOGF_TRACE("---> inRetVal=%d", inRetVal);

	LOGF_TRACE("--- End inCrpExcDsm ---");

	return inRetVal;
	
}

int inCrpExcSav (void)	
{
	CrdExc srCrdExc;
	int inRetVal;

	LOGF_TRACE("--- inCrpExcSav ---");

	inRetVal = CPX_ERC_SYS;

	if (inCrpLod((char *)CRDEXC_DAT, &srCrdExc, sizeof(srCrdExc)) == VS_SUCCESS)
	{
		char szTmp[20];
		int inTmp;

		inRetVal = VS_SUCCESS;

		if (inRetVal == VS_SUCCESS)
		{
			inSetEnvVarpch((char *)"BINPNP", (char *)"00000000", NULL);
			inSetEnvVarpch((char *)"BINVER", (char *)"00", NULL);

			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chSrc, CRD_EXC_LEN);
			inTmp = atoi(szTmp);
			inRetVal = inCrpSavExc(srCrdExc.chCrd, inTmp);

			LOGF_TRACE("---> inRetVal=%d", inRetVal);
		}

		if (inRetVal == VS_SUCCESS)
		{
			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chNam, CRD_EXC_NAM);
			inSetEnvVarpch((char *)"BINPNP", szTmp, NULL);
			LOGF_TRACE("---> NAME APP: %s",szTmp);

			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chVer, CRD_EXC_VER);
			inSetEnvVarpch((char *)"BINVER", szTmp, NULL);
			LOGF_TRACE("---> VER APP: %s",szTmp);	
		}
		
	}

	memset(&srCrdExc, 0, sizeof(srCrdExc));

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpExcSav ---");

	return inRetVal;
	
}

int inCrdExc (void)		
{
	char ch_exc [512];
	int  in_exc ;
	int  in_hnd ;
	int  in_val ;

	LOGF_TRACE("--- inCrdExc ---");

	in_hnd = -1;

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		if (!inGetEnvVar((char *)"CRDEXCAMX", NULL))
		{
			in_val = VS_ESCAPE;
		}
	}

	if (in_val > FAILURE)
	{
		in_hnd = in_open ("crdexc.txt", O_RDONLY);

		if (in_hnd < 0)
		{
			in_val = FAILURE;
		}
	}

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = in_read (in_hnd, ch_exc, sizeof (ch_exc));

		if (in_ret < 0)
		{
			in_val = FAILURE;
		}
		else
		{
			in_exc = in_ret;
		}
	}

	if (in_val > FAILURE)
	{
		in_val = inCrpSavExc (ch_exc, in_exc);
	}

	if (in_hnd > -1)
	{
		in_close (in_hnd);
	}

	LOGF_TRACE("---> in_val=%d", in_val);
	LOGF_TRACE("--- End inCrdExc ---");

	return in_val;
	
}

int inCrpSavExc(char pchCrd[], int inCrd)
{
	int inVal;
	int inHdl;
	int inIdx;
	ExcCrd srExcCrd;
	char *pchEnd;
	int  *pinEnd;
	char excCard[1000];
	int exCard;

	LOGF_TRACE("--- inCrpSavExc ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchCrd:",pchCrd, inCrd);

	inHdl = in_open(EXCCRD_DAT, O_CREAT | O_TRUNC | O_RDWR);
	LOGF_TRACE("---> inHdl = in_open(EXCCRD_DAT, O_CREAT | O_TRUNC | O_RDWR) = %i",inHdl); 
	memset(excCard,0,sizeof(excCard));		
	LOGF_TRACE("---> EXCCRD_DAT (Exception Cards): %s",EXCCRD_DAT);  
	exCard = read(inHdl,excCard,1000);
	
	LOGF_TRACE("---> exCard=%d", exCard);
	LOGF_TRACE("---> inHdl=%d", inHdl);

	if (inHdl < 0)
	{
		LOGF_TRACE("--- End inCrpSavExc ---");
		return CPX_ERC_SYS;
	}

	memset(&srExcCrd, 0, sizeof(srExcCrd));
	pchEnd =  srExcCrd.chMin;
	pinEnd = &srExcCrd.inMin;

	inIdx = 0;
	
	while (inIdx < inCrd)
	{
		char chChr;

		chChr = pchCrd[inIdx];
		LOGF_TRACE("---> chChr=%c",chChr);
		
		switch (chChr)
		{
			case 'A':
				memset(&srExcCrd, 0, sizeof(srExcCrd));
				pchEnd =  srExcCrd.chMin;
				pinEnd = &srExcCrd.inMin;
				break;

			case 'B':
				pchEnd =  srExcCrd.chMax;
				pinEnd = &srExcCrd.inMax;
				break;

			case 'C':
				LOGAPI_HEXDUMP_TRACE("---> min:",srExcCrd.chMin, srExcCrd.inMin);
				LOGAPI_HEXDUMP_TRACE("---> max:",srExcCrd.chMax, srExcCrd.inMax);
				
				if (srExcCrd.inMax < 1)
				{
					memcpy(srExcCrd.chMax, srExcCrd.chMin, srExcCrd.inMin);
					srExcCrd.inMax = srExcCrd.inMin;
					LOGAPI_HEXDUMP_TRACE("---> max:",srExcCrd.chMax, srExcCrd.inMax);
				}

				#ifdef _VRXEVO
					inVal = crypto_write(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
				#else
					inVal = cryptoWrite(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
				#endif

				memset(excCard,0,sizeof(excCard));		
				exCard = read(inHdl,excCard,1000);		

				LOGF_TRACE("---> inVal=%d", inVal);
				memset(&srExcCrd, 0, sizeof(srExcCrd));
				break;

			case 'F':
				inIdx = inCrd;
				break;

			default:
				pchEnd[*pinEnd] = chChr;
				(*pinEnd)++;
				break;
				
		}

		inIdx++;
		
	}

	inVal = in_close(inHdl);
	LOGF_TRACE("---> inVal=%d", inVal);
		
	inVal = SUCCESS;
	if (inIdx < inCrd)
	{
		LOGF_TRACE("<<< SE PRENDIO BIT !!!!! >>>");
		inVal = CPX_ERC_BIN;
	}

	LOGF_TRACE("---> inVal=%d", inVal);
	LOGF_TRACE("--- End inCrpSavExc ---");

	return inVal;
	
}

static int inCrpCrdExc (char pchAct[], int inAct)	
{
	ExcCrd srExcCrd;
	int inRetVal;
	int inHdl;
	
	LOGF_TRACE("--- inCrpCrdExc ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchAct:",pchAct, inAct);

	inRetVal = VS_FAILURE;

	inHdl = in_open(EXCCRD_DAT , O_RDONLY);
	LOGF_TRACE("---> inHdl = in_open(EXCCRD_DAT , O_RDONLY); = %i",inHdl); 
	
	if (inHdl > -1)
	{
		inRetVal = VS_ERROR;

		while (inRetVal == VS_ERROR)
		{
			int inMin;
			int inMax;
			int inChr;

			memset(&srExcCrd, 0, sizeof(srExcCrd));

			#ifdef _VRXEVO
				inChr = crypto_read(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
			#else
				inChr = cryptoRead(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
			#endif
			
			LOGF_TRACE("<<< After of crypto read >>>");   
			LOGF_TRACE("---> inChr=%d", inChr);
			
			if (inChr < sizeof(srExcCrd))
			{
				break;
			}

			LOGAPI_HEXDUMP_TRACE("---> (srExcCrd.chMax) min:",srExcCrd.chMin, srExcCrd.inMin);
			LOGAPI_HEXDUMP_TRACE("---> (pchAct) min:",pchAct, inAct);
			LOGAPI_HEXDUMP_TRACE("---> (srExcCrd.chMax) max:",srExcCrd.chMax, srExcCrd.inMax);			
			LOGAPI_HEXDUMP_TRACE("---> (pchAct) max:",pchAct, inAct);

			inMin = strncmp(pchAct, srExcCrd.chMin, srExcCrd.inMin);
			inMax = strncmp(pchAct, srExcCrd.chMax, srExcCrd.inMax);

			if ((inMin > -1) && (inMax < 1))
			{
				inRetVal = VS_SUCCESS;
			}
			
		}

		in_close(inHdl);
		
	}

	LOGF_TRACE("---> inRetVal=%d", inRetVal);
	LOGF_TRACE("--- End inCrpCrdExc ---");

	return inRetVal;
}

static int inCrpExcAsm (char pchAct[], int inAct)	
{
	ExcCrd srExcCrd;
	int inRetVal;
	int inHdl;
	char chVal [20];
	int inIdx = 0;
	
	LOGF_TRACE("--- inCrpExcAsm ---");   

	LOGAPI_HEXDUMP_TRACE("---> pchAct:",pchAct, inAct);

	inRetVal = VS_FAILURE;
	inHdl = in_open(EXCCRD_DAT , O_RDONLY);
	LOGF_TRACE("---> inHdl = in_open(EXCCRD_DAT , O_RDONLY); = %i",inHdl);  

	if (inHdl > -1)
	{
		inRetVal = VS_ERROR;

		while (inRetVal == VS_ERROR)
		{
			int inMin;
			int inMax;
			int inChr;

			memset(&srExcCrd, 0, sizeof(srExcCrd));

			#ifdef _VRXEVO
				inChr = crypto_read(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
			#else
				inChr = cryptoRead(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
			#endif
			
			LOGF_TRACE("<<< After of crypto read >>>");  
	
			if (inChr < sizeof(srExcCrd))
			{
				LOGF_TRACE("<<< After of crypto read >>>"); 
				break;
			}

			LOGAPI_HEXDUMP_TRACE("---> (srExcCrd.chMax) min:",srExcCrd.chMin, srExcCrd.inMin);
			LOGAPI_HEXDUMP_TRACE("---> (pchAct) min:",pchAct, inAct);
			
			memset (chVal, 0, sizeof (chVal));
			memcpy (chVal, srExcCrd.chMin, srExcCrd.inMin);
			pad (pchAct + inIdx, chVal, ' ', 8, RIGHT_);
			LOGAPI_HEXDUMP_TRACE("---> min:",pchAct + inIdx, 8);
			inIdx += 8;
			
			LOGAPI_HEXDUMP_TRACE("---> (srExcCrd.chMax) max:",srExcCrd.chMax, srExcCrd.inMax);			
			LOGAPI_HEXDUMP_TRACE("---> (pchAct) max:",pchAct, inAct);
			
			memset (chVal, 0, sizeof (chVal));
			memcpy (chVal, srExcCrd.chMax, srExcCrd.inMax);
			pad (pchAct + inIdx, chVal, ' ', 8, RIGHT_);
			LOGAPI_HEXDUMP_TRACE("---> max:",pchAct + inIdx, 8);
			inIdx += 8;
			
		}

		in_close(inHdl);
		
	}

	LOGF_TRACE("--- End inCrpExcAsm ---"); 

	return inIdx;
	
}

DllSPEC int inCrpExcCrd (char pch_act[], int in_act)		
{
	int in_val;

	LOGF_TRACE("--- inCrpExcCrd ---"); 

	LOGAPI_HEXDUMP_TRACE("---> act:",pch_act, in_act);

	in_val = VS_ERROR;

	if (in_val < VS_SUCCESS)
	{
		if (in_act == inGetEnvVar ((char *)"CRDEXCMIN", NULL))
		{
			in_val = VS_SUCCESS;
		}
	}

	if (in_val < VS_SUCCESS)
	{
		in_val = inCrpCrdExc (pch_act, in_act);
	}

	LOGF_TRACE("---> in_val=%d", in_val);
	LOGF_TRACE("--- End inCrpExcCrd ---");

	return in_val;
	
}

int inNewRsaBgn(int inChr)
{
	int inRetVal;

	LOGF_TRACE("--- inNewRsaBgn ---");
	LOGF_TRACE("---> inChr=%d",inChr);
		
	inRetVal = VS_ERROR;

	if (inChr > 0)
	{
		int inHdl;

		inRetVal = VS_FAILURE;

		inHdl = in_open(NEWRSA_DAT, O_CREAT | O_TRUNC);
		LOGF_TRACE("---> inHdl=%d",inHdl);

		if (inHdl > -1)
		{
			inRetVal = VS_SUCCESS;
			inSetEnvVarin((char *)"NEWRSACHR", inChr, NULL);
			in_close(inHdl);
		}
	}

	LOGF_TRACE("---> inRetVal=%d",inRetVal);
	LOGF_TRACE("--- End inNewRsaBgn ---");
	
	return inRetVal;
	
}

int inNewRsaWrt(char pchChr[], int inChr)
{
	int inRetVal;

	LOGF_TRACE("--- inNewRsaWrt ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchChr:",pchChr, inChr);
	
	inRetVal = VS_ERROR;

	if (inChr > 0)
	{
		int inHdl;

		inRetVal = VS_FAILURE;

		inHdl = in_open(NEWRSA_DAT, O_WRONLY | O_APPEND);
		LOGF_TRACE("---> inHdl=%d", inHdl);

		if (inHdl > -1)
		{
			int inVal;

			inVal = in_write(inHdl, pchChr, inChr);
			LOGF_TRACE("---> inVal=%d", inVal);

			if (inVal == inChr)
			{
				inRetVal = VS_SUCCESS;
			}

			in_close(inHdl);
		}
	}

	LOGF_TRACE("---> inRetVal=%d",inRetVal);
	LOGF_TRACE("--- End inNewRsaWrt ---");
	
	return inRetVal;
	
}
//Esto se comenta de manera temporal, porque de momento NO se encuentran funcionamiento a esta funcion
//Comento JVelazquez 11/08/14 (Pendiente)
/*
int inNewRsaEnd(void)
{
	int inRetVal;
	int inChrBgn;
	int inChrEnd;

	SAVE_FUNC_NAME("inNewRsaEnd")

	pdebug((FUNC_NAME_TITL));

	inChrBgn = inGetEnvVar("#NEWRSACHR", NULL);

	pdebug(("inChrBgn=%ld", inChrBgn));

	inChrEnd = ln_dir_get_file_sz("NewRsa.dat");

	pdebug(("inChrEnd=%ld", inChrEnd));

	inRetVal = VS_ERROR;

	if (inChrEnd == inChrBgn)
	{
		inRetVal = VS_SUCCESS;
	}

	pdebug(("inRetVal=%d", inRetVal));

	return inRetVal;
}

int inRsaSgnSav (void)		// AJM 08/08/2010 1
{
	int inRetVal;
	char chVer[20];
	char chRsa[1024];
	char chSgn[1024];
	int inVer;
	int inRsa;
	int inSgn;

	SAVE_FUNC_NAME("inRsaSgnSav")

	pdebug((FUNC_NAME_TITL));

	inRetVal = VS_SUCCESS;

	if (inRetVal == VS_SUCCESS)
	{
		inVer = sizeof(chVer);

		inRsa = sizeof(chRsa);

		inSgn = sizeof(chSgn);

		inRetVal = inRsaSgnLod (chVer, &inVer, chRsa, &inRsa, chSgn, &inSgn);

		pdebug(("inRetVal=%d", inRetVal));
	}

	if (inRetVal == VS_SUCCESS)
	{
		inRetVal = inRsaVldSgn ("RsaSgn.dat", chRsa, inRsa, chSgn, inSgn);		// AJM 09/02/2011 1

		pdebug(("inRetVal=%d", inRetVal));
	}

	if (inRetVal == VS_SUCCESS)
	{
		inRetVal = inCrpRsaSav (chVer, inVer, chRsa, inRsa);

		pdebug(("inRetVal=%d", inRetVal));
	}

	pdebug(("inRetVal=%d", inRetVal));

	return inRetVal;
}

int inRsaSgnLod (char pchVer[], int *pinVer, char pchRsa[], int *pinRsa, char pchSgn[], int *pinSgn)
{
	int inRetVal;
	int inHdl;
	char chNam[20];

	SAVE_FUNC_NAME("inRsaSgnLod")

	pdebug((FUNC_NAME_TITL));

	memset(chNam, 0, sizeof(chNam));
	inRetVal = inRsaLodNam (chNam);
	pdebug (("inRetVal=%d", inRetVal));
	if (inRetVal < VS_SUCCESS)
	{
		return inRetVal;
	}
	purge_char (chNam, ' ');

	memset (pchVer, 0, *pinVer);
	memset (pchRsa, 0, *pinRsa);
	memset (pchSgn, 0, *pinSgn);

	inRetVal = VS_FAILURE;

	inHdl = in_open ("NewRsa.dat", O_RDONLY);
	pdebug(("inHdl=%d", inHdl));
	if (inHdl > -1)
	{
		char chChr[650];
		int inChr;
		CrpSgn srCrpSgn;
		ChrSiz srChrSiz;
		VS_BOOL fFnd;

		memset(&srCrpSgn, 0, sizeof(srCrpSgn));
		memset(&srChrSiz, 0, sizeof(srChrSiz));

		fFnd = VS_FALSE;

		inChr = 0;

		while (!fFnd)
		{
			int inVal;

			inVal = read (inHdl, chChr + inChr, 1);
//			pdebug (("inVal=%d", inVal));
			if (inVal < 1)
			{
				break;
			}
//			dump (chChr + inChr, inVal, "chr:");
			switch (chChr[inChr])
			{
				case ':':
				dump (chChr, inChr, "chr:");
				chChr[inChr] = 0;
				memset(&srChrSiz, 0, sizeof(srChrSiz));
				if (!strcmp(chChr, "BIN"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chBin;
					srChrSiz.pinChr = &srCrpSgn.inBin;
		     	}
				else if (!strcmp(chChr, "ID_comercio"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chMrc;
					srChrSiz.pinChr = &srCrpSgn.inMrc;
		     	}
				else if (!strcmp(chChr, "Version_Llave"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chVer;
					srChrSiz.pinChr = &srCrpSgn.inVer;
		     	}
				else if (!strcmp(chChr, "Llave_RSA"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chKey;
					srChrSiz.pinChr = &srCrpSgn.inKey;
		     	}
				else if (!strcmp(chChr, "Nombre_Llave_Firma"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chNam;
					srChrSiz.pinChr = &srCrpSgn.inNam;
		     	}
				else if (!strcmp(chChr, "Firma"))
		     	{
					srChrSiz.pchChr =  srCrpSgn.chSgn;
					srChrSiz.pinChr = &srCrpSgn.inSgn;
		     	}
				inChr = 0;
				break;

				case ' ':
				case '[':
				inChr = 0;
				break;

				case ']':
				if (srChrSiz.pchChr)		// AJM 15/09/2010 1
				{
					memcpy (srChrSiz.pchChr, chChr, inChr);
					*srChrSiz.pinChr = inChr;
				}
				dump (srChrSiz.pchChr, *srChrSiz.pinChr, "chr:");
				if (srChrSiz.pchChr == srCrpSgn.chSgn)
				{
					dump (srCrpSgn.chNam, RSA_NAM_SIZ, "nam rcv:");
					dump (         chNam, RSA_NAM_SIZ, "nam sav:");
					if (!memcmp(srCrpSgn.chNam, chNam, RSA_NAM_SIZ))
					{
						fFnd = VS_TRUE;
					}
				}
				memset(&srChrSiz, 0, sizeof(srChrSiz));
				inChr = 0;
				break;

				case 0xA:
				case 0xD:
				memset(&srChrSiz, 0, sizeof(srChrSiz));
				inChr = 0;
				break;

				default:
				inChr++;
				break;
			}
		}

		in_close (inHdl);

		if (fFnd)
		{
			char chVer[ASC_VER_SIZ];
			int inVer;

			memset(chVer, 0, sizeof(chVer));
			inVer = 0;
			memcpy (chVer + inVer, srCrpSgn.chMrc, srCrpSgn.inMrc);
			inVer += srCrpSgn.inMrc;
			memcpy (chVer + inVer, srCrpSgn.chVer, srCrpSgn.inVer);
			inVer += srCrpSgn.inVer;

			memcpy (pchVer,          chVer,          inVer);
			memcpy (pchRsa, srCrpSgn.chKey, srCrpSgn.inKey);
			memcpy (pchSgn, srCrpSgn.chSgn, srCrpSgn.inSgn);

			*pinVer =          inVer;
			*pinRsa = srCrpSgn.inKey;
			*pinSgn = srCrpSgn.inSgn;

			dump (pchVer, *pinVer, "ver:");
			dump (pchRsa, *pinRsa, "rsa:");
			dump (pchSgn, *pinSgn, "sgn:");

			inRetVal = VS_SUCCESS;
		}
	}

	pdebug (("inRetVal=%d", inRetVal));

	return inRetVal;
}

int inRsaVldSgn (char pchRSA [], char pchVld[], int inVld, char pchSgn[], int inSgn)		// AJM 09/02/2011 1
{
	int inRetVal;
	rsa_context srRsa;

	SAVE_FUNC_NAME("inRsaVldSgn")

	pdebug((FUNC_NAME_TITL));

	//dump(pchVld, inVld, "vld");
	//dump(pchSgn, inSgn, "sgn");

	inRetVal = VS_SUCCESS;

	if (inRetVal == VS_SUCCESS)
	{
		KtkRqt srVer;

		memset(&srVer, 0, sizeof(srVer));
		inRetVal = inCrpRsaLod(pchRSA, &srVer, &srRsa, TRUE);
		pdebug(("inRetVal=%d", inRetVal));
	}

	if (inRetVal == VS_SUCCESS)
	{
		inRetVal = rsa_check_pubkey(&srRsa);
		pdebug(("inRetVal=0x%04.4x", inRetVal));
	}

	if (inRetVal == VS_SUCCESS)
	{
		char chHex[ASC_MOD_SIZ];
		int inHex;
		char chSha[40];

		memset(chHex, 0, sizeof(chHex));
		inHex = inAscToHex(pchVld, chHex, inVld);
		memset(chSha, 0, sizeof(chSha));
		SHA1("", (unsigned char *) chHex, inHex, (unsigned char *) chSha);
		//dump(chSha, SHA_256_SIZ, "sha:");

		memset(chHex, 0, sizeof(chHex));
		inHex = inAscToHex(pchSgn, chHex, inSgn);
		inRetVal = rsa_pkcs1_verify(&srRsa, RSA_PUBLIC, SIG_RSA_SHA1, 20, (unsigned char *) chSha, (unsigned char *) chHex);
		pdebug(("inRetVal=0x%04.4x", inRetVal));
	}

	rsa_free(&srRsa);

	pdebug(("inRetVal=%d", inRetVal));

	return inRetVal;
}

*/
int inCrpRsaSav (char pchVer[], int inVer, char pchRsa[], int inRsa)
{
	CrpRsa srCrpRsa;
	int inVal;

	LOGF_TRACE("--- inCrpRsaSav ---");

	LOGAPI_HEXDUMP_TRACE("---> ver:",pchVer, inVer);
	LOGAPI_HEXDUMP_TRACE("---> rsa:",pchRsa, inRsa);

	memset(&srCrpRsa, 0, sizeof (srCrpRsa));
	inVal = VS_SUCCESS;

	if (inVal == VS_SUCCESS)
	{
		inVal = inCrpRsaDsm (&srCrpRsa, pchRsa, inRsa);
		LOGF_TRACE("---> inVal=%d",inVal);
	}

	if (inVal == VS_SUCCESS)
	{
		inVal = inCrpRsaAsm (&srCrpRsa, pchVer, inVer);
		LOGF_TRACE("---> inVal=%d",inVal);
	}

	if (inVal == VS_SUCCESS)
	{
		inVal = inCrpRsaWrt (&srCrpRsa);
		LOGF_TRACE("---> inVal=%d",inVal);
	}

	LOGF_TRACE("---> inVal=%d",inVal);
	LOGF_TRACE("--- End inCrpRsaSav ---");

	return inVal;
	
}

int inCrpRsaDsm (CrpRsa *psrCrpRsa, char pchRsa[], int inRsa)
{
	int inVal;
	int inIdx;
	char chHex[ASC_MOD_SIZ];
	int inHex;

	LOGF_TRACE("--- inCrpRsaDsm ---");

	inHex = inAscToHex (pchRsa, chHex, inRsa);
	inVal = VS_SUCCESS;
	inIdx = 0;
	
	while (inIdx < inHex)
	{
		int inSeq;
		unsigned int uiSiz;

		inSeq = (unsigned char) chHex[inIdx++];
		LOGF_TRACE("---> inSeq=0x%2.2x", inSeq);

		if (inSeq == 0x30 || inSeq == 0x02)
		{
			int inSiz;
			int inChr;

			inChr = (unsigned char) chHex[inIdx++];
			LOGF_TRACE("---> inChr=0x%2.2x",inChr);
			
			inSiz = inChr & 0x7F;
			LOGF_TRACE("---> inSiz=%d", inSiz);
				
			if (inChr & 0x80)
			{
				unsigned int uiHgh;
				unsigned int uiLow;

				if (inSiz > 2)
				{
					inVal = VS_ERROR;
					break;
				}

				uiHgh = (unsigned char) chHex[inIdx++];
				uiLow = (unsigned char) chHex[inIdx++];
				uiHgh <<= 8;
				uiSiz = uiHgh | uiLow;
				
				LOGF_TRACE("---> uiSiz=0x%4.4x", uiSiz);
					
				if (inSeq == 0x02)
				{
					if (psrCrpRsa->inMod < 1)
					{
						psrCrpRsa->inMod = inHexToAsc (chHex + inIdx, psrCrpRsa->chMod, uiSiz);
						LOGAPI_HEXDUMP_TRACE("---> mod:",psrCrpRsa->chMod, psrCrpRsa->inMod);
					}
				}
			}
			else
			{
				uiSiz = inSiz;
				LOGF_TRACE("---> uiSiz=0x%4.4x",uiSiz);
					
				if (inSeq == 0x02)
				{
					if (psrCrpRsa->inExp < 1)
					{
						psrCrpRsa->inExp = inHexToAsc (chHex + inIdx, psrCrpRsa->chExp, uiSiz);
						LOGAPI_HEXDUMP_TRACE("---> exp:",psrCrpRsa->chExp, psrCrpRsa->inExp);
					}
				}
			}

			LOGAPI_HEXDUMP_TRACE("---> seq:",chHex + inIdx, uiSiz);

			if (inSeq == 0x02)
			{
				inIdx += uiSiz;
				LOGF_TRACE("---> inIdx=%d", inIdx);

				if (inIdx > inHex)
				{
					inVal = VS_ERROR;
					break;
				}
			}
		}
	}

	LOGF_TRACE("---> inVal=%d", inVal);
	LOGF_TRACE("--- End inCrpRsaDsm ---");

	return inVal;
	
}

int inCrpRsaAsm (CrpRsa *psrCrpRsa, char pchVer[], int inVer)
{
	int inVal;

	LOGF_TRACE("--- inCrpRsaAsm ---");
	
	LOGAPI_HEXDUMP_TRACE("---> ver:",pchVer, inVer);
	
	inVal = VS_SUCCESS;
	pad (pchVer, pchVer, ' ', ASC_VER_SIZ, LEFT_);
	pchVer[ASC_VER_SIZ] = 0;
	memcpy (psrCrpRsa->chVer, pchVer, ASC_VER_SIZ);
	psrCrpRsa->inVer = ASC_VER_SIZ;
	LOGAPI_HEXDUMP_TRACE("---> ver:",psrCrpRsa->chVer, psrCrpRsa->inVer);

	memcpy (psrCrpRsa->chPad, "01", 2);
	psrCrpRsa->inPad = 2;
	LOGAPI_HEXDUMP_TRACE("---> pad:",psrCrpRsa->chPad, psrCrpRsa->inPad);

	LOGF_TRACE("---> inVal=%d", inVal);
	LOGF_TRACE("--- End inCrpRsaAsm ---");

	return inVal;
	
}

int inCrpRsaWrt (CrpRsa *psrCrpRsa)
{
	int inVal;
	int inHdl;

	LOGF_TRACE("--- inCrpRsaWrt ---");

	inVal = VS_FAILURE;
	inHdl = in_open(CRPRSA, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND);
	LOGF_TRACE("---> inHdl=%d", inHdl);

	if (inHdl > -1)
	{
		char szWrt[20];
		int inWrt;

		inWrt = 1;

		if (inWrt > 0)
		{
			//inWrt = sprintf(szWrt, "%03.3d", psrCrpRsa->inMod >> 1);
			inWrt = sprintf(szWrt, "%3.3d", psrCrpRsa->inMod >> 1);
			inWrt = in_write(inHdl, szWrt, inWrt);
			LOGF_TRACE("---> inWrt=%d",inWrt);
		}

		if (inWrt > 0)
		{
			inWrt = in_write(inHdl, psrCrpRsa->chMod, psrCrpRsa->inMod);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			//inWrt = sprintf(szWrt, "%02.2d", psrCrpRsa->inExp >> 1);
			inWrt = sprintf(szWrt, "%2.2d", psrCrpRsa->inExp >> 1);
			inWrt = in_write(inHdl, szWrt, inWrt);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			inWrt = in_write(inHdl, psrCrpRsa->chExp, psrCrpRsa->inExp);
			LOGF_TRACE("---> inWrt=%d",inWrt);
		}

		if (inWrt > 0)
		{
			//inWrt = sprintf(szWrt, "%02.2d", psrCrpRsa->inVer);
			inWrt = sprintf(szWrt, "%2.2d", psrCrpRsa->inVer);
			inWrt = in_write(inHdl, szWrt, inWrt);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			inWrt = in_write(inHdl, psrCrpRsa->chVer, psrCrpRsa->inVer);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			//inWrt = sprintf(szWrt, "%02.2d", psrCrpRsa->inPad >> 1);
			inWrt = sprintf(szWrt, "%2.2d", psrCrpRsa->inPad >> 1);
			inWrt = in_write(inHdl, szWrt, inWrt);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			inWrt = in_write(inHdl, psrCrpRsa->chPad, psrCrpRsa->inPad);
			LOGF_TRACE("---> inWrt=%d", inWrt);
		}

		if (inWrt > 0)
		{
			inVal = VS_SUCCESS;
		}

		inWrt = in_close(inHdl);
		LOGF_TRACE("---> inWrt=%d", inWrt);

		if (inWrt < VS_SUCCESS)
		{
			inVal = VS_FAILURE;
		}
	}

	LOGF_TRACE("---> inVal=%d", inVal);
	LOGF_TRACE("--- End inCrpRsaWrt ---");
		
	return inVal;
	
}

int inRsaLodNam (char pchNam[])
{
	int inRetVal;
	rsa_context srRsa;
	KtkRqt srVer;

	LOGF_TRACE("--- inRsaLodNam ---");

	memset(&srVer, 0, sizeof(srVer));
	inRetVal = inCrpRsaLod((char *)RSASGN_DAT, &srVer, &srRsa, TRUE);
	LOGF_TRACE("---> inRetVal=%d", inRetVal);

	if (inRetVal == VS_SUCCESS)
	{
		memcpy(pchNam, srVer.chVer, ASC_VER_SIZ);
	}

	rsa_free(&srRsa);
	LOGF_TRACE("---> inRetVal=%d",inRetVal);
	LOGF_TRACE("--- End inRsaLodNam ---");
		
	return inRetVal;
	
}

int inCrpDukErr (int in_ath, int bo_ery)
{
	int inVal;

	LOGF_TRACE("--- inCrpDukErr ---");
	LOGF_TRACE("---> in_ath = %i",in_ath);	
	LOGF_TRACE("---> bo_ery = %i",bo_ery);	
	LOGF_TRACE("---> in_ath=%d bo_ery=%d", in_ath, bo_ery);
	
	inVal = VS_ESCAPE;

	if (in_ath < FAILED_TO_CONNECT)
	{
		int inErr;

		inErr = 0;

		if (in_ath == HOST_DECLINED)
		{
			if (bo_ery)
			{
				inErr = inGetEnvVar((char *)"DKPERR", NULL) + 1;
			}
		}

		inSetEnvVarin((char *)"DKPERR", inErr, NULL);
		inVal = inErr;
	}

	LOGF_TRACE("---> inVal=%d", inVal);
	LOGF_TRACE("--- End inCrpDukErr ---");

	return inVal;
	
}

//-----------------------------------------------------------------------------
//!  \brief     Realiza padding a una cadena
//-----------------------------------------------------------------------------
DllSPEC extern  int pad(char *pdest_buf, char *psrc_buf, char pad_char, int pad_size, int align)
{
	int ch_left, ch_right;
	   int num_pad;
	   char *d_ptr, *s_ptr;

	   /* pad _size cannot be negative */
	   if (pad_size < 0)
	      pad_size = 0;

		/* determine how many characters to add */
	    /* ensure we need to add characters */
	   if ( 0 > (num_pad = pad_size - (int) strlen (psrc_buf)))
	      num_pad = 0;


			 /* the source and destination buffer may be the same
			 *  buffer.  if they are different, copy the source
			 *  to the destination and do not molest the source
			 *
			 *	02/18/92  jwm
			 */

	   if ( psrc_buf != pdest_buf)
	   {
	         /* initialized destination and copy source
			 *  2/18/92 jwm
			 */
	      memcpy (pdest_buf, psrc_buf, strlen (psrc_buf)+1);
	   }

	         /* determine the number of characters to pad on */
	         /* each end.                                    */

	   switch (align)
	   {

	      case  RIGHT_:
	      {
	         ch_left = num_pad;
	         ch_right = 0;
	         break;
	      }

	      case CENTER_:
	      {
	         ch_left = num_pad / 2;
	         ch_right = num_pad - ch_left;
	         break;
	      }

	      case LEFT_:
	      default:
	      {
	         ch_left = 0;
	         ch_right = num_pad;
	         break;
	      }
	   }
	         /* pad the front of the string */

	   if (ch_left)
	   {
	       s_ptr = psrc_buf + strlen(psrc_buf);
	       d_ptr = pdest_buf + strlen(psrc_buf) + ch_left;
	       while ( psrc_buf <= s_ptr)     /* copy string to destination */
	          *d_ptr-- = *s_ptr--;
	       while (ch_left--)			  /* add pad characters before string */
	          *d_ptr-- = pad_char;
	   }

	         /* pad the end of the string */

	   while (ch_right --)
	   {
	      append_char (pdest_buf, pad_char);
	   }

	   return (num_pad);
	   
}

//-----------------------------------------------------------------------------
//!  \brief     Realiza padding a una cadena
//-----------------------------------------------------------------------------
extern int append_char (char *string, char c)
{
    int i;

        /* get the current length of the string, this is the
            position for the new character.
        */
    i = strlen (string);

        /* now place the passed character at the end of the
            string.  since the length is the number of characters
            in the string, then the pointer plus the number of characters
            is the current NULL position.  The user may pass an empty
            string.  In this case, the apended character will be in the
            first position.
        */
    *(string + i++) = c;
        /* Now add a NULL after the newly appended character. */

	if (c != 0)
	*(string + i) = 0x00;

	/* i is the position of the NULL which is also the new string
    length.  Return i.
    */
    return (strlen (string));
}


int inAskBinTable(char *chBinId,char *chBinVer, char *chBinRange)
{
	char szTmp0[50];
	int inBinlen=0;
	
	LOGF_TRACE("--- inAskBinTable ---"); 

	pchGetEnvVar((char *)"BINPNP", szTmp0, 10, NULL);
	pad(szTmp0, szTmp0, ' ', CRD_EXC_NAM, RIGHT_);
	memcpy(chBinId, szTmp0, CRD_EXC_NAM);
	LOGAPI_HEXDUMP_TRACE("---> BINPNP:",chBinId, CRD_EXC_NAM);
	
	pchGetEnvVar((char *)"BINVER", szTmp0, 5, NULL);
	pad(szTmp0, szTmp0, ' ', CRD_EXC_VER, RIGHT_);
	memcpy(chBinVer, szTmp0, CRD_EXC_VER);
	LOGAPI_HEXDUMP_TRACE("---> BINVER:",chBinVer, CRD_EXC_VER);
	inBinlen = inCrpExcAsm(chBinRange,inGetEnvVar((char *)"CRDEXCMIN", NULL));

	LOGF_TRACE("--- End inAskBinTable ---"); 

	return inBinlen;
	
}

#endif //MAKE_DUKPT
