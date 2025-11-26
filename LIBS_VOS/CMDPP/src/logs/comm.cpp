/*
 * comm.c
 *
 *  Created on: 07/04/2015
 *      Author: Ivan Cruz
 */

#include "errcode.h"
#include "comm.h"
#include "debug.h"   //ACM 07/09/2015

//TCPIP
int inOpenTCP(char * ip, int port)
{
	int len;
	struct sockaddr_in address;
	int result;
	int sockfd = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(!sockfd)
	{
		errno = sockfd;
		return ERR_OPEN_SOCKET;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(port);
	len = sizeof(address);
	result = connect(sockfd, (struct sockaddr*) &address, len);

	if (result == -1)
	{
		errno = result;
		return ERR_OPEN_CONNECT;
	}
	return sockfd;
}

int inCloseTCP(int hdl)
{
	if (hdl <= 0)
		return ERR_SOCK_HNDL;

	return close(hdl);
}

int inSendTCP(int hdl,char * szMsg, int inLen)
{
	if (hdl <= 0)
		return ERR_SOCK_HNDL;

	return write(hdl, szMsg, inLen);
}


