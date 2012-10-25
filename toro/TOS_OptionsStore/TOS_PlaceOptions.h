//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_PlaceOptions class.
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

#ifndef TOS_PLACE_OPTIONS_H
#define TOS_PLACE_OPTIONS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_PlaceOptions_c
{
public:

   TOS_PlaceOptions_c( void );
   TOS_PlaceOptions_c( TOS_PlaceAlgorithmMode_t algorithmMode,
                       unsigned int randomSeed,
                       double initTemp,
                       double initTempFactor,
                       double initTempEpsilon,
                       double exitTemp,
                       double exitTempFactor,
                       double exitTempEpsilon,
                       double reduceTemp,
                       double reduceTempFactor,
                       double innerNum,
                       double searchLimit,
                       TOS_PlaceCostMode_t costMode,
                       double timingCostFactor,
                       unsigned int timingUpdateInt,
                       unsigned int timingUpdateCount,
                       double slackInitWeight,
                       double slackFinalWeight );
   ~TOS_PlaceOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_PlaceAlgorithmMode_t algorithmMode;    // Selects placement algorithm

   unsigned int randomSeed;        // Simulated annealing placement constraints
   double        initTemp;         // "
   double        initTempFactor;   // "
   double        initTempEpsilon;  // "
   double        exitTemp;         // "
   double        exitTempFactor;   // "
   double        exitTempEpsilon;  // "
   double        reduceTemp;       // "
   double        reduceTempFactor; // "
   double        innerNum;         // "
   double        searchLimit;      // "

   TOS_PlaceCostMode_t costMode;   // Place cost: routability-driven|timing-driven
   double              fixedDelay; // Overrides timing analysis net delays (VPR-specific option)

   double        timingCostFactor; // Timing-driven placement constraints
   unsigned int timingUpdateInt;   // "
   unsigned int timingUpdateCount; // "
   double        slackInitWeight;  // "
   double        slackFinalWeight; // "
};

#endif 
