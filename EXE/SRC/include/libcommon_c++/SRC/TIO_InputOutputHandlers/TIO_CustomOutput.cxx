//===========================================================================//
// Purpose : Method definitions for the TIO_CustomOutput class.
//
//           Public methods include:
//           - Write
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

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
