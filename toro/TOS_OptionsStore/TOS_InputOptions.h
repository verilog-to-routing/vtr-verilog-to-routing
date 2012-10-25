//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_InputOptions class.
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
                       bool circuitFileEnable,
                       TOS_InputDataMode_t prePackedDataMode,
                       TOS_InputDataMode_t prePlacedDataMode,
                       TOS_InputDataMode_t preRoutedDataMode );
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

   TOS_InputDataMode_t prePackedDataMode; // Pre-packed mode: none|any
   TOS_InputDataMode_t prePlacedDataMode; // Pre-placed mode: none|blocks|ios|any
   TOS_InputDataMode_t preRoutedDataMode; // Pre-routed mode: none|any

private:

   enum TOS_DefCapacity_e 
   { 
      TOS_OPTIONS_FILE_NAME_LIST_DEF_CAPACITY = 1
   };
};

#endif
