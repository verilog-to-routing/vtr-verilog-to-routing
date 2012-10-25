//===========================================================================//
// Purpose : Method definitions for the TOS_InputOptions class.
//
//           Public methods include:
//           - TOS_InputOptions_c, ~TOS_InputOptions_c
//           - Print
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

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_InputOptions.h"

//===========================================================================//
// Method         : TOS_InputOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_InputOptions_c::TOS_InputOptions_c( 
      void )
      :
      optionsFileNameList( TOS_OPTIONS_FILE_NAME_LIST_DEF_CAPACITY ),
      srXmlFileName( "" ),
      srBlifFileName( "" ),
      srArchitectureFileName( "" ),
      srFabricFileName( "" ),
      srCircuitFileName( "" ),
      xmlFileEnable( false ),
      blifFileEnable( false ),
      architectureFileEnable( false ),
      fabricFileEnable( false ),
      circuitFileEnable( false ),
      prePackedDataMode( TOS_INPUT_DATA_UNDEFINED ),
      prePlacedDataMode( TOS_INPUT_DATA_UNDEFINED ),
      preRoutedDataMode( TOS_INPUT_DATA_UNDEFINED )
{
}

//===========================================================================//
TOS_InputOptions_c::TOS_InputOptions_c( 
      const TOS_OptionsNameList_t& optionsFileNameList_,
      const string&                srXmlFileName_,
      const string&                srBlifFileName_,
      const string&                srArchitectureFileName_,
      const string&                srFabricFileName_,
      const string&                srCircuitFileName_,
            bool                   xmlFileEnable_,
            bool                   blifFileEnable_,
            bool                   architectureFileEnable_,
            bool                   fabricFileEnable_,
            bool                   circuitFileEnable_,
            TOS_InputDataMode_t    prePackedDataMode_,
            TOS_InputDataMode_t    prePlacedDataMode_,
            TOS_InputDataMode_t    preRoutedDataMode_ )
      :
      optionsFileNameList( optionsFileNameList_ ),
      srXmlFileName( srXmlFileName_ ),
      srBlifFileName( srBlifFileName_ ),
      srArchitectureFileName( srArchitectureFileName_ ),
      srFabricFileName( srFabricFileName_ ),
      srCircuitFileName( srCircuitFileName_ ),
      xmlFileEnable( xmlFileEnable_ ),
      blifFileEnable( blifFileEnable_ ),
      architectureFileEnable( architectureFileEnable_ ),
      fabricFileEnable( fabricFileEnable_ ),
      circuitFileEnable( circuitFileEnable_ ),
      prePackedDataMode( prePackedDataMode_ ),
      prePlacedDataMode( prePlacedDataMode_ ),
      preRoutedDataMode( preRoutedDataMode_ )
{
}

//===========================================================================//
// Method         : ~TOS_InputOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_InputOptions_c::~TOS_InputOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_InputOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srPrePackedDataMode, srPrePlacedDataMode, srPreRoutedDataMode;
   TOS_ExtractStringInputDataMode( this->prePackedDataMode, &srPrePackedDataMode );
   TOS_ExtractStringInputDataMode( this->prePlacedDataMode, &srPrePlacedDataMode );
   TOS_ExtractStringInputDataMode( this->preRoutedDataMode, &srPreRoutedDataMode );

   if( this->srXmlFileName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "INPUT_FILE_XML             = \"%s\"\n", TIO_SR_STR( this->srXmlFileName ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// INPUT_FILE_XML          = \"\"\n" );
   }
   if( this->srBlifFileName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "INPUT_FILE_BLIF            = \"%s\"\n", TIO_SR_STR( this->srBlifFileName ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// INPUT_FILE_BLIF         = \"\"\n" );
   }

   if( this->srArchitectureFileName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "INPUT_FILE_ARCH            = \"%s\"\n", TIO_SR_STR( this->srArchitectureFileName ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// INPUT_FILE_ARCH         = \"\"\n" );
   }
   if( this->srFabricFileName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "INPUT_FILE_FABRIC          = \"%s\"\n", TIO_SR_STR( this->srFabricFileName ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// INPUT_FILE_FABRIC       = \"\"\n" );
   }
   if( this->srCircuitFileName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "INPUT_FILE_CIRCUIT         = \"%s\"\n", TIO_SR_STR( this->srCircuitFileName ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// INPUT_FILE_CIRCUIT      = \"\"\n" );
   }

   printHandler.Write( pfile, spaceLen, "INPUT_ENABLE_ARCH          = %s\n", TIO_BOOL_STR( this->xmlFileEnable ));
   printHandler.Write( pfile, spaceLen, "INPUT_ENABLE_BLIF          = %s\n", TIO_BOOL_STR( this->blifFileEnable ));

   printHandler.Write( pfile, spaceLen, "INPUT_ENABLE_ARCH          = %s\n", TIO_BOOL_STR( this->architectureFileEnable ));
   printHandler.Write( pfile, spaceLen, "INPUT_ENABLE_FABRIC        = %s\n", TIO_BOOL_STR( this->fabricFileEnable ));
   printHandler.Write( pfile, spaceLen, "INPUT_ENABLE_CIRCUIT       = %s\n", TIO_BOOL_STR( this->circuitFileEnable ));

   printHandler.Write( pfile, spaceLen, "INPUT_DATA_PREPACKED       = %s\n", TIO_SR_STR( srPrePackedDataMode ));
   printHandler.Write( pfile, spaceLen, "INPUT_DATA_PREPLACED       = %s\n", TIO_SR_STR( srPrePlacedDataMode ));
   printHandler.Write( pfile, spaceLen, "INPUT_DATA_PREROUTED       = %s\n", TIO_SR_STR( srPreRoutedDataMode ));
}
