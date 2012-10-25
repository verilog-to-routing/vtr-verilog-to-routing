//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_RouteOptions class.
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
                       unsigned int maxIterations,
                       double histCongestionFactor,
                       double initCongestionFactor,
                       double presentCongestionFactor,
                       double bendCostFactor,
                       TOS_RouteResourceMode_t resourceMode,
                       TOS_RouteCostMode_t costMode,
                       double timingAStarFactor,
                       double timingMaxCriticality,
                       double slackCriticality );

   ~TOS_RouteOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   TOS_RouteAlgorithmMode_t algorithmMode;    // Selects routement algorithm
   TOS_RouteAbstractMode_t  abstractMode;     // Selects global vs. detailed

   unsigned int windowSize;        // PathFinder routing constraints
   unsigned int channelWidth;      // "
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
};

#endif 
