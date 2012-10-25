//===========================================================================//
// Purpose : Methods for the 'TOP_OptionsHandler_c' class.
//
//           Public methods include:
//           - TOP_OptionsHandler_c, ~TOP_OptionsHandler_c
//           - Init
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

#include "TOP_OptionsHandler.h"

//===========================================================================//
// Method         : TOP_OptionsHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOP_OptionsHandler_c::TOP_OptionsHandler_c(
      TOS_OptionsStore_c* poptionsStore )
      :
      poptionsStore_( 0 )
{
   if( poptionsStore )
   {
      this->Init( poptionsStore );
   }
}

//===========================================================================//
// Method         : ~TOP_OptionsHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOP_OptionsHandler_c::~TOP_OptionsHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TOP_OptionsHandler_c::Init( 
      TOS_OptionsStore_c* poptionsStore )
{
   this->poptionsStore_ = poptionsStore;

   return( this->poptionsStore_ ? true : false );
}

//===========================================================================//
// Method         : IsValid
// Purpose        : Return true if this options handler was able to properly 
//                  initialize the current 'poptionsStore_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline bool TOP_OptionsHandler_c::IsValid( void ) const
{
   bool isValid = false;

   if( this->poptionsStore_ )
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
