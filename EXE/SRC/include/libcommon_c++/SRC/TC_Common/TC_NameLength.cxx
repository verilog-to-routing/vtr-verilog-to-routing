//===========================================================================//
// Purpose : Method definitions for the TC_NameLength class.
//
//           Public methods include:
//           - TC_NameLength_c, ~TC_NameLength_c
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
#include "TC_NameLength.h"

//===========================================================================//
// Method         : TC_NameLength_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      void )
      :
      length_( UINT_MAX )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const string&      srName,
            unsigned int length )
      :
      srName_( srName ),
      length_( length )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const char*        pszName,
            unsigned int length )
      :
      srName_( TIO_PSZ_STR( pszName )),
      length_( length )
{
} 

//===========================================================================//
TC_NameLength_c::TC_NameLength_c( 
      const TC_NameLength_c& nameLength )
      :
      srName_( nameLength.srName_ ),
      length_( nameLength.length_ )
{
} 

//===========================================================================//
// Method         : ~TC_NameLength_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameLength_c::~TC_NameLength_c( 
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
TC_NameLength_c& TC_NameLength_c::operator=( 
      const TC_NameLength_c& nameLength )
{
   if( &nameLength != this )
   {
      this->srName_ = nameLength.srName_;
      this->length_ = nameLength.length_;
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
bool TC_NameLength_c::operator<( 
      const TC_NameLength_c& nameLength ) const
{
   return(( TC_CompareStrings( this->srName_, nameLength.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameLength_c::operator==( 
      const TC_NameLength_c& nameLength ) const
{
   return(( this->srName_ == nameLength.srName_ ) &&
          ( this->length_ == nameLength.length_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameLength_c::operator!=( 
      const TC_NameLength_c& nameLength ) const
{
   return( !this->operator==( nameLength ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srNameLength;
   this->ExtractString( &srNameLength );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srNameLength ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::ExtractString( 
      string* psrNameLength ) const
{
   if( psrNameLength )
   {
      if( this->IsValid( ))
      {
         *psrNameLength = "\"";
         *psrNameLength += this->srName_;
         *psrNameLength += "\"";

         if( this->length_ != UINT_MAX )
         {
            char szLength[TIO_FORMAT_STRING_LEN_VALUE];
            sprintf( szLength, "%d", this->length_ );

            *psrNameLength += " ";
            *psrNameLength += szLength;
         }
      }
      else
      {
         *psrNameLength = "?";
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
void TC_NameLength_c::Set( 
      const string&      srName,
            unsigned int length )
{
   this->srName_ = srName;
   this->length_ = length;
} 

//===========================================================================//
void TC_NameLength_c::Set( 
      const char*        pszName,
            unsigned int length )
{
   this->srName_ = TIO_PSZ_STR( pszName );
   this->length_ = length;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameLength_c::Clear( 
      void )
{
   this->srName_ = "";
   this->length_ = UINT_MAX;
} 
