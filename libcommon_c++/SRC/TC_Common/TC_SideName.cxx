//===========================================================================//
// Purpose : Method definitions for the TC_SideName class.
//
//           Public methods include:
//           - TC_SideName_c, ~TC_SideName_c
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
#include "TC_SideName.h"

//===========================================================================//
// Method         : TC_SideName_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideName_c::TC_SideName_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED ),
      dx_( INT_MAX ),
      dy_( INT_MAX )
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
      const string&       srName )
      :
      side_( side ),
      dx_( dx ),
      dy_( dy ),
      srName_( srName )
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
      const char*         pszName )
      :
      side_( side ),
      dx_( dx ),
      dy_( dy ),
      srName_( TIO_PSZ_STR( pszName ))
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
      const TC_SideName_c& sideName )
      :
      side_( sideName.side_ ),
      dx_( sideName.dx_ ),
      dy_( sideName.dy_ ),
      srName_( sideName.srName_ )
{
} 

//===========================================================================//
// Method         : ~TC_SideName_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideName_c::~TC_SideName_c( 
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
TC_SideName_c& TC_SideName_c::operator=( 
      const TC_SideName_c& sideName )
{
   if( &sideName != this )
   {
      this->side_ = sideName.side_;
      this->dx_ = sideName.dx_;
      this->dy_ = sideName.dy_;
      this->srName_ = sideName.srName_;
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
bool TC_SideName_c::operator<( 
      const TC_SideName_c& sideName ) const
{
   bool isLessThan = false;
   
   if( this->side_ < sideName.side_ )
   {
      isLessThan = true;
   }
   else if( this->side_ == sideName.side_ )
   {
      if( this->dx_ < sideName.dx_ )
      {
         isLessThan = true;
      }
      else if( this->dx_ == sideName.dx_ )
      {
         if( this->dy_ < sideName.dy_ )
         {
            isLessThan = true;
         }
         else if( this->dy_ == sideName.dy_ )
         {
            isLessThan = (( TC_CompareStrings( this->srName_, sideName.srName_ ) < 0 ) ? 
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
bool TC_SideName_c::operator==( 
      const TC_SideName_c& sideName ) const
{
   return(( this->side_ == sideName.side_ ) &&
          ( this->dx_ == sideName.dx_ ) &&
          ( this->dy_ == sideName.dy_ ) &&
          ( this->srName_ == sideName.srName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideName_c::operator!=( 
      const TC_SideName_c& sideName ) const
{
   return( !this->operator==( sideName ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideName_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSideName;
   this->ExtractString( &srSideName );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSideName ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideName_c::ExtractString( 
      string* psrSideName ) const
{
   if( psrSideName )
   {
      if( this->IsValid( ))
      {
         *psrSideName = "";

         if( this->side_ != TC_SIDE_UNDEFINED )
         {
            string srSide;
            TC_ExtractStringSideMode( this->side_, &srSide );
            *psrSideName += srSide;
         }

         *psrSideName += psrSideName->length( ) ? " " : "";

         if( this->dx_ != INT_MAX && this->dy_ != INT_MAX )
         {
            char szDxDy[TIO_FORMAT_STRING_LEN_DATA];
            sprintf( szDxDy, "%d %d", this->dx_, this->dy_ );
            *psrSideName += szDxDy;
         }

         *psrSideName += psrSideName->length( ) ? " " : "";

         if( this->srName_.length( ))
         {
            *psrSideName += "\"";
            *psrSideName += this->srName_;
            *psrSideName += "\"";
         }
      }
      else
      {
         *psrSideName = "?";
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
void TC_SideName_c::Set( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
      const string&       srName )
{
   this->side_ = side;
   this->dx_ = dx;
   this->dy_ = dy;
   this->srName_ = srName;
} 

//===========================================================================//
void TC_SideName_c::Set( 
            TC_SideMode_t side,
            int           dx,
            int           dy,
      const char*         pszName )
{
   this->side_ = side;
   this->dx_ = dx;
   this->dy_ = dy;
   this->srName_ = TIO_PSZ_STR( pszName );
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideName_c::Clear( 
      void )
{
   this->side_ = TC_SIDE_UNDEFINED;
   this->dx_ = INT_MAX;
   this->dy_ = INT_MAX;
   this->srName_ = "";
} 
