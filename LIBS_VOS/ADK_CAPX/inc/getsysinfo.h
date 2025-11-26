#include <string>
#include <stdio.h>
#include <time.h>
#ifndef _VRXEVO
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "sysinfo/sysinfo.h"
#include "sysinfo/syspm.h"
#include "sysinfo/sysbar.h"
#include "sysinfo/sysbeep.h"

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif


DllSPEC int getModelVos(char *  value );
DllSPEC int getSoVos(char *  value );
