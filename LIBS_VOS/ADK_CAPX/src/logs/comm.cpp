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


//Serial

/*int inOpenPort(char * port, int mode)
{
	int inRes = SUCCESS;
	int inHdl;

	struct Opn_Blk OpenBlock;
	memset(&OpenBlock,0x00,sizeof(struct Opn_Blk));
	debug("--inOpenPort--");	//ACM 07/09/2015
	inRes = open(port,mode);
	debug("inRes = %i",inRes);  //ACM  07/09/2015
	if (inRes<0){
		debug("Error to open Port: %s",COM1);		//ACM 07/09/2015
		return COMM_OPEN_ERROR;
		}
	else
		debug("%s was opened",COM1);		//ACM 07/09/2015
	inHdl = inRes;

	OpenBlock.rate=Rt_19200;
	OpenBlock.format=Fmt_A8N1;
    OpenBlock.protocol=P_char_mode;

   // inRes = svcSetOpenBlock(inHdl,&OpenBlock);
    if (inRes < SUCCESS)
    	return COMM_OPEN_BLOCK_ERROR;
	else
		debug("Configuration done");
	return inHdl;
}
*/
int inClosePort(int inHdl)
{
debug("--InClose--Port");  //AC 08/09/2015
	if(!close(inHdl))
		return COMM_CLOSE_ERROR;	
	return SUCCESS;
}

int inReadPort(int inHdl, char * buff, int inSiz)
{
	int inRet;	
	if (inHdl < 0)
		return COMM_READ_HANDLER_ERROR;

	inRet = read(inHdl,buff,inSiz);	
		
	if(inRet < 0){
		debug("An Error has happened in reading process");  //ACM 07/09/2015
		return COMM_READ_ERROR;
		}
	else{
		debug("Correct Reading");		//ACM 07/09/2015
		debug("Read Data from Serial Port:");  //ACM 07/09/2015
	    debug("%s",buff);  //ACM 07/09/2015		    
		}
	
	debug("Buffer size of read Data -> inRet = %i",inRet);	//ACM 07/09/2015
	return inRet;
}

int inWritePort(int inHdl, void* buff, int inSiz)
{
	int inRet;
	debug("The port is already to send by Serial Port");  //ACM 09/07/2015
	if (inHdl < 0){
		debug("It is not possible to send by Serial Port");  //ACM 07/09/2015
		return COMM_WRITE_HANDLER_ERROR;
		}
	//debug("%i, %s, %i",inHdl,buff,inSiz);  //ACM 07/09/2015
	inRet = write(inHdl,buff,inSiz);
	debug("Size of the sent data -> inRet = %i",inRet);  //ACM 07/09/2015
	if(inRet < 0){
		debug("There is an error in the datas sending");  //ACM 07/09/2015
		return COMM_WRITE_ERROR;		
		}	
	return inRet;
}
