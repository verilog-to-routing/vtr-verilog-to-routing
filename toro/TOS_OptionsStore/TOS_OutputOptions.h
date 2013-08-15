//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_OutputOptions 
//           class.
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
