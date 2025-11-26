/*
 * file.c
 *
 *  Created on: 07/04/2015
 *      Author: Ivan Cruz
 */

#include "file.h"

DllSPEC int inOpenFile (const char pchOpn[], int inMod)
{
	int inRetVal;
	debug("--inOpenFile--");

	debug("open file=%s mode=0x%04.4x", pchOpn, inMod);
	inRetVal = open (pchOpn, inMod);
	debug("open ret=%d errno=%d", inRetVal, errno);

	return inRetVal;
}

DllSPEC int inReadFile (int inHdl, char pchChr[], int inChr)
{
	int inRetVal;

	debug("--inReadFile--");

	debug ("handler=%d", inHdl);
	if(inHdl < 0)
		return ERR_FILE_BAD_HANDLER;

	memset(pchChr, 0, inChr);
	inRetVal = read (inHdl, pchChr, inChr);
	debug("read inRetVal=%d errno=%d", inRetVal, errno);
	dump (pchChr, inRetVal, "read from file:");

	return inRetVal;
}

DllSPEC int inWriteFile (int inHdl, char pchChr[], int inChr)
{
	int inRetVal;

	debug("--inWriteFile--");

	debug("handler=%d", inHdl);
	//dump(pchChr, inChr, "write to file:");
	inRetVal = write (inHdl, pchChr, inChr);
	debug("write inRetVal=%d errno=%d", inRetVal, errno);

	return inRetVal;
}

DllSPEC int inCloseFile (int inHdl)
{
	int inRetVal;

	debug("--inCloseFile--");

	debug ("handler=%d", inHdl);
	inRetVal = close (inHdl);
	debug("close inRetVal=%d errno=%d", inRetVal, errno);

	return inRetVal;
}
