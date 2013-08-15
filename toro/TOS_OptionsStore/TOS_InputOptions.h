//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_InputOptions class.
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

#ifndef TOS_INPUT_OPTIONS_H
#define TOS_INPUT_OPTIONS_H

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
class TOS_InputOptions_c
{
public:

   TOS_InputOptions_c( void );
   TOS_InputOptions_c( const TOS_OptionsNameList_t& optionsFileNameList,
                       const string& srXmlFileName,
                       const string& srBlifFileName,
                       const string& srArchitectureFileName,
                       const string& srFabricFileName,
                       const string& srCircuitFileName,
                       bool xmlFileEnable,
                       bool blifFileEnable,
                       bool architectureFileEnable,
                       bool fabricFileEnable,
                       bool circuitFileEnable );
   ~TOS_InputOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_OptionsNameList_t optionsFileNameList;
                                  // List (1 or more) of options file names
   string srXmlFileName;          // VPR's XML architecture spec file name
   string srBlifFileName;         // VPR's BLIF circuit description file name
   string srArchitectureFileName; // Architecture spec file name
   string srFabricFileName;       // Fabric model file name 
   string srCircuitFileName;      // Circuit description file name

   bool xmlFileEnable;            // Enable/disable reading input file
   bool blifFileEnable;           // "
   bool architectureFileEnable;   // "
   bool fabricFileEnable;         // "
   bool circuitFileEnable;        // "

private:

   enum TOS_DefCapacity_e 
   { 
      TOS_OPTIONS_FILE_NAME_LIST_DEF_CAPACITY = 1
   };
};

#endif
