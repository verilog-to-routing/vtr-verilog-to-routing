//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_PackOptions class.  
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

#ifndef TOS_PACK_OPTIONS_H
#define TOS_PACK_OPTIONS_H

#include <cstdio>
using namespace std;

#include "TOS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_PackOptions_c
{
public:

   TOS_PackOptions_c( void );
   TOS_PackOptions_c( TOS_PackAlgorithmMode_t algorithmMode,
                      TOS_PackClusterNetsMode_t clusterNetsMode,
                      double areaWeight,
                      double netsWeight,
                      TOS_PackAffinityMode_t affinityMode,
                      unsigned int blockSize,
                      unsigned int lutSize,
                      TOS_PackCostMode_t costMode );
   ~TOS_PackOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_PackAlgorithmMode_t algorithmMode;     // Selects packing algorithm

   TOS_PackClusterNetsMode_t clusterNetsMode; // AAPack cluster nets: min|max-connections
   double areaWeight;                         // AAPack area (alpha) weight
   double netsWeight;                         // AAPack nets (beta) weight
   TOS_PackAffinityMode_t affinityMode;       // AAPack affinity: none|any

   unsigned int blockSize;        // Pack block (CLB) size (N) - overrides architecture
   unsigned int lutSize;          // Pack LUT size (K) - overrides architecture

   TOS_PackCostMode_t costMode;   // Pack cost: routability-driven|timing-driven
   double             fixedDelay; // Overrides timing analysis net delays (VPR-specific option)
};

#endif 
