//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_PackOptions class.  
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
                      TOS_PackCostMode_t costMode,
                      bool power_enable_ );
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

   class TOS_Power_c
   {
   public:

      bool enable;                // Enables applying packing power constraints, if any

   } power;
};

#endif 
