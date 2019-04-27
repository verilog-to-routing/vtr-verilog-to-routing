/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
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
    typedef vtr::vector<RRNodeId,RRNodeId>::const_iterator node_iterator;
    typedef vtr::vector<RREdgeId,RREdgeId>::const_iterator edge_iterator;
    typedef vtr::vector<RRSwitchId,RRSwitchId>::const_iterator switch_iterator;
    typedef vtr::vector<RRSegmentId,RRSegmentId>::const_iterator segment_iterator;

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
    size_t node_index(RRNodeId node) const;
    t_rr_type node_type(RRNodeId node) const;
    const char* node_type_string(RRNodeId node) const;

    short node_xlow(RRNodeId node) const;
    short node_ylow(RRNodeId node) const;
    short node_xhigh(RRNodeId node) const;
    short node_yhigh(RRNodeId node) const;
    short node_length(RRNodeId node) const;
    vtr::Rect<short> node_bounding_box(RRNodeId node) const;

    short node_capacity(RRNodeId node) const;
    short node_fan_in(RRNodeId node) const;
    short node_fan_out(RRNodeId node) const;

    short node_ptc_num(RRNodeId node) const;
    short node_pin_num(RRNodeId node) const;
    short node_track_num(RRNodeId node) const;
    short node_class_num(RRNodeId node) const;

    short node_cost_index(RRNodeId node) const;
    e_direction node_direction(RRNodeId node) const;
    const char* node_direction_string(RRNodeId node) const;
    e_side node_side(RRNodeId node) const;
    const char* node_side_string(RRNodeId node) const;
    float node_R(RRNodeId node) const;
    float node_C(RRNodeId node) const;
    short node_segment_id(RRNodeId node) const; /* get the segment id of a rr_node */

    short node_num_configurable_in_edges(RRNodeId node) const; /* get the number of configurable input edges of a node */
    short node_num_non_configurable_in_edges(RRNodeId node) const; /* get the number of non-configurable input edges of a node */
    short node_num_configurable_out_edges(RRNodeId node) const; /* get the number of configurable output edges of a node */
    short node_num_non_configurable_out_edges(RRNodeId node) const; /* get the number of non-configurable output edges of a node */

    /* Get the range (list) of edges related to a given node */
    edge_range node_configurable_in_edges(RRNodeId node) const;
    edge_range node_non_configurable_in_edges(RRNodeId node) const;
    edge_range node_configurable_out_edges(RRNodeId node) const;
    edge_range node_non_configurable_out_edges(RRNodeId node) const;

    edge_range node_out_edges(RRNodeId node) const;
    edge_range node_in_edges(RRNodeId node) const;

    //Edge attributes
    size_t edge_index(RREdgeId edge) const;
    RRNodeId edge_src_node(RREdgeId edge) const;
    RRNodeId edge_sink_node(RREdgeId edge) const;
    RRSwitchId edge_switch(RREdgeId edge) const;
    bool edge_is_configurable(RREdgeId edge) const;
    bool edge_is_non_configurable(RREdgeId edge) const;

    /* Switch Info */
    size_t switch_index(RRSwitchId switch_id) const;
    const t_rr_switch_inf& get_switch(RRSwitchId switch_id) const;

    /* Segment Info */
    size_t segment_index(RRSegmentId segment_id) const;
    const t_segment_inf& get_segment(RRSegmentId segment_id) const;

    //Utilities
    RREdgeId find_edge(RRNodeId src_node, RRNodeId sink_node) const;
    RRNodeId find_node(short x, short y, t_rr_type type, int ptc, e_side side=NUM_SIDES) const;
    node_range find_nodes(short x, short y, t_rr_type type, int ptc) const;

    void node_xy_deltas(RRNodeId from_node_id, RRNodeId to_node_id, 
                        int* delta_x, int* delta_y) const;
    RRNodeId get_chan_start_node_id(short start_x, short start_y, 
                                    short target_x, short target_y, 
                                    t_rr_type chan_type, 
                                    short seg_id, short track_offset) const ;


    bool is_dirty() const;

  public: /* Echo and checkers */
    void print_node(RRNodeId node) const; /* Print the detailed information of a node */
    /* Fundamental checking */
    bool check_node_duplicated_edges(RRNodeId node) const;
    bool check_duplicated_edges() const; /* identify and report any duplicated edges between two nodes */
    bool check_dangling_nodes() const; /* identify if there is any dangling nodes in the graph */

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
    void set_node_segment_id(RRNodeId node, short segment_index);

    /* Edge related */
    /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_in_edges(RRNodeId node); 
    /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_node_out_edges(RRNodeId node); 
    void partition_in_edges(); /* classify the input edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_out_edges(); /* classify the output edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void partition_edges(); /* classify the edges of each node to be configurable (1st part) and non-configurable (2nd part) */
    void load_switch_C(); /* configure the C of each node with the C of each incoming edge switch */
  
    void compress();
    bool validate();

    void clear_nodes();
    void clear_edges();
    void clear_switches();
    void clear_segments();
    void clear();
  private: //Internal
    void set_dirty();
    void clear_dirty();

    //Fast look-up
    void build_fast_node_lookup() const;
    void invalidate_fast_node_lookup() const;
    bool valid_fast_node_lookup() const;

    //Validation
    bool valid_node_id(RRNodeId node) const;
    bool valid_edge_id(RREdgeId edge) const;

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
    void build_id_maps(vtr::vector<RRNodeId,RRNodeId>& node_id_map,
                       vtr::vector<RREdgeId,RREdgeId>& edge_id_map);
    void clean_nodes(const vtr::vector<RRNodeId,RRNodeId>& node_id_map);
    void clean_edges(const vtr::vector<RREdgeId,RREdgeId>& edge_id_map);
    void rebuild_node_refs(const vtr::vector<RREdgeId,RREdgeId>& edge_id_map);

  private: //Data

    //Node related data
    vtr::vector<RRNodeId,RRNodeId> node_ids_;
    vtr::vector<RRNodeId,t_rr_type> node_types_;

    vtr::vector<RRNodeId,vtr::Rect<short>> node_bounding_boxes_;

    vtr::vector<RRNodeId,short> node_capacities_;
    vtr::vector<RRNodeId,short> node_ptc_nums_;
    vtr::vector<RRNodeId,short> node_cost_indices_;
    vtr::vector<RRNodeId,e_direction> node_directions_;
    vtr::vector<RRNodeId,e_side> node_sides_;
    vtr::vector<RRNodeId,float> node_Rs_;
    vtr::vector<RRNodeId,float> node_Cs_;
    vtr::vector<RRNodeId,short> node_segment_ids_; /* Segment ids for each node */
    /* Record the dividing point between configurable and non-configurable edges for each node */
    vtr::vector<RRNodeId,size_t> node_num_non_configurable_in_edges_; 
    vtr::vector<RRNodeId,size_t> node_num_non_configurable_out_edges_; 

    vtr::vector<RRNodeId,std::vector<RREdgeId>> node_in_edges_;
    vtr::vector<RRNodeId,std::vector<RREdgeId>> node_out_edges_;

    //Edge related data
    vtr::vector<RREdgeId,RREdgeId> edge_ids_;
    vtr::vector<RREdgeId,RRNodeId> edge_src_nodes_;
    vtr::vector<RREdgeId,RRNodeId> edge_sink_nodes_;
    vtr::vector<RREdgeId,RRSwitchId> edge_switches_;

    //Switch related data
    // Note that so far there has been no need to remove 
    // switches, so no such facility exists
    vtr::vector<RRSwitchId,RRSwitchId> switch_ids_;
    vtr::vector<RRSwitchId,t_rr_switch_inf> switches_;

    /* Segment relatex data 
     * Segment info should be corrected annotated for each rr_node
     * whose type is CHANX and CHANY
     */   
    vtr::vector<RRSegmentId,RRSegmentId> segment_ids_;
    vtr::vector<RRSegmentId,t_segment_inf> segments_;

    //Misc.
    bool dirty_ = false;

    //Fast look-up 
    typedef std::vector<std::vector<std::vector<std::vector<std::vector<RRNodeId>>>>> NodeLookup;
    mutable NodeLookup node_lookup_; //[0..xmax][0..ymax][0..NUM_TYPES-1][0..ptc_max][0..NUM_SIDES-1]
};

#endif
