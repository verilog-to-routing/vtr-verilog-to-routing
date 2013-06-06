//===========================================================================//
// Purpose : Method definitions for the TOS_RulesSwitches class.
//
//           Public methods include:
//           - TOS_RulesSwitches_c, ~TOS_RulesSwitches_c
//           - Print
//           - Init
//           - Apply
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TOS_RulesSwitches.h"

//===========================================================================//
// Method         : TOS_RulesSwitches_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_RulesSwitches_c::TOS_RulesSwitches_c( 
      void )
{
} 

//===========================================================================//
// Method         : ~TOS_RulesSwitches_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_RulesSwitches_c::~TOS_RulesSwitches_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_RulesSwitches_c::Print( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->Print( pfile, spaceLen );
}

//===========================================================================//
void TOS_RulesSwitches_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   char szSep[TIO_FORMAT_STRING_LEN_MAX];
   size_t lenSep = TIO_FORMAT_BANNER_LEN_DEF;

   TC_FormatStringFilled( '-', szSep, lenSep );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Fabric Options", lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->fabricOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Pack Options",   lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->packOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Place Options",  lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->placeOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, "Route Options",  lenSep, "//", "//" );
   printHandler.WriteCenter( pfile, spaceLen, szSep,            lenSep, "//", "//" );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->routeOptions.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );
}


//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 01/15/13 jeffr : Added support for placeOptions' relativePlace members
// 01/18/13 jeffr : Added support for placeOptions' prePlaced members
// 02/21/13 jeffr : Added support for routeOptions' preRouted members
// 04/24/13 jeffr : Added support for fabricOptions' SB and CB members
//===========================================================================//
void TOS_RulesSwitches_c::Init(
      void )
{
   this->fabricOptions.blocks.override = false;
   this->fabricOptions.switchBoxes.override = false;
   this->fabricOptions.connectionBlocks.override = false;
   this->fabricOptions.channels.override = false;

   this->packOptions.algorithmMode = TOS_PACK_ALGORITHM_AAPACK;
   this->packOptions.clusterNetsMode = TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS;
   this->packOptions.areaWeight = 0.75;
   this->packOptions.netsWeight = 0.9;
   this->packOptions.affinityMode = TOS_PACK_AFFINITY_ANY;
   this->packOptions.blockSize = 1;
   this->packOptions.lutSize = 4;
   this->packOptions.costMode = TOS_PACK_COST_TIMING_DRIVEN;
   this->packOptions.fixedDelay = 0.0;

   this->placeOptions.algorithmMode = TOS_PLACE_ALGORITHM_ANNEALING;
   this->placeOptions.channelWidth = 100;
   this->placeOptions.randomSeed = 1;
   this->placeOptions.initTemp = 0.0;
   this->placeOptions.initTempFactor = 20.0;
   this->placeOptions.initTempEpsilon = 20.0;
   this->placeOptions.exitTemp = 0.0;
   this->placeOptions.exitTempFactor = 0.005;
   this->placeOptions.exitTempEpsilon = 0.005;
   this->placeOptions.reduceTemp = 0.0;
   this->placeOptions.reduceTempFactor = 0.44;
   this->placeOptions.innerNum = 10.0;
   this->placeOptions.searchLimit = 0.44;
   this->placeOptions.costMode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
   this->placeOptions.fixedDelay = 0.0;
   this->placeOptions.timingCostFactor = 0.5;
   this->placeOptions.timingUpdateInt = 1;
   this->placeOptions.timingUpdateCount = 0;
   this->placeOptions.slackInitWeight = 1.0;
   this->placeOptions.slackFinalWeight = 8.0;
   this->placeOptions.relativePlace.enable = false;
   this->placeOptions.relativePlace.rotateEnable = true;
   this->placeOptions.relativePlace.maxPlaceRetryCt = TOS_PLACE_RELATIVE_INITIAL_PLACE_RETRY;
   this->placeOptions.relativePlace.maxMacroRetryCt = TOS_PLACE_RELATIVE_INITIAL_MACRO_RETRY;
   this->placeOptions.prePlaced.enable = false;

   this->routeOptions.algorithmMode = TOS_ROUTE_ALGORITHM_PATHFINDER;
   this->routeOptions.abstractMode = TOS_ROUTE_ABSTRACT_DETAILED;
   this->routeOptions.windowSize = 3;
   this->routeOptions.channelWidth = 0;
   this->routeOptions.trimEmptyChannels = true;
   this->routeOptions.trimObsChannels = true;
   this->routeOptions.maxIterations = 50;
   this->routeOptions.histCongestionFactor = 1.0;
   this->routeOptions.initCongestionFactor = 0.5;
   this->routeOptions.presentCongestionFactor = 1.3;
   this->routeOptions.bendCostFactor = 0.0;
   this->routeOptions.resourceMode = TOS_ROUTE_RESOURCE_DELAY_NORMALIZED;
   this->routeOptions.costMode = TOS_ROUTE_COST_TIMING_DRIVEN;
   this->routeOptions.fixedDelay = 0.0;
   this->routeOptions.timingMaxCriticality = 0.99;
   this->routeOptions.slackCriticality = 1.0;
   this->routeOptions.timingAStarFactor = 1.2;

   this->routeOptions.preRouted.enable = false;
   this->routeOptions.preRouted.orderMode = TOS_ROUTE_ORDER_FIRST;
} 

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_RulesSwitches_c::Apply(
      void )
{
    // This is only a stub... (currently looking for interesting work to do)
}
