#include <cstdio>

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_utils.h"
#include "rr_graph2.h"
#include "rr_graph.h"
#include "rr_graph_sbox.h"
#include "read_xml_arch_file.h"
#include "rr_types.h"

constexpr short UN_SET = -1;
/************************** Subroutines local to this module ****************/

static void get_switch_type(bool is_from_sb,
                            bool is_to_sb,
                            short from_node_switch,
                            short to_node_switch,
                            const int switch_override,
                            short switch_types[2]);

static void load_chan_rr_indices(const int max_chan_width,
                                 const DeviceGrid& grid,
                                 const int chan_len,
                                 const int num_chans,
                                 const t_rr_type type,
                                 const t_chan_details& chan_details,
                                 RRGraphBuilder& rr_graph_builder,
                                 int* index);

static void load_block_rr_indices(RRGraphBuilder& rr_graph_builder,
                                  const DeviceGrid& grid,
                                  int* index,
                                  bool is_flat);

static void add_pins_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                    t_physical_tile_type_ptr physical_type_ptr,
                                    const std::vector<int>& pin_num_vec,
                                    int layer,
                                    int root_x,
                                    int root_y,
                                    int* index,
                                    const std::vector<e_side>& wanted_sides);

static void add_classes_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                       t_physical_tile_type_ptr physical_type_ptr,
                                       const std::vector<int>& class_num_vec,
                                       int layer,
                                       int x,
                                       int y,
                                       int block_width,
                                       int block_height,
                                       int* index);

static int get_bidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                       const std::vector<int> conn_tracks,
                                       const int layer,
                                       const int to_chan,
                                       const int to_seg,
                                       const int to_sb,
                                       const t_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       const bool from_is_sblock,
                                       const int from_switch,
                                       const int switch_override,
                                       const enum e_directionality directionality,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create);

static int get_unidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                        const int layer,
                                        const int from_track,
                                        const int to_chan,
                                        const int to_seg,
                                        const int to_sb,
                                        const t_rr_type to_type,
                                        const int max_chan_width,
                                        const DeviceGrid& grid,
                                        const enum e_side from_side,
                                        const enum e_side to_side,
                                        const int Fs_per_side,
                                        t_sblock_pattern& sblock_pattern,
                                        const int switch_override,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        RRNodeId from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create);

static int get_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                 const int layer,
                                 const int from_track,
                                 const int to_chan,
                                 const int to_seg,
                                 const t_rr_type to_chan_type,
                                 const e_side from_side,
                                 const e_side to_side,
                                 const int swtich_override,
                                 t_sb_connection_map* sb_conn_map,
                                 RRNodeId from_rr_node,
                                 t_rr_edge_info_set& rr_edges_to_create);

static int vpr_to_phy_track(const int itrack,
                            const int chan_num,
                            const int seg_num,
                            const t_chan_seg_details* seg_details,
                            const enum e_directionality directionality);

static void label_wire_muxes(const int chan_num,
                             const int seg_num,
                             const t_chan_seg_details* seg_details,
                             const int seg_type_index,
                             const int max_len,
                             const enum Direction dir,
                             const int max_chan_width,
                             const bool check_cb,
                             std::vector<int>& labels,
                             int* num_wire_muxes,
                             int* num_wire_muxes_cb_restricted);

static void label_incoming_wires(const int chan_num,
                                 const int seg_num,
                                 const int sb_seg,
                                 const t_chan_seg_details* seg_details,
                                 const int max_len,
                                 const enum Direction dir,
                                 const int max_chan_width,
                                 std::vector<int>& labels,
                                 int* num_incoming_wires,
                                 int* num_ending_wires);

static int find_label_of_track(const std::vector<int>& wire_mux_on_track,
                               int num_wire_muxes,
                               int from_track);

void dump_seg_details(t_seg_details* seg_details,
                      int max_chan_width,
                      const char* fname);

//Returns how the switch type for the switch block at the specified location should be created
//  grid: The device grid
//  from_chan_coord: The horizontal or vertical channel index (i.e. x-coord for CHANY, y-coord for CHANX)
//  from_seg_coord: The horizontal or vertical location along the channel (i.e. y-coord for CHANY, x-coord for CHANX)
//  from_chan_type: The from channel type
//  to_chan_type: The to channel type
static int should_create_switchblock(const DeviceGrid& grid, int layer_num, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type);

static bool should_apply_switch_override(int switch_override);

/******************** Subroutine definitions *******************************/

/* This assigns tracks (individually or pairs) to segment types.
 * It tries to match requested ratio. If use_full_seg_groups is
 * true, then segments are assigned only in multiples of their
 * length. This is primarily used for making a tileable unidir
 * layout. The effect of using this is that the number of tracks
 * requested will not always be met and the result will sometimes
 * be over and sometimes under.
 * The pattern when using use_full_seg_groups is to keep adding
 * one group of the track type that wants the largest number of
 * groups of tracks. Each time a group is assigned, the types
 * demand is reduced by 1 unit. The process stops when we are
 * no longer less than the requested number of tracks. As a final
 * step, if we were closer to target before last more, undo it
 * and end up with a result that uses fewer tracks than given. */
std::unique_ptr<int[]> get_seg_track_counts(const int num_sets,
                                            const std::vector<t_segment_inf>& segment_inf,
                                            const bool use_full_seg_groups) {
    std::unique_ptr<int[]> result;
    int imax, freq_sum, assigned, size;
    double scale, max, reduce;

    result = std::make_unique<int[]>(segment_inf.size());
    std::vector<double> demand(segment_inf.size());

    /* Scale factor so we can divide by any length
     * and still use integers */
    scale = 1;
    freq_sum = 0;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        scale *= segment_inf[i].length;
        freq_sum += segment_inf[i].frequency;
    }
    reduce = scale * freq_sum;

    /* Init assignments to 0 and set the demand values */
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        result[i] = 0;
        demand[i] = scale * num_sets * segment_inf[i].frequency;
        if (use_full_seg_groups) {
            demand[i] /= segment_inf[i].length;
        }
    }

    /* Keep assigning tracks until we use them up */
    assigned = 0;
    size = 0;
    imax = 0;
    while (assigned < num_sets) {
        /* Find current maximum demand */
        max = 0;
        for (size_t i = 0; i < segment_inf.size(); ++i) {
            if (demand[i] > max) {
                imax = i;
                max = demand[i];
            }
        }

        /* Assign tracks to the type and reduce the types demand */
        size = (use_full_seg_groups ? segment_inf[imax].length : 1);
        demand[imax] -= reduce;
        result[imax] += size;
        assigned += size;
    }

    /* Undo last assignment if we were closer to goal without it */
    if ((assigned - num_sets) > (size / 2)) {
        result[imax] -= size;
    }

    /* This must be freed by caller */
    return result;
}

int get_parallel_seg_index(const int abs_index,
                           const t_unified_to_parallel_seg_index& index_map,
                           const e_parallel_axis parallel_axis) {
    int index = -1;
    auto itr_pair = index_map.equal_range(abs_index);

    for (auto itr = itr_pair.first; itr != itr_pair.second; ++itr) {
        if (itr->second.second == parallel_axis) {
            index = itr->second.first;
        }
    }

    return index;
}

/*  Returns an array of tracks per segment, with matching indices to segment_inf by combining               *
 * sets per segment for each direction. This is a helper function to avoid having to refactor              *
 * alot of the functions inside rr_graph.cpp & rr_graph2.cpp to model different horizontal and vertical    *
 * channel widths.                                                                                          */
std::unique_ptr<int[]> get_ordered_seg_track_counts(const std::vector<t_segment_inf>& segment_inf_x,
                                                    const std::vector<t_segment_inf>& segment_inf_y,
                                                    const std::vector<t_segment_inf>& segment_inf,
                                                    const std::unique_ptr<int[]>& segment_sets_x,
                                                    const std::unique_ptr<int[]>& segment_sets_y) {
    std::unordered_map<t_segment_inf, int, t_hash_segment_inf> all_segs_index;
    std::unique_ptr<int[]> ordered_seg_track_counts;
    ordered_seg_track_counts = std::make_unique<int[]>(segment_inf.size());

    for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
        all_segs_index.insert(std::make_pair(segment_inf[iseg], iseg));
    }
    for (size_t iseg_x = 0; iseg_x < segment_inf_x.size(); ++iseg_x) {
        auto seg_in_x_dir = all_segs_index.find(segment_inf_x[iseg_x]);
        if (seg_in_x_dir != all_segs_index.end()) {
            ordered_seg_track_counts[seg_in_x_dir->second] = segment_sets_x[iseg_x];
        } else {
            VTR_ASSERT_MSG(seg_in_x_dir != all_segs_index.end(),
                           "Segment in the x-direction must be a part of all segments.");
        }
    }
    for (size_t iseg_y = 0; iseg_y < segment_inf_y.size(); ++iseg_y) {
        if (segment_inf_y[iseg_y].parallel_axis == BOTH_AXIS) { /*Avoid counting segments in both horizontal and vertical direction twice*/
            continue;
        }
        auto seg_in_y_dir = all_segs_index.find(segment_inf_y[iseg_y]);
        if (seg_in_y_dir != all_segs_index.end()) {
            ordered_seg_track_counts[seg_in_y_dir->second] = segment_sets_y[iseg_y];
        } else {
            VTR_ASSERT_MSG(seg_in_y_dir != all_segs_index.end(),
                           "Segment in the x-direction must be a part of all segments.");
        }
    }

    return ordered_seg_track_counts;
}

t_seg_details* alloc_and_load_seg_details(int* max_chan_width,
                                          const int max_len,
                                          const std::vector<t_segment_inf>& segment_inf,
                                          const bool use_full_seg_groups,
                                          const enum e_directionality directionality,
                                          int* num_seg_details) {
    /* Allocates and loads the seg_details data structure.  Max_len gives the   *
     * maximum length of a segment (dimension of array).  The code below tries  *
     * to:                                                                      *
     * (1) stagger the start points of segments of the same type evenly;        *
     * (2) spread out the limited number of connection boxes or switch boxes    *
     *     evenly along the length of a segment, starting at the segment ends;  *
     * (3) stagger the connection and switch boxes on different long lines,     *
     *     as they will not be staggered by different segment start points.     */

    int cur_track, ntracks, itrack, length, j, index;
    int arch_wire_switch, arch_opin_switch, fac, num_sets, tmp;
    int arch_opin_between_dice_switch;
    int group_start, first_track;
    std::unique_ptr<int[]> sets_per_seg_type;
    t_seg_details* seg_details = nullptr;
    bool longline;

    /* Unidir tracks are assigned in pairs, and bidir tracks individually */
    if (directionality == BI_DIRECTIONAL) {
        fac = 1;
    } else {
        VTR_ASSERT(directionality == UNI_DIRECTIONAL);
        fac = 2;
    }

    if (*max_chan_width % fac != 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Routing channel width must be divisible by %d (channel width was %d)", fac, *max_chan_width);
    }

    /* Map segment type fractions and groupings to counts of tracks */
    sets_per_seg_type = get_seg_track_counts((*max_chan_width / fac),
                                             segment_inf, use_full_seg_groups);

    /* Count the number tracks actually assigned. */
    tmp = 0;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        tmp += sets_per_seg_type[i] * fac;
    }
    VTR_ASSERT(use_full_seg_groups || (tmp == *max_chan_width));
    *max_chan_width = tmp;

    seg_details = new t_seg_details[*max_chan_width];

    /* Setup the seg_details data */
    cur_track = 0;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        first_track = cur_track;

        num_sets = sets_per_seg_type[i];
        ntracks = fac * num_sets;
        if (ntracks < 1) {
            continue;
        }
        /* Avoid divide by 0 if ntracks */
        longline = segment_inf[i].longline;
        length = segment_inf[i].length;
        if (longline) {
            length = max_len;
        }

        arch_wire_switch = segment_inf[i].arch_wire_switch;
        arch_opin_switch = segment_inf[i].arch_opin_switch;
        arch_opin_between_dice_switch = segment_inf[i].arch_opin_between_dice_switch;
        VTR_ASSERT((arch_wire_switch == arch_opin_switch) || (directionality != UNI_DIRECTIONAL));

        /* Set up the tracks of same type */
        group_start = 0;
        for (itrack = 0; itrack < ntracks; itrack++) {
            /* set the name of the segment type this track belongs to */
            seg_details[cur_track].type_name = segment_inf[i].name;

            /* Remember the start track of the current wire group */
            if ((itrack / fac) % length == 0 && (itrack % fac) == 0) {
                group_start = cur_track;
            }

            seg_details[cur_track].length = length;
            seg_details[cur_track].longline = longline;

            /* Stagger the start points in for each track set. The
             * pin mappings should be aware of this when choosing an
             * intelligent way of connecting pins and tracks.
             * cur_track is used as an offset so that extra tracks
             * from different segment types are hopefully better
             * balanced. */
            seg_details[cur_track].start = (cur_track / fac) % length + 1;

            /* These properties are used for vpr_to_phy_track to determine
             * * twisting of wires. */
            seg_details[cur_track].group_start = group_start;
            seg_details[cur_track].group_size = std::min(ntracks + first_track - group_start, length * fac);
            VTR_ASSERT(0 == seg_details[cur_track].group_size % fac);
            if (0 == seg_details[cur_track].group_size) {
                seg_details[cur_track].group_size = length * fac;
            }

            seg_details[cur_track].seg_start = -1;
            seg_details[cur_track].seg_end = -1;

            /* Setup the cb and sb patterns. Global route graphs can't depopulate cb and sb
             * since this is a property of a detailed route. */
            seg_details[cur_track].cb = std::make_unique<bool[]>(length);
            seg_details[cur_track].sb = std::make_unique<bool[]>(length + 1);
            for (j = 0; j < length; ++j) {
                if (seg_details[cur_track].longline) {
                    seg_details[cur_track].cb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    index = j % segment_inf[i].cb.size();
                    seg_details[cur_track].cb[j] = segment_inf[i].cb[index];
                }
            }
            for (j = 0; j < (length + 1); ++j) {
                if (seg_details[cur_track].longline) {
                    seg_details[cur_track].sb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    index = j % segment_inf[i].sb.size();
                    seg_details[cur_track].sb[j] = segment_inf[i].sb[index];
                }
            }

            seg_details[cur_track].Rmetal = segment_inf[i].Rmetal;
            seg_details[cur_track].Cmetal = segment_inf[i].Cmetal;
            //seg_details[cur_track].Cmetal_per_m = segment_inf[i].Cmetal_per_m;

            seg_details[cur_track].arch_wire_switch = arch_wire_switch;
            seg_details[cur_track].arch_opin_switch = arch_opin_switch;
            seg_details[cur_track].arch_opin_between_dice_switch = arch_opin_between_dice_switch;

            if (BI_DIRECTIONAL == directionality) {
                seg_details[cur_track].direction = Direction::BIDIR;
            } else {
                VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                seg_details[cur_track].direction = (itrack % 2) ? Direction::DEC : Direction::INC;
            }

            seg_details[cur_track].index = i;
            seg_details[cur_track].abs_index = segment_inf[i].seg_index;

            ++cur_track;
        }
    } /* End for each segment type. */

    if (num_seg_details) {
        *num_seg_details = cur_track;
    }
    return seg_details;
}

/* Allocates and loads the chan_details data structure, a 2D array of
 * seg_details structures. This array is used to handle unique seg_details
 * (ie. channel segments) for each horizontal and vertical channel. */

void alloc_and_load_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details_x,
                                 const int num_seg_details_y,
                                 const t_seg_details* seg_details_x,
                                 const t_seg_details* seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y) {
    chan_details_x = init_chan_details(grid, nodes_per_chan,
                                       num_seg_details_x, seg_details_x, X_AXIS);
    chan_details_y = init_chan_details(grid, nodes_per_chan,
                                       num_seg_details_y, seg_details_y, Y_AXIS);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(grid, nodes_per_chan,
                        chan_details_x, chan_details_y);
}

t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 const enum e_parallel_axis seg_parallel_axis) {
    if (seg_parallel_axis == X_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan->x_max);
    } else if (seg_parallel_axis == Y_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan->y_max);
    }

    t_chan_details chan_details({grid.width(), grid.height(), size_t(num_seg_details)});

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            t_chan_seg_details* p_seg_details = chan_details[x][y].data();
            for (int i = 0; i < num_seg_details; ++i) {
                p_seg_details[i] = t_chan_seg_details(&seg_details[i]);

                int seg_start = -1;
                int seg_end = -1;

                if (seg_parallel_axis == X_AXIS) {
                    seg_start = get_seg_start(p_seg_details, i, y, x);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, y, grid.width() - 2); //-2 for no perim channels
                } else if (seg_parallel_axis == Y_AXIS) {
                    seg_start = get_seg_start(p_seg_details, i, x, y);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, x, grid.height() - 2); //-2 for no perim channels
                }

                p_seg_details[i].set_seg_start(seg_start);
                p_seg_details[i].set_seg_end(seg_end);

                if (seg_parallel_axis == X_AXIS) {
                    if (i >= nodes_per_chan->x_list[y]) {
                        p_seg_details[i].set_length(0);
                    }
                } else if (seg_parallel_axis == Y_AXIS) {
                    if (i >= nodes_per_chan->y_list[x]) {
                        p_seg_details[i].set_length(0);
                    }
                }
            }
        }
    }
    return chan_details;
}

void adjust_chan_details(const DeviceGrid& grid,
                         const t_chan_width* nodes_per_chan,
                         t_chan_details& chan_details_x,
                         t_chan_details& chan_details_y) {
    for (size_t y = 0; y <= grid.height() - 2; ++y) {    //-2 for no perim channels
        for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_x[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                               chan_details_x, X_AXIS);
        }
    }

    for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_y[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                               chan_details_y, Y_AXIS);
        }
    }
}

void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width* nodes_per_chan,
                        t_chan_details& chan_details,
                        const enum e_parallel_axis seg_parallel_axis) {
    int seg_index = (seg_parallel_axis == X_AXIS ? x : y);
    int max_chan_width = 0;
    if (seg_parallel_axis == X_AXIS) {
        max_chan_width = nodes_per_chan->x_max;
    } else if (seg_parallel_axis == Y_AXIS) {
        max_chan_width = nodes_per_chan->y_max;
    } else {
        VTR_ASSERT(seg_parallel_axis == BOTH_AXIS);
        max_chan_width = nodes_per_chan->max;
    }

    for (int track = 0; track < max_chan_width; ++track) {
        int lx = (seg_parallel_axis == X_AXIS ? x - 1 : x);
        int ly = (seg_parallel_axis == X_AXIS ? y : y - 1);
        if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
            continue;

        while (chan_details[lx][ly][track].seg_end() >= seg_index) {
            chan_details[lx][ly][track].set_seg_end(seg_index - 1);
            lx = (seg_parallel_axis == X_AXIS ? lx - 1 : lx);
            ly = (seg_parallel_axis == X_AXIS ? ly : ly - 1);
            if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
                break;
        }
    }

    for (int track = 0; track < max_chan_width; ++track) {
        size_t lx = (seg_parallel_axis == X_AXIS ? x + 1 : x);
        size_t ly = (seg_parallel_axis == X_AXIS ? y : y + 1);
        if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length() == 0) //-2 for no perim channels
            continue;

        while (chan_details[lx][ly][track].seg_start() <= seg_index) {
            chan_details[lx][ly][track].set_seg_start(seg_index + 1);
            lx = (seg_parallel_axis == X_AXIS ? lx + 1 : lx);
            ly = (seg_parallel_axis == X_AXIS ? ly : ly + 1);
            if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length() == 0) //-2 for no perim channels
                break;
        }
    }
}

void free_chan_details(t_chan_details& chan_details_x,
                       t_chan_details& chan_details_y) {
    chan_details_x.clear();
    chan_details_y.clear();
}

/* Returns the segment number at which the segment this track lies on        *
 * started.                                                                  */
int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num) {
    int seg_start = 0;
    if (seg_details[itrack].seg_start() >= 0) {
        seg_start = seg_details[itrack].seg_start();

    } else {
        seg_start = 1;
        if (false == seg_details[itrack].longline()) {
            int length = seg_details[itrack].length();
            int start = seg_details[itrack].start();

            /* Start is guaranteed to be between 1 and length.  Hence adding length to *
             * the quantity in brackets below guarantees it will be nonnegative.       */

            VTR_ASSERT(start > 0);
            VTR_ASSERT(start <= length);

            /* NOTE: Start points are staggered between different channels.
             * The start point must stagger backwards as chan_num increases.
             * Unidirectional routing expects this to allow the N-to-N
             * assumption to be made with respect to ending wires in the core. */
            seg_start = seg_num - (seg_num + length + chan_num - start) % length;
            if (seg_start < 1) {
                seg_start = 1;
            }
        }
    }
    return seg_start;
}

int get_seg_end(const t_chan_seg_details* seg_details, const int itrack, const int istart, const int chan_num, const int seg_max) {
    if (seg_details[itrack].longline()) {
        return seg_max;
    }

    if (seg_details[itrack].seg_end() >= 0) {
        return seg_details[itrack].seg_end();
    }

    int len = seg_details[itrack].length();
    int ofs = seg_details[itrack].start();

    /* Normal endpoint */
    int seg_end = istart + len - 1;

    /* If start is against edge it may have been clipped */
    if (1 == istart) {
        /* If the (staggered) startpoint of first full wire wasn't
         * also 1, we must be the clipped wire */
        int first_full = (len - (chan_num % len) + ofs - 1) % len + 1;
        if (first_full > 1) {
            /* then we stop just before the first full seg */
            seg_end = first_full - 1;
        }
    }

    /* Clip against far edge */
    if (seg_end > seg_max) {
        seg_end = seg_max;
    }

    return seg_end;
}

/* Returns the number of tracks to which clb opin #ipin at (i,j) connects.   *
 * Also stores the nodes to which this pin connects in rr_edges_to_create    */
int get_bidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                               const int opin_layer,
                               const int track_layer,
                               const int i,
                               const int j,
                               const int ipin,
                               RRNodeId from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_pin_to_track_lookup& opin_to_track_map,
                               const t_chan_details& chan_details_x,
                               const t_chan_details& chan_details_y) {
    int num_conn, tr_i, tr_j, chan, seg;
    int to_switch;
    int is_connected_track;
    t_physical_tile_type_ptr type;
    t_rr_type to_type;

    auto& device_ctx = g_vpr_ctx.device();

    type = device_ctx.grid.get_physical_type({i, j, track_layer});
    int width_offset = device_ctx.grid.get_width_offset({i, j, track_layer});
    int height_offset = device_ctx.grid.get_height_offset({i, j, track_layer});

    num_conn = 0;

    /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
    for (e_side side : SIDES) {
        /* Figure out coords of channel segment based on side */
        tr_i = ((side == LEFT) ? (i - 1) : i);
        tr_j = ((side == BOTTOM) ? (j - 1) : j);

        to_type = ((side == LEFT) || (side == RIGHT)) ? CHANY : CHANX;

        chan = ((to_type == CHANX) ? tr_j : tr_i);
        seg = ((to_type == CHANX) ? tr_i : tr_j);

        bool vert = !((side == TOP) || (side == BOTTOM));

        /* Don't connect where no tracks on fringes */
        if ((tr_i < 0) || (tr_i > int(device_ctx.grid.width() - 2))) { //-2 for no perimeter channels
            continue;
        }
        if ((tr_j < 0) || (tr_j > int(device_ctx.grid.height() - 2))) { //-2 for no perimeter channels
            continue;
        }
        if ((CHANX == to_type) && (tr_i < 1)) {
            continue;
        }
        if ((CHANY == to_type) && (tr_j < 1)) {
            continue;
        }
        if (opin_to_track_map[type->index].empty()) {
            continue;
        }

        is_connected_track = false;

        const t_chan_seg_details* seg_details = (vert ? chan_details_y[chan][seg] : chan_details_x[seg][chan]).data();

        /* Iterate of the opin to track connections */

        for (int to_track : opin_to_track_map[type->index][ipin][width_offset][height_offset][track_layer][side]) {
            /* Skip unconnected connections */
            if (OPEN == to_track || is_connected_track) {
                is_connected_track = true;
                VTR_ASSERT(OPEN == opin_to_track_map[type->index][ipin][width_offset][height_offset][track_layer][side][0]);
                continue;
            }

            /* Only connect to wire if there is a CB */
            if (is_cblock(chan, seg, to_track, seg_details)) {
                RRNodeId to_node = rr_graph_builder.node_lookup().find_node(track_layer, tr_i, tr_j, to_type, to_track);

                if (!to_node) {
                    continue;
                }

                to_switch = (track_layer == opin_layer) ? seg_details[to_track].arch_wire_switch() : seg_details[to_track].arch_opin_between_dice_switch();
                rr_edges_to_create.emplace_back(from_rr_node, to_node, to_switch, false);

                ++num_conn;
            }
        }
    }

    return num_conn;
}

/* AA: Actually builds the edges from the OPIN nodes already allocated to their correct tracks for segment seg_Inf[seg_type_index]. 
 * Note that this seg_inf vector is NOT the segment_info vectored as stored in the device variable. This index is w.r.t to seg_inf_x
 * or seg_inf_y for x-adjacent and y-adjacent segments respectively. This index is assigned in get_seg_details earlier 
 * in the rr_graph_builder routine. This t_seg_detail is then used to build t_chan_seg_details which is passed in to label_wire mux
 * routine used in this function. 
 *
 * 
 */
int get_unidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                                const int opin_layer,
                                const int track_layer,
                                const int chan,
                                const int seg,
                                int Fc,
                                const int seg_type_index,
                                const t_rr_type chan_type,
                                const t_chan_seg_details* seg_details,
                                RRNodeId from_rr_node,
                                t_rr_edge_info_set& rr_edges_to_create,
                                vtr::NdMatrix<int, 3>& Fc_ofs,
                                const int max_len,
                                const t_chan_width& nodes_per_chan,
                                bool* Fc_clipped) {
    /* Gets a linked list of Fc nodes of specified seg_type_index to connect
     * to in given chan seg. Fc_ofs is used for the opin staggering pattern. */

    int num_inc_muxes, num_dec_muxes, iconn;
    int inc_mux, dec_mux;
    int inc_track, dec_track;
    int x, y;
    int num_edges;

    *Fc_clipped = false;

    /* Fc is assigned in pairs so check it is even. */
    VTR_ASSERT(Fc % 2 == 0);

    /* get_rr_node_indices needs x and y coords. */
    x = ((CHANX == chan_type) ? seg : chan);
    y = ((CHANX == chan_type) ? chan : seg);

    /* Get the lists of possible muxes. */
    int dummy;
    std::vector<int> inc_muxes;
    std::vector<int> dec_muxes;
    /* AA: Determine the channel width instead of using max channels to not create hanging nodes*/
    int max_chan_width = (CHANX == chan_type) ? nodes_per_chan.x_list[y] : nodes_per_chan.y_list[x];

    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     Direction::INC, max_chan_width, true, inc_muxes, &num_inc_muxes, &dummy);
    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     Direction::DEC, max_chan_width, true, dec_muxes, &num_dec_muxes, &dummy);

    /* Clip Fc to the number of muxes. */
    if (((Fc / 2) > num_inc_muxes) || ((Fc / 2) > num_dec_muxes)) {
        *Fc_clipped = true;
        Fc = 2 * std::min(num_inc_muxes, num_dec_muxes);
    }

    /* Assign tracks to meet Fc demand */
    num_edges = 0;
    for (iconn = 0; iconn < (Fc / 2); ++iconn) {
        /* Figure of the next mux to use for the 'inc' and 'dec' connections */
        inc_mux = Fc_ofs[chan][seg][seg_type_index] % num_inc_muxes;
        dec_mux = Fc_ofs[chan][seg][seg_type_index] % num_dec_muxes;
        ++Fc_ofs[chan][seg][seg_type_index];

        /* Figure out the track it corresponds to. */
        inc_track = inc_muxes[inc_mux];
        dec_track = dec_muxes[dec_mux];

        /* Figure the inodes of those muxes */
        RRNodeId inc_inode_index = rr_graph_builder.node_lookup().find_node(track_layer, x, y, chan_type, inc_track);
        RRNodeId dec_inode_index = rr_graph_builder.node_lookup().find_node(track_layer, x, y, chan_type, dec_track);

        if (!inc_inode_index || !dec_inode_index) {
            continue;
        }

        /* Add to the list. */
        auto to_switch = (opin_layer == track_layer) ? seg_details[inc_track].arch_opin_switch() : seg_details[inc_track].arch_opin_between_dice_switch();
        rr_edges_to_create.emplace_back(from_rr_node, inc_inode_index, to_switch, false);
        ++num_edges;

        to_switch = (opin_layer == track_layer) ? seg_details[dec_track].arch_opin_switch() : seg_details[dec_track].arch_opin_between_dice_switch();
        rr_edges_to_create.emplace_back(from_rr_node, dec_inode_index, to_switch, false);
        ++num_edges;
    }

    return num_edges;
}

bool is_cblock(const int chan, const int seg, const int track, const t_chan_seg_details* seg_details) {
    int length, ofs, start_seg;

    length = seg_details[track].length();

    /* Make sure they gave us correct start */
    start_seg = get_seg_start(seg_details, track, chan, seg);

    ofs = seg - start_seg;

    VTR_ASSERT(ofs >= 0);
    VTR_ASSERT(ofs < length);

    /* If unidir segment that is going backwards, we need to flip the ofs */
    if (Direction::DEC == seg_details[track].direction()) {
        ofs = (length - 1) - ofs;
    }

    return seg_details[track].cb(ofs);
}

void dump_seg_details(const t_chan_seg_details* seg_details,
                      int max_chan_width,
                      FILE* fp) {
    for (int i = 0; i < max_chan_width; i++) {
        fprintf(fp, "track: %d\n", i);
        fprintf(fp, "length: %d  start: %d",
                seg_details[i].length(), seg_details[i].start());

        if (seg_details[i].length() > 0) {
            if (seg_details[i].seg_start() >= 0 && seg_details[i].seg_end() >= 0) {
                fprintf(fp, " [%d,%d]",
                        seg_details[i].seg_start(), seg_details[i].seg_end());
            }
            fprintf(fp, "  longline: %d  arch_wire_switch: %d  arch_opin_switch: %d",
                    seg_details[i].longline(),
                    seg_details[i].arch_wire_switch(), seg_details[i].arch_opin_switch());
        }
        fprintf(fp, "\n");

        fprintf(fp, "Rmetal: %g  Cmetal: %g\n",
                seg_details[i].Rmetal(), seg_details[i].Cmetal());

        fprintf(fp, "direction: %s\n",
                DIRECTION_STRING[static_cast<int>(seg_details[i].direction())]);

        fprintf(fp, "cb list:  ");
        for (int j = 0; j < seg_details[i].length(); j++)
            fprintf(fp, "%d ", seg_details[i].cb(j));
        fprintf(fp, "\n");

        fprintf(fp, "sb list: ");
        for (int j = 0; j <= seg_details[i].length(); j++)
            fprintf(fp, "%d ", seg_details[i].sb(j));
        fprintf(fp, "\n");

        fprintf(fp, "\n");
    }
}

/* Dumps out an array of seg_details structures to file fname.  Used only   *
 * for debugging.                                                           */
void dump_seg_details(const t_chan_seg_details* seg_details,
                      int max_chan_width,
                      const char* fname) {
    FILE* fp = vtr::fopen(fname, "w");
    dump_seg_details(seg_details, max_chan_width, fp);
    fclose(fp);
}

/* Dumps out a 2D array of chan_details structures to file fname.  Used     *
 * only for debugging.                                                      */
void dump_chan_details(const t_chan_details& chan_details_x,
                       const t_chan_details& chan_details_y,
                       const t_chan_width* nodes_per_chan,
                       const DeviceGrid& grid,
                       const char* fname) {
    FILE* fp = vtr::fopen(fname, "w");
    if (fp) {
        fprintf(fp, "************************\n");
        fprintf(fp, "max_chan_width= %d | max_chan_width_y= %d | max_chan_width_x= %d", nodes_per_chan->max, nodes_per_chan->y_max, nodes_per_chan->x_max);
        fprintf(fp, "************************\n");
        for (size_t y = 0; y <= grid.height() - 2; ++y) {    //-2 for no perim channels
            for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_x: [%zu][%zu]\n", x, y);
                fprintf(fp, "channel_width: %d\n", nodes_per_chan->x_list[y]);
                fprintf(fp, "========================\n");

                const t_chan_seg_details* seg_details = chan_details_x[x][y].data();
                dump_seg_details(seg_details, nodes_per_chan->x_max, fp);
            }
        }
        for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
            for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_y: [%zu][%zu]\n", x, y);
                fprintf(fp, "channel_width: %d\n", nodes_per_chan->y_list[x]);
                fprintf(fp, "========================\n");

                const t_chan_seg_details* seg_details = chan_details_y[x][y].data();
                dump_seg_details(seg_details, nodes_per_chan->y_max, fp);
            }
        }
    }
    fclose(fp);
}

/* Dumps out a 2D array of switch block pattern structures to file fname. *
 * Used for debugging purposes only.                                      */
void dump_sblock_pattern(const t_sblock_pattern& sblock_pattern,
                         int max_chan_width,
                         const DeviceGrid& grid,
                         const char* fname) {
    FILE* fp = vtr::fopen(fname, "w");
    if (fp) {
        for (size_t y = 0; y <= grid.height() - 2; ++y) {
            for (size_t x = 0; x <= grid.width() - 2; ++x) {
                fprintf(fp, "==========================\n");
                fprintf(fp, "sblock_pattern: [%zu][%zu]\n", x, y);
                fprintf(fp, "==========================\n");

                for (int from_side = 0; from_side < 4; ++from_side) {
                    for (int to_side = 0; to_side < 4; ++to_side) {
                        if (from_side == to_side)
                            continue;

                        const char* psz_from_side = "?";
                        switch (from_side) {
                            case 0:
                                psz_from_side = "T";
                                break;
                            case 1:
                                psz_from_side = "R";
                                break;
                            case 2:
                                psz_from_side = "B";
                                break;
                            case 3:
                                psz_from_side = "L";
                                break;
                            default:
                                VTR_ASSERT_MSG(false, "Unrecognized from side");
                                break;
                        }
                        const char* psz_to_side = "?";
                        switch (to_side) {
                            case 0:
                                psz_to_side = "T";
                                break;
                            case 1:
                                psz_to_side = "R";
                                break;
                            case 2:
                                psz_to_side = "B";
                                break;
                            case 3:
                                psz_to_side = "L";
                                break;
                            default:
                                VTR_ASSERT_MSG(false, "Unrecognized to side");
                                break;
                        }

                        for (int from_track = 0; from_track < max_chan_width; ++from_track) {
                            short to_mux = sblock_pattern[x][y][from_side][to_side][from_track][0];
                            short to_track = sblock_pattern[x][y][from_side][to_side][from_track][1];
                            short alt_mux = sblock_pattern[x][y][from_side][to_side][from_track][2];
                            short alt_track = sblock_pattern[x][y][from_side][to_side][from_track][3];

                            if (to_mux == UN_SET && to_track == UN_SET)
                                continue;

                            if (alt_mux == UN_SET && alt_track == UN_SET) {
                                fprintf(fp, "%s %d => %s [%d][%d]\n",
                                        psz_from_side, from_track, psz_to_side,
                                        to_mux, to_track);
                            } else {
                                fprintf(fp, "%s %d => %s [%d][%d] [%d][%d]\n",
                                        psz_from_side, from_track, psz_to_side,
                                        to_mux, to_track, alt_mux, alt_track);
                            }
                        }
                    }
                }
            }
        }
    }
    fclose(fp);
}

void dump_track_to_pin_map(t_track_to_pin_lookup& track_to_pin_map,
                           const std::vector<t_physical_tile_type>& types,
                           int max_chan_width,
                           FILE* fp) {
    if (fp) {
        auto& device_ctx = g_vpr_ctx.device();
        for (unsigned int i = 0; i < types.size(); i++) {
            if (!track_to_pin_map[i].empty()) {
                for (int layer = 0; layer < device_ctx.grid.get_num_layers(); layer++) {
                    for (int track = 0; track < max_chan_width; ++track) {
                        for (int width = 0; width < types[i].width; ++width) {
                            for (int height = 0; height < types[i].height; ++height) {
                                for (int side = 0; side < 4; ++side) {
                                    fprintf(fp, "\nTYPE:%s width:%d height:%d layer:%d\n", types[i].name, width, height, layer);
                                    fprintf(fp, "\nSIDE:%d TRACK:%d \n", side, track);
                                    for (size_t con = 0; con < track_to_pin_map[i][track][width][height][layer][side].size(); con++) {
                                        fprintf(fp, "%d ", track_to_pin_map[i][track][width][height][layer][side][con]);
                                    }
                                    fprintf(fp, "\n=====================\n");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
static void load_chan_rr_indices(const int max_chan_width,
                                 const DeviceGrid& grid,
                                 const int chan_len,
                                 const int num_chans,
                                 const t_rr_type type,
                                 const t_chan_details& chan_details,
                                 RRGraphBuilder& rr_graph_builder,
                                 int* index) {
    auto& device_ctx = g_vpr_ctx.device();

    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        /* Skip the current die if architecture file specifies that it doesn't require global resource routing */
        if (!device_ctx.inter_cluster_prog_routing_resources.at(layer)) {
            continue;
        }
        for (int chan = 0; chan < num_chans - 1; ++chan) {
            for (int seg = 1; seg < chan_len - 1; ++seg) {
                /* Assign an inode to the starts of tracks */
                int x = (type == CHANX ? seg : chan);
                int y = (type == CHANX ? chan : seg);
                const t_chan_seg_details* seg_details = chan_details[x][y].data();

                /* Reserve nodes in lookup to save memory */
                rr_graph_builder.node_lookup().reserve_nodes(layer, chan, seg, type, max_chan_width);

                for (int track = 0; track < max_chan_width; ++track) {
                    /* TODO: May let the length() == 0 case go through, to model muxes */
                    if (seg_details[track].length() <= 0)
                        continue;

                    int start = get_seg_start(seg_details, track, chan, seg);

                    /* TODO: Now we still use the (y, x) convention here for CHANX. Should rework later */
                    int node_x = chan;
                    int node_y = start;
                    if (CHANX == type) {
                        std::swap(node_x, node_y);
                    }

                    /* If the start of the wire doesn't have an inode,
                     * assign one to it. */
                    RRNodeId inode = rr_graph_builder.node_lookup().find_node(layer, node_x, node_y, type, track);
                    if (!inode) {
                        inode = RRNodeId(*index);
                        ++(*index);
                        rr_graph_builder.node_lookup().add_node(inode, layer, chan, start, type, track);
                    }

                    /* Assign inode of start of wire to current position */
                    rr_graph_builder.node_lookup().add_node(inode, layer, chan, seg, type, track);
                }
            }
        }
    }
}

/* As the rr_indices builders modify a local copy of indices, use the local copy in the builder 
 * TODO: these building functions should only talk to a RRGraphBuilder object
 */
static void load_block_rr_indices(RRGraphBuilder& rr_graph_builder,
                                  const DeviceGrid& grid,
                                  int* index,
                                  bool /*is_flat*/) {
    //Walk through the grid assigning indices to SOURCE/SINK IPIN/OPIN
    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        for (int x = 0; x < (int)grid.width(); x++) {
            for (int y = 0; y < (int)grid.height(); y++) {
                //Process each block from its root location
                if (grid.get_width_offset({x, y, layer}) == 0 && grid.get_height_offset({x, y, layer}) == 0) {
                    t_physical_tile_type_ptr physical_type = grid.get_physical_type({x,
                                                                                     y,
                                                                                     layer});
                    //Assign indices for SINKs and SOURCEs
                    // Note that SINKS/SOURCES have no side, so we always use side 0
                    std::vector<int> class_num_vec;
                    std::vector<int> pin_num_vec;

                    class_num_vec = get_tile_root_classes(physical_type);
                    pin_num_vec = get_tile_root_pins(physical_type);
                    add_classes_spatial_lookup(rr_graph_builder,
                                               physical_type,
                                               class_num_vec,
                                               layer,
                                               x,
                                               y,
                                               physical_type->width,
                                               physical_type->height,
                                               index);

                    /* Limited sides for grids
                     *   The wanted side depends on the location of the grid.
                     *   In particular for perimeter grid,
                     *   -------------------------------------------------------
                     *   Grid location |  IPIN side
                     *   -------------------------------------------------------
                     *   TOP           |  BOTTOM
                     *   -------------------------------------------------------
                     *   RIGHT         |  LEFT
                     *   -------------------------------------------------------
                     *   BOTTOM        |  TOP
                     *   -------------------------------------------------------
                     *   LEFT          |  RIGHT
                     *   -------------------------------------------------------
                     *   TOP-LEFT      |  BOTTOM & RIGHT
                     *   -------------------------------------------------------
                     *   TOP-RIGHT     |  BOTTOM & LEFT
                     *   -------------------------------------------------------
                     *   BOTTOM-LEFT   |  TOP & RIGHT
                     *   -------------------------------------------------------
                     *   BOTTOM-RIGHT  |  TOP & LEFT
                     *   -------------------------------------------------------
                     *   Other         |  First come first fit
                     *   -------------------------------------------------------
                     *
                     * Special for IPINs:
                     *   If there are multiple wanted sides, first come first fit is applied
                     *   This guarantee that there is only a unique rr_node
                     *   for the same input pin on multiple sides, and thus avoid multiple driver problems
                     */
                    std::vector<e_side> wanted_sides;
                    if ((int)grid.height() - 1 == y) { /* TOP side */
                        wanted_sides.push_back(BOTTOM);
                    }
                    if ((int)grid.width() - 1 == x) { /* RIGHT side */
                        wanted_sides.push_back(LEFT);
                    }
                    if (0 == y) { /* BOTTOM side */
                        wanted_sides.push_back(TOP);
                    }
                    if (0 == x) { /* LEFT side */
                        wanted_sides.push_back(RIGHT);
                    }

                    /* If wanted sides is empty still, this block does not have specific wanted sides,
                     * Deposit all the sides
                     */
                    if (wanted_sides.empty()) {
                        for (e_side side : {TOP, BOTTOM, LEFT, RIGHT}) {
                            wanted_sides.push_back(side);
                        }
                    }

                    add_pins_spatial_lookup(rr_graph_builder,
                                            physical_type,
                                            pin_num_vec,
                                            layer,
                                            x,
                                            y,
                                            index,
                                            wanted_sides);
                }
            }
        }
    }
}
static void add_pins_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                    t_physical_tile_type_ptr physical_type_ptr,
                                    const std::vector<int>& pin_num_vec,
                                    int layer,
                                    int root_x,
                                    int root_y,
                                    int* index,
                                    const std::vector<e_side>& wanted_sides) {
    for (e_side side : wanted_sides) {
        for (int width_offset = 0; width_offset < physical_type_ptr->width; ++width_offset) {
            int x_tile = root_x + width_offset;
            for (int height_offset = 0; height_offset < physical_type_ptr->height; ++height_offset) {
                int y_tile = root_y + height_offset;
                //only nodes on the tile may be located in a location other than the root-location
                rr_graph_builder.node_lookup().reserve_nodes(layer, x_tile, y_tile, OPIN, physical_type_ptr->num_pins, side);
                rr_graph_builder.node_lookup().reserve_nodes(layer, x_tile, y_tile, IPIN, physical_type_ptr->num_pins, side);
            }
        }
    }

    for (auto pin_num : pin_num_vec) {
        bool assigned_to_rr_node = false;
        std::vector<int> x_offset;
        std::vector<int> y_offset;
        std::vector<e_side> pin_sides;
        std::tie(x_offset, y_offset, pin_sides) = get_pin_coordinates(physical_type_ptr, pin_num, wanted_sides);
        auto pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
        for (int pin_coord_idx = 0; pin_coord_idx < (int)pin_sides.size(); pin_coord_idx++) {
            int x_tile = root_x + x_offset[pin_coord_idx];
            int y_tile = root_y + y_offset[pin_coord_idx];
            auto side = pin_sides[pin_coord_idx];
            if (pin_type == DRIVER) {
                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), layer, x_tile, y_tile, OPIN, pin_num, side);
                assigned_to_rr_node = true;
            } else {
                VTR_ASSERT(pin_type == RECEIVER);
                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), layer, x_tile, y_tile, IPIN, pin_num, side);
                assigned_to_rr_node = true;
            }
        }
        /* A pin may locate on multiple sides of a tile.
         * Instead of allocating multiple rr_nodes for the pin,
         * we just create a rr_node and make it indexable on these sides
         * As such, we can avoid redundant rr_node to be allocated
         * and multiple nets to be mapped to the pin
         *
         * Considering that some pin could be just dangling, we do not need
         * to create a void rr_node for it.
         * As such, we only allocate a rr node when the pin is indeed located
         * on at least one side
         */
        if (assigned_to_rr_node) {
            ++(*index);
        }
    }
}

static void add_classes_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                       t_physical_tile_type_ptr physical_type_ptr,
                                       const std::vector<int>& class_num_vec,
                                       int layer,
                                       int root_x,
                                       int root_y,
                                       int block_width,
                                       int block_height,
                                       int* index) {
    for (int x_tile = root_x; x_tile < (root_x + block_width); x_tile++) {
        for (int y_tile = root_y; y_tile < (root_y + block_height); y_tile++) {
            rr_graph_builder.node_lookup().reserve_nodes(layer, x_tile, y_tile, SOURCE, class_num_vec.size(), SIDES[0]);
            rr_graph_builder.node_lookup().reserve_nodes(layer, x_tile, y_tile, SINK, class_num_vec.size(), SIDES[0]);
        }
    }

    for (auto class_num : class_num_vec) {
        auto class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        e_rr_type node_type = SINK;
        if (class_type == DRIVER) {
            node_type = SOURCE;
        } else {
            VTR_ASSERT(class_type == RECEIVER);
        }

        for (int x_offset = 0; x_offset < block_width; x_offset++) {
            for (int y_offset = 0; y_offset < block_height; y_offset++) {
                int curr_x = root_x + x_offset;
                int curr_y = root_y + y_offset;

                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), layer, curr_x, curr_y, node_type, class_num);
            }
        }

        ++(*index);
    }
}

/* As the rr_indices builders modify a local copy of indices, use the local copy in the builder 
 * TODO: these building functions should only talk to a RRGraphBuilder object
 *       The biggest and fatal issue is 
 *       - the rr_graph2.h is included in the rr_graph_storage.h,
 *         which is included in the rr_graph_builder.h
 *         If we include rr_graph_builder.h in rr_graph2.h, this creates a loop
 *         for C++ compiler to identify data structures, which cannot be solved!!!
 *         This will block us when putting the RRGraphBuilder object as an input arguement 
 *         of this function
 */
void alloc_and_load_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                    const t_chan_width* nodes_per_chan,
                                    const DeviceGrid& grid,
                                    int* index,
                                    const t_chan_details& chan_details_x,
                                    const t_chan_details& chan_details_y,
                                    bool is_flat) {
    /* Allocates and loads all the structures needed for fast lookups of the   *
     * index of an rr_node.  rr_node_indices is a matrix containing the index  *
     * of the *first* rr_node at a given (i,j) location.                       */

    /* Alloc the lookup table */
    for (t_rr_type rr_type : RR_TYPES) {
        if (rr_type == CHANX) {
            rr_graph_builder.node_lookup().resize_nodes(grid.get_num_layers(), grid.height(), grid.width(), rr_type, NUM_SIDES);
        } else {
            rr_graph_builder.node_lookup().resize_nodes(grid.get_num_layers(), grid.width(), grid.height(), rr_type, NUM_SIDES);
        }
    }

    /* Assign indices for block nodes */
    load_block_rr_indices(rr_graph_builder, grid, index, is_flat);

    /* Load the data for x and y channels */
    load_chan_rr_indices(nodes_per_chan->x_max, grid, grid.width(), grid.height(),
                         CHANX, chan_details_x, rr_graph_builder, index);
    load_chan_rr_indices(nodes_per_chan->y_max, grid, grid.height(), grid.width(),
                         CHANY, chan_details_y, rr_graph_builder, index);
}

void alloc_and_load_intra_cluster_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& pin_chains_num,
                                                  int* index) {
    for (int layer = 0; layer < grid.get_num_layers(); layer++) {
        for (int x = 0; x < (int)grid.width(); x++) {
            for (int y = 0; y < (int)grid.height(); y++) {
                //Process each block from it's root location
                if (grid.get_width_offset({x, y, layer}) == 0 && grid.get_height_offset({x, y, layer}) == 0) {
                    t_physical_tile_type_ptr physical_type = grid.get_physical_type({x, y, layer});
                    //Assign indices for SINKs and SOURCEs
                    // Note that SINKS/SOURCES have no side, so we always use side 0
                    std::vector<int> class_num_vec;
                    std::vector<int> pin_num_vec;
                    class_num_vec = get_cluster_netlist_intra_tile_classes_at_loc(layer, x, y, physical_type);
                    pin_num_vec = get_cluster_netlist_intra_tile_pins_at_loc(layer,
                                                                             x,
                                                                             y,
                                                                             pin_chains,
                                                                             pin_chains_num,
                                                                             physical_type);
                    add_classes_spatial_lookup(rr_graph_builder,
                                               physical_type,
                                               class_num_vec,
                                               layer,
                                               x,
                                               y,
                                               physical_type->width,
                                               physical_type->height,
                                               index);

                    std::vector<e_side> wanted_sides;
                    wanted_sides.push_back(e_side::TOP);
                    add_pins_spatial_lookup(rr_graph_builder,
                                            physical_type,
                                            pin_num_vec,
                                            layer,
                                            x,
                                            y,
                                            index,
                                            wanted_sides);
                }
            }
        }
    }
}

/**
 * Validate the node look-up matches all the node-level information 
 * in the storage of a routing resource graph
 * This function will check the following aspects:
 * - The type of each node matches its type that is indexed in the node look-up
 * - For bounding box (xlow, ylow, xhigh, yhigh) of each node is indexable in the node look-up
 * - The number of unique indexable nodes in the node look up matches the number of nodes in the storage
 *   This ensures that every node in the storage is indexable and there are no hidden nodes in the look-up
 */
bool verify_rr_node_indices(const DeviceGrid& grid,
                            const RRGraphView& rr_graph,
                            const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const t_rr_graph_storage& rr_nodes,
                            bool is_flat) {
    std::unordered_map<RRNodeId, int> rr_node_counts;

    int width = grid.width();
    int height = grid.height();
    int layer = grid.get_num_layers();

    for (int l = 0; l < layer; ++l) {
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                for (t_rr_type rr_type : RR_TYPES) {
                    /* Get the list of nodes at a specific location (x, y) */
                    std::vector<RRNodeId> nodes_from_lookup;
                    if (rr_type == CHANX || rr_type == CHANY) {
                        nodes_from_lookup = rr_graph.node_lookup().find_channel_nodes(l, x, y, rr_type);
                    } else {
                        nodes_from_lookup = rr_graph.node_lookup().find_grid_nodes_at_all_sides(l, x, y, rr_type);
                    }
                    for (RRNodeId inode : nodes_from_lookup) {
                        rr_node_counts[inode]++;

                        if (rr_graph.node_type(inode) != rr_type) {
                            VPR_ERROR(VPR_ERROR_ROUTE, "RR node type does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                                      rr_node_typename[rr_graph.node_type(inode)],
                                      rr_node_typename[rr_type],
                                      describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                        }

                        if (rr_graph.node_type(inode) == CHANX) {
                            VTR_ASSERT_MSG(rr_graph.node_ylow(inode) == rr_graph.node_yhigh(inode), "CHANX should be horizontal");

                            if (y != rr_graph.node_ylow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.x_in_node_range(x, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else if (rr_graph.node_type(inode) == CHANY) {
                            VTR_ASSERT_MSG(rr_graph.node_xlow(inode) == rr_graph.node_xhigh(inode), "CHANY should be veritcal");

                            if (x != rr_graph.node_xlow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.y_in_node_range(y, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else if (rr_graph.node_type(inode) == SOURCE || rr_graph.node_type(inode) == SINK) {
                            // Sources have co-ordinates covering the entire block they are in, but not sinks
                            if (!rr_graph.x_in_node_range(x, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.y_in_node_range(y, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else {
                            VTR_ASSERT(rr_graph.node_type(inode) == IPIN || rr_graph.node_type(inode) == OPIN);
                            /* As we allow a pin to be indexable on multiple sides,
                             * This check code should be invalid
                             * if (rr_node.xlow() != x) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node xlow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.xlow(),
                             *               x,
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * }
                             *
                             * if (rr_node.ylow() != y) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node ylow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.ylow(),
                             *               y,
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * }
                             */
                        }

                        if (rr_type == IPIN || rr_type == OPIN) {
                            /* As we allow a pin to be indexable on multiple sides,
                             * This check code should be invalid
                             * if (rr_node.side() != side) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node xlow does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                             *               SIDE_STRING[rr_node.side()],
                             *               SIDE_STRING[side],
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * } else {
                             *     VTR_ASSERT(rr_node.side() == side);
                             * }
                             */
                        }
                    }
                }
            }
        }
    }

    if (rr_node_counts.size() != rr_nodes.size()) {
        VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch in number of unique RR nodes in rr_nodes (%zu) and rr_node_indices (%zu)",
                  rr_nodes.size(),
                  rr_node_counts.size());
    }

    for (auto kv : rr_node_counts) {
        RRNodeId inode = kv.first;
        int count = kv.second;

        auto& rr_node = rr_nodes[size_t(inode)];

        if (rr_graph.node_type(inode) == SOURCE || rr_graph.node_type(inode) == SINK) {
            int rr_width = (rr_graph.node_xhigh(rr_node.id()) - rr_graph.node_xlow(rr_node.id()) + 1);
            int rr_height = (rr_graph.node_yhigh(rr_node.id()) - rr_graph.node_ylow(rr_node.id()) + 1);
            int rr_area = rr_width * rr_height;
            if (count != rr_area) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node size (%d) and count within rr_node_indices (%d): %s",
                          rr_area,
                          rr_node.length(),
                          count,
                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
            }
            /* As we allow a pin to be indexable on multiple sides,
             * This check code should not be applied to input and output pins
             */
        } else if ((OPIN != rr_graph.node_type(inode)) && (IPIN != rr_graph.node_type(inode))) {
            if (count != rr_node.length() + 1) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node length (%d) and count within rr_node_indices (%d, should be length + 1): %s",
                          rr_node.length(),
                          count,
                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
            }
        }
    }

    return true;
}

int get_track_to_pins(RRGraphBuilder& rr_graph_builder,
                      int layer,
                      int seg,
                      int chan,
                      int track,
                      int tracks_per_chan,
                      RRNodeId from_rr_node,
                      t_rr_edge_info_set& rr_edges_to_create,
                      const t_track_to_pin_lookup& track_to_pin_lookup,
                      const t_chan_seg_details* seg_details,
                      enum e_rr_type chan_type,
                      int chan_length,
                      int wire_to_ipin_switch,
                      int wire_to_pin_between_dice_switch,
                      enum e_directionality directionality) {
    /*
     * Adds the fan-out edges from wire segment at (chan, seg, track) to adjacent
     * blocks along the wire's length
     */

    int j, pass, iconn, phy_track, end, max_conn, ipin, x, y, num_conn;

    auto& device_ctx = g_vpr_ctx.device();

    /* End of this wire */
    end = get_seg_end(seg_details, track, seg, chan, chan_length);

    num_conn = 0;

    for (j = seg; j <= end; j++) {
        if (is_cblock(chan, j, track, seg_details)) {
            for (pass = 0; pass < 2; ++pass) { //pass == 0 => TOP/RIGHT, pass == 1 => BOTTOM/LEFT
                e_side side;
                if (CHANX == chan_type) {
                    x = j;
                    y = chan + pass;
                    side = (0 == pass ? TOP : BOTTOM);
                } else {
                    VTR_ASSERT(CHANY == chan_type);
                    x = chan + pass;
                    y = j;
                    side = (0 == pass ? RIGHT : LEFT);
                }

                for (int layer_index = 0; layer_index < device_ctx.grid.get_num_layers(); layer_index++) {
                    /* PAJ - if the pointed to is an EMPTY then shouldn't look for ipins */
                    auto type = device_ctx.grid.get_physical_type({x, y, layer_index});
                    if (type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE)
                        continue;

                    /* Move from logical (straight) to physical (twisted) track index
                     * - algorithm assigns ipin connections to same physical track index
                     * so that the logical track gets distributed uniformly */

                    phy_track = vpr_to_phy_track(track, chan, j, seg_details, directionality);
                    phy_track %= tracks_per_chan;

                    /* We need the type to find the ipin map for this type */

                    int width_offset = device_ctx.grid.get_width_offset({x, y, layer_index});
                    int height_offset = device_ctx.grid.get_height_offset({x, y, layer_index});

                    max_conn = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][layer][side].size();
                    for (iconn = 0; iconn < max_conn; iconn++) {
                        ipin = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][layer][side][iconn];
                        if (!is_pin_conencted_to_layer(type, ipin, layer_index, layer, device_ctx.grid.get_num_layers())) {
                            continue;
                        }

                        /* Check there is a connection and Fc map isn't wrong */
                        /*int to_node = get_rr_node_index(L_rr_node_indices, x + width_offset, y + height_offset, IPIN, ipin, side);*/
                        RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer_index, x, y, IPIN, ipin, side);
                        int switch_type = (layer_index == layer) ? wire_to_ipin_switch : wire_to_pin_between_dice_switch;
                        if (to_node) {
                            rr_edges_to_create.emplace_back(from_rr_node, to_node, switch_type, false);
                            ++num_conn;
                        }
                    }
                }
            }
        }
    }
    return (num_conn);
}

/*
 * Collects the edges fanning-out of the 'from' track which connect to the 'to'
 * tracks, according to the switch block pattern.
 *
 * It returns the number of connections added, and updates edge_list_ptr to
 * point at the head of the (extended) linked list giving the nodes to which
 * this segment connects and the switch type used to connect to each.
 *
 * An edge is added from this segment to a y-segment if:
 * (1) this segment should have a switch box at that location, or
 * (2) the y-segment to which it would connect has a switch box, and the switch
 *     type of that y-segment is unbuffered (bidirectional pass transistor).
 *
 * For bidirectional:
 * If the switch in each direction is a pass transistor (unbuffered), both
 * switches are marked as being of the types of the larger (lower R) pass
 * transistor.
 */
int get_track_to_tracks(RRGraphBuilder& rr_graph_builder,
                        const int layer,
                        const int from_chan,
                        const int from_seg,
                        const int from_track,
                        const t_rr_type from_type,
                        const int to_seg,
                        const t_rr_type to_type,
                        const int chan_len,
                        const int max_chan_width,
                        const DeviceGrid& grid,
                        const int Fs_per_side,
                        t_sblock_pattern& sblock_pattern,
                        RRNodeId from_rr_node,
                        t_rr_edge_info_set& rr_edges_to_create,
                        const t_chan_seg_details* from_seg_details,
                        const t_chan_seg_details* to_seg_details,
                        const t_chan_details& to_chan_details,
                        const enum e_directionality directionality,
                        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                        t_sb_connection_map* sb_conn_map) {
    int to_chan, to_sb;
    std::vector<int> conn_tracks;
    bool from_is_sblock, is_behind, Fs_clipped;
    enum e_side from_side_a, from_side_b, to_side;
    bool custom_switch_block;

    /* check whether a custom switch block will be used */
    custom_switch_block = false;
    if (sb_conn_map) {
        custom_switch_block = true;
        VTR_ASSERT(switch_block_conn.empty());
    }

    VTR_ASSERT_MSG(from_seg == get_seg_start(from_seg_details, from_track, from_chan, from_seg), "From segment location must be a the wire start point");

    int from_switch = from_seg_details[from_track].arch_wire_switch();

    //The absolute coordinate along the channel where the switch block at the
    //beginning of the current wire segment is located
    int start_sb_seg = from_seg - 1;

    //The absolute coordinate along the channel where the switch block at the
    //end of the current wire segment is located
    int end_sb_seg = get_seg_end(from_seg_details, from_track, from_seg, from_chan, chan_len);

    /* Figure out the sides of SB the from_wire will use */
    if (CHANX == from_type) {
        from_side_a = RIGHT;
        from_side_b = LEFT;
    } else {
        VTR_ASSERT(CHANY == from_type);
        from_side_a = TOP;
        from_side_b = BOTTOM;
    }

    //Set the loop bounds so we iterate over the whole wire segment
    int start = start_sb_seg;
    int end = end_sb_seg;

    //If source and destination segments both lie along the same channel
    //we clip the loop bounds to the switch blocks of interest and proceed
    //normally
    if (to_type == from_type) {
        start = to_seg - 1;
        end = to_seg;
    }

    //Walk along the 'from' wire segment identifying if a switchblock is located
    //at each coordinate and add any related fan-out connections to the 'from' wire segment
    int num_conn = 0;
    for (int sb_seg = start; sb_seg <= end; ++sb_seg) {
        if (sb_seg < start_sb_seg || sb_seg > end_sb_seg) {
            continue;
        }

        /* Figure out if we are at a sblock */
        from_is_sblock = is_sblock(from_chan, from_seg, sb_seg, from_track,
                                   from_seg_details, directionality);
        if (sb_seg == end_sb_seg || sb_seg == start_sb_seg) {
            /* end of wire must be an sblock */
            from_is_sblock = true;
        }

        auto switch_override = should_create_switchblock(grid, layer, from_chan, sb_seg, from_type, to_type);
        if (switch_override == NO_SWITCH) {
            continue; //Do not create an SB here
        }

        /* Get the coordinates of the current SB from the perspective of the destination channel.
         * i.e. for segments laid in the x-direction, sb_seg corresponds to the x coordinate and from_chan to the y,
         * but for segments in the y-direction, from_chan is the x coordinate and sb_seg is the y. So here we reverse
         * the coordinates if necessary */
        if (from_type == to_type) {
            //Same channel
            to_chan = from_chan;
            to_sb = sb_seg;
        } else {
            VTR_ASSERT(from_type != to_type);
            //Different channels
            to_chan = sb_seg;
            to_sb = from_chan;
        }

        /* to_chan_details may correspond to an x-directed or y-directed channel, depending on which
         * channel type this function is used; so coordinates are reversed as necessary */
        if (to_type == CHANX) {
            to_seg_details = to_chan_details[to_seg][to_chan].data();
        } else {
            to_seg_details = to_chan_details[to_chan][to_seg].data();
        }

        if (to_seg_details[0].length() == 0)
            continue;

        /* Figure out whether the switch block at the current sb_seg coordinate is *behind*
         * the target channel segment (with respect to VPR coordinate system) */
        is_behind = false;
        if (to_type == from_type) {
            if (sb_seg == start) {
                is_behind = true;
            } else {
                is_behind = false;
            }
        } else {
            VTR_ASSERT((to_seg == from_chan) || (to_seg == (from_chan + 1)));
            if (to_seg > from_chan) {
                is_behind = true;
            }
        }

        /* Figure out which side of the SB the destination segment lies on */
        if (CHANX == to_type) {
            to_side = (is_behind ? RIGHT : LEFT);
        } else {
            VTR_ASSERT(CHANY == to_type);
            to_side = (is_behind ? TOP : BOTTOM);
        }

        /* To get to the destination seg/chan, the source track can connect to the SB from
         * one of two directions. If we're in CHANX, we can connect to it from the left or
         * right, provided we're not at a track endpoint. And similarly for a source track
         * in CHANY. */
        /* Do edges going from the right SB side (if we're in CHANX) or top (if we're in CHANY).
         * However, can't connect to right (top) if already at rightmost (topmost) track end */
        if (sb_seg < end_sb_seg) {
            if (custom_switch_block) {
                if (Direction::DEC == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan, to_seg,
                                                      to_type, from_side_a, to_side,
                                                      switch_override,
                                                      sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                     * switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_a][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(rr_graph_builder, conn_tracks, layer,
                                                            to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the top or right of SB so it
                     * makes the most sense to only get there from Direction::DEC wires. */
                    if ((from_is_sblock) && (Direction::DEC == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width, grid,
                                                                 from_side_a, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create);
                    }
                }
            }
        }

        /* Do the edges going from the left SB side (if we're in CHANX) or bottom (if we're in CHANY)
         * However, can't connect to left (bottom) if already at leftmost (bottommost) track end */
        if (sb_seg > start_sb_seg) {
            if (custom_switch_block) {
                if (Direction::INC == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan, to_seg,
                                                      to_type, from_side_b, to_side,
                                                      switch_override,
                                                      sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                     * switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_b][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(rr_graph_builder, conn_tracks, layer,
                                                            to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the bottom or left of SB so it
                     * makes the most sense to only get there from Direction::INC wires. */
                    if ((from_is_sblock)
                        && (Direction::INC == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(rr_graph_builder, layer, from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width, grid,
                                                                 from_side_b, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create);
                    }
                }
            }
        }
    }

    return num_conn;
}

void alloc_and_load_tile_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                         t_physical_tile_type_ptr physical_tile,
                                         int layer,
                                         int x,
                                         int y,
                                         int* num_rr_nodes) {
    std::vector<e_side> wanted_sides{TOP, BOTTOM, LEFT, RIGHT};
    auto class_num_range = get_flat_tile_primitive_classes(physical_tile);
    auto pin_num_vec = get_flat_tile_pins(physical_tile);

    std::vector<int> class_num_vec(class_num_range.total_num());
    std::iota(class_num_vec.begin(), class_num_vec.end(), class_num_range.low);

    add_classes_spatial_lookup(rr_graph_builder,
                               physical_tile,
                               class_num_vec,
                               layer,
                               x,
                               y,
                               physical_tile->width,
                               physical_tile->height,
                               num_rr_nodes);

    add_pins_spatial_lookup(rr_graph_builder,
                            physical_tile,
                            pin_num_vec,
                            layer,
                            x,
                            y,
                            num_rr_nodes,
                            wanted_sides);
}

static int get_bidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                       const std::vector<int> conn_tracks,
                                       const int layer,
                                       const int to_chan,
                                       const int to_seg,
                                       const int to_sb,
                                       const t_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       const bool from_is_sblock,
                                       const int from_switch,
                                       const int switch_override,
                                       const enum e_directionality directionality,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create) {
    unsigned iconn;
    int to_track, to_switch, num_conn, to_x, to_y, i;
    bool to_is_sblock;
    short switch_types[2];

    /* x, y coords for get_rr_node lookups */
    if (CHANX == to_type) {
        to_x = to_seg;
        to_y = to_chan;
    } else {
        VTR_ASSERT(CHANY == to_type);
        to_x = to_chan;
        to_y = to_seg;
    }

    /* Go through the list of tracks we can connect to */
    num_conn = 0;
    for (iconn = 0; iconn < conn_tracks.size(); ++iconn) {
        to_track = conn_tracks[iconn];
        RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_type, to_track);

        if (!to_node) {
            continue;
        }

        /* Get the switches for any edges between the two tracks */
        to_switch = seg_details[to_track].arch_wire_switch();

        to_is_sblock = is_sblock(to_chan, to_seg, to_sb, to_track, seg_details,
                                 directionality);
        get_switch_type(from_is_sblock, to_is_sblock, from_switch, to_switch,
                        switch_override,
                        switch_types);

        /* There are up to two switch edges allowed from track to track */
        for (i = 0; i < 2; ++i) {
            /* If the switch_type entry is empty, skip it */
            if (OPEN == switch_types[i]) {
                continue;
            }

            /* Add the edge to the list */
            rr_edges_to_create.emplace_back(from_rr_node, to_node, switch_types[i], false);
            ++num_conn;
        }
    }

    return num_conn;
}

/* Figures out the edges that should connect the given wire segment to the given
 * channel segment, adds these edges to 'edge_list' and returns the number of
 * edges added .
 * See route/build_switchblocks.c for a detailed description of how the switch block
 * connection map sb_conn_map is generated. */
static int get_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                 const int layer,
                                 const int from_wire,
                                 const int to_chan,
                                 const int to_seg,
                                 const t_rr_type to_chan_type,
                                 const e_side from_side,
                                 const e_side to_side,
                                 const int switch_override,
                                 t_sb_connection_map* sb_conn_map,
                                 RRNodeId from_rr_node,
                                 t_rr_edge_info_set& rr_edges_to_create) {
    int edge_count = 0;
    int to_x, to_y;
    int tile_x, tile_y;

    /* get x/y coordinates from seg/chan coordinates */
    if (CHANX == to_chan_type) {
        to_x = tile_x = to_seg;
        to_y = tile_y = to_chan;
        if (RIGHT == to_side) {
            tile_x--;
        }
    } else {
        VTR_ASSERT(CHANY == to_chan_type);
        to_x = tile_x = to_chan;
        to_y = tile_y = to_seg;
        if (TOP == to_side) {
            tile_y--;
        }
    }

    /* get coordinate to index into the SB map */
    Switchblock_Lookup sb_coord(tile_x, tile_y, from_side, to_side);
    if (sb_conn_map->count(sb_coord) > 0) {
        /* get reference to the connections vector which lists all destination wires for a given source wire
         * at a specific coordinate sb_coord */
        std::vector<t_switchblock_edge>& conn_vector = (*sb_conn_map)[sb_coord];

        /* go through the connections... */
        for (int iconn = 0; iconn < (int)conn_vector.size(); ++iconn) {
            if (conn_vector.at(iconn).from_wire != from_wire) continue;

            int to_wire = conn_vector.at(iconn).to_wire;
            RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_chan_type, to_wire);

            if (!to_node) {
                continue;
            }

            /* Get the index of the switch connecting the two wires */
            int src_switch = conn_vector[iconn].switch_ind;

            //Apply any switch overrides
            if (should_apply_switch_override(switch_override)) {
                src_switch = switch_override;
            }

            rr_edges_to_create.emplace_back(from_rr_node, to_node, src_switch, false);
            ++edge_count;

            auto& device_ctx = g_vpr_ctx.device();

            if (device_ctx.arch_switch_inf[src_switch].directionality() == BI_DIRECTIONAL) {
                //Add reverse edge since bi-directional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, src_switch, false);
                ++edge_count;
            }
        }
    } else {
        /* specified sb_conn_map entry does not exist -- do nothing */
    }
    return edge_count;
}

static int get_unidir_track_to_chan_seg(RRGraphBuilder& rr_graph_builder,
                                        const int layer,
                                        const int from_track,
                                        const int to_chan,
                                        const int to_seg,
                                        const int to_sb,
                                        const t_rr_type to_type,
                                        const int max_chan_width,
                                        const DeviceGrid& grid,
                                        const enum e_side from_side,
                                        const enum e_side to_side,
                                        const int Fs_per_side,
                                        t_sblock_pattern& sblock_pattern,
                                        const int switch_override,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        RRNodeId from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create) {
    int num_labels = 0;
    std::vector<int> mux_labels;

    /* x, y coords for get_rr_node lookups */
    int to_x = (CHANX == to_type ? to_seg : to_chan);
    int to_y = (CHANX == to_type ? to_chan : to_seg);
    int sb_x = (CHANX == to_type ? to_sb : to_chan);
    int sb_y = (CHANX == to_type ? to_chan : to_sb);
    int max_len = (CHANX == to_type ? grid.width() : grid.height()) - 2; //-2 for no perimeter channels

    enum Direction to_dir = Direction::DEC;
    if (to_sb < to_seg) {
        to_dir = Direction::INC;
    }

    *Fs_clipped = false;

    /* get list of muxes to which we can connect */
    int dummy;
    label_wire_muxes(to_chan, to_seg, seg_details, UNDEFINED, max_len,
                     to_dir, max_chan_width, false, mux_labels, &num_labels, &dummy);

    /* Can't connect if no muxes. */
    if (num_labels < 1) {
        return 0;
    }

    /* Check if Fs demand was too high. */
    if (Fs_per_side > num_labels) {
        *Fs_clipped = true;
    }

    /* Handle Fs > 3 by assigning consecutive muxes. */
    int count = 0;
    for (int i = 0; i < Fs_per_side; ++i) {
        /* Get the target label */
        for (int j = 0; j < 4; j = j + 2) {
            /* Use the balanced labeling for passing and fringe wires */
            int to_mux = sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j];
            if (to_mux == UN_SET)
                continue;

            int to_track = sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j + 1];
            if (to_track == UN_SET) {
                to_track = mux_labels[(to_mux + i) % num_labels];
                sblock_pattern[sb_x][sb_y][from_side][to_side][from_track][j + 1] = to_track;
            }
            RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, to_x, to_y, to_type, to_track);

            if (!to_node) {
                continue;
            }

            //Determine which switch to use
            int iswitch = seg_details[to_track].arch_wire_switch();

            //Apply any switch overrides
            if (should_apply_switch_override(switch_override)) {
                iswitch = switch_override;
            }
            VTR_ASSERT(iswitch != OPEN);

            /* Add edge to list. */
            rr_edges_to_create.emplace_back(from_rr_node, to_node, iswitch, false);
            ++count;

            auto& device_ctx = g_vpr_ctx.device();
            if (device_ctx.arch_switch_inf[iswitch].directionality() == BI_DIRECTIONAL) {
                //Add reverse edge since bi-directional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, iswitch, false);
                ++count;
            }
        }
    }

    return count;
}

bool is_sblock(const int chan, int wire_seg, const int sb_seg, const int track, const t_chan_seg_details* seg_details, const enum e_directionality directionality) {
    int length, ofs, fac;

    fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    length = seg_details[track].length();

    /* Make sure they gave us correct start */
    wire_seg = get_seg_start(seg_details, track, chan, wire_seg);

    ofs = sb_seg - wire_seg + 1; /* Offset 0 is behind us, so add 1 */

    VTR_ASSERT(ofs >= 0);
    VTR_ASSERT(ofs < (length + 1));

    /* If unidir segment that is going backwards, we need to flip the ofs */
    if ((ofs % fac) > 0) {
        ofs = length - ofs;
    }

    return seg_details[track].sb(ofs);
}

static void get_switch_type(bool is_from_sblock,
                            bool is_to_sblock,
                            short from_node_switch,
                            short to_node_switch,
                            const int switch_override,
                            short switch_types[2]) {
    /* This routine looks at whether the from_node and to_node want a switch,  *
     * and what type of switch is used to connect *to* each type of node       *
     * (from_node_switch and to_node_switch).  It decides what type of switch, *
     * if any, should be used to go from from_node to to_node.  If no switch   *
     * should be inserted (i.e. no connection), it returns OPEN.  Its returned *
     * values are in the switch_types array.  It needs to return an array      *
     * because some topologies (e.g. bi-dir pass gates) result in two switches.*/

    auto& device_ctx = g_vpr_ctx.device();

    switch_types[0] = NO_SWITCH;
    switch_types[1] = NO_SWITCH;

    if (switch_override == NO_SWITCH) {
        return; //No switches
    }

    if (should_apply_switch_override(switch_override)) {
        //Use the override switches instead
        from_node_switch = switch_override;
        to_node_switch = switch_override;
    }

    int used = 0;
    bool forward_switch = false;
    bool backward_switch = false;

    /* Connect forward if we are a sblock */
    if (is_from_sblock) {
        switch_types[used] = to_node_switch;
        ++used;

        forward_switch = true;
    }

    /* Check for reverse switch */
    if (is_to_sblock) {
        if (device_ctx.arch_switch_inf[from_node_switch].directionality() == e_directionality::BI_DIRECTIONAL) {
            switch_types[used] = from_node_switch;
            ++used;

            backward_switch = true;
        }
    }

    /* Take the larger switch if there are two of the same type */
    if (forward_switch
        && backward_switch
        && (device_ctx.arch_switch_inf[from_node_switch].type() == device_ctx.arch_switch_inf[to_node_switch].type())) {
        //Sanity checks
        VTR_ASSERT_SAFE_MSG(device_ctx.arch_switch_inf[from_node_switch].type() == device_ctx.arch_switch_inf[to_node_switch].type(), "Same switch type");
        VTR_ASSERT_MSG(device_ctx.arch_switch_inf[to_node_switch].directionality() == e_directionality::BI_DIRECTIONAL, "Bi-dir to switch");
        VTR_ASSERT_MSG(device_ctx.arch_switch_inf[from_node_switch].directionality() == e_directionality::BI_DIRECTIONAL, "Bi-dir from switch");

        /* Take the smaller index unless the other
         * switch is bigger (smaller R). */

        int first_switch = std::min(to_node_switch, from_node_switch);
        int second_switch = std::max(to_node_switch, from_node_switch);

        if (used < 2) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Expected 2 switches (forward and back) between RR nodes (found %d switches, min switch index: %d max switch index: %d)",
                            used, first_switch, second_switch);
        }

        int switch_to_use = first_switch;
        if (device_ctx.arch_switch_inf[second_switch].R < device_ctx.arch_switch_inf[first_switch].R) {
            switch_to_use = second_switch;
        }

        for (int i = 0; i < used; ++i) {
            switch_types[i] = switch_to_use;
        }
    }
}

static int vpr_to_phy_track(const int itrack,
                            const int chan_num,
                            const int seg_num,
                            const t_chan_seg_details* seg_details,
                            const enum e_directionality directionality) {
    int group_start, group_size;
    int vpr_offset_for_first_phy_track;
    int vpr_offset, phy_offset;
    int phy_track;
    int fac;

    /* Assign in pairs if unidir. */
    fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    group_start = seg_details[itrack].group_start();
    group_size = seg_details[itrack].group_size();

    vpr_offset_for_first_phy_track = (chan_num + seg_num - 1)
                                     % (group_size / fac);
    vpr_offset = (itrack - group_start) / fac;
    phy_offset = (vpr_offset_for_first_phy_track + vpr_offset)
                 % (group_size / fac);
    phy_track = group_start + (fac * phy_offset) + (itrack - group_start) % fac;

    return phy_track;
}

t_sblock_pattern alloc_sblock_pattern_lookup(const DeviceGrid& grid,
                                             t_chan_width* nodes_per_chan) {
    /* loading up the sblock connection pattern matrix. It's a huge matrix because
     * for nonquantized W, it's impossible to make simple permutations to figure out
     * where muxes are and how to connect to them such that their sizes are balanced */

    /* Do chunked allocations to make freeing easier, speed up malloc and free, and
     * reduce some of the memory overhead. Could use fewer malloc's but this way
     * avoids all considerations of pointer sizes and alignment. */

    /* Alloc each list of pointers in one go. items is a running product that increases
     * with each new dimension of the matrix. */

    VTR_ASSERT(grid.width() > 0);
    VTR_ASSERT(grid.height() > 0);
    //CHANGE THIS
    VTR_ASSERT(nodes_per_chan->max >= 0);

    t_sblock_pattern sblock_pattern({{
                                        grid.width() - 1,
                                        grid.height() - 1,
                                        4, //From side
                                        4, //To side
                                        size_t(nodes_per_chan->max),
                                        4 //to_mux, to_trac, alt_mux, alt_track
                                    }},
                                    UN_SET);

    /* This is the outer pointer to the full matrix */
    return sblock_pattern;
}

void load_sblock_pattern_lookup(const int i,
                                const int j,
                                const DeviceGrid& grid,
                                const t_chan_width* nodes_per_chan,
                                const t_chan_details& chan_details_x,
                                const t_chan_details& chan_details_y,
                                const int /*Fs*/,
                                const enum e_switch_block_type switch_block_type,
                                t_sblock_pattern& sblock_pattern) {
    /* This routine loads a lookup table for sblock topology. The lookup table is huge
     * because the sblock varies from location to location. The i, j means the owning
     * location of the sblock under investigation. */

    /* SB's have coords from (0, 0) to (grid.width()-2, grid.height()-2) */
    VTR_ASSERT(i >= 0);
    VTR_ASSERT(i <= int(grid.width()) - 2);
    VTR_ASSERT(j >= 0);
    VTR_ASSERT(j <= int(grid.height()) - 2);

    /* May 12 - 15, 2007
     *
     * I identify three types of sblocks in the chip: 1) The core sblock, whose special
     * property is that the number of muxes (and ending wires) on each side is the same (very useful
     * property, since it leads to a N-to-N assignment problem with ending wires). 2) The corner sblock
     * which is same as a L=1 core sblock with 2 sides only (again N-to-N assignment problem). 3) The
     * fringe / chip edge sblock which is most troublesome, as balance in each side of muxes is
     * attainable but balance in the entire sblock is not. The following code first identifies the
     * incoming wires, which can be classified into incoming passing wires with sblock and incoming
     * ending wires (the word "incoming" is sometimes dropped for ease of discussion). It appropriately
     * labels all the wires on each side by the following order: By the call to label_incoming_wires,
     * which labels for one side, the order is such that the incoming ending wires (always with sblock)
     * are labeled first 0,1,2,... p-1, then the incoming passing wires with sblock are labeled
     * p,p+1,p+2,... k-1 (for total of k). By this convention, one can easily distinguish the ending
     * wires from the passing wires by checking a label against num_ending_wires variable.
     *
     * After labeling all the incoming wires, this routine labels the muxes on the side we're currently
     * connecting to (iterated for four sides of the sblock), called the to_side. The label scheme is
     * the natural order of the muxes by their track #. Also we find the number of muxes.
     *
     * For each to_side, the total incoming wires that connect to the muxes on to_side
     * come from three sides: side_1 (to_side's right), side_2 (to_side's left) and opp_side.
     * The problem of balancing mux size is then: considering all incoming passing wires
     * with sblock on side_1, side_2 and opp_side, how to assign them to the muxes on to_side
     * (with specified Fs) in a way that mux size is imbalanced by at most 1. I solve this
     * problem by this approach: the first incoming passing wire will connect to 0, 1, 2,
     * ..., Fs_per_side - 1, then the next incoming passing wire will connect to
     * Fs_per_side, Fs_per_side+1, ..., Fs_per_side*2-1, and so on. This consistent STAGGERING
     * ensures N-to-N assignment is perfectly balanced and M-to-N assignment is imbalanced by no
     * more than 1.
     */

    /* SB's range from (0, 0) to (grid.width() - 2, grid.height() - 2) */
    /* First find all four sides' incoming wires */

    static_assert(NUM_SIDES == 4, "Should be 4 sides");
    std::array<std::vector<int>, NUM_SIDES> wire_mux_on_track;
    std::array<std::vector<int>, NUM_SIDES> incoming_wire_label;
    int num_incoming_wires[NUM_SIDES];
    int num_ending_wires[NUM_SIDES];
    int num_wire_muxes[NUM_SIDES];

    /* "Label" the wires around the switch block by connectivity. */
    for (e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
        /* Assume the channel segment doesn't exist. */
        wire_mux_on_track[side].clear();
        incoming_wire_label[side].clear();
        num_incoming_wires[side] = 0;
        num_ending_wires[side] = 0;
        num_wire_muxes[side] = 0;

        /* Skip the side and leave the zeroed value if the
         * channel segment doesn't exist. */
        bool skip = true;
        switch (side) {
            case TOP:
                if (j < int(grid.height()) - 2) {
                    skip = false;
                }
                break;
            case RIGHT:
                if (i < int(grid.width()) - 2) {
                    skip = false;
                }
                break;
            case BOTTOM:
                if (j > 0) {
                    skip = false;
                }
                break;
            case LEFT:
                if (i > 0) {
                    skip = false;
                }
                break;
            default:
                VTR_ASSERT_MSG(false, "Unrecognized side");
                break;
        }
        if (skip) {
            continue;
        }

        /* Figure out the channel and segment for a certain direction */
        bool vert = ((side == TOP) || (side == BOTTOM));
        bool pos_dir = ((side == TOP) || (side == RIGHT));
        int chan_len = (vert ? grid.height() : grid.width()) - 2; //-2 for no perim channels
        int chan = (vert ? i : j);
        int sb_seg = (vert ? j : i);
        int seg = (pos_dir ? (sb_seg + 1) : sb_seg);
        int chan_width = get_chan_width(side, nodes_per_chan);
        const t_chan_seg_details* seg_details = (vert ? chan_details_y[chan][seg] : chan_details_x[seg][chan]).data();
        if (seg_details[0].length() <= 0)
            continue;

        /* Figure out all the tracks on a side that are ending and the
         * ones that are passing through and have a SB. */
        enum Direction end_dir = (pos_dir ? Direction::DEC : Direction::INC);

        /*
         * AA: Different channel widths have different seg_details 
         * warranting modified calls to static routines in this file. 
         */
        label_incoming_wires(chan, seg, sb_seg,
                             seg_details, chan_len, end_dir, chan_width,
                             incoming_wire_label[side],
                             &num_incoming_wires[side],
                             &num_ending_wires[side]);

        /* Figure out all the tracks on a side that are starting. */
        int dummy;
        enum Direction start_dir = (pos_dir ? Direction::INC : Direction::DEC);
        label_wire_muxes(chan, seg,
                         seg_details, UNDEFINED, chan_len, start_dir, chan_width,
                         false, wire_mux_on_track[side], &num_wire_muxes[side], &dummy);
    }

    for (e_side to_side : {TOP, RIGHT, BOTTOM, LEFT}) {
        /* Can't do anything if no muxes on this side. */
        if (num_wire_muxes[to_side] == 0)
            continue;

        /* Figure out side rotations */
        VTR_ASSERT((TOP == 0) && (RIGHT == 1) && (BOTTOM == 2) && (LEFT == 3));
        int side_cw = (to_side + 1) % 4;
        int side_opp = (to_side + 2) % 4;
        int side_ccw = (to_side + 3) % 4;

        /* For the core sblock:
         * The new order for passing wires should appear as
         * 0,1,2..,scw-1, for passing wires with sblock on side_cw
         * scw,scw+1,...,sccw-1, for passing wires with sblock on side_ccw
         * sccw,sccw+1,... for passing wires with sblock on side_opp.
         * This way, I can keep the imbalance to at most 1.
         *
         * For the fringe sblocks, I don't distinguish between
         * passing and ending wires so the above statement still holds
         * if you replace "passing" by "incoming" */

        if (!incoming_wire_label[side_cw].empty()) {
            for (int ichan = 0; ichan < get_chan_width((e_side)side_cw, nodes_per_chan); ichan++) {
                int itrack = ichan;
                if (side_cw == TOP || side_cw == BOTTOM) {
                    itrack = ichan % nodes_per_chan->y_list[i];
                } else if (side_cw == RIGHT || side_cw == LEFT) {
                    itrack = ichan % nodes_per_chan->x_list[j];
                }

                if (incoming_wire_label[side_cw][itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((enum e_side)side_cw,
                                                            (enum e_side)to_side,
                                                            incoming_wire_label[side_cw][ichan],
                                                            switch_block_type,
                                                            num_wire_muxes[to_side],
                                                            num_wire_muxes[to_side]);

                    if (sblock_pattern[i][j][side_cw][to_side][itrack][0] == UN_SET) {
                        sblock_pattern[i][j][side_cw][to_side][itrack][0] = mux;
                    } else if (sblock_pattern[i][j][side_cw][to_side][itrack][2] == UN_SET) {
                        sblock_pattern[i][j][side_cw][to_side][itrack][2] = mux;
                    }
                }
            }
        }

        if (!incoming_wire_label[side_ccw].empty()) {
            for (int ichan = 0; ichan < get_chan_width((e_side)side_ccw, nodes_per_chan); ichan++) {
                int itrack = ichan;
                if (side_ccw == TOP || side_ccw == BOTTOM) {
                    itrack = ichan % nodes_per_chan->y_list[i];
                } else if (side_ccw == RIGHT || side_ccw == LEFT) {
                    itrack = ichan % nodes_per_chan->x_list[j];
                }

                if (incoming_wire_label[side_ccw][itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((enum e_side)side_ccw,
                                                            (enum e_side)to_side,
                                                            incoming_wire_label[side_ccw][ichan],
                                                            switch_block_type,
                                                            num_wire_muxes[to_side],
                                                            num_wire_muxes[to_side]);

                    if (sblock_pattern[i][j][side_ccw][to_side][itrack][0] == UN_SET) {
                        sblock_pattern[i][j][side_ccw][to_side][itrack][0] = mux;
                    } else if (sblock_pattern[i][j][side_ccw][to_side][itrack][2] == UN_SET) {
                        sblock_pattern[i][j][side_ccw][to_side][itrack][2] = mux;
                    }
                }
            }
        }

        if (!incoming_wire_label[side_opp].empty()) {
            for (int itrack = 0; itrack < get_chan_width((e_side)side_opp, nodes_per_chan); itrack++) {
                /* not ending wire nor passing wire with sblock */
                if (incoming_wire_label[side_opp][itrack] != UN_SET) {
                    /* corner sblocks for sure have no opposite channel segments so don't care about them */
                    if (incoming_wire_label[side_opp][itrack] < num_ending_wires[side_opp]) {
                        /* The ending wires in core sblocks form N-to-N assignment problem, so can
                         * use any pattern such as Wilton */
                        /* In the direct connect case, I know for sure the init mux is at the same track #
                         * as this ending wire, but still need to find the init mux label for Fs > 3 */
                        int mux = find_label_of_track(wire_mux_on_track[to_side],
                                                      num_wire_muxes[to_side], itrack);
                        sblock_pattern[i][j][side_opp][to_side][itrack][0] = mux;
                    } else {
                        /* These are wire segments that pass through the switch block.
                         *
                         * There is no connection from wire segment midpoints to the opposite switch block
                         * side, so there's nothing to be done here (at Fs=3, this connection is implicit for passing
                         * wires and at Fs>3 the code in this function seems to create heavily unbalanced
                         * switch patterns). Additionally, the code in build_rr_chan() explicitly skips
                         * connections from wire segment midpoints to the opposite sb side (for switch block patterns
                         * generated with this function) so any such assignment to sblock_pattern will be ignored anyway. */
                    }
                }
            }
        }
    }
}

/* Labels the muxes on that side (seg_num, chan_num, direction). The returned array
 * maps a label to the actual track #: array[0] = <the track number of the first/lowest mux>
 * This routine orders wire muxes by their natural order, i.e. track #
 * If seg_type_index == UNDEFINED, all segments in the channel are considered. Otherwise this routine
 * only looks at segments that belong to the specified segment type. */

static void label_wire_muxes(const int chan_num,
                             const int seg_num,
                             const t_chan_seg_details* seg_details,
                             const int seg_type_index,
                             const int max_len,
                             const enum Direction dir,
                             const int max_chan_width,
                             const bool check_cb,
                             std::vector<int>& labels,
                             int* num_wire_muxes,
                             int* num_wire_muxes_cb_restricted) {
    int itrack, start, end, num_labels, num_labels_restricted, pass;
    bool is_endpoint;

    /* COUNT pass then a LOAD pass */
    num_labels = 0;
    num_labels_restricted = 0;
    for (pass = 0; pass < 2; ++pass) {
        /* Alloc the list on LOAD pass */
        if (pass > 0) {
            labels.resize(num_labels);
            std::fill(labels.begin(), labels.end(), 0);
            num_labels = 0;
        }

        /* Find the tracks that are starting. */
        for (itrack = 0; itrack < max_chan_width; ++itrack) {
            start = get_seg_start(seg_details, itrack, chan_num, seg_num);
            end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

            /* Skip tracks that are undefined */
            if (seg_details[itrack].length() == 0) {
                continue;
            }

            /* Skip tracks going the wrong way */
            if (seg_details[itrack].direction() != dir) {
                continue;
            }

            if (seg_type_index != UNDEFINED) {
                /* skip tracks that don't belong to the specified segment type */
                if (seg_details[itrack].index() != seg_type_index) {
                    continue;
                }
            }

            /* Determine if we are a wire startpoint */
            is_endpoint = (seg_num == start);
            if (Direction::DEC == seg_details[itrack].direction()) {
                is_endpoint = (seg_num == end);
            }

            /* Count the labels and load if LOAD pass */
            if (is_endpoint) {
                /*
                 * not all wire endpoints can be driven by OPIN (depending on the <cb> pattern in the arch file)
                 * the check_cb is targeting this arch specification:
                 * if this function is called by get_unidir_opin_connections(),
                 * then we need to check if mux connections can be added to this type of wire,
                 * otherwise, this function should not consider <cb> specification.
                 */
                if ((!check_cb) || (seg_details[itrack].cb(0) == true)) {
                    if (pass > 0) {
                        labels[num_labels] = itrack;
                    }
                    ++num_labels;
                }
                if (pass > 0)
                    num_labels_restricted += (seg_details[itrack].cb(0) == true) ? 1 : 0;
            }
        }
    }

    *num_wire_muxes = num_labels;
    *num_wire_muxes_cb_restricted = num_labels_restricted;
}

static void label_incoming_wires(const int chan_num,
                                 const int seg_num,
                                 const int sb_seg,
                                 const t_chan_seg_details* seg_details,
                                 const int max_len,
                                 const enum Direction dir,
                                 const int max_chan_width,
                                 std::vector<int>& labels,
                                 int* num_incoming_wires,
                                 int* num_ending_wires) {
    /* Labels the incoming wires on that side (seg_num, chan_num, direction).
     * The returned array maps a track # to a label: array[0] = <the new hash value/label for track 0>,
     * the labels 0,1,2,.. identify consecutive incoming wires that have sblock (passing wires with sblock and ending wires) */

    int itrack, start, end, num_passing, num_ending, pass;
    bool sblock_exists, is_endpoint;

    /* Alloc the list of labels for the tracks */
    labels.resize(max_chan_width);
    std::fill(labels.begin(), labels.end(), UN_SET);

    num_ending = 0;
    num_passing = 0;
    for (pass = 0; pass < 2; ++pass) {
        for (itrack = 0; itrack < max_chan_width; ++itrack) {
            /* Skip tracks that are undefined */
            if (seg_details[itrack].length() == 0) {
                continue;
            }

            if (seg_details[itrack].direction() == dir) {
                start = get_seg_start(seg_details, itrack, chan_num, seg_num);
                end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

                /* Determine if we are a wire endpoint */
                is_endpoint = (seg_num == end);
                if (Direction::DEC == seg_details[itrack].direction()) {
                    is_endpoint = (seg_num == start);
                }

                /* Determine if we have a sblock on the wire */
                sblock_exists = is_sblock(chan_num, seg_num, sb_seg, itrack,
                                          seg_details, UNI_DIRECTIONAL);

                switch (pass) {
                        /* On first pass, only load ending wire labels. */
                    case 0:
                        if (is_endpoint) {
                            labels[itrack] = num_ending;
                            ++num_ending;
                        }
                        break;

                        /* On second pass, load the passing wire labels. They
                         * will follow after the ending wire labels. */
                    case 1:
                        if ((false == is_endpoint) && sblock_exists) {
                            labels[itrack] = num_ending + num_passing;
                            ++num_passing;
                        }
                        break;
                    default:
                        VTR_ASSERT_MSG(false, "Unrecognized pass");
                        break;
                }
            }
        }
    }

    *num_incoming_wires = num_passing + num_ending;
    *num_ending_wires = num_ending;
}

static int find_label_of_track(const std::vector<int>& wire_mux_on_track,
                               int num_wire_muxes,
                               int from_track) {
    /* Returns the index/label in array wire_mux_on_track whose entry equals from_track. If none are
     * found, then returns the index of the entry whose value is the largest */
    int i_label = -1;
    int max_track = -1;

    for (int i = 0; i < num_wire_muxes; i++) {
        if (wire_mux_on_track[i] == from_track) {
            i_label = i;
            break;
        } else if (wire_mux_on_track[i] > max_track) {
            i_label = i;
            max_track = wire_mux_on_track[i];
        }
    }
    return i_label;
}

static int should_create_switchblock(const DeviceGrid& grid, int layer_num, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type) {
    //Convert the chan/seg indices to real x/y coordinates
    int y_coord;
    int x_coord;
    if (from_chan_type == CHANX) {
        y_coord = from_chan_coord;
        x_coord = from_seg_coord;
    } else {
        VTR_ASSERT(from_chan_type == CHANY);
        y_coord = from_seg_coord;
        x_coord = from_chan_coord;
    }

    auto blk_type = grid.get_physical_type({x_coord, y_coord, layer_num});
    int width_offset = grid.get_width_offset({x_coord, y_coord, layer_num});
    int height_offset = grid.get_height_offset({x_coord, y_coord, layer_num});

    e_sb_type sb_type = blk_type->switchblock_locations[width_offset][height_offset];
    auto switch_override = blk_type->switchblock_switch_overrides[width_offset][height_offset];

    if (sb_type == e_sb_type::FULL) {
        return switch_override;
    } else if (sb_type == e_sb_type::STRAIGHT && from_chan_type == to_chan_type) {
        return switch_override;
    } else if (sb_type == e_sb_type::TURNS && from_chan_type != to_chan_type) {
        return switch_override;
    } else if (sb_type == e_sb_type::HORIZONTAL && from_chan_type == CHANX && to_chan_type == CHANX) {
        return switch_override;
    } else if (sb_type == e_sb_type::VERTICAL && from_chan_type == CHANY && to_chan_type == CHANY) {
        return switch_override;
    }

    return NO_SWITCH;
}

static bool should_apply_switch_override(int switch_override) {
    if (switch_override != NO_SWITCH && switch_override != DEFAULT_SWITCH) {
        VTR_ASSERT(switch_override >= 0);
        return true;
    }
    return false;
}

inline int get_chan_width(enum e_side side, const t_chan_width* nodes_per_chan) {
    return (side == TOP || side == BOTTOM ? nodes_per_chan->y_max : nodes_per_chan->x_max);
}