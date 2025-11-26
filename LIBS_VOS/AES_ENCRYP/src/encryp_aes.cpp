/*
 * 
 *
 * Paradox .-  FERNANDO MARTIN AYALA GARCIA  03-NOV-2016
 */

//SYSTEM INCLUDES
#include <string>
#include <stdio.h>
#include <stdlib.h>

#ifdef _VRXEVO
#include <stdlib.h> 
#else
#include <malloc.h> 
#endif

#include <cstring>
#include <time.h>
#ifndef _VRXEVO
#include <sys/time.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef __cplusplus
  extern "C++" {
		#include <math.h>
  }
  #endif

//ADK INCLUDES
#include "html/gui.h"
#include "msr/msr.h"
#include <log/liblog.h>
#include <sysinfo/sysinfo.h>
#include <sysinfo/sysbar.h>
#include <sysinfo/sysbeep.h>


//SDK INCLUDES
#ifdef _VRXEVO
#include <svc_sec.h>
#else
#include "svcsec.h"
#endif
#include <svc.h>


//USER INCLUDES
#include "encryp_aes.h"

//LOCAL
int _getEnvFile_(char *chSection,char *chVar, char *chValue, int inLen );
int _putEnvFile_(char *chSection,char *chVar, char *chValue);

//Globals Vars
Rqkrnd KeyRnd;
CapXKy KeyCx;

/*AES CBC*/		//Daee 19/05/2014
//char InitVector[]={'\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0'};


using namespace std;
using namespace vfigui;
using namespace vfisysinfo;

 unsigned char key_inyected;			  //Almacena el status de la inyeccion de la llave, 00 para cuando esta inyectada la llave, 00 en otro caso.
 unsigned char transpKey[32];		  //Almacena la llave de transporte.
 unsigned char PruebasInternas;		  //Para el caso de que NO SE REQUIERA validar, numero de serie, CRC32.
		  char InitVector[AES_SZB]={0};	  //Almacena el vector inicial, para este caso sera con 0x0;
 		  char KeyAES[AES_SZB*2];	  //Almacena la llave para encriptar;
	         char InputAES[AES_SZB*10];	  //Almacena el bloque inicial para encriptar
 		  char OutputAES[AES_SZB*10];  //Almacena el bloque final para encriptar
 		  int Bloques;              	  //Numero de bloques a cifrar.




//=============================================================================
// crc32_
// Algoritmo para calcular el CRC32 de un mensaje
//
// Regresa:
//          ~crc : Resultado del Algoritmo
//
//  JVelazquez
//=============================================================================

unsigned long crc32_funtion( unsigned char *message, int longitud )
{

int i, j;
unsigned long crc, mask;
unsigned char byte;
i = 0;
crc = 0xFFFFFFFF;
//DWORD poly = 0xedb88320L;
//LONG poly = 0xedb88320L;  // FAG 03-nov-2016
signed long int poly = 0xedb88320L;

LOGF_TRACE("crc32_funtion");


for( i = 0; i < longitud; i++ )
{
    byte = message[i]; // Get next byte.
    crc = crc ^ byte;
        for (j = 7; j >= 0; j--)
        { // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ ( poly & mask );
        }
}

return ~crc;
}
//=============================================================================
// getCRC32Data
//
//  JVelazquez
//=============================================================================
void getCRC32Data( unsigned char* InputData, unsigned char* OutputData, unsigned long nLen  )
{

	unsigned char aux_CRC32Give[4];
    memset( aux_CRC32Give, 0, sizeof( aux_CRC32Give ) );

	LOGF_TRACE("getCRC32Data");

    // Se obtiene el CRC32
    unsigned long Give_CRC32 = crc32_funtion( InputData, nLen );

    memcpy( aux_CRC32Give, &Give_CRC32, sizeof(aux_CRC32Give) );
    OutputData[0] = aux_CRC32Give[3];
    OutputData[1] = aux_CRC32Give[2];
    OutputData[2] = aux_CRC32Give[1];
    OutputData[3] = aux_CRC32Give[0];

}
//=============================================================================
// In_XOR
//
//  JVelazquez
//=============================================================================
void In_XOR( unsigned char * OutputData,unsigned char* InputData, unsigned char* InputData2, unsigned int nLen )

{
  unsigned char i;
  char A,B,C,D;

  LOGF_TRACE(" FUNCION In_XOR");


  for(i = 0;i<nLen; i++)
  {
	  //Primer paso
	  A = ~InputData[i];
	  B = InputData2[i];
	  C = A&B;
	  //Segundo paso
	  A = InputData[i];
	  B = ~InputData2[i];
	  D = A&B;

	  A = C|D;

	  OutputData[i] = A;
  }

 LOGF_TRACE("Resultado final XOR = %s ", OutputData);

}
//=============================================================================
// FormaBloques16 	Se encarga de formar el buffer con los bloques necesarios
//					para el cifrado
//
// input:			Buffer de entrada
// output:			Buffer de salida
// len:				longitud del buffer de entrada
//
// Regresa:			El numero de bloque que se formaron.
//  JVelazquez
//=============================================================================

int FormaBloques16(unsigned char input[], unsigned char len, unsigned char output[] )
{
	int i;
	unsigned char Bloque = 16;
	int resultado = 0;
	memset(output,0,sizeof(output));
	LOGF_TRACE("FUNCION  FormaBloques16");
	LOGF_TRACE("Input = %s ",input,len);
	LOGF_TRACE("len = %d ", len);

	if(len <= Bloque )
	{
		memcpy(output,input,len);
		//Realizo el padding con espacios
		for(i = 0; i < (Bloque - len); i++)
		{
			output[i + len] = 0x20;
		}

		LOGF_TRACE("output = %s ",output);
		LOGF_TRACE("Un Bloque output = %s ", output);
		return 1;
	}
	else
	{
		resultado = ceil((float)(len/Bloque)); // falta poner esta linea, solo se comentó para compilar y probar lo demas

        if( (len - (Bloque * resultado) ) > 0 )
        {
        	resultado++;

        	//Realizo el padding con espacios
        	for(i = 0; i < ( (Bloque * resultado) - len ); i++)
        		output[i + len] = 0x20;

        }

		memcpy(output,input,len);
		return (resultado);
	}

}

//=============================================================================
// LenDec2Hex	 	Se encarga de convertir la longitud decimal en hexadecimal
//
//
// lendec:			Longitud decimal
//
// Regresa:			Longitud en hexadecimal
//  JVelazquez
//================================================================== ===========
extern int LenDec2Hex(char *LenHex, int lendec )
{
	int Bloque;
	char lenhex[2];
	unsigned char len[1];

	len[0] = '0';
	memset(lenhex,0,sizeof(lenhex));
	//Siempre sera un bloque de 16
	Bloque = lendec/16;

	sprintf(&lenhex[0],"%i",Bloque);
	lenhex[1] = '0';



	//ASCtoHEX((char *)len,lenhex,1);
    LOGF_TRACE(" lendec: %i",lendec);
    LOGF_TRACE("LenHex: %s",len);
    memcpy(LenHex,len,1);
	return( 10 * Bloque );
}

//=============================================================================
// StatusLlave	 	Se encarga regresar el status de la llave de cifrado
//
//
//
//
// Regresa:			status de la llave 1 cuando NO esta cargada y 0 cuando esta cargada.
//  JVelazquez
//================================================================== ===========

int StatusLlave( void )
{
	int fp;
	char KeyAES_[32];
	unsigned char status;

	memset(KeyAES_,0x0,32);
	LOGF_TRACE("StatusLlave");
	fp = open( FILE_AES,  O_RDWR, 0666 );


	if(fp == -1 )
    {
    	
		LOGF_TRACE(" Fallo apertura key_AES");
		
        return FAILURE_OPEN_FILE;  // indicamos que hubo una falla al cargar la llave de archivo

	 }

	LOGF_TRACE("Antes de lectura se extra la llave");
	#ifdef _VRXEVO
		status = crypto_read(fp,KeyAES_,32);
	#else
		status = cryptoRead(fp,KeyAES_,32);
	#endif
	LOGF_TRACE("CLOSE FILE");
	close(fp);
	//Fallo apertura
	 if(status == 0xFF)
	 	{
	 		memset(KeyAES_, 0x00, sizeof(KeyAES_));
		 	return FAILURE_CRYPTOREAD; 	
	 	}
		
	 else
	 {
		//Pregunto si el archivo esta vacio
		if(KeyAES_[0] == 0x00)
			{
				memset(KeyAES_, 0x00, sizeof(KeyAES_));
				return FAILURE_CRYPTOREAD;
			}
			
		else //El archivo tiene una llave almacenada.
			status = 0;
	 }
	memset(KeyAES_, 0x00, sizeof(KeyAES_));
	LOGF_TRACE("StatusLlave [%d]",status);
	return status;
}



int HEXtoASC(unsigned char *source, unsigned int LenPacket, unsigned char *dest, unsigned int *BytesWritten)
{	
	int i,j;
	j=0;

 	LOGF_TRACE("HEXtoASC");
	
	for(i=0; i<LenPacket; i++)
	{
		if(((source[i]>>4)<=9) && ((source[i]>>4)>=0))
		dest[j] = ((source[i]>>4) & 0x0f) + '0';
		else
		dest[j] = (source[i]>>4)-0x0a + 'A';

		j++;
		if(((source[i]) & 0x0f)<=9 && ((source[i]) & 0x0f)>=0)
		dest[j] = ((source[i]) & 0x0f) + '0';
		else
		dest[j] = ((source[i]) & 0x0f)-0x0a + 'A';
		j++;
	}
	

	*BytesWritten = j;

	return(0);
}



int ASCtoHEX(char * pchAsc, int inAsc, char * pchOut, int * inOut)
{
	char * pos = pchAsc;
	unsigned char chPadAsc[2048];
    size_t count = 0;

	if(strlen(pchAsc) % 2 != 0){
		sprintf((char *)chPadAsc,"0%s",pchAsc);
		inAsc++;
	}
	else
		sprintf((char *)chPadAsc,"%s",pchAsc);

    for(count = 0; count < strlen(pchAsc) / 2; count++) {
        sscanf(pos, "%2hhx", &pchOut[count]);
        pos += 2;

		if(count * 2 > inAsc)
			break;
    }

	*inOut = count;	

    return(0);
	
}


int inGenerateRandom (unsigned char *random8)
{
	int inRet;
	inRet = GenerateRandom( random8 );
	return inRet;
}

int inAES(unsigned char ucAesOption, unsigned char *pucAesKey8N,unsigned char *pucInputData, unsigned char *pucOutputData)
{

	
	int inRet;
	inRet = AES(ucAesOption,pucAesKey8N,pucInputData,pucOutputData);
	return inRet;
}


int incryptoWrite (int hdl, const char *buf, int len)
{
	int inRet;
	#ifdef _VRXEVO
		inRet = crypto_write(hdl, buf, len);
	#else
		inRet = cryptoWrite(hdl, buf, len);
	#endif
	return inRet;

}

DllSPEC int incryptoRead (int hdl, char *buf, int len)
{
	int inRet;
	#ifdef _VRXEVO
		inRet = crypto_read(hdl, buf, len);
	#else
		inRet = cryptoRead(hdl, buf, len);
	#endif
	return inRet;

}

// CODIGO DE DIEGO ESCALERA

int in_Cx_Krq(Rqkrnd *KRQ)
 {
	int in_Ret = VS_ERROR_AES;
	unsigned long ulCrc;
	char ch_asc_crc32[15]={0};
	int  in_asc_crc32=0;
	
	LOGF_TRACE("in_Cx_krq");

	in_Cx_KRmv(FALSE_AES);		// AJM 05/09/2014 1

	if((KeyRnd.inRndKey = inKRnd(KeyRnd.chRndKey,SIZE_KEY_RND)) > 0)			//Rndm key
	{
		ulCrc = ulcrc32(KeyRnd.chRndKey, SIZE_KEY_RND, 0xFFFFFFFF);			//crc32
	
		in_asc_crc32= inHex2Asc((char *) &ulCrc, ch_asc_crc32, SIZE_CRC_32_HEX);

		KeyRnd.inCrc32 = inAsc2Hex(ch_asc_crc32,KeyRnd.chCrc32,in_asc_crc32);

		memcpy(KRQ,&KeyRnd,sizeof(KeyRnd));

		in_Ret = VS_SUCCESS_AES;
	}
	
	//LOGAPI_HEXDUMP_TRACE("chRndKey:", KRQ->chRndKey, KRQ->inRndKey);
	//LOGAPI_HEXDUMP_TRACE("chCrc32:" , KRQ->chCrc32 , KRQ->inCrc32);
	
	LOGF_TRACE("in_Cx_krq ret --> %d",in_Ret);
	return in_Ret;
 }




 DllSPEC int in_Cx_Kld(LdkDat *KLD)
 {
	int in_Ret = VS_SUCCESS_AES;
	LOGF_TRACE("in_Cx_Kld");
	//LOGAPI_HEXDUMP_TRACE("srln", KLD->chSrlN, KLD->inSrlN);
	//LOGAPI_HEXDUMP_TRACE("enck", KLD->chEncK,KLD->inEncK);
	//LOGAPI_HEXDUMP_TRACE("crc32",KLD->chCrc32,KLD->inCrc32);


	if(in_Ret >= VS_SUCCESS_AES)//Serial Number validate
	{
		in_Ret = inVldSrlN(KLD->chSrlN,KLD->inSrlN);	
	}
	
	if(in_Ret >= VS_SUCCESS_AES)//Decrypt key
	{
		in_Ret = inDcrptk(KLD->chEncK,KLD->inEncK);
	}

	if(in_Ret >= VS_SUCCESS_AES)//Check Value validate
	{
		in_Ret = inVldKrchV(KLD->chCrc32,KLD->inCrc32);
	}
	
	if(in_Ret >= VS_SUCCESS_AES)//Saved Key
	{
//		in_remove ("ExcCrd.dat");		// AJM 26/08/2014 1		// AJM 03/09/2014 1

		in_Ret = in_Cx_KSav();
	}

	if(in_Ret < VS_SUCCESS_AES)
	{
		in_Cx_KRmv (FALSE_AES);		// AJM 05/09/2014 1
	}

	LOGF_TRACE("in_Cx_Kld ret --> %d",in_Ret);
	return in_Ret;
 }

DllSPEC int in_Cx_fkld(void)
{
	char bo_Ret = FALSE_AES;
	char kld[5]={0};
	LOGF_TRACE("in_Cx_fklded");

	if(inGetEnvStr((char *)KCX_FLOAD,kld,sizeof(kld)) == 1)
	{
		if(kld[0]=='1')
		{
			bo_Ret = TRUE_AES;
		}
	}
	
	LOGF_TRACE("in_Cx_fklded -> boRet [%d]",bo_Ret);
	return bo_Ret;
}

int boKeyLod(void)		//Daee 29/10/2014  /*Vld Key Load AMB*/
{
	char buf[3] = {0};
    int ret = 0;
	int bo_inj_key = FALSE_DUKPT;

	inGetEnvStr((char *)"DKPKEYLOD",buf,1);
	//ret = get_env("#DKPKEYLOD",buf,1);

//	if(ret > 0)
//	{
	   if(atoi(buf) == 1) //LLave inyectada
	   {
	   	bo_inj_key = TRUE_DUKPT;
	   }	   
	//}
	
	return (bo_inj_key);
}


/***********************************************************
* in_Cx_EDta : Cifrado AES key-256 bits  (Hex_data)
*
*
************************************************************/
DllSPEC int in_Cx_EDta(char pchData[] ,int inData,char PchDOut[], int *inDOut)
{
	int  in_Ret = VS_SUCCESS_AES;
	char chDtaEnc[1024]={0};
	int  inDtaEnc      = 0 ;
	char chDtaDec[1024]={0};
	int  inDtaDec      = 0;
	int  inDtaBlocks   = 0;
	
	LOGF_TRACE("in_Cx_EDta");

	//LOGAPI_HEXDUMP_TRACE( "dcp",pchData, inData);

	//LOGAPI_HEXDUMP_TRACE("Data", pchData, inData);

	in_Ret = in_Cx_KLod();

	if(in_Ret >= VS_SUCCESS_AES)
	{
		inDtaBlocks = inPadAes(pchData,inData,chDtaEnc,&inDtaEnc);
		if(inDtaBlocks == 0)
		{
			in_Ret = VS_ERROR_AES;
		}
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{
		in_Ret = inAEScbc256(AES_ENC,KeyCx.chKeyCX,chDtaEnc,chDtaDec,inDtaBlocks);
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{
		inDtaDec = inDtaBlocks*AES_SZB;
		memcpy(PchDOut,chDtaDec,inDtaDec);
		*inDOut = inDtaDec;
		//LOGAPI_HEXDUMP_TRACE( "crp",PchDOut, *inDOut);
	}
	else
	{
		in_Ret = CX_AESED_ERR;
	}
	

	
	memset(&KeyCx ,0,sizeof(KeyCx));
	
	LOGF_TRACE("in_Cx_EDta ret --> %d",in_Ret);
	
	return in_Ret;
}

/***********************************************************
* in_Cx_DDta : Descifrado AES key-256 bits  (Hex_data)
*
*
************************************************************/
int in_Cx_DDta(char pchData[] ,int inData,char PchDOut[], int *inDOut)		// AJM 26/08/2014 1
{
	int  in_Ret = VS_SUCCESS_AES;
	char chDtaEnc[1024]={0};
	int  inDtaEnc       = 0;
	char chDtaDec[1024]={0};
	int  inDtaDec       = 0;
	int  inDtaBlocks    =0;
	
	LOGF_TRACE("in_Cx_DDta");

	//LOGAPI_HEXDUMP_TRACE("dcp", pchData, inData);
	in_Ret = in_Cx_KLod();

	if(in_Ret >= VS_SUCCESS_AES)
	{
		inDtaBlocks = inPadAes(pchData,inData,chDtaEnc,&inDtaEnc);
		if(inDtaBlocks == 0)
		{
			in_Ret = VS_ERROR_AES;
		}
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{
		in_Ret = inAEScbc256(AES_DEC,KeyCx.chKeyCX,chDtaEnc,chDtaDec,inDtaBlocks);
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{
		inDtaDec = inDtaBlocks*AES_SZB;
		memcpy(PchDOut,chDtaDec,inDtaDec);
		*inDOut = inDtaDec;
		//LOGAPI_HEXDUMP_TRACE("crp", PchDOut, *inDOut);
	}
	else
	{
		in_Ret = CX_AESED_ERR;
	}
	

	
	memset(&KeyCx ,0,sizeof(KeyCx));
	
	LOGF_TRACE("in_Cx_EDta ret --> %d",in_Ret);
	
	return in_Ret;
}

int in_Cx_GCrc(char pchCrc32[], int *inCrc32)
{
	int  in_Ret = VS_ERROR_AES;
	unsigned long ulCrc;
	char ch_asc_crc32[20]={0};
	int  in_asc_crc32;
	char ch_hex_crc32[20]={0};
	int  in_hex_crc32;
	LOGF_TRACE("in_Cx_GCrc");

	in_Ret = in_Cx_KLod();

	if(in_Ret >= VS_SUCCESS_AES)
	{	
		ulCrc = ulcrc32(KeyCx.chKeyCX, SIZE_KEY_CX, 0xFFFFFFFF);			//crc32
	
		in_asc_crc32= inHex2Asc((char *) &ulCrc, ch_asc_crc32, SIZE_CRC_32_HEX);

		in_hex_crc32 = inAsc2Hex(ch_asc_crc32,ch_hex_crc32,in_asc_crc32);

		memcpy(pchCrc32,ch_hex_crc32,in_hex_crc32);
		*inCrc32 = in_hex_crc32;
	}
	
	memset(&KeyCx ,0,sizeof(KeyCx));
	LOGF_TRACE("in_Cx_GCrc ret --> %d",in_Ret);
	
	return in_Ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
int in_Cx_KRmv(int bo_sav)		// AJM 05/09/2014 1
{
	int in_Ret;

	LOGF_TRACE("in_Cx_KRmv");

	in_Ret = VS_SUCCESS_AES;

	memset(&KeyRnd,0,sizeof(KeyRnd));
	memset(&KeyCx ,0,sizeof(KeyCx));

	if (bo_sav)
	{
		inSetEnvStr((char *)KCX_FLOAD,(char *)"0",1);
		in_Ret = in_Crp_Sav((char *)FILE_AES, &KeyCx, sizeof(KeyCx));
	}
	
	LOGF_TRACE("in_Ret=%d", in_Ret);

	return in_Ret;
}

int in_Cx_KSav(void)
{
	int in_Ret = VS_ERROR_AES;

	LOGF_TRACE("in_Cx_KSav");

	in_Ret = in_Crp_Sav((char *)FILE_AES, &KeyCx, sizeof(KeyCx));

	if(in_Ret >= VS_SUCCESS_AES)
	{
		inSetEnvStr((char *)KCX_FLOAD,(char *)"1",1);
	}
	else
	{
		in_Ret = CX_KSAVE_ERR;
	}
	memset(&KeyCx ,0,sizeof(KeyCx));
	
	memset(&KeyRnd,0,sizeof(KeyRnd));
	
	LOGF_TRACE("in_Ret=%d", in_Ret);

	return in_Ret;
}

int in_Cx_KLod(void)
{
	int in_Ret = VS_ERROR_AES;

	LOGF_TRACE("in_Cx_KLod");

	memset(&KeyCx ,0,sizeof(KeyCx));

	in_Ret = in_Crp_Lod((char *)FILE_AES, &KeyCx, sizeof(KeyCx));

	if(in_Ret < VS_SUCCESS_AES)
	{
		inSetEnvStr((char *)KCX_FLOAD,(char *)"0",1);
		in_Ret = CX_KLOAD_ERR;
	}

	//inSetEnvStr((char *)KCX_FLOAD,(char *)"0",1);	
	//inSetEnvStr((char *)KCX_FLOAD,(char *)"1",1);	
	LOGF_TRACE("in_Ret=%d", in_Ret);

	return in_Ret;
	
}



DllSPEC int in_Crp_Sav(char pchFil[], void *pvdDst, int inChr)		// AJM 03/02/2010 1
{
	int inHdl;
	int inRetVal;
	int inTmp0;

	LOGF_TRACE("inCrpSav");

	LOGF_TRACE("pchFil=%s", pchFil);

	//LOGAPI_HEXDUMP_TRACE("pvdDst:", pvdDst, inChr);

	inHdl = open(pchFil, O_CREAT | O_TRUNC | O_RDWR , 0666); //dgms agrego o_trunc
	LOGF_TRACE("inHdl=%d", inHdl);
	if (inHdl < 0)
	{
		return VS_ERROR_AES;
	}
	#ifdef _VRXEVO
		inTmp0 = crypto_write(inHdl, (const char *)pvdDst, inChr);
	#else
		inTmp0 = cryptoWrite(inHdl, (const char *)pvdDst, inChr);
	#endif
	LOGF_TRACE("inTmp0=%d", inTmp0);
	close(inHdl);

	inRetVal = VS_SUCCESS_AES;
	if (inTmp0 < inChr)
		inRetVal = VS_ERROR_AES;

	LOGF_TRACE("inRetVal=%d", inRetVal);

	return inRetVal;
}

int in_Crp_Lod(char pchFil[], void *pvdDst, int inChr)
{
	int inHdl;
	int inRetVal;
	int inTmp0;

	LOGF_TRACE("inCrpLod");

	LOGF_TRACE("pchFil=%s inChr=%d", pchFil, inChr);

	inHdl = open(pchFil, O_RDONLY);
	LOGF_TRACE("inHdl=%d", inHdl);
	if (inHdl < 0)
	{
		return VS_ERROR_AES;
	}

	memset(pvdDst, 0, inChr);
	#ifdef _VRXEVO
		inTmp0 = crypto_read(inHdl, (char *)pvdDst, inChr);
	#else
		inTmp0 = cryptoRead(inHdl, (char *)pvdDst, inChr);
	#endif
	LOGF_TRACE("inTmp0=%d", inTmp0);
	close(inHdl);

	//LOGAPI_HEXDUMP_TRACE("pvdDst:", pvdDst, inTmp0 > 0 ? inTmp0 : 0);

	inRetVal = VS_SUCCESS_AES;
	if (inTmp0 < inChr)
		inRetVal = VS_ERROR_AES;

	LOGF_TRACE("inRetVal=%d", inRetVal);

	return inRetVal;
}



int inKRnd(char pchKrnd[], int inKrnd)
 {
 	
	int inRet;
	 
	LOGF_TRACE("inKRnd");
	#if defined (HRD_COD_RSP)
	{
		
	 	char temp[50]={0};
		LOGF_TRACE("if HDR_COd_RSP rnd number of file");
	 	if(inGetEnvStr((char *)"KEYRND",temp,sizeof(temp)) == inKrnd*2)
	 	{
			inAsc2Hex(temp,pchKrnd,inKrnd*2);
	 	}
	 	else
	 	{
	 		return 0;
	 	}
	}
	#else

	//srand(time(NULL));
	inRet = inGenerateRandom((unsigned char*) pchKrnd );
//	//LOGAPI_HEXDUMP_TRACE("Key part 1",pchKrnd,sizeof(pchKrnd));
	LOGF_TRACE("inRet: %i",inRet);
	
	inRet = inGenerateRandom((unsigned char*)&pchKrnd[8]); 
	LOGF_TRACE("inRet: %i",inRet);
//	//LOGAPI_HEXDUMP_TRACE("Key part 2",pchKrnd,sizeof(pchKrnd));
/*
 
	 for (inIdx = 0; inIdx < inKrnd; inIdx++)
	 {
		 pchKrnd[inIdx] = rand() & 0xFF;
	 }
 */
	#endif
 
	//LOGAPI_HEXDUMP_TRACE("pchKrnd:",pchKrnd, inKrnd);
 
	return inKrnd;
 }

 


int inVldKrchV(char pchcrc32[], int incrc32)		//Daee 19/05/2014
{
	int  in_Ret = VS_SUCCESS_AES;
	unsigned long ulCrc;
	char ch_asc_crc32[20]={0};
	int  in_asc_crc32;
	char ch_hex_crc32[20]={0};
	int  in_hex_crc32;
	LOGF_TRACE("inVldKrchV");

	ulCrc = ulcrc32(KeyCx.chKeyCX, SIZE_KEY_CX, 0xFFFFFFFF);			//crc32
	
	in_asc_crc32= inHex2Asc((char *) &ulCrc, ch_asc_crc32, SIZE_CRC_32_HEX);

	in_hex_crc32 = inAsc2Hex(ch_asc_crc32,ch_hex_crc32,in_asc_crc32);

	//LOGAPI_HEXDUMP_TRACE("Crc32 Calculed", ch_hex_crc32,in_hex_crc32);
	//LOGAPI_HEXDUMP_TRACE("Crc32 Received", pchcrc32,incrc32);

	#if defined (HRD_COD_RSP)
	LOGF_TRACE("HDR_COd_RSP esta definido  PARA CRC32************");//cambio
	if(0)
	
	#else
	if(memcmp(ch_hex_crc32,pchcrc32,in_hex_crc32))
	#endif
	{
		in_Ret = CX_CRC32_ERR;
	}

	LOGF_TRACE("inVldKrchV ret --> %d",in_Ret);
	return in_Ret;
}



int inVldSrlN(char SrlN[], int inSrlN)		//Daee 16/05/2014
{
	string serial_number;
	char POSSN[16+1];
	int inRet = VS_SUCCESS_AES;
	
	LOGF_TRACE("inVLDSRLN");

	//Obtengo el numero de serie
	sysGetPropertyString(SYS_PROP_HW_SERIALNO,serial_number);
	memset(POSSN, 0x00, sizeof(POSSN)); 
	strcpy(POSSN,serial_number.c_str());
	inpurge_char(POSSN, '-'); 
	LOGF_TRACE("--SERIALNO [%s] --",POSSN); 

	if(!memcmp(POSSN, SrlN, inSrlN ) )
	{
		LOGF_TRACE("****NUMERO DE SERIE COINCIDE***"); 
		// numero de serie correcto  procedemos con el flujo
		inRet =VS_SUCCESS_AES;
	}
	
	else
	{
		LOGF_TRACE("****NUMERO DE SERIE NO COINCIDE***");
		// falla numero de serie, mandamoe el codigo de error correspondiente
		inRet = CX_SRLND_ERR;
	}
		
	
	LOGF_TRACE("inVldSrlN ret --> %d",inRet);
	return inRet;
	
}


int inDcrptk(char kEnc[], int inkEnc)
{
	int  in_Ret = VS_ERROR_AES;
	char key   [32 + 1]={0};
	char keyDec[32 + 1]={0};
	int  inkey = 0;
	char ktemp[35]={0};
	char TsrlN[20]={0};
	string serial_number;
	char POSSN[16+1];
	
	LOGF_TRACE("inDcrptk");
	//LOGAPI_HEXDUMP_TRACE("keyenc",kEnc,inkEnc);

	if(KeyRnd.inRndKey > 0)
	{
		memcpy(key,KeyRnd.chRndKey,KeyRnd.inRndKey);
		inkey += KeyRnd.inRndKey;
		//LOGAPI_HEXDUMP_TRACE("key1",key,inkey);
		in_Ret = VS_SUCCESS_AES;
	}
	
	if(in_Ret >= VS_SUCCESS_AES)
	{
		/*
		#if defined (HRD_COD_RSP_CX)
		LOGF_TRACE("if HDR_COd_RSP_CX esta definido ****************************************");//cambio
		if(inGetEnvStr("#TSRLN",TsrlN,sizeof(TsrlN)) <= 0) //cambio esta es la variable 
		{
			in_Ret = VS_ERROR_AES;
		}
		#else
		*/
		//Obtengo el numero de serie
		sysGetPropertyString(SYS_PROP_HW_SERIALNO,serial_number);
		memset(POSSN, 0x00, sizeof(POSSN)); 
		strcpy(POSSN,serial_number.c_str());
		inpurge_char(POSSN, '-'); 
		LOGF_TRACE("--SERIALNO [%s] --",POSSN); 

		//#endif
		
	    	aes_pad(ktemp,POSSN,' ',16,LEFT_AES);
		//LOGAPI_HEXDUMP_TRACE("key2",ktemp,16);
	
		memcpy(key + inkey,ktemp,16);
		inkey += 16;
		//LOGAPI_HEXDUMP_TRACE("key",key,inkey);
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{		
		#if defined (HRD_COD_RSP)
		char KTEMPHC[70+1]={0};
		LOGF_TRACE("HDR_COd_RSP esta definido ****************************************");//cambio
		if(inGetEnvStr((char *)"KCXCL",KTEMPHC,sizeof(KTEMPHC)) == SIZE_KEY_CX*2)
		{
			inAsc2Hex(KTEMPHC,keyDec,SIZE_KEY_CX*2);
		}
		else if(inGetEnvStr((char *)"KCXAE",KTEMPHC,sizeof(KTEMPHC)) == SIZE_KEY_CX*2)
		{
			char KTEMPhex[70+1]={0};
			inAsc2Hex(KTEMPHC,KTEMPhex,SIZE_KEY_CX*2);
			in_Ret = inAEScbc256(AES_DEC,key,KTEMPhex,keyDec,2);
		}
		else
		{
			in_Ret = VS_ERROR_AES;
		}
		#else
		
		in_Ret = inAEScbc256(AES_DEC,key,kEnc,keyDec,2);
		
		#endif
	}

	if(in_Ret >= VS_SUCCESS_AES)
	{
		//LOGAPI_HEXDUMP_TRACE("DEC", keyDec,sizeof(keyDec));
		memcpy(KeyCx.chKeyCX,keyDec,SIZE_KEY_CX);
	}
	else
	{
		in_Ret = CX_DECKR_ERR;
	}

	LOGF_TRACE("inDcrptk ret --> %d",in_Ret);
	return in_Ret;
	
}




unsigned long ulcrc32 (char pch_crc [], int in_crc, unsigned long ul_sed)
{
	unsigned long byte, crc, mask;
	unsigned char * puc_crc;
	int i, j;
	unsigned char CRC32[4];
	unsigned char OutputCRC32[4];

    LOGF_TRACE("ul_crc_32");
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

#ifndef _VRXEVO
	//Se adiciona esta seccion por el tema de big-Endian (El resultado esta invertido)
	//Agrego JVelazquez 19/08/14
	memset(CRC32,0,sizeof(CRC32));
	memset(OutputCRC32,0,sizeof(OutputCRC32));
	memcpy(CRC32,&crc,sizeof(CRC32));
	OutputCRC32[0] = CRC32[3];
	OutputCRC32[1] = CRC32[2];
	OutputCRC32[2] = CRC32[1];
	OutputCRC32[3] = CRC32[0];

	memcpy(&crc,OutputCRC32,sizeof(CRC32));
#endif
	LOGF_TRACE("crc=0x%08.8x", crc);
	return crc;
}

int inAEScbc256(int opt,char pchKey[],char pchsrc[],char pchdest[], int Nblocks)		//Daee 19/05/2014
{
	int  in_Ret = VS_ERROR_AES;
	char Block[AES_SZB + 1]={0};
	int  idx;
	LOGF_TRACE("inAEScbc256");
	LOGF_TRACE("[opt->%d (%s)]",opt,(opt==0?"ENCRIPT":"DECRYPT"));
	//LOGAPI_HEXDUMP_TRACE("KEY", pchKey,32);
	//LOGAPI_HEXDUMP_TRACE("Block Enc/Dec", pchsrc,Nblocks*AES_SZB);

	if(opt == AES_ENC)
	{

		in_Xor(Block,pchsrc,InitVector,AES_SZB);
	
		for(idx=0;(idx/AES_SZB)<Nblocks;idx+=AES_SZB)
		{
			//LOGAPI_HEXDUMP_TRACE("Block2Enc",Block,AES_SZB);
			in_Ret = inAESecb256(opt,pchKey,Block,pchdest + idx,1);
			LOGF_TRACE("retAes[%d]",in_Ret);
			////LOGAPI_HEXDUMP_TRACE("Encrypted Block",pchdest + idx,AES_SZB);
			memset(Block,0,sizeof(Block));
			in_Xor(Block,pchsrc + (idx + AES_SZB),pchdest + idx,AES_SZB);
		}
	}
	else if(opt == AES_DEC)
	{
		for(idx=0;(idx/AES_SZB)<Nblocks;idx+=AES_SZB)
		{
			//LOGAPI_HEXDUMP_TRACE("Block2Dec", pchsrc + idx,AES_SZB);
			memset(Block,0,sizeof(Block));
			in_Ret = inAESecb256(opt,pchKey,pchsrc + idx ,Block,1);
			LOGF_TRACE("retAes[%d]",in_Ret);
			if(idx == 0)
			{
				in_Xor(pchdest + idx,Block,InitVector,AES_SZB);
			}
			else
			{
				in_Xor(pchdest + idx,Block,pchsrc + (idx - AES_SZB),AES_SZB);
			}
			//LOGAPI_HEXDUMP_TRACE("Decrypted Block", pchdest + idx,AES_SZB);
		}
		
	}

	//LOGAPI_HEXDUMP_TRACE("Final Ecrypt/Decrypt", pchdest,Nblocks*AES_SZB);
	LOGF_TRACE("inAEScbc256 ret --> %d",in_Ret);
	return in_Ret;
}


int inAESecb256(int opt,char pchKey[],char pchsrc[],char pchdest[], int Nblocks)		//Daee 19/05/2014
{
	int in_Ret = VS_ERROR_AES;
	int inOpt=0;
	int idx;
	LOGF_TRACE("inAESecb256");
	LOGF_TRACE("[opt->%d (%s)]",opt,(opt==0?"ENCRIPT":"DECRYPT"));
	//LOGAPI_HEXDUMP_TRACE("KEY",pchKey,32);
	//LOGAPI_HEXDUMP_TRACE("Block Enc/Dec", pchsrc,Nblocks*AES_SZB);
	
	switch(opt)
	{
		case AES_ENC: inOpt = AES256E; break;
		case AES_DEC: inOpt = AES256D; break;
	}

	for(idx=0;(idx/AES_SZB)<Nblocks;idx+=AES_SZB)
	{
		//LOGAPI_HEXDUMP_TRACE("Decrypt/Ecrypt Block",pchsrc + idx,AES_SZB);
		in_Ret = AES(inOpt,(unsigned char *)pchKey,(unsigned char *)pchsrc + idx,(unsigned char *)pchdest + idx);
		LOGF_TRACE("retAes[%d]",in_Ret);
		//LOGAPI_HEXDUMP_TRACE("Encrypt/Decrypt Block",pchdest + idx,AES_SZB);
	}

	//LOGAPI_HEXDUMP_TRACE("Final Ecrypt",pchdest,Nblocks*AES_SZB);
	LOGF_TRACE("inAESecb256 ret --> %d",in_Ret);
	return in_Ret;
}

int inPadAes(char pchData[] ,int inData,char PchDOut[], int *inDOut)
{
	int  inDtaBlocks  =0;
	int  modAesBlock  =0;
	int  add = 0;

	LOGF_TRACE("inPadAes");
	//LOGAPI_HEXDUMP_TRACE("Data2Pad",pchData,inData);
	
	modAesBlock = inData%AES_SZB;
	LOGF_TRACE("modAesBlock [%d]",modAesBlock);
	
	if(modAesBlock == 0)
	{
		inDtaBlocks = inData/AES_SZB;
		memcpy(PchDOut,pchData,inData);
		*inDOut = inData;
	}
	else
	{
		inDtaBlocks = ((inData - modAesBlock)/AES_SZB) + 1;
		memcpy(PchDOut,pchData,inData);
		add = (AES_SZB - modAesBlock);
		memset(PchDOut + inData,AES_PAD,add);
		//add = pad(PchDOut,pchData,' ',inData + (AES_SZB - modAesBlock),LEFT);
		if(add>0)
		{
			*inDOut = inData + add;
		}
		else
		{
			inDtaBlocks = 0;
		}
	}

	LOGF_TRACE("Blocks  AES [%d]",inDtaBlocks);
	//LOGAPI_HEXDUMP_TRACE("Data Pad Aes",PchDOut,*inDOut);
	LOGF_TRACE("inPadAes ret --> %d",inDtaBlocks);
	return inDtaBlocks;	
}

DllSPEC int inHex2Asc(char pchHex[], char pchAsc[], int inHex)
{
	int inAsc;

	LOGF_TRACE("inHexToAsc");

	//LOGAPI_HEXDUMP_TRACE("hex:",pchHex, inHex);

	inAsc = inHex << 1;

	memset(pchAsc, 0, inAsc);

	_SVC_HEX_2_DSP_(pchHex, pchAsc, inHex);

	//LOGAPI_HEXDUMP_TRACE("asc:", pchAsc, inAsc);

	return inAsc;
}

int inAsc2Hex(char pchAsc[], char pchHex[], int inAsc)
{
	int inHex;

	LOGF_TRACE("inHexToAsc");

	//LOGAPI_HEXDUMP_TRACE("asc:", pchAsc, inAsc);

	inHex = inAsc >> 1;

	memset(pchHex, 0, inHex);

	_SVC_DSP_2_HEX_(pchAsc, pchHex, inHex);

	//LOGAPI_HEXDUMP_TRACE("hex:", pchHex, inHex);

	return inHex;
}

int in_Xor(char pchEnd[], char pchOp1[], char pchOp2[], int inChr)
{
	int inIdx;

	LOGF_TRACE("inXor");

	//LOGAPI_HEXDUMP_TRACE("pchOp1:",pchOp1, inChr);

	LOGF_TRACE("pchOp2:", pchOp2, inChr);

	for (inIdx = 0; inIdx < inChr; inIdx++)
	{
		pchEnd[inIdx] = ((unsigned char) pchOp1[inIdx]) ^ ((unsigned char) pchOp2[inIdx]);
	}

	//LOGAPI_HEXDUMP_TRACE("pchEnd:", pchEnd, inChr);

	return VS_SUCCESS_AES;
}

int inGetEnvStr(char pch_key[],char pch_Out[],int inSizeMax)
{
	int in_Ret = 0;
	LOGF_TRACE("inGetEnvStr [key->%s][max:%d]",pch_key,inSizeMax);
	//  hacer la prueba con standar string
	in_Ret = _getEnvFile_((char *)"perm",pch_key, pch_Out, inSizeMax);
	//in_Ret = get_env(pch_key,pch_Out,inSizeMax);
	////LOGAPI_HEXDUMP_TRACE("[Val->%s]",pch_Out,inSizeMax);
	LOGF_TRACE("[in_Ret = :%i]",in_Ret);
	return in_Ret;
}

int inSetEnvStr(char pch_key[],char pch_Val[],int inSize)
{
	int in_Ret = VS_ERROR_AES;
	char temp[64] = {0};
	LOGF_TRACE("inSetEnvStr [key->%s][val->%s][tam:%d]",pch_key,pch_Val,inSize);
	in_Ret = _putEnvFile_((char *)"perm",pch_key, pch_Val);	//ACM 06/11/2015
	LOGF_TRACE("[in_Ret: despues del put %i]",in_Ret);
	in_Ret = _getEnvFile_((char *)"perm",pch_key, temp, sizeof(temp));
	LOGF_TRACE("[in_Ret: despues del get %i]",in_Ret);
	//LOGAPI_HEXDUMP_TRACE("temp -> ",temp, sizeof(temp));
	LOGF_TRACE("inSetEnvStr [tam set:%d]",in_Ret);

	//if(in_Ret == inSize)
	//{
	//	in_Ret = VS_SUCCESS_AES;
	//}

	in_Ret = VS_SUCCESS_AES;
	return in_Ret;
}


DllSPEC int in_exc_dsm (char pchRsp[])
{
	CrdExc_AES srCrdExc;
	int inRetVal;
	char *pchCod;

	LOGF_TRACE("in_exc_dsm");
	pchCod = (char *)"53";
	inRetVal = VS_ERROR_AES;

	if (in_Crp_Lod((char *)FILE_BINES, &srCrdExc, sizeof(srCrdExc)) == VS_SUCCESS_AES)
	{
		int in_use = 0;

		pchCod = (char *)"00";
		inRetVal = VS_SUCCESS_AES;

		if (inRetVal == VS_SUCCESS_AES)
		{
			char ch_use [10] = {0};
			int  in_crd = 0;
			int  in_val;

			memcpy (ch_use, srCrdExc.chUse, CRD_EXC_LEN_AES);
			in_use = atoi (ch_use);
			in_val = in_Cx_DDta (srCrdExc.chCrp, in_use, srCrdExc.chCrd, &in_crd);
			LOGF_TRACE("in_val=%d", in_val);
			if (in_val < VS_SUCCESS_AES)
			{
				pchCod = (char *)"53";
				inRetVal = RET_ECL_E_CIF_CX;
				LOGF_TRACE("************   ******");
			}
		}

		if (inRetVal == VS_SUCCESS_AES)
		{
			char ch_crc [ASC_KEY_SIZ] = {0};
			unsigned long ul_crc = 0;

			ul_crc = ulcrc32(srCrdExc.chCrd, in_use, 0xFFFFFFFF);
			
			memcpy (ch_crc, &ul_crc, sizeof (ul_crc));
			
			
			LOGF_TRACE("***CRC32 para VOS  o EVO **");
			//LOGAPI_HEXDUMP_TRACE( "cr1", ch_crc, sizeof (ul_crc));

			//LOGAPI_HEXDUMP_TRACE( "cr0",srCrdExc.chCrc, sizeof (ul_crc));

			if ( memcmp (ch_crc, srCrdExc.chCrc, ASC_CRC_SIZ) )
			{
				pchCod = (char *)"50";
				inRetVal = VS_ERROR_AES;
				LOGF_TRACE("************   ******");
			}
			
			
			
		}


	//	LOGF_TRACE("***HARDCODE***");
	//	inRetVal = VS_SUCCESS_AES;
			

		if (inRetVal == VS_SUCCESS_AES)
		{
			char ch_asc [515] = {0};
			char ch_src [ 10] = {0};
			int  in_asc = 0;
			int  in_src = 0;
			in_src = inTrmSpcEnd (srCrdExc.chCrd, in_use);
			in_asc = inHex2Asc(srCrdExc.chCrd, ch_asc, in_src);
			//in_asc = inHexToAsc  (srCrdExc.chCrd, ch_asc, in_src);
			if (in_asc > 0)
				if (ch_asc [in_asc - 1] == 'F')
					{
					in_asc -= 1;
					LOGF_TRACE("************   ******");
					}
			memcpy  (srCrdExc.chCrd, ch_asc, in_asc);
			//LOGAPI_HEXDUMP_TRACE( "crd",srCrdExc.chCrd, in_asc);
			sprintf (srCrdExc.chSrc,  "%d" , in_asc);
			//LOGAPI_HEXDUMP_TRACE("src", srCrdExc.chSrc, sizeof (srCrdExc.chSrc));

			memcpy (ch_src, srCrdExc.chSrc, CRD_EXC_LEN_AES);
			in_src = atoi (ch_src);
			if (in_src < 1)
			in_src = 1;
			LOGF_TRACE("in_src=%d", in_src);

			//#if defined (HRD_COD_RSP)
			//	if(0)

			//#else
				if (srCrdExc.chCrd [in_src - 1] != 'C')

			//#endif
			{
				pchCod = (char *)"55";
				inRetVal = VS_ERROR_AES;
				LOGF_TRACE("****ERROR DE FORMATO******");
			}
		}

	//	LOGF_TRACE("***HARDCODE***");
	//	inRetVal = VS_SUCCESS_AES;


		if (inRetVal == VS_SUCCESS_AES)
		{
			inRetVal = in_Crp_Sav((char *)FILE_BINES, &srCrdExc, sizeof(srCrdExc));

			if (inRetVal < VS_SUCCESS_AES)
			{
				pchCod = (char  *)"53";
				inRetVal = VS_ERROR_AES;
				LOGF_TRACE("***VS_ERROR_AES***");
			}
		}
	}

	memset(&srCrdExc, 0, sizeof(srCrdExc));

	memcpy (pchRsp, pchCod, 2);
	//LOGAPI_HEXDUMP_TRACE("rsp",pchRsp, 2);

	LOGF_TRACE("inRetVal=%d", inRetVal);

//	LOGF_TRACE("***HARDCODE***");
//	inRetVal = VS_SUCCESS_AES;


	return inRetVal;
}


int inTrmSpcEnd (char pch_src [], int in_max)
{
	char ch_end [2048] = {0};
	int  in_rmv = 0;

	LOGF_TRACE("inTrmSpcEnd");
	//LOGAPI_HEXDUMP_TRACE("src",pch_src, in_max);

	inInvStr (ch_end, sizeof (ch_end), pch_src, in_max);
	in_rmv = strspn (ch_end, " ");
	in_max -= in_rmv;
	pch_src [in_max] = 0;

	//LOGAPI_HEXDUMP_TRACE("end", pch_src, in_max);

	return in_max;
}


int inInvStr (char pch_end [], int in_max, char pch_src [], int in_src)
{
	int in_idx;
	int in_end;

	LOGF_TRACE("inInvStr");
	//LOGAPI_HEXDUMP_TRACE("src", pch_src, in_src);

	for (in_idx = in_src - 1, in_end = 0; in_idx > -1; in_idx--, in_end++)
		pch_end [in_end] = pch_src [in_idx];
	pch_end [in_end] = 0;

	//LOGAPI_HEXDUMP_TRACE( "end", pch_end, in_end);

	return in_end;
}


DllSPEC int in_ipk_wrn (void)		// AJM 26/08/2014 1
{
	int in_msg = -1;

	LOGF_TRACE("in_ipk_wrn");
	if (in_Cx_fkld())
	{
		if (!bo_exc_avl())
		{
			//in_msg = MSG_CARGAR_BINES;
		}
	}
	else
	{
		//in_msg = MSG_CARGAR_LLAVE;
	}

	if (in_msg > -1)
	{
		//if (boGphAvl())
		//{
		//	inShwScr (in_msg, 1, 5, FALSE, 0, TRUE);
		//}
	//	else
	//	{
			char ch_msg [30] = {0};

	//		in_dsp_get_msg (ch_msg, in_msg);
	//		vdACPL_SetDefaultIdlePrompt (ch_msg);
	//		inSaveConfRec (gszConfigFileName, SIZE_APP_CONF_REC, 0, (char *) &gstAppConfig);
	//		iGCL_DisplayIdlePrompt ();
	//	}
	}

	LOGF_TRACE("in_val=%d", VS_SUCCESS_AES);

	return VS_SUCCESS_AES;
}


DllSPEC int bo_exc_avl (void)
{
	int bo_val = FALSE_AES;
	int in_hnd = -1;
	int in_val;

	LOGF_TRACE("bo_exc_avl");
	in_val = VS_SUCCESS_AES;

	if (in_val > VS_FAILURE_AES)
	{
		int in_ret;

		in_ret = open(FILE_AES, O_RDONLY);

		if (in_ret < 0)
		{
			in_val = VS_FAILURE_AES;
		}
		else
		{
			in_hnd = in_ret;
		}
	}

	if (in_val > VS_FAILURE_AES)
	{
		ExcCrd_AES sr_exc_crd = {0};
		int in_ret;

		#ifdef _VRXEVO
			in_ret = crypto_read(in_hnd, (char *) &sr_exc_crd, sizeof (sr_exc_crd));
		#else
			in_ret = cryptoRead(in_hnd, (char *) &sr_exc_crd, sizeof (sr_exc_crd));
		#endif

		LOGF_TRACE("in_ret=%d", in_ret);

		if (in_ret < sizeof (sr_exc_crd))
		{
			in_val = VS_FAILURE_AES;
		}
		else
		{
			bo_val = TRUE_AES;
		}
	}

	if (in_hnd > -1)
	{
		int in_ret;

		in_ret = close(in_hnd);

		if (in_ret < 0)
		{
//			in_val = FAILURE;
		}
	}

	bo_val = TRUE_AES;	// AJM 07/10/2014 1

	LOGF_TRACE("bo_val=%d", bo_val);

	return bo_val;
	
}


DllSPEC int in_Crp_Exc_Sav(char pchRsp[])		// AJM 01/08/2010 1
{
	CrdExc_AES srCrdExc; 
	int inRetVal;
	char *pchCod;

	LOGF_TRACE("inCrpExcSav");

	pchCod = (char *)"53";
	inRetVal = VS_ERROR_AES;

	if (in_Crp_Lod((char *)FILE_BINES, &srCrdExc, sizeof(srCrdExc)) == VS_SUCCESS_AES)
	{
		char szTmp[20];
		int inTmp;

		pchCod = (char*)"00";
		inRetVal = VS_SUCCESS_AES;

		if (inRetVal == VS_SUCCESS_AES)
		{
			inSetEnvStr((char *)"BINPNP" ,(char *)"00000000",8);
			//inSetEnvVarpch("#BINPNP", "00000000", NULL);
			inSetEnvStr((char *)"BINVER" ,(char *)"00",2);
			//inSetEnvVarpch("#BINVER", "00", NULL);

			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chSrc, CRD_EXC_LEN_AES);
			inTmp = atoi(szTmp);
			inRetVal = in_Crp_Sav_Exc(srCrdExc.chCrd, inTmp);
			LOGF_TRACE("inRetVal=%d", inRetVal);
			if (inRetVal < VS_SUCCESS_AES)
			{
				pchCod = (char *)"53";
				inRetVal = VS_ERROR_AES;
			}
		}

		if (inRetVal == VS_SUCCESS_AES)
		{
			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chNam, CRD_EXC_NAM_AES);
			inSetEnvStr((char *)"BINPNP" ,szTmp,strlen(szTmp));
			//inSetEnvVarpch("#BINPNP", szTmp, NULL);			
			memset(szTmp, 0, sizeof(szTmp));
			memcpy(szTmp, srCrdExc.chVer, CRD_EXC_VER_AES);
			inSetEnvStr((char *)"BINVER" ,szTmp,strlen(szTmp));
			//inSetEnvVarpch("#BINVER", szTmp, NULL);
		}
	}

	memset(&srCrdExc, 0, sizeof(srCrdExc));

	memcpy(pchRsp, pchCod, 2);
	//LOGAPI_HEXDUMP_TRACE("pchRsp:", pchRsp, 2);

	LOGF_TRACE("inRetVal=%d", inRetVal);

	return inRetVal;
}


int in_Crp_Sav_Exc(char pchCrd[], int inCrd)
{
	int inVal;
	int inHdl;
	int inIdx;
	ExcCrd_AES srExcCrd ;
	char *pchEnd;
	int  *pinEnd;
	int in_num = 0;		// AJM 26/08/2014 1
	char ch_exc_crd [1024] = {0};		// AJM 07/10/2014 1

	LOGF_TRACE("inCrpSavExc");

	//LOGAPI_HEXDUMP_TRACE("pchCrd:",pchCrd, inCrd);

	inHdl = open(FILE_EXC, O_CREAT | O_TRUNC | O_RDWR , 0666); //dgms agrego o_trunc
	LOGF_TRACE("inHdl=%d", inHdl);
	if (inHdl < 0)
	{
		return VS_FAILURE_AES;
	}

	memset(&srExcCrd, 0, sizeof(srExcCrd));
	pchEnd =  srExcCrd.chMin;
	pinEnd = &srExcCrd.inMin;

	inIdx = 0;
	while (inIdx < inCrd)
	{
		char chChr;

		chChr = pchCrd[inIdx];
		LOGF_TRACE("chChr=%c", chChr);
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
			//LOGAPI_HEXDUMP_TRACE("min:", srExcCrd.chMin, srExcCrd.inMin);
			//LOGAPI_HEXDUMP_TRACE("max:", srExcCrd.chMax, srExcCrd.inMax);
			if (srExcCrd.inMax < 1)
			{
				memcpy(srExcCrd.chMax, srExcCrd.chMin, srExcCrd.inMin);
				srExcCrd.inMax = srExcCrd.inMin;
				//LOGAPI_HEXDUMP_TRACE("max:", srExcCrd.chMax, srExcCrd.inMax);
			}
			if (srExcCrd.inMin > 0)		// AJM 08/10/2014 1
			{
				#ifdef _VRXEVO
					inVal = crypto_write(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
				#else
					inVal = cryptoWrite(inHdl, (char *) &srExcCrd, sizeof(srExcCrd));
				#endif
				
				LOGF_TRACE("inVal=%d", inVal);
	//			in_exc_sav (&srExcCrd, in_num++);		// AJM 26/08/2014 1
				sprintf (ch_exc_crd + strlen (ch_exc_crd), (in_num++ > 0) ? ",%s" : "%s", srExcCrd.chMin);
			}
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
	
	inSetEnvStr((char *)"BINEXC" ,ch_exc_crd,strlen(ch_exc_crd));
	//inSetEnvVarpch ("#BINEXC", ch_exc_crd, NULL);

	inVal = close(inHdl);
	LOGF_TRACE("inVal=%d", inVal);

	return inVal;
}


extern void  _SVC_DSP_2_HEX_ (const char *dsp, char *hex, int pairs)
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
	return;
}


extern void  _SVC_HEX_2_DSP_(const char *hex, char *dsp, int count)
{
     int i;
     char ch;

     for(i = 0; i < count; i++)
     {
         ch = (hex[i] & 0xf0) >> 4;
         dsp[i * 2] = (ch > 9) ? ch + 0x41 - 10 : ch + 0x30;
         ch = hex[i] & 0xf;
         dsp[i * 2 + 1] = (ch > 9) ? ch + 0x41 - 10 : ch + 0x30;
     }

	 return;
	 
}


int inpurge_char(char * buffer,char rem_char)
{

	char *ptr;
	char *buffer1;
	int rval = 0;
	ptr = buffer;
	buffer1 = buffer;

	/* loop until the end of the string */
	while (*buffer != 0x00)
	{
				/**
				*	if the current character is the purge character
				*	eliminate it
				**/
		if (*ptr != rem_char)
			*buffer++ = *ptr++;

		else
		{			/* otherwise leave it in */
			ptr++;
			rval++;
		}
	}
			/* return the number of characters now in the buffer */
	return (rval);

}


int aes_pad(char *pdest_buf, char *psrc_buf, char pad_char, int pad_size, int align)
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

	      case  RIGHT_AES:
	      {
	         ch_left = num_pad;
	         ch_right = 0;
	         break;
	      }

	      case CENTER_AES:
	      {
	         ch_left = num_pad / 2;
	         ch_right = num_pad - ch_left;
	         break;
	      }

	      case LEFT_AES:
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
	      aes_append_char (pdest_buf, pad_char);
	   }

	   return (num_pad);
}


int aes_append_char (char *string, char c)
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


