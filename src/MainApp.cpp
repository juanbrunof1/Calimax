//SYSTEM INCLUDES

//#include <string>
#include <stdio.h>
#include <malloc.h>
//#include <cstring>
#include <stdlib.h>
#include <time.h>
//#include <list>
#include <unistd.h>
#include <svc.h>

#include <platforminfo_api.h>


//USER INCLUDES
#include "CapX/debug.h"
#include "CapX/usrcmds.h"
#include "App/FlowPinPad.h"
#include "EmvRdc/VosEMVProcess.h"
#include "PinPad/PP_Cmd.h"
//ADK INCLUDES
#include "html/gui.h"
#include "ipc/jsobject.h"
#include "html/jsobject.h"
#include "com/libcom.h"
#include <expat/expat.h>
#include <log/liblog.h>
#include <sysinfo/sysinfo.h>
#include <sysinfo/sysbar.h>
#include <sysinfo/sysbeep.h>
#include <sys/time.h>
#include <tec/tec.h>
#include <emv/EMV_CT_Interface.h>     // FAG 05-ABRIL02017
#include <emv/EMV_CTLS_Interface.h> // FAG 05-ABRIL02017 


using namespace std;
using namespace vfigui;
using namespace vfisysinfo;

#define FALSE 0
#define TRUE  1


void vdStatusRegion(bool show_status_bar);
static void statusbar_callback(void *data, int region_id, map<string,string> &values);
void ShowSplashScreen(const char* msg, const char* progress);
std::string getversionapp(void);
std::string getversionkernel(void);
void getapptype(void);
bool isColorDisplayCapable();
int CTLS_ON(void);		//Daee 18/11/2015
//Daee 07/12/2017
std::string emvADKTextDisplayCallback(int error);
void vdSetTimeStamp ();		//Daee 27/02/2018
void setLedConfig ( );      //Daee  14/06/2018

//#define LOG_BANORTE  //cOMMENT FOR PRODUCTION
#define LOG_ENABLE  //cOMMENT FOR PRODUCTION


typedef std::map<std::string, std::string> UIParams;

int main(int argc, char *argv[])
{
	int interactionResult;
 	int font_size;
	int inRes = 0;
	char chSendbuff[100] = {0};
	char chRcvbuff[100] = {0};
	//char *szTmp = (char *)malloc( sizeof(char) * 100 );
	int h = 0;
	int w = 0;
	int erg = 0;
	
	UIParams uivalues;
 
	map<string,string> value;
	map<string,string> values;

#define LOG_INFO_NAME "CALIMAX"

#ifdef LOG_ENABLE
	LOGAPI_SET_VERBOSITY(LOGAPI_VERB_PID | LOGAPI_VERB_FILE_LINE);
//	LOGAPI_INIT("BANORTE");
	LOGAPI_INIT(LOG_INFO_NAME);
	LOGAPI_SETLEVEL(LOGAPI_TRACE);
	LOGAPI_SETOUTPUT(LOGAPI_ALL);
	const char *verLog = log_getVersion();
	const char *verComm = com_GetVersion();
	LOGF_INFO("Log Version: %s", verLog);
	LOGF_INFO("Comm Version: %s", verComm);
	LOGF_INFO("Start");

   	LOGAPI_DUMP_SYS_INFO(); 
#endif
		
	//LOGF_TRACE((char*)"--BANORTE main--");
	LOGF_TRACE((char*)"--main--");

/*
	// es para crear regiones
	LOGF_TRACE(" creamos la estructura");
	const struct UIRegion TotalScreen[]={
										{UI_REGION_DEFAULT, 0,0,-1,-1}  //Using whole screen without status bars.
																	  //This is an enumerated data type add as many elements as necessary
   									};								 //x1,x2 y1,y2

	LOGF_TRACE("invocamos uiLayout");

	uiLayout(TotalScreen,sizeof(TotalScreen)/sizeof(TotalScreen[0])); // crea las regiones
	

*/		
	//sysStartStatusbarURL(1,"statusbar.html",values,statusbar_callback);	
	
	uiGetPropertyInt(UI_DEVICE_WIDTH,&w );
	uiGetPropertyInt(UI_DEVICE_HEIGHT,&h );
	LOGF_TRACE("UI_DEVICE_WIDTH [%d]",w);
	LOGF_TRACE("UI_DEVICE_HEIGHT [%d]",h);

	ShowSplashScreen("Iniciando Emv.." , "20");
	usleep(400000);
		
//	erg = Init_EMV(EMV_ADK_TT_ATTENDED_ONL_ONLY);		
	erg = Init_EMV(EMV_ADK_TT_UNATTENDED_ONL_ONLY);	
	if(erg != EMV_ADK_OK)
		{
			LOGF_TRACE("\nEMV CHIP Error INIT");
			LOGF_TRACE("\nError:%i", erg);
			return 0;
		}
		else
			LOGF_TRACE("\nEMV CT started succesfully\n");


	if(CTLS_ON())		//Daee 08/01/2018
	{
		ShowSplashScreen("Iniciando Ctls.." , "40");
		usleep(400000);

		erg = Init_EMV_CTLS(EMV_ADK_TT_ATTENDED_ONL_ONLY);	
	//	erg = Init_EMV_CTLS(EMV_ADK_TT_UNATTENDED_ONL_ONLY);

		if(erg != EMV_ADK_OK)
		{
			LOGF_TRACE("\nEMV CTLS Error INIT");
			LOGF_TRACE("\nError:%i", erg);
			return 0;
		}
		else
			LOGF_TRACE("\nEMV CTLS started succesfully\n");

		
		ShowSplashScreen("Iniciando Config.." , "60");
		usleep(400000);

		if (ApplyCTLSConfiguration()!= EMV_ADK_OK)
		{ // Inject configuration to the reader
			
			LOGF_TRACE("\nApplyCTLSConfiguration unsuccesfull\n");
			EMV_CTLS_Exit_Framework();
			return 0;
		}
		else 
			LOGF_TRACE("\nApplyCTLSConfiguration succesfull\n");

		boCtlsON = true;
		setDisplayGetText_CB (&emvADKTextDisplayCallback);  //Daee 07/12/2017
		setLedConfig ( );
	}

	ShowSplashScreen(" Iniciando Config.." , "80");
	usleep(400000);
	erg = inPARSE_XMLCMDInit ();

	if (erg != XML_CMD_OK)
	{ // Inject configuration to the reader
		
		LOGF_TRACE("\ninPARSE_XMLCMDInit unsuccesfull\n");
		EMV_CTLS_Exit_Framework();
		return 0;
	}
	else 
		LOGF_TRACE("\ninPARSE_XMLCMDInit succesfull\n");

	erg = 0;
	if(erg == 0)
	{
		//vdStatusRegion();

		
		ShowSplashScreen("Iniciando.." , "100");
		usleep(400000);
		// This is to read the GUI system Configuration File GUI.ini
		uiReadConfig();
		uiGetPropertyInt(UI_PROP_DEFAULT_FONT_SIZE,&font_size);
		
		//string htmlFile = string("Welcome.html");
		// uiInvokeURL(0, values, "splash_intro.html");
		// sleep(2);

		// TODO: Verificar Requerido
		/*if(isColorDisplayCapable())
		 	uiInvokeURL(0, values, "video.html"); */
	//	 sleep(3);

	/*
		do
		{
			while ((interactionResult = uiInvokeURL(0, uivalues, htmlFile)) == UI_ERR_PERMISSION)
			{
			LOGF_TRACE(" welcome");
			  usleep(10000);
			}
			
			if(interactionResult == 132)
			{
				LOGF_TRACE(" welcome 1 ");
				htmlFile = string("Welcome1.html");
			} 
			else if(interactionResult == 129)
			{
				LOGF_TRACE(" welcome");
				htmlFile = string("Welcome.html");
			} 
			else if(interactionResult == 101)
			{
				LOGF_TRACE(" result 101 ");
				break;
			} 
			else if(interactionResult == 102)
			{
				LOGF_TRACE(" result 102");
				break;
			}
			
		}while(1);

		*/		
		

		//uiInvokeURL(1,value,"IdlePP.html");
		//uiInvokeURL(value,"IdlePP.html");

		vdSetTimeStamp();
		
		vdStatusRegion(true);
	
		value.clear();
		string namePinpad;//MAR 31/01/2019
	    sysGetPropertyString(SYS_PROP_HW_MODEL_NAME,namePinpad);//MAR 31/01/2019
	    value["HEADER_TITLE"]=namePinpad;//MAR 31/01/2019
	//	value["HEADER_TITLE"]="Verifone PX00";
//		value["BODY_HEADER"]="Banorte PinPad";
		value["BODY_HEADER"]="PinPad";
		value["BODY_MSG1"]=getversionkernel();
		value["BODY_MSG2"]=namePinpad+"CTV1.0.0_"+getversionapp();
		
		uiInvokeURL(value, "Welcome.html");
		sleep(SHOW_ALERT);	
		
		sysSetPropertyInt(SYS_PROP_KEYBOARD_BEEP,1); // FAG 03-ABRIL-2017
		getapptype();
		vdSetBanco(3);

		vdCmdFlow();
	}
	else
	{
		
	}

	return 0;
}

void vdStatusRegion(bool show_status_bar)
{
	std::map<std::string,std::string> values;
	std::string curLayout;
	int retval;

	LOGF_TRACE("vdStatusRegion");

	if(show_status_bar)
		curLayout = "layout-status";
	else
		curLayout = "layout";
	
	retval = vfigui::uiLayout(curLayout.c_str());

	LOGF_TRACE("retval = %d ",retval);

	LOGF_TRACE("sysStartStatusbarURL");

    retval = sysStartStatusbarURL(1,"statusbar.html",values,statusbar_callback);
	
	LOGF_TRACE("retval = %d ",retval);
    
  
}


static void statusbar_callback(void *data, int region_id, map<string,string> &values)
{
	LOGF_TRACE("statusbar_callback");

  // region ID is useful, if application uses one callback for multiple statusbars
  if(region_id==0) {
    static int signal_level=0;
    static int battery_level=11;
    static bool fDocked=FALSE;
    char buf[16];
    //sprintf(buf,"%d",signal_level);
    //values["my_sys_mob_netw_signal_level"]=string(buf);
    //values["my_sys_mob_netw_type"]="PINPAD";
    //signal_level=((signal_level+1)%6); // 0..5, update level value for next refresh

    if (0)
    {
    	//This section is intended only for demo purposes.
    	//On normal operation, sys_battery* built in system variables are used.
    	//No need to write code for it.
    	sprintf(buf,"%d",battery_level);
        values["my_sys_battery_level"]=string(buf);
        switch (battery_level)
        {
        	case 0:
        	case 100:
        		values["my_sys_battery_percentage"]=1;
        		values["my_sys_battery_percentage_2"]="1%";
        		break;
        	case 1:
        	case 101:
        		values["my_sys_battery_percentage"]=5;
        		values["my_sys_battery_percentage_2"]="5%";
        		break;
        	case 2:
        	case 102:
        		values["my_sys_battery_percentage"]=10;
        		values["my_sys_battery_percentage_2"]="10%";
        		break;
        	case 3:
        	case 103:
        		values["my_sys_battery_percentage"]=20;
        		values["my_sys_battery_percentage_2"]="20%";
        		break;
        	case 4:
        	case 104:
        		values["my_sys_battery_percentage"]=30;
        		values["my_sys_battery_percentage_2"]="30%";
        		break;
        	case 5:
        	case 105:
        		values["my_sys_battery_percentage"]=40;
        		values["my_sys_battery_percentage_2"]="40%";
        		break;
        	case 6:
        	case 106:
        		values["my_sys_battery_percentage"]=50;
        		values["my_sys_battery_percentage_2"]="50%";
        		break;
        	case 7:
        	case 107:
        		values["my_sys_battery_percentage"]=60;
        		values["my_sys_battery_percentage_2"]="60%";
        		break;
        	case 8:
        	case 108:
        		values["my_sys_battery_percentage"]=70;
        		values["my_sys_battery_percentage_2"]="70%";
        		break;
        	case 9:
        	case 109:
        		values["my_sys_battery_percentage"]=80;
        		values["my_sys_battery_percentage_2"]="80%";
        		break;
        	case 10:
        	case 110:
        		values["my_sys_battery_percentage"]=90;
        		values["my_sys_battery_percentage_2"]="90%";
        		break;
        	default: // 11 / 111 or 100%
        		values["my_sys_battery_percentage"]=100;
        		values["my_sys_battery_percentage_2"]="100%";
        		break;
        }
        if(battery_level==0 && !fDocked)
        {
        	fDocked = TRUE;
        	battery_level=100;
        }
        if(battery_level>=111 && fDocked)
        {
        	fDocked = FALSE;
        	battery_level=11;
        }
        if(fDocked)
        	battery_level=((battery_level+1)%112); // 0..111, update level value for next refresh
        else
        	battery_level=((battery_level-1)%12); // 11..0, update level value for next refresh
    }
	
  }
  
}


void ShowSplashScreen(const char* msg, const char* progress)
{
  UIParams values;
  char tmp[35+1]={0};
  int inRet=0;

  string release_version="";
  values["body_header"] = string(msg);
  values["splash_progress"] =  string(progress); 
  
  
  inRet = getEnvFile((char *)"application", (char *)"BUNDLEVER", tmp,sizeof(tmp));
  if(inRet != 0)
  	values["version"] =  string(tmp);	
  else
  	values["version"] =  "1.x.x";	

  while (uiInvokeURL(values, "SplashView.html") == UI_ERR_PERMISSION)
  {
    usleep(10000);
  }
}

std::string getversionapp(void)
{
 	std::string version="";
	int inRet=0;
	char tmp[35+1]={0};
	char AppVer[64]={0};
	

	inRet = getEnvFile((char *)"application", (char *)"VERSION", tmp,sizeof(tmp));
	if (inRet == 0)
	{
		strcpy(tmp, "UNDEFINED");
	}
	
	else
	{
		strcpy(AppVer,tmp);
		memset(tmp, 0x00, sizeof(tmp));

		inRet = getEnvFile((char *)"application", (char *)"BUNDLEVER", tmp,sizeof(tmp));
		strcat(AppVer,tmp);


		memset(tmp, 0x00, sizeof(tmp));
		inRet = getEnvFile((char *)"application", (char *)"TIMESTAMP", tmp,sizeof(tmp));
		
		if (inRet != 0)
		{
			strcat(AppVer,tmp);
		}
	}
	version.insert(0,tmp);
		
	LOGF_TRACE("app version [%s] ",version.c_str());

	return version;

}


std::string getversionkernel(void)
{
 	std::string version="";
	int inRet=0;
	char tmp[35+1]={0};
	char AppVer[64]={0};
	EMV_CT_TERMDATA_TYPE xEMVTermdata;
	inRet= EMV_CT_GetTermData(&xEMVTermdata);
	LOGF_TRACE("\nResult of EMV_CT_GetTermData [%X]",inRet);
	LOGAPI_HEXDUMP_TRACE("\nKernel Version",xEMVTermdata.KernelVersion,sizeof(xEMVTermdata.KernelVersion));
	LOGAPI_HEXDUMP_TRACE("\nFrameworkVersion",xEMVTermdata.FrameworkVersion,sizeof(xEMVTermdata.FrameworkVersion));
	strcpy(AppVer,xEMVTermdata.KernelVersion);
	version.insert(0,AppVer);

	LOGF_TRACE("app version [%s] ",version.c_str());

	return version;

}


void getapptype(void)
{
	int inRet=0;
	char tmp[3]={0};
	inRet = getEnvFile((char *)"application", (char *)"ACQMUL",tmp ,sizeof(tmp));

	if(inRet<1)
	{
		LOGF_TRACE("app type no valid");

	}

	else
	{
		acqmul = atoi(tmp);
		LOGF_TRACE("app type [%s]",tmp);                      // valor  1                                                              valor 0
		LOGF_TRACE("app type:  [%s]" ,(acqmul? ":::::: MULTI ADQUIRIENTE ::::::::":"::::::::: UNI ADQUIRIENTE :::::::::"));
		LOGF_TRACE("ACQMUL = %d", acqmul);
	}

}

bool isColorDisplayCapable() 
{
   unsigned long size;
   PI_display_info_st info;

   if(platforminfo_get(PI_DISPLAY_INFO, &info,sizeof(info),&size)==PI_OK) {

	   if(info.color)
		   return(true);
	   else
		   return(false);
   }
   return(false);
}

//Daee 07/12/2017
std::string emvADKTextDisplayCallback(int error) 
{
	std::string msg="";

	LOGF_TRACE("emvADKTextDisplayCallback");

	switch(error) 
	{
		case EMV_ADK_TXT_NO_TXT:
			break;
		case EMV_ADK_TXT_REFUND_CONF_AMOUNT:
			break;
		case EMV_ADK_TXT_AMOUNT:
			break;
		case EMV_ADK_TXT_3AMO_TIP:
			break;
		case EMV_ADK_TXT_3AMO_CASHBACK:
			break;
		case EMV_ADK_TXT_APPROVED:
		case EMV_ADK_TXT_AUTH_APPROVED:
			msg = "APROBADA";
		    break;
		case EMV_ADK_TXT_DECLINED:         
		case EMV_ADK_TXT_AUTH_DECLINED:
			msg = "DECLINADA";
		    break;
		case EMV_ADK_TXT_NOT_ACCEPTED:
			msg = "NO ACEPTADA";
		    break;
		case EMV_ADK_TXT_CARD_ERROR:         
		case EMV_ADK_TXT_PROCESSING_ERROR:
			msg = "ERROR EN TARJETA";
		    break;
		case EMV_ADK_TXT_CARD_READ_OK:
			msg = "LECTURA EXITOSA";
		    break;
		case EMV_ADK_TXT_AUTHORIZING:
			msg = "AUTORIZANDO";
		    break;
		case EMV_ADK_TXT_REMOVE_CARD:
			msg = "RETIRE TARJETA";
		    break;
		case EMV_ADK_TXT_USE_CHIP_READER:
			msg = "USE LECTOR DE CHIP";
		    break;
		case EMV_ADK_TXT_USE_MAG_STRIPE:
			msg = "USE LECTOR DE BANDA";
		    break;
		case EMV_ADK_TXT_VOICEAUT:
			break;
		case EMV_ADK_TXT_SEE_PHONE:
			msg = "VEA TELEFONO";
		    break;
		case EMV_ADK_TXT_RETAP_SAME:         
		case EMV_ADK_TXT_RETAP_SAME_L1:
			msg = "ERROR \n APROXIME DE NUEVO";
		    break;
		case EMV_ADK_TXT_2_CARDS_IN_FIELD:
			msg = "APROXIME SOLO UNA TARJETA";
		    break;
		case EMV_ADK_TXT_CARD_READ_COMPLETE:
			break;

		//1 Emv Common Interface Error 
		case EMV_ADK_FALLBACK_CHIP_ONLY:
			msg = "USE LECTOR DE CHIP";
		    break;
		case EMV_ADK_2_CTLS_CARDS:
			msg = "APROXIME SOLO UNA TARJETA";
		    break;
		case EMV_ADK_TXN_CTLS_L1_ERR:
			msg = "ERROR EN TARJETA";
		    break;
		case EMV_ADK_TXN_CTLS_EMPTY_LIST:
		case EMV_ADK_TXN_CTLS_EMV_USE_OTHER_CARD:
			msg = "USE CHIP O BANDA";
		    break;
		case EMV_ADK_CTLS_DOMESTIC_ONLY_NOT_READABLE:
			msg = "NO SOPORTADA";
		    break;
	}

	if(! msg.empty())
	{
		LOGF_TRACE("CBS TEXT ADK: ################   %s   #############",msg.c_str());
	}

	return msg;
}

void vdSetTimeStamp () 
{
	int inRet = 0;
	std::string date;
	std::string datefrmt;
	char TimeStamp[20]={0};
	char chAux[4] = {0};

	date = __DATE__;					 // =  __DATE__ Feb 27 2018  __TIME__ 14:19:46

	LOGF_TRACE("date [%s]",date.c_str());

	if (date.find("Jan") != std::string::npos)
		datefrmt = "01";
	else if (date.find("Feb") != std::string::npos)
		datefrmt = "02";
	else if (date.find("Mar") != std::string::npos)
		datefrmt = "03";
	else if (date.find("Apr") != std::string::npos)
		datefrmt = "04";
	else if (date.find("May") != std::string::npos)
		datefrmt = "05";
	else if (date.find("Jun") != std::string::npos)
		datefrmt = "06";
	else if (date.find("Jul") != std::string::npos)
		datefrmt = "07";
	else if (date.find("Aug") != std::string::npos)
		datefrmt = "08";
	else if (date.find("Sep") != std::string::npos)
		datefrmt = "09";
	else if (date.find("Oct") != std::string::npos)
		datefrmt = "10";
	else if (date.find("Nov") != std::string::npos)
		datefrmt = "11";
	else if (date.find("Dec") != std::string::npos)
		datefrmt = "12";
	
	LOGF_TRACE("datefrmt [%s]",datefrmt.c_str());

	datefrmt += date.substr(4,2);

	LOGF_TRACE("datefrmt [%s]",datefrmt.c_str());

	sprintf(chAux,"%s",date.substr(9,2).c_str());


	datefrmt += date.substr(9,2);

	LOGF_TRACE("datefrmt [%s]",datefrmt.c_str());

	sprintf(TimeStamp , "_%s_%s",datefrmt.c_str(),__TIME__); 

	inRet = putEnvFile((char *)"application", (char *)"TIMESTAMP", TimeStamp);

	return;
}


void setLedConfig ( )
{
/*	int result;
	unsigned char col_on_rgb[3] = {39, 203, 00};
    unsigned char col_off_rgb[3] = {192, 192, 192};
	unsigned char col_bg_rgb[3] = {255, 255, 255};
	unsigned short radius[2] = {8,0};
	
	EMV_CTLS_LED_COLORS col_on;
	EMV_CTLS_LED_COLORS col_off;
	
	EMV_CTLS_LED_CONFIG ledConfig;

//	std::string szModelName = customCore::customUtils::getModel();

	// Establish default configuration
	ledConfig.region_x=0;
	ledConfig.region_y=47;
	ledConfig.region_width=240;
	ledConfig.region_height=30;
	col_on.num_colors=1;
	col_on.colors=col_on_rgb;
	col_off.num_colors=1;
	col_off.colors=col_off_rgb;
	
	ledConfig.colors_off=col_off;
	ledConfig.colors_on=col_on;
	ledConfig.bg_color=col_bg_rgb;
	ledConfig.shape=EMV_CTLS_LED_SHAPE_CIRCLE;
	ledConfig.shape_params=radius;

	// Adjustments for the different platforms (if any)
//	if( szModelName.compare(0,5,"V240m") == 0 ){
		ledConfig.region_x = 0;	ledConfig.region_y = 105;
		ledConfig.region_width = 320;	ledConfig.region_height = 40;
		radius[0] = 12;
		ledConfig.shape_params = radius;
//    }
//	else if( strstr (szModelName.c_str(),"V205c") != NULL)//v205c
//	{
//		ledConfig.region_x = 0;	ledConfig.region_y = 48;
//		ledConfig.region_width = 320;	ledConfig.region_height = 20;
//		ledConfig.shape_params = radius;
//	}

	result=EMV_CTLS_LED_ConfigDesign_Extended(&ledConfig);

	LOGF_TRACE("result=%d",result);*/

		int result;
	unsigned char col_on_rgb[3] = {39, 203, 00};
    unsigned char col_off_rgb[3] = {192, 192, 192};
	unsigned char col_bg_rgb[3] = {255, 255, 255};
	unsigned short radius[2] = {8,0};
	int h=0;
	EMV_CTLS_LED_COLORS col_on;
	EMV_CTLS_LED_COLORS col_off;
	
	EMV_CTLS_LED_CONFIG ledConfig;

	// Establish default configuration
	ledConfig.region_x=0;
	ledConfig.region_y=47;
	ledConfig.region_width=240;
	ledConfig.region_height=30;
	col_on.num_colors=1;
	col_on.colors=col_on_rgb;
	col_off.num_colors=1;
	col_off.colors=col_off_rgb;
	
	ledConfig.colors_off=col_off;
	ledConfig.colors_on=col_on;
	ledConfig.bg_color=col_bg_rgb;
	ledConfig.shape=EMV_CTLS_LED_SHAPE_CIRCLE;
	ledConfig.shape_params=radius;
	ledConfig.region_x = 0;	ledConfig.region_y = 105;
	ledConfig.region_width = 320;	ledConfig.region_height = 40;
	radius[0] = 12;
	ledConfig.shape_params = radius;


	uiGetPropertyInt(UI_DEVICE_HEIGHT,&h );
	LOGF_TRACE("height= %p ",&h);
	if(h==320)
	{
		ledConfig.region_x = 0;	
		ledConfig.region_y = 75;
	}
	else if(h==480)
	{
		ledConfig.region_x = 0;	ledConfig.region_y = 105;
		ledConfig.region_width = 320;	
	}

	LOGF_TRACE("Ready config led");
	result=EMV_CTLS_LED_ConfigDesign_Extended(&ledConfig);
	LOGF_TRACE("result=%d",result);



	return;
}



