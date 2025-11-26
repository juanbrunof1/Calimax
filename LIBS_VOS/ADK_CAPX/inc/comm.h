/*
 * comm.h
 *
 *  Created on: 07/04/2015
 *      Author: Ivan Cruz
 */

#ifndef COMM_H_
#define COMM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _VRXEVO
#include <termios.h>
#include <vficom.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>



int inOpenPort(char * port, int mode);
int inClosePort(int inHdl);
int inReadPort(int inHdl, char * buff, int inSiz);
int inWritePort(int inHdl, void * buff, int inSiz);

int inOpenTCP(char * ip, int port);
int inCloseTCP(int hdl);
int inSendTCP(int hdl,char * szMsg, int inLen);

#endif /* COMM_H_ */
