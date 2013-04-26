//===========================================================================//
// Purpose : Method definitions for the TOS_OutputOptions class.
//
//           Public methods include:
//           - TOS_OutputOptions_c, ~TOS_OutputOptions_c
//           - Print
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_OutputOptions.h"

//===========================================================================//
// Method         : TOS_OutputOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OutputOptions_c::TOS_OutputOptions_c( 
      void )
      :
      srLogFileName( "" ),
      srOptionsFileName( "" ),
      srXmlFileName( "" ),
      srBlifFileName( "" ),
      srArchitectureFileName( "" ),
      srFabricFileName( "" ),
      srCircuitFileName( "" ),
      srRcDelaysFileName( "" ),
      srLaffFileName( "" ),
      srMetricsEmailAddress( "" ),
      srVPR_NetFileName( "" ),
      srVPR_PlaceFileName( "" ),
      srVPR_RouteFileName( "" ),
      srVPR_SDC_FileName( "" ),
      srVPR_OutputFilePrefix( "" ),
      logFileEnable( false ),
      optionsFileEnable( false ),
      xmlFileEnable( false ),
      blifFileEnable( false ),
      architectureFileEnable( false ),
      fabricFileEnable( false ),
      circuitFileEnable( false ),
      rcDelaysFileEnable( false ),
      laffFileEnable( false ),
      metricsEmailEnable( false ),
      laffMask( TOS_OUTPUT_LAFF_UNDEFINED ),
      rcDelaysExtractMode( TOS_RC_DELAYS_EXTRACT_UNDEFINED ),
      rcDelaysSortMode( TOS_RC_DELAYS_SORT_UNDEFINED ),
      rcDelaysNetNameList( TOS_RC_DELAYS_NET_NAME_LIST_DEF_CAPACITY ),
      rcDelaysBlockNameList( TOS_RC_DELAYS_BLOCK_NAME_LIST_DEF_CAPACITY ),
      rcDelaysMaxWireLength( TC_FLT_MAX )
{
}

//===========================================================================//
TOS_OutputOptions_c::TOS_OutputOptions_c( 
      const string&                   srLogFileName_,
      const string&                   srOptionsFileName_,
      const string&                   srXmlFileName_,
      const string&                   srBlifFileName_,
      const string&                   srArchitectureFileName_,
      const string&                   srFabricFileName_,
      const string&                   srCircuitFileName_,
      const string&                   srRcDelaysFileName_,
      const string&                   srLaffFileName_,
      const string&                   srMetricsEmailAddress_,
            bool                      logFileEnable_,
            bool                      optionsFileEnable_,
            bool                      xmlFileEnable_,
            bool                      blifFileEnable_,
            bool                      architectureFileEnable_,
            bool                      fabricFileEnable_,
            bool                      circuitFileEnable_,
            bool                      rcDelaysFileEnable_,
            bool                      laffFileEnable_,
            bool                      metricsEmailEnable_,
            int                       laffMask_,
            TOS_RcDelaysExtractMode_t rcDelaysExtractMode_,
            TOS_RcDelaysSortMode_t    rcDelaysSortMode_,
      const TOS_RcDelaysNameList_t&   rcDelaysNetNameList_,
      const TOS_RcDelaysNameList_t&   rcDelaysBlockNameList_,
            double                    rcDelaysMaxWireLength_ )
      :
      srLogFileName( srLogFileName_ ),
      srOptionsFileName( srOptionsFileName_ ),
      srXmlFileName( srXmlFileName_ ),
      srBlifFileName( srBlifFileName_ ),
      srArchitectureFileName( srArchitectureFileName_ ),
      srFabricFileName( srFabricFileName_ ),
      srCircuitFileName( srCircuitFileName_ ),
      srRcDelaysFileName( srRcDelaysFileName_ ),
      srLaffFileName( srLaffFileName_ ),
      srMetricsEmailAddress( srMetricsEmailAddress_ ),
      srVPR_NetFileName( "" ),
      srVPR_PlaceFileName( "" ),
      srVPR_RouteFileName( "" ),
      srVPR_SDC_FileName( "" ),
      srVPR_OutputFilePrefix( "" ),
      logFileEnable( logFileEnable_ ),
      optionsFileEnable( optionsFileEnable_ ),
      xmlFileEnable( xmlFileEnable_ ),
      blifFileEnable( blifFileEnable_ ),
      architectureFileEnable( architectureFileEnable_ ),
      fabricFileEnable( fabricFileEnable_ ),
      circuitFileEnable( circuitFileEnable_ ),
      rcDelaysFileEnable( rcDelaysFileEnable_ ),
      laffFileEnable( laffFileEnable_ ),
      metricsEmailEnable( metricsEmailEnable_ ),
      laffMask( laffMask_ ),
      rcDelaysExtractMode( rcDelaysExtractMode_ ),
      rcDelaysSortMode( rcDelaysSortMode_ ),
      rcDelaysNetNameList( rcDelaysNetNameList_ ),
      rcDelaysBlockNameList( rcDelaysBlockNameList_ ),
      rcDelaysMaxWireLength( rcDelaysMaxWireLength_ )
{
}

//===========================================================================//
// Method         : ~TOS_OutputOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OutputOptions_c::~TOS_OutputOptions_c( 
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
void TOS_OutputOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srLaffMask;
   TOS_ExtractStringOutputLaffMask( this->laffMask, &srLaffMask );

   string srRcDelaysExtractMode, srRcDelaysSortMode, srRcDelaysNetNameList, srRcDelaysBlockNameList;
   TOS_ExtractStringRcDelaysExtractMode( this->rcDelaysExtractMode, &srRcDelaysExtractMode );
   TOS_ExtractStringRcDelaysSortMode( this->rcDelaysSortMode, &srRcDelaysSortMode );
   this->rcDelaysNetNameList.ExtractString( &srRcDelaysNetNameList );
   this->rcDelaysBlockNameList.ExtractString( &srRcDelaysBlockNameList );

   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_LOG            = \"%s\"\n", TIO_SR_STR( this->srLogFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_OPTIONS        = \"%s\"\n", TIO_SR_STR( this->srOptionsFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_XML            = \"%s\"\n", TIO_SR_STR( this->srXmlFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_BLIF           = \"%s\"\n", TIO_SR_STR( this->srBlifFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_ARCH           = \"%s\"\n", TIO_SR_STR( this->srArchitectureFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_FABRIC         = \"%s\"\n", TIO_SR_STR( this->srFabricFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_CIRCUIT        = \"%s\"\n", TIO_SR_STR( this->srCircuitFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_RC_DELAYS      = \"%s\"\n", TIO_SR_STR( this->srRcDelaysFileName ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_FILE_LAFF           = \"%s\"\n", TIO_SR_STR( this->srLaffFileName ));

   printHandler.Write( pfile, spaceLen, "OUTPUT_EMAIL_METRICS       = \"%s\"\n", TIO_SR_STR( this->srMetricsEmailAddress ));

   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_LOG          = %s\n", TIO_BOOL_STR( this->logFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_OPTIONS      = %s\n", TIO_BOOL_STR( this->optionsFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_XML          = %s\n", TIO_BOOL_STR( this->xmlFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_BLIF         = %s\n", TIO_BOOL_STR( this->blifFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_ARCH         = %s\n", TIO_BOOL_STR( this->architectureFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_FABRIC       = %s\n", TIO_BOOL_STR( this->fabricFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_CIRCUIT      = %s\n", TIO_BOOL_STR( this->circuitFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_RC_DELAYS    = %s\n", TIO_BOOL_STR( this->rcDelaysFileEnable ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_ENABLE_LAFF         = %s\n", TIO_BOOL_STR( this->laffFileEnable ));

   printHandler.Write( pfile, spaceLen, "OUTPUT_LAFF_MODE           = %s\n", TIO_SR_STR( srLaffMask ));

   printHandler.Write( pfile, spaceLen, "OUTPUT_RC_DELAY_MODE       = %s\n", TIO_SR_STR( srRcDelaysExtractMode ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_RC_DELAY_SORT       = %s\n", TIO_SR_STR( srRcDelaysSortMode ));
   printHandler.Write( pfile, spaceLen, "OUTPUT_RC_DELAY_NETS       = { %s%s}\n", TIO_SR_STR( srRcDelaysNetNameList ),
                                                                                  srRcDelaysNetNameList.length( ) ? " " : "" );
   printHandler.Write( pfile, spaceLen, "OUTPUT_RC_DELAY_BLOCKS     = { %s%s}\n", TIO_SR_STR( srRcDelaysBlockNameList ),
                                                                                  srRcDelaysBlockNameList.length( ) ? " " : "" );
   if( TCTF_IsLT( this->rcDelaysMaxWireLength, TC_FLT_MAX ))
   {
      printHandler.Write( pfile, spaceLen, "OUTPUT_RC_DELAY_MAX_LEN    = %0.*f\n", precision, this->rcDelaysMaxWireLength );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// OUTPUT_RC_DELAY_MAX_LEN = *\n" );
   }
}
