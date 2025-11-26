//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
// AJM 14/02/2010 1
#include "crpdkp.h"

#ifdef _VRXEVO
#include <svc_sec.h>
#else
#include "svcsec.h"
#endif
//Este archivo hace referencia a las variables que se agregan por homologacion
//Agrego JVelazquez 07/08/14
#include "dukpt_defs.h"
//#include "debug.h" //ACM  28/10/2015
#include <log/liblog.h>

#ifdef _VRXEVO
#define DKPENV_DAT "I:1/DkpEnv.dat"
#define CRPCRD_DAT "I:1/CrpCrd.dat"
#else
#define DKPENV_DAT "/home/usr1/flash/DkpEnv.dat"
#define CRPCRD_DAT "/home/usr1/flash/CrpCrd.dat"
#endif

int inDkpLclLbl(DkpEnv *psrDkpEnv, int inLclLbl, CrpDuk *psrCrpDuk)
{

	switch (inLclLbl)
	{
		case DKP_NEW_KEY:
		goto NewKey;

		case DKP_NEW_KEY_1:
		goto NewKey1;

		case DKP_NEW_KEY_2:
		goto NewKey2;

		case DKP_NEW_KEY_3:
		goto NewKey3;

		case DKP_NEW_KEY_4:
		goto NewKey4;

		case DKP_RQT_DTA_1:
		goto RqtDta1;

		case DKP_RQT_DTA_2:
		goto RqtDta2;

		default:
		return VS_ERROR;
	}

	NewKey:
	{
		unsigned long ulCrpNmb = 0;
	
		inDkpKsnCrpRed(psrDkpEnv->chKsn, &ulCrpNmb);

		if (inDkpHowMnyOne(ulCrpNmb) < 10)
		{
			goto NewKey1;
		}

		inDkpRmvCurKey(psrDkpEnv);

		ulDkpAddCrpAndShf(psrDkpEnv);

		goto NewKey2;
	}

	NewKey1:
	{
	
		psrDkpEnv->ulSft >>= 1;

		if (!psrDkpEnv->ulSft)
		{
			goto NewKey4;
		}

		goto NewKey3;
	}

	NewKey2:
	{
		unsigned long ulCrpNmb = 0;

		inDkpKsnCrpRed(psrDkpEnv->chKsn, &ulCrpNmb);

		if (!ulCrpNmb)
		{
			inSetEnvVarin("DKPMAXCRP", 1, NULL);

			inSetEnvVarin("DKPKEYLOD", 0, NULL);

			return VS_ERROR;
		}

		goto End;
	}

	NewKey3:
	{
		int inRetVal;
		unsigned long ulSftHgh = 0;
		unsigned long ulSftLow = 0;
		unsigned long ulKsnHgh = 0;
		unsigned long ulKsnLow = 0;
		unsigned long ulTmp0 = 0;
		unsigned long ulTmp1 = 0;
		unsigned long ulTmp2 = 0;
		unsigned long ulTmp3 = 0;
		char *pchCrp1;
		FutKey *psrFutKey;


		ulSftHgh = 0;
		ulSftLow = psrDkpEnv->ulSft;
		//#ifdef DEBUG
			//dump(psrDkpEnv->chKsn, sizeof(psrDkpEnv->chKsn), "ksn:");
		//#endif //DEBUG
		ulTmp0 = (unsigned char) psrDkpEnv->chKsn[2];
		ulTmp0 <<= 24;
		ulTmp1 = (unsigned char) psrDkpEnv->chKsn[3];
		ulTmp1 <<= 16;
		ulTmp2 = (unsigned char) psrDkpEnv->chKsn[4];
		ulTmp2 <<=  8;
		ulTmp3 = (unsigned char) psrDkpEnv->chKsn[5];
		ulKsnHgh = ulTmp0 | ulTmp1 | ulTmp2 | ulTmp3;
		
		ulTmp0 = (unsigned char) psrDkpEnv->chKsn[6];
		ulTmp0 <<= 24;
		ulTmp1 = (unsigned char) psrDkpEnv->chKsn[7];
		ulTmp1 <<= 16;
		ulTmp2 = (unsigned char) psrDkpEnv->chKsn[8];
		ulTmp2 <<=  8;
		ulTmp3 = (unsigned char) psrDkpEnv->chKsn[9];
		ulKsnLow = ulTmp0 | ulTmp1 | ulTmp2 | ulTmp3;

		
		ulTmp0 = ulSftHgh | ulKsnHgh;
		ulTmp1 = ulSftLow | ulKsnLow;

		pchCrp1 = psrDkpEnv->chCrp1;

		pchCrp1[0] = (ulTmp0 >> 24) & 0xFF;
		pchCrp1[1] = (ulTmp0 >> 16) & 0xFF;
		pchCrp1[2] = (ulTmp0 >>  8) & 0xFF;
		pchCrp1[3] = (ulTmp0      ) & 0xFF;
		pchCrp1[4] = (ulTmp1 >> 24) & 0xFF;
		pchCrp1[5] = (ulTmp1 >> 16) & 0xFF;
		pchCrp1[6] = (ulTmp1 >>  8) & 0xFF;
		pchCrp1[7] = (ulTmp1      ) & 0xFF;
//#ifdef DEBUG
	//	dump(psrDkpEnv->chCrp1, sizeof(psrDkpEnv->chCrp1), "chCrp1:");
//#endif //DEBUG

		memcpy(psrDkpEnv->chKey, psrDkpEnv->srFutKey[psrDkpEnv->inKey].chKey, sizeof(psrDkpEnv->chKey));
//#ifdef DEBUG
	//	dump(psrDkpEnv->chKey, sizeof(psrDkpEnv->chKey), "chKey:");
//#endif //DEBUG

		inRetVal = inDkpNonRvrKeyGenPrc(psrDkpEnv);

		inRetVal = inDkpChkSftBit(psrDkpEnv->ulSft);

		if (inRetVal < VS_SUCCESS)
		{
			return inRetVal;
		}

		psrFutKey = psrDkpEnv->srFutKey + inRetVal;

		memcpy(psrFutKey->chKey, psrDkpEnv->chCrp1, sizeof(psrDkpEnv->chCrp1));
//#ifdef DEBUG
	//	dump(psrFutKey->chKey, sizeof(psrFutKey->chKey), "chKey:");
//#endif //DEBUG

		memcpy(psrFutKey->chKey + sizeof(psrDkpEnv->chCrp1), psrDkpEnv->chCrp2, sizeof(psrDkpEnv->chCrp2));
#ifdef DEBUG
		//dump(psrFutKey->chKey, sizeof(psrFutKey->chKey), "chKey:");
#endif //DEBUG

		psrFutKey->chLrc = _SVC_LRC_CALC_((unsigned char *)psrFutKey->chKey, sizeof(psrFutKey->chKey), 0);
#ifdef DEBUG
		//dump(psrFutKey, sizeof(*psrFutKey), "key + lrc:");
#endif //DEBUG

		goto NewKey1;
	}

	NewKey4:
	{
		unsigned long ulCrpNmb = 0;

		inDkpRmvCurKey(psrDkpEnv);
		inDkpKsnCrpRed(psrDkpEnv->chKsn, &ulCrpNmb);
		ulCrpNmb++;
		inDkpKsnCrpWrt(psrDkpEnv->chKsn, ulCrpNmb);

		goto NewKey2;
	}

	RqtDta1:
	{
		FutKey *psrFutKey;
		char chLrc;

		unsigned long ulCrpNmb = 0;

		psrDkpEnv->inKey = inDkpSetBit(psrDkpEnv);

		psrFutKey = psrDkpEnv->srFutKey + psrDkpEnv->inKey;
#ifdef DEBUG
		//dump(psrFutKey, sizeof(*psrFutKey), "key + lrc:");
#endif //DEBUG

		chLrc = _SVC_LRC_CALC_((unsigned char *)psrFutKey->chKey, sizeof(psrFutKey->chKey), 0);


		if (psrFutKey->chLrc == chLrc)
		{
			goto RqtDta2;
		}

		ulCrpNmb = ulDkpAddCrpAndShf(psrDkpEnv);

		if (!ulCrpNmb)
		{
			inSetEnvVarin("DKPMAXCRP", 1, NULL);

			inSetEnvVarin("DKPKEYLOD", 0, NULL);

			return VS_ERROR;
		}

		goto RqtDta1;
	}

	RqtDta2:
	{
		char chTmp0[32]  = {0};
		int inRetVal = 0;

		memcpy(psrDkpEnv->chKey, psrDkpEnv->srFutKey[psrDkpEnv->inKey].chKey, sizeof(psrDkpEnv->chKey));
#ifdef DEBUG
		//dump(psrDkpEnv->chKey, sizeof(psrDkpEnv->chKey), "key:");
#endif //DEBUG

		inXor(psrDkpEnv->chMac, psrDkpEnv->chKey, "\x00\x00\x00\x00\x00\x00\xFF\x00\x00\x00\x00\x00\x00\x00\xFF\x00", 16);
#ifdef DEBUG
		//dump(psrDkpEnv->chMac, sizeof(psrDkpEnv->chMac), "mac:");
#endif //DEBUG

		inXor(psrDkpEnv->chDta, psrDkpEnv->chKey, "\x00\x00\x00\x00\x00\xFF\x00\x00\x00\x00\x00\x00\x00\xFF\x00\x00", 16);
#ifdef DEBUG
		//dump(psrDkpEnv->chDta, sizeof(psrDkpEnv->chDta), "dta:");
#endif //DEBUG

		memset(chTmp0, 0, sizeof(chTmp0));

		inRetVal = inTrpDes(TDES2KE, psrDkpEnv->chDta, psrDkpEnv->chDta, HEX_KEY_SIZ, chTmp0);
	
		memcpy(psrDkpEnv->chDta, chTmp0, sizeof(psrDkpEnv->chDta));
		

//#ifdef DEBUG
		//dump(psrDkpEnv->chDta, sizeof(psrDkpEnv->chDta), "dta:");
//#endif //DEBUG

		inXor(psrDkpEnv->chPin, psrDkpEnv->chKey, "\x00\x00\x00\x00\x00\x00\x00\xFF\x00\x00\x00\x00\x00\x00\x00\xFF", 16);
#ifdef DEBUG
		//dump(psrDkpEnv->chPin, sizeof(psrDkpEnv->chPin), "pin:");
#endif //DEBUG

		memcpy(psrDkpEnv->chKey, psrDkpEnv->chDta, sizeof(psrDkpEnv->chKey));
#ifdef DEBUG
		//dump(psrDkpEnv->chKey, sizeof(psrDkpEnv->chKey), "key:");
#endif //DEBUG

		inRetVal = inTrpDeaEncDta(psrDkpEnv, psrCrpDuk);

		if (inRetVal < VS_SUCCESS)
		{
			return inRetVal;
		}

		goto NewKey;
	}

	End:
	{
		inCrpSav(DKPENV_DAT, psrDkpEnv, sizeof(*psrDkpEnv));
		inDkpDmp(psrDkpEnv);
		memset(psrDkpEnv, 0, sizeof(*psrDkpEnv));

		return VS_SUCCESS;
	}
}

int inDkpIpkLod(char pchKey[], int inKey, char pchKsn[], int inKsn)		// AJM 01/08/2010 1
{
	DkpEnv srDkpEnv;
	FutKey *psrFutKey;
	CrpDuk srCrpDuk;
	int inRetVal;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG
LOGF_TRACE("--inDkpIpkLod--");  //ACM 28/10/2015
	  //  debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	   // APP_TRACE(szDebugMsg);
	//dump(pchKey, inKey, "pchKey:");

	//dump(pchKsn, inKsn, "pchKsn:");
#endif //DEBUG
	memset(&srDkpEnv, 0, sizeof(srDkpEnv));

	//Se modifica por homologacion con terminales Mx9xx
	//Modifico JVelazquez 19/08/14 DKPENV_DAT
	//if (inCrpSav("DkpEnv.dat", &srDkpEnv, sizeof(srDkpEnv)) != VS_SUCCESS)
	if (inCrpSav(DKPENV_DAT, &srDkpEnv, sizeof(srDkpEnv)) != VS_SUCCESS)
	{
		return VS_ERROR;
	}

	memset(&srCrpDuk, 0, sizeof(srCrpDuk));

	srCrpDuk.inMod = DUK_CRP_CHR;

	psrFutKey = srDkpEnv.srFutKey + 21;

	memcpy(psrFutKey->chKey, pchKey, inKey);

	psrFutKey->chLrc = _SVC_LRC_CALC_((unsigned char *)pchKey, inKey, 0);
#ifdef DEBUG
	//dump(psrFutKey, sizeof(*psrFutKey), "key + lrc:");
#endif //DEBUG

	srDkpEnv.inKey = 21;

	memcpy(srDkpEnv.chKsn, pchKsn, inKsn);

	inDkpKsnCrpWrt(srDkpEnv.chKsn, 0);

	srDkpEnv.ulSft = 0x00100000;

	srDkpEnv.ulCnt = 0;

	inRetVal = inDkpLclLbl(&srDkpEnv, DKP_NEW_KEY_3, &srCrpDuk);

	memset(&srDkpEnv, 0, sizeof(srDkpEnv));

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	 //   debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	//  APP_TRACE(szDebugMsg);

	#endif

	return inRetVal;
}

int inDkpRqtDta(void)		// AJM 01/08/2010 1
{
	DkpEnv srDkpEnv;
	CrpDuk srCrpDuk;
	int inRetVal;
LOGF_TRACE("--inDkpRqtDta--");   //ACM 30/10/2015
	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
//	#ifdef DEBUG

//	    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
//	    APP_TRACE(szDebugMsg);


//	#endif

	//Se modifica por homologacion con terminales Mx9xx
	//Modifico JVelazquez 19/08/14 DKPENV_DAT
	//if (inCrpLod("DkpEnv.dat", &srDkpEnv, sizeof(srDkpEnv)) != VS_SUCCESS)
	if (inCrpLod(DKPENV_DAT, &srDkpEnv, sizeof(srDkpEnv)) != VS_SUCCESS)
	{
		inSetEnvVarin("DKPKEYLOD", 0, NULL);

		return VS_ERROR;
	}

	memset(&srCrpDuk, 0, sizeof(srCrpDuk));

	srCrpDuk.inMod = DUK_CRP_CHR;

	inRetVal = inDkpLclLbl(&srDkpEnv, DKP_RQT_DTA_1, &srCrpDuk);

	memset(&srDkpEnv, 0, sizeof(srDkpEnv));

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	//#ifdef DEBUG

//	    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
//	    APP_TRACE(szDebugMsg);

//	#endif

	return inRetVal;
}

int inDkpRqtKey(char pchKey[], char pchKsn[])		// AJM 01/08/2010 1
{
	DkpEnv srDkpEnv;
	CrpDuk srCrpDuk;
	int inRetVal = 0;

	memset(&srDkpEnv,0,sizeof(DkpEnv));
	memset(&srCrpDuk,0,sizeof(CrpDuk));

	if (inCrpLod(DKPENV_DAT, &srDkpEnv, sizeof(srDkpEnv)) != VS_SUCCESS)
	{
		inSetEnvVarin("DKPKEYLOD", 0, NULL);

		return VS_ERROR;
	}

	memset(&srCrpDuk, 0, sizeof(srCrpDuk));

	srCrpDuk.inMod = DUK_ASK_KEY;

	inRetVal = inDkpLclLbl(&srDkpEnv, DKP_RQT_DTA_1, &srCrpDuk);

	memset(&srDkpEnv, 0, sizeof(srDkpEnv));

	memcpy(pchKey, srCrpDuk.chKey, HEX_KEY_SIZ);
//	dump(pchKey, HEX_KEY_SIZ, "key:");
	memcpy(pchKsn, srCrpDuk.chKsn, HEX_KSN_SIZ);

//	dump(pchKsn, HEX_KSN_SIZ, "ksn:");

//	LOGF_TRACE("inRetVal = %i",inRetVal);	//ACM 05/11/2015
	
	return inRetVal;
}

int inDkpSetBit(DkpEnv *psrDkpEnv)
{
	unsigned long ulCrpNmb;
	unsigned long ulMsk;
	unsigned long ulBak;
	int inBit;
	int inBak;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG 
LOGF_TRACE("--inDkpSetBit--");  //ACM 28/10/2015
//	    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
//	    APP_TRACE(szDebugMsg);


	#endif

	inDkpKsnCrpRed(psrDkpEnv->chKsn, &ulCrpNmb);

	ulBak = 0;

	inBak = 0;

	inBit = 1;

	for (ulMsk = 0x00100000; ulMsk; ulMsk >>= 1)
	{
		unsigned long ulTmp;

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

		   // debug_sprintf((szDebugMsg, "%s - inBit=%d", __FUNCTION__, inBit));
		   // APP_TRACE(szDebugMsg);

		   // debug_sprintf((szDebugMsg, "%s - ulNmb=0x%8.8lx", __FUNCTION__, ulCrpNmb));
		   // APP_TRACE(szDebugMsg);

		   // debug_sprintf((szDebugMsg, "%s - ulMsk=0x%8.8lx", __FUNCTION__, ulMsk));
		   // APP_TRACE(szDebugMsg);

		#endif

		ulTmp = ulCrpNmb & ulMsk;

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

		  //  debug_sprintf((szDebugMsg, "%s - ulTmp=0x%8.8lx", __FUNCTION__, ulTmp));
		  //  APP_TRACE(szDebugMsg);


		#endif

		if (ulTmp)
		{
			ulBak = ulMsk;

			inBak = inBit;
		}

		inBit++;
	}

	psrDkpEnv->ulSft = ulBak;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	  //  debug_sprintf((szDebugMsg, "%s - ulSft=0x%8.8lx", __FUNCTION__, psrDkpEnv->ulSft));
	  //  APP_TRACE(szDebugMsg);

	  //  debug_sprintf((szDebugMsg, "%s - inBak=%d", __FUNCTION__, inBak));
	  //  APP_TRACE(szDebugMsg);


	#endif

	return inBak;
}

int inDkpNonRvrKeyGenPrc(DkpEnv *psrDkpEnv)
{
	int inRetVal;
	char chTmp0[32];

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG
	LOGF_TRACE("--inDkpNonRvrKeyGenPrc--");  //ACM 28/10/2015
	 //   debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	//    APP_TRACE(szDebugMsg);


	#endif

	inXor(psrDkpEnv->chCrp2, psrDkpEnv->chCrp1, psrDkpEnv->chKey + 8, 8);

	memset(chTmp0, 0, sizeof(chTmp0));

	inRetVal = DES(DESE, (unsigned char *) psrDkpEnv->chKey, (unsigned char *) psrDkpEnv->chCrp2, (unsigned char *) chTmp0);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	   // debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	//    APP_TRACE(szDebugMsg);

	#endif

	if (inRetVal != VS_SUCCESS)
	{
		return VS_ERROR;
	}

#ifdef DEBUG
	//dump(chTmp0, sizeof(chTmp0), "chTmp0:");
#endif //DEBUG

	memcpy(psrDkpEnv->chCrp2, chTmp0, sizeof(psrDkpEnv->chCrp2));
#ifdef DEBUG
	//dump(psrDkpEnv->chCrp2, sizeof(psrDkpEnv->chCrp2), "chCrp2:");
#endif //DEBUG

	inXor(psrDkpEnv->chCrp2, psrDkpEnv->chCrp2, psrDkpEnv->chKey + 8, 8);

	inXor(psrDkpEnv->chKey, psrDkpEnv->chKey, "\xC0\xC0\xC0\xC0\x00\x00\x00\x00\xC0\xC0\xC0\xC0\x00\x00\x00\x00", 16);

	inXor(psrDkpEnv->chCrp1, psrDkpEnv->chCrp1, psrDkpEnv->chKey + 8, 8);

	memset(chTmp0, 0, sizeof(chTmp0));
	LOGF_TRACE("--DES--");
	inRetVal = DES(DESE, (unsigned char *) psrDkpEnv->chKey, (unsigned char *) psrDkpEnv->chCrp1, (unsigned char *) chTmp0);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	//    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	//    APP_TRACE(szDebugMsg);

	#endif

	if (inRetVal != VS_SUCCESS)
	{
		return VS_ERROR;
	}

#ifdef DEBUG
	//dump(chTmp0, sizeof(chTmp0), "chTmp0:");
#endif //DEBUG

	memcpy(psrDkpEnv->chCrp1, chTmp0, sizeof(psrDkpEnv->chCrp1));

	inXor(psrDkpEnv->chCrp1, psrDkpEnv->chCrp1, psrDkpEnv->chKey + 8, 8);

	return VS_SUCCESS;
}

int inTrpDeaEncPin(DkpEnv *psrDkpEnv)
{
	char chTmp0[32];
	int inRetVal;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	    APP_TRACE(szDebugMsg);


	#endif

	memset(chTmp0, 0, sizeof(chTmp0));

	inRetVal = DES(DESE, (unsigned char *) psrDkpEnv->chKey + 0, (unsigned char *) psrDkpEnv->chCrp1, (unsigned char *) chTmp0);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	    APP_TRACE(szDebugMsg);

	#endif

	if (inRetVal != VS_SUCCESS)
	{
		return VS_ERROR;
	}

#ifdef DEBUG
	//dump(chTmp0, sizeof(chTmp0), "chTmp0:");
#endif //DEBUG

	memcpy(psrDkpEnv->chCrp1, chTmp0, sizeof(psrDkpEnv->chCrp1));
#ifdef DEBUG
	//dump(psrDkpEnv->chCrp1, sizeof(psrDkpEnv->chCrp1), "chCrp1:");
#endif //DEBUG

	memset(chTmp0, 0, sizeof(chTmp0));

	inRetVal = DES(DESD, (unsigned char *) psrDkpEnv->chKey + 8, (unsigned char *) psrDkpEnv->chCrp1, (unsigned char *) chTmp0);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	    APP_TRACE(szDebugMsg);

	#endif

	if (inRetVal != VS_SUCCESS)
	{
		return VS_ERROR;
	}

	//dump(chTmp0, sizeof(chTmp0), "chTmp0:");

	memcpy(psrDkpEnv->chCrp1, chTmp0, sizeof(psrDkpEnv->chCrp1));

	//dump(psrDkpEnv->chCrp1, sizeof(psrDkpEnv->chCrp1), "chCrp1:");

	memset(chTmp0, 0, sizeof(chTmp0));

	inRetVal = DES(DESE, (unsigned char *) psrDkpEnv->chKey + 0, (unsigned char *) psrDkpEnv->chCrp1, (unsigned char *) chTmp0);

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
	    APP_TRACE(szDebugMsg);

	#endif

	if (inRetVal != VS_SUCCESS)
	{
		return VS_ERROR;
	}

	//dump(chTmp0, sizeof(chTmp0), "chTmp0:");

	memcpy(psrDkpEnv->chCrp1, chTmp0, sizeof(psrDkpEnv->chCrp1));

	//dump(psrDkpEnv->chCrp1, sizeof(psrDkpEnv->chCrp1), "chCrp1:");

	return VS_SUCCESS;
}

int inTrpDeaEncDta(DkpEnv *psrDkpEnv, CrpDuk *psrCrpDuk)		// AJM 01/08/2010 1
{
	int inRetVal;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG 
LOGF_TRACE("--inTrpDeaEncDta--"); // ACM 28/10/2015
//	    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
//	    APP_TRACE(szDebugMsg);


	#endif

	inRetVal = VS_ERROR;

	if (psrCrpDuk->inMod == DUK_CRP_CHR)
	{
		CrpCrd srCrpCrd;

		//Se modifica para dar la nueva ruta, para las terminales Mx9xx
		//Modifico JVelazquez 14/08/14   CRPCRD_DAT
		//if (inCrpLod("CrpCrd.dat", &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)
		if (inCrpLod(CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd)) == VS_SUCCESS)
		{
			inTrpDes(TDES2KE, psrDkpEnv->chKey, srCrpCrd.chAsm, sizeof(srCrpCrd.chAsm), srCrpCrd.chDsm);
			
			inHexToAsc(psrDkpEnv->chKsn, srCrpCrd.srCrpTk2.chKsn, HEX_KSN_SIZ);

			srCrpCrd.srCrpTk2.lnNmb = ++psrDkpEnv->ulCnt;

			//Se migra toda la parte de los traces para la terminal Mx9xx
			//Modifico JVelazquez 14/08/14
			#ifdef DEBUG

			 //   debug_sprintf((szDebugMsg, "%s - lnNmb=%ld", __FUNCTION__, srCrpCrd.srCrpTk2.lnNmb));
			 //   APP_TRACE(szDebugMsg);

			#endif

				//Se modifica para dar la nueva ruta, para las terminales Mx9xx
				//Modifico JVelazquez 14/08/14   CRPCRD_DAT
				//inRetVal = inCrpSav("CrpCrd.dat", &srCrpCrd, sizeof(srCrpCrd));
			    inRetVal = inCrpSav(CRPCRD_DAT, &srCrpCrd, sizeof(srCrpCrd));
		}

		memset(&srCrpCrd, 0, sizeof(srCrpCrd));
	}
	else
	{
		memcpy(psrCrpDuk->chKey, psrDkpEnv->chKey, HEX_KEY_SIZ);

		//dump(psrCrpDuk->chKey, HEX_KEY_SIZ, "chKey:");

		memcpy(psrCrpDuk->chKsn, psrDkpEnv->chKsn, HEX_KSN_SIZ);

		//dump(psrCrpDuk->chKsn, HEX_KSN_SIZ, "chKsn:");

		psrDkpEnv->ulCnt++;

		inRetVal = VS_SUCCESS;
	}

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

//	    debug_sprintf((szDebugMsg, "%s - inRetVal=%d", __FUNCTION__, inRetVal));
//	    APP_TRACE(szDebugMsg);

	#endif

	return inRetVal;
}

int inTrpDes(int inTch, char pchKey[], char pchSrc[], int inSrc, char pchEnd[])
{
	int inIdx;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
//	#ifdef DEBUG

LOGF_TRACE("--inTrpDes--");   //ACM 28/10/2015
	   // debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	   // APP_TRACE(szDebugMsg);
LOGF_TRACE("inTch = %i",inTch);  //ACM 28/10/2015
	//	debug_sprintf((szDebugMsg, "%s - inTch=%d", __FUNCTION__, inTch));
//		APP_TRACE(szDebugMsg);



//	#endif



	//dump(pchKey, HEX_KEY_SIZ, "pchKey:");

	//dump(pchSrc, inSrc, "pchSrc:");

	for (inIdx = 0; inIdx < inSrc; inIdx += 8)
	{
		int inRetVal = 0;

		//dump(pchSrc + inIdx, 8, "pchSrc:");
		LOGF_TRACE("Algoritmo DES");    
		inRetVal = DES(inTch, (unsigned char *) pchKey, (unsigned char *) pchSrc + inIdx, (unsigned char *) pchEnd + inIdx);
		LOGF_TRACE("DES RESULT [%d]",inRetVal);
		//dump(pchEnd + inIdx, 8, "pchEnd:");
	}
	LOGF_TRACE("3DES key");  
	//dump(pchEnd, inSrc, "pchEnd:");

	return VS_SUCCESS;
}

int inDkpChkSftBit(unsigned long ulSft)
{
	unsigned long ulMsk;
	int inBit;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

	    debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	    APP_TRACE(szDebugMsg);

		debug_sprintf((szDebugMsg, "%s - ulSft=0x%8.8lx", __FUNCTION__, ulSft));
		APP_TRACE(szDebugMsg);


	#endif



	inBit = 1;

	for (ulMsk = 0x00100000; ulMsk; ulMsk >>= 1)
	{
		unsigned long ulTmp;
		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

			debug_sprintf((szDebugMsg, "%s - inBit=%d", __FUNCTION__, inBit));
			APP_TRACE(szDebugMsg);

			debug_sprintf((szDebugMsg, "%s - ulSft=0x%8.8lx", __FUNCTION__, ulSft));
			APP_TRACE(szDebugMsg);

			debug_sprintf((szDebugMsg, "%s - ulMsk=0x%8.8lx", __FUNCTION__, ulMsk));
			APP_TRACE(szDebugMsg);

		#endif

		ulTmp = ulSft & ulMsk;

		//Se migra toda la parte de los traces para la terminal Mx9xx
		//Modifico JVelazquez 14/08/14
		#ifdef DEBUG

			debug_sprintf((szDebugMsg, "%s - ulTmp=0x%8.8lx", __FUNCTION__, ulTmp));
			APP_TRACE(szDebugMsg);


		#endif

		if (ulTmp)
		{
			break;
		}

		inBit++;
	}

	if (!ulMsk)
	{
		inBit = VS_ERROR;
	}

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG

		debug_sprintf((szDebugMsg, "%s - inBit=%d", __FUNCTION__, inBit));
		APP_TRACE(szDebugMsg);

	#endif

	return inBit;
}

int inDkpHowMnyOne(unsigned long ulCrpNmb)
{
	unsigned long ulMsk = 0;
	int inBit;
	int inOne;


	inOne = 0;
	inBit = 1;

	for (ulMsk = 0x00100000; ulMsk; ulMsk >>= 1)
	{
		unsigned long ulTmp;


		ulTmp = ulCrpNmb & ulMsk;

		if (ulTmp)
		{
			inOne++;
		}

		inBit++;
	}

	return inOne;
}

unsigned long ulDkpAddCrpAndShf(DkpEnv *psrDkpEnv)
{
	unsigned long ulCrpNmb = 0;


	inDkpKsnCrpRed(psrDkpEnv->chKsn, &ulCrpNmb);

	ulCrpNmb += psrDkpEnv->ulSft;

	inDkpKsnCrpWrt(psrDkpEnv->chKsn, ulCrpNmb);

	return ulCrpNmb;
}

int inDkpRmvCurKey(DkpEnv *psrDkpEnv)
{
	FutKey *psrFutKey;


	psrFutKey = psrDkpEnv->srFutKey + psrDkpEnv->inKey;

	//dump(psrFutKey, sizeof(*psrFutKey), "key + lrc:");

	memset(psrFutKey, 0, sizeof(*psrFutKey));

	psrFutKey->chLrc = _SVC_LRC_CALC_((unsigned char *)psrFutKey->chKey, sizeof(psrFutKey->chKey), 0);

	psrFutKey->chLrc++;

	//dump(psrFutKey, sizeof(*psrFutKey), "key + lrc:");

	return VS_SUCCESS;
}

int inDkpKsnCrpRed(char pchKsn[], unsigned long *pulCrpNmb)
{
	unsigned long ulCrpNmb = 0;


	//dump(pchKsn, HEX_KSN_SIZ, "pchKsn:");

	ulCrpNmb = 0;

	ulCrpNmb |= (unsigned char) pchKsn[7];
	ulCrpNmb <<= 8;
	ulCrpNmb |= (unsigned char) pchKsn[8];
	ulCrpNmb <<= 8;
	ulCrpNmb |= (unsigned char) pchKsn[9];
	ulCrpNmb &= 0x001FFFFF;

	
	*pulCrpNmb = ulCrpNmb;

	return VS_SUCCESS;
}

int inDkpKsnCrpWrt(char pchKsn[], unsigned long ulCrpNmb)
{
	
	
	ulCrpNmb &= 0x001FFFFF;

	//dump(pchKsn, HEX_KSN_SIZ, "pchKsn:");

	pchKsn[9] = ulCrpNmb & 0xFF;
	ulCrpNmb >>= 8;
	pchKsn[8] = ulCrpNmb & 0xFF;
	ulCrpNmb >>= 8;
	pchKsn[7] &= ~0x1F;
	pchKsn[7] |= ulCrpNmb & 0x1F;

	//dump(pchKsn, HEX_KSN_SIZ, "pchKsn:");

	return VS_SUCCESS;
}

int inXor(char pchEnd[], char pchOp1[], char pchOp2[], int inChr)
{
	int inIdx;

	//Se migra toda la parte de los traces para la terminal Mx9xx
	//Modifico JVelazquez 14/08/14
	#ifdef DEBUG
LOGF_TRACE("--Xor--");   //ACM 28/10/2015
	   // debug_sprintf((szDebugMsg, "%s - ", __FUNCTION__));
	  //  APP_TRACE(szDebugMsg);


	#endif

	//dump(pchOp1, inChr, "pchOp1:");

	//dump(pchOp2, inChr, "pchOp2:");

	for (inIdx = 0; inIdx < inChr; inIdx++)
	{
		pchEnd[inIdx] = ((unsigned char) pchOp1[inIdx]) ^ ((unsigned char) pchOp2[inIdx]);
	}

	//dump(pchEnd, inChr, "pchEnd:");

	return VS_SUCCESS;
}

int inDkpDmp(DkpEnv *psrDkpEnv)
{
	#if defined (HRD_COD_RSP)

	FutKey *psrFutKey;
	int inKey;
	int inMax;

	LOGF_TRACE("VERSION HARDCODE");
	#endif


	#if defined (HRD_COD_RSP)

	LOGF_TRACE("VERSION HARDCODE");
	inMax = sizeof(psrDkpEnv->srFutKey) / sizeof(*psrDkpEnv->srFutKey);

	for (inKey = 0; inKey < inMax; inKey++)
	{
		char szPrt[100];

		char chAsc[40];

		psrFutKey = psrDkpEnv->srFutKey + inKey;

		memset(chAsc, 0, sizeof(chAsc));

		_SVC_HEX_2_DSP_(psrFutKey->chKey, chAsc, sizeof(psrFutKey->chKey));

		sprintf(szPrt, "%-2d %2.2x [%16.16s %16.16s]", inKey, (unsigned int) psrFutKey->chLrc, chAsc + 0, chAsc + 16);

	}

	#endif

	return VS_SUCCESS;
}

// AJM 14/02/2010 1

#endif //MAKE_DUKPT
