#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Forward declarations of the APNetlist class
 *
 * Forward declares the APNetlist class, and defines common types used by it.
 */

#include "netlist_fwd.h"
#include "vtr_strong_id.h"

// Forward declaration
class APNetlist;

/*
 * Ids
 *
 * The APNetlist uses unique IDs to identify any component of the netlist.
 * To avoid type-conversion errors, we use vtr::StrongIds.
 */

/**
 * @brief A unique identifier for a block in the AP netlist
 */
typedef vtr::StrongId<struct ap_block_id_tag> APBlockId;

/**
 * @brief A unique identifier for a net in the AP netlist
 */
typedef vtr::StrongId<struct ap_net_id_tag> APNetId;

/**
 * @brief A unique identifier for a port in the AP netlist
 */
typedef vtr::StrongId<struct ap_port_id_tag> APPortId;

/**
 * @brief A unique identifier for a pin in the AP netlist
 */
typedef vtr::StrongId<struct ap_pin_id_tag> APPinId;
