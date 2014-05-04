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
