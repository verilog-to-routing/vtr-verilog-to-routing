//===========================================================================//
// Purpose : Method definitions for the TCBP_CircuitBlifFile class.
//
//           Public methods include:
//           - TCBP_CircuitBlifFile, ~TCBP_CircuitBlifFile
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

#include "TCBP_CircuitBlifScanner_c.h"
#include "TCBP_CircuitBlifParser_c.h"
#include "TCBP_CircuitBlifFile.h"

//===========================================================================//
// Method         : TCBP_CircuitBlifFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifFile_c::TCBP_CircuitBlifFile_c( 
            FILE*                pfile,    
      const char*                pszFileName,
            TCD_CircuitDesign_c* pcircuitDesign )
{
   this->ok_ = true;

   // Define an input file stream for DLG
   DLGFileInput inputFileStream( pfile );

   // Define a scanner attached to the file stream
   TCBP_CircuitBlifScanner_c circuitBlifScanner( &inputFileStream );

   // Define a token buffer attached to the scanner
   GenericTokenBuffer tokenBuffer( &circuitBlifScanner );

   // Generate and pass a token to the scanner
   circuitBlifScanner.setToken( new TC_NOTHROW ANTLRToken );

   // Define a token parser attached to token buffer
   TCBP_CircuitBlifParser_c circuitBlifParser( &tokenBuffer );

   // Define a pointer to the default scanner class
   circuitBlifParser.SetScanner( &circuitBlifScanner );

   // Define input file name (for print messages)
   circuitBlifParser.SetFileName( pszFileName );

   // Define a pointer to the current circuit file parser class
   circuitBlifParser.SetCircuitBlifFile( this );

   // Define a pointer to the given circuit design class
   circuitBlifParser.SetCircuitDesign( pcircuitDesign );

   // Initialize and start the parser
   circuitBlifParser.init();
   circuitBlifParser.start();
}

//===========================================================================//
// Method         : ~TCBP_CircuitBlifFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifFile_c::~TCBP_CircuitBlifFile_c( 
      void )
{
}

//===========================================================================//
// Method         : SyntaxError
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCBP_CircuitBlifFile_c::SyntaxError( 
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
