//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_PlaceOptions class.
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
                       bool regionPlace_enable_,
                       bool relativePlace_enable_,
                       bool relativePlace_rotateEnable_,
                       bool relativePlace_carryChainEnable_,
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

   class TOS_RegionPlace_c
   {
   public:

      bool         enable;         // Enables applying region placement constraints, if any

   } regionPlace;

   class TOS_RelativePlace_c
   {
   public:

      bool         enable;         // Enables applying relative placement constraints, if any
      bool         rotateEnable;   // Enables relative placement rotate & mirror transforms
      bool         carryChainEnable;
                                   // Enables relative placement carry chain override 
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
