//===========================================================================//
// Purpose : Method definitions for the TPO_Relative class.
//
//           Public methods include:
//           - TPO_Relative_c, ~TPO_Relative_c
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
#include "TPO_Relative.h"

//===========================================================================//
// Method         : TPO_Relative_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_Relative_c::TPO_Relative_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED ),
      dx_( INT_MAX ),
      dy_( INT_MAX ),
      rotateEnable_( false )
{
} 

//===========================================================================//
TPO_Relative_c::TPO_Relative_c( 
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
TPO_Relative_c::TPO_Relative_c( 
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
TPO_Relative_c::TPO_Relative_c( 
      const TPO_Relative_c& relative )
      :
      side_( relative.side_ ),
      dx_( relative.dx_ ),
      dy_( relative.dy_ ),
      rotateEnable_( relative.rotateEnable_ ),
      srName_( relative.srName_ )
{
} 

//===========================================================================//
// Method         : ~TPO_Relative_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_Relative_c::~TPO_Relative_c( 
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
TPO_Relative_c& TPO_Relative_c::operator=( 
      const TPO_Relative_c& relative )
{
   if( &relative != this )
   {
      this->side_ = relative.side_;
      this->dx_ = relative.dx_;
      this->dy_ = relative.dy_;
      this->rotateEnable_ = relative.rotateEnable_;
      this->srName_ = relative.srName_;
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
bool TPO_Relative_c::operator<( 
      const TPO_Relative_c& relative ) const
{
   bool isLessThan = false;
   
   if( this->side_ < relative.side_ )
   {
      isLessThan = true;
   }
   else if( this->side_ == relative.side_ )
   {
      if( this->dx_ < relative.dx_ )
      {
         isLessThan = true;
      }
      else if( this->dx_ == relative.dx_ )
      {
         if( this->dy_ < relative.dy_ )
         {
            isLessThan = true;
         }
         else if( this->dy_ == relative.dy_ )
         {
            isLessThan = (( TC_CompareStrings( this->srName_, relative.srName_ ) < 0 ) ? 
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
bool TPO_Relative_c::operator==( 
      const TPO_Relative_c& relative ) const
{
   return(( this->side_ == relative.side_ ) &&
          ( this->dx_ == relative.dx_ ) &&
          ( this->dy_ == relative.dy_ ) &&
          ( this->rotateEnable_ == relative.rotateEnable_ ) &&
          ( this->srName_ == relative.srName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_Relative_c::operator!=( 
      const TPO_Relative_c& relative ) const
{
   return( !this->operator==( relative ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Relative_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srRelative;
   this->ExtractString( &srRelative );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srRelative ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Relative_c::ExtractString( 
      string* psrRelative ) const
{
   if( psrRelative )
   {
      if( this->IsValid( ))
      {
         *psrRelative = "";

         if( this->side_ != TC_SIDE_UNDEFINED )
         {
            string srSide;
            TC_ExtractStringSideMode( this->side_, &srSide );
            *psrRelative += srSide;
         }

         *psrRelative += psrRelative->length( ) ? " " : "";

         if( this->dx_ != INT_MAX && this->dy_ != INT_MAX )
         {
            char szDxDy[TIO_FORMAT_STRING_LEN_DATA];
            sprintf( szDxDy, "%d %d", this->dx_, this->dy_ );
            *psrRelative += szDxDy;
         }

         if( this->rotateEnable_ )
         {
            *psrRelative += psrRelative->length( ) ? " " : "";
            *psrRelative += "rotate";
         }

         *psrRelative += psrRelative->length( ) ? " " : "";

         if( this->srName_.length( ))
         {
            *psrRelative += "\"";
            *psrRelative += this->srName_;
            *psrRelative += "\"";
         }
      }
      else
      {
         *psrRelative = "?";
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
void TPO_Relative_c::Set( 
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
void TPO_Relative_c::Set( 
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
void TPO_Relative_c::Clear( 
      void )
{
   this->side_ = TC_SIDE_UNDEFINED;
   this->dx_ = INT_MAX;
   this->dy_ = INT_MAX;
   this->rotateEnable_ = false;
   this->srName_ = "";
} 
