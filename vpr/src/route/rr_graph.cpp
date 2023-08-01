#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <utility>
#include <vector>
#include "vtr_assert.h"

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_utils.h"
#include "rr_graph.h"
#include "rr_graph_area.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "rr_graph_timing_params.h"
#include "rr_graph_indexed_data.h"
#include "check_rr_graph.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "cb_metrics.h"
#include "build_switchblocks.h"
#include "rr_graph_writer.h"
#include "rr_graph_reader.h"
#include "router_lookahead_map.h"
#include "rr_graph_clock.h"
#include "edge_groups.h"
#include "rr_graph_builder.h"

#include "rr_types.h"
#include "echo_files.h"

//#define VERBOSE
//used for getting the exact count of each edge type and printing it to std out.

struct t_mux {
    int size;
    t_mux* next;
};

struct t_mux_size_distribution {
    int mux_count;
    int max_index;
    int* distr;
    t_mux_size_distribution* next;
};

struct t_clb_to_clb_directs {
    t_physical_tile_type_ptr from_clb_type;
    int from_clb_pin_start_index;
    int from_clb_pin_end_index;
    t_physical_tile_type_ptr to_clb_type;
    int to_clb_pin_start_index;
    int to_clb_pin_end_index;
    int switch_index; //The switch type used by this direct connection
};

struct t_pin_loc {
    int pin_index;
    int width_offset;
    int height_offset;
    int layer_offset;
    e_side side;
};

struct t_pin_spec {
    t_rr_type pin_type;
    int pin_ptc;
    RRNodeId pin_rr_node_id;
};

/******************* Variables local to this module. ***********************/

/********************* Subroutines local to this module. *******************/
void print_rr_graph_stats();

///@brief given a specific type, it returns layers that the type are located at
std::set<int> get_layers_of_physical_types(const t_physical_tile_type_ptr type);

///@brief given a specific pin number and type, it returns layers that the pin should be connected to
std::set<int> get_layers_pin_is_connected_to(const t_physical_tile_type_ptr type, int from_layer, int pin_index);

///@brief given a specific layer number and type, it returns layers which have same pin_index connected to the given layer
std::set<int> get_layers_connected_to_pin(const t_physical_tile_type_ptr type, int to_layer, int pin_index);

bool channel_widths_unchanged(const t_chan_width& current, const t_chan_width& proposed);

static vtr::NdMatrix<std::vector<int>, 5> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr Type,
                                                                          const std::set<int> type_layer,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const int* sets_per_seg_type);

static vtr::NdMatrix<int, 6> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const int seg_type_tracks,
                                                            const int Fc,
                                                            const t_physical_tile_type_ptr Type,
                                                            const std::set<int> type_layer,
                                                            const bool perturb_switch_pattern,
                                                            const e_directionality directionality);

static void advance_to_next_block_side(t_physical_tile_type_ptr Type, int& width_offset, int& height_offset, e_side& side);

static vtr::NdMatrix<std::vector<int>, 5> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 5> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
                                                                             const t_physical_tile_type_ptr Type,
                                                                             const std::set<int> type_layer,
                                                                             const int width,
                                                                             const int height,
                                                                             const int num_pins,
                                                                             const int max_chan_width,
                                                                             const std::vector<t_segment_inf>& seg_inf);

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
                                 const int layer,
                                 const int i,
                                 const int j,
                                 const e_side side,
                                 const t_pin_to_track_lookup& opin_to_track_map,
                                 const std::vector<vtr::Matrix<int>>& Fc_out,
                                 t_rr_edge_info_set& created_rr_edges,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const DeviceGrid& grid,
                                 const t_direct_inf* directs,
                                 const int num_directs,
                                 const t_clb_to_clb_directs* clb_to_clb_directs,
                                 const int num_seg_types);

static void build_unidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                  const RRGraphView& rr_graph,
                                  const int layer,
                                  const int i,
                                  const int j,
                                  const e_side side,
                                  const DeviceGrid& grid,
                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                  const t_chan_width& nodes_per_chan,
                                  const t_chan_details& chan_details_x,
                                  const t_chan_details& chan_details_y,
                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                  t_rr_edge_info_set& created_rr_edges,
                                  bool* Fc_clipped,
                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                  const t_direct_inf* directs,
                                  const int num_directs,
                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                  const int num_seg_types,
                                  int& edge_count);

static int get_opin_direct_connections(RRGraphBuilder& rr_graph_builder,
                                       const RRGraphView& rr_graph,
                                       int layer,
                                       int x,
                                       int y,
                                       e_side side,
                                       int opin,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create,
                                       const t_direct_inf* directs,
                                       const int num_directs,
                                       const t_clb_to_clb_directs* clb_to_clb_directs);

static std::function<void(t_chan_width*)> alloc_and_load_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                                  t_rr_graph_storage& L_rr_node,
                                                                  const RRGraphView& rr_graph,
                                                                  const int num_seg_types,
                                                                  const int num_seg_types_x,
                                                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                                                  const t_chan_details& chan_details_x,
                                                                  const t_chan_details& chan_details_y,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_x,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_y,
                                                                  const t_pin_to_track_lookup& opin_to_track_map,
                                                                  const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                                                                  t_sb_connection_map* sb_conn_map,
                                                                  const DeviceGrid& grid,
                                                                  const int Fs,
                                                                  t_sblock_pattern& sblock_pattern,
                                                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                                                  const t_chan_width& chan_width,
                                                                  const int wire_to_ipin_switch,
                                                                  const int wire_to_pin_between_dice_switch,
                                                                  const int delayless_switch,
                                                                  const enum e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const t_direct_inf* directs,
                                                                  const int num_directs,
                                                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const enum e_clock_modeling clock_modeling,
                                                                  bool is_flat);

static void alloc_and_load_intra_cluster_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const int delayless_switch,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& chain_pin_nums,
                                                  float R_minW_nmos,
                                                  float R_minW_pmos,
                                                  bool is_flat,
                                                  bool load_rr_graph);

static void set_clusters_pin_chains(const ClusteredNetlist& clb_nlist,
                                    vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                    bool is_flat);

static vtr::vector<ClusterBlockId, std::unordered_set<int>> get_pin_chains_flat(const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains);

static void add_classes_rr_graph(RRGraphBuilder& rr_graph_builder,
                                 const std::vector<int>& class_num_vec,
                                 const int layer,
                                 const int root_x,
                                 const int root_y,
                                 t_physical_tile_type_ptr physical_type);

static void add_pins_rr_graph(RRGraphBuilder& rr_graph_builder,
                              const std::vector<int>& pin_num_vec,
                              const int layer,
                              const int i,
                              const int j,
                              t_physical_tile_type_ptr physical_type);

/**
 * Add the edges between pins and their respective SINK/SRC. It is important to mention that in contrast to another similar function,
 * the delay of these edges is not necessarily zero. If the primitive block which a SINK/SRC belongs to is a combinational block, the delay of
 * the edge is equal to the pin delay. This is done in order to make the router lookahead aware of the different IPIN delays. In this way, more critical
 * nets are routed to the pins with less delay.
 * @param rr_graph_builder
 * @param arch_sw_inf_map
 * @param class_num_vec
 * @param layer
 * @param i
 * @param j
 * @param rr_edges_to_create
 * @param delayless_switch
 * @param physical_type_ptr
 */
static void connect_tile_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                          std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                          const std::vector<int>& class_num_vec,
                                          const int layer,
                                          const int i,
                                          const int j,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          const int delayless_switch,
                                          t_physical_tile_type_ptr physical_type_ptr);

static void connect_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                     const std::vector<int>& class_num_vec,
                                     const int layer,
                                     const int i,
                                     const int j,
                                     t_rr_edge_info_set& rr_edges_to_create,
                                     const int delayless_switch,
                                     t_physical_tile_type_ptr physical_type_ptr);

static void alloc_and_load_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                                         std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                         t_physical_tile_type_ptr physical_tile,
                                         int layer,
                                         int root_x,
                                         int root_y,
                                         const int delayless_switch);

static float pattern_fmod(float a, float b);
static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 6>& tracks_connected_to_pin,
                                                  const std::vector<t_pin_loc>& pin_locations,
                                                  const int x_chan_width,
                                                  const int y_chan_width,
                                                  const int Fc,
                                                  const enum e_directionality directionality);

static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 6>& tracks_connected_to_pin,
                                                    const std::vector<t_pin_loc>& pin_locations,
                                                    const int x_chan_width,
                                                    const int y_chan_width,
                                                    const int Fc,
                                                    const enum e_directionality directionality);

static std::vector<bool> alloc_and_load_perturb_opins(const t_physical_tile_type_ptr type, const vtr::Matrix<int>& Fc_out, const int max_chan_width, const std::vector<t_segment_inf>& segment_inf);

#ifdef ENABLE_CHECK_ALL_TRACKS
static void check_all_tracks_reach_pins(t_logical_block_type_ptr type,
                                        int***** tracks_connected_to_pin,
                                        int max_chan_width,
                                        int Fc,
                                        enum e_pin_type ipin_or_opin);
#endif

static std::vector<std::vector<bool>> alloc_and_load_perturb_ipins(const int L_num_types,
                                                                   const int num_seg_types,
                                                                   const int* sets_per_seg_type,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_in,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                   const enum e_directionality directionality);

/*
 * Create the connection between intra-tile pins
 */
static void add_intra_cluster_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                             t_rr_edge_info_set& rr_edges_to_create,
                                             const DeviceGrid& grid,
                                             const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& nodes_to_collapse,
                                             float R_minW_nmos,
                                             float R_minW_pmos,
                                             int& num_edges,
                                             bool is_flat,
                                             bool load_rr_graph);

static void add_intra_tile_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          t_physical_tile_type_ptr physical_tile,
                                          int layer,
                                          int i,
                                          int j);

/***
 * @brief Add the intra-cluster edges
 * @param rr_graph_builder
 * @param num_collapsed_nodes Return the number of nodes that are removed due to collapsing
 * @param cluster_blk_id Cluser block id of the cluster that its edges are being added
 * @param i
 * @param j
 * @param cap Capacity number of the location that cluster is being mapped to
 * @param R_minW_nmos
 * @param R_minW_pmos
 * @param rr_edges_to_create
 * @param nodes_to_collapse Sotre the nodes in the cluster that needs to be collapsed
 * @param grid
 * @param is_flat
 * @param load_rr_graph
 */
static void build_cluster_internal_edges(RRGraphBuilder& rr_graph_builder,
                                         int& num_collapsed_nodes,
                                         ClusterBlockId cluster_blk_id,
                                         const int layer,
                                         const int i,
                                         const int j,
                                         const int cap,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         const DeviceGrid& grid,
                                         bool is_flat,
                                         bool load_rr_graph);

/*
 * Connect the pins of the given t_pb to their drivers - It doesn't add the edges going in/out of pins on a chain
 */
static void add_pb_edges(RRGraphBuilder& rr_graph_builder,
                         t_rr_edge_info_set& rr_edges_to_create,
                         t_physical_tile_type_ptr physical_type,
                         const t_sub_tile* sub_tile,
                         t_logical_block_type_ptr logical_block,
                         const t_pb* pb,
                         const t_cluster_pin_chain& nodes_to_collapse,
                         int rel_cap,
                         int layer,
                         int i,
                         int j,
                         bool is_remapped);

/**
 * Edges going in/out of collapse nodes are not added by the normal routine. This function add those edges
 * @param rr_graph_builder
 * @param rr_edges_to_create
 * @param physical_type
 * @param logical_block
 * @param cluster_pins
 * @param nodes_to_collapse
 * @param R_minW_nmos
 * @param R_minW_pmos
 * @param layer
 * @param i
 * @param j
 * @return Number of the collapsed nodes
 */
static int add_edges_for_collapsed_nodes(RRGraphBuilder& rr_graph_builder,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         t_physical_tile_type_ptr physical_type,
                                         t_logical_block_type_ptr logical_block,
                                         const std::vector<int>& cluster_pins,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         int layer,
                                         int i,
                                         int j,
                                         bool load_rr_graph);
/**
 * @note This funtion is used to add the fan-in edges of the given chain node to the chain's sink with the modified delay
 * @param rr_graph_builder
 * @param rr_edges_to_create
 * @param num_collapsed_pins
 * @param physical_type
 * @param logical_block
 * @param nodes_to_collapse
 * @param cluster_pins
 * @param chain_pins
 * @param R_minW_nmos
 * @param R_minW_pmos
 * @param chain_idx
 * @param node_idx
 * @param sink_pin_num
 * @param layer
 * @param i
 * @param j
 */
static void add_chain_node_fan_in_edges(RRGraphBuilder& rr_graph_builder,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        int& num_collapsed_pins,
                                        t_physical_tile_type_ptr physical_type,
                                        t_logical_block_type_ptr logical_block,
                                        const t_cluster_pin_chain& nodes_to_collapse,
                                        const std::unordered_set<int>& cluster_pins,
                                        const std::unordered_set<int>& chain_pins,
                                        float R_minW_nmos,
                                        float R_minW_pmos,
                                        int chain_idx,
                                        int node_idx,
                                        int layer,
                                        int i,
                                        int j,
                                        bool load_rr_graph);

/**
 * @note  Return the minimum delay to the chain's sink since a pin outside of the chain may have connections to multiple pins inside the chain.
 * @param physical_type
 * @param logical_block
 * @param cluster_pins
 * @param chain_pins
 * @param pin_physical_num
 * @param chain_sink_pin
 * @return
 */
static float get_min_delay_to_chain(t_physical_tile_type_ptr physical_type,
                                    t_logical_block_type_ptr logical_block,
                                    const std::unordered_set<int>& cluster_pins,
                                    const std::unordered_set<int>& chain_pins,
                                    int pin_physical_num,
                                    int chain_sink_pin);

static std::unordered_set<int> get_chain_pins(std::vector<t_pin_chain_node> chain);

static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
                          const int layer,
                          const int i,
                          const int j,
                          const t_rr_type chan_type,
                          const t_track_to_pin_lookup& track_to_pin_lookup,
                          t_sb_connection_map* sb_conn_map,
                          const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                          const int cost_index_offset,
                          const t_chan_width& nodes_per_chan,
                          const DeviceGrid& grid,
                          const int tracks_per_chan,
                          t_sblock_pattern& sblock_pattern,
                          const int Fs_per_side,
                          const t_chan_details& chan_details_x,
                          const t_chan_details& chan_details_y,
                          t_rr_edge_info_set& created_rr_edges,
                          const int wire_to_ipin_switch,
                          const int wire_to_pin_between_dice_switch,
                          const enum e_directionality directionality);

void uniquify_edges(t_rr_edge_info_set& rr_edges_to_create);

void alloc_and_load_edges(RRGraphBuilder& rr_graph_builder,
                          const t_rr_edge_info_set& rr_edges_to_create);

static void alloc_and_load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                         std::vector<std::map<int, int>>& switch_fanin_remap,
                                         const std::map<int, t_arch_switch_inf> arch_sw_inf,
                                         const float R_minW_nmos,
                                         const float R_minW_pmos,
                                         const int wire_to_arch_ipin_switch,
                                         int* wire_to_rr_ipin_switch);

static void remap_rr_node_switch_indices(RRGraphBuilder& rr_graph_builder,
                                         const t_arch_switch_fanin& switch_fanin);

static void load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                               std::vector<std::map<int, int>>& switch_fanin_remap,
                               const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                               const float R_minW_nmos,
                               const float R_minW_pmos,
                               const t_arch_switch_fanin& arch_switch_fanins);

static void alloc_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                t_arch_switch_fanin& arch_switch_fanins,
                                const std::map<int, t_arch_switch_inf>& arch_sw_map);

static void rr_graph_externals(const std::vector<t_segment_inf>& segment_inf,
                               const std::vector<t_segment_inf>& segment_inf_x,
                               const std::vector<t_segment_inf>& segment_inf_y,
                               int wire_to_rr_ipin_switch,
                               enum e_base_cost_type base_cost_type);

static t_clb_to_clb_directs* alloc_and_load_clb_to_clb_directs(const t_direct_inf* directs, const int num_directs, const int delayless_switch);

static t_seg_details* alloc_and_load_global_route_seg_details(const int global_route_switch,
                                                              int* num_seg_details = nullptr);

static std::vector<vtr::Matrix<int>> alloc_and_load_actual_fc(const std::vector<t_physical_tile_type>& types,
                                                              const int max_pins,
                                                              const std::vector<t_segment_inf>& segment_inf,
                                                              const int* sets_per_seg_type,
                                                              const t_chan_width* nodes_per_chan,
                                                              const e_fc_type fc_type,
                                                              const enum e_directionality directionality,
                                                              bool* Fc_clipped,
                                                              bool is_flat);

static RRNodeId pick_best_direct_connect_target_rr_node(const RRGraphView& rr_graph,
                                                        RRNodeId from_rr,
                                                        const std::vector<RRNodeId>& candidate_rr_nodes);

/**
 *
 * @param cluster_pins
 * @param physical_type
 * @param logical_block
 * @param is_flat
 * @return A structure containing
 */
static t_cluster_pin_chain get_cluster_directly_connected_nodes(const std::vector<int>& cluster_pins,
                                                                t_physical_tile_type_ptr physical_type,
                                                                t_logical_block_type_ptr logical_block,
                                                                bool is_flat);

/**
 *
 * @param physical_type
 * @param logical_block
 * @param pins_in_cluster
 * @param pin_physical_num
 * @param is_flat
 * @return A chain of nodes starting from pin_physcical_num. All of the pins in this chain has a fan-out of 1
 */
static std::vector<int> get_directly_connected_nodes(t_physical_tile_type_ptr physical_type,
                                                     t_logical_block_type_ptr logical_block,
                                                     const std::unordered_set<int>& pins_in_cluster,
                                                     int pin_physical_num,
                                                     bool is_flat);

static bool is_node_chain_sorted(t_physical_tile_type_ptr physical_type,
                                 t_logical_block_type_ptr logical_block,
                                 const std::unordered_set<int>& pins_in_cluster,
                                 const std::vector<int>& pin_index_vec,
                                 const std::vector<std::vector<t_pin_chain_node>>& all_node_chain);

static std::vector<int> get_node_chain_sinks(const std::vector<std::vector<t_pin_chain_node>>& cluster_chains);

static std::vector<int> get_sink_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                 t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 const int pin_physical_num);

static std::vector<int> get_src_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                t_physical_tile_type_ptr physical_type,
                                                t_logical_block_type_ptr logical_block,
                                                const int pin_physical_num);

static int get_chain_idx(const std::vector<int>& pin_idx_vec, const std::vector<int>& pin_chain, int new_idx);

/**
 * If pin chain is a part of a chain already added to all_chains, add the new parts to the corresponding chain. Otherwise, add pin_chain as a new chain to all_chains.
 * @param pin_chain
 * @param chain_idx
 * @param pin_index_vec
 * @param all_chains
 * @param is_new_chain
 */
static void add_pin_chain(const std::vector<int>& pin_chain,
                          int chain_idx,
                          std::vector<int>& pin_index_vec,
                          std::vector<std::vector<t_pin_chain_node>>& all_chains,
                          bool is_new_chain);

/***
 * @brief Return a pair. The firt element indicates whether the switch is added or it was already added. The second element is the switch index.
 * @param rr_graph
 * @param arch_sw_inf
 * @param R_minW_nmos Needs to be passed to use create_rr_switch_from_arch_switch
 * @param R_minW_pmos Needs to be passed to use create_rr_switch_from_arch_switch
 * @param is_rr_sw If it is true, the function would search in the data structure that store rr switches.
 *                  Otherwise, it would search in the data structure that store switches that are not rr switches.
 * @param delay
 * @return
 */
static std::pair<bool, int> find_create_intra_cluster_sw(RRGraphBuilder& rr_graph,
                                                         std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                                         float R_minW_nmos,
                                                         float R_minW_pmos,
                                                         bool is_rr_sw,
                                                         float delay);

static float get_delay_directly_connected_pins(t_physical_tile_type_ptr physical_type,
                                               t_logical_block_type_ptr logical_block,
                                               const std::unordered_set<int>& cluster_pins,
                                               int first_pin_num,
                                               int second_pin_num);

static void process_non_config_sets();

static void build_rr_graph(const t_graph_type graph_type,
                           const std::vector<t_physical_tile_type>& types,
                           const DeviceGrid& grid,
                           t_chan_width nodes_per_chan,
                           const enum e_switch_block_type sb_type,
                           const int Fs,
                           const std::vector<t_switchblock_inf> switchblocks,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int wire_to_pin_between_dice_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const enum e_base_cost_type base_cost_type,
                           const enum e_clock_modeling clock_modeling,
                           const t_direct_inf* directs,
                           const int num_directs,
                           int* wire_to_rr_ipin_switch,
                           bool is_flat,
                           int* Warnings);

static void build_intra_cluster_rr_graph(const t_graph_type graph_type,
                                         const DeviceGrid& grid,
                                         const std::vector<t_physical_tile_type>& types,
                                         const RRGraphView& rr_graph,
                                         const int delayless_switch,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         RRGraphBuilder& rr_graph_builder,
                                         bool is_flat,
                                         bool load_rr_graph);

/******************* Subroutine definitions *******************************/

void create_rr_graph(const t_graph_type graph_type,
                     const std::vector<t_physical_tile_type>& block_types,
                     const DeviceGrid& grid,
                     const t_chan_width nodes_per_chan,
                     t_det_routing_arch* det_routing_arch,
                     const std::vector<t_segment_inf>& segment_inf,
                     const t_router_opts& router_opts,
                     const t_direct_inf* directs,
                     const int num_directs,
                     int* Warnings,
                     bool is_flat) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    bool echo_enabled = getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA);
    const char* echo_file_name = getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA);
    bool load_rr_graph = !det_routing_arch->read_rr_graph_filename.empty();
    if (load_rr_graph) {
        if (device_ctx.read_rr_graph_filename != det_routing_arch->read_rr_graph_filename) {
            free_rr_graph();

            load_rr_file(&mutable_device_ctx.rr_graph_builder,
                         &mutable_device_ctx.rr_graph,
                         device_ctx.physical_tile_types,
                         segment_inf,
                         &mutable_device_ctx.rr_indexed_data,
                         &mutable_device_ctx.rr_rc_data,
                         grid,
                         device_ctx.arch_switch_inf,
                         graph_type,
                         device_ctx.arch,
                         &mutable_device_ctx.chan_width,
                         router_opts.base_cost_type,
                         device_ctx.virtual_clock_network_root_idx,
                         &det_routing_arch->wire_to_rr_ipin_switch,
                         &det_routing_arch->wire_to_arch_ipin_switch_between_dice,
                         det_routing_arch->read_rr_graph_filename.c_str(),
                         &det_routing_arch->read_rr_graph_filename,
                         router_opts.read_rr_edge_metadata,
                         router_opts.do_check_rr_graph,
                         echo_enabled,
                         echo_file_name,
                         is_flat);
            if (router_opts.reorder_rr_graph_nodes_algorithm != DONT_REORDER) {
                mutable_device_ctx.rr_graph_builder.reorder_nodes(router_opts.reorder_rr_graph_nodes_algorithm,
                                                                  router_opts.reorder_rr_graph_nodes_threshold,
                                                                  router_opts.reorder_rr_graph_nodes_seed);
            }
        }
    } else {
        if (channel_widths_unchanged(device_ctx.chan_width, nodes_per_chan) && !device_ctx.rr_graph.empty()) {
            //No change in channel width, so skip re-building RR graph
            if (is_flat && !device_ctx.rr_graph_is_flat) {
                VTR_LOG("RR graph channel widths unchanged, intra-cluster resources should be added...\n");
            } else {
                VTR_LOG("RR graph channel widths unchanged, skipping RR graph rebuild\n");
                return;
            }
        } else {
            free_rr_graph();
            build_rr_graph(graph_type,
                           block_types,
                           grid,
                           nodes_per_chan,
                           det_routing_arch->switch_block_type,
                           det_routing_arch->Fs,
                           det_routing_arch->switchblocks,
                           segment_inf,
                           det_routing_arch->global_route_switch,
                           det_routing_arch->wire_to_arch_ipin_switch,
                           det_routing_arch->wire_to_arch_ipin_switch_between_dice,
                           det_routing_arch->delayless_switch,
                           det_routing_arch->R_minW_nmos,
                           det_routing_arch->R_minW_pmos,
                           router_opts.base_cost_type,
                           router_opts.clock_modeling,
                           directs, num_directs,
                           &det_routing_arch->wire_to_rr_ipin_switch,
                           is_flat,
                           Warnings);
        }
    }

    if (is_flat) {
        build_intra_cluster_rr_graph(graph_type,
                                     grid,
                                     block_types,
                                     device_ctx.rr_graph,
                                     det_routing_arch->delayless_switch,
                                     det_routing_arch->R_minW_nmos,
                                     det_routing_arch->R_minW_pmos,
                                     mutable_device_ctx.rr_graph_builder,
                                     is_flat,
                                     load_rr_graph);

        if (router_opts.reorder_rr_graph_nodes_algorithm != DONT_REORDER) {
            mutable_device_ctx.rr_graph_builder.reorder_nodes(router_opts.reorder_rr_graph_nodes_algorithm,
                                                              router_opts.reorder_rr_graph_nodes_threshold,
                                                              router_opts.reorder_rr_graph_nodes_seed);
        }

        mutable_device_ctx.rr_graph_is_flat = true;
    }

    process_non_config_sets();

    verify_rr_node_indices(grid,
                           device_ctx.rr_graph,
                           device_ctx.rr_indexed_data,
                           device_ctx.rr_graph.rr_nodes(),
                           is_flat);

    print_rr_graph_stats();

    // Write out rr graph file if needed - Currently, writing the flat rr-graph is not supported since loading from a flat rr-graph is not supported.
    // When this function is called in any stage other than routing, the is_flat flag passed to this function is false, regardless of the flag passed
    // through command line. So, the graph conrresponding to global resources will be created and written down to file if needed. During routing, if flat-routing
    // is enabled, intra-cluster resources will be added to the graph, but this new bigger graph will not be written down.
    if (!det_routing_arch->write_rr_graph_filename.empty() && !is_flat) {
        write_rr_graph(&mutable_device_ctx.rr_graph_builder,
                       &mutable_device_ctx.rr_graph,
                       device_ctx.physical_tile_types,
                       &mutable_device_ctx.rr_indexed_data,
                       &mutable_device_ctx.rr_rc_data,
                       grid,
                       device_ctx.arch_switch_inf,
                       device_ctx.arch,
                       &mutable_device_ctx.chan_width,
                       det_routing_arch->write_rr_graph_filename.c_str(),
                       device_ctx.virtual_clock_network_root_idx,
                       echo_enabled,
                       echo_file_name,
                       is_flat);
    }
}

static void add_intra_cluster_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                             t_rr_edge_info_set& rr_edges_to_create,
                                             const DeviceGrid& grid,
                                             const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& nodes_to_collapse,
                                             float R_minW_nmos,
                                             float R_minW_pmos,
                                             int& num_edges,
                                             bool is_flat,
                                             bool load_rr_graph) {
    VTR_ASSERT(is_flat);
    /* This function should be called if placement is done! */

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;
    int num_collapsed_nodes = 0;
    for (auto cluster_blk_id : cluster_net_list.blocks()) {
        auto block_loc = place_ctx.block_locs[cluster_blk_id].loc;
        int i = block_loc.x;
        int j = block_loc.y;
        int layer = block_loc.layer;
        int abs_cap = block_loc.sub_tile;
        build_cluster_internal_edges(rr_graph_builder,
                                     num_collapsed_nodes,
                                     cluster_blk_id,
                                     layer,
                                     i,
                                     j,
                                     abs_cap,
                                     R_minW_nmos,
                                     R_minW_pmos,
                                     rr_edges_to_create,
                                     nodes_to_collapse[cluster_blk_id],
                                     grid,
                                     is_flat,
                                     load_rr_graph);
        uniquify_edges(rr_edges_to_create);
        alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
        num_edges += rr_edges_to_create.size();
        rr_edges_to_create.clear();
    }

    VTR_LOG("Number of collapsed nodes: %d\n", num_collapsed_nodes);
}

static void add_intra_tile_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          t_physical_tile_type_ptr physical_tile,
                                          int layer,
                                          int i,
                                          int j) {
    auto pin_num_vec = get_flat_tile_pins(physical_tile);
    for (int pin_physical_num : pin_num_vec) {
        if (is_pin_on_tile(physical_tile, pin_physical_num)) {
            continue;
        }
        auto pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                 physical_tile,
                                                 layer,
                                                 i,
                                                 j,
                                                 pin_physical_num);
        VTR_ASSERT(pin_rr_node_id != RRNodeId::INVALID());
        auto logical_block = get_logical_block_from_pin_physical_num(physical_tile, pin_physical_num);
        auto driving_pins = get_physical_pin_src_pins(physical_tile, logical_block, pin_physical_num);
        for (auto driving_pin : driving_pins) {
            auto driving_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                          physical_tile,
                                                          layer,
                                                          i,
                                                          j,
                                                          driving_pin);
            VTR_ASSERT(driving_pin_node_id != RRNodeId::INVALID());

            int sw_idx = get_edge_sw_arch_idx(physical_tile,
                                              logical_block,
                                              driving_pin,
                                              pin_physical_num);

            VTR_ASSERT(sw_idx != -1);
            rr_edges_to_create.emplace_back(driving_pin_node_id, pin_rr_node_id, sw_idx, false);
        }
    }
}

void print_rr_graph_stats() {
    auto& device_ctx = g_vpr_ctx.device();

    const auto& rr_graph = device_ctx.rr_graph;

    size_t num_rr_edges = 0;
    for (auto& rr_node : rr_graph.rr_nodes()) {
        num_rr_edges += rr_graph.edges(rr_node.id()).size();
    }

    VTR_LOG("  RR Graph Nodes: %zu\n", rr_graph.num_nodes());
    VTR_LOG("  RR Graph Edges: %zu\n", num_rr_edges);
}

std::set<int> get_layers_of_physical_types(const t_physical_tile_type_ptr type) {
    const auto& device_ctx = g_vpr_ctx.device();
    std::set<int> phy_type_layers;
    for (int layer = 0; layer < device_ctx.grid.get_num_layers(); layer++) {
        if (device_ctx.grid.num_instances(type, layer) != 0) {
            phy_type_layers.insert(layer);
        }
    }
    return phy_type_layers;
}

std::set<int> get_layers_pin_is_connected_to(const t_physical_tile_type_ptr type, int from_layer, int pin_index) {
    const auto& device_ctx = g_vpr_ctx.device();
    std::set<int> layer_pin_index_is_connected_to;
    for (int layer = 0; layer < device_ctx.grid.get_num_layers(); layer++) {
        if (is_pin_conencted_to_layer(type, pin_index, from_layer, layer, device_ctx.grid.get_num_layers())) {
            layer_pin_index_is_connected_to.insert(layer);
        }
    }
    return layer_pin_index_is_connected_to;
}

std::set<int> get_layers_connected_to_pin(const t_physical_tile_type_ptr type, int to_layer, int pin_index) {
    const auto& device_ctx = g_vpr_ctx.device();
    std::set<int> layers_connected_to_pin;
    for (int layer = 0; layer < device_ctx.grid.get_num_layers(); layer++) {
        if (is_pin_conencted_to_layer(type, pin_index, layer, to_layer, device_ctx.grid.get_num_layers())) {
            layers_connected_to_pin.insert(layer);
        }
    }
    return layers_connected_to_pin;
}

bool channel_widths_unchanged(const t_chan_width& current, const t_chan_width& proposed) {
    if (current.max != proposed.max
        || current.x_max != proposed.x_max
        || current.y_max != proposed.y_max
        || current.x_min != proposed.x_min
        || current.y_min != proposed.y_min
        || current.x_list != proposed.x_list
        || current.y_list != proposed.y_list) {
        return false; //Different max/min or channel widths
    }

    return true; //Identical
}

static void build_rr_graph(const t_graph_type graph_type,
                           const std::vector<t_physical_tile_type>& types,
                           const DeviceGrid& grid,
                           t_chan_width nodes_per_chan,
                           const enum e_switch_block_type sb_type,
                           const int Fs,
                           const std::vector<t_switchblock_inf> switchblocks,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int wire_to_pin_between_dice_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const enum e_base_cost_type base_cost_type,
                           const enum e_clock_modeling clock_modeling,
                           const t_direct_inf* directs,
                           const int num_directs,
                           int* wire_to_rr_ipin_switch,
                           bool is_flat,
                           int* Warnings) {
    vtr::ScopedStartFinishTimer timer("Build routing resource graph");

    /* Reset warning flag */
    *Warnings = RR_GRAPH_NO_WARN;

    /* Decode the graph_type */
    bool is_global_graph = ((GRAPH_GLOBAL == graph_type) ? true : false);
    bool use_full_seg_groups = ((GRAPH_UNIDIR_TILEABLE == graph_type) ? true : false);
    enum e_directionality directionality = ((GRAPH_BIDIR == graph_type) ? BI_DIRECTIONAL : UNI_DIRECTIONAL);
    if (is_global_graph) {
        directionality = BI_DIRECTIONAL;
    }

    /* Global routing uses a single longwire track */

    int max_chan_width = nodes_per_chan.max = (is_global_graph ? 1 : nodes_per_chan.max);
    int max_chan_width_x = nodes_per_chan.x_max = (is_global_graph ? 1 : nodes_per_chan.x_max);
    int max_chan_width_y = nodes_per_chan.y_max = (is_global_graph ? 1 : nodes_per_chan.y_max);

    VTR_ASSERT(max_chan_width_x > 0 && max_chan_width_y > 0);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_clb_to_clb_directs* clb_to_clb_directs = nullptr;
    if (num_directs > 0) {
        clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, num_directs, delayless_switch);
    }

    /* START SEG_DETAILS */
    size_t num_segments = segment_inf.size();
    device_ctx.rr_graph_builder.reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        device_ctx.rr_graph_builder.add_rr_segment(segment_inf[iseg]);
    }

    int num_seg_details_x = 0;
    int num_seg_details_y = 0;

    t_seg_details* seg_details_x = nullptr;
    t_seg_details* seg_details_y = nullptr;

    t_unified_to_parallel_seg_index segment_index_map;
    std::vector<t_segment_inf> segment_inf_x = get_parallel_segs(segment_inf, segment_index_map, X_AXIS);
    std::vector<t_segment_inf> segment_inf_y = get_parallel_segs(segment_inf, segment_index_map, Y_AXIS);

    if (is_global_graph) {
        /* Sets up a single unit length segment type for global routing. */
        seg_details_x = alloc_and_load_global_route_seg_details(global_route_switch, &num_seg_details_x);
        seg_details_y = alloc_and_load_global_route_seg_details(global_route_switch, &num_seg_details_y);

    } else {
        /* Setup segments including distrubuting tracks and staggering.
         * If use_full_seg_groups is specified, max_chan_width may be
         * changed. Warning should be singled to caller if this happens. */

        /* Need to setup segments along x & y axis seperately, due to different 
         * max_channel_widths and segment specifications. */

        size_t max_dim = std::max(grid.width(), grid.height()) - 2; //-2 for no perim channels

        /*Get x & y segments seperately*/
        seg_details_x = alloc_and_load_seg_details(&max_chan_width_x,
                                                   max_dim, segment_inf_x,
                                                   use_full_seg_groups, directionality,
                                                   &num_seg_details_x);

        seg_details_y = alloc_and_load_seg_details(&max_chan_width_y,
                                                   max_dim, segment_inf_y,
                                                   use_full_seg_groups, directionality,
                                                   &num_seg_details_y);

        if (nodes_per_chan.x_max != max_chan_width_x || nodes_per_chan.y_max != max_chan_width_y) {
            nodes_per_chan.x_max = max_chan_width_x;
            *Warnings |= RR_GRAPH_WARN_CHAN_X_WIDTH_CHANGED;

        } else if (nodes_per_chan.y_max != max_chan_width_y) {
            nodes_per_chan.y_max = max_chan_width_y;
            *Warnings |= RR_GRAPH_WARN_CHAN_Y_WIDTH_CHANGED;
        }

        //TODO: Fix
        //if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SEG_DETAILS)) {
        //dump_seg_details(seg_details, max_chan_width,
        //getEchoFileName(E_ECHO_SEG_DETAILS));
        //}
    }

    /*map the internal segment indices of the networks*/
    if (clock_modeling == DEDICATED_NETWORK) {
        ClockRRGraphBuilder::map_relative_seg_indices(segment_index_map);
    }
    /* END SEG_DETAILS */

    /* START CHAN_DETAILS */

    t_chan_details chan_details_x;
    t_chan_details chan_details_y;

    alloc_and_load_chan_details(grid, &nodes_per_chan,
                                num_seg_details_x, num_seg_details_y,
                                seg_details_x, seg_details_y,
                                chan_details_x, chan_details_y);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CHAN_DETAILS)) {
        dump_chan_details(chan_details_x, chan_details_y, &nodes_per_chan,
                          grid, getEchoFileName(E_ECHO_CHAN_DETAILS));
    }
    /* END CHAN_DETAILS */

    /* START FC */
    /* Determine the actual value of Fc */
    std::vector<vtr::Matrix<int>> Fc_in;  /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1] */
    std::vector<vtr::Matrix<int>> Fc_out; /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1] */

    /* get maximum number of pins across all blocks */
    int max_pins = types[0].num_pins;
    for (const auto& type : types) {
        if (is_empty_type(&type)) {
            continue;
        }

        if (type.num_pins > max_pins) {
            max_pins = type.num_pins;
        }
    }

    /* get the number of 'sets' for each segment type -- unidirectial architectures have two tracks in a set, bidirectional have one */
    int total_sets = max_chan_width;
    int total_sets_x = max_chan_width_x;
    int total_sets_y = max_chan_width_y;

    if (directionality == UNI_DIRECTIONAL) {
        VTR_ASSERT(max_chan_width % 2 == 0);
        total_sets /= 2;
        total_sets_x /= 2;
        total_sets_y /= 2;
    }
    auto sets_per_seg_type_x = get_seg_track_counts(total_sets_x, segment_inf_x, use_full_seg_groups);
    auto sets_per_seg_type_y = get_seg_track_counts(total_sets_y, segment_inf_y, use_full_seg_groups);
    auto sets_per_seg_type = get_seg_track_counts(total_sets, segment_inf, use_full_seg_groups);

    auto sets_test = get_ordered_seg_track_counts(segment_inf_x, segment_inf_y, segment_inf, sets_per_seg_type_x, sets_per_seg_type_y);

    //VTR_ASSERT_MSG(sets_test==sets_per_seg_type,
    //                "Not equal combined output after combining segs " );

    if (is_global_graph) {
        //All pins can connect during global routing
        auto ones = vtr::Matrix<int>({size_t(max_pins), segment_inf.size()}, 1);
        Fc_in = std::vector<vtr::Matrix<int>>(types.size(), ones);
        Fc_out = std::vector<vtr::Matrix<int>>(types.size(), ones);
    } else {
        bool Fc_clipped = false;
        Fc_in = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type.get(), &nodes_per_chan,
                                         e_fc_type::IN, directionality, &Fc_clipped, is_flat);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }
        Fc_clipped = false;
        Fc_out = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type.get(), &nodes_per_chan,
                                          e_fc_type::OUT, directionality, &Fc_clipped, is_flat);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }

        for (const auto& type : types) {
            int i = type.index;

            /* Skip "EMPTY" */
            if (is_empty_type(&type)) {
                continue;
            }

            for (int j = 0; j < type.num_pins; ++j) {
                for (size_t k = 0; k < segment_inf.size(); k++) {
#ifdef VERBOSE
                    VTR_LOG(
                        "Fc Actual Values: type = %s, pin = %d (%s), "
                        "seg = %d (%s), Fc_out = %d, Fc_in = %d.\n",
                        type.name,
                        j,
                        block_type_pin_index_to_name(&type, j).c_str(),
                        k,
                        segment_inf[k].name.c_str(),
                        Fc_out[i][j][k],
                        Fc_in[i][j][k]);
#endif /* VERBOSE */
                    VTR_ASSERT_MSG(Fc_out[i][j][k] == 0 || Fc_in[i][j][k] == 0,
                                   "Pins must be inputs or outputs (i.e. can not have both non-zero Fc_out and Fc_in)");
                }
            }
        }
    }

    auto perturb_ipins = alloc_and_load_perturb_ipins(types.size(), segment_inf.size(),
                                                      sets_per_seg_type.get(), Fc_in, Fc_out, directionality);
    /* END FC */

    /* Alloc node lookups, count nodes, alloc rr nodes */
    int num_rr_nodes = 0;

    // Add routing resources to rr_graph lookup table
    alloc_and_load_rr_node_indices(device_ctx.rr_graph_builder,
                                   &nodes_per_chan,
                                   grid,
                                   &num_rr_nodes,
                                   chan_details_x,
                                   chan_details_y,
                                   is_flat);

    size_t expected_node_count = num_rr_nodes;
    if (clock_modeling == DEDICATED_NETWORK) {
        expected_node_count += ClockRRGraphBuilder::estimate_additional_nodes(grid);
        device_ctx.rr_graph_builder.reserve_nodes(expected_node_count);
    }
    device_ctx.rr_graph_builder.resize_nodes(num_rr_nodes);

    /* These are data structures used by the unidir opin mapping. They are used
     * to spread connections evenly for each segment type among the available
     * wire start points */
    vtr::NdMatrix<int, 3> Fc_xofs({grid.height() - 1,
                                   grid.width() - 1,
                                   segment_inf_x.size()},
                                  0); //[0..grid.height()-2][0..grid.width()-2][0..num_seg_types_x-1]
    vtr::NdMatrix<int, 3> Fc_yofs({grid.width() - 1,
                                   grid.height() - 1,
                                   segment_inf_y.size()},
                                  0); //[0..grid.width()-2][0..grid.height()-2][0..num_seg_types_y-1]

    /* START SB LOOKUP */
    /* Alloc and load the switch block lookup */
    vtr::NdMatrix<std::vector<int>, 3> switch_block_conn;
    t_sblock_pattern unidir_sb_pattern;
    t_sb_connection_map* sb_conn_map = nullptr; //for custom switch blocks

    //We are careful to use a single seed each time build_rr_graph is called
    //to initialize the random number generator (RNG) which is (potentially)
    //used when creating custom switchblocks. This ensures that build_rr_graph()
    //is deterministic -- always producing the same RR graph.
    constexpr unsigned SWITCHPOINT_RNG_SEED = 1;
    vtr::RandState switchpoint_rand_state = SWITCHPOINT_RNG_SEED;

    if (is_global_graph) {
        switch_block_conn = alloc_and_load_switch_block_conn(&nodes_per_chan, SUBSET, 3);
    } else if (BI_DIRECTIONAL == directionality) {
        if (sb_type == CUSTOM) {
            sb_conn_map = alloc_and_load_switchblock_permutations(chan_details_x, chan_details_y,
                                                                  grid,
                                                                  switchblocks, &nodes_per_chan, directionality,
                                                                  switchpoint_rand_state);
        } else {
            switch_block_conn = alloc_and_load_switch_block_conn(&nodes_per_chan, sb_type, Fs);
        }
    } else {
        VTR_ASSERT(UNI_DIRECTIONAL == directionality);

        if (sb_type == CUSTOM) {
            sb_conn_map = alloc_and_load_switchblock_permutations(chan_details_x, chan_details_y,
                                                                  grid,
                                                                  switchblocks, &nodes_per_chan, directionality,
                                                                  switchpoint_rand_state);
        } else {
            /* it looks like we get unbalanced muxing from this switch block code with Fs > 3 */
            VTR_ASSERT(Fs == 3);

            unidir_sb_pattern = alloc_sblock_pattern_lookup(grid, &nodes_per_chan);
            for (size_t i = 0; i < grid.width() - 1; i++) {
                for (size_t j = 0; j < grid.height() - 1; j++) {
                    load_sblock_pattern_lookup(i, j, grid, &nodes_per_chan,
                                               chan_details_x, chan_details_y,
                                               Fs, sb_type, unidir_sb_pattern);
                }
            }

            if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SBLOCK_PATTERN)) {
                dump_sblock_pattern(unidir_sb_pattern, max_chan_width, grid,
                                    getEchoFileName(E_ECHO_SBLOCK_PATTERN));
            }
        }
    }
    /* END SB LOOKUP */

    /* START IPIN MAP */
    /* Create ipin map lookups */

    t_pin_to_track_lookup ipin_to_track_map_x(types.size()); /* [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
    t_pin_to_track_lookup ipin_to_track_map_y(types.size()); /* [0..device_ctx.physical_tile_types.size()-1][0..max_chan_width-1][0..width][0..height][0..3] */

    t_track_to_pin_lookup track_to_pin_lookup_x(types.size());
    t_track_to_pin_lookup track_to_pin_lookup_y(types.size());

    for (unsigned int itype = 0; itype < types.size(); ++itype) {
        auto type_layer = get_layers_of_physical_types(&types[itype]);

        ipin_to_track_map_x[itype] = alloc_and_load_pin_to_track_map(RECEIVER,
                                                                     Fc_in[itype], &types[itype], type_layer,
                                                                     perturb_ipins[itype], directionality,
                                                                     segment_inf_x, sets_per_seg_type_x.get());

        ipin_to_track_map_y[itype] = alloc_and_load_pin_to_track_map(RECEIVER,
                                                                     Fc_in[itype], &types[itype], type_layer,
                                                                     perturb_ipins[itype], directionality,
                                                                     segment_inf_y, sets_per_seg_type_y.get());

        track_to_pin_lookup_x[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_x[itype], Fc_in[itype],
                                                                          &types[itype],
                                                                          type_layer, types[itype].width,
                                                                          types[itype].height,
                                                                          types[itype].num_pins,
                                                                          nodes_per_chan.x_max, segment_inf_x);

        track_to_pin_lookup_y[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_y[itype], Fc_in[itype],
                                                                          &types[itype],
                                                                          type_layer, types[itype].width,
                                                                          types[itype].height,
                                                                          types[itype].num_pins,
                                                                          nodes_per_chan.y_max, segment_inf_y);
    }

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_TRACK_TO_PIN_MAP)) {
        FILE* fp = vtr::fopen(getEchoFileName(E_ECHO_TRACK_TO_PIN_MAP), "w");
        fprintf(fp, "X MAP:\n\n");
        dump_track_to_pin_map(track_to_pin_lookup_y, types, nodes_per_chan.y_max, fp);
        fprintf(fp, "\n\nY MAP:\n\n");
        dump_track_to_pin_map(track_to_pin_lookup_x, types, nodes_per_chan.x_max, fp);
        fclose(fp);
    }
    /* END IPIN MAP */

    /* START OPIN MAP */
    /* Create opin map lookups */
    t_pin_to_track_lookup opin_to_track_map(types.size()); /* [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
    if (BI_DIRECTIONAL == directionality) {
        for (unsigned int itype = 0; itype < types.size(); ++itype) {
            auto type_layer = get_layers_of_physical_types(&types[itype]);
            auto perturb_opins = alloc_and_load_perturb_opins(&types[itype], Fc_out[itype],
                                                              max_chan_width, segment_inf);
            opin_to_track_map[itype] = alloc_and_load_pin_to_track_map(DRIVER,
                                                                       Fc_out[itype], &types[itype], type_layer, perturb_opins, directionality,
                                                                       segment_inf, sets_per_seg_type.get());
        }
    }
    /* END OPIN MAP */

    bool Fc_clipped = false;
    /* Draft the switches as internal data of RRGraph object
     * These are temporary switches copied from arch switches
     * We use them to build the edges
     * We will reset all the switches in the function
     *   alloc_and_load_rr_switch_inf()
     */
    device_ctx.rr_graph_builder.reserve_switches(device_ctx.all_sw_inf.size());
    // Create the switches
    for (const auto& sw_pair : device_ctx.all_sw_inf) {
        const auto& arch_sw = sw_pair.second;
        t_rr_switch_inf rr_switch = create_rr_switch_from_arch_switch(arch_sw,
                                                                      R_minW_nmos,
                                                                      R_minW_pmos);
        device_ctx.rr_graph_builder.add_rr_switch(rr_switch);
    }

    auto update_chan_width = alloc_and_load_rr_graph(
        device_ctx.rr_graph_builder,

        device_ctx.rr_graph_builder.rr_nodes(), device_ctx.rr_graph, segment_inf.size(),
        segment_inf_x.size(),
        segment_index_map,
        chan_details_x, chan_details_y,
        track_to_pin_lookup_x, track_to_pin_lookup_y,
        opin_to_track_map,
        switch_block_conn, sb_conn_map, grid, Fs, unidir_sb_pattern,
        Fc_out, Fc_xofs, Fc_yofs,
        nodes_per_chan,
        wire_to_arch_ipin_switch,
        wire_to_pin_between_dice_switch,
        delayless_switch,
        directionality,
        &Fc_clipped,
        directs, num_directs, clb_to_clb_directs,
        is_global_graph,
        clock_modeling,
        is_flat);

    // Verify no incremental node allocation.
    /* AA: Note that in the case of dedicated networks, we are currently underestimating the additional node count due to the clock networks. 
     * Thus, this below error is logged; it's not actually an error, the node estimation needs to get fixed for dedicated clock networks. */
    if (rr_graph.num_nodes() > expected_node_count) {
        VTR_LOG_ERROR("Expected no more than %zu nodes, have %zu nodes\n",
                      expected_node_count, rr_graph.num_nodes());
    }

    /* Update rr_nodes capacities if global routing */
    if (graph_type == GRAPH_GLOBAL) {
        // Using num_rr_nodes here over device_ctx.rr_nodes.size() because
        // clock_modeling::DEDICATED_NETWORK will append some rr nodes after
        // the regular graph.
        for (int i = 0; i < num_rr_nodes; i++) {
            if (rr_graph.node_type(RRNodeId(i)) == CHANX) {
                int ylow = rr_graph.node_ylow(RRNodeId(i));
                device_ctx.rr_graph_builder.set_node_capacity(RRNodeId(i), nodes_per_chan.x_list[ylow]);
            }
            if (rr_graph.node_type(RRNodeId(i)) == CHANY) {
                int xlow = rr_graph.node_xlow(RRNodeId(i));
                device_ctx.rr_graph_builder.set_node_capacity(RRNodeId(i), nodes_per_chan.y_list[xlow]);
            }
        }
    }

    update_chan_width(&nodes_per_chan);

    /* Allocate and load routing resource switches, which are derived from the switches from the architecture file,
     * based on their fanin in the rr graph. This routine also adjusts the rr nodes to point to these new rr switches */
    alloc_and_load_rr_switch_inf(g_vpr_ctx.mutable_device().rr_graph_builder,
                                 g_vpr_ctx.mutable_device().switch_fanin_remap,
                                 device_ctx.all_sw_inf,
                                 R_minW_nmos,
                                 R_minW_pmos,
                                 wire_to_arch_ipin_switch,
                                 wire_to_rr_ipin_switch);

    //Partition the rr graph edges for efficient access to configurable/non-configurable
    //edge subsets. Must be done after RR switches have been allocated
    device_ctx.rr_graph_builder.partition_edges();

    //Save the channel widths for the newly constructed graph
    device_ctx.chan_width = nodes_per_chan;

    rr_graph_externals(segment_inf, segment_inf_x, segment_inf_y, *wire_to_rr_ipin_switch, base_cost_type);

    check_rr_graph(device_ctx.rr_graph,
                   types,
                   device_ctx.rr_indexed_data,
                   grid,
                   device_ctx.chan_width,
                   graph_type,
                   device_ctx.virtual_clock_network_root_idx,
                   is_flat);

    /* Free all temp structs */
    delete[] seg_details_x;
    delete[] seg_details_y;

    seg_details_x = nullptr;
    seg_details_y = nullptr;
    if (!chan_details_x.empty() || !chan_details_y.empty()) {
        free_chan_details(chan_details_x, chan_details_y);
    }
    if (sb_conn_map) {
        free_switchblock_permutations(sb_conn_map);
        sb_conn_map = nullptr;
    }

    track_to_pin_lookup_x.clear();
    track_to_pin_lookup_y.clear();
    if (clb_to_clb_directs != nullptr) {
        delete[] clb_to_clb_directs;
    }

    // We are done with building the RR Graph. Thus, we can clear the storages only used
    // to build the RR Graph
    device_ctx.rr_graph_builder.clear_temp_storage();
}

static void build_intra_cluster_rr_graph(const t_graph_type graph_type,
                                         const DeviceGrid& grid,
                                         const std::vector<t_physical_tile_type>& types,
                                         const RRGraphView& rr_graph,
                                         const int delayless_switch,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         RRGraphBuilder& rr_graph_builder,
                                         bool is_flat,
                                         bool load_rr_graph) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    auto& device_ctx = g_vpr_ctx.mutable_device();

    vtr::ScopedStartFinishTimer timer("Build intra-cluster routing resource graph");

    rr_graph_builder.reset_rr_graph_flags();
    // When we are building intra-cluster resources, the edges already built are
    // already remapped.
    rr_graph_builder.init_edge_remap(true);

    vtr::vector<ClusterBlockId, t_cluster_pin_chain> pin_chains(clb_nlist.blocks().size());
    set_clusters_pin_chains(clb_nlist, pin_chains, is_flat);
    vtr::vector<ClusterBlockId, std::unordered_set<int>> cluster_flat_chain_pins = get_pin_chains_flat(pin_chains);

    int num_rr_nodes = rr_graph.num_nodes();
    alloc_and_load_intra_cluster_rr_node_indices(rr_graph_builder,
                                                 grid,
                                                 pin_chains,
                                                 cluster_flat_chain_pins,
                                                 &num_rr_nodes);
    size_t expected_node_count = num_rr_nodes;
    rr_graph_builder.resize_nodes(num_rr_nodes);

    alloc_and_load_intra_cluster_rr_graph(rr_graph_builder,
                                          grid,
                                          delayless_switch,
                                          pin_chains,
                                          cluster_flat_chain_pins,
                                          R_minW_nmos,
                                          R_minW_pmos,
                                          is_flat,
                                          load_rr_graph);

    /* AA: Note that in the case of dedicated networks, we are currently underestimating the additional node count due to the clock networks.
     * Thus this below error is logged; it's not actually an error, the node estimation needs to get fixed for dedicated clock networks. */
    if (rr_graph.num_nodes() > expected_node_count) {
        VTR_LOG_ERROR("Expected no more than %zu nodes, have %zu nodes\n",
                      expected_node_count, rr_graph.num_nodes());
    }

    if (!load_rr_graph) {
        remap_rr_node_switch_indices(rr_graph_builder,
                                     g_vpr_ctx.device().switch_fanin_remap);
    } else {
        rr_graph_builder.mark_edges_as_rr_switch_ids();
    }

    rr_graph_builder.partition_edges();

    rr_graph_builder.clear_temp_storage();

    check_rr_graph(device_ctx.rr_graph,
                   types,
                   device_ctx.rr_indexed_data,
                   grid,
                   device_ctx.chan_width,
                   graph_type,
                   device_ctx.virtual_clock_network_root_idx,
                   is_flat);
}

void build_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                         const t_det_routing_arch& det_routing_arch,
                         t_physical_tile_type_ptr physical_tile,
                         int layer,
                         int x,
                         int y,
                         const int delayless_switch) {
    std::map<int, t_arch_switch_inf> sw_map = g_vpr_ctx.device().all_sw_inf;

    int num_rr_nodes = 0;
    alloc_and_load_tile_rr_node_indices(rr_graph_builder,
                                        physical_tile,
                                        layer,
                                        x,
                                        y,
                                        &num_rr_nodes);
    rr_graph_builder.resize_nodes(num_rr_nodes);

    alloc_and_load_tile_rr_graph(rr_graph_builder,
                                 sw_map,
                                 physical_tile,
                                 layer,
                                 x,
                                 y,
                                 delayless_switch);

    std::vector<std::map<int, int>> switch_fanin_remap;
    int dummy_int;
    alloc_and_load_rr_switch_inf(rr_graph_builder,
                                 switch_fanin_remap,
                                 sw_map,
                                 det_routing_arch.R_minW_nmos,
                                 det_routing_arch.R_minW_pmos,
                                 det_routing_arch.wire_to_arch_ipin_switch,
                                 &dummy_int);
    rr_graph_builder.partition_edges();
}

/* Allocates and loads the global rr_switch_inf array based on the global
 * arch_switch_inf array and the fan-ins used by the rr nodes.
 * Also changes switch indices of rr_nodes to index into rr_switch_inf
 * instead of arch_switch_inf.
 *
 * Returns the number of rr switches created.
 * Also returns, through a pointer, the index of a representative ipin cblock switch.
 * - Currently we're not allowing a designer to specify an ipin cblock switch with
 * multiple fan-ins, so there's just one of these switches in the device_ctx.rr_switch_inf array.
 * But in the future if we allow this, we can return an index to a representative switch
 *
 * The rr_switch_inf switches are derived from the arch_switch_inf switches
 * (which were read-in from the architecture file) based on fan-in. The delays of
 * the rr switches depend on their fan-in, so we first go through the rr_nodes
 * and count how many different fan-ins exist for each arch switch.
 * Then we create these rr switches and update the switch indices
 * of rr_nodes to index into the rr_switch_inf array. */
static void alloc_and_load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                         std::vector<std::map<int, int>>& switch_fanin_remap,
                                         const std::map<int, t_arch_switch_inf> arch_sw_inf,
                                         const float R_minW_nmos,
                                         const float R_minW_pmos,
                                         const int wire_to_arch_ipin_switch,
                                         int* wire_to_rr_ipin_switch) {
    /* we will potentially be creating a couple of versions of each arch switch where
     * each version corresponds to a different fan-in. We will need to fill device_ctx.rr_switch_inf
     * with this expanded list of switches.
     *
     * To do this we will use arch_switch_fanins, which is indexed as:
     *      arch_switch_fanins[i_arch_switch][fanin] -> new_switch_id
     */
    t_arch_switch_fanin arch_switch_fanins(arch_sw_inf.size());

    /* Determine what the different fan-ins are for each arch switch, and also
     * how many entries the rr_switch_inf array should have */
    alloc_rr_switch_inf(rr_graph_builder,
                        arch_switch_fanins,
                        arch_sw_inf);

    /* create the rr switches. also keep track of, for each arch switch, what index of the rr_switch_inf
     * array each version of its fanin has been mapped to */
    load_rr_switch_inf(rr_graph_builder,
                       switch_fanin_remap,
                       arch_sw_inf,
                       R_minW_nmos,
                       R_minW_pmos,
                       arch_switch_fanins);

    /* next, walk through rr nodes again and remap their switch indices to rr_switch_inf */
    remap_rr_node_switch_indices(rr_graph_builder,
                                 arch_switch_fanins);

    /* now we need to set the wire_to_rr_ipin_switch variable which points the detailed routing architecture
     * to the representative ipin cblock switch. currently we're not allowing the specification of an ipin cblock switch
     * with multiple fan-ins, so right now there's just one. May change in the future, in which case we'd need to
     * return a representative switch */
    if (arch_switch_fanins[wire_to_arch_ipin_switch].count(UNDEFINED)) {
        /* only have one ipin cblock switch. OK. */
        (*wire_to_rr_ipin_switch) = arch_switch_fanins[wire_to_arch_ipin_switch][UNDEFINED];
    } else if (arch_switch_fanins[wire_to_arch_ipin_switch].size() != 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                        "Not currently allowing an ipin cblock switch to have multiple fan-ins");
    } else {
        //This likely indicates that no connection block has been constructed, indicating significant issues with
        //the generated RR graph.
        //
        //Instead of throwing an error we issue a warning. This means that check_rr_graph() etc. will run to give more information
        //and allow graphics to be brought up for users to debug their architectures.
        (*wire_to_rr_ipin_switch) = OPEN;
        VTR_LOG_WARN("No switch found for the ipin cblock in RR graph. Check if there is an error in arch file, or if no connection blocks are being built in RR graph\n");
    }
}

/* Allocates space for the global device_ctx.rr_switch_inf variable and returns the
 * number of rr switches that were allocated */
static void alloc_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                t_arch_switch_fanin& arch_switch_fanins,
                                const std::map<int, t_arch_switch_inf>& arch_sw_map) {
    std::vector<t_arch_switch_inf> all_sw_inf(arch_sw_map.size());
    for (const auto& map_it : arch_sw_map) {
        all_sw_inf[map_it.first] = map_it.second;
    }
    size_t num_rr_switches = rr_graph_builder.count_rr_switches(
        all_sw_inf,
        arch_switch_fanins);
    rr_graph_builder.resize_switches(num_rr_switches);
}

/* load the global device_ctx.rr_switch_inf variable. also keep track of, for each arch switch, what
 * index of the rr_switch_inf array each version of its fanin has been mapped to (through switch_fanin map) */
static void load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                               std::vector<std::map<int, int>>& switch_fanin_remap,
                               const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                               const float R_minW_nmos,
                               const float R_minW_pmos,
                               const t_arch_switch_fanin& arch_switch_fanins) {
    if (!switch_fanin_remap.empty()) {
        // at this stage, we rebuild the rr_graph (probably in binary search)
        // so old device_ctx.switch_fanin_remap is obsolete
        switch_fanin_remap.clear();
    }

    switch_fanin_remap.resize(arch_sw_inf.size());
    for (const auto& arch_sw_pair : arch_sw_inf) {
        int arch_sw_id = arch_sw_pair.first;
        std::map<int, int>::iterator it;
        for (auto fanin_rrswitch : arch_switch_fanins[arch_sw_id]) {
            /* the fanin value is in it->first, and we'll need to set what index this i_arch_switch/fanin
             * combination maps to (within rr_switch_inf) in it->second) */
            int fanin;
            int i_rr_switch;
            std::tie(fanin, i_rr_switch) = fanin_rrswitch;

            // setup device_ctx.switch_fanin_remap, for future swich usage analysis
            switch_fanin_remap[arch_sw_id][fanin] = i_rr_switch;

            load_rr_switch_from_arch_switch(rr_graph_builder,
                                            arch_sw_inf,
                                            arch_sw_id,
                                            i_rr_switch,
                                            fanin,
                                            R_minW_nmos,
                                            R_minW_pmos);
        }
    }
}
/* This function creates a routing switch for the usage of routing resource graph, based on a routing switch defined in architecture file.
 *
 * Since users can specify a routing switch whose buffer size is automatically tuned for routing architecture, the function here sets a definite buffer size, as required by placers and routers.
 */

t_rr_switch_inf create_rr_switch_from_arch_switch(const t_arch_switch_inf& arch_sw_inf,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos) {
    t_rr_switch_inf rr_switch_inf;

    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = arch_sw_inf.Tdel(0);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    rr_switch_inf.set_type(arch_sw_inf.type());
    rr_switch_inf.R = arch_sw_inf.R;
    rr_switch_inf.Cin = arch_sw_inf.Cin;
    rr_switch_inf.Cinternal = arch_sw_inf.Cinternal;
    rr_switch_inf.Cout = arch_sw_inf.Cout;
    rr_switch_inf.Tdel = rr_switch_Tdel;
    rr_switch_inf.mux_trans_size = arch_sw_inf.mux_trans_size;
    if (arch_sw_inf.buf_size_type == BufferSize::AUTO) {
        //Size based on resistance
        rr_switch_inf.buf_size = trans_per_buf(arch_sw_inf.R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(arch_sw_inf.buf_size_type == BufferSize::ABSOLUTE);
        //Use the specified size
        rr_switch_inf.buf_size = arch_sw_inf.buf_size;
    }
    rr_switch_inf.name = arch_sw_inf.name;
    rr_switch_inf.power_buffer_type = arch_sw_inf.power_buffer_type;
    rr_switch_inf.power_buffer_size = arch_sw_inf.power_buffer_size;

    rr_switch_inf.intra_tile = arch_sw_inf.intra_tile;

    return rr_switch_inf;
}
/* This function is same as create_rr_switch_from_arch_switch() in terms of functionality. It is tuned for clients functions in routing resource graph builder */
void load_rr_switch_from_arch_switch(RRGraphBuilder& rr_graph_builder,
                                     const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                     int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos) {
    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = arch_sw_inf.at(arch_switch_idx).Tdel(fanin);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].set_type(arch_sw_inf.at(arch_switch_idx).type());
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].R = arch_sw_inf.at(arch_switch_idx).R;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cin = arch_sw_inf.at(arch_switch_idx).Cin;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cinternal = arch_sw_inf.at(arch_switch_idx).Cinternal;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cout = arch_sw_inf.at(arch_switch_idx).Cout;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Tdel = rr_switch_Tdel;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].mux_trans_size = arch_sw_inf.at(arch_switch_idx).mux_trans_size;
    if (arch_sw_inf.at(arch_switch_idx).buf_size_type == BufferSize::AUTO) {
        //Size based on resistance
        rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].buf_size = trans_per_buf(arch_sw_inf.at(arch_switch_idx).R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(arch_sw_inf.at(arch_switch_idx).buf_size_type == BufferSize::ABSOLUTE);
        //Use the specified size
        rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].buf_size = arch_sw_inf.at(arch_switch_idx).buf_size;
    }
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].name = arch_sw_inf.at(arch_switch_idx).name;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_type = arch_sw_inf.at(arch_switch_idx).power_buffer_type;
    rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_size = arch_sw_inf.at(arch_switch_idx).power_buffer_size;
}

/* switch indices of each rr_node original point into the global device_ctx.arch_switch_inf array.
 * now we want to remap these indices to point into the global device_ctx.rr_switch_inf array
 * which contains switch info at different fan-in values */
static void remap_rr_node_switch_indices(RRGraphBuilder& rr_graph_builder,
                                         const t_arch_switch_fanin& switch_fanin) {
    rr_graph_builder.remap_rr_node_switch_indices(switch_fanin);
}

static void rr_graph_externals(const std::vector<t_segment_inf>& segment_inf,
                               const std::vector<t_segment_inf>& segment_inf_x,
                               const std::vector<t_segment_inf>& segment_inf_y,
                               int wire_to_rr_ipin_switch,
                               enum e_base_cost_type base_cost_type) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& grid = device_ctx.grid;
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_indexed_data = mutable_device_ctx.rr_indexed_data;
    bool echo_enabled = getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA);
    const char* echo_file_name = getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA);
    add_rr_graph_C_from_switches(rr_graph.rr_switch_inf(RRSwitchId(wire_to_rr_ipin_switch)).Cin);
    alloc_and_load_rr_indexed_data(rr_graph, grid, segment_inf, segment_inf_x,
                                   segment_inf_y, rr_indexed_data, wire_to_rr_ipin_switch, base_cost_type, echo_enabled, echo_file_name);
    //load_rr_index_segments(segment_inf.size());
}

static std::vector<std::vector<bool>> alloc_and_load_perturb_ipins(const int L_num_types,
                                                                   const int num_seg_types,
                                                                   const int* sets_per_seg_type,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_in,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                   const enum e_directionality directionality) {
    std::vector<std::vector<bool>> result(L_num_types);
    for (auto& seg_type_bools : result) {
        seg_type_bools.resize(num_seg_types, false);
    }

    /* factor to account for unidir vs bidir */
    int fac = 1;
    if (directionality == UNI_DIRECTIONAL) {
        fac = 2;
    }

    if (BI_DIRECTIONAL == directionality) {
        for (int iseg = 0; iseg < num_seg_types; ++iseg) {
            result[0][iseg] = false;

            int tracks_in_seg_type = sets_per_seg_type[iseg] * fac;

            for (int itype = 1; itype < L_num_types; ++itype) {
                result[itype][iseg] = false;

                float Fc_ratio;
                if (Fc_in[itype][0][iseg] > Fc_out[itype][0][iseg]) {
                    Fc_ratio = (float)Fc_in[itype][0][iseg] / (float)Fc_out[itype][0][iseg];
                } else {
                    Fc_ratio = (float)Fc_out[itype][0][iseg] / (float)Fc_in[itype][0][iseg];
                }

                if ((Fc_in[itype][0][iseg] <= tracks_in_seg_type - 2)
                    && (fabs(Fc_ratio - vtr::nint(Fc_ratio))
                        < (0.5 / (float)tracks_in_seg_type))) {
                    result[itype][iseg] = true;
                }
            }
        }
    } else {
        /* Unidirectional routing uses mux balancing patterns and
         * thus shouldn't need perturbation. */
        VTR_ASSERT(UNI_DIRECTIONAL == directionality);
        for (int itype = 0; itype < L_num_types; ++itype) {
            for (int iseg = 0; iseg < num_seg_types; ++iseg) {
                result[itype][iseg] = false;
            }
        }
    }
    return result;
}

static t_seg_details* alloc_and_load_global_route_seg_details(const int global_route_switch,
                                                              int* num_seg_details) {
    t_seg_details* seg_details = new t_seg_details[1];

    seg_details->index = 0;
    seg_details->abs_index = 0;
    seg_details->length = 1;
    seg_details->arch_wire_switch = global_route_switch;
    seg_details->arch_opin_switch = global_route_switch;
    seg_details->longline = false;
    seg_details->direction = Direction::BIDIR;
    seg_details->Cmetal = 0.0;
    seg_details->Rmetal = 0.0;
    seg_details->start = 1;
    seg_details->cb = std::make_unique<bool[]>(1);
    seg_details->cb[0] = true;
    seg_details->sb = std::make_unique<bool[]>(2);
    seg_details->sb[0] = true;
    seg_details->sb[1] = true;
    seg_details->group_size = 1;
    seg_details->group_start = 0;
    seg_details->seg_start = -1;
    seg_details->seg_end = -1;

    if (num_seg_details) {
        *num_seg_details = 1;
    }
    return seg_details;
}

/* Calculates the number of track connections from each block pin to each segment type */
static std::vector<vtr::Matrix<int>> alloc_and_load_actual_fc(const std::vector<t_physical_tile_type>& types,
                                                              const int max_pins,
                                                              const std::vector<t_segment_inf>& segment_inf,
                                                              const int* sets_per_seg_type,
                                                              const t_chan_width* nodes_per_chan,
                                                              const e_fc_type fc_type,
                                                              const enum e_directionality directionality,
                                                              bool* Fc_clipped,
                                                              bool is_flat) {
    //Initialize Fc of all blocks to zero
    auto zeros = vtr::Matrix<int>({size_t(max_pins), segment_inf.size()}, 0);
    std::vector<vtr::Matrix<int>> Fc(types.size(), zeros);

    *Fc_clipped = false;

    /* Unidir tracks formed in pairs, otherwise no effect. */
    int fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    VTR_ASSERT((nodes_per_chan->x_max % fac) == 0 && (nodes_per_chan->y_max % fac) == 0);

    for (const auto& type : types) { //Skip EMPTY
        int itype = type.index;

        for (const t_fc_specification& fc_spec : type.fc_specs) {
            if (fc_type != fc_spec.fc_type) continue;

            VTR_ASSERT(fc_spec.pins.size() > 0);

            int iseg = fc_spec.seg_index;

            if (fc_spec.fc_value == 0) {
                /* Special case indicating that this pin does not connect to general-purpose routing */
                for (int ipin : fc_spec.pins) {
                    Fc[itype][ipin][iseg] = 0;
                }
            } else {
                /* General case indicating that this pin connects to general-purpose routing */

                //Calculate how many connections there should be accross all the pins in this fc_spec
                int total_connections = 0;
                if (fc_spec.fc_value_type == e_fc_value_type::FRACTIONAL) {
                    float conns_per_pin = fac * sets_per_seg_type[iseg] * fc_spec.fc_value;
                    float flt_total_connections = conns_per_pin * fc_spec.pins.size();
                    total_connections = vtr::nint(flt_total_connections); //Round to integer
                } else {
                    VTR_ASSERT(fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE);

                    if (std::fmod(fc_spec.fc_value, fac) != 0.) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Absolute Fc value must be a multiple of %d (was %f) between block pin '%s' and wire segment '%s'",
                                        fac, fc_spec.fc_value,
                                        block_type_pin_index_to_name(&type, fc_spec.pins[0], is_flat).c_str(),
                                        segment_inf[iseg].name.c_str());
                    }

                    if (fc_spec.fc_value < fac) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Absolute Fc value must be at least %d (was %f) between block pin '%s' to wire segment %s",
                                        fac, fc_spec.fc_value,
                                        block_type_pin_index_to_name(&type, fc_spec.pins[0], is_flat).c_str(),
                                        segment_inf[iseg].name.c_str());
                    }

                    total_connections = vtr::nint(fc_spec.fc_value) * fc_spec.pins.size();
                }

                //Ensure that there are at least fac connections, this ensures that low Fc ports
                //targeting small sets of segs get connection(s), even if flt_total_connections < fac.
                total_connections = std::max(total_connections, fac);

                //Ensure total evenly divides fac by adding the remainder
                total_connections += (total_connections % fac);

                VTR_ASSERT(total_connections > 0);
                VTR_ASSERT(total_connections % fac == 0);

                //We walk through all the pins this fc_spec applies to, adding fac connections
                //to each pin, until we run out of connections. This should distribute the connections
                //as evenly as possible (if total_connections % pins.size() != 0, there will be
                //some inevitable imbalance).
                int connections_remaining = total_connections;
                while (connections_remaining != 0) {
                    //Add one set of connections to each pin
                    for (int ipin : fc_spec.pins) {
                        if (connections_remaining >= fac) {
                            Fc[itype][ipin][iseg] += fac;
                            connections_remaining -= fac;
                        } else {
                            VTR_ASSERT(connections_remaining == 0);
                            break;
                        }
                    }
                }

                for (int ipin : fc_spec.pins) {
                    //It is possible that we may want more connections that wires of this type exist;
                    //clip to the maximum number of wires
                    if (Fc[itype][ipin][iseg] > sets_per_seg_type[iseg] * fac) {
                        *Fc_clipped = true;
                        Fc[itype][ipin][iseg] = sets_per_seg_type[iseg] * fac;
                    }

                    VTR_ASSERT_MSG(Fc[itype][ipin][iseg] >= 0, "Calculated absolute Fc must be positive");
                    VTR_ASSERT_MSG(Fc[itype][ipin][iseg] % fac == 0, "Calculated absolute Fc must be divisible by 1 (bidir architecture) or 2 (unidir architecture)"); //Required by connection block construction code
                }
            }
        }
    }

    return Fc;
}

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */
static std::function<void(t_chan_width*)> alloc_and_load_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                                  t_rr_graph_storage& L_rr_node,
                                                                  const RRGraphView& rr_graph,
                                                                  const int num_seg_types,
                                                                  const int num_seg_types_x,
                                                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                                                  const t_chan_details& chan_details_x,
                                                                  const t_chan_details& chan_details_y,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_x,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_y,
                                                                  const t_pin_to_track_lookup& opin_to_track_map,
                                                                  const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                                                                  t_sb_connection_map* sb_conn_map,
                                                                  const DeviceGrid& grid,
                                                                  const int Fs,
                                                                  t_sblock_pattern& sblock_pattern,
                                                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                                                  const t_chan_width& chan_width,
                                                                  const int wire_to_ipin_switch,
                                                                  const int wire_to_pin_between_dice_switch,
                                                                  const int delayless_switch,
                                                                  const enum e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const t_direct_inf* directs,
                                                                  const int num_directs,
                                                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const enum e_clock_modeling clock_modeling,
                                                                  bool /*is_flat*/) {
    //We take special care when creating RR graph edges (there are typically many more
    //edges than nodes in an RR graph).
    //
    //In particular, all the following build_*() functions do not create the edges, but
    //instead record the edges they wish to create in rr_edges_to_create.
    //
    //We uniquify the edges to be created (avoiding any duplicates), and create
    //the edges in alloc_and_load_edges().
    //
    //By doing things in this manner we ensure we know exactly how many edges leave each RR
    //node, which avoids resizing the RR edge arrays (which can cause significant memory
    //fragmentation, and significantly increasing peak memory usage). This is important since
    //RR graph creation is the high-watermark of VPR's memory use.
    t_rr_edge_info_set rr_edges_to_create;

    /* If Fc gets clipped, this will be flagged to true */
    *Fc_clipped = false;

    int num_edges = 0;
    /* Connection SINKS and SOURCES to their pins - Initializing IPINs/OPINs. */
    for (int layer = 0; layer < grid.get_num_layers(); ++layer) {
        for (int i = 0; i < (int)grid.width(); ++i) {
            for (int j = 0; j < (int)grid.height(); ++j) {
                if (grid.get_width_offset({i, j, layer}) == 0 && grid.get_height_offset({i, j, layer}) == 0) {
                    t_physical_tile_type_ptr physical_tile = grid.get_physical_type({i, j, layer});
                    std::vector<int> class_num_vec;
                    std::vector<int> pin_num_vec;
                    class_num_vec = get_tile_root_classes(physical_tile);
                    pin_num_vec = get_tile_root_pins(physical_tile);
                    add_classes_rr_graph(rr_graph_builder,
                                         class_num_vec,
                                         layer,
                                         i,
                                         j,
                                         physical_tile);

                    add_pins_rr_graph(rr_graph_builder,
                                      pin_num_vec,
                                      layer,
                                      i,
                                      j,
                                      physical_tile);

                    connect_src_sink_to_pins(rr_graph_builder,
                                             class_num_vec,
                                             layer,
                                             i,
                                             j,
                                             rr_edges_to_create,
                                             delayless_switch,
                                             physical_tile);

                    //Create the actual SOURCE->OPIN, IPIN->SINK edges
                    uniquify_edges(rr_edges_to_create);
                    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();
                    rr_edges_to_create.clear();
                }
            }
        }
    }

    VTR_LOG("SOURCE->OPIN and IPIN->SINK edge count:%d\n", num_edges);
    num_edges = 0;
    /* Build opins */
    int rr_edges_before_directs = 0;
    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        for (size_t i = 0; i < grid.width(); ++i) {
            for (size_t j = 0; j < grid.height(); ++j) {
                for (e_side side : SIDES) {
                    if (BI_DIRECTIONAL == directionality) {
                        build_bidir_rr_opins(rr_graph_builder, rr_graph, layer, i, j, side,
                                             opin_to_track_map, Fc_out, rr_edges_to_create, chan_details_x,
                                             chan_details_y,
                                             grid,
                                             directs, num_directs, clb_to_clb_directs, num_seg_types);
                    } else {
                        VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                        bool clipped;
                        build_unidir_rr_opins(rr_graph_builder, rr_graph, layer, i, j, side, grid, Fc_out, chan_width,
                                              chan_details_x, chan_details_y, Fc_xofs, Fc_yofs,
                                              rr_edges_to_create, &clipped, seg_index_map,
                                              directs, num_directs, clb_to_clb_directs, num_seg_types,
                                              rr_edges_before_directs);
                        if (clipped) {
                            *Fc_clipped = true;
                        }
                    }

                    //Create the actual OPIN->CHANX/CHANY edges
                    uniquify_edges(rr_edges_to_create);
                    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();
                    rr_edges_to_create.clear();
                }
            }
        }
    }

    VTR_LOG("OPIN->CHANX/CHANY edge count before creating direct connections: %d\n", rr_edges_before_directs);
    VTR_LOG("OPIN->CHANX/CHANY edge count after creating direct connections: %d\n", num_edges);

    num_edges = 0;
    /* Build channels */
    VTR_ASSERT(Fs % 3 == 0);
    for (int layer = 0; layer < grid.get_num_layers(); ++layer) {
        auto& device_ctx = g_vpr_ctx.device();
        /* Skip the current die if architecture file specifies that it doesn't require inter-cluster programmable resource routing */
        if (!device_ctx.inter_cluster_prog_routing_resources.at(layer)) {
            continue;
        }
        for (size_t i = 0; i < grid.width() - 1; ++i) {
            for (size_t j = 0; j < grid.height() - 1; ++j) {
                if (i > 0) {
                    int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.x_list[j]);
                    build_rr_chan(rr_graph_builder, layer, i, j, CHANX, track_to_pin_lookup_x, sb_conn_map, switch_block_conn,
                                  CHANX_COST_INDEX_START,
                                  chan_width, grid, tracks_per_chan,
                                  sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                                  rr_edges_to_create,
                                  wire_to_ipin_switch,
                                  wire_to_pin_between_dice_switch,
                                  directionality);

                    //Create the actual CHAN->CHAN edges
                    uniquify_edges(rr_edges_to_create);
                    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();

                    rr_edges_to_create.clear();
                }
                if (j > 0) {
                    int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.y_list[i]);
                    build_rr_chan(rr_graph_builder, layer, i, j, CHANY, track_to_pin_lookup_y, sb_conn_map, switch_block_conn,
                                  CHANX_COST_INDEX_START + num_seg_types_x,
                                  chan_width, grid, tracks_per_chan,
                                  sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                                  rr_edges_to_create,
                                  wire_to_ipin_switch,
                                  wire_to_pin_between_dice_switch,
                                  directionality);

                    //Create the actual CHAN->CHAN edges
                    uniquify_edges(rr_edges_to_create);
                    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();

                    rr_edges_to_create.clear();
                }
            }
        }
    }
    VTR_LOG("CHAN->CHAN type edge count:%d\n", num_edges);
    num_edges = 0;
    std::function<void(t_chan_width*)> update_chan_width = [](t_chan_width*) noexcept {};
    if (clock_modeling == DEDICATED_NETWORK) {
        ClockRRGraphBuilder builder(chan_width, grid, &L_rr_node, &rr_graph_builder);
        builder.create_and_append_clock_rr_graph(num_seg_types_x, &rr_edges_to_create);
        uniquify_edges(rr_edges_to_create);
        alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
        num_edges += rr_edges_to_create.size();

        rr_edges_to_create.clear();
        update_chan_width = [builder](t_chan_width* c) {
            builder.update_chan_width(c);
        };
        VTR_LOG("\n Dedicated clock network edge count: %d \n", num_edges);
    }

    rr_graph_builder.init_fan_in();

    return update_chan_width;
}

static void set_clusters_pin_chains(const ClusteredNetlist& clb_nlist,
                                    vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                    bool is_flat) {
    VTR_ASSERT(is_flat);

    const auto& place_ctx = g_vpr_ctx.placement();

    t_physical_tile_type_ptr physical_type;
    t_logical_block_type_ptr logical_block;
    const t_sub_tile* sub_tile;
    int rel_cap;

    for (auto cluster_blk_id : clb_nlist.blocks()) {
        auto block_loc = place_ctx.block_locs[cluster_blk_id].loc;
        int abs_cap = block_loc.sub_tile;
        std::tie(physical_type, sub_tile, rel_cap, logical_block) = get_cluster_blk_physical_spec(cluster_blk_id);

        auto cluster_pins = get_cluster_block_pins(physical_type,
                                                   cluster_blk_id,
                                                   abs_cap);
        // Get the chains of nodes - Each chain would collapse into a single node
        t_cluster_pin_chain nodes_to_collapse = get_cluster_directly_connected_nodes(cluster_pins,
                                                                                     physical_type,
                                                                                     logical_block,
                                                                                     is_flat);
        pin_chains[cluster_blk_id] = std::move(nodes_to_collapse);
    }
}

static vtr::vector<ClusterBlockId, std::unordered_set<int>> get_pin_chains_flat(const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains) {
    vtr::vector<ClusterBlockId, std::unordered_set<int>> chain_pin_nums(pin_chains.size());

    for (int cluster_id_num = 0; cluster_id_num < (int)pin_chains.size(); cluster_id_num++) {
        auto cluster_id = ClusterBlockId(cluster_id_num);
        const auto& cluster_pin_chain_num = pin_chains[cluster_id].pin_chain_idx;
        chain_pin_nums[cluster_id].reserve(cluster_pin_chain_num.size());
        for (int pin_num = 0; pin_num < (int)cluster_pin_chain_num.size(); pin_num++) {
            if (cluster_pin_chain_num[pin_num] != OPEN) {
                chain_pin_nums[cluster_id].insert(pin_num);
            }
        }
    }

    return chain_pin_nums;
}
static void alloc_and_load_intra_cluster_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const int delayless_switch,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& chain_pin_nums,
                                                  float R_minW_nmos,
                                                  float R_minW_pmos,
                                                  bool is_flat,
                                                  bool load_rr_graph) {
    t_rr_edge_info_set rr_edges_to_create;
    int num_edges = 0;
    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        for (int i = 0; i < (int)grid.width(); ++i) {
            for (int j = 0; j < (int)grid.height(); ++j) {
                if (grid.get_width_offset({i, j, layer}) == 0 && grid.get_height_offset({i, j, layer}) == 0) {
                    t_physical_tile_type_ptr physical_tile = grid.get_physical_type({i, j, layer});
                    std::vector<int> class_num_vec;
                    std::vector<int> pin_num_vec;
                    class_num_vec = get_cluster_netlist_intra_tile_classes_at_loc(layer, i, j, physical_tile);
                    pin_num_vec = get_cluster_netlist_intra_tile_pins_at_loc(layer,
                                                                             i,
                                                                             j,
                                                                             pin_chains,
                                                                             chain_pin_nums,
                                                                             physical_tile);
                    add_classes_rr_graph(rr_graph_builder,
                                         class_num_vec,
                                         layer,
                                         i,
                                         j,
                                         physical_tile);

                    add_pins_rr_graph(rr_graph_builder,
                                      pin_num_vec,
                                      layer,
                                      i,
                                      j,
                                      physical_tile);

                    connect_src_sink_to_pins(rr_graph_builder,
                                             class_num_vec,
                                             layer,
                                             i,
                                             j,
                                             rr_edges_to_create,
                                             delayless_switch,
                                             physical_tile);

                    //Create the actual SOURCE->OPIN, IPIN->SINK edges
                    uniquify_edges(rr_edges_to_create);
                    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                    num_edges += rr_edges_to_create.size();
                    rr_edges_to_create.clear();
                }
            }
        }
    }

    VTR_LOG("Internal SOURCE->OPIN and IPIN->SINK edge count:%d\n", num_edges);
    num_edges = 0;
    {
        vtr::ScopedStartFinishTimer timer("Adding Internal Edges");
        // Add intra-tile edges
        add_intra_cluster_edges_rr_graph(rr_graph_builder,
                                         rr_edges_to_create,
                                         grid,
                                         pin_chains,
                                         R_minW_nmos,
                                         R_minW_pmos,
                                         num_edges,
                                         is_flat,
                                         load_rr_graph);
    }

    VTR_LOG("Internal edge count:%d\n", num_edges);

    rr_graph_builder.init_fan_in();
}

static void add_classes_rr_graph(RRGraphBuilder& rr_graph_builder,
                                 const std::vector<int>& class_num_vec,
                                 const int layer,
                                 const int root_x,
                                 const int root_y,
                                 t_physical_tile_type_ptr physical_type) {
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();

    for (auto class_num : class_num_vec) {
        auto class_type = get_class_type_from_class_physical_num(physical_type, class_num);
        RRNodeId class_inode = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type, layer, root_x, root_y, class_num);
        VTR_ASSERT(class_inode != RRNodeId::INVALID());
        int class_num_pins = get_class_num_pins_from_class_physical_num(physical_type, class_num);
        if (class_type == DRIVER) {
            rr_graph_builder.set_node_cost_index(class_inode, RRIndexedDataId(SOURCE_COST_INDEX));
            rr_graph_builder.set_node_type(class_inode, SOURCE);
        } else {
            VTR_ASSERT(class_type == RECEIVER);

            rr_graph_builder.set_node_cost_index(class_inode, RRIndexedDataId(SINK_COST_INDEX));
            rr_graph_builder.set_node_type(class_inode, SINK);
        }
        VTR_ASSERT(class_num_pins <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_capacity(class_inode, (short)class_num_pins);
        VTR_ASSERT(root_x <= std::numeric_limits<short>::max() && root_y <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_coordinates(class_inode, (short)root_x, (short)root_y, (short)(root_x + physical_type->width - 1), (short)(root_y + physical_type->height - 1));
        VTR_ASSERT(layer <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_layer(class_inode, layer);
        float R = 0.;
        float C = 0.;
        rr_graph_builder.set_node_rc_index(class_inode, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));
        rr_graph_builder.set_node_class_num(class_inode, class_num);
    }
}

static void add_pins_rr_graph(RRGraphBuilder& rr_graph_builder,
                              const std::vector<int>& pin_num_vec,
                              const int layer,
                              const int i,
                              const int j,
                              t_physical_tile_type_ptr physical_type) {
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    const auto& node_lookup = rr_graph_builder.node_lookup();
    for (auto pin_num : pin_num_vec) {
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_num);
        VTR_ASSERT(pin_type == DRIVER || pin_type == RECEIVER);
        std::vector<int> x_offset_vec;
        std::vector<int> y_offset_vec;
        std::vector<e_side> pin_sides_vec;
        std::tie(x_offset_vec, y_offset_vec, pin_sides_vec) = get_pin_coordinates(physical_type, pin_num, std::vector<e_side>(SIDES.begin(), SIDES.end()));
        VTR_ASSERT(!pin_sides_vec.empty());
        for (int pin_coord = 0; pin_coord < (int)pin_sides_vec.size(); pin_coord++) {
            int x_offset = x_offset_vec[pin_coord];
            int y_offset = y_offset_vec[pin_coord];
            e_side pin_side = pin_sides_vec[pin_coord];
            auto node_type = (pin_type == DRIVER) ? OPIN : IPIN;
            RRNodeId node_id = node_lookup.find_node(layer,
                                                     i + x_offset,
                                                     j + y_offset,
                                                     node_type,
                                                     pin_num,
                                                     pin_side);
            if (node_id != RRNodeId::INVALID()) {
                if (pin_type == RECEIVER) {
                    rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(IPIN_COST_INDEX));
                } else {
                    VTR_ASSERT(pin_type == DRIVER);
                    rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(OPIN_COST_INDEX));
                }

                rr_graph_builder.set_node_type(node_id, node_type);
                rr_graph_builder.set_node_capacity(node_id, 1);
                float R = 0.;
                float C = 0.;
                rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));
                rr_graph_builder.set_node_pin_num(node_id, pin_num);
                //Note that we store the grid tile location and side where the pin is located,
                //which greatly simplifies the drawing code
                //For those pins located on multiple sides, we save the rr node index
                //for the pin on all sides at which it exists
                //As such, multipler driver problem can be avoided.
                rr_graph_builder.set_node_coordinates(node_id,
                                                      i + x_offset,
                                                      j + y_offset,
                                                      i + x_offset,
                                                      j + y_offset);
                rr_graph_builder.set_node_layer(node_id, layer);
                rr_graph_builder.add_node_side(node_id, pin_side);
            }
        }
    }
}

static void connect_tile_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                          std::map<int, t_arch_switch_inf>& /*arch_sw_inf_map*/,
                                          const std::vector<int>& class_num_vec,
                                          const int layer,
                                          const int i,
                                          const int j,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          const int delayless_switch,
                                          t_physical_tile_type_ptr physical_type_ptr) {
    for (auto class_num : class_num_vec) {
        const auto& pin_list = get_pin_list_from_class_physical_num(physical_type_ptr, class_num);
        auto class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        RRNodeId class_rr_node_id = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, layer, i, j, class_num);
        VTR_ASSERT(class_rr_node_id != RRNodeId::INVALID());
        //bool is_primitive = is_primitive_pin(physical_type_ptr, pin_list[0]);
        //t_logical_block_type_ptr logical_block = is_primitive ? get_logical_block_from_pin_physical_num(physical_type_ptr, pin_list[0]) : nullptr;
        for (auto pin_num : pin_list) {
            RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, layer, i, j, pin_num);
            if (pin_rr_node_id == RRNodeId::INVALID()) {
                VTR_LOG_ERROR("In block (%d, %d, %d) pin num: %d doesn't exist to be connected to class %d\n",
                              layer,
                              i,
                              j,
                              pin_num,
                              class_num);
                continue;
            }
            auto pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
            /*int sw_id = -1;
             * if (is_primitive || pin_type == RECEIVER) {
             * VTR_ASSERT(logical_block != nullptr);
             * float primitive_comb_delay = get_pin_primitive_comb_delay(physical_type_ptr,
             * logical_block,
             * pin_num);
             * sw_id = find_create_intra_cluster_sw_arch_idx(arch_sw_inf_map,
             * primitive_comb_delay);
             * } else {
             * sw_id = delayless_switch;
             * }*/
            if (class_type == DRIVER) {
                VTR_ASSERT(pin_type == DRIVER);
                rr_edges_to_create.emplace_back(class_rr_node_id, pin_rr_node_id, delayless_switch, false);
            } else {
                VTR_ASSERT(class_type == RECEIVER);
                VTR_ASSERT(pin_type == RECEIVER);
                rr_edges_to_create.emplace_back(pin_rr_node_id, class_rr_node_id, delayless_switch, false);
            }
        }
    }
}

static void connect_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                     const std::vector<int>& class_num_vec,
                                     const int layer,
                                     const int i,
                                     const int j,
                                     t_rr_edge_info_set& rr_edges_to_create,
                                     const int delayless_switch,
                                     t_physical_tile_type_ptr physical_type_ptr) {
    for (auto class_num : class_num_vec) {
        const auto& pin_list = get_pin_list_from_class_physical_num(physical_type_ptr, class_num);
        auto class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        RRNodeId class_rr_node_id = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, layer, i, j, class_num);
        VTR_ASSERT(class_rr_node_id != RRNodeId::INVALID());
        for (auto pin_num : pin_list) {
            RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, layer, i, j, pin_num);
            if (pin_rr_node_id == RRNodeId::INVALID()) {
                VTR_LOG_ERROR("In block (%d, %d, %d) pin num: %d doesn't exist to be connected to class %d\n",
                              layer,
                              i,
                              j,
                              pin_num,
                              class_num);
                continue;
            }
            auto pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
            if (class_type == DRIVER) {
                VTR_ASSERT(pin_type == DRIVER);
                rr_edges_to_create.emplace_back(class_rr_node_id, pin_rr_node_id, delayless_switch, false);
            } else {
                VTR_ASSERT(class_type == RECEIVER);
                VTR_ASSERT(pin_type == RECEIVER);
                rr_edges_to_create.emplace_back(pin_rr_node_id, class_rr_node_id, delayless_switch, false);
            }
        }
    }
}

static void alloc_and_load_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                                         std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                         t_physical_tile_type_ptr physical_tile,
                                         int layer,
                                         int root_x,
                                         int root_y,
                                         const int delayless_switch) {
    t_rr_edge_info_set rr_edges_to_create;

    auto class_num_range = get_flat_tile_primitive_classes(physical_tile);
    auto pin_num_vec = get_flat_tile_pins(physical_tile);

    std::vector<int> class_num_vec(class_num_range.total_num());
    std::iota(class_num_vec.begin(), class_num_vec.end(), class_num_range.low);

    add_classes_rr_graph(rr_graph_builder,
                         class_num_vec,
                         layer,
                         root_x,
                         root_y,
                         physical_tile);

    add_pins_rr_graph(rr_graph_builder,
                      pin_num_vec,
                      layer,
                      root_x,
                      root_y,
                      physical_tile);

    connect_tile_src_sink_to_pins(rr_graph_builder,
                                  arch_sw_inf_map,
                                  class_num_vec,
                                  layer,
                                  root_x,
                                  root_y,
                                  rr_edges_to_create,
                                  delayless_switch,
                                  physical_tile);

    uniquify_edges(rr_edges_to_create);
    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
    rr_edges_to_create.clear();

    add_intra_tile_edges_rr_graph(rr_graph_builder,
                                  rr_edges_to_create,
                                  physical_tile,
                                  layer,
                                  root_x,
                                  root_y);

    uniquify_edges(rr_edges_to_create);
    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
    rr_edges_to_create.clear();

    rr_graph_builder.init_fan_in();

    rr_graph_builder.rr_nodes().shrink_to_fit();
}

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
                                 const int layer,
                                 const int i,
                                 const int j,
                                 const e_side side,
                                 const t_pin_to_track_lookup& opin_to_track_map,
                                 const std::vector<vtr::Matrix<int>>& Fc_out,
                                 t_rr_edge_info_set& rr_edges_to_create,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const DeviceGrid& grid,
                                 const t_direct_inf* directs,
                                 const int num_directs,
                                 const t_clb_to_clb_directs* clb_to_clb_directs,
                                 const int num_seg_types) {
    //Don't connect pins which are not adjacent to channels around the perimeter
    if ((i == 0 && side != RIGHT)
        || (i == int(grid.width() - 1) && side != LEFT)
        || (j == 0 && side != TOP)
        || (j == int(grid.width() - 1) && side != BOTTOM)) {
        return;
    }

    auto type = grid.get_physical_type({i, j, layer});
    int width_offset = grid.get_width_offset({i, j, layer});
    int height_offset = grid.get_height_offset({i, j, layer});

    const vtr::Matrix<int>& Fc = Fc_out[type->index];

    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        /* We only are working with opins so skip non-drivers */
        if (get_pin_type_from_pin_physical_num(type, pin_index) != DRIVER) {
            continue;
        }

        /* Can't do anything if pin isn't at this location */
        if (0 == type->pinloc[width_offset][height_offset][side][pin_index]) {
            continue;
        }

        /* get number of tracks that this pin connects to */
        int total_pin_Fc = 0;
        for (int iseg = 0; iseg < num_seg_types; iseg++) {
            total_pin_Fc += Fc[pin_index][iseg];
        }

        RRNodeId node_index = rr_graph_builder.node_lookup().find_node(layer, i, j, OPIN, pin_index, side);
        VTR_ASSERT(node_index);

        for (auto connected_layer : get_layers_pin_is_connected_to(type, layer, pin_index)) {
            if (total_pin_Fc > 0) {
                get_bidir_opin_connections(rr_graph_builder, layer, connected_layer, i, j, pin_index,
                                           node_index, rr_edges_to_create, opin_to_track_map,
                                           chan_details_x,
                                           chan_details_y);
            }
        }

        /* Add in direct connections */
        get_opin_direct_connections(rr_graph_builder, rr_graph, layer, i, j, side, pin_index,
                                    node_index, rr_edges_to_create,
                                    directs, num_directs, clb_to_clb_directs);
    }
}

void free_rr_graph() {
    /* Frees all the routing graph data structures, if they have been       *
     * allocated.  I use rr_mem_chunk_list_head as a flag to indicate       *
     * whether or not the graph has been allocated -- if it is not NULL,    *
     * a routing graph exists and can be freed.  Hence, you can call this   *
     * routine even if you're not sure of whether a rr_graph exists or not. */

    /* Before adding any more free calls here, be sure the data is NOT chunk *
     * allocated, as ALL the chunk allocated data is already free!           */
    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.read_rr_graph_filename.clear();

    device_ctx.rr_graph_builder.clear();

    device_ctx.rr_indexed_data.clear();

    device_ctx.switch_fanin_remap.clear();

    device_ctx.rr_graph_is_flat = false;

    invalidate_router_lookahead_cache();
}

static void build_cluster_internal_edges(RRGraphBuilder& rr_graph_builder,
                                         int& num_collapsed_nodes,
                                         ClusterBlockId cluster_blk_id,
                                         const int layer,
                                         const int i,
                                         const int j,
                                         const int abs_cap,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         const DeviceGrid& grid,
                                         bool is_flat,
                                         bool load_rr_graph) {
    VTR_ASSERT(is_flat);
    /* Internal edges are added from the start tile */
    int width_offset = grid.get_width_offset({i, j, layer});
    int height_offset = grid.get_height_offset({i, j, layer});
    VTR_ASSERT(width_offset == 0 && height_offset == 0);

    auto& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;

    t_physical_tile_type_ptr physical_type;
    const t_sub_tile* sub_tile;
    int rel_cap;
    t_logical_block_type_ptr logical_block;
    std::tie(physical_type, sub_tile, rel_cap, logical_block) = get_cluster_blk_physical_spec(cluster_blk_id);
    VTR_ASSERT(abs_cap < physical_type->capacity);
    VTR_ASSERT(rel_cap >= 0);

    auto cluster_pins = get_cluster_block_pins(physical_type,
                                               cluster_blk_id,
                                               abs_cap);

    const t_pb* pb = cluster_net_list.block_pb(cluster_blk_id);
    std::list<const t_pb*> pb_q;
    pb_q.push_back(pb);

    while (!pb_q.empty()) {
        pb = pb_q.front();
        pb_q.pop_front();

        add_pb_edges(rr_graph_builder,
                     rr_edges_to_create,
                     physical_type,
                     sub_tile,
                     logical_block,
                     pb,
                     nodes_to_collapse,
                     rel_cap,
                     layer,
                     i,
                     j,
                     load_rr_graph);

        add_pb_child_to_list(pb_q, pb);
    }

    // Edges going in/out of the nodes on the chain are not added by the previous funtions, they are added
    // by this function
    num_collapsed_nodes += add_edges_for_collapsed_nodes(rr_graph_builder,
                                                         rr_edges_to_create,
                                                         physical_type,
                                                         logical_block,
                                                         cluster_pins,
                                                         nodes_to_collapse,
                                                         R_minW_nmos,
                                                         R_minW_pmos,
                                                         layer,
                                                         i,
                                                         j,
                                                         load_rr_graph);
}

static void add_pb_edges(RRGraphBuilder& rr_graph_builder,
                         t_rr_edge_info_set& rr_edges_to_create,
                         t_physical_tile_type_ptr physical_type,
                         const t_sub_tile* sub_tile,
                         t_logical_block_type_ptr logical_block,
                         const t_pb* pb,
                         const t_cluster_pin_chain& nodes_to_collapse,
                         int rel_cap,
                         int layer,
                         int i,
                         int j,
                         bool is_remapped) {
    auto pin_num_range = get_pb_pins(physical_type,
                                     sub_tile,
                                     logical_block,
                                     pb,
                                     rel_cap);
    const auto& chain_sinks = nodes_to_collapse.chain_sink;
    const auto& pin_chain_idx = nodes_to_collapse.pin_chain_idx;
    for (auto pin_physical_num = pin_num_range.low; pin_physical_num <= pin_num_range.high; pin_physical_num++) {
        // The pin belongs to a chain - outgoing edges from this pin will be added later unless it is the sink of the chain
        // If a pin is on tile or it is a primitive pin, then it is not collapsed. Hence, we need to add it's connections. The only connection
        // outgoing from these pins is the connection to the chain sink which will be added later
        int chain_num = pin_chain_idx[pin_physical_num];
        bool primitive_pin = is_primitive_pin(physical_type, pin_physical_num);
        bool pin_on_tile = is_pin_on_tile(physical_type, pin_physical_num);
        if (chain_num != OPEN && chain_sinks[chain_num] != pin_physical_num && !primitive_pin && !pin_on_tile) {
            continue;
        }
        auto parent_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                     physical_type,
                                                     layer,
                                                     i,
                                                     j,
                                                     pin_physical_num);
        VTR_ASSERT(parent_pin_node_id != RRNodeId::INVALID());

        auto conn_pins_physical_num = get_physical_pin_sink_pins(physical_type,
                                                                 logical_block,
                                                                 pin_physical_num);

        for (auto conn_pin_physical_num : conn_pins_physical_num) {
            // The pin belongs to a chain - incoming edges to this pin will be added later unless it is the sink of the chain
            int conn_pin_chain_num = pin_chain_idx[conn_pin_physical_num];
            primitive_pin = is_primitive_pin(physical_type, conn_pin_physical_num);
            pin_on_tile = is_pin_on_tile(physical_type, conn_pin_physical_num);
            if (conn_pin_chain_num != OPEN && chain_sinks[conn_pin_chain_num] != conn_pin_physical_num && !primitive_pin && !pin_on_tile) {
                continue;
            }
            auto conn_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                       physical_type,
                                                       layer,
                                                       i,
                                                       j,
                                                       conn_pin_physical_num);
            // If the node_id is INVALID it means that it belongs to a pin which is not added to the RR Graph. The pin is not added
            // since it belongs to a certain mode or block which is not used in clustered netlist
            if (conn_pin_node_id == RRNodeId::INVALID()) {
                continue;
            }
            int sw_idx = get_edge_sw_arch_idx(physical_type,
                                              logical_block,
                                              pin_physical_num,
                                              conn_pin_physical_num);

            if (is_remapped) {
                bool found = false;
                float delay = g_vpr_ctx.device().all_sw_inf.at(sw_idx).Tdel();
                const auto& rr_switches = rr_graph_builder.rr_switch();
                for (int sw_id = 0; sw_id < (int)rr_switches.size(); sw_id++) {
                    const auto& rr_switch = rr_switches[RRSwitchId(sw_id)];
                    if (rr_switch.intra_tile) {
                        if (rr_switch.Tdel == delay) {
                            sw_idx = sw_id;
                            found = true;
                            break;
                        }
                    }
                }
                // If the graph is loaded from a file, we expect that all sw types are already listed there since currently, we are not doing any further
                // Optimization. If the optimization done when the rr graph file was generated is different from the current optimization, in the case that
                // these optimizations create different RR switches, this VTR ASSERT can be removed.
                VTR_ASSERT(found);
            }
            rr_edges_to_create.emplace_back(parent_pin_node_id, conn_pin_node_id, sw_idx, is_remapped);
        }
    }
}

static int add_edges_for_collapsed_nodes(RRGraphBuilder& rr_graph_builder,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         t_physical_tile_type_ptr physical_type,
                                         t_logical_block_type_ptr logical_block,
                                         const std::vector<int>& cluster_pins,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         int layer,
                                         int i,
                                         int j,
                                         bool load_rr_graph) {
    // Store the cluster pins in a set to make the search more run-time efficient
    std::unordered_set<int> cluster_pins_set(cluster_pins.begin(), cluster_pins.end());

    int num_collapsed_pins = 0;
    int num_chain = (int)nodes_to_collapse.chains.size();

    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const auto& chain = nodes_to_collapse.chains[chain_idx];
        int num_nodes = (int)chain.size();
        VTR_ASSERT(num_nodes > 1);
        std::unordered_set<int> chain_pins = get_chain_pins(nodes_to_collapse.chains[chain_idx]);
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            add_chain_node_fan_in_edges(rr_graph_builder,
                                        rr_edges_to_create,
                                        num_collapsed_pins,
                                        physical_type,
                                        logical_block,
                                        nodes_to_collapse,
                                        cluster_pins_set,
                                        chain_pins,
                                        R_minW_nmos,
                                        R_minW_pmos,
                                        chain_idx,
                                        node_idx,
                                        layer,
                                        i,
                                        j,
                                        load_rr_graph);
        }
    }
    return num_collapsed_pins;
}

static void add_chain_node_fan_in_edges(RRGraphBuilder& rr_graph_builder,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        int& num_collapsed_pins,
                                        t_physical_tile_type_ptr physical_type,
                                        t_logical_block_type_ptr logical_block,
                                        const t_cluster_pin_chain& nodes_to_collapse,
                                        const std::unordered_set<int>& cluster_pins,
                                        const std::unordered_set<int>& chain_pins,
                                        float R_minW_nmos,
                                        float R_minW_pmos,
                                        int chain_idx,
                                        int node_idx,
                                        int layer,
                                        int i,
                                        int j,
                                        bool load_rr_graph) {
    // Chain node pin physical number
    int pin_physical_num = nodes_to_collapse.chains[chain_idx][node_idx].pin_physical_num;
    const auto& pin_chain_idx = nodes_to_collapse.pin_chain_idx;
    int sink_pin_num = nodes_to_collapse.chain_sink[chain_idx];

    bool pin_on_tile = is_pin_on_tile(physical_type, pin_physical_num);
    bool primitive_pin = is_primitive_pin(physical_type, pin_physical_num);

    // The delay of the fan-in edges to the chain node is added to the delay of the chain node to the sink. Thus, the information in
    // all_sw_in needs to updated to reflect this change. In other words, if there isn't any edge with the new delay in all_sw_inf, a new member should
    // be added to all_sw_inf.
    auto& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;

    std::unordered_map<RRNodeId, float> src_node_edge_pair;

    // Get the chain's sink node rr node it.
    RRNodeId sink_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                  physical_type,
                                                  layer,
                                                  i,
                                                  j,
                                                  sink_pin_num);
    VTR_ASSERT(sink_rr_node_id != RRNodeId::INVALID());

    // None of the incoming/outgoing edges of the chain node, except for the chain sink pins, has been added in the previous functions.
    // Incoming/outgoing edges from the chain sink pins have been added in the previous functions.
    if (pin_physical_num != sink_pin_num) {
        auto pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);

        // Since the pins on the tile are connected to channels, etc. we do not collpase them into the intra-cluster nodes.
        // Since the primitve pins are connected to SINK/SRC nodes later, we do not collapse them.

        if (primitive_pin || pin_on_tile) {
            // Based on the previous checks, we put these assertions.
            VTR_ASSERT(!primitive_pin || pin_type == e_pin_type::DRIVER);
            VTR_ASSERT(!pin_on_tile || pin_type == e_pin_type::RECEIVER);
            if (pin_on_tile && is_primitive_pin(physical_type, sink_pin_num)) {
                return;
            } else if (primitive_pin && is_pin_on_tile(physical_type, sink_pin_num)) {
                return;
            }

            float chain_delay = get_delay_directly_connected_pins(physical_type,
                                                                  logical_block,
                                                                  cluster_pins,
                                                                  pin_physical_num,
                                                                  sink_pin_num);
            RRNodeId rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                     physical_type,
                                                     layer,
                                                     i,
                                                     j,
                                                     pin_physical_num);
            VTR_ASSERT(rr_node_id != RRNodeId::INVALID());

            src_node_edge_pair.insert(std::make_pair(rr_node_id, chain_delay));

        } else {
            num_collapsed_pins++;
            auto src_pins = get_src_pins_in_cluster(cluster_pins,
                                                    physical_type,
                                                    logical_block,
                                                    pin_physical_num);
            for (auto src_pin : src_pins) {
                // If the source pin is located on the current chain no edge should be added since the nodes should be collapsed.
                if (pin_chain_idx[src_pin] != OPEN) {
                    if ((pin_chain_idx[src_pin] == chain_idx)) {
                        continue;
                    } else {
                        // If it is located on other chain, src_pin should be the sink of that chain, otherwise the chain is not formed correctly.
                        VTR_ASSERT(src_pin == nodes_to_collapse.chain_sink[pin_chain_idx[src_pin]]);
                    }
                }
                float delay = get_min_delay_to_chain(physical_type,
                                                     logical_block,
                                                     cluster_pins,
                                                     chain_pins,
                                                     src_pin,
                                                     sink_pin_num);
                RRNodeId rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                         physical_type,
                                                         layer,
                                                         i,
                                                         j,
                                                         src_pin);
                VTR_ASSERT(rr_node_id != RRNodeId::INVALID());

                src_node_edge_pair.insert(std::make_pair(rr_node_id, delay));
            }
        }

        for (auto src_pair : src_node_edge_pair) {
            float delay = src_pair.second;
            bool is_rr_sw_id = load_rr_graph;
            bool is_new_sw;
            int sw_id;
            std::tie(is_new_sw, sw_id) = find_create_intra_cluster_sw(rr_graph_builder,
                                                                      all_sw_inf,
                                                                      R_minW_nmos,
                                                                      R_minW_pmos,
                                                                      is_rr_sw_id,
                                                                      delay);

            if (!is_rr_sw_id && is_new_sw) {
                // Currently we assume that if rr graph is read from a file, we shouldn't get into this block
                VTR_ASSERT(!load_rr_graph);
                // The internal edges are added after switch_fanin_remap is initialized; thus, if a new arch_sw is added,
                // switch _fanin_remap should be updated.
                t_rr_switch_inf rr_sw_inf = create_rr_switch_from_arch_switch(create_internal_arch_sw(delay),
                                                                              R_minW_nmos,
                                                                              R_minW_pmos);
                auto rr_sw_id = rr_graph_builder.add_rr_switch(rr_sw_inf);
                // If rr graph is loaded from a file, switch_fanin_remap is going to be empty
                if (!load_rr_graph) {
                    auto& switch_fanin_remap = g_vpr_ctx.mutable_device().switch_fanin_remap;
                    switch_fanin_remap.push_back({{UNDEFINED, size_t(rr_sw_id)}});
                }
            }

            rr_edges_to_create.emplace_back(src_pair.first, sink_rr_node_id, sw_id, is_rr_sw_id);
        }
    }
}

static float get_min_delay_to_chain(t_physical_tile_type_ptr physical_type,
                                    t_logical_block_type_ptr logical_block,
                                    const std::unordered_set<int>& cluster_pins,
                                    const std::unordered_set<int>& chain_pins,
                                    int pin_physical_num,
                                    int chain_sink_pin) {
    VTR_ASSERT(std::find(chain_pins.begin(), chain_pins.end(), pin_physical_num) == chain_pins.end());
    float min_delay = std::numeric_limits<float>::max();
    auto sink_pins = get_sink_pins_in_cluster(cluster_pins,
                                              physical_type,
                                              logical_block,
                                              pin_physical_num);
    bool sink_pin_found = false;
    for (auto sink_pin : sink_pins) {
        // If the sink is not on the chain, then we do not need to consider it.
        if (std::find(chain_pins.begin(), chain_pins.end(), sink_pin) == chain_pins.end()) {
            continue;
        }
        sink_pin_found = true;
        // Delay to the sink is equal to the delay to chain + chain's delay
        float delay = get_delay_directly_connected_pins(physical_type, logical_block, cluster_pins, sink_pin, chain_sink_pin) + get_edge_delay(physical_type, logical_block, pin_physical_num, sink_pin);
        if (delay < min_delay) {
            min_delay = delay;
        }
    }

    // We assume that the given pin has a sink in the chain.
    VTR_ASSERT(sink_pin_found);
    return min_delay;
}

static std::unordered_set<int> get_chain_pins(std::vector<t_pin_chain_node> chain) {
    std::unordered_set<int> chain_pins;
    for (auto node : chain) {
        chain_pins.insert(node.pin_physical_num);
    }
    return chain_pins;
}

/* Allocates/loads edges for nodes belonging to specified channel segment and initializes
 * node properties such as cost, occupancy and capacity */
static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
                          const int layer,
                          const int x_coord,
                          const int y_coord,
                          const t_rr_type chan_type,
                          const t_track_to_pin_lookup& track_to_pin_lookup,
                          t_sb_connection_map* sb_conn_map,
                          const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                          const int cost_index_offset,
                          const t_chan_width& nodes_per_chan,
                          const DeviceGrid& grid,
                          const int tracks_per_chan,
                          t_sblock_pattern& sblock_pattern,
                          const int Fs_per_side,
                          const t_chan_details& chan_details_x,
                          const t_chan_details& chan_details_y,
                          t_rr_edge_info_set& rr_edges_to_create,
                          const int wire_to_ipin_switch,
                          const int wire_to_pin_between_dice_switch,
                          const enum e_directionality directionality) {
    /* this function builds both x and y-directed channel segments, so set up our
     * coordinates based on channel type */

    auto& device_ctx = g_vpr_ctx.device();
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();

    //Initally assumes CHANX
    int seg_coord = x_coord;                           //The absolute coordinate of this segment within the channel
    int chan_coord = y_coord;                          //The absolute coordinate of this channel within the device
    int seg_dimension = device_ctx.grid.width() - 2;   //-2 for no perim channels
    int chan_dimension = device_ctx.grid.height() - 2; //-2 for no perim channels
    const t_chan_details& from_chan_details = (chan_type == CHANX) ? chan_details_x : chan_details_y;
    const t_chan_details& opposite_chan_details = (chan_type == CHANX) ? chan_details_y : chan_details_x;
    t_rr_type opposite_chan_type = CHANY;
    if (chan_type == CHANY) {
        //Swap values since CHANX was assumed above
        std::swap(seg_coord, chan_coord);
        std::swap(seg_dimension, chan_dimension);
        opposite_chan_type = CHANX;
    }

    const t_chan_seg_details* seg_details = from_chan_details[x_coord][y_coord].data();

    /* figure out if we're generating switch block edges based on a custom switch block
     * description */
    bool custom_switch_block = false;
    if (sb_conn_map != nullptr) {
        VTR_ASSERT(sblock_pattern.empty() && switch_block_conn.empty());
        custom_switch_block = true;
    }

    /* Loads up all the routing resource nodes in the current channel segment */
    for (int track = 0; track < tracks_per_chan; ++track) {
        if (seg_details[track].length() == 0)
            continue;

        //Start and end coordinates of this segment along the length of the channel
        //Note that these values are in the VPR coordinate system (and do not consider
        //wire directionality), so start correspond to left/bottom and end corresponds to right/top
        int start = get_seg_start(seg_details, track, chan_coord, seg_coord);
        int end = get_seg_end(seg_details, track, start, chan_coord, seg_dimension);

        if (seg_coord > start)
            continue; /* Only process segments which start at this location */
        VTR_ASSERT(seg_coord == start);

        const t_chan_seg_details* from_seg_details = nullptr;
        if (chan_type == CHANY) {
            from_seg_details = chan_details_y[x_coord][start].data();
        } else {
            from_seg_details = chan_details_x[start][y_coord].data();
        }

        RRNodeId node = rr_graph_builder.node_lookup().find_node(layer, x_coord, y_coord, chan_type, track);

        if (!node) {
            continue;
        }

        /* Add the edges from this track to all it's connected pins into the list */
        get_track_to_pins(rr_graph_builder, layer, start, chan_coord, track, tracks_per_chan, node, rr_edges_to_create,
                          track_to_pin_lookup, seg_details, chan_type, seg_dimension,
                          wire_to_ipin_switch, wire_to_pin_between_dice_switch, directionality);

        /* get edges going from the current track into channel segments which are perpendicular to it */
        if (chan_coord > 0) {
            const t_chan_seg_details* to_seg_details;
            int max_opposite_chan_width;
            if (chan_type == CHANX) {
                to_seg_details = chan_details_y[start][y_coord].data();
                max_opposite_chan_width = nodes_per_chan.y_max;
            } else {
                VTR_ASSERT(chan_type == CHANY);
                to_seg_details = chan_details_x[x_coord][start].data();
                max_opposite_chan_width = nodes_per_chan.x_max;
            }
            if (to_seg_details->length() > 0) {
                get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, chan_coord,
                                    opposite_chan_type, seg_dimension, max_opposite_chan_width, grid,
                                    Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                    from_seg_details, to_seg_details, opposite_chan_details,
                                    directionality,
                                    switch_block_conn, sb_conn_map);
            }
        }
        if (chan_coord < chan_dimension) {
            const t_chan_seg_details* to_seg_details;
            int max_opposite_chan_width = 0;
            if (chan_type == CHANX) {
                to_seg_details = chan_details_y[start][y_coord + 1].data();
                max_opposite_chan_width = nodes_per_chan.y_max;
            } else {
                VTR_ASSERT(chan_type == CHANY);
                to_seg_details = chan_details_x[x_coord + 1][start].data();
                max_opposite_chan_width = nodes_per_chan.x_max;
            }
            if (to_seg_details->length() > 0) {
                get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, chan_coord + 1,
                                    opposite_chan_type, seg_dimension, max_opposite_chan_width, grid,
                                    Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                    from_seg_details, to_seg_details, opposite_chan_details,
                                    directionality, switch_block_conn, sb_conn_map);
            }
        }

        /* walk over the switch blocks along the source track and implement edges from this track to other tracks
         * in the same channel (i.e. straight-through connections) */
        for (int target_seg = start - 1; target_seg <= end + 1; target_seg++) {
            if (target_seg != start - 1 && target_seg != end + 1) {
                /* skip straight-through connections from midpoint if non-custom switch block.
                 * currently non-custom switch blocks don't properly describe connections from the mid-point of a wire segment
                 * to other segments in the same channel (i.e. straight-through connections) */
                if (!custom_switch_block) {
                    continue;
                }
            }
            if (target_seg > 0 && target_seg < seg_dimension + 1) {
                const t_chan_seg_details* to_seg_details;
                /* AA: Same channel width for straight through connections assuming uniform width distributions along the axis*/
                int max_chan_width = 0;
                if (chan_type == CHANX) {
                    to_seg_details = chan_details_x[target_seg][y_coord].data();
                    max_chan_width = nodes_per_chan.x_max;
                } else {
                    VTR_ASSERT(chan_type == CHANY);
                    to_seg_details = chan_details_y[x_coord][target_seg].data();
                    max_chan_width = nodes_per_chan.y_max;
                }
                if (to_seg_details->length() > 0) {
                    get_track_to_tracks(rr_graph_builder, layer, chan_coord, start, track, chan_type, target_seg,
                                        chan_type, seg_dimension, max_chan_width, grid,
                                        Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                        from_seg_details, to_seg_details, from_chan_details,
                                        directionality,
                                        switch_block_conn, sb_conn_map);
                }
            }
        }

        /* Edge arrays have now been built up.  Do everything else.  */
        /* AA: The cost_index should be w.r.t the index of the segment to its **parallel** 
         * segment_inf vector. Note that when building channels, we use the indices
         * w.r.t segment_inf_x and segment_inf_y as computed earlier in 
         * build_rr_graph so it's fine to use .index() for to get the correct index.    
         */
        rr_graph_builder.set_node_cost_index(node, RRIndexedDataId(cost_index_offset + seg_details[track].index()));
        rr_graph_builder.set_node_capacity(node, 1); /* GLOBAL routing handled elsewhere */

        if (chan_type == CHANX) {
            rr_graph_builder.set_node_coordinates(node, start, y_coord, end, y_coord);
        } else {
            VTR_ASSERT(chan_type == CHANY);
            rr_graph_builder.set_node_coordinates(node, x_coord, start, x_coord, end);
        }

        rr_graph_builder.set_node_layer(node, layer);

        int length = end - start + 1;
        float R = length * seg_details[track].Rmetal();
        float C = length * seg_details[track].Cmetal();
        rr_graph_builder.set_node_rc_index(node, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));

        rr_graph_builder.set_node_type(node, chan_type);
        rr_graph_builder.set_node_track_num(node, track);
        rr_graph_builder.set_node_direction(node, seg_details[track].direction());
    }
}

void uniquify_edges(t_rr_edge_info_set& rr_edges_to_create) {
    std::sort(rr_edges_to_create.begin(), rr_edges_to_create.end());
    rr_edges_to_create.erase(std::unique(rr_edges_to_create.begin(), rr_edges_to_create.end()), rr_edges_to_create.end());
}

void alloc_and_load_edges(RRGraphBuilder& rr_graph_builder, const t_rr_edge_info_set& rr_edges_to_create) {
    rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
}

/* allocate pin to track map for each segment type individually and then combine into a single
 * vector */
static vtr::NdMatrix<std::vector<int>, 5> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr Type,
                                                                          const std::set<int> type_layer,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const int* sets_per_seg_type) {
    /* allocate 'result' matrix and initialize entries to OPEN. also allocate and intialize matrix which will be
     * used to index into the correct entries when loading up 'result' */
    auto& grid = g_vpr_ctx.device().grid;
    auto result = vtr::NdMatrix<std::vector<int>, 5>({
        size_t(Type->num_pins),        //[0..num_pins-1]
        size_t(Type->width),           //[0..width-1]
        size_t(Type->height),          //[0..height-1]
        size_t(grid.get_num_layers()), //[0..layer-1]
        4,                             //[0..sides-1]
    });

    /* multiplier for unidirectional vs bidirectional architectures */
    int fac = 1;
    if (directionality == UNI_DIRECTIONAL) {
        fac = 2;
    }

    /* load the pin to track matrix by looking at each segment type in turn */
    int num_parallel_seg_types = seg_inf.size();
    int seg_type_start_track = 0;
    for (int iseg = 0; iseg < num_parallel_seg_types; iseg++) {
        int num_seg_type_tracks = fac * sets_per_seg_type[iseg];

        /* determine the maximum Fc to this segment type across all pins */
        int max_Fc = 0;
        for (int pin_index = 0; pin_index < Type->num_pins; ++pin_index) {
            int pin_class = Type->pin_class[pin_index];
            if (Fc[pin_index][seg_inf[iseg].seg_index] > max_Fc && Type->class_inf[pin_class].type == pin_type) {
                max_Fc = Fc[pin_index][seg_inf[iseg].seg_index];
            }
        }

        /* get pin connections to tracks of the current segment type */
        auto pin_to_seg_type_map = alloc_and_load_pin_to_seg_type(pin_type, num_seg_type_tracks, max_Fc, Type, type_layer, perturb_switch_pattern[seg_inf[iseg].seg_index], directionality);

        /* connections in pin_to_seg_type_map are within that seg type -- i.e. in the [0,num_seg_type_tracks-1] range.
         * now load up 'result' array with these connections, but offset them so they are relative to the channel
         * as a whole */
        for (auto type_layer_index : type_layer) {
            for (int ipin = 0; ipin < Type->num_pins; ipin++) {
                for (int iwidth = 0; iwidth < Type->width; iwidth++) {
                    for (int iheight = 0; iheight < Type->height; iheight++) {
                        for (int iside = 0; iside < 4; iside++) {
                            for (int iconn = 0; iconn < max_Fc; iconn++) {
                                for (auto connected_layer : get_layers_pin_is_connected_to(Type, type_layer_index, ipin)) {
                                    int relative_track_ind = pin_to_seg_type_map[ipin][iwidth][iheight][connected_layer][iside][iconn];
                                    if (relative_track_ind != OPEN) {
                                        VTR_ASSERT(relative_track_ind <= num_seg_type_tracks);
                                        int absolute_track_ind = relative_track_ind + seg_type_start_track;

                                        VTR_ASSERT(absolute_track_ind >= 0);
                                        result[ipin][iwidth][iheight][connected_layer][iside].push_back(
                                            absolute_track_ind);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /* next seg type will start at this track index */
        seg_type_start_track += num_seg_type_tracks;
    }

    return result;
}

static vtr::NdMatrix<int, 6> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const int num_seg_type_tracks,
                                                            const int Fc,
                                                            const t_physical_tile_type_ptr Type,
                                                            const std::set<int> type_layer,
                                                            const bool perturb_switch_pattern,
                                                            const e_directionality directionality) {
    /* Note: currently a single value of Fc is used across each pin. In the future
     * the looping below will have to be modified if we want to account for pin-based
     * Fc values */

    /* NB:  This wastes some space.  Could set tracks_..._pin[ipin][ioff][iside] =
     * NULL if there is no pin on that side, or that pin is of the wrong type.
     * Probably not enough memory to worry about, esp. as it's temporary.
     * If pin ipin on side iside does not exist or is of the wrong type,
     * tracks_connected_to_pin[ipin][iside][0] = OPEN.                               */

    /* AA: I think we're currently not doing anything with the ability to specify different Fc values
     * for each segment. Cause we're passing in max_Fc into this function by finding the max_Fc for that
     * segment type by going through Fc[ipin][iseg] (this is a matrix for each type btw, not the overall
     * Fc[itype][ipin][iseg]  matrix). Probably should update this to allow finer control over cb along with
     * allowing different specification of the segment parallel_axis.
     */

    auto& grid = g_vpr_ctx.device().grid;

    if (Type->num_pins < 1) {
        return vtr::NdMatrix<int, 6>();
    }

    auto tracks_connected_to_pin = vtr::NdMatrix<int, 6>({
                                                             size_t(Type->num_pins),        //[0..num_pins-1]
                                                             size_t(Type->width),           //[0..width-1]
                                                             size_t(Type->height),          //[0..height-1]
                                                             size_t(grid.get_num_layers()), //[0..layer-1]
                                                             NUM_SIDES,                     //[0..NUM_SIDES-1]
                                                             size_t(Fc)                     //[0..Fc-1]
                                                         },
                                                         OPEN); //Unconnected

    //Number of *physical* pins on each side.
    //Note that his may be more than the logical number of pins (i.e.
    //Type->num_pins) if a logical pin has multiple specified physical
    //pinlocations (i.e. appears on multiple sides of the block)
    auto num_dir = vtr::NdMatrix<int, 4>({
                                             size_t(Type->width),           //[0..width-1]
                                             size_t(Type->height),          //[0..height-1]
                                             size_t(grid.get_num_layers()), //[0..layer-1]
                                             NUM_SIDES                      //[0..NUM_SIDES-1]
                                         },
                                         0);

    //List of *physical* pins of the correct type on each side of the current
    //block type. For a specific width/height/side the valid enteries in the
    //last dimension are [0 .. num_dir[width][height][side]-1]
    //
    //Max possible space alloced for simplicity
    auto dir_list = vtr::NdMatrix<int, 5>({
                                              size_t(Type->width),                                   //[0..width-1]
                                              size_t(Type->height),                                  //[0..height-1]
                                              size_t(grid.get_num_layers()),                         //[0..layer-1]
                                              NUM_SIDES,                                             //[0..NUM_SIDES-1]
                                              size_t(Type->num_pins) * size_t(grid.get_num_layers()) //[0..num_pins * num_layers-1]
                                          },
                                          -1); //Defensive coding: Initialize to invalid

    //Number of currently assigned physical pins
    auto num_done_per_dir = vtr::NdMatrix<int, 4>({
                                                      size_t(Type->width),           //[0..width-1]
                                                      size_t(Type->height),          //[0..height-1]
                                                      size_t(grid.get_num_layers()), //[0..layer-1]
                                                      NUM_SIDES                      //[0..NUM_SIDES-1]
                                                  },
                                                  0);

    //Record the physical pin locations and counts per side/offsets combination
    for (int pin = 0; pin < Type->num_pins; ++pin) {
        auto curr_pin_type = get_pin_type_from_pin_physical_num(Type, pin);
        if (curr_pin_type != pin_type) /* Doing either ipins OR opins */
            continue;

        /* Pins connecting only to global resources get no switches -> keeps area model accurate. */
        if (Type->is_ignored_pin[pin])
            continue;

        for (auto type_layer_index : type_layer) {
            for (int width = 0; width < Type->width; ++width) {
                for (int height = 0; height < Type->height; ++height) {
                    for (e_side side : SIDES) {
                        if (Type->pinloc[width][height][side][pin] == 1) {
                            for (auto i = 0; i < (int)get_layers_connected_to_pin(Type, type_layer_index, pin).size(); i++) {
                                dir_list[width][height][type_layer_index][side][num_dir[width][height][type_layer_index][side]] = pin;
                                num_dir[width][height][type_layer_index][side]++;
                            }
                        }
                    }
                }
            }
        }
    }

    //Total the number of physical pins
    std::vector<int> num_phys_pins_per_layer;
    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        int num_phys_pins = 0;
        for (int width = 0; width < Type->width; ++width) {
            for (int height = 0; height < Type->height; ++height) {
                for (e_side side : SIDES) {
                    num_phys_pins += num_dir[width][height][layer][side]; /* Num. physical pins per type */
                }
            }
        }
        num_phys_pins_per_layer.push_back(num_phys_pins);
    }

    std::vector<t_pin_loc> pin_ordering;

    /* Connection block I use distributes pins evenly across the tracks      *
     * of ALL sides of the clb at once.  Ensures that each pin connects      *
     * to spaced out tracks in its connection block, and that the other      *
     * pins (potentially in other C blocks) connect to the remaining tracks  *
     * first.  Doesn't matter for large Fc, but should make a fairly         *
     * good low Fc block that leverages the fact that usually lots of pins   *
     * are logically equivalent.                                             */

    for (int layer_index = 0; layer_index < grid.get_num_layers(); layer_index++) {
        const e_side init_side = LEFT;
        const int init_width = 0;
        const int init_height = 0;

        e_side side = init_side;
        int width = init_width;
        int height = init_height;
        int pin = 0;
        int pin_index = -1;

        //Determine the order in which physical pins will be considered while building
        //the connection block. This generally tries to order the pins so they are 'spread'
        //out (in hopes of yielding good connection diversity)
        while (pin < num_phys_pins_per_layer[layer_index]) {
            if (height == init_height && width == init_width && side == init_side) {
                //Completed one loop through all the possible offsets/side combinations
                pin_index++;
            }

            advance_to_next_block_side(Type, width, height, side);

            VTR_ASSERT_MSG(pin_index < num_phys_pins_per_layer[layer_index], "Physical block pins bound number of logical block pins");

            if (num_done_per_dir[width][height][layer_index][side] >= num_dir[width][height][layer_index][side]) {
                continue;
            }

            int pin_num = dir_list[width][height][layer_index][side][pin_index];
            VTR_ASSERT(pin_num >= 0);
            VTR_ASSERT(Type->pinloc[width][height][side][pin_num]);

            t_pin_loc pin_loc;
            pin_loc.pin_index = pin_num;
            pin_loc.width_offset = width;
            pin_loc.height_offset = height;
            pin_loc.layer_offset = layer_index;
            pin_loc.side = side;

            pin_ordering.push_back(pin_loc);

            num_done_per_dir[width][height][layer_index][side]++;
            pin++;
        }

        VTR_ASSERT(pin == num_phys_pins_per_layer[layer_index]);
    }

    if (perturb_switch_pattern) {
        load_perturbed_connection_block_pattern(tracks_connected_to_pin,
                                                pin_ordering,
                                                num_seg_type_tracks, num_seg_type_tracks, Fc, directionality);
    } else {
        load_uniform_connection_block_pattern(tracks_connected_to_pin,
                                              pin_ordering,
                                              num_seg_type_tracks, num_seg_type_tracks, Fc, directionality);
    }

#ifdef ENABLE_CHECK_ALL_TRACKS
    check_all_tracks_reach_pins(Type, tracks_connected_to_pin, num_seg_type_tracks,
                                Fc, pin_type);
#endif

    return tracks_connected_to_pin;
}

static void advance_to_next_block_side(t_physical_tile_type_ptr Type, int& width_offset, int& height_offset, e_side& side) {
    //State-machine transitions for advancing around all sides of a block

    //This state-machine transitions in the following order:
    //
    //                           TOP
    //                           --->
    //
    //            **********************************
    //            *    10    |    11    |    12    *
    //            * 3     25 | 6     22 | 9     19 *
    //            *    36    |    35    |    34    *
    //            *----------|----------|----------*
    //       ^    *    13    |    14    |    15    *    |
    //  LEFT |    * 2     26 | 5     23 | 8     20 *    | RIGHT
    //       |    *    33    |    32    |    31    *    v
    //            *----------|----------|----------*
    //            *    16    |    17    |    18    *
    //            * 1     27 | 4     24 | 7     21 *
    //            *    30    |    29    |    28    *
    //            **********************************
    //
    //                           <---
    //                          BOTTOM
    //
    // where each 'square' in the above diagram corresponds to a grid tile of
    // the block of width=3 and height=3. The numbers correspond to the visiting order starting
    // from '1' (width_offset=0, height_offset=0, side=LEFT).
    //
    // Note that for blocks of width == 1 and height == 1 this iterates through the sides
    // in clock-wise order:
    //
    //      ***********
    //      *    2    *
    //      * 1     3 *
    //      *    4    *
    //      ***********
    //

    //Validate current state
    VTR_ASSERT(width_offset >= 0 && width_offset < Type->width);
    VTR_ASSERT(height_offset >= 0 && height_offset < Type->height);
    VTR_ASSERT(side == LEFT || side == RIGHT || side == BOTTOM || side == TOP);

    if (side == LEFT) {
        if (height_offset == Type->height - 1 && width_offset == Type->width - 1) {
            //Finished the last left edge column
            side = TOP; //Turn clockwise
            width_offset = 0;
            height_offset = Type->height - 1;
        } else if (height_offset == Type->height - 1) {
            //Finished the current left edge column
            VTR_ASSERT(width_offset != Type->width - 1);

            //Move right to the next left edge column
            width_offset++;
            height_offset = 0;
        } else {
            height_offset++; //Advance up current left edge column
        }
    } else if (side == TOP) {
        if (height_offset == 0 && width_offset == Type->width - 1) {
            //Finished the last top edge row
            side = RIGHT; //Turn clockwise
            width_offset = Type->width - 1;
            height_offset = Type->height - 1;
        } else if (width_offset == Type->width - 1) {
            //Finished the current top edge row
            VTR_ASSERT(height_offset != 0);

            //Move down to the next top edge row
            height_offset--;
            width_offset = 0;
        } else {
            width_offset++; //Advance right along current top edge row
        }
    } else if (side == RIGHT) {
        if (height_offset == 0 && width_offset == 0) {
            //Finished the last right edge column
            side = BOTTOM; //Turn clockwise
            width_offset = Type->width - 1;
            height_offset = 0;
        } else if (height_offset == 0) {
            //Finished the current right edge column
            VTR_ASSERT(width_offset != 0);

            //Move left to the next right edge column
            width_offset--;
            height_offset = Type->height - 1;
        } else {
            height_offset--; //Advance down current right edge column
        }
    } else {
        VTR_ASSERT(side == BOTTOM);
        if (height_offset == Type->height - 1 && width_offset == 0) {
            //Finished the last bottom edge row
            side = LEFT; //Turn clockwise
            width_offset = 0;
            height_offset = 0;
        } else if (width_offset == 0) {
            //Finished the current bottom edge row
            VTR_ASSERT(height_offset != Type->height - 1);

            //Move up to the next bottom edge row
            height_offset++;
            width_offset = Type->width - 1;
        } else {
            width_offset--; //Advance left along current bottom edge row
        }
    }

    //Validate next state
    VTR_ASSERT(width_offset >= 0 && width_offset < Type->width);
    VTR_ASSERT(height_offset >= 0 && height_offset < Type->height);
    VTR_ASSERT(side == LEFT || side == RIGHT || side == BOTTOM || side == TOP);
}

static float pattern_fmod(float a, float b) {
    /* Compute a modulo b. */
    float raw_result = a - (int)(a / b) * b;

    if (raw_result < 0.0f) {
        return 0.0f;
    }

    if (raw_result >= b) {
        return 0.0f;
    }

    return raw_result;
}

static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 6>& tracks_connected_to_pin,
                                                  const std::vector<t_pin_loc>& pin_locations,
                                                  const int x_chan_width,
                                                  const int y_chan_width,
                                                  const int Fc,
                                                  enum e_directionality directionality) {
    /* Loads the tracks_connected_to_pin array with an even distribution of     *
     * switches across the tracks for each pin.  For example, each pin connects *
     * to every 4.3rd track in a channel, with exactly which tracks a pin       *
     * connects to staggered from pin to pin.                                   */

    /* Uni-directional drive is implemented to ensure no directional bias and this means
     * two important comments noted below                                                */
    /* 1. Spacing should be (W/2)/(Fc/2), and step_size should be spacing/(num_phys_pins),
     *    and lay down 2 switches on an adjacent pair of tracks at a time to ensure
     *    no directional bias. Basically, treat W (even) as W/2 pairs of tracks, and
     *    assign switches to a pair at a time. Can do this because W is guaranteed to
     *    be even-numbered; however same approach cannot be applied to Fc_out pattern
     *    when L > 1 and W <> 2L multiple.
     *
     * 2. This generic pattern should be considered the tileable physical layout,
     *    meaning all track # here are physical #'s,
     *    so later must use vpr_to_phy conversion to find actual logical #'s to connect.
     *    This also means I will not use get_output_block_companion_track to ensure
     *    no bias, since that describes a logical # -> that would confuse people.  */

    int max_width = 0;
    int max_height = 0;

    int num_phys_pins = pin_locations.size();

    /* Keep a record of how many times each track is selected at each
     * (side, dx, dy) of the block. This is used to ensure a diversity of tracks are
     * assigned to pins that might be related. For a given (side, dx, dy), the counts will be
     * decremented after all the tracks have been selected at least once, so the
     * counts will not get too big.
     */
    std::vector<std::vector<std::vector<std::vector<char>>>> excess_tracks_selected;
    excess_tracks_selected.resize(NUM_SIDES);

    for (int i = 0; i < num_phys_pins; ++i) {
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;

        max_width = std::max(max_width, width);
        max_height = std::max(max_height, height);
    }

    for (int iside = 0; iside < NUM_SIDES; iside++) {
        excess_tracks_selected[iside].resize(max_width + 1);

        for (int dx = 0; dx <= max_width; dx++) {
            excess_tracks_selected[iside][dx].resize(max_height + 1);

            for (int dy = 0; dy <= max_height; dy++) {
                int max_chan_width = (((iside == TOP) || (iside == BOTTOM)) ? x_chan_width : y_chan_width);
                excess_tracks_selected[iside][dx][dy].resize(max_chan_width);
            }
        }
    }

    for (int i = 0; i < num_phys_pins; ++i) {
        e_side side = pin_locations[i].side;
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;

        int max_chan_width = (((side == TOP) || (side == BOTTOM)) ? x_chan_width : y_chan_width);

        for (int j = 0; j < max_chan_width; j++) {
            excess_tracks_selected[side][width][height][j] = 0;
        }
    }

    int group_size;

    if (directionality == BI_DIRECTIONAL) {
        group_size = 1;
    } else {
        VTR_ASSERT(directionality == UNI_DIRECTIONAL);
        group_size = 2;
    }

    VTR_ASSERT((x_chan_width % group_size == 0) && (y_chan_width % group_size == 0) && (Fc % group_size == 0));

    /* offset is used to move to a different point in the track space if it is detected that
     * the tracks being assigned overlap recently assigned tracks, with the goal of increasing
     * track diversity.
     */
    int offset = 0;

    for (int i = 0; i < num_phys_pins; ++i) {
        int pin = pin_locations[i].pin_index;
        e_side side = pin_locations[i].side;
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;
        int layer = pin_locations[i].layer_offset;

        /* Bi-directional treats each track separately, uni-directional works with pairs of tracks */
        for (int j = 0; j < (Fc / group_size); ++j) {
            int max_chan_width = (((side == TOP) || (side == BOTTOM)) ? x_chan_width : y_chan_width);
            float step_size = (float)max_chan_width / (float)(Fc * num_phys_pins);

            VTR_ASSERT(Fc > 0);
            float fc_step = (float)max_chan_width / (float)Fc;

            /* We may go outside the track ID space, because of offset, so use modulo arithmetic below. */

            float ftrack = pattern_fmod((i + offset) * step_size, fc_step) + (j * fc_step);
            int itrack = ((int)ftrack) * group_size;

            if (j == 0) {
                /* Check if tracks to be assigned by the algorithm were recently assigned to pins at this
                 * (side, dx, dy). If so, loop through all possible alternative track
                 * assignments to find ones that include the most tracks that haven't been selected recently.
                 */
                for (;;) {
                    int saved_offset_increment = 0;
                    int max_num_unassigned_tracks = 0;

                    /* Across all potential track assignments, determine the maximum number of recently
                     * unassigned tracks that can be assigned this iteration. offset_increment is used to
                     * increment through the potential track assignments. The nested loops inside the
                     * offset_increment loop, iterate through all the tracks associated with a particular
                     * track assignment.
                     */

                    for (int offset_increment = 0; offset_increment < num_phys_pins; offset_increment++) {
                        int num_unassigned_tracks = 0;
                        int num_total_tracks = 0;

                        for (int j2 = 0; j2 < (Fc / group_size); ++j2) {
                            ftrack = pattern_fmod((i + offset + offset_increment) * step_size, fc_step) + (j2 * fc_step);
                            itrack = ((int)ftrack) * group_size;

                            for (int k = 0; k < group_size; ++k) {
                                if (excess_tracks_selected[side][width][height][(itrack + k) % max_chan_width] == 0) {
                                    num_unassigned_tracks++;
                                }

                                num_total_tracks++;
                            }
                        }

                        if (num_unassigned_tracks > max_num_unassigned_tracks) {
                            max_num_unassigned_tracks = num_unassigned_tracks;
                            saved_offset_increment = offset_increment;
                        }

                        if (num_unassigned_tracks == num_total_tracks) {
                            /* We can't do better than this, so end search. */
                            break;
                        }
                    }

                    if (max_num_unassigned_tracks > 0) {
                        /* Use the minimum offset increment that achieves the desired goal of track diversity,
                         * so the patterns produced are similar to the old algorithm (which doesn't explicitly
                         * monitor track diversity).
                         */

                        offset += saved_offset_increment;

                        ftrack = pattern_fmod((i + offset) * step_size, fc_step);
                        itrack = ((int)ftrack) * group_size;

                        break;
                    } else {
                        /* All tracks have been assigned recently. Decrement the excess_tracks_selected counts for
                         * this location (side, dx, dy), modifying the memory of recently assigned
                         * tracks. A decrement is done rather than a reset, so if there was some unevenness of track
                         * assignment, that will be factored into the next round of track assignment.
                         */
                        for (int k = 0; k < max_chan_width; k++) {
                            VTR_ASSERT(excess_tracks_selected[side][width][height][k] > 0);
                            excess_tracks_selected[side][width][height][k]--;
                        }
                    }
                }
            }

            /* Assign the group of tracks for the Fc pattern */
            for (int k = 0; k < group_size; ++k) {
                tracks_connected_to_pin[pin][width][height][layer][side][group_size * j + k] = (itrack + k) % max_chan_width;
                excess_tracks_selected[side][width][height][(itrack + k) % max_chan_width]++;
            }
        }
    }
}

/*AA: Will need to modify this perhaps to acount for the type of the segment that we're loading the cb for. 
 * waiting for reply from VB if yes, sides that are perpendicular to the channel don't connect to it. We look
 * at the side we are at and only distribute those connections in the two sides that work. 
 * 
 * 
 * 
 * 
 * 
 *
 *
 *
 */
static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 6>& tracks_connected_to_pin,
                                                    const std::vector<t_pin_loc>& pin_locations,
                                                    const int x_chan_width,
                                                    const int y_chan_width,
                                                    const int Fc,
                                                    enum e_directionality directionality) {
    /* Loads the tracks_connected_to_pin array with an unevenly distributed     *
     * set of switches across the channel.  This is done for inputs when        *
     * Fc_input = Fc_output to avoid creating "pin domains" -- certain output   *
     * pins being able to talk only to certain input pins because their switch  *
     * patterns exactly line up.  Distribute Fc/2 + 1 switches over half the    *
     * channel and Fc/2 - 1 switches over the other half to make the switch     *
     * pattern different from the uniform one of the outputs.  Also, have half  *
     * the pins put the "dense" part of their connections in the first half of  *
     * the channel and the other half put the "dense" part in the second half,  *
     * to make sure each track can connect to about the same number of ipins.   */

    VTR_ASSERT(directionality == BI_DIRECTIONAL);

    int Fc_dense = (Fc / 2) + 1;
    int Fc_sparse = Fc - Fc_dense; /* Works for even or odd Fc */
    int Fc_half[2];

    int num_phys_pins = pin_locations.size();

    for (int i = 0; i < num_phys_pins; ++i) {
        int pin = pin_locations[i].pin_index;
        e_side side = pin_locations[i].side;
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;
        int layer = pin_locations[i].layer_offset;

        int max_chan_width = (((side == TOP) || (side == BOTTOM)) ? x_chan_width : y_chan_width);
        float step_size = (float)max_chan_width / (float)(Fc * num_phys_pins);

        float spacing_dense = (float)max_chan_width / (float)(2 * Fc_dense);
        float spacing_sparse = (float)max_chan_width / (float)(2 * Fc_sparse);
        float spacing[2];

        /* Flip every pin to balance switch density */
        spacing[i % 2] = spacing_dense;
        Fc_half[i % 2] = Fc_dense;
        spacing[(i + 1) % 2] = spacing_sparse;
        Fc_half[(i + 1) % 2] = Fc_sparse;

        float ftrack = i * step_size; /* Start point.  Staggered from pin to pin */
        int iconn = 0;

        for (int ihalf = 0; ihalf < 2; ihalf++) { /* For both dense and sparse halves. */
            for (int j = 0; j < Fc_half[ihalf]; ++j) {
                /* Can occasionally get wraparound due to floating point rounding.
                 * This is okay because the starting position > 0 when this occurs
                 * so connection is valid and fine */
                int itrack = (int)ftrack;
                itrack = itrack % max_chan_width;
                tracks_connected_to_pin[pin][width][height][layer][side][iconn] = itrack;

                ftrack += spacing[ihalf];
                iconn++;
            }
        }
    } /* End for all physical pins. */
}

#ifdef ENABLE_CHECK_ALL_TRACKS

static void check_all_tracks_reach_pins(t_logical_block_type_ptr type,
                                        int***** tracks_connected_to_pin,
                                        int max_chan_width,
                                        int Fc,
                                        enum e_pin_type ipin_or_opin) {
    /* Checks that all tracks can be reached by some pin.   */
    VTR_ASSERT(max_chan_width > 0);

    std::vector<int> num_conns_to_track(max_chan_width, 0); /* [0..max_chan_width-1] */

    for (int pin = 0; pin < type->num_pins; ++pin) {
        for (int width = 0; width < type->width; ++width) {
            for (int height = 0; height < type->height; ++height) {
                for (int side = 0; side < 4; ++side) {
                    if (tracks_connected_to_pin[pin][width][height][side][0] != OPEN) { /* Pin exists */
                        for (int conn = 0; conn < Fc; ++conn) {
                            int track = tracks_connected_to_pin[pin][width][height][side][conn];
                            num_conns_to_track[track]++;
                        }
                    }
                }
            }
        }
    }

    for (int track = 0; track < max_chan_width; ++track) {
        if (num_conns_to_track[track] <= 0) {
            VTR_LOG_ERROR("check_all_tracks_reach_pins: Track %d does not connect to any CLB %ss.\n",
                          track, (ipin_or_opin == DRIVER ? "OPIN" : "IPIN"));
        }
    }
}
#endif

/* Allocates and loads the track to ipin lookup for each physical grid type. This
 * is the same information as the ipin_to_track map but accessed in a different way. */

static vtr::NdMatrix<std::vector<int>, 5> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 5> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
                                                                             const t_physical_tile_type_ptr Type,
                                                                             const std::set<int> type_layer,
                                                                             const int type_width,
                                                                             const int type_height,
                                                                             const int num_pins,
                                                                             const int max_chan_width,
                                                                             const std::vector<t_segment_inf>& seg_inf) {
    /* [0..max_chan_width-1][0..width][0..height][0..layer-1][0..3].  For each track number
     * it stores a vector for each of the four sides.  x-directed channels will
     * use the TOP and BOTTOM vectors to figure out what clb input pins they
     * connect to above and below them, respectively, while y-directed channels
     * use the LEFT and RIGHT vectors.  Each vector contains an nelem field
     * saying how many ipins it connects to.  The list[0..nelem-1] array then
     * gives the pin numbers.                                                  */

    /* Note that a clb pin that connects to a channel on its RIGHT means that  *
     * that channel connects to a clb pin on its LEFT.  The convention used    *
     * here is always in the perspective of the CLB                            */

    if (num_pins < 1) {
        return vtr::NdMatrix<std::vector<int>, 5>();
    }

    const int num_seg_types = seg_inf.size();
    auto& grid = g_vpr_ctx.device().grid;
    /* Alloc and zero the the lookup table */
    auto track_to_pin_lookup = vtr::NdMatrix<std::vector<int>, 5>({size_t(max_chan_width), size_t(type_width), size_t(type_height), size_t(grid.get_num_layers()), 4});

    /* Count number of pins to which each track connects  */
    for (auto type_layer_index : type_layer) {
        for (int pin = 0; pin < num_pins; ++pin) {
            for (int width = 0; width < type_width; ++width) {
                for (int height = 0; height < type_height; ++height) {
                    for (int side = 0; side < 4; ++side) {
                        /* get number of tracks to which this pin connects */
                        int num_tracks = 0;
                        for (int iseg = 0; iseg < num_seg_types; iseg++) {
                            num_tracks += Fc[pin][seg_inf[iseg].seg_index]; // AA: Fc_in and Fc_out matrices are unified for all segments so need to map index.
                        }
                        for (auto connected_layer : get_layers_pin_is_connected_to(Type, type_layer_index, pin)) {
                            if (!pin_to_track_map[pin][width][height][connected_layer][side].empty()) {
                                num_tracks = std::min(num_tracks,
                                                      (int)pin_to_track_map[pin][width][height][connected_layer][side].size());
                                for (int conn = 0; conn < num_tracks; ++conn) {
                                    int track = pin_to_track_map[pin][width][height][connected_layer][side][conn];
                                    VTR_ASSERT(track < max_chan_width);
                                    VTR_ASSERT(track >= 0);
                                    track_to_pin_lookup[track][width][height][connected_layer][side].push_back(pin);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return track_to_pin_lookup;
}

/*AA: 
 * So I need to update this cause the Fc_xofs and Fc_yofs are size of
 * X and Y segment counts. When going through the side of the logic block,
 * need to consider what segments to build Fc nodes for. More on this: 
 * I may have to update the Fc_out[][][] structure allocator because we have
 * different segments for horizontal and vertical channels. Now we are giving the 
 * fc_specs in the architecture file, which refers to a segment by name and a port
 * for a tile (block type). If we figure out [as done in previous routine 
 * get actual_fc ?] we need distribute them properly with consideration to the 
 * type of the segment they're giving specifications for. So say for uniform, we 
 * distribute them evenly across TOP and BOTTOM for a y-parallel channel and 
 * LEFT and RIGHT for x-parallel channel. 
 */
static void build_unidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                  const RRGraphView& rr_graph,
                                  const int layer,
                                  const int i,
                                  const int j,
                                  const e_side side,
                                  const DeviceGrid& grid,
                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                  const t_chan_width& nodes_per_chan,
                                  const t_chan_details& chan_details_x,
                                  const t_chan_details& chan_details_y,
                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                  t_rr_edge_info_set& rr_edges_to_create,
                                  bool* Fc_clipped,
                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                  const t_direct_inf* directs,
                                  const int num_directs,
                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                  const int num_seg_types,
                                  int& rr_edge_count) {
    /*
     * This routine adds the edges from opins to channels at the specified
     * grid location (i,j) and grid tile side
     */
    *Fc_clipped = false;

    auto type = grid.get_physical_type({i, j, layer});

    int width_offset = grid.get_width_offset({i, j, layer});
    int height_offset = grid.get_height_offset({i, j, layer});

    /* Go through each pin and find its fanout. */
    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        /* Skip global pins and pins that are not of DRIVER type */
        auto pin_type = get_pin_type_from_pin_physical_num(type, pin_index);
        if (pin_type != DRIVER) {
            continue;
        }
        if (type->is_ignored_pin[pin_index]) {
            continue;
        }

        RRNodeId opin_node_index = rr_graph_builder.node_lookup().find_node(layer, i, j, OPIN, pin_index, side);
        if (!opin_node_index) continue; //No valid from node

        for (int iseg = 0; iseg < num_seg_types; iseg++) {
            /* get Fc for this segment type */
            int seg_type_Fc = Fc_out[type->index][pin_index][iseg];
            VTR_ASSERT(seg_type_Fc >= 0);
            if (seg_type_Fc == 0) {
                continue;
            }

            /* Can't do anything if pin isn't at this location */
            if (0 == type->pinloc[width_offset][height_offset][side][pin_index]) {
                continue;
            }

            /* Figure out the chan seg at that side.
             * side is the side of the logic or io block. */
            bool vert = ((side == TOP) || (side == BOTTOM));
            bool pos_dir = ((side == TOP) || (side == RIGHT));
            t_rr_type chan_type = (vert ? CHANX : CHANY);
            int chan = (vert ? (j) : (i));
            int seg = (vert ? (i) : (j));
            int max_len = (vert ? grid.width() : grid.height());
            e_parallel_axis wanted_axis = chan_type == CHANX ? X_AXIS : Y_AXIS;
            int seg_index = get_parallel_seg_index(iseg, seg_index_map, wanted_axis);

            /*The segment at index iseg doesn't have the proper adjacency so skip building Fc_out conenctions for it*/
            if (seg_index < 0)
                continue;

            vtr::NdMatrix<int, 3>& Fc_ofs = (vert ? Fc_xofs : Fc_yofs);
            if (false == pos_dir) {
                --chan;
            }

            /* Skip the location if there is no channel. */
            if (chan < 0) {
                continue;
            }
            if (seg < 1) {
                continue;
            }
            if (seg > int(vert ? grid.width() : grid.height()) - 2) { //-2 since no channels around perim
                continue;
            }
            if (chan > int(vert ? grid.height() : grid.width()) - 2) { //-2 since no channels around perim
                continue;
            }

            const t_chan_seg_details* seg_details = (chan_type == CHANX ? chan_details_x[seg][chan] : chan_details_y[chan][seg]).data();

            if (seg_details[0].length() == 0)
                continue;

            /* Get the list of opin to mux connections for that chan seg. */
            bool clipped;

            for (auto connected_layer : get_layers_pin_is_connected_to(type, layer, pin_index)) {
                /* Check the pin physical layer and connect it to the same layer if necessary */
                rr_edge_count += get_unidir_opin_connections(rr_graph_builder, layer, connected_layer, chan, seg,
                                                             seg_type_Fc, seg_index, chan_type, seg_details,
                                                             opin_node_index,
                                                             rr_edges_to_create,
                                                             Fc_ofs, max_len, nodes_per_chan,
                                                             &clipped);
            }

            if (clipped) {
                *Fc_clipped = true;
            }
        }

        /* Add in direct connections */
        get_opin_direct_connections(rr_graph_builder, rr_graph, layer, i, j, side, pin_index, opin_node_index, rr_edges_to_create,
                                    directs, num_directs, clb_to_clb_directs);
    }
}

/**
 * Parse out which CLB pins should connect directly to which other CLB pins then store that in a clb_to_clb_directs data structure
 * This data structure supplements the the info in the "directs" data structure
 * TODO: The function that does this parsing in placement is poorly done because it lacks generality on heterogeniety, should replace with this one
 */
static t_clb_to_clb_directs* alloc_and_load_clb_to_clb_directs(const t_direct_inf* directs, const int num_directs, int delayless_switch) {
    int i;
    t_clb_to_clb_directs* clb_to_clb_directs;
    char *tile_name, *port_name;
    int start_pin_index, end_pin_index;
    t_physical_tile_type_ptr physical_tile = nullptr;
    t_physical_tile_port tile_port;

    auto& device_ctx = g_vpr_ctx.device();

    clb_to_clb_directs = new t_clb_to_clb_directs[num_directs];

    tile_name = nullptr;
    port_name = nullptr;

    for (i = 0; i < num_directs; i++) {
        //clb_to_clb_directs[i].from_clb_type;
        clb_to_clb_directs[i].from_clb_pin_start_index = 0;
        clb_to_clb_directs[i].from_clb_pin_end_index = 0;
        //clb_to_clb_directs[i]. t_physical_tile_type_ptr to_clb_type;
        clb_to_clb_directs[i].to_clb_pin_start_index = 0;
        clb_to_clb_directs[i].to_clb_pin_end_index = 0;
        clb_to_clb_directs[i].switch_index = 0;

        tile_name = new char[strlen(directs[i].from_pin) + strlen(directs[i].to_pin)];
        port_name = new char[strlen(directs[i].from_pin) + strlen(directs[i].to_pin)];

        // Load from pins
        // Parse out the pb_type name, port name, and pin range
        parse_direct_pin_name(directs[i].from_pin, directs[i].line, &start_pin_index, &end_pin_index, tile_name, port_name);

        // Figure out which type, port, and pin is used
        for (const auto& type : device_ctx.physical_tile_types) {
            if (strcmp(type.name, tile_name) == 0) {
                physical_tile = &type;
                break;
            }
        }

        if (physical_tile == nullptr) {
            VPR_THROW(VPR_ERROR_ARCH, "Unable to find block %s.\n", tile_name);
        }

        clb_to_clb_directs[i].from_clb_type = physical_tile;

        tile_port = find_tile_port_by_name(physical_tile, port_name);

        if (start_pin_index == OPEN) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = tile_port.num_pins - 1;
        }

        // Add clb directs start/end pin indices based on the absolute pin position
        // of the port defined in the direct connection. The CLB is the source one.
        clb_to_clb_directs[i].from_clb_pin_start_index = tile_port.absolute_first_pin_index + start_pin_index;
        clb_to_clb_directs[i].from_clb_pin_end_index = tile_port.absolute_first_pin_index + end_pin_index;

        // Load to pins
        // Parse out the pb_type name, port name, and pin range
        parse_direct_pin_name(directs[i].to_pin, directs[i].line, &start_pin_index, &end_pin_index, tile_name, port_name);

        // Figure out which type, port, and pin is used
        for (const auto& type : device_ctx.physical_tile_types) {
            if (strcmp(type.name, tile_name) == 0) {
                physical_tile = &type;
                break;
            }
        }

        if (physical_tile == nullptr) {
            VPR_THROW(VPR_ERROR_ARCH, "Unable to find block %s.\n", tile_name);
        }

        clb_to_clb_directs[i].to_clb_type = physical_tile;

        tile_port = find_tile_port_by_name(physical_tile, port_name);

        if (start_pin_index == OPEN) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = tile_port.num_pins - 1;
        }

        // Add clb directs start/end pin indices based on the absolute pin position
        // of the port defined in the direct connection. The CLB is the destination one.
        clb_to_clb_directs[i].to_clb_pin_start_index = tile_port.absolute_first_pin_index + start_pin_index;
        clb_to_clb_directs[i].to_clb_pin_end_index = tile_port.absolute_first_pin_index + end_pin_index;

        if (abs(clb_to_clb_directs[i].from_clb_pin_start_index - clb_to_clb_directs[i].from_clb_pin_end_index) != abs(clb_to_clb_directs[i].to_clb_pin_start_index - clb_to_clb_directs[i].to_clb_pin_end_index)) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line,
                      "Range mismatch from %s to %s.\n", directs[i].from_pin, directs[i].to_pin);
        }

        //Set the switch index
        if (directs[i].switch_type > 0) {
            //Use the specified switch
            clb_to_clb_directs[i].switch_index = directs[i].switch_type;
        } else {
            //Use the delayless switch by default
            clb_to_clb_directs[i].switch_index = delayless_switch;
        }
        delete[] tile_name;
        delete[] port_name;
    }

    return clb_to_clb_directs;
}

/* Add all direct clb-pin-to-clb-pin edges to given opin
 *
 * The current opin is located at (layer,x,y) along the specified side
 */
static int get_opin_direct_connections(RRGraphBuilder& rr_graph_builder,
                                       const RRGraphView& rr_graph,
                                       int layer,
                                       int x,
                                       int y,
                                       e_side side,
                                       int opin,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create,
                                       const t_direct_inf* directs,
                                       const int num_directs,
                                       const t_clb_to_clb_directs* clb_to_clb_directs) {
    auto& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr curr_type = device_ctx.grid.get_physical_type({x, y, layer});

    int num_pins = 0;

    int width_offset = device_ctx.grid.get_width_offset({x, y, layer});
    int height_offset = device_ctx.grid.get_height_offset({x, y, layer});
    if (!curr_type->pinloc[width_offset][height_offset][side][opin]) {
        return num_pins; //No source pin on this side
    }

    //Capacity location determined by pin number relative to pins per capacity instance
    int z, relative_opin;
    std::tie(z, relative_opin) = get_capacity_location_from_physical_pin(curr_type, opin);
    VTR_ASSERT(z >= 0 && z < curr_type->capacity);

    /* Iterate through all direct connections */
    for (int i = 0; i < num_directs; i++) {
        /* Find matching direct clb-to-clb connections with the same type as current grid location */
        if (clb_to_clb_directs[i].from_clb_type == curr_type) { //We are at a valid starting point

            if (directs[i].from_side != NUM_SIDES && directs[i].from_side != side) continue;

            //Offset must be in range
            if (x + directs[i].x_offset < int(device_ctx.grid.width() - 1)
                && x + directs[i].x_offset > 0
                && y + directs[i].y_offset < int(device_ctx.grid.height() - 1)
                && y + directs[i].y_offset > 0) {
                //Only add connections if the target clb type matches the type in the direct specification
                t_physical_tile_type_ptr target_type = device_ctx.grid.get_physical_type({x + directs[i].x_offset,
                                                                                          y + directs[i].y_offset,
                                                                                          layer});

                if (clb_to_clb_directs[i].to_clb_type == target_type
                    && z + directs[i].sub_tile_offset < int(target_type->capacity)
                    && z + directs[i].sub_tile_offset >= 0) {
                    /* Compute index of opin with regards to given pins */
                    int max_index = OPEN, min_index = OPEN;
                    bool swap = false;
                    if (clb_to_clb_directs[i].from_clb_pin_start_index > clb_to_clb_directs[i].from_clb_pin_end_index) {
                        swap = true;
                        max_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        min_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    } else {
                        swap = false;
                        min_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        max_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    }

                    if (max_index >= relative_opin && min_index <= relative_opin) {
                        int offset = relative_opin - min_index;
                        /* This opin is specified to connect directly to an ipin, now compute which ipin to connect to */
                        int relative_ipin = OPEN;
                        if (clb_to_clb_directs[i].to_clb_pin_start_index > clb_to_clb_directs[i].to_clb_pin_end_index) {
                            if (swap) {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
                            } else {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
                            }
                        } else {
                            if (swap) {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
                            } else {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
                            }
                        }

                        //directs[i].sub_tile_offset is added to from_capacity(z) to get the
                        // target_capacity
                        int target_cap = z + directs[i].sub_tile_offset;

                        // Iterate over all sub_tiles to get the sub_tile which the target_cap belongs to.
                        const t_sub_tile* target_sub_tile = nullptr;
                        for (const auto& sub_tile : target_type->sub_tiles) {
                            if (sub_tile.capacity.is_in_range(target_cap)) {
                                target_sub_tile = &sub_tile;
                                break;
                            }
                        }
                        VTR_ASSERT(target_sub_tile != nullptr);
                        if (relative_ipin >= target_sub_tile->num_phy_pins) continue;

                        //If this block has capacity > 1 then the pins of z position > 0 are offset
                        //by the number of pins per capacity instance
                        int ipin = get_physical_pin_from_capacity_location(target_type, relative_ipin, target_cap);

                        /* Add new ipin edge to list of edges */
                        std::vector<RRNodeId> inodes;

                        if (directs[i].to_side != NUM_SIDES) {
                            //Explicit side specified, only create if pin exists on that side
                            RRNodeId inode = rr_graph_builder.node_lookup().find_node(layer, x + directs[i].x_offset, y + directs[i].y_offset, IPIN, ipin, directs[i].to_side);
                            if (inode) {
                                inodes.push_back(inode);
                            }
                        } else {
                            //No side specified, get all candidates
                            inodes = rr_graph_builder.node_lookup().find_nodes_at_all_sides(layer, x + directs[i].x_offset, y + directs[i].y_offset, IPIN, ipin);
                        }

                        if (inodes.size() > 0) {
                            //There may be multiple physical pins corresponding to the logical
                            //target ipin. We only need to connect to one of them (since the physical pins
                            //are logically equivalent). This also ensures the graphics look reasonable and map
                            //back fairly directly to the architecture file in the case of pin equivalence
                            RRNodeId inode = pick_best_direct_connect_target_rr_node(rr_graph, from_rr_node, inodes);

                            rr_edges_to_create.emplace_back(from_rr_node, inode, clb_to_clb_directs[i].switch_index, false);
                            ++num_pins;
                        }
                    }
                }
            }
        }
    }
    return num_pins;
}
/* Determines whether the output pins of the specified block type should be perturbed.	*
 *  This is to prevent pathological cases where the output pin connections are		*
 *  spaced such that the connection pattern always skips some types of wire (w.r.t.	*
 *  starting points)									*/
static std::vector<bool> alloc_and_load_perturb_opins(const t_physical_tile_type_ptr type,
                                                      const vtr::Matrix<int>& Fc_out,
                                                      const int max_chan_width,
                                                      const std::vector<t_segment_inf>& segment_inf) {
    int i, Fc_max, iclass, num_wire_types;
    int num, max_primes, factor, num_factors;
    int* prime_factors;
    float step_size = 0;
    float n = 0;
    float threshold = 0.07;

    std::vector<bool> perturb_opins(segment_inf.size(), false);

    i = Fc_max = iclass = 0;
    if (segment_inf.size() > 1) {
        /* Segments of one length are grouped together in the channel.	*
         *  In the future we can determine if any of these segments will	*
         *  encounter the pathological step size case, and determine if	*
         *  we need to perturb based on the segment's frequency (if 	*
         *  frequency is small we should not perturb - testing has found	*
         *  that perturbing a channel when unnecessary increases needed	*
         *  W to achieve the same delay); but for now we just return.	*/
        return perturb_opins;
    } else {
        /* There are as many wire start points as the value of L */
        num_wire_types = segment_inf[0].length;
    }

    /* get Fc_max */
    for (i = 0; i < type->num_pins; ++i) {
        auto pin_type = get_pin_type_from_pin_physical_num(type, i);
        if (Fc_out[i][0] > Fc_max && pin_type == DRIVER) {
            Fc_max = Fc_out[i][0];
        }
    }
    /* Nothing to perturb if Fc=0; no need to perturb if Fc = 1 */
    if (Fc_max == 0 || Fc_max == max_chan_width) {
        return perturb_opins;
    }

    /* Pathological cases occur when the step size, W/Fc, is a multiple of	*
     *  the number of wire starting points, L. Specifically, when the step 	*
     *  size is a multiple of a prime factor of L, the connection pattern	*
     *  will always skip some wires. Thus, we perturb pins if we detect this	*
     *  case.								*/

    /* get an upper bound on the number of prime factors of num_wire_types	*/
    max_primes = (int)floor(log((float)num_wire_types) / log(2.0));
    max_primes = std::max(max_primes, 1); //Minimum of 1 to ensure we allocate space for at least one prime_factor

    prime_factors = new int[max_primes];
    for (i = 0; i < max_primes; i++) {
        prime_factors[i] = 0;
    }

    /* Find the prime factors of num_wire_types */
    num = num_wire_types;
    factor = 2;
    num_factors = 0;
    while (pow((float)factor, 2) <= num) {
        if (num % factor == 0) {
            num /= factor;
            if (factor != prime_factors[num_factors]) {
                prime_factors[num_factors] = factor;
                num_factors++;
            }
        } else {
            factor++;
        }
    }
    if (num_factors == 0) {
        prime_factors[num_factors++] = num_wire_types; /* covers cases when num_wire_types is prime */
    }

    /* Now see if step size is an approximate multiple of one of the factors. A 	*
     *  threshold is used because step size may not be an integer.			*/
    step_size = (float)max_chan_width / Fc_max;
    for (i = 0; i < num_factors; i++) {
        if (vtr::nint(step_size) < prime_factors[i]) {
            perturb_opins[0] = false;
            break;
        }

        n = step_size / prime_factors[i];
        n = n - (float)vtr::nint(n); /* fractinal part */
        if (fabs(n) < threshold) {
            perturb_opins[0] = true;
            break;
        } else {
            perturb_opins[0] = false;
        }
    }
    delete[] prime_factors;

    return perturb_opins;
}

static RRNodeId pick_best_direct_connect_target_rr_node(const RRGraphView& rr_graph,
                                                        RRNodeId from_rr,
                                                        const std::vector<RRNodeId>& candidate_rr_nodes) {
    //With physically equivalent pins there may be multiple candidate rr nodes (which are equivalent)
    //to connect the direct edge to.
    //As a result it does not matter (from a correctness standpoint) which is picked.
    //
    //However intuitively we would expect (e.g. when visualizing the drawn RR graph) that the 'closest'
    //candidate would be picked (i.e. to minimize the drawn edge length).
    //
    //This function attempts to pick the 'best/closest' of the candidates.
    VTR_ASSERT(rr_graph.node_type(from_rr) == OPIN);

    float best_dist = std::numeric_limits<float>::infinity();
    RRNodeId best_rr = RRNodeId::INVALID();

    for (const e_side& from_side : SIDES) {
        /* Bypass those side where the node does not appear */
        if (!rr_graph.is_node_on_specific_side(from_rr, from_side)) {
            continue;
        }

        for (RRNodeId to_rr : candidate_rr_nodes) {
            VTR_ASSERT(rr_graph.node_type(to_rr) == IPIN);
            float to_dist = std::abs(rr_graph.node_xlow(from_rr) - rr_graph.node_xlow(to_rr))
                            + std::abs(rr_graph.node_ylow(from_rr) - rr_graph.node_ylow(to_rr));

            for (const e_side& to_side : SIDES) {
                /* Bypass those side where the node does not appear */
                if (!rr_graph.is_node_on_specific_side(to_rr, to_side)) {
                    continue;
                }

                //Include a partial unit of distance based on side alignment to ensure
                //we preferr facing sides
                if ((from_side == RIGHT && to_side == LEFT)
                    || (from_side == LEFT && to_side == RIGHT)
                    || (from_side == TOP && to_side == BOTTOM)
                    || (from_side == BOTTOM && to_side == TOP)) {
                    //Facing sides
                    to_dist += 0.25;
                } else if (((from_side == RIGHT || from_side == LEFT) && (to_side == TOP || to_side == BOTTOM))
                           || ((from_side == TOP || from_side == BOTTOM) && (to_side == RIGHT || to_side == LEFT))) {
                    //Perpendicular sides
                    to_dist += 0.5;

                } else {
                    //Opposite sides
                    to_dist += 0.75;
                }

                if (to_dist < best_dist) {
                    best_dist = to_dist;
                    best_rr = to_rr;
                }
            }
        }
    }

    VTR_ASSERT(best_rr);

    return best_rr;
}

//Collects the sets of connected non-configurable edges in the RR graph
static void create_edge_groups(EdgeGroups* groups) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    rr_graph.rr_nodes().for_each_edge(
        [&](RREdgeId edge, RRNodeId src, RRNodeId sink) {
            if (!rr_graph.rr_switch_inf(RRSwitchId(rr_graph.rr_nodes().edge_switch(edge))).configurable()) {
                groups->add_non_config_edge(src, sink);
            }
        });

    groups->create_sets();
}

t_non_configurable_rr_sets identify_non_configurable_rr_sets() {
    EdgeGroups groups;
    create_edge_groups(&groups);

    return groups.output_sets();
}

bool pins_connected(t_block_loc cluster_loc,
                    t_physical_tile_type_ptr physical_type,
                    t_logical_block_type_ptr logical_block,
                    int from_pin_logical_num,
                    int to_pin_logical_num) {
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    const auto& rr_spatial_look_up = rr_graph.node_lookup();

    int x = cluster_loc.loc.x;
    int y = cluster_loc.loc.y;
    int layer = cluster_loc.loc.layer;
    int abs_cap = cluster_loc.loc.sub_tile;
    const t_sub_tile* sub_tile = nullptr;

    for (const auto& sub_tile_ : physical_type->sub_tiles) {
        if (sub_tile_.capacity.is_in_range(abs_cap)) {
            sub_tile = &sub_tile_;
            break;
        }
    }
    VTR_ASSERT(sub_tile != nullptr);
    int rel_cap = abs_cap - sub_tile->capacity.low;
    VTR_ASSERT(rel_cap >= 0);

    auto from_pb_pin = logical_block->pin_logical_num_to_pb_pin_mapping.at(from_pin_logical_num);
    int from_pin_physical_num = get_pb_pin_physical_num(physical_type,
                                                        sub_tile,
                                                        logical_block,
                                                        rel_cap,
                                                        from_pb_pin);
    VTR_ASSERT(from_pin_physical_num != OPEN);

    auto to_pb_pin = logical_block->pin_logical_num_to_pb_pin_mapping.at(to_pin_logical_num);
    int to_pin_physical_num = get_pb_pin_physical_num(physical_type,
                                                      sub_tile,
                                                      logical_block,
                                                      rel_cap,
                                                      to_pb_pin);

    VTR_ASSERT(to_pin_physical_num != OPEN);

    RRNodeId from_node = get_pin_rr_node_id(rr_spatial_look_up, physical_type, layer, x, y, from_pin_physical_num);
    VTR_ASSERT(from_node != RRNodeId::INVALID());

    RRNodeId to_node = get_pin_rr_node_id(rr_spatial_look_up, physical_type, layer, x, y, to_pin_physical_num);
    VTR_ASSERT(to_node != RRNodeId::INVALID());

    int num_edges = rr_graph.num_edges(from_node);

    for (int iedge = 0; iedge < num_edges; iedge++) {
        RRNodeId sink_node = rr_graph.edge_sink_node(from_node, iedge);
        if (sink_node == to_node) {
            return true;
        }
    }
    return false;
}

static t_cluster_pin_chain get_cluster_directly_connected_nodes(const std::vector<int>& cluster_pins,
                                                                t_physical_tile_type_ptr physical_type,
                                                                t_logical_block_type_ptr logical_block,
                                                                bool is_flat) {
    std::unordered_set<int> cluster_pins_set(cluster_pins.begin(), cluster_pins.end()); // Faster for search

    VTR_ASSERT(is_flat);
    std::vector<int> pin_index_vec(get_tile_total_num_pin(physical_type), OPEN);

    std::vector<std::vector<t_pin_chain_node>> chains;
    for (auto pin_physical_num : cluster_pins) {
        auto pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);
        auto conn_sink_pins = get_sink_pins_in_cluster(cluster_pins_set,
                                                       physical_type,
                                                       logical_block,
                                                       pin_physical_num);
        VTR_ASSERT(pin_type != OPEN);
        // Continue if the fan-out or fan-in of pin_physical_num is not equal to one or pin is already assigned to a chain.
        if (pin_index_vec[pin_physical_num] >= 0 || conn_sink_pins.size() != 1) {
            continue;
        } else {
            auto node_chain = get_directly_connected_nodes(physical_type,
                                                           logical_block,
                                                           cluster_pins_set,
                                                           pin_physical_num,
                                                           is_flat);
            // node_chain contains the pin_physical_num itself to. Thus, we store the chain if its size is greater than 1.
            if (node_chain.size() > 1) {
                int num_chain = (int)chains.size();
                int chain_idx = get_chain_idx(pin_index_vec, node_chain, num_chain);
                add_pin_chain(node_chain,
                              chain_idx,
                              pin_index_vec,
                              chains,
                              (chain_idx == num_chain));
            }
        }
    }

    auto chain_sinks = get_node_chain_sinks(chains);

    VTR_ASSERT(is_node_chain_sorted(physical_type,
                                    logical_block,
                                    cluster_pins_set,
                                    pin_index_vec,
                                    chains));

    return {pin_index_vec, chains, chain_sinks};
}

static std::vector<int> get_directly_connected_nodes(t_physical_tile_type_ptr physical_type,
                                                     t_logical_block_type_ptr logical_block,
                                                     const std::unordered_set<int>& pins_in_cluster,
                                                     int pin_physical_num,
                                                     bool is_flat) {
    VTR_ASSERT(is_flat);
    if (is_primitive_pin(physical_type, pin_physical_num)) {
        return {pin_physical_num};
    }
    std::vector<int> conn_node_chain;
    auto pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);

    // Forward
    {
        conn_node_chain.push_back(pin_physical_num);
        int last_pin_num = pin_physical_num;
        auto sink_pins = get_sink_pins_in_cluster(pins_in_cluster,
                                                  physical_type,
                                                  logical_block,
                                                  pin_physical_num);
        while (sink_pins.size() == 1) {
            last_pin_num = sink_pins[0];

            if (is_primitive_pin(physical_type, last_pin_num) || pin_type != get_pin_type_from_pin_physical_num(physical_type, last_pin_num)) {
                break;
            }

            conn_node_chain.push_back(sink_pins[0]);

            sink_pins = get_sink_pins_in_cluster(pins_in_cluster,
                                                 physical_type,
                                                 logical_block,
                                                 last_pin_num);
        }
    }
    return conn_node_chain;
}

static bool is_node_chain_sorted(t_physical_tile_type_ptr physical_type,
                                 t_logical_block_type_ptr logical_block,
                                 const std::unordered_set<int>& pins_in_cluster,
                                 const std::vector<int>& pin_index_vec,
                                 const std::vector<std::vector<t_pin_chain_node>>& all_node_chain) {
    int num_chain = (int)all_node_chain.size();
    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const auto& curr_chain = all_node_chain[chain_idx];
        int num_nodes = (int)curr_chain.size();
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            auto curr_node = curr_chain[node_idx];
            int curr_pin_num = curr_node.pin_physical_num;
            auto nxt_idx = curr_node.nxt_node_idx;
            if (pin_index_vec[curr_pin_num] != chain_idx) {
                return false;
            }
            if (nxt_idx == OPEN) {
                continue;
            }
            auto conn_pin_vec = get_sink_pins_in_cluster(pins_in_cluster, physical_type, logical_block, curr_node.pin_physical_num);
            if (conn_pin_vec.size() != 1 || conn_pin_vec[0] != curr_chain[nxt_idx].pin_physical_num) {
                return false;
            }
        }
    }

    return true;
}

static std::vector<int> get_node_chain_sinks(const std::vector<std::vector<t_pin_chain_node>>& cluster_chains) {
    int num_chain = (int)cluster_chains.size();
    std::vector<int> chain_sink_pin(num_chain);

    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const auto& chain = cluster_chains[chain_idx];
        int num_nodes = (int)chain.size();

        bool sink_node_found = false;
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            if (chain[node_idx].nxt_node_idx == OPEN) {
                // Each chain should only contain ONE sink pin
                VTR_ASSERT(!sink_node_found);
                sink_node_found = true;
                chain_sink_pin[chain_idx] = chain[node_idx].pin_physical_num;
            }
        }
        // Make sure that the chain contains a sink pin
        VTR_ASSERT(sink_node_found);
    }

    return chain_sink_pin;
}

static std::vector<int> get_sink_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                 t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 const int pin_physical_num) {
    std::vector<int> sink_pins_in_cluster;
    auto all_conn_pins = get_physical_pin_sink_pins(physical_type,
                                                    logical_block,
                                                    pin_physical_num);
    sink_pins_in_cluster.reserve(all_conn_pins.size());
    for (auto conn_pin : all_conn_pins) {
        if (pins_in_cluster.find(conn_pin) != pins_in_cluster.end()) {
            sink_pins_in_cluster.push_back(conn_pin);
        }
    }

    return sink_pins_in_cluster;
}

static std::vector<int> get_src_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                t_physical_tile_type_ptr physical_type,
                                                t_logical_block_type_ptr logical_block,
                                                const int pin_physical_num) {
    std::vector<int> src_pins_in_cluster;
    auto all_conn_pins = get_physical_pin_src_pins(physical_type,
                                                   logical_block,
                                                   pin_physical_num);
    for (auto conn_pin : all_conn_pins) {
        if (pins_in_cluster.find(conn_pin) != pins_in_cluster.end()) {
            src_pins_in_cluster.push_back(conn_pin);
        }
    }

    return src_pins_in_cluster;
}

static int get_chain_idx(const std::vector<int>& pin_idx_vec, const std::vector<int>& pin_chain, int new_idx) {
    int chain_idx = OPEN;
    for (auto pin_num : pin_chain) {
        int pin_chain_num = pin_idx_vec[pin_num];
        if (pin_chain_num != OPEN) {
            chain_idx = pin_chain_num;
            break;
        }
    }
    if (chain_idx == OPEN) {
        chain_idx = new_idx;
    }
    return chain_idx;
}

static void add_pin_chain(const std::vector<int>& pin_chain,
                          int chain_idx,
                          std::vector<int>& pin_index_vec,
                          std::vector<std::vector<t_pin_chain_node>>& all_chains,
                          bool is_new_chain) {
    std::list<int> new_chain;

    // None of the pins in the chain has been added to all_chains before
    if (is_new_chain) {
        std::vector<t_pin_chain_node> new_pin_chain;
        new_pin_chain.reserve(pin_chain.size());

        // Basically, the next pin connected to the current pin is located in the next element of the array
        int nxt_node_idx = 1;
        for (auto pin_num : pin_chain) {
            // Since we assume that none of the pins in the pin_chain has seen before
            VTR_ASSERT(pin_index_vec[pin_num] == OPEN);
            // Assign the pin its chain index
            pin_index_vec[pin_num] = chain_idx;
            new_pin_chain.emplace_back(pin_num, nxt_node_idx);
            nxt_node_idx++;
        }

        // For the last pin in the chain, there is no next pin.
        new_pin_chain[new_pin_chain.size() - 1].nxt_node_idx = OPEN;

        all_chains.push_back(new_pin_chain);

    } else {
        // If the chain_idx is not equal to the all_chains.size(), it means that some of the pins in the chain has already been added
        // to the data structure which stores all the chains.

        auto& node_chain = all_chains[chain_idx];
        int pin_chain_init_size = (int)node_chain.size();
        // For the pins in the chain which are not added, the pin connected to each of the is located in the vector element next to them.
        // Except for the last node in the chain which has not been added. We set the correct value for that particular pin later.
        int nxt_node_idx = pin_chain_init_size + 1;
        bool is_new_pin_added = false;
        bool found_pin_in_chain = false;
        for (auto pin_num : pin_chain) {
            if (pin_index_vec[pin_num] == OPEN) {
                pin_index_vec[pin_num] = chain_idx;
                node_chain.emplace_back(pin_num, nxt_node_idx);
                nxt_node_idx++;
                is_new_pin_added = true;
            } else {
                found_pin_in_chain = true;
                // pin_num is the pin_number of a pin which is the first pin in the chain that is added to all_chains
                int first_pin_in_chain_idx = OPEN;
                for (int pin_idx = 0; pin_idx < pin_chain_init_size; pin_idx++) {
                    if (node_chain[pin_idx].pin_physical_num == pin_num) {
                        first_pin_in_chain_idx = pin_idx;
                        break;
                    }
                }
                // We assume that this pin is already added to all_chains
                VTR_ASSERT(first_pin_in_chain_idx != OPEN);
                // We assume than if this block is being run, at least one new pin should be added to the chain
                VTR_ASSERT(is_new_pin_added);

                // The last added pin is connected to a pin which is already added to the chain
                node_chain[node_chain.size() - 1].nxt_node_idx = first_pin_in_chain_idx;

                // The rest of the chain should have already been added to the chain
                break;
            }
        }
        // There should be a pin which has been added to the chain before
        VTR_ASSERT(found_pin_in_chain);
    }
}

static std::pair<bool, int> find_create_intra_cluster_sw(RRGraphBuilder& rr_graph,
                                                         std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                                         float R_minW_nmos,
                                                         float R_minW_pmos,
                                                         bool is_rr_sw,
                                                         float delay) {
    const auto& rr_graph_switches = rr_graph.rr_switch();

    if (is_rr_sw) {
        for (int rr_switch_id = 0; rr_switch_id < (int)rr_graph_switches.size(); rr_switch_id++) {
            const auto& rr_sw = rr_graph_switches[RRSwitchId(rr_switch_id)];
            if (rr_sw.intra_tile) {
                if (rr_sw.Tdel == delay) {
                    return std::make_pair(false, rr_switch_id);
                }
            }
        }
        t_rr_switch_inf new_rr_switch_inf = create_rr_switch_from_arch_switch(create_internal_arch_sw(delay),
                                                                              R_minW_nmos,
                                                                              R_minW_pmos);
        RRSwitchId rr_sw_id = rr_graph.add_rr_switch(new_rr_switch_inf);

        return std::make_pair(true, (size_t)rr_sw_id);

    } else {
        // Check whether is there any other intra-tile edge with the same delay
        auto find_res = std::find_if(arch_sw_inf.begin(), arch_sw_inf.end(),
                                     [delay](const std::pair<int, t_arch_switch_inf>& sw_inf_pair) {
                                         const t_arch_switch_inf& sw_inf = std::get<1>(sw_inf_pair);
                                         if (sw_inf.intra_tile && sw_inf.Tdel() == delay) {
                                             return true;
                                         } else {
                                             return false;
                                         }
                                     });

        // There isn't any other intra-tile edge with the same delay - Create a new one!
        if (find_res == arch_sw_inf.end()) {
            auto arch_sw = create_internal_arch_sw(delay);
            int max_key_num = std::numeric_limits<int>::min();
            // Find the maximum edge index
            for (const auto& arch_sw_pair : arch_sw_inf) {
                if (arch_sw_pair.first > max_key_num) {
                    max_key_num = arch_sw_pair.first;
                }
            }
            int new_key_num = ++max_key_num;
            arch_sw_inf.insert(std::make_pair(new_key_num, arch_sw));
            // We assume that the delay of internal switches is not dependent on their fan-in
            // If this assumption proven to not be accurate, the implementation needs to be changed.
            VTR_ASSERT(arch_sw.fixed_Tdel());

            return std::make_pair(true, new_key_num);
        } else {
            return std::make_pair(false, find_res->first);
        }
    }
}

static float get_delay_directly_connected_pins(t_physical_tile_type_ptr physical_type,
                                               t_logical_block_type_ptr logical_block,
                                               const std::unordered_set<int>& cluster_pins,
                                               int first_pin_num,
                                               int second_pin_num) {
    float delay = 0.;

    int curr_pin = first_pin_num;
    while (true) {
        auto sink_pins = get_sink_pins_in_cluster(cluster_pins,
                                                  physical_type,
                                                  logical_block,
                                                  curr_pin);
        // We expect the pins to be located on a chain
        VTR_ASSERT(sink_pins.size() == 1);
        delay += get_edge_delay(physical_type,
                                logical_block,
                                curr_pin,
                                sink_pins[0]);
        if (sink_pins[0] == second_pin_num) {
            break;
        }
        curr_pin = sink_pins[0];
    }

    return delay;
}

static void process_non_config_sets() {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    EdgeGroups groups;
    create_edge_groups(&groups);
    groups.set_device_context(device_ctx);
}
