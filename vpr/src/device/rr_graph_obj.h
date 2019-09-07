/************************************************************************
 * This file introduces a class to model a Routing Resource Graph (RRGraph or RRG) 
 * which is widely used by placers, routers, analyzers etc.
 * A Routing Resource Graph (RRGraph or RRG) consists of a number of nodes and edges.
 *
 * Each node represents a routing resource, which could be 
 * 1. a routing track in X-direction or Y-direction (CHANX or CHANY) 
 * 2. an input or an output of a logic block (IPIN or OPIN) 
 * 3. a virtual source or sink node (SOURCE or SINK), which are starting/ending points of routing trees.
 *
 * Each edge represents a switch between routing resources, which could be
 * 1. a multiplexer
 * 2. a tri-state buffer
 * The switch information are categorized in the rr_switch_inf of RRGraph class.
 *
 * IMPORTANT:
 *   The RRGraph is designed to be a read-only database/graph, once created.
 *   Placement and routing should not change any attributes of RRGraph.
 *   Any placement and routing results should be stored in other data structures, such as PlaceContext and RoutingContext. 
 *
 * How to use the RRGraph data structure 
 * =====================================
 * To iterate over the nodes/edges/switches/segments in a RRGraph, 
 *    using a range-based loop is suggested.
 *  -----------------------------------------------------------------
 *    Example: iterate over all the nodes
 *      for (RRNodeId node : nodes()) {
 *        // Do something with each node
 *      }
 *
 *      for (RREdgeId edge : edges()) {
 *        // Do something with each edge
 *      }
 *
 *      for (RRSwitchId switch : switches()) {
 *        // Do something with each switch
 *      }
 *
 *      for (RRSegmentId segment : segments()) {
 *        // Do something with each segment
 *      }
 *
 * Access to a node/edge/switch/segment, please use the StrongId created
 *  -----------------------------------------------------------------
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
 *  
 * Detailed description on the RRGraph data structure
 * ==================================================
 *
 * Node-related data 
 *  -----------------------------------------------------------------
 * 1. node_ids_ : unique identifiers for the nodes
 *
 * 2. node_types_ : types of each node, can be 
 *                  channel wires (CHANX or CHANY) or 
 *                  logic block pins(OPIN or IPIN) or
 *                  virtual nodes (SOURCE or SINK)
 *                  see t_rr_type definition for more details
 *
 * 3. node_bounding_boxes_ : coordinator of a node. (xlow, xhigh, ylow, yhigh)
 *                           For OPIN/IPIN/SOURCE/SINK, xlow = xhigh and ylow = yhigh
 *                           FIXME: should confirm if this is still the case when a logic block has a height > 1
 *                           For CHANX/CHANY, (xlow, ylow) and (xhigh, yhigh) represent
 *                           where the routing segment starts and ends.
 *                           Note that our convention alway keeps
 *                           xlow <= xhigh and ylow <= yhigh
 *                           Therefore, (xlow, ylow) is a starting point for a CHANX/CHANY in INC_DIRECTION
 *                           (xhigh, yhigh) is a starting point for a CHANX/CHANY in INC_DIRECTION
 *                           
 *                        Example :
 *                        CHANX in INC_DIRECTION  
 *                        (xlow, ylow)                (xhigh, yhigh)
 *                             |                           |
 *                            \|/                         \|/
 *                             ---------------------------->
 *
 *                        CHANX in DEC_DIRECTION  
 *                        (xlow, ylow)                (xhigh, yhigh)
 *                             |                           |
 *                            \|/                         \|/
 *                             <----------------------------
 *
 *                        CHANY in INC_DIRECTION
 *                            
 *                           /|\ <-------(xhigh, yhigh)
 *                            |
 *                            |  <-------(xlow, ylow)
 *
 *                        CHANY in DEC_DIRECTION
 *                            
 *                            |  <-------(xhigh, yhigh)
 *                            |
 *                           \|/ <-------(xlow, ylow)
 *
 *
 * 4. node_capacities_ : the capacity of a node. How many nets can be mapped
 *                     to the node. Typically, each node has a capacity of 
 *                     1 but special nodes (SOURCE and SINK) will have a 
 *                     large number due to logic equivalent pins
 *
 * 5. node_ptc_nums_ : feature number of each node for indexing by node type
 *                     The ptc_num carries different meanings for different node types
 *                     CHANX or CHANY: the track id in routing channels
 *                     OPIN or IPIN: the index of pins in the logic block data structure
 *                     SOURCE and SINK: does not matter
 *                     Due to a useful identifier, ptc_num is used in building fast look-up 
 *
 * 6. node_cost_indices_: the index of cost data in the list of cost_indexed_data data structure
 *                        It contains the routing cost for different nodes in the RRGraph
 *                        when used in evaluate different routing paths
 *
 * 7. node_directions_ :  directionality of the node, only matters the routing track nodes
 *                        (CHANX and CHANY) 
 *
 * 8. node_sides_ : side of node on the perimeter of logic blocks, only applicable to 
 *                  OPIN and IPIN nodes. The side should be consistent to <pinlocation>
 *                  definition in architecture XML
 *
 * 9. node_Rs_ :  resistance of a node, used to built RC tree for timing analysis  
 *
 * 10. node_Rs_ :  capacitance of a node, used to built RC tree for timing analysis  
 *
 * 11. node_segments_ : segment id of a node, containing the information of the routing
 *                      segment that the node represents. See more details in the data
 *                      structure t_segment_inf
 *
 * 12. node_num_non_configurable_in_edges_ : number of non-configurable incoming edges 
 *                                           to a node
 *
 * 13. node_num_non_configurable_out_edges_ : number of non-configurable outgoing edges 
 *                                           from a node
 *
 * 14. node_in_edges_ : a list of edge ids, which are incoming edges to a node 
 *
 * 15. node_out_edges_ : a list of edge ids, which are outgoing edges from a node 
 *
 * Edge-related data:
 * 1. edge_ids_: unique identifiers for edges
 *
 * 2. edge_src_nodes_ : source node which drives a edge  
 *
 * 3. edge_sink_nodes_ : sink node which a edge ends to 
 *
 * 4. edge_switches_ : the switch id which a edge represents
 *                     using switch id, timing and other information can be found
 *                     for any node-to-node connection
 *
 * Switch-related data:
 *  -----------------------------------------------------------------
 * 1. switch_ids_ : unique identifiers for switches which are used in the RRGraph  
 *
 * 2. switches_: detailed information about the switches, which are used in the RRGraph
 *
 * Segment-related data:
 *  -----------------------------------------------------------------
 * 1. segment_ids_ : unique identifiers for routing segments which are used in the RRGraph  
 *
 * 2. segments_: detailed information about the segments, which are used in the RRGraph
 *
 * RRGraph compression
 *  -----------------------------------------------------------------
 * 1. dirty_ : an indicator showing if the RRGraph is compressed. 
 *
 *
 * Fast look-up
 *  -----------------------------------------------------------------
 * 1. node_lookup_: a fast look-up to quickly find a node in the RRGraph by its type, coordinator and ptc_num
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
  public: //Types
    typedef vtr::vector<RRNodeId, RRNodeId>::const_iterator node_iterator;
    typedef vtr::vector<RREdgeId, RREdgeId>::const_iterator edge_iterator;
    typedef vtr::vector<RRSwitchId, RRSwitchId>::const_iterator switch_iterator;
    typedef vtr::vector<RRSegmentId, RRSegmentId>::const_iterator segment_iterator;

    typedef vtr::Range<node_iterator> node_range;
    typedef vtr::Range<edge_iterator> edge_range;
    typedef vtr::Range<switch_iterator> switch_range;
    typedef vtr::Range<segment_iterator> segment_range;

  public: //Accessors
    //Aggregates
    node_range nodes() const;
    edge_range edges() const;
    switch_range switches() const;
    segment_range segments() const;

    //Node attributes
    size_t node_index(RRNodeId node) const; /* TODO: deprecate this accessor as outside functions should use RRNodeId */
    t_rr_type node_type(RRNodeId node) const;

    short node_xlow(RRNodeId node) const;
    short node_ylow(RRNodeId node) const;
    short node_xhigh(RRNodeId node) const;
    short node_yhigh(RRNodeId node) const;
    short node_length(RRNodeId node) const;
    vtr::Rect<short> node_bounding_box(RRNodeId node) const;
    /* Node starting and ending points */
    vtr::Point<short> node_start_coordinate(RRNodeId node) const;
    vtr::Point<short> node_end_coordinate(RRNodeId node) const;

    short node_capacity(RRNodeId node) const;
    short node_fan_in(RRNodeId node) const;
    short node_fan_out(RRNodeId node) const;

    short node_ptc_num(RRNodeId node) const;
    short node_pin_num(RRNodeId node) const;
    short node_track_num(RRNodeId node) const;
    short node_class_num(RRNodeId node) const;

    short node_cost_index(RRNodeId node) const;
    e_direction node_direction(RRNodeId node) const;
    e_side node_side(RRNodeId node) const;
    float node_R(RRNodeId node) const;
    float node_C(RRNodeId node) const;
    RRSegmentId node_segment(RRNodeId node) const; /* get the segment id of a rr_node */

    short node_num_configurable_in_edges(RRNodeId node) const;      /* get the number of configurable input edges of a node */
    short node_num_non_configurable_in_edges(RRNodeId node) const;  /* get the number of non-configurable input edges of a node */
    short node_num_configurable_out_edges(RRNodeId node) const;     /* get the number of configurable output edges of a node */
    short node_num_non_configurable_out_edges(RRNodeId node) const; /* get the number of non-configurable output edges of a node */

    /* Get the range (list) of edges related to a given node */
    edge_range node_configurable_in_edges(RRNodeId node) const;
    edge_range node_non_configurable_in_edges(RRNodeId node) const;
    edge_range node_configurable_out_edges(RRNodeId node) const;
    edge_range node_non_configurable_out_edges(RRNodeId node) const;

    edge_range node_out_edges(RRNodeId node) const;
    edge_range node_in_edges(RRNodeId node) const;

    //Edge attributes
    size_t edge_index(RREdgeId edge) const; /* TODO: deprecate this accessor as outside functions should use RREdgeId */
    RRNodeId edge_src_node(RREdgeId edge) const;
    RRNodeId edge_sink_node(RREdgeId edge) const;
    RRSwitchId edge_switch(RREdgeId edge) const;
    bool edge_is_configurable(RREdgeId edge) const;
    bool edge_is_non_configurable(RREdgeId edge) const;

    /* Switch Info */
    size_t switch_index(RRSwitchId switch_id) const; /* TODO: deprecate this accessor as outside functions should use RRSwitchId */
    const t_rr_switch_inf& get_switch(RRSwitchId switch_id) const;

    /* Segment Info */
    size_t segment_index(RRSegmentId segment_id) const; /* TODO: deprecate this accessor as outside functions should use RRSegmentId */
    const t_segment_inf& get_segment(RRSegmentId segment_id) const;

    //Utilities
    std::vector<RREdgeId> find_edges(RRNodeId src_node, RRNodeId sink_node) const;
    RRNodeId find_node(short x, short y, t_rr_type type, int ptc, e_side side = NUM_SIDES) const;
    short chan_num_tracks(short x, short y, t_rr_type type) const;

    bool is_dirty() const;

  public:                                 /* Echos */
    void print_node(RRNodeId node) const; /* Print the detailed information of a node */
  private:                                 /* Private Checkers */
    /* Node-level checking */
    bool check_node_segment(RRNodeId node) const;
    bool check_node_in_edges(RRNodeId node) const;
    bool check_node_out_edges(RRNodeId node) const;
    bool check_node_edges(RRNodeId node) const;

    /* Edge-level checking */
    bool check_node_is_edge_src(RRNodeId node, RREdgeId edge) const;
    bool check_node_is_edge_sink(RRNodeId node, RREdgeId edge) const;
    bool check_edge_switch(RREdgeId edge) const;
    bool check_edge_src_node(RREdgeId edge) const;
    bool check_edge_sink_node(RREdgeId edge) const;

    /* List-level checking */
    bool check_nodes_in_edges() const;
    bool check_nodes_out_edges() const;
    bool check_nodes_edges() const;
    bool check_node_segments() const;
    bool check_edge_switches() const;
    bool check_edge_src_nodes() const;
    bool check_edge_sink_nodes() const;

  public:                                 /* Public Checkers */
    /* Full set checking using listed checking functions*/
    bool check() const;

  public: //Validators
    bool valid_node_id(RRNodeId node) const;
    bool valid_edge_id(RREdgeId edge) const;

  public: //Mutators
    /* reserve the lists of nodes, edges, switches etc */
    void reserve_nodes(int num_nodes);
    void reserve_edges(int num_edges);
    void reserve_switches(int num_switches);
    void reserve_segments(int num_segments);

    /* Related to Nodes */
    RRNodeId create_node(t_rr_type type);
    RREdgeId create_edge(RRNodeId source, RRNodeId sink, RRSwitchId switch_id);
    RRSwitchId create_switch(t_rr_switch_inf switch_info);
    RRSegmentId create_segment(t_segment_inf segment_info);

    void remove_node(RRNodeId node);
    void remove_edge(RREdgeId edge);

    void set_node_xlow(RRNodeId node, short xlow);
    void set_node_ylow(RRNodeId node, short ylow);
    void set_node_xhigh(RRNodeId node, short xhigh);
    void set_node_yhigh(RRNodeId node, short yhigh);
    void set_node_bounding_box(RRNodeId node, vtr::Rect<short> bb);

    void set_node_capacity(RRNodeId node, short capacity);

    void set_node_ptc_num(RRNodeId node, short ptc);
    void set_node_pin_num(RRNodeId node, short pin_id);
    void set_node_track_num(RRNodeId node, short track_id);
    void set_node_class_num(RRNodeId node, short class_id);

    void set_node_cost_index(RRNodeId node, short cost_index);
    void set_node_direction(RRNodeId node, e_direction direction);
    void set_node_side(RRNodeId node, e_side side);
    void set_node_R(RRNodeId node, float R);
    void set_node_C(RRNodeId node, float C);
    void set_node_segment(RRNodeId node, RRSegmentId segment_index);

    /* Edge related */
    /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_in_edges(RRNodeId node);
    /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_out_edges(RRNodeId node);
    void partition_in_edges();  /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_out_edges(); /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_edges();     /* classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) */

    void compress();
    bool validate();

    void clear(); /* top-level function to free */

  private: //Internal free functions
    void clear_nodes();
    void clear_edges();
    void clear_switches();
    void clear_segments();

  private: //Internal
    void set_dirty();
    void clear_dirty();

    //Fast look-up
    void build_fast_node_lookup() const;
    void invalidate_fast_node_lookup() const;
    bool valid_fast_node_lookup() const;
    void initialize_fast_node_lookup() const;

    //Validation
    bool validate_sizes() const;
    bool validate_node_sizes() const;
    bool validate_edge_sizes() const;

    bool validate_invariants() const;
    bool validate_unique_edges_invariant() const;

    bool validate_crossrefs() const;
    bool validate_node_edge_crossrefs() const;

    /* For switch list */
    bool valid_switch_id(RRSwitchId switch_id) const;

    /* For segment list */
    bool valid_segment_id(RRSegmentId segment_id) const;

    //Compression related
    void build_id_maps(vtr::vector<RRNodeId, RRNodeId>& node_id_map,
                       vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void clean_nodes(const vtr::vector<RRNodeId, RRNodeId>& node_id_map);
    void clean_edges(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);
    void rebuild_node_refs(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map);

  private: //Data
    //Node related data
    vtr::vector<RRNodeId, RRNodeId> node_ids_;
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

    //Edge related data
    vtr::vector<RREdgeId, RREdgeId> edge_ids_;
    vtr::vector<RREdgeId, RRNodeId> edge_src_nodes_;
    vtr::vector<RREdgeId, RRNodeId> edge_sink_nodes_;
    vtr::vector<RREdgeId, RRSwitchId> edge_switches_;

    //Switch related data
    // Note that so far there has been no need to remove
    // switches, so no such facility exists
    vtr::vector<RRSwitchId, RRSwitchId> switch_ids_;
    vtr::vector<RRSwitchId, t_rr_switch_inf> switches_;

    /* Segment relatex data 
     * Segment info should be corrected annotated for each rr_node
     * whose type is CHANX and CHANY
     */
    vtr::vector<RRSegmentId, RRSegmentId> segment_ids_;
    vtr::vector<RRSegmentId, t_segment_inf> segments_;

    //Misc.
    bool dirty_ = false;

    //Fast look-up
    typedef std::vector<std::vector<std::vector<std::vector<std::vector<RRNodeId>>>>> NodeLookup;
    mutable NodeLookup node_lookup_; //[0..xmax][0..ymax][0..NUM_TYPES-1][0..ptc_max][0..NUM_SIDES-1]
};

#endif
