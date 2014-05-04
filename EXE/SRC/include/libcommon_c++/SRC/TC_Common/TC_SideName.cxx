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
