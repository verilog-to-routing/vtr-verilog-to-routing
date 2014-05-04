//===========================================================================//
// Purpose : Method definitions for the TC_Index class.
//
//           Public methods include:
//           - TC_Index_c, ~TC_Index_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//           - Clear
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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
#include "TC_Index.h"

//===========================================================================//
// Method         : TC_Index_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TC_Index_c::TC_Index_c( 
      void )
      :
      index_( INT_MAX )
{
} 

//===========================================================================//
TC_Index_c::TC_Index_c( 
      int index )
      :
      index_( index )
{
} 

//===========================================================================//
TC_Index_c::TC_Index_c( 
      const TC_Index_c& index )
      :
      index_( index.index_ )
{
} 

//===========================================================================//
// Method         : ~TC_Index_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TC_Index_c::~TC_Index_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TC_Index_c& TC_Index_c::operator=( 
      const TC_Index_c& index )
{
   if( &index != this )
   {
      this->index_ = index.index_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TC_Index_c::operator<( 
      const TC_Index_c& index ) const
{
   bool isLessThan = false;
   
   if( this->index_ < index.index_ )
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
// 06/25/13 jeffr : Original
//===========================================================================//
bool TC_Index_c::operator==( 
      const TC_Index_c& index ) const
{
   return(( this->index_ == index.index_ ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TC_Index_c::operator!=( 
      const TC_Index_c& index ) const
{
   return( !this->operator==( index ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TC_Index_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srIndex;
   this->ExtractString( &srIndex );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srIndex ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TC_Index_c::ExtractString( 
      string* psrIndex ) const
{
   if( psrIndex )
   {
      if( this->IsValid( ))
      {
         char szIndex[TIO_FORMAT_STRING_LEN_VALUE];
         sprintf( szIndex, "%d", this->index_ );

         *psrIndex = szIndex;
      }
      else
      {
         *psrIndex = "?";
      }
   }
} 

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TC_Index_c::Set( 
      int index )
{
   this->index_ = index;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TC_Index_c::Clear( 
      void )
{
   this->index_ = INT_MAX;
} 
