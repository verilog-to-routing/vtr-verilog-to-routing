//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
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

#ifndef TOS_STRING_UTILS_H
#define TOS_STRING_UTILS_H

#include <string>
using namespace std;

#include "TOS_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//

void TOS_ExtractStringPackAlgorithmMode( TOS_PackAlgorithmMode_t mode, string* psrMode );
void TOS_ExtractStringPackClusterNetsMode( TOS_PackClusterNetsMode_t mode, string* psrMode );
void TOS_ExtractStringPackAffinityMode( TOS_PackAffinityMode_t mode, string* psrMode );
void TOS_ExtractStringPackCostMode( TOS_PackCostMode_t mode, string* psrMode );

void TOS_ExtractStringPlaceAlgorithmMode( TOS_PlaceAlgorithmMode_t mode, string* psrMode );
void TOS_ExtractStringPlaceCostMode( TOS_PlaceCostMode_t mode, string* psrMode );

void TOS_ExtractStringRouteAlgorithmMode( TOS_RouteAlgorithmMode_t mode, string* psrMode );
void TOS_ExtractStringRouteAbstractMode( TOS_RouteAbstractMode_t mode, string* psrMode );
void TOS_ExtractStringRouteResourceMode( TOS_RouteResourceMode_t mode, string* psrMode );
void TOS_ExtractStringRouteCostMode( TOS_RouteCostMode_t mode, string* psrMode );
void TOS_ExtractStringRouteOrderMode( TOS_RouteOrderMode_t mode, string* psrMode );

void TOS_ExtractStringOutputLaffMask( int mask, string* psrMask );

void TOS_ExtractStringRcDelaysExtractMode( TOS_RcDelaysExtractMode_t mode, string* psrMode );
void TOS_ExtractStringRcDelaysSortMode( TOS_RcDelaysSortMode_t mode, string* psrMode );

void TOS_ExtractStringExecuteToolMask( int mask, string* psrMask );

#endif
