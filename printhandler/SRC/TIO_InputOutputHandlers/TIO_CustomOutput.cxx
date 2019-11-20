//===========================================================================//
// Purpose : Method definitions for the TIO_CustomOutput class.
//
//           Public methods include:
//           - Write
//
//===========================================================================//

#include <cstdio>
using namespace std;

#include "TIO_CustomOutput.h"

//===========================================================================//
// Method         : Write
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_CustomOutput_c::Write(
            TIO_PrintMode_t printMode,
      const char*           pszPrintText,
      const char*           pszPrintSrc ) const
{
   // Apply the currently installed print handler message function (if any)
   if( this->pfxCustomHandler_ )
   {
      ( this->pfxCustomHandler_ )( printMode, pszPrintText, pszPrintSrc );
   }
   else
   {
      fprintf( stderr, "TIO_CustomOutput( ) - No handler installed!\n" );
      fprintf( stderr, "TIO_CustomOutput( ) - code : %d\n"
                       "                      text : %s\n"
                       "                      src  : %s\n",
                       printMode,
                       TIO_PSZ_STR( pszPrintText ),
                       TIO_PSZ_STR( pszPrintSrc ));
   }
   return( this->pfxCustomHandler_ ? true : false );
} 
