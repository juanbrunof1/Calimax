#include <stdio.h>
#include <string.h>
#include "Convertion/Convertion.h"

void  vdHex2Asc(const char *hex, char *dsp, int count)
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
	

}

void  vdAsc2Hex (const char *dsp, char *hex, int pairs)
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
	//return;
}

void HextoBCD(char* HexaValue,int lenghtHex, char* BCDvalue)
{			
	int length = 0;
	int aux = 0;
	int i = 0;
	int BCDResult;
	
	memset(BCDvalue,0,sizeof(BCDvalue));
	
	for(i=0,length = 0; length < lenghtHex*2;length+=2,i++)
	{
		aux = HexaValue[i];		
		aux >>= 4;
		aux &= 0x0F;
		if(aux >= 0 && aux <= 9)
			aux |= 0x30;
		else
		{ 
			aux &= 0x00;
			aux |= 0x30;
		}
		BCDvalue[length] = aux;
		aux &= 0x00;
		aux = HexaValue[i];	
		aux &= 0x0F;
		if(aux >= 0 && aux <= 9)
			aux |= 0x30;
		else
		{
			aux &= 0x00;
			aux |= 0x30; 
		}
		BCDvalue[length+1] = aux;			
	}
}

/**************************************************************************************************
 * Elimina los centinelas del track 2
 *
 * @param pTrack2Buffer	Buffer del track2 con centinelas
 * @param pTrack2Length	Longitud del buffer track2
 * @return		Nueva longitud del track 2 (limpio)
 *************************************************************************************************/
int CleanTrack2(char* pTrack2Buffer, int pTrack2Length)
{
	char* lCharPosition = 0x00;

	lCharPosition = (char*)memchr(pTrack2Buffer, ';', pTrack2Length);
	if(lCharPosition)
	{
		pTrack2Length -= (lCharPosition - pTrack2Buffer);
		for(int lIndex = 0; lIndex < pTrack2Length; lIndex++)
		{
			*(pTrack2Buffer + lIndex) = *((lCharPosition + 1) + lIndex);
		}
	}

	lCharPosition = (char*)memchr(pTrack2Buffer, '?', pTrack2Length);
	if (lCharPosition)
	{
		*lCharPosition = 0;
		pTrack2Length = lCharPosition - pTrack2Buffer;
	}

	return pTrack2Length;
}

/**************************************************************************************************
 * Elimina los centinelas del track 2
 *
 * @param pTrack2Buffer	Buffer del track2 con centinelas
 * @param pTrack2Length	Longitud del buffer track2
 * @return		Nueva longitud del track 2 (limpio)
 *************************************************************************************************/
int CleanTrack1(char* pTrack2Buffer, int pTrack2Length)
{
	char* lCharPosition = 0x00;

	lCharPosition = (char*)memchr(pTrack2Buffer, '%', pTrack2Length);
	if(lCharPosition)
	{
		pTrack2Length -= (lCharPosition - pTrack2Buffer);
		for(int lIndex = 0; lIndex < pTrack2Length; lIndex++)
		{
			*(pTrack2Buffer + lIndex) = *((lCharPosition + 1) + lIndex);
		}
	}

	lCharPosition = (char*)memchr(pTrack2Buffer, '?', pTrack2Length);
	if (lCharPosition)
	{
		*lCharPosition = 0;
		pTrack2Length = lCharPosition - pTrack2Buffer;
	}

	return pTrack2Length;
}

/**************************************************************************************************
 * Realiza el ajuste del Track2, reemplazando el caracter 'D' por '=' y eliminar el padding de F's
 * al final del buffer (si existen).
 *
 * @param pTrack2			Apuntador al buffer del Track2
 * @param pTrack2Length		Longitud inicial del buffer del Track2
 * @return					Longitud final del buffer de Track2
 *************************************************************************************************/
int AdjustTrack2(char* pTrack2, int pTrack2Length)
{
	char* lCharPosition = 0x00;

	lCharPosition = (char*)memchr(pTrack2, 'D', pTrack2Length);
	if(lCharPosition)
		*lCharPosition = '=';

	lCharPosition = (char*)memchr(pTrack2, 'F', pTrack2Length);
	if (lCharPosition)
	{
		*lCharPosition = 0;
		pTrack2Length = lCharPosition - pTrack2;
	}

	return pTrack2Length;
}

/************************************************************************************************************
 * Realiza el formateo de un script
 *
 * @param pScript			Apuntador al buffer del script
 * @param pScriptLength		Longitud del buffer de entrada
 * @param pTag				Nombre del TAG que será usado en el formateo (0x71 - 0x72 -etc)
 * @param pBufferOut		Buffer de salida del script formateado
 * @param pMaxLength		Longitud máxima del buffer de salida
 * @return			Longitud resultande del script formateado
 ***********************************************************************************************************/
int Script_Format(const char* pScript, int pScriptLength, unsigned char pTag, char* pBufferOut, int pMaxLength)
{
	int lBlockLengthPosition = 1;
	char lTagsBuffer[512] = {0};
	int lTagNameLength = 0;
	int lTagDataLength = 0;
	int lOutputLength = 2;
	int lBlockLength = 0;

	if(pBufferOut == 0x00 || pMaxLength < pScriptLength + 2 || pScriptLength > 500)
		return 0;

	lTagsBuffer[0] = pTag;
	for(int lScriptIndex = 0, lIndex = 2; lScriptIndex < pScriptLength; lOutputLength = lIndex)
	{
		lTagNameLength = (pScript[lScriptIndex] & 0x1F) == 0x1F ? 2 : 1;
		lTagDataLength = pScript[lScriptIndex + lTagNameLength];

		if( lTagNameLength == 2 && memcmp(&pScript[lScriptIndex], "\x9F\x18", 2) == 0 )
		{
			if(lIndex > 2)
			{
				lTagsBuffer[lBlockLengthPosition] = lBlockLength;//Length

				lTagsBuffer[lIndex++] = pTag;
				lBlockLengthPosition = lIndex++;
				lBlockLength = 0;
			}
		}

		memcpy(&lTagsBuffer[lIndex], &pScript[lScriptIndex], lTagNameLength + 1 + lTagDataLength);
		lScriptIndex += lTagNameLength + 1 + lTagDataLength;
		lBlockLength += lTagNameLength + 1 + lTagDataLength;
		lIndex += lTagNameLength + 1 + lTagDataLength;
	}

	lTagsBuffer[lBlockLengthPosition] = lBlockLength;
	memcpy(pBufferOut, lTagsBuffer, lOutputLength);
	//LOGAPI_HEXDUMP_TRACE("Script_Format", pBufferOut, lOutputLength);
	return lOutputLength;
}


