//===========================================================================//
// Purpose : Method definitions for the TOS_ControlSwitches class.
//
//           Public methods include:
//           - TOS_ControlSwitches_c, ~TOS_ControlSwitches_c
//           - Print
//           - Init
//           - Apply
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

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TOS_ControlSwitches.h"

//===========================================================================//
// Method         : TOS_ControlSwitches_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ControlSwitches_c::TOS_ControlSwitches_c( 
      void )
{
} 

//===========================================================================//
// Method         : ~TOS_ControlSwitches_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ControlSwitches_c::~TOS_ControlSwitches_c( 
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
void TOS_ControlSwitches_c::Print( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->Print( pfile, spaceLen );
}

//===========================================================================//
void TOS_ControlSwitches_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   char szSep[TIO_FORMAT_STRING_LEN_MAX];
   size_t lenSep = TIO_FORMAT_BANNER_LEN_DEF;

   TC_FormatStringFilled( '-', szSep, lenSep );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Input Options",   lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->inputOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Output Options",  lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->outputOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Message Options", lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->messageOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Execute Options", lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,             lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->executeOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ControlSwitches_c::Init(
      void )
{
   this->inputOptions.srXmlFileName = "";
   this->inputOptions.srBlifFileName = "";
   this->inputOptions.srArchitectureFileName = "";
   this->inputOptions.srFabricFileName = "";
   this->inputOptions.srCircuitFileName = "";

   this->inputOptions.xmlFileEnable = false;
   this->inputOptions.blifFileEnable = false;
   this->inputOptions.architectureFileEnable = false;
   this->inputOptions.fabricFileEnable = false;
   this->inputOptions.circuitFileEnable = false;

   this->inputOptions.prePackedDataMode = TOS_INPUT_DATA_ANY;
   this->inputOptions.prePlacedDataMode = TOS_INPUT_DATA_ANY;
   this->inputOptions.preRoutedDataMode = TOS_INPUT_DATA_ANY;

   this->outputOptions.srLogFileName = "";
   this->outputOptions.srOptionsFileName = "";
   this->outputOptions.srXmlFileName = "";
   this->outputOptions.srBlifFileName = "";
   this->outputOptions.srArchitectureFileName = "";
   this->outputOptions.srFabricFileName = "";
   this->outputOptions.srCircuitFileName = "";
   this->outputOptions.srRcDelaysFileName = "";
   this->outputOptions.srLaffFileName = "";
   this->outputOptions.srMetricsEmailAddress = "";

   this->outputOptions.srVPR_NetFileName = "";
   this->outputOptions.srVPR_PlaceFileName = "";
   this->outputOptions.srVPR_RouteFileName = "";
   this->outputOptions.srVPR_SDC_FileName = "";
   this->outputOptions.srVPR_OutputFilePrefix = "";

   this->outputOptions.logFileEnable = true;
   this->outputOptions.optionsFileEnable = false;
   this->outputOptions.xmlFileEnable = false;
   this->outputOptions.blifFileEnable = false;
   this->outputOptions.architectureFileEnable = false;
   this->outputOptions.fabricFileEnable = false;
   this->outputOptions.circuitFileEnable = false;
   this->outputOptions.rcDelaysFileEnable = false;
   this->outputOptions.laffFileEnable = false;
   this->outputOptions.metricsEmailEnable = false;

   this->outputOptions.laffMask = TOS_OUTPUT_LAFF_ALL;

   this->outputOptions.rcDelaysExtractMode = TOS_RC_DELAYS_EXTRACT_ELMORE;
   this->outputOptions.rcDelaysSortMode = TOS_RC_DELAYS_SORT_BY_NETS;
   this->outputOptions.rcDelaysNetNameList.Clear( );
   this->outputOptions.rcDelaysBlockNameList.Clear( );
   this->outputOptions.rcDelaysMaxWireLength = TC_FLT_MAX;

   this->messageOptions.minGridPrecision = 0.01;
   this->messageOptions.timeStampsEnable = false;

   this->messageOptions.info.acceptList.Clear( );
   this->messageOptions.info.rejectList.Clear( );
   this->messageOptions.warning.acceptList.Clear( );
   this->messageOptions.warning.rejectList.Clear( );
   this->messageOptions.error.acceptList.Clear( );
   this->messageOptions.error.rejectList.Clear( );
   this->messageOptions.trace.acceptList.Clear( );
   this->messageOptions.trace.rejectList.Clear( );

   this->messageOptions.trace.read.options = false;
   this->messageOptions.trace.read.xml = false;
   this->messageOptions.trace.read.blif = false;
   this->messageOptions.trace.read.architecture = false;
   this->messageOptions.trace.read.fabric = false;
   this->messageOptions.trace.read.circuit = false;

   this->messageOptions.trace.vpr.showSetup = false;
   this->messageOptions.trace.vpr.echoFile = false;
   this->messageOptions.trace.vpr.echoFileNameList.Clear( );

   this->executeOptions.maxWarningCount = UINT_MAX;
   this->executeOptions.maxErrorCount = 1;

   this->executeOptions.toolMask = TOS_EXECUTE_TOOL_ALL;

   this->srDefaultBaseName = "";
} 

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ControlSwitches_c::Apply(
      void )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   TOS_OutputOptions_c& outputOptions_ = this->outputOptions;
   TOS_MessageOptions_c& messageOptions_ = this->messageOptions;
   TOS_ExecuteOptions_c& executeOptions_ = this->executeOptions;

   // Initialize min (manufacturing) grid bsaed on applicable option
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   MinGrid.SetGrid( messageOptions_.minGridPrecision );

   // Initialize print handler based on any applicable options
   printHandler.SetTimeStampsEnabled( messageOptions_.timeStampsEnable );
   printHandler.SetMaxErrorCount( executeOptions_.maxErrorCount );
   printHandler.SetMaxWarningCount( executeOptions_.maxWarningCount );

   // If a log file has been setup do not set again
   // (assume a start banner has been handled by the owner program)
   if( !printHandler.IsLogFileOutputEnabled( ))
   {
      if( outputOptions_.logFileEnable && outputOptions_.srLogFileName.length( ))
      {
         printHandler.SetLogFileOutput( outputOptions_.srLogFileName.data( ));

	 printHandler.DisableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
	 printHandler.WriteBanner( );
	 printHandler.EnableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
      }
   }

   // Validate email metrics address is defined, if applicable
   if( outputOptions_.metricsEmailEnable && !outputOptions_.srMetricsEmailAddress.length( ))
   {
      this->outputOptions.metricsEmailEnable = false;
   }
   else if( !outputOptions_.metricsEmailEnable && outputOptions_.srMetricsEmailAddress.length( ))
   {
      this->outputOptions.metricsEmailEnable = true;
   }
}
