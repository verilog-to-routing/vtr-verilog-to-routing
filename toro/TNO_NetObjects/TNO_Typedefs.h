//===========================================================================//
// Purpose : Enums, typedefs, and defines for TNO_NetObjects classes.
//
//===========================================================================//

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
   TNO_NODE_CHANNEL,
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

// ??? class TC_SideIndex_c; // Forward declaration for subsequent class typedefs
// ??? typedef TC_SideIndex_c TNO_SwitchBoxPin_t;
// ??? typedef TCT_OrderedVector_c< TNO_SwitchBoxPin_t > TNO_SwitchBoxPinList_t;

class TNO_SwitchBox_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TNO_SwitchBox_c > TNO_SwitchBoxList_t;

//---------------------------------------------------------------------------//
// Define channel list typedefs
//---------------------------------------------------------------------------//

class TC_NameIndex_c; // Forward declaration for subsequent class typedefs
typedef TC_NameIndex_c TNO_Channel_t;
typedef TCT_OrderedVector_c< TNO_Channel_t > TNO_ChannelList_t;

#endif
