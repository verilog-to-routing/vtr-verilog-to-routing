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
            sprintf( szIndex, "%lu", static_cast< unsigned long >( this->index_ ));

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
