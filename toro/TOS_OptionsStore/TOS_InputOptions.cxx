//===========================================================================//
// Purpose : Method definitions for the TOS_InputOptions class.
//
//           Public methods include:
//           - TOS_InputOptions_c, ~TOS_InputOptions_c
//           - Print
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
      circuitFileEnable( false )
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
            bool                   circuitFileEnable_ )
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
      circuitFileEnable( circuitFileEnable_ )
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
}
