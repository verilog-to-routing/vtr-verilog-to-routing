#ifndef RR_GRAPH_OBJ_FWD_H
#define RR_GRAPH_OBJ_FWD_H
#include "vtr_strong_id.h"

/***************************************************************
 * This file includes a light declaration for the class RRGraph
 * For a detailed description and how to use the class RRGraph,
 * please refer to rr_graph_obj.h
 ***************************************************************/

class RRGraph;

struct rr_node_id_tag;
struct rr_edge_id_tag;
struct rr_switch_id_tag;
struct rr_segment_id_tag;

typedef vtr::StrongId<rr_node_id_tag> RRNodeId;
typedef vtr::StrongId<rr_edge_id_tag> RREdgeId;
typedef vtr::StrongId<rr_switch_id_tag, short> RRSwitchId;
typedef vtr::StrongId<rr_segment_id_tag, short> RRSegmentId;

#endif
