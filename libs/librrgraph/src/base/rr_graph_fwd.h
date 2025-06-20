#pragma once

#include <cstdint>

#include "vtr_strong_id.h"

/***************************************************************
 * This file includes a light declaration for the class RRGraph
 * For a detailed description and how to use the class RRGraph,
 * please refer to rr_graph_obj.h
 ***************************************************************/

//Forward declaration
class t_rr_graph_storage;

class RRGraph;

typedef vtr::StrongId<struct rr_node_id_tag, uint32_t> RRNodeId;
typedef vtr::StrongId<struct rr_edge_id_tag, uint32_t> RREdgeId;
typedef vtr::StrongId<struct rr_indexed_data_id_tag, uint32_t> RRIndexedDataId;
typedef vtr::StrongId<struct rr_switch_id_tag, uint16_t> RRSwitchId;
typedef vtr::StrongId<struct rr_segment_id_tag, uint16_t> RRSegmentId;
typedef vtr::StrongId<struct rc_index_tag, uint16_t> NodeRCIndex;
