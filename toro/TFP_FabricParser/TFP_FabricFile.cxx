//===========================================================================//
// Purpose : Method definitions for the TFP_FabricFile class.
//
//           Public methods include:
//           - TFP_FabricFile, ~TFP_FabricFile
//           - SyntaxError
//
//===========================================================================//

#include "stdpccts.h"
#include "ATokPtr.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TFP_FabricScanner_c.h"
#include "TFP_FabricParser_c.h"
#include "TFP_FabricFile.h"

//===========================================================================//
// Method         : TFP_FabricFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TFP_FabricFile_c::TFP_FabricFile_c( 
            FILE*                  pfile,    
      const char*                  pszFileName,
            TFP_FabricInterface_c* pfabricInterface,
            TFM_FabricModel_c*     pfabricModel )
{
   this->ok_ = true;

   // Define an input file stream for DLG
   DLGFileInput inputFileStream( pfile );

   // Define a scanner attached to the file stream
   TFP_FabricScanner_c fabricScanner( &inputFileStream );

   // Define a token buffer attached to the scanner
   GenericTokenBuffer tokenBuffer( &fabricScanner );

   // Generate and pass a token to the scanner
   fabricScanner.setToken( new TC_NOTHROW ANTLRToken );

   // Define a token parser attached to token buffer
   TFP_FabricParser_c fabricParser( &tokenBuffer );

   // Define a pointer to the default interface handler class
   fabricParser.SetInterface( pfabricInterface );

   // Define a pointer to the default scanner class
   fabricParser.SetScanner( &fabricScanner );

   // Define input file name (for print messages)
   fabricParser.SetFileName( pszFileName );

   // Define a pointer to the current fabric file parser class
   fabricParser.SetFabricFile( this );

   // Define a pointer to the given fabric model class
   fabricParser.SetFabricModel( pfabricModel );

   // Initialize and start the parser
   fabricParser.init();
   fabricParser.start();
}

//===========================================================================//
// Method         : ~TFP_FabricFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TFP_FabricFile_c::~TFP_FabricFile_c( 
      void )
{
}

//===========================================================================//
// Method         : SyntaxError
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TFP_FabricFile_c::SyntaxError( 
            unsigned int lineNum, 
      const string&      srFileName,
      const char*        pszMessageText )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   this->ok_ = printHandler.Error( "Syntax error %s.\n"
                                   "%sSee file '%s', line %d.\n",
                                   TIO_PSZ_STR( pszMessageText ),
                                   TIO_PREFIX_ERROR_SPACE,
                                   TIO_SR_STR( srFileName ), lineNum );
   return( this->ok_ );
}
