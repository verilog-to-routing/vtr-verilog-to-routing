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
 * A Routing Resource Graph (RRGraph or RRG) is a directed graph (has many cycles),
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
 * 3. a pass gate
 * 4. a non-configurable (can not be turned off) buffer
 * 5. a short (metal connection that can not be turned off
 *
 * Note: Multiplexers are the most common type
 *
 * The switch information are categorized in the rr_switch_inf of RRGraph class.
 * rr_switch_inf is created to minimize memory footprint of RRGraph classs
 * While the RRG could contain millions (even much larger) of edges, there are only
 * a limited number of types of switches.
 * Hence, we use a flyweight pattern to store switch-related information that differs
 * only for types of switches (switch type, drive strength, R, C, etc.).
 * Each edge stores the ids of the switch that implements it so this additional information
 * can be easily looked up.
 *
 * Note: All the switch-related information, such as R, C, should be placed in rr_switch_inf
 * but NOT directly in the edge-related data of RRGraph. 
 * If you wish to create a new data structure to represent switches between routing resources,
 * please follow the flyweight pattern by linking your switch ids to edges only!
 *
 * Guidlines on using the RRGraph data structure 
 * =============================================
 *
 * For those want to access data from RRGraph
 * ------------------------------------------
 * Some examples for most frequent data query:
 * 
 *     // Strongly suggest to use a read-only rr_graph object
 *     const RRGraph& rr_graph;
 *
 *     // Access type of a node with a given node id
 *     // Get the unique node id that you may get 
 *     // from other data structures or functions 
 *     RRNodeId node_id;                                    
 *     t_rr_type node_type = rr_graph.node_type(node_id);
 *
 *     // Access all the fan-out edges from a given node  
 *     for (const RREdgeId& out_edge_id : rr_graph.node_out_edges(node_id)) {
 *       // Do something with out_edge
 *     }
 *     // If you only want to learn the number of fan-out edges
 *     size_t num_out_edges = rr_graph.node_fan_out(node_id);
 *
 *     // Access all the switches connected to a given node  
 *     for (const RREdgeId& in_edge_id : rr_graph.node_in_edges(node_id)) {
 *       RRSwitchId edge_switch_id = rr_graph.edge_switch(in_edge_id);
 *       // Do something with the switch
 *     }
 *
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
 * The validate() function gurantees the consistency between internal data structures, 
 * such as the id cross-reference between nodes and edges etc.,
 * so failing it indicates a fatal bug!
 * This is a must-do check! 
 *
 * Example: 
 *    RRGraph rr_graph;
 *    ... // Building RRGraph
 *    rr_graph.validate(); 
 *
 * Optionally, we strongly recommend developers to run an advance check in check_rr_graph()  
 * This guarantees legal and routable RRGraph for VPR routers.
 *
 * This checks for connectivity or other information in the RRGraph that is unexpected 
 * or unusual in an FPGA, and likely indicates a problem in your graph generation. 
 * However, if you are intentionally creating an RRGraph with this unusual, 
 * buts still technically legal, behaviour, you can write your own check_rr_graph() with weaker assumptions. 
 *
 * Note: Do NOT modify the coordinate system for nodes, they are designed for downstream drawers and routers
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
 * The rr-graph is the single largest data structure in VPR,
 * so avoid adding unnecessary information per node or per edge to it, as it will impact memory footprint.
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
 *  |        |  edge_src_node   |        |  |
 *  +--------+  edge_sink_node  +--------+  |edge_switch
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
 * This allows rapid look up by the router of additional information it needs for this node, using a flyweight pattern.
 *
 * +---------+  cost_index   +------------------------------+
 * | RRGraph |-------------->| cost_index_data[cost_index] | 
 * +---------+               +------------------------------+
 *
 * Note: if you wish to use a customized routing-cost data structure, 
 * please use the flyweigth pattern as we do for t_rr_index_data!
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

/* VPR header files go third */
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
     *      // Strongly suggest to use a read-only rr_graph object
     *      const RRGraph& rr_graph;
     *      for (const RRNodeId& node : rr_graph.nodes()) {
     *        // Do something with each node
     *      }
     *
     *      for (const RREdgeId& edge : rr_graph.edges()) {
     *        // Do something with each edge
     *      }
     *
     *      for (const RRSwitchId& switch : rr_graph.switches()) {
     *        // Do something with each switch
     *      }
     *
     *      for (const RRSegmentId& segment : rr_graph.segments()) {
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
     *   This is still the case when a logic block has a height > 1
     *   For CHANX/CHANY, (xlow, ylow) and (xhigh, yhigh) represent
     *   where the routing segment starts and ends.
     *   Note that our convention alway keeps
     *   xlow <= xhigh and ylow <= yhigh
     *   Therefore, (xlow, ylow) is a starting point for a CHANX/CHANY in INC_DIRECTION
     *   (xhigh, yhigh) is a starting point for a CHANX/CHANY in DEC_DIRECTION
     *
     *   Note: there is only a single drive point for each routing segment (track)
     *   in the context of uni-directional wires
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
     * For nodes that are OPIN/IPIN/SOURCE/SINK, the length is supposed to be always 0
     */
    short node_length(const RRNodeId& node) const;

    /* A short-cut function to get coordinates of a node,
     * where xmin of vtr::Rect = xlow, 
     *       ymin of vtr::Rect = ylow,
     *       xmax of vtr::Rect = xhigh, 
     *       ymax of vtr::Rect = yhigh. 
     */
    vtr::Rect<short> node_bounding_box(const RRNodeId& node) const;

    /* Get node starting and ending points in routing channels. 
     * See details in the figures for node_xlow(), node_ylow(), node_xhigh() and node_yhigh()
     * For routing tracks in INC_DIRECTION:
     * node_start_coordinate() returns (xlow, ylow) as the starting point 
     * node_end_coordinate() returns (xhigh, yhigh) as the ending point 
     *
     * For routing tracks in DEC_DIRECTION:
     * node_start_coordinate() returns (xhigh, yhigh) as the starting point 
     * node_end_coordinate() returns (xlow, ylow) as the ending point 
     *
     * For routing tracks in BI_DIRECTION:
     * node_start_coordinate() returns (xlow, ylow) as the starting point 
     * node_end_coordinate() returns (xhigh, yhigh) as the ending point 
     *
     * Applicable to routing track nodes only!!! 
     * This function will requires the types of node to be either CHANX or CHANY
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

    /* Get the number of edges that drives a given node
     * Note that each edge is typically driven by a node
     * (true in VPR RRG that is currently supported, may not be true in customized RRG)
     * This can also represent the number of drive nodes 
     * An example where fan-in of a node C is 2
     *      
     *                edge A
     *        node A -------->+
     *                        |
     *                        |-->node C 
     *                 edgeB  |
     *        node B -------->+
     *  
     */
    short node_fan_in(const RRNodeId& node) const;

    /* Get the number of edges that are driven a given node
     * Note that each edge typically drives by a node 
     * (true in VPR RRG that is currently supported, may not be true in customized RRG)
     * This can also represent the number of fan-out nodes 
     * An example where fan-out of a node A is 2
     *      
     *                  edge A
     *        node A -+--------> node B
     *                |
     *                |     
     *                | edgeB  
     *                +--------> node C
     *  
     */
    short node_fan_out(const RRNodeId& node) const;

    /* Get the ptc_num of a node
     * The ptc (pin, track, or class) number is an integer 
     * that allows you to differentiate between wires, pins or sources/sinks with overlapping x,y coordinates or extent. 
     * This is useful for drawing rr-graphs nicely. 
     * For example, all the CHANX rr_nodes that have the same y coordinate and x-coordinates 
     * that overlap will have different ptc numbers, by convention starting at 0. 
     * This allows them to be drawn without overlapping, as the ptc num can be used as a track identifier. 
     * The main routing code does not care about ptc num.
     *
     * The ptc_num carries different meanings for different node types
     * (true in VPR RRG that is currently supported, may not be true in customized RRG)
     * CHANX or CHANY: the track id in routing channels
     * OPIN or IPIN: the index of pins in the logic block data structure
     * SOURCE and SINK: the class id of a pin (indicating logic equivalence of pins) in the logic block data structure
     *  
     * To ease the access to ptc_num for different types of nodes:
     * node_pin_num() is designed for logic blocks, which are IPIN and OPIN nodes
     * node_track_num() is designed for routing tracks, which are CHANX and CHANY nodes
     * node_class_num() is designed for routing source and sinks, which are SOURCE and SINK nodes
     *
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

    /* This flag is raised when the RRgraph contains invalid nodes/edges etc. 
     * Invalid nodes/edges exist when users remove nodes/edges from RRGraph
     * RRGraph object will not immediately remove the nodes and edges but
     * will mark them with invalid ids.
     * Afterwards the is_dirty flag is raised as an indicator, to tell users
     * that a clean-up process (invoked by compress() function)is required.
     * After executing compress(), the is_dirty will be reset to false
     *
     * Example:
     *   RRGraph rr_graph;
     *   RRNodeId node_id; // Node id to be removed
     *   rr_graph.remove_node(node_id); 
     *   // RRGraph is now dirty (rr_graph.is_dirty() == true)
     *   ...
     *   // During this period, when you perform data query,
     *   // You may encounter invalid nodes and edges
     *   // It may happen that 
     *   // 1. their ids are invalid 
     *   // 2. the cross-reference between nodes and edge, i.e.,
     *   //    node_in_edges(), node_out_edges() 
     *   // 3. invalid node returned from find_node(), which use fast look-up
     *   ... 
     *   rr_graph.compress(); 
     *   // RRGraph is now clean (rr_graph.is_dirty() == false)
     */
    bool is_dirty() const;

  public:                                        /* Echos */
    void print_node(const RRNodeId& node) const; /* Print the detailed information of a node */

  public: /* Public Validators */
    /* Check data structure for internal consistency
     * This function will 
     * 1. re-build fast look-up for nodes
     * 2. check all the edges and nodes are connected
     *    Literally, no invalid ids in fan-in/fan-out of edges/nodes
     * 3. check that all the nodes representing routing tracks have valid id linked to its segment-related data structure
     * 4. check that all the edges have valid id linked to its switch-related data structure 
     *
     * Any valid and non-dirty RRGraph should pass this check
     * It is highly recommended to run this function after building any RRGraph object
     */
    bool validate() const;

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
     *    RRGraph rr_graph;
     *    // Add 1000 CHANX nodes to the RRGraph object
     *    rr_graph.reserve_nodes(1000);
     *    for (size_t i = 0; i < 1000; ++i) {
     *      rr_graph.create_node(CHANX);
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
     * This function just turn the nodes and edges to an invalid id without deletion
     * This will cause the RRGraph to be marked as dirty (is_dirty(): false => true)
     * To thoroughly remove the edges and nodes as well as clear the dirty flag,
     * use compress() after the remove functions
     * Example: 
     *   RRGraph rr_graph;
     *   rr_graph.remove_node()
     *   .. // remove more nodes 
     *   rr_graph.remove_edge()
     *   .. // remove more nodes 
     *   // rr_graph.is_dirty() == true
     *   // If you want to access any node or edge
     *   // Check codes for ids should be intensively used
     *   // Such as, 
     *   // for (const RREdgeId& in_edge : rr_graph.node_in_edges()) {
     *   //   if (RREdgeId::INVALID() != in_edge) {
     *   //     ...
     *   //   }
     *   // }
     *   rr_graph.compress()
     *   // rr_graph.is_dirty() == false
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
    /* TODO: the cost index should be changed to a StrongId!!! */
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

    /* Edge partitioning is performed for efficiency, 
     * so we can store configurable and non-configurable edge lists for a node in one vector, 
     * and efficiently iterate over all edges, or only the configurable or non-configurable subsets.
     * This function will re-organize the incoming and outgoing edges of each node, 
     * i.e., node_in_edges() and node_out_edges():
     * 1. configurable edges (1st part of the vectors) 
     * 2. non-configurable edges (2nd part of the vectors) 
     */
    void partition_edges();

    /* Graph-level Clean-up, remove invalid nodes/edges etc.
     * This will clear the dirty flag (query by is_dirty()) of RRGraph object, if it was set 
     */
    void compress();

    /* top-level function to free, should be called when to delete a RRGraph */
    void clear();

  private: /* Internal Mutators to perform edge partitioning */
    /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_in_edges(const RRNodeId& node);

    /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_out_edges(const RRNodeId& node);

    /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_in_edges();

    /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_out_edges();

  private: /* Internal free functions */
    void clear_nodes();
    void clear_edges();
    void clear_switches();
    void clear_segments();

  private: /* Graph Compression related */
    void build_id_maps(vtr::vector<RRNodeId, RRNodeId>& node_id_map,
                       vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void clean_nodes(const vtr::vector<RRNodeId, RRNodeId>& node_id_map);
    void clean_edges(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void rebuild_node_refs(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);

    void set_dirty();
    void clear_dirty();

  private: /* Internal validators and builders */
    /* Fast look-up builders and validators */
    void build_fast_node_lookup() const;
    void invalidate_fast_node_lookup() const;
    bool valid_fast_node_lookup() const;
    void initialize_fast_node_lookup() const;

    /* Graph property Validation */
    bool validate_sizes() const;
    bool validate_node_sizes() const;
    bool validate_edge_sizes() const;
    bool validate_switch_sizes() const;
    bool validate_segment_sizes() const;

    bool validate_invariants() const;
    bool validate_unique_edges_invariant() const;

    bool validate_crossrefs() const;
    bool validate_node_edge_crossrefs() const;

    /* Node-level validators */
    bool validate_node_segment(const RRNodeId& node) const;
    bool validate_node_in_edges(const RRNodeId& node) const;
    bool validate_node_out_edges(const RRNodeId& node) const;
    bool validate_node_edges(const RRNodeId& node) const;

    /* Edge-level checking */
    bool validate_node_is_edge_src(const RRNodeId& node, const RREdgeId& edge) const;
    bool validate_node_is_edge_sink(const RRNodeId& node, const RREdgeId& edge) const;
    bool validate_edge_switch(const RREdgeId& edge) const;
    bool validate_edge_src_node(const RREdgeId& edge) const;
    bool validate_edge_sink_node(const RREdgeId& edge) const;

    /* List-level checking */
    bool validate_nodes_in_edges() const;
    bool validate_nodes_out_edges() const;
    bool validate_nodes_edges() const;
    bool validate_node_segments() const;
    bool validate_edge_switches() const;
    bool validate_edge_src_nodes() const;
    bool validate_edge_sink_nodes() const;

    /* Validate switch list */
    bool valid_switch_id(const RRSwitchId& switch_id) const;

    /* Validate segment list */
    bool valid_segment_id(const RRSegmentId& segment_id) const;

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
