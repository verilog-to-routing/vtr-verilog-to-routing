/************************************************************************
 * This file introduces a class to model a Routing Resource Graph (RRGraph or RRG) 
 * which is widely used by placers, routers, analyzers etc.
 *
 * Overview
 * ========
 * RRGraph aims to describe in a general way how routing resources are connected
 * in a FPGA fabric.
 * It includes device-level information for routing resources, 
 * such as the physical location of nodes, 
 * routing segment information of each routing resource 
 * switch information of each routing resource.
 * 
 * A Routing Resource Graph (RRGraph or RRG) is a Directed Acyclic Graph (DAG),
 * which consists of a number of nodes and edges.
 *
 * Node
 * ----
 * Each node represents a routing resource, which could be 
 * 1. a routing track in X-direction or Y-direction (CHANX or CHANY) 
 * 2. an input or an output of a logic block (IPIN or OPIN) 
 * 3. a virtual source or sink node (SOURCE or SINK), which are starting/ending points of routing trees.
 *
 * Edge
 * ----
 * Each edge represents a switch between routing resources, which could be
 * 1. a multiplexer
 * 2. a tri-state buffer
 * The switch information are categorized in the rr_switch_inf of RRGraph class.
 *
 * Guidlines on using the RRGraph data structure 
 * =============================================
 *
 * For those want to access data from RRGraph
 * ------------------------------------------
 * Please refer to the detailed comments on each public accessors
 *
 * For those want to build/modify a RRGraph
 * -----------------------------------------
 * Do NOT add a builder to this data structure!
 * Builders should be kept as free functions that use the public mutators
 * We suggest developers to create builders in separated C/C++ source files
 * outside the rr_graph header and source files
 *
 * After build/modify a RRGraph, please do run a fundamental check, a public accessor.
 * to ensure that your RRGraph does not include invalid nodes/edges/switches/segements 
 * as well as connections.
 * This is a must-do check! 
 *
 * Example: 
 *    RRGraph rr_graph;
 *    ... // Building RRGraph
 *    rr_graph.check(); 
 *
 * Optionally, we strongly recommend developers to run an advance check in check_rr_graph()  
 * This guarantees legal and routable RRGraph for VPR routers.
 *
 * Note that these are generic checking codes. 
 * If your RRGraph is designed for a special application context, please develop your 
 * check_rr_graph() function to guarantee its correctness!
 *
 * Do NOT modify the coordinate system for nodes, they are designed for downstream drawers and routers
 *
 * For those want to extend RRGraph data structure
 * --------------------------------------------------------------------------
 * Please avoid modifying any existing public/private accessors/mutators
 * in order to keep a stable RRGraph object in the framework
 * Developers may add more internal data to RRGraph as well as associate accessors/mutators
 * Please update and comment on the added features properly to keep this data structure friendly to be extended.
 *
 * Try to keep your extension within only graph-related internal data to RRGraph.
 * In other words, extension is necessary when the new node/edge attributes are needed.
 * RRGraph should NOT include other data which are shared by other data structures outside.
 * Instead, using indices to point to the outside data source instead of embedding to RRGraph
 * For example: 
 *   For any placement/routing cost related information, try to extend t_rr_indexed_data, but not RRGraph
 *   For any placement/routing results, try to extend PlaceContext and RoutingContext, but not RRGraph
 * 
 * For those want to develop placers or routers
 * --------------------------------------------------------------------------
 * The RRGraph is designed to be a read-only database/graph, once created.
 * Placement and routing should NOT change any attributes of RRGraph.
 * Any placement and routing results should be stored in other data structures, such as PlaceContext and RoutingContext. 
 *
 * Tracing Cross-Reference 
 * =======================
 * RRGraph is designed to a self-contained data structure as much as possible.
 * It includes the switch information (rr_switch) and segment_information (rr_segment)
 * which are necessary to build-up any external data structures.
 *
 * Internal cross-reference
 * ------------------------
 *
 *  +--------+                  +--------+
 *  |        |  node_in_edges   |        |
 *  |        |  node_out_edges  |        |
 *  |  Node  |----------------->|  Edge  |--+
 *  |        |<-----------------|        |  |
 *  |        |  edge_src_nodes  |        |  |
 *  +--------+  edge_sink_nodes +--------+  |edge_switch
 *      |                                   |
 *      | node_segment                      |
 *      v                                   v
 *  +------------+                     +----------+
 *  |            |                     |          |
 *  |            |                     |          |
 *  |  Segments  |                     |  Switch  |
 *  |            |                     |          |
 *  |            |                     |          |
 *  +------------+                     +----------+
 *
 *
 * External cross-reference
 * ------------------------
 * The only cross-reference to outside data structures is the cost_index
 * corresponding to the data structure t_rr_index_data
 * Details can be found in the definition of t_rr_index_data
 *
 * +---------+  cost_index   +------------------------------+
 * | RRGraph |-------------->| cost_indexe_data[cost_index] | 
 * +---------+               +------------------------------+
 *
 * Access to a node/edge/switch/segment, please use the StrongId created
 * ---------------------------------------------------------------------
 *    For node, use RRNodeId
 *    For edge, use RREdgeId
 *    For switch, use RRSwitchId
 *    For segment, use RRSegmentId
 *
 *    These are the unique identifier for each data type.
 *    To check if your id is valid or not, use the INVALID() function of StrongId class.
 *    Example:
 *       if (node_id == RRNodeId::INVALID()) {
 *       }  
 *
 ***********************************************************************/
#ifndef RR_GRAPH_OBJ_H
#define RR_GRAPH_OBJ_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */
#include <limits>
#include <vector>

/* EXTERNAL library header files go second*/
#include "vtr_vector.h"
#include "vtr_range.h"
#include "vtr_geometry.h"
#include "arch_types.h"

/* VPR header files go second*/
#include "vpr_types.h"
#include "rr_graph_fwd.h"

class RRGraph {
  public: /* Types */
    /* Iterators used to create iterator-based loop for nodes/edges/switches/segments */
    typedef vtr::vector<RRNodeId, RRNodeId>::const_iterator node_iterator;
    typedef vtr::vector<RREdgeId, RREdgeId>::const_iterator edge_iterator;
    typedef vtr::vector<RRSwitchId, RRSwitchId>::const_iterator switch_iterator;
    typedef vtr::vector<RRSegmentId, RRSegmentId>::const_iterator segment_iterator;

    /* Ranges used to create range-based loop for nodes/edges/switches/segments */
    typedef vtr::Range<node_iterator> node_range;
    typedef vtr::Range<edge_iterator> edge_range;
    typedef vtr::Range<switch_iterator> switch_range;
    typedef vtr::Range<segment_iterator> segment_range;

  public: /* Accessors */
    /* Aggregates: create range-based loops for nodes/edges/switches/segments
     * To iterate over the nodes/edges/switches/segments in a RRGraph, 
     *    using a range-based loop is suggested.
     *  -----------------------------------------------------------------
     *    Example: iterate over all the nodes
     *      for (const RRNodeId& node : nodes()) {
     *        // Do something with each node
     *      }
     *
     *      for (const RREdgeId& edge : edges()) {
     *        // Do something with each edge
     *      }
     *
     *      for (const RRSwitchId& switch : switches()) {
     *        // Do something with each switch
     *      }
     *
     *      for (const RRSegmentId& segment : segments()) {
     *        // Do something with each segment
     *      }
     */
    node_range nodes() const;
    edge_range edges() const;
    switch_range switches() const;
    segment_range segments() const;

    /* Node-level attributes */
    size_t node_index(const RRNodeId& node) const; /* TODO: deprecate this accessor as outside functions should use RRNodeId */

    /* get the type of a RRGraph node : types of each node, can be channel wires (CHANX or CHANY) or 
     *                                  logic block pins(OPIN or IPIN) or virtual nodes (SOURCE or SINK)
     *                                  see t_rr_type definition for more details
     */
    t_rr_type node_type(const RRNodeId& node) const;

    /* Get coordinate of a node. (xlow, xhigh, ylow, yhigh):
     *   For OPIN/IPIN/SOURCE/SINK, xlow = xhigh and ylow = yhigh
     *   FIXME: should confirm if this is still the case when a logic block has a height > 1
     *   For CHANX/CHANY, (xlow, ylow) and (xhigh, yhigh) represent
     *   where the routing segment starts and ends.
     *   Note that our convention alway keeps
     *   xlow <= xhigh and ylow <= yhigh
     *   Therefore, (xlow, ylow) is a starting point for a CHANX/CHANY in INC_DIRECTION
     *   (xhigh, yhigh) is a starting point for a CHANX/CHANY in INC_DIRECTION
     *                           
     *   Example :
     *   CHANX in INC_DIRECTION  
     *   (xlow, ylow)                (xhigh, yhigh)
     *        |                           |
     *       \|/                         \|/
     *        ---------------------------->
     *
     *   CHANX in DEC_DIRECTION  
     *   (xlow, ylow)                (xhigh, yhigh)
     *        |                           |
     *       \|/                         \|/
     *        <----------------------------
     *
     *   CHANY in INC_DIRECTION
     *       
     *      /|\ <-------(xhigh, yhigh)
     *       |
     *       |  <-------(xlow, ylow)
     *
     *   CHANY in DEC_DIRECTION
     *       
     *       |  <-------(xhigh, yhigh)
     *       |
     *      \|/ <-------(xlow, ylow)
     */
    short node_xlow(const RRNodeId& node) const;
    short node_ylow(const RRNodeId& node) const;
    short node_xhigh(const RRNodeId& node) const;
    short node_yhigh(const RRNodeId& node) const;
    /* Get the length of a routing track. 
     * Note that it is ONLY meaningful for CHANX and CHANY, which represents the number of logic blocks that a routing track spans
     * For nodes that are OPIN/IPIN/SOURCE/SINK, the length is supposed to be always 1
     */
    short node_length(const RRNodeId& node) const; 
    /* A short-cut function to get coordinates of a node */
    vtr::Rect<short> node_bounding_box(const RRNodeId& node) const;

    /* Get node starting and ending points in routing channels. 
     * See details in the figure for bounding box 
     * Applicable to routing track nodes only!!! 
     */
    vtr::Point<short> node_start_coordinate(const RRNodeId& node) const;
    vtr::Point<short> node_end_coordinate(const RRNodeId& node) const;

    /* Get the capacity of a node. 
     * Literally, how many nets can be mapped to the node. 
     * Typically, each node has a capacity of 1 but special nodes (SOURCE and SINK) will have a 
     * large number due to logic equivalent pins
     * See Vaughn Betz's book for more details
     */
    short node_capacity(const RRNodeId& node) const;
    short node_fan_in(const RRNodeId& node) const;
    short node_fan_out(const RRNodeId& node) const;

    /* Get the ptc_num of a node
     * ptc number is a feature number of each node for indexing by node type
     * The ptc_num carries different meanings for different node types
     * CHANX or CHANY: the track id in routing channels
     * OPIN or IPIN: the index of pins in the logic block data structure
     * SOURCE and SINK: does not matter
     * Due to a useful identifier, ptc_num is used in building fast look-up 
     */
    short node_ptc_num(const RRNodeId& node) const;
    short node_pin_num(const RRNodeId& node) const;
    short node_track_num(const RRNodeId& node) const;
    short node_class_num(const RRNodeId& node) const;

    /* Get the index of cost data in the list of cost_indexed_data data structure
     * It contains the routing cost for different nodes in the RRGraph
     * when used in evaluate different routing paths
     * See cross-reference section in this header file for more details
     */
    short node_cost_index(const RRNodeId& node) const;

    /* Get the directionality of a node
     * see node coordinate for details 
     * only matters the routing track nodes (CHANX and CHANY) 
     */
    e_direction node_direction(const RRNodeId& node) const;
    /* Get the side where the node physically locates on a logic block. 
     * Mainly applicable to IPIN and OPIN nodes, which locates on the perimeter of logic block 
     * The side should be consistent to <pinlocation> definition in architecture XML
     *
     *             TOP
     *        +-----------+
     *        |           |
     *  LEFT  |   Logic   | RIGHT
     *        |   Block   |
     *        |           |
     *        +-----------+
     *            BOTTOM
     *
     */
    e_side node_side(const RRNodeId& node) const;

    /* Get resistance of a node, used to built RC tree for timing analysis */
    float node_R(const RRNodeId& node) const;

    /* Get capacitance of a node, used to built RC tree for timing analysis */
    float node_C(const RRNodeId& node) const;

    /* Get segment id of a node, containing the information of the routing
     * segment that the node represents. See more details in the data structure t_segment_inf
     */
    RRSegmentId node_segment(const RRNodeId& node) const;

    /* Get the number of non-configurable incoming edges to a node */
    short node_num_configurable_in_edges(const RRNodeId& node) const;
    /* Get the number of non-configurable outgoing edges from a node */
    short node_num_non_configurable_in_edges(const RRNodeId& node) const;
    /* Get the number of configurable output edges of a node */
    short node_num_configurable_out_edges(const RRNodeId& node) const;
    /* Get the number of non-configurable output edges of a node */
    short node_num_non_configurable_out_edges(const RRNodeId& node) const;

    /* Get the range (list) of edges related to a given node */
    edge_range node_configurable_in_edges(const RRNodeId& node) const;
    edge_range node_non_configurable_in_edges(const RRNodeId& node) const;
    edge_range node_configurable_out_edges(const RRNodeId& node) const;
    edge_range node_non_configurable_out_edges(const RRNodeId& node) const;

    /* Get a list of edge ids, which are incoming edges to a node */
    edge_range node_in_edges(const RRNodeId& node) const;
    /* Get a list of edge ids, which are outgoing edges from a node */
    edge_range node_out_edges(const RRNodeId& node) const;

    /* Edge-related attributes 
     * An example to explain the terminology used in RRGraph
     *          edgeA
     *  nodeA --------> nodeB
     *        | edgeB
     *        +-------> nodeC
     *
     *  +----------+----------------+----------------+
     *  |  Edge Id |  edge_src_node | edge_sink_node |
     *  +----------+----------------+----------------+
     *  |   edgeA  |     nodeA      |      nodeB     |
     *  +----------+----------------+----------------+
     *  |   edgeB  |     nodeA      |      nodeC     |
     *  +----------+----------------+----------------+
     *
     */
    size_t edge_index(const RREdgeId& edge) const; /* TODO: deprecate this accessor as outside functions should use RREdgeId */
    /* Get the source node which drives a edge */
    RRNodeId edge_src_node(const RREdgeId& edge) const;
    /* Get the sink node which a edge ends to */
    RRNodeId edge_sink_node(const RREdgeId& edge) const;
    /* Get the switch id which a edge represents
     * using switch id, timing and other information can be found
     * for any node-to-node connection
     */
    RRSwitchId edge_switch(const RREdgeId& edge) const;
    /* Judge if a edge is configurable or not. 
     * A configurable edge is controlled by a programmable memory bit
     * while a non-configurable edge is typically a hard-wired connection
     * FIXME: need a double-check for the definition
     */
    bool edge_is_configurable(const RREdgeId& edge) const;
    bool edge_is_non_configurable(const RREdgeId& edge) const;

    /* Switch Info */
    size_t switch_index(const RRSwitchId& switch_id) const; /* TODO: deprecate this accessor as outside functions should use RRSwitchId */
    /* Get the switch info of a switch used in this RRGraph */
    const t_rr_switch_inf& get_switch(const RRSwitchId& switch_id) const;

    /* Segment Info */
    size_t segment_index(const RRSegmentId& segment_id) const; /* TODO: deprecate this accessor as outside functions should use RRSegmentId */
    /* Get the segment info of a routing segment used in this RRGraph */
    const t_segment_inf& get_segment(const RRSegmentId& segment_id) const;

    /* Utilities */
    /* Find the edges connecting two nodes */
    std::vector<RREdgeId> find_edges(const RRNodeId& src_node, const RRNodeId& sink_node) const;
    /* Find a node with given features from internal fast look-up */
    RRNodeId find_node(const short& x, const short& y, const t_rr_type& type, const int& ptc, const e_side& side = NUM_SIDES) const;
    /* Find the number of routing tracks in a routing channel with a given coordinate */
    short chan_num_tracks(const short& x, const short& y, const t_rr_type& type) const;

    /* Check if the graph contains invalid nodes/edges etc. */
    bool is_dirty() const;

  public:                                 /* Echos */
    void print_node(const RRNodeId& node) const; /* Print the detailed information of a node */

  public: /* Public Checkers */
    /* Full set checking using listed checking functions*/
    bool check() const;

  public: /* Public Validators */
    /* Validate is the node id does exist in the RRGraph */
    bool valid_node_id(const RRNodeId& node) const;
    /* Validate is the edge id does exist in the RRGraph */
    bool valid_edge_id(const RREdgeId& edge) const;

  public: /* Mutators */
    /* Reserve the lists of nodes, edges, switches etc. to be memory efficient. 
     * This function is mainly used to reserve memory space inside RRGraph,
     * when adding a large number of nodes/edge/switches/segments,
     * in order to avoid memory fragements
     * For example: 
     *    reserve_nodes(1000);
     *    for (size_t i = 0; i < 1000; ++i) {
     *      create_node();
     *    }
     */
    void reserve_nodes(const int& num_nodes);
    void reserve_edges(const int& num_edges);
    void reserve_switches(const int& num_switches);
    void reserve_segments(const int& num_segments);

    /* Add new elements (node, edge, switch, etc.) to RRGraph */
    /* Add a node to the RRGraph with a deposited type 
     * Detailed node-level information should be added using the set_node_* functions
     * For example: 
     *   RRNodeId node = create_node();
     *   set_node_xlow(node, 0);
     */
    RRNodeId create_node(const t_rr_type& type);
    /* Add a edge to the RRGraph, by providing the source and sink node 
     * This function will automatically create a node and
     * configure the nodes and edges in connection   
     */
    RREdgeId create_edge(const RRNodeId& source, const RRNodeId& sink, const RRSwitchId& switch_id);
    RRSwitchId create_switch(const t_rr_switch_inf& switch_info);
    RRSegmentId create_segment(const t_segment_inf& segment_info);

    /* Remove elements from RRGraph
     * this function just turn the nodes and edges to an invalid id without deletion
     * To thoroughly remove the edges and nodes, use compress() after the remove functions
     * Example: 
     *   remove_node()
     *   .. // remove more nodes 
     *   remove_edge()
     *   .. // remove more nodes 
     *   compress()
     */
    void remove_node(const RRNodeId& node);
    void remove_edge(const RREdgeId& edge);

    /* Set node-level information */
    void set_node_xlow(const RRNodeId& node, const short& xlow);
    void set_node_ylow(const RRNodeId& node, const short& ylow);
    void set_node_xhigh(const RRNodeId& node, const short& xhigh);
    void set_node_yhigh(const RRNodeId& node, const short& yhigh);
    /* This is a short-cut function, set xlow/xhigh/ylow/yhigh for a node
     * Please respect the following to configure the bb object:
     * bb.xmin = xlow, bb.ymin = ylow; bb.xmax = xhigh, bb.ymax = yhigh;
     */
    void set_node_bounding_box(const RRNodeId& node, const vtr::Rect<short>& bb);

    void set_node_capacity(const RRNodeId& node, const short& capacity);

    /* A generic function to set the ptc_num for a node */
    void set_node_ptc_num(const RRNodeId& node, const short& ptc);
    /* Only applicable to IPIN and OPIN, set the ptc_num for a node, which is the pin id in a logic block,
     * See definition in t_type_descriptor data structure   
     */
    void set_node_pin_num(const RRNodeId& node, const short& pin_id);
    /* Only applicable to CHANX and CHANY, set the ptc_num for a node, which is the track id in a routing channel,
     * Routing channel is a group of routing tracks, each of which has a unique index
     * An example 
     *       Routing Channel 
     *    +------------------+
     *    |                  |
     * ---|----------------->|----> track_id: 0
     *    |                  |
     *   ...   More tracks  ...
     *    |                  |
     * ---|----------------->|----> track_id: 1
     *    |                  |
     *    +------------------+
     */
    void set_node_track_num(const RRNodeId& node, const short& track_id);
    /* Only applicable to SOURCE and SINK, set the ptc_num for a node, which is the class number in a logic block,
     * See definition in t_type_descriptor data structure   
     */
    void set_node_class_num(const RRNodeId& node, const short& class_id);

    /* Set the routing cost index for node, see node_cost_index() for details */
    /* TODO the cost index should be changed to a StrongId!!! */
    void set_node_cost_index(const RRNodeId& node, const short& cost_index); 
    /* Set the directionality for a node, only applicable to CHANX and CHANY */
    void set_node_direction(const RRNodeId& node, const e_direction& direction);
    /* Set the side for a node, only applicable to OPIN and IPIN */
    void set_node_side(const RRNodeId& node, const e_side& side);
    /* Set the RC information for a node */
    void set_node_R(const RRNodeId& node, const float& R);
    void set_node_C(const RRNodeId& node, const float& C);
    /* Set the routing segment linked to a node, only applicable to CHANX and CHANY */
    void set_node_segment(const RRNodeId& node, const RRSegmentId& segment_index);

    /* Edge related */
    /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_in_edges(const RRNodeId& node);
    /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_out_edges(const RRNodeId& node);
    void partition_in_edges();  /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_out_edges(); /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_edges();     /* classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) */

    /* Graph-level Clean-up, remove invalid nodes/edges etc. */
    void compress();

    /* Graph-level validation, ensure a valid RRGraph for routers */
    bool validate();

    /* top-level function to free, should be called when to delete a RRGraph */
    void clear();

  private: /* Private Checkers */
    /* Node-level checking */
    bool check_node_segment(const RRNodeId& node) const;
    bool check_node_in_edges(const RRNodeId& node) const;
    bool check_node_out_edges(const RRNodeId& node) const;
    bool check_node_edges(const RRNodeId& node) const;

    /* Edge-level checking */
    bool check_node_is_edge_src(const RRNodeId& node, const RREdgeId& edge) const;
    bool check_node_is_edge_sink(const RRNodeId& node, const RREdgeId& edge) const;
    bool check_edge_switch(const RREdgeId& edge) const;
    bool check_edge_src_node(const RREdgeId& edge) const;
    bool check_edge_sink_node(const RREdgeId& edge) const;

    /* List-level checking */
    bool check_nodes_in_edges() const;
    bool check_nodes_out_edges() const;
    bool check_nodes_edges() const;
    bool check_node_segments() const;
    bool check_edge_switches() const;
    bool check_edge_src_nodes() const;
    bool check_edge_sink_nodes() const;

  private: /* Internal free functions */
    void clear_nodes();
    void clear_edges();
    void clear_switches();
    void clear_segments();

  private: /* Internal validators and builders */
    void set_dirty();
    void clear_dirty();

    /* Fast look-up builders and validators */
    void build_fast_node_lookup() const;
    void invalidate_fast_node_lookup() const;
    bool valid_fast_node_lookup() const;
    void initialize_fast_node_lookup() const;

    /* Graph property Validation */
    bool validate_sizes() const;
    bool validate_node_sizes() const;
    bool validate_edge_sizes() const;

    bool validate_invariants() const;
    bool validate_unique_edges_invariant() const;

    bool validate_crossrefs() const;
    bool validate_node_edge_crossrefs() const;

    /* Validate switch list */
    bool valid_switch_id(const RRSwitchId& switch_id) const;

    /* Validate segment list */
    bool valid_segment_id(const RRSegmentId& segment_id) const;

    /* Graph Compression related */
    void build_id_maps(vtr::vector<RRNodeId, RRNodeId>& node_id_map,
                       vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void clean_nodes(const vtr::vector<RRNodeId, RRNodeId>& node_id_map);
    void clean_edges(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void rebuild_node_refs(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);

  private: /* Internal Data */
    /* Node related data */
    vtr::vector<RRNodeId, RRNodeId> node_ids_; /* Unique identifiers for the nodes */
    vtr::vector<RRNodeId, t_rr_type> node_types_;

    vtr::vector<RRNodeId, vtr::Rect<short>> node_bounding_boxes_;

    vtr::vector<RRNodeId, short> node_capacities_;
    vtr::vector<RRNodeId, short> node_ptc_nums_;
    vtr::vector<RRNodeId, short> node_cost_indices_;
    vtr::vector<RRNodeId, e_direction> node_directions_;
    vtr::vector<RRNodeId, e_side> node_sides_;
    vtr::vector<RRNodeId, float> node_Rs_;
    vtr::vector<RRNodeId, float> node_Cs_;
    vtr::vector<RRNodeId, RRSegmentId> node_segments_; /* Segment ids for each node */
    /* Record the dividing point between configurable and non-configurable edges for each node */
    vtr::vector<RRNodeId, size_t> node_num_non_configurable_in_edges_;
    vtr::vector<RRNodeId, size_t> node_num_non_configurable_out_edges_;

    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_in_edges_;
    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_out_edges_;

    /* Edge related data */
    vtr::vector<RREdgeId, RREdgeId> edge_ids_; /* unique identifiers for edges */
    vtr::vector<RREdgeId, RRNodeId> edge_src_nodes_;
    vtr::vector<RREdgeId, RRNodeId> edge_sink_nodes_;
    vtr::vector<RREdgeId, RRSwitchId> edge_switches_;

    /* Switch related data
     * Note that so far there has been no need to remove
     * switches, so no such facility exists
     */
    /* Unique identifiers for switches which are used in the RRGraph */
    vtr::vector<RRSwitchId, RRSwitchId> switch_ids_;
    /* Detailed information about the switches, which are used in the RRGraph */
    vtr::vector<RRSwitchId, t_rr_switch_inf> switches_;

    /* Segment relatex data 
     * Segment info should be corrected annotated for each rr_node
     * whose type is CHANX and CHANY
     */
    vtr::vector<RRSegmentId, RRSegmentId> segment_ids_; /* unique identifiers for routing segments which are used in the RRGraph */
    vtr::vector<RRSegmentId, t_segment_inf> segments_;  /* detailed information about the segments, which are used in the RRGraph */

    /* Misc. */
    /* A flag to indicate if the graph contains invalid elements (nodes/edges etc.) */
    bool dirty_ = false;

    /* Fast look-up to search a node by its type, coordinator and ptc_num 
     * Indexing of fast look-up: [0..xmax][0..ymax][0..NUM_TYPES-1][0..ptc_max][0..NUM_SIDES-1] 
     */
    typedef std::vector<std::vector<std::vector<std::vector<std::vector<RRNodeId>>>>> NodeLookup;
    mutable NodeLookup node_lookup_;
};

#endif
