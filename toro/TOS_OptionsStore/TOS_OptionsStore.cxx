//===========================================================================//
// Purpose : Method definitions for the TOS_OptionsStore class.
//
//           Public methods include:
//           - TOS_OptionsStore_c, ~TOS_OptionsStore_c
//           - Print
//           - Init
//           - Apply
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

#include "TOS_OptionsStore.h"

//===========================================================================//
// Method         : TOS_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OptionsStore_c::TOS_OptionsStore_c( 
      void )
{
} 

//===========================================================================//
// Method         : ~TOS_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OptionsStore_c::~TOS_OptionsStore_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Print( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->Print( pfile, spaceLen );
}

//===========================================================================//
void TOS_OptionsStore_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->controlSwitches.Print( pfile, spaceLen );
   this->rulesSwitches.Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Init( 
      void )
{
   this->controlSwitches.Init( );
   this->rulesSwitches.Init( );
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Apply( 
      void )
{
   this->controlSwitches.Apply( );
   this->rulesSwitches.Apply( );
}
