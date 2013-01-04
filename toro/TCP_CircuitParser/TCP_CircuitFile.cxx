//===========================================================================//
// Purpose : Method definitions for the TCP_CircuitFile class.
//
//           Public methods include:
//           - TCP_CircuitFile, ~TCP_CircuitFile
//           - SyntaxError
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
