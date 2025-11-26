#include <string>
#include <stdio.h>
#include <time.h>
#ifndef _VRXEVO
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <unistd.h>



#include "getsysinfo.h"
#include "sysinfo/sysinfo.h"
#include "sysinfo/syspm.h"
#include "sysinfo/sysbar.h"
#include "sysinfo/sysbeep.h"

using namespace std;
using namespace vfisysinfo;

DllSPEC int getModelVos(char *  value )
{
	int r=0;
	string modelname;
	
	r = sysGetPropertyString(SYS_PROP_HW_MODEL_NAME,modelname);
	sprintf(value,"%s",modelname.c_str());	
	
	return r;
}

DllSPEC int getSoVos(char *  value )
{
	int r=0;
	string version;
	
	r = sysGetPropertyString(SYS_PROP_OS_VERSION,version);
	if(version.compare(0,8,"release-") == 0)
		sprintf(value,"%s",(version.substr(8)).c_str());	
	else
		sprintf(value,"%s",version.c_str());	
	
	return r;
}

