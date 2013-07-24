//===========================================================================//
// Purpose : Method definitions for the TAS_Clock class.
//
//           Public methods include:
//           - TAS_Clock_c, ~TAS_Clock_c
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_Clock.h"

//===========================================================================//
// Method         : TAS_Clock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c::TAS_Clock_c( 
      void )
      :
      autoSize( false ),
      bufferSize( 0.0),
      capWire( 0.0 )
{
}

//===========================================================================//
TAS_Clock_c::TAS_Clock_c( 
      const TAS_Clock_c& clock )
      :
      autoSize( clock.autoSize ),
      bufferSize( clock.bufferSize ),
      capWire( clock.capWire )
{
}

//===========================================================================//
// Method         : ~TAS_Clock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c::~TAS_Clock_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c& TAS_Clock_c::operator=( 
      const TAS_Clock_c& clock )
{
   if( &clock != this )
   {
      this->autoSize = clock.autoSize;
      this->bufferSize = clock.bufferSize;
      this->capWire = clock.capWire;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Clock_c::operator==( 
      const TAS_Clock_c& clock ) const
{
   return(( this->autoSize == clock.autoSize ) &&
          ( TCTF_IsEQ( this->bufferSize, clock.bufferSize )) &&
          ( TCTF_IsEQ( this->capWire, clock.capWire )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Clock_c::operator!=( 
      const TAS_Clock_c& clock ) const
{
   return( !this->operator==( clock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Clock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<clock" );
   if( this->autoSize )
   {
      printHandler.Write( pfile, 0, " buffer_size=\"auto\"" );
   }
   else if( TCTF_IsGT( this->bufferSize, 0.0 ))
   {
      printHandler.Write( pfile, 0, " buffer_size=\"%0.*f\"", 
                                    precision, this->bufferSize );
   }
   if( TCTF_IsGT( this->capWire, 0.0 ))
   {
      printHandler.Write( pfile, 0, " C_wire=\"%0.*e\"", 
                                    precision + 1, this->capWire );
   }
   printHandler.Write( pfile, 0, "/>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Clock_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Clock_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<clock" );
   if( this->autoSize )
   {
      printHandler.Write( pfile, 0, " buffer_size=\"auto\"" );
   }
   else if( TCTF_IsGT( this->bufferSize, 0.0 ))
   {
      printHandler.Write( pfile, 0, " buffer_size=\"%0.*f\"", 
                                    precision, this->bufferSize );
   }
   if( TCTF_IsGT( this->capWire, 0.0 ))
   {
      printHandler.Write( pfile, 0, " C_wire=\"%0.*e\"", 
                                    precision + 1, this->capWire );
   }
   printHandler.Write( pfile, 0, "/>\n" );
}
