#pragma once
/**
 * @file
 * @brief Forward declaration of the RoutingNetlist class and the IDs used by it.
 */

#include "vtr_strong_id.h"

// Forward declaration
class RoutingNetlist;

/// @brief A unique identifier for a block in the routing netlist
typedef vtr::StrongId<struct routing_block_id_tag> RoutingBlockId;

/// @brief A unique identifier for a pin in the routing netlist
typedef vtr::StrongId<struct routing_pin_id_tag> RoutingPinId;

/// @brief A unique identifier for a net in the routing netlist
typedef vtr::StrongId<struct routing_net_id_tag> RoutingNetId;
