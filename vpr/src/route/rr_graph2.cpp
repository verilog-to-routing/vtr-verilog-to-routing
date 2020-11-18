#include <cstdio>

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"
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
                                 const int chan_len,
                                 const int num_chans,
                                 const t_rr_type type,
                                 const t_chan_details& chan_details,
                                 t_rr_node_indices& indices,
                                 int* index);

static void load_block_rr_indices(const DeviceGrid& grid,
                                  t_rr_node_indices& indices,
                                  int* index);

static int get_bidir_track_to_chan_seg(const std::vector<int> conn_tracks,
                                       const t_rr_node_indices& L_rr_node_indices,
                                       const int to_chan,
                                       const int to_seg,
                                       const int to_sb,
                                       const t_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       const bool from_is_sblock,
                                       const int from_switch,
                                       const int switch_override,
                                       const enum e_directionality directionality,
                                       const int from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create);

static int get_unidir_track_to_chan_seg(const int from_track,
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
                                        const t_rr_node_indices& L_rr_node_indices,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        const int from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        t_opin_connections_scratchpad* scratchpad);

static int get_track_to_chan_seg(const int from_track,
                                 const int to_chan,
                                 const int to_seg,
                                 const t_rr_type to_chan_type,
                                 const e_side from_side,
                                 const e_side to_side,
                                 const int swtich_override,
                                 const t_rr_node_indices& L_rr_node_indices,
                                 t_sb_connection_map* sb_conn_map,
                                 const int from_rr_node,
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
                             const enum e_direction dir,
                             const int max_chan_width,
                             const bool check_cb,
                             std::vector<int>* labels,
                             int* num_wire_muxes,
                             int* num_wire_muxes_cb_restricted);

static void label_incoming_wires(const int chan_num,
                                 const int seg_num,
                                 const int sb_seg,
                                 const t_chan_seg_details* seg_details,
                                 const int max_len,
                                 const enum e_direction dir,
                                 const int max_chan_width,
                                 std::vector<int>* labels,
                                 int* num_incoming_wires,
                                 int* num_ending_wires);

static int find_label_of_track(int* wire_mux_on_track,
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
static int should_create_switchblock(const DeviceGrid& grid, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type);

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

t_seg_details* alloc_and_load_seg_details(int* max_chan_width,
                                          const int max_len,
                                          const std::vector<t_segment_inf>& segment_inf,
                                          const bool use_full_seg_groups,
                                          const bool is_global_graph,
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
                if (is_global_graph || seg_details[cur_track].longline) {
                    seg_details[cur_track].cb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    index = j % segment_inf[i].cb.size();
                    seg_details[cur_track].cb[j] = segment_inf[i].cb[index];
                }
            }
            for (j = 0; j < (length + 1); ++j) {
                if (is_global_graph || seg_details[cur_track].longline) {
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

            if (BI_DIRECTIONAL == directionality) {
                seg_details[cur_track].direction = BI_DIRECTION;
            } else {
                VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                seg_details[cur_track].direction = (itrack % 2) ? DEC_DIRECTION : INC_DIRECTION;
            }

            seg_details[cur_track].index = i;

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
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y) {
    chan_details_x = init_chan_details(grid, nodes_per_chan,
                                       num_seg_details, seg_details, SEG_DETAILS_X);
    chan_details_y = init_chan_details(grid, nodes_per_chan,
                                       num_seg_details, seg_details, SEG_DETAILS_Y);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(grid, nodes_per_chan,
                        chan_details_x, chan_details_y);
}

t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 const enum e_seg_details_type seg_details_type) {
    VTR_ASSERT(num_seg_details <= nodes_per_chan->max);

    t_chan_details chan_details({grid.width(), grid.height(), size_t(num_seg_details)});

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            t_chan_seg_details* p_seg_details = chan_details[x][y].data();
            for (int i = 0; i < num_seg_details; ++i) {
                p_seg_details[i] = t_chan_seg_details(&seg_details[i]);

                int seg_start = -1;
                int seg_end = -1;

                if (seg_details_type == SEG_DETAILS_X) {
                    seg_start = get_seg_start(p_seg_details, i, y, x);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, y, grid.width() - 2); //-2 for no perim channels
                }
                if (seg_details_type == SEG_DETAILS_Y) {
                    seg_start = get_seg_start(p_seg_details, i, x, y);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, x, grid.height() - 2); //-2 for no perim channels
                }

                p_seg_details[i].set_seg_start(seg_start);
                p_seg_details[i].set_seg_end(seg_end);

                if (seg_details_type == SEG_DETAILS_X) {
                    if (i >= nodes_per_chan->x_list[y]) {
                        p_seg_details[i].set_length(0);
                    }
                }
                if (seg_details_type == SEG_DETAILS_Y) {
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
                               chan_details_x, SEG_DETAILS_X);
        }
    }

    for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_y[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                               chan_details_y, SEG_DETAILS_Y);
        }
    }
}

void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width* nodes_per_chan,
                        t_chan_details& chan_details,
                        const enum e_seg_details_type seg_details_type) {
    int seg_index = (seg_details_type == SEG_DETAILS_X ? x : y);

    for (int track = 0; track < nodes_per_chan->max; ++track) {
        int lx = (seg_details_type == SEG_DETAILS_X ? x - 1 : x);
        int ly = (seg_details_type == SEG_DETAILS_X ? y : y - 1);
        if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
            continue;

        while (chan_details[lx][ly][track].seg_end() >= seg_index) {
            chan_details[lx][ly][track].set_seg_end(seg_index - 1);
            lx = (seg_details_type == SEG_DETAILS_X ? lx - 1 : lx);
            ly = (seg_details_type == SEG_DETAILS_X ? ly : ly - 1);
            if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
                break;
        }
    }

    for (int track = 0; track < nodes_per_chan->max; ++track) {
        size_t lx = (seg_details_type == SEG_DETAILS_X ? x + 1 : x);
        size_t ly = (seg_details_type == SEG_DETAILS_X ? y : y + 1);
        if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length() == 0) //-2 for no perim channels
            continue;

        while (chan_details[lx][ly][track].seg_start() <= seg_index) {
            chan_details[lx][ly][track].set_seg_start(seg_index + 1);
            lx = (seg_details_type == SEG_DETAILS_X ? lx + 1 : lx);
            ly = (seg_details_type == SEG_DETAILS_X ? ly : ly + 1);
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
int get_bidir_opin_connections(const int i,
                               const int j,
                               const int ipin,
                               const int from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_pin_to_track_lookup& opin_to_track_map,
                               const t_rr_node_indices& L_rr_node_indices,
                               const t_chan_details& chan_details_x,
                               const t_chan_details& chan_details_y) {
    int num_conn, tr_i, tr_j, chan, seg;
    int to_switch, to_node;
    int is_connected_track;
    t_physical_tile_type_ptr type;
    t_rr_type to_type;

    auto& device_ctx = g_vpr_ctx.device();

    type = device_ctx.grid[i][j].type;
    int width_offset = device_ctx.grid[i][j].width_offset;
    int height_offset = device_ctx.grid[i][j].height_offset;

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
        for (int to_track : opin_to_track_map[type->index][ipin][width_offset][height_offset][side]) {
            /* Skip unconnected connections */
            if (OPEN == to_track || is_connected_track) {
                is_connected_track = true;
                VTR_ASSERT(OPEN == opin_to_track_map[type->index][ipin][width_offset][height_offset][side][0]);
                continue;
            }

            /* Only connect to wire if there is a CB */
            if (is_cblock(chan, seg, to_track, seg_details)) {
                to_switch = seg_details[to_track].arch_wire_switch();
                to_node = get_rr_node_index(L_rr_node_indices, tr_i, tr_j, to_type, to_track);

                if (to_node == OPEN) {
                    continue;
                }

                rr_edges_to_create.emplace_back(from_rr_node, to_node, to_switch);
                ++num_conn;
            }
        }
    }

    return num_conn;
}

int get_unidir_opin_connections(const int chan,
                                const int seg,
                                int Fc,
                                const int seg_type_index,
                                const t_rr_type chan_type,
                                const t_chan_seg_details* seg_details,
                                const int from_rr_node,
                                t_rr_edge_info_set& rr_edges_to_create,
                                vtr::NdMatrix<int, 3>& Fc_ofs,
                                const int max_len,
                                const int max_chan_width,
                                const t_rr_node_indices& L_rr_node_indices,
                                bool* Fc_clipped,
                                t_opin_connections_scratchpad* scratchpad) {
    /* Gets a linked list of Fc nodes of specified seg_type_index to connect
     * to in given chan seg. Fc_ofs is used for the opin staggering pattern. */

    int num_inc_muxes, num_dec_muxes, iconn;
    int inc_inode_index, dec_inode_index;
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
    std::vector<int>& inc_muxes = scratchpad->scratch[0];
    std::vector<int>& dec_muxes = scratchpad->scratch[1];

    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     INC_DIRECTION, max_chan_width, true, &inc_muxes, &num_inc_muxes, &dummy);
    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     DEC_DIRECTION, max_chan_width, true, &dec_muxes, &num_dec_muxes, &dummy);

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
        inc_inode_index = get_rr_node_index(L_rr_node_indices, x, y, chan_type, inc_track);
        dec_inode_index = get_rr_node_index(L_rr_node_indices, x, y, chan_type, dec_track);

        if (inc_inode_index == OPEN || dec_inode_index == OPEN) {
            continue;
        }

        /* Add to the list. */
        rr_edges_to_create.emplace_back(from_rr_node, inc_inode_index, seg_details[inc_track].arch_opin_switch());
        ++num_edges;

        rr_edges_to_create.emplace_back(from_rr_node, dec_inode_index, seg_details[dec_track].arch_opin_switch());
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
    if (DEC_DIRECTION == seg_details[track].direction()) {
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
                DIRECTION_STRING[seg_details[i].direction()]);

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
                       int max_chan_width,
                       const DeviceGrid& grid,
                       const char* fname) {
    FILE* fp = vtr::fopen(fname, "w");
    if (fp) {
        for (size_t y = 0; y <= grid.height() - 2; ++y) {    //-2 for no perim channels
            for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_x: [%zu][%zu]\n", x, y);
                fprintf(fp, "========================\n");

                const t_chan_seg_details* seg_details = chan_details_x[x][y].data();
                dump_seg_details(seg_details, max_chan_width, fp);
            }
        }
        for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
            for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_y: [%zu][%zu]\n", x, y);
                fprintf(fp, "========================\n");

                const t_chan_seg_details* seg_details = chan_details_y[x][y].data();
                dump_seg_details(seg_details, max_chan_width, fp);
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

static void load_chan_rr_indices(const int max_chan_width,
                                 const int chan_len,
                                 const int num_chans,
                                 const t_rr_type type,
                                 const t_chan_details& chan_details,
                                 t_rr_node_indices& indices,
                                 int* index) {
    VTR_ASSERT(indices[type].dim_size(0) == size_t(num_chans));
    VTR_ASSERT(indices[type].dim_size(1) == size_t(chan_len));
    VTR_ASSERT(indices[type].dim_size(2) == NUM_SIDES);
    for (int chan = 0; chan < num_chans - 1; ++chan) {
        for (int seg = 1; seg < chan_len - 1; ++seg) {
            /* Alloc the track inode lookup list */
            //Since channels have no side, we just use the first side
            indices[type][chan][seg][0].resize(max_chan_width, OPEN);
        }
    }

    for (int chan = 0; chan < num_chans - 1; ++chan) {
        for (int seg = 1; seg < chan_len - 1; ++seg) {
            /* Assign an inode to the starts of tracks */
            int x = (type == CHANX ? seg : chan);
            int y = (type == CHANX ? chan : seg);
            const t_chan_seg_details* seg_details = chan_details[x][y].data();

            for (unsigned track = 0; track < indices[type][chan][seg][0].size(); ++track) {
                if (seg_details[track].length() <= 0)
                    continue;

                int start = get_seg_start(seg_details, track, chan, seg);

                /* If the start of the wire doesn't have a inode,
                 * assign one to it. */
                int inode = indices[type][chan][start][0][track];
                if (OPEN == inode) {
                    inode = *index;
                    ++(*index);

                    indices[type][chan][start][0][track] = inode;
                }

                /* Assign inode of start of wire to current position */
                indices[type][chan][seg][0][track] = inode;
            }
        }
    }
}

static void load_block_rr_indices(const DeviceGrid& grid,
                                  t_rr_node_indices& indices,
                                  int* index) {
    //Walk through the grid assigning indices to SOURCE/SINK IPIN/OPIN
    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            if (grid[x][y].width_offset == 0 && grid[x][y].height_offset == 0) {
                //Process each block from it's root location
                auto type = grid[x][y].type;

                //Assign indices for SINKs and SOURCEs
                // Note that SINKS/SOURCES have no side, so we always use side 0
                for (const auto& class_inf : type->class_inf) {
                    auto class_type = class_inf.type;
                    if (class_type == DRIVER) {
                        indices[SOURCE][x][y][0].push_back(*index);
                        indices[SINK][x][y][0].push_back(OPEN);
                    } else {
                        VTR_ASSERT(class_type == RECEIVER);
                        indices[SINK][x][y][0].push_back(*index);
                        indices[SOURCE][x][y][0].push_back(OPEN);
                    }
                    ++(*index);
                }
                VTR_ASSERT(indices[SOURCE][x][y][0].size() == type->class_inf.size());
                VTR_ASSERT(indices[SINK][x][y][0].size() == type->class_inf.size());

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
                if (grid.height() - 1 == y) { /* TOP side */
                    wanted_sides.push_back(BOTTOM);
                }
                if (grid.width() - 1 == x) { /* RIGHT side */
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
                if (true == wanted_sides.empty()) {
                    for (e_side side : {TOP, BOTTOM, LEFT, RIGHT}) {
                        wanted_sides.push_back(side);
                    }
                }

                //Assign indices for IPINs and OPINs at all offsets from root
                for (int ipin = 0; ipin < type->num_pins; ++ipin) {
                    bool assigned_to_rr_node = false;
                    for (e_side side : wanted_sides) {
                        for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                            int x_tile = x + width_offset;
                            for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                                int y_tile = y + height_offset;
                                if (type->pinloc[width_offset][height_offset][side][ipin]) {
                                    int iclass = type->pin_class[ipin];
                                    auto class_type = type->class_inf[iclass].type;

                                    if (class_type == DRIVER) {
                                        indices[OPIN][x_tile][y_tile][side].push_back(*index);
                                        indices[IPIN][x_tile][y_tile][side].push_back(OPEN);
                                        assigned_to_rr_node = true;
                                    } else {
                                        VTR_ASSERT(class_type == RECEIVER);
                                        indices[OPIN][x_tile][y_tile][side].push_back(OPEN);
                                        indices[IPIN][x_tile][y_tile][side].push_back(*index);
                                        assigned_to_rr_node = true;
                                    }
                                } else {
                                    indices[IPIN][x_tile][y_tile][side].push_back(OPEN);
                                    indices[OPIN][x_tile][y_tile][side].push_back(OPEN);
                                }
                            }
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

                //Sanity check
                for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                    int x_tile = x + width_offset;
                    for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                        int y_tile = y + height_offset;
                        for (e_side side : SIDES) {
                            //Note that the fast look-up stores all the indices for the pins on each side
                            //It has a fixed size (either 0 or the number of pins)
                            //Case 0 pins: the side is skipped as no pins are located on it
                            //Case number of pins: there are pins on this side
                            //and data query can be applied any pin id on this side
                            VTR_ASSERT((indices[IPIN][x_tile][y_tile][side].size() == size_t(type->num_pins))
                                       || (0 == indices[IPIN][x_tile][y_tile][side].size()));
                            VTR_ASSERT((indices[OPIN][x_tile][y_tile][side].size() == size_t(type->num_pins))
                                       || (0 == indices[OPIN][x_tile][y_tile][side].size()));
                        }
                    }
                }
            }
        }
    }

    //Copy the SOURCE/SINK nodes to all offset positions for blocks with width > 1 and/or height > 1
    // This ensures that look-ups on non-root locations will still find the correct SOURCE/SINK
    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            int width_offset = grid[x][y].width_offset;
            int height_offset = grid[x][y].height_offset;
            if (width_offset != 0 || height_offset != 0) {
                int root_x = x - width_offset;
                int root_y = y - height_offset;

                indices[SOURCE][x][y][0] = indices[SOURCE][root_x][root_y][0];
                indices[SINK][x][y][0] = indices[SINK][root_x][root_y][0];
            }
        }
    }
}

t_rr_node_indices alloc_and_load_rr_node_indices(const int max_chan_width,
                                                 const DeviceGrid& grid,
                                                 int* index,
                                                 const t_chan_details& chan_details_x,
                                                 const t_chan_details& chan_details_y) {
    /* Allocates and loads all the structures needed for fast lookups of the   *
     * index of an rr_node.  rr_node_indices is a matrix containing the index  *
     * of the *first* rr_node at a given (i,j) location.                       */

    t_rr_node_indices indices;

    /* Alloc the lookup table */
    for (t_rr_type rr_type : RR_TYPES) {
        if (rr_type == CHANX) {
            indices[rr_type].resize({grid.height(), grid.width(), NUM_SIDES});
        } else {
            indices[rr_type].resize({grid.width(), grid.height(), NUM_SIDES});
        }
    }

    /* Assign indices for block nodes */
    load_block_rr_indices(grid, indices, index);

    /* Load the data for x and y channels */
    load_chan_rr_indices(max_chan_width, grid.width(), grid.height(),
                         CHANX, chan_details_x, indices, index);
    load_chan_rr_indices(max_chan_width, grid.height(), grid.width(),
                         CHANY, chan_details_y, indices, index);

    return indices;
}

bool verify_rr_node_indices(const DeviceGrid& grid, const t_rr_node_indices& rr_node_indices, const t_rr_graph_storage& rr_nodes) {
    std::unordered_map<int, int> rr_node_counts;

    for (t_rr_type rr_type : RR_TYPES) {
        int width = grid.width();
        int height = grid.height();

        //CHANX bizarely stores with x/y swapped...
        if (rr_type == CHANX) {
            std::swap(width, height);
        }

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                for (e_side side : SIDES) {
                    for (int inode : rr_node_indices[rr_type][x][y][side]) {
                        if (inode < 0) continue;

                        rr_node_counts[inode]++;

                        auto& rr_node = rr_nodes[inode];

                        if (rr_node.type() != rr_type) {
                            VPR_ERROR(VPR_ERROR_ROUTE, "RR node type does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                                      rr_node_typename[rr_node.type()],
                                      rr_node_typename[rr_type],
                                      describe_rr_node(inode).c_str());
                        }

                        if (rr_node.type() == CHANX) {
                            //CHANX has this bizare swapped x / y storage...
                            std::swap(x, y);

                            VTR_ASSERT_MSG(rr_node.ylow() == rr_node.yhigh(), "CHANX should be horizontal");

                            if (y != rr_node.ylow()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_node.ylow(),
                                          y,
                                          describe_rr_node(inode).c_str());
                            }

                            if (x < rr_node.xlow() || x > rr_node.xhigh()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_node.xlow(),
                                          rr_node.xlow(),
                                          x,
                                          describe_rr_node(inode).c_str());
                            }

                            std::swap(x, y); //Swap back
                        } else if (rr_node.type() == CHANY) {
                            VTR_ASSERT_MSG(rr_node.xlow() == rr_node.xhigh(), "CHANY should be veritcal");

                            if (x != rr_node.xlow()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_node.xlow(),
                                          x,
                                          describe_rr_node(inode).c_str());
                            }

                            if (y < rr_node.ylow() || y > rr_node.yhigh()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_node.ylow(),
                                          rr_node.ylow(),
                                          y,
                                          describe_rr_node(inode).c_str());
                            }
                        } else if (rr_node.type() == SOURCE || rr_node.type() == SINK) {
                            //Sources have co-ordintes covering the entire block they are in
                            if (x < rr_node.xlow() || x > rr_node.xhigh()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_node.xlow(),
                                          rr_node.xlow(),
                                          x,
                                          describe_rr_node(inode).c_str());
                            }

                            if (y < rr_node.ylow() || y > rr_node.yhigh()) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_node.ylow(),
                                          rr_node.ylow(),
                                          y,
                                          describe_rr_node(inode).c_str());
                            }

                        } else {
                            VTR_ASSERT(rr_node.type() == IPIN || rr_node.type() == OPIN);
                            /* As we allow a pin to be indexable on multiple sides,
                             * This check code should be invalid
                             * if (rr_node.xlow() != x) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node xlow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.xlow(),
                             *               x,
                             *               describe_rr_node(inode).c_str());
                             * }
                             *
                             * if (rr_node.ylow() != y) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node ylow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.ylow(),
                             *               y,
                             *               describe_rr_node(inode).c_str());
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
                             *               describe_rr_node(inode).c_str());
                             * } else {
                             *     VTR_ASSERT(rr_node.side() == side);
                             * }
                             */
                        } else { //Non-pin's don't have sides, and should only be in side 0
                            if (side != SIDES[0]) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "Non-Pin RR node in rr_node_indices found with non-default side %s: %s",
                                          SIDE_STRING[side],
                                          describe_rr_node(inode).c_str());
                            } else {
                                VTR_ASSERT(side == 0);
                            }
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
        int inode = kv.first;
        int count = kv.second;

        auto& rr_node = rr_nodes[inode];

        if (rr_node.type() == SOURCE || rr_node.type() == SINK) {
            int rr_width = (rr_node.xhigh() - rr_node.xlow() + 1);
            int rr_height = (rr_node.yhigh() - rr_node.ylow() + 1);
            int rr_area = rr_width * rr_height;
            if (count != rr_area) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node size (%d) and count within rr_node_indices (%d): %s",
                          rr_area,
                          rr_node.length(),
                          count,
                          describe_rr_node(inode).c_str());
            }
            /* As we allow a pin to be indexable on multiple sides,
             * This check code should not be applied to input and output pins
             */
        } else if ((OPIN != rr_node.type()) && (IPIN != rr_node.type())) {
            if (count != rr_node.length() + 1) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node length (%d) and count within rr_node_indices (%d, should be length + 1): %s",
                          rr_node.length(),
                          count,
                          describe_rr_node(inode).c_str());
            }
        }
    }

    return true;
}

std::vector<int> get_rr_node_chan_wires_at_location(const t_rr_node_indices& L_rr_node_indices,
                                                    t_rr_type rr_type,
                                                    int x,
                                                    int y) {
    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    /* Currently need to swap x and y for CHANX because of chan, seg convention */
    if (CHANX == rr_type) {
        std::swap(x, y);
    }

    return L_rr_node_indices[rr_type][x][y][SIDES[0]];
}

void get_rr_node_indices(const t_rr_node_indices& L_rr_node_indices,
                         int x,
                         int y,
                         t_rr_type rr_type,
                         int ptc,
                         std::vector<int>* indices) {
    indices->resize(0);

    /*
     * Like get_rr_node_index() but returns all matching nodes,
     * rather than just the first. This is particularly useful for getting all instances
     * of a specific IPIN/OPIN at a specific gird tile (x,y) location.
     */

    if (rr_type == IPIN || rr_type == OPIN) {
        //For pins we need to look at all the sides of the current grid tile

        for (e_side side : SIDES) {
            int rr_node_index = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc, side);

            if (rr_node_index >= 0) {
                indices->push_back(rr_node_index);
            }
        }
    } else {
        //Sides do not effect non-pins so there should only be one per ptc
        int rr_node_index = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc);

        if (rr_node_index != OPEN) {
            indices->push_back(rr_node_index);
        }
    }
}

void get_rr_node_indices(const t_rr_node_indices& L_rr_node_indices,
                         int x,
                         int y,
                         t_rr_type rr_type,
                         std::vector<int>* indices,
                         e_side side) {
    //Comparison predicate
    auto is_valid_id = [](int id) { return id != OPEN; };

    indices->resize(0);

    if (rr_type == SOURCE
        || rr_type == SINK
        || rr_type == CHANX
        || rr_type == CHANY) {
        //CHANX uses an odd swapped x/y convention...
        if (CHANX == rr_type) {
            std::swap(x, y);
        }

        VTR_ASSERT_MSG(side == NUM_SIDES, "Non-IPINs/OPINs must not specify side");
        indices->reserve(L_rr_node_indices[rr_type][x][y][0].size());
        std::copy_if(
            L_rr_node_indices[rr_type][x][y][0].begin(),
            L_rr_node_indices[rr_type][x][y][0].end(),
            std::back_inserter(*indices),
            is_valid_id);
    } else {
        VTR_ASSERT(rr_type == OPIN || rr_type == IPIN);
        if (side == NUM_SIDES) {
            //All sides
            size_t capacity_needed = 0;
            for (e_side tmp_side : SIDES) {
                capacity_needed += L_rr_node_indices[rr_type][x][y][tmp_side].size();
            }

            indices->reserve(capacity_needed);
            for (e_side tmp_side : SIDES) {
                std::copy_if(
                    L_rr_node_indices[rr_type][x][y][tmp_side].begin(),
                    L_rr_node_indices[rr_type][x][y][tmp_side].end(),
                    std::back_inserter(*indices),
                    is_valid_id);
            }
        } else {
            //Side specified
            indices->reserve(L_rr_node_indices[rr_type][x][y][side].size());
            std::copy_if(
                L_rr_node_indices[rr_type][x][y][side].begin(),
                L_rr_node_indices[rr_type][x][y][side].end(),
                std::back_inserter(*indices),
                is_valid_id);
        }
    }
}

int get_rr_node_index(const t_rr_node_indices& L_rr_node_indices,
                      int x,
                      int y,
                      t_rr_type rr_type,
                      int ptc,
                      e_side side) {
    /*
     * Returns the index of the specified routing resource node.  (x,y) are
     * the location within the FPGA, rr_type specifies the type of resource,
     * and ptc gives the number of this resource.  ptc is the class number,
     * pin number or track number, depending on what type of resource this
     * is.  All ptcs start at 0 and go up to pins_per_clb-1 or the equivalent.
     * There are class_inf size SOURCEs + SINKs, type->num_pins IPINs + OPINs,
     * and max_chan_width CHANX and CHANY (each).
     *
     * Note that for segments (CHANX and CHANY) of length > 1, the segment is
     * given an rr_index based on the (x,y) location at which it starts (i.e.
     * lowest (x,y) location at which this segment exists).
     * This routine also performs error checking to make sure the node in
     * question exists.
     *
     * The 'side' argument only applies to IPIN/OPIN types, and specifies which
     * side of the grid tile the node should be located on. The value is ignored
     * for non-IPIN/OPIN types
     */
    if (rr_type == IPIN || rr_type == OPIN) {
        VTR_ASSERT_MSG(side != NUM_SIDES, "IPIN/OPIN must specify desired side (can not be default NUM_SIDES)");
    } else {
        VTR_ASSERT(rr_type != IPIN && rr_type != OPIN);
        side = SIDES[0];
    }

    int iclass;

    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(ptc >= 0);
    VTR_ASSERT(x >= 0 && x < int(device_ctx.grid.width()));
    VTR_ASSERT(y >= 0 && y < int(device_ctx.grid.height()));

    auto type = device_ctx.grid[x][y].type;

    /* Currently need to swap x and y for CHANX because of chan, seg convention */
    if (CHANX == rr_type) {
        std::swap(x, y);
    }

    /* Start of that block.  */
    const std::vector<int>& lookup = L_rr_node_indices[rr_type][x][y][side];

    /* Check valid ptc num */
    VTR_ASSERT(ptc >= 0);

    switch (rr_type) {
        case SOURCE:
            VTR_ASSERT(ptc < (int)type->class_inf.size());
            VTR_ASSERT(type->class_inf[ptc].type == DRIVER);
            break;

        case SINK:
            VTR_ASSERT(ptc < (int)type->class_inf.size());
            VTR_ASSERT(type->class_inf[ptc].type == RECEIVER);
            break;

        case OPIN:
            VTR_ASSERT(ptc < type->num_pins);
            iclass = type->pin_class[ptc];
            VTR_ASSERT(type->class_inf[iclass].type == DRIVER);
            break;

        case IPIN:
            VTR_ASSERT(ptc < type->num_pins);
            iclass = type->pin_class[ptc];
            VTR_ASSERT(type->class_inf[iclass].type == RECEIVER);
            break;

        case CHANX:
        case CHANY:
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Bad rr_node passed to get_rr_node_index.\n"
                            "Request for type=%d ptc=%d at (%d, %d).\n",
                            rr_type, ptc, x, y);
    }

    return ((unsigned)ptc < lookup.size() ? lookup[ptc] : -1);
}

int find_average_rr_node_index(int device_width,
                               int device_height,
                               t_rr_type rr_type,
                               int ptc,
                               const t_rr_node_indices& L_rr_node_indices) {
    /* Find and return the index to a rr_node that is located at the "center" *
     * of the current grid array, if possible.  In the event the "center" of  *
     * the grid array is an EMPTY or IO node, then retry alternate locations.  *
     * Worst case, this function will simply return the 1st non-EMPTY and     *
     * non-IO node.                                                           */

    int inode = get_rr_node_index(L_rr_node_indices, (device_width) / 2, (device_height) / 2,
                                  rr_type, ptc);

    if (inode == OPEN) {
        inode = get_rr_node_index(L_rr_node_indices, (device_width) / 4, (device_height) / 4,
                                  rr_type, ptc);
    }
    if (inode == OPEN) {
        inode = get_rr_node_index(L_rr_node_indices, (device_width) / 4 * 3, (device_height) / 4 * 3,
                                  rr_type, ptc);
    }
    if (inode == OPEN) {
        auto& device_ctx = g_vpr_ctx.device();

        for (int x = 0; x < device_width; ++x) {
            for (int y = 0; y < device_height; ++y) {
                if (device_ctx.grid[x][y].type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE)
                    continue;
                if (is_io_type(device_ctx.grid[x][y].type))
                    continue;

                inode = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc);
                if (inode != OPEN)
                    break;
            }
            if (inode != OPEN)
                break;
        }
    }
    return (inode);
}

int get_track_to_pins(int seg,
                      int chan,
                      int track,
                      int tracks_per_chan,
                      int from_rr_node,
                      t_rr_edge_info_set& rr_edges_to_create,
                      const t_rr_node_indices& L_rr_node_indices,
                      const t_track_to_pin_lookup& track_to_pin_lookup,
                      const t_chan_seg_details* seg_details,
                      enum e_rr_type chan_type,
                      int chan_length,
                      int wire_to_ipin_switch,
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

                /* PAJ - if the pointed to is an EMPTY then shouldn't look for ipins */
                if (device_ctx.grid[x][y].type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE)
                    continue;

                /* Move from logical (straight) to physical (twisted) track index
                 * - algorithm assigns ipin connections to same physical track index
                 * so that the logical track gets distributed uniformly */

                phy_track = vpr_to_phy_track(track, chan, j, seg_details, directionality);
                phy_track %= tracks_per_chan;

                /* We need the type to find the ipin map for this type */
                auto type = device_ctx.grid[x][y].type;
                int width_offset = device_ctx.grid[x][y].width_offset;
                int height_offset = device_ctx.grid[x][y].height_offset;

                max_conn = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side].size();
                for (iconn = 0; iconn < max_conn; iconn++) {
                    ipin = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side][iconn];

                    /* Check there is a connection and Fc map isn't wrong */
                    /*int to_node = get_rr_node_index(L_rr_node_indices, x + width_offset, y + height_offset, IPIN, ipin, side);*/
                    int to_node = get_rr_node_index(L_rr_node_indices, x, y, IPIN, ipin, side);
                    if (to_node >= 0) {
                        rr_edges_to_create.emplace_back(from_rr_node, to_node, wire_to_ipin_switch);
                        ++num_conn;
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
int get_track_to_tracks(const int from_chan,
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
                        const int from_rr_node,
                        t_rr_edge_info_set& rr_edges_to_create,
                        const t_chan_seg_details* from_seg_details,
                        const t_chan_seg_details* to_seg_details,
                        const t_chan_details& to_chan_details,
                        const enum e_directionality directionality,
                        const t_rr_node_indices& L_rr_node_indices,
                        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                        t_sb_connection_map* sb_conn_map,
                        t_opin_connections_scratchpad* scratchpad) {
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

        auto switch_override = should_create_switchblock(grid, from_chan, sb_seg, from_type, to_type);
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

        /* to_chan_details may correspond to an x-directed or y-directed channel, depending for which
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
                if (DEC_DIRECTION == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(from_track, to_chan, to_seg,
                                                      to_type, from_side_a, to_side,
                                                      switch_override,
                                                      L_rr_node_indices,
                                                      sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                     * switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_a][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(conn_tracks,
                                                            L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the top or right of SB so it
                     * makes the most sense to only get there from DEC_DIRECTION wires. */
                    if ((from_is_sblock) && (DEC_DIRECTION == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width, grid,
                                                                 from_side_a, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 L_rr_node_indices, to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create, scratchpad);
                    }
                }
            }
        }

        /* Do the edges going from the left SB side (if we're in CHANX) or bottom (if we're in CHANY)
         * However, can't connect to left (bottom) if already at leftmost (bottommost) track end */
        if (sb_seg > start_sb_seg) {
            if (custom_switch_block) {
                if (INC_DIRECTION == from_seg_details[from_track].direction() || BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(from_track, to_chan, to_seg,
                                                      to_type, from_side_b, to_side,
                                                      switch_override,
                                                      L_rr_node_indices,
                                                      sb_conn_map, from_rr_node, rr_edges_to_create);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                     * switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_b][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(conn_tracks,
                                                            L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
                                                            to_seg_details, from_is_sblock, from_switch,
                                                            switch_override,
                                                            directionality, from_rr_node, rr_edges_to_create);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the bottom or left of SB so it
                     * makes the most sense to only get there from INC_DIRECTION wires. */
                    if ((from_is_sblock)
                        && (INC_DIRECTION == from_seg_details[from_track].direction())) {
                        num_conn += get_unidir_track_to_chan_seg(from_track, to_chan,
                                                                 to_seg, to_sb, to_type, max_chan_width, grid,
                                                                 from_side_b, to_side, Fs_per_side,
                                                                 sblock_pattern,
                                                                 switch_override,
                                                                 L_rr_node_indices, to_seg_details,
                                                                 &Fs_clipped, from_rr_node, rr_edges_to_create, scratchpad);
                    }
                }
            }
        }
    }

    return num_conn;
}

static int get_bidir_track_to_chan_seg(const std::vector<int> conn_tracks,
                                       const t_rr_node_indices& L_rr_node_indices,
                                       const int to_chan,
                                       const int to_seg,
                                       const int to_sb,
                                       const t_rr_type to_type,
                                       const t_chan_seg_details* seg_details,
                                       const bool from_is_sblock,
                                       const int from_switch,
                                       const int switch_override,
                                       const enum e_directionality directionality,
                                       const int from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create) {
    unsigned iconn;
    int to_track, to_node, to_switch, num_conn, to_x, to_y, i;
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
        to_node = get_rr_node_index(L_rr_node_indices, to_x, to_y, to_type, to_track);

        if (to_node == OPEN) {
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
            rr_edges_to_create.emplace_back(from_rr_node, to_node, switch_types[i]);
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
static int get_track_to_chan_seg(const int from_wire,
                                 const int to_chan,
                                 const int to_seg,
                                 const t_rr_type to_chan_type,
                                 const e_side from_side,
                                 const e_side to_side,
                                 const int switch_override,
                                 const t_rr_node_indices& L_rr_node_indices,
                                 t_sb_connection_map* sb_conn_map,
                                 const int from_rr_node,
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
            int to_node = get_rr_node_index(L_rr_node_indices, to_x, to_y, to_chan_type, to_wire);

            if (to_node == OPEN) {
                continue;
            }

            /* Get the index of the switch connecting the two wires */
            int src_switch = conn_vector[iconn].switch_ind;

            //Apply any switch overrides
            if (should_apply_switch_override(switch_override)) {
                src_switch = switch_override;
            }

            rr_edges_to_create.emplace_back(from_rr_node, to_node, src_switch);
            ++edge_count;

            auto& device_ctx = g_vpr_ctx.device();

            if (device_ctx.arch_switch_inf[src_switch].directionality() == BI_DIRECTIONAL) {
                //Add reverse edge since bi-directional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, src_switch);
                ++edge_count;
            }
        }
    } else {
        /* specified sb_conn_map entry does not exist -- do nothing */
    }
    return edge_count;
}

static int get_unidir_track_to_chan_seg(const int from_track,
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
                                        const t_rr_node_indices& L_rr_node_indices,
                                        const t_chan_seg_details* seg_details,
                                        bool* Fs_clipped,
                                        const int from_rr_node,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        t_opin_connections_scratchpad* scratchpad) {
    int num_labels = 0;
    std::vector<int>& mux_labels = scratchpad->scratch[0];

    /* x, y coords for get_rr_node lookups */
    int to_x = (CHANX == to_type ? to_seg : to_chan);
    int to_y = (CHANX == to_type ? to_chan : to_seg);
    int sb_x = (CHANX == to_type ? to_sb : to_chan);
    int sb_y = (CHANX == to_type ? to_chan : to_sb);
    int max_len = (CHANX == to_type ? grid.width() : grid.height()) - 2; //-2 for no perimeter channels

    enum e_direction to_dir = DEC_DIRECTION;
    if (to_sb < to_seg) {
        to_dir = INC_DIRECTION;
    }

    *Fs_clipped = false;

    /* get list of muxes to which we can connect */
    int dummy;
    label_wire_muxes(to_chan, to_seg, seg_details, UNDEFINED, max_len,
                     to_dir, max_chan_width, false, &mux_labels, &num_labels, &dummy);

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

            int to_node = get_rr_node_index(L_rr_node_indices, to_x, to_y, to_type, to_track);

            if (to_node == OPEN) {
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
            rr_edges_to_create.emplace_back(from_rr_node, to_node, iswitch);
            ++count;

            auto& device_ctx = g_vpr_ctx.device();
            if (device_ctx.arch_switch_inf[iswitch].directionality() == BI_DIRECTIONAL) {
                //Add reverse edge since bi-directional
                rr_edges_to_create.emplace_back(to_node, from_rr_node, iswitch);
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
                                             const int max_chan_width) {
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
    VTR_ASSERT(max_chan_width >= 0);

    t_sblock_pattern sblock_pattern({{
                                        grid.width() - 1,
                                        grid.height() - 1,
                                        4, //From side
                                        4, //To side
                                        size_t(max_chan_width),
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
                                t_sblock_pattern& sblock_pattern,
                                t_opin_connections_scratchpad* scratchpad) {
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
    VTR_ASSERT(scratchpad->scratch.size() == NUM_SIDES * 2);
    std::array<std::vector<int>*, NUM_SIDES> wire_mux_on_track;
    std::array<std::vector<int>*, NUM_SIDES> incoming_wire_label;
    int num_incoming_wires[NUM_SIDES];
    int num_ending_wires[NUM_SIDES];
    int num_wire_muxes[NUM_SIDES];

    wire_mux_on_track.fill(nullptr);
    incoming_wire_label.fill(nullptr);

    /* "Label" the wires around the switch block by connectivity. */
    for (e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
        /* Assume the channel segment doesn't exist. */
        wire_mux_on_track[side] = nullptr;
        incoming_wire_label[side] = nullptr;
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

        const t_chan_seg_details* seg_details = (vert ? chan_details_y[chan][seg] : chan_details_x[seg][chan]).data();
        if (seg_details[0].length() <= 0)
            continue;

        /* Figure out all the tracks on a side that are ending and the
         * ones that are passing through and have a SB. */
        enum e_direction end_dir = (pos_dir ? DEC_DIRECTION : INC_DIRECTION);
        incoming_wire_label[side] = &scratchpad->scratch[NUM_SIDES + side];
        label_incoming_wires(chan, seg, sb_seg,
                             seg_details, chan_len, end_dir, nodes_per_chan->max,
                             incoming_wire_label[side],
                             &num_incoming_wires[side],
                             &num_ending_wires[side]);

        /* Figure out all the tracks on a side that are starting. */
        int dummy;
        enum e_direction start_dir = (pos_dir ? INC_DIRECTION : DEC_DIRECTION);
        wire_mux_on_track[side] = &scratchpad->scratch[side];
        label_wire_muxes(chan, seg,
                         seg_details, UNDEFINED, chan_len, start_dir, nodes_per_chan->max,
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

        if (incoming_wire_label[side_cw]) {
            for (int ichan = 0; ichan < nodes_per_chan->max; ichan++) {
                int itrack = ichan;
                if (side_cw == TOP || side_cw == BOTTOM) {
                    itrack = ichan % nodes_per_chan->y_list[i];
                } else if (side_cw == RIGHT || side_cw == LEFT) {
                    itrack = ichan % nodes_per_chan->x_list[j];
                }

                if ((*incoming_wire_label[side_cw])[itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((enum e_side)side_cw,
                                                            (enum e_side)to_side,
                                                            (*incoming_wire_label[side_cw])[ichan],
                                                            switch_block_type,
                                                            num_wire_muxes[to_side]);

                    if (sblock_pattern[i][j][side_cw][to_side][itrack][0] == UN_SET) {
                        sblock_pattern[i][j][side_cw][to_side][itrack][0] = mux;
                    } else if (sblock_pattern[i][j][side_cw][to_side][itrack][2] == UN_SET) {
                        sblock_pattern[i][j][side_cw][to_side][itrack][2] = mux;
                    }
                }
            }
        }

        if (incoming_wire_label[side_ccw]) {
            for (int ichan = 0; ichan < nodes_per_chan->max; ichan++) {
                int itrack = ichan;
                if (side_ccw == TOP || side_ccw == BOTTOM) {
                    itrack = ichan % nodes_per_chan->y_list[i];
                } else if (side_ccw == RIGHT || side_ccw == LEFT) {
                    itrack = ichan % nodes_per_chan->x_list[j];
                }

                if ((*incoming_wire_label[side_ccw])[itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((enum e_side)side_ccw,
                                                            (enum e_side)to_side,
                                                            (*incoming_wire_label[side_ccw])[ichan],
                                                            switch_block_type, num_wire_muxes[to_side]);

                    if (sblock_pattern[i][j][side_ccw][to_side][itrack][0] == UN_SET) {
                        sblock_pattern[i][j][side_ccw][to_side][itrack][0] = mux;
                    } else if (sblock_pattern[i][j][side_ccw][to_side][itrack][2] == UN_SET) {
                        sblock_pattern[i][j][side_ccw][to_side][itrack][2] = mux;
                    }
                }
            }
        }

        if (incoming_wire_label[side_opp]) {
            for (int itrack = 0; itrack < nodes_per_chan->max; itrack++) {
                /* not ending wire nor passing wire with sblock */
                if ((*incoming_wire_label[side_opp])[itrack] != UN_SET) {
                    /* corner sblocks for sure have no opposite channel segments so don't care about them */
                    if ((*incoming_wire_label[side_opp])[itrack] < num_ending_wires[side_opp]) {
                        /* The ending wires in core sblocks form N-to-N assignment problem, so can
                         * use any pattern such as Wilton */
                        /* In the direct connect case, I know for sure the init mux is at the same track #
                         * as this ending wire, but still need to find the init mux label for Fs > 3 */
                        int mux = find_label_of_track(wire_mux_on_track[to_side]->data(),
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

static void label_wire_muxes(const int chan_num,
                             const int seg_num,
                             const t_chan_seg_details* seg_details,
                             const int seg_type_index,
                             const int max_len,
                             const enum e_direction dir,
                             const int max_chan_width,
                             const bool check_cb,
                             std::vector<int>* labels_ptr,
                             int* num_wire_muxes,
                             int* num_wire_muxes_cb_restricted) {
    /* Labels the muxes on that side (seg_num, chan_num, direction). The returned array
     * maps a label to the actual track #: array[0] = <the track number of the first/lowest mux>
     * This routine orders wire muxes by their natural order, i.e. track #
     * If seg_type_index == UNDEFINED, all segments in the channel are considered. Otherwise this routine
     * only looks at segments that belong to the specified segment type. */

    std::vector<int>& labels = *labels_ptr;
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
            if (DEC_DIRECTION == seg_details[itrack].direction()) {
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
                                 const enum e_direction dir,
                                 const int max_chan_width,
                                 std::vector<int>* labels_ptr,
                                 int* num_incoming_wires,
                                 int* num_ending_wires) {
    /* Labels the incoming wires on that side (seg_num, chan_num, direction).
     * The returned array maps a track # to a label: array[0] = <the new hash value/label for track 0>,
     * the labels 0,1,2,.. identify consecutive incoming wires that have sblock (passing wires with sblock and ending wires) */

    std::vector<int>& labels = *labels_ptr;
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
                if (DEC_DIRECTION == seg_details[itrack].direction()) {
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

static int find_label_of_track(int* wire_mux_on_track,
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

static int should_create_switchblock(const DeviceGrid& grid, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type) {
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

    auto blk_type = grid[x_coord][y_coord].type;
    int width_offset = grid[x_coord][y_coord].width_offset;
    int height_offset = grid[x_coord][y_coord].height_offset;

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

/*
 * Utility
 */
void add_to_rr_node_indices(t_rr_node_indices& rr_node_indices, const t_rr_graph_storage& rr_nodes, int inode) {
    const t_rr_node& rr_node = rr_nodes[inode];

    for (int x = rr_node.xlow(); x <= rr_node.xhigh(); ++x) {
        for (int y = rr_node.ylow(); y <= rr_node.yhigh(); ++y) {
            auto rr_type = rr_node.type();
            if (rr_type == IPIN || rr_type == OPIN) {
                insert_at_ptc_index(rr_node_indices[rr_node.type()][x][y][rr_node.side()], rr_node.ptc_num(), inode);
            } else if (rr_type == CHANX) {
                //CHANX uses odd swapped x/y indices....
                insert_at_ptc_index(rr_node_indices[rr_node.type()][y][x][0], rr_node.ptc_num(), inode);
            } else {
                insert_at_ptc_index(rr_node_indices[rr_node.type()][x][y][0], rr_node.ptc_num(), inode);
            }
        }
    }
}

void insert_at_ptc_index(std::vector<int>& rr_indices, int ptc, int inode) {
    if (ptc >= int(rr_indices.size())) {
        rr_indices.resize(ptc + 1, OPEN); //Functional, but inefficient allocations...
    }

    if (rr_indices[ptc] == inode) {
        return;
    }

    VTR_ASSERT(rr_indices[ptc] == OPEN);
    rr_indices[ptc] = inode;
}
