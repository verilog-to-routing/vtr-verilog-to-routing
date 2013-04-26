//===========================================================================//
// Purpose : Method definitions for the TC_NameLength class.
//
//           Public methods include:
//           - TC_NameLength_c, ~TC_NameLength_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//           - Clear
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

#include "TC_StringUtils.h"
#include "TC_NameLength.h"

//===========================================================================//
// Method         : TC_NameLength_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      void )
      :
      length_( UINT_MAX )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const string&      srName,
            unsigned int length )
      :
      srName_( srName ),
      length_( length )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const char*        pszName,
            unsigned int length )
      :
      srName_( TIO_PSZ_STR( pszName )),
      length_( length )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const TC_NameLength_c& nameLength )
      :
      srName_( nameLength.srName_ ),
      length_( nameLength.length_ )
{
} 

//===========================================================================//
// Method         : ~TC_NameLength_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameLength_c::~TC_NameLength_c( 
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
TC_NameLength_c& TC_NameLength_c::operator=( 
      const TC_NameLength_c& nameLength )
{
   if( &nameLength != this )
   {
      this->srName_ = nameLength.srName_;
      this->length_ = nameLength.length_;
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
bool TC_NameLength_c::operator<( 
      const TC_NameLength_c& nameLength ) const
{
   return(( TC_CompareStrings( this->srName_, nameLength.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameLength_c::operator==( 
      const TC_NameLength_c& nameLength ) const
{
   return(( this->srName_ == nameLength.srName_ ) &&
          ( this->length_ == nameLength.length_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameLength_c::operator!=( 
      const TC_NameLength_c& nameLength ) const
{
   return( !this->operator==( nameLength ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srNameLength;
   this->ExtractString( &srNameLength );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srNameLength ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::ExtractString( 
      string* psrNameLength ) const
{
   if( psrNameLength )
   {
      if( this->IsValid( ))
      {
         *psrNameLength = "\"";
         *psrNameLength += this->srName_;
         *psrNameLength += "\"";

         if( this->length_ != UINT_MAX )
         {
            char szLength[TIO_FORMAT_STRING_LEN_VALUE];
            sprintf( szLength, "%d", this->length_ );

            *psrNameLength += " ";
            *psrNameLength += szLength;
         }
      }
      else
      {
         *psrNameLength = "?";
      }
   }
} 

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::Set( 
      const string&      srName,
            unsigned int length )
{
   this->srName_ = srName;
   this->length_ = length;
} 

//===========================================================================//
void TC_NameLength_c::Set( 
      const char*        pszName,
            unsigned int length )
{
   this->srName_ = TIO_PSZ_STR( pszName );
   this->length_ = length;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::Clear( 
      void )
{
   this->srName_ = "";
   this->length_ = UINT_MAX;
} 
