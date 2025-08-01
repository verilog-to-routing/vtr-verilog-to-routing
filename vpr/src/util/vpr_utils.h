#pragma once

#include "arch_util.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "rr_graph_utils.h"
#include "vpr_types.h"
#include "vtr_vector.h"
#include "atom_pb_bimap.h"
#include <list>
#include <string>
#include <vector>

// Forward declaration
class DeviceGrid;
class UserRouteConstraints;

void print_tabs(FILE* fpout, int num_tab);

bool is_clb_external_pin(ClusterBlockId blk_id, int pb_pin_id);

bool is_empty_type(t_physical_tile_type_ptr type);
bool is_empty_type(t_logical_block_type_ptr type);

/**
 * @brief Returns the corresponding physical type given the location in the grid.
 * @param loc The block location in the grid.
 * @return The physical tile type of the given location.
 */
t_physical_tile_type_ptr physical_tile_type(t_pl_loc loc);

t_physical_tile_type_ptr physical_tile_type(AtomBlockId atom_blk);

t_physical_tile_type_ptr physical_tile_type(ParentBlockId blk_id, bool is_flat);

//Returns the sub tile corresponding to the logical block location within a physical type
int get_sub_tile_index(ClusterBlockId blk,
                       const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

int get_sub_tile_index(ClusterBlockId blk);

int get_unique_pb_graph_node_id(const t_pb_graph_node* pb_graph_node);

//Returns the physical class range relative to a block id. This must be called after placement
//as the block id is used to retrieve the information of the used physical tile.
t_class_range get_class_range_for_block(const ClusterBlockId blk_id);

t_class_range get_class_range_for_block(const AtomBlockId atom_blk);

t_class_range get_class_range_for_block(const ParentBlockId blk_id, bool is_flat);

//
/**
 * @brief Returns the physical pin range relative to a block id. This must be called after placement
 * as the block id is used to retrieve the information of the used physical tile.
 *
 * @param blk_id The unique ID of a clustered block whose pin range is desired.
 * @return std::pair<int, int> --> (pin_low, pin_high)
 */
std::pair<int, int> get_pin_range_for_block(const ClusterBlockId blk_id);

t_block_loc get_block_loc(const ParentBlockId& block_id, bool is_flat);

int get_block_num_class(const ParentBlockId& block_id, bool is_flat);

int get_block_pin_class_num(const ParentBlockId block_id, const ParentPinId pin_id, bool is_flat);

template<typename T>
inline ClusterNetId convert_to_cluster_net_id(T id) {
    std::size_t id_num = std::size_t(id);
    ClusterNetId cluster_net_id = ClusterNetId(id_num);
    return cluster_net_id;
}

template<typename T>
inline AtomNetId convert_to_atom_net_id(T id) {
    std::size_t id_num = std::size_t(id);
    AtomNetId atom_net_id = AtomNetId(id_num);
    return atom_net_id;
}

template<typename T>
inline ClusterBlockId convert_to_cluster_block_id(T id) {
    std::size_t id_num = std::size_t(id);
    ClusterBlockId cluster_block_id = ClusterBlockId(id_num);
    return cluster_block_id;
}

template<typename T>
inline AtomBlockId convert_to_atom_block_id(T id) {
    std::size_t id_num = std::size_t(id);
    AtomBlockId atom_block_id = AtomBlockId(id_num);
    return atom_block_id;
}

template<typename T>
inline ClusterPinId convert_to_cluster_pin_id(T id) {
    std::size_t id_num = std::size_t(id);
    ClusterPinId cluster_pin_id = ClusterPinId(id_num);
    return cluster_pin_id;
}

template<typename T>
inline AtomPinId convert_to_atom_pin_id(T id) {
    std::size_t id_num = std::size_t(id);
    AtomPinId atom_pin_id = AtomPinId(id_num);
    return atom_pin_id;
}

inline ParentNetId get_cluster_net_parent_id(const AtomLookup& atom_look_up, ClusterNetId net_id, bool is_flat) {
    ParentNetId par_net_id;
    if (is_flat) {
        auto atom_net_id = atom_look_up.atom_net(net_id);
        VTR_ASSERT(atom_net_id != AtomNetId::INVALID());
        par_net_id = ParentNetId(size_t(atom_net_id));
    } else {
        par_net_id = ParentNetId(size_t(net_id));
    }
    return par_net_id;
}

//Returns a user-friendly architectural identifier for the specified RR node
std::string rr_node_arch_name(RRNodeId inode, bool is_flat);

/**************************************************************
 * Intra-Logic Block Utility Functions
 **************************************************************/

//Class for looking up pb graph pins from block pin indices
class IntraLbPbPinLookup {
  public:
    IntraLbPbPinLookup() = default;
    IntraLbPbPinLookup(const std::vector<t_logical_block_type>& block_types);
    IntraLbPbPinLookup(const IntraLbPbPinLookup& rhs);
    IntraLbPbPinLookup& operator=(IntraLbPbPinLookup rhs);
    ~IntraLbPbPinLookup();

    //Returns the pb graph pin associated with the specified type (index into block types array) and
    // pb pin index (index into block_pb().pb_route)
    const t_pb_graph_pin* pb_gpin(unsigned int itype, int ipin) const;

    friend void swap(IntraLbPbPinLookup& lhs, IntraLbPbPinLookup& rhs);

  private:
    std::vector<t_logical_block_type> block_types_;

    std::vector<t_pb_graph_pin**> intra_lb_pb_pin_lookup_;
};

//Find the atom pins (driver or sinks) connected to the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pin driving to the specified top-level CLB pin
AtomPinId find_clb_pin_driver_atom_pin(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pins driven by the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

std::tuple<ClusterNetId, int, int> find_pb_route_clb_input_net_pin(ClusterBlockId clb, int sink_pb_route_id);

//Returns the port matching name within pb_gnode
const t_port* find_pb_graph_port(const t_pb_graph_node* pb_gnode, const std::string& port_name);

//Returns the graph pin matching name at pin index
const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, const std::string& port_name, int index);

const t_pb_graph_pin* find_pb_graph_pin(const AtomNetlist& netlist, const AtomPBBimap& atom_pb_lookup, const AtomPinId pin_id);

/**
 * @brief Retrieves the atom pin associated with a specific CLB and pb_graph_pin. Warning: Not all pb_graph_pins are associated with an atom pin! Only pb_graph_pins on primatives are associated with an AtomPinId. Returns AtomPinId::INVALID() if no atom pin is found.
 */
AtomPinId find_atom_pin(ClusterBlockId blk_id, const t_pb_graph_pin* pb_gpin);

//Returns the logical block type which is most common in the device grid
t_logical_block_type_ptr find_most_common_block_type(const DeviceGrid& grid);

//Returns the physical tile type which is most common in the device grid
t_physical_tile_type_ptr find_most_common_tile_type(const DeviceGrid& grid);

//Parses a block_name.port[x:y] (e.g. LAB.data_in[3:10]) pin range specification, if no pin range is specified
//looks-up the block port and fills in the full range
InstPort parse_inst_port(const std::string& str);

//Returns the block type which is most likely the logic block
t_logical_block_type_ptr infer_logic_block_type(const DeviceGrid& grid);

bool primitive_type_feasible(AtomBlockId blk_id, const t_pb_type* cur_pb_type);
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(const t_model_ports* model_port, const int model_pin, const t_pb_graph_node* pb_graph_node);
/// @brief Gets the pb_graph_node pin at the given pin index for the given
///        pb_graph_node.
t_pb_graph_pin* get_pb_graph_node_pin_from_pb_graph_node(t_pb_graph_node* pb_graph_node, int ipin);
t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(ClusterBlockId iblock, int ipin);
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_logical_block_type_ptr type);
vtr::vector<ClusterBlockId, t_pb**> alloc_and_load_pin_id_to_pb_mapping();
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type);
void free_pin_id_to_pb_mapping(vtr::vector<ClusterBlockId, t_pb**>& pin_id_to_pb_mapping);

std::tuple<t_physical_tile_type_ptr, const t_sub_tile*, int, t_logical_block_type_ptr> get_cluster_blk_physical_spec(ClusterBlockId cluster_blk_id);

std::vector<int> get_cluster_internal_class_pairs(const AtomLookup& atom_lookup,
                                                  ClusterBlockId cluster_block_id);

std::vector<int> get_cluster_internal_pins(ClusterBlockId cluster_blk_id);

t_pin_range get_pb_pins(t_physical_tile_type_ptr physical_type,
                        const t_sub_tile* sub_tile,
                        t_logical_block_type_ptr logical_block,
                        const t_pb* pb,
                        int rel_cap);

float compute_primitive_base_cost(const t_pb_graph_node* primitive);
int num_ext_inputs_atom_block(AtomBlockId blk_id);

/**
 * @brief Parses out the pb_type_name and port_name from the direct passed in.
 * If the start_pin_index and end_pin_index is specified, parse them too. *
 * @return (start_pin_index, end_pin_index, pb_type_name, port_name)
 */
std::tuple<int, int, std::string, std::string> parse_direct_pin_name(std::string_view src_string, int line);

void free_pb_stats(t_pb* pb);
void free_pb(t_pb* pb, AtomPBBimap& atom_pb_bimap);

void print_switch_usage();
void print_usage_by_wire_length();

const t_pb* find_memory_sibling(const t_pb* pb);

int get_atom_pin_class_num(const AtomPinId atom_pin_id);

int max_pins_per_grid_tile();

void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision);
void pretty_print_float(const char* prefix, double value, int num_digits, int scientific_precision);

void print_timing_stats(const std::string& name,
                        const t_timing_analysis_profile_info& current,
                        const t_timing_analysis_profile_info& past = t_timing_analysis_profile_info());

std::vector<const t_pb_graph_node*> get_all_pb_graph_node_primitives(const t_pb_graph_node* pb_graph_node);

bool is_inter_cluster_node(const RRGraphView& rr_graph_view,
                           RRNodeId node_id);

int get_rr_node_max_ptc(const RRGraphView& rr_graph_view,
                        RRNodeId node_id,
                        bool is_flat);

RRNodeId get_pin_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                            t_physical_tile_type_ptr physical_tile,
                            const int layer,
                            const int root_i,
                            const int root_j,
                            int pin_physical_num);

/**
 * @brief Returns the RR node ID for the given atom pin ID.
 * **Warning**: This function should be called only if flat-router is enabled,
 * since, otherwise, the routing resources inside clusters are not added to the RR graph.
 * @param atom_pin_id The atom pin ID.
 */
RRNodeId get_atom_pin_rr_node_id(AtomPinId atom_pin_id);

/**
 * @brief Returns the cluster block ID and pb_graph_pin for the given RR node ID.
 * @note  Use structured bindings for clarity:
 * ```cpp
 * auto [blk_id, pb_graph_pin] = get_rr_node_cluster_blk_id_pb_graph_pin ( ... );
 * ```
 * **Warning**: This function should be called only if flat-router is enabled,
 * since, otherwise, the routing resources inside clusters are not added to the RR graph.
 * @param rr_node_id The RR node ID.
 * @return A pair containing the ClusterBlockId and the corresponding t_pb_graph_pin pointer.
 */
std::pair<ClusterBlockId, t_pb_graph_pin*> get_rr_node_cluster_blk_id_pb_graph_pin(RRNodeId rr_node_id);

/**
 * @brief Returns the atom pin ID for the given RR node ID.
 * **Warning**: This function should be called only if flat-router is enabled,
 * since, otherwise, the routing resources inside clusters are not added to the RR graph.
 * Not all RRNodes have an AtomPinId associated with them.
 * See also: find_atom_pin(ClusterBlockId blk_id, const t_pb_graph_pin* pb_gpin).
 * @param rr_node_id The RR node ID.
 */
AtomPinId get_rr_node_atom_pin_id(RRNodeId rr_node_id);

RRNodeId get_class_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                              t_physical_tile_type_ptr physical_tile,
                              const int layer,
                              const int i,
                              const int j,
                              int class_physical_num);

/// @brief Check whether the given nodes are in the same cluster
bool node_in_same_physical_tile(RRNodeId node_first, RRNodeId node_second);

/**
 * @brief Checks if a direct connection exists between two RR nodes.
 *
 * A direct connection is defined as a specific path: `SOURCE -> OPIN -> IPIN -> SINK`.
 *
 * @param src_rr_node The source RR node (must be of type `SOURCE`).
 * @param sink_rr_node The sink RR node (must be of type `SINK`).
 *
 * @return `true` if a direct connection exists between the source and sink nodes;
 *         otherwise, `false`.
 *
 * @details
 * - The function performs a depth-limited search starting from the source node,
 *   traversing through OPIN, IPIN, and finally checking if the path reaches the sink node.
 * - Ensures the specified node types are respected (e.g., source node must be of type `SOURCE`).
 */

bool directconnect_exists(RRNodeId src_rr_node, RRNodeId sink_rr_node);

std::vector<int> get_cluster_netlist_intra_tile_classes_at_loc(int layer,
                                                               int i,
                                                               int j,
                                                               t_physical_tile_type_ptr physical_type);

/**
 * @brief Returns the list of pins inside the tile located at (layer, i, j), except for the ones which are on a chain
 */
std::vector<int> get_cluster_netlist_intra_tile_pins_at_loc(const int layer,
                                                            const int i,
                                                            const int j,
                                                            const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                            const vtr::vector<ClusterBlockId, std::unordered_set<int>>& pin_chains_num,
                                                            t_physical_tile_type_ptr physical_type);

std::vector<int> get_cluster_block_pins(t_physical_tile_type_ptr physical_tile,
                                        ClusterBlockId cluster_blk_id,
                                        int abs_cap);

t_arch_switch_inf create_internal_arch_sw(float delay);

void add_pb_child_to_list(std::list<const t_pb*>& pb_list, const t_pb* parent_pb);

/**
 * @brief Apply user-defined route constraints to set the 'net_is_ignored_' and 'net_is_global_' flags.
 *
 * The 'net_is_global_' flag is used to identify global nets, which can be either clock signals or specified as global by user constraints.
 * The 'net_is_ignored_' flag ensures that the router will ignore routing for the net.
 *
 * @param route_constraints User-defined route constraints to guide the application of constraints.
 */
void apply_route_constraints(const UserRouteConstraints& constraint);

/**
 * @brief Iterate over all inter-layer switch types and return the minimum delay of it.
 * useful four router lookahead to to have some estimate of the cost of crossing a layer
 * @return
 */
float get_min_cross_layer_delay();

/**
 * @class PortPinToBlockPinConverter
 * @brief Maps the block pins indices for all block types to the corresponding port indices and port_pin indices.
 *
 * @details This is necessary since there are different netlist conventions - in the cluster level,
 * ports and port pins are used while in the post-pack level, block pins are used.
 */
class PortPinToBlockPinConverter {
  public:
    /**
     * @brief Allocates and loads blk_pin_from_port_pin_ array.
     */
    PortPinToBlockPinConverter();

    /**
     * @brief Converts port and port pin indices of a specific block type to block pin index.
     *
     * @details The reason block type is used instead of blocks is to save memory.
     *
     * @param blk_type_index The block type index.
     * @param sub_tile The subtile index within the specified block type.
     * @param port The port number whose block pin number is desired.
     * @param port_pin The port pin number in the specified port whose block pin number is desired.
     * @return int The block pin index corresponding to the given port and port pin numbers.
     */
    int get_blk_pin_from_port_pin(int blk_type_index, int sub_tile, int port, int port_pin) const;

  private:
    /**
     * @brief This array allows us to quickly find what block pin a port pin corresponds to.
     * @details A 4D array that should be indexed as following:
     * [0...device_ctx.physical_tile_types.size()-1][0..num_sub_tiles][0...num_ports-1][0...num_port_pins-1]
     */
    std::vector<std::vector<std::vector<std::vector<int>>>> blk_pin_from_port_pin_;
};
