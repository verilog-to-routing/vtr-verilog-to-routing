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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
