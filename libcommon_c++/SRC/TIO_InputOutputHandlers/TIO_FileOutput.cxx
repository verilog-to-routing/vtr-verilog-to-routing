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
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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
