//===========================================================================//
// Purpose : Method definitions for the TIO_FileHandler class.
//
//           Public methods include:
//           - TIO_FileHandler_c, ~TIO_FileHandler_c
//           - Open
//           - Close
//           - Read
//           - Write
//           - ApplyPreProcessor
//           - IsValid
//
//===========================================================================//

#include <cstdio>
#include <cstdarg>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TIO_PrintHandler.h"
#include "TIO_FileHandler.h"

//===========================================================================//
// Method         : TIO_FileHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_FileHandler_c::TIO_FileHandler_c( 
      void )
      :
      pfileStream_( 0 )
{
}

//===========================================================================//
TIO_FileHandler_c::TIO_FileHandler_c( 
      const string&            srFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t    printMode )
      :
      pfileStream_( 0 )
{
   this->Open( srFileName, fileOpen, pszFileType, printMode );
}

//===========================================================================//
TIO_FileHandler_c::TIO_FileHandler_c( 
      const char*              pszFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t    printMode )
      :
      pfileStream_( 0 )
{
   if( pszFileName )
   {
      this->Open( pszFileName, fileOpen, pszFileType, printMode );
   }
}

//===========================================================================//
// Method         : ~TIO_FileHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_FileHandler_c::~TIO_FileHandler_c( 
      void )
{
   this->Close( );
}

//===========================================================================//
// Method         : Open
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 08/10/12 jeffr : Cleaned up recently added Fatal message handling code
//===========================================================================//
bool TIO_FileHandler_c::Open(
      const string&            srFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t    printMode )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( this->pfileStream_ )
   {
      this->Close( );
   }

   const char* pszFileOpen = "r";
   switch( fileOpen )
   {
   case TIO_FILE_OPEN_READ:          pszFileOpen = "r";  break;
   case TIO_FILE_OPEN_WRITE:         pszFileOpen = "w";  break;
   case TIO_FILE_OPEN_APPEND:        pszFileOpen = "a";  break;
   case TIO_FILE_OPEN_BINARY_READ:   pszFileOpen = "rb"; break;
   case TIO_FILE_OPEN_BINARY_WRITE:  pszFileOpen = "wb"; break;
   case TIO_FILE_OPEN_BINARY_APPEND: pszFileOpen = "ab"; break;
   default: 
      ok = printHandler.Fatal( "TIO_FileHandler_c::Open - Unknown TIO_FileOpenMode_t\n" );
      break;
   }

   if( ok )
   {
      this->pfileStream_ = stdout;
      if( srFileName != "stdout" )
      {
         this->pfileStream_ = fopen( srFileName.data( ), pszFileOpen );
      }

      if( this->pfileStream_ )
      {
         this->srFileName_ = srFileName;
      }
      else
      {
         if( pszFileType )
         {
            switch( printMode )
            {
            case TIO_PRINT_WARNING:
               printHandler.Warning( "Failed to open %s file '%s' in \"%s\" mode!\n",
                                     pszFileType, 
                                     TIO_SR_STR( srFileName ),
                                     pszFileOpen );
   	       break;
            case TIO_PRINT_ERROR:
               printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode!\n",
                                   pszFileType, 
                                   TIO_SR_STR( srFileName ),
                                   pszFileOpen );
   	       break;
	    default: 
               ok = printHandler.Fatal( "TIO_FileHandler_c::Open - Unknown TIO_PrintMode_t\n" );
   	       break;
	    }
         }
      }
   }
   if( ok )
   {
      ok = ( this->pfileStream_ ? true : false );
   }
   return( ok );
}

//===========================================================================//
bool TIO_FileHandler_c::Open(
      const char*              pszFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t    printMode )
{
   string srFileName( TIO_PSZ_STR( pszFileName ));
   return( this->Open( srFileName, fileOpen, pszFileType, printMode ));
}

//===========================================================================//
// Method         : Close
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_FileHandler_c::Close( 
      void )
{
   if( this->pfileStream_ )
   {
      if( this->pfileStream_ != stdout )
      {
         fclose( this->pfileStream_ );
      }
      this->pfileStream_ = 0;
   }
}

//===========================================================================//
// Method         : Read
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileHandler_c::Read( 
      char*  pszString, 
      size_t lenString ) const
{
   bool ok = false;

   if( this->pfileStream_ && pszString && lenString )
   {
      int lenString_ = static_cast< int >( lenString );
      ok = ( fgets( pszString, lenString_, this->pfileStream_ ) ? true : false );
   }
   return( ok );
}

//===========================================================================//
// Method         : Write
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileHandler_c::Write( 
      const char* pszString,
      ... )
{
   bool ok = false;

   if( this->pfileStream_ && pszString )
   {
      va_list vaArgs;                      // Make a variable argument list
      va_start( vaArgs, pszString );       // Initialize variable argument list

      static char szString[TIO_FORMAT_STRING_LEN_MAX];
      vsprintf( szString, pszString, vaArgs );

      ok = ( fputs( szString, this->pfileStream_ ) >= 0 ? true : false );

      va_end( vaArgs );                    // Reset variable argument list
   }
   return( ok );
}

//===========================================================================//
// Method         : ApplyPreProcessor
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileHandler_c::ApplyPreProcessor( 
      void )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool isValidCommand = false;
   #if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 )
      TIO_FileHandler_c fileHandler;
      isValidCommand = fileHandler.IsValid( TIO_FILE_CPP_COMMAND, TIO_FILE_OPEN_READ );
   #elif defined( WIN32 ) || defined( _WIN32 )
      isValidCommand = true;
   #endif
   if( isValidCommand )
   {
      const string& srFileName = this->GetFileName( );
      string srLeafName( srFileName );

      size_t slashPos = srFileName.rfind( TIO_FILE_DIR_DELIMITER );
      if( slashPos != string::npos )
      {
         srLeafName = srFileName.substr( slashPos + 1 );
      }
   
      string srPreProcessedFileName;
      srPreProcessedFileName = ".";
      srPreProcessedFileName += TIO_FILE_DIR_DELIMITER;
      srPreProcessedFileName += TIO_FILE_HIDDEN_PREFIX;
      srPreProcessedFileName += srLeafName;

      string srCommand;
      srCommand += TIO_FILE_CPP_COMMAND;
      srCommand += " ";
      srCommand += TIO_FILE_CPP_OPTIONS;
      srCommand += " ";
      srCommand += srFileName;
      srCommand += " > ";
      srCommand += srPreProcessedFileName;

      int rc = system( srCommand.data( ));
      if( rc == 0 )
      {
         ok = this->Open( srPreProcessedFileName,
                          TIO_FILE_OPEN_READ,
                          "preprocessed file" );
      }
      else if( rc > 0 )
      {
         printHandler.Error( "Failed to complete %s preprocessor command.\n", 
		             TIO_PSZ_STR( TIO_FILE_CPP_COMMAND ));
         ok = false;
      }
      else if( rc < 0 )
      {
         printHandler.Error( "Failed to execute %s preprocessor command.\n", 
		             TIO_PSZ_STR( TIO_FILE_CPP_COMMAND ));
         ok = false;
      }
   }
   else
   {
      const string& srFileName = this->GetFileName( );
      ok = printHandler.Warning( "Failed to find macro preprocessor command \"%s\".\n"
                                 "%sAny macro directives in file '%s' will be ignored.\n",
                                 TIO_PSZ_STR( TIO_FILE_CPP_COMMAND ),
                                 TIO_PREFIX_WARNING_SPACE,
                                 TIO_SR_STR( srFileName ));
   }
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_FileHandler_c::IsValid(
      const string&            srFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t    printMode ) const
{
   TIO_FileHandler_c fileHandler( srFileName, fileOpen, pszFileType, printMode );
   return( fileHandler.IsValid( ));
}

//===========================================================================//
bool TIO_FileHandler_c::IsValid(
      const char*              pszFileName,
            TIO_FileOpenMode_t fileOpen,
      const char*              pszFileType,
            TIO_PrintMode_t  printMode ) const
{
   string srFileName( TIO_PSZ_STR( pszFileName ));
   return( this->IsValid( srFileName, fileOpen, pszFileType, printMode ));
} 

//===========================================================================//
bool TIO_FileHandler_c::IsValid( 
      void ) const
{
   return( this->pfileStream_ ? true : false );
}
