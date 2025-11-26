#include "html/gui.h"
#include "com/libcom.h"
#include "log/liblog.h"
#include "sysinfo/sysinfo.h"
#include "ipc/jsobject.h"
#include "evt/libevt.h"

#ifndef _VRXEVO
#include <svcmgr/svc_net.h>
#endif

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "pthread.h"
#include "svc.h"

#include "ComRdc.h"

#ifdef _VRXEVO
	#define sleep(x) SVC_WAIT (x*1000) 
#endif 

//#define RAZOR 

//#define XDEBUG 1
#define XDEBUG 0

using namespace std;
using namespace vfigui;
using namespace vfisysinfo;
using namespace vfisysinfo;


typedef struct
{
	char *chSSID;
	int inKeyMgmt;
	char *chPSK;
}
ST_WLAN_DATA;



//GLOBAL 
ST_WLAN_DATA st_wlan_data;
com_ConnectHandle *Handle;
com_ErrorCodes gCom_errno = COM_ERR_NONE;
static int got_network_event = -1;
static int inWlanStatus = WLAN_NET_LINK_DOWN;



//BLUETOOTH
//struct com_networkEventData *nwBTdata;
vfiipc::JSObject jsBTobj;
int isBTConnected = 0; 
int inStatusConnect = 0; 

//#define WLAN_COM_NETWORK_FILENAME "./flash/net/WLAN_NET.xml"
#if (defined _VRXEVO)
#define PATH_COM_NETWORK_FILENAME "I:1/"
#else
#define PATH_COM_NETWORK_FILENAME "./flash/net/"
#endif
static string sCONNECTIONPROFILE ="";



//LOCAL
int inCOMFatalError(com_ErrorCodes com_errno);
int _getEnvFile_(char *chSection,char *chVar, char *chValue, int inLen );
int _putEnvFile_(char *chSection,char *chVar, char *chValue);
///////////////////////////////////////LOCAL////////////////////////////////


int inCOMFatalError(com_ErrorCodes com_errno)
{
	uiDisplay(1,uiPrint("Error com_Init [%d], %s", com_errno, com_GetErrorString(com_errno)));
	sleep(2);
	//LOGF_TRACE("com_Init failed with com_errno [%d], %s", com_errno, com_GetErrorString(com_errno));
	//LOGF_ALERT("Exit program!");
	//LOGAPI_DEINIT();
	return -1;
}

DllSPEC void connection_callback (enum com_ConnectionEvent event, enum com_ConnectionType type, const void *data, void *priv, com_ErrorCodes com_errno)
{
	//(void) event;
	//(void) priv;

	int inEventCon = -1;
	inEventCon = event;
	int inStatus;

	const char *PSK = NULL;
	const char *SSID = NULL;
	int inType = 0;
	
	LOGF_TRACE("*********connection_callback****************");


	switch(inEventCon)
	{
		case COM_EVENT_CONNECTION_ESTABLISHED:
			LOGF_TRACE("Connection established!");
			//JRS, 29082018 Contiene apuntadores dinamicos
			//memset(&st_wlan_data,0,sizeof(ST_WLAN_DATA));
			st_wlan_data.inKeyMgmt = -1;
			LOGF_TRACE("COM_EVENT_CONNECTION_ESTABLISHED");

			com_GetProfileNodePropertyString(data,type , COM_PROFILE_PROP_STRING_WLAN_SSID,0,(const char **)&st_wlan_data.chSSID, &com_errno);
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK SSID [%s] Err [%s]",st_wlan_data.chSSID,com_GetErrorString(gCom_errno));
			LOGF_TRACE("COM_EVENT_CONNECTION_ESTABLISHED");

			com_GetProfileNodePropertyString(data,type , COM_PROFILE_PROP_STRING_WLAN_PSK,0,(const char **)&st_wlan_data.chPSK, &com_errno);
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK SSID [%s] Err [%s]",st_wlan_data.chPSK,com_GetErrorString(gCom_errno));
			LOGF_TRACE("COM_EVENT_CONNECTION_ESTABLISHED");

			com_GetProfileNodePropertyInt (data,type , COM_PROFILE_PROP_INT_WLAN_KEY_MGMT,0,&st_wlan_data.inKeyMgmt, &com_errno);
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK TYPEKEY [%d] Err [%s]",st_wlan_data.inKeyMgmt,com_GetErrorString(gCom_errno));
			LOGF_TRACE("COM_EVENT_CONNECTION_ESTABLISHED");

			

			inStatus = CONNECTED;
			//JRS, 27082018 Global connection 
			inStatusConnect = inStatus; 
			break;
		case COM_EVENT_CONNECTION_NEXT:
			LOGF_TRACE("Trying new connection");

			//JRS, 29082018 Contiene apuntadores dinamicos 
			//memset(&st_wlan_data,0,sizeof(ST_WLAN_DATA));
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");
			st_wlan_data.inKeyMgmt = -1;
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");
			com_GetProfileNodePropertyString(data,type , COM_PROFILE_PROP_STRING_WLAN_SSID,0,(const char **)&st_wlan_data.chSSID, &com_errno);
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK SSID [%s] Err [%s]",st_wlan_data.chSSID,com_GetErrorString(gCom_errno));

			com_GetProfileNodePropertyString(data,type , COM_PROFILE_PROP_STRING_WLAN_PSK,0,(const char **)&st_wlan_data.chPSK, &com_errno);
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK SSID [%s] Err [%s]",st_wlan_data.chPSK,com_GetErrorString(gCom_errno));

			com_GetProfileNodePropertyInt (data,type , COM_PROFILE_PROP_INT_WLAN_KEY_MGMT,0,&st_wlan_data.inKeyMgmt, &com_errno);
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");
			//LOGF_TRACE("COM_PROFILE_PROP_STRING_WLAN_PSK TYPEKEY [%d] Err [%s]",st_wlan_data.inKeyMgmt,com_GetErrorString(gCom_errno));
			LOGF_TRACE("COM_EVENT_CONNECTION_NEXT");

			inStatus = CONNECTING;
			inStatusConnect = inStatus; 
			break;
		case COM_EVENT_CONNECTION_FAILED:
			LOGF_TRACE("Connection failed %d: %s", com_errno, com_GetErrorString(com_errno));
			inStatus = NOT_CONNECTED;
			inStatusConnect = inStatus; 
			break;
		case COM_EVENT_PROFILE_FAILED:
			LOGF_TRACE("Entire profile failed %d: %s", com_errno, com_GetErrorString(com_errno));
			inStatus = NOT_CONNECTED;
			inStatusConnect = inStatus; 
			break;
		case COM_EVENT_CONNECTION_SKIPPED:
			LOGF_TRACE("Connection skipped %d: %s", com_errno, com_GetErrorString(com_errno));
			inStatus = NOT_CONNECTED;
			inStatusConnect = inStatus; 
			break;
		default:
			LOGF_TRACE("COM EVENT %d - errno %d, %s", event, com_errno, com_GetErrorString(com_errno));
			inStatus = NOT_CONNECTED;
			inStatusConnect = inStatus; 
			break;

	}

	if(priv != NULL)
	{
		//If priv parameter is not NULL, it means that the cb function was called from
		//com_ConnectAsync, so priv is updated with the status of the connection
		*((int *)priv)=inStatus;
	}
}

//static int waitForData(com_ConnectHandle *handle, int writeable, unsigned int timeout)
static int waitForData(int writeable, unsigned int timeout)
{
	enum com_ConnectionType type = com_GetConnectionType(Handle, NULL);
	fd_set select_set;
	struct timeval select_timeout;
	int socket;

	if(type == COM_CONNECTION_TYPE_SSL)
	{
		socket = com_ConnectGetSslFD(Handle, NULL);
	}
	else
	{
		socket = com_ConnectGetFD(Handle, NULL);
	}
	select_timeout.tv_sec = timeout/1000;
	select_timeout.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&select_set);
	FD_SET(socket, &select_set);
	if(!writeable)
		return select(socket+1, &select_set, NULL, NULL, &select_timeout);
	else
		return select(socket+1, NULL, &select_set, NULL, &select_timeout);
}


static void network_event_handler (enum com_NetworkEvent event, enum com_NetworkType type, const void *data, void* priv, enum com_ErrorCodes com_errno)
{
	(void) priv;
	(void) com_errno;
	char chAux [50] = {0};
	//std::string sAux = "";

	struct com_networkEventData *nwdata = (struct com_networkEventData *)data;
	LOGF_TRACE("FUNCTION: network_event_handler");
	LOGAPI_HEXDUMP_TRACE("struct",nwdata,sizeof(nwdata));

	got_network_event = event;

	LOGF_TRACE("NETWORK TYPE [%d]",type);
	switch(event)
	{
		case COM_EVENT_NETWORK_INTERFACE_UP:
		{
			struct com_IPConfig config;
			enum com_ErrorCodes error;
			char* networkProfile = NULL;
			LOGF_TRACE("Network event COM_EVENT_NETWORK_INTERFACE_UP ERRNO[%d]",com_errno);

			if(type == COM_NETWORK_TYPE_WLAN)
				vdSetWlanNetStatus(WLAN_NET_LINK_UP);


			if (priv != NULL)
			{
				networkProfile = (char*)priv;
				LOGF_TRACE("NETWORK PROFILE [%s]",networkProfile);
				memset(&config, 0x00, sizeof(config));
				if (com_GetNetworkInfo(networkProfile, &config, &error) == 0)
				{
					LOGF_TRACE("DHCP enabled: %s", config.dhcp?"YES":"NO");
					LOGF_TRACE("OWN IP: %s", config.ip_addr);
					LOGF_TRACE("OWN NETMASK: %s", config.netmask);
					LOGF_TRACE("OWN GATEWAY: %s", config.gateway);
					LOGF_TRACE("OWN DNS1: %s", config.dns1);
					LOGF_TRACE("OWN DNS2: %s", config.dns2);
					LOGF_TRACE("OWN MAC: %s", config.mac);
				}
				else
				{
					LOGF_TRACE("com_GetNetworkInfo %s failed with com_errno [%d], %s", networkProfile, error, com_GetErrorString(error));
				}
			}
		}
			break;

		case COM_EVENT_NETWORK_INTERFACE_DOWN:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_INTERFACE_DOWN ERRNO[%d]",com_errno);
			if(type == COM_NETWORK_TYPE_WLAN)
				vdSetWlanNetStatus(WLAN_NET_LINK_DOWN);
			break;

		case COM_EVENT_SIM_PIN_REQUEST:
			LOGF_TRACE("Network event COM_EVENT_SIM_PIN_REQUEST ERRNO[%d]",com_errno);
			// set the correct PIN for the SIM
			if(com_SetDevicePropertyString(COM_PROP_GSM_SIM_PIN, "1234", &com_errno))
			{
				LOGF_TRACE("PIN could not be set [errno %d]", (int)com_errno);
			}
			break;

		case COM_EVENT_NETWORK_PACKET_SWITCH:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_PACKET_SWITCH ERRNO[%d]",com_errno);
			break;

		case COM_EVENT_NETWORK_SIGNAL:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_PACKET_SWITCH ERRNO[%d] SIGNAL[%s]",com_errno,nwdata->data1);
			break;

		case COM_EVENT_NETWORK_LINK_UP:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_LINK_UP ERRNO[%d]",com_errno);
			break;

		case COM_EVENT_NETWORK_LINK_DOWN:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_LINK_DOWN ERRNO[%d]",com_errno);
			break;

		case COM_EVENT_NETWORK_FAILED:
			LOGF_TRACE("Network event COM_EVENT_NETWORK_FAILED ERRNO[%d] [%s]",com_errno,com_GetErrorString(com_errno));
			break;
		/////////////////////////////////////////////////////////////////////////////////
		///                       NUEVOS EVENTOS BT                                  ///
		////////////////////////////////////////////////////////////////////////////////
		case COM_EVENT_BT_CONFIRM_PAIR_REQUEST:
			LOGF_TRACE("Confirmation is requested by terminal before processing BT Pair request [%s]",nwdata->data1);
			jsBTobj.clear();
			jsBTobj("pin") = std::string(nwdata->data1); 
			LOGF_TRACE("json loaded successfully [%s]", (jsBTobj.dump()).c_str());

		break;

		case COM_EVENT_BT_PAIR_DONE:
			LOGF_TRACE("Finished pairing process [%d] [%s]",com_errno,com_GetErrorString(com_errno));
		break;

		case COM_EVENT_BT_DISCOV_TO_ELAPSED:
			LOGF_TRACE("The discovery timeout after enabling discoverability elapsed ");
		break;

		case COM_EVENT_BT_SPP_SERVER_CONNECTED:
			LOGF_TRACE("A remote device has connected to an opened SPP server port");
		break;

		case COM_EVENT_BT_SPP_SERVER_DISCONNECTED:
			LOGF_TRACE("The connection on the SPP server port is lost");
		break;

		case COM_EVENT_BT_EXT_PIN:
			LOGF_TRACE("PIN entry on the remote has been requested. A PIN, [%s]",nwdata->data1);
			jsBTobj.clear();
			//if(jsBTobj.load(std::string(nwdata)))
			//	LOGF_TRACE("json loaded successfully");
		break;

		case COM_EVENT_BT_EXT_CONFIRM:
			LOGF_TRACE("Numeric comparison has been requested. [%s]",nwdata->data1);
			jsBTobj.clear();
			if(jsBTobj.load(std::string(nwdata->data1)))
				LOGF_TRACE("json loaded successfully");
		break;

		case COM_EVENT_BT_EXT_VISUALIZE:
			LOGF_TRACE("Visualizing has been requested. PIN, [%s]",nwdata->data1);
			jsBTobj.clear();
			if(jsBTobj.load(std::string(nwdata->data1)))
				LOGF_TRACE("json loaded successfully");
		break;

		case COM_EVENT_BT_EXT_TIMEOUT:
			LOGF_TRACE("The pairing with the remote device has timed out [%s]",nwdata->data1);
			jsBTobj.clear();
			if(jsBTobj.load(std::string(nwdata->data1)))
				LOGF_TRACE("json loaded successfully");
		break;

		case COM_EVENT_BT_EXT_FAILED:
			LOGF_TRACE("The pairing with the remote device has failed due a negotiation error. [%s]",nwdata->data1);
			jsBTobj.clear();
			if(jsBTobj.load(std::string(nwdata->data1)))
				LOGF_TRACE("json loaded successfully");
		break;

		case COM_EVENT_BT_EXT_SUCCESS:
			LOGF_TRACE("The pairing with the remote device has succeded. [%s]",nwdata->data1);
			jsBTobj.clear();
			if(jsBTobj.load(std::string(nwdata->data1)))
				LOGF_TRACE("json loaded successfully");
  		break;

		default:
			LOGF_TRACE("%s Got event! event=%d type=%d data=%s", __func__, event, type, nwdata->data1);
	}

}

////////////////////////////////API LIB ////////////////////////////////////

//int inInitComAdk(enum com_ErrorCodes com_errno)
DllSPEC int inInitComAdk()
{
	//enum com_ErrorCodes com_errno;
	int rc=0;

	st_wlan_data.chSSID = NULL;
	st_wlan_data.chPSK = NULL;
	st_wlan_data.inKeyMgmt = -1;
	
	while(com_Init(&gCom_errno)) //Initialize ADKCOM
	{
		if(gCom_errno==6) //comdaemon not active yet. Retry after a few seconds
		{
			if(rc <= 4)
			{
				rc++;
				sleep(10);
				//LOGF_TRACE("com_Init failed with com_errno [%d], %s", com_errno, com_GetErrorString(com_errno));
			}
			else
			{
				//Could not initialize comdaemon. Finish the program
				//return (inCOMFatalError(gCom_errno)); //JRS  21/02/2018 No mostramos error y regresamos -1 para controlar en aplicacion
				return 1;
			}
		}
		else
		{
			//An error occurred. Finish the program
			//return (inCOMFatalError(gCom_errno)); //JRS  21/02/2018 No mostramos error y regresamos -1 para controlar en aplicacion
			return 1;
		}
	}
	return 0; //EXITOSO, CUALQUIER OTRO VALOR ES ERROR
}


//void vdDestroyComAdk(void)
DllSPEC int inDestroyComAdk(void)
{
	int inRes=0;
	inRes=com_Destroy(NULL);
	LOGF_TRACE("inRes %i",inRes);
	return inRes;
}


//int inComConnectAdk(com_ConnectHandle *Handle,const char *chProfile,int inTimeOut,int inTry,com_ErrorCodes *com_errno)
//int inComConnectAdk(const char *chProfile,int inTimeOut,int inTry)
DllSPEC int inComConnectAdk(int inTimeOut,int inTry)
{
	//enum com_ErrorCodes com_errnon;
	int inRet = 1;
	int inRes = 0;
	char chprof[100] = {0};
	char chBuff[1] = {0};
	 

	//sprintf(chprof, "./flash/net/%s",chProfile);
	#if (defined _VRXEVO)
	sprintf(chprof, "I:1/%s",sGetConnectionProfile().c_str());
	#else
	sprintf(chprof, "./flash/net/%s",sGetConnectionProfile().c_str());
	#endif
	LOGF_TRACE("NETWORK FILE [%s]\n",chprof);
	
	while (inTry)
	{
		Handle = 0;

		LOGF_TRACE("INICIA INTENTO DE CONEXION");
		if(inTimeOut != 0)
			Handle = com_Connect(chprof, connection_callback, NULL, inTimeOut, &gCom_errno);
		else
			Handle = com_Connect(chprof, connection_callback, NULL, 90000, &gCom_errno);
		
		LOGF_TRACE("TERMINA INTENTO DE CONEXION");

		LOGF_TRACE("HANDLER [%d]\n",(int) Handle);
		//sleep(1);
		if(!Handle) 
		{
			LOGF_TRACE("Handler Error!\n");
			/*if(inTry)
				//return (inCOMFatalError(gCom_errno)); //JRS  21/02/2018 No mostramos error y regresamos -1 para controlar en aplicacion
				return -1;
			else*/
			//JRS 21/02/2018 SE CORRIGE LOGICA DE INTENTOS
			if(inTimeOut != 0)
				inTry-=1;
			else
				LOGF_TRACE("INTENTOS INFINITOS");
		
			//sleep(1);
			continue; 
		}
		else
		{

			//inUSB_LINK();

			LOGF_TRACE("Handler OK!\n");
			inRet = 0;
			break;
		}
	}
	
	LOGF_TRACE("OPEN SUCCES!\n");
	return inRet;
}

DllSPEC int inComConnectAsyncAdk(int inTry)
{
	//enum com_ErrorCodes com_errnon;
	int inRet = 1;
	int inRes = 0;
	char chprof[100] = {0};
	char chBuff[1] = {0};
	 

	//sprintf(chprof, "./flash/net/%s",chProfile);
	#if (defined _VRXEVO)
	sprintf(chprof, "I:1/%s",sGetConnectionProfile().c_str());
	#else
	sprintf(chprof, "./flash/net/%s",sGetConnectionProfile().c_str());
	#endif
	LOGF_TRACE("NETWORK FILE [%s]\n",chprof);
	
	while (inTry)
	{
		Handle = 0;

		Handle = com_ConnectAsync(chprof, connection_callback, NULL, &gCom_errno);
		
		
		LOGF_TRACE("HANDLER [%d]\n",(int) Handle);
		sleep(1);
		if(!Handle) 
		{
			LOGF_TRACE("Handler Error!\n");
			
			inTry-=1;
			sleep(1);
			
			continue; 
		}
		else
		{

			LOGF_TRACE("Handler OK!\n");
			inRet = 0;
			break;
		}
	}
	
	LOGF_TRACE("OPEN SUCCES!\n");
	return inRet;
}

DllSPEC int inComConnectWait(int inTO)
{
	com_WaitStatus waitRes = COM_CONNECTION_STATUS_FAIL;

	waitRes = com_ConnectWait(Handle,inTO,&gCom_errno);

	LOGF_TRACE("inComConnectWait [%d] [%d] = [%s]", waitRes, gCom_errno, com_GetErrorString(gCom_errno));

	return (int)waitRes;
}

//int inSendAdk(com_ConnectHandle *Handle,char *chSendBuffer, int inSendLen,com_ErrorCodes *com_errno)
DllSPEC int inSendAdk(char *chSendBuffer, int inSendLen)
{
	int inLenTmp = 0;
	int inRet = 0;
	//enum com_ErrorCodes com_errnoTmp;
	
	while(inLenTmp != inSendLen)
	{
		LOGF_TRACE("inLenTmp [%d] inSendLen [%d]",inLenTmp , inSendLen);
		inRet = com_Send(Handle, chSendBuffer + inLenTmp, inSendLen - inLenTmp, &gCom_errno);
		
		LOGF_TRACE("inRet [%d]",inRet);
		
		if(inRet == -1)
		{
			if(gCom_errno > COM_SYSTEM_ERRNO_BASE && ((gCom_errno - COM_SYSTEM_ERRNO_BASE == EAGAIN) || (gCom_errno - COM_SYSTEM_ERRNO_BASE == EWOULDBLOCK)))
			{
				//Timeout or error while waiting for buffer to be cleared
				//if(waitForData(Handle, 1, 200) <= 0) 
				if(waitForData(1, 200) <= 0) 
					break;
			}
			else
			{
				//critical Error
				LOGF_TRACE("Critical Error");
				return -15;
			}
		}
		else if(inRet == 0)
		{
			//Timeout or error while waiting for buffer to be cleared
			//if(waitForData(Handle, 1, 200) <= 0) 
			if(waitForData( 1, 200) <= 0) 
				break;
		}
		
		if(inRet > 0)
			inLenTmp += inRet;	
	}
	
	return inLenTmp; //NUMERO DE BYTES ENVIADOS
}

//int inRcvAdk(com_ConnectHandle *Handle,char *chRcvBuffer,int inMaxRcv,int inTimeO, com_ErrorCodes *com_errno)
DllSPEC int inRcvAdk(char *chRcvBuffer,int inMaxRcv,int inTimeO)
{
	int inLenTmp = 0;
	int inRet = 0;
	int inRcv = 0;
	int inWait = 0;

	//enum com_ErrorCodes com_errnoTmp;
		
	inLenTmp = -1;
	
	//LOGF_TRACE("****inRcvAdk*****");
	//inCheckConnectionState();
	while(1)
	{
		
		//if(waitForData(Handle, 0, inTimeO) <= 0)
		inWait = waitForData( 0, inTimeO);
		//if(waitForData( 0, inTimeO) <= 0)
		if (XDEBUG) LOGF_TRACE("WAIT FOR DATA RESPONSE [%d]",inWait );
		if(inWait <= 0)
		{
			//If no data, get out.
			if (XDEBUG) LOGF_TRACE("****NO DATA HANDLER[%d]*****\n",(int) Handle);
			break;
		};
		
		LOGF_TRACE("****RECIBIENDO\n*****");
		
		inRet = com_Receive(Handle, chRcvBuffer + inRcv, inMaxRcv - inRcv, &gCom_errno);
		
		LOGF_TRACE("****inRcvAdk[%d]*****\n",inRet);
		LOGAPI_HEXDUMP_TRACE("inRcvAdk",chRcvBuffer,inRet);  // para observar que informacion me llega en el buffer
		LOGF_TRACE(" Handle-> %d",(int*)Handle);
		LOGF_TRACE("gCom_errno -> %d",gCom_errno);
		LOGF_TRACE("com_GetErrorString -> %s",com_GetErrorString(gCom_errno));
		
		if( inRet == 0 )
		{
			
			/*if(inCheckConnectionState())
			{

				//CONEXIONES IP//BT
				LOGF_TRACE("Conection has Closed");
				LOGF_TRACE("com_GetErrorString -> %s",com_GetErrorString(gCom_errno));
				//vdConnectCloseAdk();
			}*/

			return -15;
			
		}
		else if(inRet == -1 )
		{
			//LOGF_TRACE("Problem reading data: %d, %s", com_errno[0], com_GetErrorString(com_errno[0]));
			LOGF_TRACE("Problem reading data");
			break;
		}
		else if(inRet > 0 )
		{	
			LOGF_TRACE("inRet > 0");
			inRcv += inRet;
			inLenTmp = inRcv;
			if(inRcv >= inMaxRcv)
			{
					LOGF_TRACE("\nTHIS IS THE LIMIT OF ARRAY TO AVOID AN OVERFLOW");
					break;
			}			
		}

	}
	
	return inLenTmp; //NUMERO DE BYTES ENVIADOS
}



//int inDoSerialSendRecv(com_ConnectHandle *Handle,char *chSendBuffer,int inSendLen ,char *chRcvBuffer,int inRecvLen)
DllSPEC int inDoSerialSendRecv(char *chSendBuffer,int inSendLen ,char *chRcvBuffer,int inRecvLen)
{
	int inRet = 0;
	//enum com_ErrorCodes com_errno = COM_ERR_NONE;
	
	inRet = com_Send(Handle, chSendBuffer, inSendLen, &gCom_errno);
    if(inRet > 0) 
	{
		
		while(1)
		{
			inRet = com_Receive(Handle, chRcvBuffer, inRecvLen, &gCom_errno);
			if(inRet > 0) 
			{
				return inRet;
			}
			else
			{
				//MANEJO DE ERRORES
				continue;
			}
		}
	}
	return -1;
}

//IP / GPRS

DllSPEC int netAttach(char *netprofile)
{
	int rc;
	enum com_ErrorCodes com_errno;
	char netprof [100] = {0};

	#if (defined _VRXEVO)
	sprintf(netprof, "I:1/%s",netprofile);
	#else
	sprintf(netprof, "./flash/net/%s",netprofile);
	#endif
	LOGF_TRACE("FUNCTION: netAttach to [%s]",netprof);

	com_NetworkSetCallback(network_event_handler, (void*)netprof, NULL);

	rc = com_AttachNetwork(netprof, &com_errno);
	LOGF_TRACE( "COM ATTACH to %s result=%d errno=%d", netprof, rc, com_errno);

	return rc;
}

 DllSPEC int netDetach(char *netprofile)
{
	int rc;
	enum com_ErrorCodes com_errno;
	char netprof [100] = {0};
	
	#if (defined _VRXEVO)
	sprintf(netprof, "I:1/%s",netprofile);
	#else
	sprintf(netprof, "./flash/net/%s",netprofile);
	#endif


	LOGF_TRACE("FUNCTION: netDetach [%s]",netprof);

	//com_NetworkSetCallback(network_event_handler, NULL, NULL);

	rc = com_DetachNetwork(netprof, &com_errno);

	LOGF_TRACE( "COM DETACH to %s result=%d errno=%d", netprof, rc, com_errno);

	return rc;
}

DllSPEC int netRestart(char *netprofile)
{
	int rc;
	enum com_ErrorCodes com_errno;
	char netprof [100] = {0};

	#if (defined _VRXEVO)
	sprintf(netprof, "I:1/%s",netprofile);
	#else
	sprintf(netprof, "./flash/net/%s",netprofile);
	#endif
	LOGF_TRACE("FUNCTION: netRestart");

	//com_NetworkSetCallback(network_event_handler, (void*)netprofile, NULL);

	rc = com_NetworkRestart(netprof, &com_errno);

	LOGF_TRACE("COM RESTART to %s result=%d errno=%d", netprof, rc, com_errno);

	return rc;
}

/*int main(){
    
	int inRes = 0;
	com_ConnectHandle *Handle;
	if(!inInitComAdk())
	{
		//EXITO
	    // gHandle = com_Connect("profile_name.xml", connection_callback, NULL, 10000, NULL);
		//if(!gHandle) LOGF_TRACE("error\n"); //Connection didn’t work
		inRes = inComConnectRs232Adk (Handle,"profile_name.xml",1000);
		if(!inRes)
		{
			//RECV
			//SEND
			
		}
		else 
		{
			com_ConnectClose(Handle, NULL); //Connection did work, but now close the connection
		}
		
		vdDestroyComAdk();
	}
}*/


DllSPEC int check_USB()
{
	int result=0;
	struct com_USBInfo* next=NULL;
	com_USBInfo USB_DEVICE;
	com_ErrorCodes ERROR_USB;
	
	LOGF_TRACE("******check_USB*******");
	LOGF_TRACE("USB_DEVICE %x", next);
	LOGF_TRACE("USB_DEVICE %p", next);
	LOGF_TRACE("******check_USB*******");
	
	result=com_GetUSBInfo(&next,&ERROR_USB);
	LOGF_TRACE("result %i", result);
	LOGF_TRACE("USB_DEVICE %s", USB_DEVICE.idProduct);
	LOGF_TRACE("USB_DEVICE %s", USB_DEVICE.idVendor);
	LOGF_TRACE("USB_DEVICE %s", USB_DEVICE.ttyDevice);
	LOGF_TRACE("USB_DEVICE %p", next);
	LOGAPI_HEXDUMP_TRACE("USB DEVICE STRUCT",(char*)next,sizeof(struct com_USBInfo));
	com_USBInfo_Free(next);

	return 0;
}


DllSPEC int inUSB_LINK()
{

	int result=0;
	int inValue=0;
	int ComConnectionType = 0;
	int comConnectionState = 0; 


#if (defined _VRXEVO) || (defined RAZOR)
//#ifdef RAZOR
	return 1;
#else
	
	enum com_ErrorCodes ERROR_USBLINK;
	LOGF_TRACE("******USB_LINK*******");
	LOGF_TRACE("COMPROBAMOS PRIMERO EL MODO DE USB");
	result=com_GetDevicePropertyInt(COM_PROP_USB_GADGET_MODE_ACTIVE, &inValue, &ERROR_USBLINK);
	LOGF_TRACE("result COM_PROP_USB_GADGET_MODE_ACTIVE :%i",result);
	LOGF_TRACE("inValue COM_PROP_USB_GADGET_MODE_ACTIVE :%i",inValue);
	LOGF_TRACE(" com_GetErrorString  COM_PROP_USB_GADGET_MODE_ACTIVE : %s",com_GetErrorString(ERROR_USBLINK));	
	
	
	if(result<0)
	{
		LOGF_TRACE("FALLA EL TIPO DE USB, NO SE PUEDE CONSULTAR");
		return result;
	}
	
	if(inValue!=COM_USBGADGET_SERIAL_RNDIS)
	{
		inValue=0;
		result=com_SetDevicePropertyInt(COM_PROP_USB_GADGET_MODE,COM_USBGADGET_SERIAL_RNDIS, &ERROR_USBLINK);
		LOGF_TRACE("com_GetErrorString   COM_PROP_USB_GADGET_MODE-COM_USBGADGET_SERIAL :%s",com_GetErrorString(ERROR_USBLINK));	
		if(result==0)
		{
			LOGF_TRACE("MODO SERIAL, REINICIO DE TERMINAL");	
			uiDisplay(1,uiPrint("PREPARANDO MEDIOS...."));
			sleep(1);
			uiDisplay(1,uiPrint("REINICIANDO TERMINAL...."));
			sleep(1);
			result=sysReboot();
			LOGF_TRACE("result = %i",result);	
		}
		else if(result<0)
		{
			LOGF_TRACE("NO SE PUEDE ACCESAR A ESE MODO");
			//return result;
		}
		return result;		
	}
	else
	{

		inValue=0;
		result=com_GetDevicePropertyInt(COM_PROP_ETH_USB_GADGET_LINK_STATUS, &inValue, &ERROR_USBLINK);
		LOGF_TRACE("result COM_PROP_ETH_USB_GADGET_LINK_STATUS : %i",result);
		LOGF_TRACE("inValue COM_PROP_ETH_USB_GADGET_LINK_STATUS : %i",inValue);
		LOGF_TRACE("inValue COM_PROP_ETH_USB_GADGET_LINK_STATUS : %s",com_GetErrorString(ERROR_USBLINK));

		if(inValue==1)
		{
			LOGF_TRACE("CABLE CONECTADO");
			//return inValue;

		}
		else if(inValue==0)
		{
			LOGF_TRACE("CABLE NO CONECTADO , CAMBIAR EL PERFIL DE CONEXION");
			//return inValue;

		}
		
		return inValue;

	}
#endif

}

DllSPEC int inEthLinkStatus()
{
	int inSig = 0;
	enum com_ErrorCodes com_errno;

	com_GetDevicePropertyInt(COM_PROP_ETH_0_LINK_STATUS,&inSig,&com_errno);

	return inSig;
}

DllSPEC void vdConnectCloseAdk()
{
	int inRes=0;
	inRes=com_ConnectClose(Handle, NULL); //Connection did work, but now close the connection
	LOGF_TRACE("inRes = %i",inRes);
	Handle = 0;
}


DllSPEC int inCreateConnectionProfile(string sCONPROF, string sTYPE,string sNETPROF, string sURL, string sPORT,string sServer, string sTO )
{

	FILE* hConnProfileFile;
	string sBuffer = "";
	char chConnProf [100] = {0};
	char chDump[2000] = {0};

	//TODO: MANEJAR MAS PERFILES DE CONEXION
	sBuffer =  "<CONNECTION_PROFILE>"
					"<CONNECTION>"
						"<TYPE>" + sTYPE + "</TYPE>"
						//"<NETWORK>./flash/net/" + sNETPROF + "</NETWORK>"
						"<NETWORK>" + PATH_COM_NETWORK_FILENAME + sNETPROF + "</NETWORK>"
						"<ADDRESS>" + sURL + "</ADDRESS>"
						"<PORT>" + sPORT + "</PORT>"
						"<SERVER>" + sServer + "</SERVER>"
						"<TIMEOUT>" + sTO + "</TIMEOUT>"
						"<SO_REUSEADDR>1</SO_REUSEADDR>"	
					"</CONNECTION>"
				"</CONNECTION_PROFILE>";


	//sprintf(chConnProf,"flash/net/%s",sCONPROF.c_str());
	sprintf(chConnProf,"%s%s",PATH_COM_NETWORK_FILENAME,sCONPROF.c_str());

	//hConnProfileFile = fopen(sCONPROF.c_str(), "w");
	hConnProfileFile = fopen(chConnProf, "w");

	LOGF_TRACE("%s",sBuffer.c_str());


	if (!hConnProfileFile)
	{
		LOGF_TRACE("ERROR CREANDO ARCHIVO");
		return 1;
	}

	LOGF_TRACE("%s",sBuffer.c_str());
	fputs(sBuffer.c_str(), hConnProfileFile);

	fclose(hConnProfileFile);

/*
	hConnProfileFile = fopen(chConnProf, "r");
	fseek (hConnProfileFile , 0 , SEEK_SET);
	fread(chDump,1,sizeof(chDump),hConnProfileFile);
	LOGAPI_HEXDUMP_TRACE("CONNECTION PROFILE",chDump,sizeof(chDump));*/

	return 0;
}


////////////////////////////////////////////////////////////
///                 ETH                                  ///
////////////////////////////////////////////////////////////
DllSPEC int inCreateEthNetworkProfile(string sPROFNAME, string sStartMode,string sDhcp,string sIpAddr,string sNetMask,string sGateWay,string sDns1,string sDns2, string sTO)
{

	FILE* hConnProfileFile;
	string sBuffer = "";
	char chProfName[50] = {0};
	char chDump [1000] = {0};


	sprintf(chProfName,"%s%s",PATH_COM_NETWORK_FILENAME,sPROFNAME.c_str());

	LOGF_TRACE("LAN = NETPROFILE [%s]",chProfName );

	if(sDhcp == "1")
	{
		sBuffer = 	"<NETWORK_PROFILE>"
					    "<TYPE>LAN</TYPE>"
					    "<DEVICE_NAME>ETH0</DEVICE_NAME>"
					    "<STARTUP_MODE>" + sStartMode + "</STARTUP_MODE>"
					    "<TIMEOUT>" + sTO + "</TIMEOUT>"
					    "<DHCP_ENABLED>"+ sDhcp +"</DHCP_ENABLED>"
					"</NETWORK_PROFILE>";
	}
	else
	{
				sBuffer = 	"<NETWORK_PROFILE>"
					    "<TYPE>WLAN</TYPE>"
					    "<DEVICE_NAME>ETH0</DEVICE_NAME>"
					    "<STARTUP_MODE>" + sStartMode + "</STARTUP_MODE>"
					    "<TIMEOUT>" + sTO + "</TIMEOUT>"
					    "<DHCP_ENABLED>"+ sDhcp +"</DHCP_ENABLED>"
					    "<IP_ADDRESS>" + sIpAddr + "</IP_ADDRESS>"
					    "<NETMASK>" + sNetMask + "</NETMASK>"
					    "<GATEWAY>" + sGateWay + "</GATEWAY>"
					    "<DNS_1>" + sDns1 + "</DNS_1>"
					    "<DNS_2>" + sDns2 + "</DNS_2>"
					"</NETWORK_PROFILE>";
	}
	//hConnProfileFile = fopen(WLAN_COM_NETWORK_FILENAME, "w");
	hConnProfileFile = fopen(chProfName, "w");

	LOGF_TRACE("%s",sBuffer.c_str());


	if (!hConnProfileFile)
	{
		LOGF_TRACE("ERROR CREANDO ARCHIVO");
		return 1;
	}

	LOGF_TRACE("%s",sBuffer.c_str());
	fputs(sBuffer.c_str(), hConnProfileFile);
	fclose(hConnProfileFile);

	hConnProfileFile = fopen(chProfName, "r");
	fseek (hConnProfileFile , 0 , SEEK_SET);
	fread(chDump,1,sizeof(chDump),hConnProfileFile);
	LOGAPI_HEXDUMP_TRACE("nNetworkProfile",chDump,sizeof(chDump));

	return 0;
}


////////////////////////////////////////////////////////////
///                 WLAN                                 ///
////////////////////////////////////////////////////////////

DllSPEC int inCreateWlanNetworkProfile(string sPROFNAME, string sSSID, string sKeyMgmt, string sPSK, string sStartMode,string sDhcp,string sIpAddr,string sNetMask,string sGateWay,string sDns1,string sDns2, string sTO)
{

	FILE* hConnProfileFile;
	string sBuffer = "";
	char chProfName[50] = {0};
	char chDump [1000] = {0};


	sprintf(chProfName,"%s%s",PATH_COM_NETWORK_FILENAME,sPROFNAME.c_str());

	LOGF_TRACE("WLAN = NETPROFILE [%s] SSID [%s] PKS [%s]",chProfName, sSSID.c_str(), sPSK.c_str() );

	if(sDhcp == "1")
	{
		sBuffer = 	"<NETWORK_PROFILE>"
					    "<TYPE>WLAN</TYPE>"
					    "<DEVICE_NAME>WLAN0</DEVICE_NAME>"
					    "<STARTUP_MODE>" + sStartMode + "</STARTUP_MODE>"
					    "<TIMEOUT>" + sTO + "</TIMEOUT>"
					    "<DHCP_ENABLED>"+ sDhcp +"</DHCP_ENABLED>"
					    "<WLAN_NODE>"
					        "<SSID>" + sSSID + "</SSID>"
					        "<KEY_MGMT>"+ sKeyMgmt +"</KEY_MGMT>"
					        "<PSK>" + sPSK + "</PSK>"
					    "</WLAN_NODE>"
					"</NETWORK_PROFILE>";
	}
	else
	{
				sBuffer = 	"<NETWORK_PROFILE>"
					    "<TYPE>WLAN</TYPE>"
					    "<DEVICE_NAME>WLAN0</DEVICE_NAME>"
					    "<STARTUP_MODE>" + sStartMode + "</STARTUP_MODE>"
					    "<TIMEOUT>" + sTO + "</TIMEOUT>"
					    "<DHCP_ENABLED>"+ sDhcp +"</DHCP_ENABLED>"
					    "<IP_ADDRESS>" + sIpAddr + "</IP_ADDRESS>"
					    "<NETMASK>" + sNetMask + "</NETMASK>"
					    "<GATEWAY>" + sGateWay + "</GATEWAY>"
					    "<DNS_1>" + sDns1 + "</DNS_1>"
					    "<DNS_2>" + sDns2 + "</DNS_2>"
					    "<WLAN_NODE>"
					        "<SSID>" + sSSID + "</SSID>"
					        "<KEY_MGMT>"+ sKeyMgmt +"</KEY_MGMT>"
					        "<PSK>" + sPSK + "</PSK>"
					    "</WLAN_NODE>"
					"</NETWORK_PROFILE>";
	}
	//hConnProfileFile = fopen(WLAN_COM_NETWORK_FILENAME, "w");
	hConnProfileFile = fopen(chProfName, "w");

	LOGF_TRACE("%s",sBuffer.c_str());


	if (!hConnProfileFile)
	{
		LOGF_TRACE("ERROR CREANDO ARCHIVO");
		return 1;
	}

	LOGF_TRACE("%s",sBuffer.c_str());
	fputs(sBuffer.c_str(), hConnProfileFile);
	fclose(hConnProfileFile);

	hConnProfileFile = fopen(chProfName, "r");
	fseek (hConnProfileFile , 0 , SEEK_SET);
	fread(chDump,1,sizeof(chDump),hConnProfileFile);
	LOGAPI_HEXDUMP_TRACE("nNetworkProfile",chDump,sizeof(chDump));

	return 0;
}

//vfiipc::JSObject inCom_WlanScan(char *data, int inBuffLen)
DllSPEC int inCom_WlanScan(vfiipc::JSObject& jsobj)
{

	int inRet = 0;
	char data[16000];

	if(com_WirelessScan(COM_WIRELESS_TYPE_WLAN, data, sizeof(data),&gCom_errno))
	{
		LOGF_TRACE( "WLAN Site Survey could not be done [errno %d: %s]", (int)gCom_errno, com_GetErrorString(gCom_errno));
		inRet = 1;
	}
	else
	{
	    LOGF_TRACE( "WLAN Site Survey: %s", data);
	    if(!jsobj.load(std::string(data)))
	    {
	    	inRet = 1;
    	}
	}

	return inRet;
}

DllSPEC std::vector<WLAN_Node> wirelessNetworkScan(){
	enum com_ErrorCodes com_errno;
	char data[16000] = {0};
	std::vector<WLAN_Node> vWlanNodes;

	if(com_WirelessScan(COM_WIRELESS_TYPE_WLAN, data, sizeof(data), &com_errno)){
	    LOGF_TRACE( "WLAN Site Survey could not be done [errno %d: %s]", (int)com_errno, com_GetErrorString(com_errno));
	} else {
	    LOGAPI_HEXDUMP_TRACE( "WLAN Site Survey: ", data, sizeof(data));
	    vfiipc::JSObject jsobj;
	    if(jsobj.load(std::string(data))){
	        LOGF_TRACE("Wlan Site Survey Parsed successfully");
	        
	        char surveyNumber[3] = {0};
	        
	        if (!jsobj.exists(COM_WLAN_COUNT)) {
	            LOGF_ERROR("NO WLAN_COUNT found");
	        } else {
	            int max = jsobj(COM_WLAN_COUNT).getInt();
	            LOGF_TRACE("** Found %d WLAN stations **", max);
	            
	            for(int i = 0 ; i<max; i++){
	            	vWlanNodes.push_back(WLAN_Node());
	                
	                snprintf(surveyNumber,sizeof(surveyNumber), "%u", i);
	                surveyNumber[2] = '\0';
	                
	                std::string name("WLAN");
	                std::string entry;
	                name.append(surveyNumber);
	                
	                entry = name;
	                entry.append("_SSID");
	                
	                if (jsobj.exists(entry))
	                    strcpy( vWlanNodes[i].SSID, jsobj(entry).getString().c_str() );
	                    
	                entry = name;
	                entry.append("_DBM");   
	                vWlanNodes[i].DBM = jsobj(entry).getInt();
	                
	                entry = name;
	                entry.append("_PAIRWISE");
	                vWlanNodes[i].cipherType = (enum com_WLANCipherTypes)jsobj(entry).getInt();

	                entry = name;
	                entry.append("_PERCENTAGE");    
	                vWlanNodes[i].percentage = jsobj(entry).getInt();
	                
	                entry = name;
	                entry.append("_PROTO");
	                vWlanNodes[i].protocol = (enum com_WLANProto)jsobj(entry).getInt();
	                
	                entry = name;
	                entry.append("_KEY_MGMT");
	                vWlanNodes[i].keyMgmt = (enum com_WLANKeyMgmt)jsobj(entry).getInt();
	                
	                LOGF_TRACE("[WLAN %d] \"%s\" [Protocol: %d, Key Managment: %d , %d%%, DBM: %d, Pairwise: %d]", i, 
	                	vWlanNodes[i].SSID, vWlanNodes[i].protocol, vWlanNodes[i].keyMgmt, vWlanNodes[i].percentage, 
	                		vWlanNodes[i].DBM, vWlanNodes[i].cipherType );
	            }
	        }
	    } else {
	        LOGF_TRACE("Wlan Site Survey Parsing failed");
	    }
	}
	return vWlanNodes;
}

////////////////////////////////////////////////////////////
///                 END WLAN                             ///
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
///                 BLUETOOTH                            ///
////////////////////////////////////////////////////////////

//vfiipc::JSObject inCom_BTScan(char *data, int inBuffLen)
DllSPEC int inCom_BTScan(vfiipc::JSObject& jsobj)
{	

	int inRet = 0;
	char data[5120];


	if(com_WirelessScan(COM_WIRELESS_TYPE_BT , data, sizeof(data), &gCom_errno))
	{
		LOGF_TRACE( "BT Site Survey could not be done [errno %d: %s]", (int)gCom_errno, com_GetErrorString(gCom_errno));
		inRet = 1;
	}
	else
	{
	    LOGF_TRACE( "BT Site Survey: %s", data);
	    if(!jsobj.load(std::string(data)))
	    {
	    	inRet = 1;
    	}
	}
	return inRet; 
}



DllSPEC std::vector<BT_Node> BTDivecesScan(){
	enum com_ErrorCodes com_errno;
	char data[5120] = {0};
	std::vector<BT_Node> vBTNodes;

	if(com_WirelessScan(COM_WIRELESS_TYPE_BT ,data, sizeof(data), &com_errno)){
	    LOGF_TRACE( "BT Site Survey could not be done [errno %d: %s]", (int)com_errno, com_GetErrorString(com_errno));
	} else {
	    LOGAPI_HEXDUMP_TRACE( "BT Site Survey: ", data, sizeof(data));
	    vfiipc::JSObject jsobj;
	    if(jsobj.load(std::string(data))){
	        LOGF_TRACE("BT Site Survey Parsed successfully");
	        
	        char surveyNumber[3] = {0};
	        
	        if (!jsobj.exists(COM_BT_COUNT)) {
	            LOGF_ERROR("NO BT_COUNT found");
	        } else {
	            int max = jsobj(COM_BT_COUNT).getInt();
	            LOGF_TRACE("** Found %d BT stations **", max);
	            
	            for(int i = 0 ; i<max; i++){
	            	vBTNodes.push_back(BT_Node());
	                
	                snprintf(surveyNumber,sizeof(surveyNumber), "%u", i);
	                surveyNumber[2] = '\0';
	                
	                std::string name("BT");
	                std::string entry;
	                name.append(surveyNumber);
	                
	                entry = name;
	                entry.append("_NAME");
	                
	                if (jsobj.exists(entry))
	                    strcpy( vBTNodes[i].NAME, jsobj(entry).getString().c_str() );
	                    
	                entry = name;
	                entry.append("_ADDR");   
	                strcpy(vBTNodes[i].ADDRESS, jsobj(entry).getString().c_str() );
	      
	                
	                LOGF_TRACE("[BT %d] \"%s\" [NAME: %d, ADDRESS: %s ]", i, 
	                	vBTNodes[i].NAME, vBTNodes[i].ADDRESS);
	            }
	        }
	    } else {
	        LOGF_TRACE("BT Site Survey Parsing failed");
	    }
	}
	return vBTNodes;
}

/*
int inBTDiscoverable( int inSecs)
{
	int inRes = 0;
	int inBTid = 0;
	map<string, string> lScreenValues;

	
	lScreenValues["LINE1"]="BT DISCOVERABLE";
	lScreenValues["LINE2"]="ESPERANDO SOLICITUD DE EMPAREJAMIENTO";
	lScreenValues["imgName"]="procesando.gif";


	com_NetworkSetCallback(network_event_handler, NULL, NULL);
	
	com_BTExtStopDiscovery(&gCom_errno);
	sleep(1);
	inRes = com_BTExtStartDiscovery (inSecs,&gCom_errno);
//inRes = com_SetDevicePropertyInt(COM_PROP_BT_DISCOVERABLE,1,&gCom_errno);
//	if(inRes == 0)
//		inRes = com_SetDevicePropertyInt(COM_PROP_BT_DISCOVERY_TIMEOUT ,inSecs,&gCom_errno);	

	LOGF_TRACE("ERROR - BT_DISCOVERABLE = %d  errNo = [%s]", inRes, com_GetErrorString(gCom_errno));

	inBTid = uiInvokeURLAsync(1,lScreenValues,"GenScreen.html",NULL,NULL);
	while(1)
	{
		inRes = uiInvokeWait(inBTid,50);

		if(inRes  == -27)
		{
			LOGF_TRACE("CANCELADA POR EL USAURIO");
			break;
		}
		else if(got_network_event == COM_EVENT_BT_EXT_CONFIRM)
		{
			LOGF_TRACE("COM_EVENT_BT_EXT_CONFIRM");
			//std::string sAddr = "";
			//std::string sName = "";
			//std::string sPin = "";
			//strcpy(sAddr,jsBTobj("address").getString().c_str());
			//strcpy(sName,jsBTobj("name").getString().c_str());
			//strcpy(sPin,jsBTobj("pin").getString().c_str());
		}
		else if(got_network_event == COM_EVENT_BT_EXT_VISUALIZE)
		{
			LOGF_TRACE("COM_EVENT_BT_EXT_VISUALIZE");
		}
		else if(got_network_event == COM_EVENT_BT_EXT_TIMEOUT)
		{
			LOGF_TRACE("COM_EVENT_BT_EXT_TIMEOUT");
			inRes = -3;
			break;
		}
		else if(got_network_event == COM_EVENT_BT_EXT_FAILED)
		{
			LOGF_TRACE("COM_EVENT_BT_EXT_FAILED");
			inRes = -1;
			break;
		}
		else if(got_network_event == COM_EVENT_BT_EXT_SUCCESS)
		{
			LOGF_TRACE("COM_EVENT_BT_EXT_SUCCESS");
			inRes = 0;	
			break;
		}
	}

	uiInvokeCancel(inBTid);
	com_BTExtStopDiscovery(&gCom_errno);
	return inRes;
}
*/

DllSPEC int inBTDiscoverable( int inSecs)
{
	int inRes = 0;
	int inBTid = 0;
	enum com_ErrorCodes com_errno;
	map<string, string> lScreenValues;

	com_NetworkSetCallback(network_event_handler, NULL, NULL);

	if(!isBtExtendedrSupport())
	{
		

		if(com_SetDevicePropertyInt(COM_PROP_BT_DISCOVERY_TIMEOUT, 180, &com_errno))
		{
		    LOGF_TRACE( "BT DISCOVERY TIMEOUT could not be set [errno %d: %s]", (int)com_errno, com_GetErrorString(com_errno));
		} else {
		    LOGF_TRACE( "BT DISCOVERY TIMEOUT set");
		}

		inRes = com_SetDevicePropertyInt(COM_PROP_BT_DISCOVERABLE, 1, &com_errno); 
		if(inRes)
		{
		    LOGF_TRACE( "BT DISCOVERABLE property could not be set [errno %d: %s]", (int)com_errno, com_GetErrorString(com_errno));
		} else {
		    LOGF_TRACE( "BT DISCOVERABLE: ENABLED");
		}

	}
	else
	{
		//	com_BTExtStopDiscovery(&gCom_errno);
		sleep(1);

		inRes = com_BTExtStartDiscovery (inSecs,&gCom_errno);
	}

	LOGF_TRACE("***inBTDiscoverable [%d]***",inRes);

	return inRes;
}


DllSPEC int  inSPPconfirm(int inEnable)
{
	int inRet = 0;
	enum com_ErrorCodes com_errno;

	LOGF_TRACE("COM_PROP_BT_SSP_CONFIRMATION [%d]",inEnable);
	inRet = com_SetDevicePropertyInt(COM_PROP_BT_SSP_CONFIRMATION ,inEnable, &com_errno);

	LOGF_TRACE("COM_PROP_BT_SSP_CONFIRMATION  [%d] [%d], %s",inRet ,com_errno, com_GetErrorString(com_errno));

	return inRet;
	
}



DllSPEC int  incom_SetBTSPP0(int inPortSetting)
{
	int inRes = 0;
	enum com_ErrorCodes com_errno;
	//Set SPP0 as Server
	inRes = com_SetBTSPPPort(COM_BT_CONFIG_SPP0, inPortSetting, &com_errno);	
	LOGF_TRACE("incom_SetBTSPP0 failed with com_errno [%d] [%d], %s",inRes ,com_errno, com_GetErrorString(com_errno));
	return inRes; 
}

DllSPEC int  incom_SetBTSPP2(int inPortSetting)
{
	int inRes = 0;
	enum com_ErrorCodes com_errno;
	//Set SPP0 as Server
	inRes = com_SetBTSPPPort(COM_BT_CONFIG_SPP2, inPortSetting, &com_errno);	
	LOGF_TRACE("incom_SetBTSPP2 failed with com_errno [%d], %s", com_errno, com_GetErrorString(com_errno));
	return inRes;
}


DllSPEC int inWaitBluetoothEvts()
{

	int inEvnt = 0;

	inEvnt =(int) got_network_event;

	LOGF_TRACE("**** BLUETOOTH EVENTS [%d] ****",inEvnt);

	if(got_network_event == COM_EVENT_BT_CONFIRM_PAIR_REQUEST )
	{
		LOGF_TRACE("COM_EVENT_BT_CONFIRM_PAIR_REQUEST");
	}
	else if(got_network_event == COM_EVENT_BT_PAIR_DONE)
	{
		LOGF_TRACE("COM_EVENT_BT_PAIR_DONE");
	}
	else if(got_network_event == COM_EVENT_BT_DISCOV_TO_ELAPSED )
	{
		LOGF_TRACE("COM_EVENT_BT_DISCOV_TO_ELAPSED");
	}

	/////////////////////////////////////////////////////
	//           BLUETOOTH EXTENDEN EVENTS
	/////////////////////////////////////////////////////

	else if(got_network_event == COM_EVENT_BT_EXT_CONFIRM)
	{
		LOGF_TRACE("COM_EVENT_BT_EXT_CONFIRM");
	}
	else if(got_network_event == COM_EVENT_BT_EXT_VISUALIZE)
	{
		LOGF_TRACE("COM_EVENT_BT_EXT_VISUALIZE");
	}
	else if(got_network_event == COM_EVENT_BT_EXT_TIMEOUT)
	{
		LOGF_TRACE("COM_EVENT_BT_EXT_TIMEOUT");
	}
	else if(got_network_event == COM_EVENT_BT_EXT_FAILED)
	{
		LOGF_TRACE("COM_EVENT_BT_EXT_FAILED");
	}
	else if(got_network_event == COM_EVENT_BT_EXT_SUCCESS)
	{
		LOGF_TRACE("COM_EVENT_BT_EXT_SUCCESS");			
	}
	else
		inEvnt = 0;

	return inEvnt;
}

DllSPEC void  vdBTStopDiscoverable()
{
	com_BTExtStopDiscovery(&gCom_errno);
}

DllSPEC void vdFlushBTevts()
{

	got_network_event = 0;	
}

/**
 * @brief      inGetBTFriendlyName Only works afeter invoke inWaitBluetoothEvts() and you got COM_EVENT_BT_EXT_CONFIRM | COM_EVENT_BT_EXT_VISUALIZE events
 *
 * @param      chName  Friendly Name returned by the device
 *
 * @return     return SUCCESS (0) FAIL(1)
 */
DllSPEC int inGetBTFriendlyName(char *chName)
{
	int inRes = 0;
	strcpy(chName,jsBTobj("name").getString().c_str());
	if (strlen(chName) <= 0 )
		inRes = 1;

	return inRes;
}

/**
 * @brief      inGetBTAddress Only works afeter invoke inWaitBluetoothEvts() and you got COM_EVENT_BT_EXT_CONFIRM | COM_EVENT_BT_EXT_VISUALIZE events
 *
 * @param      chName  Mac Address returned by the device
 *
 * @return     return SUCCESS (0) FAIL(1)
 */
DllSPEC int inGetBTAddress(char *chAddr)
{
	int inRes = 0;
	strcpy(chAddr,jsBTobj("address").getString().c_str());
	if (strlen(chAddr) <= 0 )
		inRes = 1;

	return inRes;
}

/**
 * @brief      inGetBTPin Only works afeter invoke inWaitBluetoothEvts() and you got COM_EVENT_BT_EXT_CONFIRM | COM_EVENT_BT_EXT_VISUALIZE events
 *
 * @param      chName  comparison pin returne by the device
 *
 * @return     return SUCCESS (0) FAIL(1)
 */
DllSPEC int inGetBTPin(char *chPin)
{
	int inRes = 0;
	strcpy(chPin,jsBTobj("pin").getString().c_str());
	if (strlen(chPin) <= 0 )
		inRes = 1;

	return inRes;
}


/**
 * @brief      inBTConfirm
 *
 * @param[in]  inyesOrNo  Parameter should be 0 in case the user has not accepted, 1 if the user has accepted
 *
 * @return    returns 0 if ok, -1 in case of error. A return code of 0 does not indicate that pairing is successful, only that the confrimation on this side has been accepted
 */
DllSPEC int inBTConfirm(int inyesOrNo)
{

	int inRet = 0;
	enum com_ErrorCodes com_errno;

	LOGF_TRACE("inBTConfirm [%d]", inyesOrNo);

	if(isBtExtendedrSupport())
	{
		LOGF_TRACE("*** EXTENDED PAIRING  ***");
		inRet = com_BTExtConfirm(inyesOrNo, &com_errno);
	}
	else
	{
		LOGF_TRACE("*** NORMAL PAIRING ***");
		inRet = com_SetDevicePropertyInt(COM_PROP_BT_CONFIRM_PAIR ,inyesOrNo, &com_errno);
	}

	LOGF_TRACE("inBTConfirm Result  [%d] errNo [%d] errDesc = %s", inRet, com_errno, com_GetErrorString(com_errno));

	return inRet;
}


DllSPEC std::vector<BT_Node> inGetBTPairedDevices()
{

	int inRet = 0;
	int max = 0;
	char chJsonRes[1024] = {0};
	vfiipc::JSObject jsBTDev;
	std::vector<BT_Node> vBTNodes;

	inRet = com_GetDevicePropertyString(COM_PROP_BT_PAIRING_STATUS, chJsonRes, sizeof(chJsonRes), &gCom_errno);
	if(inRet == 0 )
	{
		jsBTDev.clear();
		if(jsBTDev.load(std::string(chJsonRes)))
		{
			char surveyNumber[3] = {0};
			
			LOGF_TRACE("json loaded successfully");
			LOGF_TRACE("COM_PROP_BT_PAIRING_STATUS [%d] jsonRes [%s] Err [%s]",inRet,chJsonRes,com_GetErrorString(gCom_errno));

			max = jsBTDev(COM_BT_COUNT).getInt();

			LOGF_TRACE("COM_BT_COUNT [%d]", max);

			if ( max > 0)
			{
		        LOGF_ERROR("BT PAIRED DEVICES");

		        for(int i = 0 ; i<max; i++)
		        {
		        	vBTNodes.push_back(BT_Node());

					snprintf(surveyNumber,sizeof(surveyNumber), "%u", i);
					surveyNumber[2] = '\0';

					std::string name("BT");
					std::string entry;
					name.append(surveyNumber);

					entry = name;
					entry.append("_NAME");

					if (jsBTDev.exists(entry))
					{
						strcpy( vBTNodes[i].NAME, jsBTDev(entry).getString().c_str());

						entry = name;
		                entry.append("_ADDR");   
		           		strcpy( vBTNodes[i].ADDRESS, jsBTDev(entry).getString().c_str() );
					}

		           	LOGF_TRACE("%d.- BT_NAME [%s] MAC ADDR [%s]",i,vBTNodes[i].NAME, vBTNodes[i].ADDRESS);
		        }

		    }
		}
	}
	
	return vBTNodes;
}



DllSPEC int inBTunPair(char *BTaddr)
{
	int inRet = 0;
	enum com_ErrorCodes com_errno;

	LOGF_TRACE("UNPAIR [%s]", BTaddr);
	inRet = com_BTUnPair(BTaddr,&com_errno);
	LOGF_TRACE("UNPAIR RES [%d]", inRet);

	return inRet;
}

 
////////////////////////////////////////////////////////////
///                 END BLUETOOTH                        ///
////////////////////////////////////////////////////////////

DllSPEC int inCheckConnectionState()
{
	enum com_ConnectionState  comConnectionState = COM_CON_STATE_ACTIVE  ;
	comConnectionState = com_GetConnectionState (Handle);
	LOGF_TRACE("CONNECTION STATE [%d]",comConnectionState);

	switch (comConnectionState)
	{

		case COM_CON_STATE_ACTIVE:
			LOGF_TRACE("The remote is considered to be active ");
		break;
		
		case COM_CON_STATE_PARAM_INVALID:
			LOGF_TRACE("Handle passed by application is invalid");
		break;

		case COM_CON_STATE_PARAM_NO_CON:
			LOGF_TRACE("Handle passed by application is not connection-oriented");
		break;

		case COM_CON_STATE_RESET:
			LOGF_TRACE("The remote has reset the connection. The connection was not properly shut down. Pass handle to com_ConnectClose");
		break;

		case COM_CON_STATE_REMOTE_CLOSING:
			LOGF_TRACE("The remote is waiting for the connection to close. Pass handle to com_ConnectClose to properly shut down ");
		break;

		case COM_CON_STATE_TIMEDOUT:
			LOGF_TRACE("The other end didn't acknowledge retransmitted data after some time ");
		break;
	}

	
	return (int)comConnectionState ;
}


DllSPEC std::string retrieveIP_Addr(char* chProfile) {
	struct com_IPConfig com_config;
	int result;
	enum com_ErrorCodes errNo;
	char chprof[100] = {0}; 

	memset(&com_config, 0, sizeof(com_config));

	#if (defined _VRXEVO)
	sprintf(chprof, "%s",chProfile);
	#else
	sprintf(chprof, "./flash/net/%s",chProfile);
	#endif
	result = com_GetNetworkInfo(chprof, &com_config, &gCom_errno);
	if(result!=0) {
		LOGF_TRACE("ERROR - com_GetNetworkInfo = %d  errNo = %s", result, com_GetErrorString(gCom_errno));
		return "0.0.0.0";
	}
	return com_config.ip_addr;
}

DllSPEC bool isDhcp(char* chProfile)
{
	struct com_IPConfig com_config;
	int result;
	enum com_ErrorCodes errNo;
	char chprof[100] = {0}; 

	memset(&com_config, 0, sizeof(com_config));

	#if (defined _VRXEVO)
	sprintf(chprof, "%s",chProfile);
	#else
	sprintf(chprof, "./flash/net/%s",chProfile);
	#endif
	result = com_GetNetworkInfo(chprof, &com_config, &gCom_errno);
	if(result!=0) {
		LOGF_TRACE("ERROR - com_GetNetworkInfo = %d  errNo = %s", result, com_GetErrorString(gCom_errno));
		return false;
	}
	return (bool)com_config.dhcp;
}

DllSPEC void vdSetConnectionProfile(string sConnectionProfile)
{

	sCONNECTIONPROFILE = sConnectionProfile; 
	LOGF_TRACE("NEW CONNECTION PROFILE [%s]",sCONNECTIONPROFILE.c_str());

}

DllSPEC string sGetConnectionProfile()
{
	LOGF_TRACE("ACTUAL CONNECTION PROFILE [%s]",sCONNECTIONPROFILE.c_str());
	return sCONNECTIONPROFILE; 
}


DllSPEC bool bIsUSBRawSerealTerminal()
{
	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	if((inFeatures & COM_FEATURE1_SERIAL_USBD ) || (inFeatures & COM_FEATURE1_USBSER))
		bResult= true;


	return bResult;
}


DllSPEC bool bIsCom1RawSerialTerminal()
{
	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	//if((inFeatures & COM_FEATURE1_SERIAL_1) || (inFeatures & COM_FEATURE1_SERIAL_2))
	if((inFeatures & COM_FEATURE1_SERIAL_1))
		bResult= true;


	return bResult;
}

DllSPEC bool bIsCom2RawSerialTerminal()
{
	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	if((inFeatures & COM_FEATURE1_SERIAL_2))
		bResult= true;


	return bResult;
}

DllSPEC bool bIsBtTerminal()
{
	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	if(inFeatures & COM_FEATURE1_BLUETOOTH )
		bResult= true;


	return bResult;
}


DllSPEC bool isBtExtendedrSupport()
{
	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_2,&inFeatures,NULL);
	if(inFeatures & COM_FEATURE2_BT_PAIR_EXT )
		bResult= true;

	return bResult;

}

DllSPEC bool bIsWifiTerminal()
{

	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	if(inFeatures & COM_FEATURE1_WIFI )
		bResult= true;

	return bResult; 
}


DllSPEC bool isEthTerminal()
{

	int inFeatures = 0;
	bool bResult = false;

	com_GetDevicePropertyInt(COM_PROP_SUPPORTED_FEATURES_1,&inFeatures,NULL);
	if(inFeatures & COM_FEATURE1_LAN_1 )
		bResult= true;

	return bResult; 
}



DllSPEC int inGetSSID(char *chSSID, int inSize)
{

	int inRet = 0;
	inRet = com_GetDevicePropertyString(COM_PROP_WLAN_SSID, chSSID, inSize, &gCom_errno);

	LOGF_TRACE("COM_PROP_WLAN_SSID [%d] SSID [%s] Err [%s]",inRet,chSSID,com_GetErrorString(gCom_errno));
	return inRet;	
}


DllSPEC void vdSetPSK(char *chPSK)
{
	int inRet = 0;

	LOGF_TRACE ("*** vdSetPSK ***");

	inRet = _putEnvFile_((char *)"wifi",(char*)"PSK", chPSK);

	LOGF_TRACE ("vdSetPSK Res [%d]",inRet);
}

DllSPEC int inGetPSK(char *chPSK)
{


	int inRet = 0;
	char chTemp[30] = {0};
	
	inRet = _getEnvFile_((char *)"wifi",(char*)"PSK", chTemp, sizeof(chTemp));

	if(inRet > 0)
		strcat(chPSK,chTemp);
	else
		inRet = 1;


	return inRet;	
}


DllSPEC void vdSetWlanKeyMgmt(int inTypeKey)
{
 	int inRet = 0;

 	LOGF_TRACE ("*** vdSetWlanKeyMgmt [%d] ***",inTypeKey);

	if(inTypeKey == COM_WLAN_KEY_MGMT_PSK)
		inRet = _putEnvFile_((char *)"wifi",(char*)"KEYMGMT", (char*)"PSK");
	else if(inTypeKey == COM_WLAN_KEY_MGMT_EAP)
		inRet = _putEnvFile_((char *)"wifi",(char*)"KEYMGMT", (char*)"EAP");
	else
		inRet = _putEnvFile_((char *)"wifi",(char*)"KEYMGMT", (char*)"NONE");

	LOGF_TRACE ("vdSetWlanKeyMgmt Res [%d]",inRet);

}

DllSPEC int inGetWlanKeyMgmt(int *inTypeKey)
{

	int inRet = 0;
	char chTemp[30] = {0};

	LOGF_TRACE("inGetWlanKeyMgmt");
	inRet = _getEnvFile_((char *)"wifi",(char*)"KEYMGMT", chTemp, sizeof(chTemp));

	LOGF_TRACE("inGetWlanKeyMgmt [%s]",chTemp);
	if(inRet > 0)
	{
		if(!strcmp(chTemp,(char*)"PSK"))
			*inTypeKey = COM_WLAN_KEY_MGMT_PSK ;
		else if(!strcmp(chTemp,(char*)"EAP"))
			*inTypeKey = COM_WLAN_KEY_MGMT_EAP ;
		else 
			*inTypeKey = COM_WLAN_KEY_MGMT_NONE ;

	}
	else
		inRet = 1;

	LOGF_TRACE("inGetWlanKeyMgmt KEY [%d] ret [%d]", *inTypeKey,inRet);

	return inRet;
}




DllSPEC int inGetNetworkStatus(char *netprofile)
{

	char netprof [100] = {0};
	com_NetworkStatus netStatus = COM_NETWORK_STATUS_ERROR; 

	#if (defined _VRXEVO)
	sprintf(netprof, "I:1/%s",netprofile);
	#else
	sprintf(netprof, "./flash/net/%s",netprofile);
	#endif
	netStatus = com_GetNetworkStatus (netprof,&gCom_errno);

	LOGF_TRACE("NETWORK STATUS [%d] - [%s]",netStatus,com_GetErrorString(gCom_errno));

	return  (int) netStatus;
}


DllSPEC void  vdSetWlanNetStatus(enum WLANSATUS wLanstatus)
{

	inWlanStatus = wLanstatus;
}

DllSPEC int inGetWlanNetStatus(void)
{

	return inWlanStatus;
}



int _getEnvFile_(char *chSection,char *chVar, char *chValue, int inLen )
{
	int inRet = 0;
	
#ifdef _VRXEVO
	 inRet = get_env(chVar, chValue, inLen);		
#else
	inRet = getEnvFile((char *)chSection, (char *)chVar, chValue,inLen);
#endif	
	return inRet;
}

int _putEnvFile_(char *chSection,char *chVar, char *chValue)
{
	int inRet = 0;
	
#ifdef _VRXEVO
	
	inRet = put_env(chVar, chValue, strlen(chValue));	 
#else
	inRet = putEnvFile((char *)chSection,(char *)chVar,chValue);
#endif	
	return inRet;
}


DllSPEC int inGetStatusConnection()
{

	LOGF_TRACE("*** inGetStatusConnection [%d]***",inStatusConnect);
	return inStatusConnect;
}