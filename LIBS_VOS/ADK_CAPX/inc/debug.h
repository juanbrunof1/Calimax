/*
 * debug.h
 *
 *  Created on: 07/04/2015
 *      Author: Ivan Cruz
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <svc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
//extern "C"{
//#include "Asynprotocol.h"
//}

int fdebug(char * msg, char * __file, int __line, ...);
//int SendSerialPort(void* frame, int szframe);   //ACM 07/09/2105
//int ReceiveSerialPort(ParsingReceivedCommands* RcvCmd,int* countNACK,unsigned char* inputbuff); //ACM 07/09/2015

//#define __VOS_DEBUG

#ifdef __VOS_DEBUG
#define debug(M, ...) fdebug(M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

void dump(void *pvdMem, int num_bytes, char *title);
void pdump(char *in_buf, int offset, int num_bytes, int ascii_rep);

#endif /* DEBUG_H_ */
