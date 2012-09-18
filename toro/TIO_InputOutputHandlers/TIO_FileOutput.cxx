//===========================================================================//
// Purpose : Method definitions for the TIO_FileOutput class.
//
//           Public methods include:
//           - Open
//           - Close
//           - Write
//
//===========================================================================//

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
