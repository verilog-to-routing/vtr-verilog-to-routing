//===========================================================================//
// Purpose : Method definitions for the TIO_FileOutput class.
//
//           Public methods include:
//           - Open
//           - Close
//           - Write
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

#include "TIO_FileOutput.h"

//===========================================================================//
// Method         : Open
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileOutput_c::Open(
      const char*              pszFileName,
            TIO_FileOpenMode_t fileOpen )
{
   string srFileName( TIO_PSZ_STR( pszFileName ));
   return( this->Open( srFileName, fileOpen ));
}

//===========================================================================//
bool TIO_FileOutput_c::Open(
      const string&            srFileName,
            TIO_FileOpenMode_t fileOpen )
{
   this->lineNum_ = 0;
   this->isEnabled_ = this->fileHandler_.IsValid( srFileName, fileOpen );

   return( this->fileHandler_.Open( srFileName, fileOpen ));
}

//===========================================================================//
// Method         : Close
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_FileOutput_c::Close( 
      void )
{
   this->fileHandler_.Close( );
   this->isEnabled_ = false;
}

//===========================================================================//
// Method         : Write
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileOutput_c::Write( 
      const char* pszString )
{
   if( this->fileHandler_.Write( pszString ))
   {
      ++this->lineNum_;
   }
   else
   {
      this->fileHandler_.Close( );
   }
   return( this->fileHandler_.IsValid( ));
} 
