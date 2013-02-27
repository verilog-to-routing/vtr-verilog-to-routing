//===========================================================================//
// Purpose : Supporting methods for the TI_Input pre-processor class.  
//           These methods support validating input options and data files.
//
//           Private methods include:
//           - ValidateOptionsStore_
//           - ValidateArchitectureSpec_
//           - ValidateFabricModel_
//           - ValidateCircuitDesign_
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

#include "TI_Input.h"

//===========================================================================//
// Method         : ValidateOptionsStore_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateOptionsStore_( 
      void )
{
   bool ok = true;

   // This is only a stub... (currently looking for intereseting work to do)

   return( ok );
}

//===========================================================================//
// Method         : ValidateArchitectureSpec_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateArchitectureSpec_( 
      void )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Validating architecture file ...\n" );

   ok = this->parchitectureSpec_->InitValidate( );

   return( ok );
}

//===========================================================================//
// Method         : ValidateFabricModel_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateFabricModel_( 
      void )
{
   bool ok = true;

   // This is only a stub... (currently looking for intereseting work to do)

   return( ok );
}

//===========================================================================//
// Method         : ValidateCircuitDesign_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateCircuitDesign_( 
      void )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Validating circuit file '%s'...\n",
                      TIO_SR_STR( this->pcircuitDesign_->srName ));

   ok = this->pcircuitDesign_->InitValidate( );

   return( ok );
}
