#pragma once
/**
 * @file
 * @brief Forward declaration of the RoutingNetlist class and the IDs used by it.
 */

#include "netlist_fwd.h"

// Forward declaration
class RoutingNetlist;

/// @brief A unique identifier for a block in the routing netlist
typedef ParentBlockId RoutingBlockId;

/// @brief A unique identifier for a pin in the routing netlist
typedef ParentPinId RoutingPinId;

/// @brief A unique identifier for a net in the routing netlist
typedef ParentNetId RoutingNetId;
