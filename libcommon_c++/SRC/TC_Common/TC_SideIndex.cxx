//===========================================================================//
// Purpose : Method definitions for the TC_SideIndex class.
//
//           Public methods include:
//           - TC_SideIndex_c, ~TC_SideIndex_c
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
#include "TC_SideIndex.h"

//===========================================================================//
// Method         : TC_SideIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED ),
      index_( UINT_MAX )
{
} 

//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      TC_SideMode_t side,
      size_t        index )
      :
      side_( side ),
      index_( index )
{
} 

//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      const TC_SideIndex_c& sideIndex )
      :
      side_( sideIndex.side_ ),
      index_( sideIndex.index_ )
{
} 

//===========================================================================//
// Method         : ~TC_SideIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideIndex_c::~TC_SideIndex_c( 
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
TC_SideIndex_c& TC_SideIndex_c::operator=( 
      const TC_SideIndex_c& sideIndex )
{
   if( &sideIndex != this )
   {
      this->side_ = sideIndex.side_;
      this->index_ = sideIndex.index_;
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
bool TC_SideIndex_c::operator<( 
      const TC_SideIndex_c& sideIndex ) const
{
   bool isLessThan = false;
   
   if( this->side_ < sideIndex.side_ )
   {
      isLessThan = true;
   }
   else if(( this->side_ == sideIndex.side_ ) && 
           ( this->index_ < sideIndex.index_ ))
   {
      isLessThan = true;
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideIndex_c::operator==( 
      const TC_SideIndex_c& sideIndex ) const
{
   return(( this->side_ == sideIndex.side_ ) &&
          ( this->index_ == sideIndex.index_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideIndex_c::operator!=( 
      const TC_SideIndex_c& sideIndex ) const
{
   return( !this->operator==( sideIndex ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSideIndex;
   this->ExtractString( &srSideIndex );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSideIndex ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::ExtractString( 
      string* psrSideIndex,
      size_t  sideLength ) const
{
   if( psrSideIndex )
   {
      if( this->IsValid( ))
      {
         string srSide;
         TC_ExtractStringSideMode( this->side_, &srSide );

         if( sideLength )
         {
            srSide = srSide.substr( 0, sideLength );
         }
         *psrSideIndex = srSide;

         if( this->index_ != SIZE_MAX )
         {
            char szIndex[TIO_FORMAT_STRING_LEN_VALUE];
            sprintf( szIndex, "%lu", static_cast< unsigned long >( this->index_ ));

            *psrSideIndex += " ";
            *psrSideIndex += szIndex;
         }
      }
      else
      {
         *psrSideIndex = "?";
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
void TC_SideIndex_c::Set( 
      TC_SideMode_t side,
      size_t        index )
{
   this->side_ = side;
   this->index_ = index;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::Clear( 
      void )
{
   this->side_ = TC_SIDE_UNDEFINED;
   this->index_ = UINT_MAX;
} 
