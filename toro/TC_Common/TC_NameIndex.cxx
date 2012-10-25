//===========================================================================//
// Purpose : Method definitions for the TC_NameIndex class.
//
//           Public methods include:
//           - TC_NameIndex_c, ~TC_NameIndex_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//           - Clear
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

#include "TC_StringUtils.h"
#include "TC_NameIndex.h"

//===========================================================================//
// Method         : TC_NameIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameIndex_c::TC_NameIndex_c( 
      void )
      :
      index_( SIZE_MAX )
{
} 

//===========================================================================//
TC_NameIndex_c::TC_NameIndex_c( 
      const string& srName,
            size_t  index )
      :
      srName_( srName ),
      index_( index )
{
} 

//===========================================================================//
TC_NameIndex_c::TC_NameIndex_c( 
      const char*  pszName,
            size_t index )
      :
      srName_( TIO_PSZ_STR( pszName )),
      index_( index )
{
} 

//===========================================================================//
TC_NameIndex_c::TC_NameIndex_c( 
      const TC_NameIndex_c& nameIndex )
      :
      srName_( nameIndex.srName_ ),
      index_( nameIndex.index_ )
{
} 

//===========================================================================//
// Method         : ~TC_NameIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameIndex_c::~TC_NameIndex_c( 
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
TC_NameIndex_c& TC_NameIndex_c::operator=( 
      const TC_NameIndex_c& nameIndex )
{
   if( &nameIndex != this )
   {
      this->srName_ = nameIndex.srName_;
      this->index_ = nameIndex.index_;
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
bool TC_NameIndex_c::operator<( 
      const TC_NameIndex_c& nameIndex ) const
{
   return(( TC_CompareStrings( this->srName_, nameIndex.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameIndex_c::operator==( 
      const TC_NameIndex_c& nameIndex ) const
{
   return(( this->srName_ == nameIndex.srName_ ) &&
          ( this->index_ == nameIndex.index_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameIndex_c::operator!=( 
      const TC_NameIndex_c& nameIndex ) const
{
   return( !this->operator==( nameIndex ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameIndex_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srNameIndex;
   this->ExtractString( &srNameIndex );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srNameIndex ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameIndex_c::ExtractString( 
      string* psrNameIndex ) const
{
   if( psrNameIndex )
   {
      if( this->IsValid( ))
      {
         *psrNameIndex = "\"";
         *psrNameIndex += this->srName_;
         *psrNameIndex += "\"";

	 if( this->index_ != SIZE_MAX )
	 {
   	    char szIndex[TIO_FORMAT_STRING_LEN_VALUE];
            sprintf( szIndex, "%lu", this->index_ );

            *psrNameIndex += " ";
            *psrNameIndex += szIndex;
         }
      }
      else
      {
         *psrNameIndex = "?";
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
void TC_NameIndex_c::Set( 
      const string& srName,
            size_t  index )
{
   this->srName_ = srName;
   this->index_ = index;
} 

//===========================================================================//
void TC_NameIndex_c::Set( 
      const char*  pszName,
            size_t index )
{
   this->srName_ = TIO_PSZ_STR( pszName );
   this->index_ = index;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameIndex_c::Clear( 
      void )
{
   this->srName_ = "";
   this->index_ = SIZE_MAX;
} 
