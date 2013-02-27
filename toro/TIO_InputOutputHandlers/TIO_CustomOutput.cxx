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
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
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
