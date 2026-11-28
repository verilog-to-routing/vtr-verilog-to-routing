#pragma once

#include "vtr_strong_id.h"
#include "netlist_fwd.h"

/*
 * StrongId's for the ClusteredNetlist class
 */

//Forward declaration
class ClusteredNetlist;

///@brief A unique identifier for a block in the cluster netlist
typedef vtr::StrongId<struct cluster_block_id_tag> ClusterBlockId;

///@brief A unique identifier for a net in the cluster netlist
typedef vtr::StrongId<struct cluster_net_id_tag> ClusterNetId;

///@brief A unique identifier for a port in the cluster netlist
typedef vtr::StrongId<struct cluster_port_id_tag> ClusterPortId;

///@brief A unique identifier for a pin in the cluster netlist
typedef vtr::StrongId<struct cluster_pin_id_tag> ClusterPinId;
