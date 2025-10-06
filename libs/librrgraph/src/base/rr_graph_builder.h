#pragma once
/**
 * @file 
 * @brief This file defines the RRGraphBuilder data structure which allows data modification on a routing resource graph 
 *
 * The builder does not own the storage but it serves a virtual protocol for
 *   - node_storage: store the node list 
 *   - node_lookup: store a fast look-up for the nodes
 *
 * @note
 * - This is the only data structure allowed to modify a routing resource graph
 *
 */

#include "rr_graph_storage.h"
#include "rr_spatial_lookup.h"
#include "metadata_storage.h"
#include "rr_edge.h"

class RRGraphBuilder {
    /* -- Constructors -- */
  public:
    /* See detailed comments about the data structures in the internal data storage section of this file */
    RRGraphBuilder();

    /* Disable copy constructors and copy assignment operator
     * This is to avoid accidental copy because it could be an expensive operation considering that the 
     * memory footprint of the data structure could ~ Gb
     * Using the following syntax, we prohibit accidental 'pass-by-value' which can be immediately caught 
     * by compiler
     */
    RRGraphBuilder(const RRGraphBuilder&) = delete;
    void operator=(const RRGraphBuilder&) = delete;

    /* -- Mutators -- */
  public:
    /** @brief Return a writable object for rr_nodes */
    t_rr_graph_storage& rr_nodes();
    
    /** @brief Return a writable object for update the fast look-up of rr_node */
    RRSpatialLookup& node_lookup();
    
    /** @warning The Metadata should stay as an independent data structure from the rest of the internal data,
     *  e.g., node_lookup! */
    /** @brief Return a writable object for the meta data on the nodes */
    MetadataStorage<int>& rr_node_metadata();
    
    /** @brief Return a writable object for the meta data on the edge */
    MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata();
    
    /** @brief Return a writable object fo the incoming edge storage */
    vtr::vector<RRNodeId, std::vector<RREdgeId>>& node_in_edge_storage();
    
    /** @brief Return a writable object of the node ptc storage (for tileable routing resource graph) */
    vtr::vector<RRNodeId, std::vector<short>>& node_ptc_storage();

    /** @brief Return the size for rr_node_metadata */
    inline size_t rr_node_metadata_size() const {
        return rr_node_metadata_.size();
    }
    /** @brief Return the size for rr_edge_metadata */
    inline size_t rr_edge_metadata_size() const {
        return rr_edge_metadata_.size();
    }
    /** @brief Find the node in rr_node_metadata */
    inline vtr::flat_map<int, t_metadata_dict>::const_iterator find_rr_node_metadata(const int& lookup_key) const {
        return rr_node_metadata_.find(lookup_key);
    }
    /** @brief Find the edge in rr_edge_metadata */
    inline vtr::flat_map<std::tuple<int, int, short int>, t_metadata_dict>::const_iterator find_rr_edge_metadata(const std::tuple<int, int, short int>& lookup_key) const {
        return rr_edge_metadata_.find(lookup_key);
    }
    /** @brief Return the last node in rr_node_metadata */
    inline vtr::flat_map<int, t_metadata_dict>::const_iterator end_rr_node_metadata() const {
        return rr_node_metadata_.end();
    }

    /** @brief Return the last edge in rr_edge_metadata */
    inline vtr::flat_map<std::tuple<int, int, short int>, t_metadata_dict>::const_iterator end_rr_edge_metadata() const {
        return rr_edge_metadata_.end();
    }

    /** @brief Add a rr_segment to the routing resource graph. Return an valid id if successful.
     *  - Each rr_segment contains the detailed information of a routing track, which is denoted by a node in CHANX or CHANY type.
     * - It is frequently used by client functions in timing and routability prediction.
     */
    inline RRSegmentId add_rr_segment(const t_segment_inf& segment_info) {
        //Allocate an ID
        RRSegmentId segment_id = RRSegmentId(segment_ids_.size());
        segment_ids_.push_back(segment_id);

        rr_segments_.push_back(segment_info);

        return segment_id;
    }
    /** 
     * \internal
     * TODO
     * \endinternal
     *  @brief Return a writable list of all the rr_segments
     * @warning It is not recommended to use this API unless you have to. The API may be deprecated later, and future APIs will designed to return a specific data from the rr_segments.
     */
    inline vtr::vector<RRSegmentId, t_segment_inf>& rr_segments() {
        return rr_segments_;
    }

    /** @brief Add a rr_switch to the routing resource graph. Return an valid id if successful.
     *  - Each rr_switch contains the detailed information of a routing switch interconnecting two routing resource nodes.
     * - It is frequently used by client functions in timing prediction.
     */
    inline RRSwitchId add_rr_switch(const t_rr_switch_inf& switch_info) {
        //Allocate an ID
        RRSwitchId switch_id = RRSwitchId(rr_switch_inf_.size());

        rr_switch_inf_.push_back(switch_info);

        return switch_id;
    }
    /**
     * \internal 
     * TODO 
     * \endinternal
     * @brief Return a writable list of all the rr_switches
     * @warning It is not recommended to use this API unless you have to. The API may be deprecated later, and future APIs will designed to return a specific data from the rr_switches.
     */
    inline vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch() {
        return rr_switch_inf_;
    }

    /** @brief Set the type of a node with a given valid id */
    inline void set_node_type(RRNodeId id, e_rr_type type) {
        node_storage_.set_node_type(id, type);
    }

    /** @brief Create a new rr_node in the node storage and register it to the node look-up.
     * Return a valid node id if succeed. Otherwise, return an invalid id. This function is
     * currently only used when building the tileable rr_graph.
     */
    RRNodeId create_node(int layer, int x, int y, e_rr_type type, int ptc, e_side side = NUM_2D_SIDES); 

    /** @brief Set the node name with a given valid id */
    inline void set_node_name(RRNodeId id, std::string name) {
        node_storage_.set_node_name(id, name);
    }

    /**
     * @brief Add an existing rr_node in the node storage to the node look-up
     *
     * The node will be added to the lookup for every side it is on (for OPINs and IPINs) 
     * and for every (x,y) location at which it exists (for wires that span more than one (x,y)).
     *
     * This function requires a valid node which has already been allocated in the node storage, with
     *   - a valid node id
     *   - valid geometry information: xlow/ylow/xhigh/yhigh
     *   - a valid node type
     *   - a valid node ptc number
     *   - a valid side (applicable to OPIN and IPIN nodes only)
     */
    void add_node_to_all_locs(RRNodeId node);

    void init_edge_remap(bool val);

    void clear_temp_storage();

    /** @brief Clear all the underlying data storage */
    void clear();
    /** @brief reorder all the nodes
     * Reordering the rr-graph nodes may be helpful in
     *   - Increasing cache locality during routing
     *   - Improving compile time
     * Reorder RRNodeId's using one of these algorithms:
     *   - DEGREE_BFS: Order by degree primarily, and BFS traversal order secondarily.
     *   - RANDOM_SHUFFLE: Shuffle using the specified seed. Great for testing.
     * The DEGREE_BFS algorithm was selected because it had the best performance of seven
     * existing algorithms here: https://github.com/SymbiFlow/vtr-rrgraph-reordering-tool
     * It might be worth further research, as the DEGREE_BFS algorithm is simple and
     * makes some arbitrary choices, such as the starting node.
     * The re-ordering algorithm (DEGREE_BFS) does not speed up the router on most architectures
     * vs. using the node ordering created by the rr-graph builder in VPR, so it is off by default.
     * The other use of this algorithm is for some unit tests; by changing the order of the nodes
     * in the rr-graph before routing we check that no code depends on the rr-graph node order
     * Nonetheless, it does improve performance ~7% for the SymbiFlow Xilinx Artix 7 graph.
     *
     * NOTE: Re-ordering will invalidate any references to rr_graph nodes, so this
     *       should generally be called before creating such references.
     */
    void reorder_nodes(e_rr_node_reorder_algorithm reorder_rr_graph_nodes_algorithm,
                       int reorder_rr_graph_nodes_threshold,
                       int reorder_rr_graph_nodes_seed);

    /** @brief Set capacity of this node (number of routes that can use it). */
    inline void set_node_capacity(RRNodeId id, short new_capacity) {
        node_storage_.set_node_capacity(id, new_capacity);
    }

    /** @brief Set the node coordinate */
    inline void set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2) {
        node_storage_.set_node_coordinates(id, x1, y1, x2, y2);
    }

    /** @brief Set the tileable flag. This function is 
     * used by tileable routing resource graph builder 
     * only since the value of this flag is set to false by default.
     */
    inline void set_tileable(bool is_tileable) {
        node_storage_.set_tileable(is_tileable);
    }

    /**
     * @brief Set the bend start of a node
     * @param id The node id
     * @param bend_start The bend start
     */
    inline void set_node_bend_start(RRNodeId id, size_t bend_start) {
        node_storage_.set_node_bend_start(id, bend_start);
    }
    
    /**
     * @brief Set the bend end of a node
     * @param id The node id
     * @param bend_end The bend end
     */
    inline void set_node_bend_end(RRNodeId id, size_t bend_end) {
        node_storage_.set_node_bend_end(id, bend_end);
    }

    /** @brief The ptc_num carries different meanings for different node types
     * (true in VPR RRG that is currently supported, may not be true in customized RRG)
     * CHANX or CHANY: the track id in routing channels
     * OPIN or IPIN: the index of pins in the logic block data structure
     * SOURCE and SINK: the class id of a pin (indicating logic equivalence of pins) in the logic block data structure 
     * @note 
     * This API is very powerful and developers should not use it unless it is necessary,
     * e.g the node type is unknown. If the node type is known, the more specific routines, `set_node_pin_num()`,
     * `set_node_track_num()`and `set_node_class_num()`, for different types of nodes should be used.*/

    inline void set_node_ptc_num(RRNodeId id, int new_ptc_num) {
        node_storage_.set_node_ptc_num(id, new_ptc_num);
    }

    /// @brief Set the layer range where the given node spans.
    inline void set_node_layer(RRNodeId id, char layer_low, char layer_high){
        node_storage_.set_node_layer(id, layer_low, layer_high);
    }

    /** @brief set_node_pin_num() is designed for logic blocks, which are IPIN and OPIN nodes */
    inline void set_node_pin_num(RRNodeId id, int new_pin_num) {
        node_storage_.set_node_pin_num(id, new_pin_num);
    }

    /** @brief set_node_track_num() is designed for routing tracks, which are CHANX and CHANY nodes */
    inline void set_node_track_num(RRNodeId id, int new_track_num) {
        node_storage_.set_node_track_num(id, new_track_num);
    }

    // ** The following functions are only used for tileable routing resource graph generator **

    /** @brief Add a track id for a given node base on the offset in coordinate, applicable only to CHANX and CHANY nodes.
     * This API is used by tileable routing resource graph generator, which requires each routing track has a different
     * track id depending their location in FPGA fabric.
     * 
     * @param node The node to add the track id to.
     * @param node_offset Location of the portion of the node being considered. It is used
     *                    to calculate the relative location from the beginning of the node.
     * @param track_id The track id to add to the node.
     */
    void add_node_track_num(RRNodeId node, vtr::Point<size_t> node_offset, short track_id);

    /** @brief Update the node_lookup for a track node. This is applicable to tileable routing graph */
    void add_track_node_to_lookup(RRNodeId node);

    /** @brief set_node_class_num() is designed for routing source and sinks, which are SOURCE and SINK nodes */
    inline void set_node_class_num(RRNodeId id, int new_class_num) {
        node_storage_.set_node_class_num(id, new_class_num);
    }

    /** @brief set_node_mux_num() is designed for routing mux nodes */
    inline void set_node_mux_num(RRNodeId id, int new_mux_num) {
        node_storage_.set_node_mux_num(id, new_mux_num);
    }

    /** @brief Add a list of ptc number in string (split by comma) to a given node. This function is used by rr graph reader only. */
    void set_node_ptc_nums(RRNodeId node, const std::string& ptc_str);

    /** @brief With a given node, output ptc numbers into a string (use comma as delima). This function is used by rr graph writer only. */
    std::string node_ptc_nums_to_string(RRNodeId node) const;

    /** @brief Identify if a node contains multiple ptc numbers. It is used for tileable RR Graph and mainly used by I/O reader only. */
    bool node_contain_multiple_ptc(RRNodeId node) const;

    /** @brief Set the node direction; The node direction is only available of routing channel nodes, such as x-direction routing tracks (CHANX) and y-direction routing tracks (CHANY). For other nodes types, this value is not meaningful and should be set to NONE. */
    inline void set_node_direction(RRNodeId id, Direction new_direction) {
        node_storage_.set_node_direction(id, new_direction);
    }

    /** @brief Add a new edge to the cache of edges to be built 
     *  @note This will not add an edge to storage. You need to call build_edges() after all the edges are cached. */
    void create_edge_in_cache(RRNodeId src, RRNodeId dest, RRSwitchId edge_switch, bool remapped);

    /** @brief Add a new edge to the cache of edges to be built 
     *  @note This will not add an edge to storage! You need to call build_edges() after all the edges are cached! */
    void create_edge(RRNodeId src, RRNodeId dest, RRSwitchId edge_switch, bool remapped);

    /** @brief Allocate and build actual edges in storage. 
     * Once called, the cached edges will be uniquified and added to routing resource nodes, 
     * while the cache will be empty once build-up is accomplished 
     */
    void build_edges(const bool& uniquify = true);

    /** @brief Allocate and build incoming edges for each node. 
     * By default, no incoming edges are kept in storage, to be memory efficient
     * Currently, this function is only called when building the tileable rr_graph.
     */
    void build_in_edges();

    /** @brief Return incoming edges for a given routing resource node 
     *  Require build_in_edges() to be called first
     */
    std::vector<RREdgeId> node_in_edges(RRNodeId node) const;

    // ** End of functions for tileable routing resource graph generator **

    /** @brief Set the node id for clock network virtual sink */
    inline void set_virtual_clock_network_root_idx(RRNodeId virtual_clock_network_root_idx) {
        node_storage_.set_virtual_clock_network_root_idx(virtual_clock_network_root_idx);
    }

    /** @brief Reserve the lists of edges to be memory efficient.
     * This function is mainly used to reserve memory space inside RRGraph,
     * when adding a large number of edges in order to avoid memory fragments */
    inline void reserve_edges(size_t num_edges) {
        node_storage_.reserve_edges(num_edges);
    }

    /** @brief emplace_back_edge It adds one edge. This method is efficient if reserve_edges was called with
     * the number of edges present in the graph.
     * @param remapped If true, it means the switch id (edge_switch) corresponds to rr switch id. Thus, when the remapped function is called to
     * remap the arch switch id to rr switch id, the edge switch id of this edge shouldn't be changed. For example, when the intra-cluster graph
     * is built and the rr-graph related to global resources are read from a file, this parameter is true since the intra-cluster switches are
     * also listed in rr-graph file. So, we use that list to use the rr switch id instead of passing arch switch id for intra-cluster edges.*/
    inline void emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch, bool remapped) {
        node_storage_.emplace_back_edge(src, dest, edge_switch, remapped);
    }
    /** @brief Append 1 more RR node to the RR graph. */
    inline void emplace_back() {
        // No edges can be assigned if mutating the rr node array.
        node_storage_.emplace_back();
    }

    /** @brief alloc_and_load_edges; It adds a batch of edges.  */
    inline void alloc_and_load_edges(const t_rr_edge_info_set* rr_edges_to_create) {
        node_storage_.alloc_and_load_edges(rr_edges_to_create);
    }

    /** @brief Overrides the associated switch for a given edge by
     *         updating the edge to use the passed in switch. */
    inline void override_edge_switch(RREdgeId edge_id, RRSwitchId switch_id) {
        node_storage_.override_edge_switch(edge_id, switch_id);
    }

    /** @brief set_node_cost_index gets the index of cost data in the list of cost_indexed_data data structure
     * It contains the routing cost for different nodes in the RRGraph
     * when used in evaluate different routing paths
     */
    inline void set_node_cost_index(RRNodeId id, RRIndexedDataId new_cost_index) {
        node_storage_.set_node_cost_index(id, new_cost_index);
    }

    /** @brief Set the rc_index of routing resource node. */
    inline void set_node_rc_index(RRNodeId id, NodeRCIndex new_rc_index) {
        node_storage_.set_node_rc_index(id, new_rc_index);
    }

    /** @brief Add the side where the node physically locates on a logic block.
     * Mainly applicable to IPIN and OPIN nodes.*/
    inline void add_node_side(RRNodeId id, e_side new_side) {
        node_storage_.add_node_side(id, new_side);
    }

    /** @brief It maps arch_switch_inf indices to rr_switch_inf indices. */
    inline void remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin) {
        node_storage_.remap_rr_node_switch_indices(switch_fanin);
    }

    /** @brief Marks that edge switch values are rr switch indices*/
    inline void mark_edges_as_rr_switch_ids() {
        node_storage_.mark_edges_as_rr_switch_ids();
    }

    /** @brief Counts the number of rr switches needed based on fan in to support mux
     * size dependent switch delays. */
    inline size_t count_rr_switches(
        const std::vector<t_arch_switch_inf>& arch_switch_inf,
        t_arch_switch_fanin& arch_switch_fanins) {
        return node_storage_.count_rr_switches(arch_switch_inf, arch_switch_fanins);
    }

    /** @brief Unlock storage; required to modify an routing resource graph after edge is read */
    inline void unlock_storage() {
        node_storage_.edges_read_ = false;
        node_storage_.partitioned_ = false;
        node_storage_.clear_node_first_edge();
    }

    /** @brief Reserve the lists of nodes, edges, switches etc. to be memory efficient.
     * This function is mainly used to reserve memory space inside RRGraph,
     * when adding a large number of nodes/edge/switches/segments,
     * in order to avoid memory fragments */
    inline void reserve_nodes(size_t size) {
        node_storage_.reserve(size);
    }
    inline void reserve_segments(size_t num_segments) {
        this->segment_ids_.reserve(num_segments);
        this->rr_segments_.reserve(num_segments);
    }
    inline void reserve_switches(size_t num_switches) {
        this->rr_switch_inf_.reserve(num_switches);
    }
    /** @brief This function resize node storage to accomidate size RR nodes. */
    inline void resize_nodes(size_t size) {
        node_storage_.resize(size);
    }
    /** @brief This function resize node ptc nums. Only used by RR graph I/O reader and writers. */
    inline void resize_node_ptc_nums(size_t size) {
        node_tilable_track_nums_.resize(size);
    }


    /** @brief This function resize rr_switch to accomidate size RR Switch. */
    inline void resize_switches(size_t size) {
        rr_switch_inf_.resize(size);
    }

    /** @brief Validate that edge data is partitioned correctly. This function should be called
     * when all edges in cache are added.
     * @note This function is used to validate the correctness of the routing resource graph in terms
     * of graph attributes. Strongly recommend to call it when you finish the building a routing resource
     * graph. If you need more advance checks, which are related to architecture features, you should
     * consider to use the check_rr_graph() function or build your own check_rr_graph() function. */
    inline bool validate() const {
        return node_storage_.validate(rr_switch_inf_) && edges_to_build_.empty();
    }

    /** @brief Sorts edge data such that configurable edges appears before
     *  non-configurable edges. */
    inline void partition_edges() {
        return node_storage_.partition_edges(rr_switch_inf_);
    }

    /** @brief Init per node fan-in data.  Should only be called after all edges have
     * been allocated.
     * @note
     * This is an expensive, O(N), operation so it should be called once you
     * have a complete rr-graph and not called often. */
    inline void init_fan_in() {
        node_storage_.init_fan_in();
    }

    /** @brief Disable the flags which would prevent adding adding extra-resources, when flat-routing
     * is enabled, to the RR Graph
     * @note
     * When flat-routing is enabled, intra-cluster resources are added to the RR Graph after global rosources
     * are already added. This function disables the flags which would prevent adding extra-resources to the RR Graph
     */
    inline void reset_rr_graph_flags() {
        node_storage_.edges_read_ = false;
        node_storage_.partitioned_ = false;
        node_storage_.remapped_edges_ = false;
        node_storage_.clear_node_first_edge();
    }

    /* -- Internal data storage -- */
  private:
    /* TODO: When the refactoring effort finishes, 
     * the builder data structure will be the owner of the data storages. 
     * That is why the reference to storage/lookup is used here.
     * It can avoid a lot of code changes once the refactoring is finished 
     * (there is no function get data directly through the node_storage in DeviceContext).
     * If pointers are used, it may cause many codes in client functions 
     * or inside the data structures to be changed later.
     * That explains why the reference is used here temporarily
     */
    /* node-level storage including edge storages */
    t_rr_graph_storage node_storage_;
    /* Fast look-up for rr nodes */
    RRSpatialLookup node_lookup_;

    /** 
     * @brief A cache for edge-related information, required to build edges for routing resource nodes.
     * @note It is used when building a routing resource graph. It is a set of edges that have not yet been 
     * added to the main rr-graph edge storage to avoid an expensive edge-by-edge reallocation or re-shuffling 
     * of edges in the main rr-graph edge storage.
     * 
     * @note It will be cleared after calling build_edges().
     * 
     * @note This data structure is only used for tileable routing resource graph generator.
     *
     * @warning This is a temporary data which is used to collect edges to be built for nodes
     */
    t_rr_edge_info_set edges_to_build_;

    /** 
     * @brief Wire segment types in RR graph
     * @details 
     * - Each rr_segment contains the detailed information of a routing track, which is denoted by a node in CHANX or CHANY type.
     * - We use a fly-weight data structure here, in the same philosophy as the rr_indexed_data. See detailed explanation in the t_segment_inf data structure
     */
    vtr::vector<RRSegmentId, t_segment_inf> rr_segments_; /* detailed information about the segments, which are used in the RRGraph */

    /** 
     * @brief Unique identifiers for routing segments which are used in the RRGraph
     */
    vtr::vector<RRSegmentId, RRSegmentId> segment_ids_;

    /** 
     * @brief Autogenerated in build_rr_graph based on switch fan-in.
     * @details 
     *  - Each rr_switch contains the detailed information of a routing switch interconnecting two routing resource nodes.
     *  - We use a fly-weight data structure here, in the same philosophy as the rr_indexed_data. See detailed explanation in the t_rr_switch_inf data structure
     */
    vtr::vector<RRSwitchId, t_rr_switch_inf> rr_switch_inf_;

    /** 
     * @brief A list of incoming edges for each routing resource node. 
     * @note This can be built optionally, as required by applications.
     * By default, it is empty! Call build_in_edges() to construct it.
     */
    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_in_edges_;

    /** 
     * @brief Extra ptc number for each routing resource node. 
     * @note This is required by tileable routing resource graphs. The first index is the node id, and
     * the second index is is the relative distance from the starting point of the node.
     * @details 
     * In a tileable routing architecture, routing tracks, e.g., CHANX and CHANY, follow a staggered organization.
     * Hence, a routing track may appear in different routing channels, representing different ptc/track id.
     * Here is an illustrative example of a X-direction routing track (CHANX) in INC direction, which is organized in staggered way.
     *    
     *  Coord(x,y) (1,0)   (2,0)   (3,0)     (4,0)       Another track (node)
     *  ptc=0     ------>                              ------>
     *                   \                            /
     *  ptc=1             ------>                    /
     *                           \                  /
     *  ptc=2                     ------>          / 
     *                                   \        /
     *  ptc=3                             ------->
     *           ^                               ^
     *           |                               |
     *     starting point                   ending point
     */
    vtr::vector<RRNodeId, std::vector<short>> node_tilable_track_nums_;

    /** @warning The Metadata should stay as an independent data structure from the rest of the internal data,
     *  e.g., node_lookup! */
    /* Metadata is an extra data on rr-nodes and edges, respectively, that is not used by vpr
     * but simply passed through the flow so that it can be used by downstream tools.
     * The main (perhaps only) current use of this metadata is the fasm tool of symbiflow,
     * which needs extra metadata on which programming bits control which switch in order to produce a bitstream.*/
    
    /**
     * @brief Attributes for each rr_node.
     *
     * key:     rr_node index
     * value:   map of <attribute_name, attribute_value>
     */
    MetadataStorage<int> rr_node_metadata_;
    /**
     * @brief  Attributes for each rr_edge
     *
     * key:     <source rr_node_index, sink rr_node_index, iswitch>
     * iswitch: Index of the switch type used to go from this rr_node to
     *          the next one in the routing.  OPEN if there is no next node
     *          (i.e. this node is the last one (a SINK) in a branch of the
     *          net's routing).
     * value:   map of <attribute_name, attribute_value>
     */
    MetadataStorage<std::tuple<int, int, short>> rr_edge_metadata_;

    /** 
     * @brief This flag indicates if all the edges in cache are added to the main rr-graph edge storage.
     * To add all edges in cache to the main rr-graph edge storage, call build_edges().
     */
    bool is_edge_dirty_;

    /**
     * @brief This flag indicates whether node_in_edges_ is updated with 
     * edges in the main rr-graph edge storage.
     */
    bool is_incoming_edge_dirty_;
};
