//===========================================================================//
// Purpose : Method definitions for the TAS_PinAssign class.
//
//           Public methods include:
//           - TAS_PinAssign_c, ~TAS_PinAssign_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_PinAssign.h"

//===========================================================================//
// Method         : TAS_PinAssign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_PinAssign_c::TAS_PinAssign_c( 
      void )
      :
      pattern( TAS_PIN_ASSIGN_PATTERN_UNDEFINED ),
      portNameList( TAS_PORT_NAME_LIST_DEF_CAPACITY ),
      side( TC_SIDE_UNDEFINED ),
      offset( 0 )
{
}

//===========================================================================//
TAS_PinAssign_c::TAS_PinAssign_c( 
      const TAS_PinAssign_c& pinAssign )
      :
      pattern( pinAssign.pattern ),
      portNameList( pinAssign.portNameList ),
      side( pinAssign.side ),
      offset( pinAssign.offset )
{
}

//===========================================================================//
// Method         : ~TAS_PinAssign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_PinAssign_c::~TAS_PinAssign_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_PinAssign_c& TAS_PinAssign_c::operator=( 
      const TAS_PinAssign_c& pinAssign )
{
   if( &pinAssign != this )
   {
      this->pattern = pinAssign.pattern;
      this->portNameList = pinAssign.portNameList;
      this->side = pinAssign.side;
      this->offset = pinAssign.offset;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_PinAssign_c::operator==( 
      const TAS_PinAssign_c& pinAssign ) const
{
   return(( this->pattern == pinAssign.pattern ) &&
          ( this->portNameList == pinAssign.portNameList ) &&
          ( this->side == pinAssign.side ) &&
          ( this->offset == pinAssign.offset ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_PinAssign_c::operator!=( 
      const TAS_PinAssign_c& pinAssign ) const
{
   return( !this->operator==( pinAssign ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_PinAssign_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pin_assign " );

   string srPattern;
   TAS_ExtractStringPinAssignPatternType( this->pattern, &srPattern );
   printHandler.Write( pfile, 0, "%s ", TIO_SR_STR( srPattern ));

   if(( this->side != TC_SIDE_UNDEFINED ) ||
      ( this->offset > 0 ) ||
      ( this->portNameList.IsValid( )))
   {
      if( this->side != TC_SIDE_UNDEFINED )
      {
         string srSide;
         TC_ExtractStringSideMode( this->side, &srSide );
         printHandler.Write( pfile, 0, "side = %s ", TIO_SR_STR( srSide ));
      }
      if( this->offset > 0 )
      {
         printHandler.Write( pfile, 0, "offset = %u ", this->offset );
      }
      printHandler.Write( pfile, 0, ">\n" );
      spaceLen += 3;

      if( this->portNameList.IsValid( ))
      {
         const char* pszS = ( this->portNameList.GetLength( ) > 1 ? "s" : "" );
         printHandler.Write( pfile, spaceLen, "<pin%s ", TIO_PSZ_STR( pszS ));
         for( size_t i = 0; i < this->portNameList.GetLength( ); ++i )
         {
            const TC_Name_c& portName = *this->portNameList[i];
            printHandler.Write( pfile, 0, "\"%s\" ", TIO_PSZ_STR( portName.GetName( )));
         }
         printHandler.Write( pfile, 0, "/>\n" );
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</pin_assign>\n" );
   }
   else
   {
      printHandler.Write( pfile, 0, "/>\n" );
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_PinAssign_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_PinAssign_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srSide;
   TC_ExtractStringSideMode( this->side, &srSide );
   printHandler.Write( pfile, spaceLen, "<loc side=\"%s\" offset=\"%u\">",
	                                TIO_SR_STR( srSide ),
		                        this->offset );
   if( this->portNameList.IsValid( ))
   {
      spaceLen += 3;

      for( size_t i = 0; i < this->portNameList.GetLength( ); ++i )
      {
         const TC_Name_c& portName = *this->portNameList[i];
         printHandler.Write( pfile, 0, "%s%s",
	                               TIO_PSZ_STR( portName.GetName( )),
                                       i + 1 == this->portNameList.GetLength( ) ? "" : " " );
      }
      spaceLen -= 3;
   }
   printHandler.Write( pfile, 0, "</loc>\n" );
}
