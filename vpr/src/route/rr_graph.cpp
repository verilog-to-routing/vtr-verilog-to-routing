#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <vector>
#include "vtr_assert.h"

using namespace std;

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_matrix.h"
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

#include "rr_types.h"

//#define VERBOSE

struct t_mux {
    int size;
    t_mux *next;
};

struct t_mux_size_distribution {
    int mux_count;
    int max_index;
    int *distr;
    t_mux_size_distribution *next;
};

struct t_clb_to_clb_directs {
    t_type_descriptor *from_clb_type;
    int from_clb_pin_start_index;
    int from_clb_pin_end_index;
    t_type_descriptor *to_clb_type;
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
static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
        const vtr::Matrix<int>& Fc, const t_type_ptr Type, const std::vector<bool>& perturb_switch_pattern,
        const e_directionality directionality,
        const int num_seg_types, const int *sets_per_seg_type);

static vtr::NdMatrix<int, 5> alloc_and_load_pin_to_seg_type(
        const e_pin_type pin_type,
        const int seg_type_tracks, const int Fc,
        const t_type_ptr Type, const bool perturb_switch_pattern,
        const e_directionality directionality);

static void advance_to_next_block_side(t_type_ptr Type, int& width_offset, int& height_offset, e_side& side);

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(
        vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map, const vtr::Matrix<int>& Fc,
        const int width, const int height,
        const int num_pins, const int max_chan_width,
        const int num_seg_types);

static void build_bidir_rr_opins(const int i, const int j, const e_side side,
        std::vector<t_rr_node>& L_rr_node, const t_rr_node_indices& L_rr_node_indices,
        const t_pin_to_track_lookup& opin_to_track_map, const std::vector<vtr::Matrix<int>>&Fc_out,
        t_rr_edge_info_set& created_rr_edges,
        const t_seg_details * seg_details,
        const DeviceGrid& grid,
        const t_direct_inf *directs, const int num_directs, const t_clb_to_clb_directs *clb_to_clb_directs,
        const int num_seg_types);

static void build_unidir_rr_opins(
        const int i, const int j, const e_side side,
        const DeviceGrid& grid, const std::vector<vtr::Matrix<int>>&Fc_out,
        const int max_chan_width,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        vtr::NdMatrix<int, 3>& Fc_xofs, vtr::NdMatrix<int, 3>& Fc_yofs,
        t_rr_edge_info_set& created_rr_edges,
        bool * Fc_clipped, const t_rr_node_indices& L_rr_node_indices,
        const t_direct_inf *directs, const int num_directs, const t_clb_to_clb_directs *clb_to_clb_directs,
        const int num_seg_types);

static int get_opin_direct_connecions(
        int x, int y, e_side side, int opin,
        int from_rr_node, t_rr_edge_info_set& rr_edges_to_create, const t_rr_node_indices& L_rr_node_indices,
        const t_direct_inf *directs, const int num_directs,
        const t_clb_to_clb_directs *clb_to_clb_directs);

static void alloc_and_load_rr_graph(
        const int num_nodes,
        std::vector<t_rr_node>& L_rr_node, const int num_seg_types,
        const t_seg_details * seg_details,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        const t_track_to_pin_lookup& track_to_pin_lookup,
        const t_pin_to_track_lookup& opin_to_track_map, const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
        t_sb_connection_map *sb_conn_map,
        const DeviceGrid& grid, const int Fs,
        short ******sblock_pattern, const std::vector<vtr::Matrix<int>>&Fc_out,
        vtr::NdMatrix<int, 3>& Fc_xofs, vtr::NdMatrix<int, 3>& Fc_yofs,
        const t_rr_node_indices& L_rr_node_indices,
        const int max_chan_width,
        const int wire_to_ipin_switch,
        const int delayless_switch,
        const enum e_directionality directionality,
        bool * Fc_clipped,
        const t_direct_inf *directs, const int num_directs, const t_clb_to_clb_directs *clb_to_clb_directs);

static void load_uniform_connection_block_pattern(
        vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
        const std::vector<t_pin_loc>& pin_locations,
        const int x_chan_width, const int y_chan_width, const int Fc,
        const enum e_directionality directionality);

static void load_perturbed_connection_block_pattern(
        vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
        const std::vector<t_pin_loc>& pin_locations,
        const int x_chan_width, const int y_chan_width, const int Fc,
        const enum e_directionality directionality);

static std::vector<bool> alloc_and_load_perturb_opins(const t_type_ptr type, const vtr::Matrix<int>& Fc_out, const int max_chan_width,
        const int num_seg_types, const t_segment_inf *segment_inf);

#ifdef ENABLE_CHECK_ALL_TRACKS
static void check_all_tracks_reach_pins(
        t_type_ptr type,
        int *****tracks_connected_to_pin,
        int max_chan_width, int Fc,
        enum e_pin_type ipin_or_opin);
#endif

static std::vector<std::vector<bool>> alloc_and_load_perturb_ipins(
        const int L_num_types,
        const int num_seg_types,
        const int *sets_per_seg_type,
        const std::vector<vtr::Matrix<int>>&Fc_in, const std::vector<vtr::Matrix<int>>&Fc_out,
        const enum e_directionality directionality);

static void build_rr_sinks_sources(
        const int i, const int j,
        std::vector<t_rr_node>& L_rr_node, const t_rr_node_indices& L_rr_node_indices,
        const int delayless_switch, const DeviceGrid& grid);

static void build_rr_chan(
        const int i, const int j, const t_rr_type chan_type,
        const t_track_to_pin_lookup& track_to_pin_lookup,
        t_sb_connection_map *sb_conn_map,
        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn, const int cost_index_offset,
        const int max_chan_width,
        const DeviceGrid& grid,
        const int tracks_per_chan,
        short ******sblock_pattern, const int Fs_per_side,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        const t_rr_node_indices& L_rr_node_indices,
        t_rr_edge_info_set& created_rr_edges,
        std::vector<t_rr_node>& L_rr_node,
        const int wire_to_ipin_switch,
        const enum e_directionality directionality);

static int alloc_and_load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos,
                                        const int wire_to_arch_ipin_switch, int *wire_to_rr_ipin_switch);

static void remap_rr_node_switch_indices(map<int, int> *switch_fanin);

static void load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos, map<int, int> *switch_fanin);

static int alloc_rr_switch_inf(map<int, int> *switch_fanin);

static void rr_graph_externals(
        const t_segment_inf * segment_inf, int num_seg_types, int max_chan_width,
        int wire_to_rr_ipin_switch, enum e_base_cost_type base_cost_type);

void alloc_and_load_edges_and_switches(std::vector<t_rr_node>& L_rr_node,
        t_rr_edge_info_set& created_rr_edges,
        const t_rr_edge_info_set& rr_edges_to_create);

static t_clb_to_clb_directs *alloc_and_load_clb_to_clb_directs(const t_direct_inf *directs, const int num_directs,
        const int delayless_switch);

static void free_type_track_to_pin_map(
        t_track_to_pin_lookup& track_to_pin_map,
        t_type_ptr types, int max_chan_width);

static t_seg_details *alloc_and_load_global_route_seg_details(
        const int global_route_switch,
        int * num_seg_details = nullptr);

static std::vector<vtr::Matrix<int>> alloc_and_load_actual_fc(const int L_num_types, const t_type_ptr types, const int max_pins,
        const int num_seg_types,
        const t_segment_inf * segment_inf,
        const int *sets_per_seg_type,
        const int max_chan_width, const e_fc_type fc_type,
        const enum e_directionality directionality,
        bool *Fc_clipped);

static void build_rr_graph(
        const t_graph_type graph_type, const int L_num_types,
        const t_type_ptr types,
        const DeviceGrid& grid,
        t_chan_width *nodes_per_chan,
        const enum e_switch_block_type sb_type, const int Fs,
        const vector<t_switchblock_inf> switchblocks,
        const int num_seg_types, const int num_arch_switches,
        const t_segment_inf * segment_inf,
        const int global_route_switch,
        const int wire_to_arch_ipin_switch,
        const int delayless_switch,
        const float R_minW_nmos,
        const float R_minW_pmos,
        const enum e_base_cost_type base_cost_type,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        const t_direct_inf *directs, const int num_directs,
        int *wire_to_rr_ipin_switch,
        int *num_rr_switches,
        int *Warnings);

/******************* Subroutine definitions *******************************/

void create_rr_graph(
        const t_graph_type graph_type,
        const int num_block_types, const t_type_ptr block_types,
        const DeviceGrid& grid,
        t_chan_width *nodes_per_chan,
        const int num_arch_switches,
        t_det_routing_arch* det_routing_arch,
        const t_segment_inf * segment_inf,
        const enum e_base_cost_type base_cost_type,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        const t_direct_inf *directs, const int num_directs,
        int *num_rr_switches,
        int *Warnings) {

    if (!det_routing_arch->read_rr_graph_filename.empty()) {
        load_rr_file(
                graph_type,
                grid,
                nodes_per_chan,
                det_routing_arch->num_segment,
                segment_inf,
                base_cost_type,
                &det_routing_arch->wire_to_rr_ipin_switch,
                num_rr_switches,
                det_routing_arch->read_rr_graph_filename.c_str());
    } else {
        build_rr_graph(
                graph_type,
                num_block_types, block_types,
                grid,
                nodes_per_chan,
                det_routing_arch->switch_block_type,
                det_routing_arch->Fs,
                det_routing_arch->switchblocks,
                det_routing_arch->num_segment,
                num_arch_switches,
                segment_inf,
                det_routing_arch->global_route_switch,
                det_routing_arch->wire_to_arch_ipin_switch,
                det_routing_arch->delayless_switch,
                det_routing_arch->R_minW_nmos,
                det_routing_arch->R_minW_pmos,
                base_cost_type,
                trim_empty_channels,
                trim_obs_channels,
                directs, num_directs,
                &det_routing_arch->wire_to_rr_ipin_switch,
                num_rr_switches,
                Warnings);
    }

    //Write out rr graph file if needed
    if (!det_routing_arch->write_rr_graph_filename.empty()) {
        write_rr_graph(det_routing_arch->write_rr_graph_filename.c_str(), segment_inf, det_routing_arch->num_segment);
    }

}

static void build_rr_graph(
        const t_graph_type graph_type,
        const int L_num_types,
        const t_type_ptr types,
        const DeviceGrid& grid,
        t_chan_width *nodes_per_chan,
        const enum e_switch_block_type sb_type,
        const int Fs,
        const vector<t_switchblock_inf> switchblocks,
        const int num_seg_types,
        const int num_arch_switches,
        const t_segment_inf * segment_inf,
        const int global_route_switch,
        const int wire_to_arch_ipin_switch,
        const int delayless_switch,
        const float R_minW_nmos,
        const float R_minW_pmos,
        const enum e_base_cost_type base_cost_type,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        const t_direct_inf *directs,
        const int num_directs,
        int *wire_to_rr_ipin_switch,
        int *num_rr_switches,
        int *Warnings) {

    vtr::ScopedStartFinishTimer timer("Build routing resource graph");

    /* Reset warning flag */
    *Warnings = RR_GRAPH_NO_WARN;

    /* Decode the graph_type */
    bool is_global_graph = (GRAPH_GLOBAL == graph_type ? true : false);
    bool use_full_seg_groups = (GRAPH_UNIDIR_TILEABLE == graph_type ? true : false);
    enum e_directionality directionality = (GRAPH_BIDIR == graph_type ? BI_DIRECTIONAL : UNI_DIRECTIONAL);
    if (is_global_graph) {
        directionality = BI_DIRECTIONAL;
    }

    /* Global routing uses a single longwire track */
    int max_chan_width = (is_global_graph ? 1 : nodes_per_chan->max);
    VTR_ASSERT(max_chan_width > 0);

    auto& device_ctx = g_vpr_ctx.mutable_device();

    t_clb_to_clb_directs *clb_to_clb_directs = nullptr;
    if (num_directs > 0) {
        clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, num_directs, delayless_switch);
    }

    /* START SEG_DETAILS */
    int num_seg_details = 0;
    t_seg_details *seg_details = nullptr;

    if (is_global_graph) {
        /* Sets up a single unit length segment type for global routing. */
        seg_details = alloc_and_load_global_route_seg_details(
                global_route_switch, &num_seg_details);
    } else {
        /* Setup segments including distrubuting tracks and staggering.
         * If use_full_seg_groups is specified, max_chan_width may be
         * changed. Warning should be singled to caller if this happens. */
        size_t max_dim = std::max(grid.width(), grid.height()) - 2; //-2 for no perim channels

        seg_details = alloc_and_load_seg_details(&max_chan_width,
                max_dim, num_seg_types, segment_inf,
                use_full_seg_groups, is_global_graph, directionality,
                &num_seg_details);
        if ((is_global_graph ? 1 : nodes_per_chan->max) != max_chan_width) {
            nodes_per_chan->max = max_chan_width;
            *Warnings |= RR_GRAPH_WARN_CHAN_WIDTH_CHANGED;
        }

        if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SEG_DETAILS)) {
            dump_seg_details(seg_details, max_chan_width,
                    getEchoFileName(E_ECHO_SEG_DETAILS));
        }
    }
    /* END SEG_DETAILS */

    /* START CHAN_DETAILS */
    t_chan_details chan_details_x;
    t_chan_details chan_details_y;

    alloc_and_load_chan_details(grid, nodes_per_chan,
            trim_empty_channels, trim_obs_channels,
            num_seg_details, seg_details,
            chan_details_x, chan_details_y);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CHAN_DETAILS)) {
        dump_chan_details(chan_details_x, chan_details_y, max_chan_width, grid,
                getEchoFileName(E_ECHO_CHAN_DETAILS));
    }
    /* END CHAN_DETAILS */

    /* START FC */
    /* Determine the actual value of Fc */
    std::vector<vtr::Matrix<int>> Fc_in; /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1] */
    std::vector<vtr::Matrix<int>> Fc_out; /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..num_segments-1] */

    /* get maximum number of pins across all blocks */
    int max_pins = types[0].num_pins;
    for (int i = 1; i < L_num_types; ++i) {
        if (types[i].num_pins > max_pins) {
            max_pins = types[i].num_pins;
        }
    }

    /* get the number of 'sets' for each segment type -- unidirectial architectures have two tracks in a set, bidirectional have one */
    int total_sets = max_chan_width;
    if (directionality == UNI_DIRECTIONAL) {
        VTR_ASSERT(max_chan_width % 2 == 0);
        total_sets /= 2;
    }
    int *sets_per_seg_type = get_seg_track_counts(total_sets, num_seg_types, segment_inf, use_full_seg_groups);

    if (is_global_graph) {
        //All pins can connect during global routing
        auto ones = vtr::Matrix<int>({size_t(max_pins), size_t(num_seg_types)}, 1);
        Fc_in = std::vector<vtr::Matrix<int>>(L_num_types, ones);
        Fc_out = std::vector<vtr::Matrix<int>>(L_num_types, ones);
    } else {
        bool Fc_clipped = false;
        Fc_in = alloc_and_load_actual_fc(L_num_types, types, max_pins, num_seg_types, segment_inf, sets_per_seg_type, max_chan_width,
                e_fc_type::IN, directionality, &Fc_clipped);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }
        Fc_clipped = false;
        Fc_out = alloc_and_load_actual_fc(L_num_types, types, max_pins, num_seg_types, segment_inf, sets_per_seg_type, max_chan_width,
                e_fc_type::OUT, directionality, &Fc_clipped);
        if (Fc_clipped) {
            *Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
        }

        for (int i = 1; i < L_num_types; ++i) { /* Skip "EMPTY" */
            for (int j = 0; j < device_ctx.block_types[i].num_pins; ++j) {
                for (int k = 0; k < num_seg_types; k++) {
#ifdef VERBOSE
                    VTR_LOG("Fc Actual Values: type = %s, pin = %d (%s), "
                        "seg = %d (%s), Fc_out = %d, Fc_in = %d.\n",
                        device_ctx.block_types[i].name,
                        j,
                        block_type_pin_index_to_name(&device_ctx.block_types[i], j).c_str(),
                        k,
                        segment_inf[k].name,
                        Fc_out[i][j][k],
                        Fc_in[i][j][k]);
#endif /* VERBOSE */
                    VTR_ASSERT_MSG(Fc_out[i][j][k] == 0 || Fc_in[i][j][k] == 0,
                            "Pins must be inputs or outputs (i.e. can not have both non-zero Fc_out and Fc_in)");
                }
            }
        }
    }

    auto perturb_ipins = alloc_and_load_perturb_ipins(L_num_types, num_seg_types,
            sets_per_seg_type, Fc_in, Fc_out, directionality);
    /* END FC */

    /* Alloc node lookups, count nodes, alloc rr nodes */
    int num_rr_nodes = 0;

    device_ctx.rr_node_indices = alloc_and_load_rr_node_indices(max_chan_width, grid,
            &num_rr_nodes, chan_details_x, chan_details_y);
    device_ctx.rr_nodes.resize(num_rr_nodes);

    /* These are data structures used by the the unidir opin mapping. They are used
       to spread connections evenly for each segment type among the available
       wire start points */
    vtr::NdMatrix<int, 3> Fc_xofs({
                                    grid.height() - 1,
                                    grid.width() - 1,
                                    size_t(num_seg_types)
                                  },
                                  0); //[0..grid.height()-2][0..grid.width()-2][0..num_seg_types-1]
    vtr::NdMatrix<int, 3> Fc_yofs({
                                    grid.width() - 1,
                                    grid.height() - 1,
                                    size_t(num_seg_types)
                                  },
                                  0); //[0..grid.width()-2][0..grid.height()-2][0..num_seg_types-1]

    /* START SB LOOKUP */
    /* Alloc and load the switch block lookup */
    vtr::NdMatrix<std::vector<int>, 3> switch_block_conn;
    short ******unidir_sb_pattern = nullptr;
    t_sb_connection_map *sb_conn_map = nullptr; //for custom switch blocks

    if (is_global_graph) {
        switch_block_conn = alloc_and_load_switch_block_conn(1, SUBSET, 3);
    } else if (BI_DIRECTIONAL == directionality) {
        if (sb_type == CUSTOM) {
            sb_conn_map = alloc_and_load_switchblock_permutations(chan_details_x, chan_details_y,
                    grid,
                    switchblocks, nodes_per_chan, directionality);
        } else {
            switch_block_conn = alloc_and_load_switch_block_conn(max_chan_width, sb_type, Fs);
        }
    } else {
        VTR_ASSERT(UNI_DIRECTIONAL == directionality);

        if (sb_type == CUSTOM) {
            sb_conn_map = alloc_and_load_switchblock_permutations(chan_details_x, chan_details_y,
                    grid,
                    switchblocks, nodes_per_chan, directionality);
        } else {
            /* it looks like we get unbalanced muxing from this switch block code with Fs > 3 */
            VTR_ASSERT(Fs == 3);

            unidir_sb_pattern = alloc_sblock_pattern_lookup(grid, max_chan_width);
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
    /* END SB LOOKUP */

    /* START IPIN MAP */
    /* Create ipin map lookups */

    t_pin_to_track_lookup ipin_to_track_map(L_num_types); /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
    t_track_to_pin_lookup track_to_pin_lookup(L_num_types); /* [0..device_ctx.num_block_types-1][0..max_chan_width-1][0..width][0..height][0..3] */

    for (int itype = 0; itype < L_num_types; ++itype) {

        ipin_to_track_map[itype] = alloc_and_load_pin_to_track_map(RECEIVER,
                Fc_in[itype], &types[itype], perturb_ipins[itype], directionality,
                num_seg_types, sets_per_seg_type);


        track_to_pin_lookup[itype] = alloc_and_load_track_to_pin_lookup(
                ipin_to_track_map[itype], Fc_in[itype], types[itype].width, types[itype].height,
                types[itype].num_pins, max_chan_width, num_seg_types);
    }
    /* END IPIN MAP */

    /* START OPIN MAP */
    /* Create opin map lookups */
    t_pin_to_track_lookup opin_to_track_map(L_num_types); /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */

    if (BI_DIRECTIONAL == directionality) {
        for (int i = 0; i < L_num_types; ++i) {
            auto perturb_opins = alloc_and_load_perturb_opins(&types[i], Fc_out[i],
                    max_chan_width, num_seg_types, segment_inf);
            opin_to_track_map[i] = alloc_and_load_pin_to_track_map(DRIVER,
                    Fc_out[i], &types[i], perturb_opins, directionality,
                    num_seg_types, sets_per_seg_type);
        }
    }
    /* END OPIN MAP */

    bool Fc_clipped = false;
    alloc_and_load_rr_graph(device_ctx.rr_nodes.size(), device_ctx.rr_nodes, num_seg_types,
            seg_details, chan_details_x, chan_details_y,
            track_to_pin_lookup, opin_to_track_map,
            switch_block_conn, sb_conn_map, grid, Fs, unidir_sb_pattern,
            Fc_out, Fc_xofs, Fc_yofs, device_ctx.rr_node_indices, max_chan_width,
            wire_to_arch_ipin_switch,
            delayless_switch,
            directionality,
            &Fc_clipped,
            directs, num_directs, clb_to_clb_directs);

    /* Update rr_nodes capacities if global routing */
    if (graph_type == GRAPH_GLOBAL) {
        for (size_t i = 0; i < device_ctx.rr_nodes.size(); i++) {
            if (device_ctx.rr_nodes[i].type() == CHANX) {
                int ylow = device_ctx.rr_nodes[i].ylow();
                device_ctx.rr_nodes[i].set_capacity(device_ctx.chan_width.x_list[ylow]);
            }
            if (device_ctx.rr_nodes[i].type() == CHANY) {
                int xlow = device_ctx.rr_nodes[i].xlow();
                device_ctx.rr_nodes[i].set_capacity(device_ctx.chan_width.y_list[xlow]);
            }
        }
    }

    /* Allocate and load routing resource switches, which are derived from the switches from the architecture file,
       based on their fanin in the rr graph. This routine also adjusts the rr nodes to point to these new rr switches */
    (*num_rr_switches) = alloc_and_load_rr_switch_inf(num_arch_switches, R_minW_nmos, R_minW_pmos, wire_to_arch_ipin_switch, wire_to_rr_ipin_switch);

    //Partition the rr graph edges for efficient access to configurable/non-configurable
    //edge subsets. Must be done after RR switches have been allocated
    partition_rr_graph_edges(device_ctx);

    rr_graph_externals(segment_inf, num_seg_types, max_chan_width,
            *wire_to_rr_ipin_switch, base_cost_type);


    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH)) {
        dump_rr_graph(getEchoFileName(E_ECHO_RR_GRAPH));
    }


    check_rr_graph(graph_type, grid, *num_rr_switches, types);

#ifdef USE_MAP_LOOKAHEAD
    compute_router_lookahead(num_seg_types);
#endif

    /* Free all temp structs */
    if (seg_details) {
        free_seg_details(seg_details, max_chan_width);
        seg_details = nullptr;
    }
    if (!chan_details_x.empty() || !chan_details_y.empty()) {
        free_chan_details(chan_details_x, chan_details_y, max_chan_width, grid);
    }
    if (sb_conn_map) {
        free_switchblock_permutations(sb_conn_map);
        sb_conn_map = nullptr;
    }
    if (unidir_sb_pattern) {
        free_sblock_pattern_lookup(unidir_sb_pattern);
        unidir_sb_pattern = nullptr;
    }
    if (sets_per_seg_type) {
        free(sets_per_seg_type);
        sets_per_seg_type = nullptr;
    }

    free_type_track_to_pin_map(track_to_pin_lookup, types, max_chan_width);
    if (clb_to_clb_directs != nullptr) {
        free(clb_to_clb_directs);
    }
}

/* Allocates and loads the global rr_switch_inf array based on the global
   arch_switch_inf array and the fan-ins used by the rr nodes.
   Also changes switch indices of rr_nodes to index into rr_switch_inf
   instead of arch_switch_inf.

   Returns the number of rr switches created.
   Also returns, through a pointer, the index of a representative ipin cblock switch.
        - Currently we're not allowing a designer to specify an ipin cblock switch with
          multiple fan-ins, so there's just one of these switches in the device_ctx.rr_switch_inf array.
          But in the future if we allow this, we can return an index to a representative switch

   The rr_switch_inf switches are derived from the arch_switch_inf switches
   (which were read-in from the architecture file) based on fan-in. The delays of
   the rr switches depend on their fan-in, so we first go through the rr_nodes
   and count how many different fan-ins exist for each arch switch.
   Then we create these rr switches and update the switch indices
   of rr_nodes to index into the rr_switch_inf array. */
static int alloc_and_load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos,
                                        const int wire_to_arch_ipin_switch, int *wire_to_rr_ipin_switch) {
    /* we will potentially be creating a couple of versions of each arch switch where
       each version corresponds to a different fan-in. We will need to fill device_ctx.rr_switch_inf
       with this expanded list of switches.
       To do this we will use an array of maps where each map corresponds to a different arch switch.
       So for each arch switch we will use this map to keep track of the different fan-ins that it uses (map key)
       and which index in the device_ctx.rr_switch_inf array this arch switch / fanin combination will be placed in */
    map<int, int> *switch_fanin = new map<int, int>[num_arch_switches];

    /* Determine what the different fan-ins are for each arch switch, and also
       how many entries the rr_switch_inf array should have */
    int num_rr_switches = alloc_rr_switch_inf(switch_fanin);

    /* create the rr switches. also keep track of, for each arch switch, what index of the rr_switch_inf
       array each version of its fanin has been mapped to */
    load_rr_switch_inf(num_arch_switches, R_minW_nmos, R_minW_pmos, switch_fanin);

    /* next, walk through rr nodes again and remap their switch indices to rr_switch_inf */
    remap_rr_node_switch_indices(switch_fanin);

    /* now we need to set the wire_to_rr_ipin_switch variable which points the detailed routing architecture
       to the representative ipin cblock switch. currently we're not allowing the specification of an ipin cblock switch
       with multiple fan-ins, so right now there's just one. May change in the future, in which case we'd need to
       return a representative switch */
    if (switch_fanin[wire_to_arch_ipin_switch].count(UNDEFINED)) {
        /* only have one ipin cblock switch. OK. */
        (*wire_to_rr_ipin_switch) = switch_fanin[wire_to_arch_ipin_switch][UNDEFINED];
    } else if (switch_fanin[wire_to_arch_ipin_switch].size() != 0) {
        vpr_throw(VPR_ERROR_ARCH, __FILE__, __LINE__,
                "Not currently allowing an ipin cblock switch to have multiple fan-ins");
    } else {
        //This likely indicates that no connection block has been constructed, indicating significant issues with
        //the generated RR graph.
        //
        //Instead of throwing an error we issue a warning. This means that check_rr_graph() etc. will run to give more information
        //and allow graphics to be brought up for users to debug their architectures.
        (*wire_to_rr_ipin_switch) = OPEN;
        VTR_LOG_WARN(
                "No switch found for the ipin cblock in RR graph. Check if there is an error in arch file, or if no connection blocks are being built in RR graph\n");
    }

    delete[] switch_fanin;

    return num_rr_switches;
}

/* Allocates space for the global device_ctx.rr_switch_inf variable and returns the
   number of rr switches that were allocated */
static int alloc_rr_switch_inf(map<int, int> *switch_fanin) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    int num_rr_switches = 0;
    // map key: switch index specified in arch; map value: fanin for that index
    map<int, int> *inward_switch_inf = new map<int, int>[device_ctx.rr_nodes.size()];
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        const t_rr_node& from_node = device_ctx.rr_nodes[inode];
        int num_edges = from_node.num_edges();
        for (int iedge = 0; iedge < num_edges; iedge++) {
            int switch_index = from_node.edge_switch(iedge);
            int to_node_index = from_node.edge_sink_node(iedge);
            if (inward_switch_inf[to_node_index].count(switch_index) == 0)
                inward_switch_inf[to_node_index][switch_index] = 0;
            inward_switch_inf[to_node_index][switch_index]++;
        }
    }

    // get unique index / fanin combination based on inward_switch_inf
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        map<int, int>::iterator itr;
        for (itr = inward_switch_inf[inode].begin(); itr != inward_switch_inf[inode].end(); itr++) {
            int switch_index = itr->first;
            int fanin = itr->second;
            if (device_ctx.arch_switch_inf[switch_index].fixed_Tdel()) {
                fanin = UNDEFINED;
            }
            if (switch_fanin[switch_index].count(fanin) == 0) {
                switch_fanin[switch_index][fanin] = 0;
                num_rr_switches++;
            }
        }
    }
    delete[] inward_switch_inf;

    /* allocate space for the rr_switch_inf array (it's freed later in vpr_api.c-->free_arch) */
    device_ctx.rr_switch_inf = new t_rr_switch_inf[num_rr_switches];

    return num_rr_switches;
}

/* load the global device_ctx.rr_switch_inf variable. also keep track of, for each arch switch, what
   index of the rr_switch_inf array each version of its fanin has been mapped to (through switch_fanin map) */
static void load_rr_switch_inf(const int num_arch_switches, const float R_minW_nmos, const float R_minW_pmos, map<int, int> *switch_fanin) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    int i_rr_switch = 0;
    if (device_ctx.switch_fanin_remap != nullptr) {
        // at this stage, we rebuild the rr_graph (probably in binary search)
        // so old device_ctx.switch_fanin_remap is obsolete
        delete [] device_ctx.switch_fanin_remap;
    }
    device_ctx.switch_fanin_remap = new map<int, int>[num_arch_switches];
    for (int i_arch_switch = 0; i_arch_switch < num_arch_switches; i_arch_switch++) {
        map<int, int>::iterator it;
        for (it = switch_fanin[i_arch_switch].begin(); it != switch_fanin[i_arch_switch].end(); it++) {
            /* the fanin value is in it->first, and we'll need to set what index this i_arch_switch/fanin
               combination maps to (within rr_switch_inf) in it->second) */
            int fanin = it->first;
            it->second = i_rr_switch;
            // setup device_ctx.switch_fanin_remap, for future swich usage analysis
            device_ctx.switch_fanin_remap[i_arch_switch][fanin] = i_rr_switch;

            /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
               rr switch should be */
            double rr_switch_Tdel = device_ctx.arch_switch_inf[i_arch_switch].Tdel(fanin);

            /* copy over the arch switch to rr_switch_inf[i_rr_switch], but with the changed Tdel value */
            device_ctx.rr_switch_inf[i_rr_switch].set_type(device_ctx.arch_switch_inf[i_arch_switch].type());
            device_ctx.rr_switch_inf[i_rr_switch].R = device_ctx.arch_switch_inf[i_arch_switch].R;
            device_ctx.rr_switch_inf[i_rr_switch].Cin = device_ctx.arch_switch_inf[i_arch_switch].Cin;
            device_ctx.rr_switch_inf[i_rr_switch].Cout = device_ctx.arch_switch_inf[i_arch_switch].Cout;
            device_ctx.rr_switch_inf[i_rr_switch].Tdel = rr_switch_Tdel;
            device_ctx.rr_switch_inf[i_rr_switch].mux_trans_size = device_ctx.arch_switch_inf[i_arch_switch].mux_trans_size;
            if (device_ctx.arch_switch_inf[i_arch_switch].buf_size_type == BufferSize::AUTO) {
                //Size based on resistance
                device_ctx.rr_switch_inf[i_rr_switch].buf_size = trans_per_buf(device_ctx.arch_switch_inf[i_arch_switch].R, R_minW_nmos, R_minW_pmos);
            } else {
                VTR_ASSERT(device_ctx.arch_switch_inf[i_arch_switch].buf_size_type == BufferSize::ABSOLUTE);
                //Use the specified size
                device_ctx.rr_switch_inf[i_rr_switch].buf_size = device_ctx.arch_switch_inf[i_arch_switch].buf_size;
            }
            device_ctx.rr_switch_inf[i_rr_switch].name = device_ctx.arch_switch_inf[i_arch_switch].name;
            device_ctx.rr_switch_inf[i_rr_switch].power_buffer_type = device_ctx.arch_switch_inf[i_arch_switch].power_buffer_type;
            device_ctx.rr_switch_inf[i_rr_switch].power_buffer_size = device_ctx.arch_switch_inf[i_arch_switch].power_buffer_size;

            /* have created a switch in the rr_switch_inf array */
            i_rr_switch++;
        }
    }
}

/* switch indices of each rr_node original point into the global device_ctx.arch_switch_inf array.
   now we want to remap these indices to point into the global device_ctx.rr_switch_inf array
   which contains switch info at different fan-in values */
static void remap_rr_node_switch_indices(map<int, int> *switch_fanin) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& from_node = device_ctx.rr_nodes[inode];
        int num_edges = from_node.num_edges();
        for (int iedge = 0; iedge < num_edges; iedge++) {
            const t_rr_node& to_node = device_ctx.rr_nodes[ from_node.edge_sink_node(iedge) ];
            /* get the switch which this edge uses and its fanin */
            int switch_index = from_node.edge_switch(iedge);
            int fanin = to_node.fan_in();

            if (switch_fanin[switch_index].count(UNDEFINED) == 1) {
                fanin = UNDEFINED;
            }

            int rr_switch_index = switch_fanin[switch_index][fanin];

            from_node.set_edge_switch(iedge, rr_switch_index);
        }
    }
}

static void rr_graph_externals(
        const t_segment_inf * segment_inf, int num_seg_types, int max_chan_width,
        int wire_to_rr_ipin_switch, enum e_base_cost_type base_cost_type) {
    auto& device_ctx = g_vpr_ctx.device();

    add_rr_graph_C_from_switches(device_ctx.rr_switch_inf[wire_to_rr_ipin_switch].Cin);
    alloc_and_load_rr_indexed_data(segment_inf, num_seg_types, device_ctx.rr_node_indices,
            max_chan_width, wire_to_rr_ipin_switch, base_cost_type);
    load_rr_index_segments(num_seg_types);
}

static std::vector<std::vector<bool>> alloc_and_load_perturb_ipins(const int L_num_types,
        const int num_seg_types,
        const int *sets_per_seg_type,
        const std::vector<vtr::Matrix<int>>&Fc_in, const std::vector<vtr::Matrix<int>>&Fc_out,
        const enum e_directionality directionality) {

    std::vector < std::vector<bool>> result(L_num_types);
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
                    Fc_ratio = (float) Fc_in[itype][0][iseg] / (float) Fc_out[itype][0][iseg];
                } else {
                    Fc_ratio = (float) Fc_out[itype][0][iseg] / (float) Fc_in[itype][0][iseg];
                }

                if ((Fc_in[itype][0][iseg] <= tracks_in_seg_type - 2)
                        && (fabs(Fc_ratio - vtr::nint(Fc_ratio))
                        < (0.5 / (float) tracks_in_seg_type))) {
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

static t_seg_details *alloc_and_load_global_route_seg_details(
        const int global_route_switch,
        int * num_seg_details) {

    t_seg_details *seg_details = (t_seg_details *) vtr::malloc(sizeof (t_seg_details));

    seg_details->index = 0;
    seg_details->length = 1;
    seg_details->arch_wire_switch = global_route_switch;
    seg_details->arch_opin_switch = global_route_switch;
    seg_details->longline = false;
    seg_details->direction = BI_DIRECTION;
    seg_details->Cmetal = 0.0;
    seg_details->Rmetal = 0.0;
    seg_details->start = 1;
    seg_details->cb = (bool *) vtr::malloc(sizeof (bool) * 1);
    seg_details->cb[0] = true;
    seg_details->sb = (bool *) vtr::malloc(sizeof (bool) * 2);
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
static std::vector<vtr::Matrix<int>> alloc_and_load_actual_fc(const int L_num_types, const t_type_ptr types, const int max_pins,
        const int num_seg_types,
        const t_segment_inf * segment_inf,
        const int *sets_per_seg_type,
        const int max_chan_width, const e_fc_type fc_type,
        const enum e_directionality directionality,
        bool *Fc_clipped) {

    //Initialize Fc of all blocks to zero
    auto zeros = vtr::Matrix<int>({size_t(max_pins), size_t(num_seg_types)}, 0);
    std::vector<vtr::Matrix<int>> Fc(L_num_types, zeros);

    *Fc_clipped = false;

    /* Unidir tracks formed in pairs, otherwise no effect. */
    int fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    VTR_ASSERT((max_chan_width % fac) == 0);

    for (int itype = 1; itype < L_num_types; ++itype) { //Skip EMPTY
        for (const t_fc_specification& fc_spec : types[itype].fc_specs) {

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
                        VPR_THROW(VPR_ERROR_ROUTE, "Absolute Fc value must be a multiple of %d (was %f) between block pin '%s' and wire segment '%s'",
                                                    fac, fc_spec.fc_value,
                                                    block_type_pin_index_to_name(&types[itype], fc_spec.pins[0]).c_str(),
                                                    segment_inf[iseg].name);
                    }

                    if (fc_spec.fc_value < fac) {
                        VPR_THROW(VPR_ERROR_ROUTE, "Absolute Fc value must be at least %d (was %f) between block pin '%s' to wire segment %s",
                                                    fac, fc_spec.fc_value,
                                                    block_type_pin_index_to_name(&types[itype], fc_spec.pins[0]).c_str(),
                                                    segment_inf[iseg].name);
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

/* frees the track to ipin mapping for each physical grid type */
static void free_type_track_to_pin_map(t_track_to_pin_lookup& track_to_pin_map,
        t_type_ptr types, int max_chan_width) {
    auto& device_ctx = g_vpr_ctx.device();

    for (int i = 0; i < device_ctx.num_block_types; i++) {
        if (!track_to_pin_map[i].empty()) {
            for (int track = 0; track < max_chan_width; ++track) {
                for (int width = 0; width < types[i].width; ++width) {
                    for (int height = 0; height < types[i].height; ++height) {
                        for (int side = 0; side < 4; ++side) {
                            if (!track_to_pin_map[i][track][width][height][side].empty()) {
                                track_to_pin_map[i][track][width][height][side].clear();
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */
static void alloc_and_load_rr_graph(const int num_nodes,
        std::vector<t_rr_node>& L_rr_node, const int num_seg_types,
        const t_seg_details * seg_details,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        const t_track_to_pin_lookup& track_to_pin_lookup,
        const t_pin_to_track_lookup& opin_to_track_map, const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
        t_sb_connection_map *sb_conn_map,
        const DeviceGrid& grid, const int Fs,
        short ******sblock_pattern, const std::vector<vtr::Matrix<int>>&Fc_out,
        vtr::NdMatrix<int, 3>& Fc_xofs, vtr::NdMatrix<int, 3>& Fc_yofs,
        const t_rr_node_indices& L_rr_node_indices,
        const int max_chan_width,
        const int wire_to_ipin_switch,
        const int delayless_switch,
        const enum e_directionality directionality,
        bool * Fc_clipped,
        const t_direct_inf *directs, const int num_directs,
        const t_clb_to_clb_directs *clb_to_clb_directs) {

    //Track which edges have been created to avoid duplicates
    t_rr_edge_info_set created_rr_edges;

    /* If Fc gets clipped, this will be flagged to true */
    *Fc_clipped = false;

    /* Connection SINKS and SOURCES to their pins. */
    for (size_t i = 0; i < grid.width(); ++i) {
        for (size_t j = 0; j < grid.height(); ++j) {
            build_rr_sinks_sources(i, j, L_rr_node, L_rr_node_indices,
                    delayless_switch, grid);
        }
    }

    /* Build opins */
    for (size_t i = 0; i < grid.width(); ++i) {
        for (size_t j = 0; j < grid.height(); ++j) {
            for (e_side side : SIDES) {
                if (BI_DIRECTIONAL == directionality) {
                    build_bidir_rr_opins(i, j, side, L_rr_node, L_rr_node_indices,
                            opin_to_track_map, Fc_out, created_rr_edges, seg_details,
                            grid,
                            directs, num_directs, clb_to_clb_directs, num_seg_types);
                } else {
                    VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                    bool clipped;
                    build_unidir_rr_opins(i, j, side, grid, Fc_out, max_chan_width,
                            chan_details_x, chan_details_y, Fc_xofs, Fc_yofs,
                            created_rr_edges, &clipped, L_rr_node_indices,
                            directs, num_directs, clb_to_clb_directs, num_seg_types);
                    if (clipped) {
                        *Fc_clipped = true;
                    }
                }
            }
        }
    }

    /* We make a copy of the current fanin values for the nodes to
     * know the number of OPINs driving each mux presently */
    int *opin_mux_size = (int *) vtr::malloc(sizeof (int) * num_nodes);
    for (int i = 0; i < num_nodes; ++i) {
        opin_mux_size[i] = L_rr_node[i].fan_in();
    }

    /* Build channels */
    auto& device_ctx = g_vpr_ctx.device();
    VTR_ASSERT(Fs % 3 == 0);
    for (size_t i = 0; i < grid.width() - 1; ++i) {
        for (size_t j = 0; j < grid.height() - 1; ++j) {
            if (i > 0) {
                build_rr_chan(i, j, CHANX, track_to_pin_lookup, sb_conn_map, switch_block_conn,
                        CHANX_COST_INDEX_START,
                        max_chan_width, grid, device_ctx.chan_width.x_list[j],
                        sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                        L_rr_node_indices, created_rr_edges, L_rr_node,
                        wire_to_ipin_switch,
                        directionality);
            }
            if (j > 0) {
                build_rr_chan(i, j, CHANY, track_to_pin_lookup, sb_conn_map, switch_block_conn,
                        CHANX_COST_INDEX_START + num_seg_types,
                        max_chan_width, grid, device_ctx.chan_width.y_list[i],
                        sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
                        L_rr_node_indices, created_rr_edges, L_rr_node,
                        wire_to_ipin_switch,
                        directionality);
            }
        }
    }

    init_fan_in(L_rr_node, num_nodes);

    free(opin_mux_size);
}

static void build_bidir_rr_opins(const int i, const int j, const e_side side,
        std::vector<t_rr_node>& L_rr_node, const t_rr_node_indices& L_rr_node_indices,
        const t_pin_to_track_lookup& opin_to_track_map, const std::vector<vtr::Matrix<int>>&Fc_out,
        t_rr_edge_info_set& created_rr_edges,
        const t_seg_details * seg_details,
        const DeviceGrid& grid,
        const t_direct_inf *directs, const int num_directs, const t_clb_to_clb_directs *clb_to_clb_directs,
        const int num_seg_types) {

    //Don't connect pins which are not adjacent to channels around the perimeter
    if (   (i == 0 && side != RIGHT)
        || (i == int(grid.width() - 1) && side != LEFT)
        || (j == 0 && side != TOP)
        || (j == int(grid.width() - 1) && side != BOTTOM)) {
        return;
    }

    t_type_ptr type = grid[i][j].type;
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

        int node_index = get_rr_node_index(L_rr_node_indices, i, j, OPIN, pin_index, side);
        VTR_ASSERT(node_index >= 0);

        t_rr_edge_info_set rr_edges_to_create;
        if (total_pin_Fc > 0) {
                get_bidir_opin_connections(i, j, pin_index,
                        node_index, rr_edges_to_create, opin_to_track_map, L_rr_node_indices,
                        seg_details);
        }

        /* Add in direct connections */
        get_opin_direct_connecions(i, j, side, pin_index,
                node_index, rr_edges_to_create, L_rr_node_indices,
                directs, num_directs, clb_to_clb_directs);

        alloc_and_load_edges_and_switches(L_rr_node, created_rr_edges, rr_edges_to_create);
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

    device_ctx.rr_node_indices.clear();

    device_ctx.rr_nodes.clear();

    device_ctx.rr_node_indices.clear();

    free(device_ctx.rr_indexed_data);
    device_ctx.rr_indexed_data = nullptr;

    delete[] device_ctx.rr_switch_inf;
    device_ctx.rr_switch_inf = nullptr;
    device_ctx.num_rr_switches = 0;

    delete[] device_ctx.switch_fanin_remap;
    device_ctx.switch_fanin_remap = nullptr;
}

static void build_rr_sinks_sources(const int i, const int j,
        std::vector<t_rr_node>& L_rr_node, const t_rr_node_indices& L_rr_node_indices,
        const int delayless_switch, const DeviceGrid& grid) {

    /* Loads IPIN, SINK, SOURCE, and OPIN.
     * Loads IPIN to SINK edges, and SOURCE to OPIN edges */

    /* Since we share nodes within a large block, only
     * start tile can initialize sinks, sources, and pins */
    if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0)
        return;

    t_type_ptr type = grid[i][j].type;
    int num_class = type->num_class;
    t_class *class_inf = type->class_inf;
    int num_pins = type->num_pins;
    int *pin_class = type->pin_class;

    /* SINK and SOURCE-to-OPIN edges */
    for (int iclass = 0; iclass < num_class; ++iclass) {
        int inode = 0;
        if (class_inf[iclass].type == DRIVER) { /* SOURCE */
            inode = get_rr_node_index(L_rr_node_indices, i, j, SOURCE, iclass);

            //Retrieve all the physical OPINs associated with this source, this includes
            //those at different grid tiles of this block
            std::vector<int> opin_nodes;
            for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                    for (int ipin = 0; ipin < class_inf[iclass].num_pins; ++ipin) {
                        int pin_num = class_inf[iclass].pinlist[ipin];
                        auto physical_pins = get_rr_node_indices(L_rr_node_indices, i + width_offset, j + height_offset, OPIN, pin_num);
                        opin_nodes.insert(opin_nodes.end(), physical_pins.begin(), physical_pins.end());
                    }
                }
            }

            //Connect the SOURCE to each OPIN
            L_rr_node[inode].set_num_edges(opin_nodes.size());
            for (size_t iedge = 0; iedge < opin_nodes.size(); ++iedge) {
                L_rr_node[inode].set_edge_sink_node(iedge, opin_nodes[iedge]);
                L_rr_node[inode].set_edge_switch(iedge, delayless_switch);
            }

            L_rr_node[inode].set_cost_index(SOURCE_COST_INDEX);
            L_rr_node[inode].set_type(SOURCE);
        } else { /* SINK */
            VTR_ASSERT(class_inf[iclass].type == RECEIVER);
            inode = get_rr_node_index(L_rr_node_indices, i, j, SINK, iclass);

            /* NOTE:  To allow route throughs through clbs, change the lines below to  *
             * make an edge from the input SINK to the output SOURCE.  Do for just the *
             * special case of INPUTS = class 0 and OUTPUTS = class 1 and see what it  *
             * leads to.  If route throughs are allowed, you may want to increase the  *
             * base cost of OPINs and/or SOURCES so they aren't used excessively.      */

            /* Initialize to unconnected */
            L_rr_node[inode].set_num_edges(0);

            L_rr_node[inode].set_cost_index(SINK_COST_INDEX);
            L_rr_node[inode].set_type(SINK);
        }

        /* Things common to both SOURCEs and SINKs.   */
        L_rr_node[inode].set_capacity(class_inf[iclass].num_pins);
        L_rr_node[inode].set_coordinates(i, j, i + type->width - 1, j + type->height - 1);
        float R = 0.;
        float C = 0.;
        L_rr_node[inode].set_rc_index(find_create_rr_rc_data(R, C));
        L_rr_node[inode].set_ptc_num(iclass);
    }

    /* Connect IPINS to SINKS and initialize OPINS */
    //We loop through all the pin locations on the block to initialize the IPINs/OPINs,
    //and hook-up the IPINs to sinks.
    for (int width_offset = 0; width_offset < type->width; ++width_offset) {
        for (int height_offset = 0; height_offset < type->height; ++height_offset) {
            for (e_side side : {TOP, BOTTOM, LEFT, RIGHT}) {
                for (int ipin = 0; ipin < num_pins; ++ipin) {

                    if (type->pinloc[width_offset][height_offset][side][ipin]) {
                        int inode;
                        int iclass = pin_class[ipin];

                        if (class_inf[iclass].type == RECEIVER) {
                            //Connect the input pin to the sink
                            inode = get_rr_node_index(L_rr_node_indices, i + width_offset, j + height_offset, IPIN, ipin, side);
                            int to_node = get_rr_node_index(L_rr_node_indices, i, j, SINK, iclass);

                            L_rr_node[inode].set_num_edges(1);

                            L_rr_node[inode].set_edge_sink_node(0, to_node);
                            L_rr_node[inode].set_edge_switch(0, delayless_switch);

                            L_rr_node[inode].set_cost_index(IPIN_COST_INDEX);
                            L_rr_node[inode].set_type(IPIN);

                        } else {
                            VTR_ASSERT(class_inf[iclass].type == DRIVER);
                            //Initialize the output pin
                            // Note that we leave it's out-going edges unconnected (they will be hooked up to global routing later)
                            inode = get_rr_node_index(L_rr_node_indices, i + width_offset, j + height_offset, OPIN, ipin, side);

                            L_rr_node[inode].set_num_edges(0); //Initially unconnected
                            L_rr_node[inode].set_cost_index(OPIN_COST_INDEX);
                            L_rr_node[inode].set_type(OPIN);
                        }

                        /* Common to both DRIVERs and RECEIVERs */
                        L_rr_node[inode].set_capacity(1);
                        float R = 0.;
                        float C = 0.;
                        L_rr_node[inode].set_rc_index(find_create_rr_rc_data(R, C));
                        L_rr_node[inode].set_ptc_num(ipin);

                        //Note that we store the grid tile location and side where the pin is located,
                        //which greatly simplifies the drawing code
                        L_rr_node[inode].set_coordinates(i + width_offset, j + height_offset, i + width_offset, j + height_offset);
                        L_rr_node[inode].set_side(side);

                        VTR_ASSERT(type->pinloc[width_offset][height_offset][L_rr_node[inode].side()][L_rr_node[inode].pin_num()]);
                    }
                }
            }
        }
    }
}

void init_fan_in(std::vector<t_rr_node>& L_rr_node, const int num_rr_nodes) {
    //Loads fan-ins for all nodes

    //Reset all fan-ins to zero
    for (int i = 0; i < num_rr_nodes; i++) {
        L_rr_node[i].set_fan_in(0);
    }

    //Walk the graph and increment fanin on all downstream nodes
    for (int i = 0; i < num_rr_nodes; i++) {
        for (int iedge = 0; iedge < L_rr_node[i].num_edges(); iedge++) {
            int to_node = L_rr_node[i].edge_sink_node(iedge);

            L_rr_node[to_node].set_fan_in(L_rr_node[to_node].fan_in() + 1);
        }
    }
}

/* Allocates/loads edges for nodes belonging to specified channel segment and initializes
   node properties such as cost, occupancy and capacity */
static void build_rr_chan(const int x_coord, const int y_coord, const t_rr_type chan_type,
        const t_track_to_pin_lookup& track_to_pin_lookup,
        t_sb_connection_map *sb_conn_map,
        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn, const int cost_index_offset,
        const int max_chan_width,
        const DeviceGrid& grid,
        const int tracks_per_chan,
        short ******sblock_pattern, const int Fs_per_side,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        const t_rr_node_indices& L_rr_node_indices,
        t_rr_edge_info_set& created_rr_edges,
        std::vector<t_rr_node>& L_rr_node,
        const int wire_to_ipin_switch,
        const enum e_directionality directionality) {

    /* this function builds both x and y-directed channel segments, so set up our
       coordinates based on channel type */

    auto& device_ctx = g_vpr_ctx.device();

    //Initally a assumes CHANX
    int seg_coord = x_coord; //The absolute coordinate of this segment within the channel
    int chan_coord = y_coord; //The absolute coordinate of this channel within the device
    int seg_dimension = device_ctx.grid.width() - 2; //-2 for no perim channels
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

    t_seg_details * seg_details = from_chan_details[x_coord][y_coord];

    /* figure out if we're generating switch block edges based on a custom switch block
       description */
    bool custom_switch_block = false;
    if (sb_conn_map != nullptr) {
        VTR_ASSERT(sblock_pattern == nullptr && switch_block_conn.empty());
        custom_switch_block = true;
    }

    /* Loads up all the routing resource nodes in the current channel segment */
    for (int track = 0; track < tracks_per_chan; ++track) {

        if (seg_details[track].length == 0)
            continue;

        //Start and end coordinates of this segment along the length of the channel
        //Note that these values are in the VPR coordinate system (and do not consider
        //wire directionality), so start correspond to left/bottom and end corresponds to right/top
        int start = get_seg_start(seg_details, track, chan_coord, seg_coord);
        int end = get_seg_end(seg_details, track, start, chan_coord, seg_dimension);

        if (seg_coord > start)
            continue; /* Only process segments which start at this location */
        VTR_ASSERT(seg_coord == start);

        t_seg_details * from_seg_details = nullptr;
        if (chan_type == CHANY) {
            from_seg_details = chan_details_y[x_coord][start];
        } else {
            from_seg_details = chan_details_x[start][y_coord];
        }

        int node = get_rr_node_index(L_rr_node_indices, x_coord, y_coord, chan_type, track);
        VTR_ASSERT(node >= 0);

        /* Add the edges from this track to all it's connected pins into the list */
        int num_edges = 0;
        t_rr_edge_info_set rr_edges_to_create;
        num_edges += get_track_to_pins(start, chan_coord, track, tracks_per_chan, node, rr_edges_to_create,
                L_rr_node_indices, track_to_pin_lookup, seg_details, chan_type, seg_dimension,
                wire_to_ipin_switch, directionality);

        /* get edges going from the current track into channel segments which are perpendicular to it */
        if (chan_coord > 0) {
            t_seg_details *to_seg_details;
            if (chan_type == CHANX) {
                to_seg_details = chan_details_y[start][y_coord];
            } else {
                VTR_ASSERT(chan_type == CHANY);
                to_seg_details = chan_details_x[x_coord][start];
            }
            if (to_seg_details->length > 0) {
                num_edges += get_track_to_tracks(chan_coord, start, track, chan_type, chan_coord,
                        opposite_chan_type, seg_dimension, max_chan_width, grid,
                        Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                        from_seg_details, to_seg_details, opposite_chan_details,
                        directionality,
                        L_rr_node_indices,
                        switch_block_conn, sb_conn_map);
            }
        }
        if (chan_coord < chan_dimension) {
            t_seg_details *to_seg_details;
            if (chan_type == CHANX) {
                to_seg_details = chan_details_y[start][y_coord + 1];
            } else {
                VTR_ASSERT(chan_type == CHANY);
                to_seg_details = chan_details_x[x_coord + 1][start];
            }
            if (to_seg_details->length > 0) {
                num_edges += get_track_to_tracks(chan_coord, start, track, chan_type, chan_coord + 1,
                        opposite_chan_type, seg_dimension, max_chan_width, grid,
                        Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                        from_seg_details, to_seg_details, opposite_chan_details,
                        directionality,
                        L_rr_node_indices,
                        switch_block_conn, sb_conn_map);
            }
        }


        /* walk over the switch blocks along the source track and implement edges from this track to other tracks
           in the same channel (i.e. straight-through connections) */
        for (int target_seg = start - 1; target_seg <= end + 1; target_seg++) {
            if (target_seg != start - 1 && target_seg != end + 1) {
                /* skip straight-through connections from midpoint if non-custom switch block.
                   currently non-custom switch blocks don't properly describe connections from the mid-point of a wire segment
                   to other segments in the same channel (i.e. straight-through connections) */
                if (!custom_switch_block) {
                    continue;
                }
            }
            if (target_seg > 0 && target_seg < seg_dimension + 1) {
                t_seg_details *to_seg_details;
                if (chan_type == CHANX) {
                    to_seg_details = chan_details_x[target_seg][y_coord];
                } else {
                    VTR_ASSERT(chan_type == CHANY);
                    to_seg_details = chan_details_y[x_coord][target_seg];
                }
                if (to_seg_details->length > 0) {
                    num_edges += get_track_to_tracks(chan_coord, start, track, chan_type, target_seg,
                            chan_type, seg_dimension, max_chan_width, grid,
                            Fs_per_side, sblock_pattern, node, rr_edges_to_create,
                            from_seg_details, to_seg_details, from_chan_details,
                            directionality,
                            L_rr_node_indices,
                            switch_block_conn, sb_conn_map);
                }
            }
        }


        alloc_and_load_edges_and_switches(L_rr_node, created_rr_edges, rr_edges_to_create);

        /* Edge arrays have now been built up.  Do everything else.  */
        L_rr_node[node].set_cost_index(cost_index_offset + seg_details[track].index);
        L_rr_node[node].set_capacity(1); /* GLOBAL routing handled elsewhere */

        if (chan_type == CHANX) {
            L_rr_node[node].set_coordinates(start, y_coord, end, y_coord);
        } else {
            VTR_ASSERT(chan_type == CHANY);
            L_rr_node[node].set_coordinates(x_coord, start, x_coord, end);
        }

        int length = end - start + 1;
        float R = length * seg_details[track].Rmetal;
        float C = length * seg_details[track].Cmetal;
        L_rr_node[node].set_rc_index(find_create_rr_rc_data(R, C));

        L_rr_node[node].set_ptc_num(track);
        L_rr_node[node].set_type(chan_type);
        L_rr_node[node].set_direction(seg_details[track].direction);
    }
}

void alloc_and_load_edges_and_switches(std::vector<t_rr_node>& L_rr_node,
        t_rr_edge_info_set& created_rr_edges,
        const t_rr_edge_info_set& rr_edges_to_create) {

    /* Sets up all the edge related information for rr_node inode (num_edges,  *
     * the edges array and the switches array).                                */

    for (auto& edge_info : rr_edges_to_create) {
        if (created_rr_edges.count(edge_info)) continue; //Don't create duplicate edges

        L_rr_node[edge_info.from_node].add_edge(edge_info.to_node, edge_info.switch_type);

        created_rr_edges.insert(edge_info); //Record edge to avoid duplicates
    }
}

/* allocate pin to track map for each segment type individually and then combine into a single
   vector */
static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_pin_to_track_map(const e_pin_type pin_type,
        const vtr::Matrix<int>& Fc, const t_type_ptr Type, const std::vector<bool>& perturb_switch_pattern,
        const e_directionality directionality,
        const int num_seg_types, const int *sets_per_seg_type) {

    /* get the maximum number of tracks that any pin can connect to */
    size_t max_pin_tracks = 0;
    for (int iseg = 0; iseg < num_seg_types; iseg++) {
        /* determine the maximum Fc to this segment type across all pins */
        int max_Fc = 0;
        for (int pin_index = 0; pin_index < Type->num_pins; ++pin_index) {
            int pin_class = Type->pin_class[pin_index];
            if (Fc[pin_index][iseg] > max_Fc && Type->class_inf[pin_class].type == pin_type) {
                max_Fc = Fc[pin_index][iseg];
            }
        }

        max_pin_tracks += max_Fc;
    }

    /* allocate 'result' matrix and initialize entries to OPEN. also allocate and intialize matrix which will be
       used to index into the correct entries when loading up 'result' */

    auto result = vtr::NdMatrix<std::vector<int>, 4>({
            size_t(Type->num_pins), //[0..num_pins-1]
            size_t(Type->width), //[0..width-1]
            size_t(Type->height), //[0..height-1]
            4, //[0..sides-1]
        });

    /* multiplier for unidirectional vs bidirectional architectures */
    int fac = 1;
    if (directionality == UNI_DIRECTIONAL) {
        fac = 2;
    }

    /* load the pin to track matrix by looking at each segment type in turn */
    int seg_type_start_track = 0;
    for (int iseg = 0; iseg < num_seg_types; iseg++) {
        int num_seg_type_tracks = fac * sets_per_seg_type[iseg];

        /* determine the maximum Fc to this segment type across all pins */
        int max_Fc = 0;
        for (int pin_index = 0; pin_index < Type->num_pins; ++pin_index) {
            int pin_class = Type->pin_class[pin_index];
            if (Fc[pin_index][iseg] > max_Fc && Type->class_inf[pin_class].type == pin_type) {
                max_Fc = Fc[pin_index][iseg];
            }
        }

        /* get pin connections to tracks of the current segment type */
        auto pin_to_seg_type_map = alloc_and_load_pin_to_seg_type(pin_type,
                num_seg_type_tracks, max_Fc, Type, perturb_switch_pattern[iseg], directionality);

        /* connections in pin_to_seg_type_map are within that seg type -- i.e. in the [0,num_seg_type_tracks-1] range.
           now load up 'result' array with these connections, but offset them so they are relative to the channel
           as a whole */
        for (int ipin = 0; ipin < Type->num_pins; ipin++) {
            for (int iwidth = 0; iwidth < Type->width; iwidth++) {
                for (int iheight = 0; iheight < Type->height; iheight++) {
                    for (int iside = 0; iside < 4; iside++) {
                        for (int iconn = 0; iconn < max_Fc; iconn++) {
                            int relative_track_ind = pin_to_seg_type_map[ipin][iwidth][iheight][iside][iconn];

                            if (relative_track_ind == OPEN) continue;

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
        const int num_seg_type_tracks, const int Fc,
        const t_type_ptr Type, const bool perturb_switch_pattern,
        const e_directionality directionality) {

    /* Note: currently a single value of Fc is used across each pin. In the future
       the looping below will have to be modified if we want to account for pin-based
       Fc values */


    /* NB:  This wastes some space.  Could set tracks_..._pin[ipin][ioff][iside] =
     * NULL if there is no pin on that side, or that pin is of the wrong type.
     * Probably not enough memory to worry about, esp. as it's temporary.
     * If pin ipin on side iside does not exist or is of the wrong type,
     * tracks_connected_to_pin[ipin][iside][0] = OPEN.                               */

    if (Type->num_pins < 1) {
        return vtr::NdMatrix<int, 5>();
    }


    auto tracks_connected_to_pin = vtr::NdMatrix<int, 5>({
        size_t(Type->num_pins), //[0..num_pins-1]
        size_t(Type->width), //[0..width-1]
        size_t(Type->height), //[0..height-1]
        NUM_SIDES, //[0..NUM_SIDES-1]
        size_t(Fc) //[0..Fc-1]
    },
    OPEN); //Unconnected

    //Number of *physical* pins on each side.
    //Note that his may be more than the logical number of pins (i.e.
    //Type->num_pins) if a logical pin has multiple specified physical
    //pinlocations (i.e. appears on multiple sides of the block)
    auto num_dir = vtr::NdMatrix<int, 3>({
        size_t(Type->width), //[0..width-1]
        size_t(Type->height), //[0..height-1]
        NUM_SIDES //[0..NUM_SIDES-1]
    },
    0);

    //List of *physical* pins of the correct type on each side of the current
    //block type. For a specific width/height/side the valid enteries in the
    //last dimension are [0 .. num_dir[width][height][side]-1]
    //
    //Max possible space alloced for simplicity
    auto dir_list = vtr::NdMatrix<int, 4>({
        size_t(Type->width), //[0..width-1]
        size_t(Type->height), //[0..height-1]
        NUM_SIDES, //[0..NUM_SIDES-1]
        size_t(Type->num_pins) //[0..num_pins-1]
    },
    -1); //Defensive coding: Initialize to invalid

    //Number of currently assigned physical pins
    auto num_done_per_dir = vtr::NdMatrix<int, 3>({
        size_t(Type->width), //[0..width-1]
        size_t(Type->height), //[0..height-1]
        NUM_SIDES //[0..NUM_SIDES-1]
    },
    0);

    //Record the physical pin locations and counts per side/offsets combination
    for (int pin = 0; pin < Type->num_pins; ++pin) {
        int pin_class = Type->pin_class[pin];
        if (Type->class_inf[pin_class].type != pin_type) /* Doing either ipins OR opins */
            continue;

        /* Pins connecting only to global resources get no switches -> keeps area model accurate. */
        if (Type->is_global_pin[pin])
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
        pin_loc.width_offset = width;;
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

static void advance_to_next_block_side(t_type_ptr Type, int& width_offset, int& height_offset, e_side& side) {
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

static void load_uniform_connection_block_pattern(
        vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
        const std::vector<t_pin_loc>& pin_locations,
        const int x_chan_width, const int y_chan_width, const int Fc,
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

    int group_size;
    if (directionality == BI_DIRECTIONAL) {
        group_size = 1;
    } else {
        VTR_ASSERT(directionality == UNI_DIRECTIONAL);
        group_size = 2;
    }

    VTR_ASSERT((x_chan_width % group_size == 0) && (y_chan_width % group_size == 0) && (Fc % group_size == 0));

    int num_phys_pins = pin_locations.size();

    for (int i = 0; i < num_phys_pins; ++i) {

        int pin = pin_locations[i].pin_index;
        e_side side = pin_locations[i].side;
        int width = pin_locations[i].width_offset;
        int height = pin_locations[i].height_offset;

        /* Bi-directional treats each track separately, uni-directional works with pairs of tracks */
        for (int j = 0; j < (Fc / group_size); ++j) {

            int max_chan_width = (((side == TOP) || (side == BOTTOM)) ? x_chan_width : y_chan_width);
            float step_size = (float) max_chan_width / (float) (Fc * num_phys_pins);

            VTR_ASSERT(Fc > 0);
            float fc_step = (float) max_chan_width / (float) Fc;

            float ftrack = (i * step_size) + (j * fc_step);
            int itrack = ((int) ftrack) * group_size;

            /* Catch possible floating point round error */
            itrack = min(itrack, max_chan_width - group_size);

            /* Assign the group of tracks for the Fc pattern */
            for (int k = 0; k < group_size; ++k) {
                tracks_connected_to_pin[pin][width][height][side][group_size * j + k] = itrack + k;
            }
        }
    }
}

static void load_perturbed_connection_block_pattern(
        vtr::NdMatrix<int, 5>& tracks_connected_to_pin,
        const std::vector<t_pin_loc>& pin_locations,
        const int x_chan_width, const int y_chan_width, const int Fc,
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
        float step_size = (float) max_chan_width / (float) (Fc * num_phys_pins);

        float spacing_dense = (float) max_chan_width / (float) (2 * Fc_dense);
        float spacing_sparse = (float) max_chan_width / (float) (2 * Fc_sparse);
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
                 This is okay because the starting position > 0 when this occurs
                 so connection is valid and fine */
                int itrack = (int) ftrack;
                itrack = itrack % max_chan_width;
                tracks_connected_to_pin[pin][width][height][side][iconn] = itrack;

                ftrack += spacing[ihalf];
                iconn++;
            }
        }
    } /* End for all physical pins. */
}

#ifdef ENABLE_CHECK_ALL_TRACKS

static void check_all_tracks_reach_pins(t_type_ptr type,
        int *****tracks_connected_to_pin, int max_chan_width, int Fc,
        enum e_pin_type ipin_or_opin) {

    /* Checks that all tracks can be reached by some pin.   */
    VTR_ASSERT(max_chan_width > 0);

    int *num_conns_to_track; /* [0..max_chan_width-1] */
    num_conns_to_track = (int *) vtr::calloc(max_chan_width, sizeof (int));

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
            VTR_LOG_ERROR(
                    "check_all_tracks_reach_pins: Track %d does not connect to any CLB %ss.\n",
                    track, (ipin_or_opin == DRIVER ? "OPIN" : "IPIN"));
        }
    }
    free(num_conns_to_track);
}
#endif

/* Allocates and loads the track to ipin lookup for each physical grid type. This
 * is the same information as the ipin_to_track map but accessed in a different way. */

static vtr::NdMatrix<std::vector<int>, 4> alloc_and_load_track_to_pin_lookup(
        vtr::NdMatrix<std::vector<int>, 4> pin_to_track_map, const vtr::Matrix<int>& Fc,
        const int type_width, const int type_height,
        const int num_pins, const int max_chan_width,
        const int num_seg_types) {

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
                        num_tracks += Fc[pin][iseg];
                    }


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

/* A utility routine to dump the contents of the routing resource graph   *
 * (everything -- connectivity, occupancy, cost, etc.) into a file.  Used *
 * only for debugging.                                                    */
void dump_rr_graph(const char *file_name) {
    auto& device_ctx = g_vpr_ctx.device();

    FILE *fp = vtr::fopen(file_name, "w");

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        print_rr_node(fp, device_ctx.rr_nodes, inode);
        fprintf(fp, "\n");
    }

    fclose(fp);
}

/* Prints all the data about node inode to file fp.                    */
void print_rr_node(FILE * fp, const std::vector<t_rr_node> &L_rr_node, int inode) {

    std::string info = describe_rr_node(inode);
    fprintf(fp, "%s\n", info.c_str());

    fprintf(fp, "%d edge(s):", L_rr_node[inode].num_edges());
    for (int iconn = 0; iconn < L_rr_node[inode].num_edges(); ++iconn)
        fprintf(fp, " %d", L_rr_node[inode].edge_sink_node(iconn));
    fprintf(fp, "\n");

    fprintf(fp, "Switch types:");
    for (int iconn = 0; iconn < L_rr_node[inode].num_edges(); ++iconn)
        fprintf(fp, " %d", L_rr_node[inode].edge_switch(iconn));
    fprintf(fp, "\n");

    fprintf(fp, "Capacity: %d\n", L_rr_node[inode].capacity());
    fprintf(fp, "R: %g  C: %g\n", L_rr_node[inode].R(), L_rr_node[inode].C());
    fprintf(fp, "Cost_index: %d\n", L_rr_node[inode].cost_index());
}

/* Prints all the device_ctx.rr_indexed_data of index to file fp.   */
void print_rr_indexed_data(FILE * fp, int index) {
    auto& device_ctx = g_vpr_ctx.device();

    fprintf(fp, "Index: %d\n", index);

    fprintf(fp, "ortho_cost_index: %d  ", device_ctx.rr_indexed_data[index].ortho_cost_index);
    fprintf(fp, "base_cost: %g  ", device_ctx.rr_indexed_data[index].saved_base_cost);
    fprintf(fp, "saved_base_cost: %g\n", device_ctx.rr_indexed_data[index].saved_base_cost);

    fprintf(fp, "Seg_index: %d  ", device_ctx.rr_indexed_data[index].seg_index);
    fprintf(fp, "inv_length: %g\n", device_ctx.rr_indexed_data[index].inv_length);

    fprintf(fp, "T_linear: %g  ", device_ctx.rr_indexed_data[index].T_linear);
    fprintf(fp, "T_quadratic: %g  ", device_ctx.rr_indexed_data[index].T_quadratic);
    fprintf(fp, "C_load: %g\n", device_ctx.rr_indexed_data[index].C_load);
}

std::string describe_rr_node(int inode) {
    auto& device_ctx = g_vpr_ctx.device();

    std::string msg = vtr::string_fmt("RR node: %d", inode);

    const auto& rr_node = device_ctx.rr_nodes[inode];

    msg += vtr::string_fmt(" type: %s", rr_node.type_string());

    msg += vtr::string_fmt(" location: (%d,%d)", rr_node.xlow(), rr_node.ylow());
    if (rr_node.xlow() != rr_node.xhigh() || rr_node.ylow() != rr_node.yhigh()) {
        msg += vtr::string_fmt(" <-> (%d,%d)", rr_node.xhigh(), rr_node.yhigh());
    }

    if (rr_node.type() == CHANX || rr_node.type() == CHANY) {
        int cost_index = rr_node.cost_index();

        int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

        msg += vtr::string_fmt(" track: %d len: %d longline: %d seg_type: %s dir: %s",
                    rr_node.track_num(),
                    rr_node.length(),
                    device_ctx.arch.Segments[seg_index].longline,
                    device_ctx.arch.Segments[seg_index].name,
                    rr_node.direction_string());
    } else if (rr_node.type() == IPIN || rr_node.type() == OPIN) {

        t_type_ptr type = device_ctx.grid[rr_node.xlow()][rr_node.ylow()].type;
        std::string pin_name = block_type_pin_index_to_name(type, rr_node.pin_num());

        msg += vtr::string_fmt(" pin: %d pin_name: %s",
                rr_node.pin_num(),
                pin_name.c_str());
    } else {
        VTR_ASSERT(rr_node.type() == SOURCE || rr_node.type() == SINK);

        msg += vtr::string_fmt(" class: %d", rr_node.class_num());
    }

    return msg;
}
static void build_unidir_rr_opins(const int i, const int j, const e_side side,
        const DeviceGrid& grid, const std::vector<vtr::Matrix<int>>&Fc_out, const int max_chan_width,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        vtr::NdMatrix<int, 3>& Fc_xofs, vtr::NdMatrix<int, 3>& Fc_yofs,
        t_rr_edge_info_set& created_rr_edges,
        bool * Fc_clipped, const t_rr_node_indices& L_rr_node_indices,
        const t_direct_inf *directs, const int num_directs, const t_clb_to_clb_directs *clb_to_clb_directs,
        const int num_seg_types) {

    /*
     * This routine adds the edges from opins to channels at the specified
     * grid location (i,j) and grid tile side
     */
    auto& device_ctx = g_vpr_ctx.mutable_device();

    *Fc_clipped = false;

    t_type_ptr type = grid[i][j].type;

    int width_offset = grid[i][j].width_offset;
    int height_offset = grid[i][j].height_offset;

    /* Go through each pin and find its fanout. */
    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        /* Skip global pins and pins that are not of DRIVER type */
        int class_index = type->pin_class[pin_index];
        if (type->class_inf[class_index].type != DRIVER) {
            continue;
        }
        if (type->is_global_pin[pin_index]) {
            continue;
        }

        t_rr_edge_info_set rr_edges_to_create;

        int opin_node_index = get_rr_node_index(L_rr_node_indices, i, j, OPIN, pin_index, side);
        if (opin_node_index < 0) continue; //No valid from node

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

            t_seg_details * seg_details = (chan_type == CHANX ?
                    chan_details_x[seg][chan] : chan_details_y[chan][seg]);
            if (seg_details[0].length == 0)
                continue;

            /* Get the list of opin to mux connections for that chan seg. */
            bool clipped;
            get_unidir_opin_connections(chan, seg,
                    seg_type_Fc, iseg, chan_type, seg_details, 
                    opin_node_index,
                    rr_edges_to_create,
                    Fc_ofs, max_len, max_chan_width,
                    L_rr_node_indices, &clipped);
            if (clipped) {
                *Fc_clipped = true;
            }
        }

        /* Add in direct connections */
        get_opin_direct_connecions(i, j, side, pin_index, opin_node_index, rr_edges_to_create, L_rr_node_indices,
                directs, num_directs, clb_to_clb_directs);

        /* Add the edges */
        alloc_and_load_edges_and_switches(device_ctx.rr_nodes, created_rr_edges, rr_edges_to_create);
    }
}

/**
 * Parse out which CLB pins should connect directly to which other CLB pins then store that in a clb_to_clb_directs data structure
 * This data structure supplements the the info in the "directs" data structure
 * TODO: The function that does this parsing in placement is poorly done because it lacks generality on heterogeniety, should replace with this one
 */
static t_clb_to_clb_directs * alloc_and_load_clb_to_clb_directs(const t_direct_inf *directs, const int num_directs, int delayless_switch) {
    int i, j;
    t_clb_to_clb_directs *clb_to_clb_directs;
    char *pb_type_name, *port_name;
    int start_pin_index, end_pin_index;
    t_pb_type *pb_type;

    auto& device_ctx = g_vpr_ctx.device();

    clb_to_clb_directs = (t_clb_to_clb_directs*) vtr::calloc(num_directs, sizeof (t_clb_to_clb_directs));

    pb_type_name = nullptr;
    port_name = nullptr;

    for (i = 0; i < num_directs; i++) {
        pb_type_name = (char*) vtr::malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof (char));
        port_name = (char*) vtr::malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof (char));

        // Load from pins
        // Parse out the pb_type name, port name, and pin range
        parse_direct_pin_name(directs[i].from_pin, directs[i].line, &start_pin_index, &end_pin_index, pb_type_name, port_name);

        // Figure out which type, port, and pin is used
        for (j = 0; j < device_ctx.num_block_types; j++) {
            if (strcmp(device_ctx.block_types[j].name, pb_type_name) == 0) {
                break;
            }
        }
        if (j >= device_ctx.num_block_types) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line, "Unable to find block %s.\n", pb_type_name);
        }
        clb_to_clb_directs[i].from_clb_type = &device_ctx.block_types[j];
        pb_type = clb_to_clb_directs[i].from_clb_type->pb_type;

        for (j = 0; j < pb_type->num_ports; j++) {
            if (strcmp(pb_type->ports[j].name, port_name) == 0) {
                break;
            }
        }
        if (j >= pb_type->num_ports) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line, "Unable to find port %s (on block %s).\n", port_name, pb_type_name);
        }

        if (start_pin_index == OPEN) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = pb_type->ports[j].num_pins - 1;
        }
        get_blk_pin_from_port_pin(clb_to_clb_directs[i].from_clb_type->index, j, start_pin_index, &clb_to_clb_directs[i].from_clb_pin_start_index);
        get_blk_pin_from_port_pin(clb_to_clb_directs[i].from_clb_type->index, j, end_pin_index, &clb_to_clb_directs[i].from_clb_pin_end_index);

        // Load to pins
        // Parse out the pb_type name, port name, and pin range
        parse_direct_pin_name(directs[i].to_pin, directs[i].line, &start_pin_index, &end_pin_index, pb_type_name, port_name);

        // Figure out which type, port, and pin is used
        for (j = 0; j < device_ctx.num_block_types; j++) {
            if (strcmp(device_ctx.block_types[j].name, pb_type_name) == 0) {
                break;
            }
        }
        if (j >= device_ctx.num_block_types) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line, "Unable to find block %s.\n", pb_type_name);
        }
        clb_to_clb_directs[i].to_clb_type = &device_ctx.block_types[j];
        pb_type = clb_to_clb_directs[i].to_clb_type->pb_type;

        for (j = 0; j < pb_type->num_ports; j++) {
            if (strcmp(pb_type->ports[j].name, port_name) == 0) {
                break;
            }
        }
        if (j >= pb_type->num_ports) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line, "Unable to find port %s (on block %s).\n", port_name, pb_type_name);
        }

        if (start_pin_index == OPEN) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = pb_type->ports[j].num_pins - 1;
        }

        get_blk_pin_from_port_pin(clb_to_clb_directs[i].to_clb_type->index, j, start_pin_index, &clb_to_clb_directs[i].to_clb_pin_start_index);
        get_blk_pin_from_port_pin(clb_to_clb_directs[i].to_clb_type->index, j, end_pin_index, &clb_to_clb_directs[i].to_clb_pin_end_index);

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
        free(pb_type_name);
        free(port_name);

        //We must be careful to clean-up anything that we may have incidentally allocated.
        //Specifically, we can be called while generating the dummy architecture
        //for placer delay estimation.  Since the delay estimation occurs on a
        //'different' architecture it is almost certain that the f_blk_pin_from_port_pin allocated
        //by calling get_blk_pin_from_port_pin() will later be invalid.
        //We therefore must free it now.
        free_blk_pin_from_port_pin();

    }
    return clb_to_clb_directs;
}

/* Add all direct clb-pin-to-clb-pin edges to given opin
 *
 * The current opin is located at (x,y) along the specified side
 */
static int get_opin_direct_connecions(int x, int y, e_side side, int opin,
        int from_rr_node,
        t_rr_edge_info_set& rr_edges_to_create,
        const t_rr_node_indices& L_rr_node_indices,
        const t_direct_inf *directs, const int num_directs,
        const t_clb_to_clb_directs *clb_to_clb_directs) {

    t_type_ptr curr_type, target_type;
    int i, ipin;
    int max_index, min_index, offset, swap;

    auto& device_ctx = g_vpr_ctx.device();

    curr_type = device_ctx.grid[x][y].type;

    int num_pins = 0;

    int width_offset = device_ctx.grid[x][y].width_offset;
    int height_offset = device_ctx.grid[x][y].height_offset;
    if (!curr_type->pinloc[width_offset][height_offset][side][opin]) {
        return num_pins; //No source pin on this side
    }

    /* Iterate through all direct connections */
    for (i = 0; i < num_directs; i++) {
        /* Find matching direct clb-to-clb connections with the same type as current grid location */
        if (clb_to_clb_directs[i].from_clb_type == curr_type) { //We are at a valid starting point

            //Offset must be in range
            if (x + directs[i].x_offset < int(device_ctx.grid.width() - 1) &&
                    x + directs[i].x_offset > 0 &&
                    y + directs[i].y_offset < int(device_ctx.grid.height() - 1) &&
                    y + directs[i].y_offset > 0) {

                //Only add connections if the target clb type matches the type in the direct specification
                target_type = device_ctx.grid[x + directs[i].x_offset][y + directs[i].y_offset].type;
                if (clb_to_clb_directs[i].to_clb_type == target_type) {

                    /* Compute index of opin with regards to given pins */
                    if (clb_to_clb_directs[i].from_clb_pin_start_index > clb_to_clb_directs[i].from_clb_pin_end_index) {
                        swap = true;
                        max_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        min_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    } else {
                        swap = false;
                        min_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        max_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    }
                    if (max_index >= opin && min_index <= opin) {
                        offset = opin - min_index;
                        /* This opin is specified to connect directly to an ipin, now compute which ipin to connect to */
                        ipin = OPEN;
                        if (clb_to_clb_directs[i].to_clb_pin_start_index > clb_to_clb_directs[i].to_clb_pin_end_index) {
                            if (swap) {
                                ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
                            } else {
                                ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
                            }
                        } else {
                            if (swap) {
                                ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
                            } else {
                                ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
                            }
                        }

                        /* Add new ipin edge to list of edges */
                        int target_width_offset = device_ctx.grid[x + directs[i].x_offset][y + directs[i].y_offset].width_offset;
                        int target_height_offset = device_ctx.grid[x + directs[i].x_offset][y + directs[i].y_offset].height_offset;

                        auto inodes = get_rr_node_indices(L_rr_node_indices, x + directs[i].x_offset - target_width_offset, y + directs[i].y_offset - target_height_offset,
                                IPIN, ipin);

                        //There may be multiple physical pins corresponding to the logical
                        //target ipin. We only need to connect to one of them (since the physical pins
                        //are logically equivalent).
                        VTR_ASSERT(inodes.size() > 0);
                        int inode = inodes[0];

                        rr_edges_to_create.emplace(from_rr_node, inode, clb_to_clb_directs[i].switch_index);
                        ++num_pins;
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
static std::vector<bool> alloc_and_load_perturb_opins(const t_type_ptr type, const vtr::Matrix<int>& Fc_out,
        const int max_chan_width, const int num_seg_types, const t_segment_inf *segment_inf) {

    int i, Fc_max, iclass, num_wire_types;
    int num, max_primes, factor, num_factors;
    int *prime_factors;
    float step_size = 0;
    float n = 0;
    float threshold = 0.07;

    std::vector<bool> perturb_opins(num_seg_types, false);

    i = Fc_max = iclass = 0;
    if (num_seg_types > 1) {
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
    max_primes = (int) floor(log((float) num_wire_types) / log(2.0));
    max_primes = std::max(max_primes, 1); //Minimum of 1 to ensure we allocate space for at least one prime_factor

    prime_factors = (int *) vtr::malloc(max_primes * sizeof (int));
    for (i = 0; i < max_primes; i++) {
        prime_factors[i] = 0;
    }

    /* Find the prime factors of num_wire_types */
    num = num_wire_types;
    factor = 2;
    num_factors = 0;
    while (pow((float) factor, 2) <= num) {
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
    step_size = (float) max_chan_width / Fc_max;
    for (i = 0; i < num_factors; i++) {
        if (vtr::nint(step_size) < prime_factors[i]) {
            perturb_opins[0] = false;
            break;
        }

        n = step_size / prime_factors[i];
        n = n - (float) vtr::nint(n); /* fractinal part */
        if (fabs(n) < threshold) {
            perturb_opins[0] = true;
            break;
        } else {
            perturb_opins[0] = false;
        }
    }
    free(prime_factors);

    return perturb_opins;
}

