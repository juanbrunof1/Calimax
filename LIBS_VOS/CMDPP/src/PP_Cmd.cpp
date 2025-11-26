
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _VRXEVO
#include <stdlib.h> 
#else
#include <malloc.h> 
#endif

#include "ipc/jsobject.h"
#include "ipc/notify.h"

#include "expat/expat.h"
#include "log/liblog.h" // ADK Logging


#include "PP_Cmd.h"



//******************************************************************************************************************
//Global variables
JSObject CmdObj;

char   defHostName[30]={0};
char   defCMD[30] = {0};
char   defMSJ[30] = {0};
char   def_type_cmd[30] = {0};
char   defXMLElement[30] = {0};
int    PPdescElement  = 0;
char   Req_Cmd[6] = {0};

//Declaration for Commands for PIN PAN
#ifdef _VRXEVO
#define XML_CMD_PP  "I:CMD_PP.xml"
#else
#define XML_CMD_PP  "/home/usr1/flash/Comandos_PinPad/CMD_PP.xml"
#endif
#define CMD_DEF 	"CMD"
#define CMD_MSJ		"MSJ"
#define CMD_FIELD	"FIELD"
#define PP_DEF_VAR	"FIELDVAR"


/*cmd PP def attr*/
#define fld_AppName			"AppName"
#define fld_defCmd			"cmdName"
#define fld_lenCmd			"lenCmd"
#define fld_numfields		"numFields"
#define head_msg		"head_msg" //JRS 23-11-17

#define fld_flow			"Flow"
#define fld_Defined			"defined"
#define fld_length			"len"
#define fld_typecmd 		"type"
#define fld_aliascmd 		"alias"

#define PP_APPCMDS	         0
#define PP_COMMANDNAME	     1
#define PP_FIELD_CMD		 2
#define PP_NODEFINE			 99

#define STAND_ALONE			"STAND_ALONE"


//XML OPEN
int file_size = 0;
char Xmlbuff[10500] = {0};

map<string, string> stValue;
map<string, int> intValue;
map<string, char*> chValue;


//*******************************************************************************************************************
//Prototypes of functions
static int parser_xml_PP(const char* filename);
static void start_element_PP(void *userData, const char *element, const char **attribute);
static void end_element_PP(void *userData, const char *el);
static void element_value_PP(void *userData, const char *val, int len);
unsigned int CalcLRC(unsigned char* frame,int sizeframe); //Function to calculate the LRC
int ParsePP_CMD(char* command, char* in_buff,int szbuff);
int AsmPP_CMD(char * command,char* outbuff,int* szoutput);

void DUMP_XML_HEX(void* input,int size,char* name);

static void  vdAsc2Hex (const char *dsp, char *hex, int pairs);
static void  vdHex2Asc(const char *hex, char *dsp, int count);

//*******************************************************************************************************************
int PP_CMD_Pack(char* command,char* outbuff,int* szoutput,char* Application)
{
	int inResult = XML_CMD_FAIL;
	LOGF_TRACE("\n---PP_CMD_Pack---");

	if(strlen (command) > 0)
	{
		memset(Req_Cmd, 0x00, sizeof(Req_Cmd));
		memcpy(Req_Cmd,command,strlen(command));

		memset(defHostName, 0x00, sizeof(defHostName));
		if(Application==NULL)
		{
			strcpy(defHostName,(char*)STAND_ALONE);
			inResult = XML_CMD_OK;
		}
		else if(strlen (Application) > 0)
		{
			memcpy(defHostName,Application,strlen(Application));
			inResult = XML_CMD_OK;
		}
		
		//LOGF_TRACE("\nREQUESTED APPLICATION: %s",defHostName);
	//	LOGF_TRACE("\nREQUESTED COMMAND: %s",Req_Cmd);
		
		
	}	

	if(inResult == XML_CMD_OK)
	{
		inResult = AsmPP_CMD(command,outbuff,szoutput);
	}
	

return inResult;
}




// jbf 20171103
int TimeOutResp(char * command, char * outbuff, int * szoutput)
{
	int sizeNameCmd = 0;
	int len_cmd = 0;
	int Result = XML_CMD_OK;
	unsigned char LRC = 0;
	
	memcpy(outbuff,"\x02",1);	
	len_cmd += 1;

	sizeNameCmd = strlen(command);
	memcpy(outbuff + len_cmd,command,sizeNameCmd);
	len_cmd += sizeNameCmd;
	
	memcpy(outbuff + len_cmd, "06\x00\x00", 4);
	len_cmd += 4;

	memcpy(outbuff + len_cmd,"\x03",1);
	//LOGAPI_HEXDUMP_TRACE("STX",outbuff + len_cmd,sizeNameCmd);
	len_cmd += 1;
	
	LRC = CalcLRC((unsigned char*)outbuff,len_cmd);	

	outbuff[len_cmd] = LRC;
	len_cmd += 1;

	*szoutput = len_cmd;	

	return Result;
	
}

// jbf 20171103
int ErrEventResp(char * command, char * szErrCode, int inLenErrCode, char * outbuff, int * szoutput)
{
	int sizeNameCmd = 0;
	int len_cmd = 0;
	int Result = XML_CMD_OK;
	unsigned char LRC = 0;
	
	memcpy(outbuff,"\x02",1);	
	len_cmd += 1;

	sizeNameCmd = strlen(command);
	memcpy(outbuff + len_cmd,command,sizeNameCmd);
	len_cmd += sizeNameCmd;
	
	//memcpy(outbuff + len_cmd, "06\x00\x00", 4);
	//len_cmd += 4;
	memcpy(outbuff + len_cmd, szErrCode, inLenErrCode);
	len_cmd += 4;

	memcpy(outbuff + len_cmd,"\x03",1);
	//LOGAPI_HEXDUMP_TRACE("STX",outbuff + len_cmd,sizeNameCmd);
	len_cmd += 1;
	
	LRC = CalcLRC((unsigned char*)outbuff,len_cmd);	

	outbuff[len_cmd] = LRC;
	len_cmd += 1;

	*szoutput = len_cmd;	

	return Result;
	
}





int AsmPP_CMD(char * command,char* outbuff,int* szoutput){
	unsigned char LRC = 0;
	int Result = XML_CMD_OK;
	int len_cmd = 0;
	int num_flds = 0;
	int len_fld = 0;
	int i = 0;
	int sizeNameCmd = 0;
	char field[7] = {0};	
	char outfld[700] = {0};
	std::string FLOW = "";
	std::string VAL = "";	
	std::string FORMAT = "";
	std::string HEADMSG = "";

	LOGF_TRACE("\n--asmPP_CMD--");

	num_flds = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(fld_numfields).getInt();
	HEADMSG = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(head_msg).getString().c_str(); //IF EXIST USE THIS FIELD FOT HEADER MSg
	LOGF_TRACE("\nnum_flds = %d",num_flds);
	LOGF_TRACE("\nHEADMSG = %s [%d] ",HEADMSG.c_str(),HEADMSG.length());

	memcpy(outbuff,"\x02",1);	
	//DUMP_XML_HEX( outbuff,1,(char*)"STX");
	//LOGAPI_HEXDUMP_TRACE("STX",outbuff,1);
	len_cmd += 1;

	sizeNameCmd = strlen(command);
	if(HEADMSG == "null")
		memcpy(outbuff + len_cmd,command,sizeNameCmd);
	else
	{
		sizeNameCmd = HEADMSG.length();
		memcpy(outbuff + len_cmd,HEADMSG.c_str(),sizeNameCmd);
	}
		
	//DUMP_XML_HEX(outbuff + len_cmd,sizeNameCmd,(char*) "COMMAND");
	//LOGAPI_HEXDUMP_TRACE("COMMAND",outbuff + len_cmd,sizeNameCmd);
	len_cmd += sizeNameCmd;

	for(i = 0; i < num_flds ; i++)
	{
		sprintf(field,"%s%d",CMD_FIELD,i);		//We check every field to know if the field in the xml is valid in this function 
		FLOW = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_flow).getString().c_str();				

		if(!memcmp(FLOW.c_str(),"\x54",1) || !memcmp(FLOW.c_str(),"\x41",1))
		{

			VAL = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(PP_DEF_VAR).getString().c_str();	//We obtain the name of the field
			LOGF_TRACE("\n---  [%s] ---",VAL.c_str());					

			len_fld =  inGetByteValuePP((char* )VAL.c_str(),outfld);	//We obtain the value of field to build the message
			if(len_fld != -1)
			{			
				memcpy(outbuff + len_cmd,outfld,len_fld);					//We build the message
				//DUMP_XML_HEX( outbuff + len_cmd,len_fld,(char*)"Data");		
				//LOGAPI_HEXDUMP_TRACE("Data",outbuff + len_cmd,len_fld);
				len_cmd += len_fld;
				len_fld = 0;			
			}
			else if(VAL == "LENGH")
			{
				LOGF_TRACE("Longitud Campos: %d", len_fld);
				memcpy(outbuff + len_cmd,"\x00\x00",2);
				len_cmd += 2;
			}
		}
	}
	
	memcpy(outbuff + len_cmd,"\x03",1);
	//DUMP_XML_HEX(outbuff + len_cmd,sizeNameCmd,(char*) "STX");
	//LOGAPI_HEXDUMP_TRACE("STX",outbuff + len_cmd,sizeNameCmd);
	len_cmd += 1;

	LRC = CalcLRC((unsigned char*)outbuff,len_cmd);	

	outbuff[len_cmd] = LRC;
	len_cmd += 1;

	*szoutput = len_cmd;	

	return Result;
}
//*******************************************************************************************************************

DllSPEC int PP_CMD_unpack(char* command, char* in_buff,int szbuff,char* Application)
{
	int inResult = XML_CMD_FAIL;
	unsigned char LRC = 0;
	LOGF_TRACE("\n---PP_CMD_unpack---");
	
	LRC = CalcLRC((unsigned char*)in_buff,szbuff - 1);

	if(LRC != in_buff[szbuff-1])	//We check the LRC Value
	{
		LOGF_TRACE("\nFail in LRC");
		inResult = LRC_ERROR;
	}
	else if(strlen (command) > 0)
	{
		memset(Req_Cmd, 0x00, sizeof(Req_Cmd));
		memcpy(Req_Cmd,command,strlen(command));

		memset(defHostName, 0x00, sizeof(defHostName));
		if(Application==NULL)
		{
			strcpy(defHostName,(char*)STAND_ALONE);
			inResult = XML_CMD_OK;
		}
		else if(strlen (Application) > 0)
		{
			memcpy(defHostName,Application,strlen(Application));
			inResult = XML_CMD_OK;
		}
		
		LOGF_TRACE("\nREQUESTED APPLICATION: %s",defHostName);
		LOGF_TRACE("\nREQUESTED COMMAND: %s",Req_Cmd);
		
		
	}	
	
	if(inResult == XML_CMD_OK)
	{
		inResult = ParsePP_CMD(command,in_buff,szbuff);
	}

	return inResult;
}

int ParsePP_CMD(char* command, char* in_buff,int szbuff)
{
	int inResult = 0;
	char *buff_temp;
    int in_idx = 0;
	int sizeNameCmd = 0;
	int lengthCMD = 0;
	unsigned int sizetotal = 0;
	char outbuff[700] = {0};
	int soutbuff = 0;
	
	LOGF_TRACE("\n--ParsePP_CMD--");

	//DUMP_XML_HEX(in_buff, szbuff, (char*)"in_buff");
	//LOGAPI_HEXDUMP_TRACE("in_buff",in_buff, szbuff);

	buff_temp = (char*)malloc(szbuff + 1);
	sizeNameCmd = strlen(command);
	
	//WE cut the STX, ETX and LRC bytes (3), We also cut the length of the command (sizeNameCmd)
	szbuff = szbuff - (3 + sizeNameCmd); 

	//We check if the command has information
	if(szbuff == 0)
	{
		LOGF_TRACE("\nTHE COMMAND CANNOT BE PARSED DUE TO IT HAS NOT DATA TO PARSE");		
		inResult = NO_CMD_DATA;	
	}	

	if(inResult == XML_CMD_OK)
	{
		

		in_idx = sizeNameCmd +1;	
		memcpy(buff_temp,in_buff + in_idx,szbuff);
		
		//DUMP_XML_HEX(buff_temp,szbuff,(char*)"buff_temp");
		//LOGAPI_HEXDUMP_TRACE("buff_temp",buff_temp,szbuff);

		//WE begin to parse the command

		//We obtain the size of the command
		lengthCMD = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(fld_lenCmd).getInt();	
		LOGF_TRACE("\n--- LEN_cmd [%d] ---",lengthCMD);

		in_idx = 0;

		//We check if the command has a field with total size
		if(lengthCMD == 0)
		{
			LOGF_TRACE("\nThe command does not have field of total size");		
			inResult = XML_CMD_OK;
		}
		else
		{
			int byte = 0;
			//LOGF_TRACE("\nThe command has a field of total size");								
			sizetotal = 0;
			//We obtain the value of the length of the command
			for(byte = 0; byte < lengthCMD; byte++)
			{
				sizetotal |= buff_temp[byte];	
				if( byte < (lengthCMD - 1) )
					sizetotal = sizetotal << 8;
			}
			//The field of size of the information may be between 2 or 3 bytes, we check it to clean the MSB to avoid signal shift
			if(lengthCMD == 3)
			{
				sizetotal &= 0x00FFFFFF;		
			}
			
			if(lengthCMD == 2)
			{
				sizetotal &= 0x0000FFFF;					
			}		

			//We increase the index to parse the buff where we have the data to parse
			szbuff = szbuff - lengthCMD;
			in_idx += lengthCMD;
			
			LOGF_TRACE("\nSizetotal obtain from the incoming buffer = %x",sizetotal);

			//We check if ther is matching between the size that we have obtained from a reading of Serial port and the size into the buffer that we received
			if(szbuff == sizetotal)
			{			
				inResult = XML_CMD_OK;
				//LOGF_TRACE("\nCHECK TOTAL SIZE SUCCESSFUL");
			}
			else
			{
				inResult = XML_CMD_FAIL;
				LOGF_TRACE("\nERROR IN TOTAL SIZE");									
			}
		}
	}

	//We begin to parse the buffer to obtain each field
	if(inResult == XML_CMD_OK)
	{ 
		char field[7] = {0};
		char* data; 
		int i = 0;
		int numbfields = 0;
		int len_fld = 0;
		std::string FLOW = "";
		std::string VAL = "";
		std::string TYPE = "";
		std::string FORMAT = "";
		std::string ALIAS = "";
		int countbytes = 0;
		int lengTLV = 0;
		int byte = 0;

		//We obtain the number of fields that the command has.
		numbfields =  CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(fld_numfields).getInt();
		LOGF_TRACE("\n--- numbfields [%d] ---",numbfields);	
		
		for(i = 0; i < numbfields; i++ )
		{
			
			sprintf(field,"%s%d",CMD_FIELD,i);		//We check every field to know if the field in the xml is valid in this function 
			FLOW = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_flow).getString().c_str();	
			
			if(!memcmp(FLOW.c_str(),"\x41",1) || !memcmp(FLOW.c_str(),"\x48",1))   //A = 0x41, H = 0x48
			{
				//LOGF_TRACE("\n--- FLW [%s] ---",FLOW.c_str());		//We obtain the information about the field that we will parse				
							
				len_fld = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_length).getInt();	

				FORMAT  = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_Defined).getString().c_str();	
				//LOGF_TRACE("\n---  [%s] ---",FORMAT.c_str());
				
				ALIAS = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_aliascmd).getString().c_str();	//We obtain the alias of the field
				//LOGF_TRACE("\n---  ALIAS [%s : (%d)] ---",ALIAS.c_str(), ALIAS.length());

				//We obtain the length of the field		
				//LOGF_TRACE("len_fld = %d",len_fld);
				data = (char*)malloc(len_fld + 1);
				memset(data,0,len_fld + 1);
				
				VAL  = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(PP_DEF_VAR).getString().c_str();	//We obtain the name of the field
				//LOGF_TRACE("\n---  [%s] ---",VAL.c_str());
				
				TYPE = CmdObj (CMD_DEF)(defHostName)(CMD_MSJ)(Req_Cmd)(field)(fld_typecmd).getString().c_str();	//We obtain the type of command (NUM, HEX, ASC)		
				//LOGF_TRACE("\n---  TYPE [%s] ---",TYPE.c_str());					

				if(!memcmp(FORMAT.c_str(),"TLV",3))  //To verify if the field has TLV format
				{  
					LOGF_TRACE("\nTLV format");

					if((ALIAS.c_str() != NULL) && (ALIAS != "null") && (ALIAS.length() > 0) )
					{
						vdAsc2Hex(ALIAS.c_str(), field, ALIAS.length()/2);
						if(field[0] != buff_temp[in_idx])
						{
							inResult = XML_CMD_FAIL;
							free(data);
							LOGF_TRACE("\nALIAS FAIL");
							break;
						}
					}

					lengTLV = 0;
					in_idx++;
					lengTLV = buff_temp[in_idx++];
					memcpy(data,buff_temp + in_idx,lengTLV);
					in_idx += lengTLV;
					countbytes += (lengTLV + 2);					
					//DUMP_XML_HEX(data,lengTLV,(char*)"Data");
					//LOGAPI_HEXDUMP_TRACE("Data",data,lengTLV);
					vdSetByteValuePP((char* )VAL.c_str(),data,lengTLV);	
				}
				else if(!memcmp(FORMAT.c_str(),"FIX",3))  //To verify if the field has defined length 
				{
					//LOGF_TRACE("\nFormat with fix length");
					memcpy(data, buff_temp + in_idx,len_fld);
					in_idx += len_fld;
					countbytes += len_fld; 					
					//DUMP_XML_HEX(data,len_fld,(char*)"Data");
					//LOGAPI_HEXDUMP_TRACE("Data",data,len_fld);
					vdSetByteValuePP((char* )VAL.c_str(),data,len_fld);	
				}		
				else if(!memcmp(FORMAT.c_str(),"VAR",3))
				{
					char* datavar;
					char* szVar;		
					unsigned int lengVar = 0;
					
					//LOGF_TRACE("\nFormat with variable length");
					//LOGF_TRACE("\nFormat with variable length  %i",len_fld);					
					szVar = (char*)malloc(len_fld + 1);
					memcpy(szVar,buff_temp + in_idx,len_fld);						
					//LOGAPI_HEXDUMP_TRACE("VAR long 1",szVar,len_fld);
					szVar[len_fld ]	= 0x00;		
					//LOGAPI_HEXDUMP_TRACE("VAR long 2",szVar,len_fld);
					if(!memcmp(TYPE.c_str(),"ASC",3))					
						lengVar = 2*atoi(szVar);					
					if(!memcmp(TYPE.c_str(),"HEX",3) || !memcmp(TYPE.c_str(),"NUM",3))					
						lengVar = atoi(szVar);
					// FAG 22-jun-2017 se pone otro if para que guarde la longitud variable de un dato en HEXADECIMAL
					if(!memcmp(TYPE.c_str(),"2HEX",4) )	
						{
							
							unsigned int sizeparcial = 0;
							LOGF_TRACE("\n 2HEX");
							sizeparcial |= szVar[0];	
							sizeparcial = sizeparcial << 8;
							sizeparcial |= szVar[1];	
							LOGF_TRACE("\nLength VAR: %i ",sizeparcial);
							lengVar=sizeparcial;
						}
					
					LOGF_TRACE("\nLength VAR: %i",lengVar);
					free(szVar);
					
					datavar = (char*)malloc(lengVar + 1);
					LOGF_TRACE("\nLength VAR: %i",lengVar);
					in_idx += len_fld;
					
					
					memcpy(datavar, buff_temp + in_idx,lengVar);
					in_idx += lengVar;
					countbytes += (len_fld + lengVar); 					
					
					//DUMP_XML_HEX(datavar,lengVar,(char*)"Data");
					//LOGAPI_HEXDUMP_TRACE("Data",datavar,lengVar);
					vdSetByteValuePP((char* )VAL.c_str(),datavar,lengVar);	
					free(datavar);
				}			
			 	
				memset(outbuff,0x00,sizeof(outbuff));
				soutbuff = inGetByteValuePP((char* )VAL.c_str(),outbuff);
				LOGF_TRACE("LEN [%d]",soutbuff);
				//DUMP_XML_HEX(outbuff,soutbuff,(char*)"***HEX OUT***");
				//LOGAPI_HEXDUMP_TRACE("***HEX OUT***",outbuff,soutbuff);

				if(countbytes == szbuff)
				{
					inResult = XML_CMD_OK;
					LOGF_TRACE("\nCHECK TOTAL SIZE COMPLETED");
					i = numbfields;					
				}
				
				if(countbytes != szbuff)		//We can check the length of the parsed buffer
				{
					inResult = XML_CMD_FAIL;
					LOGF_TRACE("\nCHECK TOTAL SIZE NOT COMPLETED");
				}
				free(data);
			}
			
		}
	}

	free(buff_temp);
	
	
	return inResult;
}

/*=======================================================================*/
/*=======================================================================*/
/*=======================================================================*/
/*XML DATA DEFINITIONS to COMMANDS OF PINPAD*/ 
/*=======================================================================*/
/*=======================================================================*/
/*=======================================================================*/
/*This group of function will be used to parse a XML with PINPAD command definition*/
/* track the current level in the xml tree */
int inPARSE_XMLCMDInit ( void )
{
	int ret = XML_CMD_OK;
	int retparser = 0;

	LOGF_TRACE("\n--- inPARSE_XMLCMDInit ---");

	retparser = parser_xml_PP(XML_CMD_PP);
	
	if( !retparser )
	{
		ret = XML_CMD_FAIL;
	}
	
	//DUMP_XML_HEX((void*)CmdObj.dump().c_str(), strlen (CmdObj.dump().c_str()),(char*)"CmdObj");
  	LOGAPI_HEXDUMP_TRACE("CmdObj",(void*)CmdObj.dump().c_str(), strlen (CmdObj.dump().c_str()));
	LOGF_TRACE("\n--- ret [%d] ---",ret);
	
	return ret;
}

static int depthcmd = 0;
/* first when start element is encountered */
static void start_element_PP(void *userData, const char *element, const char **attribute)
{
	int i;
	
	
	LOGF_TRACE("\n-- start_element -- ");
	LOGF_TRACE("\n-- depth  [%d] -- ",depthcmd);
	LOGF_TRACE("\n-- element[%s]", element);

	if( !memcmp (element , CMD_DEF ,strlen (CMD_DEF)) )
	  
	{
		strcpy(defCMD,element);		
		PPdescElement = PP_APPCMDS;
		strcpy(defHostName,(char*)STAND_ALONE);
	}
	else if( !memcmp (element , CMD_MSJ,strlen (CMD_MSJ)) )
	{
		strcpy(defMSJ,element);
		PPdescElement = PP_COMMANDNAME;	
	}
	else if(!memcmp (element , CMD_FIELD,strlen (CMD_FIELD)))
	{
		strcpy (defXMLElement,element);
		PPdescElement = PP_FIELD_CMD;
	}
	else
	{
		PPdescElement = PP_NODEFINE;
	}

	LOGF_TRACE("\nPPdescElement=%d",PPdescElement);

	for(i = 0; attribute[i]; i += 2)
	{
		LOGF_TRACE("\nElement[%s]",element);
		LOGF_TRACE("\nAttr : [%s]= [%s]", attribute[i], attribute[i + 1]);

		if(PPdescElement == PP_APPCMDS)
		{
			if(!memcmp(attribute[i],fld_AppName,7))
			{
				strcpy (defHostName,attribute[i + 1]);
			}
		}
		else if(PPdescElement == PP_COMMANDNAME)
		{
			if(!memcmp(attribute[i],"cmdName",7))
			{
				strcpy (def_type_cmd,attribute[i + 1]);
				LOGF_TRACE("\ndef_type_cmd[%s]",def_type_cmd);
				
			}
			else
			{
				LOGF_TRACE("\nCmdObj(%s)(%s)(%s)(%s)(%s)=(%s)",defCMD,defHostName,defMSJ,def_type_cmd, attribute[i], attribute[i + 1]);
				CmdObj (defCMD)(defHostName)(defMSJ)(def_type_cmd)(attribute[i]) = attribute[i + 1];	
			}
			
		}
		else if(PPdescElement == PP_FIELD_CMD)
		{
			
			LOGF_TRACE("\nCmdObj(%s)(%s)(%s)(%s)(%s)(%s)=(%s)",defCMD,defHostName,defMSJ,def_type_cmd,defXMLElement, attribute[i], attribute[i + 1]);
			CmdObj (defCMD)(defHostName)(defMSJ)(def_type_cmd)(defXMLElement) (attribute[i]) = attribute[i + 1];			
			
		}
	}
	depthcmd++;
}


/* decrement the current level of the tree */
static void end_element_PP(void *userData, const char *el)
{
	LOGF_TRACE("\n-- end_element -- ");
	LOGF_TRACE("\n-- depth  [%d] -- ",depthcmd);
	LOGF_TRACE("\n-- element[%s]", el);
	depthcmd--;

	LOGF_TRACE("\ndefElement[%s]", el);

}

static void element_value_PP(void *userData, const char *val, int len)
{
   int I,i=0;
   char cpy[128] = {};

   /* val is readonly and not NULL terminated but the lenth *
    * is given in len hency string of length len is copies  *
    * and NULL terminated                                   */
  //  LOGF_TRACE("\n-- element_value -- ");
  //  LOGF_TRACE("\n-- depth  [%d] -- ",depthcmd);
  //  LOGF_TRACE("\n-- len    [%d] -- ",len);
   // DUMP_XML_HEX( (void*)val, len,(char*)"value");
  // LOGAPI_HEXDUMP_TRACE("value",(void*)val, len);

   for(I = 0; I < len; I++)
      cpy[I] = val[I];
   cpy[I] = 0;

   while (cpy[i])  //if it's not alpha numeric value, not consider
   {
      if ( isspace  (cpy[i]) )
    	  return;
      i++;
   }

   //LOGF_TRACE("-- Value[%s]", cpy);

   if(PPdescElement == PP_FIELD_CMD)
   {
		LOGF_TRACE("\nCmdObj(%s)(%s)(%s)(%s)(%s)(%s)=(%s)",defCMD,defHostName,defMSJ,def_type_cmd,defXMLElement, PP_DEF_VAR, cpy);
		CmdObj (defCMD)(defHostName)(defMSJ)(def_type_cmd)(defXMLElement)(PP_DEF_VAR)= cpy;				
   }

}


static int parser_xml_PP(const char* filename)
{
	int ret = 1;
	FILE *fp;

    LOGF_TRACE("\n--- parser_xml ---");
    LOGF_TRACE("\n--- file [%s] ---",filename);

   

    XML_Parser parser = XML_ParserCreate(NULL);

   
    XML_SetElementHandler(parser, start_element_PP, end_element_PP);

    XML_SetCharacterDataHandler(parser, element_value_PP);

	if (file_size == 0)
	{
		fp = fopen(filename, "r");

		if(fp == NULL)
		{
			LOGF_TRACE("\nFailed to open file\n");
			ret = 0;
			return ret;
		}
	
		memset(Xmlbuff, 0, sizeof (Xmlbuff));
		file_size = fread(Xmlbuff, sizeof(char), sizeof (Xmlbuff), fp);
		fclose(fp);
	}
    //DUMP_XML_HEX( Xmlbuff, file_size,(char*)"read xml");
    LOGF_TRACE("read xml");
	//LOGAPI_HEXDUMP_TRACE("read xml",Xmlbuff, file_size);
    /* parse the xml */
    if(XML_Parse(parser, Xmlbuff, strlen(Xmlbuff), XML_TRUE) == XML_STATUS_ERROR)
    {
    	LOGF_TRACE("\nError: %s", XML_ErrorString(XML_GetErrorCode(parser)));
    	ret = 0;
    }

    XML_ParserFree(parser);

    return ret;
}

//*****************************************************************************************************************
//Function to calculate the LRC -- PDX TEAM
//*****************************************************************************************************************
//*****************************************************************************************************************
unsigned int CalcLRC(unsigned char* frame,int sizeframe){   
	unsigned int LRC = 0;
	int i = 0;
	int inResult = 0;
	LOGF_TRACE("\n----CalcLRC----");  
	//DUMP_XML_HEX(frame,sizeframe,(char*)"Frame before LRC");
	//LOGAPI_HEXDUMP_TRACE("Frame before LRC",frame,sizeframe);
	LRC = frame[1];
	for(i=2; i< sizeframe; i++)
		LRC = LRC ^ frame[i]; 
	
    LOGF_TRACE("\nlrc1=0x%02.2X\n", LRC); 
	//LOGF_TRACE("\nLRC frame=0x%02.2X", frame[sizeframe - 1]); 

	return LRC;
}

/**********************************************************************************
***********************************************************************************/
//SET VALUES
void vdSetStringValuePP(char *chField,string stData)
{
	string stTemp;
	stTemp.assign(chField, strlen(chField));
	
	stValue.insert(pair<string,string>( stTemp, stData  )); //PAR_TID

}

void vdSetIntValuePP(char *chField,char* chData)
{
	string stTemp;
	int inData = 0;
	stTemp.assign(chField, strlen(chField));
	inData = atoi(chData); 
	intValue.insert(std::pair<string,int>( stTemp, inData  )); //PAR_TID	
}

void vdSetByteValuePP(char *chField,char *chData ,int inLenField)
{
	char *chTemp;
	string stTemp = "";
	string stData = "";
	stTemp.assign(chField, strlen(chField));
	
	chTemp =(char *)malloc(2*inLenField + 1);
	//DUMP_XML_HEX(chData,inLenField,(char*)"Buffer****");
	//LOGAPI_HEXDUMP_TRACE("Buffer****",chData,inLenField);
	memset(chTemp,0x00,(2*inLenField + 1));
	
	vdHex2Asc(chData,chTemp,inLenField);
	
	stData.assign(chTemp,2*inLenField);
	//DUMP_XML_HEX(chTemp,2*inLenField,(char*)"chTemp****");
	//LOGAPI_HEXDUMP_TRACE("chTemp****",chTemp,2*inLenField);
	stValue.erase(stTemp.c_str());   //debug
	stValue.insert(std::pair<string,string>( stTemp,stData)); //PAR_TID
	
	free(chTemp);

}

//GET VALUES

string inGetStringValuePP(char *chField)
{
	map<string, string>::iterator itGetVal;
	std::string stTemp;
	stTemp.assign(chField, strlen(chField));
	
	itGetVal=stValue.find(stTemp);
	if(itGetVal == stValue.end())
		return "NULL"; //Elemento no encontrado
	else
	{
		return itGetVal->second;
	}
}

int inGetintValuePP(char *chField)
{
	map<string, int>::iterator itGetVal;
	string stTemp;
	stTemp.assign(chField, strlen(chField));
	
	itGetVal=intValue.find(stTemp);
	if(itGetVal == intValue.end())
		return -1; //Elemento no encontrado
	else
	{
		return  itGetVal->second;
	}
}

int inGetByteValuePP(char *chField,char *chDataOut)
{
	map<string, string>::iterator itGetVal;
	string stTemp = "";
	string stToHex = "";
	stTemp.assign(chField, strlen(chField));
	LOGF_TRACE("\nchField = %s",chField);
	itGetVal=stValue.find(stTemp);
	if(itGetVal == stValue.end())
		{
		LOGF_TRACE("\nCAMPO NO ENCONTRADO");
		return -1; //Elemento no encontrado
		}
	else
	{		
		stToHex = itGetVal->second;
		//LOGF_TRACE("\nstToHex.c_str() = %s",stToHex.c_str());
		//LOGF_TRACE("\nLENG = %i",stToHex.length());
		vdAsc2Hex((char *)stToHex.c_str(),chDataOut,(stToHex.length()/2));
		return (stToHex.length()/2);
	}
}


void clear_parse()
{
	stValue.clear();	
}
/*************************************************************************/
//Functions to do convertions


static void  vdAsc2Hex (const char *dsp, char *hex, int pairs)
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
/*************************************************************************
************************************************************************/
static void  vdHex2Asc(const char *hex, char *dsp, int count)
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
//**********************************************************************************************************************************************************

void DUMP_XML_HEX(void* input,int size,char* name)
{
	char* input_frame = (char*)input;
	int input_idx;
	int idx_asc = 0;
	int char_idx;
	int aux = 0;
	int i = 0 ;
	int  output_idx = 0;
	int column = 1;

	LOGF_TRACE("\n--------------------------------------------------");
	LOGF_TRACE("\n%s:\n",name);
	for(input_idx = 0;input_idx < size;input_idx++)
	{				
		for(char_idx = 0; char_idx < 2; char_idx++)
		{
			if(char_idx == 0)
			{
			aux = input_frame[input_idx] & 0xF0;
			aux >>= 4;
			aux &= 0x0F;			
			}else
				aux = input_frame[input_idx] & 0x0F;															
		
			if( (aux >= 0) && (aux <= 9) )		
				LOGF_TRACE("%X",aux);
				 
			if( (aux >= 10) && (aux <= 15))
			{	
				LOGF_TRACE("%X",aux);		
				aux = aux - 1;
				aux = aux & 0x07;		
			}		
			output_idx ++;				
		}	
		if(column == 8)		
			LOGF_TRACE(" ");	
		if(column == 16)
		{
			LOGF_TRACE("  |  ");
			for(i = 0;i < 16; i++)
			{
				LOGF_TRACE("%c",input_frame[idx_asc]);
				idx_asc++;
			}
			LOGF_TRACE("\n");
			column = 0;
		}
		else
			LOGF_TRACE(" ");	
		column ++;
	}
	if(column > 1)
	{
		for(i = 0;i < (50 - (3*column));i++)
			LOGF_TRACE(" ");			

		LOGF_TRACE("    |  ");
				for(i = 0;i < column; i++)
				{
					if((input_frame[idx_asc] == 0x00) || ((input_frame[idx_asc] >= 0x07) && (input_frame[idx_asc] <= 0x0D)))
						LOGF_TRACE(".");
					else
						LOGF_TRACE("%c",input_frame[idx_asc]);
					idx_asc++;
				}
	}
	LOGF_TRACE("\n--------------------------------------------------\n");
}

