#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <algorithm>
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
#include "rr_graph_util.h"
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
    e_side side;
};

/******************* Variables local to this module. ***********************/

/********************* Subroutines local to this module. *******************/
void print_rr_graph_stats();

bool channel_widths_unchanged(const t_chan_width& current, const t_chan_width& proposed);

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr Type,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const int* sets_per_seg_type);

static vtr::NdMatrix<int, 5> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const int seg_type_tracks,
                                                            const int Fc,
                                                            const t_physical_tile_type_ptr Type,
                                                            const bool perturb_switch_pattern,
                                                            const e_directionality directionality);

static void advance_to_next_block_side(t_physical_tile_type_ptr Type, int& width_offset, int& height_offset, e_side& side);

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
                                                                             const int width,
                                                                             const int height,
                                                                             const int num_pins,
                                                                             const int max_chan_width,
                                                                             const std::vector<t_segment_inf>& seg_inf);

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
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
                                                                  const int delayless_switch,
                                                                  const enum e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const t_direct_inf* directs,
                                                                  const int num_directs,
                                                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const enum e_clock_modeling clock_modeling);

static float pattern_fmod(float a, float b);
static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
                                                  const std::vector<t_pin_loc>& pin_locations,
                                                  const int x_chan_width,
                                                  const int y_chan_width,
                                                  const int Fc,
                                                  const enum e_directionality directionality);

static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
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

static void build_rr_sinks_sources(RRGraphBuilder& rr_graph_builder,
                                   const int i,
                                   const int j,
                                   t_rr_edge_info_set& rr_edges_to_create,
                                   const int delayless_switch,
                                   const DeviceGrid& grid);

static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
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
                          const enum e_directionality directionality);

void uniquify_edges(t_rr_edge_info_set& rr_edges_to_create);

void alloc_and_load_edges(RRGraphBuilder& rr_graph_builder,
                          const t_rr_edge_info_set& rr_edges_to_create);

static void alloc_and_load_rr_switch_inf(const int num_arch_switches,
                                         const float R_minW_nmos,
                                         const float R_minW_pmos,
                                         const int wire_to_arch_ipin_switch,
                                         int* wire_to_rr_ipin_switch);

static void remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin);

static void load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos, const t_arch_switch_fanin& switch_fanin);

static void alloc_rr_switch_inf(t_arch_switch_fanin& switch_fanin);

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
                                                              bool* Fc_clipped);

static RRNodeId pick_best_direct_connect_target_rr_node(const RRGraphView& rr_graph,
                                                        RRNodeId from_rr,
                                                        const std::vector<RRNodeId>& candidate_rr_nodes);

static void process_non_config_sets();

static void build_rr_graph(const t_graph_type graph_type,
                           const std::vector<t_physical_tile_type>& types,
                           const DeviceGrid& grid,
                           t_chan_width nodes_per_chan,
                           const enum e_switch_block_type sb_type,
                           const int Fs,
                           const std::vector<t_switchblock_inf> switchblocks,
                           const int num_arch_switches,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const enum e_base_cost_type base_cost_type,
                           const enum e_clock_modeling clock_modeling,
                           const t_direct_inf* directs,
                           const int num_directs,
                           int* wire_to_rr_ipin_switch,
                           int* Warnings);

/******************* Subroutine definitions *******************************/

void create_rr_graph(const t_graph_type graph_type,
                     const std::vector<t_physical_tile_type>& block_types,
                     const DeviceGrid& grid,
                     const t_chan_width nodes_per_chan,
                     const int num_arch_switches,
                     t_det_routing_arch* det_routing_arch,
                     const std::vector<t_segment_inf>& segment_inf,
                     const t_router_opts& router_opts,
                     const t_direct_inf* directs,
                     const int num_directs,
                     int* Warnings) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    if (!det_routing_arch->read_rr_graph_filename.empty()) {
        if (device_ctx.read_rr_graph_filename != det_routing_arch->read_rr_graph_filename) {
            free_rr_graph();

            load_rr_file(graph_type,
                         grid,
                         segment_inf,
                         router_opts.base_cost_type,
                         &det_routing_arch->wire_to_rr_ipin_switch,
                         det_routing_arch->read_rr_graph_filename.c_str(),
                         router_opts.read_rr_edge_metadata,
                         router_opts.do_check_rr_graph);
            if (router_opts.reorder_rr_graph_nodes_algorithm != DONT_REORDER) {
                mutable_device_ctx.rr_graph_builder.reorder_nodes(router_opts.reorder_rr_graph_nodes_algorithm,
                                                                  router_opts.reorder_rr_graph_nodes_threshold,
                                                                  router_opts.reorder_rr_graph_nodes_seed);
            }
        }
    } else {
        if (channel_widths_unchanged(device_ctx.chan_width, nodes_per_chan) && !device_ctx.rr_graph.empty()) {
            //No change in channel width, so skip re-building RR graph
            VTR_LOG("RR graph channel widths unchanged, skipping RR graph rebuild\n");
            return;
        }

        free_rr_graph();
        build_rr_graph(graph_type,
                       block_types,
                       grid,
                       nodes_per_chan,
                       det_routing_arch->switch_block_type,
                       det_routing_arch->Fs,
                       det_routing_arch->switchblocks,
                       num_arch_switches,
                       segment_inf,
                       det_routing_arch->global_route_switch,
                       det_routing_arch->wire_to_arch_ipin_switch,
                       det_routing_arch->delayless_switch,
                       det_routing_arch->R_minW_nmos,
                       det_routing_arch->R_minW_pmos,
                       router_opts.base_cost_type,
                       router_opts.clock_modeling,
                       directs, num_directs,
                       &det_routing_arch->wire_to_rr_ipin_switch,
                       Warnings);
        if (router_opts.reorder_rr_graph_nodes_algorithm != DONT_REORDER) {
            mutable_device_ctx.rr_graph_builder.reorder_nodes(router_opts.reorder_rr_graph_nodes_algorithm,
                                                              router_opts.reorder_rr_graph_nodes_threshold,
                                                              router_opts.reorder_rr_graph_nodes_seed);
        }
    }

    process_non_config_sets();

    verify_rr_node_indices(grid, device_ctx.rr_graph, device_ctx.rr_graph.rr_nodes());

    print_rr_graph_stats();

    //Write out rr graph file if needed
    if (!det_routing_arch->write_rr_graph_filename.empty()) {
        write_rr_graph(det_routing_arch->write_rr_graph_filename.c_str());
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
                           const int num_arch_switches,
                           const std::vector<t_segment_inf>& segment_inf,
                           const int global_route_switch,
                           const int wire_to_arch_ipin_switch,
                           const int delayless_switch,
                           const float R_minW_nmos,
                           const float R_minW_pmos,
                           const enum e_base_cost_type base_cost_type,
                           const enum e_clock_modeling clock_modeling,
                           const t_direct_inf* directs,
                           const int num_directs,
                           int* wire_to_rr_ipin_switch,
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
                                         e_fc_type::IN, directionality, &Fc_clipped);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }
        Fc_clipped = false;
        Fc_out = alloc_and_load_actual_fc(types, max_pins, segment_inf, sets_per_seg_type.get(), &nodes_per_chan,
                                          e_fc_type::OUT, directionality, &Fc_clipped);
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

    alloc_and_load_rr_node_indices(device_ctx.rr_graph_builder,
                                   &nodes_per_chan, grid,
                                   &num_rr_nodes, chan_details_x, chan_details_y);

    size_t expected_node_count = num_rr_nodes;
    if (clock_modeling == DEDICATED_NETWORK) {
        expected_node_count += ClockRRGraphBuilder::estimate_additional_nodes(grid);
        device_ctx.rr_graph_builder.reserve_nodes(expected_node_count);
    }
    device_ctx.rr_graph_builder.resize_nodes(num_rr_nodes);

    /* These are data structures used by the the unidir opin mapping. They are used
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
        ipin_to_track_map_x[itype] = alloc_and_load_pin_to_track_map(RECEIVER,
                                                                     Fc_in[itype], &types[itype], perturb_ipins[itype], directionality,
                                                                     segment_inf_x, sets_per_seg_type_x.get());

        ipin_to_track_map_y[itype] = alloc_and_load_pin_to_track_map(RECEIVER,
                                                                     Fc_in[itype], &types[itype], perturb_ipins[itype], directionality,
                                                                     segment_inf_y, sets_per_seg_type_y.get());

        track_to_pin_lookup_x[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_x[itype], Fc_in[itype], types[itype].width, types[itype].height,
                                                                          types[itype].num_pins, nodes_per_chan.x_max, segment_inf_x);

        track_to_pin_lookup_y[itype] = alloc_and_load_track_to_pin_lookup(ipin_to_track_map_y[itype], Fc_in[itype], types[itype].width, types[itype].height,
                                                                          types[itype].num_pins, nodes_per_chan.y_max, segment_inf_y);
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
            auto perturb_opins = alloc_and_load_perturb_opins(&types[itype], Fc_out[itype],
                                                              max_chan_width, segment_inf);
            opin_to_track_map[itype] = alloc_and_load_pin_to_track_map(DRIVER,
                                                                       Fc_out[itype], &types[itype], perturb_opins, directionality,
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
    device_ctx.rr_graph_builder.reserve_switches(device_ctx.num_arch_switches);
    // Create the switches
    for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; ++iswitch) {
        const t_rr_switch_inf& temp_rr_switch = create_rr_switch_from_arch_switch(iswitch, R_minW_nmos, R_minW_pmos);
        device_ctx.rr_graph_builder.add_rr_switch(temp_rr_switch);
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
        delayless_switch,
        directionality,
        &Fc_clipped,
        directs, num_directs, clb_to_clb_directs,
        is_global_graph,
        clock_modeling);

    // Verify no incremental node allocation.
    /* AA: Note that in the case of dedicated networks, we are currently underestimating the additional node count due to the clock networks. 
     * Thus this below error is logged; it's not actually an error, the node estimation needs to get fixed for dedicated clock networks. */
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
    alloc_and_load_rr_switch_inf(num_arch_switches, R_minW_nmos, R_minW_pmos, wire_to_arch_ipin_switch, wire_to_rr_ipin_switch);

    //Partition the rr graph edges for efficient access to configurable/non-configurable
    //edge subsets. Must be done after RR switches have been allocated
    device_ctx.rr_graph_builder.partition_edges();

    //Save the channel widths for the newly constructed graph
    device_ctx.chan_width = nodes_per_chan;

    rr_graph_externals(segment_inf, segment_inf_x, segment_inf_y, *wire_to_rr_ipin_switch, base_cost_type);

    check_rr_graph(graph_type, grid, types);

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
        delete[](clb_to_clb_directs);
    }
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
static void alloc_and_load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos, const int wire_to_arch_ipin_switch, int* wire_to_rr_ipin_switch) {
    /* we will potentially be creating a couple of versions of each arch switch where
     * each version corresponds to a different fan-in. We will need to fill device_ctx.rr_switch_inf
     * with this expanded list of switches.
     *
     * To do this we will use arch_switch_fanins, which is indexed as:
     *      arch_switch_fanins[i_arch_switch][fanin] -> new_switch_id
     */
    t_arch_switch_fanin arch_switch_fanins(num_arch_switches);

    /* Determine what the different fan-ins are for each arch switch, and also
     * how many entries the rr_switch_inf array should have */
    alloc_rr_switch_inf(arch_switch_fanins);

    /* create the rr switches. also keep track of, for each arch switch, what index of the rr_switch_inf
     * array each version of its fanin has been mapped to */
    load_rr_switch_inf(num_arch_switches, R_minW_nmos, R_minW_pmos, arch_switch_fanins);

    /* next, walk through rr nodes again and remap their switch indices to rr_switch_inf */
    remap_rr_node_switch_indices(arch_switch_fanins);

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
static void alloc_rr_switch_inf(t_arch_switch_fanin& arch_switch_fanins) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    /* allocate space for the rr_switch_inf array */
    size_t num_rr_switches = device_ctx.rr_graph_builder.count_rr_switches(
        device_ctx.num_arch_switches,
        device_ctx.arch_switch_inf,
        arch_switch_fanins);
    device_ctx.rr_graph_builder.resize_switches(num_rr_switches);
}

/* load the global device_ctx.rr_switch_inf variable. also keep track of, for each arch switch, what
 * index of the rr_switch_inf array each version of its fanin has been mapped to (through switch_fanin map) */
static void load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos, const t_arch_switch_fanin& arch_switch_fanins) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (!device_ctx.switch_fanin_remap.empty()) {
        // at this stage, we rebuild the rr_graph (probably in binary search)
        // so old device_ctx.switch_fanin_remap is obsolete
        device_ctx.switch_fanin_remap.clear();
    }

    device_ctx.switch_fanin_remap.resize(num_arch_switches);
    for (int i_arch_switch = 0; i_arch_switch < num_arch_switches; i_arch_switch++) {
        std::map<int, int>::iterator it;
        for (auto fanin_rrswitch : arch_switch_fanins[i_arch_switch]) {
            /* the fanin value is in it->first, and we'll need to set what index this i_arch_switch/fanin
             * combination maps to (within rr_switch_inf) in it->second) */
            int fanin;
            int i_rr_switch;
            std::tie(fanin, i_rr_switch) = fanin_rrswitch;

            // setup device_ctx.switch_fanin_remap, for future swich usage analysis
            device_ctx.switch_fanin_remap[i_arch_switch][fanin] = i_rr_switch;

            load_rr_switch_from_arch_switch(i_arch_switch, i_rr_switch, fanin, R_minW_nmos, R_minW_pmos);
        }
    }
}
/* This function creates a routing switch for the usage of routing resource graph, based on a routing switch defined in architecture file.
 *
 * Since users can specify a routing switch whose buffer size is automatically tuned for routing architecture, the function here sets a definite buffer size, as required by placers and routers.
 */

t_rr_switch_inf create_rr_switch_from_arch_switch(int arch_switch_idx,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    t_rr_switch_inf rr_switch_inf;

    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = device_ctx.arch_switch_inf[arch_switch_idx].Tdel(0);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    rr_switch_inf.set_type(device_ctx.arch_switch_inf[arch_switch_idx].type());
    rr_switch_inf.R = device_ctx.arch_switch_inf[arch_switch_idx].R;
    rr_switch_inf.Cin = device_ctx.arch_switch_inf[arch_switch_idx].Cin;
    rr_switch_inf.Cinternal = device_ctx.arch_switch_inf[arch_switch_idx].Cinternal;
    rr_switch_inf.Cout = device_ctx.arch_switch_inf[arch_switch_idx].Cout;
    rr_switch_inf.Tdel = rr_switch_Tdel;
    rr_switch_inf.mux_trans_size = device_ctx.arch_switch_inf[arch_switch_idx].mux_trans_size;
    if (device_ctx.arch_switch_inf[arch_switch_idx].buf_size_type == BufferSize::AUTO) {
        //Size based on resistance
        rr_switch_inf.buf_size = trans_per_buf(device_ctx.arch_switch_inf[arch_switch_idx].R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(device_ctx.arch_switch_inf[arch_switch_idx].buf_size_type == BufferSize::ABSOLUTE);
        //Use the specified size
        rr_switch_inf.buf_size = device_ctx.arch_switch_inf[arch_switch_idx].buf_size;
    }
    rr_switch_inf.name = device_ctx.arch_switch_inf[arch_switch_idx].name;
    rr_switch_inf.power_buffer_type = device_ctx.arch_switch_inf[arch_switch_idx].power_buffer_type;
    rr_switch_inf.power_buffer_size = device_ctx.arch_switch_inf[arch_switch_idx].power_buffer_size;

    return rr_switch_inf;
}
/* This function is same as create_rr_switch_from_arch_switch() in terms of functionality. It is tuned for clients functions in routing resource graph builder */
void load_rr_switch_from_arch_switch(int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = device_ctx.arch_switch_inf[arch_switch_idx].Tdel(fanin);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].set_type(device_ctx.arch_switch_inf[arch_switch_idx].type());
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].R = device_ctx.arch_switch_inf[arch_switch_idx].R;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cin = device_ctx.arch_switch_inf[arch_switch_idx].Cin;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cinternal = device_ctx.arch_switch_inf[arch_switch_idx].Cinternal;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Cout = device_ctx.arch_switch_inf[arch_switch_idx].Cout;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].Tdel = rr_switch_Tdel;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].mux_trans_size = device_ctx.arch_switch_inf[arch_switch_idx].mux_trans_size;
    if (device_ctx.arch_switch_inf[arch_switch_idx].buf_size_type == BufferSize::AUTO) {
        //Size based on resistance
        device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].buf_size = trans_per_buf(device_ctx.arch_switch_inf[arch_switch_idx].R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(device_ctx.arch_switch_inf[arch_switch_idx].buf_size_type == BufferSize::ABSOLUTE);
        //Use the specified size
        device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].buf_size = device_ctx.arch_switch_inf[arch_switch_idx].buf_size;
    }
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].name = device_ctx.arch_switch_inf[arch_switch_idx].name;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_type = device_ctx.arch_switch_inf[arch_switch_idx].power_buffer_type;
    device_ctx.rr_graph_builder.rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_size = device_ctx.arch_switch_inf[arch_switch_idx].power_buffer_size;
}

/* switch indices of each rr_node original point into the global device_ctx.arch_switch_inf array.
 * now we want to remap these indices to point into the global device_ctx.rr_switch_inf array
 * which contains switch info at different fan-in values */
static void remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.rr_graph_builder.remap_rr_node_switch_indices(switch_fanin);
}

static void rr_graph_externals(const std::vector<t_segment_inf>& segment_inf,
                               const std::vector<t_segment_inf>& segment_inf_x,
                               const std::vector<t_segment_inf>& segment_inf_y,
                               int wire_to_rr_ipin_switch,
                               enum e_base_cost_type base_cost_type) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    add_rr_graph_C_from_switches(rr_graph.rr_switch_inf(RRSwitchId(wire_to_rr_ipin_switch)).Cin);
    alloc_and_load_rr_indexed_data(segment_inf, segment_inf_x,
                                   segment_inf_y, wire_to_rr_ipin_switch, base_cost_type);
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
                                                              bool* Fc_clipped) {
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
                                        block_type_pin_index_to_name(&type, fc_spec.pins[0]).c_str(),
                                        segment_inf[iseg].name.c_str());
                    }

                    if (fc_spec.fc_value < fac) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Absolute Fc value must be at least %d (was %f) between block pin '%s' to wire segment %s",
                                        fac, fc_spec.fc_value,
                                        block_type_pin_index_to_name(&type, fc_spec.pins[0]).c_str(),
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
                                                                  const int delayless_switch,
                                                                  const enum e_directionality directionality,
                                                                  bool* Fc_clipped,
                                                                  const t_direct_inf* directs,
                                                                  const int num_directs,
                                                                  const t_clb_to_clb_directs* clb_to_clb_directs,
                                                                  bool is_global_graph,
                                                                  const enum e_clock_modeling clock_modeling) {
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
    /* Connection SINKS and SOURCES to their pins. */
    for (size_t i = 0; i < grid.width(); ++i) {
        for (size_t j = 0; j < grid.height(); ++j) {
            build_rr_sinks_sources(rr_graph_builder, i, j, rr_edges_to_create,
                                   delayless_switch, grid);

            //Create the actual SOURCE->OPIN, IPIN->SINK edges
            uniquify_edges(rr_edges_to_create);
            alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
            num_edges += rr_edges_to_create.size();
            rr_edges_to_create.clear();
        }
    }
    VTR_LOG("SOURCE->OPIN and IPIN->SINK edge count:%d\n", num_edges);
    num_edges = 0;
    /* Build opins */
    int rr_edges_before_directs = 0;
    for (size_t i = 0; i < grid.width(); ++i) {
        for (size_t j = 0; j < grid.height(); ++j) {
            for (e_side side : SIDES) {
                if (BI_DIRECTIONAL == directionality) {
                    build_bidir_rr_opins(rr_graph_builder, rr_graph, i, j, side,
                                         opin_to_track_map, Fc_out, rr_edges_to_create, chan_details_x, chan_details_y,
                                         grid,
                                         directs, num_directs, clb_to_clb_directs, num_seg_types);
                } else {
                    VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                    bool clipped;
                    build_unidir_rr_opins(rr_graph_builder, rr_graph, i, j, side, grid, Fc_out, chan_width,
                                          chan_details_x, chan_details_y, Fc_xofs, Fc_yofs,
                                          rr_edges_to_create, &clipped, seg_index_map,
                                          directs, num_directs, clb_to_clb_directs, num_seg_types, rr_edges_before_directs);
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
    VTR_LOG("OPIN->CHANX/CHANY edge count before creating direct connections: %d\n", rr_edges_before_directs);
    VTR_LOG("OPIN->CHANX/CHANY edge count after creating direct connections: %d\n", num_edges);
    num_edges = 0;
    /* Build channels */
    VTR_ASSERT(Fs % 3 == 0);
    for (size_t i = 0; i < grid.width() - 1; ++i) {
        for (size_t j = 0; j < grid.height() - 1; ++j) {
            if (i > 0) {
                int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.x_list[j]);
                build_rr_chan(rr_graph_builder, i, j, CHANX, track_to_pin_lookup_x, sb_conn_map, switch_block_conn,
                              CHANX_COST_INDEX_START,
                              chan_width, grid, tracks_per_chan,
                              sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                              rr_edges_to_create,
                              wire_to_ipin_switch,
                              directionality);

                //Create the actual CHAN->CHAN edges
                uniquify_edges(rr_edges_to_create);
                alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                num_edges += rr_edges_to_create.size();

                rr_edges_to_create.clear();
            }
            if (j > 0) {
                int tracks_per_chan = ((is_global_graph) ? 1 : chan_width.y_list[i]);
                build_rr_chan(rr_graph_builder, i, j, CHANY, track_to_pin_lookup_y, sb_conn_map, switch_block_conn,
                              CHANX_COST_INDEX_START + num_seg_types_x,
                              chan_width, grid, tracks_per_chan,
                              sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                              rr_edges_to_create,
                              wire_to_ipin_switch,
                              directionality);

                //Create the actual CHAN->CHAN edges
                uniquify_edges(rr_edges_to_create);
                alloc_and_load_edges(rr_graph_builder, rr_edges_to_create);
                num_edges += rr_edges_to_create.size();

                rr_edges_to_create.clear();
            }
        }
    }
    VTR_LOG("CHAN->CHAN type edge count:%d\n", num_edges);
    num_edges = 0;
    std::function<void(t_chan_width*)> update_chan_width = [](t_chan_width*) {
    };
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

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
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

    auto type = grid[i][j].type;
    int width_offset = grid[i][j].width_offset;
    int height_offset = grid[i][j].height_offset;

    const vtr::Matrix<int>& Fc = Fc_out[type->index];

    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        /* We only are working with opins so skip non-drivers */
        if (type->class_inf[type->pin_class[pin_index]].type != DRIVER) {
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

        RRNodeId node_index = rr_graph_builder.node_lookup().find_node(i, j, OPIN, pin_index, side);
        VTR_ASSERT(node_index);

        if (total_pin_Fc > 0) {
            get_bidir_opin_connections(rr_graph_builder, i, j, pin_index,
                                       node_index, rr_edges_to_create, opin_to_track_map,
                                       chan_details_x,
                                       chan_details_y);
        }

        /* Add in direct connections */
        get_opin_direct_connections(rr_graph_builder, rr_graph, i, j, side, pin_index,
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

    invalidate_router_lookahead_cache();
}

static void build_rr_sinks_sources(RRGraphBuilder& rr_graph_builder,
                                   const int i,
                                   const int j,
                                   t_rr_edge_info_set& rr_edges_to_create,
                                   const int delayless_switch,
                                   const DeviceGrid& grid) {
    /* Loads IPIN, SINK, SOURCE, and OPIN.
     * Loads IPIN to SINK edges, and SOURCE to OPIN edges */

    /* Since we share nodes within a large block, only
     * start tile can initialize sinks, sources, and pins */
    if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0)
        return;

    auto type = grid[i][j].type;
    int num_class = (int)type->class_inf.size();
    const std::vector<t_class>& class_inf = type->class_inf;
    int num_pins = type->num_pins;
    const std::vector<int>& pin_class = type->pin_class;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    /* SINK and SOURCE-to-OPIN edges */
    //int edges_created =0;
    for (int iclass = 0; iclass < num_class; ++iclass) {
        RRNodeId inode = RRNodeId::INVALID();
        if (class_inf[iclass].type == DRIVER) { /* SOURCE */
            inode = rr_graph_builder.node_lookup().find_node(i, j, SOURCE, iclass);
            VTR_ASSERT(inode);

            //Retrieve all the physical OPINs associated with this source, this includes
            //those at different grid tiles of this block
            std::vector<RRNodeId> opin_nodes;
            for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                    for (int ipin = 0; ipin < class_inf[iclass].num_pins; ++ipin) {
                        int pin_num = class_inf[iclass].pinlist[ipin];
                        std::vector<RRNodeId> physical_pins = rr_graph_builder.node_lookup().find_nodes_at_all_sides(i + width_offset, j + height_offset, OPIN, pin_num);
                        opin_nodes.insert(opin_nodes.end(), physical_pins.begin(), physical_pins.end());
                    }
                }
            }

            //Connect the SOURCE to each OPIN
            //edges_created += opin_nodes.size();
            for (size_t iedge = 0; iedge < opin_nodes.size(); ++iedge) {
                rr_edges_to_create.emplace_back(inode, opin_nodes[iedge], delayless_switch);
            }

            rr_graph_builder.set_node_cost_index(inode, RRIndexedDataId(SOURCE_COST_INDEX));
            rr_graph_builder.set_node_type(inode, SOURCE);
        } else { /* SINK */
            VTR_ASSERT(class_inf[iclass].type == RECEIVER);
            inode = rr_graph_builder.node_lookup().find_node(i, j, SINK, iclass);

            VTR_ASSERT(inode);

            /* NOTE:  To allow route throughs through clbs, change the lines below to  *
             * make an edge from the input SINK to the output SOURCE.  Do for just the *
             * special case of INPUTS = class 0 and OUTPUTS = class 1 and see what it  *
             * leads to.  If route throughs are allowed, you may want to increase the  *
             * base cost of OPINs and/or SOURCES so they aren't used excessively.      */

            /* TODO: The casting will be removed when RRGraphBuilder has the following APIs:
             * - set_node_cost_index(RRNodeId, int);
             * - set_node_type(RRNodeId, t_rr_type);
             */
            rr_graph_builder.set_node_cost_index(inode, RRIndexedDataId(SINK_COST_INDEX));
            rr_graph_builder.set_node_type(inode, SINK);
        }

        /* Things common to both SOURCEs and SINKs.   */
        rr_graph_builder.set_node_capacity(inode, class_inf[iclass].num_pins);
        rr_graph_builder.set_node_coordinates(inode, i, j, i + type->width - 1, j + type->height - 1);
        float R = 0.;
        float C = 0.;
        rr_graph_builder.set_node_rc_index(inode, NodeRCIndex(find_create_rr_rc_data(R, C)));
        rr_graph_builder.set_node_class_num(inode, iclass);
    }

    //VTR_LOG("%d SOURCE->OPIN EDGES CREATED\n",edges_created);

    //edges_created = 0;
    /* Connect IPINS to SINKS and initialize OPINS */
    //We loop through all the pin locations on the block to initialize the IPINs/OPINs,
    //and hook-up the IPINs to sinks.
    for (int ipin = 0; ipin < num_pins; ++ipin) {
        for (e_side side : SIDES) {
            for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                    if (type->pinloc[width_offset][height_offset][side][ipin]) {
                        RRNodeId inode = RRNodeId::INVALID();
                        int iclass = pin_class[ipin];

                        if (class_inf[iclass].type == RECEIVER) {
                            //Connect the input pin to the sink
                            inode = rr_graph_builder.node_lookup().find_node(i + width_offset, j + height_offset, IPIN, ipin, side);

                            /* Input pins are uniquified, we may not always find one */
                            if (inode) {
                                RRNodeId to_node = rr_graph_builder.node_lookup().find_node(i, j, SINK, iclass);

                                //Add info about the edge to be created
                                rr_edges_to_create.emplace_back(inode, to_node, delayless_switch);
                                //edges_created ++;
                                rr_graph_builder.set_node_cost_index(inode, RRIndexedDataId(IPIN_COST_INDEX));
                                rr_graph_builder.set_node_type(inode, IPIN);
                            }
                        } else {
                            VTR_ASSERT(class_inf[iclass].type == DRIVER);
                            //Initialize the output pin
                            // Note that we leave it's out-going edges unconnected (they will be hooked up to global routing later)
                            inode = rr_graph_builder.node_lookup().find_node(i + width_offset, j + height_offset, OPIN, ipin, side);

                            /* Output pins may not exist on some sides, we may not always find one */
                            if (inode) {
                                //Initially left unconnected
                                rr_graph_builder.set_node_cost_index(inode, RRIndexedDataId(OPIN_COST_INDEX));
                                rr_graph_builder.set_node_type(inode, OPIN);
                            }
                        }

                        /* Common to both DRIVERs and RECEIVERs */
                        if (inode) {
                            rr_graph_builder.set_node_capacity(inode, 1);
                            float R = 0.;
                            float C = 0.;
                            rr_graph_builder.set_node_rc_index(inode, NodeRCIndex(find_create_rr_rc_data(R, C)));
                            rr_graph_builder.set_node_pin_num(inode, ipin);

                            //Note that we store the grid tile location and side where the pin is located,
                            //which greatly simplifies the drawing code
                            //For those pins located on multiple sides, we save the rr node index
                            //for the pin on all sides at which it exists
                            //As such, multipler driver problem can be avoided.
                            rr_graph_builder.set_node_coordinates(inode, i + width_offset, j + height_offset, i + width_offset, j + height_offset);
                            rr_graph_builder.add_node_side(inode, side);

                            // Sanity check
                            VTR_ASSERT(rr_graph.is_node_on_specific_side(RRNodeId(inode), side));
                            VTR_ASSERT(type->pinloc[width_offset][height_offset][side][rr_graph.node_pin_num(RRNodeId(inode))]);
                        }
                    }
                }
            }
        }
    }
    //VTR_LOG("%d IPIN->SOURCE EDGES CREATED\n",edges_created);

    //Create the actual edges
}

/* Allocates/loads edges for nodes belonging to specified channel segment and initializes
 * node properties such as cost, occupancy and capacity */
static void build_rr_chan(RRGraphBuilder& rr_graph_builder,
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
                          const enum e_directionality directionality) {
    /* this function builds both x and y-directed channel segments, so set up our
     * coordinates based on channel type */

    auto& device_ctx = g_vpr_ctx.device();

    //Initally a assumes CHANX
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

        RRNodeId node = rr_graph_builder.node_lookup().find_node(x_coord, y_coord, chan_type, track);

        if (!node) {
            continue;
        }

        /* Add the edges from this track to all it's connected pins into the list */
        int num_edges = 0;
        num_edges += get_track_to_pins(rr_graph_builder, start, chan_coord, track, tracks_per_chan, node, rr_edges_to_create,
                                       track_to_pin_lookup, seg_details, chan_type, seg_dimension,
                                       wire_to_ipin_switch, directionality);

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
                num_edges += get_track_to_tracks(rr_graph_builder, chan_coord, start, track, chan_type, chan_coord,
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
                num_edges += get_track_to_tracks(rr_graph_builder, chan_coord, start, track, chan_type, chan_coord + 1,
                                                 opposite_chan_type, seg_dimension, max_opposite_chan_width, grid,
                                                 Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                                                 from_seg_details, to_seg_details, opposite_chan_details,
                                                 directionality,
                                                 switch_block_conn, sb_conn_map);
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
                    num_edges += get_track_to_tracks(rr_graph_builder, chan_coord, start, track, chan_type, target_seg,
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

        int length = end - start + 1;
        float R = length * seg_details[track].Rmetal();
        float C = length * seg_details[track].Cmetal();
        rr_graph_builder.set_node_rc_index(node, NodeRCIndex(find_create_rr_rc_data(R, C)));

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
static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
                                                                          const vtr::Matrix<int>& Fc,
                                                                          const t_physical_tile_type_ptr Type,
                                                                          const std::vector<bool>& perturb_switch_pattern,
                                                                          const e_directionality directionality,
                                                                          const std::vector<t_segment_inf>& seg_inf,
                                                                          const int* sets_per_seg_type) {
    /* get the maximum number of tracks that any pin can connect to */
    size_t max_pin_tracks = 0;

    int num_parallel_seg_types = seg_inf.size();

    for (int iseg = 0; iseg < num_parallel_seg_types; iseg++) {
        /* determine the maximum Fc to this segment type across all pins */
        int max_Fc = 0;
        for (int pin_index = 0; pin_index < Type->num_pins; ++pin_index) {
            int pin_class = Type->pin_class[pin_index];
            if (Fc[pin_index][seg_inf[iseg].seg_index] > max_Fc && Type->class_inf[pin_class].type == pin_type) {
                max_Fc = Fc[pin_index][seg_inf[iseg].seg_index];
            }
        }

        max_pin_tracks += max_Fc;
    }

    /* allocate 'result' matrix and initialize entries to OPEN. also allocate and intialize matrix which will be
     * used to index into the correct entries when loading up 'result' */

    auto result = vtr::NdMatrix<std::vector<int>, 4>({
        size_t(Type->num_pins), //[0..num_pins-1]
        size_t(Type->width),    //[0..width-1]
        size_t(Type->height),   //[0..height-1]
        4,                      //[0..sides-1]
    });

    /* multiplier for unidirectional vs bidirectional architectures */
    int fac = 1;
    if (directionality == UNI_DIRECTIONAL) {
        fac = 2;
    }

    /* load the pin to track matrix by looking at each segment type in turn */
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
        auto pin_to_seg_type_map = alloc_and_load_pin_to_seg_type(pin_type, num_seg_type_tracks, max_Fc, Type, perturb_switch_pattern[seg_inf[iseg].seg_index], directionality);

        /* connections in pin_to_seg_type_map are within that seg type -- i.e. in the [0,num_seg_type_tracks-1] range.
         * now load up 'result' array with these connections, but offset them so they are relative to the channel
         * as a whole */
        for (int ipin = 0; ipin < Type->num_pins; ipin++) {
            for (int iwidth = 0; iwidth < Type->width; iwidth++) {
                for (int iheight = 0; iheight < Type->height; iheight++) {
                    for (int iside = 0; iside < 4; iside++) {
                        for (int iconn = 0; iconn < max_Fc; iconn++) {
                            int relative_track_ind = pin_to_seg_type_map[ipin][iwidth][iheight][iside][iconn];

                            if (relative_track_ind == OPEN) continue;

                            VTR_ASSERT(relative_track_ind <= num_seg_type_tracks);
                            int absolute_track_ind = relative_track_ind + seg_type_start_track;

                            VTR_ASSERT(absolute_track_ind >= 0);
                            result[ipin][iwidth][iheight][iside].push_back(absolute_track_ind);
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

static vtr::NdMatrix<int, 5> alloc_and_load_pin_to_seg_type(const e_pin_type pin_type,
                                                            const int num_seg_type_tracks,
                                                            const int Fc,
                                                            const t_physical_tile_type_ptr Type,
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

    if (Type->num_pins < 1) {
        return vtr::NdMatrix<int, 5>();
    }

    auto tracks_connected_to_pin = vtr::NdMatrix<int, 5>({
                                                             size_t(Type->num_pins), //[0..num_pins-1]
                                                             size_t(Type->width),    //[0..width-1]
                                                             size_t(Type->height),   //[0..height-1]
                                                             NUM_SIDES,              //[0..NUM_SIDES-1]
                                                             size_t(Fc)              //[0..Fc-1]
                                                         },
                                                         OPEN); //Unconnected

    //Number of *physical* pins on each side.
    //Note that his may be more than the logical number of pins (i.e.
    //Type->num_pins) if a logical pin has multiple specified physical
    //pinlocations (i.e. appears on multiple sides of the block)
    auto num_dir = vtr::NdMatrix<int, 3>({
                                             size_t(Type->width),  //[0..width-1]
                                             size_t(Type->height), //[0..height-1]
                                             NUM_SIDES             //[0..NUM_SIDES-1]
                                         },
                                         0);

    //List of *physical* pins of the correct type on each side of the current
    //block type. For a specific width/height/side the valid enteries in the
    //last dimension are [0 .. num_dir[width][height][side]-1]
    //
    //Max possible space alloced for simplicity
    auto dir_list = vtr::NdMatrix<int, 4>({
                                              size_t(Type->width),   //[0..width-1]
                                              size_t(Type->height),  //[0..height-1]
                                              NUM_SIDES,             //[0..NUM_SIDES-1]
                                              size_t(Type->num_pins) //[0..num_pins-1]
                                          },
                                          -1); //Defensive coding: Initialize to invalid

    //Number of currently assigned physical pins
    auto num_done_per_dir = vtr::NdMatrix<int, 3>({
                                                      size_t(Type->width),  //[0..width-1]
                                                      size_t(Type->height), //[0..height-1]
                                                      NUM_SIDES             //[0..NUM_SIDES-1]
                                                  },
                                                  0);

    //Record the physical pin locations and counts per side/offsets combination
    for (int pin = 0; pin < Type->num_pins; ++pin) {
        int pin_class = Type->pin_class[pin];
        if (Type->class_inf[pin_class].type != pin_type) /* Doing either ipins OR opins */
            continue;

        /* Pins connecting only to global resources get no switches -> keeps area model accurate. */
        if (Type->is_ignored_pin[pin])
            continue;

        for (int width = 0; width < Type->width; ++width) {
            for (int height = 0; height < Type->height; ++height) {
                for (e_side side : SIDES) {
                    if (Type->pinloc[width][height][side][pin] == 1) {
                        dir_list[width][height][side][num_dir[width][height][side]] = pin;
                        num_dir[width][height][side]++;
                    }
                }
            }
        }
    }

    //Total the number of physical pins
    int num_phys_pins = 0;
    for (int width = 0; width < Type->width; ++width) {
        for (int height = 0; height < Type->height; ++height) {
            for (e_side side : SIDES) {
                num_phys_pins += num_dir[width][height][side]; /* Num. physical pins per type */
            }
        }
    }

    std::vector<t_pin_loc> pin_ordering;

    /* Connection block I use distributes pins evenly across the tracks      *
     * of ALL sides of the clb at once.  Ensures that each pin connects      *
     * to spaced out tracks in its connection block, and that the other      *
     * pins (potentially in other C blocks) connect to the remaining tracks  *
     * first.  Doesn't matter for large Fc, but should make a fairly         *
     * good low Fc block that leverages the fact that usually lots of pins   *
     * are logically equivalent.                                             */

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
    while (pin < num_phys_pins) {
        if (height == init_height && width == init_width && side == init_side) {
            //Completed one loop through all the possible offsets/side combinations
            pin_index++;
        }

        advance_to_next_block_side(Type, width, height, side);

        VTR_ASSERT_MSG(pin_index < num_phys_pins, "Physical block pins bound number of logical block pins");

        if (num_done_per_dir[width][height][side] >= num_dir[width][height][side]) {
            continue;
        }

        int pin_num = dir_list[width][height][side][pin_index];
        VTR_ASSERT(pin_num >= 0);
        VTR_ASSERT(Type->pinloc[width][height][side][pin_num]);

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
    VTR_ASSERT(pin_ordering.size() == size_t(num_phys_pins));

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

static void load_uniform_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
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
                tracks_connected_to_pin[pin][width][height][side][group_size * j + k] = (itrack + k) % max_chan_width;

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
static void load_perturbed_connection_block_pattern(vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
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

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map,
                                                                             const vtr::Matrix<int>& Fc,
                                                                             const int type_width,
                                                                             const int type_height,
                                                                             const int num_pins,
                                                                             const int max_chan_width,
                                                                             const std::vector<t_segment_inf>& seg_inf) {
    /* [0..max_chan_width-1][0..width][0..height][0..3].  For each track number
     * it stores a vector for each of the four sides.  x-directed channels will
     * use the TOP and   BOTTOM vectors to figure out what clb input pins they
     * connect to above  and below them, respectively, while y-directed channels
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
    /* Alloc and zero the the lookup table */
    auto track_to_pin_lookup = vtr::NdMatrix<std::vector<int>, 4>({size_t(max_chan_width), size_t(type_width), size_t(type_height), 4});

    /* Count number of pins to which each track connects  */
    for (int pin = 0; pin < num_pins; ++pin) {
        for (int width = 0; width < type_width; ++width) {
            for (int height = 0; height < type_height; ++height) {
                for (int side = 0; side < 4; ++side) {
                    if (pin_to_track_map[pin][width][height][side].empty())
                        continue;

                    /* get number of tracks to which this pin connects */
                    int num_tracks = 0;
                    for (int iseg = 0; iseg < num_seg_types; iseg++) {
                        num_tracks += Fc[pin][seg_inf[iseg].seg_index]; // AA: Fc_in and Fc_out matrices are unified for all segments so need to map index.
                    }

                    num_tracks = std::min(num_tracks, (int)pin_to_track_map[pin][width][height][side].size());
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

    return track_to_pin_lookup;
}

/* TODO: This function should adapt RRNodeId */
std::string describe_rr_node(int inode) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    std::string msg = vtr::string_fmt("RR node: %d", inode);

    if (rr_graph.node_type(RRNodeId(inode)) == CHANX || rr_graph.node_type(RRNodeId(inode)) == CHANY) {
        auto cost_index = rr_graph.node_cost_index(RRNodeId(inode));

        int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;
        std::string rr_node_direction_string = rr_graph.node_direction_string(RRNodeId(inode));

        if (seg_index < (int)rr_graph.num_rr_segments()) {
            msg += vtr::string_fmt(" track: %d longline: %d",
                                   rr_graph.node_track_num(RRNodeId(inode)),
                                   rr_graph.rr_segments(RRSegmentId(seg_index)).longline);
        } else {
            msg += vtr::string_fmt(" track: %d seg_type: ILLEGAL_SEG_INDEX %d",
                                   rr_graph.node_track_num(RRNodeId(inode)),
                                   seg_index);
        }
    } else if (rr_graph.node_type(RRNodeId(inode)) == IPIN || rr_graph.node_type(RRNodeId(inode)) == OPIN) {
        auto type = device_ctx.grid[rr_graph.node_xlow(RRNodeId(inode))][rr_graph.node_ylow(RRNodeId(inode))].type;
        std::string pin_name = block_type_pin_index_to_name(type, rr_graph.node_pin_num(RRNodeId(inode)));

        msg += vtr::string_fmt(" pin: %d pin_name: %s",
                               rr_graph.node_pin_num(RRNodeId(inode)),
                               pin_name.c_str());
    } else {
        VTR_ASSERT(rr_graph.node_type(RRNodeId(inode)) == SOURCE || rr_graph.node_type(RRNodeId(inode)) == SINK);

        msg += vtr::string_fmt(" class: %d", rr_graph.node_class_num(RRNodeId(inode)));
    }

    msg += vtr::string_fmt(" capacity: %d", rr_graph.node_capacity(RRNodeId(inode)));
    msg += vtr::string_fmt(" fan-in: %d", rr_graph.node_fan_in(RRNodeId(inode)));
    msg += vtr::string_fmt(" fan-out: %d", rr_graph.num_edges(RRNodeId(inode)));

    msg += " " + rr_graph.node_coordinate_to_string(RRNodeId(inode));

    return msg;
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

    auto type = grid[i][j].type;

    int width_offset = grid[i][j].width_offset;
    int height_offset = grid[i][j].height_offset;

    /* Go through each pin and find its fanout. */
    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        /* Skip global pins and pins that are not of DRIVER type */
        int class_index = type->pin_class[pin_index];
        if (type->class_inf[class_index].type != DRIVER) {
            continue;
        }
        if (type->is_ignored_pin[pin_index]) {
            continue;
        }

        RRNodeId opin_node_index = rr_graph_builder.node_lookup().find_node(i, j, OPIN, pin_index, side);
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

            //VTR_ASSERT_MSG(seg_index == 0 || seg_index > 0,"seg_index map not working properly");

            rr_edge_count += get_unidir_opin_connections(rr_graph_builder, chan, seg,
                                                         seg_type_Fc, seg_index, chan_type, seg_details,
                                                         opin_node_index,
                                                         rr_edges_to_create,
                                                         Fc_ofs, max_len, nodes_per_chan,
                                                         &clipped);

            if (clipped) {
                *Fc_clipped = true;
            }
        }

        /* Add in direct connections */
        get_opin_direct_connections(rr_graph_builder, rr_graph, i, j, side, pin_index, opin_node_index, rr_edges_to_create,
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
        delete[](tile_name);
        delete[](port_name);
    }

    return clb_to_clb_directs;
}

/* Add all direct clb-pin-to-clb-pin edges to given opin
 *
 * The current opin is located at (x,y) along the specified side
 */
static int get_opin_direct_connections(RRGraphBuilder& rr_graph_builder,
                                       const RRGraphView& rr_graph,
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

    t_physical_tile_type_ptr curr_type = device_ctx.grid[x][y].type;

    int num_pins = 0;

    int width_offset = device_ctx.grid[x][y].width_offset;
    int height_offset = device_ctx.grid[x][y].height_offset;
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
                t_physical_tile_type_ptr target_type = device_ctx.grid[x + directs[i].x_offset][y + directs[i].y_offset].type;

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

                        int target_sub_tile = z + directs[i].sub_tile_offset;
                        if (relative_ipin >= target_type->sub_tiles[target_sub_tile].num_phy_pins) continue;

                        //If this block has capacity > 1 then the pins of z position > 0 are offset
                        //by the number of pins per capacity instance
                        int ipin = get_physical_pin_from_capacity_location(target_type, relative_ipin, target_sub_tile);

                        /* Add new ipin edge to list of edges */
                        std::vector<RRNodeId> inodes;

                        if (directs[i].to_side != NUM_SIDES) {
                            //Explicit side specified, only create if pin exists on that side
                            RRNodeId inode = rr_graph_builder.node_lookup().find_node(x + directs[i].x_offset, y + directs[i].y_offset, IPIN, ipin, directs[i].to_side);
                            if (inode) {
                                inodes.push_back(inode);
                            }
                        } else {
                            //No side specified, get all candidates
                            inodes = rr_graph_builder.node_lookup().find_nodes_at_all_sides(x + directs[i].x_offset, y + directs[i].y_offset, IPIN, ipin);
                        }

                        if (inodes.size() > 0) {
                            //There may be multiple physical pins corresponding to the logical
                            //target ipin. We only need to connect to one of them (since the physical pins
                            //are logically equivalent). This also ensures the graphics look reasonable and map
                            //back fairly directly to the architecture file in the case of pin equivalence
                            RRNodeId inode = pick_best_direct_connect_target_rr_node(rr_graph, from_rr_node, inodes);

                            rr_edges_to_create.emplace_back(from_rr_node, inode, clb_to_clb_directs[i].switch_index);
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
        iclass = type->pin_class[i];
        if (Fc_out[i][0] > Fc_max && type->class_inf[iclass].type == DRIVER) {
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
    delete[](prime_factors);

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
                groups->add_non_config_edge(size_t(src), size_t(sink));
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
