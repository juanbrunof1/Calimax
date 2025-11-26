#ifndef _PP_CMD_H_
#define _PP_CMD_H_

#if (defined _VRXEVO)
  #if (defined CMDPP_IMPORT)
  #define DllSPEC __declspec(dllimport)
  #else
  #define DllSPEC   __declspec(dllexport)
  #endif
#else
  #define DllSPEC
#endif

#include <stdio.h>
#include <html/gui.h> 

#include "ipc/jsobject.h"


using namespace std;
using namespace vfigui;

/*Returns Values*/
#define XML_CMD_OK	  0
#define XML_CMD_FAIL -1
#define NO_CMD_DATA  -2
#define LRC_ERROR    -3


//SET VALUES
DllSPEC void vdSetByteValuePP(char *chField,char *chData ,int inLenField);
//GET VALUES
DllSPEC int inGetByteValuePP(char *chField,char *chDataOut);

DllSPEC int inPARSE_XMLCMDInit ( void );
DllSPEC int PP_CMD_unpack(char* command, char* in_buff,int szbuff,char* Application=NULL);
DllSPEC int PP_CMD_Pack(char* command,char* outbuff,int* szoutput,char* Application=NULL);
DllSPEC void clear_parse();
DllSPEC unsigned int CalcLRC(unsigned char* frame,int sizeframe);


// jbf 20171103
DllSPEC int TimeOutResp(char * command, char * outbuff, int * szoutput);
DllSPEC int ErrEventResp(char * command, char * szErrCode, int inLenErrCode, char * outbuff, int * szoutput);









#endif /* _VOS_PP_CMD_H_ */

