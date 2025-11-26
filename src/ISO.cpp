//SYSTEM INCLUDES
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//ADK INCLUDES
#include "html/gui.h"
#include "msr/msr.h"


//USER INCLUDES
#include "ISO/ISO.h"
#include "EmvRdc/VosEMVProcess.h"
#include "CapX/debug.h"






int fld_39(char* ch_fd39,int in_fd39, char* out_status,int* sz_out_status)
{
	int iResult = 0;

	debug("--fld_39--");

	*sz_out_status = 2;

	debug("ch_fd39 = %s", ch_fd39);

	if(!memcmp(ch_fd39,"\x30\x30",in_fd39))
	{		
		debug("Aprobada");
		memcpy(out_status,"\x30\x30",*sz_out_status);
		EMV_EndTransactionCTLS("APROBADA", 0, TRUE);
	}
	else if(!memcmp(ch_fd39,"\x30\x35",in_fd39))
	{
		debug("Declinada");
		memcpy(out_status,"\x30\x30",*sz_out_status);
		EMV_EndTransactionCTLS("DECLINADA", 0, TRUE);
	}

	return iResult;
}



