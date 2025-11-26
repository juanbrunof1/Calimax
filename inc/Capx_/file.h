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

int inOpenFile (const char pchOpn[], int inMod);
int inReadFile (FILE * inHdl, char pchChr[], int inChr);
int inWriteFile (FILE * inHdl, char pchChr[], int inChr);
int inCloseFile (FILE * inHdl);

#endif /* FILE_H_ */
