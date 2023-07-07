#ifndef NOC_DATA_TYPES_H
#define NOC_DATA_TYPES_H

/** 
 * @file 
 * @brief This file contains datatype definitions which are used by the NoC datastructures.
 *  
 */

#include "vtr_strong_id.h"

// data types used to index the routers and links within the noc
struct noc_router_id_tag;
struct noc_link_id_tag;

/** Datatype to index routers within the NoC */
typedef vtr::StrongId<noc_router_id_tag, int> NocRouterId;
/** Datatype to index links within the NoC */
typedef vtr::StrongId<noc_link_id_tag, int> NocLinkId;

// data type to index traffic flows within the noc
struct noc_traffic_flow_id_tag;

/** Datatype to index traffic flows within the application */
typedef vtr::StrongId<noc_traffic_flow_id_tag, int> NocTrafficFlowId;

#endif