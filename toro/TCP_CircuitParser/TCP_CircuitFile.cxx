//===========================================================================//
// Purpose : Method definitions for the TCP_CircuitFile class.
//
//           Public methods include:
//           - TCP_CircuitFile, ~TCP_CircuitFile
//           - SyntaxError
//
//===========================================================================//

#include "stdpccts.h"
#include "ATokPtr.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TCP_CircuitScanner_c.h"
#include "TCP_CircuitParser_c.h"
#include "TCP_CircuitFile.h"

//===========================================================================//
// Method         : TCP_CircuitFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TCP_CircuitFile_c::TCP_CircuitFile_c( 
            FILE*                   pfile,    
      const char*                   pszFileName,
            TCD_CircuitDesign_c*    pcircuitDesign )
{
   this->ok_ = true;

   // Define an input file stream for DLG
   DLGFileInput inputFileStream( pfile );

   // Define a scanner attached to the file stream
   TCP_CircuitScanner_c circuitScanner( &inputFileStream );

   // Define a token buffer attached to the scanner
   GenericTokenBuffer tokenBuffer( &circuitScanner );

   // Generate and pass a token to the scanner
   circuitScanner.setToken( new TC_NOTHROW ANTLRToken );

   // Define a token parser attached to token buffer
   TCP_CircuitParser_c circuitParser( &tokenBuffer );

   // Define a pointer to the default scanner class
   circuitParser.SetScanner( &circuitScanner );

   // Define input file name (for print messages)
   circuitParser.SetFileName( pszFileName );

   // Define a pointer to the current circuit file parser class
   circuitParser.SetCircuitFile( this );

   // Define a pointer to the given circuit design class
   circuitParser.SetCircuitDesign( pcircuitDesign );

   // Initialize and start the parser
   circuitParser.init();
   circuitParser.start();
}

//===========================================================================//
// Method         : ~TCP_CircuitFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TCP_CircuitFile_c::~TCP_CircuitFile_c( 
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
bool TCP_CircuitFile_c::SyntaxError( 
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
