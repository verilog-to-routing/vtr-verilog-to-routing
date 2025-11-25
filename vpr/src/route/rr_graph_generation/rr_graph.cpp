#include <cstdio>
#include <cmath>
#include <algorithm>
#include <utility>
#include <vector>
#include <ranges>

#include "alloc_and_load_rr_indexed_data.h"
#include "build_scatter_gathers.h"
#include "get_parallel_segs.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_rc_data.h"
#include "switchblock_types.h"
#include "vpr_context.h"
#include "vtr_assert.h"

#include "vtr_util.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_utils.h"
#include "rr_graph.h"
#include "rr_graph_switch_utils.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "rr_graph_intra_cluster.h"
#include "rr_graph_tile_nodes.h"
#include "rr_graph_opin_chan_edges.h"
#include "rr_graph_chan_chan_edges.h"
#include "rr_graph_sg.h"
#include "rr_graph_interposer.h"
#include "rr_graph_timing_params.h"
#include "check_rr_graph.h"
#include "echo_files.h"
#include "build_switchblocks.h"
#include "rr_graph_writer.h"
#include "rr_graph_reader.h"
#include "rr_graph_clock.h"
#include "edge_groups.h"
#include "rr_graph_builder.h"
#include "tileable_rr_graph_builder.h"

#include "rr_types.h"
#include "rr_node_indices.h"

//#define VERBOSE
//used for getting the exact count of each edge type and printing it to std out.

struct t_pin_loc {
    int pin_index;
    int width_offset;
    int height_offset;
    e_side side;
};

/******************* Variables local to this module. ***********************/

/********************* Subroutines local to this module. *******************/
void print_rr_graph_stats();

/**
 * @brief This routine calculates pin connections to tracks for either all input or all output pins based on the Fc value defined in the architecture file.
 *        For the requested pin type (input or output), it will loop through all segments and calculate how many connections should be made, 
 *        returns the connections to be made for that type of pin in a matrix.   
 * 
 *   @param pin_type Specifies whether the routine should connect tracks to *INPUT* pins or connect *OUTPUT* pins to tracks.
 *   @param Fc Actual Fc value described in the architecture file for all pins of the specific physical type ([0..number_of_pins-1][0..number_of_segments_types-1]).
 *   @param tile_type Physical type information, such as total number of pins, block width, block height, and etc.
 *   @param perturb_switch_pattern Specifies whether connections should be distributed unevenly across the channel or not.
 *   @param directionality Segment directionality: should be either *UNI-DIRECTIONAL* or *BI-DIRECTIONAL* 
 *   @param seg_inf Segments type information, such as length, frequency, and etc.
 *   @param sets_per_seg_type Number of available sets within the channel_width of each segment type.
 * 
 * @return an 4D matrix which keeps the track indices connected to each pin ([0..num_pins-1][0..width-1][0..height-1][0..layer-1][0..sides-1]).
 * 
 */
static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr tile_type,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const std::vector<int>& sets_per_seg_type);

/**
 * @brief This routine calculates pin connections to tracks for a specific type and a specific segment based on the Fc value 
 * defined for each pin in the architecture file. This routine is called twice for each combination of block type and segment 
 * type: 1) connecting tracks to input pins 2) connecting output pins to tracks.  
 * 
 *   @param pin_type Specifies whether the routine should connect tracks to *INPUT* pins or connect *OUTPUT* pins to tracks.
 *   @param Fc Actual Fc value described in the architecture file for all pins of the specific physical type ([0..number_of_pins-1][0..number_of_segments_types-1]).
 *   @param seg_type_tracks Number of tracks that is available for the specific segment type.
 *   @param seg_index The index of the segment type to which the function tries to connect pins.
 *   @param max_Fc Used to allocate max possible space for simplicity.
 *   @param tile_type Physical type information, such as total number of pins, block width, block height, and etc.
 *   @param perturb_switch_pattern Specifies whether connections should be distributed unevenly across the channel or not.
 *   @param directionality Segment directionality, should be either *UNI-DIRECTIONAL* or *BI-DIRECTIONAL* 
 * 
 * @return an 5D matrix which keeps the track indices connected to each pin ([0..num_pins-1][0..width-1][0..height-1][0..layer-1][0..sides-1][0..Fc_to_curr_seg_type-1]).
 */
static vtr::NdMatrix<int, 5> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const vtr::Matrix<int>& Fc,
                                                            const int seg_type_tracks,
                                                            const int seg_index,
                                                            const int max_Fc,
                                                            const t_physical_tile_type_ptr tile_type,
                                                            const bool perturb_switch_pattern,
                                                            const e_directionality directionality);

static void advance_to_next_block_side(t_physical_tile_type_ptr tile_type, int& width_offset, int& height_offset, e_side& side);

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
                                                                             const int width,
                                                                             const int height,
                                                                             const int num_pins,
                                                                             const int max_chan_width,
                                                                             const std::vector<t_segment_inf>& seg_inf);

static std::function<void(t_chan_width*)> alloc_and_load_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                                  t_rr_graph_storage& L_rr_node,
                                                                  const RRGraphView& rr_graph,
                                                                  const size_t num_seg_types,
                                                                  const size_t num_seg_types_x,
                                                                  const size_t num_seg_types_y,
                                                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                                                  const t_chan_details& chan_details_x,
                                                                  const t_chan_details& chan_details_y,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_x,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_y,
                                                                  const t_pin_to_track_lookup& opin_to_track_map,
                                                                  const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                                                                  const std::vector<t_bottleneck_link>& sg_links,
                                                                  const std::vector<std::pair<RRNodeId, int>>& sg_node_indices,
                                                                  const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                                                                  t_sb_connection_map* sb_conn_map,
                                                                  const DeviceGrid& grid,
                                                                  const int Fs,
                                                                  t_sblock_pattern& sblock_pattern,
                                                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                  const t_chan_width& chan_width,
                                                                  const int wire_to_ipin_switch,
                                                                  const int delayless_switch,
                                                                  const e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const std::vector<t_direct_inf>& directs,
                                                                  const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const e_clock_modeling clock_modeling,
                                                                  const int route_verbosity);

/**
 * @brief Add the edges between pins and their respective SINK/SRC. It is important to mention that in contrast to another similar function,
 * the delay of these edges is not necessarily zero. If the primitive block which a SINK/SRC belongs to is a combinational block, the delay of
 * the edge is equal to the pin delay. This is done in order to make the router lookahead aware of the different IPIN delays. In this way, more critical
 * nets are routed to the pins with less delay.
 */
static void connect_tile_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                          std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                          const std::vector<int>& class_num_vec,
                                          const t_physical_tile_loc& root_loc,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          const int delayless_switch,
                                          t_physical_tile_type_ptr physical_type_ptr);

static void alloc_and_load_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                                         std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                         t_physical_tile_type_ptr physical_tile,
                                         const t_physical_tile_loc& root_loc,
                                         const int delayless_switch);

static float pattern_fmod(float a, float b);

/**
 * @brief Loads the tracks_connected_to_pin array with an even distribution of switches across the tracks for each pin.
 * 
 *   @param tracks_connected_to_pin The function loads up this data structure with a track index for each pin ([0..num_pins-1][0..width-1][0..height-1][0..layer-1][0..sides-1][0..Fc-1]]).
 *   @param pin_locations Physical pin information, such as pin_index in the physical type, side, etc.
 *   @param Fc Actual Fc value described in the architecture file for all pins of the specific physical type ([0..number_of_pins-1][0..number_of_segments-1]).
 *   @param seg_index The index of the segment type to which the function tries to connect pins.
 *   @param x_chan_width Number of tracks in x-axis.
 *   @param y_chan_width Number of tracks in y-axis.
 *   @param directionality Segment directionality, should be either *UNI-DIRECTIONAL* or *BI-DIRECTIONAL* 
 * 
 */
static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
                                                  const std::vector<t_pin_loc>& pin_locations,
                                                  const vtr::Matrix<int>& Fc,
                                                  const int seg_index,
                                                  const int x_chan_width,
                                                  const int y_chan_width,
                                                  const enum e_directionality directionality);

/**
 * @brief Loads the tracks_connected_to_pin array with an unevenly distributed set of switches across the channel.
 * 
 *   @param tracks_connected_to_pin The funtion loads up this data structure with a track index for each pin ([0..num_pins-1][0..width-1][0..height-1][0..layer-1][0..sides-1][0..Fc-1]]).                        
 *   @param pin_locations Physical pin informations, such as pin_index in the physical type, side, and etc.
 *   @param x_chan_width Number of tracks in x-axis.
 *   @param y_chan_width Number of tracks in y-axis.
 *   @param Fc Actual Fc value described in the architecture file for all pins of the specific phyiscal type ([0..number_of_pins-1][0..number_of_segment_types-1]).
 *   @param seg_index The index of the segment type to which the function tries to connect pins.
 *   @param directionality Segment directionality, should be either *UNI-DIRECTIONAL* or *BI-DIRECTIONAL*
 */
static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
                                                    const std::vector<t_pin_loc>& pin_locations,
                                                    const int x_chan_width,
                                                    const int y_chan_width,
                                                    const vtr::Matrix<int>& Fc,
                                                    const int seg_index,
                                                    const enum e_directionality directionality);

/**
 * @brief Determines whether the output pins of the specified block type should be perturbed.
 *  This is to prevent pathological cases where the output pin connections are
 *  spaced such that the connection pattern always skips some types of wire (w.r.t.
 *  starting points)
 * 
 * @param type The block type to check
 * @param Fc_out The Fc values for the output pins
 * @param max_chan_width The maximum channel width
 * @param segment_inf The segment information
 */
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
                                                                   const std::vector<int>& sets_per_seg_type,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_in,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                   const enum e_directionality directionality);

static void add_intra_tile_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          t_physical_tile_type_ptr physical_tile,
                                          const t_physical_tile_loc& root_loc);

void alloc_and_load_edges(RRGraphBuilder& rr_graph_builder,
                          const t_rr_edge_info_set& rr_edges_to_create);

static std::vector<t_seg_details> alloc_and_load_global_route_seg_details(const int global_route_switch);

static void process_non_config_sets();

static void build_rr_graph(e_graph_type graph_type,
                           const std::vector<t_physical_tile_type>& types,
                           const DeviceGrid& grid,
                           t_chan_width nodes_per_chan,
                           const e_switch_block_type sb_type,
                           const int Fs,
                           const std::vector<t_switchblock_inf>& switchblocks,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const e_base_cost_type base_cost_type,
                           const e_clock_modeling clock_modeling,
                           const std::vector<t_direct_inf>& directs,
                           const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                           const std::vector<t_layer_def>& interposer_inf,
                           RRSwitchId& wire_to_rr_ipin_switch,
                           bool is_flat,
                           int* Warnings,
                           const int route_verbosity);

/**
 * Return the ID for delay-less switch. If the RR graph is loaded from a file, then the assumption
 * is that the returned ID should be a RR switch ID not architecture ID.
 * @param det_routing_arch Contain the information from architecture file
 * @param load_rr_graph Indicate whether the RR graph is loaded from a file
 */
static int get_delayless_switch_id(const t_det_routing_arch& det_routing_arch,
                                   bool load_rr_graph);

/**
 * @brief Calculates the routing channel width at each grid location.
 *
 * Iterates through all RR nodes and counts how many wires pass through each (layer, x, y) location
 * for both horizontal (CHANX) and vertical (CHANY) channels.
 */
static void alloc_and_init_channel_width();

/******************* Subroutine definitions *******************************/

void create_rr_graph(e_graph_type graph_type,
                     const std::vector<t_physical_tile_type>& block_types,
                     const DeviceGrid& grid,
                     const t_chan_width& nodes_per_chan,
                     t_det_routing_arch& det_routing_arch,
                     const std::vector<t_segment_inf>& segment_inf,
                     const t_router_opts& router_opts,
                     const std::vector<t_direct_inf>& directs,
                     int* Warnings,
                     bool is_flat) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    DeviceContext& mutable_device_ctx = g_vpr_ctx.mutable_device();
    bool echo_enabled = getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA);
    const char* echo_file_name = getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA);
    bool load_rr_graph = !det_routing_arch.read_rr_graph_filename.empty();

    if (device_ctx.chan_width == nodes_per_chan && !device_ctx.rr_graph.empty()) {
        // No change in channel width, so skip re-building RR graph
        if (is_flat && !device_ctx.rr_graph_is_flat) {
            VTR_LOG("RR graph channel widths unchanged, intra-cluster resources should be added...\n");
        } else {
            VTR_LOG("RR graph channel widths unchanged, skipping RR graph rebuild\n");
            return;
        }
    } else {
        if (load_rr_graph) {
            if (device_ctx.loaded_rr_graph_filename != det_routing_arch.read_rr_graph_filename) {
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
                             &det_routing_arch.wire_to_rr_ipin_switch,
                             det_routing_arch.read_rr_graph_filename.c_str(),
                             &mutable_device_ctx.loaded_rr_graph_filename,
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
            free_rr_graph();
            if (e_graph_type::UNIDIR_TILEABLE != graph_type) {
                build_rr_graph(graph_type,
                               block_types,
                               grid,
                               nodes_per_chan,
                               det_routing_arch.switch_block_type,
                               det_routing_arch.Fs,
                               det_routing_arch.switchblocks,
                               segment_inf,
                               det_routing_arch.global_route_switch,
                               det_routing_arch.wire_to_arch_ipin_switch,
                               det_routing_arch.delayless_switch,
                               det_routing_arch.R_minW_nmos,
                               det_routing_arch.R_minW_pmos,
                               router_opts.base_cost_type,
                               router_opts.clock_modeling,
                               directs,
                               device_ctx.arch->scatter_gather_patterns,
                               device_ctx.arch->grid_layout().layers,
                               det_routing_arch.wire_to_rr_ipin_switch,
                               is_flat,
                               Warnings,
                               router_opts.route_verbosity);
            } else {
                // Note: We do not support dedicated network for clocks in tileable rr_graph generation
                build_tileable_unidir_rr_graph(block_types,
                                               grid,
                                               nodes_per_chan,
                                               det_routing_arch.switch_block_type,
                                               det_routing_arch.Fs,
                                               det_routing_arch.switch_block_subtype,
                                               det_routing_arch.sub_fs,
                                               segment_inf,
                                               det_routing_arch.delayless_switch,
                                               det_routing_arch.wire_to_arch_ipin_switch,
                                               det_routing_arch.R_minW_nmos,
                                               det_routing_arch.R_minW_pmos,
                                               router_opts.base_cost_type,
                                               directs,
                                               det_routing_arch.wire_to_rr_ipin_switch,
                                               det_routing_arch.shrink_boundary,  // Shrink to the smallest boundary, no routing wires for empty zone
                                               det_routing_arch.perimeter_cb,     // Now I/O or any programmable blocks on perimeter can have full cb access (both cbx and cby)
                                               det_routing_arch.through_channel,  // Allow/Prohibit through tracks across multi-height and multi-width grids
                                               det_routing_arch.opin2all_sides,   // Allow opin of grid to directly drive routing tracks at all sides of a switch block
                                               det_routing_arch.concat_wire,      // Allow end-point tracks to be wired to a starting point track on the opposite in a switch block.It means a wire can be continued in the same direction to another wire
                                               det_routing_arch.concat_pass_wire, // Allow passing tracks to be wired to the routing tracks in the same direction in a switch block. It means that a pass wire can jump in the same direction to another
                                               Warnings);
            }
        }

        // Check if there is an edge override file to read and that it is not already loaded.
        if (!det_routing_arch.read_rr_edge_override_filename.empty()
            && det_routing_arch.read_rr_edge_override_filename != device_ctx.loaded_rr_edge_override_filename) {

            load_rr_edge_delay_overrides(det_routing_arch.read_rr_edge_override_filename,
                                         mutable_device_ctx.rr_graph_builder,
                                         device_ctx.rr_graph);

            // Remember the loaded filename to avoid reloading it before the RR graph is cleared.
            mutable_device_ctx.loaded_rr_edge_override_filename = det_routing_arch.read_rr_edge_override_filename;
        }
    }

    if (is_flat) {
        int delayless_switch = get_delayless_switch_id(det_routing_arch, load_rr_graph);
        VTR_ASSERT(delayless_switch != UNDEFINED);
        build_intra_cluster_rr_graph(graph_type,
                                     grid,
                                     block_types,
                                     device_ctx.rr_graph,
                                     delayless_switch,
                                     det_routing_arch.R_minW_nmos,
                                     det_routing_arch.R_minW_pmos,
                                     mutable_device_ctx.rr_graph_builder,
                                     is_flat,
                                     load_rr_graph);

        // Reorder nodes upon needs in algorithms and router options
        if (router_opts.reorder_rr_graph_nodes_algorithm != DONT_REORDER) {
            mutable_device_ctx.rr_graph_builder.reorder_nodes(router_opts.reorder_rr_graph_nodes_algorithm,
                                                              router_opts.reorder_rr_graph_nodes_threshold,
                                                              router_opts.reorder_rr_graph_nodes_seed);
        }

        mutable_device_ctx.rr_graph_is_flat = true;
    }

    process_non_config_sets();

    rr_set_sink_locs(device_ctx.rr_graph, mutable_device_ctx.rr_graph_builder, grid);

    verify_rr_node_indices(grid,
                           device_ctx.rr_graph,
                           device_ctx.rr_indexed_data,
                           device_ctx.rr_graph.rr_nodes(),
                           is_flat);

    alloc_and_init_channel_width();

    print_rr_graph_stats();

    // Write out rr graph file if needed - Currently, writing the flat rr-graph is not supported since loading from a flat rr-graph is not supported.
    // When this function is called in any stage other than routing, the is_flat flag passed to this function is false, regardless of the flag passed
    // through command line. So, the graph corresponding to global resources will be created and written down to file if needed. During routing, if flat-routing
    // is enabled, intra-cluster resources will be added to the graph, but this new bigger graph will not be written down.
    if (!det_routing_arch.write_rr_graph_filename.empty() && !is_flat) {
        write_rr_graph(&mutable_device_ctx.rr_graph_builder,
                       &mutable_device_ctx.rr_graph,
                       device_ctx.physical_tile_types,
                       &mutable_device_ctx.rr_indexed_data,
                       &mutable_device_ctx.rr_rc_data,
                       grid,
                       device_ctx.arch_switch_inf,
                       device_ctx.arch,
                       &mutable_device_ctx.chan_width,
                       det_routing_arch.write_rr_graph_filename.c_str(),
                       echo_enabled,
                       echo_file_name,
                       is_flat);
    }
}

static void add_intra_tile_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          t_physical_tile_type_ptr physical_tile,
                                          const t_physical_tile_loc& root_loc) {
    std::vector<int> pin_num_vec = get_flat_tile_pins(physical_tile);
    for (int pin_physical_num : pin_num_vec) {
        if (is_pin_on_tile(physical_tile, pin_physical_num)) {
            continue;
        }
        RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                     physical_tile,
                                                     root_loc,
                                                     pin_physical_num);
        VTR_ASSERT(pin_rr_node_id != RRNodeId::INVALID());
        t_logical_block_type_ptr logical_block = get_logical_block_from_pin_physical_num(physical_tile, pin_physical_num);
        std::vector<int> driving_pins = get_physical_pin_src_pins(physical_tile, logical_block, pin_physical_num);
        for (int driving_pin : driving_pins) {
            RRNodeId driving_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                              physical_tile,
                                                              root_loc,
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
    const RRGraphView& rr_graph = g_vpr_ctx.device().rr_graph;

    size_t num_rr_edges = 0;
    for (const t_rr_node& rr_node : rr_graph.rr_nodes()) {
        num_rr_edges += rr_graph.edges(rr_node.id()).size();
    }

    VTR_LOG("  RR Graph Nodes: %zu\n", rr_graph.num_nodes());
    VTR_LOG("  RR Graph Edges: %zu\n", num_rr_edges);
}

static void build_rr_graph(e_graph_type graph_type,
                           const std::vector<t_physical_tile_type>& types,
                           const DeviceGrid& grid,
                           t_chan_width nodes_per_chan,
                           const e_switch_block_type sb_type,
                           const int Fs,
                           const std::vector<t_switchblock_inf>& switchblocks,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const e_base_cost_type base_cost_type,
                           const e_clock_modeling clock_modeling,
                           const std::vector<t_direct_inf>& directs,
                           const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                           const std::vector<t_layer_def>& interposer_inf,
                           RRSwitchId& wire_to_rr_ipin_switch,
                           bool is_flat,
                           int* Warnings,
                           const int route_verbosity) {
    vtr::ScopedStartFinishTimer timer("Build routing resource graph");

    // Reset warning flag
    *Warnings = RR_GRAPH_NO_WARN;

    // Decode the graph_type
    bool is_global_graph = (e_graph_type::GLOBAL == graph_type);
    bool use_full_seg_groups = (e_graph_type::UNIDIR_TILEABLE == graph_type);
    e_directionality directionality = (e_graph_type::BIDIR == graph_type) ? BI_DIRECTIONAL : UNI_DIRECTIONAL;
    if (is_global_graph) {
        directionality = BI_DIRECTIONAL;
    }

    // Global routing uses a single longwire track

    int max_chan_width = nodes_per_chan.max = (is_global_graph ? 1 : nodes_per_chan.max);
    int max_chan_width_x = nodes_per_chan.x_max = (is_global_graph ? 1 : nodes_per_chan.x_max);
    int max_chan_width_y = nodes_per_chan.y_max = (is_global_graph ? 1 : nodes_per_chan.y_max);

    VTR_ASSERT(max_chan_width_x > 0 && max_chan_width_y > 0);

    DeviceContext& device_ctx = g_vpr_ctx.mutable_device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    std::vector<t_clb_to_clb_directs> clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, delayless_switch);

    // START SEG_DETAILS
    const size_t num_segments = segment_inf.size();
    device_ctx.rr_graph_builder.reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        device_ctx.rr_graph_builder.add_rr_segment(segment_inf[iseg]);
    }

    t_unified_to_parallel_seg_index segment_index_map;
    std::vector<t_segment_inf> segment_inf_x = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::X_AXIS);
    std::vector<t_segment_inf> segment_inf_y = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::Y_AXIS);
    std::vector<t_segment_inf> segment_inf_z = get_parallel_segs(segment_inf, segment_index_map, e_parallel_axis::Z_AXIS);

    std::vector<t_seg_details> seg_details_x;
    std::vector<t_seg_details> seg_details_y;

    if (is_global_graph) {
        // Sets up a single unit length segment type for global routing.
        seg_details_x = alloc_and_load_global_route_seg_details(global_route_switch);
        seg_details_y = alloc_and_load_global_route_seg_details(global_route_switch);

    } else {
        // Setup segments including distributing tracks and staggering.
        // If use_full_seg_groups is specified, max_chan_width may be
        // changed. Warning should be singled to caller if this happens.

        // Need to setup segments along x & y axes separately, due to different
        // max_channel_widths and segment specifications.

        size_t max_dim = std::max(grid.width(), grid.height()) - 2; //-2 for no perim channels

        // Get x & y segments separately
        seg_details_x = alloc_and_load_seg_details(&max_chan_width_x,
                                                   max_dim, segment_inf_x,
                                                   use_full_seg_groups, directionality);

        seg_details_y = alloc_and_load_seg_details(&max_chan_width_y,
                                                   max_dim, segment_inf_y,
                                                   use_full_seg_groups, directionality);

        if (nodes_per_chan.x_max != max_chan_width_x || nodes_per_chan.y_max != max_chan_width_y) {
            nodes_per_chan.x_max = max_chan_width_x;
            *Warnings |= RR_GRAPH_WARN_CHAN_X_WIDTH_CHANGED;

        } else if (nodes_per_chan.y_max != max_chan_width_y) {
            nodes_per_chan.y_max = max_chan_width_y;
            *Warnings |= RR_GRAPH_WARN_CHAN_Y_WIDTH_CHANGED;
        }

        // TODO: Fix
        // if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SEG_DETAILS)) {
        // dump_seg_details(seg_details, max_chan_width,
        // getEchoFileName(E_ECHO_SEG_DETAILS));
        // }
    }

    // Map the internal segment indices of the networks
    if (clock_modeling == DEDICATED_NETWORK) {
        ClockRRGraphBuilder::map_relative_seg_indices(segment_index_map);
    }
    // END SEG_DETAILS

    // START CHAN_DETAILS

    t_chan_details chan_details_x;
    t_chan_details chan_details_y;

    alloc_and_load_chan_details(grid, nodes_per_chan,
                                seg_details_x, seg_details_y,
                                chan_details_x, chan_details_y);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CHAN_DETAILS)) {
        dump_chan_details(chan_details_x, chan_details_y, &nodes_per_chan,
                          grid, getEchoFileName(E_ECHO_CHAN_DETAILS));
    }
    // END CHAN_DETAILS

    // START FC
    // Determine the actual value of Fc
    std::vector<vtr::Matrix<int>> Fc_in;  // [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1]
    std::vector<vtr::Matrix<int>> Fc_out; // [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1]

    // Get maximum number of pins across all blocks
    int max_pins = types[0].num_pins;
    for (const t_physical_tile_type& type : types) {
        if (is_empty_type(&type)) {
            continue;
        }

        if (type.num_pins > max_pins) {
            max_pins = type.num_pins;
        }
    }

    // Get the number of 'sets' for each segment type -- unidirectional architectures have two tracks in a set, bidirectional have one
    int total_sets = max_chan_width;
    int total_sets_x = max_chan_width_x;
    int total_sets_y = max_chan_width_y;

    if (directionality == UNI_DIRECTIONAL) {
        VTR_ASSERT(max_chan_width % 2 == 0);
        total_sets /= 2;
        total_sets_x /= 2;
        total_sets_y /= 2;
    }
    std::vector<int> sets_per_seg_type_x = get_seg_track_counts(total_sets_x, segment_inf_x, use_full_seg_groups);
    std::vector<int> sets_per_seg_type_y = get_seg_track_counts(total_sets_y, segment_inf_y, use_full_seg_groups);
    std::vector<int> sets_per_seg_type = get_seg_track_counts(total_sets, segment_inf, use_full_seg_groups);

    if (is_global_graph) {
        // All pins can connect during global routing
        auto ones = vtr::Matrix<int>({size_t(max_pins), segment_inf.size()}, 1);
        Fc_in = std::vector<vtr::Matrix<int>>(types.size(), ones);
        Fc_out = std::vector<vtr::Matrix<int>>(types.size(), ones);
    } else {
        bool Fc_clipped = false;
        Fc_in = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type,
                                         e_fc_type::IN, directionality, &Fc_clipped, is_flat);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }
        Fc_clipped = false;
        Fc_out = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type,
                                          e_fc_type::OUT, directionality, &Fc_clipped, is_flat);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }

        for (const t_physical_tile_type& type : types) {
            int i = type.index;

            // Skip "EMPTY"
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
                        block_type_pin_index_to_name(&type, j, is_flat).c_str(),
                        k,
                        segment_inf[k].name.c_str(),
                        Fc_out[i][j][k],
                        Fc_in[i][j][k]);
#endif
                    VTR_ASSERT_MSG(Fc_out[i][j][k] == 0 || Fc_in[i][j][k] == 0,
                                   "Pins must be inputs or outputs (i.e. can not have both non-zero Fc_out and Fc_in)");
                }
            }
        }
    }

    auto perturb_ipins = alloc_and_load_perturb_ipins(types.size(), segment_inf.size(),
                                                      sets_per_seg_type, Fc_in, Fc_out, directionality);
    // END FC

    // Alloc node lookups, count nodes, alloc rr nodes
    int num_rr_nodes = 0;

    // Add routing resources to rr_graph lookup table
    alloc_and_load_rr_node_indices(device_ctx.rr_graph_builder,
                                   nodes_per_chan,
                                   grid,
                                   &num_rr_nodes,
                                   chan_details_x,
                                   chan_details_y);

    size_t expected_node_count = num_rr_nodes;
    if (clock_modeling == DEDICATED_NETWORK) {
        expected_node_count += ClockRRGraphBuilder::estimate_additional_nodes(grid);
        device_ctx.rr_graph_builder.reserve_nodes(expected_node_count);
    }
    device_ctx.rr_graph_builder.resize_nodes(num_rr_nodes);

    // START SB LOOKUP
    // Alloc and load the switch block lookup
    vtr::NdMatrix<std::vector<int>, 3> switch_block_conn;
    t_sblock_pattern unidir_sb_pattern;
    t_sb_connection_map* sb_conn_map = nullptr; //for custom switch blocks

    //We are careful to use a single seed each time build_rr_graph is called
    //to initialize the random number generator (RNG) which is (potentially)
    //used when creating custom switchblocks. This ensures that build_rr_graph()
    //is deterministic -- always producing the same RR graph.
    constexpr unsigned SWITCHPOINT_RNG_SEED = 1;
    vtr::RngContainer switchpoint_rng(SWITCHPOINT_RNG_SEED);
    const std::vector<bool>& inter_cluster_prog_rr = device_ctx.inter_cluster_prog_routing_resources;

    if (is_global_graph) {
        switch_block_conn = alloc_and_load_switch_block_conn(&nodes_per_chan, e_switch_block_type::SUBSET, /*Fs=*/3);
    } else {
        if (sb_type == e_switch_block_type::CUSTOM) {
            sb_conn_map = alloc_and_load_switchblock_permutations(chan_details_x, chan_details_y,
                                                                  grid, inter_cluster_prog_rr,
                                                                  switchblocks, nodes_per_chan, directionality,
                                                                  switchpoint_rng);
        } else {
            if (directionality == BI_DIRECTIONAL) {
                switch_block_conn = alloc_and_load_switch_block_conn(&nodes_per_chan, sb_type, Fs);
            } else {
                // it looks like we get unbalanced muxing from this switch block code with Fs > 3
                VTR_ASSERT(Fs == 3);
                // Since directionality is a C enum it could technically be any value. This assertion makes sure it's either BI_DIRECTIONAL or UNI_DIRECTIONAL
                VTR_ASSERT(directionality == UNI_DIRECTIONAL);

                unidir_sb_pattern = alloc_sblock_pattern_lookup(grid, nodes_per_chan);
                for (size_t i = 0; i < grid.width() - 1; i++) {
                    for (size_t j = 0; j < grid.height() - 1; j++) {
                        load_sblock_pattern_lookup(i, j, grid, nodes_per_chan,
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
    }
    // END SB LOOKUP

    vtr::NdMatrix<std::vector<t_bottleneck_link>, 2> interdie_3d_links;

    std::vector<t_scatter_gather_pattern> sg_patterns_copy = scatter_gather_patterns;

    convert_interposer_cuts_to_sg_patterns(interposer_inf, sg_patterns_copy);

    const std::vector<t_bottleneck_link> bottleneck_links = alloc_and_load_scatter_gather_connections(sg_patterns_copy,
                                                                                                      inter_cluster_prog_rr,
                                                                                                      segment_inf_x, segment_inf_y, segment_inf_z,
                                                                                                      chan_details_x, chan_details_y,
                                                                                                      nodes_per_chan,
                                                                                                      switchpoint_rng,
                                                                                                      interdie_3d_links);

    // Check whether RR graph need to allocate new nodes for 3D connections.
    // To avoid wasting memory, the data structures are only allocated if we have more than one die in device grid.
    if (grid.get_num_layers() > 1) {
        // Allocate new nodes in each switchblocks
        alloc_and_load_inter_die_rr_node_indices(device_ctx.rr_graph_builder, interdie_3d_links, &num_rr_nodes);
        device_ctx.rr_graph_builder.resize_nodes(num_rr_nodes);
    }

    std::vector<std::pair<RRNodeId, int>> non_3d_sg_nodes = alloc_and_load_non_3d_sg_pattern_rr_node_indices(device_ctx.rr_graph_builder,
                                                                                                             bottleneck_links,
                                                                                                             nodes_per_chan,
                                                                                                             num_rr_nodes);
    device_ctx.rr_graph_builder.resize_nodes(num_rr_nodes);

    // START IPIN MAP
    // Create ipin map lookups

    t_pin_to_track_lookup ipin_to_track_map_x(types.size()); // [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width-1][0..height-1][0..layers-1][0..sides-1][0..Fc-1]
    t_pin_to_track_lookup ipin_to_track_map_y(types.size()); // [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width-1][0..height-1][0..layers-1][0..sides-1][0..Fc-1]

    t_track_to_pin_lookup track_to_pin_lookup_x(types.size());
    t_track_to_pin_lookup track_to_pin_lookup_y(types.size());

    for (size_t itype = 0; itype < types.size(); ++itype) {
        ipin_to_track_map_x[itype] = alloc_and_load_pin_to_track_map(e_pin_type::RECEIVER,
                                                                     Fc_in[itype], &types[itype],
                                                                     perturb_ipins[itype], directionality,
                                                                     segment_inf_x, sets_per_seg_type_x);

        ipin_to_track_map_y[itype] = alloc_and_load_pin_to_track_map(e_pin_type::RECEIVER,
                                                                     Fc_in[itype], &types[itype],
                                                                     perturb_ipins[itype], directionality,
                                                                     segment_inf_y, sets_per_seg_type_y);

        track_to_pin_lookup_x[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_x[itype], Fc_in[itype],
                                                                          types[itype].width,
                                                                          types[itype].height,
                                                                          types[itype].num_pins,
                                                                          nodes_per_chan.x_max, segment_inf_x);

        track_to_pin_lookup_y[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_y[itype], Fc_in[itype],
                                                                          types[itype].width,
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
    // END IPIN MAP

    // START OPIN MAP
    // Create opin map lookups
    t_pin_to_track_lookup opin_to_track_map(types.size()); // [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1]
    if (BI_DIRECTIONAL == directionality) {
        for (size_t itype = 0; itype < types.size(); ++itype) {
            std::vector<bool> perturb_opins = alloc_and_load_perturb_opins(&types[itype], Fc_out[itype],
                                                                           max_chan_width, segment_inf);
            opin_to_track_map[itype] = alloc_and_load_pin_to_track_map(e_pin_type::DRIVER,
                                                                       Fc_out[itype], &types[itype], perturb_opins, directionality,
                                                                       segment_inf, sets_per_seg_type);
        }
    }
    // END OPIN MAP

    bool Fc_clipped = false;
    // Draft the switches as internal data of RRGraph object
    // These are temporary switches copied from arch switches
    // We use them to build the edges
    // We will reset all the switches in the function
    //   alloc_and_load_rr_switch_inf()
    device_ctx.rr_graph_builder.reserve_switches(device_ctx.all_sw_inf.size());
    // Create the switches
    for (const t_arch_switch_inf& arch_sw : device_ctx.all_sw_inf | std::views::values) {
        t_rr_switch_inf rr_switch = create_rr_switch_from_arch_switch(arch_sw,
                                                                      R_minW_nmos,
                                                                      R_minW_pmos);
        device_ctx.rr_graph_builder.add_rr_switch(rr_switch);
    }

    auto update_chan_width = alloc_and_load_rr_graph(
        device_ctx.rr_graph_builder,
        device_ctx.rr_graph_builder.rr_nodes(), device_ctx.rr_graph, segment_inf.size(),
        segment_inf_x.size(), segment_inf_y.size(),
        segment_index_map,
        chan_details_x, chan_details_y,
        track_to_pin_lookup_x, track_to_pin_lookup_y,
        opin_to_track_map,
        interdie_3d_links,
        bottleneck_links,
        non_3d_sg_nodes,
        switch_block_conn, sb_conn_map, grid, Fs, unidir_sb_pattern,
        Fc_out,
        nodes_per_chan,
        wire_to_arch_ipin_switch,
        delayless_switch,
        directionality,
        &Fc_clipped,
        directs,
        clb_to_clb_directs,
        is_global_graph,
        clock_modeling,
        route_verbosity);

    // Verify no incremental node allocation.
    // AA: Note that in the case of dedicated networks, we are currently underestimating the additional node count due to the clock networks.
    // For now, the node count comparison is being skipped in the presence of clock networks.
    // TODO: The node estimation needs to be fixed for dedicated clock networks.
    if (rr_graph.num_nodes() > expected_node_count && clock_modeling != DEDICATED_NETWORK) {
        VTR_LOG_ERROR("Expected no more than %zu nodes, have %zu nodes\n",
                      expected_node_count, rr_graph.num_nodes());
    }

    // Update rr_nodes capacities if global routing
    if (graph_type == e_graph_type::GLOBAL) {
        // Using num_rr_nodes here over device_ctx.rr_nodes.size() because
        // clock_modeling::DEDICATED_NETWORK will append some rr nodes after
        // the regular graph.
        for (int i = 0; i < num_rr_nodes; i++) {
            if (rr_graph.node_type(RRNodeId(i)) == e_rr_type::CHANX) {
                int ylow = rr_graph.node_ylow(RRNodeId(i));
                device_ctx.rr_graph_builder.set_node_capacity(RRNodeId(i), nodes_per_chan.x_list[ylow]);
            }
            if (rr_graph.node_type(RRNodeId(i)) == e_rr_type::CHANY) {
                int xlow = rr_graph.node_xlow(RRNodeId(i));
                device_ctx.rr_graph_builder.set_node_capacity(RRNodeId(i), nodes_per_chan.y_list[xlow]);
            }
        }
    }

    update_chan_width(&nodes_per_chan);

    // Allocate and load routing resource switches, which are derived from the switches from the architecture file,
    // based on their fanin in the rr graph. This routine also adjusts the rr nodes to point to these new rr switches
    alloc_and_load_rr_switch_inf(g_vpr_ctx.mutable_device().rr_graph_builder,
                                 g_vpr_ctx.mutable_device().switch_fanin_remap,
                                 device_ctx.all_sw_inf,
                                 R_minW_nmos,
                                 R_minW_pmos,
                                 wire_to_arch_ipin_switch,
                                 wire_to_rr_ipin_switch);

    // Partition the rr graph edges for efficient access to configurable/non-configurable
    // edge subsets. Must be done after RR switches have been allocated
    device_ctx.rr_graph_builder.partition_edges();

    // Save the channel widths for the newly constructed graph
    device_ctx.chan_width = nodes_per_chan;

    rr_graph_externals(segment_inf, segment_inf_x, segment_inf_y, segment_inf_z, wire_to_rr_ipin_switch, base_cost_type);

    const VibDeviceGrid vib_grid;
    check_rr_graph(device_ctx.rr_graph,
                   types,
                   device_ctx.rr_indexed_data,
                   grid,
                   vib_grid,
                   device_ctx.chan_width,
                   graph_type,
                   is_flat);

    if (sb_conn_map) {
        free_switchblock_permutations(sb_conn_map);
        sb_conn_map = nullptr;
    }

    // We are done with building the RR Graph. Thus, we can clear the storages only used
    // to build the RR Graph
    device_ctx.rr_graph_builder.clear_temp_storage();
}

static int get_delayless_switch_id(const t_det_routing_arch& det_routing_arch,
                                   bool load_rr_graph) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    int delayless_switch = UNDEFINED;
    if (load_rr_graph) {
        const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches = device_ctx.rr_graph.rr_switch();
        for (size_t switch_id = 0; switch_id < rr_switches.size(); switch_id++) {
            const t_rr_switch_inf& rr_switch = rr_switches[RRSwitchId(switch_id)];
            if (rr_switch.name.find("delayless") != std::string::npos) {
                delayless_switch = static_cast<int>(switch_id);
                break;
            }
        }
    } else {
        delayless_switch = static_cast<int>(det_routing_arch.delayless_switch);
    }

    return delayless_switch;
}

static void alloc_and_init_channel_width() {
    DeviceContext& mutable_device_ctx = g_vpr_ctx.mutable_device();
    const DeviceGrid& grid = mutable_device_ctx.grid;
    const auto& rr_graph = mutable_device_ctx.rr_graph;

    vtr::NdMatrix<int, 3>& chanx_width = mutable_device_ctx.rr_chanx_segment_width;
    vtr::NdMatrix<int, 3>& chany_width = mutable_device_ctx.rr_chany_segment_width;
    vtr::NdMatrix<int, 2>& chanz_width = mutable_device_ctx.rr_chanz_segment_width;

    chanx_width.resize({grid.get_num_layers(), grid.width(), grid.height()});
    chany_width.resize({grid.get_num_layers(), grid.width(), grid.height()});

    chanx_width.fill(0);
    chany_width.fill(0);

    if (grid.get_num_layers() > 1) {
        chanz_width.resize({grid.width(), grid.height()});
        chanz_width.fill(0);
    }

    for (RRNodeId node_id : rr_graph.nodes()) {
        e_rr_type rr_type = rr_graph.node_type(node_id);

        if (rr_type == e_rr_type::CHANX) {
            int y = rr_graph.node_ylow(node_id);
            int layer = rr_graph.node_layer_low(node_id);
            for (int x = rr_graph.node_xlow(node_id); x <= rr_graph.node_xhigh(node_id); x++) {
                chanx_width[layer][x][y] += rr_graph.node_capacity(node_id);
            }
        } else if (rr_type == e_rr_type::CHANY) {
            int x = rr_graph.node_xlow(node_id);
            int layer = rr_graph.node_layer_low(node_id);
            for (int y = rr_graph.node_ylow(node_id); y <= rr_graph.node_yhigh(node_id); y++) {
                chany_width[layer][x][y] += rr_graph.node_capacity(node_id);
            }
        } else if (rr_type == e_rr_type::CHANZ) {
            int x = rr_graph.node_xlow(node_id);
            int y = rr_graph.node_ylow(node_id);
            chanz_width[x][y]++;
        }
    }

    std::vector<int>& chanx_width_list = mutable_device_ctx.rr_chanx_width;
    std::vector<int>& chany_width_list = mutable_device_ctx.rr_chany_width;

    chanx_width_list.resize(grid.height());
    chany_width_list.resize(grid.width());

    std::ranges::fill(chanx_width_list, 0);
    std::ranges::fill(chany_width_list, 0);

    for (t_physical_tile_loc loc : grid.all_locations()) {
        chanx_width_list[loc.y] = std::max(chanx_width[loc.layer_num][loc.x][loc.y], chanx_width_list[loc.y]);
        chany_width_list[loc.x] = std::max(chany_width[loc.layer_num][loc.x][loc.y], chany_width_list[loc.x]);
    }
}

void build_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                         const t_det_routing_arch& det_routing_arch,
                         t_physical_tile_type_ptr physical_tile,
                         const t_physical_tile_loc& tile_loc,
                         const int delayless_switch) {
    std::map<int, t_arch_switch_inf> sw_map = g_vpr_ctx.device().all_sw_inf;

    int num_rr_nodes = 0;
    alloc_and_load_tile_rr_node_indices(rr_graph_builder,
                                        physical_tile,
                                        tile_loc,
                                        &num_rr_nodes);
    rr_graph_builder.resize_nodes(num_rr_nodes);

    alloc_and_load_tile_rr_graph(rr_graph_builder,
                                 sw_map,
                                 physical_tile,
                                 tile_loc,
                                 delayless_switch);

    t_arch_switch_fanin switch_fanin_remap;
    RRSwitchId dummy_sw_id;
    alloc_and_load_rr_switch_inf(rr_graph_builder,
                                 switch_fanin_remap,
                                 sw_map,
                                 det_routing_arch.R_minW_nmos,
                                 det_routing_arch.R_minW_pmos,
                                 det_routing_arch.wire_to_arch_ipin_switch,
                                 dummy_sw_id);
    rr_graph_builder.partition_edges();
}

void rr_graph_externals(const std::vector<t_segment_inf>& segment_inf,
                        const std::vector<t_segment_inf>& segment_inf_x,
                        const std::vector<t_segment_inf>& segment_inf_y,
                        const std::vector<t_segment_inf>& segment_inf_z,
                        RRSwitchId wire_to_rr_ipin_switch,
                        e_base_cost_type base_cost_type) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const DeviceGrid& grid = device_ctx.grid;
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data = mutable_device_ctx.rr_indexed_data;
    bool echo_enabled = getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA);
    const char* echo_file_name = getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA);
    add_rr_graph_C_from_switches(rr_graph.rr_switch_inf(wire_to_rr_ipin_switch).Cin);
    alloc_and_load_rr_indexed_data(rr_graph, grid, segment_inf, segment_inf_x, segment_inf_y, segment_inf_z,
                                   rr_indexed_data, wire_to_rr_ipin_switch, base_cost_type, echo_enabled, echo_file_name);
    //load_rr_index_segments(segment_inf.size());
}

static std::vector<std::vector<bool>> alloc_and_load_perturb_ipins(const int L_num_types,
                                                                   const int num_seg_types,
                                                                   const std::vector<int>& sets_per_seg_type,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_in,
                                                                   const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                   const e_directionality directionality) {
    std::vector<std::vector<bool>> result(L_num_types);
    for (std::vector<bool>& seg_type_bools : result) {
        seg_type_bools.resize(num_seg_types, false);
    }

    // factor to account for unidir vs bidir
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
                    && (fabs(Fc_ratio - vtr::nint(Fc_ratio)) < (0.5 / (float)tracks_in_seg_type))) {
                    result[itype][iseg] = true;
                }
            }
        }
    } else {
        // Unidirectional routing uses mux balancing patterns and thus shouldn't need perturbation.
        VTR_ASSERT(UNI_DIRECTIONAL == directionality);
        for (int itype = 0; itype < L_num_types; ++itype) {
            for (int iseg = 0; iseg < num_seg_types; ++iseg) {
                result[itype][iseg] = false;
            }
        }
    }
    return result;
}

static std::vector<t_seg_details> alloc_and_load_global_route_seg_details(const int global_route_switch) {
    std::vector<t_seg_details> seg_details(1);

    seg_details[0].index = 0;
    seg_details[0].abs_index = 0;
    seg_details[0].length = 1;
    seg_details[0].arch_wire_switch = global_route_switch;
    seg_details[0].arch_opin_switch = global_route_switch;
    seg_details[0].longline = false;
    seg_details[0].direction = Direction::BIDIR;
    seg_details[0].Cmetal = 0.0;
    seg_details[0].Rmetal = 0.0;
    seg_details[0].start = 1;
    seg_details[0].cb = std::make_unique<bool[]>(1);
    seg_details[0].cb[0] = true;
    seg_details[0].sb = std::make_unique<bool[]>(2);
    seg_details[0].sb[0] = true;
    seg_details[0].sb[1] = true;
    seg_details[0].group_size = 1;
    seg_details[0].group_start = 0;

    return seg_details;
}

/* Calculates the number of track connections from each block pin to each segment type */
std::vector<vtr::Matrix<int>> alloc_and_load_actual_fc(const std::vector<t_physical_tile_type>& types,
                                                       const int max_pins,
                                                       const std::vector<t_segment_inf>& segment_inf,
                                                       const std::vector<int>& sets_per_seg_type,
                                                       const e_fc_type fc_type,
                                                       const e_directionality directionality,
                                                       bool* Fc_clipped,
                                                       bool is_flat) {
    // Initialize Fc of all blocks to zero
    auto zeros = vtr::Matrix<int>({size_t(max_pins), segment_inf.size()}, 0);
    std::vector<vtr::Matrix<int>> Fc(types.size(), zeros);

    *Fc_clipped = false;

    // VTR_ASSERT((nodes_per_chan->x_max % fac) == 0 && (nodes_per_chan->y_max % fac) == 0);

    for (const t_physical_tile_type& type : types) { // Skip EMPTY
        int itype = type.index;

        for (const t_fc_specification& fc_spec : type.fc_specs) {
            if (fc_type != fc_spec.fc_type) continue;

            VTR_ASSERT(!fc_spec.pins.empty());

            int iseg = fc_spec.seg_index;

            if (fc_spec.fc_value == 0) {
                // Special case indicating that this pin does not connect to general-purpose routing
                for (int ipin : fc_spec.pins) {
                    Fc[itype][ipin][iseg] = 0;
                }
                continue;
            }

            // General case indicating that this pin connects to general-purpose routing

            // Unidir tracks formed in pairs, otherwise no effect.
            int fac = 1;
            if (directionality == UNI_DIRECTIONAL && segment_inf[iseg].parallel_axis != e_parallel_axis::Z_AXIS) {
                fac = 2;
            }

            // Calculate how many connections there should be across all the pins in this fc_spec
            int total_connections = 0;
            if (fc_spec.fc_value_type == e_fc_value_type::FRACTIONAL) {
                float conns_per_pin = fac * sets_per_seg_type[iseg] * fc_spec.fc_value;
                float flt_total_connections = conns_per_pin * fc_spec.pins.size();
                total_connections = vtr::nint(flt_total_connections); //Round to integer
            } else {
                VTR_ASSERT(fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE);

                if (std::fmod(fc_spec.fc_value, fac) != 0. && segment_inf[iseg].parallel_axis != e_parallel_axis::Z_AXIS) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Absolute Fc value must be a multiple of %d (was %f) between block pin '%s' and wire segment '%s'",
                                    fac, fc_spec.fc_value,
                                    block_type_pin_index_to_name(&type, fc_spec.pins[0], is_flat).c_str(),
                                    segment_inf[iseg].name.c_str());
                }

                if (fc_spec.fc_value < fac && segment_inf[iseg].parallel_axis != e_parallel_axis::Z_AXIS) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Absolute Fc value must be at least %d (was %f) between block pin '%s' to wire segment %s",
                                    fac, fc_spec.fc_value,
                                    block_type_pin_index_to_name(&type, fc_spec.pins[0], is_flat).c_str(),
                                    segment_inf[iseg].name.c_str());
                }

                total_connections = vtr::nint(fc_spec.fc_value) * fc_spec.pins.size();
            }

            // Ensure that there are at least fac connections, this ensures that low Fc ports
            // targeting small sets of segs get connection(s), even if flt_total_connections < fac.
            total_connections = std::max(total_connections, fac);

            // Ensure total evenly divides fac by adding the remainder
            total_connections += (total_connections % fac);

            VTR_ASSERT(total_connections > 0);
            VTR_ASSERT(total_connections % fac == 0);

            // We walk through all the pins this fc_spec applies to, adding fac connections
            // to each pin, until we run out of connections. This should distribute the connections
            // as evenly as possible (if total_connections % pins.size() != 0, there will be
            // some inevitable imbalance).
            int connections_remaining = total_connections;
            while (connections_remaining != 0) {
                // Add one set of connections to each pin
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

            if (segment_inf[iseg].parallel_axis != e_parallel_axis::Z_AXIS) {
                for (int ipin : fc_spec.pins) {
                    // It is possible that we may want more connections that wires of this type exist;
                    // clip to the maximum number of wires
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
                                                                  const size_t num_seg_types,
                                                                  const size_t num_seg_types_x,
                                                                  const size_t num_seg_types_y,
                                                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                                                  const t_chan_details& chan_details_x,
                                                                  const t_chan_details& chan_details_y,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_x,
                                                                  const t_track_to_pin_lookup& track_to_pin_lookup_y,
                                                                  const t_pin_to_track_lookup& opin_to_track_map,
                                                                  const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                                                                  const std::vector<t_bottleneck_link>& sg_links,
                                                                  const std::vector<std::pair<RRNodeId, int>>& sg_node_indices,
                                                                  const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                                                                  t_sb_connection_map* sb_conn_map,
                                                                  const DeviceGrid& grid,
                                                                  const int Fs,
                                                                  t_sblock_pattern& sblock_pattern,
                                                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                                                  const t_chan_width& chan_width,
                                                                  const int wire_to_ipin_switch,
                                                                  const int delayless_switch,
                                                                  const e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const std::vector<t_direct_inf>& directs,
                                                                  const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const e_clock_modeling clock_modeling,
                                                                  const int route_verbosity) {
    // We take special care when creating RR graph edges (there are typically many more
    // edges than nodes in an RR graph).
    //
    // In particular, all the following build_*() functions do not create the edges, but
    // instead record the edges they wish to create in rr_edges_to_create.
    //
    // We uniquify the edges to be created (avoiding any duplicates), and create
    // the edges in alloc_and_load_edges().
    //
    // By doing things in this manner we ensure we know exactly how many edges leave each RR
    // node, which avoids resizing the RR edge arrays (which can cause significant memory
    // fragmentation, and significantly increasing peak memory usage). This is important since
    // RR graph creation is the high-watermark of VPR's memory use.
    t_rr_edge_info_set rr_edges_to_create;

    // If Fc gets clipped, this will be flagged to true
    *Fc_clipped = false;

    // This function is called to build the general routing graph resources. Thus,
    // the edges are not remapped yet.
    bool switches_remapped = false;

    int num_edges = 0;
    // Connection SINKS and SOURCES to their pins - Initializing IPINs/OPINs.

    for (const t_physical_tile_loc& tile_loc : grid.all_locations()) {
        if (grid.is_root_location(tile_loc)) {
            t_physical_tile_type_ptr physical_tile = grid.get_physical_type(tile_loc);
            std::vector<int> class_num_vec = get_tile_root_classes(physical_tile);
            std::vector<int> pin_num_vec = get_tile_root_pins(physical_tile);

            add_classes_rr_graph(rr_graph_builder,
                                 class_num_vec,
                                 tile_loc,
                                 physical_tile);

            add_pins_rr_graph(rr_graph_builder,
                              pin_num_vec,
                              tile_loc,
                              physical_tile);

            connect_src_sink_to_pins(rr_graph_builder,
                                     class_num_vec,
                                     tile_loc,
                                     rr_edges_to_create,
                                     delayless_switch,
                                     physical_tile,
                                     switches_remapped);

            // Create the actual SOURCE->OPIN, IPIN->SINK edges
            uniquify_edges(rr_edges_to_create);
            alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
            num_edges += rr_edges_to_create.size();
            rr_edges_to_create.clear();
        }
    }

    VTR_LOGV(route_verbosity > 1, "SOURCE->OPIN and IPIN->SINK edge count:%d\n", num_edges);

    num_edges = 0;
    int rr_edges_before_directs = 0;
    add_opin_chan_edges(rr_graph_builder, rr_graph, num_seg_types, num_seg_types_x, num_seg_types_y,
                        chan_width, chan_details_x, chan_details_y,
                        seg_index_map, opin_to_track_map, interdie_3d_links,
                        Fc_out, directs, clb_to_clb_directs, directionality,
                        num_edges, rr_edges_before_directs, Fc_clipped);

    VTR_LOGV(route_verbosity > 1, "OPIN->CHANX/CHANY edge count before creating direct connections: %d\n", rr_edges_before_directs);
    VTR_LOGV(route_verbosity > 1, "OPIN->CHANX/CHANY edge count after creating direct connections: %d\n", num_edges);

    num_edges = 0;
    add_chan_chan_edges(rr_graph_builder,
                        num_seg_types_x, num_seg_types_y,
                        track_to_pin_lookup_x, track_to_pin_lookup_y,
                        chan_width, chan_details_x, chan_details_y,
                        sb_conn_map, switch_block_conn, interdie_3d_links, sblock_pattern,
                        Fs, wire_to_ipin_switch, directionality, is_global_graph, num_edges);

    VTR_LOGV(route_verbosity > 1, "CHAN->CHAN type edge count:%d\n", num_edges);

    add_and_connect_non_3d_sg_links(rr_graph_builder,
                                    sg_links, sg_node_indices,
                                    chan_details_x, chan_details_y,
                                    num_seg_types_x, num_edges);
    VTR_LOGV(route_verbosity > 1, "Non-3D scatter-gather edge count:%d\n", num_edges);

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

static void connect_tile_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                                          std::map<int, t_arch_switch_inf>& /*arch_sw_inf_map*/,
                                          const std::vector<int>& class_num_vec,
                                          const t_physical_tile_loc& root_loc,
                                          t_rr_edge_info_set& rr_edges_to_create,
                                          const int delayless_switch,
                                          t_physical_tile_type_ptr physical_type_ptr) {
    for (int class_num : class_num_vec) {
        const std::vector<int>& pin_list = get_pin_list_from_class_physical_num(physical_type_ptr, class_num);
        e_pin_type class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        RRNodeId class_rr_node_id = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, root_loc, class_num);
        VTR_ASSERT(class_rr_node_id != RRNodeId::INVALID());
        for (int pin_num : pin_list) {
            RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, root_loc, pin_num);
            if (pin_rr_node_id == RRNodeId::INVALID()) {
                VTR_LOG_ERROR("In block (%d, %d, %d) pin num: %d doesn't exist to be connected to class %d\n",
                              root_loc.layer_num,
                              root_loc.x,
                              root_loc.y,
                              pin_num,
                              class_num);
                continue;
            }
            e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
            if (class_type == e_pin_type::DRIVER) {
                VTR_ASSERT(pin_type == e_pin_type::DRIVER);
                rr_edges_to_create.emplace_back(class_rr_node_id, pin_rr_node_id, delayless_switch, false);
            } else {
                VTR_ASSERT(class_type == e_pin_type::RECEIVER);
                VTR_ASSERT(pin_type == e_pin_type::RECEIVER);
                rr_edges_to_create.emplace_back(pin_rr_node_id, class_rr_node_id, delayless_switch, false);
            }
        }
    }
}

static void alloc_and_load_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                                         std::map<int, t_arch_switch_inf>& arch_sw_inf_map,
                                         t_physical_tile_type_ptr physical_tile,
                                         const t_physical_tile_loc& root_loc,
                                         const int delayless_switch) {
    t_rr_edge_info_set rr_edges_to_create;

    t_class_range class_num_range = get_flat_tile_primitive_classes(physical_tile);
    const std::vector<int> pin_num_vec = get_flat_tile_pins(physical_tile);

    std::vector<int> class_num_vec(class_num_range.total_num());
    std::iota(class_num_vec.begin(), class_num_vec.end(), class_num_range.low);

    add_classes_rr_graph(rr_graph_builder,
                         class_num_vec,
                         root_loc,
                         physical_tile);

    add_pins_rr_graph(rr_graph_builder,
                      pin_num_vec,
                      root_loc,
                      physical_tile);

    connect_tile_src_sink_to_pins(rr_graph_builder,
                                  arch_sw_inf_map,
                                  class_num_vec,
                                  root_loc,
                                  rr_edges_to_create,
                                  delayless_switch,
                                  physical_tile);

    uniquify_edges(rr_edges_to_create);
    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
    rr_edges_to_create.clear();

    add_intra_tile_edges_rr_graph(rr_graph_builder,
                                  rr_edges_to_create,
                                  physical_tile,
                                  root_loc);

    uniquify_edges(rr_edges_to_create);
    alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
    rr_edges_to_create.clear();

    rr_graph_builder.init_fan_in();

    rr_graph_builder.rr_nodes().shrink_to_fit();
}

void free_rr_graph() {
    // Frees all the routing graph data structures, if they have been allocated.
    // I use rr_mem_chunk_list_head as a flag to indicate whether or not the graph has been allocated -- if it is not NULL,
    // a routing graph exists and can be freed.  Hence, you can call this routine even if you're not sure of whether a rr_graph exists or not.

    // Before adding any more free calls here, be sure the data is NOT chunk allocated, as ALL the chunk allocated data is already free!
    DeviceContext& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.loaded_rr_graph_filename.clear();
    device_ctx.loaded_rr_edge_override_filename.clear();

    device_ctx.rr_graph_builder.clear();

    device_ctx.rr_node_track_ids.clear();

    device_ctx.rr_indexed_data.clear();

    device_ctx.switch_fanin_remap.clear();

    device_ctx.rr_graph_is_flat = false;

    invalidate_router_lookahead_cache();
}

void alloc_and_load_edges(RRGraphBuilder& rr_graph_builder, const t_rr_edge_info_set& rr_edges_to_create) {
    rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
}

/* allocate pin to track map for each segment type individually and then combine into a single
 * vector */
static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr tile_type,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const std::vector<int>& sets_per_seg_type) {
    // allocate 'result' matrix and initialize entries to UNDEFINED. also allocate and initialize matrix which will be used
    // to index into the correct entries when loading up 'result'
    auto result = vtr::NdMatrix<std::vector<int>, 4>({
        size_t(tile_type->num_pins), //[0..num_pins-1]
        size_t(tile_type->width),    //[0..width-1]
        size_t(tile_type->height),   //[0..height-1]
        4,                           //[0..sides-1]
    });

    // multiplier for unidirectional vs bidirectional architectures
    int fac = 1;
    if (directionality == UNI_DIRECTIONAL) {
        fac = 2;
    }

    // load the pin to track matrix by looking at each segment type in turn
    int num_parallel_seg_types = seg_inf.size();
    int seg_type_start_track = 0;
    for (int iseg = 0; iseg < num_parallel_seg_types; iseg++) {
        int num_seg_type_tracks = fac * sets_per_seg_type[iseg];

        // determine the maximum Fc to this segment type across all pins
        int max_Fc = 0;
        for (int pin_index = 0; pin_index < tile_type->num_pins; ++pin_index) {
            int pin_class = tile_type->pin_class[pin_index];
            if (Fc[pin_index][seg_inf[iseg].seg_index] > max_Fc && tile_type->class_inf[pin_class].type == pin_type) {
                max_Fc = Fc[pin_index][seg_inf[iseg].seg_index];
            }
        }

        // get pin connections to tracks of the current segment type
        auto pin_to_seg_type_map = alloc_and_load_pin_to_seg_type(pin_type, Fc, num_seg_type_tracks, seg_inf[iseg].seg_index, max_Fc, tile_type, perturb_switch_pattern[seg_inf[iseg].seg_index], directionality);

        // connections in pin_to_seg_type_map are within that seg type -- i.e. in the [0,num_seg_type_tracks-1] range.
        // now load up 'result' array with these connections, but offset them so they are relative to the channel as a whole
        for (int ipin = 0; ipin < tile_type->num_pins; ipin++) {
            int cur_Fc = Fc[ipin][seg_inf[iseg].seg_index];
            for (int iwidth = 0; iwidth < tile_type->width; iwidth++) {
                for (int iheight = 0; iheight < tile_type->height; iheight++) {
                    for (int iside = 0; iside < 4; iside++) {
                        for (int iconn = 0; iconn < cur_Fc; iconn++) {
                            int relative_track_ind = pin_to_seg_type_map[ipin][iwidth][iheight][iside][iconn];
                            if (relative_track_ind != UNDEFINED) {
                                VTR_ASSERT(relative_track_ind <= num_seg_type_tracks);
                                int absolute_track_ind = relative_track_ind + seg_type_start_track;

                                VTR_ASSERT(absolute_track_ind >= 0);
                                result[ipin][iwidth][iheight][iside].push_back(absolute_track_ind);
                            }
                        }
                    }
                }
            }
        }

        // next seg type will start at this track index
        seg_type_start_track += num_seg_type_tracks;
    }

    return result;
}

static vtr::NdMatrix<int, 5> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const vtr::Matrix<int>& Fc,
                                                            const int num_seg_type_tracks,
                                                            const int seg_index,
                                                            const int max_Fc,
                                                            const t_physical_tile_type_ptr tile_type,
                                                            const bool perturb_switch_pattern,
                                                            const e_directionality directionality) {
    // Note: currently a single value of Fc is used across each pin. In the future the looping below will
    // have to be modified if we want to account for pin-based Fc values

    // NB:  This wastes some space.  Could set tracks_..._pin[ipin][ioff][iside] = NULL if there is
    // no pin on that side, or that pin is of the wrong type.
    // Probably not enough memory to worry about, esp. as it's temporary.
    // If pin ipin on side iside does not exist or is of the wrong type,
    // tracks_connected_to_pin[ipin][iside][0] = UNDEFINED.

    if (tile_type->num_pins < 1) {
        return vtr::NdMatrix<int, 5>();
    }

    auto tracks_connected_to_pin = vtr::NdMatrix<int, 5>({
                                                             size_t(tile_type->num_pins), // [0..num_pins-1]
                                                             size_t(tile_type->width),    // [0..width-1]
                                                             size_t(tile_type->height),   // [0..height-1]
                                                             NUM_2D_SIDES,                // [0..NUM_2D_SIDES-1]
                                                             size_t(max_Fc)               // [0..Fc-1]
                                                         },
                                                         UNDEFINED); // Unconnected

    // Number of *physical* pins on each side.
    // Note that his may be more than the logical number of pins (i.e.
    // Type->num_pins) if a logical pin has multiple specified physical
    // pinlocations (i.e. appears on multiple sides of the block)
    auto num_dir = vtr::NdMatrix<int, 3>({
                                             size_t(tile_type->width),  // [0..width-1]
                                             size_t(tile_type->height), // [0..height-1]
                                             NUM_2D_SIDES               // [0..NUM_2D_SIDES-1]
                                         },
                                         0);

    // List of *physical* pins of the correct type on each side of the current
    // block type. For a specific width/height/side the valid entries in the
    // last dimension are [0 .. num_dir[width][height][side]-1]
    //
    // Max possible space allocated for simplicity
    auto dir_list = vtr::NdMatrix<int, 4>({
                                              size_t(tile_type->width),   // [0..width-1]
                                              size_t(tile_type->height),  // [0..height-1]
                                              NUM_2D_SIDES,               // [0..NUM_2D_SIDES-1]
                                              size_t(tile_type->num_pins) // [0..num_pins * num_layers-1]
                                          },
                                          -1); // Defensive coding: Initialize to invalid

    // Number of currently assigned physical pins
    auto num_done_per_dir = vtr::NdMatrix<int, 3>({
                                                      size_t(tile_type->width),  // [0..width-1]
                                                      size_t(tile_type->height), // [0..height-1]
                                                      NUM_2D_SIDES               // [0..NUM_2D_SIDES-1]
                                                  },
                                                  0);

    // Record the physical pin locations and counts per side/offsets combination
    for (int pin = 0; pin < tile_type->num_pins; ++pin) {
        e_pin_type curr_pin_type = get_pin_type_from_pin_physical_num(tile_type, pin);
        if (curr_pin_type != pin_type) // Doing either ipins OR opins
            continue;

        // Pins connecting only to global resources get no switches -> keeps area model accurate.
        if (tile_type->is_ignored_pin[pin])
            continue;

        for (int width = 0; width < tile_type->width; ++width) {
            for (int height = 0; height < tile_type->height; ++height) {
                for (e_side side : TOTAL_2D_SIDES) {
                    if (tile_type->pinloc[width][height][side][pin] == 1) {
                        dir_list[width][height][side][num_dir[width][height][side]] = pin;
                        num_dir[width][height][side]++;
                    }
                }
            }
        }
    }

    // Total the number of physical pin
    int num_phys_pins = 0;
    for (int width = 0; width < tile_type->width; ++width) {
        for (int height = 0; height < tile_type->height; ++height) {
            for (e_side side : TOTAL_2D_SIDES) {
                num_phys_pins += num_dir[width][height][side]; // Num. physical pins per type
            }
        }
    }

    std::vector<t_pin_loc> pin_ordering;

    // Connection block I use distributes pins evenly across the tracks of ALL sides of the clb at once.
    // Ensures that each pin connects to spaced out tracks in its connection block, and that the other pins
    // (potentially in other C blocks) connect to the remaining tracks first. Doesn't matter for large Fc,
    // but should make a fairly good low Fc block that leverages the fact that usually lots of pins are logically equivalent.

    e_side side = LEFT;
    int width = 0;
    int height = 0;
    int pin = 0;
    int pin_index = -1;

    // Determine the order in which physical pins will be considered while building
    // the connection block. This generally tries to order the pins so they are 'spread'
    // out (in hopes of yielding good connection diversity)
    while (pin < num_phys_pins) {
        if (height == 0 && width == 0 && side == LEFT) {
            // Completed one loop through all the possible offsets/side combinations
            pin_index++;
        }

        advance_to_next_block_side(tile_type, width, height, side);

        VTR_ASSERT_MSG(pin_index < num_phys_pins, "Physical block pins bound number of logical block pins");

        if (num_done_per_dir[width][height][side] >= num_dir[width][height][side]) {
            continue;
        }

        int pin_num = dir_list[width][height][side][pin_index];
        VTR_ASSERT(pin_num >= 0);
        VTR_ASSERT(tile_type->pinloc[width][height][side][pin_num]);

        t_pin_loc pin_loc;
        pin_loc.pin_index = pin_num;
        pin_loc.width_offset = width;
        pin_loc.height_offset = height;
        pin_loc.side = side;

        pin_ordering.push_back(pin_loc);

        num_done_per_dir[width][height][side]++;
        pin++;
    }

    VTR_ASSERT(pin == num_phys_pins);

    if (perturb_switch_pattern) {
        load_perturbed_connection_block_pattern(tracks_connected_to_pin,
                                                pin_ordering,
                                                num_seg_type_tracks, num_seg_type_tracks, Fc, seg_index, directionality);
    } else {
        load_uniform_connection_block_pattern(tracks_connected_to_pin,
                                              pin_ordering, Fc, seg_index,
                                              num_seg_type_tracks, num_seg_type_tracks, directionality);
    }

#ifdef ENABLE_CHECK_ALL_TRACKS
    check_all_tracks_reach_pins(tile_type, tracks_connected_to_pin, num_seg_type_tracks,
                                Fc, pin_type);
#endif

    return tracks_connected_to_pin;
}

static void advance_to_next_block_side(t_physical_tile_type_ptr tile_type, int& width_offset, int& height_offset, e_side& side) {
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
    VTR_ASSERT(width_offset >= 0 && width_offset < tile_type->width);
    VTR_ASSERT(height_offset >= 0 && height_offset < tile_type->height);
    VTR_ASSERT(side == LEFT || side == RIGHT || side == BOTTOM || side == TOP);

    if (side == LEFT) {
        if (height_offset == tile_type->height - 1 && width_offset == tile_type->width - 1) {
            //Finished the last left edge column
            side = TOP; //Turn clockwise
            width_offset = 0;
            height_offset = tile_type->height - 1;
        } else if (height_offset == tile_type->height - 1) {
            //Finished the current left edge column
            VTR_ASSERT(width_offset != tile_type->width - 1);

            //Move right to the next left edge column
            width_offset++;
            height_offset = 0;
        } else {
            height_offset++; //Advance up current left edge column
        }
    } else if (side == TOP) {
        if (height_offset == 0 && width_offset == tile_type->width - 1) {
            //Finished the last top edge row
            side = RIGHT; //Turn clockwise
            width_offset = tile_type->width - 1;
            height_offset = tile_type->height - 1;
        } else if (width_offset == tile_type->width - 1) {
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
            width_offset = tile_type->width - 1;
            height_offset = 0;
        } else if (height_offset == 0) {
            //Finished the current right edge column
            VTR_ASSERT(width_offset != 0);

            //Move left to the next right edge column
            width_offset--;
            height_offset = tile_type->height - 1;
        } else {
            height_offset--; //Advance down current right edge column
        }
    } else {
        VTR_ASSERT(side == BOTTOM);
        if (height_offset == tile_type->height - 1 && width_offset == 0) {
            //Finished the last bottom edge row
            side = LEFT; //Turn clockwise
            width_offset = 0;
            height_offset = 0;
        } else if (width_offset == 0) {
            //Finished the current bottom edge row
            VTR_ASSERT(height_offset != tile_type->height - 1);

            //Move up to the next bottom edge row
            height_offset++;
            width_offset = tile_type->width - 1;
        } else {
            width_offset--; //Advance left along current bottom edge row
        }
    }

    //Validate next state
    VTR_ASSERT(width_offset >= 0 && width_offset < tile_type->width);
    VTR_ASSERT(height_offset >= 0 && height_offset < tile_type->height);
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

static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
                                                  const std::vector<t_pin_loc>& pin_locations,
                                                  const vtr::Matrix<int>& Fc,
                                                  const int seg_index,
                                                  const int x_chan_width,
                                                  const int y_chan_width,
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
    excess_tracks_selected.resize(NUM_2D_SIDES);

    for (int i = 0; i < num_phys_pins; ++i) {
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;

        max_width = std::max(max_width, width);
        max_height = std::max(max_height, height);
    }

    for (int iside = 0; iside < NUM_2D_SIDES; iside++) {
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

    VTR_ASSERT((x_chan_width % group_size == 0) && (y_chan_width % group_size == 0));

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
        int pin_fc = Fc[pin][seg_index];

        VTR_ASSERT(pin_fc % group_size == 0);

        // Bi-directional treats each track separately, uni-directional works with pairs of tracks
        for (int j = 0; j < (pin_fc / group_size); ++j) {
            int max_chan_width = (side == TOP || side == BOTTOM) ? x_chan_width : y_chan_width;

            // if the number of tracks we can assign is zero break from the loop
            if (max_chan_width == 0) {
                break;
            }
            float step_size = (float)max_chan_width / (float)(pin_fc * num_phys_pins);

            VTR_ASSERT(pin_fc > 0);
            float fc_step = (float)max_chan_width / (float)pin_fc;

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

                        for (int j2 = 0; j2 < (pin_fc / group_size); ++j2) {
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
                tracks_connected_to_pin[pin][width][height][side][group_size * j + k] = (itrack + k) % max_chan_width;
                excess_tracks_selected[side][width][height][(itrack + k) % max_chan_width]++;
            }
        }
    }
}

static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
                                                    const std::vector<t_pin_loc>& pin_locations,
                                                    const int x_chan_width,
                                                    const int y_chan_width,
                                                    const vtr::Matrix<int>& Fc,
                                                    const int seg_index,
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

    int num_phys_pins = pin_locations.size();

    for (int i = 0; i < num_phys_pins; ++i) {
        int pin = pin_locations[i].pin_index;
        e_side side = pin_locations[i].side;
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;

        int pin_Fc = Fc[pin][seg_index];
        int Fc_dense = (pin_Fc / 2) + 1;
        int Fc_sparse = pin_Fc - Fc_dense;
        int Fc_half[2];

        int max_chan_width = (((side == TOP) || (side == BOTTOM)) ? x_chan_width : y_chan_width);
        float step_size = (float)max_chan_width / (float)(pin_Fc * num_phys_pins);

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
                tracks_connected_to_pin[pin][width][height][side][iconn] = itrack;

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
                    if (tracks_connected_to_pin[pin][width][height][side][0] != UNDEFINED) { /* Pin exists */
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

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
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
        return vtr::NdMatrix<std::vector<int>, 4>();
    }

    const int num_seg_types = seg_inf.size();
    // Alloc and zero the lookup table
    auto track_to_pin_lookup = vtr::NdMatrix<std::vector<int>, 4>({size_t(max_chan_width), size_t(type_width), size_t(type_height), 4});

    // Count number of pins to which each track connects
    for (int pin = 0; pin < num_pins; ++pin) {
        for (int width = 0; width < type_width; ++width) {
            for (int height = 0; height < type_height; ++height) {
                for (int side = 0; side < 4; ++side) {
                    // get number of tracks to which this pin connects
                    int num_tracks = 0;
                    for (int iseg = 0; iseg < num_seg_types; iseg++) {
                        num_tracks += Fc[pin][seg_inf[iseg].seg_index]; // Fc_in and Fc_out matrices are unified for all segments so need to map index.
                    }
                    if (!pin_to_track_map[pin][width][height][side].empty()) {
                        num_tracks = std::min(num_tracks,
                                              (int)pin_to_track_map[pin][width][height][side].size());
                        for (int conn = 0; conn < num_tracks; ++conn) {
                            int track = pin_to_track_map[pin][width][height][side][conn];
                            VTR_ASSERT(track < max_chan_width);
                            VTR_ASSERT(track >= 0);
                            track_to_pin_lookup[track][width][height][side].push_back(pin);
                        }
                    }
                }
            }
        }
    }

    return track_to_pin_lookup;
}

static std::vector<bool> alloc_and_load_perturb_opins(const t_physical_tile_type_ptr type,
                                                      const vtr::Matrix<int>& Fc_out,
                                                      const int max_chan_width,
                                                      const std::vector<t_segment_inf>& segment_inf) {
    int num_wire_types;
    float step_size = 0;
    float n = 0;
    float threshold = 0.07;

    std::vector<bool> perturb_opins(segment_inf.size(), false);

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

    // get Fc_max
    int Fc_max = 0;
    for (int i = 0; i < type->num_pins; ++i) {
        auto pin_type = get_pin_type_from_pin_physical_num(type, i);
        if (Fc_out[i][0] > Fc_max && pin_type == e_pin_type::DRIVER) {
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

    // get an upper bound on the number of prime factors of num_wire_types
    int max_primes = (int)floor(log((float)num_wire_types) / log(2.0));
    max_primes = std::max(max_primes, 1); //Minimum of 1 to ensure we allocate space for at least one prime_factor

    int* prime_factors = new int[max_primes];
    for (int i = 0; i < max_primes; i++) {
        prime_factors[i] = 0;
    }

    // Find the prime factors of num_wire_types
    int num = num_wire_types;
    int factor = 2;
    int num_factors = 0;
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
    for (int i = 0; i < num_factors; i++) {
        if (vtr::nint(step_size) < prime_factors[i]) {
            perturb_opins[0] = false;
            break;
        }

        n = step_size / prime_factors[i];
        n = n - (float)vtr::nint(n); /* fractional part */
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

static void process_non_config_sets() {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    EdgeGroups groups;
    create_edge_groups(&groups);
    groups.set_device_context(device_ctx);
}
