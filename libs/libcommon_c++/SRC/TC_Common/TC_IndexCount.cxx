//===========================================================================//
// Purpose : Method definitions for the TC_IndexCount class.
//
//           Public methods include:
//           - TC_IndexCount_c, ~TC_IndexCount_c
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
#include "TC_IndexCount.h"

//===========================================================================//
// Method         : TC_IndexCount_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
TC_IndexCount_c::TC_IndexCount_c( 
      void )
      :
      index_( INT_MAX ),
      count_( SIZE_MAX )
{
} 

//===========================================================================//
TC_IndexCount_c::TC_IndexCount_c( 
      int    index,
      size_t count )
      :
      index_( index ),
      count_( count )
{
} 

//===========================================================================//
TC_IndexCount_c::TC_IndexCount_c( 
      const TC_IndexCount_c& indexCount )
      :
      index_( indexCount.index_ ),
      count_( indexCount.count_ )
{
} 

//===========================================================================//
// Method         : ~TC_IndexCount_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
TC_IndexCount_c::~TC_IndexCount_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
TC_IndexCount_c& TC_IndexCount_c::operator=( 
      const TC_IndexCount_c& indexCount )
{
   if( &indexCount != this )
   {
      this->index_ = indexCount.index_;
      this->count_ = indexCount.count_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
bool TC_IndexCount_c::operator<( 
      const TC_IndexCount_c& indexCount ) const
{
   bool isLessThan = false;
   
   if( this->index_ < indexCount.index_ )
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
// 10/05/12 jeffr : Original
//===========================================================================//
bool TC_IndexCount_c::operator==( 
      const TC_IndexCount_c& indexCount ) const
{
   return(( this->index_ == indexCount.index_ ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
bool TC_IndexCount_c::operator!=( 
      const TC_IndexCount_c& indexCount ) const
{
   return( !this->operator==( indexCount ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TC_IndexCount_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srIndexCount;
   this->ExtractString( &srIndexCount );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srIndexCount ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TC_IndexCount_c::ExtractString( 
      string* psrIndexCount ) const
{
   if( psrIndexCount )
   {
      if( this->IsValid( ))
      {
         char szIndex[TIO_FORMAT_STRING_LEN_VALUE];
         sprintf( szIndex, "%d", this->index_ );

         *psrIndexCount = szIndex;

         if( this->count_ != SIZE_MAX )
         {
            char szCount[TIO_FORMAT_STRING_LEN_VALUE];
            sprintf( szIndex, "%lu", static_cast< unsigned long >( this->count_ ));

            *psrIndexCount += " ";
            *psrIndexCount += szCount;
         }
      }
      else
      {
         *psrIndexCount = "?";
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
void TC_IndexCount_c::Set( 
      int    index,
      size_t count )
{
   this->index_ = index;
   this->count_ = count;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TC_IndexCount_c::Clear( 
      void )
{
   this->index_ = INT_MAX;
   this->count_ = SIZE_MAX;
} 
