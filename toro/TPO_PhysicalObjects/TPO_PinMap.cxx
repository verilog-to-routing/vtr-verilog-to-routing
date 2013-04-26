//===========================================================================//
// Purpose : Method definitions for the TPO_PinMap class.
//
//           Public methods include:
//           - TPO_PinMap_c, ~TPO_PinMap_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - Set
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TPO_PinMap.h"

//===========================================================================//
// Method         : TPO_PinMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      void )
      :
      type_( TC_TYPE_UNDEFINED )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const string&       srInstPinName,
      const string&       srCellPinName,
            TC_TypeMode_t type )
      :
      srInstPinName_( srInstPinName ),
      srCellPinName_( srCellPinName ),
      type_( type )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const char*         pszInstPinName,
      const char*         pszCellPinName,
            TC_TypeMode_t type )
      :
      srInstPinName_( TIO_PSZ_STR( pszInstPinName )),
      srCellPinName_( TIO_PSZ_STR( pszCellPinName )),
      type_( type )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const TPO_PinMap_c& pinMap )
      :
      srInstPinName_( pinMap.srInstPinName_ ),
      srCellPinName_( pinMap.srCellPinName_ ),
      type_( pinMap.type_ )
{
} 

//===========================================================================//
// Method         : ~TPO_PinMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PinMap_c::~TPO_PinMap_c( 
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
TPO_PinMap_c& TPO_PinMap_c::operator=( 
      const TPO_PinMap_c& pinMap )
{
   if( &pinMap != this )
   {
      this->srInstPinName_ = pinMap.srInstPinName_;
      this->srCellPinName_ = pinMap.srCellPinName_;
      this->type_ = pinMap.type_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator<( 
      const TPO_PinMap_c& pinMap ) const
{
   return(( TC_CompareStrings( this->srInstPinName_, pinMap.srInstPinName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator==( 
      const TPO_PinMap_c& pinMap ) const
{
   return(( this->srInstPinName_ == pinMap.srInstPinName_ ) &&
          ( this->srCellPinName_ == pinMap.srCellPinName_ ) &&
          ( this->type_ == pinMap.type_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator!=( 
      const TPO_PinMap_c& pinMap ) const
{
   return( !this->operator==( pinMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PinMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srType;
   TC_ExtractStringTypeMode( this->type_, &srType );
   printHandler.Write( pfile, spaceLen, "\"%s\"=\"%s\" %s\n",
                                        TIO_SR_STR( this->srInstPinName_ ),
                                        TIO_SR_STR( this->srCellPinName_ ),
                                        TIO_SR_STR( srType ));
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PinMap_c::Set(
      const string&       srInstPinName,
      const string&       srCellPinName,
            TC_TypeMode_t type )
{
   this->srInstPinName_ = srInstPinName;
   this->srCellPinName_ = srCellPinName;
   this->type_ = type;
} 

//===========================================================================//
void TPO_PinMap_c::Set(
      const char*         pszInstPinName,
      const char*         pszCellPinName,
            TC_TypeMode_t type )
{
   this->srInstPinName_ = TIO_PSZ_STR( pszInstPinName );
   this->srCellPinName_ = TIO_PSZ_STR( pszCellPinName );
   this->type_ = type;
} 
