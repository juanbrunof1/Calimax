
#ifdef MAKE_DUKPT
#include <stdio.h>

#include <errno.h>	
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#ifndef _VRXEVO
#include <printer.h>	
#endif

#include "debug.h"

//ADK
#include "tec/tec.h"
#include <log/liblog.h>

/*
===========================================================================+
 NOTE: | APPLICATION INCLUDE
===========================================================================+
*/
#include <svc.h>
#include "dkprsa.h"
#include <fcntl.h>
#include <unistd.h>

#undef OK

#include "usrcmds.h"
#include "dukpt_defs.h"
#include "dkprsa.h"
#include "rsa.h"
#include "getsysinfo.h"
//#include "polarssl\include\polarssl\sha1.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>


//ADK INCLUDES
#include <log/liblog.h>
#include <html/gui.h>
#include <sysinfo/sysinfo.h> //JRS, 30102018 


#ifdef _VRXEVO
#define DKPENV_DAT "/home/usr1/flash/DkpEnv.dat"
#else
#define DKPENV_DAT "I:1/DkpEnv.dat"
#endif

using namespace std;
using namespace vfigui;
using namespace vfisysinfo; //JRS, 30102018

//using namespace vfisysinfo;
/*
===========================================================================+
DEFINITION
===========================================================================+
*/
#define MIN_SAV_CHR		264

int inBanco = 1; //DEFUALT BANAMEX
int flagBIN=0; // FAG 17-nov-2017
int entry_mode=0; // FAG 14-mayo-2017
char HW_model[16];
//Se eliminan estas variables dado que NO aplican para el desarrollo de la Mx9xx
/*
#define SAV_DWN_NAM		"f:cpx.crt"   //Numeros de Serie para validar licencia de libreria
#define CPX_APL_VER		"f:ver.txt"   //Nombre de la version
#define CPX_CRT_RSA		"F:CRTRSA.DAT" //LLave para validar los numeros de Serie
*/
/*
;|===========================================================================+
;| NOTE: | EXTERNAL FUNCTIONS
;|===========================================================================+
*/

/*
===========================================================================+
LOCAL FUNCTIONS
===========================================================================+
*/
char *pchGetEnvVar(char pchVar[], char pchVal[], int inMaxChr, char pchFil[]);
int inSetEnvVarpch(char pchVar[], char pchVal[], char pchFil[]);
static int in_cpx_vld (void);
int getModel(char *  value );

/*
===========================================================================+
EXTERNAL VARIABLES
===========================================================================+
*/

/*
===========================================================================+
GLOBAL VARIABLES
===========================================================================+
*/
static CrdDsm g_srCrdDsm; //= {0};
int g_bo_mrc_crd;

/*
===========================================================================+
CODE
===========================================================================+
*/

DllSPEC int inHexToAsc(char pchHex[], char pchAsc[], int inHex)
{
	int inAsc;

	LOGF_TRACE("--- inHexToAsc ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchHex:",pchHex, inHex);
	
	inAsc = inHex << 1;
	memset(pchAsc, 0, inAsc); 
	//AscHex((unsigned char *)pchHex, pchAsc, inHex);
	_SVC_HEX_2_DSP_(pchHex, pchAsc, inHex);
	LOGAPI_HEXDUMP_TRACE("---> pchAsc:", pchAsc, inAsc);
	
	LOGF_TRACE("--- End inHexToAsc ---");
	
	return inAsc;
}

DllSPEC int inAscToHex(char pchAsc[], char pchHex[], int inAsc)		
{
	int inHex;

	LOGF_TRACE("--- inAscToHex ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchAsc:",pchAsc, inAsc);

	inHex = inAsc >> 1;
	memset(pchHex, 0, inHex);
	_SVC_DSP_2_HEX_(pchAsc, pchHex, inHex);
	LOGAPI_HEXDUMP_TRACE("---> pchHex:",pchHex, inHex);
	
	LOGF_TRACE("--- End inAscToHex ---");

	return inHex;
}

/*
#ifdef DEBUG_95

void dump(char *in_buf, int offset, int num_bytes, int ascii_rep)
{
	int     index=0;
	int		inMaxChr;
	char    temp_buf[10];
	char    disp_buf[500];

    if ( ascii_rep == TRUE )
    {
        if ( num_bytes > 16 )
        {
            // disp_buf overflow!
            pdebug ("dump ERR - num_bytes=%d > 15", num_bytes);
            return;
        }
		inMaxChr = 16;
    }
    else
    {
        if ( num_bytes > 23 )
        {
            // disp_buf overflow!
            pdebug (("dump ERR - num_bytes=%d > 20", num_bytes));
            return;
        }
		inMaxChr = 23;
    }

    MEMCLR (temp_buf, sizeof (temp_buf));
    MEMCLR (disp_buf, sizeof (disp_buf));
    sprintf(temp_buf, "%04X | ", offset);
    strcat (disp_buf, temp_buf);
    for (index = 0; index < inMaxChr; ++index)
    {
        MEMCLR (temp_buf, sizeof (temp_buf));
		if (index < num_bytes)
			sprintf(temp_buf, "%02.2X ", (int) (unsigned char) in_buf[offset + index]);
		else
			strcpy (temp_buf, "   ");

        strcat (disp_buf, temp_buf);

		if (!((index + 1) % 8) && ((index + 1) < inMaxChr))
			strcat(disp_buf, "  ");
    }

    if ( ascii_rep == TRUE )
    {
		char    c;
        strcat (disp_buf, "| ");
        for (index = 0; index < num_bytes; ++index)
        {
            MEMCLR (temp_buf, sizeof (temp_buf));
			c = in_buf[offset + index];
			if (c < 0x20 || c > 0x7E)
				c = '.';
            sprintf (temp_buf, "%c", c);
            strcat (disp_buf, temp_buf);
        }
    }

	pdebug (("%s", disp_buf));

	SVC_WAIT (20);
}

void pdump(void *pvdMem, int num_bytes, char *title)
{
	int offset = 0;
	int r=0;
	char *in_buf;

	if (num_bytes < 0)
		num_bytes = 0;

	if ( strlen (title) > 0 )
		pdebug (("%s (%d):", title, num_bytes));

	in_buf = pvdMem;
	while (offset < num_bytes)
	{
		r = num_bytes - offset;
		if (r > 16)
			r = 16;
		dump (in_buf, offset, r, TRUE);
		offset += r;
	}
}

#endif
// AJM 05/03/2009 1
*/
// AJM 21/05/2008 1		// AJM 21/07/2008 1		// AJM 31/07/2008 1		// AJM 05/03/2009 1
DllSPEC int inGetEnvVar(char pchVar[], char pchFil[])
{
	int inRetVal;
	char szVal[10];

	LOGF_TRACE("--- inGetEnvVar ---");

	pchGetEnvVar(pchVar, szVal, sizeof(szVal), pchFil);
	inRetVal = atoi(szVal);
	LOGF_TRACE("---> inRetVal = %i", inRetVal);
	
	LOGF_TRACE("--- End inGetEnvVar ---");

	return inRetVal;
}

DllSPEC long lnGetEnvVar(char pchVar[], char pchFil[])		
{
	long lnRetVal;
	char szVal[20];

	LOGF_TRACE("--- lnGetEnvVar ---");

	pchGetEnvVar(pchVar, szVal, sizeof(szVal), pchFil);
	lnRetVal = atol(szVal);
	LOGF_TRACE("---> lnRetVal = %i",lnRetVal);
	
	LOGF_TRACE("--- End lnGetEnvVar ---");

	return lnRetVal;
}

char *pchGetEnvVar(char pchVar[], char pchVal[], int inMaxChr, char pchFil[])
{
	int inRetVal=0;

	LOGF_TRACE("--- pchGetEnvVar ---");

	memset(pchVal, 0, inMaxChr);

	if (pchFil)
	{
		LOGF_TRACE("---> pchFil = [%s]",pchFil);
		//inRetVal = getkey(pchVar, pchVal, inMaxChr - 1, pchFil);
	}
	else
	{
		#ifdef _VRXEVO
			inRetVal = get_env(pchVar, pchVal, inMaxChr + 1);
		#else
			inRetVal = getEnvFile((char *)"perm", pchVar, pchVal, inMaxChr + 1);
		#endif
	}

	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	LOGF_TRACE("---> errno = %i",errno);
	LOGAPI_HEXDUMP_TRACE("---> val",pchVal,inRetVal);

	LOGF_TRACE("--- End pchGetEnvVar ---");

	return pchVal;
}

DllSPEC int inSetEnvVarin(char pchVar[], int inVal, char pchFil[])
{
	char szVal[10];
	int inRetVal;

	LOGF_TRACE("--- inSetEnvVarin ---");

	memset(szVal, 0, sizeof(szVal));
	sprintf(szVal, "%Xd", inVal);

	LOGAPI_HEXDUMP_TRACE("---> pchVar",pchVar, inVal); 

	inRetVal = inSetEnvVarpch(pchVar, szVal, pchFil);
	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	
	LOGF_TRACE("--- End inSetEnvVarin ---");

	return inRetVal;
}

int inSetEnvVarln(char pchVar[], long lnVal, char pchFil[])	
{
	char szVal[30];
	int inRetVal;

	LOGF_TRACE("--- inSetEnvVarin ---");

	memset(szVal, 0, sizeof(szVal));
	sprintf(szVal, "%ld", lnVal);
	LOGF_TRACE("---> pchVar = %s", pchVar);
	LOGF_TRACE("---> szVal=%s", szVal);

	inRetVal = inSetEnvVarpch(pchVar, szVal, pchFil);
	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	
	LOGF_TRACE("--- End inSetEnvVarln ---");

	return inRetVal;
}

int inSetEnvVarpch(char pchVar[], char pchVal[], char pchFil[])
{
	int inRetVal=0;

	LOGF_TRACE("--- inSetEnvVarpch ---");
	
	LOGF_TRACE("---> pchVar = %s", pchVar);
	LOGF_TRACE("---> pchVal=%s", pchVal);

	if (pchFil)
	{
		LOGF_TRACE("---> pchFil [%s]",pchFil);
		//inRetVal = putkey(pchVar, pchVal, strlen(pchVal), pchFil);
	}
	else
	{
		#ifdef _VRXEVO
			inRetVal = put_env(pchVar, pchVal,sizeof(pchVal));
		#else
			inRetVal = putEnvFile((char *)"perm",pchVar, pchVal);
		#endif
	}

	LOGF_TRACE("---> inRetVal = %i",inRetVal);
	
	LOGF_TRACE("--- End inSetEnvVarpch ---");

	return inRetVal;
}

int in_remove(char pchRmv[])
{
	int inRetVal;

	LOGF_TRACE("--- in_remove ---");
	
	LOGF_TRACE("---> pchRmv = %s", pchRmv);

	inRetVal = remove(pchRmv);
	LOGF_TRACE("---> inRetVal = %d",inRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	
	LOGF_TRACE("--- End in_remove ---");

	return inRetVal;
}
/*
int in_file_copy (const char pchOld[], const char pchNew[])		// AJM 27/01/2011 1
{
	int inRetVal;

	

	LOGF_TRACE ("in_file_copy");

	LOGF_TRACE("pchOld=%s pchNew=%s", pchOld, pchNew);

	inRetVal = file_copy (pchOld, pchNew);

	LOGF_TRACE("inRetVal=%d errno=%d", inRetVal, errno);

	return inRetVal;
}
*/
int in_open (const char pchOpn[], int inMod)	
{
	int inRetVal;

	LOGF_TRACE("--- in_open ---");
	
	LOGF_TRACE("---> pchOpn = %s", pchOpn);
	LOGF_TRACE("---> inMod = 0x%4.4x", inMod);

	inRetVal = open (pchOpn, inMod,0666);
	LOGF_TRACE("---> inRetVal = %d",inRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	
	LOGF_TRACE("--- End in_open ---");

	return inRetVal;
}

long ln_lseek (int inHdl, long lnChr, int inOrg)		
{
	long lnRetVal;

	LOGF_TRACE("--- ln_lseek ---");
	
	LOGF_TRACE("---> inHdl = %d", inHdl);
	LOGF_TRACE("---> lnChr = %ld", lnChr);
	LOGF_TRACE("---> inOrg = %d", inOrg);

	lnRetVal = lseek (inHdl, lnChr, inOrg);
	LOGF_TRACE("---> lnRetVal = %d",lnRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	
	LOGF_TRACE("--- End ln_lseek ---");

	return lnRetVal;
}

int in_read (int inHdl, char pchChr[], int inChr)
{
	int inRetVal;

	LOGF_TRACE("--- in_read ---");
	
	LOGF_TRACE("---> inHdl = %d", inHdl);

	memset(pchChr, 0, inChr);
	inRetVal = read (inHdl, pchChr, inChr);
	LOGF_TRACE("---> inRetVal = %d",inRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	LOGAPI_HEXDUMP_TRACE("---> pchChr:", pchChr, inRetVal);
	
	LOGF_TRACE("--- End in_read ---");

	return inRetVal;
}

int in_write (int inHdl, char pchChr[], int inChr)
{
	int inRetVal;

	LOGF_TRACE("--- in_write ---");
	
	LOGF_TRACE("---> inHdl = %d", inHdl);
	LOGAPI_HEXDUMP_TRACE("---> pchChr:", pchChr, inChr);

	inRetVal = write (inHdl, pchChr, inChr);
	LOGF_TRACE("---> inRetVal = %d",inRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	
	LOGF_TRACE("--- End in_write ---");

	return inRetVal;
}
int in_close (int inHdl)
{
	int inRetVal;

	LOGF_TRACE("--- in_close ---");
	
	LOGF_TRACE("---> inHdl = %d", inHdl);

	inRetVal = close (inHdl);
	LOGF_TRACE("---> inRetVal = %d",inRetVal);
	LOGF_TRACE("---> errno = %d",errno);
	
	LOGF_TRACE("--- End in_close ---");

	return inRetVal;
}

DllSPEC unsigned long ul_crc_32 (char pch_crc [], int in_crc, unsigned long ul_sed)
{
	unsigned long byte, crc, mask;
	unsigned char * puc_crc;
	int i, j;
	unsigned char CRC32[4];
	unsigned char OutputCRC32[4];

	LOGF_TRACE("--- ul_crc_32 ---");
	
	LOGF_TRACE("---> ul_sed = 0x%08.8x",ul_sed);
	LOGAPI_HEXDUMP_TRACE("---> crc", pch_crc, in_crc);

	puc_crc = (unsigned char *) pch_crc;
	crc = ul_sed;

	for (i = 0; i < in_crc; i++)
	{
		byte = puc_crc [i];		// Get next byte.
		crc = crc ^ byte;

		for (j = 0; j < 8; j++)		// Do eight times.
		{
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}

	crc = ~crc;
	LOGF_TRACE("---> crc = 0x%08.8x", crc);

	#ifndef _VRXEVO
	//Se adiciona esta seccion por el tema de big-Endian (El resultado esta invertido)
	memset(CRC32,0,sizeof(CRC32));
	memset(OutputCRC32,0,sizeof(OutputCRC32));
	memcpy(CRC32,&crc,sizeof(CRC32));
	OutputCRC32[0] = CRC32[3];
	OutputCRC32[1] = CRC32[2];
	OutputCRC32[2] = CRC32[1];
	OutputCRC32[3] = CRC32[0];

	memcpy(&crc,OutputCRC32,sizeof(CRC32));
	LOGF_TRACE("---> CRC32 = %s",CRC32);
	LOGF_TRACE("---> OutputCRC32 = %s",OutputCRC32);
	LOGF_TRACE("---> crc = %ld",crc);
	#endif
	LOGF_TRACE("--- End ul_crc_32 ---");


	return crc;
}

DllSPEC extern int inCrdDsm(struct TRACK *psrTrk, char chCrd[], int inCrd, const char szWch[])
{
	int inRetVal;

	LOGF_TRACE("--- inCrdDsm ---");
	
	LOGAPI_HEXDUMP_TRACE("---> chCrd:",chCrd, inCrd);
	LOGF_TRACE("---> szWch=%s", szWch);

	memset(psrTrk, 0, sizeof(*psrTrk));

	//inRetVal = card_parse(chCrd, psrTrk, szWch);
	inRetVal = INVLD_FORMAT;
	LOGF_TRACE("---> inRetVal = %i",inRetVal);  

	#if 1	
	if (inRetVal == INVLD_FORMAT)
	{
		int inSwpTrk;
		int inTrkSrc;
		int inRqtTrk;
		int inSiz;
		int inStt;

		inStt = 1;
		inSiz = 0;

		inRqtTrk = *szWch - '0';
		inTrkSrc = 0;
		LOGF_TRACE("RQ TRACK [%d]",inRqtTrk);
		for (inSwpTrk = 1; inSwpTrk < 4; inSwpTrk++)
		{
			//JRS AQUI ESTA EL PEDO
			//dump(chCrd + inTrkSrc, inCrd - inTrkSrc, "trk raw:");
			LOGAPI_HEXDUMP_TRACE("trk raw:",chCrd + inTrkSrc, inCrd - inTrkSrc);

			inSiz = chCrd[inTrkSrc++] - 2;
			LOGF_TRACE("inSiz %i",inSiz);
			
			inStt = chCrd[inTrkSrc++];
			LOGF_TRACE("inStt %i",inStt);

			LOGF_TRACE("inSwpTrk [%d] == inRqtTrk [%d]",inSwpTrk,inRqtTrk);
			if (inSwpTrk == inRqtTrk)
			{
				break;
			}

			//FIX SAMSUNG PAY
			LOGF_TRACE("TRACK 1 VACIO [%i]",(inSwpTrk == 1) && (inSiz <= 0));
			if((inSwpTrk == 1) && (inSiz <= 0))
			{
				LOGF_TRACE("TRACK 1 VACIO");
				inSiz=76;
			}

			inTrkSrc += inSiz;
			LOGF_TRACE("PON BUFFER [%d]",inTrkSrc);
		}

		LOGF_TRACE("!inStt && inSiz = %i",!inStt && (inSiz>0));
		
		if (!inStt && (inSiz>0))		// AJM 23/11/2012 1		// AJM 15/06/2013 1
		{
			char * pch_equ;
			int inSz0 = inSiz;

			memset(psrTrk->track, 0, sizeof(psrTrk->track));
			memcpy(psrTrk->track, chCrd + inTrkSrc, inSiz);
			psrTrk->track[inSiz] = 0;
			LOGAPI_HEXDUMP_TRACE("track:",psrTrk->track, inSiz);

			pch_equ = (char*)memchr (psrTrk->track, '=', inSiz);
			if (pch_equ)
			inSiz = pch_equ - psrTrk->track;
			if (inSiz > 20)
			inSiz = 20;

			memset(psrTrk->acct, 0, sizeof(psrTrk->acct));
			memcpy(psrTrk->acct, psrTrk->track, inSiz);
			psrTrk->acct[inSiz] = 0;
			LOGAPI_HEXDUMP_TRACE("acct:",psrTrk->acct, inSiz);

			inSiz = 0;
			if (pch_equ)
			inSiz = psrTrk->track + inSz0 - ++pch_equ;
			if (inSiz > 4)
			inSiz = 4;

			memset(psrTrk->exp, 0, sizeof(psrTrk->exp));
			memcpy(psrTrk->exp, pch_equ, inSiz);
			psrTrk->exp[inSiz] = 0;
			LOGAPI_HEXDUMP_TRACE("exp:",psrTrk->exp, inSiz);

			inRetVal = inRqtTrk;
		}
		
	}
	#endif

   	//svcWait(5000);
	LOGF_TRACE("---> inRetVal = %d",inRetVal); 
	
	LOGF_TRACE("--- End inCrdDsm ---");
	
	return inRetVal;
}

DllSPEC char *pchGetAppVer(char pchAppVer[])
{
	int inHndl;

	LOGF_TRACE("--- pchGetAppVer ---"); 

	#ifdef _VRXEVO
		inHndl = open("I:1/about.txt", O_RDONLY);
	#else
		inHndl = open("./flash/themes/themes/Aboutsoft/about.txt", O_RDONLY);
	#endif

	LOGF_TRACE("---> inHndl=%i", inHndl);
	
	if (inHndl > -1)
	{
		read(inHndl, pchAppVer, 16);
		close(inHndl);
	
		purge_char(pchAppVer, '\r');
		purge_char(pchAppVer, '\n');
	}
		
	if(inBanco == BNMX_MODE)
	{
		getSoVos(pchAppVer+strlen(pchAppVer));
		purge_char(pchAppVer, '\r');
		purge_char(pchAppVer, '\n');
	}

	LOGF_TRACE("--- End pchGetAppVer ---"); 

	return pchAppVer;
}

DllSPEC char *pchGetSrlNmb (char pchSrlNmb [])	
{
	int inPos = 0;
	char chModel [30] = {0};

	LOGF_TRACE("--- pchGetSrlNmb ---"); 

	if(inBanco == BNMX_MODE)
	{
		memcpy (pchSrlNmb + 0, "VF", 2);
		inPos +=2;
		//memcpy (pchSrlNmb + inPos, "VOS", 3);
		getModel(chModel);
		
		memcpy (pchSrlNmb + inPos, "VXV820", 6); //JRS, 04122018  Una version mas , capricho Evopayments

		/*
		if(!strcmp(chModel,"Razor"))
			memcpy (pchSrlNmb + inPos, "VXV925", 6);
		else if(!strncmp(chModel,"P400",4)) 
			memcpy (pchSrlNmb + inPos, "VXV400", 6);
		else if(!strncmp(chModel,"P200",4))
			memcpy (pchSrlNmb + inPos, "VXV200", 6);
		else if(!strncmp(chModel,"E285",4))
			memcpy (pchSrlNmb + inPos, "VXV285", 6);
		else if(!strncmp(chModel,"E280",4))
			memcpy (pchSrlNmb + inPos, "VXV280", 6);
		else if(!strncmp(chModel,"M400",4))
			memcpy (pchSrlNmb + inPos, "VXV400", 6);
		else if(!strncmp(chModel,"X10",3))
			memcpy (pchSrlNmb + inPos, "VXVX10", 6);
		else if(!strncmp(chModel,"E355",4))
			memcpy (pchSrlNmb + inPos, "VXV355", 6);
		else
			memcpy (pchSrlNmb + inPos, "VXV820", 6);
		*/

		//inPos +=3;
		inPos +=6;
		//getModelVos(chModel);
		//memcpy (pchSrlNmb + inPos, chModel, 3);
		//inPos +=3;
	}
	else if(inBanco == BBVA_MODE)
	{
		memcpy (pchSrlNmb + 0, "VF", 2);
		inPos +=2;
	}

	#ifdef _VRXEVO
		SVC_INFO_SERLNO(pchSrlNmb + inPos);
	#else
		svcInfoSerialNum(pchSrlNmb + inPos);
	#endif
	
	purge_char (pchSrlNmb + inPos, '-');
	LOGAPI_HEXDUMP_TRACE("---> srl", pchSrlNmb, strlen (pchSrlNmb));
	
	LOGF_TRACE("--- End pchGetSrlNmb ---");

	return pchSrlNmb;
	
}

int inCrdRawDsm(CrdDsm *psrCrdDsm, char pchRaw[], int inRaw, char pchMod[])
{
	LOGF_TRACE("--- inCrdRawDsm ---");
	
	LOGAPI_HEXDUMP_TRACE("---> pchRaw:",pchRaw, inRaw);
	LOGAPI_HEXDUMP_TRACE("---> pchMod:",pchMod, 2);

	memset(psrCrdDsm, 0, sizeof(*psrCrdDsm));

	if (inRaw > 0)
	{
		struct TRACK srTrk;
		int inDsm;

		psrCrdDsm->inMod = 2;
		memcpy(psrCrdDsm->chMod, pchMod, psrCrdDsm->inMod);
		LOGAPI_HEXDUMP_TRACE("---> chMod:",psrCrdDsm->chMod, psrCrdDsm->inMod);

		inDsm = inCrdDsm(&srTrk, pchRaw, inRaw, "1");
		LOGF_TRACE("---> inDsm = %i",inDsm);	

		if (inDsm == TK_1)
		{
			psrCrdDsm->inTk1 = strlen(srTrk.track);
			memcpy(psrCrdDsm->chTk1, srTrk.track, psrCrdDsm->inTk1);
			LOGAPI_HEXDUMP_TRACE("---> chTk1:",psrCrdDsm->chTk1, psrCrdDsm->inTk1);

			psrCrdDsm->inHld = strlen(srTrk.name);
			memcpy(psrCrdDsm->chHld, srTrk.name, psrCrdDsm->inHld);
			LOGAPI_HEXDUMP_TRACE("---> chNam:",psrCrdDsm->chHld, psrCrdDsm->inHld);
		}

		inDsm = inCrdDsm(&srTrk, pchRaw, inRaw, "2");
		//inDsm = inCrdDsm( );
		//inDsm = TK_2;
		LOGF_TRACE("---> inDsm = %i",inDsm); 
		
		if (inDsm == TK_2)		
		{
			char chHex[50];
			int inHex;

			psrCrdDsm->inTk2 = strlen(srTrk.track);
			memcpy(psrCrdDsm->chTk2, srTrk.track, psrCrdDsm->inTk2);
			LOGAPI_HEXDUMP_TRACE("chTk2:",psrCrdDsm->chTk2, psrCrdDsm->inTk2);

			psrCrdDsm->inAct = strlen(srTrk.acct);
			memcpy(psrCrdDsm->chAct, srTrk.acct, psrCrdDsm->inAct);
			LOGAPI_HEXDUMP_TRACE("chAct:",psrCrdDsm->chAct, psrCrdDsm->inAct);

			psrCrdDsm->inExp = strlen(srTrk.exp);	
			memcpy(psrCrdDsm->chExp, srTrk.exp, psrCrdDsm->inExp);
			LOGAPI_HEXDUMP_TRACE("chExp:",psrCrdDsm->chExp, psrCrdDsm->inExp);

			psrCrdDsm->inSvC = strlen(srTrk.type);
			memcpy(psrCrdDsm->chSvC, srTrk.type, psrCrdDsm->inSvC);
			LOGAPI_HEXDUMP_TRACE("chSvC:",psrCrdDsm->chSvC, psrCrdDsm->inSvC);

			//psrCrdDsm->inAcH = inAscToHex(psrCrdDsm->chAct, psrCrdDsm->chAcH, psrCrdDsm->inAct);
			memset(chHex, 0, sizeof(chHex));
			inHex = 0;
			memcpy(chHex, psrCrdDsm->chAct, psrCrdDsm->inAct);
			inHex = psrCrdDsm->inAct;
			
			if (inHex & 0x01)
				chHex[inHex++] = 'F';
			
			psrCrdDsm->inAcH = inAscToHex(chHex, psrCrdDsm->chAcH, inHex);
			LOGAPI_HEXDUMP_TRACE("chAcH:",psrCrdDsm->chAcH, psrCrdDsm->inAcH);
		}
	}

	if (psrCrdDsm->inTk2 < 6)
	{
		inRaw = VS_ERROR;
	}

	LOGF_TRACE("---> inRaw = %i",inRaw);  
	
	LOGF_TRACE("--- End inCrdRawDsm ---");
	
	return inRaw;
	
}

static int bo_exd_vld (char pch_exd [], int in_max)	
{
	int bo_val;

	LOGF_TRACE("--- bo_exd_vld ---");
	
	LOGAPI_HEXDUMP_TRACE("---> exd", pch_exd, in_max);

	bo_val = FALSE;

	if (in_max > 3)
	{
		char ch_exd [20] = {0};
		int  in_exd = 0;

		memcpy (ch_exd + in_exd, "20", 2);
		in_exd += 2;
		memcpy (ch_exd + in_exd, pch_exd, 4);
		in_exd += 4;
		memcpy (ch_exd + in_exd, "01010101", 8);
		in_exd += 8;

		if ( in_SVC_VALID_DATE (ch_exd) > -1 )
		{
			bo_val = TRUE;
		}
	}

	LOGF_TRACE("---> bo_val (bo_exd_vld ) = %i",bo_val);
	
	LOGF_TRACE("--- End bo_exd_vld ---");
	
	return bo_val;
}

static int bo_mrc_crd (CrdDsm * psr_crd_dsm)	
{
	int bo_val;

	LOGF_TRACE("--- bo_mrc_crd ---");

	bo_val = FALSE;

	if (g_bo_mrc_crd)
	{
		if ((!memcmp(psr_crd_dsm->chMod, "90" , 2)) || (!memcmp(psr_crd_dsm->chMod, "80" , 2)))
		{
			if (!bo_exd_vld(psr_crd_dsm->chExp, psr_crd_dsm->inExp))
			{
				bo_val = TRUE;
			}
		}
	}

	LOGF_TRACE("---> bo_val (bo_mrc_crd) = %i",bo_val);
	
	LOGF_TRACE("--- End bo_mrc_crd ---");

	return bo_val;
}

DllSPEC int in_cpx_crd_opn (void)
{
	LOGF_TRACE("--- in_cpx_crd_opn ---");

	memset (&g_srCrdDsm, 0, sizeof (g_srCrdDsm));
	inCrpCrdRmv ();

	LOGF_TRACE("--- End in_cpx_crd_opn ---");

	return SUCCESS;
}

static int in_cpx_crd_dmp (void)
{
	LOGF_TRACE("--- in_cpx_crd_dmp ---");
	
	LOGAPI_HEXDUMP_TRACE("---> tk2", g_srCrdDsm.chTk2, g_srCrdDsm.inTk2);
	LOGAPI_HEXDUMP_TRACE("---> tk1", g_srCrdDsm.chTk1, g_srCrdDsm.inTk1);
	LOGAPI_HEXDUMP_TRACE("---> cv2", g_srCrdDsm.chCv2, g_srCrdDsm.inCv2);
	LOGAPI_HEXDUMP_TRACE("---> act", g_srCrdDsm.chAct, g_srCrdDsm.inAct);
	LOGAPI_HEXDUMP_TRACE("---> exp", g_srCrdDsm.chExp, g_srCrdDsm.inExp);
	LOGAPI_HEXDUMP_TRACE("---> svc", g_srCrdDsm.chSvC, g_srCrdDsm.inSvC);
	LOGAPI_HEXDUMP_TRACE("---> hld", g_srCrdDsm.chHld, g_srCrdDsm.inHld);
	LOGAPI_HEXDUMP_TRACE("---> mod", g_srCrdDsm.chMod, g_srCrdDsm.inMod);

	LOGF_TRACE("--- End in_cpx_crd_dmp ---");
	
	return SUCCESS;
}

DllSPEC int in_cpx_crd_chp (char pch_tg57 [], int in_tg57, char pch_5f20 [], int in_5f20, bool isCtls)
{
	int in_val;

	// jbf 20250113
	 char auxtk2[48] = {0};
	int lenauxtk2 = 0;

	LOGF_TRACE("--- in_cpx_crd_chp ---");
	
	LOGAPI_HEXDUMP_TRACE("---> tg57", pch_tg57, in_tg57);
	LOGAPI_HEXDUMP_TRACE("---> 5f20", pch_5f20, in_5f20);

	// jbf 20250113 remove tag, len del TLV
	lenauxtk2 = in_tg57 - 2;
	memcpy(auxtk2, pch_tg57 + 2, lenauxtk2);
	LOGAPI_HEXDUMP_TRACE("---> auxtk2", auxtk2, lenauxtk2);

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		in_val = in_cpx_vld ();
	}

	if (in_val > FAILURE)
	{
		char ch_asc [50] = {0};
		int  in_asc = 0;
		int  in_act = 0;
		char * pch_equ;

		// jbf 20250113 using auxtk2 instead of pch_tg57
		//in_asc = inHexToAsc (pch_tg57, ch_asc, in_tg57);
		in_asc = inHexToAsc (auxtk2, ch_asc, lenauxtk2);

		LOGAPI_HEXDUMP_TRACE("---> asc", ch_asc, in_asc);
		
		pch_equ = (char*)memchr (ch_asc, 'D', in_asc);
		
		if (pch_equ)
			*pch_equ = '=';
		
		pch_equ = (char*)memchr (ch_asc, 'F', in_asc);

		if (pch_equ)
		{
			*pch_equ = 0;
			in_asc = pch_equ - ch_asc;
		}
		
		LOGAPI_HEXDUMP_TRACE("---> asc", ch_asc, in_asc);
		
		if (in_asc > sizeof (g_srCrdDsm.chTk2))
			in_asc = sizeof (g_srCrdDsm.chTk2);

		memcpy (g_srCrdDsm.chTk2, ch_asc, in_asc);
		g_srCrdDsm.inTk2 = in_asc;

		in_act = in_asc;
		pch_equ = (char*)memchr (ch_asc, '=', in_asc);
		
		if (pch_equ)
			in_act = pch_equ - ch_asc;
		
		memcpy (g_srCrdDsm.chAct, ch_asc, in_act);
		g_srCrdDsm.inAct = in_act;

		if (pch_equ)
		{
			int in_avl;

			pch_equ += 1;
			in_avl = (ch_asc + in_asc) - pch_equ;
			
			if (in_avl > 3)
			{
				memcpy (g_srCrdDsm.chExp, pch_equ, 4);
				g_srCrdDsm.inExp = 4;
			}

			pch_equ += 4;
			in_avl = (ch_asc + in_asc) - pch_equ;
			
			if (in_avl > 2)
			{
				memcpy (g_srCrdDsm.chSvC, pch_equ, 3);
				g_srCrdDsm.inSvC = 3;
			}
			
		}

		if (in_act & 0x01)
			ch_asc [in_act++] = 'F';
		
		g_srCrdDsm.inAcH = inAscToHex (ch_asc, g_srCrdDsm.chAcH, in_act);

		if (in_5f20 > sizeof (g_srCrdDsm.chHld))
			in_5f20 = sizeof (g_srCrdDsm.chHld);
		
		memcpy (g_srCrdDsm.chHld, pch_5f20, in_5f20);
		g_srCrdDsm.inHld = in_5f20;
		
		 LOGF_TRACE("\n--------entry_mode = %i", entry_mode);
		if(isCtls)
		{
			memcpy (g_srCrdDsm.chMod, "07", 2);
			LOGF_TRACE("\n--------entry_mode = %s",g_srCrdDsm.chMod);

		}

		else
		{
			memcpy (g_srCrdDsm.chMod, "05", 2);
			LOGF_TRACE("\n--------entry_mode = %s",g_srCrdDsm.chMod);
		}
	
		g_srCrdDsm.inMod = 2;

		in_cpx_crd_dmp ();
		inCrpChpCrdSav (g_srCrdDsm.chTk2, g_srCrdDsm.inTk2, g_srCrdDsm.chAct, g_srCrdDsm.inAct,isCtls);

		//inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, TURE); //JRS, 04102018 CHE FAG , MANDABA LA BANDERA DE CVV ENCENDIDA ENCHIP -_- 
		//inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, FALSE);
		inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, CVV_NOTASKED);
	}

	LOGF_TRACE("--- End in_cpx_crd_chp ---");

	return in_val;
}

DllSPEC int in_cpx_crd_mag (char pchRaw[], int inRaw, int bo_fbk, char pchCv2[], int inCv2, bool isCtls, int flgCVV)		// AJM 11/04/2011 1		// AJM 09/08/2011 1		// AJM 14/02/2012 1
{
	char * pchMod;
	int inVal;

	LOGF_TRACE("--- in_cpx_crd_mag ---");
	LOGF_TRACE("*** bo_fbk = %d",bo_fbk);


	if(isCtls)
	{
		pchMod = "91";
	}
	else
	{
		
		pchMod = "90";
		if (bo_fbk)
		pchMod = "80";
	}
	LOGF_TRACE("pchMod = %s",pchMod);
	LOGAPI_HEXDUMP_TRACE ( "pchRaw",pchRaw, inRaw);
	LOGAPI_HEXDUMP_TRACE ( "pchMod",pchMod, 2);
	LOGAPI_HEXDUMP_TRACE ( "pchCv2",pchCv2, inCv2);

	//dump (pchRaw, inRaw, "pchRaw");
	//dump (pchMod, 2, "pchMod");
	//dump (pchCv2, inCv2, "pchCv2");

	inVal = SUCCESS;

	if (inVal > FAILURE)
	{
		inVal = in_cpx_vld ();
	}

	if (inVal > FAILURE)
	{
		int in_ret;

		in_ret = inCrdRawDsm (&g_srCrdDsm, pchRaw, inRaw, pchMod);

		if (in_ret < SUCCESS)
		{
			inVal = CPX_ERC_TK2;
		}
	}

	if (inVal > FAILURE)
	{
		if (inCv2 > 4)
			inCv2 = 4;

		memcpy (g_srCrdDsm.chCv2, pchCv2, inCv2);
		g_srCrdDsm.inCv2 = inCv2;

		memcpy (g_srCrdDsm.chMod, pchMod, 2);
		g_srCrdDsm.inMod = 2;

		in_cpx_crd_dmp ();

		inCrpSwpCrdSav (pchRaw, inRaw, pchMod);

		//inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, TRUE); 
		inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, flgCVV);
	}

	LOGF_TRACE("---> inVal = %d", inVal);
	
	LOGF_TRACE("--- End in_cpx_crd_mag ---");

	return inVal;
}

DllSPEC int in_cpx_crd_kbd (char pch_act [], int in_act, char pch_exd [], int in_exd, char pch_cv2 [], int in_cv2)		// AJM 14/02/2012 1
{
	int inVal;

	LOGF_TRACE("--- in_cpx_crd_kbd ---");  
	
	LOGAPI_HEXDUMP_TRACE("---> act", pch_act, in_act);
	LOGAPI_HEXDUMP_TRACE("---> exd", pch_exd, in_exd);
	LOGAPI_HEXDUMP_TRACE("---> cv2", pch_cv2, in_cv2);

	inVal = SUCCESS;

	if (inVal > FAILURE)
	{
		inVal = in_cpx_vld ();
	}

	if (inVal > FAILURE)
	{
		char * pch_end = NULL;

		if (in_act > sizeof (g_srCrdDsm.chAct))
			in_act = sizeof (g_srCrdDsm.chAct);

		memcpy (g_srCrdDsm.chAct, pch_act, in_act);
		g_srCrdDsm.inAct = in_act;
		
		LOGAPI_HEXDUMP_TRACE ("act",g_srCrdDsm.chAct, g_srCrdDsm.inAct );

		pch_end = (char*)memchr (g_srCrdDsm.chAct, 'F', g_srCrdDsm.inAct);
		if (pch_end)
		{
			*pch_end = 0;
			g_srCrdDsm.inAct = pch_end - g_srCrdDsm.chAct;
			LOGAPI_HEXDUMP_TRACE ( "act",g_srCrdDsm.chAct, g_srCrdDsm.inAct);
		}
		
	}

	if (inVal > FAILURE)
	{
		if (in_exd > 4)
			in_exd = 4;

		if (in_exd < 4)
		{
			memcpy (g_srCrdDsm.chExp, "0000", 4);
			g_srCrdDsm.inExp = 4;
		}
		else
		{
			memcpy (g_srCrdDsm.chExp, pch_exd, in_exd);
			g_srCrdDsm.inExp = in_exd;
		}
	}

	if (inVal > FAILURE)
	{
		if (in_cv2 > 4)
			in_cv2 = 4;

		memcpy (g_srCrdDsm.chCv2, pch_cv2, in_cv2);
		g_srCrdDsm.inCv2 = in_cv2;
	}

	if (inVal > FAILURE)
	{
		memcpy (g_srCrdDsm.chMod, "01", 2);
		g_srCrdDsm.inMod = 2;
	}

	if (inVal > FAILURE)
	{
		char chHex[50];
		int inHex;
		int inTk2;

		memset (chHex, 0, sizeof(chHex));
		inHex = 0;
		memcpy (chHex, g_srCrdDsm.chAct, g_srCrdDsm.inAct);
		inHex = g_srCrdDsm.inAct;
		if (inHex & 0x01)
			chHex[inHex++] = 'F';
		g_srCrdDsm.inAcH = inAscToHex (chHex, g_srCrdDsm.chAcH, inHex);

		memset (g_srCrdDsm.chTk2, 0, sizeof (g_srCrdDsm.chTk2 + inTk2));
		inTk2 = 0;

		memcpy (g_srCrdDsm.chTk2 + inTk2, g_srCrdDsm.chAct, g_srCrdDsm.inAct);
		inTk2 += g_srCrdDsm.inAct;

		memcpy (g_srCrdDsm.chTk2 + inTk2, "=", 1);
		inTk2 += 1;

		memcpy (g_srCrdDsm.chTk2 + inTk2, g_srCrdDsm.chExp + 2, 2);
		inTk2 += 2;

		memcpy (g_srCrdDsm.chTk2 + inTk2, g_srCrdDsm.chExp + 0, 2);
		inTk2 += 2;

		g_srCrdDsm.inTk2 = inTk2;

		LOGF_TRACE("TRAC2 DUMMY [%s]",g_srCrdDsm.chTk2);

		in_cpx_crd_dmp ();

		inCrpKbdCrdSav (g_srCrdDsm.chAct, g_srCrdDsm.inAct, g_srCrdDsm.chExp, g_srCrdDsm.inExp);

		//inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, TRUE);
		inCrpCrdCv2Sav (g_srCrdDsm.chCv2, g_srCrdDsm.inCv2, CVV_CAPTURED);
	}

	LOGF_TRACE("---> inVal = %d", inVal);
	
	LOGF_TRACE("--- End in_cpx_crd_kbd ---");

	return inVal;
}

int in_cry_asm (char pch_qs [], int * pin_qs, char pch_qz [], int * pin_qz, char pch_qy [], int * pin_qy)
{
	int in_val;

	LOGF_TRACE("--- in_cry_asm ---");

	in_cpx_crd_dmp ();

	if ( bo_mrc_crd (&g_srCrdDsm) )	
	{
		in_val = VS_ESCAPE;
	}
	else
	{
		in_val = inCrpDkpRun (g_srCrdDsm.chAct, g_srCrdDsm.inAct);
		LOGF_TRACE("<<< After inCrpDkpRun >>>");  
		LOGF_TRACE("---> in_val = %i",in_val);
	}

	if (in_val == SUCCESS)
	{
		memset (g_srCrdDsm.chTk2, 0, sizeof (g_srCrdDsm.chTk2));
		memset (g_srCrdDsm.chTk1, 0, sizeof (g_srCrdDsm.chTk1));
		memset (g_srCrdDsm.chCv2, 0, sizeof (g_srCrdDsm.chCv2));

		g_srCrdDsm.inTk2 = 0;
		g_srCrdDsm.inTk1 = 0;
		g_srCrdDsm.inCv2 = 0;
	}
	else
	{
		inCrpCrdRmv ();
		inCrdExpSwc (g_srCrdDsm.chTk2, g_srCrdDsm.inTk2, g_srCrdDsm.chMod, g_srCrdDsm.inMod);
	}

	*pin_qs = inAsmQs (pch_qs, FALSE);

	LOGF_TRACE("<<< After QS >>>");	
	LOGF_TRACE("---> in_val = %i",in_val);

	if (in_val == SUCCESS)
	{
		*pin_qz = inAsmQz (pch_qz);
		*pin_qy = inAsmQy (pch_qy);
	}

	if (in_val == CPX_ERC_EXC)
	{
		LOGF_TRACE("--ACTIVAMOS LA BANDERA de flag bin,1=no encriptar--");
		in_val =  SUCCESS;
	}

	in_cpx_crd_dmp ();

	LOGF_TRACE("--- End in_cry_asm ---");

	return in_val;
}

DllSPEC int in_cpx_crd_cry (char pch_qs [], int * pin_qs, char pch_qz [], int * pin_qz, char pch_qy [], int * pin_qy)
{
	int in_val;

	LOGF_TRACE("--- in_cpx_crd_cry ---");

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		in_val = in_cpx_vld ();
	}

	if (in_val > FAILURE)
	{
		in_val = in_cry_asm (pch_qs, pin_qs, pch_qz, pin_qz, pch_qy, pin_qy);
	}

	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_cpx_crd_cry ---");

	return in_val;
}

DllSPEC int in_cpx_crd_end (int in_ath, int bo_ery)
{
	LOGF_TRACE("--- in_cpx_crd_end ---");

	inCrpDukErr (in_ath, bo_ery);
	in_cpx_crd_opn ();

	LOGF_TRACE("--- End in_cpx_crd_end ---");

	return SUCCESS;
}

DllSPEC int in_cpx_ipk_opn (void)
{
	LOGF_TRACE("--- in_cpx_ipk_opn ---");

	inCrpKtkRmv ();

	LOGF_TRACE("--- in_cpx_ipk_opn ---");

	return SUCCESS;
}

DllSPEC int in_cpx_ipk_ktk (char pch_qs [], int * pin_qs, char pch_qw [], int * pin_qw)
{
	int in_val;

	LOGF_TRACE("--- in_cpx_ipk_ktk ---");

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		in_val = in_cpx_vld ();
	}

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inCrpKtkAsm();
		LOGF_TRACE("---> in_ret = %i",in_ret); 

		if (in_ret < SUCCESS)
		{
			in_val = CPX_ERC_TKA;
		}
	}

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inAsmQw (pch_qw);

		if (in_ret < 1)
		{
			in_val = CPX_ERC_QWA;
		}
		else
		{
			*pin_qw = in_ret;
		}
	}

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inAsmQs (pch_qs, TRUE);

		if (in_ret < 1)
		{
			in_val = CPX_ERC_QSA;
		}
		else
		{
			*pin_qs = in_ret;
		}
	}

	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_cpx_ipk_ktk ---");

	return in_val;
}

DllSPEC int in_cpx_ipk_sav (char pch_qx [], int in_qx)
{
	int in_val;

	LOGF_TRACE("--- in_cpx_ipk_sav ---");
	
	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		in_val = in_cpx_vld ();
	}

	LOGF_TRACE("---> Serial Num validate: %i", in_val);
	
	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inDsmQx (pch_qx, in_qx);

		if (in_ret < in_qx)
		{
			in_val = CPX_ERC_QXD;
		}
	}

	LOGF_TRACE("--->  inDsmQx validate: %i", in_val);

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inCrpKtkDsm ();

		if (in_ret < SUCCESS)
		{
			in_val = in_ret;
		}
	}

	LOGF_TRACE("---> inCrpKtkDsm  validate: %i", in_val);
	
	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = in_ipk_sav ();

		if (in_ret < SUCCESS)
		{
			in_val = in_ret;
		}
	}

	LOGF_TRACE("---> in_ipk_sav validate: %i", in_val);
	
	LOGF_TRACE("--- End in_cpx_ipk_sav ---");

	return in_val;
}

DllSPEC int in_cpx_ipk_end (void)
{
	LOGF_TRACE("--- in_cpx_ipk_end ---");

	in_cpx_ipk_opn ();

	LOGF_TRACE("--- End in_cpx_ipk_end ---");

	return SUCCESS;
}

DllSPEC int in_cpx_crd_exc (char pch_qt [], int in_qt)
{
	int in_val;

	LOGF_TRACE("--- in_cpx_crd_exc ---");

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inDsmQt (pch_qt, in_qt);

		if (in_ret < SUCCESS)
		{
			in_val = CPX_ERC_QTD;
		}
	}

	if (in_val > FAILURE)
	{
		in_val = inCrpExcDsm ();
	}
	
	LOGF_TRACE("---> in_val = %i",in_val);

	if (in_val > FAILURE)
	{
		in_val = inCrpExcSav ();
	}

	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_cpx_crd_exc ---");

	return in_val;
}
// AJM 28/07/2010 1
#if 1		// AJM 01/03/2014 1
// AJM 04/01/2011 1
DllSPEC int inCrpSmlMax (void)
{
	int inVal;
	long lnEnd;
	std::map<std::string, std::string> value;
	

	LOGF_TRACE ("inCrpSmlMax");

	inVal = VS_ESCAPE;

	lnEnd = lnGetEnvVar("#CRPSMLMAX", NULL);

	if (lnEnd > 0)
	{
		long lnSrc;
		char chKey[HEX_KEY_SIZ];
		char chKsn[HEX_KSN_SIZ];
		char szShw[30];

		for (lnSrc = ulCrpCnt(); lnSrc < lnEnd; lnSrc++)
		{
			//LOGF_TRACE ("lnSrc=%ld", lnSrc);

			//inSetEnvVarln("CRPSMLNOW", lnSrc, NULL);

			sprintf(szShw, "(%ld/%ld)", lnSrc, lnEnd);

			//LOGF_TRACE("CONTADOR = %s",szShw);
			
			value["BODY_HEADER"] = "PRUEBA DE UMBRALES";
			value["BODY_MSG"] = std::string(szShw);
			value["NAME_IMG"] = "";
			//uiInvokeURL(1, value, "screen_global_msg.html");

			inVal = inDkpRqtKey(chKey, chKsn);

			LOGF_TRACE ("inVal=%d", inVal);

			if (inVal < VS_SUCCESS)
			{
				break;
			}
		}
	}

	LOGF_TRACE ("inVal=%d", inVal);

	return inVal;
}
// AJM 04/01/2011 1
// AJM 06/01/2011 1
unsigned long ulCrpCnt (void)
{
	DkpEnv srDkpEnv;
	unsigned long ulVal;

	LOGF_TRACE ("ulCrpCnt");

	ulVal = 0;

	if (inCrpLod("DkpEnv.dat", &srDkpEnv, sizeof(srDkpEnv)) == VS_SUCCESS)
	{
		ulVal = srDkpEnv.ulCnt;
	}

	memset(&srDkpEnv, 0, sizeof(srDkpEnv));

	LOGF_TRACE ("ulVal=%d", ulVal);

	return ulVal;
}
// AJM 06/01/2011 1
#endif
// AJM 23/01/2011 1
int in_SVC_VALID_DATE (char pchVld[])
{
	int inVal;

	LOGF_TRACE("--- in_SVC_VALID_DATE ---");
	
	LOGAPI_HEXDUMP_TRACE("---> vld", pchVld, 14);

	inVal = _SVC_VALID_DATE_(pchVld);
	LOGF_TRACE("---> in_val = %i",inVal);
	
	LOGF_TRACE("--- End in_SVC_VALID_DATE ---");

	return inVal;
}

int inCrdExpSwc (char p_chTk2 [], int inTk2, char p_chMod [], int inMod)
{
	LOGF_TRACE("--- inCrdExpSwc ---");
	
	LOGAPI_HEXDUMP_TRACE("---> mod", p_chMod, inMod);
	LOGAPI_HEXDUMP_TRACE("---> tk2", p_chTk2, inTk2);

	if (!memcmp (p_chMod, "01", 2))
	{
		char * p_chEql;

		p_chEql = strchr (p_chTk2, '=');

		if (p_chEql)
		{
			char chMmYy [10];

			++p_chEql;

			memset (chMmYy, 0, sizeof (chMmYy));
			memcpy (chMmYy, p_chEql, 4);
			memcpy (p_chEql + 0, chMmYy + 2, 2);
			memcpy (p_chEql + 2, chMmYy + 0, 2);
		}
	}

	LOGAPI_HEXDUMP_TRACE("---> tk2", p_chTk2, inTk2);

	LOGF_TRACE("--- End inCrdExpSwc ---");

	return inTk2;
}
//Se comenta esta funcion dado que no aplica para el desarrollo de la Mx9xx
//Esta funcion se encarga de leer los numeros de serie, para validar licencia de libreria
//Comento JVelazquez 07/08/14
/*
// AJM 19/10/2011 1
// AJM 08/07/2013 1
static int inSavRed (char pchChr[], int inChr, long lnChr)
{
	int inVal;

	SAVE_FUNC_NAME ("inSavRed")

	pdebug ((FUNC_NAME_TITL));

	inVal = VS_ERROR;

	if (inChr > 0)
	{
		int inHdl;

		inVal = VS_FAILURE;

		inHdl = in_open (SAV_DWN_NAM, O_RDONLY);

		if (inHdl > -1)
		{
			long lnRet;

			lnRet = ln_lseek (inHdl, lnChr, SEEK_SET);

			if (lnRet > -1)
			{
				memset (pchChr, 0, inChr);

				inVal = in_read (inHdl, pchChr, inChr);

				pdump (pchChr, inVal, "chr");
			}

			in_close (inHdl);
		}
	}

	pdebug (("inVal=%d", inVal));

	return inVal;
}

static unsigned long ulSavCrc (void)
{
	unsigned long ulVal;
	int inHdl;
	int inVal;

	SAVE_FUNC_NAME ("ulSavCrc")

	pdebug ((FUNC_NAME_TITL));

	ulVal = 0;

	inVal = VS_SUCCESS;

	inHdl = in_open (SAV_DWN_NAM, O_RDONLY);

	if (inHdl < 0)
	{
		inVal = VS_FAILURE;
	}

	if (inVal > VS_ERROR)
	{
		long lnRet;

		lnRet = ln_lseek (inHdl, MIN_SAV_CHR, SEEK_SET);

		if (lnRet < 0)
		{
			inVal = VS_FAILURE;
		}
	}

	if (inVal > VS_ERROR)
	{
		unsigned long crc;

		crc = 0xFFFFFFFF;

		for (;;)
		{
			char chChr[5];
			int inRet;

			memset (chChr, 0, sizeof(chChr));

			inRet = read (inHdl, chChr, 1);

			if (inRet < 1)
			{
				break;
			}

			crc = ul_crc_32 (chChr, 1, crc);

			crc = ~crc;
		}

		crc = ~crc;

		pdebug (("crc=0x%08.8x", crc));

		ulVal = crc;
	}

	if (inHdl > -1)
	{
		in_close (inHdl);
	}

	pdebug (("crc=0x%08.8x", ulVal));

	return ulVal;
}

*/

//Se comenta estas funciones dado que NO aplica para el desarrollo de la Mx9xx
//Estas funciones se encargan de leer la llave RSA para validar la firma de los numeros de serie
//Comento JVelazquez 12/08/14
/*
static int in_sha1_hnd (char pch_sha [20], int in_sha, char pch_hnd [], int in_mov)
{
	sha1_context ctx = {0};
	FILE *psr_hnd = NULL;
	unsigned long ul_siz;
	int inVal;

	SAVE_FUNC_NAME ("in_sha1_hnd")

	pdebug ((FUNC_NAME_TITL));

	inVal = SUCCESS;

	if (inVal > FAILURE)
	{
		psr_hnd = fopen (pch_hnd, "rb");

		if (!psr_hnd)
		{
			inVal = FAILURE;
		}
	}

	if (inVal > FAILURE)
	{
		int inRet;

		inRet = fseek (psr_hnd, 0, SEEK_END);

		if (inRet)
		{
			inVal = FAILURE;
		}
	}

	if (inVal > FAILURE)
	{
		long ln_siz;

		ln_siz = ftell (psr_hnd);

		if (ln_siz < 0)
		{
			inVal = FAILURE;
		}
		else
		{
			ul_siz = ln_siz;
		}
	}

	if (inVal > FAILURE)
	{
		if (ul_siz < (unsigned long) in_mov)
		{
			inVal = FAILURE;
		}
		else
		{
			ul_siz -= in_mov;
		}
	}

	if (inVal > FAILURE)
	{
		int inRet;

		inRet = fseek (psr_hnd, in_mov, SEEK_SET);

		if (inRet)
		{
			inVal = FAILURE;
		}
	}

	if (inVal > FAILURE)
	{
		memset (pch_sha, 0, in_sha);

		memset (&ctx, 0, sizeof (ctx));

		sha1_starts (&ctx);
	}

	if (inVal > FAILURE)
	{
		unsigned long ul_avl;
		unsigned long ul_mov;
		char ch_val [ASC_MOD_SIZ];
		int in_val;
		int in_rcv;
		int in_ret;

		in_val = HEX_MOD_SIZ;

		for (ul_mov = 0; ul_mov < ul_siz; ul_mov += in_val)
		{
			ul_avl = ul_siz - ul_mov;

			in_rcv = ul_avl;
			if (ul_avl > (unsigned long) in_val)
			in_rcv = in_val;

			memset (ch_val, 0, in_val);

			in_ret = fread (ch_val, sizeof (char), in_rcv, psr_hnd);

			if (in_ret < in_rcv)
			{
				inVal = FAILURE;

				break;
			}

			sha1_update (&ctx, (unsigned char *) ch_val, in_rcv);
		}
	}

	if (inVal > FAILURE)
	{
		sha1_finish (&ctx, (unsigned char *) pch_sha);

		memset (&ctx, 0, sizeof (ctx));
	}

	if (inVal > FAILURE)
	{
		int in_ret;

		in_ret = ferror (psr_hnd);

		if (in_ret != SUCCESS)
		{
			inVal = FAILURE;
		}
	}

	if (psr_hnd)
	{
		int in_ret;

		in_ret = fclose (psr_hnd);

		if (in_ret < 0)
		{
//			inVal = FAILURE;
		}
	}

	pdebug (("inVal=0x%08.8x", inVal));

	return inVal;
}

static int in_vfy_hnd (char pchRSA [], char pchSha[], int inSha, char pchSgn[], int inSgn)
{
	int inRetVal;
	rsa_context srRsa;

	SAVE_FUNC_NAME("inRsaVldSgn")

	pdebug((FUNC_NAME_TITL));

	pdump(pchSha, inSha, "sha");
	pdump(pchSgn, inSgn, "sgn");

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
		inRetVal = rsa_pkcs1_verify(&srRsa, RSA_PUBLIC, SIG_RSA_SHA1, 20, (unsigned char *) pchSha, (unsigned char *) pchSgn);
		pdebug(("inRetVal=0x%04.4x", inRetVal));
	}

	rsa_free(&srRsa);

	pdebug(("inRetVal=%d", inRetVal));

	return inRetVal;
}

*/

//Se comenta estas funciones dado que NO aplica para el desarrollo de la Mx9xx
//Estas funciones se encargan de leer la llave RSA para validar la firma de los numeros de serie
//Comento JVelazquez 07/08/14
/*
static int inSavSgn (char pchSgn [], int inSgn)
{
	char ch_sha [50] = {0};
	int inVal;

	SAVE_FUNC_NAME ("inSavSgn")

	pdebug ((FUNC_NAME_TITL));

	pdump (pchSgn, inSgn, "sgn");

	inVal = VS_SUCCESS;

	if (inVal == VS_SUCCESS)
	{
		int inRet;

		inRet = in_sha1_hnd (ch_sha, sizeof (ch_sha), SAV_DWN_NAM, MIN_SAV_CHR);

		if (inRet != VS_SUCCESS)
		{
			inVal = VS_ERROR;
		}
	}

	if (inVal == VS_SUCCESS)
	{
		int inRet;

		inRet = in_vfy_hnd (CPX_CRT_RSA, ch_sha, 20, pchSgn, HEX_MOD_SIZ);

		if (inRet != VS_SUCCESS)
		{
			inVal = VS_ERROR;
		}
	}

//	inVal = VS_SUCCESS;		// debug yo

	pdebug(("inVal=%d", inVal));

	return inVal;
}



static int inSavVld (void)
{
	char chVld [300];
	int inVal;

	SAVE_FUNC_NAME ("inSavVld")

	pdebug ((FUNC_NAME_TITL));

	inVal = VS_SUCCESS;

	if (inVal == VS_SUCCESS)
	{
		int inRet;

		inRet = inSavRed (chVld, MIN_SAV_CHR, 0);

		if (inRet < MIN_SAV_CHR)
		{
			inVal = SAV_ERR_HND;
		}
	}

	if (inVal == VS_SUCCESS)
	{
		long lnSz1;
		long lnSz2;

		lnSz1  = (unsigned char) chVld [0];
		lnSz1 <<= 8;
		lnSz1 |= (unsigned char) chVld [1];
		lnSz1 <<= 8;
		lnSz1 |= (unsigned char) chVld [2];
		lnSz1 <<= 8;
		lnSz1 |= (unsigned char) chVld [3];

		lnSz1 += MIN_SAV_CHR;

		pdebug (("lnSz1=%ld", lnSz1));

		lnSz2 = ln_dir_get_file_sz (SAV_DWN_NAM);

		pdebug (("lnSz2=%ld", lnSz2));

		if (lnSz2 != lnSz1)
		{
			inVal = SAV_ERR_SIZ;
		}
	}

	if (inVal == VS_SUCCESS)
	{
		unsigned long ulCr1;
		unsigned long ulCr2;

		ulCr1  = (unsigned char) chVld [4];
		ulCr1 <<= 8;
		ulCr1 |= (unsigned char) chVld [5];
		ulCr1 <<= 8;
		ulCr1 |= (unsigned char) chVld [6];
		ulCr1 <<= 8;
		ulCr1 |= (unsigned char) chVld [7];

		pdebug (("crc1=0x%08.8x", ulCr1));

		ulCr2 = ulSavCrc ();

		pdebug (("crc2=0x%08.8x", ulCr2));

		if (ulCr2 != ulCr1)
		{
			inVal = SAV_ERR_CRC;
		}
	}

	if (inVal == VS_SUCCESS)
	{
		char chSgn [ASC_MOD_SIZ];

		memset (chSgn, 0, sizeof(chSgn));
		memcpy (chSgn, chVld + 8, HEX_MOD_SIZ);
		pdump (chSgn, HEX_MOD_SIZ, "sgn");

		if ( inSavSgn (chSgn, HEX_MOD_SIZ) )
		{
			inVal = SAV_ERR_SGN;
		}
	}

	pdebug (("inVal=%d", inVal));

	return inVal;
}

static int bo_crt_vld (char pch_srl [], int in_srl)
{
	int bo_fnd = FALSE;
	int in_hnd = -1;
	int in_val;

	SAVE_FUNC_NAME ("bo_crt_vld")

	pdebug ((FUNC_NAME_TITL));

	pdump (pch_srl, in_srl, "srl");

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		if (in_srl < 5)
		{
			in_val = FAILURE;
		}
	}

	if (in_val > FAILURE)
	{
		in_hnd = in_open (SAV_DWN_NAM, O_RDONLY);

		if (in_hnd < 0)
		{
			in_val = FAILURE;
		}
	}

	if (in_val > FAILURE)
	{
		long ln_ret;

		ln_ret = ln_lseek (in_hnd, MIN_SAV_CHR, SEEK_SET);

		if (ln_ret < 0)
		{
			in_val = FAILURE;
		}
	}

	if (in_val > FAILURE)
	{
		char ch_hex [10];
		int  in_ret ;

		while (!bo_fnd)
		{
			memset (ch_hex, 0, sizeof (ch_hex));

			in_ret = in_read (in_hnd, ch_hex, 5);

			if (in_ret < 5)
			{
				break;
			}

			if (!memcmp(ch_hex, pch_srl, 5))
			{
				bo_fnd = TRUE;
			}
		}
	}

	if (in_hnd > -1)
	{
		in_close (in_hnd);
	}

	pdebug (("bo_fnd=%d", bo_fnd));

	return bo_fnd;
}

*/
static int in_srl_hex (char pch_srl [], int in_srl)
{
	char ch_asc [20];
	int  in_val ;

	LOGF_TRACE("--- in_srl_hex ---");


	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		memset (ch_asc, 0, sizeof (ch_asc));

		#ifdef _VRXEVO
			SVC_INFO_SERLNO(ch_asc);
		#else
			svcInfoSerialNum(ch_asc);
		#endif
		
		LOGF_TRACE("---> InfoSerialNum = %s",ch_asc); 

		purge_char (ch_asc, '-');
			
		if (strlen (ch_asc) < 9)
		{
			in_val = FAILURE;
		}
		LOGF_TRACE("---> InfoSerialNum = %s",ch_asc);
	}

	if (in_val > FAILURE)
	{
		memset (pch_srl, 0, in_srl);

		pad (ch_asc, ch_asc, '0', 10, RIGHT_);
		in_srl = inAscToHex (ch_asc, pch_srl, 10);
		LOGF_TRACE("---> Info Serial Num = %i",in_srl);

		in_val = in_srl;
	}

	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_srl_hex ---");

	return in_val;
}

static int in_crt_vld (void)
{
	char ch_hex [10] = {0};
	int  in_val ;

	LOGF_TRACE("--- in_crt_vld ---");

	in_val = SUCCESS;

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = in_srl_hex (ch_hex, sizeof (ch_hex));

		LOGF_TRACE("---> in_ret = %i",in_ret);

		if (in_ret < 5)
		{
			in_val = SAV_ERR_SRL;
		}
	}
	//Se comenta estas funciones dado que NO aplica para el desarrollo de la Mx9xx
	//Estas funciones se encargan de leer la llave RSA para validar la firma de los numeros de serie
	//Comento JVelazquez 11/08/14
    /*

	if (in_val > FAILURE)
	{
		int in_ret;

		in_ret = inSavVld ();

		if (in_ret < SUCCESS)
		{
			in_val = in_ret;
		}
	}
    */

	//Se comenta estas funciones dado que NO aplica para el desarrollo de la Mx9xx
	//Estas funciones se encargan de leer la llave RSA para validar la firma de los numeros de serie
	//Comento JVelazquez 11/08/14
    /*
	if (in_val > FAILURE)
	{
		if (! bo_crt_vld (ch_hex, 5))
		{
			in_val = SAV_ERR_CRT;
		}
	}
    */
	in_val = SUCCESS;		// debug yo

	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_crt_vld ---");

	return in_val;
}

static int in_cpx_vld (void)
{
	static int bo_vld = FALSE;
	static int in_erc = SUCCESS;
	int in_val;

	LOGF_TRACE("--- in_cpx_vld ---");

	if (bo_vld)
	{
		in_val = in_erc;
	}
	else
	{
		in_val = in_crt_vld ();
		in_erc = in_val;
		bo_vld = TRUE;
	}
	
	LOGF_TRACE("---> in_val = %i",in_val);
	
	LOGF_TRACE("--- End in_cpx_vld ---");

	return in_val;
}

int inKeyAbmLoad(void)	
{
	int boRet = 0;

	LOGF_TRACE("--- inKeyAbmLoad ---");
	
	if (inGetEnvVar((char *)"DKPKEYLOD", NULL))
	{
		boRet = 1;
	}

	LOGF_TRACE("---> LLave Inyectada? : %d",boRet);
	
	LOGF_TRACE("--- End inKeyAbmLoad ---");
	
	return boRet;
}

DllSPEC int cryp_msr(char* trk1, char* trk2,char* CVV, bool isFllbck, bool isCtls, int iflgCVV)
{
	int inVal = 0;
	int szTrk1 = 0;
	int szTrk2 = 0;
	int szCVV = 0;
	
	//Variables to perform DUKPT
	char len_hex[1] = {0};
	char len_hex_pan[2] = {0}; 
	char TrackI[S_TRTRK2 + S_TRTRK1] = {0};
	char TrackII[S_TRTRK2] = {0};
	char FechaExp[S_TREXPD*2] = {0};
	char Track1_DUKPT[S_TRTRK1] = {0};
	char Track2_DUKPT[S_TRTRK2+1] = {0};
	int len_trk1_DUKPT = 0;
	int len_trk2_DUKPT = 0;
	
	LOGF_TRACE("--- cryp_msr ---");
	
	szTrk1 = strlen(trk1);
	szTrk2 = strlen(trk2);
	szCVV = strlen(CVV);

	LOGF_TRACE("LONGITUD TRACK1 [%d]",szTrk1);
	LOGF_TRACE("LONGITUD TRACK2 [%d]",szTrk2);
	LOGF_TRACE("LONGITUD CVV    [%d]",szCVV);
	LOGF_TRACE("iflgCVV         [%d]",iflgCVV);
	
	//printf("\nTrack 1: %s   Lenght = %i",trk1,szTrk1);
	//printf("\nTrack 2: %s   Lenght = %i",trk2,szTrk2);
	//printf("\nCVV: %s   Lenght = %i",CVV,szCVV);
	 
	len_trk1_DUKPT = copy_track (Track1_DUKPT, trk1+1, S_TRTRK1);

	// jbf 20250106
	//len_trk2_DUKPT = copy_track (Track2_DUKPT, trk2+1, S_TRTRK2);
	len_trk2_DUKPT = copy_track (Track2_DUKPT, trk2, S_TRTRK2);

	LOGF_TRACE("*******LONGITUD %d",len_trk2_DUKPT);
	LOGF_TRACE("*******TRACK2 %s",Track2_DUKPT);

	//JRS FIX BBVA
	//len_trk2_DUKPT = copy_track (Track2_DUKPT, trk2, S_TRTRK2);
			
	len_hex[0] = '0';
	//Convierte la longitud de dec a hexadecimal
	len_trk1_DUKPT +=2;
	//lendec = LenDec2Hex(len_hex ,len_trk1_DUKPT  );

	//**********************************************
	memset(len_hex_pan,0,sizeof(len_hex_pan));
	sprintf((char *)len_hex_pan,"%x",len_trk1_DUKPT);
	LOGAPI_HEXDUMP_TRACE("---> TrackI_len_trk1", len_hex_pan, 1);
			
	//Valido que se traten de digitos validos
	if( (len_hex_pan[0] >= 0x61) && (len_hex_pan[0] <= 0x66) )
	{
		len_hex_pan[0] = len_hex_pan[0] - 0x20;
	}

	if( (len_hex_pan[1] >= 0x61) && (len_hex_pan[1] <= 0x66) )
	{
		len_hex_pan[1] = len_hex_pan[1] - 0x20;
	}
	    	
	LOGAPI_HEXDUMP_TRACE("---> len_hex_pan:", len_hex_pan,2);
	    	
	//FIX SAMSUNG PAY 
	//TODO: NO ESTOY SEGURO DE QUE TRACK VACIO DEBA ARMAR UN BUFFER CON LONGITUD MAXIMA Y VALORES EN 0
	if(szTrk1 < 16  )
	{
		len_hex[0] = len_hex_pan[0];
		len_hex_pan[0] = '0';
		len_hex_pan[1] = len_hex[0] - 0x20;
	}
	    	
	AscHex((UBYTE *)&len_hex[0],(char *)len_hex_pan,1);
	
	//*********************************************
	TrackI[0] = len_hex[0];
	TrackI[1] = 0x0;
	memcpy(&TrackI[2],Track1_DUKPT,(len_trk1_DUKPT - 2));

	//Se realiza este padding para cumplir con lo requerimientos de la libreria de AJasso.
	len_hex[0] = '0';
	//Convierte la longitud de dec a hexadecimal
	//**********************************************
	memset(len_hex_pan,0,sizeof(len_hex_pan));

			
	LOGF_TRACE("len_trk2_DUKPT = %i",len_trk2_DUKPT);
	//sprintf((char* )len_hex_pan,"%x",len_trk2_DUKPT);
	//JRS FIX  BBVA , CONTEMPLAR BYTES DE LONGITUD PERO DEBEN SER SUMADOS SOLO EN EL BYTE QUE INFORMA DICHA LONGITUD
	sprintf((char* )len_hex_pan,"%x",len_trk2_DUKPT+2);

	LOGAPI_HEXDUMP_TRACE("len_hex_pan(len_trk2_DUKPT)",len_hex_pan,1);
				
	LOGAPI_HEXDUMP_TRACE("TrackI_len_trk2",len_hex_pan, 1);

	//Valido que se traten de digitos validos
	if( (len_hex_pan[0] >= 0x61) && (len_hex_pan[0] <= 0x66) )
	{
	len_hex_pan[0] = len_hex_pan[0] - 0x20;
	}

	if( (len_hex_pan[1] >= 0x61) && (len_hex_pan[1] <= 0x66) )
	{
		len_hex_pan[1] = len_hex_pan[1] - 0x20;
	}
	    	
	LOGAPI_HEXDUMP_TRACE("len_hex_pan:",len_hex_pan,2);
	   	
	 //FIX SAMSUG PAY
	 //TODO: NO ESTOY SEGURO DE QUE ESTA VALIDACION DEBA DE ESTAR
  	/*if(szTrk1 < 16)
   	{
  		len_hex[0] = len_hex_pan[0];
   		len_hex_pan[0] = '0';
   		len_hex_pan[1] = len_hex[0] - 0x20;
   	}*/
   	LOGAPI_HEXDUMP_TRACE("len_hex_pan:",len_hex_pan,2);

    AscHex((UBYTE *)&len_hex[0],(char *)len_hex_pan,1);

   	//*********************************************
   	LOGF_TRACE("len_hex[0] = %i",len_hex[0]);
   	TrackI[len_trk1_DUKPT] = len_hex[0];
   	len_trk1_DUKPT++;
   	TrackI[len_trk1_DUKPT] = 0x0;
   	len_trk1_DUKPT++;
   	//Se agrega informacion de Track II
   	memcpy(&TrackI[len_trk1_DUKPT],Track2_DUKPT,len_trk2_DUKPT);
   	len_trk1_DUKPT+=(len_trk2_DUKPT);
   	//Se realiza este padding para cumplir con lo requerimientos de la libreria de AJasso.
   	TrackI[len_trk1_DUKPT] = 0x02;
   	len_trk1_DUKPT++;
   	TrackI[len_trk1_DUKPT] = 0x01;
   	len_trk1_DUKPT++;
			
	//JRS FIX BBVA
	//len_trk1_DUKPT+=2;
   	//dump (TrackI, len_trk1_DUKPT, "TrackI:");
   	LOGAPI_HEXDUMP_TRACE((char *)"TRACK_ALL:",TrackI,len_trk1_DUKPT);
		
	//inVal = in_cpx_crd_mag (&TrackI[0],len_trk1_DUKPT, FALSE, CVV,szCVV);

	inVal = in_cpx_crd_mag (&TrackI[0], len_trk1_DUKPT, isFllbck, CVV, szCVV, isCtls, iflgCVV);
	//printf("\ninVal = %i",inVal);
	
	//printf("\n--End of cryp_msr--");
	
	return inVal;
}

DllSPEC int inAskBinTbl(char *chBinId,char *chBinVer, char *chBinRange)
{
	int inRet= 0;
	
	LOGF_TRACE("--- inAskBinTbl ---");
	
	inRet = inAskBinTable(chBinId,chBinVer,chBinRange);
	LOGF_TRACE("---> inAskBinTbl [%d]",inRet);

	LOGF_TRACE("--- End inAskBinTbl ---");
	
	return inRet;
}

DllSPEC void vdSetBanco(int inBank)
{
	LOGF_TRACE("--- vdSetBanco ---");
	inBanco = inBank;
	LOGF_TRACE("--- End vdSetBanco ---");
}

int  intGetBanco(void)
{
	LOGF_TRACE("--- intGetBanco ---");
	LOGF_TRACE("--- End intGetBanco ---");
	return inBanco;
}


// FAG 17-nov-2017

DllSPEC void vdsetflagBIN(int inFlag)
{
	//flagBIN=0;
	flagBIN = inFlag;
	LOGF_TRACE("flagBINset %i",flagBIN);
}
DllSPEC int inGetflagBIN(void)
{
	
	LOGF_TRACE("flagBINGet %i",flagBIN);
	return flagBIN;
}


DllSPEC void vdSet_entrymode(int tech)  // FAG 14-mayo-2018
{
	entry_mode=tech;

}
DllSPEC int inGet_entrymode(void)  // FAG 14-mayo-2018
{
	return entry_mode;
}

int getModel(char *  value )
{
	int r=0;
	string modelname;
	
	r = sysGetPropertyString(SYS_PROP_HW_MODEL_NAME,modelname);
	sprintf(value,"%s",modelname.c_str());	

	LOGF_TRACE("MODEL NAME [%s]",value);
	
	return r;
}



//Migrado por JVelazquez 22/08/14
#endif //MAKE_DUKPT
