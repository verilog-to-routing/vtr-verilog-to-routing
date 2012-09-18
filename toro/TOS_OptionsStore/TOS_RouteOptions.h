//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_RouteOptions class.
//
//===========================================================================//

#ifndef TOS_ROUTE_OPTIONS_H
#define TOS_ROUTE_OPTIONS_H

#include <stdio.h>

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
