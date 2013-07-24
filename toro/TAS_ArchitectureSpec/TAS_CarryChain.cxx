//===========================================================================//
// Purpose : Method definitions for the TAS_CarryChain class.
//
//           Public methods include:
//           - TAS_CarryChain_c, ~TAS_CarryChain_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TAS_CarryChain.h"

//===========================================================================//
// Method         : TAS_CarryChain_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
TAS_CarryChain_c::TAS_CarryChain_c( 
      void )
{
}

//===========================================================================//
TAS_CarryChain_c::TAS_CarryChain_c( 
      const string& srName_ )
      :
      srName( srName_ )
{
}

//===========================================================================//
TAS_CarryChain_c::TAS_CarryChain_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName ))
{
}

//===========================================================================//
TAS_CarryChain_c::TAS_CarryChain_c( 
      const TAS_CarryChain_c& carryChain )
      :
      srName( carryChain.srName ),
      srFromPinName( carryChain.srFromPinName ),
      srToPinName( carryChain.srToPinName ),
      offset( carryChain.offset )
{
}

//===========================================================================//
// Method         : ~TAS_CarryChain_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
TAS_CarryChain_c::~TAS_CarryChain_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
TAS_CarryChain_c& TAS_CarryChain_c::operator=( 
      const TAS_CarryChain_c& carryChain )
{
   if( &carryChain != this )
   {
      this->srName = carryChain.srName;
      this->srFromPinName = carryChain.srFromPinName;
      this->srToPinName = carryChain.srToPinName;
      this->offset = carryChain.offset;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
bool TAS_CarryChain_c::operator==( 
      const TAS_CarryChain_c& carryChain ) const
{
   return(( this->srName == carryChain.srName ) &&
          ( this->srFromPinName == carryChain.srFromPinName ) &&
          ( this->srToPinName == carryChain.srToPinName ) &&
          ( this->offset == carryChain.offset ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
bool TAS_CarryChain_c::operator!=( 
      const TAS_CarryChain_c& carryChain ) const
{
   return( !this->operator==( carryChain ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
void TAS_CarryChain_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<carrychain name=\"%s\"", 
                                        TIO_SR_STR( this->srName ));
   if( this->srFromPinName.length( ))
   {
      printHandler.Write( pfile, 0, " from_pin=\"%s\"", 
                                           TIO_SR_STR( this->srFromPinName ));
   }
   if( this->srToPinName.length( ))
   {
      printHandler.Write( pfile, 0, " to_pin=\"%s\"", 
                                           TIO_SR_STR( this->srToPinName ));
   }
   if( this->offset.IsValid( ))
   {
      printHandler.Write( pfile, 0, " x_offset=\"%d\" y_offset=\"%d\" z_offset=\"%d\"", 
                                           this->offset.dx, this->offset.dy, this->offset.dz );
   }
   printHandler.Write( pfile, 0, "/>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
void TAS_CarryChain_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_CarryChain_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "<direct name=\"%s\" from_pin=\"%s\" to_pin=\"%s\" x_offset=\"%d\" y_offset=\"%d\" z_offset=\"%d\"/>\n", 
                                        TIO_SR_STR( this->srName ),
                                        TIO_SR_STR( this->srFromPinName ),
                                        TIO_SR_STR( this->srToPinName ),
                                        this->offset.dx, this->offset.dy, this->offset.dz );
}
