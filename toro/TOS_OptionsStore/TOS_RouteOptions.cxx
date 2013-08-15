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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

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
      trimEmptyChannels( false ),
      trimObsChannels( false ),
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
      bool                     trimEmptyChannels_,
      bool                     trimObsChannels_,
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
      trimEmptyChannels( trimEmptyChannels_ ),
      trimObsChannels( trimObsChannels_ ),
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

   if( this->windowSize > 0 )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_WINDOW_SIZE          = %u\n", this->windowSize );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_WINDOW_SIZE\n" );
   }
   if( this->channelWidth > 0 )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_CHANNEL_WIDTH        = %u\n", this->channelWidth );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_CHANNEL_WIDTH\n" );
   }
   if( this->maxIterations > 0 )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_MAX_ITERATIONS       = %u\n", this->maxIterations );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_MAX_ITERATIONS\n" );
   }
   if( TCTF_IsGT( this->histCongestionFactor, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_CONG_HIST_FACTOR     = %0.*f\n", precision, this->histCongestionFactor );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_CONG_HIST_FACTOR\n" );
   }
   if( TCTF_IsGT( this->initCongestionFactor, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_CONG_INIT_FACTOR     = %0.*f\n", precision, this->initCongestionFactor );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_CONG_INIT_FACTOR\n" );
   }
   if( TCTF_IsGT( this->presentCongestionFactor, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_CONG_PRESENT_FACTOR  = %0.*f\n", precision, this->presentCongestionFactor );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_CONG_PRESENT_FACTOR\n" );
   }
   if( TCTF_IsGT( this->bendCostFactor, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_BEND_COST_FACTOR     = %0.*f\n", precision, this->bendCostFactor );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_BEND_COST_FACTOR\n" );
   }
   if( this->resourceMode != TOS_ROUTE_RESOURCE_UNDEFINED )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_RESOURCE_COST_MODE   = %s\n", TIO_SR_STR( srResourceMode ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_RESOURCE_COST_MODE\n" );

   }
   printHandler.Write( pfile, spaceLen, "ROUTE_COST_MODE            = %s\n", TIO_SR_STR( srCostMode ));

   if( TCTF_IsGT( this->timingAStarFactor, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_ASTAR_FACTOR  = %0.*f\n", precision, this->timingAStarFactor );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_TIMING_ASTAR_FACTOR\n" );
   }
   if( TCTF_IsGT( this->timingMaxCriticality, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_MAX_CRIT      = %0.*f\n", precision, this->timingMaxCriticality );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_TIMING_MAX_CRIT\n" );
   }
   if( TCTF_IsGT( this->slackCriticality, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_TIMING_SLACK_CRIT    = %0.*f\n", precision, this->slackCriticality );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_TIMING_SLACK_CRIT\n" );
   }

   printHandler.Write( pfile, spaceLen, "\n" );
   if( this->trimEmptyChannels )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_TRIM_EMPTY_CHANNELS  = %s\n", TIO_BOOL_STR( this->trimEmptyChannels ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_TRIM_EMPTY_CHANNELS\n" );
   }
   if( this->trimEmptyChannels )
   {
      printHandler.Write( pfile, spaceLen, "ROUTE_TRIM_OBS_CHANNELS    = %s\n", TIO_BOOL_STR( this->trimObsChannels ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// ROUTE_TRIM_OBS_CHANNELS\n" );
   }
   printHandler.Write( pfile, spaceLen, "ROUTE_PREROUTED_ENABLE     = %s\n", TIO_BOOL_STR( this->preRouted.enable ));
   printHandler.Write( pfile, spaceLen, "ROUTE_PREROUTED_ORDER      = %s\n", TIO_SR_STR( srOrderMode ));
}
