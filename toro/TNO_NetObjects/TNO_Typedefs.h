//===========================================================================//
// Purpose : Enums, typedefs, and defines for TNO_NetObjects classes.
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
   TNO_STATUS_FLOAT = TNO_STATUS_OPEN,
   TNO_STATUS_FIXED,
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

#define TNO_NET_INDEX_INVALID SIZE_MAX
#define TNO_PORT_INDEX_INVALID SIZE_MAX
#define TNO_PIN_INDEX_INVALID SIZE_MAX

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
