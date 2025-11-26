//Migrado por JVelazquez 22/08/14
#ifdef MAKE_DUKPT
/*
 * dukpt_defs.c
 *
 *  Created on: 11/08/2013
 *      Author: JVelazquez
 *
 *  Este archivo contiene definiciones necesarias para la migracion de DUKPT.
 */
#include <time.h>

#ifdef _VRXEVO
#else
#include <msrDevice.h>
#endif

//#include <Ecore.h>
#include <svc.h>
#include <fcntl.h>
#include <string.h>


//Este archivo hace referencia a las variables que se agregan por homologacion
//Agrego JVelazquez 11/08/14
#include "dukpt_defs.h"





//-----------------------------------------------------------------------------
//!  \brief     Realiza padding a una cadena
//!
//!  \param
//!     None
//!
//!  \return
//!     None
//************** se deja pendiente implementacion *****************************
//-----------------------------------------------------------------------------
DllSPEC extern int	purge_char (char *buffer, char rem_char)
{

	char *ptr;
	//char *buffer1;
	int rval = 0;
	ptr = buffer;
	//buffer1 = buffer;

	
	/* loop until the end of the string */
	while (*buffer != 0x00)
	{
				/**
				*	if the current character is the purge character
				*	eliminate it
				**/
		if (*ptr != rem_char)
			*buffer++ = *ptr++;

		else
		{			/* otherwise leave it in */
			ptr++;
			rval++;
		}
	}
			/* return the number of characters now in the buffer */
	return (rval);

}

//-----------------------------------------------------------------------------
//!  \brief     Valida si una fecha es correcta o no.
//!
//!  \param
//!     None
//!
//!  \return
//!     None
//************** se deja pendiente implementacion *****************************
//-----------------------------------------------------------------------------
extern int _SVC_VALID_DATE_( const char *yyyymmddhhmmss )
{
	return 1;
}

//-----------------------------------------------------------------------------
//!  \brief     Valida si una fecha es correcta o no.
//!
//!  \param
//!     None
//!
//!  \return
//!     None
//************** se deja pendiente implementacion *****************************
//-----------------------------------------------------------------------------
extern void  _SVC_DSP_2_HEX_ (const char *dsp, char *hex, int pairs)
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
	return;
}
/*************************************************************************
    Funtion Name: SVC_HEX_2_DSP
    Objective: Convierte un numero HEX -> Decimal, ambos son cadenas
    Inputs:
    Outputs:
    Returns:
    Created by  JVelazquez 11/08/14
	Notes:   Esta funcion se migro de la aplicacion de PinPad de Banorte,
	revisar comportamiento.
************************************************************************/
extern void  _SVC_HEX_2_DSP_(const char *hex, char *dsp, int count)
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

/*************************************************************************
    Funtion Name: _SVC_LRC_CALC_
    Objective: Obtiene el valor LRC
    Inputs:
    Outputs:
    Returns:
    Created by  JVelazquez 11/08/14
	Notes:  Esta funcion se migro de la aplicacion de PinPad de Banorte,
	revisar comportamiento.
************************************************************************/
extern unsigned char _SVC_LRC_CALC_(unsigned char *buffer, int sz, unsigned int seed)
{
	unsigned int i;
	unsigned char lrc;

	lrc = 0;

	for (i = 0; i < sz; i++)
		lrc ^= *(buffer + i);

	return(lrc);

}
//-----------------------------------------------------------------------------
//! \brief Converts ASCII format numbers (decimal or hexidecimal) into BCD or hex
//!
//! \param[out]     pBuffer            the address of the destination field
//! \param[in]      pAscii             the address of the ASCII source field
//! \param[in]      NumBytes           length of the destination field. The
//!                                    source field is twice as long.
//!
//! \return None
//-----------------------------------------------------------------------------

extern void AscHex( UBYTE * pBuffer, const char  * pAscii, UBYTE   NumBytes )
{
  UBYTE i;
  UBYTE b1,b2;

  for (i=0; i<NumBytes; i++)
  {
    b1 = A2hex(*pAscii++);
    b2 = A2hex(*pAscii++);
    *pBuffer++ = (b1 << 4) + (b2 << 0);
  }
}

//-----------------------------------------------------------------------------
//! \brief Converts a one digit ASCII value, 0-9 or A-F, to a binary value 0-F.
//!
//! \param[in]      AsciiChar             contains ASCII value
//!
//! \return zero if '0' or invalid ASCII character else binary value
//-----------------------------------------------------------------------------
static UBYTE A2hex( char AsciiChar )
{
  // '7' + 10 = 'A'

  if      (AsciiChar < '0') return (UBYTE)              0;
  else if (AsciiChar < ':') return (UBYTE)(AsciiChar - '0');
  else if (AsciiChar < 'A') return (UBYTE)              0;
  else if (AsciiChar < 'G') return (UBYTE)(AsciiChar - '7');
  else                      return (UBYTE)              0;
}

//-----------------------------------------------------------------------------
//!  \brief    function to copy track data and remove the start and end sentinels
//!
//!  \param
//!     None
//!
//!  \return
//!   None
//-----------------------------------------------------------------------------
//
UWORD copy_track ( char * destination, char * source, int max  )
{
	// ignore first byte (is 3B % char)
	// loop until 00 or 3F
	UWORD count = 0 ;

	while ((*source != 0x3F) && (count < max)) 
	//while ((*source != 0x3F) && (*source != 0x00) && (count < max)) // jbf 20250106 - just in case...
	{
		*destination++ = *source++ ;
		count++ ;

	}
	return count ;

} // end copy_track


#endif //MAKE_DUKPT
