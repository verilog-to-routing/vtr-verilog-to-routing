//===========================================================================//
// Purpose : Methods for the 'TCBP_CircutBlifHandler_c' class.
//
//           Public methods include:
//           - TCBP_CircuitBlifHandler_c, ~TCBP_CircuitBlifHandler_c
//           - Init
//           - IsValid
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

#include "TIO_PrintHandler.h"

#include "TCBP_CircuitBlifHandler.h"

//===========================================================================//
// Method         : TCBP_CircuitBlifHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifHandler_c::TCBP_CircuitBlifHandler_c(
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
// Method         : ~TCBP_CircuitBlifHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifHandler_c::~TCBP_CircuitBlifHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCBP_CircuitBlifHandler_c::Init( 
      TCD_CircuitDesign_c* pcircuitDesign )
{
   this->pcircuitDesign_ = pcircuitDesign;

   return( this->pcircuitDesign_ ? true : false );
}

//===========================================================================//
// Method         : IsValid
// Purpose        : Return true if this circuit handler was able to properly 
//                  initialize the current 'pcircuitDesign_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TCBP_CircuitBlifHandler_c::IsValid( void ) const
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
