//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_RouteOptions class.
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

#ifndef TOS_ROUTE_OPTIONS_H
#define TOS_ROUTE_OPTIONS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_RouteOptions_c
{
public:

   TOS_RouteOptions_c( void );
   TOS_RouteOptions_c( TOS_RouteAlgorithmMode_t algorithmMode,
                       TOS_RouteAbstractMode_t abstractMode,
                       unsigned int windowSize,
                       unsigned int channelWidth,
                       bool trimEmptyChannels,
                       bool trimObsChannels,
                       unsigned int maxIterations,
                       double histCongestionFactor,
                       double initCongestionFactor,
                       double presentCongestionFactor,
                       double bendCostFactor,
                       TOS_RouteResourceMode_t resourceMode,
                       TOS_RouteCostMode_t costMode,
                       double timingAStarFactor,
                       double timingMaxCriticality,
                       double slackCriticality,
                       bool preRouted_enable_,
                       TOS_RouteOrderMode_t preRouted_orderMode_ );

   ~TOS_RouteOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_RouteAlgorithmMode_t algorithmMode;    // Selects routement algorithm
   TOS_RouteAbstractMode_t  abstractMode;     // Selects global vs. detailed

   unsigned int windowSize;        // PathFinder routing constraints
   unsigned int channelWidth;      // "
   bool trimEmptyChannels;         // "
   bool trimObsChannels;           // "
   unsigned int maxIterations;     // "
   double histCongestionFactor;    // "
   double initCongestionFactor;    // "
   double presentCongestionFactor; // "
   double bendCostFactor;          // "

   TOS_RouteResourceMode_t resourceMode;      // Resource code: demand vs. delay 

   TOS_RouteCostMode_t costMode;   // Route cost: breath-first|directed-search|timing-driven
   double              fixedDelay; // Overrides timing analysis net delays (VPR-specific option)

   double timingAStarFactor;       // Timing-driven routing constraints
   double timingMaxCriticality;    // "
   double slackCriticality;        // "

   class TOS_PreRouted_c
   {
   public:

      bool                 enable;    // Enables applying pre-routed route constraints, if any
      TOS_RouteOrderMode_t orderMode; // Selects pre-routed route order: first|auto

   } preRouted;
};

#endif 
