/*
 * libtcpdbgr.c
 *
 *  Created on: 07/04/2015
 *      Author: Ivan Cruz
 */

#include "debug.h"
#include "comm.h"
#include "errcode.h"
#include "symbols.h"
#include "stdarg.h"

#include <string.h>		//ACM 07/09/2015

int fdebug(char * msg, char * __file, int __line, ...)
{
	/*int inRet = 0;
	char szIP[16] = {0};
	int inPort = 0;
	char szMsg [128] = {0};
	char szDebug [256] = {0};
	char szVal[512] = {0};

	int tcp_hndl = 0;

	va_list args;

	inRet = getEnvFile("debug", "DEBUG", szVal,sizeof(szVal));

	if (inRet == 0)
		return ERR_DEBUG_NOT_FOUND;
	if (strncmp(szVal,"1",1))
		return ERR_DEBUG_NOT_SET;

	inRet = getEnvFile("debug", "DEBUGIP", szVal,sizeof(szVal));
	if (inRet == 0)
		return ERR_DEBUG_IP_NOT_FOUND;

	strncpy(szIP, szVal,15);

	inRet = getEnvFile("debug", "DEBUGPORT", szVal,sizeof(szVal));
	if (inRet == 0)
		return ERR_DEBUG_PORT_NOT_FOUND;

	inPort = atoi(szVal);

	tcp_hndl = inOpenTCP(szIP, inPort);

	if (inRet < SUCCESS) return inRet;


	memset(szMsg,0,sizeof(szMsg));
	memset(szDebug,0,sizeof(szDebug));

    va_start(args, __line);
	vsprintf(szMsg,msg,args);
    va_end(args);

    sprintf(szDebug,"%s:%d: ", __file, __line);
    strcat(szDebug,szMsg);
    strcat(szDebug,"\n");

    inRet = inSendTCP(tcp_hndl, szDebug, strlen(szDebug));
    inRet = inCloseTCP(tcp_hndl);

    usleep(5000);*/

    return SUCCESS;
}

void pdump(char *in_buf, int offset, int num_bytes, int ascii_rep)
{
	/*int     index=0;
	int		inMaxChr;
	char    temp_buf[10];
	char    disp_buf[500];

    if ( ascii_rep == TRUE )
    {
        if ( num_bytes > 16 )
        {
            // disp_buf overflow!
            debug ("dump ERR - num_bytes=%d > 15", num_bytes);
            return;
        }
		inMaxChr = 16;
    }
    else
    {
        if ( num_bytes > 23 )
        {
            // disp_buf overflow!
            debug ("dump ERR - num_bytes=%d > 20", num_bytes);
            return;
        }
		inMaxChr = 23;
    }

    memset(temp_buf,0, sizeof (temp_buf));
    memset (disp_buf,0, sizeof (disp_buf));
    sprintf(temp_buf, "%04X | ", offset);
    strcat (disp_buf, temp_buf);
    for (index = 0; index < inMaxChr; ++index)
    {
        memset (temp_buf, 0,sizeof (temp_buf));
		if (index < num_bytes)
			sprintf(temp_buf, "%2.2X ", (int) (unsigned char) in_buf[offset + index]);
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
            memset (temp_buf, 0, sizeof (temp_buf));
			c = in_buf[offset + index];
			if (c < 0x20 || c > 0x7E)
				c = '.';
            sprintf (temp_buf, "%c", c);
            strcat (disp_buf, temp_buf);
        }
    }

    debug ("%s", disp_buf);*/
}

void dump(void *pvdMem, int num_bytes, char *title)
{
	/*int offset = 0;
	int r=0;
	char *in_buf;

	if (num_bytes < 0)
		num_bytes = 0;

	if ( strlen (title) > 0 )
		debug ("%s (%d):", title, num_bytes);

	in_buf = (char *) pvdMem;
	while (offset < num_bytes)
	{
		r = num_bytes - offset;
		if (r > 16)
			r = 16;
		pdump (in_buf, offset, r, TRUE);
		offset += r;
	}*/
}
