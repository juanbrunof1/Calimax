#include "com/libcom.h"
#include "ipc/jsobject.h"
#include <string>
#include <stdio.h>


#if (defined _VRXEVO)
	#if (defined RDCCOM_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif


//Connection status
#define CONNECTED 1
#define CONNECTING 0
#define NOT_CONNECTED -1

//DEFUAL CONNECTION PROFILE NAME'S
#define WLAN_COM_NETWORK_FILENAME "WLAN_NET.xml"
#define ETH_COM_NETWORK_FILENAME "ETH_NET.xml"

#if (defined _VRXEVO)
#define PATH_COM_NETWORK_FILENAME "I:1/"
#else
#define PATH_COM_NETWORK_FILENAME "./flash/net/"
#endif

typedef struct 
{
	char 	SSID[65];  /*!< SSID name */
	enum com_WLANProto	 	protocol; /*!< Authentication protocol */
	enum com_WLANKeyMgmt 	keyMgmt; /*!< Key management protocol */
	enum com_WLANCipherTypes	cipherType; /*!< Cipher scheme */
	int 	percentage; /*!< Signal intensity */
	char	PSK[65];    /*!< Pre-shared key */
	enum com_WLANEAPType	EAPType; /*!< EAP type */
	char 	identity[65]; /*!< Identity in case of PAP authentication */
	char	password[65]; /*!< Password in case of PAP authentication */
	int 	DBM; /*!< Signal strength in dBM */
} WLAN_Node;

typedef struct 
{
	char 	NAME[60];  /*!< SSID name */
	char    ADDRESS[60];
} BT_Node;


enum WLANSATUS
{
	
	WLAN_NET_LINK_DOWN	= 0,
	WLAN_NET_LINK_UP    = 1,

};

extern int isBTConnected; 

DllSPEC int inInitComAdk();
//void vdDestroyComAdk(void);  // FAG 29-dic-2017
DllSPEC int inDestroyComAdk(void); // FAG 29-dic-2017
//int inComConnectAdk(const char *chProfile,int inTimeOut,int inTry);
DllSPEC int inComConnectAdk(int inTimeOut,int inTry);
DllSPEC int inComConnectAsyncAdk(int inTry);
DllSPEC int inComConnectWait(int inTO);
DllSPEC int inSendAdk(char *chSendBuffer, int inSendLen);
DllSPEC int inRcvAdk(char *chRcvBuffer,int inMaxRcv,int inTimeO);
DllSPEC void connection_callback (enum com_ConnectionEvent event, enum com_ConnectionType type, const void *data, void *priv, enum com_ErrorCodes com_errno);

DllSPEC int netDetach(char *netprofile);
DllSPEC int netRestart(char *netprofile);

DllSPEC int check_USB();  // FAG 25-AGO-2017
DllSPEC int inUSB_LINK();    // FAG 31-AGO-2017
DllSPEC int inEthLinkStatus(); //JRS 04/09/2017
DllSPEC void vdConnectCloseAdk (); //JRS 05/09/2017

DllSPEC int netAttach(char *netprofile); //JRS 28/09/2017
DllSPEC int netDetach(char *netprofile); //JRS 28/09/2017

DllSPEC int inCreateConnectionProfile(std::string sCONPROF, std::string sTYPE,std::string sNETPROF, std::string sURL, std::string sPORT,std::string sServer, std::string sTO ); //JRS 18/05/2018

DllSPEC int inDoSerialSendRecv(char *chSendBuffer,int inSendLen ,char *chRcvBuffer,int inRecvLen);
//BLUETOOTH
//JRS 18/05/18
DllSPEC int  inSPPconfirm(int inEnable);
DllSPEC int incom_SetBTSPP0(int inPortSetting);
DllSPEC int incom_SetBTSPP2(int inPortSetting);
DllSPEC int inCom_BTScan(vfiipc::JSObject& jsobj);
DllSPEC int inBTDiscoverable( int inSecs);
DllSPEC std::vector<BT_Node> BTDivecesScan();
DllSPEC int inWaitBluetoothEvts();
DllSPEC int inGetBTPin(char *chPin);
DllSPEC int inGetBTAddress(char *chAddr);
DllSPEC int inGetBTFriendlyName(char *chName);
DllSPEC void  vdBTStopDiscoverable();
DllSPEC int inBTConfirm(int inyesOrNo);
DllSPEC void vdFlushBTevts();
DllSPEC std::vector<BT_Node> inGetBTPairedDevices();
DllSPEC int inBTunPair(char *BTaddr);
//WIFI
//JRS 18/05/18
//int inCreateWlanNetworkProfile(std::string sSSID, std::string sPSK);
//DllSPEC int inCreateWlanNetworkProfile(std::string sPROFNAME, std::string sSSID, std::string sPSK, std::string sStartMode,std::string sDhcp, std::string sTO);
DllSPEC int inCreateWlanNetworkProfile(std::string sPROFNAME, std::string sSSID, std::string sKeyMgmt, std::string sPSK, std::string sStartMode,std::string sDhcp,std::string sIpAddr,std::string sNetMask,std::string sGateWay,std::string sDns1,std::string sDns2, std::string sTO);
DllSPEC int inCreateEthNetworkProfile(std::string sPROFNAME, std::string sStartMode,std::string sDhcp,std::string sIpAddr,std::string sNetMask,std::string sGateWay,std::string sDns1,std::string sDns2, std::string sTO);
DllSPEC int inCom_WlanScan(vfiipc::JSObject& jsobj);
DllSPEC std::vector<WLAN_Node> wirelessNetworkScan(void);
DllSPEC int inGetSSID(char *chSSID, int inSize);
DllSPEC void vdSetPSK(char *chPSK);
DllSPEC int inGetPSK(char *chPSK);
//DllSPEC int inWlanKeyMgmt(int *inTypeKey);
DllSPEC void vdSetWlanKeyMgmt(int inTypeKey);
DllSPEC int inGetWlanKeyMgmt(int *inTypeKey);
DllSPEC void vdSetWlanNetStatus(enum WLANSATUS wLanstatus);
DllSPEC int inGetWlanNetStatus(void);
DllSPEC void  vdSetWlanNetStatus(enum WLANSATUS wLanstatus);

DllSPEC int inCheckConnectionState();
DllSPEC std::string retrieveIP_Addr(char* chProfile);
DllSPEC bool isDhcp(char* chProfile);
DllSPEC void vdSetConnectionProfile(std::string sConnectionProfile);
DllSPEC std::string sGetConnectionProfile();

DllSPEC bool bIsWifiTerminal();
DllSPEC bool isEthTerminal();
DllSPEC bool bIsBtTerminal();
DllSPEC bool isBtExtendedrSupport();
DllSPEC bool bIsCom1RawSerialTerminal();
DllSPEC bool bIsCom2RawSerialTerminal();
DllSPEC bool bIsUSBRawSerealTerminal();
DllSPEC int inGetNetworkStatus(char *netprofile);
DllSPEC int inGetStatusConnection();

