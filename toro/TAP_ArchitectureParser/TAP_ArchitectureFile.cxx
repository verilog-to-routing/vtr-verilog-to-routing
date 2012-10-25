//===========================================================================//
// Purpose : Method definitions for the TAP_ArchitectureFile class.
//
//           Public methods include:
//           - TAP_ArchitectureFile, ~TAP_ArchitectureFile
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

#include "TAP_ArchitectureScanner_c.h"
#include "TAP_ArchitectureParser_c.h"
#include "TAP_ArchitectureFile.h"

//===========================================================================//
// Method         : TAP_ArchitectureFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAP_ArchitectureFile_c::TAP_ArchitectureFile_c( 
            FILE*                        pfile,    
      const char*                        pszFileName,
            TAP_ArchitectureInterface_c* parchitectureInterface,
            TAS_ArchitectureSpec_c*      parchitectureSpec )
{
   this->ok_ = true;

   // Define an input file stream for DLG
   DLGFileInput inputFileStream( pfile );

   // Define a scanner attached to the file stream
   TAP_ArchitectureScanner_c architectureScanner( &inputFileStream );

   // Define a token buffer attached to the scanner
   GenericTokenBuffer tokenBuffer( &architectureScanner );

   // Generate and pass a token to the scanner
   architectureScanner.setToken( new TC_NOTHROW ANTLRToken );

   // Define a token parser attached to token buffer
   TAP_ArchitectureParser_c architectureParser( &tokenBuffer );

   // Define a pointer to the default interface handler class
   architectureParser.SetInterface( parchitectureInterface );

   // Define a pointer to the default scanner class
   architectureParser.SetScanner( &architectureScanner );

   // Define input file name (for print messages)
   architectureParser.SetFileName( pszFileName );

   // Define a pointer to the current architecture file parser class
   architectureParser.SetArchitectureFile( this );

   // Define a pointer to the given architecture spec class
   architectureParser.SetArchitectureSpec( parchitectureSpec );

   // Initialize and start the parser
   architectureParser.init();
   architectureParser.start();
}

//===========================================================================//
// Method         : ~TAP_ArchitectureFile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAP_ArchitectureFile_c::~TAP_ArchitectureFile_c( 
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
bool TAP_ArchitectureFile_c::SyntaxError( 
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
