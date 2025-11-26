/*
 * encryp_aes.h
 */

#ifndef ENCRYP_AES_H_
#define ENCRYP_AES_H_

#if (defined _VRXEVO)
	#if (defined AESENCR_IMPORT)
	#define DllSPEC __declspec(dllimport)
	#else
	#define DllSPEC   __declspec(dllexport)
	#endif
#else
	#define DllSPEC
#endif

#define VS_FAILURE_AES -1
#define VS_SUCCESS_AES  0
#define VS_ERROR_AES   -2
#define VS_ESCAPE_AES  -3
#define TRUE_DUKPT      1
#define FALSE_DUKPT     0
#define TRUE_AES        1
#define FALSE_AES       0

/* AES() */
#define AES128E_F     0x04    /* AES encryption using a 128-bit key */
#define AES128D_F     0x05    /* AES decryption using a 128-bit key */
#define AES192E_F     0x06    /* AES encryption using a 192-bit key */
#define AES192D_F     0x07    /* AES decryption using a 192-bit key */
#define AES256E_F     0x08    /* AES encryption using a 256-bit key */
#define AES256D_F     0x09    /* AES decryption using a 256-bit key */


/*Defs Mod*/
#define SIZE_KEY_RND	16
#define SIZE_KEY_CX	    32
#define SIZE_CRC_32_HEX	 4
#define AES_SZB			16
#define AES_ENC			0		//ENCRYPT
#define AES_DEC			1		//DECRYPT
#define AES_SZB  	    16
#define AES_PAD  	    0X20

//#define FILE_AES "/home/usr1/flash/Key_data.txt"
#if (defined _VRXEVO)
	#define FILE_AES 	"I:/CxKey.dat"
	#define FILE_BINES  "I:/CrdExc.dat"
	#define FILE_EXC 	"I:/ExcCrd.dat"
#else
	#define FILE_AES 	"/home/usr1/flash/CxKey.dat"
	#define FILE_BINES  "/home/usr1/flash/CrdExc.dat"
	#define FILE_EXC 	"/home/usr1/flash/ExcCrd.dat"
#endif
#define KCX_FLOAD  "KCXLOD"



/*Defs Error*/
#define CX_SRLND_ERR	-50
#define CX_DECKR_ERR	-55
#define CX_CRC32_ERR	-60
#define CX_KSAVE_ERR	-65
#define CX_KLOAD_ERR	-70
#define CX_AESED_ERR	-75
#define CX_IPEKE_ERR	-76		// AJM 03/09/2014 1


#define RET_ECL_E_CIF_CX					 -320		//Daee 21/05/2014



#define FAILURE_OPEN_FILE	-3
#define FAILURE_CRYPTOREAD	-2
#define AES_ERROR           -1
#define AES_SUCCESS  	     0


#define CRD_EXC_NME_AES		8		// AJM 26/02/2015 1
#define CRD_EXC_NAM_AES		15		// AJM 26/08/2014 1
#define CRD_EXC_VER_AES		2
#define CRD_EXC_LEN_AES		4
#define CRD_EXC_CRD_AES		515		// AJM 26/08/2014 1
#define EXC_CRD_SIZ_AES		15


#define HEX_KEY_SIZ			16
#define ASC_KEY_SIZ			HEX_KEY_SIZ << 1
#define HEX_KSN_SIZ			10
#define ASC_KSN_SIZ			HEX_KSN_SIZ << 1
#define HEX_CHK_SIZ			3
#define ASC_CHK_SIZ			HEX_CHK_SIZ << 1
#define HEX_CRC_SIZ			4
#define ASC_CRC_SIZ			HEX_CRC_SIZ << 1
#define HEX_VER_SIZ			5
#define ASC_VER_SIZ			HEX_VER_SIZ << 1
#define HEX_PAD_SIZ			1
#define ASC_PAD_SIZ			HEX_PAD_SIZ << 1
#define HEX_MOD_SIZ			256
#define ASC_MOD_SIZ			HEX_MOD_SIZ << 1
#define HEX_EXP_SIZ			3
#define ASC_EXP_SIZ			HEX_EXP_SIZ << 1


#define  LEFT_AES               0x00
#define  RIGHT_AES             0x80
#define  CENTER_AES          0x88



// CODIGO DE LA LIB DE DIEGO ESCALERA
/*Data Struct*/
typedef struct Rqkrnd
{
	char chRndKey[SIZE_KEY_RND + 1];
	char chCrc32 [SIZE_CRC_32_HEX + 1];
	int	 inRndKey;
	int  inCrc32;
}Rqkrnd;


typedef struct LdkDat
{
	char chSrlN[30];
	char chEncK[40];
	char chCrc32[20];

	int  inSrlN;
	int  inEncK;
	int  inCrc32;
}LdkDat;


typedef struct CapXKy
{
	char chKeyCX[SIZE_KEY_CX];
}CapXKy;



typedef struct CrdExc_AES
{
	// Qt
	char chNam[CRD_EXC_NAM_AES];
	char chVer[CRD_EXC_VER_AES];
	char chKsA[ASC_KSN_SIZ];
	char chUse[CRD_EXC_LEN_AES];
	char chSrc[CRD_EXC_LEN_AES];
	char chCrp[CRD_EXC_CRD_AES];
	char chCrc[ASC_CRC_SIZ];

	// crd
	char chCrd[CRD_EXC_CRD_AES];

	// key + ksn
	char chKey[HEX_KEY_SIZ];
	char chKsn[HEX_KSN_SIZ];
}
CrdExc_AES;


typedef struct
{
	char chMin[EXC_CRD_SIZ_AES];
	char chMax[EXC_CRD_SIZ_AES];

	int inMin;
	int inMax;
}
ExcCrd_AES;



extern unsigned char key_inyected;			  //Almacena el status de la inyeccion de la llave, 00 para cuando esta inyectada la llave, 00 en otro caso.
extern unsigned char transpKey[32];		  //Almacena la llave de transporte.
extern unsigned char PruebasInternas;		  //Para el caso de que NO SE REQUIERA validar, numero de serie, CRC32.
extern char InitVector[AES_SZB];	  //Almacena el vector inicial, para este caso sera con 0x0;
extern char KeyAES[AES_SZB*2];	  //Almacena la llave para encriptar;
extern char InputAES[AES_SZB*10];	  //Almacena el bloque inicial para encriptar
extern char OutputAES[AES_SZB*10];  //Almacena el bloque final para encriptar
extern int Bloques;              	  //Numero de bloques a cifrar.



DllSPEC int StatusLlave( void );
DllSPEC unsigned long crc32_funtion( unsigned char *message, int longitud );
DllSPEC void getCRC32Data( unsigned char * InputData, unsigned char* OutputData, unsigned long nLen  );
DllSPEC void In_XOR( unsigned char * OutputData,unsigned char* InputData, unsigned char* InputData2, unsigned int nLen );

DllSPEC int inAEScbc256(int opt,char pchKey[],char pchsrc[],char pchdest[], int Nblocks);
DllSPEC int FormaBloques16(unsigned char input[], unsigned char len, unsigned char output[] );
DllSPEC int LenDec2Hex(char *LenHex, int lendec );
DllSPEC int in_Cx_EDta(char pchData[] ,int inData,char PchDOut[], int *inDOut);
DllSPEC int HEXtoASC(unsigned char *source, unsigned int LenPacket, unsigned char *dest, unsigned int *BytesWritten);
DllSPEC int ASCtoHEX(char * pchAsc, int inAsc, char * pchOut, int * inOut);
DllSPEC int inGenerateRandom (unsigned char *random8); // FAG 07-NOV-2016
DllSPEC int inAES(unsigned char ucAesOption, unsigned char *pucAesKey8N,unsigned char *pucInputData, unsigned char *pucOutputData);  // FAG 07-nov-2016
DllSPEC int incryptoWrite (int hdl, const char *buf, int len);
DllSPEC int incryptoRead (int hdl, char *buf, int len);

/*CODIGO DE DIEGO ESCALERA*/
DllSPEC int in_Cx_Kld(LdkDat *KLD);
DllSPEC int in_Cx_fkld(void);
DllSPEC int in_Cx_GCrc(char pchCrc32[], int *inCrc32);
DllSPEC int in_Cx_DDta(char pchData[] ,int inData,char PchDOut[], int *inDOut);		// AJM 26/08/2014 1
DllSPEC int boKeyLod(void);															//Daee 29/1072014

//

DllSPEC int inKRnd(char pchKrnd[], int inKrnd);
DllSPEC int inVldKrchV(char pchcrc32[], int incrc32)	;		//Daee 19/05/2014
DllSPEC int inVldSrlN(char SrlN[], int inSrlN);
DllSPEC int inDcrptk(char kEnc[], int inkEnc);
DllSPEC int in_Cx_KRmv(int bo_sav);		// AJM 05/09/2014 1
DllSPEC int in_Cx_KSav(void);

//

DllSPEC int in_Cx_KLod(void);
DllSPEC int in_Crp_Sav(char pchFil[], void *pvdDst, int inChr);
DllSPEC int in_Crp_Lod(char pchFil[], void *pvdDst, int inChr);
DllSPEC unsigned long ulcrc32 (char pch_crc [], int in_crc, unsigned long ul_sed);
DllSPEC int inAEScbc256(int opt,char pchKey[],char pchsrc[],char pchdest[], int Nblocks);		 //Daee 19/05/2014
DllSPEC int inAESecb256(int opt,char pchKey[],char pchsrc[],char pchdest[], int Nblocks); 	 //Daee 19/05/2014
DllSPEC int in_exc_dsm (char pchRsp[]);  // FAG 22-11-2016
DllSPEC int inTrmSpcEnd (char pch_src [], int in_max);  // FAG 22 nov-2016
DllSPEC int inInvStr (char pch_end [], int in_max, char pch_src [], int in_src);   // FAG 22 nov-2016
DllSPEC int in_ipk_wrn (void);
DllSPEC int bo_exc_avl (void);
DllSPEC int in_Crp_Exc_Sav(char pchRsp[]);
DllSPEC int in_Crp_Sav_Exc(char pchCrd[], int inCrd);


DllSPEC int inGetEnvStr(char pch_key[],char pch_Out[],int inSizeMax);
DllSPEC int inSetEnvStr(char pch_key[],char pch_Val[],int inSize);
DllSPEC int inPadAes(char pchData[] ,int inData,char PchDOut[], int *inDOut);

DllSPEC int inHex2Asc(char pchHex[], char pchAsc[], int inHex);
DllSPEC int inAsc2Hex(char pchAsc[], char pchHex[], int inAsc);


DllSPEC int in_Xor(char pchEnd[], char pchOp1[], char pchOp2[], int inChr);
extern void  _SVC_DSP_2_HEX_ (const char *dsp, char *hex, int pairs);
extern void  _SVC_HEX_2_DSP_(const char *hex, char *dsp, int count);




DllSPEC int in_Cx_Krq(Rqkrnd *KRQ); // FAG 30-ENE-2017
DllSPEC int inpurge_char(char * buffer,char rem_char);
DllSPEC int aes_pad(char *pdest_buf, char *psrc_buf, char pad_char, int pad_size, int align);
DllSPEC int aes_append_char (char *string, char c);




#endif /* ENCRYP_AES_H_ */
