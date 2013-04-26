//===========================================================================//
// Purpose : Supporting methods for the TI_Input pre-processor class.  
//
//           Private methods include:
//           - DisplayProgramOwner_
//           - DisplayProgramCommand_
//           - BuildDefaultBaseName_
//           - ApplyDefaultBaseName_
//           - ReplaceWildCardName_
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TIO_StringText.h"
#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

#include "TI_Input.h"

//===========================================================================//
// Method         : DisplayProgramOwner_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::DisplayProgramOwner_( 
      void ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srUserName;
   printHandler.FindUserName( &srUserName );

   string srHostName;
   printHandler.FindHostName( &srHostName );

   if( srUserName.length( ) && srHostName.length( ))
   {
      printHandler.Info( "Owner is '%s' on host '%s'.\n",
                         TIO_SR_STR( srUserName ),
                         TIO_SR_STR( srHostName ));
   }
   else if( srUserName.length( ))
   {
      printHandler.Info( "Owner is '%s'.\n",
                         TIO_SR_STR( srUserName ));
   }
   else if( srHostName.length( ))
   {
      printHandler.Info( "Owner is on host '%s'.\n",
                         TIO_SR_STR( srHostName ));
   }
}

//===========================================================================//
// Method         : DisplayProgramCommand_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::DisplayProgramCommand_( 
      void ) const
{
   if( this->pcommandLine_ )
   {
      int argc = this->pcommandLine_->GetArgc( );
      if( argc > 1 )
      {
         char** pargv = this->pcommandLine_->GetArgv( );

         string srArgs = "";
         for( int i = 0; i < argc; ++i )
         {
            srArgs += pargv[i] ;
            srArgs += ( i < argc - 1 ? " " : "" );
         }

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         printHandler.Info( "Command is '%s'.\n", 
                            TIO_SR_STR( srArgs ));
      }
   }
}

//===========================================================================//
// Method         : BuildDefaultBaseName_
// Purpose        : Extract the default base name for all input files based 
//                  on the options file name minus any dot-seperated suffix.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::BuildDefaultBaseName_(
      const string& srOptionsFileName,
            string* psrDefaultBaseName ) const
{
   // Use the given options file name to build a default base name
   *psrDefaultBaseName = srOptionsFileName;

   // Strip options file name extension, if any
   if( psrDefaultBaseName->length( ) >= 1 )
   {
      size_t i = psrDefaultBaseName->rfind( '.' );
      if( i != string::npos )
      {
         psrDefaultBaseName->erase( i );
      }
   }

   // Strip options file "binary" name too, if any
   if( psrDefaultBaseName->length( ) >= 1 )
   {
      TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
      string srBinaryName = skinHandler.GetBinaryName( );
      size_t lenBinaryName = srBinaryName.length( );

      size_t i = psrDefaultBaseName->rfind( '.' );
      if( i != string::npos && 
          psrDefaultBaseName->substr( i + 1, lenBinaryName ) == srBinaryName )
      {
         psrDefaultBaseName->erase( i );
      }
   }
}

//===========================================================================//
// Method         : ApplyDefaultBaseName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::ApplyDefaultBaseName_( 
      const string& srDefaultBaseName ) const
{
   TOS_ControlSwitches_c* pcontrolSwitches = &this->poptionsStore_->controlSwitches;
   TOS_InputOptions_c* pinputOptions = &pcontrolSwitches->inputOptions;
   TOS_OutputOptions_c* poutputOptions = &pcontrolSwitches->outputOptions;
   TOS_MessageOptions_c* pmessageOptions = &pcontrolSwitches->messageOptions;

   // For each undefined file name, apply default file name and extension
   this->ApplyDefaultBaseName_( &pinputOptions->srXmlFileName,
                                srDefaultBaseName, TIO_SZ_INPUT_XML_DEF_EXT );
   this->ApplyDefaultBaseName_( &pinputOptions->srBlifFileName,
                                srDefaultBaseName, TIO_SZ_INPUT_BLIF_DEF_EXT );
   this->ApplyDefaultBaseName_( &pinputOptions->srArchitectureFileName,
                                srDefaultBaseName, TIO_SZ_INPUT_ARCHITECTURE_DEF_EXT );
   this->ApplyDefaultBaseName_( &pinputOptions->srFabricFileName,
                                srDefaultBaseName, TIO_SZ_INPUT_FABRIC_DEF_EXT );
   this->ApplyDefaultBaseName_( &pinputOptions->srCircuitFileName,
                                srDefaultBaseName, TIO_SZ_INPUT_CIRCUIT_DEF_EXT );

   this->ApplyDefaultBaseName_( &poutputOptions->srLogFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_LOG_DEF_EXT );
   if( poutputOptions->rcDelaysFileEnable )
   {
      this->ApplyDefaultBaseName_( &poutputOptions->srRcDelaysFileName,
                                   srDefaultBaseName, TIO_SZ_OUTPUT_RC_DELAYS_DEF_EXT );
   }
   this->ApplyDefaultBaseName_( &poutputOptions->srOptionsFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_OPTIONS_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srXmlFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_XML_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srBlifFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_BLIF_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srArchitectureFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_ARCHITECTURE_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srFabricFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_FABRIC_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srCircuitFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_CIRCUIT_DEF_EXT );
   this->ApplyDefaultBaseName_( &poutputOptions->srLaffFileName,
                                srDefaultBaseName, TIO_SZ_OUTPUT_LAFF_DEF_EXT );

   for( size_t i = 0; i < pmessageOptions->trace.vpr.echoFileNameList.GetLength( ); ++i )
   {
      TC_NameFile_c* pechoFileName = pmessageOptions->trace.vpr.echoFileNameList[i];
      if( !pechoFileName->GetFileName( ) || !*pechoFileName->GetFileName( ))
         continue;
   
      string srEchoName( pechoFileName->GetName( ));
      string srFileName( pechoFileName->GetFileName( ));
      this->ApplyDefaultBaseName_( &srFileName, 
                                   srDefaultBaseName );
      pechoFileName->Set( srEchoName, srFileName );
   }
}

//===========================================================================//
void TI_Input_c::ApplyDefaultBaseName_( 
            string* psrFileName,
      const string& srDefaultBaseName ) const
{
   if( srDefaultBaseName.length( ))
   { 
      string srFileName( *psrFileName );
      string srDefaultExtName;

      this->ApplyDefaultBaseName_( srFileName, &srFileName,
                                   srDefaultBaseName, srDefaultExtName );
      *psrFileName = srFileName;
   }
}

//===========================================================================//
void TI_Input_c::ApplyDefaultBaseName_( 
            string* psrFileName,
      const string& srDefaultBaseName,
      const char*   pszDefaultExtName ) const
{
   if( srDefaultBaseName.length( ))
   { 
      string srFileName( *psrFileName );
      string srDefaultExtName( pszDefaultExtName );

      this->ApplyDefaultBaseName_( srFileName, &srFileName,
                                   srDefaultBaseName, srDefaultExtName );
      *psrFileName = srFileName;
   }
}

//===========================================================================//
void TI_Input_c::ApplyDefaultBaseName_( 
      const string& srFileName,
            string* psrFileName,
      const string& srDefaultBaseName,
      const string& srDefaultExtName ) const
{
   if( !srFileName.length( ))
   {
      // No user-specified file name, use default name and extension instead
      *psrFileName = srDefaultBaseName;
      if( srDefaultExtName.length( ))
      {
         TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
         string srBinaryName = skinHandler.GetBinaryName( );

         *psrFileName += ".";
         *psrFileName += srBinaryName;

         *psrFileName += ".";
         *psrFileName += srDefaultExtName;   
      }
   }
   else if(( srFileName.length( ) == 1 ) && ( srFileName.find( '*' ) != string::npos ))
   {
      // User-specified file name is a wildcard, use default 'stdout' output
      string srReplaceName( "stdout" );
      this->ReplaceWildCardName_( srFileName, psrFileName,
                                  "*", srReplaceName );
   }
   else if(( srFileName.length( ) >= 2 ) && ( srFileName.find( '*' ) != string::npos ))
   {
      // User-specified file name has a wildcard, use default for wildcard
      this->ReplaceWildCardName_( srFileName, psrFileName, 
                                  "*", srDefaultBaseName );
   }
   else
   {
      // User-specified file name given, just use it
      *psrFileName = srFileName;
   }
}

//===========================================================================//
// Method         : ReplaceWildCardName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::ReplaceWildCardName_( 
      const string& srFileName,
            string* psrFileName,
      const char*   pszWildCardName,
      const string& srReplaceName ) const
{
   *psrFileName = srFileName;

   string srWildCardName( TIO_PSZ_STR( pszWildCardName ));
   for( size_t i = psrFileName->find( srWildCardName, 0 ); 
        i != string::npos; 
        i = psrFileName->find( srWildCardName, i+1 ))
   {
      psrFileName->replace( i, srWildCardName.length( ), srReplaceName );
   }
}
