//===========================================================================//
// Purpose : Methods for the 'TCP_CircuitHandler_c' class.
//
//           Public methods include:
//           - TCP_CircuitHandler_c, ~TCP_CircuitHandler_c
//           - Init
//           - SyntaxError
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TIO_PrintHandler.h"

#include "TCP_CircuitHandler.h"

//===========================================================================//
// Method         : TCP_CircuitHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TCP_CircuitHandler_c::TCP_CircuitHandler_c(
      TCD_CircuitDesign_c* pcircuitDesign )
      :
      pcircuitDesign_( 0 )
{
   if( pcircuitDesign )
   {
      this->Init( pcircuitDesign );
   }
}

//===========================================================================//
// Method         : ~TCP_CircuitHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TCP_CircuitHandler_c::~TCP_CircuitHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TCP_CircuitHandler_c::Init( 
      TCD_CircuitDesign_c* pcircuitDesign )
{
   this->pcircuitDesign_ = pcircuitDesign;

   return( this->pcircuitDesign_ ? true : false );
}

//===========================================================================//
// Method         : SyntaxError
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TCP_CircuitHandler_c::SyntaxError( 
            unsigned int lineNum, 
      const string&      srFileName,
      const char*        pszMessageText )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = printHandler.Error( "Syntax error %s.\n"
                                 "%sSee file '%s', line %d.\n",
                                 TIO_PSZ_STR( pszMessageText ),
                                 TIO_PREFIX_ERROR_SPACE,
                                 TIO_SR_STR( srFileName ), lineNum );
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Purpose        : Return true if this circuit handler was able to 
//                  properly initialize a 'pcircuitDesign_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
inline bool TCP_CircuitHandler_c::IsValid( void ) const
{
   bool isValid = false;

   if( this->pcircuitDesign_ )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      if( printHandler.IsWithinMaxErrorCount( ) &&
          printHandler.IsWithinMaxWarningCount( ))
      {
         isValid = true;
      }
   }
   return( isValid );
}
