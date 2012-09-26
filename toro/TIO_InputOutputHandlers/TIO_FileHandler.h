//===========================================================================//
// Purpose : Declaration and inline definitions for a TIO_FileHandler class.
//
//           Inline methods include:
//           - Flush
//           - Reset
//           - GetFileName
//           - GetFileStream
//
//===========================================================================//

#ifndef TIO_FILE_HANDLER_H
#define TIO_FILE_HANDLER_H

#include <cstdio>
#include <string>
using namespace std;

#include "TIO_Typedefs.h"

//===========================================================================//
// Purpose:       : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_FileHandler_c
{
public:

   TIO_FileHandler_c( void );
   TIO_FileHandler_c( const string& srFileName, 
                      TIO_FileOpenMode_t fileOpen = TIO_FILE_OPEN_UNDEFINED,
                      const char* pszFileType = 0,
                      TIO_PrintMode_t printMode = TIO_PRINT_ERROR );
   TIO_FileHandler_c( const char* pszFileName,
                      TIO_FileOpenMode_t fileOpen = TIO_FILE_OPEN_UNDEFINED,
                      const char* pszFileType = 0,
                      TIO_PrintMode_t printMode = TIO_PRINT_ERROR );
   ~TIO_FileHandler_c( void );
    
   bool Open( const string& srFileName, 
              TIO_FileOpenMode_t fileOpen,
              const char* pszFileType = 0,
              TIO_PrintMode_t printMode = TIO_PRINT_ERROR );
   bool Open( const char* pszFileName, 
              TIO_FileOpenMode_t fileOpen,
              const char* pszFileType = 0,
              TIO_PrintMode_t printMode = TIO_PRINT_ERROR );
   void Close( void );

   bool Read( char* pszString, size_t lenString ) const;
   bool Write( const char* pszString, ... );

   void Flush( void );
   void Reset( void ) const;

   const char* GetFileName( void ) const;
   FILE* GetFileStream( void ) const;

   bool ApplyPreProcessor( void );

   bool IsValid( const string& srFileName, 
                 TIO_FileOpenMode_t fileOpen,
                 const char* pszFileType = 0,
                 TIO_PrintMode_t printMode = TIO_PRINT_ERROR ) const;
   bool IsValid( const char* pszFileName, 
                 TIO_FileOpenMode_t fileOpen,
                 const char* pszFileType = 0,
                 TIO_PrintMode_t printMode = TIO_PRINT_ERROR ) const;
   bool IsValid( void ) const;

private:

   string srFileName_;
   FILE*  pfileStream_;
};

//===========================================================================//
// Purpose:       : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline void TIO_FileHandler_c::Flush( 
      void )
{
   if( this->pfileStream_ )
   {
      fflush( this->pfileStream_ );
   }
}

//===========================================================================//
inline void TIO_FileHandler_c::Reset( 
      void ) const
{
   if( this->pfileStream_ )
   {
      fseek( this->pfileStream_, 0, SEEK_SET );
   }
}

//===========================================================================//
inline FILE* TIO_FileHandler_c::GetFileStream( 
      void ) const
{
   return( this->pfileStream_ );
}

//===========================================================================//
inline const char* TIO_FileHandler_c::GetFileName( 
      void ) const
{
   return( this->srFileName_.data( ));
}

#endif
