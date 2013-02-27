//===========================================================================//
// Purpose : Method definitions for the TOS_RouteOptions class.
//
//           Public methods include:
//           - TOS_RouteOptions_c, ~TOS_RouteOptions_c
//           - Print
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

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_RouteOptions.h"

//===========================================================================//
// Method         : TOS_RouteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 02/21/13 jeffr : Added support for preRouted members
//===========================================================================//
TOS_RouteOptions_c::TOS_RouteOptions_c( 
      void )
      :
      algorithmMode( TOS_ROUTE_ALGORITHM_UNDEFINED ),
      abstractMode( TOS_ROUTE_ABSTRACT_UNDEFINED ),
      windowSize( 0 ),
      channelWidth( 0 ),
      maxIterations( 0 ),
      histCongestionFactor( 0.0 ),
      initCongestionFactor( 0.0 ),
      presentCongestionFactor( 0.0 ),
      bendCostFactor( 0.0 ),
      resourceMode( TOS_ROUTE_RESOURCE_UNDEFINED ),
      costMode( TOS_ROUTE_COST_UNDEFINED ),
      fixedDelay( 0.0 ),
      timingAStarFactor( 0.0 ),
      timingMaxCriticality( 0.0 ),
      slackCriticality( 0.0 )
{
   this->preRouted.enable = false;
   this->preRouted.orderMode = TOS_ROUTE_ORDER_UNDEFINED;
}

//===========================================================================//
TOS_RouteOptions_c::TOS_RouteOptions_c( 
      TOS_RouteAlgorithmMode_t algorithmMode_,
      TOS_RouteAbstractMode_t  abstractMode_,
      unsigned int             windowSize_,
      unsigned int             channelWidth_,
      unsigned int             maxIterations_,
      double                   histCongestionFactor_,
      double                   initCongestionFactor_,
      double                   presentCongestionFactor_,
      double                   bendCostFactor_,
      TOS_RouteResourceMode_t  resourceMode_,
      TOS_RouteCostMode_t      costMode_,
      double                   timingAStarFactor_,
      double                   timingMaxCriticality_,
      double                   slackCriticality_,
      bool                     preRouted_enable_,
      TOS_RouteOrderMode_t     preRouted_orderMode_ )
      :
      algorithmMode( algorithmMode_ ),
      abstractMode( abstractMode_ ), 
      windowSize( windowSize_ ), 
      channelWidth( channelWidth_ ), 
      maxIterations( maxIterations_ ), 
      histCongestionFactor( histCongestionFactor_ ), 
      initCongestionFactor( initCongestionFactor_ ), 
      presentCongestionFactor( presentCongestionFactor_ ), 
      bendCostFactor( bendCostFactor_ ), 
      resourceMode( resourceMode_ ), 
      costMode( costMode_ ), 
      fixedDelay( 0.0 ),
      timingAStarFactor( timingAStarFactor_ ),
      timingMaxCriticality( timingMaxCriticality_ ), 
      slackCriticality( slackCriticality_ )
{
   this->preRouted.enable = preRouted_enable_;
   this->preRouted.orderMode = preRouted_orderMode_;
}

//===========================================================================//
// Method         : ~TOS_RouteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_RouteOptions_c::~TOS_RouteOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 02/21/13 jeffr : Added support for preRouted members
//===========================================================================//
void TOS_RouteOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srAlgorithmMode, srAbstractMode, srResourceMode, srCostMode, srOrderMode;
   TOS_ExtractStringRouteAlgorithmMode( this->algorithmMode, &srAlgorithmMode );
   TOS_ExtractStringRouteAbstractMode( this->abstractMode, &srAbstractMode );
   TOS_ExtractStringRouteResourceMode( this->resourceMode, &srResourceMode );
   TOS_ExtractStringRouteCostMode( this->costMode, &srCostMode );
   TOS_ExtractStringRouteOrderMode( this->preRouted.orderMode, &srOrderMode );

   printHandler.Write( pfile, spaceLen, "ROUTE_ALGORITHM            = %s\n", TIO_SR_STR( srAlgorithmMode ));
   printHandler.Write( pfile, spaceLen, "ROUTE_TYPE                 = %s\n", TIO_SR_STR( srAbstractMode ));

   printHandler.Write( pfile, spaceLen, "ROUTE_WINDOW_SIZE          = %u\n", this->windowSize );
   printHandler.Write( pfile, spaceLen, "ROUTE_CHANNEL_WIDTH        = %u\n", this->channelWidth );
   printHandler.Write( pfile, spaceLen, "ROUTE_MAX_ITERATIONS       = %u\n", this->maxIterations );
   printHandler.Write( pfile, spaceLen, "ROUTE_CONG_HIST_FACTOR     = %0.*f\n", precision, this->histCongestionFactor );
   printHandler.Write( pfile, spaceLen, "ROUTE_CONG_INIT_FACTOR     = %0.*f\n", precision, this->initCongestionFactor );
   printHandler.Write( pfile, spaceLen, "ROUTE_CONG_PRESENT_FACTOR  = %0.*f\n", precision, this->presentCongestionFactor );
   printHandler.Write( pfile, spaceLen, "ROUTE_BEND_COST_FACTOR     = %0.*f\n", precision, this->bendCostFactor );
   printHandler.Write( pfile, spaceLen, "ROUTE_RESOURCE_COST_MODE   = %s\n", TIO_SR_STR( srResourceMode ));

   printHandler.Write( pfile, spaceLen, "ROUTE_COST_MODE            = %s\n", TIO_SR_STR( srCostMode ));

   printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_ASTAR_FACTOR  = %0.*f\n", precision, this->timingAStarFactor );
   printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_MAX_CRIT      = %0.*f\n", precision, this->timingMaxCriticality );
   printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_SLACK_CRIT    = %0.*f\n", precision, this->slackCriticality );

   printHandler.Write( pfile, spaceLen, "ROUTE_PREROUTED_ENABLE     = %s\n", TIO_BOOL_STR( this->preRouted.enable ));
   printHandler.Write( pfile, spaceLen, "ROUTE_PREROUTED_ORDER      = %s\n", TIO_SR_STR( srOrderMode ));
}
