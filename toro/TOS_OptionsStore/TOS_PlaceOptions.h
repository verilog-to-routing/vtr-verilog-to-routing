//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_PlaceOptions class.
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
                       unsigned int channelWidth,
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
                       double slackFinalWeight,
                       bool relativePlace_enable_,
                       bool relativePlace_rotateEnable_,
                       unsigned int relativePlace_maxPlaceRetryCt_,
                       unsigned int relativePlace_maxMacroRetryCt_,
                       bool prePlaced_enable_ );
   ~TOS_PlaceOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_PlaceAlgorithmMode_t algorithmMode;    // Selects placement algorithm

   unsigned int channelWidth;      // Simulated annealing placement constraints
   unsigned int randomSeed;        // "
   double       initTemp;          // "
   double       initTempFactor;    // "
   double       initTempEpsilon;   // "
   double       exitTemp;          // "
   double       exitTempFactor;    // "
   double       exitTempEpsilon;   // "
   double       reduceTemp;        // "
   double       reduceTempFactor;  // "
   double       innerNum;          // "
   double       searchLimit;       // "

   TOS_PlaceCostMode_t costMode;   // Place cost: routability-driven|timing-driven
   double              fixedDelay; // Overrides timing analysis net delays (VPR-specific option)

   double       timingCostFactor;  // Timing-driven placement constraints
   unsigned int timingUpdateInt;   // "
   unsigned int timingUpdateCount; // "
   double       slackInitWeight;   // "
   double       slackFinalWeight;  // "

   class TOS_RelativePlace_c
   {
   public:

      bool         enable;         // Enables applying relative placement constraints, if any
      bool         rotateEnable;   // Enables relative placement rotate & mirror transforms
      unsigned int maxPlaceRetryCt;// Defines initial relative placement retry count
                                   // (default = 3)
      unsigned int maxMacroRetryCt;// Defines initial relative macro retry count
                                   // (default = 10 * rotate (1 or 8) * relative macro len)
   } relativePlace;

   class TOS_PrePlaced_c
   {
   public:

      bool         enable;         // Enables applying pre-placed placement constraints, if any

   } prePlaced;
};

#endif 
