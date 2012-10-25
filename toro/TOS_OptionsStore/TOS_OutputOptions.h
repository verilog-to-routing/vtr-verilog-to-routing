//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_OutputOptions 
//           class.
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

#ifndef TOS_OUTPUT_OPTIONS_H
#define TOS_OUTPUT_OPTIONS_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_Name.h"

#include "TOS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_OutputOptions_c
{
public:

   TOS_OutputOptions_c( void );
   TOS_OutputOptions_c( const string& srLogFileName,
                        const string& srOptionsFileName,
                        const string& srXmlFileName,
                        const string& srBlifFileName,
                        const string& srArchitectureFileName,
                        const string& srFabricFileName,
                        const string& srCircuitFileName,
                        const string& srRcDelaysFileName,
                        const string& srLaffFileName,
                        const string& srMetricsEmailAddress,
                        bool logFileEnable,
                        bool optionsFileEnable,
                        bool xmlFileEnable,
                        bool blifFileEnable,
                        bool architectureFileEnable,
                        bool fabricFileEnable,
                        bool circuitFileEnable,
                        bool rcDelaysFileEnable,
                        bool laffFileEnable,
                        bool metricsEmailEnable,
                        int laffMask,
			TOS_RcDelaysExtractMode_t rcDelaysExtractMode,
			TOS_RcDelaysSortMode_t rcDelaysSortMode,
			const TOS_RcDelaysNameList_t& rcDelaysNetNameList,
			const TOS_RcDelaysNameList_t& rcDelaysBlockNameList,
			double rcDelaysMaxWireLength );
   ~TOS_OutputOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   string srLogFileName;             // Log file name for program output
   string srOptionsFileName;         // Options file name for program inspection
   string srXmlFileName;             // VPR's XML file name for program inspection
   string srBlifFileName;            // VPR's BLIF file name for program inspection
   string srArchitectureFileName;    // Architecture file name for program inspection
   string srFabricFileName;          // Fabric file name for pack/place/route
   string srCircuitFileName;         // Circuit file name for program inspection
   string srRcDelaysFileName;        // RC delays file name for data analysis
   string srLaffFileName;            // LAFF file name for data visualization
   string srMetricsEmailAddress;     // Metrics Email address for program usage

   string srVPR_NetFileName;         // VPR-specific file names (command-line)
   string srVPR_PlaceFileName;       // "
   string srVPR_RouteFileName;       // "
   string srVPR_SDC_FileName;        // "
   string srVPR_OutputFilePrefix;    // "

   bool logFileEnable;               // Enable writing output files
   bool optionsFileEnable;           // "
   bool xmlFileEnable;               // "
   bool blifFileEnable;              // "
   bool architectureFileEnable;      // "
   bool fabricFileEnable;            // "
   bool circuitFileEnable;           // "
   bool rcDelaysFileEnable;          // "
   bool laffFileEnable;              // "
   bool metricsEmailEnable;          // Enable sending email notification

   int laffMask;                     // Selects LAFF file object content

   TOS_RcDelaysExtractMode_t rcDelaysExtractMode;
                                     // Define RC delays extract: EMORE|D2M
   TOS_RcDelaysSortMode_t    rcDelaysSortMode;
                                     // Define RC delays sort: BY_NETS|BY_DELAYS
   TOS_RcDelaysNameList_t    rcDelaysNetNameList;
                                     // Define RC delays net name list (optional)
   TOS_RcDelaysNameList_t    rcDelaysBlockNameList;
                                     // Define RC delays block name list (optional)
   double                    rcDelaysMaxWireLength;
                                     // Define RC delays max wire length (optional)
private:

   enum TOS_DefCapacity_e 
   { 
      TOS_RC_DELAYS_NET_NAME_LIST_DEF_CAPACITY = 1,
      TOS_RC_DELAYS_BLOCK_NAME_LIST_DEF_CAPACITY = 1
   };
};

#endif
