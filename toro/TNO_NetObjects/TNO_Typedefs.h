//===========================================================================//
// Purpose : Enums, typedefs, and defines for TNO_NetObjects classes.
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

#ifndef TNO_TYPEDEFS_H
#define TNO_TYPEDEFS_H

#include "TCT_OrderedVector.h"
#include "TCT_SortedVector.h"

#include "TCT_NameList.h"

//---------------------------------------------------------------------------//
// Define net object constants and typedefs
//---------------------------------------------------------------------------//

enum TNO_StatusMode_e
{
   TNO_STATUS_UNDEFINED = 0,
   TNO_STATUS_OPEN,
   TNO_STATUS_GLOBAL_ROUTED,
   TNO_STATUS_GROUTED = TNO_STATUS_GLOBAL_ROUTED,
   TNO_STATUS_ROUTED
};
typedef enum TNO_StatusMode_e TNO_StatusMode_t;

enum TNO_NodeType_e
{
   TNO_NODE_UNDEFINED = 0,
   TNO_NODE_INST_PIN,
   TNO_NODE_SEGMENT,
   TNO_NODE_SWITCH_BOX
};
typedef enum TNO_NodeType_e TNO_NodeType_t;

//---------------------------------------------------------------------------//
// Define net list typedefs
//---------------------------------------------------------------------------//

class TC_Name_c; // Forward declaration for subsequent class typedefs
typedef TCT_NameList_c< TC_Name_c > TNO_NameList_t;

//---------------------------------------------------------------------------//
// Define instance pin typedefs
//---------------------------------------------------------------------------//

class TNO_InstPin_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TNO_InstPin_c > TNO_InstPinList_t;

//---------------------------------------------------------------------------//
// Define global route list typedefs
//---------------------------------------------------------------------------//

class TC_NameLength_c; // Forward declaration for subsequent class typedefs
typedef TC_NameLength_c TNO_GlobalRoute_t;
typedef TCT_OrderedVector_c< TNO_GlobalRoute_t > TNO_GlobalRouteList_t;

//---------------------------------------------------------------------------//
// Define route list typedefs
//---------------------------------------------------------------------------//

class TNO_Node_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TNO_Node_c > TNO_Route_t;

class TNO_Route_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TNO_Route_t > TNO_RouteList_t;

//---------------------------------------------------------------------------//
// Define switch box list typedefs
//---------------------------------------------------------------------------//

class TNO_SwitchBox_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TNO_SwitchBox_c > TNO_SwitchBoxList_t;

#endif
