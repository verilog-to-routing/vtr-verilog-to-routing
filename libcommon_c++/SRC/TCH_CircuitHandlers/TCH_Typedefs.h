//===========================================================================//
// Purpose : Enums, typedefs, and defines for TCH_CircuitHandler classes.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCH_TYPEDEFS_H
#define TCH_TYPEDEFS_H

#include "TCT_OrderedVector.h"
#include "TCT_SortedVector.h"
#include "TCT_SortedNameDynamicVector.h"
#include "TCT_Hash.h"

//---------------------------------------------------------------------------//
// Define relative place handler constants and typedefs
//---------------------------------------------------------------------------//

enum TCH_PlaceMacroRotateMode_e 
{ 
   TCH_PLACE_MACRO_ROTATE_UNDEFINED,
   TCH_PLACE_MACRO_ROTATE_FROM,
   TCH_PLACE_MACRO_ROTATE_TO
};
typedef enum TCH_PlaceMacroRotateMode_e TCH_PlaceMacroRotateMode_t;

// Define maximum number of attempts made while trying to find a random,
// yet valid (legal) origin point (ie. placement) position.
#define TCH_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS 64

//---------------------------------------------------------------------------//
// Define relative macro constants and typedefs
//---------------------------------------------------------------------------//

class TCH_RelativeMacro_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TCH_RelativeMacro_c > TCH_RelativeMacroList_t;

#define TCH_RELATIVE_MACRO_UNDEFINED SIZE_MAX
 
//---------------------------------------------------------------------------//
// Define relative node constants and typedefs
//---------------------------------------------------------------------------//

class TCH_RelativeNode_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TCH_RelativeNode_c > TCH_RelativeNodeList_t;

#define TCH_RELATIVE_NODE_UNDEFINED SIZE_MAX

//---------------------------------------------------------------------------//
// Define relative block constants and typedefs
//---------------------------------------------------------------------------//

class TCH_RelativeBlock_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TCH_RelativeBlock_c > TCH_RelativeBlockList_t;

//---------------------------------------------------------------------------//
// Define relative block constants and typedefs
//---------------------------------------------------------------------------//

class TCH_RelativeMove_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TCH_RelativeMove_c > TCH_RelativeMoveList_t;

//---------------------------------------------------------------------------//
// Define rotate mask hash constants and typedefs
//---------------------------------------------------------------------------//

class TCH_RotateMaskKey_c; // Forward declaration for subsequent class typedefs
class TCH_RotateMaskValue_c; // Forward declaration for subsequent class typedefs
typedef TCT_Hash_c< TCH_RotateMaskKey_c, TCH_RotateMaskValue_c > TCH_RotateMaskHash_t;

//---------------------------------------------------------------------------//
// Define pre-place handler constants and typedefs
//---------------------------------------------------------------------------//

class TCH_PrePlacedBlock_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TCH_PrePlacedBlock_c > TCH_PrePlacedBlockList_t;

enum TCH_PlaceStatusMode_e
{
   TCH_PLACE_STATUS_UNDEFINED = 0,
   TCH_PLACE_STATUS_FLOAT,
   TCH_PLACE_STATUS_FIXED,
   TCH_PLACE_STATUS_PLACED
};
typedef enum TCH_PlaceStatusMode_e TCH_PlaceStatusMode_t;

//---------------------------------------------------------------------------//
// Define pre-route handler constants and typedefs
//---------------------------------------------------------------------------//

class TC_NameIndex_c; // Forward declaration for subsequent class typedefs
typedef TC_NameIndex_c TCH_NetName_t;
typedef TCT_SortedNameDynamicVector_c< TCH_NetName_t > TCH_NetNameList_t;

class TCH_Net_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TCH_Net_c > TCH_NetList_t;

class TCH_RoutePath_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TCH_RoutePath_c > TCH_RoutePathList_t;

class TCH_RouteNode_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TCH_RouteNode_c > TCH_RouteNodeList_t;

class TCH_VPR_GraphNode_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TCH_VPR_GraphNode_c > TCH_VPR_GraphNodeList_t;

class TCH_VPR_GraphToRoute_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TCH_VPR_GraphToRoute_c > TCH_VPR_GraphToRouteMap_t;

#define TCH_ROUTE_NODE_INDEX_INVALID SIZE_MAX

// ???
enum TCH_RouteStatusMode_e
{
   TCH_ROUTE_STATUS_UNDEFINED = 0,
   TCH_ROUTE_STATUS_FLOAT,
   TCH_ROUTE_STATUS_FIXED,
   TCH_ROUTE_STATUS_ROUTED
};
typedef enum TCH_RouteStatusMode_e TCH_RouteStatusMode_t;

enum TCH_RouteOrderMode_e 
{ 
   TCH_ROUTE_ORDER_UNDEFINED,
   TCH_ROUTE_ORDER_FIRST,
   TCH_ROUTE_ORDER_AUTO
};
typedef enum TCH_RouteOrderMode_e TCH_RouteOrderMode_t;

#endif
