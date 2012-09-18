//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

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

void TOS_ExtractStringInputDataMode( TOS_InputDataMode_t mode, string* psrMode );

void TOS_ExtractStringOutputLaffMask( int mask, string* psrMask );

void TOS_ExtractStringRcDelaysExtractMode( TOS_RcDelaysExtractMode_t mode, string* psrMode );
void TOS_ExtractStringRcDelaysSortMode( TOS_RcDelaysSortMode_t mode, string* psrMode );

void TOS_ExtractStringExecuteToolMask( int mask, string* psrMask );

#endif
