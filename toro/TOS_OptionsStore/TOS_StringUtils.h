//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
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
