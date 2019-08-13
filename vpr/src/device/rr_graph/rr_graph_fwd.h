/************************************************************************
 * This file introduces data types and pre-decleration for the RRGraph class 
 * If you want to use RRGraph class. This is a light header file you can include in your source files. 
 ***********************************************************************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */

#ifndef RR_GRAPH_OBJ_FWD_H
#    define RR_GRAPH_OBJ_FWD_H
#    include "vtr_strong_id.h"

class RRGraph;

struct rr_node_id_tag;
struct rr_edge_id_tag;
struct rr_switch_id_tag;
struct rr_segment_id_tag;

typedef vtr::StrongId<rr_node_id_tag> RRNodeId;
typedef vtr::StrongId<rr_edge_id_tag> RREdgeId;
typedef vtr::StrongId<rr_switch_id_tag, short> RRSwitchId;
typedef vtr::StrongId<rr_segment_id_tag, short> RRSegmentId;

/* Create an alias to the open NodeId
 * Useful in identifying if a node exist in a rr_graph
 */
#    define RRGRAPH_OPEN_NODE_ID RRNodeId(-1)
#    define RRGRAPH_OPEN_EDGE_ID RREdgeId(-1)
#    define RRGRAPH_OPEN_SWITCH_ID RRSwitchId(-1)
#    define RRGRAPH_OPEN_SEGMENT_ID RRSegmentId(-1)

#endif

/************************************************************
 * End Of File (EOF)
 ***********************************************************/
