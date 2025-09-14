#pragma once
/** 
 * @file 
 * @brief This file contains datatype definitions which are used by the NoC data structures.
 *  
 */

#include "vtr_strong_id.h"

// data types used to index the routers and links within the noc

/** Datatype to index routers within the NoC */
typedef vtr::StrongId<struct noc_router_id_tag, int> NocRouterId;
/** Datatype to index links within the NoC */
typedef vtr::StrongId<struct noc_link_id_tag, int> NocLinkId;

// data type to index traffic flows within the noc

/** Datatype to index traffic flows within the application */
typedef vtr::StrongId<struct noc_traffic_flow_id_tag, int> NocTrafficFlowId;

/** Data type to index NoC groups. */
typedef vtr::StrongId<struct noc_group_id_tag, int> NocGroupId;
