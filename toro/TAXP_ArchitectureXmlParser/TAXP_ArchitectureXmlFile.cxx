//===========================================================================//
// Purpose : Method definitions for the TAXP_ArchitectureXmlFile class.
//
//           Public methods include:
//           - TAXP_ArchitectureXmlFile, ~TAXP_ArchitectureXmlFile
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

#include "TAXP_ArchitectureXmlScanner_c.h"
#include "TAXP_ArchitectureXmlParser_c.h"
#include "TAXP_ArchitectureXmlFile.h"

//===========================================================================//
// Method         : TAXP_ArchitectureXmlFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAXP_ArchitectureXmlFile_c::TAXP_ArchitectureXmlFile_c( 
            FILE*                            pfile,    
      const char*                            pszFileName,
            TAXP_ArchitectureXmlInterface_c* parchitectureXmlInterface,
            TAS_ArchitectureSpec_c*          parchitectureSpec )
{
   this->ok_ = true;

   // Define an input file stream for DLG
   DLGFileInput inputFileStream( pfile );

   // Define a scanner attached to the file stream
   TAXP_ArchitectureXmlScanner_c architectureXmlScanner( &inputFileStream );

   // Define a token buffer attached to the scanner
   GenericTokenBuffer tokenBuffer( &architectureXmlScanner );

   // Generate and pass a token to the scanner
   architectureXmlScanner.setToken( new TC_NOTHROW ANTLRToken );

   // Define a token parser attached to token buffer
   TAXP_ArchitectureXmlParser_c architectureXmlParser( &tokenBuffer );

   // Define a pointer to the default interface handler class
   architectureXmlParser.SetInterface( parchitectureXmlInterface );

   // Define a pointer to the default scanner class
   architectureXmlParser.SetScanner( &architectureXmlScanner );

   // Define input file name (for print messages)
   architectureXmlParser.SetFileName( pszFileName );

   // Define a pointer to the current architecture file parser class
   architectureXmlParser.SetArchitectureXmlFile( this );

   // Define a pointer to the given architecture spec class
   architectureXmlParser.SetArchitectureSpec( parchitectureSpec );

   // Initialize and start the parser
   architectureXmlParser.init();
   architectureXmlParser.start();
}

//===========================================================================//
// Method         : ~TAXP_ArchitectureXmlFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAXP_ArchitectureXmlFile_c::~TAXP_ArchitectureXmlFile_c( 
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
bool TAXP_ArchitectureXmlFile_c::SyntaxError( 
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
