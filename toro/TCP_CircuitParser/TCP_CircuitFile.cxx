//===========================================================================//
// Purpose : Method definitions for the TCP_CircuitFile class.
//
//           Public methods include:
//           - TCP_CircuitFile, ~TCP_CircuitFile
//           - SyntaxError
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
            TCP_CircuitInterface_c* pcircuitInterface,
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

   // Define a pointer to the default interface handler class
   circuitParser.SetInterface( pcircuitInterface );

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
