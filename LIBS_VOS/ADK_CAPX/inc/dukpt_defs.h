//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
/*
 * dukpt_defs.h
 *
 *  Created on: 07/08/2013
 *      Author: JVelazquez
 *
 *  Este archivo contiene definiciones necesarias para la migracion de DUKPT.
 */

#ifndef DUKPT_DEFS_H_
#define DUKPT_DEFS_H_

#if (defined _VRXEVO)
	#if (defined CAPX_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif


//#ifdef __cplusplus
//extern "C" {
//#endif

//Se utiliza en usrcmds.c
#define INVLD_FORMAT    -4

#define VS_SUCCESS     0               /* General purpose error code */
#define VS_ERROR       (-1)            /* Operator error */
#define VS_FAILURE     (-2)            /* System-level error */
#define VS_ESCAPE      (-3)            /* Operator quit transaction */


// Host or referral decisions
#define HOST_AUTHORISED       1
#define HOST_DECLINED         2
#define FAILED_TO_CONNECT     3
#define REFERRAL_AUTHORISED   4
#define REFERRAL_DECLINED     5
#define TRANS_CANCELED        6
#define HOST_UNKNOW        	  7


#define VS_BOOL	int

#define VS_FALSE           ((VS_BOOL) 0)
#define VS_TRUE            ((VS_BOOL) 1)

typedef unsigned char UBYTE;
typedef unsigned char UWORD;


#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif
#ifndef SUCCESS
#define SUCCESS 0
#endif
#ifndef FAILURE
#define FAILURE -1
#endif

struct TRACK {
    char acct   [23];       /* acct nums are digits + NULL + maybe 3 spaces */
    char exp    [5];        /* exp is 4 + NULL */
    char name   [27];       /* Name for trk 2, 26 chars + NULL */
    char type   [4];        /* Type reqd by VISA/MC, 3 + NULL */
    char PVN    [6];        /* PVN reqd VISA/MC 5 + NULL */
    char disc   [17];       /* TK1 and TK2 only + NULL*/
    char track  [108];      /* raw track data */
};

#define TK_1             1
#define TK_2             2
#define TK_3             3


DllSPEC extern int	purge_char (char *buffer, char rem_char);
extern int _SVC_VALID_DATE_( const char *yyyymmddhhmmss );
extern void  _SVC_DSP_2_HEX_ (const char *dsp, char *hex, int pairs);
extern void  _SVC_HEX_2_DSP_(const char *hex, char *dsp, int count);
extern unsigned char _SVC_LRC_CALC_(unsigned char *buffer, int sz, unsigned int seed);
extern void AscHex( unsigned char * pBuffer, const char  * pAscii, unsigned char   NumBytes );
static UBYTE A2hex( char AsciiChar );
UWORD copy_track ( char * destination, char * source, int max  );
//#ifdef __cplusplus
//}
//#endif

#endif /* DUKPT_DEFS_H_ */

//Migrado por JVelazquez 22/08/14
#endif //MAKE_DUKPT
