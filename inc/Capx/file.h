/*
 * file.h
 *
 *  Created on: 07/04/2015
 *      Author: Administrator
 */

#ifndef FILE_H_
#define FILE_H_

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "errcode.h"
#include "debug.h"

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif


DllSPEC int inOpenFile (const char pchOpn[], int inMod);
DllSPEC int inReadFile (FILE * inHdl, char pchChr[], int inChr);
DllSPEC int inWriteFile (FILE * inHdl, char pchChr[], int inChr);
DllSPEC int inCloseFile (FILE * inHdl);

#endif /* FILE_H_ */
