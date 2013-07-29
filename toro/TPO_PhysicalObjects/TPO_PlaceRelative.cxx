//===========================================================================//
// Purpose : Method definitions for the TPO_PlaceRelative class.
//
//           Public methods include:
//           - TPO_PlaceRelative_c, ~TPO_PlaceRelative_c
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
#include "TPO_PlaceRelative.h"

//===========================================================================//
// Method         : TPO_PlaceRelative_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PlaceRelative_c::TPO_PlaceRelative_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED ),
      dx_( INT_MAX ),
      dy_( INT_MAX ),
      rotateEnable_( false )
{
} 

//===========================================================================//
TPO_PlaceRelative_c::TPO_PlaceRelative_c( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
            bool          rotateEnable,
      const string&       srName )
      :
      side_( side ),
      dx_( dx ),
      dy_( dy ),
      rotateEnable_( rotateEnable ),
      srName_( srName )
{
} 

//===========================================================================//
TPO_PlaceRelative_c::TPO_PlaceRelative_c( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
            bool          rotateEnable,
      const char*         pszName )
      :
      side_( side ),
      dx_( dx ),
      dy_( dy ),
      rotateEnable_( rotateEnable ),
      srName_( TIO_PSZ_STR( pszName ))
{
} 

//===========================================================================//
TPO_PlaceRelative_c::TPO_PlaceRelative_c( 
      const TPO_PlaceRelative_c& placeRelative )
      :
      side_( placeRelative.side_ ),
      dx_( placeRelative.dx_ ),
      dy_( placeRelative.dy_ ),
      rotateEnable_( placeRelative.rotateEnable_ ),
      srName_( placeRelative.srName_ )
{
} 

//===========================================================================//
// Method         : ~TPO_PlaceRelative_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PlaceRelative_c::~TPO_PlaceRelative_c( 
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
TPO_PlaceRelative_c& TPO_PlaceRelative_c::operator=( 
      const TPO_PlaceRelative_c& placeRelative )
{
   if( &placeRelative != this )
   {
      this->side_ = placeRelative.side_;
      this->dx_ = placeRelative.dx_;
      this->dy_ = placeRelative.dy_;
      this->rotateEnable_ = placeRelative.rotateEnable_;
      this->srName_ = placeRelative.srName_;
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
bool TPO_PlaceRelative_c::operator<( 
      const TPO_PlaceRelative_c& placeRelative ) const
{
   bool isLessThan = false;
   
   if( this->side_ < placeRelative.side_ )
   {
      isLessThan = true;
   }
   else if( this->side_ == placeRelative.side_ )
   {
      if( this->dx_ < placeRelative.dx_ )
      {
         isLessThan = true;
      }
      else if( this->dx_ == placeRelative.dx_ )
      {
         if( this->dy_ < placeRelative.dy_ )
         {
            isLessThan = true;
         }
         else if( this->dy_ == placeRelative.dy_ )
         {
            isLessThan = (( TC_CompareStrings( this->srName_, placeRelative.srName_ ) < 0 ) ? 
                          true : false );
         }
      }
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
bool TPO_PlaceRelative_c::operator==( 
      const TPO_PlaceRelative_c& placeRelative ) const
{
   return(( this->side_ == placeRelative.side_ ) &&
          ( this->dx_ == placeRelative.dx_ ) &&
          ( this->dy_ == placeRelative.dy_ ) &&
          ( this->rotateEnable_ == placeRelative.rotateEnable_ ) &&
          ( this->srName_ == placeRelative.srName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PlaceRelative_c::operator!=( 
      const TPO_PlaceRelative_c& placeRelative ) const
{
   return( !this->operator==( placeRelative ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PlaceRelative_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srPlaceRelative;
   this->ExtractString( &srPlaceRelative );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srPlaceRelative ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PlaceRelative_c::ExtractString( 
      string* psrPlaceRelative ) const
{
   if( psrPlaceRelative )
   {
      if( this->IsValid( ))
      {
         *psrPlaceRelative = "";

         if( this->side_ != TC_SIDE_UNDEFINED )
         {
            string srSide;
            TC_ExtractStringSideMode( this->side_, &srSide );
            *psrPlaceRelative += srSide;
         }

         *psrPlaceRelative += psrPlaceRelative->length( ) ? " " : "";

         if( this->dx_ != INT_MAX && this->dy_ != INT_MAX )
         {
            char szDxDy[TIO_FORMAT_STRING_LEN_DATA];
            sprintf( szDxDy, "%d %d", this->dx_, this->dy_ );
            *psrPlaceRelative += szDxDy;
         }

         if( this->rotateEnable_ )
         {
            *psrPlaceRelative += psrPlaceRelative->length( ) ? " " : "";
            *psrPlaceRelative += "rotate";
         }

         *psrPlaceRelative += psrPlaceRelative->length( ) ? " " : "";

         if( this->srName_.length( ))
         {
            *psrPlaceRelative += "\"";
            *psrPlaceRelative += this->srName_;
            *psrPlaceRelative += "\"";
         }
      }
      else
      {
         *psrPlaceRelative = "?";
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
void TPO_PlaceRelative_c::Set( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
            bool          rotateEnable,
      const string&       srName )
{
   this->side_ = side;
   this->dx_ = dx;
   this->dy_ = dy;
   this->rotateEnable_ = rotateEnable;
   this->srName_ = srName;
} 

//===========================================================================//
void TPO_PlaceRelative_c::Set( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
            bool          rotateEnable,
      const char*         pszName )
{
   this->side_ = side;
   this->dx_ = dx;
   this->dy_ = dy;
   this->rotateEnable_ = rotateEnable;
   this->srName_ = TIO_PSZ_STR( pszName );
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PlaceRelative_c::Clear( 
      void )
{
   this->side_ = TC_SIDE_UNDEFINED;
   this->dx_ = INT_MAX;
   this->dy_ = INT_MAX;
   this->rotateEnable_ = false;
   this->srName_ = "";
} 
